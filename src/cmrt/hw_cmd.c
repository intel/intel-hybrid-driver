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
#include "hw_cmd.h"

extern CONST BINDING_TABLE_STATE_G5 g_cInit_BINDING_TABLE_STATE_G5 = {
	{
	 0,
	 0,
	 0,
	 0}
};

extern CONST MI_BATCH_BUFFER_END_CMD_G5 g_cInit_MI_BATCH_BUFFER_END_CMD_G5 = {
	{
	 MI_BATCH_BUFFER_END,
	 INSTRUCTION_MI}
};

extern CONST MI_NOOP_CMD_G5 g_cInit_MI_NOOP_CMD_G5 = {
	{
	 0,
	 FALSE,
	 MI_NOOP,
	 INSTRUCTION_MI}
};

extern CONST PIPELINE_SELECT_CMD_G5 g_cInit_PIPELINE_SELECT_CMD_MEDIA_G575 = {
	{
	 GFXPIPELINE_MEDIA,
	 GFXSUBOP_PIPELINE_SELECT,
	 GFXOP_NONPIPELINED,
	 PIPE_SINGLE_DWORD,
	 INSTRUCTION_GFX}
};

extern CONST SURFACE_STATE_TOKEN_G75 g_cInit_SURFACE_STATE_TOKEN_G75 = {
	{
	 OP_LENGTH(SIZE32(SURFACE_STATE_TOKEN_G75)),
	 GFXSUBOP_SURFACE_STATE_TOKEN,
	 GFXOP_PIPELINED,
	 PIPE_3D,
	 INSTRUCTION_GFX,
	 1},

	{
	 0,
	 0},

	{
	 0},

	{
	 0,
	 0,
	 0},

	{
	 0},

	{
	 0},
};

extern CONST MEDIA_OBJECT_HEADER_G6 g_cInit_MEDIA_OBJECT_HEADER_G6 = {
	{
	 OP_LENGTH(SIZE32(MEDIA_OBJECT_FC_CMD_G6)),
	 MEDIASUBOP_MEDIA_OBJECT,
	 MEDIAOP_MEDIA_OBJECT,
	 PIPE_MEDIA,
	 INSTRUCTION_GFX},

	{
	 0,
	 0},

	{
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0},

	{
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0},
};

extern CONST PIPE_CONTROL_CMD_G6 g_cInit_PIPE_CONTROL_CMD_G6 = {
	{
	 OP_LENGTH(SIZE32(PIPE_CONTROL_CMD_G6)),
	 GFX3DSUBOP_3DCONTROL,
	 GFX3DOP_3DCONTROL,
	 PIPE_3D,
	 INSTRUCTION_GFX},

	{
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 1,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0},

	{
	 1,
	 0},

	{
	 0},

	{
	 0}
};

extern CONST MI_LOAD_REGISTER_IMM_CMD_G6 g_cInit_MI_LOAD_REGISTER_IMM_CMD_G6 = {
	{
	 OP_LENGTH(SIZE32(MI_LOAD_REGISTER_IMM_CMD_G6)),
	 0,
	 MI_LOAD_REGISTER_IMM,
	 INSTRUCTION_MI},

	{
	 0},

	{
	 0}
};

extern CONST MI_BATCH_BUFFER_START_CMD_G75 g_cInit_MI_BATCH_BUFFER_START_CMD_G75
    = {
	{
	 OP_LENGTH(SIZE32(MI_BATCH_BUFFER_START_CMD_G75)),
	 MI_BB_ADDRESS_USE_PPGTT,
	 FALSE,
	 FALSE,
	 FALSE,
	 MI_BB_SECURITY_PRIVILEGED,
	 FALSE,
	 FALSE,
	 FALSE,
	 MI_BATCH_BUFFER_START,
	 INSTRUCTION_MI},

	{
	 0}
};

extern CONST MI_BATCH_BUFFER_START_CMD_G8 g_cInit_MI_BATCH_BUFFER_START_CMD_G8 = {
	{
	 OP_LENGTH(SIZE32(MI_BATCH_BUFFER_START_CMD_G8)),
	 MI_BB_ADDRESS_USE_PPGTT,
	 FALSE,
	 FALSE,
	 FALSE,
	 FALSE,
	 MI_BATCH_BUFFER_START,
	 INSTRUCTION_MI},

	{
	 0},

	{
	 0}
};

extern CONST SURFACE_STATE_G6 g_cInit_SURFACE_STATE_G6 = {
	{
	 FALSE,
	 FALSE,
	 FALSE,
	 FALSE,
	 FALSE,
	 FALSE,
	 GFX3DSTATE_BOUNDARY_NORMAL,
	 GFX3DSTATE_WRITE_ONLY_ON_MISS,
	 GFX3DSTATE_CUBE_REPLICATE,
	 GFX3DSTATE_SURFACE_MIPMAPLAYOUT_BELOW,
	 0,
	 0,
	 FALSE,
	 FALSE,
	 FALSE,
	 FALSE,
	 FALSE,
	 GFX3DSTATE_SURFACEFORMAT_R8_UNORM,
	 GFX3DSTATE_SURFACERETURNFORMAT_FLOAT32,
	 GFX3DSTATE_SURFACETYPE_2D},

	{
	 0},

	{
	 0,
	 0,
	 0,
	 0},

	{
	 GFX3DSTATE_TILEWALK_XMAJOR,
	 FALSE,
	 0,
	 0},

	{
	 0,
	 0,
	 0,
	 0,
	 0},

	{
	 0,
	 0,
	 0,
	 0,
	 0,
	 0}
};

extern CONST STATE_BASE_ADDRESS_CMD_G6 g_cInit_STATE_BASE_ADDRESS_CMD_G6 = {
	{
	 OP_LENGTH(SIZE32(STATE_BASE_ADDRESS_CMD_G6)),
	 GFXSUBOP_STATE_BASE_ADDRESS,
	 GFXOP_NONPIPELINED,
	 PIPE_COMMON,
	 INSTRUCTION_GFX},

	{
	 TRUE,
	 0,
	 0,
	 0,
	 0},

	{
	 TRUE,
	 0,
	 0},

	{
	 TRUE,
	 0,
	 0},

	{
	 TRUE,
	 0,
	 0},

	{
	 TRUE,
	 0,
	 0},

	{
	 TRUE,
	 0},

	{
	 TRUE,
	 0},

	{
	 TRUE,
	 0x0},

	{
	 TRUE,
	 0}

};

extern CONST MEDIA_VFE_STATE_CMD_G6 g_cInit_MEDIA_VFE_STATE_CMD_G6 = {
	{
	 OP_LENGTH(SIZE32(MEDIA_VFE_STATE_CMD_G6)),
	 MEDIASUBOP_MEDIA_VFE_STATE,
	 GFXOP_PIPELINED,
	 PIPE_MEDIA,
	 INSTRUCTION_GFX},

	{
	 0,
	 0},

	{
	 0,
	 MEDIASTATE_MEDIA_MODE,
	 0,
	 FALSE,
	 FALSE,
	 FALSE,
	 0,
	 0},

	{
	 0},

	{
	 0,
	 0},

	{
	 0xFF,
	 0,
	 FALSE},

	{
	 0xF,
	 0,
	 0,
	 0xF,
	 1,
	 0xF,
	 0xF,
	 0xF},

	{
	 0xF,
	 1,
	 0,
	 0xE,
	 1,
	 0xE,
	 0xF,
	 0xE}
};

extern CONST MEDIA_VFE_STATE_CMD_G8 g_cInit_MEDIA_VFE_STATE_CMD_G8 = {
	{
	 OP_LENGTH(SIZE32(MEDIA_VFE_STATE_CMD_G8)),
	 MEDIASUBOP_MEDIA_VFE_STATE,
	 GFXOP_PIPELINED,
	 PIPE_MEDIA,
	 INSTRUCTION_GFX},

	{
	 0,
	 0},

	{
	 0},

	{
	 0,
	 MEDIASTATE_MEDIA_MODE,
	 0,
	 FALSE,
	 FALSE,
	 FALSE,
	 0,
	 0},

	{
	 0},

	{
	 0,
	 0},

	{
	 0xFF,
	 0,
	 FALSE},

	{
	 0xF,
	 0,
	 0,
	 0xF,
	 1,
	 0xF,
	 0xF,
	 0xF},

	{
	 0xF,
	 1,
	 0,
	 0xE,
	 1,
	 0xE,
	 0xF,
	 0xE}
};

extern CONST MEDIA_CURBE_LOAD_CMD_G6 g_cInit_MEDIA_CURBE_LOAD_CMD_G6 = {
	{
	 OP_LENGTH(SIZE32(MEDIA_CURBE_LOAD_CMD_G6)),
	 MEDIASUBOP_MEDIA_CURBE_LOAD,
	 GFXOP_PIPELINED,
	 PIPE_MEDIA,
	 INSTRUCTION_GFX},

	{
	 0},

	{
	 0},

	{
	 0}
};

extern CONST MEDIA_INTERFACE_DESCRIPTOR_LOAD_CMD_G6
    g_cInit_MEDIA_INTERFACE_DESCRIPTOR_LOAD_CMD_G6 = {
	{
	 OP_LENGTH(SIZE32(MEDIA_INTERFACE_DESCRIPTOR_LOAD_CMD_G6)),
	 MEDIASUBOP_MEDIA_INTERFACE_DESCRIPTOR_LOAD,
	 GFXOP_PIPELINED,
	 PIPE_MEDIA,
	 INSTRUCTION_GFX},

	{
	 0},

	{
	 0},

	{
	 0}
};

extern CONST INTERFACE_DESCRIPTOR_DATA_G6 g_cInit_INTERFACE_DESCRIPTOR_DATA_G6 = {
	{
	 0},

	{
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 },

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0,
	 0,
	 FALSE,
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0}
};

extern CONST INTERFACE_DESCRIPTOR_DATA_G8 g_cInit_INTERFACE_DESCRIPTOR_DATA_G8 = {
	{
	 0},

	{
	 0},

	{
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 },

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 1,
	 0,
	 0,
	 FALSE,
	 0,
	 },

	{
	 0}
};

extern CONST MEDIA_OBJECT_WALKER_CMD_G6 g_cInit_MEDIA_OBJECT_WALKER_CMD_G6 = {
	{
	 OP_LENGTH(SIZE32(MEDIA_OBJECT_WALKER_CMD_G6)),
	 MEDIASUBOP_MEDIA_OBJECT_WALKER,
	 GFXOP_NONPIPELINED,
	 PIPE_MEDIA,
	 INSTRUCTION_GFX},

	{
	 0,
	 0},

	{
	 0,
	 0,
	 0,
	 0},

	{
	 0},

	{
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0}
};

extern CONST GPGPU_WALKER_CMD_G75 g_cInit_GPGPU_WALKER_CMD_G75 = {
	{
	 OP_LENGTH(SIZE32(GPGPU_WALKER_CMD_G75)),
	 0,
	 0,
	 MEDIASUBOP_GPGPU_WALKER,
	 GFXOP_NONPIPELINED,
	 PIPE_MEDIA,
	 INSTRUCTION_GFX},

	{
	 0,
	 0},

	{
	 0,
	 0,
	 0,
	 2},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0xffffffff},

	{
	 0xffffffff},
};

extern CONST GPGPU_WALKER_CMD_G8 g_cInit_GPGPU_WALKER_CMD_G8 = {
	{
	 OP_LENGTH(SIZE32(GPGPU_WALKER_CMD_G8)),
	 0,
	 0,
	 MEDIASUBOP_GPGPU_WALKER,
	 GFXOP_NONPIPELINED,
	 PIPE_MEDIA,
	 INSTRUCTION_GFX},

	{
	 0,
	 0},

	{
	 0},

	{
	 0},

	{
	 0,
	 0,
	 0,
	 2},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0xffffffff},

	{
	 0xffffffff},
};

extern CONST PIPE_CONTROL_CMD_G7 g_cInit_PIPE_CONTROL_CMD_G7 = {
	{
	 OP_LENGTH(SIZE32(PIPE_CONTROL_CMD_G7)),
	 GFX3DSUBOP_3DCONTROL,
	 GFX3DOP_3DCONTROL,
	 PIPE_3D,
	 INSTRUCTION_GFX},

	{
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 1,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 1,
	 0},

	{
	 0},

	{
	 0},

	{
	 0}
};

extern CONST PIPE_CONTROL_CMD_G8 g_cInit_PIPE_CONTROL_CMD_G8 = {
	{
	 OP_LENGTH(SIZE32(PIPE_CONTROL_CMD_G8)),
	 GFX3DSUBOP_3DCONTROL,
	 GFX3DOP_3DCONTROL,
	 PIPE_3D,
	 INSTRUCTION_GFX},

	{
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 1,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 1,
	 0},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0}
};

extern CONST SURFACE_STATE_G7 g_cInit_SURFACE_STATE_G7 = {
	{
	 FALSE,
	 FALSE,
	 FALSE,
	 FALSE,
	 FALSE,
	 FALSE,
	 GFX3DSTATE_BOUNDARY_NORMAL,
	 GFX3DSTATE_WRITE_ONLY_ON_MISS,
	 1,
	 0,
	 0,
	 GFX3DSTATE_TILEWALK_XMAJOR,
	 FALSE,
	 GFX3DSTATE_SURFACE_HORIZONTAL_ALIGNMENT_4,
	 GFX3DSTATE_SURFACE_VERTICAL_ALIGNMENT_2,
	 GFX3DSTATE_SURFACEFORMAT_R8_UNORM,
	 0,
	 FALSE,
	 GFX3DSTATE_SURFACETYPE_2D},

	{
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 0,
	 0,
	 0,
	 0,
	 0},

	{
	 0,
	 0,
	 0,
	 0,
	 0,
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0,
	 SCS_ALPHA,
	 SCS_BLUE,
	 SCS_GREEN,
	 SCS_RED,
	 0,
	 0,
	 0,
	 0}
};

extern CONST BINDING_TABLE_STATE_G8 g_cInit_BINDING_TABLE_STATE_G8 = {
	{
	 0,
	 0,
	 0,
	 0}
};

extern CONST SURFACE_STATE_G8 g_cInit_SURFACE_STATE_G8 = {
	{
	 FALSE,
	 FALSE,
	 FALSE,
	 FALSE,
	 FALSE,
	 FALSE,
	 GFX3DSTATE_BOUNDARY_NORMAL,
	 GFX3DSTATE_WRITE_ONLY_ON_MISS,
	 0,
	 0,
	 0,
	 1,
	 1,
	 GFX3DSTATE_SURFACEFORMAT_R8_UNORM,
	 0,
	 GFX3DSTATE_SURFACETYPE_2D},

	{
	 0,
	 0x20},

	{
	 0,
	 0},

	{
	 0,
	 0},

	{
	 0},

	{
	 0,
	 0,
	 0,
	 0,
	 0},

	{
	 0,
	 },

	{
	 0,
	 SCS_ALPHA,
	 SCS_BLUE,
	 SCS_GREEN,
	 SCS_RED,
	 0,
	 0,
	 0,
	 0},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0},

	{
	 0,
	 0,
	 0},
};

extern CONST STATE_BASE_ADDRESS_CMD_G75 g_cInit_STATE_BASE_ADDRESS_CMD_G75 = {
	{
	 OP_LENGTH(SIZE32(STATE_BASE_ADDRESS_CMD_G75)),
	 GFXSUBOP_STATE_BASE_ADDRESS,
	 GFXOP_NONPIPELINED,
	 PIPE_COMMON,
	 INSTRUCTION_GFX},

	{
	 TRUE,
	 0,
	 0,
	 0},

	{
	 FALSE,
	 0,
	 0},

	{
	 TRUE,
	 0,
	 0},

	{
	 TRUE,
	 0,
	 0},

	{
	 TRUE,
	 0,
	 0},

	{
	 TRUE,
	 0},

	{
	 TRUE,
	 0},

	{
	 TRUE,
	 0xFFFFF},

	{
	 TRUE,
	 0},

	{
	 FALSE,
	 0},

	{
	 FALSE,
	 0}
};

extern CONST STATE_BASE_ADDRESS_CMD_G8 g_cInit_STATE_BASE_ADDRESS_CMD_G8 = {
	{
	 OP_LENGTH(SIZE32(STATE_BASE_ADDRESS_CMD_G8)),
	 GFXSUBOP_STATE_BASE_ADDRESS,
	 GFXOP_NONPIPELINED,
	 PIPE_COMMON,
	 INSTRUCTION_GFX},

	{
	 TRUE,
	 0,
	 0},

	{
	 0,
	 },

	{
	 0,
	 0},

	{
	 FALSE,
	 0,
	 0},

	{
	 0},

	{
	 TRUE,
	 0,
	 0},

	{
	 0},

	{
	 TRUE,
	 0,
	 0},

	{
	 0},

	{
	 TRUE,
	 0,
	 0},

	{
	 0},

	{
	 TRUE,
	 0},

	{
	 TRUE,
	 0},

	{
	 TRUE,
	 0},

	{
	 TRUE,
	 0},
};

extern CONST MI_SET_PREDICATE_CMD_G75 g_cInit_MI_SET_PREDICATE_CMD_G75 = {
	{
	 0,
	 MI_SET_PREDICATE,
	 INSTRUCTION_MI}
};

extern CONST MEDIA_STATE_FLUSH_CMD_G75 g_cInit_MEDIA_STATE_FLUSH_CMD_G75 = {
	{
	 OP_LENGTH(SIZE32(MEDIA_STATE_FLUSH_CMD_G75)),
	 MEDIASUBOP_MEDIA_STATE_FLUSH,
	 GFXOP_PIPELINED,
	 PIPE_MEDIA,
	 INSTRUCTION_GFX},

	{
	 0,
	 0,
	 0,
	 0,
	 }
};
