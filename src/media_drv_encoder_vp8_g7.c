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

#include"media_drv_encoder.h"
#include "media_drv_kernels_g7.h"
#include "media_drv_hw_g7.h"
#include "media_drv_encoder_vp8.h"

static VOID
media_gpe_context_constants_g7 (MEDIA_GPE_CTX *gpe_context)
{
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 32 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 123;
  gpe_context->vfe_state.curbe_allocation_size =
    (CURBE_ALLOCATION_SIZE - 12 - 1);
  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = 0x80;
}

VOID
media_me_context_init_g7 (VADriverContextP ctx,
			  MEDIA_ENCODER_CTX * encoder_context)
{
  ME_CONTEXT *me_context = &encoder_context->me_context;
  MEDIA_GPE_CTX *gpe_context = &me_context->gpe_context;

  media_gpe_context_constants_g7 (gpe_context);
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels_g7[5], 1);
  media_gpe_context_init (ctx, gpe_context);
  media_sampler_setup_me_g7 (encoder_context);
  media_interface_setup_me (encoder_context);
  media_alloc_resource_me (ctx, encoder_context);
}

VOID
media_scaling_context_init_g7 (VADriverContextP ctx,
			       MEDIA_ENCODER_CTX * encoder_context)
{
  SCALING_CONTEXT *scaling_context = &encoder_context->scaling_context;
  MEDIA_GPE_CTX *gpe_context = &scaling_context->gpe_context;

  media_gpe_context_constants_g7 (gpe_context);
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels_g7[6], 1);
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_scaling (encoder_context);
  media_alloc_resource_scaling (ctx, encoder_context);
}

VOID
media_mbenc_context_init_g7 (VADriverContextP ctx,
			     MEDIA_ENCODER_CTX * encoder_context)
{
  MBENC_CONTEXT *mbenc_context = &encoder_context->mbenc_context;
  MEDIA_GPE_CTX *gpe_context = &mbenc_context->gpe_context;
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 32 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 123;	//121;    //16;
  gpe_context->vfe_state.curbe_allocation_size =
    (CURBE_ALLOCATION_SIZE - 12 - 1) /*CURBE_ALLOCATION_SIZE -1 */ ;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels_g7[0],
			  5);
  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	//* MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = /*0x140 */ CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = /*0 */ 0x80;
  media_gpe_context_init (ctx, gpe_context);
  media_sampler_setup_mbenc_g7 (encoder_context);
  media_interface_setup_mbenc (encoder_context);
  media_alloc_resource_mbenc (ctx, encoder_context);
  return;
}

VOID
media_mbpak_context_init_vp8_g7 (VADriverContextP ctx,
				 MEDIA_ENCODER_CTX * encoder_context)
{
  MBPAK_CONTEXT *mbpak_context = &encoder_context->mbpak_context;
  MEDIA_GPE_CTX *gpe_context = &mbpak_context->gpe_context;
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = /* 0xc0; */ CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 32 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 123;	//16;
  gpe_context->vfe_state.curbe_allocation_size =
    CURBE_ALLOCATION_SIZE - 12 - 1;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels_g7[7],
			  2);

  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	// * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;	//0xco
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_mbpak (gpe_context);

  gpe_context = &mbpak_context->gpe_context2;
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = /* 0xc0; */ CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 32 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 123;	//16;
  gpe_context->vfe_state.curbe_allocation_size =
    CURBE_ALLOCATION_SIZE - 12 - 1;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels_g7[7],
			  2);

  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	// * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;	//0xco
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_mbpak (gpe_context);
  media_alloc_resource_mbpak (ctx, encoder_context);
  return;
}

VOID
media_brc_init_reset_context_init_g7(VADriverContextP ctx,
				     MEDIA_ENCODER_CTX * encoder_context)
{
  BRC_INIT_RESET_CONTEXT *brc_init_reset_context = &encoder_context->brc_init_reset_context;
  MEDIA_GPE_CTX *gpe_context = &brc_init_reset_context->gpe_context;

  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 32 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 123;	//121;    //16;
  gpe_context->vfe_state.curbe_allocation_size =
    (CURBE_ALLOCATION_SIZE - 12 - 1) /*CURBE_ALLOCATION_SIZE -1 */ ;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels_g7[9],
			  2);
  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	//* MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = /*0x140 */ CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = /*0 */ 0x80;
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_brc_init_reset (encoder_context);
  media_alloc_resource_brc_init_reset (ctx, encoder_context);

  return;
}

VOID
media_brc_update_context_init_g7(VADriverContextP ctx,
				 MEDIA_ENCODER_CTX * encoder_context)
{
  BRC_UPDATE_CONTEXT *distortion_ctx = &encoder_context->brc_update_context;
  MEDIA_GPE_CTX *gpe_context = &distortion_ctx->gpe_context;

  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 32 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 123;	//121;    //16;
  gpe_context->vfe_state.curbe_allocation_size =
    (CURBE_ALLOCATION_SIZE - 12 - 1) /*CURBE_ALLOCATION_SIZE -1 */ ;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels_g7[11],
			  1);
  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	//* MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = /*0x140 */ CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = /*0 */ 0x80;
  media_gpe_context_init (ctx, gpe_context);

  encoder_context->brc_distortion_buffer_supported = 1;
  encoder_context->brc_constant_buffer_supported = 1;

  media_interface_setup_brc_update (encoder_context);
#if 0
  media_alloc_resource_brc_update (ctx, encoder_context);
#endif
  return;
}
