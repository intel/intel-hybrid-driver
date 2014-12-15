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
 *     Wei Lin<wei.w.lin@intel.com>
 *     Yuting Yang<yuting.yang@intel.com>
 */
#ifndef __GENOS_RESOURCE_DEFS_H__
#define __GENOS_RESOURCE_DEFS_H__

typedef enum _GENOS_FORMAT {
	Format_Invalid = -14,
	Format_Source = -13,

	Format_420O = -12,
	Format_RGB_Swap = -11,
	Format_RGB_No_Swap = -10,
	Format_RGB = -9,
	Format_RGB32 = -8,
	Format_PA = -7,
	Format_PL2 = -6,
	Format_PL2_UnAligned = -5,
	Format_PL3 = -4,
	Format_PL3_RGB = -3,
	Format_PAL = -2,

	Format_None = -1,
	Format_Any = 0,

	Format_A8R8G8B8,
	Format_X8R8G8B8,
	Format_A8B8G8R8,
	Format_X8B8G8R8,
	Format_A16B16G16R16,
	Format_A16R16G16B16,
	Format_R5G6B5,
	Format_R32U,
	Format_R32F,

	Format_RGBP,
	Format_BGRP,

	Format_YUY2,
	Format_YUYV,
	Format_YVYU,
	Format_UYVY,
	Format_VYUY,

	Format_Y416,
	Format_AYUV,
	Format_AUYV,

	Format_400P,

	Format_NV12,
	Format_NV12_UnAligned,
	Format_NV21,
	Format_NV11,
	Format_NV11_UnAligned,
	Format_P208,
	Format_P208_UnAligned,

	Format_IMC1,
	Format_IMC2,
	Format_IMC3,
	Format_IMC4,
	Format_422H,
	Format_422V,
	Format_444P,
	Format_411P,
	Format_411R,
	Format_I420,
	Format_IYUV,
	Format_YV12,
	Format_YVU9,

	Format_AI44,
	Format_IA44,
	Format_P8,
	Format_A8P8,

	Format_A8,
	Format_L8,
	Format_A4L4,
	Format_A8L8,

	Format_IRW0,
	Format_IRW1,
	Format_IRW2,
	Format_IRW3,
	Format_IRW4,
	Format_IRW5,
	Format_IRW6,
	Format_IRW7,

	Format_STMM,

	Format_Buffer,
	Format_Buffer_2D,

	Format_V8U8,

	Format_R32S,
	Format_R8U,
	Format_R8G8UN,
	Format_R8G8SN,
	Format_G8R8_G8B8,
	Format_R16U,
	Format_R16S,
	Format_R16UN,
	Format_RAW,

	Format_Y8,
	Format_Y1,
	Format_Y16U,
	Format_Y16S,

	Format_L16,
	Format_D16,
	Format_R10G10B10A2,

	Format_P016,
	Format_P010,
	Format_YV12_Planar,
	Format_Count
} GENOS_FORMAT, *PGENOS_FORMAT;
C_ASSERT(Format_Count == 80);

#define IS_PAL_FORMAT(format)            \
            ( (format == Format_AI44) || \
              (format == Format_IA44) || \
              (format == Format_P8)   || \
              (format == Format_A8P8))

#define IS_PL3_RGB_FORMAT(format)        \
            ( (format == Format_RGBP) || \
              (format == Format_BGRP) )

#define IS_RGB16_FORMAT(format)              \
            (format == Format_R5G6B5)

#define IS_RGB32_FORMAT(format)              \
            ( (format == Format_A8R8G8B8) || \
              (format == Format_X8R8G8B8) || \
              (format == Format_A8B8G8R8) || \
              (format == Format_X8B8G8R8) || \
              (format == Format_RGB32) )

#define IS_RGB64_FORMAT(format)                  \
            ( (format == Format_A16B16G16R16) || \
              (format == Format_A16R16G16B16))

#define IS_RGB_FORMAT(format)              \
            ( IS_RGB64_FORMAT(format)   || \
              IS_RGB32_FORMAT(format)   || \
              IS_RGB16_FORMAT(format)   || \
              IS_PL3_RGB_FORMAT(format) || \
              (format == Format_RGB) )

#define IS_PA_FORMAT(format)             \
            ( (format == Format_PA)   || \
              (format == Format_YUY2) || \
              (format == Format_YUYV) || \
              (format == Format_YVYU) || \
              (format == Format_UYVY) || \
              (format == Format_VYUY) )

#define IS_PL2_FORMAT(format)            \
            ( (format == Format_PL2)  || \
              (format == Format_NV12) || \
              (format == Format_NV21) || \
              (format == Format_NV11) || \
              (format == Format_P208) )

#define IS_PL3_FORMAT(format)            \
            ( (format == Format_PL3)  || \
              (format == Format_IMC1) || \
              (format == Format_IMC2) || \
              (format == Format_IMC3) || \
              (format == Format_IMC4) || \
              (format == Format_I420) || \
              (format == Format_IYUV) || \
              (format == Format_YV12) || \
              (format == Format_YVU9) || \
              (format == Format_422H) || \
              (format == Format_422V) || \
              (format == Format_411P) || \
              (format == Format_411R) || \
              (format == Format_444P) )

#define IS_YUV_FORMAT(format)              \
          ( IS_PL2_FORMAT(format)       || \
            IS_PL3_FORMAT(format)       || \
            IS_PA_FORMAT(format)        || \
            (format == Format_400P))

#define GENOS_YTILE_H_ALIGNMENT  32
#define GENOS_YTILE_W_ALIGNMENT  128
#define GENOS_XTILE_H_ALIGNMENT  8
#define GENOS_XTILE_W_ALIGNMENT  512

typedef enum _GENOS_TILE_TYPE {
	GENOS_TILE_X,
	GENOS_TILE_Y,
	GENOS_TILE_LINEAR,
	GENOS_TILE_INVALID
} GENOS_TILE_TYPE;
C_ASSERT(GENOS_TILE_LINEAR == 2);

typedef enum _GENOS_GFXRES_TYPE {
	GENOS_GFXRES_INVALID = -1,
	GENOS_GFXRES_BUFFER,
	GENOS_GFXRES_2D,
	GENOS_GFXRES_VOLUME,
} GENOS_GFXRES_TYPE;

#endif
