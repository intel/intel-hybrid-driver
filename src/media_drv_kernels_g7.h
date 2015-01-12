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

#ifndef _MEDIA__DRIVER_KERNELS_G7_H
#define _MEDIA__DRIVER_KERNELS_G7_H
#include "media_drv_defines.h"

#define MEDIA_VP8_MBENC_I_SZ_G7 0x62c0
extern const UINT MEDIA_VP8_MBENC_I_G7[MEDIA_VP8_MBENC_I_SZ_G7];

#define MEDIA_VP8_MBENC_ICHROMA_SZ_G7 0xe300	//0xe310
extern const UINT MEDIA_VP8_MBENC_ICHROMA_G7[MEDIA_VP8_MBENC_ICHROMA_SZ_G7];

#define MEDIA_VP8_MBENC_FRM_P_SZ_G7 0xb1c0
extern const UINT MEDIA_VP8_MBENC_FRM_P_G7[MEDIA_VP8_MBENC_FRM_P_SZ_G7];

#define MEDIA_VP8_PAK_PHASE1_SZ_G7 0x7940
extern const UINT MEDIA_VP8_PAK_PHASE1_G7[MEDIA_VP8_PAK_PHASE1_SZ_G7];

#define MEDIA_VP8_PAK_PHASE2_SZ_G7 0x9af0
extern const UINT MEDIA_VP8_PAK_PHASE2_G7[MEDIA_VP8_PAK_PHASE2_SZ_G7];

#define MEDIA_VP8_INTRA_DIS_BRC_SZ_G7 0xe00
extern const UINT MEDIA_VP8_INTRA_DIS_BRC_G7[MEDIA_VP8_INTRA_DIS_BRC_SZ_G7];

#define MEDIA_VP8_BRC_INIT_SZ_G7 0x17c0
extern const UINT MEDIA_VP8_BRC_INIT_G7[MEDIA_VP8_BRC_INIT_SZ_G7];

#define MEDIA_VP8_BRC_RESET_SZ_G7 0x1a00
extern const UINT MEDIA_VP8_BRC_RESET_G7[MEDIA_VP8_BRC_RESET_SZ_G7];

#define MEDIA_VP8_BRC_UPDATE_SZ_G7 0x8a80
extern const UINT MEDIA_VP8_BRC_UPDATE_G7[MEDIA_VP8_BRC_UPDATE_SZ_G7];

#define MEDIA_VP8_MBENC_ILuma_SZ_G7 16
extern const UINT MEDIA_VP8_MBENC_ILuma_G7[MEDIA_VP8_MBENC_ILuma_SZ_G7];

#define MEDIA_VP8_HME_P_SZ_G7 0x900
extern const UINT MEDIA_VP8_HME_P_G7[MEDIA_VP8_HME_P_SZ_G7];

#define MEDIA_VP8_HME_DOWNSCALE_SZ_G7 0xf80
extern const UINT MEDIA_VP8_HME_DOWNSCALE_G7[MEDIA_VP8_HME_DOWNSCALE_SZ_G7];

#endif
