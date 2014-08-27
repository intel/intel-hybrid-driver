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

#ifndef _MEDIA__DRIVER_CMDBUFFER_H
#define _MEDIA__DRIVER_CMDBUFFER_H
#include "media_drv_defines.h"
#include "media_drv_util.h"
#include "media_drv_data.h"
#include "media_drv_gpe_utils.h"
#define __ADVANCE_BATCH(batch) do {             \
        media_batchbuffer_advance(batch); \
    } while (0)

#define ADVANCE_BATCH(batch)            __ADVANCE_BATCH(batch)
typedef struct _media_command_buffer
{
  struct media_driver_data *drv_data;
  dri_bo *buffer;
  UINT size;
  BYTE *map;
  BYTE *cmd_ptr;
  UINT offset;
  UINT flag;
  INT atomic;
  UINT emit_total;
  BYTE *emit_start;
} MEDIA_BATCH_BUFFER;
typedef struct media_alloc_params
{
  UINT bo_size;
  UINT width;
  UINT height;
  BYTE *buf_name;
  UINT format;
  UINT tiling;
} MEDIA_ALLOC_PARAMS;
VOID media_batchbuffer_advance (MEDIA_BATCH_BUFFER * batch);
VOID media_batchbuffer_emit_dword (MEDIA_BATCH_BUFFER * batch, UINT cmd);
VOID media_batchbuffer_begin (MEDIA_BATCH_BUFFER * batch, INT total);
VOID media_batchbuffer_require_space (MEDIA_BATCH_BUFFER * batch, UINT size);
VOID
media_batchbuffer_emit_reloc (MEDIA_BATCH_BUFFER * batch, dri_bo * bo,
			      UINT read_domains, UINT write_domains,
			      UINT delta);

VOID media_batchbuffer_check_flag (MEDIA_BATCH_BUFFER * batch, INT flag);
#define __OUT_BATCH(batch, d) do {              \
       media_batchbuffer_emit_dword(batch, d); \
    } while (0)

#define OUT_BATCH(batch, d)             __OUT_BATCH(batch, d)

#define __BEGIN_BATCH(batch, n, f) do {                         \
        assert(f == batch->flag);                               \
        media_batchbuffer_check_flag(batch, f);     \
        media_batchbuffer_require_space(batch, (n) * 4);        \
        media_batchbuffer_begin(batch, (n));              \
    } while (0)

#define BEGIN_BATCH(batch, n)           __BEGIN_BATCH(batch, n, I915_EXEC_RENDER)

#define __OUT_RELOC(batch, bo, read_domains, write_domain, delta) do {  \
        assert((delta) >= 0);                                           \
        media_batchbuffer_emit_reloc(batch, bo,                         \
                                     read_domains, write_domain,        \
                                     delta);                            \
    } while (0)
#define OUT_RELOC(batch, bo, read_domains, write_domain, delta) \
    __OUT_RELOC(batch, bo, read_domains, write_domain, delta)

MEDIA_BATCH_BUFFER *media_batchbuffer_new (struct media_driver_data *drv_data,
					   INT flag, INT buffer_size);

BOOL
media_allocate_resource (MEDIA_RESOURCE * res, dri_bufmgr * bufmgr,
			 const BYTE * name, UINT size, UINT align);
VOID *media_map_buffer_obj (dri_bo * bo);
BOOL media_unmap_buffer_obj (dri_bo * bo);
VOID media_batchbuffer_submit (MEDIA_BATCH_BUFFER * batch);
VOID media_batchbuffer_flush (MEDIA_BATCH_BUFFER * batch);
VOID media_batchbuffer_free (MEDIA_BATCH_BUFFER * batch);
#endif
