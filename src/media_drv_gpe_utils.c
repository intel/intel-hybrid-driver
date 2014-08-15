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

#include "media_drv_init.h"
#include "media_drv_util.h"
#include "media_drv_gpe_utils.h"
#include "media_drv_batchbuffer.h"
VOID
media_gpe_load_kernels (VADriverContextP ctx,
			MEDIA_GPE_CTX * gpe_context,
			MEDIA_KERNEL * kernel_list, UINT num_kernels)
{
  MEDIA_DRV_CONTEXT *i965 = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  INT i;
  UINT kernel_offset, end_offset, kernel_size = 0;
  BYTE *kernel_ptr;
  MEDIA_KERNEL *kernel;
  INSTRUCTION_TYPE *instruction_state = &gpe_context->instruction_state;
  MEDIA_DRV_ASSERT (num_kernels <= MAX_GPE_KERNELS);
  media_drv_memcpy (gpe_context->kernels,
		    (MAX_GPE_KERNELS * sizeof (MEDIA_KERNEL)), kernel_list,
		    (sizeof (MEDIA_KERNEL) * num_kernels));
  gpe_context->num_kernels = num_kernels;
  for (i = 0; i < num_kernels; i++)
    {
      kernel = &gpe_context->kernels[i];
      kernel_size += ALIGN(kernel->size, 64);
    }
  media_allocate_resource (&instruction_state->buff_obj,
			   i965->drv_data.bufmgr,
			   (const BYTE *) "kernel shader", kernel_size,
			   0x4096);
  if (instruction_state->buff_obj.bo == NULL)
    {
      printf ("failure to allocate the buffer space for kernel shader\n");
      return;
    }

  MEDIA_DRV_ASSERT (instruction_state->buff_obj.bo);

  instruction_state->buff_obj.bo_size = kernel_size;
  instruction_state->end_offset = 0;
  end_offset = 0;
  dri_bo_map (instruction_state->buff_obj.bo, 1);
  memset (instruction_state->buff_obj.bo->virtual, 0,
	  instruction_state->buff_obj.bo->size);
  kernel_ptr = (BYTE *) (instruction_state->buff_obj.bo->virtual);
  for (i = 0; i < num_kernels; i++)
    {
      kernel_offset = end_offset;
      kernel = &gpe_context->kernels[i];
      kernel->kernel_offset = kernel_offset;	//>0?kernel_offset:1;
      if (kernel->size)
	{
	  media_drv_memcpy ((UINT *) (kernel_ptr + kernel_offset),
			    (kernel_size - end_offset), kernel->bin,
			    kernel->size);
	  end_offset = kernel_offset + ALIGN(kernel->size, 64);
	}
    }
  instruction_state->end_offset = end_offset;
  dri_bo_unmap (instruction_state->buff_obj.bo);
  return;
}

VOID
media_gpe_context_dinit (MEDIA_GPE_CTX * gpe_context)
{
  if (gpe_context->surface_state_binding_table.res.bo != NULL)
    {
      dri_bo_unreference (gpe_context->surface_state_binding_table.res.bo);
      gpe_context->surface_state_binding_table.res.bo = NULL;
    }
  if (gpe_context->dynamic_state.res.bo != NULL)
    {
      dri_bo_unreference (gpe_context->dynamic_state.res.bo);
      gpe_context->dynamic_state.res.bo = NULL;
    }

}

VOID
media_gpe_context_destroy (MEDIA_GPE_CTX * gpe_context)
{
  INT i;


  media_gpe_context_dinit (gpe_context);

  if (gpe_context->status_buffer.res.bo != NULL)
    {
      dri_bo_unreference (gpe_context->status_buffer.res.bo);
      gpe_context->status_buffer.res.bo = NULL;
    }
  for (i = 0; i < gpe_context->num_kernels; i++)
    {
      MEDIA_KERNEL *kernel = &gpe_context->kernels[i];
      dri_bo_unreference (kernel->bo);
      kernel->bo = NULL;
    }
  if (gpe_context->instruction_state.buff_obj.bo != NULL)
    {
      dri_bo_unreference (gpe_context->instruction_state.buff_obj.bo);
      gpe_context->instruction_state.buff_obj.bo = NULL;
    }

}

VOID
media_gpe_context_init (VADriverContextP ctx, MEDIA_GPE_CTX * gpe_context)
{
  INT stat_buff_sz = 0;
  UINT start_offset, end_offset, bo_size = 0;
  MEDIA_DRV_CONTEXT *i965 = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  DYNAMIC_STATE *dynamic_state = &gpe_context->dynamic_state;
  STATUS_BUFFER *status_buffer = &gpe_context->status_buffer;
  bo_size =
    (gpe_context->idrt_size * MAX_INTERFACE_DESC_GEN6) +
    gpe_context->curbe_size +
    (gpe_context->sampler_size * MAX_INTERFACE_DESC_GEN6) + 192;
  media_allocate_resource (&dynamic_state->res, i965->drv_data.bufmgr,
			   (const BYTE *) "dynamic state heap", bo_size,
			   4096);
  MEDIA_DRV_ASSERT (dynamic_state->res.bo);
  end_offset = 0;
  dynamic_state->end_offset = 0;

  /* Constant buffer offset */
  start_offset = ALIGN (end_offset, 64);
  gpe_context->curbe_offset = start_offset;
  end_offset = start_offset + gpe_context->curbe_size;

  /* Interface descriptor offset */
  start_offset = ALIGN (end_offset, 64);
  gpe_context->idrt_offset = start_offset;
  end_offset = start_offset + (gpe_context->idrt_size * MAX_INTERFACE_DESC_GEN6);	//gpe_context->idrt_size;

  /* Sampler state offset */
  start_offset = ALIGN (end_offset, 64);
  gpe_context->sampler_offset = start_offset;
  end_offset =
    start_offset + (gpe_context->sampler_size * MAX_INTERFACE_DESC_GEN6);

  /* update the end offset of dynamic_state */
  dynamic_state->end_offset = end_offset;
/*FIXME:Hardcoded the size need to change this*/
  stat_buff_sz = 0x8000;
  media_allocate_resource (&status_buffer->res, i965->drv_data.bufmgr,
			   (const BYTE *) "status heap", 0x8000, 4096);
  status_buffer->res.bo_size = stat_buff_sz;
  MEDIA_DRV_ASSERT (status_buffer->res.bo);

}
