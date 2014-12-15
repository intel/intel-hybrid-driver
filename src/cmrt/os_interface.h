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
#ifndef __GENOS_OS_H__
#define __GENOS_OS_H__

#include "os_utilities.h"
#include "hw_info.h"
#include "os_resource_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <i915_drm.h>
#include <intel_bufmgr.h>
#include <xf86drm.h>

#ifdef __cplusplus
}
#endif
#define CMD_BUFFER_SIZE_IN_BYTES  (8 * 1024)
#define CM_MAX_KERNELS_PER_TASK     16
typedef struct tagKdll_CacheEntry {
	BYTE *pBinary;
	int iSize;

	int iKUID;

	int iKCID;
	DWORD dwLoaded;
} Kdll_CacheEntry;

typedef struct _GENHW_BB_CM_ARGS {
	UINT64 uiKernelIds[CM_MAX_KERNELS_PER_TASK];
	UINT64 uiRefCount;
	BOOL bLatest;
} GENHW_BB_CM_ARGS, *PGENHW_BB_CM_ARGS;

typedef struct _GENHW_BATCH_BUFFER_PARAMS {
	BOOL bMatch;
	INT iCallID;
	INT iSize;
	union {
		GENHW_BB_CM_ARGS BbCmArgs;
	} BbArgs;
} GENHW_BATCH_BUFFER_PARAMS;

typedef unsigned int GENOS_OS_FORMAT;

#define MAKEFOURCC(ch0, ch1, ch2, ch3)  \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

typedef enum _GFX_FORMAT {
	GFX_FMT_UNKNOWN = 0,

	GFX_FMT_R8G8B8 = 20,
	GFX_FMT_A8R8G8B8 = 21,
	GFX_FMT_X8R8G8B8 = 22,
	GFX_FMT_R5G6B5 = 23,
	GFX_FMT_X1R5G5B5 = 24,
	GFX_FMT_A1R5G5B5 = 25,
	GFX_FMT_A4R4G4B4 = 26,
	GFX_FMT_R3G3B2 = 27,
	GFX_FMT_A8 = 28,
	GFX_FMT_A8R3G3B2 = 29,
	GFX_FMT_X4R4G4B4 = 30,
	GFX_FMT_A2B10G10R10 = 31,
	GFX_FMT_A8B8G8R8 = 32,
	GFX_FMT_X8B8G8R8 = 33,
	GFX_FMT_G16R16 = 34,
	GFX_FMT_A2R10G10B10 = 35,
	GFX_FMT_A16B16G16R16 = 36,

	GFX_FMT_A8P8 = 40,
	GFX_FMT_P8 = 41,

	GFX_FMT_L8 = 50,
	GFX_FMT_A8L8 = 51,
	GFX_FMT_A4L4 = 52,

	GFX_FMT_V8U8 = 60,
	GFX_FMT_L6V5U5 = 61,
	GFX_FMT_X8L8V8U8 = 62,
	GFX_FMT_Q8W8V8U8 = 63,
	GFX_FMT_V16U16 = 64,
	GFX_FMT_A2W10V10U10 = 67,

	GFX_FMT_NV12 = MAKEFOURCC('N', 'V', '1', '2'),
	GFX_FMT_UYVY = MAKEFOURCC('U', 'Y', 'V', 'Y'),
	GFX_FMT_R8G8_B8G8 = MAKEFOURCC('R', 'G', 'B', 'G'),
	GFX_FMT_YUY2 = MAKEFOURCC('Y', 'U', 'Y', '2'),
	GFX_FMT_G8R8_G8B8 = MAKEFOURCC('G', 'R', 'G', 'B'),
	GFX_FMT_DXT1 = MAKEFOURCC('D', 'X', 'T', '1'),
	GFX_FMT_DXT2 = MAKEFOURCC('D', 'X', 'T', '2'),
	GFX_FMT_DXT3 = MAKEFOURCC('D', 'X', 'T', '3'),
	GFX_FMT_DXT4 = MAKEFOURCC('D', 'X', 'T', '4'),
	GFX_FMT_DXT5 = MAKEFOURCC('D', 'X', 'T', '5'),

	GFX_FMT_D16_LOCKABLE = 70,
	GFX_FMT_D32 = 71,
	GFX_FMT_D15S1 = 73,
	GFX_FMT_D24S8 = 75,
	GFX_FMT_D24X8 = 77,
	GFX_FMT_D24X4S4 = 79,
	GFX_FMT_D16 = 80,

	GFX_FMT_D32F_LOCKABLE = 82,
	GFX_FMT_D24FS8 = 83,

	GFX_FMT_L16 = 81,

	GFX_FMT_VERTEXDATA = 100,
	GFX_FMT_INDEX16 = 101,
	GFX_FMT_INDEX32 = 102,

	GFX_FMT_Q16W16V16U16 = 110,

	GFX_FMT_MULTI2_ARGB8 = MAKEFOURCC('M', 'E', 'T', '1'),

	GFX_FMT_R32U = MAKEFOURCC('R', '3', '2', 'U'),
	GFX_FMT_R32S = MAKEFOURCC('R', '3', '2', 'S'),
	GFX_FMT_R8U = MAKEFOURCC('R', '8', 'U', 'I'),
	GFX_FMT_R8G8U = MAKEFOURCC('R', 'G', '8', 'U'),
	GFX_FMT_R8G8S = MAKEFOURCC('R', 'G', '8', 'S'),

	GFX_FMT_R16F = 111,
	GFX_FMT_G16R16F = 112,
	GFX_FMT_A16B16G16R16F = 113,

	GFX_FMT_R32F = 114,
	GFX_FMT_G32R32F = 115,
	GFX_FMT_A32B32G32R32F = 116,

	GFX_FMT_CxV8U8 = 117,

	GFX_FMT_FORCE_DWORD = 0x7fffffff
} GFX_FORMAT;

#define GFX_DDIFMT_UNKNOWN       GFX_FMT_UNKNOWN
#define GFX_DDIFMT_A8B8G8R8      GFX_FMT_A8B8G8R8
#define GFX_DDIFMT_X8B8G8R8      GFX_FMT_X8B8G8R8
#define GFX_DDIFMT_A8R8G8B8      GFX_FMT_A8R8G8B8
#define GFX_DDIFMT_X8R8G8B8      GFX_FMT_X8R8G8B8
#define GFX_DDIFMT_R5G6B5        GFX_FMT_R5G6B5
#define GFX_DDIFMT_YUY2          GFX_FMT_YUY2
#define GFX_DDIFMT_P8            GFX_FMT_P8
#define GFX_DDIFMT_A8P8          GFX_FMT_A8P8
#define GFX_DDIFMT_A8            GFX_FMT_A8
#define GFX_DDIFMT_L8            GFX_FMT_L8
#define GFX_DDIFMT_A4L4          GFX_FMT_A4L4
#define GFX_DDIFMT_A8L8          GFX_FMT_A8L8
#define GFX_DDIFMT_R32F          GFX_FMT_R32F
#define GFX_DDIFMT_V8U8          GFX_FMT_V8U8
#define GFX_DDIFMT_UYVY          GFX_FMT_UYVY
#define GFX_DDIFMT_NV12          GFX_FMT_NV12

#define GFX_DDITEXF_NONE            GFX_TEXF_NONE
#define GFX_DDITEXF_POINT           GFX_TEXF_POINT
#define GFX_DDITEXF_LINEAR          GFX_TEXF_LINEAR
#define GFX_DDITEXF_ANISOTROPIC     GFX_TEXF_ANISOTROPIC
#define GFX_DDITEXF_PYRAMIDALQUAD   GFX_TEXF_PYRAMIDALQUAD
#define GFX_DDITEXF_GAUSSIANQUAD    GFX_TEXF_GAUSSIANQUAD

typedef enum _GFX_TEXTUREADDRESS {
	GFX_TADDRESS_WRAP = 1,
	GFX_TADDRESS_MIRROR = 2,
	GFX_TADDRESS_CLAMP = 3,
	GFX_TADDRESS_BORDER = 4,
	GFX_TADDRESS_MIRRORONCE = 5,
	GFX_TADDRESS_FORCE_DWORD = 0x7fffffff,	/* force 32-bit size enum */
} GFX_TEXTUREADDRESS;

typedef enum _GFX_TEXTUREFILTERTYPE {
	GFX_TEXF_NONE = 0,
	GFX_TEXF_POINT = 1,
	GFX_TEXF_LINEAR = 2,
	GFX_TEXF_ANISOTROPIC = 3,
	GFX_TEXF_PYRAMIDALQUAD = 6,
	GFX_TEXF_GAUSSIANQUAD = 7,
	GFX_TEXF_FORCE_DWORD = 0x7fffffff,
} GFX_TEXTUREFILTERTYPE;

typedef GFX_TEXTUREFILTERTYPE GFX_DDITEXTUREFILTERTYPE;

#define RtlEqualMemory(Destination,Source,Length) (!memcmp((Destination),(Source),(Length)))

#define COMMAND_BUFFER_SIZE                       32768

#define MAX_CMD_BUF_NUM                           30

#define GENOS_LOCKFLAG_WRITEONLY                    USERMODE_WRITEONLY_LOCKFLAG
#define GENOS_LOCKFLAG_READONLY                     USERMODE_READONLY_LOCKFLAG
#define GENOS_LOCKFLAG_NOOVERWRITE                  USERMODE_NOOVERWRITE_LOCKFLAG

#define GENOS_DIR_SEPERATOR                         '/'

#define GENOS_INVALID_ALLOC_INDEX                         -1
#define GENOS_MAX_REGS                              128
#define GENOS_STATUS_REPORT_DEFAULT                 0

#define OSKM_LOCKFLAG_WRITEONLY                   0x00000001
#define OSKM_LOCKFLAG_READONLY                    0x00000002
#define OSKM_LOCKFLAG_NOOVERWRITE                 0x00000004

#define I915_EXEC_VCS2                   (5<<0)

typedef struct _GENOS_RESOURCE_LINUX GENOS_RESOURCE, *PGENOS_RESOURCE;
#define GFX_DDIFORMAT GFX_FORMAT
typedef struct _GENOS_INTERFACE *PGENOS_INTERFACE;
typedef struct _GENOS_COMMAND_BUFFER *PGENOS_COMMAND_BUFFER;

typedef enum _GENOS_MEDIA_OPERATION {
	GENOS_MEDIA_OPERATION_NONE = 0,
	GENOS_MEDIA_OPERATION_DECODE,
	GENOS_MEDIA_OPERATION_ENCODE,
	GENOS_MEDIA_OPERATION_MAX
} GENOS_MEDIA_OPERATION, *PGENOS_MEDIA_OPERATION;

typedef enum _GENOS_GPU_NODE {
	GENOS_GPU_NODE_3D = I915_EXEC_RENDER,
	GENOS_GPU_NODE_VIDEO = I915_EXEC_BSD,
	GENOS_GPU_NODE_VIDEO2 = I915_EXEC_VCS2,
	GENOS_GPU_NODE_VXD = I915_EXEC_BSD,
	GENOS_GPU_NODE_MAX = 6
} GENOS_GPU_NODE, *PGENOS_GPU_NODE;

static inline GENOS_GPU_NODE OSKMGetGpuNode(GENOS_GPU_CONTEXT uiGpuContext)
{
	switch (uiGpuContext) {
	case GENOS_GPU_CONTEXT_RENDER:
	case GENOS_GPU_CONTEXT_RENDER3:
	case GENOS_GPU_CONTEXT_RENDER4:
		return GENOS_GPU_NODE_3D;
		break;

	default:
		return GENOS_GPU_NODE_MAX;
		break;
	}
}

struct _GENOS_RESOURCE_LINUX {
	GENOS_FORMAT Format;
	INT iWidth;
	INT iHeight;
	INT iPitch;
	INT iDepth;
	INT iCount;
	INT iAllocationIndex[GENOS_GPU_CONTEXT_MAX];
	DWORD dwGfxAddress;
	PBYTE pData;
	CONST CHAR *bufname;
	UINT isTiled;
	GENOS_TILE_TYPE TileType;
	UINT bMapped;
	drm_intel_bo *bo;
	UINT name;

	struct {
		BOOL bSemInitialized;
		PGENOS_SEMAPHORE **ppSyncSem;
		PUINT puiSemIndex;
	};
};

typedef struct _PATCHLOCATIONLIST {
	UINT AllocationIndex;
	UINT AllocationOffset;
	UINT PatchOffset;
	BOOL uiRelocFlag;
} PATCHLOCATIONLIST, *PPATCHLOCATIONLIST;

#define CODECHAL_MAX_REGS  128
#define PATCHLOCATIONLIST_SIZE CODECHAL_MAX_REGS

typedef struct _ALLOCATION_LIST {
	PVOID hAllocation;
	UINT WriteOperation;
} ALLOCATION_LIST, *PALLOCATION_LIST;

#define ALLOCATIONLIST_SIZE CODECHAL_MAX_REGS

typedef struct _COMMAND_BUFFER {
	struct _COMMAND_BUFFER *pNext;
	struct _COMMAND_BUFFER *pPrev;

	LONGLONG *pSyncTag;
	LONGLONG qSyncTag;

	BOOL bActive;
	BOOL bRunning;
	LARGE_INTEGER TimeStart;
	LARGE_INTEGER TimeEnd;

	PBYTE pCmdBase;
	PBYTE pCmdCurrent;
	INT iSize;
	INT iCurrent;
	INT iRemaining;
} COMMAND_BUFFER, *PCOMMAND_BUFFER;

typedef struct _CODECHAL_OS_GPU_CONTEXT {
	volatile BOOL bCBFlushed;
	PGENOS_COMMAND_BUFFER pCB;
	UINT uiCommandBufferSize;

	ALLOCATION_LIST *pAllocationList;
	UINT uiNumAllocations;
	UINT uiMaxPatchLocationsize;

	PATCHLOCATIONLIST *pPatchLocationList;
	UINT uiCurrentNumPatchLocations;
	UINT uiMaxNumAllocations;

	PCOMMAND_BUFFER pStartCB;
	PCOMMAND_BUFFER pCurrentCB;

	UINT uiResCount;
	INT iResIndex[CODECHAL_MAX_REGS];
	GENOS_RESOURCE *pResources;
	BOOL *pbWriteMode;
} GENOS_OS_GPU_CONTEXT, *PGENOS_OS_GPU_CONTEXT;

typedef struct _CMD_BUFFER_BO_POOL {
	INT iFetch;
	drm_intel_bo *pCmd_bo[MAX_CMD_BUF_NUM];
} CMD_BUFFER_BO_POOL;

typedef struct _GENOS_OS_CONTEXT GENOS_CONTEXT, *PGENOS_CONTEXT,
    GENOS_OS_CONTEXT, *PGENOS_OS_CONTEXT, GENOS_DRIVER_CONTEXT,
    *PGENOS_DRIVER_CONTEXT;

struct _GENOS_OS_CONTEXT {
	BOOL bFreeContext;

	UINT uIndirectStateSize;

	GENOS_OS_GPU_CONTEXT OsGpuContext[GENOS_GPU_CONTEXT_MAX];

	LARGE_INTEGER Frequency;
	LARGE_INTEGER LastCB;

	CMD_BUFFER_BO_POOL CmdBufferPool;

	PLATFORM platform;

	dri_bufmgr *bufmgr;

	int wDeviceID;
	int wRevision;
	int fd;

	 VOID(*pfnDestroy) (struct _GENOS_OS_CONTEXT * pOsContext);

	 BOOL(*pfnRefresh) (struct _GENOS_OS_CONTEXT * pOsContext);

	 BOOL(*pfnGetCommandBuffer) (struct _GENOS_OS_CONTEXT * pOsContext,
				     PGENOS_COMMAND_BUFFER pCmdBuffer,
				     INT iSize);

	 VOID(*pfnReturnCommandBuffer) (struct _GENOS_OS_CONTEXT * pOsContext,
					GENOS_GPU_CONTEXT GpuContext,
					PGENOS_COMMAND_BUFFER pCmdBuffer);

	 BOOL(*pfnFlushCommandBuffer) (struct _GENOS_OS_CONTEXT * pOsContext,
				       GENOS_GPU_CONTEXT GpuContext);

	 HRESULT(*pfnInsertCmdBufferToPool) (struct _GENOS_OS_CONTEXT *
					     pOsContext, drm_intel_bo * bo);

};

typedef enum _GENOS_RESOURCE_TYPE_LINUX {
	TYPE_INVALID_LINUX,
	TYPE_RESOURCE_LINUX,
	TYPE_RESOURCE_UP_LINUX,
	TYPE_BUFFER_LINUX,
	TYPE_BUFFER_UP_LINUX
} GENOS_RESOURCE_TYPE_LINUX;

typedef struct _GENOS_OS_ALLOCATION {
	PGENOS_RESOURCE pOsResource;
	BOOL bWriteMode;
} GENOS_OS_ALLOCATION, *PGENOS_OS_ALLOCATION;

#ifdef __cplusplus
extern "C" {
#endif
	BOOL IntelGen_OsResourceIsNull(PGENOS_RESOURCE pOsResource);

	VOID IntelGen_OsResetResource(PGENOS_RESOURCE pOsResource);

	GENOS_TILE_TYPE OsToGenTileType(UINT type);

	HRESULT IntelGen_OsInitInterface_Linux(PGENOS_INTERFACE pOsInterface,
					       PGENOS_CONTEXT pOsDriverContext);

#define WAIT_OBJECT_0 0

#ifdef __cplusplus
}
#endif
#define GENOS_NAL_UNIT_LENGTH                4
#define GENOS_NAL_UNIT_STARTCODE_LENGTH      3
#define GENOS_MAX_PATH_LENGTH                256
#define GENOS_MAX_SEMAPHORE_COUNT            3
#define GENOS_MAX_OBJECT_SIGNALED            32
#ifndef E_NOTIMPL
#define E_NOTIMPL       0x80004001L
#endif
#ifndef E_UNEXPECTED
#define E_UNEXPECTED    0x8000FFFFL
#endif
typedef enum _GENOS_OS {
	GENOS_OS_NONE = 0,
	GENOS_OS_LINUX = 4,
	GENOS_OS_NEXT = 5
} GENOS_OS, *PGENOS_OS;

typedef enum _GENOS_COMPONENT {
	COMPONENT_UNKNOWN = 0,
	COMPONENT_CM,
} GENOS_COMPONENT;
C_ASSERT(COMPONENT_CM == 1);

typedef struct _GENOS_PLANE_OFFSET {
	int iSurfaceOffset;
	int iXOffset;
	int iYOffset;
} GENOS_PLANE_OFFSET, *PGENOS_PLANE_OFFSET;

typedef struct _GENOS_COMMAND_BUFFER {
	GENOS_RESOURCE OsResource;

	PDWORD pCmdBase;
	PDWORD pCmdPtr;
	INT iOffset;
	INT iRemaining;
	INT iTokenOffsetInCmdBuf;
} GENOS_COMMAND_BUFFER, *PGENOS_COMMAND_BUFFER;

typedef struct _GENOS_LOCK_PARAMS {
	union {
		struct {
			UINT ReadOnly:1;
			UINT WriteOnly:1;
			UINT TiledAsTiled:1;
			UINT NoOverWrite:1;
			UINT NoDecompress:1;
			UINT Reserved:27;
		};
		UINT Value;
	};
} GENOS_LOCK_PARAMS, *PGENOS_LOCK_PARAMS;

typedef struct _GENOS_GFXRES_FLAGS {
	BOOL bNotLockable;
	BOOL bFlipChain;
} GENOS_GFXRES_FLAGS, *PGENOS_GFXRES_FLAGS;

typedef struct _GENOS_ALLOC_GFXRES_PARAMS {
	GENOS_GFXRES_TYPE Type;
	GENOS_GFXRES_FLAGS Flags;
	union {
		DWORD dwWidth;
		DWORD dwBytes;
	};
	DWORD dwHeight;
	DWORD dwDepth;
	DWORD dwArraySize;
	GENOS_TILE_TYPE TileType;
	GENOS_FORMAT Format;
	PVOID pSystemMemory;
	PCCHAR pBufName;
	BOOL bIsCompressed;
} GENOS_ALLOC_GFXRES_PARAMS, *PGENOS_ALLOC_GFXRES_PARAMS;

typedef enum _GENOS_MEMCOMP_STATE {
	GENOS_MEMCOMP_DISABLED = 0,
	GENOS_MEMCOMP_HORIZONTAL,
	GENOS_MEMCOMP_VERTICAL
} GENOS_MEMCOMP_STATE, *PGENOS_MEMCOMP_STATE;

typedef struct _GENOS_RESOURCE_OFFSETS {
	DWORD BaseOffset;
	DWORD XOffset;
	DWORD YOffset;
} GENOS_RESOURCE_OFFSETS;

typedef struct _GENOS_SURFACE {
	GENOS_RESOURCE OsResource;

	UINT dwArraySlice;
	UINT dwMipSlice;

	GENOS_GFXRES_TYPE Type;
	BOOL bFlipChain;

	DWORD dwSubResourceIndex;

	DWORD dwWidth;
	DWORD dwHeight;
	DWORD dwSize;
	DWORD dwDepth;
	DWORD dwArraySize;
	DWORD dwLockPitch;
	DWORD dwPitch;
	GENOS_TILE_TYPE TileType;
	GENOS_FORMAT Format;
	BOOL bArraySpacing;
	BOOL bCompressible;

	DWORD dwOffset;
	GENOS_PLANE_OFFSET UPlaneOffset;
	GENOS_PLANE_OFFSET VPlaneOffset;

	union {
		struct {
			GENOS_RESOURCE_OFFSETS Y;
			GENOS_RESOURCE_OFFSETS U;
			GENOS_RESOURCE_OFFSETS V;
		} YUV;

		GENOS_RESOURCE_OFFSETS RGB;
	} RenderOffset;

	union {
		struct {
			DWORD Y;
			DWORD U;
			DWORD V;
		} YUV;

		DWORD RGB;
	} LockOffset;

	BOOL bIsCompressed;
} GENOS_SURFACE, *PGENOS_SURFACE;

typedef struct _GENOS_INTERFACE {
	PGENOS_CONTEXT pOsContext;
	GENOS_GPU_CONTEXT CurrentGpuContextOrdinal;
	UINT CurrentGpuContextHandle;
	HANDLE CurrentGpuContextRuntimeHandle;

	GENOS_OS OS;
	BOOL b64bit;
	BOOL bDeallocateOnExit;

	BOOL bUsesCmdBufHeader;
	BOOL bUsesCmdBufHeaderInResize;
	BOOL bNoParsingAssistanceInKmd;
	DWORD dwCommandBufferReservedSpace;
	DWORD dwNumNalUnitBytesIncluded;

	BOOL bUsesPatchList;
	BOOL bUsesGfxAddress;

	BOOL bMapOnCreate;

	BOOL bTagEngineSync;
	BOOL bTagResourceSync;
	BOOL bOsEngineSync;
	BOOL bOsResourceSync;

	 VOID(*pfnDestroy) (PGENOS_INTERFACE pOsInterface,
			    BOOL bDestroyVscVppDeviceTag);

	 VOID(*pfnGetPlatform) (PGENOS_INTERFACE pOsInterface,
				PLATFORM * pPlatform);

	 VOID(*pfnResetOsStates) (PGENOS_INTERFACE pOsInterface);

	 HRESULT(*pfnAllocateResource) (PGENOS_INTERFACE pOsInterface,
					PGENOS_ALLOC_GFXRES_PARAMS pParams,
					PGENOS_RESOURCE pOsResource);

	 VOID(*pfnFreeResource) (PGENOS_INTERFACE pOsInterface,
				 PGENOS_RESOURCE pResource);

	 PVOID(*pfnLockResource) (PGENOS_INTERFACE pOsInterface,
				  PGENOS_RESOURCE pResource,
				  PGENOS_LOCK_PARAMS pFlags);

	 HRESULT(*pfnUnlockResource) (PGENOS_INTERFACE pOsInterface,
				      PGENOS_RESOURCE pResource);

	 HRESULT(*pfnRegisterResource) (PGENOS_INTERFACE pOsInterface,
					PGENOS_RESOURCE pResource,
					BOOL bWrite,
					BOOL bWritebSetResourceSyncTag);

	 VOID(*pfnResetResourceAllocationIndex) (PGENOS_INTERFACE pOsInterface,
						 PGENOS_RESOURCE pResource);

	 INT(*pfnGetResourceAllocationIndex) (PGENOS_INTERFACE pOsInterface,
					      PGENOS_RESOURCE pResource);

	 HRESULT(*pfnSetPatchEntry) (PGENOS_INTERFACE pOsInterface,
				     const UINT iAllocationIndex,
				     const UINT ResourceOffset,
				     const UINT PatchOffset);

	 HRESULT(*pfnWaitOnResource) (PGENOS_INTERFACE pOsInterface,
				      PGENOS_RESOURCE pResource);

	 HRESULT(*pfnVerifyCommandBufferSize) (PGENOS_INTERFACE pOsInterface,
					       DWORD dwRequestedSize);

	 HRESULT(*pfnResizeCommandBufferAndPatchList) (PGENOS_INTERFACE
						       pOsInterface,
						       DWORD
						       dwRequestedCommandBufferSize,
						       DWORD
						       dwRequestedPatchListSize);

	 HRESULT(*pfnGetCommandBuffer) (PGENOS_INTERFACE pOsInterface,
					PGENOS_COMMAND_BUFFER pCmdBuffer);

	 HRESULT(*pfnSetIndirectStateSize) (PGENOS_INTERFACE pOsInterface,
					    UINT uSize);

	 HRESULT(*pfnGetIndirectState) (PGENOS_INTERFACE pOsInterface,
					UINT * puOffset, UINT * puSize);

	 HRESULT(*pfnGetIndirectStatePointer) (PGENOS_INTERFACE pOsInterface,
					       PBYTE * pIndirectState);

	 VOID(*pfnReturnCommandBuffer) (PGENOS_INTERFACE pOsInterface,
					PGENOS_COMMAND_BUFFER pCmdBuffer);

	 HRESULT(*pfnSubmitCommandBuffer) (PGENOS_INTERFACE pOsInterface,
					   PGENOS_COMMAND_BUFFER pCmdBuffer,
					   BOOL bNullRendering);

	 VOID(*pfnSleepMs) (PGENOS_INTERFACE pOsInterface, DWORD dwWaitMs);

	 GENOS_FORMAT(*pfnFmt_OsToGen) (GENOS_OS_FORMAT format);

	 GENOS_OS_FORMAT(*pfnFmt_GenToOs) (GENOS_FORMAT format);

	 VOID(*pfnQueryPerformanceFrequency) (PLONGLONG pFrequency);

	 VOID(*pfnQueryPerformanceCounter) (PLONGLONG pCount);

	 HINSTANCE(*pfnLoadLibrary) (CONST PCHAR pFileName);

	 HRESULT(*pfnFreeLibrary) (HINSTANCE hInstance);

	 PVOID(*pfnGetProcAddress) (HINSTANCE hInstance, PCHAR pModuleName);

	 HRESULT(*pfnResetCommandBuffer) (PGENOS_INTERFACE pOsInterface,
					  PGENOS_COMMAND_BUFFER pCmdBuffer);

} GENOS_INTERFACE;

#ifdef __cplusplus
extern "C" {
#endif

	HRESULT IntelGen_OsInitInterface(PGENOS_INTERFACE pOsInterface,
					 PGENOS_CONTEXT pOsDriverContext,
					 GENOS_COMPONENT component);

	GENOS_STATUS IntelGen_OsAddCommand(PGENOS_COMMAND_BUFFER pCmdBuffer,
					   PCVOID pCmd, DWORD dwCmdSize);

	PVOID IntelGen_OsGetCmdBufferSpace(PGENOS_COMMAND_BUFFER pCmdBuffer,
					   DWORD dwCmdSize);

	VOID IntelGen_OsAdjustCmdBufferFreeSpace(PGENOS_COMMAND_BUFFER
						 pCmdBuffer, DWORD dwCmdSize);

#ifdef __cplusplus
}
#endif
#endif
