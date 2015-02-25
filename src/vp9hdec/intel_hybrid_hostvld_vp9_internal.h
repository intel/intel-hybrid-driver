/*
 * Copyright © 2014 Intel Corporation
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
 *     Zhao Yakui <yakui.zhao@intel.com>
 *
 */

#ifndef __INTEL_HOSTVLD_VP9_INTERNAL_H__
#define __INTEL_HOSTVLD_VP9_INTERNAL_H__

#include "media_drv_driver.h"
#include <pthread.h>
#include <semaphore.h>
#include "cmrt_api.h"
#include "intel_hybrid_hostvld_vp9.h"
#include "intel_hybrid_common_vp9.h"

// Macro to enable separated loop filter
#define SEPERATE_LOOPFILTER_ENABLE

#define VP9_COEF_BANDS          6 // Middle dimension reflects the coefficient position within the transform.
#define VP9_PREV_COEF_CONTEXTS  6
#define VP9_UNCONSTRAINED_NODES 3
#define VP9_MAX_SEGMENTS        8
#define VP9_BLK_SIZE_GROUPS     4
#define VP9_MBSKIP_CONTEXTS     3
#define VP9_PARTITION_PLOFFSET  4  // number of probability models per block size
#define VP9_PARTITION_CONTEXTS  (4 * VP9_PARTITION_PLOFFSET)
#define VP9_TX_SIZE_CONTEXTS    2
#define VP9_TOKEN_CACHE_SIZE    1024
#define VP9_MAX_PROB            255

#define VP9_SEG_PRED_PROBS              3
#define VP9_INTER_MODE_CONTEXTS         7
#define VP9_SWITCHABLE_FILTERS          3
#define VP9_SWITCHABLE_FILTER_CONTEXTS  (VP9_SWITCHABLE_FILTERS + 1)
#define VP9_INTRA_INTER_CONTEXTS        4
#define VP9_COMPOUND_INTER_CONTEXTS     5
#define VP9_REF_CONTEXTS                5

#define VP9_MAX_TILE_ROWS       4
#define VP9_MAX_TILE_COLUMNS    64
#define VP9_MAX_TILES           (VP9_MAX_TILE_ROWS * VP9_MAX_TILE_COLUMNS)

#define VP9_LOG2_B64_SIZE       6
#define VP9_LOG2_B8_SIZE        3
#define VP9_LOG2_B4_SIZE        2
#define VP9_LOG2_B64_SIZE_IN_B8 (VP9_LOG2_B64_SIZE - VP9_LOG2_B8_SIZE)
#define VP9_LOG2_B64_SIZE_IN_B4 (VP9_LOG2_B64_SIZE - VP9_LOG2_B4_SIZE)
#define VP9_B64_SIZE            (1 << VP9_LOG2_B64_SIZE)
#define VP9_B64_SIZE_IN_B8      (1 << VP9_LOG2_B64_SIZE_IN_B8)
#define VP9_B64_SIZE_IN_B4      (1 << VP9_LOG2_B64_SIZE_IN_B4)

#define VP9_MV_CLASS0_BITS      1
#define VP9_MV_CLASS0_SIZE      (1 << VP9_MV_CLASS0_BITS)
#define VP9_MV_OFFSET_BITS      (VP9_MV_CLASSES + VP9_MV_CLASS0_BITS - 2)
#define VP9_MAX_MV_REF_CANDIDATES   2
#define VP9_LOG2_MV_REF_NEIGHBOURS  3
#define VP9_MV_REF_NEIGHBOURS       (1 << VP9_LOG2_MV_REF_NEIGHBOURS)
#define VP9_MV_BORDER               (16 << 4)  // allow 16 pixels in unit of 1/8th pixel 
#define VP9_MV_MARGIN               (156 << 4)


#define VP9_COMPANDED_MVREF_THRESH  8


// Coefficient token alphabet
#define VP9_ZERO_TOKEN              0       /* 0         Extra Bits 0+0 */
#define VP9_ONE_TOKEN               1       /* 1         Extra Bits 0+1 */
#define VP9_TWO_TOKEN               2       /* 2         Extra Bits 0+1 */
#define VP9_THREE_TOKEN             3       /* 3         Extra Bits 0+1 */
#define VP9_FOUR_TOKEN              4       /* 4         Extra Bits 0+1 */
#define VP9_DCT_VAL_CATEGORY1       5       /* 5-6       Extra Bits 1+1 */
#define VP9_DCT_VAL_CATEGORY2       6       /* 7-10      Extra Bits 2+1 */
#define VP9_DCT_VAL_CATEGORY3       7       /* 11-18     Extra Bits 3+1 */
#define VP9_DCT_VAL_CATEGORY4       8       /* 19-34     Extra Bits 4+1 */
#define VP9_DCT_VAL_CATEGORY5       9       /* 35-66     Extra Bits 5+1 */
#define VP9_DCT_VAL_CATEGORY6       10      /* 67+       Extra Bits 14+1 */
#define VP9_DCT_EOB_TOKEN           11      /* EOB       Extra Bits 0+0 */
#define VP9_MAX_ENTROPY_TOKENS      12
#define VP9_ENTROPY_NODES           11

#define VP9_DCT_EOB_MODEL_TOKEN     3       /* EOB       Extra Bits 0+0 */

#define VP9_MAX_LOOP_FILTER         63

#define VP9_TKN_TREE_SZ(NUM_LEAFS) (2 * (NUM_LEAFS) - 1)

#define INTEL_HOSTVLD_VP9_READ_BIT(Prob)    Intel_HostvldVp9_BacEngineReadBit(pBacEngine, Prob)
#define INTEL_HOSTVLD_VP9_READ_ONE_BIT      Intel_HostvldVp9_BacEngineReadSingleBit(pBacEngine)
#define INTEL_HOSTVLD_VP9_READ_BITS(Bits)   Intel_HostvldVp9_BacEngineReadMultiBits(pBacEngine, Bits)
#define INTEL_HOSTVLD_VP9_READ_TREE(Tree)   Intel_HostvldVp9_BacEngineReadTree(pBacEngine, Tree)
#define INTEL_HOSTVLD_VP9_READ_DWORD(Data)   (((Data)[0] << 24) | ((Data)[1] << 16) | ((Data)[2] << 8) | (Data)[3])

#define INTEL_VP9_CLAMP(Value, Min, Max) \
    ((Value) < (Min) ? (Min) : ((Value) > (Max) ? (Max) : (Value)))

#define INTEL_VP9_ROUND_POWER_OF_TWO(Value, n) \
    (((Value) + (1 << ((n) - 1))) >> (n))

#define INTEL_HOSTVLD_VP9_INTRA_MODE_PROB_TREE(p0, p1, p2, p3, p4, p5, p6, p7, p8)\
    {\
        {-1, p0}, {PRED_MD_DC, 0}, {-1, p1}, {PRED_MD_TM, 0}, {-1, p2}, {PRED_MD_V, 0}, {-1, p3},\
        {-2, p4}, {-5, p6}, {PRED_MD_H, 0}, {-1, p5}, {PRED_MD_D135, 0}, {PRED_MD_D117, 0},\
        {PRED_MD_D45, 0}, {-1, p7}, {PRED_MD_D63, 0}, {-1, p8}, {PRED_MD_D153, 0}, {PRED_MD_D207, 0}\
    }
#define INTEL_HOSTVLD_VP9_SEGMENT_PROB_TREE(p0, p1, p2, p3, p4, p5, p6)\
    {\
        {-1, p0}, {-2, p1}, {-3, p2}, {-4, p3}, {-5, p4}, {-6, p5}, {-7, p6},\
        {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}\
    }
#define INTEL_HOSTVLD_VP9_PARTITION_PROB_TREE(p0, p1, p2)\
    {\
        {-1, p0}, {PARTITION_NONE, 0}, {-1, p1}, {PARTITION_HORZ, 0}, {-1, p2},\
        {PARTITION_VERT, 0}, {PARTITION_SPLIT, 0}\
    }

typedef enum 
{
    VP9_CODED_YUV_PLANE_Y, 
    VP9_CODED_YUV_PLANE_U, 
    VP9_CODED_YUV_PLANE_V, 
    VP9_CODED_YUV_PLANES
} INTEL_HOSTVLD_VP9_CODED_YUV_PLANE;

typedef enum {
    KEY_FRAME   = 0,
    INTER_FRAME = 1,
    FRAME_TYPES,
} INTEL_HOSTVLD_VP9_FRAME_TYPE;

typedef enum
{
    BLOCK_4X4       = 0,
    BLOCK_8X8,
    BLOCK_16X16,
    BLOCK_32X32,
    BLOCK_64X64,
    BLOCK_8X4       = 5,
    BLOCK_16X8,
    BLOCK_32X16,
    BLOCK_64X32,
    BLOCK_4X8       = 9,
    BLOCK_8X16,
    BLOCK_16X32,
    BLOCK_32X64,
    BLOCK_SIZES,
    BLOCK_INVALID = BLOCK_SIZES
} INTEL_HOSTVLD_VP9_BLOCK_SIZE;

typedef enum
{
    PARTITION_NONE,
    PARTITION_HORZ,
    PARTITION_VERT,
    PARTITION_SPLIT,
    PARTITION_TYPES,
    PARTITION_INVALID = PARTITION_TYPES
} INTEL_HOSTVLD_VP9_PARTITION_TYPE;

typedef enum
{
    INTRA = 0,
    INTER = 1,
    REF_TYPES
} INTEL_HOSTVLD_VP9_PREDICTION_TYPE;

typedef enum
{
    PRED_MD_DC,
    PRED_MD_V,
    PRED_MD_H,
    PRED_MD_D45,
    PRED_MD_D135,
    PRED_MD_D117,
    PRED_MD_D153,
    PRED_MD_D207,
    PRED_MD_D63,
    PRED_MD_TM,
    INTRA_MODE_COUNT,
    PRED_MD_NEARESTMV = INTRA_MODE_COUNT,
    PRED_MD_NEARMV,
    PRED_MD_ZEROMV,
    PRED_MD_NEWMV,
    INTER_MODE_COUNT = PRED_MD_NEWMV    - PRED_MD_TM,
    MB_MODE_COUNT    = INTER_MODE_COUNT + INTRA_MODE_COUNT
} INTEL_HOSTVLD_VP9_MB_PRED_MODE;

typedef enum
{
    TX_4X4   = 0,   // 4x4 dct transform
    TX_8X8   = 1,   // 8x8 dct transform
    TX_16X16 = 2,   // 16x16 dct transform
    TX_32X32 = 3,   // 32x32 dct transform
    TX_SIZES
} INTEL_HOSTVLD_VP9_TX_SIZE;

typedef enum {
    ONLY_4X4            = 0,
    ALLOW_8X8           = 1,
    ALLOW_16X16         = 2,
    ALLOW_32X32         = 3,
    TX_MODE_SELECT      = 4,
    TX_MODES            = 5,
} INTEL_HOSTVLD_VP9_TX_MODE;

typedef enum
{
    TX_DCT      = 0,   // DCT  in both horizontal and vertical
    TX_ADST_DCT = 1,   // ADST in vertical, DCT in horizontal
    TX_DCT_ADST = 2,   // DCT  in vertical, ADST in horizontal
    TX_ADST     = 3,   // ADST in both directions
    TX_LOSSLESS = 4,
    TX_TYPE_COUNT
} INTEL_HOSTVLD_VP9_TX_TYPE;

typedef enum
{
    SEG_LVL_ALT_Q     = 0,  // Use alternate Quantizer ....
    SEG_LVL_ALT_LF    = 1,  // Use alternate loop filter value...
    SEG_LVL_REF_FRAME = 2,  // Optional Segment reference frame
    SEG_LVL_SKIP      = 3,  // Optional Segment (0,0) + skip mode
    SEG_LVL_MAX       = 4   // Number of features supported
} INTEL_HOSTVLD_VP9_SEG_LVL;

// interpolation filter type
typedef enum 
{
    VP9_INTERP_EIGHTTAP         = 0,
    VP9_INTERP_EIGHTTAP_SMOOTH  = 1,
    VP9_INTERP_EIGHTTAP_SHARP   = 2,
    VP9_INTERP_BILINEAR         = 3,
    VP9_INTERP_SWITCHABLE       = 4
} INTEL_HOSTVLD_VP9_INTERPOLATION_TYPE;

typedef enum 
{
    VP9_REF_FRAME_NONE  = -2,
    VP9_REF_FRAME_INTRA = -1,
    VP9_REF_FRAME_LAST  = 0,
    VP9_REF_FRAME_GOLDEN,
    VP9_REF_FRAME_ALTREF,
    VP9_REF_FRAME_MAX
} INTEL_HOSTVLD_VP9_REF_FRAME, *PINTEL_HOSTVLD_VP9_REF_FRAME;

typedef enum 
{
    VP9_SINGLE_PREDICTION_ONLY = 0,
    VP9_COMPOUND_PREDICTION_ONLY,
    VP9_HYBRID_PREDICTION
} INTEL_HOSTVLD_VP9_PREDICTION_MODE_TYPE;

typedef enum
{
    VP9_MV_HORIZONTAL, 
    VP9_MV_VERTICAL,
    VP9_MV_COMPONENTS
} INTEL_HOSTVLD_VP9_MV_COMPONENT;

typedef enum 
{
    VP9_MV_JOINT_ZERO       = 0,  
    VP9_MV_JOINT_HNZ_VZ     = 1 << VP9_MV_HORIZONTAL, 
    VP9_MV_JOINT_HZ_VNZ     = 1 << VP9_MV_VERTICAL, 
    VP9_MV_JOINT_HNZ_VNZ    = VP9_MV_JOINT_HNZ_VZ | VP9_MV_JOINT_HZ_VNZ,
    VP9_MV_JOINTS
} INTEL_HOSTVLD_VP9_MV_JOINT_TYPE;

typedef enum 
{
    VP9_MV_CLASS_0  = 0, 
    VP9_MV_CLASS_1  = 1, 
    VP9_MV_CLASS_2  = 2, 
    VP9_MV_CLASS_3  = 3, 
    VP9_MV_CLASS_4  = 4, 
    VP9_MV_CLASS_5  = 5, 
    VP9_MV_CLASS_6  = 6, 
    VP9_MV_CLASS_7  = 7, 
    VP9_MV_CLASS_8  = 8, 
    VP9_MV_CLASS_9  = 9, 
    VP9_MV_CLASS_10 = 10,
    VP9_MV_CLASSES
} INTEL_HOSTVLD_VP9_MV_CLASS_TYPE;

typedef enum {
    VP9_MV_CONTEXT_BOTH_ZERO            = 0,
    VP9_MV_CONTEXT_ZERO_PLUS_PREDICTED  = 1,
    VP9_MV_CONTEXT_BOTH_PREDICTED       = 2,
    VP9_MV_CONTEXT_NEW_PLUS_NON_INTRA   = 3,
    VP9_MV_CONTEXT_BOTH_NEW             = 4,
    VP9_MV_CONTEXT_INTRA_PLUS_NON_INTRA = 5,
    VP9_MV_CONTEXT_BOTH_INTRA           = 6,
    VP9_MV_CONTEXT_INVALID_CASE         = 9
} INTEL_HOSTVLD_VP9_MV_CONTEXT;

typedef struct _INTEL_HOSTVLD_VP9_PARTITION_PROBS
{
    BYTE Prob[4];
} INTEL_HOSTVLD_VP9_PARTITION_PROBS, *PINTEL_HOSTVLD_VP9_PARTITION_PROBS;

// BAC engine definitions
typedef DWORD INTEL_HOSTVLD_VP9_BAC_VALUE;

typedef struct _INTEL_HOSTVLD_VP9_BAC_ENGINE
{
    PBYTE                          pBuf;
    PBYTE                          pBufEnd;
    INTEL_HOSTVLD_VP9_BAC_VALUE BacValue;
    INT                            iCount;
    UINT                           uiRange;
} INTEL_HOSTVLD_VP9_BAC_ENGINE, *PINTEL_HOSTVLD_VP9_BAC_ENGINE;

typedef struct _INTEL_HOSTVLD_VP9_TKN_TREE_NODE
{
    union
    {
        INT8 i8Token;
        INT8 i8Offset;
    };
    UINT8 ui8Prob;
} INTEL_HOSTVLD_VP9_TKN_TREE_NODE, *PINTEL_HOSTVLD_VP9_TKN_TREE_NODE, *INTEL_HOSTVLD_VP9_TKN_TREE;

typedef INTEL_HOSTVLD_VP9_TKN_TREE_NODE INTEL_HOSTVLD_VP9_INTRA_MODE_TREE[VP9_TKN_TREE_SZ(INTRA_MODE_COUNT)];
typedef INTEL_HOSTVLD_VP9_TKN_TREE_NODE INTEL_HOSTVLD_VP9_SEGMENT_TREE[VP9_TKN_TREE_SZ(VP9_MAX_SEGMENTS)];
typedef INTEL_HOSTVLD_VP9_TKN_TREE_NODE INTEL_HOSTVLD_VP9_PARTITION_TREE[VP9_TKN_TREE_SZ(PARTITION_TYPES)];
typedef struct _INTEL_HOSTVLD_VP9_STATE INTEL_HOSTVLD_VP9_STATE, *PINTEL_HOSTVLD_VP9_STATE;
typedef struct _INTEL_HOSTVLD_VP9_FRAME_STATE INTEL_HOSTVLD_VP9_FRAME_STATE, *PINTEL_HOSTVLD_VP9_FRAME_STATE;
typedef struct _INTEL_HOSTVLD_VP9_TILE_STATE INTEL_HOSTVLD_VP9_TILE_STATE, *PINTEL_HOSTVLD_VP9_TILE_STATE;

typedef union _INTEL_HOSTVLD_VP9_MV
{
    struct  
    {
        INT16 i16X;
        INT16 i16Y;
    };
    DWORD dwValue;
} INTEL_HOSTVLD_VP9_MV, *PINTEL_HOSTVLD_VP9_MV;

// Segmentation feature
typedef struct _INTEL_HOSTVLD_VP9_SEG_FEATURE
{
    INT16 Data[SEG_LVL_MAX];
    UINT  uiMask;
} INTEL_HOSTVLD_VP9_SEG_FEATURE, *PINTEL_HOSTVLD_VP9_SEG_FEATURE;

typedef struct _INTEL_HOSTVLD_VP9_MV_PROB_SET
{
    UINT8   MvSignProbs;
    UINT8   MvClassProbs[VP9_MV_CLASSES - 1];
    UINT8   MvClass0Probs[VP9_MV_CLASS0_SIZE - 1];
    UINT8   MvBitsProbs[VP9_MV_OFFSET_BITS];
    UINT8   MvClass0FpProbs[VP9_MV_CLASS0_SIZE][4 - 1];
    UINT8   MvFpProbs[4 - 1];
    UINT8   MvClass0HpProbs;
    UINT8   MvHpProbs;
} INTEL_HOSTVLD_VP9_MV_PROB_SET, *PINTEL_HOSTVLD_VP9_MV_PROB_SET;

typedef struct _INTEL_HOSTVLD_VP9_MV_COUNT_SET
{
    UINT    MvSignCounts[2];
    UINT    MvClassCounts[VP9_MV_CLASSES];
    UINT    MvClass0Counts[VP9_MV_CLASS0_SIZE];
    UINT    MvBitsCounts[VP9_MV_OFFSET_BITS][2];
    UINT    MvClass0FpCounts[VP9_MV_CLASS0_SIZE][4];
    UINT    MvFpCounts[4];
    UINT    MvClass0HpCounts[2];
    UINT    MvHpCounts[2];
} INTEL_HOSTVLD_VP9_MV_COUNT_SET, *PINTEL_HOSTVLD_VP9_MV_COUNT_SET;

typedef struct _INTEL_HOSTVLD_VP9_TX_PROB_TABLE_SET
{
    UINT8 Tx_32X32[VP9_TX_SIZE_CONTEXTS][TX_32X32];
    UINT8 Tx_16X16[VP9_TX_SIZE_CONTEXTS][TX_16X16];
    UINT8 Tx_8X8[VP9_TX_SIZE_CONTEXTS][TX_8X8];
} INTEL_HOSTVLD_VP9_TX_PROB_TABLE_SET, *PINTEL_HOSTVLD_VP9_TX_PROB_TABLE_SET;

typedef struct _INTEL_HOSTVLD_VP9_TX_COUNT_TABLE_SET
{
    UINT Tx_32X32[VP9_TX_SIZE_CONTEXTS][TX_32X32 + 1];
    UINT Tx_16X16[VP9_TX_SIZE_CONTEXTS][TX_16X16 + 1];
    UINT Tx_8X8[VP9_TX_SIZE_CONTEXTS][TX_8X8 + 1];
} INTEL_HOSTVLD_VP9_TX_COUNT_TABLE_SET, *PINTEL_HOSTVLD_VP9_TX_COUNT_TABLE_SET;

typedef struct _INTEL_HOSTVLD_VP9_TX_PROB_TABLE
{
    PUINT8 pui8ProbTable;
    UINT   uiStride;
} INTEL_HOSTVLD_VP9_TX_PROB_TABLE;
typedef struct _INTEL_HOSTVLD_VP9_TX_COUNT_TABLE
{
    PUINT puiCountTable;
    UINT  uiStride;
} INTEL_HOSTVLD_VP9_TX_COUNT_TABLE;

typedef UINT8 INTEL_HOSTVLD_VP9_COEFF_PROBS_MODEL[REF_TYPES][VP9_COEF_BANDS]
                                      [VP9_PREV_COEF_CONTEXTS]
                                      [VP9_UNCONSTRAINED_NODES];

typedef UINT INTEL_HOSTVLD_VP9_COEFF_COUNT_MODEL[REF_TYPES][VP9_COEF_BANDS]
                                          [VP9_PREV_COEF_CONTEXTS]
                                          [VP9_UNCONSTRAINED_NODES + 1];
typedef UINT INTEL_HOSTVLD_VP9_EOB_BRANCH_COUNT_MODEL[REF_TYPES][VP9_COEF_BANDS][VP9_PREV_COEF_CONTEXTS];

typedef struct _INTEL_HOSTVLD_VP9_COUNT
{
    INTEL_HOSTVLD_VP9_COEFF_COUNT_MODEL      CoeffCounts[TX_SIZES][INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER];
    INTEL_HOSTVLD_VP9_EOB_BRANCH_COUNT_MODEL EobBranchCounts[TX_SIZES][INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER];
    INTEL_HOSTVLD_VP9_TX_COUNT_TABLE_SET     TxCountSet;

    UINT    IntraModeCounts_Y[VP9_BLK_SIZE_GROUPS][INTRA_MODE_COUNT];
    UINT    IntraModeCounts_UV[INTRA_MODE_COUNT][INTRA_MODE_COUNT];
    UINT    MbSkipCounts[VP9_MBSKIP_CONTEXTS][2];
    UINT    PartitionCounts[VP9_PARTITION_CONTEXTS][PARTITION_TYPES];
    UINT    InterModeCounts[VP9_INTER_MODE_CONTEXTS][INTER_MODE_COUNT];
    UINT    SwitchableInterpCounts[VP9_SWITCHABLE_FILTER_CONTEXTS][VP9_SWITCHABLE_FILTERS];
    UINT    IntraInterCounts[VP9_INTRA_INTER_CONTEXTS][2];
    UINT    CompoundInterCounts[VP9_COMPOUND_INTER_CONTEXTS][2];
    UINT    SingleRefCounts[VP9_REF_CONTEXTS][2][2];
    UINT    CompoundRefCounts[VP9_REF_CONTEXTS][2];

    UINT                                MvJointCounts[VP9_MV_JOINTS];
    INTEL_HOSTVLD_VP9_MV_COUNT_SET   MvCountSet[VP9_MV_COMPONENTS];
} INTEL_HOSTVLD_VP9_COUNT, *PINTEL_HOSTVLD_VP9_COUNT;

typedef struct _INTEL_HOSTVLD_VP9_FRAME_CONTEXT
{
    INTEL_HOSTVLD_VP9_INTRA_MODE_TREE    ModeTree_Y[VP9_BLK_SIZE_GROUPS];
    INTEL_HOSTVLD_VP9_INTRA_MODE_TREE    ModeTree_UV[INTRA_MODE_COUNT];
    INTEL_HOSTVLD_VP9_SEGMENT_TREE       SegmentTree;
    INTEL_HOSTVLD_VP9_PARTITION_PROBS    PartitionProbs[VP9_PARTITION_CONTEXTS];

    INTEL_HOSTVLD_VP9_TX_PROB_TABLE_SET  TxProbTableSet;
    INTEL_HOSTVLD_VP9_TX_PROB_TABLE      TxProbTables[TX_SIZES]; //{NULL, table_set.Tx_8X8, table_set.Tx_16X16, table_set.Tx_32X32}
    UINT8                                   MbSkipProbs[VP9_MBSKIP_CONTEXTS];
    INTEL_HOSTVLD_VP9_COEFF_PROBS_MODEL  CoeffProbs[TX_SIZES][INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER];

    // context for inter
    UINT8   SegPredProbs[VP9_SEG_PRED_PROBS];
    UINT8   InterModeProbs[VP9_INTER_MODE_CONTEXTS][INTER_MODE_COUNT - 1];
    UINT8   SwitchableInterpProbs[VP9_SWITCHABLE_FILTER_CONTEXTS][VP9_SWITCHABLE_FILTERS - 1];
    UINT8   IntraInterProbs[VP9_INTRA_INTER_CONTEXTS];
    UINT8   CompoundInterProbs[VP9_COMPOUND_INTER_CONTEXTS];
    UINT8   SingleRefProbs[VP9_REF_CONTEXTS][2];
    UINT8   CompoundRefProbs[VP9_REF_CONTEXTS];
    // mv context
    UINT8                               MvJointProbs[VP9_MV_JOINTS - 1];
    INTEL_HOSTVLD_VP9_MV_PROB_SET    MvProbSet[VP9_MV_COMPONENTS];

} INTEL_HOSTVLD_VP9_FRAME_CONTEXT, *PINTEL_HOSTVLD_VP9_FRAME_CONTEXT;

typedef struct _INTEL_HOSTVLD_VP9_CONTEXT
{
    PINTEL_HOSTVLD_VP9_COUNT         pCurrCount;
    INTEL_HOSTVLD_VP9_FRAME_CONTEXT  CurrContext;
    INTEL_HOSTVLD_VP9_FRAME_CONTEXT  ContextTable[4];
} INTEL_HOSTVLD_VP9_CONTEXT, *PINTEL_HOSTVLD_VP9_CONTEXT;

typedef struct _INTEL_HOSTVLD_VP9_LOOP_FILTER_MASK
{
  UINT64    LeftY[TX_SIZES];
  UINT64    AboveY[TX_SIZES];
  UINT64    Int4x4Y;
  UINT16    LeftUv[TX_SIZES];
  UINT16    AboveUv[TX_SIZES];
  UINT16    Int4x4Uv;
  UINT8     LfLevelY[64];
  UINT8     LfLevelUv[16];
} INTEL_HOSTVLD_VP9_LOOP_FILTER_MASK, *PINTEL_HOSTVLD_VP9_LOOP_FILTER_MASK;

typedef struct _INTEL_HOSTVLD_VP9_TILE_INFO
{
    DWORD                           dwTileWidth;      // tile width in 8x8 block
    DWORD                           dwTileHeight;     // tile height in 8x8 block
    DWORD                           dwTileTop;        // tile top index in 8x8 block
    DWORD                           dwTileLeft;       // tile left index in 8x8 block
    INTEL_HOSTVLD_VP9_1D_BUFFER  BitsBuffer;

    // Segment context for partition decode
    UINT8                           SegContextLeft[8];

    // Context for predicted segment id
    UINT8                           PredSegIdContextLeft[8];

    // Entropy context for coeff decode
    UINT8                           pEntropyContextLeft[VP9_CODED_YUV_PLANES][16];
    UINT8                           TokenCache[VP9_TOKEN_CACHE_SIZE];
} INTEL_HOSTVLD_VP9_TILE_INFO, *PINTEL_HOSTVLD_VP9_TILE_INFO;

// MB level info
typedef struct _INTEL_HOSTVLD_VP9_MB_INFO
{
    INTEL_HOSTVLD_VP9_BLOCK_SIZE BlockSize; // MB block type

    // 8x8 token
    PUINT8  pSegmentId;
    PUINT8  pLastSegmentId;
    PUINT8  pSkipCoeff;
    PUINT8  pIsInter;
    PUINT8  pBlockSize;
    PUINT8  pTxSizeLuma;
    PUINT8  pTxSizeChroma;
    PUINT8  pPredModeChroma;
    PUINT16 pReferenceFrame;
    PUINT8  pFilterType;
    PUINT16 pPrevRefFrame;
    PUINT32 pQPLuma;
    PUINT32 pQPChroma;

    // 4x4 token
    PUINT8                   pTxTypeLuma;
    PUINT8                   pPredModeLuma;
    PINTEL_HOSTVLD_VP9_MV pMotionVector; // 2 MVs per 4x4 block
    PINTEL_HOSTVLD_VP9_MV pPrevMv;       // 2 MVs per 4x4 block

    PINTEL_HOSTVLD_VP9_TILE_INFO   pCurrTile;

    // 1D offset for current MB in zigzag order in unit of 8x8 block
    DWORD dwMbOffset;

    // distance between the head of the current line and
    // the end of the last line in 8x8 token buffer
    DWORD dwLineDist;

    // MB position in unit of 8x8 block
    DWORD dwMbPosX;
    DWORD dwMbPosY;

    // MB position in 64x64, 8x8 granularity
    INT iMbPosInB64X;
    INT iMbPosInB64Y;

    INT  iB4Number;
    INT8 i8ZOrder;
    INT  iLCtxOffset;
    INT  iACtxOffset;
    BOOL bAboveValid;
    BOOL bLeftValid;
    BOOL bRightValid;
    BOOL bSegRefSkip;
    BOOL bSegRefEnabled;
    INT8 i8SegReference;
    BOOL bIsSkipped;
    BOOL bIsInter;

    INT8 ReferenceFrame[2];
    
    INTEL_HOSTVLD_VP9_MB_PRED_MODE ePredMode;
    INTEL_HOSTVLD_VP9_MB_PRED_MODE ePredModeChroma;

    INTEL_HOSTVLD_VP9_MV Mv[2];
    INTEL_HOSTVLD_VP9_MV BestMv[2];
    INTEL_HOSTVLD_VP9_MV NearestMv[2];
    INTEL_HOSTVLD_VP9_MV NearMv[2];

    INTEL_HOSTVLD_VP9_INTERPOLATION_TYPE eInterpolationType;

    // Entropy context for Above and Left
    PUINT8 pAboveContext[VP9_CODED_YUV_PLANES]; // TOCHECK: when and how to initialize these entropy contexts?
    PUINT8 pLeftContext[VP9_CODED_YUV_PLANES];

    // Loop filter
    PINTEL_HOSTVLD_VP9_LOOP_FILTER_MASK pLoopFilterMaskSB;
} INTEL_HOSTVLD_VP9_MB_INFO, *PINTEL_HOSTVLD_VP9_MB_INFO;

// Frame level info
typedef struct _INTEL_HOSTVLD_VP9_FRAME_INFO
{
    // Currently only support subsampling_x=subsampling_y=1 for Chroma plane
    // INT iChromaPlaneSubSamplingX;
    // INT iChromaPlaneSubSamplingY;

    PINTEL_VP9_PIC_PARAMS        pPicParams;

    // Segmentation data
    PINTEL_VP9_SEGMENT_PARAMS    pSegmentData;
    UINT8 ui8SegEnabled;
    UINT8 ui8SegUpdMap;
    UINT8 ui8TemporalUpd;

    DWORD dwPicWidth;
    DWORD dwPicHeight;
    DWORD dwPicWidthCropped;
    DWORD dwPicHeightCropped;
    DWORD dwPicWidthAligned;
    DWORD dwLog2TileRows;
    DWORD dwLog2TileColumns;
    DWORD dwTileRows;
    DWORD dwTileColumns;
    DWORD dwB8Rows;
    DWORD dwB8RowsAligned;
    DWORD dwB8Columns;
    DWORD dwB8ColumnsAligned;
    BOOL  bIsKeyFrame;

    // Partition
    INTEL_HOSTVLD_VP9_1D_BUFFER FirstPartition;
    INTEL_HOSTVLD_VP9_1D_BUFFER SecondPartition;

    DWORD dwMbStride;
    BOOL  bLossLess;
    BOOL  bIsIntraOnly;
    BOOL  bFrameParallelDisabled;
    BOOL  bErrorResilientMode;
    BOOL  bShowFrame;
    BOOL  bResetContext;
    UINT  uiFrameContextIndex;
    UINT  uiResetFrameContext;
    UINT32 SegQP[VP9_MAX_SEGMENTS][INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER];

    INTEL_HOSTVLD_VP9_TX_MODE     TxMode;
    INTEL_HOSTVLD_VP9_FRAME_TYPE  LastFrameType;

    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext;
	INTEL_HOSTVLD_VP9_TILE_INFO      TileInfo[VP9_MAX_TILES];

	// Number of elements for above context in 8x8 blocks
	DWORD dwNumAboveCtx;

    // Segment context for partition decode
    PUINT8 pSegContextAbove;

    // Context for predicted segment id
    PUINT8 pPredSegIdContextAbove;

    // Entropy context for coeff decode
    PUINT8 pEntropyContextAbove[VP9_CODED_YUV_PLANES];

    // Inter
    BOOL    bIsSwitchableInterpolation;
    BOOL    bAllowHighPrecisionMv;
    BOOL    RefFrameSignBias[VP9_REF_FRAME_MAX];
    BOOL    bHasPrevFrame;
    DWORD   dwPredictionMode;
    INTEL_HOSTVLD_VP9_REF_FRAME CompondFixedRef;
    INTEL_HOSTVLD_VP9_REF_FRAME CompondVarRef[2];
    INTEL_HOSTVLD_VP9_INTERPOLATION_TYPE eInterpolationType;

    VAStatus (* pfnParseFrmModeInfo) (
        PINTEL_HOSTVLD_VP9_TILE_STATE   pTileState);
} INTEL_HOSTVLD_VP9_FRAME_INFO, *PINTEL_HOSTVLD_VP9_FRAME_INFO;


// Multi-threading
typedef struct _INTEL_HOSTVLD_VP9_MULTI_THREAD
{
    BOOL        bTileParallel;
    BOOL        bFrameParallel;
    BOOL        bParserMDParallel;

    DWORD       dwParserThreadNumber;

    // Thread handles
    HANDLE* phParserThread;
    HANDLE  hMDThread;

    // Thread sync
    HANDLE* phParserThreadStart;
    HANDLE* phParserThreadFinish;
    HANDLE  phMDThreadStart;
    HANDLE  phMDThreadFinish;
} INTEL_HOSTVLD_VP9_MULTI_THREAD, *PINTEL_HOSTVLD_VP9_MULTI_THREAD;

typedef struct _INTEL_HOSTVLD_VP9_TASK_USERDATA
{
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pVideoBuffer;
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER pOutputBuffer;
    
    DWORD                               dwCurrIndex;
    DWORD                               dwPrevIndex;
} INTEL_HOSTVLD_VP9_TASK_USERDATA, *PINTEL_HOSTVLD_VP9_TASK_USERDATA;

struct _INTEL_HOSTVLD_VP9_TILE_STATE
{
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState;
    INTEL_HOSTVLD_VP9_BAC_ENGINE     BacEngine;
    INTEL_HOSTVLD_VP9_MB_INFO        MbInfo;
    INTEL_HOSTVLD_VP9_COUNT          Count;
    DWORD                               dwCurrColIndex;
    DWORD                               dwTileStateNumber;
    DWORD                               dwTileColumns;
};

struct _INTEL_HOSTVLD_VP9_FRAME_STATE
{
    PINTEL_HOSTVLD_VP9_STATE         pVp9HostVld;
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pVideoBuffer;
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER pOutputBuffer;
    INTEL_HOSTVLD_VP9_BAC_ENGINE     BacEngine;
    INTEL_HOSTVLD_VP9_FRAME_INFO     FrameInfo;
    INTEL_HOSTVLD_VP9_1D_BUFFER      ReferenceFrame; // Y, U and V share the same reference frame buffer
    PINTEL_HOSTVLD_VP9_1D_BUFFER     pLastSegIdBuf;  // For inter frame seg id prediction.
    struct object_surface               *pRenderTarget;

    PINTEL_HOSTVLD_VP9_TILE_STATE    pTileStateBase;
    DWORD                               dwTileStatesInUse;

    DWORD                               dwCurrIndex;
    DWORD                               dwPrevIndex;

    DWORD                               dwLastTaskID;
};

// Hostvld state
struct _INTEL_HOSTVLD_VP9_STATE
{

    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameStateBase;
    PINTEL_HOSTVLD_VP9_VIDEO_BUFFER  pVideoBufferBase;
    PINTEL_HOSTVLD_VP9_OUTPUT_BUFFER pOutputBufferBase;
    PINTEL_HOSTVLD_VP9_TASK_USERDATA pTaskUserData;
    DWORD                               dwThreadNumber;
    DWORD                               dwBufferNumber;

    DWORD                               dwDDIBufNumber;
    DWORD                               dwCurrDDIBufIndex;
    DWORD                               dwPrevDDIBufIndex;
    INTEL_HOSTVLD_VP9_CONTEXT        Context;

    INTEL_HOSTVLD_VP9_FRAME_TYPE     LastFrameType;
    DWORD                               dwCurrIndex;
    DWORD                               dwTaskUserDataIndex;

    PFNINTEL_HOSTVLD_VP9_DEBLOCKCB   pfnDeblockCb;
    PFNINTEL_HOSTVLD_VP9_RENDERCB    pfnRenderCb;
    PFNINTEL_HOSTVLD_VP9_SYNCCB      pfnSyncCb;

    UINT                                uiTileParserID[VP9_MAX_TILE_COLUMNS];
    UINT                                PrevParserID;
    UINT                                PrevLPID;
    UINT                                PrevMDFID;

    MOS_SEMAPHORE                       SemAllTaskDone;
    MOS_MUTEX                           MutexSync;
    DWORD                               dwPendingTaskNum;
    BOOL                                bIsDestroyCall;
    PMOS_SEMAPHORE                      *ppEnqueueSem;
    PMOS_MUTEX                          *ppResourceMutex;

    INTEL_HOSTVLD_VP9_1D_BUFFER      LastSegmentIndex;   // For segment id prediction for inter frame

    PVOID pvStandardState;

};

#endif // __INTEL_HOSTVLD_VP9_INTERNAL_H__
