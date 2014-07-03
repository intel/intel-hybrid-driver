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
 *    Xiang Haihao <haihao.xiang@intel.com>
 *    Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 *
 */

#ifndef _MEDIA__DRIVER_RENDER_H
#define _MEDIA__DRIVER_RENDER_H
#include "media_drv_defines.h"

#define NUM_RENDER_KERNEL       3
struct media_render_kernel
{
  CHAR *name;
  INT interface;
  const UINT (*bin)[4];
  INT size;
  dri_bo *bo;
  UINT kernel_offset;
};


struct media_render_state
{
  struct
  {
    dri_bo *vertex_buffer;
  } vb;

  struct
  {
    dri_bo *state;
  } vs;

  struct
  {
    dri_bo *state;
  } sf;

  struct
  {
    INT sampler_count;
    dri_bo *sampler;
    dri_bo *state;
    dri_bo *surface_state_binding_table_bo;
  } wm;

  struct
  {
    dri_bo *state;
    dri_bo *viewport;
    dri_bo *blend;
    dri_bo *depth_stencil;
  } cc;

  struct
  {
    dri_bo *bo;
  } curbe;

  UINT16 interleaved_uv;
  UINT16 inited;
  struct region *draw_region;

  INT pp_flag;			/* 0: disable, 1: enable */

  struct media_render_kernel render_kernels[3];

  INT max_wm_threads;

  struct
  {
    dri_bo *bo;
    INT bo_size;
    UINT end_offset;
  } instruction_state;

  struct
  {
    dri_bo *bo;
  } indirect_state;

  struct
  {
    dri_bo *bo;
    INT bo_size;
    UINT end_offset;
  } dynamic_state;

  UINT curbe_offset;
  INT curbe_size;

  UINT sampler_offset;
  INT sampler_size;

  UINT cc_viewport_offset;
  INT cc_viewport_size;

  UINT cc_state_offset;
  INT cc_state_size;

  UINT blend_state_offset;
  INT blend_state_size;

  UINT sf_clip_offset;
  INT sf_clip_size;

  UINT scissor_offset;
  INT scissor_size;

};
BOOL media_render_init (VADriverContextP ctx);
VOID media_render_terminate (VADriverContextP ctx);
#endif
