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
 */

#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <iostream>
#include <exception>
#include <string.h>
#include <math.h>
#include "dlfcn.h"

#include "cm_debug.h"
#include "cm_csync.h"
#include "cm_common.h"

#define USERMODE_DEVICE_CONTEXT      GENOS_CONTEXT

static inline char *strtok_s(char *strToken, const char *strDelimit,
			     char **context)
{
	return strtok_r(strToken, strDelimit, context);
}

typedef unsigned char byte;

#define CM_1_0 100
#define CM_2_0 200
#define CM_2_1 201
#define CM_2_2 202
#define CM_2_3 203
#define CM_2_4 204
#define CM_3_0 300
#define CM_4_0 400
#define CURRENT_CM_VERSION  CM_4_0

#define CM_RT_API

#define CISA_MAGIC_NUMBER       0x41534943
#define CM_MIN_SURF_WIDTH       1
#define CM_MIN_SURF_HEIGHT      1
#define CM_MIN_SURF_DEPTH       2

#define CM_MAX_1D_SURF_WIDTH    0x8000000
#define CM_PAGE_ALIGNMENT       0x1000
#define CM_PAGE_ALIGNMENT_MASK  0x0FFF

#define CM_MAX_2D_SURF_WIDTH_IVB_PLUS   16384
#define CM_MAX_2D_SURF_HEIGHT_IVB_PLUS  16384

#define CM_INIT_PROGRAM_COUNT       16
#define CM_INIT_KERNEL_COUNT        64
#define CM_INIT_TASK_COUNT              16
#define CM_INIT_THREADGROUPSPACE_COUNT  8
#define CM_INIT_EVENT_COUNT             128
#define CM_INIT_THREADSPACE_COUNT       8

#define CM_NO_EVENT                     ((CmEvent *)(-1))

#define _NAME(...) #__VA_ARGS__
#define CM_MAX_OPTION_SIZE_IN_BYTE          512
#define CM_MAX_KERNEL_NAME_SIZE_IN_BYTE     256
#define CM_MAX_ISA_FILE_NAME_SIZE_IN_BYTE   256
#define CM_MAX_KERNEL_STRING_IN_BYTE        512

#define CM_MAX_TIMEOUT                      2
#define CM_MAX_TIMEOUT_MS                   CM_MAX_TIMEOUT*1000

#define CM_INVALID_KERNEL_INDEX             0xFFFFFFFF
#define CM_INVALID_GLOBAL_SURFACE       0xFFFFFFFF

#define CM_MAX_ENTRY_FOR_A_SURFACE      6
#define CM_GTPIN_BUFFER_NUM             3

#define CM_INIT_KERNEL_PER_PROGRAM              64

#define CM_MAX_SURFACE2D_FORMAT_COUNT   17

#define CM_MAX_SURFACE2D_FORMAT_COUNT_INTERNAL   17

#define CM_MAX_SURFACE3D_FORMAT_COUNT   2

#define CM_RT_PLATFORM              "CM_RT_PLATFORM"
#define INCLUDE_GTENVVAR_NAME       "CM_DYNGT_INCLUDE"
#define CM_RT_SKU                   "CM_RT_SKU"
#define CM_RT_MAX_THREADS           "CM_RT_MAX_THREADS"
#define CM_RT_AUB_PARAM             "CM_RT_AUB_PARAM"
#define CM_RT_MUL_FRAME_FILE_BEGIN   0
#define CM_RT_MUL_FRAME_FILE_MIDDLE  1
#define CM_RT_MUL_FRAME_FILE_END     2
#define CM_RT_JITTER_DEBUG_FLAG      "-debug"
#define CM_RT_JITTER_GTPIN_FLAG      "-gtpin"
#define CM_RT_JITTER_GTPIN_NORESERVEREGS_FLAG "-gtpin_noreserve"
#define CM_RT_JITTER_NCSTATELESS_FLAG      "-ncstateless"
#define CM_RT_JITTER_MAX_NUM_FLAGS      30
#define CM_RT_JITTER_NUM_RESERVED_FLAGS 3
#define CM_RT_JITTER_MAX_NUM_USER_FLAGS (CM_RT_JITTER_MAX_NUM_FLAGS - CM_RT_JITTER_NUM_RESERVED_FLAGS)

#define CM_RT_REGISTRY_FORCE_COHERENT_STATELESSBTI    "ForceCoherentStatelessBTI"

#define CM_HAL_LOCKFLAG_READONLY        0x00000001
#define CM_HAL_LOCKFLAG_WRITEONLY       0x00000002

#define CM_MAX_DEPENDENCY_COUNT         8
#define CM_MAX_THREADSPACE_WIDTH        511
#define CM_MAX_THREADSPACE_HEIGHT       511

#define MAX_SLM_SIZE_PER_GROUP_IN_1K        64
#define CM_MAX_THREAD_GROUP                 64

#define COMMON_ISA_NUM_PREDEFINED_SURF_VER_2    1
#define COMMON_ISA_NUM_PREDEFINED_SURF_VER_2_1  5
#define COMMON_ISA_NUM_PREDEFINED_SURF_VER_3_1  6

#define CM_FLAG_CURBE_ENABLED                   0x00000001
#define CM_FLAG_NONSTALLING_SCOREBOARD_ENABLED  0x00000002

#define GT_PIN_MSG_SIZE 1024

#define CM_GLOBAL_SURFACE_NUMBER      4

#define GT_RESERVED_INDEX_START                                 250
#define CM_GLOBAL_SURFACE_INDEX_START                           243
#define CM_NULL_SURFACE_BINDING_INDEX                           0

#define CM_NULL_SURFACE                     0xFFFF

#define R64_OFFSET                          32*64
#define CM_MOVE_INSTRUCTION_SIZE            16

#define CM_IVB_HSW_ADJUST_Y_SCOREBOARD_DW0      0x00000040
#define CM_IVB_HSW_ADJUST_Y_SCOREBOARD_DW1      0x20063d29
#define CM_IVB_HSW_ADJUST_Y_SCOREBOARD_DW2      0x00000006

#define CM_BDW_ADJUST_Y_SCOREBOARD_DW0          0x00000040
#define CM_BDW_ADJUST_Y_SCOREBOARD_DW1          0x20061248
#define CM_BDW_ADJUST_Y_SCOREBOARD_DW2          0x1e000006

#define CM_MINIMUM_NUM_KERNELS_ENQWHINTS        2

#define CM_THREADSPACE_MAX_COLOR_COUNT      16
#define CM_INVALID_COLOR_COUNT              0

#define CM_KERNEL_DATA_CLEAN                    0
#define CM_KERNEL_DATA_KERNEL_ARG_DIRTY         1
#define CM_KERNEL_DATA_THREAD_ARG_DIRTY         (1 << 1)
#define CM_KERNEL_DATA_PAYLOAD_DATA_DIRTY       (1 << 2)
#define CM_KERNEL_DATA_PAYLOAD_DATA_SIZE_DIRTY  (1 << 3)
#define CM_KERNEL_DATA_GLOBAL_SURFACE_DIRTY     (1 << 4)
#define CM_KERNEL_DATA_THREAD_COUNT_DIRTY       (1 << 5)

#define SIWA_ONLY_A0            0x0fff0001u
#define SIWA_ONLY_A1            0x0fff0002u
#define SIWA_ONLY_A2            0x0fff0004u
#define SIWA_ONLY_A3            0x0fff0008u
#define SIWA_ONLY_A4            0x0fff0010u
#define SIWA_ONLY_A5            0x0fff0020u
#define SIWA_ONLY_A6            0x0fff0040u
#define SIWA_ONLY_A7            0x0fff0080u
#define SIWA_ONLY_A8            0x0fff0100u
#define SIWA_ONLY_A9            0x0fff0200u
#define SIWA_ONLY_AA            0x0fff0400u
#define SIWA_ONLY_AB            0x0fff0800u
#define SIWA_ONLY_AC            0x0fff1000u

typedef enum _GPU_PLATFORM {
	PLATFORM_INTEL_UNKNOWN = 0,
	PLATFORM_INTEL_SNB = 1,
	PLATFORM_INTEL_IVB = 2,
	PLATFORM_INTEL_HSW = 3,
	PLATFORM_INTEL_BDW = 4,
	PLATFORM_INTEL_CHV = 6,
	PLATFORM_INTEL_TOTAL
} GPU_PLATFORM;

typedef enum _GPU_GT_PLATFORM {
	PLATFORM_INTEL_GT_UNKNOWN = 0,
	PLATFORM_INTEL_GT1 = 1,
	PLATFORM_INTEL_GT2 = 2,
	PLATFORM_INTEL_GT3 = 3,
	PLATFORM_INTEL_GTCHV = 7,
	PLATFORM_INTEL_GTA = 8,
	PLATFORM_INTEL_GTC = 9,
	PLATFORM_INTEL_GTX = 11,

	PLATFORM_INTEL_GT_TOTAL
} GPU_GT_PLATFORM;

typedef enum _CM_DEVICE_CAP_NAME {
	CAP_KERNEL_COUNT_PER_TASK,
	CAP_KERNEL_BINARY_SIZE,
	CAP_BUFFER_COUNT,
	CAP_SURFACE2D_COUNT,
	CAP_SURFACE_COUNT_PER_KERNEL,
	CAP_ARG_COUNT_PER_KERNEL,
	CAP_ARG_SIZE_PER_KERNEL,
	CAP_USER_DEFINED_THREAD_COUNT_PER_TASK,
	CAP_HW_THREAD_COUNT,
	CAP_SURFACE2D_FORMAT_COUNT,
	CAP_SURFACE2D_FORMATS,
	CAP_GPU_PLATFORM,
	CAP_GT_PLATFORM,
	CAP_MIN_FREQUENCY,
	CAP_MAX_FREQUENCY,
	CAP_L3_CONFIG,
	CAP_GPU_CURRENT_FREQUENCY,
	CAP_USER_DEFINED_THREAD_COUNT_PER_TASK_NO_THREAD_ARG,
	CAP_USER_DEFINED_THREAD_COUNT_PER_MEDIA_WALKER,
	CAP_USER_DEFINED_THREAD_COUNT_PER_THREAD_GROUP
} CM_DEVICE_CAP_NAME;

#define HW_GT_STEPPING_A0   "A0"
#define HW_GT_STEPPING_A1   "A1"
#define HW_GT_STEPPING_B0   "B0"
#define HW_GT_STEPPING_C0   "C0"
#define HW_GT_STEPPING_D0   "D0"

typedef enum _SURFACE_DESTROY_KIND {
	APP_DESTROY = 0,
	GC_DESTROY = 1,
	FORCE_DESTROY = 2,
	DELAYED_DESTROY = 3
} SURFACE_DESTROY_KIND;

typedef enum _CM_GPUCOPY_DIRECTION {
	CM_FASTCOPY_GPU2CPU = 0,
	CM_FASTCOPY_CPU2GPU = 1,
	CM_FASTCOPY_GPU2GPU = 2,
	CM_FASTCOPY_CPU2CPU = 3
} CM_GPUCOPY_DIRECTION;

typedef enum _CM_FASTCOPY_OPTION {
	CM_FASTCOPY_OPTION_NONBLOCKING = 0x00,
	CM_FASTCOPY_OPTION_BLOCKING = 0x01
} CM_FASTCOPY_OPTION;

typedef enum _CM_STATUS {
	CM_STATUS_QUEUED = 0,
	CM_STATUS_FLUSHED = 1,
	CM_STATUS_FINISHED = 2,
	CM_STATUS_STARTED = 3
} CM_STATUS;

typedef enum _CM_TS_FLAG {
	WHITE = 0,
	GRAY = 1,
	BLACK = 2
} CM_TS_FLAG;

typedef struct _CM_COORDINATE {
	INT x;
	INT y;
} CM_COORDINATE, *PCM_COORDINATE;

typedef struct _CM_THREAD_SPACE_UNIT {
	PVOID pKernel;
	UINT threadId;
	INT numEdges;
	CM_COORDINATE scoreboardCoordinates;
	BYTE dependencyMask;
	BYTE reset;
} CM_THREAD_SPACE_UNIT;

typedef enum _CM_THREAD_SPACE_DIRTY_STATUS {
	CM_THREAD_SPACE_CLEAN = 0,
	CM_THREAD_SPACE_DEPENDENCY_MASK_DIRTY = 1,
	CM_THREAD_SPACE_DATA_DIRTY = 2
} CM_THREAD_SPACE_DIRTY_STATUS, *PCM_THREAD_SPACE_DIRTY_STATUS;

typedef struct _CM_DEPENDENCY {
	UINT count;
	INT deltaX[CM_MAX_DEPENDENCY_COUNT];
	INT deltaY[CM_MAX_DEPENDENCY_COUNT];
} CM_DEPENDENCY;

typedef enum _CM_INTERNAL_TASK_TYPE {
	CM_INTERNAL_TASK_WITH_THREADSPACE,
	CM_INTERNAL_TASK_WITH_THREADGROUPSPACE,
	CM_INTERNAL_TASK_ENQUEUEWITHHINTS
} CM_INTERNAL_TASK_TYPE;

#define CM_TASK_TYPE_DEFAULT    CM_INTERNAL_TASK_WITH_THREADSPACE

typedef enum _MEMORY_OBJECT_CONTROL {
	MEMORY_OBJECT_CONTROL_USE_GTT_ENTRY,
	MEMORY_OBJECT_CONTROL_FROM_GTT_ENTRY =
	    MEMORY_OBJECT_CONTROL_USE_GTT_ENTRY,

	MEMORY_OBJECT_CONTROL_USE_PTE = MEMORY_OBJECT_CONTROL_FROM_GTT_ENTRY,
	MEMORY_OBJECT_CONTROL_L3_USE_PTE,
	MEMORY_OBJECT_CONTROL_UC,
	MEMORY_OBJECT_CONTROL_L3_UC,
	MEMORY_OBJECT_CONTROL_LLC_ELLC,
	MEMORY_OBJECT_CONTROL_L3_LLC_ELLC,
	MEMORY_OBJECT_CONTROL_ELLC,
	MEMORY_OBJECT_CONTROL_L3_ELLC,

	MEMORY_OBJECT_CONTROL_BDW_ELLC_ONLY = 0,
	MEMORY_OBJECT_CONTROL_BDW_LLC_ONLY,
	MEMORY_OBJECT_CONTROL_BDW_LLC_ELLC_ALLOWED,
	MEMORY_OBJECT_CONTROL_BDW_L3_LLC_ELLC_ALLOWED,

	MEMORY_OBJECT_CONTROL_UNKNOW = 0xff
} MEMORY_OBJECT_CONTROL;

typedef enum _MEMORY_TYPE {
	CM_USE_PTE,
	CM_UN_CACHEABLE,
	CM_WRITE_THROUGH,
	CM_WRITE_BACK,

	MEMORY_TYPE_BDW_UC_WITH_FENCE = 0,
	MEMORY_TYPE_BDW_UC,
	MEMORY_TYPE_BDW_WT,
	MEMORY_TYPE_BDW_WB
} MEMORY_TYPE;

typedef struct _CM_SURFACE_MEM_OBJ_CTRL {
	MEMORY_OBJECT_CONTROL mem_ctrl;
	MEMORY_TYPE mem_type;
	INT age;
} CM_SURFACE_MEM_OBJ_CTRL;

typedef struct _DXVA_CM_SET_CAPS {
	DXVA_CM_SET_TYPE Type;
	UINT MaxValue;
} DXVA_CM_SET_CAPS, *PDXVA_CM_SET_CAPS;

typedef struct _CM_HAL_EXEC_GROUPED_TASK_PARAM {
	PVOID *pKernels;
	PUINT piKernelSizes;
	UINT iNumKernels;
	INT iTaskIdOut;
	UINT threadSpaceWidth;
	UINT threadSpaceHeight;
	UINT groupSpaceWidth;
	UINT groupSpaceHeight;
	UINT iSLMSize;
} CM_HAL_EXEC_GROUPED_TASK_PARAM, *PCM_HAL_EXEC_GROUPED_TASK_PARAM;

typedef enum _CM_ARG_KIND {
	ARG_KIND_GENERAL = 0x0,
	ARG_KIND_SURFACE_2D = 0x2,
	ARG_KIND_SURFACE_1D = 0x3,
	ARG_KIND_SURFACE_2D_UP = 0x7,
	ARG_KIND_SURFACE_2D_DUAL = 0xa,
	ARG_KIND_SURFACE = 0xc,
} CM_ARG_KIND;

typedef enum _SURFACE_KIND {
	DATA_PORT_SURF,
	DUAL_SURF
} SURFACE_KIND;

typedef struct _CM_ARG {
	WORD unitKind;
	WORD unitKindOrig;

	WORD index;
	SURFACE_KIND s_k;

	UINT unitCount;

	WORD unitSize;
	WORD unitSizeOrig;

	WORD unitOffsetInPayload;
	WORD unitOffsetInPayloadOrig;
	BOOL bIsDirty;
	BOOL bIsSet;
	UINT nCustomValue;

	union {
		BYTE *pValue;
		INT *pIValue;
		UINT *pUIValue;
		FLOAT *pFValue;
	};

	WORD *surfIndex;

	 _CM_ARG() {
		unitKind = 0;
		unitCount = 0;
		unitSize = 0;
		unitOffsetInPayload = 0;
		pValue = NULL;
		bIsDirty = FALSE;
}} CM_ARG;

#define  CM_JIT_FLAG_SIZE                          256
#define  CM_JIT_ERROR_MESSAGE_SIZE                 512
#define  CM_JIT_PROF_INFO_SIZE                     4096
#define  CM_PROFILE_KIND_DUAL_2D_SURFACE_STATES    0

typedef struct _CM_PROFILE_INFO {
	int kind;
	int index;
	int value;
} CM_PROFILE_INFO;

typedef struct _CM_JIT_INFO {
	bool isSpill;
	int numGRFUsed;
	int numAsmCount;

	unsigned int spillMemUsed;
	void *genDebugInfo;
	unsigned int genDebugInfoSize;
} CM_JIT_INFO;

typedef struct {
	unsigned short name_index;
	unsigned char size;
	unsigned char *values;
	char *name;
} attribute_info_t;

typedef struct {
	unsigned short name_index;
	unsigned char bit_properties;
	unsigned short num_elements;
	unsigned short alias_index;
	unsigned short alias_offset;
	unsigned char attribute_count;
	attribute_info_t *attributes;
} gen_var_info_t;

typedef struct {
	unsigned short name_index;
	unsigned short num_elements;
	unsigned char attribute_count;
	attribute_info_t *attributes;
} spec_var_info_t;

typedef struct {
	unsigned short name_index;
	unsigned char kind;
	unsigned char attribute_count;
	attribute_info_t *attributes;
} label_info_t;

typedef struct _CM_KERNEL_INFO {
	char kernelName[CM_MAX_KERNEL_NAME_SIZE_IN_BYTE];
	UINT inputCountOffset;

	UINT kernelIsaOffset;
	UINT kernelIsaSize;

	union {
		UINT jitBinarySize;
		UINT genxBinarySize;
	};

	union {
		void *jitBinaryCode;
		UINT genxBinaryOffset;
	};

	void *pOrigBinary;
	UINT uiOrigBinarySize;

	unsigned short globalStringCount;
	const char **globalStrings;
	char kernelASMName[CM_MAX_KERNEL_NAME_SIZE_IN_BYTE + 1];
	BYTE kernelSLMSize;

	CM_JIT_INFO *jitInfo;

	UINT variable_count;
	gen_var_info_t *variables;
	UINT address_count;
	spec_var_info_t *address;
	UINT predicte_count;
	spec_var_info_t *predictes;
	UINT label_count;
	label_info_t *label;
	UINT surface_count;
	spec_var_info_t *surface;

	UINT kernelInfoRefCount;
} CM_KERNEL_INFO;

#define NUM_SEARCH_PATH_STATES_G6       14
#define NUM_MBMODE_SETS_G6  4

typedef struct _CM_ARG_64 {
	void *pValue;
	int size;
} CM_ARG_64;

typedef enum _CM_MESSAGE_SEQUENCE_ {
	CM_MS_1x1 = 0,
	CM_MS_16x1 = 1,
	CM_MS_16x4 = 2,
	CM_MS_32x1 = 3,
	CM_MS_32x4 = 4,
	CM_MS_64x1 = 5,
	CM_MS_64x4 = 6
} CM_MESSAGE_SEQUENCE;

typedef enum _CM_MIN_MAX_FILTER_CONTROL_ {
	CM_MIN_FILTER = 0,
	CM_MAX_FILTER = 1,
	CM_BOTH_FILTER = 3
} CM_MIN_MAX_FILTER_CONTROL;

typedef enum _CM_VA_FUNCTION_ {
	CM_VA_MINMAXFILTER = 0,
	CM_VA_DILATE = 1,
	CM_VA_ERODE = 2
} CM_VA_FUNCTION;

typedef enum _CM_SURFACE_ADDRESS_CONTROL_MODE_ {
	CM_SURFACE_CLAMP = 0,
	CM_SURFACE_MIRROR = 1
} CM_SURFACE_ADDRESS_CONTROL_MODE;

typedef enum _GFX_TEXTUREFILTERTYPE GFX_TEXTUREFILTERTYPE;

static const CM_HAL_POWER_OPTION_PARAM
    CM_PLATFORM_POWER_CONFIGURATION[PLATFORM_INTEL_TOTAL]
    [PLATFORM_INTEL_GT_TOTAL] = {
	{
	 0},

	{
	 {0},
	 {1, 1, 10},
	 {1, 2, 20},
	 {2, 4, 40}
	 },

	{
	 {0},
	 {1, 2, 12},
	 {1, 3, 23},
	 {2, 6, 47}
	 },

	{
	 0}
};

typedef int (*pJITCompile) (const char *kernelName,
			    const void *kernelIsa,
			    UINT kernelIsaSize,
			    void *&genBinary,
			    UINT & genBinarySize,
			    const char *platform,
			    int majorVersion,
			    int minorVersion,
			    int numArgs,
			    const char *args[],
			    char *errorMsg, CM_JIT_INFO * jitInfo);
typedef int (*pJITCompileSim) (const char *kernelName,
			       const void *kernelIsa,
			       UINT kernelIsaSize,
			       const char *asmFileName,
			       const char *platform,
			       int majorVersion,
			       int minorVersion,
			       int numArgs,
			       const char *args[],
			       char *errorMsg, CM_JIT_INFO * jitInfo);
typedef void (*pFreeBlock) (void *);
typedef void (*pJITVersion) (unsigned int &majorV, unsigned int &minorV);

#define JITCOMPILE_FUNCTION_STR   "JITCompile"
#define JITCOMPILESIM_FUNCTION_STR  "JITCompileSim"
#define FREEBLOCK_FUNCTION_STR    "freeBlock"
#define JITVERSION_FUNCTION_STR     "getJITVersion"

typedef enum _JITDLL_FUNCTION_ORDINAL_ {
	JITDLL_ORDINAL_JITCOMPILE = 1,
	JITDLL_ORDINAL_JITCOMPILESIM = 2,
	JITDLL_ORDINAL_FREEBLOCK = 3,
	JITDLL_ORDINAL_JITVERSION = 4
} JITDLL_FUNCTION_ORDINAL;

typedef enum _CM_ENUM_CLASS_TYPE {
	CM_ENUM_CLASS_TYPE_CMBUFFER_RT = 0,
	CM_ENUM_CLASS_TYPE_CMSURFACE2D = 1,
	CM_ENUM_CLASS_TYPE_CMSURFACE2DUP = 2,
} CM_ENUM_CLASS_TYPE;

#define CM_NOINLINE __attribute__((noinline))

class SurfaceIndex {
 public:
	CM_NOINLINE SurfaceIndex() {
		index = 0;
	};
	CM_NOINLINE SurfaceIndex(const SurfaceIndex & _src) {
		index = _src.index;
	};
	CM_NOINLINE SurfaceIndex(const unsigned int &_n) {
		index = _n;
	};
	CM_NOINLINE SurfaceIndex & operator =(const unsigned int &_n) {
		this->index = _n;
		return *this;
	};
	CM_NOINLINE SurfaceIndex & operator +(const unsigned int &_n) {
		this->index += _n;
		return *this;
	};
	virtual unsigned int get_data(void) {
		return index;
	};

	virtual ~ SurfaceIndex() {
	};

 private:
	unsigned int index;

	unsigned char extra_byte;

	SurfaceIndex & operator=(const SurfaceIndex & other);
};

typedef enum _CM_KERNEL_INTERNAL_ARG_TYPE {
	CM_KERNEL_INTERNEL_ARG_PERKERNEL = 0,
	CM_KERNEL_INTERNEL_ARG_PERTHREAD = 1
} CM_KERNEL_INTERNAL_ARG_TYPE, *PCM_KERNEL_INTERNAL_ARG_TYPE;

typedef struct _CM_CONTEXT {
	GENOS_CONTEXT GenHwDrvCtx;
	union {
		PVOID pCmHal;
		PCM_HAL_STATE pCmHalState;
	};
} CM_CONTEXT, *PCM_CONTEXT;
