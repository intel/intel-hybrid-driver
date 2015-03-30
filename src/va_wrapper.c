/*
 * Copyright ©  2015 Intel Corporation
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
 *    March20, 2015 : support vaGetImage() via i965_drv_video.so by this wrapper
 *      Peng Chen <peng.c.chen@intel.com>
 *
 */

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "sysdeps.h"
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
#include "va_wrapper.h"

#define WR_LOG(frmt, args...)  \
      printf("HyBrid-vaWrapper: "frmt,##args)

#define DRIVER_EXTENSION  "_drv_video.so"

#define SUF_ID_SIZE  256
#define SUF_ID_MASK  (SUF_ID_SIZE - 1)

#define BUF_ID_SIZE  256

struct WrapperContext
{
  VADriverContextP hybrid_ctx;
  VADriverContextP i965_ctx;

  //backup initital hybrid vttable for accessing the original
  //defined media_drv_xxx APIs
  struct VADriverVTable initial_hybrid_vt;

  //backup some parameters of the driver context
  VASurfaceID cursurf_id[SUF_ID_SIZE];
  VASurfaceID presurf_id[SUF_ID_SIZE];

  VABufferID image_id[BUF_ID_SIZE];
  VABufferID image_buf_id[BUF_ID_SIZE];
};

#define WR_CTX(ctx)     ((struct WrapperContext *)(((MEDIA_DRV_CONTEXT *)ctx->pDriverData)->wrapper_ctx))
#define I965_CTX(ctx) (WR_CTX(ctx)->i965_ctx)

static int
find_CurSurface(struct WrapperContext *wr_ctx, VASurfaceID pre_surface, VASurfaceID *cur_surface)
{
  unsigned int index = pre_surface & SUF_ID_MASK;

  *cur_surface = wr_ctx->cursurf_id[index];

  return (wr_ctx->presurf_id[index] != pre_surface ? 0 : 1);
}

static void
add_Surface(struct WrapperContext *wr_ctx, VASurfaceID pre_surface, VASurfaceID cur_surface)
{
  unsigned int index = pre_surface & SUF_ID_MASK;

  wr_ctx->cursurf_id[index] = cur_surface;
  if(cur_surface == VA_INVALID_ID)
    wr_ctx->presurf_id[index] = VA_INVALID_ID;// delete this surface mapping entry
  else
    wr_ctx->presurf_id[index] = pre_surface;
}

static VAStatus
destroy_Surface(VADriverContextP ctx, VASurfaceID surface)
{
  VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;

  vaStatus = ctx->vtable->vaDestroySurfaces(ctx, &surface, 1);

  return vaStatus;
}

static VAStatus
destroy_PreSurface(struct WrapperContext *wr_ctx, VASurfaceID pre_surface, int mask)
{
  VASurfaceID cur_surface = VA_INVALID_ID;
  VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
  int ret = 0;

  ret = find_CurSurface(wr_ctx, pre_surface, &cur_surface);
  if((cur_surface != VA_INVALID_ID) && (mask || ret)) {
    vaStatus = destroy_Surface(wr_ctx->i965_ctx, cur_surface);

    add_Surface(wr_ctx, pre_surface, VA_INVALID_ID);
  }

  return vaStatus;
}

static VASurfaceID
derive_Surface(struct WrapperContext *wr_ctx, VASurfaceID pre_surface)
{
  VASurfaceID cur_surface = VA_INVALID_ID;
  int ret = 0;

  ret = find_CurSurface(wr_ctx, pre_surface, &cur_surface);
  if(ret == 0) {
    VADriverContextP i965_ctx = wr_ctx->i965_ctx;
    VADriverContextP hybrid_ctx = wr_ctx->hybrid_ctx;
    struct VADriverVTable *initial_hybrid_vt = &wr_ctx->initial_hybrid_vt;
    VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
    VAImage  image;

    if(cur_surface != VA_INVALID_ID) {
      i965_ctx->vtable->vaSyncSurface(i965_ctx, cur_surface);
      destroy_PreSurface(wr_ctx, pre_surface, 0);

      cur_surface = VA_INVALID_ID;
    }

    vaStatus = initial_hybrid_vt->vaDeriveImage(hybrid_ctx, pre_surface, &image);
    if(vaStatus == VA_STATUS_SUCCESS) {
      VASurfaceAttrib attrib_list[2] = {};
      VASurfaceAttribExternalBuffers buffer_descriptor;
      VABufferInfo buf_info;

      memset((void *)&buf_info, 0, sizeof(buf_info));
      vaStatus = initial_hybrid_vt->vaAcquireBufferHandle(hybrid_ctx, image.buf, &buf_info);
      if(vaStatus == VA_STATUS_SUCCESS) {
        memset((void *)&buffer_descriptor, 0, sizeof(buffer_descriptor));
        buffer_descriptor.num_buffers = 1;
        buffer_descriptor.num_planes = image.num_planes;
        buffer_descriptor.width = image.width;
        buffer_descriptor.height = image.height;
        buffer_descriptor.pixel_format = image.format.fourcc;
        buffer_descriptor.data_size = image.data_size;
        buffer_descriptor.pitches[0] = image.pitches[0];
        buffer_descriptor.pitches[1] = image.pitches[1];
        buffer_descriptor.pitches[2] = image.pitches[1];
        buffer_descriptor.offsets[0] = image.offsets[0];
        buffer_descriptor.offsets[1] = image.offsets[1];
        buffer_descriptor.offsets[2] = image.offsets[1];
        buffer_descriptor.buffers = (void *)&(buf_info.handle);

        attrib_list[0].type = VASurfaceAttribExternalBufferDescriptor;
        attrib_list[0].flags |= VA_SURFACE_ATTRIB_SETTABLE;
        attrib_list[0].value.value.p = &buffer_descriptor;
        attrib_list[0].value.type = VAGenericValueTypePointer;

        attrib_list[1].type = VASurfaceAttribMemoryType;
        attrib_list[1].flags |= VA_SURFACE_ATTRIB_SETTABLE;
        attrib_list[1].value.value.i = buf_info.mem_type;
        attrib_list[1].value.type = VAGenericValueTypeInteger;

        vaStatus = i965_ctx->vtable->vaCreateSurfaces2(i965_ctx, VA_RT_FORMAT_YUV420, image.width,
          image.height, &cur_surface, 1, &attrib_list[0], 2);
        if(vaStatus == VA_STATUS_SUCCESS) {
          add_Surface(wr_ctx, pre_surface, cur_surface);
        }
        else {
          WR_LOG("create surface fail\n");
        }

        initial_hybrid_vt->vaReleaseBufferHandle(hybrid_ctx, image.buf);
      }

      initial_hybrid_vt->vaDestroyImage(hybrid_ctx, image.image_id);
    }
  }

EXIT:
  return cur_surface;
}

static VAStatus
derive_Buffer(VADriverContextP in_ctx, VABufferID in_buf_id, VADriverContextP out_ctx, VABufferID *out_buf_id)
{
  VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
    VABufferType type;
    unsigned int size;
    unsigned int num_elements;
  VABufferInfo buf_info;

  memset((void *)&buf_info, 0, sizeof(buf_info));
  vaStatus = in_ctx->vtable->vaAcquireBufferHandle(in_ctx, in_buf_id, &buf_info);
  if(vaStatus != VA_STATUS_SUCCESS) {
    WR_LOG("AcquireBuf fail\n");
    goto EXIT;
  }

  vaStatus= in_ctx->vtable->vaBufferInfo(in_ctx, in_buf_id, &type, &size, &num_elements);
  if(vaStatus != VA_STATUS_SUCCESS) {
    WR_LOG("BufferInfo fail\n");
    goto RELEAEBUF;
  }

  vaStatus = vawr_DeriveBuffer(out_ctx, type, size, num_elements, NULL, &buf_info, out_buf_id);
  if(vaStatus != VA_STATUS_SUCCESS) {
    WR_LOG("vawr_DeriveBuffer fail status %x\n", vaStatus);
  }

RELEAEBUF:
  in_ctx->vtable->vaReleaseBufferHandle(in_ctx, in_buf_id);

EXIT:
  return vaStatus;
}

static VAStatus
destroy_Buffer(VADriverContextP ctx, VABufferID outbuf_id)
{
  return vawr_DestroyBuffer(ctx, outbuf_id);
}

static VABufferID
find_CurImageBuffer(struct WrapperContext *wr_ctx, VAImageID image_id)
{
  VABufferID buf_id = VA_INVALID_ID;
  int i = 0;

  for(i = 0;i < BUF_ID_SIZE;i++)
    if(wr_ctx->image_id[i] == image_id)
      break;

  if(i < BUF_ID_SIZE)
    buf_id = wr_ctx->image_buf_id[i];

  return buf_id;
}

static int
add_CurImageBuffer(struct WrapperContext *wr_ctx, VAImageID image_id, VABufferID image_buf_id)
{
  int i = 0;
  int ret = 0;

  for(i = 0;i < BUF_ID_SIZE;i++)
    if((wr_ctx->image_id[i] == VA_INVALID_ID))
      break;

  if(i < BUF_ID_SIZE) {
    wr_ctx->image_id[i] = image_id;
    wr_ctx->image_buf_id[i] = image_buf_id;
    ret = 1;
  }

  return ret;
}

static VAStatus
derive_ImageBuffer(struct WrapperContext *wr_ctx, VAImage *image, VABufferID *buf_id)
{
  VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;

  vaStatus = derive_Buffer(wr_ctx->i965_ctx, image->buf, wr_ctx->hybrid_ctx, buf_id);
  if(vaStatus == VA_STATUS_SUCCESS) {
    add_CurImageBuffer(wr_ctx, image->image_id, *buf_id);
  }

  return vaStatus;
}

static VAStatus
destroy_ImageBuffer(struct WrapperContext *wr_ctx, VAImageID image_id, VABufferID image_buf_id)
{
  VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
  int i = 0;

  vaStatus = destroy_Buffer(wr_ctx->hybrid_ctx, image_buf_id);

  for(i = 0;i < BUF_ID_SIZE;i++)
    if((wr_ctx->image_id[i] == image_id))
      break;

  if(i < BUF_ID_SIZE) {
    wr_ctx->image_id[i] = VA_INVALID_ID;
    wr_ctx->image_buf_id[i] = VA_INVALID_ID;
  }

  return vaStatus;
}

static VAStatus
vawr_QueryImageFormats(VADriverContextP ctx, VAImageFormat *format_list,
  int *num_formats)
{
  VADriverContextP i965_ctx = I965_CTX(ctx);

  return i965_ctx->vtable->vaQueryImageFormats(i965_ctx, format_list, num_formats);
}

static VAStatus
vawr_CreateImage(VADriverContextP ctx, VAImageFormat *format,
  int width, int height, VAImage *image)
{
  struct WrapperContext *wr_ctx = WR_CTX(ctx);
  VADriverContextP i965_ctx = I965_CTX(ctx);
  VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;

  vaStatus = i965_ctx->vtable->vaCreateImage(i965_ctx, format, width, height, image);
  if(vaStatus == VA_STATUS_SUCCESS) {
    VABufferID outbuf_id;

    vaStatus = derive_ImageBuffer(wr_ctx, image, &outbuf_id);
    if(vaStatus == VA_STATUS_SUCCESS)
      image->buf = outbuf_id;
    else {
      WR_LOG("derive buffer for image %d fail\n", image->image_id);

      i965_ctx->vtable->vaDestroyImage(i965_ctx, image->image_id);
    }
  }

  return vaStatus;
}

static VAStatus
vawr_DestroyImage(VADriverContextP ctx,VAImageID image_id)
{
  struct WrapperContext *wr_ctx = WR_CTX(ctx);
  VADriverContextP i965_ctx = I965_CTX(ctx);
  VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
    VABufferID buf_id;

  vaStatus = i965_ctx->vtable->vaDestroyImage(i965_ctx, image_id);
  if(vaStatus == VA_STATUS_SUCCESS) {
    buf_id = find_CurImageBuffer(wr_ctx, image_id);
    if(buf_id != VA_INVALID_ID) {
      destroy_ImageBuffer(wr_ctx, image_id, buf_id);
    }
    else
    {
      vaStatus = VA_STATUS_ERROR_UNKNOWN;

      WR_LOG("find cur_buf_id fail for image_id %d\n", image_id);
    }
  }

  return vaStatus;
}

static VAStatus
vawr_DeriveImage(VADriverContextP ctx, VASurfaceID surface,
  VAImage *image)
{
  struct WrapperContext *wr_ctx = WR_CTX(ctx);
  VADriverContextP i965_ctx = I965_CTX(ctx);
  VASurfaceID cur_surface = VA_INVALID_ID;
  VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;

  cur_surface = derive_Surface(wr_ctx, surface);
  if(cur_surface == VA_INVALID_ID) {
    WR_LOG("%s: transfer surface id %d fail\n", __FUNCTION__, surface);
    return VA_STATUS_ERROR_UNKNOWN;
  }

  vaStatus = i965_ctx->vtable->vaDeriveImage(i965_ctx, cur_surface, image);
  if(vaStatus == VA_STATUS_SUCCESS) {
    VABufferID outbuf_id;

    vaStatus = derive_ImageBuffer(wr_ctx, image, &outbuf_id);
    if(vaStatus == VA_STATUS_SUCCESS)
      image->buf = outbuf_id;
    else {
      WR_LOG("derive buffer for image %d fail\n", image->image_id);

      i965_ctx->vtable->vaDestroyImage(i965_ctx, image->image_id);
    }
  }

  return vaStatus;
}

static VAStatus
vawr_GetImage(VADriverContextP ctx, VASurfaceID surface,
  int x, int y, unsigned int width, unsigned int height,
  VAImageID image_id)
{
  struct WrapperContext *wr_ctx = WR_CTX(ctx);
  VADriverContextP i965_ctx = I965_CTX(ctx);
  VASurfaceID cur_surface = VA_INVALID_ID;

  cur_surface = derive_Surface(wr_ctx, surface);
  if(cur_surface == VA_INVALID_ID) {
    WR_LOG("%s: transfer surface id %d fail\n", __FUNCTION__, surface);
    return VA_STATUS_ERROR_UNKNOWN;
  }

    return i965_ctx->vtable->vaGetImage(i965_ctx, cur_surface, x, y, width, height, image_id);
}

static VAStatus
vawr_PutImage(VADriverContextP ctx, VASurfaceID surface,
  VAImageID image_id, int src_x, int src_y, unsigned int src_width,
  unsigned int src_height, int dest_x, int dest_y, unsigned int dest_width,
  unsigned int dest_height)
{
  struct WrapperContext *wr_ctx = WR_CTX(ctx);
  VADriverContextP i965_ctx = I965_CTX(ctx);
  VASurfaceID cur_surface = VA_INVALID_ID;

  cur_surface = derive_Surface(wr_ctx, surface);
  if(cur_surface == VA_INVALID_ID) {
    WR_LOG("%s: transfer surface id %d fail\n", __FUNCTION__, surface);
    return VA_STATUS_ERROR_UNKNOWN;
  }

  return i965_ctx->vtable->vaPutImage(i965_ctx, cur_surface, image_id, src_x, src_y, src_width
    , src_height, dest_x, dest_y, dest_width, dest_height);
}

static inline int
va_getDriverInitName(char *name, int namelen, int major, int minor)
{
    int ret = snprintf(name, namelen, "__vaDriverInit_%d_%d", major, minor);
    return ret > 0 && ret < namelen;
}

static VADriverContextP
init_i965_contex(struct WrapperContext *wr_ctx)
{
  VADriverContextP ctx = NULL;
  char driver_name[] = "i965";
  char driver_dir_default[] = VA_DRIVERS_PATH;
  char *driver_path = NULL, *search_path = NULL;
  char *driver_dir = NULL;
    char *saveptr;
    void *handle = NULL;

  VADriverInit init_func = NULL;
  char init_func_s[256];
  int i;

  driver_dir = driver_dir_default;
    search_path = getenv("LIBVA_DRIVERS_PATH");
    if (search_path != NULL) {
        search_path = strdup((const char *)search_path);
        driver_dir = strtok_r(search_path, ":", &saveptr);
    }

    while(driver_dir) {
      driver_path = (char *)calloc(strlen(driver_dir) +
                                           strlen(driver_name) +
                                           strlen(DRIVER_EXTENSION) + 2, 1);
      if (!driver_path) {
        WR_LOG("allocate driver patch fail %s\n", driver_path);
        break;
      }

      strncpy(driver_path, driver_dir, strlen(driver_dir) + 1);
      strncat(driver_path, "/", strlen("/"));
      strncat(driver_path, driver_name, strlen(driver_name));
      strncat(driver_path, DRIVER_EXTENSION, strlen(DRIVER_EXTENSION));
      WR_LOG("Trying to open %s\n", driver_path);
      handle = dlopen( driver_path, RTLD_NOW| RTLD_GLOBAL);
      if(handle == NULL) {
        WR_LOG("dlopen %s fail\n", driver_path);
      }
      else {
      static const struct {
          int major;
          int minor;
      } compatible_versions[] = {
          { VA_MAJOR_VERSION, VA_MINOR_VERSION },
          { 0, 34 },
          { 0, 33 },
          { 0, 32 },
          { -1, }
      };

      for (i = 0; compatible_versions[i].major >= 0; i++) {
          if (va_getDriverInitName(init_func_s, sizeof(init_func_s),
                                   compatible_versions[i].major,
                                   compatible_versions[i].minor)) {
              init_func = (VADriverInit)dlsym(handle, init_func_s);
              if (init_func) {
                  WR_LOG("Found init function %s\n", init_func_s);
                  break;
              }
          }
      }

        if (compatible_versions[i].major < 0) {
            WR_LOG("%s has no function %s\n", driver_path, init_func_s);
            dlclose(handle);
        }
        else {
          struct VADriverVTable *vtable = NULL;
          struct VADriverVTableVPP *vtable_vpp = NULL;
          VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;

          vaStatus = VA_STATUS_SUCCESS;
          ctx = calloc(sizeof(*ctx), 1);
          if(ctx == NULL) {
            WR_LOG("allocate i965 ctx fail\n");

            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            goto CHECK;
          }

          vtable = calloc(sizeof(*vtable), 1);
          if(vtable == NULL) {
            WR_LOG("allocate i965 vtable fail\n");

            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            goto CHECK;
          }
          else if(ctx != NULL)
            ctx->vtable = vtable;

          vtable_vpp = calloc(sizeof(*vtable_vpp), 1);
          if(vtable_vpp == NULL) {
            WR_LOG("allocate i965 vtable_vpp fail\n");

            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            goto CHECK;
          }
          else if(ctx != NULL) {
            ctx->vtable_vpp = vtable_vpp;
            vtable_vpp->version = VA_DRIVER_VTABLE_VPP_VERSION;
          }

          //keep the drm_state
          ctx->drm_state = wr_ctx->hybrid_ctx->drm_state;

          if (init_func && VA_STATUS_SUCCESS == vaStatus)
              vaStatus = (*init_func)(ctx);

CHECK:
          if (VA_STATUS_SUCCESS != vaStatus) {
              WR_LOG("%s init failed\n", driver_path);
              dlclose(handle);

              if(ctx != NULL) {
                free(ctx);
                ctx = NULL;
              }

              if(vtable != NULL)
                free(vtable);

              if(vtable_vpp != NULL)
                free(vtable_vpp);
          }
          else
          {
            free(driver_path);
            //init func done, get the right context of the driver
              ctx->handle = handle;
            break;
          }
        }
    }

    free(driver_path);
        driver_dir = strtok_r(NULL, ":", &saveptr);
  }

  return ctx;
}

static void
term_i965_contex(VADriverContextP ctx)
{
  if(ctx->handle) {
    ctx->vtable->vaTerminate(ctx);
    dlclose(ctx->handle);
  }

  free(ctx->vtable);
  free(ctx->vtable_vpp);
  free(ctx);
}

void *
vawrapper_init(VADriverContextP hybrid_ctx)
{
   struct WrapperContext *wr_ctx = NULL;
   struct VADriverVTable *const vtable = hybrid_ctx->vtable;
   int i = 0;

   wr_ctx = calloc(sizeof(struct WrapperContext), 1);
   if(wr_ctx == NULL) {
       WR_LOG("allocate wrapper context fail\n");
      goto FAIL;
   }

   wr_ctx->hybrid_ctx = hybrid_ctx;
   memcpy((void *)&wr_ctx->initial_hybrid_vt, (void *)hybrid_ctx->vtable, sizeof(struct VADriverVTable));

   wr_ctx->i965_ctx = init_i965_contex(wr_ctx);
   if(wr_ctx->i965_ctx == NULL) {
        WR_LOG("init i965 contex fail\n");
        goto FAIL;
   }

   // replace some parameters of hybrid diver with i965 or wrapper code
   hybrid_ctx->max_image_formats = wr_ctx->i965_ctx->max_image_formats;
   vtable->vaQueryImageFormats = vawr_QueryImageFormats;
   vtable->vaCreateImage = vawr_CreateImage;
   vtable->vaDeriveImage = vawr_DeriveImage;
   vtable->vaDestroyImage = vawr_DestroyImage;
   vtable->vaGetImage = vawr_GetImage;
   vtable->vaPutImage = vawr_PutImage;

   for(i = 0;i < SUF_ID_SIZE;i++) {
       wr_ctx->cursurf_id[i] = VA_INVALID_ID;
       wr_ctx->presurf_id[i] = VA_INVALID_ID;
   }

   for(i = 0;i < BUF_ID_SIZE;i++) {
       wr_ctx->image_id[i] = VA_INVALID_ID;
       wr_ctx->image_buf_id[i] = VA_INVALID_ID;
   }

   return (void *)wr_ctx;

FAIL:
  if(wr_ctx != NULL) {
    if(wr_ctx->i965_ctx != NULL)
      term_i965_contex(wr_ctx->i965_ctx);

    free(wr_ctx);
  }

  return (void *)NULL;
}

void
vawrapper_term(void *wrapper_ctx)
{
  struct WrapperContext *wr_ctx = (struct WrapperContext *)wrapper_ctx;
  int i = 0;

  if(wr_ctx != NULL) {
    for(i = 0;i < SUF_ID_SIZE;i++) {
      destroy_PreSurface(wr_ctx, i, 1);
    }

    if(wr_ctx->i965_ctx != NULL)
      term_i965_contex(wr_ctx->i965_ctx);

    free(wr_ctx);
  }
}

void
vawrapper_destroysurface(void *wrapper_ctx, VASurfaceID surface)
{
  struct WrapperContext *wr_ctx = (struct WrapperContext *)wrapper_ctx;

  destroy_PreSurface(wr_ctx, surface, 0);
}
