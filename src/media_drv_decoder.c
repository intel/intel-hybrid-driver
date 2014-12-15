/*
 * Copyright © 2014 Intel Corporation
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
 *     Zhao Yakui <yakui.zhao@intel.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <va/va.h>
#include <va/va_backend.h>
#include "media_drv_defines.h"
#include "media_drv_init.h"
#include "media_drv_util.h"
#include "media_drv_gpe_utils.h"
#include "media_drv_surface.h"
#include "media_drv_driver.h"
#include "media_drv_decoder.h"

#include "media_drv_hw.h"
#include "media_drv_hybrid_vp9_common.h"

/*
 * Currently this is bogus. It is only for calling back the structure/function
 * related with decoding.
 * The real implementation will be added later.
 */

static VAStatus
intel_media_decode_picture(VADriverContextP ctx,
                           VAProfile profile,
                           union codec_state *codec_state,
                           struct hw_context *hw_context)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static void
intel_media_context_destroy(void *hw_context)
{
  struct hw_context *decoder_context = hw_context;

  if (decoder_context)
    free(decoder_context);
  return;
}

struct hw_context *
media_dec_hw_context_init (VADriverContextP ctx,
                           struct object_config *obj_config)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  struct hw_context *decoder_context = NULL;

  if (drv_ctx->codec_info->vp9_dec_hybrid_support &&
      (obj_config->profile == VAProfileVP9Version0)) {
    return media_hybrid_dec_hw_context_init(ctx, obj_config);
  }

  decoder_context = (struct hw_context *) media_drv_alloc_memory (sizeof(struct hw_context));

  decoder_context->run = intel_media_decode_picture;
  decoder_context->destroy = intel_media_context_destroy;

  return decoder_context;
}
