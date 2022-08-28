/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *    Keith Packard <keithp@keithp.com>
 *    Xiang Haihao <haihao.xiang@intel.com>
 *    Zhao Yakui <yakui.zhao@intel.com>
 *
 */

/*
 * Most of rendering codes are ported from intel-driver/src/i965_render.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <va/va.h>
#include <va/va_backend.h>
#include <va/va_drmcommon.h>

#include "media_drv_defines.h"

#include "media_drv_util.h"
#include "media_drv_driver.h"
#include "media_drv_render.h"
#include "media_drv_surface.h"
#include "media_drv_init.h"

#include "media_drv_batchbuffer.h"
#include "media_render_common.h"

#define SF_KERNEL_NUM_GRF       16
#define SF_MAX_THREADS          1


/* Programs for Haswell */
static const uint32_t ps_kernel_static_gen7_haswell[][4] = {
#include "shaders/render/exa_wm_src_affine.g7b"
#include "shaders/render/exa_wm_src_sample_planar.g7b.haswell"
#include "shaders/render/exa_wm_yuv_color_balance.g7b.haswell"
#include "shaders/render/exa_wm_yuv_rgb.g7b"
#include "shaders/render/exa_wm_write.g7b"
};

static const uint32_t ps_subpic_kernel_static_haswell[][4] = {
#include "shaders/render/exa_wm_src_affine.g7b"
#include "shaders/render/exa_wm_src_sample_argb.g7b"
#include "shaders/render/exa_wm_write.g7b"
};

#define RENDER_SURFACE_STATE_SIZE      sizeof(struct gen7_surface_state)

#define RENDER_SURFACE_STATE_OFFSET(index)     (RENDER_SURFACE_STATE_SIZE * index)
#define RENDER_BINDING_TABLE_OFFSET            RENDER_SURFACE_STATE_OFFSET(MAX_RENDER_SURFACES)

#define DEFAULT_BRIGHTNESS      0
#define DEFAULT_CONTRAST        10
#define DEFAULT_HUE             0
#define DEFAULT_SATURATION      10

#define URB_CS_ENTRY_SIZE     4


static struct media_render_kernel render_kernels_gen7_haswell[] = {
    {
        "PS",
        PS_KERNEL,
        ps_kernel_static_gen7_haswell,
        sizeof(ps_kernel_static_gen7_haswell),
        NULL
    },
    {
        "PS_SUBPIC",
        PS_SUBPIC_KERNEL,
        ps_subpic_kernel_static_haswell,
        sizeof(ps_subpic_kernel_static_haswell),
        NULL
    }

};

static float yuv_to_rgb_bt601[3][4] = {
{1.164,        0,        1.596,        -0.06275,},
{1.164,        -0.392,   -0.813,       -0.50196,},
{1.164,        2.017,    0,            -0.50196,},
};

static float yuv_to_rgb_bt709[3][4] = {
{1.164,        0,        1.793,        -0.06275,},
{1.164,        -0.213,   -0.533,       -0.50196,},
{1.164,        2.112,    0,            -0.50196,},
};

static float yuv_to_rgb_smpte_240[3][4] = {
{1.164,        0,        1.794,        -0.06275,},
{1.164,        -0.258,   -0.5425,      -0.50196,},
{1.164,        2.078,    0,            -0.50196,},
};


static void
i965_render_cc_viewport(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    struct i965_cc_viewport *cc_viewport;

    dri_bo_map(render_state->cc.viewport, 1);
    assert(render_state->cc.viewport->virtual);
    cc_viewport = render_state->cc.viewport->virtual;
    memset(cc_viewport, 0, sizeof(*cc_viewport));

    cc_viewport->min_depth = -1.e35;
    cc_viewport->max_depth = 1.e35;

    dri_bo_unmap(render_state->cc.viewport);
}

static void
gen7_render_set_surface_tiling(struct gen7_surface_state *ss, uint32_t tiling)
{
   switch (tiling) {
   case I915_TILING_NONE:
      ss->ss0.tiled_surface = 0;
      ss->ss0.tile_walk = 0;
      break;
   case I915_TILING_X:
      ss->ss0.tiled_surface = 1;
      ss->ss0.tile_walk = I965_TILEWALK_XMAJOR;
      break;
   case I915_TILING_Y:
      ss->ss0.tiled_surface = 1;
      ss->ss0.tile_walk = I965_TILEWALK_YMAJOR;
      break;
   }
}

/* Set "Shader Channel Select" */
static void
gen7_render_set_surface_scs(struct gen7_surface_state *ss)
{
    ss->ss7.shader_chanel_select_r = RENDER_HSW_SCS_RED;
    ss->ss7.shader_chanel_select_g = RENDER_HSW_SCS_GREEN;
    ss->ss7.shader_chanel_select_b = RENDER_HSW_SCS_BLUE;
    ss->ss7.shader_chanel_select_a = RENDER_HSW_SCS_ALPHA;
}

static void
gen7_render_set_surface_state(
    struct gen7_surface_state *ss,
    dri_bo                    *bo,
    unsigned long              offset,
    int                        width,
    int                        height,
    int                        pitch,
    int                        format,
    unsigned int               flags
)
{
    unsigned int tiling;
    unsigned int swizzle;

    memset(ss, 0, sizeof(*ss));

    ss->ss0.surface_type = I965_SURFACE_2D;
    ss->ss0.surface_format = format;

    ss->ss1.base_addr = bo->offset + offset;

    ss->ss2.width = width - 1;
    ss->ss2.height = height - 1;

    ss->ss3.pitch = pitch - 1;

    dri_bo_get_tiling(bo, &tiling, &swizzle);
    gen7_render_set_surface_tiling(ss, tiling);
}


static void
i965_render_src_surface_state(
    VADriverContextP ctx,
    int              index,
    dri_bo          *region,
    unsigned long    offset,
    int              w,
    int              h,
    int              pitch,
    int              format,
    unsigned int     flags
)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    void *ss;
    dri_bo *ss_bo = render_state->wm.surface_state_binding_table_bo;

    assert(index < MAX_RENDER_SURFACES);

    dri_bo_map(ss_bo, 1);
    assert(ss_bo->virtual);
    ss = (char *)ss_bo->virtual + RENDER_SURFACE_STATE_OFFSET(index);

        gen7_render_set_surface_state(ss,
                                      region, offset,
                                      w, h,
                                      pitch, format, flags);
        gen7_render_set_surface_scs(ss);
        dri_bo_emit_reloc(ss_bo,
                          I915_GEM_DOMAIN_SAMPLER, 0,
                          offset,
                          RENDER_SURFACE_STATE_OFFSET(index) + offsetof(struct gen7_surface_state, ss1),
                          region);

    ((unsigned int *)((char *)ss_bo->virtual + RENDER_BINDING_TABLE_OFFSET))[index] = RENDER_SURFACE_STATE_OFFSET(index);
    dri_bo_unmap(ss_bo);
    render_state->wm.sampler_count++;
}

static void
i965_render_src_surfaces_state(
    VADriverContextP ctx,
    struct object_surface *obj_surface,
    unsigned int     flags
)
{
    int region_pitch;
    int rw, rh;
    dri_bo *region;

    region_pitch = obj_surface->width;
    rw = obj_surface->orig_width;
    rh = obj_surface->orig_height;
    region = obj_surface->bo;

    i965_render_src_surface_state(ctx, 1, region, 0, rw, rh, region_pitch, I965_SURFACEFORMAT_R8_UNORM, flags);     /* Y */
    i965_render_src_surface_state(ctx, 2, region, 0, rw, rh, region_pitch, I965_SURFACEFORMAT_R8_UNORM, flags);

    if (obj_surface->fourcc == VA_FOURCC_Y800) /* single plane for grayscale */
        return;

    if (obj_surface->fourcc == VA_FOURCC_NV12) {
        i965_render_src_surface_state(ctx, 3, region,
                                      region_pitch * obj_surface->y_cb_offset,
                                      obj_surface->cb_cr_width, obj_surface->cb_cr_height, obj_surface->cb_cr_pitch,
                                      I965_SURFACEFORMAT_R8G8_UNORM, flags); /* UV */
        i965_render_src_surface_state(ctx, 4, region,
                                      region_pitch * obj_surface->y_cb_offset,
                                      obj_surface->cb_cr_width, obj_surface->cb_cr_height, obj_surface->cb_cr_pitch,
                                      I965_SURFACEFORMAT_R8G8_UNORM, flags);
    } else {
        i965_render_src_surface_state(ctx, 3, region,
                                      region_pitch * obj_surface->y_cb_offset,
                                      obj_surface->cb_cr_width, obj_surface->cb_cr_height, obj_surface->cb_cr_pitch,
                                      I965_SURFACEFORMAT_R8_UNORM, flags); /* U */
        i965_render_src_surface_state(ctx, 4, region,
                                      region_pitch * obj_surface->y_cb_offset,
                                      obj_surface->cb_cr_width, obj_surface->cb_cr_height, obj_surface->cb_cr_pitch,
                                      I965_SURFACEFORMAT_R8_UNORM, flags);
        i965_render_src_surface_state(ctx, 5, region,
                                      region_pitch * obj_surface->y_cr_offset,
                                      obj_surface->cb_cr_width, obj_surface->cb_cr_height, obj_surface->cb_cr_pitch,
                                      I965_SURFACEFORMAT_R8_UNORM, flags); /* V */
        i965_render_src_surface_state(ctx, 6, region,
                                      region_pitch * obj_surface->y_cr_offset,
                                      obj_surface->cb_cr_width, obj_surface->cb_cr_height, obj_surface->cb_cr_pitch,
                                      I965_SURFACEFORMAT_R8_UNORM, flags);
    }
}

static void
i965_render_dest_surface_state(VADriverContextP ctx, int index)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    void *ss;
    dri_bo *ss_bo = render_state->wm.surface_state_binding_table_bo;
    struct region *dest_region = render_state->draw_region;
    int format;
    assert(index < MAX_RENDER_SURFACES);

    if (dest_region->cpp == 2) {
        format = I965_SURFACEFORMAT_B5G6R5_UNORM;
    } else {
        format = I965_SURFACEFORMAT_B8G8R8A8_UNORM;
    }

    dri_bo_map(ss_bo, 1);
    assert(ss_bo->virtual);
    ss = (char *)ss_bo->virtual + RENDER_SURFACE_STATE_OFFSET(index);

        gen7_render_set_surface_state(ss,
                                      dest_region->bo, 0,
                                      dest_region->width, dest_region->height,
                                      dest_region->pitch, format, 0);
        gen7_render_set_surface_scs(ss);
        dri_bo_emit_reloc(ss_bo,
                          I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                          0,
                          RENDER_SURFACE_STATE_OFFSET(index) + offsetof(struct gen7_surface_state, ss1),
                          dest_region->bo);

    ((unsigned int *)((char *)ss_bo->virtual + RENDER_BINDING_TABLE_OFFSET))[index] = RENDER_SURFACE_STATE_OFFSET(index);
    dri_bo_unmap(ss_bo);
}

static void
i965_fill_vertex_buffer(
    VADriverContextP ctx,
    float tex_coords[4], /* [(u1,v1);(u2,v2)] */
    float vid_coords[4]  /* [(x1,y1);(x2,y2)] */
)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    float vb[12];

    enum { X1, Y1, X2, Y2 };

    static const unsigned int g_rotation_indices[][6] = {
        [VA_ROTATION_NONE] = { X2, Y2, X1, Y2, X1, Y1 },
        [VA_ROTATION_90]   = { X2, Y1, X2, Y2, X1, Y2 },
        [VA_ROTATION_180]  = { X1, Y1, X2, Y1, X2, Y2 },
        [VA_ROTATION_270]  = { X1, Y2, X1, Y1, X2, Y1 },
    };

    const unsigned int * const rotation_indices =
        g_rotation_indices[drv_ctx->rotation_attrib->value];

    vb[0]  = tex_coords[rotation_indices[0]]; /* bottom-right corner */
    vb[1]  = tex_coords[rotation_indices[1]];
    vb[2]  = vid_coords[X2];
    vb[3]  = vid_coords[Y2];

    vb[4]  = tex_coords[rotation_indices[2]]; /* bottom-left corner */
    vb[5]  = tex_coords[rotation_indices[3]];
    vb[6]  = vid_coords[X1];
    vb[7]  = vid_coords[Y2];

    vb[8]  = tex_coords[rotation_indices[4]]; /* top-left corner */
    vb[9]  = tex_coords[rotation_indices[5]];
    vb[10] = vid_coords[X1];
    vb[11] = vid_coords[Y1];

    dri_bo_subdata(drv_ctx->render_state.vb.vertex_buffer, 0, sizeof(vb), vb);
}

static void
i965_render_upload_vertex(
    VADriverContextP   ctx,
    struct object_surface *obj_surface,
    const VARectangle *src_rect,
    const VARectangle *dst_rect
)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    struct region *dest_region = render_state->draw_region;
    float tex_coords[4], vid_coords[4];
    int width, height;

    width  = obj_surface->orig_width;
    height = obj_surface->orig_height;

    tex_coords[0] = (float)src_rect->x / width;
    tex_coords[1] = (float)src_rect->y / height;
    tex_coords[2] = (float)(src_rect->x + src_rect->width) / width;
    tex_coords[3] = (float)(src_rect->y + src_rect->height) / height;

    vid_coords[0] = dest_region->x + dst_rect->x;
    vid_coords[1] = dest_region->y + dst_rect->y;
    vid_coords[2] = vid_coords[0] + dst_rect->width;
    vid_coords[3] = vid_coords[1] + dst_rect->height;

    i965_fill_vertex_buffer(ctx, tex_coords, vid_coords);
}

#define PI  3.1415926

static void
i965_render_upload_constants(VADriverContextP ctx,
                             struct object_surface *obj_surface,
                             unsigned int flags)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    unsigned short *constant_buffer;
    float *color_balance_base;
    float contrast = (float)drv_ctx->contrast_attrib->value / DEFAULT_CONTRAST;
    float brightness = (float)drv_ctx->brightness_attrib->value / 255; /* YUV is float in the shader */
    float hue = (float)drv_ctx->hue_attrib->value / 180 * PI;
    float saturation = (float)drv_ctx->saturation_attrib->value / DEFAULT_SATURATION;
    float *yuv_to_rgb;
    unsigned int color_flag;

    dri_bo_map(render_state->curbe.bo, 1);
    assert(render_state->curbe.bo->virtual);
    constant_buffer = render_state->curbe.bo->virtual;

    if (obj_surface->subsampling == SUBSAMPLE_YUV400) {
        assert(obj_surface->fourcc == VA_FOURCC_Y800);

        constant_buffer[0] = 2;
    } else {
        if (obj_surface->fourcc == VA_FOURCC_NV12)
            constant_buffer[0] = 1;
        else
            constant_buffer[0] = 0;
    }

    if (drv_ctx->contrast_attrib->value == DEFAULT_CONTRAST &&
        drv_ctx->brightness_attrib->value == DEFAULT_BRIGHTNESS &&
        drv_ctx->hue_attrib->value == DEFAULT_HUE &&
        drv_ctx->saturation_attrib->value == DEFAULT_SATURATION)
        constant_buffer[1] = 1; /* skip color balance transformation */
    else
        constant_buffer[1] = 0;

    color_balance_base = (float *)constant_buffer + 4;
    *color_balance_base++ = contrast;
    *color_balance_base++ = brightness;
    *color_balance_base++ = cos(hue) * contrast * saturation;
    *color_balance_base++ = sin(hue) * contrast * saturation;

    color_flag = flags & VA_SRC_COLOR_MASK;
    yuv_to_rgb = (float *)constant_buffer + 8;
    if (color_flag == VA_SRC_BT709)
        memcpy(yuv_to_rgb, yuv_to_rgb_bt709, sizeof(yuv_to_rgb_bt709));
    else if (color_flag == VA_SRC_SMPTE_240)
        memcpy(yuv_to_rgb, yuv_to_rgb_smpte_240, sizeof(yuv_to_rgb_smpte_240));
    else
        memcpy(yuv_to_rgb, yuv_to_rgb_bt601, sizeof(yuv_to_rgb_bt601));

    dri_bo_unmap(render_state->curbe.bo);
}


static void
i965_render_drawing_rectangle(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;
    struct region *dest_region = render_state->draw_region;

    BEGIN_BATCH(batch, 4);
    OUT_BATCH(batch, RCMD_DRAWING_RECTANGLE | 2);
    OUT_BATCH(batch, 0x00000000);
    OUT_BATCH(batch, (dest_region->width - 1) | (dest_region->height - 1) << 16);
    OUT_BATCH(batch, 0x00000000);

    ADVANCE_BATCH(batch);
}

static void
i965_clear_dest_region(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;
    struct region *dest_region = render_state->draw_region;
    unsigned int blt_cmd, br13;
    int pitch;

    blt_cmd = XY_COLOR_BLT_CMD;
    br13 = 0xf0 << 16;
    pitch = dest_region->pitch;

    if (dest_region->cpp == 4) {
        br13 |= BR13_8888;
        blt_cmd |= (XY_COLOR_BLT_WRITE_RGB | XY_COLOR_BLT_WRITE_ALPHA);
    } else {
        assert(dest_region->cpp == 2);
        br13 |= BR13_565;
    }

    if (dest_region->tiling != I915_TILING_NONE) {
        blt_cmd |= XY_COLOR_BLT_DST_TILED;
        pitch /= 4;
    }

    br13 |= pitch;

    media_batchbuffer_start_atomic_blt(batch, 0x1000);
    __BEGIN_BATCH(batch, 6, I915_EXEC_BLT);

    OUT_BATCH(batch, blt_cmd);
    OUT_BATCH(batch, br13);
    OUT_BATCH(batch, (dest_region->y << 16) | (dest_region->x));
    OUT_BATCH(batch, ((dest_region->y + dest_region->height) << 16) |
              (dest_region->x + dest_region->width));
    OUT_RELOC(batch, dest_region->bo,
              I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
              0);
    OUT_BATCH(batch, 0x0);
    ADVANCE_BATCH(batch);
    media_batchbuffer_end_atomic(batch);
}
/*
 * for GEN7
 */
static void
gen7_render_initialize(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    dri_bo *bo;

    /* VERTEX BUFFER */
    dri_bo_unreference(render_state->vb.vertex_buffer);
    bo = dri_bo_alloc(drv_ctx->drv_data.bufmgr,
                      "vertex buffer",
                      4096,
                      4096);
    assert(bo);
    render_state->vb.vertex_buffer = bo;

    /* WM */
    dri_bo_unreference(render_state->wm.surface_state_binding_table_bo);
    bo = dri_bo_alloc(drv_ctx->drv_data.bufmgr,
                      "surface state & binding table",
                      (SURFACE_STATE_PADDED_SIZE + sizeof(unsigned int)) * MAX_RENDER_SURFACES,
                      4096);
    assert(bo);
    render_state->wm.surface_state_binding_table_bo = bo;

    dri_bo_unreference(render_state->wm.sampler);
    bo = dri_bo_alloc(drv_ctx->drv_data.bufmgr,
                      "sampler state",
                      MAX_SAMPLERS * sizeof(struct gen7_sampler_state),
                      4096);
    assert(bo);
    render_state->wm.sampler = bo;
    render_state->wm.sampler_count = 0;

    /* COLOR CALCULATOR */
    dri_bo_unreference(render_state->cc.state);
    bo = dri_bo_alloc(drv_ctx->drv_data.bufmgr,
                      "color calc state",
                      sizeof(struct gen6_color_calc_state),
                      4096);
    assert(bo);
    render_state->cc.state = bo;

    /* CC VIEWPORT */
    dri_bo_unreference(render_state->cc.viewport);
    bo = dri_bo_alloc(drv_ctx->drv_data.bufmgr,
                      "cc viewport",
                      sizeof(struct i965_cc_viewport),
                      4096);
    assert(bo);
    render_state->cc.viewport = bo;

    /* BLEND STATE */
    dri_bo_unreference(render_state->cc.blend);
    bo = dri_bo_alloc(drv_ctx->drv_data.bufmgr,
                      "blend state",
                      sizeof(struct gen6_blend_state),
                      4096);
    assert(bo);
    render_state->cc.blend = bo;

    /* DEPTH & STENCIL STATE */
    dri_bo_unreference(render_state->cc.depth_stencil);
    bo = dri_bo_alloc(drv_ctx->drv_data.bufmgr,
                      "depth & stencil state",
                      sizeof(struct gen6_depth_stencil_state),
                      4096);
    assert(bo);
    render_state->cc.depth_stencil = bo;
}

static void
gen7_render_color_calc_state(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    struct gen6_color_calc_state *color_calc_state;

    dri_bo_map(render_state->cc.state, 1);
    assert(render_state->cc.state->virtual);
    color_calc_state = render_state->cc.state->virtual;
    memset(color_calc_state, 0, sizeof(*color_calc_state));
    color_calc_state->constant_r = 1.0;
    color_calc_state->constant_g = 0.0;
    color_calc_state->constant_b = 1.0;
    color_calc_state->constant_a = 1.0;
    dri_bo_unmap(render_state->cc.state);
}

static void
gen7_render_blend_state(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    struct gen6_blend_state *blend_state;

    dri_bo_map(render_state->cc.blend, 1);
    assert(render_state->cc.blend->virtual);
    blend_state = render_state->cc.blend->virtual;
    memset(blend_state, 0, sizeof(*blend_state));
    blend_state->blend1.logic_op_enable = 1;
    blend_state->blend1.logic_op_func = 0xc;
    blend_state->blend1.pre_blend_clamp_enable = 1;
    dri_bo_unmap(render_state->cc.blend);
}

static void
gen7_render_depth_stencil_state(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    struct gen6_depth_stencil_state *depth_stencil_state;

    dri_bo_map(render_state->cc.depth_stencil, 1);
    assert(render_state->cc.depth_stencil->virtual);
    depth_stencil_state = render_state->cc.depth_stencil->virtual;
    memset(depth_stencil_state, 0, sizeof(*depth_stencil_state));
    dri_bo_unmap(render_state->cc.depth_stencil);
}

static void
gen7_render_sampler(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    struct gen7_sampler_state *sampler_state;
    int i;

    assert(render_state->wm.sampler_count > 0);
    assert(render_state->wm.sampler_count <= MAX_SAMPLERS);

    dri_bo_map(render_state->wm.sampler, 1);
    assert(render_state->wm.sampler->virtual);
    sampler_state = render_state->wm.sampler->virtual;
    for (i = 0; i < render_state->wm.sampler_count; i++) {
        memset(sampler_state, 0, sizeof(*sampler_state));
        sampler_state->ss0.min_filter = I965_MAPFILTER_LINEAR;
        sampler_state->ss0.mag_filter = I965_MAPFILTER_LINEAR;
        sampler_state->ss3.r_wrap_mode = I965_TEXCOORDMODE_CLAMP;
        sampler_state->ss3.s_wrap_mode = I965_TEXCOORDMODE_CLAMP;
        sampler_state->ss3.t_wrap_mode = I965_TEXCOORDMODE_CLAMP;
        sampler_state++;
    }

    dri_bo_unmap(render_state->wm.sampler);
}


static void
gen7_render_setup_states(
    VADriverContextP   ctx,
    struct object_surface *obj_surface,
    const VARectangle *src_rect,
    const VARectangle *dst_rect,
    unsigned int       flags
)
{
    i965_render_dest_surface_state(ctx, 0);
    i965_render_src_surfaces_state(ctx, obj_surface, flags);
    gen7_render_sampler(ctx);
    i965_render_cc_viewport(ctx);
    gen7_render_color_calc_state(ctx);
    gen7_render_blend_state(ctx);
    gen7_render_depth_stencil_state(ctx);
    i965_render_upload_constants(ctx, obj_surface, flags);
    i965_render_upload_vertex(ctx, obj_surface, src_rect, dst_rect);
}


static void
gen7_emit_invarient_states(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;

    BEGIN_BATCH(batch, 1);
    OUT_BATCH(batch, RCMD_PIPELINE_SELECT | PIPELINE_SELECT_3D);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 4);
    OUT_BATCH(batch, GEN6_3DSTATE_MULTISAMPLE | (4 - 2));
    OUT_BATCH(batch, GEN6_3DSTATE_MULTISAMPLE_PIXEL_LOCATION_CENTER |
              GEN6_3DSTATE_MULTISAMPLE_NUMSAMPLES_1); /* 1 sample/pixel */
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, GEN6_3DSTATE_SAMPLE_MASK | (2 - 2));
    OUT_BATCH(batch, 1);
    ADVANCE_BATCH(batch);

    /* Set system instruction pointer */
    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, RCMD_STATE_SIP | 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);
}

static void
gen7_emit_state_base_address(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;
    struct media_render_state *render_state = &drv_ctx->render_state;

    OUT_BATCH(batch, RCMD_STATE_BASE_ADDRESS | (10 - 2));
    OUT_BATCH(batch, BASE_ADDRESS_MODIFY); /* General state base address */
    OUT_RELOC(batch, render_state->wm.surface_state_binding_table_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, BASE_ADDRESS_MODIFY); /* Surface state base address */
    OUT_BATCH(batch, BASE_ADDRESS_MODIFY); /* Dynamic state base address */
    OUT_BATCH(batch, BASE_ADDRESS_MODIFY); /* Indirect object base address */
    OUT_BATCH(batch, BASE_ADDRESS_MODIFY); /* Instruction base address */
    OUT_BATCH(batch, BASE_ADDRESS_MODIFY); /* General state upper bound */
    OUT_BATCH(batch, BASE_ADDRESS_MODIFY); /* Dynamic state upper bound */
    OUT_BATCH(batch, BASE_ADDRESS_MODIFY); /* Indirect object upper bound */
    OUT_BATCH(batch, BASE_ADDRESS_MODIFY); /* Instruction access upper bound */
}

static void
gen7_emit_viewport_state_pointers(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;
    struct media_render_state *render_state = &drv_ctx->render_state;

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_CC | (2 - 2));
    OUT_RELOC(batch,
              render_state->cc.viewport,
              I915_GEM_DOMAIN_INSTRUCTION, 0,
              0);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CL | (2 - 2));
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);
}

static void
gen7_emit_urb(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;
    unsigned int num_urb_entries = 32;

    if (IS_HASWELL(drv_ctx->drv_data.device_id))
        num_urb_entries = 64;

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_PS | (2 - 2));
    OUT_BATCH(batch, 8); /* in 1KBs */
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, GEN7_3DSTATE_URB_VS | (2 - 2));
    OUT_BATCH(batch,
              (num_urb_entries << GEN7_URB_ENTRY_NUMBER_SHIFT) |
              (2 - 1) << GEN7_URB_ENTRY_SIZE_SHIFT |
              (1 << GEN7_URB_STARTING_ADDRESS_SHIFT));
   ADVANCE_BATCH(batch);

   BEGIN_BATCH(batch, 2);
   OUT_BATCH(batch, GEN7_3DSTATE_URB_GS | (2 - 2));
   OUT_BATCH(batch,
             (0 << GEN7_URB_ENTRY_SIZE_SHIFT) |
             (1 << GEN7_URB_STARTING_ADDRESS_SHIFT));
   ADVANCE_BATCH(batch);

   BEGIN_BATCH(batch, 2);
   OUT_BATCH(batch, GEN7_3DSTATE_URB_HS | (2 - 2));
   OUT_BATCH(batch,
             (0 << GEN7_URB_ENTRY_SIZE_SHIFT) |
             (2 << GEN7_URB_STARTING_ADDRESS_SHIFT));
   ADVANCE_BATCH(batch);

   BEGIN_BATCH(batch, 2);
   OUT_BATCH(batch, GEN7_3DSTATE_URB_DS | (2 - 2));
   OUT_BATCH(batch,
             (0 << GEN7_URB_ENTRY_SIZE_SHIFT) |
             (2 << GEN7_URB_STARTING_ADDRESS_SHIFT));
   ADVANCE_BATCH(batch);
}

static void
gen7_emit_cc_state_pointers(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;
    struct media_render_state *render_state = &drv_ctx->render_state;

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, GEN6_3DSTATE_CC_STATE_POINTERS | (2 - 2));
    OUT_RELOC(batch,
              render_state->cc.state,
              I915_GEM_DOMAIN_INSTRUCTION, 0,
              1);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, GEN7_3DSTATE_BLEND_STATE_POINTERS | (2 - 2));
    OUT_RELOC(batch,
              render_state->cc.blend,
              I915_GEM_DOMAIN_INSTRUCTION, 0,
              1);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, GEN7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS | (2 - 2));
    OUT_RELOC(batch,
              render_state->cc.depth_stencil,
              I915_GEM_DOMAIN_INSTRUCTION, 0,
              1);
    ADVANCE_BATCH(batch);
}

static void
gen7_emit_sampler_state_pointers(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;
    struct media_render_state *render_state = &drv_ctx->render_state;

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, GEN7_3DSTATE_SAMPLER_STATE_POINTERS_PS | (2 - 2));
    OUT_RELOC(batch,
              render_state->wm.sampler,
              I915_GEM_DOMAIN_INSTRUCTION, 0,
              0);
    ADVANCE_BATCH(batch);
}

static void
gen7_emit_binding_table(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, GEN7_3DSTATE_BINDING_TABLE_POINTERS_PS | (2 - 2));
    OUT_BATCH(batch, RENDER_BINDING_TABLE_OFFSET);
    ADVANCE_BATCH(batch);
}

static void
gen7_emit_depth_buffer_state(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;

    BEGIN_BATCH(batch, 7);
    OUT_BATCH(batch, GEN7_3DSTATE_DEPTH_BUFFER | (7 - 2));
    OUT_BATCH(batch,
              (I965_DEPTHFORMAT_D32_FLOAT << 18) |
              (I965_SURFACE_NULL << 29));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 3);
    OUT_BATCH(batch, GEN7_3DSTATE_CLEAR_PARAMS | (3 - 2));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);
}

static void
gen7_emit_drawing_rectangle(VADriverContextP ctx)
{
    i965_render_drawing_rectangle(ctx);
}

static void
gen7_emit_vs_state(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;

    /* disable VS constant buffer */
    OUT_BATCH(batch, GEN6_3DSTATE_CONSTANT_VS | (7 - 2));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);

    OUT_BATCH(batch, GEN6_3DSTATE_VS | (6 - 2));
    OUT_BATCH(batch, 0); /* without VS kernel */
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0); /* pass-through */
}

static void
gen7_emit_bypass_state(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;

    /* bypass GS */
    BEGIN_BATCH(batch, 7);
    OUT_BATCH(batch, GEN6_3DSTATE_CONSTANT_GS | (7 - 2));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 7);
    OUT_BATCH(batch, GEN6_3DSTATE_GS | (7 - 2));
    OUT_BATCH(batch, 0); /* without GS kernel */
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0); /* pass-through */
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, GEN7_3DSTATE_BINDING_TABLE_POINTERS_GS | (2 - 2));
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    /* disable HS */
    BEGIN_BATCH(batch, 7);
    OUT_BATCH(batch, GEN7_3DSTATE_CONSTANT_HS | (7 - 2));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 7);
    OUT_BATCH(batch, GEN7_3DSTATE_HS | (7 - 2));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, GEN7_3DSTATE_BINDING_TABLE_POINTERS_HS | (2 - 2));
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    /* Disable TE */
    BEGIN_BATCH(batch, 4);
    OUT_BATCH(batch, GEN7_3DSTATE_TE | (4 - 2));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    /* Disable DS */
    BEGIN_BATCH(batch, 7);
    OUT_BATCH(batch, GEN7_3DSTATE_CONSTANT_DS | (7 - 2));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 6);
    OUT_BATCH(batch, GEN7_3DSTATE_DS | (6 - 2));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, GEN7_3DSTATE_BINDING_TABLE_POINTERS_DS | (2 - 2));
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    /* Disable STREAMOUT */
    BEGIN_BATCH(batch, 3);
    OUT_BATCH(batch, GEN7_3DSTATE_STREAMOUT | (3 - 2));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);
}

static void
gen7_emit_clip_state(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;

    OUT_BATCH(batch, GEN6_3DSTATE_CLIP | (4 - 2));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0); /* pass-through */
    OUT_BATCH(batch, 0);
}

static void
gen7_emit_sf_state(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;

    BEGIN_BATCH(batch, 14);
    OUT_BATCH(batch, GEN7_3DSTATE_SBE | (14 - 2));
    OUT_BATCH(batch,
              (1 << GEN7_SBE_NUM_OUTPUTS_SHIFT) |
              (1 << GEN7_SBE_URB_ENTRY_READ_LENGTH_SHIFT) |
              (0 << GEN7_SBE_URB_ENTRY_READ_OFFSET_SHIFT));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0); /* DW4 */
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0); /* DW9 */
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 7);
    OUT_BATCH(batch, GEN6_3DSTATE_SF | (7 - 2));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, GEN6_3DSTATE_SF_CULL_NONE);
    OUT_BATCH(batch, 2 << GEN6_3DSTATE_SF_TRIFAN_PROVOKE_SHIFT);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);
}

static void
gen7_emit_wm_state(VADriverContextP ctx, int kernel)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;
    struct media_render_state *render_state = &drv_ctx->render_state;
    unsigned int max_threads_shift = GEN7_PS_MAX_THREADS_SHIFT_IVB;
    unsigned int num_samples = 0;

    if (IS_HASWELL(drv_ctx->drv_data.device_id)) {
        max_threads_shift = GEN7_PS_MAX_THREADS_SHIFT_HSW;
        num_samples = 1 << GEN7_PS_SAMPLE_MASK_SHIFT_HSW;
    }

    BEGIN_BATCH(batch, 3);
    OUT_BATCH(batch, GEN6_3DSTATE_WM | (3 - 2));
    OUT_BATCH(batch,
              GEN7_WM_DISPATCH_ENABLE |
              GEN7_WM_PERSPECTIVE_PIXEL_BARYCENTRIC);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 7);
    OUT_BATCH(batch, GEN6_3DSTATE_CONSTANT_PS | (7 - 2));
    OUT_BATCH(batch, URB_CS_ENTRY_SIZE);
    OUT_BATCH(batch, 0);
    OUT_RELOC(batch,
              render_state->curbe.bo,
              I915_GEM_DOMAIN_INSTRUCTION, 0,
              0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 8);
    OUT_BATCH(batch, GEN7_3DSTATE_PS | (8 - 2));
    OUT_RELOC(batch,
              render_state->render_kernels[kernel].bo,
              I915_GEM_DOMAIN_INSTRUCTION, 0,
              0);
    OUT_BATCH(batch,
              (1 << GEN7_PS_SAMPLER_COUNT_SHIFT) |
              (5 << GEN7_PS_BINDING_TABLE_ENTRY_COUNT_SHIFT));
    OUT_BATCH(batch, 0); /* scratch space base offset */
    OUT_BATCH(batch,
              ((drv_ctx->render_state.max_wm_threads - 1) << max_threads_shift) | num_samples |
              GEN7_PS_PUSH_CONSTANT_ENABLE |
              GEN7_PS_ATTRIBUTE_ENABLE |
              GEN7_PS_16_DISPATCH_ENABLE);
    OUT_BATCH(batch,
              (6 << GEN7_PS_DISPATCH_START_GRF_SHIFT_0));
    OUT_BATCH(batch, 0); /* kernel 1 pointer */
    OUT_BATCH(batch, 0); /* kernel 2 pointer */
    ADVANCE_BATCH(batch);
}

static void
gen7_emit_vertex_element_state(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;

    /* Set up our vertex elements, sourced from the single vertex buffer. */
    OUT_BATCH(batch, RCMD_VERTEX_ELEMENTS | (5 - 2));
    /* offset 0: X,Y -> {X, Y, 1.0, 1.0} */
    OUT_BATCH(batch, (0 << GEN6_VE0_VERTEX_BUFFER_INDEX_SHIFT) |
              GEN6_VE0_VALID |
              (I965_SURFACEFORMAT_R32G32_FLOAT << VE0_FORMAT_SHIFT) |
              (0 << VE0_OFFSET_SHIFT));
    OUT_BATCH(batch, (I965_VFCOMPONENT_STORE_SRC << VE1_VFCOMPONENT_0_SHIFT) |
              (I965_VFCOMPONENT_STORE_SRC << VE1_VFCOMPONENT_1_SHIFT) |
              (I965_VFCOMPONENT_STORE_1_FLT << VE1_VFCOMPONENT_2_SHIFT) |
              (I965_VFCOMPONENT_STORE_1_FLT << VE1_VFCOMPONENT_3_SHIFT));
    /* offset 8: S0, T0 -> {S0, T0, 1.0, 1.0} */
    OUT_BATCH(batch, (0 << GEN6_VE0_VERTEX_BUFFER_INDEX_SHIFT) |
              GEN6_VE0_VALID |
              (I965_SURFACEFORMAT_R32G32_FLOAT << VE0_FORMAT_SHIFT) |
              (8 << VE0_OFFSET_SHIFT));
    OUT_BATCH(batch, (I965_VFCOMPONENT_STORE_SRC << VE1_VFCOMPONENT_0_SHIFT) |
              (I965_VFCOMPONENT_STORE_SRC << VE1_VFCOMPONENT_1_SHIFT) |
              (I965_VFCOMPONENT_STORE_1_FLT << VE1_VFCOMPONENT_2_SHIFT) |
              (I965_VFCOMPONENT_STORE_1_FLT << VE1_VFCOMPONENT_3_SHIFT));
}

static void
gen7_emit_vertices(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;

    BEGIN_BATCH(batch, 5);
    OUT_BATCH(batch, RCMD_VERTEX_BUFFERS | (5 - 2));
    OUT_BATCH(batch,
              (0 << GEN6_VB0_BUFFER_INDEX_SHIFT) |
              GEN6_VB0_VERTEXDATA |
              GEN7_VB0_ADDRESS_MODIFYENABLE |
              ((4 * 4) << VB0_BUFFER_PITCH_SHIFT));
    OUT_RELOC(batch, render_state->vb.vertex_buffer, I915_GEM_DOMAIN_VERTEX, 0, 0);
    OUT_RELOC(batch, render_state->vb.vertex_buffer, I915_GEM_DOMAIN_VERTEX, 0, 12 * 4);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);

    BEGIN_BATCH(batch, 7);
    OUT_BATCH(batch, RCMD_3DPRIMITIVE | (7 - 2));
    OUT_BATCH(batch,
              _3DPRIM_RECTLIST |
              GEN7_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL);
    OUT_BATCH(batch, 3); /* vertex count per instance */
    OUT_BATCH(batch, 0); /* start vertex offset */
    OUT_BATCH(batch, 1); /* single instance */
    OUT_BATCH(batch, 0); /* start instance location */
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);
}

static void
gen7_render_emit_states(VADriverContextP ctx, int kernel)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;

    media_batchbuffer_start_atomic(batch, 0x1000);
    media_batchbuffer_emit_mi_flush(batch);
    gen7_emit_invarient_states(ctx);
    gen7_emit_state_base_address(ctx);
    gen7_emit_viewport_state_pointers(ctx);
    gen7_emit_urb(ctx);
    gen7_emit_cc_state_pointers(ctx);
    gen7_emit_sampler_state_pointers(ctx);
    gen7_emit_bypass_state(ctx);
    gen7_emit_vs_state(ctx);
    gen7_emit_clip_state(ctx);
    gen7_emit_sf_state(ctx);
    gen7_emit_wm_state(ctx, kernel);
    gen7_emit_binding_table(ctx);
    gen7_emit_depth_buffer_state(ctx);
    gen7_emit_drawing_rectangle(ctx);
    gen7_emit_vertex_element_state(ctx);
    gen7_emit_vertices(ctx);
    media_batchbuffer_end_atomic(batch);
}

static void
i965_subpic_render_src_surfaces_state(VADriverContextP ctx,
                                      struct object_surface *obj_surface)
{
    dri_bo *subpic_region;
    unsigned int index = obj_surface->subpic_render_idx;
    struct object_subpic *obj_subpic = obj_surface->obj_subpic[index];
    struct object_image *obj_image = obj_subpic->obj_image;

    subpic_region = obj_image->bo;
    /*subpicture surface*/
    i965_render_src_surface_state(ctx, 1, subpic_region, 0, obj_subpic->width, obj_subpic->height, obj_subpic->pitch, obj_subpic->format, 0);
    i965_render_src_surface_state(ctx, 2, subpic_region, 0, obj_subpic->width, obj_subpic->height, obj_subpic->pitch, obj_subpic->format, 0);

}

static void
gen7_subpicture_render_blend_state(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    struct gen6_blend_state *blend_state;

    dri_bo_unmap(render_state->cc.state);
    dri_bo_map(render_state->cc.blend, 1);
    assert(render_state->cc.blend->virtual);
    blend_state = render_state->cc.blend->virtual;
    memset(blend_state, 0, sizeof(*blend_state));
    blend_state->blend0.dest_blend_factor = I965_BLENDFACTOR_INV_SRC_ALPHA;
    blend_state->blend0.source_blend_factor = I965_BLENDFACTOR_SRC_ALPHA;
    blend_state->blend0.blend_func = I965_BLENDFUNCTION_ADD;
    blend_state->blend0.blend_enable = 1;
    blend_state->blend1.post_blend_clamp_enable = 1;
    blend_state->blend1.pre_blend_clamp_enable = 1;
    blend_state->blend1.clamp_range = 0; /* clamp range [0, 1] */
    dri_bo_unmap(render_state->cc.blend);
}

static void
i965_subpic_render_upload_constants(VADriverContextP ctx,
                                    struct object_surface *obj_surface)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    float *constant_buffer;
    float global_alpha = 1.0;
    unsigned int index = obj_surface->subpic_render_idx;
    struct object_subpic *obj_subpic = obj_surface->obj_subpic[index];

    if (obj_subpic->flags & VA_SUBPICTURE_GLOBAL_ALPHA) {
        global_alpha = obj_subpic->global_alpha;
    }

    dri_bo_map(render_state->curbe.bo, 1);

    assert(render_state->curbe.bo->virtual);
    constant_buffer = render_state->curbe.bo->virtual;
    *constant_buffer = global_alpha;

    dri_bo_unmap(render_state->curbe.bo);
}

static void
i965_subpic_render_upload_vertex(VADriverContextP ctx,
                                 struct object_surface *obj_surface,
                                 const VARectangle *output_rect)
{
    unsigned int index = obj_surface->subpic_render_idx;
    struct object_subpic     *obj_subpic   = obj_surface->obj_subpic[index];
    float tex_coords[4], vid_coords[4];
    VARectangle dst_rect;

    if (obj_subpic->flags & VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD)
        dst_rect = obj_subpic->dst_rect;
    else {
        const float sx  = (float)output_rect->width  / obj_surface->orig_width;
        const float sy  = (float)output_rect->height / obj_surface->orig_height;
        dst_rect.x      = output_rect->x + sx * obj_subpic->dst_rect.x;
        dst_rect.y      = output_rect->y + sy * obj_subpic->dst_rect.y;
        dst_rect.width  = sx * obj_subpic->dst_rect.width;
        dst_rect.height = sy * obj_subpic->dst_rect.height;
    }

    tex_coords[0] = (float)obj_subpic->src_rect.x / obj_subpic->width;
    tex_coords[1] = (float)obj_subpic->src_rect.y / obj_subpic->height;
    tex_coords[2] = (float)(obj_subpic->src_rect.x + obj_subpic->src_rect.width) / obj_subpic->width;
    tex_coords[3] = (float)(obj_subpic->src_rect.y + obj_subpic->src_rect.height) / obj_subpic->height;

    vid_coords[0] = dst_rect.x;
    vid_coords[1] = dst_rect.y;
    vid_coords[2] = (float)(dst_rect.x + dst_rect.width);
    vid_coords[3] = (float)(dst_rect.y + dst_rect.height);

    i965_fill_vertex_buffer(ctx, tex_coords, vid_coords);
}

static void
gen7_subpicture_render_setup_states(
    VADriverContextP   ctx,
    struct object_surface *obj_surface,
    const VARectangle *src_rect,
    const VARectangle *dst_rect
)
{
    i965_render_dest_surface_state(ctx, 0);
    i965_subpic_render_src_surfaces_state(ctx, obj_surface);
    gen7_render_sampler(ctx);
    i965_render_cc_viewport(ctx);
    gen7_render_color_calc_state(ctx);
    gen7_subpicture_render_blend_state(ctx);
    gen7_render_depth_stencil_state(ctx);
    i965_subpic_render_upload_constants(ctx, obj_surface);
    i965_subpic_render_upload_vertex(ctx, obj_surface, dst_rect);
}

static void
gen7_render_put_surface(
    VADriverContextP   ctx,
    struct object_surface *obj_surface,
    const VARectangle *src_rect,
    const VARectangle *dst_rect,
    unsigned int       flags
)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;

    gen7_render_initialize(ctx);
    gen7_render_setup_states(ctx, obj_surface, src_rect, dst_rect, flags);
    i965_clear_dest_region(ctx);
    gen7_render_emit_states(ctx, PS_KERNEL);
    media_batchbuffer_flush(batch);
}

static void
gen7_render_put_subpicture(
    VADriverContextP   ctx,
    struct object_surface *obj_surface,
    const VARectangle *src_rect,
    const VARectangle *dst_rect
)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    MEDIA_BATCH_BUFFER *batch = drv_ctx->render_batch;

    gen7_render_initialize(ctx);
    gen7_subpicture_render_setup_states(ctx, obj_surface, src_rect, dst_rect);
    gen7_render_emit_states(ctx, PS_SUBPIC_KERNEL);
    media_batchbuffer_flush(batch);
}



void
media_render_put_surface(
    VADriverContextP   ctx,
    struct object_surface *obj_surface,
    const VARectangle *src_rect,
    const VARectangle *dst_rect,
    unsigned int       flags
)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;

    if (render_state->render_put_surface)
        render_state->render_put_surface(ctx, obj_surface, src_rect, dst_rect, flags);

    return;
}

void
media_render_put_subpicture(
    VADriverContextP   ctx,
    struct object_surface *obj_surface,
    const VARectangle *src_rect,
    const VARectangle *dst_rect
)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;

    if (render_state->render_put_subpicture)
        render_state->render_put_subpicture(ctx, obj_surface, src_rect, dst_rect);
}

static void
genx_render_terminate(VADriverContextP ctx)
{
    int i;
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;

    dri_bo_unreference(render_state->curbe.bo);
    render_state->curbe.bo = NULL;

    for (i = 0; i < sizeof(render_kernels_gen7_haswell) / sizeof(struct media_render_kernel); i++) {
        struct media_render_kernel *kernel = &render_state->render_kernels[i];

        dri_bo_unreference(kernel->bo);
        kernel->bo = NULL;
    }

    dri_bo_unreference(render_state->vb.vertex_buffer);
    render_state->vb.vertex_buffer = NULL;
    dri_bo_unreference(render_state->vs.state);
    render_state->vs.state = NULL;
    dri_bo_unreference(render_state->sf.state);
    render_state->sf.state = NULL;
    dri_bo_unreference(render_state->wm.sampler);
    render_state->wm.sampler = NULL;
    dri_bo_unreference(render_state->wm.state);
    render_state->wm.state = NULL;
    dri_bo_unreference(render_state->wm.surface_state_binding_table_bo);
    dri_bo_unreference(render_state->cc.viewport);
    render_state->cc.viewport = NULL;
    dri_bo_unreference(render_state->cc.state);
    render_state->cc.state = NULL;
    dri_bo_unreference(render_state->cc.blend);
    render_state->cc.blend = NULL;
    dri_bo_unreference(render_state->cc.depth_stencil);
    render_state->cc.depth_stencil = NULL;

    if (render_state->draw_region) {
        dri_bo_unreference(render_state->draw_region->bo);
        free(render_state->draw_region);
        render_state->draw_region = NULL;
    }
}

bool
media_drv_gen75_render_init(VADriverContextP ctx)
{
    MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
    struct media_render_state *render_state = &drv_ctx->render_state;
    int i;

    /* kernel */

    if (IS_HASWELL (drv_ctx->drv_data.device_id)) {
        memcpy(render_state->render_kernels, render_kernels_gen7_haswell,
               sizeof(render_kernels_gen7_haswell));
        render_state->render_put_surface = gen7_render_put_surface;
        render_state->render_put_subpicture = gen7_render_put_subpicture;
    } else {
        return false;
    }

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
    render_state->render_terminate = genx_render_terminate;

    for (i = 0; i < sizeof(render_kernels_gen7_haswell) / sizeof(struct media_render_kernel); i++) {
        struct media_render_kernel *kernel = &render_state->render_kernels[i];

        if (!kernel->size)
            continue;

        kernel->bo = dri_bo_alloc(drv_ctx->drv_data.bufmgr, kernel->name,
                                  kernel->size, 0x1000);
        assert(kernel->bo);
        dri_bo_subdata(kernel->bo, 0, kernel->size, kernel->bin);
    }

    /* constant buffer */
    render_state->curbe.bo = dri_bo_alloc(drv_ctx->drv_data.bufmgr,
                      "constant buffer",
                      4096, 64);
    assert(render_state->curbe.bo);

    return true;
}
