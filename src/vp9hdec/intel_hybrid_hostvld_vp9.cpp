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

#include "intel_hybrid_hostvld_vp9_parser.h"
#include "intel_hybrid_hostvld_vp9_loopfilter.h"
#include "intel_hybrid_hostvld_vp9_context.h"
#include "intel_hybrid_hostvld_vp9_engine.h"


#define VP9_SafeFreeMemory(ptr)               \
    if (ptr) free(ptr);             \

#define INTEL_HOSTVLD_VP9_THREAD_NUM     1
#define INTEL_HOSTVLD_VP9_HOSTBUF_NUM    2
#define INTEL_HOSTVLD_VP9_DDIBUF_NUM     INTEL_MT_DXVA_BUF_NUM
#define INTEL_HOSTVLD_VP9_SEM_QUEUE_SIZE 128
#define INTEL_HOSTVLD_VP9_PAGE_SIZE      0x1000
#define INTEL_HOSTVLD_VP9_EARLY_DECODE_BUFFER_NUM  3

#define VP9_ALIGNED_FREE_MEMORY(pAlignedBuffer)                \
do {                                                           \
    if (pAlignedBuffer)                                        \
    {                                                          \
        free(pAlignedBuffer);                                  \
    }                                                          \
} while(0)

#define VP9_REALLOCATE_ABOVE_CTX_BUFFER(pAboveCtxBuffer, size) \
do {                                                           \
    VP9_ALIGNED_FREE_MEMORY(pAboveCtxBuffer);                                     \
    pAboveCtxBuffer = (PUINT8)memalign(INTEL_HOSTVLD_VP9_PAGE_SIZE, size); \
} while(0)

#define VP9_REALLOCATE_HOSTVLD_1D_BUFFER_UINT8(pHostvldBuffer, dwBufferSize)    \
do                                                                              \
{                                                                               \
    VP9_ALIGNED_FREE_MEMORY((pHostvldBuffer)->pu8Buffer);                       \
    (pHostvldBuffer)->dwSize = dwBufferSize;                                    \
    (pHostvldBuffer)->pu8Buffer = (PUINT8)memalign(INTEL_HOSTVLD_VP9_PAGE_SIZE, dwBufferSize); \
} while (0)

VAStatus Intel_HostvldVp9_Execute_MT (
    INTEL_HOSTVLD_VP9_HANDLE         hHostVld);

VAStatus Intel_HostvldVp9_PostLoopFilter (
    PVOID                               pVp9FrameState);

static VAStatus Intel_HostvldVp9_GetPartitions(
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo, 
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pVideoBuffer, 
    PINTEL_VP9_PIC_PARAMS            pPicParams)
{
    VAStatus  eStatus     = VA_STATUS_SUCCESS;


    if (pPicParams->UncompressedHeaderLengthInBytes >= 2)
    pFrameInfo->FirstPartition.pu8Buffer  = 
        pVideoBuffer->pbBitsData + pPicParams->UncompressedHeaderLengthInBytes;
    pFrameInfo->FirstPartition.dwSize     = pPicParams->FirstPartitionSize;

    pFrameInfo->SecondPartition.pu8Buffer = 
        pFrameInfo->FirstPartition.pu8Buffer + pFrameInfo->FirstPartition.dwSize;
    pFrameInfo->SecondPartition.dwSize    = 
        pVideoBuffer->dwBitsSize - pPicParams->UncompressedHeaderLengthInBytes - pFrameInfo->FirstPartition.dwSize;

    return eStatus;
}

static VAStatus Intel_HostvldVp9_FillIntraFrameRefFrame(
    PINTEL_HOSTVLD_VP9_1D_BUFFER  pBufferRefFrame)
{
    PINT8       pi8Buffer;
    UINT        i;
    VAStatus  eStatus     = VA_STATUS_SUCCESS;


    pi8Buffer = (PINT8)pBufferRefFrame->pu8Buffer;

    for(i = 0; i < pBufferRefFrame->dwSize; i++)
    {
        *(pi8Buffer++) = VP9_REF_FRAME_INTRA;
        *(pi8Buffer++) = VP9_REF_FRAME_INTRA;
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_Create (
    PINTEL_HOSTVLD_VP9_HANDLE        phHostVld,
    PINTEL_HOSTVLD_VP9_CALLBACKS     pCallbacks)
{
    PINTEL_HOSTVLD_VP9_STATE         pVp9HostVld = NULL;
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState;
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pVideoBuffer, pVideoBufferData;
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext;
    uint32_t                               dwThreadNumber;
    uint32_t                               dwBufferNumber;
    uint32_t                               dwDDIBufNumber;
    uint32_t                               dwBufNumEarlyDec;
    uint32_t                                i           = 0;
    uint32_t                                uiTileIndex = 0;
    VAStatus                          eStatus     = VA_STATUS_SUCCESS;


    pVp9HostVld = (PINTEL_HOSTVLD_VP9_STATE)calloc(1, sizeof(*pVp9HostVld));
    *phHostVld  = (INTEL_HOSTVLD_VP9_HANDLE)pVp9HostVld;
    

    dwThreadNumber = INTEL_HOSTVLD_VP9_THREAD_NUM;

    pVp9HostVld->pfnRenderCb        = pCallbacks->pfnHostVldRenderCb;
    pVp9HostVld->pfnSyncCb          = pCallbacks->pfnHostVldSyncCb;
    pVp9HostVld->pvStandardState    = pCallbacks->pvStandardState;
    pVp9HostVld->dwThreadNumber     = dwThreadNumber;
    pVp9HostVld->dwBufferNumber     = INTEL_HOSTVLD_VP9_HOSTBUF_NUM;
    pVp9HostVld->dwDDIBufNumber     = dwThreadNumber;
    pVp9HostVld->ui8BufNumEarlyDec  = 1; //not early decode for Sinlge thread case
    pVp9HostVld->PrevParserID       = -1;

    pthread_mutex_init(&pVp9HostVld->MutexSync, NULL);
    // Create Frame State
    pFrameState = (PINTEL_HOSTVLD_VP9_FRAME_STATE)calloc(pVp9HostVld->dwBufferNumber, sizeof(*pFrameState));
    pVp9HostVld->pFrameStateBase = pFrameState;

    for (i = 0; i < pVp9HostVld->dwBufferNumber; i++)
    {

         // Create Tile State
        pTileState = (PINTEL_HOSTVLD_VP9_TILE_STATE)calloc(dwThreadNumber, sizeof(*pTileState));
        pFrameState->pTileStateBase     = pTileState;

        // Initialize Tile States
        for (uiTileIndex = 0; uiTileIndex < dwThreadNumber; uiTileIndex++)
        {
            pTileState->pFrameState     = pFrameState;
            pTileState->dwCurrColIndex  = uiTileIndex;
            pTileState++;
        }

        // Create Context Model
        pFrameState->pVp9HostVld        = pVp9HostVld;
        pFrameState->dwLastTaskID       = -1; // Invalid ID
        pFrameState++;
    }

    pVp9HostVld->pEarlyDecBufferBase = (PINTEL_HOSTVLD_VP9_EARLY_DEC_BUFFER)calloc(
                                            pVp9HostVld->ui8BufNumEarlyDec, sizeof(*(pVp9HostVld->pEarlyDecBufferBase)));

    pVp9HostVld->pLastParserTaskID = (PUINT)malloc(pVp9HostVld->ui8BufNumEarlyDec * sizeof(UINT));
    memset(pVp9HostVld->pLastParserTaskID, -1, pVp9HostVld->ui8BufNumEarlyDec * sizeof(UINT));

    for (i = 0; i < pVp9HostVld->ui8BufNumEarlyDec; i++)
    {
        pContext                                        = &pVp9HostVld->pEarlyDecBufferBase[i].CurrContext;
        pContext->TxProbTables[TX_8X8].pui8ProbTable    = &pContext->TxProbTableSet.Tx_8X8[0][0];
        pContext->TxProbTables[TX_8X8].uiStride         = TX_8X8;
        pContext->TxProbTables[TX_16X16].pui8ProbTable  = &pContext->TxProbTableSet.Tx_16X16[0][0];
        pContext->TxProbTables[TX_16X16].uiStride       = TX_16X16;
        pContext->TxProbTables[TX_32X32].pui8ProbTable  = &pContext->TxProbTableSet.Tx_32X32[0][0];
        pContext->TxProbTables[TX_32X32].uiStride       = TX_32X32;
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_QueryBufferSize (
    INTEL_HOSTVLD_VP9_HANDLE         hHostVld,
    uint32_t                             *pdwBufferSize)
{
    PINTEL_HOSTVLD_VP9_STATE pVp9HostVld = NULL;
    VAStatus                  eStatus     = VA_STATUS_SUCCESS;

    pVp9HostVld = (PINTEL_HOSTVLD_VP9_STATE)hHostVld;

    if (pdwBufferSize)
    {
        *pdwBufferSize = pVp9HostVld->dwBufferNumber;
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_SetOutputBuffer (
    INTEL_HOSTVLD_VP9_HANDLE         hHostVld,
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER pOutputBuffer)
{
    PINTEL_HOSTVLD_VP9_STATE pVp9HostVld = NULL;
    VAStatus                  eStatus     = VA_STATUS_SUCCESS;



    pVp9HostVld = (PINTEL_HOSTVLD_VP9_STATE)hHostVld;

    pVp9HostVld->pOutputBufferBase = pOutputBuffer;

    return eStatus;
}

VAStatus Intel_HostvldVp9_Initialize (
    INTEL_HOSTVLD_VP9_HANDLE         hHostVld,
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pVideoBuffer)
{
    PINTEL_HOSTVLD_VP9_STATE         pVp9HostVld = NULL;
    VAStatus                          eStatus     = VA_STATUS_SUCCESS;


    pVp9HostVld = (PINTEL_HOSTVLD_VP9_STATE)hHostVld;

    {
        PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState = NULL;
        DWORD                               dwCurrIndex;

        dwCurrIndex = (pVp9HostVld->dwCurrIndex + 1) % pVp9HostVld->dwBufferNumber;
        pFrameState = pVp9HostVld->pFrameStateBase + dwCurrIndex;

        pFrameState->dwPrevIndex    = pVp9HostVld->dwCurrIndex;
        pFrameState->dwCurrIndex    = dwCurrIndex;
        pFrameState->pOutputBuffer  = pVp9HostVld->pOutputBufferBase + dwCurrIndex;
        pFrameState->pVideoBuffer   = pVideoBuffer;
        pFrameState->LastFrameType  = pVp9HostVld->LastFrameType;
        pFrameState->FrameInfo.pContext = &pVp9HostVld->pEarlyDecBufferBase[0].CurrContext; //for ST, there is only 1 early dec buffer
        pFrameState->pLastSegIdBuf      = &pVp9HostVld->pEarlyDecBufferBase[0].LastSegId;

        pVp9HostVld->dwCurrIndex    = dwCurrIndex;
    }

    return eStatus;
}

VAStatus Intel_HostvldVp9_PreParser (PVOID pVp9FrameState)
{
    PINTEL_HOSTVLD_VP9_STATE         pVp9HostVld     = NULL;
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState     = NULL;
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pPrevFrameState = NULL;
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState      = NULL;
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pVideoBuffer;
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER pOutputBuffer;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_VP9_PIC_PARAMS            pPicParams;
    PINTEL_VP9_SEGMENT_PARAMS        pSegmentData;
    INT                                 iMarkerBit;
    DWORD                               dwNumAboveCtx, dwSize, i;
    DWORD                               dwPrevPicWidth, dwPrevPicHeight;
    BOOL                                bPrevShowFrame;
    BOOL                                bResetLastSegId = FALSE;
    VAStatus                          eStatus     = VA_STATUS_SUCCESS;



    pFrameState     = (PINTEL_HOSTVLD_VP9_FRAME_STATE)pVp9FrameState;
    pVp9HostVld     = pFrameState->pVp9HostVld;
    pVideoBuffer    = pFrameState->pVideoBuffer;
    pOutputBuffer   = pFrameState->pOutputBuffer;
    pFrameInfo      = &pFrameState->FrameInfo;

    pPrevFrameState = pFrameState->pVp9HostVld->pFrameStateBase + pFrameState->dwPrevIndex;
    dwPrevPicWidth  = pPrevFrameState->FrameInfo.dwPicWidthCropped;
    dwPrevPicHeight = pPrevFrameState->FrameInfo.dwPicHeightCropped;
    bPrevShowFrame  = pPrevFrameState->FrameInfo.bShowFrame;

    pPicParams                      = pVideoBuffer->pVp9PicParams;
    pSegmentData                    = pVideoBuffer->pVp9SegmentData;
    pFrameInfo->pPicParams          = pPicParams;
    pFrameInfo->pSegmentData        = pVideoBuffer->pVp9SegmentData;
    
    pFrameInfo->ui8SegEnabled       = pPicParams->PicFlags.fields.segmentation_enabled;
    pFrameInfo->ui8SegUpdMap        = pPicParams->PicFlags.fields.segmentation_update_map;
    pFrameInfo->ui8TemporalUpd      = pPicParams->PicFlags.fields.segmentation_temporal_update;
    
    pFrameInfo->LastFrameType       = pFrameState->LastFrameType;
    pFrameInfo->dwPicWidthCropped   = pPicParams->FrameWidthMinus1 + 1;
    pFrameInfo->dwPicHeightCropped  = pPicParams->FrameHeightMinus1 + 1;
    pFrameInfo->dwPicWidth          = ALIGN(pPicParams->FrameWidthMinus1 + 1, 8);
    pFrameInfo->dwPicHeight         = ALIGN(pPicParams->FrameHeightMinus1 + 1, 8);
    pFrameInfo->dwB8Columns         = pFrameInfo->dwPicWidth >> VP9_LOG2_B8_SIZE;
    pFrameInfo->dwB8Rows            = pFrameInfo->dwPicHeight >> VP9_LOG2_B8_SIZE;
    pFrameInfo->dwB8ColumnsAligned  = ALIGN(pFrameInfo->dwB8Columns, VP9_B64_SIZE_IN_B8);
    pFrameInfo->dwB8RowsAligned     = ALIGN(pFrameInfo->dwB8Rows, VP9_B64_SIZE_IN_B8);
    pFrameInfo->dwPicWidthAligned   = pFrameInfo->dwB8ColumnsAligned << VP9_LOG2_B8_SIZE;
    pFrameInfo->dwLog2TileRows      = pPicParams->log2_tile_rows;
    pFrameInfo->dwLog2TileColumns   = pPicParams->log2_tile_columns;
    pFrameInfo->dwTileRows          = 1 << pPicParams->log2_tile_rows;
    pFrameInfo->dwTileColumns       = 1 << pPicParams->log2_tile_columns;
    pFrameInfo->dwMbStride          = pFrameInfo->dwB8ColumnsAligned;
    pFrameInfo->bLossLess           = pPicParams->PicFlags.fields.LosslessFlag;
    pFrameInfo->bShowFrame          = pPicParams->PicFlags.fields.show_frame;
    pFrameInfo->bIsIntraOnly        = 
        (pPicParams->PicFlags.fields.frame_type == KEY_FRAME) || 
        (pPicParams->PicFlags.fields.intra_only);
    pFrameInfo->bFrameParallelDisabled   = 
        !pPicParams->PicFlags.fields.frame_parallel_decoding_mode;
    pFrameInfo->bErrorResilientMode     = 
        pPicParams->PicFlags.fields.error_resilient_mode;
    pFrameInfo->uiFrameContextIndex     = 
        pPicParams->PicFlags.fields.frame_context_idx;
    pFrameInfo->uiResetFrameContext     = 
        pPicParams->PicFlags.fields.reset_frame_context;
    pFrameInfo->bIsKeyFrame         = pPicParams->PicFlags.fields.frame_type == KEY_FRAME;
    pFrameInfo->eInterpolationType      = 
        (INTEL_HOSTVLD_VP9_INTERPOLATION_TYPE)pPicParams->PicFlags.fields.mcomp_filter_type;
    
    //Inter
    pFrameInfo->bIsSwitchableInterpolation  = 
        pPicParams->PicFlags.fields.mcomp_filter_type == VP9_INTERP_SWITCHABLE;
    pFrameInfo->bAllowHighPrecisionMv                   = pPicParams->PicFlags.fields.allow_high_precision_mv;
    pFrameInfo->RefFrameSignBias[VP9_REF_FRAME_LAST]    = pPicParams->PicFlags.fields.LastRefSignBias;
    pFrameInfo->RefFrameSignBias[VP9_REF_FRAME_GOLDEN]  = pPicParams->PicFlags.fields.GoldenRefSignBias;
    pFrameInfo->RefFrameSignBias[VP9_REF_FRAME_ALTREF]  = pPicParams->PicFlags.fields.AltRefSignBias;
    
    for (i = 0; i < VP9_MAX_SEGMENTS; i++)
    {
        // pack segment QP
        pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y][i]  =
            (((UINT32)pSegmentData->SegData[i].LumaACQuantScale) << 16) | pSegmentData->SegData[i].LumaDCQuantScale;
        pFrameInfo->SegQP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV][i] =
            (((UINT32)pSegmentData->SegData[i].ChromaACQuantScale) << 16) | pSegmentData->SegData[i].ChromaDCQuantScale;
    }
    
    Intel_HostvldVp9_GetPartitions(pFrameInfo, pVideoBuffer, pPicParams);
    
    // Initialize BAC engine
    iMarkerBit = Intel_HostvldVp9_BacEngineInit(
        &pFrameState->BacEngine, 
        pFrameInfo->FirstPartition.pu8Buffer, 
        pFrameInfo->FirstPartition.dwSize);
    
    if (0 != iMarkerBit)
    {
        eStatus = VA_STATUS_ERROR_OPERATION_FAILED;
        goto finish;
    }
    
    pFrameInfo->bResetContext = FALSE;
    if (pFrameInfo->bIsIntraOnly || pFrameInfo->bErrorResilientMode)
    {
        // reset context
        Intel_HostvldVp9_ResetContext(pVp9HostVld->ContextTable, pFrameInfo);
    }
    
    if (!pFrameInfo->bResetContext)
    {
        // If we didn't reset the context, initialize current frame context by copying from context table
        Intel_HostvldVp9_GetCurrFrameContext(
            pVp9HostVld->ContextTable,
            pFrameInfo);
    }
    
        Intel_HostvldVp9_SetupSegmentationProbs(
            pFrameInfo->pContext,
            pPicParams->SegTreeProbs,
            pPicParams->SegPredProbs);
    
    pFrameState->dwTileStatesInUse = MIN(pFrameInfo->dwTileColumns, pVp9HostVld->dwThreadNumber);
    pTileState                     = pFrameState->pTileStateBase;
    for (i = 0; i < pFrameState->dwTileStatesInUse; i++)
    {
        memset(&(pTileState->Count), 0, sizeof(pTileState->Count));
        pTileState++;
    }
    
    // Only reallocate above context buffers when picture width becomes bigger           //Need To Check
    dwNumAboveCtx = pFrameInfo->dwPicWidthAligned >> VP9_LOG2_B8_SIZE;
    if (dwNumAboveCtx > pFrameInfo->dwNumAboveCtx)
    {
        pFrameInfo->dwNumAboveCtx = dwNumAboveCtx;
    
        // allocate above context
        dwSize = dwNumAboveCtx * sizeof(*pFrameInfo->pContextAbove);
        VP9_ALIGNED_FREE_MEMORY(pFrameInfo->pContextAbove);
        pFrameInfo->pContextAbove =
            (PINTEL_HOSTVLD_VP9_NEIGHBOR)memalign(INTEL_HOSTVLD_VP9_PAGE_SIZE, dwSize);

        dwNumAboveCtx <<= 1;
        // Entropy context, per 4x4 block. Allocate once for all the planes
        dwSize = dwNumAboveCtx * sizeof(*pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_Y]) * 2;
        VP9_REALLOCATE_ABOVE_CTX_BUFFER(pFrameInfo->EntropyContextAbove.pu8Buffer, dwSize);
        pFrameInfo->EntropyContextAbove.dwSize = dwSize;
        pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_Y] =
            pFrameInfo->EntropyContextAbove.pu8Buffer;
        pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_U] =
            pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_Y] + (dwSize >> 1);
        pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_V] =
            pFrameInfo->pEntropyContextAbove[VP9_CODED_YUV_PLANE_U] + (dwSize >> 2); // we only support 4:2:0 so far.
    }

    // Reallocate mode info buffer if changed to bigger resolution
    dwSize = pFrameInfo->dwB8ColumnsAligned * pFrameInfo->dwB8RowsAligned;
    if (dwSize > pFrameInfo->ModeInfo.dwSize)
    {
        VP9_ALIGNED_FREE_MEMORY(pFrameInfo->ModeInfo.pBuffer);
        pFrameInfo->ModeInfo.pBuffer = memalign(INTEL_HOSTVLD_VP9_PAGE_SIZE,
            dwSize * sizeof(INTEL_HOSTVLD_VP9_MODE_INFO));
        pFrameInfo->ModeInfo.dwSize  = dwSize;
    }

    // Zero last segment id buffer if resolution changed
    if (dwSize > pFrameState->pLastSegIdBuf->dwSize)
    {
        // Per 8x8 block, UINT8
        VP9_REALLOCATE_HOSTVLD_1D_BUFFER_UINT8(pFrameState->pLastSegIdBuf, dwSize);
        bResetLastSegId |= TRUE;
    }
    if ((pFrameInfo->dwPicWidthCropped  != dwPrevPicWidth) || 
        (pFrameInfo->dwPicHeightCropped != dwPrevPicHeight) ||
         pFrameInfo->bIsIntraOnly ||
         pFrameInfo->bErrorResilientMode)
    {
        bResetLastSegId |= TRUE;
    }

    if (bResetLastSegId)
    {
        memset(pFrameState->pLastSegIdBuf->pu8Buffer, 0, pFrameState->pLastSegIdBuf->dwSize);
    }
    
    if (pVp9HostVld->pfnSyncCb)
    {
        pVp9HostVld->pfnSyncCb(
            pVp9HostVld->pvStandardState, 
            pVideoBuffer, 
            pFrameState->dwCurrIndex, 
            pFrameState->dwPrevIndex);
    }
    
    pFrameInfo->bHasPrevFrame = 
        (pFrameInfo->dwPicWidthCropped == dwPrevPicWidth)   &&
        (pFrameInfo->dwPicHeightCropped == dwPrevPicHeight) &&
        !pFrameInfo->bErrorResilientMode                    &&
        !pFrameInfo->bIsIntraOnly                           &&
        bPrevShowFrame;
    
    if (pFrameInfo->bIsIntraOnly)
    {
        Intel_HostvldVp9_FillIntraFrameRefFrame(&pOutputBuffer->ReferenceFrame);
    }
    
    Intel_HostvldVp9_ParseCompressedHeader(pFrameState);

    Intel_HostvldVp9_PreParseTiles(pFrameState);

finish:
    return eStatus;
}


VAStatus Intel_HostvldVp9_PostParser (PVOID pVp9FrameState)
{
    PINTEL_HOSTVLD_VP9_STATE         pVp9HostVld = NULL;
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState = NULL;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo  = NULL;
    VAStatus                          eStatus     = VA_STATUS_SUCCESS;


    pFrameState = (PINTEL_HOSTVLD_VP9_FRAME_STATE)pVp9FrameState;
    pVp9HostVld = pFrameState->pVp9HostVld;
    pFrameInfo  = &pFrameState->FrameInfo;

    Intel_HostvldVp9_PostParseTiles(pFrameState);

    if (pFrameInfo->bIsIntraOnly || pFrameInfo->bErrorResilientMode)
    {
        Intel_HostvldVp9_UpdateContextTables(pVp9HostVld->ContextTable, pFrameInfo);
    }

    Intel_HostvldVp9_AdaptProbabilities(pFrameState);

    Intel_HostvldVp9_RefreshFrameContext(pVp9HostVld->ContextTable, pFrameInfo);

    pFrameState->ReferenceFrame.pu16Buffer = pFrameState->pOutputBuffer->ReferenceFrame.pu16Buffer;
    pFrameState->ReferenceFrame.dwSize     = pFrameState->pOutputBuffer->ReferenceFrame.dwSize;

    if(pFrameState->FrameInfo.dwTileColumns > 1)
    {
        Intel_HostvldVp9_PostLoopFilter(pVp9FrameState);
    }

    return eStatus;
}

VAStatus Intel_HostvldVp9_TileColumnParser (PVOID pVp9TileState)
{
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState  = NULL;
    DWORD                               dwTileColumns, dwCurrColIndex, dwTileStateNumber;
    VAStatus                          eStatus     = VA_STATUS_SUCCESS;


    pTileState     = (PINTEL_HOSTVLD_VP9_TILE_STATE)pVp9TileState;
    dwTileColumns     = pTileState->pFrameState->FrameInfo.dwTileColumns;
    dwCurrColIndex    = pTileState->dwCurrColIndex;
    dwTileStateNumber = pTileState->pFrameState->pVp9HostVld->dwThreadNumber;

    while(dwCurrColIndex < dwTileColumns)
    {
        Intel_HostvldVp9_ParseTileColumn(pTileState, dwCurrColIndex);
        dwCurrColIndex  += dwTileStateNumber;
    }

    return eStatus;
}

VAStatus Intel_HostvldVp9_Parser (PVOID pVp9FrameState)
{
    VAStatus eStatus = VA_STATUS_SUCCESS;


    eStatus = Intel_HostvldVp9_PreParser(pVp9FrameState);

    eStatus = Intel_HostvldVp9_ParseTiles((PINTEL_HOSTVLD_VP9_FRAME_STATE)pVp9FrameState);

    if (((PINTEL_HOSTVLD_VP9_FRAME_STATE)pVp9FrameState)->pVp9HostVld->dwThreadNumber == 1)
    {
       eStatus = Intel_HostvldVp9_PostParser(pVp9FrameState);
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_LoopFilterTiles (PVOID pVp9TileState)
{
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState  = NULL;
    DWORD                               dwTileColumns, dwCurrColIndex, dwTileStateNumber;
    VAStatus                            eStatus     = VA_STATUS_SUCCESS;

    pTileState        = (PINTEL_HOSTVLD_VP9_TILE_STATE)pVp9TileState;
    dwTileColumns     = pTileState->pFrameState->FrameInfo.dwTileColumns;
    dwCurrColIndex    = pTileState->dwCurrColIndex;
    dwTileStateNumber = pTileState->pFrameState->pVp9HostVld->dwThreadNumber;

    while(dwCurrColIndex < dwTileColumns)
    {
        Intel_HostvldVp9_LoopfilterTileColumn(pTileState, dwCurrColIndex);
        dwCurrColIndex  += dwTileStateNumber;
    }

    return eStatus;
}

VAStatus Intel_HostvldVp9_PostLoopFilter (PVOID pVp9FrameState)
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState = NULL;
    VAStatus                          eStatus     = VA_STATUS_SUCCESS;

    pFrameState = (PINTEL_HOSTVLD_VP9_FRAME_STATE)pVp9FrameState;

    // Calc Loop filter threshold
    Intel_HostvldVp9_LoopfilterCalcThreshold(pFrameState);

    Intel_HostvldVp9_SetOutOfBoundValues(pFrameState);

    return eStatus;
}

VAStatus Intel_HostvldVp9_LoopfilterFrame(PVOID pVp9FrameState)
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState = NULL;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo  = NULL;
    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileState  = NULL;
    DWORD                               dwTileIndex,dwTileX;
    VAStatus                            eStatus     = VA_STATUS_SUCCESS;

    pFrameState = (PINTEL_HOSTVLD_VP9_FRAME_STATE)pVp9FrameState;

    pFrameInfo              = &pFrameState->FrameInfo;
    pTileState              = pFrameState->pTileStateBase;
    pTileState->pFrameState = pFrameState;

    // decode tiles
    for (dwTileX = 0; dwTileX < pFrameInfo->dwTileColumns; dwTileX++)
    {
        Intel_HostvldVp9_LoopfilterTileColumn(pTileState,dwTileX);
    }

    Intel_HostvldVp9_PostLoopFilter(pFrameState);

    return eStatus;
}

VAStatus Intel_HostvldVp9_Render (PVOID pVp9FrameState)
{
    PINTEL_HOSTVLD_VP9_STATE         pVp9HostVld = NULL;
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState = NULL;
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo  = NULL;
    VASurfaceID                      ucLastRefIdx, ucGoldenRefIdx, ucAltRefIdx;
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pVideoBuffer= NULL;
    PINTEL_VP9_PIC_PARAMS            pPicParams  = NULL;
    VAStatus                          eStatus     = VA_STATUS_SUCCESS;


    pFrameState     = (PINTEL_HOSTVLD_VP9_FRAME_STATE)pVp9FrameState;
    pVp9HostVld     = pFrameState->pVp9HostVld;
    pFrameInfo      = &pFrameState->FrameInfo;
    pVideoBuffer    = pFrameState->pVideoBuffer;
    pPicParams      = pVideoBuffer->pVp9PicParams;

    ucLastRefIdx    = pPicParams->RefFrameList[pPicParams->PicFlags.fields.LastRefIdx];
    ucGoldenRefIdx  = pPicParams->RefFrameList[pPicParams->PicFlags.fields.GoldenRefIdx];
    ucAltRefIdx     = pPicParams->RefFrameList[pPicParams->PicFlags.fields.AltRefIdx];
   
    if (pVp9HostVld->pfnRenderCb)
    {
        pVp9HostVld->pfnRenderCb(
            pVp9HostVld->pvStandardState, 
            pFrameState->dwCurrIndex, 
            pFrameState->dwPrevIndex);
    }

    return eStatus;
}

VAStatus Intel_HostvldVp9_Execute (
    INTEL_HOSTVLD_VP9_HANDLE         hHostVld)
{
    PINTEL_HOSTVLD_VP9_STATE         pVp9HostVld = NULL;
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState = NULL;
    VAStatus                          eStatus     = VA_STATUS_SUCCESS;
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER pVp9VideoBuffer = NULL;


    pVp9HostVld     = (PINTEL_HOSTVLD_VP9_STATE)hHostVld;

    pFrameState = pVp9HostVld->pFrameStateBase + pVp9HostVld->dwCurrIndex;

    eStatus = Intel_HostvldVp9_Parser(pFrameState);
    if (eStatus != VA_STATUS_SUCCESS)
       goto finish;

    eStatus = Intel_HostvldVp9_LoopfilterFrame(pFrameState);
    if (eStatus != VA_STATUS_SUCCESS)
       goto finish;

    pVp9VideoBuffer = pFrameState->pVideoBuffer;

    if (pVp9VideoBuffer->slice_data_bo) {
        dri_bo_unmap(pVp9VideoBuffer->slice_data_bo);
	pVp9VideoBuffer->slice_data_bo = NULL;
    }

    eStatus = Intel_HostvldVp9_Render(pFrameState);
    if (eStatus != VA_STATUS_SUCCESS)
       goto finish;

    pVp9HostVld->LastFrameType = (INTEL_HOSTVLD_VP9_FRAME_TYPE)(pFrameState->pVideoBuffer->pVp9PicParams->PicFlags.fields.frame_type);

finish:
    return eStatus;
}


VAStatus Intel_HostvldVp9_InitFrameState (
    PVOID                      pInitData,
    PVOID                      pData)
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
    PINTEL_HOSTVLD_VP9_TASK_USERDATA pTaskUserData;
    VAStatus                          eStatus     = VA_STATUS_SUCCESS;


    pFrameState     = (PINTEL_HOSTVLD_VP9_FRAME_STATE)pData;
    pTaskUserData   = (PINTEL_HOSTVLD_VP9_TASK_USERDATA)pInitData;

    pFrameState->pVideoBuffer  = pTaskUserData->pVideoBuffer;
    pFrameState->pOutputBuffer = pTaskUserData->pOutputBuffer;
    pFrameState->pRenderTarget = pTaskUserData->pVideoBuffer->pRenderTarget;
    pFrameState->dwCurrIndex   = pTaskUserData->dwCurrIndex;
    pFrameState->dwPrevIndex   = pTaskUserData->dwPrevIndex;
    pFrameState->LastFrameType      = pTaskUserData->LastFrameType;
    pFrameState->FrameInfo.pContext = pTaskUserData->pCurrContext;
    pFrameState->pLastSegIdBuf      = pTaskUserData->pLastSegIdBuf;

    return eStatus;
}


VAStatus Intel_HostvldVp9_Destroy (
    INTEL_HOSTVLD_VP9_HANDLE         hHostVld)
{
    PINTEL_HOSTVLD_VP9_STATE pVp9HostVld = NULL;
    VAStatus                  eStatus     = VA_STATUS_SUCCESS;
    unsigned int                        i           = 0;



    pVp9HostVld = (PINTEL_HOSTVLD_VP9_STATE)hHostVld;

    if (pVp9HostVld)
    {
        PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pVideoBuffer;
        PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
        PINTEL_HOSTVLD_VP9_EARLY_DEC_BUFFER pEarlyDecBufferBase;

        pFrameState = pVp9HostVld->pFrameStateBase;
        if (pFrameState)
        {
            for(i = 0; i < pVp9HostVld->dwBufferNumber; i++)
            {
                if (pFrameState)
                {
                    VP9_ALIGNED_FREE_MEMORY(pFrameState->FrameInfo.pContextAbove);
                    VP9_ALIGNED_FREE_MEMORY(pFrameState->FrameInfo.EntropyContextAbove.pu8Buffer);
                    VP9_ALIGNED_FREE_MEMORY(pFrameState->FrameInfo.ModeInfo.pBuffer);
                    VP9_SafeFreeMemory(pFrameState->pTileStateBase);
                }
                pFrameState++;
            }
            
            VP9_SafeFreeMemory(pVp9HostVld->pFrameStateBase);
        }
        pEarlyDecBufferBase = pVp9HostVld->pEarlyDecBufferBase;
        for (i = 0; i< pVp9HostVld->ui8BufNumEarlyDec; i++)
        {
            VP9_ALIGNED_FREE_MEMORY(pEarlyDecBufferBase->LastSegId.pu8Buffer);

            pEarlyDecBufferBase++;
        }
        VP9_SafeFreeMemory(pVp9HostVld->pEarlyDecBufferBase);
        VP9_SafeFreeMemory(pVp9HostVld->pLastParserTaskID);

        pthread_mutex_destroy(&pVp9HostVld->MutexSync);

        free(pVp9HostVld);
    }

finish:
    return eStatus;
}
