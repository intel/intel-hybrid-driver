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

#ifndef _MEDIA__DRIVER_HW_G75_H
#define _MEDIA__DRIVER_HW_G75_H

#include "media_drv_encoder.h"
#include "media_drv_hw.h"
#define NUM_OF_VP8_KERNELS 12
#define MAX_QP_VP8               128
extern MEDIA_KERNEL media_hybrid_vp8_kernels[NUM_OF_VP8_KERNELS];
extern struct hw_codec_info gen75_hw_codec_info;
extern const BYTE rasterscan_48x40_vp8_g75[56];
extern const UINT new_mv_skip_threshold_VP8_g75[128];
extern const BYTE diamond_vp8_g75[56];
extern const UINT16 mv_ref_cost_context_vp8_g75[6][4][2];
extern const BYTE fullspiral_48x40_vp8_g75[56];
extern const UINT single_su_vp8_g75[14];
extern const UINT cost_table_vp8_g75[128][7];
extern const UINT new_mv_skip_threshold_VP8_g75[128];
extern const UINT16 quant_dc_vp8_g75[MAX_QP_VP8];
extern const UINT16 quant_ac_vp8_g75[MAX_QP_VP8];
extern const BYTE frame_i_vme_cost_vp8_g75[128][4];
extern const UINT16 quant_dc2_vp8_g75[];
extern const UINT16 quant_ac2_vp8_g75[];
extern const UINT16 quant_dc_uv_vp8_g75[];
extern const SURFACE_SET_PARAMS surface_set_params_init;
extern const BYTE brc_qpadjustment_distthreshold_maxframethreshold_distqpadjustment_IPB_vp8_g75[576];
extern const BYTE brc_iframe_cost_table_vp8_g75 [512];
extern const UINT16 brc_skip_mv_threshold_table_vp8_g75 [128];

#define HSW_SCS_ZERO                      0
#define HSW_SCS_ONE                       1
#define HSW_SCS_RED                       4
#define HSW_SCS_GREEN                     5
#define HSW_SCS_BLUE                      6
#define HSW_SCS_ALPHA                     7
#define MAX_QP_VP8_G75                          127
#define MB_CODE_SIZE_VP8       204
#define MB_MV_CODE_SIZE_VP8     64
#define MB_CODE_ALIGNMENT       32
#define MB_MV_ALIGNMENT         4096
#define DC_BIAS_SEGMENT_DEFAULT_VAL_VP8                    1500

#define QUAND_INDEX_Y1_DC_VP8                0
#define QUAND_INDEX_UV_DC_VP8                1
#define QUAND_INDEX_UV_AC_VP8                2
#define QUAND_INDEX_Y2_DC_VP8                3
#define QUAND_INDEX_Y2_AC_VP8                4

// AVC_ME CURBE
#if 1
#define OBJECT_BUFFER_TO_MEDIA_RESOURCE_STRUCT(obj_buffer,obj_buffer_res)   \
{                                                                                                   \
     obj_buffer_res->bo=obj_buffer->buffer_store->bo;                                            \
     obj_buffer_res->bo_size=obj_buffer->size_element;/*need to fill this later*/     \
     obj_buffer_res->buf=NULL;     \
     obj_buffer_res->surface_array_spacing=0;   \
 }
#define OBJECT_SURFACE_TO_MEDIA_RESOURCE_STRUCT(surface_2d,obj_surface)   \
{                                                                                                   \
     surface_2d.width=obj_surface->orig_width;                           \
     surface_2d.height=obj_surface->orig_height;                   \
     surface_2d.bo=obj_surface->bo;                                            \
     surface_2d.bo_size=0;     \
     surface_2d.pitch=obj_surface->width;    \
     dri_bo_get_tiling(obj_surface->bo, &surface_2d.tiling, &surface_2d.swizzle);    \
     surface_2d.buf=NULL;     \
     surface_2d.surface_array_spacing=0;   \
     surface_2d.cb_cr_pitch=obj_surface->cb_cr_pitch; \
     surface_2d.x_cb_offset=obj_surface->x_cb_offset; \
     surface_2d.y_cb_offset=obj_surface->y_cb_offset; \
  }

typedef enum _BINDING_TABLE_OFFSET_VP8_ME_G75
{
  VP8_ME_MV_DATA_SURFACE_G75 = 0,
  VP8_16xME_MV_DATA_SURFACE_G75 = 1,
  VP8_ME_DISTORTION_SURFACE_G75 = 2,
  VP8_ME_BRC_DISTORTION_SURFACE_G75 = 3,
  VP8_ME_INTER_PRED_G75 = 4,
  VP8_ME_LAST_REF_PIC_G75 = 5,
  VP8_ME_GOLDEN_REF_PIC_G75 = 7,
  VP8_ME_ALTERNATE_REF_PIC_G75 = 9,
  VP8_ME_NUM_SURFACES_G75 = 10
} BINDING_TABLE_OFFSET_VP8_ME_G75;

typedef enum _BINDING_TABLE_OFFSET_VP8_MBPAK_G75
{
  VP8_MBPAK_PER_MB_OUT_G75 = 0,
  VP8_MBPAK_CURR_Y_G75 = 1,
  VP8_MBPAK_CURR_UV_G75 = 2,
  VP8_MBPAK_CURR_RECON_Y_G75 = 3,
  VP8_MBPAK_CURR_RECON_UV_G75 = 4,
  //phase1 surfaces
  VP8_MBPAK_LAST_REF_Y_G75 = 5,
  VP8_MBPAK_LAST_REF_UV_G75 = 6,
  VP8_MBPAK_GOLDEN_REF_Y_G75 = 7,
  VP8_MBPAK_GOLDEN_REF_UV_G75 = 8,
  VP8_MBPAK_ALTERNATE_REF_Y_G75 = 9,
  VP8_MBPAK_ALTERNATE_REF_UV_G75 = 10,
  VP8_MBPAK_IND_MV_DATA_G75 = 11,
  //Phase2 surfaces 
  VP8_MBPAK_ROW_BUFF_Y_G75 = 5,
  VP8_MBPAK_ROW_BUFF_UV_G75 = 6,
  VP8_MBPAK_COL_BUFF_Y_G75 = 7,
  VP8_MBPAK_COL_BUFF_UV_G75 = 8,
  VP8_MBPAK_DEBUG_STREAMOUT_G75 = 12,
  VP8_MBPAK_NUM_SURFACES_G75 = 13
} BINDING_TABLE_OFFSET_VP8_MBPAK_G75;

typedef enum _BINDING_TABLE_OFFSET_VP8_BRC_INIT_RESET_G75
{
  VP8_BRC_INIT_RESET_HISTORY_G75 = 0,
  VP8_BRC_INIT_RESET_DISTORTION_G75 = 1,
  VP8_BRC_INIT_RESET_NUM_SURFACES_G75 = 2
} BINDING_TABLE_OFFSET_VP8_BRC_INIT_RESET_G75;

typedef enum _BINDING_TABLE_OFFSET_VP8_BRC_UPDATE_G75
{
  VP8_BRC_UPDATE_HISTORY_G75 = 0,
  VP8_BRC_UPDATE_PAK_SURFACE_INDEX_G75 = 1,
  VP8_BRC_UPDATE_MBPAK1_CURBE_WRITE_G75 = 2,
  VP8_BRC_UPDATE_MBPAK2_CURBE_WRITE_G75 = 3,
  VP8_BRC_UPDATE_MBENC_CURBE_READ_G75 = 4,
  VP8_BRC_UPDATE_MBENC_CURBE_WRITE_G75 = 5,
  VP8_BRC_UPDATE_DISTORTION_SURFACE_G75 = 6,
  VP8_BRC_UPDATE_CONSTANT_DATA_G75 = 7,
  VP8_BRC_UPDATE_MBPAK_TABLE_INDEX_G75 = 8,
  VP8_BRC_UPDATE_SEGMENT_MAP_G75 = 9,
  VP8_BRC_UPDATE_NUM_SURFACES_G75 = 10
} BINDING_TABLE_OFFSET_VP8_BRC_UPDATE_G75;

typedef struct _search_path_delta
{
  BYTE search_path_delta_x:4;
  BYTE search_path_delta_y:4;
} SEARCH_PATH_DELTA;
typedef struct _media_curbe_data_mbpak_p1_g75
{
  // dw0
  union
  {
    struct
    {
      UINT frame_width:16;
      UINT frame_height:16;
    };
    struct
    {
      UINT dw0_val;
    };
  } dw0;

  // dw1
  union
  {
    struct
    {
      UINT:8;
      UINT sharpness_level:8;
        UINT:4;
      UINT loop_filter_type:1;
      UINT frame_type:1;
      UINT recon_filter_type:3;
      UINT clamping_flag:1;
        UINT:6;
    };
    struct
    {
      UINT dw1_val;
    };
  } dw1;

  // dw2
  union
  {
    struct
    {
      UINT y_dc_q_mul_factor_segment0:16;
      UINT y_ac_q_mul_factor_segment0:16;
    };
    struct
    {
      UINT dw2_val;
    };
  } dw2;

  // dw3
  union
  {
    struct
    {
      UINT y2_dc_q_mul_factor_segment0:16;
      UINT y2_ac_q_mul_factor_segment0:16;
    };
    struct
    {
      UINT dw3_val;
    };
  } dw3;

  // dw4
  union
  {
    struct
    {
      UINT uv_dc_q_mul_factor_segment0:16;
      UINT uv_ac_q_mul_factor_segment0:16;
    };
    struct
    {
      UINT dw4_val;
    };
  } dw4;

  // dw5
  union
  {
    struct
    {
      UINT y_dc_inv_q_mul_factor_segment0:16;
      UINT y_ac_inv_q_mul_factor_segment0:16;
    };
    struct
    {
      UINT dw5_val;
    };
  } dw5;

  // dw6
  union
  {
    struct
    {
      UINT y2_dc_inv_q_mul_factor_segment0:16;
      UINT y2_ac_inv_q_mul_factor_segment0:16;
    };
    struct
    {
      UINT dw6_val;
    };
  } dw6;

  // dw7
  union
  {
    struct
    {
      UINT uv_dc_inv_q_mul_factor_segment0:16;
      UINT uv_ac_inv_q_mul_factor_segment0:16;
    };
    struct
    {
      UINT dw7_val;
    };
  } dw7;

  // dw8
  union
  {
    struct
    {
      UINT y2_dc_q_shift_factor_segment0:8;
      UINT y2_ac_q_shift_factor_segment0:8;
      UINT y_dc_q_shift_factor_segment0:8;
      UINT y_ac_q_shift_factor_segment0:8;
    };
    struct
    {
      UINT dw8_val;
    };
  } dw8;

  // dw9
  union
  {
    struct
    {
      UINT uv_dc_q_shift_factor_segment0:8;
      UINT uv_ac_q_shift_factor_segment0:8;
    };
    struct
    {
      UINT dw9_val;
    };
  } dw9;

  // dw10
  union
  {
    struct
    {
      UINT y_dc_q_mul_factor_segment1:16;
      UINT y_ac_q_mul_factor_segment1:16;
    };
    struct
    {
      UINT dw10_val;
    };
  } dw10;

  // dw11
  union
  {
    struct
    {
      UINT y2_dc_q_mul_factor_segment1:16;
      UINT y2_ac_q_mul_factor_segment1:16;
    };
    struct
    {
      UINT dw11_val;
    };
  } dw11;

  // dw12
  union
  {
    struct
    {
      UINT uv_dc_q_mul_factor_segment1:16;
      UINT uv_ac_q_mul_factor_segment1:16;
    };
    struct
    {
      UINT dw12_val;
    };
  } dw12;

  // dw13
  union
  {
    struct
    {
      UINT y_dc_inv_q_mul_factor_segment1:16;
      UINT y_ac_inv_q_mul_factor_segment1:16;
    };
    struct
    {
      UINT dw13_val;
    };
  } dw13;

  // dw14
  union
  {
    struct
    {
      UINT y2_dc_inv_q_mul_factor_segment1:16;
      UINT y2_ac_inv_q_mul_factor_segment1:16;
    };
    struct
    {
      UINT dw14_val;
    };
  } dw14;

  // dw15
  union
  {
    struct
    {
      UINT uv_dc_inv_q_mul_factor_segment1:16;
      UINT uv_ac_inv_q_mul_factor_segment1:16;
    };
    struct
    {
      UINT dw15_val;
    };
  } dw15;

  // dw16
  union
  {
    struct
    {
      UINT y2_dc_q_shift_factor_segment1:8;
      UINT y2_ac_q_shift_factor_segment1:8;
      UINT y_dc_q_shift_factor_segment1:8;
      UINT y_ac_q_shift_factor_segment1:8;
    };
    struct
    {
      UINT dw16_val;
    };
  } dw16;

  // dw17
  union
  {
    struct
    {
      UINT uv_dc_q_shift_factor_segment1:8;
      UINT uv_ac_q_shift_factor_segment1:8;
    };
    struct
    {
      UINT dw17_val;
    };
  } dw17;

  // dw18
  union
  {
    struct
    {
      UINT y_dc_q_mul_factor_segment2:16;
      UINT y_ac_q_mul_factor_segment2:16;
    };
    struct
    {
      UINT dw18_val;
    };
  } dw18;

  // dw19
  union
  {
    struct
    {
      UINT y2_dc_q_mul_factor_segment2:16;
      UINT y2_ac_q_mul_factor_segment2:16;
    };
    struct
    {
      UINT dw19_val;
    };
  } dw19;

  // dw20
  union
  {
    struct
    {
      UINT uv_dc_q_mul_factor_segment2:16;
      UINT uv_ac_q_mul_factor_segment2:16;
    };
    struct
    {
      UINT dw20_val;
    };
  } dw20;

  // dw21
  union
  {
    struct
    {
      UINT y_dc_inv_q_mul_factor_segment2:16;
      UINT y_ac_inv_q_mul_factor_segment2:16;
    };
    struct
    {
      UINT dw21_val;
    };
  } dw21;

  // dw22
  union
  {
    struct
    {
      UINT y2_dc_inv_q_mul_factor_segment2:16;
      UINT y2_ac_inv_q_mul_factor_segment2:16;
    };
    struct
    {
      UINT dw22_val;
    };
  } dw22;

  // dw23
  union
  {
    struct
    {
      UINT uv_dc_inv_q_mul_factor_segment2:16;
      UINT uv_ac_inv_q_mul_factor_segment2:16;
    };
    struct
    {
      UINT dw23_val;
    };
  } dw23;

  // dw24
  union
  {
    struct
    {
      UINT y2_dc_q_shift_factor_segment2:8;
      UINT y2_ac_q_shift_factor_segment2:8;
      UINT y_dc_q_shift_factor_segment2:8;
      UINT y_ac_q_shift_factor_segment2:8;
    };
    struct
    {
      UINT dw24_val;
    };
  } dw24;

  // dw25
  union
  {
    struct
    {
      UINT uv_dc_q_shift_factor_segment2:8;
      UINT uv_ac_q_shift_factor_segment2:8;
    };
    struct
    {
      UINT dw25_val;
    };
  } dw25;

  // dw26
  union
  {
    struct
    {
      UINT y_dc_q_mul_factor_segment3:16;
      UINT y_ac_q_mul_factor_segment3:16;
    };
    struct
    {
      UINT dw26_val;
    };
  } dw26;

  // dw27
  union
  {
    struct
    {
      UINT y2_dc_q_mul_factor_segment3:16;
      UINT y2_ac_q_mul_factor_segment3:16;
    };
    struct
    {
      UINT dw27_val;
    };
  } dw27;

  // dw28
  union
  {
    struct
    {
      UINT uv_dc_q_mul_factor_segment3:16;
      UINT uv_ac_q_mul_factor_segment3:16;
    };
    struct
    {
      UINT dw28_val;
    };
  } dw28;

  // dw29
  union
  {
    struct
    {
      UINT y_dc_inv_q_mul_factor_segment3:16;
      UINT y_ac_inv_q_mul_factor_segment3:16;
    };
    struct
    {
      UINT dw29_val;
    };
  } dw29;

  // dw30
  union
  {
    struct
    {
      UINT y2_dc_inv_q_mul_factor_segment3:16;
      UINT y2_ac_inv_q_mul_factor_segment3:16;
    };
    struct
    {
      UINT dw30_val;
    };
  } dw30;

  // dw31
  union
  {
    struct
    {
      UINT uv_dc_inv_q_mul_factor_segment3:16;
      UINT uv_ac_inv_q_mul_factor_segment3:16;
    };
    struct
    {
      UINT dw31_val;
    };
  } dw31;

  // dw32
  union
  {
    struct
    {
      UINT y2_dc_q_shift_factor_segment3:8;
      UINT y2_ac_q_shift_factor_segment3:8;
      UINT y_dc_q_shift_factor_segment3:8;
      UINT y_ac_q_shift_factor_segment3:8;
    };
    struct
    {
      UINT dw32_val;
    };
  } dw32;

  // dw33
  union
  {
    struct
    {
      UINT uv_dc_q_shift_factor_segment3:8;
      UINT uv_ac_q_shift_factor_segment3:8;
    };
    struct
    {
      UINT dw33_val;
    };
  } dw33;

  // dw34
  union
  {
    struct
    {
      UINT ref_frame_lf_delta0:8;
      UINT ref_frame_lf_delta1:8;
      UINT ref_frame_lf_delta2:8;
      UINT ref_frame_lf_delta3:8;
    };
    struct
    {
      UINT dw34_val;
    };
  } dw34;

  // dw35
  union
  {
    struct
    {
      UINT mode_lf_delta0:8;
      UINT mode_lf_delta1:8;
      UINT mode_lf_delta2:8;
      UINT mode_lf_delta3:8;
    };
    struct
    {
      UINT dw35_val;
    };
  } dw35;

  // dw36
  union
  {
    struct
    {

      UINT lf_level0:8;
      UINT lf_level1:8;
      UINT lf_level2:8;
      UINT lf_level3:8;
    };
    struct
    {
      UINT dw36_val;
    };
  } dw36;

  // dw37
  union
  {
    struct
    {
      UINT dw37_val;
    };
  } dw37;

  // dw38
  union
  {
    struct
    {
      UINT dw38_val;
    };
  } dw38;

  // dw39
  union
  {
    struct
    {
      UINT dw39_val;
    };
  } dw39;

  // dw40
  union
  {
    struct
    {
      UINT pak_per_mb_out_data_surf_bti:32;
    };
    struct
    {
      UINT dw40_val;
    };
  } dw40;

  // dw41
  union
  {
    struct
    {
      UINT mb_enc_curr_y_bti:32;
    };
    struct
    {
      UINT dw41_val;
    };
  } dw41;

  // dw42
  union
  {
    struct
    {
      UINT pak_recon_y_bti:32;
    };
    struct
    {
      UINT dw42_val;
    };
  } dw42;

  // dw43
  union
  {
    struct
    {
      UINT pak_last_ref_pic_y_bti:32;
    };
    struct
    {
      UINT dw43_val;
    };
  } dw43;

  // dw44
  union
  {
    struct
    {
      UINT pak_golden_ref_pic_y_bti:32;
    };
    struct
    {
      UINT dw44_val;
    };
  } dw44;

  // dw45
  union
  {
    struct
    {
      UINT pak_alternate_ref_pic_y_bti:32;
    };
    struct
    {
      UINT dw45_val;
    };
  } dw45;

  // dw46
  union
  {
    struct
    {
      UINT pak_ind_mv_data_bti:32;
    };
    struct
    {
      UINT dw46_val;
    };
  } dw46;

  // dw47
  union
  {
    struct
    {
      UINT pak_kernel_debug_bti:32;
    };
    struct
    {
      UINT dw47_val;
    };
  } dw47;

} MEDIA_CURBE_DATA_MBPAK_P1_G75;

typedef struct _media_curbe_data_mbpak_p2_g75
{
  // dw0
  union
  {
    struct
    {
      UINT frame_width:16;
      UINT frame_height:16;
    };
    struct
    {
      UINT dw0_val;
    };
  } dw0;

  // dw1
  union
  {
    struct
    {
      UINT:8;
      UINT sharpness_level:8;
        UINT:4;
      UINT loop_filter_type:1;
      UINT frame_type:1;
      UINT recon_filter_type:3;
      UINT clamping_flag:1;
        UINT:6;
    };
    struct
    {
      UINT dw1_val;
    };
  } dw1;

  // dw2
  union
  {
    struct
    {
      UINT y_dc_q_mul_factor_segment0:16;
      UINT y_ac_q_mul_factor_segment0:16;
    };
    struct
    {
      UINT dw2_val;
    };
  } dw2;

  // dw3
  union
  {
    struct
    {
      UINT y2_dc_q_mul_factor_segment0:16;
      UINT y2_ac_q_mul_factor_segment0:16;
    };
    struct
    {
      UINT dw3_val;
    };
  } dw3;

  // dw4
  union
  {
    struct
    {
      UINT uv_dc_q_mul_factor_segment0:16;
      UINT uv_ac_q_mul_factor_segment0:16;
    };
    struct
    {
      UINT dw4_val;
    };
  } dw4;

  // dw5
  union
  {
    struct
    {
      UINT y_dc_inv_q_mul_factor_segment0:16;
      UINT y_ac_inv_q_mul_factor_segment0:16;
    };
    struct
    {
      UINT dw5_val;
    };
  } dw5;

  // dw6
  union
  {
    struct
    {
      UINT y2_dc_inv_q_mul_factor_segment0:16;
      UINT y2_ac_inv_q_mul_factor_segment0:16;
    };
    struct
    {
      UINT dw6_val;
    };
  } dw6;

  // dw7
  union
  {
    struct
    {
      UINT uv_dc_inv_q_mul_factor_segment0:16;
      UINT uv_ac_inv_q_mul_factor_segment0:16;
    };
    struct
    {
      UINT dw7_val;
    };
  } dw7;

  // dw8
  union
  {
    struct
    {
      UINT y2_dc_q_shift_factor_segment0:8;
      UINT y2_ac_q_shift_factor_segment0:8;
      UINT y_dc_q_shift_factor_segment0:8;
      UINT y_ac_q_shift_factor_segment0:8;
    };
    struct
    {
      UINT dw8_val;
    };
  } dw8;

  // dw9
  union
  {
    struct
    {
      UINT uv_dc_q_shift_factor_segment0:8;
      UINT uv_ac_q_shift_factor_segment0:8;
    };
    struct
    {
      UINT dw9_val;
    };
  } dw9;

  // dw10
  union
  {
    struct
    {
      UINT y_dc_q_mul_factor_segment1:16;
      UINT y_ac_q_mul_factor_segment1:16;
    };
    struct
    {
      UINT dw10_val;
    };
  } dw10;

  // dw11
  union
  {
    struct
    {
      UINT y2_dc_q_mul_factor_segment1:16;
      UINT y2_ac_q_mul_factor_segment1:16;
    };
    struct
    {
      UINT dw11_val;
    };
  } dw11;

  // dw12
  union
  {
    struct
    {
      UINT uv_dc_q_mul_factor_segment1:16;
      UINT uv_ac_q_mul_factor_segment1:16;
    };
    struct
    {
      UINT dw12_val;
    };
  } dw12;

  // dw13
  union
  {
    struct
    {
      UINT y_dc_inv_q_mul_factor_segment1:16;
      UINT y_ac_inv_q_mul_factor_segment1:16;
    };
    struct
    {
      UINT dw13_val;
    };
  } dw13;

  // dw14
  union
  {
    struct
    {
      UINT y2_dc_inv_q_mul_factor_segment1:16;
      UINT y2_ac_inv_q_mul_factor_segment1:16;
    };
    struct
    {
      UINT dw14_val;
    };
  } dw14;

  // dw15
  union
  {
    struct
    {
      UINT uv_dc_inv_q_mul_factor_segment1:16;
      UINT uv_ac_inv_q_mul_factor_segment1:16;
    };
    struct
    {
      UINT dw15_val;
    };
  } dw15;

  // dw16
  union
  {
    struct
    {
      UINT y2_dc_q_shift_factor_segment1:8;
      UINT y2_ac_q_shift_factor_segment1:8;
      UINT y_dc_q_shift_factor_segment1:8;
      UINT y_ac_q_shift_factor_segment1:8;
    };
    struct
    {
      UINT dw16_val;
    };
  } dw16;

  // dw17
  union
  {
    struct
    {
      UINT uv_dc_q_shift_factor_segment1:8;
      UINT uv_ac_q_shift_factor_segment1:8;
    };
    struct
    {
      UINT dw17_val;
    };
  } dw17;

  // dw18
  union
  {
    struct
    {
      UINT y_dc_q_mul_factor_segment2:16;
      UINT y_ac_q_mul_factor_segment2:16;
    };
    struct
    {
      UINT dw18_val;
    };
  } dw18;

  // dw19
  union
  {
    struct
    {
      UINT y2_dc_q_mul_factor_segment2:16;
      UINT y2_ac_q_mul_factor_segment2:16;
    };
    struct
    {
      UINT dw19_val;
    };
  } dw19;

  // dw20
  union
  {
    struct
    {
      UINT uv_dc_q_mul_factor_segment2:16;
      UINT uv_ac_q_mul_factor_segment2:16;
    };
    struct
    {
      UINT dw20_val;
    };
  } dw20;

  // dw21
  union
  {
    struct
    {
      UINT y_dc_inv_q_mul_factor_segment2:16;
      UINT y_ac_inv_q_mul_factor_segment2:16;
    };
    struct
    {
      UINT dw21_val;
    };
  } dw21;

  // dw22
  union
  {
    struct
    {
      UINT y2_dc_inv_q_mul_factor_segment2:16;
      UINT y2_ac_inv_q_mul_factor_segment2:16;
    };
    struct
    {
      UINT dw22_val;
    };
  } dw22;

  // dw23
  union
  {
    struct
    {
      UINT uv_dc_inv_q_mul_factor_segment2:16;
      UINT uv_ac_inv_q_mul_factor_segment2:16;
    };
    struct
    {
      UINT dw23_val;
    };
  } dw23;

  // dw24
  union
  {
    struct
    {
      UINT y2_dc_q_shift_factor_segment2:8;
      UINT y2_ac_q_shift_factor_segment2:8;
      UINT y_dc_q_shift_factor_segment2:8;
      UINT y_ac_q_shift_factor_segment2:8;
    };
    struct
    {
      UINT dw24_val;
    };
  } dw24;

  // dw25
  union
  {
    struct
    {
      UINT uv_dc_q_shift_factor_segment2:8;
      UINT uv_ac_q_shift_factor_segment2:8;
    };
    struct
    {
      UINT dw25_val;
    };
  } dw25;

  // dw26
  union
  {
    struct
    {
      UINT y_dc_q_mul_factor_segment3:16;
      UINT y_ac_q_mul_factor_segment3:16;
    };
    struct
    {
      UINT dw26_val;
    };
  } dw26;

  // dw27
  union
  {
    struct
    {
      UINT y2_dc_q_mul_factor_segment3:16;
      UINT y2_ac_q_mul_factor_segment3:16;
    };
    struct
    {
      UINT dw27_val;
    };
  } dw27;

  // dw28
  union
  {
    struct
    {
      UINT uv_dc_q_mul_factor_segment3:16;
      UINT uv_ac_q_mul_factor_segment3:16;
    };
    struct
    {
      UINT dw28_val;
    };
  } dw28;

  // dw29
  union
  {
    struct
    {
      UINT y_dc_inv_q_mul_factor_segment3:16;
      UINT y_ac_inv_q_mul_factor_segment3:16;
    };
    struct
    {
      UINT dw29_val;
    };
  } dw29;

  // dw30
  union
  {
    struct
    {
      UINT y2_dc_inv_q_mul_factor_segment3:16;
      UINT y2_ac_inv_q_mul_factor_segment3:16;
    };
    struct
    {
      UINT dw30_val;
    };
  } dw30;

  // dw31
  union
  {
    struct
    {
      UINT uv_dc_inv_q_mul_factor_segment3:16;
      UINT uv_ac_inv_q_mul_factor_segment3:16;
    };
    struct
    {
      UINT dw31_val;
    };
  } dw31;

  // dw32
  union
  {
    struct
    {
      UINT y2_dc_q_shift_factor_segment3:8;
      UINT y2_ac_q_shift_factor_segment3:8;
      UINT y_dc_q_shift_factor_segment3:8;
      UINT y_ac_q_shift_factor_segment3:8;
    };
    struct
    {
      UINT dw32_val;
    };
  } dw32;

  // dw33
  union
  {
    struct
    {
      UINT uv_dc_q_shift_factor_segment3:8;
      UINT uv_ac_q_shift_factor_segment3:8;
    };
    struct
    {
      UINT dw33_val;
    };
  } dw33;

  // dw34
  union
  {
    struct
    {
      UINT ref_frame_lf_delta0:8;
      UINT ref_frame_lf_delta1:8;
      UINT ref_frame_lf_delta2:8;
      UINT ref_frame_lf_delta3:8;
    };
    struct
    {
      UINT dw34_val;
    };
  } dw34;

  // dw35
  union
  {
    struct
    {
      UINT mode_lf_delta0:8;
      UINT mode_lf_delta1:8;
      UINT mode_lf_delta2:8;
      UINT mode_lf_delta3:8;
    };
    struct
    {
      UINT dw35_val;
    };
  } dw35;

  // dw36
  union
  {
    struct
    {

      UINT lf_level0:8;
      UINT lf_level1:8;
      UINT lf_level2:8;
      UINT lf_level3:8;
    };
    struct
    {
      UINT dw36_val;
    };
  } dw36;

  // dw37
  union
  {
    struct
    {
      UINT dw37_val;
    };
  } dw37;

  // dw38
  union
  {
    struct
    {
      UINT dw38_val;
    };
  } dw38;

  // dw39
  union
  {
    struct
    {
      UINT dw39_val;
    };
  } dw39;

  // dw40
  union
  {
    struct
    {
      UINT pak_per_mb_out_data_surf_bti:32;
    };
    struct
    {
      UINT dw40_val;
    };
  } dw40;

  // dw41
  union
  {
    struct
    {
      UINT mb_enc_curr_y_bti:32;
    };
    struct
    {
      UINT dw41_val;
    };
  } dw41;

  // dw42
  union
  {
    struct
    {
      UINT pak_recon_y_bti:32;
    };
    struct
    {
      UINT dw42_val;
    };
  } dw42;

  // dw43
  union
  {
    struct
    {
      UINT pak_row_buffer_y_bti:32;
    };
    struct
    {
      UINT dw43_val;
    };
  } dw43;

  // dw44
  union
  {
    struct
    {
      UINT pak_row_buffer_uv_bti:32;
    };
    struct
    {
      UINT dw44_val;
    };
  } dw44;

  // dw45
  union
  {
    struct
    {
      UINT pak_col_buffer_y_bti:32;
    };
    struct
    {
      UINT dw45_val;
    };
  } dw45;

  // dw46
  union
  {
    struct
    {
      UINT pak_col_buffer_uv_bti:32;
    };
    struct
    {
      UINT dw46_val;
    };
  } dw46;

  // dw47
  union
  {
    struct
    {
      UINT pak_kernel_debug_bti:32;
    };
    struct
    {
      UINT dw47_val;
    };
  } dw47;

} MEDIA_CURBE_DATA_MBPAK_P2_G75;

// VP8_MBEnc I frame kernel CURBE
typedef struct _media_curbe_data_mbenc_i_g75
{
  // dw0
  struct
  {
    UINT frame_width:16;
    UINT frame_height:16;
  } dw0;

  // dw1
  struct
  {
    UINT frame_type:1;
    UINT enable_segmentation:1;
    UINT enable_hw_intra_prediction:1;
    UINT enable_debug_dumps:1;
    UINT enable_coeff_clamp:1;
    UINT enable_chroma_ip_enhancement:1;
    UINT enable_mpu_histogram_update:1;
    UINT reserved_mbz:1;
    UINT vme_enable_tm_check:1;
    UINT vme_distortion_measure:2;
      UINT:21;
  } dw1;


  // dw2
  struct
  {
    UINT lambda_seg_0:16;
    UINT lambda_seg_1:16;
  } dw2;

  // dw3

  struct
  {
    UINT lambda_seg_2:16;
    UINT lambda_seg_3:16;
  } dw3;

  // dw4
  struct
  {
    UINT all_dc_bias_segment_0:16;
    UINT all_dc_bias_segment_1:16;
  } dw4;

  // dw5
  struct
  {
    UINT all_dc_bias_segment_2:16;
    UINT all_dc_bias_segment_3:16;
  } dw5;

  //dw6
  struct
  {
    UINT chroma_dc_de_quant_segment_0:16;
    UINT chroma_dc_de_quant_segment_1:16;
  } dw6;


  // dw7
  struct
  {
    UINT chroma_dc_de_quant_segment_2:16;
    UINT chroma_dc_de_quant_segment_3:16;
  } dw7;

  // dw8
  struct
  {
    UINT chroma_ac_de_quant_segment0:16;
    UINT chroma_ac_de_quant_segment1:16;
  } dw8;

  //dw9
  struct
  {
    UINT chroma_ac_de_quant_segment2:16;
    UINT chroma_ac_de_quant_segment3:16;
  } dw9;

  // 
  struct
  {
    UINT chroma_ac0_threshold0_segment0:16;
    UINT chroma_ac0_threshold1_segment0:16;
  } dw10;

  // dw10

  struct
  {
    UINT chroma_ac0_threshold0_segment1:16;
    UINT chroma_ac0_threshold1_segment1:16;
  } dw11;


  // dw12
  struct
  {
    UINT chroma_ac0_threshold0_segment2:16;
    UINT chroma_ac0_threshold1_segment2:16;

  } dw12;

  //dw13
  struct
  {
    UINT chroma_ac0_threshold0_segment3:16;
    UINT chroma_ac0_threshold1_segment3:16;
  } dw13;
  // dw14
  struct
  {
    UINT chroma_dc_threshold0_segment0:16;
    UINT chroma_dc_threshold1_segment0:16;
  } dw14;

  // dw15
  struct
  {
    UINT chroma_dc_threshold2_segment0:16;
    UINT chroma_dc_threshold3_segment0:16;
  } dw15;

  // dw16
  struct
  {
    UINT chroma_dc_threshold0_segment1:16;
    UINT chroma_dc_threshold1_segment1:16;
  } dw16;

  // dw17
  struct
  {
    UINT chroma_dc_threshold2_segment1:16;
    UINT chroma_dc_threshold3_segment1:16;
  } dw17;

  // dw18
  struct
  {
    UINT chroma_dc_threshold0_segment2:16;
    UINT chroma_dc_threshold1_segment2:16;
  } dw18;

  // dw19
  struct
  {
    UINT chroma_dc_threshold2_segment2:16;
    UINT chroma_dc_threshold3_segment2:16;
  } dw19;

  // dw20
  struct
  {
    UINT chroma_dc_threshold0_segment3:16;
    UINT chroma_dc_threshold1_segment3:16;
  } dw20;

  // dw21
  struct
  {
    UINT chroma_dc_threshold2_segment3:16;
    UINT chroma_dc_threshold3_segment3:16;
  } dw21;


  // dw22
  struct
  {
    UINT chroma_ac1_threshold_segment0:16;
    UINT chroma_ac1_threshold_segment1:16;
  } dw22;

  // dw23
  struct
  {
    UINT chroma_ac1_threshold_segment2:16;
    UINT chroma_ac1_threshold_segment3:16;
  } dw23;


  // dw24
  struct
  {
    UINT vme_16x16_cost_segment0:8;
    UINT vme_16x16_cost_segment1:8;
    UINT vme_16x16_cost_segment2:8;
    UINT vme_16x16_cost_segment3:8;
  } dw24;

  //dw25

  struct
  {
    UINT vme_4x4_cost_segment0:8;
    UINT vme_4x4_cost_segment1:8;
    UINT vme_4x4_cost_segment2:8;
    UINT vme_4x4_cost_segment3:8;
  } dw25;

  // dw26
  struct
  {
    UINT vme_16x16_non_dc_penalty_segment0:8;
    UINT vme_16x16_non_dc_penalty_segment1:8;
    UINT vme_16x16_non_dc_penalty_segment2:8;
    UINT vme_16x16_non_dc_penalty_segment3:8;
  } dw26;

  // dw27
  struct
  {
    UINT vme_4x4_non_dc_penalty_segment0:8;
    UINT vme_4x4_non_dc_penalty_segment1:8;
    UINT vme_4x4_non_dc_penalty_segment2:8;
    UINT vme_4x4_non_dc_penalty_segment3:8;
  } dw27;

  // dw28
  struct
  {
    UINT reserved:32;
  } dw28;

  // dw29
  struct
  {
    UINT reserved:32;
  } dw29;


  // dw30
  struct
  {
    UINT reserved:32;
  } dw30;
  // dw31
  struct
  {
    UINT reserved:32;
  } dw31;

  // dw32
  struct
  {
    UINT mb_enc_per_mb_out_data_surf_bti:32;
  } dw32;
  // dw33
  struct
  {
    UINT mb_enc_curr_y_bti:32;
  } dw33;
  // dw34
  struct
  {
    UINT mb_enc_curr_uv_bti:32;
  } dw34;

  // dw35
  struct
  {
    UINT mb_mode_cost_luma_bti:32;
  } dw35;
  // dw36
  struct
  {
    UINT mb_enc_block_mode_cost_bti:32;
  } dw36;

  // dw37
  struct
  {
    UINT chroma_recon_surf_bti:32;
  } dw37;

  // dw38
  struct
  {
    UINT segmentation_map_bti:32;
  } dw38;
  // dw39
  struct
  {
    UINT histogram_bti:32;
  } dw39;
  // dw40
  struct
  {
    UINT mb_enc_vme_debug_stream_out_bti:32;
  } dw40;

  // dw41
  struct
  {
    UINT vme_bti:32;
  } dw41;

  // dw42
  struct
  {
    UINT idist_surface:32;
  } dw42;
  // dw43
  struct
  {
    UINT curr_y_surface4x_downscaled:32;
  } dw43;
  // dw44
  struct
  {
    UINT vme_coarse_intra_surface:32;
  } dw44;

} MEDIA_CURBE_DATA_MBENC_I_G75;


typedef struct _media_curbe_data_hme_kernel
{

  struct
  {
    UINT skip_mode_en:1;
    UINT adaptive_en:1;
    UINT Bi_dir_mix_dis:1;
      UINT:2;
    UINT early_ime_success_en:1;
      UINT:1;
    UINT t_8x8_flag_for_inter_en:1;
      UINT:16;
    UINT early_ime_stop:8;
  } dw0;



  struct
  {
    UINT max_num_mvs:6;
      UINT:10;
    UINT bi_weight:6;
      UINT:6;
    UINT uni_mix_disable:1;
      UINT:3;
  } dw1;


  struct
  {
    UINT max_len_sp:8;
    UINT max_num_su:8;
      UINT:16;
  } dw2;



  struct
  {
    UINT src_size:2;
      UINT:2;
    UINT mb_type_remap:2;
    UINT src_access:1;
    UINT ref_access:1;
    UINT search_ctrl:3;
    UINT dual_search_path_option:1;
    UINT sub_pel_mode:2;
    UINT skip_type:1;
    UINT disable_field_cache_alloc:1;
    UINT inter_chroma_mode:1;
    UINT fte_enable:1;
    UINT bme_disable_fbr:1;
    UINT block_based_skip_enable:1;
    UINT inter_sad:2;
    UINT intra_sad:2;
    UINT sub_mb_part_mask:7;
      UINT:1;
  } dw3;


  struct
  {
    UINT:8;
    UINT picture_height_minus1:8;
    UINT picture_width:8;
    UINT b_file_idr:1;
      UINT:7;
  } dw4;

  struct
  {
    UINT:8;
    UINT qp_prime_y:8;
    UINT ref_width:8;
    UINT ref_height:8;

  } dw5;


  struct
  {
    UINT:3;
    UINT me_modes:2;
      UINT:3;
    UINT super_combine_dist:8;
    UINT max_vmv_range:16;
  } dw6;

  struct
  {
    UINT:16;
    UINT mv_cost_scale_factor:2;
    UINT bilinear_enable:1;
    UINT src_field_polarity:1;
    UINT en_weighted_sad_haar:1;
    UINT ac_only_haar:1;
    UINT ref_id_cost_mode:1;
      UINT:1;
    UINT skip_center_mask:8;
  } dw7;


  struct
  {
    UINT mode_0_cost:8;
    UINT mode_1_cost:8;
    UINT mode_2_cost:8;
    UINT mode_3_Cost:8;
  } dw8;


  struct
  {
    UINT mode_4_cost:8;
    UINT mode_5_cost:8;
    UINT mode_6_cost:8;
    UINT mode_7_cost:8;
  } dw9;


  struct
  {
    UINT mode_8_cost:8;
    UINT mode_9_cost:8;
    UINT ref_id_cost:8;
    UINT chroma_intra_mode_cost:8;
  } dw10;

  struct
  {
    UINT mv_0_cost:8;
    UINT mv_1_cost:8;
    UINT mv_2_cost:8;
    UINT mv_3_cost:8;
  } dw11;

  struct
  {
    UINT mv_4_cost:8;
    UINT mv_5_cost:8;
    UINT mv_6_cost:8;
    UINT mv_7_cost:8;
  } dw12;


  struct
  {
    UINT num_ref_idx_l0_minus_one:8;
    UINT num_ref_idx_l1_minus_one:8;
      UINT:16;
  } dw13;

  struct
  {
    UINT reserved:32;
  } dw14;

  struct
  {
    UINT reserved:32;

  } dw15;

  struct
  {
    SEARCH_PATH_DELTA sp_delta_0;
    SEARCH_PATH_DELTA sp_delta_1;
    SEARCH_PATH_DELTA sp_delta_2;
    SEARCH_PATH_DELTA sp_delta_3;
  } dw16;

  struct
  {
    SEARCH_PATH_DELTA sp_delta_4;
    SEARCH_PATH_DELTA sp_delta_5;
    SEARCH_PATH_DELTA sp_delta_6;
    SEARCH_PATH_DELTA sp_delta_7;
  } dw17;

  struct
  {
    SEARCH_PATH_DELTA SPDelta_8;
    SEARCH_PATH_DELTA SPDelta_9;
    SEARCH_PATH_DELTA SPDelta_10;
    SEARCH_PATH_DELTA SPDelta_11;
  } dw18;

  struct
  {
    SEARCH_PATH_DELTA sp_delta_12;
    SEARCH_PATH_DELTA sp_delta_13;
    SEARCH_PATH_DELTA sp_delta_14;
    SEARCH_PATH_DELTA sp_delta_15;
  } dw19;

  struct
  {
    SEARCH_PATH_DELTA sp_delta_16;
    SEARCH_PATH_DELTA sp_delta_17;
    SEARCH_PATH_DELTA sp_delta_18;
    SEARCH_PATH_DELTA sp_delta_19;
  } dw20;

  struct
  {
    SEARCH_PATH_DELTA sp_delta_20;
    SEARCH_PATH_DELTA sp_delta_21;
    SEARCH_PATH_DELTA sp_delta_22;
    SEARCH_PATH_DELTA sp_delta_23;
  } dw21;

  struct
  {
    SEARCH_PATH_DELTA sp_delta_24;
    SEARCH_PATH_DELTA sp_delta_25;
    SEARCH_PATH_DELTA sp_delta_26;
    SEARCH_PATH_DELTA sp_delta_27;
  } dw22;

  struct
  {
    SEARCH_PATH_DELTA sp_delta_28;
    SEARCH_PATH_DELTA sp_delta_29;
    SEARCH_PATH_DELTA sp_delta_30;
    SEARCH_PATH_DELTA sp_delta_31;
  } dw23;

  struct
  {
    SEARCH_PATH_DELTA sp_delta_32;
    SEARCH_PATH_DELTA sp_delta_33;
    SEARCH_PATH_DELTA sp_delta_34;
    SEARCH_PATH_DELTA sp_delta_35;
  } dw24;

  struct
  {
    SEARCH_PATH_DELTA sp_delta_36;
    SEARCH_PATH_DELTA sp_delta_37;
    SEARCH_PATH_DELTA sp_delta_38;
    SEARCH_PATH_DELTA sp_delta_39;
  } dw25;
  struct
  {
    SEARCH_PATH_DELTA sp_delta_40;
    SEARCH_PATH_DELTA sp_delta_41;
    SEARCH_PATH_DELTA sp_delta_42;
    SEARCH_PATH_DELTA sp_delta_43;
  } dw26;

  struct
  {
    SEARCH_PATH_DELTA sp_delta_44;
    SEARCH_PATH_DELTA sp_delta_45;
    SEARCH_PATH_DELTA sp_delta_46;
    SEARCH_PATH_DELTA sp_delta_47;
  } dw27;

  struct
  {
    SEARCH_PATH_DELTA sp_delta_48;
    SEARCH_PATH_DELTA sp_delta_49;
    SEARCH_PATH_DELTA sp_delta_50;
    SEARCH_PATH_DELTA sp_delta_51;
  } dw28;

  struct
  {
    SEARCH_PATH_DELTA sp_delta_52;
    SEARCH_PATH_DELTA sp_delta_53;
    SEARCH_PATH_DELTA sp_delta_54;
    SEARCH_PATH_DELTA sp_delta_55;
  } dw29;
  struct
  {
    UINT reserved;

  } dw30;

  struct
  {
    UINT reserved;

  } dw31;

  struct
  {
    UINT mv_data_surf:32;
  } dw32;
  struct
  {
    UINT mv_data_inp_surf:32;
  } dw33;

  struct
  {
    UINT dist_surf:32;
  } dw34;

  struct
  {
    UINT min_dist_brc_surf:32;
  } dw35;

  struct
  {
    UINT mb_enc_vme_interpred:32;
  } dw36;

  struct
  {
    UINT backward_ref_pic:32;
  } dw37;
  struct
  {
    UINT reserved:32;

  } dw38;
} MEDIA_CURBE_DATA_ME;
#endif
typedef struct _media_curbe_data_mbenc_p_g75
{
  // dw0
  struct
  {
    UINT frame_width:16;
    UINT frame_height:16;
  } dw0;

  // dw1
  struct
  {
    UINT frame_type:1;
    UINT motion_compensation_filter_type:2;
    UINT hme_enable:1;
    UINT hme_combine_overlap:2;
    UINT all_fractional:1;
    UINT multiple_pred_enable:1;
    UINT hme_combined_extra_su:8;
    UINT ref_ctrl:4;
    UINT enable_segmentation:1;
    UINT enable_segmentation_info_update:1;
    UINT enable_coeff_clamp:1;
    UINT multi_reference_qp_check:1;
    UINT mode_cost_enable_flag:1;
    UINT hme_coarse_shape:2;
    UINT reserved_mbz:4;
    UINT enable_debug_dumps:1;
  } dw1;
  // dw2
  struct
  {
    UINT lambda_intra_segment0:16;
    UINT lambda_inter_segment0:16;
  } dw2;

  // dw3
  struct
  {
    UINT lambda_intra_segment1:16;
    UINT lambda_inter_segment1:16;
  } dw3;

  //dw4
  struct
  {
    UINT lambda_intra_segment2:16;
    UINT lambda_inter_segment2:16;
  } dw4;

  // dw5
  struct
  {
    UINT lambda_intra_segment3:16;
    UINT lambda_inter_segment3:16;
  } dw5;
  // dw6
  struct
  {
    UINT reference_frame_sign_bias_0:8;
    UINT reference_frame_sign_bias_1:8;
    UINT reference_frame_sign_bias_2:8;
    UINT reference_frame_sign_bias_3:8;
  } dw6;
  // dw7
  struct
  {
    UINT raw_dist_threshold:16;
    UINT reserved_mbz:16;
  } dw7;
  // dw8
  struct
  {
    UINT skip_mode_enable:1;
    UINT adaptive_search_enable:1;
    UINT bidirectional_mix_disbale:1;
    UINT reserved_mbz1:2;
    UINT early_ime_success_enable:1;
    UINT reserved_mbz2:1;
    UINT transform8x8_flag_for_inter_enable:1;
    UINT reserved_mbz3:16;
    UINT early_ime_successful_stop_threshold:8;
  } dw8;

  //dw9
  struct
  {
    UINT max_num_of_motion_vectors:6;
    UINT reserved_mbz1:2;
    UINT ref_id_polarity_bits:8;
    UINT bidirectional_weight:6;
    UINT reserved_mbz2:6;
    UINT unidirection_mix_enable:1;
    UINT ref_pixel_bias_enable:1;
    UINT reserved_mbz3:2;
  } dw9;


  //dw10
  struct
  {
    UINT max_fixed_search_path_length:8;
    UINT maximum_search_path_length:8;
    UINT reserved_mbz:16;
  } dw10;

  struct
  {
    UINT source_block_size:2;
    UINT reserved_mbz1:2;
    UINT inter_mb_type_road_map:2;
    UINT source_access:1;
    UINT reference_access:1;
    UINT search_control:3;
    UINT dual_search_path_option:1;
    UINT sub_pel_mode:2;
    UINT skip_mode_type:1;
    UINT disable_field_cache_allocation:1;
    UINT process_inter_chroma_pixels_mode:1;
    UINT forward_trans_form_skip_check_enable:1;
    UINT bme_disable_for_fbr_message:1;
    UINT block_based_skip_enable:1;
    UINT inter_sad_measure_adjustment:2;
    UINT intra_sad_measure_adjustment:2;
    UINT submacro_block_subPartition_mask:6;
    UINT reserved_mbz2:1;
  } dw11;

  //dw12
  struct
  {
    UINT reserved_mbz:16;
    UINT reference_search_windows_width:8;
    UINT reference_search_windows_height:8;
  } dw12;

  // dw13
  union
  {
    struct
    {
      UINT mode_0_cost_seg_0:8;
      UINT mode_1_cost_seg_0:8;
      UINT mode_2_cost_seg_0:8;
      UINT mode_3_cost_seg_0:8;
    };
    struct
    {
      UINT mode_0_3_cost_seg0;
    };
  } dw13;

  // dw14
  union
  {
    struct
    {
      UINT mode_4_cost_seg_0:8;
      UINT mode_5_cost_seg_0:8;
      UINT mode_6_cost_seg_0:8;
      UINT mode_7_cost_seg_0:8;
    };
    struct
    {
      UINT mode_4_7_cost_seg0;

    };

  } dw14;


  //dw15
  union
  {
    struct
    {
      UINT mode_8_cost_seg_0:8;
      UINT mode_9_cost_seg_0:8;
      UINT ref_id_cost_seg_0:8;
      UINT chroma_cost_seg_0:8;
    };
    struct
    {
      UINT val;

    };

  } dw15;

  //dw16
  struct
  {
    SEARCH_PATH_DELTA sp_delta_0;
    SEARCH_PATH_DELTA sp_delta_1;
    SEARCH_PATH_DELTA sp_delta_2;
    SEARCH_PATH_DELTA sp_delta_3;
  } dw16;

  // dw17
  struct
  {
    SEARCH_PATH_DELTA sp_delta_4;
    SEARCH_PATH_DELTA sp_delta_5;
    SEARCH_PATH_DELTA sp_delta_6;
    SEARCH_PATH_DELTA sp_delta_7;
  } dw17;

  // dw18
  struct
  {
    SEARCH_PATH_DELTA sp_delta_8;
    SEARCH_PATH_DELTA sp_delta_9;
    SEARCH_PATH_DELTA sp_delta_10;
    SEARCH_PATH_DELTA sp_delta_11;
  } dw18;

  //dw19
  struct
  {
    SEARCH_PATH_DELTA sp_delta_12;
    SEARCH_PATH_DELTA sp_delta_13;
    SEARCH_PATH_DELTA sp_delta_14;
    SEARCH_PATH_DELTA sp_delta_15;
  } dw19;

  // dw20
  struct
  {
    SEARCH_PATH_DELTA sp_delta_16;
    SEARCH_PATH_DELTA sp_delta_17;
    SEARCH_PATH_DELTA sp_delta_18;
    SEARCH_PATH_DELTA sp_delta_19;
  } dw20;

  // dw21
  struct
  {
    SEARCH_PATH_DELTA sp_delta_20;
    SEARCH_PATH_DELTA sp_delta_21;
    SEARCH_PATH_DELTA sp_delta_22;
    SEARCH_PATH_DELTA sp_delta_23;
  } dw21;

  // dw22
  struct
  {
    SEARCH_PATH_DELTA sp_delta_24;
    SEARCH_PATH_DELTA sp_delta_25;
    SEARCH_PATH_DELTA sp_delta_26;
    SEARCH_PATH_DELTA sp_delta_27;
  } dw22;

  // dw23
  struct
  {
    SEARCH_PATH_DELTA sp_delta_28;
    SEARCH_PATH_DELTA sp_delta_29;
    SEARCH_PATH_DELTA sp_delta_30;
    SEARCH_PATH_DELTA sp_delta_31;
  } dw23;


  //dw24
  struct
  {
    SEARCH_PATH_DELTA sp_delta_32;
    SEARCH_PATH_DELTA sp_delta_33;
    SEARCH_PATH_DELTA sp_delta_34;
    SEARCH_PATH_DELTA sp_delta_35;
  } dw24;

  //dw25
  struct
  {
    SEARCH_PATH_DELTA sp_delta_36;
    SEARCH_PATH_DELTA sp_delta_37;
    SEARCH_PATH_DELTA sp_delta_38;
    SEARCH_PATH_DELTA sp_delta_39;
  } dw25;

  // dw26
  struct
  {
    SEARCH_PATH_DELTA sp_delta_40;
    SEARCH_PATH_DELTA sp_delta_41;
    SEARCH_PATH_DELTA sp_delta_42;
    SEARCH_PATH_DELTA sp_delta_43;
  };

  // dw27
  struct
  {
    SEARCH_PATH_DELTA sp_delta_44;
    SEARCH_PATH_DELTA sp_delta_45;
    SEARCH_PATH_DELTA sp_delta_46;
    SEARCH_PATH_DELTA sp_delta_47;
  } dw27;


  // dw28
  struct
  {
    SEARCH_PATH_DELTA sp_delta_48;
    SEARCH_PATH_DELTA sp_delta_49;
    SEARCH_PATH_DELTA sp_delta_50;
    SEARCH_PATH_DELTA sp_delta_51;
  } dw28;

  // dw29
  struct
  {
    SEARCH_PATH_DELTA sp_delta_52;
    SEARCH_PATH_DELTA sp_delta_53;
    SEARCH_PATH_DELTA sp_delta_54;
    SEARCH_PATH_DELTA sp_delta_55;
  } dw29;

  // dw30
  union
  {
    struct
    {
      UINT mv_0_cost_seg_0:8;
      UINT mv_1_cost_seg_0:8;
      UINT mv_2_cost_seg_0:8;
      UINT mv_3_cost_seg_0:8;

    };
    struct
    {
      UINT mv_cost_seg_val;
    };

  } dw30;

  // dw31
  union
  {
    struct
    {
      UINT mv_4_cost_seg_0:8;
      UINT mv_5_cost_seg_0:8;
      UINT mv_6_cost_seg_0:8;
      UINT mv_7_cost_seg_0:8;
    } dw31;
    struct
    {
      UINT mv_cost_seg_val;
    };

  } dw31;
  // dw32

  struct
  {
    UINT intra_16x16_no_dc_penalty_segment0:8;
    UINT intra_16x16_no_dc_penalty_segment1:8;
    UINT reserved_mbz1:7;
    UINT bilinear_enable:1;
    UINT reserved_mbz2:8;
  } dw32;

  // dw33
  struct
  {
    UINT hme_combine_len:16;
    UINT intra_16x16_no_dc_penalty_segment2:8;
    UINT intra_16x16_no_dc_penalty_segment3:8;
  } dw33;

  // dw34
  struct
  {
    UINT mv_ref_cost_context_0_0_0:16;
    UINT mv_ref_cost_context_0_0_1:16;
  } dw34;


  // dw35
  struct
  {
    UINT mv_ref_cost_context_0_1_0:16;
    UINT mv_ref_cost_context_0_1_1:16;
  } dw35;


  // dw36
  struct
  {
    UINT mv_ref_cost_context_0_2_0:16;
    UINT mv_ref_cost_context_0_2_1:16;
  } dw36;

  // dw37
  struct
  {
    UINT mv_ref_cost_context_0_3_0:16;
    UINT mv_ref_cost_context_0_3_1:16;
  } dw37;


  // dw38
  struct
  {
    UINT mv_ref_cost_context_1_0_0:16;
    UINT mv_ref_cost_context_1_0_1:16;
  } dw38;

  // dw39
  struct
  {
    UINT mv_ref_cost_context_1_1_0:16;
    UINT mv_ref_cost_context_1_1_1:16;
  } dw39;

  // dw40
  struct
  {
    UINT mv_ref_cost_context_1_2_0:16;
    UINT mv_ref_cost_context_1_2_1:16;
  } dw40;

  // dw41
  struct
  {
    UINT mv_ref_cost_context_1_3_0:16;
    UINT mv_ref_cost_context_1_3_1:16;
  } dw41;

  // dw42
  struct
  {
    UINT mv_ref_cost_context_2_0_0:16;
    UINT mv_ref_cost_context_2_0_1:16;
  };
  // dw43
  struct
  {
    UINT mv_ref_cost_context_2_1_0:16;
    UINT mv_ref_cost_context_2_1_1:16;
  };

  // dw44
  struct
  {
    UINT mv_ref_cost_context_2_2_0:16;
    UINT mv_ref_cost_context_2_2_1:16;
  } dw44;

  // dw45
  struct
  {
    UINT mv_ref_cost_context_2_3_0:16;
    UINT mv_ref_cost_context_2_3_1:16;
  } dw45;

  // dw46
  struct
  {
    UINT mv_ref_cost_context_3_0_0:16;
    UINT mv_ref_cost_context_3_0_1:16;
  } dw46;
  // dw47
  struct
  {
    UINT mv_ref_cost_context_3_1_0:16;
    UINT mv_ref_cost_context_3_1_1:16;
  } dw47;

  // dw48
  struct
  {
    UINT mv_ref_cost_context_3_2_0:16;
    UINT mv_ref_cost_context_3_2_1:16;
  } dw48;

  // dw49
  struct
  {
    UINT mv_ref_cost_context_3_3_0:16;
    UINT mv_ref_cost_context_3_3_1:16;
  } dw49;

  struct
  {
    UINT mv_ref_cost_context_4_0_0:16;
    UINT mv_ref_cost_context_4_0_1:16;
  } dw50;

  // dw51
  struct
  {
    UINT mv_ref_cost_context_4_1_0:16;
    UINT mv_ref_cost_context_4_1_1:16;
  } dw51;

  // dw52
  struct
  {
    UINT mv_ref_cost_context_4_2_0:16;
    UINT mv_ref_cost_context_4_2_1:16;
  };

  // dw53
  struct
  {
    UINT mv_ref_cost_context_4_3_0:16;
    UINT mv_ref_cost_context_4_3_1:16;
  };

  // dw54
  struct
  {
    UINT mv_ref_cost_context_5_0_0:16;
    UINT mv_ref_cost_context_5_0_1:16;
  };

  // dw55
  struct
  {
    UINT mv_ref_cost_context_5_1_0:16;
    UINT mv_ref_cost_context_5_1_1:16;
  } dw55;

  // dw56
  struct
  {
    UINT mv_ref_cost_context_5_2_0:16;
    UINT mv_ref_cost_context_5_2_1:16;
  } dw56;

  // dw57
  struct
  {
    UINT mv_ref_cost_context_5_3_0:16;
    UINT mv_ref_cost_context_5_3_1:16;
  } dw57;

  // dw58
  struct
  {
    UINT enc_cost_16x16:16;
    UINT enc_cost_16x8:16;
  } dw58;

  // dw59
  struct
  {
    UINT enc_cost_8x8:16;
    UINT enc_cost_4x4:16;
  } dw59;

  // dw60
  struct
  {
    UINT frame_count_probability_ref_frame_cost_0:16;
    UINT frame_count_probability_ref_frame_cost_1:16;
  } dw60;

  // dw61
  struct
  {
    UINT frame_count_probability_ref_frame_cost_2:16;
    UINT frame_count_probability_ref_frame_cost_3:16;
  } dw61;

  // dw62
  struct
  {
    UINT average_qp_of_last_ref_frame:8;
    UINT average_qp_of_gold_ref_frame:8;
    UINT average_qp_of_alt_ref_frame:8;
    UINT reserved_mbz:8;
  } dw62;

  // dw63
  struct
  {
    UINT intra_4x4_no_dc_penalty_segment0:8;
    UINT intra_4x4_no_dc_penalty_segment1:8;
    UINT intra_4x4_no_dc_penalty_segment2:8;
    UINT intra_4x4_no_dc_penalty_segment3:8;
  } dw63;

  // dw64
  union
  {
    struct
    {
      UINT mode_0_cost_segment1:8;
      UINT mode_1_cost_segment1:8;
      UINT mode_2_cost_segment1:8;
      UINT mode_3_cost_segment1:8;
    };
    struct
    {
      UINT mode_cost_seg_1_val;

    };
  } dw64;

  //dw65
  union
  {
    struct
    {
      UINT mode_4_cost_segment1:8;
      UINT mode_5_cost_segment1:8;
      UINT mode_6_cost_segment1:8;
      UINT mode_7_cost_segment1:8;
    };
    struct
    {
      UINT mode_cost_seg_1_val;

    };
  } dw65;

  // dw66
  union
  {
    struct
    {
      UINT mode_8_cost_segment1:8;
      UINT mode_9_cost_segment1:8;
      UINT ref_id_cost_segment1:8;
      UINT chroma_cost_sSegment1:8;
    };

    struct
    {
      UINT mode_cost_seg_1_val;

    };
  } dw66;


  // dw67
  union
  {
    struct
    {
      UINT mv_0_cost_segment1:8;
      UINT mv_1_cost_segment1:8;
      UINT mv_2_cost_segment1:8;
      UINT mv_3_cost_segment1:8;
    };
    struct
    {
      UINT mv_cost_seg1_val;
    };

  } dw67;

  // dw68
  union
  {
    struct
    {
      UINT mv_4_cost_segment1:8;
      UINT mv_5_cost_segment1:8;
      UINT mv_6_cost_segment1:8;
      UINT mv_7_cost_segment1:8;
    };

    struct
    {
      UINT mv_cost_seg1_val;
    };
  } dw68;

  // dw69
  union
  {
    struct
    {
      UINT mode_0_cost_segment2:8;
      UINT mode_1_cost_segment2:8;
      UINT mode_2_cost_segment2:8;
      UINT mode_3_cost_segment2:8;
    };
    struct
    {
      UINT mv_cost_seg2_val;
    };

  } dw69;

  // dw70
  union
  {
    struct
    {
      UINT mode_4_cost_segment2:8;
      UINT mode_5_cost_segment2:8;
      UINT mode_6_cost_segment2:8;
      UINT mode_7_cost_segment2:8;
    };

    struct
    {
      UINT mv_cost_seg2_val;

    };
  } dw70;

  // dw71
  union
  {
    struct
    {
      UINT mode_8_cost_segment2:8;
      UINT mode_9_cost_segment2:8;
      UINT ref_id_cost_segment2:8;
      UINT chroma_cost_segment2:8;
    };
    struct
    {
      UINT mv_cost_seg2_val;
    };
  } dw71;

  // dw72
  union
  {
    struct
    {
      UINT mv_0_cost_segment2:8;
      UINT mv_1_cost_segment2:8;
      UINT mv_2_cost_segment2:8;
      UINT mv_3_cost_segment2:8;
    };
    struct
    {
      UINT mv_cost_seg2;
    };
  } dw72;

  // dw73
  union
  {
    struct
    {
      UINT mv_4_cost_segment2:8;
      UINT mv_5_cost_segment2:8;
      UINT mv_6_cost_segment2:8;
      UINT mv_7_cost_segment2:8;
    };
    struct
    {
      UINT mv_cost_seg2;
    };
  } dw73;
  // dw74
  union
  {
    struct
    {
      UINT mode_0_cost_segment3:8;
      UINT mode_1_cost_segment3:8;
      UINT mode_2_cost_segment3:8;
      UINT mode_3_cost_segment3:8;
    };
    struct
    {
      UINT mode_cost_seg3;

    };
  } dw74;
  // dw75
  union
  {
    struct
    {
      UINT mode_4_costSegment3:8;
      UINT mode_5_costSegment3:8;
      UINT mode_6_costSegment3:8;
      UINT mode_7_costSegment3:8;
    };
    struct
    {
      UINT mode_cost_seg3;
    };
  } dw75;


  // dw76
  union
  {
    struct
    {
      UINT mode_8_cost_segment3:8;
      UINT mode_9_cost_segment3:8;
      UINT ref_id_cost_segment3:8;
      UINT chroma_cost_segment3:8;

    };
    struct
    {
      UINT mode_cost_seg3;
    };
  } dw76;

  // dw77
  union
  {
    struct
    {
      UINT mv_0_cost_segment3:8;
      UINT mv_1_cost_segment3:8;
      UINT mv_2_cost_segment3:8;
      UINT mv_3_cost_segment3:8;
    };
    struct
    {
      UINT mv_cost_seg3;
    };
  } dw77;


  // dw78
  union
  {
    struct
    {
      UINT mv_4_cost_segment3:8;
      UINT mv_5_cost_segment3:8;
      UINT mv_6_cost_segment3:8;
      UINT mv_7_cost_segmant3:8;
    };
    struct
    {
      UINT mv_cost_seg3;
    };
  } dw78;
  // dw79
  struct
  {
    UINT new_mv_skip_threshold_segment0:16;
    UINT new_mv_skip_threshold_segment1:16;
  } dw79;

  // dw80
  struct
  {
    UINT new_mv_skip_threshold_segment2:16;
    UINT new_mv_skip_threshold_segment3:16;
  } dw80;

  // dw81
  struct
  {
    UINT per_mb_output_data_surface_bti:32;
  } dw81;

  // dw82
  struct
  {
    UINT current_picture_y_surface_bti:32;
  } dw82;

  // dw83
  struct
  {
    UINT current_picture_interleaved_uv_surface_bti:32;
  } dw83;

  // dw84
  struct
  {
    UINT hme_mv_data_surface_bti:32;
  } dw84;

  // dw85
  struct
  {
    UINT mv_data_surface_bti:32;
  } dw85;

  // dw86
  struct
  {
    UINT mb_count_per_reference_frame_bti:32;
  } dw86;

  // dw87
  struct
  {
    UINT vme_inter_prediction_bti:32;
  } dw87;

  // dw88
  struct
  {
    UINT last_picture_bti:32;
  } dw88;
  // dw89
  struct
  {
    UINT gold_picture_bti:32;
  } dw89;

  // dw90
  struct
  {
    UINT alternate_picture_bti:32;
  } dw90;

  // dw91
  struct
  {
    UINT per_mb_quant_data_bti:32;
  } dw91;

  // dw92
  struct
  {
    UINT segment_map_bti:32;
  } dw92;

  // dw93
  struct
  {
    UINT inter_prediction_distortion_bti:32;
  } dw93;
  // dw94
  struct
  {
    UINT histogram_bti:32;
  } dw94;

  // dw95
  struct
  {
    UINT pred_mv_data_bti:32;
  } dw95;

  // dw96
  struct
  {
    UINT mode_cost_update_bti:32;
  } dw96;
  // dw97
  struct
  {
    UINT kernel_debug_dump_bti:32;
  } dw97;
} MEDIA_CURBE_DATA_MBENC_P_G75;
typedef struct surface_state_g7
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
    unsigned int surface_array_spacing:1;
    unsigned int vert_line_stride_offset:1;
    unsigned int vert_line_stride:1;
    unsigned int tile_walk:1;
    unsigned int tiled_surface:1;
    unsigned int horizontal_alignment:1;
    unsigned int vertical_alignment:2;
    unsigned int surface_format:9;	   /**< BRW_SURFACEFORMAT_x */
    unsigned int min_mag_state_not_eq:1;
    unsigned int surface_array:1;
    unsigned int surface_type:3;	   /**< BRW_SURFACE_1D/2D/3D/CUBE */
    /*};
       struct {
       unsigned int val;
       }; */
  } dw0;
/*union {*/
  struct
  {
    unsigned int base_addr;
/*  };
 struct {
   unsigned int val;
    };*/
  } dw1;

/*union {*/
  struct
  {
    unsigned int width:14;
    unsigned int:2;
    unsigned int height:14;
    unsigned int:2;
/*};
 struct {
   unsigned int val;
    };*/
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
    unsigned int reserved0:8;
    unsigned int obj_ctrl_state:4;
    unsigned int y_offset:4;
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
    unsigned int mcs_enable:1;
    unsigned int:2;
    unsigned int mcs_surface_picth:9;
    unsigned int mcs_base_addr:20;
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
} SURFACE_STATE_G7;


typedef struct _binding_table_state
{
  //dw0
  struct
  {
    unsigned int enable:1;
    unsigned int copy:1;
    unsigned int binding_table_state_type:1;
    unsigned int:2;
    unsigned int surface_state_pointer:27;
  } dw0;

} BINDING_TABLE_STATE;

VOID media_add_binding_table (MEDIA_GPE_CTX * gpe_ctx);
VOID media_interface_setup_mbenc (MEDIA_ENCODER_CTX * encoder_context);

typedef struct _media_curbe_data_brc_init_reset_g75
{
  struct {
    UINT profile_level_max_frame; /* in bytes */
  } dw0;

  struct {
    UINT init_buf_full_in_bits;
  } dw1;

  struct {
    UINT buf_size_in_bits;
  } dw2;

  struct {
    UINT average_bit_rate;
  } dw3;

  struct {
    UINT max_bit_rate;
  } dw4;

  struct {
    UINT min_bit_rate;
  } dw5;

  struct {
    UINT frame_rate_m;
  } dw6;

  struct {
    UINT frame_rate_d;
  } dw7;

  struct {
    UINT brc_flag:16;
    UINT number_pframes_in_gop:16;
  } dw8;

  struct {
    UINT constant_0:16;
    UINT frame_width:16; /* in bytes */
  } dw9;

  struct {
    UINT frame_height:16; /* in bytes */
    UINT avbr_accuracy:16;
  } dw10;

  struct {
    UINT avbr_convergence:16;
    UINT min_qp:16;
  } dw11;

  struct {
    UINT max_qp:16;
    UINT level_qp:16;
  } dw12;

  struct {
    UINT max_section_pct:16;
    UINT under_shoot_cbr_pct:16;
  } dw13;

  struct {
    UINT vbr_bias_pct:16;
    UINT min_section_pct:16;
  } dw14;

  struct {
    UINT instant_rate_threshold0_pframe:8;
    UINT instant_rate_threshold1_pframe:8;
    UINT instant_rate_threshold2_pframe:8;
    UINT instant_rate_threshold3_pframe:8;
  } dw15;

  struct {
    UINT constant_0:8;
    UINT constant_1:8;
    UINT constant_2:8;
    UINT constant_3:8;
  } dw16;

  struct {
    UINT instant_rate_threshold0_iframe:8;
    UINT instant_rate_threshold1_iframe:8;
    UINT instant_rate_threshold2_iframe:8;
    UINT instant_rate_threshold3_iframe:8;
  } dw17;

  struct {
    UINT deviation_threshold0_pframe:8;      // signed byte
    UINT deviation_threshold1_pframe:8;      // signed byte
    UINT deviation_threshold2_pframe:8;      // signed byte
    UINT deviation_threshold3_pframe:8;      // signed byte
  } dw18;

  struct {
    UINT deviation_threshold4_pframe:8;      // signed byte
    UINT deviation_threshold5_pframe:8;      // signed byte
    UINT deviation_threshold6_pframe:8;      // signed byte
    UINT deviation_threshold7_pframe:8;      // signed byte
  } dw19;

  struct {
    UINT deviation_threshold0_vbr:8;         // signed byte
    UINT deviation_threshold1_vbr:8;         // signed byte
    UINT deviation_threshold2_vbr:8;         // signed byte
    UINT deviation_threshold3_vbr:8;         // signed byte
  } dw20;

  struct {
    UINT deviation_threshold4_vbr:8;         // signed byte
    UINT deviation_threshold5_vbr:8;         // signed byte
    UINT deviation_threshold6_vbr:8;         // signed byte
    UINT deviation_threshold7_vbr:8;         // signed byte
  } dw21;

  struct {
    UINT deviation_threshold0_iframe:8;      // signed byte
    UINT deviation_threshold1_iframe:8;      // signed byte
    UINT deviation_threshold2_iframe:8;      // signed byte
    UINT deviation_threshold3_iframe:8;      // signed byte
  } dw22;

  struct {
    UINT deviation_threshold4_iframe:8;      // signed byte
    UINT deviation_threshold5_iframe:8;      // signed byte
    UINT deviation_threshold6_iframe:8;      // signed byte
    UINT deviation_threshold7_iframe:8;      // signed byte
  } dw23;

  struct {
    UINT initial_qp_iframe:8;
    UINT initial_qp_pframe:8;
    UINT pad0:8;
    UINT pad1:8;
  } dw24;

  struct {
    UINT history_buffer_bti;
  } dw25;

  struct {
    UINT distortion_buffer_bti;
  } dw26;
} MEDIA_CURBE_DATA_BRC_INIT_RESET_G75;

typedef struct _media_curbe_data_brc_update_g75
{
  struct {
    UINT target_size;
  } dw0;

  struct {
    UINT frame_number;
  } dw1;

  struct {
    UINT picture_header_size:32;
  } dw2;

  struct {
    UINT start_global_adjust_frame0:16;
    UINT start_global_adjust_frame1:16;
  } dw3;

  struct {
    UINT start_global_adjust_frame2:16;
    UINT start_global_adjust_frame3:16;
  } dw4;

  struct {
    UINT target_size_flag:8;
    UINT brc_flag:8;
    UINT max_num_paks:8;
    UINT curr_frame_type:8;
  } dw5;

  struct {
    UINT reserved0:32;
  } dw6;

  struct {
    UINT reserved0:32;
  } dw7;

  struct {
    UINT start_global_adjust_mult0:8;
    UINT start_global_adjust_mult1:8;
    UINT start_global_adjust_mult2:8;
    UINT start_global_adjust_mult3:8;
  } dw8;

  struct {
    UINT start_global_adjust_mult4:8;
    UINT start_global_adjust_div0:8;
    UINT start_global_adjust_div1:8;
    UINT start_global_adjust_div2:8;
  } dw9;

  struct {
    UINT start_global_adjust_div3:8;
    UINT start_global_adjust_div4:8;
    UINT qp_threshold0:8;
    UINT qp_threshold1:8;
  } dw10;

  struct {
    UINT qp_threshold2:8;
    UINT qp_threshold3:8;
    UINT rate_ratio_threshold0:8;
    UINT rate_ratio_threshold1:8;
  } dw11;

  struct {
    UINT rate_ratio_threshold2:8;
    UINT rate_ratio_threshold3:8;
    UINT rate_ratio_threshold4:8;
    UINT rate_ratio_threshold5:8;
  } dw12;

  struct {
    UINT rate_ratio_threshold_qp0:8;
    UINT rate_ratio_threshold_qp1:8;
    UINT rate_ratio_threshold_qp2:8;
    UINT rate_ratio_threshold_qp3:8;
  } dw13;

  struct {
    UINT rate_ratio_threshold_qp4:8;
    UINT rate_ratio_threshold_qp5:8;
    UINT rate_ratio_threshold_qp6:8;
    UINT index_of_previous_qp:8;
  } dw14;

  struct {
    UINT frame_width_in_mb:8;
    UINT frame_height_in_mb:8;
    UINT prev_flag:8;
    UINT reserved:8;
  } dw15;

  struct {
    UINT frame_byte_count:32;
  } dw16;

  struct {
    UINT key_frame_qp_seg0:8;
    UINT key_frame_qp_seg1:8;
    UINT key_frame_qp_seg2:8;
    UINT key_frame_qp_seg3:8;
  } dw17;

  struct {
    UINT qp_delta_plane0:8;
    UINT qp_delta_plane1:8;
    UINT qp_delta_plane2:8;
    UINT qp_delta_plane3:8;
  } dw18;

  struct {
    UINT qp_delta_plane4:8;
    UINT qp:8;
    UINT reserved:16;
  } dw19;

  struct {
    UINT segmentation_enabled:8;
    UINT mb_rc:8;
    UINT brc_method:8;
    UINT vme_intraprediction:8;
  } dw20;

  struct {
    UINT history_buffer_index;
  } dw21;

  struct {
    UINT pak_surface_index;
  } dw22;

  struct {
    UINT mbpak_curbe1_index;
  } dw23;

  struct {
    UINT mbpak_curbe2_index;
  } dw24;

  struct {
    UINT mbenc_curbe_input_index;
  } dw25;

  struct {
    UINT mbenc_curbe_output_index;
  } dw26;

  struct {
    UINT distortion_input_index;
  } dw27;

  struct {
    UINT constant_data_input_index;
  } dw28;

  struct {
    UINT pak_table_surface_index;
  } dw29;

  struct {
    UINT reserved;
  } dw30;

  struct {
    UINT reserved;
  } dw31;

  struct {
    UINT reserved;
  } dw32;

  struct {
    UINT reserved;
  } dw33;
} MEDIA_CURBE_DATA_BRC_UPDATE_G75;

void media_interface_setup_mbpak (MEDIA_GPE_CTX *mbpak_gpe_ctx);
void
media_surface_state_vp8_mbpak (MEDIA_ENCODER_CTX * encoder_context,
			       struct encode_state *encode_state,
			       MBPAK_SURFACE_PARAMS_VP8 *
			       mbpak_sutface_params);
void
media_surface_state_vp8_mbenc (MEDIA_ENCODER_CTX * encoder_context,
			       struct encode_state *encode_state,
			       MBENC_SURFACE_PARAMS_VP8 *
			       mbenc_sutface_params);
void
media_set_curbe_i_vp8_mbenc (struct encode_state *encode_state,
			     MEDIA_MBENC_CURBE_PARAMS_VP8 * params);
void media_set_curbe_p_vp8_mbenc (struct encode_state *encode_state,
				  MEDIA_MBENC_CURBE_PARAMS_VP8 * params);
void media_set_curbe_vp8_me (VP8_ME_CURBE_PARAMS * params);
void
media_surface_state_vp8_me (MEDIA_ENCODER_CTX * encoder_context,
			    ME_SURFACE_PARAMS_VP8 * me_sutface_params);
void media_set_curbe_vp8_mbpak (struct encode_state *encode_state,
				MEDIA_MBPAK_CURBE_PARAMS_VP8 * params);
void media_surface_state_scaling (MEDIA_ENCODER_CTX * encoder_context,
				  SCALING_SURFACE_PARAMS *
				  scaling_sutface_params);
void
  media_encode_init_mbenc_constant_buffer_vp8_g75
  (MBENC_CONSTANT_BUFFER_PARAMS_VP8 * params);

void
media_set_curbe_vp8_brc_init_reset(struct encode_state *encode_state,
                                   MEDIA_BRC_INIT_RESET_PARAMS_VP8 * params);
VOID
media_interface_setup_brc_init_reset (MEDIA_ENCODER_CTX * encoder_context);

VOID
media_surface_state_vp8_brc_init_reset (MEDIA_ENCODER_CTX * encoder_context,
                                        struct encode_state *encode_state,
                                        BRC_INIT_RESET_SURFACE_PARAMS_VP8 *surface_params);

VOID
media_interface_setup_brc_update (MEDIA_ENCODER_CTX * encoder_context);

VOID
media_surface_state_vp8_brc_update (MEDIA_ENCODER_CTX * encoder_context,
                                    struct encode_state *encode_state,
                                    BRC_UPDATE_SURFACE_PARAMS_VP8 *surface_params);

VOID
media_set_curbe_vp8_brc_update(struct encode_state *encode_state,
                               MEDIA_BRC_UPDATE_PARAMS_VP8 * params);

VOID
media_encode_init_brc_update_constant_data_vp8_g75(BRC_UPDATE_CONSTANT_DATA_PARAMS_VP8 *params);

VOID
media_add_surface_state (SURFACE_SET_PARAMS * params);

VOID
media_interface_setup_scaling (MEDIA_ENCODER_CTX * encoder_context);

VOID
media_interface_setup_me (MEDIA_ENCODER_CTX * encoder_context);

#endif
