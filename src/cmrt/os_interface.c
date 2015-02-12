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
#include "os_util_debug.h"
#include <unistd.h>
#include "hw_info.h"
#include "fourcc.h"
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#define ALIGN(x, align)  (((x) + (align) - 1) & (~((align) -1)))

#define Y_TILE_WIDTH  128
#define Y_TILE_HEIGHT 32
#define X_TILE_WIDTH  512
#define X_TILE_HEIGHT 8

#define MI_BATCHBUFFER_END 0x05000000

static int OpenDevice();
static int updatePlatformInfo(PLATFORM * pPlatform);

GENOS_STATUS IntelGen_OsAddCommand(PGENOS_COMMAND_BUFFER pCmdBuffer,
				   PCVOID pCmd, DWORD dwCmdSize)
{
	DWORD dwCmdSizeDwAligned;

	GENOS_OS_ASSERT(pCmdBuffer);
	GENOS_OS_ASSERT(pCmd);

	dwCmdSizeDwAligned = GENOS_ALIGN_CEIL(dwCmdSize, sizeof(DWORD));

	pCmdBuffer->iOffset += dwCmdSizeDwAligned;
	pCmdBuffer->iRemaining -= dwCmdSizeDwAligned;

	if (pCmdBuffer->iRemaining < 0) {
		GENOS_OS_ASSERTMESSAGE("Unable to add command (no space).");
		return GENOS_STATUS_UNKNOWN;
	}

	GENOS_SecureMemcpy(pCmdBuffer->pCmdPtr, dwCmdSize, pCmd, dwCmdSize);
	pCmdBuffer->pCmdPtr += (dwCmdSizeDwAligned / sizeof(DWORD));

	return GENOS_STATUS_SUCCESS;
}

PVOID IntelGen_OsGetCmdBufferSpace(PGENOS_COMMAND_BUFFER pCmdBuffer,
				   DWORD dwCmdSize)
{
	GENOS_OS_ASSERT(pCmdBuffer);

	if ((pCmdBuffer->iRemaining - (int)dwCmdSize) < 0) {
		GENOS_OS_ASSERTMESSAGE("Unable to add command (no space).");
		return NULL;
	}

	return pCmdBuffer->pCmdPtr;
}

VOID IntelGen_OsAdjustCmdBufferFreeSpace(PGENOS_COMMAND_BUFFER pCmdBuffer,
					 DWORD dwCmdSize)
{
	GENOS_OS_ASSERT(pCmdBuffer);
	GENOS_OS_ASSERT((dwCmdSize % sizeof(DWORD)) == 0);

	pCmdBuffer->iOffset += dwCmdSize;
	pCmdBuffer->iRemaining -= dwCmdSize;
	pCmdBuffer->pCmdPtr += (dwCmdSize / sizeof(DWORD));
}

VOID IntelGen_OsClear_OsGpuContext(GENOS_CONTEXT * pContext)
{
	INT iLoop;

	GENOS_OS_FUNCTION_ENTER;

	for (iLoop = 0; iLoop < GENOS_GPU_CONTEXT_MAX; iLoop++) {
		if (pContext->OsGpuContext[iLoop].pCB != NULL) {
			GENOS_FreeMemory(pContext->OsGpuContext[iLoop].pCB);
			pContext->OsGpuContext[iLoop].pCB = NULL;
		}

		if (pContext->OsGpuContext[iLoop].pAllocationList != NULL) {
			GENOS_FreeMemory(pContext->
					 OsGpuContext[iLoop].pAllocationList);
			pContext->OsGpuContext[iLoop].pAllocationList = NULL;
		}

		if (pContext->OsGpuContext[iLoop].pPatchLocationList) {
			GENOS_FreeMemory(pContext->OsGpuContext
					 [iLoop].pPatchLocationList);
			pContext->OsGpuContext[iLoop].pPatchLocationList = NULL;
		}

		if (pContext->OsGpuContext[iLoop].pResources != NULL) {
			GENOS_FreeMemory(pContext->
					 OsGpuContext[iLoop].pResources);
			pContext->OsGpuContext[iLoop].pResources = NULL;
		}

		if (pContext->OsGpuContext[iLoop].pbWriteMode != NULL) {
			GENOS_FreeMemory(pContext->
					 OsGpuContext[iLoop].pbWriteMode);
			pContext->OsGpuContext[iLoop].pbWriteMode = NULL;
		}

		pContext->OsGpuContext[iLoop].uiMaxNumAllocations = 0;
		pContext->OsGpuContext[iLoop].uiMaxPatchLocationsize = 0;
	}
}

BOOL Ctx_GetCommandBuffer(PGENOS_CONTEXT pOsContext,
			  PGENOS_COMMAND_BUFFER pCmdBuffer, INT iSize)
{
	BOOL bResult = FALSE;
	drm_intel_bo *cmd_bo = NULL;

	if (pOsContext == NULL || pCmdBuffer == NULL) {
		bResult = FALSE;
		GENOS_OS_ASSERTMESSAGE
		    ("Ctx_GetCommandBuffer:pOsContext == NULL || pCmdBuffer == NULL");
		goto finish;
	}
	cmd_bo =
	    drm_intel_bo_alloc(pOsContext->bufmgr, "Intel GenOS CmdBuf", iSize,
			       4096);
	if (cmd_bo == NULL) {
		GENOS_OS_ASSERTMESSAGE("Allocation of command buffer failed.");
		bResult = FALSE;
		goto finish;
	}

	if (drm_intel_bo_map(cmd_bo, 1) != 0) {
		GENOS_OS_ASSERTMESSAGE("Mapping of command buffer failed.");
		bResult = FALSE;
		goto finish;
	}

	IntelGen_OsResetResource(&pCmdBuffer->OsResource);

	pCmdBuffer->OsResource.Format = Format_Buffer;
	pCmdBuffer->OsResource.iWidth = cmd_bo->size;
	pCmdBuffer->OsResource.iHeight = 1;
	pCmdBuffer->OsResource.iPitch = cmd_bo->size;
	pCmdBuffer->OsResource.iCount = 1;
	pCmdBuffer->OsResource.pData = (PBYTE) cmd_bo->virt;
	pCmdBuffer->OsResource.TileType = GENOS_TILE_LINEAR;
	pCmdBuffer->OsResource.bo = cmd_bo;
	pCmdBuffer->OsResource.bMapped = TRUE;

	pCmdBuffer->pCmdBase = (PDWORD) cmd_bo->virt;
	pCmdBuffer->pCmdPtr = (PDWORD) cmd_bo->virt;
	pCmdBuffer->iOffset = 0;
	pCmdBuffer->iRemaining = cmd_bo->size;

	GENOS_ZeroMemory(pCmdBuffer->pCmdBase, cmd_bo->size);
	bResult = TRUE;

 finish:
	if ((FALSE == bResult) && (NULL != cmd_bo)) {
		drm_intel_bo_unreference(cmd_bo);
	}
	return bResult;
}

VOID Ctx_ReturnCommandBuffer(PGENOS_CONTEXT pOsContext,
			     GENOS_GPU_CONTEXT GpuContext,
			     PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	GENOS_OS_GPU_CONTEXT *pOsGpuContext;

	if (pOsContext == NULL || pCmdBuffer == NULL ||
	    IntelGen_OsResourceIsNull(&(pCmdBuffer->OsResource))) {
		goto finish;
	}
	pOsGpuContext = &pOsContext->OsGpuContext[GpuContext];

	pOsGpuContext->pCB->iOffset = pCmdBuffer->iOffset;
	pOsGpuContext->pCB->iRemaining = pCmdBuffer->iRemaining;
	pOsGpuContext->pCB->pCmdPtr = pCmdBuffer->pCmdPtr;

 finish:
	return;
}

BOOL Ctx_FlushCommandBuffer(PGENOS_CONTEXT pOsContext,
			    GENOS_GPU_CONTEXT GpuContext)
{
	PCOMMAND_BUFFER pCurrCB;
	BOOL bResult = FALSE;
	PGENOS_OS_GPU_CONTEXT pOsGpuContext;

	if (pOsContext == NULL) {
		goto finish;
	}

	pOsGpuContext = &pOsContext->OsGpuContext[GpuContext];
	pOsContext->pfnRefresh(pOsContext);

	pOsGpuContext->uiCurrentNumPatchLocations = 0;

	pCurrCB = pOsGpuContext->pCurrentCB;
	if (pCurrCB->bActive) {
		goto finish;
	}

	pCurrCB->bActive = TRUE;
	bResult = TRUE;

 finish:
	return bResult;
}

VOID Ctx_InitCmdBufferPool(PGENOS_CONTEXT pOsContext)
{
	GENOS_OS_FUNCTION_ENTER;

	GENOS_ZeroMemory(&pOsContext->CmdBufferPool,
			 sizeof(CMD_BUFFER_BO_POOL));
	pOsContext->CmdBufferPool.iFetch = 0;
}

HRESULT Ctx_WaitAndReleaseCmdBuffer(PGENOS_CONTEXT pOsContext, INT index)
{
	drm_intel_bo *cmd_bo;
	HRESULT hr;

	GENOS_OS_FUNCTION_ENTER;

	hr = S_OK;

	if (index < 0 || index >= MAX_CMD_BUF_NUM) {
		hr = E_FAIL;
		goto finish;
	}

	cmd_bo = pOsContext->CmdBufferPool.pCmd_bo[index];
	if (cmd_bo != NULL) {
		drm_intel_bo_wait_rendering(cmd_bo);
		drm_intel_bo_unreference(cmd_bo);
		pOsContext->CmdBufferPool.pCmd_bo[index] = NULL;
	}

 finish:
	return hr;
}

HRESULT Ctx_ReleaseCmdBufferPool(PGENOS_CONTEXT pOsContext)
{
	INT i;
	HRESULT hr;

	GENOS_OS_FUNCTION_ENTER;

	hr = S_OK;

	for (i = 0; i < MAX_CMD_BUF_NUM; i++) {
		GENOS_OS_CHK_HR(Ctx_WaitAndReleaseCmdBuffer(pOsContext, i));
	}

 finish:
	return hr;
}

HRESULT Ctx_WaitForAvailableCmdBo(PGENOS_CONTEXT pOsContext)
{
	INT index;
	HRESULT hr;

	GENOS_OS_FUNCTION_ENTER;

	hr = S_OK;

	index = pOsContext->CmdBufferPool.iFetch;
	GENOS_OS_CHK_HR(Ctx_WaitAndReleaseCmdBuffer(pOsContext, index));

 finish:
	return hr;
}

HRESULT Ctx_InsertCmdBufferToPool(PGENOS_CONTEXT pOsContext, drm_intel_bo * bo)
{
	INT index;
	HRESULT hr;

	GENOS_OS_FUNCTION_ENTER;

	hr = S_OK;

	GENOS_OS_CHK_HR(Ctx_WaitForAvailableCmdBo(pOsContext));

	index = pOsContext->CmdBufferPool.iFetch;

	pOsContext->CmdBufferPool.pCmd_bo[index] = bo;

	pOsContext->CmdBufferPool.iFetch++;
	if (pOsContext->CmdBufferPool.iFetch >= MAX_CMD_BUF_NUM) {
		pOsContext->CmdBufferPool.iFetch = 0;
	}

 finish:
	return hr;
}

BOOL Ctx_Refresh(GENOS_CONTEXT * pOsContext)
{
	return TRUE;
}

VOID Ctx_Destroy(PGENOS_CONTEXT pOsContext)
{
	PCOMMAND_BUFFER pCurrCB, pNextCB;
	int i = 0;

	Ctx_ReleaseCmdBufferPool(pOsContext);

	for (i = 0; i < GENOS_GPU_CONTEXT_MAX; i++) {
		GENOS_FreeMemAndSetNull(pOsContext->OsGpuContext[i].pCB);

		pCurrCB = pOsContext->OsGpuContext[i].pStartCB;
		for (; (pCurrCB); pCurrCB = pNextCB) {
			pNextCB = pCurrCB->pNext;
			GENOS_FreeMemAndSetNull(pCurrCB);
		}
	}

	GENOS_FreeMemAndSetNull(pOsContext);
}

HRESULT Ctx_InitContext(GENOS_OS_CONTEXT * pContext,
			PGENOS_CONTEXT pOsDriverContext)
{
	INT iDeviceId;
	drm_i915_getparam_t gp;
	HRESULT hr;
	INT i;

	GENOS_OS_FUNCTION_ENTER;

	hr = S_OK;

	if (pContext == NULL || pOsDriverContext == NULL) {
		GENOS_OS_ASSERT(FALSE);
		return E_FAIL;
	}

	if ((pOsDriverContext->bufmgr != NULL) &&
	    (pOsDriverContext->wDeviceID != 0)) {
		pContext->bufmgr = pOsDriverContext->bufmgr;

		iDeviceId = pOsDriverContext->wDeviceID;
		pContext->wDeviceID = iDeviceId;
		pContext->platform.pchDeviceID = iDeviceId;

		pContext->wRevision = pOsDriverContext->wRevision;
		pContext->platform.usRevId = pOsDriverContext->wRevision;
	} else {
		pContext->fd = OpenDevice();
		if (pContext->fd < 0) {
			GENOS_OS_ASSERT(FALSE);
			return E_FAIL;
		}

		pContext->bufmgr = drm_intel_bufmgr_gem_init(pContext->fd,
							     CMD_BUFFER_SIZE_IN_BYTES);
		if (pContext->bufmgr == NULL) {
			GENOS_OS_ASSERT(FALSE);
			return E_FAIL;
		}

		memset(&gp, 0, sizeof(gp));
		gp.param = I915_PARAM_CHIPSET_ID;
		gp.value = &iDeviceId;
		hr = drmIoctl(pContext->fd, DRM_IOCTL_I915_GETPARAM, &gp);
		if ((hr != 0) || (iDeviceId == 0)) {
			GENOS_OS_ASSERT(FALSE);
			return E_FAIL;

		}
		pContext->wDeviceID = iDeviceId;
		pContext->platform.pchDeviceID = iDeviceId;

		pContext->platform.usRevId = 0;
	}

	if (FALSE == IS_GEN7_5_PLUS(iDeviceId)) {
		GENOS_OS_ASSERTMESSAGE
		    ("Platform *NOT* Supported by Current Linux Driver: Only supports: SandyBridge, IvyBridge, HasWell A0/A1/B0 Stepping");
		return E_FAIL;
	}

	updatePlatformInfo(&pContext->platform);

	Ctx_InitCmdBufferPool(pContext);

	for (i = 0; i < GENOS_GPU_CONTEXT_MAX; i++) {
		pContext->OsGpuContext[i].pStartCB = NULL;
		pContext->OsGpuContext[i].pCurrentCB = NULL;
		pContext->OsGpuContext[i].bCBFlushed = TRUE;
		pContext->OsGpuContext[i].uiCommandBufferSize =
		    COMMAND_BUFFER_SIZE;
		pContext->OsGpuContext[i].pCB = (PGENOS_COMMAND_BUFFER)
		    GENOS_AllocAndZeroMemory(sizeof(GENOS_COMMAND_BUFFER));

		if (NULL == pContext->OsGpuContext[i].pCB) {
			GENOS_OS_ASSERTMESSAGE("No More Avaliable Memory");
			hr = E_OUTOFMEMORY;
			goto finish;
		}
		pContext->OsGpuContext[i].pAllocationList = (ALLOCATION_LIST *)
		    GENOS_AllocAndZeroMemory(sizeof(ALLOCATION_LIST) *
					     ALLOCATIONLIST_SIZE);
		if (NULL == pContext->OsGpuContext[i].pAllocationList) {
			GENOS_OS_ASSERTMESSAGE
			    ("pContext->OsGpuContext[%d].pAllocationList malloc failed.",
			     i);
			hr = E_OUTOFMEMORY;
			goto finish;
		}
		pContext->OsGpuContext[i].uiMaxNumAllocations =
		    ALLOCATIONLIST_SIZE;

		pContext->OsGpuContext[i].pPatchLocationList =
		    (PATCHLOCATIONLIST *)
		    GENOS_AllocAndZeroMemory(sizeof(PATCHLOCATIONLIST) *
					     PATCHLOCATIONLIST_SIZE);
		if (NULL == pContext->OsGpuContext[i].pPatchLocationList) {
			GENOS_OS_ASSERTMESSAGE
			    ("pContext->OsGpuContext[%d].pPatchLocationList malloc failed.",
			     i);
			hr = E_OUTOFMEMORY;
			goto finish;
		}
		pContext->OsGpuContext[i].uiMaxPatchLocationsize =
		    PATCHLOCATIONLIST_SIZE;

		pContext->OsGpuContext[i].pResources = (PGENOS_RESOURCE)
		    GENOS_AllocAndZeroMemory(sizeof(GENOS_RESOURCE)
					     * ALLOCATIONLIST_SIZE);
		if (NULL == pContext->OsGpuContext[i].pResources) {
			GENOS_OS_ASSERTMESSAGE
			    ("pContext->OsGpuContext[%d].pResources malloc failed.",
			     i);
			hr = E_OUTOFMEMORY;
			goto finish;
		}

		pContext->OsGpuContext[i].pbWriteMode =
		    (BOOL *) GENOS_AllocAndZeroMemory(sizeof(BOOL) *
						      ALLOCATIONLIST_SIZE);
		if (NULL == pContext->OsGpuContext[i].pbWriteMode) {
			GENOS_OS_ASSERTMESSAGE
			    ("pContext->OsGpuContext[%d].pbWriteMode malloc failed.",
			     i);
			hr = E_OUTOFMEMORY;
			goto finish;
		}
	}

	pContext->pfnDestroy = Ctx_Destroy;
	pContext->pfnGetCommandBuffer = Ctx_GetCommandBuffer;
	pContext->pfnReturnCommandBuffer = Ctx_ReturnCommandBuffer;
	pContext->pfnFlushCommandBuffer = Ctx_FlushCommandBuffer;
	pContext->pfnInsertCmdBufferToPool = Ctx_InsertCmdBufferToPool;
	pContext->pfnRefresh = Ctx_Refresh;

 finish:
	if (hr != S_OK)
		IntelGen_OsClear_OsGpuContext(pContext);

	return hr;
}

VOID IntelGen_OsGetPlatform(PGENOS_INTERFACE pOsInterface, PLATFORM * pPlatform)
{
	if (pOsInterface && pPlatform && pOsInterface->pOsContext) {
		*pPlatform = pOsInterface->pOsContext->platform;
	}
}

VOID IntelGen_OsDestroy(PGENOS_INTERFACE pOsInterface,
			BOOL bDestroyVscVppDeviceTag)
{
	if (pOsInterface &&
	    pOsInterface->pOsContext &&
	    pOsInterface->pOsContext->bFreeContext) {
		IntelGen_OsClear_OsGpuContext(pOsInterface->pOsContext);
		pOsInterface->pOsContext->pfnDestroy(pOsInterface->pOsContext);
		pOsInterface->pOsContext = NULL;
	}
}

VOID IntelGen_OsResetOsStates(PGENOS_INTERFACE pOsInterface)
{
	PGENOS_OS_CONTEXT pOsContext;
	PGENOS_OS_GPU_CONTEXT pOsGpuContext;

	if (pOsInterface == NULL || pOsInterface->pOsContext == NULL) {
		return;
	}

	pOsContext = pOsInterface->pOsContext;
	pOsGpuContext =
	    &pOsContext->OsGpuContext[pOsInterface->CurrentGpuContextOrdinal];
	pOsGpuContext->uiNumAllocations = 0;
	GENOS_ZeroMemory(pOsGpuContext->pAllocationList,
			 sizeof(ALLOCATION_LIST) *
			 pOsGpuContext->uiMaxNumAllocations);
	pOsGpuContext->uiCurrentNumPatchLocations = 0;
	GENOS_ZeroMemory(pOsGpuContext->pPatchLocationList,
			 sizeof(PATCHLOCATIONLIST) *
			 pOsGpuContext->uiMaxPatchLocationsize);
	pOsGpuContext->uiResCount = 0;

	if ((pOsGpuContext->bCBFlushed == TRUE)
	    && pOsGpuContext->pCB->OsResource.bo) {
		pOsGpuContext->pCB->OsResource.bo = NULL;
	}
}

HRESULT IntelGen_OsAllocateResource(PGENOS_INTERFACE pOsInterface,
				    PGENOS_ALLOC_GFXRES_PARAMS pParams,
				    PGENOS_RESOURCE pOsResource)
{
	CONST CHAR *bufname;
	INT iSize;
	INT iHeight;
	INT iWidth;
	INT iPitch;
	unsigned long ulPitch;
	HRESULT hr;
	drm_intel_bo *bo;
	GENOS_TILE_TYPE tileformat;
	UINT tileformat_linux;

	INT width_align;
	INT height_align;

	bufname = pParams->pBufName;
	tileformat_linux = I915_TILING_NONE;
	hr = S_OK;

	tileformat = pParams->TileType;

	switch (tileformat) {
	case GENOS_TILE_LINEAR:
		width_align = 64;
		height_align = 32;
		break;
	case GENOS_TILE_Y:
		width_align = 128;
		height_align = 32;
		break;
	default:
		width_align = 16;
		height_align = 16;
	}

	iWidth = ALIGN(pParams->dwWidth, width_align);
	iHeight = ALIGN(pParams->dwHeight, height_align);

	if (NULL == pOsResource) {
		GENOS_OS_ASSERTMESSAGE("input parameter pOSResource is NULL.");
		return E_FAIL;
	}

	switch (pParams->Format) {
	case Format_Buffer:
	case Format_RAW:
		iPitch = iWidth;
		iSize = iPitch;
		break;
	case Format_X8R8G8B8:
	case Format_A8R8G8B8:
	case Format_R32F:
	case Format_R32U:
		iPitch = 4 * iWidth;
		iSize = iPitch * iHeight;
		break;
	case Format_R5G6B5:
	case Format_V8U8:
	case Format_YUY2:
	case Format_UYVY:
	case Format_AYUV:
	case Format_R16U:
		iPitch = 2 * iWidth;
		iSize = iPitch * iHeight;
		break;
	case Format_AI44:
	case Format_IA44:
	case Format_L8:
	case Format_P8:
	case Format_A8:
	case Format_STMM:
	case Format_R8U:
		iPitch = iWidth;
		iSize = iPitch * iHeight;
		break;
	case Format_NV12:
		if (iHeight & 0x1) {
			hr = E_FAIL;
			goto finish;
		}
		iPitch = iWidth;
		iSize = iPitch * iHeight * 3 / 2;
		break;
	case Format_411P:
	case Format_422H:
	case Format_444P:
		iPitch = iWidth;
		iSize = iPitch * (3 * iHeight);
		break;
	case Format_422V:
	case Format_IMC3:
		iPitch = iWidth;
		iSize = iPitch * (2 * iHeight);
		break;
	case Format_YV12:
		iPitch = iWidth;
		iSize = iPitch * (3 * iHeight / 2);
		break;
	case Format_NV21:
	case Format_Buffer_2D:
	default:
		GENOS_OS_ASSERTMESSAGE("Unsupported format");
		hr = E_FAIL;
		goto finish;
	}

	switch (tileformat) {
	case GENOS_TILE_Y:
		tileformat_linux = I915_TILING_Y;
		break;
	case GENOS_TILE_X:
		tileformat_linux = I915_TILING_X;
		break;
	default:
		tileformat_linux = I915_TILING_NONE;
	}

	iSize = ALIGN(iSize, 4096);
	if (tileformat_linux == I915_TILING_NONE) {
		bo = drm_intel_bo_alloc(pOsInterface->pOsContext->bufmgr,
					bufname, iSize, 4096);
	} else {
		bo = drm_intel_bo_alloc_tiled(pOsInterface->pOsContext->bufmgr,
					      bufname, iPitch, iSize / iPitch,
					      1, &tileformat_linux, &ulPitch,
					      0);
		iPitch = (INT) ulPitch;
	}

	pOsResource->bMapped = FALSE;
	if (bo) {
		pOsResource->Format = pParams->Format;
		pOsResource->iWidth = pParams->dwWidth;
		pOsResource->iHeight = iHeight;
		pOsResource->iPitch = iPitch;
		pOsResource->iCount = 0;
		pOsResource->bufname = bufname;
		pOsResource->bo = bo;
		pOsResource->TileType = tileformat;
		pOsResource->pData = (PBYTE) bo->virt;
		GENOS_OS_VERBOSEMESSAGE("Alloc %7d bytes (%d x %d resource).",
					iSize, pParams->dwWidth, iHeight);
	} else {
		GENOS_OS_ASSERTMESSAGE
		    ("Fail to Alloc %7d bytes (%d x %d resource).", iSize,
		     pParams->dwWidth, pParams->dwHeight);
		hr = E_FAIL;
	}

 finish:
	return hr;
}

VOID IntelGen_OsFreeResource(PGENOS_INTERFACE pOsInterface,
			     PGENOS_RESOURCE pOsResource)
{
	if (pOsResource && pOsResource->bo) {
		drm_intel_bo_unreference((drm_intel_bo *) (pOsResource->bo));
		pOsResource->bo = NULL;
	}

}

PVOID IntelGen_OsLockResource(PGENOS_INTERFACE pOsInterface,
			      PGENOS_RESOURCE pOsResource,
			      PGENOS_LOCK_PARAMS pLockFlags)
{
	PVOID pData;

	pData = NULL;

	GENOS_OS_ASSERT(pOsInterface);
	GENOS_OS_ASSERT(pOsInterface->pOsContext);
	GENOS_OS_ASSERT(pOsResource);

	if (pOsResource && pOsResource->bo) {
		drm_intel_bo *bo = pOsResource->bo;

		if (FALSE == pOsResource->bMapped) {
			if (GFX_IS_PRODUCT
			    (pOsInterface->pOsContext->platform,
			     IGFX_CHERRYVIEW)) {
				//uncache mapping for the BOs mapped once and then shared in multi-task submissions
				//   it is to W/A GPU hang on CHV when i915.ppgtt = 1 in KMD.
				drm_intel_gem_bo_map_gtt(bo);
			} else {
				if (pOsResource->TileType == GENOS_TILE_LINEAR) {
					drm_intel_bo_map(bo,
							 (OSKM_LOCKFLAG_WRITEONLY
							  &
							  pLockFlags->WriteOnly));
				} else {
					drm_intel_gem_bo_map_gtt(bo);
				}
			}

			pOsResource->pData = (PBYTE) bo->virt;
			pOsResource->bMapped = TRUE;
		}
		pData = pOsResource->pData;
	}

	GENOS_OS_ASSERT(pData);
	return pData;
}

HRESULT IntelGen_OsUnlockResource(PGENOS_INTERFACE pOsInterface,
				  PGENOS_RESOURCE pOsResource)
{
	HRESULT hr;

	hr = S_OK;

	GENOS_OS_CHK_NULL_WITH_HR(pOsInterface);
	GENOS_OS_CHK_NULL_WITH_HR(pOsInterface->pOsContext);
	GENOS_OS_CHK_NULL_WITH_HR(pOsResource);

	if (pOsResource->bo) {
		if (TRUE == pOsResource->bMapped) {
			if (GFX_IS_PRODUCT
			    (pOsInterface->pOsContext->platform,
			     IGFX_CHERRYVIEW)) {
				// to be paired with IntelGen_OsLockResource
				drm_intel_gem_bo_unmap_gtt(pOsResource->bo);
			} else {
				if (pOsResource->TileType == GENOS_TILE_LINEAR) {
					drm_intel_bo_unmap(pOsResource->bo);
				} else {
					drm_intel_gem_bo_unmap_gtt
					    (pOsResource->bo);
				}
			}

			pOsResource->bo->virt = NULL;
			pOsResource->bMapped = FALSE;
		}
		pOsResource->pData = NULL;
	}
 finish:
	return hr;
}

HRESULT IntelGen_OsSetPatchEntry(PGENOS_INTERFACE pOsInterface,
				 const UINT iAllocationIndex,
				 const UINT ResourceOffset,
				 const UINT PatchOffset)
{
	PGENOS_OS_CONTEXT pOsContext;
	GENOS_OS_GPU_CONTEXT *pOsGpuContext;
	PPATCHLOCATIONLIST pPatchList;
	HRESULT hr = S_OK;

	pOsContext = pOsInterface->pOsContext;
	pOsGpuContext =
	    &pOsContext->OsGpuContext[pOsInterface->CurrentGpuContextOrdinal];
	pPatchList = pOsGpuContext->pPatchLocationList;

	pPatchList[pOsGpuContext->uiCurrentNumPatchLocations].AllocationIndex =
	    iAllocationIndex;
	pPatchList[pOsGpuContext->uiCurrentNumPatchLocations].AllocationOffset =
	    ResourceOffset;
	pPatchList[pOsGpuContext->uiCurrentNumPatchLocations].PatchOffset =
	    PatchOffset;

	pOsGpuContext->uiCurrentNumPatchLocations++;

	return hr;
}

HRESULT IntelGen_OsRegisterResource(PGENOS_INTERFACE pOsInterface,
				    PGENOS_RESOURCE pOsResource,
				    BOOL bWrite, BOOL bWritebSetResourceSyncTag)
{
	PGENOS_OS_CONTEXT pOsContext;
	PGENOS_RESOURCE pResources;
	UINT uiAllocation;
	HRESULT hResult = S_OK;
	GENOS_OS_GPU_CONTEXT *pOsGpuContext;

	GENOS_OS_ASSERT(pOsInterface);
	GENOS_OS_ASSERT(pOsInterface->pOsContext);

	pOsContext = pOsInterface->pOsContext;
	pOsGpuContext =
	    &pOsContext->OsGpuContext[pOsInterface->CurrentGpuContextOrdinal];

	pResources = pOsGpuContext->pResources;
	if (NULL == pResources) {
		GENOS_OS_ASSERTMESSAGE("pResouce is NULL.");
		return S_OK;
	}
	for (uiAllocation = 0;
	     uiAllocation < pOsGpuContext->uiResCount;
	     uiAllocation++, pResources++) {
		if (pOsResource->bo == pResources->bo)
			break;
	}
	if (uiAllocation < pOsGpuContext->uiMaxNumAllocations) {
		if (uiAllocation == pOsGpuContext->uiResCount) {
			pOsGpuContext->uiResCount++;
		}
		pOsResource->
		    iAllocationIndex[pOsInterface->CurrentGpuContextOrdinal] =
		    uiAllocation;
		pOsGpuContext->pResources[uiAllocation] = *pOsResource;
		pOsGpuContext->pbWriteMode[uiAllocation] |= bWrite;
		pOsGpuContext->pAllocationList[uiAllocation].hAllocation =
		    &pOsGpuContext->pResources[uiAllocation];
		pOsGpuContext->pAllocationList[uiAllocation].WriteOperation =
		    bWrite;
		pOsGpuContext->uiNumAllocations = pOsGpuContext->uiResCount;
	} else {
		GENOS_OS_ASSERTMESSAGE("Reached max # registrations.");
		hResult = E_FAIL;
	}
	return hResult;
}

HRESULT IntelGen_OsVerifyCommandBufferSize(PGENOS_INTERFACE pOsInterface,
					   DWORD dwRequestedSize)
{
	PGENOS_OS_CONTEXT pOsContext;
	GENOS_OS_GPU_CONTEXT OsGpuContext;

	GENOS_OS_ASSERT(pOsInterface);
	GENOS_OS_ASSERT(pOsInterface->pOsContext);

	pOsContext = pOsInterface->pOsContext;
	OsGpuContext =
	    pOsContext->OsGpuContext[pOsInterface->CurrentGpuContextOrdinal];

	if (OsGpuContext.uiCommandBufferSize < dwRequestedSize) {
		return E_FAIL;
	}

	return S_OK;
}

VOID IntelGen_OsResetResourceAllocation(PGENOS_INTERFACE pOsInterface,
					PGENOS_RESOURCE pOsResource)
{
	INT i;

	for (i = 0; i < GENOS_GPU_CONTEXT_MAX; i++) {
		pOsResource->iAllocationIndex[i] = GENOS_INVALID_ALLOC_INDEX;
	}
}

HRESULT IntelGen_OsGetCommandBuffer(PGENOS_INTERFACE pOsInterface,
				    PGENOS_COMMAND_BUFFER pCmdBuffer)
{

	PGENOS_OS_CONTEXT pOsContext;
	HRESULT hr = S_OK;
	GENOS_STATUS eStatus = GENOS_STATUS_SUCCESS;
	PGENOS_OS_GPU_CONTEXT pOsGpuContext;
	UINT uiCommandBufferSize;

	GENOS_OS_CHK_NULL_WITH_HR(pOsInterface);
	GENOS_OS_CHK_NULL_WITH_HR(pOsInterface->pOsContext);
	GENOS_OS_CHK_NULL_WITH_HR(pCmdBuffer);

	pOsContext = pOsInterface->pOsContext;
	pOsGpuContext =
	    &pOsContext->OsGpuContext[pOsInterface->CurrentGpuContextOrdinal];
	uiCommandBufferSize = pOsGpuContext->uiCommandBufferSize;

	if (pOsGpuContext->bCBFlushed == TRUE) {
		if (pOsContext->pfnGetCommandBuffer(pOsContext,
						    pCmdBuffer,
						    uiCommandBufferSize)) {
			GENOS_OS_CHK_HR(pOsContext->pfnInsertCmdBufferToPool
					(pOsContext,
					 pCmdBuffer->OsResource.bo));
			pOsGpuContext->bCBFlushed = FALSE;
			eStatus =
			    GENOS_SecureMemcpy(pOsGpuContext->pCB,
					       sizeof(GENOS_COMMAND_BUFFER),
					       pCmdBuffer,
					       sizeof(GENOS_COMMAND_BUFFER));
			GENOS_OS_CHECK_CONDITION((eStatus !=
						  GENOS_STATUS_SUCCESS),
						 "Failed to copy command buffer",
						 E_FAIL);
		} else {
			GENOS_OS_ASSERTMESSAGE
			    ("Failed to activate command buffer.");
			hr = E_FAIL;
			goto finish;
		}
	}

	GENOS_OS_CHK_HR(pOsInterface->pfnRegisterResource(pOsInterface,
							  &(pOsGpuContext->pCB->
							    OsResource), FALSE,
							  FALSE));
	eStatus =
	    GENOS_SecureMemcpy(pCmdBuffer, sizeof(GENOS_COMMAND_BUFFER),
			       pOsGpuContext->pCB,
			       sizeof(GENOS_COMMAND_BUFFER));
	GENOS_OS_CHECK_CONDITION((eStatus != GENOS_STATUS_SUCCESS),
				 "Failed to copy command buffer", E_FAIL);

 finish:
	return hr;
}

HRESULT IntelGen_OsSetIndirectStateSize(PGENOS_INTERFACE pOsInterface,
					UINT uSize)
{
	PGENOS_CONTEXT pOsContext;
	HRESULT hr;

	GENOS_OS_CHK_NULL_WITH_HR(pOsInterface);

	hr = S_OK;

	pOsContext = pOsInterface->pOsContext;
	GENOS_OS_CHK_NULL_WITH_HR(pOsContext);

	pOsContext->uIndirectStateSize = uSize;

 finish:
	return hr;
}

HRESULT IntelGen_OsGetIndirectState(PGENOS_INTERFACE pOsInterface,
				    UINT * puOffset, UINT * puSize)
{
	PGENOS_CONTEXT pOsContext;
	GENOS_OS_GPU_CONTEXT OsGpuContext;

	pOsContext = pOsInterface->pOsContext;
	if (pOsContext) {
		OsGpuContext =
		    pOsContext->
		    OsGpuContext[pOsInterface->CurrentGpuContextOrdinal];

		if (puOffset) {
			*puOffset =
			    OsGpuContext.uiCommandBufferSize -
			    pOsContext->uIndirectStateSize;
		}

		if (puSize) {
			*puSize = pOsContext->uIndirectStateSize;
		}
	}
	return S_OK;
}

INT IntelGen_OsGetResourceAllocationIndex(PGENOS_INTERFACE pOsInterface,
					  PGENOS_RESOURCE pResource)
{
	GENOS_OS_FUNCTION_ENTER;

	if (pResource) {
		return (pResource->iAllocationIndex
			[pOsInterface->CurrentGpuContextOrdinal]);
	}

	return GENOS_INVALID_ALLOC_INDEX;

}

HRESULT IntelGen_OsGetIndirectStatePointer(PGENOS_INTERFACE pOsInterface,
					   PBYTE * pIndirectState)
{
	PGENOS_OS_CONTEXT pOsContext;
	GENOS_OS_GPU_CONTEXT OsGpuContext;
	HRESULT hr;

	GENOS_OS_FUNCTION_ENTER;

	hr = E_FAIL;
	pOsContext = (pOsInterface) ? pOsInterface->pOsContext : NULL;

	if (pOsContext) {
		OsGpuContext =
		    pOsContext->
		    OsGpuContext[pOsInterface->CurrentGpuContextOrdinal];

		if (OsGpuContext.pCB && OsGpuContext.pCB->pCmdBase) {
			*pIndirectState =
			    (PBYTE) OsGpuContext.pCB->pCmdBase +
			    OsGpuContext.uiCommandBufferSize -
			    pOsContext->uIndirectStateSize;

			hr = S_OK;
		}
	}

	return hr;
}

VOID IntelGen_OsReturnCommandBuffer(PGENOS_INTERFACE pOsInterface,
				    PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	PGENOS_OS_CONTEXT pOsContext;

	pOsContext = (pOsInterface) ? pOsInterface->pOsContext : NULL;
	if (pOsContext == NULL || pCmdBuffer == NULL) {
		GENOS_OS_ASSERTMESSAGE("Invalid parameters.");
		goto finish;
	}

	pOsContext->pfnReturnCommandBuffer(pOsContext,
					   pOsInterface->CurrentGpuContextOrdinal,
					   pCmdBuffer);

 finish:
	return;
}

HRESULT IntelGen_OsSubmitCommandBuffer(PGENOS_INTERFACE pOsInterface,
				       PGENOS_COMMAND_BUFFER pCmdBuffer,
				       BOOL bNullRendering)
{
	PGENOS_CONTEXT pOsContext;
	PGENOS_RESOURCE pResource;
	PGENOS_OS_GPU_CONTEXT pOsGpuContext;
	GENOS_GPU_CONTEXT GpuContext;
	UINT PatchIndex, AllocationIndex;
	HRESULT hr;
	PPATCHLOCATIONLIST pPatchList, pCurrentPatch;
	drm_intel_bo *alloc_bo, *cmd_bo;
	INT ResourceOffset, PatchOffset;
	DWORD dwBatchBufferEndCmd;
	PLATFORM platform;

	hr = S_OK;

	GENOS_OS_CHK_NULL_WITH_HR(pOsInterface);

	pOsContext = pOsInterface->pOsContext;
	GENOS_OS_CHK_NULL_WITH_HR(pOsContext);

	GpuContext = pOsInterface->CurrentGpuContextOrdinal;

	GENOS_OS_CHK_NULL_WITH_HR(pOsContext->OsGpuContext);
	pOsGpuContext = &pOsContext->OsGpuContext[GpuContext];
	GENOS_OS_CHK_NULL_WITH_HR(pOsGpuContext);

	pPatchList = pOsGpuContext->pPatchLocationList;
	GENOS_OS_CHK_NULL_WITH_HR(pPatchList);

	pOsInterface->pfnGetPlatform(pOsInterface, &platform);

	pOsGpuContext->bCBFlushed = TRUE;
	cmd_bo = pCmdBuffer->OsResource.bo;

	for (PatchIndex = 0;
	     PatchIndex < pOsGpuContext->uiCurrentNumPatchLocations;
	     PatchIndex++) {
		pCurrentPatch = &pPatchList[PatchIndex];
		GENOS_OS_CHK_NULL_WITH_HR(pCurrentPatch);

		AllocationIndex = pCurrentPatch->AllocationIndex;
		ResourceOffset = pCurrentPatch->AllocationOffset;
		PatchOffset = pCurrentPatch->PatchOffset;

		pResource = (PGENOS_RESOURCE)
		    pOsGpuContext->pAllocationList[AllocationIndex].hAllocation;
		GENOS_OS_CHK_NULL_WITH_HR(pResource);

		GENOS_OS_ASSERT(pResource->bo);

		alloc_bo = (pResource->bo) ? pResource->bo : cmd_bo;

		*((DWORD *) ((BYTE *) cmd_bo->virt + PatchOffset)) =
		    alloc_bo->offset + ResourceOffset;

		hr = drm_intel_bo_emit_reloc(cmd_bo,
					     PatchOffset,
					     alloc_bo,
					     ResourceOffset, 0x2, 0x0);
		if (hr != 0) {
			GENOS_OS_ASSERTMESSAGE
			    ("Error patching alloc_bo = 0x%x, cmd_bo = 0x%x.",
			     (UINT32) alloc_bo, (UINT32) cmd_bo);
			goto finish;
		}
	}

	dwBatchBufferEndCmd = MI_BATCHBUFFER_END;
	if (GENOS_FAILED(IntelGen_OsAddCommand(pCmdBuffer,
					       &dwBatchBufferEndCmd,
					       sizeof(DWORD)))) {
		hr = E_FAIL;
		goto finish;
	}

	drm_intel_bo_unmap(cmd_bo);

	if (OSKMGetGpuNode(GpuContext) == I915_EXEC_RENDER) {
		{
			hr = drm_intel_bo_exec(cmd_bo,
					       pOsGpuContext->uiCommandBufferSize,
					       NULL, 0, 0);
		}
	} else {
		hr = drm_intel_bo_mrb_exec(cmd_bo,
					   pOsGpuContext->uiCommandBufferSize,
					   NULL,
					   0, 0, OSKMGetGpuNode(GpuContext));
	}

	pOsGpuContext->uiNumAllocations = 0;
	GENOS_ZeroMemory(pOsGpuContext->pAllocationList,
			 sizeof(ALLOCATION_LIST) *
			 pOsGpuContext->uiMaxNumAllocations);
	pOsGpuContext->uiCurrentNumPatchLocations = 0;
	GENOS_ZeroMemory(pOsGpuContext->pPatchLocationList,
			 sizeof(PATCHLOCATIONLIST) *
			 pOsGpuContext->uiMaxPatchLocationsize);
	pOsGpuContext->uiResCount = 0;

 finish:
	return hr;
}

HRESULT IntelGen_OsResizeCommandBufferAndPatchList(PGENOS_INTERFACE
						   pOsInterface,
						   DWORD
						   dwRequestedCommandBufferSize,
						   DWORD
						   dwRequestedPatchListSize)
{
	PGENOS_CONTEXT pOsContext;
	PGENOS_OS_GPU_CONTEXT pOsGpuContext;
	PPATCHLOCATIONLIST pNewPatchList;
	HRESULT hr;

	GENOS_OS_FUNCTION_ENTER;

	GENOS_OS_ASSERT(pOsInterface);
	GENOS_OS_ASSERT(pOsInterface->pOsContext);

	hr = S_OK;

	pOsContext = pOsInterface->pOsContext;
	pOsGpuContext =
	    &(pOsContext->OsGpuContext[pOsInterface->CurrentGpuContextOrdinal]);

	pOsGpuContext->uiCommandBufferSize =
	    ALIGN(dwRequestedCommandBufferSize, 8);

	if (dwRequestedPatchListSize > pOsGpuContext->uiMaxPatchLocationsize) {
		pNewPatchList = (PPATCHLOCATIONLIST)
		    realloc(pOsGpuContext->pPatchLocationList,
			    sizeof(PATCHLOCATIONLIST) *
			    dwRequestedPatchListSize);
		if (NULL == pNewPatchList) {
			GENOS_OS_ASSERTMESSAGE
			    ("pOsGpuContext->pPatchLocationList realloc failed.");
			hr = E_FAIL;
			goto finish;
		}

		pOsGpuContext->pPatchLocationList = pNewPatchList;

		GENOS_ZeroMemory((pOsGpuContext->pPatchLocationList +
				  pOsGpuContext->uiMaxPatchLocationsize),
				 sizeof(PATCHLOCATIONLIST) *
				 (dwRequestedPatchListSize -
				  pOsGpuContext->uiMaxPatchLocationsize));
		pOsGpuContext->uiMaxPatchLocationsize =
		    dwRequestedPatchListSize;
	}

 finish:
	return hr;
}

HRESULT IntelGen_OsResizeCommandBuffer(PGENOS_INTERFACE pOsInterface,
				       DWORD dwRequestedSize)
{
	PGENOS_CONTEXT pOsContext;
	PGENOS_OS_GPU_CONTEXT pOsGpuContext;
	GENOS_GPU_CONTEXT GpuContext;
	HRESULT hr;

	hr = S_OK;

	GENOS_OS_CHK_NULL_WITH_HR(pOsInterface);
	GENOS_OS_CHK_NULL_WITH_HR(pOsInterface->pOsContext);

	pOsContext =
	    &pOsInterface->pOsContext[pOsInterface->CurrentGpuContextOrdinal];
	GENOS_OS_CHK_NULL_WITH_HR(pOsContext);

	GpuContext = pOsInterface->CurrentGpuContextOrdinal;

	GENOS_OS_CHK_NULL_WITH_HR(pOsContext->OsGpuContext);
	pOsGpuContext = &pOsContext->OsGpuContext[GpuContext];
	GENOS_OS_CHK_NULL_WITH_HR(pOsGpuContext);

	pOsGpuContext->uiCommandBufferSize = dwRequestedSize;

 finish:
	return hr;
}

GENOS_FORMAT IntelGen_OsFmt_OsToGen(GENOS_OS_FORMAT format)
{
	switch ((INT) format) {
	case GFX_DDIFMT_A8B8G8R8:
		return Format_A8R8G8B8;
	case GFX_DDIFMT_X8B8G8R8:
		return Format_X8R8G8B8;
	case GFX_DDIFMT_R32F:
		return Format_R32F;
	case GFX_DDIFMT_A8R8G8B8:
		return Format_A8R8G8B8;
	case GFX_DDIFMT_X8R8G8B8:
		return Format_X8R8G8B8;
	case GFX_DDIFMT_R5G6B5:
		return Format_R5G6B5;
	case GFX_DDIFMT_YUY2:
		return Format_YUY2;
	case GFX_DDIFMT_P8:
		return Format_P8;
	case GFX_DDIFMT_A8P8:
		return Format_A8P8;
	case GFX_DDIFMT_A8:
		return Format_A8;
	case GFX_DDIFMT_L8:
		return Format_L8;
	case GFX_DDIFMT_A4L4:
		return Format_A4L4;
	case GFX_DDIFMT_A8L8:
		return Format_A8L8;
	case GFX_DDIFMT_V8U8:
		return Format_V8U8;
	case (GFX_DDIFORMAT) FOURCC_YVYU:
		return Format_YVYU;
	case (GFX_DDIFORMAT) FOURCC_UYVY:
		return Format_UYVY;
	case (GFX_DDIFORMAT) FOURCC_VYUY:
		return Format_VYUY;
	case (GFX_DDIFORMAT) FOURCC_AYUV:
		return Format_AYUV;
	case (GFX_DDIFORMAT) FOURCC_NV12:
		return Format_NV12;
	case (GFX_DDIFORMAT) FOURCC_NV21:
		return Format_NV21;
	case (GFX_DDIFORMAT) FOURCC_NV11:
		return Format_NV11;
	case (GFX_DDIFORMAT) FOURCC_P208:
		return Format_P208;
	case (GFX_DDIFORMAT) FOURCC_IMC1:
		return Format_IMC1;
	case (GFX_DDIFORMAT) FOURCC_IMC2:
		return Format_IMC2;
	case (GFX_DDIFORMAT) FOURCC_IMC3:
		return Format_IMC3;
	case (GFX_DDIFORMAT) FOURCC_IMC4:
		return Format_IMC4;
	case (GFX_DDIFORMAT) FOURCC_I420:
		return Format_I420;
	case (GFX_DDIFORMAT) FOURCC_IYUV:
		return Format_IYUV;
	case (GFX_DDIFORMAT) FOURCC_YV12:
		return Format_YV12;
	case (GFX_DDIFORMAT) FOURCC_YVU9:
		return Format_YVU9;
	case (GFX_DDIFORMAT) FOURCC_AI44:
		return Format_AI44;
	case (GFX_DDIFORMAT) FOURCC_IA44:
		return Format_IA44;
	case (GFX_DDIFORMAT) FOURCC_400P:
		return Format_400P;
	case (GFX_DDIFORMAT) FOURCC_411P:
		return Format_411P;
	case (GFX_DDIFORMAT) FOURCC_422H:
		return Format_422H;
	case (GFX_DDIFORMAT) FOURCC_422V:
		return Format_422V;
	case (GFX_DDIFORMAT) FOURCC_444P:
		return Format_444P;
	case (GFX_DDIFORMAT) FOURCC_RGBP:
		return Format_RGBP;
	case (GFX_DDIFORMAT) FOURCC_BGRP:
		return Format_BGRP;
	default:
		return Format_Invalid;
	}
}

GENOS_OS_FORMAT IntelGen_OsFmt_GenToOs(GENOS_FORMAT format)
{
	switch (format) {
	case Format_A8R8G8B8:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_A8R8G8B8;
	case Format_X8R8G8B8:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_X8R8G8B8;
	case Format_R32U:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_R32F;
	case Format_R32F:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_R32F;
	case Format_R5G6B5:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_R5G6B5;
	case Format_YUY2:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_YUY2;
	case Format_P8:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_P8;
	case Format_A8P8:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_A8P8;
	case Format_A8:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_A8;
	case Format_L8:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_L8;
	case Format_A4L4:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_A4L4;
	case Format_A8L8:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_A8L8;
	case Format_V8U8:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_V8U8;
	case Format_YVYU:
		return (GENOS_OS_FORMAT) FOURCC_YVYU;
	case Format_UYVY:
		return (GENOS_OS_FORMAT) FOURCC_UYVY;
	case Format_VYUY:
		return (GENOS_OS_FORMAT) FOURCC_VYUY;
	case Format_AYUV:
		return (GENOS_OS_FORMAT) FOURCC_AYUV;
	case Format_NV12:
		return (GENOS_OS_FORMAT) FOURCC_NV12;
	case Format_NV21:
		return (GENOS_OS_FORMAT) FOURCC_NV21;
	case Format_NV11:
		return (GENOS_OS_FORMAT) FOURCC_NV11;
	case Format_P208:
		return (GENOS_OS_FORMAT) FOURCC_P208;
	case Format_IMC1:
		return (GENOS_OS_FORMAT) FOURCC_IMC1;
	case Format_IMC2:
		return (GENOS_OS_FORMAT) FOURCC_IMC2;
	case Format_IMC3:
		return (GENOS_OS_FORMAT) FOURCC_IMC3;
	case Format_IMC4:
		return (GENOS_OS_FORMAT) FOURCC_IMC4;
	case Format_I420:
		return (GENOS_OS_FORMAT) FOURCC_I420;
	case Format_IYUV:
		return (GENOS_OS_FORMAT) FOURCC_IYUV;
	case Format_YV12:
		return (GENOS_OS_FORMAT) FOURCC_YV12;
	case Format_YVU9:
		return (GENOS_OS_FORMAT) FOURCC_YVU9;
	case Format_AI44:
		return (GENOS_OS_FORMAT) FOURCC_AI44;
	case Format_IA44:
		return (GENOS_OS_FORMAT) FOURCC_IA44;
	case Format_400P:
		return (GENOS_OS_FORMAT) FOURCC_400P;
	case Format_411P:
		return (GENOS_OS_FORMAT) FOURCC_411P;
	case Format_422H:
		return (GENOS_OS_FORMAT) FOURCC_422H;
	case Format_422V:
		return (GENOS_OS_FORMAT) FOURCC_422V;
	case Format_444P:
		return (GENOS_OS_FORMAT) FOURCC_444P;
	case Format_RGBP:
		return (GENOS_OS_FORMAT) FOURCC_RGBP;
	case Format_BGRP:
		return (GENOS_OS_FORMAT) FOURCC_BGRP;
	case Format_STMM:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_P8;
	default:
		return (GENOS_OS_FORMAT) GFX_DDIFMT_UNKNOWN;
	}
}

VOID IntelGen_OsSleepMs(PGENOS_INTERFACE pOsInterface, DWORD dwWaitMs)
{
	usleep(dwWaitMs);
}

HRESULT IntelGen_OsResetCommandBuffer(PGENOS_INTERFACE pOsInterface,
				      PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	PGENOS_OS_CONTEXT pOsContext;
	PGENOS_OS_GPU_CONTEXT pOsGpuContext;

	GENOS_OS_FUNCTION_ENTER;

	pOsContext = pOsInterface->pOsContext;
	pOsGpuContext =
	    &pOsContext->OsGpuContext[pOsInterface->CurrentGpuContextOrdinal];

	pOsGpuContext->bCBFlushed = TRUE;

	return S_OK;
}

HRESULT IntelGen_OsInitInterface(PGENOS_INTERFACE pOsInterface,
				 PGENOS_CONTEXT pOsDriverContext)
{
	PGENOS_OS_CONTEXT pOsContext;
	HRESULT hr;

	GENOS_OS_FUNCTION_ENTER;

	pOsContext = NULL;
	hr = S_OK;

	GENOS_OS_NORMALMESSAGE("mm:IntelGen_OsInitInterface called.");

	GENOS_OS_CHK_NULL_WITH_HR(pOsInterface);
	GENOS_OS_CHK_NULL_WITH_HR(pOsDriverContext);

	pOsContext = (PGENOS_OS_CONTEXT)
	    GENOS_AllocAndZeroMemory(sizeof(GENOS_OS_CONTEXT));
	if (pOsContext == NULL) {
		GENOS_OS_ASSERTMESSAGE("Unable to allocate memory.");
		hr = E_OUTOFMEMORY;
		goto finish;
	}
	hr = Ctx_InitContext(pOsContext, pOsDriverContext);
	if (S_OK != hr) {
		GENOS_OS_ASSERTMESSAGE("Unable to initialize context.");
		goto finish;
	}

	pOsContext->bFreeContext = TRUE;
	pOsInterface->OS = GENOS_OS_LINUX;
	pOsInterface->pOsContext = pOsContext;
	pOsInterface->bUsesPatchList = TRUE;
	pOsInterface->bUsesGfxAddress = FALSE;
	pOsInterface->bNoParsingAssistanceInKmd = TRUE;
	pOsInterface->bUsesCmdBufHeaderInResize = FALSE;
	pOsInterface->bUsesCmdBufHeader = FALSE;
	pOsInterface->dwNumNalUnitBytesIncluded =
	    GENOS_NAL_UNIT_LENGTH - GENOS_NAL_UNIT_STARTCODE_LENGTH;

	drm_intel_bufmgr_gem_enable_reuse(pOsContext->bufmgr);

	pOsInterface->pfnGetPlatform = IntelGen_OsGetPlatform;
	pOsInterface->pfnDestroy = IntelGen_OsDestroy;

	pOsInterface->pfnResetOsStates = IntelGen_OsResetOsStates;
	pOsInterface->pfnAllocateResource = IntelGen_OsAllocateResource;
	pOsInterface->pfnFreeResource = IntelGen_OsFreeResource;
	pOsInterface->pfnLockResource = IntelGen_OsLockResource;
	pOsInterface->pfnUnlockResource = IntelGen_OsUnlockResource;
	pOsInterface->pfnRegisterResource = IntelGen_OsRegisterResource;
	pOsInterface->pfnResetResourceAllocationIndex =
	    IntelGen_OsResetResourceAllocation;
	pOsInterface->pfnGetResourceAllocationIndex =
	    IntelGen_OsGetResourceAllocationIndex;
	pOsInterface->pfnGetCommandBuffer = IntelGen_OsGetCommandBuffer;
	pOsInterface->pfnResetCommandBuffer = IntelGen_OsResetCommandBuffer;
	pOsInterface->pfnReturnCommandBuffer = IntelGen_OsReturnCommandBuffer;
	pOsInterface->pfnSubmitCommandBuffer = IntelGen_OsSubmitCommandBuffer;
	pOsInterface->pfnVerifyCommandBufferSize =
	    IntelGen_OsVerifyCommandBufferSize;
	pOsInterface->pfnResizeCommandBufferAndPatchList =
	    IntelGen_OsResizeCommandBufferAndPatchList;
	pOsInterface->pfnFmt_OsToGen = IntelGen_OsFmt_OsToGen;
	pOsInterface->pfnFmt_GenToOs = IntelGen_OsFmt_GenToOs;
	pOsInterface->pfnSetIndirectStateSize = IntelGen_OsSetIndirectStateSize;
	pOsInterface->pfnGetIndirectState = IntelGen_OsGetIndirectState;
	pOsInterface->pfnGetIndirectStatePointer =
	    IntelGen_OsGetIndirectStatePointer;
	pOsInterface->pfnSetPatchEntry = IntelGen_OsSetPatchEntry;

	pOsInterface->pfnSleepMs = IntelGen_OsSleepMs;

	hr = S_OK;

 finish:
	if (S_OK != hr && NULL != pOsContext) {
		if (pOsContext->fd >= 0) {
			close(pOsContext->fd);
		}
		GENOS_FreeMemAndSetNull(pOsContext);
	}
	return hr;
}

BOOL IntelGen_OsResourceIsNull(PGENOS_RESOURCE pOsResource)
{
	GENOS_OS_ASSERT(pOsResource);

	return ((pOsResource->bo == NULL));
}

VOID IntelGen_OsResetResource(PGENOS_RESOURCE pOsResource)
{
	INT i;

	GENOS_OS_FUNCTION_ENTER;

	GENOS_OS_ASSERT(pOsResource);

	GENOS_ZeroMemory(pOsResource, sizeof(GENOS_RESOURCE));
	pOsResource->Format = Format_None;
	for (i = 0; i < GENOS_GPU_CONTEXT_MAX; i++) {
		pOsResource->iAllocationIndex[i] = GENOS_INVALID_ALLOC_INDEX;
	}
}

HRESULT IntelGen_OsWaitOnResource(PGENOS_INTERFACE pOsInterface,
				  PGENOS_RESOURCE pOsResource)
{
	HRESULT hr;
	GENOS_LOCK_PARAMS LockFlags;

	GENOS_OS_ASSERT(pOsInterface);
	GENOS_OS_ASSERT(pOsResource);
	GENOS_OS_ASSERT(pOsInterface->pOsContext);

	hr = S_OK;

	GENOS_ZeroMemory(&LockFlags, sizeof(GENOS_LOCK_PARAMS));

	LockFlags.WriteOnly = 1;

	GENOS_OS_CHK_NULL_WITH_HR(pOsInterface->pfnLockResource(pOsInterface,
								pOsResource,
								&LockFlags));

	GENOS_OS_CHK_HR(pOsInterface->pfnUnlockResource
			(pOsInterface, pOsResource));

 finish:
	return hr;
}

HRESULT IntelGen_OsInitInterface(PGENOS_INTERFACE pOsInterface,
				 PGENOS_CONTEXT pOsDriverContext,
				 GENOS_COMPONENT component)
{
	HRESULT hr = E_FAIL;

	pOsInterface->pfnWaitOnResource = IntelGen_OsWaitOnResource;
	hr = IntelGen_OsInitInterface(pOsInterface, pOsDriverContext);

	return hr;
}

GENOS_TILE_TYPE OsToGenTileType(UINT type)
{
	switch (type) {
	case I915_TILING_NONE:
		return GENOS_TILE_LINEAR;
	case I915_TILING_X:
		return GENOS_TILE_X;
	case I915_TILING_Y:
		return GENOS_TILE_Y;
	default:
		return GENOS_TILE_INVALID;
	}
};

#define  PATH_MAX 256
static int FindDeviceNum(const char *name)
{
	int fd = -1;
	char path[PATH_MAX];
	drmVersionPtr version;
	int num;

	for (num = 0; num < 32; num++) {
		sprintf(path, "/dev/dri/card%d", num);
		if ((fd = open(path, O_RDWR)) >= 0) {
			if ((version = drmGetVersion(fd))) {
				if (!strcmp(version->name, name)) {
					drmFreeVersion(version);
					close(fd);
					break;
				}
				drmFreeVersion(version);
			}
			close(fd);
		}
	}
	return num;
}

static int OpenDevice()
{
	int num = FindDeviceNum("i915");
	int fd = -1;
	char path[PATH_MAX];
	sprintf(path, "/dev/dri/renderD%d", 128 + num);
	fd = open(path, O_RDWR);
	if (fd < 0) {
		sprintf(path, "/dev/dri/card%d", num);
		fd = open(path, O_RDWR);
	}
	return fd;
}

static inline void platform_setTypeAndFamily(PLATFORM * pPlatform,
					     const PLATFORM_TYPE platformType,
					     const PRODUCT_FAMILY productFamily,
					     const GFXCORE_FAMILY displayFamily,
					     const GFXCORE_FAMILY renderFamily)
{
	GENOS_OS_ASSERT(pPlatform);

	pPlatform->ePlatformType = platformType;
	pPlatform->eProductFamily = productFamily;
	pPlatform->ePCHProductFamily = PCH_UNKNOWN;
	pPlatform->eDisplayCoreFamily = displayFamily;
	pPlatform->eRenderCoreFamily = renderFamily;
}

static int updatePlatformInfo(PLATFORM * pPlatform)
{
	if (!pPlatform) {
		return -EINVAL;
	}

	const unsigned int uDeviceID = pPlatform->pchDeviceID;

	switch (uDeviceID) {
		/* HSW DESKTOP */
	case IHSW_CL_DESK_GT1_DEV_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_DESKTOP,
					  IGFX_HASWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN7_5_CORE);
		pPlatform->GtType = GTTYPE_GT1;
		break;
	case IHSW_GTH_DESK_DEVICE_F0_ID:
	case IHSW_GTM_DESK_DEVICE_F0_ID:
	case IHSW_GTL_DESK_DEVICE_F0_ID:
	case IHSW_DESK_DEV_F0_ID:
	case IHSW_DESK_DEV_F0_M_ID:
	case IHSW_DESK_DEV_F0_H_ID:
	case IHSW_CL_DESK_GT2_DEV_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_DESKTOP,
					  IGFX_HASWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN7_5_CORE);
		pPlatform->GtType = GTTYPE_GT2;
		break;
	case IHSW_CRW_DESK_GT2_DEV_ID:
	case IHSW_CRW_DESK_GT3_DEV_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_DESKTOP,
					  IGFX_HASWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN7_5_CORE);
		pPlatform->GtType = GTTYPE_GT3;
		break;

		/* HSW MOBILE */
	case IHSW_CL_MOBL_GT1_DEV_ID:
	case IHSW_ULT_MOBL_GT1_DEV_ID:
	case IHSW_ULX_MOBL_GT1_DEV_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_MOBILE,
					  IGFX_HASWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN7_5_CORE);
		pPlatform->GtType = GTTYPE_GT1;
		break;
	case IHSW_MOBL_DEV_F0_ID:
	case IHSW_MOBL_DEV_F0_M_ID:
	case IHSW_MOBL_DEV_F0_H_ID:
	case IHSW_VA_DEV_F0_ID:
	case IHSW_MOBL_DEVICE_F0_ID:
	case IHSW_CL_MOBL_GT2_DEV_ID:
	case IHSW_ULT_MOBL_GT2_DEV_ID:
	case IHSW_ULX_MOBL_GT2_DEV_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_MOBILE,
					  IGFX_HASWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN7_5_CORE);
		pPlatform->GtType = GTTYPE_GT2;
		break;
	case IHSW_ULT_MOBL_GT3_DEV_ID:
	case IHSW_ULT_MRKT_GT3_DEV_ID:
	case IHSW_CRW_MOBL_GT2_DEV_ID:
	case IHSW_CRW_MOBL_GT3_DEV_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_MOBILE,
					  IGFX_HASWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN7_5_CORE);
		pPlatform->GtType = GTTYPE_GT3;
		break;
		/* HSW SERVER */
	case IHSW_CL_SERV_GT1_DEV_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_DESKTOP,
					  IGFX_HASWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN7_5_CORE);
		pPlatform->GtType = GTTYPE_GT1;
		break;
	case IHSW_CL_SERV_GT2_DEV_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_DESKTOP,
					  IGFX_HASWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN7_5_CORE);
		pPlatform->GtType = GTTYPE_GT2;
		break;
	case IHSW_CRW_SERV_GT3_DEV_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_DESKTOP,
					  IGFX_HASWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN7_5_CORE);
		pPlatform->GtType = GTTYPE_GT3;
		break;

	case IBDW_GT0_DESK_DEVICE_F0_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_DESKTOP,
					  IGFX_BROADWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN8_CORE);
		pPlatform->GtType = GTTYPE_GT0;
		break;
	case IBDW_GT1_SERV_DEVICE_F0_ID:
	case IBDW_GT1_WRK_DEVICE_F0_ID:
	case IBDW_GT1_ULX_DEVICE_F0_ID:
	case IBDW_GT1_RSVD_DEVICE_F0_ID:
	case IBDW_GT1_DESK_DEVICE_F0_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_DESKTOP,
					  IGFX_BROADWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN8_CORE);
		pPlatform->GtType = GTTYPE_GT1;
		break;
	case IBDW_GT2_DESK_DEVICE_F0_ID:
	case IBDW_GT2_RSVD_DEVICE_F0_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_DESKTOP,
					  IGFX_BROADWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN8_CORE);
		pPlatform->GtType = GTTYPE_GT2;
		break;
	case IBDW_GT1_HALO_MOBL_DEVICE_F0_ID:
	case IBDW_GT1_ULT_MOBL_DEVICE_F0_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_MOBILE,
					  IGFX_BROADWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN8_CORE);
		pPlatform->GtType = GTTYPE_GT1;
		break;
	case IBDW_GT2_HALO_MOBL_DEVICE_F0_ID:
	case IBDW_GT2_ULT_MOBL_DEVICE_F0_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_MOBILE,
					  IGFX_BROADWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN8_CORE);
		pPlatform->GtType = GTTYPE_GT2;
		break;
	case IBDW_GT2_SERV_DEVICE_F0_ID:
	case IBDW_GT2_WRK_DEVICE_F0_ID:
	case IBDW_GT2_ULX_DEVICE_F0_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_DESKTOP,
					  IGFX_BROADWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN8_CORE);
		pPlatform->GtType = GTTYPE_GT2;
		break;
	case IBDW_GT3_ULT_MOBL_DEVICE_F0_ID:
	case IBDW_GT3_HALO_MOBL_DEVICE_F0_ID:
	case IBDW_GT3_ULT25W_MOBL_DEVICE_F0_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_MOBILE,
					  IGFX_BROADWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN8_CORE);
		pPlatform->GtType = GTTYPE_GT3;
		break;
	case IBDW_GT3_SERV_DEVICE_F0_ID:
	case IBDW_GT3_WRK_DEVICE_F0_ID:
	case IBDW_GT3_ULX_DEVICE_F0_ID:
	case IBDW_GT3_DESK_DEVICE_F0_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_DESKTOP,
					  IGFX_BROADWELL, IGFX_GEN7_5_CORE,
					  IGFX_GEN8_CORE);
		pPlatform->GtType = GTTYPE_GT3;
		break;
	case ICHV_MOBL_DEVICE_F0_ID:
	case ICHV_PLUS_MOBL_DEVICE_F0_ID:
	case ICHV_DESK_DEVICE_F0_ID:
	case ICHV_PLUS_DESK_DEVICE_F0_ID:
		platform_setTypeAndFamily(pPlatform, PLATFORM_MOBILE,
					  IGFX_CHERRYVIEW, IGFX_GEN5_CORE,
					  IGFX_GEN8_CORE);
		pPlatform->GtType = GTTYPE_GT1;
		break;

	default:
		GENOS_OS_ASSERTMESSAGE("Unrecognized device ID %04X",
				       uDeviceID);
		return -ENODEV;
	}

	pPlatform->usRevId = pPlatform->usRevId;
	pPlatform->usRevId_PCH = pPlatform->usRevId_PCH;
	pPlatform->ePlatformType = pPlatform->ePlatformType;

	return 0;
}
