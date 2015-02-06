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
#include "hal_cm_g8.h"
#include "cm_common.h"
#include "hw_interface_g8.h"

GENOS_STATUS HalCm_SubmitCommands_g8(PCM_HAL_STATE pState,
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
	UINT i;
	UINT uiPatchOffset;
	PCM_HAL_TASK_PARAM pTaskParam = pState->pTaskParam;
	MEDIA_STATE_FLUSH_CMD_G75 CmdMediaStateFlush;

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

	HalCm_HwSendL3CacheConfig_g8(pState, &CmdBuffer);

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

		CM_CHK_GENOSSTATUS(pHwInterface->pfnSendMediaStateFlush
				   (pHwInterface, &CmdBuffer));
	} else if (enableGpGpu) {
		GENHW_LOAD_REGISTER_IMM_PARAM LoadRegImm;
		GENOS_ZeroMemory(&LoadRegImm,
				 sizeof(GENHW_LOAD_REGISTER_IMM_PARAM));

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

		CM_CHK_GENOSSTATUS(pHwInterface->pfnSendMediaStateFlush
				   (pHwInterface, &CmdBuffer));

	} else {
		CM_CHK_GENOSSTATUS(pHwInterface->pfnSendBatchBufferStart
				   (pHwInterface, &CmdBuffer, pBatchBuffer));

		if ((pBatchBuffer->pBBRenderData->BbArgs.BbCmArgs.uiRefCount ==
		     1) || (pState->pTaskParam->reuseBBUpdateMask == 1)) {
			CmdMediaStateFlush =
			    *(pHwInterface->pHwCommands->pMediaStateFlush_g75);
			CM_CHK_GENOSSTATUS(HalCm_AddMediaStateFlushBb_g8
					   (pHwInterface, pBatchBuffer,
					    &CmdMediaStateFlush));

			pHwInterface->pfnAddBatchBufferEndCmdBb(pHwInterface,
								pBatchBuffer);
		} else {
			HalCm_SkipMediaStateFlushBb_g8(pHwInterface,
						       pBatchBuffer);

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

	for (i = 0; i < pState->CmDeviceParam.iMaxBufferTableSize; i++) {
		if (pState->pBufferTable[i].pAddress) {
			CM_HRESULT2GENOSSTATUS_AND_CHECK
			    (pOsInterface->pfnRegisterResource
			     (pOsInterface, &pState->pBufferTable[i].OsResource,
			      TRUE, FALSE));

			uiPatchOffset = CmdBuffer.iOffset + (2 * sizeof(DWORD));

			CM_CHK_GENOSSTATUS
			    (pOsInterface->pfnSetPatchEntry
			     (pOsInterface,
			      pOsInterface->pfnGetResourceAllocationIndex
			      (pOsInterface,
			       &(pState->pBufferTable[i].OsResource)),
			      0, uiPatchOffset));
		}
	}

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

GENOS_STATUS HalCm_HwSetSurfaceMemoryObjectControl_g8(PCM_HAL_STATE pState,
						      WORD wMemObjCtl,
						      PGENHW_SURFACE_STATE_PARAMS
						      pParams)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;

	CM_HAL_MEMORY_OBJECT_CONTROL_G8 cache_type;
	GENOS_ZeroMemory(&cache_type, sizeof(CM_HAL_MEMORY_OBJECT_CONTROL_G8));

	if ((wMemObjCtl & CM_MEMOBJCTL_CACHE_MASK) >> 8 == CM_INVALID_MEMOBJCTL) {
		cache_type.DwordValue = 0x79;
	} else {
		cache_type.Gen8.Age = (wMemObjCtl & 0xF);
		cache_type.Gen8.CacheControl = (wMemObjCtl & 0xF0) >> 4;
		cache_type.Gen8.TargetCache =
		    (wMemObjCtl & CM_MEMOBJCTL_CACHE_MASK) >> 8;
	}

	pParams->MemObjCtl = cache_type.DwordValue;

	return hr;
}

VOID HalCm_HwSendL3CacheConfig_g8(PCM_HAL_STATE pState,
				  PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	GENHW_LOAD_REGISTER_IMM_PARAM LoadRegImm;
	PGENHW_HW_INTERFACE pHwInterface = pState->pHwInterface;

	GENOS_ZeroMemory(&LoadRegImm, sizeof(GENHW_LOAD_REGISTER_IMM_PARAM));

	LoadRegImm.dwRegisterAddress = GENHW_REG_L3_CACHE_CNTLREG_G8;

	if (GFX_IS_PRODUCT(pState->Platform, IGFX_CHERRYVIEW)) {
		LoadRegImm.dwData =
		    pState->bSLMMode ? CM_CONFIG_CNTLREG_VALUE_G8_CHV_SLM :
		    CM_CONFIG_CNTLREG_VALUE_G8_CHV_SLM;
	} else {
		LoadRegImm.dwData =
		    pState->bSLMMode ? CM_CONFIG_CNTLREG_VALUE_G8_BDW_SLM :
		    CM_CONFIG_CNTLREG_VALUE_G8_BDW_NONSLM;
	}

	pHwInterface->pfnSendLoadRegImmCmd(pHwInterface, pCmdBuffer,
					   &LoadRegImm);

	return;
}

INT HalCm_GetCurbeBlockAlignSize_g8()
{
	return GENHW_CURBE_BLOCK_ALIGN_G8;
}

GENOS_STATUS HalCm_AddMediaStateFlushBb_g8(PGENHW_HW_INTERFACE pHwInterface,
					   PGENHW_BATCH_BUFFER pBatchBuffer,
					   PMEDIA_STATE_FLUSH_CMD_G75
					   pMediaStateFlush)
{
	PBYTE pBuffer;
	PMEDIA_STATE_FLUSH_CMD_G75 pCmd;
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;

	pBuffer = pBatchBuffer->pData + pBatchBuffer->iCurrent;
	pCmd = (PMEDIA_STATE_FLUSH_CMD_G75) pBuffer;

	if (pMediaStateFlush != NULL) {
		*pCmd = *pMediaStateFlush;
	} else {
		*pCmd = *(pHwInterface->pHwCommands->pMediaStateFlush_g75);
	}

	pBatchBuffer->iCurrent += sizeof(MEDIA_STATE_FLUSH_CMD_G75);
	return hr;
}

GENOS_STATUS HalCm_SkipMediaStateFlushBb_g8(PGENHW_HW_INTERFACE pHwInterface,
					    PGENHW_BATCH_BUFFER pBatchBuffer)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;

	if (((INT) pBatchBuffer->iSize - pBatchBuffer->iCurrent) <
	    (INT) sizeof(MEDIA_STATE_FLUSH_CMD_G75)) {
		hr = GENOS_STATUS_EXCEED_MAX_BB_SIZE;
	} else {
		pBatchBuffer->iCurrent += sizeof(MEDIA_STATE_FLUSH_CMD_G75);
	}

	return hr;
}

GENOS_STATUS HalCm_GetUserDefinedThreadCountPerThreadGroup_g8(PCM_HAL_STATE
							      pState,
							      UINT *
							      pThreadsPerThreadGroup)
{
	GENOS_STATUS hr = GENOS_STATUS_SUCCESS;
	int threads_per_eu = 0;
	int eu_per_subslice = 0;
	if (pState->Platform.eProductFamily == IGFX_BROADWELL) {
		if (pState->pHwInterface->Platform.GtType == GTTYPE_GT1) {
			threads_per_eu = GENHW_CM_THREADS_PER_EU_BDW_GT1;
			eu_per_subslice = GENHW_CM_EU_PER_SUBSLICE_BDW_GT1;
		} else if (pState->pHwInterface->Platform.GtType == GTTYPE_GT2) {
			threads_per_eu = GENHW_CM_THREADS_PER_EU_BDW_GT2;
			eu_per_subslice = GENHW_CM_EU_PER_SUBSLICE_BDW_GT2;
		} else if (pState->pHwInterface->Platform.GtType == GTTYPE_GT3) {
			threads_per_eu = GENHW_CM_THREADS_PER_EU_BDW_GT3;
			eu_per_subslice = GENHW_CM_EU_PER_SUBSLICE_BDW_GT3;
		} else {
			threads_per_eu = GENHW_CM_THREADS_PER_EU_BDW_GT2;
			eu_per_subslice = GENHW_CM_EU_PER_SUBSLICE_BDW_GT2;
		}
	} else if (pState->Platform.eProductFamily == IGFX_CHERRYVIEW) {
		threads_per_eu = GENHW_CM_THREADS_PER_EU_CHV;
		eu_per_subslice = GENHW_CM_EU_PER_SUBSLICE_CHV;
	} else {
		hr = GENOS_STATUS_PLATFORM_NOT_SUPPORTED;
	}

	*pThreadsPerThreadGroup = threads_per_eu * eu_per_subslice;
	return hr;
}
