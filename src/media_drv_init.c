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
 * Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 * Xiang Haihao <haihao.xiang@intel.com> 
 */

#include "media_drv_util.h"
#include "media_drv_defines.h"
#include "media_drv_output_dri.h"
#include "media_drv_render.h"
#include "media_drv_surface.h"
#include "media_drv_hw.h"
#include "media_drv_gpe_utils.h"
#include "media_drv_encoder.h"
#include "media_drv_driver.h"
#include "media_drv_init.h"
#include "media_drv_decoder.h"

//#define DEBUG 
#define DEFAULT_BRIGHTNESS      0
#define DEFAULT_CONTRAST        10
#define DEFAULT_HUE             0
#define DEFAULT_SATURATION      10

MEDIA_DRV_MUTEX mutex_global = PTHREAD_MUTEX_INITIALIZER;
VAProfile profile_table[MEDIA_GEN_MAX_PROFILES] = {
  VAProfileVP8Version0_3,
  VAProfileNone
};

config_attr_list config_attributes_list[MEDIA_GEN_MAX_CONFIG_ATTRIBUTES] = {
  {VAProfileVP8Version0_3, (VAEntrypoint) VAEntrypointHybridEncSlice,
   VA_RC_CQP, 1}
  ,
  {VAProfileVP8Version0_3, (VAEntrypoint) VAEntrypointHybridEncSlice,
   VA_RC_CBR, 1}
  ,
  {VAProfileVP8Version0_3, (VAEntrypoint) VAEntrypointHybridEncSlice,
   VA_RC_VBR, 1}
};

/*list of supported display attributes */
static const VADisplayAttribute media_display_attributes[] = {
  {
   VADisplayAttribBrightness,
   -100, 100, DEFAULT_BRIGHTNESS,
   VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE},

  {
   VADisplayAttribContrast,
   0, 100, DEFAULT_CONTRAST,
   VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE},

  {
   VADisplayAttribHue,
   -180, 180, DEFAULT_HUE,
   VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE},

  {
   VADisplayAttribSaturation,
   0, 100, DEFAULT_SATURATION,
   VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE},

  {
   VADisplayAttribRotation,
   0, 3, VA_ROTATION_NONE,
   VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE},
};

INT
format_to_fourcc (UINT format)
{
  INT expected_fourcc;
  switch (format)
    {
    case VA_RT_FORMAT_YUV420:
      expected_fourcc = VA_FOURCC_NV12;
      break;
    case VA_RT_FORMAT_RGB32:
      expected_fourcc = VA_FOURCC_BGRA;
      break;
    case VA_RT_FORMAT_YUV422:
      expected_fourcc = VA_FOURCC_422H;
      break;
    case VA_RT_FORMAT_YUV444:
      expected_fourcc = VA_FOURCC_444P;
      break;
    case VA_RT_FORMAT_YUV400:
      expected_fourcc = VA_FOURCC ('4', '0', '0', 'P');
      break;
    case VA_RT_FORMAT_YUV411:
      expected_fourcc = VA_FOURCC_411P;
      break;
    case VA_FOURCC_NV12:
      expected_fourcc = VA_FOURCC_NV12;
      break;
    case VA_FOURCC_ABGR:
      expected_fourcc = VA_FOURCC_ABGR;
      break;
    case VA_FOURCC_ARGB:
      expected_fourcc = VA_FOURCC_ARGB;
      break;
    case VA_FOURCC_YUY2:
      expected_fourcc = VA_FOURCC_YUY2;
      break;
    case VA_FOURCC_YV12:
      expected_fourcc = VA_FOURCC_YV12;
      break;
    case VA_FOURCC_422H:
      expected_fourcc = VA_FOURCC_422H;
      break;
    case VA_FOURCC_422V:
      expected_fourcc = VA_FOURCC_422V;
      break;
    case VA_FOURCC_P208:
      expected_fourcc = VA_FOURCC_P208;
      break;
    default:
      return ERROR;
    }
  return expected_fourcc;

}

VAStatus
media_QueryVideoProcPipelineCaps (VADriverContextP ctx, VAContextID context, VABufferID * filters, UINT num_filters, VAProcPipelineCaps * pipeline_cap	/* out */
  )
{
  return VA_STATUS_SUCCESS;
}

VAStatus
media_QueryVideoProcFilterCaps (VADriverContextP ctx,
				VAContextID context,
				VAProcFilterType type,
				VOID * filter_caps, UINT * num_filter_caps)
{
  return VA_STATUS_SUCCESS;
}

VAStatus
media_QueryVideoProcFilters (VADriverContextP ctx,
			     VAContextID context,
			     VAProcFilterType * filters, UINT * num_filters)
{
  return VA_STATUS_SUCCESS;
}


static VAStatus
media_validate_config(VADriverContextP ctx, VAProfile profile,
    VAEntrypoint entrypoint);

static VAStatus
media_CreateSurfaces2 (VADriverContextP ctx,
		       UINT format,
		       UINT width,
		       UINT height,
		       VASurfaceID * surfaces,
		       UINT num_surfaces,
		       VASurfaceAttrib * attrib_list, UINT num_attribs)
{
  //VAStatus vaStatus = VA_STATUS_SUCCESS;
  INT i = 0;
  INT expected_fourcc;
  INT memory_type = I965_SURFACE_MEM_NATIVE;	/* native */
  UINT surface_usage_hint = VA_SURFACE_ATTRIB_USAGE_HINT_GENERIC;
  VASurfaceAttribExternalBuffers *memory_attibute = NULL;
  input_surf_params params;
  VAStatus status = VA_STATUS_SUCCESS;
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  MEDIA_DRV_ASSERT (ctx);
  MEDIA_DRV_ASSERT (drv_ctx);

  if (num_surfaces == 0 || surfaces == NULL || width == 0 || height == 0
      || (num_attribs > 0 && attrib_list == NULL))
    {
      printf ("media_CreateSurfaces2:VA_STATUS_ERROR_INVALID_PARAMETER");
      return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
  /*FIXME:Make sure that we supported only the set of format mentioned below */
  if (VA_RT_FORMAT_YUV420 != format && VA_RT_FORMAT_YUV422 != format
      && VA_RT_FORMAT_YUV444 != format && VA_RT_FORMAT_YUV411 != format
      && VA_RT_FORMAT_YUV400 != format && VA_RT_FORMAT_RGB32 != format
      && VA_FOURCC_NV12 != format && VA_FOURCC_P208 != format)
    {
      return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
    }
  expected_fourcc = format_to_fourcc (format);
  for (i = 0; i < num_attribs && attrib_list; i++)
    {

      if (attrib_list[i].flags & VA_SURFACE_ATTRIB_SETTABLE)
	{
	  switch (attrib_list[i].type)
	    {
	    case VASurfaceAttribUsageHint:
	      MEDIA_DRV_ASSERT (attrib_list[i].value.type ==
				VAGenericValueTypeInteger);
	      surface_usage_hint = attrib_list[i].value.value.i;
	      break;
	    case VASurfaceAttribPixelFormat:
	      MEDIA_DRV_ASSERT (attrib_list[i].value.type ==
				VAGenericValueTypeInteger);
	      expected_fourcc = attrib_list[i].value.value.i;
	      break;
	    case VASurfaceAttribMemoryType:
	      MEDIA_DRV_ASSERT (attrib_list[i].value.type ==
				VAGenericValueTypeInteger);
	      if (attrib_list[i].value.value.i ==
		  VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM)
		memory_type = I965_SURFACE_MEM_GEM_FLINK;	/* flinked GEM handle */
	      else if (attrib_list[i].value.value.i ==
		       VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME)
		memory_type = I965_SURFACE_MEM_DRM_PRIME;	/* drm prime fd */
	      break;
	    case VASurfaceAttribExternalBufferDescriptor:
	      MEDIA_DRV_ASSERT (attrib_list[i].value.type ==
				VAGenericValueTypeInteger);
	      MEDIA_DRV_ASSERT (attrib_list[i].value.value.p);
	      memory_attibute =
		(VASurfaceAttribExternalBuffers *) attrib_list[i].value.
		value.p;

	      break;

	    default:
	      printf ("media_CreateSurface2:attrib type not supported\n");
	      break;
	    }
	}
    }
  params.width = width;
  params.height = height;
  params.format = format;
  params.expected_fourcc = expected_fourcc;
  params.memory_type = memory_type;
  params.surfaces = surfaces;
  params.memory_attibute = memory_attibute;
  params.surface_usage_hint = surface_usage_hint;
  for (i = 0; i < num_surfaces; i++)
    {
      params.index = i;
      status = media_drv_create_surface (ctx, &params);
      if (status != VA_STATUS_SUCCESS)
	{
	  break;
	}
    }
  /* Error recovery */
  if (VA_STATUS_SUCCESS != status)
    {
      /* surfaces[i-1] was the last successful allocation */
      for (; i--;)
	{
	  struct object_surface *obj_surface = SURFACE (surfaces[i]);

	  surfaces[i] = VA_INVALID_SURFACE;
	  MEDIA_DRV_ASSERT (obj_surface);
	  media_destroy_surface (&drv_ctx->surface_heap,
				 (struct object_base *) obj_surface);
	}
    }
  return status;
}


VAStatus
media_QuerySurfaceAttributes (VADriverContextP ctx,
			      VAConfigID config,
			      VASurfaceAttrib * attrib_list,
			      UINT * num_attribs)
{
  return VA_STATUS_SUCCESS;
}

static VAStatus
media_GetSurfaceAttributes (VADriverContextP ctx,
			    VAConfigID config,
			    VASurfaceAttrib * attrib_list, UINT num_attribs)
{
  return VA_STATUS_SUCCESS;
}

static VAStatus
media_UnlockSurface (VADriverContextP ctx,	/* in */
		     VASurfaceID surface	/* in */
  )
{

  return VA_STATUS_SUCCESS;
}

static VAStatus
media_LockSurface (VADriverContextP ctx,	/* in */
		   VASurfaceID surface,	/* in */
		   UINT * fourcc,	/* out */
		   UINT * luma_stride,	/* out */
		   UINT * chroma_u_stride,	/* out */
		   UINT * chroma_v_stride,	/* out */
		   UINT * luma_offset,	/* out */
		   UINT * chroma_u_offset,	/* out */
		   UINT * chroma_v_offset,	/* out */
		   UINT * buffer_name,	/* out */
		   VOID ** buffer	/* out */
  )
{

  return VA_STATUS_SUCCESS;
}

static VAStatus
media_BufferInfo (VADriverContextP ctx,	/* in */
		  VABufferID buf_id,	/* in */
		  VABufferType * type,	/* out */
		  UINT * size,	/* out */
		  UINT * num_elements	/* out */
  )
{
  MEDIA_DRV_CONTEXT *drv_ctx = NULL;
  struct object_buffer *obj_buffer = NULL;

  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  obj_buffer = BUFFER (buf_id);

  MEDIA_DRV_ASSERT (obj_buffer);

  if (!obj_buffer)
    return VA_STATUS_ERROR_INVALID_BUFFER;

  *type = obj_buffer->type;
  *size = obj_buffer->size_element;
  *num_elements = obj_buffer->num_elements;

  return VA_STATUS_SUCCESS;
}

VAStatus
media_SetDisplayAttributes (VADriverContextP ctx, VADisplayAttribute * attribs,	/* in */
			    INT num_attribs	/* in */
  )
{
  return VA_STATUS_SUCCESS;

}

VAStatus
media_GetDisplayAttributes (VADriverContextP ctx, VADisplayAttribute * attribs,	/* inout */
			    INT num_attribs	/* in */
  )
{
  return VA_STATUS_SUCCESS;
}

VAStatus
media_QueryDisplayAttributes (VADriverContextP ctx, VADisplayAttribute * attribs,	/* out */
			      INT * num_attribs_ptr	/* out */
  )
{
  return VA_STATUS_SUCCESS;
}

VAStatus
media_DeassociateSubpicture (VADriverContextP ctx,
			     VASubpictureID subpicture,
			     VASurfaceID * target_surfaces, INT num_surfaces)
{
  return VA_STATUS_SUCCESS;

}

VAStatus
media_AssociateSubpicture (VADriverContextP ctx, VASubpictureID subpicture, VASurfaceID * target_surfaces, INT num_surfaces, INT16 src_x,	/* upper left offset in subpicture */
			   INT16 src_y, UINT16 src_width, UINT16 src_height, INT16 dest_x,	/* upper left offset in surface */
			   INT16 dest_y,
			   UINT16 dest_width, UINT16 dest_height,
			   /*
			    * whether to enable chroma-keying or global-alpha
			    * see VA_SUBPICTURE_XXX values
			    */
			   UINT flags)
{

  return VA_STATUS_SUCCESS;

}

VAStatus
media_SetSubpictureGlobalAlpha (VADriverContextP ctx,
				VASubpictureID subpicture, float global_alpha)
{
  return VA_STATUS_SUCCESS;
}

VAStatus
media_SetSubpictureChromakey (VADriverContextP ctx,
			      VASubpictureID subpicture,
			      UINT chromakey_min,
			      UINT chromakey_max, UINT chromakey_mask)
{
  /* TODO */
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
media_SetSubpictureImage (VADriverContextP ctx,
			  VASubpictureID subpicture, VAImageID image)
{
  /* TODO */
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
media_DestroySubpicture (VADriverContextP ctx, VASubpictureID subpicture)
{

  return VA_STATUS_SUCCESS;
}

VAStatus
media_CreateSubpicture (VADriverContextP ctx, VAImageID image, VASubpictureID * subpicture)	/* out */
{

  return VA_STATUS_SUCCESS;
}

VAStatus
media_QuerySubpictureFormats (VADriverContextP ctx, VAImageFormat * format_list,	/* out */
			      UINT * flags,	/* out */
			      UINT * num_formats)	/* out */
{
  return VA_STATUS_SUCCESS;
}

static VAStatus
media_PutImage (VADriverContextP ctx,
		VASurfaceID surface,
		VAImageID image,
		INT src_x,
		INT src_y,
		UINT src_width,
		UINT src_height,
		INT dest_x, INT dest_y, UINT dest_width, UINT dest_height)
{
  return VA_STATUS_SUCCESS;
}

VAStatus
media_GetImage (VADriverContextP ctx, VASurfaceID surface, INT x,	/* coordinates of the upper left source pixel */
		INT y, UINT width,	/* width and height of the region */
		UINT height, VAImageID image)
{

  return VA_STATUS_SUCCESS;
}

VAStatus
media_SetImagePalette (VADriverContextP ctx, VAImageID image, BYTE * palette)
{
  return VA_STATUS_SUCCESS;
}

VAStatus
media_CreateImage (VADriverContextP ctx, VAImageFormat * format, INT width, INT height, VAImage * out_image)	/* out */
{

  return VA_STATUS_SUCCESS;
}

VOID
media_reference_buffer_store (struct buffer_store ** ptr,
			      struct buffer_store * buffer_store)
{
  MEDIA_DRV_ASSERT (*ptr == NULL);

  if (buffer_store)
    {
      buffer_store->ref_count++;
      *ptr = buffer_store;
    }
}

VAStatus
media_create_buffer (MEDIA_DRV_CONTEXT * drv_ctx, VABufferType type,
		     UINT size, UINT num_elements, VOID * data,
		     dri_bo * store_bo, VABufferID * buf_id)
{
  INT bufferID;
  struct object_buffer *obj_buffer = NULL;
  struct buffer_store *buffer_store = NULL;
  bufferID = NEW_BUFFER_ID ();
  obj_buffer = BUFFER (bufferID);
  if (NULL == obj_buffer)
    {
      return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }
  if (type == VAEncCodedBufferType)
    {
      size += I965_CODEDBUFFER_HEADER_SIZE;
      size += 0x1000;		/* for upper bound check */
    }
  obj_buffer->max_num_elements = num_elements;
  obj_buffer->num_elements = num_elements;
  obj_buffer->size_element = size;
  obj_buffer->type = type;
  obj_buffer->buffer_store = NULL;
  buffer_store = media_drv_alloc_memory (sizeof (struct buffer_store));
  MEDIA_DRV_ASSERT (buffer_store);
  buffer_store->ref_count = 1;
  if (store_bo != NULL)
    {
      buffer_store->bo = store_bo;
      dri_bo_reference (buffer_store->bo);
      if (data)
	dri_bo_subdata (buffer_store->bo, 0, size * num_elements, data);
    }
  else if (type == VASliceDataBufferType ||
	   type == VAImageBufferType ||
	   type == VAEncCodedBufferType || type == VAProbabilityBufferType)
    {
      buffer_store->bo = dri_bo_alloc (drv_ctx->drv_data.bufmgr,
				       "Buffer", size * num_elements, 64);
      MEDIA_DRV_ASSERT (buffer_store->bo);

      if (type == VAEncCodedBufferType)
	{
	  struct coded_buffer_segment *coded_buffer_segment;
	  dri_bo_map (buffer_store->bo, 1);
	  coded_buffer_segment =
	    (struct coded_buffer_segment *) buffer_store->bo->virtual;
	  coded_buffer_segment->base.size =
	    size - I965_CODEDBUFFER_HEADER_SIZE;
	  coded_buffer_segment->base.bit_offset = 0;
	  coded_buffer_segment->base.status = 0;
	  coded_buffer_segment->base.buf = NULL;
	  coded_buffer_segment->base.next = NULL;
	  coded_buffer_segment->mapped = 0;
	  /*FIXME:currently only vp8 is supported so seeting codec to CODEC_VP8 */
	  coded_buffer_segment->codec = CODEC_VP8;
	  dri_bo_unmap (buffer_store->bo);
	}
      else if (data)
	{
	  dri_bo_subdata (buffer_store->bo, 0, size * num_elements, data);
	}

    }
  else
    {
      INT msize = size;

      if (type == VAEncPackedHeaderDataBufferType)
	{
	  msize = ALIGN (size, 4);
	}
      buffer_store->buffer = media_drv_alloc_memory (msize * num_elements);
      MEDIA_DRV_ASSERT (buffer_store->buffer);

      if (data)
	media_drv_memcpy (buffer_store->buffer, (msize * num_elements), data,
			  (size * num_elements));
    }
  buffer_store->num_elements = obj_buffer->num_elements;

  media_reference_buffer_store (&obj_buffer->buffer_store, buffer_store);
  media_release_buffer_store (&buffer_store);
  *buf_id = bufferID;
  return VA_STATUS_SUCCESS;
}

static VAStatus
media_create_buffer_internal (VADriverContextP ctx,
			      VAContextID context,
			      VABufferType type,
			      UINT size,
			      UINT num_elements,
			      VOID * data,
			      dri_bo * store_bo, VABufferID * buf_id)
{
  MEDIA_DRV_CONTEXT *drv_ctx;
  MEDIA_DRV_ASSERT (ctx);
  VAStatus status = VA_STATUS_SUCCESS;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  MEDIA_DRV_ASSERT (drv_ctx);
  switch ((INT) type)
    {
    case VAPictureParameterBufferType:
    case VAIQMatrixBufferType:
    case VAQMatrixBufferType:
    case VAEncMacroblockMapBufferType:
    case VABitPlaneBufferType:
    case VASliceGroupMapBufferType:
    case VASliceParameterBufferType:
    case VASliceDataBufferType:
    case VAMacroblockParameterBufferType:
    case VAResidualDataBufferType:
    case VADeblockingParameterBufferType:
    case VAImageBufferType:
    case VAEncCodedBufferType:
    case VAEncSequenceParameterBufferType:
    case VAEncPictureParameterBufferType:
    case VAEncSliceParameterBufferType:
    case VAEncPackedHeaderParameterBufferType:
    case VAEncPackedHeaderDataBufferType:
    case VAEncMiscParameterBufferType:
    case VAProcPipelineParameterBufferType:
    case VAProcFilterParameterBufferType:
    case VAHuffmanTableBufferType:
    case VAProbabilityBufferType:
    case VAEncMiscParameterTypeVP8SegmentMapParams:
    case VAEncMiscParameterTypeVP8HybridFrameUpdate:
      /* Ok */
      break;

    default:
      return VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
    }
  status =
    media_create_buffer (drv_ctx, type, size, num_elements, data, store_bo,
			 buf_id);
  return status;
}

VAStatus
media_CreateBuffer (VADriverContextP ctx, VAContextID context,	/* in */
		    VABufferType type,	/* in */
		    UINT size,	/* in */
		    UINT num_elements,	/* in */
		    VOID * data,	/* in */
		    VABufferID * buf_id)	/* out */
{

  return media_create_buffer_internal (ctx, context, type, size, num_elements,
				       data, NULL, buf_id);


}

VOID
media_destroy_image (struct object_heap * heap, struct object_base * obj)
{
  object_heap_free (heap, obj);
}

VAStatus
media_DestroyBuffer (VADriverContextP ctx, VABufferID buffer_id)
{

  MEDIA_DRV_CONTEXT *drv_ctx;
  struct object_buffer *obj_buffer;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  obj_buffer = BUFFER (buffer_id);

  if (!obj_buffer)
    return VA_STATUS_ERROR_INVALID_BUFFER;


  media_destroy_buffer (&drv_ctx->buffer_heap,
			(struct object_base *) obj_buffer);

  return VA_STATUS_SUCCESS;
}

VAStatus
media_DestroyImage (VADriverContextP ctx, VAImageID image)
{
  MEDIA_DRV_CONTEXT *drv_ctx;
  struct object_image *obj_image;
  struct object_surface *obj_surface;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  obj_image = IMAGE (image);
  if (!obj_image)
    return VA_STATUS_SUCCESS;

  dri_bo_unreference (obj_image->bo);
  obj_image->bo = NULL;

  if (obj_image->image.buf != VA_INVALID_ID)
    {
      media_DestroyBuffer (ctx, obj_image->image.buf);
      obj_image->image.buf = VA_INVALID_ID;
    }

  if (obj_image->palette)
    {
      free (obj_image->palette);
      obj_image->palette = NULL;
    }

  obj_surface = SURFACE (obj_image->derived_surface);

  if (obj_surface)
    {
      obj_surface->flags &= ~SURFACE_DERIVED;
    }

  media_destroy_image (&drv_ctx->image_heap,
		       (struct object_base *) obj_image);
  return VA_STATUS_SUCCESS;
}

VAStatus
media_DeriveImage (VADriverContextP ctx, VASurfaceID surface, VAImage * out_image)	/* out */
{

  MEDIA_DRV_CONTEXT *drv_ctx;
  struct object_image *obj_image;
  struct object_surface *obj_surface;
  VAImageID image_id;
  UINT w_pitch;
  VAStatus va_status = VA_STATUS_ERROR_OPERATION_FAILED;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  out_image->image_id = VA_INVALID_ID;
  obj_surface = SURFACE (surface);
  if (!obj_surface)
    return VA_STATUS_ERROR_INVALID_SURFACE;
  if (!obj_surface->bo)
    {
      UINT is_tiled = 0;
      UINT fourcc = VA_FOURCC ('Y', 'V', '1', '2');
      media_guess_surface_format (ctx, surface, &fourcc, &is_tiled);
      INT sampling = get_sampling_from_fourcc (fourcc);
      media_alloc_surface_bo (ctx, obj_surface, is_tiled, fourcc, sampling);
    }
  MEDIA_DRV_ASSERT (obj_surface->fourcc);
  w_pitch = obj_surface->width;
  image_id = NEW_IMAGE_ID ();

  if (image_id == VA_INVALID_ID)
    return VA_STATUS_ERROR_ALLOCATION_FAILED;

  obj_image = IMAGE (image_id);

  if (!obj_image)
    return VA_STATUS_ERROR_ALLOCATION_FAILED;

  obj_image->bo = NULL;
  obj_image->palette = NULL;
  obj_image->derived_surface = VA_INVALID_ID;

  VAImage *const image = &obj_image->image;

  memset (image, 0, sizeof (*image));
  image->image_id = image_id;
  image->buf = VA_INVALID_ID;
  image->num_palette_entries = 0;
  image->entry_bytes = 0;
  image->width = obj_surface->orig_width;
  image->height = obj_surface->orig_height;
  image->data_size = obj_surface->size;

  image->format.fourcc = obj_surface->fourcc;
  image->format.byte_order = VA_LSB_FIRST;
  image->format.bits_per_pixel = 12;


  switch (image->format.fourcc)
    {
    case VA_FOURCC ('Y', 'V', '1', '2'):
      image->num_planes = 3;
      image->pitches[0] = w_pitch;	/* Y */
      image->offsets[0] = 0;
      image->pitches[1] = obj_surface->cb_cr_pitch;	/* V */
      image->offsets[1] = w_pitch * obj_surface->y_cr_offset;
      image->pitches[2] = obj_surface->cb_cr_pitch;	/* U */
      image->offsets[2] = w_pitch * obj_surface->y_cb_offset;
      break;

    case VA_FOURCC ('N', 'V', '1', '2'):
      image->num_planes = 2;
      image->pitches[0] = w_pitch;	/* Y */
      image->offsets[0] = 0;
      image->pitches[1] = obj_surface->cb_cr_pitch;	/* UV */
      image->offsets[1] = w_pitch * obj_surface->y_cb_offset;
      break;

    case VA_FOURCC ('I', '4', '2', '0'):
      image->num_planes = 3;
      image->pitches[0] = w_pitch;	/* Y */
      image->offsets[0] = 0;
      image->pitches[1] = obj_surface->cb_cr_pitch;	/* U */
      image->offsets[1] = w_pitch * obj_surface->y_cb_offset;
      image->pitches[2] = obj_surface->cb_cr_pitch;	/* V */
      image->offsets[2] = w_pitch * obj_surface->y_cr_offset;
      break;
    case VA_FOURCC ('Y', 'U', 'Y', '2'):
    case VA_FOURCC ('U', 'Y', 'V', 'Y'):
      image->num_planes = 1;
      image->pitches[0] = obj_surface->width;	/* Y, width is aligned already */
      image->offsets[0] = 0;
      break;
    case VA_FOURCC ('R', 'G', 'B', 'A'):
    case VA_FOURCC ('R', 'G', 'B', 'X'):
    case VA_FOURCC ('B', 'G', 'R', 'A'):
    case VA_FOURCC ('B', 'G', 'R', 'X'):
      image->num_planes = 1;
      image->pitches[0] = obj_surface->width;
      break;
    default:
#ifdef DEBUG
      printf ("default\n");
#endif
      image->num_planes = 2;
      image->data_size = w_pitch * obj_surface->height * 3 / 2;
      image->pitches[0] = w_pitch;	/* Y */
      image->offsets[0] = 0;
      image->pitches[1] = w_pitch;	/* U */
      image->offsets[1] = w_pitch * obj_surface->height;
      image->pitches[2] = w_pitch;	/* V */
      image->offsets[2] = image->offsets[1] + 1;
    }
  va_status = media_create_buffer_internal (ctx, 0, VAImageBufferType,
					    obj_surface->size, 1, NULL,
					    obj_surface->bo, &image->buf);
  if (va_status != VA_STATUS_SUCCESS)
    goto error;

  struct object_buffer *obj_buffer = BUFFER (image->buf);

  if (!obj_buffer ||
      !obj_buffer->buffer_store || !obj_buffer->buffer_store->bo)
    return VA_STATUS_ERROR_ALLOCATION_FAILED;

  obj_image->bo = obj_buffer->buffer_store->bo;
  dri_bo_reference (obj_image->bo);

  if (image->num_palette_entries > 0 && image->entry_bytes > 0)
    {
      obj_image->palette =
	malloc (image->num_palette_entries * sizeof (*obj_image->palette));
      if (!obj_image->palette)
	{
	  va_status = VA_STATUS_ERROR_ALLOCATION_FAILED;
	  goto error;
	}
    }

  *out_image = *image;
  obj_surface->flags |= SURFACE_DERIVED;
  obj_image->derived_surface = surface;

  return VA_STATUS_SUCCESS;

error:
  media_DestroyImage (ctx, image_id);
  return va_status;

}

VAStatus
media_QueryImageFormats (VADriverContextP ctx, VAImageFormat * format_list,	/* out */
			 INT * num_formats)	/* out */
{
  return VA_STATUS_SUCCESS;
}

VAStatus
media_PutSurface (VADriverContextP ctx, VASurfaceID surface, VOID * draw,	/* X Drawable */
		  INT16 srcx, INT16 srcy, UINT16 srcw, UINT16 srch, INT16 destx, INT16 desty, UINT16 destw, UINT16 desth, VARectangle * cliprects,	/* client supplied clip list */
		  UINT number_cliprects,	/* number of clip rects in the clip list */
		  UINT flags)	/* de-interlacing flags */
{
  return VA_STATUS_SUCCESS;
}

VAStatus
media_QuerySurfaceStatus (VADriverContextP ctx, VASurfaceID render_target, VASurfaceStatus * status)	/* out */
{
  MEDIA_DRV_CONTEXT *drv_ctx;
  MEDIA_DRV_ASSERT (ctx);
  MEDIA_DRV_ASSERT (ctx->pDriverData);
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  struct object_surface *obj_surface = SURFACE (render_target);
  MEDIA_DRV_ASSERT (obj_surface);
  if (obj_surface->bo)
    {
      if (drm_intel_bo_busy (obj_surface->bo))
	{
	  *status = VASurfaceRendering;
	}
      else
	{
	  *status = VASurfaceReady;
	}
    }
  else
    {
      *status = VASurfaceReady;
    }
  return VA_STATUS_SUCCESS;
}

VAStatus
media_SyncSurface (VADriverContextP ctx, VASurfaceID render_target)
{
  VAStatus status;
  MEDIA_DRV_CONTEXT *drv_ctx;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  status = media_sync_surface (drv_ctx, render_target);
  return status;
}

VAStatus
media_EndPicture (VADriverContextP ctx, VAContextID context)
{
  VAStatus status;
  MEDIA_DRV_CONTEXT *drv_ctx;
  struct object_context *obj_context;
  struct object_config *obj_config;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  obj_context = CONTEXT (context);

  MEDIA_DRV_ASSERT (obj_context);

  if (!obj_context)
    return VA_STATUS_ERROR_INVALID_CONTEXT;

  obj_config = obj_context->obj_config;

  if (obj_context->codec_type == CODEC_ENC)
    {

      MEDIA_DRV_ASSERT (VAEntrypointHybridEncSlice == obj_config->entrypoint);

      if (!(obj_context->codec_state.encode.pic_param ||
	    obj_context->codec_state.encode.pic_param_ext))
	{
	  return VA_STATUS_ERROR_INVALID_PARAMETER;
	}

      if (!(obj_context->codec_state.encode.seq_param ||
	    obj_context->codec_state.encode.seq_param_ext))
	{
	  return VA_STATUS_ERROR_INVALID_PARAMETER;
	}
      status =
	media_encoder_picture (ctx, obj_config->profile,
			       &obj_context->codec_state,
			       obj_context->hw_context);
    }
  else if (obj_context->codec_type == CODEC_DEC) {
    if (obj_context->codec_state.decode.pic_param == NULL) {
      return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
    if (obj_context->codec_state.decode.num_slice_params <=0) {
      return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
    if (obj_context->codec_state.decode.num_slice_datas <=0) {
      return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (obj_context->codec_state.decode.num_slice_params !=
            obj_context->codec_state.decode.num_slice_datas) {
      return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (obj_context->hw_context && obj_context->hw_context->run)
      return obj_context->hw_context->run(ctx, obj_config->profile,
                                        &obj_context->codec_state,
                                        obj_context->hw_context);
    else
      return VA_STATUS_ERROR_UNIMPLEMENTED;

  } else
    {
      /*VPP is not supported currently */
      status = VA_STATUS_ERROR_INVALID_PARAMETER;
    }

  return status;
}

#define MEDIA_RENDER_BUFFER(category, name) media_render_##category##_##name##_buffer(ctx, obj_context, obj_buffer)

#define DEF_RENDER_SINGLE_BUFFER_FUNC(category, name, member)           \
    static VAStatus                                                     \
    media_render_##category##_##name##_buffer(VADriverContextP ctx,      \
                                             struct object_context *obj_context, \
                                             struct object_buffer *obj_buffer) \
    {                                                                   \
        struct category##_state *category = &obj_context->codec_state.category; \
        media_release_buffer_store(&category->member);                   \
        media_reference_buffer_store(&category->member, obj_buffer->buffer_store); \
        return VA_STATUS_SUCCESS;                                       \
}
#define DEF_RENDER_MULTI_BUFFER_FUNC(category, name, member)            \
    static VAStatus                                                     \
    media_render_##category##_##name##_buffer(VADriverContextP ctx,      \
                                             struct object_context *obj_context, \
                                             struct object_buffer *obj_buffer) \
    {                                                                   \
        struct category##_state *category = &obj_context->codec_state.category; \
        if (category->num_##member == category->max_##member) {         \
            category->member = realloc(category->member, (category->max_##member + NUM_SLICES) * sizeof(*category->member)); \
            memset(category->member + category->max_##member, 0, NUM_SLICES * sizeof(*category->member)); \
            category->max_##member += NUM_SLICES;                       \
        }                                                               \
        media_release_buffer_store(&category->member[category->num_##member]); \
        media_reference_buffer_store(&category->member[category->num_##member], obj_buffer->buffer_store); \
        category->num_##member++;                                       \
        return VA_STATUS_SUCCESS;                                       \
   }
#define MEDIA_RENDER_ENCODE_BUFFER(name) MEDIA_RENDER_BUFFER(encode, name)

#define MEDIA_DEF_RENDER_ENCODE_SINGLE_BUFFER_FUNC(name, member) DEF_RENDER_SINGLE_BUFFER_FUNC(encode, name, member)
// MEDIA_DEF_RENDER_ENCODE_SINGLE_BUFFER_FUNC(sequence_parameter, seq_param)    
// MEDIA_DEF_RENDER_ENCODE_SINGLE_BUFFER_FUNC(picture_parameter, pic_param)
// MEDIA_DEF_RENDER_ENCODE_SINGLE_BUFFER_FUNC(picture_control, pic_control)
MEDIA_DEF_RENDER_ENCODE_SINGLE_BUFFER_FUNC (qmatrix, q_matrix)
MEDIA_DEF_RENDER_ENCODE_SINGLE_BUFFER_FUNC (iqmatrix, iq_matrix)
MEDIA_DEF_RENDER_ENCODE_SINGLE_BUFFER_FUNC (frame_update, frame_update_param)

/* extended buffer */
  MEDIA_DEF_RENDER_ENCODE_SINGLE_BUFFER_FUNC (sequence_parameter_ext,
					    seq_param_ext)
MEDIA_DEF_RENDER_ENCODE_SINGLE_BUFFER_FUNC (picture_parameter_ext,
					    pic_param_ext)
#define MEDIA_DEF_RENDER_ENCODE_MULTI_BUFFER_FUNC(name, member) DEF_RENDER_MULTI_BUFFER_FUNC(encode, name, member)
// MEDIA_DEF_RENDER_ENCODE_MULTI_BUFFER_FUNC(slice_parameter, slice_params)
  MEDIA_DEF_RENDER_ENCODE_MULTI_BUFFER_FUNC (slice_parameter_ext,
					   slice_params_ext)


#define MEDIA_RENDER_DECODE_BUFFER(name) MEDIA_RENDER_BUFFER(decode, name)
#define MEDIA_DEF_RENDER_DECODE_SINGLE_BUFFER_FUNC(name, member) DEF_RENDER_SINGLE_BUFFER_FUNC(decode, name, member)
MEDIA_DEF_RENDER_DECODE_SINGLE_BUFFER_FUNC(picture_parameter, pic_param)
MEDIA_DEF_RENDER_DECODE_SINGLE_BUFFER_FUNC(iq_matrix, iq_matrix)
MEDIA_DEF_RENDER_DECODE_SINGLE_BUFFER_FUNC(bit_plane, bit_plane)
MEDIA_DEF_RENDER_DECODE_SINGLE_BUFFER_FUNC(huffman_table, huffman_table)
MEDIA_DEF_RENDER_DECODE_SINGLE_BUFFER_FUNC(probability_data, probability_data)

#define MEDIA_DEF_RENDER_DECODE_MULTI_BUFFER_FUNC(name, member) DEF_RENDER_MULTI_BUFFER_FUNC(decode, name, member)
MEDIA_DEF_RENDER_DECODE_MULTI_BUFFER_FUNC(slice_parameter, slice_params)
MEDIA_DEF_RENDER_DECODE_MULTI_BUFFER_FUNC(slice_data, slice_datas)


     static VAStatus
       media_encoder_render_packed_header_data_buffer (VADriverContextP ctx,
						       struct object_context
						       *obj_context,
						       struct object_buffer
						       *obj_buffer,
						       INT type_index)
{
  struct encode_state *encode = &obj_context->codec_state.encode;

  MEDIA_DRV_ASSERT (obj_buffer->buffer_store->bo == NULL);
  MEDIA_DRV_ASSERT (obj_buffer->buffer_store->buffer);
  media_release_buffer_store (&encode->packed_header_data[type_index]);
  media_reference_buffer_store (&encode->packed_header_data[type_index],
				obj_buffer->buffer_store);

  return VA_STATUS_SUCCESS;
}

static VAStatus
media_encoder_render_misc_parameter_buffer (VADriverContextP ctx,
					    struct object_context
					    *obj_context,
					    struct object_buffer *obj_buffer)
{
  struct encode_state *encode = &obj_context->codec_state.encode;
  VAEncMiscParameterBuffer *param = NULL;
  int index;

  MEDIA_DRV_ASSERT (obj_buffer->buffer_store->bo == NULL);
  MEDIA_DRV_ASSERT (obj_buffer->buffer_store->buffer);

  param = (VAEncMiscParameterBuffer *) obj_buffer->buffer_store->buffer;
  index = media_drv_va_misc_type_to_index(param->type);

  if (index == -1)
    return VA_STATUS_ERROR_UNIMPLEMENTED;

  media_release_buffer_store (&encode->misc_param[index]);
  media_reference_buffer_store (&encode->misc_param[index],
				obj_buffer->buffer_store);

  return VA_STATUS_SUCCESS;
}

static VAStatus
media_encoder_render_packed_header_parameter_buffer (VADriverContextP ctx,
						     struct object_context
						     *obj_context,
						     struct object_buffer
						     *obj_buffer,
						     INT type_index)
{
  struct encode_state *encode = &obj_context->codec_state.encode;

  MEDIA_DRV_ASSERT (obj_buffer->buffer_store->bo == NULL);
  MEDIA_DRV_ASSERT (obj_buffer->buffer_store->buffer);
  media_release_buffer_store (&encode->packed_header_param[type_index]);
  media_reference_buffer_store (&encode->packed_header_param[type_index],
				obj_buffer->buffer_store);

  return VA_STATUS_SUCCESS;
}

INT
media_va_enc_packed_type_to_idx (INT packed_type)
{
  INT idx = 0;

  if (packed_type & VAEncPackedHeaderMiscMask)
    {
      idx = I965_PACKED_MISC_HEADER_BASE;
      packed_type = (~VAEncPackedHeaderMiscMask & packed_type);
      assert (packed_type > 0);
      idx += (packed_type - 1);
    }
  else
    {
      idx = I965_PACKED_HEADER_BASE;

      switch (packed_type)
	{
	case VAEncPackedHeaderSequence:
	  idx = I965_PACKED_HEADER_BASE + 0;
	  break;

	case VAEncPackedHeaderPicture:
	  idx = I965_PACKED_HEADER_BASE + 1;
	  break;

	case VAEncPackedHeaderSlice:
	  idx = I965_PACKED_HEADER_BASE + 2;
	  break;

	default:
	  /* Should not get here */
	  MEDIA_DRV_ASSERT (0);
	  break;
	}
    }

  MEDIA_DRV_ASSERT (idx < 4);
  return idx;
}

static VAStatus
media_encoder_render_picture (VADriverContextP ctx,
			      VAContextID context,
			      VABufferID * buffers, INT num_buffers)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;;
  struct object_context *obj_context = CONTEXT (context);
  VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
  INT i;
  MEDIA_DRV_ASSERT (obj_context);
  if (!obj_context)
    return VA_STATUS_ERROR_INVALID_CONTEXT;
  for (i = 0; i < num_buffers; i++)
    {
      struct object_buffer *obj_buffer = BUFFER (buffers[i]);

      if (!obj_buffer)
	return VA_STATUS_ERROR_INVALID_BUFFER;
      switch ((INT) obj_buffer->type)
	{
	case VAIQMatrixBufferType:
	case VAQMatrixBufferType:
	  vaStatus = MEDIA_RENDER_ENCODE_BUFFER (qmatrix);
	  break;
	case VAEncSequenceParameterBufferType:
	  vaStatus = MEDIA_RENDER_ENCODE_BUFFER (sequence_parameter_ext);
	  break;

	case VAEncPictureParameterBufferType:
	  vaStatus = MEDIA_RENDER_ENCODE_BUFFER (picture_parameter_ext);
	  break;

	case VAEncSliceParameterBufferType:
	  vaStatus = MEDIA_RENDER_ENCODE_BUFFER (slice_parameter_ext);
	  break;

	case VAEncPackedHeaderParameterBufferType:
	  {
	    struct encode_state *encode = &obj_context->codec_state.encode;
	    VAEncPackedHeaderParameterBuffer *param =
	      (VAEncPackedHeaderParameterBuffer *) obj_buffer->
	      buffer_store->buffer;
	    encode->last_packed_header_type = param->type;

	    vaStatus =
	      media_encoder_render_packed_header_parameter_buffer (ctx,
								   obj_context,
								   obj_buffer,
								   media_va_enc_packed_type_to_idx
								   (encode->last_packed_header_type));
	    break;
	  }

	case VAEncPackedHeaderDataBufferType:
	  {
	    struct encode_state *encode = &obj_context->codec_state.encode;

	    MEDIA_DRV_ASSERT (encode->last_packed_header_type ==
			      VAEncPackedHeaderSequence
			      || encode->last_packed_header_type ==
			      VAEncPackedHeaderPicture
			      || encode->last_packed_header_type ==
			      VAEncPackedHeaderSlice
			      ||
			      (((encode->last_packed_header_type &
				 VAEncPackedHeaderMiscMask) ==
				VAEncPackedHeaderMiscMask)
			       &&
			       ((encode->last_packed_header_type &
				 (~VAEncPackedHeaderMiscMask)) != 0)));
	    vaStatus =
	      media_encoder_render_packed_header_data_buffer (ctx,
							      obj_context,
							      obj_buffer,
							      media_va_enc_packed_type_to_idx
							      (encode->last_packed_header_type));
	    break;
	  }

	case VAEncMiscParameterBufferType:
	  vaStatus = media_encoder_render_misc_parameter_buffer (ctx,
								 obj_context,
								 obj_buffer);
	  break;
	case VAEncMiscParameterTypeVP8SegmentMapParams:
#ifdef DEBUG
	  printf ("VAEncMiscParameterTypeVP8SegmentMapParams:\n");
#endif
	  break;
	case VAEncMiscParameterTypeVP8HybridFrameUpdate:
	  vaStatus = MEDIA_RENDER_ENCODE_BUFFER (frame_update);
#ifdef DEBUG
	  printf ("VAEncHackTypeVP8HybridFrameUpdate\n");
#endif
	  break;

	default:
	  printf
	    ("media_encoder_render_picture error:VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE obj_buffer->type=%d\n",
	     obj_buffer->type);
	  vaStatus = VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
	  break;
	}
    }

  return vaStatus;
}

static VAStatus
media_decoder_render_picture(VADriverContextP ctx,
                            VAContextID context,
                            VABufferID *buffers,
                            int num_buffers)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;;
  struct object_context *obj_context = CONTEXT(context);
  VAStatus vaStatus = VA_STATUS_SUCCESS;
  int i;

  if (!obj_context)
    return VA_STATUS_ERROR_INVALID_CONTEXT;

  for (i = 0; i < num_buffers && vaStatus == VA_STATUS_SUCCESS; i++) {
    struct object_buffer *obj_buffer = BUFFER(buffers[i]);

    if (!obj_buffer)
      return VA_STATUS_ERROR_INVALID_BUFFER;

    switch (obj_buffer->type) {
    case VAPictureParameterBufferType:
      vaStatus = MEDIA_RENDER_DECODE_BUFFER(picture_parameter);
      break;

    case VAIQMatrixBufferType:
      vaStatus = MEDIA_RENDER_DECODE_BUFFER(iq_matrix);
      break;

    case VABitPlaneBufferType:
      vaStatus = MEDIA_RENDER_DECODE_BUFFER(bit_plane);
      break;

    case VASliceParameterBufferType:
      vaStatus = MEDIA_RENDER_DECODE_BUFFER(slice_parameter);
      break;

    case VASliceDataBufferType:
      vaStatus = MEDIA_RENDER_DECODE_BUFFER(slice_data);
      break;

    case VAHuffmanTableBufferType:
      vaStatus = MEDIA_RENDER_DECODE_BUFFER(huffman_table);
      break;

    case VAProbabilityBufferType:
      vaStatus = MEDIA_RENDER_DECODE_BUFFER(probability_data);
      break;

    default:
      vaStatus = VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
      break;
    }
  }

  return vaStatus;
}

VAStatus
media_RenderPicture (VADriverContextP ctx,
		     VAContextID context,
		     VABufferID * buffers, INT num_buffers)
{

  MEDIA_DRV_CONTEXT *drv_ctx;
  struct object_context *obj_context;
  struct object_config *obj_config;
  VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  obj_context = CONTEXT (context);
  MEDIA_DRV_ASSERT (obj_context);

  if (!obj_context)
    return VA_STATUS_ERROR_INVALID_CONTEXT;

  if (num_buffers <= 0)
    return VA_STATUS_ERROR_INVALID_PARAMETER;

  obj_config = obj_context->obj_config;
  MEDIA_DRV_ASSERT (obj_config);

  if (VAEntrypointHybridEncSlice == obj_config->entrypoint)
    {
      vaStatus =
	media_encoder_render_picture (ctx, context, buffers, num_buffers);
    }
  else if (obj_config->entrypoint == VAEntrypointVLD) {
      vaStatus =
        media_decoder_render_picture (ctx, context, buffers, num_buffers);
  } else
    {
      printf ("media_RenderPicture:entrypoint %d not supported\n",
	      obj_config->entrypoint);
      MEDIA_DRV_ASSERT (0);
    }

  return vaStatus;
}

VAStatus
media_BeginPicture (VADriverContextP ctx,
		    VAContextID context, VASurfaceID render_target)
{

  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  struct object_context *obj_context = CONTEXT (context);
  struct object_surface *obj_surface = SURFACE (render_target);
  struct object_config *obj_config;
  VAStatus status;
  INT i;

  MEDIA_DRV_ASSERT (ctx);
  if (!obj_context)
    return VA_STATUS_ERROR_INVALID_CONTEXT;
  if (!obj_surface)
    return VA_STATUS_ERROR_INVALID_SURFACE;
  obj_config = obj_context->obj_config;
  MEDIA_DRV_ASSERT (obj_config);
  switch (obj_config->profile)
    {
    case VAProfileVP8Version0_3:
      status = VA_STATUS_SUCCESS;
      break;

    default:
      status = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
      break;
    }
  if (obj_context->codec_type == CODEC_ENC)
    {
      media_release_buffer_store (&obj_context->codec_state.encode.pic_param);

      for (i = 0; i < obj_context->codec_state.encode.num_slice_params; i++)
	{
	  media_release_buffer_store (&obj_context->codec_state.
				      encode.slice_params[i]);
	}
      obj_context->codec_state.encode.num_slice_params = 0;
      /* ext */
      media_release_buffer_store (&obj_context->codec_state.
				  encode.pic_param_ext);

      for (i = 0;
	   i <
	   ARRAY_ELEMS (obj_context->codec_state.encode.packed_header_param);
	   i++)
	media_release_buffer_store (&obj_context->codec_state.
				    encode.packed_header_param[i]);

      for (i = 0;
	   i <
	   ARRAY_ELEMS (obj_context->codec_state.encode.packed_header_data);
	   i++)
	media_release_buffer_store (&obj_context->codec_state.
				    encode.packed_header_data[i]);

      for (i = 0; i < obj_context->codec_state.encode.num_slice_params_ext;
	   i++)
	media_release_buffer_store (&obj_context->codec_state.
				    encode.slice_params_ext[i]);

      obj_context->codec_state.encode.num_slice_params_ext = 0;
      obj_context->codec_state.encode.current_render_target = render_target;	/*This is input new frame */
      obj_context->codec_state.encode.last_packed_header_type = 0;
    } else if (obj_context->codec_type == CODEC_DEC) {
      obj_context->codec_state.decode.current_render_target = render_target;
      media_release_buffer_store(&obj_context->codec_state.decode.pic_param);
      media_release_buffer_store(&obj_context->codec_state.decode.iq_matrix);
      media_release_buffer_store(&obj_context->codec_state.decode.bit_plane);
      media_release_buffer_store(&obj_context->codec_state.decode.huffman_table);

      for (i = 0; i < obj_context->codec_state.decode.num_slice_params; i++) {
        media_release_buffer_store(&obj_context->codec_state.decode.slice_params[i]);
        media_release_buffer_store(&obj_context->codec_state.decode.slice_datas[i]);
      }

      obj_context->codec_state.decode.num_slice_params = 0;
      obj_context->codec_state.decode.num_slice_datas = 0;
    }
  return status;
}

VAStatus
media_UnmapBuffer (VADriverContextP ctx, VABufferID buf_id)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  struct object_buffer *obj_buffer = BUFFER (buf_id);
  VAStatus status = VA_STATUS_ERROR_UNKNOWN;
  MEDIA_DRV_ASSERT (ctx);

  if ((buf_id & OBJECT_HEAP_OFFSET_MASK) != BUFFER_ID_OFFSET)
    return VA_STATUS_ERROR_INVALID_BUFFER;

  MEDIA_DRV_ASSERT (obj_buffer && obj_buffer->buffer_store);
  MEDIA_DRV_ASSERT (obj_buffer->buffer_store->bo
		    || obj_buffer->buffer_store->buffer);
  MEDIA_DRV_ASSERT (!
		    (obj_buffer->buffer_store->bo
		     && obj_buffer->buffer_store->buffer));

  if (!obj_buffer || !obj_buffer->buffer_store)
    return VA_STATUS_ERROR_INVALID_BUFFER;

  if (NULL != obj_buffer->buffer_store->bo)
    {
      UINT tiling, swizzle;

      dri_bo_get_tiling (obj_buffer->buffer_store->bo, &tiling, &swizzle);

      if (tiling != I915_TILING_NONE)
	drm_intel_gem_bo_unmap_gtt (obj_buffer->buffer_store->bo);
      else
	dri_bo_unmap (obj_buffer->buffer_store->bo);
      status = VA_STATUS_SUCCESS;
    }
  else if (NULL != obj_buffer->buffer_store->buffer)
    {
      /* Do nothing */
      status = VA_STATUS_SUCCESS;
    }
  return status;
}

VAStatus
media_MapBuffer (VADriverContextP ctx, VABufferID buf_id,	/* in */
		 VOID ** pbuf)	/* out */
{
  MEDIA_DRV_CONTEXT *drv_ctx;
  struct object_buffer *obj_buffer;
  VAStatus status = VA_STATUS_ERROR_UNKNOWN;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  MEDIA_DRV_ASSERT (drv_ctx);
  obj_buffer = BUFFER (buf_id);
  MEDIA_DRV_ASSERT (obj_buffer && obj_buffer->buffer_store);
  MEDIA_DRV_ASSERT (obj_buffer->buffer_store->bo
		    || obj_buffer->buffer_store->buffer);
  MEDIA_DRV_ASSERT (!
		    (obj_buffer->buffer_store->bo
		     && obj_buffer->buffer_store->buffer));
  if (!obj_buffer || !obj_buffer->buffer_store)
    return VA_STATUS_ERROR_INVALID_BUFFER;

  if (NULL != obj_buffer->buffer_store->bo)
    {
      UINT tiling, swizzle;
      drm_intel_bo_wait_rendering (obj_buffer->buffer_store->bo);
      dri_bo_get_tiling (obj_buffer->buffer_store->bo, &tiling, &swizzle);

      if (tiling != I915_TILING_NONE)
	drm_intel_gem_bo_map_gtt (obj_buffer->buffer_store->bo);
      else
	dri_bo_map (obj_buffer->buffer_store->bo, 1);

      MEDIA_DRV_ASSERT (obj_buffer->buffer_store->bo->virtual);
      *pbuf = obj_buffer->buffer_store->bo->virtual;
      /*if (obj_buffer->type == VAEncCodedBufferType)
         {

         }
       */
      status = VA_STATUS_SUCCESS;
    }
  else if (NULL != obj_buffer->buffer_store->buffer)
    {
      *pbuf = obj_buffer->buffer_store->buffer;
      status = VA_STATUS_SUCCESS;
    }
  return status;
}

VAStatus
media_BufferSetNumElements (VADriverContextP ctx, VABufferID buf_id,	/* in */
			    UINT num_elements)	/* in */
{
  return VA_STATUS_SUCCESS;
}

VAStatus
hybridQueryBufferAttributes (VADisplay dpy,
			     VAContextID context,
			     VABufferType bufferType,
			     VOID * outputData, UINT * outputDataLen)
{
  //VAStatus                 vaStatus;
  VADriverContextP ctx;
  MEDIA_DRV_CONTEXT *drv_ctx;
  struct object_context *obj_context;
  ctx = (((VADisplayContextP) dpy)->pDriverContext);
  MEDIA_DRV_ASSERT (ctx);

  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  obj_context = CONTEXT (context);
  MEDIA_DRV_ASSERT (obj_context);

  MEDIA_ENCODER_CTX *encoder_context =
    (MEDIA_ENCODER_CTX *) obj_context->hw_context;

  if (bufferType != (VABufferType) VAEncMbDataBufferType)
    {
      return VA_STATUS_ERROR_INVALID_BUFFER;
    }
  media_encode_mb_layout_vp8 (encoder_context, outputData, outputDataLen);

  return VA_STATUS_SUCCESS;
}

VAStatus
media_DestroyContext (VADriverContextP ctx, VAContextID context)
{
  MEDIA_DRV_CONTEXT *drv_ctx;
  struct object_context *obj_context;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  obj_context = CONTEXT (context);
  MEDIA_DRV_ASSERT (obj_context);

  if (drv_ctx->current_context_id == context)
    drv_ctx->current_context_id = VA_INVALID_ID;

  media_destroy_context (&drv_ctx->context_heap,
			 (struct object_base *) obj_context);
  return VA_STATUS_SUCCESS;
}

VAStatus
media_CreateContext (VADriverContextP ctx, VAConfigID config_id, INT picture_width, INT picture_height, INT flag, VASurfaceID * render_targets, INT num_render_targets, VAContextID * context)	/* out */
{
  VAStatus status = VA_STATUS_SUCCESS;
  INT contextID;
  INT i;
  struct object_context *obj_context = NULL;
  struct object_config *obj_config;
  struct media_render_state *render_state;
  MEDIA_DRV_CONTEXT *drv_ctx;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  MEDIA_DRV_ASSERT (drv_ctx);
  render_state = &drv_ctx->render_state;
  obj_config = CONFIG (config_id);
  if (NULL == obj_config)
    {
      printf ("media_CreateContext obj_config==NULL\n");
      status = VA_STATUS_ERROR_INVALID_CONFIG;
      return status;

    }
  if (picture_width > drv_ctx->codec_info->max_width ||
      picture_height > drv_ctx->codec_info->max_height)
    {
      status = VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
      return status;
    }
  /* Validate flag */
  /* Validate picture dimensions */
  contextID = NEW_CONTEXT_ID ();
  obj_context = CONTEXT (contextID);
  if (NULL == obj_context)
    {
      status = VA_STATUS_ERROR_ALLOCATION_FAILED;
      return status;
    }
  render_state->interleaved_uv = 1;

  status = media_validate_config(ctx, obj_config->profile, obj_config->entrypoint);

  if (status != VA_STATUS_SUCCESS)
    return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;

  *context = contextID;
  obj_context->flags = flag;
  obj_context->context_id = contextID;
  obj_context->obj_config = obj_config;
  obj_context->picture_width = picture_width;
  obj_context->picture_height = picture_height;
  obj_context->num_render_targets = num_render_targets;
  obj_context->render_targets =
    (VASurfaceID *) media_drv_alloc_memory (num_render_targets *
					    sizeof (VASurfaceID));

  obj_context->hw_context = NULL;

  for (i = 0; i < num_render_targets; i++)
    {
      if (NULL == SURFACE (render_targets[i]))
	{
	  status = VA_STATUS_ERROR_INVALID_SURFACE;
	  break;
	}
      obj_context->render_targets[i] = render_targets[i];
    }
  if (VA_STATUS_SUCCESS == status)
    {

      if (VAEntrypointHybridEncSlice == obj_config->entrypoint)
	{			/*encode routin only */

	  obj_context->codec_type = CODEC_ENC;
	  memset (&obj_context->codec_state.encode, 0,
		  sizeof (obj_context->codec_state.encode));
	  obj_context->codec_state.encode.current_render_target =
	    VA_INVALID_ID;
	  obj_context->codec_state.encode.max_slice_params = NUM_SLICES;
	  obj_context->codec_state.encode.slice_params =
	    media_drv_alloc_memory (obj_context->codec_state.
				    encode.max_slice_params *
				    sizeof (*obj_context->codec_state.
					    encode.slice_params));

	  obj_context->hw_context =
	    media_enc_context_init (ctx, obj_config, picture_width,
				    picture_height);

        } else if (obj_config->entrypoint == VAEntrypointVLD) {
          obj_context->codec_type = CODEC_DEC;
          memset(&obj_context->codec_state.decode, 0, sizeof(obj_context->codec_state.decode));
          obj_context->codec_state.decode.current_render_target = -1;
          obj_context->codec_state.decode.max_slice_params = NUM_SLICES;
          obj_context->codec_state.decode.max_slice_datas = NUM_SLICES;
          obj_context->codec_state.decode.slice_params = calloc(obj_context->codec_state.decode.max_slice_params,
                                                             sizeof(*obj_context->codec_state.decode.slice_params));
          obj_context->codec_state.decode.slice_datas = calloc(obj_context->codec_state.decode.max_slice_datas,
                                                            sizeof(*obj_context->codec_state.decode.slice_datas));

          obj_context->hw_context = media_dec_hw_context_init(ctx, obj_config);
        }
    }
  /* Error recovery */
  if (VA_STATUS_SUCCESS != status)
    {
      media_destroy_context (&drv_ctx->context_heap,
			     (struct object_base *) obj_context);
    }

  drv_ctx->current_context_id = contextID;
  return status;

}


VAStatus
media_DestroySurfaces (VADriverContextP ctx,
		       VASurfaceID * surface_list, INT num_surfaces)
{
  INT i;
  MEDIA_DRV_CONTEXT *drv_ctx;
  MEDIA_DRV_ASSERT (ctx);
  if (num_surfaces == 0 || surface_list == NULL)
    {
      printf ("media_DestroySurfaces:VA_STATUS_ERROR_INVALID_PARAMETER");
      return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  for (i = num_surfaces; i--;)
    {
      struct object_surface *obj_surface = SURFACE (surface_list[i]);

      MEDIA_DRV_ASSERT (obj_surface);
      media_destroy_surface (&drv_ctx->surface_heap,
			     (struct object_base *) obj_surface);
    }

  return VA_STATUS_SUCCESS;
}


VAStatus
media_CreateSurfaces (VADriverContextP ctx, INT width, INT height, INT format, INT num_surfaces, VASurfaceID * surfaces)	/* out */
{
  VAStatus status = VA_STATUS_SUCCESS;
  MEDIA_DRV_ASSERT (ctx);
  status = media_CreateSurfaces2 (ctx,
				  format,
				  width,
				  height, surfaces, num_surfaces, NULL, 0);
  return status;
}

VAStatus
media_GetConfigAttributes (VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint, VAConfigAttrib * attrib_list,	/* in/out */
			   INT num_attribs)
{
  INT i;
  MEDIA_DRV_CONTEXT *drv_ctx;
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  /* other attributes don't seem to be defined */
  /* What to do if we don't know the attribute? */
  for (i = 0; i < num_attribs; i++)
    {
      switch (attrib_list[i].type)
	{
	case VAConfigAttribRTFormat:
	  attrib_list[i].value = VA_RT_FORMAT_YUV420;
	  break;

	case VAConfigAttribRateControl:
          /*For now supported rate control methods (VA_RC_CBR/VA_RC_CQP/VA_RC_VBR) are set based on platform*/
            attrib_list[i].value = drv_ctx->codec_info->ratecontrol;
          break;
	case VAConfigAttribEncPackedHeaders:
	  attrib_list[i].value =
	    VA_ENC_PACKED_HEADER_SEQUENCE | VA_ENC_PACKED_HEADER_PICTURE |
	    VA_ENC_PACKED_HEADER_MISC;
	  break;

	case VAConfigAttribEncMaxRefFrames:
	  attrib_list[i].value = (1 << 16) | (1 << 0);
	  break;


	default:
	  attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
	  break;
	}
    }


  return VA_STATUS_SUCCESS;

}

VAStatus
media_DestroyConfig (VADriverContextP ctx, VAConfigID config_id)
{
  MEDIA_DRV_CONTEXT *drv_ctx;
  MEDIA_DRV_ASSERT (ctx);
  VAStatus status = VA_STATUS_SUCCESS;
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  struct object_config *obj_config = CONFIG (config_id);
  if (NULL == obj_config)
    {
      return VA_STATUS_ERROR_INVALID_CONFIG;
    }
  media_destroy_config (&drv_ctx->config_heap,
			(struct object_base *) obj_config);
  return status;

}

static VAStatus
media_validate_config(VADriverContextP ctx, VAProfile profile,
    VAEntrypoint entrypoint)
{
  MEDIA_DRV_CONTEXT *drv_ctx;
  VAStatus va_status;
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;

  /* Validate profile & entrypoint */
  switch (profile) {
  case VAProfileVP8Version0_3:
    if ((entrypoint == VAEntrypointEncSlice) &&
        drv_ctx->codec_info->vp8_enc_hybrid_support) {
      va_status = VA_STATUS_SUCCESS;
    } else {
      va_status = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
    }
    break;
  default:
    va_status = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
    break;
  }
  return va_status;
}

static VAConfigAttrib *
media_lookup_config_attribute(struct object_config *obj_config,
    VAConfigAttribType type)
{
  int i;

  for (i = 0; i < obj_config->num_attribs; i++) {
    VAConfigAttrib * const attrib = &obj_config->attrib_list[i];
    if (attrib->type == type)
      return attrib;
  }
  return NULL;
}


static VAStatus
media_append_config_attribute(struct object_config *obj_config,
    const VAConfigAttrib *new_attrib)
{
  VAConfigAttrib *attrib;

  if (obj_config->num_attribs >= MEDIA_GEN_MAX_CONFIG_ATTRIBUTES)
    return VA_STATUS_ERROR_MAX_NUM_EXCEEDED;

  attrib = &obj_config->attrib_list[obj_config->num_attribs++];
  attrib->type = new_attrib->type;
  attrib->value = new_attrib->value;
  return VA_STATUS_SUCCESS;
}


static VAStatus
media_ensure_config_attribute(struct object_config *obj_config,
    const VAConfigAttrib *new_attrib)
{
  VAConfigAttrib *attrib;

  /* Check for existing attributes */
  attrib = media_lookup_config_attribute(obj_config, new_attrib->type);
  if (attrib) {
    /* Update existing attribute */
    attrib->value = new_attrib->value;
    return VA_STATUS_SUCCESS;
  }
  return media_append_config_attribute(obj_config, new_attrib);
}

VAStatus
media_CreateConfig (VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint, VAConfigAttrib * attrib_list, INT num_attribs, VAConfigID * config_id)	/* out */
{

  INT i, j, attr_table_sz;
  UINT rc_method = VA_RC_NONE;
  struct object_config *obj_config;
  INT configID;
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;;
  VAStatus status = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
  MEDIA_DRV_ASSERT (ctx);
  MEDIA_DRV_ASSERT (config_id);
  MEDIA_DRV_ASSERT (drv_ctx);

  status = media_validate_config(ctx, profile, entrypoint);
  if (VA_STATUS_SUCCESS != status) {
    return status;
  }

  attr_table_sz =
    sizeof (config_attributes_list) / sizeof (config_attributes_list[0]);
  for (i = 0; i < attr_table_sz; i++)
    {
      if (config_attributes_list[i].profile == profile &&
	  config_attributes_list[i].entrypoint == entrypoint)
	{
	  for (j = 0; j < num_attribs; j++)
	    {
	      if (VAConfigAttribRateControl == attrib_list[j].type)
		{
		  rc_method = attrib_list[j].value;
		}
	    }
	  if (config_attributes_list[i].ratectrl_method == rc_method)
	    break;
	}
    }

  configID = NEW_CONFIG_ID ();
  obj_config = CONFIG (configID);
  obj_config->profile = profile;
  obj_config->entrypoint = entrypoint;

  obj_config->num_attribs = 0;

  for (i = 0; i < num_attribs; i++) {
    status = media_ensure_config_attribute(obj_config, &attrib_list[i]);
    if (status != VA_STATUS_SUCCESS)
      break;
  }

  if (status == VA_STATUS_SUCCESS) {
    VAConfigAttrib attrib, *attrib_found;
    attrib.type = VAConfigAttribRTFormat;
    attrib.value = VA_RT_FORMAT_YUV420;
    attrib_found = media_lookup_config_attribute(obj_config, attrib.type);
    if (!attrib_found)
      status = media_append_config_attribute(obj_config, &attrib);
    else if (attrib_found->value != attrib.value)
      status = VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
  }

  /* Error recovery */
  if (VA_STATUS_SUCCESS != status) {
    media_destroy_config(&drv_ctx->config_heap, (struct object_base *)obj_config);
  } else {
    *config_id = configID;
    status = VA_STATUS_SUCCESS;
  }
  return status;
}

VAStatus
media_QueryConfigAttributes (VADriverContextP ctx, VAConfigID config_id, VAProfile * profile,	/* out */
			     VAEntrypoint * entrypoint,	/* out */
			     VAConfigAttrib * attrib_list,	/* out */
			     INT * num_attribs)	/* out */
{

  return VA_STATUS_SUCCESS;
}

VAStatus
media_QueryConfigProfiles (VADriverContextP ctx, VAProfile * profile_list,	/* out */
			   INT * num_profiles)	/* out */
{
  BOOL retval = true;
  MEDIA_DRV_ASSERT (profile_list);
  MEDIA_DRV_ASSERT (num_profiles);

  retval =
    media_drv_memcpy (profile_list, sizeof (profile_table), profile_table,
		      sizeof (profile_table));
  MEDIA_DRV_ASSERT (retval == true);
  *num_profiles = sizeof (profile_table) / sizeof (profile_table);
  return VA_STATUS_SUCCESS;
}

VAStatus
media_QueryConfigEntrypoints (VADriverContextP ctx, VAProfile profile, VAEntrypoint * entrypoint_list,	/* out */
			      INT * num_entrypoints)	/* out */
{
  INT index = 0;
  switch (profile)
    {
    case VAProfileVP8Version0_3:
      entrypoint_list[index++] = (VAEntrypoint) VAEntrypointHybridEncSlice;
      break;
    default:
      //printf ("Unsupported profile\n");
      break;
    }
  MEDIA_DRV_ASSERT (index <= MEDIA_GEN_MAX_ENTRYPOINTS);
  *num_entrypoints = index;
  return index > 0 ? VA_STATUS_SUCCESS : VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
}

MEDIA_DRV_CONTEXT *
media_drv_context_alloc ( /*size_t *//*UINT size */ )
{
  MEDIA_DRV_CONTEXT *drv_ctx;
  drv_ctx =
    (MEDIA_DRV_CONTEXT *) media_drv_alloc_memory (sizeof (MEDIA_DRV_CONTEXT));
  return drv_ctx;
}


VADisplayAttribute *
get_display_attribute (VADriverContextP ctx, VADisplayAttribType type)
{
  UINT i;
  MEDIA_DRV_CONTEXT *drv_ctx = NULL;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = ctx->pDriverData;

  if (!drv_ctx->display_attributes)
    return NULL;

  for (i = 0; i < drv_ctx->num_display_attributes; i++)
    {
      if (drv_ctx->display_attributes[i].type == type)
	return &drv_ctx->display_attributes[i];
    }
  return NULL;
}

static VOID
media_display_attributes_terminate (VADriverContextP ctx)
{
  MEDIA_DRV_CONTEXT *drv_ctx = NULL;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = ctx->pDriverData;
  MEDIA_DRV_ASSERT (drv_ctx);
  if (drv_ctx->display_attributes)
    {
      media_drv_free_memory (drv_ctx->display_attributes);
      drv_ctx->display_attributes = NULL;
      drv_ctx->num_display_attributes = 0;
    }
}

static BOOL
media_display_attributes_init (VADriverContextP ctx)
{
  MEDIA_DRV_CONTEXT *drv_ctx = NULL;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = ctx->pDriverData;

  drv_ctx->num_display_attributes = ARRAY_ELEMS (media_display_attributes);
  drv_ctx->display_attributes =
    media_drv_alloc_memory (drv_ctx->num_display_attributes *
			    sizeof (drv_ctx->display_attributes[0]));
  if (!drv_ctx->display_attributes)
    goto error;

  memcpy (drv_ctx->display_attributes,
	  media_display_attributes, sizeof (media_display_attributes));

  drv_ctx->rotation_attrib =
    get_display_attribute (ctx, VADisplayAttribRotation);
  drv_ctx->brightness_attrib =
    get_display_attribute (ctx, VADisplayAttribBrightness);
  drv_ctx->contrast_attrib =
    get_display_attribute (ctx, VADisplayAttribContrast);
  drv_ctx->hue_attrib = get_display_attribute (ctx, VADisplayAttribHue);
  drv_ctx->saturation_attrib =
    get_display_attribute (ctx, VADisplayAttribSaturation);

  if (!drv_ctx->rotation_attrib ||
      !drv_ctx->brightness_attrib ||
      !drv_ctx->contrast_attrib ||
      !drv_ctx->hue_attrib || !drv_ctx->saturation_attrib)
    {
      goto error;
    }
  return true;

error:
  media_display_attributes_terminate (ctx);
  return false;
}

static VAStatus
media_drv_init (VADriverContextP ctx)
{
  MEDIA_DRV_CONTEXT *drv_ctx = NULL;
  MEDIA_DRV_ASSERT (ctx);

  drv_ctx =
    (MEDIA_DRV_CONTEXT *) media_drv_context_alloc ( /*sizeof(MEDIA_DRV_CONTEXT) */ );
  if (!drv_ctx)
    {
      //MEDIA_DRV_ASSERT (drv_ctx);
      ctx->pDriverData = NULL;
      return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

  ctx->pDriverData = (VOID *) drv_ctx;
  if (!media_driver_init (ctx))
    goto dri_init_err;
  if (!media_driver_data_init (ctx))
    goto data_init_err;
  if (!media_display_attributes_init (ctx))
    goto dis_attr_err;
  #if 0
  if (!media_render_init (ctx))
    goto render_init_err;
  #endif
  if (!media_output_dri_init (ctx))
    goto dri_init_error;

  sprintf (drv_ctx->drv_version, "%s %s driver - %d.%d.%d.pre%d",
	   INTEL_STR_DRIVER_VENDOR,
	   INTEL_STR_DRIVER_NAME,
	   INTEL_DRIVER_MAJOR_VERSION,
	   INTEL_DRIVER_MINOR_VERSION,
	   INTEL_DRIVER_MICRO_VERSION, INTEL_DRIVER_PRE_VERSION);
  drv_ctx->current_context_id = VA_INVALID_ID;
  ctx->str_vendor = drv_ctx->drv_version;

  return VA_STATUS_SUCCESS;

dri_init_error:media_output_dri_terminate (ctx);
#if 0
render_init_err:media_render_terminate (ctx);
#endif
dis_attr_err:media_display_attributes_terminate (ctx);
data_init_err:media_driver_data_terminate (ctx);
dri_init_err:media_driver_terminate (ctx);
  if (drv_ctx)
    media_drv_free_memory (drv_ctx);
  ctx->pDriverData = NULL;

  return VA_STATUS_ERROR_UNKNOWN;

}

VAStatus
media_Terminate (VADriverContextP ctx)
{
  MEDIA_DRV_CONTEXT *drv_ctx;
  MEDIA_DRV_ASSERT (ctx);
  drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  MEDIA_DRV_ASSERT (drv_ctx);

  media_output_dri_terminate (ctx);

  media_render_terminate (ctx);

  media_display_attributes_terminate (ctx);

  media_driver_data_terminate (ctx);

  media_driver_terminate (ctx);
  if (drv_ctx)
    media_drv_free_memory (drv_ctx);
  ctx->pDriverData = NULL;
  return VA_STATUS_SUCCESS;
}


VAStatus
va_driver_init (VADriverContextP ctx)
{
  VAStatus ret = VA_STATUS_ERROR_UNKNOWN;
  struct VADriverVTable *const vtable = ctx->vtable;
  ctx->version_major = VA_MAJOR_VERSION;
  ctx->version_minor = VA_MINOR_VERSION;
  ctx->max_profiles = MEDIA_GEN_MAX_PROFILES;
  ctx->max_entrypoints = MEDIA_GEN_MAX_ENTRYPOINTS;
  ctx->max_attributes = MEDIA_GEN_MAX_CONFIG_ATTRIBUTES;
  ctx->max_image_formats = MEDIA_GEN_MAX_IMAGE_FORMATS;
  ctx->max_subpic_formats = MEDIA_GEN_MAX_SUBPIC_FORMATS;
  ctx->max_display_attributes = 1 + ARRAY_ELEMS (media_display_attributes);

  vtable->vaTerminate = media_Terminate;
  vtable->vaQueryConfigEntrypoints = media_QueryConfigEntrypoints;
  vtable->vaQueryConfigProfiles = media_QueryConfigProfiles;
  vtable->vaQueryConfigAttributes = media_QueryConfigAttributes;
  vtable->vaCreateConfig = media_CreateConfig;
  vtable->vaDestroyConfig = media_DestroyConfig;
  vtable->vaGetConfigAttributes = media_GetConfigAttributes;
  vtable->vaCreateSurfaces = media_CreateSurfaces;
  vtable->vaDestroySurfaces = media_DestroySurfaces;
  vtable->vaCreateContext = media_CreateContext;
  vtable->vaDestroyContext = media_DestroyContext;
  vtable->vaCreateBuffer = media_CreateBuffer;
  vtable->vaBufferSetNumElements = media_BufferSetNumElements;
  vtable->vaMapBuffer = media_MapBuffer;
  vtable->vaUnmapBuffer = media_UnmapBuffer;
  vtable->vaDestroyBuffer = media_DestroyBuffer;
  vtable->vaBeginPicture = media_BeginPicture;
  vtable->vaRenderPicture = media_RenderPicture;
  vtable->vaEndPicture = media_EndPicture;
  vtable->vaSyncSurface = media_SyncSurface;
  vtable->vaQuerySurfaceStatus = media_QuerySurfaceStatus;
  vtable->vaPutSurface = media_PutSurface;
  vtable->vaQueryImageFormats = media_QueryImageFormats;
  vtable->vaCreateImage = media_CreateImage;
  vtable->vaDeriveImage = media_DeriveImage;
  vtable->vaDestroyImage = media_DestroyImage;
  vtable->vaSetImagePalette = media_SetImagePalette;
  vtable->vaGetImage = media_GetImage;
  vtable->vaPutImage = media_PutImage;
  vtable->vaQuerySubpictureFormats = media_QuerySubpictureFormats;
  vtable->vaCreateSubpicture = media_CreateSubpicture;
  vtable->vaDestroySubpicture = media_DestroySubpicture;
  vtable->vaSetSubpictureImage = media_SetSubpictureImage;
  vtable->vaSetSubpictureChromakey = media_SetSubpictureChromakey;
  vtable->vaSetSubpictureGlobalAlpha = media_SetSubpictureGlobalAlpha;
  vtable->vaAssociateSubpicture = media_AssociateSubpicture;
  vtable->vaDeassociateSubpicture = media_DeassociateSubpicture;
  vtable->vaQueryDisplayAttributes = media_QueryDisplayAttributes;
  vtable->vaGetDisplayAttributes = media_GetDisplayAttributes;
  vtable->vaSetDisplayAttributes = media_SetDisplayAttributes;
  vtable->vaBufferInfo = media_BufferInfo;
  vtable->vaLockSurface = media_LockSurface;
  vtable->vaUnlockSurface = media_UnlockSurface;
  vtable->vaGetSurfaceAttributes = media_GetSurfaceAttributes;
  vtable->vaQuerySurfaceAttributes = media_QuerySurfaceAttributes;
  vtable->vaCreateSurfaces2 = media_CreateSurfaces2;

  ret = media_drv_init (ctx);
  return ret;
}

VAStatus DLL_EXPORT __vaDriverInit_0_34 (VADriverContextP ctx);
VAStatus
__vaDriverInit_0_34 (VADriverContextP ctx)
{
  VAStatus ret = VA_STATUS_ERROR_UNKNOWN;
  ret = va_driver_init (ctx);
  return ret;
}
