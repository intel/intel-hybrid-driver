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

#ifndef _MEDIA__DRIVER_HW_H
#define _MEDIA__DRIVER_HW_H
#include "media_drv_util.h"
#include <stddef.h>
#include <stdbool.h>
#include <i915_drm.h>
#include<xf86drm.h>
#include "media_drv_gpe_utils.h"

#define STATUS_QUERY_END_FLAG   0xFF

typedef struct generic_kernel_params
{
  UINT idrt_kernel_offset;
}GENERIC_KERNEL_PARAMS;

typedef struct _mbenc_constant_buffer_params_vp8
{
  MEDIA_RESOURCE *mb_mode_cost_luma_buffer;
  MEDIA_RESOURCE *block_mode_cost_buffer;

} MBENC_CONSTANT_BUFFER_PARAMS_VP8;

typedef struct _media_mbpak_curbe_params_vp8
{
  UINT updated;
  UINT pak_phase_type;
  VOID *curbe_cmd_buff;
} MEDIA_MBPAK_CURBE_PARAMS_VP8;
typedef struct _media_mbenc_curbe_params_vp8
{
  UINT kernel_mode;
  UINT mb_enc_iframe_dist_in_use;
  UINT updated;
  UINT hme_enabled;
  UINT ref_frame_ctrl;
  UINT brc_enabled;
  VOID *curbe_cmd_buff;
} MEDIA_MBENC_CURBE_PARAMS_VP8;
typedef struct surface_set_params
{

  UINT vert_line_stride_offset;
  UINT vert_line_stride;
  UINT pitch;
  UINT tiling;
  UINT format;
  UINT offset;
  UINT size;
  BOOL surface_is_2d;
  BOOL surface_is_uv_2d;
  BOOL surface_is_raw;
  BOOL media_block_raw;
  BOOL advance_state;
  UINT uv_direction;
  UINT cacheability_control;
  unsigned long binding_table_offset;
  unsigned long surface_state_offset;
  MEDIA_RESOURCE binding_surface_state;
  MEDIA_RESOURCE *surface_2d;
  MEDIA_RESOURCE buf_object;
} SURFACE_SET_PARAMS;

typedef struct mbpak_surface_set_params_vp8
{
  UINT orig_frame_width;
  UINT orig_frame_height;
  UINT mbpak_phase_type;
  BOOL kernel_dump;
  MEDIA_RESOURCE kernel_dump_buffer;
  UINT cacheability_control;
} MBPAK_SURFACE_PARAMS_VP8;
typedef struct mbenc_surface_set_params_vp8
{
  UINT orig_frame_width;
  UINT orig_frame_height;
  UINT pic_coding;
  BOOL kernel_dump;
  BOOL seg_enabled;
  BOOL hme_enabled;
  UINT cacheability_control;
} MBENC_SURFACE_PARAMS_VP8;

typedef struct me_surface_set_params_vp8
{
  BOOL me_16x_in_use;
  BOOL me_i6x_enabled;
  SURFACE_STATE_BINDING_TABLE *me_surface_state_binding_table;
} ME_SURFACE_PARAMS_VP8;
typedef struct scaling_surface_set_params
{
  MEDIA_RESOURCE scaling_input_surface;
  MEDIA_RESOURCE scaling_output_surface;
  SURFACE_STATE_BINDING_TABLE surface_state_binding_table_scaling;
  UINT input_width;
  UINT input_height;
  UINT output_width;
  UINT output_height;
} SCALING_SURFACE_PARAMS;
struct hw_codec_info
{
  INT max_width;
  INT max_height;
  UINT mpeg2_dec_support:1;
  UINT mpeg2_enc_support:1;
  UINT h264_dec_support:1;
  UINT h264_enc_support:1;
  UINT vc1_dec_support:1;
  UINT vc1_enc_support:1;
  UINT jpeg_dec_support:1;
  UINT jpeg_enc_support:1;
  UINT vpp_support:1;
  UINT accelerated_getimage:1;
  UINT accelerated_putimage:1;
  UINT tiled_surface:1;
  UINT di_motion_adptive:1;
  UINT di_motion_compensated:1;
  UINT blending:1;
  UINT vp8_dec_support:1;
  UINT vp8_enc_support:1;
  UINT vp8_enc_hybrid_support:1;
  UINT num_filters;
};

typedef struct mi_store_data_imm_params
{
  MEDIA_RESOURCE status_buffer;
  UINT value;
} MI_STORE_DATA_IMM_PARAMS;
typedef struct media_object_walker_params
{
  BOOL use_scoreboard;
  UINT walker_mode;
  UINT pic_coding_type;
  UINT direct_spatial_mv_pred;
  BOOL me_in_use;
  BOOL mb_enc_iframe_dist_en;
  BOOL force_26_degree;
  BOOL hybrid_pak2_pattern_enabled_45_deg;
  UINT frmfield_h_in_mb;
  UINT frm_w_in_mb;
} MEDIA_OBJ_WALKER_PARAMS;

enum MI_SET_PREDICATE_ENABLE
{
  MI_SET_PREDICATE_ENABLE_ALWAYS = 0x0,
  MI_SET_PREDICATE_ENABLE_ON_CLEAR = 0x1,
  MI_SET_PREDICATE_ENABLE_ON_SET = 0x2,
  MI_SET_PREDICATE_DISABLE = 0x3,
};
typedef struct mi_set_predicate_params
{
  UINT predicate_en;
} MI_SET_PREDICATE_PARAMS;

typedef struct media_idt_params
{
  UINT idrt_size;
  UINT idrt_offset;

} ID_LOAD_PARAMS;

typedef struct media_curbe_load_params
{
  UINT curbe_size;
  UINT curbe_offset;

} CURBE_LOAD_PARAMS;
typedef struct media_vfe_state_params
{
  UINT gpgpu_mode;
  UINT max_num_threads;
  UINT num_urb_entries;
  UINT urb_entry_size;
  UINT curbe_allocation_size;
  UINT scoreboard_enable;
  UINT scoreboard_type;
  UINT scoreboard_mask;
  UINT scoreboardDW5;
  UINT scoreboardDW6;
  UINT scoreboardDW7;

} VFE_STATE_PARAMS;

typedef struct state_base_addr_params
{
  MEDIA_RESOURCE general_state;
  MEDIA_RESOURCE surface_state;
  MEDIA_RESOURCE dynamic_state;
  MEDIA_RESOURCE indirect_object;
  MEDIA_RESOURCE instruction_buffer;
} STATE_BASE_ADDR_PARAMS;

typedef struct pipe_control_params
{
  MEDIA_RESOURCE status_buffer;
  UINT flush_mode;
  UINT immediate_data;
} PIPE_CONTROL_PARAMS;

typedef struct scaling_curbe_params
{
  UINT input_pic_height;
  UINT input_pic_width;
} SCALING_CURBE_PARAMS;

typedef struct _media_hw_context
{
  MEDIA_GPE_CTX gpe_context;
} MEDIA_HW_CONTEXT;

typedef struct _vp8_me_curbe_params
{
  UINT kernel_mode;
  UINT frame_width;
  UINT frame_field_height;
  UINT me_16x_enabled;
  UINT me_16x;
  UINT picture_coding_type;
  VOID *curbe_cmd_buff;
} VP8_ME_CURBE_PARAMS;

#endif
