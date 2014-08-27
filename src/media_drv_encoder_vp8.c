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

#include "media_drv_encoder.h"
#include "media_drv_encoder_vp8.h"
#include "media_drv_driver.h"
#include "media_drv_hw_g75.h"
#include "media_drv_hw_g7.h"
#include "media_drv_hwcmds.h"
VOID
gpe_context_vfe_scoreboardinit_vp8 (MEDIA_GPE_CTX * gpe_context)
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

VOID
gpe_context_vfe_scoreboardinit_pak_vp8 (MEDIA_GPE_CTX * gpe_context)
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
media_alloc_resource_mbpak (VADriverContextP ctx,
			    MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  MBPAK_CONTEXT *mbpak_context = &encoder_context->mbpak_context;
  /*UINT pic_w_h_in_mb =
     (encoder_context->picture_width_in_mbs *
     ((encoder_context->picture_height_in_mbs + 1) >> 1)) * 2; */
  //row buffer y
  mbpak_context->row_buffer_y.surface_array_spacing = 0x1;
  mbpak_context->row_buffer_y.tiling = I915_TILING_NONE;
  mbpak_context->row_buffer_y.bo_size =
    encoder_context->picture_width_in_mbs * 16;
  media_allocate_resource (&mbpak_context->row_buffer_y,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "row_buffer y",
			   mbpak_context->row_buffer_y.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->row_buffer_y.bo);

  //row buffer uv
  mbpak_context->row_buffer_uv.surface_array_spacing = 0x1;
  mbpak_context->row_buffer_uv.tiling = I915_TILING_NONE;
  mbpak_context->row_buffer_uv.bo_size =
    encoder_context->picture_width_in_mbs * 16;
  media_allocate_resource (&mbpak_context->row_buffer_uv,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "row_buffer uv",
			   mbpak_context->row_buffer_uv.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->row_buffer_uv.bo);

  //column buffer y
  mbpak_context->column_buffer_y.surface_array_spacing = 0x1;
  mbpak_context->column_buffer_y.tiling = I915_TILING_NONE;
  mbpak_context->column_buffer_y.bo_size =
    encoder_context->picture_height_in_mbs * 16;
  media_allocate_resource (&mbpak_context->column_buffer_y,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "column buffer y",
			   mbpak_context->column_buffer_y.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->column_buffer_y.bo);

  //column buffer uv
  mbpak_context->column_buffer_uv.surface_array_spacing = 0x1;
  mbpak_context->column_buffer_uv.tiling = I915_TILING_NONE;
  mbpak_context->column_buffer_uv.bo_size =
    encoder_context->picture_height_in_mbs * 16;
  media_allocate_resource (&mbpak_context->column_buffer_uv,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "column buffer uv",
			   mbpak_context->column_buffer_uv.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->column_buffer_uv.bo);

  mbpak_context->kernel_dump_buffer.surface_array_spacing = 0x1;
  mbpak_context->kernel_dump_buffer.tiling = I915_TILING_NONE;
  mbpak_context->kernel_dump_buffer.bo_size = KERNEL_DUMP_SIZE_VP8;	//pic_w_h_in_mb;
  media_allocate_resource (&mbpak_context->kernel_dump_buffer,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "kernel dump buffer mbpak",
			   mbpak_context->kernel_dump_buffer.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->kernel_dump_buffer.bo);
}

VOID
media_alloc_resource_mbenc (VADriverContextP ctx,
			    MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  MBENC_CONTEXT *mbenc_context = &encoder_context->mbenc_context;
  UINT sz = 0;
  UINT pic_w_h_in_mb;
  UINT num_of_mb_blocks =
    encoder_context->picture_width_in_mbs *
    encoder_context->picture_height_in_mbs;
  pic_w_h_in_mb = num_of_mb_blocks;	/*(encoder_context->picture_width_in_mbs *
					   ((encoder_context->picture_height_in_mbs + 1) >> 1)) * 2; */
  if (encoder_context->brc_enabled)
    {
      encoder_context->mv_offset =
	ALIGN (((pic_w_h_in_mb + 0) * MB_CODE_SIZE_VP8 * 4), 0x1000);
    }
  else
    {
      encoder_context->mv_offset =
	ALIGN ((pic_w_h_in_mb * MB_CODE_SIZE_VP8 * 4), 0x1000);
    }
  mbenc_context->mb_mode_cost_luma_buffer.width =
    ALIGN ((sizeof (UINT) * 10), 64);
  mbenc_context->mb_mode_cost_luma_buffer.height = 1;
  mbenc_context->mb_mode_cost_luma_buffer.surface_array_spacing = 0x1;
  mbenc_context->mb_mode_cost_luma_buffer.tiling = I915_TILING_NONE;
  mbenc_context->mb_mode_cost_luma_buffer.pitch =
    mbenc_context->mb_mode_cost_luma_buffer.width;

  media_allocate_resource (&mbenc_context->mb_mode_cost_luma_buffer,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "mb mode cost luma buffer", 0x1000,
			   4096);
  MEDIA_DRV_ASSERT (mbenc_context->mb_mode_cost_luma_buffer.bo);

  mbenc_context->block_mode_cost_buffer.width =
    ALIGN ((sizeof (UINT16)) * 10 * 10 * 10, 64);
  mbenc_context->block_mode_cost_buffer.height = 1;
  mbenc_context->block_mode_cost_buffer.surface_array_spacing = 0x1;
  mbenc_context->block_mode_cost_buffer.tiling = I915_TILING_NONE;
  mbenc_context->block_mode_cost_buffer.pitch =
    mbenc_context->block_mode_cost_buffer.width;
  media_allocate_resource (&mbenc_context->block_mode_cost_buffer,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "block mode cost buffer", 0x1000,
			   4096);
  MEDIA_DRV_ASSERT (mbenc_context->block_mode_cost_buffer.bo);

  mbenc_context->chroma_reconst_buffer.width = 64;
  mbenc_context->chroma_reconst_buffer.height = num_of_mb_blocks;
  mbenc_context->chroma_reconst_buffer.surface_array_spacing = 0x1;
  mbenc_context->chroma_reconst_buffer.tiling = I915_TILING_NONE;
  mbenc_context->chroma_reconst_buffer.pitch =
    mbenc_context->chroma_reconst_buffer.width;
  sz =
    mbenc_context->chroma_reconst_buffer.width *
    mbenc_context->chroma_reconst_buffer.height;
  media_allocate_resource (&mbenc_context->chroma_reconst_buffer,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "chrome reconst buffer", sz, 4096);
  MEDIA_DRV_ASSERT (mbenc_context->chroma_reconst_buffer.bo);


  mbenc_context->histogram_buffer.surface_array_spacing = 0x1;
  mbenc_context->histogram_buffer.tiling = I915_TILING_NONE;
  mbenc_context->histogram_buffer.bo_size = HISTOGRAM_SIZE;
  media_allocate_resource (&mbenc_context->histogram_buffer,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "histogram buffer", HISTOGRAM_SIZE,
			   4096);
  MEDIA_DRV_ASSERT (mbenc_context->histogram_buffer.bo);

  mbenc_context->kernel_dump_buffer.surface_array_spacing = 0x1;
  mbenc_context->kernel_dump_buffer.tiling = I915_TILING_NONE;
  mbenc_context->kernel_dump_buffer.bo_size = KERNEL_DUMP_SIZE_VP8;	// pic_w_h_in_mb;
  media_allocate_resource (&mbenc_context->kernel_dump_buffer,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "kernel dump buffer",
			   mbenc_context->kernel_dump_buffer.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbenc_context->kernel_dump_buffer.bo);

  mbenc_context->ref_frm_count_surface.surface_array_spacing = 0x1;
  mbenc_context->ref_frm_count_surface.tiling = I915_TILING_NONE;
  mbenc_context->ref_frm_count_surface.bo_size = 32;
  media_allocate_resource (&mbenc_context->ref_frm_count_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "reference frame mb count surface",
			   mbenc_context->ref_frm_count_surface.bo_size,
			   4096);
  MEDIA_DRV_ASSERT (mbenc_context->ref_frm_count_surface.bo);

  mbenc_context->pred_mv_data_surface.surface_array_spacing = 0x1;
  mbenc_context->pred_mv_data_surface.tiling = I915_TILING_NONE;
  mbenc_context->pred_mv_data_surface.bo_size =
    4 * encoder_context->picture_width_in_mbs *
    encoder_context->picture_height_in_mbs * sizeof (UINT);
  media_allocate_resource (&mbenc_context->pred_mv_data_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "pred mv data surface",
			   mbenc_context->pred_mv_data_surface.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbenc_context->pred_mv_data_surface.bo);

  mbenc_context->mode_cost_update_surface.surface_array_spacing = 0x1;
  mbenc_context->mode_cost_update_surface.tiling = I915_TILING_NONE;
  mbenc_context->mode_cost_update_surface.bo_size = 16 * sizeof (UINT);
  media_allocate_resource (&mbenc_context->mode_cost_update_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "mode cost update surface",
			   mbenc_context->mode_cost_update_surface.bo_size,
			   4096);
  MEDIA_DRV_ASSERT (mbenc_context->mode_cost_update_surface.bo);

  mbenc_context->pred_mb_quant_data_surface.surface_array_spacing = 0x1;
  mbenc_context->pred_mb_quant_data_surface.tiling = I915_TILING_NONE;
  mbenc_context->pred_mb_quant_data_surface.width =
    ALIGN ((encoder_context->picture_width_in_mbs * 4), 64);
  mbenc_context->pred_mb_quant_data_surface.height =
    encoder_context->picture_height_in_mbs;
  mbenc_context->pred_mb_quant_data_surface.pitch =
    mbenc_context->pred_mb_quant_data_surface.width;
  sz =
    mbenc_context->pred_mb_quant_data_surface.width *
    mbenc_context->pred_mb_quant_data_surface.height;
  media_allocate_resource (&mbenc_context->pred_mb_quant_data_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "pred mb quant data surface", sz,
			   4096);
  MEDIA_DRV_ASSERT (mbenc_context->pred_mb_quant_data_surface.bo);
}

VOID
media_alloc_resource_me (VADriverContextP ctx,
			 MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  ME_CONTEXT *me_context = &encoder_context->me_context;

  me_context->mv_data_surface_4x_me.width =
    ALIGN ((encoder_context->down_scaled_width_mb4x * 32), 64);
  me_context->mv_data_surface_4x_me.height =
    encoder_context->down_scaled_height_mb4x * 4;
  me_context->mv_data_surface_4x_me.surface_array_spacing = 0x1;
  me_context->mv_data_surface_4x_me.tiling = I915_TILING_NONE;
  me_context->mv_data_surface_4x_me.pitch =
    me_context->mv_data_surface_4x_me.width;
  media_allocate_resource (&me_context->mv_data_surface_4x_me,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "mv data surface 4x_me",
			   (me_context->mv_data_surface_4x_me.width *
			    me_context->mv_data_surface_4x_me.height), 4096);
  MEDIA_DRV_ASSERT (me_context->mv_data_surface_4x_me.bo);


  me_context->mv_distortion_surface_4x_me.width =
    ALIGN ((encoder_context->down_scaled_width_mb4x * 8), 64);
  me_context->mv_distortion_surface_4x_me.height =
    ALIGN ((encoder_context->down_scaled_height_mb4x * 4), 64);
  me_context->mv_distortion_surface_4x_me.surface_array_spacing = 0x1;
  me_context->mv_distortion_surface_4x_me.tiling = I915_TILING_NONE;
  me_context->mv_distortion_surface_4x_me.pitch =
    ALIGN ((me_context->mv_distortion_surface_4x_me.width * 8), 64);
  media_allocate_resource (&me_context->mv_distortion_surface_4x_me,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "mv distortion surface 4x_me",
			   0x1000, 4096);
  MEDIA_DRV_ASSERT (me_context->mv_distortion_surface_4x_me.bo);

  me_context->mv_data_surface_16x_me.width =
    ALIGN ((encoder_context->down_scaled_width_mb16x * 32), 64);
  me_context->mv_data_surface_16x_me.height =
    encoder_context->down_scaled_height_mb16x * 4;
  me_context->mv_data_surface_16x_me.surface_array_spacing = 0x1;
  me_context->mv_data_surface_16x_me.tiling = I915_TILING_NONE;
  me_context->mv_data_surface_16x_me.pitch =
    me_context->mv_data_surface_16x_me.width;
  media_allocate_resource (&me_context->mv_data_surface_16x_me,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "mv data surface 16x_me", 0x1000,
			   4096);
  MEDIA_DRV_ASSERT (me_context->mv_data_surface_16x_me.bo);

}

static VOID
media_encoder_context_params_init (MEDIA_DRV_CONTEXT * drv_ctx,
				   MEDIA_ENCODER_CTX * encoder_context)
{
  if (IS_HSW_GT1 (drv_ctx->drv_data.device_id)
      || (IS_IVYBRIDGE (drv_ctx->drv_data.device_id)))
    {
      encoder_context->walker_mode = SINGLE_MODE;
    }
  else if (IS_HSW_GT2 (drv_ctx->drv_data.device_id))
    {
      encoder_context->walker_mode = DUAL_MODE;
    }
  else if (IS_HSW_GT3 (drv_ctx->drv_data.device_id))
    {
      encoder_context->walker_mode = QUAD_MODE;
    }

  encoder_context->kernel_mode = 0;
  encoder_context->frame_num = 0;
  encoder_context->use_hw_scoreboard = 1;
  encoder_context->scaling_enabled = 0;
  encoder_context->me_16x_supported = 0;
  encoder_context->hme_supported = 0;
  encoder_context->kernel_dump_enable = 0;

  if (encoder_context->hme_supported == 1)
    {
      encoder_context->scaling_enabled = 1;
      encoder_context->me_16x_supported = 1;
    }
  encoder_context->mbenc_chroma_kernel = TRUE;
  encoder_context->mbenc_curbe_set_brc_update = FALSE;
  encoder_context->frame_width = encoder_context->picture_width;
  encoder_context->frame_height = encoder_context->picture_height;

  encoder_context->picture_width_in_mbs =
    (UINT) WIDTH_IN_MACROBLOCKS (encoder_context->picture_width);
  encoder_context->picture_height_in_mbs =
    (UINT) HEIGHT_IN_MACROBLOCKS (encoder_context->picture_height);
  encoder_context->down_scaled_width_mb4x =
    WIDTH_IN_MACROBLOCKS (encoder_context->picture_width / SCALE_FACTOR_4x);
  encoder_context->down_scaled_height_mb4x =
    HEIGHT_IN_MACROBLOCKS (encoder_context->picture_height / SCALE_FACTOR_4x);
  encoder_context->down_scaled_width_mb16x =
    WIDTH_IN_MACROBLOCKS (encoder_context->picture_width / SCALE_FACTOR_16x);
  encoder_context->down_scaled_height_mb16x =
    HEIGHT_IN_MACROBLOCKS (encoder_context->picture_height /
			   SCALE_FACTOR_16x);
  encoder_context->down_scaled_width_mb32x =
    WIDTH_IN_MACROBLOCKS (encoder_context->picture_width / SCALE_FACTOR_32x);
  encoder_context->down_scaled_height_mb32x =
    HEIGHT_IN_MACROBLOCKS (encoder_context->picture_height /
			   SCALE_FACTOR_32x);
}

VOID
media_mbpak_context_init_vp8 (VADriverContextP ctx,
			      MEDIA_ENCODER_CTX * encoder_context)
{
  MBPAK_CONTEXT *mbpak_context = &encoder_context->mbpak_context;
  MEDIA_GPE_CTX *gpe_context = &mbpak_context->gpe_context;
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = /* 0xc0; */ CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 280 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 120;	//16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE - 1;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels[6], 2);

  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	// * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;	//0xco
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_mbpak (encoder_context);
  media_alloc_resource_mbpak (ctx, encoder_context);
  return;
}

VOID
media_mbenc_context_init (VADriverContextP ctx,
			  MEDIA_ENCODER_CTX * encoder_context)
{
  MBENC_CONTEXT *mbenc_context = &encoder_context->mbenc_context;
  MEDIA_GPE_CTX *gpe_context = &mbenc_context->gpe_context;
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 280 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 120;	//121;    //16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE - 1;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels[0], 3);
  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	//* MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_mbenc (encoder_context);
  media_alloc_resource_mbenc (ctx, encoder_context);
  return;
}

VOID
media_me_context_init (VADriverContextP ctx,
		       MEDIA_ENCODER_CTX * encoder_context)
{
  ME_CONTEXT *me_context = &encoder_context->me_context;
  MEDIA_GPE_CTX *gpe_context = &me_context->gpe_context;

  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 280 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 121;	//16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE - 1;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  /*media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels[3], 1); */
#if 0
  gpe_context->surface_state_binding_table.res.bo_size =
    (SURFACE_STATE_PADDED_SIZE + sizeof (UINT)) * MAX_MEDIA_SURFACES_GEN6;
  gpe_context->surface_state_binding_table.table_name = "binding table me";
#endif
  gpe_context->idrt_size =
    sizeof (struct media_interface_descriptor_data) * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);

  media_alloc_resource_me (ctx, encoder_context);
}

VOID
media_alloc_resource_scaling (VADriverContextP ctx,
			      MEDIA_ENCODER_CTX * encoder_context)
{
  UINT down_scaled_width4x, downscaled_height_4x;
  UINT down_scaled_width16x, downscaled_height_16x;
  UINT down_scaled_width32x, downscaled_height_32x;
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  SCALING_CONTEXT *scaling_context = &encoder_context->scaling_context;
  down_scaled_width4x = encoder_context->frame_width / 4;
  downscaled_height_4x = encoder_context->frame_height / 4;
  down_scaled_width4x = ALIGN (down_scaled_width4x, 16);
  downscaled_height_4x = (downscaled_height_4x + 1) >> 1;
  downscaled_height_4x = 2 * ALIGN (downscaled_height_4x, 32);

  scaling_context->scaled_4x_surface.width = down_scaled_width4x;
  scaling_context->scaled_4x_surface.height = downscaled_height_4x;
  scaling_context->scaled_4x_surface.surface_array_spacing = 0x1;
  scaling_context->scaled_4x_surface.tiling = I915_TILING_NONE;
  scaling_context->scaled_4x_surface.pitch = 0x80;	//hardcoded..need to fix this later
  media_allocate_resource (&scaling_context->scaled_4x_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "scaled surface 4x", 0x1000, 4096);
  MEDIA_DRV_ASSERT (scaling_context->scaled_4x_surface.bo);

  down_scaled_width16x = encoder_context->frame_width / 16;
  downscaled_height_16x = encoder_context->frame_height / 16;
  down_scaled_width16x = ALIGN (down_scaled_width16x, 16);
  downscaled_height_16x = (downscaled_height_16x + 1) >> 1;
  downscaled_height_16x = 2 * ALIGN (downscaled_height_16x, 32);

  scaling_context->scaled_16x_surface.width = down_scaled_width16x;
  scaling_context->scaled_16x_surface.height = downscaled_height_16x;
  scaling_context->scaled_16x_surface.surface_array_spacing = 0x1;
  scaling_context->scaled_16x_surface.tiling = I915_TILING_NONE;
  scaling_context->scaled_16x_surface.pitch = 0x80;	//hardcoded..need to fix this later
  media_allocate_resource (&scaling_context->scaled_16x_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "scaled surface 16x", 0x1000, 4096);
  MEDIA_DRV_ASSERT (scaling_context->scaled_16x_surface.bo);

  down_scaled_width32x = encoder_context->frame_width / 32;
  downscaled_height_32x = encoder_context->frame_height / 32;
  down_scaled_width32x = ALIGN (down_scaled_width32x, 16);
  downscaled_height_32x = (downscaled_height_32x + 1) >> 1;
  downscaled_height_32x = 2 * ALIGN (downscaled_height_32x, 32);

  scaling_context->scaled_32x_surface.width = down_scaled_width32x;
  scaling_context->scaled_32x_surface.height = downscaled_height_32x;
  scaling_context->scaled_32x_surface.surface_array_spacing = 0x1;
  scaling_context->scaled_32x_surface.tiling = I915_TILING_NONE;
  scaling_context->scaled_32x_surface.pitch = 0x80;	//hardcoded..need to fix this later
  media_allocate_resource (&scaling_context->scaled_32x_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "scaled surface 32x", 0x1000, 4096);
  MEDIA_DRV_ASSERT (scaling_context->scaled_32x_surface.bo);

}

VOID
media_scaling_context_init (VADriverContextP ctx,
			    MEDIA_ENCODER_CTX * encoder_context)
{
  SCALING_CONTEXT *scaling_context = &encoder_context->scaling_context;
  MEDIA_GPE_CTX *gpe_context = &scaling_context->gpe_context;

  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 280 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 121;	//16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE - 1;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);

  /*media_gpe_load_kernels (ctx, gpe_context, media_hybrid_vp8_kernels, 1); */
#if 0
  gpe_context->surface_state_binding_table.res.bo_size =
    (SURFACE_STATE_PADDED_SIZE + sizeof (UINT)) * MAX_MEDIA_SURFACES_GEN6;
#endif
  gpe_context->idrt_size =
    sizeof (struct media_interface_descriptor_data) * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_alloc_resource_scaling (ctx, encoder_context);
}

void
media_encoder_init_vp8 (VADriverContextP ctx,
			MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  encoder_context->codec = CODEC_VP8;
  //encoder_context->picture_width = picture_width;
  //encoder_context->picture_height = picture_height;

  if (IS_HASWELL (drv_ctx->drv_data.device_id))
    {
      encoder_context->num_of_kernels =
	sizeof (media_hybrid_vp8_kernels) / sizeof (MEDIA_KERNEL);
      encoder_context->disable_multi_ref = 0;
      media_encoder_context_params_init (drv_ctx, encoder_context);
      media_scaling_context_init (ctx, encoder_context);
      media_me_context_init (ctx, encoder_context);
      media_mbenc_context_init (ctx, encoder_context);
      media_mbpak_context_init_vp8 (ctx, encoder_context);
      encoder_context->set_curbe_i_vp8_mbenc = media_set_curbe_i_vp8_mbenc;
      encoder_context->set_curbe_p_vp8_mbenc = media_set_curbe_p_vp8_mbenc;
      encoder_context->set_curbe_vp8_mbpak = media_set_curbe_vp8_mbpak;
      encoder_context->surface_state_vp8_mbenc =
	media_surface_state_vp8_mbenc;
      encoder_context->surface_state_vp8_mbpak =
	media_surface_state_vp8_mbpak;
    }
  else if (IS_GEN7 (drv_ctx->drv_data.device_id))
    {
      encoder_context->num_of_kernels =
	sizeof (media_hybrid_vp8_kernels_g7) / sizeof (MEDIA_KERNEL);
      encoder_context->disable_multi_ref = 1;
      media_encoder_context_params_init (drv_ctx, encoder_context);
      media_scaling_context_init (ctx, encoder_context);
      media_me_context_init (ctx, encoder_context);
      media_mbenc_context_init_g7 (ctx, encoder_context);
      media_mbpak_context_init_vp8_g7 (ctx, encoder_context);
      encoder_context->set_curbe_i_vp8_mbenc = media_set_curbe_i_vp8_mbenc_g7;
      encoder_context->set_curbe_p_vp8_mbenc = media_set_curbe_p_vp8_mbenc_g7;
      encoder_context->set_curbe_vp8_mbpak = media_set_curbe_vp8_mbpak_g7;
      encoder_context->surface_state_vp8_mbenc =
	media_surface_state_vp8_mbenc_g7;
      encoder_context->surface_state_vp8_mbpak =
	media_surface_state_vp8_mbpak_g7;
    }
}
