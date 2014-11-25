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

#ifndef _MEDIA__DRIVER_HW_G8_H
#define _MEDIA__DRIVER_HW_G8_H
#include "media_drv_hw.h"
#include "media_drv_hw_g75.h"
#define NUM_OF_VP8_KERNELS_G8 12

#define BINDING_TABLE_OFFSET_G8(index)  (/*ALIGN(sizeof(BINDING_TABLE_STATE),32)*/(sizeof(BINDING_TABLE_STATE)) * index)
#define SURFACE_STATE_SIZE_G8  0x40	//(ALIGN(sizeof(SURFACE_STATE_G8),32))
#define SURFACE_STATE_OFFSET_G8(index) (BINDING_TABLE_OFFSET_G8(BINDING_TABLE_ENTRIES)+(SURFACE_STATE_SIZE_G8*index))

extern MEDIA_KERNEL media_hybrid_vp8_kernels_g8[NUM_OF_VP8_KERNELS_G8];
extern struct hw_codec_info gen8_hw_codec_info;


typedef struct surface_state_g8
{
  /*union { */
  struct
  {
    unsigned int cube_pos_z:1;
    unsigned int cube_neg_z:1;
    unsigned int cube_pos_y:1;
    unsigned int cube_neg_y:1;
    unsigned int cube_pos_x:1;
    unsigned int cube_neg_x:1;
    unsigned int media_boundry_pix_mode:2;
    unsigned int render_cache_read_write:1;
    unsigned int reserved0:1;
    unsigned int vert_line_stride_offset:1;
    unsigned int vert_line_stride:1;
    unsigned int tile_mode:2;
    unsigned int horizontal_alignment:2;
    unsigned int vertical_alignment:2;
    unsigned int surface_format:9;	   /**< BRW_SURFACEFORMAT_x */
    unsigned int reserved:1;
    unsigned int surface_array:1;
    unsigned int surface_type:3;	   /**< BRW_SURFACE_1D/2D/3D/CUBE */
  } dw0;
  struct
  {
    unsigned int surface_q_pitch:15;
    unsigned int:9;
    unsigned int obj_ctrl_state:7;
    unsigned int:1;
  } dw1;
  struct
  {
    unsigned int width:14;
    unsigned int:2;
    unsigned int height:14;
    unsigned int:2;
  } dw2;

/* union {*/
  struct
  {
    unsigned int surface_pitch:18;
    unsigned int:3;
    unsigned int depth:11;
/* };
 struct {
   unsigned int val;
    };
*/
  } dw3;
/*union {*/
  struct
  {
    unsigned int multisample_position_palette_index:3;
    unsigned int num_multisamples:3;
    unsigned int multisampled_surface_storage_format:1;
    unsigned int render_target_view_extent:11;
    unsigned int min_array_elt:11;
    unsigned int rotation:2;
    unsigned int:1;
/*};
 struct {
   unsigned int val;
    };
*/
  } dw4;

/*union {*/
  struct
  {
    unsigned int mip_count:4;
    unsigned int min_lod:4;
    unsigned int reserved0:6;
    unsigned int coherency_type:1;
    unsigned int:6;
    unsigned int y_offset:3;
    unsigned int pad0:1;
    unsigned int x_offset:7;
/* };
 struct {
   unsigned int val;
    };*/
  } dw5;

/*union {*/
  struct
  {
    /* Multisample ontrol surface stuff */
    unsigned int y_offset_uv_plane:14;
    unsigned int:2;
    unsigned int mcs_surface_picth:14;
    unsigned int:2;
/*};
struct
{
unsigned int val;
};*/
  } dw6;
/*union {*/
  struct
  {
    unsigned int resource_min_lod:12;
    unsigned int pad0:4;
    unsigned int shader_chanel_select_a:3;
    unsigned int shader_chanel_select_b:3;
    unsigned int shader_chanel_select_g:3;
    unsigned int shader_chanel_select_r:3;
    unsigned int alpha_clear_color:1;
    unsigned int blue_clear_color:1;
    unsigned int green_clear_color:1;
    unsigned int red_clear_color:1;
/*};
struct
{
unsigned int val;
};*/
  } dw7;
  struct
  {
    unsigned int base_addr;
  } dw8;

  struct
  {
    unsigned int base_addr64:16;
    unsigned int:16;
  } dw9;
  struct
  {
    unsigned int aux_base_addr:16;
    unsigned int:16;
  } dw10;
  struct
  {
    unsigned int aux_base_addr64:16;
    unsigned int:16;
  } dw11;

  struct
  {
    unsigned int hierarchical_depth_clear;
  } dw12;

  unsigned int padding[3];
} SURFACE_STATE_G8;


typedef struct gen8_surface_state2
{

  /* union
     { */
  struct
  {
    UINT value;
  } ss0;
  /*struct
     {
     unsigned int val;

     };
     } ss0; */
  /*union
     { */
  struct
  {
    UINT cbcr_pixel_offset_v_direction:2;
    UINT picture_structure:2;
    UINT width:14;
    UINT height:14;
    /* };
       struct
       {
       unsigned int val;
       }; */
  } ss1;
  /*union
     { */
  struct
  {
    UINT tile_mode:2;
    UINT half_pitch_for_chroma:1;
    UINT pitch:18;
    UINT address_control:1;
      UINT:4;
    UINT interleave_chroma:1;
    UINT surface_format:5;
    /* };
       struct
       {
       unsigned int val;
       }; */
  } ss2;

/*  union
  {*/
  struct
  {
    UINT y_offset_for_cb:15;
    UINT pad0:1;
    UINT x_offset_for_cb:14;
    UINT pad1:2;
    /* };
       struct
       {
       unsigned int val;
       }; */
  } ss3;

  /*union
     { */
  struct
  {
    UINT y_offset_for_cr:15;
    UINT pad0:1;
    UINT x_offset_for_cr:14;
    UINT pad1:2;
    /*};
       struct
       {
       unsigned int val;
       }; */
  } ss4;

  struct
  {
    UINT surface_memobj_ctrl_state:7;
      UINT:23;
    UINT vertical_line_stride_oofset:1;
    UINT vertical_line_stride:1;
  } ss5;

  struct
  {
    UINT surface_base_address:32;
  } ss6;

  struct
  {
    UINT surface_base_address64:16;
      UINT:16;
  } ss7;
  UINT padding[8];
} SURFACE_STATE_ADV_G8;

struct gen8_interface_descriptor_data
{
  struct
  {
    unsigned int pad0:6;
    unsigned int kernel_start_pointer:26;
  } desc0;

  struct
  {
    unsigned int kernel_start_pointer_high:16;
    unsigned int pad0:16;
  } desc1;

  struct
  {
    unsigned int pad0:7;
    unsigned int software_exception_enable:1;
    unsigned int pad1:3;
    unsigned int maskstack_exception_enable:1;
    unsigned int pad2:1;
    unsigned int illegal_opcode_exception_enable:1;
    unsigned int pad3:2;
    unsigned int floating_point_mode:1;
    unsigned int thread_priority:1;
    unsigned int single_program_flow:1;
    unsigned int denorm_mode:1;
    unsigned int pad4:12;
  } desc2;

  struct
  {
    unsigned int pad0:2;
    unsigned int sampler_count:3;
    unsigned int sampler_state_pointer:27;
  } desc3;

  struct
  {
    unsigned int binding_table_entry_count:5;
    unsigned int binding_table_pointer:11;
    unsigned int pad0:16;
  } desc4;

  struct
  {
    unsigned int constant_urb_entry_read_offset:16;
    unsigned int constant_urb_entry_read_length:16;
  } desc5;

  struct
  {
    unsigned int num_threads_in_tg:10;
    unsigned int pad0:5;
    unsigned int global_barrier_enable:1;
    unsigned int shared_local_memory_size:5;
    unsigned int barrier_enable:1;
    unsigned int rounding_mode:2;
    unsigned int pad1:8;
  } desc6;

  struct
  {
    unsigned int cross_thread_constant_data_read_length:8;
    unsigned int pad0:24;
  } desc7;
};
VOID
media_add_surface_state_g8 (SURFACE_SET_PARAMS * params);
VOID
media_add_binding_table_g8 (MEDIA_GPE_CTX * gpe_ctx);
VOID
media_surface_state_vp8_mbpak_g8 (MEDIA_ENCODER_CTX * encoder_context,
                                  struct encode_state *encode_state,
                                  MBPAK_SURFACE_PARAMS_VP8 *
                                  mbpak_sutface_params);
VOID
media_surface_state_vp8_mbenc_g8 (MEDIA_ENCODER_CTX * encoder_context,
                                  struct encode_state *encode_state,
                                  MBENC_SURFACE_PARAMS_VP8 *
                                  mbenc_sutface_params);
VOID
media_interface_setup_mbenc_g8 (MEDIA_ENCODER_CTX * encoder_context);

VOID
media_interface_setup_mbpak_g8 (MEDIA_GPE_CTX * mbpak_gpe_ctx);

VOID
media_hw_context_init_g8(VADriverContextP ctx, MEDIA_HW_CONTEXT *hw_ctx);

#endif
