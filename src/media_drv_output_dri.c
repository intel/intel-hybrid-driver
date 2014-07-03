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


#include <va/va_dricommon.h>
#include "media_drv_output_dri.h"
#include "media_drv_init.h"
#include "dso_utils.h"
#include "media_drv_util.h"

typedef struct dri_drawable *(*dri_get_drawable_func) (VADriverContextP ctx,
						       XID drawable);
typedef union dri_buffer *(*dri_get_rendering_buffer_func) (VADriverContextP
							    ctx,
							    struct
							    dri_drawable * d);
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
    {"dri_get_drawable",
     offsetof (struct dri_vtable, get_drawable)},
    {"dri_get_rendering_buffer",
     offsetof (struct dri_vtable, get_rendering_buffer)},
    {"dri_swap_buffer",
     offsetof (struct dri_vtable, swap_buffer)},
    {NULL,}
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
