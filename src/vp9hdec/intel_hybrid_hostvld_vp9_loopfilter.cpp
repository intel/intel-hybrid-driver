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

#ifdef SEPERATE_LOOPFILTER_ENABLE
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

VAStatus Intel_HostvldVp9_LoopfilterLevelAndMaskInSingleBlock(
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
    INT OffsetXInB8, OffsetYInB8, ShiftY, ShiftUv, YMaskOnlyFlag, i, Index, i4ZOrder, iB4X, iB4Y;
    INTEL_HOSTVLD_VP9_BLOCK_SIZE  BlkSize;
    UCHAR bSkipCoeffFlag;  
    UCHAR TxSize;
    UCHAR TxSizeChroma;
    CHAR  RefFrame;
    UCHAR Seg;
    UCHAR PredModeLuma;

    eStatus = VA_STATUS_SUCCESS;

    pFrameInfo = &pFrameState->FrameInfo;
    pMbInfo    = &pFrameState->pTileStateBase->MbInfo;

    // Read Skip Coeff Flag, TX size and IsInterFlag from buffers
    BlkSize         = pMbInfo->BlockSize;
    bSkipCoeffFlag  = *(pFrameState->pOutputBuffer->SkipFlag.pu8Buffer + pMbInfo->dwMbOffset + pMbInfo->i8ZOrder);
    TxSize          = *(pFrameState->pOutputBuffer->TransformSize[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer + pMbInfo->dwMbOffset + pMbInfo->i8ZOrder);
    TxSizeChroma    = *(pFrameState->pOutputBuffer->TransformSize[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer + pMbInfo->dwMbOffset + pMbInfo->i8ZOrder);
    RefFrame        = *(pFrameState->ReferenceFrame.pu8Buffer + ((pMbInfo->dwMbOffset + pMbInfo->i8ZOrder) << 1));
    Seg             = *(pFrameState->pOutputBuffer->SegmentIndex.pu8Buffer + pMbInfo->dwMbOffset + pMbInfo->i8ZOrder);
    iB4X            = ((pMbInfo->dwMbPosX % VP9_B64_SIZE_IN_B8) << 1) + 1;//Note that pred mode is stored in the last address of 4x4 granularity z-order buffer
    iB4Y            = ((pMbInfo->dwMbPosY % VP9_B64_SIZE_IN_B8) << 1) + 1;
    i4ZOrder        = g_Vp9TxBlockIndex2ZOrderIndexMapSquare256[iB4X + (iB4Y << VP9_LOG2_B64_SIZE_IN_B4)];
    PredModeLuma    = *(pFrameState->pOutputBuffer->PredictionMode[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer + (pMbInfo->dwMbOffset << 2) + i4ZOrder);

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

VAStatus Intel_HostvldVp9_LoopfilterCalcMaskInSuperBlock(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState, 
    DWORD                               dwB8Row,
    DWORD                               dwB8Col,
    DWORD                               dwTileBottomB8,
    DWORD                               dwTileRightB8)
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
    pMbInfo    = &pFrameState->pTileStateBase->MbInfo;

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

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_LoopfilterBlock(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_TILE_INFO     pTileInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo;
    VAStatus                          eStatus = VA_STATUS_SUCCESS;


    pFrameInfo  = &pFrameState->FrameInfo;
    pMbInfo    = &pFrameState->pTileStateBase->MbInfo;
    pTileInfo   = pMbInfo->pCurrTile;

    //Update pMbInfo->i8ZOrder per block location
    pMbInfo->i8ZOrder   = (INT8)g_Vp9TxBlockIndex2ZOrderIndexMapSquare64[(pMbInfo->dwMbPosX % 8) + ((pMbInfo->dwMbPosY % 8) << 3)];
    Intel_HostvldVp9_LoopfilterLevelAndMaskInSingleBlock(pFrameState);

finish:
    return eStatus;

}

VAStatus Intel_HostvldVp9_LoopfilterSuperBlock(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState,
    DWORD                               dwB8X, 
    DWORD                               dwB8Y, 
    INTEL_HOSTVLD_VP9_BLOCK_SIZE     BlockSize)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO       pMbInfo;
    DWORD                               dwSplitBlockSize;    
    DWORD                               dwBlockSizeInSurface;
    PUINT8                              pBlockSize;
    VAStatus                          eStatus = VA_STATUS_SUCCESS;


    if ((BlockSize <= BLOCK_4X4) || (BlockSize >= BLOCK_INVALID))
    {
        eStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        goto finish;
    }

    pFrameInfo = &pFrameState->FrameInfo;
    pMbInfo    = &pFrameState->pTileStateBase->MbInfo;

    // if the block is out of picture boundary, skip it since it is not coded.
    if ((dwB8X >= pFrameInfo->dwB8Columns) || (dwB8Y >= pFrameInfo->dwB8Rows))
    {
        goto finish;
    }

    dwSplitBlockSize = (1 << BlockSize) >> 2;

    pMbInfo->dwMbPosX    = dwB8X;
    pMbInfo->dwMbPosY    = dwB8Y;
    pMbInfo->i8ZOrder   = (INT8)g_Vp9TxBlockIndex2ZOrderIndexMapSquare64[(dwB8X % 8) + ((dwB8Y % 8) << 3)];//will update in Intel_HostvldVp9_LoopfilterBlock() for each single block 

    //Read blocksize from output surface
    pBlockSize = pFrameState->pOutputBuffer->BlockSize.pu8Buffer + pMbInfo->dwMbOffset + pMbInfo->i8ZOrder;
    dwBlockSizeInSurface = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(*pBlockSize);
    pMbInfo->BlockSize = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)g_Vp9BlockSizeMapLookup[dwBlockSizeInSurface];

    if (BlockSize == BLOCK_8X8)
    {
        pMbInfo->iB4Number  = g_Vp9B4NumberLookup[pMbInfo->BlockSize];
        Intel_HostvldVp9_LoopfilterBlock(pFrameState);
    }
    else if (pMbInfo->BlockSize == BlockSize)//PARTITION_NONE
    {
        pMbInfo->iB4Number  = g_Vp9B4NumberLookup[pMbInfo->BlockSize];
        Intel_HostvldVp9_LoopfilterBlock(pFrameState);
    }
    else if (pMbInfo->BlockSize  == (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize + 4))//PARTITION_HORZ
    {
        pMbInfo->iB4Number  = g_Vp9B4NumberLookup[pMbInfo->BlockSize];
        Intel_HostvldVp9_LoopfilterBlock(pFrameState);
        pMbInfo->dwMbPosY    += dwSplitBlockSize;
        if (pMbInfo->dwMbPosY < (INT)pFrameInfo->dwB8Rows)
        {
            Intel_HostvldVp9_LoopfilterBlock(pFrameState);
        }
    }
    else if (pMbInfo->BlockSize  == (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize + 8))//PARTITION_VERT
    {
        pMbInfo->iB4Number  = g_Vp9B4NumberLookup[pMbInfo->BlockSize];
        Intel_HostvldVp9_LoopfilterBlock(pFrameState);
        pMbInfo->dwMbPosX    += dwSplitBlockSize;
        if (pMbInfo->dwMbPosX < (INT)pFrameInfo->dwB8Columns)
        {
            Intel_HostvldVp9_LoopfilterBlock(pFrameState);
        }
    }
    else if(dwBlockSizeInSurface <= g_Vp9BlockSizeLookup[BlockSize - 1])//PARTITION_SPLIT
    {
        BlockSize = (INTEL_HOSTVLD_VP9_BLOCK_SIZE)(BlockSize - 1);
        Intel_HostvldVp9_LoopfilterSuperBlock(
            pFrameState, 
            dwB8X, 
            dwB8Y, 
            BlockSize);
        Intel_HostvldVp9_LoopfilterSuperBlock(
            pFrameState, 
            dwB8X + dwSplitBlockSize, 
            dwB8Y, 
            BlockSize);
        Intel_HostvldVp9_LoopfilterSuperBlock(
            pFrameState, 
            dwB8X, 
            dwB8Y + dwSplitBlockSize, 
            BlockSize);
        Intel_HostvldVp9_LoopfilterSuperBlock(
            pFrameState, 
            dwB8X + dwSplitBlockSize, 
            dwB8Y + dwSplitBlockSize, 
            BlockSize);
    }
    else
    {
        eStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        printf("Invalid partition type.");
    }

finish:
    return eStatus;

}

VAStatus Intel_HostvldVp9_LoopfilterOneTile(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState,
    PINTEL_HOSTVLD_VP9_TILE_INFO     pTileInfo)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO      pFrameInfo;
    PINTEL_HOSTVLD_VP9_MB_INFO         pMbInfo;
    INTEL_HOSTVLD_VP9_LOOP_FILTER_MASK LoopFilterMaskSB;
    DWORD                                 dwB8X, dwB8Y, dwTileBottomB8, dwTileRightB8, dwLineDist;
    VAStatus                            eStatus = VA_STATUS_SUCCESS;


    pFrameInfo                 = &pFrameState->FrameInfo;
    pMbInfo                    = &pFrameState->pTileStateBase->MbInfo;
    pMbInfo->pCurrTile         = pTileInfo;
    pMbInfo->pLoopFilterMaskSB = &LoopFilterMaskSB;

    if (pTileInfo->dwTileTop == 0)
    {
        pMbInfo->dwMbOffset = pTileInfo->dwTileLeft << VP9_LOG2_B64_SIZE_IN_B8;
    }

    dwTileRightB8  = pTileInfo->dwTileLeft + pTileInfo->dwTileWidth;
    dwTileBottomB8 = pTileInfo->dwTileTop  + pTileInfo->dwTileHeight;
    dwLineDist     = (pFrameInfo->dwMbStride -
        ALIGN(pMbInfo->pCurrTile->dwTileWidth, VP9_B64_SIZE_IN_B8)) << VP9_LOG2_B64_SIZE_IN_B8;

    for (dwB8Y = pTileInfo->dwTileTop; dwB8Y < dwTileBottomB8; dwB8Y += VP9_B64_SIZE_IN_B8)
    {
        // Loopfilter one row
        for (dwB8X = pTileInfo->dwTileLeft; dwB8X < dwTileRightB8; dwB8X += VP9_B64_SIZE_IN_B8)
        {
            memset(&LoopFilterMaskSB, 0, sizeof(INTEL_HOSTVLD_VP9_LOOP_FILTER_MASK));
            Intel_HostvldVp9_LoopfilterSuperBlock(
                pFrameState, 
                dwB8X, 
                dwB8Y, 
                BLOCK_64X64);
            Intel_HostvldVp9_LoopfilterCalcMaskInSuperBlock(
                pFrameState,
                dwB8Y,
                dwB8X,
                dwTileBottomB8,
                dwTileRightB8);

            pMbInfo->dwMbOffset += VP9_B64_SIZE;
        }

        pMbInfo->dwMbOffset += dwLineDist;
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_LoopfilterFrame(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_TILE_INFO     pTileInfo;
    DWORD                               dwTileIndex;
    DWORD                               dwTileX, dwTileY;
    VAStatus                          eStatus     = VA_STATUS_SUCCESS;


    pFrameInfo      = &pFrameState->FrameInfo;
    pTileInfo       = pFrameInfo->TileInfo;

    // decode tiles
    for (dwTileX = 0; dwTileX < pFrameInfo->dwTileColumns; dwTileX++)
    {
        for (dwTileY = 0; dwTileY < pFrameInfo->dwTileRows; dwTileY++)
        {
            dwTileIndex = dwTileY * pFrameInfo->dwTileColumns +dwTileX;
            pTileInfo   = pFrameInfo->TileInfo + dwTileIndex;

            // Loopfilter one tile
            Intel_HostvldVp9_LoopfilterOneTile(pFrameState, pTileInfo);
        }
    }

    // Calc Loop filter threshold
    Intel_HostvldVp9_LoopfilterCalcThreshold(pFrameState);

finish:
    return eStatus;
}
#endif//SEPERATE_LOOPFILTER_ENABLE












