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

#ifndef __GENHW_HW_H__
#define __GENHW_HW_H__

#include "os_interface.h"
#include "hw_cmd.h"
#include "gen_hw.h"

#define GENHW_PAGE_SIZE         0x1000

#define GENHW_YTILE_H_ALIGNMENT  32
#define GENHW_YTILE_W_ALIGNMENT  128
#define GENHW_XTILE_H_ALIGNMENT  8
#define GENHW_XTILE_W_ALIGNMENT  512

#define GENHW_YTILE_H_SHIFTBITS  5
#define GENHW_YTILE_W_SHIFTBITS  7
#define GENHW_XTILE_H_SHITFBITS  3
#define GENHW_XTILE_W_SHIFTBITS  9

#define GENHW_MACROBLOCK_SIZE   16

#define GFX_MASK(lo,hi)          ((1UL << (hi)) |    \
                                 ((1UL << (hi)) -    \
                                  (1UL << (lo))))

#define GFX_MIN(a,b)             (((a) < (b)) ? (a) : (b))
#define GFX_MAX(a,b)             (((a) > (b)) ? (a) : (b))

#define GFX_CLAMP_MIN_MAX(a,min,max)   ((a) < (min) ? (min) : GFX_MIN ((a), (max)))

#define GENHW_KERNEL_LOAD_FAIL  -1

#define GENHW_MAX_SURFACE_PLANES    3

#define GENHW_KERNEL_ALLOCATION_FREE    0
#define GENHW_KERNEL_ALLOCATION_USED    1
#define GENHW_KERNEL_ALLOCATION_LOCKED  2

#define GENHW_SSH_INSTANCES            16
#define GENHW_SSH_INSTANCES_MAX        64

#define GENHW_SSH_BINDING_TABLES        1
#define GENHW_SSH_BINDING_TABLES_MIN    1
#define GENHW_SSH_BINDING_TABLES_MAX   16
#define GENHW_SSH_BINDING_TABLE_ALIGN  32
#define GENHW_SSH_BINDING_TABLE_ALIGN_G8  64

#define GENHW_SSH_SURFACE_STATES       40
#define GENHW_SSH_SURFACE_STATES_MIN   16
#define GENHW_SSH_SURFACE_STATES_MAX   256

#define GENHW_SSH_SURFACES_PER_BT      64
#define GENHW_SSH_SURFACES_PER_BT_MIN  4
#define GENHW_SSH_SURFACES_PER_BT_MAX  256

#define GENHW_SYNC_SIZE_G75             128

#define GENHW_MEDIA_STATES_G75          16

#define GENHW_MEDIA_IDS_G75             16

#define GENHW_URB_SIZE_MAX_G75          2048

#define GENHW_URB_ENTRIES_MAX_G75_GT1   64
#define GENHW_URB_ENTRIES_MAX_G75_GT2   64
#define GENHW_URB_ENTRIES_MAX_G75_GT3   128

#define GENHW_INTERFACE_DESCRIPTOR_ENTRIES_MAX_G75  64

#define GENHW_URB_ENTRY_SIZE_MAX_G75   (GENHW_URB_SIZE_MAX_G75 - GENHW_INTERFACE_DESCRIPTOR_ENTRIES_MAX_G75)

#define GENHW_CURBE_SIZE_MAX_G75       (GENHW_URB_SIZE_MAX_G75 - GENHW_INTERFACE_DESCRIPTOR_ENTRIES_MAX_G75)

#define GENHW_CURBE_SIZE_G75           320
#define GENHW_CURBE_SIZE_G8            832

#define GENHW_KERNEL_COUNT_G75          32

#define GENHW_KERNEL_COUNT_MIN          2

#define GENHW_KERNEL_HEAP_G75           2097152

#define GENHW_KERNEL_HEAP_MIN           65536
#define GENHW_KERNEL_HEAP_MAX           2097152

#define GENHW_KERNEL_BLOCK_SIZE_G75     65536

#define GENHW_KERNEL_BLOCK_MIN          1024
#define GENHW_KERNEL_BLOCK_MAX          65536

#define GENHW_SUBSLICES_MAX_G75_GT1         1
#define GENHW_SUBSLICES_MAX_G75_GT2         2
#define GENHW_SUBSLICES_MAX_G75_GT3         4
#define GENHW_SUBSLICES_MAX_G8_GT1          1
#define GENHW_SUBSLICES_MAX_G8_GT2          2
#define GENHW_SUBSLICES_MAX_G8_GT3          4
#define GENHW_SUBSLICES_MAX_G8_LCIA         2

#define GENHW_EU_INDEX_MAX_G75              13
#define GENHW_EU_INDEX_MAX_G8               12

#define GENHW_MEDIA_THREADS_PER_EU_MAX_G75  7
#define GENHW_MEDIA_THREADS_PER_EU_MAX_G8   7

#define GENHW_SIZE_REGISTERS_PER_THREAD_G75 0x1140
#define GENHW_SIZE_REGISTERS_PER_THREAD_G8  0x1800

#define GENHW_USE_MEDIA_THREADS_MAX         0
#define GENHW_MEDIA_THREADS_MAX_G75_GT1     70
#define GENHW_MEDIA_THREADS_MAX_G75_GT2     140
#define GENHW_MEDIA_THREADS_MAX_G75_GT3     280
#define GENHW_MEDIA_THREADS_MAX_G8_GT1      84
#define GENHW_MEDIA_THREADS_MAX_G8_GT2      168
#define GENHW_MEDIA_THREADS_MAX_G8_GT3      336
#define GENHW_MEDIA_THREADS_MAX_G8_LCIA     112

#define GENHW_MEDIAL_SUBSLICE_SCRATCH_DISTANCE_G75 128

#define GENHW_MAX_SIP_SIZE              0x4000

#define GENHW_KERNEL_BLOCK_ALIGN        64
#define GENHW_URB_BLOCK_ALIGN           64
#define GENHW_SYNC_BLOCK_ALIGN          128
#define GENHW_CURBE_BLOCK_ALIGN_G7      32
#define GENHW_CURBE_BLOCK_ALIGN_G8      64

#define GENHW_INSTRUCTION_CACHE_G75     1500

#define GENHW_TIMEOUT_MS_DEFAULT        100
#define GENHW_EVENT_TIMEOUT_MS          5

#define GENHW_MAX_DEPENDENCY_COUNT  8

typedef enum _GENHW_SURFACE_STATE_TYPE {
	GENHW_SURFACE_TYPE_INVALID = 0,
	GENHW_SURFACE_TYPE_G5,
	GENHW_SURFACE_TYPE_G6,
	GENHW_SURFACE_TYPE_G7,
	GENHW_SURFACE_TYPE_G8,
} GENHW_SURFACE_STATE_TYPE, *PGENHW_SURFACE_STATE_TYPE;

typedef enum _GENHW_MEDIA_WALKER_MODE {
	GENHW_MEDIA_WALKER_MODE_NOT_SET = -1,
	GENHW_MEDIA_WALKER_DISABLED = 0,
	GENHW_MEDIA_WALKER_REPEL_MODE,
	GENHW_MEDIA_WALKER_DUAL_MODE,
	GENHW_MEDIA_WALKER_QUAD_MODE
} GENHW_MEDIA_WALKER_MODE;

typedef enum _GENHW_PLANE {
	GENHW_GENERIC_PLANE = 0,
	GENHW_Y_PLANE,
	GENHW_U_PLANE,
	GENHW_V_PLANE
} GENHW_PLANE;

typedef enum _GENHW_PLANE_DEFINITION {
	GENHW_PLANES_PL3 = 0,
	GENHW_PLANES_NV12,
	GENHW_PLANES_YUY2,
	GENHW_PLANES_UYVY,
	GENHW_PLANES_YVYU,
	GENHW_PLANES_VYUY,
	GENHW_PLANES_ARGB,
	GENHW_PLANES_XRGB,
	GENHW_PLANES_ABGR,
	GENHW_PLANES_XBGR,
	GENHW_PLANES_RGB16,
	GENHW_PLANES_R16U,
	GENHW_PLANES_R16S,
	GENHW_PLANES_R32U,
	GENHW_PLANES_R32S,
	GENHW_PLANES_R32F,
	GENHW_PLANES_V8U8,
	GENHW_PLANES_R8G8_UNORM,
	GENHW_PLANES_411P,
	GENHW_PLANES_411R,
	GENHW_PLANES_422H,
	GENHW_PLANES_422V,
	GENHW_PLANES_444P,
	GENHW_PLANES_RGBP,
	GENHW_PLANES_BGRP,

	GENHW_PLANES_AI44_PALLETE_0,
	GENHW_PLANES_IA44_PALLETE_0,
	GENHW_PLANES_P8_PALLETE_0,
	GENHW_PLANES_A8P8_PALLETE_0,
	GENHW_PLANES_AI44_PALLETE_1,
	GENHW_PLANES_IA44_PALLETE_1,
	GENHW_PLANES_P8_PALLETE_1,
	GENHW_PLANES_A8P8_PALLETE_1,

	GENHW_PLANES_AYUV,
	GENHW_PLANES_STMM,
	GENHW_PLANES_L8,

	GENHW_PLANES_A8,
	GENHW_PLANES_R8,
	GENHW_PLANES_NV12_2PLANES,
	GENHW_PLANES_R16_UNORM,
	GENHW_PLANES_Y8,
	GENHW_PLANES_Y1,
	GENHW_PLANES_Y16U,
	GENHW_PLANES_Y16S,
	GENHW_PLANES_A16B16G16R16,
	GENHW_PLANES_R10G10B10A2,
	GENHW_PLANES_L16,
	GENHW_PLANES_NV21,
	GENHW_PLANES_YV12,
	GENHW_PLANES_P016,
	GENHW_PLANES_P010,
	GENHW_PLANES_DEFINITION_COUNT
} GENHW_PLANE_DEFINITION, *PGENHW_PLANE_DEFINITION;

#define GENHW_MEMORY_OBJECT_CONTROL     DWORD

typedef enum _GENHW_INDIRECT_PATCH_CMD {
	CMD_INDIRECT_INVALID,
	CMD_INDIRECT_PIPE_CONTROL,
	CMD_INDIRECT_MI_STORE_DATA_IMM,
	CMD_INDIRECT_MAX
} GENHW_INDIRECT_PATCH_COMMAND;

typedef struct _GENHW_PLANE_SETTING {
	BYTE PlaneID;
	BYTE ScaleWidth;
	BYTE ScaleHeight;
	BYTE AlignWidth;
	BYTE AlignHeight;
	BYTE PixelsPerDword;
	BOOL bAdvanced;
	DWORD dwFormat;
} GENHW_PLANE_SETTING, *PGENHW_PLANE_SETTING;

typedef struct _GENHW_SURFACE_PLANES {
	int NumPlanes;
	GENHW_PLANE_SETTING Plane[GENHW_MAX_SURFACE_PLANES];
} GENHW_SURFACE_PLANES, *PGENHW_SURFACE_PLANES;

typedef struct _GENHW_KERNEL_PARAM {
	INT GRF_Count;
	INT BT_Count;
	INT Thread_Count;
	INT GRF_Start_Register;
	INT CURBE_Length;
	INT block_width;
	INT block_height;
	INT blocks_x;
	INT blocks_y;
} GENHW_KERNEL_PARAM, *PGENHW_KERNEL_PARAM;

typedef struct _GENHW_KERNEL_ENTRY {
	LPCSTR pszName;
	PVOID pExtra;
	PBYTE pBinary;
	INT iSize;
} GENHW_KERNEL_ENTRY, *PGENHW_KERNEL_ENTRY;

typedef struct _GENHW_KERNEL_CACHE {
	INT iNumKernels;
	GENHW_KERNEL_ENTRY pKernelEntry;
	PBYTE pKernelBase;
} GENHW_KERNEL_CACHE, *PGENHW_KERNEL_CACHE;

typedef struct _GENHW_KRN_ALLOCATION {
	INT iKID;
	INT iKUID;
	INT iKCID;
	DWORD dwSync;
	DWORD dwOffset;
	INT iSize;
	DWORD dwFlags:4;
	DWORD dwCount:28;
	GENHW_KERNEL_PARAM Params;
	Kdll_CacheEntry *pKernel;
} GENHW_KRN_ALLOCATION, *PGENHW_KRN_ALLOCATION;

typedef struct _GENHW_MEDIA_STATE {
	DWORD dwOffset;
	PINT piAllocation;

	DWORD dwSyncTag;
	DWORD dwSyncCount;
	INT iCurbeOffset;
	DWORD bBusy:1;
	 DWORD:31;
} GENHW_MEDIA_STATE, *PGENHW_MEDIA_STATE;

typedef struct _GENHW_BATCH_BUFFER *PGENHW_BATCH_BUFFER;
typedef struct _GENHW_BATCH_BUFFER_PARAMS *PGENHW_BATCH_BUFFER_PARAMS;

typedef struct _GENHW_BATCH_BUFFER {
	GENOS_RESOURCE OsResource;
	INT iSize;
	INT iCurrent;
	BOOL bLocked;
	PBYTE pData;

	BOOL bBusy;
	DWORD dwSyncTag;
	PGENHW_BATCH_BUFFER pNext;
	PGENHW_BATCH_BUFFER pPrev;

	PGENHW_BATCH_BUFFER_PARAMS pBBRenderData;
} GENHW_BATCH_BUFFER;

typedef struct _GENHW_VFE_STATE_PARAMS {
	DWORD dwDebugCounterControl;
	DWORD dwMaximumNumberofThreads;
	DWORD dwCURBEAllocationSize;
	DWORD dwURBEntryAllocationSize;
} GENHW_VFE_STATE_PARAMS;

typedef struct _GENHW_HW_CAPS {
	DWORD dwMaxBTIndex;
	DWORD dwMaxThreads;
	DWORD dwMaxMediaPayloadSize;
	DWORD dwMaxURBSize;
	DWORD dwMaxURBEntries;
	DWORD dwMaxURBEntryAllocationSize;
	DWORD dwMaxCURBEAllocationSize;
	DWORD dwMaxInterfaceDescriptorEntries;
	DWORD dwMaxSubslice;
	DWORD dwMaxEUIndex;
	DWORD dwNumThreadsPerEU;
	DWORD dwSizeRegistersPerThread;
} GENHW_HW_CAPS, *PGENHW_HW_CAPS;
typedef CONST struct _GENHW_HW_CAPS *PCGENHW_HW_CAPS;

typedef struct _GENHW_GSH_SETTINGS {
	INT iSyncSize;
	INT iMediaStateHeaps;
	INT iMediaIDs;
	INT iCurbeSize;
	INT iKernelCount;
	INT iKernelHeapSize;
	INT iKernelBlockSize;

	INT iPerThreadScratchSize;
	INT iSipSize;
} GENHW_GSH_SETTINGS, *PGENHW_GSH_SETTINGS;

typedef struct _GENHW_SSH_SETTINGS {
	INT iSurfaceStateHeaps;
	INT iBindingTables;
	INT iSurfaceStates;
	INT iSurfacesPerBT;
	INT iBTAlignment;
} GENHW_SSH_SETTINGS, *PGENHW_SSH_SETTINGS;

typedef struct _GENHW_GSH {
	GENOS_RESOURCE OsResource;
	DWORD dwGSHSize;
	BOOL bGSHLocked;
	PBYTE pGSH;

	DWORD dwOffsetSync;
	DWORD dwSizeSync;

	volatile PDWORD pSync;
	DWORD dwNextTag;
	DWORD dwSyncTag;

	INT iCurMediaState;
	INT iNextMediaState;
	PGENHW_MEDIA_STATE pCurMediaState;

	DWORD dwOffsetMediaID;
	DWORD dwSizeMediaID;

	DWORD dwOffsetCurbe;
	DWORD dwSizeCurbe;

	DWORD dwKernelBase;
	INT iKernelSize;
	INT iKernelUsed;
	PBYTE pKernelLoadMap;
	DWORD dwAccessCounter;

	DWORD dwScratchSpaceSize;
	DWORD dwScratchSpaceBase;

	DWORD dwSipBase;

	PGENHW_KRN_ALLOCATION pKernelAllocation;
	PGENHW_MEDIA_STATE pMediaStates;
} GENHW_GSH, *PGENHW_GSH;

typedef struct _GENHW_VFE_SCOREBOARD_DELTA {
	BYTE x:4;
	BYTE y:4;
} GENHW_VFE_SCOREBOARD_DELTA, *PGENHW_VFE_SCOREBOARD_DELTA;

typedef struct _GENHW_VFE_SCOREBOARD {
	struct {
		DWORD ScoreboardMask:8;
		 DWORD:22;
		DWORD ScoreboardType:1;
		DWORD ScoreboardEnable:1;
	};

	union {
		GENHW_VFE_SCOREBOARD_DELTA
		    ScoreboardDelta[GENHW_MAX_DEPENDENCY_COUNT];
		struct {
			DWORD Value[2];
		};
	};

} GENHW_VFE_SCOREBOARD, *PGENHW_VFE_SCOREBOARD;

typedef struct _GENHW_WALKER_XY {
	union {
		struct {
			DWORD x:16;
			DWORD y:16;
		};
		DWORD value;
	};
} GENHW_WALKER_XY, *PGENHW_WALKER_XY;

typedef struct _GENHW_WALKER_PARAMS {
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
	GENHW_WALKER_XY LoopExecCount;
	GENHW_WALKER_XY BlockResolution;
	GENHW_WALKER_XY LocalStart;
	GENHW_WALKER_XY LocalEnd;
	GENHW_WALKER_XY LocalOutLoopStride;
	GENHW_WALKER_XY LocalInnerLoopUnit;
	GENHW_WALKER_XY GlobalResolution;
	GENHW_WALKER_XY GlobalStart;
	GENHW_WALKER_XY GlobalOutlerLoopStride;
	GENHW_WALKER_XY GlobalInnerLoopUnit;
} GENHW_WALKER_PARAMS, *PGENHW_WALKER_PARAMS;

typedef struct _GENHW_GPGPU_WALKER_PARAMS {
	DWORD InterfaceDescriptorOffset:5;
	DWORD GpGpuEnable:1;
	 DWORD:26;
	DWORD ThreadWidth;
	DWORD ThreadHeight;
	DWORD GroupWidth;
	DWORD GroupHeight;
	DWORD SLMSize;
} GENHW_GPGPU_WALKER_PARAMS, *PGENHW_GPGPU_WALKER_PARAMS;

typedef struct _GENHW_INTERFACE_DESCRIPTOR_PARAMS {
	INT iMediaID;
	INT iBindingTableID;
	INT iCurbeOffset;
	INT iCurbeLength;
	INT iCrsThrdConstDataLn;
} GENHW_INTERFACE_DESCRIPTOR_PARAMS, *PGENHW_INTERFACE_DESCRIPTOR_PARAMS;

typedef struct _GENHW_HW_MEDIAOBJECT_PARAM {
	DWORD dwIDOffset;
	DWORD dwMediaObjectSize;
} GENHW_HW_MEDIAOBJECT_PARAM, *PGENHW_HW_MEDIAOBJECT_PARAM;

typedef struct _GENHW_INDIRECT_PATCH_PARAM {
	GENHW_INDIRECT_PATCH_COMMAND Command;
	PGENOS_RESOURCE pSrcOsResource;
	DWORD dwSrcOffset;
	PGENOS_RESOURCE pTgtOsResource;
	DWORD dwTgtOffset;
} GENHW_INDIRECT_PATCH_PARAM, *PGENHW_INDIRECT_PATCH_PARAM;

typedef struct _GENHW_PIPECONTROL_PARAM {
	PGENOS_RESOURCE pOsResource;
	DWORD dwOffset;
	GFX3DCONTROL_OPERATION Operation;
	DWORD dwImmData;
	DWORD dwInvalidateStateCache:1;
	DWORD dwInvaliateConstantCache:1;
	DWORD dwInvalidateVFECache:1;
	DWORD dwInvalidateInstructionCache:1;
	DWORD dwFlushRenderTargetCache:1;
	DWORD dwCSStall:1;
	DWORD dwTlbInvalidate:1;
} GENHW_PIPECONTROL_PARAM, *PGENHW_PIPECONTROL_PARAM;

typedef struct _GENHW_STORE_DATA_IMM_PARAM {
	PGENOS_RESOURCE pOsResource;
	DWORD dwOffset;
	DWORD dwValue;
} GENHW_STORE_DATA_IMM_PARAM, *PGENHW_STORE_DATA_IMM_PARAM;

typedef struct _GENHW_LOAD_REGISTER_IMM_PARAM {
	PGENOS_RESOURCE pOsResource;
	DWORD dwRegisterAddress;
	DWORD dwData;
} GENHW_LOAD_REGISTER_IMM_PARAM, *PGENHW_LOAD_REGISTER_IMM_PARAM;

typedef struct _GENHW_SCOREBOARD_PARAMS {
	BYTE numMask;
	BYTE ScoreboardType;
	GENHW_VFE_SCOREBOARD_DELTA ScoreboardDelta[GENHW_MAX_DEPENDENCY_COUNT];
} GENHW_SCOREBOARD_PARAMS, *PGENHW_SCOREBOARD_PARAMS;

typedef struct _GENHW_SURFACE_STATE_PARAMS {
	GENHW_SURFACE_STATE_TYPE Type:5;
	DWORD bRenderTarget:1;
	DWORD bVertStride:1;
	DWORD bVertStrideOffs:1;
	DWORD bWidthInDword_Y:1;
	DWORD bWidthInDword_UV:1;
	DWORD bAVS:1;
	DWORD bWidth16Align:1;
	DWORD b2PlaneNV12NeededByKernel:1;
	DWORD bForceNV12:1;
	DWORD bUncoded:1;
	DWORD b32MWColorFillKernWA:1;
	DWORD bVASurface:1;
	DWORD AddressControl:2;
	DWORD bWAUseSrcHeight:1;
	DWORD bWAUseSrcWidth:1;
	 DWORD:8;
	GENHW_MEMORY_OBJECT_CONTROL MemObjCtl;
} GENHW_SURFACE_STATE_PARAMS, *PGENHW_SURFACE_STATE_PARAMS;

typedef union _GENHW_SURFACE_STATE {
	PACKET_SURFACE_STATE_G75 PacketSurfaceState_g75;
	PACKET_SURFACE_STATE_G8 PacketSurfaceState_g8;
} GENHW_SURFACE_STATE, *PGENHW_SURFACE_STATE;

typedef struct _GENHW_SURFACE_STATE_ENTRY {
	GENHW_SURFACE_STATE_TYPE Type;
	PGENHW_SURFACE pGenHwSurface;
	PGENHW_SURFACE_STATE pSurfaceState;
	INT iSurfStateID;
	DWORD dwSurfStateOffset;
	DWORD dwFormat;
	DWORD dwWidth;
	DWORD dwHeight;
	DWORD dwPitch;
	DWORD YUVPlane:2;
	DWORD bAVS:1;
	DWORD bRenderTarget:1;
	DWORD bVertStride:1;
	DWORD bVertStrideOffs:1;
	DWORD bWidthInDword:1;
	DWORD bTiledSurface:1;
	DWORD bTileWalk:1;
	DWORD bHalfPitchChroma:1;
	DWORD bInterleaveChroma:1;
	DWORD DirectionV:3;
	DWORD DirectionU:1;
	DWORD AddressControl:2;
	 DWORD:15;
	WORD wUXOffset;
	WORD wUYOffset;
	WORD wVXOffset;
	WORD wVYOffset;
} GENHW_SURFACE_STATE_ENTRY, *PGENHW_SURFACE_STATE_ENTRY;

typedef struct _GENHW_SSH {
	GENOS_RESOURCE OsResource;
	PBYTE pSshBuffer;
	DWORD dwSshSize;
	DWORD dwSshIntanceSize;
	BOOL bLocked;

	INT iBindingTableSize;
	INT iBindingTableOffset;
	INT iSurfaceStateOffset;

	PGENHW_SURFACE_STATE_ENTRY pSurfaceEntry;

	INT iCurSshBufferIndex;
	INT iCurrentBindingTable;
	INT iCurrentSurfaceState;
} GENHW_SSH, *PGENHW_SSH;

typedef CONST struct _GENHW_PLANE_SETTING CGENHW_PLANE_SETTING,
    *PCGENHW_PLANE_SETTING;

typedef CONST struct _GENHW_SURFACE_PLANES CGENHW_SURFACE_PLANES,
    *PCGENHW_SURFACE_PLANES;

typedef CONST struct _GENHW_GSH_SETTINGS CGENHW_GSH_SETTINGS,
    *PCGENHW_GSH_SETTINGS;

typedef CONST struct _GENHW_SSH_SETTINGS CGENHW_SSH_SETTINGS,
    *PCGENHW_SSH_SETTINGS;

typedef CONST struct _GENHW_KERNEL_PARAM CGENHW_KERNEL_PARAM,
    *PCGENHW_KERNEL_PARAM;

typedef struct _MEDIA_OBJECT_KA2_CMD {
	MEDIA_OBJECT_FC_CMD_G6 MediaObjectFC;
} MEDIA_OBJECT_KA2_CMD, *PMEDIA_OBJECT_KA2_CMD;

typedef struct _GENHW_HW_COMMANDS {
	PLATFORM Platform;

	DWORD dwMediaObjectHeaderCmdSize;

	CONST SURFACE_STATE_G7 *pSurfaceState_g75;
	CONST SURFACE_STATE_G8 *pSurfaceState_g8;

	CONST BINDING_TABLE_STATE_G5 *pBindingTableState_g75;
	CONST BINDING_TABLE_STATE_G8 *pBindingTableState_g8;

	CONST MI_BATCH_BUFFER_END_CMD_G5 *pBatchBufferEnd;

	CONST PIPELINE_SELECT_CMD_G5 *pPipelineSelectMedia;

	CONST SURFACE_STATE_TOKEN_G75 *pSurfaceStateToken_g75;

	CONST MEDIA_VFE_STATE_CMD_G6 *pVideoFrontEnd_g75;
	CONST MEDIA_CURBE_LOAD_CMD_G6 *pMediaCurbeLoad_g75;
	CONST MEDIA_INTERFACE_DESCRIPTOR_LOAD_CMD_G6 *pMediaIDLoad_g75;
	CONST MEDIA_OBJECT_WALKER_CMD_G6 *pMediaWalker_g75;
	CONST GPGPU_WALKER_CMD_G75 *pGpGpuWalker_g75;
	CONST INTERFACE_DESCRIPTOR_DATA_G6 *pInterfaceDescriptor_g75;
	CONST MI_LOAD_REGISTER_IMM_CMD_G6 *pLoadRegImm_g75;

	CONST PIPE_CONTROL_CMD_G7 *pPipeControl_g75;

	CONST MEDIA_STATE_FLUSH_CMD_G75 *pMediaStateFlush_g75;

	CONST GENHW_PIPECONTROL_PARAM *pcPipeControlParam;
	CONST GENHW_STORE_DATA_IMM_PARAM *pcStoreDataImmParam;
	CONST GENHW_INDIRECT_PATCH_PARAM *pcPatchParam;

	CONST STATE_BASE_ADDRESS_CMD_G75 *pStateBaseAddress_g75;
	CONST MI_BATCH_BUFFER_START_CMD_G75 *pBatchBufferStart_g75;
	CONST MI_ARB_CHECK_CMD_G75 *pArbCheck_g75;

	CONST PIPE_CONTROL_CMD_G8 *pPipeControl_g8;
	CONST INTERFACE_DESCRIPTOR_DATA_G8 *pInterfaceDescriptor_g8;
	CONST STATE_BASE_ADDRESS_CMD_G8 *pStateBaseAddress_g8;
	CONST MI_BATCH_BUFFER_START_CMD_G8 *pBatchBufferStart_g8;
	CONST MEDIA_VFE_STATE_CMD_G8 *pVideoFrontEnd_g8;
	CONST GPGPU_WALKER_CMD_G8 *pGpGpuWalker_g8;

	MEDIA_OBJECT_HEADER_G6 MediaObjectIStabGMC_g75;

} GENHW_HW_COMMANDS, *PGENHW_HW_COMMANDS;

typedef struct _GENHW_HW_INTERFACE {
	PGENOS_INTERFACE pOsInterface;
	PGENHW_HW_COMMANDS pHwCommands;
	PGENHW_GSH pGeneralStateHeap;
	PGENHW_SSH pSurfaceStateHeap;

	PGENHW_BATCH_BUFFER pBatchBufferList;

	PLATFORM Platform;

	GENHW_VFE_SCOREBOARD VfeScoreboard;
	PCGENHW_SURFACE_PLANES pPlaneDefinitions;

	PCGENHW_HW_CAPS pHwCaps;
	GENHW_GSH_SETTINGS GshSettings;
	GENHW_SSH_SETTINGS SshSettings;

	GENHW_VFE_STATE_PARAMS VfeStateParams;

	GENHW_SURFACE_STATE_TYPE SurfaceTypeDefault;

	BOOL bEnableYV12SinglePass;
	BOOL bMediaReset;
	BOOL bUsesPatchList;
	BOOL bRequestSingleSlice;

	GENHW_MEDIA_WALKER_MODE MediaWalkerMode;
	DWORD dwIndirectHeapSize;
	DWORD dwTimeoutMs;

	INT iSizeBindingTableState;
	INT iSizeInstructionCache;

	INT iSizeInterfaceDescriptor;

	INT iMediaStatesInUse;
	INT iBuffersInUse;

	 GENOS_STATUS(*pfnInitialize) (PGENHW_HW_INTERFACE pHwInterface,
				       PCGENHW_SETTINGS pSettings);

	 VOID(*pfnDestroy) (PGENHW_HW_INTERFACE pHwInterface);

	 GENOS_STATUS(*pfnResetHwStates) (PGENHW_HW_INTERFACE pHwInterface);

	 PGENOS_INTERFACE(*pfnGetOsInterface) (PGENHW_HW_INTERFACE
					       pHwInterface);

	 GENOS_STATUS(*pfnAllocateCommands) (PGENHW_HW_INTERFACE pHwInterface);

	 VOID(*pfnFreeCommands) (PGENHW_HW_INTERFACE pHwInterface);

	 VOID(*pfnInitCommandsCommon) (PGENHW_HW_INTERFACE pHwInterface);

	 VOID(*pfnInitCommands) (PGENHW_HW_INTERFACE pHwInterface);

	 GENOS_STATUS(*pfnAllocateSSH) (PGENHW_HW_INTERFACE pHwInterface,
					PCGENHW_SSH_SETTINGS pSshSettings);

	 VOID(*pfnFreeSSH) (PGENHW_HW_INTERFACE pHwInterface);

	 GENOS_STATUS(*pfnAllocateSshBuffer) (PGENHW_HW_INTERFACE pHwInterface,
					      PGENHW_SSH pSSH);

	 VOID(*pfnFreeSshBuffer) (PGENHW_HW_INTERFACE pHwInterface,
				  PGENHW_SSH pSSH);

	 GENOS_STATUS(*pfnAssignSshInstance) (PGENHW_HW_INTERFACE pHwInterface);

	 GENOS_STATUS(*pfnGetSurfaceStateEntries) (PGENHW_HW_INTERFACE
						   pHwInterface,
						   PGENHW_SURFACE pSurface,
						   PGENHW_SURFACE_STATE_PARAMS
						   pParams, PINT piNumEntries,
						   PGENHW_SURFACE_STATE_ENTRY *
						   ppSurfaceEntries);

	 GENOS_STATUS(*pfnSetupSurfaceState) (PGENHW_HW_INTERFACE pHwInterface,
					      PGENHW_SURFACE pSurface,
					      PGENHW_SURFACE_STATE_PARAMS
					      pParams, PINT piNumEntries,
					      PGENHW_SURFACE_STATE_ENTRY *
					      ppSurfaceEntries);

	 GENOS_STATUS(*pfnAssignSurfaceState) (PGENHW_HW_INTERFACE pHwInterface,
					       GENHW_SURFACE_STATE_TYPE Type,
					       PGENHW_SURFACE_STATE_ENTRY *
					       ppSurfaceEntry);

	 VOID(*pfnGetAlignUnit) (PWORD pwWidthAlignUnit,
				 PWORD pwHeightAlignUnit,
				 PGENHW_SURFACE pSurface);

	 VOID(*pfnAdjustBoundary) (PGENHW_HW_INTERFACE pHwInterface,
				   PGENHW_SURFACE pSurface,
				   PDWORD pdwSurfaceWidth,
				   PDWORD pdwSurfaceHeight);

	 GENOS_STATUS(*pfnAssignBindingTable) (PGENHW_HW_INTERFACE pHwInterface,
					       PINT piBindingTable);

	 GENOS_STATUS(*pfnBindSurfaceState) (PGENHW_HW_INTERFACE pHwInterface,
					     INT iBindingTableIndex,
					     INT iBindingTableEntry,
					     PGENHW_SURFACE_STATE_ENTRY
					     pSurfaceEntry);

	 DWORD(*pfnGetSurfaceMemoryObjectControl) (PGENHW_HW_INTERFACE
						   pHwInterface,
						   PGENHW_SURFACE_STATE_PARAMS
						   pParams);

	 GENOS_STATUS(*pfnSetupSurfaceStateOs) (PGENHW_HW_INTERFACE
						pHwInterface,
						PGENHW_SURFACE pSurface,
						PGENHW_SURFACE_STATE_PARAMS
						pParams,
						PGENHW_SURFACE_STATE_ENTRY
						pSurfaceStateEntry);

	 GENOS_STATUS(*pfnAllocateGSH) (PGENHW_HW_INTERFACE pHwInterface,
					PCGENHW_GSH_SETTINGS pGshSettings);

	 GENOS_STATUS(*pfnFreeGSH) (PGENHW_HW_INTERFACE pHwInterface);

	 GENOS_STATUS(*pfnLockGSH) (PGENHW_HW_INTERFACE pHwInterface);

	 GENOS_STATUS(*pfnUnlockGSH) (PGENHW_HW_INTERFACE pHwInterface);

	 GENOS_STATUS(*pfnResetGSH) (PGENHW_HW_INTERFACE pHwInterface);

	 GENOS_STATUS(*pfnRefreshSync) (PGENHW_HW_INTERFACE pHwInterface);

	 GENOS_STATUS(*pfnAllocateBB) (PGENHW_HW_INTERFACE pHwInterface,
				       PGENHW_BATCH_BUFFER pBatchBuffer,
				       INT iSize);

	 GENOS_STATUS(*pfnFreeBB) (PGENHW_HW_INTERFACE pHwInterface,
				   PGENHW_BATCH_BUFFER pBatchBuffer);

	 GENOS_STATUS(*pfnLockBB) (PGENHW_HW_INTERFACE pHwInterface,
				   PGENHW_BATCH_BUFFER pBatchBuffer);

	 GENOS_STATUS(*pfnUnlockBB) (PGENHW_HW_INTERFACE pHwInterface,
				     PGENHW_BATCH_BUFFER pBatchBuffer);

	 PGENHW_MEDIA_STATE(*pfnAssignMediaState) (PGENHW_HW_INTERFACE
						   pHwInterface);

	 INT(*pfnLoadCurbeData) (PGENHW_HW_INTERFACE pHwInterface,
				 PGENHW_MEDIA_STATE pMediaState,
				 PVOID pData, INT iSize);

	 VOID(*pfnInitInterfaceDescriptor) (PGENHW_HW_INTERFACE pHwInterface,
					    PBYTE pBase,
					    DWORD dwBase, DWORD dwOffsetID);

	 VOID(*pfnSetupInterfaceDescriptor) (PGENHW_HW_INTERFACE pHwInterface,
					     PGENHW_MEDIA_STATE pMediaState,
					     PGENHW_KRN_ALLOCATION
					     pKernelAllocation,
					     PGENHW_INTERFACE_DESCRIPTOR_PARAMS
					     pInterfaceDescriptorParams,
					     PGENHW_GPGPU_WALKER_PARAMS
					     pGpGpuWalkerParams);

	 VOID(*pfnSetVfeStateParams) (PGENHW_HW_INTERFACE pHwInterface,
				      DWORD dwDebugCounterControl,
				      DWORD dwMaximumNumberofThreads,
				      DWORD dwCURBEAllocationSize,
				      DWORD dwURBEntryAllocationSize,
				      PGENHW_SCOREBOARD_PARAMS
				      pScoreboardParams);

	 BOOL(*pfnGetMediaWalkerStatus) (PGENHW_HW_INTERFACE pHwInterface);

	 UINT(*pfnGetMediaWalkerBlockSize) (PGENHW_HW_INTERFACE pHwInterface);

	 GENOS_STATUS(*pfnSendMediaStateFlush) (PGENHW_HW_INTERFACE
						pHwInterface,
						PGENOS_COMMAND_BUFFER
						pCmdBuffer);

	 GENOS_STATUS(*pfnSendCommandBufferHeader) (PGENHW_HW_INTERFACE
						    pHwInterface,
						    PGENOS_COMMAND_BUFFER
						    pCmdBuffer);

	 GENOS_STATUS(*pfnSendSurfaces) (PGENHW_HW_INTERFACE pHwInterface,
					 PGENOS_COMMAND_BUFFER pCmdBuffer);

	 GENOS_STATUS(*pfnSendSyncTag) (PGENHW_HW_INTERFACE pHwInterface,
					PGENOS_COMMAND_BUFFER pCmdBuffer);

	 GENOS_STATUS(*pfnSendStateBaseAddr) (PGENHW_HW_INTERFACE pHwInterface,
					      PGENOS_COMMAND_BUFFER pCmdBuffer);

	 GENOS_STATUS(*pfnSendPipelineSelectCmd) (PGENHW_HW_INTERFACE
						  pHwInterface,
						  PGENOS_COMMAND_BUFFER
						  pCmdBuffer,
						  DWORD dwGfxPipelineSelect);

	 GENOS_STATUS(*pfnSendVfeState) (PGENHW_HW_INTERFACE pHwInterface,
					 PGENOS_COMMAND_BUFFER pCmdBuffer,
					 BOOL blGpGpuWalkerMode);

	 GENOS_STATUS(*pfnSendCurbeLoad) (PGENHW_HW_INTERFACE pHwInterface,
					  PGENOS_COMMAND_BUFFER pCmdBuffer);

	 GENOS_STATUS(*pfnSendIDLoad) (PGENHW_HW_INTERFACE pHwInterface,
				       PGENOS_COMMAND_BUFFER pCmdBuffer);

	 GENOS_STATUS(*pfnSendMediaObjectWalker) (PGENHW_HW_INTERFACE
						  pHwInterface,
						  PGENOS_COMMAND_BUFFER
						  pCmdBuffer,
						  PGENHW_WALKER_PARAMS
						  pWalkerParams);

	 GENOS_STATUS(*pfnSendMISetPredicateCmd) (PGENHW_HW_INTERFACE
						  pHwInterface,
						  PGENOS_COMMAND_BUFFER
						  pCmdBuffer,
						  DWORD PredicateEnable);

	 GENOS_STATUS(*pfnSendMIArbCheckCmd) (PGENHW_HW_INTERFACE pHwInterface,
					      PGENOS_COMMAND_BUFFER pCmdBuffer);

	 GENOS_STATUS(*pfnSendGpGpuWalkerState) (PGENHW_HW_INTERFACE
						 pHwInterface,
						 PGENOS_COMMAND_BUFFER
						 pCmdBuffer,
						 PGENHW_GPGPU_WALKER_PARAMS
						 pGpGpuWalkerParams);

	 GENOS_STATUS(*pfnSendBatchBufferStart) (PGENHW_HW_INTERFACE
						 pHwInterface,
						 PGENOS_COMMAND_BUFFER
						 pCmdBuffer,
						 PGENHW_BATCH_BUFFER
						 pBatchBuffer);

	 GENOS_STATUS(*pfnSendBatchBufferEnd) (PGENHW_HW_INTERFACE pHwInterface,
					       PGENOS_COMMAND_BUFFER
					       pCmdBuffer);

	 GENOS_STATUS(*pfnSendPipeControl) (PGENHW_HW_INTERFACE pHwInterface,
					    PGENOS_COMMAND_BUFFER pCmdBuffer,
					    PGENOS_RESOURCE pOsResource,
					    BOOL AllocEnable,
					    DWORD dwOffset,
					    INT ControlMode,
					    INT FlushMode, DWORD dwSyncWord);

	 GENOS_STATUS(*pfnSendStoreDataImmCmd) (PGENHW_HW_INTERFACE
						pHwInterface,
						PGENOS_COMMAND_BUFFER
						pCmdBuffer,
						PGENHW_STORE_DATA_IMM_PARAM
						pParam);

	 GENOS_STATUS(*pfnSendLoadRegImmCmd) (PGENHW_HW_INTERFACE pHwInterface,
					      PGENOS_COMMAND_BUFFER pCmdBuffer,
					      PGENHW_LOAD_REGISTER_IMM_PARAM
					      pParam);

	 GENOS_STATUS(*pfnSetupBufferSurfaceState) (PGENHW_HW_INTERFACE
						    pHwInterface,
						    PGENHW_SURFACE pSurface,
						    PGENHW_SURFACE_STATE_PARAMS
						    pParams,
						    PGENHW_SURFACE_STATE_ENTRY *
						    ppSurfaceEntry);

	 GENOS_STATUS(*pfnSendIndirectPatch) (PGENHW_HW_INTERFACE pHwInterface,
					      PGENOS_COMMAND_BUFFER pCmdBuffer,
					      PGENHW_INDIRECT_PATCH_PARAM
					      pParam);

	 VOID(*pfnAddBatchBufferEndCmdBb) (PGENHW_HW_INTERFACE pHwInterface,
					   PGENHW_BATCH_BUFFER pBatchBuffer);

	 VOID(*pfnSkipBatchBufferEndCmdBb) (PGENHW_HW_INTERFACE pHwInterface,
					    PGENHW_BATCH_BUFFER pBatchBuffer);

	 VOID(*pfnAddMediaObjectCmdBb) (PGENHW_HW_INTERFACE pHwInterface,
					PGENHW_BATCH_BUFFER pBatchBuffer,
					PGENHW_HW_MEDIAOBJECT_PARAM pParam);

	 VOID(*pfnAddPipeControlCmdBb) (PGENHW_HW_INTERFACE pHwInterface,
					PGENHW_BATCH_BUFFER pBatchBuffer,
					PGENHW_PIPECONTROL_PARAM pParam);

	 VOID(*pfnSkipPipeControlCmdBb) (PGENHW_HW_INTERFACE pHwInterface,
					 PGENHW_BATCH_BUFFER pBatchBuffer,
					 PGENHW_PIPECONTROL_PARAM pParam);

	 GENOS_STATUS(*pfnAddPipelineFlushPatch) (PGENHW_HW_INTERFACE
						  pHwInterface,
						  PGENOS_COMMAND_BUFFER
						  pCmdBuffer,
						  PGENHW_BATCH_BUFFER
						  pBatchBuffer);

	 GENOS_STATUS(*pfnSkipPipelineFlushPatch) (PGENHW_HW_INTERFACE
						   pHwInterface,
						   PGENOS_COMMAND_BUFFER
						   pCmdBuffer,
						   PGENHW_BATCH_BUFFER
						   pBatchBuffer);

	 VOID(*pfnConvertToNanoSeconds) (PGENHW_HW_INTERFACE pHwInterface,
					 UINT64 iTicks, PUINT64 piNs);

	 DWORD(*pfnGetScratchSpaceSize) (PGENHW_HW_INTERFACE pHwInterface,
					 DWORD iPerThreadScratchSpaceSize);

	 BOOL(*pfnIs2PlaneNV12Needed) (PGENHW_HW_INTERFACE pHwInterface,
				       PGENHW_SURFACE pSurface);

} GENHW_HW_INTERFACE;

GENOS_STATUS IntelGen_HwInitInterface(PGENHW_HW_INTERFACE pHwInterface,
				      PGENOS_INTERFACE pOsInterface);

DWORD IntelGen_HwGetCurBindingTableBase(PGENHW_SSH pSSH);

DWORD IntelGen_HwGetCurSurfaceStateBase(PGENHW_SSH pSSH);

VOID IntelGen_GetPixelsPerSample(GENOS_FORMAT format,
				 PDWORD pdwPixelsPerSampleUV);

#endif
