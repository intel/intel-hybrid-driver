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
 *     Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 *
 */

#ifndef _MEDIA__DRIVER_UTILS_H
#define _MEDIA__DRIVER_UTILS_H
#include "media_drv_defines.h"
#include "media_drv_util.h"
#include <intel_bufmgr.h>
#include <va/va.h>
#include <va/va_backend.h>
#define MAX_GPE_KERNELS    32
typedef struct media_resource
{
  dri_bo *bo;
  UINT bo_size;
  UINT pitch;
  UINT tiling;
  UINT swizzle;
  UINT width;
  UINT height;
  BYTE *buf;
  UINT surface_array_spacing;	//move this out of this struct later
  UINT cb_cr_pitch;
  UINT x_cb_offset;
  UINT y_cb_offset;
} MEDIA_RESOURCE;

typedef struct _media_kernel
{
  BYTE *name;
  INT interface;
  const UINT *bin;
  UINT size;
  dri_bo *bo;
  UINT kernel_offset;
} MEDIA_KERNEL;
typedef struct _instruction_state
{
  MEDIA_RESOURCE buff_obj;
  UINT end_offset;
} INSTRUCTION_TYPE;

typedef struct _surface_state_binding_table
{
  MEDIA_RESOURCE res;		/* in bytes */
  CHAR *table_name;
} SURFACE_STATE_BINDING_TABLE;


typedef struct _dynamic_state
{
  MEDIA_RESOURCE res;
  UINT end_offset;
} DYNAMIC_STATE;
typedef struct _idrt
{
  MEDIA_RESOURCE buff_obj;
  UINT max_entries;
  UINT entry_size;	/* in bytes */
} IDRT;

typedef struct _curbe
{
  MEDIA_RESOURCE buff_obj;
  UINT length;		/* in bytes */
} CURBE;
typedef struct _vfe_state
{
  UINT gpgpu_mode:1;
  UINT pad0:7;
  UINT max_num_threads:16;
  UINT num_urb_entries:8;
  UINT urb_entry_size:16;
  UINT curbe_allocation_size:16;

  /* vfe_desc5/6/7 is used to determine whether the HW scoreboard is used.
   * If scoreboard is not used, don't touch them
   */
  union
  {
    UINT dword;
    struct
    {
      UINT mask:8;
      UINT pad:22;
      UINT type:1;
      UINT enable:1;
    } scoreboard0;
  } vfe_desc5;

  union
  {
    UINT dword;
    struct
    {
      INT delta_x0:4;
      INT delta_y0:4;
      INT delta_x1:4;
      INT delta_y1:4;
      INT delta_x2:4;
      INT delta_y2:4;
      INT delta_x3:4;
      INT delta_y3:4;
    } scoreboard1;
  } vfe_desc6;

  union
  {
    UINT dword;
    struct
    {
      INT delta_x4:4;
      INT delta_y4:4;
      INT delta_x5:4;
      INT delta_y5:4;
      INT delta_x6:4;
      INT delta_y6:4;
      INT delta_x7:4;
      INT delta_y7:4;
    } scoreboard2;
  } vfe_desc7;
} VFE_STATE;

typedef struct _status_buffer
{
  MEDIA_RESOURCE res;
} STATUS_BUFFER;

typedef struct _media_gpe_context
{
  MEDIA_KERNEL kernels[MAX_GPE_KERNELS];
  UINT num_kernels;
  INSTRUCTION_TYPE instruction_state;
  SURFACE_STATE_BINDING_TABLE surface_state_binding_table;
  DYNAMIC_STATE dynamic_state;
  STATUS_BUFFER status_buffer;
  IDRT idrt;
  CURBE curbe;
  VFE_STATE vfe_state;
  UINT sampler_offset;
  INT sampler_size;
  UINT idrt_offset;
  INT idrt_size;
  UINT curbe_offset;
  INT curbe_size;
} MEDIA_GPE_CTX;
VOID
media_gpe_context_init (VADriverContextP ctx, MEDIA_GPE_CTX * gpe_context);
VOID
media_gpe_load_kernels (VADriverContextP ctx,
			MEDIA_GPE_CTX * gpe_context,
			MEDIA_KERNEL * kernel_list, UINT num_kernels);
VOID media_gpe_context_destroy (MEDIA_GPE_CTX * gpe_context);
#endif
