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

#include "media_drv_driver.h"
#include "media_drv_batchbuffer.h"
#include "media_drv_gpe_utils.h"
#include "media_drv_util.h"
#include "media_drv_hwcmds.h"

//#define DEBUG
VOID
media_batchbuffer_advance (MEDIA_BATCH_BUFFER * batch)
{

  MEDIA_DRV_ASSERT (batch->emit_total ==
		    (batch->cmd_ptr - batch->emit_start));
}

static VOID
media_batchbuffer_reset (MEDIA_BATCH_BUFFER * batch, INT buffer_size)
{
  struct media_driver_data *drv_data = batch->drv_data;
  INT batch_size = buffer_size;

  MEDIA_DRV_ASSERT (batch->flag == I915_EXEC_RENDER ||
		    batch->flag == I915_EXEC_BLT ||
		    batch->flag == I915_EXEC_BSD ||
		    batch->flag == I915_EXEC_VEBOX);

  dri_bo_unreference (batch->buffer);
  batch->buffer = dri_bo_alloc (drv_data->bufmgr,
				"batch buffer", batch_size, 0x1000);
  MEDIA_DRV_ASSERT (batch->buffer);
  dri_bo_map (batch->buffer, 1);
  MEDIA_DRV_ASSERT (batch->buffer->virtual);
  batch->map = batch->buffer->virtual;
  batch->size = batch_size;
  batch->cmd_ptr = batch->map;
  batch->atomic = 0;
}
VOID
media_batchbuffer_submit (MEDIA_BATCH_BUFFER * batch)
{
  UINT used = batch->cmd_ptr - batch->map;
  if (used == 0)
    {
      return;
    }
  if ((used & 4) == 0)
    {
      *(UINT *) batch->cmd_ptr = 0;
      batch->cmd_ptr += 4;
    }

  *(UINT *) batch->cmd_ptr = MI_BATCH_BUFFER_END;
  batch->cmd_ptr += 4;
  dri_bo_unmap (batch->buffer);
  used = batch->cmd_ptr - batch->map;
  drm_intel_bo_mrb_exec (batch->buffer, used, 0, 0, 0, batch->flag);
  media_batchbuffer_free (batch);
}

VOID
media_batchbuffer_flush (MEDIA_BATCH_BUFFER * batch)
{
  UINT used = batch->cmd_ptr - batch->map;
  if (used == 0)
    {
      return;
    }
  if ((used & 4) == 0)
    {
      *(UINT *) batch->cmd_ptr = 0;
      batch->cmd_ptr += 4;
    }

  *(UINT *) batch->cmd_ptr = MI_BATCH_BUFFER_END;
  batch->cmd_ptr += 4;
  dri_bo_unmap (batch->buffer);
  used = batch->cmd_ptr - batch->map;
  drm_intel_bo_mrb_exec (batch->buffer, used, 0, 0, 0, batch->flag);
  media_batchbuffer_reset (batch, batch->size);
}

VOID
media_batchbuffer_begin (MEDIA_BATCH_BUFFER * batch, INT total)
{
  batch->emit_total = total * 4;
  batch->emit_start = batch->cmd_ptr;
}

static UINT
media_batchbuffer_check_space (MEDIA_BATCH_BUFFER * batch)
{
  return (batch->size - BATCH_RESERVED) - (batch->cmd_ptr - batch->map);
}

VOID
media_batchbuffer_check_flag (MEDIA_BATCH_BUFFER * batch, INT flag)
{
  if (flag != I915_EXEC_RENDER &&
      flag != I915_EXEC_BLT &&
      flag != I915_EXEC_BSD && flag != I915_EXEC_VEBOX)
    return;

  if (batch->flag == flag)
    return;

  media_batchbuffer_flush (batch);
  batch->flag = flag;
}

VOID
media_batchbuffer_require_space (MEDIA_BATCH_BUFFER * batch, UINT size)
{
  MEDIA_DRV_ASSERT (size < batch->size - 8);
  if (media_batchbuffer_check_space (batch) < size)
    {
      media_batchbuffer_flush (batch);
    }
}

VOID
media_batchbuffer_emit_dword (MEDIA_BATCH_BUFFER * batch, UINT cmd)
{
  MEDIA_DRV_ASSERT (media_batchbuffer_check_space (batch) >= 4);
  *(UINT *) batch->cmd_ptr = cmd;
  batch->cmd_ptr += 4;
}

VOID
media_batchbuffer_emit_reloc (MEDIA_BATCH_BUFFER * batch, dri_bo * bo,
			      UINT read_domains, UINT write_domains,
			      UINT delta)
{
  assert (batch->cmd_ptr - batch->map < batch->size);
  dri_bo_emit_reloc (batch->buffer,/* I915_GEM_DOMAIN_RENDER*/ read_domains, write_domains,delta, batch->cmd_ptr - batch->map, bo);
  media_batchbuffer_emit_dword (batch, bo->offset + delta);
}

MEDIA_BATCH_BUFFER *
media_batchbuffer_new (struct media_driver_data * drv_data, INT flag,
		       INT buffer_size)
{
  MEDIA_BATCH_BUFFER *batch = media_drv_alloc_memory (sizeof (*batch));
  MEDIA_DRV_ASSERT (flag == I915_EXEC_RENDER);

  if (!buffer_size || buffer_size < BATCH_SIZE)
    {
      buffer_size = BATCH_SIZE;
    }
  /* the buffer size can't exceed 4M */
  if (buffer_size > MAX_BATCH_SIZE)
    {
      buffer_size = MAX_BATCH_SIZE;
    }
  batch->drv_data = drv_data;
  batch->flag = flag;

  media_batchbuffer_reset (batch, buffer_size);

  return batch;
}

VOID
media_batchbuffer_free (MEDIA_BATCH_BUFFER * batch)
{
  if (batch->map)
    {
      dri_bo_unmap (batch->buffer);
      batch->map = NULL;
    }

  dri_bo_unreference (batch->buffer);
  media_drv_free_memory (batch);
}

BOOL
media_allocate_resource (MEDIA_RESOURCE * res, dri_bufmgr * bufmgr,
			 const BYTE * name, UINT size, UINT align)
{
  res->bo = dri_bo_alloc (bufmgr,(const CHAR *) name, size, align);
  res->bo_size = size;
  return SUCCESS;
}

BOOL
media_allocate_resource_ext (MEDIA_RESOURCE * res, dri_bufmgr * bufmgr,
			     MEDIA_ALLOC_PARAMS * params, UINT align)
{
  res->bo = dri_bo_alloc (bufmgr,(const CHAR *) params->buf_name, params->bo_size, align);
  res->bo_size = params->bo_size;
  res->width = params->width;
  res->height = params->height;
  res->tiling = params->tiling;

  return SUCCESS;
}

VOID *
media_map_buffer_obj (dri_bo * bo)
{
  dri_bo_map (bo, 1);
  MEDIA_DRV_ASSERT (bo->virtual);
  return bo->virtual;
}

BOOL
media_unmap_buffer_obj (dri_bo * bo)
{
  dri_bo_unmap (bo);
  return SUCCESS;
}

void
media_batchbuffer_emit_mi_flush (MEDIA_BATCH_BUFFER * batch)
{
  struct media_driver_data *drv_data = batch->drv_data;

  if (IS_GEN75(drv_data->device_id) ||
      IS_GEN8(drv_data->device_id)) {
      if (batch->flag == I915_EXEC_RENDER) {
          if (IS_GEN8(drv_data->device_id)) {
              BEGIN_BATCH(batch, 6);
              OUT_BATCH(batch, CMD_PIPE_CONTROL | (6 - 2));

              OUT_BATCH(batch,
                        CMD_PIPE_CONTROL_CS_STALL |
                        CMD_PIPE_CONTROL_WC_FLUSH |
                        CMD_PIPE_CONTROL_TC_FLUSH |
                        CMD_PIPE_CONTROL_DC_FLUSH |
                        CMD_PIPE_CONTROL_NOWRITE);
              OUT_BATCH(batch, 0); /* write address */
              OUT_BATCH(batch, 0);
              OUT_BATCH(batch, 0); /* write data */
              OUT_BATCH(batch, 0);
              ADVANCE_BATCH(batch);
          } else {
              BEGIN_BATCH(batch, 4);
              OUT_BATCH(batch, CMD_PIPE_CONTROL | (4 - 2));

              OUT_BATCH(batch,
                        CMD_PIPE_CONTROL_WC_FLUSH |
                        CMD_PIPE_CONTROL_TC_FLUSH |
                        CMD_PIPE_CONTROL_DC_FLUSH |
                        CMD_PIPE_CONTROL_NOWRITE);
              OUT_BATCH(batch, 0); /* write address */
              OUT_BATCH(batch, 0); /* write data */
              ADVANCE_BATCH(batch);
          }

      } else {
          if (batch->flag == I915_EXEC_BLT) {
              __BEGIN_BATCH(batch, 4, I915_EXEC_BLT);
              OUT_BATCH(batch, MI_FLUSH_DW);
              OUT_BATCH(batch, 0);
              OUT_BATCH(batch, 0);
              OUT_BATCH(batch, 0);
              ADVANCE_BATCH(batch);
          } else if (batch->flag == I915_EXEC_BSD) {
              __BEGIN_BATCH(batch, 4, I915_EXEC_BSD);
              OUT_BATCH(batch, MI_FLUSH_DW | MI_FLUSH_DW_VIDEO_PIPELINE_CACHE_INVALIDATE);
              OUT_BATCH(batch, 0);
              OUT_BATCH(batch, 0);
              OUT_BATCH(batch, 0);
              ADVANCE_BATCH(batch);
          }
      }
  }
}

static void
media_batchbuffer_start_atomic_helper(MEDIA_BATCH_BUFFER *batch,
                                      int flag,
                                      unsigned int size)
{
    media_batchbuffer_check_flag(batch, flag);
    media_batchbuffer_require_space(batch, size);
    batch->atomic = 1;
}

void
media_batchbuffer_start_atomic(MEDIA_BATCH_BUFFER *batch, unsigned int size)
{
    media_batchbuffer_start_atomic_helper(batch, I915_EXEC_RENDER, size);
}

void
media_batchbuffer_start_atomic_blt(MEDIA_BATCH_BUFFER *batch, unsigned int size)
{
    media_batchbuffer_start_atomic_helper(batch, I915_EXEC_BLT, size);
}

void
media_batchbuffer_start_atomic_bcs(MEDIA_BATCH_BUFFER *batch, unsigned int size)
{
    media_batchbuffer_start_atomic_helper(batch, I915_EXEC_BSD, size);
}

void
media_batchbuffer_start_atomic_veb(MEDIA_BATCH_BUFFER *batch, unsigned int size)
{
    media_batchbuffer_start_atomic_helper(batch, I915_EXEC_VEBOX, size);
}

void
media_batchbuffer_end_atomic(MEDIA_BATCH_BUFFER *batch)
{
    batch->atomic = 0;
}
