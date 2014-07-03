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
 *   
 *    Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 *
 */

#ifndef _MEDIA__DRIVER_INIT_H
#define _MEDIA__DRIVER_INIT_H
#include <stddef.h>
#include <stdbool.h>

#include <va/va.h>
#include <va/va_backend.h>
#include <va/va_vpp.h>
#include <va/va_backend_vpp.h>
#include <va/va_drmcommon.h>
#include <drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>
#include "media_drv_batchbuffer.h"
#include "va_private.h"
#include "object_heap.h"
#include "media_drv_render.h"

#define I965_PACKED_HEADER_BASE         0
#define I965_PACKED_MISC_HEADER_BASE    3

#define NEW_CONFIG_ID() object_heap_allocate(&drv_ctx->config_heap);
#define CONFIG(id) ((struct object_config *)object_heap_lookup(&drv_ctx->config_heap, id))

#define NEW_CONTEXT_ID() object_heap_allocate(&drv_ctx->context_heap);
#define CONTEXT(id) ((struct object_context *)object_heap_lookup(&drv_ctx->context_heap, id))


#define NEW_BUFFER_ID() object_heap_allocate(&drv_ctx->buffer_heap);
#define BUFFER(id) ((struct object_buffer *)object_heap_lookup(&drv_ctx->buffer_heap, id))

#define NEW_IMAGE_ID() object_heap_allocate(&drv_ctx->image_heap);
#define IMAGE(id) ((struct object_image *)object_heap_lookup(&drv_ctx->image_heap, id))
struct coded_buffer_segment
{
  VACodedBufferSegment base;
  BYTE mapped;
  BYTE codec;
};

#define I965_CODEDBUFFER_HEADER_SIZE   ALIGN(sizeof(struct coded_buffer_segment), 64)
struct object_buffer
{
  struct object_base base;
  struct buffer_store *buffer_store;
  INT max_num_elements;
  INT num_elements;
  INT size_element;
  VABufferType type;
};
struct buffer_store
{
  BYTE *buffer;
  dri_bo *bo;
  INT ref_count;
  INT num_elements;
};

struct object_image
{
  struct object_base base;
  VAImage image;
  dri_bo *bo;
  UINT *palette;
  VASurfaceID derived_surface;
};
struct encode_state
{
  struct buffer_store *seq_param;
  struct buffer_store *pic_param;
  struct buffer_store *pic_control;
  struct buffer_store *iq_matrix;
  struct buffer_store *q_matrix;
  struct buffer_store **slice_params;
  INT max_slice_params;
  INT num_slice_params;
  /* for ext */
  struct buffer_store *seq_param_ext;
  struct buffer_store *pic_param_ext;
  struct buffer_store *packed_header_param[4];
  struct buffer_store *packed_header_data[4];
  struct buffer_store **slice_params_ext;
  INT max_slice_params_ext;
  INT num_slice_params_ext;
  INT last_packed_header_type;
  struct buffer_store *misc_param[16];
  VASurfaceID current_render_target;
  struct object_surface *input_yuv_object;
  struct object_surface *reconstructed_object;
  struct object_surface *coded_buf_surface;
  struct object_surface *reference_objects[16];	/* Up to 2 reference surfaces are valid for MPEG-2, */
  //vp8
  struct object_surface *ref_last_frame;
  struct object_surface *ref_gf_frame;
  struct object_surface *ref_arf_frame;
  UINT mv_offset;
  BOOL hme_enabled;
  BOOL me_16x_enabled;
  BOOL hme_done;
  BOOL me_16x_done;
};


union codec_state
{
  struct encode_state encode;
};

struct hw_context
{
  VAStatus (*run) (VADriverContextP ctx,
		   VAProfile profile,
		   union codec_state * codec_state,
		   struct hw_context * hw_context);
  VOID (*destroy) (VOID *);
  struct intel_batchbuffer *batch;
};

struct object_context
{
  struct object_base base;
  VAContextID context_id;
  struct object_config *obj_config;
  VASurfaceID *render_targets;	//input->encode, output->decode
  INT num_render_targets;
  INT picture_width;
  INT picture_height;
  INT flags;
  INT codec_type;
  union codec_state codec_state;
  struct hw_context *hw_context;
};

struct object_config
{
  struct object_base base;
  VAProfile profile;
  VAEntrypoint entrypoint;
  VAConfigAttrib attrib_list[MEDIA_GEN_MAX_CONFIG_ATTRIBUTES];
  INT num_attribs;
};
typedef struct _media_drv_context
{
  struct media_driver_data drv_data;
  struct object_heap config_heap;
  struct object_heap context_heap;
  struct object_heap surface_heap;
  struct object_heap buffer_heap;
  struct object_heap subpic_heap;
  struct object_heap image_heap;
  struct hw_codec_info *codec_info;
  INT locked;
  MEDIA_DRV_MUTEX ctxmutex;
  MEDIA_BATCH_BUFFER *batch;
  MEDIA_BATCH_BUFFER *pp_batch;
  MEDIA_DRV_MUTEX render_mutex;
  MEDIA_DRV_MUTEX pp_mutex;
  CHAR drv_version[256];
  //display attributes
  VADisplayAttribute *display_attributes;
  UINT num_display_attributes;
  VADisplayAttribute *rotation_attrib;
  VADisplayAttribute *brightness_attrib;
  VADisplayAttribute *contrast_attrib;
  VADisplayAttribute *hue_attrib;
  VADisplayAttribute *saturation_attrib;
  VAContextID current_context_id;
  struct media_render_state render_state;
  struct va_dri_output *dri_output;
} MEDIA_DRV_CONTEXT;

typedef struct _config_attr_list
{

  VAProfile profile;
  VAEntrypoint entrypoint;
  UINT ratectrl_method;
  UINT dec_slice_mode;
} config_attr_list;

VOID media_destroy_image (struct object_heap *heap, struct object_base *obj);
VAStatus
media_DestroySurfaces (VADriverContextP ctx,
		       VASurfaceID * surface_list, INT num_surfaces);
#ifdef __cplusplus
extern "C"
{
#endif
  __attribute__ ((visibility ("default"))) VAStatus
    hybridQueryBufferAttributes (VADisplay dpy, VAContextID context,
				 VABufferType bufferType, VOID *outputData,
				 UINT *outputDataLen);

#ifdef __cplusplus
}
#endif
#endif
