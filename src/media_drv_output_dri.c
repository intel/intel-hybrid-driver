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
 *    Xiang Haihao <haihao.xiang@intel.com>
 *    Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 *
 */

#include <va/va_backend.h>
#include <va/va_dricommon.h>
#include "media_drv_driver.h"
#include "media_drv_output_dri.h"
#include "media_drv_init.h"
#include "dso_utils.h"
#include "media_drv_util.h"
#include "media_drv_surface.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>


#ifdef __cplusplus
}
#endif



typedef struct dri_drawable *(*dri_get_drawable_func) (VADriverContextP ctx,
                                                       XID drawable);
typedef union dri_buffer *(*dri_get_rendering_buffer_func) (VADriverContextP
                                                            ctx,
                                                            struct dri_drawable * d);

typedef VOID (*dri_swap_buffer_func) (VADriverContextP ctx,
                                      struct dri_drawable * d);

struct dri_vtable
{
  dri_get_drawable_func get_drawable;
  dri_get_rendering_buffer_func get_rendering_buffer;
  dri_swap_buffer_func swap_buffer;
};

struct va_dri_output
{
  struct dso_handle *handle;
  struct dri_vtable vtable;
};

VOID
media_output_dri_terminate (VADriverContextP ctx)
{
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  MEDIA_DRV_ASSERT (ctx);
  struct va_dri_output *const dri_output = drv_ctx->dri_output;

  if (!dri_output)
    return;

  if (dri_output->handle)
    {
      dso_close (dri_output->handle);
      dri_output->handle = NULL;
    }

  media_drv_free_memory (dri_output);
  drv_ctx->dri_output = NULL;
}


BOOL
media_output_dri_init (VADriverContextP ctx)
{
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  MEDIA_DRV_ASSERT (ctx);

  struct dso_handle *dso_handle;
  struct dri_vtable *dri_vtable;

  static const struct dso_symbol symbols[] = {
#if VA_CHECK_VERSION(1,0,0)
    {"va_dri_get_drawable",
     offsetof (struct dri_vtable, get_drawable)},
    {"va_dri_get_rendering_buffer",
     offsetof (struct dri_vtable, get_rendering_buffer)},
    {"va_dri_swap_buffer",
     offsetof (struct dri_vtable, swap_buffer)},
    {NULL,}
#else
    {"dri_get_drawable",
     offsetof (struct dri_vtable, get_drawable)},
    {"dri_get_rendering_buffer",
     offsetof (struct dri_vtable, get_rendering_buffer)},
    {"dri_swap_buffer",
     offsetof (struct dri_vtable, swap_buffer)},
    {NULL,}
#endif
  };

  drv_ctx->dri_output =
    media_drv_alloc_memory (sizeof (struct va_dri_output));
  if (!drv_ctx->dri_output)
    goto error;

  drv_ctx->dri_output->handle = dso_open (LIBVA_X11_NAME);
  if (!drv_ctx->dri_output->handle)
    goto error;

  dso_handle = drv_ctx->dri_output->handle;
  dri_vtable = &drv_ctx->dri_output->vtable;
  if (!dso_get_symbols
      (dso_handle, dri_vtable, sizeof (*dri_vtable), symbols))
    goto error;
  return true;

error:
  media_output_dri_terminate (ctx);
  return false;
}

# define VA_CHECK_DRM_AUTH_TYPE(ctx, type) \
    (((struct drm_state *)(ctx)->drm_state)->auth_type == (type))

VAStatus
media_put_surface_dri(
  VADriverContextP    ctx,
  VASurfaceID         surface,
  void               *draw,
  const VARectangle  *src_rect,
  const VARectangle  *dst_rect,
  const VARectangle  *cliprects,
  unsigned int        num_cliprects,
  unsigned int        flags
)
{
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  struct dri_vtable * const dri_vtable = &drv_ctx->dri_output->vtable;
  struct media_render_state * const render_state = &drv_ctx->render_state;
  struct dri_drawable *dri_drawable;
  union dri_buffer *buffer;
  struct region *dest_region;
  struct object_surface *obj_surface;
  unsigned int pp_flag = 0;
  bool new_region = false;
  uint32_t name;
  int i;
  unsigned int color_flag = 0;

  /* Currently don't support DRI1 */
  if (!VA_CHECK_DRM_AUTH_TYPE(ctx, VA_DRM_AUTH_DRI2))
    return VA_STATUS_ERROR_UNKNOWN;

  obj_surface = SURFACE(surface);

  if (!obj_surface || !obj_surface->bo)
    return VA_STATUS_ERROR_INVALID_SURFACE;

  _i965LockMutex(&drv_ctx->render_mutex);

  dri_drawable = dri_vtable->get_drawable(ctx, (Drawable)draw);

  buffer = dri_vtable->get_rendering_buffer(ctx, dri_drawable);

  dest_region = render_state->draw_region;

  if (dest_region) {
    dri_bo_flink(dest_region->bo, &name);

    if (buffer->dri2.name != name) {
      new_region = True;
      dri_bo_unreference(dest_region->bo);
    }
  } else {
    dest_region = (struct region *)calloc(1, sizeof(*dest_region));
    render_state->draw_region = dest_region;
    new_region = True;
  }

  if (new_region) {
    dest_region->x = dri_drawable->x;
    dest_region->y = dri_drawable->y;
    dest_region->width = dri_drawable->width;
    dest_region->height = dri_drawable->height;
    dest_region->cpp = buffer->dri2.cpp;
    dest_region->pitch = buffer->dri2.pitch;

    dest_region->bo = intel_bo_gem_create_from_name(drv_ctx->drv_data.bufmgr, "rendering buffer", buffer->dri2.name);

    dri_bo_get_tiling(dest_region->bo, &(dest_region->tiling), &(dest_region->swizzle));
  }

  color_flag = flags & VA_SRC_COLOR_MASK;
  if (color_flag == 0)
    color_flag = VA_SRC_BT601;

  pp_flag = color_flag;

  media_render_put_surface(ctx, obj_surface, src_rect, dst_rect, pp_flag);

  for (i = 0; i < MEDIA_GEN_MAX_SUBPIC; i++) {
    if (obj_surface->obj_subpic[i] != NULL) {
      obj_surface->subpic_render_idx = i;
      media_render_put_subpicture(ctx, obj_surface, src_rect, dst_rect);
    }
  }


  dri_vtable->swap_buffer(ctx, dri_drawable);

  _i965UnlockMutex(&drv_ctx->render_mutex);

  return VA_STATUS_SUCCESS;
}
