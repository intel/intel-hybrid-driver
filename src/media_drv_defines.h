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

#ifndef _MEDIA__DRIVER_DEFINES_H
#define _MEDIA__DRIVER_DEFINES_H
#include <stdbool.h>

#define FRAME_TYPE_I            1
#define FRAME_TYPE_P            2
#define FRAME_TYPE_B            3


#define CODEC_H264      0
#define CODEC_MPEG2     1
#define CODEC_VP8       3


#define VP8_DELIMITER0 0x00
#define VP8_DELIMITER1 0x00
#define VP8_DELIMITER2 0x00
#define VP8_DELIMITER3 0x00
#define VP8_DELIMITER4 0x0

#define NUM_SLICES     10

#define MEDIA_GEN_MAX_PROFILES                 16	// VAProfileH264Baseline, VAProfileH264Main,VAProfileH264High,VAProfileH264ConstrainedBaseline VAProfileMPEG2Main, VAProfileMPEG2Simple, VAProfileHEVCMain and VAProfileNone
#define MEDIA_GEN_MAX_ENTRYPOINTS              4	// VAEntrypointHybridEnc
#define MEDIA_GEN_MAX_CONFIG_ATTRIBUTES        46	// VAConfigAttribRTFormat plus VAConfigAttribRateControl
#define MEDIA_GEN_MAX_IMAGE_FORMATS            1	// NV12 only
#define MEDIA_GEN_MAX_SUBPIC_FORMATS           4	// no sub-pic blending support, still set to 4 for further implementation
#define MEDIA_GEN_MAX_DISPLAY_ATTRIBUTES       4	// Use the same value as I965
#define MEDIA_GEN_MAX_ATTRIBS_TYPE             4	//VAConfigAttribRTFormat,    VAConfigAttribRateControl,    VAConfigAttribDecSliceMode,    VAConfigAttribEncPackedHeaders

//
#define I965_MAX_SUBPIC_SUM                     4

#define BUFFER_ID_OFFSET                0x08000000
//
#define MAX_GPE_KERNELS    32
#define MAX_INTERFACE_DESC_GEN6      MAX_GPE_KERNELS
#define CURBE_ALLOCATION_SIZE   0X35	//37
#define CURBE_TOTAL_DATA_LENGTH  0x1C0	//0x140   //(4 * 35)       /* in byte, it should be less than or equal to CURBE_ALLOCATION_SIZE * 32 */
#define  CURBE_URB_ENTRY_LENGTH 4
//
#define SUBSAMPLE_YUV400        0
#define SUBSAMPLE_YUV420        1
#define SUBSAMPLE_YUV422H       2
#define SUBSAMPLE_YUV422V       3
#define SUBSAMPLE_YUV444        4
#define SUBSAMPLE_YUV411        5
#define SUBSAMPLE_RGBX          6
#define SUBSAMPLE_P208          7
#define SURFACE_REFERENCED      (1 << 0)
#define SURFACE_DISPLAYED       (1 << 1)
#define SURFACE_DERIVED         (1 << 2)

#define I965_SURFACE_MEM_NATIVE             0
#define I965_SURFACE_MEM_GEM_FLINK          1
#define I965_SURFACE_MEM_DRM_PRIME          2

#define INTEL_STR_DRIVER_VENDOR                 "Intel"
#define INTEL_STR_DRIVER_NAME                   "hybrid"

/* Major version of the driver */
#define INTEL_DRIVER_MAJOR_VERSION 0
/* Micro version of the driver */
#define INTEL_DRIVER_MICRO_VERSION 1
/* Minor version of the driver */
#define INTEL_DRIVER_MINOR_VERSION 0
/* Preversion of the driver */
#define INTEL_DRIVER_PRE_VERSION 0

#define DOUBLE double
#define FLOAT float
#define ULONG unsigned long
#define DWORD unsigned int
#define UINT unsigned int
#define UINT16 unsigned short
#define INT16 short
#define INT int
#define BYTE unsigned char
#define CHAR char
#define BOOL bool
#define STATUS bool
#define VOID void
#define TRUE 1
#define FALSE 0
#define ERROR -1


/*debug enable*/
#define SUCCESS 1
#define FAILED 0
//#define DEBUG 
//#define DEBUG_LEVEL1
#define CODEC_DEC       0
#define CODEC_ENC       1
#define CODEC_PROC      2


//VP*

// HME
#define SCALE_FACTOR_2x    2
#define SCALE_FACTOR_4x    4
#define SCALE_FACTOR_16x    16
#define SCALE_FACTOR_32x    32

//BRC
#define ENCODE_BRC_KBPS                         1000
#define BRC_IMG_STATE_SIZE_PER_PASS             128

#define WIDTH_IN_MACROBLOCKS(width)      (((width) + (16 - 1)) / 16)
#define HEIGHT_IN_MACROBLOCKS(height)    (((height) + (16 - 1)) / 16)


#define SCOREBOARD_STALLING     0
#define SCOREBOARD_NON_STALLING 1

//I965 DEFINES...
#define MEDIA_SURFACE_1D      0
#define MEDIA_SURFACE_2D      1
#define MEDIA__SURFACE_3D      2
#define MEDIA__SURFACE_CUBE    3
#define MEDIA_SURFACE_BUFFER  4
#define MEDIA_SURFACE_NULL    7

#define STATE_SURFACEFORMAT_R8_UNORM   0x140
#define STATE_SURFACEFORMAT_R16_UINT  0x10D
#define STATE_SURFACEFORMAT_RAW           0x1FF
#define  STATE_SURFACEFORMAT_R32_UINT     0x0D7

#define MEDIA_TILEWALK_XMAJOR                 0
#define MEDIA_TILEWALK_YMAJOR                 1

#define MAX_MEDIA_SURFACES_GEN6      34
#define SURFACE_STATE_PADDED_SIZE_0_GEN7        sizeof(struct gen7_surface_state)	/*ALIGN(sizeof(struct gen7_surface_state), 32) */
#define SURFACE_STATE_PADDED_SIZE_1_GEN7        sizeof(struct gen7_surface_state2)	/*ALIGN(sizeof(struct gen7_surface_state2), 32) */
#define SURFACE_STATE_PADDED_SIZE_GEN7          MAX(SURFACE_STATE_PADDED_SIZE_0_GEN7, SURFACE_STATE_PADDED_SIZE_1_GEN7)
#define SURFACE_STATE_PADDED_SIZE               SURFACE_STATE_PADDED_SIZE_GEN7	/*MAX(SURFACE_STATE_PADDED_SIZE_GEN6, SURFACE_STATE_PADDED_SIZE_GEN7) */

#define BINDING_TABLE_ENTRIES 0xc0
#define LAST_BINDING_TABLE_ENTRIES 0x20
#define BINDING_TABLE_OFFSET(index)  (/*ALIGN(sizeof(BINDING_TABLE_STATE),32)*/(sizeof(BINDING_TABLE_STATE)) * index)

#define SURFACE_STATE_SIZE  (ALIGN(sizeof(SURFACE_STATE_G7),32))
#define SURFACE_STATE_OFFSET(index) (BINDING_TABLE_OFFSET(BINDING_TABLE_ENTRIES)+(SURFACE_STATE_SIZE*index))
#define BINDING_TABLE_SURFACE_SHIFT 5


#define MFX_SURFACE_PLANAR_420_8        4
#define MFX_SURFACE_PLANAR_411_8        5
#define MFX_SURFACE_PLANAR_422_8        6


#define HISTOGRAM_SIZE                      (136 * sizeof(unsigned int))

#define     MBPAK_HYBRID_STATE_P1        1
#define    MBPAK_HYBRID_STATE_P2     2

#define VAEncMiscParameterTypeVP8SegmentMapParams -4

#define CACHEABILITY_TYPE_NONE 0
#define CACHEABILITY_TYPE_LLC 1
#define CACHEABILITY_TYPE_L3 2

#define VDIRECTION_FULL_FRAME 2

#define MBENC_ILUMA_START_OFFSET 0
#define MBENC_ICHROMA_START_OFFSET 1
#define MBENC_P_START_OFFSET 2
#define MBENC_I_START_OFFSET 3
#define MBENC_IFRAME_DIST_OFFSET 4

#define MBPAK_PHASE1_OFFSET 1
#define MBPAK_PHASE2_OFFSET 0

#define BRC_INIT_OFFSET         0
#define BRC_RESET_OFFSET        1

#define BRC_UPDATE_OFFSET       0

#define KERNEL_DUMP_SIZE_VP8                (600000 * sizeof(DWORD))

#define LAST_REF_FLAG_VP8					0x1
#define GOLDEN_REF_FLAG_VP8					0x2
#define ALT_REF_FLAG_VP8					0x4

#define NUM_BRC_IMG_STATE_READ_BUFFERS          3
#define NUM_BRC_CONSTANT_DATA_BUFFERS           3

#define BRC_CONSTANTSURFACE_WIDTH_G75           64
#define BRC_CONSTANTSURFACE_HEIGHT_G75          43

#endif
