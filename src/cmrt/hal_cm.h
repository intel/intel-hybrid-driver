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
#ifndef __CM_HAL_H__
#define __CM_HAL_H__

#include "gen_hw.h"
#include "hw_interface.h"
#include "cm_common.h"
#include "cm_debug.h"

#define CM_MAX_GSH_KERNEL_ENTRIES   64
#define CM_32K                      (32*1024)
#define CM_FIXED_GSH_SPACE          (CM_MAX_GSH_KERNEL_ENTRIES * CM_32K)

#ifndef HAL_CM_UNSETBIT
#define HAL_CM_UNSETBIT(value, bitPos)                                         \
{                                                                              \
    value = (value & ~(1 << bitPos));                                          \
}
#endif

#ifndef HAL_CM_SETBIT
#define HAL_CM_SETBIT(value, bitPos)                                           \
{                                                                              \
    value = (value | (1 << bitPos));                                           \
}
#endif

#ifndef HAL_CM_CHECKBIT_IS_SET
#define HAL_CM_CHECKBIT_IS_SET(bitIsSet, value, bitPos)                        \
{                                                                              \
    bitIsSet = ((value) & (1 << bitPos));                                      \
}
#endif

typedef struct _CM_HAL_BUFFER_PARAM {
	UINT iSize;
	CM_BUFFER_TYPE type;
	PVOID pData;
	DWORD dwHandle;
	UINT iLockFlag;
	CmOsResource *pCmOsResource;
	UINT isAllocatedbyCmrtUmd;
} CM_HAL_BUFFER_PARAM, *PCM_HAL_BUFFER_PARAM;

typedef struct _CM_HAL_SURFACE2D_UP_PARAM {
	UINT iWidth;
	UINT iHeight;
	GENOS_FORMAT format;
	PVOID pData;
	UINT iPitch;
	UINT iPhysicalSize;
	DWORD dwHandle;
} CM_HAL_SURFACE2D_UP_PARAM, *PCM_HAL_SURFACE2D_UP_PARAM;

typedef struct _CM_HAL_SURFACE2D_INFO_PARAM {
	UINT iWidth;
	UINT iHeight;
	GENOS_FORMAT format;
	UINT iPitch;
	UINT SurfaceAllocationIndex;
} CM_HAL_SURFACE2D_INFO_PARAM, *PCM_HAL_SURFACE2D_INFO_PARAM;

typedef struct _CM_HAL_SURFACE2D_SURFACE_STATE_DIMENSIONS_PARAM {
	UINT iWidth;
	UINT iHeight;
	DWORD dwHandle;
} CM_HAL_SURFACE2D_SURFACE_STATE_DIMENSIONS_PARAM,
    *PCM_HAL_SURFACE2D_SURFACE_STATE_DIMENSIONS_PARAM;

typedef struct _CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM {
	UINT iWidth;
	UINT iHeight;
	GENOS_FORMAT format;
	PVOID pData;
	UINT iPitch;
	UINT iLockFlag;
	DWORD dwHandle;
} CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM, *PCM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM;

typedef struct _CM_HAL_SURFACE2D_PARAM {
	UINT isAllocatedbyCmrtUmd;
	CmOsResource *pCmOsResource;
	UINT iWidth;
	UINT iHeight;
	GENOS_FORMAT format;
	PVOID pData;
	UINT iPitch;
	DWORD dwHandle;
} CM_HAL_SURFACE2D_PARAM, *PCM_HAL_SURFACE2D_PARAM;

typedef struct _CM_HAL_KERNEL_SETUP {
	GENHW_KERNEL_PARAM Param;
	Kdll_CacheEntry CacheEntry;
} CM_HAL_KERNEL_SETUP, *PCM_HAL_KERNEL_SETUP;

typedef struct _CM_HAL_MISC_STATE_MSG {
	union {
		struct {
			DWORD Row0:16;
			DWORD Reserved:8;
			DWORD Width:4;
			DWORD Height:4;
		};
		struct {
			DWORD value;
		};
	} DW0;

	union {
		struct {
			DWORD Row1:16;
			DWORD Row2:16;
		};
		struct {
			DWORD value;
		};
	} DW1;

	union {
		struct {
			DWORD Row3:16;
			DWORD Row4:16;
		};
		struct {
			DWORD value;
		};
	} DW2;

	union {
		struct {
			DWORD Row5:16;
			DWORD Row6:16;
		};
		struct {
			DWORD value;
		};
	} DW3;

	union {
		struct {
			DWORD Row7:16;
			DWORD Row8:16;
		};
		struct {
			DWORD value;
		};
	} DW4;

	union {
		struct {
			DWORD Row9:16;
			DWORD Row10:16;
		};
		struct {
			DWORD value;
		};
	} DW5;

	union {
		struct {
			DWORD Row11:16;
			DWORD Row12:16;
		};
		struct {
			DWORD value;
		};
	} DW6;

	union {
		struct {
			DWORD Row13:16;
			DWORD Row14:16;
		};
		struct {
			DWORD value;
		};
	} DW7;
} CM_HAL_MISC_STATE_MSG;

typedef struct _CM_HAL_BUFFER_ENTRY {
	GENOS_RESOURCE OsResource;
	UINT iSize;
	PVOID pAddress;
	PVOID pGmmResourceInfo;
	BOOL isAllocatedbyCmrtUmd;
} CM_HAL_BUFFER_ENTRY, *PCM_HAL_BUFFER_ENTRY;

typedef struct _CM_HAL_SURFACE2D_UP_ENTRY {
	GENOS_RESOURCE OsResource;
	UINT iWidth;
	UINT iHeight;
	GENOS_FORMAT format;
	PVOID pGmmResourceInfo;
} CM_HAL_SURFACE2D_UP_ENTRY, *PCM_HAL_SURFACE2D_UP_ENTRY;

typedef struct _CM_HAL_SURFACE2D_ENTRY {
	GENOS_RESOURCE OsResource;
	UINT iWidth;
	UINT iHeight;
	GENOS_FORMAT format;
	PVOID pGmmResourceInfo;
	UINT isAllocatedbyCmrtUmd;
	UINT iSurfaceStateWidth;
	UINT iSurfaceStateHeight;
	BOOL bReadSync;
} CM_HAL_SURFACE2D_ENTRY, *PCM_HAL_SURFACE2D_ENTRY;

typedef struct _CM_HAL_3DRESOURCE_ENTRY {
	GENOS_RESOURCE OsResource;
	UINT iWidth;
	UINT iHeight;
	UINT iDepth;
	GENOS_FORMAT format;
} CM_HAL_3DRESOURCE_ENTRY, *PCM_HAL_3DRESOURCE_ENTRY;

typedef struct _CM_HAL_TS_RESOURCE {
	GENOS_RESOURCE OsResource;
	BOOL bLocked;
	PBYTE pData;
} CM_HAL_TS_RESOURCE, *PCM_HAL_TS_RESOURCE;

typedef union _CM_HAL_MULTI_USE_BTI_ENTRY {
	struct {
		DWORD RegularSurfIndex:8;
	};
	struct {
		DWORD Value;
	};
} CM_HAL_MULTI_USE_BTI_ENTRY, *PCM_HAL_MULTI_USE_BTI_ENTRY;

typedef struct _CM_HAL_STATE {
	PLATFORM Platform;
	PGENOS_INTERFACE pOsInterface;
	PGENHW_HW_INTERFACE pHwInterface;
	PGENHW_BATCH_BUFFER pBatchBuffers;
	PCM_HAL_TASK_PARAM pTaskParam;
	PCM_HAL_TASK_TIMESTAMP pTaskTimeStamp;
	CM_HAL_TS_RESOURCE TsResource;
	PVOID pTableMem;
	CM_HAL_HINT_TASK_INDEXES HintIndexes;
	BOOL bRequestSingleSlice;
	BOOL bSLMMode;

	PCMLOOKUP_ENTRY pSurf2DTable;
	PCM_HAL_SURFACE2D_ENTRY pUmdSurf2DTable;
	PCM_HAL_BUFFER_ENTRY pBufferTable;
	PCM_HAL_SURFACE2D_UP_ENTRY pSurf2DUPTable;
	PCM_HAL_3DRESOURCE_ENTRY pSurf3DTable;
	PCHAR pTaskStatusTable;
	PCM_HAL_MULTI_USE_BTI_ENTRY pBT2DIndexTable;
	PCM_HAL_MULTI_USE_BTI_ENTRY pBT2DUPIndexTable;
	PCM_HAL_MULTI_USE_BTI_ENTRY pBT3DIndexTable;
	PCM_HAL_MULTI_USE_BTI_ENTRY pBTBufferIndexTable;

	CMSURFACE_REG_TABLE SurfaceRegTable;
	CM_HAL_DEVICE_PARAM CmDeviceParam;
	CM_HAL_KERNEL_SETUP KernelSetup;
	INT iNumBatchBuffers;
	DWORD dwDummyArg;
	CM_HAL_MAX_HW_THREAD_VALUES MaxHWThreadValues;
	GENHW_SCOREBOARD_PARAMS ScoreboardParams;
	GENHW_WALKER_PARAMS WalkerParams;
	PVOID pResourceList;
	CM_HAL_L3_CONFIG L3Config;

	BOOL bNullHwRenderCm;
	HMODULE hLibModule;
	DWORD cmDeubgBTIndex;

	pDrmVMapFnc pDrmVMap;

	CM_HAL_POWER_OPTION_PARAM PowerOption;
	BOOL bEUSaturationEnabled;
	BOOL bEUSaturationNoSSD;
	INT nNumKernelsInGSH;
	INT pTotalKernelSize[CM_MAX_GSH_KERNEL_ENTRIES];

	GENOS_GPU_CONTEXT GpuContext;
#ifndef CM_CHK_GENOSSTATUS
#define CM_CHK_GENOSSTATUS(_stmt)                                                    \
{                                                                               \
    hr = (GENOS_STATUS)(_stmt);                                                   \
    if (hr != GENOS_STATUS_SUCCESS)                                               \
    {                                                                           \
        CM_NORMALMESSAGE("hr check failed.");                                   \
        goto finish;                                                            \
    }                                                                           \
}
#endif

#ifndef CM_CHK_NULL_RETURN_GENOSSTATUS
#define CM_CHK_NULL_RETURN_GENOSSTATUS(_ptr)                                                       \
{                                                                               \
    if ((_ptr) == NULL)                                                         \
    {                                                                           \
        CM_ASSERTMESSAGE("Invalid (NULL) Pointer");                             \
        hr = GENOS_STATUS_NULL_POINTER;                                           \
        goto finish;                                                            \
    }                                                                           \
}
#endif

#ifndef CM_HRESULT2GENOSSTATUS_AND_CHECK
#define CM_HRESULT2GENOSSTATUS_AND_CHECK(_stmt)                                   \
{                                                                               \
    hr = (GENOS_STATUS)OsResultToGENOS_Status(_stmt);                               \
    if (hr != GENOS_STATUS_SUCCESS)                                               \
    {                                                                           \
        CM_NORMALMESSAGE("hr check failed.");                                   \
        goto finish;                                                            \
    }                                                                           \
}
#endif

	 GENOS_STATUS(*pfnCmAllocate)
	 (PCM_HAL_STATE pState);

	 GENOS_STATUS(*pfnGetMaxValues)
	 (PCM_HAL_STATE pState, PCM_HAL_MAX_VALUES pMaxValues);

	 GENOS_STATUS(*pfnGetMaxValuesEx)
	 (PCM_HAL_STATE pState, PCM_HAL_MAX_VALUES_EX pMaxValuesEx);

	 GENOS_STATUS(*pfnExecuteTask)
	 (PCM_HAL_STATE pState, PCM_HAL_EXEC_TASK_PARAM pParam);

	 GENOS_STATUS(*pfnExecuteGroupTask)
	 (PCM_HAL_STATE pState, PCM_HAL_EXEC_GROUP_TASK_PARAM pParam);

	 GENOS_STATUS(*pfnExecuteHintsTask)
	 (PCM_HAL_STATE pState, PCM_HAL_EXEC_HINTS_TASK_PARAM pParam);

	 GENOS_STATUS(*pfnQueryTask)
	 (PCM_HAL_STATE pState, PCM_HAL_QUERY_TASK_PARAM pParam);

	 GENOS_STATUS(*pfnAllocateBuffer)
	 (PCM_HAL_STATE pState, PCM_HAL_BUFFER_PARAM pParam);

	 GENOS_STATUS(*pfnFreeBuffer)
	 (PCM_HAL_STATE pState, DWORD dwHandle);

	 GENOS_STATUS(*pfnUpdateBuffer)
	 (PCM_HAL_STATE pState, DWORD dwHandle, DWORD dwSize);

	 GENOS_STATUS(*pfnUpdateSurface2D)
	 (PCM_HAL_STATE pState, DWORD dwHandle, DWORD dwWidth, DWORD dwHeight);

	 GENOS_STATUS(*pfnLockBuffer)
	 (PCM_HAL_STATE pState, PCM_HAL_BUFFER_PARAM pParam);

	 GENOS_STATUS(*pfnUnlockBuffer)
	 (PCM_HAL_STATE pState, PCM_HAL_BUFFER_PARAM pParam);

	 GENOS_STATUS(*pfnAllocateSurface2DUP)
	 (PCM_HAL_STATE pState, PCM_HAL_SURFACE2D_UP_PARAM pParam);

	 GENOS_STATUS(*pfnFreeSurface2DUP)
	 (PCM_HAL_STATE pState, DWORD dwHandle);

	 GENOS_STATUS(*pfnGetSurface2DPitchAndSize)
	 (PCM_HAL_STATE pState, PCM_HAL_SURFACE2D_UP_PARAM pParam);

	 GENOS_STATUS(*pfnAllocateSurface2D)
	 (PCM_HAL_STATE pState, PCM_HAL_SURFACE2D_PARAM pParam);

	 GENOS_STATUS(*pfnFreeSurface2D)
	 (PCM_HAL_STATE pState, DWORD dwHandle);

	 GENOS_STATUS(*pfnLock2DResource)
	 (PCM_HAL_STATE pState, PCM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM pParam);

	 GENOS_STATUS(*pfnUnlock2DResource)
	 (PCM_HAL_STATE pState, PCM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM pParam);

	 GENOS_STATUS(*pfnGetSurface2DTileYPitch)
	 (PCM_HAL_STATE pState, PCM_HAL_SURFACE2D_PARAM pParam);

	 GENOS_STATUS(*pfnSetCaps)
	 (PCM_HAL_STATE pState, PCM_HAL_MAX_SET_CAPS_PARAM pParam);

	 GENOS_STATUS(*pfnGetGPUCurrentFrequency)
	 (PCM_HAL_STATE pState, UINT * pGPUCurrentFreq);

	 GENOS_STATUS(*pfnSet2DSurfaceStateDimensions)
	 (PCM_HAL_STATE pState,
	  PCM_HAL_SURFACE2D_SURFACE_STATE_DIMENSIONS_PARAM pParam);

	 GENOS_STATUS(*pfnSetPowerOption)
	 (PCM_HAL_STATE pState, PCM_HAL_POWER_OPTION_PARAM pPowerOption);

	 GENOS_STATUS(*pfnEscapeCallKMD)
	 (PCM_HAL_STATE pState, CM_KMD_ESCAPE_CALL nEscapeCall, PVOID pData);

	 GENOS_STATUS(*pfnSubmitCommands)
	 (PCM_HAL_STATE pState,
	  PGENHW_BATCH_BUFFER pBatchBuffer,
	  INT iTaskId, PCM_HAL_KERNEL_PARAM * pKernels, PVOID * ppCmdBuffer);

	 INT(*pfnGetTaskSyncLocation)
	 (INT iTaskId);

	 GENOS_STATUS(*pfnSetSurfaceMemoryObjectControl)
	 (PCM_HAL_STATE pState,
	  WORD wMemObjCtl, PGENHW_SURFACE_STATE_PARAMS pParams);

	 INT(*pfnGetCurbeBlockAlignSize) ();

	 GENOS_STATUS(*pfnGetUserDefinedThreadCountPerThreadGroup)
	 (PCM_HAL_STATE pState, UINT * pThreadsPerThreadGroup);

	 GENOS_STATUS(*pfnGetGpuTime)
	 (PCM_HAL_STATE pState, PUINT64 piGpuTime);

	 GENOS_STATUS(*pfnConvertToQPCTime)
	 (UINT64 nanoseconds, LARGE_INTEGER * QPCTime);

	 GENOS_STATUS(*pfnGetGlobalTime)
	 (LARGE_INTEGER * pGlobalTime);

	 GENOS_STATUS(*pfnSendMediaWalkerState)
	 (PCM_HAL_STATE pState,
	  PCM_HAL_KERNEL_PARAM pKernelParam, PGENOS_COMMAND_BUFFER pCmdBuffer);

	 GENOS_STATUS(*pfnSendGpGpuWalkerState)
	 (PCM_HAL_STATE pState,
	  PCM_HAL_KERNEL_PARAM pKernelParam, PGENOS_COMMAND_BUFFER pCmdBuffer);

	 GENOS_STATUS(*pfnSendCommandBufferHeaderEUSaturation)
	 (PCM_HAL_STATE pState, PGENOS_COMMAND_BUFFER pCmdBuffer);

	 GENOS_STATUS(*pfnSetSurfaceReadFlag)
	 (PCM_HAL_STATE pState, DWORD dwHandle);

} CM_HAL_STATE;

__inline GENOS_STATUS HalCm_GetNewTaskId(PCM_HAL_STATE pState, PINT piIndex)
{
	UINT i;
	UINT maxTasks;

	maxTasks = pState->CmDeviceParam.iMaxTasks;

	for (i = 0; i < maxTasks; i++) {
		if (pState->pTaskStatusTable[i] == CM_INVALID_INDEX) {
			*piIndex = i;
			return GENOS_STATUS_SUCCESS;
		}
	}

	CM_ASSERTMESSAGE("Unable to find a free slot for Task.");
	return GENOS_STATUS_UNKNOWN;
}

GENOS_STATUS HalCm_Create(PGENOS_CONTEXT pOsDriverContext,
			  PCM_HAL_CREATE_PARAM pCmCreateParam,
			  PCM_HAL_STATE * pCmState);

VOID HalCm_Destroy(PCM_HAL_STATE pState);

VOID HalCm_GetRegistrySettings(PCM_HAL_STATE);

GENOS_STATUS HalCm_GetSurfaceDetails(PCM_HAL_STATE pState,
				     PCM_HAL_INDEX_PARAM pIndexParam,
				     UINT iBTIndex,
				     GENHW_SURFACE & Surface,
				     SHORT globalSurface,
				     PGENHW_SURFACE_STATE_ENTRY pSurfaceEntry,
				     UINT dwTempPlaneIndex,
				     GENHW_SURFACE_STATE_PARAMS SurfaceParam,
				     CM_HAL_KERNEL_ARG_KIND argKind);

GENOS_STATUS HalCm_AllocateTsResource(PCM_HAL_STATE pState);

GENOS_STATUS HalCm_AllocateTables(PCM_HAL_STATE pState);

GENOS_STATUS HalCm_Allocate(PCM_HAL_STATE pState);

VOID HalCm_OsInitInterface(PCM_HAL_STATE pCmState);

GENOS_STATUS HalCm_GetSurfaceAndRegister(PCM_HAL_STATE pState,
					 PGENHW_SURFACE pSurface,
					 CM_HAL_KERNEL_ARG_KIND surfKind,
					 UINT iIndex);

GENOS_STATUS HalCm_SendMediaWalkerState(PCM_HAL_STATE pState,
					PCM_HAL_KERNEL_PARAM pKernelParam,
					PGENOS_COMMAND_BUFFER pCmdBuffer);

GENOS_STATUS HalCm_SendGpGpuWalkerState(PCM_HAL_STATE pState,
					PCM_HAL_KERNEL_PARAM pKernelParam,
					PGENOS_COMMAND_BUFFER pCmdBuffer);

DWORD Halcm_GetFreeBindingIndex(PCM_HAL_STATE pState,
				PCM_HAL_INDEX_PARAM pIndexParam, DWORD count);

void Halcm_PreSetBindingIndex(PCM_HAL_INDEX_PARAM pIndexParam,
			      DWORD start, DWORD end);

GENOS_STATUS HalCm_Setup2DSurfaceStateWithBTIndex(PCM_HAL_STATE pState,
						  INT iBindingTable,
						  UINT surfIndex, UINT btIndex);

GENOS_STATUS HalCm_SetupBufferSurfaceStateWithBTIndex(PCM_HAL_STATE pState,
						      INT iBindingTable,
						      UINT surfIndex,
						      UINT btIndex);

GENOS_STATUS HalCm_Setup2DSurfaceUPStateWithBTIndex(PCM_HAL_STATE pState,
						    INT iBindingTable,
						    UINT surfIndex,
						    UINT btIndex);

VOID HalCm_OsResource_Unreference(PGENOS_RESOURCE pOsResource);

VOID HalCm_OsResource_Reference(PGENOS_RESOURCE pOsResource);

GENOS_STATUS HalCm_SetSurfaceReadFlag(PCM_HAL_STATE pState, DWORD dwHandle);

#endif
