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

/*
 * Copyright (c) 2010, The WebM Project authors. All rights reserved.
 *
 * An additional intellectual property rights grant can be found
 * in the file LIBVPX_PATENTS.  All contributing project authors may
 * be found in the LIBVPX_AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.

 * Neither the name of Google, nor the WebM Project, nor the names
 * of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdlib.h>
#include "intel_hybrid_hostvld_vp9_parser.h"
#include "intel_hybrid_hostvld_vp9_parser_tables.h"
#include "intel_hybrid_hostvld_vp9_context.h"
#include "intel_hybrid_hostvld_vp9_engine.h"

#define VP9_INVALID_MV_VALUE    0x80008000

#define VP9_PROP8x8(pSyntax, Value)\
    Intel_HostvldVp9_PropagateByte(g_Vp9Propagate8x8, BlkSize, (pSyntax), (Value))
#define VP9_PROP8x8_WORD(pSyntax, Value)\
    Intel_HostvldVp9_PropagateWord(g_Vp9Propagate8x8, BlkSize, (pSyntax), (Value))
#define VP9_PROP4x4_QWORD(pSyntax, Value)\
    Intel_HostvldVp9_PropagateQWord(g_Vp9Propagate4x4, BlkSize, (pSyntax), (Value))

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

#define VP9_GET_TX_TYPE(TxSize, IsLossLess, PredModeLuma) \
    (((TxSize) == TX_4X4 && (IsLossLess))? TX_DCT : g_Vp9Mode2TxTypeMap[(PredModeLuma)])

// Shift BAC engine
#define INTEL_HOSTVLD_VP9_BACENGINE_SHIFT()          \
do                                                      \
{                                                       \
    uiShift = g_Vp9NormTable[uiRange];                  \
    uiRange  <<= uiShift;                               \
    BacValue <<= uiShift;                               \
    iCount    -= uiShift;                               \
} while (0)

// Update BAC engine
#define INTEL_HOSTVLD_VP9_BACENGINE_UPDATE(Bit)          \
do                                                          \
{                                                           \
    Bit = (BacValue >= BacSplitValue);                      \
    uiRange  = Bit ? (uiRange - uiSplit) : uiSplit;         \
    BacValue = Bit ? (BacValue - BacSplitValue) : BacValue; \
} while (0)

// Read one bit
#define INTEL_HOSTVLD_VP9_READ_MODE_BIT(iProb, Bit)  \
do                                                      \
{                                                       \
    INTEL_HOSTVLD_VP9_BACENGINE_SHIFT();             \
                                                        \
    uiSplit       = ((uiRange * iProb) + (BAC_ENG_PROB_RANGE - iProb)) >> BAC_ENG_PROB_BITS;             \
    BacSplitValue = (INTEL_HOSTVLD_VP9_BAC_VALUE)uiSplit << (BAC_ENG_VALUE_BITS - BAC_ENG_PROB_BITS); \
                                                        \
    INTEL_HOSTVLD_VP9_BACENGINE_FILL();              \
                                                        \
    INTEL_HOSTVLD_VP9_BACENGINE_UPDATE(Bit);         \
} while (0)

// Read one bit
#define INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(iProb)   \
do                                                      \
{                                                       \
    INTEL_HOSTVLD_VP9_BACENGINE_SHIFT();             \
                                                        \
    uiSplit       = ((uiRange * iProb) + (BAC_ENG_PROB_RANGE - iProb)) >> BAC_ENG_PROB_BITS; \
    BacSplitValue = (INTEL_HOSTVLD_VP9_BAC_VALUE)uiSplit << (BAC_ENG_VALUE_BITS - BAC_ENG_PROB_BITS); \
                                                        \
    INTEL_HOSTVLD_VP9_BACENGINE_FILL();              \
} while (0)

// Write Coeff and Continue to next coeff
#define VP9_WRITE_COEF_CONTINUE(val, token)                                       \
{                                                                                 \
    INTEL_HOSTVLD_VP9_BACENGINE_SHIFT();                                       \
                                                                                  \
    uiSplit       = (uiRange + 1) >> 1;                                           \
    BacSplitValue = (INTEL_HOSTVLD_VP9_BAC_VALUE)uiSplit << (BAC_ENG_VALUE_BITS - BAC_ENG_PROB_BITS); \
                                                                                  \
    INTEL_HOSTVLD_VP9_BACENGINE_FILL();                                        \
                                                                                  \
    INTEL_HOSTVLD_VP9_BACENGINE_UPDATE(iBit);                                  \
                                                                                  \
    pCoeffAddr[pScan[CoeffIdx]] = iBit ? (INT16)(-val) : (INT16)(val);            \
    pMbInfo->TokenCache[pScan[CoeffIdx]] = g_Vp9PtEnergyClass[token];             \
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
    pMbInfo->TokenCache[pScan[CoeffIdx]] = g_Vp9PtEnergyClass[token];             \
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

static inline VOID Intel_HostvldVp9_InitTokenParser(
    PINTEL_HOSTVLD_VP9_TILE_STATE   pTileState,
    PINTEL_HOSTVLD_VP9_TILE_INFO    pTileInfo)
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER pOutputBuffer;
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pVideoBuffer;
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    INT                                 iOffset;

    pFrameState     = pTileState->pFrameState;
    pOutputBuffer   = pFrameState->pOutputBuffer;
    pVideoBuffer    = pFrameState->pVideoBuffer;
    pFrameInfo      = &pFrameState->FrameInfo;
    pMbInfo         = &pTileState->MbInfo;
    iOffset         = pTileInfo->dwTileLeft << VP9_LOG2_B64_SIZE_IN_B8;

    // 8x8 token
    pMbInfo->pLastSegmentId  = pFrameState->pLastSegIdBuf->pu8Buffer        + iOffset;
    pMbInfo->pReferenceFrame = pOutputBuffer->ReferenceFrame.pu16Buffer     + iOffset;
    pMbInfo->pPrevRefFrame   = pVideoBuffer->PrevReferenceFrame.pu16Buffer  + iOffset;

    iOffset <<= 2;
    // 4x4 token
    pMbInfo->pMotionVector = (PINTEL_HOSTVLD_VP9_MV)pOutputBuffer->MotionVector.pu32Buffer       + (iOffset << 1); // 2 MVs per block
    pMbInfo->pPrevMv       = (PINTEL_HOSTVLD_VP9_MV)pVideoBuffer->PrevMotionVector.pu32Buffer    + (iOffset << 1); // 2 MVs per block

    pMbInfo->i8ZOrder   = 0;
    pMbInfo->dwLineDist = pFrameInfo->dwMbStride - ALIGN(pTileInfo->dwTileWidth, VP9_B64_SIZE_IN_B8);
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
    pMbInfo->pLastSegmentId  += iOffset;
    pMbInfo->pReferenceFrame += iOffset;
    pMbInfo->pPrevRefFrame   += iOffset;

    iOffset <<= 3;
    // 4x4 token
    pMbInfo->pMotionVector   += iOffset;   // 2 MVs per block
    pMbInfo->pPrevMv         += iOffset;   // 2 MVs per block

    pMbInfo->i8ZOrder = (UINT8)iZOrder;
}

static inline INTEL_HOSTVLD_VP9_MB_PRED_MODE Intel_HostvldVp9_ReadIntraMode_KeyFrmY(
    PINTEL_HOSTVLD_VP9_MB_INFO     pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE  pBacEngine,
    UINT8                             ModeAbove,
    UINT8                             ModeLeft)
{
    return (INTEL_HOSTVLD_VP9_MB_PRED_MODE)INTEL_HOSTVLD_VP9_READ_TREE(
        g_Vp9TknTreeIntra_KF_Y[ModeAbove][ModeLeft]);
}

static inline VOID Intel_HostvldVp9_ReadIntraMode_KeyFrmUV(
    PINTEL_HOSTVLD_VP9_MB_INFO     pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE  pBacEngine,
    UINT8                             ModeY)
{
    pMbInfo->pMode->DW0.ui8PredModeChroma = (INTEL_HOSTVLD_VP9_MB_PRED_MODE)INTEL_HOSTVLD_VP9_READ_TREE(
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
    PINTEL_HOSTVLD_VP9_MODE_INFO     pMode;
    INTEL_HOSTVLD_VP9_TX_PROB_TABLE  TxProbTable;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlkSize;
    INTEL_HOSTVLD_VP9_TX_SIZE        MaxTxSize;
    PUINT8                              pProbs;
    UINT8                               ui8Ctx, ui8LCtx, ui8ACtx, ui8TxSize;

    pFrameInfo = &pTileState->pFrameState->FrameInfo;
    pMode      = pMbInfo->pMode;
    BlkSize    = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)pMode->DW0.ui8BlockSize;
    MaxTxSize  = g_Vp9MaxTxSizeTable[BlkSize];

    ui8TxSize = MIN(MaxTxSize, g_Vp9TxSizeModeLimit[pFrameInfo->TxMode]);
    if ((pMode->DW1.ui8Flags ^ ((1 << VP9_SKIP_FLAG) | (1 << VP9_IS_INTER_FLAG))) && // ( not skipped or is not inter )
        (pFrameInfo->TxMode == TX_MODE_SELECT)       && 
        (pMbInfo->iB4Number >= 4))
    {
        ui8LCtx = ui8LSkip || !pMbInfo->bLeftValid  ?
            (UINT8)MaxTxSize : pMbInfo->pContextLeft->DW0.ui8TxSizeLuma;
        ui8ACtx = ui8ASkip || !pMbInfo->bAboveValid ?
            (UINT8)MaxTxSize : pMbInfo->pContextAbove->DW0.ui8TxSizeLuma;
        ui8Ctx  = ((pMbInfo->bLeftValid ? ui8LCtx : ui8ACtx) +
            (pMbInfo->bAboveValid ? ui8ACtx: ui8LCtx) > MaxTxSize);

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
    pMode->DW1.ui8TxSizeLuma   = ui8TxSize;
    pMode->DW0.ui8TxSizeChroma = MIN(ui8TxSize, (UINT8)g_Vp9MaxChromaTxSize[BlkSize]);
}

VAStatus Intel_HostvldVp9_ParseIntraFrameModeInfo(
    PINTEL_HOSTVLD_VP9_TILE_STATE   pTileState)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo;
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlkSize;
    INTEL_HOSTVLD_VP9_TX_SIZE        MaxTxSize;
    PINTEL_HOSTVLD_VP9_MODE_INFO     pMode;

    UINT8      ui8PredModeLeft[2], ui8PredModeAbove[2];
    UINT8      ui8Ctx, ui8LSkip, ui8ASkip;
    UINT8      uiSegId, ui8SkipCoeff;
    INT        iBlkW4x4, iBlkH4x4, iX4x4, iY4x4;
    VAStatus eStatus = VA_STATUS_SUCCESS;

    pFrameInfo = &pTileState->pFrameState->FrameInfo;
    pMbInfo    = &pTileState->MbInfo;
    pBacEngine = &pTileState->BacEngine;
    pMode      = pMbInfo->pMode;

    BlkSize   = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)pMode->DW0.ui8BlockSize;
    MaxTxSize = g_Vp9MaxTxSizeTable[BlkSize];

    pMbInfo->pRefFrameIndex[0] = VP9_REF_FRAME_INTRA;
    pMbInfo->pRefFrameIndex[1] = VP9_REF_FRAME_INTRA;

    // segment id
    uiSegId = 0;
    if (pFrameInfo->ui8SegEnabled && pFrameInfo->ui8SegUpdMap)
    {
        uiSegId = (UINT8)INTEL_HOSTVLD_VP9_READ_TREE(pFrameInfo->pContext->SegmentTree);
        VP9_PROP8x8(pMbInfo->pLastSegmentId, uiSegId);
    }
    pMode->DW0.ui8SegId = uiSegId;
    pMbInfo->bSegRefSkip =
        pFrameInfo->pSegmentData->SegData[uiSegId].SegmentFlags.fields.SegmentReferenceSkipped;

    // skip coefficient flag
    ui8SkipCoeff = 1;
    ui8LSkip     = pMbInfo->pContextLeft->DW0.ui8SkipFlag;
    ui8ASkip     = pMbInfo->pContextAbove->DW0.ui8SkipFlag;
    if (!pMbInfo->bSegRefSkip)
    {
        ui8Ctx       = ui8LSkip + ui8ASkip;
        ui8SkipCoeff = (UINT8)INTEL_HOSTVLD_VP9_READ_BIT(pFrameInfo->pContext->MbSkipProbs[ui8Ctx]);

        pTileState->Count.MbSkipCounts[ui8Ctx][ui8SkipCoeff] += pFrameInfo->bFrameParallelDisabled;
    }
    pMode->DW1.ui8Flags = ui8SkipCoeff; // "is inter" flag is FALSE

	// transform size
    Intel_HostvldVp9_ParseTransformSize(pTileState, pMbInfo, pBacEngine, ui8LSkip, ui8ASkip);

    // transform type and prediction mode
    if (pMbInfo->iB4Number < 4) // 4X4, 4X8 or 8X4
    {
        iBlkW4x4            = g_Vp9BlkW4x4[BlkSize];
        iBlkH4x4            = g_Vp9BlkH4x4[BlkSize];

        if (pMbInfo->bLeftValid)
        {
            ui8PredModeLeft[0]  = pMbInfo->pModeLeft->PredModeLuma[0][1];
            ui8PredModeLeft[1]  = pMbInfo->pModeLeft->PredModeLuma[1][1];
        }
        else
        {
            ui8PredModeLeft[0]  = PRED_MD_DC;
            ui8PredModeLeft[1]  = PRED_MD_DC;
        }

        if (pMbInfo->bAboveValid)
        {
            ui8PredModeAbove[0] = pMbInfo->pModeAbove->PredModeLuma[1][0];
            ui8PredModeAbove[1] = pMbInfo->pModeAbove->PredModeLuma[1][1];
        }
        else
        {
            ui8PredModeAbove[0] = PRED_MD_DC;
            ui8PredModeAbove[1] = PRED_MD_DC;
        }

        for (iY4x4 = 0; iY4x4 < 2; iY4x4 += iBlkH4x4)
        {
            for (iX4x4 = 0; iX4x4 < 2; iX4x4 += iBlkW4x4)
            {
                // No check of IsInter flag for key/intra-only frame
                pMode->PredModeLuma[iY4x4][iX4x4] = Intel_HostvldVp9_ReadIntraMode_KeyFrmY(
                    pMbInfo, pBacEngine, ui8PredModeAbove[iX4x4], ui8PredModeLeft[iY4x4]);
                ui8PredModeLeft[iY4x4]  = pMode->PredModeLuma[iY4x4][iX4x4];
                ui8PredModeAbove[iX4x4] = pMode->PredModeLuma[iY4x4][iX4x4];

                pMode->TxTypeLuma[iY4x4][iX4x4] = VP9_GET_TX_TYPE(
                    pMbInfo->pMode->DW1.ui8TxSizeLuma,
                    pFrameInfo->bLossLess,
                    pMode->PredModeLuma[iY4x4][iX4x4]);
            }

            if (BlkSize == BLOCK_8X4)
            {
                pMode->PredModeLuma[iY4x4][1] = pMode->PredModeLuma[iY4x4][0];
                pMode->TxTypeLuma[iY4x4][1]   = pMode->TxTypeLuma[iY4x4][0];
            }
        }

        if (BlkSize == BLOCK_4X8)
        {
            pMode->PredModeLuma[1][0] = pMode->PredModeLuma[0][0];
            pMode->PredModeLuma[1][1] = pMode->PredModeLuma[0][1];
            pMode->TxTypeLuma[1][0]   = pMode->TxTypeLuma[0][0];
            pMode->TxTypeLuma[1][1]   = pMode->TxTypeLuma[0][1];
        }
    }
    else // >= 8X8
    {
        ui8PredModeLeft[0]    = pMbInfo->bLeftValid  ? pMbInfo->pModeLeft->PredModeLuma[0][1]  : (UINT8)PRED_MD_DC;
        ui8PredModeAbove[0]   = pMbInfo->bAboveValid ? pMbInfo->pModeAbove->PredModeLuma[1][0] : (UINT8)PRED_MD_DC;
        pMode->dwPredModeLuma = Intel_HostvldVp9_ReadIntraMode_KeyFrmY(
            pMbInfo, pBacEngine, ui8PredModeAbove[0], ui8PredModeLeft[0]);
        pMode->dwPredModeLuma = (pMode->dwPredModeLuma << 8) + pMode->dwPredModeLuma;
        pMode->dwPredModeLuma = (pMode->dwPredModeLuma << 16) + pMode->dwPredModeLuma;

        pMode->dwTxTypeLuma = VP9_GET_TX_TYPE(
            pMbInfo->pMode->DW1.ui8TxSizeLuma,
            pFrameInfo->bLossLess,
            pMode->PredModeLuma[0][0]);
        pMode->dwTxTypeLuma = (pMode->dwTxTypeLuma << 8) + pMode->dwTxTypeLuma;
        pMode->dwTxTypeLuma = (pMode->dwTxTypeLuma << 16) + pMode->dwTxTypeLuma;
    }

    Intel_HostvldVp9_ReadIntraMode_KeyFrmUV(pMbInfo, pBacEngine, pMode->PredModeLuma[1][1]);

    pMode->DW1.ui8FilterLevel =
        pFrameInfo->pSegmentData->SegData[uiSegId].FilterLevel[VP9_REF_FRAME_INTRA + 1][0];

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
    PINTEL_HOSTVLD_VP9_MODE_INFO     pMode       = pMbInfo->pMode;
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine  = &pTileState->BacEngine;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlkSize     = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)pMode->DW0.ui8BlockSize;
    PINTEL_HOSTVLD_VP9_MV            pMotionVector;
    DWORD                               dwPredModeLuma;

    VAStatus  eStatus   = VA_STATUS_SUCCESS;

    pMotionVector = pMbInfo->pMotionVector;

    pMbInfo->pRefFrameIndex[0] = VP9_REF_FRAME_INTRA;
    pMbInfo->pRefFrameIndex[1] = VP9_REF_FRAME_INTRA;
    VP9_PROP8x8_WORD(pMbInfo->pReferenceFrame, *((PUINT16)pMbInfo->pRefFrameIndex));

    if (pMbInfo->iB4Number >= 4)
    {
        dwPredModeLuma = Intel_HostvldVp9_ReadIntraMode_InterFrmY(
            pTileState, pBacEngine, BlkSize);
        dwPredModeLuma = (dwPredModeLuma << 8) + dwPredModeLuma;
        pMode->dwPredModeLuma = (dwPredModeLuma << 16) + dwPredModeLuma;

        pMode->dwTxTypeLuma = VP9_GET_TX_TYPE(
            pMode->DW1.ui8TxSizeLuma,
            pFrameInfo->bLossLess,
            pMode->PredModeLuma[0][0]);
        pMode->dwTxTypeLuma = (pMode->dwTxTypeLuma << 8) + pMode->dwTxTypeLuma;
        pMode->dwTxTypeLuma = (pMode->dwTxTypeLuma << 16) + pMode->dwTxTypeLuma;
        VP9_PROP4x4_QWORD((PUINT64)pMotionVector, 0);
    }
    else // 4x4, 4x8 or 8x4
    {
        INT    iX, iY;
        INT    iWidth4x4, iHeight4x4;

        iWidth4x4   = 1 << (g_Vp9BlockSizeB4Log2[BlkSize][0]);
        iHeight4x4  = 1 << (g_Vp9BlockSizeB4Log2[BlkSize][1]);

        for (iY = 0; iY < 2; iY += iHeight4x4)
        {
            for (iX = 0; iX < 2; iX += iWidth4x4)
            {
                pMode->PredModeLuma[iY][iX] = Intel_HostvldVp9_ReadIntraMode_InterFrmY(
                    pTileState, pBacEngine, BlkSize);

                pMode->TxTypeLuma[iY][iX] = VP9_GET_TX_TYPE(
                    pMode->DW1.ui8TxSizeLuma,
                    pFrameInfo->bLossLess,
                    pMode->PredModeLuma[iY][iX]);
                VP9_PROP4x4_QWORD((PUINT64)pMotionVector, 0);
            }

            if (BlkSize == BLOCK_8X4)
            {
                pMode->PredModeLuma[iY][1] = pMode->PredModeLuma[iY][0];
                pMode->TxTypeLuma[iY][1]   = pMode->TxTypeLuma[iY][0];
            }
        }

        if (BlkSize == BLOCK_4X8)
        {
            pMode->PredModeLuma[1][0] = pMode->PredModeLuma[0][0];
            pMode->PredModeLuma[1][1] = pMode->PredModeLuma[0][1];
            pMode->TxTypeLuma[1][0]   = pMode->TxTypeLuma[0][0];
            pMode->TxTypeLuma[1][1]   = pMode->TxTypeLuma[0][1];
        }
    }

    pMode->DW0.ui8PredModeChroma = Intel_HostvldVp9_ReadIntraMode_InterFrmUV(
        pTileState, pBacEngine, pMode->PredModeLuma[1][1]);

    pMode->DW1.ui8FilterLevel =
        pFrameInfo->pSegmentData->SegData[pMode->DW0.ui8SegId].FilterLevel[VP9_REF_FRAME_INTRA + 1][0];

    return eStatus;
}

static VAStatus Intel_HostvldVp9_ParseRefereceFrames(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine,
    PUINT8                              pRefFrameForward)
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
    pReferenceFrame = pMbInfo->pRefFrameIndex;
    BlkSize         = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)pMbInfo->pMode->DW0.ui8BlockSize;

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
                        !pMbInfo->pContextAbove->DW0.ui8IsInter);
                }
                else if (VP9_IS_SINGLE(Left))
                {
                    dwContext = 2 + 
                        ((ReferenceFrameLeft[0] == pFrameInfo->CompondFixedRef) || 
                        !pMbInfo->pContextLeft->DW0.ui8IsInter);
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
                UINT uiLIsIntra  = !pMbInfo->pContextLeft->DW0.ui8IsInter;
                UINT uiAIsIntra  = !pMbInfo->pContextAbove->DW0.ui8IsInter;

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

                if ((pMbInfo->bLeftValid  && !pMbInfo->pContextLeft->DW0.ui8IsInter) ||
                    (pMbInfo->bAboveValid && !pMbInfo->pContextAbove->DW0.ui8IsInter))
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
                UINT uiLIsIntra  = !pMbInfo->pContextLeft->DW0.ui8IsInter;
                UINT uiAIsIntra  = !pMbInfo->pContextAbove->DW0.ui8IsInter;

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

                if ((pMbInfo->bLeftValid  && !pMbInfo->pContextLeft->DW0.ui8IsInter) ||
                    (pMbInfo->bAboveValid && !pMbInfo->pContextAbove->DW0.ui8IsInter))
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
                    UINT uiLIsIntra  = !pMbInfo->pContextLeft->DW0.ui8IsInter;
                    UINT uiAIsIntra  = !pMbInfo->pContextAbove->DW0.ui8IsInter;

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
                    bIsInterBlock    = (pMbInfo->bLeftValid  && !pMbInfo->pContextLeft->DW0.ui8IsInter) ||
                                       (pMbInfo->bAboveValid && !pMbInfo->pContextAbove->DW0.ui8IsInter);

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

    *pRefFrameForward = *pReferenceFrame;

    return eStatus;
}

static INT Intel_HostvldVp9_GetInterModeContext(
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo)
{
    INT i, iCounter, iOffset, iX, iY;
    const INTEL_HOSTVLD_VP9_MV *pMv;

    iCounter = 0;
    pMv      = g_Vp9MvRefBlocks + (pMbInfo->pMode->DW0.ui8BlockSize << VP9_LOG2_MV_REF_NEIGHBOURS);
    for (i = 0; i < 2; i++)
    {
        iX = pMbInfo->dwMbPosX + pMv->i16X;
        iY = pMbInfo->dwMbPosY + pMv->i16Y;
        if (VP9_IN_TILE_COLUMN(iX, iY))
        {
            iOffset = (iY & ~7) * pFrameInfo->dwMbStride + ((iX & ~7) << VP9_LOG2_B64_SIZE_IN_B8);
            iOffset += ((iY & 7) << VP9_LOG2_B64_SIZE_IN_B8) + (iX & 7);
            iCounter += g_Vp9ModeContextCounter[((PINTEL_HOSTVLD_VP9_MODE_INFO)pFrameInfo->ModeInfo.pBuffer)[iOffset].PredModeLuma[1][1]];
        }
        pMv++;
    }

    return g_Vp9ModeContext[iCounter];
}

static INTEL_HOSTVLD_VP9_MB_PRED_MODE Intel_HostvldVp9_ParseInterMode(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine, 
    INT                                 iContext)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    DWORD                               dwPredMode;
    PUINT8                              pContext;
    INTEL_HOSTVLD_VP9_BAC_VALUE      BacValue;
    INT                                 iCount, iBit;
    UINT                                uiSplit, uiRange, uiShift;
    INTEL_HOSTVLD_VP9_BAC_VALUE      BacSplitValue;

    // block size must be >= 8x8
    pFrameInfo = &pTileState->pFrameState->FrameInfo;
    pContext   = pFrameInfo->pContext->InterModeProbs[iContext];

    uiRange     = pBacEngine->uiRange;
    BacValue    = pBacEngine->BacValue;
    iCount      = pBacEngine->iCount;

    INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pContext[0]);
    if (BacValue >= BacSplitValue)
    {
        uiRange -= uiSplit;
        BacValue -= BacSplitValue;

        INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pContext[1]);
        if (BacValue >= BacSplitValue)
        {
            uiRange -= uiSplit;
            BacValue -= BacSplitValue;

            INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pContext[2]);
            if (BacValue >= BacSplitValue)
            {
                uiRange -= uiSplit;
                BacValue -= BacSplitValue;
                dwPredMode = PRED_MD_NEWMV;
            }
            else
            {
                uiRange = uiSplit;
                dwPredMode = PRED_MD_NEARMV;
            }
        }
        else
        {
            uiRange = uiSplit;
            dwPredMode = PRED_MD_NEARESTMV;
        }
    }
    else
    {
        uiRange = uiSplit;
        dwPredMode = PRED_MD_ZEROMV;
    }

    pBacEngine->BacValue = BacValue;
    pBacEngine->iCount   = iCount;
    pBacEngine->uiRange  = uiRange;

    pTileState->Count.InterModeCounts[iContext][dwPredMode - PRED_MD_NEARESTMV] += pFrameInfo->bFrameParallelDisabled;

    return (INTEL_HOSTVLD_VP9_MB_PRED_MODE)dwPredMode;
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
        BOOL bLIsInter = pMbInfo->bLeftValid && pMbInfo->pContextLeft->DW0.ui8IsInter;
        BOOL bAIsInter = pMbInfo->bAboveValid && pMbInfo->pContextAbove->DW0.ui8IsInter;
        INT  iLFilter  = bLIsInter ? pMbInfo->pContextLeft->DW0.ui8FilterType : VP9_SWITCHABLE_FILTERS;
        INT  iAFilter  = bAIsInter ? pMbInfo->pContextAbove->DW0.ui8FilterType : VP9_SWITCHABLE_FILTERS;

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
                pMbInfo->pMode->DW1.ui8FilterType = VP9_INTERP_EIGHTTAP_SHARP;
            }
            else
            {
                pMbInfo->pMode->DW1.ui8FilterType = VP9_INTERP_EIGHTTAP_SMOOTH;
            }
        }
        else
        {
            pMbInfo->pMode->DW1.ui8FilterType = VP9_INTERP_EIGHTTAP;
        }

        pTileState->Count.SwitchableInterpCounts[iContext][pMbInfo->pMode->DW1.ui8FilterType] += pFrameInfo->bFrameParallelDisabled;
    }
    else
    {
        pMbInfo->pMode->DW1.ui8FilterType = pFrameInfo->eInterpolationType;
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
    i8RefFrame          = pMbInfo->pRefFrameIndex[bIsSecondRef];

    // check the neighbors first
    pMv      = g_Vp9MvRefBlocks + (pMbInfo->pMode->DW0.ui8BlockSize << VP9_LOG2_MV_REF_NEIGHBOURS);
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
        pMv = g_Vp9MvRefBlocks + (pMbInfo->pMode->DW0.ui8BlockSize << VP9_LOG2_MV_REF_NEIGHBOURS);
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
        i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
        NearestMv.i16X = INTEL_VP9_CLAMP(NearestMv.i16X, i32MvMin, i32MvMax);
        i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
        i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
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
            i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_MARGIN;
            NearestMv.i16X = INTEL_VP9_CLAMP(NearestMv.i16X, i32MvMin, i32MvMax);
            i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_MARGIN;
            i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_MARGIN;
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
    i8RefFrame          = pMbInfo->pRefFrameIndex[bIsSecondRef];

    // check the neighbors first
    pMv      = g_Vp9MvRefBlocks + (pMbInfo->pMode->DW0.ui8BlockSize << VP9_LOG2_MV_REF_NEIGHBOURS);
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
        pMv = g_Vp9MvRefBlocks + (pMbInfo->pMode->DW0.ui8BlockSize << VP9_LOG2_MV_REF_NEIGHBOURS);
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
        i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
        NearMv.i16X = INTEL_VP9_CLAMP(NearMv.i16X, i32MvMin, i32MvMax);
        i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
        i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
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
            i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_MARGIN;
            NearMv.i16X = INTEL_VP9_CLAMP(NearMv.i16X, i32MvMin, i32MvMax);
            i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_MARGIN;
            i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_MARGIN;
            NearMv.i16Y = INTEL_VP9_CLAMP(NearMv.i16Y, i32MvMin, i32MvMax);
        }
        pMbInfo->NearMv[bIsSecondRef].dwValue = NearMv.dwValue;
    }
    else if ((iBlockIndex == 1) || (iBlockIndex == 2))
    {
        i32MvMin = -static_cast<int>(pMbInfo->dwMbPosX << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
        i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
        NearestMv.i16X = INTEL_VP9_CLAMP(NearestMv.i16X, i32MvMin, i32MvMax);
        i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
        i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
        NearestMv.i16Y = INTEL_VP9_CLAMP(NearestMv.i16Y, i32MvMin, i32MvMax);

        Mv.dwValue = pMbInfo->pMotionVector[0 * 2 + bIsSecondRef].dwValue;
        if (NearestMv.dwValue != Mv.dwValue)
        {
            pMbInfo->NearMv[bIsSecondRef].dwValue = NearestMv.dwValue;
        }
        else 
        {
            i32MvMin = -static_cast<int>(pMbInfo->dwMbPosX << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
            i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
            NearMv.i16X = INTEL_VP9_CLAMP(NearMv.i16X, i32MvMin, i32MvMax);
            i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
            i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
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
            i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
            NearestMv.i16X = INTEL_VP9_CLAMP(NearestMv.i16X, i32MvMin, i32MvMax);
            i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
            i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
            NearestMv.i16Y = INTEL_VP9_CLAMP(NearestMv.i16Y, i32MvMin, i32MvMax);

            if (NearestMv.dwValue != Mv.dwValue)
            {
                pMbInfo->NearMv[bIsSecondRef].dwValue = NearestMv.dwValue;
            }
            else 
            {
                i32MvMin = -static_cast<int>(pMbInfo->dwMbPosX << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
                i32MvMax = (((INT32)pFrameInfo->dwB8Columns - pMbInfo->dwMbPosX - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][0]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
                NearMv.i16X = INTEL_VP9_CLAMP(NearMv.i16X, i32MvMin, i32MvMax);
                i32MvMin = -static_cast<int>(pMbInfo->dwMbPosY << (VP9_LOG2_B8_SIZE + 4)) - VP9_MV_BORDER;
                i32MvMax = (((INT32)pFrameInfo->dwB8Rows - pMbInfo->dwMbPosY - g_Vp9BlockSizeB8[pMbInfo->pMode->DW0.ui8BlockSize][1]) << (VP9_LOG2_B8_SIZE + 4)) + VP9_MV_BORDER;
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

    INTEL_HOSTVLD_VP9_BAC_VALUE      BacValue;
    INT                                 iCount, iBit;
    UINT                                uiSplit, uiRange, uiShift;
    INTEL_HOSTVLD_VP9_BAC_VALUE      BacSplitValue;

    pFrameInfo  = &pTileState->pFrameState->FrameInfo;
    pMvProbSet  = &pFrameInfo->pContext->MvProbSet[!MvComponent];
    pMvCountSet = &pTileState->Count.MvCountSet[!MvComponent];

    uiRange     = pBacEngine->uiRange;
    BacValue    = pBacEngine->BacValue;
    iCount      = pBacEngine->iCount;

    // read sign bit
    INTEL_HOSTVLD_VP9_READ_MODE_BIT(pMvProbSet->MvSignProbs, iSign);
    pMvCountSet->MvSignCounts[iSign]++;

    // read MV class
    pui8Probs = pMvProbSet->MvClassProbs;
    INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pui8Probs[0]);
    if (BacValue >= BacSplitValue)
    {
        uiRange  -= uiSplit;
        BacValue -= BacSplitValue;
        INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pui8Probs[1]);
        if (BacValue >= BacSplitValue)
        {
            uiRange  -= uiSplit;
            BacValue -= BacSplitValue;
            INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pui8Probs[2]);
            if (BacValue >= BacSplitValue)
            {
                uiRange  -= uiSplit;
                BacValue -= BacSplitValue;

                pBacEngine->BacValue = BacValue;
                pBacEngine->iCount   = iCount;
                pBacEngine->uiRange  = uiRange;

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

                uiRange     = pBacEngine->uiRange;
                BacValue    = pBacEngine->BacValue;
                iCount      = pBacEngine->iCount;
            }
            else
            {
                uiRange = uiSplit;
                INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pui8Probs[3]);
                if (BacValue >= BacSplitValue)
                {
                    uiRange  -= uiSplit;
                    BacValue -= BacSplitValue;
                    MvClass  = VP9_MV_CLASS_3;
                }
                else
                {
                    uiRange = uiSplit;
                    MvClass = VP9_MV_CLASS_2;
                }
            }
        }
        else
        {
            uiRange = uiSplit;
            MvClass = VP9_MV_CLASS_1;
        }
    }
    else
    {
        uiRange = uiSplit;
        MvClass = VP9_MV_CLASS_0;
    }

    pMvCountSet->MvClassCounts[MvClass]++;

    // read integer
    if (MvClass == VP9_MV_CLASS_0)
    {
        INTEL_HOSTVLD_VP9_READ_MODE_BIT(pMvProbSet->MvClass0Probs[0], iInteger);
        pMvCountSet->MvClass0Counts[iInteger]++;
    }
    else
    {
        INT i, iBits = MvClass + VP9_MV_CLASS0_BITS - 1;

        iInteger = 0;
        for (i = 0; i < iBits; i++)
        {
            INTEL_HOSTVLD_VP9_READ_MODE_BIT(pMvProbSet->MvBitsProbs[i], iBit);
            iInteger |= iBit << i;

            pMvCountSet->MvBitsCounts[i][iBit]++;
        }
    }

    // read fractional
    pui8Probs = (MvClass == VP9_MV_CLASS_0) ? 
        pMvProbSet->MvClass0FpProbs[iInteger] : pMvProbSet->MvFpProbs;
    INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pui8Probs[0]);
    if (BacValue >= BacSplitValue)
    {
        uiRange  -= uiSplit;
        BacValue -= BacSplitValue;
        INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pui8Probs[1]);
        if (BacValue >= BacSplitValue)
        {
            uiRange  -= uiSplit;
            BacValue -= BacSplitValue;
            INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pui8Probs[2]);
            if (BacValue >= BacSplitValue)
            {
                uiRange     -= uiSplit;
                BacValue    -= BacSplitValue;
                iFractional = 3;
            }
            else
            {
                uiRange     = uiSplit;
                iFractional = 2;
            }
        }
        else
        {
            uiRange     = uiSplit;
            iFractional = 1;
        }
    }
    else
    {
        uiRange     = uiSplit;
        iFractional = 0;
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
            INTEL_HOSTVLD_VP9_READ_MODE_BIT(pMvProbSet->MvClass0HpProbs, iHp);
            pMvCountSet->MvClass0HpCounts[iHp]++;
        }
        else
        {
            INTEL_HOSTVLD_VP9_READ_MODE_BIT(pMvProbSet->MvHpProbs, iHp);
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

    pBacEngine->BacValue = BacValue;
    pBacEngine->iCount   = iCount;
    pBacEngine->uiRange  = uiRange;

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

    pMbInfo->pMv[bIsSecondRef].i16X = BestMv.i16X + MvDiff.i16X;
    pMbInfo->pMv[bIsSecondRef].i16Y = BestMv.i16Y + MvDiff.i16Y;

    return eStatus;
}

static VAStatus Intel_HostvldVp9_ParseMotionVectors(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine, 
    INT                                 iBlockIndex,
    INTEL_HOSTVLD_VP9_MB_PRED_MODE   ePredMode)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_MV            pMv;
    VAStatus                          eStatus   = VA_STATUS_SUCCESS;

    pFrameInfo      = &pTileState->pFrameState->FrameInfo;
    pMv             = pMbInfo->pMv;
    pMv[1].dwValue  = 0;

    if (ePredMode == PRED_MD_NEWMV)
    {
        Intel_HostvldVp9_FindBestMv(pFrameInfo, pMbInfo, 0);
        Intel_HostvldVp9_ParseOneMv(pTileState, pMbInfo, pBacEngine, 0);

        if (pMbInfo->pRefFrameIndex[1] > VP9_REF_FRAME_INTRA)
        {
            Intel_HostvldVp9_FindBestMv(pFrameInfo, pMbInfo, 1);
            Intel_HostvldVp9_ParseOneMv(pTileState, pMbInfo, pBacEngine, 1);
        }
    }
    else if (ePredMode == PRED_MD_NEARESTMV)
    {
        Intel_HostvldVp9_FindNearestMv(pFrameInfo, pMbInfo, 0, iBlockIndex);
        pMv[0].dwValue = pMbInfo->NearestMv[0].dwValue;

        if (pMbInfo->pRefFrameIndex[1] > VP9_REF_FRAME_INTRA)
        {
            Intel_HostvldVp9_FindNearestMv(pFrameInfo, pMbInfo, 1, iBlockIndex);
            pMv[1].dwValue = pMbInfo->NearestMv[1].dwValue;
        }
    }
    else if (ePredMode == PRED_MD_NEARMV)
    {
        Intel_HostvldVp9_FindNearMv(pFrameInfo, pMbInfo, 0, iBlockIndex);
        pMv[0].dwValue = pMbInfo->NearMv[0].dwValue;

        if (pMbInfo->pRefFrameIndex[1] > VP9_REF_FRAME_INTRA)
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
    PINTEL_HOSTVLD_VP9_MODE_INFO  pMode      = pMbInfo->pMode;
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine = &pTileState->BacEngine;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE  BlkSize    = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)pMbInfo->pMode->DW0.ui8BlockSize;
    PINTEL_HOSTVLD_VP9_MV         pMotionVector;
    INT                              iContext   = 0;
    UINT8                            ui8RefFrameForward;
    VAStatus                       eStatus    = VA_STATUS_SUCCESS;

    pMotionVector   = pMbInfo->pMotionVector;

    Intel_HostvldVp9_ParseRefereceFrames(pTileState, pMbInfo, pBacEngine, &ui8RefFrameForward);

    // parse inter mode
    if (pMbInfo->bSegRefSkip)
    {
        pMode->dwPredModeLuma =
            ((DWORD)PRED_MD_ZEROMV << 24) | ((DWORD)PRED_MD_ZEROMV << 16) | ((DWORD)PRED_MD_ZEROMV << 8) | (DWORD)PRED_MD_ZEROMV;
        if (pMbInfo->iB4Number < 4)
        {
            eStatus = VA_STATUS_ERROR_UNKNOWN;
            goto finish;
        }
        pMode->dwTxTypeLuma = VP9_GET_TX_TYPE(
            pMode->DW1.ui8TxSizeLuma,
            pFrameInfo->bLossLess,
            pMode->PredModeLuma[0][0]);
        pMode->dwTxTypeLuma = (pMode->dwTxTypeLuma << 8) + pMode->dwTxTypeLuma;
        pMode->dwTxTypeLuma = (pMode->dwTxTypeLuma << 16) + pMode->dwTxTypeLuma;
    }
    else
    {
        iContext = Intel_HostvldVp9_GetInterModeContext(pFrameInfo, pMbInfo);
        if (pMbInfo->iB4Number >= 4)
        {
            pMode->dwPredModeLuma = Intel_HostvldVp9_ParseInterMode(pTileState, pMbInfo, pBacEngine, iContext);
            pMode->dwPredModeLuma = (pMode->dwPredModeLuma << 8)  + pMode->dwPredModeLuma;
            pMode->dwPredModeLuma = (pMode->dwPredModeLuma << 16) + pMode->dwPredModeLuma;
            pMode->dwTxTypeLuma = VP9_GET_TX_TYPE(
                pMode->DW1.ui8TxSizeLuma,
                pFrameInfo->bLossLess,
                pMode->PredModeLuma[0][0]);
            pMode->dwTxTypeLuma = (pMode->dwTxTypeLuma << 8) + pMode->dwTxTypeLuma;
            pMode->dwTxTypeLuma = (pMode->dwTxTypeLuma << 16) + pMode->dwTxTypeLuma;
        }
    }

    Intel_HostvldVp9_ParseInterpolationType(pTileState, pMbInfo, pBacEngine);

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
                pMode->PredModeLuma[iY][iX] =
                    Intel_HostvldVp9_ParseInterMode(pTileState, pMbInfo, pBacEngine, iContext);
                pMode->TxTypeLuma[iY][iX] = VP9_GET_TX_TYPE(
                    pMode->DW1.ui8TxSizeLuma,
                    pFrameInfo->bLossLess,
                    pMode->PredModeLuma[iY][iX]);

                iBlockIndex = iY * 2 + iX;
                Intel_HostvldVp9_ParseMotionVectors(pTileState, pMbInfo, pBacEngine, iBlockIndex,
                    (INTEL_HOSTVLD_VP9_MB_PRED_MODE)pMode->PredModeLuma[iY][iX]);
                VP9_PROP4x4_QWORD((PUINT64)pMotionVector, *((PUINT64)(pMbInfo->pMv)));

                pMotionVector += iWidth4x4 << 1;
            }

            if (BlkSize == BLOCK_8X4)
            {
                pMode->PredModeLuma[iY][1] = pMode->PredModeLuma[iY][0];
                pMode->TxTypeLuma[iY][1]   = pMode->TxTypeLuma[iY][0];
            }
        }

        if (BlkSize == BLOCK_4X8)
        {
            pMode->PredModeLuma[1][0] = pMode->PredModeLuma[0][0];
            pMode->PredModeLuma[1][1] = pMode->PredModeLuma[0][1];
            pMode->TxTypeLuma[1][0]   = pMode->TxTypeLuma[0][0];
            pMode->TxTypeLuma[1][1]   = pMode->TxTypeLuma[0][1];
        }
    }
    else
    {
        Intel_HostvldVp9_ParseMotionVectors(pTileState, pMbInfo, pBacEngine, -1,
            (INTEL_HOSTVLD_VP9_MB_PRED_MODE)pMode->PredModeLuma[0][0]);
        VP9_PROP4x4_QWORD((PUINT64)pMotionVector, *((PUINT64)(pMbInfo->pMv)));
    }

    pMode->DW0.ui8PredModeChroma = pMode->PredModeLuma[1][1];
    pMode->DW1.ui8FilterLevel    =
        pFrameInfo->pSegmentData->SegData[pMode->DW0.ui8SegId].FilterLevel[ui8RefFrameForward + 1]
                                                                          [pMode->PredModeLuma[1][1] != PRED_MD_ZEROMV];

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

    UINT8  ui8Ctx, ui8LSkip, ui8ASkip;
    UINT8  ui8SegId = 0, ui8SkipCoeff, ui8IsInter;

    VAStatus eStatus = VA_STATUS_SUCCESS;


    pFrameInfo = &pTileState->pFrameState->FrameInfo;
    pMbInfo    = &pTileState->MbInfo;
    pBacEngine = &pTileState->BacEngine;

    BlkSize = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)pMbInfo->pMode->DW0.ui8BlockSize;

    if (pFrameInfo->ui8SegEnabled)
    {
        ui8SegId = Intel_HostvldVp9_GetPredSegmentId(pMbInfo->pLastSegmentId, g_Vp9Propagate8x8[BlkSize]);

        if (pFrameInfo->ui8SegUpdMap)
        {
            BOOL bSegPredFlag = FALSE;
            if (pFrameInfo->ui8TemporalUpd)
            {
                ui8Ctx  = pMbInfo->pContextLeft->DW1.ui8SegPredFlag + pMbInfo->pContextAbove->DW1.ui8SegPredFlag;
                bSegPredFlag = INTEL_HOSTVLD_VP9_READ_BIT(pFrameInfo->pContext->SegPredProbs[ui8Ctx]);
            }

            if (!bSegPredFlag)
            {
                ui8SegId = (UINT8)INTEL_HOSTVLD_VP9_READ_TREE(pFrameInfo->pContext->SegmentTree);
            }

            VP9_PROP8x8(pMbInfo->pLastSegmentId, ui8SegId);
            pMbInfo->pContextLeft->DW1.ui8SegPredFlag  = (UINT8)bSegPredFlag;
            pMbInfo->pContextAbove->DW1.ui8SegPredFlag = (UINT8)bSegPredFlag;
        }
    }
    pMbInfo->pMode->DW0.ui8SegId = ui8SegId;
    pMbInfo->bSegRefSkip = pFrameInfo->ui8SegEnabled &&
        pFrameInfo->pSegmentData->SegData[ui8SegId].SegmentFlags.fields.SegmentReferenceSkipped;
    pMbInfo->bSegRefEnabled = pFrameInfo->ui8SegEnabled &&
        pFrameInfo->pSegmentData->SegData[ui8SegId].SegmentFlags.fields.SegmentReferenceEnabled;

    // read skip flag
    ui8SkipCoeff = 1;
    ui8LSkip     = pMbInfo->pContextLeft->DW0.ui8SkipFlag;
    ui8ASkip     = pMbInfo->pContextAbove->DW0.ui8SkipFlag;
    if (!pMbInfo->bSegRefSkip)
    {
        ui8Ctx       = ui8LSkip + ui8ASkip;
        ui8SkipCoeff = (UINT8)INTEL_HOSTVLD_VP9_READ_BIT(pFrameInfo->pContext->MbSkipProbs[ui8Ctx]);

        pTileState->Count.MbSkipCounts[ui8Ctx][ui8SkipCoeff] += pFrameInfo->bFrameParallelDisabled;
    }

    // read inter/intra flag
    if (pMbInfo->bSegRefEnabled)
    {
        pMbInfo->i8SegReference =
            pFrameInfo->pSegmentData->SegData[ui8SegId].SegmentFlags.fields.SegmentReference;
        ui8IsInter = (--pMbInfo->i8SegReference != VP9_REF_FRAME_INTRA);
    }
    else
    {
        UINT uiLIsIntra  = pMbInfo->bLeftValid  && !pMbInfo->pContextLeft->DW0.ui8IsInter;
        UINT uiLIsInter  = pMbInfo->bLeftValid  &&  pMbInfo->pContextLeft->DW0.ui8IsInter;
        UINT uiAIsIntra  = pMbInfo->bAboveValid && !pMbInfo->pContextAbove->DW0.ui8IsInter;
        UINT uiAIsInter  = pMbInfo->bAboveValid &&  pMbInfo->pContextAbove->DW0.ui8IsInter;
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

    pMbInfo->pMode->DW1.ui8Flags = (ui8SkipCoeff << VP9_SKIP_FLAG) | (ui8IsInter << VP9_IS_INTER_FLAG);

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
    BOOL        bIsInterFlag;
    UCHAR       TxSizeChroma, TxSize, TxType;
    UINT        nEobMax, uiEobTotal;
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

    BlkSize = (pMbInfo->iB4Number < 4) ?
        BLOCK_8X8 : (INTEL_HOSTVLD_VP9_BLOCK_SIZE)pMbInfo->pMode->DW0.ui8BlockSize;

    iWidth4x4  = 1 << (g_Vp9BlockSizeB4Log2[BlkSize][0]);
    iHeight4x4 = 1 << (g_Vp9BlockSizeB4Log2[BlkSize][1]);

    // entropy context pointers
    pMbInfo->pAboveContext[VP9_CODED_YUV_PLANE_Y] = pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_Y] + (pMbInfo->dwMbPosX << 1);
    pMbInfo->pAboveContext[VP9_CODED_YUV_PLANE_U] = pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_U] + pMbInfo->dwMbPosX;
    pMbInfo->pAboveContext[VP9_CODED_YUV_PLANE_V] = pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_V] + pMbInfo->dwMbPosX;
    pMbInfo->pLeftContext[VP9_CODED_YUV_PLANE_Y]  = pMbInfo->EntropyContextLeft[VP9_CODED_YUV_PLANE_Y] + (pMbInfo->iMbPosInB64Y << 1);
    pMbInfo->pLeftContext[VP9_CODED_YUV_PLANE_U]  = pMbInfo->EntropyContextLeft[VP9_CODED_YUV_PLANE_U] + pMbInfo->iMbPosInB64Y;
    pMbInfo->pLeftContext[VP9_CODED_YUV_PLANE_V]  = pMbInfo->EntropyContextLeft[VP9_CODED_YUV_PLANE_V] + pMbInfo->iMbPosInB64Y;

    if ((pMbInfo->pMode->DW1.ui8Flags >> VP9_SKIP_FLAG) & 1)
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
        TxSize       = pMbInfo->pMode->DW1.ui8TxSizeLuma;
        TxSizeChroma = pMbInfo->pMode->DW0.ui8TxSizeChroma;
        bIsInterFlag = (pMbInfo->pMode->DW1.ui8Flags >> VP9_IS_INTER_FLAG) & 1;

        uiEobTotal   = 0;

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
                TxType               = pMbInfo->pMode->TxTypeLuma[0][0];
            }
            else // UV plane
            {
                Subsampling_x = Subsampling_y = 1;
                TxSize = TxSizeChroma;
                TxType = TX_DCT;
                pCoeffStatusAddrBase = pFrameState->pOutputBuffer->CoeffStatus[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer + (CoeffStatusOffset >> 2);
                pCoeffAddrBase = (PINT16)(pFrameState->pOutputBuffer->TransformCoeff[iPlane].pu16Buffer) + (CoeffOffset >> 2);
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

                    pCoeffStatusAddr = pCoeffStatusAddrBase + (pZigzagBuf[i + j * iTxCol] * (1 << (TxSize << 1)));

                    // Tx type reading per 4x4 for TX_4X4
                    if((iPlane == INTEL_HOSTVLD_VP9_YUV_PLANE_Y) && (BlkSize == BLOCK_8X8))
                    {
                        TxType = pMbInfo->pMode->TxTypeLuma[j][i];
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
                            Pt = (1 + pMbInfo->TokenCache[pNeighbor[(CoeffIdx * VP9_MAX_NEIGHBORS) + 0]] + pMbInfo->TokenCache[pNeighbor[(CoeffIdx * VP9_MAX_NEIGHBORS) + 1]]) >> 1;
                        }
                        Band = *pBandTranslate++;
                        pProb = pCoeffProbs[Band][Pt];
                        pEobBranchCount[Band][Pt] += pFrameInfo->bFrameParallelDisabled;

                        INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pProb[VP9_EOB_CONTEXT_NODE]);
                        if (BacValue >= BacSplitValue)
                        {
                            uiRange  -= uiSplit;
                            BacValue -= BacSplitValue;
                        }
                        else
                        {
                            uiRange = uiSplit;
                            pCoeffCounts[Band][Pt][VP9_DCT_EOB_MODEL_TOKEN] += pFrameInfo->bFrameParallelDisabled;
                            break;
                        }

                        do
                        {
                            INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pProb[VP9_ZERO_CONTEXT_NODE]);
                            if (BacValue >= BacSplitValue)
                            {
                                uiRange  -= uiSplit;
                                BacValue -= BacSplitValue;
                                break;
                            }
                            else
                            {
                                uiRange = uiSplit;
                            }

                            // Increase Coeff Counters for VP9_ZERO_TOKEN
                            pCoeffCounts[Band][Pt][VP9_ZERO_TOKEN] += pFrameInfo->bFrameParallelDisabled;

                            // Update Token Cache
                            pMbInfo->TokenCache[pScan[CoeffIdx]] = g_Vp9PtEnergyClass[VP9_ZERO_TOKEN];
                            ++CoeffIdx;

                            if (CoeffIdx >= (INT)nEobMax)
                            {
                                goto finish_block;
                            }
                            if (CoeffIdx)
                            {
                                Pt = (1 + pMbInfo->TokenCache[pNeighbor[(CoeffIdx * VP9_MAX_NEIGHBORS) + 0]] + pMbInfo->TokenCache[pNeighbor[(CoeffIdx * VP9_MAX_NEIGHBORS) + 1]]) >> 1;
                            }
                            Band = *pBandTranslate++;
                            pProb = pCoeffProbs[Band][Pt];
                        } while (1);

                        // ONE_CONTEXT_NODE_0_
                        INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pProb[VP9_ONE_CONTEXT_NODE]);
                        if (BacValue >= BacSplitValue)
                        {
                            uiRange  -= uiSplit;
                            BacValue -= BacSplitValue;
                        }
                        else
                        {
                            uiRange = uiSplit;
                            // Write Coeff S1 and Continue to next coeff
                            pCoeffCounts[Band][Pt][VP9_ONE_TOKEN] += pFrameInfo->bFrameParallelDisabled;
                            VP9_WRITE_COEF_CONTINUE(1, VP9_ONE_TOKEN);
                        }

                        pCoeffCounts[Band][Pt][VP9_TWO_TOKEN] += pFrameInfo->bFrameParallelDisabled;

                        pProb = g_Vp9ModelCoefProbsPareto8[pProb[VP9_PIVOT_NODE] - 1];

                        // LOW_VAL_CONTEXT_NODE_0_
                        INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pProb[VP9_LOW_VAL_CONTEXT_NODE]);
                        if (BacValue >= BacSplitValue)
                        {
                            uiRange  -= uiSplit;
                            BacValue -= BacSplitValue;
                        }
                        else
                        {
                            uiRange = uiSplit;
                            INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pProb[VP9_TWO_CONTEXT_NODE]);
                            if (BacValue >= BacSplitValue)
                            {
                                uiRange -= uiSplit;
                                BacValue -= BacSplitValue;
                            }
                            else
                            {
                                uiRange = uiSplit;
                                // Write Coeff S2 and Continue to next coeff
                                VP9_WRITE_COEF_CONTINUE(2, VP9_TWO_TOKEN);
                            }
                            INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pProb[VP9_THREE_CONTEXT_NODE]);
                            if (BacValue >= BacSplitValue)
                            {
                                uiRange -= uiSplit;
                                BacValue -= BacSplitValue;
                            }
                            else
                            {
                                uiRange = uiSplit;
                                // Write Coeff S3 and Continue to next coeff
                                VP9_WRITE_COEF_CONTINUE(3, VP9_THREE_TOKEN);
                            }

                            // Write Coeff S4 and Continue to next coeff
                            VP9_WRITE_COEF_CONTINUE(4, VP9_FOUR_TOKEN);
                        }
                        // HIGH_LOW_CONTEXT_NODE_0_
                        INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pProb[VP9_HIGH_LOW_CONTEXT_NODE]);
                        if (BacValue >= BacSplitValue)
                        {
                            uiRange -= uiSplit;
                            BacValue -= BacSplitValue;
                        }
                        else
                        {
                            uiRange = uiSplit;
                            INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(pProb[VP9_CAT_ONE_CONTEXT_NODE]);
                            if (BacValue >= BacSplitValue)
                            {
                                uiRange -= uiSplit;
                                BacValue -= BacSplitValue;
                            }
                            else
                            {
                                uiRange = uiSplit;
                                // Parse category1
                                INTEL_HOSTVLD_VP9_READ_BIT_NOUPDATE(VP9_CAT1_PROB0);
                                if (BacValue >= BacSplitValue)
                                {
                                    uiRange -= uiSplit;
                                    BacValue -= BacSplitValue;
                                    val = VP9_CAT1_MIN_VAL + 1;
                                }
                                else
                                {
                                    uiRange = uiSplit;
                                    val = VP9_CAT1_MIN_VAL;
                                }
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

finish_block:
                    uiEobTotal += CoeffIdx;

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

        // Set skip flag if block >= 8x8 and no non-zero coefficient
        pMbInfo->pMode->DW1.ui8Flags |= (UINT8)((uiEobTotal == 0) && (pMbInfo->iB4Number >= 4) && bIsInterFlag) << VP9_SKIP_FLAG;
    } //!bSkipCoeffFlag

finish:
    return eStatus;

}

VAStatus Intel_HostvldVp9_ParseBlock(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlockSize)
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_TILE_INFO     pTileInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo;
    PINTEL_HOSTVLD_VP9_MODE_INFO     pMode;
    PINTEL_HOSTVLD_VP9_NEIGHBOR      pContext;
    INT                                 i, iCount;
    VAStatus                         eStatus = VA_STATUS_SUCCESS;

    pFrameState = pTileState->pFrameState;
    pFrameInfo  = &pFrameState->FrameInfo;
    pMbInfo     = &pTileState->MbInfo;
    pTileInfo   = pMbInfo->pCurrTile;

    pMbInfo->iMbPosInB64X = pMbInfo->dwMbPosX & (VP9_B64_SIZE_IN_B8 - 1);
    pMbInfo->iMbPosInB64Y = pMbInfo->dwMbPosY & (VP9_B64_SIZE_IN_B8 - 1);

    pMbInfo->dwOffsetInB64  = (pMbInfo->iMbPosInB64Y << VP9_LOG2_B64_SIZE_IN_B8) + pMbInfo->iMbPosInB64X;
    pMbInfo->pRefFrameIndex = &pMbInfo->RefFrameIndexCache[pMbInfo->dwOffsetInB64 << 1]; // 2 reference frame indexes for each 8x8 block
    pMbInfo->pMv            = &pMbInfo->MvCache[pMbInfo->dwOffsetInB64 << 3];            // 2 MVs for each 4x4 block
    pMbInfo->iB4Number      = g_Vp9B4NumberLookup[BlockSize];
    pMode                   = pMbInfo->pModeInfoCache + pMbInfo->dwOffsetInB64;
    pMbInfo->pMode          = pMode;

    pMbInfo->pModeLeft  = (pMbInfo->iMbPosInB64X == 0) ?
        ((pMode - VP9_B64_SIZE) + (VP9_B64_SIZE_IN_B8 - 1)) : (pMode  - 1);
    pMbInfo->pModeAbove = (pMbInfo->iMbPosInB64Y == 0) ?
        ((pMode - ((pFrameInfo->dwB8ColumnsAligned - (VP9_B64_SIZE_IN_B8 - 1)) << VP9_LOG2_B64_SIZE_IN_B8))) :
        (pMode  - VP9_B64_SIZE_IN_B8);
    pMbInfo->pContextLeft  = pMbInfo->ContextLeft + pMbInfo->iMbPosInB64Y;
    pMbInfo->pContextAbove = pFrameInfo->pContextAbove + pMbInfo->dwMbPosX;

    pMbInfo->pContextLeft->DW1.ui8PartitionCtx  = pMbInfo->ui8PartitionCtxLeft;
    pMbInfo->pContextAbove->DW1.ui8PartitionCtx = pMbInfo->ui8PartitionCtxAbove;

    pMode->DW0.ui8BlockSize = BlockSize;

    // set neighbor availabilities
    pMbInfo->bAboveValid = pMbInfo->dwMbPosY > 0;
    pMbInfo->bLeftValid  = (DWORD)pMbInfo->dwMbPosX > pTileInfo->dwTileLeft;

    if (pMbInfo->bAboveValid || pMbInfo->bLeftValid)
    {
        Intel_HostvldVp9_UpdateTokenParser(pFrameInfo, pMbInfo);
    }

    // parse single block
    pFrameInfo->pfnParseFrmModeInfo(pTileState);
    Intel_HostvldVp9_ParseCoefficient(pTileState, pMbInfo->i8ZOrder);

    // Update above context
    iCount = g_Vp9BlockSizeB8[BlockSize][0];
    pContext = pMbInfo->pContextAbove;
    pContext->DW0.dwValue       = pMbInfo->pMode->DW1.dwValue;
    pContext->DW0.ui8SkipFlag   = (pMbInfo->pMode->DW1.ui8Flags >> VP9_SKIP_FLAG) & 1;
    pContext->DW0.ui8IsInter    = (pMbInfo->pMode->DW1.ui8Flags >> VP9_IS_INTER_FLAG) & 1;
    for (i = 0; i < iCount; i++)
    {
        *(pContext + i) = *pContext;
    }

    // Update left context
    iCount = g_Vp9BlockSizeB8[BlockSize][1];
    pContext = pMbInfo->pContextLeft;
    pContext->DW0.dwValue = pMbInfo->pContextAbove->DW0.dwValue;
    for (i = 0; i < iCount; i++)
    {
        *(pContext + i) = *pContext;
    }

    // Propagate luma prediction mode
    {
        PINTEL_HOSTVLD_VP9_MODE_INFO pCurrMode = pMode;
        INT x, y;
        for (y = 0; y < g_Vp9BlockSizeB8[BlockSize][1]; y++)
        {
            for (x = 0; x < g_Vp9BlockSizeB8[BlockSize][0]; x++)
            {
                pCurrMode[x].dwPredModeLuma = pMode->dwPredModeLuma;
                pCurrMode[x].DW0.dwValue    = pMode->DW0.dwValue;
            }
            pCurrMode += VP9_B64_SIZE_IN_B8;
        }
    }

finish:
    return eStatus;

}

static INTEL_HOSTVLD_VP9_PARTITION_TYPE Intel_HostvldVp9_ParsePartitionType(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine,
    DWORD                               dwB8X,
    DWORD                               dwB8Y,
    DWORD                               dwSplitBlockSize,
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlockSize)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext;
    INT                                 iContext;
    PINTEL_HOSTVLD_VP9_NEIGHBOR      pbAboveContext, pbLeftContext;
    DWORD                               dwPosition;
    INT                                 iLog2BlockSizeInB8, iBlockSizeInB8, iOffset, i;
    INT                                 iLeft, iAbove;
    INT                                 iPartitionType = PARTITION_SPLIT;

    pFrameInfo = &pTileState->pFrameState->FrameInfo;
    pContext   = pFrameInfo->pContext;

    // get partition context
    pbAboveContext      = pFrameInfo->pContextAbove + dwB8X;
    pbLeftContext       = pTileState->MbInfo.ContextLeft + (dwB8Y & (VP9_B64_SIZE_IN_B8 - 1));
    iLog2BlockSizeInB8  = BlockSize - BLOCK_8X8;
    iBlockSizeInB8      = 1 << iLog2BlockSizeInB8;
    iOffset             = VP9_LOG2_B64_SIZE_IN_B8 - iLog2BlockSizeInB8;
    iLeft = iAbove      = 0;

    for (i = 0; i < iBlockSizeInB8; i++)
    {
        iAbove |= pbAboveContext[i].DW1.ui8PartitionCtx;
        iLeft  |= pbLeftContext[i].DW1.ui8PartitionCtx;
    }

    iAbove   = ((iAbove & (1 << iOffset)) > 0);
    iLeft    = ((iLeft & (1 << iOffset)) > 0);
    iContext = ((iLeft << 1) + iAbove) + iLog2BlockSizeInB8 * VP9_PARTITION_PLOFFSET;

    // get partition type
    {
        const BYTE *pProbs = pFrameInfo->bIsIntraOnly ?
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
    VAStatus                          eStatus = VA_STATUS_SUCCESS;

    pFrameInfo      = &pTileState->pFrameState->FrameInfo;
    pMbInfo         = &pTileState->MbInfo;

    // if the block is out of picture boundary, skip it since it is not coded.
    if ((dwB8X >= pFrameInfo->dwB8Columns) || (dwB8Y >= pFrameInfo->dwB8Rows))
    {
        pMbInfo->iMbPosInB64X  = dwB8X & (VP9_B64_SIZE_IN_B8 - 1);
        pMbInfo->iMbPosInB64Y  = dwB8Y & (VP9_B64_SIZE_IN_B8 - 1);
        pMbInfo->dwOffsetInB64 = (pMbInfo->iMbPosInB64Y << VP9_LOG2_B64_SIZE_IN_B8) + pMbInfo->iMbPosInB64X;
        pMbInfo->pModeInfoCache[pMbInfo->dwOffsetInB64].DW0.ui8BlockSize = BlockSize;
        goto finish;
    }

    dwSplitBlockSize = (1 << BlockSize) >> 2;
    PartitionType = Intel_HostvldVp9_ParsePartitionType(
        pTileState,
        &pTileState->BacEngine,
        dwB8X, 
        dwB8Y, 
        dwSplitBlockSize, 
        BlockSize);

    pMbInfo->dwMbPosX    = dwB8X;
    pMbInfo->dwMbPosY    = dwB8Y;

    if (BlockSize == BLOCK_8X8)
    {
        BOOL   bIsSpilit     = PartitionType == PARTITION_SPLIT;
        INT    iAboveValue   = 0x0F - (bIsSpilit || (PartitionType == PARTITION_VERT));
        INT    iLeftValue    = 0x0F - (bIsSpilit || (PartitionType == PARTITION_HORZ));
        pMbInfo->ui8PartitionCtxLeft  = ~(iLeftValue << 3);
        pMbInfo->ui8PartitionCtxAbove = ~(iAboveValue << 3);
        Intel_HostvldVp9_ParseBlock(pTileState, g_Vp9B8SubBlock[PartitionType]);
    }
    else if (PartitionType == PARTITION_NONE)
    {
        pMbInfo->ui8PartitionCtxLeft  = ~(0x0F << (BLOCK_64X64 - BlockSize));
        pMbInfo->ui8PartitionCtxAbove = ~(0x0F << (BLOCK_64X64 - BlockSize));
        Intel_HostvldVp9_ParseBlock(pTileState, BlockSize);
    }
    else if (PartitionType == PARTITION_HORZ)
    {
        pMbInfo->ui8PartitionCtxLeft  = ~(0x0E << (BLOCK_64X64 - BlockSize));
        pMbInfo->ui8PartitionCtxAbove = ~(0x0F << (BLOCK_64X64 - BlockSize));
        Intel_HostvldVp9_ParseBlock(pTileState, (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize + 4));
        pMbInfo->dwMbPosY    += dwSplitBlockSize;
        if (pMbInfo->dwMbPosY < (INT)pFrameInfo->dwB8Rows)
        {
            Intel_HostvldVp9_ParseBlock(pTileState, (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize + 4));
        }
        else
        {
            pMbInfo->iMbPosInB64X  = pMbInfo->dwMbPosX & (VP9_B64_SIZE_IN_B8 - 1);
            pMbInfo->iMbPosInB64Y  = pMbInfo->dwMbPosY & (VP9_B64_SIZE_IN_B8 - 1);
            pMbInfo->dwOffsetInB64 = (pMbInfo->iMbPosInB64Y << VP9_LOG2_B64_SIZE_IN_B8) + pMbInfo->iMbPosInB64X;
            pMbInfo->pModeInfoCache[pMbInfo->dwOffsetInB64].DW0.ui8BlockSize = BlockSize + 4;
        }
    }
    else if (PartitionType == PARTITION_VERT)
    {
        pMbInfo->ui8PartitionCtxLeft  = ~(0x0F << (BLOCK_64X64 - BlockSize));
        pMbInfo->ui8PartitionCtxAbove = ~(0x0E << (BLOCK_64X64 - BlockSize));
        Intel_HostvldVp9_ParseBlock(pTileState, (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize + 8));
        pMbInfo->dwMbPosX    += dwSplitBlockSize;
        if (pMbInfo->dwMbPosX < (INT)pFrameInfo->dwB8Columns)
        {
            Intel_HostvldVp9_ParseBlock(pTileState, (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize + 8));
        }
        else
        {
            pMbInfo->iMbPosInB64X  = pMbInfo->dwMbPosX & (VP9_B64_SIZE_IN_B8 - 1);
            pMbInfo->iMbPosInB64Y  = pMbInfo->dwMbPosY & (VP9_B64_SIZE_IN_B8 - 1);
            pMbInfo->dwOffsetInB64 = (pMbInfo->iMbPosInB64Y << VP9_LOG2_B64_SIZE_IN_B8) + pMbInfo->iMbPosInB64X;
            pMbInfo->pModeInfoCache[pMbInfo->dwOffsetInB64].DW0.ui8BlockSize = BlockSize + 8;
        }
    }
    else if (PartitionType == PARTITION_SPLIT)
    {
        BlockSize = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize - 1);
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
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER   pOutputBuffer;
    DWORD                                 dwB8X, dwB8Y, dwTileBottomB8, dwTileRightB8, dwLineDist;
    VAStatus                            eStatus = VA_STATUS_SUCCESS;

    pFrameState                = pTileState->pFrameState;
    pFrameInfo                 = &pFrameState->FrameInfo;
    pMbInfo                    = &pTileState->MbInfo;
    pMbInfo->pCurrTile         = pTileInfo;
    pOutputBuffer              = pTileState->pFrameState->pOutputBuffer;

    if (pTileInfo->dwTileTop == 0)
    {
        pMbInfo->dwMbOffset = pTileInfo->dwTileLeft << VP9_LOG2_B64_SIZE_IN_B8;
    }
    pMbInfo->pModeInfoCache = (PINTEL_HOSTVLD_VP9_MODE_INFO)pFrameInfo->ModeInfo.pBuffer + pMbInfo->dwMbOffset;

    dwTileRightB8  = pTileInfo->dwTileLeft + pTileInfo->dwTileWidth;
    dwTileBottomB8 = pTileInfo->dwTileTop  + pTileInfo->dwTileHeight;
    dwLineDist     = (pFrameInfo->dwMbStride -
        ALIGN(pTileInfo->dwTileWidth, VP9_B64_SIZE_IN_B8)) << VP9_LOG2_B64_SIZE_IN_B8;

    for (dwB8Y = pTileInfo->dwTileTop; dwB8Y < dwTileBottomB8; dwB8Y += VP9_B64_SIZE_IN_B8)
    {
        // Reset left contexts
        CMOS_ZeroMemory(pMbInfo->ContextLeft, sizeof(pMbInfo->ContextLeft[0]) * VP9_B64_SIZE_IN_B8);
        CMOS_ZeroMemory(pMbInfo->EntropyContextLeft, sizeof(pMbInfo->EntropyContextLeft));

        // Deocde one row
        for (dwB8X = pTileInfo->dwTileLeft; dwB8X < dwTileRightB8; dwB8X += VP9_B64_SIZE_IN_B8)
        {
            Intel_HostvldVp9_ParseSuperBlock(
                pTileState, 
                dwB8X, 
                dwB8Y, 
                BLOCK_64X64);

            pMbInfo->dwMbOffset     += VP9_B64_SIZE;
            pMbInfo->pModeInfoCache += VP9_B64_SIZE;
        }

        pMbInfo->dwMbOffset     += dwLineDist;
        pMbInfo->pModeInfoCache += dwLineDist;
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

static VAStatus Intel_HostvldVp9_SetOutOfBoundSegId(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState) 
{
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER pOutputBuffer; 
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_1D_BUFFER     pSegIdBuf;
    PUINT8                              pu8SegIdBuffer;
    UINT                                i, x, y;
    DWORD                               dwOutBoundB8Columns, dwOutBoundB8Rows;
    VAStatus                          eStatus = VA_STATUS_SUCCESS;

    pOutputBuffer = pFrameState->pOutputBuffer;
    pFrameInfo    = &pFrameState->FrameInfo;
    
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
                    *(pu8SegIdBuffer + g_Vp9TxBlockIndex2ZOrderIndexMapSquare64[(y << 3) + x]) = VP9_MAX_SEGMENTS;
                }
            }
            pu8SegIdBuffer += (pFrameInfo->dwB8ColumnsAligned << 3);
        }
    }

    // SB Rows
    pu8SegIdBuffer = pSegIdBuf->pu8Buffer + pFrameInfo->dwB8ColumnsAligned * (pFrameInfo->dwB8RowsAligned - 8);
    if (dwOutBoundB8Rows)
    {
        for (i = 0; i < pFrameInfo->dwB8ColumnsAligned >> 3; i++)
        {
            for (y = 8 - dwOutBoundB8Rows; y < 8; y++)
            {
                for (x = 0; x < 8; x++)
                {
                    *(pu8SegIdBuffer + g_Vp9TxBlockIndex2ZOrderIndexMapSquare64[(y << 3) + x]) = VP9_MAX_SEGMENTS;
                }
            }
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
    CMOS_ZeroMemory(pFrameInfo->pContextAbove,
        sizeof(*pFrameInfo->pContextAbove) * pFrameInfo->dwB8ColumnsAligned);
    CMOS_ZeroMemory(pFrameInfo->EntropyContextAbove.pu8Buffer, pFrameInfo->EntropyContextAbove.dwSize);

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
        (DWORD)(pFrameInfo->SecondPartition.pu8Buffer + dwPartitionSize - pTileInfo->BitsBuffer.pu8Buffer);
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

    Intel_HostvldVp9_InitTokenParser(pTileState, pFrameInfo->TileInfo + dwTileX);

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
            eStatus = VA_STATUS_ERROR_UNKNOWN;
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

    Intel_HostvldVp9_SetOutOfBoundSegId(pFrameState);

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

    // decode tile columns
    for (dwTileX = 0; dwTileX < pFrameInfo->dwTileColumns; dwTileX++)
    {
        Intel_HostvldVp9_ParseTileColumn(pTileState, dwTileX);
    }

finish:
    return eStatus;
}
