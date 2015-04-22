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

#ifndef _DECODE_HYBRID_VP9_H__
#define _DECODE_HYBRID_VP9_H__

#include "media_drv_driver.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"

#include "cmrt_api.h"
#include "intel_hybrid_hostvld_vp9.h"
#include "intel_hybrid_common_vp9.h"

#include <malloc.h>

#define INTEL_HYBRID_VP9_PAGE_SIZE       0x1000

#define INTEL_HYBRID_VP9_KEY_FRAME       0

typedef enum _INTEL_HYBRID_VP9_MDF_YUV_PLANE
{
    INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y = 0,
    INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV,          // UV interleaved
    INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER,
    INTEL_HYBRID_VP9_MDF_YUV_PLANE_U = INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV,
    INTEL_HYBRID_VP9_MDF_YUV_PLANE_V
} INTEL_HYBRID_VP9_MDF_YUV_PLANE;

typedef enum _INTEL_HYBRID_VP9_MDF_INTRAPRED_KERNELMODE
{
    INTEL_HYBRID_VP9_MDF_INTRAPRED_8x8 = 0,
    INTEL_HYBRID_VP9_MDF_INTRAPRED_16x16,
    INTEL_HYBRID_VP9_MDF_INTRAPRED_32x32
} INTEL_HYBRID_VP9_MDF_INTRAPRED_KERNELMODE;

typedef enum _INTEL_HYBRID_VP9_MDF_DEBLOCK_REGION
{
    INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_TOP = 0,
    INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_TOP,
    INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_BOTTOM,
    INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_BOTTOM,
    INTEL_HYBRID_VP9_MDF_DEBLOCK_REGIONS
} INTEL_HYBRID_VP9_MDF_DEBLOCK_REGION;

typedef struct _INTEL_DECODE_HYBRID_VP9_MDF_DEBLOCK
{
    DWORD           dwIndicator;
    CmThreadSpace   *pThreadSpace[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    CmKernel        *pKernel[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
} INTEL_DECODE_HYBRID_VP9_MDF_DEBLOCK, *PINTEL_DECODE_HYBRID_VP9_MDF_DEBLOCK;

typedef struct _INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER
{
    union
    {
        uint8_t      *pu8Buffer;
        uint16_t     *pu16Buffer;
        uint32_t     *pu32Buffer;
        uint64_t     *pu64Buffer;
        void       *pBuffer;
    };
    CmBuffer      *pMdfBuffer;
    uint32_t           dwSize;     // size in bytes or words depending on the buffer data
    dri_bo	*bo;
    int bo_mapped; /* indicate whether bo is mapped */
} INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER, *PINTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER;

typedef struct _INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER
{
    union
    {
        uint8_t      *pu8Buffer;
        int16_t     *pu16Buffer;
        void       *pBuffer;
    };
    union
    {
        CmSurface2D     *pMdfSurface;
    };
    uint32_t           dwWidth;        // width in bytes or words depending on the buffer data
    uint32_t           dwHeight;
    uint32_t           dwPitch;
    uint32_t           dwSize;
    dri_bo	*bo;
    int bo_mapped; /* indicate whether bo is mapped */
} INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER, *PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER;

typedef struct _INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE
{
    INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER    Frame;
    bool                                        bHasPadding;
    int32_t                                       dwWidth;    // Surface width
    int32_t                                       dwHeight;   // Surface height
} INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE, *PINTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE;


typedef struct _INTEL_DECODE_HYBRID_VP9_MDF_BUFFER
{
    // Input surfaces
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_V + 1];
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    TransformSize[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    QP[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER    VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER    HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER    DeblockOntheFlyThreadMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER    ThreadInfo[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    TransformType;      // For Luma only; Chroma should always use DCT_DCT transform
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    TileIndex;          // Y, U and V share the same tile index
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    BlockSize;          // Y, U and V share the same block size
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    ReferenceFrame;     // Y, U and V share the same reference frame index
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    FilterType;         // Y, U and V share the same interpolation filter type
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    MotionVector;       // Y, U and V share the same motion vector
    INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER    FilterLevel;        // Y, U and V share the same filter levels
    INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER    Threshold;          // Y, U and V share the same thresholds

    // previous frame surfaces used for inter prediction
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    PrevMotionVector;   // Previous frame motion vector buffer
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    PrevReferenceFrame; // Previous frame reference frame index buffer
} INTEL_DECODE_HYBRID_VP9_MDF_BUFFER, *PINTEL_DECODE_HYBRID_VP9_MDF_BUFFER;

typedef struct _INTEL_DECODE_HYBRID_VP9_MDF_FRAME
{
    // Host Buffers
    INTEL_DECODE_HYBRID_VP9_MDF_BUFFER       MdfDecodeBuffer;

    CmQueue         *pMdfQueue;
    CmEvent         *pMdfEvent; // Check if the kernel is executed before next round write surface

    int32_t           dwWidth;
    int32_t           dwHeight;
    uint32_t           dwMaxWidth;
    uint32_t           dwMaxHeight;
    uint32_t           dwAlignedWidth;     // SB64 aligned Width
    uint32_t           dwAlignedHeight;    // SB64 aligned Height
    uint32_t           dwWidthB8;
    uint32_t           dwHeightB8;
    uint32_t           dwWidthB16;
    uint32_t           dwHeightB16;
    uint32_t           dwWidthB32;
    uint32_t           dwHeightB32;
    uint32_t           dwWidthB64;
    uint32_t           dwHeightB64;

	VASurfaceID	CurrPic;
	VASurfaceID	ucCurrIndex;
	VASurfaceID	ucLastRefIndex;
	VASurfaceID	ucGoldenRefIndex;
	VASurfaceID	ucAltRefIndex;
    bool            bShowFrame;
    bool            bIntraOnly;
    bool            bSwitchableFilterType;
    bool            bNeedDeblock;
    bool            bLossless;
    bool            bResolutionChange;
    bool            bUseCollocatedMV;
    uint32_t           dwInterpolationFilterType;
    uint32_t            uiStatusReportFeedbackNumber;         

    // previous frame information
    bool            bPrevShowFrame;

    uint32_t           dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
} INTEL_DECODE_HYBRID_VP9_MDF_FRAME, *PINTEL_DECODE_HYBRID_VP9_MDF_FRAME;

#define INTEL_NUM_UNCOMPRESSED_SURFACE_VP9   128

typedef struct _INTEL_DECODE_HYBRID_VP9_MDF_ENGINE
{
    CmKernel        *pKernelIqIt[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    CmKernel        *pKernelIqItLossless[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    CmKernel        *pKernelIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    CmKernel        *pKernelZeroFill[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    CmKernel        *pKernelRefPadding;
    CmKernel        *pKernelRefPaddingYOnly;
    CmKernel        *pKernelInter;
    CmKernel        *pKernelInterScaling;
    CmKernel        *pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER][INTEL_HYBRID_VP9_MDF_DEBLOCK_REGIONS]; // Different frames (different resolutions) may use different deblocking kernels
    CmKernel        *pKernelDeblock8x8[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    CmKernel        *pKernelDeblock16x8[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];

    CmThreadSpace   *pThreadSpaceIqIt[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    CmThreadSpace   *pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    CmThreadSpace   *pThreadSpaceZeroFill[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    CmThreadSpace   *pThreadSpaceInter;
    CmThreadSpace   *pThreadSpaceDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER][INTEL_HYBRID_VP9_MDF_DEBLOCK_REGIONS];

    CmTask          *pTaskIqIt[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    CmTask          *pTaskIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];
    CmTask          *pTaskInter;
    CmTask          *pTaskDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];

    CmDevice        *pMdfDevice;

    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME       pMdfDecodeFrame;
    INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE FrameList[INTEL_NUM_UNCOMPRESSED_SURFACE_VP9];

    // Intermediate surfaces for kernels
    INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER    Residue[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];

    // Combined 3x8-tap filter coefficients (each filter is 16x8 bytes).
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    CombinedFilters;

    // kernel related
    uint32_t           dwLumaDeblockThreadWidth;
    uint32_t           dwLumaDeblockThreadHeight;
    uint32_t           dwLumaDeblock26ZIMBWidth;
    uint32_t           dwChromaDeblockThreadWidth;
    uint32_t           dwChromaDeblockThreadHeight;
    uint32_t           dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER];

    uint32_t           dwMdfBufferSize;
    bool            bAllocated;
    bool            bResolutionChanged;
} INTEL_DECODE_HYBRID_VP9_MDF_ENGINE, *PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE;

typedef struct _INTEL_DECODE_HYBRID_VP9_STATE INTEL_DECODE_HYBRID_VP9_STATE, *PINTEL_DECODE_HYBRID_VP9_STATE;

struct _INTEL_DECODE_HYBRID_VP9_STATE
{
    int                                     dwThreadNumber;
    // Parameters passed by application
    uint32_t                                 dwWidth;            // Picture Width
    uint32_t                                 dwHeight;           // Picture Height
    uint32_t                                 dwCropWidth;        // Cropped/Original Picture Width
    uint32_t                                 dwCropHeight;       // Cropped/Original Picture Height
    /* Update it to VASurfaceID */
    VASurfaceID                                 ucCurrIndex;

    struct object_surface                           *sDestSurface;
                                      
    // Internally maintained            
    INTEL_DECODE_HYBRID_VP9_MDF_ENGINE MdfDecodeEngine;
    INTEL_HOSTVLD_VP9_HANDLE           hHostVld;
    INTEL_HOSTVLD_VP9_VIDEO_BUFFER     HostVldVideoBuf;
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER   pHostVldOutputBuf;
    uint32_t                                 dwMdfBufferSize;
    PBYTE                                 pCopyDataBuffer;
    uint32_t                              dwCopyDataBufferSize;
    uint32_t                              dwCopyDataOffset;
    bool                                  bCopyDataBufferInUse;
    uint32_t                                 dwDataSize;
    PINTEL_VP9_PIC_PARAMS                 pVp9PicParams;
    bool                                  bStatusReportingEnabled;
    void                                  *pDecodeStatusBuf;

    /* This is to keep the VADriverContextP */
    void	*driver_context;

};

void Intel_HybridVp9Decode_Destroy(
    VADriverContextP ctx, 
    void *hw_context);

VAStatus Intel_HybridVp9Decode_Initialize(
    VADriverContextP ctx, 
    void *hw_context);

VAStatus Intel_HybridVp9Decode_AllocateResources(
    PINTEL_DECODE_HYBRID_VP9_STATE pVp9State);

typedef struct _Hybrid_VP9_HW_CONTEXT_ {
	struct hw_context context;
	INTEL_DECODE_HYBRID_VP9_STATE vp9_state;
     
        INTEL_VP9_PIC_PARAMS vp9_pic_params;
	INTEL_VP9_SEGMENT_PARAMS vp9_matrixbuffer;
        struct object_surface *sDestSurface; 
} hybrid_vp9_hw_context;

#endif  // _DECODE_HYBRID_VP9_H__
