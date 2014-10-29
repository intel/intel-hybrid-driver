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
#ifndef _MEDIA__DRIVER_KERNELS_G8_H
#define _MEDIA__DRIVER_KERNELS_G8_H

#include "media_drv_defines.h"
#define MEDIA_VP8_MBENC_I_SZ_G8 0x5c00 //0x5ba0
extern const UINT MEDIA_VP8_MBENC_I_G8[MEDIA_VP8_MBENC_I_SZ_G8];

#define MEDIA_VP8_MBENC_ICHROMA_SZ_G8 0x9e00
extern const UINT MEDIA_VP8_MBENC_ICHROMA_G8[MEDIA_VP8_MBENC_ICHROMA_SZ_G8];

#define MEDIA_VP8_MBENC_FRM_P_SZ_G8 0x6e60
extern const UINT MEDIA_VP8_MBENC_FRM_P_G8[MEDIA_VP8_MBENC_FRM_P_SZ_G8];

#define MEDIA_VP8_PAK_PHASE1_SZ_G8 0x7760
extern const UINT MEDIA_VP8_PAK_PHASE1_G8[MEDIA_VP8_PAK_PHASE1_SZ_G8];

#define MEDIA_VP8_PAK_PHASE2_SZ_G8 0xa380
extern const UINT MEDIA_VP8_PAK_PHASE2_G8[MEDIA_VP8_PAK_PHASE2_SZ_G8];
#endif
