#ifndef _MEDIA_RENDER_COMMON_H_
#define _MEDIA_RENDER_COMMON_H_

#include "object_heap.h"
#ifdef __cplusplus
extern "C"
{
#endif

#include <drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>


#ifdef __cplusplus
}
#endif

#include <va/va.h>
#include <va/va_backend.h>

#define MAX_SAMPLERS         16
#define MAX_RENDER_SURFACES  17

struct i965_sampler_state
{
  struct {
    unsigned int shadow_function:3;
    unsigned int lod_bias:11;
    unsigned int min_filter:3;
    unsigned int mag_filter:3;
    unsigned int mip_filter:2;
    unsigned int base_level:5;
    unsigned int pad:1;
    unsigned int lod_preclamp:1;
    unsigned int border_color_mode:1;
    unsigned int pad0:1;
    unsigned int disable:1;
  } ss0;

  struct {
    unsigned int r_wrap_mode:3;
    unsigned int t_wrap_mode:3;
    unsigned int s_wrap_mode:3;
    unsigned int pad:3;
    unsigned int max_lod:10;
    unsigned int min_lod:10;
  } ss1;

  struct {
    unsigned int pad:5;
    unsigned int border_color_pointer:27;
  } ss2;

  struct {
    unsigned int pad:19;
    unsigned int max_aniso:3;
    unsigned int chroma_key_mode:1;
    unsigned int chroma_key_index:2;
    unsigned int chroma_key_enable:1;
    unsigned int monochrome_filter_width:3;
    unsigned int monochrome_filter_height:3;
  } ss3;
};

struct i965_cc_viewport
{
  float min_depth;
  float max_depth;
};

struct i965_cc_unit_state
{
  struct {
    unsigned int pad0:3;
    unsigned int bf_stencil_pass_depth_pass_op:3;
    unsigned int bf_stencil_pass_depth_fail_op:3;
    unsigned int bf_stencil_fail_op:3;
    unsigned int bf_stencil_func:3;
    unsigned int bf_stencil_enable:1;
    unsigned int pad1:2;
    unsigned int stencil_write_enable:1;
    unsigned int stencil_pass_depth_pass_op:3;
    unsigned int stencil_pass_depth_fail_op:3;
    unsigned int stencil_fail_op:3;
    unsigned int stencil_func:3;
    unsigned int stencil_enable:1;
  } cc0;

  struct {
    unsigned int bf_stencil_ref:8;
    unsigned int stencil_write_mask:8;
    unsigned int stencil_test_mask:8;
    unsigned int stencil_ref:8;
  } cc1;

  struct {
    unsigned int logicop_enable:1;
    unsigned int pad0:10;
    unsigned int depth_write_enable:1;
    unsigned int depth_test_function:3;
    unsigned int depth_test:1;
    unsigned int bf_stencil_write_mask:8;
    unsigned int bf_stencil_test_mask:8;
  } cc2;

  struct {
    unsigned int pad0:8;
    unsigned int alpha_test_func:3;
    unsigned int alpha_test:1;
    unsigned int blend_enable:1;
    unsigned int ia_blend_enable:1;
    unsigned int pad1:1;
    unsigned int alpha_test_format:1;
    unsigned int pad2:16;
  } cc3;

  struct {
    unsigned int pad0:5;
    unsigned int cc_viewport_state_offset:27;
  } cc4;

  struct {
    unsigned int pad0:2;
    unsigned int ia_dest_blend_factor:5;
    unsigned int ia_src_blend_factor:5;
    unsigned int ia_blend_function:3;
    unsigned int statistics_enable:1;
    unsigned int logicop_func:4;
    unsigned int pad1:11;
    unsigned int dither_enable:1;
  } cc5;

  struct {
    unsigned int clamp_post_alpha_blend:1;
    unsigned int clamp_pre_alpha_blend:1;
    unsigned int clamp_range:2;
    unsigned int pad0:11;
    unsigned int y_dither_offset:2;
    unsigned int x_dither_offset:2;
    unsigned int dest_blend_factor:5;
    unsigned int src_blend_factor:5;
    unsigned int blend_function:3;
  } cc6;

  struct {
    union {
      float f;
      unsigned char ub[4];
    } alpha_ref;
  } cc7;
};

struct gen6_blend_state
{
  struct {
    unsigned int dest_blend_factor:5;
    unsigned int source_blend_factor:5;
    unsigned int pad3:1;
    unsigned int blend_func:3;
    unsigned int pad2:1;
    unsigned int ia_dest_blend_factor:5;
    unsigned int ia_source_blend_factor:5;
    unsigned int pad1:1;
    unsigned int ia_blend_func:3;
    unsigned int pad0:1;
    unsigned int ia_blend_enable:1;
    unsigned int blend_enable:1;
  } blend0;

  struct {
    unsigned int post_blend_clamp_enable:1;
    unsigned int pre_blend_clamp_enable:1;
    unsigned int clamp_range:2;
    unsigned int pad0:4;
    unsigned int x_dither_offset:2;
    unsigned int y_dither_offset:2;
    unsigned int dither_enable:1;
    unsigned int alpha_test_func:3;
    unsigned int alpha_test_enable:1;
    unsigned int pad1:1;
    unsigned int logic_op_func:4;
    unsigned int logic_op_enable:1;
    unsigned int pad2:1;
    unsigned int write_disable_b:1;
    unsigned int write_disable_g:1;
    unsigned int write_disable_r:1;
    unsigned int write_disable_a:1;
    unsigned int pad3:1;
    unsigned int alpha_to_coverage_dither:1;
    unsigned int alpha_to_one:1;
    unsigned int alpha_to_coverage:1;
  } blend1;
};

struct gen6_color_calc_state
{
    struct {
        unsigned int alpha_test_format:1;
        unsigned int pad0:14;
        unsigned int round_disable:1;
        unsigned int bf_stencil_ref:8;
        unsigned int stencil_ref:8;
    } cc0;

    union {
        float alpha_ref_f;
        struct {
            unsigned int ui:8;
            unsigned int pad0:24;
        } alpha_ref_fi;
    } cc1;

    float constant_r;
    float constant_g;
    float constant_b;
    float constant_a;
};

struct gen6_depth_stencil_state
{
    struct {
        unsigned int pad0:3;
        unsigned int bf_stencil_pass_depth_pass_op:3;
        unsigned int bf_stencil_pass_depth_fail_op:3;
        unsigned int bf_stencil_fail_op:3;
        unsigned int bf_stencil_func:3;
        unsigned int bf_stencil_enable:1;
        unsigned int pad1:2;
        unsigned int stencil_write_enable:1;
        unsigned int stencil_pass_depth_pass_op:3;
        unsigned int stencil_pass_depth_fail_op:3;
        unsigned int stencil_fail_op:3;
        unsigned int stencil_func:3;
        unsigned int stencil_enable:1;
    } ds0;

    struct {
        unsigned int bf_stencil_write_mask:8;
        unsigned int bf_stencil_test_mask:8;
        unsigned int stencil_write_mask:8;
        unsigned int stencil_test_mask:8;
    } ds1;

    struct {
        unsigned int pad0:26;
        unsigned int depth_write_enable:1;
        unsigned int depth_test_func:3;
        unsigned int pad1:1;
        unsigned int depth_test_enable:1;
    } ds2;
};



struct gen7_sampler_state
{
   struct
   {
      unsigned int aniso_algorithm:1;
      unsigned int lod_bias:13;
      unsigned int min_filter:3;
      unsigned int mag_filter:3;
      unsigned int mip_filter:2;
      unsigned int base_level:5;
      unsigned int pad1:1;
      unsigned int lod_preclamp:1;
      unsigned int default_color_mode:1;
      unsigned int pad0:1;
      unsigned int disable:1;
   } ss0;

   struct
   {
      unsigned int cube_control_mode:1;
      unsigned int shadow_function:3;
      unsigned int pad:4;
      unsigned int max_lod:12;
      unsigned int min_lod:12;
   } ss1;

   struct
   {
      unsigned int pad:5;
      unsigned int default_color_pointer:27;
   } ss2;

   struct
   {
      unsigned int r_wrap_mode:3;
      unsigned int t_wrap_mode:3;
      unsigned int s_wrap_mode:3;
      unsigned int pad:1;
      unsigned int non_normalized_coord:1;
      unsigned int trilinear_quality:2;
      unsigned int address_round:6;
      unsigned int max_aniso:3;
      unsigned int chroma_key_mode:1;
      unsigned int chroma_key_index:2;
      unsigned int chroma_key_enable:1;
      unsigned int pad0:6;
   } ss3;
};


struct gen7_sampler_8x8
{
  struct {
    unsigned int global_noise_estimation:8;
    unsigned int pad0:8;
    unsigned int chroma_key_index:2;
    unsigned int chroma_key_enable:1;
    unsigned int pad1:10;
    unsigned int ief_bypass:1;
    unsigned int pad2:1;
    unsigned int disable_8x8_filter:1;
  } dw0;

  struct {
    unsigned int pad0:5;
    unsigned int sampler_8x8_state_pointer:27;
  } dw1;

  struct {
    unsigned int weak_edge_threshold:6;
    unsigned int pad0:2;
    unsigned int strong_edge_threshold:6;
    unsigned int pad1:2;
    unsigned int r5x_coefficient:5;
    unsigned int r5cx_coefficient:5;
    unsigned int r5c_coefficient:5;
    unsigned int pad2:1;
  } dw2;

  struct {
    unsigned int r3x_coefficient:5;
    unsigned int pad0:1;
    unsigned int r3c_coefficient:5;
    unsigned int pad1:3;
    unsigned int gain_factor:6;
    unsigned int non_edge_weight:3;
    unsigned int pad2:1;
    unsigned int regular_weight:3;
    unsigned int pad3:1;
    unsigned int strong_edge_weight:3;
    unsigned int ief4_smooth_enable:1;
  } dw3;
};


#define RENDER_CMD(pipeline,op,sub_op)          ((3 << 29) | \
                                                 ((pipeline) << 27) | \
                                                 ((op) << 24) | \
                                                 ((sub_op) << 16))

#define RCMD_URB_FENCE                           RENDER_CMD(0, 0, 0)
#define RCMD_CS_URB_STATE                        RENDER_CMD(0, 0, 1)
#define RCMD_CONSTANT_BUFFER                     RENDER_CMD(0, 0, 2)
#define RCMD_STATE_PREFETCH                      RENDER_CMD(0, 0, 3)

#define RCMD_STATE_BASE_ADDRESS                  RENDER_CMD(0, 1, 1)
#define RCMD_STATE_SIP                           RENDER_CMD(0, 1, 2)
#define RCMD_PIPELINE_SELECT                     RENDER_CMD(1, 1, 4)
#define RCMD_SAMPLER_PALETTE_LOAD                RENDER_CMD(3, 1, 2)


#define RCMD_PIPELINED_POINTERS                  RENDER_CMD(3, 0, 0)
#define RCMD_BINDING_TABLE_POINTERS              RENDER_CMD(3, 0, 1)
# define GEN6_BINDING_TABLE_MODIFY_PS           (1 << 12)/* for GEN6 */
# define GEN6_BINDING_TABLE_MODIFY_GS           (1 << 9) /* for GEN6 */
# define GEN6_BINDING_TABLE_MODIFY_VS           (1 << 8) /* for GEN6 */

#define RCMD_VERTEX_BUFFERS                      RENDER_CMD(3, 0, 8)
#define RCMD_VERTEX_ELEMENTS                     RENDER_CMD(3, 0, 9)
#define RCMD_DRAWING_RECTANGLE                   RENDER_CMD(3, 1, 0)
#define RCMD_CONSTANT_COLOR                      RENDER_CMD(3, 1, 1)
#define RCMD_3DPRIMITIVE                         RENDER_CMD(3, 3, 0)

#define RCMD_DEPTH_BUFFER                        RENDER_CMD(3, 1, 5)
# define RCMD_DEPTH_BUFFER_TYPE_SHIFT            29
# define RCMD_DEPTH_BUFFER_FORMAT_SHIFT          18

#define RCMD_CLEAR_PARAMS                        RENDER_CMD(3, 1, 0x10)
/* DW1 */
# define RCMD_CLEAR_PARAMS_DEPTH_CLEAR_VALID     (1 << 15)

/* for GEN6+ */
#define GEN6_3DSTATE_SAMPLER_STATE_POINTERS	RENDER_CMD(3, 0, 0x02)
# define GEN6_3DSTATE_SAMPLER_STATE_MODIFY_PS	(1 << 12)
# define GEN6_3DSTATE_SAMPLER_STATE_MODIFY_GS	(1 << 9)
# define GEN6_3DSTATE_SAMPLER_STATE_MODIFY_VS	(1 << 8)

#define GEN6_3DSTATE_URB                        RENDER_CMD(3, 0, 0x05)
/* DW1 */
# define GEN6_3DSTATE_URB_VS_SIZE_SHIFT         16
# define GEN6_3DSTATE_URB_VS_ENTRIES_SHIFT      0
/* DW2 */
# define GEN6_3DSTATE_URB_GS_ENTRIES_SHIFT      8
# define GEN6_3DSTATE_URB_GS_SIZE_SHIFT         0

#define GEN6_3DSTATE_VIEWPORT_STATE_POINTERS            RENDER_CMD(3, 0, 0x0d)
# define GEN6_3DSTATE_VIEWPORT_STATE_MODIFY_CC          (1 << 12)
# define GEN6_3DSTATE_VIEWPORT_STATE_MODIFY_SF          (1 << 11)
# define GEN6_3DSTATE_VIEWPORT_STATE_MODIFY_CLIP        (1 << 10)

#define GEN6_3DSTATE_CC_STATE_POINTERS          RENDER_CMD(3, 0, 0x0e)

#define GEN6_3DSTATE_VS                         RENDER_CMD(3, 0, 0x10)

#define GEN6_3DSTATE_GS                         RENDER_CMD(3, 0, 0x11)
/* DW4 */
# define GEN6_3DSTATE_GS_DISPATCH_START_GRF_SHIFT        0

#define GEN6_3DSTATE_CLIP                       RENDER_CMD(3, 0, 0x12)

#define GEN6_3DSTATE_SF                         RENDER_CMD(3, 0, 0x13)
/* DW1 on GEN6 */
# define GEN6_3DSTATE_SF_NUM_OUTPUTS_SHIFT              22
# define GEN6_3DSTATE_SF_URB_ENTRY_READ_LENGTH_SHIFT    11
# define GEN6_3DSTATE_SF_URB_ENTRY_READ_OFFSET_SHIFT    4
/* DW1 on GEN7 */
# define GEN7_SF_DEPTH_BUFFER_SURFACE_FORMAT_SHIFT      12


/* DW2 */
/* DW3 */
# define GEN6_3DSTATE_SF_CULL_BOTH			(0 << 29)
# define GEN6_3DSTATE_SF_CULL_NONE			(1 << 29)
# define GEN6_3DSTATE_SF_CULL_FRONT			(2 << 29)
# define GEN6_3DSTATE_SF_CULL_BACK			(3 << 29)
/* DW4 */
# define GEN6_3DSTATE_SF_TRI_PROVOKE_SHIFT		29
# define GEN6_3DSTATE_SF_LINE_PROVOKE_SHIFT		27
# define GEN6_3DSTATE_SF_TRIFAN_PROVOKE_SHIFT		25

#define GEN6_3DSTATE_WM				RENDER_CMD(3, 0, 0x14)
/* DW2 */
# define GEN6_3DSTATE_WM_SAMPLER_COUNT_SHITF			27
# define GEN6_3DSTATE_WM_BINDING_TABLE_ENTRY_COUNT_SHIFT	18
/* DW4 */
# define GEN6_3DSTATE_WM_DISPATCH_START_GRF_0_SHIFT		16
/* DW5 */
# define GEN6_3DSTATE_WM_MAX_THREADS_SHIFT			25
# define GEN6_3DSTATE_WM_DISPATCH_ENABLE			(1 << 19)
# define GEN6_3DSTATE_WM_16_DISPATCH_ENABLE			(1 << 1)
# define GEN6_3DSTATE_WM_8_DISPATCH_ENABLE			(1 << 0)
/* DW6 */
# define GEN6_3DSTATE_WM_NUM_SF_OUTPUTS_SHIFT			20
# define GEN6_3DSTATE_WM_NONPERSPECTIVE_SAMPLE_BARYCENTRIC	(1 << 15)
# define GEN6_3DSTATE_WM_NONPERSPECTIVE_CENTROID_BARYCENTRIC	(1 << 14)
# define GEN6_3DSTATE_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC	(1 << 13)
# define GEN6_3DSTATE_WM_PERSPECTIVE_SAMPLE_BARYCENTRIC		(1 << 12)
# define GEN6_3DSTATE_WM_PERSPECTIVE_CENTROID_BARYCENTRIC	(1 << 11)
# define GEN6_3DSTATE_WM_PERSPECTIVE_PIXEL_BARYCENTRIC		(1 << 10)

/* 3DSTATE_WM on GEN7 */
/* DW1 */
# define GEN7_WM_STATISTICS_ENABLE                              (1 << 31)
# define GEN7_WM_DEPTH_CLEAR                                    (1 << 30)
# define GEN7_WM_DISPATCH_ENABLE                                (1 << 29)
# define GEN6_WM_DEPTH_RESOLVE                                  (1 << 28)
# define GEN7_WM_HIERARCHICAL_DEPTH_RESOLVE                     (1 << 27)
# define GEN7_WM_KILL_ENABLE                                    (1 << 25)
# define GEN7_WM_PSCDEPTH_OFF                                   (0 << 23)
# define GEN7_WM_PSCDEPTH_ON                                    (1 << 23)
# define GEN7_WM_PSCDEPTH_ON_GE                                 (2 << 23)
# define GEN7_WM_PSCDEPTH_ON_LE                                 (3 << 23)
# define GEN7_WM_USES_SOURCE_DEPTH                              (1 << 20)
# define GEN7_WM_USES_SOURCE_W                                  (1 << 19)
# define GEN7_WM_POSITION_ZW_PIXEL                              (0 << 17)
# define GEN7_WM_POSITION_ZW_CENTROID                           (2 << 17)
# define GEN7_WM_POSITION_ZW_SAMPLE                             (3 << 17)
# define GEN7_WM_NONPERSPECTIVE_SAMPLE_BARYCENTRIC              (1 << 16)
# define GEN7_WM_NONPERSPECTIVE_CENTROID_BARYCENTRIC            (1 << 15)
# define GEN7_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC               (1 << 14)
# define GEN7_WM_PERSPECTIVE_SAMPLE_BARYCENTRIC                 (1 << 13)
# define GEN7_WM_PERSPECTIVE_CENTROID_BARYCENTRIC               (1 << 12)
# define GEN7_WM_PERSPECTIVE_PIXEL_BARYCENTRIC                  (1 << 11)
# define GEN7_WM_USES_INPUT_COVERAGE_MASK                       (1 << 10)
# define GEN7_WM_LINE_END_CAP_AA_WIDTH_0_5                      (0 << 8)
# define GEN7_WM_LINE_END_CAP_AA_WIDTH_1_0                      (1 << 8)
# define GEN7_WM_LINE_END_CAP_AA_WIDTH_2_0                      (2 << 8)
# define GEN7_WM_LINE_END_CAP_AA_WIDTH_4_0                      (3 << 8)
# define GEN7_WM_LINE_AA_WIDTH_0_5                              (0 << 6)
# define GEN7_WM_LINE_AA_WIDTH_1_0                              (1 << 6)
# define GEN7_WM_LINE_AA_WIDTH_2_0                              (2 << 6)
# define GEN7_WM_LINE_AA_WIDTH_4_0                              (3 << 6)
# define GEN7_WM_POLYGON_STIPPLE_ENABLE                         (1 << 4)
# define GEN7_WM_LINE_STIPPLE_ENABLE                            (1 << 3)
# define GEN7_WM_POINT_RASTRULE_UPPER_RIGHT                     (1 << 2)
# define GEN7_WM_MSRAST_OFF_PIXEL                               (0 << 0)
# define GEN7_WM_MSRAST_OFF_PATTERN                             (1 << 0)
# define GEN7_WM_MSRAST_ON_PIXEL                                (2 << 0)
# define GEN7_WM_MSRAST_ON_PATTERN                              (3 << 0)
/* DW2 */
# define GEN7_WM_MSDISPMODE_PERPIXEL                            (1 << 31)

#define GEN6_3DSTATE_CONSTANT_VS		RENDER_CMD(3, 0, 0x15)
#define GEN6_3DSTATE_CONSTANT_GS          	RENDER_CMD(3, 0, 0x16)
#define GEN6_3DSTATE_CONSTANT_PS          	RENDER_CMD(3, 0, 0x17)



# define GEN6_3DSTATE_CONSTANT_BUFFER_3_ENABLE  (1 << 15)
# define GEN6_3DSTATE_CONSTANT_BUFFER_2_ENABLE  (1 << 14)
# define GEN6_3DSTATE_CONSTANT_BUFFER_1_ENABLE  (1 << 13)
# define GEN6_3DSTATE_CONSTANT_BUFFER_0_ENABLE  (1 << 12)

#define GEN6_3DSTATE_SAMPLE_MASK		RENDER_CMD(3, 0, 0x18)

#define GEN6_3DSTATE_MULTISAMPLE		RENDER_CMD(3, 1, 0x0d)
/* DW1 */
# define GEN6_3DSTATE_MULTISAMPLE_PIXEL_LOCATION_CENTER         (0 << 4)
# define GEN6_3DSTATE_MULTISAMPLE_PIXEL_LOCATION_UPPER_LEFT     (1 << 4)
# define GEN6_3DSTATE_MULTISAMPLE_NUMSAMPLES_1                  (0 << 1)
# define GEN6_3DSTATE_MULTISAMPLE_NUMSAMPLES_4                  (2 << 1)
# define GEN6_3DSTATE_MULTISAMPLE_NUMSAMPLES_8                  (3 << 1)

/* GEN7 */
#define GEN7_3DSTATE_CLEAR_PARAMS               RENDER_CMD(3, 0, 0x04)
#define GEN7_3DSTATE_DEPTH_BUFFER               RENDER_CMD(3, 0, 0x05)
#define GEN7_3DSTATE_HIER_DEPTH_BUFFER		RENDER_CMD(3, 0, 0x07)

#define GEN7_3DSTATE_URB_VS                     RENDER_CMD(3, 0, 0x30)
#define GEN7_3DSTATE_URB_HS                     RENDER_CMD(3, 0, 0x31)
#define GEN7_3DSTATE_URB_DS                     RENDER_CMD(3, 0, 0x32)
#define GEN7_3DSTATE_URB_GS                     RENDER_CMD(3, 0, 0x33)
/* DW1 */
# define GEN7_URB_ENTRY_NUMBER_SHIFT            0
# define GEN7_URB_ENTRY_SIZE_SHIFT              16
# define GEN7_URB_STARTING_ADDRESS_SHIFT        25

#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_VS     RENDER_CMD(3, 1, 0x12)
#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_PS     RENDER_CMD(3, 1, 0x16)

#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_DS     RENDER_CMD(3, 1, 0x14)
#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_HS     RENDER_CMD(3, 1, 0x13)
#define GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_GS     RENDER_CMD(3, 1, 0x15)
/* DW1 */
# define GEN7_PUSH_CONSTANT_BUFFER_OFFSET_SHIFT 16

#define GEN7_3DSTATE_CONSTANT_HS                RENDER_CMD(3, 0, 0x19)
#define GEN7_3DSTATE_CONSTANT_DS                RENDER_CMD(3, 0, 0x1a)

#define GEN7_3DSTATE_HS                         RENDER_CMD(3, 0, 0x1b)
#define GEN7_3DSTATE_TE                         RENDER_CMD(3, 0, 0x1c)
#define GEN7_3DSTATE_DS                         RENDER_CMD(3, 0, 0x1d)
#define GEN7_3DSTATE_STREAMOUT                  RENDER_CMD(3, 0, 0x1e)
#define GEN7_3DSTATE_SBE                        RENDER_CMD(3, 0, 0x1f)

/* DW1 */
# define GEN7_SBE_SWIZZLE_CONTROL_MODE          (1 << 28)
# define GEN7_SBE_NUM_OUTPUTS_SHIFT             22
# define GEN7_SBE_SWIZZLE_ENABLE                (1 << 21)
# define GEN7_SBE_POINT_SPRITE_LOWERLEFT        (1 << 20)
# define GEN7_SBE_URB_ENTRY_READ_LENGTH_SHIFT   11
# define GEN7_SBE_URB_ENTRY_READ_OFFSET_SHIFT   4


#define GEN7_3DSTATE_PS                                 RENDER_CMD(3, 0, 0x20)
/* DW1: kernel pointer */
/* DW2 */
# define GEN7_PS_SPF_MODE                               (1 << 31)
# define GEN7_PS_VECTOR_MASK_ENABLE                     (1 << 30)
# define GEN7_PS_SAMPLER_COUNT_SHIFT                    27
# define GEN7_PS_BINDING_TABLE_ENTRY_COUNT_SHIFT        18
# define GEN7_PS_FLOATING_POINT_MODE_IEEE_754           (0 << 16)
# define GEN7_PS_FLOATING_POINT_MODE_ALT                (1 << 16)
/* DW3: scratch space */
/* DW4 */
# define GEN7_PS_MAX_THREADS_SHIFT_IVB                  24
# define GEN7_PS_MAX_THREADS_SHIFT_HSW                  23
# define GEN7_PS_SAMPLE_MASK_SHIFT_HSW                  12
# define GEN7_PS_PUSH_CONSTANT_ENABLE                   (1 << 11)
# define GEN7_PS_ATTRIBUTE_ENABLE                       (1 << 10)
# define GEN7_PS_OMASK_TO_RENDER_TARGET                 (1 << 9)
# define GEN7_PS_DUAL_SOURCE_BLEND_ENABLE               (1 << 7)
# define GEN7_PS_POSOFFSET_NONE                         (0 << 3)
# define GEN7_PS_POSOFFSET_CENTROID                     (2 << 3)
# define GEN7_PS_POSOFFSET_SAMPLE                       (3 << 3)
# define GEN7_PS_32_DISPATCH_ENABLE                     (1 << 2)
# define GEN7_PS_16_DISPATCH_ENABLE                     (1 << 1)
# define GEN7_PS_8_DISPATCH_ENABLE                      (1 << 0)
/* DW5 */
# define GEN7_PS_DISPATCH_START_GRF_SHIFT_0             16
# define GEN7_PS_DISPATCH_START_GRF_SHIFT_1             8
# define GEN7_PS_DISPATCH_START_GRF_SHIFT_2             0
/* DW6: kernel 1 pointer */
/* DW7: kernel 2 pointer */



#define GEN7_3DSTATE_STENCIL_BUFFER			RENDER_CMD(3, 0, 0x06)

#define GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CL      RENDER_CMD(3, 0, 0x21)
#define GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_CC         RENDER_CMD(3, 0, 0x23)

#define GEN7_3DSTATE_BLEND_STATE_POINTERS               RENDER_CMD(3, 0, 0x24)
#define GEN7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS       RENDER_CMD(3, 0, 0x25)

#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_VS          RENDER_CMD(3, 0, 0x26)
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_HS          RENDER_CMD(3, 0, 0x27)
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_DS          RENDER_CMD(3, 0, 0x28)
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_GS          RENDER_CMD(3, 0, 0x29)
#define GEN7_3DSTATE_BINDING_TABLE_POINTERS_PS          RENDER_CMD(3, 0, 0x2a)

#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_VS          RENDER_CMD(3, 0, 0x2b)
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_GS          RENDER_CMD(3, 0, 0x2e)
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_PS          RENDER_CMD(3, 0, 0x2f)
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_HS          RENDER_CMD(3, 0, 0x2c)
#define GEN7_3DSTATE_SAMPLER_STATE_POINTERS_DS          RENDER_CMD(3, 0, 0x2d)

#define I965_DEPTHFORMAT_D32_FLOAT              1

#define BASE_ADDRESS_MODIFY             (1 << 0)

#define PIPELINE_SELECT_3D              0
#define PIPELINE_SELECT_MEDIA           1


#define FLOATING_POINT_IEEE_754        0
#define FLOATING_POINT_NON_IEEE_754    1


#define I965_SURFACE_1D      0
#define I965_SURFACE_2D      1
#define I965_SURFACE_3D      2
#define I965_SURFACE_CUBE    3
#define I965_SURFACE_BUFFER  4
#define I965_SURFACE_NULL    7

#define I965_SURFACEFORMAT_R32G32B32A32_FLOAT             0x000
#define I965_SURFACEFORMAT_R32G32B32A32_SINT              0x001
#define I965_SURFACEFORMAT_R32G32B32A32_UINT              0x002
#define I965_SURFACEFORMAT_R32G32B32A32_UNORM             0x003
#define I965_SURFACEFORMAT_R32G32B32A32_SNORM             0x004
#define I965_SURFACEFORMAT_R64G64_FLOAT                   0x005
#define I965_SURFACEFORMAT_R32G32B32X32_FLOAT             0x006
#define I965_SURFACEFORMAT_R32G32B32A32_SSCALED           0x007
#define I965_SURFACEFORMAT_R32G32B32A32_USCALED           0x008
#define I965_SURFACEFORMAT_R32G32B32_FLOAT                0x040
#define I965_SURFACEFORMAT_R32G32B32_SINT                 0x041
#define I965_SURFACEFORMAT_R32G32B32_UINT                 0x042
#define I965_SURFACEFORMAT_R32G32B32_UNORM                0x043
#define I965_SURFACEFORMAT_R32G32B32_SNORM                0x044
#define I965_SURFACEFORMAT_R32G32B32_SSCALED              0x045
#define I965_SURFACEFORMAT_R32G32B32_USCALED              0x046
#define I965_SURFACEFORMAT_R16G16B16A16_UNORM             0x080
#define I965_SURFACEFORMAT_R16G16B16A16_SNORM             0x081
#define I965_SURFACEFORMAT_R16G16B16A16_SINT              0x082
#define I965_SURFACEFORMAT_R16G16B16A16_UINT              0x083
#define I965_SURFACEFORMAT_R16G16B16A16_FLOAT             0x084
#define I965_SURFACEFORMAT_R32G32_FLOAT                   0x085
#define I965_SURFACEFORMAT_R32G32_SINT                    0x086
#define I965_SURFACEFORMAT_R32G32_UINT                    0x087
#define I965_SURFACEFORMAT_R32_FLOAT_X8X24_TYPELESS       0x088
#define I965_SURFACEFORMAT_X32_TYPELESS_G8X24_UINT        0x089
#define I965_SURFACEFORMAT_L32A32_FLOAT                   0x08A
#define I965_SURFACEFORMAT_R32G32_UNORM                   0x08B
#define I965_SURFACEFORMAT_R32G32_SNORM                   0x08C
#define I965_SURFACEFORMAT_R64_FLOAT                      0x08D
#define I965_SURFACEFORMAT_R16G16B16X16_UNORM             0x08E
#define I965_SURFACEFORMAT_R16G16B16X16_FLOAT             0x08F
#define I965_SURFACEFORMAT_A32X32_FLOAT                   0x090
#define I965_SURFACEFORMAT_L32X32_FLOAT                   0x091
#define I965_SURFACEFORMAT_I32X32_FLOAT                   0x092
#define I965_SURFACEFORMAT_R16G16B16A16_SSCALED           0x093
#define I965_SURFACEFORMAT_R16G16B16A16_USCALED           0x094
#define I965_SURFACEFORMAT_R32G32_SSCALED                 0x095
#define I965_SURFACEFORMAT_R32G32_USCALED                 0x096
#define I965_SURFACEFORMAT_B8G8R8A8_UNORM                 0x0C0
#define I965_SURFACEFORMAT_B8G8R8A8_UNORM_SRGB            0x0C1
#define I965_SURFACEFORMAT_R10G10B10A2_UNORM              0x0C2
#define I965_SURFACEFORMAT_R10G10B10A2_UNORM_SRGB         0x0C3
#define I965_SURFACEFORMAT_R10G10B10A2_UINT               0x0C4
#define I965_SURFACEFORMAT_R10G10B10_SNORM_A2_UNORM       0x0C5
#define I965_SURFACEFORMAT_R8G8B8A8_UNORM                 0x0C7
#define I965_SURFACEFORMAT_R8G8B8A8_UNORM_SRGB            0x0C8
#define I965_SURFACEFORMAT_R8G8B8A8_SNORM                 0x0C9
#define I965_SURFACEFORMAT_R8G8B8A8_SINT                  0x0CA
#define I965_SURFACEFORMAT_R8G8B8A8_UINT                  0x0CB
#define I965_SURFACEFORMAT_R16G16_UNORM                   0x0CC
#define I965_SURFACEFORMAT_R16G16_SNORM                   0x0CD
#define I965_SURFACEFORMAT_R16G16_SINT                    0x0CE
#define I965_SURFACEFORMAT_R16G16_UINT                    0x0CF
#define I965_SURFACEFORMAT_R16G16_FLOAT                   0x0D0
#define I965_SURFACEFORMAT_B10G10R10A2_UNORM              0x0D1
#define I965_SURFACEFORMAT_B10G10R10A2_UNORM_SRGB         0x0D2
#define I965_SURFACEFORMAT_R11G11B10_FLOAT                0x0D3
#define I965_SURFACEFORMAT_R32_SINT                       0x0D6
#define I965_SURFACEFORMAT_R32_UINT                       0x0D7
#define I965_SURFACEFORMAT_R32_FLOAT                      0x0D8
#define I965_SURFACEFORMAT_R24_UNORM_X8_TYPELESS          0x0D9
#define I965_SURFACEFORMAT_X24_TYPELESS_G8_UINT           0x0DA
#define I965_SURFACEFORMAT_L16A16_UNORM                   0x0DF
#define I965_SURFACEFORMAT_I24X8_UNORM                    0x0E0
#define I965_SURFACEFORMAT_L24X8_UNORM                    0x0E1
#define I965_SURFACEFORMAT_A24X8_UNORM                    0x0E2
#define I965_SURFACEFORMAT_I32_FLOAT                      0x0E3
#define I965_SURFACEFORMAT_L32_FLOAT                      0x0E4
#define I965_SURFACEFORMAT_A32_FLOAT                      0x0E5
#define I965_SURFACEFORMAT_B8G8R8X8_UNORM                 0x0E9
#define I965_SURFACEFORMAT_B8G8R8X8_UNORM_SRGB            0x0EA
#define I965_SURFACEFORMAT_R8G8B8X8_UNORM                 0x0EB
#define I965_SURFACEFORMAT_R8G8B8X8_UNORM_SRGB            0x0EC
#define I965_SURFACEFORMAT_R9G9B9E5_SHAREDEXP             0x0ED
#define I965_SURFACEFORMAT_B10G10R10X2_UNORM              0x0EE
#define I965_SURFACEFORMAT_L16A16_FLOAT                   0x0F0
#define I965_SURFACEFORMAT_R32_UNORM                      0x0F1
#define I965_SURFACEFORMAT_R32_SNORM                      0x0F2
#define I965_SURFACEFORMAT_R10G10B10X2_USCALED            0x0F3
#define I965_SURFACEFORMAT_R8G8B8A8_SSCALED               0x0F4
#define I965_SURFACEFORMAT_R8G8B8A8_USCALED               0x0F5
#define I965_SURFACEFORMAT_R16G16_SSCALED                 0x0F6
#define I965_SURFACEFORMAT_R16G16_USCALED                 0x0F7
#define I965_SURFACEFORMAT_R32_SSCALED                    0x0F8
#define I965_SURFACEFORMAT_R32_USCALED                    0x0F9
#define I965_SURFACEFORMAT_B5G6R5_UNORM                   0x100
#define I965_SURFACEFORMAT_B5G6R5_UNORM_SRGB              0x101
#define I965_SURFACEFORMAT_B5G5R5A1_UNORM                 0x102
#define I965_SURFACEFORMAT_B5G5R5A1_UNORM_SRGB            0x103
#define I965_SURFACEFORMAT_B4G4R4A4_UNORM                 0x104
#define I965_SURFACEFORMAT_B4G4R4A4_UNORM_SRGB            0x105
#define I965_SURFACEFORMAT_R8G8_UNORM                     0x106
#define I965_SURFACEFORMAT_R8G8_SNORM                     0x107
#define I965_SURFACEFORMAT_R8G8_SINT                      0x108
#define I965_SURFACEFORMAT_R8G8_UINT                      0x109
#define I965_SURFACEFORMAT_R16_UNORM                      0x10A
#define I965_SURFACEFORMAT_R16_SNORM                      0x10B
#define I965_SURFACEFORMAT_R16_SINT                       0x10C
#define I965_SURFACEFORMAT_R16_UINT                       0x10D
#define I965_SURFACEFORMAT_R16_FLOAT                      0x10E
#define I965_SURFACEFORMAT_I16_UNORM                      0x111
#define I965_SURFACEFORMAT_L16_UNORM                      0x112
#define I965_SURFACEFORMAT_A16_UNORM                      0x113
#define I965_SURFACEFORMAT_L8A8_UNORM                     0x114
#define I965_SURFACEFORMAT_I16_FLOAT                      0x115
#define I965_SURFACEFORMAT_L16_FLOAT                      0x116
#define I965_SURFACEFORMAT_A16_FLOAT                      0x117
#define I965_SURFACEFORMAT_R5G5_SNORM_B6_UNORM            0x119
#define I965_SURFACEFORMAT_B5G5R5X1_UNORM                 0x11A
#define I965_SURFACEFORMAT_B5G5R5X1_UNORM_SRGB            0x11B
#define I965_SURFACEFORMAT_R8G8_SSCALED                   0x11C
#define I965_SURFACEFORMAT_R8G8_USCALED                   0x11D
#define I965_SURFACEFORMAT_R16_SSCALED                    0x11E
#define I965_SURFACEFORMAT_R16_USCALED                    0x11F
#define I965_SURFACEFORMAT_P8A8_UNORM                     0x122
#define I965_SURFACEFORMAT_A8P8_UNORM                     0x123
#define I965_SURFACEFORMAT_R8_UNORM                       0x140
#define I965_SURFACEFORMAT_R8_SNORM                       0x141
#define I965_SURFACEFORMAT_R8_SINT                        0x142
#define I965_SURFACEFORMAT_R8_UINT                        0x143
#define I965_SURFACEFORMAT_A8_UNORM                       0x144
#define I965_SURFACEFORMAT_I8_UNORM                       0x145
#define I965_SURFACEFORMAT_L8_UNORM                       0x146
#define I965_SURFACEFORMAT_P4A4_UNORM                     0x147
#define I965_SURFACEFORMAT_A4P4_UNORM                     0x148
#define I965_SURFACEFORMAT_R8_SSCALED                     0x149
#define I965_SURFACEFORMAT_R8_USCALED                     0x14A
#define I965_SURFACEFORMAT_R1_UINT                        0x181
#define I965_SURFACEFORMAT_YCRCB_NORMAL                   0x182
#define I965_SURFACEFORMAT_YCRCB_SWAPUVY                  0x183
#define I965_SURFACEFORMAT_BC1_UNORM                      0x186
#define I965_SURFACEFORMAT_BC2_UNORM                      0x187
#define I965_SURFACEFORMAT_BC3_UNORM                      0x188
#define I965_SURFACEFORMAT_BC4_UNORM                      0x189
#define I965_SURFACEFORMAT_BC5_UNORM                      0x18A
#define I965_SURFACEFORMAT_BC1_UNORM_SRGB                 0x18B
#define I965_SURFACEFORMAT_BC2_UNORM_SRGB                 0x18C
#define I965_SURFACEFORMAT_BC3_UNORM_SRGB                 0x18D
#define I965_SURFACEFORMAT_MONO8                          0x18E
#define I965_SURFACEFORMAT_YCRCB_SWAPUV                   0x18F
#define I965_SURFACEFORMAT_YCRCB_SWAPY                    0x190
#define I965_SURFACEFORMAT_DXT1_RGB                       0x191
#define I965_SURFACEFORMAT_FXT1                           0x192
#define I965_SURFACEFORMAT_R8G8B8_UNORM                   0x193
#define I965_SURFACEFORMAT_R8G8B8_SNORM                   0x194
#define I965_SURFACEFORMAT_R8G8B8_SSCALED                 0x195
#define I965_SURFACEFORMAT_R8G8B8_USCALED                 0x196
#define I965_SURFACEFORMAT_R64G64B64A64_FLOAT             0x197
#define I965_SURFACEFORMAT_R64G64B64_FLOAT                0x198
#define I965_SURFACEFORMAT_BC4_SNORM                      0x199
#define I965_SURFACEFORMAT_BC5_SNORM                      0x19A
#define I965_SURFACEFORMAT_R16G16B16_UNORM                0x19C
#define I965_SURFACEFORMAT_R16G16B16_SNORM                0x19D
#define I965_SURFACEFORMAT_R16G16B16_SSCALED              0x19E
#define I965_SURFACEFORMAT_R16G16B16_USCALED              0x19F

#define I965_CULLMODE_BOTH      0
#define I965_CULLMODE_NONE      1
#define I965_CULLMODE_FRONT     2
#define I965_CULLMODE_BACK      3

#define I965_MAPFILTER_NEAREST        0x0
#define I965_MAPFILTER_LINEAR         0x1
#define I965_MAPFILTER_ANISOTROPIC    0x2

#define I965_MIPFILTER_NONE        0
#define I965_MIPFILTER_NEAREST     1
#define I965_MIPFILTER_LINEAR      3

#define RENDER_HSW_SCS_ZERO                      0
#define RENDER_HSW_SCS_ONE                       1
#define RENDER_HSW_SCS_RED                       4
#define RENDER_HSW_SCS_GREEN                     5
#define RENDER_HSW_SCS_BLUE                      6
#define RENDER_HSW_SCS_ALPHA                     7

#define I965_TEXCOORDMODE_WRAP            0
#define I965_TEXCOORDMODE_MIRROR          1
#define I965_TEXCOORDMODE_CLAMP           2
#define I965_TEXCOORDMODE_CUBE            3
#define I965_TEXCOORDMODE_CLAMP_BORDER    4
#define I965_TEXCOORDMODE_MIRROR_ONCE     5

#define I965_BLENDFACTOR_ONE                 0x1
#define I965_BLENDFACTOR_SRC_COLOR           0x2
#define I965_BLENDFACTOR_SRC_ALPHA           0x3
#define I965_BLENDFACTOR_DST_ALPHA           0x4
#define I965_BLENDFACTOR_DST_COLOR           0x5
#define I965_BLENDFACTOR_SRC_ALPHA_SATURATE  0x6
#define I965_BLENDFACTOR_CONST_COLOR         0x7
#define I965_BLENDFACTOR_CONST_ALPHA         0x8
#define I965_BLENDFACTOR_SRC1_COLOR          0x9
#define I965_BLENDFACTOR_SRC1_ALPHA          0x0A
#define I965_BLENDFACTOR_ZERO                0x11
#define I965_BLENDFACTOR_INV_SRC_COLOR       0x12
#define I965_BLENDFACTOR_INV_SRC_ALPHA       0x13
#define I965_BLENDFACTOR_INV_DST_ALPHA       0x14
#define I965_BLENDFACTOR_INV_DST_COLOR       0x15
#define I965_BLENDFACTOR_INV_CONST_COLOR     0x17
#define I965_BLENDFACTOR_INV_CONST_ALPHA     0x18
#define I965_BLENDFACTOR_INV_SRC1_COLOR      0x19
#define I965_BLENDFACTOR_INV_SRC1_ALPHA      0x1A

#define I965_BLENDFUNCTION_ADD               0
#define I965_BLENDFUNCTION_SUBTRACT          1
#define I965_BLENDFUNCTION_REVERSE_SUBTRACT  2
#define I965_BLENDFUNCTION_MIN               3
#define I965_BLENDFUNCTION_MAX               4

#define I965_SURFACERETURNFORMAT_FLOAT32  0
#define I965_SURFACERETURNFORMAT_S1       1

#define I965_VFCOMPONENT_NOSTORE      0
#define I965_VFCOMPONENT_STORE_SRC    1
#define I965_VFCOMPONENT_STORE_0      2
#define I965_VFCOMPONENT_STORE_1_FLT  3
#define I965_VFCOMPONENT_STORE_1_INT  4
#define I965_VFCOMPONENT_STORE_VID    5
#define I965_VFCOMPONENT_STORE_IID    6
#define I965_VFCOMPONENT_STORE_PID    7

#define VE0_VERTEX_BUFFER_INDEX_SHIFT	27
#define GEN6_VE0_VERTEX_BUFFER_INDEX_SHIFT      26 /* for GEN6 */
#define VE0_VALID			(1 << 26)
#define GEN6_VE0_VALID                  (1 << 25) /* for GEN6 */
#define VE0_FORMAT_SHIFT		16
#define VE0_OFFSET_SHIFT		0
#define VE1_VFCOMPONENT_0_SHIFT		28
#define VE1_VFCOMPONENT_1_SHIFT		24
#define VE1_VFCOMPONENT_2_SHIFT		20
#define VE1_VFCOMPONENT_3_SHIFT		16
#define VE1_DESTINATION_ELEMENT_OFFSET_SHIFT	0

#define VB0_BUFFER_INDEX_SHIFT          27
#define GEN6_VB0_BUFFER_INDEX_SHIFT     26
#define VB0_VERTEXDATA                  (0 << 26)
#define VB0_INSTANCEDATA                (1 << 26)
#define GEN6_VB0_VERTEXDATA             (0 << 20)
#define GEN6_VB0_INSTANCEDATA           (1 << 20)
#define GEN7_VB0_ADDRESS_MODIFYENABLE   (1 << 14)
#define VB0_BUFFER_PITCH_SHIFT          0

#define _3DPRIMITIVE_VERTEX_SEQUENTIAL  (0 << 15)
#define _3DPRIMITIVE_VERTEX_RANDOM      (1 << 15)
#define _3DPRIMITIVE_TOPOLOGY_SHIFT     10
/* DW1 on GEN7*/
# define GEN7_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL     (0 << 8)
# define GEN7_3DPRIM_VERTEXBUFFER_ACCESS_RANDOM         (1 << 8)

#define _3DPRIM_POINTLIST         0x01
#define _3DPRIM_LINELIST          0x02
#define _3DPRIM_LINESTRIP         0x03
#define _3DPRIM_TRILIST           0x04
#define _3DPRIM_TRISTRIP          0x05
#define _3DPRIM_TRIFAN            0x06
#define _3DPRIM_QUADLIST          0x07
#define _3DPRIM_QUADSTRIP         0x08
#define _3DPRIM_LINELIST_ADJ      0x09
#define _3DPRIM_LINESTRIP_ADJ     0x0A
#define _3DPRIM_TRILIST_ADJ       0x0B
#define _3DPRIM_TRISTRIP_ADJ      0x0C
#define _3DPRIM_TRISTRIP_REVERSE  0x0D
#define _3DPRIM_POLYGON           0x0E
#define _3DPRIM_RECTLIST          0x0F
#define _3DPRIM_LINELOOP          0x10
#define _3DPRIM_POINTLIST_BF      0x11
#define _3DPRIM_LINESTRIP_CONT    0x12
#define _3DPRIM_LINESTRIP_BF      0x13
#define _3DPRIM_LINESTRIP_CONT_BF 0x14
#define _3DPRIM_TRIFAN_NOSTIPPLE  0x15


#define I965_TILEWALK_XMAJOR                 0
#define I965_TILEWALK_YMAJOR                 1


#define RCMD_MI                                  (0x0 << 29)
#define RCMD_2D                                  (0x2 << 29)
#define RCMD_3D                                  (0x3 << 29)

#define INTEL_MI_NOOP                                 (RCMD_MI | 0)

#define INTEL_MI_BATCH_BUFFER_END                     (RCMD_MI | (0xA << 23))
#define INTEL_MI_BATCH_BUFFER_START                   (RCMD_MI | (0x31 << 23))

#define INTEL_MI_FLUSH                                (RCMD_MI | (0x4 << 23))
#define   MI_FLUSH_STATE_INSTRUCTION_CACHE_INVALIDATE   (0x1 << 0)

#define INTEL_MI_FLUSH_DW                             (RCMD_MI | (0x26 << 23) | 0x2)
#define   MI_FLUSH_DW_VIDEO_PIPELINE_CACHE_INVALIDATE   (0x1 << 7)

#define XY_COLOR_BLT_CMD                        (RCMD_2D | (0x50 << 22) | 0x04)
#define XY_COLOR_BLT_WRITE_ALPHA                (1 << 21)
#define XY_COLOR_BLT_WRITE_RGB                  (1 << 20)
#define XY_COLOR_BLT_DST_TILED                  (1 << 11)


/* BR13 */
#define BR13_8                                  (0x0 << 24)
#define BR13_565                                (0x1 << 24)
#define BR13_1555                               (0x2 << 24)
#define BR13_8888                               (0x3 << 24)

#define RCMD_PIPE_CONTROL                        (RCMD_3D | (3 << 27) | (2 << 24) | (0 << 16))
#define RCMD_PIPE_CONTROL_CS_STALL               (1 << 20)
#define RCMD_PIPE_CONTROL_NOWRITE                (0 << 14)
#define RCMD_PIPE_CONTROL_WRITE_QWORD            (1 << 14)
#define RCMD_PIPE_CONTROL_WRITE_DEPTH            (2 << 14)
#define RCMD_PIPE_CONTROL_WRITE_TIME             (3 << 14)
#define RCMD_PIPE_CONTROL_DEPTH_STALL            (1 << 13)
#define RCMD_PIPE_CONTROL_WC_FLUSH               (1 << 12)
#define RCMD_PIPE_CONTROL_IS_FLUSH               (1 << 11)
#define RCMD_PIPE_CONTROL_TC_FLUSH               (1 << 10)
#define RCMD_PIPE_CONTROL_NOTIFY_ENABLE          (1 << 8)
#define RCMD_PIPE_CONTROL_DC_FLUSH               (1 << 5)
#define RCMD_PIPE_CONTROL_GLOBAL_GTT             (1 << 2)
#define RCMD_PIPE_CONTROL_LOCAL_PGTT             (0 << 2)
#define RCMD_PIPE_CONTROL_STALL_AT_SCOREBOARD    (1 << 1)
#define RCMD_PIPE_CONTROL_DEPTH_CACHE_FLUSH      (1 << 0)

enum
{
    SF_KERNEL = 0,
    PS_KERNEL,
    PS_SUBPIC_KERNEL
};

struct gen8_interface_descriptor_data
{
  struct {
    unsigned int pad0:6;
    unsigned int kernel_start_pointer:26;
  } desc0;

  struct {
    unsigned int kernel_start_pointer_high:16;
    unsigned int pad0:16;
  } desc1;

  struct {
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

  struct {
    unsigned int pad0:2;
    unsigned int sampler_count:3;
    unsigned int sampler_state_pointer:27;
  } desc3;

  struct {
    unsigned int binding_table_entry_count:5;
    unsigned int binding_table_pointer:11;
    unsigned int pad0: 16;
  } desc4;

  struct {
    unsigned int constant_urb_entry_read_offset:16;
    unsigned int constant_urb_entry_read_length:16;
  } desc5;

  struct {
    unsigned int num_threads_in_tg:10;
    unsigned int pad0:5;
    unsigned int global_barrier_enable:1;
    unsigned int shared_local_memory_size:5;
    unsigned int barrier_enable:1;
    unsigned int rounding_mode:2;
    unsigned int pad1:8;
  } desc6;

  struct {
    unsigned int cross_thread_constant_data_read_length:8;
    unsigned int pad0:24;
  } desc7;
};

struct gen8_surface_state
{
  struct {
    unsigned int cube_pos_z:1;
    unsigned int cube_neg_z:1;
    unsigned int cube_pos_y:1;
    unsigned int cube_neg_y:1;
    unsigned int cube_pos_x:1;
    unsigned int cube_neg_x:1;
    unsigned int media_boundary_pixel_mode:2;
    unsigned int render_cache_read_write:1;
    unsigned int sampler_l2bypass_disable:1;
    unsigned int vert_line_stride_ofs:1;
    unsigned int vert_line_stride:1;
    unsigned int tile_walk:1;
    unsigned int tiled_surface:1;
    unsigned int horizontal_alignment:2;
    /* Field 16 */
    unsigned int vertical_alignment:2;
    unsigned int surface_format:9;     /**< BRW_SURFACEFORMAT_x */
    unsigned int pad0:1;
    unsigned int is_array:1;
    unsigned int surface_type:3;       /**< BRW_SURFACE_1D/2D/3D/CUBE */
  } ss0;

  struct {
    unsigned int surface_qpitch:15;
    unsigned int pad0:4;
    unsigned int base_mip_level:5;
    unsigned int surface_mocs:7;
    unsigned int pad1:1;
  } ss1;

  struct {
    unsigned int width:14;
    unsigned int pad0:2;
    unsigned int height:14;
    unsigned int pad1:2;
  } ss2;

  struct {
    unsigned int pitch:18;
    unsigned int pad:3;
    unsigned int depth:11;
  } ss3;

  struct {
    unsigned int multisample_position_palette_index:3;
    unsigned int num_multisamples:3;
    unsigned int multisampled_surface_storage_format:1;
    unsigned int render_target_view_extent:11;
    unsigned int min_array_elt:11;
    unsigned int rotation:2;
    unsigned int force_ncmp_reduce_type:1;
  } ss4;

  struct {
    unsigned int mip_count:4;
    unsigned int min_lod:4;
    unsigned int pad0:4;
    unsigned int pad1:2;
    unsigned int coherence_type:1;
    unsigned int pad2:3;
    unsigned int pad3:2;
    unsigned int ewa_disable_cube:1;
    unsigned int y_offset:3;
    unsigned int pad4:1;
    unsigned int x_offset:7;
  } ss5;

  struct {
    unsigned int y_offset_uv_plane:14;
    unsigned int pad0:2;
    unsigned int x_offset_uv_plane:14;
    unsigned int pad1:1;
    unsigned int separate_uv_plane:1;
  } ss6;

  struct {
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
  } ss7;

  struct {
    unsigned int base_addr;
  } ss8;

  struct {
    unsigned int base_addr_high:16;
    unsigned int pad0:16;
  } ss9;

  struct {
    unsigned int pad0:12;
    unsigned int aux_base_addr:20;
  } ss10;

  union {
    struct {
      unsigned int y_offset_v_plane:14;
      unsigned int pad0:2;
      unsigned int x_offset_v_plane:14;
      unsigned int pad1:2;
    } planar;
    struct {
      unsigned int aux_base_addr_high:16;
      unsigned int pad2:16;
    } aux_buffer;
  } ss11;

  struct {
    unsigned int hier_depth_clear;
  } ss12;

  struct {
    unsigned int pad0;
  } ss13;

  struct {
    unsigned int pad0;
  } ss14;

  struct {
    unsigned int pad0;
  } ss15;
};

struct gen8_sampler_state
{
  struct
  {
    unsigned int aniso_algorithm:1;
    unsigned int lod_bias:13;
    unsigned int min_filter:3;
    unsigned int mag_filter:3;
    unsigned int mip_filter:2;
    unsigned int base_level:5;
    unsigned int lod_preclamp:2;
    unsigned int default_color_mode:1;
    unsigned int pad0:1;
    unsigned int disable:1;
  } ss0;

  struct
  {
    unsigned int cube_control_mode:1;
    unsigned int shadow_function:3;
    unsigned int chroma_key_mode:1;
    unsigned int chroma_key_index:2;
    unsigned int chroma_key_enable:1;
    unsigned int max_lod:12;
    unsigned int min_lod:12;
  } ss1;

  struct
  {
    unsigned int lod_clamp_mag_mode:1; /* MIPNONE or MIPFILTER */
    unsigned int flex_filter_vert_align:1;
    unsigned int flex_filter_hort_align:1;
    unsigned int flex_filter_coff_size:1; /* coff8 or coff 16 */
    unsigned int flex_filter_mode:1;
    unsigned int pad0:1;
    unsigned int indirect_state_pointer:18; /* point to the SAMPLE_INDIRECT_STATE */
    union {
      unsigned char nonsep_filter_footer_highmask;
      struct {
        unsigned char pad1:2;
        unsigned char sep_filter_height:2;
        unsigned char sep_filter_width:2;
        unsigned char sep_filter_coff_size:2;
      } sep_filter;
    } ss2_byte3;
  } ss2;

  struct
  {
    unsigned int r_wrap_mode:3;
    unsigned int t_wrap_mode:3;
    unsigned int s_wrap_mode:3;
    unsigned int pad0:1;
    unsigned int non_normalized_coord:1;
    unsigned int trilinear_quality:2;
    unsigned int address_round:6;
    unsigned int max_aniso:3;
    unsigned int pad1:2;
    unsigned int nonsep_filter_foot_lowmask:8;
  } ss3;
};

struct gen8_global_blend_state
{
  unsigned int pad0:19;
  unsigned int ydither_offset:2;
  unsigned int xdither_offset:2;
  unsigned int color_dither_enable:1;
  unsigned int alpha_test_func:3;
  unsigned int alpha_test_enable:1;
  unsigned int alpha_to_coverage_dither:1;
  unsigned int alpha_to_one:1;
  unsigned int ia_blend_enable:1;
  unsigned int alpha_to_coverage:1;
};

struct gen8_blend_state_rt {
  struct {
    unsigned int blue_write_dis:1;
    unsigned int green_write_dis:1;
    unsigned int red_write_dis:1;
    unsigned int alpha_write_dis:1;
    unsigned int pad0:1;
    unsigned int alpha_blend_func:3;
    unsigned int ia_dest_blend_factor:5;
    unsigned int ia_src_blend_factor:5;
    unsigned int color_blend_func:3;
    unsigned int dest_blend_factor:5;
    unsigned int src_blend_factor:5;
    unsigned int colorbuf_blend:1;
  } blend0;

  struct {
    unsigned int post_blend_clamp_enable:1;
    unsigned int pre_blend_clamp_enable:1;
    unsigned int clamp_range:2;
    unsigned int pre_blend_src_clamp:1;
    unsigned int pad0:22;
    unsigned int logic_op_func:4;
    unsigned int logic_op_enable:1;
  } blend1;
};

#define GEN8_3DSTATE_RASTER			RENDER_CMD(3, 0, 0x50)
# define GEN8_3DSTATE_RASTER_CULL_BOTH			(0 << 16)
# define GEN8_3DSTATE_RASTER_CULL_NONE			(1 << 16)
# define GEN8_3DSTATE_RASTER_CULL_FRONT			(2 << 16)
# define GEN8_3DSTATE_RASTER_CULL_BACK			(3 << 16)

/* Gen8 WM_HZ_OP */
#define GEN8_3DSTATE_WM_HZ_OP			RENDER_CMD(3, 0, 0x52)

#define GEN8_3DSTATE_MULTISAMPLE		RENDER_CMD(3, 0, 0x0d)
#define GEN8_3DSTATE_SAMPLE_PATTERN		RENDER_CMD(3, 1, 0x1C)

# define GEN8_PUSH_CONSTANT_BUFFER_OFFSET_SHIFT	16
# define GEN8_PUSH_CONSTANT_BUFFER_SIZE_SHIFT	0

# define GEN8_SBE_FORCE_URB_ENTRY_READ_LENGTH  (1 << 29)
# define GEN8_SBE_FORCE_URB_ENTRY_READ_OFFSET  (1 << 28)

# define GEN8_SBE_URB_ENTRY_READ_OFFSET_SHIFT   5
#define GEN8_3DSTATE_SBE_SWIZ                    RENDER_CMD(3, 0, 0x51)

# define GEN8_PS_MAX_THREADS_SHIFT                      23

#define GEN8_3DSTATE_PSEXTRA				RENDER_CMD(3, 0, 0x4f)
/* DW1 */
# define GEN8_PSX_PIXEL_SHADER_VALID                    (1 << 31)
# define GEN8_PSX_PSCDEPTH_OFF                          (0 << 26)
# define GEN8_PSX_PSCDEPTH_ON                           (1 << 26)
# define GEN8_PSX_PSCDEPTH_ON_GE                        (2 << 26)
# define GEN8_PSX_PSCDEPTH_ON_LE                        (3 << 26)
# define GEN8_PSX_ATTRIBUTE_ENABLE			(1 << 8)

#define GEN8_3DSTATE_PSBLEND				RENDER_CMD(3, 0, 0x4d)
/* DW1 */
# define GEN8_PS_BLEND_ALPHA_TO_COVERAGE_ENABLE         (1 << 31)
# define GEN8_PS_BLEND_HAS_WRITEABLE_RT                 (1 << 30)
# define GEN8_PS_BLEND_COLOR_BUFFER_BLEND_ENABLE        (1 << 29)
# define GEN8_PS_BLEND_SRC_ALPHA_BLEND_FACTOR_MASK      INTEL_MASK(28, 24)
# define GEN8_PS_BLEND_SRC_ALPHA_BLEND_FACTOR_SHIFT     24
# define GEN8_PS_BLEND_DST_ALPHA_BLEND_FACTOR_MASK      INTEL_MASK(23, 19)
# define GEN8_PS_BLEND_DST_ALPHA_BLEND_FACTOR_SHIFT     19
# define GEN8_PS_BLEND_SRC_BLEND_FACTOR_MASK            INTEL_MASK(18, 14)
# define GEN8_PS_BLEND_SRC_BLEND_FACTOR_SHIFT           14
# define GEN8_PS_BLEND_DST_BLEND_FACTOR_MASK            INTEL_MASK(13, 9)
# define GEN8_PS_BLEND_DST_BLEND_FACTOR_SHIFT           9
# define GEN8_PS_BLEND_ALPHA_TEST_ENABLE                (1 << 8)
# define GEN8_PS_BLEND_INDEPENDENT_ALPHA_BLEND_ENABLE   (1 << 7)

#define GEN8_3DSTATE_WM_DEPTH_STENCIL			RENDER_CMD(3, 0, 0x4e)

#define GEN8_VE0_VERTEX_BUFFER_INDEX_SHIFT      26 /* for GEN8 */
#define GEN8_VE0_VALID                  (1 << 25)  /* for GEN8 */
#define GEN8_VB0_BUFFER_INDEX_SHIFT     26
#define GEN8_VB0_MOCS_SHIFT		16

#define GEN8_3DSTATE_VF_TOPOLOGY	RENDER_CMD(3, 0, 0x4b)

#define GEN8_XY_COLOR_BLT_CMD                   (RCMD_2D | (0x50 << 22) | 0x05)

#define GEN7_3DSTATE_VF                 RENDER_CMD(3, 0, 0x0c)
#define GEN8_3DSTATE_VF_INSTANCING      RENDER_CMD(3, 0, 0x49)

#define GEN8_3DSTATE_VF_SGVS            RENDER_CMD(3, 0, 0x4a)
#endif /* _MEDIA_RENDER_COMMON_H_ */
