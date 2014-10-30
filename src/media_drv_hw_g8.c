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

#include <va/va.h>
#include <va/va_enc_vp8.h>
#include "media_drv_hw_g7.h"
#include "media_drv_hw_g8.h"
#include "media_drv_kernels_g8.h"
#include "media_drv_surface.h"
//#define DEBUG
MEDIA_KERNEL media_hybrid_vp8_kernels_g8[] = {
  {
   (BYTE *) "VP8_MBENC_I",
   0,
   MEDIA_VP8_MBENC_I_G8,
   MEDIA_VP8_MBENC_I_SZ_G8,
   NULL,
   0}
  ,
  {
   (BYTE *) "VP8_MBENC_ICHROMA",
   0,
   MEDIA_VP8_MBENC_ICHROMA_G8,
   MEDIA_VP8_MBENC_ICHROMA_SZ_G8,
   NULL,
   0}
  ,
  {
   (BYTE *) "VP8_MBENC_FRM_P",
   0,
   MEDIA_VP8_MBENC_FRM_P_G8,
   MEDIA_VP8_MBENC_FRM_P_SZ_G8,
   NULL,
   0}
  ,
  {
   (BYTE *) "VP8_PAK_PHASE2",
   0,
   MEDIA_VP8_PAK_PHASE2_G8,
   MEDIA_VP8_PAK_PHASE2_SZ_G8,
   NULL,
   0}
  ,
  {
   (BYTE *) "VP8_PAK_PHASE1",
   0,
   MEDIA_VP8_PAK_PHASE1_G8,
   MEDIA_VP8_PAK_PHASE1_SZ_G8,
   NULL,
   0}
};

const SURFACE_STATE_G8 SURFACE_STATE_INIT_G8 = {
  //dw0
  {
   FALSE,			// cube_pos_z:1;
   FALSE,			//cube_neg_z:1
   FALSE,			//cube_pos_y:1
   FALSE,			//cube_neg_y:1
   FALSE,			//cube_pos_x:1
   FALSE,			//cube_neg_x:1
   0,				// media_boundry_pix_mode:2
   0,				//render_cache_read_write:1
   0,				//reserved0
   0,				//vert_line_stride_ofs:1
   0,				//vert_line_stride:1
   0,				//tilemode
   1,				//horizontal_alignment:1;
   1,				//vertical_alignment:2;
   STATE_SURFACEFORMAT_R8_UNORM,	// surface_format:9;
   0,
   0,				//surface_array:1;
   1				//MEDIA_SURFACE_2D  // surface_type:3;
   },
  //dw1
  {
   0,
   0,
   0x20,
   0},
  //dw2
  {
   0,
   0,
   0,
   0},
  //dw3
  {
   0,
   0,
   0},
  //dw4
  {
   0,
   0,
   0,
   0,
   0,
   0,
   0},
  //dw5
  {
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0},
  //dw6
  {
   0,
   0,
   0,
   0},
  //dw7
  {
   0,
   0,
   HSW_SCS_ALPHA,
   HSW_SCS_BLUE,
   HSW_SCS_GREEN,
   HSW_SCS_RED,
   0,
   0,
   0,
   0},
  //dw8
  {
   0},
//dw9
  {
   0,
   0},
//dw10
  {
   0,
   0},
//dw11
  {
   0,
   0},
//dw12
  {
   0},
//dw 13,14,15
  0,
  0,
  0
};

const SURFACE_STATE_ADV_G8 SURFACE_STATE_ADV_INIT_G8 = {
  //dw0
  {
   0},
  {
//dw1
   0,
   0,
   0,
   0},
  {
//dw2
   0,
   0,
   0,
   0,
   0,
   0,
   0},
  {
//dw3
   0,
   0,
   0,
   0},
  {
//dw4
   0,
   0,
   0,
   0},
//dw5
  {
   0x20,
   0,
   0,
   0},
  //dw6
  {
   0},
//dw7
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
   0}
};

VOID
media_add_binding_table_g8 (MEDIA_GPE_CTX * gpe_ctx)
{
  BYTE *binding_surface_state_buf = NULL;
  UINT i;
  binding_surface_state_buf =
    (BYTE *) media_map_buffer_obj (gpe_ctx->surface_state_binding_table.res.
				   bo);
  media_drv_memset (binding_surface_state_buf,
		    gpe_ctx->surface_state_binding_table.res.bo->size);

  for (i = 0; i < LAST_BINDING_TABLE_ENTRIES; i++)
    {
      *((UINT *) ((BYTE *) binding_surface_state_buf +
		  BINDING_TABLE_OFFSET_G8 (i))) =
	SURFACE_STATE_OFFSET_G8 (i); /*<< BINDING_TABLE_SURFACE_SHIFT */ ;
    }
  media_unmap_buffer_obj (gpe_ctx->surface_state_binding_table.res.bo);
}

media_interface_setup_mbpak_g8 (MEDIA_GPE_CTX *mbpak_gpe_ctx)
{
  struct gen8_interface_descriptor_data *desc;
  INT i;
  dri_bo *bo;
  BYTE *desc_ptr;
  UINT samplet_offset;
  bo = mbpak_gpe_ctx->dynamic_state.res.bo;
  dri_bo_map (bo, 1);
  MEDIA_DRV_ASSERT (bo->virtual);
  desc_ptr = (BYTE *) bo->virtual + mbpak_gpe_ctx->idrt_offset;
  desc = (struct gen8_interface_descriptor_data *) desc_ptr;
  for (i = 0; i < mbpak_gpe_ctx->num_kernels; i++)
    {
      MEDIA_KERNEL *kernel;
      kernel = &mbpak_gpe_ctx->kernels[i];
      MEDIA_DRV_ASSERT (sizeof (*desc) == 32);
      /*Setup the descritor table */
      memset (desc, 0, sizeof (*desc));
      desc->desc0.kernel_start_pointer = kernel->kernel_offset >> 6;
#if 0
      desc->desc3.sampler_count = 4;	/* FIXME: */
      desc->desc3.sampler_state_pointer =
	(mbenc_gpe_ctx->sampler_offset +
	 (i * mbenc_gpe_ctx->sampler_size)) >> 5;;
#endif
      desc->desc4.binding_table_entry_count = 0;	//1; /* FIXME: */
      desc->desc4.binding_table_pointer = (BINDING_TABLE_OFFSET (0) >> 5);
      desc->desc5.constant_urb_entry_read_offset = 0;
      desc->desc5.constant_urb_entry_read_length = (mbpak_gpe_ctx->curbe_size + 31) >> 5;	//CURBE_URB_ENTRY_LENGTH;
      desc->desc6.num_threads_in_tg = 1;
      desc++;
      desc++;
    }
  dri_bo_unmap (bo);
}


media_interface_setup_mbenc_g8 (MEDIA_ENCODER_CTX * encoder_context)
{
  MBENC_CONTEXT *mbenc_ctx = &encoder_context->mbenc_context;
  MEDIA_GPE_CTX *mbenc_gpe_ctx = &mbenc_ctx->gpe_context;
  struct gen8_interface_descriptor_data *desc;
  INT i;
  dri_bo *bo;
  BYTE *desc_ptr;
  UINT samplet_offset;
  bo = mbenc_ctx->gpe_context.dynamic_state.res.bo;
  dri_bo_map (bo, 1);
  MEDIA_DRV_ASSERT (bo->virtual);
  desc_ptr = (BYTE *) bo->virtual + mbenc_gpe_ctx->idrt_offset;
  desc = (struct gen8_interface_descriptor_data *) desc_ptr;
  for (i = 0; i < mbenc_gpe_ctx->num_kernels; i++)
    {
      MEDIA_KERNEL *kernel;
      kernel = &mbenc_gpe_ctx->kernels[i];
      MEDIA_DRV_ASSERT (sizeof (*desc) == 32);
      /*Setup the descritor table */
      memset (desc, 0, sizeof (*desc));
      desc->desc0.kernel_start_pointer = kernel->kernel_offset >> 6;
#if 0
      desc->desc3.sampler_count = 4;	/* FIXME: */
      desc->desc3.sampler_state_pointer =
	(mbenc_gpe_ctx->sampler_offset +
	 (i * mbenc_gpe_ctx->sampler_size)) >> 5;;
#endif
      desc->desc4.binding_table_entry_count = 0;	//1; /* FIXME: */
      desc->desc4.binding_table_pointer = (BINDING_TABLE_OFFSET (0) >> 5);
      desc->desc5.constant_urb_entry_read_offset = 0;
      desc->desc5.constant_urb_entry_read_length = (mbenc_gpe_ctx->curbe_size + 31) >> 5;	//CURBE_URB_ENTRY_LENGTH;
      desc->desc6.num_threads_in_tg = 1;
      desc++;
      desc++;
    }
  dri_bo_unmap (bo);
}

VOID
media_set_surface_state_adv_g8 (SURFACE_STATE_ADV_G8 * cmd,
				SURFACE_SET_PARAMS * params, INT format)
{
  cmd->ss1.cbcr_pixel_offset_v_direction = params->uv_direction;
  cmd->ss1.width = params->surface_2d->width - 1;
  cmd->ss1.height = params->surface_2d->height - 1;
  cmd->ss2.surface_format = format;
  cmd->ss2.interleave_chroma = 1;
  cmd->ss2.tile_mode =
    ((params->surface_2d->tiling !=
      I915_TILING_NONE) << 1) | ((params->surface_2d->tiling ==
				  I915_TILING_Y) ? 0x1 : 0x0);
  cmd->ss2.pitch = params->surface_2d->pitch - 1;
  cmd->ss3.y_offset_for_cb = params->surface_2d->y_cb_offset;	//(params->surface_2d->width * params->surface_2d->height);
  cmd->ss5.surface_memobj_ctrl_state = params->cacheability_control;
  cmd->ss6.surface_base_address = params->surface_2d->bo->offset;
}

VOID
media_set_surface_state_buffer_surface_g8 (SURFACE_STATE_G8 * cmd,
					   SURFACE_SET_PARAMS * params,
					   INT format, INT pitch)
{
  cmd->dw0.surface_format = format;
  cmd->dw0.surface_type = MEDIA_SURFACE_BUFFER;
  cmd->dw0.tile_mode = 0;
  cmd->dw1.obj_ctrl_state = params->cacheability_control;
  cmd->dw2.width = (params->size - 1) & 0x7F;
  cmd->dw2.height = ((params->size - 1) & 0x1FFF80) >> 7;
  cmd->dw3.depth = ((params->size - 1) & 0xFE00000) >> 21;
  cmd->dw3.surface_pitch = pitch;
  cmd->dw7.shader_chanel_select_a = HSW_SCS_ALPHA;
  cmd->dw7.shader_chanel_select_b = HSW_SCS_BLUE;
  cmd->dw7.shader_chanel_select_g = HSW_SCS_GREEN;
  cmd->dw7.shader_chanel_select_r = HSW_SCS_RED;
  cmd->dw8.base_addr = params->buf_object.bo->offset + params->offset;
}

VOID
media_set_surface_state_2d_surface_g8 (SURFACE_STATE_G8 * cmd,
				       SURFACE_SET_PARAMS * params,
				       INT format, UINT width, UINT height,
				       UINT offset, UINT cbcr_offset,
				       UINT y_offset)
{
  cmd->dw0.vert_line_stride_offset = params->vert_line_stride_offset;
  cmd->dw0.vert_line_stride = params->vert_line_stride;
  cmd->dw0.surface_format = format;
  cmd->dw0.surface_type = MEDIA_SURFACE_2D;
  cmd->dw0.tile_mode =
    ((params->surface_2d->tiling !=
      I915_TILING_NONE) << 1) | ((params->surface_2d->tiling ==
				  I915_TILING_Y) ? 0x1 : 0x0);
  cmd->dw1.obj_ctrl_state = params->cacheability_control;
  cmd->dw2.width = width - 1;
  cmd->dw2.height = height - 1;
  cmd->dw3.surface_pitch = params->surface_2d->pitch - 1;
  cmd->dw5.y_offset = y_offset;
  cmd->dw7.shader_chanel_select_a = HSW_SCS_ALPHA;
  cmd->dw7.shader_chanel_select_b = HSW_SCS_BLUE;
  cmd->dw7.shader_chanel_select_g = HSW_SCS_GREEN;
  cmd->dw7.shader_chanel_select_r = HSW_SCS_RED;
  cmd->dw8.base_addr = params->surface_2d->bo->offset + cbcr_offset;
}

VOID
media_add_surface_state_g8 (SURFACE_SET_PARAMS * params)
{
  UINT width, height, format, pitch, tile_alignment, y_offset = 0;
  if (params->surface_is_2d)
    {
      SURFACE_STATE_G8 *cmd =
	(SURFACE_STATE_G8 *) (params->binding_surface_state.buf +
			      params->surface_state_offset);
      *cmd = SURFACE_STATE_INIT_G8;
      width =
	(params->media_block_raw) ? ((params->surface_2d->width +
				      0x3) >> 2) : params->surface_2d->width;
      height = params->surface_2d->height;
      media_set_surface_state_2d_surface_g8 (cmd, params, params->format,
					     width, height, 0, 0, 0);

      dri_bo_emit_reloc (params->binding_surface_state.bo,
			 I915_GEM_DOMAIN_RENDER, 0, 0,
			 params->surface_state_offset +
			 offsetof (SURFACE_STATE_G8, dw8),
			 params->surface_2d->bo);

      *((UINT *) ((CHAR *) params->binding_surface_state.buf +
		  params->binding_table_offset)) =
	params->surface_state_offset /*<< BINDING_TABLE_SURFACE_SHIFT */ ;
    }
  else if (params->surface_is_uv_2d)
    {
      UINT cbcr_offset;
      SURFACE_STATE_G8 *cmd =
	(SURFACE_STATE_G8 *) (params->binding_surface_state.buf +
			      params->surface_state_offset);
      *cmd = SURFACE_STATE_INIT_G8;
      if (params->surface_2d->tiling == I915_TILING_Y)
	{
	  tile_alignment = 32;
	}
      else if (params->surface_2d->tiling == I915_TILING_X)
	{
	  tile_alignment = 8;
	}
      else
	tile_alignment = 1;
      width =
	(params->media_block_raw) ? ((params->surface_2d->width +
				      0x3) >> 2) : params->surface_2d->width;
      height = params->surface_2d->height / 2;
      y_offset = (params->surface_2d->y_cb_offset % tile_alignment) >> 2;
      cbcr_offset =
	ALIGN_FLOOR ( /*params->surface_2d->height */ params->surface_2d->
		     y_cb_offset, tile_alignment) *	/*params->surface_2d->width */
	params->surface_2d->pitch;
      media_set_surface_state_2d_surface_g8 (cmd, params,
					     STATE_SURFACEFORMAT_R16_UINT,
					     width, height, 0, cbcr_offset,
					     y_offset);

      dri_bo_emit_reloc (params->binding_surface_state.bo,
			 I915_GEM_DOMAIN_RENDER, 0,
			 cbcr_offset,
			 params->surface_state_offset +
			 offsetof (SURFACE_STATE_G8, dw8),
			 params->surface_2d->bo);

      *((UINT *) ((CHAR *) params->binding_surface_state.buf +
		  params->binding_table_offset)) =
	params->surface_state_offset /*<< BINDING_TABLE_SURFACE_SHIFT */ ;
    }
  else if (params->advance_state)
    {

      SURFACE_STATE_ADV_G8 *cmd =
	(SURFACE_STATE_ADV_G8 *) (params->binding_surface_state.buf +
				  params->surface_state_offset);
      *cmd = SURFACE_STATE_ADV_INIT_G8;
      media_set_surface_state_adv_g8 (cmd, params, MFX_SURFACE_PLANAR_420_8);

      dri_bo_emit_reloc (params->binding_surface_state.bo,
			 I915_GEM_DOMAIN_RENDER, 0,
			 params->offset,
			 params->surface_state_offset +
			 offsetof (SURFACE_STATE_ADV_G8, ss6),
			 params->surface_2d->bo);
      *((UINT *) ((CHAR *) params->binding_surface_state.buf +
		  params->binding_table_offset)) =
	params->surface_state_offset /*<< BINDING_TABLE_SURFACE_SHIFT */ ;
    }
  else
    {
      SURFACE_STATE_G8 *cmd =
	(SURFACE_STATE_G8 *) (params->binding_surface_state.buf +
			      params->surface_state_offset);
      *cmd = SURFACE_STATE_INIT_G8;
      MEDIA_DRV_ASSERT (params->buf_object.bo);

      if (params->surface_is_raw)
	{
	  format = STATE_SURFACEFORMAT_RAW;
	  pitch = 0;
	}
      else
	{
	  format = STATE_SURFACEFORMAT_R32_UINT;
	  pitch = sizeof (UINT) - 1;
	}

      media_set_surface_state_buffer_surface_g8 (cmd, params, format, pitch);
      dri_bo_emit_reloc (params->binding_surface_state.bo,
			 I915_GEM_DOMAIN_RENDER,
			 0 /* I915_GEM_DOMAIN_RENDER */ ,
			 params->offset,
			 params->surface_state_offset +
			 offsetof (SURFACE_STATE_G8, dw8),
			 params->buf_object.bo);
      *((UINT *) ((CHAR *) params->binding_surface_state.buf +
		  params->binding_table_offset)) =
	params->surface_state_offset /*<< BINDING_TABLE_SURFACE_SHIFT */ ;
    }
}
