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

#ifndef _MEDIA__DRIVER_ENCODER_H
#define _MEDIA__DRIVER_ENCODER_H
#include "media_drv_init.h"
#include "media_drv_gpe_utils.h"
#include "media_drv_util.h"
#include "media_drv_hw.h"
//#define WIDTH_IN_MACROBLOCKS(width)      (((width) + (16 - 1)) / 16)
//#define HEIGHT_IN_MACROBLOCKS(height)    (((height) + (16 - 1)) / 16)

#define HB_BRC_NONE	0
#define HB_BRC_CBR	1
#define HB_BRC_VBR	2
#define HB_BRC_CQP	3

typedef struct _scaling_kernel_params
{
  bool scaling_16x_en;
  bool scaling_32x_en;
} SCALING_KERNEL_PARAMS;

typedef struct _mbpak_context
{
  MEDIA_GPE_CTX gpe_context;
  MEDIA_GPE_CTX gpe_context2;
  MEDIA_RESOURCE row_buffer_y;
  MEDIA_RESOURCE row_buffer_uv;
  MEDIA_RESOURCE column_buffer_y;
  MEDIA_RESOURCE column_buffer_uv;
  MEDIA_RESOURCE kernel_dump_buffer;
  SURFACE_STATE_BINDING_TABLE surface_state_binding_table_mbpak_p1;
  SURFACE_STATE_BINDING_TABLE surface_state_binding_table_mbpak_p2;
} MBPAK_CONTEXT;

typedef struct _mbenc_context
{
  MEDIA_GPE_CTX gpe_context;
  MEDIA_RESOURCE mb_mode_cost_luma_buffer;
  MEDIA_RESOURCE block_mode_cost_buffer;
  MEDIA_RESOURCE chroma_reconst_buffer;
  MEDIA_RESOURCE histogram_buffer;
  MEDIA_RESOURCE kernel_dump_buffer;
  MEDIA_RESOURCE ref_frm_count_surface;
  MEDIA_RESOURCE pred_mv_data_surface;
  MEDIA_RESOURCE mode_cost_update_surface;
  MEDIA_RESOURCE pred_mb_quant_data_surface;
  SURFACE_STATE_BINDING_TABLE surface_state_binding_table_mbenc_p1;
  SURFACE_STATE_BINDING_TABLE surface_state_binding_table_mbenc_p2;
  SURFACE_STATE_BINDING_TABLE surface_state_binding_table_mbenc_iframe_dist;
} MBENC_CONTEXT;

typedef struct _me_context
{
  MEDIA_GPE_CTX gpe_context;
  MEDIA_RESOURCE mv_distortion_surface_4x_me;
  MEDIA_RESOURCE mv_data_surface_16x_me;
  MEDIA_RESOURCE mv_data_surface_4x_me;
  SURFACE_STATE_BINDING_TABLE surface_state_binding_table_4x_me;
  SURFACE_STATE_BINDING_TABLE surface_state_binding_table_16x_me;
} ME_CONTEXT;

typedef struct _scaling_context
{
  MEDIA_GPE_CTX gpe_context;
  MEDIA_RESOURCE scaled_32x_surface;
  MEDIA_RESOURCE scaled_16x_surface;
  MEDIA_RESOURCE scaled_4x_surface;
  SURFACE_STATE_BINDING_TABLE surface_state_binding_table_scaling;
  SURFACE_STATE_BINDING_TABLE surface_state_binding_table_scaling_16x;
} SCALING_CONTEXT;
typedef struct _brc_init_reset_context
{
  MEDIA_GPE_CTX gpe_context;
  MEDIA_RESOURCE brc_distortion;
  MEDIA_RESOURCE brc_history;
  MEDIA_RESOURCE brc_pak_qp_input_table;
  MEDIA_RESOURCE brc_constant_data;
  MEDIA_RESOURCE brc_constant_buffer[NUM_BRC_CONSTANT_DATA_BUFFERS];
  SURFACE_STATE_BINDING_TABLE surface_state_binding_table_brc_init_reset;
} BRC_INIT_RESET_CONTEXT;

typedef struct _brc_update_context
{
  MEDIA_GPE_CTX gpe_context;
  SURFACE_STATE_BINDING_TABLE surface_state_binding_table_brc_update;
} BRC_UPDATE_CONTEXT;

typedef struct media_encoder_ctx
{
  struct hw_context base;
  int codec;
  VASurfaceID input_yuv_surface;
  int is_tmp_id;
  unsigned int rate_control_mode;
  unsigned int internal_rate_mode;
  ME_CONTEXT me_context;
  MBENC_CONTEXT mbenc_context;
  MBPAK_CONTEXT mbpak_context;
  SCALING_CONTEXT scaling_context;
  BRC_INIT_RESET_CONTEXT brc_init_reset_context;
  BRC_UPDATE_CONTEXT brc_update_context;
  int num_of_kernels;
  unsigned int walker_mode;
  unsigned int kernel_mode;
  bool use_hw_scoreboard;
  int frame_width;
  int frame_height;
  int picture_width;
  int picture_height;
  int picture_width_in_mbs;
  int picture_height_in_mbs;
  int down_scaled_width_mb4x;
  int down_scaled_height_mb4x;
  int down_scaled_width_mb16x;
  int down_scaled_height_mb16x;
  int down_scaled_width_mb32x;
  int down_scaled_height_mb32x;

  int down_scaled_frame_field_height_mb4x;
  int down_scaled_frame_field_width_mb4x;
  int down_scaled_frame_field_height_mb16x;
  int down_scaled_frame_field_width_mb16x;
  int down_scaled_frame_field_height_mb32x;
  int down_scaled_frame_field_width_mb32x;
  unsigned int ref_frame_ctrl;
  unsigned int pic_coding_type;
  bool hme_supported;
  bool scaling_enabled;
  bool me_16x_supported;
  bool brc_enabled;
  bool brc_mb_enabled;
  bool brc_initted;
  bool brc_need_reset;
  bool brc_distortion_buffer_supported;
  bool brc_constant_buffer_supported;
  bool init_brc_distortion_buffer;

  //bool hme_enabled;
  //bool me_16x_enabled;
  //bool hme_done;
  //bool me_16x_done;
  bool kernel_dump_enable;
  bool mbenc_chroma_kernel;
  bool mbenc_curbe_set_brc_update;
  bool mbpak_curbe_set_brc_update;
  bool disable_multi_ref;
  unsigned int mv_offset;
  unsigned int mv_in_bytes;
  unsigned int mb_data_offset;
  unsigned int mb_data_in_bytes;
  unsigned int frame_num;
  UINT frame_rate;
  UINT16 avbr_accuracy; /* ignore ??? */
  UINT16 avbr_convergence; /* ignore ??? */
  DOUBLE brc_init_current_target_buf_full_in_bits;
  DOUBLE brc_init_reset_input_bits_per_frame;
  UINT brc_init_reset_buf_size_in_bits;
  UINT target_bit_rate;
  UINT max_bit_rate;
  UINT min_bit_rate;
  UINT gop_pic_size;
  ULONG init_vbv_buffer_fullness_in_bit;
  ULONG vbv_buffer_size_in_bit;
  MEDIA_FRAME_UPDATE frame_update;

  void (*set_curbe_i_vp8_mbenc) (struct encode_state * encode_state,
				 MEDIA_MBENC_CURBE_PARAMS_VP8 * params);
  void (*set_curbe_p_vp8_mbenc) (struct encode_state * encode_state,
				 MEDIA_MBENC_CURBE_PARAMS_VP8 * params);
  void (*surface_state_vp8_mbenc) (struct media_encoder_ctx * encoder_context,
				   struct encode_state * encode_state,
				   MBENC_SURFACE_PARAMS_VP8 *
				   mbenc_sutface_params);
  void (*surface_state_vp8_mbpak) (struct media_encoder_ctx * encoder_context,
				   struct encode_state * encode_state,
				   MBPAK_SURFACE_PARAMS_VP8 *
				   mbpak_sutface_params);
  void (*set_curbe_vp8_mbpak) (struct encode_state * encode_state,
			       MEDIA_MBPAK_CURBE_PARAMS_VP8 * params);
  void (*media_add_surface_state) (SURFACE_SET_PARAMS * params);
  void (*media_add_binding_table) (MEDIA_GPE_CTX * gpe_ctx);
  void (*gpe_context_vfe_scoreboardinit) (MEDIA_GPE_CTX * gpe_context);
  void (*media_object_walker_pak_init)(UINT pak_phase_type,struct media_encoder_ctx * encoder_context,MEDIA_OBJ_WALKER_PARAMS *media_obj_walker_params);
  void (*media_object_walker_mbenc_init)(BOOL mbenc_i_frame_dist_in_use,BOOL mbenc_phase_2,struct media_encoder_ctx * encoder_context,MEDIA_OBJ_WALKER_PARAMS *media_obj_walker_params);
  void (*gpe_context_vfe_scoreboardinit_pak) (MEDIA_GPE_CTX * gpe_context);
  STATUS (*mediadrv_gen_state_base_address_cmd) (MEDIA_BATCH_BUFFER * batch,STATE_BASE_ADDR_PARAMS * params);
  STATUS (*mediadrv_gen_media_vfe_state_cmd) (MEDIA_BATCH_BUFFER * batch,VFE_STATE_PARAMS * params);

} MEDIA_ENCODER_CTX;

typedef struct media_encoder_vp8_surface
{
  VADriverContextP ctx;
  VASurfaceID scaled_4x_surface_id;
  struct object_surface *scaled_4x_surface_obj;
  unsigned int qp_index;
} MEDIA_ENCODER_VP8_SURFACE;

void
media_alloc_resource_scaling (VADriverContextP ctx,
			      MEDIA_ENCODER_CTX * encoder_context);
void
media_encode_mb_layout_vp8 (MEDIA_ENCODER_CTX * encoder_context, void *data,
			    UINT * data_size);
struct hw_context *media_enc_context_init (VADriverContextP ctx,
					   struct object_config *obj_config,
					   int picture_width,
					   int picture_height);
VAStatus
media_encoder_picture (VADriverContextP ctx,
		       VAProfile profile,
		       union codec_state *codec_state,
		       struct hw_context *hw_context);
void
gpe_context_vfe_scoreboardinit (MEDIA_GPE_CTX * gpe_context);
void
gpe_context_vfe_scoreboardinit_pak (MEDIA_GPE_CTX * gpe_context);
#endif
