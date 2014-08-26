#include"media_drv_encoder.h"
#include "media_drv_kernels_g7.h"
#include "media_drv_hw_g7.h"
#include "media_drv_encoder_vp8.h"
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
			  3);
  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	//* MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = /*0x140 */ CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = /*0 */ 0x80;
  media_gpe_context_init (ctx, gpe_context);
  media_sampler_setup_mbenc (encoder_context);
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
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels_g7[3],
			  2);

  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	// * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;	//0xco
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_mbpak (encoder_context);
  media_alloc_resource_mbpak (ctx, encoder_context);
  return;
}
