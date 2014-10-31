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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <va/va.h>
#include <va/va_enc_h264.h>
#include <va/va_enc_vp8.h>
#include <va/va_backend.h>
#include "media_drv_defines.h"
#include "media_drv_init.h"
#include "media_drv_util.h"
#include "media_drv_gpe_utils.h"
#include "media_drv_encoder.h"
#include "media_drv_surface.h"
#include "media_drv_hw.h"
#include "media_drv_hwcmds.h"
#include "media_drv_hw_g75.h"
#include "media_drv_driver.h"

VOID
media_free_resource_me (ME_CONTEXT * me_context)
{

  dri_bo_unreference (me_context->mv_distortion_surface_4x_me.bo);
  me_context->mv_distortion_surface_4x_me.bo = NULL;

  dri_bo_unreference (me_context->mv_data_surface_16x_me.bo);
  me_context->mv_data_surface_16x_me.bo = NULL;

  dri_bo_unreference (me_context->mv_data_surface_4x_me.bo);
  me_context->mv_data_surface_4x_me.bo = NULL;
}

VOID
media_free_resource_mbenc (MBENC_CONTEXT * mbenc_context)
{
  dri_bo_unreference (mbenc_context->mb_mode_cost_luma_buffer.bo);
  mbenc_context->mb_mode_cost_luma_buffer.bo = NULL;

  dri_bo_unreference (mbenc_context->block_mode_cost_buffer.bo);
  mbenc_context->block_mode_cost_buffer.bo = NULL;

  dri_bo_unreference (mbenc_context->chroma_reconst_buffer.bo);
  mbenc_context->chroma_reconst_buffer.bo = NULL;

  dri_bo_unreference (mbenc_context->histogram_buffer.bo);
  mbenc_context->histogram_buffer.bo = NULL;

  dri_bo_unreference (mbenc_context->kernel_dump_buffer.bo);
  mbenc_context->kernel_dump_buffer.bo = NULL;

  dri_bo_unreference (mbenc_context->ref_frm_count_surface.bo);
  mbenc_context->ref_frm_count_surface.bo = NULL;

  dri_bo_unreference (mbenc_context->pred_mv_data_surface.bo);
  mbenc_context->pred_mv_data_surface.bo = NULL;

  dri_bo_unreference (mbenc_context->mode_cost_update_surface.bo);
  mbenc_context->mode_cost_update_surface.bo = NULL;

  dri_bo_unreference (mbenc_context->pred_mb_quant_data_surface.bo);
  mbenc_context->pred_mb_quant_data_surface.bo = NULL;
}

VOID
media_free_resource_mbpak (MBPAK_CONTEXT * mbpak_context)
{

  dri_bo_unreference (mbpak_context->row_buffer_y.bo);
  mbpak_context->row_buffer_y.bo = NULL;

  dri_bo_unreference (mbpak_context->row_buffer_uv.bo);
  mbpak_context->row_buffer_uv.bo = NULL;

  dri_bo_unreference (mbpak_context->column_buffer_y.bo);
  mbpak_context->column_buffer_y.bo = NULL;

  dri_bo_unreference (mbpak_context->column_buffer_uv.bo);
  mbpak_context->column_buffer_uv.bo = NULL;

  dri_bo_unreference (mbpak_context->kernel_dump_buffer.bo);
  mbpak_context->kernel_dump_buffer.bo = NULL;

}

VOID
media_mbpak_context_destroy (MEDIA_ENCODER_CTX * encoder_context)
{
  MBPAK_CONTEXT *mbpak_ctx = &encoder_context->mbpak_context;
  MEDIA_GPE_CTX *mbpak_gpe_ctx = &mbpak_ctx->gpe_context;
  media_free_resource_mbpak (mbpak_ctx);
  media_gpe_context_destroy (mbpak_gpe_ctx);
  mbpak_gpe_ctx = &mbpak_ctx->gpe_context2;
  media_gpe_context_destroy (mbpak_gpe_ctx);
}

VOID
media_mbenc_context_destroy (MEDIA_ENCODER_CTX * encoder_context)
{
  MBENC_CONTEXT *mbenc_ctx = &encoder_context->mbenc_context;
  MEDIA_GPE_CTX *mbenc_gpe_ctx = &mbenc_ctx->gpe_context;
  media_free_resource_mbenc (mbenc_ctx);
  media_gpe_context_destroy (mbenc_gpe_ctx);
}

VOID
media_me_context_destroy (MEDIA_ENCODER_CTX * encoder_context)
{
  ME_CONTEXT *me_ctx = &encoder_context->me_context;
  MEDIA_GPE_CTX *me_gpe_ctx = &me_ctx->gpe_context;
  media_free_resource_me (me_ctx);
  media_gpe_context_destroy (me_gpe_ctx);
}

VOID
media_free_resource_scaling (SCALING_CONTEXT * scaling_context)
{

  dri_bo_unreference (scaling_context->scaled_4x_surface.bo);
  scaling_context->scaled_4x_surface.bo = NULL;

  dri_bo_unreference (scaling_context->scaled_16x_surface.bo);
  scaling_context->scaled_16x_surface.bo = NULL;

  dri_bo_unreference (scaling_context->scaled_32x_surface.bo);
  scaling_context->scaled_32x_surface.bo = NULL;
}

VOID
media_scaling_context_destroy (MEDIA_ENCODER_CTX * encoder_context)
{
  SCALING_CONTEXT *scaling_ctx = &encoder_context->scaling_context;
  MEDIA_GPE_CTX *scaling_gpe_ctx = &scaling_ctx->gpe_context;
  media_free_resource_scaling (scaling_ctx);
  media_gpe_context_destroy (scaling_gpe_ctx);
}

VOID
media_free_resource_brc_init_reset (BRC_INIT_RESET_CONTEXT * context)
{
  int i;

  dri_bo_unreference (context->brc_history.bo);
  context->brc_history.bo = NULL;

  dri_bo_unreference (context->brc_distortion.bo);
  context->brc_distortion.bo = NULL;

  dri_bo_unreference (context->brc_pak_qp_input_table.bo);
  context->brc_pak_qp_input_table.bo = NULL;

  dri_bo_unreference (context->brc_constant_data.bo);
  context->brc_constant_data.bo = NULL;

  for (i = 0; i < NUM_BRC_CONSTANT_DATA_BUFFERS; i++) {
    dri_bo_unreference(context->brc_constant_buffer[i].bo);
    context->brc_constant_buffer[i].bo = NULL;
  }
}

VOID
media_brc_init_reset_context_destroy (MEDIA_ENCODER_CTX * encoder_context)
{
  BRC_INIT_RESET_CONTEXT *ctx = &encoder_context->brc_init_reset_context;
  MEDIA_GPE_CTX *gpe_ctx = &ctx->gpe_context;
  media_free_resource_brc_init_reset (ctx);
  media_gpe_context_destroy (gpe_ctx);
}

VOID
media_free_resource_brc_update (BRC_UPDATE_CONTEXT *context)
{
}

VOID
media_brc_update_context_destroy (MEDIA_ENCODER_CTX *encoder_context)
{
  BRC_UPDATE_CONTEXT *ctx = &encoder_context->brc_update_context;
  MEDIA_GPE_CTX *gpe_ctx = &ctx->gpe_context;

  media_free_resource_brc_update (ctx);
  media_gpe_context_destroy (gpe_ctx);
}

static VOID
media_encoder_context_destroy (VOID * hw_context)
{
  MEDIA_ENCODER_CTX *encoder_context = (MEDIA_ENCODER_CTX *) hw_context;
  media_scaling_context_destroy (encoder_context);
  media_me_context_destroy (encoder_context);
  media_mbenc_context_destroy (encoder_context);
  media_mbpak_context_destroy (encoder_context);
  media_brc_init_reset_context_destroy(encoder_context);
  media_brc_update_context_destroy(encoder_context);
  media_drv_free_memory (encoder_context);
}

static VOID
gpe_context_vfe_scoreboardinit (MEDIA_GPE_CTX * gpe_context)
{
  gpe_context->vfe_state.vfe_desc5.scoreboard0.mask = 0xFF;
  gpe_context->vfe_state.vfe_desc5.scoreboard0.type = SCOREBOARD_NON_STALLING;
  gpe_context->vfe_state.vfe_desc5.scoreboard0.enable = 1;

  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x0 = 0xF;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y0 = 0;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x1 = 0;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y1 = 0xF;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x2 = 1;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y2 = 0xF;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x3 = 0xF;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y3 = 0xF;

  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x4 = 0xF;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y4 = 1;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x5 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y5 = 0xE;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x6 = 1;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y6 = 0xE;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x7 = 0xF;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y7 = 0xE;
}

static VOID
gpe_context_vfe_scoreboardinit_pak (MEDIA_GPE_CTX * gpe_context)
{
  gpe_context->vfe_state.vfe_desc5.scoreboard0.mask = 0x07;
  gpe_context->vfe_state.vfe_desc5.scoreboard0.type = SCOREBOARD_NON_STALLING;
  gpe_context->vfe_state.vfe_desc5.scoreboard0.enable = 1;

  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x0 = 0xF;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y0 = 0;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x1 = 0;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y1 = 0xF;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x2 = 1;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y2 = 0xE;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x3 = 0x0;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y3 = 0x0;

  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x4 = 0x0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y4 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x5 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y5 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x6 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y6 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x7 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y7 = 0;
}

VOID
media_generic_gpe_context_init (VADriverContextP ctx,
				MEDIA_GPE_CTX * gpe_context,
				INT num_of_kernels)
{

  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 280 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 121;	//16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE - 1;

  gpe_context_vfe_scoreboardinit (gpe_context);
  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	// * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
}

BOOL
media_encoder_init (VADriverContextP ctx, MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  INT num_of_kernels;
  switch (encoder_context->codec)
    {
    case CODEC_VP8:
      media_encoder_init_vp8 (ctx, encoder_context);
      break;
    default:
      /* never get here */
      MEDIA_DRV_ASSERT (0);
      break;
    }
  return SUCCESS;
}

struct hw_context *
media_enc_context_init (VADriverContextP ctx,
			struct object_config *obj_config, INT picture_width,
			INT picture_height)
{
  INT i;
  MEDIA_ENCODER_CTX *encoder_context =
    (MEDIA_ENCODER_CTX *) media_drv_alloc_memory (sizeof (MEDIA_ENCODER_CTX));

  encoder_context->base.destroy = media_encoder_context_destroy;
  encoder_context->input_yuv_surface = VA_INVALID_SURFACE;
  encoder_context->is_tmp_id = 0;
  encoder_context->rate_control_mode = VA_RC_NONE;
  encoder_context->internal_rate_mode = HB_BRC_NONE;
  switch (obj_config->profile)
    {
    case VAProfileVP8Version0_3:
      encoder_context->codec = CODEC_VP8;
      break;
    default:
      /* Never get here */
      MEDIA_DRV_ASSERT (0);
      break;

    }
  for (i = 0; i < obj_config->num_attribs; i++)
    {
      if (obj_config->attrib_list[i].type == VAConfigAttribRateControl)
	{
	  encoder_context->rate_control_mode =
	    obj_config->attrib_list[i].value;

	  switch (encoder_context->rate_control_mode) {
	  case VA_RC_CQP:
	    encoder_context->internal_rate_mode = HB_BRC_CQP;
	    break;

	  case VA_RC_CBR:
	    encoder_context->internal_rate_mode = HB_BRC_CBR;
	    break;

	  case VA_RC_VBR:
	    encoder_context->internal_rate_mode = HB_BRC_VBR;
	    break;

	  default:
	    encoder_context->internal_rate_mode = HB_BRC_NONE;
	    break;
	  }
	}
    }
  encoder_context->picture_width = picture_width;
  encoder_context->picture_height = picture_height;
  media_encoder_init (ctx, encoder_context);

  return (struct hw_context *) encoder_context;
}

VOID
media_drv_status_report (VADriverContextP ctx, MEDIA_BATCH_BUFFER * batch,
			 MEDIA_GPE_CTX * gpe_context)
{
  PIPE_CONTROL_PARAMS pipe_ctrl_params = { {NULL, 0, 0} };
  pipe_ctrl_params.status_buffer.bo = gpe_context->status_buffer.res.bo;
  pipe_ctrl_params.immediate_data = 1;
  pipe_ctrl_params.flush_mode = FLUSH_NONE;
  mediadrv_gen_pipe_ctrl_cmd (batch, &pipe_ctrl_params);
}

VOID
media_drv_generic_kernel_cmds (VADriverContextP ctx,
			       MEDIA_ENCODER_CTX * encoder_context,
			       MEDIA_BATCH_BUFFER * batch,
			       MEDIA_GPE_CTX * gpe_context,
			       GENERIC_KERNEL_PARAMS * params)
{
  PIPE_CONTROL_PARAMS pipe_ctrl_params = { {NULL, 0, 0} };
  STATE_BASE_ADDR_PARAMS state_base_addr_params;
  VFE_STATE_PARAMS vfe_state_params;
  CURBE_LOAD_PARAMS curbe_load_params;
  ID_LOAD_PARAMS id_load_params;

  pipe_ctrl_params.flush_mode = FLUSH_WRITE_CACHE;
  mediadrv_gen_pipe_ctrl_cmd (batch, &pipe_ctrl_params);
  pipe_ctrl_params.immediate_data = encoder_context->frame_num;
  pipe_ctrl_params.flush_mode = FLUSH_READ_CACHE;
  mediadrv_gen_pipe_ctrl_cmd (batch, &pipe_ctrl_params);

#if 0
#ifdef STATUS_REPORT
  media_drv_status_report (ctx, batch, gpe_context);
#endif
#endif
  mediadrv_gen_pipeline_select_cmd (batch);

  state_base_addr_params.surface_state.bo =
    /*batch->buffer; */ gpe_context->surface_state_binding_table.res.bo;
  state_base_addr_params.dynamic_state.bo = gpe_context->dynamic_state.res.bo;
  state_base_addr_params.indirect_object.bo = NULL;
  state_base_addr_params.instruction_buffer.bo =
    gpe_context->instruction_state.buff_obj.bo;

  encoder_context->mediadrv_gen_state_base_address_cmd (batch, &state_base_addr_params);
  vfe_state_params.gpgpu_mode = gpe_context->vfe_state.gpgpu_mode;
  vfe_state_params.max_num_threads = gpe_context->vfe_state.max_num_threads;
  vfe_state_params.num_urb_entries = gpe_context->vfe_state.num_urb_entries;
  vfe_state_params.urb_entry_size = gpe_context->vfe_state.urb_entry_size;
  vfe_state_params.curbe_allocation_size =
    gpe_context->vfe_state.curbe_allocation_size;
  vfe_state_params.scoreboard_enable = 1;
  vfe_state_params.scoreboard_type = 0;
  vfe_state_params.scoreboard_mask = 0;
  vfe_state_params.scoreboardDW5 = gpe_context->vfe_state.vfe_desc5.dword;
  vfe_state_params.scoreboardDW6 = gpe_context->vfe_state.vfe_desc6.dword;
  vfe_state_params.scoreboardDW7 = gpe_context->vfe_state.vfe_desc7.dword;
  encoder_context->mediadrv_gen_media_vfe_state_cmd (batch, &vfe_state_params);

  curbe_load_params.curbe_size = gpe_context->curbe_size;
  curbe_load_params.curbe_offset = gpe_context->curbe_offset;
  mediadrv_gen_media_curbe_load_cmd (batch, &curbe_load_params);
  id_load_params.idrt_size = gpe_context->idrt_size;
  id_load_params.idrt_offset =
    (gpe_context->idrt_offset +
     (params->idrt_kernel_offset * gpe_context->idrt_size));
  mediadrv_gen_media_id_load_cmd (batch, &id_load_params);
}

VOID
media_drv_add_predicate_media_obj_wallker_cmd (VADriverContextP ctx,
					       MEDIA_BATCH_BUFFER * batch,
					       MEDIA_OBJ_WALKER_PARAMS *
					       walker_params)
{
  MI_SET_PREDICATE_PARAMS mi_set_predicate_params;
  MEDIA_OBJ_WALKER_PARAMS media_obj_walker_params = *walker_params;

  mi_set_predicate_params.predicate_en = MI_SET_PREDICATE_ENABLE_ON_CLEAR;
  mediadrv_media_mi_set_predicate_cmd (batch, &mi_set_predicate_params);

  media_object_walker_cmd (batch, &media_obj_walker_params);

  mi_set_predicate_params.predicate_en = MI_SET_PREDICATE_ENABLE_ON_SET;
  mediadrv_media_mi_set_predicate_cmd (batch, &mi_set_predicate_params);

  if (walker_params->walker_mode == QUAD_MODE)
    media_obj_walker_params.walker_mode = DUAL_MODE;

  media_object_walker_cmd (batch, &media_obj_walker_params);

  mi_set_predicate_params.predicate_en = MI_SET_PREDICATE_DISABLE;
  mediadrv_media_mi_set_predicate_cmd (batch, &mi_set_predicate_params);
}

VOID
media_drv_end_status_report (VADriverContextP ctx, MEDIA_BATCH_BUFFER * batch,
			     MEDIA_GPE_CTX * gpe_context)
{
  PIPE_CONTROL_PARAMS pipe_ctrl_params;
  MI_STORE_DATA_IMM_PARAMS mi_store_data_imm_params;

  pipe_ctrl_params.status_buffer.bo = gpe_context->status_buffer.res.bo;
  pipe_ctrl_params.immediate_data = 0;
  pipe_ctrl_params.flush_mode = FLUSH_WRITE_CACHE;
  mediadrv_gen_pipe_ctrl_cmd (batch, &pipe_ctrl_params);
  mi_store_data_imm_params.status_buffer.bo =
    gpe_context->status_buffer.res.bo;
  mi_store_data_imm_params.value = STATUS_QUERY_END_FLAG;
  mediadrv_mi_store_data_imm_cmd (batch, &mi_store_data_imm_params);

}

VOID
mediadrv_gen_encode_scaling (VADriverContextP ctx,
			     MEDIA_ENCODER_CTX * encoder_context,
			     struct encode_state *encode_state,
			     SCALING_KERNEL_PARAMS * params, BOOL phase_16x)
{
  UINT pic_coding_type, down_scaled_width_mb, down_scaled_height_mb;
  SCALING_CURBE_PARAMS scaling_curbe_params;
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  SCALING_CONTEXT *scaling_ctx = &encoder_context->scaling_context;
  MEDIA_RESOURCE *scaling_input_surface, *scaling_output_surface, surface_2d;
  struct object_surface *obj_surface;
  MEDIA_BATCH_BUFFER *batch;
  UINT input_width, input_height, output_width, output_height;
  MEDIA_GPE_CTX *scaling_gpe_ctx = &scaling_ctx->gpe_context;
  MEDIA_OBJ_WALKER_PARAMS walker_params;
  SCALING_SURFACE_PARAMS scaling_sutface_params;
  GENERIC_KERNEL_PARAMS kernel_params;
  VAEncSequenceParameterBufferVP8 *seq_param =
    (VAEncSequenceParameterBufferVP8 *) encode_state->seq_param_ext->buffer;
  VAEncPictureParameterBufferVP8 *pic_param =
    (VAEncPictureParameterBufferVP8 *) encode_state->pic_param_ext->buffer;
  pic_coding_type = pic_param->pic_flags.bits.frame_type;
  if (!phase_16x)
    scaling_gpe_ctx->surface_state_binding_table =
      scaling_ctx->surface_state_binding_table_scaling;
  else
    scaling_gpe_ctx->surface_state_binding_table =
      scaling_ctx->surface_state_binding_table_scaling_16x;
  if (params->scaling_32x_en)
    {
      down_scaled_width_mb = encoder_context->down_scaled_width_mb32x;
      down_scaled_height_mb = encoder_context->down_scaled_height_mb32x;
      scaling_curbe_params.input_pic_width =
	ALIGN ((seq_param->frame_width / SCALE_FACTOR_16x), 16);
      scaling_curbe_params.input_pic_height =
	ALIGN ((seq_param->frame_height / SCALE_FACTOR_16x), 16);
      scaling_input_surface = &scaling_ctx->scaled_16x_surface;
      scaling_output_surface = &scaling_ctx->scaled_32x_surface;
      input_width = encoder_context->down_scaled_width_mb16x;
      input_height = encoder_context->down_scaled_height_mb16x;
      output_width = encoder_context->down_scaled_width_mb32x;
      output_height = encoder_context->down_scaled_height_mb32x;
    }
  else if (params->scaling_16x_en)
    {
      down_scaled_width_mb = encoder_context->down_scaled_width_mb16x;
      down_scaled_height_mb = encoder_context->down_scaled_height_mb16x;
      scaling_curbe_params.input_pic_width =
	ALIGN ((seq_param->frame_width / SCALE_FACTOR_4x), 16);
      scaling_curbe_params.input_pic_height =
	ALIGN ((seq_param->frame_height / SCALE_FACTOR_4x), 16);
      scaling_input_surface = &scaling_ctx->scaled_4x_surface;
      scaling_output_surface = &scaling_ctx->scaled_16x_surface;
      input_width = encoder_context->down_scaled_width_mb4x;
      input_height = encoder_context->down_scaled_height_mb4x;
      output_width = encoder_context->down_scaled_width_mb16x;
      output_height = encoder_context->down_scaled_height_mb16x;
    }
  else
    {
      down_scaled_width_mb = encoder_context->down_scaled_width_mb4x;
      down_scaled_height_mb = encoder_context->down_scaled_height_mb4x;
      scaling_curbe_params.input_pic_width = seq_param->frame_width;
      scaling_curbe_params.input_pic_height = seq_param->frame_height;
      obj_surface = encode_state->input_yuv_object;
      OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT (surface_2d, obj_surface);
      scaling_input_surface = &surface_2d;
      scaling_output_surface = &scaling_ctx->scaled_4x_surface;
      input_width = seq_param->frame_width;
      input_height = seq_param->frame_height;
      output_width = encoder_context->down_scaled_width_mb4x;
      output_height = encoder_context->down_scaled_height_mb4x;
    }

  mediadrv_set_curbe_scaling (scaling_gpe_ctx, &scaling_curbe_params);

  scaling_sutface_params.scaling_input_surface = *scaling_input_surface;
  scaling_sutface_params.input_width = input_width;
  scaling_sutface_params.input_height = input_height;
  scaling_sutface_params.scaling_output_surface = *scaling_output_surface;
  scaling_sutface_params.output_width = output_width;
  scaling_sutface_params.output_height = output_height;

  media_surface_state_scaling (encoder_context, &scaling_sutface_params);

  batch = media_batchbuffer_new (&drv_ctx->drv_data, I915_EXEC_RENDER, 0);
  //media_batchbuffer_start_atomic(batch, 0x4000);
  kernel_params.idrt_kernel_offset = 0;
  media_drv_generic_kernel_cmds (ctx, encoder_context, batch, scaling_gpe_ctx,
				 &kernel_params);

  walker_params.walker_mode = encoder_context->walker_mode;
  walker_params.frm_w_in_mb = down_scaled_width_mb * 2;	/* looping for Walker is needed at 8x8 block level */
  walker_params.frmfield_h_in_mb = down_scaled_height_mb * 2;
  walker_params.pic_coding_type = pic_coding_type;
  walker_params.use_scoreboard = 0;	//encoder_context->use_hw_scoreboard;//setting this to zero because walker_params.me_in_use == TRUE
  walker_params.me_in_use = TRUE;	//no dependency dispatch order for Scaling kernel

  media_drv_add_predicate_media_obj_wallker_cmd (ctx, batch, &walker_params);
#if 0
#ifdef STATUS_REPORT
  media_drv_end_status_report (ctx, batch, scaling_gpe_ctx);
#endif
#endif
  media_batchbuffer_submit (batch);

}

VOID
mediadrv_gen_encode_mbpak (VADriverContextP ctx,
			   MEDIA_ENCODER_CTX * encoder_context,
			   struct encode_state *encode_state,
			   UINT pak_phase_type)
{
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  MBPAK_CONTEXT *mbpak_ctx = &encoder_context->mbpak_context;
  MEDIA_GPE_CTX *mbpak_gpe_ctx;
  MEDIA_BATCH_BUFFER *batch;
  MEDIA_MBPAK_CURBE_PARAMS_VP8 curbe_params;
  MBPAK_SURFACE_PARAMS_VP8 sutface_params;
  MEDIA_OBJ_WALKER_PARAMS media_obj_walker_params;
  GENERIC_KERNEL_PARAMS kernel_params;
  /*UINT phase; */
  if (pak_phase_type == MBPAK_HYBRID_STATE_P1)
    {
      mbpak_gpe_ctx = &mbpak_ctx->gpe_context;
      mbpak_gpe_ctx->surface_state_binding_table =
	mbpak_ctx->surface_state_binding_table_mbpak_p1;
      kernel_params.idrt_kernel_offset = MBPAK_PHASE1_OFFSET;
      gpe_context_vfe_scoreboardinit (mbpak_gpe_ctx);
#ifdef DEBUG
      //phase = 1;
#endif
    }
  else
    {
      mbpak_gpe_ctx = &mbpak_ctx->gpe_context2;
      mbpak_gpe_ctx->surface_state_binding_table =
	mbpak_ctx->surface_state_binding_table_mbpak_p2;
      kernel_params.idrt_kernel_offset = MBPAK_PHASE2_OFFSET;
      //FIXME:Need to find a better way to handle this..!
      gpe_context_vfe_scoreboardinit_pak (mbpak_gpe_ctx);
#ifdef DEBUG
      //phase = 2;
#endif
    }

  if (!encoder_context->mbpak_curbe_set_brc_update) {
    curbe_params.curbe_cmd_buff =
      media_map_buffer_obj (mbpak_gpe_ctx->dynamic_state.res.bo);
    curbe_params.updated = encoder_context->mbpak_curbe_set_brc_update;
    curbe_params.pak_phase_type = pak_phase_type;
    encoder_context->set_curbe_vp8_mbpak (encode_state, &curbe_params);
    media_unmap_buffer_obj (mbpak_gpe_ctx->dynamic_state.res.bo);
  }

  media_drv_memset (&sutface_params, sizeof (sutface_params));
  sutface_params.orig_frame_width = encoder_context->picture_width;
  sutface_params.orig_frame_height = encoder_context->picture_height;
  sutface_params.mbpak_phase_type = pak_phase_type;
  sutface_params.kernel_dump = encoder_context->kernel_dump_enable;
  sutface_params.kernel_dump_buffer = mbpak_ctx->kernel_dump_buffer;
  sutface_params.kernel_dump = 1;
  sutface_params.cacheability_control = CACHEABILITY_TYPE_LLC;
  media_add_binding_table (mbpak_gpe_ctx);
  encoder_context->surface_state_vp8_mbpak (encoder_context, encode_state,
					    &sutface_params);

  batch = media_batchbuffer_new (&drv_ctx->drv_data, I915_EXEC_RENDER, 0);
  //kernel_params.idrt_kernel_offset=0;
  media_drv_generic_kernel_cmds (ctx, encoder_context, batch, mbpak_gpe_ctx,
				 &kernel_params);

  media_drv_memset (&media_obj_walker_params,
		    sizeof (media_obj_walker_params));
  media_obj_walker_params.use_scoreboard = encoder_context->use_hw_scoreboard;
  media_obj_walker_params.walker_mode = encoder_context->walker_mode;
  media_obj_walker_params.pic_coding_type = encoder_context->pic_coding_type;
  //media_obj_walker_params.direct_spatial_mv_pred;
  //media_obj_walker_params.me_in_use = TRUE;
  //media_obj_walker_params.mb_enc_iframe_dist_en = mbenc_i_frame_dist_in_use;
  //media_obj_walker_params.force_26_degree;
  //media_obj_walker_params.frmfield_h_in_mb =encoder_context->picture_height_in_mbs;
  media_obj_walker_params.frm_w_in_mb =
    (UINT) encoder_context->picture_width_in_mbs;
  if (pak_phase_type == MBPAK_HYBRID_STATE_P1)
    {
      media_obj_walker_params.me_in_use = TRUE;
      media_obj_walker_params.frmfield_h_in_mb =
	encoder_context->picture_height_in_mbs;
    }
  else if (pak_phase_type == MBPAK_HYBRID_STATE_P2)
    {
      media_obj_walker_params.hybrid_pak2_pattern_enabled_45_deg = 1;
      media_obj_walker_params.frmfield_h_in_mb =
	encoder_context->picture_height_in_mbs * 2;
    }

  media_object_walker_cmd (batch, &media_obj_walker_params);
#if 0
#ifdef STATUS_REPORT
  media_drv_end_status_report (ctx, batch, mbpak_gpe_ctx);
#endif
#endif
  media_batchbuffer_submit (batch);
}

VOID
mediadrv_gen_encode_mbenc (VADriverContextP ctx,
			   MEDIA_ENCODER_CTX * encoder_context,
			   struct encode_state *encode_state,
			   BOOL mbenc_phase_2, BOOL mbenc_i_frame_dist_in_use)
{
  //VAStatus status = VA_STATUS_SUCCESS;
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  MBENC_CONTEXT *mbenc_ctx = &encoder_context->mbenc_context;
  MEDIA_GPE_CTX *mbenc_gpe_ctx = &mbenc_ctx->gpe_context;
  VAEncPictureParameterBufferVP8 *pic_param =
    (VAEncPictureParameterBufferVP8 *) encode_state->pic_param_ext->buffer;
  MEDIA_BATCH_BUFFER *batch;
  VOID *buf = NULL;
  MBENC_CONSTANT_BUFFER_PARAMS_VP8 const_buff_params;
  MEDIA_MBENC_CURBE_PARAMS_VP8 curbe_params;
  MBENC_SURFACE_PARAMS_VP8 mbenc_sutface_params;
  MEDIA_OBJ_WALKER_PARAMS media_obj_walker_params;
  GENERIC_KERNEL_PARAMS kernel_params;

  UINT /*phase, */ ref_frame_flag_final, ref_frame_flag;

  if (mbenc_i_frame_dist_in_use) {
      mbenc_gpe_ctx->surface_state_binding_table =
	mbenc_ctx->surface_state_binding_table_mbenc_iframe_dist;
      kernel_params.idrt_kernel_offset = MBENC_IFRAME_DIST_OFFSET;
  } else
  if (mbenc_phase_2 == FALSE)
    {
      mbenc_gpe_ctx->surface_state_binding_table =
	mbenc_ctx->surface_state_binding_table_mbenc_p1;
      if (encoder_context->pic_coding_type == FRAME_TYPE_I)
	kernel_params.idrt_kernel_offset = MBENC_ILUMA_START_OFFSET;
      else
	kernel_params.idrt_kernel_offset = MBENC_P_START_OFFSET;
#ifdef DEBUG
      //phase = 2;
#endif
    }
  else
    {
      mbenc_gpe_ctx->surface_state_binding_table =
	mbenc_ctx->surface_state_binding_table_mbenc_p2;
      kernel_params.idrt_kernel_offset = MBENC_ICHROMA_START_OFFSET;
#ifdef DEBUG
      //phase = 1;
#endif
    }

  if (mbenc_phase_2 == FALSE) {
    if (!encoder_context->mbenc_curbe_set_brc_update) {
      curbe_params.kernel_mode = encoder_context->kernel_mode;
      curbe_params.mb_enc_iframe_dist_in_use = mbenc_i_frame_dist_in_use;
      curbe_params.updated = encoder_context->mbenc_curbe_set_brc_update;
      curbe_params.hme_enabled = encode_state->hme_enabled;
      //curbe_params.ref_frame_ctrl = encoder_context->ref_frame_ctrl;
      curbe_params.brc_enabled = encoder_context->brc_enabled;

      if (encoder_context->pic_coding_type == FRAME_TYPE_P)
	{
	  ref_frame_flag = 0x07;
	  if (pic_param->ref_last_frame == pic_param->ref_gf_frame)
	    {
	      ref_frame_flag &= ~GOLDEN_REF_FLAG_VP8;
	    }
	  if (pic_param->ref_last_frame == pic_param->ref_arf_frame)
	    {
	      ref_frame_flag &= ~ALT_REF_FLAG_VP8;
	    }
	  if (pic_param->ref_gf_frame == pic_param->ref_arf_frame)
	    {
	      ref_frame_flag &= ~ALT_REF_FLAG_VP8;
	    }
	}
      else
	{
	  ref_frame_flag = 1;
	}

      switch (encoder_context->ref_frame_ctrl)
	{
	case 0:
	  ref_frame_flag_final = 0;
	  break;
	case 1:
	  ref_frame_flag_final = 1;	//Last Ref only
	  break;
	case 2:
	  ref_frame_flag_final = 2;	//Gold Ref only
	  break;
	case 4:
	  ref_frame_flag_final = 4;	//Alt Ref only
	  break;
	default:
	  ref_frame_flag_final =
	    (ref_frame_flag & encoder_context->ref_frame_ctrl)
	    ? (ref_frame_flag & encoder_context->ref_frame_ctrl) : 0x1;
	}
      encoder_context->ref_frame_ctrl = ref_frame_flag_final;
      curbe_params.ref_frame_ctrl = encoder_context->ref_frame_ctrl;

      if (encoder_context->pic_coding_type == FRAME_TYPE_I)
	{
	  buf = media_map_buffer_obj (mbenc_gpe_ctx->dynamic_state.res.bo);
	  curbe_params.curbe_cmd_buff = buf;
	  encoder_context->set_curbe_i_vp8_mbenc (encode_state,
						  &curbe_params);
	  media_unmap_buffer_obj (mbenc_gpe_ctx->dynamic_state.res.bo);
	}
      else if (encoder_context->pic_coding_type == FRAME_TYPE_P)
	{

	  buf = media_map_buffer_obj (mbenc_gpe_ctx->dynamic_state.res.bo);
	  curbe_params.curbe_cmd_buff = buf;
	  encoder_context->set_curbe_p_vp8_mbenc (encode_state,
						  &curbe_params);
	  media_unmap_buffer_obj (mbenc_gpe_ctx->dynamic_state.res.bo);
	}
      }

      if (encoder_context->pic_coding_type == FRAME_TYPE_I) {
	  const_buff_params.mb_mode_cost_luma_buffer =
	    &mbenc_ctx->mb_mode_cost_luma_buffer;
	  const_buff_params.block_mode_cost_buffer =
	    &mbenc_ctx->block_mode_cost_buffer;
	  const_buff_params.mode_cost_update_surface =
	    &mbenc_ctx->mode_cost_update_surface;
	  media_encode_init_mbenc_constant_buffer_vp8_g75
	    (&const_buff_params);
      }

      if (encoder_context->init_brc_distortion_buffer &&
	  mbenc_i_frame_dist_in_use) {
	  BRC_INIT_RESET_CONTEXT *brc_init_reset_context = &encoder_context->brc_init_reset_context;
	  BYTE *brc_distortion_data = NULL;

	  brc_distortion_data = (BYTE *) media_map_buffer_obj (brc_init_reset_context->brc_distortion.bo);
	  media_drv_memset (brc_distortion_data,
			    brc_init_reset_context->brc_distortion.pitch *
			    brc_init_reset_context->brc_distortion.height);
	  media_unmap_buffer_obj (brc_init_reset_context->brc_distortion.bo);
      }
    }

  media_drv_memset (&mbenc_sutface_params, sizeof (mbenc_sutface_params));
  mbenc_sutface_params.pic_coding = encoder_context->pic_coding_type;
  mbenc_sutface_params.orig_frame_width = encoder_context->picture_width;
  mbenc_sutface_params.orig_frame_height = encoder_context->picture_height;
  mbenc_sutface_params.iframe_dist_in_use = mbenc_i_frame_dist_in_use;
  mbenc_sutface_params.cacheability_control = CACHEABILITY_TYPE_LLC;
  mbenc_sutface_params.kernel_dump = 1;
  media_add_binding_table (&mbenc_ctx->gpe_context);

  encoder_context->surface_state_vp8_mbenc (encoder_context, encode_state,
					    &mbenc_sutface_params);
  batch = media_batchbuffer_new (&drv_ctx->drv_data, I915_EXEC_RENDER, 0);
  media_drv_generic_kernel_cmds (ctx, encoder_context, batch, mbenc_gpe_ctx,
				 &kernel_params);

  media_drv_memset (&media_obj_walker_params,
		    sizeof (media_obj_walker_params));
  media_obj_walker_params.pic_coding_type = encoder_context->pic_coding_type;
  if ((encoder_context->pic_coding_type == FRAME_TYPE_I)
      && (mbenc_phase_2 == FALSE))
    media_obj_walker_params.me_in_use = TRUE;
  else
    {
      media_obj_walker_params.pic_coding_type = FRAME_TYPE_I;
    }

  media_obj_walker_params.use_scoreboard = encoder_context->use_hw_scoreboard;
  media_obj_walker_params.walker_mode = encoder_context->walker_mode;
  //media_obj_walker_params.direct_spatial_mv_pred;
  //media_obj_walker_params.me_in_use = TRUE;
  media_obj_walker_params.mb_enc_iframe_dist_en = mbenc_i_frame_dist_in_use;
  //media_obj_walker_params.force_26_degree;
  media_obj_walker_params.frmfield_h_in_mb =
    mbenc_i_frame_dist_in_use ?
    encoder_context->down_scaled_frame_field_height_mb4x :
    encoder_context->picture_height_in_mbs;
  media_obj_walker_params.frm_w_in_mb =
    mbenc_i_frame_dist_in_use ? encoder_context->down_scaled_width_mb4x
    : (UINT) encoder_context->picture_width_in_mbs;

  media_object_walker_cmd (batch, &media_obj_walker_params);
#if 0
#ifdef STATUS_REPORT
  media_drv_end_status_report (ctx, batch, mbenc_gpe_ctx);
#endif
#endif
  media_batchbuffer_submit (batch);
}

VOID
mediadrv_gen_encode_me (VADriverContextP ctx,
			MEDIA_ENCODER_CTX * encoder_context,
			struct encode_state *encode_state, BOOL me_phase)
{
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  VP8_ME_CURBE_PARAMS me_curbe_params;
  ME_SURFACE_PARAMS_VP8 me_sutface_params;
  BOOL me_16x = encode_state->me_16x_enabled && !encode_state->me_16x_done;
  VOID *buf = NULL;
  MEDIA_OBJ_WALKER_PARAMS media_obj_walker_params;
  MEDIA_BATCH_BUFFER *batch;
  ME_CONTEXT *me_ctx = &encoder_context->me_context;
  MEDIA_GPE_CTX *me_gpe_ctx = &me_ctx->gpe_context;
  VAEncSequenceParameterBufferVP8 *seq_param =
    (VAEncSequenceParameterBufferVP8 *) encode_state->seq_param_ext->buffer;
  VAEncPictureParameterBufferVP8 *pic_param =
    (VAEncPictureParameterBufferVP8 *) encode_state->pic_param_ext->buffer;
  UINT pic_coding_type = pic_param->pic_flags.bits.frame_type;
  GENERIC_KERNEL_PARAMS kernel_params;
  if (me_phase)
    {
      me_gpe_ctx->surface_state_binding_table =
	me_ctx->surface_state_binding_table_4x_me;
    }
  else
    {
      me_gpe_ctx->surface_state_binding_table =
	me_ctx->surface_state_binding_table_16x_me;
    }
  me_curbe_params.frame_width = seq_param->frame_width;
  me_curbe_params.frame_field_height = seq_param->frame_height;
  me_curbe_params.picture_coding_type = pic_coding_type;
  me_curbe_params.me_16x = me_16x;
  me_curbe_params.me_16x_enabled = encode_state->me_16x_enabled;
  me_curbe_params.kernel_mode = encoder_context->kernel_mode;
  buf = media_map_buffer_obj (me_gpe_ctx->dynamic_state.res.bo);
  me_curbe_params.curbe_cmd_buff = buf;
  media_set_curbe_vp8_me (&me_curbe_params);
  media_unmap_buffer_obj (me_gpe_ctx->dynamic_state.res.bo);
  //batch = media_batchbuffer_new (&drv_ctx->drv_data, I915_EXEC_RENDER, 0);
  //_(ctx, batch, me_gpe_ctx);

  me_sutface_params.me_16x_in_use = 1;
  me_sutface_params.me_i6x_enabled = 1;
  me_sutface_params.me_surface_state_binding_table =
    &me_gpe_ctx->surface_state_binding_table;

  //media_alloc_binding_surface_state(drv_ctx,&me_gpe_ctx->context->surface_state_binding_table);
  media_surface_state_vp8_me (encoder_context, &me_sutface_params);


  batch = media_batchbuffer_new (&drv_ctx->drv_data, I915_EXEC_RENDER, 0);
  kernel_params.idrt_kernel_offset = 0;
  media_drv_generic_kernel_cmds (ctx, encoder_context, batch, me_gpe_ctx,
				 &kernel_params);


  media_drv_memset (&media_obj_walker_params,
		    sizeof (media_obj_walker_params));
  media_obj_walker_params.use_scoreboard = 0;
  media_obj_walker_params.walker_mode = encoder_context->walker_mode;
  media_obj_walker_params.pic_coding_type = pic_coding_type;
  //media_obj_walker_params.direct_spatial_mv_pred;
  media_obj_walker_params.me_in_use = TRUE;
  //media_obj_walker_params.mb_enc_iframe_dist_en;
  //media_obj_walker_params.force_26_degree;
  media_obj_walker_params.frmfield_h_in_mb =
    me_16x ? encoder_context->down_scaled_frame_field_height_mb16x :
    encoder_context->down_scaled_frame_field_height_mb4x;
  media_obj_walker_params.frm_w_in_mb =
    (me_16x) ? encoder_context->down_scaled_width_mb16x : encoder_context->
    down_scaled_width_mb4x;
  media_object_walker_cmd (batch, &media_obj_walker_params);
  media_batchbuffer_submit (batch);
}

VOID
mediadrv_gen_encode_brc_init_reset(VADriverContextP ctx,
                                   MEDIA_ENCODER_CTX * encoder_context,
                                   struct encode_state *encode_state)
{
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  MEDIA_BRC_INIT_RESET_PARAMS_VP8 curbe_params;
  BRC_INIT_RESET_CONTEXT *brc_init_reset_context = &encoder_context->brc_init_reset_context;
  MEDIA_GPE_CTX *gpe_ctx = &brc_init_reset_context->gpe_context;
  BRC_INIT_RESET_SURFACE_PARAMS_VP8 surface_params;
  GENERIC_KERNEL_PARAMS kernel_params;
  MEDIA_BATCH_BUFFER *batch;
  MEDIA_OBJECT_PARAMS media_object_params;

  /* kernel id */
  if (!encoder_context->brc_initted)
    kernel_params.idrt_kernel_offset = BRC_INIT_OFFSET;
  else
    kernel_params.idrt_kernel_offset = BRC_RESET_OFFSET;

  /* binding table */
  gpe_ctx->surface_state_binding_table =
    brc_init_reset_context->surface_state_binding_table_brc_init_reset;

  /* curbe */
  curbe_params.frame_width = encoder_context->frame_width;
  curbe_params.frame_height = encoder_context->frame_height;
  curbe_params.avbr_accuracy = encoder_context->avbr_accuracy;
  curbe_params.avbr_convergence = encoder_context->avbr_convergence;
  curbe_params.brc_initted = encoder_context->brc_initted;
  curbe_params.brc_mb_enabled = encoder_context->brc_mb_enabled;
  curbe_params.frame_rate = encoder_context->frame_rate;
  curbe_params.rate_control_mode = encoder_context->rate_control_mode;
  curbe_params.target_bit_rate = encoder_context->target_bit_rate;
  curbe_params.max_bit_rate = encoder_context->max_bit_rate;
  curbe_params.min_bit_rate = encoder_context->min_bit_rate;
  curbe_params.init_vbv_buffer_fullness_in_bit = encoder_context->init_vbv_buffer_fullness_in_bit;
  curbe_params.vbv_buffer_size_in_bit = encoder_context->vbv_buffer_size_in_bit;
  curbe_params.gop_pic_size = encoder_context->gop_pic_size;
  curbe_params.brc_init_current_target_buf_full_in_bits =
    &encoder_context->brc_init_current_target_buf_full_in_bits;
  curbe_params.brc_init_reset_buf_size_in_bits =
    &encoder_context->brc_init_reset_buf_size_in_bits;
  curbe_params.brc_init_reset_input_bits_per_frame =
    &encoder_context->brc_init_reset_input_bits_per_frame;

  curbe_params.curbe_cmd_buff =
    media_map_buffer_obj (gpe_ctx->dynamic_state.res.bo);
  media_set_curbe_vp8_brc_init_reset(encode_state, &curbe_params);
  media_unmap_buffer_obj (gpe_ctx->dynamic_state.res.bo);

  /* surface & binding table */
  media_drv_memset (&surface_params, sizeof (surface_params));
  surface_params.cacheability_control = CACHEABILITY_TYPE_LLC;
  media_add_binding_table (gpe_ctx);
  media_surface_state_vp8_brc_init_reset (encoder_context,
					  encode_state,
					  &surface_params);

  /* kernels */
  batch = media_batchbuffer_new (&drv_ctx->drv_data, I915_EXEC_RENDER, 0);
  media_drv_generic_kernel_cmds (ctx,
				 encoder_context,
				 batch,
				 gpe_ctx,
				 &kernel_params);

  /* object command */
  media_object_params.interface_offset = 0;
  media_object_params.use_scoreboard = 0;
  media_object_cmd(batch, &media_object_params);

  media_batchbuffer_submit (batch);
}

VOID
mediadrv_gen_encode_brc_update(VADriverContextP ctx,
                               MEDIA_ENCODER_CTX * encoder_context,
                               struct encode_state *encode_state)
{
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  MEDIA_BRC_UPDATE_PARAMS_VP8 curbe_params;
  BRC_UPDATE_CONTEXT *brc_update_context = &encoder_context->brc_update_context;
  MEDIA_GPE_CTX *gpe_ctx = &brc_update_context->gpe_context;
  BRC_UPDATE_SURFACE_PARAMS_VP8 surface_params;
  GENERIC_KERNEL_PARAMS kernel_params;
  MEDIA_BATCH_BUFFER *batch;
  MEDIA_OBJECT_PARAMS media_object_params;
  VAEncPictureParameterBufferVP8 *pic_param =
    (VAEncPictureParameterBufferVP8 *) encode_state->pic_param_ext->buffer;
  MEDIA_MBENC_CURBE_PARAMS_VP8 mbenc_curbe_params;
  MBENC_CONTEXT *mbenc_ctx = &encoder_context->mbenc_context;
  MEDIA_GPE_CTX *mbenc_gpe_ctx = &mbenc_ctx->gpe_context;
  MEDIA_MBPAK_CURBE_PARAMS_VP8 mbpak_curbe_params;
  MBPAK_CONTEXT *mbpak_ctx = &encoder_context->mbpak_context;
  MEDIA_GPE_CTX *mbpak_gpe_ctx;
  BRC_INIT_RESET_CONTEXT *brc_init_reset_context = &encoder_context->brc_init_reset_context;
  BRC_UPDATE_CONSTANT_DATA_PARAMS_VP8 constant_data_params;

  VOID *buf = NULL;
  UINT ref_frame_flag_final, ref_frame_flag;

  /* setup mbenc curbe ??? */
  mbenc_curbe_params.kernel_mode = encoder_context->kernel_mode;
  mbenc_curbe_params.mb_enc_iframe_dist_in_use = FALSE;
  mbenc_curbe_params.updated = 0; // encoder_context->mbenc_curbe_set_brc_update;
  mbenc_curbe_params.hme_enabled = encode_state->hme_enabled;
  mbenc_curbe_params.brc_enabled = encoder_context->brc_enabled;

  if (encoder_context->pic_coding_type == FRAME_TYPE_P) {
    ref_frame_flag = 0x07;

    if (pic_param->ref_last_frame == pic_param->ref_gf_frame) {
      ref_frame_flag &= ~GOLDEN_REF_FLAG_VP8;
    }

    if (pic_param->ref_last_frame == pic_param->ref_arf_frame) {
      ref_frame_flag &= ~ALT_REF_FLAG_VP8;
    }

    if (pic_param->ref_gf_frame == pic_param->ref_arf_frame) {
      ref_frame_flag &= ~ALT_REF_FLAG_VP8;
    }
  } else {
    ref_frame_flag = 1;
  }

  switch (encoder_context->ref_frame_ctrl) {
  case 0:
    ref_frame_flag_final = 0;
    break;

  case 1:
    ref_frame_flag_final = 1;	//Last Ref only
    break;

  case 2:
    ref_frame_flag_final = 2;	//Gold Ref only
    break;

  case 4:
    ref_frame_flag_final = 4;	//Alt Ref only
    break;

  default:
    ref_frame_flag_final =
      (ref_frame_flag & encoder_context->ref_frame_ctrl) ?
      (ref_frame_flag & encoder_context->ref_frame_ctrl) : 0x1;
  }

  encoder_context->ref_frame_ctrl = ref_frame_flag_final;
  mbenc_curbe_params.ref_frame_ctrl = encoder_context->ref_frame_ctrl;

  buf = media_map_buffer_obj (mbenc_gpe_ctx->dynamic_state.res.bo);
  mbenc_curbe_params.curbe_cmd_buff = buf;

  if (encoder_context->pic_coding_type == FRAME_TYPE_I) {
    encoder_context->set_curbe_i_vp8_mbenc (encode_state, &mbenc_curbe_params);
  } else if (encoder_context->pic_coding_type == FRAME_TYPE_P) {
    encoder_context->set_curbe_p_vp8_mbenc (encode_state, &mbenc_curbe_params);
  }

  media_unmap_buffer_obj (mbenc_gpe_ctx->dynamic_state.res.bo);

  encoder_context->mbenc_curbe_set_brc_update = TRUE;

  /* setup mbpak curbe ??? */
  mbpak_gpe_ctx = &mbpak_ctx->gpe_context;
  mbpak_curbe_params.curbe_cmd_buff =
    media_map_buffer_obj (mbpak_gpe_ctx->dynamic_state.res.bo);
  mbpak_curbe_params.updated = 0;
  mbpak_curbe_params.pak_phase_type = MBPAK_HYBRID_STATE_P1;
  encoder_context->set_curbe_vp8_mbpak (encode_state, &mbpak_curbe_params);
  media_unmap_buffer_obj (mbpak_gpe_ctx->dynamic_state.res.bo);

  mbpak_gpe_ctx = &mbpak_ctx->gpe_context2;
  mbpak_curbe_params.curbe_cmd_buff =
    media_map_buffer_obj (mbpak_gpe_ctx->dynamic_state.res.bo);
  mbpak_curbe_params.updated = 0;
  mbpak_curbe_params.pak_phase_type = MBPAK_HYBRID_STATE_P2;
  encoder_context->set_curbe_vp8_mbpak (encode_state, &mbpak_curbe_params);
  media_unmap_buffer_obj (mbpak_gpe_ctx->dynamic_state.res.bo);

  encoder_context->mbpak_curbe_set_brc_update = TRUE;

  /* kernel id */
  kernel_params.idrt_kernel_offset = BRC_UPDATE_OFFSET;

  /* binding table */
  gpe_ctx->surface_state_binding_table =
    brc_update_context->surface_state_binding_table_brc_update;

  /* brc update curbe */
  curbe_params.frame_width_in_mbs = encoder_context->picture_width_in_mbs;
  curbe_params.frame_height_in_mbs = encoder_context->picture_height_in_mbs;
  curbe_params.avbr_accuracy = encoder_context->avbr_accuracy;
  curbe_params.avbr_convergence = encoder_context->avbr_convergence;
  curbe_params.hme_enabled = encode_state->hme_enabled;
  curbe_params.brc_initted = encoder_context->brc_initted;
  curbe_params.kernel_mode = encoder_context->kernel_mode;
  curbe_params.pic_coding_type = encoder_context->pic_coding_type;
  curbe_params.frame_number = encoder_context->frame_num;
  curbe_params.brc_init_current_target_buf_full_in_bits =
    &encoder_context->brc_init_current_target_buf_full_in_bits;
  curbe_params.brc_init_reset_buf_size_in_bits =
    encoder_context->brc_init_reset_buf_size_in_bits;
  curbe_params.brc_init_reset_input_bits_per_frame =
    encoder_context->brc_init_reset_input_bits_per_frame;
  curbe_params.frame_update = &encoder_context->frame_update;

  curbe_params.curbe_cmd_buff =
    media_map_buffer_obj (gpe_ctx->dynamic_state.res.bo);
  media_set_curbe_vp8_brc_update(encode_state, &curbe_params);
  media_unmap_buffer_obj (gpe_ctx->dynamic_state.res.bo);

  /* init constant data surface */
  constant_data_params.brc_update_constant_data = &brc_init_reset_context->brc_constant_data;
  media_encode_init_brc_update_constant_data_vp8_g75(&constant_data_params);

  /* surface & binding table */
  media_drv_memset (&surface_params, sizeof (surface_params));
  surface_params.cacheability_control = CACHEABILITY_TYPE_LLC;
  media_add_binding_table (gpe_ctx);
  media_surface_state_vp8_brc_update (encoder_context,
				      encode_state,
				      &surface_params);

  /* kernels */
  batch = media_batchbuffer_new (&drv_ctx->drv_data, I915_EXEC_RENDER, 0);
  media_drv_generic_kernel_cmds (ctx,
				 encoder_context,
				 batch,
				 gpe_ctx,
				 &kernel_params);

  /* object command */
  media_object_params.interface_offset = 0;
  media_object_params.use_scoreboard = 0;
  media_object_cmd(batch, &media_object_params);
  media_batchbuffer_submit (batch);
}

VAStatus
media_encode_kernel_functions (VADriverContextP ctx,
			       VAProfile profile,
			       struct encode_state *encode_state,
			       MEDIA_ENCODER_CTX * encoder_context)
{
  VAStatus status = VA_STATUS_SUCCESS;
  SCALING_KERNEL_PARAMS scaling_params;

  scaling_params.scaling_16x_en = 0;
  scaling_params.scaling_32x_en = 0;
  encoder_context->mbenc_curbe_set_brc_update = FALSE;
  encoder_context->mbpak_curbe_set_brc_update = FALSE;

  if (encoder_context->brc_enabled) {
    if (encoder_context->pic_coding_type == FRAME_TYPE_I) {
      if (!encoder_context->brc_initted ||
	  encoder_context->brc_need_reset) {
	mediadrv_gen_encode_brc_init_reset(ctx, encoder_context, encode_state);
      }
    }
  }

#if 0
  if (encoder_context->scaling_enabled == 1)
    {
      mediadrv_gen_encode_scaling (ctx, encoder_context, encode_state,
				   &scaling_params, FALSE);
      if (encoder_context->me_16x_supported == 1)
	{
	  scaling_params.scaling_16x_en = 1;
	  mediadrv_gen_encode_scaling (ctx, encoder_context, encode_state,
				       &scaling_params, TRUE);
	}
    }

  if (encode_state->hme_enabled)
    {
      if (encode_state->me_16x_enabled)
	{
	  mediadrv_gen_encode_me (ctx, encoder_context, encode_state, FALSE);
	}
      mediadrv_gen_encode_me (ctx, encoder_context, encode_state, TRUE);
    }

#endif

  if (encoder_context->brc_enabled) {
    if (encoder_context->brc_distortion_buffer_supported &&
	encoder_context->pic_coding_type == FRAME_TYPE_I) {
      mediadrv_gen_encode_mbenc (ctx, encoder_context, encode_state, FALSE,
				 TRUE);
    }

    mediadrv_gen_encode_brc_update(ctx, encoder_context, encode_state);
  }

  encoder_context->brc_initted = 1;

  mediadrv_gen_encode_mbenc (ctx, encoder_context, encode_state, FALSE,
			     FALSE);

  if ((encoder_context->pic_coding_type == FRAME_TYPE_I)
      && (encoder_context->mbenc_chroma_kernel == TRUE))
    {
      //phase2
      mediadrv_gen_encode_mbenc (ctx, encoder_context, encode_state, TRUE,
				 FALSE);
    }

  //PAK 
  if (encoder_context->pic_coding_type == FRAME_TYPE_P)
    {
      mediadrv_gen_encode_mbpak (ctx, encoder_context, encode_state,
				 MBPAK_HYBRID_STATE_P1);
    }
  mediadrv_gen_encode_mbpak (ctx, encoder_context, encode_state,
			     MBPAK_HYBRID_STATE_P2);

  if (encoder_context->brc_enabled) {
    encoder_context->mbenc_curbe_set_brc_update = FALSE;
    encoder_context->mbpak_curbe_set_brc_update = FALSE;
  }

  return status;
}

VOID
media_encode_mb_layout_vp8 (MEDIA_ENCODER_CTX * encoder_context, VOID * data,
			    UINT * data_size)
{

  VAEncMbDataLayout *mb_layout = (VAEncMbDataLayout *) data;

  mb_layout->MbCodeSize = MB_CODE_SIZE_VP8;
  mb_layout->MbCodeStride = MB_CODE_SIZE_VP8 * sizeof (UINT);
  mb_layout->MvNumber = 16;
  mb_layout->MvStride = 16 * sizeof (UINT);

  mb_layout->MvOffset = encoder_context->mv_offset;
  mb_layout->MbCodeOffset = encoder_context->mb_data_offset;

  *data_size = sizeof (VAEncMbDataLayout);
}

VOID
media_verify_input_params (struct encode_state *encode_state)
{
#if 0
  VAEncPictureParameterBufferVP8 *pic_param =
    (VAEncPictureParameterBufferVP8 *) encode_state->pic_param_ext->buffer;
  VAEncSequenceParameterBufferVP8 *seq_param =
    (VAEncSequenceParameterBufferVP8 *) encode_state->seq_param_ext->buffer;
  VAQMatrixBufferVP8 *quant_params =
    (VAQMatrixBufferVP8 *) encode_state->q_matrix->buffer;
  INT i;

  printf ("media_drv_encoder_initialize seq_param->frame_width=%d\n",
	  seq_param->frame_width);
  printf ("media_drv_encoder_initialize seq_param->frame_height=%d\n",
	  seq_param->frame_height);
  printf ("media_drv_encoder_initialize seq_param->frame_width_scale=%d\n",
	  seq_param->frame_width_scale);
  printf ("media_drv_encoder_initialize seq_param->frame_height_scale=%d\n",
	  seq_param->frame_height_scale);
  printf ("media_drv_encoder_initialize seq_param->error_resilient=%d\n",
	  seq_param->error_resilient);
  printf ("media_drv_encoder_initialize seq_param->kf_auto=%d\n",
	  seq_param->kf_auto);
  printf ("media_drv_encoder_initialize seq_param->kf_min_dist=%d\n",
	  seq_param->kf_min_dist);
  printf ("media_drv_encoder_initialize seq_param->kf_max_dist=%d\n",
	  seq_param->kf_max_dist);
  printf ("media_drv_encoder_initialize seq_param->bits_per_second=%d\n",
	  seq_param->bits_per_second);
  printf ("media_drv_encoder_initialize seq_param->intra_period=%d\n",
	  seq_param->intra_period);
  printf ("media_drv_encoder_initialize:pic_param->reconstructed_frame=%x\n",
	  pic_param->reconstructed_frame);
  printf ("media_drv_encoder_initialize:pic_param->ref_last_frame=%x\n",
	  pic_param->ref_last_frame);
  printf ("media_drv_encoder_initialize:pic_param->ref_gf_frame=%x\n",
	  pic_param->ref_gf_frame);
  printf ("media_drv_encoder_initialize:pic_param->ref_arf_frame=%x\n",
	  pic_param->ref_arf_frame);
  printf ("media_drv_encoder_initialize:pic_param->coded_buf=%x\n",
	  pic_param->coded_buf);
#endif
}

VOID
media_get_seq_params_vp8_encode (VADriverContextP ctx,
				 MEDIA_ENCODER_CTX * encoder_context,
				 struct encode_state *encode_state)
{
  VAEncSequenceParameterBufferVP8 *seq_param =
    (VAEncSequenceParameterBufferVP8 *) encode_state->seq_param_ext->buffer;
/*if frame width and height is know at context init time can we move this to context init*/
/*is it required to update frame width and height for each frame*/
  encoder_context->picture_width_in_mbs =
    (UINT) WIDTH_IN_MACROBLOCKS (seq_param->frame_width);
  encoder_context->picture_height_in_mbs =
    (UINT) HEIGHT_IN_MACROBLOCKS (seq_param->frame_height);

  encoder_context->frame_width = encoder_context->picture_width_in_mbs * 16;
  encoder_context->frame_height = encoder_context->picture_height_in_mbs * 16;

  encoder_context->down_scaled_width_mb4x =
    WIDTH_IN_MACROBLOCKS (encoder_context->frame_width / SCALE_FACTOR_4x);
  encoder_context->down_scaled_height_mb4x =
    HEIGHT_IN_MACROBLOCKS (encoder_context->frame_height / SCALE_FACTOR_4x);
  encoder_context->down_scaled_frame_field_height_mb4x =
    encoder_context->down_scaled_height_mb4x;
  encoder_context->down_scaled_frame_field_width_mb4x =
    encoder_context->down_scaled_width_mb4x;

  encoder_context->down_scaled_width_mb16x =
    WIDTH_IN_MACROBLOCKS (encoder_context->frame_width / SCALE_FACTOR_16x);
  encoder_context->down_scaled_height_mb16x =
    HEIGHT_IN_MACROBLOCKS (encoder_context->frame_height / SCALE_FACTOR_16x);
  encoder_context->down_scaled_frame_field_height_mb16x =
    encoder_context->down_scaled_height_mb16x;
  encoder_context->down_scaled_frame_field_width_mb16x =
    encoder_context->down_scaled_width_mb16x;


  encoder_context->down_scaled_width_mb32x =
    WIDTH_IN_MACROBLOCKS (encoder_context->frame_width / SCALE_FACTOR_32x);
  encoder_context->down_scaled_height_mb32x =
    HEIGHT_IN_MACROBLOCKS (encoder_context->frame_height / SCALE_FACTOR_32x);
  encoder_context->down_scaled_frame_field_height_mb32x =
    encoder_context->down_scaled_height_mb32x;
  encoder_context->down_scaled_frame_field_width_mb32x =
    encoder_context->down_scaled_width_mb32x;

  encoder_context->gop_pic_size = seq_param->intra_period;
  encoder_context->target_bit_rate = seq_param->bits_per_second;
  if (encoder_context->rate_control_mode == VA_RC_VBR &&
      encoder_context->max_bit_rate == 0)
    encoder_context->max_bit_rate = encoder_context->target_bit_rate;
}

static VAStatus
media_get_pic_params_vp8_encode (VADriverContextP ctx,
				 MEDIA_ENCODER_CTX * encoder_context,
				 struct encode_state *encode_state)
{
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;

  VAEncPictureParameterBufferVP8 *pic_param =
    (VAEncPictureParameterBufferVP8 *) encode_state->pic_param_ext->buffer;
  INT picture_coding_type = pic_param->pic_flags.bits.frame_type;
  struct object_surface *obj_surface;
  CHAR *coded_buf;

  encode_state->hme_enabled = encoder_context->hme_supported
    && picture_coding_type != FRAME_TYPE_I;
  encode_state->me_16x_enabled = encoder_context->me_16x_supported
    && picture_coding_type != FRAME_TYPE_I;
  encoder_context->pic_coding_type =
    picture_coding_type ? FRAME_TYPE_P : FRAME_TYPE_I;
  if (encode_state->me_16x_enabled)
    {
      encode_state->me_16x_done = FALSE;
    }
  if (encode_state->hme_enabled)
    {
      encode_state->hme_done = FALSE;
    }
  if (pic_param->pic_flags.bits.frame_type == 0)
    {
      encoder_context->ref_frame_ctrl = 0;
    }
  else
    {
      if (encoder_context->disable_multi_ref == 0)
	{
	  encoder_context->ref_frame_ctrl =
	    (!pic_param->ref_flags.bits.
	     no_ref_last) | ((!pic_param->ref_flags.bits.
			      no_ref_gf) << 1) | ((!pic_param->ref_flags.bits.
						   no_ref_arf) << 2);
	}
      else
	{
	  //FIXME:multired ref frame is not enabled for BYT now.
	  encoder_context->ref_frame_ctrl = 1;
	}
    }
  //hybrid 
  obj_surface = SURFACE (pic_param->coded_buf);
  MEDIA_DRV_ASSERT (obj_surface && obj_surface->bo);
  if (!obj_surface || !obj_surface->bo)
    return VA_STATUS_ERROR_INVALID_PARAMETER;

  encode_state->coded_buf_surface = obj_surface;
  coded_buf =
    (CHAR *) media_map_buffer_obj (encode_state->coded_buf_surface->bo);
  media_drv_memset (coded_buf, encode_state->coded_buf_surface->bo->size);
  media_unmap_buffer_obj (encode_state->coded_buf_surface->bo);

  //ref frame
  if (pic_param->ref_last_frame != VA_INVALID_SURFACE)
    {
      obj_surface = SURFACE (pic_param->ref_last_frame);

      if (obj_surface->bo)
	encode_state->ref_last_frame = obj_surface;
      else
	encode_state->ref_last_frame = NULL;
    }
  else
    {
      encode_state->ref_last_frame = NULL;
      //return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

  if (pic_param->ref_gf_frame != VA_INVALID_SURFACE)
    {
      obj_surface = SURFACE (pic_param->ref_gf_frame);
      if (obj_surface->bo)
	encode_state->ref_gf_frame = obj_surface;
      else
	encode_state->ref_gf_frame = NULL;
    }
  else
    {
      encode_state->ref_gf_frame = NULL;
      //return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

  if (pic_param->ref_arf_frame != VA_INVALID_SURFACE)
    {
      obj_surface = SURFACE (pic_param->ref_arf_frame);
      if (obj_surface->bo)
	encode_state->ref_arf_frame = obj_surface;
      else
	encode_state->ref_arf_frame = NULL;
    }
  else
    {
      encode_state->ref_arf_frame = NULL;
      //return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
  if (pic_param->reconstructed_frame != VA_INVALID_SURFACE)
    {
      obj_surface = SURFACE (pic_param->reconstructed_frame);
      if (obj_surface->bo)
	encode_state->reconstructed_object = obj_surface;
      else
	encode_state->reconstructed_object = NULL;
    }
  else
    {
      encode_state->reconstructed_object = NULL;
      //return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
  return VA_STATUS_SUCCESS;
}

VOID
media_get_frame_update_params_vp8_encode (VADriverContextP ctx,
                                          MEDIA_ENCODER_CTX *encoder_context,
                                          VAEncMiscParameterVP8HybridFrameUpdate *misc);
static VOID
media_get_frame_update_vp8_encode (VADriverContextP ctx,
				   MEDIA_ENCODER_CTX * encoder_context,
				   struct encode_state *encode_state)
{
  VAEncMiscParameterVP8HybridFrameUpdate *frame_update;

  if (!encode_state->frame_update_param || !encode_state->frame_update_param->buffer)
    return;

  frame_update = (VAEncMiscParameterVP8HybridFrameUpdate *) encode_state->frame_update_param->buffer;

  media_get_frame_update_params_vp8_encode (ctx, encoder_context, frame_update);
}

VOID
media_get_hrd_params_vp8_encode (VADriverContextP ctx,
                                 MEDIA_ENCODER_CTX *encoder_context,
                                 VAEncMiscParameterHRD *misc)
{
  encoder_context->internal_rate_mode = HB_BRC_CBR;
  encoder_context->vbv_buffer_size_in_bit = misc->buffer_size;
  encoder_context->init_vbv_buffer_fullness_in_bit = misc->initial_buffer_fullness;
}

VOID
media_get_frame_rate_params_vp8_encode (VADriverContextP ctx,
                                        MEDIA_ENCODER_CTX *encoder_context,
                                        VAEncMiscParameterFrameRate *misc)
{
  encoder_context->frame_rate = misc->framerate;
}

VOID
media_get_rate_control_params_vp8_encode (VADriverContextP ctx,
                                          MEDIA_ENCODER_CTX *encoder_context,
                                          VAEncMiscParameterRateControl *misc)
{
  UINT max_bit_rate, target_bit_rate;

  max_bit_rate = encoder_context->max_bit_rate;
  target_bit_rate = encoder_context->target_bit_rate;
  encoder_context->max_bit_rate = misc->bits_per_second;

  if (encoder_context->rate_control_mode == VA_RC_CBR) {
    encoder_context->internal_rate_mode = HB_BRC_CBR;
    encoder_context->min_bit_rate = encoder_context->max_bit_rate;
  } else if (encoder_context->rate_control_mode == VA_RC_VBR) {
    encoder_context->internal_rate_mode = HB_BRC_VBR;
    encoder_context->min_bit_rate = encoder_context->max_bit_rate * (2 * misc->target_percentage - 100) / 100;
    encoder_context->target_bit_rate = encoder_context->max_bit_rate * misc->target_percentage / 100;

    if ((target_bit_rate != encoder_context->target_bit_rate) ||
	(max_bit_rate != encoder_context->max_bit_rate))
      encoder_context->brc_need_reset = 1;
  }
}

VOID
media_get_private_params_vp8_encode (VADriverContextP ctx,
                                     MEDIA_ENCODER_CTX *encoder_context,
                                     VOID  *misc)
{
    // do nothing
}

VOID
media_get_frame_update_params_vp8_encode (VADriverContextP ctx,
                                          MEDIA_ENCODER_CTX *encoder_context,
                                          VAEncMiscParameterVP8HybridFrameUpdate *misc)
{
  memcpy(&encoder_context->frame_update, misc, sizeof(MEDIA_FRAME_UPDATE));
}


static VAStatus
media_get_misc_params_vp8_encode (VADriverContextP ctx,
                                  MEDIA_ENCODER_CTX * encoder_context,
                                  struct encode_state *encode_state)
{
  int i;
  VAEncMiscParameterType type;
  VAEncMiscParameterBuffer *misc_param;

  for (i = 0; i < ARRAY_ELEMS(encode_state->misc_param); i++) {
    if (!encode_state->misc_param[i])
      continue;

    type = media_drv_index_to_va_misc_type(i);

    if (type == (VAEncMiscParameterType)(-1000))
      return VA_STATUS_ERROR_UNKNOWN;

    misc_param = (VAEncMiscParameterBuffer *)encode_state->misc_param[i]->buffer;
    assert (misc_param->type == type);

    switch (type) {
    case VAEncMiscParameterTypeHRD:
      media_get_hrd_params_vp8_encode(ctx,
				      encoder_context,
				      (VAEncMiscParameterHRD *)misc_param->data);
      break;

    case VAEncMiscParameterTypeFrameRate:
      media_get_frame_rate_params_vp8_encode(ctx,
					     encoder_context,
					     (VAEncMiscParameterFrameRate *)misc_param->data);
      break;

    case VAEncMiscParameterTypeRateControl:
      media_get_rate_control_params_vp8_encode(ctx,
					       encoder_context,
					       (VAEncMiscParameterRateControl *)misc_param->data);
      break;

    case VAEncMiscParameterTypePrivate:
      media_get_private_params_vp8_encode(ctx,
					  encoder_context,
					  misc_param->data);
      break;

    case VAEncMiscParameterTypeVP8HybridFrameUpdate:
      media_get_frame_update_params_vp8_encode(ctx,
					       encoder_context,
					       (VAEncMiscParameterVP8HybridFrameUpdate *)misc_param->data);
      break;

    default:
      break;
    }
  }

  return VA_STATUS_SUCCESS;
}

static VAStatus
media_encoder_get_yuv_surface (VADriverContextP ctx,
			       VAProfile profile,
			       struct encode_state *encode_state,
			       MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  struct object_surface *obj_surface;
  /* releae the temporary surface */
  if (encoder_context->is_tmp_id)
    {
      media_DestroySurfaces (ctx, &encoder_context->input_yuv_surface, 1);
      encode_state->input_yuv_object = NULL;
    }

  encoder_context->is_tmp_id = 0;
  obj_surface = SURFACE (encode_state->current_render_target);
  assert (obj_surface && obj_surface->bo);

  if (!obj_surface || !obj_surface->bo)
    return VA_STATUS_ERROR_INVALID_PARAMETER;

  if (obj_surface->fourcc == VA_FOURCC ('N', 'V', '1', '2'))
    {
      UINT tiling = 0, swizzle = 0;
      dri_bo_get_tiling (obj_surface->bo, &tiling, &swizzle);

      if (tiling == I915_TILING_Y)
	{
	  encoder_context->input_yuv_surface =
	    encode_state->current_render_target;
	  encode_state->input_yuv_object = obj_surface;
	  return VA_STATUS_SUCCESS;
	}
    }
  else
    {
      /*FIXME:handle this later..!!returining error for now.! */
      return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
  return VA_STATUS_SUCCESS;
}

static VOID
media_encoder_free_priv_surface(void **data)
{
  MEDIA_ENCODER_VP8_SURFACE *vp8_surface;

  if (!data || *data)
    return;

  vp8_surface = *data;

  if (vp8_surface->scaled_4x_surface_obj) {
    MEDIA_DRV_ASSERT(vp8_surface->scaled_4x_surface_id != VA_INVALID_SURFACE);
    media_DestroySurfaces(vp8_surface->ctx, &vp8_surface->scaled_4x_surface_id, 1);
    vp8_surface->scaled_4x_surface_id = VA_INVALID_SURFACE;
    vp8_surface->scaled_4x_surface_obj = NULL;
  }
}

static VOID
media_encoder_init_priv_surfaces(VADriverContextP ctx,
				 MEDIA_ENCODER_CTX * encoder_context,
				 struct object_surface *obj_surface)
{
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  MEDIA_ENCODER_VP8_SURFACE *vp8_surface;
  int down_scaled_width4x, down_scaled_height4x;

  if (!obj_surface || obj_surface->private_data)
    return;

  vp8_surface = calloc(1, sizeof(MEDIA_ENCODER_VP8_SURFACE));
  vp8_surface->ctx = ctx;

  if (encoder_context->brc_enabled) {
    down_scaled_width4x = encoder_context->down_scaled_width_mb4x * 16;
    down_scaled_height4x = encoder_context->down_scaled_height_mb4x * 16;
    media_CreateSurfaces (ctx,
			  down_scaled_width4x, down_scaled_height4x,
			  VA_FOURCC ('N', 'V', '1', '2'), 1,
			  &vp8_surface->scaled_4x_surface_id);
    vp8_surface->scaled_4x_surface_obj = SURFACE(vp8_surface->scaled_4x_surface_id);
    MEDIA_DRV_ASSERT(vp8_surface->scaled_4x_surface_obj &&
		     vp8_surface->scaled_4x_surface_obj->bo);
  }

  obj_surface->private_data = vp8_surface;
  obj_surface->free_private_data = media_encoder_free_priv_surface;
}

VOID
media_alloc_binding_surface_state (MEDIA_DRV_CONTEXT * drv_ctx,
				   SURFACE_STATE_BINDING_TABLE *
				   surface_state_binding_table)
{
  surface_state_binding_table->res.bo_size =
    (SURFACE_STATE_PADDED_SIZE + sizeof (UINT)) * MAX_MEDIA_SURFACES_GEN6;
  //surface_state_binding_table->table_name = "binding table me";
  media_allocate_resource (&surface_state_binding_table->res,
			   drv_ctx->drv_data.bufmgr,
			   /*"surface state & binding table" */
			   (const BYTE *) surface_state_binding_table->
			   table_name,
			   surface_state_binding_table->res.bo_size, 4096);
  MEDIA_DRV_ASSERT (surface_state_binding_table->res.bo);
}

VOID
media_free_binding_surface_state (SURFACE_STATE_BINDING_TABLE *
				  surface_state_binding_table)
{
  dri_bo_unreference (surface_state_binding_table->res.bo);
  surface_state_binding_table->res.bo = NULL;
}

VOID
media_mbpak_kernel_pic_resource_dinit (MEDIA_ENCODER_CTX * encoder_context)
{
  MBPAK_CONTEXT *mbpak_ctx = &encoder_context->mbpak_context;
  MEDIA_GPE_CTX *mbpak_gpe_ctx = &mbpak_ctx->gpe_context;
  media_free_binding_surface_state
    (&mbpak_ctx->surface_state_binding_table_mbpak_p1);
  media_free_binding_surface_state
    (&mbpak_ctx->surface_state_binding_table_mbpak_p2);
  mbpak_gpe_ctx->surface_state_binding_table.res.bo = NULL;

  mbpak_gpe_ctx = &mbpak_ctx->gpe_context2;
  mbpak_gpe_ctx->surface_state_binding_table.res.bo = NULL;
}

VOID
media_mbenc_kernel_pic_resource_dinit (MEDIA_ENCODER_CTX * encoder_context)
{
  MBENC_CONTEXT *mbenc_ctx = &encoder_context->mbenc_context;
  MEDIA_GPE_CTX *mbenc_gpe_ctx = &mbenc_ctx->gpe_context;
  media_free_binding_surface_state
    (&mbenc_ctx->surface_state_binding_table_mbenc_p1);
  media_free_binding_surface_state
    (&mbenc_ctx->surface_state_binding_table_mbenc_p2);
  media_free_binding_surface_state
    (&mbenc_ctx->surface_state_binding_table_mbenc_iframe_dist);
  mbenc_gpe_ctx->surface_state_binding_table.res.bo = NULL;
}

VOID
media_me_kernel_pic_resource_dinit (MEDIA_ENCODER_CTX * encoder_context)
{
  ME_CONTEXT *me_ctx = &encoder_context->me_context;
  MEDIA_GPE_CTX *me_gpe_ctx = &me_ctx->gpe_context;
  media_free_binding_surface_state
    (&me_ctx->surface_state_binding_table_4x_me);
  media_free_binding_surface_state
    (&me_ctx->surface_state_binding_table_16x_me);
  me_gpe_ctx->surface_state_binding_table.res.bo = NULL;
}

VOID
media_scaling_kernel_pic_resource_dinit (MEDIA_ENCODER_CTX * encoder_context)
{
  SCALING_CONTEXT *scaling_ctx = &encoder_context->scaling_context;
  media_free_binding_surface_state
    (&scaling_ctx->surface_state_binding_table_scaling);
  media_free_binding_surface_state
    (&scaling_ctx->surface_state_binding_table_scaling_16x);
}

VOID
media_brc_init_reset_kernel_pic_resource_dinit (MEDIA_ENCODER_CTX * encoder_context)
{
  BRC_INIT_RESET_CONTEXT *brc_init_reset_context = &encoder_context->brc_init_reset_context;
  MEDIA_GPE_CTX *gpe_context = &brc_init_reset_context->gpe_context;

  media_free_binding_surface_state
    (&brc_init_reset_context->surface_state_binding_table_brc_init_reset);
  gpe_context->surface_state_binding_table.res.bo = NULL;
}

VOID
media_brc_update_kernel_pic_resource_dinit (MEDIA_ENCODER_CTX * encoder_context)
{
  BRC_UPDATE_CONTEXT *brc_update_context = &encoder_context->brc_update_context;
  MEDIA_GPE_CTX *gpe_context = &brc_update_context->gpe_context;

  media_free_binding_surface_state
    (&brc_update_context->surface_state_binding_table_brc_update);
  gpe_context->surface_state_binding_table.res.bo = NULL;
}

static VAStatus
media_kernel_dinit (VADriverContextP ctx,
		    MEDIA_ENCODER_CTX * encoder_context,
		    struct encode_state *encode_state)
{
  VAStatus status = VA_STATUS_SUCCESS;
  media_scaling_kernel_pic_resource_dinit (encoder_context);
  media_me_kernel_pic_resource_dinit (encoder_context);
  media_mbenc_kernel_pic_resource_dinit (encoder_context);
  media_mbpak_kernel_pic_resource_dinit (encoder_context);
  media_brc_init_reset_kernel_pic_resource_dinit (encoder_context);
  media_brc_update_kernel_pic_resource_dinit (encoder_context);
  return status;
}

VOID
media_mbpak_kernel_pic_resource_init (VADriverContextP ctx,
				      MEDIA_ENCODER_CTX * encoder_context,
				      struct encode_state * encode_state)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  MBPAK_CONTEXT *mbpak_ctx = &encoder_context->mbpak_context;
  mbpak_ctx->surface_state_binding_table_mbpak_p1.table_name =
    "surface state binding table mbpak p1";
  media_alloc_binding_surface_state (drv_ctx,
				     &mbpak_ctx->surface_state_binding_table_mbpak_p1);
  mbpak_ctx->surface_state_binding_table_mbpak_p2.table_name =
    "surface state binding table mbpak p2";
  media_alloc_binding_surface_state (drv_ctx,
				     &mbpak_ctx->surface_state_binding_table_mbpak_p2);
}

VOID
media_mbenc_kernel_pic_resource_init (VADriverContextP ctx,
				      MEDIA_ENCODER_CTX * encoder_context,
				      struct encode_state *encode_state)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  MBENC_CONTEXT *mbenc_ctx = &encoder_context->mbenc_context;
  mbenc_ctx->surface_state_binding_table_mbenc_p1.table_name =
    "surface state binding table mbenc p1";
  media_alloc_binding_surface_state (drv_ctx,
				     &mbenc_ctx->surface_state_binding_table_mbenc_p1);
  mbenc_ctx->surface_state_binding_table_mbenc_p2.table_name =
    "surface state binding table mbenc p2";
  media_alloc_binding_surface_state (drv_ctx,
				     &mbenc_ctx->surface_state_binding_table_mbenc_p2);

  mbenc_ctx->surface_state_binding_table_mbenc_iframe_dist.table_name =
    "surface state binding table mbenc iframe dist";
  media_alloc_binding_surface_state (drv_ctx,
				     &mbenc_ctx->
				     surface_state_binding_table_mbenc_iframe_dist);
}

VOID
media_me_kernel_pic_resource_init (VADriverContextP ctx,
				   MEDIA_ENCODER_CTX * encoder_context,
				   struct encode_state *encode_state)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  ME_CONTEXT *me_ctx = &encoder_context->me_context;
  me_ctx->surface_state_binding_table_4x_me.table_name =
    "surface state binding table 4x me";
  media_alloc_binding_surface_state (drv_ctx,
				     &me_ctx->surface_state_binding_table_4x_me);
  me_ctx->surface_state_binding_table_16x_me.table_name =
    "surface state binding table 16x me";
  media_alloc_binding_surface_state (drv_ctx,
				     &me_ctx->surface_state_binding_table_16x_me);
}

VOID
media_scaling_kernel_pic_resource_init (VADriverContextP ctx,
					MEDIA_ENCODER_CTX * encoder_context,
					struct encode_state *encode_state)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  SCALING_CONTEXT *scaling_ctx = &encoder_context->scaling_context;
  scaling_ctx->surface_state_binding_table_scaling.table_name =
    "surface state binding table scaling";
  media_alloc_binding_surface_state (drv_ctx,
				     &scaling_ctx->surface_state_binding_table_scaling);
  scaling_ctx->surface_state_binding_table_scaling_16x.table_name =
    "surface state binding table scaling 16x";
  media_alloc_binding_surface_state (drv_ctx,
				     &scaling_ctx->surface_state_binding_table_scaling_16x);
}

VOID
media_brc_init_reset_kernel_pic_resource_init (VADriverContextP ctx,
                                               MEDIA_ENCODER_CTX * encoder_context,
                                               struct encode_state *encode_state)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  BRC_INIT_RESET_CONTEXT *brc_init_reset_context = &encoder_context->brc_init_reset_context;

  brc_init_reset_context->surface_state_binding_table_brc_init_reset.table_name =
    "surface state binding table brc init reset";
  media_alloc_binding_surface_state (drv_ctx,
				     &brc_init_reset_context->surface_state_binding_table_brc_init_reset);
}

VOID
media_brc_update_kernel_pic_resource_init (VADriverContextP ctx,
                                           MEDIA_ENCODER_CTX * encoder_context,
                                           struct encode_state *encode_state)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  BRC_UPDATE_CONTEXT *brc_update_context = &encoder_context->brc_update_context;

  brc_update_context->surface_state_binding_table_brc_update.table_name =
    "surface state binding table brc distortion";
  media_alloc_binding_surface_state (drv_ctx,
				     &brc_update_context->surface_state_binding_table_brc_update);
}

static VAStatus
media_kernel_init (VADriverContextP ctx,
		   MEDIA_ENCODER_CTX * encoder_context,
		   struct encode_state *encode_state)
{
  VAStatus status = VA_STATUS_SUCCESS;
  media_scaling_kernel_pic_resource_init (ctx, encoder_context, encode_state);
  media_me_kernel_pic_resource_init (ctx, encoder_context, encode_state);
  media_mbenc_kernel_pic_resource_init (ctx, encoder_context, encode_state);
  media_mbpak_kernel_pic_resource_init (ctx, encoder_context, encode_state);
  media_brc_init_reset_kernel_pic_resource_init(ctx, encoder_context, encode_state);
  media_brc_update_kernel_pic_resource_init(ctx, encoder_context, encode_state);
  return status;
}

static VAStatus
media_encoder_picture_init (VADriverContextP ctx, VAProfile profile,
			    MEDIA_ENCODER_CTX * encoder_context,
			    struct encode_state *encode_state)
{
  VAStatus status = VA_STATUS_SUCCESS;
  MEDIA_ENCODER_VP8_SURFACE *vp8_surface;
  VAQMatrixBufferVP8 *quant_params =
    (VAQMatrixBufferVP8 *) encode_state->q_matrix->buffer;

#ifdef DEBUG
  media_verify_input_params (encode_state);
#endif
  status =
    media_get_pic_params_vp8_encode (ctx, encoder_context, encode_state);
  if (status != VA_STATUS_SUCCESS)
    return status;
  media_get_seq_params_vp8_encode (ctx, encoder_context, encode_state);

  media_get_frame_update_vp8_encode (ctx, encoder_context, encode_state);

  status =
    media_encoder_get_yuv_surface (ctx, profile, encode_state,
				   encoder_context);
  if (status != VA_STATUS_SUCCESS)
    return status;

  status =
    media_get_misc_params_vp8_encode (ctx, encoder_context, encode_state);

  if (status != VA_STATUS_SUCCESS)
    return status;

  if (VA_RC_CQP == encoder_context->rate_control_mode) {
    encoder_context->internal_rate_mode = HB_BRC_CQP;
    encoder_context->target_bit_rate = 0;
    encoder_context->max_bit_rate = 0;
    encoder_context->min_bit_rate = 0;
    encoder_context->init_vbv_buffer_fullness_in_bit = 0;
    encoder_context->vbv_buffer_size_in_bit = 0;
  } else if (VA_RC_CBR == encoder_context->rate_control_mode) {
    encoder_context->internal_rate_mode = HB_BRC_CBR;
    encoder_context->max_bit_rate = encoder_context->target_bit_rate;
    encoder_context->min_bit_rate = encoder_context->target_bit_rate;
    encoder_context->init_vbv_buffer_fullness_in_bit = encoder_context->target_bit_rate;
    encoder_context->vbv_buffer_size_in_bit = encoder_context->target_bit_rate;
  } else if (VA_RC_VBR == encoder_context->rate_control_mode) {
    encoder_context->internal_rate_mode = HB_BRC_VBR;
  }

  encoder_context->init_brc_distortion_buffer = 0;
  media_encoder_init_priv_surfaces(ctx,
				   encoder_context,
				   encode_state->reconstructed_object);
  vp8_surface = encode_state->reconstructed_object->private_data;
  vp8_surface->qp_index = quant_params->quantization_index[0];

  if (encoder_context->brc_enabled) {
    if (encoder_context->pic_coding_type == FRAME_TYPE_I)
      encoder_context->init_brc_distortion_buffer = 1;
    else {
      if (encoder_context->frame_num % encoder_context->gop_pic_size == 1)
	encoder_context->init_brc_distortion_buffer = 1;
    }
  }

  media_kernel_init (ctx, encoder_context, encode_state);

  return status;
}

VAStatus
media_encoder_picture (VADriverContextP ctx,
		       VAProfile profile,
		       union codec_state * codec_state,
		       struct hw_context * hw_context)
{
  MEDIA_ENCODER_CTX *encoder_context = (MEDIA_ENCODER_CTX *) hw_context;
  struct encode_state *encode_state = &codec_state->encode;
  VAStatus status = VA_STATUS_SUCCESS;
#ifdef DEBUG
  printf ("media_encoder_picture\n");
#endif
  status =
    media_encoder_picture_init (ctx, profile, encoder_context, encode_state);
#if 0
  if (status != VA_STATUS_SUCCESS)
    return status;
#endif
  status =
    media_encode_kernel_functions (ctx, profile, encode_state,
				   encoder_context);
#if 0
  if (status != VA_STATUS_SUCCESS)
    return status;
#endif
  status = media_kernel_dinit (ctx, encoder_context, encode_state);
#if 0
  if (status != VA_STATUS_SUCCESS)
    return status;
#endif

  encoder_context->frame_num = encoder_context->frame_num + 1;
  encoder_context->brc_need_reset = 0;
  return status;
}
