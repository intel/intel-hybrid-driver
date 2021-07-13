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

#include "media_drv_driver.h"
#include "media_drv_surface.h"
#include <fcntl.h>
#include "cmrt_api.h"
#include "decode_hybrid_vp9.h"
#include "intel_hybrid_debug_dump.h"
#include <errno.h>

#include <va/va_dec_vp9.h>
#include <sys/ioctl.h>

#define MDF_KERNEL_FUNCTION_IQIT_LUMA               "InverseTransform_Luma_Granularity_32x32"
#define MDF_KERNEL_FUNCTION_IQIT_CHROMA             "InverseTransform_Chroma_Granularity_32x32"
#define MDF_KERNEL_FUNCTION_IQIT_LOSSLESS_LUMA      "InverseTransform_LL_Luma_Granularity_32x32"
#define MDF_KERNEL_FUNCTION_IQIT_LOSSLESS_CHROMA    "InverseTransform_LL_Chroma_Granularity_32x32"
#define MDF_KERNEL_FUNCTION_INTRA_PRED_8X8_LUMA     "PredictIntra_Luma_Granularity_8x8"
#define MDF_KERNEL_FUNCTION_INTRA_PRED_8X8_CHROMA   "PredictIntra_Chroma_Granularity_8x8"
#define MDF_KERNEL_FUNCTION_INTRA_PRED_16X16_LUMA   "PredictIntra_Luma_Granularity_16x16"
#define MDF_KERNEL_FUNCTION_INTRA_PRED_16X16_CHROMA "PredictIntra_Chroma_Granularity_16x16"
#define MDF_KERNEL_FUNCTION_INTRA_PRED_32X32_LUMA   "PredictIntra_Luma_Granularity_32x32"
#define MDF_KERNEL_FUNCTION_INTRA_PRED_32X32_CHROMA "PredictIntra_Chroma_Granularity_32x32"
#define MDF_KERNEL_FUNCTION_ZERO_FILL               "ZeroFill_ThreadStatusSurface"
#define MDF_KERNEL_FUNCTION_INTER_PRED              "Inter_Pred_GENX"
#define MDF_KERNEL_FUNCTION_INTER_PRED_SCALING      "Inter_Pred_Scaling_GENX"
#define MDF_KERNEL_FUNCTION_REF_PADDING             "Ref_Padding_GENX"
#define MDF_KERNEL_FUNCTION_REF_Y_ONLY_PADDING      "Ref_Padding_Y_Only_GENX"
#define MDF_KERNEL_FUNCTION_DEBLOCK_LUMA            "Deblock_Luma_Granularity_8x8_SingleTile"
#define MDF_KERNEL_FUNCTION_DEBLOCK_CHROMA          "Deblock_Chroma_Granularity_8x8_SingleTile"
#define MDF_KERNEL_FUNCTION_DEBLOCK16x8_LUMA        "Deblock_Luma_Granularity_16x8_SingleTile"

// Block size
#define INTEL_HYBRID_VP9_LOG2_B8_SIZE    3
#define INTEL_HYBRID_VP9_LOG2_B64_SIZE   6
#define INTEL_HYBRID_VP9_B64_SIZE        (1 << INTEL_HYBRID_VP9_LOG2_B64_SIZE) // 64
#define INTEL_HYBRID_VP9_B8_SIZE         (1 << INTEL_HYBRID_VP9_LOG2_B8_SIZE)  // 8

// Flags for kernel buffer allocation and release
#define VP9_HYBRID_DECODE_CURRENT_FRAME     (1 << 0)
#define VP9_HYBRID_DECODE_PREVIOUS_FRAME    (1 << 1)
#define VP9_HYBRID_DECODE_ALL_FRAMES        (VP9_HYBRID_DECODE_PREVIOUS_FRAME | VP9_HYBRID_DECODE_CURRENT_FRAME)

#define VP9_HYBRID_DECODE_REF_SCALE_SHIFT   14

#define VP9_HYBRID_DECODE_COMBINED_FILETER_SIZE 512

#define INTEL_DECODE_CHK_MDF_STATUS(_mdfstatus)                      \
do                                                                      \
{                                                                       \
    if (CM_SUCCESS != _mdfstatus)                                       \
    {                                                                   \
        eStatus = VA_STATUS_ERROR_OPERATION_FAILED;                                   \
        goto finish;                                                    \
    }                                                                   \
} while (0)

#define INTEL_DECODE_HYBRID_VP9_DESTROY_THREADSPACE(pThreadSpace)                \
do                                                                                  \
{                                                                                   \
    if ((pThreadSpace) != NULL)                                                     \
    {                                                                               \
        pMdfDevice->DestroyThreadSpace(pThreadSpace);   \
        (pThreadSpace) = NULL;                                                      \
    }                                                                               \
} while (0)

// Kernel HEX Arrays G75
extern uint32_t Vp9Deblock_g75_size;
extern uint32_t Vp9Transform_g75_size;
extern uint32_t Vp9InterPred_g75_size;
extern uint32_t Vp9InterPredScaling_g75_size;
extern uint32_t Vp9IntraPred_g75_size;
extern uint32_t Vp9Deblock_g75[]; 
extern uint32_t Vp9Transform_g75[];
extern uint32_t Vp9InterPred_g75[];
extern uint32_t Vp9InterPredScaling_g75[];
extern uint32_t Vp9IntraPred_g75[];

extern uint32_t Vp9Deblock_g8_size;
extern uint32_t Vp9Transform_g8_size;
extern uint32_t Vp9InterPred_g8_size;
extern uint32_t Vp9InterPredScaling_g8_size;
extern uint32_t Vp9IntraPred_g8_size;
extern uint32_t Vp9Deblock_g8[];
extern uint32_t Vp9Transform_g8[];
extern uint32_t Vp9InterPred_g8[];
extern uint32_t Vp9InterPredScaling_g8[];
extern uint32_t Vp9IntraPred_g8[];

extern uint32_t Vp9Deblock_g8lp_size;
extern uint32_t Vp9Transform_g8lp_size;
extern uint32_t Vp9InterPred_g8lp_size;
extern uint32_t Vp9InterPredScaling_g8lp_size;
extern uint32_t Vp9IntraPred_g8lp_size;
extern uint32_t Vp9Deblock_g8lp[];
extern uint32_t Vp9Transform_g8lp[];
extern uint32_t Vp9InterPred_g8lp[];
extern uint32_t Vp9InterPredScaling_g8lp[];
extern uint32_t Vp9IntraPred_g8lp[];

extern uint32_t Vp9Deblock_g9_size;
extern uint32_t Vp9Transform_g9_size;
extern uint32_t Vp9InterPred_g9_size;
extern uint32_t Vp9InterPredScaling_g9_size;
extern uint32_t Vp9IntraPred_g9_size;
extern uint32_t Vp9Deblock_g9[];
extern uint32_t Vp9Transform_g9[];
extern uint32_t Vp9InterPred_g9[];
extern uint32_t Vp9InterPredScaling_g9[];
extern uint32_t Vp9IntraPred_g9[];

int16_t g_Filters8Tap[16][8] = 
{
    {  0,   0,   0, 128,   0,   0,   0,   0},
    {  0,   1,  -5, 126,   8,  -3,   1,   0},
    { -1,   3, -10, 122,  18,  -6,   2,   0},
    { -1,   4, -13, 118,  27,  -9,   3,  -1},
    { -1,   4, -16, 112,  37, -11,   4,  -1},
    { -1,   5, -18, 105,  48, -14,   4,  -1},
    { -1,   5, -19,  97,  58, -16,   5,  -1},
    { -1,   6, -19,  88,  68, -18,   5,  -1},
    { -1,   6, -19,  78,  78, -19,   6,  -1},
    { -1,   5, -18,  68,  88, -19,   6,  -1},
    { -1,   5, -16,  58,  97, -19,   5,  -1},
    { -1,   4, -14,  48, 105, -18,   5,  -1},
    { -1,   4, -11,  37, 112, -16,   4,  -1},
    { -1,   3,  -9,  27, 118, -13,   4,  -1},
    {  0,   2,  -6,  18, 122, -10,   3,  -1},
    {  0,   1,  -3,   8, 126,  -5,   1,   0}
};

int16_t g_Filters8TapSmooth[16][8] = 
{
    {  0,   0,   0, 128,   0,   0,   0,   0},
    { -3,  -1,  32,  64,  38,   1,  -3,   0},
    { -2,  -2,  29,  63,  41,   2,  -3,   0},
    { -2,  -2,  26,  63,  43,   4,  -4,   0},
    { -2,  -3,  24,  62,  46,   5,  -4,   0},
    { -2,  -3,  21,  60,  49,   7,  -4,   0},
    { -1,  -4,  18,  59,  51,   9,  -4,   0},
    { -1,  -4,  16,  57,  53,  12,  -4,  -1},
    { -1,  -4,  14,  55,  55,  14,  -4,  -1},
    { -1,  -4,  12,  53,  57,  16,  -4,  -1},
    {  0,  -4,   9,  51,  59,  18,  -4,  -1},
    {  0,  -4,   7,  49,  60,  21,  -3,  -2},
    {  0,  -4,   5,  46,  62,  24,  -3,  -2},
    {  0,  -4,   4,  43,  63,  26,  -2,  -2},
    {  0,  -3,   2,  41,  63,  29,  -2,  -2},
    {  0,  -3,   1,  38,  64,  32,  -1,  -3}
};

int16_t g_Filters8TapSharp[16][8] = 
{
    {  0,   0,   0, 128,   0,   0,   0,   0},
    { -1,   3,  -7, 127,   8,  -3,   1,   0},
    { -2,   5, -13, 125,  17,  -6,   3,  -1},
    { -3,   7, -17, 121,  27, -10,   5,  -2},
    { -4,   9, -20, 115,  37, -13,   6,  -2},
    { -4,  10, -23, 108,  48, -16,   8,  -3},
    { -4,  10, -24, 100,  59, -19,   9,  -3},
    { -4,  11, -24,  90,  70, -21,  10,  -4},
    { -4,  11, -23,  80,  80, -23,  11,  -4},
    { -4,  10, -21,  70,  90, -24,  11,  -4},
    { -3,   9, -19,  59, 100, -24,  10,  -4},
    { -3,   8, -16,  48, 108, -23,  10,  -4},
    { -2,   6, -13,  37, 115, -20,   9,  -4},
    { -2,   5, -10,  27, 121, -17,   7,  -3},
    { -1,   3,  -6,  17, 125, -13,   5,  -2},
    {  0,   1,  -3,   8, 127,  -7,   3,  -1}
};

int16_t g_FiltersBilinear[16][8] =
{
    {  0,   0,   0, 128,   0,   0,   0,   0},
    {  0,   0,   0, 120,   8,   0,   0,   0},
    {  0,   0,   0, 112,  16,   0,   0,   0},
    {  0,   0,   0, 104,  24,   0,   0,   0},
    {  0,   0,   0,  96,  32,   0,   0,   0},
    {  0,   0,   0,  88,  40,   0,   0,   0},
    {  0,   0,   0,  80,  48,   0,   0,   0},
    {  0,   0,   0,  72,  56,   0,   0,   0},
    {  0,   0,   0,  64,  64,   0,   0,   0},
    {  0,   0,   0,  56,  72,   0,   0,   0},
    {  0,   0,   0,  48,  80,   0,   0,   0},
    {  0,   0,   0,  40,  88,   0,   0,   0},
    {  0,   0,   0,  32,  96,   0,   0,   0},
    {  0,   0,   0,  24, 104,   0,   0,   0},
    {  0,   0,   0,  16, 112,   0,   0,   0},
    {  0,   0,   0,   8, 120,   0,   0,   0}
};
#ifdef _CM_BUFFER_UP_
/* This is reserved for the future usage. It is valid only when it can map the user_space 
 * allocated memory into gfx memory. At the same time the libdrm should also 
 * be updated
 */
static VAStatus
INTEL_HYBRID_VP9_ALLOCATE_MDF_2DUP_BUFFER_UINT8(
	VADriverContextP ctx,
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER *pMdfBuffer2D,
	int Width, int Height)
{
    int buf_size;
    INT cm_status;

    pMdfBuffer2D->dwWidth           = Width;
    pMdfBuffer2D->dwHeight          = Height;
    cm_status = pMdfDevice->GetSurface2DInfo(
	pMdfBuffer2D->dwWidth,
	pMdfBuffer2D->dwHeight,
	VA_CM_FMT_A8,
	(UINT &)pMdfBuffer2D->dwPitch,
	(UINT &)pMdfBuffer2D->dwSize);

    if (cm_status != CM_SUCCESS)
	goto allocation_fail;

    buf_size = ALIGN(pMdfBuffer2D->dwSize, INTEL_HYBRID_VP9_PAGE_SIZE);

    /* allocation based on alignment */
    pMdfBuffer2D->pBuffer = memalign(INTEL_HYBRID_VP9_PAGE_SIZE, buf_size);
    memset(pMdfBuffer2D->pBuffer, 0, buf_size);

    cm_status = pMdfDevice->CreateSurface2DUP(
	pMdfBuffer2D->dwWidth,
	pMdfBuffer2D->dwHeight,
	VA_CM_FMT_A8,
	pMdfBuffer2D->pBuffer,
	pMdfBuffer2D->pMdfSurfaceUP);

    if (cm_status != CM_SUCCESS)
	goto allocation_fail;

    return VA_STATUS_SUCCESS;

allocation_fail:
    return VA_STATUS_ERROR_ALLOCATION_FAILED;
}


static
VAStatus INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT8(
	VADriverContextP ctx,
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER *pMdfBuffer1D,
	int dwBufferSize)
{
    int buf_size, cm_status;

    buf_size = ALIGN((dwBufferSize) * sizeof(uint8_t), INTEL_HYBRID_VP9_PAGE_SIZE);

    pMdfBuffer1D->dwSize    = (dwBufferSize);
    pMdfBuffer1D->pu8Buffer = memalign(INTEL_HYBRID_VP9_PAGE_SIZE, buf_size);
    memset(pMdfBuffer1D->pu8Buffer, 0, buf_size);
    cm_status = pMdfDevice->CreateBufferUP(buf_size, pMdfBuffer1D->pu8Buffer,pMdfBuffer1D->pMdfBuffer);

    if (cm_status == CM_SUCCESS)
	return VA_STATUS_SUCCESS;
    else
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
}

static VAStatus
INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT16(
	VADriverContextP ctx,
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER *pMdfBuffer1D,
	int dwBufferSize)
{
    int buf_size, cm_status;

    buf_size = ALIGN((dwBufferSize) * sizeof(uint16_t), INTEL_HYBRID_VP9_PAGE_SIZE);

    pMdfBuffer1D->dwSize    = (dwBufferSize);
    pMdfBuffer1D->pu8Buffer = memalign(INTEL_HYBRID_VP9_PAGE_SIZE, buf_size);
    memset(pMdfBuffer1D->pu16Buffer, 0, buf_size);
    cm_status = pMdfDevice->CreateBufferUP(buf_size, pMdfBuffer1D->pu16Buffer,pMdfBuffer1D->pMdfBuffer);

    if (cm_status == CM_SUCCESS)
	return VA_STATUS_SUCCESS;
    else
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
}

static VAStatus
INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT64(
	VADriverContextP ctx,
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER *pMdfBuffer1D,
	int dwBufferSize)
{
    int buf_size, cm_status;

    buf_size = ALIGN((dwBufferSize) * sizeof(uint64_t), INTEL_HYBRID_VP9_PAGE_SIZE);

    pMdfBuffer1D->dwSize    = (dwBufferSize);
    pMdfBuffer1D->pu8Buffer = memalign(INTEL_HYBRID_VP9_PAGE_SIZE, buf_size);
    memset(pMdfBuffer1D->pu64Buffer, 0, buf_size);
    cm_status = pMdfDevice->CreateBufferUP(buf_size, pMdfBuffer1D->pu64Buffer,pMdfBuffer1D->pMdfBuffer);

    if (cm_status == CM_SUCCESS)
	return VA_STATUS_SUCCESS;
    else
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
}


static void INTEL_HYBRID_VP9_DESTROY_MDF_2D_BUFFER(
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER *pMdfBuffer2D)
{
    if (pMdfBuffer2D->pMdfSurface)
    {
        pMdfDevice->DestroySurface(pMdfBuffer2D->pMdfSurface);
	pMdfBuffer2D->pMdfSurface = NULL;
    }
}

static void INTEL_HYBRID_VP9_DESTROY_MDF_2DUP_BUFFER(
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER *pMdfBuffer2D)
{

    if (pMdfBuffer2D->pBuffer)
    {
        free(pMdfBuffer2D->pBuffer);
	pMdfBuffer2D->pBuffer = NULL;
    }

    if (pMdfBuffer2D->pMdfSurfaceUP)
    {
        pMdfDevice->DestroySurface(pMdfBuffer2D->pMdfSurfaceUP);
	pMdfBuffer2D->pMdfSurface = NULL;
    }
}


static void INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER *pMdfBuffer1D)
{
    if (pMdfBuffer1D->pBuffer)
    {
	free(pMdfBuffer1D->pBuffer);
	pMdfBuffer1D->pBuffer = NULL;
    }
    if (pMdfBuffer1D->pMdfBuffer)
    {
	pMdfDevice->DestroyBufferUP(pMdfBuffer1D->pMdfBuffer);
	pMdfBuffer1D->pMdfBuffer = NULL;
    }

}

#else

static
void GetCmOsResourceFor2DBuffer(
	CmOsResource *target_source,
	INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER *pMdfBuffer2D,
	dri_bo *bo,
	int format)
{
        /* Use the linear format */
	target_source->format = (VA_CM_FORMAT)format;
	target_source->aligned_width = pMdfBuffer2D->dwWidth; 
	target_source->pitch = pMdfBuffer2D->dwPitch; 
	target_source->aligned_height = pMdfBuffer2D->dwHeight;
	target_source->bo = bo;
	target_source->tile_type = I915_TILING_NONE;
	target_source->bo_flags = DRM_BO_HANDLE;
	target_source->bo_size = pMdfBuffer2D->dwSize;
	target_source->orig_width = pMdfBuffer2D->dwWidth; 
	target_source->pitch = pMdfBuffer2D->dwPitch; 
	target_source->orig_height = pMdfBuffer2D->dwHeight;
}

static VAStatus
INTEL_HYBRID_VP9_ALLOCATE_MDF_2DUP_BUFFER_UINT8(
	VADriverContextP ctx,
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER *pMdfBuffer2D,
	int Width, int Height)
{
    int buf_size;
    INT cm_status;
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    CmOsResource target_resource;

    pMdfBuffer2D->dwWidth           = Width;
    pMdfBuffer2D->dwHeight          = Height;
    cm_status = pMdfDevice->GetSurface2DInfo(
	pMdfBuffer2D->dwWidth,
	pMdfBuffer2D->dwHeight,
	VA_CM_FMT_A8,
	(UINT &)pMdfBuffer2D->dwPitch,
	(UINT &)pMdfBuffer2D->dwSize);

    if (cm_status != CM_SUCCESS)
	goto allocation_fail;

    buf_size = ALIGN(pMdfBuffer2D->dwSize, INTEL_HYBRID_VP9_PAGE_SIZE);

      
    pMdfBuffer2D->bo  = dri_bo_alloc(drv_ctx->drv_data.bufmgr, 
                                        "Buffer", 
                                        buf_size, 4096);

   {
        struct drm_i915_gem_caching bo_cache;

        memset(&bo_cache, 0, sizeof(bo_cache));
        bo_cache.handle = pMdfBuffer2D->bo->handle;
        bo_cache.caching = I915_CACHING_CACHED;

        drmIoctl(drv_ctx->drv_data.fd, DRM_IOCTL_I915_GEM_SET_CACHING, &bo_cache);
    }

    /* Disable reuse this bo which set I915_CACHING_CACHED on CHV/BSW without LLC.
     * Otherwise if it was reused and GTT mapped later, SIGBUS when access the GTT virtual addr.
     * Because drm:i915 "i915_gem_fault", the page fault handler for GTT map pages,
     * checks it as incoherent for accessing snoopable pages through the GTT on GPU without LLC.
     */
    if (IS_CHERRYVIEW(drv_ctx->drv_data.device_id)) {
        drm_intel_bo_disable_reuse(pMdfBuffer2D->bo);
    }

    memset(&target_resource, 0, sizeof(target_resource));

    GetCmOsResourceFor2DBuffer(&target_resource, pMdfBuffer2D, pMdfBuffer2D->bo, VA_CM_FMT_A8);

    cm_status = pMdfDevice->CreateSurface2D(&target_resource,
	pMdfBuffer2D->pMdfSurface);

    if (cm_status != CM_SUCCESS) {
	dri_bo_unreference(pMdfBuffer2D->bo);
	pMdfBuffer2D->bo = NULL;

	goto allocation_fail;
    }
    dri_bo_map(pMdfBuffer2D->bo, 1);
    memset(pMdfBuffer2D->bo->virt, 0, buf_size);
    pMdfBuffer2D->pBuffer = pMdfBuffer2D->bo->virt;
    pMdfBuffer2D->bo_mapped = 1;

    return VA_STATUS_SUCCESS;

allocation_fail:
    return VA_STATUS_ERROR_ALLOCATION_FAILED;
}

static
void GetCmOsResourceFor1DBuffer(
	CmOsResource *target_source,
	INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER *pMdfBuffer1D,
	dri_bo *bo,
        int bpp)
{
	/* only the buf_size is meaningful for 1D buffer */
	target_source->format = VA_CM_FMT_BUFFER;
	target_source->buf_bytes = pMdfBuffer1D->dwSize * bpp;
	target_source->pitch = 1; 
	target_source->aligned_height = 1;
	target_source->bo = bo;
	target_source->tile_type = I915_TILING_NONE;
	target_source->bo_flags = DRM_BO_HANDLE;
	target_source->bo_size = pMdfBuffer1D->dwSize * bpp;
	target_source->orig_width = pMdfBuffer1D->dwSize * bpp; 
	target_source->orig_height = 1;
   
}

static
VAStatus INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT8(
	VADriverContextP ctx,
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER *pMdfBuffer1D,
	int dwBufferSize)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    int buf_size, cm_status;
    CmOsResource target_resource;

    buf_size = ALIGN((dwBufferSize) * sizeof(uint8_t), INTEL_HYBRID_VP9_PAGE_SIZE);

    pMdfBuffer1D->dwSize    = (dwBufferSize);

    pMdfBuffer1D->bo  = dri_bo_alloc(drv_ctx->drv_data.bufmgr, 
                                        "Buffer", 
                                        buf_size, 64);
   {
        struct drm_i915_gem_caching bo_cache;

        memset(&bo_cache, 0, sizeof(bo_cache));
        bo_cache.handle = pMdfBuffer1D->bo->handle;
        bo_cache.caching = I915_CACHING_CACHED;

        drmIoctl(drv_ctx->drv_data.fd, DRM_IOCTL_I915_GEM_SET_CACHING, &bo_cache);
    }

    /* Disable reuse this bo which set I915_CACHING_CACHED on CHV/BSW without LLC.
     * Otherwise if it was reused and GTT mapped later, SIGBUS when access the GTT virtual addr.
     */
    if (IS_CHERRYVIEW(drv_ctx->drv_data.device_id)) {
        drm_intel_bo_disable_reuse(pMdfBuffer1D->bo);
    }

    pMdfBuffer1D->bo_mapped = 0;
    
    pMdfBuffer1D->pu8Buffer = NULL;

    memset(&target_resource, 0, sizeof(target_resource));
    GetCmOsResourceFor1DBuffer(&target_resource, pMdfBuffer1D, pMdfBuffer1D->bo, 1);
        
    cm_status = pMdfDevice->CreateBuffer(&target_resource,pMdfBuffer1D->pMdfBuffer);

    if (cm_status != CM_SUCCESS) {
	dri_bo_unreference(pMdfBuffer1D->bo);
	pMdfBuffer1D->bo = NULL;
	goto allocation_fail;
    }
  
    dri_bo_map(pMdfBuffer1D->bo, 1);
    memset(pMdfBuffer1D->bo->virt, 0, buf_size);
    pMdfBuffer1D->pu8Buffer = (UINT8 *)pMdfBuffer1D->bo->virt;
    pMdfBuffer1D->bo_mapped = 1;
    
    return VA_STATUS_SUCCESS;

allocation_fail:
    return VA_STATUS_ERROR_ALLOCATION_FAILED;
}

static VAStatus
INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT16(
	VADriverContextP ctx,
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER *pMdfBuffer1D,
	int dwBufferSize)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    int buf_size, cm_status;
    CmOsResource target_resource;

    buf_size = ALIGN((dwBufferSize) * sizeof(uint16_t), INTEL_HYBRID_VP9_PAGE_SIZE);

    pMdfBuffer1D->dwSize    = (dwBufferSize);
    pMdfBuffer1D->bo  = dri_bo_alloc(drv_ctx->drv_data.bufmgr, 
                                        "Buffer", 
                                        buf_size, 64);
   {
        struct drm_i915_gem_caching bo_cache;

        memset(&bo_cache, 0, sizeof(bo_cache));
        bo_cache.handle = pMdfBuffer1D->bo->handle;
        bo_cache.caching = I915_CACHING_CACHED;

        drmIoctl(drv_ctx->drv_data.fd, DRM_IOCTL_I915_GEM_SET_CACHING, &bo_cache);
    }

    /* Disable reuse this bo which set I915_CACHING_CACHED on CHV/BSW without LLC.
     * Otherwise if it was reused and GTT mapped later, SIGBUS when access the GTT virtual addr.
     */
    if (IS_CHERRYVIEW(drv_ctx->drv_data.device_id)) {
        drm_intel_bo_disable_reuse(pMdfBuffer1D->bo);
    }

    pMdfBuffer1D->bo_mapped = 0;
    
    pMdfBuffer1D->pu16Buffer = NULL;

    memset(&target_resource, 0, sizeof(target_resource));

    memset(&target_resource, 0, sizeof(target_resource));
    GetCmOsResourceFor1DBuffer(&target_resource, pMdfBuffer1D, pMdfBuffer1D->bo, 2);
    cm_status = pMdfDevice->CreateBuffer(&target_resource,pMdfBuffer1D->pMdfBuffer);

    if (cm_status != CM_SUCCESS) {
	dri_bo_unreference(pMdfBuffer1D->bo);
	pMdfBuffer1D->bo = NULL;
	goto allocation_fail;
    }
  
    dri_bo_map(pMdfBuffer1D->bo, 1);
    memset(pMdfBuffer1D->bo->virt, 0, buf_size);
    pMdfBuffer1D->pu16Buffer = (UINT16 *) pMdfBuffer1D->bo->virt;
    pMdfBuffer1D->bo_mapped = 1;

    return VA_STATUS_SUCCESS;

allocation_fail:
    return VA_STATUS_ERROR_ALLOCATION_FAILED;
    
}

static VAStatus
INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT64(
	VADriverContextP ctx,
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER *pMdfBuffer1D,
	int dwBufferSize)
{
    int buf_size, cm_status;
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    CmOsResource target_resource;

    buf_size = ALIGN((dwBufferSize) * sizeof(uint64_t), INTEL_HYBRID_VP9_PAGE_SIZE);

    pMdfBuffer1D->dwSize    = (dwBufferSize);

    pMdfBuffer1D->bo  = dri_bo_alloc(drv_ctx->drv_data.bufmgr, 
                                        "Buffer", 
                                        buf_size, 64);
   {
        struct drm_i915_gem_caching bo_cache;

        memset(&bo_cache, 0, sizeof(bo_cache));
        bo_cache.handle = pMdfBuffer1D->bo->handle;
        bo_cache.caching = I915_CACHING_CACHED;

        drmIoctl(drv_ctx->drv_data.fd, DRM_IOCTL_I915_GEM_SET_CACHING, &bo_cache);
    }

    /* Disable reuse this bo which set I915_CACHING_CACHED on CHV/BSW without LLC.
     * Otherwise if it was reused and GTT mapped later, SIGBUS when access the GTT virtual addr.
     */
    if (IS_CHERRYVIEW(drv_ctx->drv_data.device_id)) {
        drm_intel_bo_disable_reuse(pMdfBuffer1D->bo);
    }

    pMdfBuffer1D->bo_mapped = 0;
    
    pMdfBuffer1D->pu64Buffer = NULL;

    memset(&target_resource, 0, sizeof(target_resource));
    GetCmOsResourceFor1DBuffer(&target_resource, pMdfBuffer1D, pMdfBuffer1D->bo, 8);

    cm_status = pMdfDevice->CreateBuffer(&target_resource,pMdfBuffer1D->pMdfBuffer);

    if (cm_status != CM_SUCCESS) {
	dri_bo_unreference(pMdfBuffer1D->bo);
	pMdfBuffer1D->bo = NULL;
	goto allocation_fail;
    }
  
    dri_bo_map(pMdfBuffer1D->bo, 1);
    memset(pMdfBuffer1D->bo->virt, 0, buf_size);
    pMdfBuffer1D->pu64Buffer = (UINT64 *)pMdfBuffer1D->bo->virt;
    pMdfBuffer1D->bo_mapped = 1;

    return VA_STATUS_SUCCESS;

allocation_fail:
    return VA_STATUS_ERROR_ALLOCATION_FAILED;
}


static void INTEL_HYBRID_VP9_DESTROY_MDF_2D_BUFFER(
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER *pMdfBuffer2D)
{
    if (pMdfBuffer2D->pMdfSurface)
    {
        pMdfDevice->DestroySurface(pMdfBuffer2D->pMdfSurface);
	pMdfBuffer2D->pMdfSurface = NULL;
    }
}

static void INTEL_HYBRID_VP9_DESTROY_MDF_2DUP_BUFFER(
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER *pMdfBuffer2D)
{

    if (pMdfBuffer2D->pMdfSurface)
    {
        pMdfDevice->DestroySurface(pMdfBuffer2D->pMdfSurface);
	pMdfBuffer2D->pMdfSurface = NULL;
    }

    if (pMdfBuffer2D->bo)
    {
        if (pMdfBuffer2D->pBuffer)
		dri_bo_unmap(pMdfBuffer2D->bo);
	dri_bo_unreference(pMdfBuffer2D->bo);
	pMdfBuffer2D->pBuffer = NULL;
	pMdfBuffer2D->bo = NULL;
    }

}


static void INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(
	CmDevice    *pMdfDevice,
	INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER *pMdfBuffer1D)
{
    if (pMdfBuffer1D->pMdfBuffer)
    {
	pMdfDevice->DestroySurface(pMdfBuffer1D->pMdfBuffer);
	pMdfBuffer1D->pMdfBuffer = NULL;
    }

    if (pMdfBuffer1D->bo)
    {
        if (pMdfBuffer1D->bo_mapped)
		dri_bo_unmap(pMdfBuffer1D->bo);
	pMdfBuffer1D->bo_mapped = 0;
	pMdfBuffer1D->pBuffer = NULL;
	dri_bo_unreference(pMdfBuffer1D->bo);
	pMdfBuffer1D->bo = NULL;
    }
}

#endif

void Intel_HybridVp9Decode_ConstructCombinedFilters(PVOID pCombinedFilters)
{
    int8_t  *pi8CombinedFilters;
    int16_t *pi16Filters;
    int    i;

    // 8 Tap Filters
    pi16Filters = (int16_t *)&g_Filters8Tap;
    pi8CombinedFilters = (int8_t *)pCombinedFilters;
    for (i = 0; i < 8 * 16; i++)
    {
        pi8CombinedFilters[i] = (int8_t)((*pi16Filters++) - 1);
    }

    // 8 Tap Smooth Filters
    pi16Filters =(int16_t *)&g_Filters8TapSmooth;
    pi8CombinedFilters += 8 * 16;
    for (i = 0; i < 8 * 16; i++)
    {
        pi8CombinedFilters[i] = (int8_t)((*pi16Filters++) - 1);
    }

    // 8 Tap Sharp Filters
    pi16Filters = (int16_t *)&g_Filters8TapSharp;
    pi8CombinedFilters += 8 * 16;
    for (i = 0; i < 8 * 16; i++)
    {
        pi8CombinedFilters[i] = (int8_t)((*pi16Filters++) - 1);
    }

    // Bilinear Filters
    pi16Filters = (int16_t *)&g_FiltersBilinear;
    pi8CombinedFilters += 8 * 16;
    for (i = 0; i < 8 * 16; i++)
    {
        pi8CombinedFilters[i] = (int8_t)((*pi16Filters++) - 1);
    }
}

void Intel_HybridVp9Decode_SetHostBuffers(
    PINTEL_DECODE_HYBRID_VP9_STATE pHybridVp9State, 
    UINT                              uiIndex)
{
    PINTEL_DECODE_HYBRID_VP9_MDF_BUFFER  pMdfDecodeBuffer;
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER     pHostVldOutputBuf;

    pHostVldOutputBuf   = pHybridVp9State->pHostVldOutputBuf + uiIndex;
    pMdfDecodeBuffer    = &pHybridVp9State->MdfDecodeEngine.pMdfDecodeFrame[uiIndex].MdfDecodeBuffer;

    pHostVldOutputBuf->TransformCoeff[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu16Buffer  = 
        pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pu16Buffer;
    pHostVldOutputBuf->TransformCoeff[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].dwSize      = 
        pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize;
    pHostVldOutputBuf->TransformCoeff[INTEL_HOSTVLD_VP9_YUV_PLANE_U].pu16Buffer  = 
        pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_U].pu16Buffer;
    pHostVldOutputBuf->TransformCoeff[INTEL_HOSTVLD_VP9_YUV_PLANE_U].dwSize      = 
        pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_U].dwSize;
    pHostVldOutputBuf->TransformCoeff[INTEL_HOSTVLD_VP9_YUV_PLANE_V].pu16Buffer  = 
        pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_V].pu16Buffer;
    pHostVldOutputBuf->TransformCoeff[INTEL_HOSTVLD_VP9_YUV_PLANE_V].dwSize      = 
        pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_V].dwSize;

    pHostVldOutputBuf->TransformSize[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer    = 
        pMdfDecodeBuffer->TransformSize[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pu8Buffer;
    pHostVldOutputBuf->TransformSize[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].dwSize       = 
        pMdfDecodeBuffer->TransformSize[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize;
    pHostVldOutputBuf->TransformSize[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer   = 
        pMdfDecodeBuffer->TransformSize[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pu8Buffer;
    pHostVldOutputBuf->TransformSize[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].dwSize      = 
        pMdfDecodeBuffer->TransformSize[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwSize;

    pHostVldOutputBuf->CoeffStatus[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer   = 
        pMdfDecodeBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pu8Buffer;
    pHostVldOutputBuf->CoeffStatus[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].dwSize      = 
        pMdfDecodeBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize;
    pHostVldOutputBuf->CoeffStatus[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer  = 
        pMdfDecodeBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pu8Buffer;
    pHostVldOutputBuf->CoeffStatus[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].dwSize     = 
        pMdfDecodeBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwSize;

    pHostVldOutputBuf->PredictionMode[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer   = 
        pMdfDecodeBuffer->PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pu8Buffer;
    pHostVldOutputBuf->PredictionMode[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].dwSize      = 
        pMdfDecodeBuffer->PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize;
    pHostVldOutputBuf->PredictionMode[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer  = 
        pMdfDecodeBuffer->PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pu8Buffer;
    pHostVldOutputBuf->PredictionMode[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].dwSize     = 
        pMdfDecodeBuffer->PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwSize;

    pHostVldOutputBuf->QP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer   = 
        pMdfDecodeBuffer->QP[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pu8Buffer;
    pHostVldOutputBuf->QP[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].dwSize      = 
        pMdfDecodeBuffer->QP[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize;
    pHostVldOutputBuf->QP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer  = 
        pMdfDecodeBuffer->QP[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pu8Buffer;
    pHostVldOutputBuf->QP[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].dwSize     = 
        pMdfDecodeBuffer->QP[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwSize;

    pHostVldOutputBuf->VerticalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer   = 
        pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pu8Buffer;
    pHostVldOutputBuf->VerticalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].dwPitch     = 
        pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwPitch;
    pHostVldOutputBuf->VerticalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].dwSize      = 
        pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize;
    pHostVldOutputBuf->VerticalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer  = 
        pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pu8Buffer;
    pHostVldOutputBuf->VerticalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].dwPitch    = 
        pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwPitch;
    pHostVldOutputBuf->VerticalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].dwSize     = 
        pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwSize;

    pHostVldOutputBuf->HorizontalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].pu8Buffer   = 
        pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pu8Buffer;
    pHostVldOutputBuf->HorizontalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].dwPitch     = 
        pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwPitch;
    pHostVldOutputBuf->HorizontalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_Y].dwSize      = 
        pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize;
    pHostVldOutputBuf->HorizontalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].pu8Buffer  = 
        pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pu8Buffer;
    pHostVldOutputBuf->HorizontalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].dwPitch    = 
        pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwPitch;
    pHostVldOutputBuf->HorizontalEdgeMask[INTEL_HOSTVLD_VP9_YUV_PLANE_UV].dwSize     = 
        pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwSize;

    pHostVldOutputBuf->TransformType.pu8Buffer      = pMdfDecodeBuffer->TransformType.pu8Buffer;
    pHostVldOutputBuf->TransformType.dwSize         = pMdfDecodeBuffer->TransformType.dwSize;
    pHostVldOutputBuf->TileIndex.pu8Buffer          = pMdfDecodeBuffer->TileIndex.pu8Buffer;
    pHostVldOutputBuf->TileIndex.dwSize             = pMdfDecodeBuffer->TileIndex.dwSize;
    pHostVldOutputBuf->BlockSize.pu8Buffer          = pMdfDecodeBuffer->BlockSize.pu8Buffer;
    pHostVldOutputBuf->BlockSize.dwSize             = pMdfDecodeBuffer->BlockSize.dwSize;
    pHostVldOutputBuf->ReferenceFrame.pu8Buffer     = pMdfDecodeBuffer->ReferenceFrame.pu8Buffer;
    pHostVldOutputBuf->ReferenceFrame.dwSize        = pMdfDecodeBuffer->ReferenceFrame.dwSize;
    pHostVldOutputBuf->FilterType.pu8Buffer         = pMdfDecodeBuffer->FilterType.pu8Buffer;
    pHostVldOutputBuf->FilterType.dwSize            = pMdfDecodeBuffer->FilterType.dwSize;
    pHostVldOutputBuf->MotionVector.pu8Buffer       = pMdfDecodeBuffer->MotionVector.pu8Buffer;
    pHostVldOutputBuf->MotionVector.dwSize          = pMdfDecodeBuffer->MotionVector.dwSize;
    pHostVldOutputBuf->FilterLevel.pu8Buffer        = pMdfDecodeBuffer->FilterLevel.pu8Buffer;
    pHostVldOutputBuf->FilterLevel.dwPitch          = pMdfDecodeBuffer->FilterLevel.dwPitch;
    pHostVldOutputBuf->FilterLevel.dwSize           = pMdfDecodeBuffer->FilterLevel.dwSize;
    pHostVldOutputBuf->Threshold.pu8Buffer          = pMdfDecodeBuffer->Threshold.pu8Buffer;
    pHostVldOutputBuf->Threshold.dwPitch            = pMdfDecodeBuffer->Threshold.dwPitch;
    pHostVldOutputBuf->Threshold.dwSize             = pMdfDecodeBuffer->Threshold.dwSize;
}

VAStatus Intel_HybridVp9Decode_MdfHost_SetKernelThreadCount (
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine, 
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame)
{
    DWORD       dwIntraPredKernelModeLuma, dwIntraPredKernelModeChroma;
    VAStatus  eStatus = VA_STATUS_SUCCESS;


    // IQ/IT kernels
    pMdfDecodeEngine->pKernelIqIt[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]->SetThreadCount(
        pMdfDecodeFrame->dwWidthB32 * pMdfDecodeFrame->dwHeightB32);
    pMdfDecodeEngine->pKernelIqIt[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]->SetThreadCount(
        pMdfDecodeFrame->dwWidthB32 * pMdfDecodeFrame->dwHeightB32);
    pMdfDecodeEngine->pKernelIqItLossless[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]->SetThreadCount(
        pMdfDecodeFrame->dwWidthB32 * pMdfDecodeFrame->dwHeightB32);
    pMdfDecodeEngine->pKernelIqItLossless[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]->SetThreadCount(
        pMdfDecodeFrame->dwWidthB32 * pMdfDecodeFrame->dwHeightB32);

    // Intra Prediction kernels and Zero Fill Thread Status kernel
    dwIntraPredKernelModeLuma   = pMdfDecodeEngine->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y];
    dwIntraPredKernelModeChroma = pMdfDecodeEngine->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV];

    switch (dwIntraPredKernelModeLuma)
    {
    case INTEL_HYBRID_VP9_MDF_INTRAPRED_8x8:
        pMdfDecodeEngine->pKernelIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]->SetThreadCount(
            pMdfDecodeFrame->dwWidthB8 * pMdfDecodeFrame->dwHeightB8);
        pMdfDecodeEngine->pKernelZeroFill[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]->SetThreadCount(
            pMdfDecodeFrame->dwWidthB8 * pMdfDecodeFrame->dwHeightB8);
        break;
    case INTEL_HYBRID_VP9_MDF_INTRAPRED_16x16:
        pMdfDecodeEngine->pKernelIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]->SetThreadCount(
            pMdfDecodeFrame->dwWidthB16 * pMdfDecodeFrame->dwHeightB16);
        pMdfDecodeEngine->pKernelZeroFill[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]->SetThreadCount(
            pMdfDecodeFrame->dwWidthB16 * pMdfDecodeFrame->dwHeightB16);
        break;
    case INTEL_HYBRID_VP9_MDF_INTRAPRED_32x32:
        pMdfDecodeEngine->pKernelIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]->SetThreadCount(
            pMdfDecodeFrame->dwWidthB32 * pMdfDecodeFrame->dwHeightB32);
        break;
    default:
        eStatus = VA_STATUS_ERROR_UNKNOWN;
        goto finish;
    }

    switch (dwIntraPredKernelModeChroma)
    {
    case INTEL_HYBRID_VP9_MDF_INTRAPRED_8x8:
        pMdfDecodeEngine->pKernelIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]->SetThreadCount(
            pMdfDecodeFrame->dwWidthB16 * pMdfDecodeFrame->dwHeightB16);
        pMdfDecodeEngine->pKernelZeroFill[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]->SetThreadCount(
            pMdfDecodeFrame->dwWidthB16 * pMdfDecodeFrame->dwHeightB16);
        break;
    case INTEL_HYBRID_VP9_MDF_INTRAPRED_16x16:
        pMdfDecodeEngine->pKernelIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]->SetThreadCount(
            pMdfDecodeFrame->dwWidthB32 * pMdfDecodeFrame->dwHeightB32);
        break;
    case INTEL_HYBRID_VP9_MDF_INTRAPRED_32x32:
        pMdfDecodeEngine->pKernelIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]->SetThreadCount(
            pMdfDecodeFrame->dwWidthB64 * pMdfDecodeFrame->dwHeightB64);
        break;
    default:
        eStatus = VA_STATUS_ERROR_UNKNOWN;
        goto finish;
    }

    // Inter Prediction kernels
    pMdfDecodeEngine->pKernelInter->SetThreadCount(
        pMdfDecodeFrame->dwWidthB16 * pMdfDecodeFrame->dwHeightB16);

    // Inter Prediction Scaling kernels
    pMdfDecodeEngine->pKernelInterScaling->SetThreadCount(
        pMdfDecodeFrame->dwWidthB16 * pMdfDecodeFrame->dwHeightB16);

    // Deblocking kernels
    pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_TOP]->SetThreadCount(
        pMdfDecodeEngine->dwLumaDeblockThreadWidth * pMdfDecodeEngine->dwLumaDeblockThreadHeight);
    pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_TOP]->SetThreadCount(
        pMdfDecodeEngine->dwChromaDeblockThreadWidth * pMdfDecodeEngine->dwChromaDeblockThreadHeight);

    // To support true 4K cases (4096 width or height), we need other deblocking kernels and thread spaces for blocks outside of 4080x4088
    if (pMdfDecodeFrame->dwWidth > 4080)
    {
        // Need deblocking kernel for right top block
        pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_TOP]->SetThreadCount(
            (ALIGN(pMdfDecodeFrame->dwWidthB8, 2) - 503 + 7) * pMdfDecodeEngine->dwLumaDeblockThreadHeight);

        // Also need deblocking kernel for right region of chroma
        pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV][INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_TOP]->SetThreadCount(
            (ALIGN(pMdfDecodeFrame->dwWidthB8, 2) - 503 + 7) * pMdfDecodeEngine->dwChromaDeblockThreadHeight);

        if (pMdfDecodeFrame->dwHeight > 4088)
        {
            // Need deblocking kernel for left bottom region and right bottom region out of 4080x4088 bound
            pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_BOTTOM]->SetThreadCount(
                pMdfDecodeEngine->dwLumaDeblockThreadWidth * (pMdfDecodeFrame->dwHeightB8 - 504));

            pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_BOTTOM]->SetThreadCount(
                (ALIGN(pMdfDecodeFrame->dwWidthB8, 2) - 503 + 7) * (pMdfDecodeFrame->dwHeightB8 - 504));
        }
    }
    else if (pMdfDecodeFrame->dwHeight > 4088)
    {
        // Need deblocking kernel for  left bottom region out of 4080x4088 bound
        pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_BOTTOM]->SetThreadCount(
            pMdfDecodeEngine->dwLumaDeblockThreadWidth * (pMdfDecodeFrame->dwHeightB8 - 504));
    }

finish:
    return eStatus;
}

void Intel_HybridVp9Decode_MdfHost_PickDeblockKernel (
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine, 
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame)
{
    if ((pMdfDecodeFrame->dwWidthB8 << 1) > CM_MAX_THREAD_WIDTH_FOR_MW) // width > 2040
    {
        // use 16x8 deblocking
        pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_TOP] =
            pMdfDecodeEngine->pKernelDeblock16x8[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y];
        if (ALIGN(pMdfDecodeFrame->dwWidthB8, 2) > CM_MAX_THREAD_WIDTH_FOR_MW) // width > 4080
        {
            // Has right top region.
            // If heightB8 > CM_MAX_THREAD_WIDTH_FOR_MW, also has left bottom region and right bottom region.
            pMdfDecodeEngine->dwLumaDeblockThreadWidth  = 503;
            pMdfDecodeEngine->dwLumaDeblockThreadHeight =
                (pMdfDecodeFrame->dwHeightB8 > CM_MAX_THREAD_WIDTH_FOR_MW) ? 504 : pMdfDecodeFrame->dwHeightB8;
        }
        else if (pMdfDecodeFrame->dwHeightB8 > CM_MAX_THREAD_WIDTH_FOR_MW) // height > 4088
        {
            // Has left bottom region only
            pMdfDecodeEngine->dwLumaDeblockThreadWidth  = ALIGN(pMdfDecodeFrame->dwWidthB8, 2);
            pMdfDecodeEngine->dwLumaDeblockThreadHeight = 504;

        }
        else // width <= 4080 and height <= 4088. Thread space width and height do not exceed HW limitation.
        {
            // Normal case
            pMdfDecodeEngine->dwLumaDeblockThreadWidth = ALIGN(pMdfDecodeFrame->dwWidthB8, 2);
            pMdfDecodeEngine->dwLumaDeblockThreadHeight = pMdfDecodeFrame->dwHeightB8;
        }
        pMdfDecodeEngine->dwLumaDeblock26ZIMBWidth  = 8;
    }
    else
    {
        // use 8x8 deblocking
        pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_TOP] =
            pMdfDecodeEngine->pKernelDeblock8x8[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y];
       pMdfDecodeEngine->dwLumaDeblockThreadWidth   = pMdfDecodeFrame->dwWidthB8 << 1;
       pMdfDecodeEngine->dwLumaDeblockThreadHeight  = pMdfDecodeFrame->dwHeightB8;
       pMdfDecodeEngine->dwLumaDeblock26ZIMBWidth   = 16;
    }

    pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_TOP] =
        pMdfDecodeEngine->pKernelDeblock8x8[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV];
    pMdfDecodeEngine->dwChromaDeblockThreadWidth    =
        (ALIGN(pMdfDecodeFrame->dwWidthB8, 2) > CM_MAX_THREAD_WIDTH_FOR_MW) ?
        503 : ALIGN(pMdfDecodeFrame->dwWidthB8, 2);
    pMdfDecodeEngine->dwChromaDeblockThreadHeight   = (pMdfDecodeFrame->dwHeightB8 + 1) >> 1;
}

VAStatus Intel_HybridVp9Decode_MdfHost_CreateKernels (
    VADriverContextP ctx, 
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);

    CmDevice    *pMdfDevice;
    CmProgram   *pProgram;
    uint32_t     dwIntraPredKernelModeLuma, dwIntraPredKernelModeChroma;
    VAStatus  eStatus = VA_STATUS_SUCCESS;

    const char  *IntraPredKernelsLuma[] = {
        MDF_KERNEL_FUNCTION_INTRA_PRED_8X8_LUMA,
        MDF_KERNEL_FUNCTION_INTRA_PRED_16X16_LUMA,
        MDF_KERNEL_FUNCTION_INTRA_PRED_32X32_LUMA };
    const char  *IntraPredKernelsChroma[] = {
        MDF_KERNEL_FUNCTION_INTRA_PRED_8X8_CHROMA,
        MDF_KERNEL_FUNCTION_INTRA_PRED_16X16_CHROMA,
        MDF_KERNEL_FUNCTION_INTRA_PRED_32X32_CHROMA };

    pMdfDevice = pMdfDecodeEngine->pMdfDevice;

    dwIntraPredKernelModeLuma   = pMdfDecodeEngine->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y];
    dwIntraPredKernelModeChroma = pMdfDecodeEngine->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV];

    // IQ/IT kernels
    if (IS_HASWELL(drv_ctx->drv_data.device_id))
    {
	INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9Transform_g75, Vp9Transform_g75_size, pProgram, "-nojitter"));
    }
    else if (IS_BROADWELL(drv_ctx->drv_data.device_id))
    {
	INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9Transform_g8, Vp9Transform_g8_size, pProgram, "-nojitter"));
    }
    else if (IS_CHERRYVIEW(drv_ctx->drv_data.device_id))
    {
	INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9Transform_g8lp, Vp9Transform_g8lp_size, pProgram, "-nojitter"));
    }
    else if (IS_SKYLAKE(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9Transform_g9, Vp9Transform_g9_size, pProgram, "-nojitter"));
    }
    else
    {
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_IQIT_LUMA, 
        pMdfDecodeEngine->pKernelIqIt[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]));
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_IQIT_CHROMA, 
        pMdfDecodeEngine->pKernelIqIt[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]));
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_IQIT_LOSSLESS_LUMA, 
        pMdfDecodeEngine->pKernelIqItLossless[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]));
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_IQIT_LOSSLESS_CHROMA, 
        pMdfDecodeEngine->pKernelIqItLossless[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]));

    // Intra Prediction kernels
    if (IS_HASWELL(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9IntraPred_g75, Vp9IntraPred_g75_size, pProgram, "-nojitter"));
    }
    else if (IS_BROADWELL(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9IntraPred_g8, Vp9IntraPred_g8_size, pProgram, "-nojitter"));
    }
    else if (IS_CHERRYVIEW(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9IntraPred_g8lp, Vp9IntraPred_g8lp_size, pProgram, "-nojitter"));
    }
    else if (IS_SKYLAKE(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9IntraPred_g9, Vp9IntraPred_g9_size, pProgram, "-nojitter"));
    }
    else
    {
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, IntraPredKernelsLuma[dwIntraPredKernelModeLuma],
        pMdfDecodeEngine->pKernelIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]));
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, IntraPredKernelsChroma[dwIntraPredKernelModeChroma],
        pMdfDecodeEngine->pKernelIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]));
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_ZERO_FILL,
        pMdfDecodeEngine->pKernelZeroFill[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]));
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_ZERO_FILL,
        pMdfDecodeEngine->pKernelZeroFill[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]));

    // Inter Prediction & Ref Padding kernels
    if (IS_HASWELL(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9InterPred_g75, Vp9InterPred_g75_size, pProgram, "-nojitter"));
    }
    else if (IS_BROADWELL(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9InterPred_g8, Vp9InterPred_g8_size, pProgram, "-nojitter"));
    }
    else if (IS_CHERRYVIEW(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9InterPred_g8lp, Vp9InterPred_g8lp_size, pProgram, "-nojitter"));
    }
    else if (IS_SKYLAKE(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9InterPred_g9, Vp9InterPred_g9_size, pProgram, "-nojitter"));
    }
    else
    {
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    } 

    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_INTER_PRED, 
        pMdfDecodeEngine->pKernelInter));

    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_REF_PADDING, 
        pMdfDecodeEngine->pKernelRefPadding));

    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_REF_Y_ONLY_PADDING,
        pMdfDecodeEngine->pKernelRefPaddingYOnly));

    // Inter Prediction with Scaling kernel
    if (IS_HASWELL(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9InterPredScaling_g75, Vp9InterPredScaling_g75_size, pProgram, "-nojitter"));
    }
    else if (IS_BROADWELL(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9InterPredScaling_g8, Vp9InterPredScaling_g8_size, pProgram, "-nojitter"));
    }
    else if (IS_CHERRYVIEW(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9InterPredScaling_g8lp, Vp9InterPredScaling_g8lp_size, pProgram, "-nojitter"));
    }
    else if (IS_SKYLAKE(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9InterPredScaling_g9, Vp9InterPredScaling_g9_size, pProgram, "-nojitter"));
    }
    else
    {
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_INTER_PRED_SCALING,
        pMdfDecodeEngine->pKernelInterScaling));

    // Deblocking kernels
    if (IS_HASWELL(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9Deblock_g75, Vp9Deblock_g75_size, pProgram, "-nojitter"));
    }
    else if (IS_BROADWELL(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9Deblock_g8, Vp9Deblock_g8_size, pProgram, "-nojitter"));
    }
    else if (IS_CHERRYVIEW(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9Deblock_g8lp, Vp9Deblock_g8lp_size, pProgram, "-nojitter"));
    }
    else if (IS_SKYLAKE(drv_ctx->drv_data.device_id))
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->LoadProgram(
            &Vp9Deblock_g9, Vp9Deblock_g9_size, pProgram, "-nojitter"));
    }
    else
    {
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_DEBLOCK_LUMA, 
        pMdfDecodeEngine->pKernelDeblock8x8[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]));
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_DEBLOCK16x8_LUMA, 
        pMdfDecodeEngine->pKernelDeblock16x8[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]));
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_DEBLOCK_CHROMA, 
        pMdfDecodeEngine->pKernelDeblock8x8[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]));

    // For true 4K cases (4096 width or height), we need other deblocking kernels for blocks outside of 4080x4088
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_DEBLOCK16x8_LUMA,
        pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_TOP]));
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_DEBLOCK16x8_LUMA,
        pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_BOTTOM]));
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_DEBLOCK16x8_LUMA,
        pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_BOTTOM]));
    // We only need right top kernel for chroma since 8x8 chroma deblocking already supports (8K - 16)
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateKernel(
        pProgram, MDF_KERNEL_FUNCTION_DEBLOCK_CHROMA,
        pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV][INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_TOP]));

finish:
    return eStatus;
}

VAStatus Intel_HybridVp9Decode_MdfHost_CreateThreadSpaces (
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine, 
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame, 
    CmDevice                                *pMdfDevice)
{
    DWORD       dwIntraPredKernelModeLuma, dwIntraPredKernelModeChroma;
    CmThreadSpace                *pThreadSpace;
    VAStatus  eStatus = VA_STATUS_SUCCESS;

    // IQ/IT thread space
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
        pMdfDecodeFrame->dwWidthB32, 
        pMdfDecodeFrame->dwHeightB32, 
        pMdfDecodeEngine->pThreadSpaceIqIt[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]));
    pMdfDecodeEngine->pThreadSpaceIqIt[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]->SelectThreadDependencyPattern(CM_NONE_DEPENDENCY);

    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
        pMdfDecodeFrame->dwWidthB32, 
        pMdfDecodeFrame->dwHeightB32, 
        pMdfDecodeEngine->pThreadSpaceIqIt[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]));
    pMdfDecodeEngine->pThreadSpaceIqIt[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]->SelectThreadDependencyPattern(CM_NONE_DEPENDENCY);

    // Intra prediction thread space
    dwIntraPredKernelModeLuma = pMdfDecodeEngine->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y];
    dwIntraPredKernelModeChroma = pMdfDecodeEngine->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV];

    switch (dwIntraPredKernelModeLuma)
    {
    case INTEL_HYBRID_VP9_MDF_INTRAPRED_8x8:
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
            pMdfDecodeFrame->dwWidthB8,
            pMdfDecodeFrame->dwHeightB8,
            pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]));
        pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]->SelectThreadDependencyPattern(CM_WAVEFRONT26Z);

        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
            pMdfDecodeFrame->dwWidthB8,
            pMdfDecodeFrame->dwHeightB8,
            pMdfDecodeEngine->pThreadSpaceZeroFill[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]));
        break;
    case INTEL_HYBRID_VP9_MDF_INTRAPRED_16x16:
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
            pMdfDecodeFrame->dwWidthB16,
            pMdfDecodeFrame->dwHeightB16,
            pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]));
        pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]->SelectThreadDependencyPattern(CM_WAVEFRONT26Z);

        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
            pMdfDecodeFrame->dwWidthB16,
            pMdfDecodeFrame->dwHeightB16,
            pMdfDecodeEngine->pThreadSpaceZeroFill[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]));
        break;
    case INTEL_HYBRID_VP9_MDF_INTRAPRED_32x32:
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
            pMdfDecodeFrame->dwWidthB32,
            pMdfDecodeFrame->dwHeightB32,
            pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]));
        pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]->SelectThreadDependencyPattern(
            pMdfDecodeFrame->dwWidthB32 == 1 ? CM_HORIZONTAL_DEPENDENCY : CM_WAVEFRONT26);
        break;
    default:
        eStatus = VA_STATUS_ERROR_UNKNOWN;
        goto finish;
    }

    pMdfDecodeEngine->pKernelIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]->AssociateThreadSpace(pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]);

    switch (dwIntraPredKernelModeChroma)
    {
    case INTEL_HYBRID_VP9_MDF_INTRAPRED_8x8:
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
            pMdfDecodeFrame->dwWidthB16,
            pMdfDecodeFrame->dwHeightB16,
            pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]));
        pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]->SelectThreadDependencyPattern(CM_WAVEFRONT26Z);

        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
            pMdfDecodeFrame->dwWidthB16,
            pMdfDecodeFrame->dwHeightB16,
            pMdfDecodeEngine->pThreadSpaceZeroFill[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]));
        break;
    case INTEL_HYBRID_VP9_MDF_INTRAPRED_16x16:
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
            pMdfDecodeFrame->dwWidthB32,
            pMdfDecodeFrame->dwHeightB32,
            pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]));
        pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]->SelectThreadDependencyPattern(
            pMdfDecodeFrame->dwWidthB32 == 1 ? CM_WAVEFRONT : CM_WAVEFRONT26Z);
        break;
    case INTEL_HYBRID_VP9_MDF_INTRAPRED_32x32:
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
            pMdfDecodeFrame->dwWidthB64,
            pMdfDecodeFrame->dwHeightB64,
            pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]));
        pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]->SelectThreadDependencyPattern(CM_WAVEFRONT);
        break;
    default:
        eStatus = VA_STATUS_ERROR_UNKNOWN;
        goto finish;
    }

    pMdfDecodeEngine->pKernelIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]->AssociateThreadSpace(pMdfDecodeEngine->pThreadSpaceIntra[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]);

    // Inter prediction thread space
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
        pMdfDecodeFrame->dwWidthB16, 
        pMdfDecodeFrame->dwHeightB16, 
        pMdfDecodeEngine->pThreadSpaceInter));

    // Loopfilter/Deblocking thread space
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
        pMdfDecodeEngine->dwLumaDeblockThreadWidth,
        pMdfDecodeEngine->dwLumaDeblockThreadHeight,
        pThreadSpace));
    pThreadSpace->SelectThreadDependencyPattern(CM_WAVEFRONT26ZI);
    pThreadSpace->Set26ZIDispatchPattern(VVERTICAL1X26_HHORIZONTAL1X26);
    pThreadSpace->Set26ZIMacroBlockSize(
        pMdfDecodeEngine->dwLumaDeblock26ZIMBWidth, 8);
    pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_TOP]->AssociateThreadSpace(pThreadSpace);
    pMdfDecodeEngine->pThreadSpaceDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_TOP] = pThreadSpace;
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
        pMdfDecodeEngine->dwChromaDeblockThreadWidth,
        pMdfDecodeEngine->dwChromaDeblockThreadHeight,
        pThreadSpace));
    pThreadSpace->SelectThreadDependencyPattern(CM_WAVEFRONT26ZI);
    pThreadSpace->Set26ZIDispatchPattern(VVERTICAL1X26_HHORIZONTAL1X26);
    pThreadSpace->Set26ZIMacroBlockSize(8, 4);
    pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_TOP]->AssociateThreadSpace(pThreadSpace);
    pMdfDecodeEngine->pThreadSpaceDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_TOP] = pThreadSpace;

    // To support true 4K cases (4096 width or height), we need other deblocking kernels and thread spaces for blocks outside of 4080x4088
    if (pMdfDecodeFrame->dwWidth > 4080)
    {
        // Need thread space for the deblocking kernel for right top block
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
            ALIGN(pMdfDecodeFrame->dwWidthB8, 2) - 503 + 7, // 16 x dwLumaDeblockThreadWidth first 7 threads in a row do nothing
            pMdfDecodeEngine->dwLumaDeblockThreadHeight,
            pThreadSpace));
        pThreadSpace->SelectThreadDependencyPattern(CM_WAVEFRONT26ZI);
        pThreadSpace->Set26ZIDispatchPattern(VVERTICAL1X26_HHORIZONTAL1X26);
        pThreadSpace->Set26ZIMacroBlockSize(8, 8);
        pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_TOP]->AssociateThreadSpace(pThreadSpace);
        pMdfDecodeEngine->pThreadSpaceDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_TOP] = pThreadSpace;

        // Also need thread space for right region of chroma
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
            ALIGN(pMdfDecodeFrame->dwWidthB8, 2) - 503 + 7, // 16 x dwLumaDeblockThreadWidth first 7 threads in a row do nothing
            pMdfDecodeEngine->dwChromaDeblockThreadHeight,
            pThreadSpace));
        pThreadSpace->SelectThreadDependencyPattern(CM_WAVEFRONT26ZI);
        pThreadSpace->Set26ZIDispatchPattern(VVERTICAL1X26_HHORIZONTAL1X26);
        pThreadSpace->Set26ZIMacroBlockSize(8, 4);
        pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV][INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_TOP]->AssociateThreadSpace(pThreadSpace);
        pMdfDecodeEngine->pThreadSpaceDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV][INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_TOP] = pThreadSpace;

        if (pMdfDecodeFrame->dwHeight > 4088)
        {
            // Need thread space for the deblocking kernels for left bottom region and right bottom region out of 4080x4088 bound
           INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
                pMdfDecodeEngine->dwLumaDeblockThreadWidth,
                pMdfDecodeFrame->dwHeightB8 - 504,
                pThreadSpace));
            pThreadSpace->SelectThreadDependencyPattern(CM_WAVEFRONT26ZI);
            pThreadSpace->Set26ZIDispatchPattern(VVERTICAL1X26_HHORIZONTAL1X26);
            pThreadSpace->Set26ZIMacroBlockSize(8, 8);
            pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_BOTTOM]->AssociateThreadSpace(pThreadSpace);
            pMdfDecodeEngine->pThreadSpaceDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_BOTTOM] = pThreadSpace;

            INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
                ALIGN(pMdfDecodeFrame->dwWidthB8, 2) - 503 + 7, // 16 x dwLumaDeblockThreadWidth first 7 threads in a row do nothing
                pMdfDecodeFrame->dwHeightB8 - 504,
                pThreadSpace));
            pThreadSpace->SelectThreadDependencyPattern(CM_WAVEFRONT26ZI);
            pThreadSpace->Set26ZIDispatchPattern(VVERTICAL1X26_HHORIZONTAL1X26);
            pThreadSpace->Set26ZIMacroBlockSize(8, 8);
            pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_BOTTOM]->AssociateThreadSpace(pThreadSpace);
            pMdfDecodeEngine->pThreadSpaceDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_BOTTOM] = pThreadSpace;
        }
    }
    else if (pMdfDecodeFrame->dwHeight > 4088)
    {
        // Need thread space for the deblocking kernels for left bottom region out of 4080x4088 bound
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateThreadSpace(
            pMdfDecodeEngine->dwLumaDeblockThreadWidth,
            pMdfDecodeFrame->dwHeightB8 - 504,
            pThreadSpace));
        pThreadSpace->SelectThreadDependencyPattern(CM_WAVEFRONT26ZI);
        pThreadSpace->Set26ZIDispatchPattern(VVERTICAL1X26_HHORIZONTAL1X26);
        pThreadSpace->Set26ZIMacroBlockSize(8, 8);
        pMdfDecodeEngine->pKernelDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_BOTTOM]->AssociateThreadSpace(pThreadSpace);
        pMdfDecodeEngine->pThreadSpaceDeblock[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y][INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_BOTTOM] = pThreadSpace;
    }

finish:
    return eStatus;
}

VAStatus Intel_HybridVp9Decode_MdfHost_DestroyThreadSpaces (
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine,
    CmDevice                                *pMdfDevice)
{
    INT         i, j;
    VAStatus  eStatus = VA_STATUS_SUCCESS;

    for (i = INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y; i < INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER; i++)
    {
        INTEL_DECODE_HYBRID_VP9_DESTROY_THREADSPACE(pMdfDecodeEngine->pThreadSpaceIqIt[i]);
        INTEL_DECODE_HYBRID_VP9_DESTROY_THREADSPACE(pMdfDecodeEngine->pThreadSpaceIntra[i]);
        for (j = INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_TOP; j < INTEL_HYBRID_VP9_MDF_DEBLOCK_REGIONS; j++)
        {
            INTEL_DECODE_HYBRID_VP9_DESTROY_THREADSPACE(pMdfDecodeEngine->pThreadSpaceDeblock[i][j]);
        }
        INTEL_DECODE_HYBRID_VP9_DESTROY_THREADSPACE(pMdfDecodeEngine->pThreadSpaceZeroFill[i]);
    }

    INTEL_DECODE_HYBRID_VP9_DESTROY_THREADSPACE(pMdfDecodeEngine->pThreadSpaceInter);

    return eStatus;
}

VAStatus Intel_HybridVp9Decode_MdfHost_Allocate( 
    PINTEL_DECODE_HYBRID_VP9_STATE   pHybridVp9State,
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame, 
    CmDevice                                *pMdfDevice,
    uint32_t                                   dwFlags)
{
    PINTEL_DECODE_HYBRID_VP9_MDF_BUFFER      pMdfDecodeBuffer;
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER   pMdfBuffer2D;
    DWORD                                       dwAlignedWidth;     // SB64 aligned Width
    DWORD                                       dwAlignedHeight;    // SB64 aligned Height
    DWORD                                       dwWidthB8;
    DWORD                                       dwHeightB8;
    VAStatus                                  eStatus = VA_STATUS_SUCCESS;
    VADriverContextP		ctx = (VADriverContextP) (pHybridVp9State->driver_context);

    pMdfDecodeBuffer = &pMdfDecodeFrame->MdfDecodeBuffer;
    dwAlignedWidth   = pMdfDecodeFrame->dwAlignedWidth;
    dwAlignedHeight  = pMdfDecodeFrame->dwAlignedHeight;
    dwWidthB8        = pMdfDecodeFrame->dwWidthB8;
    dwHeightB8       = pMdfDecodeFrame->dwHeightB8;

    // Create surfaces of current frame
    if (dwFlags & VP9_HYBRID_DECODE_CURRENT_FRAME)
    {
       // Thread info surface for intra prediction - Luma (2D uint32 per 16x16 or 8x8; scrach buffer for kernel)
       if (pMdfDecodeFrame->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y] <= INTEL_HYBRID_VP9_MDF_INTRAPRED_16x16)
       {
           unsigned int   uiShift = pMdfDecodeFrame->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y] + 3;

           pMdfBuffer2D                    = &pMdfDecodeBuffer->ThreadInfo[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y];
           pMdfBuffer2D->dwWidth           = dwAlignedWidth >> uiShift;
           pMdfBuffer2D->dwHeight          = dwAlignedHeight >> uiShift;
           INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->GetSurface2DInfo(                           
           pMdfBuffer2D->dwWidth,          
           pMdfBuffer2D->dwHeight,         
           VA_CM_FMT_R32F,       
           (UINT &)pMdfBuffer2D->dwPitch,  
           (UINT &)pMdfBuffer2D->dwSize)); 
           INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateSurface2D(
           pMdfBuffer2D->dwWidth, 
           pMdfBuffer2D->dwHeight,
           VA_CM_FMT_R32F,
           pMdfBuffer2D->pMdfSurface));
       }

       // Thread info surface for intra prediction - Chroma (2D uint32 per 8x8; scrach buffer for kernel)
        if (pMdfDecodeFrame->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV] == INTEL_HYBRID_VP9_MDF_INTRAPRED_8x8)
        {
           pMdfBuffer2D                    = &pMdfDecodeBuffer->ThreadInfo[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV];
           pMdfBuffer2D->dwWidth           = dwAlignedWidth >> 3;
           pMdfBuffer2D->dwHeight          = dwAlignedHeight >> 3;
           INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->GetSurface2DInfo(                           
           pMdfBuffer2D->dwWidth,          
           pMdfBuffer2D->dwHeight,         
           VA_CM_FMT_R32F,       
           (UINT &)pMdfBuffer2D->dwPitch,  
           (UINT &)pMdfBuffer2D->dwSize)); 
           INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateSurface2D(
           pMdfBuffer2D->dwWidth, 
           pMdfBuffer2D->dwHeight,
           VA_CM_FMT_R32F,
           pMdfBuffer2D->pMdfSurface));
        }
        
        // transform coefficient - Luma (1D uint16 per pixel; Packed in Z-order)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT16(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y], 
            dwAlignedWidth * dwAlignedHeight);

        // transform coefficient - Chroma Cb (1D uint16 per pixel; Packed in Z-order)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT16(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_U], 
            (dwAlignedWidth >> 1) * (dwAlignedHeight >> 1));

        // transform coefficient - Chroma Cr (1D uint16 per pixel; Packed in Z-order)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT16(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_V], 
            (dwAlignedWidth >> 1) * (dwAlignedHeight >> 1));

        // transform size - Luma (uint8 per 8x8; Packed in Z-order)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->TransformSize[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y], 
            (dwAlignedWidth >> 3) * (dwAlignedHeight >> 3));

        // transform size - Chroma (uint8 per 4x4; Packed in Z-order, shared U & V)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->TransformSize[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV], 
            (dwAlignedWidth >> 3) * (dwAlignedHeight >> 3));

        // coefficient status flag - Luma (uint8 per 4x4; Packed in Z-order)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y], 
            (dwAlignedWidth >> 2) * (dwAlignedHeight >> 2));

        // coefficient status flag - Chroma (uint8 per 4x4; Packed in Z-order, packed U & V)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV], 
            (dwAlignedWidth >> 3) * (dwAlignedHeight >> 3));

        // QP - Luma (2 * uint16 per 8x8; Packed in Z-order)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT16(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->QP[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y], 
            (dwAlignedWidth >> 3) * (dwAlignedHeight >> 3) * 2);

        // QP - Chroma (2 * uint16 per 4x4; Packed in Z-order, packed U & V)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT16(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->QP[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV], 
            (dwAlignedWidth >> 3) * (dwAlignedHeight >> 3) * 2);

        // transform type - Luma (uint8 per 4x4; Packed in Z-order)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->TransformType, 
            (dwAlignedWidth >> 2) * (dwAlignedHeight >> 2));

        // Tile Index (uint8 per 32-pixel width column + 2; shared Y, U & V)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->TileIndex, 
            (dwAlignedWidth >> 5) + 2);

        // Prediction mode flags - Luma (uint8 per 4x4; Packed in Z-order)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y], 
            (dwAlignedWidth >> 2) * (dwAlignedHeight >> 2));

        // Prediction mode flags - Chroma (uint8 per 4x4; Packed in Z-order)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV], 
            (dwAlignedWidth >> 3) * (dwAlignedHeight >> 3));

        // Block size (uint8 per 8x8; Packed in Z-order, shared Y, U & V)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->BlockSize, 
            (dwAlignedWidth >> 3) * (dwAlignedHeight >> 3));

        // Reference frame index (uint16 per 8x8; Packed in Z-order, shared Y, U & V)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT16(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->ReferenceFrame, 
            (dwAlignedWidth >> 3) * (dwAlignedHeight >> 3));

        // Interpolation filter type (uint8 per 8x8; Packed in Z-order, shared Y, U & V)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->FilterType, 
            (dwAlignedWidth >> 3) * (dwAlignedHeight >> 3));

        // Motion vector (uint8 per 4x4; Packed in Z-order, shared Y, U & V)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT64(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->MotionVector, 
            (dwAlignedWidth >> 2) * (dwAlignedHeight >> 2));

        // Vertical edge mask - Luma (2D; uint8 per 16x8)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_2DUP_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y],
            (dwWidthB8 + 1) >> 1,
            dwHeightB8);

        // Vertical edge mask - Chroma (2D; uint8 per 16x8)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_2DUP_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV],
            (dwWidthB8 + 3) >> 2,
            (dwHeightB8 + 1) >> 1);

        // Horizontal edge mask - Luma (2D; uint8 per 16x8)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_2DUP_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y],
            (dwWidthB8 + 1) >> 1,
            dwHeightB8);

        // Horizontal edge mask - Chroma (2D; uint8 per 16x8)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_2DUP_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV],
            (dwWidthB8 + 3) >> 2,
            (dwHeightB8 + 1) >> 1);

        // filter level (2D; uint8 per 8x8, shared Y, U & V)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_2DUP_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->FilterLevel,
            dwWidthB8,
            dwHeightB8);

        // threshold (2D; 4 * 64, shared Y, U & V)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_2DUP_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->Threshold,
            4,
            64);

        // On-the-fly mask buffers for deblocking
        INTEL_HYBRID_VP9_ALLOCATE_MDF_2DUP_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->DeblockOntheFlyThreadMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y],
            dwAlignedWidth >> 2,
            dwAlignedHeight >> 3);
        INTEL_HYBRID_VP9_ALLOCATE_MDF_2DUP_BUFFER_UINT8(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->DeblockOntheFlyThreadMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV],
            dwAlignedWidth >> 3,
            dwAlignedHeight >> 4);
    }

    // Create previous frame/motion_vector for current frame
    if (dwFlags & VP9_HYBRID_DECODE_PREVIOUS_FRAME)
    {
        // Previous frame reference frame index (uint16 per 8x8; Packed in Z-order, shared Y, U & V)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT16(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->PrevReferenceFrame, 
            (dwAlignedWidth >> 3) * (dwAlignedHeight >> 3));

        // Previous frame motion vector (uint8 per 4x4; Packed in Z-order, shared Y, U & V)
        INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT64(
            ctx,
            pMdfDevice, 
            &pMdfDecodeBuffer->PrevMotionVector, 
            (dwAlignedWidth >> 2) * (dwAlignedHeight >> 2));
    }

finish:
    return eStatus;
}

VAStatus Intel_HybridVp9Decode_MdfHost_AllocateResidue (
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine,
    CmDevice                                *pMdfDevice,
    DWORD                                   dwAlignedWidth,
    DWORD                                   dwAlignedHeight)
{
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER   pMdfBuffer2D;
    VAStatus                                 eStatus = VA_STATUS_SUCCESS;

    // Residue surface - Luma (2D int16)
    pMdfBuffer2D                    = &pMdfDecodeEngine->Residue[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y];
    pMdfBuffer2D->dwWidth           = dwAlignedWidth;
    pMdfBuffer2D->dwHeight          = dwAlignedHeight;
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->GetSurface2DInfo(
        pMdfBuffer2D->dwWidth,
        pMdfBuffer2D->dwHeight,
        VA_CM_FMT_V8U8,
        (UINT &)pMdfBuffer2D->dwPitch,
        (UINT &)pMdfBuffer2D->dwSize));
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateSurface2D(
        pMdfBuffer2D->dwWidth,
        pMdfBuffer2D->dwHeight,
        VA_CM_FMT_V8U8,
        pMdfBuffer2D->pMdfSurface));

    // Residue surface - Chroma (2D int16)
    pMdfBuffer2D                    = &pMdfDecodeEngine->Residue[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV];
    pMdfBuffer2D->dwWidth           = dwAlignedWidth;
    pMdfBuffer2D->dwHeight          = dwAlignedHeight >> 1;
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->GetSurface2DInfo(
        pMdfBuffer2D->dwWidth,
        pMdfBuffer2D->dwHeight,
        VA_CM_FMT_V8U8,
        (UINT &)pMdfBuffer2D->dwPitch,
        (UINT &)pMdfBuffer2D->dwSize));
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateSurface2D(
        pMdfBuffer2D->dwWidth,
        pMdfBuffer2D->dwHeight,
        VA_CM_FMT_V8U8,
        pMdfBuffer2D->pMdfSurface));

finish:
    return eStatus;
}

VAStatus Intel_HybridVp9Decode_MdfHost_Create (
    VADriverContextP ctx, 
    PINTEL_DECODE_HYBRID_VP9_STATE   pHybridVp9State)
{
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine;
    CmDevice                                *pMdfDevice = NULL;
    unsigned int                                    i;
    VAStatus                              eStatus = VA_STATUS_SUCCESS;
    int				          cm_status;

    pMdfDecodeEngine    = &pHybridVp9State->MdfDecodeEngine;

    pMdfDecodeEngine->dwMdfBufferSize           = pHybridVp9State->dwMdfBufferSize;

    pMdfDecodeEngine->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y] =
		INTEL_HYBRID_VP9_MDF_INTRAPRED_16x16;
    /* It can also be set to INTRAPRED_8x8 or INTRAPRED_32x32 */
    /*
     * 
     * pMdfDecodeEngine->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y] =
		INTEL_HYBRID_VP9_MDF_INTRAPRED_8x8;
     * pMdfDecodeEngine->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y] =
		INTEL_HYBRID_VP9_MDF_INTRAPRED_32x32;
     */

    /* It is similar to INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y */
    pMdfDecodeEngine->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV] =
		INTEL_HYBRID_VP9_MDF_INTRAPRED_16x16;

   {
	UINT cm_version;
    	CmDriverContext driver_context;
        MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
	
	
	memset(&driver_context, 0, sizeof(driver_context));
	driver_context.bufmgr = drv_ctx->drv_data.bufmgr; 
	driver_context.deviceid = drv_ctx->drv_data.device_id;
	driver_context.device_rev = drv_ctx->drv_data.revision;
        driver_context.shared_bufmgr = 1;

	cm_version = CM_4_0;
        cm_status = CreateCmDevice(pMdfDevice, cm_version, &driver_context, CM_DEVICE_CREATE_OPTION_FOR_VP9);
        if (cm_status != CM_SUCCESS) {
                return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        pMdfDecodeEngine->pMdfDevice = pMdfDevice;
        pMdfDecodeEngine->iMdfDeviceTsc = __rdtsc();
    }

    eStatus = Intel_HybridVp9Decode_MdfHost_CreateKernels(ctx, pMdfDecodeEngine);

    if (eStatus != VA_STATUS_SUCCESS)
	goto finish;

    // allocate INTEL_DECODE_HYBRID_VP9_MDF_FRAME array
    pMdfDecodeEngine->pMdfDecodeFrame = 
        (PINTEL_DECODE_HYBRID_VP9_MDF_FRAME) calloc(pMdfDecodeEngine->dwMdfBufferSize,
					sizeof(*pMdfDecodeEngine->pMdfDecodeFrame));

    // Create MDF queues
    for (i = 0; i < pMdfDecodeEngine->dwMdfBufferSize; i++)
    {
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateQueue(pMdfDecodeEngine->pMdfDecodeFrame[i].pMdfQueue));
    }

    // Allocate and initialize combined filter coefficient buffer
    INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT16(ctx, pMdfDevice, &pMdfDecodeEngine->CombinedFilters, VP9_HYBRID_DECODE_COMBINED_FILETER_SIZE);
    Intel_HybridVp9Decode_ConstructCombinedFilters(pMdfDecodeEngine->CombinedFilters.pBuffer);

finish:
    return eStatus;
}

void Intel_HybridVp9Decode_MdfHost_Release (
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame, 
    CmDevice                                *pMdfDevice,
    uint32_t                                   dwFlags)
{
    PINTEL_DECODE_HYBRID_VP9_MDF_BUFFER      pMdfDecodeBuffer;

    pMdfDecodeBuffer = &pMdfDecodeFrame->MdfDecodeBuffer;

    // release surfaces of current frame
    if (dwFlags & VP9_HYBRID_DECODE_CURRENT_FRAME)
    {
        // hostVLD input surfaces
        if (pMdfDecodeFrame->pMdfEvent)
        {
            pMdfDecodeFrame->pMdfEvent->WaitForTaskFinished(5000);
        }

        // host surfaces
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_U]);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_V]);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->TransformSize[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->TransformSize[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->QP[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->QP[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]);
        INTEL_HYBRID_VP9_DESTROY_MDF_2DUP_BUFFER(pMdfDevice, &pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]);
        INTEL_HYBRID_VP9_DESTROY_MDF_2DUP_BUFFER(pMdfDevice, &pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]);
        INTEL_HYBRID_VP9_DESTROY_MDF_2DUP_BUFFER(pMdfDevice, &pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]);
        INTEL_HYBRID_VP9_DESTROY_MDF_2DUP_BUFFER(pMdfDevice, &pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]);
        INTEL_HYBRID_VP9_DESTROY_MDF_2DUP_BUFFER(pMdfDevice, &pMdfDecodeBuffer->DeblockOntheFlyThreadMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]);
        INTEL_HYBRID_VP9_DESTROY_MDF_2DUP_BUFFER(pMdfDevice, &pMdfDecodeBuffer->DeblockOntheFlyThreadMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]);
        INTEL_HYBRID_VP9_DESTROY_MDF_2D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->ThreadInfo[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]);
        INTEL_HYBRID_VP9_DESTROY_MDF_2D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->ThreadInfo[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->TransformType);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->TileIndex);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->BlockSize);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->ReferenceFrame);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->FilterType);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->MotionVector);
        INTEL_HYBRID_VP9_DESTROY_MDF_2DUP_BUFFER(pMdfDevice, &pMdfDecodeBuffer->FilterLevel);
        INTEL_HYBRID_VP9_DESTROY_MDF_2DUP_BUFFER(pMdfDevice, &pMdfDecodeBuffer->Threshold);
    }

    // release previous frame/motion_vector buffer
    if (dwFlags & VP9_HYBRID_DECODE_PREVIOUS_FRAME)
    {
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->PrevReferenceFrame);
        INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeBuffer->PrevMotionVector);
    }

    return;
}

VOID Intel_HybridVp9Decode_MdfHost_ReleaseResidue(
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE     pMdfDecodeEngine,
    CmDevice                                *pMdfDevice)
{
    // intermedia surfaces - residual
    INTEL_HYBRID_VP9_DESTROY_MDF_2D_BUFFER(pMdfDevice, &pMdfDecodeEngine->Residue[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]);
    INTEL_HYBRID_VP9_DESTROY_MDF_2D_BUFFER(pMdfDevice, &pMdfDecodeEngine->Residue[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV]);
}

void Intel_HybridVp9Decode_MdfHost_Destroy (
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine)
{
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame;
    CmDevice                                *pMdfDevice;
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE    pFrame;
    unsigned int                                    i;

    
    pMdfDevice = pMdfDecodeEngine->pMdfDevice;

    for (i = 0; i < pMdfDecodeEngine->dwMdfBufferSize; i++)
    {
        pMdfDecodeFrame = pMdfDecodeEngine->pMdfDecodeFrame + i;
        Intel_HybridVp9Decode_MdfHost_Release(pMdfDecodeFrame, pMdfDevice, VP9_HYBRID_DECODE_ALL_FRAMES);
    }

    Intel_HybridVp9Decode_MdfHost_ReleaseResidue(pMdfDecodeEngine, pMdfDevice);

    Intel_HybridVp9Decode_MdfHost_DestroyThreadSpaces(pMdfDecodeEngine, pMdfDevice);

    INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeEngine->CombinedFilters);

    for (i = 0; i < INTEL_NUM_UNCOMPRESSED_SURFACE_VP9; i++)
    {
        pFrame = &pMdfDecodeEngine->FrameList[i];
        if (pFrame->Frame.pMdfSurface)
        {
            pMdfDecodeEngine->pMdfDevice->DestroySurface(pFrame->Frame.pMdfSurface);
            pFrame->Frame.pMdfSurface = NULL;
        }
    }

    free(pMdfDecodeEngine->pMdfDecodeFrame);
    pMdfDecodeEngine->pMdfDecodeFrame = NULL;

    if (pMdfDecodeEngine->pMdfDevice)
    {
        DestroyCmDevice(pMdfDecodeEngine->pMdfDevice);
	pMdfDecodeEngine->pMdfDevice = NULL;
	pMdfDecodeEngine->iMdfDeviceTsc = 0;
    }

    return;
}

VAStatus Intel_HybridVp9Decode_MdfHost_ReallocateCurrentFrame (
    PINTEL_DECODE_HYBRID_VP9_STATE   pHybridVp9State,
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame, 
    CmDevice                                *pMdfDevice)
{
    VAStatus eStatus = VA_STATUS_SUCCESS;

    Intel_HybridVp9Decode_MdfHost_Release(pMdfDecodeFrame, pMdfDevice, VP9_HYBRID_DECODE_CURRENT_FRAME);

    eStatus = Intel_HybridVp9Decode_MdfHost_Allocate(pHybridVp9State, pMdfDecodeFrame, pMdfDevice, VP9_HYBRID_DECODE_CURRENT_FRAME);

    return eStatus;
}

VAStatus Intel_HybridVp9Decode_MdfHost_ReallocateMvBuffer (
    PINTEL_DECODE_HYBRID_VP9_STATE   pHybridVp9State,
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame, 
    CmDevice                                *pMdfDevice)
{
    VAStatus eStatus = VA_STATUS_SUCCESS;
    VADriverContextP		ctx = (VADriverContextP) (pHybridVp9State->driver_context);

    INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeFrame->MdfDecodeBuffer.MotionVector);

    // Motion vector (uint8 per 4x4; Packed in Z-order, shared Y, U & V)
    INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT64(
        ctx,
        pMdfDevice,
        &pMdfDecodeFrame->MdfDecodeBuffer.MotionVector,
        (pMdfDecodeFrame->dwAlignedWidth >> 2) * (pMdfDecodeFrame->dwAlignedHeight >> 2));

    return eStatus;
}


VAStatus Intel_HybridVp9Decode_MdfHost_ReallocateRefFrameIndexBuffer(
    PINTEL_DECODE_HYBRID_VP9_STATE   pHybridVp9State,
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame, 
    CmDevice                                *pMdfDevice)
{
    VAStatus eStatus = VA_STATUS_SUCCESS;
    VADriverContextP		ctx = (VADriverContextP) (pHybridVp9State->driver_context);

    INTEL_HYBRID_VP9_DESTROY_MDF_1D_BUFFER(pMdfDevice, &pMdfDecodeFrame->MdfDecodeBuffer.ReferenceFrame);

    // Reference frame index (uint16 per 8x8; Packed in Z-order, shared Y, U & V)
    INTEL_HYBRID_VP9_ALLOCATE_MDF_1D_BUFFER_UINT16(
	ctx,
	pMdfDevice,
        &pMdfDecodeFrame->MdfDecodeBuffer.ReferenceFrame,
        (pMdfDecodeFrame->dwAlignedWidth >> 3) * (pMdfDecodeFrame->dwAlignedHeight >> 3));

    return eStatus;
}

VAStatus Intel_HybridVp9Decode_MdfHost_Initialize (
    PINTEL_DECODE_HYBRID_VP9_STATE   pHybridVp9State)
{
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE          pMdfDecodeEngine;
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME           pMdfDecodeFrame = NULL;
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE    pFrame;
    CmDevice                                        *pMdfDevice;
    VAStatus                                      eStatus = VA_STATUS_SUCCESS;
    struct object_surface *sDestSurface;

    pMdfDecodeEngine    = &pHybridVp9State->MdfDecodeEngine;
    pMdfDevice          = pMdfDecodeEngine->pMdfDevice;

    if (!pMdfDecodeEngine->bAllocated)
    {
        // This is the first frame
        uint32_t   dwWidth;
        uint32_t   dwHeight;
        unsigned int   i;

        dwWidth     = pHybridVp9State->dwWidth;
        dwHeight    = pHybridVp9State->dwHeight;

        // Allocate resources
        for (i = 0; i < pMdfDecodeEngine->dwMdfBufferSize; i++)
        {
            pMdfDecodeFrame = pMdfDecodeEngine->pMdfDecodeFrame + i;

            pMdfDecodeFrame->dwWidth            = dwWidth;
            pMdfDecodeFrame->dwHeight           = dwHeight;
            pMdfDecodeFrame->dwAlignedWidth     = ALIGN(dwWidth, INTEL_HYBRID_VP9_B64_SIZE);
            pMdfDecodeFrame->dwAlignedHeight    = ALIGN(dwHeight, INTEL_HYBRID_VP9_B64_SIZE);
            pMdfDecodeFrame->dwWidthB8          = dwWidth >> INTEL_HYBRID_VP9_LOG2_B8_SIZE;   // may not be SB64 aligned
            pMdfDecodeFrame->dwHeightB8         = dwHeight >> INTEL_HYBRID_VP9_LOG2_B8_SIZE;  // may not be SB64 aligned
            pMdfDecodeFrame->dwWidthB16         = pMdfDecodeFrame->dwAlignedWidth >> 4;
            pMdfDecodeFrame->dwHeightB16        = pMdfDecodeFrame->dwAlignedHeight >> 4;
            pMdfDecodeFrame->dwWidthB32         = pMdfDecodeFrame->dwAlignedWidth >> 5;
            pMdfDecodeFrame->dwHeightB32        = pMdfDecodeFrame->dwAlignedHeight >> 5;
            pMdfDecodeFrame->dwWidthB64         = pMdfDecodeFrame->dwAlignedWidth >> 6;
            pMdfDecodeFrame->dwHeightB64        = pMdfDecodeFrame->dwAlignedHeight >> 6;
            pMdfDecodeFrame->dwMaxWidth         = dwWidth;
            pMdfDecodeFrame->dwMaxHeight        = dwHeight;

            pMdfDecodeFrame->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y] =
                pMdfDecodeEngine->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y];
            pMdfDecodeFrame->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV] =
                pMdfDecodeEngine->dwIntraPredKernelMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV];

            Intel_HybridVp9Decode_MdfHost_Allocate(
		pHybridVp9State,
                pMdfDecodeFrame, pMdfDevice, VP9_HYBRID_DECODE_ALL_FRAMES);

            // update HostVLD output buffers
            Intel_HybridVp9Decode_SetHostBuffers(pHybridVp9State, i);
        }

        Intel_HybridVp9Decode_MdfHost_PickDeblockKernel(pMdfDecodeEngine, pMdfDecodeFrame);

        Intel_HybridVp9Decode_MdfHost_SetKernelThreadCount(
            pMdfDecodeEngine, pMdfDecodeFrame);
        Intel_HybridVp9Decode_MdfHost_CreateThreadSpaces(
            pMdfDecodeEngine, pMdfDecodeFrame, pMdfDevice);

        pMdfDecodeEngine->bAllocated    = true;
    }

    sDestSurface = pHybridVp9State->sDestSurface; 
    pFrame = (PINTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE )sDestSurface->private_data;

    pMdfDecodeEngine->bResolutionChanged =
        ((pHybridVp9State->dwCropWidth  != pFrame->Frame.dwWidth) ||
         (pHybridVp9State->dwCropHeight != pFrame->Frame.dwHeight));
    pMdfDecodeEngine->bResolutionChanged &= ((pFrame->Frame.dwWidth > 0) && (pFrame->Frame.dwHeight > 0));

    return eStatus;
}

VAStatus Intel_HybridVp9Decode_MdfHost_RecreateRefCmSurface(
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine,
    struct object_surface                   *pRefSurface,
    INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *pFrameSource)
{
    VAStatus eStatus = VA_STATUS_SUCCESS;
    pFrameSource->Frame.pMdfSurface = NULL;
    CmOsResource target_source;
    memset(&target_source, 0, sizeof(target_source));

    target_source.format = VA_CM_FMT_NV12;
    target_source.aligned_width = pFrameSource->dwWidth;
    target_source.pitch =  pFrameSource->dwWidth;
    target_source.aligned_height = pFrameSource->dwHeight;
    target_source.bo = pRefSurface->bo;
    target_source.tile_type = I915_TILING_Y;
    target_source.orig_width = pFrameSource->dwWidth;
    target_source.orig_height = pFrameSource->dwHeight;
    target_source.bo_flags = DRM_BO_HANDLE;

    INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pMdfDevice->CreateSurface2D(
        &target_source,
        pFrameSource->Frame.pMdfSurface));

    pFrameSource->pMdfDevice = pMdfDecodeEngine->pMdfDevice;
    pFrameSource->iMdfDeviceTsc = pMdfDecodeEngine->iMdfDeviceTsc;

finish:
    return eStatus;
}

VAStatus Intel_HybridVp9Decode_MdfHost_UpdateResolution(
    PINTEL_DECODE_HYBRID_VP9_STATE       pHybridVp9State,
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine,
    struct object_surface                   *psDestSurface,
    DWORD                                   dwCurrIndex,
    DWORD                                   dwCropWidth,
    DWORD                                   dwCropHeight)
{
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE    pFrame;
    VAStatus                                     eStatus = VA_STATUS_SUCCESS;
    VADriverContextP	ctx;
    ctx = (VADriverContextP) (pHybridVp9State->driver_context);
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);

    struct object_surface *surface;

	surface = SURFACE(dwCurrIndex);
	pFrame = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);

    if ( pFrame->pMdfDevice && pFrame->iMdfDeviceTsc &&
        ((pFrame->pMdfDevice != pMdfDecodeEngine->pMdfDevice) ||
        (pFrame->iMdfDeviceTsc != pMdfDecodeEngine->iMdfDeviceTsc)))
    {
        pFrame->Frame.pMdfSurface = NULL;
    }

    // if surface resolution changed, we don't reuse render target since it may have been re-created.
    if (pFrame->Frame.pMdfSurface)
    {
        if ((psDestSurface->width  != pFrame->dwWidth) ||
            (psDestSurface->height != pFrame->dwHeight))
        {
            pMdfDecodeEngine->pMdfDevice->DestroySurface(pFrame->Frame.pMdfSurface);
            pFrame->Frame.pMdfSurface = NULL;
        }
    }
    
    if (pFrame->Frame.pMdfSurface == NULL)
    {
        CmOsResource target_source;
	memset(&target_source, 0, sizeof(target_source));

	target_source.format = VA_CM_FMT_NV12;
	target_source.aligned_width = psDestSurface->width;
	target_source.pitch = psDestSurface->width;
	target_source.aligned_height = psDestSurface->height;
	target_source.bo = psDestSurface->bo;
	target_source.tile_type = I915_TILING_Y;
	target_source.orig_width = psDestSurface->width;
	target_source.orig_height = psDestSurface->height;
        target_source.bo_flags = DRM_BO_HANDLE;
        // Create MDF surface for render target
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pMdfDevice->CreateSurface2D(
            &target_source,
            //&pHybridVp9State->sDestSurface.OsResource,
            pFrame->Frame.pMdfSurface));

        pFrame->dwWidth  = psDestSurface->width;
        pFrame->dwHeight = psDestSurface->height;

       pFrame->pMdfDevice = pMdfDecodeEngine->pMdfDevice;
       pFrame->iMdfDeviceTsc = pMdfDecodeEngine->iMdfDeviceTsc;
    }

    pFrame->Frame.dwWidth  = dwCropWidth;
    pFrame->Frame.dwHeight = dwCropHeight;

finish:
    return eStatus;
}

VAStatus Intel_HybridVp9Decode_MdfHost_SyncResource (
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame)
{
    CmEvent         *pMdfEvent;
    VAStatus      eStatus = VA_STATUS_SUCCESS;


    pMdfEvent = pMdfDecodeFrame->pMdfEvent;

    if (pMdfEvent)
    {
        pMdfEvent->WaitForTaskFinished(5000);
        pMdfDecodeFrame->pMdfQueue->DestroyEvent(pMdfEvent);
        pMdfDecodeFrame->pMdfEvent = NULL;
    }

    return eStatus;
}

VAStatus Intel_HybridVp9Decode_MdfHost_PadFrame (
    VADriverContextP ctx,
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine, 
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame, 
    VASurfaceID                                   ucFrameIndex)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *pFrameResource;
    uint32_t                                       dwWidth, dwHeight;
    VAStatus                                  eStatus = VA_STATUS_SUCCESS;
    struct object_surface *surface;
    CmEvent *pMdfEvent;

    pMdfEvent = CM_NO_EVENT;

    surface = SURFACE(ucFrameIndex); 

    pFrameResource  = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
    dwWidth         = pFrameResource->Frame.dwWidth;
    dwHeight        = pFrameResource->Frame.dwHeight;

    if ((pFrameResource->pMdfDevice != pMdfDecodeEngine->pMdfDevice) ||
        (pFrameResource->iMdfDeviceTsc != pMdfDecodeEngine->iMdfDeviceTsc))
    {
        // Top decoder may call vaDestroy/CreateContext to recreate media ctx and MdfDevice
        // if target frame size becomes larger.
        // CmSurface2D of each reference frame will be reclaimed sequentially.
        // Detect this change with CmDevice's TSC and pointer and reallocate them.
        Intel_HybridVp9Decode_MdfHost_RecreateRefCmSurface(
                             pMdfDecodeEngine,
                             surface,
                             pFrameResource);
    }

    if (!pFrameResource->bHasPadding && (dwWidth & 3))
    {
        CmKernel        *pKernel;
        CmTask          *pTask;
        CmThreadSpace   *pThreadSpace;
        SurfaceIndex    *pSurfaceIndex = NULL;
        uint32_t           dwThreadCount;
        uint32_t           dwLastDwOffset, dwWidthMod4Minus1;

        dwThreadCount       = (dwHeight + 7) >> 3;
        dwLastDwOffset      = dwWidth & ~3;
        dwWidthMod4Minus1   = (dwWidth & 3) - 1;
        pKernel             = (dwWidthMod4Minus1 < 2) ? 
            pMdfDecodeEngine->pKernelRefPadding : pMdfDecodeEngine->pKernelRefPaddingYOnly;


        pKernel->SetThreadCount(dwThreadCount);
        pFrameResource->Frame.pMdfSurface->GetIndex(pSurfaceIndex);
        pKernel->SetKernelArg(0, sizeof(SurfaceIndex), pSurfaceIndex);
        pKernel->SetKernelArg(1, sizeof(DWORD), &dwLastDwOffset);
        pKernel->SetKernelArg(2, sizeof(DWORD), &dwWidthMod4Minus1);

        // Create thread space
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pMdfDevice->CreateThreadSpace(
            1, 
            dwThreadCount, 
            pThreadSpace));

        // Create MDF Tasks and add kernels
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pMdfDevice->CreateTask(pTask));
        INTEL_DECODE_CHK_MDF_STATUS(pTask->AddKernel(pKernel));

        // enqueue tasks
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeFrame->pMdfQueue->Enqueue(
            pTask, 
            pMdfEvent,
            pThreadSpace));

        // destroy MDF tasks
        pMdfDecodeEngine->pMdfDevice->DestroyTask(pTask);
        pMdfDecodeEngine->pMdfDevice->DestroyThreadSpace(pThreadSpace);

        pFrameResource->bHasPadding = TRUE;
    }

    if ((dwWidth & 7) || (dwHeight & 7))
    {
        pFrameResource->Frame.pMdfSurface->SetSurfaceStateDimensions(
            ALIGN(dwWidth, 2),
            dwHeight);
    }

finish:
    return eStatus;
}

BOOL Intel_HybridVp9Decode_MdfHost_IsScaling(
    VADriverContextP ctx,
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME       pMdfDecodeFrame) 
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER pCurrFrame;
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER pLastRefFrame;
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER pGoldenRefFrame;
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER pAltRefFrame;
    BOOL                                      bScaling;
    struct object_surface *surface;
    INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *pFrameSource;

    bScaling = false;

    surface = SURFACE(pMdfDecodeFrame->ucCurrIndex);
    if (surface == NULL)
        return bScaling;

    pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
    pCurrFrame = &(pFrameSource->Frame); 
    
    surface = SURFACE(pMdfDecodeFrame->ucLastRefIndex);
    if (surface == NULL)
        return bScaling;

    pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
    pLastRefFrame = &(pFrameSource->Frame);
 
    surface = SURFACE(pMdfDecodeFrame->ucGoldenRefIndex);
    if (surface == NULL)
        return bScaling;

    pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
    pGoldenRefFrame = &(pFrameSource->Frame);
 
    surface = SURFACE(pMdfDecodeFrame->ucAltRefIndex);
    if (surface == NULL)
        return bScaling;

    pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
    pAltRefFrame = &(pFrameSource->Frame);
 
    bScaling = 
        (pCurrFrame->dwWidth != pLastRefFrame->dwWidth)     || 
        (pCurrFrame->dwHeight != pLastRefFrame->dwHeight)   ||
        (pCurrFrame->dwWidth != pGoldenRefFrame->dwWidth)   ||
        (pCurrFrame->dwHeight != pGoldenRefFrame->dwHeight) ||
        (pCurrFrame->dwWidth != pAltRefFrame->dwWidth)      ||
        (pCurrFrame->dwHeight != pAltRefFrame->dwHeight);

    return bScaling;
}

VOID Intel_HybridVp9Decode_MdfHost_SetScaleFactors(
    VADriverContextP ctx,
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME       pMdfDecodeFrame,
    INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE FrameList[INTEL_NUM_UNCOMPRESSED_SURFACE_VP9], 
    UINT32                                      ScaleFactor[6])
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER pCurrFrame;
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER pLastRefFrame;
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER pGoldenRefFrame;
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER pAltRefFrame;
    struct object_surface *surface;
    INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *pFrameSource;

    surface = SURFACE(pMdfDecodeFrame->ucCurrIndex);
    pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
    pCurrFrame = &(pFrameSource->Frame); 
    
    surface = SURFACE(pMdfDecodeFrame->ucLastRefIndex);
    pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
    pLastRefFrame = &(pFrameSource->Frame); 
    
    surface = SURFACE(pMdfDecodeFrame->ucGoldenRefIndex);
    pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
    pGoldenRefFrame = &(pFrameSource->Frame);

    surface = SURFACE(pMdfDecodeFrame->ucAltRefIndex);
    pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
    pAltRefFrame = &(pFrameSource->Frame); 


    ScaleFactor[0] = (pLastRefFrame->dwWidth << VP9_HYBRID_DECODE_REF_SCALE_SHIFT) / pCurrFrame->dwWidth;
    ScaleFactor[1] = (pLastRefFrame->dwHeight << VP9_HYBRID_DECODE_REF_SCALE_SHIFT) / pCurrFrame->dwHeight;
    ScaleFactor[2] = (pGoldenRefFrame->dwWidth << VP9_HYBRID_DECODE_REF_SCALE_SHIFT) / pCurrFrame->dwWidth;
    ScaleFactor[3] = (pGoldenRefFrame->dwHeight << VP9_HYBRID_DECODE_REF_SCALE_SHIFT) / pCurrFrame->dwHeight;
    ScaleFactor[4] = (pAltRefFrame->dwWidth << VP9_HYBRID_DECODE_REF_SCALE_SHIFT) / pCurrFrame->dwWidth;
    ScaleFactor[5] = (pAltRefFrame->dwHeight << VP9_HYBRID_DECODE_REF_SCALE_SHIFT) / pCurrFrame->dwHeight;
}

VAStatus Intel_HybridVp9Decode_MdfHost_ZeroFillThreadInfo(
    CmDevice        *pMdfDevice,
    CmKernel        *pKernel,
    CmQueue         *pMdfQueue, 
    CmThreadSpace   *pThreadSpaceZeroFill,
    SurfaceIndex    *pSurfaceIndexThreadInfo)
{
    CmTask          *pTask;
    CmEvent         *pMdfEvent;
    VAStatus      eStatus = VA_STATUS_SUCCESS;

    pMdfEvent = CM_NO_EVENT;
    pKernel->SetKernelArg(0, sizeof(SurfaceIndex), pSurfaceIndexThreadInfo);

    // Create MDF Tasks and add kernels
    INTEL_DECODE_CHK_MDF_STATUS(pMdfDevice->CreateTask(pTask));
    INTEL_DECODE_CHK_MDF_STATUS(pTask->AddKernel(pKernel));

    // enqueue tasks
    INTEL_DECODE_CHK_MDF_STATUS(pMdfQueue->Enqueue(
        pTask,
        pMdfEvent,
        pThreadSpaceZeroFill));

    // destroy MDF tasks
    pMdfDevice->DestroyTask(pTask);

finish:
    return eStatus;
}

VAStatus Intel_HybridVp9Decode_MdfHost_Execute (
    VADriverContextP ctx, 
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine, 
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame)
{
    PINTEL_DECODE_HYBRID_VP9_MDF_BUFFER  pMdfBuffer;
    CmKernel                                *pKernel;
    SurfaceIndex                            *pSurfaceIndexTransformCoeff[2] = {NULL, NULL};
    SurfaceIndex                            *pSurfaceIndexResiduals[2]      = {NULL, NULL};
    SurfaceIndex                            *pSurfaceIndexCoeffStatus[2]    = {NULL, NULL};
    SurfaceIndex                            *pSurfaceIndexThreadInfo[2]     = {NULL, NULL};
    SurfaceIndex                            *pSurfaceIndexTransformSize     = NULL;
    SurfaceIndex                            *pSurfaceIndexTransformType     = NULL;
    SurfaceIndex                            *pSurfaceIndexQP                = NULL;
    SurfaceIndex                            *pSurfaceIndexTileIndex         = NULL;
    SurfaceIndex                            *pSurfaceIndexReconFrame        = NULL;
    SurfaceIndex                            *pSurfaceIndexPredictionMode    = NULL;
    SurfaceIndex                            *pSurfaceIndexBlockSize         = NULL;
    SurfaceIndex                            *pSurfaceIndexReferenceFrame    = NULL;
    SurfaceIndex                            *pSurfaceIndexFilterType        = NULL;
    SurfaceIndex                            *pSurfaceIndexMotionVector      = NULL;
    SurfaceIndex                            *pSurfaceIndexVerticalEdgeMask  = NULL;
    SurfaceIndex                            *pSurfaceIndexHorizontalEdgeMask = NULL;
    SurfaceIndex                            *pSurfaceIndexFilterLevel       = NULL;
    SurfaceIndex                            *pSurfaceIndexThreshold         = NULL;
    SurfaceIndex                            *pSurfaceThreadDependency       = NULL;
    int                                     i;
    unsigned int                                    uiArgIndex;
    VAStatus                              eStatus = VA_STATUS_SUCCESS;
    int		last_task = 0;
    CmEvent *pCmNoEvent = CM_NO_EVENT;

    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER pCurrFrame;
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER pLastRefFrame;
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER pGoldenRefFrame;
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER pAltRefFrame;
    struct object_surface *surface;
    INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *pFrameSource;

    pMdfBuffer = &pMdfDecodeFrame->MdfDecodeBuffer;

    for (i = INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y; i < INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER; i++)
    {
        /////////////////// IQ/IT ///////////////////
        uiArgIndex = 0;
        pKernel = pMdfDecodeFrame->bLossless ? 
            pMdfDecodeEngine->pKernelIqItLossless[i] : pMdfDecodeEngine->pKernelIqIt[i];
        pMdfBuffer->TransformCoeff[i].pMdfBuffer->GetIndex(pSurfaceIndexTransformCoeff[0]);
        pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexTransformCoeff[0]);        

        if (i == INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y)
        {
            pMdfDecodeEngine->Residue[i].pMdfSurface->GetIndex(pSurfaceIndexResiduals[i]);
            pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexResiduals[i]);        

            if (!pMdfDecodeFrame->bLossless)
            {
                pMdfBuffer->TransformSize[i].pMdfBuffer->GetIndex(pSurfaceIndexTransformSize);
                pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexTransformSize);        

                pMdfBuffer->TransformType.pMdfBuffer->GetIndex(pSurfaceIndexTransformType);
                pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexTransformType);        
            }
        }
        else
        {
            pMdfBuffer->TransformCoeff[i + 1].pMdfBuffer->GetIndex(pSurfaceIndexTransformCoeff[1]);
            pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexTransformCoeff[1]);        

            pMdfDecodeEngine->Residue[i].pMdfSurface->GetIndex(pSurfaceIndexResiduals[i]);
            pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexResiduals[i]);        

            if (!pMdfDecodeFrame->bLossless)
            {
                pMdfBuffer->TransformSize[i].pMdfBuffer->GetIndex(pSurfaceIndexTransformSize);
                pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexTransformSize);        
            }
        }

        pMdfBuffer->CoeffStatus[i].pMdfBuffer->GetIndex(pSurfaceIndexCoeffStatus[i]);
        pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexCoeffStatus[i]);        

        pMdfBuffer->QP[i].pMdfBuffer->GetIndex(pSurfaceIndexQP);
        pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexQP);        

        pKernel->SetKernelArg(uiArgIndex++, sizeof(UINT16), &pMdfDecodeFrame->dwWidth);        
        pKernel->SetKernelArg(uiArgIndex++, sizeof(UINT16), &pMdfDecodeFrame->dwHeight);    

        // Create MDF Tasks and add kernels
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pMdfDevice->CreateTask(pMdfDecodeEngine->pTaskIqIt[i]));
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pTaskIqIt[i]->AddKernel(pKernel));

        /* Enqueue will reset pCmNoEvent to NULL. Set to CM_NO_EVENT here in each Enqueue iteration,
         * otherwise new CmEvent will be allocated for each iteration leading to large number of
         * CmEvent references in the global CmQueue, which causes mem leak and performance slow down.
         */
        pCmNoEvent = CM_NO_EVENT;

        // enqueue tasks
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeFrame->pMdfQueue->Enqueue(
            pMdfDecodeEngine->pTaskIqIt[i],
            pCmNoEvent,
            pMdfDecodeEngine->pThreadSpaceIqIt[i]));

        // destroy MDF tasks
        pMdfDecodeEngine->pMdfDevice->DestroyTask(pMdfDecodeEngine->pTaskIqIt[i]);
    }

    /////////////////// Inter Prediction ///////////////////
    if (!pMdfDecodeFrame->bIntraOnly)
    {
        SurfaceIndex SurfaceIndexFrame[3];
        SurfaceIndex *pSurfaceIndexFrame = NULL;
        BOOL         bScaling;

        bScaling = Intel_HybridVp9Decode_MdfHost_IsScaling(ctx,
            pMdfDecodeFrame);

        // Pad reference frames
	Intel_HybridVp9Decode_MdfHost_PadFrame(ctx,
            pMdfDecodeEngine, pMdfDecodeFrame, pMdfDecodeFrame->ucLastRefIndex);
	Intel_HybridVp9Decode_MdfHost_PadFrame(ctx,
            pMdfDecodeEngine, pMdfDecodeFrame, pMdfDecodeFrame->ucGoldenRefIndex);
	Intel_HybridVp9Decode_MdfHost_PadFrame(ctx,
            pMdfDecodeEngine, pMdfDecodeFrame, pMdfDecodeFrame->ucAltRefIndex);

        pKernel = bScaling ? pMdfDecodeEngine->pKernelInterScaling : pMdfDecodeEngine->pKernelInter;

        // set arguments
        uiArgIndex = 0;
        pMdfBuffer->PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pMdfBuffer->GetIndex(pSurfaceIndexPredictionMode);
        pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexPredictionMode);

        pMdfBuffer->BlockSize.pMdfBuffer->GetIndex(pSurfaceIndexBlockSize);
        pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexBlockSize);

        if (pMdfDecodeFrame->bSwitchableFilterType)
        {
            pMdfBuffer->FilterType.pMdfBuffer->GetIndex(pSurfaceIndexFilterType);
            pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexFilterType);
        }
        else
        {
            pMdfBuffer->BlockSize.pMdfBuffer->GetIndex(pSurfaceIndexBlockSize);
            pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexBlockSize);
        }

        pMdfBuffer->ReferenceFrame.pMdfBuffer->GetIndex(pSurfaceIndexReferenceFrame);
        pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexReferenceFrame);

        pMdfBuffer->MotionVector.pMdfBuffer->GetIndex(pSurfaceIndexMotionVector);
        pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexMotionVector);

        pMdfBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pMdfBuffer->GetIndex(pSurfaceIndexCoeffStatus[0]);
        pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexCoeffStatus[0]);

        pMdfBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pMdfBuffer->GetIndex(pSurfaceIndexCoeffStatus[1]);
        pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexCoeffStatus[1]);

        pMdfDecodeEngine->Residue[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pMdfSurface->GetIndex(pSurfaceIndexResiduals[0]);
        pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexResiduals[0]);

        pMdfDecodeEngine->Residue[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pMdfSurface->GetIndex(pSurfaceIndexResiduals[1]);
        pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexResiduals[1]);


        /* Last ref */
	surface = SURFACE(pMdfDecodeFrame->ucLastRefIndex);
	pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
	pLastRefFrame = &(pFrameSource->Frame);
        pLastRefFrame->pMdfSurface->GetIndex(pSurfaceIndexFrame);
        SurfaceIndexFrame[0] = pSurfaceIndexFrame->get_data();

        /* Golden Ref */
	surface = SURFACE(pMdfDecodeFrame->ucGoldenRefIndex);
	pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
	pGoldenRefFrame = &(pFrameSource->Frame);
        pGoldenRefFrame->pMdfSurface->GetIndex(pSurfaceIndexFrame);
        SurfaceIndexFrame[1] = pSurfaceIndexFrame->get_data();

        /* Alt Ref */
	surface = SURFACE(pMdfDecodeFrame->ucAltRefIndex);
	pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
	pAltRefFrame = &(pFrameSource->Frame);
        pAltRefFrame->pMdfSurface->GetIndex(pSurfaceIndexFrame);
        SurfaceIndexFrame[2] = pSurfaceIndexFrame->get_data();

        pKernel->SetKernelArg(uiArgIndex++, 3 * sizeof(SurfaceIndex), SurfaceIndexFrame);

	surface = SURFACE(pMdfDecodeFrame->ucCurrIndex);
	pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
	pCurrFrame = &(pFrameSource->Frame); 
        pCurrFrame->pMdfSurface->GetIndex(pSurfaceIndexReconFrame);
        pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexReconFrame);

        pKernel->SetKernelArg(uiArgIndex++, sizeof(DWORD), &pMdfDecodeFrame->dwInterpolationFilterType);

        pKernel->SetKernelArg(uiArgIndex++, sizeof(DWORD), &pMdfDecodeFrame->dwWidthB64);

        if (bScaling)
        {
            SurfaceIndex *pSurfaceCombinedFilters = NULL;
            uint32_t ScaleFactor[6];

            Intel_HybridVp9Decode_MdfHost_SetScaleFactors(ctx,
                pMdfDecodeFrame, pMdfDecodeEngine->FrameList, ScaleFactor);
            pKernel->SetKernelArg(uiArgIndex++, 6 * sizeof(UINT32), ScaleFactor);
            pKernel->SetKernelArg(uiArgIndex++, sizeof(DWORD), &pMdfDecodeFrame->dwWidth);
            pKernel->SetKernelArg(uiArgIndex++, sizeof(DWORD), &pMdfDecodeFrame->dwHeight);
            pMdfDecodeEngine->CombinedFilters.pMdfBuffer->GetIndex(pSurfaceCombinedFilters);
            pKernel->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceCombinedFilters);
        }

        else
        {
            pKernel->SetKernelArg(uiArgIndex++, VP9_HYBRID_DECODE_COMBINED_FILETER_SIZE, pMdfDecodeEngine->CombinedFilters.pu8Buffer);
        }

        // Create MDF Tasks and add kernels
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pMdfDevice->CreateTask(pMdfDecodeEngine->pTaskInter));
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pTaskInter->AddKernel(pKernel));

        // Enqueue will reset pCmNoEvent to NULL. Set to CM_NO_EVENT here in each Enqueue iteration.
        pCmNoEvent = CM_NO_EVENT;

        // enqueue tasks
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeFrame->pMdfQueue->Enqueue(
            pMdfDecodeEngine->pTaskInter,
            pCmNoEvent,
            pMdfDecodeEngine->pThreadSpaceInter));

        // destroy MDF tasks
        pMdfDecodeEngine->pMdfDevice->DestroyTask(pMdfDecodeEngine->pTaskInter);
    }

    for (i = INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y; i < INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER; i++)
    {
        /////////////////// Intra prediction ///////////////////
        // set arguments
        uiArgIndex = 0;
	surface = SURFACE(pMdfDecodeFrame->ucCurrIndex);
	pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
	pCurrFrame = &(pFrameSource->Frame);

        pCurrFrame->pMdfSurface->GetIndex(pSurfaceIndexReconFrame);
        pMdfDecodeEngine->pKernelIntra[i]->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexReconFrame);        

        pMdfDecodeEngine->Residue[i].pMdfSurface->GetIndex(pSurfaceIndexResiduals[i]);
        pMdfDecodeEngine->pKernelIntra[i]->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexResiduals[i]);        

        pMdfBuffer->BlockSize.pMdfBuffer->GetIndex(pSurfaceIndexBlockSize);
        pMdfDecodeEngine->pKernelIntra[i]->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexBlockSize);        

        pMdfBuffer->TransformSize[i].pMdfBuffer->GetIndex(pSurfaceIndexTransformSize);
        pMdfDecodeEngine->pKernelIntra[i]->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexTransformSize);        

        pMdfBuffer->CoeffStatus[i].pMdfBuffer->GetIndex(pSurfaceIndexCoeffStatus[i]);
        pMdfDecodeEngine->pKernelIntra[i]->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexCoeffStatus[i]);        

        pMdfBuffer->PredictionMode[i].pMdfBuffer->GetIndex(pSurfaceIndexPredictionMode);
        pMdfDecodeEngine->pKernelIntra[i]->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexPredictionMode);        

        pMdfBuffer->TileIndex.pMdfBuffer->GetIndex(pSurfaceIndexTileIndex);
        pMdfDecodeEngine->pKernelIntra[i]->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexTileIndex);        

        if (pMdfBuffer->ThreadInfo[i].pMdfSurface)
        {            
            pMdfBuffer->ThreadInfo[i].pMdfSurface->GetIndex(pSurfaceIndexThreadInfo[i]);

            Intel_HybridVp9Decode_MdfHost_ZeroFillThreadInfo(
                pMdfDecodeEngine->pMdfDevice, 
                pMdfDecodeEngine->pKernelZeroFill[i], 
                pMdfDecodeFrame->pMdfQueue, 
                pMdfDecodeEngine->pThreadSpaceZeroFill[i],
                pSurfaceIndexThreadInfo[i]);

            pMdfDecodeEngine->pKernelIntra[i]->SetKernelArg(uiArgIndex++, sizeof(SurfaceIndex), pSurfaceIndexThreadInfo[i]);        
        }

        pMdfDecodeEngine->pKernelIntra[i]->SetKernelArg(uiArgIndex++, sizeof(UINT16), &pMdfDecodeFrame->dwWidth);        
        pMdfDecodeEngine->pKernelIntra[i]->SetKernelArg(uiArgIndex++, sizeof(UINT16), &pMdfDecodeFrame->dwHeight);        

        // Create MDF Tasks and add kernels
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pMdfDevice->CreateTask(pMdfDecodeEngine->pTaskIntra[i]));
        INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pTaskIntra[i]->AddKernel(pMdfDecodeEngine->pKernelIntra[i]));

        // enqueue tasks
        if (((i + 1) == INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER) && !pMdfDecodeFrame->bNeedDeblock)
            last_task = 1;

        // Enqueue will reset pCmNoEvent to NULL. Set to CM_NO_EVENT here in each Enqueue iteration.
        pCmNoEvent = CM_NO_EVENT;

        INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeFrame->pMdfQueue->Enqueue(
            pMdfDecodeEngine->pTaskIntra[i],
            (last_task ? pMdfDecodeFrame->pMdfEvent : pCmNoEvent)));

        // destroy MDF tasks
        pMdfDecodeEngine->pMdfDevice->DestroyTask(pMdfDecodeEngine->pTaskIntra[i]);
    }

    /////////////////// Loopfilter/Deblocking ///////////////////
    if (pMdfDecodeFrame->bNeedDeblock)
    {
        DWORD dwRegion;

        for (i = INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y; i < INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER; i++)
        {

            // set arguments
            dwRegion = INTEL_HYBRID_VP9_MDF_DEBLOCK_LEFT_TOP;
            pKernel  = pMdfDecodeEngine->pKernelDeblock[i][dwRegion];

	    surface = SURFACE(pMdfDecodeFrame->ucCurrIndex);
	    pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
	    pCurrFrame = &(pFrameSource->Frame);
            pCurrFrame->pMdfSurface->GetIndex(pSurfaceIndexReconFrame);
            pKernel->SetKernelArg(0, sizeof(SurfaceIndex), pSurfaceIndexReconFrame);

            pMdfBuffer->FilterLevel.pMdfSurface->GetIndex(pSurfaceIndexFilterLevel);
            pKernel->SetKernelArg(1, sizeof(SurfaceIndex), pSurfaceIndexFilterLevel);

            pMdfBuffer->VerticalEdgeMask[i].pMdfSurface->GetIndex(pSurfaceIndexVerticalEdgeMask);
            pKernel->SetKernelArg(2, sizeof(SurfaceIndex), pSurfaceIndexVerticalEdgeMask);

            pMdfBuffer->HorizontalEdgeMask[i].pMdfSurface->GetIndex(pSurfaceIndexHorizontalEdgeMask);
            pKernel->SetKernelArg(3, sizeof(SurfaceIndex), pSurfaceIndexHorizontalEdgeMask);

            pMdfBuffer->Threshold.pMdfSurface->GetIndex(pSurfaceIndexThreshold);
            pKernel->SetKernelArg(4, sizeof(SurfaceIndex), pSurfaceIndexThreshold);

            pMdfBuffer->DeblockOntheFlyThreadMask[i].pMdfSurface->GetIndex(pSurfaceThreadDependency);
            pKernel->SetKernelArg(5, sizeof(SurfaceIndex), pSurfaceThreadDependency);

            if (pKernel != pMdfDecodeEngine->pKernelDeblock8x8[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y]) // if it's not 8x8 luma deblocking
            {
                pKernel->SetKernelArg(6, sizeof(DWORD), &dwRegion);
            }

            // Create MDF Tasks and add kernels
            INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pMdfDevice->CreateTask(pMdfDecodeEngine->pTaskDeblock[i]));
            INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pTaskDeblock[i]->AddKernel(pKernel));

            if (((i + 1) == INTEL_HYBRID_VP9_MDF_YUV_PLANE_NUMBER))
		last_task = 1;

            // Enqueue will reset pCmNoEvent to NULL. Set to CM_NO_EVENT here in each Enqueue iteration.
            pCmNoEvent = CM_NO_EVENT;

            // enqueue tasks
            INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeFrame->pMdfQueue->Enqueue(
                pMdfDecodeEngine->pTaskDeblock[i],
                (last_task ? pMdfDecodeFrame->pMdfEvent : pCmNoEvent)));

            // destroy MDF tasks
            pMdfDecodeEngine->pMdfDevice->DestroyTask(pMdfDecodeEngine->pTaskDeblock[i]);

            // Enqueue more deblocking tasks for blocks out of 4080x4088 bound
            for (dwRegion = INTEL_HYBRID_VP9_MDF_DEBLOCK_RIGHT_TOP;
                dwRegion < INTEL_HYBRID_VP9_MDF_DEBLOCK_REGIONS;
                dwRegion++)
            {
                pKernel = pMdfDecodeEngine->pKernelDeblock[i][dwRegion];
                if (pMdfDecodeEngine->pThreadSpaceDeblock[i][dwRegion])
                {
                    pKernel->SetKernelArg(0, sizeof(SurfaceIndex), pSurfaceIndexReconFrame);
                    pKernel->SetKernelArg(1, sizeof(SurfaceIndex), pSurfaceIndexFilterLevel);
                    pKernel->SetKernelArg(2, sizeof(SurfaceIndex), pSurfaceIndexVerticalEdgeMask);
                    pKernel->SetKernelArg(3, sizeof(SurfaceIndex), pSurfaceIndexHorizontalEdgeMask);
                    pKernel->SetKernelArg(4, sizeof(SurfaceIndex), pSurfaceIndexThreshold);
                    pKernel->SetKernelArg(5, sizeof(SurfaceIndex), pSurfaceThreadDependency);
                    pKernel->SetKernelArg(6, sizeof(DWORD), &dwRegion);

                    // Create MDF Tasks and add kernels
                    INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pMdfDevice->CreateTask(pMdfDecodeEngine->pTaskDeblock[i]));
                    INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeEngine->pTaskDeblock[i]->AddKernel(pKernel));

                    // enqueue tasks
                    INTEL_DECODE_CHK_MDF_STATUS(pMdfDecodeFrame->pMdfQueue->Enqueue(
                        pMdfDecodeEngine->pTaskDeblock[i],
                        pMdfDecodeFrame->pMdfEvent));

                    // destroy MDF tasks
                    pMdfDecodeEngine->pMdfDevice->DestroyTask(pMdfDecodeEngine->pTaskDeblock[i]);
                }
            }
        }
    }

finish:
    return eStatus;
}

static INT g_iCounter = 0, g_iInterCounter = 0;

int intel_hybrid_Vp9Decode_WriteFileFromPtr(
    const char     *pFilename,
    void           *pData,
    DWORD           dwSize, 
    INT             iCounter)
{
    int      eStatus;

    if (iCounter == 0)
    {
        eStatus = intel_hybrid_writefilefromptr(pFilename, pData, dwSize);
    }
    else
    {
        eStatus = intel_hybrid_appendfilefromptr(pFilename, pData, dwSize);
    }

    return eStatus;
}

int intel_hybrid_Vp9Decode_DebugDump (
    PINTEL_DECODE_HYBRID_VP9_STATE       pHybridVp9State, 
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame)
{
    VADriverContextP	ctx;
    PINTEL_DECODE_HYBRID_VP9_MDF_BUFFER      pMdfDecodeBuffer;
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER   pMdfBuffer2D;
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    MdfBuffer1D;
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE      pMdfDecodeEngine;
    INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    Buffer;
    MEDIA_DRV_CONTEXT *drv_ctx;

    ctx = (VADriverContextP) (pHybridVp9State->driver_context);
    drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    pMdfDecodeBuffer    = &pMdfDecodeFrame->MdfDecodeBuffer;
    struct object_surface *surface;
    INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *pFrameSource;


    // dump recon
    pMdfDecodeEngine = &pHybridVp9State->MdfDecodeEngine;

    surface = SURFACE(pMdfDecodeFrame->ucCurrIndex);

    pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);

    Buffer.dwSize  = pFrameSource->dwWidth * pFrameSource->dwHeight * 3 / 2;
    Buffer.pBuffer  = intel_alloc_zero_aligned_memory(Buffer.dwSize, INTEL_HYBRID_VP9_PAGE_SIZE);

    if (Buffer.pBuffer == NULL)
	goto allocation_fail;

    pFrameSource->Frame.pMdfSurface->ReadSurface(Buffer.pu8Buffer, pMdfDecodeFrame->pMdfEvent);
    intel_hybrid_Vp9Decode_WriteFileFromPtr("driver_dump\\Recon_U8.dat", Buffer.pBuffer, Buffer.dwSize, g_iCounter);

    Buffer.dwSize = pFrameSource->dwWidth * pFrameSource->dwHeight;
    if (pMdfDecodeFrame->bNeedDeblock)
    {
        // dump after loopfilter
        intel_hybrid_Vp9Decode_WriteFileFromPtr(
            "driver_dump\\AfterLoopFilter_U8_Y.dat", 
            Buffer.pBuffer, 
            Buffer.dwSize, 
            g_iCounter);
        intel_hybrid_Vp9Decode_WriteFileFromPtr(
            "driver_dump\\AfterLoopFilter_U8_CbCr.dat", 
            Buffer.pu8Buffer + Buffer.dwSize, 
            Buffer.dwSize >> 1, 
            g_iCounter);
    }
    else
    {
        // dump after loopfilter
        intel_hybrid_Vp9Decode_WriteFileFromPtr(
            "driver_dump\\BeforeLoopFilter_U8_Y.dat", 
            Buffer.pBuffer, 
            Buffer.dwSize, 
            g_iCounter);
        intel_hybrid_Vp9Decode_WriteFileFromPtr(
            "driver_dump\\BeforeLoopFilter_U8_CbCr.dat", 
            Buffer.pu8Buffer + Buffer.dwSize, 
            Buffer.dwSize >> 1, 
            g_iCounter);
    }
 
    free(Buffer.pBuffer);

    // dump Luma residual
    pMdfBuffer2D           = &pMdfDecodeEngine->Residue[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y];
    MdfBuffer1D.dwSize     = pMdfDecodeFrame->dwAlignedWidth * pMdfDecodeFrame->dwAlignedHeight;
    MdfBuffer1D.pu16Buffer = (PUINT16)intel_alloc_zero_aligned_memory(
	MdfBuffer1D.dwSize * sizeof(UINT16),
	INTEL_HYBRID_VP9_PAGE_SIZE);
    
    if (MdfBuffer1D.pu16Buffer == NULL)
	    goto allocation_fail;

    pMdfBuffer2D->pMdfSurface->ReadSurface(MdfBuffer1D.pu8Buffer, pMdfDecodeFrame->pMdfEvent);
    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\PostITResidual_I16_Y.dat", 
	MdfBuffer1D.pBuffer, 
	MdfBuffer1D.dwSize * sizeof(UINT16), 
	g_iCounter);
    free(MdfBuffer1D.pBuffer);
    MdfBuffer1D.pBuffer = NULL;

    // dump Chroma residual
    pMdfBuffer2D           = &pMdfDecodeEngine->Residue[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV];
    MdfBuffer1D.dwSize     = pMdfDecodeFrame->dwAlignedWidth * (pMdfDecodeFrame->dwAlignedHeight >> 1);
    MdfBuffer1D.pu16Buffer = (PUINT16)intel_alloc_zero_aligned_memory(
	MdfBuffer1D.dwSize * sizeof(UINT16),
	INTEL_HYBRID_VP9_PAGE_SIZE);

    if (MdfBuffer1D.pu16Buffer == NULL)
	    goto allocation_fail;

    pMdfBuffer2D->pMdfSurface->ReadSurface(MdfBuffer1D.pu8Buffer, pMdfDecodeFrame->pMdfEvent);
    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\PostITResidual_I16_CbCr.dat", 
	MdfBuffer1D.pBuffer, 
	MdfBuffer1D.dwSize * sizeof(UINT16), 
	g_iCounter);
    free(MdfBuffer1D.pBuffer);
    MdfBuffer1D.pBuffer = NULL;

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\PreITCoeff_RawCoeffI16_Y.dat", 
	pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pBuffer, 
	pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize * sizeof(UINT16), 
	g_iCounter);

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\PreITCoeff_RawCoeffI16_Cb.dat", 
	pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_U].pBuffer, 
	pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_U].dwSize * sizeof(UINT16), 
	g_iCounter);

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\PreITCoeff_RawCoeffI16_Cr.dat", 
	pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_V].pBuffer, 
	pMdfDecodeBuffer->TransformCoeff[INTEL_HYBRID_VP9_MDF_YUV_PLANE_V].dwSize * sizeof(UINT16), 
	g_iCounter);

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\TxSize_U8_Y.dat", 
	pMdfDecodeBuffer->TransformSize[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pBuffer, 
	pMdfDecodeBuffer->TransformSize[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize, 
	g_iCounter);

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\TxSize_U8_CbCr.dat", 
	pMdfDecodeBuffer->TransformSize[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pBuffer, 
	pMdfDecodeBuffer->TransformSize[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwSize, 
	g_iCounter);

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\CoefficientStatusFlag_U8_Y.dat", 
	pMdfDecodeBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pBuffer, 
	pMdfDecodeBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize, 
	g_iCounter);

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\CoefficientStatusFlag_U8_CbCr.dat", 
	pMdfDecodeBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pBuffer, 
	pMdfDecodeBuffer->CoeffStatus[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwSize, 
	g_iCounter);

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\PredictionMode_U8_Y.dat", 
	pMdfDecodeBuffer->PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pBuffer, 
	pMdfDecodeBuffer->PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize, 
	g_iCounter);

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\PredictionMode_U8_CbCr.dat", 
	pMdfDecodeBuffer->PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pBuffer, 
	pMdfDecodeBuffer->PredictionMode[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwSize, 
	g_iCounter);

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\DequantValue_U8_Y.dat", 
	pMdfDecodeBuffer->QP[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pBuffer, 
	pMdfDecodeBuffer->QP[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize * sizeof(UINT16), 
	g_iCounter);

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\DequantValue_U8_CbCr.dat", 
	pMdfDecodeBuffer->QP[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pBuffer, 
	pMdfDecodeBuffer->QP[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwSize * sizeof(UINT16), 
	g_iCounter);

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\TxType_U8_Y.dat", 
	pMdfDecodeBuffer->TransformType.pBuffer, 
	pMdfDecodeBuffer->TransformType.dwSize, 
	g_iCounter);

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\TileSliceInfo_U8.dat", 
	pMdfDecodeBuffer->TileIndex.pBuffer, 
	pMdfDecodeBuffer->TileIndex.dwSize, 
	g_iCounter);

    intel_hybrid_Vp9Decode_WriteFileFromPtr(
	"driver_dump\\BlockSize_U8_Y.dat", 
	pMdfDecodeBuffer->BlockSize.pBuffer, 
	pMdfDecodeBuffer->BlockSize.dwSize, 
	g_iCounter);

    if (pMdfDecodeFrame->bNeedDeblock)
    {
	intel_hybrid_Vp9Decode_WriteFileFromPtr(
	    "driver_dump\\LoopFilterVerticalMask_U8_Y.dat", 
	    pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pBuffer, 
	    pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize, 
	    g_iCounter);

	intel_hybrid_Vp9Decode_WriteFileFromPtr(
	    "driver_dump\\LoopFilterVerticalMask_U8_CbCr.dat", 
	    pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pBuffer, 
	    pMdfDecodeBuffer->VerticalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwSize, 
	    g_iCounter);

	intel_hybrid_Vp9Decode_WriteFileFromPtr(
	    "driver_dump\\LoopFilterHorizontalMask_U8_Y.dat", 
	    pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].pBuffer, 
	    pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwSize, 
	    g_iCounter);

	intel_hybrid_Vp9Decode_WriteFileFromPtr(
	    "driver_dump\\LoopFilterHorizontalMask_U8_CbCr.dat", 
	    pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].pBuffer, 
	    pMdfDecodeBuffer->HorizontalEdgeMask[INTEL_HYBRID_VP9_MDF_YUV_PLANE_UV].dwSize, 
	    g_iCounter);

	intel_hybrid_Vp9Decode_WriteFileFromPtr(
	    "driver_dump\\FilterLevel_U8.dat", 
	    pMdfDecodeBuffer->FilterLevel.pBuffer, 
	    pMdfDecodeBuffer->FilterLevel.dwSize, 
	    g_iCounter);

	intel_hybrid_Vp9Decode_WriteFileFromPtr(
	    "driver_dump\\LoopFilterThreshold_U8.dat", 
	    pMdfDecodeBuffer->Threshold.pBuffer, 
	    pMdfDecodeBuffer->Threshold.dwSize, 
	    g_iCounter);
    }

    if (!pMdfDecodeFrame->bIntraOnly)
    {
	INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER tmp_Buffer;

        /* dump Last Ref frame */
        surface = SURFACE(pMdfDecodeFrame->ucLastRefIndex);

        pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
        tmp_Buffer.dwSize  = pFrameSource->dwWidth * pFrameSource->dwHeight * 3 / 2;
        tmp_Buffer.pBuffer  = intel_alloc_zero_aligned_memory(tmp_Buffer.dwSize, INTEL_HYBRID_VP9_PAGE_SIZE);

        if (tmp_Buffer.pBuffer == NULL)
	    goto allocation_fail;

	pFrameSource->Frame.pMdfSurface->ReadSurface(tmp_Buffer.pu8Buffer, pMdfDecodeFrame->pMdfEvent);
	intel_hybrid_Vp9Decode_WriteFileFromPtr("driver_dump\\ReferenceFrameLast.dat", tmp_Buffer.pBuffer, tmp_Buffer.dwSize, g_iInterCounter);

        free(tmp_Buffer.pBuffer);
        tmp_Buffer.pBuffer = NULL;

        /* dump Golden Ref frame */
        surface = SURFACE(pMdfDecodeFrame->ucGoldenRefIndex);

        pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
        tmp_Buffer.dwSize  = pFrameSource->dwWidth * pFrameSource->dwHeight * 3 / 2;
        tmp_Buffer.pBuffer  = intel_alloc_zero_aligned_memory(tmp_Buffer.dwSize, INTEL_HYBRID_VP9_PAGE_SIZE);

        if (tmp_Buffer.pBuffer == NULL)
	    goto allocation_fail;

	pFrameSource->Frame.pMdfSurface->ReadSurface(tmp_Buffer.pu8Buffer, pMdfDecodeFrame->pMdfEvent);
	intel_hybrid_Vp9Decode_WriteFileFromPtr("driver_dump\\ReferenceFrameGolden.dat", tmp_Buffer.pBuffer, tmp_Buffer.dwSize, g_iInterCounter);

        free(tmp_Buffer.pBuffer);
        tmp_Buffer.pBuffer = NULL;

        /* dump Alt Ref frame */
        surface = SURFACE(pMdfDecodeFrame->ucAltRefIndex);

        pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);

        tmp_Buffer.dwSize  = pFrameSource->dwWidth * pFrameSource->dwHeight * 3 / 2;
        tmp_Buffer.pBuffer  = intel_alloc_zero_aligned_memory(tmp_Buffer.dwSize, INTEL_HYBRID_VP9_PAGE_SIZE);

        if (tmp_Buffer.pBuffer == NULL)
	    goto allocation_fail;

	pFrameSource->Frame.pMdfSurface->ReadSurface(tmp_Buffer.pu8Buffer, pMdfDecodeFrame->pMdfEvent);
	intel_hybrid_Vp9Decode_WriteFileFromPtr("driver_dump\\ReferenceFrameAlt.dat", tmp_Buffer.pBuffer, tmp_Buffer.dwSize, g_iInterCounter);

	free(tmp_Buffer.pBuffer);

        tmp_Buffer.pBuffer = NULL;

	intel_hybrid_Vp9Decode_WriteFileFromPtr(
	    "driver_dump\\RefIndex_I8I8.dat", 
	    pMdfDecodeBuffer->ReferenceFrame.pBuffer, 
	    pMdfDecodeBuffer->ReferenceFrame.dwSize * sizeof(UINT16), 
	    g_iInterCounter);

	if (pMdfDecodeFrame->bSwitchableFilterType)
	{
	    intel_hybrid_Vp9Decode_WriteFileFromPtr(
		"driver_dump\\InterpFilterType_U8.dat", 
		pMdfDecodeBuffer->FilterType.pBuffer, 
		pMdfDecodeBuffer->FilterType.dwSize, 
		g_iInterCounter);
	}

	intel_hybrid_Vp9Decode_WriteFileFromPtr(
	    "driver_dump\\MotionVector_I16I16.dat", 
	    pMdfDecodeBuffer->MotionVector.pBuffer, 
	    pMdfDecodeBuffer->MotionVector.dwSize * sizeof(UINT64), 
	    g_iInterCounter);
    }

    return 0;

allocation_fail:
    return -ENOMEM;
}

VAStatus Intel_HybridVp9Decode_HostVldRenderCb (
    void *      pvStandardState, 
    uint32_t        uiCurrIndex, 
    uint32_t        uiPrevIndex)
{
    VADriverContextP	ctx;
    PINTEL_DECODE_HYBRID_VP9_STATE       pHybridVp9State;
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine;
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame, pMdfPreviousFrame;
    CmDevice                                *pMdfDevice;
    VAStatus                              eStatus = VA_STATUS_SUCCESS;

    MEDIA_DRV_CONTEXT *drv_ctx;
    PINTEL_DECODE_HYBRID_VP9_MDF_2D_BUFFER pCurrFrame;
    struct object_surface                *surface;
    INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *pFrameSource;

    pHybridVp9State     = (PINTEL_DECODE_HYBRID_VP9_STATE)pvStandardState;

    ctx = (VADriverContextP) (pHybridVp9State->driver_context);
    drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);

    pMdfDecodeEngine    = &pHybridVp9State->MdfDecodeEngine;
    pMdfDecodeFrame     = pMdfDecodeEngine->pMdfDecodeFrame + uiCurrIndex;
    pMdfPreviousFrame   = pMdfDecodeEngine->pMdfDecodeFrame + uiPrevIndex;
    pMdfDevice          = pMdfDecodeEngine->pMdfDevice;

    // Swap to get previous frame motion vector buffer, reference frame index buffer and segment ID buffer
    // Previous frame must be displayable
    if (pMdfDecodeFrame->bUseCollocatedMV)
    {
        PINTEL_DECODE_HYBRID_VP9_MDF_BUFFER      pMdfDecodeBuffer, pMdfPreviousBuffer;
        INTEL_DECODE_HYBRID_VP9_MDF_1D_BUFFER    sTempBuffer;

        pMdfDecodeBuffer   = &pMdfDecodeFrame->MdfDecodeBuffer;
        pMdfPreviousBuffer = &pMdfPreviousFrame->MdfDecodeBuffer;

        sTempBuffer = pMdfDecodeBuffer->PrevReferenceFrame;
        pMdfDecodeBuffer->PrevReferenceFrame = pMdfPreviousBuffer->ReferenceFrame;
        pMdfPreviousBuffer->ReferenceFrame   = sTempBuffer;

        sTempBuffer = pMdfDecodeBuffer->PrevMotionVector;
        pMdfDecodeBuffer->PrevMotionVector = pMdfPreviousBuffer->MotionVector;
        pMdfPreviousBuffer->MotionVector   = sTempBuffer;
    }

    // Select proper deblocking kernel and set kernel thread count if resolution changed.
    if (pMdfDecodeFrame->bResolutionChange)
    {
        Intel_HybridVp9Decode_MdfHost_PickDeblockKernel(pMdfDecodeEngine, pMdfDecodeFrame);
        Intel_HybridVp9Decode_MdfHost_SetKernelThreadCount(pMdfDecodeEngine, pMdfDecodeFrame);
        pMdfDecodeFrame->bResolutionChange = FALSE;

        // Re-allocate thread spaces before enqueue task
        Intel_HybridVp9Decode_MdfHost_DestroyThreadSpaces(pMdfDecodeEngine, pMdfDevice);
        Intel_HybridVp9Decode_MdfHost_CreateThreadSpaces(pMdfDecodeEngine, pMdfDecodeFrame, pMdfDevice);
    }

    // Reallocate residual buffers if they are not big enough
    if ((pMdfDecodeFrame->dwAlignedWidth  > pMdfDecodeEngine->Residue[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwWidth) ||
        (pMdfDecodeFrame->dwAlignedHeight > pMdfDecodeEngine->Residue[INTEL_HYBRID_VP9_MDF_YUV_PLANE_Y].dwHeight))
    {
        Intel_HybridVp9Decode_MdfHost_ReleaseResidue(pMdfDecodeEngine, pMdfDevice);
        Intel_HybridVp9Decode_MdfHost_AllocateResidue(
            pMdfDecodeEngine, pMdfDevice, pMdfDecodeFrame->dwAlignedWidth, pMdfDecodeFrame->dwAlignedHeight);
    }

    // Reset padding flag of current frame and update surface dimension
    surface = SURFACE(pMdfDecodeFrame->ucCurrIndex);
    if ((surface == NULL) || (surface->private_data == NULL))
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    pFrameSource = (INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *)(surface->private_data);
    pCurrFrame = &(pFrameSource->Frame);
    pFrameSource->bHasPadding = false;

    if (pCurrFrame->pMdfSurface == NULL)
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    pCurrFrame->pMdfSurface->SetSurfaceStateDimensions(
        pMdfDecodeFrame->dwWidth,
        pMdfDecodeFrame->dwHeight);

    if (g_intel_debug_option_flags & VA_INTEL_HYBRID_PRE_DUMP)
    {
        intel_hybrid_Vp9Decode_DebugDump(pHybridVp9State, pMdfDecodeFrame);
    }

    // execute MDFHost
    Intel_HybridVp9Decode_MdfHost_Execute(ctx, pMdfDecodeEngine, pMdfDecodeFrame);

    if (g_intel_debug_option_flags & VA_INTEL_HYBRID_POST_DUMP)
    {
        intel_hybrid_Vp9Decode_DebugDump(pHybridVp9State, pMdfDecodeFrame);
    }

    return eStatus;
}

VAStatus Intel_HybridVp9Decode_HostVldSyncResourceCb (
    void                               *pvStandardState, 
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pHostVldVideoBuf, 
    unsigned int                                uiCurrIndex, 
    unsigned int                                uiPrevIndex)
{
    PINTEL_DECODE_HYBRID_VP9_STATE       pHybridVp9State;
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER     pHostVldOutputBuf, pHostVldPrevOutBuf;
    PINTEL_DECODE_HYBRID_VP9_MDF_ENGINE  pMdfDecodeEngine;
    PINTEL_DECODE_HYBRID_VP9_MDF_FRAME   pMdfDecodeFrame, pMdfPreviousFrame;
    PINTEL_VP9_PIC_PARAMS                pVp9PicParams;
    PINTEL_HOSTVLD_VP9_1D_BUFFER         pBuffer;
    uint32_t                                   dwWidth;
    uint32_t                                   dwHeight;
    uint32_t                                   dwBufferSize;
    VAStatus                              eStatus = VA_STATUS_SUCCESS;


    pHybridVp9State     = (PINTEL_DECODE_HYBRID_VP9_STATE)pvStandardState;
    pMdfDecodeEngine    = &pHybridVp9State->MdfDecodeEngine;
    pMdfDecodeFrame     = pMdfDecodeEngine->pMdfDecodeFrame + uiCurrIndex;
    pMdfPreviousFrame   = pMdfDecodeEngine->pMdfDecodeFrame + uiPrevIndex;
    pHostVldOutputBuf   = pHybridVp9State->pHostVldOutputBuf + uiCurrIndex;
    pHostVldPrevOutBuf  = pHybridVp9State->pHostVldOutputBuf + uiPrevIndex;
    pVp9PicParams       = pHostVldVideoBuf->pVp9PicParams;
    dwWidth             = ALIGN(pVp9PicParams->FrameWidthMinus1 + 1,  INTEL_HYBRID_VP9_B8_SIZE);
    dwHeight            = ALIGN(pVp9PicParams->FrameHeightMinus1 + 1, INTEL_HYBRID_VP9_B8_SIZE);

    // make sure the last frame is not using the MDF host buffers
    Intel_HybridVp9Decode_MdfHost_SyncResource(pMdfDecodeFrame);

    pMdfDecodeFrame->CurrPic            = pVp9PicParams->CurrPic;
    pMdfDecodeFrame->ucCurrIndex        = pVp9PicParams->CurrPic;
	/* Use the VASurfaceID directly to simplify the RefFrameList logic */
    pMdfDecodeFrame->ucLastRefIndex     = pVp9PicParams->RefFrameList[pVp9PicParams->PicFlags.fields.LastRefIdx];
    pMdfDecodeFrame->ucGoldenRefIndex   = pVp9PicParams->RefFrameList[pVp9PicParams->PicFlags.fields.GoldenRefIdx];
    pMdfDecodeFrame->ucAltRefIndex      = pVp9PicParams->RefFrameList[pVp9PicParams->PicFlags.fields.AltRefIdx];

    pMdfDecodeFrame->bNeedDeblock       = pVp9PicParams->filter_level > 0;
    pMdfDecodeFrame->bShowFrame         = pVp9PicParams->PicFlags.fields.show_frame;
    pMdfDecodeFrame->bLossless          = pVp9PicParams->PicFlags.fields.LosslessFlag;
    pMdfDecodeFrame->bIntraOnly         =         
        (pVp9PicParams->PicFlags.fields.frame_type == INTEL_HYBRID_VP9_KEY_FRAME) || 
        (pVp9PicParams->PicFlags.fields.intra_only);
    pMdfDecodeFrame->bSwitchableFilterType = pVp9PicParams->PicFlags.fields.mcomp_filter_type == 4;   
    pMdfDecodeFrame->dwInterpolationFilterType      = pVp9PicParams->PicFlags.fields.mcomp_filter_type;
    pMdfDecodeFrame->uiStatusReportFeedbackNumber   = pVp9PicParams->StatusReportFeedbackNumber;

    pMdfDecodeFrame->bPrevShowFrame     = pMdfPreviousFrame->bShowFrame;

    if ((pMdfDecodeFrame->dwWidth  != dwWidth) ||
        (pMdfDecodeFrame->dwHeight != dwHeight))
    {
        // need to update current frame information
        pMdfDecodeFrame->dwWidth            = dwWidth;
        pMdfDecodeFrame->dwHeight           = dwHeight;
        pMdfDecodeFrame->dwAlignedWidth     = ALIGN(dwWidth, INTEL_HYBRID_VP9_B64_SIZE);
        pMdfDecodeFrame->dwAlignedHeight    = ALIGN(dwHeight, INTEL_HYBRID_VP9_B64_SIZE);
        pMdfDecodeFrame->dwWidthB8          = dwWidth >> INTEL_HYBRID_VP9_LOG2_B8_SIZE;   // may not be SB64 aligned
        pMdfDecodeFrame->dwHeightB8         = dwHeight >> INTEL_HYBRID_VP9_LOG2_B8_SIZE;  // may not be SB64 aligned
        pMdfDecodeFrame->dwWidthB16         = pMdfDecodeFrame->dwAlignedWidth >> 4;
        pMdfDecodeFrame->dwHeightB16        = pMdfDecodeFrame->dwAlignedHeight >> 4;
        pMdfDecodeFrame->dwWidthB32         = pMdfDecodeFrame->dwAlignedWidth >> 5;
        pMdfDecodeFrame->dwHeightB32        = pMdfDecodeFrame->dwAlignedHeight >> 5;
        pMdfDecodeFrame->dwWidthB64         = pMdfDecodeFrame->dwAlignedWidth >> 6;
        pMdfDecodeFrame->dwHeightB64        = pMdfDecodeFrame->dwAlignedHeight >> 6;

        if ((pMdfDecodeFrame->dwWidth  > pMdfDecodeFrame->dwMaxWidth) ||
            (pMdfDecodeFrame->dwHeight > pMdfDecodeFrame->dwMaxHeight))
        {
            pMdfDecodeFrame->dwMaxWidth  = dwWidth;
            pMdfDecodeFrame->dwMaxHeight = dwHeight;

            // Reallocate host buffers of current frame if resolution changed
            Intel_HybridVp9Decode_MdfHost_ReallocateCurrentFrame(
		pHybridVp9State,
                pMdfDecodeFrame, pMdfDecodeEngine->pMdfDevice);

            // update HostVLD output buffers
            Intel_HybridVp9Decode_SetHostBuffers(pHybridVp9State, uiCurrIndex);
        }
    }

    // If motion vector buffer and reference frame index buffer are not big enough, reallocate them.
    dwBufferSize = (pMdfDecodeFrame->dwAlignedWidth >> 2) * (pMdfDecodeFrame->dwAlignedHeight >> 2);
    if (pMdfDecodeFrame->MdfDecodeBuffer.MotionVector.dwSize < dwBufferSize)
    {
        Intel_HybridVp9Decode_MdfHost_ReallocateMvBuffer(
	    pHybridVp9State,
            pMdfDecodeFrame, pMdfDecodeEngine->pMdfDevice);
        pHostVldOutputBuf->MotionVector.pu8Buffer   = pMdfDecodeFrame->MdfDecodeBuffer.MotionVector.pu8Buffer;
        pHostVldOutputBuf->MotionVector.dwSize      = pMdfDecodeFrame->MdfDecodeBuffer.MotionVector.dwSize;
    }

    dwBufferSize >>= 2;
    if (pMdfDecodeFrame->MdfDecodeBuffer.ReferenceFrame.dwSize < dwBufferSize)
    {
        Intel_HybridVp9Decode_MdfHost_ReallocateRefFrameIndexBuffer(
	    pHybridVp9State,
            pMdfDecodeFrame, pMdfDecodeEngine->pMdfDevice);
        pHostVldOutputBuf->ReferenceFrame.pu8Buffer = pMdfDecodeFrame->MdfDecodeBuffer.ReferenceFrame.pu8Buffer;
        pHostVldOutputBuf->ReferenceFrame.dwSize    = pMdfDecodeFrame->MdfDecodeBuffer.ReferenceFrame.dwSize;
    }


    pMdfDecodeFrame->bResolutionChange =
        ((pMdfDecodeFrame->dwWidth != pMdfPreviousFrame->dwWidth) ||
        (pMdfDecodeFrame->dwHeight != pMdfPreviousFrame->dwHeight));

    pMdfDecodeFrame->bUseCollocatedMV = !pMdfDecodeFrame->bResolutionChange &&
                                          pMdfDecodeFrame->bPrevShowFrame   &&
                                          !pMdfDecodeFrame->bIntraOnly      &&
                                          !pVp9PicParams->PicFlags.fields.error_resilient_mode;

    // Swap to get previous frame motion vector buffer and reference frame index buffer
    if ((pMdfDecodeFrame->bUseCollocatedMV))
    {
        PINTEL_DECODE_HYBRID_VP9_MDF_BUFFER  pMdfDecodeBuffer, pMdfPreviousBuffer;

        pMdfDecodeBuffer   = &pMdfDecodeFrame->MdfDecodeBuffer;
        pMdfPreviousBuffer = &pMdfPreviousFrame->MdfDecodeBuffer;

        // Swap reference frame index buffer
        pHostVldVideoBuf->PrevReferenceFrame.pu16Buffer = pMdfPreviousBuffer->ReferenceFrame.pu16Buffer;
        pHostVldVideoBuf->PrevReferenceFrame.dwSize = pMdfPreviousBuffer->ReferenceFrame.dwSize;
        pHostVldPrevOutBuf->ReferenceFrame.pu16Buffer = pMdfDecodeBuffer->PrevReferenceFrame.pu16Buffer;
        pHostVldPrevOutBuf->ReferenceFrame.dwSize = pMdfDecodeBuffer->PrevReferenceFrame.dwSize;

        // Swap motion vector buffer
        pHostVldVideoBuf->PrevMotionVector.pu8Buffer = pMdfPreviousBuffer->MotionVector.pu8Buffer;
        pHostVldVideoBuf->PrevMotionVector.dwSize = pMdfPreviousBuffer->MotionVector.dwSize;
        pHostVldPrevOutBuf->MotionVector.pu8Buffer = pMdfDecodeBuffer->PrevMotionVector.pu8Buffer;
        pHostVldPrevOutBuf->MotionVector.dwSize = pMdfDecodeBuffer->PrevMotionVector.dwSize;
    }

    // Reset coefficient status buffer. leave it here for now, should change hostvld code to zero status for skipped block.
    pBuffer = &pHostVldOutputBuf->CoeffStatus[INTEL_HOSTVLD_VP9_YUV_PLANE_Y];
    memset(pBuffer->pu8Buffer, 0, pBuffer->dwSize * sizeof (uint8_t));
    pBuffer = &pHostVldOutputBuf->CoeffStatus[INTEL_HOSTVLD_VP9_YUV_PLANE_UV];
    memset(pBuffer->pu8Buffer, 0, pBuffer->dwSize * sizeof (uint8_t));

    return eStatus;
}

VAStatus Intel_HybridVp9Decode_AllocateResources (
    VADriverContextP ctx, 
    PINTEL_DECODE_HYBRID_VP9_STATE pHybridVp9State)
{
    INTEL_HOSTVLD_VP9_CALLBACKS  HostVldCallbacks;
    VAStatus                      eStatus = VA_STATUS_SUCCESS;

    // Create HostVLD
    HostVldCallbacks.pvStandardState        = pHybridVp9State;
    HostVldCallbacks.pfnHostVldRenderCb     = Intel_HybridVp9Decode_HostVldRenderCb;
    HostVldCallbacks.pfnHostVldSyncCb       = Intel_HybridVp9Decode_HostVldSyncResourceCb;

    eStatus = Intel_HostvldVp9_Create(
        &pHybridVp9State->hHostVld, 
        &HostVldCallbacks);

    if (eStatus != VA_STATUS_SUCCESS)
	goto error_status;

    eStatus = Intel_HostvldVp9_QueryBufferSize(
        pHybridVp9State->hHostVld, 
        &pHybridVp9State->dwMdfBufferSize);

    if (eStatus != VA_STATUS_SUCCESS)
	goto error_status;

    // Create MDF resources for kernels
    eStatus = Intel_HybridVp9Decode_MdfHost_Create(ctx, pHybridVp9State);

    if (eStatus != VA_STATUS_SUCCESS)
	goto error_status;

    // HostVld output buffers
    pHybridVp9State->pHostVldOutputBuf = 
        (PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER) calloc(
        pHybridVp9State->dwMdfBufferSize, sizeof(*pHybridVp9State->pHostVldOutputBuf));

    eStatus = Intel_HostvldVp9_SetOutputBuffer(
        pHybridVp9State->hHostVld, 
        pHybridVp9State->pHostVldOutputBuf);

error_status:
    return eStatus;
}

void Intel_HybridVp9Decode_Destroy(
    void *hw_context)
{
    
    PINTEL_DECODE_HYBRID_VP9_STATE pHybridVp9State;
    hybrid_vp9_hw_context *vp9_context = (hybrid_vp9_hw_context *) hw_context;

    pHybridVp9State = &vp9_context->vp9_state;

    // destroy HostVLD
    if (pHybridVp9State->hHostVld)
    {
        Intel_HostvldVp9_Destroy(pHybridVp9State->hHostVld);
        pHybridVp9State->hHostVld = NULL;
    }

    free(pHybridVp9State->pHostVldOutputBuf);
    pHybridVp9State->pHostVldOutputBuf = NULL;

    // destroy MDFHost
    Intel_HybridVp9Decode_MdfHost_Destroy(&pHybridVp9State->MdfDecodeEngine);

    if (pHybridVp9State->pHostVldOutputBuf)
    {
    	free(pHybridVp9State->pHostVldOutputBuf);
        pHybridVp9State->pHostVldOutputBuf = NULL;
    }

    free(pHybridVp9State->pCopyDataBuffer);
    pHybridVp9State->pCopyDataBuffer = NULL;

    free(hw_context);
    hw_context = NULL;
    
    return;
}


static void 
codechal_free_surface(void **data)
{
    void *frame_data;

    frame_data = *data;

    if (!frame_data) {
        return;
    }

    free(frame_data);
    *data = NULL;
}

static VAStatus codechal_allocate_frame_source(struct object_surface *surface)
{
    INTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE *pFrameSource;
    VAStatus eStatus = VA_STATUS_SUCCESS;

    if (surface->free_private_data && surface->private_data) {
        /* check whether it is codechal_free_surface. If not, free it.
         * If yes, return
         */
	if (surface->free_private_data == codechal_free_surface)
	    return eStatus;

        surface->free_private_data(&surface->private_data);
        surface->free_private_data = NULL; 
    }
    
    pFrameSource = (PINTEL_DECODE_HYBRID_VP9_MDF_FRAME_SOURCE)calloc(1, sizeof(*pFrameSource));
    if (pFrameSource == NULL) {
        eStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
	return eStatus;
    }
    surface->free_private_data = codechal_free_surface;
    surface->private_data = pFrameSource;

    return eStatus;
}

VAStatus Intel_HybridVp9_DecodeInitialize(
    union codec_state *codec_state,
    PINTEL_DECODE_HYBRID_VP9_STATE       pHybridVp9State,
    void *hw_context)
{
    PINTEL_VP9_PIC_PARAMS                    pVp9PicParams;
    PINTEL_VP9_SEGMENT_PARAMS                pVp9SegmentParams;
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER          pHostVldVideoBuffer;
    VAStatus                                  eStatus = VA_STATUS_SUCCESS;
    hybrid_vp9_hw_context *vp9_context = (hybrid_vp9_hw_context *) hw_context;
    struct decode_state *decode_state = &codec_state->decode;
    dri_bo	*slice_data_bo;

    pHostVldVideoBuffer = &pHybridVp9State->HostVldVideoBuf;
    pVp9PicParams       = (PINTEL_VP9_PIC_PARAMS)&vp9_context->vp9_pic_params;
    pVp9SegmentParams   = (PINTEL_VP9_SEGMENT_PARAMS)&vp9_context->vp9_matrixbuffer;

    pHybridVp9State->sDestSurface       = vp9_context->sDestSurface;
    pHybridVp9State->pVp9PicParams      = pVp9PicParams;
    pHybridVp9State->dwDataSize         = 0;
    pHybridVp9State->dwCropWidth        = pVp9PicParams->FrameWidthMinus1 + 1;
    pHybridVp9State->dwCropHeight       = pVp9PicParams->FrameHeightMinus1 + 1;
    pHybridVp9State->dwWidth            = ALIGN(pHybridVp9State->dwCropWidth, INTEL_HYBRID_VP9_B8_SIZE);  // 8 aligned width
    pHybridVp9State->dwHeight           = ALIGN(pHybridVp9State->dwCropHeight, INTEL_HYBRID_VP9_B8_SIZE); // 8 aligned height
    pHybridVp9State->ucCurrIndex        = pVp9PicParams->CurrPic;

    codechal_allocate_frame_source(pHybridVp9State->sDestSurface);

    // Initialize/Update MDF Host
    Intel_HybridVp9Decode_MdfHost_Initialize(pHybridVp9State);

    // Initialize HostVLD
    pHostVldVideoBuffer->pVp9PicParams      = pVp9PicParams;
    pHostVldVideoBuffer->pVp9SegmentData    = pVp9SegmentParams;
    pHostVldVideoBuffer->dwBitsSize         = 0;
    pHostVldVideoBuffer->pRenderTarget      = pHybridVp9State->sDestSurface;
    pHostVldVideoBuffer->bResolutionChanged = pHybridVp9State->MdfDecodeEngine.bResolutionChanged;

    slice_data_bo = decode_state->slice_datas[0]->bo;

    if (slice_data_bo) {
	dri_bo_map(slice_data_bo, 0);
	pHostVldVideoBuffer->slice_data_bo = slice_data_bo;
	pHostVldVideoBuffer->pbBitsData = (uint8_t *)slice_data_bo->virt;
	pHostVldVideoBuffer->dwBitsSize = pVp9PicParams->BSBytesInBuffer;
    }

    Intel_HostvldVp9_Initialize(
        pHybridVp9State->hHostVld, 
        pHostVldVideoBuffer);

    Intel_HybridVp9Decode_MdfHost_UpdateResolution(
        pHybridVp9State,
        &pHybridVp9State->MdfDecodeEngine,
        pHybridVp9State->sDestSurface,
        pHybridVp9State->ucCurrIndex,
        pHybridVp9State->dwCropWidth,
        pHybridVp9State->dwCropHeight);

    return eStatus;
}

VAStatus Intel_HybridVp9_DecodePictureLevel(
    PINTEL_DECODE_HYBRID_VP9_STATE       pHybridVp9State,
    void *hw_context)
{
    return VA_STATUS_SUCCESS;
}

VAStatus Intel_HybridVp9_DecodeSliceLevel(
    PINTEL_DECODE_HYBRID_VP9_STATE       pHybridVp9State,
    void *hw_context)
{
    VAStatus                              eStatus = VA_STATUS_SUCCESS;

    // execute HostVLD to parse bitstream and prepare MDF resources for kernels
    Intel_HostvldVp9_Execute(pHybridVp9State->hHostVld);

    return eStatus;
}

#define SUBSAMPLE_YUV420	1

static VAStatus
intel_hybrid_vp9_check_rendertarget(VADriverContextP ctx, 
                        union codec_state *codec_state,
                        struct hw_context *hw_context)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct decode_state *decode_state = &codec_state->decode;
    struct object_surface *obj_surface;
    hybrid_vp9_hw_context *vp9_context = (hybrid_vp9_hw_context *) hw_context;
    
    if (decode_state->current_render_target == VA_INVALID_SURFACE)
	return VA_STATUS_ERROR_INVALID_PARAMETER;

    obj_surface = SURFACE(decode_state->current_render_target);
 
    media_alloc_surface_bo(ctx, obj_surface, 1, VA_FOURCC_NV12, SUBSAMPLE_YUV420);

    vp9_context->sDestSurface = obj_surface;

    return VA_STATUS_SUCCESS;
}

static VAStatus 
intel_hybrid_vp9_convert_picture(VADriverContextP ctx, 
                        union codec_state *codec_state,
                        struct hw_context *hw_context)
{
    hybrid_vp9_hw_context *vp9_context = (hybrid_vp9_hw_context *) hw_context;
    struct decode_state *decode_state = &codec_state->decode;
    PINTEL_VP9_PIC_PARAMS                    pVp9PicParams;
    int                          i;
    VADecPictureParameterBufferVP9 *pPP;

    pVp9PicParams       = (PINTEL_VP9_PIC_PARAMS)&vp9_context->vp9_pic_params;


    pPP           = (VADecPictureParameterBufferVP9 *)(decode_state->pic_param->buffer);

    if (pPP == NULL)
    {
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    pVp9PicParams->FrameHeightMinus1                            = pPP->frame_height - 1;
    pVp9PicParams->FrameWidthMinus1                             = pPP->frame_width - 1;

    pVp9PicParams->PicFlags.fields.frame_type                   = pPP->pic_fields.bits.frame_type;
    pVp9PicParams->PicFlags.fields.show_frame                   = pPP->pic_fields.bits.show_frame;
    pVp9PicParams->PicFlags.fields.error_resilient_mode         = pPP->pic_fields.bits.error_resilient_mode;
    pVp9PicParams->PicFlags.fields.intra_only                   = pPP->pic_fields.bits.intra_only;
    pVp9PicParams->PicFlags.fields.LastRefIdx                   = pPP->pic_fields.bits.last_ref_frame;
    pVp9PicParams->PicFlags.fields.LastRefSignBias              = pPP->pic_fields.bits.last_ref_frame_sign_bias;
    pVp9PicParams->PicFlags.fields.GoldenRefIdx                 = pPP->pic_fields.bits.golden_ref_frame;    
    pVp9PicParams->PicFlags.fields.GoldenRefSignBias            = pPP->pic_fields.bits.golden_ref_frame_sign_bias;
    pVp9PicParams->PicFlags.fields.AltRefIdx                    = pPP->pic_fields.bits.alt_ref_frame;    
    pVp9PicParams->PicFlags.fields.AltRefSignBias               = pPP->pic_fields.bits.alt_ref_frame_sign_bias;
    pVp9PicParams->PicFlags.fields.allow_high_precision_mv      = pPP->pic_fields.bits.allow_high_precision_mv;
    pVp9PicParams->PicFlags.fields.mcomp_filter_type            = pPP->pic_fields.bits.mcomp_filter_type;
    pVp9PicParams->PicFlags.fields.frame_parallel_decoding_mode = pPP->pic_fields.bits.frame_parallel_decoding_mode;
    pVp9PicParams->PicFlags.fields.segmentation_enabled         = pPP->pic_fields.bits.segmentation_enabled;
    pVp9PicParams->PicFlags.fields.segmentation_temporal_update = pPP->pic_fields.bits.segmentation_temporal_update;
    pVp9PicParams->PicFlags.fields.segmentation_update_map      = pPP->pic_fields.bits.segmentation_update_map;
    pVp9PicParams->PicFlags.fields.reset_frame_context          = pPP->pic_fields.bits.reset_frame_context;
    pVp9PicParams->PicFlags.fields.refresh_frame_context        = pPP->pic_fields.bits.refresh_frame_context;
    pVp9PicParams->PicFlags.fields.frame_context_idx            = pPP->pic_fields.bits.frame_context_idx;
    pVp9PicParams->PicFlags.fields.LosslessFlag                 = pPP->pic_fields.bits.lossless_flag;

    for (i = 0; i < 8; i++)
    {
        pVp9PicParams->RefFrameList[i]                 = pPP->reference_frames[i];
    }

    pVp9PicParams->filter_level                                 = pPP->filter_level;
    pVp9PicParams->sharpness_level                              = pPP->sharpness_level;
    pVp9PicParams->log2_tile_rows                               = pPP->log2_tile_rows;
    pVp9PicParams->log2_tile_columns                            = pPP->log2_tile_columns;
    pVp9PicParams->UncompressedHeaderLengthInBytes              = pPP->frame_header_length_in_bytes;
    pVp9PicParams->FirstPartitionSize                           = pPP->first_partition_size;


    pVp9PicParams->CurrPic = decode_state->current_render_target;
    memcpy(pVp9PicParams->SegTreeProbs, pPP->mb_segment_tree_probs, 7);
    memcpy(pVp9PicParams->SegPredProbs, pPP->segment_pred_probs, 3);


    if (pPP->first_partition_size == 0) {
	return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
    return VA_STATUS_SUCCESS;
}

static VAStatus 
intel_hybrid_vp9_convert_slice(VADriverContextP ctx, 
                        union codec_state *codec_state,
                        struct hw_context *hw_context)
{
    hybrid_vp9_hw_context *vp9_context = (hybrid_vp9_hw_context *) hw_context;
    struct decode_state *decode_state = &codec_state->decode;
    PINTEL_VP9_PIC_PARAMS                    pVp9PicParams;
    PINTEL_VP9_SEGMENT_PARAMS                pVp9SegmentParams;
    VASliceParameterBufferVP9    *pSegData;
    int                          i,j,k;
    uint32_t                     buffer_size;

    pVp9PicParams       = (PINTEL_VP9_PIC_PARAMS)&vp9_context->vp9_pic_params;
    pVp9SegmentParams   = (PINTEL_VP9_SEGMENT_PARAMS)&vp9_context->vp9_matrixbuffer;

    if (decode_state->num_slice_params != 1) {
	if (decode_state->num_slice_params == 0) {
	    /* there is no slice_param. */
	    return VA_STATUS_ERROR_INVALID_PARAMETER;
        }
        if (decode_state->num_slice_params) {
	    /* there are too many params */
	    return VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
        }
    }

    pSegData = (VASliceParameterBufferVP9 *)(decode_state->slice_params[0]->buffer);

    if (pSegData == NULL)
    {
	/* Null slice_parameter */
	return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    pVp9PicParams->BSBytesInBuffer = pSegData->slice_data_size;

    for (i = 0; i < 8; i++)
    {
        pVp9SegmentParams->SegData[i].SegmentFlags.fields.SegmentReferenceEnabled = pSegData->seg_param[i].segment_flags.fields.segment_reference_enabled;
        pVp9SegmentParams->SegData[i].SegmentFlags.fields.SegmentReference        = pSegData->seg_param[i].segment_flags.fields.segment_reference;
        pVp9SegmentParams->SegData[i].SegmentFlags.fields.SegmentReferenceSkipped = pSegData->seg_param[i].segment_flags.fields.segment_reference_skipped;

        for (j = 0; j < 4; j++)
        {
            for (k = 0; k < 2; k++)
            {
                pVp9SegmentParams->SegData[i].FilterLevel[j][k] = pSegData->seg_param[i].filter_level[j][k];
            }
        }

        pVp9SegmentParams->SegData[i].LumaACQuantScale          = pSegData->seg_param[i].luma_ac_quant_scale;
        pVp9SegmentParams->SegData[i].LumaDCQuantScale          = pSegData->seg_param[i].luma_dc_quant_scale;
        pVp9SegmentParams->SegData[i].ChromaACQuantScale        = pSegData->seg_param[i].chroma_ac_quant_scale;
        pVp9SegmentParams->SegData[i].ChromaDCQuantScale        = pSegData->seg_param[i].chroma_dc_quant_scale;
    }

    buffer_size = (uint32_t) pVp9PicParams->FirstPartitionSize +
                  (uint32_t) pVp9PicParams->UncompressedHeaderLengthInBytes;
    if (buffer_size >= pVp9PicParams->BSBytesInBuffer) {
	return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
    return VA_STATUS_SUCCESS;
}


 VAStatus
intel_hybrid_decode_picture(VADriverContextP ctx, 
                        VAProfile profile, 
                        union codec_state *codec_state,
                        struct hw_context *hw_context)

{
    PINTEL_DECODE_HYBRID_VP9_STATE  pHybridVp9State;
    VAStatus                             eStatus = VA_STATUS_SUCCESS;
    hybrid_vp9_hw_context *vp9_context = (hybrid_vp9_hw_context *) hw_context;
    
    pHybridVp9State = &vp9_context->vp9_state;

    /* Assure that the render_target surface is initialized */
    eStatus = intel_hybrid_vp9_check_rendertarget(ctx, codec_state, hw_context);

    if (eStatus != VA_STATUS_SUCCESS)
	goto error_status;

    /* Implement the conversion from VP9 parameter in codec_state to Hybrid_state->VP9 parameter */
    /* convert picture parameter */

    eStatus = intel_hybrid_vp9_convert_picture(ctx, codec_state, hw_context);

    if (eStatus != VA_STATUS_SUCCESS)
	goto error_status;

    /* Convert slice_parameter */
    eStatus = intel_hybrid_vp9_convert_slice(ctx, codec_state, hw_context);

    if (eStatus != VA_STATUS_SUCCESS)
	goto error_status;

    eStatus = Intel_HybridVp9_DecodeInitialize(codec_state, pHybridVp9State, hw_context);

    if (eStatus != VA_STATUS_SUCCESS)
	goto error_status;

    eStatus = Intel_HybridVp9_DecodePictureLevel(pHybridVp9State, hw_context);
 
    if (eStatus != VA_STATUS_SUCCESS)
	goto error_status;

    eStatus = Intel_HybridVp9_DecodeSliceLevel(pHybridVp9State, hw_context);
    
    return eStatus;
error_status: 
    return eStatus;
}

VAStatus Intel_HybridVp9Decode_Initialize(
    VADriverContextP ctx, 
    void *hw_context)
{
    PINTEL_DECODE_HYBRID_VP9_STATE  pHybridVp9State;
    VAStatus                             eStatus = VA_STATUS_SUCCESS;
    hybrid_vp9_hw_context *vp9_context = (hybrid_vp9_hw_context *) hw_context;
    
    pHybridVp9State = &vp9_context->vp9_state;
    
    /* No status reporting */
    pHybridVp9State->bStatusReportingEnabled = 0;
    pHybridVp9State->pDecodeStatusBuf   = NULL;

    /* Record the VADriverContextP so that the SURFACE(x) can be used */
    pHybridVp9State->driver_context = ctx;

    vp9_context->context.destroy 	=  Intel_HybridVp9Decode_Destroy;
    vp9_context->context.run		=  intel_hybrid_decode_picture;

    pHybridVp9State->dwThreadNumber = 1;
    eStatus = Intel_HybridVp9Decode_AllocateResources(ctx, pHybridVp9State);

    return eStatus;
}

