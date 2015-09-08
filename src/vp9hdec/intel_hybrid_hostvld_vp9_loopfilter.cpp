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

#include "intel_hybrid_hostvld_vp9_parser_tables.h"
#include "intel_hybrid_hostvld_vp9_context.h"
#include "intel_hybrid_hostvld_vp9_loopfilter.h"

#define VP9_LEFT_BORDER_LUMA_MASK    0x1111111111111111
#define VP9_ABOVE_BORDER_LUMA_MASK   0x000000ff000000ff
#define VP9_LEFT_BORDER_CHROMA_MASK  0x1111
#define VP9_ABOVE_BORDER_CHROMA_MASK 0x000f

static const UINT8 g_Vp9BlockSizeMapLookup[BLOCK_SIZES] =
{
  0,    //BLOCK_4X4
  9,    //BLOCK_4X8
  5,    //BLOCK_8X4
  1,    //BLOCK_8X8
  10,   //BLOCK_8X16
  6,    //BLOCK_16X8
  2,    //BLOCK_16X16
  11,   //BLOCK_16X32
  7,    //BLOCK_32X16
  3,    //BLOCK_32X32
  12,   //BLOCK_32X64
  8,    //BLOCK_64X32
  4,    //BLOCK_64X64
};

#define VP9_FLUSH_PARTITION_NONE(pDst, Byte, DwCount)       \
do                                                          \
{                                                           \
    dwValue = (DWORD)pMode->Byte;                           \
    dwValue = (dwValue << 8) | dwValue;                     \
    dwValue = (dwValue << 16) | dwValue;                    \
    i       = 0;                                            \
    do                                                      \
    {                                                       \
        *(pDst++) = dwValue;                                \
    } while (++i < DwCount);                                \
} while (0)

#define VP9_FLUSH_PARTITION_HOR_16X16(pDst, Byte)               \
do                                                              \
{                                                               \
    dwValue = (DWORD)(pMode + VP9_B64_SIZE_IN_B8)->Byte;        \
    dwValue = (dwValue << 16) | (DWORD)pMode->Byte;             \
    *(pDst++) = (dwValue << 8) | dwValue;                       \
} while (0)

#define VP9_FLUSH_PARTITION_HOR(pDst, Byte, DwCount)            \
do                                                              \
{                                                               \
    dwValue = (DWORD)pMode->Byte;                               \
    dwValue = (dwValue << 8) | dwValue;                         \
    dwValue = (dwValue << 16) | dwValue;                        \
    i       = 0;                                                \
    do                                                          \
    {                                                           \
        *(pDst++) = dwValue;                                    \
    } while (++i < DwCount);                                    \
    dwValue = (DWORD)(pMode + (VP9_B64_SIZE_IN_B8 << (BlockSize - BLOCK_16X16)))->Byte; \
    dwValue = (dwValue << 8) | dwValue;                         \
    dwValue = (dwValue << 16) | dwValue;                        \
    i       = 0;                                                \
    do                                                          \
    {                                                           \
        *(pDst++) = dwValue;                                    \
    } while (++i < DwCount);                                    \
} while (0)

#define VP9_FLUSH_PARTITION_VER_16X16(pDst, Byte)               \
do                                                              \
{                                                               \
    dwValue = (DWORD)(pMode + 1)->Byte;                         \
    dwValue = (dwValue << 8) | (DWORD)pMode->Byte;              \
    *(pDst++) = (dwValue << 16) | dwValue;                      \
} while (0)

#define VP9_FLUSH_PARTITION_VER(pDst, Byte, DwCount)            \
do                                                              \
{                                                               \
    dwValue[0] = (DWORD)pMode->Byte;                            \
    dwValue[0] = (DWORD)(dwValue[0] << 8) | dwValue[0];         \
    dwValue[0] = (DWORD)(dwValue[0] << 16) | dwValue[0];        \
    dwValue[1] = (DWORD)(pMode + (UINT)(1 << (BlockSize - BLOCK_16X16)))->Byte; \
    dwValue[1] = (DWORD)(dwValue[1] << 8) | dwValue[1];         \
    dwValue[1] = (DWORD)(dwValue[1] << 16) | dwValue[1];        \
    for (i = 0; i < 2; i++)                                     \
    {                                                           \
        for (j = 0; j < 2; j++)                                 \
        {                                                       \
            k = 0;                                              \
            while (k++ < DwCount)                               \
            {                                                   \
                *(pDst++) = dwValue[j];                         \
            }                                                   \
        }                                                       \
    }                                                           \
} while (0)

#define VP9_FLUSH_PARTITION_SPLIT_16X16(pDst, Byte)             \
do                                                              \
{                                                               \
    dwValue = (DWORD)(pMode + VP9_B64_SIZE_IN_B8 + 1)->Byte;    \
    dwValue = (dwValue << 8) | (DWORD)(pMode + VP9_B64_SIZE_IN_B8)->Byte; \
    dwValue = (dwValue << 8) | (DWORD)(pMode + 1)->Byte;        \
    *(pDst++) = (dwValue << 8) | (DWORD)pMode->Byte;            \
} while (0)

#define VP9_FLUSH_PARTITION_NONE_QP(DwCount)                \
do                                                          \
{                                                           \
    DWORD dwQPLuma   = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y][pMode->DW0.ui8SegId];  \
    DWORD dwQPChroma = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV][pMode->DW0.ui8SegId]; \
    i = 0;                                                  \
    do                                                      \
    {                                                       \
        *(pMbInfo->pdwQPLuma++)       = dwQPLuma;        \
        *(pMbInfo->pdwQPChroma++)     = dwQPChroma;      \
        *(pMbInfo->pdwPredModeLuma++) = pMode->dwPredModeLuma; \
        *(pMbInfo->pdwTxTypeLuma++)   = pMode->dwTxTypeLuma; \
    } while (++i < DwCount);                                \
} while (0)

#define VP9_FLUSH_PARTITION_HOR_QP(DwCount)                 \
do                                                          \
{                                                           \
    PINTEL_HOSTVLD_VP9_MODE_INFO pCurrMode;              \
    DWORD dwQPLuma, dwQPChroma, dwPredModeLuma, dwTxTypeLuma; \
    UINT  uiSegId = pMode->DW0.ui8SegId;                    \
    dwQPLuma   = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y][uiSegId];  \
    dwQPChroma = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV][uiSegId]; \
    dwPredModeLuma = pMode->dwPredModeLuma;                 \
    dwTxTypeLuma   = pMode->dwTxTypeLuma;                   \
    i = 0;                                                  \
    do                                                      \
    {                                                       \
        *(pMbInfo->pdwQPLuma++)       = dwQPLuma;        \
        *(pMbInfo->pdwQPChroma++)     = dwQPChroma;      \
        *(pMbInfo->pdwPredModeLuma++) = dwPredModeLuma;  \
        *(pMbInfo->pdwTxTypeLuma++)   = dwTxTypeLuma;    \
    } while (++i < DwCount);                                \
    pCurrMode  = pMode + (VP9_B64_SIZE_IN_B8 << (BlockSize - BLOCK_16X16)); \
    uiSegId    = pCurrMode->DW0.ui8SegId;                   \
    dwQPLuma   = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y][uiSegId];  \
    dwQPChroma = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV][uiSegId]; \
    dwPredModeLuma = pCurrMode->dwPredModeLuma;             \
    dwTxTypeLuma   = pCurrMode->dwTxTypeLuma;               \
    i = 0;                                                  \
    do                                                      \
    {                                                       \
        *(pMbInfo->pdwQPLuma++)       = dwQPLuma;        \
        *(pMbInfo->pdwQPChroma++)     = dwQPChroma;      \
        *(pMbInfo->pdwPredModeLuma++) = dwPredModeLuma;  \
        *(pMbInfo->pdwTxTypeLuma++)   = dwTxTypeLuma;    \
    } while (++i < DwCount);                                \
} while (0)

#define VP9_FLUSH_PARTITION_VER_QP(DwCount)                             \
do                                                                      \
{                                                                       \
    PINTEL_HOSTVLD_VP9_MODE_INFO pCurrMode;                          \
    DWORD dwQPLuma[2], dwQPChroma[2], dwPredModeLuma[2], dwTxTypeLuma[2]; \
    UINT  uiSegId = pMode->DW0.ui8SegId;                                \
    dwQPLuma[0]   = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y][uiSegId];  \
    dwQPChroma[0] = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV][uiSegId]; \
    dwPredModeLuma[0] = pMode->dwPredModeLuma;                          \
    dwTxTypeLuma[0]   = pMode->dwTxTypeLuma;                            \
    pCurrMode     = pMode + (DWORD)(1 << (BlockSize - BLOCK_16X16));    \
    uiSegId       = pCurrMode->DW0.ui8SegId;                            \
    dwQPLuma[1]   = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y][uiSegId];  \
    dwQPChroma[1] = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV][uiSegId]; \
    dwPredModeLuma[1] = pCurrMode->dwPredModeLuma;                      \
    dwTxTypeLuma[1]   = pCurrMode->dwTxTypeLuma;                        \
    i = 0;                                                              \
    for (i = 0; i < 2; i++)                                             \
    {                                                                   \
        for (j = 0; j < 2; j++)                                         \
        {                                                               \
            k = 0;                                                      \
            while (k++ < DwCount)                                       \
            {                                                           \
                *(pMbInfo->pdwQPLuma++)   = dwQPLuma[j];             \
                *(pMbInfo->pdwQPChroma++) = dwQPChroma[j];           \
                *(pMbInfo->pdwPredModeLuma++) = dwPredModeLuma[j];   \
                *(pMbInfo->pdwTxTypeLuma++)   = dwTxTypeLuma[j];     \
            }                                                           \
        }                                                               \
    }                                                                   \
} while (0)

#define VP9_FLUSH_PARTITION_SPLIT_16X16_QP()                \
do                                                          \
{                                                           \
    PINTEL_HOSTVLD_VP9_MODE_INFO pCurrMode;              \
    *(pMbInfo->pdwQPLuma++)       = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y][pMode->DW0.ui8SegId]; \
    *(pMbInfo->pdwQPChroma++)     = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV][pMode->DW0.ui8SegId]; \
    *(pMbInfo->pdwPredModeLuma++) = pMode->dwPredModeLuma; \
    *(pMbInfo->pdwTxTypeLuma++)   = pMode->dwTxTypeLuma;  \
    pCurrMode = pMode + 1;                                  \
    *(pMbInfo->pdwQPLuma++)       = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y][pCurrMode->DW0.ui8SegId]; \
    *(pMbInfo->pdwQPChroma++)     = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV][pCurrMode->DW0.ui8SegId]; \
    *(pMbInfo->pdwPredModeLuma++) = pCurrMode->dwPredModeLuma; \
    *(pMbInfo->pdwTxTypeLuma++)   = pCurrMode->dwTxTypeLuma; \
    pCurrMode = pMode + VP9_B64_SIZE_IN_B8;                 \
    *(pMbInfo->pdwQPLuma++)       = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y][pCurrMode->DW0.ui8SegId]; \
    *(pMbInfo->pdwQPChroma++)     = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV][pCurrMode->DW0.ui8SegId]; \
    *(pMbInfo->pdwPredModeLuma++) = pCurrMode->dwPredModeLuma; \
    *(pMbInfo->pdwTxTypeLuma++)   = pCurrMode->dwTxTypeLuma; \
    pCurrMode = pMode + VP9_B64_SIZE_IN_B8 + 1;             \
    *(pMbInfo->pdwQPLuma++)       = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y][pCurrMode->DW0.ui8SegId]; \
    *(pMbInfo->pdwQPChroma++)     = (DWORD)pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV][pCurrMode->DW0.ui8SegId]; \
    *(pMbInfo->pdwPredModeLuma++) = pCurrMode->dwPredModeLuma; \
    *(pMbInfo->pdwTxTypeLuma++)   = pCurrMode->dwTxTypeLuma; \
} while (0)

VAStatus Intel_HostvldVp9_LoopfilterLevelAndMaskInSingleBlock(
    PINTEL_HOSTVLD_VP9_TILE_STATE   pTileState)
{
    VAStatus  eStatus;
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
    PINTEL_HOSTVLD_VP9_MB_INFO    pMbInfo;
    PINTEL_HOSTVLD_VP9_MODE_INFO  pMode;
    PINTEL_HOSTVLD_VP9_LOOP_FILTER_MASK pLoopFilterMaskSB;
    INT iWidth8x8, iHeight8x8, RowOffset8B;
    UCHAR FilterLevel;
    PUINT64 pMaskLeftY, pMaskAboveY, pMaskInt4x4Y;
    PUINT16 pMaskLeftUv, pMaskAboveUv, pMaskInt4x4Uv;
    DWORD nFilterLevelStride;
    PUINT8 pLoopFilterLevel;
    INT OffsetXInB8, OffsetYInB8, ShiftY, ShiftUv, YMaskOnlyFlag;

    eStatus = VA_STATUS_SUCCESS;

    pFrameState = pTileState->pFrameState;
    pMbInfo    = &pTileState->MbInfo;
    pMode      = pMbInfo->pMode;

    pLoopFilterMaskSB = &(pMbInfo->LoopFilterMaskSB);

    if ((pMode->DW1.ui8TxSizeLuma >= TX_SIZES) || (pMode->DW0.ui8TxSizeChroma >= TX_SIZES))
    {
        eStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        return eStatus;
    }

    FilterLevel     = pMode->DW1.ui8FilterLevel;
    pMaskLeftY      = &pLoopFilterMaskSB->LeftY[pMode->DW1.ui8TxSizeLuma];
    pMaskAboveY     = &pLoopFilterMaskSB->AboveY[pMode->DW1.ui8TxSizeLuma];
    pMaskInt4x4Y    = &pLoopFilterMaskSB->Int4x4Y;
    pMaskLeftUv     = &pLoopFilterMaskSB->LeftUv[pMode->DW0.ui8TxSizeChroma];
    pMaskAboveUv    = &pLoopFilterMaskSB->AboveUv[pMode->DW0.ui8TxSizeChroma];
    pMaskInt4x4Uv   = &pLoopFilterMaskSB->Int4x4Uv;

    iWidth8x8  = g_Vp9BlockSizeB8[pMode->DW0.ui8BlockSize][0];
    iHeight8x8 = g_Vp9BlockSizeB8[pMode->DW0.ui8BlockSize][1];
    
    ////////////////////////////////////////////////////////
    // Pack filter level surface per kernel required layout
    ////////////////////////////////////////////////////////
    nFilterLevelStride = pFrameState->pOutputBuffer->FilterLevel.dwPitch;
    pLoopFilterLevel =
        (PUINT8)(pFrameState->pOutputBuffer->FilterLevel.pu8Buffer) +
        pMbInfo->dwMbPosY * nFilterLevelStride + pMbInfo->dwMbPosX;
    switch (iWidth8x8)
    {
        case 1:
        {
            for (RowOffset8B = 0; RowOffset8B < iHeight8x8; RowOffset8B++)
            {
                *pLoopFilterLevel = FilterLevel;
                pLoopFilterLevel += nFilterLevelStride;
            }
            break;
        }
        case 2:
        {
            UINT16 ui16Value = FilterLevel;
            ui16Value |= (ui16Value << 8);
            for (RowOffset8B = 0; RowOffset8B < iHeight8x8; RowOffset8B++)
            {
                *(PUINT16)pLoopFilterLevel = ui16Value;
                pLoopFilterLevel += nFilterLevelStride;
            }
            break;
        }
        case 4:
        {
            UINT32 ui32Value = FilterLevel;
            ui32Value |= (ui32Value << 8);
            ui32Value |= (ui32Value << 16);
            for (RowOffset8B = 0; RowOffset8B < iHeight8x8; RowOffset8B++)
            {
                *(PUINT32)pLoopFilterLevel = ui32Value;
                pLoopFilterLevel += nFilterLevelStride;
            }
            break;
        }
        case 8:
        {
            UINT64 ui64Value = FilterLevel;
            ui64Value |= (ui64Value << 8);
            ui64Value |= (ui64Value << 16);
            ui64Value |= (ui64Value << 32);
            for (RowOffset8B = 0; RowOffset8B < iHeight8x8; RowOffset8B++)
            {
                *(PUINT64)pLoopFilterLevel = ui64Value;
                pLoopFilterLevel += nFilterLevelStride;
            }
            break;
        }
    }

    /////////////////////////
    // Calc loop filter mask
    /////////////////////////
    // Calc shift_y & shift_uv according to current block location in its SB
    // shift_y & shift_uv are the index of 8x8 block in raster scan order
    OffsetXInB8 = pMbInfo->dwMbPosX & 0x7;
    OffsetYInB8 = pMbInfo->dwMbPosY & 0x7;

    // Judge if UV mask needs to calc for this block
    YMaskOnlyFlag = (OffsetXInB8 & 0x1) || (OffsetYInB8 & 0x1);

    if(!FilterLevel)
    {
        goto finish; //No need to do loop filter
    }

    ShiftY  = (OffsetYInB8 << 3) + OffsetXInB8;
    *pMaskAboveY |= g_Vp9AbovePredictionMask[pMode->DW0.ui8BlockSize] << ShiftY;
    *pMaskLeftY  |= g_Vp9LeftPredictionMask[pMode->DW0.ui8BlockSize]  << ShiftY;
    if(!YMaskOnlyFlag)
    {
        ShiftUv = (OffsetYInB8 << 1) + (OffsetXInB8 >> 1);
        *pMaskAboveUv |= g_Vp9AbovePredictionMaskUv[pMode->DW0.ui8BlockSize] << ShiftUv;
        *pMaskLeftUv  |= g_Vp9LeftPredictionMaskUv[pMode->DW0.ui8BlockSize]  << ShiftUv;
    }    

    if (pMode->DW1.ui8Flags == ((1 << VP9_SKIP_FLAG) | (1 << VP9_IS_INTER_FLAG))) // is skipped inter block
    {
        goto finish;
    }

    // add a mask for the transform size
    *pMaskAboveY |= (g_Vp9SizeMask[pMode->DW0.ui8BlockSize] & g_Vp9Above64x64TxMask[pMode->DW1.ui8TxSizeLuma]) << ShiftY;
    *pMaskLeftY  |= (g_Vp9SizeMask[pMode->DW0.ui8BlockSize] & g_Vp9Left64x64TxMask[pMode->DW1.ui8TxSizeLuma])  << ShiftY;
    if(!YMaskOnlyFlag)
    {
        *pMaskAboveUv |= (g_Vp9SizeMaskUv[pMode->DW0.ui8BlockSize] & g_Vp9Above64x64TxMaskUv[pMode->DW0.ui8TxSizeChroma]) << ShiftUv;
        *pMaskLeftUv  |= (g_Vp9SizeMaskUv[pMode->DW0.ui8BlockSize] & g_Vp9Left64x64TxMaskUV[pMode->DW0.ui8TxSizeChroma])  << ShiftUv;
    }

    if (pMode->DW1.ui8TxSizeLuma == TX_4X4)
    {
        *pMaskInt4x4Y |= (g_Vp9SizeMask[pMode->DW0.ui8BlockSize] & 0xffffffffffffffff) << ShiftY;
    }
    if (pMode->DW0.ui8TxSizeChroma == TX_4X4 && (!YMaskOnlyFlag))
    {
        *pMaskInt4x4Uv |= (g_Vp9SizeMaskUv[pMode->DW0.ui8BlockSize] & 0xffff) << ShiftUv;
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_LoopfilterCalcMaskInSuperBlock(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    DWORD                               dwB8Row,
    DWORD                               dwB8Col,
    DWORD                               dwTileBottomB8,
    DWORD                               dwTileRightB8)
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
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

    pFrameState = pTileState->pFrameState;
    pFrameInfo = &pFrameState->FrameInfo;
    pMbInfo    = &pTileState->MbInfo;

    pMaskYVertical       = (PUINT8)(pFrameState->pOutputBuffer->VerticalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer);
    pMaskYHorizontal     = (PUINT8)(pFrameState->pOutputBuffer->HorizontalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer);
    pMaskUvVertical      = (PUINT8)(pFrameState->pOutputBuffer->VerticalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer);
    pMaskUvHorizontal    = (PUINT8)(pFrameState->pOutputBuffer->HorizontalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer);

    pLoopFilterMaskSB = &pMbInfo->LoopFilterMaskSB;

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
        const UINT64 MaskY  = ((((UINT64)1 << ActualColumns) - 1)) * 0x0101010101010101;
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
            
            uiMaskVertical = (((LeftY4x4 >> MaskPosition) & 1) + (((LeftY8x8 >> MaskPosition) & 1) << 1)
                + (((LeftY16x16 >> MaskPosition) & 1) * 3)) << 4;	//Left 4 bit
            uiMaskVertical += ((LeftY4x4 >> (MaskPosition + 1)) & 1) + ((LeftY8x8 >> MaskPosition) & 2)
                + (((LeftY16x16 >> (MaskPosition + 1)) & 1) * 3);	//Right 4 bit
            if((dwB8Col + MaskPositionX) < pFrameInfo->dwB8Columns)
            {
                uiMaskVertical += (((Int4x4Y >> MaskPosition) & 1) << (2 + 4)) + (((Int4x4Y >> MaskPosition) & 2) << 1);
            }
            
            if(MaskPositionY + dwB8Row == 0)
            {
                //Picture Top Boundary
                uiMaskHorizontal = ((Int4x4Y >> MaskPosition) & 1) << (2 + 4);	//Left 4 bit
                uiMaskHorizontal += ((Int4x4Y >> MaskPosition) & 2) << 1;	    //Right 4 bit
            }
            else
            {
                //No 4x4 Internal horizontal
                uiMaskHorizontal = (((AboveY4x4 >> MaskPosition) & 1) + (((AboveY8x8 >> MaskPosition) & 1) << 1)
                    + (((AboveY16x16 >> MaskPosition) & 1) * 3)) << 4;	//Left 4 bit
                uiMaskHorizontal += ((AboveY4x4 >> (MaskPosition + 1)) & 1) + ((AboveY8x8 >> MaskPosition) & 2)
                    + (((AboveY16x16 >> (MaskPosition + 1)) & 1) * 3);	//Right 4 bit
                if ((dwB8Row + MaskPositionY) < pFrameInfo->dwB8Rows)
                {
                    uiMaskHorizontal += (((Int4x4Y >> MaskPosition) & 1) << (2 + 4)) + (((Int4x4Y >> MaskPosition) & 2) << 1);
                }
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

            uiMaskVertical = (((LeftUv4x4 >> MaskPosition) & 1) + (((LeftUv8x8 >> MaskPosition) & 1) << 1)
                + (((LeftUv16x16 >> MaskPosition) & 1) * 3)) << 4;	//Left 4 bit
            uiMaskVertical += ((LeftUv4x4 >> (MaskPosition + 1)) & 1) + ((LeftUv8x8 >> MaskPosition) & 2)
                + (((LeftUv16x16 >> (MaskPosition + 1)) & 1) * 3);	//Right 4 bit
            if(dwB8Col + (MaskPositionX << 1) < pFrameInfo->dwB8Columns)
            {
                uiMaskVertical += (((Int4x4Uv >> MaskPosition) & 1) << (2 + 4)) + (((Int4x4Uv >> MaskPosition) & 2) << 1);
            }

            if(MaskPositionY + dwB8Row == 0)
            {
                //Picture Top Boundary
                uiMaskHorizontal = 0;
                if (pFrameInfo->dwPicHeight > 8)
                {
                    uiMaskHorizontal = ((Int4x4Uv >> MaskPosition) & 1) << (2 + 4);	//Left 4 bit
                    uiMaskHorizontal += ((Int4x4Uv >> MaskPosition) & 2) << 1;	    //Right 4 bit
                }
            }
            else
            {
                //No 4x4 Internal horizontal
                uiMaskHorizontal = (((AboveUv4x4 >> MaskPosition) & 1) + (((AboveUv8x8 >> MaskPosition) & 1) << 1)
                    + (((AboveUv16x16 >> MaskPosition) & 1) * 3)) << 4;	//Left 4 bit
                uiMaskHorizontal += ((AboveUv4x4 >> (MaskPosition + 1)) & 1) + ((AboveUv8x8 >> MaskPosition) & 2)
                    + (((AboveUv16x16 >> (MaskPosition + 1)) & 1) * 3);	//Right 4 bit
                if(dwB8Row + ((MaskPositionY + 1) << 1) <= pFrameInfo->dwB8Rows)
                {
                    uiMaskHorizontal += (((Int4x4Uv >> MaskPosition) & 1) << (2 + 4)) + (((Int4x4Uv >> MaskPosition) & 2) << 1);
                }
            }

            pMaskUvVertical[(MaskPositionY + (dwB8Row >> 1) )* nMaskUVVertStride + (((dwB8Col>>1) + MaskPositionX)>>1)] = uiMaskVertical;
            pMaskUvHorizontal[(MaskPositionY + (dwB8Row >> 1) ) * nMaskUVHorStride + (((dwB8Col>>1) + MaskPositionX)>>1)] = uiMaskHorizontal;
        }
    }

    return eStatus;
}

VAStatus Intel_HostvldVp9_LoopfilterCalcThreshold(
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
        pLoopFilterThresholdAddr[Level * nFilterThresholdStride + 1]    = (UINT8)BlockInsideLimit;          //Byte1: Lim  
        pLoopFilterThresholdAddr[Level * nFilterThresholdStride + 2]    = Level >> 4;                               //Byte2: Hev_threshold
    }

    return eStatus;
}

VAStatus Intel_HostvldVp9_LoopfilterBlock(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState)
{
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo;
    VAStatus                          eStatus = VA_STATUS_SUCCESS;

    pMbInfo    = &pTileState->MbInfo;

    //Update pMbInfo->i8ZOrder per block location
    pMbInfo->i8ZOrder   = (INT8)g_Vp9TxBlockIndex2ZOrderIndexMapSquare64[(pMbInfo->dwMbPosX % 8) + ((pMbInfo->dwMbPosY % 8) << 3)];
    Intel_HostvldVp9_LoopfilterLevelAndMaskInSingleBlock(pTileState);

    return eStatus;
}

VAStatus Intel_HostvldVp9_LoopfilterSuperBlock(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    PINTEL_HOSTVLD_VP9_MODE_INFO     pMode,
    DWORD                               dwB8X, 
    DWORD                               dwB8Y, 
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlockSize)
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     TargetBlockSize;
    DWORD                               dwSplitBlockSize;    
    DWORD                               dwBlockSizeInSurface;
    BOOL                                bUpdateBlockSize = TRUE;
    VAStatus                          eStatus = VA_STATUS_SUCCESS;

    if ((BlockSize <= BLOCK_4X4) || (BlockSize >= BLOCK_INVALID))
    {
        eStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        goto finish;
    }

    pFrameState = pTileState->pFrameState;
    pFrameInfo = &pFrameState->FrameInfo;
    pMbInfo    = &pTileState->MbInfo;

    //Read blocksize from output surface
    pMbInfo->pMode       = pMode;
    TargetBlockSize      = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)pMode->DW0.ui8BlockSize;
    dwBlockSizeInSurface = g_Vp9BlockSizeLookup[TargetBlockSize];

    // if the block is out of picture boundary, skip it since it is not coded.
    if ((dwB8X >= pFrameInfo->dwB8Columns) || (dwB8Y >= pFrameInfo->dwB8Rows))
    {
        if (BlockSize > BLOCK_8X8)
        {
            UINT uiOffset = g_Vp9B4NumberLookup[BlockSize] >> 4;
            pMbInfo->pdwTxSizeLuma       += uiOffset;
            pMbInfo->pdwTxSizeChroma     += uiOffset;
            pMbInfo->pdwFilterType       += uiOffset;
            pMbInfo->pdwPredModeChroma   += uiOffset;
            uiOffset <<= 2;
            pMbInfo->pdwQPLuma           += uiOffset;
            pMbInfo->pdwQPChroma         += uiOffset;
            pMbInfo->pdwPredModeLuma     += uiOffset;
            pMbInfo->pdwTxTypeLuma       += uiOffset;
        }
        else
        {
            bUpdateBlockSize = FALSE;
        }
        goto update_block_size;
    }

    dwSplitBlockSize = (1 << BlockSize) >> 2;

    pMbInfo->dwMbPosX    = dwB8X;
    pMbInfo->dwMbPosY    = dwB8Y;

    if (BlockSize == BLOCK_8X8)
    {
        pMbInfo->iB4Number  = g_Vp9B4NumberLookup[TargetBlockSize];
        Intel_HostvldVp9_LoopfilterBlock(pTileState);
        bUpdateBlockSize = FALSE;
    }
    else if (TargetBlockSize == BlockSize) //PARTITION_NONE
    {
        UINT i, uiCount = g_Vp9B4NumberLookup[BlockSize] >> 4;
        DWORD dwValue;

        VP9_FLUSH_PARTITION_NONE(pMbInfo->pdwTxSizeLuma, DW1.ui8TxSizeLuma, uiCount);
        VP9_FLUSH_PARTITION_NONE(pMbInfo->pdwTxSizeChroma, DW0.ui8TxSizeChroma, uiCount);
        VP9_FLUSH_PARTITION_NONE(pMbInfo->pdwFilterType, DW1.ui8FilterType, uiCount);
        VP9_FLUSH_PARTITION_NONE(pMbInfo->pdwPredModeChroma, DW0.ui8PredModeChroma, uiCount);
        uiCount <<= 2;
        VP9_FLUSH_PARTITION_NONE_QP(uiCount);

        pMbInfo->iB4Number = g_Vp9B4NumberLookup[TargetBlockSize];
        Intel_HostvldVp9_LoopfilterBlock(pTileState);
    }
    else if (TargetBlockSize == (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize + 4)) //PARTITION_HORZ
    {
        UINT i, uiCount;
        DWORD dwValue;
        if (BlockSize == BLOCK_16X16)
        {
            VP9_FLUSH_PARTITION_HOR_16X16(pMbInfo->pdwTxSizeLuma, DW1.ui8TxSizeLuma);
            VP9_FLUSH_PARTITION_HOR_16X16(pMbInfo->pdwTxSizeChroma, DW0.ui8TxSizeChroma);
            VP9_FLUSH_PARTITION_HOR_16X16(pMbInfo->pdwFilterType, DW1.ui8FilterType);
            VP9_FLUSH_PARTITION_HOR_16X16(pMbInfo->pdwPredModeChroma, DW0.ui8PredModeChroma);
        }
        else
        {
            uiCount = (g_Vp9B4NumberLookup[BlockSize] >> 4) >> 1;
            VP9_FLUSH_PARTITION_HOR(pMbInfo->pdwTxSizeLuma, DW1.ui8TxSizeLuma, uiCount);
            VP9_FLUSH_PARTITION_HOR(pMbInfo->pdwTxSizeChroma, DW0.ui8TxSizeChroma, uiCount);
            VP9_FLUSH_PARTITION_HOR(pMbInfo->pdwFilterType, DW1.ui8FilterType, uiCount);
            VP9_FLUSH_PARTITION_HOR(pMbInfo->pdwPredModeChroma, DW0.ui8PredModeChroma, uiCount);
        }
        uiCount = (g_Vp9B4NumberLookup[BlockSize] >> 4) << 1;
        VP9_FLUSH_PARTITION_HOR_QP(uiCount);

        pMbInfo->iB4Number = g_Vp9B4NumberLookup[TargetBlockSize];
        Intel_HostvldVp9_LoopfilterBlock(pTileState);
        pMbInfo->dwMbPosY    += dwSplitBlockSize;
        if (pMbInfo->dwMbPosY < pFrameInfo->dwB8Rows)
        {
            pMbInfo->pMode += dwSplitBlockSize << VP9_LOG2_B64_SIZE_IN_B8;
            Intel_HostvldVp9_LoopfilterBlock(pTileState);
        }
    }
    else if (TargetBlockSize == (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize + 8)) //PARTITION_VERT
    {
        UINT i, j, k, uiCount;
        if (BlockSize == BLOCK_16X16)
        {
            DWORD dwValue;
            VP9_FLUSH_PARTITION_VER_16X16(pMbInfo->pdwTxSizeLuma, DW1.ui8TxSizeLuma);
            VP9_FLUSH_PARTITION_VER_16X16(pMbInfo->pdwTxSizeChroma, DW0.ui8TxSizeChroma);
            VP9_FLUSH_PARTITION_VER_16X16(pMbInfo->pdwFilterType, DW1.ui8FilterType);
            VP9_FLUSH_PARTITION_VER_16X16(pMbInfo->pdwPredModeChroma, DW0.ui8PredModeChroma);
        }
        else
        {
            DWORD dwValue[2];
            uiCount = (g_Vp9B4NumberLookup[BlockSize] >> 4) >> 2;
            VP9_FLUSH_PARTITION_VER(pMbInfo->pdwTxSizeLuma, DW1.ui8TxSizeLuma, uiCount);
            VP9_FLUSH_PARTITION_VER(pMbInfo->pdwTxSizeChroma, DW0.ui8TxSizeChroma, uiCount);
            VP9_FLUSH_PARTITION_VER(pMbInfo->pdwFilterType, DW1.ui8FilterType, uiCount);
            VP9_FLUSH_PARTITION_VER(pMbInfo->pdwPredModeChroma, DW0.ui8PredModeChroma, uiCount);
        }
        uiCount = g_Vp9B4NumberLookup[BlockSize] >> 4;
        VP9_FLUSH_PARTITION_VER_QP(uiCount);

        pMbInfo->iB4Number = g_Vp9B4NumberLookup[TargetBlockSize];
        Intel_HostvldVp9_LoopfilterBlock(pTileState);
        pMbInfo->dwMbPosX    += dwSplitBlockSize;
        if (pMbInfo->dwMbPosX < pFrameInfo->dwB8Columns)
        {
            pMbInfo->pMode += dwSplitBlockSize;
            Intel_HostvldVp9_LoopfilterBlock(pTileState);
        }
    }
    else if(dwBlockSizeInSurface <= g_Vp9BlockSizeLookup[BlockSize - 1])//PARTITION_SPLIT
    {
        if (BlockSize == BLOCK_16X16)
        {
            DWORD dwValue;
            VP9_FLUSH_PARTITION_SPLIT_16X16(pMbInfo->pdwTxSizeLuma, DW1.ui8TxSizeLuma);
            VP9_FLUSH_PARTITION_SPLIT_16X16(pMbInfo->pdwTxSizeChroma, DW0.ui8TxSizeChroma);
            VP9_FLUSH_PARTITION_SPLIT_16X16(pMbInfo->pdwFilterType, DW1.ui8FilterType);
            VP9_FLUSH_PARTITION_SPLIT_16X16(pMbInfo->pdwPredModeChroma, DW0.ui8PredModeChroma);
            VP9_FLUSH_PARTITION_SPLIT_16X16_QP();

            dwBlockSizeInSurface = (DWORD)g_Vp9BlockSizeLookup[(pMode + VP9_B64_SIZE_IN_B8 + 1)->DW0.ui8BlockSize];
            dwBlockSizeInSurface = (dwBlockSizeInSurface << 8) | (DWORD)g_Vp9BlockSizeLookup[(pMode + VP9_B64_SIZE_IN_B8)->DW0.ui8BlockSize];
            dwBlockSizeInSurface = (dwBlockSizeInSurface << 8) | (DWORD)g_Vp9BlockSizeLookup[(pMode + 1)->DW0.ui8BlockSize];
            *(pMbInfo->pdwBlockSize++) = (dwBlockSizeInSurface << 8) | (DWORD)g_Vp9BlockSizeLookup[pMode->DW0.ui8BlockSize];
        }
        bUpdateBlockSize = FALSE;

        BlockSize = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize - 1);
        Intel_HostvldVp9_LoopfilterSuperBlock(
            pTileState,
            pMode,
            dwB8X, 
            dwB8Y, 
            BlockSize);
        Intel_HostvldVp9_LoopfilterSuperBlock(
            pTileState,
            pMode + (UINT)(1 << (BlockSize - 1)),
            dwB8X + dwSplitBlockSize, 
            dwB8Y, 
            BlockSize);
        pMode += (VP9_B64_SIZE_IN_B8 << (BlockSize - 1));
        Intel_HostvldVp9_LoopfilterSuperBlock(
            pTileState,
            pMode,
            dwB8X, 
            dwB8Y + dwSplitBlockSize, 
            BlockSize);
        Intel_HostvldVp9_LoopfilterSuperBlock(
            pTileState,
            pMode + (UINT)(1 << (BlockSize - 1)),
            dwB8X + dwSplitBlockSize, 
            dwB8Y + dwSplitBlockSize, 
            BlockSize);
    }
    else
    {
        eStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        printf("Invalid partition type.");
    }

update_block_size:
    if (bUpdateBlockSize)
    {
        UINT uiCount = g_Vp9B4NumberLookup[BlockSize] >> 4;
        do
        {
            dwBlockSizeInSurface = (dwBlockSizeInSurface << 8) | dwBlockSizeInSurface;
            dwBlockSizeInSurface = (dwBlockSizeInSurface << 16) | dwBlockSizeInSurface;
            *(pMbInfo->pdwBlockSize++) = dwBlockSizeInSurface;
        } while (--uiCount);
    }

finish:
    return eStatus;

}

VAStatus Intel_HostvldVp9_SetOutOfBoundValues(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState)
{
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER pOutputBuffer;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_1D_BUFFER     pLumaPredBuffer;
    PUINT32                             pu32LumaPredBuffer;
    UINT                                i, x, y;
    DWORD                               dwOutBoundB8Columns, dwOutBoundB8Rows;
    VAStatus                            eStatus = VA_STATUS_SUCCESS;

    pOutputBuffer = pFrameState->pOutputBuffer;
    pFrameInfo = &pFrameState->FrameInfo;

    // Luma prediction mode: out of boundary values must be <= 9 (TM mode). We set them to 0 here
    pLumaPredBuffer = &pOutputBuffer->PredictionMode[INTEL_HOSTVLD_VP9_YUV_PLANE_Y];
    pu32LumaPredBuffer = pLumaPredBuffer->pu32Buffer + ((pFrameInfo->dwB8ColumnsAligned - 8) << 3);

    dwOutBoundB8Columns = pFrameInfo->dwB8ColumnsAligned - pFrameInfo->dwB8Columns;
    dwOutBoundB8Rows = pFrameInfo->dwB8RowsAligned - pFrameInfo->dwB8Rows;

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
                }
            }
            pu32LumaPredBuffer += (pFrameInfo->dwB8ColumnsAligned << 3);
        }
    }

    // SB Rows
    pu32LumaPredBuffer = pLumaPredBuffer->pu32Buffer + pFrameInfo->dwB8ColumnsAligned * (pFrameInfo->dwB8RowsAligned - 8);
    if (dwOutBoundB8Rows)
    {
        for (i = 0; i < pFrameInfo->dwB8ColumnsAligned >> 3; i++)
        {
            for (y = 8 - dwOutBoundB8Rows; y < 8; y++)
            {
                for (x = 0; x < 8; x++)
                {
                    *(pu32LumaPredBuffer + g_Vp9TxBlockIndex2ZOrderIndexMapSquare64[(y << 3) + x]) = 0;
                }
            }
            pu32LumaPredBuffer += 64;
        }
    }

    return eStatus;
}

VAStatus Intel_HostvldVp9_LoopfilterOneTile(
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
    pOutputBuffer              = pFrameState->pOutputBuffer;
    pMbInfo->pCurrTile         = pTileInfo;

    if (pTileInfo->dwTileTop == 0)
    {
        pMbInfo->dwMbOffset = pTileInfo->dwTileLeft << VP9_LOG2_B64_SIZE_IN_B8;
    }
    pMbInfo->pModeInfoCache = (PINTEL_HOSTVLD_VP9_MODE_INFO)pFrameInfo->ModeInfo.pBuffer + pMbInfo->dwMbOffset;

    dwTileRightB8  = pTileInfo->dwTileLeft + pTileInfo->dwTileWidth;
    dwTileBottomB8 = pTileInfo->dwTileTop  + pTileInfo->dwTileHeight;
    dwLineDist     = (pFrameInfo->dwMbStride -
        ALIGN(pMbInfo->pCurrTile->dwTileWidth, VP9_B64_SIZE_IN_B8)) << VP9_LOG2_B64_SIZE_IN_B8;

    for (dwB8Y = pTileInfo->dwTileTop; dwB8Y < dwTileBottomB8; dwB8Y += VP9_B64_SIZE_IN_B8)
    {
        // Set host buffer pointers
        pMbInfo->pdwBlockSize        = (PDWORD)(pOutputBuffer->BlockSize.pu8Buffer + pMbInfo->dwMbOffset);
        pMbInfo->pdwTxSizeLuma       = (PDWORD)(pOutputBuffer->TransformSize[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer + pMbInfo->dwMbOffset);
        pMbInfo->pdwTxSizeChroma     = (PDWORD)(pOutputBuffer->TransformSize[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer + pMbInfo->dwMbOffset);
        pMbInfo->pdwFilterType       = (PDWORD)(pOutputBuffer->FilterType.pu8Buffer + pMbInfo->dwMbOffset);
        pMbInfo->pdwPredModeChroma   = (PDWORD)(pOutputBuffer->PredictionMode[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer + pMbInfo->dwMbOffset);
        pMbInfo->pdwQPLuma           = (PDWORD)(pOutputBuffer->QP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu32Buffer + pMbInfo->dwMbOffset);
        pMbInfo->pdwQPChroma         = (PDWORD)(pOutputBuffer->QP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu32Buffer + pMbInfo->dwMbOffset);
        pMbInfo->pdwPredModeLuma     = (PDWORD)(pOutputBuffer->PredictionMode[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu32Buffer + pMbInfo->dwMbOffset);
        pMbInfo->pdwTxTypeLuma       = (PDWORD)(pOutputBuffer->TransformType.pu32Buffer + pMbInfo->dwMbOffset);

        // Loopfilter one row
        for (dwB8X = pTileInfo->dwTileLeft; dwB8X < dwTileRightB8; dwB8X += VP9_B64_SIZE_IN_B8)
        {
            memset(&pMbInfo->LoopFilterMaskSB, 0, sizeof(INTEL_HOSTVLD_VP9_LOOP_FILTER_MASK));
            Intel_HostvldVp9_LoopfilterSuperBlock(
                pTileState,
                pMbInfo->pModeInfoCache,
                dwB8X, 
                dwB8Y, 
                BLOCK_64X64);
            Intel_HostvldVp9_LoopfilterCalcMaskInSuperBlock(
                pTileState,
                dwB8Y,
                dwB8X,
                dwTileBottomB8,
                dwTileRightB8);

            pMbInfo->dwMbOffset += VP9_B64_SIZE;
            pMbInfo->pModeInfoCache += VP9_B64_SIZE;
        }

        pMbInfo->dwMbOffset += dwLineDist;
        pMbInfo->pModeInfoCache += dwLineDist;
    }

    return eStatus;
}

VAStatus Intel_HostvldVp9_LoopfilterTileColumn(
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState,
    DWORD                               dwTileX)
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_TILE_INFO     pTileInfo;
    DWORD                               dwTileIndex;
    DWORD                               dwTileY;
    VAStatus                          eStatus     = VA_STATUS_SUCCESS;

    pFrameState     = pTileState->pFrameState;
    pFrameInfo      = &pFrameState->FrameInfo;
    pTileInfo       = pFrameInfo->TileInfo;

    // decode tiles
    for (dwTileY = 0; dwTileY < pFrameInfo->dwTileRows; dwTileY++)
    {
        dwTileIndex = dwTileY * pFrameInfo->dwTileColumns + dwTileX;
        pTileInfo   = pFrameInfo->TileInfo + dwTileIndex;

        // Loopfilter one tile
        Intel_HostvldVp9_LoopfilterOneTile(pTileState, pTileInfo);
    }

    return eStatus;
}












