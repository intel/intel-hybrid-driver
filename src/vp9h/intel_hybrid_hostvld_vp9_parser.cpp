/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *     Zhao Yakui <yakui.zhao@intel.com>
 *
 */

#include <stdlib.h>
#include "intel_hybrid_hostvld_vp9_parser.h"
#include "intel_hybrid_hostvld_vp9_parser_tables.h"
#include "intel_hybrid_hostvld_vp9_context.h"
#include "intel_hybrid_hostvld_vp9_engine.h"

#define VP9_INVALID_MV_VALUE    0x80008000

#define VP9_L_CTX(pSyntax, Default)\
    (pMbInfo->bLeftValid? (pSyntax)[pMbInfo->iLCtxOffset]: (Default))
#define VP9_A_CTX(pSyntax, Default)\
    (pMbInfo->bAboveValid? (pSyntax)[pMbInfo->iACtxOffset]: (Default))

#define VP9_PROP8x8(pSyntax, Value)\
    Intel_HostvldVp9_PropagateByte(g_Vp9Propagate8x8, BlkSize, (pSyntax), (Value))
#define VP9_PROP4x4(pSyntax, Value)\
    Intel_HostvldVp9_PropagateByte(g_Vp9Propagate4x4, BlkSize, (pSyntax), (Value))
#define VP9_PROP8x8_WORD(pSyntax, Value)\
    Intel_HostvldVp9_PropagateWord(g_Vp9Propagate8x8, BlkSize, (pSyntax), (Value))
#define VP9_PROP8x8_DWORD(pSyntax, Value)\
    Intel_HostvldVp9_PropagateDWord(g_Vp9Propagate8x8, BlkSize, (pSyntax), (Value))
#define VP9_PROP4x4_QWORD(pSyntax, Value)\
    Intel_HostvldVp9_PropagateQWord(g_Vp9Propagate4x4, BlkSize, (pSyntax), (Value))

#define VP9_IS_INTER(Position)\
    (ReferenceFrame##Position[0] > VP9_REF_FRAME_INTRA)

#define VP9_IS_INTRA(Position)\
    (ReferenceFrame##Position[0] < VP9_REF_FRAME_LAST)

#define VP9_IS_COMPOUND(Position)\
    (ReferenceFrame##Position[1] > VP9_REF_FRAME_INTRA)

#define VP9_IS_SINGLE(Position)\
    (ReferenceFrame##Position[1] < VP9_REF_FRAME_LAST)

#define VP9_IN_TILE_COLUMN(PosX, PosY) \
    (!(((PosY) < 0) || \
    ((PosY) >= (INT)pFrameInfo->dwB8Rows) || \
    ((PosX) < (INT)pMbInfo->pCurrTile->dwTileLeft) || \
    ((PosX) >= (INT)pMbInfo->pCurrTile->dwTileLeft + (INT)pMbInfo->pCurrTile->dwTileWidth)))

#define VP9_GET_ZORDER_OFFSET_B8(PosX, PosY) \
    (INT)((PosY & ~7) * pFrameInfo->dwMbStride + ((PosX & ~7) << VP9_LOG2_B64_SIZE_IN_B8) + \
    g_Vp9SB_ZOrder8X8[PosY & 7][PosX & 7] - pMbInfo->dwMbOffset - pMbInfo->i8ZOrder)

// Read one bit
#define INTEL_HOSTVLD_VP9_READ_COEFF_BIT(iProb)      \
do                                                      \
{                                                       \
    uiSplit       = ((uiRange * iProb) + (BAC_ENG_PROB_RANGE - iProb)) >> BAC_ENG_PROB_BITS; \
    BacSplitValue = (INTEL_HOSTVLD_VP9_BAC_VALUE)uiSplit << (BAC_ENG_VALUE_BITS - BAC_ENG_PROB_BITS); \
                                                        \
    if (iCount < 8)                                     \
    {                                                   \
        pBacEngine->BacValue = BacValue;                \
        pBacEngine->iCount   = iCount;                  \
        Intel_HostvldVp9_BacEngineFill(pBacEngine);   \
        BacValue = pBacEngine->BacValue;                \
        iCount   = pBacEngine->iCount;                  \
    }                                                   \
                                                        \
    if (BacValue >= BacSplitValue)                      \
    {                                                   \
        iBit     = 1;                                   \
        uiRange  -= uiSplit;                            \
        BacValue -= BacSplitValue;                      \
    }                                                   \
    else                                                \
    {                                                   \
        iBit     = 0;                                   \
        uiRange  = uiSplit;                             \
    }                                                   \
                                                        \
    uiShift = g_Vp9NormTable[uiRange];                  \
    uiRange  <<= uiShift;                               \
    BacValue <<= uiShift;                               \
    iCount -= uiShift;                                  \
} while (0)

// Write Coeff and Continue to next coeff
#define VP9_WRITE_COEF_CONTINUE(val, token)                                       \
{                                                                                 \
    uiSplit       = (uiRange + 1) >> 1;                                           \
    BacSplitValue = (INTEL_HOSTVLD_VP9_BAC_VALUE)uiSplit << (BAC_ENG_VALUE_BITS - BAC_ENG_PROB_BITS); \
                                                                                  \
    if (iCount < 8)                                                               \
    {                                                                             \
        pBacEngine->BacValue = BacValue;                                          \
        pBacEngine->iCount   = iCount;                                            \
        Intel_HostvldVp9_BacEngineFill(pBacEngine);                             \
        BacValue = pBacEngine->BacValue;                                          \
        iCount   = pBacEngine->iCount;                                            \
    }                                                                             \
                                                                                  \
    if (BacValue >= BacSplitValue)                                                \
    {                                                                             \
        pCoeffAddr[pScan[CoeffIdx]] = (INT16)(-val);                              \
        uiRange  -= uiSplit;                                                      \
        BacValue -= BacSplitValue;                                                \
    }                                                                             \
    else                                                                          \
    {                                                                             \
        pCoeffAddr[pScan[CoeffIdx]] = (INT16)val;                                 \
        uiRange  = uiSplit;                                                       \
    }                                                                             \
                                                                                  \
    uiShift = g_Vp9NormTable[uiRange];                                            \
    uiRange  <<= uiShift;                                                         \
    BacValue <<= uiShift;                                                         \
    iCount -= uiShift;                                                            \
    pTileInfo->TokenCache[pScan[CoeffIdx]] = g_Vp9PtEnergyClass[token];           \
    ++CoeffIdx;                                                                   \
    continue;                                                                     \
}

// Parse Category Coeff and Continue to next
#define VP9_PARSE_CAT_COEF_CONTINUE(min_val, token)                               \
{                                                                                 \
    val = 0;                                                                      \
    while (*pCatProb)                                                             \
    {                                                                             \
        val = (val << 1) | INTEL_HOSTVLD_VP9_READ_BIT(*pCatProb++);            \
    }                                                                             \
    val += min_val;                                                               \
    pCoeffAddr[pScan[CoeffIdx]] = INTEL_HOSTVLD_VP9_READ_ONE_BIT ? -val : val; \
    pTileInfo->TokenCache[pScan[CoeffIdx]] = g_Vp9PtEnergyClass[token];           \
    ++CoeffIdx;                                                                   \
    uiRange  = pBacEngine->uiRange;                                               \
    BacValue = pBacEngine->BacValue;                                              \
    iCount   = pBacEngine->iCount;                                                \
    continue;                                                                     \
}

// Merge two count structures
#define VP9_MERGE_COUNT_ARRAY(Array)                        \
do                                                          \
{                                                           \
    puiBaseCount = (PUINT)pBaseCount->Array;                \
    puiCount = (PUINT)pCount->Array;                        \
    for (i = 0; i < (sizeof(pBaseCount->Array) >> 2); i++)  \
    {                                                       \
        puiBaseCount[i] += puiCount[i];                     \
    }                                                       \
} while (0)

// Coeff Parser Huffman Tree related
#define VP9_EOB_CONTEXT_NODE            0
#define VP9_ZERO_CONTEXT_NODE           1
#define VP9_ONE_CONTEXT_NODE            2
#define VP9_LOW_VAL_CONTEXT_NODE        0
#define VP9_TWO_CONTEXT_NODE            1
#define VP9_THREE_CONTEXT_NODE          2
#define VP9_HIGH_LOW_CONTEXT_NODE       3
#define VP9_CAT_ONE_CONTEXT_NODE        4
#define VP9_CAT_THREEFOUR_CONTEXT_NODE  5
#define VP9_CAT_THREE_CONTEXT_NODE      6
#define VP9_CAT_FIVE_CONTEXT_NODE       7

#define VP9_CAT1_MIN_VAL    5
#define VP9_CAT2_MIN_VAL    7
#define VP9_CAT3_MIN_VAL   11
#define VP9_CAT4_MIN_VAL   19
#define VP9_CAT5_MIN_VAL   35
#define VP9_CAT6_MIN_VAL   67

#define VP9_CAT1_PROB0    159
#define VP9_CAT2_PROB0    145
#define VP9_CAT2_PROB1    165

#define VP9_CAT3_PROB0 140
#define VP9_CAT3_PROB1 148
#define VP9_CAT3_PROB2 173

#define VP9_CAT4_PROB0 135
#define VP9_CAT4_PROB1 140
#define VP9_CAT4_PROB2 155
#define VP9_CAT4_PROB3 176

#define VP9_CAT5_PROB0 130
#define VP9_CAT5_PROB1 134
#define VP9_CAT5_PROB2 141
#define VP9_CAT5_PROB3 157
#define VP9_CAT5_PROB4 180

#define VP9_PIVOT_NODE 2

#define VP9_LEFT_BORDER_LUMA_MASK    0x1111111111111111
#define VP9_ABOVE_BORDER_LUMA_MASK   0x000000ff000000ff
#define VP9_LEFT_BORDER_CHROMA_MASK  0x1111
#define VP9_ABOVE_BORDER_CHROMA_MASK 0x000f

VAStatus Intel_HostvldVp9_ParseCompressedHeader(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState)
{
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_VP9_PIC_PARAMS            pPicParams;
    VAStatus                          eStatus = VA_STATUS_SUCCESS;


    pBacEngine = &pFrameState->BacEngine;
    pFrameInfo = &pFrameState->FrameInfo;
    pPicParams = pFrameInfo->pPicParams;

    // Transform Mode
    if (!pPicParams->PicFlags.fields.LosslessFlag)
    {
        pFrameInfo->TxMode = (INTEL_HOSTVLD_VP9_TX_MODE)INTEL_HOSTVLD_VP9_READ_BITS(2);
        if (pFrameInfo->TxMode == ALLOW_32X32)
        {
            INT TxMode = pFrameInfo->TxMode;
            TxMode += INTEL_HOSTVLD_VP9_READ_ONE_BIT;
            pFrameInfo->TxMode = (INTEL_HOSTVLD_VP9_TX_MODE)TxMode;
        }
    }
    else
    {
        pFrameInfo->TxMode = ONLY_4X4;
    }

    // Read probabilities
    Intel_HostvldVp9_ReadProbabilities(pFrameInfo->pContext, pFrameInfo, pBacEngine);

finish:
    return eStatus;
}

static inline DWORD Intel_HostvldVp9_GetTileOffset(
    DWORD   dwTileIdx,
    DWORD   dwB8Number, 
    DWORD   dwTileNumber, 
    DWORD   dwLog2TileNumber)
{
    DWORD dwB64Number = ALIGN(dwB8Number, VP9_B64_SIZE_IN_B8) >> VP9_LOG2_B64_SIZE_IN_B8;
    return ((dwTileIdx * dwB64Number) >> dwLog2TileNumber) << VP9_LOG2_B64_SIZE_IN_B8;
}


static inline VOID CMOS_FillMemory(PVOID pDestination, size_t stLength, UINT8 bFill)
{
    if(pDestination != NULL)
    {
        memset(pDestination, bFill, stLength);
    }
}

static inline VOID CMOS_ZeroMemory(PVOID pDestination, size_t stLength)
{

    if(pDestination != NULL)
    {
        memset(pDestination, 0, stLength);
    }
}

static VOID Intel_HostvldVp9_UpdatePartitionContext(
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo,
    PINTEL_HOSTVLD_VP9_TILE_INFO     pTileInfo,
    DWORD                               dwB8X, 
    DWORD                               dwB8Y, 
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     SubBlockSize,
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlockSize)
{
    PUINT8 pbAboveContext, pbLeftContext;
    INT    iContextCount = 1 << (BlockSize - BLOCK_8X8);
    INT    iOffset       = BLOCK_64X64 - BlockSize;
    BOOL   bIsSpilit     = BlockSize > SubBlockSize;
    INT    iAboveValue   = 0x0F - (bIsSpilit || (SubBlockSize >= BLOCK_4X8));
    INT    iLeftValue    = 0x0F - (bIsSpilit || ((SubBlockSize <= BLOCK_64X32) && (SubBlockSize >= BLOCK_8X4)));

    pbAboveContext  = pFrameInfo->pSegContextAbove + dwB8X;
    pbLeftContext   = pTileInfo->SegContextLeft + (dwB8Y & (VP9_B64_SIZE_IN_B8 - 1));

    CMOS_FillMemory(pbAboveContext, iContextCount, ~(iAboveValue << iOffset));
    CMOS_FillMemory(pbLeftContext,  iContextCount, ~(iLeftValue << iOffset));
}

static inline VOID Intel_HostvldVp9_InitTokenParser(
    PINTEL_HOSTVLD_VP9_TILE_STATE   pTileState)
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER pOutputBuffer;
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pVideoBuffer;
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    INT                                 iOffset;
    DWORD                               dwMbStride;

    pFrameState     = pTileState->pFrameState;
    pOutputBuffer   = pFrameState->pOutputBuffer;
    pVideoBuffer    = pFrameState->pVideoBuffer;
    pFrameInfo      = &pFrameState->FrameInfo;
    pMbInfo         = &pTileState->MbInfo;
    dwMbStride      = pFrameInfo->dwMbStride;
    iOffset         = pMbInfo->pCurrTile->dwTileLeft << VP9_LOG2_B64_SIZE_IN_B8;

    // 8x8 token
    pMbInfo->pSegmentId      = pOutputBuffer->SegmentIndex.pu8Buffer                                      + iOffset;
    pMbInfo->pLastSegmentId  = pFrameState->pLastSegIdBuf->pu8Buffer                                      + iOffset;
    pMbInfo->pSkipCoeff      = pOutputBuffer->SkipFlag.pu8Buffer                                          + iOffset;
    pMbInfo->pIsInter        = pOutputBuffer->InterIntraFlag.pu8Buffer                                    + iOffset;
    pMbInfo->pBlockSize      = pOutputBuffer->BlockSize.pu8Buffer                                         + iOffset;
    pMbInfo->pReferenceFrame = pOutputBuffer->ReferenceFrame.pu16Buffer                                   + iOffset;
    pMbInfo->pFilterType     = pOutputBuffer->FilterType.pu8Buffer                                        + iOffset;
    pMbInfo->pTxSizeLuma     = pOutputBuffer->TransformSize[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer   + iOffset;
    pMbInfo->pTxSizeChroma   = pOutputBuffer->TransformSize[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer  + iOffset;
    pMbInfo->pQPLuma         = pOutputBuffer->QP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu32Buffer             + iOffset;
    pMbInfo->pQPChroma       = pOutputBuffer->QP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu32Buffer            + iOffset;
    pMbInfo->pPredModeChroma = pOutputBuffer->PredictionMode[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer + iOffset;
    pMbInfo->pPrevRefFrame   = pVideoBuffer->PrevReferenceFrame.pu16Buffer                                + iOffset;

    iOffset <<= 2;
    // 4x4 token
    pMbInfo->pTxTypeLuma   = pOutputBuffer->TransformType.pu8Buffer                                    + iOffset;
    pMbInfo->pMotionVector = (PINTEL_HOSTVLD_VP9_MV)pOutputBuffer->MotionVector.pu32Buffer          + (iOffset << 1); // 2 MVs per block
    pMbInfo->pPredModeLuma = pOutputBuffer->PredictionMode[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer + iOffset;
    pMbInfo->pPrevMv       = (PINTEL_HOSTVLD_VP9_MV)pVideoBuffer->PrevMotionVector.pu32Buffer       + (iOffset << 1); // 2 MVs per block

    pMbInfo->i8ZOrder   = 0;
    pMbInfo->dwLineDist = dwMbStride - ALIGN(pMbInfo->pCurrTile->dwTileWidth, VP9_B64_SIZE_IN_B8);
}

static inline VOID Intel_HostvldVp9_UpdateTokenParser(
    PINTEL_HOSTVLD_VP9_FRAME_INFO pFrameInfo,
    PINTEL_HOSTVLD_VP9_MB_INFO    pMbInfo)
{
    INT iSB_X   = pMbInfo->iMbPosInB64X;
    INT iSB_Y   = pMbInfo->iMbPosInB64Y;
    INT iZOrder = g_Vp9SB_ZOrder8X8[iSB_Y][iSB_X];
    INT iOffset = iZOrder - pMbInfo->i8ZOrder;

    pMbInfo->iLCtxOffset = g_Vp9LeftOffset[iSB_X];
    pMbInfo->iACtxOffset = (iSB_Y > 0)? g_Vp9AboveOffset[iSB_Y]:
        (g_Vp9AboveOffset[iSB_Y] - (pFrameInfo->dwMbStride << VP9_LOG2_B64_SIZE_IN_B8));

    if (iZOrder == 0)
    {
        iOffset += VP9_B64_SIZE;
        if (!pMbInfo->bLeftValid)
        {
            iOffset += (pMbInfo->dwLineDist << VP9_LOG2_B64_SIZE_IN_B8);
        }
    }

    // 8x8 token
    pMbInfo->pSegmentId      += iOffset;
    pMbInfo->pLastSegmentId  += iOffset;
    pMbInfo->pSkipCoeff      += iOffset;
    pMbInfo->pIsInter        += iOffset;
    pMbInfo->pBlockSize      += iOffset;
    pMbInfo->pTxSizeLuma     += iOffset;
    pMbInfo->pTxSizeChroma   += iOffset;
    pMbInfo->pQPLuma         += iOffset;
    pMbInfo->pQPChroma       += iOffset;
    pMbInfo->pPredModeChroma += iOffset;
    pMbInfo->pReferenceFrame += iOffset;
    pMbInfo->pFilterType     += iOffset;
    pMbInfo->pPrevRefFrame   += iOffset;

    iOffset <<= 2;
    // 4x4 token
    pMbInfo->pTxTypeLuma     += iOffset;
    pMbInfo->pPredModeLuma   += iOffset;
    pMbInfo->pMotionVector   += iOffset << 1;   // 2 MVs per block
    pMbInfo->pPrevMv         += iOffset << 1;   // 2 MVs per block

    pMbInfo->i8ZOrder = (UINT8)iZOrder;
}

static inline VOID Intel_HostvldVp9_ReadIntraMode_KeyFrmY(
    PINTEL_HOSTVLD_VP9_MB_INFO     pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE  pBacEngine,
    UINT8                             ModeAbove,
    UINT8                             ModeLeft)
{
    pMbInfo->ePredMode = (INTEL_HOSTVLD_VP9_MB_PRED_MODE)INTEL_HOSTVLD_VP9_READ_TREE(
        g_Vp9TknTreeIntra_KF_Y[ModeAbove][ModeLeft]);
}

static inline VOID Intel_HostvldVp9_ReadIntraMode_KeyFrmUV(
    PINTEL_HOSTVLD_VP9_MB_INFO     pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE  pBacEngine,
    UINT8                             ModeY)
{
    pMbInfo->ePredModeChroma = (INTEL_HOSTVLD_VP9_MB_PRED_MODE)INTEL_HOSTVLD_VP9_READ_TREE(
        g_Vp9TknTreeIntra_KF_UV[ModeY]);
}

static inline INTEL_HOSTVLD_VP9_MB_PRED_MODE Intel_HostvldVp9_ReadIntraMode_InterFrmY(
    PINTEL_HOSTVLD_VP9_TILE_STATE  pTileState,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE  pBacEngine,
    INTEL_HOSTVLD_VP9_BLOCK_SIZE   BlockSize)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext;
    PINTEL_HOSTVLD_VP9_COUNT         pCount;
    INT                                 iPredMode, iBlockSizeGroup;

    pFrameInfo      = &pTileState->pFrameState->FrameInfo;
    pContext        = pFrameInfo->pContext;
    pCount          = &pTileState->Count;
    iBlockSizeGroup = g_Vp9BlockSizeGroup[BlockSize];
    iPredMode       = INTEL_HOSTVLD_VP9_READ_TREE(pContext->ModeTree_Y[iBlockSizeGroup]);

    pCount->IntraModeCounts_Y[iBlockSizeGroup][iPredMode] += pFrameInfo->bFrameParallelDisabled;

    return (INTEL_HOSTVLD_VP9_MB_PRED_MODE)iPredMode;
}

static inline INTEL_HOSTVLD_VP9_MB_PRED_MODE Intel_HostvldVp9_ReadIntraMode_InterFrmUV(
    PINTEL_HOSTVLD_VP9_TILE_STATE  pTileState,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE  pBacEngine,
    UINT8                             ModeY)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext;
    PINTEL_HOSTVLD_VP9_COUNT         pCount;
    INT                                 iPredMode;

    pFrameInfo      = &pTileState->pFrameState->FrameInfo;
    pContext        = pFrameInfo->pContext;
    pCount          = &pTileState->Count;
    iPredMode       = INTEL_HOSTVLD_VP9_READ_TREE(pContext->ModeTree_UV[ModeY]);

    pCount->IntraModeCounts_UV[ModeY][iPredMode] += pFrameInfo->bFrameParallelDisabled;

    return (INTEL_HOSTVLD_VP9_MB_PRED_MODE)iPredMode;
}

static VOID Intel_HostvldVp9_PropagateByte(
    PVP9_PROPAGATE                  pPropagate,
    INTEL_HOSTVLD_VP9_BLOCK_SIZE BlkSize,
    register PUINT8                 pDst,
    UINT8                           ui8Value)
{
    VP9_PROPAGATE Propagate = pPropagate[BlkSize];
    register UINT8 ui8Copy  = Propagate.ui8Copy;
    do
    {
        *(pDst++) = ui8Value;
    } while (ui8Copy--);

    if (Propagate.ui8Jump)
    {
        pDst += Propagate.ui8Jump;
        ui8Copy = Propagate.ui8Copy;
        do
        {
            *(pDst++) = ui8Value;
        } while (ui8Copy--);
    }
}

static VOID Intel_HostvldVp9_PropagateWord(
    PVP9_PROPAGATE                  pPropagate,
    INTEL_HOSTVLD_VP9_BLOCK_SIZE BlkSize,
    register PUINT16                pDst,
    UINT16                          ui16Value)
{
    VP9_PROPAGATE  Propagate = pPropagate[BlkSize];
    register UINT8 ui8Copy   = Propagate.ui8Copy;
    do
    {
        *(pDst++) = ui16Value;
    } while (ui8Copy--);

    if (Propagate.ui8Jump)
    {
        pDst += Propagate.ui8Jump;
        ui8Copy = Propagate.ui8Copy;
        do
        {
            *(pDst++) = ui16Value;
        } while (ui8Copy--);
    }
}

static VOID Intel_HostvldVp9_PropagateDWord(
    PVP9_PROPAGATE                  pPropagate,
    INTEL_HOSTVLD_VP9_BLOCK_SIZE BlkSize,
    register PUINT32                pDst,
    UINT32                          ui32Value)
{
    VP9_PROPAGATE Propagate = pPropagate[BlkSize];
    register UINT8 ui8Copy  = Propagate.ui8Copy;
    do
    {
        *(pDst++) = ui32Value;
    } while (ui8Copy--);

    if (Propagate.ui8Jump)
    {
        pDst += Propagate.ui8Jump;
        ui8Copy = Propagate.ui8Copy;
        do
        {
            *(pDst++) = ui32Value;
        } while (ui8Copy--);
    }
}

static VOID Intel_HostvldVp9_PropagateQWord(
    PVP9_PROPAGATE                  pPropagate,
    INTEL_HOSTVLD_VP9_BLOCK_SIZE BlkSize,
    register PUINT64                pDst,
    UINT64                          ui64Value)
{
    VP9_PROPAGATE Propagate = pPropagate[BlkSize];
    register UINT8 ui8Copy  = Propagate.ui8Copy;
    do
    {
        *(pDst++) = ui64Value;
    } while (ui8Copy--);

    if (Propagate.ui8Jump)
    {
        pDst += Propagate.ui8Jump;
        ui8Copy = Propagate.ui8Copy;
        do
        {
            *(pDst++) = ui64Value;
        } while (ui8Copy--);
    }
}

static VOID Intel_HostvldVp9_ParseTransformSize(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine,
    UINT8                               ui8LSkip,
    UINT8                               ui8ASkip)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    INTEL_HOSTVLD_VP9_TX_PROB_TABLE  TxProbTable;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlkSize;
    INTEL_HOSTVLD_VP9_TX_SIZE        MaxTxSize;
    PUINT8                              pProbs;
    UINT8                               ui8Ctx, ui8LCtx, ui8ACtx, ui8TxSize;

    pFrameInfo = &pTileState->pFrameState->FrameInfo;
    BlkSize    = pMbInfo->BlockSize;
    MaxTxSize  = g_Vp9MaxTxSizeTable[BlkSize];

    ui8TxSize = MIN(MaxTxSize, g_Vp9TxSizeModeLimit[pFrameInfo->TxMode]);
    if ((!pMbInfo->bIsSkipped || !pMbInfo->bIsInter) && 
        (pFrameInfo->TxMode == TX_MODE_SELECT)       && 
        (pMbInfo->iB4Number >= 4))
    {
        ui8LCtx = ui8LSkip? MaxTxSize: VP9_L_CTX(pMbInfo->pTxSizeLuma, MaxTxSize);
        ui8ACtx = ui8ASkip? MaxTxSize: VP9_A_CTX(pMbInfo->pTxSizeLuma, MaxTxSize);
        ui8Ctx  = ((pMbInfo->bLeftValid? ui8LCtx: ui8ACtx) +
            (pMbInfo->bAboveValid? ui8ACtx: ui8LCtx) > MaxTxSize);

        TxProbTable = pFrameInfo->pContext->TxProbTables[MaxTxSize];
        pProbs      = TxProbTable.pui8ProbTable + ui8Ctx * TxProbTable.uiStride;
        ui8TxSize   = (UINT8)INTEL_HOSTVLD_VP9_READ_BIT(pProbs[TX_4X4]);
        if (ui8TxSize != TX_4X4 && MaxTxSize >= TX_16X16)
        {
            ui8TxSize += (UINT8)INTEL_HOSTVLD_VP9_READ_BIT(pProbs[TX_8X8]);
            if (ui8TxSize != TX_8X8 && MaxTxSize >= TX_32X32)
            {
                ui8TxSize += (UINT8)INTEL_HOSTVLD_VP9_READ_BIT(pProbs[TX_16X16]);
            }
        }

        if (MaxTxSize == TX_8X8)
        {
            pTileState->Count.TxCountSet.Tx_8X8[ui8Ctx][ui8TxSize] += pFrameInfo->bFrameParallelDisabled;
        }
        else if (MaxTxSize == TX_16X16)
        {
            pTileState->Count.TxCountSet.Tx_16X16[ui8Ctx][ui8TxSize] += pFrameInfo->bFrameParallelDisabled;
        }
        else if (MaxTxSize == TX_32X32)
        {
            pTileState->Count.TxCountSet.Tx_32X32[ui8Ctx][ui8TxSize] += pFrameInfo->bFrameParallelDisabled;
        }
    }
    VP9_PROP8x8(pMbInfo->pTxSizeLuma, ui8TxSize);
    ui8TxSize = MIN(ui8TxSize, g_Vp9MaxChromaTxSize[BlkSize]);
    VP9_PROP8x8(pMbInfo->pTxSizeChroma, ui8TxSize);
}

static inline UINT8 Intel_HostvldVp9_GetTxType(
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo)
{
    return (*(pMbInfo->pTxSizeLuma) == TX_4X4 && pFrameInfo->bLossLess)?
        TX_DCT : g_Vp9Mode2TxTypeMap[pMbInfo->ePredMode];
}

VAStatus Intel_HostvldVp9_ParseIntraFrameModeInfo(
    PINTEL_HOSTVLD_VP9_TILE_STATE   pTileState)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo;
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlkSize;
    INTEL_HOSTVLD_VP9_TX_SIZE        MaxTxSize;

    UINT8      ui8Ctx, ui8LCtx, ui8ACtx, ui8LSkip, ui8ASkip;
    UINT8      uiSegId, ui8SkipCoeff, ui8TxType;
    INT        iBlkW4x4, iBlkH4x4, iX4x4, iY4x4;
    INT        iLCtxOffset, iACtxOffset;
    BOOL       bAboveValid, bLeftValid, bRightValid;
    PUINT8     pTxTypeLuma, pPredModeLuma;
    VAStatus eStatus = VA_STATUS_SUCCESS;


    pFrameInfo = &pTileState->pFrameState->FrameInfo;
    pMbInfo    = &pTileState->MbInfo;
    pBacEngine = &pTileState->BacEngine;

    BlkSize   = pMbInfo->BlockSize;
    MaxTxSize = g_Vp9MaxTxSizeTable[BlkSize];

    if (!pMbInfo->bAboveValid && !pMbInfo->bLeftValid)
    {
        Intel_HostvldVp9_InitTokenParser(pTileState);
    }
    else
    {
        Intel_HostvldVp9_UpdateTokenParser(pFrameInfo, pMbInfo);
    }

    pMbInfo->ReferenceFrame[0] = VP9_REF_FRAME_INTRA;
    pMbInfo->ReferenceFrame[1] = VP9_REF_FRAME_INTRA;

    // block size
    VP9_PROP8x8(pMbInfo->pBlockSize, g_Vp9BlockSizeLookup[pMbInfo->BlockSize]);

    // segment id
    uiSegId = 0;
    if (pFrameInfo->ui8SegEnabled && pFrameInfo->ui8SegUpdMap)
    {
        uiSegId = (UINT8)INTEL_HOSTVLD_VP9_READ_TREE(pFrameInfo->pContext->SegmentTree);
        VP9_PROP8x8(pMbInfo->pLastSegmentId, uiSegId);
    }
    VP9_PROP8x8(pMbInfo->pSegmentId, uiSegId);
    VP9_PROP8x8_DWORD(pMbInfo->pQPLuma, pFrameInfo->SegQP[uiSegId][INTEL_HOSTVLD_VP9_YUV_PLANE_Y]);
    VP9_PROP8x8_DWORD(pMbInfo->pQPChroma, pFrameInfo->SegQP[uiSegId][INTEL_HOSTVLD_VP9_YUV_PLANE_UV]);
    pMbInfo->bSegRefSkip =
        pFrameInfo->pSegmentData->SegData[uiSegId].SegmentFlags.fields.SegmentReferenceSkipped;

    // skip coefficient flag
    ui8SkipCoeff = 1;
    ui8LSkip     = VP9_L_CTX(pMbInfo->pSkipCoeff, 0);
    ui8ASkip     = VP9_A_CTX(pMbInfo->pSkipCoeff, 0);
    if (!pMbInfo->bSegRefSkip)
    {
        ui8Ctx       = ui8LSkip + ui8ASkip;
        ui8SkipCoeff = (UINT8)INTEL_HOSTVLD_VP9_READ_BIT(pFrameInfo->pContext->MbSkipProbs[ui8Ctx]);

        pTileState->Count.MbSkipCounts[ui8Ctx][ui8SkipCoeff] += pFrameInfo->bFrameParallelDisabled;
    }
    VP9_PROP8x8(pMbInfo->pSkipCoeff, ui8SkipCoeff);
    pMbInfo->bIsSkipped = ui8SkipCoeff;

    VP9_PROP8x8(pMbInfo->pIsInter, 0);
    pMbInfo->bIsInter = FALSE;

	// transform size
    Intel_HostvldVp9_ParseTransformSize(pTileState, pMbInfo, pBacEngine, ui8LSkip, ui8ASkip);

    // transform type and prediction mode

    iBlkW4x4      = g_Vp9BlkW4x4[BlkSize];
    iBlkH4x4      = g_Vp9BlkH4x4[BlkSize];
    iACtxOffset   = (pMbInfo->iACtxOffset << 2) + 2;
    bAboveValid   = pMbInfo->bAboveValid;
    pTxTypeLuma   = pMbInfo->pTxTypeLuma;
    pPredModeLuma = pMbInfo->pPredModeLuma;
    for (iY4x4 = 0; iY4x4 < 2; iY4x4 += iBlkH4x4)
    {
        iLCtxOffset = (pMbInfo->iLCtxOffset << 2) + 1;
        bLeftValid  = pMbInfo->bLeftValid;
        bRightValid = 1;
        for (iX4x4 = 0; iX4x4 < 2; iX4x4 += iBlkW4x4)
        {
            // No check of IsInter flag for key/intra-only frame
            ui8LCtx = (bLeftValid?  pPredModeLuma[iLCtxOffset]: PRED_MD_DC);
            ui8ACtx = (bAboveValid? pPredModeLuma[iACtxOffset]: PRED_MD_DC);
            Intel_HostvldVp9_ReadIntraMode_KeyFrmY(pMbInfo, pBacEngine, ui8ACtx, ui8LCtx);

            ui8TxType = Intel_HostvldVp9_GetTxType(pFrameInfo, pMbInfo);

            VP9_PROP4x4(pPredModeLuma++, pMbInfo->ePredMode);
            VP9_PROP4x4(pTxTypeLuma++, ui8TxType);

            iLCtxOffset = -1;
            bLeftValid  = 1;
            bRightValid = pMbInfo->bRightValid;
        }

        iACtxOffset   = -2;
        bAboveValid   = 1;
        pTxTypeLuma   = pMbInfo->pTxTypeLuma   + 2;
        pPredModeLuma = pMbInfo->pPredModeLuma + 2;
    }

    Intel_HostvldVp9_ReadIntraMode_KeyFrmUV(pMbInfo, pBacEngine, pMbInfo->ePredMode);
    VP9_PROP8x8(pMbInfo->pPredModeChroma, pMbInfo->ePredModeChroma);

finish:
    return VA_STATUS_SUCCESS;
}

static UINT8 Intel_HostvldVp9_GetPredSegmentId(
    register PUINT8 pSegmentId,
    VP9_PROPAGATE   Propagate)
{
    register UINT8 ui8SegId = VP9_MAX_SEGMENTS;
    UINT8          ui8Copy  = Propagate.ui8Copy;

    do
    {
        if (ui8SegId > *pSegmentId)
        {
            ui8SegId = *pSegmentId;
        }
        pSegmentId++;
    } while (ui8Copy--);

    if (Propagate.ui8Jump)
    {
        pSegmentId += Propagate.ui8Jump;
        ui8Copy = Propagate.ui8Copy;
        do
        {
            if (ui8SegId > *pSegmentId)
            {
                ui8SegId = *pSegmentId;
            }
            pSegmentId++;
        } while (ui8Copy--);
    }

    return ui8SegId;
}

static VAStatus Intel_HostvldVp9_ParseIntraBlockModeInfo(
    PINTEL_HOSTVLD_VP9_TILE_STATE   pTileState)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo  = &pTileState->pFrameState->FrameInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo     = &pTileState->MbInfo;
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine  = &pTileState->BacEngine;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlkSize     = pMbInfo->BlockSize;
    PINTEL_HOSTVLD_VP9_MV            pMotionVector;
    PUINT8                              pPredModeLuma, pTxTypeLuma;

    VAStatus  eStatus   = VA_STATUS_SUCCESS;

    pPredModeLuma = pMbInfo->pPredModeLuma;
    pTxTypeLuma   = pMbInfo->pTxTypeLuma;
    pMotionVector = pMbInfo->pMotionVector;

    pMbInfo->ReferenceFrame[0] = VP9_REF_FRAME_INTRA;
    pMbInfo->ReferenceFrame[1] = VP9_REF_FRAME_INTRA;
    VP9_PROP8x8_WORD(pMbInfo->pReferenceFrame, *((PUINT16)pMbInfo->ReferenceFrame));

    if (pMbInfo->iB4Number >= 4)
    {
        pMbInfo->ePredMode = Intel_HostvldVp9_ReadIntraMode_InterFrmY(
            pTileState, pBacEngine, pMbInfo->BlockSize);
        VP9_PROP4x4(pPredModeLuma, pMbInfo->ePredMode);
        VP9_PROP4x4(pTxTypeLuma, Intel_HostvldVp9_GetTxType(pFrameInfo, pMbInfo));
        VP9_PROP4x4_QWORD((PUINT64)pMotionVector, 0);
    }
    else
    {
        INT    iX, iY;
        INT    iWidth4x4, iHeight4x4;

        iWidth4x4   = 1 << (g_Vp9BlockSizeB4Log2[BlkSize][0]);
        iHeight4x4  = 1 << (g_Vp9BlockSizeB4Log2[BlkSize][1]);

        for (iY = 0; iY < 2; iY += iHeight4x4)
        {
            for (iX = 0; iX < 2; iX += iWidth4x4)
            {
                pMbInfo->ePredMode = Intel_HostvldVp9_ReadIntraMode_InterFrmY(
                    pTileState, pBacEngine, pMbInfo->BlockSize);
                VP9_PROP4x4(pPredModeLuma, pMbInfo->ePredMode);
                VP9_PROP4x4(pTxTypeLuma, Intel_HostvldVp9_GetTxType(pFrameInfo, pMbInfo));
                VP9_PROP4x4_QWORD((PUINT64)pMotionVector, 0);

                pPredModeLuma += iWidth4x4;
                pTxTypeLuma   += iWidth4x4;
            }
        }
    }

    pMbInfo->ePredModeChroma = Intel_HostvldVp9_ReadIntraMode_InterFrmUV(
        pTileState, pBacEngine, pMbInfo->ePredMode);

    VP9_PROP8x8(pMbInfo->pPredModeChroma, pMbInfo->ePredModeChroma);

    return eStatus;
}

static VAStatus Intel_HostvldVp9_ParseRefereceFrames(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlkSize;
    DWORD                               dwPredictionMode;
    DWORD                               dwContext;
    PINT8                               pReferenceFrame;

    VAStatus                          eStatus   = VA_STATUS_SUCCESS;

    pFrameInfo      = &pTileState->pFrameState->FrameInfo;
    pContext        = pFrameInfo->pContext;
    BlkSize         = pMbInfo->BlockSize;
    pReferenceFrame = pMbInfo->ReferenceFrame;

    // read prediction mode
    if (pMbInfo->bSegRefEnabled)
    {
        pReferenceFrame[0] = pMbInfo->i8SegReference;
        pReferenceFrame[1] = VP9_REF_FRAME_INTRA;
    }
    else
    {
        INTEL_HOSTVLD_VP9_REF_FRAME      ReferenceFrameLeft[2];
        INTEL_HOSTVLD_VP9_REF_FRAME      ReferenceFrameAbove[2];
        INT                                 iBit, iVarRefIndex;

        if (pMbInfo->bLeftValid)
        {
            ReferenceFrameLeft[0]  = (INTEL_HOSTVLD_VP9_REF_FRAME)(INT8)(pMbInfo->pReferenceFrame[pMbInfo->iLCtxOffset] & 0xFF);
            ReferenceFrameLeft[1]  = (INTEL_HOSTVLD_VP9_REF_FRAME)(INT8)(pMbInfo->pReferenceFrame[pMbInfo->iLCtxOffset] >> 8);
        }
        if (pMbInfo->bAboveValid)
        {
            ReferenceFrameAbove[0] = (INTEL_HOSTVLD_VP9_REF_FRAME)(INT8)(pMbInfo->pReferenceFrame[pMbInfo->iACtxOffset] & 0xFF);
            ReferenceFrameAbove[1] = (INTEL_HOSTVLD_VP9_REF_FRAME)(INT8)(pMbInfo->pReferenceFrame[pMbInfo->iACtxOffset] >> 8);
        }

        dwPredictionMode = pFrameInfo->dwPredictionMode;
        if (dwPredictionMode == VP9_HYBRID_PREDICTION)
        {
            if (pMbInfo->bLeftValid && pMbInfo->bAboveValid)
            {
                if (VP9_IS_SINGLE(Left) && VP9_IS_SINGLE(Above))
                {
                    dwContext = 
                        (ReferenceFrameLeft[0] == pFrameInfo->CompondFixedRef) ^ 
                        (ReferenceFrameAbove[0] == pFrameInfo->CompondFixedRef);
                }
                else if (VP9_IS_SINGLE(Above))
                {
                    dwContext = 2 + 
                        ((ReferenceFrameAbove[0] == pFrameInfo->CompondFixedRef) || 
                        !pMbInfo->pIsInter[pMbInfo->iACtxOffset]);
                }
                else if (VP9_IS_SINGLE(Left))
                {
                    dwContext = 2 + 
                        ((ReferenceFrameLeft[0] == pFrameInfo->CompondFixedRef) || 
                        !pMbInfo->pIsInter[pMbInfo->iLCtxOffset]);
                }
                else
                {
                    dwContext = 4;
                }
            }
            else if (pMbInfo->bLeftValid || pMbInfo->bAboveValid)
            {
                if (pMbInfo->bAboveValid && VP9_IS_SINGLE(Above))
                {
                    dwContext = ReferenceFrameAbove[0] == pFrameInfo->CompondFixedRef;
                }
                else if (pMbInfo->bLeftValid && VP9_IS_SINGLE(Left))
                {
                    dwContext = ReferenceFrameLeft[0] == pFrameInfo->CompondFixedRef;
                }
                else
                {
                    dwContext = 3;
                }
            }
            else
            {
                dwContext = 1;
            }

            dwPredictionMode = INTEL_HOSTVLD_VP9_READ_BIT(pFrameInfo->pContext->CompoundInterProbs[dwContext]);

            pTileState->Count.CompoundInterCounts[dwContext][dwPredictionMode] += pFrameInfo->bFrameParallelDisabled;
        }

        // parse reference frame index
        if (dwPredictionMode == VP9_COMPOUND_PREDICTION_ONLY) 
        {
            iVarRefIndex = !pFrameInfo->RefFrameSignBias[pFrameInfo->CompondFixedRef];

            if (pMbInfo->bLeftValid && pMbInfo->bAboveValid)
            {
                UINT uiLIsIntra  = !pMbInfo->pIsInter[pMbInfo->iLCtxOffset];
                UINT uiAIsIntra  = !pMbInfo->pIsInter[pMbInfo->iACtxOffset];

                if (uiLIsIntra && uiAIsIntra)
                {
                    dwContext = 2;
                }
                else if (uiLIsIntra || uiAIsIntra)
                {
                    PINTEL_HOSTVLD_VP9_REF_FRAME ReferenceFrameNb;

                    ReferenceFrameNb = uiLIsIntra ? ReferenceFrameAbove : ReferenceFrameLeft;
                    if (VP9_IS_SINGLE(Nb))
                    {
                        dwContext = 1 + 2 * (ReferenceFrameNb[0] != pFrameInfo->CompondVarRef[1]);
                    }
                    else
                    {
                        dwContext = 1 + 2 * (ReferenceFrameNb[iVarRefIndex] != pFrameInfo->CompondVarRef[1]);
                    }
                }
                else
                {
                    INTEL_HOSTVLD_VP9_REF_FRAME eVarRefLeft, eVarRefAbove;
                    BOOL bIsSingleRefLeft  = VP9_IS_SINGLE(Left);
                    BOOL bIsSingleRefAbove = VP9_IS_SINGLE(Above);

                    eVarRefLeft  = bIsSingleRefLeft  ? ReferenceFrameLeft[0]  : ReferenceFrameLeft[iVarRefIndex];
                    eVarRefAbove = bIsSingleRefAbove ? ReferenceFrameAbove[0] : ReferenceFrameAbove[iVarRefIndex];

                    if ((eVarRefLeft == eVarRefAbove) && (pFrameInfo->CompondVarRef[1] == eVarRefAbove))
                    {
                        dwContext = 0;
                    }
                    else if (bIsSingleRefLeft && bIsSingleRefAbove)
                    {
                        if ((eVarRefLeft == pFrameInfo->CompondFixedRef) && (eVarRefAbove == pFrameInfo->CompondVarRef[0]) || 
                            (eVarRefAbove == pFrameInfo->CompondFixedRef) && (eVarRefLeft == pFrameInfo->CompondVarRef[0]))
                        {
                            dwContext = 4;
                        }
                        else if (eVarRefLeft == eVarRefAbove)
                        {
                            dwContext = 3;
                        }
                        else
                        {
                            dwContext = 1;
                        }
                    }
                    else if (bIsSingleRefLeft || bIsSingleRefAbove)
                    {
                        INTEL_HOSTVLD_VP9_REF_FRAME eCompoundRef, eSingleRef;

                        if (bIsSingleRefLeft)
                        {
                            eCompoundRef = eVarRefAbove;
                            eSingleRef   = eVarRefLeft;
                        }
                        else
                        {
                            eCompoundRef = eVarRefLeft;
                            eSingleRef   = eVarRefAbove;
                        }

                        if ((eCompoundRef == pFrameInfo->CompondVarRef[1]) && 
                            (eSingleRef != pFrameInfo->CompondVarRef[1]))
                        {
                            dwContext = 1;
                        }
                        else if ((eSingleRef == pFrameInfo->CompondVarRef[1]) && 
                            (eCompoundRef != pFrameInfo->CompondVarRef[1]))
                        {
                            dwContext = 2;
                        }
                        else
                        {
                            dwContext = 4;
                        }
                    }
                    else if (eVarRefAbove == eVarRefLeft)
                    {
                        dwContext = 4;
                    }
                    else
                    {
                        dwContext = 2;
                    }
                }
            }
            else if (pMbInfo->bLeftValid || pMbInfo->bAboveValid)
            {
                PINTEL_HOSTVLD_VP9_REF_FRAME ReferenceFrameNb;

                ReferenceFrameNb = pMbInfo->bAboveValid ? ReferenceFrameAbove : ReferenceFrameLeft;

                if ((pMbInfo->bLeftValid  && !pMbInfo->pIsInter[pMbInfo->iLCtxOffset]) ||
                    (pMbInfo->bAboveValid && !pMbInfo->pIsInter[pMbInfo->iACtxOffset]))
                {
                    dwContext = 2;
                }
                else
                {
                    if (VP9_IS_SINGLE(Nb)) // no second ref
                    {
                        dwContext = 3 * (ReferenceFrameNb[0] != pFrameInfo->CompondVarRef[1]);
                    }
                    else
                    {
                        dwContext = 4 * (ReferenceFrameNb[iVarRefIndex] != pFrameInfo->CompondVarRef[1]);
                    }
                }
            }
            else
            {
                dwContext = 2;
            }

            iBit = INTEL_HOSTVLD_VP9_READ_BIT(pFrameInfo->pContext->CompoundRefProbs[dwContext]);
            pTileState->Count.CompoundRefCounts[dwContext][iBit] += pFrameInfo->bFrameParallelDisabled;

            pReferenceFrame[!iVarRefIndex] = pFrameInfo->CompondFixedRef;
            pReferenceFrame[iVarRefIndex] = pFrameInfo->CompondVarRef[iBit];
        }
        else if (dwPredictionMode == VP9_SINGLE_PREDICTION_ONLY)
        {
            if (pMbInfo->bLeftValid && pMbInfo->bAboveValid)
            {
                UINT uiLIsIntra  = !pMbInfo->pIsInter[pMbInfo->iLCtxOffset];
                UINT uiAIsIntra  = !pMbInfo->pIsInter[pMbInfo->iACtxOffset];

                if (uiLIsIntra && uiAIsIntra)
                {
                    dwContext = 2;
                }
                else if (uiLIsIntra || uiAIsIntra)
                {
                    PINTEL_HOSTVLD_VP9_REF_FRAME ReferenceFrameNb;

                    ReferenceFrameNb = uiLIsIntra ? ReferenceFrameAbove : ReferenceFrameLeft;
                    if (VP9_IS_SINGLE(Nb))
                    {
                        dwContext = 4 * (ReferenceFrameNb[0] == VP9_REF_FRAME_LAST);
                    }
                    else
                    {
                        dwContext = 1 + ((ReferenceFrameNb[0] == VP9_REF_FRAME_LAST) || 
                                         (ReferenceFrameNb[1] == VP9_REF_FRAME_LAST));
                    }
                }
                else
                {
                    if (VP9_IS_SINGLE(Left) &&
                        VP9_IS_SINGLE(Above))
                    {
                        dwContext = 2 * (ReferenceFrameLeft[0]  == VP9_REF_FRAME_LAST) + 
                                    2 * (ReferenceFrameAbove[0] == VP9_REF_FRAME_LAST);
                    }
                    else if (VP9_IS_COMPOUND(Left) &&
                             VP9_IS_COMPOUND(Above))
                    {
                        dwContext = 1 + ((ReferenceFrameLeft[0]  == VP9_REF_FRAME_LAST) || 
                                         (ReferenceFrameLeft[1]  == VP9_REF_FRAME_LAST) || 
                                         (ReferenceFrameAbove[0] == VP9_REF_FRAME_LAST) || 
                                         (ReferenceFrameAbove[1] == VP9_REF_FRAME_LAST));
                    }
                    else
                    {
                        if (VP9_IS_COMPOUND(Left))
                        {
                            if (ReferenceFrameAbove[0] == VP9_REF_FRAME_LAST)
                            {
                                dwContext = 3 + ((ReferenceFrameLeft[0] == VP9_REF_FRAME_LAST) || 
                                                 (ReferenceFrameLeft[1] == VP9_REF_FRAME_LAST));
                            }
                            else
                            {
                                dwContext = (ReferenceFrameLeft[0] == VP9_REF_FRAME_LAST) || 
                                            (ReferenceFrameLeft[1] == VP9_REF_FRAME_LAST);
                            }
                        }
                        else // above has 2nd reference
                        {
                            if (ReferenceFrameLeft[0] == VP9_REF_FRAME_LAST)
                            {
                                dwContext = 3 + ((ReferenceFrameAbove[0] == VP9_REF_FRAME_LAST) || 
                                                 (ReferenceFrameAbove[1] == VP9_REF_FRAME_LAST));
                            }
                            else
                            {
                                dwContext = (ReferenceFrameAbove[0] == VP9_REF_FRAME_LAST) || 
                                            (ReferenceFrameAbove[1] == VP9_REF_FRAME_LAST);
                            }
                        }
                    }
                }
            }
            else if (pMbInfo->bLeftValid || pMbInfo->bAboveValid)
            {
                PINTEL_HOSTVLD_VP9_REF_FRAME ReferenceFrameNb;

                ReferenceFrameNb = pMbInfo->bAboveValid ? ReferenceFrameAbove : ReferenceFrameLeft;

                if ((pMbInfo->bLeftValid  && !pMbInfo->pIsInter[pMbInfo->iLCtxOffset]) ||
                    (pMbInfo->bAboveValid && !pMbInfo->pIsInter[pMbInfo->iACtxOffset]))
                {
                    dwContext = 2;
                }
                else
                {
                    if (VP9_IS_SINGLE(Nb)) // single ref
                    {
                        dwContext = 4 * (ReferenceFrameNb[0] == VP9_REF_FRAME_LAST);
                    }
                    else
                    {
                        dwContext = 1 + ((ReferenceFrameNb[0] == VP9_REF_FRAME_LAST) || 
                                         (ReferenceFrameNb[1] == VP9_REF_FRAME_LAST));
                    }
                }
            }
            else
            {
                dwContext = 2;
            }

            iBit = INTEL_HOSTVLD_VP9_READ_BIT(pFrameInfo->pContext->SingleRefProbs[dwContext][0]);
            pTileState->Count.SingleRefCounts[dwContext][0][iBit] += pFrameInfo->bFrameParallelDisabled;

            if (iBit) 
            {
                if (pMbInfo->bLeftValid && pMbInfo->bAboveValid)
                {
                    UINT uiLIsIntra  = !pMbInfo->pIsInter[pMbInfo->iLCtxOffset];
                    UINT uiAIsIntra  = !pMbInfo->pIsInter[pMbInfo->iACtxOffset];

                    if (uiLIsIntra && uiAIsIntra)
                    {
                        dwContext = 2;
                    }
                    else if (uiLIsIntra || uiAIsIntra)
                    {
                        PINTEL_HOSTVLD_VP9_REF_FRAME ReferenceFrameNb;

                        ReferenceFrameNb = uiLIsIntra ? ReferenceFrameAbove : ReferenceFrameLeft;
                        if (VP9_IS_SINGLE(Nb))
                        {
                            dwContext = (ReferenceFrameNb[0] == VP9_REF_FRAME_LAST) ? 
                                3 : 4 * (ReferenceFrameNb[0] == VP9_REF_FRAME_GOLDEN);
                        }
                        else
                        {
                            dwContext = 1 + 2 * ((ReferenceFrameNb[0] == VP9_REF_FRAME_GOLDEN) || 
                                                 (ReferenceFrameNb[1] == VP9_REF_FRAME_GOLDEN));
                        }
                    }
                    else
                    {
                        if (VP9_IS_SINGLE(Left) &&
                            VP9_IS_SINGLE(Above))
                        {
                            if ((ReferenceFrameLeft[0]  == VP9_REF_FRAME_LAST) && 
                                (ReferenceFrameAbove[0] == VP9_REF_FRAME_LAST))
                            {
                                dwContext = 3;
                            }
                            else if ((ReferenceFrameLeft[0]  == VP9_REF_FRAME_LAST) || 
                                     (ReferenceFrameAbove[0] == VP9_REF_FRAME_LAST))
                            {
                                dwContext = ReferenceFrameLeft[0] == VP9_REF_FRAME_LAST ? 
                                    4 * (ReferenceFrameAbove[0] == VP9_REF_FRAME_GOLDEN) : 
                                    4 * (ReferenceFrameLeft[0]  == VP9_REF_FRAME_GOLDEN);
                            }
                            else
                            {
                                dwContext = 2 * (ReferenceFrameLeft[0]  == VP9_REF_FRAME_GOLDEN) + 
                                            2 * (ReferenceFrameAbove[0] == VP9_REF_FRAME_GOLDEN);
                            }
                        }
                        else if (VP9_IS_COMPOUND(Left) &&
                                 VP9_IS_COMPOUND(Above))
                        {
                            if ((ReferenceFrameLeft[0] == ReferenceFrameAbove[0]) && 
                                (ReferenceFrameLeft[1] == ReferenceFrameAbove[1]))
                            {
                                dwContext = 3 * ((ReferenceFrameLeft[0]  == VP9_REF_FRAME_GOLDEN) || 
                                                 (ReferenceFrameLeft[1]  == VP9_REF_FRAME_GOLDEN) || 
                                                 (ReferenceFrameAbove[0] == VP9_REF_FRAME_GOLDEN) || 
                                                 (ReferenceFrameAbove[1] == VP9_REF_FRAME_GOLDEN));
                            }
                            else
                            {
                                dwContext = 2;
                            }
                        }
                        else
                        {
                            if (VP9_IS_COMPOUND(Left))
                            {
                                if (ReferenceFrameAbove[0] == VP9_REF_FRAME_GOLDEN)
                                {
                                    dwContext = 3 + ((ReferenceFrameLeft[0] == VP9_REF_FRAME_GOLDEN) || 
                                                     (ReferenceFrameLeft[1] == VP9_REF_FRAME_GOLDEN));
                                }
                                else if (ReferenceFrameAbove[0] == VP9_REF_FRAME_ALTREF)
                                {
                                    dwContext = (ReferenceFrameLeft[0] == VP9_REF_FRAME_GOLDEN) || 
                                                (ReferenceFrameLeft[1] == VP9_REF_FRAME_GOLDEN);
                                }
                                else
                                {
                                    dwContext = 1 + 2 * ((ReferenceFrameLeft[0] == VP9_REF_FRAME_GOLDEN) || 
                                                         (ReferenceFrameLeft[1] == VP9_REF_FRAME_GOLDEN));
                                }
                            }
                            else
                            {
                                if (ReferenceFrameLeft[0] == VP9_REF_FRAME_GOLDEN)
                                {
                                    dwContext = 3 + ((ReferenceFrameAbove[0] == VP9_REF_FRAME_GOLDEN) || 
                                                     (ReferenceFrameAbove[1] == VP9_REF_FRAME_GOLDEN));
                                }
                                else if (ReferenceFrameLeft[0] == VP9_REF_FRAME_ALTREF)
                                {
                                    dwContext = (ReferenceFrameAbove[0] == VP9_REF_FRAME_GOLDEN) || 
                                                (ReferenceFrameAbove[1] == VP9_REF_FRAME_GOLDEN);
                                }
                                else
                                {
                                    dwContext = 1 + 2 * ((ReferenceFrameAbove[0] == VP9_REF_FRAME_GOLDEN) || 
                                                         (ReferenceFrameAbove[1] == VP9_REF_FRAME_GOLDEN));
                                }
                            }
                        }
                    }
                }
                else if (pMbInfo->bLeftValid || pMbInfo->bAboveValid)
                {
                    PINTEL_HOSTVLD_VP9_REF_FRAME ReferenceFrameNb;
                    BOOL                            bIsInterBlock;

                    ReferenceFrameNb = pMbInfo->bAboveValid ? ReferenceFrameAbove : ReferenceFrameLeft;
                    bIsInterBlock    = (pMbInfo->bLeftValid  && !pMbInfo->pIsInter[pMbInfo->iLCtxOffset]) ||
                                       (pMbInfo->bAboveValid && !pMbInfo->pIsInter[pMbInfo->iACtxOffset]);

                    if (bIsInterBlock || 
                        ((ReferenceFrameNb[0] == VP9_REF_FRAME_LAST) && 
                         VP9_IS_SINGLE(Nb)))
                    {
                        dwContext = 2;
                    }
                    else if (VP9_IS_SINGLE(Nb))
                    {
                        dwContext = 4 * (ReferenceFrameNb[0] == VP9_REF_FRAME_GOLDEN);
                    }
                    else
                    {
                        dwContext = 3 * ((ReferenceFrameNb[0] == VP9_REF_FRAME_GOLDEN) || 
                                         (ReferenceFrameNb[1] == VP9_REF_FRAME_GOLDEN));
                    }
                }
                else
                {
                    dwContext = 2;
                }

                iBit = INTEL_HOSTVLD_VP9_READ_BIT(pFrameInfo->pContext->SingleRefProbs[dwContext][1]);
                pTileState->Count.SingleRefCounts[dwContext][1][iBit] += pFrameInfo->bFrameParallelDisabled;
                pReferenceFrame[0] = iBit ? VP9_REF_FRAME_ALTREF : VP9_REF_FRAME_GOLDEN;
            } 
            else 
            {
                pReferenceFrame[0] = VP9_REF_FRAME_LAST;
            }

            pReferenceFrame[1] = VP9_REF_FRAME_INTRA;
        }
        else
        {
            eStatus = VA_STATUS_ERROR_DECODING_ERROR;
            /* Use assert */
            assert(0);
        }
    }

    VP9_PROP8x8_WORD(pMbInfo->pReferenceFrame, *((PUINT16)pReferenceFrame));

    return eStatus;
}

static INT Intel_HostvldVp9_GetInterModeContext(
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo)
{
    INT i, iCounter, iOffset, iX, iY;
    const INTEL_HOSTVLD_VP9_MV *pMv;

    iCounter = 0;
    pMv      = g_Vp9MvRefBlocks + (pMbInfo->BlockSize << VP9_LOG2_MV_REF_NEIGHBOURS);
    for (i = 0; i < 2; i++)
    {
        iX = pMbInfo->dwMbPosX + pMv->i16X;
        iY = pMbInfo->dwMbPosY + pMv->i16Y;
        if (VP9_IN_TILE_COLUMN(iX, iY))
        {
            iOffset = (iY & ~7) * pFrameInfo->dwMbStride + ((iX & ~7) << VP9_LOG2_B64_SIZE_IN_B8);
            iOffset += g_Vp9SB_ZOrder8X8[iY & 7][iX & 7];
            iOffset = (INT)((iOffset - pMbInfo->dwMbOffset - pMbInfo->i8ZOrder) << 2) + 3;
            iCounter += g_Vp9ModeContextCounter[pMbInfo->pPredModeLuma[iOffset]];
        }
        pMv++;
    }

    return g_Vp9ModeContext[iCounter];
}

static VAStatus Intel_HostvldVp9_ParseInterMode(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine, 
    INT                                 iContext)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PUINT8                              pContext;
    VAStatus                          eStatus = VA_STATUS_SUCCESS;

    pFrameInfo = &pTileState->pFrameState->FrameInfo;
    pContext   = pFrameInfo->pContext->InterModeProbs[iContext];

    if (INTEL_HOSTVLD_VP9_READ_BIT(pContext[0]))
    {
        if (INTEL_HOSTVLD_VP9_READ_BIT(pContext[1]))
        {
            if (INTEL_HOSTVLD_VP9_READ_BIT(pContext[2]))
            {
                pMbInfo->ePredMode = PRED_MD_NEWMV;
            }
            else
            {
                pMbInfo->ePredMode = PRED_MD_NEARMV;
            }
        }
        else
        {
            pMbInfo->ePredMode = PRED_MD_NEARESTMV;
        }
    }
    else
    {
        pMbInfo->ePredMode = PRED_MD_ZEROMV;
    }

    pTileState->Count.InterModeCounts[iContext][pMbInfo->ePredMode - PRED_MD_NEARESTMV] += pFrameInfo->bFrameParallelDisabled;

    return eStatus;
}

static VAStatus Intel_HostvldVp9_ParseInterpolationType(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    VAStatus                          eStatus = VA_STATUS_SUCCESS;

    pFrameInfo = &pTileState->pFrameState->FrameInfo;

    if (pFrameInfo->eInterpolationType == VP9_INTERP_SWITCHABLE)
    {
        // get context
        INT iContext;
        PUINT8 pContext;
        BOOL bLIsInter = pMbInfo->bLeftValid && pMbInfo->pIsInter[pMbInfo->iLCtxOffset];
        BOOL bAIsInter = pMbInfo->bAboveValid && pMbInfo->pIsInter[pMbInfo->iACtxOffset];
        INT  iLFilter  = bLIsInter ? pMbInfo->pFilterType[pMbInfo->iLCtxOffset] : VP9_SWITCHABLE_FILTERS;
        INT  iAFilter  = bAIsInter ? pMbInfo->pFilterType[pMbInfo->iACtxOffset] : VP9_SWITCHABLE_FILTERS;

        if (iLFilter == iAFilter)
        {
            iContext = iLFilter;
        }
        else if ((iLFilter == VP9_SWITCHABLE_FILTERS) && (iAFilter != VP9_SWITCHABLE_FILTERS))
        {
            iContext = iAFilter;
        }
        else if ((iAFilter == VP9_SWITCHABLE_FILTERS) && (iLFilter != VP9_SWITCHABLE_FILTERS))
        {
            iContext = iLFilter;
        }
        else
        {
            iContext = VP9_SWITCHABLE_FILTERS;
        }

        // read interploation type from bitsteam
        pContext = pFrameInfo->pContext->SwitchableInterpProbs[iContext];
        if (INTEL_HOSTVLD_VP9_READ_BIT(pContext[0]))
        {
            if (INTEL_HOSTVLD_VP9_READ_BIT(pContext[1]))
            {
                pMbInfo->eInterpolationType = VP9_INTERP_EIGHTTAP_SHARP;
            }
            else
            {
                pMbInfo->eInterpolationType = VP9_INTERP_EIGHTTAP_SMOOTH;
            }
        }
        else
        {
            pMbInfo->eInterpolationType = VP9_INTERP_EIGHTTAP;
        }

        pTileState->Count.SwitchableInterpCounts[iContext][pMbInfo->eInterpolationType] += pFrameInfo->bFrameParallelDisabled;
    }
    else
    {
        pMbInfo->eInterpolationType = pFrameInfo->eInterpolationType;
    }

    return eStatus;
}

#define ABS	abs

static VAStatus Intel_HostvldVp9_FindNearestMv(
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo, 
    BOOL                                bIsSecondRef, 
    INT                                 iBlockIndex)
{
    INT i, iOffset, iX, iY;
    INT8 i8RefFrame;
    const INTEL_HOSTVLD_VP9_MV *pMv;
    INTEL_HOSTVLD_VP9_MV NearestMv;
    PINTEL_HOSTVLD_VP9_MV pPrevMv, pRefMv;
    PINT8 pRefBlockRefFrame, pPrevRefFrame;
    INT32 i32MvMax, i32MvMin;
    BOOL  bFoundDifferentRef;
    VAStatus eStatus   = VA_STATUS_SUCCESS;

    if ((iBlockIndex != 0) && (iBlockIndex != -1))
    {
        goto finish;
    }

    bFoundDifferentRef  = 0;
    NearestMv.dwValue   = 0;
    i8RefFrame          = pMbInfo->ReferenceFrame[bIsSecondRef];

    // check the neighbors first
    pMv      = g_Vp9MvRefBlocks + (pMbInfo->BlockSize << VP9_LOG2_MV_REF_NEIGHBOURS);
    for (i = 0; i < VP9_MV_REF_NEIGHBOURS; i++)
    {
        iX = pMbInfo->dwMbPosX + pMv->i16X;
        iY = pMbInfo->dwMbPosY + pMv->i16Y;

        if (VP9_IN_TILE_COLUMN(iX, iY))
        {
            iOffset = VP9_GET_ZORDER_OFFSET_B8(iX, iY);
            pRefBlockRefFrame = (PINT8)(pMbInfo->pReferenceFrame + iOffset);

            if (pRefBlockRefFrame[0] == i8RefFrame)
            {
                if ((i < 2) && (pMbInfo->iB4Number < 4) && (iBlockIndex >= 0))
                {
                    iOffset = ((iOffset << 2) + g_Vp9IndexColumnToSubblock[iBlockIndex][pMv->i16X == 0]) * 2;
                    NearestMv.dwValue = pMbInfo->pMotionVector[iOffset].dwValue;
                }
                else
                {
                    iOffset = ((iOffset << 2) + 3) * 2;
                    NearestMv.dwValue = pMbInfo->pMotionVector[iOffset].dwValue;
                }
                goto finish;
            }
            else 
            {
                if (pRefBlockRefFrame[1] == i8RefFrame)
                {
                    if ((i < 2) && (pMbInfo->iB4Number < 4) && (iBlockIndex >= 0))
                    {
                        iOffset = ((iOffset << 2) + g_Vp9IndexColumnToSubblock[iBlockIndex][pMv->i16X == 0]) * 2 + 1;
                        NearestMv.dwValue = pMbInfo->pMotionVector[iOffset].dwValue;
                    }
                    else
                    {
                        iOffset = ((iOffset << 2) + 3) * 2 + 1;
                        NearestMv.dwValue = pMbInfo->pMotionVector[iOffset].dwValue;
                    }
                    goto finish;
                }
                bFoundDifferentRef = 1;
            }
        }

        pMv++;
    }

    // If cannot find best MV in neighbors, try to find it in previous frame.
    pPrevMv         = pMbInfo->pPrevMv + 6;

    if (pFrameInfo->bHasPrevFrame)
    {
        pPrevRefFrame   = (PINT8)pMbInfo->pPrevRefFrame;

        if (pPrevRefFrame[0] == i8RefFrame)
        {
            NearestMv.dwValue = pPrevMv[0].dwValue;
            goto finish;
        }
        else if (pPrevRefFrame[1] == i8RefFrame)
        {
            NearestMv.dwValue = pPrevMv[1].dwValue;
            goto finish;
        }
    }

    if (bFoundDifferentRef)
    {
        pMv = g_Vp9MvRefBlocks + (pMbInfo->BlockSize << VP9_LOG2_MV_REF_NEIGHBOURS);
        for (i = 0; i < VP9_MV_REF_NEIGHBOURS; i++)
        {
            iX = pMbInfo->dwMbPosX + pMv->i16X;
            iY = pMbInfo->dwMbPosY + pMv->i16Y;

            if (VP9_IN_TILE_COLUMN(iX, iY))
            {
                iOffset         = VP9_GET_ZORDER_OFFSET_B8(iX, iY);
                pPrevRefFrame   = (PINT8)(pMbInfo->pReferenceFrame + iOffset);

                if (pPrevRefFrame[0] > VP9_REF_FRAME_INTRA) // is inter block
                {
                    PBOOL pRefFrameSignBias = pFrameInfo->RefFrameSignBias;

                    iOffset = (iOffset << 3) + (3 * 2);
                    pRefMv = pMbInfo->pMotionVector + iOffset;

                    if (pPrevRefFrame[0] != i8RefFrame) 
                    {
                        if (pRefFrameSignBias[(UINT8)pPrevRefFrame[0]] != pRefFrameSignBias[(UINT8)i8RefFrame])
                        {
                            NearestMv.i16X = pRefMv[0].i16X * -1;
                            NearestMv.i16Y = pRefMv[0].i16Y * -1;
                        }
                        else
                        {
                            NearestMv.dwValue = pRefMv[0].dwValue;
                        }
                        goto finish;
                    }
                    if ((pPrevRefFrame[1] != i8RefFrame)         && 
                        (pPrevRefFrame[1] > VP9_REF_FRAME_INTRA) && 
                        (pRefMv[0].dwValue != pRefMv[1].dwValue))
                    {
                        if (pRefFrameSignBias[(UINT8)pPrevRefFrame[1]] != pRefFrameSignBias[(UINT8)i8RefFrame])
                        {
                            NearestMv.i16X = pRefMv[1].i16X * -1;
                            NearestMv.i16Y = pRefMv[1].i16Y * -1;
                        }
                        else
                        {
                            NearestMv.dwValue = pRefMv[1].dwValue;
                        }
                        goto finish;
                    }
                }
            }

            pMv++;
        }
    }

    pPrevRefFrame   = (PINT8)pMbInfo->pPrevRefFrame;
    if (pFrameInfo->bHasPrevFrame && (pPrevRefFrame[0] > VP9_REF_FRAME_INTRA))
    {
        PBOOL pRefFrameSignBias = pFrameInfo->RefFrameSignBias;

        if (pPrevRefFrame[0] != i8RefFrame) 
        {
            if (pRefFrameSignBias[(UINT8)pPrevRefFrame[0]] != pRefFrameSignBias[(UINT8)i8RefFrame])
            {
                NearestMv.i16X = pPrevMv[0].i16X * -1;
                NearestMv.i16Y = pPrevMv[0].i16Y * -1;
            }
            else
            {
                NearestMv.dwValue = pPrevMv[0].dwValue;
            }
            goto finish;
        }
        if ((pPrevRefFrame[1] != i8RefFrame)         && 
            (pPrevRefFrame[1] > VP9_REF_FRAME_INTRA) && 
            (pPrevMv[0].dwValue != pPrevMv[1].dwValue))
        {
            if (pRefFrameSignBias[(UINT8)pPrevRefFrame[1]] != pRefFrameSignBias[(UINT8)i8RefFrame])
            {
                NearestMv.i16X = pPrevMv[1].i16X * -1;
                NearestMv.i16Y = pPrevMv[1].i16Y * -1;
            }
            else
            {
                NearestMv.dwValue = pPrevMv[1].dwValue;
            }
            goto finish;
        }
    }

finish:
    // clamp MV
    if ((iBlockIndex == 0) || (iBlockIndex == -1))
    {
        i32MvMin = -static_cast<int>(pMbInfo->dwMbPosX << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
        i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
        NearestMv.i16X = INTEL_VP9_CLAMP(NearestMv.i16X, i32MvMin, i32MvMax);
        i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
        i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
        NearestMv.i16Y = INTEL_VP9_CLAMP(NearestMv.i16Y, i32MvMin, i32MvMax);

        if (iBlockIndex == -1)
        {
            BOOL bUseHighPrecisionMv = pFrameInfo->bAllowHighPrecisionMv  && 
                ((ABS(NearestMv.i16X) >> 4) < VP9_COMPANDED_MVREF_THRESH) &&
                ((ABS(NearestMv.i16Y) >> 4) < VP9_COMPANDED_MVREF_THRESH);
            if (!bUseHighPrecisionMv)
            {
                if (NearestMv.i16X & 3)
                {
                    NearestMv.i16X += NearestMv.i16X > 0 ? -2 : 2;
                }
                if (NearestMv.i16Y & 3)
                {
                    NearestMv.i16Y += NearestMv.i16Y > 0 ? -2 : 2;
                }
            }

            i32MvMin = -static_cast<int>(pMbInfo->dwMbPosX << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_MARGIN;
            i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_MARGIN;
            NearestMv.i16X = INTEL_VP9_CLAMP(NearestMv.i16X, i32MvMin, i32MvMax);
            i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_MARGIN;
            i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_MARGIN;
            NearestMv.i16Y = INTEL_VP9_CLAMP(NearestMv.i16Y, i32MvMin, i32MvMax);
        }
        pMbInfo->NearestMv[bIsSecondRef].dwValue = NearestMv.dwValue;
    }
    else if ((iBlockIndex == 1) || (iBlockIndex == 2))
    {
        pMbInfo->NearestMv[bIsSecondRef].dwValue = pMbInfo->pMotionVector[0 * 2 + bIsSecondRef].dwValue;
    }
    else // if (iBlockIndex == 3)
    {
        pMbInfo->NearestMv[bIsSecondRef].dwValue = pMbInfo->pMotionVector[2 * 2 + bIsSecondRef].dwValue;
    }

    return eStatus;
}

static VAStatus Intel_HostvldVp9_FindBestMv(
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo, 
    BOOL                                bIsSecondRef)
{
    VAStatus eStatus   = VA_STATUS_SUCCESS;

    if (pMbInfo->BestMv[bIsSecondRef].dwValue == VP9_INVALID_MV_VALUE)
    {
        eStatus = Intel_HostvldVp9_FindNearestMv(pFrameInfo, pMbInfo, bIsSecondRef, -1);
        pMbInfo->BestMv[bIsSecondRef].dwValue = pMbInfo->NearestMv[bIsSecondRef].dwValue;
    }

    return eStatus;
}

static VAStatus Intel_HostvldVp9_FindNearMv(
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo, 
    BOOL                                bIsSecondRef, 
    INT                                 iBlockIndex)
{
    INT i, iOffset, iX, iY, iCount;
    INT8 i8RefFrame;
    const INTEL_HOSTVLD_VP9_MV *pMv;
    INTEL_HOSTVLD_VP9_MV NearestMv, NearMv, Mv;
    PINTEL_HOSTVLD_VP9_MV pPrevMv, pRefMv;
    PINT8 pRefBlockRefFrame, pPrevRefFrame;
    INT32 i32MvMax, i32MvMin;
    BOOL  bFoundDifferentRef;
    VAStatus eStatus  = VA_STATUS_SUCCESS;

    iCount              = 0;
    bFoundDifferentRef  = 0;
    NearestMv.dwValue   = 0;
    NearMv.dwValue      = 0;
    i8RefFrame          = pMbInfo->ReferenceFrame[bIsSecondRef];

    // check the neighbors first
    pMv      = g_Vp9MvRefBlocks + (pMbInfo->BlockSize << VP9_LOG2_MV_REF_NEIGHBOURS);
    for (i = 0; i < VP9_MV_REF_NEIGHBOURS; i++)
    {
        iX = pMbInfo->dwMbPosX + pMv->i16X;
        iY = pMbInfo->dwMbPosY + pMv->i16Y;

        if (VP9_IN_TILE_COLUMN(iX, iY))
        {
            iOffset = VP9_GET_ZORDER_OFFSET_B8(iX, iY);
            pRefBlockRefFrame = (PINT8)(pMbInfo->pReferenceFrame + iOffset);

            if (pRefBlockRefFrame[0] == i8RefFrame)
            {
                iOffset <<= 3;
                iOffset += ((i < 2) && (pMbInfo->iB4Number < 4) && (iBlockIndex >= 0)) ? 
                    (g_Vp9IndexColumnToSubblock[iBlockIndex][pMv->i16X == 0] * 2) : (3 * 2);
                Mv.dwValue = pMbInfo->pMotionVector[iOffset].dwValue;
                if (iCount)
                {
                    if (Mv.dwValue != NearestMv.dwValue)
                    {
                        NearMv.dwValue = Mv.dwValue;
                        goto finish;
                    }
                }
                else
                {
                    NearestMv.dwValue = Mv.dwValue;
                    iCount++;
                }
                bFoundDifferentRef = pRefBlockRefFrame[1] == i8RefFrame;
            }
            else 
            {
                if (pRefBlockRefFrame[1] == i8RefFrame)
                {
                    iOffset <<= 3;
                    iOffset += ((i < 2) && (pMbInfo->iB4Number < 4) && (iBlockIndex >= 0)) ? 
                        (g_Vp9IndexColumnToSubblock[iBlockIndex][pMv->i16X == 0] * 2 + 1) : (3 * 2 + 1);
                    Mv.dwValue = pMbInfo->pMotionVector[iOffset].dwValue;
                    if (iCount)
                    {
                        if (Mv.dwValue != NearestMv.dwValue)
                        {
                            NearMv.dwValue = Mv.dwValue;
                            goto finish;
                        }
                    }
                    else
                    {
                        NearestMv.dwValue = Mv.dwValue;
                        iCount++;
                    }
                }
            }

            bFoundDifferentRef = 1;
        }

        pMv++;
    }

    // If cannot find best MV in neighbors, try to find it in previous frame.
    if (pFrameInfo->bHasPrevFrame)
    {
        pPrevRefFrame   = (PINT8)pMbInfo->pPrevRefFrame;
        pPrevMv         = pMbInfo->pPrevMv + 6;

        if (pPrevRefFrame[0] == i8RefFrame)
        {
            if (iCount)
            {
                if (pPrevMv[0].dwValue != NearestMv.dwValue)
                {
                    NearMv.dwValue = pPrevMv[0].dwValue;
                    goto finish;
                }
            }
            else
            {
                NearestMv.dwValue = pPrevMv[0].dwValue;
                iCount++;
            }
        }
        else if (pPrevRefFrame[1] == i8RefFrame)
        {
            if (iCount)
            {
                if (pPrevMv[1].dwValue != NearestMv.dwValue)
                {
                    NearMv.dwValue = pPrevMv[1].dwValue;
                    goto finish;
                }
            }
            else
            {
                NearestMv.dwValue = pPrevMv[1].dwValue;
                iCount++;
            }
        }
    }

    if (bFoundDifferentRef)
    {
        pMv = g_Vp9MvRefBlocks + (pMbInfo->BlockSize << VP9_LOG2_MV_REF_NEIGHBOURS);
        for (i = 0; i < VP9_MV_REF_NEIGHBOURS; i++)
        {
            iX = pMbInfo->dwMbPosX + pMv->i16X;
            iY = pMbInfo->dwMbPosY + pMv->i16Y;

            if (VP9_IN_TILE_COLUMN(iX, iY))
            {
                iOffset         = VP9_GET_ZORDER_OFFSET_B8(iX, iY);
                pPrevRefFrame   = (PINT8)(pMbInfo->pReferenceFrame + iOffset);

                if (pPrevRefFrame[0] > VP9_REF_FRAME_INTRA) // is inter block
                {
                    PBOOL pRefFrameSignBias = pFrameInfo->RefFrameSignBias;

                    iOffset = (iOffset << 3) + (3 * 2);
                    pRefMv = pMbInfo->pMotionVector + iOffset;

                    if (pPrevRefFrame[0] != i8RefFrame) 
                    {
                        // scale MV
                        if (pRefFrameSignBias[(UINT8)pPrevRefFrame[0]] != pRefFrameSignBias[(UINT8)i8RefFrame])
                        {
                            Mv.i16X = pRefMv[0].i16X * -1;
                            Mv.i16Y = pRefMv[0].i16Y * -1;
                        }
                        else
                        {
                            Mv.dwValue = pRefMv[0].dwValue;
                        }

                        if (iCount)
                        {
                            if (Mv.dwValue != NearestMv.dwValue)
                            {
                                NearMv.dwValue = Mv.dwValue;
                                goto finish;
                            }
                        }
                        else
                        {
                            NearestMv.dwValue = Mv.dwValue;
                            iCount++;
                        }
                    }
                    if ((pPrevRefFrame[1] != i8RefFrame)         && 
                        (pPrevRefFrame[1] > VP9_REF_FRAME_INTRA) && 
                        (pRefMv[0].dwValue != pRefMv[1].dwValue))
                    {
                        if (pRefFrameSignBias[(UINT8)pPrevRefFrame[1]] != pRefFrameSignBias[(UINT8)i8RefFrame])
                        {
                            Mv.i16X = pRefMv[1].i16X * -1;
                            Mv.i16Y = pRefMv[1].i16Y * -1;
                        }
                        else
                        {
                            Mv.dwValue = pRefMv[1].dwValue;
                        }

                        if (iCount)
                        {
                            if (Mv.dwValue != NearestMv.dwValue)
                            {
                                NearMv.dwValue = Mv.dwValue;
                                goto finish;
                            }
                        }
                        else
                        {
                            NearestMv.dwValue = Mv.dwValue;
                            iCount++;
                        }
                    }
                }
            }

            pMv++;
        }
    }

    pPrevRefFrame   = (PINT8)pMbInfo->pPrevRefFrame;
    if (pFrameInfo->bHasPrevFrame && (pPrevRefFrame[0] > VP9_REF_FRAME_INTRA))
    {
        PBOOL pRefFrameSignBias = pFrameInfo->RefFrameSignBias;

        pPrevMv = pMbInfo->pPrevMv + 6;

        if (pPrevRefFrame[0] != i8RefFrame) 
        {
            if (pRefFrameSignBias[(UINT8)pPrevRefFrame[0]] != pRefFrameSignBias[(UINT8)i8RefFrame])
            {
                Mv.i16X = pPrevMv[0].i16X * -1;
                Mv.i16Y = pPrevMv[0].i16Y * -1;
            }
            else
            {
                Mv.dwValue = pPrevMv[0].dwValue;
            }

            if (iCount)
            {
                if (Mv.dwValue != NearestMv.dwValue)
                {
                    NearMv.dwValue = Mv.dwValue;
                    goto finish;
                }
            }
            else
            {
                NearestMv.dwValue = Mv.dwValue;
                iCount++;
            }
        }
        if ((pPrevRefFrame[1] != i8RefFrame)         && 
            (pPrevRefFrame[1] > VP9_REF_FRAME_INTRA) && 
            (pPrevMv[0].dwValue != pPrevMv[1].dwValue))
        {
            if (pRefFrameSignBias[(UINT8)pPrevRefFrame[1]] != pRefFrameSignBias[(UINT8)i8RefFrame])
            {
                Mv.i16X = pPrevMv[1].i16X * -1;
                Mv.i16Y = pPrevMv[1].i16Y * -1;
            }
            else
            {
                Mv.dwValue = pPrevMv[1].dwValue;
            }

            if (iCount)
            {
                if (Mv.dwValue != NearestMv.dwValue)
                {
                    NearMv.dwValue = Mv.dwValue;
                    goto finish;
                }
            }
            else
            {
                NearestMv.dwValue = Mv.dwValue;
                iCount++;
            }
        }
    }

finish:
    if ((iBlockIndex == 0) || (iBlockIndex == -1))
    {
        // clamp MV
        i32MvMin = -static_cast<int>(pMbInfo->dwMbPosX << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
        i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
        NearMv.i16X = INTEL_VP9_CLAMP(NearMv.i16X, i32MvMin, i32MvMax);
        i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
        i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
        NearMv.i16Y = INTEL_VP9_CLAMP(NearMv.i16Y, i32MvMin, i32MvMax);

        if (iBlockIndex == -1)
        {
            BOOL bUseHighPrecisionMv = pFrameInfo->bAllowHighPrecisionMv  && 
                ((ABS(NearMv.i16X) >> 4) < VP9_COMPANDED_MVREF_THRESH)    &&
                ((ABS(NearMv.i16Y) >> 4) < VP9_COMPANDED_MVREF_THRESH);
            if (!bUseHighPrecisionMv)
            {
                if (NearMv.i16X & 3)
                {
                    NearMv.i16X += NearMv.i16X > 0 ? -2 : 2;
                }
                if (NearMv.i16Y & 3)
                {
                    NearMv.i16Y += NearMv.i16Y > 0 ? -2 : 2;
                }
            }

            i32MvMin = -static_cast<int>(pMbInfo->dwMbPosX << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_MARGIN;
            i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_MARGIN;
            NearMv.i16X = INTEL_VP9_CLAMP(NearMv.i16X, i32MvMin, i32MvMax);
            i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_MARGIN;
            i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_MARGIN;
            NearMv.i16Y = INTEL_VP9_CLAMP(NearMv.i16Y, i32MvMin, i32MvMax);
        }
        pMbInfo->NearMv[bIsSecondRef].dwValue = NearMv.dwValue;
    }
    else if ((iBlockIndex == 1) || (iBlockIndex == 2))
    {
        i32MvMin = -static_cast<int>(pMbInfo->dwMbPosX << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
        i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
        NearestMv.i16X = INTEL_VP9_CLAMP(NearestMv.i16X, i32MvMin, i32MvMax);
        i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
        i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
        NearestMv.i16Y = INTEL_VP9_CLAMP(NearestMv.i16Y, i32MvMin, i32MvMax);

        Mv.dwValue = pMbInfo->pMotionVector[0 * 2 + bIsSecondRef].dwValue;
        if (NearestMv.dwValue != Mv.dwValue)
        {
            pMbInfo->NearMv[bIsSecondRef].dwValue = NearestMv.dwValue;
        }
        else 
        {
            i32MvMin = -static_cast<int>(pMbInfo->dwMbPosX << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
            i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
            NearMv.i16X = INTEL_VP9_CLAMP(NearMv.i16X, i32MvMin, i32MvMax);
            i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
            i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
            NearMv.i16Y = INTEL_VP9_CLAMP(NearMv.i16Y, i32MvMin, i32MvMax);

            if (NearMv.dwValue != Mv.dwValue)
            {
                pMbInfo->NearMv[bIsSecondRef].dwValue = NearMv.dwValue;
            }
            else
            {
                pMbInfo->NearMv[bIsSecondRef].dwValue = 0;
            }
        }
    }
    else // if (iBlockIndex == 3)
    {
        Mv.dwValue = pMbInfo->pMotionVector[2 * 2 + bIsSecondRef].dwValue;
        if (pMbInfo->pMotionVector[1 * 2 + bIsSecondRef].dwValue != Mv.dwValue)
        {
            pMbInfo->NearMv[bIsSecondRef].dwValue = pMbInfo->pMotionVector[1 * 2 + bIsSecondRef].dwValue;
        }
        else if (pMbInfo->pMotionVector[0 * 2 + bIsSecondRef].dwValue != Mv.dwValue)
        {
            pMbInfo->NearMv[bIsSecondRef].dwValue = pMbInfo->pMotionVector[0 * 2 + bIsSecondRef].dwValue;
        }
        else 
        {
            i32MvMin = -static_cast<int>(pMbInfo->dwMbPosX << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
            i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
            NearestMv.i16X = INTEL_VP9_CLAMP(NearestMv.i16X, i32MvMin, i32MvMax);
            i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
            i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
            NearestMv.i16Y = INTEL_VP9_CLAMP(NearestMv.i16Y, i32MvMin, i32MvMax);

            if (NearestMv.dwValue != Mv.dwValue)
            {
                pMbInfo->NearMv[bIsSecondRef].dwValue = NearestMv.dwValue;
            }
            else 
            {
                i32MvMin = -static_cast<int>(pMbInfo->dwMbPosX << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
                i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
                NearMv.i16X = INTEL_VP9_CLAMP(NearMv.i16X, i32MvMin, i32MvMax);
                i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
                i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
                NearMv.i16Y = INTEL_VP9_CLAMP(NearMv.i16Y, i32MvMin, i32MvMax);

                if (NearMv.dwValue != Mv.dwValue)
                {
                    pMbInfo->NearMv[bIsSecondRef].dwValue = NearMv.dwValue;
                }
                else
                {
                    pMbInfo->NearMv[bIsSecondRef].dwValue = 0;
                }
            }
        }
    }

    return eStatus;
}

static INT16 Intel_HostvldVp9_ParseMvComponent(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine,
    INTEL_HOSTVLD_VP9_MV_COMPONENT   MvComponent, 
    BOOL                                bUseHighPrecisionMv)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    INTEL_HOSTVLD_VP9_MV_CLASS_TYPE  MvClass;
    PINTEL_HOSTVLD_VP9_MV_PROB_SET   pMvProbSet;
    PINTEL_HOSTVLD_VP9_MV_COUNT_SET  pMvCountSet;
    PUINT8                              pui8Probs;
    INT                                 iSign, iInteger, iFractional, iHp;
    INT16                               i16MvComponent = 0;

    pFrameInfo  = &pTileState->pFrameState->FrameInfo;
    pMvProbSet  = &pFrameInfo->pContext->MvProbSet[!MvComponent];
    pMvCountSet = &pTileState->Count.MvCountSet[!MvComponent];

    // read sign bit
    iSign = INTEL_HOSTVLD_VP9_READ_BIT(pMvProbSet->MvSignProbs);
    pMvCountSet->MvSignCounts[iSign]++;

    // read MV class
    pui8Probs = pMvProbSet->MvClassProbs;
    if (INTEL_HOSTVLD_VP9_READ_BIT(pui8Probs[0]))
    {
        if (INTEL_HOSTVLD_VP9_READ_BIT(pui8Probs[1]))
        {
            if (INTEL_HOSTVLD_VP9_READ_BIT(pui8Probs[2]))
            {
                if (INTEL_HOSTVLD_VP9_READ_BIT(pui8Probs[4]))
                {
                    if (INTEL_HOSTVLD_VP9_READ_BIT(pui8Probs[6]))
                    {
                        if (INTEL_HOSTVLD_VP9_READ_BIT(pui8Probs[7]))
                        {
                            MvClass = INTEL_HOSTVLD_VP9_READ_BIT(pui8Probs[9]) ? 
                                VP9_MV_CLASS_10 : VP9_MV_CLASS_9;
                        }
                        else
                        {
                            MvClass = INTEL_HOSTVLD_VP9_READ_BIT(pui8Probs[8]) ? 
                                VP9_MV_CLASS_8 : VP9_MV_CLASS_7;
                        }
                    }
                    else
                    {
                        MvClass = VP9_MV_CLASS_6;
                    }
                }
                else
                {
                    MvClass = INTEL_HOSTVLD_VP9_READ_BIT(pui8Probs[5]) ? 
                        VP9_MV_CLASS_5 : VP9_MV_CLASS_4;
                }
            }
            else
            {
                MvClass = INTEL_HOSTVLD_VP9_READ_BIT(pui8Probs[3]) ? 
                    VP9_MV_CLASS_3 : VP9_MV_CLASS_2;
            }
        }
        else
        {
            MvClass = VP9_MV_CLASS_1;
        }
    }
    else
    {
        MvClass = VP9_MV_CLASS_0;
    }

    pMvCountSet->MvClassCounts[MvClass]++;

    // read integer
    if (MvClass == VP9_MV_CLASS_0)
    {
        iInteger = INTEL_HOSTVLD_VP9_READ_BIT(pMvProbSet->MvClass0Probs[0]);

        pMvCountSet->MvClass0Counts[iInteger]++;
    }
    else
    {
        INT i, iBit, iBits = MvClass + VP9_MV_CLASS0_BITS - 1;

        iInteger = 0;
        for (i = 0; i < iBits; i++)
        {
            iBit = INTEL_HOSTVLD_VP9_READ_BIT(pMvProbSet->MvBitsProbs[i]);
            iInteger |= iBit << i;

            pMvCountSet->MvBitsCounts[i][iBit]++;
        }
    }

    // read fractional
    pui8Probs = (MvClass == VP9_MV_CLASS_0) ? 
        pMvProbSet->MvClass0FpProbs[iInteger] : pMvProbSet->MvFpProbs;
    iFractional = INTEL_HOSTVLD_VP9_READ_BIT(pui8Probs[0]);
    if (iFractional)
    {
        if (INTEL_HOSTVLD_VP9_READ_BIT(pui8Probs[1]))
        {
            iFractional = INTEL_HOSTVLD_VP9_READ_BIT(pui8Probs[2]) ? 3 : 2;
        }
        else
        {
            iFractional = 1;
        }
    }

    if (MvClass == VP9_MV_CLASS_0)
    {
        pMvCountSet->MvClass0FpCounts[iInteger][iFractional]++;
    }
    else
    {
        pMvCountSet->MvFpCounts[iFractional]++;
    }

    // read high precision
    if (bUseHighPrecisionMv)
    {
        if (MvClass == VP9_MV_CLASS_0)
        {
            iHp = INTEL_HOSTVLD_VP9_READ_BIT(pMvProbSet->MvClass0HpProbs);

            pMvCountSet->MvClass0HpCounts[iHp]++;
        }
        else
        {
            iHp = INTEL_HOSTVLD_VP9_READ_BIT(pMvProbSet->MvHpProbs);

            pMvCountSet->MvHpCounts[iHp]++;
        }
    }
    else
    {
        iHp = 1;

        if (MvClass == VP9_MV_CLASS_0)
        {
            pMvCountSet->MvClass0HpCounts[iHp]++;
        }
        else
        {
            pMvCountSet->MvHpCounts[iHp]++;
        }
    }

    // calculate the MV component
    i16MvComponent = (iInteger << 3) | (iFractional << 1) | iHp;
    i16MvComponent += MvClass ? (VP9_MV_CLASS0_SIZE << (MvClass + 2)) : 0;
    i16MvComponent += 1;
    i16MvComponent = iSign ? -i16MvComponent : i16MvComponent;

    return (i16MvComponent << 1);
}

static VAStatus Intel_HostvldVp9_ParseOneMv(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine, 
    BOOL                                bIsSecondRef)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext;
    INTEL_HOSTVLD_VP9_MV_JOINT_TYPE  MvJointType;
    BOOL                                bUseHighPrecisionMv;
    INTEL_HOSTVLD_VP9_MV             BestMv, MvDiff;
    VAStatus                          eStatus   = VA_STATUS_SUCCESS;

    pFrameInfo = &pTileState->pFrameState->FrameInfo;
    pContext   = pFrameInfo->pContext;

    // read MV joint type
    if (INTEL_HOSTVLD_VP9_READ_BIT(pContext->MvJointProbs[0]))
    {
        if (INTEL_HOSTVLD_VP9_READ_BIT(pContext->MvJointProbs[1]))
        {
            MvJointType = INTEL_HOSTVLD_VP9_READ_BIT(pContext->MvJointProbs[2]) ? 
                VP9_MV_JOINT_HNZ_VNZ : VP9_MV_JOINT_HZ_VNZ;
        }
        else
        {
            MvJointType = VP9_MV_JOINT_HNZ_VZ;
        }
    }
    else
    {
        MvJointType = VP9_MV_JOINT_ZERO;
    }

    BestMv = pMbInfo->BestMv[bIsSecondRef];
    bUseHighPrecisionMv = pFrameInfo->bAllowHighPrecisionMv     && 
        ((ABS(BestMv.i16X) >> 4) < VP9_COMPANDED_MVREF_THRESH)  &&
        ((ABS(BestMv.i16Y) >> 4) < VP9_COMPANDED_MVREF_THRESH);

    // read DMV components
    MvDiff.dwValue = 0;

    if ((MvJointType == VP9_MV_JOINT_HNZ_VNZ) || 
        (MvJointType == VP9_MV_JOINT_HZ_VNZ))
    {
        MvDiff.i16Y = Intel_HostvldVp9_ParseMvComponent(
            pTileState, pMbInfo, pBacEngine, VP9_MV_VERTICAL, bUseHighPrecisionMv);
    }

    if ((MvJointType == VP9_MV_JOINT_HNZ_VNZ) || 
        (MvJointType == VP9_MV_JOINT_HNZ_VZ))
    {
        MvDiff.i16X = Intel_HostvldVp9_ParseMvComponent(
            pTileState, pMbInfo, pBacEngine, VP9_MV_HORIZONTAL, bUseHighPrecisionMv);
    }

    // update counter
    //MvJointType = (INTEL_HOSTVLD_VP9_MV_JOINT_TYPE)((MvDiff.i16X != 0) | ((MvDiff.i16Y != 0) << 1));
    pTileState->Count.MvJointCounts[MvJointType] += pFrameInfo->bFrameParallelDisabled;

    pMbInfo->Mv[bIsSecondRef].i16X = BestMv.i16X + MvDiff.i16X;
    pMbInfo->Mv[bIsSecondRef].i16Y = BestMv.i16Y + MvDiff.i16Y;

    return eStatus;
}

static VAStatus Intel_HostvldVp9_ParseMotionVectors(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine, 
    INT                                 iBlockIndex)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_MV            pMv;
    VAStatus                          eStatus   = VA_STATUS_SUCCESS;

    pFrameInfo      = &pTileState->pFrameState->FrameInfo;
    pMv             = pMbInfo->Mv;
    pMv[1].dwValue  = 0;

    if (pMbInfo->ePredMode == PRED_MD_NEWMV)
    {
        Intel_HostvldVp9_FindBestMv(pFrameInfo, pMbInfo, 0);
        Intel_HostvldVp9_ParseOneMv(pTileState, pMbInfo, pBacEngine, 0);

        if (pMbInfo->ReferenceFrame[1] > VP9_REF_FRAME_INTRA)
        {
            Intel_HostvldVp9_FindBestMv(pFrameInfo, pMbInfo, 1);
            Intel_HostvldVp9_ParseOneMv(pTileState, pMbInfo, pBacEngine, 1);
        }
    }
    else if (pMbInfo->ePredMode == PRED_MD_NEARESTMV)
    {
        Intel_HostvldVp9_FindNearestMv(pFrameInfo, pMbInfo, 0, iBlockIndex);
        pMv[0].dwValue = pMbInfo->NearestMv[0].dwValue;

        if (pMbInfo->ReferenceFrame[1] > VP9_REF_FRAME_INTRA)
        {
            Intel_HostvldVp9_FindNearestMv(pFrameInfo, pMbInfo, 1, iBlockIndex);
            pMv[1].dwValue = pMbInfo->NearestMv[1].dwValue;
        }
    }
    else if (pMbInfo->ePredMode == PRED_MD_NEARMV)
    {
        Intel_HostvldVp9_FindNearMv(pFrameInfo, pMbInfo, 0, iBlockIndex);
        pMv[0].dwValue = pMbInfo->NearMv[0].dwValue;

        if (pMbInfo->ReferenceFrame[1] > VP9_REF_FRAME_INTRA)
        {
            Intel_HostvldVp9_FindNearMv(pFrameInfo, pMbInfo, 1, iBlockIndex);
            pMv[1].dwValue = pMbInfo->NearMv[1].dwValue;
        }
    }
    else // PRED_MD_ZEROMV
    {
        pMv[0].dwValue = 0;
        pMv[1].dwValue = 0;
    }

    return eStatus;
}

static VAStatus Intel_HostvldVp9_ParseInterBlockModeInfo(
    PINTEL_HOSTVLD_VP9_TILE_STATE   pTileState)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO pFrameInfo = &pTileState->pFrameState->FrameInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO    pMbInfo    = &pTileState->MbInfo;
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine = &pTileState->BacEngine;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE  BlkSize    = pMbInfo->BlockSize;
    PINTEL_HOSTVLD_VP9_MV         pMotionVector;
    PUINT8                           pPredModeLuma, pPredModeChroma, pTxTypeLuma;
    INT                              iContext   = 0;
    UINT64                           ui64PackedMvs;
    VAStatus                       eStatus    = VA_STATUS_SUCCESS;

    pPredModeLuma   = pMbInfo->pPredModeLuma;
    pPredModeChroma = pMbInfo->pPredModeChroma;
    pTxTypeLuma     = pMbInfo->pTxTypeLuma;
    pMotionVector   = pMbInfo->pMotionVector;

    Intel_HostvldVp9_ParseRefereceFrames(pTileState, pMbInfo, pBacEngine);

    // parse inter mode
    if (pMbInfo->bSegRefSkip)
    {
        pMbInfo->ePredMode = PRED_MD_ZEROMV;
        if (pMbInfo->iB4Number < 4)
        {
            eStatus = VA_STATUS_ERROR_DECODING_ERROR;
            goto finish;
        }
        VP9_PROP4x4(pPredModeLuma, pMbInfo->ePredMode);
        VP9_PROP4x4(pTxTypeLuma, Intel_HostvldVp9_GetTxType(pFrameInfo, pMbInfo));
    }
    else
    {
        iContext = Intel_HostvldVp9_GetInterModeContext(pFrameInfo, pMbInfo);
        if (pMbInfo->iB4Number >= 4)
        {
            Intel_HostvldVp9_ParseInterMode(pTileState, pMbInfo, pBacEngine, iContext);
            VP9_PROP4x4(pPredModeLuma, pMbInfo->ePredMode);
            VP9_PROP4x4(pTxTypeLuma, Intel_HostvldVp9_GetTxType(pFrameInfo, pMbInfo));
        }
    }

    Intel_HostvldVp9_ParseInterpolationType(pTileState, pMbInfo, pBacEngine);
    VP9_PROP8x8(pMbInfo->pFilterType, pMbInfo->eInterpolationType);

    pMbInfo->BestMv[0].dwValue = VP9_INVALID_MV_VALUE;
    pMbInfo->BestMv[1].dwValue = VP9_INVALID_MV_VALUE;

    if (pMbInfo->iB4Number < 4)
    {
        INT iX, iY;
        INT iWidth4x4, iHeight4x4;
        INT iBlockIndex;

        iWidth4x4  = 1 << (g_Vp9BlockSizeB4Log2[BlkSize][0]);
        iHeight4x4 = 1 << (g_Vp9BlockSizeB4Log2[BlkSize][1]);

        for (iY = 0; iY < 2; iY += iHeight4x4)
        {
            for (iX = 0; iX < 2; iX += iWidth4x4)
            {
                Intel_HostvldVp9_ParseInterMode(pTileState, pMbInfo, pBacEngine, iContext);
                VP9_PROP4x4(pPredModeLuma, pMbInfo->ePredMode);
                VP9_PROP4x4(pTxTypeLuma, Intel_HostvldVp9_GetTxType(pFrameInfo, pMbInfo));

                iBlockIndex = iY * 2 + iX;
                Intel_HostvldVp9_ParseMotionVectors(pTileState, pMbInfo, pBacEngine, iBlockIndex);
                ui64PackedMvs = pMbInfo->Mv[0].dwValue;
                *((PDWORD)(&ui64PackedMvs) + 1) = pMbInfo->Mv[1].dwValue;
                VP9_PROP4x4_QWORD((PUINT64)pMotionVector, ui64PackedMvs);

                pPredModeLuma += iWidth4x4;
                pTxTypeLuma   += iWidth4x4;
                pMotionVector += iWidth4x4 << 1;
            }
        }
    }
    else
    {
        Intel_HostvldVp9_ParseMotionVectors(pTileState, pMbInfo, pBacEngine, -1);
        ui64PackedMvs = pMbInfo->Mv[0].dwValue;
        *((PDWORD)(&ui64PackedMvs) + 1) = pMbInfo->Mv[1].dwValue;
        VP9_PROP4x4_QWORD((PUINT64)pMotionVector, ui64PackedMvs);
    }

    VP9_PROP8x8(pMbInfo->pPredModeChroma, pMbInfo->ePredMode);

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_ParseInterFrameModeInfo(
    PINTEL_HOSTVLD_VP9_TILE_STATE   pTileState)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO pFrameInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO    pMbInfo;
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE  BlkSize;

    UINT8  ui8Ctx, ui8LSIP, ui8ASIP, ui8LSkip, ui8ASkip;
    UINT8  ui8SegId = 0, ui8SkipCoeff, ui8IsInter;

    VAStatus eStatus = VA_STATUS_SUCCESS;


    pFrameInfo = &pTileState->pFrameState->FrameInfo;
    pMbInfo    = &pTileState->MbInfo;
    pBacEngine = &pTileState->BacEngine;

    BlkSize = pMbInfo->BlockSize;

    if (!pMbInfo->bAboveValid && !pMbInfo->bLeftValid)
    {
        Intel_HostvldVp9_InitTokenParser(pTileState);
    }
    else
    {
        Intel_HostvldVp9_UpdateTokenParser(pFrameInfo, pMbInfo);
    }

    // block size
    VP9_PROP8x8(pMbInfo->pBlockSize, g_Vp9BlockSizeLookup[BlkSize]);

    if (pFrameInfo->ui8SegEnabled)
    {
        ui8SegId = Intel_HostvldVp9_GetPredSegmentId(pMbInfo->pLastSegmentId, g_Vp9Propagate8x8[BlkSize]);

        if (pFrameInfo->ui8SegUpdMap)
        {
            BOOL bSegPredFlag = FALSE;
            if (pFrameInfo->ui8TemporalUpd)
            {
                ui8ASIP = *(pFrameInfo->pPredSegIdContextAbove + pMbInfo->dwMbPosX);
                ui8LSIP = *(pMbInfo->pCurrTile->PredSegIdContextLeft + pMbInfo->iMbPosInB64Y);
                ui8Ctx  = ui8ASIP + ui8LSIP;

                bSegPredFlag = INTEL_HOSTVLD_VP9_READ_BIT(pTileState->pFrameState->pVp9HostVld->Context.CurrContext.SegPredProbs[ui8Ctx]);
            }

            if (!bSegPredFlag)
            {
                ui8SegId = (UINT8)INTEL_HOSTVLD_VP9_READ_TREE(pFrameInfo->pContext->SegmentTree);
            }

            memset(pFrameInfo->pPredSegIdContextAbove + pMbInfo->dwMbPosX, bSegPredFlag, g_Vp9BlockSizeB8[BlkSize][0]);
            memset(pMbInfo->pCurrTile->PredSegIdContextLeft + pMbInfo->iMbPosInB64Y, bSegPredFlag, g_Vp9BlockSizeB8[BlkSize][1]);
            VP9_PROP8x8(pMbInfo->pLastSegmentId, ui8SegId);
        }
    }
    VP9_PROP8x8(pMbInfo->pSegmentId, ui8SegId);
    VP9_PROP8x8_DWORD(pMbInfo->pQPLuma, pFrameInfo->SegQP[ui8SegId][INTEL_HOSTVLD_VP9_YUV_PLANE_Y]);
    VP9_PROP8x8_DWORD(pMbInfo->pQPChroma, pFrameInfo->SegQP[ui8SegId][INTEL_HOSTVLD_VP9_YUV_PLANE_UV]);
    pMbInfo->bSegRefSkip    = pFrameInfo->ui8SegEnabled &&
        pFrameInfo->pSegmentData->SegData[ui8SegId].SegmentFlags.fields.SegmentReferenceSkipped;
    pMbInfo->bSegRefEnabled = pFrameInfo->ui8SegEnabled &&
        pFrameInfo->pSegmentData->SegData[ui8SegId].SegmentFlags.fields.SegmentReferenceEnabled;

    // read skip flag
    ui8SkipCoeff = 1;
    ui8LSkip     = VP9_L_CTX(pMbInfo->pSkipCoeff, 0);
    ui8ASkip     = VP9_A_CTX(pMbInfo->pSkipCoeff, 0);
    if (!pMbInfo->bSegRefSkip)
    {
        ui8Ctx       = ui8LSkip + ui8ASkip;
        ui8SkipCoeff = (UINT8)INTEL_HOSTVLD_VP9_READ_BIT(pFrameInfo->pContext->MbSkipProbs[ui8Ctx]);

        pTileState->Count.MbSkipCounts[ui8Ctx][ui8SkipCoeff] += pFrameInfo->bFrameParallelDisabled;
    }
    VP9_PROP8x8(pMbInfo->pSkipCoeff, ui8SkipCoeff);
    pMbInfo->bIsSkipped = ui8SkipCoeff;

    // read inter/intra flag
    if (pMbInfo->bSegRefEnabled)
    {
        pMbInfo->i8SegReference =
            pFrameInfo->pSegmentData->SegData[ui8SegId].SegmentFlags.fields.SegmentReference;
        ui8IsInter = (--pMbInfo->i8SegReference != VP9_REF_FRAME_INTRA);
    }
    else
    {
        UINT uiLIsIntra  = pMbInfo->bLeftValid && !pMbInfo->pIsInter[pMbInfo->iLCtxOffset];
        UINT uiLIsInter  = pMbInfo->bLeftValid && pMbInfo->pIsInter[pMbInfo->iLCtxOffset];
        UINT uiAIsIntra  = pMbInfo->bAboveValid && !pMbInfo->pIsInter[pMbInfo->iACtxOffset];
        UINT uiAIsInter  = pMbInfo->bAboveValid && pMbInfo->pIsInter[pMbInfo->iACtxOffset];
        if (!(uiLIsIntra | uiAIsIntra)) // no intra
        {
            ui8Ctx  = 0;
        }
        else // at least one intra
        {
            ui8Ctx  = 1 + (uiLIsIntra + uiAIsIntra) - (uiLIsInter + uiAIsInter);
        }
        ui8IsInter  = (UINT8)INTEL_HOSTVLD_VP9_READ_BIT(pFrameInfo->pContext->IntraInterProbs[ui8Ctx]);

        pTileState->Count.IntraInterCounts[ui8Ctx][ui8IsInter] += pFrameInfo->bFrameParallelDisabled;
    }
    VP9_PROP8x8(pMbInfo->pIsInter, ui8IsInter);
    pMbInfo->bIsInter = ui8IsInter;

    // read transform size
    Intel_HostvldVp9_ParseTransformSize(pTileState, pMbInfo, pBacEngine, ui8LSkip, ui8ASkip);

    // parse block mode info
    if (ui8IsInter)
    {
        eStatus = Intel_HostvldVp9_ParseInterBlockModeInfo(pTileState);
    }
    else
    {
        eStatus = Intel_HostvldVp9_ParseIntraBlockModeInfo(pTileState);
    }

finish:
    return eStatus;
}

// Function: Parse Coefficient for one macroblock
// [In/Out]: pVp9State
// [In]: iSubOffsetZOrder: the z-order offset [0, 1, 2, 3] in 8x8 block. 
//       iSubOffsetIn8x8 and MbInfo->dwMbOffset determines the 4x4 block offset. It is in Luma.
// [In]: iSubOffsetX: [0, 1]; iSubOffsetY: [0, 1]
VAStatus Intel_HostvldVp9_ParseCoefficient(
    PINTEL_HOSTVLD_VP9_TILE_STATE pTileState, 
    INT                              iSubOffsetZOrder)
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_TILE_INFO     pTileInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo;
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlkSize;

    INT         iPlane, i, j, iTxCol, iTxRow, Pt = 0;
    BOOL        bSkipCoeffFlag, bIsInterFlag;
    UCHAR       TxSizeChroma, TxSize, TxType=0;
    PUCHAR      pTxTypeBuf, pTxTypeBufBase;
    UINT        nEobMax;
    PUINT8      pAboveContext, pLeftContext;
    PUINT8      pCatProb;
    PINT16      pCoeffAddr, pCoeffAddrBase;
    PUINT8      pCoeffStatusAddr, pCoeffStatusAddrBase;
    PUINT16     pZigzagBuf;
    INT         Subsampling_x, Subsampling_y;
    INT         CoeffOffset, CoeffStatusOffset;
    INT         iWidth4x4, iHeight4x4;

    INTEL_HOSTVLD_VP9_BAC_VALUE BacValue;
    INT  iCount;
    INT  iBit;
    UINT uiSplit, uiRange, uiShift;
    INTEL_HOSTVLD_VP9_BAC_VALUE BacSplitValue;
    INT  OffsetX, OffsetY, MbRightToFrameRight, ValidBlockWidthIn4x4;
    INT  MbBottomToFrameBottom, ValidBlockHeightIn4x4;
    UINT PlaneType;

    INT32       Band, CoeffIdx, i32EobThreshold;
    VP9_SCAN_NEIGHBOR_BANDTRANS ScanNbBandTrans;
    PUINT8          pBandTranslate;
    const UINT8     *pProb;
    INT             iEntropyIdx;
    UINT64          u64Value;

    VAStatus  eStatus   = VA_STATUS_SUCCESS;


    pFrameState = pTileState->pFrameState;
    pFrameInfo  = &pFrameState->FrameInfo;
    pMbInfo     = &pTileState->MbInfo;
    pTileInfo   = pMbInfo->pCurrTile;
    pBacEngine  = &pTileState->BacEngine;


    BlkSize        = (pMbInfo->iB4Number < 4) ? BLOCK_8X8 : pMbInfo->BlockSize;
    bSkipCoeffFlag = pMbInfo->bIsSkipped;

    iWidth4x4  = 1 << (g_Vp9BlockSizeB4Log2[BlkSize][0]);
    iHeight4x4 = 1 << (g_Vp9BlockSizeB4Log2[BlkSize][1]);

    if(bSkipCoeffFlag)
    {
        // Reset pMbInfo planes above_context/left_context.
        switch(iWidth4x4)
        {
        case 2:
            *(PUINT16)(pMbInfo->pAboveContext[INTEL_HOSTVLD_VP9_YUV_PLANE_Y]) = 0;
            *(pMbInfo->pAboveContext[INTEL_HOSTVLD_VP9_YUV_PLANE_U])          = 0;
            *(pMbInfo->pAboveContext[INTEL_HOSTVLD_VP9_YUV_PLANE_V])          = 0;
            break;
        case 4:
            *(PUINT32)(pMbInfo->pAboveContext[INTEL_HOSTVLD_VP9_YUV_PLANE_Y]) = 0;
            *(PUINT16)(pMbInfo->pAboveContext[INTEL_HOSTVLD_VP9_YUV_PLANE_U]) = 0;
            *(PUINT16)(pMbInfo->pAboveContext[INTEL_HOSTVLD_VP9_YUV_PLANE_V]) = 0;
            break;
        case 8:
            *(PUINT64)(pMbInfo->pAboveContext[INTEL_HOSTVLD_VP9_YUV_PLANE_Y]) = 0;
            *(PUINT32)(pMbInfo->pAboveContext[INTEL_HOSTVLD_VP9_YUV_PLANE_U]) = 0;
            *(PUINT32)(pMbInfo->pAboveContext[INTEL_HOSTVLD_VP9_YUV_PLANE_V]) = 0;
            break;
        case 16:
            *(PUINT64)(pMbInfo->pAboveContext[INTEL_HOSTVLD_VP9_YUV_PLANE_Y])     = 0;
            *(PUINT64)(pMbInfo->pAboveContext[INTEL_HOSTVLD_VP9_YUV_PLANE_Y] + 8) = 0;
            *(PUINT64)(pMbInfo->pAboveContext[INTEL_HOSTVLD_VP9_YUV_PLANE_U])     = 0;
            *(PUINT64)(pMbInfo->pAboveContext[INTEL_HOSTVLD_VP9_YUV_PLANE_V])     = 0;
            break;
        default:
            assert(0); 
            break;
        }

        switch(iHeight4x4)
        {
        case 2:
            *(PUINT16)(pMbInfo->pLeftContext[INTEL_HOSTVLD_VP9_YUV_PLANE_Y]) = 0;
            *(pMbInfo->pLeftContext[INTEL_HOSTVLD_VP9_YUV_PLANE_U])          = 0;
            *(pMbInfo->pLeftContext[INTEL_HOSTVLD_VP9_YUV_PLANE_V])          = 0;
            break;
        case 4:
            *(PUINT32)(pMbInfo->pLeftContext[INTEL_HOSTVLD_VP9_YUV_PLANE_Y]) = 0;
            *(PUINT16)(pMbInfo->pLeftContext[INTEL_HOSTVLD_VP9_YUV_PLANE_U]) = 0;
            *(PUINT16)(pMbInfo->pLeftContext[INTEL_HOSTVLD_VP9_YUV_PLANE_V]) = 0;
            break;
        case 8:
            *(PUINT64)(pMbInfo->pLeftContext[INTEL_HOSTVLD_VP9_YUV_PLANE_Y]) = 0;
            *(PUINT32)(pMbInfo->pLeftContext[INTEL_HOSTVLD_VP9_YUV_PLANE_U]) = 0;
            *(PUINT32)(pMbInfo->pLeftContext[INTEL_HOSTVLD_VP9_YUV_PLANE_V]) = 0;
            break;
        case 16:
            *(PUINT64)(pMbInfo->pLeftContext[INTEL_HOSTVLD_VP9_YUV_PLANE_Y])     = 0;
            *(PUINT64)(pMbInfo->pLeftContext[INTEL_HOSTVLD_VP9_YUV_PLANE_Y] + 8) = 0;
            *(PUINT64)(pMbInfo->pLeftContext[INTEL_HOSTVLD_VP9_YUV_PLANE_U])     = 0;
            *(PUINT64)(pMbInfo->pLeftContext[INTEL_HOSTVLD_VP9_YUV_PLANE_V])     = 0;
            break;
        default: 
            assert(0); 
            break;
        }
    }
    else
    {
        // load BAC engine context
        uiRange  = pBacEngine->uiRange;
        BacValue = pBacEngine->BacValue;
        iCount   = pBacEngine->iCount;

        // Read TX size and IsInterFlag from buffers
        TxSize = *(pMbInfo->pTxSizeLuma);
        bIsInterFlag = *(pMbInfo->pIsInter);
        TxSizeChroma = *(pMbInfo->pTxSizeChroma);


        // Calc Luma Coeff Offset and Luma Coeff Status Offset for this partition block
        CoeffOffset       = (pMbInfo->dwMbOffset + iSubOffsetZOrder) << 6;
        CoeffStatusOffset = CoeffOffset >> 4;

        for(iPlane = 0; iPlane < INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER + 1; iPlane++)
        {
            // Split the partition block into TX blocks
            if(!iPlane) // Y Plane
            {
                Subsampling_x = Subsampling_y = 0;
                pCoeffAddrBase       = (PINT16)(pFrameState->pOutputBuffer->TransformCoeff[iPlane].pu16Buffer) + CoeffOffset;
                pCoeffStatusAddrBase = pFrameState->pOutputBuffer->CoeffStatus[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer + CoeffStatusOffset;
                pTxTypeBufBase       = pMbInfo->pTxTypeLuma;
            }
            else // UV plane
            {
                Subsampling_x = Subsampling_y = 1;
                TxSize = TxSizeChroma;
                TxType = TX_DCT;
                pCoeffStatusAddrBase = pFrameState->pOutputBuffer->CoeffStatus[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer + (CoeffStatusOffset >> 2);
                pCoeffAddrBase = (PINT16)(pFrameState->pOutputBuffer->TransformCoeff[iPlane].pu16Buffer) + (CoeffOffset >> 2);
                pTxTypeBufBase = NULL;
            }

            iTxCol = 1 << (g_Vp9BlockSizeB4Log2[BlkSize][0] - TxSize - Subsampling_x);
            iTxRow = 1 << (g_Vp9BlockSizeB4Log2[BlkSize][1] - TxSize - Subsampling_y);

            i32EobThreshold = (TxSize > TX_16X16) ? 9 : 10;

            // Find the Z-order table
            pZigzagBuf = NULL;
            if(iTxRow == (iTxCol << 1)) // Vertical Case
            {
                switch(iTxCol) 
                {
                case 1:
                    pZigzagBuf = g_Vp9TxBlockIndex2ZOrderIndexMapVer1x2;
                    break;
                case 2:
                    pZigzagBuf = g_Vp9TxBlockIndex2ZOrderIndexMapVer2x4;
                    break;
                case 4:
                    pZigzagBuf = g_Vp9TxBlockIndex2ZOrderIndexMapVer4x8;
                    break;
                case 8:
                    pZigzagBuf = g_Vp9TxBlockIndex2ZOrderIndexMapVer8x16;
                    break;
                default:
		    assert(0);
                    break;
                }
            }
            else // Horizontal and Square Cases
            {
                switch(iTxCol)
                {
                case 1:
                    pZigzagBuf = g_Vp9TxBlockIndex2ZOrderIndexMapSquare1;
                    break;
                case 2:
                    pZigzagBuf = g_Vp9TxBlockIndex2ZOrderIndexMapSquare4;
                    break;
                case 4:
                    pZigzagBuf = g_Vp9TxBlockIndex2ZOrderIndexMapSquare16;
                    break;
                case 8:
                    pZigzagBuf = g_Vp9TxBlockIndex2ZOrderIndexMapSquare64;
                    break;
                case 16:
                    pZigzagBuf = g_Vp9TxBlockIndex2ZOrderIndexMapSquare256;
                    break;
                default:
            	    assert(0); 
                    break;
                }
            }

            // Calc MB location and out-of-frame-boundary information
            OffsetX = (pMbInfo->dwMbPosX << 3);
            OffsetY = (pMbInfo->dwMbPosY << 3);
            MbRightToFrameRight = pFrameInfo->dwPicWidth - OffsetX - (iWidth4x4 << 2);
            ValidBlockWidthIn4x4 = (iWidth4x4 >> Subsampling_x);
            if(MbRightToFrameRight < 0)
            {
                ValidBlockWidthIn4x4 += (MbRightToFrameRight >> (2 + Subsampling_x));
            }
            MbBottomToFrameBottom = pFrameInfo->dwPicHeight - ((pMbInfo->dwMbPosY << 3) + (iHeight4x4 << 2));
            ValidBlockHeightIn4x4 = (iHeight4x4 >> Subsampling_y);
            if(MbBottomToFrameBottom < 0)
            {
                ValidBlockHeightIn4x4 += (MbBottomToFrameBottom >> (2 + Subsampling_y));
            }

            // Loop through each TX blocks in raster scan order
            for(j = 0; j < iTxRow; j++)
            {
                for(i = 0; i < iTxCol; i++)
                {
                    // Bypass if TX block left-up corner exceeds the frame boundary
                    // TX block width = TX block height = 1<<(TxSize + 2)
                    if(((OffsetX >> Subsampling_x)  + (i << (TxSize + 2)) > (INT)((pFrameInfo->dwPicWidth >> Subsampling_x) - 1))
                            || ((OffsetY >> Subsampling_y) + (j << (TxSize + 2)) > (INT)((pFrameInfo->dwPicHeight >> Subsampling_y) - 1)))
                    {
                        continue;
                    }
                                        
                    // Calc Buffer offset for current TX block and initialize the TX block coeff memory to 0
                    // Chroma offset increases 2x TX block size since U & V interlaced                   
                    pCoeffAddr = pCoeffAddrBase + (pZigzagBuf[i + j * iTxCol] * (1 << ((TxSize + 2) << 1)));

                    memset(pCoeffAddr, 0, (2 << ((TxSize + 2) << 1)));

                    pCoeffStatusAddr = pCoeffStatusAddrBase + (pZigzagBuf[i + j * iTxCol] * (1 << (TxSize << 1)));

                    // Tx type reading per 4x4 for TX_4X4
                    if(pTxTypeBufBase) // Y Plane
                    {
                        pTxTypeBuf = pTxTypeBufBase;
                        if(TxSize == TX_4X4)
                        {
                            pTxTypeBuf += pZigzagBuf[i + j * iTxCol];
                        }
                        TxType = *pTxTypeBuf;
                    }

                    // Max coeff number of this TX block
                    nEobMax = pMbInfo->bSegRefSkip? 0 : (1 << ((TxSize + 2) << 1));
                        
                    // TxSize = log2(TxWidth/4) = log2(TxWidth in 4x4 block) = log2(TxHeight in 4x4 block)
                    pAboveContext = pMbInfo->pAboveContext[iPlane] + (1<<TxSize) * i;
                    pLeftContext  = pMbInfo->pLeftContext[iPlane]  + (1<<TxSize) * j;

                    // Calc entropy context pt
                    switch(TxSize)
                    {
                    case TX_4X4:
                        Pt = ((*pAboveContext) != 0) + ((*pLeftContext) != 0);
                        break;
                    case TX_8X8:
                        Pt = ((*(PUINT16)pAboveContext) != 0) + ((*(PUINT16)pLeftContext) != 0);
                        break;
                    case TX_16X16:
                        Pt = ((*(PUINT32)pAboveContext) != 0) + ((*(PUINT32)pLeftContext) != 0);
                        break;
                    case TX_32X32:
                        Pt = ((*(PUINT64)pAboveContext) != 0) + ((*(PUINT64)pLeftContext) != 0);
                        break;
                    default: 
            	    assert(0); 
                        break;
                    }

                    // Probability & counters
                    PlaneType = (iPlane == INTEL_HOSTVLD_VP9_YUV_PLANE_Y) ? 0 : 1;                    
                    const UINT8 (*pCoeffProbs)[VP9_PREV_COEF_CONTEXTS][VP9_UNCONSTRAINED_NODES] = pFrameInfo->pContext->CoeffProbs[TxSize][PlaneType][bIsInterFlag];
                    UINT        (*pCoeffCounts)[VP9_PREV_COEF_CONTEXTS][VP9_UNCONSTRAINED_NODES + 1] = pTileState->Count.CoeffCounts[TxSize][PlaneType][bIsInterFlag];
                    UINT        (*pEobBranchCount)[VP9_PREV_COEF_CONTEXTS] = pTileState->Count.EobBranchCounts[TxSize][PlaneType][bIsInterFlag];
                    UCHAR       LoadMapFlag[VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS] = { { 0 } };
                    
                    Band = 0, CoeffIdx = 0;// Reset index at the beginning of a TX block
                    ScanNbBandTrans = g_Vp9ScanNeighborBandTransTable[TxType][TxSize];
                    const PINT16 pScan = ScanNbBandTrans.pScanTable;
                    const PINT16 pNeighbor = ScanNbBandTrans.pNeighborTable;
                    pBandTranslate = ScanNbBandTrans.pCoeffBandTranslate;

                    while (CoeffIdx < (INT)nEobMax)
                    {
                        INT val;
                        if (CoeffIdx)
                        {
                            Pt = (1 + pTileInfo->TokenCache[pNeighbor[(CoeffIdx * VP9_MAX_NEIGHBORS) + 0]] + pTileInfo->TokenCache[pNeighbor[(CoeffIdx * VP9_MAX_NEIGHBORS) + 1]]) >> 1;
                        }
                        Band = *pBandTranslate++;
                        pProb = pCoeffProbs[Band][Pt];
                        pEobBranchCount[Band][Pt] += pFrameInfo->bFrameParallelDisabled;

                        INTEL_HOSTVLD_VP9_READ_COEFF_BIT(pProb[VP9_EOB_CONTEXT_NODE]);
                        if (!iBit)
                        {
                            pCoeffCounts[Band][Pt][VP9_DCT_EOB_MODEL_TOKEN] += pFrameInfo->bFrameParallelDisabled;

                            break;
                        }

                        do
                        {
                            INTEL_HOSTVLD_VP9_READ_COEFF_BIT(pProb[VP9_ZERO_CONTEXT_NODE]);
                            if (iBit)
                            {
                                break;
                            }

                            // Increase Coeff Counters for VP9_ZERO_TOKEN
                            pCoeffCounts[Band][Pt][VP9_ZERO_TOKEN] += pFrameInfo->bFrameParallelDisabled;

                            // Update Token Cache
                            pTileInfo->TokenCache[pScan[CoeffIdx]] = g_Vp9PtEnergyClass[VP9_ZERO_TOKEN];
                            ++CoeffIdx;

                            if (CoeffIdx >= (INT)nEobMax)
                            {
                                break;
                            }
                            if (CoeffIdx)
                            {
                                Pt = (1 + pTileInfo->TokenCache[pNeighbor[(CoeffIdx * VP9_MAX_NEIGHBORS) + 0]] + pTileInfo->TokenCache[pNeighbor[(CoeffIdx * VP9_MAX_NEIGHBORS) + 1]]) >> 1;
                            }
                            Band = *pBandTranslate++;
                            pProb = pCoeffProbs[Band][Pt];
                        } while (1);

                        // ONE_CONTEXT_NODE_0_
                        INTEL_HOSTVLD_VP9_READ_COEFF_BIT(pProb[VP9_ONE_CONTEXT_NODE]);
                        if (!iBit) {
                            // Write Coeff S1 and Continue to next coeff
                            pCoeffCounts[Band][Pt][VP9_ONE_TOKEN] += pFrameInfo->bFrameParallelDisabled;
                            VP9_WRITE_COEF_CONTINUE(1, VP9_ONE_TOKEN);
                        }

                        pCoeffCounts[Band][Pt][VP9_TWO_TOKEN] += pFrameInfo->bFrameParallelDisabled;

                        pProb = g_Vp9ModelCoefProbsPareto8[pProb[VP9_PIVOT_NODE] - 1];

                        // LOW_VAL_CONTEXT_NODE_0_
                        INTEL_HOSTVLD_VP9_READ_COEFF_BIT(pProb[VP9_LOW_VAL_CONTEXT_NODE]);
                        if (!iBit) 
                        {
                            INTEL_HOSTVLD_VP9_READ_COEFF_BIT(pProb[VP9_TWO_CONTEXT_NODE]);
                            if (!iBit) 
                            {
                                // Write Coeff S2 and Continue to next coeff
                                VP9_WRITE_COEF_CONTINUE(2, VP9_TWO_TOKEN);
                            }
                            INTEL_HOSTVLD_VP9_READ_COEFF_BIT(pProb[VP9_THREE_CONTEXT_NODE]);
                            if (!iBit) 
                            {
                                // Write Coeff S3 and Continue to next coeff
                                VP9_WRITE_COEF_CONTINUE(3, VP9_THREE_TOKEN);
                            }

                            // Write Coeff S4 and Continue to next coeff
                            VP9_WRITE_COEF_CONTINUE(4, VP9_FOUR_TOKEN);
                        }
                        // HIGH_LOW_CONTEXT_NODE_0_
                        INTEL_HOSTVLD_VP9_READ_COEFF_BIT(pProb[VP9_HIGH_LOW_CONTEXT_NODE]);
                        if (!iBit) 
                        {
                            INTEL_HOSTVLD_VP9_READ_COEFF_BIT(pProb[VP9_CAT_ONE_CONTEXT_NODE]);
                            if (!iBit) 
                            {
                                // Parse category1
                                INTEL_HOSTVLD_VP9_READ_COEFF_BIT(VP9_CAT1_PROB0);
                                val = VP9_CAT1_MIN_VAL + iBit;
                                VP9_WRITE_COEF_CONTINUE(val, VP9_DCT_VAL_CATEGORY1);
                            }

                            // Update BAC engine context since VP9_PARSE_CAT_COEF_CONTINUE needs pBacEngine
                            pBacEngine->BacValue = BacValue;
                            pBacEngine->iCount   = iCount;
                            pBacEngine->uiRange  = uiRange;

                            // Parse category2
                            pCatProb = g_Vp9Cat2Prob;
                            VP9_PARSE_CAT_COEF_CONTINUE(VP9_CAT2_MIN_VAL, VP9_DCT_VAL_CATEGORY2);
                        }

                        // Update BAC engine context since VP9_PARSE_CAT_COEF_CONTINUE needs pBacEngine
                        pBacEngine->BacValue = BacValue;
                        pBacEngine->iCount   = iCount;
                        pBacEngine->uiRange  = uiRange;
                        // CAT_THREEFOUR_CONTEXT_NODE_0_
                        if (!INTEL_HOSTVLD_VP9_READ_BIT(pProb[VP9_CAT_THREEFOUR_CONTEXT_NODE])) 
                        {
                            if (!INTEL_HOSTVLD_VP9_READ_BIT(pProb[VP9_CAT_THREE_CONTEXT_NODE])) 
                            {
                                // Parse category3
                                pCatProb = g_Vp9Cat3Prob;
                                VP9_PARSE_CAT_COEF_CONTINUE(VP9_CAT3_MIN_VAL, VP9_DCT_VAL_CATEGORY3);
                            }

                            // Parse Category4
                            pCatProb = g_Vp9Cat4Prob;
                            VP9_PARSE_CAT_COEF_CONTINUE(VP9_CAT4_MIN_VAL, VP9_DCT_VAL_CATEGORY4);
                        }
                        // CAT_FIVE_CONTEXT_NODE_0_:
                        if (!INTEL_HOSTVLD_VP9_READ_BIT(pProb[VP9_CAT_FIVE_CONTEXT_NODE])) 
                        {
                            // Parse Category5
                            pCatProb = g_Vp9Cat5Prob;
                            VP9_PARSE_CAT_COEF_CONTINUE(VP9_CAT5_MIN_VAL, VP9_DCT_VAL_CATEGORY5);
                        }

                        // Parse Category6
                        pCatProb = g_Vp9Cat6Prob;
                        VP9_PARSE_CAT_COEF_CONTINUE(VP9_CAT6_MIN_VAL, VP9_DCT_VAL_CATEGORY6);
                    } //while (CoeffIdx < nEobMax)

                    // Write coefficient status
                    if (iPlane > INTEL_HOSTVLD_VP9_YUV_PLANE_U) // V
                    {
                        if (CoeffIdx < 2)
                        {
                            *pCoeffStatusAddr |= (UINT8)CoeffIdx << 4;
                        }
                        else if (CoeffIdx <= i32EobThreshold)
                        {
                            *pCoeffStatusAddr |= 2 << 4;
                        }
                        else if (CoeffIdx <= 34)
                        {
                            *pCoeffStatusAddr |= 3 << 4;
                        }
                        else
                        {
                            *pCoeffStatusAddr |= 4 << 4;
                        }
                    }
                    else // Y or U
                    {
                        if (CoeffIdx < 2)
                        {
                            *pCoeffStatusAddr = (UINT8)CoeffIdx;
                        }
                        else if (CoeffIdx <= i32EobThreshold)
                        {
                            *pCoeffStatusAddr = 2;
                        }
                        else if (CoeffIdx <= 34)
                        {
                            *pCoeffStatusAddr = 3;
                        }
                        else
                        {
                            *pCoeffStatusAddr = 4;
                        }
                    }

                    // Set Entropy Context                    
                    u64Value = (CoeffIdx > 0) ? 0x0101010101010101 : 0;
                    // above context, if the current TX block right edge is out of the frame boundary, only set the entropy context within frame boundary
                    if((CoeffIdx > 0) && ((1<<TxSize) * (i + 1) > ValidBlockWidthIn4x4))
                    {
                        INT ValidTxWidthIn4x4 = ValidBlockWidthIn4x4 - (1 << TxSize) * i;
                        for(iEntropyIdx = 0; iEntropyIdx < ValidTxWidthIn4x4; iEntropyIdx++)
                        {
                            *(pAboveContext + iEntropyIdx) = 1;
                        }
                    }
                    else
                    {
                        switch(TxSize)
                        {
                        case TX_4X4:
                            *(pAboveContext) = (UINT8)(u64Value >> 56);
                            break;
                        case TX_8X8:
                            *(PUINT16)(pAboveContext) = (UINT16)(u64Value >> 48);
                            break;
                        case TX_16X16:
                            *(PUINT32)(pAboveContext) = (UINT32)(u64Value >> 32);
                            break;
                        case TX_32X32:
                            *(PUINT64)(pAboveContext) = u64Value;
                            break;
                        default:
                            assert(0); 
                            break;
                        }
                    }

                    // left context, if the current TX block bottom edge is out of the frame boundary, only set the entropy context within frame boundary                    
                    if((CoeffIdx > 0) && ((1 << TxSize) * (j + 1) > ValidBlockHeightIn4x4))
                    {
                        INT ValidTxHeightIn4x4 = ValidBlockHeightIn4x4 - (1<<TxSize) * j;
                        for(iEntropyIdx = 0; iEntropyIdx < ValidTxHeightIn4x4; iEntropyIdx++)
                        {
                            *(pLeftContext + iEntropyIdx) = 1;
                        }
                    }
                    else
                    {
                        switch(TxSize)
                        {
                        case TX_4X4:
                            *(pLeftContext) = (UINT8)(u64Value >> 56);
                            break;
                        case TX_8X8:
                            *(PUINT16)(pLeftContext) = (UINT16)(u64Value >> 48);
                            break;
                        case TX_16X16:
                            *(PUINT32)(pLeftContext) = (UINT32)(u64Value >> 32);
                            break;
                        case TX_32X32:
                            *(PUINT64)(pLeftContext) = u64Value;
                            break;
                        default: 
                            assert(0); 
                            break;
                        }
                    }
                } // end of for(i)
            } // end of for(j)
        } // for(iPlane)        

        // Update BAC engine context
        pBacEngine->BacValue = BacValue;
        pBacEngine->iCount   = iCount;
        pBacEngine->uiRange  = uiRange;
    } //!bSkipCoeffFlag

finish:
    return eStatus;

}
#ifndef SEPERATE_LOOPFILTER_ENABLE
// NOTE: calling flow for loop filter:
// Intel_HostvldVp9_ParseLoopFilterLevelAndMaskInSingleBlock: output: filter level in kernel required layout, mask in SB structure
// Intel_HostvldVp9_ParseLoopFilterMaskInSuperBlock: output: mask in kernel required layout
// Intel_HostvldVp9_ParseLoopFilterThreshold: output: threshold in kernel required layout

VAStatus Intel_HostvldVp9_ParseLoopFilterLevelAndMaskInSingleBlock(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState)
{
    VAStatus  eStatus;
    PINTEL_HOSTVLD_VP9_FRAME_INFO pFrameInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO    pMbInfo;
    PINTEL_VP9_SEGMENT_PARAMS   pSegmentData;
    PINTEL_HOSTVLD_VP9_LOOP_FILTER_MASK pLoopFilterMaskSB;
    INT Mode, iWidth8x8, iHeight8x8, RowOffset8B, ColOffset8B;
    UCHAR FilterLevel;
    PUINT64 pMaskLeftY, pMaskAboveY, pMaskInt4x4Y;
    PUINT16 pMaskLeftUv, pMaskAboveUv, pMaskInt4x4Uv;
    DWORD nFilterLevelStride;
    PUINT8 pLoopFilterLevel;
    INT OffsetXInB8, OffsetYInB8, ShiftY, ShiftUv, YMaskOnlyFlag, i, Index;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE  BlkSize;
    UCHAR bSkipCoeffFlag;  
    UCHAR TxSize;
    UCHAR TxSizeChroma;
    CHAR  RefFrame;
    UCHAR Seg;
    UCHAR PredModeLuma;

    eStatus = VA_STATUS_SUCCESS;

    pFrameInfo = &pFrameState->FrameInfo;
    pMbInfo    = &pFrameState->MbInfo;

    // Read Skip Coeff Flag, TX size and IsInterFlag from buffers
    BlkSize = pMbInfo->BlockSize;
    bSkipCoeffFlag  = *(pMbInfo->pSkipCoeff);
    TxSize          = *(pMbInfo->pTxSizeLuma);
    TxSizeChroma    = *(pMbInfo->pTxSizeChroma);
    RefFrame        = pMbInfo->ReferenceFrame[0];
    Seg             = *(pMbInfo->pSegmentId);
    PredModeLuma    = *(pMbInfo->pPredModeLuma + 3);// Note that pred mode is stored in the last address of 4x4 granularity z-order buffer

    pSegmentData = pFrameInfo->pSegmentData;
    pLoopFilterMaskSB = pMbInfo->pLoopFilterMaskSB;


    Mode = g_Vp9ModeLoopFilterLut[PredModeLuma];
    FilterLevel = pSegmentData->SegData[Seg].FilterLevel[RefFrame + 1][Mode];
    pMaskLeftY      = &pLoopFilterMaskSB->LeftY[TxSize];
    pMaskAboveY     = &pLoopFilterMaskSB->AboveY[TxSize];
    pMaskInt4x4Y    = &pLoopFilterMaskSB->Int4x4Y;
    pMaskLeftUv     = &pLoopFilterMaskSB->LeftUv[TxSizeChroma];
    pMaskAboveUv    = &pLoopFilterMaskSB->AboveUv[TxSizeChroma];
    pMaskInt4x4Uv   = &pLoopFilterMaskSB->Int4x4Uv;

    iWidth8x8   = (g_Vp9BlockSizeB4Log2[BlkSize][0] > 0)? (1 << (g_Vp9BlockSizeB4Log2[BlkSize][0] - 1)) : 1;
    iHeight8x8  = (g_Vp9BlockSizeB4Log2[BlkSize][1] > 0)? (1 << (g_Vp9BlockSizeB4Log2[BlkSize][1] - 1)) : 1;
    
    ////////////////////////////////////////////////////////
    // Pack filter level surface per kernel required layout
    ////////////////////////////////////////////////////////
    nFilterLevelStride = pFrameState->pOutputBuffer->FilterLevel.dwPitch;
    pLoopFilterLevel = (PUINT8)(pFrameState->pOutputBuffer->FilterLevel.pu8Buffer);
    for(RowOffset8B = 0; RowOffset8B < iHeight8x8; RowOffset8B++)
    {
        for(ColOffset8B = 0; ColOffset8B < iWidth8x8; ColOffset8B++)
        {
            pLoopFilterLevel[(pMbInfo->dwMbPosY + RowOffset8B) * nFilterLevelStride + (pMbInfo->dwMbPosX + ColOffset8B)] = FilterLevel;
        }
    }

    /////////////////////////
    // Calc loop filter mask
    /////////////////////////
    // Calc shift_y & shift_uv according to current block location in its SB
    // shift_y & shift_uv are the index of 8x8 block in raster scan order
    OffsetXInB8 = pMbInfo->dwMbPosX & 0x7;
    OffsetYInB8 = pMbInfo->dwMbPosY & 0x7;
    ShiftY  = (OffsetYInB8 << 3) + OffsetXInB8;
    ShiftUv = (OffsetYInB8 << 1) + (OffsetXInB8 >> 1);

    // Judge if UV mask needs to calc for this block
    YMaskOnlyFlag = (OffsetXInB8 & 0x1) || (OffsetYInB8 & 0x1);

    if(!FilterLevel)
    {
        return eStatus; //No need to do loop filter
    }
    else
    {
        Index = ShiftY;
        for (i = 0; i < iHeight8x8; i++)
        {
            memset(&pLoopFilterMaskSB->LfLevelY[Index], FilterLevel, iWidth8x8);
            Index += 8;
        }
    }

    *pMaskAboveY |= g_Vp9AbovePredictionMask[BlkSize] << ShiftY;    
    *pMaskLeftY |= g_Vp9LeftPredictionMask[BlkSize] << ShiftY;
    if(!YMaskOnlyFlag)
    {
        *pMaskAboveUv |= g_Vp9AbovePredictionMaskUv[BlkSize] << ShiftUv;
        *pMaskLeftUv |= g_Vp9LeftPredictionMaskUv[BlkSize] << ShiftUv;
    }    

    if (bSkipCoeffFlag && RefFrame > VP9_REF_FRAME_INTRA)
    {
        return eStatus;
    }

    // add a mask for the transform size
    *pMaskAboveY |= (g_Vp9SizeMask[BlkSize] & g_Vp9Above64x64TxMask[TxSize]) << ShiftY;
    *pMaskLeftY |= (g_Vp9SizeMask[BlkSize] & g_Vp9Left64x64TxMask[TxSize]) << ShiftY;
    if(!YMaskOnlyFlag)
    {
        *pMaskAboveUv |= (g_Vp9SizeMaskUv[BlkSize] & g_Vp9Above64x64TxMaskUv[TxSizeChroma]) << ShiftUv;
        *pMaskLeftUv |= (g_Vp9SizeMaskUv[BlkSize] & g_Vp9Left64x64TxMaskUV[TxSizeChroma]) << ShiftUv;
    }

    if (TxSize == TX_4X4)
    {
        *pMaskInt4x4Y |= (g_Vp9SizeMask[BlkSize] & 0xffffffffffffffff) << ShiftY;
    }
    if (TxSizeChroma == TX_4X4 && (!YMaskOnlyFlag))
    {
        *pMaskInt4x4Uv |= (g_Vp9SizeMaskUv[BlkSize] & 0xffff) << ShiftUv;
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_ParseLoopFilterMaskInSuperBlock(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState,
    DWORD   dwB8Row,
    DWORD   dwB8Col,
    DWORD   dwTileBottomB8,
    DWORD   dwTileRightB8)
{
    VAStatus  eStatus ;
    INT    i;
    UINT64 LeftY4x4, LeftY8x8, LeftY16x16, Int4x4Y;
    UINT64 AboveY4x4, AboveY8x8, AboveY16x16;
    UINT8  MaskPosition, MaskPositionX, MaskPositionY;
    UINT8  uiMaskVertical, uiMaskHorizontal;
    UINT16 LeftUv4x4, LeftUv8x8, LeftUv16x16, Int4x4Uv;
    UINT16 AboveUv4x4, AboveUv8x8, AboveUv16x16;
    PUINT8 pMaskYVertical, pMaskYHorizontal, pMaskUvVertical, pMaskUvHorizontal;
    UINT ValidRowsWithinSB, ValidColumnsWithinSB;
    UINT nMaskYVertStride, nMaskYHorStride, nMaskUVVertStride, nMaskUVHorStride;
    PINTEL_HOSTVLD_VP9_FRAME_INFO pFrameInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO    pMbInfo;
    PINTEL_HOSTVLD_VP9_LOOP_FILTER_MASK pLoopFilterMaskSB;

    eStatus   = VA_STATUS_SUCCESS;

    pFrameInfo = &pFrameState->FrameInfo;
    pMbInfo    = &pFrameState->MbInfo;

    pMaskYVertical       = (PUINT8)(pFrameState->pOutputBuffer->VerticalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer);
    pMaskYHorizontal     = (PUINT8)(pFrameState->pOutputBuffer->HorizontalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer);
    pMaskUvVertical      = (PUINT8)(pFrameState->pOutputBuffer->VerticalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer);
    pMaskUvHorizontal    = (PUINT8)(pFrameState->pOutputBuffer->HorizontalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer);

    pLoopFilterMaskSB = pMbInfo->pLoopFilterMaskSB;

    // The largest loopfilter we have is 16x16 so we use the 16x16 mask
    // for 32x32 transforms also.
    pLoopFilterMaskSB->LeftY[TX_16X16]      |= pLoopFilterMaskSB->LeftY[TX_32X32];
    pLoopFilterMaskSB->AboveY[TX_16X16]     |= pLoopFilterMaskSB->AboveY[TX_32X32];
    pLoopFilterMaskSB->LeftUv[TX_16X16]     |= pLoopFilterMaskSB->LeftUv[TX_32X32];
    pLoopFilterMaskSB->AboveUv[TX_16X16]    |= pLoopFilterMaskSB->AboveUv[TX_32X32];

    // We do at least 8 tap filter on every 32x32 even if the transform size
    // is 4x4.  So if the 4x4 is set on a border pixel add it to the 8x8 and
    // remove it from the 4x4.
    pLoopFilterMaskSB->LeftY[TX_8X8] |= pLoopFilterMaskSB->LeftY[TX_4X4] & VP9_LEFT_BORDER_LUMA_MASK;
    pLoopFilterMaskSB->LeftY[TX_4X4] &= ~VP9_LEFT_BORDER_LUMA_MASK;
    pLoopFilterMaskSB->AboveY[TX_8X8] |= pLoopFilterMaskSB->AboveY[TX_4X4] & VP9_ABOVE_BORDER_LUMA_MASK;
    pLoopFilterMaskSB->AboveY[TX_4X4] &= ~VP9_ABOVE_BORDER_LUMA_MASK;
    pLoopFilterMaskSB->LeftUv[TX_8X8] |= pLoopFilterMaskSB->LeftUv[TX_4X4] & VP9_LEFT_BORDER_CHROMA_MASK;
    pLoopFilterMaskSB->LeftUv[TX_4X4] &= ~VP9_LEFT_BORDER_CHROMA_MASK;
    pLoopFilterMaskSB->AboveUv[TX_8X8] |= pLoopFilterMaskSB->AboveUv[TX_4X4] & VP9_ABOVE_BORDER_CHROMA_MASK;
    pLoopFilterMaskSB->AboveUv[TX_4X4] &= ~VP9_ABOVE_BORDER_CHROMA_MASK;

    // Handle the case that the current SB bottom edge exceeds tile boundary
    if (dwB8Row + VP9_B64_SIZE_IN_B8 > dwTileBottomB8)
    {
        const UINT64 ActualRows = dwTileBottomB8 - dwB8Row;

        // Each pixel inside the border gets a 1,
        const UINT64 MaskY = (((UINT64) 1 << (ActualRows << 3)) - 1);
        const UINT16 MaskUv = (((UINT16) 1 << (((ActualRows + 1) >> 1) << 2)) - 1);

        // Remove values completely outside our border.
        for (i = 0; i < TX_32X32; i++)
        {
            pLoopFilterMaskSB->LeftY[i] &= MaskY;
            pLoopFilterMaskSB->AboveY[i] &= MaskY;
            pLoopFilterMaskSB->LeftUv[i] &= MaskUv;
            pLoopFilterMaskSB->AboveUv[i] &= MaskUv;
        }
        pLoopFilterMaskSB->Int4x4Y &= MaskY;
        pLoopFilterMaskSB->Int4x4Uv &= MaskUv;

        // We don't apply a wide loop filter on the last uv block row.  If set
        // apply the shorter one instead.
        if (ActualRows == 1)
        {
            pLoopFilterMaskSB->AboveUv[TX_8X8] |= pLoopFilterMaskSB->AboveUv[TX_16X16];
            pLoopFilterMaskSB->AboveUv[TX_16X16] = 0;
        }
        if (ActualRows == 5)
        {
            pLoopFilterMaskSB->AboveUv[TX_8X8] |= pLoopFilterMaskSB->AboveUv[TX_16X16] & 0xff00;
            pLoopFilterMaskSB->AboveUv[TX_16X16] &= ~(pLoopFilterMaskSB->AboveUv[TX_16X16] & 0xff00);
        }
    }

    // Handle the case that the current SB right edge exceeds tile boundary
    if (dwB8Col + VP9_B64_SIZE_IN_B8 > dwTileRightB8)
    {
        const UINT64 ActualColumns = dwTileRightB8 - dwB8Col;

        // Each pixel inside the border gets a 1, the multiply copies the border
        // to where we need it.
        const UINT64 MaskY  = (((1 << ActualColumns) - 1)) * 0x0101010101010101;
        const UINT16 MaskUv = ((1 << ((ActualColumns + 1) >> 1)) - 1) * 0x1111;

        // Internal edges are not applied on the last column of the image so
        // we mask 1 more for the internal edges
        const UINT16 mask_uv_int = ((1 << (ActualColumns >> 1)) - 1) * 0x1111;

        // Remove the bits outside the image edge.
        for (i = 0; i < TX_32X32; i++)
        {
            pLoopFilterMaskSB->LeftY[i] &= MaskY;
            pLoopFilterMaskSB->AboveY[i] &= MaskY;
            pLoopFilterMaskSB->LeftUv[i] &= MaskUv;
            pLoopFilterMaskSB->AboveUv[i] &= MaskUv;
        }
        pLoopFilterMaskSB->Int4x4Y &= MaskY;
        pLoopFilterMaskSB->Int4x4Uv &= mask_uv_int;

        // We don't apply a wide loop filter on the last uv column.  If set
        // apply the shorter one instead.
        if (ActualColumns == 1) {
            pLoopFilterMaskSB->LeftUv[TX_8X8] |= pLoopFilterMaskSB->LeftUv[TX_16X16];
            pLoopFilterMaskSB->LeftUv[TX_16X16] = 0;
        }
        if (ActualColumns == 5) {
            pLoopFilterMaskSB->LeftUv[TX_8X8] |= (pLoopFilterMaskSB->LeftUv[TX_16X16] & 0xcccc);
            pLoopFilterMaskSB->LeftUv[TX_16X16] &= ~(pLoopFilterMaskSB->LeftUv[TX_16X16] & 0xcccc);
        }
    }

    // We don't a loop filter on the first column in the image.  Mask that out.
    if (dwB8Col == 0)
    {
        for (i = 0; i < TX_32X32; i++) {
            pLoopFilterMaskSB->LeftY[i]     &= 0xfefefefefefefefe;
            pLoopFilterMaskSB->LeftUv[i]    &= 0xeeee;
        }
    }

    // 00 -> No vertical DBLK on 8x8 edge; 
    // 01 -> 4x4 vertical DBLK on 8x8 edge; 
    // 10 -> 8x8 vertical DBLK on 8x8 edge; 
    LeftY4x4	    = pLoopFilterMaskSB->LeftY[TX_4X4];
    LeftY8x8	    = pLoopFilterMaskSB->LeftY[TX_8X8];
    LeftY16x16	    = pLoopFilterMaskSB->LeftY[TX_16X16];
    Int4x4Y		    = pLoopFilterMaskSB->Int4x4Y;
    
    AboveY4x4	    = pLoopFilterMaskSB->AboveY[TX_4X4];
    AboveY8x8	    = pLoopFilterMaskSB->AboveY[TX_8X8];
    AboveY16x16	    = pLoopFilterMaskSB->AboveY[TX_16X16];
    
    LeftUv4x4	    = pLoopFilterMaskSB->LeftUv[TX_4X4];
    LeftUv8x8	    = pLoopFilterMaskSB->LeftUv[TX_8X8];
    LeftUv16x16	    = pLoopFilterMaskSB->LeftUv[TX_16X16];
    Int4x4Uv	    = pLoopFilterMaskSB->Int4x4Uv;
    
    AboveUv4x4	    = pLoopFilterMaskSB->AboveUv[TX_4X4];
    AboveUv8x8	    = pLoopFilterMaskSB->AboveUv[TX_8X8];
    AboveUv16x16    = pLoopFilterMaskSB->AboveUv[TX_16X16];
    
    //Prepare V&H Mask for Luma Plane
    ValidRowsWithinSB = (dwTileBottomB8 - dwB8Row > VP9_B64_SIZE_IN_B8)? VP9_B64_SIZE_IN_B8 : (dwTileBottomB8 - dwB8Row);
    ValidColumnsWithinSB = (dwTileRightB8 - dwB8Col > VP9_B64_SIZE_IN_B8)? VP9_B64_SIZE_IN_B8 : (dwTileRightB8 - dwB8Col);
    nMaskYVertStride = pFrameState->pOutputBuffer->VerticalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].dwPitch;
    nMaskYHorStride = pFrameState->pOutputBuffer->HorizontalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].dwPitch;
    nMaskUVVertStride = pFrameState->pOutputBuffer->VerticalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].dwPitch;
    nMaskUVHorStride = pFrameState->pOutputBuffer->HorizontalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].dwPitch;
    for(MaskPositionY = 0; MaskPositionY < ValidRowsWithinSB; MaskPositionY++)
    {
        for(MaskPositionX = 0; MaskPositionX < ValidColumnsWithinSB; MaskPositionX+=2)
        {
            MaskPosition = (MaskPositionY << 3) + MaskPositionX;
            
            if(((dwB8Col + MaskPositionX + 1) << 3) > pFrameInfo->dwPicWidth)
            {
                uiMaskVertical = ((( LeftY4x4 & (((UINT64)1) << MaskPosition) ) >> MaskPosition) + (( LeftY8x8 & (((UINT64)1) << MaskPosition) ) >> MaskPosition) * 2
                    + (( LeftY16x16 & (((UINT64)1) << MaskPosition) ) >> MaskPosition) * 3) * 16;	//Left 4 bit
                uiMaskVertical += (( LeftY4x4 & (((UINT64)1) << (MaskPosition+1) ) ) >> (MaskPosition+1)) + (( LeftY8x8 & (((UINT64)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 2
                    + (( LeftY16x16 & (((UINT64)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 3;	//Right 4 bit
            }
            else
            {
                uiMaskVertical = ((( LeftY4x4 & (((UINT64)1) << MaskPosition) ) >> MaskPosition) + (( LeftY8x8 & (((UINT64)1) << MaskPosition) ) >> MaskPosition) * 2
                    + (( LeftY16x16 & (((UINT64)1) << MaskPosition) ) >> MaskPosition) * 3 + (( Int4x4Y & (((UINT64)1) << MaskPosition) ) >> MaskPosition) * 4) * 16;	//Left 4 bit
                uiMaskVertical += (( LeftY4x4 & (((UINT64)1) << (MaskPosition+1) ) ) >> (MaskPosition+1)) + (( LeftY8x8 & (((UINT64)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 2
                    + (( LeftY16x16 & (((UINT64)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 3 + (( Int4x4Y & (((UINT64)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 4;	//Right 4 bit
            }
            
            if(MaskPositionY + dwB8Row == 0)
            {
                //Picture Top Boundary
                uiMaskHorizontal = ((( Int4x4Y & (((UINT64)1) << MaskPosition) ) >> MaskPosition) * 4) * 16;	//Left 4 bit
                uiMaskHorizontal += (( Int4x4Y & (((UINT64)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 4;	//Right 4 bit
            }
            else if(((dwB8Row + MaskPositionY + 1) << 3) > pFrameInfo->dwPicHeight)
            {
                //No 4x4 Internal horizontal
                uiMaskHorizontal = ((( AboveY4x4 & (((UINT64)1) << MaskPosition) ) >> MaskPosition) + (( AboveY8x8 & (((UINT64)1) << MaskPosition) ) >> MaskPosition) * 2
                    + (( AboveY16x16 & (((UINT64)1) << MaskPosition) ) >> MaskPosition) * 3 ) * 16;	//Left 4 bit
                uiMaskHorizontal += (( AboveY4x4 & (((UINT64)1) << (MaskPosition+1) ) ) >> (MaskPosition+1)) + (( AboveY8x8 & (((UINT64)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 2
                    + (( AboveY16x16 & (((UINT64)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 3;	//Right 4 bit						
            }
            else
            {
                uiMaskHorizontal = ((( AboveY4x4 & (((UINT64)1) << MaskPosition) ) >> MaskPosition) + (( AboveY8x8 & (((UINT64)1) << MaskPosition) ) >> MaskPosition) * 2
                    + (( AboveY16x16 & (((UINT64)1) << MaskPosition) ) >> MaskPosition) * 3 + (( Int4x4Y & (((UINT64)1) << MaskPosition) ) >> MaskPosition) * 4) * 16;	//Left 4 bit
                uiMaskHorizontal += (( AboveY4x4 & (((UINT64)1) << (MaskPosition+1) ) ) >> (MaskPosition+1)) + (( AboveY8x8 & (((UINT64)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 2
                    + (( AboveY16x16 & (((UINT64)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 3 + (( Int4x4Y & (((UINT64)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 4;	//Right 4 bit
            }
            
            pMaskYVertical[(MaskPositionY + dwB8Row)* nMaskYVertStride + ((dwB8Col + MaskPositionX)>>1)] = uiMaskVertical;
            pMaskYHorizontal[(MaskPositionY + dwB8Row) * nMaskYHorStride + ((dwB8Col + MaskPositionX)>>1)] = uiMaskHorizontal;
        }
    }

    //Prepare V&H Mask for Chroma Plane
    ValidRowsWithinSB = (dwTileBottomB8 - dwB8Row + 1 > VP9_B64_SIZE_IN_B8)? (VP9_B64_SIZE_IN_B8 >> 1) : ((dwTileBottomB8 - dwB8Row + 1) >> 1);
    ValidColumnsWithinSB = (dwTileRightB8 - dwB8Col + 1 > VP9_B64_SIZE_IN_B8)? (VP9_B64_SIZE_IN_B8 >> 1) : ((dwTileRightB8 - dwB8Col + 1) >> 1);
    for(MaskPositionY = 0; MaskPositionY < ValidRowsWithinSB; MaskPositionY++)
    {
        for(MaskPositionX = 0; MaskPositionX < ValidColumnsWithinSB; MaskPositionX+=2)
        {
            MaskPosition = (MaskPositionY << 2) + MaskPositionX;

            if((dwB8Col << 3) + ((MaskPositionX + 1) << 4) > pFrameInfo->dwPicWidth)
            {
                uiMaskVertical = ((( LeftUv4x4 & (((UINT16)1) << MaskPosition) ) >> MaskPosition) + (( LeftUv8x8 & (((UINT16)1) << MaskPosition) ) >> MaskPosition) * 2
                    + (( LeftUv16x16 & (((UINT16)1) << MaskPosition) ) >> MaskPosition) * 3) * 16;	//Left 4 bit
                uiMaskVertical += (( LeftUv4x4 & (((UINT16)1) << (MaskPosition+1) ) ) >> (MaskPosition+1)) + (( LeftUv8x8 & (((UINT16)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 2
                    + (( LeftUv16x16 & (((UINT16)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 3;	//Right 4 bit
            }
            else
            {
                uiMaskVertical = ((( LeftUv4x4 & (((UINT16)1) << MaskPosition) ) >> MaskPosition) + (( LeftUv8x8 & (((UINT16)1) << MaskPosition) ) >> MaskPosition) * 2
                    + (( LeftUv16x16 & (((UINT16)1) << MaskPosition) ) >> MaskPosition) * 3 + (( Int4x4Uv & (((UINT16)1) << MaskPosition) ) >> MaskPosition) * 4) * 16;	//Left 4 bit
                uiMaskVertical += (( LeftUv4x4 & (((UINT16)1) << (MaskPosition+1) ) ) >> (MaskPosition+1)) + (( LeftUv8x8 & (((UINT16)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 2
                    + (( LeftUv16x16 & (((UINT16)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 3 + (( Int4x4Uv & (((UINT16)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 4;	//Right 4 bit
            }

            if(MaskPositionY + dwB8Row == 0)
            {
                //Picture Top Boundary
                uiMaskHorizontal = 0;
                if (pFrameInfo->dwPicHeight > 8)
                {
                    uiMaskHorizontal = ((( Int4x4Uv & (((UINT16)1) << MaskPosition) ) >> MaskPosition) * 4) * 16;	//Left 4 bit
                    uiMaskHorizontal += (( Int4x4Uv & (((UINT16)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 4;	//Right 4 bit
                }
            }
            else if((dwB8Row << 3) + ((MaskPositionY + 1) << 4) > pFrameInfo->dwPicHeight)
            {
                //No 4x4 Internal horizontal
                uiMaskHorizontal = ((( AboveUv4x4 & (((UINT16)1) << MaskPosition) ) >> MaskPosition) + (( AboveUv8x8 & (((UINT16)1) << MaskPosition) ) >> MaskPosition) * 2
                    + (( AboveUv16x16 & (((UINT16)1) << MaskPosition) ) >> MaskPosition) * 3 ) * 16;	//Left 4 bit
                uiMaskHorizontal += (( AboveUv4x4 & (((UINT16)1) << (MaskPosition+1) ) ) >> (MaskPosition+1)) + (( AboveUv8x8 & (((UINT16)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 2
                    + (( AboveUv16x16 & (((UINT16)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 3 ;	//Right 4 bit						
            }
            else
            {
                uiMaskHorizontal = ((( AboveUv4x4 & (((UINT16)1) << MaskPosition) ) >> MaskPosition) + (( AboveUv8x8 & (((UINT16)1) << MaskPosition) ) >> MaskPosition) * 2
                    + (( AboveUv16x16 & (((UINT16)1) << MaskPosition) ) >> MaskPosition) * 3 + (( Int4x4Uv & (((UINT16)1) << MaskPosition) ) >> MaskPosition) * 4) * 16;	//Left 4 bit
                uiMaskHorizontal += (( AboveUv4x4 & (((UINT16)1) << (MaskPosition+1) ) ) >> (MaskPosition+1)) + (( AboveUv8x8 & (((UINT16)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 2
                    + (( AboveUv16x16 & (((UINT16)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 3 + (( Int4x4Uv & (((UINT16)1) << (MaskPosition+1)) ) >> (MaskPosition+1)) * 4;	//Right 4 bit
            }

            pMaskUvVertical[(MaskPositionY + (dwB8Row >> 1) )* nMaskUVVertStride + (((dwB8Col>>1) + MaskPositionX)>>1)] = uiMaskVertical;
            pMaskUvHorizontal[(MaskPositionY + (dwB8Row >> 1) ) * nMaskUVHorStride + (((dwB8Col>>1) + MaskPositionX)>>1)] = uiMaskHorizontal;
        }
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_ParseLoopFilterThreshold(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState)
{
    VAStatus  eStatus;
    INT         Level, BlockInsideLimit;
    PINTEL_HOSTVLD_VP9_FRAME_INFO pFrameInfo;
    PINTEL_VP9_PIC_PARAMS         pPicParams;
    UCHAR                           SharpnessLevel;
    PUINT8                           pLoopFilterThresholdAddr;
    DWORD                            nFilterThresholdStride;

    eStatus   = VA_STATUS_SUCCESS;

    pFrameInfo = &pFrameState->FrameInfo;
    pPicParams = pFrameInfo->pPicParams;
    SharpnessLevel = pPicParams->sharpness_level;
    pLoopFilterThresholdAddr = (PUINT8)(pFrameState->pOutputBuffer->Threshold.pu8Buffer);
    nFilterThresholdStride = pFrameState->pOutputBuffer->Threshold.dwPitch;

    for (Level = 0; Level <= VP9_MAX_LOOP_FILTER; Level++)
    {
        // Calc MbLim/Lim per sharpness level
        BlockInsideLimit = Level >> ((SharpnessLevel > 0) + (SharpnessLevel > 4));

        if (SharpnessLevel > 0)
        {
          if (BlockInsideLimit > (9 - SharpnessLevel))
          {
              BlockInsideLimit = (9 - SharpnessLevel);
          }
        }

        if (BlockInsideLimit < 1)
        {
            BlockInsideLimit = 1;
        }

        pLoopFilterThresholdAddr[Level * nFilterThresholdStride]        = ((Level + 2) << 1) + BlockInsideLimit;    //Byte0: MbLim
        pLoopFilterThresholdAddr[Level * nFilterThresholdStride + 1]    = (UINT8)BlockInsideLimit;                  //Byte1: Lim  
        pLoopFilterThresholdAddr[Level * nFilterThresholdStride + 2]    = Level >> 4;                               //Byte2: Hev_threshold
        pLoopFilterThresholdAddr[Level * nFilterThresholdStride + 3]    = 0;                                        //Byte3: 0
    }

finish:
    return eStatus;
}
#endif
    VAStatus Intel_HostvldVp9_ParseBlock(
    PINTEL_HOSTVLD_VP9_TILE_STATE   pTileState)
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_TILE_INFO     pTileInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo;
    VAStatus                          eStatus = VA_STATUS_SUCCESS;


    pFrameState = pTileState->pFrameState;
    pFrameInfo  = &pFrameState->FrameInfo;
    pMbInfo     = &pTileState->MbInfo;
    pTileInfo   = pMbInfo->pCurrTile;

    pMbInfo->iMbPosInB64X = pMbInfo->dwMbPosX & (VP9_B64_SIZE_IN_B8 - 1);
    pMbInfo->iMbPosInB64Y = pMbInfo->dwMbPosY & (VP9_B64_SIZE_IN_B8 - 1);

    // set neighbor availabilities
    pMbInfo->bAboveValid = pMbInfo->dwMbPosY > 0;
    pMbInfo->bLeftValid  = (DWORD)pMbInfo->dwMbPosX > pTileInfo->dwTileLeft;
    pMbInfo->bRightValid = (DWORD)pMbInfo->dwMbPosX < (pTileInfo->dwTileLeft + pTileInfo->dwTileWidth);

    // entropy context pointers
    pMbInfo->pAboveContext[VP9_CODED_YUV_PLANE_Y] = pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_Y] + (pMbInfo->dwMbPosX << 1);
    pMbInfo->pAboveContext[VP9_CODED_YUV_PLANE_U] = pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_U] + pMbInfo->dwMbPosX;
    pMbInfo->pAboveContext[VP9_CODED_YUV_PLANE_V] = pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_V] + pMbInfo->dwMbPosX;
    pMbInfo->pLeftContext[VP9_CODED_YUV_PLANE_Y]  = pTileInfo->pEntropyContextLeft[VP9_CODED_YUV_PLANE_Y] + (pMbInfo->iMbPosInB64Y << 1);
    pMbInfo->pLeftContext[VP9_CODED_YUV_PLANE_U]  = pTileInfo->pEntropyContextLeft[VP9_CODED_YUV_PLANE_U] + pMbInfo->iMbPosInB64Y;
    pMbInfo->pLeftContext[VP9_CODED_YUV_PLANE_V]  = pTileInfo->pEntropyContextLeft[VP9_CODED_YUV_PLANE_V] + pMbInfo->iMbPosInB64Y;

    // parse single block
    pFrameInfo->pfnParseFrmModeInfo(pTileState);
    Intel_HostvldVp9_ParseCoefficient(pTileState, pMbInfo->i8ZOrder);
#ifndef SEPERATE_LOOPFILTER_ENABLE
    Intel_HostvldVp9_ParseLoopFilterLevelAndMaskInSingleBlock(pFrameState);
#endif

finish:
    return eStatus;

}

static INTEL_HOSTVLD_VP9_PARTITION_TYPE Intel_HostvldVp9_ParsePartitionType(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    PINTEL_HOSTVLD_VP9_TILE_INFO     pTileInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine,
    DWORD                               dwB8X,
    DWORD                               dwB8Y,
    DWORD                               dwSplitBlockSize,
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlockSize)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext;
    INT                                 iContext;
    PUINT8                              pbAboveContext, pbLeftContext;
    DWORD                               dwPosition;
    INT                                 iLog2BlockSizeInB8, iBlockSizeInB8, iOffset, i;
    INT                                 iLeft, iAbove;
    INT                                 iPartitionType = PARTITION_SPLIT;

    pFrameInfo = &pTileState->pFrameState->FrameInfo;
    pContext   = pFrameInfo->pContext;

    // get partition context
    pbAboveContext      = pFrameInfo->pSegContextAbove + dwB8X;
    pbLeftContext       = pTileInfo->SegContextLeft + (dwB8Y & (VP9_B64_SIZE_IN_B8 - 1));
    iLog2BlockSizeInB8  = BlockSize - BLOCK_8X8;
    iBlockSizeInB8      = 1 << iLog2BlockSizeInB8;
    iOffset             = VP9_LOG2_B64_SIZE_IN_B8 - iLog2BlockSizeInB8;
    iLeft = iAbove      = 0;

    for (i = 0; i < iBlockSizeInB8; i++)
    {
        iAbove |= (pbAboveContext[i] & (1 << iOffset));
        iLeft  |= (pbLeftContext[i]  & (1 << iOffset));
    }

    iAbove   = (iAbove > 0);
    iLeft    = (iLeft > 0);
    iContext = ((iLeft << 1) + iAbove) + iLog2BlockSizeInB8 * VP9_PARTITION_PLOFFSET;

    // get partition type
    {
        const BYTE *pProbs = pFrameInfo->bIsKeyFrame ?
            g_Vp9KeyFramePartitionProbs[iContext].Prob : pContext->PartitionProbs[iContext].Prob;

        dwPosition =
            (((dwB8Y + dwSplitBlockSize) < pFrameInfo->dwB8Rows) << 1) |
            ((dwB8X + dwSplitBlockSize) < pFrameInfo->dwB8Columns);
        if (dwPosition == 3) // inside picture
        {
            iPartitionType = INTEL_HOSTVLD_VP9_READ_BIT(pProbs[0]);
            if (iPartitionType == 1)
            {
                iPartitionType += INTEL_HOSTVLD_VP9_READ_BIT(pProbs[1]);
                if (iPartitionType == 2)
                {
                    iPartitionType += INTEL_HOSTVLD_VP9_READ_BIT(pProbs[2]);
                }
            }
        }
        else if (dwPosition != 0)
        {
            if (!INTEL_HOSTVLD_VP9_READ_BIT(pProbs[dwPosition]))
            {
                iPartitionType = (INT)dwPosition;
            }
        }
    }

    pTileState->Count.PartitionCounts[iContext][iPartitionType] += pFrameInfo->bFrameParallelDisabled;

    return (INTEL_HOSTVLD_VP9_PARTITION_TYPE)iPartitionType;
}

VAStatus Intel_HostvldVp9_ParseSuperBlock(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState, 
    DWORD                               dwB8X, 
    DWORD                               dwB8Y, 
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlockSize)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo;
    INTEL_HOSTVLD_VP9_PARTITION_TYPE PartitionType;
    DWORD                               dwSplitBlockSize;
    BOOL                                bUpdateContext;
    VAStatus                          eStatus = VA_STATUS_SUCCESS;


    pFrameInfo      = &pTileState->pFrameState->FrameInfo;
    pMbInfo         = &pTileState->MbInfo;
    bUpdateContext  = TRUE;

    // if the block is out of picture boundary, skip it since it is not coded.
    if ((dwB8X >= pFrameInfo->dwB8Columns) || (dwB8Y >= pFrameInfo->dwB8Rows))
    {
        goto finish;
    }

    dwSplitBlockSize = (1 << BlockSize) >> 2;
    PartitionType = Intel_HostvldVp9_ParsePartitionType(
        pTileState,
        pMbInfo->pCurrTile, 
        &pTileState->BacEngine,
        dwB8X, 
        dwB8Y, 
        dwSplitBlockSize, 
        BlockSize);

    pMbInfo->dwMbPosX    = dwB8X;
    pMbInfo->dwMbPosY    = dwB8Y;

    if (BlockSize == BLOCK_8X8)
    {
        pMbInfo->BlockSize  = g_Vp9B8SubBlock[PartitionType];
        pMbInfo->iB4Number  = g_Vp9B4NumberLookup[pMbInfo->BlockSize];
        Intel_HostvldVp9_ParseBlock(pTileState);
    }
    else if (PartitionType == PARTITION_NONE)
    {
        pMbInfo->BlockSize  = BlockSize;
        pMbInfo->iB4Number  = g_Vp9B4NumberLookup[pMbInfo->BlockSize];
        Intel_HostvldVp9_ParseBlock(pTileState);
    }
    else if (PartitionType == PARTITION_HORZ)
    {
        pMbInfo->BlockSize  = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize + 4);
        pMbInfo->iB4Number  = g_Vp9B4NumberLookup[pMbInfo->BlockSize];
        Intel_HostvldVp9_ParseBlock(pTileState);
        pMbInfo->dwMbPosY    += dwSplitBlockSize;
        if (pMbInfo->dwMbPosY < (INT)pFrameInfo->dwB8Rows)
        {
            Intel_HostvldVp9_ParseBlock(pTileState);
        }
    }
    else if (PartitionType == PARTITION_VERT)
    {
        pMbInfo->BlockSize  = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize + 8);
        pMbInfo->iB4Number  = g_Vp9B4NumberLookup[pMbInfo->BlockSize];
        Intel_HostvldVp9_ParseBlock(pTileState);
        pMbInfo->dwMbPosX    += dwSplitBlockSize;
        if (pMbInfo->dwMbPosX < (INT)pFrameInfo->dwB8Columns)
        {
            Intel_HostvldVp9_ParseBlock(pTileState);
        }
    }
    else if (PartitionType == PARTITION_SPLIT)
    {
        BlockSize = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize - 1);
        bUpdateContext     = FALSE;
        Intel_HostvldVp9_ParseSuperBlock(
            pTileState, 
            dwB8X, 
            dwB8Y, 
            BlockSize);
        Intel_HostvldVp9_ParseSuperBlock(
            pTileState, 
            dwB8X + dwSplitBlockSize, 
            dwB8Y, 
            BlockSize);
        Intel_HostvldVp9_ParseSuperBlock(
            pTileState, 
            dwB8X, 
            dwB8Y + dwSplitBlockSize, 
            BlockSize);
        Intel_HostvldVp9_ParseSuperBlock(
            pTileState, 
            dwB8X + dwSplitBlockSize, 
            dwB8Y + dwSplitBlockSize, 
            BlockSize);
    }
    else
    {
        assert(0);
    }

    // Update partition context
    if (bUpdateContext)
    {
        Intel_HostvldVp9_UpdatePartitionContext(
            pFrameInfo, pMbInfo->pCurrTile, dwB8X, dwB8Y, pMbInfo->BlockSize, BlockSize);
    }

finish:
    return eStatus;

}

VAStatus Intel_HostvldVp9_ParseOneTile(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState, 
    PINTEL_HOSTVLD_VP9_TILE_INFO     pTileInfo)
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE     pFrameState;
    PINTEL_HOSTVLD_VP9_FRAME_INFO      pFrameInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO         pMbInfo;
#ifndef SEPERATE_LOOPFILTER_ENABLE
    INTEL_HOSTVLD_VP9_LOOP_FILTER_MASK LoopFilterMaskSB;
#endif
    DWORD                                 dwB8X, dwB8Y, dwTileBottomB8, dwTileRightB8, dwLineDist;
    VAStatus                            eStatus = VA_STATUS_SUCCESS;


    pFrameState                = pTileState->pFrameState;
    pFrameInfo                 = &pFrameState->FrameInfo;
    pMbInfo                    = &pTileState->MbInfo;
    pMbInfo->pCurrTile         = pTileInfo;
#ifndef SEPERATE_LOOPFILTER_ENABLE
    pMbInfo->pLoopFilterMaskSB = &LoopFilterMaskSB;
#endif

    if (pTileInfo->dwTileTop == 0)
    {
        pMbInfo->dwMbOffset = pTileInfo->dwTileLeft << VP9_LOG2_B64_SIZE_IN_B8;
    }

    dwTileRightB8  = pTileInfo->dwTileLeft + pTileInfo->dwTileWidth;
    dwTileBottomB8 = pTileInfo->dwTileTop  + pTileInfo->dwTileHeight;
    dwLineDist     = (pFrameInfo->dwMbStride -
        ALIGN(pTileInfo->dwTileWidth, VP9_B64_SIZE_IN_B8)) << VP9_LOG2_B64_SIZE_IN_B8;

    for (dwB8Y = pTileInfo->dwTileTop; dwB8Y < dwTileBottomB8; dwB8Y += VP9_B64_SIZE_IN_B8)
    {
        // Reset left contexts
        CMOS_ZeroMemory(pTileInfo->SegContextLeft, sizeof(pTileInfo->SegContextLeft));
        CMOS_ZeroMemory(pTileInfo->PredSegIdContextLeft, sizeof(pTileInfo->PredSegIdContextLeft));
        CMOS_ZeroMemory(pTileInfo->pEntropyContextLeft, sizeof(pTileInfo->pEntropyContextLeft));

        // Deocde one row
        for (dwB8X = pTileInfo->dwTileLeft; dwB8X < dwTileRightB8; dwB8X += VP9_B64_SIZE_IN_B8)
        {
#ifndef SEPERATE_LOOPFILTER_ENABLE
            CMOS_ZeroMemory(&LoopFilterMaskSB, sizeof(INTEL_HOSTVLD_VP9_LOOP_FILTER_MASK));
#endif
            Intel_HostvldVp9_ParseSuperBlock(
                pTileState, 
                dwB8X, 
                dwB8Y, 
                BLOCK_64X64);
#ifndef SEPERATE_LOOPFILTER_ENABLE
            Intel_HostvldVp9_ParseLoopFilterMaskInSuperBlock(
                pFrameState,
                dwB8Y,
                dwB8X,
                dwTileBottomB8,
                dwTileRightB8);
#endif

            pMbInfo->dwMbOffset += VP9_B64_SIZE;
        }

        pMbInfo->dwMbOffset += dwLineDist;
    }

finish:
    return eStatus;
}

static VOID Intel_HostvldVp9_SetTileIndex(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_TILE_INFO     pTileInfo;
    PUINT8                              pTileIndex;
    DWORD                               dwTileIndex, dwB32Column;

    pFrameInfo  = &pFrameState->FrameInfo;
    pTileIndex  = pFrameState->pOutputBuffer->TileIndex.pu8Buffer;
    pTileInfo   = pFrameInfo->TileInfo;

    CMOS_FillMemory(pTileIndex, pFrameState->pOutputBuffer->TileIndex.dwSize, 0xFF);

    pTileIndex++;

    for (dwTileIndex = 0; dwTileIndex < pFrameInfo->dwTileColumns; dwTileIndex++)
    {
        for (dwB32Column = 0;
            dwB32Column < ((pTileInfo->dwTileWidth + 3) >> 2);
            dwB32Column++)
        {
            *pTileIndex++ = (UINT8)dwTileIndex;
        }
        pTileInfo++;
    }
}

static VAStatus Intel_HostvldVp9_SetOutOfBoundValues(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState) 
{
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER pOutputBuffer; 
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_1D_BUFFER     pLumaPredBuffer, pSegIdBuf;
    PUINT32                             pu32LumaPredBuffer;
    PUINT8                              pu8SegIdBuffer;
    UINT                                i, x, y;
    DWORD                               dwOutBoundB8Columns, dwOutBoundB8Rows;
    VAStatus                          eStatus = VA_STATUS_SUCCESS;

    
    pOutputBuffer = pFrameState->pOutputBuffer;
    pFrameInfo    = &pFrameState->FrameInfo;

    // Luma prediction mode: out of boundary values must be <= 9 (TM mode). We set them to 0 here
    pLumaPredBuffer    = &pOutputBuffer->PredictionMode[INTEL_HOSTVLD_VP9_YUV_PLANE_Y];
    pu32LumaPredBuffer = pLumaPredBuffer->pu32Buffer + ((pFrameInfo->dwB8ColumnsAligned - 8) << 3);
    
    //Last Segment Id buffer: set out of boundary values to VP9_MAX_SEGMENTS.
    pSegIdBuf = pFrameState->pLastSegIdBuf;
    pu8SegIdBuffer = pSegIdBuf->pu8Buffer + ((pFrameInfo->dwB8ColumnsAligned - 8) << 3);

    dwOutBoundB8Columns = pFrameInfo->dwB8ColumnsAligned - pFrameInfo->dwB8Columns;
    dwOutBoundB8Rows    = pFrameInfo->dwB8RowsAligned - pFrameInfo->dwB8Rows;

    //SB Columns 
    if (dwOutBoundB8Columns)
    {
        for (i = 0; i < pFrameInfo->dwB8RowsAligned >> 3; i++)
        {
            for (y = 0; y < 8; y++)
            {
                for (x = 8 - dwOutBoundB8Columns; x < 8; x++)
                {
                    *(pu32LumaPredBuffer + g_Vp9TxBlockIndex2ZOrderIndexMapSquare64[(y << 3) + x]) = 0;
                    *(pu8SegIdBuffer + g_Vp9TxBlockIndex2ZOrderIndexMapSquare64[(y << 3) + x]) = VP9_MAX_SEGMENTS;
                }
            }
            pu32LumaPredBuffer += (pFrameInfo->dwB8ColumnsAligned << 3);
            pu8SegIdBuffer += (pFrameInfo->dwB8ColumnsAligned << 3);
        }
    }

    // SB Rows
    pu32LumaPredBuffer = pLumaPredBuffer->pu32Buffer + pFrameInfo->dwB8ColumnsAligned * (pFrameInfo->dwB8RowsAligned - 8);
    pu8SegIdBuffer = pSegIdBuf->pu8Buffer + pFrameInfo->dwB8ColumnsAligned * (pFrameInfo->dwB8RowsAligned - 8);
    if (dwOutBoundB8Rows)
    {
        for (i = 0; i < pFrameInfo->dwB8ColumnsAligned >> 3; i++)
        {
            for (y = 8 - dwOutBoundB8Rows; y < 8; y++)
            {
                for (x = 0; x < 8; x++)
                {
                    *(pu32LumaPredBuffer + g_Vp9TxBlockIndex2ZOrderIndexMapSquare64[(y << 3) + x]) = 0;
                    *(pu8SegIdBuffer + g_Vp9TxBlockIndex2ZOrderIndexMapSquare64[(y << 3) + x]) = VP9_MAX_SEGMENTS;
                }
            }
            pu32LumaPredBuffer += 64;
            pu8SegIdBuffer += 64;
        }
    }

finish:
    return eStatus;
}

VOID Intel_HostvldVp9_MergeCounts(
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo,
    PINTEL_HOSTVLD_VP9_COUNT         pBaseCount,
    PINTEL_HOSTVLD_VP9_COUNT         pCount)
{
    INT   i;
    PUINT puiBaseCount, puiCount;

    VP9_MERGE_COUNT_ARRAY(CoeffCounts);
    VP9_MERGE_COUNT_ARRAY(EobBranchCounts);

    if (!pFrameInfo->bIsIntraOnly)
    {
        VP9_MERGE_COUNT_ARRAY(IntraModeCounts_Y);
        VP9_MERGE_COUNT_ARRAY(IntraModeCounts_UV);
        VP9_MERGE_COUNT_ARRAY(MbSkipCounts);
        VP9_MERGE_COUNT_ARRAY(PartitionCounts);
        VP9_MERGE_COUNT_ARRAY(InterModeCounts);
        VP9_MERGE_COUNT_ARRAY(SwitchableInterpCounts);
        VP9_MERGE_COUNT_ARRAY(IntraInterCounts);
        VP9_MERGE_COUNT_ARRAY(CompoundInterCounts);
        VP9_MERGE_COUNT_ARRAY(SingleRefCounts);
        VP9_MERGE_COUNT_ARRAY(CompoundRefCounts);
        VP9_MERGE_COUNT_ARRAY(MvJointCounts);

        puiBaseCount = (PUINT)pBaseCount->MvCountSet;
        puiCount = (PUINT)pCount->MvCountSet;
        for (i = 0; i < (sizeof(pBaseCount->MvCountSet) >> 2); i++)
        {
            puiBaseCount[i] += puiCount[i];
        }

        if (pFrameInfo->TxMode == TX_MODE_SELECT)
        {
            puiBaseCount = (PUINT)&pBaseCount->TxCountSet;
            puiCount = (PUINT)&pCount->TxCountSet;
            for (i = 0; i < (sizeof(pBaseCount->TxCountSet) >> 2); i++)
            {
                puiBaseCount[i] += puiCount[i];
            }
        }
    }
}

VAStatus Intel_HostvldVp9_PreParseTiles(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_TILE_INFO     pTileInfo;
    PBYTE                               pbPartitionData;
    DWORD                               dwPartitionSize;
    DWORD                               dwTileX, dwTileY;
    DWORD                               dwTileBgnX, dwTileBgnY;
    DWORD                               dwTileEndX, dwTileEndY;
    VAStatus                          eStatus     = VA_STATUS_SUCCESS;


    pFrameInfo      = &pFrameState->FrameInfo;
    pbPartitionData = pFrameInfo->SecondPartition.pu8Buffer;
    dwPartitionSize = pFrameInfo->SecondPartition.dwSize;
    pTileInfo       = pFrameInfo->TileInfo;

    if (pFrameInfo->bIsIntraOnly)
    {
        pFrameInfo->pfnParseFrmModeInfo = Intel_HostvldVp9_ParseIntraFrameModeInfo;
    }
    else
    {
        pFrameInfo->pfnParseFrmModeInfo = Intel_HostvldVp9_ParseInterFrameModeInfo;
    }

    // Reset above contexts
    CMOS_ZeroMemory(pFrameInfo->pSegContextAbove, sizeof(*pFrameInfo->pSegContextAbove) * pFrameInfo->dwB8ColumnsAligned);
    CMOS_ZeroMemory(pFrameInfo->pPredSegIdContextAbove, sizeof(*pFrameInfo->pPredSegIdContextAbove) * pFrameInfo->dwB8ColumnsAligned);
    CMOS_ZeroMemory(pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_Y], 
        sizeof(*pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_Y]) * (pFrameInfo->dwB8ColumnsAligned << 1));
    CMOS_ZeroMemory(pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_U], 
        sizeof(*pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_U]) * (pFrameInfo->dwB8ColumnsAligned << 1));
    CMOS_ZeroMemory(pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_V], 
        sizeof(*pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_V]) * (pFrameInfo->dwB8ColumnsAligned << 1));

    // Get tile info and bitstream
    dwTileBgnY = 0;
    for (dwTileY = 0; dwTileY < pFrameInfo->dwTileRows; dwTileY++)
    {
        dwTileEndY = (dwTileY + 1 == pFrameInfo->dwTileRows)? pFrameInfo->dwB8Rows:
            Intel_HostvldVp9_GetTileOffset(
                dwTileY + 1,
                pFrameInfo->dwB8Rows,
                pFrameInfo->dwTileRows,
                pFrameInfo->dwLog2TileRows);

        dwTileBgnX = 0;
        for (dwTileX = 0; dwTileX < pFrameInfo->dwTileColumns; dwTileX++)
        {
            dwTileEndX = Intel_HostvldVp9_GetTileOffset(
                dwTileX + 1,
                pFrameInfo->dwB8Columns,
                pFrameInfo->dwTileColumns,
                pFrameInfo->dwLog2TileColumns);

            pTileInfo->dwTileLeft   = dwTileBgnX;
            pTileInfo->dwTileWidth  = dwTileEndX - dwTileBgnX;
            pTileInfo->dwTileTop    = dwTileBgnY;
            pTileInfo->dwTileHeight = dwTileEndY - dwTileBgnY;

            pTileInfo->BitsBuffer.dwSize = INTEL_HOSTVLD_VP9_READ_DWORD(pbPartitionData);
            pbPartitionData += 4;
            pTileInfo->BitsBuffer.pu8Buffer = pbPartitionData;
            pbPartitionData += pTileInfo->BitsBuffer.dwSize;

            dwTileBgnX = dwTileEndX;
            pTileInfo++;
        }

        dwTileBgnY = dwTileEndY;
        pTileInfo--;
        pTileInfo->dwTileWidth = pFrameInfo->dwB8Columns - pTileInfo->dwTileLeft;
        pTileInfo++;
    }

    // Correct the last tile buffer
    pTileInfo--;
    pTileInfo->BitsBuffer.pu8Buffer -= 4;
    pTileInfo->BitsBuffer.dwSize     = 
        (DWORD)pFrameInfo->SecondPartition.pu8Buffer + dwPartitionSize - (DWORD)pTileInfo->BitsBuffer.pu8Buffer;
    pTileInfo->dwTileHeight = pFrameInfo->dwB8Rows - pTileInfo->dwTileTop;

finish:
    return eStatus;
}


VAStatus Intel_HostvldVp9_ParseTileColumn(
    PINTEL_HOSTVLD_VP9_TILE_STATE   pTileState, 
    DWORD                              dwTileX)
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_TILE_INFO     pTileInfo;
    DWORD                               dwTileIndex;
    DWORD                               dwTileY;
    INT                                 iMarkerBit;
    VAStatus                          eStatus     = VA_STATUS_SUCCESS;


    pFrameState     = pTileState->pFrameState;
    pFrameInfo      = &pFrameState->FrameInfo;

    // decode one tile column
    for (dwTileY = 0; dwTileY < pFrameInfo->dwTileRows; dwTileY++)
    {
        dwTileIndex = dwTileY * pFrameInfo->dwTileColumns + dwTileX;
        pTileInfo   = pFrameInfo->TileInfo + dwTileIndex;

        // initialize BAC Engine
        iMarkerBit = Intel_HostvldVp9_BacEngineInit(
            &(pTileState->BacEngine),
            pTileInfo->BitsBuffer.pu8Buffer,
            pTileInfo->BitsBuffer.dwSize);
        if (0 != iMarkerBit)
        {
            eStatus = VA_STATUS_ERROR_DECODING_ERROR;
            goto finish;
        }

        // Parse one tile
        Intel_HostvldVp9_ParseOneTile(pTileState, pTileInfo);
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_PostParseTiles(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileStateBase, pTileState;
    DWORD                               i;
    VAStatus                          eStatus = VA_STATUS_SUCCESS;


    Intel_HostvldVp9_SetTileIndex(pFrameState);

    Intel_HostvldVp9_SetOutOfBoundValues(pFrameState);

    // Combine counts of different tile columns for context update
    pFrameInfo = &pFrameState->FrameInfo;
    if (!pFrameInfo->bErrorResilientMode && pFrameInfo->bFrameParallelDisabled)
    {
        pTileStateBase = pFrameState->pTileStateBase;
        for (i = 1; i < pFrameState->dwTileStatesInUse; i++)
        {
            pTileState = pTileStateBase + i;
            Intel_HostvldVp9_MergeCounts(pFrameInfo, &pTileStateBase->Count, &pTileState->Count);
        }
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_ParseTiles(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState;
    DWORD                               dwTileX;
    VAStatus                          eStatus = VA_STATUS_SUCCESS;


    pFrameInfo = &pFrameState->FrameInfo;
    pTileState = pFrameState->pTileStateBase;

    pTileState->pFrameState = pFrameState;

    Intel_HostvldVp9_PreParseTiles(pFrameState);

    // decode tile columns
    for (dwTileX = 0; dwTileX < pFrameInfo->dwTileColumns; dwTileX++)
    {
        Intel_HostvldVp9_ParseTileColumn(pTileState, dwTileX);
    }

    Intel_HostvldVp9_PostParseTiles(pFrameState);

finish:
    return eStatus;
}
