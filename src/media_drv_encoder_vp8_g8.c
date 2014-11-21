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
#include "media_drv_kernels_g8.h"
#include "media_drv_hw_g8.h"
#include "media_drv_hw_g7.h"
#include "media_drv_encoder_vp8.h"

VOID
gpe_context_vfe_scoreboardinit_pak_p2_g8 (MEDIA_ENCODER_CTX * encoder_context,MEDIA_GPE_CTX * gpe_context)
{
  gpe_context->vfe_state.vfe_desc5.scoreboard0.type = SCOREBOARD_NON_STALLING;
  gpe_context->vfe_state.vfe_desc5.scoreboard0.enable = 1;

  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x0 = 0xF;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y0 = 0;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x1 = 0;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y1 = 0xF;

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

  if (encoder_context->pic_coding_type == FRAME_TYPE_I){
    gpe_context->vfe_state.vfe_desc5.scoreboard0.mask = 0x07;
    gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x2 = 1;
    gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y2 = 0xE;
    }
  else {
    gpe_context->vfe_state.vfe_desc5.scoreboard0.mask = 0x1f;
    gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x2 = 0x0;
    gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y2 = 0xE;
    gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x3 = 0x0;
    gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y3 = 0xd;
    gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x4 = 0x1;
    gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y4 = 0xd;
   }
}

VOID media_object_walker_mbenc_init_g8(BOOL mbenc_i_frame_dist_in_use,BOOL mbenc_phase_2,MEDIA_ENCODER_CTX * encoder_context,MEDIA_OBJ_WALKER_PARAMS *media_obj_walker_params)
{
  media_drv_memset (media_obj_walker_params,
		    sizeof (MEDIA_OBJ_WALKER_PARAMS));
  media_obj_walker_params->pic_coding_type = encoder_context->pic_coding_type;
  if ((encoder_context->pic_coding_type == FRAME_TYPE_I)
      && (mbenc_phase_2 == FALSE))
    media_obj_walker_params->me_in_use = TRUE;
  else
    {
      media_obj_walker_params->pic_coding_type = FRAME_TYPE_I;
    }

  //media_obj_walker_params->use_scoreboard = encoder_context->use_hw_scoreboard;
  media_obj_walker_params->walker_mode = encoder_context->walker_mode;
  //media_obj_walker_params->direct_spatial_mv_pred;
  //media_obj_walker_params->me_in_use = TRUE;
  media_obj_walker_params->mb_enc_iframe_dist_en = mbenc_i_frame_dist_in_use;
  //media_obj_walker_params->force_26_degree;
  media_obj_walker_params->frmfield_h_in_mb =
    mbenc_i_frame_dist_in_use ?
    encoder_context->down_scaled_frame_field_height_mb4x :
    encoder_context->picture_height_in_mbs;
  media_obj_walker_params->frm_w_in_mb =
    mbenc_i_frame_dist_in_use ? encoder_context->down_scaled_width_mb4x
    : (UINT) encoder_context->picture_width_in_mbs;
 if ((encoder_context->pic_coding_type == FRAME_TYPE_I)&&(mbenc_phase_2 == FALSE)){
   //media_obj_walker_params.use_scoreboard =
  }
  else {
        media_obj_walker_params->use_scoreboard =1;
  }
}

VOID media_object_walker_pak_init_g8(UINT pak_phase_type,MEDIA_ENCODER_CTX * encoder_context,MEDIA_OBJ_WALKER_PARAMS *media_obj_walker_params)
{

  media_drv_memset (media_obj_walker_params,
		    sizeof (MEDIA_OBJ_WALKER_PARAMS));
  media_obj_walker_params->use_scoreboard = encoder_context->use_hw_scoreboard;
  media_obj_walker_params->walker_mode = encoder_context->walker_mode;
  media_obj_walker_params->pic_coding_type = encoder_context->pic_coding_type;
  //media_obj_walker_params->direct_spatial_mv_pred;
  //media_obj_walker_params->me_in_use = TRUE;
  //media_obj_walker_params->mb_enc_iframe_dist_en = mbenc_i_frame_dist_in_use;
  //media_obj_walker_params->force_26_degree;
  //media_obj_walker_params->frmfield_h_in_mb =encoder_context->picture_height_in_mbs;
   media_obj_walker_params->frm_w_in_mb =
    (UINT) encoder_context->picture_width_in_mbs;
 if (pak_phase_type == MBPAK_HYBRID_STATE_P1)
    {
      media_obj_walker_params->me_in_use = TRUE;
      media_obj_walker_params->frmfield_h_in_mb =
	encoder_context->picture_height_in_mbs;
    }
else if (pak_phase_type == MBPAK_HYBRID_STATE_P2)
    {
     if (encoder_context->pic_coding_type == FRAME_TYPE_I){
      media_obj_walker_params->frmfield_h_in_mb =
	encoder_context->picture_height_in_mbs * 2;
         media_obj_walker_params->scoreboard_mask=0x7;
      }
   else {
    media_obj_walker_params->frmfield_h_in_mb =
   encoder_context->picture_height_in_mbs * 3;
    media_obj_walker_params->scoreboard_mask=0x1f;
      }
   media_obj_walker_params->walker_degree=DEGREE_46;
    }

}

VOID
media_mbpak_context_init_vp8_g8(VADriverContextP ctx,
			      MEDIA_ENCODER_CTX * encoder_context)
{
  MBPAK_CONTEXT *mbpak_context = &encoder_context->mbpak_context;
  MEDIA_GPE_CTX *gpe_context = &mbpak_context->gpe_context;
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = /* 0xc0; */ CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 160;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 0x75;	//16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE*2 - 6;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels_g8[3], 2);

  gpe_context->idrt_size = ALIGN(sizeof (struct gen8_interface_descriptor_data),64);//sizeof (struct media_interface_descriptor_data);	// * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;	//0xco
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_mbpak_g8 (gpe_context);

  gpe_context = &mbpak_context->gpe_context2;
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = /* 0xc0; */ CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 160; //60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;  //64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 0x75; //16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE*2 - 6;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels_g8[3], 2);

  gpe_context->idrt_size = ALIGN(sizeof (struct gen8_interface_descriptor_data),64);//sizeof (struct media_interface_descriptor_data);  // * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;    //0xco
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_mbpak_g8 (gpe_context);

  media_alloc_resource_mbpak (ctx, encoder_context);

   return;
}

VOID
media_mbenc_context_init_vp8_g8 (VADriverContextP ctx,
			  MEDIA_ENCODER_CTX * encoder_context)
{
  MBENC_CONTEXT *mbenc_context = &encoder_context->mbenc_context;
  MEDIA_GPE_CTX *gpe_context = &mbenc_context->gpe_context;
  MEDIA_DRV_ASSERT (ctx);
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  //gpe_context->vfe_state.max_num_threads = 280 - 1;	//60 - 1;
  gpe_context->vfe_state.max_num_threads = 160;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 0x75;	//121;    //16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE*2 - 6;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels_g8[0], 3);
  gpe_context->idrt_size = /*sizeof (struct gen8_interface_descriptor_data);*/ALIGN(sizeof (struct gen8_interface_descriptor_data),64);	//* MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_mbenc_g8 (encoder_context);
  media_alloc_resource_mbenc (ctx, encoder_context);
  return;
}
