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
#ifndef __INTEL_HOSTVLD_VP9_H__
#define __INTEL_HOSTVLD_VP9_H__

#include "pthread.h"
#include "media_drv_driver.h"
#include "intel_hybrid_common_vp9.h"

typedef enum _INTEL_HOSTVLD_VP9_YUV_PLANE
{
    INTEL_HOSTVLD_VP9_YUV_PLANE_Y = 0,
    INTEL_HOSTVLD_VP9_YUV_PLANE_UV,                                    // UV interleaved
    INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER,
    INTEL_HOSTVLD_VP9_YUV_PLANE_U = INTEL_HOSTVLD_VP9_YUV_PLANE_UV, // separate U & V planes
    INTEL_HOSTVLD_VP9_YUV_PLANE_V
} INTEL_HOSTVLD_VP9_YUV_PLANE;

typedef void *INTEL_HOSTVLD_VP9_HANDLE, **PINTEL_HOSTVLD_VP9_HANDLE;

// 1D buffer type
typedef struct _INTEL_HOSTVLD_VP9_1D_BUFFER
{
    union
    {
        uint8_t      *pu8Buffer;
        uint16_t     *pu16Buffer;
        uint32_t     *pu32Buffer;
    };
    uint32_t           dwSize;     // size in bytes or words
} INTEL_HOSTVLD_VP9_1D_BUFFER, *PINTEL_HOSTVLD_VP9_1D_BUFFER;

// 2D buffer type
typedef struct _INTEL_HOSTVLD_VP9_2D_BUFFER
{
    union
    {
        uint8_t      *pu8Buffer;
        uint16_t     *pu16Buffer;
    };
    uint32_t           dwWidth;        // width in bytes or words depending on the buffer data
    uint32_t           dwHeight;
    uint32_t           dwPitch;
    uint32_t           dwSize;
} INTEL_HOSTVLD_VP9_2D_BUFFER, *PINTEL_HOSTVLD_VP9_2D_BUFFER;

// video buffer structure used as HostVLD input
typedef struct _INTEL_HOSTVLD_VP9_VIDEO_BUFFER
{
    PINTEL_VP9_PIC_PARAMS        pVp9PicParams;
    PINTEL_VP9_SEGMENT_PARAMS    pVp9SegmentData;

    uint8_t                           *pbBitsData;     // bitstream buffer
    uint32_t                           dwBitsSize;     // bitstream size

    /* Added by Zhao Yakui */
    dri_bo			*slice_data_bo;

    INTEL_HOSTVLD_VP9_1D_BUFFER  PrevReferenceFrame;
    INTEL_HOSTVLD_VP9_1D_BUFFER  PrevMotionVector;

    struct object_surface                   *pRenderTarget;
} INTEL_HOSTVLD_VP9_VIDEO_BUFFER, *PINTEL_HOSTVLD_VP9_VIDEO_BUFFER;

// data planes used as HostVLD output
typedef struct _INTEL_HOSTVLD_VP9_OUTPUT_BUFFER
{
    INTEL_HOSTVLD_VP9_1D_BUFFER  TransformCoeff[INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER + 1];
    INTEL_HOSTVLD_VP9_1D_BUFFER  TransformSize[INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER];
    INTEL_HOSTVLD_VP9_1D_BUFFER  CoeffStatus[INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER];
    INTEL_HOSTVLD_VP9_1D_BUFFER  PredictionMode[INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER];
    INTEL_HOSTVLD_VP9_1D_BUFFER  QP[INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER];
    INTEL_HOSTVLD_VP9_2D_BUFFER  VerticalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER];
    INTEL_HOSTVLD_VP9_2D_BUFFER  HorizontalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER];
    INTEL_HOSTVLD_VP9_1D_BUFFER  TransformType;      // For Luma only; Chroma should always use DCT_DCT transform
    INTEL_HOSTVLD_VP9_1D_BUFFER  SkipFlag;           // Y, U and V share the same skip flags
    INTEL_HOSTVLD_VP9_1D_BUFFER  SegmentIndex;       // Y, U and V share the same segmentation index lookup table
    INTEL_HOSTVLD_VP9_1D_BUFFER  TileIndex;          // Y, U and V share the same tile index
    INTEL_HOSTVLD_VP9_1D_BUFFER  InterIntraFlag;     // Y, U and V share the same Inter/Intra flags
    INTEL_HOSTVLD_VP9_1D_BUFFER  BlockSize;          // Y, U and V share the same block size
    INTEL_HOSTVLD_VP9_1D_BUFFER  ReferenceFrame;     // Y, U and V share the same reference frame buffer
    INTEL_HOSTVLD_VP9_1D_BUFFER  FilterType;         // Y, U and V share the same interpolation filter type
    INTEL_HOSTVLD_VP9_1D_BUFFER  MotionVector;       // Y, U and V share the same motion vector buffer
    INTEL_HOSTVLD_VP9_2D_BUFFER  FilterLevel;        // Y, U and V share the same filter levels
    INTEL_HOSTVLD_VP9_2D_BUFFER  Threshold;          // Y, U and V share the same thresholds
} INTEL_HOSTVLD_VP9_OUTPUT_BUFFER, *PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER;

// Callback functions
typedef VAStatus (* PFNINTEL_HOSTVLD_VP9_DEBLOCKCB) (
    void       *pvStandardState,
    uint32_t        uiCurrIndex);

typedef VAStatus (* PFNINTEL_HOSTVLD_VP9_RENDERCB) (
    void       *pvStandardState,
    uint32_t        uiCurrIndex, 
    uint32_t        uiPrevIndex);

typedef VAStatus (* PFNINTEL_HOSTVLD_VP9_SYNCCB) (
    void                               *pvStandardState, 
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pHostVldVideoBuf, 
    uint32_t                                uiCurrIndex, 
    uint32_t                                uiPrevIndex);

typedef struct _INTEL_HOSTVLD_VP9_CALLBACKS
{
    PFNINTEL_HOSTVLD_VP9_DEBLOCKCB pfnHostVldDeblockCb;
    PFNINTEL_HOSTVLD_VP9_RENDERCB  pfnHostVldRenderCb;
    PFNINTEL_HOSTVLD_VP9_SYNCCB    pfnHostVldSyncCb;
    void                             *pvStandardState;
} INTEL_HOSTVLD_VP9_CALLBACKS, *PINTEL_HOSTVLD_VP9_CALLBACKS;

// function interface
//
VAStatus Intel_HostvldVp9_Create (
    PINTEL_HOSTVLD_VP9_HANDLE         hHostVld,
    PINTEL_HOSTVLD_VP9_CALLBACKS     pCallbacks);

VAStatus Intel_HostvldVp9_QueryBufferSize (
    INTEL_HOSTVLD_VP9_HANDLE         hHostVld,
    uint32_t                            *pdwBufferSize);

VAStatus Intel_HostvldVp9_SetOutputBuffer (
    INTEL_HOSTVLD_VP9_HANDLE         hHostVld,
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER pOutputBuffer);

VAStatus Intel_HostvldVp9_Initialize (
    INTEL_HOSTVLD_VP9_HANDLE         hHostVld,
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pVideoBuffer);

VAStatus Intel_HostvldVp9_Execute (
    INTEL_HOSTVLD_VP9_HANDLE         hHostVld);

VAStatus Intel_HostvldVp9_Destroy (
    INTEL_HOSTVLD_VP9_HANDLE         hHostVld);

#endif // __INTEL_HOSTVLD_VP9_H__
