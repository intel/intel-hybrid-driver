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
 *    Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 *
 */

#include "media_drv_util.h"
#include "media_drv_data.h"
#include "media_drv_driver.h"
#include "media_drv_defines.h"
BOOL
media_drv_memcpy (VOID * dst_ptr, size_t dst_len, const VOID * src_ptr,
		  size_t src_len)
{
  if (((dst_ptr == NULL) || (src_ptr == NULL)) || (dst_len < src_len))
    {
      printf
	("memcpy failed:Destination buffer is NULL or doesn't have enough space\n");
      MEDIA_DRV_ASSERT (0);
      return INVALID_PARAMETER;
    }

  memcpy (dst_ptr, src_ptr, src_len);

  return true;
}

VOID
media_drv_memset (VOID * dest_ptr, size_t len)
{
  if (dest_ptr != NULL)
    {
      memset (dest_ptr, 0, len);
    }
}

VOID *
media_drv_alloc_memory ( /*size_t */ UINT size)
{
  VOID *ptr = NULL;
  ptr = calloc (size, 1);
  if (ptr == NULL)
    {
      printf ("ERROR:media_drv_alloc_memory:MEM ALLOC FAILED FOR size=%d\n",
	      (UINT) size);
    }
  return (ptr);
}

VOID
media_drv_free_memory (VOID * ptr)
{
  if (ptr != NULL)
    {
      free (ptr);
    }
}

VOID
media_drv_mutex_init (MEDIA_DRV_MUTEX * mutex)
{
  pthread_mutex_init (mutex, NULL);
}

VOID
media_drv_mutex_destroy (MEDIA_DRV_MUTEX * mutex)
{
  INT retval = 0;
  retval = pthread_mutex_destroy (mutex);
  if (retval != 0)
    {
      printf ("pthread mutex destroy failed:retval=%d\n", retval);
      /*FIXME:Need to return failed here...? */
    }
}


VOID
media_drv_mutex_lock (MEDIA_DRV_MUTEX * mutex)
{
  INT retval = 0;
  retval = pthread_mutex_lock (mutex);
  if (retval != 0)
    {
      printf ("pthread mutex lock failed:retval=%d\n", retval);
    }

}

VOID
media_drv_mutex_unlock (MEDIA_DRV_MUTEX * mutex)
{
  INT retval = 0;
  retval = pthread_mutex_unlock (mutex);
  if (retval != 0)
    {
      printf ("pthread mutex unlock failed:retval=%d\n", retval);
    }

}

VOID
media_guess_surface_format (VADriverContextP ctx,
			    VASurfaceID surface,
			    UINT * fourcc, UINT * is_tiled)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) ctx->pDriverData;
  struct object_context *obj_context = NULL;
  struct object_config *obj_config = NULL;

  MEDIA_DRV_ASSERT (ctx);
  MEDIA_DRV_ASSERT (drv_ctx);
  *fourcc = VA_FOURCC ('Y', 'V', '1', '2');
  *is_tiled = 0;

  if (drv_ctx->current_context_id == VA_INVALID_ID)
    return;

  obj_context = CONTEXT (drv_ctx->current_context_id);

  if (!obj_context)
    return;

  obj_config = obj_context->obj_config;
  assert (obj_config);

  if (!obj_config)
    return;

  if (IS_HASWELL (drv_ctx->drv_data.device_id)
      || IS_IVYBRIDGE (drv_ctx->drv_data.device_id))
    {
      *fourcc = VA_FOURCC ('N', 'V', '1', '2');
      *is_tiled = 1;
    }

  return;

}

INT
get_sampling_from_fourcc (UINT fourcc)
{
  INT surface_sampling = -1;

  switch (fourcc)
    {
    case VA_FOURCC ('N', 'V', '1', '2'):
    case VA_FOURCC ('Y', 'V', '1', '2'):
    case VA_FOURCC ('I', '4', '2', '0'):
    case VA_FOURCC ('I', 'Y', 'U', 'V'):
    case VA_FOURCC ('I', 'M', 'C', '1'):
    case VA_FOURCC ('I', 'M', 'C', '3'):
      surface_sampling = SUBSAMPLE_YUV420;
      break;
    case VA_FOURCC ('Y', 'U', 'Y', '2'):
    case VA_FOURCC ('U', 'Y', 'V', 'Y'):
    case VA_FOURCC ('4', '2', '2', 'H'):
      surface_sampling = SUBSAMPLE_YUV422H;
      break;
    case VA_FOURCC ('4', '2', '2', 'V'):
      surface_sampling = SUBSAMPLE_YUV422V;
      break;

    case VA_FOURCC ('4', '4', '4', 'P'):
      surface_sampling = SUBSAMPLE_YUV444;
      break;

    case VA_FOURCC ('4', '1', '1', 'P'):
      surface_sampling = SUBSAMPLE_YUV411;
      break;

    case VA_FOURCC ('Y', '8', '0', '0'):
      surface_sampling = SUBSAMPLE_YUV400;
      break;
    case VA_FOURCC ('R', 'G', 'B', 'A'):
    case VA_FOURCC ('R', 'G', 'B', 'X'):
    case VA_FOURCC ('B', 'G', 'R', 'A'):
    case VA_FOURCC ('B', 'G', 'R', 'X'):
      surface_sampling = SUBSAMPLE_RGBX;
      break;
    default:
      /* Never get here */
      assert (0);
      break;

    }

  return surface_sampling;
}

INT
media_get_sampling_from_fourcc (UINT fourcc)
{
  INT surface_sampling = -1;

  switch (fourcc)
    {
    case VA_FOURCC ('N', 'V', '1', '2'):
    case VA_FOURCC ('Y', 'V', '1', '2'):
    case VA_FOURCC ('I', '4', '2', '0'):
    case VA_FOURCC ('I', 'Y', 'U', 'V'):
    case VA_FOURCC ('I', 'M', 'C', '1'):
    case VA_FOURCC ('I', 'M', 'C', '3'):
      surface_sampling = SUBSAMPLE_YUV420;
      break;
    case VA_FOURCC ('Y', 'U', 'Y', '2'):
    case VA_FOURCC ('U', 'Y', 'V', 'Y'):
    case VA_FOURCC ('4', '2', '2', 'H'):
      surface_sampling = SUBSAMPLE_YUV422H;
      break;
    case VA_FOURCC ('4', '2', '2', 'V'):
      surface_sampling = SUBSAMPLE_YUV422V;
      break;

    case VA_FOURCC ('4', '4', '4', 'P'):
      surface_sampling = SUBSAMPLE_YUV444;
      break;

    case VA_FOURCC ('4', '1', '1', 'P'):
      surface_sampling = SUBSAMPLE_YUV411;
      break;

    case VA_FOURCC ('Y', '8', '0', '0'):
      surface_sampling = SUBSAMPLE_YUV400;
      break;
    case VA_FOURCC ('R', 'G', 'B', 'A'):
    case VA_FOURCC ('R', 'G', 'B', 'X'):
    case VA_FOURCC ('B', 'G', 'R', 'A'):
    case VA_FOURCC ('B', 'G', 'R', 'X'):
      surface_sampling = SUBSAMPLE_RGBX;
      break;
    case VA_FOURCC ('P', '2', '0', '8'):
      surface_sampling = SUBSAMPLE_P208;
      break;
    default:
      /* Never get here */
      assert (0);
      break;

    }

  return surface_sampling;
}

VOID
media_drv_dump_buf_to_file (CHAR * filename, UINT * ptr, UINT size)
{
  FILE *fp;

  fp = fopen (filename, "w+");
  fwrite (ptr, size, 1, fp);
  fclose (fp);
}

VOID
media_drv_dump_bo_buf_to_file (CHAR * filename, dri_bo * bo)
{
  FILE *fp;
  dri_bo_map (bo, 1);
  fp = fopen (filename, "w+");
  fwrite (bo->virtual, bo->size, 1, fp);
  fclose (fp);
  dri_bo_unmap (bo);

}

VOID
media_drv_dump_bo_buf_to_file_v2 (dri_bo * bo, UINT frame_num,
				  CHAR * func_name, CHAR * buf_name,
				  UINT phase)
{
  FILE *fp;
  CHAR filename[100];
  sprintf (filename, "%s%s%d_%s_%s%d_%s", "/tmp/otc_dump/", "Frame",
	   frame_num, func_name, "Phase", phase, buf_name);
  dri_bo_map (bo, 1);
  fp = fopen (filename, "w+");
  fwrite (bo->virtual, bo->size, 1, fp);
  fclose (fp);
  dri_bo_unmap (bo);

}
