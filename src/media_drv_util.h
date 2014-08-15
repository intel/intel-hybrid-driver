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
 *    Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 *
 */

#ifndef _MEDIA__DRIVER_UTIL_H
#define _MEDIA__DRIVER_UTIL_H
#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <va/va.h>
#include <va/va_backend.h>
#include "media_drv_defines.h"
#include "media_drv_data.h"

#define MEDIA_DRV_ASSERT(val) assert(val);
#define ALIGN(i, n)    (((i) + (n) - 1) & ~((n) - 1))
#define ALIGN_FLOOR(i,n)     ((i) & (~(n-1)))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
typedef enum _MEDIA_DRV_STATUS_
{
  SUCESSS,
  INVALID_PARAMETER,
  UNKNOW_STATUS
} MEDIA_DRV_STATUS;
BOOL
media_drv_memcpy (VOID * dst_ptr, size_t dst_len, const VOID * src_ptr,
		  size_t src_len);

VOID media_drv_mutex_init (MEDIA_DRV_MUTEX * mutex);
VOID media_drv_mutex_destroy (MEDIA_DRV_MUTEX * mutex);
INT media_get_sampling_from_fourcc (UINT fourcc);
VOID *media_drv_alloc_memory ( /*size_t */ UINT size);
VOID media_drv_free_memory (VOID * ptr);
INT get_sampling_from_fourcc (UINT fourcc);
VOID
media_guess_surface_format (VADriverContextP ctx,
			    VASurfaceID surface,
			    UINT * fourcc, UINT * is_tiled);
VOID media_drv_memset (VOID * dest_ptr, size_t len);
int media_drv_va_misc_type_to_index(VAEncMiscParameterType type);
VAEncMiscParameterType media_drv_index_to_va_misc_type(int index);

#endif
