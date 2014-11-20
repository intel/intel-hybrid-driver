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
 *     Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 *
 */

#include <math.h>
#include <va/va.h>
#include <va/va_enc_vp8.h>
#include "media_drv_hw_g7.h"
#include "media_drv_kernels_g7.h"
#include "media_drv_surface.h"
//#define DEBUG
struct hw_codec_info gen7_hw_codec_info = {
  .max_width = 4096,
  .max_height = 4096,
  .vp8_enc_hybrid_support = 1,
  .tiled_surface = 1,
  .vp8_enc_hybrid_support=1,
  .ratecontrol= VA_RC_CQP | VA_RC_CBR | VA_RC_VBR,
};

MEDIA_KERNEL media_hybrid_vp8_kernels_g7[] = {
  {
   (BYTE *) "VP8_MBENC_I",
   0,
   MEDIA_VP8_MBENC_I_G7,
   MEDIA_VP8_MBENC_I_SZ_G7,
   NULL,
   0},
  {
   (BYTE *) "VP8_MBENC_ICHROMA",
   0,
   MEDIA_VP8_MBENC_ICHROMA_G7,
   MEDIA_VP8_MBENC_ICHROMA_SZ_G7,
   NULL,
   0},
  {
   (BYTE *) "VP8_MBENC_FRM_P",
   0,
   MEDIA_VP8_MBENC_FRM_P_G7,
   MEDIA_VP8_MBENC_FRM_P_SZ_G7,
   NULL,
   0},
  {
   (BYTE *) "VP8_MBENC_ILuma",
   0,
   MEDIA_VP8_MBENC_ILuma_G7,
   MEDIA_VP8_MBENC_ILuma_SZ_G7,
   NULL,
   0},
  {
   (BYTE *) "VP8_INTRA_DIS_BRC",
   0,
   MEDIA_VP8_INTRA_DIS_BRC_G7,
   MEDIA_VP8_INTRA_DIS_BRC_SZ_G7,
   NULL,
   0},
  {
   (BYTE *) "VP8_HME_P",
   0,
   MEDIA_VP8_HME_P_G7,
   MEDIA_VP8_HME_P_SZ_G7,
   NULL,
   0},
  {
   (BYTE *) "VP8_HME_DOWNSCALE",
   0,
   MEDIA_VP8_HME_DOWNSCALE_G7,
   MEDIA_VP8_HME_DOWNSCALE_SZ_G7,
   NULL,
   0},

  {
   (BYTE *) "VP8_PAK_PHASE2",
   0,
   MEDIA_VP8_PAK_PHASE2_G7,
   MEDIA_VP8_PAK_PHASE2_SZ_G7,
   NULL,
   0},
  {
   (BYTE *) "VP8_PAK_PHASE1",
   0,
   MEDIA_VP8_PAK_PHASE1_G7,
   MEDIA_VP8_PAK_PHASE1_SZ_G7,
   NULL,
   0},
  {
   (BYTE *) "VP8_BRC_INIT",
   0,
   MEDIA_VP8_BRC_INIT_G7,
   MEDIA_VP8_BRC_INIT_SZ_G7,
   NULL,
   0},

  {
   (BYTE *) "VP8_BRC_RESET",
   0,
   MEDIA_VP8_BRC_RESET_G7,
   MEDIA_VP8_BRC_RESET_SZ_G7,
   NULL,
   0},

  {
   (BYTE *) "VP8_BRC_UPDATE",
   0,
   MEDIA_VP8_BRC_UPDATE_G7,
   MEDIA_VP8_BRC_UPDATE_SZ_G7,
   NULL,
   0}
};

const UINT cost_table_vp8_g7[128][7] = {
  {0x298f0200, 0x6f6f6f6f, 0x0000006f, 0x03020201, 0x0a060403, 0x04, 0x07},
  {0x2b8f0300, 0x6f6f6f6f, 0x0000006f, 0x03020201, 0x0a060403, 0x05, 0x08},
  {0x2e8f0300, 0x6f6f6f6f, 0x0000006f, 0x03020201, 0x0a060403, 0x06, 0x0a},
  {0x388f0400, 0x6f6f6f6f, 0x0000006f, 0x03020201, 0x0a060403, 0x07, 0x0c},
  {0x398f0500, 0x6f6f6f6f, 0x0000006f, 0x06040402, 0x190b0907, 0x08, 0x0d},
  {0x3a8f0500, 0x6f6f6f6f, 0x0000006f, 0x06040402, 0x190b0907, 0x09, 0x0f},
  {0x3b8f0600, 0x6f6f6f6f, 0x0000006f, 0x06040402, 0x190b0907, 0x0a, 0x11},
  {0x3b8f0600, 0x6f6f6f6f, 0x0000006f, 0x06040402, 0x190b0907, 0x0a, 0x11},
  {0x3d8f0600, 0x6f6f6f6f, 0x0000006f, 0x06040402, 0x190b0907, 0x0b, 0x12},
  {0x3e8f0700, 0x6f6f6f6f, 0x0000006f, 0x09050603, 0x1e180e0a, 0x0c, 0x14},
  {0x3f8f0700, 0x6f6f6f6f, 0x0000006f, 0x09050603, 0x1e180e0a, 0x0d, 0x16},
  {0x488f0800, 0x6f6f6f6f, 0x0000006f, 0x09050603, 0x1e180e0a, 0x0e, 0x17},
  {0x488f0900, 0x6f6f6f6f, 0x0000006f, 0x09050603, 0x1e180e0a, 0x0f, 0x19},
  {0x498f0900, 0x6f6f6f6f, 0x0000006f, 0x0c070705, 0x291b190e, 0x10, 0x1b},
  {0x4a8f0a00, 0x6f6f6f6f, 0x0000006f, 0x0c070705, 0x291b190e, 0x11, 0x1d},
  {0x4a8f0a00, 0x6f6f6f6f, 0x0000006f, 0x0c070705, 0x291b190e, 0x11, 0x1d},
  {0x4a8f0a00, 0x6f6f6f6f, 0x0000006f, 0x0c070705, 0x291b190e, 0x12, 0x1e},
  {0x4b8f0b00, 0x6f6f6f6f, 0x0000006f, 0x0c070705, 0x291b190e, 0x13, 0x20},
  {0x4b8f0c00, 0x6f6f6f6f, 0x0000006f, 0x18090906, 0x2c1e1b19, 0x15, 0x22},
  {0x4b8f0c00, 0x6f6f6f6f, 0x0000006f, 0x18090906, 0x2c1e1b19, 0x15, 0x22},
  {0x4c8f0c00, 0x6f6f6f6f, 0x0000006f, 0x18090906, 0x2c1e1b19, 0x16, 0x23},
  {0x4c8f0c00, 0x6f6f6f6f, 0x0000006f, 0x18090906, 0x2c1e1b19, 0x16, 0x23},
  {0x4d8f0d00, 0x6f6f6f6f, 0x0000006f, 0x18090906, 0x2c1e1b19, 0x17, 0x25},
  {0x4d8f0d00, 0x6f6f6f6f, 0x0000006f, 0x18090906, 0x2c1e1b19, 0x17, 0x25},
  {0x4d8f0d00, 0x6f6f6f6f, 0x0000006f, 0x18090906, 0x2c1e1b19, 0x18, 0x27},
  {0x4d8f0d00, 0x6f6f6f6f, 0x0000006f, 0x18090906, 0x2c1e1b19, 0x18, 0x27},
  {0x4e8f0e00, 0x6f6f6f6f, 0x0000006f, 0x190b0b07, 0x2e281e1a, 0x19, 0x28},
  {0x4e8f0f00, 0x6f6f6f6f, 0x0000006f, 0x190b0b07, 0x2e281e1a, 0x1a, 0x2a},
  {0x4e8f0f00, 0x6f6f6f6f, 0x0000006f, 0x190b0b07, 0x2e281e1a, 0x1a, 0x2a},
  {0x4f8f0f00, 0x6f6f6f6f, 0x0000006f, 0x190b0b07, 0x2e281e1a, 0x1b, 0x2c},
  {0x588f1800, 0x6f6f6f6f, 0x0000006f, 0x190b0b07, 0x2e281e1a, 0x1c, 0x2d},
  {0x588f1800, 0x6f6f6f6f, 0x0000006f, 0x1b0d0d08, 0x382a281c, 0x1d, 0x2f},
  {0x588f1800, 0x6f6f6f6f, 0x0000006f, 0x1b0d0d08, 0x382a281c, 0x1e, 0x31},
  {0x588f1900, 0x6f6f6f6f, 0x0000006f, 0x1b0d0d08, 0x382a281c, 0x1f, 0x32},
  {0x598f1900, 0x6f6f6f6f, 0x0000006f, 0x1b0d0d08, 0x382a281c, 0x20, 0x34},
  {0x598f1900, 0x6f6f6f6f, 0x0000006f, 0x1c0f0f09, 0x392b291e, 0x21, 0x36},
  {0x598f1900, 0x6f6f6f6f, 0x0000006f, 0x1c0f0f09, 0x392b291e, 0x22, 0x38},
  {0x5a8f1a00, 0x6f6f6f6f, 0x0000006f, 0x1c0f0f09, 0x392b291e, 0x23, 0x39},
  {0x5a8f1a00, 0x6f6f6f6f, 0x0000006f, 0x1c0f0f09, 0x392b291e, 0x24, 0x3b},
  {0x5a8f1a00, 0x6f6f6f6f, 0x0000006f, 0x1e18180b, 0x3b2d2a28, 0x25, 0x3d},
  {0x5b8f1b00, 0x6f6f6f6f, 0x0000006f, 0x1e18180b, 0x3b2d2a28, 0x26, 0x3e},
  {0x5b8f1b00, 0x6f6f6f6f, 0x0000006f, 0x1e18180b, 0x3b2d2a28, 0x26, 0x3e},
  {0x5b8f1b00, 0x6f6f6f6f, 0x0000006f, 0x1e18180b, 0x3b2d2a28, 0x27, 0x40},
  {0x5b8f1b00, 0x6f6f6f6f, 0x0000006f, 0x1e18180b, 0x3b2d2a28, 0x28, 0x42},
  {0x5b8f1c00, 0x6f6f6f6f, 0x0000006f, 0x1f19190c, 0x3c2e2b28, 0x29, 0x43},
  {0x5c8f1c00, 0x6f6f6f6f, 0x0000006f, 0x1f19190c, 0x3c2e2b28, 0x2a, 0x45},
  {0x5c8f1c00, 0x6f6f6f6f, 0x0000006f, 0x1f19190c, 0x3c2e2b28, 0x2b, 0x47},
  {0x5c8f1c00, 0x6f6f6f6f, 0x0000006f, 0x1f19190c, 0x3c2e2b28, 0x2c, 0x48},
  {0x5d8f1d00, 0x6f6f6f6f, 0x0000006f, 0x281a1a0d, 0x3d382c29, 0x2d, 0x4a},
  {0x5d8f1d00, 0x6f6f6f6f, 0x0000006f, 0x281a1a0d, 0x3d382c29, 0x2f, 0x4c},
  {0x5d8f1d00, 0x6f6f6f6f, 0x0000006f, 0x281a1a0d, 0x3d382c29, 0x30, 0x4e},
  {0x5d8f1d00, 0x6f6f6f6f, 0x0000006f, 0x281a1a0d, 0x3d382c29, 0x30, 0x4e},
  {0x5d8f1e00, 0x6f6f6f6f, 0x0000006f, 0x281a1a0d, 0x3d382c29, 0x31, 0x4f},
  {0x5e8f1e00, 0x6f6f6f6f, 0x0000006f, 0x291b1b0e, 0x3e382e2a, 0x32, 0x51},
  {0x5e8f1e00, 0x6f6f6f6f, 0x0000006f, 0x291b1b0e, 0x3e382e2a, 0x33, 0x53},
  {0x5e8f1e00, 0x6f6f6f6f, 0x0000006f, 0x291b1b0e, 0x3e382e2a, 0x34, 0x54},
  {0x5f8f1f00, 0x6f6f6f6f, 0x0000006f, 0x291b1b0e, 0x3e382e2a, 0x35, 0x56},
  {0x5f8f1f00, 0x6f6f6f6f, 0x0000006f, 0x2a1c1c0f, 0x3f392f2b, 0x36, 0x58},
  {0x5f8f1f00, 0x6f6f6f6f, 0x0000006f, 0x2a1c1c0f, 0x3f392f2b, 0x37, 0x59},
  {0x688f2800, 0x6f6f6f6f, 0x0000006f, 0x2a1c1c0f, 0x3f392f2b, 0x38, 0x5b},
  {0x688f2800, 0x6f6f6f6f, 0x0000006f, 0x2a1c1c0f, 0x3f392f2b, 0x39, 0x5d},
  {0x688f2800, 0x6f6f6f6f, 0x0000006f, 0x2b1d1d18, 0x483a382c, 0x3a, 0x5e},
  {0x688f2800, 0x6f6f6f6f, 0x0000006f, 0x2b1d1d18, 0x483a382c, 0x3b, 0x60},
  {0x688f2800, 0x6f6f6f6f, 0x0000006f, 0x2b1d1d18, 0x483a382c, 0x3c, 0x62},
  {0x688f2800, 0x6f6f6f6f, 0x0000006f, 0x2b1d1d18, 0x483a382c, 0x3d, 0x64},
  {0x688f2800, 0x6f6f6f6f, 0x0000006f, 0x2b1e1e19, 0x493b382d, 0x3e, 0x65},
  {0x698f2900, 0x6f6f6f6f, 0x0000006f, 0x2b1e1e19, 0x493b382d, 0x3f, 0x67},
  {0x698f2900, 0x6f6f6f6f, 0x0000006f, 0x2b1e1e19, 0x493b382d, 0x40, 0x69},
  {0x698f2900, 0x6f6f6f6f, 0x0000006f, 0x2b1e1e19, 0x493b382d, 0x41, 0x6a},
  {0x698f2900, 0x6f6f6f6f, 0x0000006f, 0x2c1f1f19, 0x493b392e, 0x42, 0x6c},
  {0x698f2900, 0x6f6f6f6f, 0x0000006f, 0x2c1f1f19, 0x493b392e, 0x43, 0x6e},
  {0x698f2900, 0x6f6f6f6f, 0x0000006f, 0x2c1f1f19, 0x493b392e, 0x44, 0x6f},
  {0x698f2a00, 0x6f6f6f6f, 0x0000006f, 0x2c1f1f19, 0x493b392e, 0x45, 0x71},
  {0x6a8f2a00, 0x6f6f6f6f, 0x0000006f, 0x2d28281a, 0x4a3c392f, 0x46, 0x73},
  {0x6a8f2a00, 0x6f6f6f6f, 0x0000006f, 0x2d28281a, 0x4a3c392f, 0x47, 0x74},
  {0x6a8f2a00, 0x6f6f6f6f, 0x0000006f, 0x2d28281a, 0x4a3c392f, 0x48, 0x76},
  {0x6a8f2a00, 0x6f6f6f6f, 0x0000006f, 0x2d28281a, 0x4a3c392f, 0x4a, 0x78},
  {0x6a8f2a00, 0x6f6f6f6f, 0x0000006f, 0x2e28281b, 0x4b3d3a38, 0x4b, 0x7a},
  {0x6a8f2a00, 0x6f6f6f6f, 0x0000006f, 0x2e28281b, 0x4b3d3a38, 0x4c, 0x7b},
  {0x6b8f2b00, 0x6f6f6f6f, 0x0000006f, 0x2e28281b, 0x4b3d3a38, 0x4d, 0x7d},
  {0x6b8f2b00, 0x6f6f6f6f, 0x0000006f, 0x2e28281b, 0x4b3d3a38, 0x4e, 0x7f},
  {0x6b8f2b00, 0x6f6f6f6f, 0x0000006f, 0x2f28291b, 0x4b3d3b38, 0x4f, 0x80},
  {0x6b8f2b00, 0x6f6f6f6f, 0x0000006f, 0x2f28291b, 0x4b3d3b38, 0x4f, 0x80},
  {0x6b8f2b00, 0x6f6f6f6f, 0x0000006f, 0x2f28291b, 0x4b3d3b38, 0x50, 0x82},
  {0x6b8f2b00, 0x6f6f6f6f, 0x0000006f, 0x2f28291b, 0x4b3d3b38, 0x51, 0x84},
  {0x6b8f2b00, 0x6f6f6f6f, 0x0000006f, 0x2f28291b, 0x4b3d3b38, 0x52, 0x85},
  {0x6b8f2b00, 0x6f6f6f6f, 0x0000006f, 0x2f29291c, 0x4c3e3b38, 0x53, 0x87},
  {0x6c8f2c00, 0x6f6f6f6f, 0x0000006f, 0x2f29291c, 0x4c3e3b38, 0x54, 0x89},
  {0x6c8f2c00, 0x6f6f6f6f, 0x0000006f, 0x2f29291c, 0x4c3e3b38, 0x55, 0x8a},
  {0x6c8f2c00, 0x6f6f6f6f, 0x0000006f, 0x2f29291c, 0x4c3e3b38, 0x56, 0x8c},
  {0x6c8f2c00, 0x6f6f6f6f, 0x0000006f, 0x38292a1c, 0x4c3f3c39, 0x57, 0x8e},
  {0x6c8f2c00, 0x6f6f6f6f, 0x0000006f, 0x38292a1c, 0x4c3f3c39, 0x58, 0x90},
  {0x6c8f2c00, 0x6f6f6f6f, 0x0000006f, 0x38292a1c, 0x4c3f3c39, 0x59, 0x91},
  {0x6c8f2c00, 0x6f6f6f6f, 0x0000006f, 0x38292a1c, 0x4c3f3c39, 0x5a, 0x93},
  {0x6d8f2d00, 0x6f6f6f6f, 0x0000006f, 0x382a2a1d, 0x4d483c39, 0x5b, 0x95},
  {0x6d8f2d00, 0x6f6f6f6f, 0x0000006f, 0x382a2a1d, 0x4d483c39, 0x5c, 0x96},
  {0x6d8f2d00, 0x6f6f6f6f, 0x0000006f, 0x382a2a1d, 0x4d483c39, 0x5e, 0x9a},
  {0x6d8f2d00, 0x6f6f6f6f, 0x0000006f, 0x392a2b1e, 0x4e483d3a, 0x60, 0x9d},
  {0x6e8f2e00, 0x6f6f6f6f, 0x0000006f, 0x392a2b1e, 0x4e483d3a, 0x62, 0xa0},
  {0x6e8f2e00, 0x6f6f6f6f, 0x0000006f, 0x392b2b1e, 0x4e483e3a, 0x64, 0xa2},
  {0x6e8f2e00, 0x6f6f6f6f, 0x0000006f, 0x392b2b1e, 0x4e483e3a, 0x66, 0xa6},
  {0x6e8f2e00, 0x6f6f6f6f, 0x0000006f, 0x392b2c1f, 0x4f493e3b, 0x68, 0xa9},
  {0x6f8f2f00, 0x6f6f6f6f, 0x0000006f, 0x392b2c1f, 0x4f493e3b, 0x69, 0xab},
  {0x6f8f2f00, 0x6f6f6f6f, 0x0000006f, 0x392b2c1f, 0x4f493e3b, 0x6a, 0xac},
  {0x6f8f2f00, 0x6f6f6f6f, 0x0000006f, 0x3a2c2c1f, 0x4f493f3b, 0x6c, 0xb0},
  {0x6f8f2f00, 0x6f6f6f6f, 0x0000006f, 0x3a2c2c1f, 0x4f493f3b, 0x6e, 0xb3},
  {0x788f3800, 0x6f6f6f6f, 0x0000006f, 0x3a2c2d28, 0x58493f3c, 0x70, 0xb6},
  {0x788f3800, 0x6f6f6f6f, 0x0000006f, 0x3a2c2d28, 0x58493f3c, 0x72, 0xba},
  {0x788f3800, 0x6f6f6f6f, 0x0000006f, 0x3b2d2d28, 0x584a483c, 0x74, 0xbd},
  {0x788f3800, 0x6f6f6f6f, 0x0000006f, 0x3b2d2d28, 0x584a483c, 0x76, 0xc1},
  {0x788f3800, 0x6f6f6f6f, 0x0000006f, 0x3b2d2e28, 0x584a483c, 0x78, 0xc4},
  {0x788f3800, 0x6f6f6f6f, 0x0000006f, 0x3b2d2e28, 0x584a483c, 0x7a, 0xc7},
  {0x798f3900, 0x6f6f6f6f, 0x0000006f, 0x3b2e2e29, 0x594b483d, 0x7f, 0xce},
  {0x798f3900, 0x6f6f6f6f, 0x0000006f, 0x3c2e2e29, 0x594b493d, 0x81, 0xd1},
  {0x798f3900, 0x6f6f6f6f, 0x0000006f, 0x3c2e2e29, 0x594b493d, 0x83, 0xd5},
  {0x798f3900, 0x6f6f6f6f, 0x0000006f, 0x3c2f2f29, 0x594b493e, 0x85, 0xd8},
  {0x798f3900, 0x6f6f6f6f, 0x0000006f, 0x3c2f2f29, 0x594b493e, 0x87, 0xdc},
  {0x798f3900, 0x6f6f6f6f, 0x0000006f, 0x3d2f2f2a, 0x5a4c493e, 0x89, 0xdf},
  {0x798f3900, 0x6f6f6f6f, 0x0000006f, 0x3d2f2f2a, 0x5a4c493e, 0x8b, 0xe2},
  {0x7a8f3a00, 0x6f6f6f6f, 0x0000006f, 0x3d38382a, 0x5a4c493f, 0x8d, 0xe6},
  {0x7a8f3a00, 0x6f6f6f6f, 0x0000006f, 0x3d38382a, 0x5a4c493f, 0x8f, 0xe9},
  {0x7a8f3a00, 0x6f6f6f6f, 0x0000006f, 0x3d38382a, 0x5a4c4a3f, 0x91, 0xed},
  {0x7a8f3a00, 0x6f6f6f6f, 0x0000006f, 0x3d38382a, 0x5a4c4a3f, 0x94, 0xf2},
  {0x7a8f3a00, 0x6f6f6f6f, 0x0000006f, 0x3e38382a, 0x5a4d4a48, 0x96, 0xf5},
  {0x7b8f3b00, 0x6f6f6f6f, 0x0000006f, 0x3e38382b, 0x5b4d4a48, 0x9a, 0xfa},
  {0x7b8f3b00, 0x6f6f6f6f, 0x0000006f, 0x3e38382b, 0x5b4d4a48, 0x9d, 0xff},
  {0x7b8f3b00, 0x6f6f6f6f, 0x0000006f, 0x3f38392b, 0x5b4d4b48, 0xa0, 0xff},
  {0x7b8f3b00, 0x6f6f6f6f, 0x0000006f, 0x3f39392b, 0x5b4e4b48, 0xa3, 0xff}
};

const UINT vme_lut_sp_state_vp8_g7[8][32] = {

  {
   0x120FF10F, 0x1E22E20D, 0x20E2FF10, 0x2EDD06FC, 0x11D33FF1, 0xEB1FF33D,
   0x4EF1F1F1, 0xF1F21211,
   0x0DFFFFE0, 0x11201F1F, 0x1105F1CF, 0x00000000, 0x00000000, 0x00000000,
   0x00010001, 0x00020001,
   0x298f0200, 0x076f0204, 0x03020201, 0x0a060403, 0x398f0500, 0x0e6f0409,
   0x06040402, 0x190b0907,
   0x3d8f0600, 0x0e6f0409, 0x06040402, 0x190b0907, 0x488f0900, 0x1a6f060e,
   0x09050603, 0x1e180e0a},

  {
   0x120FF10F, 0x1E22E20D, 0x20E2FF10, 0x2EDD06FC, 0x11D33FF1, 0xEB1FF33D,
   0x4EF1F1F1, 0xF1F21211,
   0x0DFFFFE0, 0x11201F1F, 0x1105F1CF, 0x00000000, 0x00000000, 0x00000000,
   0x00040003, 0x00050004,
   0x4a8f0a00, 0x1e6f0819, 0x0c070705, 0x291b190e, 0x4c8f0c00, 0x286f091b,
   0x18090906, 0x2c1e1b19,
   0x4d8f0d00, 0x286f091b, 0x18090906, 0x2c1e1b19, 0x4e8f0f00, 0x2a6f0b1d,
   0x190b0b07, 0x2e281e1a},

  {
   0x120FF10F, 0x1E22E20D, 0x20E2FF10, 0x2EDD06FC, 0x11D33FF1, 0xEB1FF33D,
   0x4EF1F1F1, 0xF1F21211,
   0x0DFFFFE0, 0x11201F1F, 0x1105F1CF, 0x00000000, 0x00000000, 0x00000000,
   0x00060006, 0x00080007,
   0x588f1800, 0x2c6f0d28, 0x1b0d0d08, 0x382a281c, 0x598f1900, 0x2e6f0f29,
   0x1c0f0f09, 0x392b291e,
   0x5b8f1b00, 0x386f182a, 0x1e18180b, 0x3b2d2a28, 0x5b8f1c00, 0x386f192b,
   0x1f19190c, 0x3c2e2b28},

  {
   0x120FF10F, 0x1E22E20D, 0x20E2FF10, 0x2EDD06FC, 0x11D33FF1, 0xEB1FF33D,
   0x4EF1F1F1, 0xF1F21211,
   0x0DFFFFE0, 0x11201F1F, 0x1105F1CF, 0x00000000, 0x00000000, 0x00000000,
   0x00090009, 0x000b000a,
   0x5d8f1d00, 0x396f1a2c, 0x281a1a0d, 0x3d382c29, 0x5d8f1e00, 0x396f1a2c,
   0x281a1a0d, 0x3d382c29,
   0x5f8f1f00, 0x3a6f1b2d, 0x291b1b0e, 0x3e382e2a, 0x688f2800, 0x3b6f1c2f,
   0x2a1c1c0f, 0x3f392f2b},

  {
   0x120FF10F, 0x1E22E20D, 0x20E2FF10, 0x2EDD06FC, 0x11D33FF1, 0xEB1FF33D,
   0x4EF1F1F1, 0xF1F21211,
   0x0DFFFFE0, 0x11201F1F, 0x1105F1CF, 0x00000000, 0x00000000, 0x00000000,
   0x000c000b, 0x000e000d,
   0x688f2800, 0x3c6f1d38, 0x2b1d1d18, 0x483a382c, 0x698f2900, 0x3d6f1e38,
   0x2b1e1e19, 0x493b382d,
   0x698f2a00, 0x3e6f1f39, 0x2c1f1f19, 0x493b392e, 0x6a8f2a00, 0x3f6f2839,
   0x2d28281a, 0x4a3c392f},

  {
   0x120FF10F, 0x1E22E20D, 0x20E2FF10, 0x2EDD06FC, 0x11D33FF1, 0xEB1FF33D,
   0x4EF1F1F1, 0xF1F21211,
   0x0DFFFFE0, 0x11201F1F, 0x1105F1CF, 0x00000000, 0x00000000, 0x00000000,
   0x000f000f, 0x00180018,
   0x6b8f2b00, 0x486f283a, 0x2e28281b, 0x4b3d3a38, 0x6b8f2b00, 0x486f293b,
   0x2f28291b, 0x4b3d3b38,
   0x6c8f2c00, 0x486f293b, 0x2f29291c, 0x4c3e3b38, 0x6c8f2c00, 0x496f2a3c,
   0x38292a1c, 0x4c3f3c39},

  {
   0x120FF10F, 0x1E22E20D, 0x20E2FF10, 0x2EDD06FC, 0x11D33FF1, 0xEB1FF33D,
   0x4EF1F1F1, 0xF1F21211,
   0x0DFFFFE0, 0x11201F1F, 0x1105F1CF, 0x00000000, 0x00000000, 0x00000000,
   0x001a0019, 0x001b001a,
   0x6d8f2d00, 0x496f2a3c, 0x382a2a1d, 0x4d483c39, 0x6e8f2e00, 0x4a6f2b3d,
   0x392b2b1e, 0x4e483e3a,
   0x6f8f2f00, 0x4b6f2c3f, 0x3a2c2c1f, 0x4f493f3b, 0x788f3800, 0x4c6f2d48,
   0x3b2d2d28, 0x584a483c},

  {
   0x120FF10F, 0x1E22E20D, 0x20E2FF10, 0x2EDD06FC, 0x11D33FF1, 0xEB1FF33D,
   0x4EF1F1F1, 0xF1F21211,
   0x0DFFFFE0, 0x11201F1F, 0x1105F1CF, 0x00000000, 0x00000000, 0x00000000,
   0x001d001c, 0x001f001e,
   0x798f3900, 0x4d6f2e48, 0x3b2e2e29, 0x594b483d, 0x798f3900, 0x4e6f2f49,
   0x3c2f2f29, 0x594b493e,
   0x7a8f3a00, 0x4f6f3849, 0x3d38382a, 0x5a4c493f, 0x7b8f3b00, 0x586f394a,
   0x3e38382b, 0x5b4d4a48}
};

static const BYTE brc_pframe_cost_table_vp8_g7[256] =
{
  0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
  0x10,0x11,0x11,0x12,0x13,0x15,0x15,0x16,0x16,0x17,0x17,0x18,0x18,
  0x19,0x1a,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,
  0x25,0x26,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2f,0x30,0x30,
  0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,
  0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x4a,0x4b,
  0x4c,0x4d,0x4e,0x4f,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
  0x58,0x59,0x5a,0x5b,0x5c,0x5e,0x60,0x62,0x64,0x66,0x68,0x69,0x6a,
  0x6c,0x6e,0x70,0x72,0x74,0x76,0x78,0x7a,0x7f,0x81,0x83,0x85,0x87,
  0x89,0x8b,0x8d,0x8f,0x91,0x94,0x96,0x9a,0x9d,0xa0,0xa3,0x07,0x08,
  0x0a,0x0c,0x0d,0x0f,0x11,0x11,0x12,0x14,0x16,0x17,0x19,0x1b,0x1d,
  0x1d,0x1e,0x20,0x22,0x22,0x23,0x23,0x25,0x25,0x27,0x27,0x28,0x2a,
  0x2a,0x2c,0x2d,0x2f,0x31,0x32,0x34,0x36,0x38,0x39,0x3b,0x3d,0x3e,
  0x3e,0x40,0x42,0x43,0x45,0x47,0x48,0x4a,0x4c,0x4e,0x4e,0x4f,0x51,
  0x53,0x54,0x56,0x58,0x59,0x5b,0x5d,0x5e,0x60,0x62,0x64,0x65,0x67,
  0x69,0x6a,0x6c,0x6e,0x6f,0x71,0x73,0x74,0x76,0x78,0x7a,0x7b,0x7d,
  0x7f,0x80,0x80,0x82,0x84,0x85,0x87,0x89,0x8a,0x8c,0x8e,0x90,0x91,
  0x93,0x95,0x96,0x9a,0x9d,0xa0,0xa2,0xa6,0xa9,0xab,0xac,0xb0,0xb3,
  0xb6,0xba,0xbd,0xc1,0xc4,0xc7,0xce,0xd1,0xd5,0xd8,0xdc,0xdf,0xe2,
  0xe6,0xe9,0xed,0xf2,0xf5,0xfa,0xff,0xff,0xff
};

const BYTE brc_qpadjustment_distthreshold_maxframethreshold_distqpadjustment_IPB_vp8_g7[576] =
{
  0x01, 0x03, 0x05, 0x07, 0x09, 0x01, 0x02, 0x03, 0x05, 0x07, 0x00, 0x00, 0x01, 0x02, 0x04, 0x00,
  0x00, 0x00, 0x01, 0x02, 0xff, 0x00, 0x00, 0x00, 0x01, 0xfd, 0xfe, 0xff, 0x00, 0x00, 0xfb, 0xfc,
  0xfe, 0xff, 0x00, 0xf9, 0xfa, 0xfc, 0xfe, 0xff, 0xf7, 0xf9, 0xfb, 0xfe, 0xff, 0x00, 0x04, 0x1e,
  0x3c, 0x50, 0x78, 0x8c, 0xc8, 0xff, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x02, 0x05, 0x08, 0x0a, 0x01, 0x02, 0x04, 0x06, 0x08, 0x00, 0x01, 0x02, 0x04, 0x06, 0x00,
  0x00, 0x00, 0x01, 0x02, 0xff, 0x00, 0x00, 0x00, 0x01, 0xfe, 0xff, 0xff, 0x00, 0x00, 0xfd, 0xfe,
  0xff, 0xff, 0x00, 0xfb, 0xfd, 0xfe, 0xff, 0x00, 0xf9, 0xfa, 0xfc, 0xfe, 0xff, 0x00, 0x04, 0x1e,
  0x3c, 0x50, 0x78, 0x8c, 0xc8, 0xff, 0x04, 0x05, 0x06, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x02, 0x05, 0x08, 0x0a, 0x01, 0x02, 0x04, 0x06, 0x08, 0x00, 0x01, 0x02, 0x04, 0x06, 0x00,
  0x00, 0x00, 0x01, 0x02, 0xff, 0x00, 0x00, 0x00, 0x01, 0xfe, 0xff, 0xff, 0x00, 0x00, 0xfd, 0xfe,
  0xff, 0xff, 0x00, 0xfb, 0xfd, 0xfe, 0xff, 0x00, 0xf9, 0xfa, 0xfc, 0xfe, 0xff, 0x00, 0x02, 0x14,
  0x28, 0x46, 0x82, 0xa0, 0xc8, 0xff, 0x04, 0x05, 0x06, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x06, 0x08, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x05,
  0x07, 0x09, 0xff, 0x00, 0x00, 0x00, 0x00, 0x03, 0x04, 0x06, 0x07, 0xfe, 0xff, 0x00, 0x00, 0x00,
  0x01, 0x02, 0x03, 0x05, 0xfd, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x01, 0x03, 0x05, 0xfc, 0xfe, 0xff,
  0x00, 0x00, 0x00, 0x01, 0x03, 0x05, 0xfb, 0xfd, 0xfe, 0xff, 0x00, 0x00, 0x01, 0x03, 0x05, 0xfa,
  0xfc, 0xfe, 0xff, 0x00, 0x00, 0x01, 0x03, 0x05, 0xfa, 0xfc, 0xfe, 0xff, 0x00, 0x00, 0x01, 0x03,
  0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x05, 0x07, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x05,
  0x06, 0x08, 0xff, 0x00, 0x00, 0x00, 0x00, 0x03, 0x05, 0x07, 0x08, 0xfe, 0xff, 0x00, 0x00, 0x00,
  0x02, 0x04, 0x05, 0x06, 0xfd, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x01, 0x04, 0x05, 0xfc, 0xfe, 0xff,
  0x00, 0x00, 0x00, 0x01, 0x04, 0x05, 0xfc, 0xfe, 0xff, 0xff, 0x00, 0x00, 0x00, 0x04, 0x05, 0xfc,
  0xfd, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x01, 0x05, 0xfb, 0xfc, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x01,
  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x05, 0x07, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x05,
  0x06, 0x08, 0xff, 0x00, 0x00, 0x00, 0x00, 0x03, 0x05, 0x07, 0x08, 0xfe, 0xff, 0x00, 0x00, 0x00,
  0x02, 0x04, 0x05, 0x06, 0xfd, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x01, 0x04, 0x05, 0xfc, 0xfe, 0xff,
  0x00, 0x00, 0x00, 0x01, 0x04, 0x05, 0xfc, 0xfe, 0xff, 0xff, 0x00, 0x00, 0x00, 0x04, 0x05, 0xfc,
  0xfd, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x01, 0x05, 0xfb, 0xfc, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x01,
  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const BYTE brc_mv_cost_table_vp8_g7[256] =
{
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x11,
  0x12, 0x13, 0x15, 0x15, 0x16, 0x16, 0x17, 0x17, 0x18, 0x18, 0x19, 0x1a, 0x1a, 0x1b, 0x1c, 0x1d,
  0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c,
  0x2d, 0x2f, 0x30, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c,
  0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x4a, 0x4b, 0x4c, 0x4d,
  0x4e, 0x4f, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c,
  0x5e, 0x60, 0x62, 0x64, 0x66, 0x68, 0x69, 0x6a, 0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7a,
  0x7f, 0x81, 0x83, 0x85, 0x87, 0x89, 0x8b, 0x8d, 0x8f, 0x91, 0x94, 0x96, 0x9a, 0x9d, 0xa0, 0xa3,
  0x07, 0x08, 0x0a, 0x0c, 0x0d, 0x0f, 0x11, 0x11, 0x12, 0x14, 0x16, 0x17, 0x19, 0x1b, 0x1d, 0x1d,
  0x1e, 0x20, 0x22, 0x22, 0x23, 0x23, 0x25, 0x25, 0x27, 0x27, 0x28, 0x2a, 0x2a, 0x2c, 0x2d, 0x2f,
  0x31, 0x32, 0x34, 0x36, 0x38, 0x39, 0x3b, 0x3d, 0x3e, 0x3e, 0x40, 0x42, 0x43, 0x45, 0x47, 0x48,
  0x4a, 0x4c, 0x4e, 0x4e, 0x4f, 0x51, 0x53, 0x54, 0x56, 0x58, 0x59, 0x5b, 0x5d, 0x5e, 0x60, 0x62,
  0x64, 0x65, 0x67, 0x69, 0x6a, 0x6c, 0x6e, 0x6f, 0x71, 0x73, 0x74, 0x76, 0x78, 0x7a, 0x7b, 0x7d,
  0x7f, 0x80, 0x80, 0x82, 0x84, 0x85, 0x87, 0x89, 0x8a, 0x8c, 0x8e, 0x90, 0x91, 0x93, 0x95, 0x96,
  0x9a, 0x9d, 0xa0, 0xa2, 0xa6, 0xa9, 0xab, 0xac, 0xb0, 0xb3, 0xb6, 0xba, 0xbd, 0xc1, 0xc4, 0xc7,
  0xce, 0xd1, 0xd5, 0xd8, 0xdc, 0xdf, 0xe2, 0xe6, 0xe9, 0xed, 0xf2, 0xf5, 0xfa, 0xff, 0xff, 0xff,
};

const UINT16 brc_quant_dc_table_vp8_g7[128] =
{
  0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000a,
  0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 0x0010, 0x0011, 0x0011,
  0x0012, 0x0013, 0x0014, 0x0014, 0x0015, 0x0015, 0x0016, 0x0016,
  0x0017, 0x0017, 0x0018, 0x0019, 0x0019, 0x001a, 0x001b, 0x001c,
  0x001d, 0x001e, 0x001f, 0x0020, 0x0021, 0x0022, 0x0023, 0x0024,
  0x0025, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b,
  0x002c, 0x002d, 0x002e, 0x002e, 0x002f, 0x0030, 0x0031, 0x0032,
  0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a,
  0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 0x0040, 0x0041, 0x0042,
  0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a,
  0x004b, 0x004c, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050, 0x0051,
  0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059,
  0x005b, 0x005d, 0x005f, 0x0060, 0x0062, 0x0064, 0x0065, 0x0066,
  0x0068, 0x006a, 0x006c, 0x006e, 0x0070, 0x0072, 0x0074, 0x0076,
  0x007a, 0x007c, 0x007e, 0x0080, 0x0082, 0x0084, 0x0086, 0x0088,
  0x008a, 0x008c, 0x008f, 0x0091, 0x0094, 0x0097, 0x009a, 0x009d,
};

const UINT16 brc_quant_ac_table_vp8_g7[128] =
{
  0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b,
  0x000c, 0x000d, 0x000e, 0x000f, 0x0010, 0x0011, 0x0012, 0x0013,
  0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b,
  0x001c, 0x001d, 0x001e, 0x001f, 0x0020, 0x0021, 0x0022, 0x0023,
  0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b,
  0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031, 0x0032, 0x0033,
  0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003c,
  0x003e, 0x0040, 0x0042, 0x0044, 0x0046, 0x0048, 0x004a, 0x004c,
  0x004e, 0x0050, 0x0052, 0x0054, 0x0056, 0x0058, 0x005a, 0x005c,
  0x005e, 0x0060, 0x0062, 0x0064, 0x0066, 0x0068, 0x006a, 0x006c,
  0x006e, 0x0070, 0x0072, 0x0074, 0x0077, 0x007a, 0x007d, 0x0080,
  0x0083, 0x0086, 0x0089, 0x008c, 0x008f, 0x0092, 0x0095, 0x0098,
  0x009b, 0x009e, 0x00a1, 0x00a4, 0x00a7, 0x00aa, 0x00ad, 0x00b1,
  0x00b5, 0x00b9, 0x00bd, 0x00c1, 0x00c5, 0x00c9, 0x00cd, 0x00d1,
  0x00d5, 0x00d9, 0x00dd, 0x00e1, 0x00e5, 0x00ea, 0x00ef, 0x00f5,
  0x00f9, 0x00fe, 0x0103, 0x0108, 0x010d, 0x0112, 0x0117, 0x011c,
};

const UINT16 brc_skip_mv_threshold_table_vp8_g7[128] =
{
  0x006f, 0x0078, 0x0081, 0x0089, 0x0092, 0x009b, 0x00a3, 0x00ac,
  0x00b4, 0x00bd, 0x00c6, 0x00ce, 0x00d7, 0x00e0, 0x00e8, 0x00f1,
  0x00f9, 0x0102, 0x010b, 0x0113, 0x011c, 0x0125, 0x012d, 0x0136,
  0x013e, 0x0147, 0x0150, 0x0158, 0x0161, 0x016a, 0x0172, 0x017b,
  0x0183, 0x018c, 0x0195, 0x019d, 0x01a6, 0x01af, 0x01b7, 0x01c0,
  0x01c8, 0x01d1, 0x01da, 0x01e2, 0x01eb, 0x01f4, 0x01fc, 0x0205,
  0x020d, 0x0216, 0x021f, 0x0227, 0x0230, 0x0239, 0x0241, 0x024a,
  0x0252, 0x025b, 0x0264, 0x026c, 0x0275, 0x027e, 0x0286, 0x028f,
  0x0297, 0x02a0, 0x02a9, 0x02b1, 0x02ba, 0x02c3, 0x02cb, 0x02d4,
  0x02dd, 0x02e5, 0x02ee, 0x02f6, 0x02ff, 0x0308, 0x0310, 0x0319,
  0x0322, 0x032a, 0x0333, 0x033b, 0x0344, 0x034d, 0x0355, 0x035e,
  0x0367, 0x036f, 0x0378, 0x0380, 0x0389, 0x0392, 0x039a, 0x03a3,
  0x03ac, 0x03b4, 0x03bd, 0x03c5, 0x03ce, 0x03d7, 0x03df, 0x03e8,
  0x03f1, 0x03f9, 0x0402, 0x040a, 0x0413, 0x041c, 0x0424, 0x042d,
  0x0436, 0x043e, 0x0447, 0x044f, 0x0458, 0x0461, 0x0469, 0x0472,
  0x047b, 0x0483, 0x048c, 0x0494, 0x049d, 0x04a6, 0x04ae, 0x04b8,
};

VOID
media_sampler_setup_mbenc_g7 (MEDIA_ENCODER_CTX * encoder_context)
{
  MBENC_CONTEXT *mbenc_ctx = &encoder_context->mbenc_context;
  MEDIA_GPE_CTX *mbenc_gpe_ctx = &mbenc_ctx->gpe_context;
  UINT sampler_size = mbenc_gpe_ctx->sampler_size;
  dri_bo *bo;
  BYTE *sampler_ptr, *sampler_start_ptr;

  bo = mbenc_ctx->gpe_context.dynamic_state.res.bo;
  dri_bo_map (bo, 1);
  MEDIA_DRV_ASSERT (bo->virtual);
  sampler_start_ptr = (BYTE *) bo->virtual + mbenc_gpe_ctx->sampler_offset;
#if 0
  //For MBENC I LUMA
  sampler_ptr = sampler_start_ptr + (sampler_size * MBENC_ILUMA_START_OFFSET);
  media_drv_memcpy (sampler_ptr, sampler_size * 8,
		    (BYTE *) & vme_lut_sp_state_vp8_g7[0][0],
		    sampler_size * 8);
#endif
  //For MBENC P
  sampler_ptr = sampler_start_ptr + (sampler_size * MBENC_P_START_OFFSET);
  media_drv_memcpy (sampler_ptr, sampler_size * 8,
		    (BYTE *) & vme_lut_sp_state_vp8_g7[0][0],
		    sampler_size * 8);
  dri_bo_unmap (bo);
}

VOID
media_set_curbe_i_vp8_mbenc_g7 (struct encode_state *encode_state,
				MEDIA_MBENC_CURBE_PARAMS_VP8 * params)
{

  VAQMatrixBufferVP8 *quant_params =
    (VAQMatrixBufferVP8 *) encode_state->q_matrix->buffer;
  VAEncSequenceParameterBufferVP8 *seq_params =
    (VAEncSequenceParameterBufferVP8 *) encode_state->seq_param_ext->buffer;
  VAEncPictureParameterBufferVP8 *pic_params =
    (VAEncPictureParameterBufferVP8 *) encode_state->pic_param_ext->buffer;
  UINT segmentation_enabled = pic_params->pic_flags.bits.segmentation_enabled;
  MEDIA_CURBE_DATA_MBENC_I_G75 *cmd =
    (MEDIA_CURBE_DATA_MBENC_I_G75 *) params->curbe_cmd_buff;
  UINT16 y_quanta_dc_idx, uv_quanta_dc_idx, uv_quanta_ac_idx;

  media_drv_memset (cmd, sizeof (*cmd));
  cmd->dw0.frame_width = (seq_params->frame_width + 15) & (~0xF);	/* kernel require MB boundary aligned dimensions */
  cmd->dw0.frame_height = (seq_params->frame_height + 15) & (~0xF);
  cmd->dw1.frame_type = 0;	/*key frame(I-frame) */
  cmd->dw1.enable_segmentation = segmentation_enabled;
  cmd->dw1.enable_hw_intra_prediction =
    (params->kernel_mode == PERFORMANCE_MODE) ? 1 : 0;
  cmd->dw1.enable_debug_dumps = 0;
  cmd->dw1.enable_chroma_ip_enhancement = 1;	/* always enabled and cannot be disabled */
  cmd->dw1.enable_mpu_histogram_update = 1;
  cmd->dw1.vme_enable_tm_check = 0;
  cmd->dw1.vme_distortion_measure = 2;	//defualt value is 2-HAAR transform
  cmd->dw1.reserved_mbz = 1;	//do we need to set this reserved bit to 1?

  y_quanta_dc_idx =
    quant_params->quantization_index[0] +
    quant_params->quantization_index_delta[0];
  y_quanta_dc_idx =
    y_quanta_dc_idx < 0 ? 0 : (y_quanta_dc_idx >
			       MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
			       y_quanta_dc_idx);
  cmd->dw2.lambda_seg_0 =
    (UINT16) ((quant_dc_vp8_g75[y_quanta_dc_idx] *
	       quant_dc_vp8_g75[y_quanta_dc_idx]) / 4);

  if (segmentation_enabled)
    {
      y_quanta_dc_idx =
	quant_params->quantization_index[1] +
	quant_params->quantization_index_delta[0];
      y_quanta_dc_idx =
	y_quanta_dc_idx < 0 ? 0 : (y_quanta_dc_idx >
				   MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				   y_quanta_dc_idx);
      cmd->dw2.lambda_seg_1 =
	(UINT16) ((quant_dc_vp8_g75[y_quanta_dc_idx] *
		   quant_dc_vp8_g75[y_quanta_dc_idx]) / 4);

      y_quanta_dc_idx =
	quant_params->quantization_index[2] +
	quant_params->quantization_index_delta[0];
      y_quanta_dc_idx =
	y_quanta_dc_idx < 0 ? 0 : (y_quanta_dc_idx >
				   MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				   y_quanta_dc_idx);
      cmd->dw3.lambda_seg_2 =
	(UINT16) ((quant_dc_vp8_g75[y_quanta_dc_idx] *
		   quant_dc_vp8_g75[y_quanta_dc_idx]) / 4);

      y_quanta_dc_idx =
	quant_params->quantization_index[3] +
	quant_params->quantization_index_delta[0];
      y_quanta_dc_idx =
	y_quanta_dc_idx < 0 ? 0 : (y_quanta_dc_idx >
				   MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				   y_quanta_dc_idx);
      cmd->dw3.lambda_seg_3 =
	(UINT16) ((quant_dc_vp8_g75[y_quanta_dc_idx] *
		   quant_dc_vp8_g75[y_quanta_dc_idx]) / 4);
    }

  cmd->dw4.all_dc_bias_segment_0 = DC_BIAS_SEGMENT_DEFAULT_VAL_VP8;
  if (segmentation_enabled)
    {
      cmd->dw4.all_dc_bias_segment_1 = DC_BIAS_SEGMENT_DEFAULT_VAL_VP8;
      cmd->dw5.all_dc_bias_segment_2 = DC_BIAS_SEGMENT_DEFAULT_VAL_VP8;
      cmd->dw5.all_dc_bias_segment_3 = DC_BIAS_SEGMENT_DEFAULT_VAL_VP8;
    }

  uv_quanta_dc_idx =
    quant_params->quantization_index[0] +
    quant_params->quantization_index_delta[1];
  uv_quanta_dc_idx =
    uv_quanta_dc_idx < 0 ? 0 : (uv_quanta_dc_idx >
				MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				uv_quanta_dc_idx);
  cmd->dw6.chroma_dc_de_quant_segment_0 = quant_dc_vp8_g75[uv_quanta_dc_idx];
  if (segmentation_enabled)
    {
      uv_quanta_dc_idx =
	quant_params->quantization_index[1] +
	quant_params->quantization_index_delta[1];
      uv_quanta_dc_idx =
	uv_quanta_dc_idx < 0 ? 0 : (uv_quanta_dc_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_dc_idx);
      cmd->dw6.chroma_dc_de_quant_segment_1 =
	quant_dc_vp8_g75[uv_quanta_dc_idx];
      uv_quanta_dc_idx =
	quant_params->quantization_index[2] +
	quant_params->quantization_index_delta[1];
      uv_quanta_dc_idx =
	uv_quanta_dc_idx < 0 ? 0 : (uv_quanta_dc_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_dc_idx);
      cmd->dw7.chroma_dc_de_quant_segment_2 =
	quant_dc_vp8_g75[uv_quanta_dc_idx];
      uv_quanta_dc_idx =
	quant_params->quantization_index[3] +
	quant_params->quantization_index_delta[1];
      uv_quanta_dc_idx =
	uv_quanta_dc_idx < 0 ? 0 : (uv_quanta_dc_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_dc_idx);
      cmd->dw7.chroma_dc_de_quant_segment_3 =
	quant_dc_vp8_g75[uv_quanta_dc_idx];
    }

  uv_quanta_ac_idx =
    quant_params->quantization_index[0] +
    quant_params->quantization_index_delta[2];
  uv_quanta_ac_idx =
    uv_quanta_ac_idx < 0 ? 0 : (uv_quanta_ac_idx >
				MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				uv_quanta_ac_idx);
  cmd->dw8.chroma_ac_de_quant_segment0 = quant_ac_vp8_g75[uv_quanta_ac_idx];
  cmd->dw10.chroma_ac0_threshold0_segment0 =
    (UINT16) ((((((1) << 16) -
		 1) * 1.0 / ((1 << 16) / quant_ac_vp8_g75[uv_quanta_ac_idx]) -
		((48 * quant_ac_vp8_g75[uv_quanta_ac_idx]) >> 7)) *
	       (1 << 13) + 3400) / 2217.0);
  cmd->dw10.chroma_ac0_threshold1_segment0 =
    (UINT16) ((((((2) << 16) -
		 1) * 1.0 / ((1 << 16) / quant_ac_vp8_g75[uv_quanta_ac_idx]) -
		((48 * quant_ac_vp8_g75[uv_quanta_ac_idx]) >> 7)) *
	       (1 << 13) + 3400) / 2217.0);
  if (segmentation_enabled)
    {
      uv_quanta_ac_idx =
	quant_params->quantization_index[1] +
	quant_params->quantization_index_delta[2];
      uv_quanta_ac_idx =
	uv_quanta_ac_idx < 0 ? 0 : (uv_quanta_ac_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_ac_idx);
      cmd->dw8.chroma_ac_de_quant_segment1 =
	quant_ac_vp8_g75[uv_quanta_ac_idx];
      cmd->dw10.chroma_ac0_threshold0_segment0 =
	(UINT16) ((((((1) << 16) -
		     1) * 1.0 / ((1 << 16) /
				 quant_ac_vp8_g75[uv_quanta_ac_idx]) -
		    ((48 * quant_ac_vp8_g75[uv_quanta_ac_idx]) >> 7)) *
		   (1 << 13) + 3400) / 2217.0);
      cmd->dw10.chroma_ac0_threshold1_segment0 =
	(UINT16) ((((((2) << 16) -
		     1) * 1.0 / ((1 << 16) /
				 quant_ac_vp8_g75[uv_quanta_ac_idx]) -
		    ((48 * quant_ac_vp8_g75[uv_quanta_ac_idx]) >> 7)) *
		   (1 << 13) + 3400) / 2217.0);

      uv_quanta_ac_idx =
	quant_params->quantization_index[2] +
	quant_params->quantization_index_delta[2];
      uv_quanta_ac_idx =
	uv_quanta_ac_idx < 0 ? 0 : (uv_quanta_ac_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_ac_idx);
      cmd->dw9.chroma_ac_de_quant_segment2 =
	quant_ac_vp8_g75[uv_quanta_ac_idx];
      cmd->dw12.chroma_ac0_threshold0_segment2 =
	(UINT16) ((((((1) << 16) -
		     1) * 1.0 / ((1 << 16) /
				 quant_ac_vp8_g75[uv_quanta_ac_idx]) -
		    ((48 * quant_ac_vp8_g75[uv_quanta_ac_idx]) >> 7)) *
		   (1 << 13) + 3400) / 2217.0);
      cmd->dw12.chroma_ac0_threshold1_segment2 =
	(UINT16) ((((((2) << 16) -
		     1) * 1.0 / ((1 << 16) /
				 quant_ac_vp8_g75[uv_quanta_ac_idx]) -
		    ((48 * quant_ac_vp8_g75[uv_quanta_ac_idx]) >> 7)) *
		   (1 << 13) + 3400) / 2217.0);

      uv_quanta_ac_idx =
	quant_params->quantization_index[3] +
	quant_params->quantization_index_delta[2];
      uv_quanta_ac_idx =
	uv_quanta_ac_idx < 0 ? 0 : (uv_quanta_ac_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_ac_idx);
      cmd->dw9.chroma_ac_de_quant_segment3 =
	quant_ac_vp8_g75[uv_quanta_ac_idx];
      cmd->dw13.chroma_ac0_threshold0_segment3 =
	(UINT16) ((((((1) << 16) -
		     1) * 1.0 / ((1 << 16) /
				 quant_ac_vp8_g75[uv_quanta_ac_idx]) -
		    ((48 * quant_ac_vp8_g75[uv_quanta_ac_idx]) >> 7)) *
		   (1 << 13) + 3400) / 2217.0);
      cmd->dw13.chroma_ac0_threshold1_segment3 =
	(UINT16) ((((((2) << 16) -
		     1) * 1.0 / ((1 << 16) /
				 quant_ac_vp8_g75[uv_quanta_ac_idx]) -
		    ((48 * quant_ac_vp8_g75[uv_quanta_ac_idx]) >> 7)) *
		   (1 << 13) + 3400) / 2217.0);
    }

  uv_quanta_dc_idx =
    quant_params->quantization_index[0] +
    quant_params->quantization_index_delta[1];
  uv_quanta_dc_idx =
    uv_quanta_dc_idx < 0 ? 0 : (uv_quanta_dc_idx >
				MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				uv_quanta_dc_idx);
  cmd->dw14.chroma_dc_threshold0_segment0 =
    (((1) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
    ((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
  cmd->dw14.chroma_dc_threshold1_segment0 =
    (((2) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
    ((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
  cmd->dw15.chroma_dc_threshold2_segment0 =
    (((3) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
    ((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
  cmd->dw15.chroma_dc_threshold3_segment0 =
    (((4) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
    ((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
  if (segmentation_enabled)
    {
      uv_quanta_dc_idx =
	quant_params->quantization_index[1] +
	quant_params->quantization_index_delta[1];
      uv_quanta_dc_idx =
	uv_quanta_dc_idx < 0 ? 0 : (uv_quanta_dc_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_dc_idx);
      cmd->dw16.chroma_dc_threshold0_segment1 =
	(((1) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
	((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
      cmd->dw16.chroma_dc_threshold1_segment1 =
	(((2) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
	((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
      cmd->dw17.chroma_dc_threshold2_segment1 =
	(((3) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
	((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
      cmd->dw17.chroma_dc_threshold3_segment1 =
	(((4) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
	((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);

      uv_quanta_dc_idx =
	quant_params->quantization_index[2] +
	quant_params->quantization_index_delta[1];
      uv_quanta_dc_idx =
	uv_quanta_dc_idx < 0 ? 0 : (uv_quanta_dc_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_dc_idx);
      cmd->dw18.chroma_dc_threshold0_segment2 =
	(((1) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
	((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
      cmd->dw18.chroma_dc_threshold1_segment2 =
	(((2) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
	((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
      cmd->dw19.chroma_dc_threshold2_segment2 =
	(((3) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
	((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
      cmd->dw19.chroma_dc_threshold3_segment2 =
	(((4) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
	((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);

      uv_quanta_dc_idx =
	quant_params->quantization_index[3] +
	quant_params->quantization_index_delta[1];
      uv_quanta_dc_idx =
	uv_quanta_dc_idx < 0 ? 0 : (uv_quanta_dc_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_dc_idx);
      cmd->dw20.chroma_dc_threshold0_segment3 =
	(((1) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
	((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
      cmd->dw20.chroma_dc_threshold1_segment3 =
	(((2) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
	((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
      cmd->dw21.chroma_dc_threshold2_segment3 =
	(((3) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
	((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
      cmd->dw21.chroma_dc_threshold3_segment3 =
	(((4) << 16) - 1) / ((1 << 16) / quant_dc_vp8_g75[uv_quanta_dc_idx]) -
	((48 * quant_dc_vp8_g75[uv_quanta_dc_idx]) >> 7);
    }

  uv_quanta_ac_idx =
    quant_params->quantization_index[0] +
    quant_params->quantization_index_delta[2];
  uv_quanta_ac_idx =
    uv_quanta_ac_idx < 0 ? 0 : (uv_quanta_ac_idx >
				MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				uv_quanta_ac_idx);
  cmd->dw22.chroma_ac1_threshold_segment0 =
    ((1 << (16)) - 1) / ((1 << 16) / quant_ac_vp8_g75[uv_quanta_ac_idx]) -
    ((48 * quant_ac_vp8_g75[uv_quanta_ac_idx]) >> 7);
  if (segmentation_enabled)
    {
      uv_quanta_ac_idx =
	quant_params->quantization_index[1] +
	quant_params->quantization_index_delta[2];
      uv_quanta_ac_idx =
	uv_quanta_ac_idx < 0 ? 0 : (uv_quanta_ac_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_ac_idx);
      cmd->dw22.chroma_ac1_threshold_segment1 =
	((1 << (16)) - 1) / ((1 << 16) / quant_ac_vp8_g75[uv_quanta_ac_idx]) -
	((48 * quant_ac_vp8_g75[uv_quanta_ac_idx]) >> 7);

      uv_quanta_ac_idx =
	quant_params->quantization_index[2] +
	quant_params->quantization_index_delta[2];
      uv_quanta_ac_idx =
	uv_quanta_ac_idx < 0 ? 0 : (uv_quanta_ac_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_ac_idx);
      cmd->dw23.chroma_ac1_threshold_segment2 =
	((1 << (16)) - 1) / ((1 << 16) / quant_ac_vp8_g75[uv_quanta_ac_idx]) -
	((48 * quant_ac_vp8_g75[uv_quanta_ac_idx]) >> 7);
      uv_quanta_ac_idx =
	quant_params->quantization_index[3] +
	quant_params->quantization_index_delta[2];
      uv_quanta_ac_idx =
	uv_quanta_ac_idx < 0 ? 0 : (uv_quanta_ac_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_ac_idx);
      cmd->dw23.chroma_ac1_threshold_segment3 =
	((1 << (16)) - 1) / ((1 << 16) / quant_ac_vp8_g75[uv_quanta_ac_idx]) -
	((48 * quant_ac_vp8_g75[uv_quanta_ac_idx]) >> 7);
    }

  uv_quanta_dc_idx =
    quant_params->quantization_index[0] +
    quant_params->quantization_index_delta[0];
  uv_quanta_dc_idx =
    uv_quanta_dc_idx < 0 ? 0 : (uv_quanta_dc_idx >
				MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				uv_quanta_dc_idx);
  if (segmentation_enabled)
    {
      uv_quanta_dc_idx =
	quant_params->quantization_index[1] +
	quant_params->quantization_index_delta[0];
      uv_quanta_dc_idx =
	uv_quanta_dc_idx < 0 ? 0 : (uv_quanta_dc_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_dc_idx);
      uv_quanta_dc_idx =
	quant_params->quantization_index[2] +
	quant_params->quantization_index_delta[0];
      uv_quanta_dc_idx =
	uv_quanta_dc_idx < 0 ? 0 : (uv_quanta_dc_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_dc_idx);
      uv_quanta_dc_idx =
	quant_params->quantization_index[3] +
	quant_params->quantization_index_delta[0];
      uv_quanta_dc_idx =
	uv_quanta_dc_idx < 0 ? 0 : (uv_quanta_dc_idx >
				    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 :
				    uv_quanta_dc_idx);
    }
  cmd->dw32.mb_enc_per_mb_out_data_surf_bti = 0;
  cmd->dw33.mb_enc_curr_y_bti = 1;
  cmd->dw34.mb_enc_curr_uv_bti = 1;	//2
  cmd->dw35.mb_mode_cost_luma_bti = 3;
  cmd->dw36.mb_enc_block_mode_cost_bti = 4;
  cmd->dw37.chroma_recon_surf_bti = 5;
  cmd->dw38.segmentation_map_bti = 6;
  cmd->dw39.histogram_bti = 7;
  cmd->dw40.mb_enc_vme_debug_stream_out_bti = 8;
  cmd->dw41.vme_bti = 9;

  if (params->mb_enc_iframe_dist_in_use)
    {
      cmd->dw41.vme_bti = 0;
      cmd->dw42.idist_surface = 12;
      cmd->dw43.curr_y_surface4x_downscaled = 11;
      cmd->dw44.vme_coarse_intra_surface = 10;
    }
}

VOID
media_set_curbe_p_vp8_mbenc_g7 (struct encode_state *encode_state,
				MEDIA_MBENC_CURBE_PARAMS_VP8 * params)
{
  VAQMatrixBufferVP8 *quant_params =
    (VAQMatrixBufferVP8 *) encode_state->q_matrix->buffer;
  VAEncSequenceParameterBufferVP8 *seq_params =
    (VAEncSequenceParameterBufferVP8 *) encode_state->seq_param_ext->buffer;
  VAEncPictureParameterBufferVP8 *pic_params =
    (VAEncPictureParameterBufferVP8 *) encode_state->pic_param_ext->buffer;
  UINT segmentation_enabled = pic_params->pic_flags.bits.segmentation_enabled;
  UINT sign_bias_golden = pic_params->pic_flags.bits.sign_bias_golden;
  UINT sign_bias_alternate = pic_params->pic_flags.bits.sign_bias_alternate;
  UINT version = pic_params->pic_flags.bits.version;
  MEDIA_CURBE_DATA_MBENC_P_G7 *cmd =
    (MEDIA_CURBE_DATA_MBENC_P_G7 *) params->curbe_cmd_buff;;
  UINT16 luma_dc_seg0, luma_dc_seg1, luma_dc_seg2, luma_dc_seg3;
  UINT16 qp_seg0, qp_seg1, qp_seg2, qp_seg3;

  media_drv_memset (cmd, sizeof (MEDIA_CURBE_DATA_MBENC_P_G75));

  luma_dc_seg0 =
    quant_params->quantization_index[0] +
    quant_params->quantization_index_delta[0];
  luma_dc_seg1 =
    quant_params->quantization_index[1] +
    quant_params->quantization_index_delta[0];
  luma_dc_seg2 =
    quant_params->quantization_index[2] +
    quant_params->quantization_index_delta[0];
  luma_dc_seg3 =
    quant_params->quantization_index[3] +
    quant_params->quantization_index_delta[0];

  qp_seg0 =
    luma_dc_seg0 < 0 ? 0 : (luma_dc_seg0 >
			    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 : luma_dc_seg0);
  qp_seg1 =
    luma_dc_seg1 < 0 ? 0 : (luma_dc_seg1 >
			    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 : luma_dc_seg1);
  qp_seg2 =
    luma_dc_seg2 < 0 ? 0 : (luma_dc_seg2 >
			    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 : luma_dc_seg2);
  qp_seg3 =
    luma_dc_seg3 < 0 ? 0 : (luma_dc_seg3 >
			    MAX_QP_VP8_G75 ? MAX_QP_VP8_G75 : luma_dc_seg3);


  //BYTE me_method = (params->kernel_mode == NORMAL_MODE) ? 6 : 4;
  //dw0
  cmd->dw0.frame_width = (seq_params->frame_width + 15) & (~0xF);
  cmd->dw0.frame_height = (seq_params->frame_height + 15) & (~0xF);
  // dw1
  cmd->dw1.frame_type = 1;	// P-frame
  cmd->dw1.motion_compensation_filter_type =
    (version == 0) ? 0 /*6-tap filter */ :
    ((version == 3) ? 2 : 1);

  cmd->dw1.hme_enable = params->hme_enabled;
  cmd->dw1.hme_combine_overlap = (params->kernel_mode == NORMAL_MODE) ? 1 : 2;
  cmd->dw1.hme_combined_extra_su = 0;
  cmd->dw1.multi_pred_en = 0;
  cmd->dw1.ref_ctrl = params->ref_frame_ctrl;
  cmd->dw1.enable_segmentation = segmentation_enabled;
  cmd->dw1.enable_segmentation_info_update = 0;
  cmd->dw1.multi_reference_qp_check = 1;
  cmd->dw1.mode_cost_enable_flag = 0;
  cmd->dw1.enable_debug_dumps = 0;
  cmd->dw1.all_fractional = 0;
  cmd->dw1.enable_coeff_clamp = 0;

  if (params->brc_enabled)
    cmd->dw1.mode_cost_enable_flag = 1;

  //dw2
  cmd->dw2.lambda_intra_segment0 = quant_dc_vp8_g75[qp_seg0];
  cmd->dw2.lambda_inter_segment0 = (quant_dc_vp8_g75[qp_seg0] >> 2);

  if (segmentation_enabled)
    {
      //dw3
      cmd->dw3.lambda_intra_segment1 = (quant_dc_vp8_g75[qp_seg1]);
      cmd->dw3.lambda_inter_segment1 = (quant_dc_vp8_g75[qp_seg1] >> 2);
      //dw4
      cmd->dw4.lambda_intra_segment2 = (quant_dc_vp8_g75[qp_seg2]);
      cmd->dw4.lambda_inter_segment2 = (quant_dc_vp8_g75[qp_seg2] >> 2);
      //dw5
      cmd->dw5.lambda_intra_segment3 = (quant_dc_vp8_g75[qp_seg3]);
      cmd->dw5.lambda_inter_segment3 = (quant_dc_vp8_g75[qp_seg3] >> 2);
    }

  cmd->dw6.reference_frame_sign_bias_3 = sign_bias_golden;
  cmd->dw6.reference_frame_sign_bias_2 = sign_bias_alternate;
  cmd->dw6.reference_frame_sign_bias_1 =
    sign_bias_golden ^ sign_bias_alternate;
  cmd->dw6.reference_frame_sign_bias_0 = 0;
  //dw7
  cmd->dw7.raw_dist_threshold = 0;	//kernel is currently setting it to 0
  //dw8
  cmd->dw8.early_ime_successful_stop_threshold = 0;
  cmd->dw8.transform8x8_flag_for_inter_enable = 0;
  cmd->dw8.early_ime_success_enable = 0;
  cmd->dw8.bidirectional_mix_disbale = 0;
  cmd->dw8.adaptive_search_enable =
    (params->kernel_mode != PERFORMANCE_MODE) ? 1 : 0;
  cmd->dw8.skip_mode_enable = 1;
  //dw9
  cmd->dw9.ref_pixel_bias_enable = 0;
  cmd->dw9.unidirection_mix_enable = 0;
  cmd->dw9.bidirectional_weight = 32;
  cmd->dw9.ref_id_polarity_bits = 0;
  cmd->dw9.max_num_of_motion_vectors = 32;
  //dw10
  cmd->dw10.max_fixed_search_path_length =
    (params->kernel_mode ==
     NORMAL_MODE) ? 16 : ((params->kernel_mode == PERFORMANCE_MODE) ? 9 : 57);
  cmd->dw10.maximum_search_path_length = 57;
  cmd->dw10.start_centre_0x =
    ((((params->kernel_mode !=
	PERFORMANCE_MODE) ? 48 : 28) - 16) >> 3) & 0x0F;
  cmd->dw10.start_centre_0y =
    ((((params->kernel_mode !=
	PERFORMANCE_MODE) ? 40 : 28) - 16) >> 3) & 0x0F;
  //dw11

  cmd->dw11.submacro_block_subpartition_mask = 0 /* 0x30 */ ;	//from Bdw
  cmd->dw11.intra_sad_measure_adjustment = 2;
  cmd->dw11.inter_sad_measure_adjustment = 2;
  cmd->dw11.block_based_skip_enable = 0;
  cmd->dw11.bme_disable_for_fbr_message = 0 /* 1 */ ;	// from Bdw
  cmd->dw11.forward_trans_form_skip_check_enable = 0;
  cmd->dw11.process_inter_chroma_pixels_mode = 0;
  cmd->dw11.disable_field_cache_allocation = 0;
  cmd->dw11.skip_mode_type = 0;
  cmd->dw11.sub_pel_mode = 3;
  cmd->dw11.dual_search_path_option = 0;
  cmd->dw11.search_control = 0;
  cmd->dw11.reference_access = 0;
  cmd->dw11.source_access = 0;
  cmd->dw11.inter_mb_type_road_map = 0;
  cmd->dw11.source_block_size = 0;
  //dw12
  cmd->dw12.reference_search_windows_height =
    (params->kernel_mode != PERFORMANCE_MODE) ? 40 : 28;
  cmd->dw12.reference_search_windows_width =
    (params->kernel_mode != PERFORMANCE_MODE) ? 48 : 28;
  //dw13
  cmd->dw13.bilinear_enable = 0;
  cmd->dw13.mv_cost_scale_factor = 0;
  cmd->dw13.hme_combined_len = 8;	//based on target usage part of par file param

  if (cmd->dw1.mode_cost_enable_flag == 0) {
    //dw14
    cmd->dw14.frame_count_probability_ref_frame_cost_0 = 516;
    cmd->dw14.frame_count_probability_ref_frame_cost_1 = 106;
    //dw15
    cmd->dw15.frame_count_probability_ref_frame_cost_2 = 2407;
    cmd->dw15.frame_count_probability_ref_frame_cost_3 = 2409;
  }

  //dw16
  cmd->dw16.mv_ref_cost_context_0_0_1 = 10;
  cmd->dw16.mv_ref_cost_context_0_0_0 = 1328;
  //dw17
  cmd->dw17.mv_ref_cost_context_0_1_1 = 1;
  cmd->dw17.mv_ref_cost_context_0_1_0 = 2047;
  //dw18
  cmd->dw18.mv_ref_cost_context_0_2_1 = 1;
  cmd->dw18.mv_ref_cost_context_0_2_0 = 2047;
  //dw19
  cmd->dw19.mv_ref_cost_context_0_3_1 = 304;
  cmd->dw19.mv_ref_cost_context_0_3_0 = 214;
  //dw20
  cmd->dw20.mv_ref_cost_context_1_0_1 = 21;
  cmd->dw20.mv_ref_cost_context_1_0_0 = 1072;
  //dw21
  cmd->dw21.mv_ref_cost_context_1_1_1 = 27;
  cmd->dw21.mv_ref_cost_context_1_1_0 = 979;
  //dw22
  cmd->dw22.mv_ref_cost_context_1_2_1 = 21;
  cmd->dw22.mv_ref_cost_context_1_2_0 = 1072;
  //dw23
  cmd->dw23.mv_ref_cost_context_1_3_1 = 201;
  cmd->dw23.mv_ref_cost_context_1_3_0 = 321;
  //dw24
  cmd->dw24.mv_ref_cost_context_2_0_1 = 278;
  cmd->dw24.mv_ref_cost_context_2_0_0 = 235;
  //dw25
  cmd->dw25.mv_ref_cost_context_2_1_1 = 107;
  cmd->dw25.mv_ref_cost_context_2_1_0 = 511;
  //dw26
  cmd->dw26.mv_ref_cost_context_2_2_1 = 93;
  cmd->dw26.mv_ref_cost_context_2_2_0 = 553;
  //dw27
  cmd->dw27.mv_ref_cost_context_2_3_1 = 115;
  cmd->dw27.mv_ref_cost_context_2_3_0 = 488;
  //dw28
  cmd->dw28.mv_ref_cost_context_3_0_1 = 99;
  cmd->dw28.mv_ref_cost_context_3_0_0 = 534;
  //dw29
  cmd->dw29.mv_ref_cost_context_3_1_1 = 92;
  cmd->dw29.mv_ref_cost_context_3_1_0 = 560;
  //dw30
  cmd->dw30.mv_ref_cost_context_3_2_1 = 257;
  cmd->dw30.mv_ref_cost_context_3_2_0 = 255;
  //dw31
  cmd->dw31.mv_ref_cost_context_3_3_1 = 109;
  cmd->dw31.mv_ref_cost_context_3_3_0 = 505;
  //dw32
  cmd->dw32.mv_ref_cost_context_4_0_1 = 361;
  cmd->dw32.mv_ref_cost_context_4_0_0 = 174;
  //dw33
  cmd->dw33.mv_ref_cost_context_4_1_1 = 275;
  cmd->dw33.mv_ref_cost_context_4_1_0 = 238;
  //dw34
  cmd->dw34.mv_ref_cost_context_4_2_1 = 257;
  cmd->dw34.mv_ref_cost_context_4_2_0 = 255;
  //dw35
  cmd->dw35.mv_ref_cost_context_4_3_1 = 53;
  cmd->dw35.mv_ref_cost_context_4_3_0 = 744;
  //dw36
  cmd->dw36.mv_ref_cost_context_5_0_1 = 922;
  cmd->dw36.mv_ref_cost_context_5_0_0 = 32;
  //dw37
  cmd->dw37.mv_ref_cost_context_5_1_1 = 494;
  cmd->dw37.mv_ref_cost_context_5_1_0 = 113;
  //dw38
  cmd->dw38.mv_ref_cost_context_5_2_1 = 257;
  cmd->dw38.mv_ref_cost_context_5_2_0 = 255;
  //dw39
  cmd->dw39.mv_ref_cost_context_5_3_1 = 43;
  cmd->dw39.mv_ref_cost_context_5_3_0 = 816;
  //dw40
  if (!params->brc_enabled) {
    cmd->dw40.average_qp_of_alt_ref_frame = quant_dc_vp8_g75[qp_seg0];	//0x0f;
    cmd->dw40.average_qp_of_gold_ref_frame = quant_dc_vp8_g75[qp_seg0];	//0x0f;
    cmd->dw40.average_qp_of_last_ref_frame = quant_dc_vp8_g75[qp_seg0];	//0x0f; may have to change this later
  } else {
    cmd->dw40.average_qp_of_alt_ref_frame = params->frame_update->ref_q_index[0];
    cmd->dw40.average_qp_of_gold_ref_frame = params->frame_update->ref_q_index[1];
    cmd->dw40.average_qp_of_last_ref_frame = params->frame_update->ref_q_index[2];
  }

  //dw41
  cmd->dw41.mv_skip_threshold0 = new_mv_skip_threshold_VP8_g75[qp_seg0];

  if (segmentation_enabled)
    {
      cmd->dw41.mv_skip_threshold1 = new_mv_skip_threshold_VP8_g75[qp_seg1];
      //dw42
      cmd->dw42.mv_skip_threshold2 = new_mv_skip_threshold_VP8_g75[qp_seg2];
      cmd->dw42.mv_skip_threshold3 = new_mv_skip_threshold_VP8_g75[qp_seg3];
    }
  //dw43
  cmd->dw43.intra_16x16_no_dc_penalty_segment0 =
    cost_table_vp8_g7[qp_seg0][5];

  if (params->brc_enabled == TRUE)
  {
    cmd->dw43.intra_16x16_no_dc_penalty_segment0 = 0;
  }

  if (segmentation_enabled)
    {
      cmd->dw43.intra_16x16_no_dc_penalty_segment1 =
	cost_table_vp8_g7[qp_seg1][5];
      cmd->dw43.intra_16x16_no_dc_penalty_segment2 =
	cost_table_vp8_g7[qp_seg2][5];
      cmd->dw43.intra_16x16_no_dc_penalty_segment3 =
	cost_table_vp8_g7[qp_seg3][5];
    }
  //dw44
  cmd->dw44.intra_4x4_no_dc_penalty_segment0 = cost_table_vp8_g7[qp_seg0][6];

  if (segmentation_enabled)
    {
      cmd->dw44.intra_4x4_no_dc_penalty_segment1 =
	cost_table_vp8_g7[qp_seg1][6];
      cmd->dw44.intra_4x4_no_dc_penalty_segment2 =
	cost_table_vp8_g7[qp_seg2][6];
      cmd->dw44.intra_4x4_no_dc_penalty_segment3 =
	cost_table_vp8_g7[qp_seg3][6];
    }
  //dw45
  cmd->dw45.reserved_1 = 0;
  cmd->dw45.reserved_2 = 143;
  cmd->dw45.reserved_3 = 175;
  cmd->dw45.reserved_4 = 0;
  //dw46
  cmd->dw46.qp_index_seg0 = qp_seg0;
  if (segmentation_enabled)
    {
      cmd->dw46.qp_index_seg1 = qp_seg1;
      cmd->dw46.qp_index_seg2 = qp_seg2;
      cmd->dw46.qp_index_seg3 = qp_seg3;
    }
  //dw61
  cmd->dw61.vme_spl_ut0 = 0;
  //dw62
  cmd->dw62.vme_spl_ut1 = 1;
  //dw63
  cmd->dw63.vme_spl_ut2 = 2;
  //dw64
  cmd->dw64.vme_spl_ut3 = 3;
  //dw65
  cmd->dw65.vme_spl_ut4 = 4;
  //dw66
  cmd->dw66.vme_spl_ut5 = 5;
  //dw67
  cmd->dw67.vme_spl_ut6 = 6;
  //dw68
  cmd->dw68.vme_spl_ut7 = 7;
  //setup binding table index entries
  cmd->dw48.output_data_surface_bti = 0;
  cmd->dw49.current_pic_y_surface_bti = 1;
  cmd->dw50.current_pic_uv_surface_bti = 1;
  cmd->dw51.hme_data_surface_bti = 3;
  cmd->dw52.mv_data_surface_bti = 4;
  cmd->dw53.seg_map_bti = 5;
  cmd->dw54.inter_pred_dis_bti = 6;
  cmd->dw55.mode_cost_update_bti = 7;
  cmd->dw56.near_cnt_bti = 7;
  cmd->dw57.cnt_index_2spindex_bti = 7;
  cmd->dw58.vme_inter_pred_last_ref_frame_bti = 10;
  cmd->dw59.vme_inter_pred_gold_ref_frame_bti = 12;
  cmd->dw60.vme_inter_pred_alt_ref_frame_bti = 14;
  cmd->dw69.kernel_debug_dump_bti = 16;

}

VOID
media_set_curbe_vp8_mbpak_g7 (struct encode_state *encode_state,
			      MEDIA_MBPAK_CURBE_PARAMS_VP8 * params)
{
  VAQMatrixBufferVP8 *quant_params =
    (VAQMatrixBufferVP8 *) encode_state->q_matrix->buffer;
  VAEncSequenceParameterBufferVP8 *seq_params =
    (VAEncSequenceParameterBufferVP8 *) encode_state->seq_param_ext->buffer;
  VAEncPictureParameterBufferVP8 *pic_params =
    (VAEncPictureParameterBufferVP8 *) encode_state->pic_param_ext->buffer;
  UINT shift_factor, mul_factor;
  UINT16 y_quanta_ac_idx, y_quanta_dc_idx, uv_quanta_dc_idx,
    uv_quanta_ac_idx, y2_quanta_ac_idx, y2_quanta_dc_idx;
  // qIndex should be the sum of base and delta qp values.
  y_quanta_ac_idx = quant_params->quantization_index[0];	/* Use entry 0 as for BDW segmentation is disabled */
  y_quanta_dc_idx =
    y_quanta_ac_idx +
    quant_params->quantization_index_delta[QUAND_INDEX_Y1_DC_VP8];
  uv_quanta_dc_idx =
    y_quanta_ac_idx +
    quant_params->quantization_index_delta[QUAND_INDEX_UV_DC_VP8];
  uv_quanta_ac_idx =
    y_quanta_ac_idx +
    quant_params->quantization_index_delta[QUAND_INDEX_UV_AC_VP8];
  y2_quanta_dc_idx =
    y_quanta_ac_idx +
    quant_params->quantization_index_delta[QUAND_INDEX_Y2_DC_VP8];
  y2_quanta_ac_idx =
    y_quanta_ac_idx +
    quant_params->quantization_index_delta[QUAND_INDEX_Y2_AC_VP8];

  shift_factor = 16;
  mul_factor = 1 << shift_factor;

  if (params->pak_phase_type == MBPAK_HYBRID_STATE_P1)
    {

      MEDIA_CURBE_DATA_MBPAK_P1_G75 *cmd =
	(MEDIA_CURBE_DATA_MBPAK_P1_G75 *) params->curbe_cmd_buff;
      media_drv_memset (cmd, sizeof (MEDIA_CURBE_DATA_MBPAK_P1_G75));
      cmd->dw0.frame_width = (seq_params->frame_width + 15) & (~0xF);	/* kernel require MB boundary aligned dimensions */
      cmd->dw0.frame_height = (seq_params->frame_height + 15) & (~0xF);

      cmd->dw1.frame_type = 1;	/* phase1 is always for P frames only */
      cmd->dw1.recon_filter_type =
	(pic_params->pic_flags.bits.version == 0) ? 0 /*6-tap filter */ :
	((pic_params->pic_flags.bits.version == 3) ? 2
	 /*full pixel mvs for chroma,half pixel mvs derived using bilinear filter for luma */
	 : 1 /*bilinear filter */ );
      cmd->dw1.clamping_flag = pic_params->pic_flags.bits.clamping_type;

      cmd->dw2.y_dc_q_mul_factor_segment0 =
	mul_factor / quant_dc_vp8_g75[y_quanta_dc_idx];
      cmd->dw2.y_ac_q_mul_factor_segment0 =
	mul_factor / quant_ac_vp8_g75[y_quanta_ac_idx];

      cmd->dw3.y2_dc_q_mul_factor_segment0 =
	mul_factor / quant_dc2_vp8_g75[y2_quanta_dc_idx];
      cmd->dw3.y2_ac_q_mul_factor_segment0 =
	mul_factor / quant_ac2_vp8_g75[y2_quanta_ac_idx];

      cmd->dw4.uv_dc_q_mul_factor_segment0 =
	mul_factor / quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
      cmd->dw4.uv_ac_q_mul_factor_segment0 =
	mul_factor / quant_ac_vp8_g75[uv_quanta_ac_idx];

      cmd->dw5.y_dc_inv_q_mul_factor_segment0 =
	quant_dc_vp8_g75[y_quanta_dc_idx];
      cmd->dw5.y_ac_inv_q_mul_factor_segment0 =
	quant_ac_vp8_g75[y_quanta_ac_idx];

      cmd->dw6.y2_dc_inv_q_mul_factor_segment0 =
	quant_dc2_vp8_g75[y2_quanta_dc_idx];
      cmd->dw6.y2_ac_inv_q_mul_factor_segment0 =
	quant_ac2_vp8_g75[y2_quanta_ac_idx];

      cmd->dw7.uv_dc_inv_q_mul_factor_segment0 =
	quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
      cmd->dw7.uv_ac_inv_q_mul_factor_segment0 =
	quant_ac_vp8_g75[uv_quanta_ac_idx];

      cmd->dw8.y2_dc_q_shift_factor_segment0 = shift_factor;
      cmd->dw8.y2_ac_q_shift_factor_segment0 = shift_factor;
      cmd->dw8.y_dc_q_shift_factor_segment0 = shift_factor;
      cmd->dw8.y_ac_q_shift_factor_segment0 = shift_factor;

      cmd->dw9.uv_dc_q_shift_factor_segment0 = shift_factor;
      cmd->dw9.uv_ac_q_shift_factor_segment0 = shift_factor;

      if (pic_params->pic_flags.bits.segmentation_enabled)
	{
	  y_quanta_ac_idx = quant_params->quantization_index[1];
	  y_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y1_DC_VP8];
	  uv_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_UV_DC_VP8];
	  uv_quanta_ac_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_UV_AC_VP8];
	  y2_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y2_DC_VP8];
	  y2_quanta_ac_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y2_AC_VP8];

	  cmd->dw10.y_dc_q_mul_factor_segment1 =
	    mul_factor / quant_dc_vp8_g75[y_quanta_dc_idx];
	  cmd->dw10.y_ac_q_mul_factor_segment1 =
	    mul_factor / quant_ac_vp8_g75[y_quanta_ac_idx];

	  cmd->dw11.y2_dc_q_mul_factor_segment1 =
	    mul_factor / quant_dc2_vp8_g75[y2_quanta_dc_idx];
	  cmd->dw11.y2_ac_q_mul_factor_segment1 =
	    mul_factor / quant_ac2_vp8_g75[y2_quanta_ac_idx];

	  cmd->dw12.uv_dc_q_mul_factor_segment1 =
	    mul_factor / quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
	  cmd->dw12.uv_ac_q_mul_factor_segment1 =
	    mul_factor / quant_ac_vp8_g75[uv_quanta_ac_idx];

	  cmd->dw13.y_dc_inv_q_mul_factor_segment1 =
	    quant_dc_vp8_g75[y_quanta_dc_idx];
	  cmd->dw13.y_ac_inv_q_mul_factor_segment1 =
	    quant_ac_vp8_g75[y_quanta_ac_idx];

	  cmd->dw14.y2_dc_inv_q_mul_factor_segment1 =
	    quant_dc2_vp8_g75[y2_quanta_dc_idx];
	  cmd->dw14.y2_ac_inv_q_mul_factor_segment1 =
	    quant_ac2_vp8_g75[y2_quanta_ac_idx];

	  cmd->dw15.uv_dc_inv_q_mul_factor_segment1 =
	    quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
	  cmd->dw15.uv_ac_inv_q_mul_factor_segment1 =
	    quant_ac_vp8_g75[uv_quanta_ac_idx];

	  cmd->dw16.y2_dc_q_shift_factor_segment1 = shift_factor;
	  cmd->dw16.y2_ac_q_shift_factor_segment1 = shift_factor;
	  cmd->dw16.y_dc_q_shift_factor_segment1 = shift_factor;
	  cmd->dw16.y_ac_q_shift_factor_segment1 = shift_factor;

	  cmd->dw17.uv_dc_q_shift_factor_segment1 = shift_factor;
	  cmd->dw17.uv_ac_q_shift_factor_segment1 = shift_factor;

	  y_quanta_ac_idx = quant_params->quantization_index[2];
	  y_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y1_DC_VP8];
	  uv_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_UV_DC_VP8];
	  uv_quanta_ac_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_UV_AC_VP8];
	  y2_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y2_DC_VP8];
	  y2_quanta_ac_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y2_AC_VP8];

	  cmd->dw18.y_dc_q_mul_factor_segment2 =
	    mul_factor / quant_dc_vp8_g75[y_quanta_dc_idx];
	  cmd->dw18.y_ac_q_mul_factor_segment2 =
	    mul_factor / quant_ac_vp8_g75[y_quanta_ac_idx];

	  cmd->dw19.y2_dc_q_mul_factor_segment2 =
	    mul_factor / quant_dc2_vp8_g75[y2_quanta_dc_idx];
	  cmd->dw19.y2_ac_q_mul_factor_segment2 =
	    mul_factor / quant_ac2_vp8_g75[y2_quanta_ac_idx];

	  cmd->dw20.uv_dc_q_mul_factor_segment2 =
	    mul_factor / quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
	  cmd->dw20.uv_ac_q_mul_factor_segment2 =
	    mul_factor / quant_ac_vp8_g75[uv_quanta_ac_idx];

	  cmd->dw21.y_dc_inv_q_mul_factor_segment2 =
	    quant_dc_vp8_g75[y_quanta_dc_idx];
	  cmd->dw21.y_ac_inv_q_mul_factor_segment2 =
	    quant_ac_vp8_g75[y_quanta_ac_idx];

	  cmd->dw22.y2_dc_inv_q_mul_factor_segment2 =
	    quant_dc2_vp8_g75[y2_quanta_dc_idx];
	  cmd->dw22.y2_ac_inv_q_mul_factor_segment2 =
	    quant_ac2_vp8_g75[y2_quanta_ac_idx];

	  cmd->dw23.uv_dc_inv_q_mul_factor_segment2 =
	    quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
	  cmd->dw23.uv_ac_inv_q_mul_factor_segment2 =
	    quant_ac_vp8_g75[uv_quanta_ac_idx];

	  cmd->dw24.y2_dc_q_shift_factor_segment2 = shift_factor;
	  cmd->dw24.y2_ac_q_shift_factor_segment2 = shift_factor;
	  cmd->dw24.y_dc_q_shift_factor_segment2 = shift_factor;
	  cmd->dw24.y_ac_q_shift_factor_segment2 = shift_factor;

	  cmd->dw25.uv_dc_q_shift_factor_segment2 = shift_factor;
	  cmd->dw25.uv_ac_q_shift_factor_segment2 = shift_factor;

	  y_quanta_ac_idx = quant_params->quantization_index[3];
	  y_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y1_DC_VP8];
	  uv_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_UV_DC_VP8];
	  uv_quanta_ac_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_UV_AC_VP8];
	  y2_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y2_DC_VP8];
	  y2_quanta_ac_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y2_AC_VP8];

	  cmd->dw26.y_dc_q_mul_factor_segment3 =
	    mul_factor / quant_dc_vp8_g75[y_quanta_dc_idx];
	  cmd->dw26.y_ac_q_mul_factor_segment3 =
	    mul_factor / quant_ac_vp8_g75[y_quanta_ac_idx];

	  cmd->dw27.y2_dc_q_mul_factor_segment3 =
	    mul_factor / quant_dc2_vp8_g75[y2_quanta_dc_idx];
	  cmd->dw27.y2_ac_q_mul_factor_segment3 =
	    mul_factor / quant_ac2_vp8_g75[y2_quanta_ac_idx];

	  cmd->dw28.uv_dc_q_mul_factor_segment3 =
	    mul_factor / quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
	  cmd->dw28.uv_ac_q_mul_factor_segment3 =
	    mul_factor / quant_ac_vp8_g75[uv_quanta_ac_idx];

	  cmd->dw29.y_dc_inv_q_mul_factor_segment3 =
	    quant_dc_vp8_g75[y_quanta_dc_idx];
	  cmd->dw29.y_ac_inv_q_mul_factor_segment3 =
	    quant_ac_vp8_g75[y_quanta_ac_idx];

	  cmd->dw30.y2_dc_inv_q_mul_factor_segment3 =
	    quant_dc2_vp8_g75[y2_quanta_dc_idx];
	  cmd->dw30.y2_ac_inv_q_mul_factor_segment3 =
	    quant_ac2_vp8_g75[y2_quanta_ac_idx];

	  cmd->dw31.uv_dc_inv_q_mul_factor_segment3 =
	    quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
	  cmd->dw31.uv_ac_inv_q_mul_factor_segment3 =
	    quant_ac_vp8_g75[uv_quanta_ac_idx];

	  cmd->dw32.y2_dc_q_shift_factor_segment3 = shift_factor;
	  cmd->dw32.y2_ac_q_shift_factor_segment3 = shift_factor;
	  cmd->dw32.y_dc_q_shift_factor_segment3 = shift_factor;
	  cmd->dw32.y_ac_q_shift_factor_segment3 = shift_factor;

	  cmd->dw33.uv_dc_q_shift_factor_segment3 = shift_factor;
	  cmd->dw33.uv_ac_q_shift_factor_segment3 = shift_factor;
	}

      cmd->dw34.ref_frame_lf_delta0 = pic_params->ref_lf_delta[0];
      cmd->dw34.ref_frame_lf_delta1 = pic_params->ref_lf_delta[1];
      cmd->dw34.ref_frame_lf_delta2 = pic_params->ref_lf_delta[2];	/*referenceFrame loopfilter delta for last frame */
      cmd->dw34.ref_frame_lf_delta3 = pic_params->ref_lf_delta[3];	/*referenceFrame loopfilter delta for intra frame */

      cmd->dw35.mode_lf_delta0 = pic_params->mode_lf_delta[0];
      cmd->dw35.mode_lf_delta1 = pic_params->mode_lf_delta[1];
      cmd->dw35.mode_lf_delta2 = pic_params->mode_lf_delta[2];
      cmd->dw35.mode_lf_delta3 = pic_params->mode_lf_delta[3];

      cmd->dw36.lf_level0 = pic_params->loop_filter_level[0];
      if (pic_params->pic_flags.bits.segmentation_enabled)
	{
	  cmd->dw36.lf_level1 = pic_params->loop_filter_level[1];
	  cmd->dw36.lf_level2 = pic_params->loop_filter_level[2];
	  cmd->dw36.lf_level3 = pic_params->loop_filter_level[3];
	}
      cmd->dw40.pak_per_mb_out_data_surf_bti = VP8_MBPAK_PER_MB_OUT_G75;
      cmd->dw41.mb_enc_curr_y_bti = VP8_MBPAK_CURR_Y_G75;
      cmd->dw42.pak_recon_y_bti = VP8_MBPAK_CURR_RECON_Y_G75;
      cmd->dw43.pak_last_ref_pic_y_bti = VP8_MBPAK_LAST_REF_Y_G75;
      cmd->dw44.pak_golden_ref_pic_y_bti = VP8_MBPAK_GOLDEN_REF_Y_G75;
      cmd->dw45.pak_alternate_ref_pic_y_bti = VP8_MBPAK_ALTERNATE_REF_Y_G75;
      cmd->dw46.pak_ind_mv_data_bti = VP8_MBPAK_IND_MV_DATA_G75;
      cmd->dw47.pak_kernel_debug_bti = VP8_MBPAK_DEBUG_STREAMOUT_G75;
    }
  else
    {
      MEDIA_CURBE_DATA_MBPAK_P2_G75 *cmd =
	(MEDIA_CURBE_DATA_MBPAK_P2_G75 *) params->curbe_cmd_buff;
      media_drv_memset (cmd, sizeof (MEDIA_CURBE_DATA_MBPAK_P2_G75));
      cmd->dw0.frame_width = (seq_params->frame_width + 15) & (~0xF);	/* kernel require MB boundary aligned dimensions */
      cmd->dw0.frame_height = (seq_params->frame_height + 15) & (~0xF);

      cmd->dw1.sharpness_level = pic_params->sharpness_level;
      cmd->dw1.loop_filter_type =
	(pic_params->pic_flags.bits.version ==
	 0) ? 0 /*normal loop filter */ : 1 /*simple loop filter */ ;
      cmd->dw1.frame_type = pic_params->pic_flags.bits.frame_type;
      cmd->dw1.clamping_flag = pic_params->pic_flags.bits.clamping_type;
      cmd->dw2.y_dc_q_mul_factor_segment0 =
	mul_factor / quant_dc_vp8_g75[y_quanta_dc_idx];
      cmd->dw2.y_ac_q_mul_factor_segment0 =
	mul_factor / quant_ac_vp8_g75[y_quanta_ac_idx];

      cmd->dw3.y2_dc_q_mul_factor_segment0 =
	mul_factor / quant_dc2_vp8_g75[y2_quanta_dc_idx];
      cmd->dw3.y2_ac_q_mul_factor_segment0 =
	mul_factor / quant_ac2_vp8_g75[y2_quanta_ac_idx];

      cmd->dw4.uv_dc_q_mul_factor_segment0 =
	mul_factor / quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
      cmd->dw4.uv_ac_q_mul_factor_segment0 =
	mul_factor / quant_ac_vp8_g75[uv_quanta_ac_idx];

      cmd->dw5.y_dc_inv_q_mul_factor_segment0 =
	quant_dc_vp8_g75[y_quanta_dc_idx];
      cmd->dw5.y_ac_inv_q_mul_factor_segment0 =
	quant_ac_vp8_g75[y_quanta_ac_idx];

      cmd->dw6.y2_dc_inv_q_mul_factor_segment0 =
	quant_dc2_vp8_g75[y2_quanta_dc_idx];
      cmd->dw6.y2_ac_inv_q_mul_factor_segment0 =
	quant_ac2_vp8_g75[y2_quanta_ac_idx];

      cmd->dw7.uv_dc_inv_q_mul_factor_segment0 =
	quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
      cmd->dw7.uv_ac_inv_q_mul_factor_segment0 =
	quant_ac_vp8_g75[uv_quanta_ac_idx];

      cmd->dw8.y2_dc_q_shift_factor_segment0 = shift_factor;
      cmd->dw8.y2_ac_q_shift_factor_segment0 = shift_factor;
      cmd->dw8.y_dc_q_shift_factor_segment0 = shift_factor;
      cmd->dw8.y_ac_q_shift_factor_segment0 = shift_factor;

      cmd->dw9.uv_dc_q_shift_factor_segment0 = shift_factor;
      cmd->dw9.uv_ac_q_shift_factor_segment0 = shift_factor;
      if (pic_params->pic_flags.bits.segmentation_enabled)
	{
	  y_quanta_ac_idx = quant_params->quantization_index[1];
	  y_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y1_DC_VP8];
	  uv_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_UV_DC_VP8];
	  uv_quanta_ac_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_UV_AC_VP8];
	  y2_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y2_DC_VP8];
	  y2_quanta_ac_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y2_AC_VP8];

	  cmd->dw10.y_dc_q_mul_factor_segment1 =
	    mul_factor / quant_dc_vp8_g75[y_quanta_dc_idx];
	  cmd->dw10.y_ac_q_mul_factor_segment1 =
	    mul_factor / quant_ac_vp8_g75[y_quanta_ac_idx];

	  cmd->dw11.y2_dc_q_mul_factor_segment1 =
	    mul_factor / quant_dc2_vp8_g75[y2_quanta_dc_idx];
	  cmd->dw11.y2_ac_q_mul_factor_segment1 =
	    mul_factor / quant_ac2_vp8_g75[y2_quanta_ac_idx];

	  cmd->dw12.uv_dc_q_mul_factor_segment1 =
	    mul_factor / quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
	  cmd->dw12.uv_ac_q_mul_factor_segment1 =
	    mul_factor / quant_ac_vp8_g75[uv_quanta_ac_idx];

	  cmd->dw13.y_dc_inv_q_mul_factor_segment1 =
	    quant_dc_vp8_g75[y_quanta_dc_idx];
	  cmd->dw13.y_ac_inv_q_mul_factor_segment1 =
	    quant_ac_vp8_g75[y_quanta_ac_idx];

	  cmd->dw14.y2_dc_inv_q_mul_factor_segment1 =
	    quant_dc2_vp8_g75[y2_quanta_dc_idx];
	  cmd->dw14.y2_ac_inv_q_mul_factor_segment1 =
	    quant_ac2_vp8_g75[y2_quanta_ac_idx];

	  cmd->dw15.uv_dc_inv_q_mul_factor_segment1 =
	    quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
	  cmd->dw15.uv_ac_inv_q_mul_factor_segment1 =
	    quant_ac_vp8_g75[uv_quanta_ac_idx];

	  cmd->dw16.y2_dc_q_shift_factor_segment1 = shift_factor;
	  cmd->dw16.y2_ac_q_shift_factor_segment1 = shift_factor;
	  cmd->dw16.y_dc_q_shift_factor_segment1 = shift_factor;
	  cmd->dw16.y_ac_q_shift_factor_segment1 = shift_factor;

	  cmd->dw17.uv_dc_q_shift_factor_segment1 = shift_factor;
	  cmd->dw17.uv_ac_q_shift_factor_segment1 = shift_factor;

	  y_quanta_ac_idx = quant_params->quantization_index[2];
	  y_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y1_DC_VP8];
	  uv_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_UV_DC_VP8];
	  uv_quanta_ac_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_UV_AC_VP8];
	  y2_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y2_DC_VP8];
	  y2_quanta_ac_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y2_AC_VP8];

	  cmd->dw18.y_dc_q_mul_factor_segment2 =
	    mul_factor / quant_dc_vp8_g75[y_quanta_dc_idx];
	  cmd->dw18.y_ac_q_mul_factor_segment2 =
	    mul_factor / quant_ac_vp8_g75[y_quanta_ac_idx];

	  cmd->dw19.y2_dc_q_mul_factor_segment2 =
	    mul_factor / quant_dc2_vp8_g75[y2_quanta_dc_idx];
	  cmd->dw19.y2_ac_q_mul_factor_segment2 =
	    mul_factor / quant_ac2_vp8_g75[y2_quanta_ac_idx];

	  cmd->dw20.uv_dc_q_mul_factor_segment2 =
	    mul_factor / quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
	  cmd->dw20.uv_ac_q_mul_factor_segment2 =
	    mul_factor / quant_ac_vp8_g75[uv_quanta_ac_idx];

	  cmd->dw21.y_dc_inv_q_mul_factor_segment2 =
	    quant_dc_vp8_g75[y_quanta_dc_idx];
	  cmd->dw21.y_ac_inv_q_mul_factor_segment2 =
	    quant_ac_vp8_g75[y_quanta_ac_idx];

	  cmd->dw22.y2_dc_inv_q_mul_factor_segment2 =
	    quant_dc2_vp8_g75[y2_quanta_dc_idx];
	  cmd->dw22.y2_ac_inv_q_mul_factor_segment2 =
	    quant_ac2_vp8_g75[y2_quanta_ac_idx];

	  cmd->dw23.uv_dc_inv_q_mul_factor_segment2 =
	    quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
	  cmd->dw23.uv_ac_inv_q_mul_factor_segment2 =
	    quant_ac_vp8_g75[uv_quanta_ac_idx];

	  cmd->dw24.y2_dc_q_shift_factor_segment2 = shift_factor;
	  cmd->dw24.y2_ac_q_shift_factor_segment2 = shift_factor;
	  cmd->dw24.y_dc_q_shift_factor_segment2 = shift_factor;
	  cmd->dw24.y_ac_q_shift_factor_segment2 = shift_factor;

	  cmd->dw25.uv_dc_q_shift_factor_segment2 = shift_factor;
	  cmd->dw25.uv_ac_q_shift_factor_segment2 = shift_factor;

	  y_quanta_ac_idx = quant_params->quantization_index[3];
	  y_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y1_DC_VP8];
	  uv_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_UV_DC_VP8];
	  uv_quanta_ac_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_UV_AC_VP8];
	  y2_quanta_dc_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y2_DC_VP8];
	  y2_quanta_ac_idx =
	    y_quanta_ac_idx +
	    quant_params->quantization_index_delta[QUAND_INDEX_Y2_AC_VP8];

	  cmd->dw26.y_dc_q_mul_factor_segment3 =
	    mul_factor / quant_dc_vp8_g75[y_quanta_dc_idx];
	  cmd->dw26.y_ac_q_mul_factor_segment3 =
	    mul_factor / quant_ac_vp8_g75[y_quanta_ac_idx];

	  cmd->dw27.y2_dc_q_mul_factor_segment3 =
	    mul_factor / quant_dc2_vp8_g75[y2_quanta_dc_idx];
	  cmd->dw27.y2_ac_q_mul_factor_segment3 =
	    mul_factor / quant_ac2_vp8_g75[y2_quanta_ac_idx];

	  cmd->dw28.uv_dc_q_mul_factor_segment3 =
	    mul_factor / quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
	  cmd->dw28.uv_ac_q_mul_factor_segment3 =
	    mul_factor / quant_ac_vp8_g75[uv_quanta_ac_idx];

	  cmd->dw29.y_dc_inv_q_mul_factor_segment3 =
	    quant_dc_vp8_g75[y_quanta_dc_idx];
	  cmd->dw29.y_ac_inv_q_mul_factor_segment3 =
	    quant_ac_vp8_g75[y_quanta_ac_idx];

	  cmd->dw30.y2_dc_inv_q_mul_factor_segment3 =
	    quant_dc2_vp8_g75[y2_quanta_dc_idx];
	  cmd->dw30.y2_ac_inv_q_mul_factor_segment3 =
	    quant_ac2_vp8_g75[y2_quanta_ac_idx];

	  cmd->dw31.uv_dc_inv_q_mul_factor_segment3 =
	    quant_dc_uv_vp8_g75[uv_quanta_dc_idx];
	  cmd->dw31.uv_ac_inv_q_mul_factor_segment3 =
	    quant_ac_vp8_g75[uv_quanta_ac_idx];

	  cmd->dw32.y2_dc_q_shift_factor_segment3 = shift_factor;
	  cmd->dw32.y2_ac_q_shift_factor_segment3 = shift_factor;
	  cmd->dw32.y_dc_q_shift_factor_segment3 = shift_factor;
	  cmd->dw32.y_ac_q_shift_factor_segment3 = shift_factor;

	  cmd->dw33.uv_dc_q_shift_factor_segment3 = shift_factor;
	  cmd->dw33.uv_ac_q_shift_factor_segment3 = shift_factor;
	}

      cmd->dw34.ref_frame_lf_delta0 = pic_params->ref_lf_delta[0];	/*referenceframe loopfilter delta for alt ref frame */
      cmd->dw34.ref_frame_lf_delta1 = pic_params->ref_lf_delta[1];	/*referenceframe loopfilter delta for golden frame */
      cmd->dw34.ref_frame_lf_delta2 = pic_params->ref_lf_delta[2];	/*referenceframe loopfilter delta for last frame */
      cmd->dw34.ref_frame_lf_delta3 = pic_params->ref_lf_delta[3];	/*referenceframe loopfilter delta for intra frame */

      cmd->dw35.mode_lf_delta0 = pic_params->mode_lf_delta[0];
      cmd->dw35.mode_lf_delta1 = pic_params->mode_lf_delta[1];
      cmd->dw35.mode_lf_delta2 = pic_params->mode_lf_delta[2];
      cmd->dw35.mode_lf_delta3 = pic_params->mode_lf_delta[3];

      cmd->dw36.lf_level0 = pic_params->loop_filter_level[0];
      if (pic_params->pic_flags.bits.segmentation_enabled)
	{
	  cmd->dw36.lf_level1 = pic_params->loop_filter_level[1];
	  cmd->dw36.lf_level2 = pic_params->loop_filter_level[2];
	  cmd->dw36.lf_level3 = pic_params->loop_filter_level[3];
	}
      cmd->dw40.pak_per_mb_out_data_surf_bti = VP8_MBPAK_PER_MB_OUT_G75;
      cmd->dw41.mb_enc_curr_y_bti = VP8_MBPAK_CURR_Y_G75;
      cmd->dw42.pak_recon_y_bti = VP8_MBPAK_CURR_RECON_Y_G75;
      cmd->dw43.pak_row_buffer_y_bti = VP8_MBPAK_ROW_BUFF_Y_G75;
      cmd->dw44.pak_row_buffer_uv_bti = VP8_MBPAK_ROW_BUFF_UV_G75;
      cmd->dw45.pak_col_buffer_y_bti = VP8_MBPAK_COL_BUFF_Y_G75;
      cmd->dw46.pak_col_buffer_uv_bti = VP8_MBPAK_COL_BUFF_UV_G75;
      cmd->dw47.pak_kernel_debug_bti = VP8_MBPAK_DEBUG_STREAMOUT_G75;
    }
}

VOID
media_surface_state_vp8_mbenc_g7 (MEDIA_ENCODER_CTX * encoder_context,
				  struct encode_state *encode_state,
				  MBENC_SURFACE_PARAMS_VP8 *
				  mbenc_sutface_params)
{
  MBENC_CONTEXT *mbenc_ctx = &encoder_context->mbenc_context;
  MEDIA_GPE_CTX *mbenc_gpe_ctx = &mbenc_ctx->gpe_context;
  BRC_INIT_RESET_CONTEXT *brc_init_reset_ctx = &encoder_context->brc_init_reset_context;
  //ME_CONTEXT *me_ctx = &encoder_context->me_context;
  #if 0
  UINT kernel_dump_offset = 0;
  #endif
  SURFACE_SET_PARAMS params;
  struct object_surface *obj_surface;
  //struct object_buffer *obj_buffer;
  BYTE *binding_surface_state_buf = NULL;
  MEDIA_RESOURCE surface_2d;
  //MEDIA_RESOURCE *obj_buffer_res;
  binding_surface_state_buf =
    (BYTE *) media_map_buffer_obj (mbenc_gpe_ctx->
				   surface_state_binding_table.res.bo);
  //media_drv_memset(binding_surface_state_buf,mbenc_gpe_ctx->surface_state_binding_table.res.bo->size);
  //coded data buffer
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    mbenc_gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.binding_table_offset = BINDING_TABLE_OFFSET (0);
  params.surface_state_offset = SURFACE_STATE_OFFSET (0);
  obj_surface = encode_state->coded_buf_surface;
  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
  params.buf_object = surface_2d;
  //params.surface_is_raw = 1;
  params.offset = encoder_context->mb_data_offset;
  params.size = encoder_context->mb_data_in_bytes;
  params.cacheability_control = mbenc_sutface_params->cacheability_control;
  encoder_context->media_add_surface_state (&params);

//current pic luma
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    mbenc_gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.surface_is_2d = 1;
  params.media_block_raw = 1;
  params.vert_line_stride_offset = 0;
  params.vert_line_stride = 0;
  params.format = STATE_SURFACEFORMAT_R8_UNORM;
  params.binding_table_offset = BINDING_TABLE_OFFSET (1);
  params.surface_state_offset = SURFACE_STATE_OFFSET (1);
  obj_surface = encode_state->input_yuv_object;
  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
  params.surface_2d = &surface_2d;
  params.surface_2d->surface_array_spacing = 1;
  params.cacheability_control = mbenc_sutface_params->cacheability_control;
  encoder_context->media_add_surface_state (&params);

//current pic uv
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    mbenc_gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.surface_is_uv_2d = 1;
  params.media_block_raw = 1;
  params.vert_line_stride_offset = 0;
  params.vert_line_stride = 0;
  params.format = STATE_SURFACEFORMAT_R8_UNORM;
  params.binding_table_offset = BINDING_TABLE_OFFSET (2);
  params.surface_state_offset = SURFACE_STATE_OFFSET (2);
  obj_surface = encode_state->input_yuv_object;
  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
  params.surface_2d = &surface_2d;
  params.surface_2d->surface_array_spacing = 1;
  params.cacheability_control = mbenc_sutface_params->cacheability_control;
  encoder_context->media_add_surface_state (&params);


  if (mbenc_sutface_params->pic_coding == FRAME_TYPE_I)
    {
      //current pic vme
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbenc_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.advance_state = 1;
      params.format = STATE_SURFACEFORMAT_R8_UNORM;
      params.binding_table_offset = BINDING_TABLE_OFFSET (9);
      params.surface_state_offset = SURFACE_STATE_OFFSET (9);
      obj_surface = encode_state->input_yuv_object;
      OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
      params.surface_2d = &surface_2d;
      params.uv_direction = VDIRECTION_FULL_FRAME;
      params.cacheability_control =
	mbenc_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);

      //MBMode Cost Luma surface
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbenc_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.surface_is_2d = 1;
      params.format = STATE_SURFACEFORMAT_R8_UNORM;
      params.binding_table_offset = BINDING_TABLE_OFFSET (3);
      params.surface_state_offset = SURFACE_STATE_OFFSET (3);
      params.surface_2d = &mbenc_ctx->mb_mode_cost_luma_buffer;
      params.cacheability_control =
	mbenc_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);

      //Block Mode cost surface  
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbenc_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.surface_is_2d = 1;
      params.format = STATE_SURFACEFORMAT_R8_UNORM;
      params.binding_table_offset = BINDING_TABLE_OFFSET (4);
      params.surface_state_offset = SURFACE_STATE_OFFSET (4);
      params.surface_2d = &mbenc_ctx->block_mode_cost_buffer;
      params.cacheability_control =
	mbenc_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);

      //Chroma Reconstruction Surface
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbenc_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.surface_is_2d = 1;
      params.media_block_raw = 1;
      params.format = STATE_SURFACEFORMAT_R8_UNORM;
      params.binding_table_offset = BINDING_TABLE_OFFSET (5);
      params.surface_state_offset = SURFACE_STATE_OFFSET (5);
      params.surface_2d = &mbenc_ctx->chroma_reconst_buffer;
      params.cacheability_control =
	mbenc_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);
      //histogram
#if 0
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbenc_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.binding_table_offset = BINDING_TABLE_OFFSET (7);
      params.surface_state_offset = SURFACE_STATE_OFFSET (7);
      params.buf_object = mbenc_ctx->histogram_buffer;
      params.surface_is_raw = 1;
      params.size = mbenc_ctx->histogram_buffer.bo_size;
      params.cacheability_control =
	mbenc_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);
#endif
     #if 0
      kernel_dump_offset = 8;
     #endif

      if (mbenc_sutface_params->iframe_dist_in_use) {
	MEDIA_ENCODER_VP8_SURFACE *vp8_surface;

	params = surface_set_params_init;
	params.binding_surface_state.bo =
	  mbenc_gpe_ctx->surface_state_binding_table.res.bo;

	params.binding_surface_state.buf = binding_surface_state_buf;
	params.surface_is_2d = 1;
	params.media_block_raw = 1; /* media block read/write message */
	params.vert_line_stride_offset = 0;
	params.vert_line_stride = 0;
	params.format = STATE_SURFACEFORMAT_R8_UNORM;
	params.binding_table_offset = BINDING_TABLE_OFFSET (12);
	params.surface_state_offset = SURFACE_STATE_OFFSET (12);
	params.surface_2d = &brc_init_reset_ctx->brc_distortion;
	media_add_surface_state (&params);

	vp8_surface = encode_state->reconstructed_object->private_data;

	if (vp8_surface && vp8_surface->scaled_4x_surface_obj) {
	  params = surface_set_params_init;
	  params.binding_surface_state.bo =
	    mbenc_gpe_ctx->surface_state_binding_table.res.bo;
	  params.binding_surface_state.buf = binding_surface_state_buf;
	  params.surface_is_2d = 1;
	  params.vert_line_stride_offset = 0;
	  params.vert_line_stride = 0;
	  params.format = STATE_SURFACEFORMAT_R8_UNORM;
	  params.binding_table_offset = BINDING_TABLE_OFFSET (11);
	  params.surface_state_offset = SURFACE_STATE_OFFSET (11);
	  obj_surface = vp8_surface->scaled_4x_surface_obj;
	  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
	  params.surface_2d = &surface_2d;
	  params.cacheability_control = mbenc_sutface_params->cacheability_control;
	  media_add_surface_state (&params);

	  params = surface_set_params_init;
	  params.binding_surface_state.bo =
	    mbenc_gpe_ctx->surface_state_binding_table.res.bo;
	  params.binding_surface_state.buf = binding_surface_state_buf;
	  params.advance_state = 1;
	  params.format = STATE_SURFACEFORMAT_R8_UNORM;
	  params.binding_table_offset = BINDING_TABLE_OFFSET (10);
	  params.surface_state_offset = SURFACE_STATE_OFFSET (10);
	  obj_surface = vp8_surface->scaled_4x_surface_obj;
	  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
	  params.surface_2d = &surface_2d;
	  params.uv_direction = VDIRECTION_FULL_FRAME;
	  params.cacheability_control = mbenc_sutface_params->cacheability_control;
	  media_add_surface_state (&params);
	}
      }
    }
  else
    {
      //MV Data surface
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbenc_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.binding_table_offset = BINDING_TABLE_OFFSET (4);
      params.surface_state_offset = SURFACE_STATE_OFFSET (4);
      obj_surface = encode_state->coded_buf_surface;
      OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
      params.buf_object = surface_2d;
      params.media_block_raw = 1;
      //params.surface_is_raw = 1;
      params.offset = encoder_context->mv_offset;
      params.size = encoder_context->mv_in_bytes;

      params.cacheability_control =
	mbenc_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);
      if (mbenc_sutface_params->hme_enabled)
	{
	  /*need to add me mv data buffer surface states here later */

	}
      //current picture VME inter prediction surface..!i
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbenc_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.advance_state = 1;
      params.format = STATE_SURFACEFORMAT_R8_UNORM;
      params.binding_table_offset = BINDING_TABLE_OFFSET (10);
      params.surface_state_offset = SURFACE_STATE_OFFSET (10);
      obj_surface = encode_state->input_yuv_object;
      OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
      params.surface_2d = &surface_2d;
      params.uv_direction = VDIRECTION_FULL_FRAME;
      params.cacheability_control =
	mbenc_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);

      //current picture VME inter prediction surface..!i
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbenc_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.advance_state = 1;
      params.format = STATE_SURFACEFORMAT_R8_UNORM;
      params.binding_table_offset = BINDING_TABLE_OFFSET (12);
      params.surface_state_offset = SURFACE_STATE_OFFSET (12);
      obj_surface = encode_state->input_yuv_object;
      OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
      params.surface_2d = &surface_2d;
      params.uv_direction = VDIRECTION_FULL_FRAME;
      params.cacheability_control =
	mbenc_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);

      //current picture VME inter prediction surface..!i
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbenc_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.advance_state = 1;
      params.format = STATE_SURFACEFORMAT_R8_UNORM;
      params.binding_table_offset = BINDING_TABLE_OFFSET (14);
      params.surface_state_offset = SURFACE_STATE_OFFSET (14);
      obj_surface = encode_state->input_yuv_object;
      OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
      params.surface_2d = &surface_2d;
      params.uv_direction = VDIRECTION_FULL_FRAME;
      params.cacheability_control =
	mbenc_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);

      //last ref
      if (encode_state->ref_last_frame != NULL
	  && encode_state->ref_last_frame->bo != NULL)
	{
	  params = surface_set_params_init;
	  params.binding_surface_state.bo =
	    mbenc_gpe_ctx->surface_state_binding_table.res.bo;
	  params.binding_surface_state.buf = binding_surface_state_buf;
	  params.advance_state = 1;
	  params.format = STATE_SURFACEFORMAT_R8_UNORM;
	  params.binding_table_offset = BINDING_TABLE_OFFSET (11);
	  params.surface_state_offset = SURFACE_STATE_OFFSET (11);
	  obj_surface = encode_state->ref_last_frame;
	  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
	  params.surface_2d = &surface_2d;
	  params.uv_direction = VDIRECTION_FULL_FRAME;
	  params.cacheability_control =
	    mbenc_sutface_params->cacheability_control;
	  encoder_context->media_add_surface_state (&params);
	}

      //goldeb ref
      if (encode_state->ref_gf_frame != NULL
	  && encode_state->ref_gf_frame->bo != NULL)
	{
	  params = surface_set_params_init;
	  params.binding_surface_state.bo =
	    mbenc_gpe_ctx->surface_state_binding_table.res.bo;
	  params.binding_surface_state.buf = binding_surface_state_buf;
	  params.advance_state = 1;
	  params.format = STATE_SURFACEFORMAT_R8_UNORM;
	  params.binding_table_offset = BINDING_TABLE_OFFSET (13);
	  params.surface_state_offset = SURFACE_STATE_OFFSET (13);
	  obj_surface = encode_state->ref_gf_frame;
	  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
	  params.surface_2d = &surface_2d;
	  params.uv_direction = VDIRECTION_FULL_FRAME;
	  params.cacheability_control =
	    mbenc_sutface_params->cacheability_control;
	  encoder_context->media_add_surface_state (&params);
	}

      //alternate ref
      if (encode_state->ref_arf_frame != NULL
	  && encode_state->ref_arf_frame->bo != NULL)
	{
	  params = surface_set_params_init;
	  params.binding_surface_state.bo =
	    mbenc_gpe_ctx->surface_state_binding_table.res.bo;
	  params.binding_surface_state.buf = binding_surface_state_buf;
	  params.advance_state = 1;
	  params.format = STATE_SURFACEFORMAT_R8_UNORM;
	  params.binding_table_offset = BINDING_TABLE_OFFSET (15);
	  params.surface_state_offset = SURFACE_STATE_OFFSET (15);
	  obj_surface = encode_state->ref_arf_frame;
	  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
	  params.surface_2d = &surface_2d;
	  params.uv_direction = VDIRECTION_FULL_FRAME;
	  params.cacheability_control =
	    mbenc_sutface_params->cacheability_control;
	  encoder_context->media_add_surface_state (&params);
	}
     #if 0
      kernel_dump_offset = 21;
     #endif
    }
#if 0
  if (mbenc_sutface_params->kernel_dump)
    {
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbenc_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.binding_table_offset = BINDING_TABLE_OFFSET (kernel_dump_offset);
      params.surface_state_offset = SURFACE_STATE_OFFSET (kernel_dump_offset);
      params.buf_object = mbenc_ctx->kernel_dump_buffer;
      //params.surface_is_raw = 1;
      params.cacheability_control =
	mbenc_sutface_params->cacheability_control;
      params.size =
	WIDTH_IN_MACROBLOCKS ((mbenc_sutface_params->orig_frame_width) *
			      HEIGHT_IN_MACROBLOCKS
			      (mbenc_sutface_params->orig_frame_height) *
			      /*MB_CODE_SIZE_VP8 */ 32);
      params.offset = encoder_context->mv_offset;
      encoder_context->media_add_surface_state (&params);
    }
#endif
  media_unmap_buffer_obj (mbenc_gpe_ctx->surface_state_binding_table.res.bo);
}

VOID
media_surface_state_vp8_mbpak_g7 (MEDIA_ENCODER_CTX * encoder_context,
				  struct encode_state *encode_state,
				  MBPAK_SURFACE_PARAMS_VP8 *
				  mbpak_sutface_params)
{
  MBPAK_CONTEXT *mbpak_ctx = &encoder_context->mbpak_context;
  MEDIA_GPE_CTX *mbpak_gpe_ctx = &mbpak_ctx->gpe_context;
  SURFACE_SET_PARAMS params;
  #if 0
  UINT kernel_dump_offset = 0;
  #endif
  struct object_surface *obj_surface;
  //struct object_buffer *obj_buffer;
  BYTE *binding_surface_state_buf = NULL;
  MEDIA_RESOURCE surface_2d;	//={0,0,0};

  if (mbpak_sutface_params->mbpak_phase_type == MBPAK_HYBRID_STATE_P2)
    mbpak_gpe_ctx = &mbpak_ctx->gpe_context2;

  binding_surface_state_buf =
    (BYTE *) media_map_buffer_obj (mbpak_gpe_ctx->
				   surface_state_binding_table.res.bo);
  //coded data buffer
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    mbpak_gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.binding_table_offset = BINDING_TABLE_OFFSET (0);
  params.surface_state_offset = SURFACE_STATE_OFFSET (0);
  obj_surface = encode_state->coded_buf_surface;
  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
  params.buf_object = surface_2d;
  //params.surface_is_raw = 1;
  params.offset = encoder_context->mb_data_offset;
  params.cacheability_control = mbpak_sutface_params->cacheability_control;
  params.size = encoder_context->mb_data_in_bytes;
  encoder_context->media_add_surface_state (&params);

  //current pic luma
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    mbpak_gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.surface_is_2d = 1;
  params.media_block_raw = 0;
  params.vert_line_stride_offset = 0;
  params.vert_line_stride = 0;
  params.format = STATE_SURFACEFORMAT_R8_UNORM;
  params.binding_table_offset = BINDING_TABLE_OFFSET (1);
  params.surface_state_offset = SURFACE_STATE_OFFSET (1);
  obj_surface = encode_state->input_yuv_object;
  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
  params.surface_2d = &surface_2d;
  params.surface_2d->surface_array_spacing = 1;
  params.cacheability_control = mbpak_sutface_params->cacheability_control;
  encoder_context->media_add_surface_state (&params);

//current pic uv
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    mbpak_gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.surface_is_uv_2d = 1;
  params.media_block_raw = 0;
  params.vert_line_stride_offset = 0;
  params.vert_line_stride = 0;
  params.format = STATE_SURFACEFORMAT_R8_UNORM;
  params.binding_table_offset = BINDING_TABLE_OFFSET (2);
  params.surface_state_offset = SURFACE_STATE_OFFSET (2);
  obj_surface = encode_state->input_yuv_object;
  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
  params.surface_2d = &surface_2d;
  params.surface_2d->surface_array_spacing = 1;
  params.cacheability_control = mbpak_sutface_params->cacheability_control;
  encoder_context->media_add_surface_state (&params);

  //current reconstructed picture luma
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    mbpak_gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.surface_is_2d = 1;
  params.media_block_raw = 0;
  params.vert_line_stride_offset = 0;
  params.vert_line_stride = 0;
  params.format = STATE_SURFACEFORMAT_R8_UNORM;
  params.binding_table_offset = BINDING_TABLE_OFFSET (3);
  params.surface_state_offset = SURFACE_STATE_OFFSET (3);
  obj_surface = encode_state->reconstructed_object;
  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
  params.surface_2d = &surface_2d;
  params.surface_2d->surface_array_spacing = 1;
  params.cacheability_control = mbpak_sutface_params->cacheability_control;
  encoder_context->media_add_surface_state (&params);

// current reconstructed picture uv
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    mbpak_gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.surface_is_uv_2d = 1;
  params.media_block_raw = 0;
  params.vert_line_stride_offset = 0;
  params.vert_line_stride = 0;
  params.format = STATE_SURFACEFORMAT_R8_UNORM;
  params.binding_table_offset = BINDING_TABLE_OFFSET (4);
  params.surface_state_offset = SURFACE_STATE_OFFSET (4);
  obj_surface = encode_state->reconstructed_object;
  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
  params.surface_2d = &surface_2d;

  params.surface_2d->surface_array_spacing = 1;
  params.cacheability_control = mbpak_sutface_params->cacheability_control;
  encoder_context->media_add_surface_state (&params);

  if (mbpak_sutface_params->mbpak_phase_type == MBPAK_HYBRID_STATE_P1)
    {
      //MV Data surface 
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbpak_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.binding_table_offset = BINDING_TABLE_OFFSET (11);
      params.surface_state_offset = SURFACE_STATE_OFFSET (11);
      obj_surface = encode_state->coded_buf_surface;
      OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
      params.buf_object = surface_2d;
      //params.surface_is_raw = 1;
      params.offset = encoder_context->mv_offset;
      params.size = encoder_context->mv_in_bytes;
      params.cacheability_control =
	mbpak_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);

      //last ref
      if (encode_state->ref_last_frame != NULL
	  && encode_state->ref_last_frame->bo != NULL)
	{
	  params = surface_set_params_init;
	  params.binding_surface_state.bo =
	    mbpak_gpe_ctx->surface_state_binding_table.res.bo;
	  params.binding_surface_state.buf = binding_surface_state_buf;
	  params.surface_is_2d = 1;
	  params.media_block_raw = 1;

	  //params.advance_state = 1;
	  params.format = STATE_SURFACEFORMAT_R8_UNORM;
	  params.binding_table_offset = BINDING_TABLE_OFFSET (5);
	  params.surface_state_offset = SURFACE_STATE_OFFSET (5);
	  obj_surface = encode_state->ref_last_frame;
	  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
	  params.surface_2d = &surface_2d;
	  params.cacheability_control =
	    mbpak_sutface_params->cacheability_control;
	  params.surface_2d->surface_array_spacing = 1;
	  encoder_context->media_add_surface_state (&params);

	  params = surface_set_params_init;
	  params.binding_surface_state.bo =
	    mbpak_gpe_ctx->surface_state_binding_table.res.bo;
	  params.binding_surface_state.buf = binding_surface_state_buf;
	  params.surface_is_uv_2d = 1;
	  params.media_block_raw = 1;
	  //params.advance_state = 1;
	  params.format = STATE_SURFACEFORMAT_R8_UNORM;
	  params.binding_table_offset = BINDING_TABLE_OFFSET (6);
	  params.surface_state_offset = SURFACE_STATE_OFFSET (6);
	  obj_surface = encode_state->ref_last_frame;
	  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
	  params.surface_2d = &surface_2d;
	  params.surface_2d->surface_array_spacing = 1;
	  params.cacheability_control =
	    mbpak_sutface_params->cacheability_control;
	  encoder_context->media_add_surface_state (&params);
	}
      //goldeb ref
      if (encode_state->ref_gf_frame != NULL
	  && encode_state->ref_gf_frame->bo != NULL)
	{
	  params = surface_set_params_init;
	  params.binding_surface_state.bo =
	    mbpak_gpe_ctx->surface_state_binding_table.res.bo;
	  params.binding_surface_state.buf = binding_surface_state_buf;
	  //params.advance_state = 1;
	  params.surface_is_2d = 1;
	  params.media_block_raw = 1;
	  params.format = STATE_SURFACEFORMAT_R8_UNORM;
	  params.binding_table_offset = BINDING_TABLE_OFFSET (7);
	  params.surface_state_offset = SURFACE_STATE_OFFSET (7);
	  obj_surface = encode_state->ref_gf_frame;
	  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
	  params.surface_2d = &surface_2d;
	  params.surface_2d->surface_array_spacing = 1;
	  params.cacheability_control =
	    mbpak_sutface_params->cacheability_control;
	  encoder_context->media_add_surface_state (&params);

	  params = surface_set_params_init;
	  params.binding_surface_state.bo =
	    mbpak_gpe_ctx->surface_state_binding_table.res.bo;
	  params.binding_surface_state.buf = binding_surface_state_buf;
	  //params.advance_state = 1;
	  params.surface_is_uv_2d = 1;
	  params.media_block_raw = 1;
	  params.format = STATE_SURFACEFORMAT_R8_UNORM;
	  params.binding_table_offset = BINDING_TABLE_OFFSET (8);
	  params.surface_state_offset = SURFACE_STATE_OFFSET (8);
	  obj_surface = encode_state->ref_gf_frame;
	  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
	  params.surface_2d = &surface_2d;
	  params.cacheability_control =
	    mbpak_sutface_params->cacheability_control;
	  params.surface_2d->surface_array_spacing = 1;
	  encoder_context->media_add_surface_state (&params);

	}
      //alterbate ref
      if (encode_state->ref_arf_frame != NULL
	  && encode_state->ref_arf_frame->bo != NULL)
	{
	  params = surface_set_params_init;
	  params.binding_surface_state.bo =
	    mbpak_gpe_ctx->surface_state_binding_table.res.bo;
	  params.binding_surface_state.buf = binding_surface_state_buf;
	  //params.advance_state = 1;
	  params.surface_is_2d = 1;
	  params.media_block_raw = 1;
	  params.format = STATE_SURFACEFORMAT_R8_UNORM;
	  params.binding_table_offset = BINDING_TABLE_OFFSET (9);
	  params.surface_state_offset = SURFACE_STATE_OFFSET (9);
	  obj_surface = encode_state->ref_arf_frame;
	  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
	  params.surface_2d = &surface_2d;
	  params.cacheability_control =
	    mbpak_sutface_params->cacheability_control;
	  params.surface_2d->surface_array_spacing = 1;
	  encoder_context->media_add_surface_state (&params);
	  params = surface_set_params_init;
	  params.binding_surface_state.bo =
	    mbpak_gpe_ctx->surface_state_binding_table.res.bo;
	  params.binding_surface_state.buf = binding_surface_state_buf;
	  //params.advance_state = 1;
	  params.surface_is_uv_2d = 1;
	  params.media_block_raw = 1;
	  params.format = STATE_SURFACEFORMAT_R8_UNORM;
	  params.binding_table_offset = BINDING_TABLE_OFFSET (10);
	  params.surface_state_offset = SURFACE_STATE_OFFSET (10);
	  obj_surface = encode_state->ref_arf_frame;
	  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
	  params.surface_2d = &surface_2d;
	  params.surface_2d->surface_array_spacing = 1;
	  params.cacheability_control =
	    mbpak_sutface_params->cacheability_control;
	  encoder_context->media_add_surface_state (&params);

	}
      #if 0
      kernel_dump_offset = 12;
      #endif
    }
  else
    {
      //row buffer y
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbpak_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.binding_table_offset = BINDING_TABLE_OFFSET (5);
      params.surface_state_offset = SURFACE_STATE_OFFSET (5);
      params.buf_object = mbpak_ctx->row_buffer_y;
      //params.surface_is_raw = 1;
      params.size = mbpak_ctx->row_buffer_y.bo_size;
      params.cacheability_control =
	mbpak_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);

      //row buffer uv
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbpak_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.binding_table_offset = BINDING_TABLE_OFFSET (6);
      params.surface_state_offset = SURFACE_STATE_OFFSET (6);
      params.buf_object = mbpak_ctx->row_buffer_uv;
      //params.surface_is_raw = 1;
      params.size = mbpak_ctx->row_buffer_uv.bo_size;
      params.cacheability_control =
	mbpak_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);

      //column buffer .y
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbpak_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.binding_table_offset = BINDING_TABLE_OFFSET (7);
      params.surface_state_offset = SURFACE_STATE_OFFSET (7);
      params.buf_object = mbpak_ctx->column_buffer_y;
      //params.surface_is_raw = 1;
      params.cacheability_control =
	mbpak_sutface_params->cacheability_control;
      params.size = mbpak_ctx->column_buffer_y.bo_size;
      encoder_context->media_add_surface_state (&params);

      //column buffer uv
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbpak_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.binding_table_offset = BINDING_TABLE_OFFSET (8);
      params.surface_state_offset = SURFACE_STATE_OFFSET (8);
      params.buf_object = mbpak_ctx->column_buffer_uv;
      //params.surface_is_raw = 1;
      params.size = mbpak_ctx->column_buffer_uv.bo_size;
      params.cacheability_control =
	mbpak_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);
      #if 0
      kernel_dump_offset = 12;
      #endif
}
#if 0
  if (mbpak_sutface_params->kernel_dump)
    {
      params = surface_set_params_init;
      params.binding_surface_state.bo =
	mbpak_gpe_ctx->surface_state_binding_table.res.bo;
      params.binding_surface_state.buf = binding_surface_state_buf;
      params.binding_table_offset = BINDING_TABLE_OFFSET (kernel_dump_offset);
      params.surface_state_offset = SURFACE_STATE_OFFSET (kernel_dump_offset);
      //FIXME:need to pass right buffer here..!
      params.buf_object = mbpak_sutface_params->kernel_dump_buffer;	//mbpak_ctx->kernel_dump_buffer;
      //params.surface_is_raw = 1;
      params.size =
	WIDTH_IN_MACROBLOCKS (mbpak_sutface_params->orig_frame_width) *
	HEIGHT_IN_MACROBLOCKS (mbpak_sutface_params->orig_frame_height) * 32;
      params.cacheability_control =
	mbpak_sutface_params->cacheability_control;
      encoder_context->media_add_surface_state (&params);
    }
#endif
  media_unmap_buffer_obj (mbpak_gpe_ctx->surface_state_binding_table.res.bo);
}

VOID
media_set_curbe_vp8_brc_init_reset_g7(struct encode_state *encode_state,
				      MEDIA_BRC_INIT_RESET_PARAMS_VP8 * params)
{
  DOUBLE input_bits_per_frame, bps_ratio;
  MEDIA_CURBE_DATA_BRC_INIT_RESET_G75 *cmd =
    (MEDIA_CURBE_DATA_BRC_INIT_RESET_G75 *)params->curbe_cmd_buff;

  media_drv_memset(cmd, sizeof(MEDIA_CURBE_DATA_BRC_INIT_RESET_G75));

  // profile & level max frame size
  cmd->dw0.profile_level_max_frame = params->frame_width * params->frame_height;

  // initial buffer fullness, buffer size, max and target bitrate
  cmd->dw1.init_buf_full_in_bits = params->init_vbv_buffer_fullness_in_bit;
  cmd->dw2.buf_size_in_bits = params->vbv_buffer_size_in_bit;
  cmd->dw3.average_bit_rate = (params->target_bit_rate + ENCODE_BRC_KBPS - 1) / ENCODE_BRC_KBPS * ENCODE_BRC_KBPS;
  cmd->dw4.max_bit_rate = (params->max_bit_rate + ENCODE_BRC_KBPS - 1) / ENCODE_BRC_KBPS * ENCODE_BRC_KBPS;
  cmd->dw5.min_bit_rate = 0;

  cmd->dw6.frame_rate_m = params->frame_rate;
  cmd->dw7.frame_rate_d = 1;

  // params->rate_control_mode = HB_BRC_CBR;
  if (params->rate_control_mode == HB_BRC_CBR) {
    // for cbr max bitrate is same as target
    cmd->dw4.max_bit_rate = cmd->dw3.average_bit_rate;
    cmd->dw8.brc_flag = 0x0010;
  } else if (params->rate_control_mode == HB_BRC_VBR) {
    if (cmd->dw4.max_bit_rate < cmd->dw3.average_bit_rate)
    {
      cmd->dw4.max_bit_rate = 2 * cmd->dw3.average_bit_rate;
    }

    cmd->dw8.brc_flag = 0x0020;
  }

  cmd->dw8.number_pframes_in_gop = params->gop_pic_size - 1;

  // set dynamic thresholds
  input_bits_per_frame = ((DOUBLE)(cmd->dw4.max_bit_rate) * (DOUBLE)(cmd->dw7.frame_rate_d) / (DOUBLE)(cmd->dw6.frame_rate_m));
  bps_ratio = input_bits_per_frame / ((DOUBLE)(cmd->dw2.buf_size_in_bits) / 30);
  bps_ratio = (bps_ratio < 0.1) ? 0.1 : (bps_ratio > 3.5) ? 3.5 : bps_ratio;

  cmd->dw9.frame_width = params->frame_width;
  cmd->dw9.constant_0 = 30;
  cmd->dw10.frame_height = params->frame_height;
  cmd->dw10.avbr_accuracy = 30;
  cmd->dw11.avbr_convergence = 150;
  cmd->dw11.min_qp = 1; // hardcoded to match
  cmd->dw12.max_qp = 106; // hardcoded to match
  cmd->dw12.level_qp = 60; // hardcoded to match kernel cmodel

  // dw13-dw14 default 100
  cmd->dw13.max_section_pct = 100; // hardcoded to match kernel cmodel
  cmd->dw13.under_shoot_cbr_pct = 100; // hardcoded to match kernel cmodel
  cmd->dw14.min_section_pct = 100; // hardcoded to match kernel cmodel
  cmd->dw14.vbr_bias_pct = 100; // hardcoded to match kernel cmodel

  cmd->dw15.instant_rate_threshold0_pframe = 30;
  cmd->dw15.instant_rate_threshold1_pframe = 50;
  cmd->dw15.instant_rate_threshold2_pframe = 70;
  cmd->dw15.instant_rate_threshold3_pframe = 120;

  cmd->dw16.constant_0 = 30;
  cmd->dw16.constant_1 = 50;
  cmd->dw16.constant_2 = 70;
  cmd->dw16.constant_3 = 120;

  cmd->dw17.instant_rate_threshold0_iframe = 30;
  cmd->dw17.instant_rate_threshold1_iframe = 50;
  cmd->dw17.instant_rate_threshold2_iframe = 90;
  cmd->dw17.instant_rate_threshold3_iframe = 115;
  cmd->dw18.deviation_threshold0_pframe = (UINT)(-50 * pow(0.9, bps_ratio));
  cmd->dw18.deviation_threshold1_pframe = (UINT)(-50 * pow(0.66, bps_ratio));
  cmd->dw18.deviation_threshold2_pframe = (UINT)(-50 * pow(0.46, bps_ratio));
  cmd->dw18.deviation_threshold3_pframe = (UINT)(-50 * pow(0.3, bps_ratio));
  cmd->dw19.deviation_threshold4_pframe = (UINT)(50 * pow(0.3, bps_ratio));
  cmd->dw19.deviation_threshold5_pframe = (UINT)(50 * pow(0.46, bps_ratio));
  cmd->dw19.deviation_threshold6_pframe = (UINT)(50 * pow(0.7, bps_ratio));
  cmd->dw19.deviation_threshold7_pframe = (UINT)(50 * pow(0.9, bps_ratio));
  cmd->dw20.deviation_threshold0_vbr = (UINT)(-50 * pow(0.9, bps_ratio));
  cmd->dw20.deviation_threshold1_vbr = (UINT)(-50 * pow(0.7, bps_ratio));
  cmd->dw20.deviation_threshold2_vbr = (UINT)(-50 * pow(0.5, bps_ratio));
  cmd->dw20.deviation_threshold3_vbr = (UINT)(-50 * pow(0.3, bps_ratio));
  cmd->dw21.deviation_threshold4_vbr = (UINT)(100 * pow(0.4, bps_ratio));
  cmd->dw21.deviation_threshold5_vbr = (UINT)(100 * pow(0.5, bps_ratio));
  cmd->dw21.deviation_threshold6_vbr = (UINT)(100 * pow(0.75, bps_ratio));
  cmd->dw21.deviation_threshold7_vbr = (UINT)(100 * pow(0.9, bps_ratio));
  cmd->dw22.deviation_threshold0_iframe = (UINT)(-50 * pow(0.8, bps_ratio));
  cmd->dw22.deviation_threshold1_iframe = (UINT)(-50 * pow(0.6, bps_ratio));
  cmd->dw22.deviation_threshold2_iframe = (UINT)(-50 * pow(0.34, bps_ratio));
  cmd->dw22.deviation_threshold3_iframe = (UINT)(-50 * pow(0.2, bps_ratio));
  cmd->dw23.deviation_threshold4_iframe = (UINT)(50 * pow(0.2, bps_ratio));
  cmd->dw23.deviation_threshold5_iframe = (UINT)(50 * pow(0.4, bps_ratio));
  cmd->dw23.deviation_threshold6_iframe = (UINT)(50 * pow(0.66, bps_ratio));
  cmd->dw23.deviation_threshold7_iframe = (UINT)(50 * pow(0.9, bps_ratio));

  cmd->dw24.initial_qp_iframe = 0;
  cmd->dw24.initial_qp_pframe = 0;

  if (!params->brc_initted) {
    *params->brc_init_current_target_buf_full_in_bits = cmd->dw1.init_buf_full_in_bits;
  }

  *params->brc_init_reset_buf_size_in_bits = cmd->dw2.buf_size_in_bits;
  *params->brc_init_reset_input_bits_per_frame = input_bits_per_frame;

  cmd->dw25.history_buffer_bti = VP8_BRC_INIT_RESET_HISTORY_G75;
  cmd->dw26.distortion_buffer_bti = VP8_BRC_INIT_RESET_DISTORTION_G75;
}

VOID
media_surface_state_vp8_brc_init_reset_g7(MEDIA_ENCODER_CTX * encoder_context,
					  struct encode_state *encode_state,
					  BRC_INIT_RESET_SURFACE_PARAMS_VP8 *surface_params)
{
  BRC_INIT_RESET_CONTEXT *ctx = &encoder_context->brc_init_reset_context;
  MEDIA_GPE_CTX *gpe_ctx = &ctx->gpe_context;
  SURFACE_SET_PARAMS params;
  BYTE *binding_surface_state_buf = NULL;

  binding_surface_state_buf =
    (BYTE *) media_map_buffer_obj (gpe_ctx->surface_state_binding_table.res.bo);

  /* history buffer */
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.binding_table_offset = BINDING_TABLE_OFFSET (0);
  params.surface_state_offset = SURFACE_STATE_OFFSET (0);
  params.buf_object = ctx->brc_history;
  params.size = ctx->brc_history.bo_size;
  params.cacheability_control =
    surface_params->cacheability_control;
  assert(params.size == 544);
  media_add_surface_state (&params);

  /* distortion buffer */
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.surface_is_2d = 1;
  params.media_block_raw = 1;
  params.vert_line_stride_offset = 0;
  params.vert_line_stride = 0;
  params.format = STATE_SURFACEFORMAT_R8_UNORM;
  params.binding_table_offset = BINDING_TABLE_OFFSET (1);
  params.surface_state_offset = SURFACE_STATE_OFFSET (1);
  params.surface_2d = &ctx->brc_distortion;
  media_add_surface_state (&params);

  media_unmap_buffer_obj (gpe_ctx->surface_state_binding_table.res.bo);
}

VOID
media_set_curbe_vp8_brc_update_g7(struct encode_state *encode_state,
				  MEDIA_BRC_UPDATE_PARAMS_VP8 * params)
{
  MEDIA_CURBE_DATA_BRC_UPDATE_G75 *cmd =
    (MEDIA_CURBE_DATA_BRC_UPDATE_G75 *)params->curbe_cmd_buff;
  VAEncPictureParameterBufferVP8 *pic_params =
    (VAEncPictureParameterBufferVP8 *) encode_state->pic_param_ext->buffer;
  VAQMatrixBufferVP8 *quant_params =
    (VAQMatrixBufferVP8 *) encode_state->q_matrix->buffer;

  memset (cmd, 0, sizeof (*cmd));

  cmd->dw1.frame_number = params->frame_number;
  cmd->dw2.picture_header_size = 0; // matching kernel value
  cmd->dw5.target_size_flag= 0;

  if (*params->brc_init_current_target_buf_full_in_bits > (DOUBLE)params->brc_init_reset_buf_size_in_bits) {
    *params->brc_init_current_target_buf_full_in_bits -= (DOUBLE)params->brc_init_reset_buf_size_in_bits;
    cmd->dw5.target_size_flag = 1;
  }

  cmd->dw0.target_size = (UINT)(*params->brc_init_current_target_buf_full_in_bits); // 500000 bits

  cmd->dw3.start_global_adjust_frame0 = 10;
  cmd->dw3.start_global_adjust_frame1 = 50;

  cmd->dw4.start_global_adjust_frame2 = 100;
  cmd->dw4.start_global_adjust_frame3 = 150;

  cmd->dw5.curr_frame_type = (params->pic_coding_type == FRAME_TYPE_I) ? 2 : 0;
  cmd->dw5.brc_flag = 16;
  cmd->dw5.max_num_paks = 4;

  cmd->dw8.start_global_adjust_mult0 = 1;
  cmd->dw8.start_global_adjust_mult1 = 1;
  cmd->dw8.start_global_adjust_mult2 = 3;
  cmd->dw8.start_global_adjust_mult3 = 2;

  cmd->dw9.start_global_adjust_div0 = 40;
  cmd->dw9.start_global_adjust_div1 = 5;
  cmd->dw9.start_global_adjust_div2 = 5;
  cmd->dw9.start_global_adjust_mult4 = 1;

  cmd->dw10.start_global_adjust_div3 = 3;
  cmd->dw10.start_global_adjust_div4 = 1;
  cmd->dw10.qp_threshold0 = 20; // 7;
  cmd->dw10.qp_threshold1 = 40; // 18;

  cmd->dw11.qp_threshold2 = 60; // 25;
  cmd->dw11.qp_threshold3 = 90; // 37;
  cmd->dw11.rate_ratio_threshold0 = 40;
  cmd->dw11.rate_ratio_threshold1 = 75;

  cmd->dw12.rate_ratio_threshold2 = 97;
  cmd->dw12.rate_ratio_threshold3 = 103;
  cmd->dw12.rate_ratio_threshold4 = 125;
  cmd->dw12.rate_ratio_threshold5 = 160;

  cmd->dw13.rate_ratio_threshold_qp0 = -3;
  cmd->dw13.rate_ratio_threshold_qp1 = -2;
  cmd->dw13.rate_ratio_threshold_qp2 = -1;
  cmd->dw13.rate_ratio_threshold_qp3 = 0;

  cmd->dw14.rate_ratio_threshold_qp4 = 1;
  cmd->dw14.rate_ratio_threshold_qp5 = 2;
  cmd->dw14.rate_ratio_threshold_qp6 = 3;
  cmd->dw14.index_of_previous_qp = 0;

  *params->brc_init_current_target_buf_full_in_bits += params->brc_init_reset_input_bits_per_frame;

  cmd->dw15.frame_width_in_mb = params->frame_width_in_mbs;
  cmd->dw15.frame_height_in_mb = params->frame_height_in_mbs;
  cmd->dw15.prev_flag = !params->frame_update->two_prev_frame_flag;

   if (params->frame_update->prev_frame_size != 0)
    cmd->dw16.frame_byte_count  = params->frame_update->prev_frame_size - 12;

  if (params->frame_number == 1)
    cmd->dw16.frame_byte_count -= 32;

  if (params->frame_update->two_prev_frame_flag == 1 && params->frame_number == 1)
    cmd->dw16.frame_byte_count = 0;

  cmd->dw17.key_frame_qp_seg0 = quant_params->quantization_index[0];
  cmd->dw17.key_frame_qp_seg1 = quant_params->quantization_index[1];
  cmd->dw17.key_frame_qp_seg2 = quant_params->quantization_index[2];
  cmd->dw17.key_frame_qp_seg3 = quant_params->quantization_index[3];

  cmd->dw18.qp_delta_plane0 = quant_params->quantization_index_delta[0];
  cmd->dw18.qp_delta_plane1 = quant_params->quantization_index_delta[4];
  cmd->dw18.qp_delta_plane2 = quant_params->quantization_index_delta[3];
  cmd->dw18.qp_delta_plane3 = quant_params->quantization_index_delta[2];

  cmd->dw19.qp = 0;
  cmd->dw19.qp_delta_plane4 = quant_params->quantization_index_delta[1];
  cmd->dw19.reserved = 9;

  cmd->dw20.segmentation_enabled = pic_params->pic_flags.bits.segmentation_enabled;
  cmd->dw20.brc_method = 1;
  cmd->dw20.mb_rc = 0;

  cmd->dw20.vme_intraprediction = (params->kernel_mode == PERFORMANCE_MODE) ? 1 : 0;
  cmd->dw21.history_buffer_index = VP8_BRC_UPDATE_HISTORY_G75;
  cmd->dw22.pak_surface_index = VP8_BRC_UPDATE_PAK_SURFACE_INDEX_G75;
  cmd->dw23.mbpak_curbe1_index = VP8_BRC_UPDATE_MBPAK1_CURBE_WRITE_G75;
  cmd->dw24.mbpak_curbe2_index = VP8_BRC_UPDATE_MBPAK2_CURBE_WRITE_G75;
  cmd->dw25.mbenc_curbe_input_index = VP8_BRC_UPDATE_MBENC_CURBE_READ_G75;
  cmd->dw26.mbenc_curbe_output_index = VP8_BRC_UPDATE_MBENC_CURBE_WRITE_G75;
  cmd->dw27.distortion_input_index = VP8_BRC_UPDATE_DISTORTION_SURFACE_G75;
  cmd->dw28.constant_data_input_index = VP8_BRC_UPDATE_CONSTANT_DATA_G75;
  cmd->dw29.pak_table_surface_index = VP8_BRC_UPDATE_MBPAK_TABLE_INDEX_G75;
}

VOID
media_surface_state_vp8_brc_update_g7(MEDIA_ENCODER_CTX * encoder_context,
				      struct encode_state *encode_state,
				      BRC_UPDATE_SURFACE_PARAMS_VP8 *surface_params)
{
  BRC_UPDATE_CONTEXT *ctx = &encoder_context->brc_update_context;
  MEDIA_GPE_CTX *gpe_ctx = &ctx->gpe_context;
  SURFACE_SET_PARAMS params;
  BYTE *binding_surface_state_buf = NULL;
  BRC_INIT_RESET_CONTEXT *brc_init_reset_ctx = &encoder_context->brc_init_reset_context;
  struct object_surface *obj_surface;
  MEDIA_RESOURCE surface_2d;
  MBPAK_CONTEXT *mbpak_ctx = &encoder_context->mbpak_context;
  MEDIA_GPE_CTX *mbpak_gpe_ctx;
  MBENC_CONTEXT *mbenc_ctx = &encoder_context->mbenc_context;
  MEDIA_GPE_CTX *mbenc_gpe_ctx = &mbenc_ctx->gpe_context;

  binding_surface_state_buf =
    (BYTE *) media_map_buffer_obj (gpe_ctx->surface_state_binding_table.res.bo);

  /* 0 BRC history buffer */
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.binding_table_offset = BINDING_TABLE_OFFSET (0); /* ??? 1 ? */
  params.surface_state_offset = SURFACE_STATE_OFFSET (0);
  params.buf_object = brc_init_reset_ctx->brc_history;
  params.size = brc_init_reset_ctx->brc_history.bo_size;
  params.cacheability_control =
    surface_params->cacheability_control;
  media_add_surface_state (&params);

  /* 1 Coded buffer (MB data) buffer */
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.binding_table_offset = BINDING_TABLE_OFFSET (1);
  params.surface_state_offset = SURFACE_STATE_OFFSET (1);
  obj_surface = encode_state->coded_buf_surface;
  OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
  params.buf_object = surface_2d;
  params.offset = 0;
  params.size = 8 * sizeof(UINT);
  params.cacheability_control = surface_params->cacheability_control;
  media_add_surface_state (&params);

  /* 2 MBPAK Curbe1 surface */
  mbpak_gpe_ctx = &mbpak_ctx->gpe_context;
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.binding_table_offset = BINDING_TABLE_OFFSET (2);
  params.surface_state_offset = SURFACE_STATE_OFFSET (2);
  params.buf_object = mbpak_gpe_ctx->dynamic_state.res;
  params.size = sizeof(MEDIA_CURBE_DATA_MBPAK_P1_G75);
  params.cacheability_control = surface_params->cacheability_control;
  media_add_surface_state (&params);

  /* 3 MBPAK Curbe2 surface */
  mbpak_gpe_ctx = &mbpak_ctx->gpe_context2;
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.binding_table_offset = BINDING_TABLE_OFFSET (3);
  params.surface_state_offset = SURFACE_STATE_OFFSET (3);
  params.buf_object = mbpak_gpe_ctx->dynamic_state.res;
  params.size = sizeof(MEDIA_CURBE_DATA_MBPAK_P2_G75);
  params.cacheability_control = surface_params->cacheability_control;
  media_add_surface_state (&params);

  /* 4 MBENC Curbe read surface */
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.binding_table_offset = BINDING_TABLE_OFFSET (4);
  params.surface_state_offset = SURFACE_STATE_OFFSET (4);
  params.buf_object = mbenc_gpe_ctx->dynamic_state.res;
  params.size = ALIGN(MAX(sizeof(MEDIA_CURBE_DATA_MBENC_P_G75), sizeof(MEDIA_CURBE_DATA_MBENC_I_G75)), 64);
  params.cacheability_control = surface_params->cacheability_control;
  media_add_surface_state (&params);

  /* 5 MBENC Curbe write surface */
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.binding_table_offset = BINDING_TABLE_OFFSET (5);
  params.surface_state_offset = SURFACE_STATE_OFFSET (5);
  params.buf_object = mbenc_gpe_ctx->dynamic_state.res;
  params.size = ALIGN(MAX(sizeof(MEDIA_CURBE_DATA_MBENC_P_G75), sizeof(MEDIA_CURBE_DATA_MBENC_I_G75)), 64);
  params.cacheability_control = surface_params->cacheability_control;
  media_add_surface_state (&params);

  /* 6 BRC Distortion data buffer */
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.surface_is_2d = 1;
  params.media_block_raw = 1; /* media block read/write message */
  params.vert_line_stride_offset = 0;
  params.vert_line_stride = 0;
  params.format = STATE_SURFACEFORMAT_R8_UNORM;
  params.binding_table_offset = BINDING_TABLE_OFFSET (6);
  params.surface_state_offset = SURFACE_STATE_OFFSET (6);
  params.surface_2d = &brc_init_reset_ctx->brc_distortion;
  media_add_surface_state (&params);

  /* 7 BRC Constant buffer */
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.binding_table_offset = BINDING_TABLE_OFFSET (7);
  params.surface_state_offset = SURFACE_STATE_OFFSET (7);
  params.buf_object = brc_init_reset_ctx->brc_constant_data;
  params.size = 2880;
  params.cacheability_control =
    surface_params->cacheability_control;
  media_add_surface_state (&params);

  /* 8 MBPAK table buffer */
  params = surface_set_params_init;
  params.binding_surface_state.bo =
    gpe_ctx->surface_state_binding_table.res.bo;
  params.binding_surface_state.buf = binding_surface_state_buf;
  params.binding_table_offset = BINDING_TABLE_OFFSET (8);
  params.surface_state_offset = SURFACE_STATE_OFFSET (8);
  params.buf_object = brc_init_reset_ctx->brc_pak_qp_input_table;
  params.size = 160 * 18 * sizeof(UINT16);
  params.cacheability_control =
    surface_params->cacheability_control;
  media_add_surface_state (&params);

  media_unmap_buffer_obj (gpe_ctx->surface_state_binding_table.res.bo);
}

VOID
media_encode_init_brc_update_constant_data_vp8_g7(BRC_UPDATE_CONSTANT_DATA_PARAMS_VP8 *params)
{
  BYTE *pbuffer;
  BOOL status;

  pbuffer =
    (BYTE *) media_map_buffer_obj (params->brc_update_constant_data->bo);
  MEDIA_DRV_ASSERT (pbuffer);

  media_drv_memset (pbuffer, 2880);

  status = media_drv_memcpy (pbuffer,
			     sizeof (brc_qpadjustment_distthreshold_maxframethreshold_distqpadjustment_IPB_vp8_g7),
			     (VOID *) brc_qpadjustment_distthreshold_maxframethreshold_distqpadjustment_IPB_vp8_g7,
			     sizeof (brc_qpadjustment_distthreshold_maxframethreshold_distqpadjustment_IPB_vp8_g7));

  if (status != TRUE) {
    media_unmap_buffer_obj (params->brc_update_constant_data->bo);
    MEDIA_DRV_ASSERT ("media_drv_memcpy failed");
  }

  pbuffer += sizeof(brc_qpadjustment_distthreshold_maxframethreshold_distqpadjustment_IPB_vp8_g7);

  status = media_drv_memcpy (pbuffer,
			     sizeof (brc_mv_cost_table_vp8_g7),
			     (VOID *) brc_mv_cost_table_vp8_g7,
			     sizeof (brc_mv_cost_table_vp8_g7));

  if (status != TRUE) {
    media_unmap_buffer_obj (params->brc_update_constant_data->bo);
    MEDIA_DRV_ASSERT ("media_drv_memcpy failed");
  }

  pbuffer += sizeof(brc_mv_cost_table_vp8_g7);

  status = media_drv_memcpy (pbuffer,
			     sizeof (brc_quant_dc_table_vp8_g7),
			     (VOID *) brc_quant_dc_table_vp8_g7,
			     sizeof (brc_quant_dc_table_vp8_g7));

  if (status != TRUE) {
    media_unmap_buffer_obj (params->brc_update_constant_data->bo);
    MEDIA_DRV_ASSERT ("media_drv_memcpy failed");
  }

  pbuffer += sizeof(brc_quant_dc_table_vp8_g7);

  status = media_drv_memcpy (pbuffer,
			     sizeof (brc_quant_ac_table_vp8_g7),
			     (VOID *) brc_quant_ac_table_vp8_g7,
			     sizeof (brc_quant_ac_table_vp8_g7));

  if (status != TRUE) {
    media_unmap_buffer_obj (params->brc_update_constant_data->bo);
    MEDIA_DRV_ASSERT ("media_drv_memcpy failed");
  }

  pbuffer += sizeof(brc_quant_ac_table_vp8_g7);

  status = media_drv_memcpy (pbuffer,
			     sizeof (brc_skip_mv_threshold_table_vp8_g7),
			     (VOID *) brc_skip_mv_threshold_table_vp8_g7,
			     sizeof (brc_skip_mv_threshold_table_vp8_g7));

  if (status != TRUE) {
    media_unmap_buffer_obj (params->brc_update_constant_data->bo);
    MEDIA_DRV_ASSERT ("media_drv_memcpy failed");
  }

  pbuffer += sizeof(brc_skip_mv_threshold_table_vp8_g7);

  media_unmap_buffer_obj (params->brc_update_constant_data->bo);
}
