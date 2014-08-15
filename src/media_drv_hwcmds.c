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

#include "media_drv_hw.h"
#include "media_drv_hwcmds.h"
#include "media_drv_defines.h"
//#define DEBUG
STATUS
mediadrv_gen_pipe_ctrl_cmd (MEDIA_BATCH_BUFFER * batch,
			    PIPE_CONTROL_PARAMS * params)
{
  STATUS status = SUCCESS;
  BEGIN_BATCH (batch, 5);
  OUT_BATCH (batch, CMD_PIPE_CONTROL | CMD_PIPE_CONTROL_DWORD_LEN);

  switch (params->flush_mode)
    {
    case FLUSH_WRITE_CACHE:
      OUT_BATCH (batch,
		 CMD_PIPE_CONTROL_DEST_ADDR_TYPE | CMD_PIPE_CONTROL_CS_STALL |
		 CMD_PIPE_CONTROL_RT_FLUSH_ENABLE |
		 CMD_PIPE_CONTROL_FLUSH_ENABLE | CMD_PIPE_CONTROL_DC_FLUSH |
		 CMD_PIPE_CONTROL_NOWRITE);
      OUT_BATCH (batch, 0);	/* write address */
      break;

    case FLUSH_READ_CACHE:
      OUT_BATCH (batch,
		 CMD_PIPE_CONTROL_DEST_ADDR_TYPE | CMD_PIPE_CONTROL_NOWRITE |
		 CMD_PIPE_CONTROL_INSTR_CI_ENABLE |
		 CMD_PIPE_CONTROL_FLUSH_ENABLE | CMD_PIPE_CONTROL_VF_CI_ENABLE
		 | CMD_PIPE_CONTROL_CONSTANT_CI_ENABLE |
		 CMD_PIPE_CONTROL_STATE_CI_ENABLE);
      OUT_BATCH (batch, 0);	/* write address */
      break;

    default:
#ifdef DEBUG
      printf ("params->status_buffer.bo=%x\n", params->status_buffer.bo);
#endif
      if (params->status_buffer.bo)
	{
	  OUT_BATCH (batch, CMD_PIPE_CONTROL_CS_STALL | CMD_PIPE_CONTROL_WRITE_QWORD |	/*CMD_PIPE_CONTROL_DEST_ADDR_TYPE | */
		     CMD_PIPE_CONTROL_FLUSH_ENABLE);
	  OUT_RELOC (batch, params->status_buffer.bo,
		     I915_GEM_DOMAIN_INSTRUCTION, 0, 0);

	}
      else
	{

	  OUT_BATCH (batch, CMD_PIPE_CONTROL_FLUSH_ENABLE);
	  OUT_BATCH (batch, 0);	/* write address */

	}
      break;
    }
  /*immediate_data needs to set before calling this function if Post Sync Operation is 1h */
  /*in case params->status_buffer!=NULL immediate_data is set to 1 */
  OUT_BATCH (batch, params->immediate_data);	/* write data */
  OUT_BATCH (batch, 0);
  ADVANCE_BATCH (batch);
  return status;
}

STATUS
mediadrv_gen_pipeline_select_cmd (MEDIA_BATCH_BUFFER * batch)
{
  STATUS status = SUCCESS;
  BEGIN_BATCH (batch, 1);
  OUT_BATCH (batch, CMD_PIPELINE_SELECT | PIPELINE_SELECT_MEDIA);
  ADVANCE_BATCH (batch);
  return status;
}

STATUS
mediadrv_gen_state_base_address_cmd (MEDIA_BATCH_BUFFER * batch,
				     STATE_BASE_ADDR_PARAMS * params)
{
  STATUS status = SUCCESS;
  BEGIN_BATCH (batch, CMD_STATE_BASE_ADDRESS_LEN);
  OUT_BATCH (batch, (CMD_STATE_BASE_ADDRESS | (CMD_STATE_BASE_ADDRESS_LEN-2)));
  OUT_BATCH (batch, 0 /*| BASE_ADDRESS_MODIFY*/);	//General State Base Address
  /*DW4 Surface state base address */
  if (params->surface_state.bo) {
   OUT_RELOC (batch, params->surface_state.bo, I915_GEM_DOMAIN_INSTRUCTION, 0, BASE_ADDRESS_MODIFY);	/* Surface state base address */
  }
  else
    OUT_BATCH (batch, 0 /*| BASE_ADDRESS_MODIFY */ );
  /*DW6. Dynamic state base address */
  if (params->dynamic_state.bo)
    OUT_RELOC (batch, params->dynamic_state.bo,
	       I915_GEM_DOMAIN_RENDER | I915_GEM_DOMAIN_SAMPLER,
	       0, BASE_ADDRESS_MODIFY);
  else
    OUT_BATCH (batch, 0 /*| BASE_ADDRESS_MODIFY*/);
  /*DW8. Indirect Object base address */
  if (params->indirect_object.bo)
    OUT_RELOC (batch, params->indirect_object.bo,
	       I915_GEM_DOMAIN_SAMPLER, 0, BASE_ADDRESS_MODIFY);
  else
    OUT_BATCH (batch, 0 /*| BASE_ADDRESS_MODIFY*/);
  /*DW10. Instruct base address */
  if (params->instruction_buffer.bo)
    OUT_RELOC (batch, params->instruction_buffer.bo,
	       I915_GEM_DOMAIN_INSTRUCTION, 0, BASE_ADDRESS_MODIFY);
  else
    OUT_BATCH (batch, 0 | BASE_ADDRESS_MODIFY);
  /* DW12. Size limitation */
  OUT_BATCH (batch, 0 /*| BASE_ADDRESS_MODIFY */ );	//General State Access Upper Bound      
  OUT_BATCH (batch,0xFFFFF000 | BASE_ADDRESS_MODIFY);	//Dynamic State Access Upper Bound
  OUT_BATCH (batch, 0 /*| BASE_ADDRESS_MODIFY */ );	//Indirect Object Access Upper Bound
  OUT_BATCH (batch,0xFFFFF000 | BASE_ADDRESS_MODIFY);	//Instruction Access Upper Bound
  OUT_BATCH (batch, 0 /*| BASE_ADDRESS_MODIFY*/);
  OUT_BATCH (batch, 0 /*| BASE_ADDRESS_MODIFY*/);
  ADVANCE_BATCH (batch);
  return status;
}

STATUS
mediadrv_gen_media_vfe_state_cmd (MEDIA_BATCH_BUFFER * batch,
				  VFE_STATE_PARAMS * params)
{
  STATUS status = SUCCESS;
  BEGIN_BATCH (batch, CMD_MEDIA_VFE_STATE_LEN);
  OUT_BATCH (batch, CMD_MEDIA_VFE_STATE | (CMD_MEDIA_VFE_STATE_LEN - 2));
  OUT_BATCH (batch, 0);		/* Scratch Space Base Pointer and Space */
  OUT_BATCH (batch, params->max_num_threads << 16 |	/* Maximum Number of Threads */
	     params->num_urb_entries << 8 |	/* Number of URB Entries */
	     params->gpgpu_mode << 2);	/* MEDIA Mode */
  OUT_BATCH (batch, 0);		/* Debug: Object ID */
  OUT_BATCH (batch, params->urb_entry_size << 16 |	/* URB Entry Allocation Size */
	     params->curbe_allocation_size);	/* CURBE Allocation Size */
  /* the vfe_desc5/6/7 will decide whether the scoreboard is used. */
  if (params->scoreboard_enable)
    {
      OUT_BATCH (batch, params->scoreboardDW5);
      OUT_BATCH (batch, params->scoreboardDW6);
      OUT_BATCH (batch, params->scoreboardDW7);
    }
  else
    {
      OUT_BATCH (batch, 0);
      OUT_BATCH (batch, 0);
      OUT_BATCH (batch, 0);
    }
  ADVANCE_BATCH (batch);
  return status;

}

STATUS
mediadrv_gen_media_curbe_load_cmd (MEDIA_BATCH_BUFFER * batch,
				   CURBE_LOAD_PARAMS * params)
{
  STATUS status = SUCCESS;
  BEGIN_BATCH (batch, 4);
  OUT_BATCH (batch, CMD_MEDIA_CURBE_LOAD | (4 - 2));
  OUT_BATCH (batch, 0);
  OUT_BATCH (batch, params->curbe_size);
  OUT_BATCH (batch, params->curbe_offset);
  ADVANCE_BATCH (batch);
  return status;
}

STATUS
mediadrv_gen_media_id_load_cmd (MEDIA_BATCH_BUFFER * batch,
				ID_LOAD_PARAMS * params)
{
  STATUS status = SUCCESS;
  BEGIN_BATCH (batch, 4);
  OUT_BATCH (batch, CMD_MEDIA_INTERFACE_LOAD | (4 - 2));
  OUT_BATCH (batch, 0);
  OUT_BATCH (batch, params->idrt_size);
  OUT_BATCH (batch, params->idrt_offset);
  ADVANCE_BATCH (batch);
  return status;
}

STATUS
mediadrv_media_mi_set_predicate_cmd (MEDIA_BATCH_BUFFER * batch,
				     MI_SET_PREDICATE_PARAMS * params)
{
  STATUS status = SUCCESS;
  BEGIN_BATCH (batch, 1);
  OUT_BATCH (batch, CMD_MI_SET_PREDICATE | params->predicate_en);
  ADVANCE_BATCH (batch);
  return status;
}

STATUS
media_object_walker_cmd (MEDIA_BATCH_BUFFER * batch,
			 MEDIA_OBJ_WALKER_PARAMS * params)
{
  STATUS status = SUCCESS;
  UINT dw5_cmd = 0, dw10_cmd = 0, dw11_cmd = 0, dw12_cmd = 0,use_scoreboard=0;
  UINT mode = params->walker_mode;
  UINT repel = (mode == SINGLE_MODE) ? 1 : 0;
  UINT dual_mode = (mode == DUAL_MODE) ? 1 : 0;
  UINT quad_mode = (mode == QUAD_MODE) ? 1 : 0;
  BEGIN_BATCH (batch, CMD_MEDIA_OBJECT_WALKER_LEN /*17 */ );
  if (params->mb_enc_iframe_dist_en || params->me_in_use)
    {
      use_scoreboard=0;
      dw5_cmd = 0;
      dw10_cmd = (params->frm_w_in_mb - 1);	//do we really need to set this for HSW
      dw11_cmd = dw11_cmd | 1 << 16;
      dw12_cmd = dw12_cmd | 0x1;
    }
  else
    {
      use_scoreboard=params->use_scoreboard;
      dw10_cmd = 0;
      if (params->hybrid_pak2_pattern_enabled_45_deg)
	{

	  dw11_cmd = 0x1;
	  dw12_cmd = dw12_cmd | 1 << 16 | 0x3FF;
	  dw5_cmd = 0x07;

	}
      else if ((params->pic_coding_type == I_FRM ||
		(params->pic_coding_type == B_FRM &&
		 !params->direct_spatial_mv_pred)) &&
	       !params->force_26_degree)
	{
	  dw5_cmd = 0x3;
	  dw11_cmd = 0x1;
	  dw12_cmd = dw12_cmd | 1 << 16 | 0x3FF;

	}
      else
	{
	  dw5_cmd = 0x0F;
	  dw11_cmd = 0x1;
	  dw12_cmd = dw12_cmd | 1 << 16 | 0x3FE;
	}

    }
  OUT_BATCH (batch,CMD_MEDIA_OBJECT_WALKER | (CMD_MEDIA_OBJECT_WALKER_LEN - 2));
  OUT_BATCH (batch, 0);
  OUT_BATCH (batch,use_scoreboard << 21);
  OUT_BATCH (batch, 0);
  OUT_BATCH (batch, 0);
  OUT_BATCH (batch, dw5_cmd);
  OUT_BATCH (batch, ((dual_mode << 31) | (repel << 30) | (quad_mode << 29)));
  OUT_BATCH (batch, ((0x3FF << 16) | 0x3FF));
  OUT_BATCH (batch, ((params->frmfield_h_in_mb << 16) | params->frm_w_in_mb));
  OUT_BATCH (batch, 0);
  OUT_BATCH (batch, dw10_cmd);
  OUT_BATCH (batch, dw11_cmd);
  OUT_BATCH (batch, dw12_cmd);
  OUT_BATCH (batch, ((params->frmfield_h_in_mb << 16) | params->frm_w_in_mb));
  OUT_BATCH (batch, 0);
  OUT_BATCH (batch, (0 | params->frm_w_in_mb));
  OUT_BATCH (batch, (0 | (params->frmfield_h_in_mb << 16)));
  ADVANCE_BATCH (batch);
  return status;
}

STATUS
media_object_cmd (MEDIA_BATCH_BUFFER *batch,
                  MEDIA_OBJECT_PARAMS *params)
{
  STATUS status = SUCCESS;

  BEGIN_BATCH (batch, CMD_MEDIA_OBJECT_LEN);
  OUT_BATCH (batch, CMD_MEDIA_OBJECT | (CMD_MEDIA_OBJECT_LEN - 2));
  OUT_BATCH (batch, params->interface_offset);
  OUT_BATCH (batch, 0);
  OUT_BATCH (batch, 0);
  OUT_BATCH (batch, 0);
  OUT_BATCH (batch, 0);
  ADVANCE_BATCH (batch);

  return status;
}

STATUS
mediadrv_mi_store_data_imm_cmd (MEDIA_BATCH_BUFFER * batch,
				MI_STORE_DATA_IMM_PARAMS * params)
{
  STATUS status = SUCCESS;
  BEGIN_BATCH (batch, 4);
  OUT_BATCH (batch, CMD_MI_STORE_DATA_IMM | 0x2);
  OUT_BATCH (batch, 0);
  OUT_RELOC (batch, params->status_buffer.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
	     0);

  OUT_BATCH (batch, params->value);
  ADVANCE_BATCH (batch);
  return status;
}

STATUS
mediadrv_set_curbe_scaling (MEDIA_GPE_CTX * gpe_context,
			    SCALING_CURBE_PARAMS * params)
{
  STATUS status = SUCCESS;
  CURBE_SCALING_DATA *cmd;
  dri_bo_map (gpe_context->dynamic_state.res.bo, 1);
  MEDIA_DRV_ASSERT (gpe_context->dynamic_state.res.bo->virtual);
  cmd = (CURBE_SCALING_DATA *) gpe_context->dynamic_state.res.bo->virtual +
    gpe_context->curbe_offset;
  cmd->input_pic_height = params->input_pic_height;
  cmd->input_pic_width = params->input_pic_width;
  cmd->src_planar_y = SCALE_SRC_Y;
  cmd->dest_planar_y = SCALE_DST_Y;

  dri_bo_unmap (gpe_context->dynamic_state.res.bo);

  return status;
}
