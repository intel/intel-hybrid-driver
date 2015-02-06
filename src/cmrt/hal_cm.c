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
#include "os_interface.h"
#include "hw_interface.h"
#include "hal_cm.h"
#include "hal_cm_g75.h"
#include "hal_cm_g8.h"
#include "cm_def.h"
#include <math.h>

VOID HalCm_OsInitInterface(PCM_HAL_STATE pCmState);
VOID HalCm_OsResource_Reference(PGENOS_RESOURCE pOsResource);

VOID HalCm_OsResource_Unreference(PGENOS_RESOURCE pOsResource);

__inline UINT HalCm_GetPow2Aligned(UINT d)
{
	CM_ASSERT(d > 0);

	--d;

	d |= d >> 1;
	d |= d >> 2;
	d |= d >> 4;
	d |= d >> 8;
	d |= d >> 16;

	return ++d;
}

BOOLEAN HalCm_GetTaskHasThreadArg(PCM_HAL_KERNEL_PARAM * pKernels,
				  UINT numKernels)
{
	PCM_HAL_KERNEL_PARAM pKernelParam;
	PCM_HAL_KERNEL_ARG_PARAM pArgParam;
	BOOLEAN threadArgExists = FALSE;

	for (UINT iKrn = 0; iKrn < numKernels; iKrn++) {
		pKernelParam = pKernels[iKrn];
		for (UINT argIndex = 0; argIndex < pKernelParam->iNumArgs;
		     argIndex++) {
			pArgParam = &pKernelParam->CmArgParams[argIndex];
			if (pArgParam->bPerThread) {
				threadArgExists = TRUE;
				break;
			}
		}

		if (threadArgExists)
			break;
	}

	return threadArgExists;
}

GENOS_STATUS HalCm_AllocateTsResource(PCM_HAL_STATE pState)
{
	GENOS_STATUS hr;
	UINT iSize;
	PGENOS_INTERFACE pOsInterface;
	GENOS_ALLOC_GFXRES_PARAMS AllocParams;
	GENOS_LOCK_PARAMS LockFlags;

	hr = GENOS_STATUS_SUCCESS;
	pOsInterface = pState->pHwInterface->pOsInterface;

	iSize =
	    (sizeof(UINT64) * CM_SYNC_QWORD_PER_TASK) *
	    pState->CmDeviceParam.iMaxTasks;

	GENOS_ZeroMemory(&AllocParams, sizeof(GENOS_ALLOC_GFXRES_PARAMS));
	AllocParams.Type = GENOS_GFXRES_BUFFER;
	AllocParams.dwBytes = iSize;
	AllocParams.Format = Format_Buffer;
	AllocParams.TileType = GENOS_TILE_LINEAR;
	AllocParams.pBufName = "TsResource";

	CM_HRESULT2GENOSSTATUS_AND_CHECK(pOsInterface->pfnAllocateResource
					 (pOsInterface, &AllocParams,
					  &pState->TsResource.OsResource));

	GENOS_ZeroMemory(&LockFlags, sizeof(GENOS_LOCK_PARAMS));

	LockFlags.ReadOnly = 1;

	pState->TsResource.pData =
	    (PBYTE) pOsInterface->pfnLockResource(pOsInterface,
						  &pState->
						  TsResource.OsResource,
						  &LockFlags);

	CM_CHK_NULL_RETURN_GENOSSTATUS(pState->TsResource.pData);

	pState->TsResource.bLocked = TRUE;

 finish:
	return hr;
}

__inline VOID HalCm_FreeTsResource(PCM_HAL_STATE pState)
{
	PGENOS_INTERFACE pOsInterface;

	if (!pState || !pState->pHwInterface
	    || !pState->pHwInterface->pOsInterface) {
		return;
	}
	pOsInterface = pState->pHwInterface->pOsInterface;

	if (!IntelGen_OsResourceIsNull(&pState->TsResource.OsResource)) {
		if (pState->TsResource.bLocked) {
			pOsInterface->pfnUnlockResource(pOsInterface,
							&pState->
							TsResource.OsResource);
		}

		pOsInterface->pfnFreeResource(pOsInterface,
					      &pState->TsResource.OsResource);
	}
}

__inline GENOS_STATUS HalCm_GetGfxMapFilter(GFX_DDITEXTUREFILTERTYPE GfxFilter,
					    GFX3DSTATE_MAPFILTER * pGfxFilter)
{
	GENOS_STATUS hr;
	hr = GENOS_STATUS_SUCCESS;

	switch (GfxFilter) {
	case GFX_DDITEXF_LINEAR:
		*pGfxFilter = GFX3DSTATE_MAPFILTER_LINEAR;
		break;
	case GFX_DDITEXF_POINT:
		*pGfxFilter = GFX3DSTATE_MAPFILTER_NEAREST;
		break;
	case GFX_DDITEXF_ANISOTROPIC:
		*pGfxFilter = GFX3DSTATE_MAPFILTER_ANISOTROPIC;
		break;
	default:
		CM_ERROR_ASSERT("Filter '%d' not supported", GfxFilter);
		goto finish;
	}

 finish:
	return hr;
}

__inline GENOS_STATUS HalCm_GetGfxTextAddress(GFX_TEXTUREADDRESS GfxAddress,
					      GFX3DSTATE_TEXCOORDMODE *
					      pGfxAddress)
{
	GENOS_STATUS hr;
	hr = GENOS_STATUS_SUCCESS;

	switch (GfxAddress) {
	case GFX_TADDRESS_WRAP:
		*pGfxAddress = GFX3DSTATE_TEXCOORDMODE_WRAP;
		break;
	case GFX_TADDRESS_MIRROR:
		*pGfxAddress = GFX3DSTATE_TEXCOORDMODE_MIRROR;
		break;
	case GFX_TADDRESS_CLAMP:
		*pGfxAddress = GFX3DSTATE_TEXCOORDMODE_CLAMP;
		break;
	case GFX_TADDRESS_BORDER:
		*pGfxAddress = GFX3DSTATE_TEXCOORDMODE_CLAMP_BORDER;
		break;
	default:
		CM_ERROR_ASSERT("Address '%d' not supported", GfxAddress);
		goto finish;
	}

 finish:
	return hr;
}

__inline DWORD HalCm_GetKernelField(CONST PBYTE pKernel,
				    UINT iOffset, DWORD dwSize)
{
	DWORD dwData = 0;

	CM_ASSERT((dwSize > 0) && (dwSize <= sizeof(dwData)));

	GENOS_SecureMemcpy(&dwData, dwSize, (pKernel + iOffset), dwSize);
	return dwData;
}

__inline DWORD HalCm_GetThreadField(CONST PBYTE pValue,
				    DWORD dwSize, UINT iThreadIndex)
{
	DWORD dwData;
	UINT iOffset;

	CM_ASSERT((dwSize > 0) && (dwSize <= sizeof(dwData)));

	iOffset = (iThreadIndex * dwSize);
	GENOS_SecureMemcpy(&dwData, dwSize, (pValue + iOffset), dwSize);
	return dwData;
}

__inline DWORD HalCm_GetBlockArg(CONST PBYTE pKernel,
				 UINT iArgIndex,
				 DWORD dwRelOffset, DWORD dwSize)
{
	DWORD dwOffset;

	dwOffset =
	    CM_KNL_POS_ARG_BLOCK_BASE + (iArgIndex * CM_KNL_SZ_ARG_BLOCK) +
	    dwRelOffset;

	return HalCm_GetKernelField(pKernel, dwOffset, dwSize);
}

__inline VOID HalCm_SetArgData(PCM_HAL_KERNEL_ARG_PARAM pArgParam,
			       UINT iThreadIndex, PBYTE pBuffer)
{
	PBYTE pDst;
	PBYTE pSrc;

	pDst = pBuffer + pArgParam->iPayloadOffset;
	pSrc = pArgParam->pFirstValue + (iThreadIndex * pArgParam->iUnitSize);

	GENOS_SecureMemcpy(pDst, pArgParam->iUnitSize, pSrc,
			   pArgParam->iUnitSize);
}

__inline GENOS_STATUS HalCm_GetResourceUPEntry(PCM_HAL_STATE pState,
					       DWORD dwHandle,
					       PCM_HAL_SURFACE2D_UP_ENTRY *
					       pEntryOut)
{
	GENOS_STATUS hr;
	PCM_HAL_SURFACE2D_UP_ENTRY pEntry;

	hr = GENOS_STATUS_SUCCESS;

	if (dwHandle >= pState->CmDeviceParam.iMax2DSurfaceUPTableSize) {
		CM_ERROR_ASSERT("Invalid handle '%d'", dwHandle);
		goto finish;
	}

	pEntry = &pState->pSurf2DUPTable[dwHandle];
	if (pEntry->iWidth == 0) {
		CM_ERROR_ASSERT("handle '%d' is not set", dwHandle);
		goto finish;
	}

	*pEntryOut = pEntry;

 finish:
	return hr;
}

__inline GENOS_STATUS HalCm_GetBufferEntry(PCM_HAL_STATE pState,
					   DWORD dwHandle,
					   PCM_HAL_BUFFER_ENTRY * pEntryOut)
{
	GENOS_STATUS hr;
	PCM_HAL_BUFFER_ENTRY pEntry;

	hr = GENOS_STATUS_SUCCESS;

	if (dwHandle >= pState->CmDeviceParam.iMaxBufferTableSize) {
		CM_ERROR_ASSERT("Invalid handle '%d'", dwHandle);
		goto finish;
	}

	pEntry = &pState->pBufferTable[dwHandle];
	if (pEntry->iSize == 0) {
		CM_ERROR_ASSERT("handle '%d' is not set", dwHandle);
		goto finish;
	}

	*pEntryOut = pEntry;

 finish:
	return hr;
}

__inline GENOS_STATUS HalCm_GetSurface2DEntry(PCM_HAL_STATE pState,
					      DWORD dwHandle,
					      PCM_HAL_SURFACE2D_ENTRY *
					      pEntryOut)
{
	GENOS_STATUS hr;
	PCM_HAL_SURFACE2D_ENTRY pEntry;

	hr = GENOS_STATUS_SUCCESS;

	if (dwHandle >= pState->CmDeviceParam.iMax2DSurfaceTableSize) {
		CM_ERROR_ASSERT("Invalid handle '%d'", dwHandle);
		goto finish;
	}

	pEntry = &pState->pUmdSurf2DTable[dwHandle];
	if ((pEntry->iWidth == 0) || (pEntry->iHeight == 0)) {
		CM_ERROR_ASSERT("handle '%d' is not set", dwHandle);
		goto finish;
	}

	*pEntryOut = pEntry;

 finish:
	return hr;
}

GENOS_STATUS HalCm_AllocateTables(PCM_HAL_STATE pState)
{
	GENOS_STATUS hr;
	PCM_HAL_DEVICE_PARAM pDeviceParam;
	PBYTE pb;
	UINT iLookUpTableSize;
	UINT iTaskStatusTableSize;
	UINT iBT2DIndexTableSize;
	UINT iBT2DUPIndexTableSize;
	UINT iBTBufferIndexTableSize;
	UINT iBufferTableSize;
	UINT i2DSURFUPTableSize;
	UINT iSize;
	UINT i2DSURFTableSize;

	hr = GENOS_STATUS_SUCCESS;
	pDeviceParam = &pState->CmDeviceParam;

	iLookUpTableSize = pDeviceParam->iMax2DSurfaceTableSize *
	    sizeof(CMLOOKUP_ENTRY);
	i2DSURFTableSize = pDeviceParam->iMax2DSurfaceTableSize *
	    sizeof(CM_HAL_SURFACE2D_ENTRY);
	iBufferTableSize = pDeviceParam->iMaxBufferTableSize *
	    sizeof(CM_HAL_BUFFER_ENTRY);
	i2DSURFUPTableSize = pDeviceParam->iMax2DSurfaceUPTableSize *
	    sizeof(CM_HAL_SURFACE2D_UP_ENTRY);
	iTaskStatusTableSize = pDeviceParam->iMaxTasks * sizeof(CHAR);
	iBT2DIndexTableSize =
	    pDeviceParam->iMax2DSurfaceTableSize *
	    sizeof(CM_HAL_MULTI_USE_BTI_ENTRY);
	iBT2DUPIndexTableSize =
	    pDeviceParam->iMax2DSurfaceUPTableSize *
	    sizeof(CM_HAL_MULTI_USE_BTI_ENTRY);
	iBTBufferIndexTableSize =
	    pDeviceParam->iMaxBufferTableSize *
	    sizeof(CM_HAL_MULTI_USE_BTI_ENTRY);

	iSize = iLookUpTableSize +
	    i2DSURFTableSize +
	    iBufferTableSize +
	    i2DSURFUPTableSize +
	    iTaskStatusTableSize +
	    iBT2DIndexTableSize +
	    iBT2DUPIndexTableSize + iBTBufferIndexTableSize;

	pState->pTableMem = GENOS_AllocAndZeroMemory(iSize);
	CM_CHK_NULL_RETURN_GENOSSTATUS(pState->pTableMem);
	pb = (PBYTE) pState->pTableMem;

	pState->pSurf2DTable = (PCMLOOKUP_ENTRY) pb;
	pb += iLookUpTableSize;

	pState->pUmdSurf2DTable = (PCM_HAL_SURFACE2D_ENTRY) pb;
	pb += i2DSURFTableSize;

	pState->pBufferTable = (PCM_HAL_BUFFER_ENTRY) pb;
	pb += iBufferTableSize;

	pState->pSurf2DUPTable = (PCM_HAL_SURFACE2D_UP_ENTRY) pb;
	pb += i2DSURFUPTableSize;

	pState->pTaskStatusTable = (PCHAR) pb;
	pb += iTaskStatusTableSize;

	pState->pBT2DIndexTable = (PCM_HAL_MULTI_USE_BTI_ENTRY) pb;
	pb += iBT2DIndexTableSize;

	pState->pBT2DUPIndexTable = (PCM_HAL_MULTI_USE_BTI_ENTRY) pb;
	pb += iBT2DUPIndexTableSize;

	pState->pBTBufferIndexTable = (PCM_HAL_MULTI_USE_BTI_ENTRY) pb;
	pb += iBTBufferIndexTableSize;

 finish:
	return hr;
}

GENOS_STATUS HalCm_AddKernelIDTag(PCM_HAL_KERNEL_PARAM * pKernels,
				  UINT iNumKernels,
				  UINT iNumTasks, UINT iNumCurrentTask)
{
	UINT i;
	UINT64 numTasks;
	UINT64 numCurrentTask;
	UINT64 numTasksMask;
	UINT64 numCurrentTaskMask;

	numTasks = iNumTasks;
	numCurrentTask = iNumCurrentTask;
	numTasksMask = numTasks << 45;
	numCurrentTaskMask = numCurrentTask << 42;

	for (i = 0; i < iNumKernels; ++i) {
		pKernels[i]->uiKernelId |= numTasksMask;
		pKernels[i]->uiKernelId |= numCurrentTaskMask;
	}

	return GENOS_STATUS_SUCCESS;
}

GENOS_STATUS HalCm_GetBatchBuffer(PCM_HAL_STATE pState,
				  UINT iNumKernels,
				  PCM_HAL_KERNEL_PARAM * pKernels,
				  PGENHW_BATCH_BUFFER * ppBb)
{
	GENOS_STATUS hr;
	PGENHW_BATCH_BUFFER pBb = NULL;
	PGENHW_HW_INTERFACE pHwInterface;
	INT iSize;
	UINT i;
	UINT j;
	UINT k;
	INT iFreeIdx;
	UINT64 uiKernelIds[CM_MAX_KERNELS_PER_TASK];
	UINT64 uiKernelParamsIds[CM_MAX_KERNELS_PER_TASK];
	CM_HAL_BB_DIRTY_STATUS bbDirtyStatus;

	hr = GENOS_STATUS_SUCCESS;
	pHwInterface = pState->pHwInterface;
	iFreeIdx = CM_INVALID_INDEX;
	bbDirtyStatus = CM_HAL_BB_CLEAN;

	iSize = HalCm_GetPow2Aligned(pState->pTaskParam->iBatchBufferSize);

	GENOS_ZeroMemory(&uiKernelIds,
			 CM_MAX_KERNELS_PER_TASK * sizeof(UINT64));
	GENOS_ZeroMemory(&uiKernelParamsIds,
			 CM_MAX_KERNELS_PER_TASK * sizeof(UINT64));

	if (iSize > CM_MAX_BB_SIZE) {
		CM_ERROR_ASSERT_RETURN(GENOS_STATUS_EXCEED_MAX_BB_SIZE,
				       "Batch Buffer Size exeeceds Max '%d'",
				       iSize);
		goto finish;
	}

	for (i = 0; i < iNumKernels; ++i) {
		uiKernelParamsIds[i] = ((pKernels[i])->uiKernelId << 16) >> 16;
	}

	bbDirtyStatus = CM_HAL_BB_CLEAN;
	for (k = 0; k < iNumKernels; ++k) {
		if (pKernels[k]->CmKernelThreadSpaceParam.BBdirtyStatus ==
		    CM_HAL_BB_DIRTY) {
			bbDirtyStatus = CM_HAL_BB_DIRTY;
			break;
		}
	}

	for (i = 0; i < (UINT) pState->iNumBatchBuffers; i++) {
		pBb = &pState->pBatchBuffers[i];
		CM_CHK_NULL_RETURN_GENOSSTATUS(pBb);

		if (!IntelGen_OsResourceIsNull(&pBb->OsResource)) {
			GENOS_FillMemory(uiKernelIds,
					 sizeof(UINT64) *
					 CM_MAX_KERNELS_PER_TASK, 0);
			for (j = 0; j < iNumKernels; j++) {
				uiKernelIds[j] = uiKernelParamsIds[j];
			}
			if (RtlEqualMemory
			    (uiKernelIds,
			     pBb->pBBRenderData->BbArgs.BbCmArgs.uiKernelIds,
			     sizeof(UINT64) * CM_MAX_KERNELS_PER_TASK)) {
				if (pBb->bBusy
				    && bbDirtyStatus == CM_HAL_BB_DIRTY) {
					pBb->pBBRenderData->BbArgs.
					    BbCmArgs.bLatest = FALSE;
				} else if (pBb->pBBRenderData->BbArgs.
					   BbCmArgs.bLatest == TRUE) {
					break;
				}
			}
		}
	}
	if (i < (UINT) pState->iNumBatchBuffers) {
		CM_CHK_NULL_RETURN_GENOSSTATUS(pBb);
		pBb->pBBRenderData->BbArgs.BbCmArgs.uiRefCount++;
		pBb->iCurrent = 0;
		pBb->dwSyncTag = 0;
		*ppBb = pBb;
		hr = GENOS_STATUS_SUCCESS;
		goto finish;
	}

	for (i = 0; i < (UINT) pState->iNumBatchBuffers; i++) {
		pBb = &pState->pBatchBuffers[i];
		CM_CHK_NULL_RETURN_GENOSSTATUS(pBb);

		if (IntelGen_OsResourceIsNull(&pBb->OsResource)) {
			iFreeIdx = i;
			break;
		}
	}

	if (iFreeIdx == CM_INVALID_INDEX) {
		for (i = 0; i < (UINT) pState->iNumBatchBuffers; i++) {
			pBb = &pState->pBatchBuffers[i];
			CM_CHK_NULL_RETURN_GENOSSTATUS(pBb);
			if (!pBb->bBusy) {
				if (pBb->iSize >= iSize) {
					pBb->iCurrent = 0;
					pBb->dwSyncTag = 0;

					pBb->pBBRenderData->BbArgs.
					    BbCmArgs.uiRefCount = 1;
					for (i = 0; i < iNumKernels; i++) {
						pBb->pBBRenderData->
						    BbArgs.BbCmArgs.
						    uiKernelIds[i] =
						    uiKernelParamsIds[i];
					}

					pBb->pBBRenderData->BbArgs.
					    BbCmArgs.bLatest = TRUE;

					*ppBb = pBb;
					hr = GENOS_STATUS_SUCCESS;
					goto finish;
				}

				if (iFreeIdx == CM_INVALID_INDEX) {
					iFreeIdx = i;
				}
			}
		}
	}
	if (iFreeIdx == CM_INVALID_INDEX) {
		CM_ERROR_ASSERT("No batch buffer available");
		goto finish;
	}

	pBb = &pState->pBatchBuffers[iFreeIdx];
	CM_CHK_NULL_RETURN_GENOSSTATUS(pBb);

	pBb->pBBRenderData->BbArgs.BbCmArgs.uiRefCount = 1;
	for (i = 0; i < iNumKernels; i++) {
		pBb->pBBRenderData->BbArgs.BbCmArgs.uiKernelIds[i] =
		    uiKernelParamsIds[i];
	}

	pBb->pBBRenderData->BbArgs.BbCmArgs.bLatest = TRUE;

	if (!IntelGen_OsResourceIsNull(&pBb->OsResource)) {
		CM_CHK_GENOSSTATUS(pHwInterface->pfnFreeBB(pHwInterface, pBb));
	}
	CM_CHK_GENOSSTATUS(pHwInterface->pfnAllocateBB
			   (pHwInterface, pBb, iSize));
	*ppBb = pBb;

 finish:
	return hr;
}

GENOS_STATUS HalCm_ParseTask(PCM_HAL_STATE pState,
			     PCM_HAL_EXEC_TASK_PARAM pExecParam)
{
	GENOS_STATUS hr;
	PCM_HAL_TASK_PARAM pTaskParam;
	PCM_HAL_KERNEL_PARAM pKernelParam;
	UINT iHdrSize;
	UINT iTotalThreads;
	UINT iKrn;
	PGENHW_SCOREBOARD_PARAMS pScoreboardParams;
	bool bNonstallingScoreboardEnable;
	CM_HAL_DEPENDENCY vfeDependencyInfo;
	PCM_HAL_KERNEL_THREADSPACE_PARAM pKernelTSParam;
	UINT i, j, k;
	BYTE reuseBBUpdateMask;
	BOOLEAN bitIsSet;
	PCM_HAL_MASK_AND_RESET pDependencyMask;
	UINT uSurfaceNumber;
	UINT uSurfaceIndex;
	BOOLEAN threadArgExists;

	hr = GENOS_STATUS_SUCCESS;
	iTotalThreads = 0;
	pTaskParam = pState->pTaskParam;
	pTaskParam->iBatchBufferSize = 0;
	bNonstallingScoreboardEnable = TRUE;
	reuseBBUpdateMask = 0;
	bitIsSet = FALSE;
	threadArgExists = FALSE;
	iHdrSize =
	    pState->pHwInterface->pHwCommands->dwMediaObjectHeaderCmdSize;
	pTaskParam->iPreemptionMode = UN_PREEMPTABLE_MODE;
	pTaskParam->DependencyPattern = pExecParam->DependencyPattern;
	pTaskParam->threadSpaceWidth = pExecParam->threadSpaceWidth;
	pTaskParam->threadSpaceHeight = pExecParam->threadSpaceHeight;
	pTaskParam->WalkingPattern = pExecParam->WalkingPattern;
	pTaskParam->walkingParamsValid = pExecParam->walkingParamsValid;
	pTaskParam->dependencyVectorsValid = pExecParam->dependencyVectorsValid;
	if (pTaskParam->walkingParamsValid) {
		pTaskParam->walkingParams = pExecParam->walkingParams;
	}
	if (pTaskParam->dependencyVectorsValid) {
		pTaskParam->dependencyVectors = pExecParam->dependencyVectors;
	}
	pTaskParam->SurEntryInfoArrays = pExecParam->SurEntryInfoArrays;

	pTaskParam->surfacePerBT = 0;

	pTaskParam->ColorCountMinusOne = pExecParam->ColorCountMinusOne;

	if (pExecParam->ppThreadCoordinates) {
		pTaskParam->ppThreadCoordinates =
		    pExecParam->ppThreadCoordinates;
	}

	pTaskParam->ppDependencyMasks = pExecParam->ppDependencyMasks;
	pTaskParam->uiSyncBitmap = pExecParam->uiSyncBitmap;
	pTaskParam->uiNumKernels = pExecParam->iNumKernels;

	pState->WalkerParams.CmWalkerEnable = TRUE;

	for (iKrn = 0; iKrn < pExecParam->iNumKernels; iKrn++) {
		if ((pExecParam->pKernels[iKrn] == NULL) ||
		    (pExecParam->piKernelSizes[iKrn] == 0)) {
			CM_ERROR_ASSERT("Invalid Kernel data");
			goto finish;
		}

		pKernelParam =
		    (PCM_HAL_KERNEL_PARAM) pExecParam->pKernels[iKrn];
		PCM_INDIRECT_SURFACE_INFO pIndirectSurfaceInfo =
		    pKernelParam->CmIndirectDataParam.pSurfaceInfo;
		uSurfaceNumber = 0;
		if (pKernelParam->CmIndirectDataParam.iSurfaceCount) {
			uSurfaceIndex = 0;
			for (i = 0;
			     i <
			     pKernelParam->CmIndirectDataParam.iSurfaceCount;
			     i++) {
				uSurfaceIndex =
				    (pIndirectSurfaceInfo +
				     i)->iBindingTableIndex >
				    uSurfaceIndex ? (pIndirectSurfaceInfo +
						     i)->iBindingTableIndex :
				    uSurfaceIndex;
				uSurfaceNumber++;
			}
			pTaskParam->surfacePerBT =
			    pTaskParam->surfacePerBT >
			    uSurfaceIndex ? pTaskParam->surfacePerBT :
			    uSurfaceIndex;
		}

		uSurfaceNumber += pKernelParam->iNumSurfaces;
		pTaskParam->surfacePerBT =
		    pTaskParam->surfacePerBT <
		    uSurfaceNumber ? uSurfaceNumber : pTaskParam->surfacePerBT;

		if (pKernelParam->iPayloadSize == 0) {
			if ((pKernelParam->
			     CmKernelThreadSpaceParam.iThreadSpaceWidth != 0)
			    && (pKernelParam->
				CmKernelThreadSpaceParam.patternType !=
				CM_DEPENDENCY_WAVEFRONT26Z)
			    && (pKernelParam->
				CmKernelThreadSpaceParam.patternType !=
				CM_DEPENDENCY_WAVEFRONT26ZI)
			    && (pKernelParam->
				CmKernelThreadSpaceParam.pThreadCoordinates ==
				NULL)) {
				pKernelParam->WalkerParams.CmWalkerEnable =
				    TRUE;
			} else if (pKernelParam->
				   CmKernelThreadSpaceParam.iThreadSpaceWidth ==
				   0) {
				if (pState->pTaskParam->ppThreadCoordinates) {
					if (pState->
					    pTaskParam->ppThreadCoordinates
					    [iKrn] == NULL) {
						pKernelParam->
						    WalkerParams.CmWalkerEnable
						    = TRUE;
					}
				} else {
					pKernelParam->
					    WalkerParams.CmWalkerEnable = TRUE;
				}
			}
		}
		pState->WalkerParams.CmWalkerEnable &=
		    pKernelParam->WalkerParams.CmWalkerEnable;

		if (!pState->WalkerParams.CmWalkerEnable) {
			pTaskParam->iBatchBufferSize +=
			    pKernelParam->iNumThreads * (iHdrSize +
							 MAX
							 (pKernelParam->iPayloadSize,
							  4));
		}

		iTotalThreads += pKernelParam->iNumThreads;

	}

	pTaskParam->iBatchBufferSize += CM_EXTRA_BB_SPACE;

	pScoreboardParams = &pState->ScoreboardParams;
	pScoreboardParams->numMask = 0;
	pScoreboardParams->ScoreboardType = bNonstallingScoreboardEnable;

	GENOS_ZeroMemory(&vfeDependencyInfo, sizeof(CM_HAL_DEPENDENCY));
	for (iKrn = 0; iKrn < pExecParam->iNumKernels; iKrn++) {
		pKernelParam = pExecParam->pKernels[iKrn];
		pKernelTSParam = &pKernelParam->CmKernelThreadSpaceParam;

		if (pKernelTSParam->dependencyInfo.count
		    || pKernelTSParam->dependencyVectorsValid) {
			if (vfeDependencyInfo.count == 0) {
				GENOS_SecureMemcpy(&vfeDependencyInfo,
						   sizeof(CM_HAL_DEPENDENCY),
						   &pKernelTSParam->dependencyInfo,
						   sizeof(CM_HAL_DEPENDENCY));
				pKernelTSParam->globalDependencyMask =
				    (1 << vfeDependencyInfo.count) - 1;
			} else {
				UINT count = 0;
				CM_HAL_DEPENDENCY dependencyInfo;
				if (pKernelTSParam->dependencyVectorsValid) {
					count =
					    pKernelTSParam->
					    dependencyVectors.count;
					GENOS_SecureMemcpy
					    (&dependencyInfo.deltaX,
					     sizeof(INT) * count,
					     &pKernelTSParam->dependencyVectors.deltaX,
					     sizeof(INT) * count);
					GENOS_SecureMemcpy
					    (&dependencyInfo.deltaY,
					     sizeof(INT) * count,
					     &pKernelTSParam->dependencyVectors.deltaY,
					     sizeof(INT) * count);
				} else {
					count =
					    pKernelTSParam->
					    dependencyInfo.count;
					GENOS_SecureMemcpy
					    (&dependencyInfo.deltaX,
					     sizeof(INT) * count,
					     &pKernelTSParam->dependencyInfo.
					     deltaX, sizeof(INT) * count);
					GENOS_SecureMemcpy
					    (&dependencyInfo.deltaY,
					     sizeof(INT) * count,
					     &pKernelTSParam->dependencyInfo.
					     deltaY, sizeof(INT) * count);
				}

				for (j = 0; j < count; ++j) {
					for (k = 0; k < vfeDependencyInfo.count;
					     ++k) {
						if ((dependencyInfo.deltaX[j] ==
						     vfeDependencyInfo.deltaX
						     [k])
						    && (dependencyInfo.deltaY[j]
							==
							vfeDependencyInfo.deltaY
							[k])) {
							HAL_CM_SETBIT
							    (pKernelTSParam->globalDependencyMask,
							     k);
							break;
						}
					}
					if (k == vfeDependencyInfo.count) {
						vfeDependencyInfo.deltaX
						    [vfeDependencyInfo.count] =
						    dependencyInfo.deltaX[j];
						vfeDependencyInfo.deltaY
						    [vfeDependencyInfo.count] =
						    dependencyInfo.deltaY[j];
						HAL_CM_SETBIT
						    (pKernelTSParam->globalDependencyMask,
						     vfeDependencyInfo.count);
						vfeDependencyInfo.count++;
					}
				}
			}
		}

		reuseBBUpdateMask |= pKernelTSParam->reuseBBUpdateMask;
	}

	if (vfeDependencyInfo.count > CM_HAL_MAX_DEPENDENCY_COUNT) {
		CM_ERROR_ASSERT
		    ("Union of kernel dependencies exceeds max dependency count (8)");
		goto finish;
	}

	pScoreboardParams->numMask = (BYTE) vfeDependencyInfo.count;
	for (i = 0; i < pScoreboardParams->numMask; ++i) {
		pScoreboardParams->ScoreboardDelta[i].x =
		    vfeDependencyInfo.deltaX[i];
		pScoreboardParams->ScoreboardDelta[i].y =
		    vfeDependencyInfo.deltaY[i];
	}

	if (pScoreboardParams->numMask == 0) {
		if (pTaskParam->dependencyVectorsValid) {
			pScoreboardParams->numMask =
			    (BYTE) pTaskParam->dependencyVectors.count;
			for (UINT i = 0; i < pScoreboardParams->numMask; ++i) {
				pScoreboardParams->ScoreboardDelta[i].x =
				    pTaskParam->dependencyVectors.deltaX[i];
				pScoreboardParams->ScoreboardDelta[i].y =
				    pTaskParam->dependencyVectors.deltaY[i];
			}
		} else {
			switch (pTaskParam->DependencyPattern) {
			case CM_DEPENDENCY_NONE:
				break;

			case CM_DEPENDENCY_VERTICAL:
				pScoreboardParams->numMask = 1;
				pScoreboardParams->ScoreboardDelta[0].x = 0xF;	// -1 in BYTE:4
				pScoreboardParams->ScoreboardDelta[0].y = 0;
				break;

			case CM_DEPENDENCY_HORIZONTAL:
				pScoreboardParams->numMask = 1;
				pScoreboardParams->ScoreboardDelta[0].x = 0;
				pScoreboardParams->ScoreboardDelta[0].y = 0xF;
				break;

			case CM_DEPENDENCY_WAVEFRONT:
				pScoreboardParams->numMask = 3;
				pScoreboardParams->ScoreboardDelta[0].x = 0xF;
				pScoreboardParams->ScoreboardDelta[0].y = 0;
				pScoreboardParams->ScoreboardDelta[1].x = 0xF;
				pScoreboardParams->ScoreboardDelta[1].y = 0xF;
				pScoreboardParams->ScoreboardDelta[2].x = 0;
				pScoreboardParams->ScoreboardDelta[2].y = 0xF;
				break;

			case CM_DEPENDENCY_WAVEFRONT26:
				pScoreboardParams->numMask = 4;
				pScoreboardParams->ScoreboardDelta[0].x = 0xF;
				pScoreboardParams->ScoreboardDelta[0].y = 0;
				pScoreboardParams->ScoreboardDelta[1].x = 0xF;
				pScoreboardParams->ScoreboardDelta[1].y = 0xF;
				pScoreboardParams->ScoreboardDelta[2].x = 0;
				pScoreboardParams->ScoreboardDelta[2].y = 0xF;
				pScoreboardParams->ScoreboardDelta[3].x = 1;
				pScoreboardParams->ScoreboardDelta[3].y = 0xF;
				break;

			case CM_DEPENDENCY_WAVEFRONT26Z:
				pScoreboardParams->numMask = 5;
				pScoreboardParams->ScoreboardDelta[0].x = 0xF;
				pScoreboardParams->ScoreboardDelta[0].y = 1;
				pScoreboardParams->ScoreboardDelta[1].x = 0xF;
				pScoreboardParams->ScoreboardDelta[1].y = 0;
				pScoreboardParams->ScoreboardDelta[2].x = 0xF;
				pScoreboardParams->ScoreboardDelta[2].y = 0xF;
				pScoreboardParams->ScoreboardDelta[3].x = 0;
				pScoreboardParams->ScoreboardDelta[3].y = 0xF;
				pScoreboardParams->ScoreboardDelta[4].x = 1;
				pScoreboardParams->ScoreboardDelta[4].y = 0xF;
				break;

			case CM_DEPENDENCY_WAVEFRONT26ZI:
				pScoreboardParams->numMask = 7;
				pScoreboardParams->ScoreboardDelta[0].x = 0xF;
				pScoreboardParams->ScoreboardDelta[0].y = 1;
				pScoreboardParams->ScoreboardDelta[1].x = 0xE;
				pScoreboardParams->ScoreboardDelta[1].y = 0;
				pScoreboardParams->ScoreboardDelta[2].x = 0xF;
				pScoreboardParams->ScoreboardDelta[2].y = 0;
				pScoreboardParams->ScoreboardDelta[3].x = 0xF;
				pScoreboardParams->ScoreboardDelta[3].y = 0xF;
				pScoreboardParams->ScoreboardDelta[4].x = 0;
				pScoreboardParams->ScoreboardDelta[4].y = 0xF;
				pScoreboardParams->ScoreboardDelta[5].x = 1;
				pScoreboardParams->ScoreboardDelta[5].y = 0xF;
				pScoreboardParams->ScoreboardDelta[6].x = 1;
				pScoreboardParams->ScoreboardDelta[6].y = 0;
				break;

			default:
				CM_ASSERT(1);
				pTaskParam->DependencyPattern =
				    CM_DEPENDENCY_NONE;
				break;

			}
		}
	}

	pTaskParam->surfacePerBT += CM_RESERVED_SURFACE_NUMBER_FROM_0(pState);

	if ((pExecParam->bGlobalSurfaceUsed
	     && pState->Platform.eRenderCoreFamily <= IGFX_GEN8_CORE)
	    || (pTaskParam->surfacePerBT > CM_MAX_STATIC_SURFACE_STATES_PER_BT)) {
		pTaskParam->surfacePerBT = CM_MAX_STATIC_SURFACE_STATES_PER_BT;
	}

	if (pTaskParam->ppDependencyMasks) {
		for (iKrn = 0; iKrn < pExecParam->iNumKernels; iKrn++) {
			pKernelParam = pExecParam->pKernels[iKrn];
			pDependencyMask = pTaskParam->ppDependencyMasks[iKrn];
			if (pDependencyMask) {
				for (i = 0; i < pKernelParam->iNumThreads; ++i) {
					reuseBBUpdateMask |=
					    pDependencyMask[i].resetMask;
				}
			}
		}
	}

	HAL_CM_CHECKBIT_IS_SET(bitIsSet, reuseBBUpdateMask,
			       CM_NO_BATCH_BUFFER_REUSE_BIT_POS);
	if (bitIsSet || reuseBBUpdateMask == 0) {
		pTaskParam->reuseBBUpdateMask = 0;
	} else {
		pTaskParam->reuseBBUpdateMask = 1;
	}

	threadArgExists =
	    HalCm_GetTaskHasThreadArg(pExecParam->pKernels,
				      pExecParam->iNumKernels);

	if (!pState->WalkerParams.CmWalkerEnable) {
		if (!threadArgExists) {
			if (iTotalThreads > CM_MAX_USER_THREADS_NO_THREADARG) {
				CM_ERROR_ASSERT
				    ("Total task threads '%d' exceeds max allowed threads '%d'",
				     iTotalThreads,
				     CM_MAX_USER_THREADS_NO_THREADARG);
				goto finish;
			}
		} else {
			if (iTotalThreads > CM_MAX_USER_THREADS) {
				CM_ERROR_ASSERT
				    ("Total task threads '%d' exceeds max allowed threads '%d'",
				     iTotalThreads, CM_MAX_USER_THREADS);
				goto finish;
			}
		}
	}

 finish:
	return hr;
}

GENOS_STATUS HalCm_ParseGroupTask(PCM_HAL_STATE pState,
				  PCM_HAL_EXEC_GROUP_TASK_PARAM pExecGroupParam)
{
	PCM_HAL_TASK_PARAM pTaskParam = pState->pTaskParam;
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	PCM_HAL_KERNEL_PARAM pKernelParam = NULL;
	UINT uSurfaceNumber;
	UINT uSurfaceIndex;
	UINT iKrn;
	UINT i;

	pTaskParam->SurEntryInfoArrays = pExecGroupParam->SurEntryInfoArrays;
	pTaskParam->iBatchBufferSize = 0;

	pTaskParam->uiNumKernels = pExecGroupParam->iNumKernels;
	pTaskParam->uiSyncBitmap = pExecGroupParam->uiSyncBitmap;

	for (iKrn = 0; iKrn < pExecGroupParam->iNumKernels; iKrn++) {
		pKernelParam = pExecGroupParam->pKernels[iKrn];
		PCM_INDIRECT_SURFACE_INFO pIndirectSurfaceInfo =
		    pKernelParam->CmIndirectDataParam.pSurfaceInfo;
		uSurfaceNumber = 0;
		if (pKernelParam->CmIndirectDataParam.iSurfaceCount) {
			uSurfaceIndex = 0;
			for (i = 0;
			     i <
			     pKernelParam->CmIndirectDataParam.iSurfaceCount;
			     i++) {
				uSurfaceIndex =
				    (pIndirectSurfaceInfo +
				     i)->iBindingTableIndex >
				    uSurfaceIndex ? (pIndirectSurfaceInfo +
						     i)->iBindingTableIndex :
				    uSurfaceIndex;
				uSurfaceNumber++;
			}
			pTaskParam->surfacePerBT =
			    pTaskParam->surfacePerBT >
			    uSurfaceIndex ? pTaskParam->surfacePerBT :
			    uSurfaceIndex;
		}

		uSurfaceNumber += pKernelParam->iNumSurfaces;

		pTaskParam->surfacePerBT =
		    pTaskParam->surfacePerBT <
		    uSurfaceNumber ? uSurfaceNumber : pTaskParam->surfacePerBT;
	}

	pTaskParam->surfacePerBT += CM_RESERVED_SURFACE_NUMBER_FROM_0(pState);

	if ((pExecGroupParam->bGlobalSurfaceUsed
	     && pState->Platform.eRenderCoreFamily <= IGFX_GEN8_CORE)
	    || (pTaskParam->surfacePerBT > CM_MAX_STATIC_SURFACE_STATES_PER_BT)) {
		pTaskParam->surfacePerBT = CM_MAX_STATIC_SURFACE_STATES_PER_BT;
	}

	pTaskParam->iPreemptionMode = pExecGroupParam->iPreemptionMode;

	return hr;
}

GENOS_STATUS HalCm_ParseHintsTask(PCM_HAL_STATE pState,
				  PCM_HAL_EXEC_HINTS_TASK_PARAM pExecHintsParam)
{
	GENOS_STATUS hr;
	PCM_HAL_TASK_PARAM pTaskParam;
	PCM_HAL_KERNEL_PARAM pKernelParam;
	UINT iHdrSize;
	UINT iTotalThreads;
	UINT iKrn;
	PGENHW_SCOREBOARD_PARAMS pScoreboardParams;
	bool bNonstallingScoreboardEnable;
	BOOLEAN bitIsSet;
	BYTE reuseBBUpdateMask;
	BOOLEAN threadArgExists;

	hr = GENOS_STATUS_SUCCESS;
	iKrn = 0;
	pTaskParam = pState->pTaskParam;
	bNonstallingScoreboardEnable = TRUE;
	bitIsSet = FALSE;
	iTotalThreads = 0;
	reuseBBUpdateMask = 0;
	threadArgExists = FALSE;

	iHdrSize =
	    pState->pHwInterface->pHwCommands->dwMediaObjectHeaderCmdSize;
	pScoreboardParams = &pState->ScoreboardParams;

	for (iKrn = 0; iKrn < pExecHintsParam->iNumKernels; ++iKrn) {
		if ((pExecHintsParam->pKernels[iKrn] == NULL) ||
		    (pExecHintsParam->piKernelSizes[iKrn] == 0)) {
			CM_ERROR_ASSERT("Invalid Kernel data");
			goto finish;
		}
		pKernelParam = pExecHintsParam->pKernels[iKrn];

		bNonstallingScoreboardEnable &=
		    (pKernelParam->dwCmFlags &
		     CM_KERNEL_FLAGS_NONSTALLING_SCOREBOARD) ? TRUE : FALSE;

		if (!pState->WalkerParams.CmWalkerEnable) {
			pTaskParam->iBatchBufferSize +=
			    pKernelParam->iNumThreads * (iHdrSize +
							 MAX
							 (pKernelParam->iPayloadSize,
							  4));
		}

		iTotalThreads += pKernelParam->iNumThreads;

		reuseBBUpdateMask |=
		    pKernelParam->CmKernelThreadSpaceParam.reuseBBUpdateMask;
	}

	HAL_CM_CHECKBIT_IS_SET(bitIsSet, reuseBBUpdateMask,
			       CM_NO_BATCH_BUFFER_REUSE_BIT_POS);
	if (bitIsSet || reuseBBUpdateMask == 0) {
		pTaskParam->reuseBBUpdateMask = 0;
	} else {
		pTaskParam->reuseBBUpdateMask = 1;
	}

	pTaskParam->iBatchBufferSize += CM_EXTRA_BB_SPACE;

	pScoreboardParams->ScoreboardType = bNonstallingScoreboardEnable;

	threadArgExists =
	    HalCm_GetTaskHasThreadArg(pExecHintsParam->pKernels,
				      pExecHintsParam->iNumKernels);

	if (!pState->WalkerParams.CmWalkerEnable) {
		if (!threadArgExists) {
			if (iTotalThreads > CM_MAX_USER_THREADS_NO_THREADARG) {
				CM_ERROR_ASSERT
				    ("Total task threads '%d' exceeds max allowed threads '%d'",
				     iTotalThreads,
				     CM_MAX_USER_THREADS_NO_THREADARG);
				goto finish;
			}
		} else {
			if (iTotalThreads > CM_MAX_USER_THREADS) {
				CM_ERROR_ASSERT
				    ("Total task threads '%d' exceeds max allowed threads '%d'",
				     iTotalThreads, CM_MAX_USER_THREADS);
				goto finish;
			}
		}
	}

 finish:
	return hr;
}

BOOL bIsFree(PGENHW_KRN_ALLOCATION pKAlloc)
{
	if (pKAlloc == NULL) {
		return FALSE;
	} else {
		if (pKAlloc->dwFlags != GENHW_KERNEL_ALLOCATION_FREE) {
			return FALSE;
		}
	}

	return TRUE;
}

void CmLoadKernel(PGENHW_GSH pGSH,
		  PGENHW_KRN_ALLOCATION pKernelAllocation,
		  DWORD dwSync,
		  DWORD dwCount,
		  PGENHW_KERNEL_PARAM pParameters,
		  PCM_HAL_KERNEL_PARAM pKernelParam, Kdll_CacheEntry * pKernel)
{
	if (pKernel) {
		pKernelAllocation->iKID = -1;
		pKernelAllocation->iKUID = pKernel->iKUID;
		pKernelAllocation->iKCID = pKernel->iKCID;
		pKernelAllocation->dwSync = dwSync;
		pKernelAllocation->dwCount = dwCount & 0xFFFFFFFF;
		pKernelAllocation->dwFlags = GENHW_KERNEL_ALLOCATION_USED;
		pKernelAllocation->Params = *pParameters;
		pKernelAllocation->pKernel = pKernel;
		GENOS_SecureMemcpy(pGSH->pGSH + pKernelAllocation->dwOffset,
				   pKernelParam->iMovInsDataSize,
				   pKernelParam->pMovInsData,
				   pKernelParam->iMovInsDataSize);

		GENOS_SecureMemcpy(pGSH->pGSH + pKernelAllocation->dwOffset +
				   pKernelParam->iMovInsDataSize,
				   pKernelParam->iKernelBinarySize -
				   pKernelParam->iMovInsDataSize,
				   pKernelParam->pKernelBinary,
				   pKernelParam->iKernelBinarySize -
				   pKernelParam->iMovInsDataSize);
	} else {
		pKernelAllocation->iKID = -1;
		pKernelAllocation->iKUID = -1;
		pKernelAllocation->iKCID = -1;
		pKernelAllocation->dwSync = 0;
		pKernelAllocation->dwCount = 0;
		pKernelAllocation->dwFlags = GENHW_KERNEL_ALLOCATION_FREE;
		pKernelAllocation->pKernel = NULL;
	}
}

INT CmSearchFreeSlotSize(PCM_HAL_STATE pState, Kdll_CacheEntry * pKernel)
{
	PGENHW_GSH pGSH;
	PGENHW_KRN_ALLOCATION pKernelAllocation;
	PGENHW_HW_INTERFACE pHwInterface;
	INT iKernelAllocationID;
	INT iReturnVal = -1;

	pHwInterface = pState->pHwInterface;
	pGSH = pHwInterface->pGeneralStateHeap;
	pKernelAllocation = pGSH->pKernelAllocation;

	for (iKernelAllocationID = 0;
	     iKernelAllocationID < pState->nNumKernelsInGSH;
	     iKernelAllocationID++, pKernelAllocation++) {
		if (pKernelAllocation->dwFlags == GENHW_KERNEL_ALLOCATION_FREE) {
			if (pState->pTotalKernelSize[iKernelAllocationID] >=
			    pKernel->iSize) {
				return iKernelAllocationID;
			}
		}
	}

	return iReturnVal;
}

INT CmAddCurrentKerenelToFreeSlot(PCM_HAL_STATE pState,
				  INT slot,
				  PGENHW_KERNEL_PARAM pParameters,
				  PCM_HAL_KERNEL_PARAM pKernelParam,
				  Kdll_CacheEntry * pKernel)
{
	PGENHW_GSH pGSH;
	PGENHW_KRN_ALLOCATION pKernelAllocation, pKernelAllocationN;
	PGENHW_HW_INTERFACE pHwInterface;

	INT i;
	INT totalSize, tmpSize, dwOffset;
	BOOL bAdjust;

	pHwInterface = pState->pHwInterface;
	pGSH = pHwInterface->pGeneralStateHeap;
	pKernelAllocation = pGSH->pKernelAllocation;

	if (pGSH->pKernelAllocation[slot].iSize == pKernel->iSize) {
		bAdjust = FALSE;
	} else {
		bAdjust = TRUE;
	}

	if ((pState->nNumKernelsInGSH < CM_MAX_GSH_KERNEL_ENTRIES) && bAdjust) {
		INT lastKernel = pState->nNumKernelsInGSH - 1;
		for (i = lastKernel; i > slot; i--) {
			pKernelAllocation = &pGSH->pKernelAllocation[i];
			pKernelAllocationN = &pGSH->pKernelAllocation[i + 1];
			*pKernelAllocationN = *pKernelAllocation;
			pState->pTotalKernelSize[i + 1] =
			    pState->pTotalKernelSize[i];
		}

		totalSize = pState->pTotalKernelSize[slot];
		tmpSize = pKernel->iSize;

		if (pGSH->pKernelAllocation[slot].dwOffset == 0) {
			pGSH->pKernelAllocation[slot].dwOffset =
			    pGSH->dwKernelBase;
		}
		dwOffset = pGSH->pKernelAllocation[slot].dwOffset;

		pKernelAllocation = &pGSH->pKernelAllocation[slot];
		CmLoadKernel(pGSH, pKernelAllocation, pGSH->dwNextTag,
			     pGSH->dwAccessCounter, pParameters, pKernelParam,
			     pKernel);
		pGSH->dwAccessCounter++;
		pKernelAllocation->dwOffset = dwOffset;
		pKernelAllocation->iSize = tmpSize;
		pState->pTotalKernelSize[slot] = GENOS_ALIGN_CEIL(tmpSize, 64);

		tmpSize = GENOS_ALIGN_CEIL(tmpSize, 64);
		pKernelAllocation = &pGSH->pKernelAllocation[slot + 1];
		CmLoadKernel(pGSH, pKernelAllocation, 0, 0, pParameters,
			     pKernelParam, NULL);
		pKernelAllocation->dwOffset = dwOffset + tmpSize;
		pKernelAllocation->iSize = 0;
		pState->pTotalKernelSize[slot + 1] = totalSize - tmpSize;

		pState->nNumKernelsInGSH++;
	} else if (pState->nNumKernelsInGSH < CM_MAX_GSH_KERNEL_ENTRIES) {
		pKernelAllocation = &pGSH->pKernelAllocation[slot];
		if (pKernelAllocation->dwOffset == 0) {
			pKernelAllocation->dwOffset = pGSH->dwKernelBase;
		}
		CmLoadKernel(pGSH, pKernelAllocation, pGSH->dwNextTag,
			     pGSH->dwAccessCounter, pParameters, pKernelParam,
			     pKernel);
		pGSH->dwAccessCounter++;
		pKernelAllocation->iSize = pKernel->iSize;
		pState->pTotalKernelSize[slot] =
		    GENOS_ALIGN_CEIL(pKernel->iSize, 64);
	} else {
		pKernelAllocation = &pGSH->pKernelAllocation[slot];
		CmLoadKernel(pGSH, pKernelAllocation, pGSH->dwNextTag,
			     pGSH->dwAccessCounter, pParameters, pKernelParam,
			     pKernel);
		pGSH->dwAccessCounter++;
		pKernelAllocation->iSize = pKernel->iSize;
	}

	return S_OK;
}

HRESULT HalCm_UnloadKernel(PGENHW_HW_INTERFACE pHwInterface,
			   INT iKernelAllocationID)
{
	PGENHW_GSH pGSH;
	PGENHW_KRN_ALLOCATION pKernelAllocation;
	HRESULT hr;

	CMCHK_NULL(pHwInterface);
	CMCHK_NULL(pHwInterface->pGeneralStateHeap);
	CM_ASSERT(iKernelAllocationID >= 0);

	hr = E_FAIL;
	pGSH = pHwInterface->pGeneralStateHeap;

	CMCHK_NULL(pGSH->pKernelAllocation);
	CM_ASSERT(iKernelAllocationID < pHwInterface->GshSettings.iKernelCount);

	pKernelAllocation = &(pGSH->pKernelAllocation[iKernelAllocationID]);

	if (pKernelAllocation->dwFlags == GENHW_KERNEL_ALLOCATION_FREE) {
		goto finish;
	}
	CHK_HRESULT(pHwInterface->pfnRefreshSync(pHwInterface));

	if ((INT) (pGSH->dwSyncTag - pKernelAllocation->dwSync) < 0) {
		goto finish;
	}
	if (pKernelAllocation->pKernel) {
		pKernelAllocation->pKernel->dwLoaded = 0;
	}
	pKernelAllocation->iKID = -1;
	pKernelAllocation->iKUID = -1;
	pKernelAllocation->iKCID = -1;
	pKernelAllocation->dwSync = 0;
	pKernelAllocation->dwFlags = GENHW_KERNEL_ALLOCATION_FREE;
	pKernelAllocation->dwCount = 0;
	pKernelAllocation->pKernel = NULL;

	hr = S_OK;

 finish:
	return hr;
}

VOID HalCm_TouchKernel(PGENHW_HW_INTERFACE pHwInterface,
		       INT iKernelAllocationID)
{
	PGENHW_GSH pGSH;
	PGENHW_KRN_ALLOCATION pKernelAllocation;

	pGSH = (pHwInterface) ? pHwInterface->pGeneralStateHeap : NULL;
	if (pGSH == NULL ||
	    pGSH->pKernelAllocation == NULL ||
	    iKernelAllocationID < 0 ||
	    iKernelAllocationID >= pHwInterface->GshSettings.iKernelCount) {
		return;
	}
	pKernelAllocation = &(pGSH->pKernelAllocation[iKernelAllocationID]);
	if (pKernelAllocation->dwFlags != GENHW_KERNEL_ALLOCATION_FREE &&
	    pKernelAllocation->dwFlags != GENHW_KERNEL_ALLOCATION_LOCKED) {
		pKernelAllocation->dwCount = pGSH->dwAccessCounter++;
	}
	pKernelAllocation->dwSync = pGSH->dwNextTag;
}

HRESULT CmDeleteOldestKernel(PCM_HAL_STATE pState, Kdll_CacheEntry * pKernel)
{
	PGENHW_KRN_ALLOCATION pKernelAllocation;
	PGENHW_HW_INTERFACE pHwInterface = pState->pHwInterface;;
	PGENHW_GSH pGSH = pHwInterface->pGeneralStateHeap;

	DWORD dwOldest = 0;
	DWORD dwLastUsed;
	INT iKernelAllocationID, iSearchIndex = -1;
	INT alignedSize, shiftOffset;

	pKernelAllocation = pGSH->pKernelAllocation;

	pKernelAllocation = pGSH->pKernelAllocation;
	for (iKernelAllocationID = 0;
	     iKernelAllocationID < pState->nNumKernelsInGSH;
	     iKernelAllocationID++, pKernelAllocation++) {
		if (pKernelAllocation->dwFlags == GENHW_KERNEL_ALLOCATION_FREE
		    || pKernelAllocation->dwFlags ==
		    GENHW_KERNEL_ALLOCATION_LOCKED) {
			continue;
		}
		if ((INT) (pGSH->dwSyncTag - pKernelAllocation->dwSync) < 0) {
			continue;
		}
		dwLastUsed =
		    (DWORD) (pGSH->dwAccessCounter -
			     pKernelAllocation->dwCount);
		if (dwLastUsed > dwOldest) {
			iSearchIndex = iKernelAllocationID;
			dwOldest = dwLastUsed;
		}
	}

	if (iSearchIndex < 0) {
		CM_ASSERTMESSAGE
		    ("Failed to delete any slot from GSH. It is impossible.");
		iKernelAllocationID = GENHW_KERNEL_LOAD_FAIL;
		return E_FAIL;
	}
	if (HalCm_UnloadKernel(pHwInterface, iSearchIndex) != S_OK) {
		CM_ASSERTMESSAGE
		    ("Failed to load kernel - no space available in GSH.");
		iKernelAllocationID = GENHW_KERNEL_LOAD_FAIL;
		return E_FAIL;
	}
	INT index = iSearchIndex;
	PGENHW_KRN_ALLOCATION pKAlloc0, pKAlloc2;
	pKAlloc0 = (index == 0) ? NULL : &pGSH->pKernelAllocation[index - 1];
	pKAlloc2 =
	    (index ==
	     CM_MAX_GSH_KERNEL_ENTRIES -
	     1) ? NULL : &pGSH->pKernelAllocation[index + 1];

	if (bIsFree(pKAlloc0) && bIsFree(pKAlloc2)) {
		pGSH->pKernelAllocation[index - 1].dwFlags =
		    GENHW_KERNEL_ALLOCATION_FREE;
		pState->pTotalKernelSize[index - 1] +=
		    pState->pTotalKernelSize[index] +
		    pState->pTotalKernelSize[index + 1];
		pGSH->pKernelAllocation[index - 1].iSize = 0;

		for (INT i = index + 2; i < pState->nNumKernelsInGSH; i++) {
			pGSH->pKernelAllocation[i - 2] =
			    pGSH->pKernelAllocation[i];
			pState->pTotalKernelSize[i - 2] =
			    pState->pTotalKernelSize[i];
		}
		pState->nNumKernelsInGSH -= 2;
	} else if (bIsFree(pKAlloc0)) {
		pGSH->pKernelAllocation[index - 1].dwFlags =
		    GENHW_KERNEL_ALLOCATION_FREE;
		pState->pTotalKernelSize[index - 1] +=
		    pState->pTotalKernelSize[index];
		pGSH->pKernelAllocation[index - 1].iSize = 0;

		for (INT i = index + 1; i < pState->nNumKernelsInGSH; i++) {
			pGSH->pKernelAllocation[i - 1] =
			    pGSH->pKernelAllocation[i];
			pState->pTotalKernelSize[i - 1] =
			    pState->pTotalKernelSize[i];
		}
		pState->nNumKernelsInGSH -= 1;
	} else if (bIsFree(pKAlloc2)) {
		pGSH->pKernelAllocation[index].dwFlags =
		    GENHW_KERNEL_ALLOCATION_FREE;
		pState->pTotalKernelSize[index] +=
		    pState->pTotalKernelSize[index + 1];
		pGSH->pKernelAllocation[index].iSize = 0;
		if (pKAlloc0) {
			alignedSize = GENOS_ALIGN_CEIL(pKAlloc0->iSize, 64);
			shiftOffset =
			    pState->pTotalKernelSize[index - 1] - alignedSize;

			pState->pTotalKernelSize[index - 1] -= shiftOffset;
			pState->pTotalKernelSize[index] += shiftOffset;
			pGSH->pKernelAllocation[index].dwOffset -= shiftOffset;
		}

		for (INT i = index + 1; i < pState->nNumKernelsInGSH; i++) {
			pGSH->pKernelAllocation[i] =
			    pGSH->pKernelAllocation[i + 1];
			pState->pTotalKernelSize[i] =
			    pState->pTotalKernelSize[i + 1];
		}
		pState->nNumKernelsInGSH -= 1;
	} else {
		pGSH->pKernelAllocation[index].dwFlags =
		    GENHW_KERNEL_ALLOCATION_FREE;
		pGSH->pKernelAllocation[index].iSize = 0;
		if (pKAlloc0) {
			alignedSize = GENOS_ALIGN_CEIL(pKAlloc0->iSize, 64);
			shiftOffset =
			    pState->pTotalKernelSize[index - 1] - alignedSize;
			pState->pTotalKernelSize[index - 1] -= shiftOffset;
			pState->pTotalKernelSize[index] += shiftOffset;
			pGSH->pKernelAllocation[index].dwOffset -= shiftOffset;
		}
	}

	return S_OK;
}

HRESULT HalCm_LoadKernel(PCM_HAL_STATE pState,
			 PCM_HAL_KERNEL_PARAM pKernelParam, PINT piKAID)
{
	PGENHW_GSH pGSH;
	PGENHW_KRN_ALLOCATION pKernelAllocation;
	PGENHW_HW_INTERFACE pHwInterface;
	PGENHW_KERNEL_PARAM pParameters;
	Kdll_CacheEntry *pKernel;

	INT iKernelAllocationID;
	INT iKernelCacheID;
	INT iKernelUniqueID;
	INT iFreeSlot;

	pHwInterface = pState->pHwInterface;
	pGSH = (pHwInterface) ? pHwInterface->pGeneralStateHeap : NULL;
	iKernelAllocationID = GENHW_KERNEL_LOAD_FAIL;
	pKernel = &(pState->KernelSetup.CacheEntry);
	pParameters = &(pState->KernelSetup.Param);

	if (pGSH == NULL ||
	    pGSH->bGSHLocked == FALSE ||
	    pGSH->pKernelAllocation == NULL ||
	    pParameters == NULL ||
	    pKernel == NULL ||
	    pKernelParam->iKernelBinarySize == 0 ||
	    pState->nNumKernelsInGSH > CM_MAX_GSH_KERNEL_ENTRIES) {
		CM_ASSERTMESSAGE("Failed to load kernel - invalid parameters.");
		return E_FAIL;
	}

	pKernel->iKUID = static_cast < int >((pKernelParam->uiKernelId >> 32));
	pKernel->iKCID = -1;
	pKernel->pBinary = pKernelParam->pKernelBinary;
	pKernel->iSize = pKernelParam->iKernelBinarySize;

	iKernelUniqueID = pKernel->iKUID;
	iKernelCacheID = pKernel->iKCID;

	pKernelAllocation = pGSH->pKernelAllocation;
	for (iKernelAllocationID = 0;
	     iKernelAllocationID < pState->nNumKernelsInGSH;
	     iKernelAllocationID++, pKernelAllocation++) {
		if (pKernelAllocation->iKUID == iKernelUniqueID &&
		    pKernelAllocation->iKCID == iKernelCacheID) {
			HalCm_TouchKernel(pHwInterface, iKernelAllocationID);
			pKernel->dwLoaded = 1;
			*piKAID = iKernelAllocationID;

			return S_OK;
		}
	}

	do {
		iFreeSlot = CmSearchFreeSlotSize(pState, pKernel);
		if (iFreeSlot >= 0) {
			CmAddCurrentKerenelToFreeSlot(pState, iFreeSlot,
						      pParameters, pKernelParam,
						      pKernel);
			break;
		} else {
			if (CmDeleteOldestKernel(pState, pKernel) ==
			    (HRESULT) E_FAIL) {
				return E_FAIL;
			}
		}
	} while (1);

	pKernel->dwLoaded = 1;
	*piKAID = iFreeSlot;

	return S_OK;
}

INT HalCm_AllocateMediaID(PGENHW_HW_INTERFACE pHwInterface,
			  INT iKernelAllocationID,
			  INT iBindingTableID,
			  INT iCurbeOffset,
			  INT iCurbeLength,
			  INT iCrsThrdConstDataLn,
			  PGENHW_GPGPU_WALKER_PARAMS pGpGpuWalkerParams)
{
	PGENHW_GSH pGSH;
	PGENHW_KRN_ALLOCATION pKernelAllocation;
	PGENHW_MEDIA_STATE pCurMediaState;
	PINT Allocation;
	INT iCurbeSize;
	INT iInterfaceDescriptor;
	GENHW_INTERFACE_DESCRIPTOR_PARAMS InterfaceDescriptorParams;

	iInterfaceDescriptor = -1;

	pGSH = (pHwInterface) ? pHwInterface->pGeneralStateHeap : NULL;
	if (pGSH == NULL ||
	    pGSH->pKernelAllocation == NULL || pGSH->bGSHLocked == FALSE) {
		CM_ASSERTMESSAGE("Invalid GSH State.");
		goto finish;
	}
	pCurMediaState = pGSH->pCurMediaState;
	if (pCurMediaState == NULL || pCurMediaState->piAllocation == NULL) {
		CM_ASSERTMESSAGE("Invalid Media State.");
		goto finish;
	}
	if (iKernelAllocationID < 0 ||
	    iKernelAllocationID >= pHwInterface->GshSettings.iKernelCount) {
		CM_ASSERTMESSAGE("Invalid Kernel Allocation ID.");
		goto finish;
	}
	pKernelAllocation = &(pGSH->pKernelAllocation[iKernelAllocationID]);
	if (pKernelAllocation->dwFlags == GENHW_KERNEL_ALLOCATION_FREE ||
	    pKernelAllocation->iSize == 0) {
		CM_ASSERTMESSAGE("Invalid Kernel Allocation.");
		goto finish;
	}
	iCurbeSize = iCurbeLength;
	if (iCurbeSize <= 0) {
		iCurbeSize = iCurbeOffset = 0;
	} else if (iCurbeOffset < 0 ||
		   (iCurbeOffset & 0x1F) != 0 ||
		   (iCurbeOffset + iCurbeSize) > pCurMediaState->iCurbeOffset) {
		CM_ASSERTMESSAGE("Invalid Curbe Allocation.");
		goto finish;
	}
	Allocation = pCurMediaState->piAllocation;
	iInterfaceDescriptor = pKernelAllocation->iKID;
	if (iInterfaceDescriptor >= 0 &&
	    Allocation[iInterfaceDescriptor] >= 0 &&
	    Allocation[iInterfaceDescriptor] != iKernelAllocationID) {
		iInterfaceDescriptor = -1;
	}
	if (iInterfaceDescriptor < 0) {
		INT iMax = pHwInterface->GshSettings.iMediaIDs;
		for (iInterfaceDescriptor = 0;
		     iInterfaceDescriptor < iMax; iInterfaceDescriptor++) {
			if (Allocation[iInterfaceDescriptor] < 0) {
				break;
			}
		}

		if (iInterfaceDescriptor >= iMax) {
			CM_ASSERTMESSAGE("No Interface Descriptor available.");
			iInterfaceDescriptor = -1;
			goto finish;
		}
	}

	InterfaceDescriptorParams.iMediaID = iInterfaceDescriptor;
	InterfaceDescriptorParams.iBindingTableID = iBindingTableID;
	InterfaceDescriptorParams.iCurbeOffset = iCurbeOffset;
	InterfaceDescriptorParams.iCurbeLength = iCurbeLength;
	InterfaceDescriptorParams.iCrsThrdConstDataLn = iCrsThrdConstDataLn;

	pHwInterface->pfnSetupInterfaceDescriptor(pHwInterface,
						  pCurMediaState,
						  pKernelAllocation,
						  &InterfaceDescriptorParams,
						  pGpGpuWalkerParams);

	Allocation[iInterfaceDescriptor] = iKernelAllocationID;

	if (pKernelAllocation->iKID < 0) {
		pKernelAllocation->iKID = iInterfaceDescriptor;
	}

 finish:
	return iInterfaceDescriptor;
}

GENOS_STATUS HalCm_SetupBufferSurfaceState(PCM_HAL_STATE pState,
					   PCM_HAL_KERNEL_ARG_PARAM pArgParam,
					   PCM_HAL_INDEX_PARAM pIndexParam,
					   INT iBindingTable,
					   SHORT globalSurface,
					   UINT iThreadIndex, PBYTE pBuffer)
{
	GENOS_STATUS hr;
	GENHW_SURFACE Surface;
	GENHW_SURFACE_STATE_PARAMS SurfaceParam;
	PGENHW_HW_INTERFACE pHwInterface;
	PGENHW_SURFACE_STATE_ENTRY pSurfaceEntry;
	PBYTE pSrc;
	PBYTE pDst;
	UINT iIndex;
	UINT iBTIndex;
	WORD memObjCtl;

	hr = GENOS_STATUS_UNKNOWN;
	pHwInterface = pState->pHwInterface;
	PCM_HAL_TASK_PARAM pTaskParam = pState->pTaskParam;

	CM_ASSERT(pArgParam->iUnitSize == sizeof(iIndex));

	pSrc = pArgParam->pFirstValue + (iThreadIndex * pArgParam->iUnitSize);
	iIndex = *((PUINT) pSrc) & CM_SURFACE_MASK;
	if (iIndex == CM_NULL_SURFACE) {
		if (pBuffer) {
			pDst = pBuffer + pArgParam->iPayloadOffset;
			*((PUINT) pDst) = CM_NULL_SURFACE_BINDING_INDEX;
		}

		hr = GENOS_STATUS_SUCCESS;
		goto finish;
	}

	memObjCtl =
	    (WORD) ((*((PUINT) pSrc) & CM_MEMORY_OBJECT_CONTROL_MASK) >> 16);
	if (!memObjCtl) {
		memObjCtl = CM_DEFAULT_CACHE_TYPE;
	}
	if (iIndex >= pState->CmDeviceParam.iMaxBufferTableSize ||
	    (pState->pBufferTable[iIndex].iSize == 0)) {
		CM_ERROR_ASSERT("Invalid Buffer Surface array index '%d'",
				iIndex);
		goto finish;
	}
	iBTIndex = pState->pBTBufferIndexTable[iIndex].RegularSurfIndex;
	if (iBTIndex == (unsigned char)CM_INVALID_INDEX) {
		if (globalSurface < 0) {
			iBTIndex =
			    Halcm_GetFreeBindingIndex(pState, pIndexParam, 1);
		} else {
			iBTIndex =
			    globalSurface +
			    CM_BINDING_START_INDEX_OF_GLOBAL_SURFACE(pState);
			if ((INT) iBTIndex >=
			    (CM_BINDING_START_INDEX_OF_GLOBAL_SURFACE(pState) +
			     CM_MAX_GLOBAL_SURFACE_NUMBER)) {
				CM_ERROR_ASSERT
				    ("Exceeded Max Global Surfaces '%d'",
				     iBTIndex);
				goto finish;
			}
		}
		CM_CHK_GENOSSTATUS(HalCm_GetSurfaceAndRegister
				   (pState, &Surface, CM_ARGUMENT_SURFACEBUFFER,
				    iIndex));

		GENOS_ZeroMemory(&SurfaceParam, sizeof(SurfaceParam));

		pState->pfnSetSurfaceMemoryObjectControl(pState,
							 memObjCtl,
							 &SurfaceParam);
		CM_CHK_GENOSSTATUS(pHwInterface->pfnSetupBufferSurfaceState
				   (pHwInterface, &Surface, &SurfaceParam,
				    &pSurfaceEntry));

		CM_ASSERT(((INT) iBTIndex) <
			  pHwInterface->SshSettings.iSurfacesPerBT);
		CM_CHK_GENOSSTATUS(pHwInterface->pfnBindSurfaceState
				   (pHwInterface, iBindingTable, iBTIndex,
				    pSurfaceEntry));

		if ((pTaskParam->SurEntryInfoArrays.dwKrnNum != 0) &&
		    (pTaskParam->SurEntryInfoArrays.pSurfEntryInfosArray !=
		     NULL)) {
			UINT dummy = 0;
			CM_CHK_GENOSSTATUS(HalCm_GetSurfaceDetails(pState,
								   pIndexParam,
								   iBTIndex,
								   Surface,
								   globalSurface,
								   NULL,
								   dummy,
								   SurfaceParam,
								   CM_ARGUMENT_SURFACEBUFFER));
		}
		pState->pBTBufferIndexTable[iIndex].RegularSurfIndex = iBTIndex;
	}
	if (pBuffer) {
		pDst = pBuffer + pArgParam->iPayloadOffset;
		*((PUINT) pDst) = iBTIndex;
	}
	hr = GENOS_STATUS_SUCCESS;

 finish:
	return hr;
}

GENOS_STATUS HalCm_Setup2DSurfaceStateBasic(PCM_HAL_STATE pState,
					    PCM_HAL_KERNEL_ARG_PARAM pArgParam,
					    PCM_HAL_INDEX_PARAM pIndexParam,
					    INT iBindingTable,
					    UINT iThreadIndex,
					    PBYTE pBuffer, BOOL multipleBinding)
{
	GENOS_STATUS hr;
	GENHW_SURFACE Surface;
	GENHW_SURFACE_STATE_PARAMS SurfaceParam;
	PGENHW_HW_INTERFACE pHwInterface;
	PGENHW_SURFACE_STATE_ENTRY pSurfaceEntries[GENHW_MAX_SURFACE_PLANES];
	PBYTE pSrc;
	PBYTE pDst;
	INT iSurfaceEntries = 0;
	UINT iIndex;
	UINT iBTIndex = 0;
	UINT i;
	UINT dwTempPlaneIndex = 0;
	WORD memObjCtl = CM_DEFAULT_CACHE_TYPE;

	hr = GENOS_STATUS_UNKNOWN;
	pHwInterface = pState->pHwInterface;
	iSurfaceEntries = 0;

	PCM_HAL_TASK_PARAM pTaskParam = pState->pTaskParam;

	CM_ASSERT(pArgParam->iUnitSize == sizeof(iIndex));
	pSrc = pArgParam->pFirstValue + (iThreadIndex * pArgParam->iUnitSize);
	iIndex = *((PUINT) pSrc) & CM_SURFACE_MASK;
	if (iIndex == CM_NULL_SURFACE) {
		if (pBuffer) {
			pDst = pBuffer + pArgParam->iPayloadOffset;
			*((PUINT) pDst) = CM_NULL_SURFACE_BINDING_INDEX;
		}

		hr = GENOS_STATUS_SUCCESS;
		goto finish;
	}

	memObjCtl =
	    (WORD) ((*((PUINT) pSrc) & CM_MEMORY_OBJECT_CONTROL_MASK) >> 16);
	if (!memObjCtl) {
		memObjCtl = CM_DEFAULT_CACHE_TYPE;
	}
	if (iIndex >= pState->CmDeviceParam.iMax2DSurfaceTableSize ||
	    IntelGen_OsResourceIsNull(&pState->
				      pUmdSurf2DTable[iIndex].OsResource)) {
		CM_ERROR_ASSERT("Invalid 2D Surface array index '%d'", iIndex);
		goto finish;
	}
	unsigned char nBTIRegularSurf;
	nBTIRegularSurf = pState->pBT2DIndexTable[iIndex].RegularSurfIndex;
	if (nBTIRegularSurf != (unsigned char)CM_INVALID_INDEX) {
		iBTIndex = nBTIRegularSurf;

		if (pBuffer) {
			pDst = pBuffer + pArgParam->iPayloadOffset;
			*((PUINT) pDst) = iBTIndex;
		}

		hr = GENOS_STATUS_SUCCESS;
		goto finish;
	}

	CM_CHK_GENOSSTATUS(HalCm_GetSurfaceAndRegister
			   (pState, &Surface, CM_ARGUMENT_SURFACE2D, iIndex));

	GENOS_ZeroMemory(&SurfaceParam, sizeof(SurfaceParam));
	SurfaceParam.Type = pHwInterface->SurfaceTypeDefault;
	SurfaceParam.bRenderTarget = FALSE;
	SurfaceParam.bWidthInDword_UV = TRUE;
	SurfaceParam.bWidthInDword_Y = TRUE;

	Surface.dwWidth = pState->pUmdSurf2DTable[iIndex].iSurfaceStateWidth;
	Surface.dwHeight = pState->pUmdSurf2DTable[iIndex].iSurfaceStateHeight;

	pState->pfnSetSurfaceMemoryObjectControl(pState,
						 memObjCtl, &SurfaceParam);

	CM_CHK_GENOSSTATUS(pHwInterface->pfnSetupSurfaceState(pHwInterface,
							      &Surface,
							      &SurfaceParam,
							      &iSurfaceEntries,
							      pSurfaceEntries));

	iBTIndex =
	    Halcm_GetFreeBindingIndex(pState, pIndexParam, iSurfaceEntries);
	for (i = 0; i < (UINT) iSurfaceEntries; i++) {
		CM_CHK_GENOSSTATUS(pHwInterface->pfnBindSurfaceState
				   (pHwInterface, iBindingTable, iBTIndex + i,
				    pSurfaceEntries[i]));
		if ((pTaskParam->SurEntryInfoArrays.dwKrnNum != 0)
		    && (pTaskParam->SurEntryInfoArrays.pSurfEntryInfosArray !=
			NULL)) {
			CM_CHK_GENOSSTATUS(HalCm_GetSurfaceDetails(pState,
								   pIndexParam,
								   iBTIndex + i,
								   Surface,
								   0,
								   pSurfaceEntries
								   [i],
								   dwTempPlaneIndex,
								   SurfaceParam,
								   CM_ARGUMENT_SURFACE2D));
		}
	}

	pState->pBT2DIndexTable[iIndex].RegularSurfIndex = iBTIndex;

	if (pBuffer) {
		pDst = pBuffer + pArgParam->iPayloadOffset;
		*((PUINT) pDst) = iBTIndex;
	}
	Surface.dwWidth = pState->pUmdSurf2DTable[iIndex].iWidth;
	Surface.dwHeight = pState->pUmdSurf2DTable[iIndex].iHeight;

	hr = GENOS_STATUS_SUCCESS;

 finish:
	return hr;
}

GENOS_STATUS HalCm_Setup2DSurfaceState(PCM_HAL_STATE pState,
				       PCM_HAL_KERNEL_ARG_PARAM pArgParam,
				       PCM_HAL_INDEX_PARAM pIndexParam,
				       INT iBindingTable,
				       UINT iThreadIndex, PBYTE pBuffer)
{
	GENOS_STATUS hr;

	CM_CHK_GENOSSTATUS(HalCm_Setup2DSurfaceStateBasic
			   (pState, pArgParam, pIndexParam, iBindingTable,
			    iThreadIndex, pBuffer, FALSE));
	hr = GENOS_STATUS_SUCCESS;

 finish:
	return hr;
}

GENOS_STATUS HalCm_Setup2DSurfaceUPStateBasic(PCM_HAL_STATE pState,
					      PCM_HAL_KERNEL_ARG_PARAM
					      pArgParam,
					      PCM_HAL_INDEX_PARAM pIndexParam,
					      INT iBindingTable,
					      UINT iThreadIndex, PBYTE pBuffer)
{
	GENOS_STATUS hr;
	GENHW_SURFACE Surface;
	GENHW_SURFACE_STATE_PARAMS SurfaceParam;
	PGENHW_HW_INTERFACE pHwInterface;
	PGENHW_SURFACE_STATE_ENTRY pSurfaceEntries[GENHW_MAX_SURFACE_PLANES];
	PBYTE pSrc;
	PBYTE pDst;
	INT iSurfaceEntries;
	UINT iIndex;
	UINT iBTIndex = 0;
	UINT i;
	WORD memObjCtl = CM_DEFAULT_CACHE_TYPE;

	hr = GENOS_STATUS_UNKNOWN;
	pHwInterface = pState->pHwInterface;
	PCM_HAL_TASK_PARAM pTaskParam = pState->pTaskParam;

	CM_ASSERT(pArgParam->iUnitSize == sizeof(iIndex));
	pSrc = pArgParam->pFirstValue + (iThreadIndex * pArgParam->iUnitSize);
	iIndex = *((PUINT) pSrc) & CM_SURFACE_MASK;
	if (iIndex == CM_NULL_SURFACE) {
		if (pBuffer) {
			pDst = pBuffer + pArgParam->iPayloadOffset;
			*((PUINT) pDst) = CM_NULL_SURFACE_BINDING_INDEX;
		}

		hr = GENOS_STATUS_SUCCESS;
		goto finish;
	}

	memObjCtl =
	    (WORD) ((*((PUINT) pSrc) & CM_MEMORY_OBJECT_CONTROL_MASK) >> 16);
	if (!memObjCtl) {
		memObjCtl = CM_DEFAULT_CACHE_TYPE;
	}
	if (iIndex >= pState->CmDeviceParam.iMax2DSurfaceUPTableSize ||
	    (pState->pSurf2DUPTable[iIndex].iWidth == 0)) {
		CM_ERROR_ASSERT("Invalid 2D SurfaceUP array index '%d'",
				iIndex);
		goto finish;
	}
	iBTIndex = pState->pBT2DUPIndexTable[iIndex].RegularSurfIndex;

	if (iBTIndex == (unsigned char)CM_INVALID_INDEX) {
		UINT dwTempPlaneIndex = 0;

		CM_CHK_GENOSSTATUS(HalCm_GetSurfaceAndRegister
				   (pState, &Surface, CM_ARGUMENT_SURFACE2D_UP,
				    iIndex));

		GENOS_ZeroMemory(&SurfaceParam, sizeof(SurfaceParam));
		SurfaceParam.Type = pHwInterface->SurfaceTypeDefault;

		SurfaceParam.bRenderTarget = FALSE;
		SurfaceParam.bWidthInDword_UV = TRUE;
		SurfaceParam.bWidthInDword_Y = TRUE;

		pState->pfnSetSurfaceMemoryObjectControl(pState,
							 memObjCtl,
							 &SurfaceParam);

		CM_CHK_GENOSSTATUS(pHwInterface->pfnSetupSurfaceState
				   (pHwInterface, &Surface, &SurfaceParam,
				    &iSurfaceEntries, pSurfaceEntries));

		iBTIndex =
		    Halcm_GetFreeBindingIndex(pState, pIndexParam,
					      iSurfaceEntries);
		for (i = 0; i < (UINT) iSurfaceEntries; i++) {
			CM_CHK_GENOSSTATUS(pHwInterface->pfnBindSurfaceState
					   (pHwInterface, iBindingTable,
					    iBTIndex + i, pSurfaceEntries[i]));
			if ((pTaskParam->SurEntryInfoArrays.dwKrnNum != 0) &&
			    (pTaskParam->
			     SurEntryInfoArrays.pSurfEntryInfosArray != NULL)) {
				CM_CHK_GENOSSTATUS(HalCm_GetSurfaceDetails
						   (pState, pIndexParam,
						    iBTIndex + i, Surface, 0,
						    pSurfaceEntries[i],
						    dwTempPlaneIndex,
						    SurfaceParam,
						    CM_ARGUMENT_SURFACE2D_UP));
			}
		}

		pState->pBT2DUPIndexTable[iIndex].RegularSurfIndex = iBTIndex;
	}
	if (pBuffer) {
		pDst = pBuffer + pArgParam->iPayloadOffset;
		*((PUINT) pDst) = iBTIndex;
	}

	hr = GENOS_STATUS_SUCCESS;

 finish:
	return hr;
}

GENOS_STATUS HalCm_Setup2DSurfaceUPState(PCM_HAL_STATE pState,
					 PCM_HAL_KERNEL_ARG_PARAM pArgParam,
					 PCM_HAL_INDEX_PARAM pIndexParam,
					 INT iBindingTable,
					 UINT iThreadIndex, PBYTE pBuffer)
{
	GENOS_STATUS hr;

	CM_CHK_GENOSSTATUS(HalCm_Setup2DSurfaceUPStateBasic
			   (pState, pArgParam, pIndexParam, iBindingTable,
			    iThreadIndex, pBuffer));
	hr = GENOS_STATUS_SUCCESS;

 finish:
	return hr;
}

GENOS_STATUS HalCm_GetPlatformInfo(PCM_HAL_STATE pState,
				   PCM_HAL_PLATFORM_SUBSLICE_INFO platformInfo,
				   PBOOL pbIsSingleSubSlice)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	PGENHW_HW_INTERFACE pHwInterface = pState->pHwInterface;

	switch (pState->Platform.eRenderCoreFamily) {
	case IGFX_GEN7_5_CORE:
		platformInfo->numHWThreadsPerEU = CM_GEN7_5_HW_THREADS_PER_EU;
		if (pHwInterface->Platform.GtType == GTTYPE_GT3) {
			if (pState->bRequestSingleSlice == TRUE
			    || pHwInterface->bRequestSingleSlice == TRUE
			    || pState->PowerOption.nSlice == 1) {
				platformInfo->numEUsPerSubSlice =
				    CM_GEN7_5_GT2_EUS_PER_SUBSLICE;
				platformInfo->numSlices =
				    CM_GEN7_5_GT2_SLICE_NUM;
				platformInfo->numSubSlices =
				    CM_GEN7_5_GT2_SUBSLICE_NUM;
				pState->bEUSaturationNoSSD = FALSE;
			} else {
				platformInfo->numEUsPerSubSlice =
				    CM_GEN7_5_GT3_EUS_PER_SUBSLICE;
				platformInfo->numSlices =
				    CM_GEN7_5_GT3_SLICE_NUM;
				platformInfo->numSubSlices =
				    CM_GEN7_5_GT3_SUBSLICE_NUM;
				pState->bEUSaturationNoSSD = TRUE;
			}
		} else if (pHwInterface->Platform.GtType == GTTYPE_GT2) {
			platformInfo->numEUsPerSubSlice =
			    CM_GEN7_5_GT2_EUS_PER_SUBSLICE;
			platformInfo->numSlices = CM_GEN7_5_GT2_SLICE_NUM;
			platformInfo->numSubSlices = CM_GEN7_5_GT2_SUBSLICE_NUM;
		} else if (pHwInterface->Platform.GtType == GTTYPE_GT1) {
			platformInfo->numEUsPerSubSlice =
			    CM_GEN7_5_GT1_EUS_PER_SUBSLICE;
			platformInfo->numSlices = CM_GEN7_5_GT1_SLICE_NUM;
			platformInfo->numSubSlices = CM_GEN7_5_GT1_SUBSLICE_NUM;
			*pbIsSingleSubSlice = TRUE;
		} else {
			CM_ERROR_ASSERT("Invalid GT for Gen 7_5");
			goto finish;
		}
		break;

	case IGFX_GEN8_CORE:
		platformInfo->numHWThreadsPerEU = CM_GEN8_HW_THREADS_PER_EU;
		if (GFX_IS_PRODUCT(pHwInterface->Platform, IGFX_CHERRYVIEW)) {
			platformInfo->numEUsPerSubSlice =
			    CM_GEN8LP_EUS_PER_SUBSLICE;
			platformInfo->numSlices = CM_GEN8LP_SLICE_NUM;
			platformInfo->numSubSlices = CM_GEN8LP_SUBSLICE_NUM;
		} else if (pHwInterface->Platform.GtType == GTTYPE_GT3) {
			if (pState->bRequestSingleSlice == TRUE
			    || pHwInterface->bRequestSingleSlice == TRUE
			    || pState->PowerOption.nSlice == 1) {
				platformInfo->numEUsPerSubSlice =
				    CM_GEN8_GT2_EUS_PER_SUBSLICE;
				platformInfo->numSlices = CM_GEN8_GT2_SLICE_NUM;
				platformInfo->numSubSlices =
				    CM_GEN8_GT2_SUBSLICE_NUM;
				pState->bEUSaturationNoSSD = FALSE;
			} else {
				platformInfo->numEUsPerSubSlice =
				    CM_GEN8_GT3_EUS_PER_SUBSLICE;
				platformInfo->numSlices = CM_GEN8_GT3_SLICE_NUM;
				platformInfo->numSubSlices =
				    CM_GEN8_GT3_SUBSLICE_NUM;
				pState->bEUSaturationNoSSD = TRUE;
			}
		} else if (pHwInterface->Platform.GtType == GTTYPE_GT2) {
			platformInfo->numEUsPerSubSlice =
			    CM_GEN8_GT2_EUS_PER_SUBSLICE;
			platformInfo->numSlices = CM_GEN8_GT2_SLICE_NUM;
			platformInfo->numSubSlices = CM_GEN8_GT2_SUBSLICE_NUM;
		} else if (pHwInterface->Platform.GtType == GTTYPE_GT1) {
			platformInfo->numEUsPerSubSlice =
			    CM_GEN8_GT1_EUS_PER_SUBSLICE;
			platformInfo->numSlices = CM_GEN8_GT1_SLICE_NUM;
			platformInfo->numSubSlices = CM_GEN8_GT1_SUBSLICE_NUM;
		} else {
			CM_ERROR_ASSERT("Invalid GT for Gen 8");
			goto finish;
		}
		break;

	default:
		CM_ERROR_ASSERT
		    ("Platform currently not supported for EnqueueWithHints");
		goto finish;
	}

 finish:
	return hr;
}

GENOS_STATUS HalCm_GetNumKernelsPerGroup(BYTE hintsBits,
					 UINT numKernels,
					 PUINT pNumKernelsPerGroup,
					 PUINT pNumKernelGroups,
					 PUINT pRemapKrnToGrp,
					 PUINT pRemapGrpToKrn)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	UINT currGrp = 0;
	UINT i = 0;

	pNumKernelsPerGroup[currGrp]++;
	pRemapGrpToKrn[currGrp] = 0;

	for (i = 0; i < numKernels - 1; ++i) {
		if ((hintsBits & CM_HINTS_LEASTBIT_MASK) ==
		    CM_HINTS_LEASTBIT_MASK) {
			currGrp++;
			*pNumKernelGroups = *pNumKernelGroups + 1;

			pRemapGrpToKrn[currGrp] = i + 1;
		}
		pNumKernelsPerGroup[currGrp]++;
		hintsBits >>= 1;
		pRemapKrnToGrp[i + 1] = currGrp;
	}

	return hr;
}

GENOS_STATUS HalCm_GetParallelGraphInfo(UINT maximum,
					UINT numThreads,
					UINT width,
					UINT height,
					PCM_HAL_PARALLELISM_GRAPH_INFO
					graphInfo,
					CM_HAL_DEPENDENCY_PATTERN pattern)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	UINT numThreadsOnSides = 0;
	UINT numMaxRepeat = 0;
	UINT numSteps = 0;

	switch (pattern) {
	case CM_DEPENDENCY_NONE:
		break;

	case CM_DEPENDENCY_VERTICAL:
		numMaxRepeat = width;
		numSteps = width;
		break;

	case CM_DEPENDENCY_HORIZONTAL:
		numMaxRepeat = height;
		numSteps = height;
		break;

	case CM_DEPENDENCY_WAVEFRONT:
		numThreadsOnSides = (maximum - 1) * maximum;
		numMaxRepeat = (numThreads - numThreadsOnSides) / maximum;
		numSteps = (maximum - 1) * 2 + numMaxRepeat;
		break;

	case CM_DEPENDENCY_WAVEFRONT26:
		numThreadsOnSides = (maximum - 1) * maximum * 2;
		numMaxRepeat = (numThreads - numThreadsOnSides) / maximum;
		numSteps = ((maximum - 1) * 2) * 2 + numMaxRepeat;
		break;

	case CM_DEPENDENCY_WAVEFRONT26Z:
		break;

	default:
		CM_ERROR_ASSERT
		    ("Unsupported dependency pattern for EnqueueWithHints");
		goto finish;
	}

	graphInfo->maxParallelism = maximum;
	graphInfo->numMaxRepeat = numMaxRepeat;
	graphInfo->numSteps = numSteps;

 finish:
	return hr;
}

GENOS_STATUS HalCm_SetDispatchPattern(CM_HAL_PARALLELISM_GRAPH_INFO graphInfo,
				      CM_HAL_DEPENDENCY_PATTERN pattern,
				      PUINT pDispatchFreq)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	UINT i = 0;
	UINT j = 0;
	UINT k = 0;

	switch (pattern) {
	case CM_DEPENDENCY_NONE:
		break;
	case CM_DEPENDENCY_HORIZONTAL:
	case CM_DEPENDENCY_VERTICAL:
		for (i = 0; i < graphInfo.numSteps; ++i) {
			pDispatchFreq[i] = graphInfo.maxParallelism;
		}
		break;
	case CM_DEPENDENCY_WAVEFRONT:
		for (i = 1; i < graphInfo.maxParallelism; ++i) {
			pDispatchFreq[i - 1] = i;
		}
		for (j = 0; j < graphInfo.numMaxRepeat; ++i, ++j) {
			pDispatchFreq[i - 1] = graphInfo.maxParallelism;
		}
		for (j = graphInfo.maxParallelism - 1; i <= graphInfo.numSteps;
		     ++i, --j) {
			pDispatchFreq[i - 1] = j;
		}
		break;
	case CM_DEPENDENCY_WAVEFRONT26:
		for (i = 1, j = 0; i < graphInfo.maxParallelism; ++i, j += 2) {
			pDispatchFreq[j] = i;
			pDispatchFreq[j + 1] = i;
		}
		for (k = 0; k < graphInfo.numMaxRepeat; ++k, ++j) {
			pDispatchFreq[j] = graphInfo.maxParallelism;
		}
		for (i = graphInfo.maxParallelism - 1; j < graphInfo.numSteps;
		     j += 2, --i) {
			pDispatchFreq[j] = i;
			pDispatchFreq[j + 1] = i;
		}
		break;
	case CM_DEPENDENCY_WAVEFRONT26Z:
		break;
	default:
		CM_ERROR_ASSERT
		    ("Unsupported dependency pattern for EnqueueWithHints");
		goto finish;
	}

 finish:
	return hr;
}

GENOS_STATUS HalCm_SetKernelGrpFreqDispatch(PCM_HAL_PARALLELISM_GRAPH_INFO
					    graphInfo,
					    PCM_HAL_KERNEL_GROUP_INFO groupInfo,
					    UINT numKernelGroups,
					    PUINT pMinSteps)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	UINT i = 0;
	UINT j = 0;
	UINT tmpSteps = 0;
	UINT kerIndex = 0;

	for (i = 0; i < numKernelGroups; ++i) {
		for (j = 0; j < groupInfo[i].numKernelsInGroup; ++j) {
			tmpSteps += graphInfo[kerIndex].numSteps;
			kerIndex++;
		}

		if (tmpSteps) {
			*pMinSteps = MIN(*pMinSteps, tmpSteps);
			groupInfo[i].numStepsInGrp = tmpSteps;
		}

		tmpSteps = 0;
	}

	for (i = 0; i < numKernelGroups; ++i) {
		groupInfo[i].freqDispatch = (UINT)
		    ceil((groupInfo[i].numStepsInGrp / (double)*pMinSteps));
	}

	return hr;
}

GENOS_STATUS HalCm_SetNoDependKernelDispatchPattern(UINT numThreads,
						    UINT minSteps,
						    PUINT pDispatchFreq)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	UINT i = 0;
	UINT numEachStep = 0;
	UINT total = 0;

	numEachStep = numThreads / minSteps;
	for (i = 0; i < minSteps; ++i) {
		pDispatchFreq[i] = numEachStep;
		total += numEachStep;
	}

	while (total != numThreads) {
		i = 0;
		pDispatchFreq[i]++;
		total++;
		i++;
	}

	return hr;
}

GENOS_STATUS HalCm_FinishStatesForKernelMix(PCM_HAL_STATE pState,
					    PGENHW_BATCH_BUFFER pBatchBuffer,
					    INT iTaskId,
					    PCM_HAL_KERNEL_PARAM * pExecKernels,
					    PCM_HAL_INDEX_PARAM pIndexParams,
					    PINT pBindingTableEntries,
					    PINT pMediaIds,
					    PINT pKAIDs,
					    UINT iNumKernels,
					    UINT iHints, BOOLEAN lastTask)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	PGENHW_HW_INTERFACE pHwInterface = pState->pHwInterface;
	PGENHW_HW_MEDIAOBJECT_PARAM pMediaObjectParams = NULL;
	PCM_HAL_KERNEL_PARAM *pKernelParams = NULL;
	PCM_HAL_KERNEL_ARG_PARAM *pArgParams = NULL;
	PMEDIA_OBJECT_HEADER_G6 *pCmds = NULL;
	PGENHW_SCOREBOARD_PARAMS pScoreboardParams = NULL;
	PCM_HAL_PARALLELISM_GRAPH_INFO pParallelGraphInfo = NULL;
	PCM_HAL_KERNEL_ARG_PARAM pArgParam = NULL;
	PCM_HAL_KERNEL_SUBSLICE_INFO pKernelsSliceInfo = NULL;
	PCM_HAL_KERNEL_THREADSPACE_PARAM pKernelTSParam = NULL;
	PCM_HAL_KERNEL_GROUP_INFO pGroupInfo = NULL;
	CM_HAL_DEPENDENCY vfeDependencyInfo;
	CM_HAL_PLATFORM_SUBSLICE_INFO platformInfo;
	CM_HAL_SCOREBOARD_XY_MASK threadCoordinates;
	PUINT *pDependRemap = NULL;
	PUINT *pDispatchFreq = NULL;
	PBYTE *pCmd_data = NULL;
	PBYTE *pCmd_inline = NULL;
	PDWORD pCmd_sizes = NULL;
	PUINT pRemapKrnToGrp = NULL;
	PUINT pRemapGrpToKrn = NULL;
	PUINT pNumKernelsPerGrp = NULL;
	PBYTE pKernelScoreboardMask = NULL;
	BYTE hintsBits = 0;
	BYTE tmpThreadScoreboardMask = 0;
	BYTE scoreboardMask = 0;
	BOOL bSingleSubSlice = FALSE;
	BOOL bEnableThreadSpace = FALSE;
	BOOL bKernelFound = FALSE;
	BOOL bUpdateCurrKernel = FALSE;
	DWORD iAdjustedYCoord = 0;
	UINT numKernelGroups = CM_HINTS_DEFAULT_NUM_KERNEL_GRP;
	UINT totalNumThreads = 0;
	UINT iHdrSize = 0;
	UINT i = 0;
	UINT j = 0;
	UINT k = 0;
	UINT tmp = 0;
	UINT tmp1 = 0;
	UINT loopCount = 0;
	UINT aIndex = 0;
	UINT index = 0;
	UINT totalReqSubSlices = 0;
	UINT difference = 0;
	UINT curKernel = 0;
	UINT numSet = 0;
	UINT sliceIndex = 0;
	UINT tmpNumSubSlice = 0;
	UINT tmpNumKernelsPerGrp = 0;
	UINT maximum = 0;
	UINT count = 0;
	UINT numDispatched = 0;
	UINT tmpIndex = 0;
	UINT numStepsDispatched = 0;
	UINT minSteps = UINT_MAX;
	UINT grpId = 0;
	UINT allocSize = 0;
	UINT iCurrentKernel = 0;
	UINT iRoundRobinCount = 0;
	UINT numTasks = 0;
	UINT extraSWThreads = 0;
	UINT subSliceDestStart = 0;
	UINT plusNSubSliceDest = 0;

	CM_CHK_NULL_RETURN_GENOSSTATUS(pBatchBuffer);

	GENOS_ZeroMemory(&threadCoordinates, sizeof(CM_HAL_SCOREBOARD_XY_MASK));
	GENOS_ZeroMemory(&vfeDependencyInfo, sizeof(CM_HAL_DEPENDENCY));
	GENOS_ZeroMemory(&platformInfo, sizeof(CM_HAL_PLATFORM_SUBSLICE_INFO));

	pMediaObjectParams = (PGENHW_HW_MEDIAOBJECT_PARAM)
	    GENOS_AllocAndZeroMemory(sizeof(GENHW_HW_MEDIAOBJECT_PARAM) *
				     iNumKernels);
	pKernelParams = (PCM_HAL_KERNEL_PARAM *)
	    GENOS_AllocAndZeroMemory(sizeof(PCM_HAL_KERNEL_PARAM) *
				     iNumKernels);
	pArgParams = (PCM_HAL_KERNEL_ARG_PARAM *)
	    GENOS_AllocAndZeroMemory(sizeof(PCM_HAL_KERNEL_ARG_PARAM) *
				     iNumKernels);
	pCmds = (PMEDIA_OBJECT_HEADER_G6 *)
	    GENOS_AllocAndZeroMemory(sizeof(PMEDIA_OBJECT_HEADER_G6) *
				     iNumKernels);
	pCmd_data =
	    (PBYTE *) GENOS_AllocAndZeroMemory(sizeof(PBYTE) * iNumKernels);
	pCmd_inline =
	    (PBYTE *) GENOS_AllocAndZeroMemory(sizeof(PBYTE) * iNumKernels);
	pCmd_sizes =
	    (PDWORD) GENOS_AllocAndZeroMemory(sizeof(DWORD) * iNumKernels);
	pRemapKrnToGrp =
	    (PUINT) GENOS_AllocAndZeroMemory(sizeof(UINT) * iNumKernels);
	pRemapGrpToKrn =
	    (PUINT) GENOS_AllocAndZeroMemory(sizeof(UINT) * iNumKernels);
	pKernelScoreboardMask =
	    (PBYTE) GENOS_AllocAndZeroMemory(sizeof(BYTE) * iNumKernels);
	pDependRemap =
	    (PUINT *) GENOS_AllocAndZeroMemory(sizeof(PUINT) * iNumKernels);
	pParallelGraphInfo = (PCM_HAL_PARALLELISM_GRAPH_INFO)
	    GENOS_AllocAndZeroMemory(sizeof(CM_HAL_PARALLELISM_GRAPH_INFO) *
				     iNumKernels);
	pDispatchFreq =
	    (PUINT *) GENOS_AllocAndZeroMemory(sizeof(PUINT) * iNumKernels);
	pNumKernelsPerGrp =
	    (PUINT) GENOS_AllocAndZeroMemory(sizeof(UINT) * iNumKernels);

	if (!pMediaObjectParams || !pKernelParams || !pArgParams ||
	    !pCmds || !pCmd_data || !pCmd_inline || !pCmd_sizes ||
	    !pRemapKrnToGrp || !pRemapGrpToKrn || !pKernelScoreboardMask
	    || !pDependRemap || !pParallelGraphInfo || !pDispatchFreq
	    || !pNumKernelsPerGrp) {
		CM_ERROR_ASSERT("Memory allocation failed in EnqueueWithHints");
		goto finish;
	}

	pState->bEUSaturationEnabled = TRUE;

	hintsBits =
	    (iHints & CM_HINTS_MASK_KERNEL_GROUPS) >>
	    CM_HINTS_NUM_BITS_WALK_OBJ;
	CM_CHK_GENOSSTATUS(HalCm_GetNumKernelsPerGroup
			   (hintsBits, iNumKernels, pNumKernelsPerGrp,
			    &numKernelGroups, pRemapKrnToGrp, pRemapGrpToKrn));

	pKernelsSliceInfo = (PCM_HAL_KERNEL_SUBSLICE_INFO)
	    GENOS_AllocAndZeroMemory(sizeof(CM_HAL_KERNEL_SUBSLICE_INFO) *
				     numKernelGroups);
	pGroupInfo = (PCM_HAL_KERNEL_GROUP_INFO)
	    GENOS_AllocAndZeroMemory(sizeof(CM_HAL_KERNEL_GROUP_INFO) *
				     numKernelGroups);
	if (!pKernelsSliceInfo || !pGroupInfo) {
		CM_ERROR_ASSERT("Memory allocation failed in EnqueueWithHints");
		goto finish;
	}

	for (i = 0; i < numKernelGroups; ++i) {
		pGroupInfo[i].numKernelsInGroup = pNumKernelsPerGrp[i];
	}

	iHdrSize = pHwInterface->pHwCommands->dwMediaObjectHeaderCmdSize;

	for (i = 0; i < iNumKernels; ++i) {
		pKernelParams[i] = pExecKernels[i];

		pMediaObjectParams[i].dwIDOffset = pMediaIds[i];
		pMediaObjectParams[i].dwMediaObjectSize =
		    iHdrSize + MAX(pKernelParams[i]->iPayloadSize, 4);

		pCmd_data[i] =
		    (PBYTE) GENOS_AllocAndZeroMemory(sizeof(BYTE) * 1024);
		pCmd_inline[i] = pCmd_data[i] + iHdrSize;
		pCmds[i] = (PMEDIA_OBJECT_HEADER_G6) pCmd_data[i];
		pCmd_sizes[i] = pMediaObjectParams[i].dwMediaObjectSize;

		*pCmds[i] = g_cInit_MEDIA_OBJECT_HEADER_G6;
		pCmds[i]->DW0.DWordLength =
		    OP_LENGTH(SIZE_IN_DW
			      (pMediaObjectParams[i].dwMediaObjectSize));
		pCmds[i]->DW1.InterfaceDescriptorOffset =
		    pMediaObjectParams[i].dwIDOffset;

		totalNumThreads += pKernelParams[i]->iNumThreads;
	}

	numTasks =
	    (iHints & CM_HINTS_MASK_NUM_TASKS) >> CM_HINTS_NUM_BITS_TASK_POS;
	if (numTasks > 1) {
		if (lastTask) {
			extraSWThreads = totalNumThreads % numTasks;
		}

		totalNumThreads = (totalNumThreads / numTasks) + extraSWThreads;
	}

	for (i = 0; i < iNumKernels; ++i) {
		pDependRemap[i] =
		    (PUINT) GENOS_AllocAndZeroMemory(sizeof(UINT) *
						     CM_HAL_MAX_DEPENDENCY_COUNT);
		for (k = 0; k < CM_HAL_MAX_DEPENDENCY_COUNT; ++k) {
			pDependRemap[i][k] = k;
		}
	}

	for (i = 0; i < iNumKernels; ++i) {
		pKernelTSParam = &pKernelParams[i]->CmKernelThreadSpaceParam;

		if (pKernelTSParam->dependencyInfo.count) {
			if (vfeDependencyInfo.count == 0) {
				GENOS_SecureMemcpy(&vfeDependencyInfo,
						   sizeof(CM_HAL_DEPENDENCY),
						   &pKernelTSParam->dependencyInfo,
						   sizeof(CM_HAL_DEPENDENCY));
				pKernelScoreboardMask[i] =
				    (1 << vfeDependencyInfo.count) - 1;
			} else {
				for (j = 0;
				     j < pKernelTSParam->dependencyInfo.count;
				     ++j) {
					for (k = 0; k < vfeDependencyInfo.count;
					     ++k) {
						if ((pKernelTSParam->dependencyInfo.deltaX[j] == vfeDependencyInfo.deltaX[k])
						    &&
						    (pKernelTSParam->dependencyInfo.deltaY
						     [j] ==
						     vfeDependencyInfo.deltaY
						     [k])) {
							HAL_CM_SETBIT
							    (pKernelScoreboardMask
							     [i], k);
							pDependRemap[i][j] = k;
							break;
						}
					}
					if (k == vfeDependencyInfo.count) {
						vfeDependencyInfo.deltaX
						    [vfeDependencyInfo.count] =
						    pKernelTSParam->dependencyInfo.
						    deltaX[j];
						vfeDependencyInfo.deltaY
						    [vfeDependencyInfo.count] =
						    pKernelTSParam->dependencyInfo.
						    deltaY[j];
						HAL_CM_SETBIT
						    (pKernelScoreboardMask[i],
						     vfeDependencyInfo.count);
						vfeDependencyInfo.count++;
						pDependRemap[i][j] = k;
					}
				}
			}
		}
	}

	if (vfeDependencyInfo.count > CM_HAL_MAX_DEPENDENCY_COUNT) {
		CM_ERROR_ASSERT
		    ("Union of kernel dependencies exceeds max dependency count (8)");
		goto finish;
	}
	pScoreboardParams = &pState->ScoreboardParams;
	pScoreboardParams->numMask = (BYTE) vfeDependencyInfo.count;
	for (i = 0; i < pScoreboardParams->numMask; ++i) {
		pScoreboardParams->ScoreboardDelta[i].x =
		    vfeDependencyInfo.deltaX[i];
		pScoreboardParams->ScoreboardDelta[i].y =
		    vfeDependencyInfo.deltaY[i];
	}

	CM_CHK_GENOSSTATUS(HalCm_GetPlatformInfo
			   (pState, &platformInfo, &bSingleSubSlice));

	if (!bSingleSubSlice) {
		for (i = 0; i < numKernelGroups; ++i) {
			tmpNumKernelsPerGrp = pNumKernelsPerGrp[i];

			for (j = 0; j < tmpNumKernelsPerGrp; ++j) {
				pKernelTSParam =
				    &pKernelParams
				    [count]->CmKernelThreadSpaceParam;

				switch (pKernelTSParam->patternType) {
				case CM_DEPENDENCY_NONE:
					maximum =
					    pKernelParams[count]->iNumThreads;
					break;
				case CM_DEPENDENCY_WAVEFRONT:
					maximum =
					    MIN
					    (pKernelTSParam->iThreadSpaceWidth,
					     pKernelTSParam->iThreadSpaceHeight);
					break;
				case CM_DEPENDENCY_WAVEFRONT26:
					maximum =
					    MIN(((pKernelTSParam->iThreadSpaceWidth + 1) >> 1), pKernelTSParam->iThreadSpaceHeight);
					break;
				case CM_DEPENDENCY_VERTICAL:
					maximum =
					    pKernelTSParam->iThreadSpaceHeight;
					break;
				case CM_DEPENDENCY_HORIZONTAL:
					maximum =
					    pKernelTSParam->iThreadSpaceWidth;
					break;
				case CM_DEPENDENCY_WAVEFRONT26Z:
					maximum =
					    MIN(((pKernelTSParam->iThreadSpaceWidth - 1) >> 1), pKernelTSParam->iThreadSpaceHeight);
					break;
				default:
					CM_ERROR_ASSERT
					    ("Unsupported dependency pattern for EnqueueWithHints");
					goto finish;
				}

				if (pKernelTSParam->patternType !=
				    CM_DEPENDENCY_WAVEFRONT26Z) {
					CM_CHK_GENOSSTATUS
					    (HalCm_GetParallelGraphInfo
					     (maximum,
					      pKernelParams[count]->iNumThreads,
					      pKernelTSParam->iThreadSpaceWidth,
					      pKernelTSParam->iThreadSpaceHeight,
					      &pParallelGraphInfo[count],
					      pKernelTSParam->patternType));
				} else {
					pParallelGraphInfo[count].numSteps =
					    pKernelTSParam->
					    dispatchInfo.numWaves;
				}

				if (pKernelTSParam->patternType !=
				    CM_DEPENDENCY_NONE) {
					pDispatchFreq[count] = (PUINT)
					    GENOS_AllocAndZeroMemory(sizeof
								     (UINT)
								     *
								     pParallelGraphInfo
								     [count].numSteps);
					if (!pDispatchFreq[count]) {
						CM_ERROR_ASSERT
						    ("Memory allocation failed for EnqueueWithHints");
						goto finish;
					}

					if (pKernelTSParam->patternType !=
					    CM_DEPENDENCY_WAVEFRONT26Z) {
						CM_CHK_GENOSSTATUS
						    (HalCm_SetDispatchPattern
						     (pParallelGraphInfo[count],
						      pKernelTSParam->patternType,
						      pDispatchFreq[count]));
					} else {
						GENOS_SecureMemcpy(pDispatchFreq
								   [count],
								   sizeof(UINT)
								   *
								   pParallelGraphInfo
								   [count].numSteps,
								   pKernelTSParam->dispatchInfo.pNumThreadsInWave,
								   sizeof(UINT)
								   *
								   pParallelGraphInfo
								   [count].numSteps);
					}
				}

				tmpNumSubSlice =
				    (maximum /
				     (platformInfo.numEUsPerSubSlice *
				      platformInfo.numHWThreadsPerEU)) + 1;

				if (tmpNumSubSlice > platformInfo.numSubSlices) {
					tmpNumSubSlice =
					    platformInfo.numSubSlices - 1;
				}

				if (tmpNumSubSlice >
				    pKernelsSliceInfo[i].numSubSlices) {
					pKernelsSliceInfo[i].numSubSlices =
					    tmpNumSubSlice;
				}

				count++;
			}
		}

		for (i = 0; i < numKernelGroups; ++i) {
			totalReqSubSlices += pKernelsSliceInfo[i].numSubSlices;
		}

		if (totalReqSubSlices < platformInfo.numSubSlices) {
			difference =
			    platformInfo.numSubSlices - totalReqSubSlices;
			tmp = tmp1 = 0;
			for (i = 0; i < difference; ++i) {
				tmp = tmp1 % numKernelGroups;
				pKernelsSliceInfo[tmp].numSubSlices++;
				totalReqSubSlices++;
				tmp1++;
			}
		} else if (totalReqSubSlices > platformInfo.numSubSlices) {
			difference =
			    totalReqSubSlices - platformInfo.numSubSlices;
			tmp = 0;
			tmp1 = numKernelGroups - 1;
			for (i = numKernelGroups - 1, j = 0; j < difference;
			     --i, ++j) {
				tmp = tmp1 % numKernelGroups;
				pKernelsSliceInfo[tmp].numSubSlices--;
				totalReqSubSlices--;
				tmp1 += numKernelGroups - 1;
			}
		}

		if (totalReqSubSlices != platformInfo.numSubSlices) {
			CM_ERROR_ASSERT
			    ("Total requested sub-slices does not match platform's number of sub-slices");
			goto finish;
		}

		for (i = 0; i < numKernelGroups; ++i) {
			pKernelsSliceInfo[i].pDestination =
			    (PCM_HAL_KERNEL_SLICE_SUBSLICE)
			    GENOS_AllocAndZeroMemory(sizeof
						     (CM_HAL_KERNEL_SLICE_SUBSLICE)
						     *
						     pKernelsSliceInfo
						     [i].numSubSlices);
			if (!pKernelsSliceInfo[i].pDestination) {
				CM_ERROR_ASSERT
				    ("Memory allocation failed in EnqueueWithHints");
				goto finish;
			}
		}

		if (pState->Platform.eRenderCoreFamily < IGFX_GEN8_CORE) {
			subSliceDestStart = 1;
			plusNSubSliceDest = 1;
		} else {
			subSliceDestStart = 0;
			plusNSubSliceDest = 0;
		}
		for (i = 0; i < platformInfo.numSlices; ++i) {
			for (j = subSliceDestStart;
			     j <
			     (platformInfo.numSubSlices /
			      platformInfo.numSlices) + plusNSubSliceDest;
			     ++j) {
				if (pKernelsSliceInfo[curKernel].numSubSlices ==
				    numSet) {
					curKernel++;
					numSet = 0;
				}

				pKernelsSliceInfo[curKernel].pDestination
				    [numSet].slice = i;
				pKernelsSliceInfo[curKernel].pDestination
				    [numSet].subSlice = j;

				numSet++;
			}
		}

		CM_CHK_GENOSSTATUS(HalCm_SetKernelGrpFreqDispatch
				   (pParallelGraphInfo, pGroupInfo,
				    numKernelGroups, &minSteps));

		for (i = 0; i < iNumKernels; ++i) {
			if (pKernelParams[i]->
			    CmKernelThreadSpaceParam.patternType ==
			    CM_DEPENDENCY_NONE) {
				grpId = pRemapKrnToGrp[i];
				allocSize = 0;

				if (pGroupInfo[grpId].freqDispatch == 0) {
					allocSize = minSteps;
					pGroupInfo[grpId].freqDispatch = 1;
				} else {
					allocSize =
					    minSteps *
					    pGroupInfo[grpId].freqDispatch;
					pGroupInfo[grpId].freqDispatch =
					    pGroupInfo[grpId].freqDispatch * 2;
				}

				pDispatchFreq[i] = (PUINT)
				    GENOS_AllocAndZeroMemory(sizeof(UINT)
							     * allocSize);
				if (!pDispatchFreq[i]) {
					CM_ERROR_ASSERT
					    ("Memory allocation failed in EnqueueWithHints");
					goto finish;
				}

				CM_CHK_GENOSSTATUS
				    (HalCm_SetNoDependKernelDispatchPattern
				     (pKernelParams[i]->iNumThreads, allocSize,
				      pDispatchFreq[i]));
			}
		}
	}
	if (pBatchBuffer->pBBRenderData->BbArgs.BbCmArgs.uiRefCount > 1) {

		PBYTE pBBuffer = pBatchBuffer->pData + pBatchBuffer->iCurrent;
		bUpdateCurrKernel = FALSE;
		for (i = 0; i < totalNumThreads; ++i) {
			if (!bSingleSubSlice) {
				if ((pDispatchFreq[iCurrentKernel]
				     [pState->HintIndexes.iDispatchIndexes
				      [iCurrentKernel]] == numDispatched)
				    || (pState->HintIndexes.iKernelIndexes
					[iCurrentKernel] >=
					pKernelParams
					[iCurrentKernel]->iNumThreads)) {
					numDispatched = 0;
					numStepsDispatched++;
					pState->HintIndexes.iDispatchIndexes
					    [iCurrentKernel]++;

					if (pState->HintIndexes.iKernelIndexes
					    [iCurrentKernel] >=
					    pKernelParams
					    [iCurrentKernel]->iNumThreads) {
						bUpdateCurrKernel = TRUE;
						pGroupInfo[pRemapKrnToGrp
							   [iCurrentKernel]].numKernelsFinished++;
						if (pGroupInfo
						    [pRemapKrnToGrp
						     [iCurrentKernel]].numKernelsFinished
						    ==
						    pGroupInfo[pRemapKrnToGrp
							       [iCurrentKernel]].numKernelsInGroup)
						{
							pGroupInfo
							    [pRemapKrnToGrp
							     [iCurrentKernel]].groupFinished
							    = 1;
						} else {
							pRemapGrpToKrn
							    [tmpIndex]++;
						}
					}

					if ((pGroupInfo
					     [pRemapKrnToGrp
					      [iCurrentKernel]].freqDispatch ==
					     numStepsDispatched)
					    || bUpdateCurrKernel) {
						numStepsDispatched = 0;
						iRoundRobinCount++;

						tmpIndex =
						    iRoundRobinCount %
						    numKernelGroups;

						if (pGroupInfo
						    [tmpIndex].groupFinished) {
							loopCount = 0;
							while ((loopCount <
								numKernelGroups)
							       &&
							       (!bKernelFound))
							{
								iRoundRobinCount++;
								tmpIndex =
								    iRoundRobinCount
								    %
								    numKernelGroups;
								if (pState->HintIndexes.iKernelIndexes[pRemapGrpToKrn[tmpIndex]]
								    <
								    pKernelParams
								    [pRemapGrpToKrn
								     [tmpIndex]]->iNumThreads)
								{
									bKernelFound
									    =
									    TRUE;
								}
								loopCount++;
							}
							if (!bKernelFound) {
								CM_ERROR_ASSERT
								    ("Couldn't find kernel with threads left for EnqueueWithHints");
								goto finish;
							}
						}
						iCurrentKernel =
						    pRemapGrpToKrn[tmpIndex];
					}
				}
			} else {
				if (pState->
				    HintIndexes.iKernelIndexes[iCurrentKernel]
				    >=
				    pKernelParams[iCurrentKernel]->iNumThreads)
				{
					iCurrentKernel++;
				}
			}

			if (pKernelParams
			    [iCurrentKernel]->CmKernelThreadSpaceParam.
			    pThreadCoordinates) {
				threadCoordinates.y =
				    pKernelParams
				    [iCurrentKernel]->CmKernelThreadSpaceParam.pThreadCoordinates
				    [pState->
				     HintIndexes.iKernelIndexes
				     [iCurrentKernel]].y;
				threadCoordinates.mask =
				    pKernelParams
				    [iCurrentKernel]->CmKernelThreadSpaceParam.pThreadCoordinates
				    [pState->
				     HintIndexes.iKernelIndexes
				     [iCurrentKernel]].mask;
				bEnableThreadSpace = TRUE;
				threadCoordinates.resetMask =
				    pKernelParams
				    [iCurrentKernel]->
				    CmKernelThreadSpaceParam.pThreadCoordinates
				    [pState->
				     HintIndexes.iKernelIndexes
				     [iCurrentKernel]].resetMask;
			}

			if (bEnableThreadSpace) {
				if (threadCoordinates.mask !=
				    CM_DEFAULT_THREAD_DEPENDENCY_MASK) {
					tmpThreadScoreboardMask =
					    pKernelScoreboardMask
					    [iCurrentKernel];
					for (k = 0;
					     k <
					     pKernelParams
					     [iCurrentKernel]->CmKernelThreadSpaceParam.dependencyInfo.
					     count; ++k) {
						if ((threadCoordinates.mask &
						     CM_HINTS_LEASTBIT_MASK) ==
						    0) {
							HAL_CM_UNSETBIT
							    (tmpThreadScoreboardMask,
							     pDependRemap
							     [iCurrentKernel]
							     [k]);
						}

						threadCoordinates.mask =
						    threadCoordinates.mask >> 1;
					}
					scoreboardMask =
					    tmpThreadScoreboardMask;
				} else {
					scoreboardMask =
					    pKernelScoreboardMask
					    [iCurrentKernel];
				}
			} else {
				threadCoordinates.y =
				    pState->
				    HintIndexes.iKernelIndexes[iCurrentKernel] /
				    pKernelParams
				    [iCurrentKernel]->CmKernelThreadSpaceParam.
				    iThreadSpaceWidth;
				scoreboardMask =
				    pKernelScoreboardMask[iCurrentKernel];
			}

			iAdjustedYCoord = 0;
			if (iCurrentKernel > 0) {
				if (pKernelScoreboardMask[iCurrentKernel]) {
					if (threadCoordinates.y == 0) {
						for (k = 0;
						     k <
						     vfeDependencyInfo.count;
						     ++k) {
							if (vfeDependencyInfo.deltaY[k] < 0) {
								HAL_CM_UNSETBIT
								    (scoreboardMask,
								     k);
							}
						}
					}
				}
			}

			if (iCurrentKernel < iNumKernels - 1) {
				if (pKernelScoreboardMask[iCurrentKernel]) {
					if (threadCoordinates.y ==
					    (pKernelParams
					     [iCurrentKernel]->CmKernelThreadSpaceParam.iThreadSpaceHeight
					     - 1)) {
						for (k = 0;
						     k <
						     vfeDependencyInfo.count;
						     ++k) {
							if (vfeDependencyInfo.deltaY[k] > 0) {
								HAL_CM_UNSETBIT
								    (scoreboardMask,
								     k);
							}
						}
					}
				}
			}

			for (aIndex = 0;
			     aIndex < pKernelParams[iCurrentKernel]->iNumArgs;
			     aIndex++) {
				pArgParams[iCurrentKernel] =
				    &pKernelParams[iCurrentKernel]->CmArgParams
				    [aIndex];
				index =
				    pState->
				    HintIndexes.iKernelIndexes[iCurrentKernel] *
				    pArgParams[iCurrentKernel]->bPerThread;

				if ((pKernelParams[iCurrentKernel]->dwCmFlags &
				     CM_KERNEL_FLAGS_CURBE)
				    && !pArgParams[iCurrentKernel]->bPerThread) {
					continue;
				}

				CM_ASSERT(pArgParams
					  [iCurrentKernel]->iPayloadOffset <
					  pKernelParams
					  [iCurrentKernel]->iPayloadSize);

				switch (pArgParams[iCurrentKernel]->Kind) {
				case CM_ARGUMENT_GENERAL:
					break;

				case CM_ARGUMENT_SURFACEBUFFER:
					CM_CHK_GENOSSTATUS
					    (HalCm_SetupBufferSurfaceState
					     (pState,
					      pArgParams[iCurrentKernel],
					      &pIndexParams[iCurrentKernel],
					      pBindingTableEntries
					      [iCurrentKernel], -1, index,
					      NULL));
					break;

				case CM_ARGUMENT_SURFACE2D_UP:
					CM_CHK_GENOSSTATUS
					    (HalCm_Setup2DSurfaceUPState
					     (pState,
					      pArgParams[iCurrentKernel],
					      &pIndexParams[iCurrentKernel],
					      pBindingTableEntries
					      [iCurrentKernel], index, NULL));
					break;

				case CM_ARGUMENT_SURFACE2D:
					CM_CHK_GENOSSTATUS
					    (HalCm_Setup2DSurfaceState
					     (pState,
					      pArgParams[iCurrentKernel],
					      &pIndexParams[iCurrentKernel],
					      pBindingTableEntries
					      [iCurrentKernel], index, NULL));
					break;

				default:
					hr = GENOS_STATUS_UNKNOWN;
					CM_ERROR_ASSERT
					    ("Argument kind '%d' is not supported",
					     pArgParams[iCurrentKernel]->Kind);
					goto finish;

				}
			}

			if (threadCoordinates.resetMask ==
			    CM_RESET_DEPENDENCY_MASK) {
				GENOS_SecureMemcpy(pBBuffer +
						   (CM_SCOREBOARD_MASK_POS_IN_MEDIA_OBJECT_CMD
						    * sizeof(DWORD)),
						   sizeof(BYTE),
						   &scoreboardMask,
						   sizeof(BYTE));
			}

			pBatchBuffer->iCurrent += pCmd_sizes[iCurrentKernel];
			pBBuffer += pCmd_sizes[iCurrentKernel];

			pState->HintIndexes.iKernelIndexes[iCurrentKernel]++;
			bEnableThreadSpace = FALSE;
			bKernelFound = FALSE;
			bUpdateCurrKernel = FALSE;
			numDispatched++;
		}
	} else {
		PBYTE pBBuffer = pBatchBuffer->pData + pBatchBuffer->iCurrent;
		bUpdateCurrKernel = FALSE;

		for (i = 0; i < totalNumThreads; ++i) {
			if (!bSingleSubSlice) {
				if ((pDispatchFreq[iCurrentKernel]
				     [pState->HintIndexes.iDispatchIndexes
				      [iCurrentKernel]] == numDispatched)
				    || (pState->HintIndexes.iKernelIndexes
					[iCurrentKernel] >=
					pKernelParams
					[iCurrentKernel]->iNumThreads)) {
					numDispatched = 0;
					numStepsDispatched++;
					pState->HintIndexes.iDispatchIndexes
					    [iCurrentKernel]++;

					if (pState->HintIndexes.iKernelIndexes
					    [iCurrentKernel] >=
					    pKernelParams
					    [iCurrentKernel]->iNumThreads) {
						bUpdateCurrKernel = TRUE;
						pGroupInfo[pRemapKrnToGrp
							   [iCurrentKernel]].numKernelsFinished++;
						if (pGroupInfo
						    [pRemapKrnToGrp
						     [iCurrentKernel]].numKernelsFinished
						    ==
						    pGroupInfo[pRemapKrnToGrp
							       [iCurrentKernel]].numKernelsInGroup)
						{
							pGroupInfo
							    [pRemapKrnToGrp
							     [iCurrentKernel]].groupFinished
							    = 1;
						} else {
							pRemapGrpToKrn
							    [tmpIndex]++;
						}
					}

					if ((pGroupInfo
					     [pRemapKrnToGrp
					      [iCurrentKernel]].freqDispatch ==
					     numStepsDispatched)
					    || bUpdateCurrKernel) {
						numStepsDispatched = 0;
						iRoundRobinCount++;

						tmpIndex =
						    iRoundRobinCount %
						    numKernelGroups;

						if (pGroupInfo
						    [tmpIndex].groupFinished) {
							loopCount = 0;
							while ((loopCount <
								numKernelGroups)
							       &&
							       (!bKernelFound))
							{
								iRoundRobinCount++;
								tmpIndex =
								    iRoundRobinCount
								    %
								    numKernelGroups;
								if (pState->HintIndexes.iKernelIndexes[pRemapGrpToKrn[tmpIndex]]
								    <
								    pKernelParams
								    [pRemapGrpToKrn
								     [tmpIndex]]->iNumThreads)
								{
									bKernelFound
									    =
									    TRUE;
								}
								loopCount++;
							}
							if (!bKernelFound) {
								CM_ERROR_ASSERT
								    ("Couldn't find kernel with threads left for EnqueueWithHints");
								goto finish;
							}
						}

						iCurrentKernel =
						    pRemapGrpToKrn[tmpIndex];
					}
				}
			} else {
				if (pState->
				    HintIndexes.iKernelIndexes[iCurrentKernel]
				    >=
				    pKernelParams[iCurrentKernel]->iNumThreads)
				{
					iCurrentKernel++;
				}
			}

			if (pKernelParams
			    [iCurrentKernel]->CmKernelThreadSpaceParam.
			    pThreadCoordinates) {
				threadCoordinates.x =
				    pKernelParams
				    [iCurrentKernel]->CmKernelThreadSpaceParam.pThreadCoordinates
				    [pState->
				     HintIndexes.iKernelIndexes
				     [iCurrentKernel]].x;
				threadCoordinates.y =
				    pKernelParams
				    [iCurrentKernel]->CmKernelThreadSpaceParam.pThreadCoordinates
				    [pState->
				     HintIndexes.iKernelIndexes
				     [iCurrentKernel]].y;
				threadCoordinates.mask =
				    pKernelParams
				    [iCurrentKernel]->CmKernelThreadSpaceParam.pThreadCoordinates
				    [pState->
				     HintIndexes.iKernelIndexes
				     [iCurrentKernel]].mask;
				bEnableThreadSpace = TRUE;
			}

			pCmds[iCurrentKernel]->DW2.UseScoreboard =
			    (pKernelParams
			     [iCurrentKernel]->CmKernelThreadSpaceParam.
			     dependencyInfo.count == 0) ? 0 : 1;

			if (!bSingleSubSlice) {
				sliceIndex =
				    pKernelsSliceInfo[pRemapKrnToGrp
						      [iCurrentKernel]].counter
				    %
				    pKernelsSliceInfo[pRemapKrnToGrp
						      [iCurrentKernel]].numSubSlices;
				pCmds[iCurrentKernel]->
				    DW2.SliceDestinationSelect =
				    pKernelsSliceInfo[pRemapKrnToGrp
						      [iCurrentKernel]].pDestination
				    [sliceIndex].slice;
				pCmds[iCurrentKernel]->
				    DW2.HalfSliceDestinationSelect =
				    pKernelsSliceInfo[pRemapKrnToGrp
						      [iCurrentKernel]].pDestination
				    [sliceIndex].subSlice;

				if (pState->Platform.eRenderCoreFamily >=
				    IGFX_GEN8_CORE) {
					pCmds[iCurrentKernel]->
					    DW2.ForceDestination = 1;
				}

				pKernelsSliceInfo[pRemapKrnToGrp
						  [iCurrentKernel]].counter++;
			}

			if (bEnableThreadSpace) {
				pCmds[iCurrentKernel]->DW4.ScoreboardX =
				    threadCoordinates.x;
				pCmds[iCurrentKernel]->DW4.ScoreboardY =
				    threadCoordinates.y;
				if (threadCoordinates.mask !=
				    CM_DEFAULT_THREAD_DEPENDENCY_MASK) {
					tmpThreadScoreboardMask =
					    pKernelScoreboardMask
					    [iCurrentKernel];
					for (k = 0;
					     k <
					     pKernelParams
					     [iCurrentKernel]->CmKernelThreadSpaceParam.dependencyInfo.
					     count; ++k) {
						if ((threadCoordinates.mask &
						     CM_HINTS_LEASTBIT_MASK) ==
						    0) {
							HAL_CM_UNSETBIT
							    (tmpThreadScoreboardMask,
							     pDependRemap
							     [iCurrentKernel]
							     [k]);
						}

						threadCoordinates.mask =
						    threadCoordinates.mask >> 1;
					}

					pCmds[iCurrentKernel]->
					    DW5.ScoreboardMask =
					    tmpThreadScoreboardMask;
				} else {
					pCmds[iCurrentKernel]->
					    DW5.ScoreboardMask =
					    pKernelScoreboardMask
					    [iCurrentKernel];
				}
			} else {
				pCmds[iCurrentKernel]->DW4.ScoreboardX =
				    pState->
				    HintIndexes.iKernelIndexes[iCurrentKernel] %
				    pKernelParams
				    [iCurrentKernel]->CmKernelThreadSpaceParam.
				    iThreadSpaceWidth;
				pCmds[iCurrentKernel]->DW4.ScoreboardY =
				    pState->
				    HintIndexes.iKernelIndexes[iCurrentKernel] /
				    pKernelParams
				    [iCurrentKernel]->CmKernelThreadSpaceParam.
				    iThreadSpaceWidth;
				pCmds[iCurrentKernel]->DW5.ScoreboardMask =
				    pKernelScoreboardMask[iCurrentKernel];
			}

			iAdjustedYCoord = 0;
			if (iCurrentKernel > 0) {
				if (pKernelScoreboardMask[iCurrentKernel]) {
					if (pCmds[iCurrentKernel]->
					    DW4.ScoreboardY == 0) {
						for (k = 0;
						     k <
						     vfeDependencyInfo.count;
						     ++k) {
							if (vfeDependencyInfo.deltaY[k] < 0) {
								HAL_CM_UNSETBIT
								    (pCmds
								     [iCurrentKernel]->DW5.ScoreboardMask,
								     k);
							}
						}
					}
				}

				for (j = iCurrentKernel; j > 0; --j) {
					iAdjustedYCoord +=
					    pKernelParams[j -
							  1]->CmKernelThreadSpaceParam.iThreadSpaceHeight;
				}
			}

			if (iCurrentKernel < iNumKernels - 1) {
				if (pKernelScoreboardMask[iCurrentKernel]) {
					if (pCmds[iCurrentKernel]->
					    DW4.ScoreboardY ==
					    (pKernelParams
					     [iCurrentKernel]->CmKernelThreadSpaceParam.iThreadSpaceHeight
					     - 1)) {
						for (k = 0;
						     k <
						     vfeDependencyInfo.count;
						     ++k) {
							if (vfeDependencyInfo.deltaY[k] > 0) {
								HAL_CM_UNSETBIT
								    (pCmds
								     [iCurrentKernel]->DW5.ScoreboardMask,
								     k);
							}
						}
					}
				}
			}

			pCmds[iCurrentKernel]->DW4.ScoreboardY =
			    pCmds[iCurrentKernel]->DW4.ScoreboardY +
			    iAdjustedYCoord;

			for (aIndex = 0;
			     aIndex < pKernelParams[iCurrentKernel]->iNumArgs;
			     aIndex++) {
				pArgParams[iCurrentKernel] =
				    &pKernelParams[iCurrentKernel]->CmArgParams
				    [aIndex];
				index =
				    pState->
				    HintIndexes.iKernelIndexes[iCurrentKernel] *
				    pArgParams[iCurrentKernel]->bPerThread;

				if ((pKernelParams[iCurrentKernel]->dwCmFlags &
				     CM_KERNEL_FLAGS_CURBE)
				    && !pArgParams[iCurrentKernel]->bPerThread) {
					continue;
				}

				CM_ASSERT(pArgParams
					  [iCurrentKernel]->iPayloadOffset <
					  pKernelParams
					  [iCurrentKernel]->iPayloadSize);

				switch (pArgParams[iCurrentKernel]->Kind) {
				case CM_ARGUMENT_GENERAL:
					GENOS_SecureMemcpy(pCmd_inline
							   [iCurrentKernel] +
							   pArgParams
							   [iCurrentKernel]->iPayloadOffset,
							   pArgParams
							   [iCurrentKernel]->iUnitSize,
							   pArgParams
							   [iCurrentKernel]->pFirstValue
							   +
							   index *
							   pArgParams
							   [iCurrentKernel]->iUnitSize,
							   pArgParams
							   [iCurrentKernel]->iUnitSize);
					break;

				case CM_ARGUMENT_SURFACEBUFFER:
					CM_CHK_GENOSSTATUS
					    (HalCm_SetupBufferSurfaceState
					     (pState,
					      pArgParams[iCurrentKernel],
					      &pIndexParams[iCurrentKernel],
					      pBindingTableEntries
					      [iCurrentKernel], -1, index,
					      pCmd_inline[iCurrentKernel]));
					break;

				case CM_ARGUMENT_SURFACE2D_UP:
					CM_CHK_GENOSSTATUS
					    (HalCm_Setup2DSurfaceUPState
					     (pState,
					      pArgParams[iCurrentKernel],
					      &pIndexParams[iCurrentKernel],
					      pBindingTableEntries
					      [iCurrentKernel], index,
					      pCmd_inline[iCurrentKernel]));
					break;

				case CM_ARGUMENT_SURFACE2D:
					CM_CHK_GENOSSTATUS
					    (HalCm_Setup2DSurfaceState
					     (pState,
					      pArgParams[iCurrentKernel],
					      &pIndexParams[iCurrentKernel],
					      pBindingTableEntries
					      [iCurrentKernel], index,
					      pCmd_inline[iCurrentKernel]));
					break;

				default:
					hr = GENOS_STATUS_UNKNOWN;
					CM_ERROR_ASSERT
					    ("Argument kind '%d' is not supported",
					     pArgParams[iCurrentKernel]->Kind);
					goto finish;
				}
			}

			GENOS_SecureMemcpy(pBBuffer, pCmd_sizes[iCurrentKernel],
					   pCmd_data[iCurrentKernel],
					   pCmd_sizes[iCurrentKernel]);
			pBBuffer += pCmd_sizes[iCurrentKernel];
			pBatchBuffer->iCurrent += pCmd_sizes[iCurrentKernel];

			pState->HintIndexes.iKernelIndexes[iCurrentKernel]++;
			bEnableThreadSpace = FALSE;
			bKernelFound = FALSE;
			bUpdateCurrKernel = FALSE;
			numDispatched++;
		}
	}

	for (j = 0; j < iNumKernels; ++j) {
		for (i = 0; i < CM_MAX_GLOBAL_SURFACE_NUMBER; ++i) {
			if ((pKernelParams[j]->globalSurface[i] &
			     CM_SURFACE_MASK) != CM_NULL_SURFACE) {
				CM_HAL_KERNEL_ARG_PARAM ArgParam;
				pArgParam = &ArgParam;

				ArgParam.Kind = CM_ARGUMENT_SURFACEBUFFER;
				ArgParam.iPayloadOffset = 0;
				ArgParam.iUnitCount = 1;
				ArgParam.iUnitSize = sizeof(DWORD);
				ArgParam.bPerThread = FALSE;
				ArgParam.pFirstValue =
				    (PBYTE) &
				    pKernelParams[j]->globalSurface[i];

				CM_CHK_GENOSSTATUS(HalCm_SetupBufferSurfaceState
						   (pState, pArgParam,
						    &pIndexParams[j],
						    pBindingTableEntries[j],
						    (SHORT) i, 0, NULL));
			}
		}

	}

	if (numTasks <= 1 || lastTask) {
		for (i = 0; i < iNumKernels; ++i) {
			if (pState->HintIndexes.iKernelIndexes[i] <
			    pKernelParams[i]->iNumThreads) {
				CM_ERROR_ASSERT
				    ("Not all threads for all kernels were put into batch buffer");
				goto finish;
			}
		}
	}

	if (lastTask) {
		GENOS_ZeroMemory(&pState->HintIndexes.iKernelIndexes,
				 sizeof(UINT) * CM_MAX_TASKS_EU_SATURATION);
		GENOS_ZeroMemory(&pState->HintIndexes.iDispatchIndexes,
				 sizeof(UINT) * CM_MAX_TASKS_EU_SATURATION);
	}

 finish:
	if (pMediaObjectParams)
		GENOS_FreeMemory(pMediaObjectParams);
	if (pKernelParams)
		GENOS_FreeMemory(pKernelParams);
	if (pArgParams)
		GENOS_FreeMemory(pArgParams);
	if (pCmds)
		GENOS_FreeMemory(pCmds);
	if (pCmd_inline)
		GENOS_FreeMemory(pCmd_inline);
	if (pCmd_sizes)
		GENOS_FreeMemory(pCmd_sizes);
	if (pRemapKrnToGrp)
		GENOS_FreeMemory(pRemapKrnToGrp);
	if (pRemapGrpToKrn)
		GENOS_FreeMemory(pRemapGrpToKrn);
	if (pKernelScoreboardMask)
		GENOS_FreeMemory(pKernelScoreboardMask);
	if (pParallelGraphInfo)
		GENOS_FreeMemory(pParallelGraphInfo);
	if (pNumKernelsPerGrp)
		GENOS_FreeMemory(pNumKernelsPerGrp);
	if (pGroupInfo)
		GENOS_FreeMemory(pGroupInfo);

	if (pCmd_data) {
		for (i = 0; i < iNumKernels; ++i) {
			if (pCmd_data[i])
				GENOS_FreeMemory(pCmd_data[i]);
		}
		GENOS_FreeMemory(pCmd_data);
	}

	if (pKernelsSliceInfo) {
		for (i = 0; i < numKernelGroups; ++i) {
			if (pKernelsSliceInfo[i].pDestination)
				GENOS_FreeMemory(pKernelsSliceInfo
						 [i].pDestination);
		}
		GENOS_FreeMemory(pKernelsSliceInfo);
	}

	if (pDependRemap) {
		for (i = 0; i < iNumKernels; ++i) {
			if (pDependRemap[i])
				GENOS_FreeMemory(pDependRemap[i]);
		}
		GENOS_FreeMemory(pDependRemap);
	}

	if (pDispatchFreq) {
		for (i = 0; i < iNumKernels; ++i) {
			if (pDispatchFreq[i])
				GENOS_FreeMemory(pDispatchFreq[i]);
		}
		GENOS_FreeMemory(pDispatchFreq);
	}

	return hr;
}

GENOS_STATUS HalCm_SetupStatesForKernelInitial(PCM_HAL_STATE pState,
					       PGENHW_BATCH_BUFFER pBatchBuffer,
					       INT iTaskId,
					       PCM_HAL_KERNEL_PARAM
					       pKernelParam,
					       PCM_HAL_INDEX_PARAM pIndexParam,
					       UINT iKernelCurbeOffset,
					       INT & iBindingTable,
					       INT & iMediaID, INT & iKAID)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	PGENHW_HW_INTERFACE pHwInterface = pState->pHwInterface;
	PCM_INDIRECT_SURFACE_INFO pIndirectSurfaceInfo =
	    pKernelParam->CmIndirectDataParam.pSurfaceInfo;
	GENHW_GPGPU_WALKER_PARAMS GpGpuWalkerParams = { 0 };
	PCM_GPGPU_WALKER_PARAMS pPerKernelGpGpuWalkerParames =
	    &pKernelParam->GpGpuWalkerParams;
	PGENHW_GSH pGsh = pHwInterface->pGeneralStateHeap;

	PCM_HAL_KERNEL_ARG_PARAM pArgParam;
	UINT index;
	UINT value;
	UINT btIndex;
	UINT surfIndex;
	UINT aIndex;
	UINT id_y;
	UINT id_x;

	if (pPerKernelGpGpuWalkerParames->CmGpGpuEnable) {
		GpGpuWalkerParams.GpGpuEnable = TRUE;
		GpGpuWalkerParams.ThreadWidth =
		    pPerKernelGpGpuWalkerParames->ThreadWidth;
		GpGpuWalkerParams.ThreadHeight =
		    pPerKernelGpGpuWalkerParames->ThreadHeight;
		GpGpuWalkerParams.GroupWidth =
		    pPerKernelGpGpuWalkerParames->GroupWidth;
		GpGpuWalkerParams.GroupHeight =
		    pPerKernelGpGpuWalkerParames->GroupHeight;
		GpGpuWalkerParams.SLMSize =
		    pPerKernelGpGpuWalkerParames->SLMSize;
	}

	Halcm_PreSetBindingIndex(pIndexParam, CM_NULL_SURFACE_BINDING_INDEX,
				 CM_NULL_SURFACE_BINDING_INDEX);
	Halcm_PreSetBindingIndex(pIndexParam,
				 CM_BINDING_START_INDEX_OF_GLOBAL_SURFACE
				 (pState),
				 CM_BINDING_START_INDEX_OF_GLOBAL_SURFACE
				 (pState) + CM_MAX_GLOBAL_SURFACE_NUMBER - 1);
	if (pKernelParam->CmIndirectDataParam.iSurfaceCount) {
		for (index = 0;
		     index < pKernelParam->CmIndirectDataParam.iSurfaceCount;
		     index++) {
			value =
			    (pIndirectSurfaceInfo + index)->iBindingTableIndex;
			Halcm_PreSetBindingIndex(pIndexParam, value, value);
		}
	}

	GENOS_FillMemory(pState->pBT2DIndexTable,
			 pState->CmDeviceParam.iMax2DSurfaceTableSize *
			 sizeof(CM_HAL_MULTI_USE_BTI_ENTRY), CM_INVALID_INDEX);

	GENOS_FillMemory(pState->pBT2DUPIndexTable,
			 pState->CmDeviceParam.iMax2DSurfaceUPTableSize *
			 sizeof(CM_HAL_MULTI_USE_BTI_ENTRY), CM_INVALID_INDEX);

	GENOS_FillMemory(pState->pBTBufferIndexTable,
			 pState->CmDeviceParam.iMaxBufferTableSize *
			 sizeof(CM_HAL_MULTI_USE_BTI_ENTRY), CM_INVALID_INDEX);

	CM_CHK_GENOSSTATUS(pHwInterface->pfnAssignBindingTable
			   (pHwInterface, &iBindingTable));

	CM_CHK_GENOSSTATUS(HalCm_LoadKernel(pState, pKernelParam, &iKAID));

	if (pKernelParam->iKrnCurbeSize > 0) {
		pGsh->pCurMediaState->iCurbeOffset +=
		    GENOS_ALIGN_CEIL(pKernelParam->iKrnCurbeSize,
				     pState->pfnGetCurbeBlockAlignSize());
	}
	iMediaID = HalCm_AllocateMediaID(pHwInterface,
					 iKAID,
					 iBindingTable,
					 iKernelCurbeOffset,
					 pKernelParam->iCurbeSizePerThread,
					 pKernelParam->iCrsThrdConstDataLn,
					 &GpGpuWalkerParams);

	if (iMediaID < 0) {
		CM_ERROR_ASSERT("Unable to get Media ID");
		goto finish;
	}
	if (pKernelParam->CmIndirectDataParam.iSurfaceCount) {
		for (index = 0;
		     index < pKernelParam->CmIndirectDataParam.iSurfaceCount;
		     index++) {
			btIndex =
			    (pIndirectSurfaceInfo + index)->iBindingTableIndex;
			surfIndex =
			    (pIndirectSurfaceInfo + index)->iSurfaceIndex;
			switch ((pIndirectSurfaceInfo + index)->iKind) {
			case CM_ARGUMENT_SURFACEBUFFER:
				CM_CHK_GENOSSTATUS
				    (HalCm_SetupBufferSurfaceStateWithBTIndex
				     (pState, iBindingTable, surfIndex,
				      btIndex));
				break;

			case CM_ARGUMENT_SURFACE2D:
				CM_CHK_GENOSSTATUS
				    (HalCm_Setup2DSurfaceStateWithBTIndex
				     (pState, iBindingTable, surfIndex,
				      btIndex));
				break;

			case CM_ARGUMENT_SURFACE2D_UP:
				CM_CHK_GENOSSTATUS
				    (HalCm_Setup2DSurfaceUPStateWithBTIndex
				     (pState, iBindingTable, surfIndex,
				      btIndex));
				break;

			default:
				CM_ERROR_ASSERT
				    ("Indirect Data Surface kind is not supported");
				goto finish;
			}
		}
	}

	if (pKernelParam->iCurbeSizePerThread > 0) {
		BYTE data[CM_MAX_THREAD_PAYLOAD_SIZE + 32];
		BYTE curbe[CM_MAX_CURBE_SIZE_PER_TASK + 32];
		for (aIndex = 0; aIndex < pKernelParam->iNumArgs; aIndex++) {
			pArgParam = &pKernelParam->CmArgParams[aIndex];

			if (pArgParam->bPerThread) {
				continue;
			}

			switch (pArgParam->Kind) {
			case CM_ARGUMENT_GENERAL:
				HalCm_SetArgData(pArgParam, 0, data);
				break;

			case CM_ARGUMENT_SURFACEBUFFER:
				CM_CHK_GENOSSTATUS(HalCm_SetupBufferSurfaceState
						   (pState, pArgParam,
						    pIndexParam, iBindingTable,
						    -1, 0, data));
				break;

			case CM_ARGUMENT_SURFACE2D_UP:
				CM_CHK_GENOSSTATUS(HalCm_Setup2DSurfaceUPState
						   (pState, pArgParam,
						    pIndexParam, iBindingTable,
						    0, data));
				break;

			case CM_ARGUMENT_SURFACE2D:
				CM_CHK_GENOSSTATUS(HalCm_Setup2DSurfaceState
						   (pState, pArgParam,
						    pIndexParam, iBindingTable,
						    0, data));
				break;

			default:
				CM_ERROR_ASSERT
				    ("Argument kind '%d' is not supported",
				     pArgParam->Kind);
				goto finish;
			}
		}

		if (pPerKernelGpGpuWalkerParames->CmGpGpuEnable) {
			DWORD offset = 0;
			DWORD local_id_x_offset =
			    pKernelParam->CmArgParams[pKernelParam->iNumArgs -
						      2].iPayloadOffset;
			DWORD local_id_y_offset =
			    pKernelParam->CmArgParams[pKernelParam->iNumArgs -
						      1].iPayloadOffset;

			INT crossThreadSize = pKernelParam->iCrsThrdConstDataLn;

			GENOS_SecureMemcpy(curbe + offset, crossThreadSize,
					   data, crossThreadSize);
			offset += crossThreadSize;

			for (id_y = 0;
			     id_y < pPerKernelGpGpuWalkerParames->ThreadHeight;
			     id_y++) {
				for (id_x = 0;
				     id_x <
				     pPerKernelGpGpuWalkerParames->ThreadWidth;
				     id_x++) {
					*((DWORD *) (data +
						     local_id_x_offset)) = id_x;
					*((DWORD *) (data +
						     local_id_y_offset)) = id_y;
					GENOS_SecureMemcpy(curbe + offset,
							   pKernelParam->iCurbeSizePerThread,
							   data +
							   crossThreadSize,
							   pKernelParam->iCurbeSizePerThread);
					offset +=
					    pKernelParam->iCurbeSizePerThread;
				}
			}

			pGsh->pCurMediaState->iCurbeOffset -=
			    GENOS_ALIGN_CEIL(pKernelParam->iKrnCurbeSize,
					     pState->pfnGetCurbeBlockAlignSize
					     ());
			pHwInterface->pfnLoadCurbeData(pHwInterface,
						       pGsh->pCurMediaState,
						       curbe,
						       pKernelParam->iKrnCurbeSize);
		} else {
			CM_ASSERT(pKernelParam->iKrnCurbeSize ==
				  pKernelParam->iCurbeSizePerThread);

			pGsh->pCurMediaState->iCurbeOffset -=
			    GENOS_ALIGN_CEIL(pKernelParam->iKrnCurbeSize,
					     pState->pfnGetCurbeBlockAlignSize
					     ());
			pHwInterface->pfnLoadCurbeData(pHwInterface,
						       pGsh->pCurMediaState,
						       data,
						       pKernelParam->iKrnCurbeSize);
		}
	}

 finish:
	return hr;
}

GENOS_STATUS HalCm_FinishStatesForKernel(PCM_HAL_STATE pState,
					 PGENHW_BATCH_BUFFER pBatchBuffer,
					 INT iTaskId,
					 PCM_HAL_KERNEL_PARAM pKernelParam,
					 INT iKernelIndex,
					 PCM_HAL_INDEX_PARAM pIndexParam,
					 INT iBindingTable,
					 INT iMediaID, INT iKAID)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	PCM_HAL_TASK_PARAM pTaskParam = pState->pTaskParam;
	PGENHW_HW_INTERFACE pHwInterface = pState->pHwInterface;
	PCM_HAL_WALKER_PARAMS pWalkerParams = &pKernelParam->WalkerParams;
	PCM_GPGPU_WALKER_PARAMS pPerKernelGpGpuWalkerParams =
	    &pKernelParam->GpGpuWalkerParams;
	PCM_HAL_SCOREBOARD_XY pThreadCoordinates = NULL;
	PCM_HAL_MASK_AND_RESET pDependencyMask = NULL;
	bool enableThreadSpace = FALSE;
	bool enableKernelThreadSpace = FALSE;
	PCM_HAL_SCOREBOARD_XY_MASK pKernelThreadCoordinates = NULL;

	GENHW_HW_MEDIAOBJECT_PARAM MediaObjectParam;
	PCM_HAL_KERNEL_ARG_PARAM pArgParam;
	GENHW_PIPECONTROL_PARAM PipeControlParam;
	UINT i;
	UINT iHdrSize;
	UINT aIndex;
	UINT tIndex;
	UINT index;

	pTaskParam->iCurKrnIndex = iKernelIndex;

	if (pTaskParam->ppThreadCoordinates) {
		pThreadCoordinates =
		    pTaskParam->ppThreadCoordinates[iKernelIndex];
		if (pThreadCoordinates) {
			enableThreadSpace = TRUE;
		}
	} else if (pKernelParam->CmKernelThreadSpaceParam.pThreadCoordinates) {
		pKernelThreadCoordinates =
		    pKernelParam->CmKernelThreadSpaceParam.pThreadCoordinates;
		if (pKernelThreadCoordinates) {
			enableKernelThreadSpace = TRUE;
		}
	}

	if (pTaskParam->ppDependencyMasks) {
		pDependencyMask = pTaskParam->ppDependencyMasks[iKernelIndex];
	}
	pWalkerParams->CmWalkerEnable = pState->WalkerParams.CmWalkerEnable;

	iHdrSize = pHwInterface->pHwCommands->dwMediaObjectHeaderCmdSize;
	MediaObjectParam.dwIDOffset = iMediaID;
	if (pKernelParam->CmIndirectDataParam.iIndirectDataSize) {
		MediaObjectParam.dwMediaObjectSize = iHdrSize;
	} else {
		MediaObjectParam.dwMediaObjectSize =
		    iHdrSize + MAX(pKernelParam->iPayloadSize, 4);
	}

	if (pPerKernelGpGpuWalkerParams->CmGpGpuEnable) {
		pPerKernelGpGpuWalkerParams->InterfaceDescriptorOffset =
		    MediaObjectParam.dwIDOffset;
	} else if (pWalkerParams->CmWalkerEnable) {
		CM_HAL_KERNEL_THREADSPACE_PARAM kernelThreadSpace;
		if (pKernelParam->CmKernelThreadSpaceParam.iThreadSpaceWidth) {
			kernelThreadSpace.iThreadSpaceWidth =
			    pKernelParam->
			    CmKernelThreadSpaceParam.iThreadSpaceWidth;
			kernelThreadSpace.iThreadSpaceHeight =
			    pKernelParam->
			    CmKernelThreadSpaceParam.iThreadSpaceHeight;
			kernelThreadSpace.patternType =
			    pKernelParam->CmKernelThreadSpaceParam.patternType;
		} else {
			kernelThreadSpace.iThreadSpaceWidth =
			    (WORD) pTaskParam->threadSpaceWidth;
			kernelThreadSpace.iThreadSpaceHeight =
			    (WORD) pTaskParam->threadSpaceHeight;
			kernelThreadSpace.patternType =
			    pTaskParam->DependencyPattern;
		}

		pWalkerParams->InterfaceDescriptorOffset =
		    MediaObjectParam.dwIDOffset;
		pWalkerParams->InlineDataLength =
		    GENOS_ALIGN_CEIL(pKernelParam->
				     CmIndirectDataParam.iIndirectDataSize, 4);
		pWalkerParams->pInlineData =
		    pKernelParam->CmIndirectDataParam.pIndirectData;

		pWalkerParams->ColorCountMinusOne =
		    pTaskParam->ColorCountMinusOne;

		CM_HAL_WALKING_PATTERN walkPattern = pTaskParam->WalkingPattern;
		switch (kernelThreadSpace.patternType) {
		case CM_DEPENDENCY_NONE:
			break;
		case CM_DEPENDENCY_HORIZONTAL:
			walkPattern = CM_WALK_HORIZONTAL;
			break;
		case CM_DEPENDENCY_VERTICAL:
			walkPattern = CM_WALK_VERTICAL;
			break;
		case CM_DEPENDENCY_WAVEFRONT:
			walkPattern = CM_WALK_WAVEFRONT;
			break;
		case CM_DEPENDENCY_WAVEFRONT26:
			walkPattern = CM_WALK_WAVEFRONT26;
			break;
		default:
			CM_ASSERT(0);
			walkPattern = CM_WALK_DEFAULT;
			break;
		}
		if (pTaskParam->walkingParamsValid) {
			MEDIA_OBJECT_WALKER_CMD_G6 tmpMWCmd;
			tmpMWCmd.DW5.Value = pTaskParam->walkingParams.Value[0];
			pWalkerParams->ScoreboardMask =
			    tmpMWCmd.DW5.ScoreboardMask;

			tmpMWCmd.DW6.Value = pTaskParam->walkingParams.Value[1];
			pWalkerParams->ColorCountMinusOne =
			    tmpMWCmd.DW6.ColorCountMinusOne;
			pWalkerParams->MidLoopUnitX = tmpMWCmd.DW6.MidLoopUnitX;
			pWalkerParams->MidLoopUnitY = tmpMWCmd.DW6.MidLoopUnitY;
			pWalkerParams->MiddleLoopExtraSteps =
			    tmpMWCmd.DW6.MidLoopExtraSteps;

			tmpMWCmd.DW7.Value = pTaskParam->walkingParams.Value[2];
			pWalkerParams->LoopExecCount.x =
			    tmpMWCmd.DW7.LocalLoopExecCount;
			pWalkerParams->LoopExecCount.y =
			    tmpMWCmd.DW7.GlobalLoopExecCount;

			tmpMWCmd.DW8.Value = pTaskParam->walkingParams.Value[3];
			pWalkerParams->BlockResolution.x =
			    tmpMWCmd.DW8.BlockResolutionX;
			pWalkerParams->BlockResolution.y =
			    tmpMWCmd.DW8.BlockResolutionY;

			tmpMWCmd.DW9.Value = pTaskParam->walkingParams.Value[4];
			pWalkerParams->LocalStart.x = tmpMWCmd.DW9.LocalStartX;
			pWalkerParams->LocalStart.y = tmpMWCmd.DW9.LocalStartY;

			tmpMWCmd.DW11.Value =
			    pTaskParam->walkingParams.Value[6];
			pWalkerParams->LocalOutLoopStride.x =
			    tmpMWCmd.DW11.LocalOuterLoopStrideX;
			pWalkerParams->LocalOutLoopStride.y =
			    tmpMWCmd.DW11.LocalOuterLoopStrideY;

			tmpMWCmd.DW12.Value =
			    pTaskParam->walkingParams.Value[7];
			pWalkerParams->LocalInnerLoopUnit.x =
			    tmpMWCmd.DW12.LocalInnerLoopUnitX;
			pWalkerParams->LocalInnerLoopUnit.y =
			    tmpMWCmd.DW12.LocalInnerLoopUnitY;

			tmpMWCmd.DW13.Value =
			    pTaskParam->walkingParams.Value[8];
			pWalkerParams->GlobalResolution.x =
			    tmpMWCmd.DW13.GlobalResolutionX;
			pWalkerParams->GlobalResolution.y =
			    tmpMWCmd.DW13.GlobalResolutionY;

			tmpMWCmd.DW14.Value =
			    pTaskParam->walkingParams.Value[9];
			pWalkerParams->GlobalStart.x =
			    tmpMWCmd.DW14.GlobalStartX;
			pWalkerParams->GlobalStart.y =
			    tmpMWCmd.DW14.GlobalStartY;

			tmpMWCmd.DW15.Value =
			    pTaskParam->walkingParams.Value[10];
			pWalkerParams->GlobalOutlerLoopStride.x =
			    tmpMWCmd.DW15.GlobalOuterLoopStrideX;
			pWalkerParams->GlobalOutlerLoopStride.y =
			    tmpMWCmd.DW15.GlobalOuterLoopStrideY;

			tmpMWCmd.DW16.Value =
			    pTaskParam->walkingParams.Value[11];
			pWalkerParams->GlobalInnerLoopUnit.x =
			    tmpMWCmd.DW16.GlobalInnerLoopUnitX;
			pWalkerParams->GlobalInnerLoopUnit.y =
			    tmpMWCmd.DW16.GlobalInnerLoopUnitY;

			pWalkerParams->LocalEnd.x = 0;
			pWalkerParams->LocalEnd.y = 0;

			if (walkPattern == CM_WALK_HORIZONTAL
			    || walkPattern == CM_WALK_DEFAULT) {
				pWalkerParams->LocalEnd.x =
				    pWalkerParams->BlockResolution.x - 1;
			} else if (walkPattern == CM_WALK_VERTICAL) {
				pWalkerParams->LocalEnd.y =
				    pWalkerParams->BlockResolution.y - 1;
			}
		} else if (pKernelParam->
			   CmKernelThreadSpaceParam.walkingParamsValid) {
			MEDIA_OBJECT_WALKER_CMD_G6 tmpMWCmd;
			tmpMWCmd.DW5.Value =
			    pKernelParam->
			    CmKernelThreadSpaceParam.walkingParams.Value[0];
			pWalkerParams->ScoreboardMask =
			    tmpMWCmd.DW5.ScoreboardMask;

			tmpMWCmd.DW6.Value =
			    pKernelParam->
			    CmKernelThreadSpaceParam.walkingParams.Value[1];
			pWalkerParams->ColorCountMinusOne =
			    tmpMWCmd.DW6.ColorCountMinusOne;
			pWalkerParams->MidLoopUnitX = tmpMWCmd.DW6.MidLoopUnitX;
			pWalkerParams->MidLoopUnitY = tmpMWCmd.DW6.MidLoopUnitY;
			pWalkerParams->MiddleLoopExtraSteps =
			    tmpMWCmd.DW6.MidLoopExtraSteps;

			tmpMWCmd.DW7.Value =
			    pKernelParam->
			    CmKernelThreadSpaceParam.walkingParams.Value[2];
			pWalkerParams->LoopExecCount.x =
			    tmpMWCmd.DW7.LocalLoopExecCount;
			pWalkerParams->LoopExecCount.y =
			    tmpMWCmd.DW7.GlobalLoopExecCount;

			tmpMWCmd.DW8.Value =
			    pKernelParam->
			    CmKernelThreadSpaceParam.walkingParams.Value[3];
			pWalkerParams->BlockResolution.x =
			    tmpMWCmd.DW8.BlockResolutionX;
			pWalkerParams->BlockResolution.y =
			    tmpMWCmd.DW8.BlockResolutionY;

			tmpMWCmd.DW9.Value =
			    pKernelParam->
			    CmKernelThreadSpaceParam.walkingParams.Value[4];
			pWalkerParams->LocalStart.x = tmpMWCmd.DW9.LocalStartX;
			pWalkerParams->LocalStart.y = tmpMWCmd.DW9.LocalStartY;

			tmpMWCmd.DW10.Value =
			    pKernelParam->
			    CmKernelThreadSpaceParam.walkingParams.Value[5];

			tmpMWCmd.DW11.Value =
			    pKernelParam->
			    CmKernelThreadSpaceParam.walkingParams.Value[6];
			pWalkerParams->LocalOutLoopStride.x =
			    tmpMWCmd.DW11.LocalOuterLoopStrideX;
			pWalkerParams->LocalOutLoopStride.y =
			    tmpMWCmd.DW11.LocalOuterLoopStrideY;

			tmpMWCmd.DW12.Value =
			    pKernelParam->
			    CmKernelThreadSpaceParam.walkingParams.Value[7];
			pWalkerParams->LocalInnerLoopUnit.x =
			    tmpMWCmd.DW12.LocalInnerLoopUnitX;
			pWalkerParams->LocalInnerLoopUnit.y =
			    tmpMWCmd.DW12.LocalInnerLoopUnitY;

			tmpMWCmd.DW13.Value =
			    pKernelParam->
			    CmKernelThreadSpaceParam.walkingParams.Value[8];
			pWalkerParams->GlobalResolution.x =
			    tmpMWCmd.DW13.GlobalResolutionX;
			pWalkerParams->GlobalResolution.y =
			    tmpMWCmd.DW13.GlobalResolutionY;

			tmpMWCmd.DW14.Value =
			    pKernelParam->
			    CmKernelThreadSpaceParam.walkingParams.Value[9];
			pWalkerParams->GlobalStart.x =
			    tmpMWCmd.DW14.GlobalStartX;
			pWalkerParams->GlobalStart.y =
			    tmpMWCmd.DW14.GlobalStartY;

			tmpMWCmd.DW15.Value =
			    pKernelParam->
			    CmKernelThreadSpaceParam.walkingParams.Value[10];
			pWalkerParams->GlobalOutlerLoopStride.x =
			    tmpMWCmd.DW15.GlobalOuterLoopStrideX;
			pWalkerParams->GlobalOutlerLoopStride.y =
			    tmpMWCmd.DW15.GlobalOuterLoopStrideY;

			tmpMWCmd.DW16.Value =
			    pKernelParam->
			    CmKernelThreadSpaceParam.walkingParams.Value[11];
			pWalkerParams->GlobalInnerLoopUnit.x =
			    tmpMWCmd.DW16.GlobalInnerLoopUnitX;
			pWalkerParams->GlobalInnerLoopUnit.y =
			    tmpMWCmd.DW16.GlobalInnerLoopUnitY;

			pWalkerParams->LocalEnd.x = 0;
			pWalkerParams->LocalEnd.y = 0;

			if (walkPattern == CM_WALK_HORIZONTAL
			    || walkPattern == CM_WALK_DEFAULT) {
				pWalkerParams->LocalEnd.x =
				    pWalkerParams->BlockResolution.x - 1;
			} else if (walkPattern == CM_WALK_VERTICAL) {
				pWalkerParams->LocalEnd.y =
				    pWalkerParams->BlockResolution.y - 1;
			}

		} else {
			pWalkerParams->BlockResolution.x =
			    kernelThreadSpace.iThreadSpaceWidth;
			pWalkerParams->BlockResolution.y =
			    kernelThreadSpace.iThreadSpaceHeight;

			pWalkerParams->LocalStart.x = 0;
			pWalkerParams->LocalStart.y = 0;
			pWalkerParams->LocalEnd.x = 0;
			pWalkerParams->LocalEnd.y = 0;

			switch (walkPattern) {
			case CM_WALK_DEFAULT:
			case CM_WALK_HORIZONTAL:
				if (kernelThreadSpace.iThreadSpaceWidth ==
				    pKernelParam->iNumThreads
				    && kernelThreadSpace.iThreadSpaceHeight ==
				    1) {
					pWalkerParams->BlockResolution.x =
					    MIN(pKernelParam->iNumThreads,
						CM_MAX_THREAD_WIDTH);
					pWalkerParams->BlockResolution.y =
					    1 +
					    pKernelParam->iNumThreads /
					    CM_MAX_THREAD_WIDTH;
				}
				pWalkerParams->LoopExecCount.x =
				    pWalkerParams->BlockResolution.y - 1;

				pWalkerParams->LocalOutLoopStride.x = 0;
				pWalkerParams->LocalOutLoopStride.y = 1;
				pWalkerParams->LocalInnerLoopUnit.x = 1;
				pWalkerParams->LocalInnerLoopUnit.y = 0;

				pWalkerParams->LocalEnd.x =
				    pWalkerParams->BlockResolution.x - 1;

				break;

			case CM_WALK_WAVEFRONT:
				pWalkerParams->LoopExecCount.x =
				    kernelThreadSpace.iThreadSpaceWidth +
				    (kernelThreadSpace.iThreadSpaceHeight -
				     1) * 1 - 1;

				pWalkerParams->LocalOutLoopStride.x = 1;
				pWalkerParams->LocalOutLoopStride.y = 0;
				pWalkerParams->LocalInnerLoopUnit.x = 0xFFFF;
				pWalkerParams->LocalInnerLoopUnit.y = 1;
				break;

			case CM_WALK_WAVEFRONT26:
				pWalkerParams->LoopExecCount.x =
				    kernelThreadSpace.iThreadSpaceWidth +
				    (kernelThreadSpace.iThreadSpaceHeight -
				     1) * 2 - 1;

				pWalkerParams->LocalOutLoopStride.x = 1;
				pWalkerParams->LocalOutLoopStride.y = 0;
				pWalkerParams->LocalInnerLoopUnit.x = 0xFFFE;	// -2 in DWORD:16
				pWalkerParams->LocalInnerLoopUnit.y = 1;
				break;

			case CM_WALK_VERTICAL:
				pWalkerParams->LoopExecCount.x =
				    pWalkerParams->BlockResolution.x - 1;

				pWalkerParams->LocalOutLoopStride.x = 1;
				pWalkerParams->LocalOutLoopStride.y = 0;
				pWalkerParams->LocalInnerLoopUnit.x = 0;
				pWalkerParams->LocalInnerLoopUnit.y = 1;

				pWalkerParams->LocalEnd.y =
				    pWalkerParams->BlockResolution.y - 1;

				break;

			default:
				CM_ASSERT(1);
				pWalkerParams->LoopExecCount.x =
				    MIN(pKernelParam->iNumThreads, 0x3FF);

				pWalkerParams->LocalOutLoopStride.x = 0;
				pWalkerParams->LocalOutLoopStride.y = 1;
				pWalkerParams->LocalInnerLoopUnit.x = 1;
				pWalkerParams->LocalInnerLoopUnit.y = 0;
				break;
			}

			pWalkerParams->LoopExecCount.y = 1;
			pWalkerParams->GlobalStart.x = 0;
			pWalkerParams->GlobalStart.y = 0;
			pWalkerParams->GlobalResolution.x =
			    pWalkerParams->BlockResolution.x;
			pWalkerParams->GlobalResolution.y =
			    pWalkerParams->BlockResolution.y;
			pWalkerParams->GlobalOutlerLoopStride.x =
			    pWalkerParams->GlobalResolution.x;
			pWalkerParams->GlobalOutlerLoopStride.y = 0;
			pWalkerParams->GlobalInnerLoopUnit.x = 0;
			pWalkerParams->GlobalInnerLoopUnit.y =
			    pWalkerParams->GlobalResolution.y;
		}
	} else {
		CM_CHK_NULL_RETURN_GENOSSTATUS(pBatchBuffer);

		BYTE cmd_data[CM_MAX_THREAD_PAYLOAD_SIZE +
			      sizeof(MEDIA_OBJECT_HEADER_G6)];
		PBYTE pCmd_inline = cmd_data + iHdrSize;
		PMEDIA_OBJECT_HEADER_G6 pCmd =
		    (PMEDIA_OBJECT_HEADER_G6) cmd_data;
		DWORD cmd_size = MediaObjectParam.dwMediaObjectSize;
		*pCmd = g_cInit_MEDIA_OBJECT_HEADER_G6;
		pCmd->DW0.DWordLength =
		    OP_LENGTH(SIZE_IN_DW(MediaObjectParam.dwMediaObjectSize));
		pCmd->DW1.InterfaceDescriptorOffset =
		    MediaObjectParam.dwIDOffset;

		if (pBatchBuffer->pBBRenderData->BbArgs.BbCmArgs.uiRefCount > 1) {
			PBYTE pBBuffer =
			    pBatchBuffer->pData + pBatchBuffer->iCurrent;
			for (aIndex = 0; aIndex < pKernelParam->iNumArgs;
			     aIndex++) {
				pArgParam = &pKernelParam->CmArgParams[aIndex];
				if ((pKernelParam->dwCmFlags &
				     CM_KERNEL_FLAGS_CURBE)
				    && !pArgParam->bPerThread) {
					continue;
				}

				for (tIndex = 0;
				     tIndex < pKernelParam->iNumThreads;
				     tIndex++) {
					index = tIndex * pArgParam->bPerThread;
					CM_ASSERT(pArgParam->iPayloadOffset <
						  pKernelParam->iPayloadSize);

					switch (pArgParam->Kind) {
					case CM_ARGUMENT_GENERAL:
						break;

					case CM_ARGUMENT_SURFACEBUFFER:
						CM_CHK_GENOSSTATUS
						    (HalCm_SetupBufferSurfaceState
						     (pState, pArgParam,
						      pIndexParam,
						      iBindingTable, -1, index,
						      NULL));
						break;

					case CM_ARGUMENT_SURFACE2D_UP:
						CM_CHK_GENOSSTATUS
						    (HalCm_Setup2DSurfaceUPState
						     (pState, pArgParam,
						      pIndexParam,
						      iBindingTable, index,
						      NULL));
						break;

					case CM_ARGUMENT_SURFACE2D:
						CM_CHK_GENOSSTATUS
						    (HalCm_Setup2DSurfaceState
						     (pState, pArgParam,
						      pIndexParam,
						      iBindingTable, index,
						      NULL));
						break;

					default:
						CM_ERROR_ASSERT
						    ("Argument kind '%d' is not supported",
						     pArgParam->Kind);
						goto finish;
					}
				}

				if (pDependencyMask) {
					if (pDependencyMask[tIndex].resetMask ==
					    CM_RESET_DEPENDENCY_MASK) {
						GENOS_SecureMemcpy(pBBuffer +
								   (CM_SCOREBOARD_MASK_POS_IN_MEDIA_OBJECT_CMD
								    *
								    sizeof
								    (DWORD)),
								   sizeof(BYTE),
								   &pDependencyMask
								   [tIndex].mask,
								   sizeof
								   (BYTE));
					}
				}
				pBatchBuffer->iCurrent += cmd_size;
				pBBuffer += cmd_size;
			}
		} else {
			if ((iKernelIndex > 0)
			    &&
			    ((pTaskParam->uiSyncBitmap &
			      ((UINT64) 1 << (iKernelIndex - 1)))
			     || (pKernelParam->
				 CmKernelThreadSpaceParam.patternType !=
				 CM_DEPENDENCY_NONE))) {
				PipeControlParam =
				    *pHwInterface->
				    pHwCommands->pcPipeControlParam;
				PipeControlParam.Operation =
				    GFX3DCONTROLOP_NOWRITE;
				PipeControlParam.pOsResource = NULL;
				PipeControlParam.dwCSStall = TRUE;
				PipeControlParam.dwTlbInvalidate = FALSE;
				PipeControlParam.dwFlushRenderTargetCache =
				    TRUE;

				pHwInterface->pfnAddPipeControlCmdBb
				    (pHwInterface, pBatchBuffer,
				     &PipeControlParam);
			}

			PBYTE pBBuffer =
			    pBatchBuffer->pData + pBatchBuffer->iCurrent;
			for (tIndex = 0; tIndex < pKernelParam->iNumThreads;
			     tIndex++) {
				if (enableThreadSpace) {
					pCmd->DW2.UseScoreboard =
					    (pState->ScoreboardParams.numMask ==
					     0) ? 0 : 1;
					pCmd->DW4.ScoreboardX =
					    pThreadCoordinates[tIndex].x;
					pCmd->DW4.ScoreboardY =
					    pThreadCoordinates[tIndex].y;
					if (!pDependencyMask)
						pCmd->DW5.ScoreboardMask =
						    (1 <<
						     pState->ScoreboardParams.
						     numMask) - 1;
					else
						pCmd->DW5.ScoreboardMask =
						    pDependencyMask
						    [tIndex].mask;
				} else if (enableKernelThreadSpace) {
					pCmd->DW2.UseScoreboard =
					    (pState->ScoreboardParams.numMask ==
					     0) ? 0 : 1;
					pCmd->DW4.ScoreboardX =
					    pKernelThreadCoordinates[tIndex].x;
					pCmd->DW4.ScoreboardY =
					    pKernelThreadCoordinates[tIndex].y;
					if (!pDependencyMask)
						pCmd->DW5.ScoreboardMask =
						    (1 <<
						     pState->ScoreboardParams.
						     numMask) - 1;
					else
						pCmd->DW5.ScoreboardMask =
						    pDependencyMask
						    [tIndex].mask;
				} else {
					pCmd->DW4.ScoreboardX =
					    tIndex %
					    pTaskParam->threadSpaceWidth;
					pCmd->DW4.ScoreboardY =
					    tIndex /
					    pTaskParam->threadSpaceWidth;
				}

				for (aIndex = 0;
				     aIndex < pKernelParam->iNumArgs;
				     aIndex++) {
					pArgParam =
					    &pKernelParam->CmArgParams[aIndex];
					index = tIndex * pArgParam->bPerThread;

					if ((pKernelParam->dwCmFlags &
					     CM_KERNEL_FLAGS_CURBE)
					    && !pArgParam->bPerThread) {
						continue;
					}
					CM_ASSERT(pArgParam->iPayloadOffset <
						  pKernelParam->iPayloadSize);

					switch (pArgParam->Kind) {
					case CM_ARGUMENT_GENERAL:
						GENOS_SecureMemcpy(pCmd_inline +
								   pArgParam->iPayloadOffset,
								   pArgParam->iUnitSize,
								   pArgParam->pFirstValue
								   +
								   index *
								   pArgParam->iUnitSize,
								   pArgParam->iUnitSize);
						break;

					case CM_ARGUMENT_SURFACEBUFFER:
						CM_CHK_GENOSSTATUS
						    (HalCm_SetupBufferSurfaceState
						     (pState, pArgParam,
						      pIndexParam,
						      iBindingTable, -1, index,
						      pCmd_inline));
						break;

					case CM_ARGUMENT_SURFACE2D_UP:
						CM_CHK_GENOSSTATUS
						    (HalCm_Setup2DSurfaceUPState
						     (pState, pArgParam,
						      pIndexParam,
						      iBindingTable, index,
						      pCmd_inline));
						break;

					case CM_ARGUMENT_SURFACE2D:
						CM_CHK_GENOSSTATUS
						    (HalCm_Setup2DSurfaceState
						     (pState, pArgParam,
						      pIndexParam,
						      iBindingTable, index,
						      pCmd_inline));
						break;

					default:
						CM_ERROR_ASSERT
						    ("Argument kind '%d' is not supported",
						     pArgParam->Kind);
						goto finish;
					}
				}

				GENOS_SecureMemcpy(pBBuffer, cmd_size, cmd_data,
						   cmd_size);
				pBBuffer += cmd_size;
				pBatchBuffer->iCurrent += cmd_size;
			}
		}
	}

	for (i = 0; i < CM_MAX_GLOBAL_SURFACE_NUMBER; i++) {
		if ((pKernelParam->globalSurface[i] & CM_SURFACE_MASK) !=
		    CM_NULL_SURFACE) {
			CM_HAL_KERNEL_ARG_PARAM ArgParam;
			pArgParam = &ArgParam;

			ArgParam.Kind = CM_ARGUMENT_SURFACEBUFFER;
			ArgParam.iPayloadOffset = 0;
			ArgParam.iUnitCount = 1;
			ArgParam.iUnitSize = sizeof(DWORD);
			ArgParam.bPerThread = FALSE;
			ArgParam.pFirstValue =
			    (PBYTE) & pKernelParam->globalSurface[i];

			CM_CHK_GENOSSTATUS(HalCm_SetupBufferSurfaceState
					   (pState, pArgParam, pIndexParam,
					    iBindingTable, (SHORT) i, 0, NULL));
		}
	}

 finish:
	return hr;
}

GENOS_STATUS HalCm_Allocate(PCM_HAL_STATE pState)
{
	GENOS_STATUS hr;
	PCM_HAL_DEVICE_PARAM pDeviceParam;
	PGENHW_HW_INTERFACE pHwInterface;
	PGENHW_GSH_SETTINGS pGshSettings;
	PGENHW_SSH_SETTINGS pSshSettings;
	UINT i;
	UINT maxTasks;

	PGENHW_BATCH_BUFFER pBb = NULL;

	CM_ASSERT(pState);

	hr = GENOS_STATUS_UNKNOWN;
	pDeviceParam = &pState->CmDeviceParam;
	pHwInterface = pState->pHwInterface;
	pGshSettings = &pHwInterface->GshSettings;
	pSshSettings = &pHwInterface->SshSettings;

	pGshSettings->iCurbeSize = CM_MAX_CURBE_SIZE_PER_TASK;
	pGshSettings->iMediaStateHeaps = pDeviceParam->iMaxTasks + 1;

	pGshSettings->iMediaIDs = pDeviceParam->iMaxKernelsPerTask;
	pGshSettings->iKernelCount = CM_MAX_GSH_KERNEL_ENTRIES;
	pGshSettings->iKernelBlockSize = pDeviceParam->iMaxKernelBinarySize;
	pGshSettings->iKernelHeapSize = CM_FIXED_GSH_SPACE;

	pGshSettings->iPerThreadScratchSize =
	    pDeviceParam->iMaxPerThreadScratchSpaceSize;
	pGshSettings->iSipSize = CM_MAX_SIP_SIZE;
	pSshSettings->iBindingTables = pDeviceParam->iMaxKernelsPerTask;
	pSshSettings->iSurfacesPerBT = CM_MAX_SURFACE_STATES_PER_BT;
	pSshSettings->iSurfaceStates = CM_MAX_SURFACE_STATES;

	CM_CHK_GENOSSTATUS(pHwInterface->pfnInitialize(pHwInterface, NULL));

	for (UINT iKernelID = 0; iKernelID < CM_MAX_GSH_KERNEL_ENTRIES;
	     ++iKernelID) {
		if (iKernelID > 0) {
			pState->pTotalKernelSize[iKernelID] = 0;
		} else {
			pState->pTotalKernelSize[iKernelID] =
			    CM_FIXED_GSH_SPACE;
		}
	}
	pState->nNumKernelsInGSH = 1;

	pState->iNumBatchBuffers = pGshSettings->iMediaStateHeaps;
	pState->pBatchBuffers = (PGENHW_BATCH_BUFFER)
	    GENOS_AllocAndZeroMemory(pState->iNumBatchBuffers *
				     sizeof(GENHW_BATCH_BUFFER));
	CM_CHK_NULL_RETURN_GENOSSTATUS(pState->pBatchBuffers);

	pBb = pState->pBatchBuffers;
	for (i = 0; i < (UINT) pState->iNumBatchBuffers; i++, pBb++) {
		pBb->pBBRenderData = (PGENHW_BATCH_BUFFER_PARAMS)
		    GENOS_AllocAndZeroMemory(sizeof(GENHW_BATCH_BUFFER_PARAMS));
		CM_CHK_NULL_RETURN_GENOSSTATUS(pBb->pBBRenderData);
		GENOS_FillMemory(pBb->pBBRenderData->BbArgs.
				 BbCmArgs.uiKernelIds,
				 sizeof(UINT64) * CM_MAX_KERNELS_PER_TASK, 0);
		pBb->pBBRenderData->BbArgs.BbCmArgs.uiRefCount = 1;
	}

	CM_CHK_GENOSSTATUS(HalCm_AllocateTsResource(pState));

	CM_CHK_GENOSSTATUS(HalCm_AllocateTables(pState));

	pState->pTaskParam = (PCM_HAL_TASK_PARAM)
	    GENOS_AllocAndZeroMemory(sizeof(CM_HAL_TASK_PARAM));
	CM_CHK_NULL_RETURN_GENOSSTATUS(pState->pTaskParam);

	pState->pTaskTimeStamp = (PCM_HAL_TASK_TIMESTAMP)
	    GENOS_AllocAndZeroMemory(sizeof(CM_HAL_TASK_TIMESTAMP));
	CM_CHK_NULL_RETURN_GENOSSTATUS(pState->pTaskTimeStamp);

	pState->SurfaceRegTable.Count =
	    pState->CmDeviceParam.iMax2DSurfaceTableSize;
	pState->SurfaceRegTable.pEntries = pState->pSurf2DTable;

	maxTasks = pState->CmDeviceParam.iMaxTasks;
	GENOS_FillMemory(pState->pTaskStatusTable, (SIZE_T) maxTasks,
			 CM_INVALID_INDEX);

	hr = GENOS_STATUS_SUCCESS;

 finish:
	return hr;
}

GENOS_STATUS HalCm_ExecuteTask(PCM_HAL_STATE pState,
			       PCM_HAL_EXEC_TASK_PARAM pExecParam)
{
	GENOS_STATUS hr;
	PGENHW_HW_INTERFACE pHwInterface;
	PGENHW_MEDIA_STATE pMediaState;
	PGENHW_BATCH_BUFFER pBatchBuffer;
	PCM_HAL_KERNEL_PARAM pKernelParam;
	INT iTaskId;
	INT iRemBindingTables;
	INT iBindingTable;
	INT iBTI;
	INT iMediaID;
	INT iKAID;
	DWORD dwVfeCurbeSize;
	DWORD dwMaxInlineDataSize, dwMaxIndirectDataSize;
	UINT i;
	PVOID pCmdBuffer = NULL;
	PCM_HAL_TASK_PARAM pTaskParam = pState->pTaskParam;
	UINT uiBTSizePower_2;

	CM_ASSERT(pState);
	CM_ASSERT(pExecParam);

	hr = GENOS_STATUS_SUCCESS;
	pHwInterface = pState->pHwInterface;
	pBatchBuffer = NULL;

	if (pExecParam->iNumKernels > pState->CmDeviceParam.iMaxKernelsPerTask) {
		CM_ERROR_ASSERT("Number of Kernels per task exceeds maximum");
		goto finish;
	}
	pHwInterface->pOsInterface->
	    pfnResetOsStates(pHwInterface->pOsInterface);
	CM_CHK_GENOSSTATUS(pHwInterface->pfnResetHwStates(pHwInterface));

	GENOS_ZeroMemory(pState->pTaskParam, sizeof(CM_HAL_TASK_PARAM));
	pState->WalkerParams.CmWalkerEnable = 0;

	dwVfeCurbeSize = 0;
	dwMaxInlineDataSize = 0;
	dwMaxIndirectDataSize = 0;

	CM_CHK_GENOSSTATUS(HalCm_GetNewTaskId(pState, &iTaskId));

	CM_CHK_GENOSSTATUS(HalCm_ParseTask(pState, pExecParam));

	uiBTSizePower_2 =
	    (UINT) pHwInterface->SshSettings.iBTAlignment /
	    sizeof(BINDING_TABLE_STATE_G5);
	while (uiBTSizePower_2 < pTaskParam->surfacePerBT) {
		uiBTSizePower_2 = uiBTSizePower_2 * 2;
	}
	pTaskParam->surfacePerBT = uiBTSizePower_2;
	pHwInterface->pSurfaceStateHeap->iBindingTableSize =
	    GENOS_ALIGN_CEIL(pTaskParam->surfacePerBT *
			     sizeof(BINDING_TABLE_STATE_G5),
			     pHwInterface->SshSettings.iBTAlignment);

	pHwInterface->SshSettings.iBindingTables =
	    pHwInterface->SshSettings.iBindingTables *
	    pHwInterface->SshSettings.iSurfacesPerBT / pTaskParam->surfacePerBT;
	pHwInterface->SshSettings.iSurfacesPerBT = pTaskParam->surfacePerBT;

	if (pExecParam->iNumKernels >
	    (UINT) pHwInterface->SshSettings.iBindingTables) {
		CM_ERROR_ASSERT
		    ("Number of Kernels per task exceeds the number can be hold by binding table");
		goto finish;
	}
	pMediaState = pHwInterface->pfnAssignMediaState(pHwInterface);
	CM_CHK_NULL_RETURN_GENOSSTATUS(pMediaState);

	CM_ASSERT((pHwInterface->pGeneralStateHeap->iCurMediaState >= 0)
		  && (pHwInterface->pGeneralStateHeap->iCurMediaState <
		      pState->iNumBatchBuffers));

	CM_CHK_GENOSSTATUS(pHwInterface->pfnAssignSshInstance(pHwInterface));

	if (!pState->WalkerParams.CmWalkerEnable) {
		CM_CHK_GENOSSTATUS(HalCm_GetBatchBuffer
				   (pState, pExecParam->iNumKernels,
				    pExecParam->pKernels, &pBatchBuffer));
		CM_CHK_NULL_RETURN_GENOSSTATUS(pBatchBuffer);
		if ((pBatchBuffer->pBBRenderData->BbArgs.BbCmArgs.uiRefCount ==
		     1) || (pState->pTaskParam->reuseBBUpdateMask == 1)) {
			CM_CHK_GENOSSTATUS(pHwInterface->pfnLockBB
					   (pHwInterface, pBatchBuffer));
		}
	}

	for (i = 0; i < pExecParam->iNumKernels; i++) {
		CM_HAL_INDEX_PARAM indexParam;
		GENOS_ZeroMemory(&indexParam, sizeof(CM_HAL_INDEX_PARAM));
		pKernelParam = pExecParam->pKernels[i];

		CM_CHK_GENOSSTATUS(HalCm_SetupStatesForKernelInitial
				   (pState, pBatchBuffer, iTaskId, pKernelParam,
				    &indexParam,
				    pExecParam->piKernelCurbeOffset[i], iBTI,
				    iMediaID, iKAID));

		CM_CHK_GENOSSTATUS(HalCm_FinishStatesForKernel
				   (pState, pBatchBuffer, iTaskId, pKernelParam,
				    i, &indexParam, iBTI, iMediaID, iKAID));

		dwVfeCurbeSize +=
		    GENOS_ALIGN_CEIL(pKernelParam->iKrnCurbeSize,
				     pState->pfnGetCurbeBlockAlignSize());
		if (pKernelParam->iPayloadSize > dwMaxInlineDataSize) {
			dwMaxInlineDataSize = pKernelParam->iPayloadSize;
		}
		if (pKernelParam->CmIndirectDataParam.iIndirectDataSize >
		    dwMaxIndirectDataSize) {
			dwMaxIndirectDataSize =
			    pKernelParam->CmIndirectDataParam.iIndirectDataSize;
		}
	}

	pState->pTaskParam->dwVfeCurbeSize = dwVfeCurbeSize;
	if (dwMaxIndirectDataSize) {
		pState->pTaskParam->dwUrbEntrySize = dwMaxIndirectDataSize;
	} else {
		pState->pTaskParam->dwUrbEntrySize = dwMaxInlineDataSize;
	}

	iRemBindingTables =
	    pHwInterface->SshSettings.iBindingTables - pExecParam->iNumKernels;

	if (iRemBindingTables > 0) {
		for (i = 0; i < (UINT) iRemBindingTables; i++) {
			CM_CHK_GENOSSTATUS(pHwInterface->pfnAssignBindingTable
					   (pHwInterface, &iBindingTable));
		}
	}

	CM_CHK_GENOSSTATUS(pState->pfnGetGpuTime(pState,
						 &pState->
						 pTaskTimeStamp->iCMSubmitTimeStamp
						 [iTaskId]));

	CM_CHK_GENOSSTATUS(pState->pfnGetGlobalTime
			   (&pState->
			    pTaskTimeStamp->iGlobalCmSubmitTime[iTaskId]));

	CM_CHK_GENOSSTATUS(pState->pfnSubmitCommands
			   (pState, pBatchBuffer, iTaskId, pExecParam->pKernels,
			    &pCmdBuffer));

	pExecParam->iTaskIdOut = iTaskId;

	if (pCmdBuffer) {
		pExecParam->OsData = pCmdBuffer;
	}
	pState->pTaskStatusTable[iTaskId] = (CHAR) iTaskId;

 finish:
	if (pBatchBuffer) {
		if (pBatchBuffer->bLocked) {
			CM_ASSERT(0);
			if (pBatchBuffer->pBBRenderData->BbArgs.
			    BbCmArgs.uiRefCount == 1) {
				pHwInterface->pfnUnlockBB(pHwInterface,
							  pBatchBuffer);
			}
		}
	}

	return hr;
}

GENOS_STATUS HalCm_ExecuteGroupTask(PCM_HAL_STATE pState,
				    PCM_HAL_EXEC_GROUP_TASK_PARAM
				    pExecGroupParam)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	PGENHW_HW_INTERFACE pHwInterface = pState->pHwInterface;
	CM_HAL_INDEX_PARAM indexParam;
	INT iTaskId;
	UINT iRemBindingTables;
	INT iBindingTable;
	INT iBTI;
	INT iMediaID;
	INT iKAID;
	PGENHW_MEDIA_STATE pMediaState;
	UINT i;
	PVOID pCmdBuffer = NULL;
	PCM_HAL_KERNEL_PARAM pKernelParam = NULL;
	PCM_HAL_TASK_PARAM pTaskParam = pState->pTaskParam;
	UINT uiBTSizePower_2;
	DWORD dwVfeCurbeSize = 0;

	CM_ASSERT(pState);
	CM_ASSERT(pExecGroupParam);

	GENOS_ZeroMemory(pState->pTaskParam, sizeof(CM_HAL_TASK_PARAM));
	GENOS_ZeroMemory(&indexParam, sizeof(CM_HAL_INDEX_PARAM));

	pHwInterface->pOsInterface->
	    pfnResetOsStates(pHwInterface->pOsInterface);
	CM_CHK_GENOSSTATUS(pHwInterface->pfnResetHwStates(pHwInterface));

	pState->WalkerParams.CmWalkerEnable = 0;
	pState->pTaskParam->blGpGpuWalkerEnabled = 1;

	CM_CHK_GENOSSTATUS(HalCm_GetNewTaskId(pState, &iTaskId));

	CM_CHK_GENOSSTATUS(HalCm_ParseGroupTask(pState, pExecGroupParam));

	uiBTSizePower_2 =
	    (UINT) pHwInterface->SshSettings.iBTAlignment /
	    sizeof(BINDING_TABLE_STATE_G5);
	while (uiBTSizePower_2 < pTaskParam->surfacePerBT) {
		uiBTSizePower_2 = uiBTSizePower_2 * 2;
	}
	pTaskParam->surfacePerBT = uiBTSizePower_2;
	pHwInterface->pSurfaceStateHeap->iBindingTableSize =
	    GENOS_ALIGN_CEIL(pTaskParam->surfacePerBT *
			     sizeof(BINDING_TABLE_STATE_G5),
			     pHwInterface->SshSettings.iBTAlignment);

	pHwInterface->SshSettings.iBindingTables =
	    pHwInterface->SshSettings.iBindingTables *
	    pHwInterface->SshSettings.iSurfacesPerBT / pTaskParam->surfacePerBT;
	pHwInterface->SshSettings.iSurfacesPerBT = pTaskParam->surfacePerBT;

	if (pExecGroupParam->iNumKernels >
	    (UINT) pHwInterface->SshSettings.iBindingTables) {
		CM_ERROR_ASSERT
		    ("Number of Kernels per task exceeds the number can be hold by binding table");
		goto finish;
	}
	pMediaState = pHwInterface->pfnAssignMediaState(pHwInterface);
	CM_CHK_NULL_RETURN_GENOSSTATUS(pMediaState);

	CM_ASSERT((pHwInterface->pGeneralStateHeap->iCurMediaState >= 0)
		  && (pHwInterface->pGeneralStateHeap->iCurMediaState <
		      pState->iNumBatchBuffers));

	CM_CHK_GENOSSTATUS(pHwInterface->pfnAssignSshInstance(pHwInterface));

	for (i = 0; i < pExecGroupParam->iNumKernels; i++) {
		CM_HAL_INDEX_PARAM indexParam;
		GENOS_ZeroMemory(&indexParam, sizeof(CM_HAL_INDEX_PARAM));
		pKernelParam = pExecGroupParam->pKernels[i];

		CM_CHK_GENOSSTATUS(HalCm_SetupStatesForKernelInitial
				   (pState, NULL, iTaskId, pKernelParam,
				    &indexParam,
				    pExecGroupParam->piKernelCurbeOffset[i],
				    iBTI, iMediaID, iKAID));

		CM_CHK_GENOSSTATUS(HalCm_FinishStatesForKernel
				   (pState, NULL, iTaskId, pKernelParam, i,
				    &indexParam, iBTI, iMediaID, iKAID));

		dwVfeCurbeSize +=
		    GENOS_ALIGN_CEIL(pKernelParam->iKrnCurbeSize,
				     pState->pfnGetCurbeBlockAlignSize());
	}

	pState->pTaskParam->dwVfeCurbeSize = dwVfeCurbeSize;
	pState->pTaskParam->dwUrbEntrySize = 0;

	iRemBindingTables =
	    pHwInterface->SshSettings.iBindingTables -
	    pExecGroupParam->iNumKernels;

	if (iRemBindingTables > 0) {
		for (i = 0; i < iRemBindingTables; i++) {
			CM_CHK_GENOSSTATUS(pHwInterface->pfnAssignBindingTable
					   (pHwInterface, &iBindingTable));
		}
	}

	CM_CHK_GENOSSTATUS(pState->pfnGetGpuTime(pState,
						 &pState->
						 pTaskTimeStamp->iCMSubmitTimeStamp
						 [iTaskId]));

	CM_CHK_GENOSSTATUS(pState->pfnGetGlobalTime
			   (&pState->
			    pTaskTimeStamp->iGlobalCmSubmitTime[iTaskId]));

	CM_CHK_GENOSSTATUS(pState->pfnSubmitCommands(pState, NULL, iTaskId,
						     pExecGroupParam->pKernels,
						     &pCmdBuffer));

	pExecGroupParam->iTaskIdOut = iTaskId;

	if (pCmdBuffer) {
		pExecGroupParam->OsData = pCmdBuffer;
	}
	pState->pTaskStatusTable[iTaskId] = (CHAR) iTaskId;

 finish:

	return hr;
}

GENOS_STATUS HalCm_ExecuteHintsTask(PCM_HAL_STATE pState,
				    PCM_HAL_EXEC_HINTS_TASK_PARAM
				    pExecHintsParam)
{
	GENOS_STATUS hr;
	PGENHW_HW_INTERFACE pHwInterface;
	PGENHW_MEDIA_STATE pMediaState;
	PGENHW_BATCH_BUFFER pBatchBuffer;
	PCM_HAL_KERNEL_PARAM pKernelParam;
	UINT i;
	UINT numTasks;
	UINT64 uiOrigKernelIds[CM_MAX_KERNELS_PER_TASK];
	INT iTaskId;
	INT iRemBindingTables;
	INT iBindingTable;
	DWORD dwVfeCurbeSize;
	DWORD dwMaxInlineDataSize;
	DWORD dwMaxIndirectDataSize;
	INT *pBindingTableEntries;
	INT *pMediaIds;
	INT *pKAIDs;
	PCM_HAL_INDEX_PARAM pIndexParams;
	PVOID pCmdBuffer;
	BOOL splitTask;

	CM_ASSERT(pState);
	CM_ASSERT(pExecHintsParam);

	hr = GENOS_STATUS_SUCCESS;
	pHwInterface = pState->pHwInterface;
	pBatchBuffer = NULL;
	pBindingTableEntries = NULL;
	pMediaIds = NULL;
	pKAIDs = NULL;
	pIndexParams = NULL;
	pCmdBuffer = NULL;
	splitTask = FALSE;

	if (pExecHintsParam->iNumKernels >
	    pState->CmDeviceParam.iMaxKernelsPerTask) {
		CM_ERROR_ASSERT("Number of Kernels per task exceeds maximum");
		goto finish;
	}

	pBindingTableEntries =
	    (int *)GENOS_AllocAndZeroMemory(sizeof(int) *
					    pExecHintsParam->iNumKernels);
	pMediaIds =
	    (int *)GENOS_AllocAndZeroMemory(sizeof(int) *
					    pExecHintsParam->iNumKernels);
	pKAIDs =
	    (int *)GENOS_AllocAndZeroMemory(sizeof(int) *
					    pExecHintsParam->iNumKernels);
	pIndexParams = (PCM_HAL_INDEX_PARAM)
	    GENOS_AllocAndZeroMemory(sizeof(CM_HAL_INDEX_PARAM) *
				     pExecHintsParam->iNumKernels);
	if (!pBindingTableEntries || !pMediaIds || !pKAIDs || !pIndexParams) {
		CM_ERROR_ASSERT
		    ("Memory allocation failed in ExecuteHints Task");
		goto finish;
	}
	numTasks =
	    (pExecHintsParam->iHints & CM_HINTS_MASK_NUM_TASKS) >>
	    CM_HINTS_NUM_BITS_TASK_POS;
	if (numTasks > 1) {
		splitTask = TRUE;
	}

	GENOS_FillMemory(pBindingTableEntries,
			 sizeof(int) * pExecHintsParam->iNumKernels,
			 CM_INVALID_INDEX);
	GENOS_FillMemory(pMediaIds, sizeof(int) * pExecHintsParam->iNumKernels,
			 CM_INVALID_INDEX);
	GENOS_FillMemory(pKAIDs, sizeof(int) * pExecHintsParam->iNumKernels,
			 CM_INVALID_INDEX);

	pHwInterface->pOsInterface->
	    pfnResetOsStates(pHwInterface->pOsInterface);
	CM_CHK_GENOSSTATUS(pHwInterface->pfnResetHwStates(pHwInterface));

	GENOS_ZeroMemory(pState->pTaskParam, sizeof(CM_HAL_TASK_PARAM));
	pState->WalkerParams.CmWalkerEnable = 0;

	dwVfeCurbeSize = 0;
	dwMaxInlineDataSize = 0;
	dwMaxIndirectDataSize = 0;

	GENOS_ZeroMemory(&uiOrigKernelIds,
			 CM_MAX_KERNELS_PER_TASK * sizeof(UINT64));

	CM_CHK_GENOSSTATUS(HalCm_GetNewTaskId(pState, &iTaskId));

	CM_CHK_GENOSSTATUS(HalCm_ParseHintsTask(pState, pExecHintsParam));

	pMediaState = pHwInterface->pfnAssignMediaState(pHwInterface);
	CM_CHK_NULL_RETURN_GENOSSTATUS(pMediaState);

	CM_ASSERT((pHwInterface->pGeneralStateHeap->iCurMediaStatei >= 0)
		  && (pHwInterface->pGeneralStateHeap->iCurMediaState <
		      pState->iNumBatchBuffers));

	CM_CHK_GENOSSTATUS(pHwInterface->pfnAssignSshInstance(pHwInterface));

	if (!pState->WalkerParams.CmWalkerEnable) {
		if (splitTask) {
			for (i = 0; i < pExecHintsParam->iNumKernels; ++i) {
				uiOrigKernelIds[i] =
				    pExecHintsParam->pKernels[i]->uiKernelId;
			}

			CM_CHK_GENOSSTATUS(HalCm_AddKernelIDTag
					   (pExecHintsParam->pKernels,
					    pExecHintsParam->iNumKernels,
					    numTasks,
					    pExecHintsParam->iNumTasksGenerated));
		}
		CM_CHK_GENOSSTATUS(HalCm_GetBatchBuffer
				   (pState, pExecHintsParam->iNumKernels,
				    pExecHintsParam->pKernels, &pBatchBuffer));

		if (splitTask) {
			for (i = 0; i < pExecHintsParam->iNumKernels; ++i) {
				pExecHintsParam->pKernels[i]->uiKernelId =
				    uiOrigKernelIds[i];
			}
		}
		if ((pBatchBuffer->pBBRenderData->BbArgs.BbCmArgs.uiRefCount ==
		     1) || (pState->pTaskParam->reuseBBUpdateMask == 1)) {
			CM_CHK_GENOSSTATUS(pHwInterface->pfnLockBB
					   (pHwInterface, pBatchBuffer));
		}
	}
	if ((pExecHintsParam->iHints & CM_HINTS_MASK_MEDIAOBJECT) ==
	    CM_HINTS_MASK_MEDIAOBJECT) {
		for (i = 0; i < pExecHintsParam->iNumKernels; ++i) {
			CM_CHK_GENOSSTATUS(HalCm_SetupStatesForKernelInitial
					   (pState, pBatchBuffer, iTaskId,
					    pExecHintsParam->pKernels[i],
					    &pIndexParams[i],
					    pExecHintsParam->piKernelCurbeOffset
					    [i], pBindingTableEntries[i],
					    pMediaIds[i], pKAIDs[i]));
		}

		CM_CHK_NULL_RETURN_GENOSSTATUS(pBatchBuffer);

		CM_CHK_GENOSSTATUS(HalCm_FinishStatesForKernelMix
				   (pState, pBatchBuffer, iTaskId,
				    pExecHintsParam->pKernels, pIndexParams,
				    pBindingTableEntries, pMediaIds, pKAIDs,
				    pExecHintsParam->iNumKernels,
				    pExecHintsParam->iHints,
				    pExecHintsParam->isLastTask));

		for (i = 0; i < pExecHintsParam->iNumKernels; ++i) {
			pKernelParam = pExecHintsParam->pKernels[i];
			dwVfeCurbeSize +=
			    GENOS_ALIGN_CEIL(pKernelParam->iKrnCurbeSize,
					     pState->pfnGetCurbeBlockAlignSize
					     ());
			if (pKernelParam->iPayloadSize > dwMaxInlineDataSize) {
				dwMaxInlineDataSize =
				    pKernelParam->iPayloadSize;
			}
			if (pKernelParam->
			    CmIndirectDataParam.iIndirectDataSize >
			    dwMaxIndirectDataSize) {
				dwMaxIndirectDataSize =
				    pKernelParam->
				    CmIndirectDataParam.iIndirectDataSize;
			}
		}

		pState->pTaskParam->dwVfeCurbeSize = dwVfeCurbeSize;
		if (dwMaxIndirectDataSize) {
			pState->pTaskParam->dwVfeCurbeSize =
			    dwMaxIndirectDataSize;
		} else {
			pState->pTaskParam->dwUrbEntrySize =
			    dwMaxInlineDataSize;
		}

		iRemBindingTables = pState->CmDeviceParam.iMaxKernelsPerTask -
		    pExecHintsParam->iNumKernels;

		if (iRemBindingTables > 0) {
			for (i = 0; i < (UINT) iRemBindingTables; ++i) {
				CM_CHK_GENOSSTATUS
				    (pHwInterface->pfnAssignBindingTable
				     (pHwInterface, &iBindingTable));
			}
		}

		CM_CHK_GENOSSTATUS(pState->pfnGetGpuTime(pState,
							 &pState->
							 pTaskTimeStamp->iCMSubmitTimeStamp
							 [iTaskId]));

		CM_CHK_GENOSSTATUS(pState->pfnGetGlobalTime
				   (&pState->pTaskTimeStamp->iGlobalCmSubmitTime
				    [iTaskId]));

		CM_CHK_GENOSSTATUS(pState->pfnSubmitCommands
				   (pState, pBatchBuffer, iTaskId,
				    pExecHintsParam->pKernels, &pCmdBuffer));

		pExecHintsParam->iTaskIdOut = iTaskId;

		if (pCmdBuffer) {
			pExecHintsParam->OsData = pCmdBuffer;
		}
		pState->pTaskStatusTable[iTaskId] = (CHAR) iTaskId;
	} else {
		CM_ASSERT(0);
		hr = GENOS_STATUS_UNKNOWN;
	}

 finish:
	if (pBatchBuffer) {
		if (pBatchBuffer->bLocked) {
			CM_ASSERT(0);
			if (pBatchBuffer->pBBRenderData->BbArgs.
			    BbCmArgs.uiRefCount == 1) {
				pHwInterface->pfnUnlockBB(pHwInterface,
							  pBatchBuffer);
			}
		}
	}
	if (pBindingTableEntries)
		GENOS_FreeMemory(pBindingTableEntries);
	if (pMediaIds)
		GENOS_FreeMemory(pMediaIds);
	if (pKAIDs)
		GENOS_FreeMemory(pKAIDs);
	if (pIndexParams)
		GENOS_FreeMemory(pIndexParams);

	return hr;
}

GENOS_STATUS HalCm_QueryTask(PCM_HAL_STATE pState,
			     PCM_HAL_QUERY_TASK_PARAM pQueryParam)
{
	GENOS_STATUS hr;
	PGENHW_HW_INTERFACE pHwInterface;
	PINT64 piSyncStart;
	PINT64 piSyncEnd;
	UINT64 iTicks;
	INT iSyncOffset;
	UINT64 iHWStartTicks;
	UINT64 iHWEndTicks;
	INT iMaxTasks;

	CM_ASSERT(pState);
	CM_ASSERT(pQueryParam);

	hr = GENOS_STATUS_SUCCESS;

	iMaxTasks = (INT) pState->CmDeviceParam.iMaxTasks;
	if ((pQueryParam->iTaskId < 0) || (pQueryParam->iTaskId >= iMaxTasks) ||
	    (pState->pTaskStatusTable[pQueryParam->iTaskId] ==
	     CM_INVALID_INDEX)) {
		CM_ERROR_ASSERT("Invalid Task ID'%d'.", pQueryParam->iTaskId);
		goto finish;
	}

	pHwInterface = pState->pHwInterface;
	iSyncOffset = pState->pfnGetTaskSyncLocation(pQueryParam->iTaskId);
	piSyncStart = (PINT64) (pState->TsResource.pData + iSyncOffset);
	piSyncEnd = piSyncStart + 1;
	pQueryParam->iTaskDuration = CM_INVALID_INDEX;

	if (*piSyncStart == CM_INVALID_INDEX) {
		pQueryParam->status = CM_TASK_QUEUED;
	} else if (*piSyncEnd == CM_INVALID_INDEX) {
		pQueryParam->status = CM_TASK_IN_PROGRESS;
	} else {
		pQueryParam->status = CM_TASK_FINISHED;

		pHwInterface->pfnConvertToNanoSeconds(pHwInterface,
						      *piSyncStart,
						      &iHWStartTicks);

		pHwInterface->pfnConvertToNanoSeconds(pHwInterface,
						      *piSyncEnd, &iHWEndTicks);

		CM_CHK_GENOSSTATUS(pState->pfnGetGlobalTime
				   (&pState->pTaskTimeStamp->iCompleteTime
				    [pQueryParam->iTaskId]));

		iTicks = *piSyncEnd - *piSyncStart;

		pHwInterface->pfnConvertToNanoSeconds(pHwInterface,
						      iTicks,
						      &pQueryParam->iTaskDuration);

		pQueryParam->iTaskGlobalCMSubmitTime =
		    pState->pTaskTimeStamp->
		    iGlobalCmSubmitTime[pQueryParam->iTaskId];
		pQueryParam->iTaskCompleteTime =
		    pState->pTaskTimeStamp->iCompleteTime[pQueryParam->iTaskId];
		CM_CHK_GENOSSTATUS(pState->pfnConvertToQPCTime
				   (pState->
				    pTaskTimeStamp->iCMSubmitTimeStamp
				    [pQueryParam->iTaskId],
				    &pQueryParam->iTaskCMSubmitTimeStamp));
		CM_CHK_GENOSSTATUS(pState->pfnConvertToQPCTime
				   (iHWStartTicks,
				    &pQueryParam->iTaskHWStartTimeStamp));
		CM_CHK_GENOSSTATUS(pState->pfnConvertToQPCTime
				   (iHWEndTicks,
				    &pQueryParam->iTaskHWEndTimeStamp));

		pState->pTaskStatusTable[pQueryParam->iTaskId] =
		    CM_INVALID_INDEX;
	}

 finish:
	return hr;
}

GENOS_STATUS HalCm_GetMaxValues(PCM_HAL_STATE pState,
				PCM_HAL_MAX_VALUES pMaxValues)
{
	PGENHW_HW_INTERFACE pHwInterface;

	pHwInterface = pState->pHwInterface;

	pMaxValues->iMaxTasks = pState->CmDeviceParam.iMaxTasks;
	pMaxValues->iMaxKernelsPerTask = CM_MAX_KERNELS_PER_TASK;
	pMaxValues->iMaxKernelBinarySize =
	    pState->CmDeviceParam.iMaxKernelBinarySize;
	pMaxValues->iMaxSpillSizePerHwThread =
	    pState->CmDeviceParam.iMaxPerThreadScratchSpaceSize;
	pMaxValues->iMaxBufferTableSize = CM_MAX_BUFFER_SURFACE_TABLE_SIZE;
	pMaxValues->iMax2DSurfaceTableSize = CM_MAX_2D_SURFACE_TABLE_SIZE;
	pMaxValues->iMaxArgsPerKernel = CM_MAX_ARGS_PER_KERNEL;
	pMaxValues->iMaxUserThreadsPerTask = CM_MAX_USER_THREADS;
	pMaxValues->iMaxUserThreadsPerTaskNoThreadArg =
	    CM_MAX_USER_THREADS_NO_THREADARG;
	pMaxValues->iMaxArgByteSizePerKernel = CM_MAX_ARG_BYTE_PER_KERNEL;
	pMaxValues->iMaxSurfacesPerKernel = pHwInterface->pHwCaps->dwMaxBTIndex;
	pMaxValues->iMaxHwThreads = pHwInterface->pHwCaps->dwMaxThreads;

	return GENOS_STATUS_SUCCESS;
}

GENOS_STATUS HalCm_GetMaxValuesEx(PCM_HAL_STATE pState,
				  PCM_HAL_MAX_VALUES_EX pMaxValuesEx)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	pMaxValuesEx->iMax2DUPSurfaceTableSize =
	    CM_MAX_2D_SURFACE_UP_TABLE_SIZE;
	pMaxValuesEx->iMaxCURBESizePerKernel = CM_MAX_CURBE_SIZE_PER_KERNEL;
	pMaxValuesEx->iMaxCURBESizePerTask = CM_MAX_CURBE_SIZE_PER_TASK;
	pMaxValuesEx->iMaxIndirectDataSizePerKernel =
	    CM_MAX_INDIRECT_DATA_SIZE_PER_KERNEL;

	if (pState->Platform.eRenderCoreFamily <= IGFX_GEN8_CORE) {
		pMaxValuesEx->iMaxUserThreadsPerMediaWalker =
		    CM_MAX_USER_THREADS_PER_MEDIA_WALKER;
	}

	CM_CHK_GENOSSTATUS(pState->pfnGetUserDefinedThreadCountPerThreadGroup
			   (pState,
			    &pMaxValuesEx->iMaxUserThreadsPerThreadGroup));

 finish:
	return hr;
}

GENOS_STATUS HalCm_FreeBuffer(PCM_HAL_STATE pState, DWORD dwHandle)
{
	GENOS_STATUS hr;
	PCM_HAL_BUFFER_ENTRY pEntry;
	PGENOS_INTERFACE pOsInterface;

	hr = GENOS_STATUS_SUCCESS;
	pOsInterface = pState->pHwInterface->pOsInterface;

	CM_CHK_GENOSSTATUS(HalCm_GetBufferEntry(pState, dwHandle, &pEntry));
	if (pEntry->isAllocatedbyCmrtUmd) {
		pOsInterface->pfnFreeResource(pOsInterface,
					      &pEntry->OsResource);
	} else {
		HalCm_OsResource_Unreference(&pEntry->OsResource);
	}
	pOsInterface->pfnResetResourceAllocationIndex(pOsInterface,
						      &pEntry->OsResource);
	pEntry->iSize = 0;

 finish:
	return hr;
}

GENOS_STATUS HalCm_UpdateBuffer(PCM_HAL_STATE pState,
				DWORD dwHandle, DWORD dwSize)
{
	GENOS_STATUS hr;
	PCM_HAL_BUFFER_ENTRY pEntry;

	hr = GENOS_STATUS_SUCCESS;

	CM_CHK_GENOSSTATUS(HalCm_GetBufferEntry(pState, dwHandle, &pEntry));

	pEntry->iSize = dwSize;

 finish:
	return hr;
}

GENOS_STATUS HalCm_UpdateSurface2D(PCM_HAL_STATE pState,
				   DWORD dwHandle,
				   DWORD dwWidth, DWORD dwHeight)
{
	GENOS_STATUS hr;
	PCM_HAL_SURFACE2D_ENTRY pEntry;

	hr = GENOS_STATUS_SUCCESS;

	CM_CHK_GENOSSTATUS(HalCm_GetSurface2DEntry(pState, dwHandle, &pEntry));

	pEntry->iWidth = dwWidth;
	pEntry->iHeight = dwHeight;

 finish:
	return hr;
}

GENOS_STATUS HalCm_SetSurfaceReadFlag(PCM_HAL_STATE pState, DWORD dwHandle)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	PCM_HAL_SURFACE2D_ENTRY pEntry;

	CM_CHK_GENOSSTATUS(HalCm_GetSurface2DEntry(pState, dwHandle, &pEntry));

	pEntry->bReadSync = TRUE;

 finish:
	return hr;
}

GENOS_STATUS HalCm_LockBuffer(PCM_HAL_STATE pState, PCM_HAL_BUFFER_PARAM pParam)
{
	GENOS_STATUS hr;
	PCM_HAL_BUFFER_ENTRY pEntry;
	PGENOS_INTERFACE pOsInterface;
	GENOS_LOCK_PARAMS LockFlags;
	hr = GENOS_STATUS_SUCCESS;
	pOsInterface = pState->pHwInterface->pOsInterface;

	CM_CHK_GENOSSTATUS(HalCm_GetBufferEntry
			   (pState, pParam->dwHandle, &pEntry));

	if ((pParam->iLockFlag != CM_HAL_LOCKFLAG_READONLY)
	    && (pParam->iLockFlag != CM_HAL_LOCKFLAG_WRITEONLY)) {
		CM_ERROR_ASSERT("Invalid lock flag!");
		hr = GENOS_STATUS_UNKNOWN;
		goto finish;
	}
	GENOS_ZeroMemory(&LockFlags, sizeof(GENOS_LOCK_PARAMS));

	if (pParam->iLockFlag == CM_HAL_LOCKFLAG_READONLY) {
		LockFlags.ReadOnly = TRUE;
	} else {
		LockFlags.WriteOnly = TRUE;
	}

	pParam->pData = pOsInterface->pfnLockResource(pOsInterface,
						      &pEntry->OsResource,
						      &LockFlags);
	CM_CHK_NULL_RETURN_GENOSSTATUS(pParam->pData);

 finish:
	return hr;
}

GENOS_STATUS HalCm_UnlockBuffer(PCM_HAL_STATE pState,
				PCM_HAL_BUFFER_PARAM pParam)
{
	GENOS_STATUS hr;
	PCM_HAL_BUFFER_ENTRY pEntry;
	PGENOS_INTERFACE pOsInterface;

	hr = GENOS_STATUS_SUCCESS;
	pOsInterface = pState->pHwInterface->pOsInterface;

	CM_CHK_GENOSSTATUS(HalCm_GetBufferEntry
			   (pState, pParam->dwHandle, &pEntry));

	CM_HRESULT2GENOSSTATUS_AND_CHECK(pOsInterface->pfnUnlockResource
					 (pOsInterface, &pEntry->OsResource));

 finish:
	return hr;
}

GENOS_STATUS HalCm_FreeSurface2DUP(PCM_HAL_STATE pState, DWORD dwHandle)
{
	GENOS_STATUS hr;
	PCM_HAL_SURFACE2D_UP_ENTRY pEntry;
	PGENOS_INTERFACE pOsInterface;

	hr = GENOS_STATUS_SUCCESS;
	pOsInterface = pState->pHwInterface->pOsInterface;

	CM_CHK_GENOSSTATUS(HalCm_GetResourceUPEntry(pState, dwHandle, &pEntry));

	pOsInterface->pfnFreeResource(pOsInterface, &pEntry->OsResource);
	pOsInterface->pfnResetResourceAllocationIndex(pOsInterface,
						      &pEntry->OsResource);
	pEntry->iWidth = 0;

 finish:
	return hr;
}

GENOS_STATUS IntelGen_GetSurfaceInfo(PGENOS_INTERFACE pOsInterface,
				     PGENHW_SURFACE pSurface)
{

	GENOS_STATUS eStatus = GENOS_STATUS_UNKNOWN;

	GENHW_PUBLIC_ASSERT(pOsInterface);
	GENHW_PUBLIC_ASSERT(pSurface);

	PGENOS_RESOURCE pResource = &pSurface->OsResource;

	GENHW_PUBLIC_ASSERT(!IntelGen_OsResourceIsNull(&pSurface->OsResource));

	pSurface->dwPitch = pResource->iPitch;
	pSurface->TileType = pResource->TileType;
	pSurface->dwDepth = pResource->iDepth;

	pSurface->UPlaneOffset.iSurfaceOffset = 0;
	pSurface->UPlaneOffset.iYOffset = 0;
	pSurface->UPlaneOffset.iXOffset = 0;
	pSurface->VPlaneOffset.iSurfaceOffset = 0;
	pSurface->VPlaneOffset.iXOffset = 0;
	pSurface->VPlaneOffset.iYOffset = 0;

	switch (pResource->Format) {
	case Format_NV12:
	case Format_P010:
	case Format_P016:
		pSurface->UPlaneOffset.iSurfaceOffset =
		    (pSurface->dwHeight -
		     pSurface->dwHeight % 32) * pSurface->dwPitch;
		pSurface->UPlaneOffset.iYOffset = pSurface->dwHeight % 32;
		break;
	case Format_NV21:
		pSurface->UPlaneOffset.iSurfaceOffset =
		    pSurface->dwHeight * pSurface->dwPitch;
		break;
	case Format_YV12:
		pSurface->VPlaneOffset.iSurfaceOffset =
		    pSurface->dwHeight * pSurface->dwPitch;
		pSurface->VPlaneOffset.iYOffset = 0;
		pSurface->UPlaneOffset.iSurfaceOffset =
		    pSurface->dwHeight * pSurface->dwPitch * 5 / 4;
		pSurface->UPlaneOffset.iYOffset = 0;
		break;
	case Format_422H:
		pSurface->UPlaneOffset.iSurfaceOffset =
		    (pSurface->dwHeight -
		     pSurface->dwHeight % 32) * pSurface->dwPitch;
		pSurface->UPlaneOffset.iYOffset = pSurface->dwHeight % 32;
		pSurface->VPlaneOffset.iSurfaceOffset =
		    (pSurface->dwHeight * 2 -
		     (pSurface->dwHeight * 2) % 32) * pSurface->dwPitch;
		pSurface->VPlaneOffset.iYOffset = (pSurface->dwHeight * 2) % 32;
		break;
	case Format_IMC3:
	case Format_422V:
		pSurface->UPlaneOffset.iSurfaceOffset =
		    (pSurface->dwHeight -
		     pSurface->dwHeight % 32) * pSurface->dwPitch;
		pSurface->UPlaneOffset.iYOffset = pSurface->dwHeight % 32;
		pSurface->VPlaneOffset.iSurfaceOffset =
		    (pSurface->dwHeight * 3 / 2 -
		     (pSurface->dwHeight * 3 / 2) % 32) * pSurface->dwPitch;
		pSurface->VPlaneOffset.iYOffset =
		    (pSurface->dwHeight * 3 / 2) % 32;
		break;
	case Format_IMC4:
		pSurface->UPlaneOffset.iSurfaceOffset =
		    pSurface->dwHeight * pSurface->dwPitch;
		pSurface->UPlaneOffset.iYOffset = pSurface->dwHeight;
		pSurface->VPlaneOffset.iYOffset = pSurface->dwHeight * 3 / 2;
		break;
	case Format_411P:
		pSurface->UPlaneOffset.iSurfaceOffset =
		    pSurface->dwHeight * pSurface->dwPitch;
		pSurface->UPlaneOffset.iYOffset = 0;
		pSurface->VPlaneOffset.iSurfaceOffset =
		    pSurface->dwHeight * pSurface->dwPitch * 2;
		pSurface->VPlaneOffset.iYOffset = 0;
		break;
	case Format_444P:
		pSurface->UPlaneOffset.iSurfaceOffset =
		    pSurface->dwHeight * pSurface->dwPitch;
		pSurface->UPlaneOffset.iYOffset = 0;
		pSurface->VPlaneOffset.iSurfaceOffset =
		    pSurface->dwHeight * pSurface->dwPitch * 2;
		pSurface->VPlaneOffset.iYOffset = 0;
		break;

	default:
		break;
	}

	eStatus = GENOS_STATUS_SUCCESS;
	goto finish;

 finish:
	return eStatus;
}

GENOS_STATUS HalCm_GetSurface2DTileYPitch(PCM_HAL_STATE pState,
					  PCM_HAL_SURFACE2D_PARAM pParam)
{
	GENOS_STATUS hr;
	GENHW_SURFACE Surface;
	PGENHW_HW_INTERFACE pHwInterface;
	UINT iIndex;

	CM_ASSERT(pState);

	hr = GENOS_STATUS_UNKNOWN;
	pHwInterface = pState->pHwInterface;
	iIndex = pParam->dwHandle;

	GENOS_ZeroMemory(&Surface, sizeof(Surface));

	Surface.OsResource = pState->pUmdSurf2DTable[iIndex].OsResource;
	Surface.dwWidth = pState->pUmdSurf2DTable[iIndex].iWidth;
	Surface.dwHeight = pState->pUmdSurf2DTable[iIndex].iHeight;
	Surface.Format = pState->pUmdSurf2DTable[iIndex].format;
	Surface.dwDepth = 1;

	CM_CHK_GENOSSTATUS(IntelGen_GetSurfaceInfo(pHwInterface->pOsInterface,
						   &Surface));

	pParam->iPitch = Surface.dwPitch;

 finish:
	return hr;
}

GENOS_STATUS HalCm_Set2DSurfaceStateDimensions(PCM_HAL_STATE pState,
					       PCM_HAL_SURFACE2D_SURFACE_STATE_DIMENSIONS_PARAM
					       pParam)
{
	GENOS_STATUS hr;
	UINT width;
	UINT height;
	UINT iIndex;

	CM_CHK_NULL_RETURN_GENOSSTATUS(pState);
	CM_CHK_NULL_RETURN_GENOSSTATUS(pParam);

	hr = GENOS_STATUS_SUCCESS;
	width = pParam->iWidth;
	height = pParam->iHeight;
	iIndex = pParam->dwHandle;

	pState->pUmdSurf2DTable[iIndex].iSurfaceStateWidth = width;
	pState->pUmdSurf2DTable[iIndex].iSurfaceStateHeight = height;

 finish:
	return hr;
}

GENOS_STATUS HalCm_AllocateSurface2D(PCM_HAL_STATE pState,
				     PCM_HAL_SURFACE2D_PARAM pParam)
{
	GENOS_STATUS hr;
	PGENOS_INTERFACE pOsInterface;
	PCM_HAL_SURFACE2D_ENTRY pEntry = NULL;
	GENOS_ALLOC_GFXRES_PARAMS AllocParams;
	UINT i;
	PGENOS_RESOURCE pOsResource;

	CM_ASSERT(pParam->iWidth > 0);

	hr = GENOS_STATUS_SUCCESS;
	pOsInterface = pState->pHwInterface->pOsInterface;

	for (i = 0; i < pState->CmDeviceParam.iMax2DSurfaceTableSize; i++) {
		if (IntelGen_OsResourceIsNull
		    (&pState->pUmdSurf2DTable[i].OsResource)) {
			pEntry = &pState->pUmdSurf2DTable[i];
			pParam->dwHandle = (DWORD) i;
			break;
		}
	}

	if (!pEntry) {
		CM_ERROR_ASSERT("Surface2D table is full");
		goto finish;
	}

	pOsResource = &(pEntry->OsResource);
	IntelGen_OsResetResource(pOsResource);

	if (pParam->isAllocatedbyCmrtUmd) {
		GENOS_ZeroMemory(&AllocParams,
				 sizeof(GENOS_ALLOC_GFXRES_PARAMS));
		AllocParams.Type = GENOS_GFXRES_2D;
		AllocParams.dwWidth = pParam->iWidth;
		AllocParams.dwHeight = pParam->iHeight;
		AllocParams.pSystemMemory = pParam->pData;
		AllocParams.Format = pParam->format;
		AllocParams.TileType = GENOS_TILE_Y;
		AllocParams.pBufName = "CmSurface2D";

		CM_HRESULT2GENOSSTATUS_AND_CHECK
		    (pOsInterface->pfnAllocateResource
		     (pOsInterface, &AllocParams, &pEntry->OsResource));

		pEntry->iWidth = pParam->iWidth;
		pEntry->iHeight = pParam->iHeight;
		pEntry->format = pParam->format;
		pEntry->isAllocatedbyCmrtUmd = pParam->isAllocatedbyCmrtUmd;
		pEntry->iSurfaceStateWidth = pParam->iWidth;
		pEntry->iSurfaceStateHeight = pParam->iHeight;
	} else {
		pEntry->iWidth = pParam->iWidth;
		pEntry->iHeight = pParam->iHeight;
		pEntry->format = pParam->format;
		pEntry->isAllocatedbyCmrtUmd = FALSE;

		pOsResource->bMapped = FALSE;
		pOsResource->Format = pParam->format;
		pOsResource->iWidth = pParam->iWidth;
		pOsResource->iHeight = pParam->iHeight;
		pOsResource->iPitch = pParam->pCmOsResource->pitch;
		pOsResource->bo = pParam->pCmOsResource->bo;

		pOsResource->TileType =
		    OsToGenTileType(pParam->pCmOsResource->tile_type);

		if (pParam->pCmOsResource->bo_flags == DRM_BO_HANDLE) {
			HalCm_OsResource_Reference(&pEntry->OsResource);
		} else {
			//bo_flink
		}

		pEntry->iSurfaceStateWidth = pParam->iWidth;
		pEntry->iSurfaceStateHeight = pParam->iHeight;
	}

 finish:
	return hr;
}

GENOS_STATUS HalCm_FreeSurface2D(PCM_HAL_STATE pState, DWORD dwHandle)
{
	GENOS_STATUS hr;
	PCM_HAL_SURFACE2D_ENTRY pEntry;
	PGENOS_INTERFACE pOsInterface;

	hr = GENOS_STATUS_SUCCESS;
	pOsInterface = pState->pHwInterface->pOsInterface;

	CM_CHK_GENOSSTATUS(HalCm_GetSurface2DEntry(pState, dwHandle, &pEntry));
	if (pEntry->isAllocatedbyCmrtUmd) {
		pOsInterface->pfnFreeResource(pOsInterface,
					      &pEntry->OsResource);
	} else {
		HalCm_OsResource_Unreference(&pEntry->OsResource);
	}

	GENOS_ZeroMemory(&pEntry->OsResource, sizeof(pEntry->OsResource));

	pEntry->iWidth = 0;
	pEntry->iHeight = 0;
	pEntry->bReadSync = FALSE;

 finish:
	return hr;
}

GENOS_STATUS HalCm_Lock2DResource(PCM_HAL_STATE pState,
				  PCM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM pParam)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	GENOS_LOCK_PARAMS LockFlags;

	GENHW_SURFACE Surface;
	PGENOS_INTERFACE pOsInterface = NULL;

	if ((pParam->iLockFlag != CM_HAL_LOCKFLAG_READONLY)
	    && (pParam->iLockFlag != CM_HAL_LOCKFLAG_WRITEONLY)) {
		CM_ERROR_ASSERT("Invalid lock flag!");
		hr = GENOS_STATUS_UNKNOWN;
		goto finish;
	}

	GENOS_ZeroMemory(&Surface, sizeof(Surface));
	Surface.Format = Format_Invalid;
	pOsInterface = pState->pHwInterface->pOsInterface;

	if (pParam->pData == NULL) {
		PCM_HAL_SURFACE2D_ENTRY pEntry;

		CM_CHK_GENOSSTATUS(HalCm_GetSurface2DEntry
				   (pState, pParam->dwHandle, &pEntry));

		Surface.OsResource = pEntry->OsResource;

		CM_CHK_GENOSSTATUS(IntelGen_GetSurfaceInfo(pOsInterface,
							   &Surface));

		pParam->iPitch = Surface.dwPitch;
		GENOS_ZeroMemory(&LockFlags, sizeof(GENOS_LOCK_PARAMS));

		if (pParam->iLockFlag == CM_HAL_LOCKFLAG_READONLY) {
			LockFlags.ReadOnly = TRUE;
		} else {
			LockFlags.WriteOnly = TRUE;
		}

		pParam->pData = pOsInterface->pfnLockResource(pOsInterface,
							      &pEntry->OsResource,
							      &LockFlags);

	} else {
		CM_ASSERT(0);
		hr = GENOS_STATUS_UNKNOWN;
	}
	CM_CHK_NULL_RETURN_GENOSSTATUS(pParam->pData);

 finish:
	return hr;
}

GENOS_STATUS HalCm_Unlock2DResource(PCM_HAL_STATE pState,
				    PCM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM pParam)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	PGENOS_INTERFACE pOsInterface = pState->pHwInterface->pOsInterface;

	if (pParam->pData == NULL) {
		PCM_HAL_SURFACE2D_ENTRY pEntry;

		CM_CHK_GENOSSTATUS(HalCm_GetSurface2DEntry
				   (pState, pParam->dwHandle, &pEntry));

		CM_HRESULT2GENOSSTATUS_AND_CHECK(pOsInterface->pfnUnlockResource
						 (pOsInterface,
						  &(pEntry->OsResource)));
	} else {
		CM_ASSERT(0);
		hr = GENOS_STATUS_UNKNOWN;
	}

 finish:
	return hr;
}

GENOS_STATUS HalCm_SetCaps(PCM_HAL_STATE pState,
			   PCM_HAL_MAX_SET_CAPS_PARAM pSetCapsParam)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;

	CM_ASSERT(pState);
	CM_ASSERT(pSetCapsParam);

	switch (pSetCapsParam->Type) {
	case DXVA_CM_MAX_HW_THREADS:
		if (pSetCapsParam->MaxValue <= 0 ||
		    pSetCapsParam->MaxValue >
		    pState->pHwInterface->pHwCaps->dwMaxThreads) {
			hr = GENOS_STATUS_UNKNOWN;
			goto finish;
		} else {
			pState->MaxHWThreadValues.APIValue =
			    pSetCapsParam->MaxValue;
		}

		break;

	default:
		hr = GENOS_STATUS_UNKNOWN;
		goto finish;
	}

 finish:
	return hr;
}

GENOS_STATUS HalCm_SetPowerOption(PCM_HAL_STATE pState,
				  PCM_HAL_POWER_OPTION_PARAM pPowerOption)
{
	GENOS_SecureMemcpy(&pState->PowerOption, sizeof(pState->PowerOption),
			   pPowerOption, sizeof(pState->PowerOption));
	return GENOS_STATUS_SUCCESS;
}

GENOS_STATUS HalCm_Create(PGENOS_CONTEXT pOsDriverContext,
			  PCM_HAL_CREATE_PARAM pParam, PCM_HAL_STATE * pCmState)
{
	GENOS_STATUS hr;
	PCM_HAL_STATE pState = NULL;

	CM_ASSERT(pOsDriverContext);
	CM_ASSERT(pParam);
	CM_ASSERT(pCmState);

	hr = GENOS_STATUS_SUCCESS;

	pState = (PCM_HAL_STATE) GENOS_AllocAndZeroMemory(sizeof(CM_HAL_STATE));
	CM_CHK_NULL_RETURN_GENOSSTATUS(pState);

	pState->pOsInterface = (PGENOS_INTERFACE)
	    GENOS_AllocAndZeroMemory(sizeof(GENOS_INTERFACE));
	CM_CHK_NULL_RETURN_GENOSSTATUS(pState->pOsInterface);
	pState->pOsInterface->bDeallocateOnExit = TRUE;
	CM_HRESULT2GENOSSTATUS_AND_CHECK(IntelGen_OsInitInterface
					 (pState->pOsInterface,
					  pOsDriverContext, COMPONENT_CM));

	pState->pOsInterface->pfnGetPlatform(pState->pOsInterface,
					     &pState->Platform);

	pState->pHwInterface = (PGENHW_HW_INTERFACE)
	    GENOS_AllocAndZeroMemory(sizeof(GENHW_HW_INTERFACE));
	CM_CHK_NULL_RETURN_GENOSSTATUS(pState->pHwInterface);
	CM_CHK_GENOSSTATUS(IntelGen_HwInitInterface
			   (pState->pHwInterface, pState->pOsInterface));

	pState->CmDeviceParam.iMaxKernelBinarySize =
	    CM_KERNEL_BINARY_BLOCK_SIZE;

	if (pParam->DisableScratchSpace) {
		pState->CmDeviceParam.iMaxPerThreadScratchSpaceSize = 0;
	} else {

		if (pParam->ScratchSpaceSize ==
		    CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_DEFAULT) {
			pState->CmDeviceParam.iMaxPerThreadScratchSpaceSize =
			    8 * CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_16K_STEP;
		} else {
			pState->CmDeviceParam.iMaxPerThreadScratchSpaceSize =
			    (pParam->ScratchSpaceSize) *
			    CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_16K_STEP;
		}

	}

	pState->pHwInterface->bMediaReset = pParam->bMediaReset;

	pState->bRequestSingleSlice = pParam->bRequestSliceShutdown;

	pState->bSLMMode = pParam->bSLMMode;

	GENOS_ZeroMemory(&pState->HintIndexes.iKernelIndexes,
			 sizeof(UINT) * CM_MAX_TASKS_EU_SATURATION);
	GENOS_ZeroMemory(&pState->HintIndexes.iDispatchIndexes,
			 sizeof(UINT) * CM_MAX_TASKS_EU_SATURATION);

	pState->CmDeviceParam.iMaxKernelsPerTask = CM_MAX_KERNELS_PER_TASK;
	pState->CmDeviceParam.iMaxBufferTableSize =
	    CM_MAX_BUFFER_SURFACE_TABLE_SIZE;
	pState->CmDeviceParam.iMax2DSurfaceUPTableSize =
	    CM_MAX_2D_SURFACE_UP_TABLE_SIZE;
	pState->CmDeviceParam.iMax2DSurfaceTableSize =
	    CM_MAX_2D_SURFACE_TABLE_SIZE;
	pState->CmDeviceParam.iMaxTasks = pParam->MaxTaskNumber;

	pState->pfnCmAllocate = HalCm_Allocate;
	pState->pfnGetMaxValues = HalCm_GetMaxValues;
	pState->pfnGetMaxValuesEx = HalCm_GetMaxValuesEx;
	pState->pfnExecuteTask = HalCm_ExecuteTask;
	pState->pfnExecuteGroupTask = HalCm_ExecuteGroupTask;
	pState->pfnExecuteHintsTask = HalCm_ExecuteHintsTask;
	pState->pfnQueryTask = HalCm_QueryTask;
	pState->pfnFreeBuffer = HalCm_FreeBuffer;
	pState->pfnUpdateBuffer = HalCm_UpdateBuffer;
	pState->pfnUpdateSurface2D = HalCm_UpdateSurface2D;
	pState->pfnLockBuffer = HalCm_LockBuffer;
	pState->pfnUnlockBuffer = HalCm_UnlockBuffer;
	pState->pfnFreeSurface2DUP = HalCm_FreeSurface2DUP;
	pState->pfnGetSurface2DTileYPitch = HalCm_GetSurface2DTileYPitch;
	pState->pfnSet2DSurfaceStateDimensions =
	    HalCm_Set2DSurfaceStateDimensions;
	pState->pfnAllocateSurface2D = HalCm_AllocateSurface2D;
	pState->pfnFreeSurface2D = HalCm_FreeSurface2D;
	pState->pfnLock2DResource = HalCm_Lock2DResource;
	pState->pfnUnlock2DResource = HalCm_Unlock2DResource;

	pState->pfnSetCaps = HalCm_SetCaps;
	pState->pfnSetPowerOption = HalCm_SetPowerOption;

	pState->pfnSendMediaWalkerState = HalCm_SendMediaWalkerState;
	pState->pfnSendGpGpuWalkerState = HalCm_SendGpGpuWalkerState;
	pState->pfnSetSurfaceReadFlag = HalCm_SetSurfaceReadFlag;

	HalCm_OsInitInterface(pState);

	pState->MaxHWThreadValues.registryValue = 0;
	pState->MaxHWThreadValues.APIValue = 0;

	switch (pState->Platform.eRenderCoreFamily) {
	case IGFX_GEN7_5_CORE:
		pState->pfnSubmitCommands = HalCm_SubmitCommands_g75;
		pState->pfnGetTaskSyncLocation = HalCm_GetTaskSyncLocation_g75;
		pState->pfnGetCurbeBlockAlignSize =
		    HalCm_GetCurbeBlockAlignSize_g75;
		pState->pfnGetUserDefinedThreadCountPerThreadGroup =
		    HalCm_GetUserDefinedThreadCountPerThreadGroup_g75;
		pState->pfnSetSurfaceMemoryObjectControl =
		    HalCm_HwSetSurfaceMemoryObjectControl_g75;
		break;

	case IGFX_GEN8_CORE:
		pState->pfnSubmitCommands = HalCm_SubmitCommands_g8;
		pState->pfnGetTaskSyncLocation = HalCm_GetTaskSyncLocation_g75;
		pState->pfnGetCurbeBlockAlignSize =
		    HalCm_GetCurbeBlockAlignSize_g8;
		pState->pfnGetUserDefinedThreadCountPerThreadGroup =
		    HalCm_GetUserDefinedThreadCountPerThreadGroup_g8;
		pState->pfnSetSurfaceMemoryObjectControl =
		    HalCm_HwSetSurfaceMemoryObjectControl_g8;
		break;

	default:
		break;
	}

 finish:
	if (hr != GENOS_STATUS_SUCCESS) {
		HalCm_Destroy(pState);
		*pCmState = NULL;
	} else {
		*pCmState = pState;
	}

	return hr;
}

VOID HalCm_Destroy(PCM_HAL_STATE pState)
{
	GENOS_STATUS hr;
	INT i;

	if (pState) {
		if (pState->pBatchBuffers) {
			for (i = 0; i < pState->iNumBatchBuffers; i++) {
				if (!IntelGen_OsResourceIsNull
				    (&pState->pBatchBuffers[i].OsResource)) {
					hr = (GENOS_STATUS)
					    pState->
					    pHwInterface->pfnFreeBB(pState->
								    pHwInterface,
								    &pState->pBatchBuffers
								    [i]);

					if (hr != GENOS_STATUS_SUCCESS) {
						CM_ASSERT(hr ==
							  GENOS_STATUS_SUCCESS);
					}
				}
				GENOS_FreeMemory(pState->pBatchBuffers
						 [i].pBBRenderData);
			}
			GENOS_FreeMemory(pState->pBatchBuffers);
			pState->pBatchBuffers = NULL;
		}
		HalCm_FreeTsResource(pState);

		if (pState->hLibModule) {
			FreeLibrary(pState->hLibModule);
			pState->hLibModule = NULL;
		}
		if (pState->pHwInterface) {
			pState->pHwInterface->pfnDestroy(pState->pHwInterface);
			GENOS_FreeMemory(pState->pHwInterface);
			pState->pHwInterface = NULL;
		}
		if (pState->pOsInterface) {
			if (pState->pOsInterface->pfnDestroy) {
				pState->pOsInterface->
				    pfnDestroy(pState->pOsInterface, TRUE);
			}

			if (pState->pOsInterface->bDeallocateOnExit) {
				GENOS_FreeMemory(pState->pOsInterface);
				pState->pOsInterface = NULL;
			}
		}
		GENOS_FreeMemory(pState->pTaskParam);

		GENOS_FreeMemory(pState->pTaskTimeStamp);

		GENOS_FreeMemory(pState->pTableMem);

		GENOS_FreeMemory(pState);
	}
}

GENOS_STATUS HalCm_GetSurfaceDetails(PCM_HAL_STATE pCmState,
				     PCM_HAL_INDEX_PARAM pIndexParam,
				     UINT iBTIndex,
				     GENHW_SURFACE & Surface,
				     SHORT globalSurface,
				     PGENHW_SURFACE_STATE_ENTRY pSurfaceEntry,
				     UINT dwTempPlaneIndex,
				     GENHW_SURFACE_STATE_PARAMS SurfaceParam,
				     CM_HAL_KERNEL_ARG_KIND argKind)
{
	GENOS_STATUS hr = GENOS_STATUS_UNKNOWN;
	PCM_HAL_SURFACE_ENTRY_INFO pSurfaceInfos = NULL;
	PCM_HAL_SURFACE_ENTRY_INFO pgSurfaceInfos = NULL;
	PCM_HAL_TASK_PARAM pTaskParam = pCmState->pTaskParam;
	UINT iCurKrnIndex = pTaskParam->iCurKrnIndex;
	PGENHW_HW_INTERFACE pHwInterface = pCmState->pHwInterface;
	PGENHW_PLANE_OFFSET pPlaneOffset = 0;
	UINT maxEntryNum = 0;
	GENOS_OS_FORMAT tempOsFormat;

	if (iCurKrnIndex + 1 > pTaskParam->SurEntryInfoArrays.dwKrnNum) {
		CM_ERROR_ASSERT
		    ("Mismatched kernel index: iCurKrnIndex '%d' vs dwKrnNum '%d'",
		     iCurKrnIndex, pTaskParam->SurEntryInfoArrays.dwKrnNum);
		goto finish;
	}

	pSurfaceInfos =
	    pTaskParam->SurEntryInfoArrays.
	    pSurfEntryInfosArray[iCurKrnIndex].pSurfEntryInfos;
	pgSurfaceInfos =
	    pTaskParam->SurEntryInfoArrays.
	    pSurfEntryInfosArray[iCurKrnIndex].pGlobalSurfInfos;

	tempOsFormat =
	    pHwInterface->pOsInterface->pfnFmt_GenToOs(Surface.Format);

	switch (argKind) {
	case CM_ARGUMENT_SURFACEBUFFER:

		if ((iBTIndex >=
		     (UINT) CM_BINDING_START_INDEX_OF_GLOBAL_SURFACE(pCmState))
		    && (iBTIndex < (UINT)
			CM_BINDING_START_INDEX_OF_GLOBAL_SURFACE(pCmState) +
			CM_MAX_GLOBAL_SURFACE_NUMBER)) {
			iBTIndex =
			    iBTIndex -
			    CM_BINDING_START_INDEX_OF_GLOBAL_SURFACE(pCmState);

			maxEntryNum =
			    pTaskParam->
			    SurEntryInfoArrays.pSurfEntryInfosArray->
			    dwGlobalSurfNum;
			if (iBTIndex >= maxEntryNum) {
				CM_ERROR_ASSERT
				    ("Array for surface details is full: Max number of entries '%d' and trying to add index '%d'",
				     maxEntryNum, iBTIndex);
				goto finish;
			}

			GENOS_ZeroMemory(&pgSurfaceInfos[iBTIndex],
					 sizeof(CM_HAL_SURFACE_ENTRY_INFO));
			pgSurfaceInfos[iBTIndex].dwWidth = Surface.dwWidth;
			pgSurfaceInfos[iBTIndex].dwFormat = GFX_DDIFMT_UNKNOWN;
		} else {
			iBTIndex =
			    iBTIndex -
			    CM_BINDING_START_INDEX_OF_GENERAL_SURFACE(pCmState);
			maxEntryNum =
			    pTaskParam->
			    SurEntryInfoArrays.pSurfEntryInfosArray->
			    dwMaxEntryNum;
			if (iBTIndex >= maxEntryNum) {
				CM_ERROR_ASSERT
				    ("Array for surface details is full: Max number of entries '%d' and trying to add index '%d'",
				     maxEntryNum, iBTIndex);
				goto finish;
			}

			GENOS_ZeroMemory(&pSurfaceInfos[iBTIndex],
					 sizeof(CM_HAL_SURFACE_ENTRY_INFO));
			pSurfaceInfos[iBTIndex].dwWidth = Surface.dwWidth;
			pSurfaceInfos[iBTIndex].dwFormat = GFX_DDIFMT_UNKNOWN;
		}

		if (globalSurface < 0) {
			++pTaskParam->SurEntryInfoArrays.pSurfEntryInfosArray
			    [iCurKrnIndex].dwUsedIndex;
		}

		hr = GENOS_STATUS_SUCCESS;
		break;

	case CM_ARGUMENT_SURFACE2D_UP:
	case CM_ARGUMENT_SURFACE2D:
		iBTIndex =
		    iBTIndex -
		    CM_BINDING_START_INDEX_OF_GENERAL_SURFACE(pCmState);
		maxEntryNum =
		    pTaskParam->SurEntryInfoArrays.
		    pSurfEntryInfosArray->dwMaxEntryNum;

		if (iBTIndex >= maxEntryNum) {
			CM_ERROR_ASSERT
			    ("Array for surface details is full: Max number of entries '%d' and trying to add index '%d'",
			     maxEntryNum, iBTIndex);
			goto finish;
		}

		pSurfaceInfos[iBTIndex].dwWidth = pSurfaceEntry->dwWidth;
		pSurfaceInfos[iBTIndex].dwHeight = pSurfaceEntry->dwHeight;
		pSurfaceInfos[iBTIndex].dwDepth = 0;
		pSurfaceInfos[iBTIndex].dwFormat = (GFX_DDIFORMAT) tempOsFormat;
		pSurfaceInfos[iBTIndex].dwPlaneIndex = dwTempPlaneIndex;
		pSurfaceInfos[iBTIndex].dwPitch = pSurfaceEntry->dwPitch;
		pSurfaceInfos[iBTIndex].dwSurfaceBaseAddress = 0;
		pSurfaceInfos[iBTIndex].u8TileWalk = pSurfaceEntry->bTileWalk;
		pSurfaceInfos[iBTIndex].u8TiledSurface =
		    pSurfaceEntry->bTiledSurface;

		if (pSurfaceEntry->YUVPlane == GENHW_U_PLANE ||
		    pSurfaceEntry->YUVPlane == GENHW_V_PLANE) {
			pPlaneOffset =
			    (pSurfaceEntry->YUVPlane == GENHW_U_PLANE)
			    ? &Surface.UPlaneOffset : &Surface.VPlaneOffset;

			pSurfaceInfos[iBTIndex].dwYOffset =
			    pPlaneOffset->iYOffset >> 1;

			if (argKind == CM_ARGUMENT_SURFACE2D_UP) {
				pSurfaceInfos[iBTIndex].dwXOffset =
				    (pPlaneOffset->iXOffset /
				     (DWORD) sizeof(DWORD)) >> 2;
			} else {
				DWORD dwPixelsPerSampleUV = 0;
				if (SurfaceParam.bWidthInDword_UV) {
					IntelGen_GetPixelsPerSample
					    (Surface.Format,
					     &dwPixelsPerSampleUV);
				} else {
					dwPixelsPerSampleUV = 1;
				}

				if (dwPixelsPerSampleUV == 1) {
					pSurfaceInfos[iBTIndex].dwXOffset =
					    pPlaneOffset->iXOffset >> 2;
				} else {
					pSurfaceInfos[iBTIndex].dwXOffset =
					    (pPlaneOffset->iXOffset /
					     (DWORD) sizeof(DWORD)) >> 2;
				}
			}
		} else {
			pSurfaceInfos[iBTIndex].dwXOffset =
			    (Surface.YPlaneOffset.iXOffset /
			     (DWORD) sizeof(DWORD)) >> 2;
			pSurfaceInfos[iBTIndex].dwYOffset =
			    Surface.YPlaneOffset.iYOffset >> 1;
		}

		++pTaskParam->
		    SurEntryInfoArrays.pSurfEntryInfosArray[iCurKrnIndex].
		    dwUsedIndex;
		++dwTempPlaneIndex;

		hr = GENOS_STATUS_SUCCESS;
		break;

	case CM_ARGUMENT_SURFACE3D:

		iBTIndex =
		    iBTIndex -
		    CM_BINDING_START_INDEX_OF_GENERAL_SURFACE(pCmState);
		maxEntryNum =
		    pTaskParam->SurEntryInfoArrays.
		    pSurfEntryInfosArray->dwMaxEntryNum;

		if (iBTIndex >= maxEntryNum) {
			CM_ERROR_ASSERT
			    ("Array for surface details is full: Max number of entries '%d' and trying to add index '%d'",
			     maxEntryNum, iBTIndex);
			goto finish;
		}

		pSurfaceInfos[iBTIndex].dwWidth = pSurfaceEntry->dwWidth;
		pSurfaceInfos[iBTIndex].dwHeight = pSurfaceEntry->dwHeight;
		pSurfaceInfos[iBTIndex].dwDepth = Surface.dwDepth;
		pSurfaceInfos[iBTIndex].dwFormat = (GFX_DDIFORMAT) tempOsFormat;
		pSurfaceInfos[iBTIndex].dwPitch = pSurfaceEntry->dwPitch;
		pSurfaceInfos[iBTIndex].dwPlaneIndex = dwTempPlaneIndex;
		pSurfaceInfos[iBTIndex].dwSurfaceBaseAddress = 0;
		pSurfaceInfos[iBTIndex].u8TileWalk = pSurfaceEntry->bTileWalk;
		pSurfaceInfos[iBTIndex].u8TiledSurface =
		    pSurfaceEntry->bTiledSurface;

		if (pSurfaceEntry->YUVPlane == GENHW_U_PLANE ||
		    pSurfaceEntry->YUVPlane == GENHW_V_PLANE) {
			pPlaneOffset =
			    (pSurfaceEntry->YUVPlane == GENHW_U_PLANE)
			    ? &Surface.UPlaneOffset : &Surface.VPlaneOffset;

			pSurfaceInfos[iBTIndex].dwYOffset =
			    pPlaneOffset->iYOffset >> 1;
			pSurfaceInfos[iBTIndex].dwXOffset =
			    (pPlaneOffset->iXOffset /
			     (DWORD) sizeof(DWORD)) >> 2;
		} else {
			pSurfaceInfos[iBTIndex].dwXOffset =
			    (Surface.YPlaneOffset.iXOffset /
			     (DWORD) sizeof(DWORD)) >> 2;
			pSurfaceInfos[iBTIndex].dwYOffset =
			    Surface.YPlaneOffset.iYOffset >> 1;
		}

		++dwTempPlaneIndex;
		++pTaskParam->
		    SurEntryInfoArrays.pSurfEntryInfosArray[iCurKrnIndex].
		    dwUsedIndex;

		hr = GENOS_STATUS_SUCCESS;
		break;

	default:
		break;
	}

 finish:
	return hr;
}

DWORD Halcm_GetFreeBindingIndex(PCM_HAL_STATE pState,
				PCM_HAL_INDEX_PARAM pIndexParam, DWORD total)
{
	DWORD bt_index = CM_BINDING_START_INDEX_OF_GENERAL_SURFACE(pState);
	DWORD un_allocated = total;

	while (bt_index < 256 && un_allocated > 0) {
		DWORD array_index = bt_index >> 5;
		DWORD bit_mask = 1 << (bt_index % 32);
		if (pIndexParam->dwBTArray[array_index] & bit_mask) {
			if (un_allocated != total) {
				DWORD allocated = total - un_allocated;
				DWORD tmp_index = bt_index - 1;
				while (allocated > 0) {
					DWORD array_index = tmp_index >> 5;
					DWORD bit_mask = 1 << (tmp_index % 32);
					pIndexParam->dwBTArray[array_index] &=
					    ~bit_mask;
					allocated--;
					tmp_index--;
				}
				un_allocated = total;
			}
		} else {
			pIndexParam->dwBTArray[array_index] |= bit_mask;
			un_allocated--;
		}
		bt_index++;
	}

	if (un_allocated == 0) {
		return bt_index - total;
	}
	return 0;
}

void Halcm_PreSetBindingIndex(PCM_HAL_INDEX_PARAM pIndexParam,
			      DWORD start, DWORD end)
{
	DWORD bt_index;
	for (bt_index = start; bt_index <= end; bt_index++) {
		DWORD array_index = bt_index >> 5;
		DWORD bit_mask = 1 << (bt_index % 32);
		pIndexParam->dwBTArray[array_index] |= bit_mask;
	}
}

GENOS_STATUS HalCm_Setup2DSurfaceStateWithBTIndex(PCM_HAL_STATE pState,
						  INT iBindingTable,
						  UINT surfIndex, UINT btIndex)
{
	PGENHW_HW_INTERFACE pHwInterface = pState->pHwInterface;
	GENOS_STATUS hr;
	GENHW_SURFACE Surface;
	GENHW_SURFACE_STATE_PARAMS SurfaceParam;
	PGENHW_SURFACE_STATE_ENTRY pSurfaceEntries[GENHW_MAX_SURFACE_PLANES];
	INT iSurfaceEntries, i;
	WORD memObjCtl = CM_DEFAULT_CACHE_TYPE;

	hr = GENOS_STATUS_UNKNOWN;
	iSurfaceEntries = 0;

	if (surfIndex == CM_NULL_SURFACE) {
		return GENOS_STATUS_SUCCESS;
	}
	if (surfIndex >= pState->CmDeviceParam.iMax2DSurfaceTableSize ||
	    IntelGen_OsResourceIsNull(&pState->
				      pUmdSurf2DTable[surfIndex].OsResource)) {
		CM_ERROR_ASSERT("Invalid 2D Surface array index '%d'",
				surfIndex);
		return GENOS_STATUS_UNKNOWN;
	}
	UINT nBTInTable = (unsigned char)CM_INVALID_INDEX;
	nBTInTable = pState->pBT2DIndexTable[surfIndex].RegularSurfIndex;

	if (btIndex == nBTInTable) {
		return GENOS_STATUS_SUCCESS;
	}
	CM_CHK_GENOSSTATUS(HalCm_GetSurfaceAndRegister
			   (pState, &Surface, CM_ARGUMENT_SURFACE2D,
			    surfIndex));

	GENOS_ZeroMemory(&SurfaceParam, sizeof(SurfaceParam));
	SurfaceParam.Type = pHwInterface->SurfaceTypeDefault;
	SurfaceParam.bRenderTarget = FALSE;
	SurfaceParam.bWidthInDword_UV = TRUE;
	SurfaceParam.bWidthInDword_Y = TRUE;

	pState->pfnSetSurfaceMemoryObjectControl(pState,
						 memObjCtl, &SurfaceParam);

	CM_CHK_GENOSSTATUS(pHwInterface->pfnSetupSurfaceState(pHwInterface,
							      &Surface,
							      &SurfaceParam,
							      &iSurfaceEntries,
							      pSurfaceEntries));

	for (i = 0; i < iSurfaceEntries; i++) {
		CM_CHK_GENOSSTATUS(pHwInterface->pfnBindSurfaceState
				   (pHwInterface, iBindingTable, btIndex + i,
				    pSurfaceEntries[i]));
	}

	pState->pBT2DIndexTable[surfIndex].RegularSurfIndex = btIndex;

	hr = GENOS_STATUS_SUCCESS;

 finish:
	return hr;
}

GENOS_STATUS HalCm_SetupBufferSurfaceStateWithBTIndex(PCM_HAL_STATE pState,
						      INT iBindingTable,
						      UINT surfIndex,
						      UINT btIndex)
{
	PGENHW_HW_INTERFACE pHwInterface = pState->pHwInterface;
	GENOS_STATUS hr;
	GENHW_SURFACE Surface;
	GENHW_SURFACE_STATE_PARAMS SurfaceParam;
	PGENHW_SURFACE_STATE_ENTRY pSurfaceEntry;
	WORD memObjCtl = CM_DEFAULT_CACHE_TYPE;

	hr = GENOS_STATUS_UNKNOWN;

	if (surfIndex == CM_NULL_SURFACE) {
		return GENOS_STATUS_SUCCESS;
	}
	if (btIndex ==
	    (UINT) pState->pBTBufferIndexTable[surfIndex].RegularSurfIndex) {
		return GENOS_STATUS_SUCCESS;
	}
	CM_CHK_GENOSSTATUS(HalCm_GetSurfaceAndRegister
			   (pState, &Surface, CM_ARGUMENT_SURFACEBUFFER,
			    surfIndex));

	GENOS_ZeroMemory(&SurfaceParam, sizeof(SurfaceParam));

	pState->pfnSetSurfaceMemoryObjectControl(pState,
						 memObjCtl, &SurfaceParam);

	CM_CHK_GENOSSTATUS(pHwInterface->pfnSetupBufferSurfaceState
			   (pHwInterface, &Surface, &SurfaceParam,
			    &pSurfaceEntry));

	CM_CHK_GENOSSTATUS(pHwInterface->pfnBindSurfaceState(pHwInterface,
							     iBindingTable,
							     btIndex,
							     pSurfaceEntry));

	pState->pBTBufferIndexTable[surfIndex].RegularSurfIndex = btIndex;

	hr = GENOS_STATUS_SUCCESS;

 finish:
	return hr;
}

GENOS_STATUS HalCm_Setup2DSurfaceUPStateWithBTIndex(PCM_HAL_STATE pState,
						    INT iBindingTable,
						    UINT surfIndex,
						    UINT btIndex)
{
	GENOS_STATUS hr;
	GENHW_SURFACE Surface;
	GENHW_SURFACE_STATE_PARAMS SurfaceParam;
	PGENHW_HW_INTERFACE pHwInterface;
	PGENHW_SURFACE_STATE_ENTRY pSurfaceEntries[GENHW_MAX_SURFACE_PLANES];
	INT iSurfaceEntries, i;
	WORD memObjCtl = CM_DEFAULT_CACHE_TYPE;

	hr = GENOS_STATUS_UNKNOWN;
	pHwInterface = pState->pHwInterface;

	if (surfIndex == CM_NULL_SURFACE) {
		return GENOS_STATUS_SUCCESS;
	}
	UINT nBTInTable = (unsigned char)CM_INVALID_INDEX;
	nBTInTable = pState->pBT2DUPIndexTable[surfIndex].RegularSurfIndex;

	if (btIndex == nBTInTable) {
		return GENOS_STATUS_SUCCESS;
	}
	CM_CHK_GENOSSTATUS(HalCm_GetSurfaceAndRegister
			   (pState, &Surface, CM_ARGUMENT_SURFACE2D_UP,
			    surfIndex));

	GENOS_ZeroMemory(&SurfaceParam, sizeof(SurfaceParam));
	SurfaceParam.Type = pHwInterface->SurfaceTypeDefault;

	pState->pfnSetSurfaceMemoryObjectControl(pState,
						 memObjCtl, &SurfaceParam);

	CM_CHK_GENOSSTATUS(pHwInterface->pfnSetupSurfaceState(pHwInterface,
							      &Surface,
							      &SurfaceParam,
							      &iSurfaceEntries,
							      pSurfaceEntries));

	for (i = 0; i < iSurfaceEntries; i++) {
		CM_CHK_GENOSSTATUS(pHwInterface->pfnBindSurfaceState
				   (pHwInterface, iBindingTable, btIndex + i,
				    pSurfaceEntries[i]));
	}

	pState->pBT2DUPIndexTable[surfIndex].RegularSurfIndex = btIndex;

	hr = GENOS_STATUS_SUCCESS;

 finish:
	return hr;
}

GENOS_STATUS HalCm_SendMediaWalkerState(PCM_HAL_STATE pState,
					PCM_HAL_KERNEL_PARAM pKernelParam,
					PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	PGENHW_HW_INTERFACE pHwInterface;
	GENHW_WALKER_PARAMS MediaWalkerParams = { 0 };
	GENOS_STATUS hr;

	hr = GENOS_STATUS_SUCCESS;
	pHwInterface = pState->pHwInterface;

	GENOS_SecureMemcpy(&MediaWalkerParams, sizeof(GENHW_WALKER_PARAMS),
			   &pKernelParam->WalkerParams,
			   sizeof(CM_HAL_WALKER_PARAMS));

	if (pKernelParam->CmKernelThreadSpaceParam.iThreadSpaceWidth) {
		MediaWalkerParams.ScoreboardMask =
		    pKernelParam->CmKernelThreadSpaceParam.globalDependencyMask;
	} else {
		MediaWalkerParams.ScoreboardMask =
		    pHwInterface->VfeScoreboard.ScoreboardMask;
	}

	hr = pHwInterface->pfnSendMediaObjectWalker(pHwInterface, pCmdBuffer,
						    &MediaWalkerParams);

	return hr;
}

GENOS_STATUS HalCm_SendGpGpuWalkerState(PCM_HAL_STATE pState,
					PCM_HAL_KERNEL_PARAM pKernelParam,
					PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	PGENHW_HW_INTERFACE pHwInterface;
	GENHW_GPGPU_WALKER_PARAMS GpGpuWalkerParams = { 0 };
	GENOS_STATUS hr;

	hr = GENOS_STATUS_SUCCESS;
	pHwInterface = pState->pHwInterface;

	GpGpuWalkerParams.InterfaceDescriptorOffset =
	    pKernelParam->GpGpuWalkerParams.InterfaceDescriptorOffset;
	GpGpuWalkerParams.GpGpuEnable =
	    pKernelParam->GpGpuWalkerParams.CmGpGpuEnable;
	GpGpuWalkerParams.GroupWidth =
	    pKernelParam->GpGpuWalkerParams.GroupWidth;
	GpGpuWalkerParams.GroupHeight =
	    pKernelParam->GpGpuWalkerParams.GroupHeight;
	GpGpuWalkerParams.ThreadWidth =
	    pKernelParam->GpGpuWalkerParams.ThreadWidth;
	GpGpuWalkerParams.ThreadHeight =
	    pKernelParam->GpGpuWalkerParams.ThreadHeight;
	GpGpuWalkerParams.SLMSize = pKernelParam->GpGpuWalkerParams.SLMSize;

	hr = pHwInterface->pfnSendGpGpuWalkerState(pHwInterface, pCmdBuffer,
						   &GpGpuWalkerParams);

	return hr;
}

#define Y_TILE_WIDTH  64
#define Y_TILE_HEIGHT 32
#define X_TILE_WIDTH  512
#define X_TILE_HEIGHT 8
#define ROUND_UP_TO(x, y) (((x) + (y) - 1) / (y) * (y))

VOID HalCm_OsResource_Reference(PGENOS_RESOURCE pOsResource)
{
	if (pOsResource && pOsResource->bo) {
		drm_intel_bo_reference((drm_intel_bo *) (pOsResource->bo));
	}
}

VOID HalCm_OsResource_Unreference(PGENOS_RESOURCE pOsResource)
{
	if (pOsResource && pOsResource->bo) {
		drm_intel_bo_unreference((drm_intel_bo *) (pOsResource->bo));
		pOsResource->bo = NULL;
	}
}

GENOS_STATUS HalCm_GetSurfaceAndRegister(PCM_HAL_STATE pState,
					 PGENHW_SURFACE pSurface,
					 CM_HAL_KERNEL_ARG_KIND surfKind,
					 UINT iIndex)
{
	GENOS_STATUS hr;
	PGENHW_HW_INTERFACE pHwInterface;

	hr = GENOS_STATUS_UNKNOWN;
	pHwInterface = pState->pHwInterface;

	if (!pSurface) {
		goto finish;
	} else {
		GENOS_ZeroMemory(pSurface, sizeof(GENHW_SURFACE));
	}

	switch (surfKind) {
	case CM_ARGUMENT_SURFACEBUFFER:
		pSurface->dwWidth = pState->pBufferTable[iIndex].iSize;
		pSurface->dwHeight = 1;
		pSurface->Format = Format_Buffer;
		pSurface->rcSrc.right = pSurface->dwWidth;
		pSurface->rcSrc.bottom = pSurface->dwHeight;
		pSurface->rcDst = pSurface->rcSrc;
		CM_HRESULT2GENOSSTATUS_AND_CHECK(pHwInterface->
						 pOsInterface->pfnRegisterResource
						 (pHwInterface->pOsInterface,
						  &(pState->
						    pBufferTable[iIndex].
						    OsResource), TRUE, TRUE));
		pSurface->OsResource = pState->pBufferTable[iIndex].OsResource;

		break;

	case CM_ARGUMENT_SURFACE2D:
		CM_HRESULT2GENOSSTATUS_AND_CHECK(pHwInterface->
						 pOsInterface->pfnRegisterResource
						 (pHwInterface->pOsInterface,
						  &(pState->pUmdSurf2DTable
						    [iIndex].OsResource), TRUE,
						  TRUE));

		pSurface->OsResource =
		    pState->pUmdSurf2DTable[iIndex].OsResource;
		pSurface->dwWidth = pState->pUmdSurf2DTable[iIndex].iWidth;
		pSurface->dwHeight = pState->pUmdSurf2DTable[iIndex].iHeight;
		pSurface->Format = pState->pUmdSurf2DTable[iIndex].format;
		pSurface->dwDepth = 1;

		CM_CHK_GENOSSTATUS(IntelGen_GetSurfaceInfo
				   (pHwInterface->pOsInterface, pSurface));

		pSurface->rcSrc.right = pSurface->dwWidth;
		pSurface->rcSrc.bottom = pSurface->dwHeight;
		pSurface->rcDst = pSurface->rcSrc;

		break;

	case CM_ARGUMENT_SURFACE2D_UP:
		CM_HRESULT2GENOSSTATUS_AND_CHECK(pHwInterface->
						 pOsInterface->pfnRegisterResource
						 (pHwInterface->pOsInterface,
						  &(pState->pSurf2DUPTable
						    [iIndex].OsResource), TRUE,
						  TRUE));

		pSurface->OsResource =
		    pState->pSurf2DUPTable[iIndex].OsResource;
		pSurface->dwWidth = pState->pSurf2DUPTable[iIndex].iWidth;
		pSurface->dwHeight = pState->pSurf2DUPTable[iIndex].iHeight;
		pSurface->Format = pState->pSurf2DUPTable[iIndex].format;
		pSurface->dwDepth = 1;
		pSurface->TileType = GENOS_TILE_LINEAR;
		pSurface->dwOffset = 0;
		pSurface->SurfType = SURF_OUT_RENDERTARGET;

		CM_CHK_GENOSSTATUS(IntelGen_GetSurfaceInfo
				   (pHwInterface->pOsInterface, pSurface));

		pSurface->rcSrc.right = pSurface->dwWidth;
		pSurface->rcSrc.bottom = pSurface->dwHeight;
		pSurface->rcDst = pSurface->rcSrc;
		break;

	default:
		CM_ERROR_ASSERT("Argument kind '%d' is not supported",
				surfKind);
		goto finish;
	}

	hr = GENOS_STATUS_SUCCESS;
 finish:
	return hr;
}

GENOS_STATUS HalCm_GetSurfPitchSize(UINT Width, UINT Height,
				    GENOS_FORMAT Format, UINT * pPitch,
				    UINT * pPhysicalSize)
{

	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	UINT iPitch = 0;
	UINT iSize = 0;
	UINT iWidth = ROUND_UP_TO(Width, Y_TILE_WIDTH);;
	UINT iHeight = ROUND_UP_TO(Height, Y_TILE_HEIGHT);;

	switch (Format) {
	case Format_L8:
	case Format_P8:
	case Format_A8:
		iPitch = iWidth;
		iSize = iPitch * iHeight;
		break;
	case Format_NV12:
	case Format_YV12:
		iPitch = iWidth;
		if (iHeight & 0x1) {
			CM_ASSERT(0);
			hr = GENOS_STATUS_INVALID_PARAMETER;
			break;
		}
		iSize = iPitch * (3 * iHeight / 2);
		break;

	case Format_X8R8G8B8:
	case Format_A8R8G8B8:
	case Format_A8B8G8R8:
	case Format_R32F:
		iPitch = 4 * iWidth;
		iSize = iPitch * iHeight;
		break;

	case Format_V8U8:
	case Format_YUY2:
	case Format_UYVY:
		iPitch = 2 * iWidth;
		iSize = iPitch * iHeight;
		break;

	case Format_411P:
	case Format_422H:
	case Format_444P:
		iPitch = iWidth;
		iSize = iPitch * (3 * iHeight);
		break;

	case Format_IMC3:
	case Format_422V:
		iPitch = iWidth;
		iSize = iPitch * (2 * iHeight);
		break;

	default:
		CM_ASSERT(0);
		break;
	}

	*pPitch = iPitch;
	*pPhysicalSize = ROUND_UP_TO(iSize, CM_PAGE_ALIGNMENT);
	return hr;
}

GENOS_STATUS HalCm_GetSurface2DPitchAndSize(PCM_HAL_STATE pState,
					    PCM_HAL_SURFACE2D_UP_PARAM pParam)
{

	return HalCm_GetSurfPitchSize(pParam->iWidth, pParam->iHeight,
				      pParam->format, &pParam->iPitch,
				      &pParam->iPhysicalSize);
}

UINT HalCm_GetSurf2DUPBaseWidth(UINT Width, UINT Pitch, GENOS_FORMAT format)
{
	UINT BaseWidth = Width;
	UINT PixelSize = 1;

	switch (format) {
	case Format_L8:
	case Format_P8:
	case Format_A8:
	case Format_NV12:
		PixelSize = 1;
		break;

	case Format_X8R8G8B8:
	case Format_A8R8G8B8:
	case Format_A8B8G8R8:
	case Format_R32F:
		PixelSize = 4;
		break;

	case Format_V8U8:
	case Format_YUY2:
	case Format_UYVY:
		PixelSize = 2;
		break;

	default:
		CM_ASSERT(0);
		break;
	}

	BaseWidth = Pitch / PixelSize;
	return BaseWidth;
}

GENOS_STATUS HalCm_AllocateBuffer(PCM_HAL_STATE pState,
				  PCM_HAL_BUFFER_PARAM pParam)
{
	GENOS_STATUS hr;
	PGENOS_INTERFACE pOsInterface;
	PCM_HAL_BUFFER_ENTRY pEntry = NULL;
	GENOS_ALLOC_GFXRES_PARAMS AllocParams;
	UINT i;
	UINT iSize;
	UINT tileformat;
	PGENOS_RESOURCE pOsResource;
	drm_intel_bo *bo = NULL;

	iSize = pParam->iSize;
	tileformat = I915_TILING_NONE;

	CM_ASSERT(pParam->iSize > 0);

	hr = GENOS_STATUS_SUCCESS;
	pOsInterface = pState->pHwInterface->pOsInterface;

	for (i = 0; i < pState->CmDeviceParam.iMaxBufferTableSize; i++) {
		if (pState->pBufferTable[i].iSize == 0) {
			pEntry = &pState->pBufferTable[i];
			pParam->dwHandle = (DWORD) i;
			break;
		}
	}

	if (!pEntry) {
		CM_ERROR_ASSERT("Buffer table is full");
		goto finish;
	}

	pOsResource = &(pEntry->OsResource);
	IntelGen_OsResetResource(pOsResource);

	if (pParam->isAllocatedbyCmrtUmd) {
		if (pParam->pData == NULL) {
			GENOS_ZeroMemory(&AllocParams,
					 sizeof(GENOS_ALLOC_GFXRES_PARAMS));
			AllocParams.Type = GENOS_GFXRES_BUFFER;
			AllocParams.TileType = GENOS_TILE_LINEAR;
			AllocParams.dwBytes = pParam->iSize;
			AllocParams.pSystemMemory = pParam->pData;
			AllocParams.Format = Format_Buffer;
			AllocParams.pBufName = "CmBuffer";

			CM_HRESULT2GENOSSTATUS_AND_CHECK
			    (pOsInterface->pfnAllocateResource
			     (pOsInterface, &AllocParams, &pEntry->OsResource));
		} else {
			if (pState->hLibModule && pState->pDrmVMap) {
				bo = pState->pDrmVMap(pOsInterface->
						      pOsContext->bufmgr,
						      "CM Buffer UP",
						      (void *)(pParam->pData),
						      tileformat,
						      ROUND_UP_TO(iSize,
								  GENOS_PAGE_SIZE),
						      ROUND_UP_TO(iSize,
								  GENOS_PAGE_SIZE),
						      0);
			}

			pOsResource->bMapped = FALSE;
			if (bo) {
				pOsResource->Format = Format_Buffer;
				pOsResource->iWidth =
				    ROUND_UP_TO(iSize, GENOS_PAGE_SIZE);
				pOsResource->iHeight = 1;
				pOsResource->iPitch =
				    ROUND_UP_TO(iSize, GENOS_PAGE_SIZE);
				pOsResource->bo = bo;
				pOsResource->TileType =
				    OsToGenTileType(tileformat);
				pOsResource->pData = (PBYTE) bo->virt;
			} else {
				CM_DDI_ASSERTMESSAGE
				    ("Fail to Alloc BufferUP %7d bytes (%d x %d %s resource)\n",
				     iSize, iSize, 1, "BufferUP");
				hr = GENOS_STATUS_UNKNOWN;
			}
		}
	} else {
		pOsResource->bMapped = FALSE;
		pOsResource->Format = Format_Buffer;
		pOsResource->iWidth = pParam->pCmOsResource->orig_width;
		pOsResource->iHeight = 1;
		pOsResource->iPitch = pParam->pCmOsResource->orig_width;
		pOsResource->bo = pParam->pCmOsResource->bo;

		pOsResource->TileType =
		    OsToGenTileType(pParam->pCmOsResource->tile_type);

		if (pParam->pCmOsResource->bo_flags == DRM_BO_HANDLE) {
			HalCm_OsResource_Reference(&pEntry->OsResource);
		} else {
			//bo_flink
		}
		pParam->iSize = pParam->pCmOsResource->orig_width;
	}

	pEntry->iSize = pParam->iSize;
	pEntry->isAllocatedbyCmrtUmd = pParam->isAllocatedbyCmrtUmd;

 finish:
	return hr;
}

GENOS_STATUS HalCm_AllocateSurface2DUP(PCM_HAL_STATE pState,
				       PCM_HAL_SURFACE2D_UP_PARAM pParam)
{
	GENOS_STATUS hr;
	PGENOS_INTERFACE pOsInterface;
	PCM_HAL_SURFACE2D_UP_ENTRY pEntry = NULL;
	UINT i;

	PGENOS_RESOURCE pOsResource;
	GENOS_FORMAT Format;
	UINT iHeight;
	UINT iWidth;
	UINT iSize = 0;
	UINT align_x = 0;
	drm_intel_bo *bo = NULL;
	PVOID pSysMem = NULL;
	UINT tileformat = I915_TILING_NONE;

	CM_ASSERT(pState);
	CM_ASSERT(pParam->iWidth > 0);

	hr = GENOS_STATUS_SUCCESS;
	pOsInterface = pState->pHwInterface->pOsInterface;

	for (i = 0; i < pState->CmDeviceParam.iMax2DSurfaceUPTableSize; i++) {
		if (pState->pSurf2DUPTable[i].iWidth == 0) {
			pEntry = &pState->pSurf2DUPTable[i];
			pParam->dwHandle = (DWORD) i;
			break;
		}
	}

	if (!pEntry) {
		CM_ERROR_ASSERT("Surface2DUP table is full");
		goto finish;
	}

	Format = pParam->format;
	iWidth = pParam->iWidth;
	iHeight = pParam->iHeight;
	pSysMem = (PVOID) pParam->pData;

	pOsResource = &(pEntry->OsResource);
	IntelGen_OsResetResource(pOsResource);
	HalCm_GetSurfPitchSize(iWidth, iHeight, Format, &align_x, &iSize);

	if (pState->hLibModule && pState->pDrmVMap) {
		bo = pState->pDrmVMap(pOsInterface->pOsContext->bufmgr,
				      "CM Surface2D UP",
				      (void *)(pSysMem),
				      tileformat, align_x, iSize, 0);
	}

	pOsResource->bMapped = FALSE;
	if (bo) {
		pOsResource->Format = Format;
		pOsResource->iWidth = iWidth;
		pOsResource->iHeight = iHeight;
		pOsResource->iPitch = align_x;
		pOsResource->bo = bo;
		pOsResource->TileType = OsToGenTileType(tileformat);
		pOsResource->pData = (PBYTE) bo->virt;

	} else {
		hr = GENOS_STATUS_UNKNOWN;
	}

	pEntry->iWidth = pParam->iWidth;
	pEntry->iHeight = pParam->iHeight;
	pEntry->format = Format;

 finish:
	return hr;
}

GENOS_STATUS HalCm_GetGPUCurrentFrequency(PCM_HAL_STATE pState,
					  UINT * pCurrentFreq)
{
	GENOS_STATUS hr;

	CM_ASSERT(pState);
	*pCurrentFreq = 0;
	hr = GENOS_STATUS_SUCCESS;

	return hr;
}

GENOS_STATUS HalCm_GetGlobalTime(LARGE_INTEGER * pGlobalTime)
{
	GENOS_STATUS hr;

	hr = GENOS_STATUS_SUCCESS;

	return hr;
}

GENOS_STATUS HalCm_ConvertToQPCTime(UINT64 nanoseconds, LARGE_INTEGER * QPCTime)
{
	GENOS_STATUS hr;

	hr = GENOS_STATUS_SUCCESS;
	return hr;
}

GENOS_STATUS HalCm_GetGpuTime(PCM_HAL_STATE pState, PUINT64 pGpuTime)
{
	GENOS_STATUS hr;

	CM_ASSERT(pState);
	hr = GENOS_STATUS_SUCCESS;

	return hr;
}

VOID HalCm_GetLibDrmVMapFnt(PCM_HAL_STATE pCmState)
{
	if (!pCmState->hLibModule) {
		pCmState->hLibModule = LoadLibrary("libdrm_intel.so");
	}

	if (pCmState->hLibModule) {
		pCmState->pDrmVMap =
		    (pDrmVMapFnc) GetProcAddress(pCmState->hLibModule,
						 DRMVMAP_FUNCTION_STR);
	} else {
		pCmState->pDrmVMap = NULL;
	}
	return;
}

VOID HalCm_OsInitInterface(PCM_HAL_STATE pCmState)
{
	CM_ASSERT(pCmState);

	pCmState->pfnGetSurface2DPitchAndSize = HalCm_GetSurface2DPitchAndSize;
	pCmState->pfnAllocateBuffer = HalCm_AllocateBuffer;
	pCmState->pfnAllocateSurface2DUP = HalCm_AllocateSurface2DUP;
	pCmState->pfnGetGPUCurrentFrequency = HalCm_GetGPUCurrentFrequency;
	pCmState->pfnGetGlobalTime = HalCm_GetGlobalTime;
	pCmState->pfnGetGpuTime = HalCm_GetGpuTime;
	pCmState->pfnConvertToQPCTime = HalCm_ConvertToQPCTime;

	HalCm_GetLibDrmVMapFnt(pCmState);
	return;
}
