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

#ifndef __HWCMD__
#define __HWCMD__

#include "oscl_platform_def.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum {
		MI_BUFFER_MEMORY_MAIN = 0,
		MI_BUFFER_MEMORY_LOCAL = 1,
		MI_BUFFER_MEMORY_GTT = 2
	} _MI_BUFFER_MEMORY_SPACE;

	typedef enum {
		MI_BUFFER_SECURE = 0,
		MI_BUFFER_NONSECURE = 1
	} _MI_BUFFER_SECURITY_TYPE;

#ifndef SIZE16
#define SIZE16( x )         ((DWORD)( sizeof(x) / sizeof(WORD) ))
#endif

#ifndef SIZE32
#define SIZE32( x )         ((DWORD)( sizeof(x) / sizeof(DWORD) ))
#endif

#ifndef OP_LENGTH
#define OP_LENGTH( x )      ((DWORD)(x) - 2 )
#endif

#define SIZE_IN_DW(x) ((x) / sizeof(DWORD))

#ifndef BITFIELD_RANGE
#define BITFIELD_RANGE( startbit, endbit )     ((endbit)-(startbit)+1)
#endif

#ifndef BITFIELD_BIT
#define BITFIELD_BIT( bit )                   1
#endif

	enum INSTRUCTION_PIPELINE {
		PIPE_COMMON = 0x0,
		PIPE_SINGLE_DWORD = 0x1,
		PIPE_COMMON_CTG = 0x1,
		PIPE_MEDIA = 0x2,
		PIPE_3D = 0x3
	};

	enum GFX_OPCODE {
		GFXOP_PIPELINED = 0x0,
		GFXOP_NONPIPELINED = 0x1,
		GFXOP_3DPRIMITIVE = 0x3
	};

	enum GFX3D_OPCODE {
		GFX3DOP_3DSTATE_PIPELINED = 0x0,
		GFX3DOP_3DSTATE_NONPIPELINED = 0x1,
		GFX3DOP_3DCONTROL = 0x2,
		GFX3DOP_3DPRIMITIVE = 0x3
	};

	enum GFX_MEDIA_OPCODE {
		MEDIAOP_MEDIA_STATE_POINTERS = 0,
		MEDIAOP_MEDIA_OBJECT = 1,
	};

	enum GFX_MEDIA_SUBOPCODE {
		MEDIASUBOP_MEDIA_OBJECT = 0,
		MEDIASUBOP_MEDIA_OBJECT_EX = 1
	};

	enum GFX_NONPIPELINED_SUBOPCODE {
		GFXSUBOP_STATE_BASE_ADDRESS = 0x1,
		GFXSUBOP_STATE_SIP = 0x2,
		GFXSUBOP_STATE_PREFETCH = 0x3,
		GFXSUBOP_PIPELINE_SELECT = 0x4
	};

	enum GFX3DCONTROL_SUBOPCODE {
		GFX3DSUBOP_3DCONTROL = 0x00
	};

	typedef enum _INSTRUCTION_TYPE {
		INSTRUCTION_MI = 0x0,
		INSTRUCTION_TRUSTED = 0x1,
		INSTRUCTION_2D = 0x2,
		INSTRUCTION_GFX = 0x3
	} INSTRUCTION_TYPE;

	enum GFX_COMMON_PIPELINED_SUBOPCODE {
		GFXSUBOP_URB_FENCE = 0x0,
		GFXSUBOP_CS_URB_STATE = 0x1,
		GFXSUBOP_CONSTANT_BUFFER = 0x2
	};

	enum GFX_COMMON_TOKEN_SUBOPCODE {
		GFXSUBOP_BINDING_TABLE_STATE_TOKEN = 0xFE,
		GFXSUBOP_SURFACE_STATE_TOKEN = 0xFF
	};
	enum GFX3DSTATE_CUBE_MAP_CORNER_MODE {
		GFX3DSTATE_CUBE_REPLICATE = 0x0,
		GFX3DSTATE_CUBE_AVERAGE = 0X1
	};
	enum GFX3DSTATE_RENDER_CACHE_READ_WRITE_MODE {
		GFX3DSTATE_WRITE_ONLY_ON_MISS = 0x0,
		GFX3DSTATE_READ_WRITE_ONLY_ON_MISS = 0x1
	};

	enum GFX3DSTATE_MEDIA_BOUNDARY_PIXEL_MODE {
		GFX3DSTATE_BOUNDARY_NORMAL = 0x0,
		GFX3DSTATE_BOUNDARY_PROGRESSIVE_FRAME = 0x2,
		GFX3DSTATE_BOUNDARY_INTERLACED_FRAME = 0x3
	};

	enum GFX3DSTATE_SURFACERETURNFORMAT {
		GFX3DSTATE_SURFACERETURNFORMAT_FLOAT32 = 0,
		GFX3DSTATE_SURFACERETURNFORMAT_S1_14 = 1
	};

	enum GFX3DSTATE_CONSTANT_BUFFER_ADDRESS_MODE {
		GFX3DSTATE_CONSTANT_BUFFER_ADDRESS_GENERAL_STATE,
		GFX3DSTATE_CONSTANT_BUFFER_ADDRESS_GTT,
		GFX3DSTATE_CONSTANT_BUFFER_ADDRESS_SURFACE_STATE,
		NUM_GFX3DSTATE_CONSTANT_BUFFER_ADDRESS_MODES
	};

	enum GFX3DSTATE_PREFILTER_OPERATION {
		GFX3DSTATE_PREFILTER_ALWAYS = 0x0,
		GFX3DSTATE_PREFILTER_NEVER = 0x1,
		GFX3DSTATE_PREFILTER_LESS = 0x2,
		GFX3DSTATE_PREFILTER_EQUAL = 0x3,
		GFX3DSTATE_PREFILTER_LEQUAL = 0x4,
		GFX3DSTATE_PREFILTER_GREATER = 0x5,
		GFX3DSTATE_PREFILTER_NOTEQUAL = 0x6,
		GFX3DSTATE_PREFILTER_GEQUAL = 0x7
	};

	enum GFX3DSTATE_MIPFILTER {
		GFX3DSTATE_MIPFILTER_NONE = 0,
		GFX3DSTATE_MIPFILTER_NEAREST = 1,
		GFX3DSTATE_MIPFILTER_LINEAR = 3
	};

	typedef enum _GFX3DSTATE_MAPFILTER {
		GFX3DSTATE_MAPFILTER_NEAREST = 0x0,
		GFX3DSTATE_MAPFILTER_LINEAR = 0x1,
		GFX3DSTATE_MAPFILTER_ANISOTROPIC = 0x2,
		GFX3DSTATE_MAPFILTER_FLEXIBLE = 0x3,
		GFX3DSTATE_MAPFILTER_MONO = 0x6
	} GFX3DSTATE_MAPFILTER;

	enum GFX3DSTATE_DEFAULTCOLOR_MODE {
		GFX3DSTATE_DEFAULTCOLOR_R32G32B32A32_FLOAT = 0,
		GFX3DSTATE_DEFAULTCOLOR_R8G8B8A8_UNORM = 1
	};

	typedef enum _GFX3DSTATE_TEXCOORDMODE {
		GFX3DSTATE_TEXCOORDMODE_WRAP = 0,
		GFX3DSTATE_TEXCOORDMODE_MIRROR = 1,
		GFX3DSTATE_TEXCOORDMODE_CLAMP = 2,
		GFX3DSTATE_TEXCOORDMODE_CUBE = 3,
		GFX3DSTATE_TEXCOORDMODE_CLAMP_BORDER = 4,
		GFX3DSTATE_TEXCOORDMODE_MIRROR_ONCE = 5
	} GFX3DSTATE_TEXCOORDMODE;

	enum GFX3DSTATE_CUBESURFACECONTROLMODE {
		GFX3DSTATE_CUBESURFACECONTROLMODE_PROGRAMMED = 0,
		GFX3DSTATE_CUBESURFACECONTROLMODE_OVERRIDE = 1
	};

	enum GFX3DSTATE_ANISORATIO {
		GFX3DSTATE_ANISORATIO_2 = 0,
		GFX3DSTATE_ANISORATIO_4 = 1,
		GFX3DSTATE_ANISORATIO_6 = 2,
		GFX3DSTATE_ANISORATIO_8 = 3,
		GFX3DSTATE_ANISORATIO_10 = 4,
		GFX3DSTATE_ANISORATIO_12 = 5,
		GFX3DSTATE_ANISORATIO_14 = 6,
		GFX3DSTATE_ANISORATIO_16 = 7
	};

	enum GFX3DSTATE_CHROMAKEY_MODE {
		GFX3DSTATE_CHROMAKEY_KILL_ON_ANY_MATCH = 0,
		GFX3DSTATE_CHROMAKEY_REPLACE_BLACK = 1
	};

	enum GFX3DSTATE_SURFACETYPE {
		GFX3DSTATE_SURFACETYPE_1D = 0,
		GFX3DSTATE_SURFACETYPE_2D = 1,
		GFX3DSTATE_SURFACETYPE_3D = 2,
		GFX3DSTATE_SURFACETYPE_CUBE = 3,
		GFX3DSTATE_SURFACETYPE_BUFFER = 4,
		GFX3DSTATE_SURFACETYPE_NULL = 7
	};

	enum GFX3DSTATE_RENDERTARGET_ROTATE {
		GFX3DSTATE_RENDERTARGET_ROTATE_0DEG = 0,
		GFX3DSTATE_RENDERTARGET_ROTATE_90DEG = 1,
		GFX3DSTATE_RENDERTARGET_ROTATE_270DEG = 3,
	};

	enum GFX3DSTATE_PIPELINED_SUBOPCODE {
		GFX3DSUBOP_3DSTATE_PIPELINED_POINTERS = 0x00,
		GFX3DSUBOP_3DSTATE_BINDING_TABLE_POINTERS = 0x01,
		GFX3DSUBOP_3DSTATE_STATE_POINTER_INVALIDATE = 0x02,
		GFX3DSUBOP_3DSTATE_VERTEX_BUFFERS = 0x08,
		GFX3DSUBOP_3DSTATE_VERTEX_ELEMENTS = 0x09,
		GFX3DSUBOP_3DSTATE_INDEX_BUFFER = 0x0A,
		GFX3DSUBOP_3DSTATE_VF_STATISTICS = 0x0B
	};

	enum GFX3DSTATE_SURFACE_MIPMAPLAYOUT {
		GFX3DSTATE_SURFACE_MIPMAPLAYOUT_BELOW = 0,
		GFX3DSTATE_SURFACE_MIPMAPLAYOUT_RIGHT = 1
	};

	enum GFX3DSTATE_SURFACEFORMAT {
		GFX3DSTATE_SURFACEFORMAT_R32G32B32A32_FLOAT = 0x000,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32A32_SINT = 0x001,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32A32_UINT = 0x002,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32A32_UNORM = 0x003,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32A32_SNORM = 0x004,
		GFX3DSTATE_SURFACEFORMAT_R64G64_FLOAT = 0x005,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32X32_FLOAT = 0x006,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32A32_SSCALED = 0x007,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32A32_USCALED = 0x008,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32_FLOAT = 0x040,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32_SINT = 0x041,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32_UINT = 0x042,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32_UNORM = 0x043,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32_SNORM = 0x044,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32_SSCALED = 0x045,
		GFX3DSTATE_SURFACEFORMAT_R32G32B32_USCALED = 0x046,
		GFX3DSTATE_SURFACEFORMAT_R16G16B16A16_UNORM = 0x080,
		GFX3DSTATE_SURFACEFORMAT_R16G16B16A16_SNORM = 0x081,
		GFX3DSTATE_SURFACEFORMAT_R16G16B16A16_SINT = 0x082,
		GFX3DSTATE_SURFACEFORMAT_R16G16B16A16_UINT = 0x083,
		GFX3DSTATE_SURFACEFORMAT_R16G16B16A16_FLOAT = 0x084,
		GFX3DSTATE_SURFACEFORMAT_R32G32_FLOAT = 0x085,
		GFX3DSTATE_SURFACEFORMAT_R32G32_SINT = 0x086,
		GFX3DSTATE_SURFACEFORMAT_R32G32_UINT = 0x087,
		GFX3DSTATE_SURFACEFORMAT_R32_FLOAT_X8X24_TYPELESS = 0x088,
		GFX3DSTATE_SURFACEFORMAT_X32_TYPELESS_G8X24_UINT = 0x089,
		GFX3DSTATE_SURFACEFORMAT_L32A32_FLOAT = 0x08A,
		GFX3DSTATE_SURFACEFORMAT_R32G32_UNORM = 0x08B,
		GFX3DSTATE_SURFACEFORMAT_R32G32_SNORM = 0x08C,
		GFX3DSTATE_SURFACEFORMAT_R64_FLOAT = 0x08D,
		GFX3DSTATE_SURFACEFORMAT_R16G16B16X16_UNORM = 0x08E,
		GFX3DSTATE_SURFACEFORMAT_R16G16B16X16_FLOAT = 0x08F,
		GFX3DSTATE_SURFACEFORMAT_A32X32_FLOAT = 0x090,
		GFX3DSTATE_SURFACEFORMAT_L32X32_FLOAT = 0x091,
		GFX3DSTATE_SURFACEFORMAT_I32X32_FLOAT = 0x092,
		GFX3DSTATE_SURFACEFORMAT_R16G16B16A16_SSCALED = 0x093,
		GFX3DSTATE_SURFACEFORMAT_R16G16B16A16_USCALED = 0x094,
		GFX3DSTATE_SURFACEFORMAT_R32G32_SSCALED = 0x095,
		GFX3DSTATE_SURFACEFORMAT_R32G32_USCALED = 0x096,
		GFX3DSTATE_SURFACEFORMAT_B8G8R8A8_UNORM = 0x0C0,
		GFX3DSTATE_SURFACEFORMAT_B8G8R8A8_UNORM_SRGB = 0x0C1,
		GFX3DSTATE_SURFACEFORMAT_R10G10B10A2_UNORM = 0x0C2,
		GFX3DSTATE_SURFACEFORMAT_R10G10B10A2_UNORM_SRGB = 0x0C3,
		GFX3DSTATE_SURFACEFORMAT_R10G10B10A2_UINT = 0x0C4,
		GFX3DSTATE_SURFACEFORMAT_R10G10B10_SNORM_A2_UNORM = 0x0C5,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8A8_UNORM = 0x0C7,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8A8_UNORM_SRGB = 0x0C8,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8A8_SNORM = 0x0C9,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8A8_SINT = 0x0CA,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8A8_UINT = 0x0CB,
		GFX3DSTATE_SURFACEFORMAT_R16G16_UNORM = 0x0CC,
		GFX3DSTATE_SURFACEFORMAT_R16G16_SNORM = 0x0CD,
		GFX3DSTATE_SURFACEFORMAT_R16G16_SINT = 0x0CE,
		GFX3DSTATE_SURFACEFORMAT_R16G16_UINT = 0x0CF,
		GFX3DSTATE_SURFACEFORMAT_R16G16_FLOAT = 0x0D0,
		GFX3DSTATE_SURFACEFORMAT_B10G10R10A2_UNORM = 0x0D1,
		GFX3DSTATE_SURFACEFORMAT_B10G10R10A2_UNORM_SRGB = 0x0D2,
		GFX3DSTATE_SURFACEFORMAT_R11G11B10_FLOAT = 0x0D3,
		GFX3DSTATE_SURFACEFORMAT_R32_SINT = 0x0D6,
		GFX3DSTATE_SURFACEFORMAT_R32_UINT = 0x0D7,
		GFX3DSTATE_SURFACEFORMAT_R32_FLOAT = 0x0D8,
		GFX3DSTATE_SURFACEFORMAT_R24_UNORM_X8_TYPELESS = 0x0D9,
		GFX3DSTATE_SURFACEFORMAT_X24_TYPELESS_G8_UINT = 0x0DA,
		GFX3DSTATE_SURFACEFORMAT_L16A16_UNORM = 0x0DF,
		GFX3DSTATE_SURFACEFORMAT_I24X8_UNORM = 0x0E0,
		GFX3DSTATE_SURFACEFORMAT_L24X8_UNORM = 0x0E1,
		GFX3DSTATE_SURFACEFORMAT_A24X8_UNORM = 0x0E2,
		GFX3DSTATE_SURFACEFORMAT_I32_FLOAT = 0x0E3,
		GFX3DSTATE_SURFACEFORMAT_L32_FLOAT = 0x0E4,
		GFX3DSTATE_SURFACEFORMAT_A32_FLOAT = 0x0E5,
		GFX3DSTATE_SURFACEFORMAT_B8G8R8X8_UNORM = 0x0E9,
		GFX3DSTATE_SURFACEFORMAT_B8G8R8X8_UNORM_SRGB = 0x0EA,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8X8_UNORM = 0x0EB,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8X8_UNORM_SRGB = 0x0EC,
		GFX3DSTATE_SURFACEFORMAT_R9G9B9E5_SHAREDEXP = 0x0ED,
		GFX3DSTATE_SURFACEFORMAT_B10G10R10X2_UNORM = 0x0EE,
		GFX3DSTATE_SURFACEFORMAT_L16A16_FLOAT = 0x0F0,
		GFX3DSTATE_SURFACEFORMAT_R32_UNORM = 0x0F1,
		GFX3DSTATE_SURFACEFORMAT_R32_SNORM = 0x0F2,
		GFX3DSTATE_SURFACEFORMAT_R10G10B10X2_USCALED = 0x0F3,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8A8_SSCALED = 0x0F4,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8A8_USCALED = 0x0F5,
		GFX3DSTATE_SURFACEFORMAT_R16G16_SSCALED = 0x0F6,
		GFX3DSTATE_SURFACEFORMAT_R16G16_USCALED = 0x0F7,
		GFX3DSTATE_SURFACEFORMAT_R32_SSCALED = 0x0F8,
		GFX3DSTATE_SURFACEFORMAT_R32_USCALED = 0x0F9,
		GFX3DSTATE_SURFACEFORMAT_R8B8G8A8_UNORM = 0x0FA,
		GFX3DSTATE_SURFACEFORMAT_B5G6R5_UNORM = 0x100,
		GFX3DSTATE_SURFACEFORMAT_B5G6R5_UNORM_SRGB = 0x101,
		GFX3DSTATE_SURFACEFORMAT_B5G5R5A1_UNORM = 0x102,
		GFX3DSTATE_SURFACEFORMAT_B5G5R5A1_UNORM_SRGB = 0x103,
		GFX3DSTATE_SURFACEFORMAT_B4G4R4A4_UNORM = 0x104,
		GFX3DSTATE_SURFACEFORMAT_B4G4R4A4_UNORM_SRGB = 0x105,
		GFX3DSTATE_SURFACEFORMAT_R8G8_UNORM = 0x106,
		GFX3DSTATE_SURFACEFORMAT_R8G8_SNORM = 0x107,
		GFX3DSTATE_SURFACEFORMAT_R8G8_SINT = 0x108,
		GFX3DSTATE_SURFACEFORMAT_R8G8_UINT = 0x109,
		GFX3DSTATE_SURFACEFORMAT_R16_UNORM = 0x10A,
		GFX3DSTATE_SURFACEFORMAT_R16_SNORM = 0x10B,
		GFX3DSTATE_SURFACEFORMAT_R16_SINT = 0x10C,
		GFX3DSTATE_SURFACEFORMAT_R16_UINT = 0x10D,
		GFX3DSTATE_SURFACEFORMAT_R16_FLOAT = 0x10E,
		GFX3DSTATE_SURFACEFORMAT_I16_UNORM = 0x111,
		GFX3DSTATE_SURFACEFORMAT_L16_UNORM = 0x112,
		GFX3DSTATE_SURFACEFORMAT_A16_UNORM = 0x113,
		GFX3DSTATE_SURFACEFORMAT_L8A8_UNORM = 0x114,
		GFX3DSTATE_SURFACEFORMAT_I16_FLOAT = 0x115,
		GFX3DSTATE_SURFACEFORMAT_L16_FLOAT = 0x116,
		GFX3DSTATE_SURFACEFORMAT_A16_FLOAT = 0x117,
		GFX3DSTATE_SURFACEFORMAT_L8A8_UNORM_SRGB = 0x118,
		GFX3DSTATE_SURFACEFORMAT_R5G5_SNORM_B6_UNORM = 0x119,
		GFX3DSTATE_SURFACEFORMAT_B5G5R5X1_UNORM = 0x11A,
		GFX3DSTATE_SURFACEFORMAT_B5G5R5X1_UNORM_SRGB = 0x11B,
		GFX3DSTATE_SURFACEFORMAT_R8G8_SSCALED = 0x11C,
		GFX3DSTATE_SURFACEFORMAT_R8G8_USCALED = 0x11D,
		GFX3DSTATE_SURFACEFORMAT_R16_SSCALED = 0x11E,
		GFX3DSTATE_SURFACEFORMAT_R16_USCALED = 0x11F,
		GFX3DSTATE_SURFACEFORMAT_P8A8_UNORM_PALETTE_0 = 0x122,
		GFX3DSTATE_SURFACEFORMAT_P8A8_UNORM_PALETTE_1 = 0x123,
		GFX3DSTATE_SURFACEFORMAT_R8_UNORM = 0x140,
		GFX3DSTATE_SURFACEFORMAT_R8_SNORM = 0x141,
		GFX3DSTATE_SURFACEFORMAT_R8_SINT = 0x142,
		GFX3DSTATE_SURFACEFORMAT_R8_UINT = 0x143,
		GFX3DSTATE_SURFACEFORMAT_A8_UNORM = 0x144,
		GFX3DSTATE_SURFACEFORMAT_I8_UNORM = 0x145,
		GFX3DSTATE_SURFACEFORMAT_L8_UNORM = 0x146,
		GFX3DSTATE_SURFACEFORMAT_P4A4_UNORM_PALETTE_0 = 0x147,
		GFX3DSTATE_SURFACEFORMAT_A4P4_UNORM_PALETTE_0 = 0x148,
		GFX3DSTATE_SURFACEFORMAT_R8_SSCALED = 0x149,
		GFX3DSTATE_SURFACEFORMAT_R8_USCALED = 0x14A,
		GFX3DSTATE_SURFACEFORMAT_P8_UNORM_PALETTE_0 = 0x14B,
		GFX3DSTATE_SURFACEFORMAT_L8_UNORM_SRGB = 0x14C,
		GFX3DSTATE_SURFACEFORMAT_P8_UNORM_PALETTE_1 = 0x14D,
		GFX3DSTATE_SURFACEFORMAT_P4A4_UNORM_PALETTE_1 = 0x14E,
		GFX3DSTATE_SURFACEFORMAT_A4P4_UNORM_PALETTE_1 = 0x14F,
		GFX3DSTATE_SURFACEFORMAT_DXT1_RGB_SRGB = 0x180,
		GFX3DSTATE_SURFACEFORMAT_R1_UINT = 0x181,
		GFX3DSTATE_SURFACEFORMAT_YCRCB_NORMAL = 0x182,
		GFX3DSTATE_SURFACEFORMAT_YCRCB_SWAPUVY = 0x183,
		GFX3DSTATE_SURFACEFORMAT_P2_UNORM_PALETTE_0 = 0x184,
		GFX3DSTATE_SURFACEFORMAT_P2_UNORM_PALETTE_1 = 0x185,
		GFX3DSTATE_SURFACEFORMAT_BC1_UNORM = 0x186,
		GFX3DSTATE_SURFACEFORMAT_BC2_UNORM = 0x187,
		GFX3DSTATE_SURFACEFORMAT_BC3_UNORM = 0x188,
		GFX3DSTATE_SURFACEFORMAT_BC4_UNORM = 0x189,
		GFX3DSTATE_SURFACEFORMAT_BC5_UNORM = 0x18A,
		GFX3DSTATE_SURFACEFORMAT_BC1_UNORM_SRGB = 0x18B,
		GFX3DSTATE_SURFACEFORMAT_BC2_UNORM_SRGB = 0x18C,
		GFX3DSTATE_SURFACEFORMAT_BC3_UNORM_SRGB = 0x18D,
		GFX3DSTATE_SURFACEFORMAT_MONO8 = 0x18E,
		GFX3DSTATE_SURFACEFORMAT_YCRCB_SWAPUV = 0x18F,
		GFX3DSTATE_SURFACEFORMAT_YCRCB_SWAPY = 0x190,
		GFX3DSTATE_SURFACEFORMAT_DXT1_RGB = 0x191,
		GFX3DSTATE_SURFACEFORMAT_FXT1 = 0x192,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8_UNORM = 0x193,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8_SNORM = 0x194,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8_SSCALED = 0x195,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8_USCALED = 0x196,
		GFX3DSTATE_SURFACEFORMAT_R64G64B64A64_FLOAT = 0x197,
		GFX3DSTATE_SURFACEFORMAT_R64G64B64_FLOAT = 0x198,
		GFX3DSTATE_SURFACEFORMAT_BC4_SNORM = 0x199,
		GFX3DSTATE_SURFACEFORMAT_BC5_SNORM = 0x19A,
		GFX3DSTATE_SURFACEFORMAT_R16G16B16_UNORM = 0x19C,
		GFX3DSTATE_SURFACEFORMAT_R16G16B16_SNORM = 0x19D,
		GFX3DSTATE_SURFACEFORMAT_R16G16B16_SSCALED = 0x19E,
		GFX3DSTATE_SURFACEFORMAT_R16G16B16_USCALED = 0x19F,
		GFX3DSTATE_SURFACEFORMAT_PLANAR_420_8 = 0x1A5,
		GFX3DSTATE_SURFACEFORMAT_R8G8B8_UNORM_SRGB = 0x1A8,
		GFX3DSTATE_SURFACEFORMAT_RAW = 0x1FF,
		NUM_GFX3DSTATE_SURFACEFORMATS
	};

	enum GFX3DSTATE_TILEWALK {
		GFX3DSTATE_TILEWALK_XMAJOR = 0,
		GFX3DSTATE_TILEWALK_YMAJOR = 1
	};

	enum GFX3DSTATE_TILEMODE {
		GFX3DSTATE_TILEMODE_LINEAR = 0,
		GFX3DSTATE_TILEMODE_WMAJOR = 1,
		GFX3DSTATE_TILEMODE_XMAJOR = 2,
		GFX3DSTATE_TILEMODE_YMAJOR = 3
	};

	enum GFX3DSTATE_SURFACE_VERTICAL_ALIGNMENT {
		GFX3DSTATE_SURFACE_VERTICAL_ALIGNMENT_2 = 0x0,
		GFX3DSTATE_SURFACE_VERTICAL_ALIGNMENT_4 = 0x1
	};

	enum GFX3DSTATE_SURFACE_HORIZONTAL_ALIGNMENT {
		GFX3DSTATE_SURFACE_HORIZONTAL_ALIGNMENT_4 = 0x0,
		GFX3DSTATE_SURFACE_HORIZONTAL_ALIGNMENT_8 = 0x1
	};

	typedef enum _GFX3DCONTROL_OPERATION {
		GFX3DCONTROLOP_NOWRITE = 0x00,
		GFX3DCONTROLOP_WRITEIMMEDIATE = 0x01,
		GFX3DCONTROLOP_WRITEDEPTH = 0x02,
		GFX3DCONTROLOP_WRITETIMESTAMP = 0x03
	} GFX3DCONTROL_OPERATION;

	typedef enum _GFX3DFLUSH_OPERATION {
		GFX3DFLUSH_NONE = 0x00,
		GFX3DFLUSH_WRITE_CACHE = 0x01,
		GFX3DFLUSH_READ_CACHE = 0x02
	} GFX3DFLUSH_OPERATION;

	enum GFXPIPELINE_SELECT {
		GFXPIPELINE_3D = 0x0,
		GFXPIPELINE_MEDIA = 0x1,
		GFXPIPELINE_GPGPU = 0x2
	};

	enum MEDIASTATE_VFE_GPGPU_MODE {
		MEDIASTATE_MEDIA_MODE = 0,
		MEDIASTATE_GPGPU_MODE = 1
	};

	enum MI_OPCODE {
		MI_NOOP = 0x00,
		MI_SET_PREDICATE = 0x01,
		MI_USER_INTERRUPT = 0x02,
		MI_WAIT_FOR_EVENT = 0x03,
		MI_FLUSH = 0x04,
		MI_ARB_CHECK = 0x05,
		MI_REPORT_HEAD = 0x07,
		MI_ARB_ON_OFF = 0x08,
		MI_BATCH_BUFFER_END = 0x0A,
		MI_LOAD_SCAN_LINES_INCL = 0x12,
		MI_LOAD_SCAN_LINES_EXCL = 0x13,
		MI_DISPLAY_BUFFER_INFO = 0x14,
		MI_SET_CONTEXT = 0x18,
		MI_STORE_DATA_IMM = 0x20,
		MI_STORE_DATA_INDEX = 0x21,
		MI_LOAD_REGISTER_IMM = 0x22,
		MI_STORE_REGISTER_MEM = 0x24,
		MI_LOAD_REGISTER_REG = 0x2A,
		MI_BATCH_BUFFER_START = 0x31,
	};

	enum MI_SET_PREDICATE_ENABLE_GEN7P5 {
		MI_SET_PREDICATE_ENABLE_ALWAYS = 0x0,
		MI_SET_PREDICATE_ENABLE_ON_CLEAR = 0x1,
		MI_SET_PREDICATE_ENABLE_ON_SET = 0x2,
		MI_SET_PREDICATE_DISABLE = 0x3,
	};

	typedef enum _MEDIASTATE_SURFACEFORMAT {
		MEDIASTATE_SURFACEFORMAT_YCRCB_NORMAL = 0,
		MEDIASTATE_SURFACEFORMAT_YCRCB_SWAPUVY = 1,
		MEDIASTATE_SURFACEFORMAT_YCRCB_SWAPUV = 2,
		MEDIASTATE_SURFACEFORMAT_YCRCB_SWAPY = 3,
		MEDIASTATE_SURFACEFORMAT_PLANAR_420_8 = 4,
		MEDIASTATE_SURFACEFORMAT_PLANAR_411_8 = 5,
		MEDIASTATE_SURFACEFORMAT_PLANAR_422_8 = 6,
		MEDIASTATE_SURFACEFORMAT_STMM_DN_STATISTICS = 7,
		MEDIASTATE_SURFACEFORMAT_R10G10B10A2_UNORM = 8,
		MEDIASTATE_SURFACEFORMAT_R8G8B8A8_UNORM = 9,
		MEDIASTATE_SURFACEFORMAT_R8B8_UNORM = 10,
		MEDIASTATE_SURFACEFORMAT_R8_UNORM = 11,
		MEDIASTATE_SURFACEFORMAT_Y8_UNORM = 12,
		MEDIASTATE_SURFACEFORMAT_A8Y8U8V8_UNORM = 13,
		MEDIASTATE_SURFACEFORMAT_B8G8R8A8_UNORM = 14,
		MEDIASTATE_SURFACEFORMAT_R16G16B16A16 = 15
	} MEDIASTATE_SURFACEFORMAT;

	enum MI_OPCODE_G6 {
		MI_SEMAPHORE_MBOX = 0x16,
		MI_FLUSH_DW = 0x26,
		MI_COND_BATCH_BUFFER_END = 0x36
	};

	enum GFX_MEDIA_SUBOPCODE_G6 {
		MEDIASUBOP_MEDIA_VFE_STATE = 0,
		MEDIASUBOP_MEDIA_CURBE_LOAD = 1,
		MEDIASUBOP_MEDIA_INTERFACE_DESCRIPTOR_LOAD = 2,
		MEDIASUBOP_MEDIA_GATEWAY_STATE = 3,
		MEDIASUBOP_MEDIA_OBJECT_WALKER = 3,
		MEDIASUBOP_MEDIA_STATE_FLUSH = 4,
		MEDIASUBOP_GPGPU_WALKER = 5
	};

	enum MEDIASTATE_VALIDPTE_CONTROL {
		MEDIASTATE_INVALID_PTE = 0,
		MEDIASTATE_VALID_PTE = 1
	};

	enum PIPE_BUF_ADDR_STATE_MEMORY_TYPE {
		MEMORY_TYPE_USE_PTE_CHACHEABILITY = 0,
		MEMORY_TYPE_UNCHACHEABLE = 1,
		MEMORY_TYPE_WRITE_THROUGH = 2,
		MEMORY_TYPE_WRITEBACK = 3
	};

	enum MI_POST_SYNC_OPERATION {
		FLUSH_NOWRITE = 0,
		FLUSH_WRITE_IMMEDIATE_DATA = 1,
		FLUSH_WRITE_TIMESTAMP_REG = 3
	};

	enum MI_ADDRESS_SPACE_INDICATOR {
		MI_BB_ADDRESS_USE_GGTT = 0,
		MI_BB_ADDRESS_USE_PPGTT = 1
	};

	enum MI_BB_SECURITY_INDICATOR {
		MI_BB_SECURITY_PRIVILEGED = 0,
		MI_BB_SECURITY_NON_PRIVILEGED = 1
	};

	enum SHADER_CHANNEL_SELECT {
		SCS_ZERO = 0,
		SCS_ONE = 1,
		SCS_RED = 4,
		SCS_GREEN = 5,
		SCS_BLUE = 6,
		SCS_ALPHA = 7
	};
	enum MI_OPCODE_G7 {
		MI_SET_APP_ID = 0x0E
	};

	typedef struct _MEDIA_OBJECT_KA2_INLINE_DATA_G575 {
		union {
			struct {
				DWORD DestinationBlockHorizontalOrigin:16;
				DWORD DestinationBlockVerticalOrigin:16;
			};

			struct {
				DWORD BlockHeight:16;
				DWORD BufferOffset:16;
			};

			struct {
				DWORD StartRowOffset;
			};

			DWORD Value;
		} DW00;

		union {
			struct {
				DWORD HorizontalBlockCompositeMaskLayer0:16;
				DWORD VerticalBlockCompositeMaskLayer0:16;
			};

			struct {
				DWORD TotalRows;
			};

			DWORD Value;
		} DW01;

		union {
			struct {
				DWORD HorizontalBlockCompositeMaskLayer1:16;
				DWORD VerticalBlockCompositeMaskLayer1:16;
			};

			struct {
				DWORD StartColumnOffset;
			};

			DWORD Value;
		} DW02;

		union {
			struct {
				DWORD HorizontalBlockCompositeMaskLayer2:16;
				DWORD VerticalBlockCompositeMaskLayer2:16;
			};

			struct {
				DWORD TotalColumns;
			};

			DWORD Value;
		} DW03;

		union {
			struct {
				FLOAT VideoXScalingStep;
			};

			DWORD Value;
		} DW04;

		union {
			struct {
				FLOAT VideoStepDelta;
			};

			DWORD Value;
		} DW05;

		union {
			struct {
				DWORD VerticalBlockNumber:17;
				DWORD AreaOfInterest:1;
				 DWORD:14;
			};

			DWORD Value;
		} DW06;

		union {
			struct {
				DWORD GroupIDNumber;
			};

			DWORD Value;
		} DW07;

		union {
			struct {
				DWORD HorizontalBlockCompositeMaskLayer3:16;
				DWORD VerticalBlockCompositeMaskLayer3:16;
			};

			DWORD Value;
		} DW08;

		union {
			struct {
				DWORD HorizontalBlockCompositeMaskLayer4:16;
				DWORD VerticalBlockCompositeMaskLayer4:16;
			};

			DWORD Value;
		} DW09;

		union {
			struct {
				DWORD HorizontalBlockCompositeMaskLayer5:16;
				DWORD VerticalBlockCompositeMaskLayer5:16;
			};

			DWORD Value;
		} DW10;

		union {
			struct {
				DWORD HorizontalBlockCompositeMaskLayer6:16;
				DWORD VerticalBlockCompositeMaskLayer6:16;
			};

			DWORD Value;
		} DW11;

		union {
			struct {
				DWORD HorizontalBlockCompositeMaskLayer7:16;
				DWORD VerticalBlockCompositeMaskLayer7:16;
			};

			DWORD Value;
		} DW12;

		union {
			struct {
				DWORD Reserved;
			};

			DWORD Value;
		} DW13;

		union {
			struct {
				DWORD Reserved;
			};

			DWORD Value;
		} DW14;

		union {
			struct {
				DWORD Reserved;
			};

			DWORD Value;
		} DW15;
	} MEDIA_OBJECT_KA2_INLINE_DATA_G575,
	    *PMEDIA_OBJECT_KA2_INLINE_DATA_G575;

	typedef struct _MEDIA_OBJECT_HEADER_G6 {
		union {
			struct {
				DWORD DWordLength:16;
				DWORD CommandSubOpcode:8;
				DWORD CommandOpcode:3;
				DWORD CommandPipeLine:2;
				DWORD CommandType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD InterfaceDescriptorOffset:6;
				 DWORD:2;
				DWORD ObjectID:24;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD IndirectDataLength:17;
				DWORD HalfSliceDestinationSelect:2;
				DWORD SliceDestinationSelect:2;
				DWORD UseScoreboard:1;
				DWORD ForceDestination:1;
				 DWORD:1;
				DWORD ThreadSynchronization:1;
				 DWORD:6;
				DWORD ChildrenPresent:1;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD IndirectDataStartAddress;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD ScoreboardX:9;
				 DWORD:7;
				DWORD ScoreboardY:9;
				 DWORD:7;
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD ScoreboardMask:8;
				 DWORD:8;
				DWORD ScoreboardColor:4;
				 DWORD:12;
			};
			struct {
				DWORD Value;
			};
		} DW5;
	} MEDIA_OBJECT_HEADER_G6, *PMEDIA_OBJECT_HEADER_G6;

	typedef struct _MEDIA_OBJECT_FC_CMD_G6 {
		MEDIA_OBJECT_HEADER_G6 Header;
		MEDIA_OBJECT_KA2_INLINE_DATA_G575 InlineData;
	} MEDIA_OBJECT_FC_CMD_G6, *PMEDIA_OBJECT_FC_CMD_G6;

	typedef struct _BINDING_TABLE_STATE_G5 {
		union {
			struct {
				DWORD Enable:1;
				DWORD Copy:1;
				DWORD BindingTableStateType:1;
				 DWORD:2;
				DWORD SurfaceStatePointer:27;
			};
			struct {
				DWORD Value;
			};
		} DW0;
	} BINDING_TABLE_STATE_G5, *PBINDING_TABLE_STATE_G5;

	typedef struct _BINDING_TABLE_STATE_G8 {
		union {
			struct {
				DWORD Enable:BITFIELD_BIT(0);
				DWORD Copy:BITFIELD_BIT(1);
				DWORD BindingTableStateType:BITFIELD_BIT(2);
				 DWORD:BITFIELD_RANGE(3, 5);
				DWORD SurfaceStatePointer:BITFIELD_RANGE(6, 31);
			};
			struct {
				DWORD Value;
			};
		} DW0;
	} BINDING_TABLE_STATE_G8, *PBINDING_TABLE_STATE_G8;

	typedef struct _MI_BATCH_BUFFER_END_CMD_G5 {
		union {
			struct {
				DWORD:23;
				DWORD InstructionOpcode:6;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;
	} MI_BATCH_BUFFER_END_CMD_G5, *PMI_BATCH_BUFFER_END_CMD_G5;

	typedef struct _PIPELINE_SELECT_CMD_G5 {
		union {
			struct {
				DWORD PipelineSelect:2;
				 DWORD:14;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionPipeline:2;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;
	} PIPELINE_SELECT_CMD_G5, *PPIPELINE_SELECT_CMD_G5;

	typedef struct _MI_NOOP_CMD_G5 {
		union {
			struct {
				DWORD IdentificationNumber:22;
				DWORD IdentificationWriteEnable:1;
				DWORD InstructionOpcode:6;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;
	} MI_NOOP_CMD_G5, *PMI_NOOP_CMD_G5;

	typedef struct _ALLOC {
		struct {
			DWORD AllocationIndex:16;
			 DWORD:13;
			DWORD UpperBoundEnable:1;
			DWORD RenderTargetEnable:1;
			DWORD AllocationEnable:1;
		} DW0;

		DWORD AllocationOffset;
	} ALLOC, *PALLOC;

	typedef struct _BINDING_TABLE_STATE_TOKEN_G6 {
		union {
			struct {
				DWORD Length:8;
				 DWORD:8;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionPipeLine:2;
				DWORD InstructionType:2;
				DWORD Token:1;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD BindingTableHeapOffset:16;
				DWORD NumBindingTableEntries:16;
			};
			struct {
				DWORD Value;
			};
		} DW1;
	} BINDING_TABLE_STATE_TOKEN_G6, *PBINDING_TABLE_STATE_TOKEN_G6;

	typedef struct _SURFACE_STATE_TOKEN_G75 {
		union {
			struct {
				DWORD Length:8;
				 DWORD:8;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionPipeLine:2;
				DWORD InstructionType:2;
				DWORD Token:1;
			};

			struct {
				DWORD DriverID;
			};

			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD SurfaceStateHeapOffset:16;
				DWORD SurfaceAllocationIndex:16;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD SurfaceOffset:32;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD RenderTargetEnable:1;
				DWORD YUVPlane:2;
				DWORD SurfaceStateType:1;
				 DWORD:28;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD SurfaceBaseAddress;
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD SurfaceBaseAddress64:BITFIELD_RANGE(0,
									  15);
				 DWORD:BITFIELD_RANGE(16, 31);
			};
			struct {
				DWORD Value;
			};
		} DW5;
	} SURFACE_STATE_TOKEN_G75, *PSURFACE_STATE_TOKEN_G75;

	typedef struct _MI_LOAD_REGISTER_IMM_CMD_G6 {
		union {
			struct {
				DWORD Length:6;
				 DWORD:2;
				DWORD ByteWriteDisables:4;
				 DWORD:11;
				DWORD InstructionOpcode:6;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD:2;
				DWORD RegisterAddress:30;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD DataDword:32;
			};
			struct {
				DWORD Value;
			};
		} DW2;
	} MI_LOAD_REGISTER_IMM_CMD_G6, *PMI_LOAD_REGISTER_IMM_CMD_G6;

	typedef struct _MI_SET_PREDICATE_CMD_G75 {
		union {
			struct {
				DWORD Enable:2;
				 DWORD:21;
				DWORD CommandOpcode:6;
				DWORD CommandType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;
	} MI_SET_PREDICATE_CMD_G75, *PMI_SET_PREDICATE_CMD_G75;

	typedef struct _PIPE_CONTROL_CMD_G6 {
		union {
			struct {
				DWORD DWordLength:8;
				 DWORD:8;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionSubType:2;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD DepthCacheFlushEnable:1;
				DWORD StallAtPixelScoreboard:1;
				DWORD StateCacheInvalidationEnable:1;
				DWORD ConstantCacheInvalidationEnable:1;
				DWORD VFCacheInvalidationEnable:1;
				 DWORD:1;
				DWORD ProtectedMemoryApplicationID:1;
				 DWORD:1;
				DWORD NotifyEnable:1;
				DWORD IndirectStatePointersDisable:1;
				DWORD TextureCacheInvalidationEnable:1;
				DWORD InstructionCacheInvalidateEnable:1;
				DWORD RenderTargetCacheFlushEnable:1;
				DWORD DepthStallEnable:1;
				DWORD PostSyncOperation:2;
				DWORD GenericMediaStateClear:1;
				DWORD SynchronizeGFDTSurface:1;
				DWORD TLBInvalidate:1;
				DWORD GlobalSnapshotCountReset:1;
				DWORD CSStall:1;
				DWORD StoreDataIndex:1;
				DWORD ProtectedMemoryEnable:1;
				DWORD LRIPostSync:1;
				DWORD DestinationAddressType:1;
				DWORD CoreModeEnable:1;
				 DWORD:6;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD:2;
				DWORD DestinationAddressType:1;
				DWORD Address:29;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD ImmediateData;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD ImmediateData;
			};
			struct {
				DWORD Value;
			};
		} DW4;
	} PIPE_CONTROL_CMD_G6, *PPIPE_CONTROL_CMD_G6;

	typedef struct _MI_BATCH_BUFFER_START_CMD_G75 {
		union {
			struct {
				DWORD Length:8;
				DWORD AddressSpaceIndicator:1;
				 DWORD:1;
				DWORD ResourceStreamerEnable:1;
				DWORD ClearCommandBufferEnable:1;
				DWORD CodedCsMemoryReadEnable:1;
				DWORD NonPrivileged:1;
				 DWORD:1;
				DWORD PredictionEnable:1;
				DWORD AddOffsetEnable:1;
				 DWORD:5;
				DWORD SecondLevelBatchBuffer:1;
				DWORD InstructionOpcode:6;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD:2;
				DWORD BufferStartAddress:30;
			};
			struct {
				DWORD Value;
			};
		} DW1;
	} MI_BATCH_BUFFER_START_CMD_G75, *PMI_BATCH_BUFFER_START_CMD_G75;

	typedef struct _MI_BATCH_BUFFER_START_CMD_G8 {
		union {
			struct {
				DWORD Length:BITFIELD_RANGE(0, 7);
				DWORD AddressSpaceIndicator:BITFIELD_BIT(8);
				 DWORD:BITFIELD_BIT(9);
				DWORD ResourceStreamerEnable:BITFIELD_BIT(10);
				 DWORD:BITFIELD_RANGE(11, 14);
				DWORD PredictionEnable:BITFIELD_BIT(15);
				DWORD AddOffsetEnable:BITFIELD_BIT(16);
				 DWORD:BITFIELD_RANGE(17, 21);
				DWORD SecondLevelBatchBuffer:BITFIELD_BIT(22);
				DWORD InstructionOpcode:BITFIELD_RANGE(23, 28);
				DWORD InstructionType:BITFIELD_RANGE(29, 31);
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD:BITFIELD_RANGE(0, 1);
				DWORD BufferStartAddress:BITFIELD_RANGE(2, 31);
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD BufferStartAddress64:BITFIELD_RANGE(0,
									  15);
				 DWORD:BITFIELD_RANGE(16, 31);
			};
			struct {
				DWORD Value;
			};
		} DW2;

	} MI_BATCH_BUFFER_START_CMD_G8, *PMI_BATCH_BUFFER_START_CMD_G8;

	 C_ASSERT(SIZE32(MI_BATCH_BUFFER_START_CMD_G8) == 3);

	typedef struct _STATE_BASE_ADDRESS_CMD_G6 {
		union {
			struct {
				DWORD Length:8;
				 DWORD:8;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionPipeline:2;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:2;
				DWORD StatelessDPAccessForceWriteThru:1;
				DWORD StatelessDPMemObjCtrlState:4;
				DWORD GeneralStateMemObjCtrlState:4;
				DWORD GeneralStateBaseAddress:20;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:7;
				DWORD SurfaceStateMemObjCtrlState:4;
				DWORD SurfaceStateBaseAddress:20;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:7;
				DWORD DynamicStateMemObjCtrlState:4;
				DWORD DynamicStateBaseAddress:20;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:7;
				DWORD IndirectObjectMemObjCtrlState:4;
				DWORD IndirectObjectBaseAddress:20;
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:7;
				DWORD InstructionMemObjCtrlState:4;
				DWORD InstructionBaseAddress:20;
			};
			struct {
				DWORD Value;
			};

		} DW5;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:11;
				DWORD GeneralStateAccessUpperBound:20;
			};
			struct {
				DWORD Value;
			};
		} DW6;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:11;
				DWORD DynamicStateAccessUpperBound:20;
			};
			struct {
				DWORD Value;
			};
		} DW7;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:11;
				DWORD IndirectObjectAccessUpperBound:20;
			};
			struct {
				DWORD Value;
			};
		} DW8;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:11;
				DWORD InstructionAccessUpperBound:20;
			};
			struct {
				DWORD Value;
			};
		} DW9;
	} STATE_BASE_ADDRESS_CMD_G6, *PSTATE_BASE_ADDRESS_CMD_G6;

	typedef struct _SURFACE_STATE_G6 {
		union {
			struct {
				DWORD CubeFaceEnablesPositiveZ:1;
				DWORD CubeFaceEnablesNegativeZ:1;
				DWORD CubeFaceEnablesPositiveY:1;
				DWORD CubeFaceEnablesNegativeY:1;
				DWORD CubeFaceEnablesPositiveX:1;
				DWORD CubeFaceEnablesNegativeX:1;
				DWORD MediaBoundaryPixelMode:2;
				DWORD RenderCacheReadWriteMode:1;
				DWORD CubeMapCornerMode:1;
				DWORD MipMapLayoutMode:1;
				DWORD VerticalLineStrideOffset:1;
				DWORD VerticalLineStride:1;
				DWORD ColorBlendEnable:1;
				DWORD ColorBufferBlueWriteDisable:1;
				DWORD ColorBufferGreenWriteDisable:1;
				DWORD ColorBufferRedWriteDisable:1;
				DWORD ColorBufferAlphaWriteDisable:1;
				DWORD SurfaceFormat:9;
				DWORD DataReturnFormat:1;
				 DWORD:1;
				DWORD SurfaceType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD SurfaceBaseAddress;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD RenderTargetRotation:2;
				DWORD MipCount:4;
				DWORD Width:13;
				DWORD Height:13;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD TileWalk:1;
				DWORD TiledSurface:1;
				 DWORD:1;
				DWORD SurfacePitch:17;
				 DWORD:1;
				DWORD Depth:11;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD MultisamplePositionPaletteIndex:3;
				 DWORD:1;
				DWORD NumberofMultisamples:3;
				 DWORD:1;
				DWORD RenderTargetViewExtent:9;
				DWORD MinimumArrayElement:11;
				DWORD SurfaceMinL:4;
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD:16;
				DWORD CacheabilityControl:2;
				DWORD GraphicsDataType:1;
				DWORD CodedData:1;
				DWORD YOffset:4;
				DWORD SurfaceVerticalAlignment:1;
				DWORD XOffset:7;
			};
			struct {
				DWORD Value;
			};
		} DW5;

		DWORD dwPad[2];
	} SURFACE_STATE_G6, *PSURFACE_STATE_G6;

	typedef struct _MEDIA_VFE_STATE_CMD_G6 {
		union {
			struct {
				DWORD Length:16;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionPipeline:2;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD PerThreadScratchSpace:4;
				 DWORD:6;
				DWORD ScratchSpaceBasePointer:22;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD DebugCounterControl:2;
				DWORD GPGPUMode:1;
				DWORD GatewayMMIOAccessControl:2;
				DWORD FastPreempt:1;
				DWORD BypassGatewayControl:1;
				DWORD ResetGatewayTimer:1;
				DWORD NumberofURBEntries:8;
				DWORD MaximumNumberofThreads:16;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD:8;
				DWORD DebugObjectID:24;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD CURBEAllocationSize:16;
				DWORD URBEntryAllocationSize:16;
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD ScoreboardMask:8;
				 DWORD:22;
				DWORD ScoreboardType:1;
				DWORD ScoreboardEnable:1;
			};
			struct {
				DWORD Value;
			};
		} DW5;

		union {
			struct {
				DWORD Scoreboard0DeltaX:4;
				DWORD Scoreboard0DeltaY:4;
				DWORD Scoreboard1DeltaX:4;
				DWORD Scoreboard1DeltaY:4;
				DWORD Scoreboard2DeltaX:4;
				DWORD Scoreboard2DeltaY:4;
				DWORD Scoreboard3DeltaX:4;
				DWORD Scoreboard3DeltaY:4;
			};
			struct {
				DWORD Value;
			};
		} DW6;

		union {
			struct {
				DWORD Scoreboard4DeltaX:4;
				DWORD Scoreboard4DeltaY:4;
				DWORD Scoreboard5DeltaX:4;
				DWORD Scoreboard5DeltaY:4;
				DWORD Scoreboard6DeltaX:4;
				DWORD Scoreboard6DeltaY:4;
				DWORD Scoreboard7DeltaX:4;
				DWORD Scoreboard7DeltaY:4;
			};
			struct {
				DWORD Value;
			};
		} DW7;
	} MEDIA_VFE_STATE_CMD_G6, *PMEDIA_VFE_STATE_CMD_G6;

	typedef struct _MEDIA_VFE_STATE_CMD_G8 {
		union {
			struct {
				DWORD Length:BITFIELD_RANGE(0, 15);
				DWORD InstructionSubOpcode:BITFIELD_RANGE(16,
									  23);
				DWORD InstructionOpcode:BITFIELD_RANGE(24, 26);
				DWORD InstructionPipeline:BITFIELD_RANGE(27,
									 28);
				DWORD InstructionType:BITFIELD_RANGE(29, 31);
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD PerThreadScratchSpace:BITFIELD_RANGE(0,
									   3);
				DWORD StackSize:BITFIELD_RANGE(4, 7);
				 DWORD:BITFIELD_RANGE(8, 9);
				DWORD ScratchSpaceBasePointer:BITFIELD_RANGE(10,
									     31);
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD
				    ScratchSpaceBasePointer64:BITFIELD_RANGE(0,
									     15);
				DWORD:BITFIELD_RANGE(16, 31);
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD DebugCounterControl:BITFIELD_RANGE(0, 1);
				DWORD GPGPUMode:BITFIELD_BIT(2);
				DWORD GatewayMMIOAccessControl:BITFIELD_RANGE(3,
									      4);
				DWORD FastPreempt:BITFIELD_BIT(5);
				DWORD BypassGatewayControl:BITFIELD_BIT(6);
				DWORD ResetGatewayTimer:BITFIELD_BIT(7);
				DWORD NumberofURBEntries:BITFIELD_RANGE(8, 15);
				DWORD MaximumNumberofThreads:BITFIELD_RANGE(16,
									    31);
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD SliceDisable:BITFIELD_RANGE(0, 1);
				 DWORD:BITFIELD_RANGE(2, 7);
				DWORD DebugObjectID:BITFIELD_RANGE(8, 31);
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD CURBEAllocationSize:BITFIELD_RANGE(0, 15);
				DWORD URBEntryAllocationSize:BITFIELD_RANGE(16,
									    31);
			};
			struct {
				DWORD Value;
			};
		} DW5;

		union {
			struct {
				DWORD ScoreboardMask:BITFIELD_RANGE(0, 7);
				 DWORD:BITFIELD_RANGE(8, 29);
				DWORD ScoreboardType:BITFIELD_BIT(30);
				DWORD ScoreboardEnable:BITFIELD_BIT(31);
			};
			struct {
				DWORD Value;
			};
		} DW6;

		union {
			struct {
				DWORD Scoreboard0DeltaX:BITFIELD_RANGE(0, 3);
				DWORD Scoreboard0DeltaY:BITFIELD_RANGE(4, 7);
				DWORD Scoreboard1DeltaX:BITFIELD_RANGE(8, 11);
				DWORD Scoreboard1DeltaY:BITFIELD_RANGE(12, 15);
				DWORD Scoreboard2DeltaX:BITFIELD_RANGE(16, 19);
				DWORD Scoreboard2DeltaY:BITFIELD_RANGE(20, 23);
				DWORD Scoreboard3DeltaX:BITFIELD_RANGE(24, 27);
				DWORD Scoreboard3DeltaY:BITFIELD_RANGE(28, 31);
			};
			struct {
				DWORD Value;
			};
		} DW7;

		union {
			struct {
				DWORD Scoreboard4DeltaX:BITFIELD_RANGE(0, 3);
				DWORD Scoreboard4DeltaY:BITFIELD_RANGE(4, 7);
				DWORD Scoreboard5DeltaX:BITFIELD_RANGE(8, 11);
				DWORD Scoreboard5DeltaY:BITFIELD_RANGE(12, 15);
				DWORD Scoreboard6DeltaX:BITFIELD_RANGE(16, 19);
				DWORD Scoreboard6DeltaY:BITFIELD_RANGE(20, 23);
				DWORD Scoreboard7DeltaX:BITFIELD_RANGE(24, 27);
				DWORD Scoreboard7DeltaY:BITFIELD_RANGE(28, 31);
			};
			struct {
				DWORD Value;
			};
		} DW8;

	} MEDIA_VFE_STATE_CMD_G8, *PMEDIA_VFE_STATE_CMD_G8;

	 C_ASSERT(SIZE32(MEDIA_VFE_STATE_CMD_G8) == 9);

	typedef struct _MEDIA_CURBE_LOAD_CMD_G6 {
		union {
			struct {
				DWORD Length:16;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionPipeline:2;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD Reserved:32;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD CURBETotalDataLength:17;
				 DWORD:15;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD CURBEDataStartAddress:32;
			};
			struct {
				DWORD Value;
			};
		} DW3;
	} MEDIA_CURBE_LOAD_CMD_G6, *PMEDIA_CURBE_LOAD_CMD_G6;

	typedef struct _MEDIA_INTERFACE_DESCRIPTOR_LOAD_CMD_G6 {
		union {
			struct {
				DWORD Length:16;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionPipeline:2;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD Reserved:32;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD InterfaceDescriptorLength:17;
				 DWORD:15;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD InterfaceDescriptorStartAddress:32;
			};
			struct {
				DWORD Value;
			};
		} DW3;
	} MEDIA_INTERFACE_DESCRIPTOR_LOAD_CMD_G6,
	    *PMEDIA_INTERFACE_DESCRIPTOR_LOAD_CMD_G6;

	typedef struct _INTERFACE_DESCRIPTOR_DATA_G6 {
		union {
			struct {
				DWORD:6;
				DWORD KernelStartPointer:26;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD:7;
				DWORD SoftwareExceptionEnable:1;
				 DWORD:3;
				DWORD MaskStackExceptionEnable:1;
				 DWORD:1;
				DWORD IllegalOpcodeExceptionEnable:1;
				 DWORD:2;
				DWORD FloatingPointMode:1;
				DWORD ThreadPriority:1;
				DWORD SingleProgramFlow:1;
				 DWORD:13;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD:2;
				DWORD SamplerCount:3;
				DWORD SamplerStatePointer:27;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD BindingTableEntryCount:5;
				DWORD BindingTablePointer:27;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD ConstantURBEntryReadOffset:16;
				DWORD ConstantURBEntryReadLength:16;
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD NumberofThreadsInGPGPUGroup:8;
				DWORD BarrierReturnByte:8;
				DWORD SharedLocalMemorySize:5;
				DWORD BarrierEnable:1;
				DWORD RoundingMode:2;
				DWORD BarrierReturnGRFOffset:8;
			};
			struct {
				DWORD Value;
			};
		} DW5;

		union {
			struct {
				DWORD CrsThdConDataRdLn:8;
				DWORD Reserved:24;
			};
			struct {
				DWORD Value;
			};
		} DW6;

		union {
			struct {
				DWORD Reserved:32;
			};
			struct {
				DWORD Value;
			};
		} DW7;
	} INTERFACE_DESCRIPTOR_DATA_G6, *PINTERFACE_DESCRIPTOR_DATA_G6;

	typedef struct _INTERFACE_DESCRIPTOR_DATA_G8 {
		union {
			struct {
				DWORD:BITFIELD_RANGE(0, 5);
				DWORD KernelStartPointer:BITFIELD_RANGE(6, 31);
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD KernelStartPointer64:BITFIELD_RANGE(0,
									  15);
				 DWORD:BITFIELD_RANGE(16, 31);
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD:BITFIELD_RANGE(0, 6);
				DWORD SoftwareExceptionEnable:BITFIELD_BIT(7);
				 DWORD:BITFIELD_RANGE(8, 10);
				DWORD MaskStackExceptionEnable:BITFIELD_BIT(11);
				 DWORD:BITFIELD_BIT(12);
				 DWORD
				    IllegalOpcodeExceptionEnable:BITFIELD_BIT
				    (13);
				 DWORD:BITFIELD_RANGE(14, 15);
				DWORD FloatingPointMode:BITFIELD_BIT(16);
				DWORD ThreadPriority:BITFIELD_BIT(17);
				DWORD SingleProgramFlow:BITFIELD_BIT(18);
				DWORD DenormMode:BITFIELD_BIT(19);
				 DWORD:BITFIELD_RANGE(20, 31);
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD:BITFIELD_RANGE(0, 1);
				DWORD SamplerCount:BITFIELD_RANGE(2, 4);
				DWORD SamplerStatePointer:BITFIELD_RANGE(5, 31);
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD BindingTableEntryCount:BITFIELD_RANGE(0,
									    4);
				DWORD BindingTablePointer:BITFIELD_RANGE(5, 15);
				 DWORD:BITFIELD_RANGE(16, 31);
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD
				    ConstantURBEntryReadOffset:BITFIELD_RANGE(0,
									      15);
				DWORD
				    ConstantURBEntryReadLength:BITFIELD_RANGE
				    (16, 31);
			};
			struct {
				DWORD Value;
			};
		} DW5;

		union {
			struct {

				DWORD
				    NumberofThreadsInGPGPUGroup:BITFIELD_RANGE
				    (0, 9);
				DWORD:BITFIELD_RANGE(10, 14);
				DWORD GlobalBarrierEnable:BITFIELD_BIT(15);
				DWORD SharedLocalMemorySize:BITFIELD_RANGE(16,
									   20);
				DWORD BarrierEnable:BITFIELD_BIT(21);
				DWORD RoundingMode:BITFIELD_RANGE(22, 23);
				 DWORD:BITFIELD_RANGE(24, 31);
			};
			struct {
				DWORD Value;
			};
		} DW6;

		union {
			struct {
				DWORD CrsThdConDataRdLn:BITFIELD_RANGE(0, 7);
				 DWORD:BITFIELD_RANGE(8, 31);
			};
			struct {
				DWORD Value;
			};
		} DW7;
	} INTERFACE_DESCRIPTOR_DATA_G8, *PINTERFACE_DESCRIPTOR_DATA_G8;

	 C_ASSERT(SIZE32(INTERFACE_DESCRIPTOR_DATA_G8) == 8);

	typedef struct _MEDIA_OBJECT_WALKER_CMD_G6 {
		union {
			struct {
				DWORD Length:16;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionPipeline:2;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD InterfaceDescriptorOffset:6;
				 DWORD:2;
				DWORD ObjectID:24;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD IndirectDataLength:17;
				 DWORD:4;
				DWORD UseScoreboard:1;
				 DWORD:2;
				DWORD ThreadSynchronization:1;
				 DWORD:6;
				DWORD ChildrenPresent:1;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD IndirectDataStartAddress;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD Reserved:32;
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD ScoreboardMask:8;
				DWORD GroupIdLoopSelect:24;
			};
			struct {
				DWORD Value;
			};
		} DW5;

		union {
			struct {
				DWORD:8;
				DWORD MidLoopUnitX:2;
				 DWORD:2;
				DWORD MidLoopUnitY:2;
				 DWORD:2;
				DWORD MidLoopExtraSteps:5;
				 DWORD:3;
				DWORD ColorCountMinusOne:4;
				 DWORD:1;
				DWORD QuadMode:1;
				DWORD Repel:1;
				DWORD DualMode:1;
			};
			struct {
				DWORD Value;
			};
		} DW6;

		union {
			struct {
				DWORD LocalLoopExecCount:10;
				 DWORD:6;
				DWORD GlobalLoopExecCount:10;
				 DWORD:6;
			};
			struct {
				DWORD Value;
			};
		} DW7;

		union {
			struct {
				DWORD BlockResolutionX:9;
				 DWORD:7;
				DWORD BlockResolutionY:9;
				 DWORD:7;
			};
			struct {
				DWORD Value;
			};
		} DW8;

		union {
			struct {
				DWORD LocalStartX:9;
				 DWORD:7;
				DWORD LocalStartY:9;
				 DWORD:7;
			};
			struct {
				DWORD Value;
			};
		} DW9;

		union {
			struct {
				DWORD LocalEndX:9;
				 DWORD:7;
				DWORD LocalEndY:9;
				 DWORD:7;
			};
			struct {
				DWORD Value;
			};
		} DW10;

		union {
			struct {
				DWORD LocalOuterLoopStrideX:10;
				 DWORD:6;
				DWORD LocalOuterLoopStrideY:10;
				 DWORD:6;
			};
			struct {
				DWORD Value;
			};
		} DW11;

		union {
			struct {
				DWORD LocalInnerLoopUnitX:10;
				 DWORD:6;
				DWORD LocalInnerLoopUnitY:10;
				 DWORD:6;
			};
			struct {
				DWORD Value;
			};
		} DW12;

		union {
			struct {
				DWORD GlobalResolutionX:9;
				 DWORD:7;
				DWORD GlobalResolutionY:9;
				 DWORD:7;
			};
			struct {
				DWORD Value;
			};
		} DW13;

		union {
			struct {
				DWORD GlobalStartX:10;
				 DWORD:6;
				DWORD GlobalStartY:10;
				 DWORD:6;
			};
			struct {
				DWORD Value;
			};
		} DW14;

		union {
			struct {
				DWORD GlobalOuterLoopStrideX:10;
				 DWORD:6;
				DWORD GlobalOuterLoopStrideY:10;
				 DWORD:6;
			};
			struct {
				DWORD Value;
			};
		} DW15;

		union {
			struct {
				DWORD GlobalInnerLoopUnitX:10;
				 DWORD:6;
				DWORD GlobalInnerLoopUnitY:10;
				 DWORD:6;
			};
			struct {
				DWORD Value;
			};
		} DW16;
	} MEDIA_OBJECT_WALKER_CMD_G6, *PMEDIA_OBJECT_WALKER_CMD_G6;

	typedef struct _GPGPU_WALKER_CMD_G75 {
		union {
			struct {
				DWORD Length:8;
				DWORD PredicateEnable:1;
				 DWORD:1;
				DWORD IndirectParameterEnable:1;
				 DWORD:5;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionPipeline:2;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD InterfaceDescriptorOffset:5;
				 DWORD:3;
				DWORD ObjectID:24;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD ThreadWidthCounterMax:6;
				 DWORD:2;
				DWORD ThreadHeightCounterMax:6;
				 DWORD:2;
				DWORD ThreadDepthCounterMax:6;
				 DWORD:8;
				DWORD SIMDSize:2;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD ThreadGroupIDStartingX;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD ThreadGroupIDDimensionX;
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD ThreadGroupIDStartingY;
			};
			struct {
				DWORD Value;
			};
		} DW5;

		union {
			struct {
				DWORD ThreadGroupIDDimensionY;
			};
			struct {
				DWORD Value;
			};
		} DW6;

		union {
			struct {
				DWORD ThreadGroupIDStartingZ;
			};
			struct {
				DWORD Value;
			};
		} DW7;

		union {
			struct {
				DWORD ThreadGroupIDDimensionZ;
			};
			struct {
				DWORD Value;
			};
		} DW8;

		union {
			struct {
				DWORD RightExecutionMask;
			};
			struct {
				DWORD Value;
			};
		} DW9;

		union {
			struct {
				DWORD BottomExecutionMask;
			};
			struct {
				DWORD Value;
			};
		} DW10;
	} GPGPU_WALKER_CMD_G75, *PGPGPU_WALKER_CMD_G75;

	typedef struct _GPGPU_WALKER_CMD_G8 {
		union {
			struct {
				DWORD Length:8;
				DWORD PredicateEnable:1;
				 DWORD:1;
				DWORD IndirectParameterEnable:1;
				 DWORD:5;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionPipeline:2;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD InterfaceDescriptorOffset:5;
				 DWORD:3;
				DWORD ObjectID:24;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD IndirectDataLength:17;
				 DWORD:15;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD IndirectDataStartAddress;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD ThreadWidthCounterMax:6;
				 DWORD:2;
				DWORD ThreadHeightCounterMax:6;
				 DWORD:2;
				DWORD ThreadDepthCounterMax:6;
				 DWORD:8;
				DWORD SIMDSize:2;
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD ThreadGroupIDStartingX;
			};
			struct {
				DWORD Value;
			};
		} DW5;

		union {
			struct {
				DWORD Reserved;
			};
			struct {
				DWORD Value;
			};
		} DW6;

		union {
			struct {
				DWORD ThreadGroupIDDimensionX;
			};
			struct {
				DWORD Value;
			};
		} DW7;

		union {
			struct {
				DWORD ThreadGroupIDStartingY;
			};
			struct {
				DWORD Value;
			};
		} DW8;

		union {
			struct {
				DWORD Reserved;
			};
			struct {
				DWORD Value;
			};
		} DW9;

		union {
			struct {
				DWORD ThreadGroupIDDimensionY;
			};
			struct {
				DWORD Value;
			};
		} DW10;

		union {
			struct {
				DWORD ThreadGroupIDStartingZ;
			};
			struct {
				DWORD Value;
			};
		} DW11;

		union {
			struct {
				DWORD ThreadGroupIDDimensionZ;
			};
			struct {
				DWORD Value;
			};
		} DW12;

		union {
			struct {
				DWORD RightExecutionMask;
			};
			struct {
				DWORD Value;
			};
		} DW13;

		union {
			struct {
				DWORD BottomExecutionMask;
			};
			struct {
				DWORD Value;
			};
		} DW14;
	} GPGPU_WALKER_CMD_G8, *PGPGPU_WALKER_CMD_G8;

	typedef struct _MI_ARB_CHECK_CMD_G75 {
		union _DW0 {
			struct _BitField {
				DWORD Reserved:BITFIELD_RANGE(0, 22);
				DWORD InstructionOpcode:BITFIELD_RANGE(23, 28);
				DWORD InstructionType:BITFIELD_RANGE(29, 31);
			} BitField;

			DWORD Value;
		} DW0;
	} MI_ARB_CHECK_CMD_G75, *PMI_ARB_CHECK_CMD_G75;

	typedef struct _MEDIA_STATE_FLUSH_CMD_G75 {
		union {
			struct {
				DWORD Length:16;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionPipeline:2;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD InterfaceDescriptorOffset:6;
				DWORD WatermarkRequired:1;
				DWORD FlushToGo:1;
				DWORD DisablePreemption:1;
				 DWORD:23;
			};
			struct {
				DWORD Value;
			};
		} DW1;
	} MEDIA_STATE_FLUSH_CMD_G75, *PMEDIA_STATE_FLUSH_CMD_G75;

	typedef struct _PIPE_CONTROL_CMD_G7 {
		union {
			struct {
				DWORD DWordLength:8;
				 DWORD:8;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionSubType:2;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD DepthCacheFlushEnable:1;
				DWORD StallAtPixelScoreboard:1;
				DWORD StateCacheInvalidationEnable:1;
				DWORD ConstantCacheInvalidationEnable:1;
				DWORD VFCacheInvalidationEnable:1;
				DWORD DCFlushEnable:1;
				DWORD ProtectedMemoryApplicationID:1;
				DWORD PIPE_CONTROLFlushEnable:1;
				DWORD NotifyEnable:1;
				DWORD IndirectStatePointersDisable:1;
				DWORD TextureCacheInvalidationEnable:1;
				DWORD InstructionCacheInvalidateEnable:1;
				DWORD RenderTargetCacheFlushEnable:1;
				DWORD DepthStallEnable:1;
				DWORD PostSyncOperation:2;
				DWORD GenericMediaStateClear:1;
				DWORD SynchronizeGFDTSurface:1;
				DWORD TLBInvalidate:1;
				DWORD GlobalSnapshotCountReset:1;
				DWORD CSStall:1;
				DWORD StoreDataIndex:1;
				DWORD ProtectedMemoryEnable:1;
				DWORD LRIPostSyncOperation:1;
				DWORD DestinationAddressType:1;
				DWORD CoreModeEnable:1;
				 DWORD:6;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD:2;
				DWORD Address:30;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD ImmediateData;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD ImmediateData;
			};
			struct {
				DWORD Value;
			};
		} DW4;
	} PIPE_CONTROL_CMD_G7, *PPIPE_CONTROL_CMD_G7;

	typedef struct _PIPE_CONTROL_CMD_G8 {
		union {
			struct {
				DWORD DWordLength:8;
				 DWORD:8;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionSubType:2;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD DepthCacheFlushEnable:1;
				DWORD StallAtPixelScoreboard:1;
				DWORD StateCacheInvalidationEnable:1;
				DWORD ConstantCacheInvalidationEnable:1;
				DWORD VFCacheInvalidationEnable:1;
				DWORD DCFlushEnable:1;
				DWORD ProtectedMemoryApplicationID:1;
				DWORD PIPE_CONTROLFlushEnable:1;
				DWORD NotifyEnable:1;
				DWORD IndirectStatePointersDisable:1;
				DWORD TextureCacheInvalidationEnable:1;
				DWORD InstructionCacheInvalidateEnable:1;
				DWORD RenderTargetCacheFlushEnable:1;
				DWORD DepthStallEnable:1;
				DWORD PostSyncOperation:2;
				DWORD GenericMediaStateClear:1;
				DWORD SynchronizeGFDTSurface:1;
				DWORD TLBInvalidate:1;
				DWORD GlobalSnapshotCountReset:1;
				DWORD CSStall:1;
				DWORD StoreDataIndex:1;
				DWORD ProtectedMemoryEnable:1;
				DWORD LRIPostSyncOperation:1;
				DWORD DestinationAddressType:1;
				DWORD CoreModeEnable:1;
				 DWORD:6;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD:2;
				DWORD Address:30;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD Address64:32;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD ImmediateData;
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD ImmediateData;
			};
			struct {
				DWORD Value;
			};
		} DW5;
	} PIPE_CONTROL_CMD_G8, *PPIPE_CONTROL_CMD_G8;

	typedef struct _SURFACE_STATE_G7 {
		union {
			struct {
				DWORD CubeFaceEnablesPositiveZ:1;
				DWORD CubeFaceEnablesNegativeZ:1;
				DWORD CubeFaceEnablesPositiveY:1;
				DWORD CubeFaceEnablesNegativeY:1;
				DWORD CubeFaceEnablesPositiveX:1;
				DWORD CubeFaceEnablesNegativeX:1;
				DWORD MediaBoundaryPixelMode:2;
				DWORD RenderCacheReadWriteMode:1;
				 DWORD:1;
				DWORD SurfaceArraySpacing:1;
				DWORD VerticalLineStrideOffset:1;
				DWORD VerticalLineStride:1;
				DWORD TileWalk:1;
				DWORD TiledSurface:1;
				DWORD SurfaceHorizontalAlignment:1;
				DWORD SurfaceVerticalAlignment:2;
				DWORD SurfaceFormat:9;
				DWORD MinMagStateNotEqual:1;
				DWORD SurfaceArray:1;
				DWORD SurfaceType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD SurfaceBaseAddress;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD Width:14;
				 DWORD:2;
				DWORD Height:14;
				 DWORD:2;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD SurfacePitch:18;
				 DWORD:3;
				DWORD Depth:11;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD MultiSamplePositionPaletteIndex:3;
				DWORD NumberofMultiSamples:3;
				DWORD MultiSampledSurfaceStorageFormat:1;
				DWORD RenderTargetViewExtent:11;
				DWORD MinimumArrayElement:11;
				DWORD RenderTargetRotation:2;
				 DWORD:1;
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD MipCount:4;
				DWORD SurfaceMinL:4;
				 DWORD:6;
				DWORD CoherencyType:1;
				DWORD StatelessDataPortAccessWriteThru:1;
				DWORD SurfaceObjectControlState:4;
				DWORD YOffset:4;
				 DWORD:1;
				DWORD XOffset:7;
			};
			struct {
				DWORD Value;
			};
		} DW5;

		union {
			struct {
				DWORD YOffsetUVPlane:14;
				 DWORD:2;
				DWORD XOffsetUVPlane:14;
				 DWORD:2;
			};
			struct {
				DWORD MCSEnable:1;
				 DWORD:2;
				DWORD MCSSurfacePitch:9;
				DWORD MCSBaseAddress:20;
			};
			struct {
				DWORD Value;
			};
		} DW6;

		union {
			struct {
				DWORD ResourceMinL:12;
				 DWORD:4;
				DWORD ShaderChannelSelectA:3;
				DWORD ShaderChannelSelectB:3;
				DWORD ShaderChannelSelectG:3;
				DWORD ShaderChannelSelectR:3;
				DWORD AlphaClearColor:1;
				DWORD BlueClearColor:1;
				DWORD GreenClearColor:1;
				DWORD RedClearColor:1;
			};
			struct {
				DWORD Value;
			};
		} DW7;
	} SURFACE_STATE_G7, *PSURFACE_STATE_G7;

	typedef struct _SURFACE_STATE_G8 {
		union {
			struct {
				DWORD CubeFaceEnablesPositiveZ:BITFIELD_BIT(0);
				DWORD CubeFaceEnablesNegativeZ:BITFIELD_BIT(1);
				DWORD CubeFaceEnablesPositiveY:BITFIELD_BIT(2);
				DWORD CubeFaceEnablesNegativeY:BITFIELD_BIT(3);
				DWORD CubeFaceEnablesPositiveX:BITFIELD_BIT(4);
				DWORD CubeFaceEnablesNegativeX:BITFIELD_BIT(5);
				DWORD MediaBoundaryPixelMode:BITFIELD_RANGE(6,
									    7);
				DWORD RenderCacheReadWriteMode:BITFIELD_BIT(8);
				 DWORD:BITFIELD_BIT(9);
				DWORD VerticalLineStrideOffset:BITFIELD_BIT(10);
				DWORD VerticalLineStride:BITFIELD_BIT(11);
				DWORD TileMode:BITFIELD_RANGE(12, 13);
				 DWORD
				    SurfaceHorizontalAlignment:BITFIELD_RANGE
				    (14, 15);
				 DWORD
				    SurfaceVerticalAlignment:BITFIELD_RANGE(16,
									    17);
				DWORD SurfaceFormat:BITFIELD_RANGE(18, 26);
				 DWORD:BITFIELD_BIT(27);
				DWORD SurfaceArray:BITFIELD_BIT(28);
				DWORD SurfaceType:BITFIELD_RANGE(29, 31);
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD SurfaceQPitch:BITFIELD_RANGE(0, 14);
				 DWORD:BITFIELD_RANGE(15, 23);
				DWORD SurfaceMemObjCtrlState:BITFIELD_RANGE(24,
									    30);
				 DWORD:BITFIELD_BIT(31);
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD Width:BITFIELD_RANGE(0, 13);
				 DWORD:BITFIELD_RANGE(14, 15);
				DWORD Height:BITFIELD_RANGE(16, 29);
				 DWORD:BITFIELD_RANGE(30, 31);
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD SurfacePitch:BITFIELD_RANGE(0, 17);
				 DWORD:BITFIELD_RANGE(18, 20);
				DWORD Depth:BITFIELD_RANGE(21, 31);
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD
				    MultiSamplePositionPaletteIndex:BITFIELD_RANGE
				    (0, 2);
				DWORD NumberofMultiSamples:BITFIELD_RANGE(3, 5);
				 DWORD
				    MultiSampledSurfaceStorageFormat:BITFIELD_BIT
				    (6);
				DWORD RenderTargetViewExtent:BITFIELD_RANGE(7,
									    17);
				DWORD MinimumArrayElement:BITFIELD_RANGE(18,
									 28);
				DWORD RenderTargetRotation:BITFIELD_RANGE(29,
									  30);
				 DWORD:BITFIELD_BIT(31);
			};
			struct {
				DWORD MinArrrayElement:BITFIELD_RANGE(0, 26);
				 DWORD:BITFIELD_RANGE(27, 31);
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD MipCount:BITFIELD_RANGE(0, 3);
				DWORD SurfaceMinL:BITFIELD_RANGE(4, 7);
				 DWORD:BITFIELD_RANGE(8, 13);
				DWORD CoherencyType:BITFIELD_BIT(14);
				 DWORD:BITFIELD_RANGE(15, 20);
				DWORD YOffset:BITFIELD_RANGE(21, 23);
				 DWORD:BITFIELD_BIT(24);
				DWORD XOffset:BITFIELD_RANGE(25, 31);
			};
			struct {
				DWORD Value;
			};
		} DW5;

		union {
			struct {
				DWORD AuxSurfaceMode:BITFIELD_RANGE(0, 1);
				 DWORD:BITFIELD_BIT(2);
				DWORD AuxSurfacePitch:BITFIELD_RANGE(3, 11);
				 DWORD:BITFIELD_RANGE(12, 15);
				DWORD AuxSurfaceQPitch:BITFIELD_RANGE(16, 30);
				 DWORD:BITFIELD_BIT(31);
			};
			struct {
				DWORD YOffsetUVPlane:BITFIELD_RANGE(0, 13);
				 DWORD:BITFIELD_RANGE(14, 15);
				DWORD XOffsetUVPlane:BITFIELD_RANGE(16, 29);
				 DWORD:BITFIELD_RANGE(30, 31);
			};
			struct {
				DWORD Value;
			};
		} DW6;

		union {
			struct {
				DWORD ResourceMinL:BITFIELD_RANGE(0, 11);
				 DWORD:BITFIELD_RANGE(12, 15);
				DWORD ShaderChannelSelectA:BITFIELD_RANGE(16,
									  18);
				DWORD ShaderChannelSelectB:BITFIELD_RANGE(19,
									  21);
				DWORD ShaderChannelSelectG:BITFIELD_RANGE(22,
									  24);
				DWORD ShaderChannelSelectR:BITFIELD_RANGE(25,
									  27);
				DWORD AlphaClearColor:BITFIELD_BIT(28);
				DWORD BlueClearColor:BITFIELD_BIT(29);
				DWORD GreenClearColor:BITFIELD_BIT(30);
				DWORD RedClearColor:BITFIELD_BIT(31);
			};
			struct {
				DWORD Value;
			};
		} DW7;

		union {
			struct {
				DWORD SurfaceBaseAddress;
			};
			struct {
				DWORD Value;
			};
		} DW8;

		union {
			struct {
				DWORD SurfaceBaseAddress64:BITFIELD_RANGE(0,
									  15);
				 DWORD:BITFIELD_RANGE(16, 31);
			};
			struct {
				DWORD Value;
			};
		} DW9;

		union {
			struct {
				DWORD:BITFIELD_RANGE(0, 11);
				DWORD AuxSurfaceBaseAddress:BITFIELD_RANGE(12,
									   31);
			};
			struct {
				DWORD Value;
			};
		} DW10;

		union {
			struct {
				DWORD AuxSurfaceBaseAddress64:BITFIELD_RANGE(0,
									     15);
				 DWORD:BITFIELD_RANGE(16, 31);
			};
			struct {
				DWORD Value;
			};
		} DW11;

		union {
			struct {
				DWORD HierarchicalDepthClear;
			};
			struct {
				DWORD Value;
			};
		} DW12;

		DWORD Padding[3];

	} SURFACE_STATE_G8, *PSURFACE_STATE_G8;

	 C_ASSERT(SIZE32(SURFACE_STATE_G8) == 16);

	typedef struct _PACKET_SURFACE_STATE_G75 {
		SURFACE_STATE_TOKEN_G75 Token;

		union {
			SURFACE_STATE_G7 cmdSurfaceState_g75;
		};
	} PACKET_SURFACE_STATE_G75, *PPACKET_SURFACE_STATE_G75;

	typedef struct _PACKET_SURFACE_STATE_G8 {
		SURFACE_STATE_TOKEN_G75 Token;
		SURFACE_STATE_G8 cmdSurfaceState_g8;
	} PACKET_SURFACE_STATE_G8, *PPACKET_SURFACE_STATE_G8;

	typedef struct _STATE_BASE_ADDRESS_CMD_G75 {
		union {
			struct {
				DWORD Length:8;
				 DWORD:8;
				DWORD InstructionSubOpcode:8;
				DWORD InstructionOpcode:3;
				DWORD InstructionPipeline:2;
				DWORD InstructionType:3;
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:3;
				DWORD StatelessDPMemObjCtrlState:4;
				DWORD GeneralStateMemObjCtrlState:4;
				DWORD GeneralStateBaseAddress:20;
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:7;
				DWORD SurfaceStateMemObjCtrlState:4;
				DWORD SurfaceStateBaseAddress:20;
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:7;
				DWORD DynamicStateMemObjCtrlState:4;
				DWORD DynamicStateBaseAddress:20;
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:7;
				DWORD IndirectObjectMemObjCtrlState:4;
				DWORD IndirectObjectBaseAddress:20;
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:7;
				DWORD InstructionMemObjCtrlState:4;
				DWORD InstructionBaseAddress:20;
			};
			struct {
				DWORD Value;
			};

		} DW5;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:11;
				DWORD GeneralStateAccessUpperBound:20;
			};
			struct {
				DWORD Value;
			};
		} DW6;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:11;
				DWORD DynamicStateAccessUpperBound:20;
			};
			struct {
				DWORD Value;
			};
		} DW7;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:11;
				DWORD IndirectObjectAccessUpperBound:20;
			};
			struct {
				DWORD Value;
			};
		} DW8;

		union {
			struct {
				DWORD Modify:1;
				 DWORD:11;
				DWORD InstructionAccessUpperBound:20;
			};
			struct {
				DWORD Value;
			};
		} DW9;

		union {
			struct {
				DWORD LLCCoherentAddressModify:1;
				 DWORD:11;
				DWORD LLCCoherentBaseAddress:20;
			};
			struct {
				DWORD Value;
			};
		} DW10;

		union {
			struct {
				DWORD LLCCoherentUpperBoundModify:1;
				 DWORD:11;
				DWORD LLCCoherentUpperBound:20;
			};
			struct {
				DWORD Value;
			};
		} DW11;
	} STATE_BASE_ADDRESS_CMD_G75, *PSTATE_BASE_ADDRESS_CMD_G75;

	typedef struct _STATE_BASE_ADDRESS_CMD_G8 {
		union {
			struct {
				DWORD Length:BITFIELD_RANGE(0, 7);
				 DWORD:BITFIELD_RANGE(8, 15);
				DWORD InstructionSubOpcode:BITFIELD_RANGE(16,
									  23);
				DWORD InstructionOpcode:BITFIELD_RANGE(24, 26);
				DWORD InstructionSubType:BITFIELD_RANGE(27, 28);
				DWORD InstructionType:BITFIELD_RANGE(29, 31);
			};
			struct {
				DWORD Value;
			};
		} DW0;

		union {
			struct {
				DWORD
				    GeneralStateBaseAddressModify:BITFIELD_BIT
				    (0);
				DWORD:BITFIELD_RANGE(1, 3);
				DWORD
				    GeneralStateMemObjCtrlState:BITFIELD_RANGE
				    (4, 10);
				DWORD:BITFIELD_BIT(11);
				DWORD GeneralStateBaseAddress:BITFIELD_RANGE(12,
									     31);
			};
			struct {
				DWORD Value;
			};
		} DW1;

		union {
			struct {
				DWORD
				    GeneralStateBaseAddress64:BITFIELD_RANGE(0,
									     31);
			};
			struct {
				DWORD Value;
			};
		} DW2;

		union {
			struct {
				DWORD:BITFIELD_RANGE(0, 15);
				DWORD
				    StatelessDPMemObjCtrlState:BITFIELD_RANGE
				    (16, 22);
				DWORD
				    StatelessDPAccessForceWrite:BITFIELD_BIT
				    (23);
				DWORD:BITFIELD_RANGE(24, 31);
			};
			struct {
				DWORD Value;
			};
		} DW3;

		union {
			struct {
				DWORD
				    SurfaceStateBaseAddressModify:BITFIELD_BIT
				    (0);
				DWORD:BITFIELD_RANGE(1, 3);
				DWORD
				    SurfaceStateMemObjCtrlState:BITFIELD_RANGE
				    (4, 10);
				DWORD:BITFIELD_BIT(11);
				DWORD SurfaceStateBaseAddress:BITFIELD_RANGE(12,
									     31);
			};
			struct {
				DWORD Value;
			};
		} DW4;

		union {
			struct {
				DWORD
				    SurfaceStateBaseAddress64:BITFIELD_RANGE(0,
									     31);
			};
			struct {
				DWORD Value;
			};
		} DW5;

		union {
			struct {
				DWORD
				    DynamicStateBaseAddressModify:BITFIELD_BIT
				    (0);
				DWORD:BITFIELD_RANGE(1, 3);
				DWORD
				    DynamicStateMemObjCtrlState:BITFIELD_RANGE
				    (4, 10);
				DWORD:BITFIELD_BIT(11);
				DWORD DynamicStateBaseAddress:BITFIELD_RANGE(12,
									     31);
			};
			struct {
				DWORD Value;
			};
		} DW6;

		union {
			struct {
				DWORD
				    DynamicStateBaseAddress64:BITFIELD_RANGE(0,
									     31);
			};
			struct {
				DWORD Value;
			};
		} DW7;

		union {
			struct {
				DWORD
				    IndirectObjBaseAddressModify:BITFIELD_BIT
				    (0);
				DWORD:BITFIELD_RANGE(1, 3);
				DWORD
				    IndirectObjectMemObjCtrlState:BITFIELD_RANGE
				    (4, 10);
				DWORD:BITFIELD_BIT(11);
				DWORD IndirectObjBaseAddress:BITFIELD_RANGE(12,
									    31);
			};
			struct {
				DWORD Value;
			};
		} DW8;

		union {
			struct {
				DWORD IndirectObjBaseAddress64:BITFIELD_RANGE(0,
									      31);
			};
			struct {
				DWORD Value;
			};
		} DW9;

		union {
			struct {
				DWORD
				    InstructionBaseAddressModify:BITFIELD_BIT
				    (0);
				DWORD:BITFIELD_RANGE(1, 3);
				DWORD
				    InstructionMemObjCtrlState:BITFIELD_RANGE(4,
									      10);
				DWORD:BITFIELD_BIT(11);
				DWORD InstructionBaseAddress:BITFIELD_RANGE(12,
									    31);
			};
			struct {
				DWORD Value;
			};
		} DW10;

		union {
			struct {
				DWORD InstructionBaseAddress64:BITFIELD_RANGE(0,
									      31);
			};
			struct {
				DWORD Value;
			};
		} DW11;

		union {
			struct {
				DWORD
				    GeneralStateBufferSizeModifyEnable:BITFIELD_BIT
				    (0);
				DWORD:BITFIELD_RANGE(1, 11);
				DWORD GeneralStateBufferSize:BITFIELD_RANGE(12,
									    31);
			};
			struct {
				DWORD Value;
			};
		} DW12;

		union {
			struct {
				DWORD
				    DynamicStateBufferSizeModifyEnable:BITFIELD_BIT
				    (0);
				DWORD:BITFIELD_RANGE(1, 11);
				DWORD DynamicStateBufferSize:BITFIELD_RANGE(12,
									    31);
			};
			struct {
				DWORD Value;
			};
		} DW13;

		union {
			struct {
				DWORD
				    IndirectObjBufferSizeModifyEnable:BITFIELD_BIT
				    (0);
				DWORD:BITFIELD_RANGE(1, 11);
				DWORD IndirectObjBufferSize:BITFIELD_RANGE(12,
									   31);
			};
			struct {
				DWORD Value;
			};
		} DW14;

		union {
			struct {
				DWORD
				    InstructionBufferSizeModifyEnable:BITFIELD_BIT
				    (0);
				DWORD:BITFIELD_RANGE(1, 11);
				DWORD InstructionBufferSize:BITFIELD_RANGE(12,
									   31);
			};
			struct {
				DWORD Value;
			};
		} DW15;

	} STATE_BASE_ADDRESS_CMD_G8, *PSTATE_BASE_ADDRESS_CMD_G8;

	 C_ASSERT(SIZE32(STATE_BASE_ADDRESS_CMD_G8) == 16);

	extern CONST MI_BATCH_BUFFER_END_CMD_G5
	    g_cInit_MI_BATCH_BUFFER_END_CMD_G5;

	extern CONST MI_NOOP_CMD_G5 g_cInit_MI_NOOP_CMD_G5;
	extern CONST PIPELINE_SELECT_CMD_G5
	    g_cInit_PIPELINE_SELECT_CMD_MEDIA_G575;

	extern CONST MI_LOAD_REGISTER_IMM_CMD_G6
	    g_cInit_MI_LOAD_REGISTER_IMM_CMD_G6;
	extern CONST PIPE_CONTROL_CMD_G6 g_cInit_PIPE_CONTROL_CMD_G6;
	extern CONST BINDING_TABLE_STATE_G5 g_cInit_BINDING_TABLE_STATE_G5;
	extern CONST STATE_BASE_ADDRESS_CMD_G6
	    g_cInit_STATE_BASE_ADDRESS_CMD_G6;
	extern CONST MEDIA_VFE_STATE_CMD_G6 g_cInit_MEDIA_VFE_STATE_CMD_G6;
	extern CONST MEDIA_CURBE_LOAD_CMD_G6 g_cInit_MEDIA_CURBE_LOAD_CMD_G6;
	extern CONST MEDIA_INTERFACE_DESCRIPTOR_LOAD_CMD_G6
	    g_cInit_MEDIA_INTERFACE_DESCRIPTOR_LOAD_CMD_G6;
	extern CONST INTERFACE_DESCRIPTOR_DATA_G6
	    g_cInit_INTERFACE_DESCRIPTOR_DATA_G6;
	extern CONST MEDIA_OBJECT_WALKER_CMD_G6
	    g_cInit_MEDIA_OBJECT_WALKER_CMD_G6;
	extern CONST GPGPU_WALKER_CMD_G75 g_cInit_GPGPU_WALKER_CMD_G75;
	extern CONST SURFACE_STATE_G6 g_cInit_SURFACE_STATE_G6;
	extern CONST MEDIA_STATE_FLUSH_CMD_G75
	    g_cInit_MEDIA_STATE_FLUSH_CMD_G75;
	extern CONST MEDIA_OBJECT_HEADER_G6 g_cInit_MEDIA_OBJECT_HEADER_G6;
	extern CONST SURFACE_STATE_TOKEN_G75 g_cInit_SURFACE_STATE_TOKEN_G75;

	extern CONST PIPE_CONTROL_CMD_G7 g_cInit_PIPE_CONTROL_CMD_G7;
	extern CONST SURFACE_STATE_G7 g_cInit_SURFACE_STATE_G7;

	extern CONST MI_SET_PREDICATE_CMD_G75 g_cInit_MI_SET_PREDICATE_CMD_G75;
	extern CONST STATE_BASE_ADDRESS_CMD_G75
	    g_cInit_STATE_BASE_ADDRESS_CMD_G75;
	extern CONST MI_BATCH_BUFFER_START_CMD_G75
	    g_cInit_MI_BATCH_BUFFER_START_CMD_G75;

	extern CONST BINDING_TABLE_STATE_G8 g_cInit_BINDING_TABLE_STATE_G8;
	extern CONST SURFACE_STATE_G8 g_cInit_SURFACE_STATE_G8;
	extern CONST PIPE_CONTROL_CMD_G8 g_cInit_PIPE_CONTROL_CMD_G8;
	extern CONST STATE_BASE_ADDRESS_CMD_G8
	    g_cInit_STATE_BASE_ADDRESS_CMD_G8;
	extern CONST MI_BATCH_BUFFER_START_CMD_G8
	    g_cInit_MI_BATCH_BUFFER_START_CMD_G8;
	extern CONST MEDIA_VFE_STATE_CMD_G8 g_cInit_MEDIA_VFE_STATE_CMD_G8;
	extern CONST INTERFACE_DESCRIPTOR_DATA_G8
	    g_cInit_INTERFACE_DESCRIPTOR_DATA_G8;
	extern CONST GPGPU_WALKER_CMD_G8 g_cInit_GPGPU_WALKER_CMD_G8;

#ifdef __cplusplus
}
#endif
#endif
