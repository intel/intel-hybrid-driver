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
 *    Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 *
 */

#include <stdio.h>
#include <va/va_drmcommon.h>
#include "media_drv_surface.h"
#include "media_drv_driver.h"
#include "media_drv_gpe_utils.h"
#include "media_drv_util.h"
#include "media_drv_hw.h"
#include "media_drv_hw_g8.h"
#include "media_drv_hw_g75.h"
#include "media_drv_hw_g7.h"
#include "object_heap.h"

BOOL
media_drv_intel_bufmgr_init (MEDIA_DRV_CONTEXT * drv_ctx)
{
  BOOL status = SUCCESS;
  drv_ctx->drv_data.bufmgr =
    intel_bufmgr_gem_init (drv_ctx->drv_data.fd, BATCH_BUF_SIZE);
  if (drv_ctx->drv_data.bufmgr == NULL)
    {
      //MEDIA_DRV_ASSERT (drv_ctx->bufmgr);
      return FAILED;
    }
  intel_bufmgr_gem_enable_reuse (drv_ctx->drv_data.bufmgr);
  return status;
}

VOID
media_drv_bufmgr_destroy (MEDIA_DRV_CONTEXT * drv_ctx)
{
  drm_intel_bufmgr_destroy (drv_ctx->drv_data.bufmgr);
}

static VOID
media_driver_get_revid (INT * value)
{
#define PCI_REVID       8
  FILE *fp;
  CHAR config_data[16];

  fp = fopen ("/sys/devices/pci0000:00/0000:00:02.0/config", "r");

  if (fp)
    {
      if (fread (config_data, 1, 16, fp))
	*value = config_data[PCI_REVID];
      else
	*value = 2;		/* assume it is at least  B-steping */
      fclose (fp);
    }
  else
    {
      *value = 2;		/* assume it is at least  B-steping */
    }

  return;
}

BOOL
media_drv_get_param (MEDIA_DRV_CONTEXT * drv_ctx, INT param, INT * value)
{
  struct drm_i915_getparam gp;

  gp.param = param;
  gp.value = value;
  return drmCommandWriteRead (drv_ctx->drv_data.fd, DRM_I915_GETPARAM, &gp,
			      sizeof (gp)) == 0;

}

BOOL
media_driver_init (VADriverContextP ctx)
{
  MEDIA_DRV_CONTEXT *drv_ctx = NULL;
  MEDIA_DRV_ASSERT (ctx);

  struct drm_state *const drm_state = (struct drm_state *) ctx->drm_state;
  INT has_exec2 = 0, has_bsd = 0, has_blt = 0, has_vebox = 0;
  drv_ctx = ctx->pDriverData;
  MEDIA_DRV_ASSERT (drm_state);
  MEDIA_DRV_ASSERT (VA_CHECK_DRM_AUTH_TYPE (ctx, VA_DRM_AUTH_DRI1) ||
		    VA_CHECK_DRM_AUTH_TYPE (ctx, VA_DRM_AUTH_DRI2) ||
		    VA_CHECK_DRM_AUTH_TYPE (ctx, VA_DRM_AUTH_CUSTOM));

  drv_ctx->drv_data.fd = drm_state->fd;
  drv_ctx->drv_data.dri2_enabled =
    (VA_CHECK_DRM_AUTH_TYPE (ctx, VA_DRM_AUTH_DRI2)
     || VA_CHECK_DRM_AUTH_TYPE (ctx, VA_DRM_AUTH_CUSTOM));

  if (!drv_ctx->drv_data.dri2_enabled)
    {
      return FALSE;
    }

  drv_ctx->locked = 0;
  media_drv_mutex_init (&drv_ctx->ctxmutex);

  media_drv_get_param (drv_ctx, I915_PARAM_CHIPSET_ID,
		       &drv_ctx->drv_data.device_id);
  if (media_drv_get_param (drv_ctx, I915_PARAM_HAS_EXECBUF2, &has_exec2))
    drv_ctx->drv_data.exec2_flag = has_exec2;
  if (media_drv_get_param (drv_ctx, I915_PARAM_HAS_BSD, &has_bsd))
    drv_ctx->drv_data.bsd_flag = has_bsd;
  if (media_drv_get_param (drv_ctx, I915_PARAM_HAS_BLT, &has_blt))
    drv_ctx->drv_data.blt_flag = has_blt;
  if (media_drv_get_param (drv_ctx, I915_PARAM_HAS_VEBOX, &has_vebox))
    drv_ctx->drv_data.vebox_flag = ! !has_vebox;

  media_driver_get_revid (&drv_ctx->drv_data.revision);
  media_drv_intel_bufmgr_init (drv_ctx);
  return TRUE;
}

VOID
media_driver_terminate (VADriverContextP ctx)
{
  MEDIA_DRV_CONTEXT *drv_ctx = NULL;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = ctx->pDriverData;
  media_drv_bufmgr_destroy (drv_ctx);
  media_drv_mutex_destroy (&drv_ctx->ctxmutex);
}


BOOL
media_driver_data_init (VADriverContextP ctx)
{
  MEDIA_DRV_CONTEXT *drv_ctx = NULL;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = ctx->pDriverData;
  if (IS_GEN75 (drv_ctx->drv_data.device_id))
    drv_ctx->codec_info = &gen75_hw_codec_info;
  else if (IS_GEN7 (drv_ctx->drv_data.device_id))
    drv_ctx->codec_info = &gen7_hw_codec_info;
  else if (IS_GEN8(drv_ctx->drv_data.device_id))
    drv_ctx->codec_info = &gen8_hw_codec_info;
  else
    return false;

  if (object_heap_init (&drv_ctx->config_heap,
			sizeof (struct object_config), CONFIG_ID_OFFSET))
    goto err_config_heap;
  if (object_heap_init (&drv_ctx->context_heap,
			sizeof (struct object_context), CONTEXT_ID_OFFSET))
    goto err_context_heap;

  if (object_heap_init (&drv_ctx->surface_heap,
			sizeof (struct object_surface), SURFACE_ID_OFFSET))
    goto err_surface_heap;
  if (object_heap_init (&drv_ctx->buffer_heap,
			sizeof (struct object_buffer), BUFFER_ID_OFFSET))
    goto err_buffer_heap;
  if (object_heap_init (&drv_ctx->image_heap,
			sizeof (struct object_image), IMAGE_ID_OFFSET))
    goto err_image_heap;

  drv_ctx->batch =
    media_batchbuffer_new (&drv_ctx->drv_data, I915_EXEC_RENDER, 0);
  drv_ctx->pp_batch =
    media_batchbuffer_new (&drv_ctx->drv_data, I915_EXEC_RENDER, 0);
  drv_ctx->render_batch =
    media_batchbuffer_new (&drv_ctx->drv_data, I915_EXEC_RENDER, 0);
  media_drv_mutex_init (&drv_ctx->render_mutex);
  media_drv_mutex_init (&drv_ctx->pp_mutex);

  return true;

err_image_heap:
  object_heap_destroy (&drv_ctx->buffer_heap);
err_buffer_heap:
  object_heap_destroy (&drv_ctx->surface_heap);
err_surface_heap:
  object_heap_destroy (&drv_ctx->context_heap);
err_context_heap:
  object_heap_destroy (&drv_ctx->config_heap);
err_config_heap:
  return false;
}

VOID
media_release_buffer_store (struct buffer_store ** ptr)
{
  struct buffer_store *buffer_store = *ptr;

  if (buffer_store == NULL)
    return;

  MEDIA_DRV_ASSERT (buffer_store->bo || buffer_store->buffer);
  MEDIA_DRV_ASSERT (!(buffer_store->bo && buffer_store->buffer));
  buffer_store->ref_count--;

  if (buffer_store->ref_count == 0)
    {
      dri_bo_unreference (buffer_store->bo);
      media_drv_free_memory (buffer_store->buffer);
      buffer_store->bo = NULL;
      buffer_store->buffer = NULL;
      media_drv_free_memory (buffer_store);
    }

  *ptr = NULL;
}

VOID
media_destroy_context (struct object_heap *heap, struct object_base *obj)
{
  struct object_context *obj_context = (struct object_context *) obj;
  INT i;

  if (obj_context->hw_context)
    {
      obj_context->hw_context->destroy (obj_context->hw_context);
      obj_context->hw_context = NULL;
    }

  if (obj_context->codec_type == CODEC_ENC)
    {
      MEDIA_DRV_ASSERT (obj_context->codec_state.encode.num_slice_params <=
			obj_context->codec_state.encode.max_slice_params);
      media_release_buffer_store (&obj_context->codec_state.encode.pic_param);
      media_release_buffer_store (&obj_context->codec_state.encode.seq_param);
      media_release_buffer_store (&obj_context->codec_state.encode.q_matrix);
      for (i = 0; i < obj_context->codec_state.encode.num_slice_params; i++)
	media_release_buffer_store (&obj_context->codec_state.encode.
				    slice_params[i]);

      media_drv_free_memory (obj_context->codec_state.encode.slice_params);

      MEDIA_DRV_ASSERT (obj_context->codec_state.encode.
			num_slice_params_ext <=
			obj_context->codec_state.encode.max_slice_params_ext);
      media_release_buffer_store (&obj_context->codec_state.encode.
				  pic_param_ext);
      media_release_buffer_store (&obj_context->codec_state.encode.
				  seq_param_ext);
      media_release_buffer_store (&obj_context->codec_state.
				  encode.frame_update_param);

      for (i = 0;
	   i <
	   ARRAY_ELEMS (obj_context->codec_state.encode.packed_header_param);
	   i++)
	media_release_buffer_store (&obj_context->codec_state.encode.
				    packed_header_param[i]);

      for (i = 0;
	   i <
	   ARRAY_ELEMS (obj_context->codec_state.encode.packed_header_data);
	   i++)
	media_release_buffer_store (&obj_context->codec_state.encode.
				    packed_header_data[i]);

      for (i = 0;
	   i < ARRAY_ELEMS (obj_context->codec_state.encode.misc_param); i++)
	media_release_buffer_store (&obj_context->codec_state.encode.
				    misc_param[i]);

      for (i = 0; i < obj_context->codec_state.encode.num_slice_params_ext;
	   i++)
	media_release_buffer_store (&obj_context->codec_state.encode.
				    slice_params_ext[i]);

      media_drv_free_memory (obj_context->codec_state.encode.
			     slice_params_ext);
    }
    else if (obj_context->codec_type == CODEC_DEC)
    {
      media_release_buffer_store(&obj_context->codec_state.decode.pic_param);
      media_release_buffer_store(&obj_context->codec_state.decode.iq_matrix);
      media_release_buffer_store(&obj_context->codec_state.decode.bit_plane);
      media_release_buffer_store(&obj_context->codec_state.decode.huffman_table);

      for (i = 0; i < obj_context->codec_state.decode.num_slice_params; i++) {
        media_release_buffer_store(&obj_context->codec_state.decode.slice_params[i]);
        media_release_buffer_store(&obj_context->codec_state.decode.slice_datas[i]);
      }

      media_drv_free_memory (obj_context->codec_state.decode.slice_params);
      media_drv_free_memory (obj_context->codec_state.decode.slice_datas);
    }

  media_drv_free_memory (obj_context->render_targets);
  object_heap_free (heap, obj);
}

VOID
media_destroy_config (struct object_heap *heap, struct object_base *obj)
{
  object_heap_free (heap, obj);
}

static VOID
media_destroy_heap (struct object_heap *heap,
		    VOID (*func) (struct object_heap * heap,
				  struct object_base * object))
{
  struct object_base *object;
  object_heap_iterator iter;

  object = object_heap_first (heap, &iter);

  while (object)
    {
      if (func)
	func (heap, object);

      object = object_heap_next (heap, &iter);
    }

  object_heap_destroy (heap);
}

VOID
media_destroy_buffer (struct object_heap *heap, struct object_base *obj)
{
  struct object_buffer *obj_buffer = (struct object_buffer *) obj;
  MEDIA_DRV_ASSERT (obj_buffer->buffer_store);
  media_release_buffer_store (&obj_buffer->buffer_store);
  object_heap_free (heap, obj);
}

VOID
media_driver_data_terminate (VADriverContextP ctx)
{
  MEDIA_DRV_CONTEXT *drv_ctx = NULL;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = ctx->pDriverData;

  media_drv_mutex_destroy (&drv_ctx->pp_mutex);
  media_drv_mutex_destroy (&drv_ctx->render_mutex);

  if (drv_ctx->batch)
    media_batchbuffer_free (drv_ctx->batch);

  if (drv_ctx->pp_batch)
    media_batchbuffer_free (drv_ctx->pp_batch);

  if (drv_ctx->render_batch)
    media_batchbuffer_free (drv_ctx->render_batch);

  media_destroy_heap (&drv_ctx->image_heap, media_destroy_image);
  media_destroy_heap (&drv_ctx->buffer_heap, media_destroy_buffer);
  media_destroy_heap (&drv_ctx->surface_heap, media_destroy_surface);
  media_destroy_heap (&drv_ctx->context_heap, media_destroy_context);
  media_destroy_heap (&drv_ctx->config_heap, media_destroy_config);
}
