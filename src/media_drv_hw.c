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
 *     Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 *
 */

#include "media_drv_init.h"
#include "media_drv_driver.h"
#include "media_drv_hw_g75.h"
#include "media_drv_hw_g7.h"
#include "media_drv_hw_g8.h"
#include "media_drv_hw_g9.h"

VOID
media_hw_context_init(VADriverContextP ctx)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  MEDIA_HW_CONTEXT *hw_ctx = &drv_ctx->hw_context;

  if (IS_HASWELL (drv_ctx->drv_data.device_id)) {
    media_hw_context_init_g75(ctx, hw_ctx);
  } else if (IS_GEN7 (drv_ctx->drv_data.device_id)) {
    media_hw_context_init_g7(ctx, hw_ctx);
  } else if (IS_GEN8 (drv_ctx->drv_data.device_id)) {
    media_hw_context_init_g8(ctx, hw_ctx);
  } else if (IS_GEN9 (drv_ctx->drv_data.device_id)) {
    media_hw_context_init_g9(ctx, hw_ctx);
  } else {
    printf ("Platform not supported");
    MEDIA_DRV_ASSERT (0);
  }
}
