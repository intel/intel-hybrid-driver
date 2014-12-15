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
#include "hal_cm.h"
#include "hw_interface_g75.h"
#include "hal_cm_g75.h"

GENOS_STATUS HalCm_SubmitCommands_g75(PCM_HAL_STATE pState,
				      PGENHW_BATCH_BUFFER pBatchBuffer,
				      INT iTaskId,
				      PCM_HAL_KERNEL_PARAM * pKernels,
				      PVOID * ppCmdBuffer)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	PGENHW_HW_INTERFACE pHwInterface = pState->pHwInterface;
	PGENOS_INTERFACE pOsInterface = pHwInterface->pOsInterface;
	PIPELINE_SELECT_CMD_G5 cmd_select =
	    *(pHwInterface->pHwCommands->pPipelineSelectMedia);
	INT iRemaining = 0;
	BOOL enableWalker = pState->WalkerParams.CmWalkerEnable;
	BOOL enableGpGpu = pState->pTaskParam->blGpGpuWalkerEnabled;
	GENOS_COMMAND_BUFFER CmdBuffer;
	DWORD dwSyncTag;
	PINT64 pTaskSyncLocation;
	INT iSyncOffset;
	INT iTmp;
	PCM_HAL_TASK_PARAM pTaskParam = pState->pTaskParam;

	GENOS_ZeroMemory(&CmdBuffer, sizeof(GENOS_COMMAND_BUFFER));

	iSyncOffset = pState->pfnGetTaskSyncLocation(iTaskId);

	pTaskSyncLocation = (PINT64) (pState->TsResource.pData + iSyncOffset);
	*pTaskSyncLocation = CM_INVALID_INDEX;
	*(pTaskSyncLocation + 1) = CM_INVALID_INDEX;

	if (!enableWalker && !enableGpGpu) {
		CM_HRESULT2GENOSSTATUS_AND_CHECK
		    (pOsInterface->pfnRegisterResource
		     (pOsInterface, &pBatchBuffer->OsResource, TRUE, TRUE));
	}
	CM_HRESULT2GENOSSTATUS_AND_CHECK(pOsInterface->pfnRegisterResource
					 (pOsInterface,
					  &pState->TsResource.OsResource, TRUE,
					  TRUE));

	CM_HRESULT2GENOSSTATUS_AND_CHECK(pOsInterface->pfnGetCommandBuffer
					 (pOsInterface, &CmdBuffer));
	iRemaining = CmdBuffer.iRemaining;

	if (pState->bEUSaturationEnabled) {
		CM_CHK_GENOSSTATUS
		    (pState->pfnSendCommandBufferHeaderEUSaturation
		     (pState, &CmdBuffer));
	} else {
		CM_CHK_GENOSSTATUS(pHwInterface->pfnSendCommandBufferHeader
				   (pHwInterface, &CmdBuffer));
	}

	CM_CHK_GENOSSTATUS(pHwInterface->pfnSendPipeControl(pHwInterface,
							    &CmdBuffer,
							    &pState->
							    TsResource.OsResource,
							    TRUE, iSyncOffset,
							    GFX3DCONTROLOP_WRITETIMESTAMP,
							    GFX3DFLUSH_WRITE_CACHE,
							    0));

	dwSyncTag = pHwInterface->pGeneralStateHeap->dwNextTag++;

	CM_CHK_GENOSSTATUS(pHwInterface->pfnSendSyncTag
			   (pHwInterface, &CmdBuffer));

	HalCm_HwSendL3CacheConfig_g75(pState, &CmdBuffer);

	if (enableGpGpu) {
		cmd_select.DW0.PipelineSelect = GFXPIPELINE_GPGPU;
	}
	CM_CHK_GENOSSTATUS(IntelGen_OsAddCommand(&CmdBuffer,
						 &cmd_select,
						 sizeof
						 (PIPELINE_SELECT_CMD_G5)));

	CM_CHK_GENOSSTATUS(pHwInterface->pfnSendStateBaseAddr
			   (pHwInterface, &CmdBuffer));

	CM_CHK_GENOSSTATUS(pHwInterface->pfnSendSurfaces
			   (pHwInterface, &CmdBuffer));

	iTmp = GENHW_USE_MEDIA_THREADS_MAX;
	if (pState->MaxHWThreadValues.registryValue != 0) {
		if (pState->MaxHWThreadValues.registryValue <
		    pHwInterface->pHwCaps->dwMaxThreads) {
			iTmp = pState->MaxHWThreadValues.registryValue;
		}
	} else if (pState->MaxHWThreadValues.APIValue != 0) {
		if (pState->MaxHWThreadValues.APIValue <
		    pHwInterface->pHwCaps->dwMaxThreads) {
			iTmp = pState->MaxHWThreadValues.APIValue;
		}
	}

	pHwInterface->pfnSetVfeStateParams(pHwInterface,
					   0,
					   iTmp,
					   pState->pTaskParam->dwVfeCurbeSize,
					   pState->pTaskParam->dwUrbEntrySize,
					   &pState->ScoreboardParams);

	CM_CHK_GENOSSTATUS(pHwInterface->pfnSendVfeState
			   (pHwInterface, &CmdBuffer, enableGpGpu));

	if (pState->pTaskParam->dwVfeCurbeSize > 0) {
		CM_CHK_GENOSSTATUS(pHwInterface->pfnSendCurbeLoad
				   (pHwInterface, &CmdBuffer));
	}
	CM_CHK_GENOSSTATUS(pHwInterface->pfnSendIDLoad
			   (pHwInterface, &CmdBuffer));

	if (enableWalker) {
		for (UINT i = 0; i < pState->pTaskParam->uiNumKernels; i++) {
			if ((i > 0)
			    &&
			    ((pTaskParam->uiSyncBitmap &
			      ((UINT64) 1 << (i - 1)))
			     || (pKernels[i]->
				 CmKernelThreadSpaceParam.patternType !=
				 CM_DEPENDENCY_NONE))) {
				CM_CHK_GENOSSTATUS
				    (pHwInterface->pfnSendPipeControl
				     (pHwInterface, &CmdBuffer,
				      &pState->TsResource.OsResource, FALSE, 0,
				      GFX3DCONTROLOP_NOWRITE,
				      GFX3DFLUSH_WRITE_CACHE, 0));
			}

			CM_CHK_GENOSSTATUS(pState->pfnSendMediaWalkerState
					   (pState, pKernels[i], &CmdBuffer));
		}
	} else if (enableGpGpu) {
		for (UINT i = 0; i < pState->pTaskParam->uiNumKernels; i++) {
			if ((i > 0)
			    && (pTaskParam->uiSyncBitmap &
				((UINT64) 1 << (i - 1)))) {
				CM_CHK_GENOSSTATUS
				    (pHwInterface->pfnSendPipeControl
				     (pHwInterface, &CmdBuffer,
				      &pState->TsResource.OsResource, FALSE, 0,
				      GFX3DCONTROLOP_NOWRITE,
				      GFX3DFLUSH_WRITE_CACHE, 0));
			}

			CM_CHK_GENOSSTATUS(pState->pfnSendGpGpuWalkerState
					   (pState, pKernels[i], &CmdBuffer));
		}
	} else {
		CM_CHK_GENOSSTATUS(pHwInterface->pfnSendBatchBufferStart
				   (pHwInterface, &CmdBuffer, pBatchBuffer));

		if ((pBatchBuffer->pBBRenderData->BbArgs.BbCmArgs.uiRefCount ==
		     1) || (pState->pTaskParam->reuseBBUpdateMask == 1)) {
			pHwInterface->pfnAddBatchBufferEndCmdBb(pHwInterface,
								pBatchBuffer);
		} else {
			pHwInterface->pfnSkipBatchBufferEndCmdBb(pHwInterface,
								 pBatchBuffer);
		}

		if ((pBatchBuffer->pBBRenderData->BbArgs.BbCmArgs.uiRefCount ==
		     1) || (pState->pTaskParam->reuseBBUpdateMask == 1)) {
			CM_CHK_GENOSSTATUS(pHwInterface->pfnUnlockBB
					   (pHwInterface, pBatchBuffer));
		}
	}

	CM_CHK_GENOSSTATUS(pHwInterface->pfnSendPipeControl(pHwInterface,
							    &CmdBuffer,
							    &pState->
							    TsResource.OsResource,
							    FALSE, 0,
							    GFX3DCONTROLOP_NOWRITE,
							    GFX3DFLUSH_WRITE_CACHE,
							    0));

	iSyncOffset += sizeof(UINT64);
	CM_CHK_GENOSSTATUS(pHwInterface->pfnSendPipeControl(pHwInterface,
							    &CmdBuffer,
							    &pState->
							    TsResource.OsResource,
							    TRUE, iSyncOffset,
							    GFX3DCONTROLOP_WRITETIMESTAMP,
							    GFX3DFLUSH_READ_CACHE,
							    0));

	CM_CHK_GENOSSTATUS(pHwInterface->pfnSendBatchBufferEnd
			   (pHwInterface, &CmdBuffer));

	pOsInterface->pfnReturnCommandBuffer(pOsInterface, &CmdBuffer);

	CM_HRESULT2GENOSSTATUS_AND_CHECK(pOsInterface->pfnSubmitCommandBuffer
					 (pOsInterface, &CmdBuffer,
					  pState->bNullHwRenderCm));

	if (pState->bNullHwRenderCm == FALSE) {
		pHwInterface->pGeneralStateHeap->pCurMediaState->bBusy = TRUE;
		if (!enableWalker && !enableGpGpu) {
			pBatchBuffer->bBusy = TRUE;
			pBatchBuffer->dwSyncTag = dwSyncTag;
		}
	}
	pState->MaxHWThreadValues.APIValue = 0;

	pState->bEUSaturationEnabled = FALSE;
	pState->bEUSaturationNoSSD = FALSE;

	if (ppCmdBuffer) {
		drm_intel_bo_reference(CmdBuffer.OsResource.bo);
		*ppCmdBuffer = CmdBuffer.OsResource.bo;
	}

	hr = GENOS_STATUS_SUCCESS;

 finish:
	if (hr != GENOS_STATUS_SUCCESS) {
		if (CmdBuffer.iRemaining < 0) {
			GENHW_PUBLIC_ASSERTMESSAGE
			    ("Command Buffer overflow by %d bytes.",
			     -CmdBuffer.iRemaining);
		}
		iTmp = iRemaining - CmdBuffer.iRemaining;
		CmdBuffer.iRemaining = iRemaining;
		CmdBuffer.iOffset -= iTmp;
		CmdBuffer.pCmdPtr =
		    CmdBuffer.pCmdBase + CmdBuffer.iOffset / sizeof(DWORD);

		pOsInterface->pfnReturnCommandBuffer(pOsInterface, &CmdBuffer);
	}

	return hr;
}

GENOS_STATUS HalCm_HwSetSurfaceMemoryObjectControl_g75(PCM_HAL_STATE pState,
						       WORD wMemObjCtl,
						       PGENHW_SURFACE_STATE_PARAMS
						       pParams)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;

	CM_HAL_MEMORY_OBJECT_CONTROL_G75 cache_type =
	    (CM_HAL_MEMORY_OBJECT_CONTROL_G75) ((wMemObjCtl &
						 CM_MEMOBJCTL_CACHE_MASK) >> 8);

	if (cache_type == CM_INVALID_MEMOBJCTL) {
		cache_type = CM_MEMORY_OBJECT_CONTROL_L3_LLC_ELLC_WB_CACHED;
	}

	if (cache_type < CM_MEMORY_OBJECT_CONTROL_USE_PTE
	    || cache_type > CM_MEMORY_OBJECT_CONTROL_L3_ELLC_WB_CACHED) {
		hr = GENOS_STATUS_UNKNOWN;
		return hr;
	}

	pParams->MemObjCtl = cache_type;

	return hr;
}

VOID HalCm_HwSendL3CacheConfig_g75(PCM_HAL_STATE pState,
				   PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	GENHW_LOAD_REGISTER_IMM_PARAM LoadRegImm;
	PGENHW_HW_INTERFACE pHwInterface = pState->pHwInterface;

	GENOS_ZeroMemory(&LoadRegImm, sizeof(GENHW_LOAD_REGISTER_IMM_PARAM));
	LoadRegImm.dwRegisterAddress = GENHW_REG_L3_CACHE_CNTLREG2_G75;
	LoadRegImm.dwData = pState->bSLMMode ? CM_CONFIG_CNTLREG2_VALUE_G75_SLM
	    : CM_CONFIG_CNTLREG2_VALUE_G75_NONSLM;
	pHwInterface->pfnSendLoadRegImmCmd
	    (pHwInterface, pCmdBuffer, &LoadRegImm);

	GENOS_ZeroMemory(&LoadRegImm, sizeof(GENHW_LOAD_REGISTER_IMM_PARAM));
	LoadRegImm.dwRegisterAddress = GENHW_REG_L3_CACHE_CNTLREG3_G75;
	LoadRegImm.dwData = pState->bSLMMode ? CM_CONFIG_CNTLREG3_VALUE_G75_SLM
	    : CM_CONFIG_CNTLREG3_VALUE_G75_NONSLM;

	pHwInterface->pfnSendLoadRegImmCmd
	    (pHwInterface, pCmdBuffer, &LoadRegImm);

	return;
}

UINT HalCm_GetPerThreadScratchSpaceSize_g75()
{
	return CM_MAX_SPILL_SIZE_PER_THREAD_HSW_BDW;
}

GENOS_STATUS HalCm_AddMediaStateFlushBb_g75(PGENHW_HW_INTERFACE pHwInterface,
					    PGENHW_BATCH_BUFFER pBatchBuffer)
{
	return GENOS_STATUS_SUCCESS;
}

INT HalCm_GetTaskSyncLocation_g75(INT iTaskId)
{
	return (iTaskId * CM_SYNC_QWORD_PER_TASK * sizeof(UINT64));
}

INT HalCm_GetCurbeBlockAlignSize_g75()
{
	return GENHW_CURBE_BLOCK_ALIGN_G7;
}

GENOS_STATUS HalCm_GetUserDefinedThreadCountPerThreadGroup_g75(PCM_HAL_STATE
							       pState,
							       UINT *
							       pThreadsPerThreadGroup)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	int threads_per_eu = 0;
	int eu_per_subslice = 0;
	if (pState->pHwInterface->Platform.GtType == GTTYPE_GT1) {
		threads_per_eu = GENHW_CM_THREADS_PER_EU_HSW_GT1;
		eu_per_subslice = GENHW_CM_EU_PER_SUBSLICE_HSW_GT1;
	} else if (pState->pHwInterface->Platform.GtType == GTTYPE_GT2) {
		threads_per_eu = GENHW_CM_THREADS_PER_EU_HSW_GT2;
		eu_per_subslice = GENHW_CM_EU_PER_SUBSLICE_HSW_GT2;
	} else if (pState->pHwInterface->Platform.GtType == GTTYPE_GT3) {
		threads_per_eu = GENHW_CM_THREADS_PER_EU_HSW_GT3;
		eu_per_subslice = GENHW_CM_EU_PER_SUBSLICE_HSW_GT3;
	} else {
		threads_per_eu = GENHW_CM_THREADS_PER_EU_HSW_GT2;
		eu_per_subslice = GENHW_CM_EU_PER_SUBSLICE_HSW_GT2;
	}

	*pThreadsPerThreadGroup = threads_per_eu * eu_per_subslice;
	return hr;
}
