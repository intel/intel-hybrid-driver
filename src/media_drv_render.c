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

#include <va/va_drmcommon.h>
#include "media_drv_util.h"
#include "media_drv_driver.h"
#include "media_drv_render.h"
#include "media_drv_hw.h"

BOOL
media_render_init (VADriverContextP ctx)
{
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  MEDIA_DRV_ASSERT (ctx);

  if (drv_ctx->codec_info && drv_ctx->codec_info->render_init)
    drv_ctx->codec_info->render_init(ctx);

  return true;
}

VOID
media_render_terminate (VADriverContextP ctx)
{
  MEDIA_DRV_CONTEXT *drv_ctx = ctx->pDriverData;
  MEDIA_DRV_ASSERT (ctx);
  struct media_render_state *render_state = &drv_ctx->render_state;


  if (render_state->render_terminate)
    render_state->render_terminate(ctx);
}
