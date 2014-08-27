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

#ifndef _MEDIA__DRIVER_DATA_H
#define _MEDIA__DRIVER_DATA_H
#include <stdbool.h>
#include <pthread.h>
#include <drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>

#define TRUE 1
#define FALSE 0
#define ERROR -1
#define MAX_COLOR_PLANES                 4	//Maximum color planes supported by media driver, like (A/R/G/B in different planes)
#define BATCH_BUF_SIZE 0x80000
#define MEDIA_HEAP_INCREMENTAL_SIZE      8

enum ME_MODES
{
  ME16x_BEFORE_ME4x = 0,
  ME16x_ONLY = 1,
  ME4x_ONLY = 2,
  ME4x_AFTER_ME16x = 3
};
enum
{
  NORMAL_MODE = 0,
  PERFORMANCE_MODE = 1,
  QUALITY_MODE = 2
};

typedef pthread_mutex_t MEDIA_DRV_MUTEX;
struct media_driver_data
{
  INT fd;
  INT device_id;
  INT revision;
  dri_bufmgr *bufmgr;
  INT dri2_enabled;
  UINT exec2_flag:1;	/* Flag: has execbuffer2? */
  UINT bsd_flag:1;	/* Flag: has bitstream decoder for H.264? */
  UINT blt_flag:1;	/* Flag: has BLT unit? */
  UINT vebox_flag:1;	/* Flag: has VEBOX unit */
};

struct media_interface_descriptor_data
{
  struct
  {
    UINT pad0:6;
    UINT kernel_start_pointer:26;
  } desc0;

  struct
  {
    UINT kernel_start_pointer_high:16;
    UINT pad0:16;
  } desc1;

  struct
  {
    UINT pad0:7;
    UINT software_exception_enable:1;
    UINT pad1:3;
    UINT maskstack_exception_enable:1;
    UINT pad2:1;
    UINT illegal_opcode_exception_enable:1;
    UINT pad3:2;
    UINT floating_point_mode:1;
    UINT thread_priority:1;
    UINT single_program_flow:1;
    UINT denorm_mode:1;
    UINT pad4:12;
  } desc2;

  struct
  {
    UINT pad0:2;
    UINT sampler_count:3;
    UINT sampler_state_pointer:27;
  } desc3;

  struct
  {
    UINT binding_table_entry_count:5;
    UINT binding_table_pointer:11;
    UINT pad0:16;
  } desc4;

  struct
  {
    UINT constant_urb_entry_read_offset:16;
    UINT constant_urb_entry_read_length:16;
  } desc5;

  struct
  {
    UINT num_threads_in_tg:10;
    UINT pad0:5;
    UINT global_barrier_enable:1;
    UINT shared_local_memory_size:5;
    UINT barrier_enable:1;
    UINT rounding_mode:2;
    UINT pad1:8;
  } desc6;

  struct
  {
    UINT cross_thread_constant_data_read_length:8;
    UINT pad0:24;
  } desc7;
};

struct gen6_interface_descriptor_data
{
  struct
  {
    UINT pad0:6;
    UINT kernel_start_pointer:26;
  } desc0;

  struct
  {
    UINT pad0:7;
    UINT software_exception_enable:1;
    UINT pad1:3;
    UINT maskstack_exception_enable:1;
    UINT pad2:1;
    UINT illegal_opcode_exception_enable:1;
    UINT pad3:2;
    UINT floating_point_mode:1;
    UINT thread_priority:1;
    UINT single_program_flow:1;
    UINT pad4:13;
  } desc1;

  struct
  {
    UINT pad0:2;
    UINT sampler_count:3;
    UINT sampler_state_pointer:27;
  } desc2;

  struct
  {
    UINT binding_table_entry_count:5;
    UINT binding_table_pointer:27;
  } desc3;

  struct
  {
    UINT constant_urb_entry_read_offset:16;
    UINT constant_urb_entry_read_length:16;
  } desc4;

  union
  {
    struct
    {
      UINT num_threads:8;
      UINT barrier_return_byte:8;
      UINT shared_local_memory_size:5;
      UINT barrier_enable:1;
      UINT rounding_mode:2;
      UINT barrier_return_grf_offset:8;
    } gen7;

    struct
    {
      UINT barrier_id:4;
      UINT pad0:28;
    } gen6;
  } desc5;

  struct
  {
    UINT cross_thread_constant_data_read_length:8;
    UINT pad0:24;
  } desc6;

  struct
  {
    UINT pad0;
  } desc7;
};
#endif
