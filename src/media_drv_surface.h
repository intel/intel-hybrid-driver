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

#ifndef _MEDIA__DRIVER_SURFACE_H
#define _MEDIA__DRIVER_SURFACE_H
#include <va/va_drmcommon.h>
#include <drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>
#include "object_heap.h"
#include "media_drv_defines.h"
#include <va/va_backend.h>
#include "media_drv_init.h"
#define SURFACE_REFERENCED      (1 << 0)
#define SURFACE_DISPLAYED       (1 << 1)
#define SURFACE_DERIVED         (1 << 2)
#define SURFACE_REF_DIS_MASK    ((SURFACE_REFERENCED) | \
                                 (SURFACE_DISPLAYED))
#define SURFACE_ALL_MASK        ((SURFACE_REFERENCED) | \
                                 (SURFACE_DISPLAYED) |  \
                                 (SURFACE_DERIVED))
#define HAS_TILED_SURFACE(drv_ctx) ((drv_ctx)->codec_info->tiled_surface)
#define NEW_SURFACE_ID() object_heap_allocate(&drv_ctx->surface_heap);
#define SURFACE(id) ((struct object_surface *)object_heap_lookup(&drv_ctx->surface_heap, id))



typedef struct _input_surf_params
{
  UINT width;
  UINT height;
  INT format;
  INT expected_fourcc;
  INT memory_type;
  UINT index;
  UINT surface_usage_hint;
  VASurfaceID *surfaces;
  VASurfaceAttribExternalBuffers *memory_attibute;
} input_surf_params;


struct object_surface
{
  struct object_base base;
  VASurfaceStatus status;
  VASubpictureID subpic[MEDIA_GEN_MAX_SUBPIC_FORMATS];
  struct object_subpic *obj_subpic[MEDIA_GEN_MAX_SUBPIC_FORMATS];
  UINT subpic_render_idx;

  INT width;			/* the pitch of plane 0 in bytes in horizontal direction */
  INT height;			/* the pitch of plane 0 in bytes in vertical direction */
  INT size;
  INT orig_width;		/* the width of plane 0 in pixels */
  INT orig_height;		/* the height of plane 0 in pixels */
  INT flags;
  UINT fourcc;
  dri_bo *bo;
  VAImageID locked_image_id;
  VOID (*free_private_data) (VOID **data);
  VOID *private_data;
  UINT subsampling;
  INT x_cb_offset;
  INT y_cb_offset;
  INT x_cr_offset;
  INT y_cr_offset;
  INT cb_cr_width;
  INT cb_cr_height;
  INT cb_cr_pitch;
};
struct gen7_surface_state
{
  struct
  {
    UINT cube_pos_z:1;
    UINT cube_neg_z:1;
    UINT cube_pos_y:1;
    UINT cube_neg_y:1;
    UINT cube_pos_x:1;
    UINT cube_neg_x:1;
    UINT pad2:2;
    UINT render_cache_read_write:1;
    UINT pad1:1;
    UINT surface_array_spacing:1;
    UINT vert_line_stride_ofs:1;
    UINT vert_line_stride:1;
    UINT tile_walk:1;
    UINT tiled_surface:1;
    UINT horizontal_alignment:1;
    UINT vertical_alignment:2;
    UINT surface_format:9;	   /**< BRW_SURFACEFORMAT_x */
    UINT pad0:1;
    UINT is_array:1;
    UINT surface_type:3;	   /**< BRW_SURFACE_1D/2D/3D/CUBE */
  } ss0;

  struct
  {
    UINT base_addr;
  } ss1;

  struct
  {
    UINT width:14;
    UINT pad1:2;
    UINT height:14;
    UINT pad0:2;
  } ss2;

  struct
  {
    UINT pitch:18;
    UINT pad:3;
    UINT depth:11;
  } ss3;

  struct
  {
    UINT multisample_position_palette_index:3;
    UINT num_multisamples:3;
    UINT multisampled_surface_storage_format:1;
    UINT render_target_view_extent:11;
    UINT min_array_elt:11;
    UINT rotation:2;
    UINT pad0:1;
  } ss4;

  struct
  {
    UINT mip_count:4;
    UINT min_lod:4;
    UINT pad1:12;
    UINT y_offset:4;
    UINT pad0:1;
    UINT x_offset:7;
  } ss5;

  struct
  {
    UINT pad;		/* Multisample Control Surface stuff */
  } ss6;

  struct
  {
    UINT resource_min_lod:12;
    UINT pad0:4;
    UINT shader_chanel_select_a:3;
    UINT shader_chanel_select_b:3;
    UINT shader_chanel_select_g:3;
    UINT shader_chanel_select_r:3;
    UINT alpha_clear_color:1;
    UINT blue_clear_color:1;
    UINT green_clear_color:1;
    UINT red_clear_color:1;
  } ss7;
};

typedef struct gen7_surface_state2
{

 /* union
  {*/
    struct
    {
      UINT surface_base_address;
    }ss0;
    /*struct
    {
      unsigned int val;

    };
  } ss0;*/
  /*union
  {*/
    struct
    {
      UINT cbcr_pixel_offset_v_direction:2;
      UINT picture_structure:2;
      UINT width:14;
      UINT height:14;
   /* };
    struct
    {
      unsigned int val;
    };*/
  } ss1;
  /*union
  {*/
    struct
    {
      UINT tile_walk:1;
      UINT tiled_surface:1;
      UINT half_pitch_for_chroma:1;
      UINT pitch:18;
      UINT pad0:1;
      UINT surface_object_control_data:4;
      UINT pad1:1;
      UINT interleave_chroma:1;
      UINT surface_format:4;
   /* };
    struct
    {
      unsigned int val;
    };*/
  } ss2;

/*  union
  {*/
    struct
    {
      UINT y_offset_for_cb:15;
      UINT pad0:1;
      UINT x_offset_for_cb:14;
      UINT pad1:2;
   /* };
    struct
    {
      unsigned int val;
    };*/
  } ss3;

  /*union
  {*/
    struct
    {
      UINT y_offset_for_cr:15;
      UINT pad0:1;
      UINT x_offset_for_cr:14;
      UINT pad1:2;
    /*};
   struct
    {
      unsigned int val;
    };*/
  } ss4;

  struct
  {
    UINT pad0;
  } ss5;

  struct
  {
    UINT pad0;
  } ss6;

  struct
  {
    UINT pad0;
  } ss7;
} SURFACE_STATE_ADV_G7;
VOID
media_destroy_surface (struct object_heap *heap, struct object_base *obj);
VAStatus
media_sync_surface (MEDIA_DRV_CONTEXT * drv_ctx, VASurfaceID render_target);
VOID
media_alloc_surface_bo (VADriverContextP ctx,
			struct object_surface *obj_surface,
			INT tiled,
			UINT fourcc, UINT subsampling);
VAStatus
media_drv_create_surface (VADriverContextP ctx, input_surf_params * params);
#endif
