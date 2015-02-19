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

#include <va/va.h>
#include <va/va_enc_vp8.h>
#include "media_drv_hw_g7.h"
#include "media_drv_hw_g8.h"
#include "media_drv_surface.h"
//#define DEBUG
struct hw_codec_info gen8_hw_codec_info = {
  .max_width = 4096,
  .max_height = 4096,
  .tiled_surface = 1,
  .vp8_enc_hybrid_support = 0,
  .vp9_dec_hybrid_support = 1,
  .ratecontrol= VA_RC_CQP,
  .render_init = media_drv_gen8_render_init,
 };

struct hw_codec_info chv_hw_codec_info = {
  .max_width = 4096,
  .max_height = 4096,
  .vp9_dec_hybrid_support = 1,
  .tiled_surface = 1,
  .render_init = media_drv_gen8_render_init,
 };


VOID
media_hw_context_init_g8(VADriverContextP ctx, MEDIA_HW_CONTEXT *hw_ctx)
{
}
