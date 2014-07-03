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
 *    Xiang Haihao <haihao.xiang@intel.com>
 *    Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 *
 */

#include <va/va_drmcommon.h>
#include "media_drv_util.h"
#include "media_drv_driver.h"
#include"media_drv_render.h"

enum
{
  SF_KERNEL = 0,
  PS_KERNEL,
  PS_SUBPIC_KERNEL
};
/* programs for Ivybridge */
static const UINT sf_kernel_static_gen7[][4] = {
};

/* Programs for Haswell */
static const UINT ps_kernel_static_gen7_haswell[][4] = {
#include "shaders/render/exa_wm_src_affine.g7b"
#include "shaders/render/exa_wm_src_sample_planar.g7b.haswell"
#include "shaders/render/exa_wm_yuv_color_balance.g7b.haswell"
#include "shaders/render/exa_wm_yuv_rgb.g7b"
#include "shaders/render/exa_wm_write.g7b"
};

static const UINT ps_subpic_kernel_static_gen7[][4] = {
#include "shaders/render/exa_wm_src_affine.g7b"
#include "shaders/render/exa_wm_src_sample_argb.g7b"
#include "shaders/render/exa_wm_write.g7b"
};

static struct media_render_kernel render_kernels_gen7_haswell[] = {
  {
   "SF",
   SF_KERNEL,
   sf_kernel_static_gen7,
   sizeof (sf_kernel_static_gen7),
   NULL}
  ,
  {
   "PS",
   PS_KERNEL,
   ps_kernel_static_gen7_haswell,
   sizeof (ps_kernel_static_gen7_haswell),
   NULL}
  ,

  {
   "PS_SUBPIC",
   PS_SUBPIC_KERNEL,
   ps_subpic_kernel_static_gen7,
   sizeof (ps_subpic_kernel_static_gen7),
   NULL}
};

BOOL
media_render_init (VADriverContextP ctx)
{
  INT i;
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  MEDIA_DRV_ASSERT (ctx);
  struct media_render_state *render_state = &drv_ctx->render_state;


  /* kernel */
  /*MEDIA_DRV_ASSERT(NUM_RENDER_KERNEL == (sizeof(render_kernels_gen6) / 
     sizeof(render_kernels_gen6[0]))); */


  if (IS_GEN75 (drv_ctx->drv_data.device_id))
    memcpy (render_state->render_kernels, render_kernels_gen7_haswell,
	    sizeof (render_state->render_kernels));
  else
    {
      printf
	("This device is not supported by driver:loading render kernel failed\n");
      MEDIA_DRV_ASSERT (0);
    }

  for (i = 0; i < NUM_RENDER_KERNEL; i++)
    {
      struct media_render_kernel *kernel = &render_state->render_kernels[i];

      if (!kernel->size)
	continue;

      kernel->bo = dri_bo_alloc (drv_ctx->drv_data.bufmgr,
				 kernel->name, kernel->size, 0x1000);
      MEDIA_DRV_ASSERT (kernel->bo);
      dri_bo_subdata (kernel->bo, 0, kernel->size, kernel->bin);
    }

  /* constant buffer */
  render_state->curbe.bo = dri_bo_alloc (drv_ctx->drv_data.bufmgr,
					 "constant buffer", 4096, 64);
  MEDIA_DRV_ASSERT (render_state->curbe.bo);

  if (IS_HSW_GT1 (drv_ctx->drv_data.device_id))
    {
      render_state->max_wm_threads = 102;
    }
  else if (IS_HSW_GT2 (drv_ctx->drv_data.device_id))
    {
      render_state->max_wm_threads = 204;
    }
  else if (IS_HSW_GT3 (drv_ctx->drv_data.device_id))
    {
      render_state->max_wm_threads = 408;
    }
  else
    {
      /* should never get here !!! */
      MEDIA_DRV_ASSERT (0);
    }

  return true;
}

VOID
media_render_terminate (VADriverContextP ctx)
{
  INT i;
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  MEDIA_DRV_ASSERT (ctx);
  struct media_render_state *render_state = &drv_ctx->render_state;


  dri_bo_unreference (render_state->curbe.bo);
  render_state->curbe.bo = NULL;

  for (i = 0; i < NUM_RENDER_KERNEL; i++)
    {
      struct media_render_kernel *kernel = &render_state->render_kernels[i];

      dri_bo_unreference (kernel->bo);
      kernel->bo = NULL;
    }

  dri_bo_unreference (render_state->vb.vertex_buffer);
  render_state->vb.vertex_buffer = NULL;
  dri_bo_unreference (render_state->vs.state);
  render_state->vs.state = NULL;
  dri_bo_unreference (render_state->sf.state);
  render_state->sf.state = NULL;
  dri_bo_unreference (render_state->wm.sampler);
  render_state->wm.sampler = NULL;
  dri_bo_unreference (render_state->wm.state);
  render_state->wm.state = NULL;
  dri_bo_unreference (render_state->wm.surface_state_binding_table_bo);
  dri_bo_unreference (render_state->cc.viewport);
  render_state->cc.viewport = NULL;
  dri_bo_unreference (render_state->cc.state);
  render_state->cc.state = NULL;
  dri_bo_unreference (render_state->cc.blend);
  render_state->cc.blend = NULL;
  dri_bo_unreference (render_state->cc.depth_stencil);
  render_state->cc.depth_stencil = NULL;

  if (render_state->draw_region)
    {
      dri_bo_unreference (render_state->draw_region->bo);
      free (render_state->draw_region);
      render_state->draw_region = NULL;
    }
}
