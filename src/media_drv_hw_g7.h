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

#ifndef _MEDIA__DRIVER_HW_G7_H
#define _MEDIA__DRIVER_HW_G7_H
#include "media_drv_hw.h"
#include "media_drv_hw_g75.h"
#define NUM_OF_VP8_KERNELS_G7 12
extern MEDIA_KERNEL media_hybrid_vp8_kernels_g7[NUM_OF_VP8_KERNELS_G7];
extern struct hw_codec_info gen7_hw_codec_info;
typedef struct _media_curbe_data_mbenc_i_g7
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

} MEDIA_CURBE_DATA_MBENC_I_G7;

typedef struct _media_curbe_data_mbenc_p_g7
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
    UINT reserved1:1;
    UINT hme_combined_extra_su:8;
    UINT ref_ctrl:4;
    UINT enable_segmentation:1;
    UINT enable_segmentation_info_update:1;
    UINT enable_coeff_clamp:1;
    UINT multi_reference_qp_check:1;
    UINT mode_cost_enable_flag:1;
    UINT multi_pred_en:2;
    UINT reserved2:4;
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
    UINT start_centre_0x:4;
    UINT start_centre_0y:4;
    UINT reserved_mbz:8;
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
    UINT submacro_block_subpartition_mask:6;
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
      UINT hme_combined_len:16;
      UINT mv_cost_scale_factor:2;
      UINT bilinear_enable:1;
      UINT reserved_mbz:13;
    };
    struct
    {
      UINT val;
    };
  } dw13;

  // dw14
  union
  {
    struct
    {
      UINT frame_count_probability_ref_frame_cost_0:16;
      UINT frame_count_probability_ref_frame_cost_1:16;
    };
    struct
    {
      UINT val;

    };

  } dw14;


  //dw15
  union
  {
    struct
    {
      UINT frame_count_probability_ref_frame_cost_2:16;
      UINT frame_count_probability_ref_frame_cost_3:16;
    };
    struct
    {
      UINT val;

    };

  } dw15;

  //dw16
  struct
  {
    UINT mv_ref_cost_context_0_0_0:16;
    UINT mv_ref_cost_context_0_0_1:16;
  } dw16;

  // dw17
  struct
  {
    UINT mv_ref_cost_context_0_1_0:16;
    UINT mv_ref_cost_context_0_1_1:16;
  } dw17;

  // dw18
  struct
  {
    UINT mv_ref_cost_context_0_2_0:16;
    UINT mv_ref_cost_context_0_2_1:16;
  } dw18;

  //dw19
  struct
  {
    UINT mv_ref_cost_context_0_3_0:16;
    UINT mv_ref_cost_context_0_3_1:16;
  } dw19;

  // dw20
  struct
  {
    UINT mv_ref_cost_context_1_0_0:16;
    UINT mv_ref_cost_context_1_0_1:16;
  } dw20;

  // dw21
  struct
  {
    UINT mv_ref_cost_context_1_1_0:16;
    UINT mv_ref_cost_context_1_1_1:16;
  } dw21;

  // dw22
  struct
  {
    UINT mv_ref_cost_context_1_2_0:16;
    UINT mv_ref_cost_context_1_2_1:16;
  } dw22;

  // dw23
  struct
  {
    UINT mv_ref_cost_context_1_3_0:16;
    UINT mv_ref_cost_context_1_3_1:16;
  } dw23;


  //dw24
  struct
  {
    UINT mv_ref_cost_context_2_0_0:16;
    UINT mv_ref_cost_context_2_0_1:16;
  } dw24;

  //dw25
  struct
  {
    UINT mv_ref_cost_context_2_1_0:16;
    UINT mv_ref_cost_context_2_1_1:16;
  } dw25;

  // dw26
  struct
  {
    UINT mv_ref_cost_context_2_2_0:16;
    UINT mv_ref_cost_context_2_2_1:16;
  } dw26;

  // dw27
  struct
  {
    UINT mv_ref_cost_context_2_3_0:16;
    UINT mv_ref_cost_context_2_3_1:16;
  } dw27;


  // dw28
  struct
  {
    UINT mv_ref_cost_context_3_0_0:16;
    UINT mv_ref_cost_context_3_0_1:16;
  } dw28;

  // dw29
  struct
  {
    UINT mv_ref_cost_context_3_1_0:16;
    UINT mv_ref_cost_context_3_1_1:16;
  } dw29;

  // dw30
  union
  {
    struct
    {
      UINT mv_ref_cost_context_3_2_0:16;
      UINT mv_ref_cost_context_3_2_1:16;
    };
    struct
    {
      UINT val;
    };

  } dw30;

  // dw31
  struct
  {
    UINT mv_ref_cost_context_3_3_0:16;
    UINT mv_ref_cost_context_3_3_1:16;
  } dw31;
  // dw32

  struct
  {
    UINT mv_ref_cost_context_4_0_0:16;
    UINT mv_ref_cost_context_4_0_1:16;
  } dw32;

  // dw33
  struct
  {
    UINT mv_ref_cost_context_4_1_0:16;
    UINT mv_ref_cost_context_4_1_1:16;
  } dw33;

  // dw34
  struct
  {
    UINT mv_ref_cost_context_4_2_0:16;
    UINT mv_ref_cost_context_4_2_1:16;
  } dw34;


  // dw35
  struct
  {
    UINT mv_ref_cost_context_4_3_0:16;
    UINT mv_ref_cost_context_4_3_1:16;
  } dw35;


  // dw36
  struct
  {
    UINT mv_ref_cost_context_5_0_0:16;
    UINT mv_ref_cost_context_5_0_1:16;
  } dw36;

  // dw37
  struct
  {
    UINT mv_ref_cost_context_5_1_0:16;
    UINT mv_ref_cost_context_5_1_1:16;
  } dw37;


  // dw38
  struct
  {
    UINT mv_ref_cost_context_5_2_0:16;
    UINT mv_ref_cost_context_5_2_1:16;
  } dw38;

  // dw39
  struct
  {
    UINT mv_ref_cost_context_5_3_0:16;
    UINT mv_ref_cost_context_5_3_1:16;
  } dw39;

  // dw40
  struct
  {
    UINT average_qp_of_last_ref_frame:8;
    UINT average_qp_of_gold_ref_frame:8;
    UINT average_qp_of_alt_ref_frame:8;
    UINT reserved_mbz:8;

  } dw40;

  // dw41
  struct
  {
    UINT mv_skip_threshold0:16;
    UINT mv_skip_threshold1:16;
  } dw41;

  // dw42
  struct
  {
    UINT mv_skip_threshold2:16;
    UINT mv_skip_threshold3:16;
  } dw42;
  // dw43
  struct
  {
    UINT intra_16x16_no_dc_penalty_segment0:8;
    UINT intra_16x16_no_dc_penalty_segment1:8;
    UINT intra_16x16_no_dc_penalty_segment2:8;
    UINT intra_16x16_no_dc_penalty_segment3:8;
  } dw43;

  // dw44
  struct
  {
    UINT intra_4x4_no_dc_penalty_segment0:8;
    UINT intra_4x4_no_dc_penalty_segment1:8;
    UINT intra_4x4_no_dc_penalty_segment2:8;
    UINT intra_4x4_no_dc_penalty_segment3:8;
  } dw44;

  // dw45
  struct
  {
    UINT reserved_1:8;
    UINT reserved_2:8;
    UINT reserved_3:8;
    UINT reserved_4:8;
  } dw45;

  // dw46
  struct
  {
    UINT qp_index_seg0:8;
    UINT qp_index_seg1:8;
    UINT qp_index_seg2:8;
    UINT qp_index_seg3:8;

  } dw46;
  // dw47
  struct
  {
    UINT reserved:32;
  } dw47;

  // dw48
  struct
  {
    UINT output_data_surface_bti:32;
  } dw48;

  // dw49
  struct
  {
    UINT current_pic_y_surface_bti:32;
  } dw49;

  struct
  {
    UINT current_pic_uv_surface_bti:32;
  } dw50;

  // dw51
  struct
  {
    UINT hme_data_surface_bti:32;
  } dw51;

  // dw52
  struct
  {
    UINT mv_data_surface_bti:32;
  } dw52;

  // dw53
  struct
  {
    UINT seg_map_bti:32;
  } dw53;

  // dw54
  struct
  {
    UINT inter_pred_dis_bti:32;
  } dw54;

  // dw55
  struct
  {
    UINT mode_cost_update_bti:32;
  } dw55;

  // dw56
  struct
  {
    UINT near_cnt_bti:32;
  } dw56;

  // dw57
  struct
  {
    UINT cnt_index_2spindex_bti:32;
  } dw57;

  // dw58
  struct
  {
    UINT vme_inter_pred_last_ref_frame_bti:32;
  } dw58;

  // dw59
  struct
  {
    UINT vme_inter_pred_gold_ref_frame_bti:32;
  } dw59;

  // dw60
  struct
  {
    UINT vme_inter_pred_alt_ref_frame_bti:32;
  } dw60;

  // dw61
  struct
  {
    UINT vme_spl_ut0:32;
  } dw61;

  // dw62
  struct
  {
    UINT vme_spl_ut1:32;
  } dw62;

  // dw63
  struct
  {
    UINT vme_spl_ut2:32;
  } dw63;

  // dw64

  struct
  {
    UINT vme_spl_ut3:32;
  } dw64;

  //dw65
  struct
  {
    UINT vme_spl_ut4:32;
  } dw65;

  // dw66
  struct
  {
    UINT vme_spl_ut5:32;
  } dw66;
  // dw67
  struct
  {
    UINT vme_spl_ut6:32;
  } dw67;

  // dw68
  struct
  {
    UINT vme_spl_ut7:32;
  } dw68;

  // dw69
  struct
  {
    UINT kernel_debug_dump_bti:32;
  } dw69;
} MEDIA_CURBE_DATA_MBENC_P_G7;

typedef struct _MEDIA_CURBE_DATA_ME_G7
{
  struct {
    UINT skip_mode_enable:1;
    UINT adaptive_enable:1;
    UINT bi_mix_disable:1;
    UINT partition_candidates_enable:1;
    UINT early_success_enable:1;
    UINT early_ime_success_enable:1;
    UINT quit_inter_search_enable:1;
    UINT t8x8_flag_for_inter_enable:1;
    UINT early_skip_success_threshold:8;
    UINT early_fme_success_threshold:8;
    UINT early_ime_stop:8;
  } dw0;

  struct {
    UINT max_num_mvs:6;
    UINT reserved0:10;
    UINT bi_weight:6;
    UINT reserved1:2;
    UINT bi_sub_mb_part_mask:4;
    UINT uni_mix_disable:1;
    UINT adaptiv_validation_ctrl:1;
    UINT fb_prun_enable:1;
    UINT repart_enable:1;
  } dw1;

  struct {
    UINT max_len_sp:8;
    UINT max_num_su:8;
    UINT reserved0:16;
  } dw2;

  struct {
    UINT fb_prun_threshold:8;
    UINT part_tolerance_threshold:8;
    UINT ime_too_bad:8;
    UINT ime_too_good:8;
  } dw3;

  struct {
    UINT reserved0:8;
    UINT picture_height_minus1:8;
    UINT picture_width:8;
    UINT reserved1:8;
  } dw4;

  struct {
    UINT src_size:2;
    UINT reserved0:2;
    UINT mbtyperemap:2;
    UINT src_access:1;
    UINT ref_access:1;
    UINT search_ctrl:3;
    UINT dual_search_path_option:1;
    UINT subpel_mode:2;
    UINT skip_type:1;
    UINT disable_field_cache_alloc:1;
    UINT aligned_vme_ref_fetch_disable:1;
    UINT aligned_vme_src_fetch_disable:1;
    UINT boundary_check_skip_enable:1;
    UINT block_based_skip_enable:1;
    UINT inter_sad:2;
    UINT intra_sad:2;
    UINT sub_mb_part_mask:7;
    UINT reserved1:1;
  } dw5;

  struct {
    UINT reserved0:8;
    UINT qp_primey:8;
    UINT ref_width:8;
    UINT ref_height:8;
  } dw6;

  struct {
    UINT mv_cost_scale_factor:2;
    UINT bilinear_enable:1;
    UINT me_modes:2;
    UINT reserved0:11;
    UINT max_vmvr:16;
  } dw7;
} MEDIA_CURBE_DATA_ME_G7;

VOID
media_set_curbe_i_vp8_mbenc_g7 (struct encode_state *encode_state,
				MEDIA_MBENC_CURBE_PARAMS_VP8 * params);
VOID
media_set_curbe_p_vp8_mbenc_g7 (struct encode_state *encode_state,
				MEDIA_MBENC_CURBE_PARAMS_VP8 * params);
VOID
media_surface_state_vp8_mbenc_g7 (MEDIA_ENCODER_CTX * encoder_context,
				  struct encode_state *encode_state,
				  MBENC_SURFACE_PARAMS_VP8 *
				  mbenc_sutface_params);

VOID
media_surface_state_vp8_mbpak_g7 (MEDIA_ENCODER_CTX * encoder_context,
				  struct encode_state *encode_state,
				  MBPAK_SURFACE_PARAMS_VP8 *
				  mbpak_sutface_params);
VOID
media_set_curbe_vp8_mbpak_g7 (struct encode_state *encode_state,
			      MEDIA_MBPAK_CURBE_PARAMS_VP8 * params);

VOID
media_sampler_setup_mbenc_g7 (MEDIA_ENCODER_CTX * encoder_context);
VOID
media_set_curbe_vp8_brc_init_reset_g7(struct encode_state *encode_state,
				      MEDIA_BRC_INIT_RESET_PARAMS_VP8 * params);
VOID
media_surface_state_vp8_brc_init_reset_g7(MEDIA_ENCODER_CTX * encoder_context,
					  struct encode_state *encode_state,
					  BRC_INIT_RESET_SURFACE_PARAMS_VP8 *surface_params);
VOID
media_set_curbe_vp8_brc_update_g7(struct encode_state *encode_state,
				  MEDIA_BRC_UPDATE_PARAMS_VP8 * params);
VOID
media_surface_state_vp8_brc_update_g7(MEDIA_ENCODER_CTX * encoder_context,
				      struct encode_state *encode_state,
				      BRC_UPDATE_SURFACE_PARAMS_VP8 *surface_params);
VOID
media_encode_init_brc_update_constant_data_vp8_g7(BRC_UPDATE_CONSTANT_DATA_PARAMS_VP8 *params);

VOID
media_set_curbe_vp8_me_g7 (VP8_ME_CURBE_PARAMS * params);

VOID
media_surface_state_vp8_me_g7 (MEDIA_ENCODER_CTX * encoder_context,
			       struct encode_state *encode_state,
			       ME_SURFACE_PARAMS_VP8 * me_sutface_params);

VOID
media_sampler_setup_me_g7 (MEDIA_ENCODER_CTX * encoder_context);

VOID
media_hw_context_init_g7(VADriverContextP ctx, MEDIA_HW_CONTEXT *hw_ctx);

VOID
media_init_brc_distortion_buffer_g7 (MEDIA_ENCODER_CTX * encoder_context);

#endif
