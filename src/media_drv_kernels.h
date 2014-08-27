/*
 * Copyright ©  2014 Intel Corporation
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
 *    Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 *
 */


#ifndef _MEDIA__DRIVER_KERNELS_H
#define _MEDIA__DRIVER_KERNELS_H
#include "media_drv_defines.h"
#define MEDIA_VP8_MBENC_I_SZ 0x5c40
extern const UINT MEDIA_VP8_MBENC_I[MEDIA_VP8_MBENC_I_SZ];

#define MEDIA_VP8_MBENC_ICHROMA_SZ 0xa300
extern const UINT MEDIA_VP8_MBENC_ICHROMA[MEDIA_VP8_MBENC_ICHROMA_SZ];

#define MEDIA_VP8_MBENC_FRM_P_SZ 0x6f40	//0x10750
extern const UINT MEDIA_VP8_MBENC_FRM_P[MEDIA_VP8_MBENC_FRM_P_SZ];


#define MEDIA_VP8_MBENC_ILuma_SZ 0xb9f0
extern const UINT MEDIA_VP8_MBENC_ILuma[MEDIA_VP8_MBENC_ILuma_SZ];


#define MEDIA_VP8_HME_P_SZ 0x1190
extern const UINT MEDIA_VP8_HME_P[MEDIA_VP8_HME_P_SZ];


#define MEDIA_VP8_HME_DOWNSCALE_SZ 0xf10
extern const UINT MEDIA_VP8_HME_DOWNSCALE[MEDIA_VP8_HME_DOWNSCALE_SZ];

#define MEDIA_VP8_PAK_PHASE1_SZ 0x78d0
extern const UINT MEDIA_VP8_PAK_PHASE1[MEDIA_VP8_PAK_PHASE1_SZ];

#define MEDIA_VP8_PAK_PHASE2_SZ 0x9a80
extern const UINT MEDIA_VP8_PAK_PHASE2[MEDIA_VP8_PAK_PHASE2_SZ];


#define MEDIA_VP8_INTRA_DIS_BRC_SZ 0xa10
extern const UINT MEDIA_VP8_INTRA_DIS_BRC[MEDIA_VP8_INTRA_DIS_BRC_SZ];

#define MEDIA_VP8_BRC_INIT_SZ 0x17c0
extern const UINT MEDIA_VP8_BRC_INIT[MEDIA_VP8_BRC_INIT_SZ];


#define MEDIA_VP8_BRC_RESET_SZ 0x19f0
extern const UINT MEDIA_VP8_BRC_RESET[MEDIA_VP8_BRC_RESET_SZ];

#define MEDIA_VP8_BRC_UPDATE_SZ 0x8980
extern const UINT MEDIA_VP8_BRC_UPDATE[MEDIA_VP8_BRC_UPDATE_SZ];
#endif
