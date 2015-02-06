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
 *     Wei Lin<wei.w.lin@intel.com>
 *     Yuting Yang<yuting.yang@intel.com>
 *     Lina Sun<lina.sun@intel.com>
 */

#pragma once

#include "oscl_impl_linux.h"
#include "os_interface.h"

#ifdef __cplusplus
#define EXTERN_C     extern "C"
#else
#define EXTERN_C
#endif

#define DRMVMAP_FUNCTION_STR        "drm_intel_bo_alloc_userptr"
typedef drm_intel_bo *(*pDrmVMapFnc) (drm_intel_bufmgr * bufmgr,
				      const char *name,
				      void *addr,
				      uint32_t tiling_mode,
				      uint32_t stride,
				      unsigned long size, unsigned long flags);

typedef void *DXVAUMD_RESOURCE;

typedef struct cm_tagLOOKUP_ENTRY {
	void *pDirect3DSurface9;
	DXVAUMD_RESOURCE SurfaceHandle;
	UINT SurfaceAllocationIndex;
} CMLOOKUP_ENTRY, *PCMLOOKUP_ENTRY;

typedef struct cm_tagSURFACE_REG_TABLE {
	UINT Count;
	CMLOOKUP_ENTRY *pEntries;
} CMSURFACE_REG_TABLE, *PCMSURFACE_REG_TABLE;

typedef enum _CM_RETURN_CODE {
	CM_SUCCESS = 0,
	/*
	 * RANGE -1 ~ -9999 FOR EXTERNAL ERROR CODE
	 */
	CM_FAILURE = -1,
	CM_NOT_IMPLEMENTED = -2,
	CM_SURFACE_ALLOCATION_FAILURE = -3,
	CM_OUT_OF_HOST_MEMORY = -4,
	CM_SURFACE_FORMAT_NOT_SUPPORTED = -5,
	CM_EXCEED_SURFACE_AMOUNT = -6,
	CM_EXCEED_KERNEL_ARG_AMOUNT = -7,
	CM_EXCEED_KERNEL_ARG_SIZE_IN_BYTE = -8,
	CM_INVALID_ARG_INDEX = -9,
	CM_INVALID_ARG_VALUE = -10,
	CM_INVALID_ARG_SIZE = -11,
	CM_INVALID_THREAD_INDEX = -12,
	CM_INVALID_WIDTH = -13,
	CM_INVALID_HEIGHT = -14,
	CM_INVALID_DEPTH = -15,
	CM_INVALID_COMMON_ISA = -16,
	CM_EXCEED_MAX_KERNEL_PER_ENQUEUE = -21,
	CM_EXCEED_MAX_KERNEL_SIZE_IN_BYTE = -22,
	CM_EXCEED_MAX_THREAD_AMOUNT_PER_ENQUEUE = -23,
	CM_INVALID_THREAD_SPACE = -25,
	CM_EXCEED_MAX_TIMEOUT = -26,
	CM_JITDLL_LOAD_FAILURE = -27,
	CM_JIT_COMPILE_FAILURE = -28,
	CM_JIT_COMPILESIM_FAILURE = -29,
	CM_INVALID_THREAD_GROUP_SPACE = -30,
	CM_THREAD_ARG_NOT_ALLOWED = -31,
	CM_INVALID_GLOBAL_BUFFER_INDEX = -32,
	CM_INVALID_BUFFER_HANDLER = -33,
	CM_EXCEED_MAX_SLM_SIZE = -34,
	CM_JITDLL_OLDER_THAN_ISA = -35,
	CM_INVALID_HARDWARE_THREAD_NUMBER = -36,
	CM_GTPIN_INVOKE_FAILURE = -37,
	CM_INVALIDE_L3_CONFIGURATION = -38,
	CM_INTEL_GFX_NOTFOUND = -40,
	CM_GPUCOPY_INVALID_SYSMEM = -41,
	CM_GPUCOPY_INVALID_WIDTH = -42,
	CM_GPUCOPY_INVALID_STRIDE = -43,
	CM_EVENT_DRIVEN_FAILURE = -44,
	CM_LOCK_SURFACE_FAIL = -45,
	CM_INVALID_GENX_BINARY = -46,
	CM_FEATURE_NOT_SUPPORTED_IN_DRIVER = -47,
	CM_QUERY_DLL_VERSION_FAILURE = -48,
	CM_KERNELPAYLOAD_PERTHREADARG_MUTEX_FAIL = -49,
	CM_KERNELPAYLOAD_PERKERNELARG_MUTEX_FAIL = -50,
	CM_KERNELPAYLOAD_SETTING_FAILURE = -51,
	CM_KERNELPAYLOAD_SURFACE_INVALID_BTINDEX = -52,
	CM_NOT_SET_KERNEL_ARGUMENT = -53,
	CM_GPUCOPY_INVALID_SURFACES = -54,
	CM_GPUCOPY_INVALID_SIZE = -55,
	CM_GPUCOPY_OUT_OF_RESOURCE = -56,
	CM_SURFACE_DELAY_DESTROY = -58,
	CM_FEATURE_NOT_SUPPORTED_BY_HARDWARE = -61,
	CM_RESOURCE_USAGE_NOT_SUPPORT_READWRITE = -62,
	CM_MULTIPLE_MIPLEVELS_NOT_SUPPORTED = -63,
	CM_INVALID_UMD_CONTEXT = -64,
	CM_INVALID_LIBVA_SURFACE = -65,
	CM_INVALID_LIBVA_INITIALIZE = -66,
	CM_KERNEL_THREADSPACE_NOT_SET = -67,
	CM_INVALID_KERNEL_THREADSPACE = -68,
	CM_KERNEL_THREADSPACE_THREADS_NOT_ASSOCIATED = -69,
	CM_KERNEL_THREADSPACE_INTEGRITY_FAILED = -70,
	CM_INVALID_USERPROVIDED_GENBINARY = -71,
	CM_INVALID_PRIVATE_DATA = -72,
	CM_INVALID_GENOS_RESOURCE_HANDLE = -73,
	CM_SURFACE_CACHED = -74,
	CM_SURFACE_IN_USE = -75,
	CM_INVALID_GPUCOPY_KERNEL = -76,
	CM_INVALID_DEPENDENCY_WITH_WALKING_PATTERN = -77,
	CM_INVALID_MEDIA_WALKING_PATTERN = -78,
	CM_EXCEED_MAX_POWER_OPTION_FOR_PLATFORM = -80,
	CM_INVALID_KERNEL_THREADGROUPSPACE = -81,
	CM_INVALID_KERNEL_SPILL_CODE = -82,
	CM_UMD_DRIVER_NOT_SUPPORTED = -83,
	CM_INVALID_GPU_FREQUENCY_VALUE = -84,
	CM_SYSTEM_MEMORY_NOT_4KPAGE_ALIGNED = -85,
	CM_KERNEL_ARG_SETTING_FAILED = -86,
	CM_NO_AVAILABLE_SURFACE = -87,
	CM_VA_SURFACE_NOT_SUPPORTED = -88,
	CM_TOO_MUCH_THREADS = -89,
	CM_NULL_POINTER = -90,

	/*
	 * RANGE -10000 ~ -19999 FOR INTERNAL ERROR CODE
	 */
	CM_INTERNAL_ERROR_CODE_OFFSET = -10000,

	/*
	 * RANGE <=-20000 AREAD FOR MOST STATUS CONVERSION
	 */
	CM_GENOS_STATUS_CONVERTED_CODE_OFFSET = -20000
} CM_RETURN_CODE;

#define GENHW_CM_MAX_THREADS    "CmMaxThreads"

#define CM_HAL_LOCKFLAG_READONLY      0x00000001
#define CM_HAL_LOCKFLAG_WRITEONLY     0x00000002

#define CM_BATCH_BUFFER_REUSE_ENABLE        1
#define CM_MAX_TASKS_DEFAULT                4
#define CM_MAXIMUM_TASKS                    64
#define CM_MAX_TASKS_EU_SATURATION          4

#define CM_KERNEL_BINARY_BLOCK_SIZE         65536
#define CM_MAX_KERNELS_PER_TASK             16
#define CM_MAX_SPILL_SIZE_PER_THREAD_IVB        11264
#define CM_MAX_SPILL_SIZE_PER_THREAD_HSW_BDW 131072
#define CM_MAX_SPILL_SIZE_PER_THREAD_DEFAULT CM_MAX_SPILL_SIZE_PER_THREAD

#define CM_MAX_BUFFER_SURFACE_TABLE_SIZE    256
#define CM_MAX_2D_SURFACE_UP_TABLE_SIZE     512
#define CM_MAX_2D_SURFACE_TABLE_SIZE        256
#define CM_MAX_3D_SURFACE_TABLE_SIZE        64
#define CM_MAX_USER_THREADS                 261121
#define CM_MAX_USER_THREADS_NO_THREADARG    261121
#define CM_MAX_USER_THREADS_PER_MEDIA_WALKER (CM_MAX_THREADSPACE_WIDTH * CM_MAX_THREADSPACE_HEIGHT * CM_THREADSPACE_MAX_COLOR_COUNT)

#define MAX_THREAD_SPACE_WIDTH_PERGROUP     64
#define MAX_THREAD_SPACE_HEIGHT_PERGROUP    64
#define CM_MAX_BB_SIZE                      16777216
#define CM_MAX_ARGS_PER_KERNEL              255
#define CM_MAX_THREAD_PAYLOAD_SIZE          2016
#define CM_MAX_ARG_BYTE_PER_KERNEL          CM_MAX_THREAD_PAYLOAD_SIZE
#define CM_EXTRA_BB_SPACE                   256
#define CM_MAX_STATIC_SURFACE_STATES_PER_BT 256
#define CM_MAX_SURFACE_STATES_PER_BT        64
#define CM_MAX_SURFACE_STATES               256
#define CM_PAYLOAD_OFFSET                   32
#define CM_PER_KERNEL_ARG_VAL               1
#define CM_MAX_CURBE_SIZE_PER_TASK          8192
#define CM_MAX_CURBE_SIZE_PER_KERNEL        CM_MAX_THREAD_PAYLOAD_SIZE
#define CM_MAX_THREAD_WIDTH                 511
#define CM_MAX_INDIRECT_DATA_SIZE_PER_KERNEL    1984
#define CM_HAL_MAX_DEPENDENCY_COUNT         8

#define CM_MAX_SIP_SIZE                     0x1800
#define CM_DEBUG_SURFACE_INDEX              252
#define CM_DEBUG_SURFACE_SIZE               0x200000
#define CM_SYNC_QWORD_PER_TASK              2

#define CM_NULL_SURFACE                     0xFFFF
#define CM_SURFACE_MASK                     0xFFFF
#define CM_MEMORY_OBJECT_CONTROL_MASK       0xFFFF0000
#define CM_DEFAULT_CACHE_TYPE               0xFF00

#define CM_NULL_SURFACE_BINDING_INDEX       0
#define CM_MAX_GLOBAL_SURFACE_NUMBER        4

#define CM_BINDING_START_INDEX_OF_GLOBAL_SURFACE(pState)  243
#define CM_BINDING_START_INDEX_OF_GENERAL_SURFACE(pState) 1
#define CM_RESERVED_SURFACE_NUMBER_FROM_0(pState)         1

#define CM_RESERVED_SURFACE_NUMBER_FOR_KERNEL_DEBUG        1
#define CM_GPUWALKER_IMPLICIT_ARG_NUM       6

#define CM_KNL_SZ_BINARY_SIZE               4
#define CM_KNL_SZ_BINARY_OFFSET             4
#define CM_KNL_SZ_THREAD_COUNT              4
#define CM_KNL_SZ_ARG_COUNT                 4

#define CM_KNL_POS_BINARY_SIZE              0
#define CM_KNL_POS_BINARY_OFFSET            (CM_KNL_POS_BINARY_SIZE             +\
                                             CM_KNL_SZ_BINARY_SIZE)
#define CM_KNL_POS_THREAD_COUNT             (CM_KNL_POS_BINARY_OFFSET           +\
                                             CM_KNL_SZ_BINARY_OFFSET)
#define CM_KNL_POS_ARG_COUNT                (CM_KNL_POS_THREAD_COUNT            +\
                                             CM_KNL_SZ_THREAD_COUNT)
#define CM_KNL_POS_ARG_BLOCK_BASE           (CM_KNL_POS_ARG_COUNT               +\
                                             CM_KNL_SZ_ARG_COUNT)

#define CM_KNL_SZ_ARG_KIND                  2
#define CM_KNL_SZ_ARG_UNIT_COUNT            2
#define CM_KNL_SZ_ARG_UNIT_SIZE             2
#define CM_KNL_SZ_ARG_PAYLOAD_OFFSET        2
#define CM_KNL_SZ_ARG_VALUE_OFFSET          4
#define CM_KNL_SZ_ARG_BLOCK                 (CM_KNL_SZ_ARG_KIND                 +\
                                             CM_KNL_SZ_ARG_UNIT_COUNT           +\
                                             CM_KNL_SZ_ARG_UNIT_SIZE            +\
                                             CM_KNL_SZ_ARG_PAYLOAD_OFFSET       +\
                                             CM_KNL_SZ_ARG_VALUE_OFFSET)

#define CM_KNL_RPOS_ARG_KIND                0
#define CM_KNL_RPOS_ARG_UNIT_COUNT          (CM_KNL_RPOS_ARG_KIND               +\
                                             CM_KNL_SZ_ARG_KIND)
#define CM_KNL_RPOS_ARG_UNIT_SIZE           (CM_KNL_RPOS_ARG_UNIT_COUNT         +\
                                             CM_KNL_SZ_ARG_UNIT_COUNT)
#define CM_KNL_RPOS_ARG_PAYLOAD_OFFSET      (CM_KNL_RPOS_ARG_UNIT_SIZE          +\
                                             CM_KNL_SZ_ARG_UNIT_SIZE)
#define CM_KNL_RPOS_ARG_VAL_OFFSET          (CM_KNL_RPOS_ARG_PAYLOAD_OFFSET     +\
                                             CM_KNL_SZ_ARG_PAYLOAD_OFFSET)

#define CM_INVALID_INDEX                    -1

#define CM_KERNEL_FLAGS_CURBE                       0x00000001
#define CM_KERNEL_FLAGS_NONSTALLING_SCOREBOARD      0x00000002

#define ADDRESS_PAGE_ALIGNMENT_MASK_X64             0xFFFFFFFFFFFFF000ULL
#define ADDRESS_PAGE_ALIGNMENT_MASK_X86             0xFFFFF000

#define CM_INVALID_MEMOBJCTL            0xFF
#define CM_MEMOBJCTL_CACHE_MASK         0xFF00

#define CM_NO_KERNEL_SYNC               0

#define CM_HINTS_MASK_MEDIAOBJECT                  0x1
#define CM_HINTS_MASK_KERNEL_GROUPS                0xE
#define CM_HINTS_NUM_BITS_WALK_OBJ                 0x1
#define CM_HINTS_LEASTBIT_MASK                     1
#define CM_HINTS_DEFAULT_NUM_KERNEL_GRP            1
#define CM_DEFAULT_THREAD_DEPENDENCY_MASK          0xFF
#define CM_REUSE_DEPENDENCY_MASK                   0x1
#define CM_RESET_DEPENDENCY_MASK                   0x2
#define CM_NO_BATCH_BUFFER_REUSE                   0x4
#define CM_NO_BATCH_BUFFER_REUSE_BIT_POS           0x2
#define CM_SCOREBOARD_MASK_POS_IN_MEDIA_OBJECT_CMD 0x5
#define CM_HINTS_MASK_NUM_TASKS                    0x70
#define CM_HINTS_NUM_BITS_TASK_POS                 0x4

#define CM_GEN7_5_HW_THREADS_PER_EU      7
#define CM_GEN7_5_GT1_EUS_PER_SUBSLICE   10
#define CM_GEN7_5_GT2_EUS_PER_SUBSLICE   10
#define CM_GEN7_5_GT3_EUS_PER_SUBSLICE   10
#define CM_GEN7_5_GT1_SLICE_NUM          1
#define CM_GEN7_5_GT2_SLICE_NUM          1
#define CM_GEN7_5_GT3_SLICE_NUM          2
#define CM_GEN7_5_GT1_SUBSLICE_NUM       1
#define CM_GEN7_5_GT2_SUBSLICE_NUM       2
#define CM_GEN7_5_GT3_SUBSLICE_NUM       4

#define CM_GEN8_HW_THREADS_PER_EU        7
#define CM_GEN8_GT1_EUS_PER_SUBSLICE     6
#define CM_GEN8_GT2_EUS_PER_SUBSLICE     8
#define CM_GEN8_GT3_EUS_PER_SUBSLICE     8
#define CM_GEN8_GT1_SLICE_NUM            1
#define CM_GEN8_GT2_SLICE_NUM            1
#define CM_GEN8_GT3_SLICE_NUM            2
#define CM_GEN8_GT1_SUBSLICE_NUM         2
#define CM_GEN8_GT2_SUBSLICE_NUM         3
#define CM_GEN8_GT3_SUBSLICE_NUM         6

#define CM_GEN8LP_HW_THREADS_PER_EU      7
#define CM_GEN8LP_EUS_PER_SUBSLICE       8
#define CM_GEN8LP_SLICE_NUM              1
#define CM_GEN8LP_SUBSLICE_NUM           2

#define CM_DEVICE_CREATE_OPTION_SCRATCH_SPACE_ENABLE        0
#define CM_DEVICE_CREATE_OPTION_SCRATCH_SPACE_DISABLE       1
#define CM_DEVICE_CREATE_OPTION_SCRATCH_SPACE_MASK          1

#define CM_DEVICE_CREATE_OPTION_TDR_DISABLE                 64

#define CM_DEVICE_CREATE_OPTION_SURFACE_REUSE_ENABLE        1024

#define CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_OFFSET          1
#define CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_MASK            (7 << CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_OFFSET)
#define CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_16K_STEP        16384
#define CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_16K             1
#define CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_32K             2
#define CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_48K             3
#define CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_64K             4
#define CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_80K             5
#define CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_96K             6
#define CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_112K            7
#define CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_128K            0
#define CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_DEFAULT         CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_128K

#define CM_DEVICE_CONFIG_TASK_NUM_OFFSET                    4
#define CM_DEVICE_CONFIG_TASK_NUM_MASK                     (3 << CM_DEVICE_CONFIG_TASK_NUM_OFFSET)
#define CM_DEVICE_CONFIG_TASK_NUM_DEFAULT                   0
#define CM_DEVICE_CONFIG_TASK_NUM_8                         1
#define CM_DEVICE_CONFIG_TASK_NUM_12                        2
#define CM_DEVICE_CONFIG_TASK_NUM_16                        3
#define CM_DEVICE_CONFIG_TASK_NUM_STEP                      4

#define CM_DEVICE_CONFIG_MEDIA_RESET_OFFSET                 7
#define CM_DEVICE_CONFIG_MEDIA_RESET_ENABLE                (1 << CM_DEVICE_CONFIG_MEDIA_RESET_OFFSET)

#define CM_DEVICE_CONFIG_EXTRA_TASK_NUM_OFFSET              8
#define CM_DEVICE_CONFIG_EXTRA_TASK_NUM_MASK               (3 << CM_DEVICE_CONFIG_EXTRA_TASK_NUM_OFFSET)
#define CM_DEVICE_CONFIG_EXTRA_TASK_NUM_4                   3

#define CM_DEVICE_CONFIG_SLICESHUTDOWN_OFFSET               10
#define CM_DEVICE_CONFIG_SLICESHUTDOWN_ENABLE              (1 << CM_DEVICE_CONFIG_SLICESHUTDOWN_OFFSET)

#define CM_DEVICE_CONFIG_SURFACE_REUSE_ENABLE               11

#define CM_DEVICE_CONFIG_GPUCONTEXT_OFFSET                  12
#define CM_DEVICE_CONFIG_GPUCONTEXT_ENABLE                 (1 << CM_DEVICE_CONFIG_GPUCONTEXT_OFFSET)

#define CM_DEVICE_CONFIG_SLM_MODE_OFFSET                    13
#define CM_DEVICE_CONFIG_SLM_MODE_ENABLE                   (1 << CM_DEVICE_CONFIG_SLM_MODE_OFFSET)

#define CM_DEVICE_CREATE_OPTION_DEFAULT                     ((CM_DEVICE_CREATE_OPTION_SCRATCH_SPACE_ENABLE) \
                                                             | (CM_DEVICE_CONFIG_SLM_MODE_ENABLE))
// VP9 config : 
// Scratch space size :16k
// Number of task: 16
// Media Reset Option : TRUE
// Extra task num: 4
#define CM_DEVICE_CREATE_OPTION_FOR_VP9                    ((CM_DEVICE_CREATE_OPTION_SCRATCH_SPACE_ENABLE) \
                                                             | (CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_16K << CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_OFFSET) \
                                                             | (CM_DEVICE_CONFIG_TASK_NUM_16 << CM_DEVICE_CONFIG_TASK_NUM_OFFSET) \
                                                             | (CM_DEVICE_CONFIG_MEDIA_RESET_ENABLE) \
                                                             | (CM_DEVICE_CONFIG_EXTRA_TASK_NUM_4 << CM_DEVICE_CONFIG_EXTRA_TASK_NUM_OFFSET)\
                                                             | (CM_DEVICE_CONFIG_GPUCONTEXT_ENABLE))

#define CM_ARGUMENT_SURFACE_SIZE         4

#define SIWA_ONLY_HSW_A0    SIWA_ONLY_A0
#define SIWA_ONLY_HSW_A1    SIWA_ONLY_A1
#define SIWA_ONLY_HSW_B0    SIWA_ONLY_A2
#define SIWA_ONLY_HSW_C0    SIWA_ONLY_A3
#define SIWA_ONLY_HSW_D0    SIWA_ONLY_A4

#define SIWA_ONLY_BDW_A0    SIWA_ONLY_A0
#define SIWA_ONLY_BDW_E0    SIWA_ONLY_A5
#define SIWA_ONLY_BDW_F0    SIWA_ONLY_A6
#define SIWA_UNTIL_BDW_F0   SIWA_UNTIL_A6
#define SIWA_UNTIL_BDW_G0   SIWA_UNTIL_A7

#define CM_26ZI_BLOCK_WIDTH              16
#define CM_26ZI_BLOCK_HEIGHT             8

#define CM_NUM_DWORD_FOR_MW_PARAM        16

#define CM_DDI_1_0 100
#define CM_DDI_1_1 101 #define CM_DDI_1_2 102
#define CM_DDI_1_3 103
#define CM_DDI_1_4 104
#define CM_DDI_2_0 200
#define CM_DDI_2_1 201
#define CM_DDI_2_2 202
#define CM_DDI_2_3 203
#define CM_DDI_2_4 204
#define CM_DDI_3_0 300
#define CM_DDI_4_0 400

#define DXVA_CM_VERSION       CM_DDI_4_0
#define VA_CM_VERSION         DXVA_CM_VERSION

typedef struct _CM_HAL_STATE *PCM_HAL_STATE;
typedef struct _CM_HAL_TASK_PARAM *PCM_HAL_TASK_PARAM;
typedef struct _CM_HAL_TASK_TIMESTAMP *PCM_HAL_TASK_TIMESTAMP;
typedef struct _CM_HAL_KERNEL_PARAM *PCM_HAL_KERNEL_PARAM;

typedef enum _CM_HAL_TASK_STATUS {
	CM_TASK_QUEUED,
	CM_TASK_IN_PROGRESS,
	CM_TASK_FINISHED
} CM_HAL_TASK_STATUS;

typedef enum {
	GENX_NONE = -1,
	GENX_HSW = 2,
	GENX_BDW = 3,
	GENX_CHV = 4,
	ALL = 8
} CISA_GEN_ID;

typedef enum _CM_BUFFER_TYPE {
	CM_BUFFER_N = 0,
	CM_BUFFER_UP = 1,
	CM_BUFFER_GLOBAL = 3
} CM_BUFFER_TYPE;

typedef struct _CM_HAL_MAX_VALUES {
	UINT iMaxTasks;
	UINT iMaxKernelsPerTask;
	UINT iMaxKernelBinarySize;
	UINT iMaxSpillSizePerHwThread;
	UINT iMaxBufferTableSize;
	UINT iMax2DSurfaceTableSize;
	UINT iMax3DSurfaceTableSize;
	UINT iMaxArgsPerKernel;
	UINT iMaxArgByteSizePerKernel;
	UINT iMaxSurfacesPerKernel;
	UINT iMaxHwThreads;
	UINT iMaxUserThreadsPerTask;
	UINT iMaxUserThreadsPerTaskNoThreadArg;
} CM_HAL_MAX_VALUES, *PCM_HAL_MAX_VALUES;

typedef struct _CM_HAL_MAX_VALUES_EX {
	UINT iMax2DUPSurfaceTableSize;
	UINT iMaxCURBESizePerKernel;
	UINT iMaxCURBESizePerTask;
	UINT iMaxIndirectDataSizePerKernel;
	UINT iMaxUserThreadsPerMediaWalker;
	UINT iMaxUserThreadsPerThreadGroup;
} CM_HAL_MAX_VALUES_EX, *PCM_HAL_MAX_VALUES_EX;

typedef struct _CM_INDIRECT_SURFACE_INFO {
	WORD iKind;
	WORD iSurfaceIndex;
	WORD iBindingTableIndex;
} CM_INDIRECT_SURFACE_INFO, *PCM_INDIRECT_SURFACE_INFO;

typedef struct _CM_HAL_CREATE_PARAM {
	BOOL DisableScratchSpace;
	UINT ScratchSpaceSize;
	UINT MaxTaskNumber;
	BOOL bMediaReset;
	BOOL bRequestSliceShutdown;
	BOOL EnableSurfaceReuse;
	BOOL bRequestCustomGpuContext;
	BOOL bSLMMode;
} CM_HAL_CREATE_PARAM, *PCM_HAL_CREATE_PARAM;

typedef struct _CM_HAL_DEVICE_PARAM {
	UINT iMaxTasks;
	UINT iMaxKernelsPerTask;
	UINT iMaxKernelBinarySize;
	UINT iMaxBufferTableSize;
	UINT iMax2DSurfaceUPTableSize;
	UINT iMax2DSurfaceTableSize;
	UINT iMax3DSurfaceTableSize;
	UINT iMaxPerThreadScratchSpaceSize;
} CM_HAL_DEVICE_PARAM, *PCM_HAL_DEVICE_PARAM;

typedef struct _CM_HAL_SURFACE_ENTRY_INFO {
	UINT dwWidth;
	UINT dwHeight;
	UINT dwDepth;

	GFX_DDIFORMAT dwFormat;
	UINT dwPlaneIndex;

	UINT dwPitch;
	UINT dwSurfaceBaseAddress;
	UINT8 u8TiledSurface;
	UINT8 u8TileWalk;
	UINT dwXOffset;
	UINT dwYOffset;
} CM_HAL_SURFACE_ENTRY_INFO, CM_SURFACE_DETAILS, *PCM_HAL_SURFACE_ENTRY_INFO;

typedef struct _CM_HAL_SURFACE_ENTRY_INFO_ARRAY {
	UINT dwMaxEntryNum;
	UINT dwUsedIndex;
	PCM_HAL_SURFACE_ENTRY_INFO pSurfEntryInfos;
	UINT dwGlobalSurfNum;
	PCM_HAL_SURFACE_ENTRY_INFO pGlobalSurfInfos;
} CM_HAL_SURFACE_ENTRY_INFO_ARRAY;

typedef struct _CM_HAL_SURFACE_ENTRY_INFO_ARRAYS {
	UINT dwKrnNum;
	CM_HAL_SURFACE_ENTRY_INFO_ARRAY *pSurfEntryInfosArray;
} CM_HAL_SURFACE_ENTRY_INFO_ARRAYS;

typedef struct _CM_HAL_SCOREBOARD_XY {
	INT x;
	INT y;
} CM_HAL_SCOREBOARD_XY, *PCM_HAL_SCOREBOARD_XY;

typedef struct _CM_HAL_SCOREBOARD_XY_MASK {
	INT x;
	INT y;
	BYTE mask;
	BYTE resetMask;
} CM_HAL_SCOREBOARD_XY_MASK, *PCM_HAL_SCOREBOARD_XY_MASK;

typedef struct _CM_HAL_MASK_AND_RESET {
	BYTE mask;
	BYTE resetMask;
} CM_HAL_MASK_AND_RESET, *PCM_HAL_MASK_AND_RESET;

typedef enum _CM_DEPENDENCY_PATTERN {
	CM_DEPENDENCY_NONE = 0,
	CM_DEPENDENCY_WAVEFRONT = 1,
	CM_DEPENDENCY_WAVEFRONT26 = 2,
	CM_DEPENDENCY_VERTICAL = 3,
	CM_DEPENDENCY_HORIZONTAL = 4,
	CM_DEPENDENCY_WAVEFRONT26Z = 5,
	CM_DEPENDENCY_WAVEFRONT26ZI = 8
} CM_DEPENDENCY_PATTERN;

#define CM_HAL_DEPENDENCY_PATTERN CM_DEPENDENCY_PATTERN
#define _CM_HAL_DEPENDENCY_PATTERN _CM_DEPENDENCY_PATTERN

typedef enum _CM_26ZI_DISPATCH_PATTERN {
	VVERTICAL_HVERTICAL_26 = 0,
	VVERTICAL_HHORIZONTAL_26 = 1,
	VVERTICAL26_HHORIZONTAL26 = 2,
	VVERTICAL1X26_HHORIZONTAL1X26 = 3
} CM_26ZI_DISPATCH_PATTERN;
#define _CM_HAL_26ZI_DISPATCH_PATTERN _CM_26ZI_DISPATCH_PATTERN
#define CM_HAL_26ZI_DISPATCH_PATTERN CM_26ZI_DISPATCH_PATTERN
typedef enum _CM_HAL_WALKING_PATTERN {
	CM_WALK_DEFAULT = 0,
	CM_WALK_WAVEFRONT = 1,
	CM_WALK_WAVEFRONT26 = 2,
	CM_WALK_VERTICAL = 3,
	CM_WALK_HORIZONTAL = 4
} CM_HAL_WALKING_PATTERN;

typedef struct _CM_HAL_WALKING_PARAMETERS {
	DWORD Value[CM_NUM_DWORD_FOR_MW_PARAM];
} CM_HAL_WALKING_PARAMETERS, *PCM_HAL_WALKING_PARAMETERS;

typedef struct _CM_HAL_DEPENDENCY {
	UINT count;
	INT deltaX[CM_HAL_MAX_DEPENDENCY_COUNT];
	INT deltaY[CM_HAL_MAX_DEPENDENCY_COUNT];
} CM_HAL_DEPENDENCY;

typedef enum _CM_HAL_BB_DIRTY_STATUS {
	CM_HAL_BB_CLEAN = 0,
	CM_HAL_BB_DIRTY = 1
} CM_HAL_BB_DIRTY_STATUS, *PCM_HAL_BB_DIRTY_STATUS;

typedef struct _CM_HAL_WAVEFRONT26Z_DISPATCH_INFO {
	UINT numWaves;
	PUINT pNumThreadsInWave;
} CM_HAL_WAVEFRONT26Z_DISPATCH_INFO;

typedef struct _CM_HAL_KERNEL_SLICE_SUBSLICE {
	UINT slice;
	UINT subSlice;
} CM_HAL_KERNEL_SLICE_SUBSLICE, *PCM_HAL_KERNEL_SLICE_SUBSLICE;

typedef struct _CM_HAL_KERNEL_SUBLICE_INFO {
	UINT numSubSlices;
	UINT counter;
	PCM_HAL_KERNEL_SLICE_SUBSLICE pDestination;
} CM_HAL_KERNEL_SUBSLICE_INFO, *PCM_HAL_KERNEL_SUBSLICE_INFO;

typedef struct _CM_HAL_PLATFORM_SUBSLICE_INFO {
	UINT numSlices;
	UINT numSubSlices;
	UINT numEUsPerSubSlice;
	UINT numHWThreadsPerEU;
} CM_HAL_PLATFORM_SUBSLICE_INFO, *PCM_HAL_PLATFORM_SUBSLICE_INFO;

typedef struct _CM_HAL_PARALLELISM_GRAPH_INFO {
	UINT maxParallelism;
	UINT numMaxRepeat;
	UINT numSteps;
} CM_HAL_PARALLELISM_GRAPH_INFO, *PCM_HAL_PARALLELISM_GRAPH_INFO;

typedef struct _CM_HAL_KERNEL_GROUP_INFO {
	UINT numKernelsFinished;
	UINT numKernelsInGroup;
	UINT groupFinished;
	UINT numStepsInGrp;
	UINT freqDispatch;
} CM_HAL_KERNEL_GROUP_INFO, *PCM_HAL_KERNEL_GROUP_INFO;

typedef struct _CM_HAL_MAX_HW_THREAD_VALUES {
	UINT registryValue;
	UINT APIValue;
} CM_HAL_MAX_HW_THREAD_VALUES;

typedef enum _DXVA_CM_SET_TYPE {
	DXVA_CM_MAX_HW_THREADS,
	DXVA_CM_MAX_HW_L3_CONFIG
} DXVA_CM_SET_TYPE;

typedef struct _CM_HAL_MAX_SET_CAPS_PARAM {
	DXVA_CM_SET_TYPE Type;
	union {
		UINT MaxValue;
		struct {
			UINT L3_SQCREG1;
			UINT L3_CNTLREG2;
			UINT L3_CNTLREG3;
			UINT L3_CNTLREG;
		};
	};

} CM_HAL_MAX_SET_CAPS_PARAM, *PCM_HAL_MAX_SET_CAPS_PARAM;

typedef struct _CM_HAL_EXEC_GROUP_TASK_PARAM {
	PCM_HAL_KERNEL_PARAM *pKernels;
	PUINT piKernelSizes;
	UINT iNumKernels;
	INT iTaskIdOut;
	UINT threadSpaceWidth;
	UINT threadSpaceHeight;
	UINT groupSpaceWidth;
	UINT groupSpaceHeight;
	UINT iSLMSize;
	CM_HAL_SURFACE_ENTRY_INFO_ARRAYS SurEntryInfoArrays;
	PVOID OsData;
	UINT64 uiSyncBitmap;
	BOOL bGlobalSurfaceUsed;
	PUINT piKernelCurbeOffset;
	UINT iPreemptionMode;
	BOOL bKernelDebugEnabled;
} CM_HAL_EXEC_TASK_GROUP_PARAM, *PCM_HAL_EXEC_GROUP_TASK_PARAM;

typedef struct _CM_HAL_EXEC_HINTS_TASK_PARAM {
	PCM_HAL_KERNEL_PARAM *pKernels;
	PUINT piKernelSizes;
	UINT iNumKernels;
	INT iTaskIdOut;
	UINT iHints;
	UINT iNumTasksGenerated;
	BOOLEAN isLastTask;
	PVOID OsData;
	PUINT piKernelCurbeOffset;
} CM_HAL_EXEC_HINTS_TASK_PARAM, *PCM_HAL_EXEC_HINTS_TASK_PARAM;

typedef struct _CM_HAL_QUERY_TASK_PARAM {
	INT iTaskId;
	CM_HAL_TASK_STATUS status;
	UINT64 iTaskDuration;
	LARGE_INTEGER iTaskGlobalCMSubmitTime;
	LARGE_INTEGER iTaskCMSubmitTimeStamp;
	LARGE_INTEGER iTaskHWStartTimeStamp;
	LARGE_INTEGER iTaskHWEndTimeStamp;
	LARGE_INTEGER iTaskCompleteTime;
} CM_HAL_QUERY_TASK_PARAM, *PCM_HAL_QUERY_TASK_PARAM;

typedef struct _CM_HAL_OSSYNC_PARAM {
	HANDLE iOSSyncEvent;
} CM_HAL_OSSYNC_PARAM, *PCM_HAL_OSSYNC_PARAM;

typedef enum _CM_HAL_KERNEL_ARG_KIND {
	CM_ARGUMENT_GENERAL = 0x0,
	CM_ARGUMENT_SURFACE2D = 0x2,
	CM_ARGUMENT_SURFACEBUFFER = 0x3,
	CM_ARGUMENT_SURFACE3D = 0x4,
	CM_ARGUMENT_SURFACE2D_UP = 0x7,
	CM_ARGUMENT_SURFACE2D_DUAL = 0xa,
	CM_ARGUMENT_SURFACE = 0xc,
	CM_ARGUMENT_MAX = 0xe
} CM_HAL_KERNEL_ARG_KIND;

typedef struct _CM_HAL_KERNEL_ARG_PARAM {
	CM_HAL_KERNEL_ARG_KIND Kind;
	UINT iUnitCount;
	UINT iUnitSize;
	UINT iPayloadOffset;
	BOOL bPerThread;
	PBYTE pFirstValue;
	UINT nCustomValue;
} CM_HAL_KERNEL_ARG_PARAM, *PCM_HAL_KERNEL_ARG_PARAM;

typedef struct _CM_HAL_INDIRECT_SURFACE {
	WORD iKind;
	WORD iSurfaceIndex;
	WORD iBindingTableIndex;
} CM_HAL_INDIRECT_SURFACE, *PCM_HAL_INDIRECT_SURFACE;

typedef struct _CM_HAL_INDIRECT_DATA_PARAM {
	WORD iIndirectDataSize;
	WORD iSurfaceCount;
	PBYTE pIndirectData;
	PCM_INDIRECT_SURFACE_INFO pSurfaceInfo;
} CM_HAL_INDIRECT_DATA_PARAM, *PCM_HAL_INDIRECT_DATA_PARAM;

typedef enum _CM_GPUCOPY_KERNEL_ID {
	GPU_COPY_KERNEL_UNKNOWN = 0x0,

	GPU_COPY_KERNEL_GPU2CPU_UNALIGNED_NV12_ID = 0x1,
	GPU_COPY_KERNEL_GPU2CPU_ALIGNED_NV12_ID = 0x2,
	GPU_COPY_KERNEL_GPU2CPU_UNALIGNED_ID = 0x3,
	GPU_COPY_KERNEL_GPU2CPU_ALIGNED_ID = 0x4,

	GPU_COPY_KERNEL_CPU2GPU_NV12_ID = 0x5,
	GPU_COPY_KERNEL_CPU2GPU_ID = 0x6,

	GPU_COPY_KERNEL_GPU2GPU_NV12_ID = 0x7,
	GPU_COPY_KERNEL_GPU2GPU_ID = 0x8,

	GPU_COPY_KERNEL_CPU2CPU_ID = 0x9
} CM_GPUCOPY_KERNEL_ID;

typedef struct _CM_HAL_KERNEL_THREADSPACE_PARAM {
	WORD iThreadSpaceWidth;
	WORD iThreadSpaceHeight;
	CM_HAL_DEPENDENCY_PATTERN patternType;
	CM_HAL_DEPENDENCY dependencyInfo;
	PCM_HAL_SCOREBOARD_XY_MASK pThreadCoordinates;
	BYTE reuseBBUpdateMask;
	CM_HAL_WAVEFRONT26Z_DISPATCH_INFO dispatchInfo;
	BYTE globalDependencyMask;
	BYTE walkingParamsValid;
	CM_HAL_WALKING_PARAMETERS walkingParams;
	BYTE dependencyVectorsValid;
	CM_HAL_DEPENDENCY dependencyVectors;
	CM_HAL_BB_DIRTY_STATUS BBdirtyStatus;
} CM_HAL_KERNEL_THREADSPACE_PARAM, *PCM_HAL_KERNEL_THREADSPACE_PARAM;

typedef struct _CM_HAL_WALKER_XY {
	union {
		struct {
			DWORD x:16;
			DWORD y:16;
		};
		DWORD value;
	};
} CM_HAL_WALKER_XY, *PCM_HAL_WALKER_XY;

typedef struct _CM_HAL_WALKER_PARAMS {
	DWORD InterfaceDescriptorOffset:5;
	DWORD CmWalkerEnable:1;
	DWORD ColorCountMinusOne:4;
	DWORD ScoreboardMask:8;
	DWORD MidLoopUnitX:2;
	DWORD MidLoopUnitY:2;
	DWORD MiddleLoopExtraSteps:5;
	 DWORD:5;
	DWORD InlineDataLength;
	PBYTE pInlineData;
	CM_HAL_WALKER_XY LoopExecCount;
	CM_HAL_WALKER_XY BlockResolution;
	CM_HAL_WALKER_XY LocalStart;
	CM_HAL_WALKER_XY LocalEnd;
	CM_HAL_WALKER_XY LocalOutLoopStride;
	CM_HAL_WALKER_XY LocalInnerLoopUnit;
	CM_HAL_WALKER_XY GlobalResolution;
	CM_HAL_WALKER_XY GlobalStart;
	CM_HAL_WALKER_XY GlobalOutlerLoopStride;
	CM_HAL_WALKER_XY GlobalInnerLoopUnit;
} CM_HAL_WALKER_PARAMS, *PCM_HAL_WALKER_PARAMS;

typedef struct _CM_GPGPU_WALKER_PARAMS {
	DWORD InterfaceDescriptorOffset:5;
	DWORD CmGpGpuEnable:1;
	 DWORD:26;
	DWORD ThreadWidth;
	DWORD ThreadHeight;
	DWORD GroupWidth;
	DWORD GroupHeight;
	DWORD SLMSize;
} CM_GPGPU_WALKER_PARAMS, *PCM_GPGPU_WALKER_PARAMS;

typedef struct _CM_HAL_KERNEL_PARAM {
	CM_HAL_KERNEL_ARG_PARAM CmArgParams[CM_MAX_ARGS_PER_KERNEL];
	PBYTE pKernelData;
	PBYTE pKernelBinary;
	UINT iKernelDataSize;
	UINT iKernelBinarySize;
	UINT iNumThreads;
	UINT iNumArgs;
	UINT iNumSurfaces;
	UINT iPayloadSize;
	UINT iKrnCurbeSize;
	UINT iCurbeSizePerThread;
	UINT iCrsThrdConstDataLn;
	DWORD dwCmFlags;
	UINT64 uiKernelId;
	DWORD globalSurface[CM_MAX_GLOBAL_SURFACE_NUMBER];
	CM_HAL_INDIRECT_DATA_PARAM CmIndirectDataParam;
	PBYTE pMovInsData;
	UINT iMovInsDataSize;
	CM_HAL_KERNEL_THREADSPACE_PARAM CmKernelThreadSpaceParam;
	BOOL bGlobalSurfaceUsed;
	CM_HAL_WALKER_PARAMS WalkerParams;
	CM_GPGPU_WALKER_PARAMS GpGpuWalkerParams;
	BOOL bKernelDebugEnabled;
	BOOL bPerThreadArgExisted;
} CM_HAL_KERNEL_PARAM, *PCM_HAL_KERNEL_PARAM;

typedef enum _CM_HAL_MEMORY_OBJECT_CONTROL_G75 {
	CM_MEMORY_OBJECT_CONTROL_USE_PTE = 0x0,
	CM_MEMORY_OBJECT_CONTROL_UC = 0x2,
	CM_MEMORY_OBJECT_CONTROL_LLC_ELLC_WB_CACHED = 0x4,
	CM_MEMORY_OBJECT_CONTROL_ELLC_WB_CACHED = 0x6,
	CM_MEMORY_OBJECT_CONTROL_L3_USE_PTE = 0x1,
	CM_MEMORY_OBJECT_CONTROL_L3_UC = 0x3,
	CM_MEMORY_OBJECT_CONTROL_L3_LLC_ELLC_WB_CACHED = 0x5,
	CM_MEMORY_OBJECT_CONTROL_L3_ELLC_WB_CACHED = 0x7
} CM_HAL_MEMORY_OBJECT_CONTROL_G75;

typedef union _CM_HAL_MEMORY_OBJECT_CONTROL_G8 {
	struct {
		ULONG Age:2;
		 ULONG:1;
		ULONG TargetCache:2;
		ULONG CacheControl:2;
		 ULONG:25;
	} Gen8;

	ULONG DwordValue;
} CM_HAL_MEMORY_OBJECT_CONTROL_G8;

typedef struct _CM_HAL_TASK_PARAM {
	UINT uiNumKernels;
	UINT64 uiSyncBitmap;
	UINT iBatchBufferSize;
	DWORD dwVfeCurbeSize;
	DWORD dwUrbEntrySize;
	CM_HAL_SCOREBOARD_XY **ppThreadCoordinates;
	CM_HAL_DEPENDENCY_PATTERN DependencyPattern;
	UINT threadSpaceWidth;
	UINT threadSpaceHeight;
	UINT groupSpaceWidth;
	UINT groupSpaceHeight;
	UINT SLMSize;
	CM_HAL_SURFACE_ENTRY_INFO_ARRAYS SurEntryInfoArrays;
	UINT iCurKrnIndex;
	UINT ColorCountMinusOne;
	PCM_HAL_MASK_AND_RESET *ppDependencyMasks;
	BYTE reuseBBUpdateMask;
	UINT surfacePerBT;
	BOOL blGpGpuWalkerEnabled;
	CM_HAL_WALKING_PATTERN WalkingPattern;
	UINT iPreemptionMode;
	DWORD HasBarrier;
	BYTE walkingParamsValid;
	CM_HAL_WALKING_PARAMETERS walkingParams;
	BYTE dependencyVectorsValid;
	CM_HAL_DEPENDENCY dependencyVectors;
	UINT KernelDebugEnabled;
} CM_HAL_TASK_PARAM;

typedef struct _CM_HAL_TASK_TIMESTAMP {
	LARGE_INTEGER iGlobalCmSubmitTime[CM_MAXIMUM_TASKS];
	UINT64 iCMSubmitTimeStamp[CM_MAXIMUM_TASKS];
	LARGE_INTEGER iCompleteTime[CM_MAXIMUM_TASKS];
} CM_HAL_TASK_TIMESTAMP;

typedef struct _CM_HAL_HINT_TASK_INDEXES {
	UINT iKernelIndexes[CM_MAX_TASKS_EU_SATURATION];
	UINT iDispatchIndexes[CM_MAX_TASKS_EU_SATURATION];
} CM_HAL_HINT_TASK_INDEXES;

typedef struct _CM_HAL_L3_CONFIG {
	UINT L3_SQCREG1;
	UINT L3_CNTLREG2;
	UINT L3_CNTLREG3;
	UINT L3_CNTLREG;
} CM_HAL_L3_CONFIG;

typedef struct _CM_HAL_INDEX_PARAM {
	DWORD dwBTArray[8];
} CM_HAL_INDEX_PARAM, *PCM_HAL_INDEX_PARAM;

typedef struct _CM_HAL_EXEC_TASK_PARAM {
	PCM_HAL_KERNEL_PARAM *pKernels;
	PUINT piKernelSizes;
	UINT iNumKernels;
	INT iTaskIdOut;
	CM_HAL_SCOREBOARD_XY **ppThreadCoordinates;
	CM_HAL_DEPENDENCY_PATTERN DependencyPattern;
	UINT threadSpaceWidth;
	UINT threadSpaceHeight;
	CM_HAL_SURFACE_ENTRY_INFO_ARRAYS SurEntryInfoArrays;
	PVOID OsData;
	UINT ColorCountMinusOne;
	PCM_HAL_MASK_AND_RESET *ppDependencyMasks;
	UINT64 uiSyncBitmap;
	BOOL bGlobalSurfaceUsed;
	PUINT piKernelCurbeOffset;
	CM_HAL_WALKING_PATTERN WalkingPattern;
	BYTE walkingParamsValid;
	CM_HAL_WALKING_PARAMETERS walkingParams;
	BYTE dependencyVectorsValid;
	CM_HAL_DEPENDENCY dependencyVectors;
	BOOL bKernelDebugEnabled;
} CM_HAL_EXEC_TASK_PARAM, *PCM_HAL_EXEC_TASK_PARAM;

typedef struct _CM_HAL_POWER_OPTION_PARAM {
	USHORT nSlice;
	USHORT nSubSlice;
	USHORT nEU;
} CM_HAL_POWER_OPTION_PARAM, *PCM_HAL_POWER_OPTION_PARAM;

typedef enum {
	CM_KMD_ESCAPE_SET_FREQUENCY,
	CM_KMD_ESCAPE_TURBO_SYNC
} CM_KMD_ESCAPE_CALL;

typedef enum {
	UN_PREEMPTABLE_MODE,
	COMMAND_BUFFER_MODE,
	THREAD_GROUP_MODE,
	MIDDLE_THREAD_MODE
} CM_HAL_PREEMPTION_MODE;

typedef enum _VA_CM_FORMAT {

	VA_CM_FMT_UNKNOWN = 0,

	VA_CM_FMT_BUFFER = 10,
	VA_CM_FMT_A8R8G8B8 = 21,
	VA_CM_FMT_X8R8G8B8 = 22,
	VA_CM_FMT_A8 = 28,
	VA_CM_FMT_A2B10G10R10 = 31,
	VA_CM_FMT_A16B16G16R16 = 36,
	VA_CM_FMT_P8 = 41,
	VA_CM_FMT_L8 = 50,
	VA_CM_FMT_A8L8 = 51,
	VA_CM_FMT_R16U = 57,
	VA_CM_FMT_V8U8 = 60,
	VA_CM_FMT_R8U = 62,
	VA_CM_FMT_D16 = 80,
	VA_CM_FMT_L16 = 81,
	VA_CM_FMT_A16B16G16R16F = 113,
	VA_CM_FMT_R32F = 114,
	VA_CM_FMT_NV12 = VA_FOURCC_NV12,
	VA_CM_FMT_UYVY = VA_FOURCC_UYVY,
	VA_CM_FMT_YUY2 = VA_FOURCC_YUY2,
	VA_CM_FMT_444P = VA_FOURCC_444P,
	VA_CM_FMT_411P = VA_FOURCC_411P,
	VA_CM_FMT_422H = VA_FOURCC_422H,
	VA_CM_FMT_422V = VA_FOURCC_422V,
	VA_CM_FMT_IMC3 = VA_FOURCC_IMC3,
	VA_CM_FMT_YV12 = VA_FOURCC_YV12,

	VA_CM_FMT_MAX = 0xFFFFFFFF
} VA_CM_FORMAT;

#define CM_SURFACE_FORMAT                       VA_CM_FORMAT

#define CM_SURFACE_FORMAT_UNKNOWN               VA_CM_FMT_UNKNOWN
#define CM_SURFACE_FORMAT_A8R8G8B8              VA_CM_FMT_A8R8G8B8
#define CM_SURFACE_FORMAT_X8R8G8B8              VA_CM_FMT_X8R8G8B8
#define CM_SURFACE_FORMAT_A8                    VA_CM_FMT_A8
#define CM_SURFACE_FORMAT_P8                    VA_CM_FMT_P8
#define CM_SURFACE_FORMAT_R32F                  VA_CM_FMT_R32F
#define CM_SURFACE_FORMAT_NV12                  VA_CM_FMT_NV12
#define CM_SURFACE_FORMAT_UYVY                  VA_CM_FMT_UYVY
#define CM_SURFACE_FORMAT_YUY2                  VA_CM_FMT_YUY2
#define CM_SURFACE_FORMAT_V8U8			VA_CM_FMT_V8U8

#define CM_SURFACE_FORMAT_R8_UINT		VA_CM_FMT_R8U
#define CM_SURFACE_FORMAT_R16_UINT		VA_CM_FMT_R16U
#define CM_SURFACE_FORMAT_R16_SINT              VA_CM_FMT_A8L8
#define CM_SURFACE_FORMAT_D16			VA_CM_FMT_D16
#define CM_SURFACE_FORMAT_L16			VA_CM_FMT_L16
#define CM_SURFACE_FORMAT_A16B16G16R16          VA_CM_FMT_A16B16G16R16
#define CM_SURFACE_FORMAT_R10G10B10A2		VA_CM_FMT_A2B10G10R10
#define CM_SURFACE_FORMAT_A16B16G16R16F         VA_CM_FMT_A16B16G16R16F

#define CM_SURFACE_FORMAT_444P                  VA_CM_FMT_444P
#define CM_SURFACE_FORMAT_422H                  VA_CM_FMT_422H
#define CM_SURFACE_FORMAT_422V                  VA_CM_FMT_422V
#define CM_SURFACE_FORMAT_411P                  VA_CM_FMT_411P
#define CM_SURFACE_FORMAT_IMC3                  VA_CM_FMT_IMC3
#define CM_SURFACE_FORMAT_YV12                  VA_CM_FMT_YV12

#define CM_TEXTURE_ADDRESS_TYPE                 VACMTEXTUREADDRESS
#define CM_TEXTURE_ADDRESS_WRAP                 VACMTADDRESS_WRAP
#define CM_TEXTURE_ADDRESS_MIRROR               VACMTADDRESS_MIRROR
#define CM_TEXTURE_ADDRESS_CLAMP                VACMTADDRESS_CLAMP
#define CM_TEXTURE_ADDRESS_BORDER               VACMTADDRESS_BORDER
#define CM_TEXTURE_ADDRESS_MIRRORONCE           VACMTADDRESS_MIRRORONCE

#define CM_TEXTURE_FILTER_TYPE                  VACMTEXTUREFILTERTYPE
#define CM_TEXTURE_FILTER_TYPE_NONE             VACMTEXF_NONE
#define CM_TEXTURE_FILTER_TYPE_POINT            VACMTEXF_POINT
#define CM_TEXTURE_FILTER_TYPE_LINEAR           VACMTEXF_LINEAR
#define CM_TEXTURE_FILTER_TYPE_ANISOTROPIC      VACMTEXF_ANISOTROPIC
#define CM_TEXTURE_FILTER_TYPE_FLATCUBIC        VACMTEXF_FLATCUBIC
#define CM_TEXTURE_FILTER_TYPE_GAUSSIANCUBIC    VACMTEXF_GAUSSIANCUBIC
#define CM_TEXTURE_FILTER_TYPE_PYRAMIDALQUAD    VACMTEXF_PYRAMIDALQUAD
#define CM_TEXTURE_FILTER_TYPE_GAUSSIANQUAD     VACMTEXF_GAUSSIANQUAD
#define CM_TEXTURE_FILTER_TYPE_CONVOLUTIONMONO  VACMTEXF_CONVOLUTIONMONO

typedef enum _DXVA_CM_QUERY_TYPE {
	DXVA_CM_QUERY_VERSION,
	DXVA_CM_QUERY_REG_HANDLE,
	DXVA_CM_MAX_VALUES,
	DXVA_CM_QUERY_GPU,
	DXVA_CM_QUERY_GT,
	DXVA_CM_MIN_RENDER_FREQ,
	DXVA_CM_MAX_RENDER_FREQ,
	DXVA_CM_QUERY_STEP,
	DXVA_CM_QUERY_GPU_FREQ,
	DXVA_CM_MAX_VALUES_EX,
	DXVA_CM_QUERY_SURFACE2D_FORMAT_COUNT,
	DXVA_CM_QUERY_SURFACE2D_FORMATS
} DXVA_CM_QUERY_TYPE;

typedef struct _DXVA_CM_QUERY_CAPS {
	DXVA_CM_QUERY_TYPE Type;
	union {
		INT iVersion;
		HANDLE *hRegistration;
		CM_HAL_MAX_VALUES MaxValues;
		CM_HAL_MAX_VALUES_EX MaxValuesEx;
		UINT genCore;
		UINT genGT;
		UINT MinRenderFreq;
		UINT MaxRenderFreq;
		UINT genStepId;
		UINT GPUCurrentFreq;
		UINT Surface2DCount;
		CM_SURFACE_FORMAT *pSurface2DFormats;
	};
} DXVA_CM_QUERY_CAPS, *PDXVA_CM_QUERY_CAPS;

typedef struct _CmDriverContext_ {
	int deviceid;
	int device_rev;
	/* indicates whether the CreateBufferUp/CreateSurface2DUP is not supported */
	int userptr_enabled;
	/* indicates whether CM uses the shared bufmgr or not
	 * if it is zero, CM needs to initialize its own bufmgr. bufmgr is meaningless
	 * If it is true, CM will share bufmgr with other components*/
	int shared_bufmgr;
	dri_bufmgr *bufmgr;
} CmDriverContext;

#define DRM_BO_FLINK	0x01
#define DRM_BO_HANDLE	0x02

typedef struct _CmOsResource_ {
	VA_CM_FORMAT format;
	union {
		int aligned_width;
		int buf_bytes;
	};
	int aligned_height;
	/* indicates whether it is BO_FLINK or BO_HANLDE */
	int bo_flags;
	int bo_size;
	union {
		dri_bo *bo;
		UINT bo_flink;
	};
	/*
	 * the pitch/tile_type is only for 2D surface type. It should be ignored
	 * for VA_CM_FORMAT_BUFFER.
	 * pitch is in byte unit
	 * And width/height is in pixel unit.
	 */
	int pitch;
	/*
	 * It directly uses the I915_TILING_XX definition in i915_drm.h
	 * I915_TILING_NONE 0
	 * I915_TILING_X    1
	 * I915_TILIGN_Y    2
	 */
	int tile_type;
	/* orig_width/height represents original width/height of 2D surface */
	/* For the buffer type it can be ignored */
	int orig_width, orig_height;
} CmOsResource;
