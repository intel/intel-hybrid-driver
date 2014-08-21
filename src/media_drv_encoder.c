/*
 * Copyright © 2009 Intel Corporation
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

static VOID
media_encoder_context_destroy (VOID *hw_context)
{
  MEDIA_ENCODER_CTX *encoder_context = (MEDIA_ENCODER_CTX *) hw_context;
  media_scaling_context_destroy (encoder_context);
  media_me_context_destroy (encoder_context);
  media_mbenc_context_destroy (encoder_context);
  media_mbpak_context_destroy (encoder_context);
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

static VOID
media_encoder_context_params_init (MEDIA_DRV_CONTEXT * drv_ctx,
				   MEDIA_ENCODER_CTX * encoder_context)
{
  if (IS_HSW_GT1 (drv_ctx->drv_data.device_id))
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
media_alloc_resource_mbpak (VADriverContextP ctx,
			    MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  MBPAK_CONTEXT *mbpak_context = &encoder_context->mbpak_context;
  /*UINT pic_w_h_in_mb =
    (encoder_context->picture_width_in_mbs *
     ((encoder_context->picture_height_in_mbs + 1) >> 1)) * 2;*/
  //row buffer y
  mbpak_context->row_buffer_y.surface_array_spacing = 0x1;
  mbpak_context->row_buffer_y.tiling = I915_TILING_NONE;
  mbpak_context->row_buffer_y.bo_size =
    encoder_context->picture_width_in_mbs * 16;
  media_allocate_resource (&mbpak_context->row_buffer_y,
			   drv_ctx->drv_data.bufmgr,(const BYTE*) "row_buffer y",
			   mbpak_context->row_buffer_y.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->row_buffer_y.bo);

  //row buffer uv
  mbpak_context->row_buffer_uv.surface_array_spacing = 0x1;
  mbpak_context->row_buffer_uv.tiling = I915_TILING_NONE;
  mbpak_context->row_buffer_uv.bo_size =
    encoder_context->picture_width_in_mbs * 16;
  media_allocate_resource (&mbpak_context->row_buffer_uv,
			   drv_ctx->drv_data.bufmgr,(const BYTE*) "row_buffer uv",
			   mbpak_context->row_buffer_uv.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->row_buffer_uv.bo);

  //column buffer y
  mbpak_context->column_buffer_y.surface_array_spacing = 0x1;
  mbpak_context->column_buffer_y.tiling = I915_TILING_NONE;
  mbpak_context->column_buffer_y.bo_size =
    encoder_context->picture_height_in_mbs * 16;
  media_allocate_resource (&mbpak_context->column_buffer_y,
			   drv_ctx->drv_data.bufmgr,(const BYTE*) "column buffer y",
			   mbpak_context->column_buffer_y.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->column_buffer_y.bo);

  //column buffer uv
  mbpak_context->column_buffer_uv.surface_array_spacing = 0x1;
  mbpak_context->column_buffer_uv.tiling = I915_TILING_NONE;
  mbpak_context->column_buffer_uv.bo_size =
    encoder_context->picture_height_in_mbs * 16;
  media_allocate_resource (&mbpak_context->column_buffer_uv,
			   drv_ctx->drv_data.bufmgr,(const BYTE*) "column buffer uv",
			   mbpak_context->column_buffer_uv.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->column_buffer_uv.bo);

  mbpak_context->kernel_dump_buffer.surface_array_spacing = 0x1;
  mbpak_context->kernel_dump_buffer.tiling = I915_TILING_NONE;
  mbpak_context->kernel_dump_buffer.bo_size = KERNEL_DUMP_SIZE_VP8;	//pic_w_h_in_mb;
  media_allocate_resource (&mbpak_context->kernel_dump_buffer,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE*)"kernel dump buffer mbpak",
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
			   (const BYTE*)"mb mode cost luma buffer", 0x1000, 4096);
  MEDIA_DRV_ASSERT (mbenc_context->mb_mode_cost_luma_buffer.bo);

  mbenc_context->block_mode_cost_buffer.width =
    ALIGN ((sizeof (UINT16)) * 10 * 10 * 10, 64);
  mbenc_context->block_mode_cost_buffer.height = 1;
  mbenc_context->block_mode_cost_buffer.surface_array_spacing = 0x1;
  mbenc_context->block_mode_cost_buffer.tiling = I915_TILING_NONE;
  mbenc_context->block_mode_cost_buffer.pitch =
    mbenc_context->block_mode_cost_buffer.width;
  media_allocate_resource (&mbenc_context->block_mode_cost_buffer,
			   drv_ctx->drv_data.bufmgr,(const BYTE*) "block mode cost buffer",
			   0x1000, 4096);
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
			   drv_ctx->drv_data.bufmgr,(const BYTE*) "chrome reconst buffer",
			   sz, 4096);
  MEDIA_DRV_ASSERT (mbenc_context->chroma_reconst_buffer.bo);


  mbenc_context->histogram_buffer.surface_array_spacing = 0x1;
  mbenc_context->histogram_buffer.tiling = I915_TILING_NONE;
  mbenc_context->histogram_buffer.bo_size = HISTOGRAM_SIZE;
  media_allocate_resource (&mbenc_context->histogram_buffer,
			   drv_ctx->drv_data.bufmgr,(const BYTE*) "histogram buffer",
			   HISTOGRAM_SIZE, 4096);
  MEDIA_DRV_ASSERT (mbenc_context->histogram_buffer.bo);

  mbenc_context->kernel_dump_buffer.surface_array_spacing = 0x1;
  mbenc_context->kernel_dump_buffer.tiling = I915_TILING_NONE;
  mbenc_context->kernel_dump_buffer.bo_size = KERNEL_DUMP_SIZE_VP8;	// pic_w_h_in_mb;
  media_allocate_resource (&mbenc_context->kernel_dump_buffer,
			   drv_ctx->drv_data.bufmgr,(const BYTE*) "kernel dump buffer",
			   mbenc_context->kernel_dump_buffer.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbenc_context->kernel_dump_buffer.bo);

  mbenc_context->ref_frm_count_surface.surface_array_spacing = 0x1;
  mbenc_context->ref_frm_count_surface.tiling = I915_TILING_NONE;
  mbenc_context->ref_frm_count_surface.bo_size = 32;
  media_allocate_resource (&mbenc_context->ref_frm_count_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE*)"reference frame mb count surface",
			   mbenc_context->ref_frm_count_surface.bo_size,
			   4096);
  MEDIA_DRV_ASSERT (mbenc_context->ref_frm_count_surface.bo);

  mbenc_context->pred_mv_data_surface.surface_array_spacing = 0x1;
  mbenc_context->pred_mv_data_surface.tiling = I915_TILING_NONE;
  mbenc_context->pred_mv_data_surface.bo_size =
    4 * encoder_context->picture_width_in_mbs *
    encoder_context->picture_height_in_mbs * sizeof (UINT);
  media_allocate_resource (&mbenc_context->pred_mv_data_surface,
			   drv_ctx->drv_data.bufmgr,(const BYTE*) "pred mv data surface",
			   mbenc_context->pred_mv_data_surface.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbenc_context->pred_mv_data_surface.bo);

  mbenc_context->mode_cost_update_surface.surface_array_spacing = 0x1;
  mbenc_context->mode_cost_update_surface.tiling = I915_TILING_NONE;
  mbenc_context->mode_cost_update_surface.bo_size =
    16 * sizeof (UINT);
  media_allocate_resource (&mbenc_context->mode_cost_update_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE*)"mode cost update surface",
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
			   (const BYTE*)"pred mb quant data surface", sz, 4096);
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
			   drv_ctx->drv_data.bufmgr,(const BYTE*) "mv data surface 4x_me",
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
			   (const BYTE*)"mv distortion surface 4x_me", 0x1000, 4096);
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
			   drv_ctx->drv_data.bufmgr, (const BYTE*)"mv data surface 16x_me",
			   0x1000, 4096);
  MEDIA_DRV_ASSERT (me_context->mv_data_surface_16x_me.bo);

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

VOID
media_mbpak_context_init (VADriverContextP ctx,
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
  gpe_context_vfe_scoreboardinit (gpe_context);
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
  gpe_context_vfe_scoreboardinit (gpe_context);
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
  gpe_context_vfe_scoreboardinit (gpe_context);
  /*media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels[3], 1); */
#if 0
  gpe_context->surface_state_binding_table.res.bo_size =
    (SURFACE_STATE_PADDED_SIZE +
     sizeof (UINT)) * MAX_MEDIA_SURFACES_GEN6;
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
			   drv_ctx->drv_data.bufmgr, (const BYTE*)"scaled surface 4x",
			   0x1000, 4096);
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
			   drv_ctx->drv_data.bufmgr, (const BYTE*)"scaled surface 16x",
			   0x1000, 4096);
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
			   drv_ctx->drv_data.bufmgr, (const BYTE*)"scaled surface 32x",
			   0x1000, 4096);
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
  gpe_context_vfe_scoreboardinit (gpe_context);

  /*media_gpe_load_kernels (ctx, gpe_context, media_hybrid_vp8_kernels, 1); */
#if 0
  gpe_context->surface_state_binding_table.res.bo_size =
    (SURFACE_STATE_PADDED_SIZE +
     sizeof (UINT)) * MAX_MEDIA_SURFACES_GEN6;
#endif
  gpe_context->idrt_size =
    sizeof (struct media_interface_descriptor_data) * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_alloc_resource_scaling (ctx, encoder_context);
}

BOOL
media_encoder_init (VADriverContextP ctx, MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  INT num_of_kernels;
  switch (encoder_context->codec)
    {
    case CODEC_VP8:
      //kernel_list = media_hybrid_vp8_kernels;
      num_of_kernels =
	sizeof (media_hybrid_vp8_kernels) / sizeof (MEDIA_KERNEL);
      break;
    default:
      /* never get here */
      MEDIA_DRV_ASSERT (0);
      break;
    }
  encoder_context->num_of_kernels = num_of_kernels;
  media_encoder_context_params_init (drv_ctx, encoder_context);
  media_scaling_context_init (ctx, encoder_context);
  media_me_context_init (ctx, encoder_context);
  media_mbenc_context_init (ctx, encoder_context);
  media_mbpak_context_init (ctx, encoder_context);
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
	}
      break;
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

#ifdef STATUS_REPORT
  media_drv_status_report (ctx, batch, gpe_context);
#endif
  mediadrv_gen_pipeline_select_cmd (batch);

  state_base_addr_params.surface_state.bo =
    /*batch->buffer; */ gpe_context->surface_state_binding_table.res.bo;
  state_base_addr_params.dynamic_state.bo = gpe_context->dynamic_state.res.bo;
  state_base_addr_params.indirect_object.bo = NULL;
  state_base_addr_params.instruction_buffer.bo =
    gpe_context->instruction_state.buff_obj.bo;

  mediadrv_gen_state_base_address_cmd (batch, &state_base_addr_params);
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
  mediadrv_gen_media_vfe_state_cmd (batch, &vfe_state_params);

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
#ifdef STATUS_REPORT
  media_drv_end_status_report (ctx, batch, scaling_gpe_ctx);
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
  MEDIA_GPE_CTX *mbpak_gpe_ctx = &mbpak_ctx->gpe_context;
  MEDIA_BATCH_BUFFER *batch;
  MEDIA_MBPAK_CURBE_PARAMS_VP8 curbe_params;
  MBPAK_SURFACE_PARAMS_VP8 sutface_params;
  MEDIA_OBJ_WALKER_PARAMS media_obj_walker_params;
  GENERIC_KERNEL_PARAMS kernel_params;
  //UINT phase;
  if (pak_phase_type == MBPAK_HYBRID_STATE_P1)
    {
      mbpak_gpe_ctx->surface_state_binding_table =
	mbpak_ctx->surface_state_binding_table_mbpak_p1;
      kernel_params.idrt_kernel_offset = MBPAK_PHASE1_OFFSET;
      gpe_context_vfe_scoreboardinit (mbpak_gpe_ctx);
#ifdef DEBUG
    //  phase = 1;
#endif
    }
  else
    {
      mbpak_gpe_ctx->surface_state_binding_table =
	mbpak_ctx->surface_state_binding_table_mbpak_p2;
      kernel_params.idrt_kernel_offset = MBPAK_PHASE2_OFFSET;
      //FIXME:Need to find a better way to handle this..!
      gpe_context_vfe_scoreboardinit_pak (mbpak_gpe_ctx);
#ifdef DEBUG
      //phase = 2;
#endif
    }

  curbe_params.curbe_cmd_buff =
    media_map_buffer_obj (mbpak_gpe_ctx->dynamic_state.res.bo);
  curbe_params.updated = 0;
  curbe_params.pak_phase_type = pak_phase_type;
  media_set_curbe_vp8_mbpak (encode_state, &curbe_params);
  media_unmap_buffer_obj (mbpak_gpe_ctx->dynamic_state.res.bo);

  media_drv_memset (&sutface_params, sizeof (sutface_params));
  sutface_params.orig_frame_width = encoder_context->picture_width;
  sutface_params.orig_frame_height = encoder_context->picture_height;
  sutface_params.mbpak_phase_type = pak_phase_type;
  sutface_params.kernel_dump = encoder_context->kernel_dump_enable;
  sutface_params.kernel_dump_buffer = mbpak_ctx->kernel_dump_buffer;
  sutface_params.kernel_dump = 1;
  sutface_params.cacheability_control = CACHEABILITY_TYPE_LLC;
  media_add_binding_table (&mbpak_ctx->gpe_context);
  media_surface_state_vp8_mbpak (encoder_context, encode_state,
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
#ifdef STATUS_REPORT
  media_drv_end_status_report (ctx, batch, mbpak_gpe_ctx);
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
  UINT /*phase,*/ ref_frame_flag_final, ref_frame_flag;
  
  if (mbenc_phase_2 == FALSE)
    {
      mbenc_gpe_ctx->surface_state_binding_table =
	mbenc_ctx->surface_state_binding_table_mbenc_p1;
      if (encoder_context->pic_coding_type == FRAME_TYPE_I)
	kernel_params.idrt_kernel_offset = MBENC_ILUMA_START_OFFSET;
      else
	kernel_params.idrt_kernel_offset = MBENC_P_START_OFFSET;
      /*phase = 2;*/
    }
  else
    {
      mbenc_gpe_ctx->surface_state_binding_table =
	mbenc_ctx->surface_state_binding_table_mbenc_p2;
      kernel_params.idrt_kernel_offset = MBENC_ICHROMA_START_OFFSET;
     /* phase = 1;*/
    }

  if (mbenc_phase_2 == FALSE)
    {
      curbe_params.kernel_mode = encoder_context->kernel_mode;
      curbe_params.mb_enc_iframe_dist_in_use = FALSE;
      curbe_params.updated = encoder_context->mbenc_curbe_set_brc_update;
      curbe_params.hme_enabled = encode_state->hme_enabled;
      //curbe_params.ref_frame_ctrl = encoder_context->ref_frame_ctrl;
      curbe_params.brc_enabled = encode_state->hme_enabled;

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
	    (ref_frame_flag & encoder_context->
	     ref_frame_ctrl) ? (ref_frame_flag & encoder_context->
				ref_frame_ctrl) : 0x1;
	}
      encoder_context->ref_frame_ctrl = ref_frame_flag_final;
      curbe_params.ref_frame_ctrl = encoder_context->ref_frame_ctrl;

      if (encoder_context->pic_coding_type == FRAME_TYPE_I)
	{
	  buf = media_map_buffer_obj (mbenc_gpe_ctx->dynamic_state.res.bo);
	  curbe_params.curbe_cmd_buff = buf;
	  media_set_curbe_i_vp8_mbenc (encode_state, &curbe_params);

	  media_unmap_buffer_obj (mbenc_gpe_ctx->dynamic_state.res.bo);
	  const_buff_params.mb_mode_cost_luma_buffer =
	    &mbenc_ctx->mb_mode_cost_luma_buffer;
	  const_buff_params.block_mode_cost_buffer =
	    &mbenc_ctx->block_mode_cost_buffer;
	  media_encode_init_mbenc_constant_buffer_vp8_g75
	    (&const_buff_params);
	}
      else if (encoder_context->pic_coding_type == FRAME_TYPE_P)
	{

	  buf = media_map_buffer_obj (mbenc_gpe_ctx->dynamic_state.res.bo);
	  curbe_params.curbe_cmd_buff = buf;
	  media_drv_memset (buf, mbenc_gpe_ctx->curbe.length);
	  media_set_curbe_p_vp8_mbenc (encode_state, &curbe_params);
	  media_unmap_buffer_obj (mbenc_gpe_ctx->dynamic_state.res.bo);
	}
    }

  media_drv_memset (&mbenc_sutface_params, sizeof (mbenc_sutface_params));
  mbenc_sutface_params.pic_coding = encoder_context->pic_coding_type;
  mbenc_sutface_params.orig_frame_width = encoder_context->picture_width;
  mbenc_sutface_params.orig_frame_height = encoder_context->picture_height;
  mbenc_sutface_params.cacheability_control = CACHEABILITY_TYPE_LLC;
  mbenc_sutface_params.kernel_dump = 1;
  media_add_binding_table (&mbenc_ctx->gpe_context);

  media_surface_state_vp8_mbenc (encoder_context, encode_state,
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
#ifdef STATUS_REPORT
  media_drv_end_status_report (ctx, batch, mbenc_gpe_ctx);
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
    (me_16x) ? encoder_context->
    down_scaled_width_mb16x : encoder_context->down_scaled_width_mb4x;
  media_object_walker_cmd (batch, &media_obj_walker_params);
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
  return status;
}

VOID
media_encode_mb_layout_vp8 (MEDIA_ENCODER_CTX * encoder_context, VOID *data,
			    UINT * data_size)
{

  VAEncMbDataLayout *mb_layout = (VAEncMbDataLayout *) data;

  mb_layout->MbCodeOffset = 0;
  mb_layout->MbCodeSize = MB_CODE_SIZE_VP8;
  mb_layout->MbCodeStride = MB_CODE_SIZE_VP8 * sizeof (UINT);
  mb_layout->MvNumber = 16;
  mb_layout->MvStride = 16 * sizeof (UINT);

  mb_layout->MvOffset = encoder_context->mv_offset;

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
  printf
    ("media_drv_encoder_initialize:pic_param->ref_flags.bits.force_kf=%x\n",
     pic_param->ref_flags.bits.force_kf);
  printf
    ("media_drv_encoder_initialize:pic_param->ref_flags.bits.no_ref_last=%x\n",
     pic_param->ref_flags.bits.no_ref_last);
  printf
    ("media_drv_encoder_initialize:pic_param->ref_flags.bits.no_ref_gf=%x\n",
     pic_param->ref_flags.bits.no_ref_gf);
  printf
    ("media_drv_encoder_initialize:pic_param->ref_flags.bits.no_ref_arf=%x\n",
     pic_param->ref_flags.bits.no_ref_arf);
  printf
    ("media_drv_encoder_initialize:pic_param->ref_flags.bits.temporal_id=%x\n",
     pic_param->ref_flags.bits.temporal_id);

  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.bits.frame_type=%x\n",
     pic_param->pic_flags.bits.frame_type);
  printf ("media_drv_encoder_initialize:pic_param->pic_flags.version=%x\n",
	  pic_param->pic_flags.bits.version);
  printf ("media_drv_encoder_initialize:pic_param->pic_flags.show_frame=%x\n",
	  pic_param->pic_flags.bits.show_frame);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.color_space=%x\n",
     pic_param->pic_flags.bits.color_space);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.recon_filter_type=%x\n",
     pic_param->pic_flags.bits.recon_filter_type);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.loop_filter_type=%x\n",
     pic_param->pic_flags.bits.loop_filter_type);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.auto_partitions=%x\n",
     pic_param->pic_flags.bits.auto_partitions);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.num_token_partitions=%x\n",
     pic_param->pic_flags.bits.num_token_partitions);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.clamping_type=%x\n",
     pic_param->pic_flags.bits.clamping_type);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.segmentation_enabled=%x\n",
     pic_param->pic_flags.bits.segmentation_enabled);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.update_mb_segmentation_map=%x\n",
     pic_param->pic_flags.bits.update_mb_segmentation_map);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.update_segment_feature_data=%x\n",
     pic_param->pic_flags.bits.update_segment_feature_data);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.loop_filter_adj_enable=%x\n",
     pic_param->pic_flags.bits.loop_filter_adj_enable);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.refresh_entropy_probs=%x\n",
     pic_param->pic_flags.bits.refresh_entropy_probs);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.refresh_golden_frame=%x\n",
     pic_param->pic_flags.bits.refresh_golden_frame);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.refresh_alternate_frame=%x\n",
     pic_param->pic_flags.bits.refresh_alternate_frame);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.refresh_last=%x\n",
     pic_param->pic_flags.bits.refresh_last);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.copy_buffer_to_golden=%x\n",
     pic_param->pic_flags.bits.copy_buffer_to_golden);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.copy_buffer_to_alternate=%x\n",
     pic_param->pic_flags.bits.copy_buffer_to_alternate);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.sign_bias_golden=%x\n",
     pic_param->pic_flags.bits.sign_bias_golden);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.sign_bias_alternate=%x\n",
     pic_param->pic_flags.bits.sign_bias_alternate);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.mb_no_coeff_skip=%x\n",
     pic_param->pic_flags.bits.mb_no_coeff_skip);
  printf
    ("media_drv_encoder_initialize:pic_param->pic_flags.forced_lf_adjustment=%x\n",
     pic_param->pic_flags.bits.forced_lf_adjustment);

  printf ("media_drv_encoder_initialize:pic_param->sharpness_level=%x\n",
	  pic_param->sharpness_level);
  printf ("media_drv_encoder_initialize:pic_param->clamp_qindex_high=%x\n",
	  pic_param->clamp_qindex_high);
  printf ("media_drv_encoder_initialize:pic_param->clamp_qindex_low=%x\n",
	  pic_param->clamp_qindex_low);

  //qmatrix
  printf ("quant_params=%x\n", quant_params);
  for (i = 0; i < 4; i++)
    {
      printf ("quant_params->quantization_index[%d]=%d\n", i,
	      quant_params->quantization_index[i]);
    }

  for (i = 0; i < 5; i++)
    {
      printf ("quant_params->quantization_index_delta[%d]= %d\n", i,
	      quant_params->quantization_index_delta[i]);
    }

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
      encoder_context->ref_frame_ctrl =
	(!pic_param->ref_flags.
	 bits.no_ref_last) | ((!pic_param->ref_flags.
			       bits.no_ref_gf) << 1) | ((!pic_param->
							 ref_flags.
							 bits.no_ref_arf) <<
							2);
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

VOID
media_alloc_binding_surface_state (MEDIA_DRV_CONTEXT * drv_ctx,
				   SURFACE_STATE_BINDING_TABLE *
				   surface_state_binding_table)
{
  surface_state_binding_table->res.bo_size =
    (SURFACE_STATE_PADDED_SIZE +
     sizeof (UINT)) * MAX_MEDIA_SURFACES_GEN6;
  //surface_state_binding_table->table_name = "binding table me";
  media_allocate_resource (&surface_state_binding_table->res,
			   drv_ctx->drv_data.bufmgr,
			   /*"surface state & binding table" */
			   (const BYTE*)surface_state_binding_table->table_name,
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
  return status;
}

VOID
media_mbpak_kernel_pic_resource_init (VADriverContextP ctx,
				      MEDIA_ENCODER_CTX * encoder_context,
				      struct encode_state *encode_state)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  MBPAK_CONTEXT *mbpak_ctx = &encoder_context->mbpak_context;
  mbpak_ctx->surface_state_binding_table_mbpak_p1.table_name =
    "surface state binding table mbpak p1";
  media_alloc_binding_surface_state (drv_ctx,
				     &mbpak_ctx->
				     surface_state_binding_table_mbpak_p1);
  mbpak_ctx->surface_state_binding_table_mbpak_p2.table_name =
    "surface state binding table mbpak p2";
  media_alloc_binding_surface_state (drv_ctx,
				     &mbpak_ctx->
				     surface_state_binding_table_mbpak_p2);
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
				     &mbenc_ctx->
				     surface_state_binding_table_mbenc_p1);
  mbenc_ctx->surface_state_binding_table_mbenc_p2.table_name =
    "surface state binding table mbenc p2";
  media_alloc_binding_surface_state (drv_ctx,
				     &mbenc_ctx->
				     surface_state_binding_table_mbenc_p2);

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
				     &me_ctx->
				     surface_state_binding_table_4x_me);
  me_ctx->surface_state_binding_table_16x_me.table_name =
    "surface state binding table 16x me";
  media_alloc_binding_surface_state (drv_ctx,
				     &me_ctx->
				     surface_state_binding_table_16x_me);
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
				     &scaling_ctx->
				     surface_state_binding_table_scaling);
  scaling_ctx->surface_state_binding_table_scaling_16x.table_name =
    "surface state binding table scaling 16x";
  media_alloc_binding_surface_state (drv_ctx,
				     &scaling_ctx->
				     surface_state_binding_table_scaling_16x);
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

  return status;
}

static VAStatus
media_encoder_picture_init (VADriverContextP ctx, VAProfile profile,
			    MEDIA_ENCODER_CTX * encoder_context,
			    struct encode_state *encode_state)
{
  VAStatus status = VA_STATUS_SUCCESS;
#ifdef DEBUG
  media_verify_input_params (encode_state);
#endif
  status =
    media_get_pic_params_vp8_encode (ctx, encoder_context, encode_state);
  if (status != VA_STATUS_SUCCESS)
    return status;
  media_get_seq_params_vp8_encode (ctx, encoder_context, encode_state);
  status =
    media_encoder_get_yuv_surface (ctx, profile, encode_state,
				   encoder_context);
  if (status != VA_STATUS_SUCCESS)
    return status;
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
  return status;
}
