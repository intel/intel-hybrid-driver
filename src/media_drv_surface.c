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

#include <stdio.h>
#include <va/va_backend.h>
#include "media_drv_init.h"
#include "media_drv_hw.h"
#include "media_drv_util.h"
#include "media_drv_surface.h"

//#define DEBUG
VAStatus
media_sync_surface (MEDIA_DRV_CONTEXT * drv_ctx, VASurfaceID render_target)
{

  struct object_surface *obj_surface = SURFACE (render_target);

  MEDIA_DRV_ASSERT (obj_surface);

  if (obj_surface->bo)
    drm_intel_bo_wait_rendering (obj_surface->bo);

  return VA_STATUS_SUCCESS;
}

static VAStatus
media_suface_external_memory (VADriverContextP ctx,
			      struct object_surface *obj_surface,
			      INT external_memory_type,
			      VASurfaceAttribExternalBuffers *
			      memory_attibute, INT index)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  MEDIA_DRV_ASSERT (ctx);
  MEDIA_DRV_ASSERT (drv_ctx);


  if (!memory_attibute ||
      !memory_attibute->buffers || index > memory_attibute->num_buffers)
    return VA_STATUS_ERROR_INVALID_PARAMETER;

  MEDIA_DRV_ASSERT (obj_surface->orig_width == memory_attibute->width);
  MEDIA_DRV_ASSERT (obj_surface->orig_height == memory_attibute->height);
  MEDIA_DRV_ASSERT (memory_attibute->num_planes >= 1);

  obj_surface->fourcc = memory_attibute->pixel_format;
  obj_surface->width = memory_attibute->pitches[0];
  obj_surface->size = memory_attibute->data_size;

  if (memory_attibute->num_planes == 1)
    obj_surface->height = memory_attibute->data_size / obj_surface->width;
  else
    obj_surface->height = memory_attibute->offsets[1] / obj_surface->width;

  obj_surface->x_cb_offset = 0;	/* X offset is always 0 */
  obj_surface->x_cr_offset = 0;

  switch (obj_surface->fourcc)
    {
    case VA_FOURCC ('N', 'V', '1', '2'):
      MEDIA_DRV_ASSERT (memory_attibute->num_planes == 2);
      MEDIA_DRV_ASSERT (memory_attibute->pitches[0] ==
			memory_attibute->pitches[1]);

      obj_surface->subsampling = SUBSAMPLE_YUV420;
      obj_surface->y_cb_offset = obj_surface->height;
      obj_surface->y_cr_offset = obj_surface->height;
      obj_surface->cb_cr_width = obj_surface->orig_width / 2;
      obj_surface->cb_cr_height = obj_surface->orig_height / 2;
      obj_surface->cb_cr_pitch = memory_attibute->pitches[1];

      break;

    case VA_FOURCC ('Y', 'V', '1', '2'):
    case VA_FOURCC ('I', 'M', 'C', '1'):
      MEDIA_DRV_ASSERT (memory_attibute->num_planes == 3);
      MEDIA_DRV_ASSERT (memory_attibute->pitches[1] ==
			memory_attibute->pitches[2]);

      obj_surface->subsampling = SUBSAMPLE_YUV420;
      obj_surface->y_cr_offset = obj_surface->height;
      obj_surface->y_cb_offset =
	memory_attibute->offsets[2] / obj_surface->width;
      obj_surface->cb_cr_width = obj_surface->orig_width / 2;
      obj_surface->cb_cr_height = obj_surface->orig_height / 2;
      obj_surface->cb_cr_pitch = memory_attibute->pitches[1];

      break;

    case VA_FOURCC ('I', '4', '2', '0'):
    case VA_FOURCC ('I', 'Y', 'U', 'V'):
    case VA_FOURCC ('I', 'M', 'C', '3'):
      MEDIA_DRV_ASSERT (memory_attibute->num_planes == 3);
      MEDIA_DRV_ASSERT (memory_attibute->pitches[1] ==
			memory_attibute->pitches[2]);

      obj_surface->subsampling = SUBSAMPLE_YUV420;
      obj_surface->y_cb_offset = obj_surface->height;
      obj_surface->y_cr_offset =
	memory_attibute->offsets[2] / obj_surface->width;
      obj_surface->cb_cr_width = obj_surface->orig_width / 2;
      obj_surface->cb_cr_height = obj_surface->orig_height / 2;
      obj_surface->cb_cr_pitch = memory_attibute->pitches[1];

      break;

    case VA_FOURCC ('Y', 'U', 'Y', '2'):
    case VA_FOURCC ('U', 'Y', 'V', 'Y'):
      MEDIA_DRV_ASSERT (memory_attibute->num_planes == 1);

      obj_surface->subsampling = SUBSAMPLE_YUV422H;
      obj_surface->y_cb_offset = 0;
      obj_surface->y_cr_offset = 0;
      obj_surface->cb_cr_width = obj_surface->orig_width / 2;
      obj_surface->cb_cr_height = obj_surface->orig_height;
      obj_surface->cb_cr_pitch = memory_attibute->pitches[0];

      break;

    case VA_FOURCC ('R', 'G', 'B', 'A'):
    case VA_FOURCC ('R', 'G', 'B', 'X'):
    case VA_FOURCC ('B', 'G', 'R', 'A'):
    case VA_FOURCC ('B', 'G', 'R', 'X'):
      MEDIA_DRV_ASSERT (memory_attibute->num_planes == 1);

      obj_surface->subsampling = SUBSAMPLE_RGBX;
      obj_surface->y_cb_offset = 0;
      obj_surface->y_cr_offset = 0;
      obj_surface->cb_cr_width = 0;
      obj_surface->cb_cr_height = 0;
      obj_surface->cb_cr_pitch = 0;

      break;

    case VA_FOURCC ('Y', '8', '0', '0'):	/* monochrome surface */
      MEDIA_DRV_ASSERT (memory_attibute->num_planes == 1);

      obj_surface->subsampling = SUBSAMPLE_YUV400;
      obj_surface->y_cb_offset = 0;
      obj_surface->y_cr_offset = 0;
      obj_surface->cb_cr_width = 0;
      obj_surface->cb_cr_height = 0;
      obj_surface->cb_cr_pitch = 0;

      break;

    case VA_FOURCC ('4', '1', '1', 'P'):
      MEDIA_DRV_ASSERT (memory_attibute->num_planes == 3);
      MEDIA_DRV_ASSERT (memory_attibute->pitches[1] ==
			memory_attibute->pitches[2]);

      obj_surface->subsampling = SUBSAMPLE_YUV411;
      obj_surface->y_cb_offset = 0;
      obj_surface->y_cr_offset = 0;
      obj_surface->cb_cr_width = obj_surface->orig_width / 4;
      obj_surface->cb_cr_height = obj_surface->orig_height;
      obj_surface->cb_cr_pitch = memory_attibute->pitches[1];

      break;

    case VA_FOURCC ('4', '2', '2', 'H'):
      MEDIA_DRV_ASSERT (memory_attibute->num_planes == 3);
      MEDIA_DRV_ASSERT (memory_attibute->pitches[1] ==
			memory_attibute->pitches[2]);

      obj_surface->subsampling = SUBSAMPLE_YUV422H;
      obj_surface->y_cb_offset = obj_surface->height;
      obj_surface->y_cr_offset =
	memory_attibute->offsets[2] / obj_surface->width;
      obj_surface->cb_cr_width = obj_surface->orig_width / 2;
      obj_surface->cb_cr_height = obj_surface->orig_height;
      obj_surface->cb_cr_pitch = memory_attibute->pitches[1];

      break;

    case VA_FOURCC ('4', '2', '2', 'V'):
      MEDIA_DRV_ASSERT (memory_attibute->num_planes == 3);
      MEDIA_DRV_ASSERT (memory_attibute->pitches[1] ==
			memory_attibute->pitches[2]);

      obj_surface->subsampling = SUBSAMPLE_YUV422H;
      obj_surface->y_cb_offset = obj_surface->height;
      obj_surface->y_cr_offset =
	memory_attibute->offsets[2] / obj_surface->width;
      obj_surface->cb_cr_width = obj_surface->orig_width;
      obj_surface->cb_cr_height = obj_surface->orig_height / 2;
      obj_surface->cb_cr_pitch = memory_attibute->pitches[1];

      break;

    case VA_FOURCC ('4', '4', '4', 'P'):
      MEDIA_DRV_ASSERT (memory_attibute->num_planes == 3);
      MEDIA_DRV_ASSERT (memory_attibute->pitches[1] ==
			memory_attibute->pitches[2]);

      obj_surface->subsampling = SUBSAMPLE_YUV444;
      obj_surface->y_cb_offset = obj_surface->height;
      obj_surface->y_cr_offset =
	memory_attibute->offsets[2] / obj_surface->width;
      obj_surface->cb_cr_width = obj_surface->orig_width;
      obj_surface->cb_cr_height = obj_surface->orig_height;
      obj_surface->cb_cr_pitch = memory_attibute->pitches[1];

      break;

    default:

      return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

  if (external_memory_type == I965_SURFACE_MEM_GEM_FLINK)
    obj_surface->bo =
      drm_intel_bo_gem_create_from_name (drv_ctx->drv_data.bufmgr,
					 "gem flinked vaapi surface",
					 memory_attibute->buffers[index]);
  else if (external_memory_type == I965_SURFACE_MEM_DRM_PRIME)
    obj_surface->bo =
      drm_intel_bo_gem_create_from_prime (drv_ctx->drv_data.bufmgr,
					  memory_attibute->buffers[index],
					  obj_surface->size);

  if (!obj_surface->bo)
    return VA_STATUS_ERROR_INVALID_PARAMETER;

  return VA_STATUS_SUCCESS;
}

VOID
media_alloc_surface_bo (VADriverContextP ctx,
			struct object_surface * obj_surface,
			INT tiled, UINT fourcc, UINT subsampling)
{
  INT region_width, region_height;
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  MEDIA_DRV_ASSERT (ctx);
  MEDIA_DRV_ASSERT (drv_ctx);
  if (obj_surface->bo)
    {
      MEDIA_DRV_ASSERT (obj_surface->fourcc);
      MEDIA_DRV_ASSERT (obj_surface->fourcc == fourcc);
      MEDIA_DRV_ASSERT (obj_surface->subsampling == subsampling);
      return;
    }
  obj_surface->x_cb_offset = 0;	/* X offset is always 0 */
  obj_surface->x_cr_offset = 0;
  if (tiled)
    {
      MEDIA_DRV_ASSERT (fourcc != VA_FOURCC ('I', '4', '2', '0') &&
			fourcc != VA_FOURCC ('I', 'Y', 'U', 'V') &&
			fourcc != VA_FOURCC ('Y', 'V', '1', '2'));

      obj_surface->width = ALIGN (obj_surface->orig_width, 128);
      obj_surface->height = ALIGN (obj_surface->orig_height, 32);
      region_height = obj_surface->height;
      switch (fourcc)
	{
	case VA_FOURCC ('N', 'V', '1', '2'):
	  MEDIA_DRV_ASSERT (subsampling == SUBSAMPLE_YUV420);
	  obj_surface->cb_cr_pitch = obj_surface->width;
	  obj_surface->cb_cr_width = obj_surface->orig_width / 2;
	  obj_surface->cb_cr_height = obj_surface->orig_height / 2;
	  obj_surface->y_cb_offset = obj_surface->height;
	  obj_surface->y_cr_offset = obj_surface->height;
	  region_width = obj_surface->width;
	  region_height =
	    obj_surface->height + ALIGN (obj_surface->cb_cr_height, 32);

	  break;

	case VA_FOURCC ('I', 'M', 'C', '1'):
	  MEDIA_DRV_ASSERT (subsampling == SUBSAMPLE_YUV420);
	  obj_surface->cb_cr_pitch = obj_surface->width;
	  obj_surface->cb_cr_width = obj_surface->orig_width / 2;
	  obj_surface->cb_cr_height = obj_surface->orig_height / 2;
	  obj_surface->y_cr_offset = obj_surface->height;
	  obj_surface->y_cb_offset =
	    obj_surface->y_cr_offset + ALIGN (obj_surface->cb_cr_height, 32);
	  region_width = obj_surface->width;
	  region_height =
	    obj_surface->height + ALIGN (obj_surface->cb_cr_height, 32) * 2;

	  break;

	case VA_FOURCC ('I', 'M', 'C', '3'):
	  MEDIA_DRV_ASSERT (subsampling == SUBSAMPLE_YUV420);
	  obj_surface->cb_cr_pitch = obj_surface->width;
	  obj_surface->cb_cr_width = obj_surface->orig_width / 2;
	  obj_surface->cb_cr_height = obj_surface->orig_height / 2;
	  obj_surface->y_cb_offset = obj_surface->height;
	  obj_surface->y_cr_offset =
	    obj_surface->y_cb_offset + ALIGN (obj_surface->cb_cr_height, 32);
	  region_width = obj_surface->width;
	  region_height =
	    obj_surface->height + ALIGN (obj_surface->cb_cr_height, 32) * 2;

	  break;

	case VA_FOURCC ('4', '2', '2', 'H'):
	  MEDIA_DRV_ASSERT (subsampling == SUBSAMPLE_YUV422H);
	  obj_surface->cb_cr_pitch = obj_surface->width;
	  obj_surface->cb_cr_width = obj_surface->orig_width / 2;
	  obj_surface->cb_cr_height = obj_surface->orig_height;
	  obj_surface->y_cb_offset = obj_surface->height;
	  obj_surface->y_cr_offset =
	    obj_surface->y_cb_offset + ALIGN (obj_surface->cb_cr_height, 32);
	  region_width = obj_surface->width;
	  region_height =
	    obj_surface->height + ALIGN (obj_surface->cb_cr_height, 32) * 2;

	  break;

	case VA_FOURCC ('4', '2', '2', 'V'):
	  MEDIA_DRV_ASSERT (subsampling == SUBSAMPLE_YUV422V);
	  obj_surface->cb_cr_pitch = obj_surface->width;
	  obj_surface->cb_cr_width = obj_surface->orig_width;
	  obj_surface->cb_cr_height = obj_surface->orig_height / 2;
	  obj_surface->y_cb_offset = obj_surface->height;
	  obj_surface->y_cr_offset =
	    obj_surface->y_cb_offset + ALIGN (obj_surface->cb_cr_height, 32);
	  region_width = obj_surface->width;
	  region_height =
	    obj_surface->height + ALIGN (obj_surface->cb_cr_height, 32) * 2;

	  break;

	case VA_FOURCC ('4', '1', '1', 'P'):
	  MEDIA_DRV_ASSERT (subsampling == SUBSAMPLE_YUV411);
	  obj_surface->cb_cr_pitch = obj_surface->width;
	  obj_surface->cb_cr_width = obj_surface->orig_width / 4;
	  obj_surface->cb_cr_height = obj_surface->orig_height;
	  obj_surface->y_cb_offset = obj_surface->height;
	  obj_surface->y_cr_offset =
	    obj_surface->y_cb_offset + ALIGN (obj_surface->cb_cr_height, 32);
	  region_width = obj_surface->width;
	  region_height =
	    obj_surface->height + ALIGN (obj_surface->cb_cr_height, 32) * 2;

	  break;

	case VA_FOURCC ('4', '4', '4', 'P'):
	  MEDIA_DRV_ASSERT (subsampling == SUBSAMPLE_YUV444);
	  obj_surface->cb_cr_pitch = obj_surface->width;
	  obj_surface->cb_cr_width = obj_surface->orig_width;
	  obj_surface->cb_cr_height = obj_surface->orig_height;
	  obj_surface->y_cb_offset = obj_surface->height;
	  obj_surface->y_cr_offset =
	    obj_surface->y_cb_offset + ALIGN (obj_surface->cb_cr_height, 32);
	  region_width = obj_surface->width;
	  region_height =
	    obj_surface->height + ALIGN (obj_surface->cb_cr_height, 32) * 2;

	  break;

	case VA_FOURCC ('Y', '8', '0', '0'):
	  MEDIA_DRV_ASSERT (subsampling == SUBSAMPLE_YUV400);
	  obj_surface->cb_cr_pitch = obj_surface->width;
	  obj_surface->cb_cr_width = 0;
	  obj_surface->cb_cr_height = 0;
	  obj_surface->y_cb_offset = obj_surface->height;
	  obj_surface->y_cr_offset =
	    obj_surface->y_cb_offset + ALIGN (obj_surface->cb_cr_height, 32);
	  region_width = obj_surface->width;
	  region_height =
	    obj_surface->height + ALIGN (obj_surface->cb_cr_height, 32) * 2;

	  break;

	case VA_FOURCC ('Y', 'U', 'Y', '2'):
	case VA_FOURCC ('U', 'Y', 'V', 'Y'):
	  MEDIA_DRV_ASSERT (subsampling == SUBSAMPLE_YUV422H);
	  obj_surface->width = ALIGN (obj_surface->orig_width * 2, 128);
	  obj_surface->cb_cr_pitch = obj_surface->width;
	  obj_surface->y_cb_offset = 0;
	  obj_surface->y_cr_offset = 0;
	  obj_surface->cb_cr_width = obj_surface->orig_width / 2;
	  obj_surface->cb_cr_height = obj_surface->orig_height / 2;
	  region_width = obj_surface->width;
	  region_height = obj_surface->height;

	  break;

	case VA_FOURCC ('R', 'G', 'B', 'A'):
	case VA_FOURCC ('R', 'G', 'B', 'X'):
	case VA_FOURCC ('B', 'G', 'R', 'A'):
	case VA_FOURCC ('B', 'G', 'R', 'X'):
	  MEDIA_DRV_ASSERT (subsampling == SUBSAMPLE_RGBX);

	  obj_surface->width = ALIGN (obj_surface->orig_width * 4, 128);
	  region_width = obj_surface->width;
	  region_height = obj_surface->height;
	  break;

	default:
	  /* Never get here */
	  MEDIA_DRV_ASSERT (0);
	  break;
	}
    }
  else
    {
      MEDIA_DRV_ASSERT (subsampling == SUBSAMPLE_YUV420 ||
			subsampling == SUBSAMPLE_YUV422H ||
			subsampling == SUBSAMPLE_YUV422V ||
			subsampling == SUBSAMPLE_RGBX
			|| subsampling == SUBSAMPLE_P208);

      region_width = obj_surface->width;
      region_height = obj_surface->height;

      switch (fourcc)
	{
	case VA_FOURCC ('N', 'V', '1', '2'):
	  obj_surface->y_cb_offset = obj_surface->height;
	  obj_surface->y_cr_offset = obj_surface->height;
	  obj_surface->cb_cr_width = obj_surface->orig_width / 2;
	  obj_surface->cb_cr_height = obj_surface->orig_height / 2;
	  obj_surface->cb_cr_pitch = obj_surface->width;
	  region_height = obj_surface->height + obj_surface->height / 2;
	  break;

	case VA_FOURCC ('Y', 'V', '1', '2'):
	case VA_FOURCC ('I', '4', '2', '0'):
	  if (fourcc == VA_FOURCC ('Y', 'V', '1', '2'))
	    {
	      obj_surface->y_cr_offset = obj_surface->height;
	      obj_surface->y_cb_offset =
		obj_surface->height + obj_surface->height / 4;
	    }
	  else
	    {
	      obj_surface->y_cb_offset = obj_surface->height;
	      obj_surface->y_cr_offset =
		obj_surface->height + obj_surface->height / 4;
	    }

	  obj_surface->cb_cr_width = obj_surface->orig_width / 2;
	  obj_surface->cb_cr_height = obj_surface->orig_height / 2;
	  obj_surface->cb_cr_pitch = obj_surface->width / 2;
	  region_height = obj_surface->height + obj_surface->height / 2;
	  break;

	case VA_FOURCC ('Y', 'U', 'Y', '2'):
	case VA_FOURCC ('U', 'Y', 'V', 'Y'):
	  obj_surface->width = ALIGN (obj_surface->orig_width * 2, 16);
	  obj_surface->y_cb_offset = 0;
	  obj_surface->y_cr_offset = 0;
	  obj_surface->cb_cr_width = obj_surface->orig_width / 2;
	  obj_surface->cb_cr_height = obj_surface->orig_height;
	  obj_surface->cb_cr_pitch = obj_surface->width;
	  region_width = obj_surface->width;
	  region_height = obj_surface->height;
	  break;
	case VA_FOURCC ('R', 'G', 'B', 'A'):
	case VA_FOURCC ('R', 'G', 'B', 'X'):
	case VA_FOURCC ('B', 'G', 'R', 'A'):
	case VA_FOURCC ('B', 'G', 'R', 'X'):
	  obj_surface->width = ALIGN (obj_surface->orig_width * 4, 16);
	  region_width = obj_surface->width;
	  region_height = obj_surface->height;
	  break;
	case VA_FOURCC ('P', '2', '0', '8'):
	  obj_surface->width = ALIGN (obj_surface->orig_width, 32);
	  region_width = obj_surface->width;
	  region_height = obj_surface->height;
	  break;

	default:
	  /* Never get here */
	  MEDIA_DRV_ASSERT (0);
	  break;
	}
    }

  obj_surface->size = ALIGN (region_width * region_height, 0x1000);

  if (tiled)
    {
      UINT tiling_mode = I915_TILING_Y;	/* always uses Y-tiled format */
      ULONG pitch;

      obj_surface->bo = drm_intel_bo_alloc_tiled (drv_ctx->drv_data.bufmgr,
						  "vaapi surface",
						  region_width,
						  region_height,
						  1, &tiling_mode, &pitch, 0);
      MEDIA_DRV_ASSERT (tiling_mode == I915_TILING_Y);
      MEDIA_DRV_ASSERT (pitch == obj_surface->width);
    }
  else
    {
      obj_surface->bo = dri_bo_alloc (drv_ctx->drv_data.bufmgr,
				      "vaapi surface",
				      obj_surface->size, 0x1000);
    }

  obj_surface->fourcc = fourcc;
  obj_surface->subsampling = subsampling;
  MEDIA_DRV_ASSERT (obj_surface->bo);
}

static VAStatus
media_surface_native_memory (VADriverContextP ctx,
			     struct object_surface *obj_surface,
			     INT format, INT expected_fourcc)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  MEDIA_DRV_ASSERT (ctx);
  MEDIA_DRV_ASSERT (drv_ctx);
  INT tiling = HAS_TILED_SURFACE (drv_ctx);
  if (!expected_fourcc)
    return VA_STATUS_SUCCESS;

  // todo, should we disable tiling for 422 format?
  if (expected_fourcc == VA_FOURCC ('I', '4', '2', '0') ||
      expected_fourcc == VA_FOURCC ('I', 'Y', 'U', 'V') ||
      expected_fourcc == VA_FOURCC ('Y', 'V', '1', '2')
      || expected_fourcc == VA_FOURCC ('P', '2', '0', '8'))
    tiling = 0;

  media_alloc_surface_bo (ctx, obj_surface, tiling, expected_fourcc,
			  media_get_sampling_from_fourcc (expected_fourcc));

  return VA_STATUS_SUCCESS;
}

VAStatus
media_drv_create_surface (VADriverContextP ctx, input_surf_params * params)
{
  INT j = 0;
  VAStatus status = VA_STATUS_SUCCESS;
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  MEDIA_DRV_ASSERT (ctx);
  MEDIA_DRV_ASSERT (drv_ctx);
  INT surfaceID = NEW_SURFACE_ID ();
  struct object_surface *obj_surface = SURFACE (surfaceID);
  if (NULL == obj_surface)
    {
      status = VA_STATUS_ERROR_ALLOCATION_FAILED;
      return status;
    }

  params->surfaces[params->index] = surfaceID;
  obj_surface->status = VASurfaceReady;
  obj_surface->orig_width = params->width;
  obj_surface->orig_height = params->height;

  obj_surface->subpic_render_idx = 0;
  for (j = 0; j < I965_MAX_SUBPIC_SUM; j++)
    {
      obj_surface->subpic[j] = VA_INVALID_ID;
      obj_surface->obj_subpic[j] = NULL;
    }
  if (VA_SURFACE_ATTRIB_USAGE_HINT_ENCODER != params->surface_usage_hint)
    {
      obj_surface->height = ALIGN (params->height, 32);
    }
  else
    {
      obj_surface->height = params->height;
    }
  obj_surface->width = ALIGN (params->width, 16);
  obj_surface->flags = SURFACE_REFERENCED;
  obj_surface->fourcc = 0;
  obj_surface->bo = NULL;
  obj_surface->locked_image_id = VA_INVALID_ID;
  obj_surface->private_data = NULL;
  obj_surface->free_private_data = NULL;
  obj_surface->subsampling = SUBSAMPLE_YUV420;

  switch (params->memory_type)
    {
    case I965_SURFACE_MEM_NATIVE:
      media_surface_native_memory (ctx,
				   obj_surface,
				   params->format, params->expected_fourcc);
      break;

    case I965_SURFACE_MEM_GEM_FLINK:
    case I965_SURFACE_MEM_DRM_PRIME:
      media_suface_external_memory (ctx,
				    obj_surface,
				    params->memory_type,
				    params->memory_attibute, params->index);
      break;
    }
  return status;
}

VOID
media_destroy_surface (struct object_heap * heap, struct object_base * obj)
{
  struct object_surface *obj_surface = (struct object_surface *) obj;
  dri_bo_unreference (obj_surface->bo);
  obj_surface->bo = NULL;

  if (obj_surface->free_private_data != NULL)
    {
      obj_surface->free_private_data (&obj_surface->private_data);
      if(obj_surface->private_data!=NULL)
      {
        media_drv_free_memory(obj_surface->private_data);
      }
      obj_surface->private_data = NULL;
    }
  object_heap_free (heap, obj);
}
