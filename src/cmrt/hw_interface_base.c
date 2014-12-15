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
#include "hw_interface_g75.h"

GENOS_STATUS IntelGen_HwSendCommandBufferHeader(PGENHW_HW_INTERFACE
						pHwInterface,
						PGENOS_COMMAND_BUFFER
						pCmdBuffer)
{
	return GENOS_STATUS_SUCCESS;
}

#define GENHW_KERNEL_SECURITY_HEADER_SIZE          32
#define GENHW_KERNEL_SECURITY_HEADER_FIELD_SIZE    4
#define GENHW_VSC_REGISTER_TIMEOUT_MS              300

__inline void IntelGen_KernelSecurity_GetData(PBYTE pbSignedData,
					      PUINT32 puiBuf,
					      PUINT puiOffset,
					      PBYTE * ppbData,
					      PUINT puiDataSize)
{
	*ppbData = (PBYTE) pbSignedData + (*(puiBuf + *puiOffset));
	(*puiOffset) += 1;

	*puiDataSize = *(puiBuf + *puiOffset);
	(*puiOffset) += 1;
}

GENOS_STATUS IntelGen_HwResetHwStates(PGENHW_HW_INTERFACE pHwInterface)
{
	PGENOS_INTERFACE pOsInterface;
	PGENHW_GSH pGSH;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pOsInterface);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap);

	eStatus = GENOS_STATUS_SUCCESS;
	pOsInterface = pHwInterface->pOsInterface;
	pGSH = pHwInterface->pGeneralStateHeap;

	GENHW_HW_CHK_STATUS(pOsInterface->pfnRegisterResource(pOsInterface,
							      &pGSH->OsResource,
							      TRUE, TRUE));
 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwAllocateSshBuffer(PGENHW_HW_INTERFACE pHwInterface,
					  PGENHW_SSH pSSH)
{
	GENOS_STATUS eStatus;
	PGENOS_INTERFACE pOsInterface;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pSSH);

	eStatus = GENOS_STATUS_UNKNOWN;

	pOsInterface = pHwInterface->pfnGetOsInterface(pHwInterface);

	if (pSSH->pSshBuffer != NULL) {
		GENHW_HW_ASSERTMESSAGE("SSH buffer already allocated.");
		goto finish;
	}
	pSSH->iBindingTableSize =
	    GENOS_ALIGN_CEIL(pHwInterface->SshSettings.iSurfacesPerBT *
			     sizeof(BINDING_TABLE_STATE_G5),
			     pHwInterface->SshSettings.iBTAlignment);

	pSSH->iBindingTableOffset = 0;
	pSSH->iSurfaceStateOffset = pHwInterface->SshSettings.iBindingTables *
	    pSSH->iBindingTableSize;

	pSSH->dwSshIntanceSize = pSSH->iSurfaceStateOffset +
	    (pHwInterface->SshSettings.iSurfaceStates *
	     sizeof(GENHW_SURFACE_STATE));

	pSSH->dwSshSize = pSSH->dwSshIntanceSize;

	pHwInterface->dwIndirectHeapSize =
	    GENOS_ALIGN_CEIL(pSSH->dwSshIntanceSize, 0x00001000);

	GENHW_HW_CHK_STATUS(pOsInterface->pfnSetIndirectStateSize(pOsInterface,
								  pHwInterface->dwIndirectHeapSize));

	pSSH->pSshBuffer = (PBYTE) GENOS_AllocAndZeroMemory(pSSH->dwSshSize);
	if (!pSSH->pSshBuffer) {
		GENHW_HW_ASSERTMESSAGE("Fail to Allocate SSH buffer.");
		goto finish;
	}
	pSSH->iCurSshBufferIndex = 0;
	pSSH->iCurrentBindingTable = 0;
	pSSH->iCurrentSurfaceState = 0;

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

VOID IntelGen_HwFreeSshBuffer(PGENHW_HW_INTERFACE pHwInterface, PGENHW_SSH pSSH)
{
	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pSSH);

	if (pSSH->pSshBuffer) {
		GENOS_FreeMemory(pSSH->pSshBuffer);
		pSSH->pSshBuffer = NULL;
	}
}

GENOS_STATUS IntelGen_HwAssignSshInstance(PGENHW_HW_INTERFACE pHwInterface)
{
	GENOS_STATUS eStatus;
	PGENHW_SSH pSSH;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pSurfaceStateHeap);

	eStatus = GENOS_STATUS_SUCCESS;

	pSSH = (pHwInterface) ? pHwInterface->pSurfaceStateHeap : NULL;

	if (pSSH) {
		pSSH->iCurrentBindingTable = 0;
		pSSH->iCurrentSurfaceState = 0;
	} else {
		eStatus = GENOS_STATUS_UNKNOWN;
	}

	return eStatus;
}

GENOS_STATUS IntelGen_HwSetupSurfaceState_g75(PGENHW_HW_INTERFACE pHwInterface,
					      PGENHW_SURFACE pSurface,
					      PGENHW_SURFACE_STATE_PARAMS
					      pParams,
					      PGENHW_SURFACE_STATE_ENTRY
					      pSurfaceEntry)
{
	PGENHW_HW_COMMANDS pHwCommands;
	PPACKET_SURFACE_STATE_G75 pPacketSurfaceState;
	PSURFACE_STATE_TOKEN_G75 pTokenState;
	PGENOS_INTERFACE pOsInterface;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pOsInterface);
	GENHW_HW_ASSERT(pSurface);
	GENHW_HW_ASSERT(pParams);
	GENHW_HW_ASSERT(pSurfaceEntry);

	pHwCommands = pHwInterface->pHwCommands;
	pPacketSurfaceState =
	    &pSurfaceEntry->pSurfaceState->PacketSurfaceState_g75;
	pTokenState = &pPacketSurfaceState->Token;
	pOsInterface = pHwInterface->pOsInterface;

	*pTokenState = *(pHwCommands->pSurfaceStateToken_g75);

	pTokenState->DW1.SurfaceAllocationIndex =
	    pOsInterface->pfnGetResourceAllocationIndex(pOsInterface,
							&
							(pSurface->OsResource));
	GENHW_HW_ASSERT(pTokenState->DW1.SurfaceAllocationIndex !=
			GENOS_INVALID_ALLOC_INDEX);
	pTokenState->DW3.RenderTargetEnable = pSurfaceEntry->bRenderTarget;
	pTokenState->DW3.YUVPlane = pSurfaceEntry->YUVPlane;

	pTokenState->DW3.SurfaceStateType = 0;

	switch (pSurfaceEntry->YUVPlane) {
	case GENHW_U_PLANE:
		pTokenState->DW2.SurfaceOffset =
		    pSurface->UPlaneOffset.iSurfaceOffset;
		break;
	case GENHW_V_PLANE:
		pTokenState->DW2.SurfaceOffset =
		    pSurface->VPlaneOffset.iSurfaceOffset;
		break;
	default:
		pTokenState->DW2.SurfaceOffset = pSurface->dwOffset;
		break;
	}

	return GENOS_STATUS_SUCCESS;
}

GENOS_STATUS IntelGen_HwBindSurfaceState(PGENHW_HW_INTERFACE pHwInterface,
					 INT iBindingTableIndex,
					 INT iBindingTableEntry,
					 PGENHW_SURFACE_STATE_ENTRY
					 pSurfaceEntry)
{
	PGENHW_SSH pSSH;
	PBINDING_TABLE_STATE_G5 pBindingTableEntry;
	DWORD dwOffset;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pSurfaceEntry);
	GENHW_HW_ASSERT(pHwInterface->pSurfaceStateHeap);
	GENHW_HW_ASSERT(iBindingTableIndex >= 0);
	GENHW_HW_ASSERT(iBindingTableEntry >= 0);

	pSSH = pHwInterface->pSurfaceStateHeap;

	dwOffset = (pSSH->iCurSshBufferIndex * pSSH->dwSshIntanceSize) +
	    (pSSH->iBindingTableOffset) +
	    (iBindingTableIndex * pSSH->iBindingTableSize) +
	    (iBindingTableEntry * sizeof(BINDING_TABLE_STATE_G5));

	pBindingTableEntry =
	    (PBINDING_TABLE_STATE_G5) (pSSH->pSshBuffer + dwOffset);

	pBindingTableEntry->DW0.Enable = TRUE;
	pBindingTableEntry->DW0.Copy = TRUE;
	pBindingTableEntry->DW0.BindingTableStateType = 0;

	pBindingTableEntry->DW0.SurfaceStatePointer =
	    pSurfaceEntry->dwSurfStateOffset >> 5;

	return GENOS_STATUS_SUCCESS;
}

GENOS_STATUS IntelGen_HwSendSurfaces_g75_PatchList(PGENHW_HW_INTERFACE
						   pHwInterface,
						   PGENOS_COMMAND_BUFFER
						   pCmdBuffer)
{
	PGENOS_INTERFACE pOsInterface;
	PBYTE pIndirectState;
	UINT IndirectStateBase;
	UINT IndirectStateSize;
	PGENHW_SSH pSSH;
	INT iBindingTableOffs;
	PBYTE pBindingTablePtr;
	PSURFACE_STATE_TOKEN_G75 pSurfaceStateToken;
	PBINDING_TABLE_STATE_G5 pBindingTableCurrent;
	PBINDING_TABLE_STATE_G5 pBindingTableOutput;
	PGENHW_SURFACE_STATE pSurfaceState;
	PGENHW_SURFACE_STATE pSurfaceStateOutput;
	INT iSurfaceStateOffset;
	INT iSurfaceState, iSurfacesPerBT;
	INT i;
	INT j;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);
	GENHW_HW_ASSERT(pHwInterface->pSurfaceStateHeap);

	eStatus = GENOS_STATUS_SUCCESS;
	pSSH = pHwInterface->pSurfaceStateHeap;

	pOsInterface = pHwInterface->pfnGetOsInterface(pHwInterface);
	GENHW_HW_CHK_STATUS(pOsInterface->pfnGetIndirectState(pOsInterface,
							      &IndirectStateBase,
							      &IndirectStateSize));
	pIndirectState = (PBYTE) pCmdBuffer->pCmdBase + IndirectStateBase;

	pSurfaceState =
	    (PGENHW_SURFACE_STATE) (pSSH->pSshBuffer +
				    pSSH->iSurfaceStateOffset);
	iBindingTableOffs = pSSH->iBindingTableOffset;
	pBindingTablePtr = pSSH->pSshBuffer + iBindingTableOffs;

	iSurfacesPerBT = pHwInterface->SshSettings.iSurfacesPerBT;
	for (i = pSSH->iCurrentBindingTable; i > 0; i--,
	     iBindingTableOffs += pSSH->iBindingTableSize,
	     pBindingTablePtr += pSSH->iBindingTableSize) {
		pBindingTableCurrent =
		    (PBINDING_TABLE_STATE_G5) pBindingTablePtr;
		pBindingTableOutput =
		    (PBINDING_TABLE_STATE_G5) (pIndirectState +
					       iBindingTableOffs);

		for (j = iSurfacesPerBT; j > 0;
		     j--, pBindingTableCurrent++, pBindingTableOutput++) {
			pBindingTableOutput->DW0.Value = 0;
			pBindingTableOutput->DW0.SurfaceStatePointer =
			    pBindingTableCurrent->DW0.SurfaceStatePointer;

			if (!pBindingTableCurrent->DW0.Copy) {
				continue;
			}
			iSurfaceStateOffset = pBindingTableOutput->DW0.Value;
			iSurfaceState =
			    (iSurfaceStateOffset -
			     pSSH->iSurfaceStateOffset) >> 5;
			pSurfaceStateToken =
			    &pSurfaceState
			    [iSurfaceState].PacketSurfaceState_g75.Token;
			pSurfaceStateOutput =
			    (PGENHW_SURFACE_STATE) (pIndirectState +
						    iSurfaceStateOffset);

			if (pSurfaceStateToken->DW3.SurfaceStateType == 0) {
				iSurfaceStateOffset += 1 * sizeof(DWORD);

				GENOS_SecureMemcpy(pSurfaceStateOutput,
						   sizeof(SURFACE_STATE_G7),
						   &pSurfaceState
						   [iSurfaceState].PacketSurfaceState_g75.cmdSurfaceState_g75,
						   sizeof(SURFACE_STATE_G7));
			} else {
				GENHW_HW_ASSERT(0);
			}

			pOsInterface->pfnSetPatchEntry(pOsInterface,
						       pSurfaceStateToken->
						       DW1.SurfaceAllocationIndex,
						       pSurfaceStateToken->
						       DW2.SurfaceOffset,
						       IndirectStateBase +
						       iSurfaceStateOffset);
		}
	}

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendSurfaces_g8(PGENHW_HW_INTERFACE pHwInterface,
					PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	PGENOS_INTERFACE pOsInterface;
	PBYTE pIndirectState;
	UINT IndirectStateBase;
	UINT IndirectStateSize;
	PGENHW_SSH pSSH;
	PBYTE pBindingTablePtr;
	PSURFACE_STATE_TOKEN_G75 pSurfaceStateToken;
	PBINDING_TABLE_STATE_G5 pBindingTableCurrent;
	PBINDING_TABLE_STATE_G5 pBindingTableOutput;
	PGENHW_SURFACE_STATE pSurfaceState;
	PGENHW_SURFACE_STATE pSurfaceStateBase;
	PSURFACE_STATE_G8 pSurfaceStateOutput;
	INT iSurfaceStateOffset;
	INT iSurfaceStateIndex;
	INT iCurrentBindingTableOffset;
	INT i;
	INT j;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);
	GENHW_HW_ASSERT(pHwInterface->pSurfaceStateHeap);

	eStatus = GENOS_STATUS_SUCCESS;
	pSSH = pHwInterface->pSurfaceStateHeap;
	pOsInterface = pHwInterface->pfnGetOsInterface(pHwInterface);

	GENHW_HW_CHK_STATUS(pOsInterface->pfnGetIndirectState(pOsInterface,
							      &IndirectStateBase,
							      &IndirectStateSize));

	pIndirectState = (PBYTE) pCmdBuffer->pCmdBase + IndirectStateBase;

	pSurfaceStateBase = (PGENHW_SURFACE_STATE)
	    (pSSH->pSshBuffer + pSSH->iSurfaceStateOffset);

	iCurrentBindingTableOffset = pSSH->iBindingTableOffset;
	pBindingTablePtr = pSSH->pSshBuffer + pSSH->iBindingTableOffset;

	for (i = pSSH->iCurrentBindingTable;
	     i > 0;
	     i--, iCurrentBindingTableOffset +=
	     pSSH->iBindingTableSize, pBindingTablePtr +=
	     pSSH->iBindingTableSize) {
		pBindingTableCurrent =
		    (PBINDING_TABLE_STATE_G5) pBindingTablePtr;
		pBindingTableOutput =
		    (PBINDING_TABLE_STATE_G5) (pIndirectState +
					       iCurrentBindingTableOffset);

		for (j = pHwInterface->SshSettings.iSurfacesPerBT;
		     j > 0;
		     j--, pBindingTableCurrent++, pBindingTableOutput++) {
			pBindingTableOutput->DW0.Value = 0;
			pBindingTableOutput->DW0.SurfaceStatePointer =
			    pBindingTableCurrent->DW0.SurfaceStatePointer;

			if (!pBindingTableCurrent->DW0.Copy) {
				continue;
			}
			iSurfaceStateOffset = pBindingTableOutput->DW0.Value;
			iSurfaceStateIndex =
			    (iSurfaceStateOffset -
			     pSSH->iSurfaceStateOffset) >> 6;

			GENHW_HW_ASSERT(iSurfaceStateOffset >= 0);
			GENHW_HW_ASSERT(iSurfaceStateIndex >= 0);

			pSurfaceState = pSurfaceStateBase + iSurfaceStateIndex;
			pSurfaceStateToken =
			    &pSurfaceState->PacketSurfaceState_g8.Token;
			pSurfaceStateOutput =
			    (PSURFACE_STATE_G8) (pIndirectState +
						 iSurfaceStateOffset);

			if (pSurfaceStateToken->DW3.SurfaceStateType == 0) {
				iSurfaceStateOffset += 8 * sizeof(DWORD);

				GENOS_SecureMemcpy(pSurfaceStateOutput,
						   sizeof(SURFACE_STATE_G8),
						   &pSurfaceState->PacketSurfaceState_g8.cmdSurfaceState_g8,
						   sizeof(SURFACE_STATE_G8));
			} else {
				GENHW_HW_ASSERT(0);
			}

			pOsInterface->pfnSetPatchEntry(pOsInterface,
						       pSurfaceStateToken->
						       DW1.SurfaceAllocationIndex,
						       pSurfaceStateToken->
						       DW2.SurfaceOffset,
						       IndirectStateBase +
						       iSurfaceStateOffset);
		}
	}

 finish:
	GENHW_HW_ASSERT(eStatus == GENOS_STATUS_SUCCESS);
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendSyncTag_g75(PGENHW_HW_INTERFACE pHwInterface,
					PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	PGENHW_GSH pGSH;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap);

	pGSH = pHwInterface->pGeneralStateHeap;

	GENHW_HW_CHK_STATUS(pHwInterface->pfnSendPipeControl(pHwInterface,
							     pCmdBuffer,
							     &pGSH->OsResource,
							     FALSE,
							     0,
							     GFX3DCONTROLOP_NOWRITE,
							     GFX3DFLUSH_WRITE_CACHE,
							     0));

	GENHW_HW_CHK_STATUS(pHwInterface->pfnSendPipeControl(pHwInterface,
							     pCmdBuffer,
							     &pGSH->OsResource,
							     TRUE,
							     pGSH->dwOffsetSync,
							     GFX3DCONTROLOP_WRITEIMMEDIATE,
							     GFX3DFLUSH_READ_CACHE,
							     pGSH->dwNextTag));

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendLoadRegImmCmd_g75(PGENHW_HW_INTERFACE pHwInterface,
					      PGENOS_COMMAND_BUFFER pCmdBuffer,
					      PGENHW_LOAD_REGISTER_IMM_PARAM
					      pParam)
{
	GENOS_STATUS eStatus;
	MI_LOAD_REGISTER_IMM_CMD_G6 Cmd;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pParam);

	eStatus = GENOS_STATUS_SUCCESS;

	Cmd = *pHwInterface->pHwCommands->pLoadRegImm_g75;
	Cmd.DW1.RegisterAddress = pParam->dwRegisterAddress >> 2;
	Cmd.DW2.DataDword = pParam->dwData;

	GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer,
						  (PVOID) & Cmd,
						  sizeof
						  (MI_LOAD_REGISTER_IMM_CMD_G6)));
 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendPipeControl_g75(PGENHW_HW_INTERFACE
					    pHwInterface,
					    PGENOS_COMMAND_BUFFER pCmdBuffer,
					    PGENOS_RESOURCE pOsResource,
					    BOOL AllocationEnable,
					    DWORD AllocationOffset,
					    INT ControlMode, INT FlushMode,
					    DWORD dwSyncWord)
{
	PGENOS_INTERFACE pOsInterface;
	PIPE_CONTROL_CMD_G7 PipeControl;
	PGENHW_HW_COMMANDS pHwCommands;
	INT iAllocationIndex;
	UINT uiPatchOffset;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pOsResource);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);
	GENHW_HW_ASSERT(pOsResource->iAllocationIndex !=
			GENOS_INVALID_ALLOC_INDEX);

	eStatus = GENOS_STATUS_SUCCESS;
	pOsInterface = pHwInterface->pOsInterface;
	pHwCommands = pHwInterface->pHwCommands;
	iAllocationIndex =
	    pOsInterface->pfnGetResourceAllocationIndex(pOsInterface,
							pOsResource);

	uiPatchOffset = pCmdBuffer->iOffset + (2 * sizeof(DWORD));

	GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry(pOsInterface,
							   iAllocationIndex,
							   AllocationOffset,
							   uiPatchOffset));

	PipeControl = *(pHwCommands->pPipeControl_g75);

	switch (FlushMode) {
	case GFX3DFLUSH_WRITE_CACHE:
		PipeControl.DW1.RenderTargetCacheFlushEnable = TRUE;
		PipeControl.DW1.DCFlushEnable = TRUE;
		break;

	case GFX3DFLUSH_READ_CACHE:
		PipeControl.DW1.RenderTargetCacheFlushEnable = FALSE;
		PipeControl.DW1.StateCacheInvalidationEnable = TRUE;
		PipeControl.DW1.ConstantCacheInvalidationEnable = TRUE;
		PipeControl.DW1.VFCacheInvalidationEnable = TRUE;
		PipeControl.DW1.InstructionCacheInvalidateEnable = TRUE;
		break;

	case GFX3DFLUSH_NONE:
	default:
		PipeControl.DW1.RenderTargetCacheFlushEnable = FALSE;
		break;
	}

	PipeControl.DW1.CSStall = TRUE;

	PipeControl.DW1.PostSyncOperation = ControlMode;
	PipeControl.DW1.DestinationAddressType = FALSE;
	PipeControl.DW3.ImmediateData = dwSyncWord;

	PipeControl.DW1.PIPE_CONTROLFlushEnable = TRUE;

	GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand
			    (pCmdBuffer, &PipeControl, sizeof(PipeControl)));

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendPipeControl_g8(PGENHW_HW_INTERFACE pHwInterface,
					   PGENOS_COMMAND_BUFFER pCmdBuffer,
					   PGENOS_RESOURCE pOsResource,
					   BOOL AllocationEnable,
					   DWORD AllocationOffset,
					   INT ControlMode,
					   INT FlushMode, DWORD dwSyncWord)
{
	PGENOS_INTERFACE pOsInterface;
	PIPE_CONTROL_CMD_G8 PipeControl;
	PGENHW_HW_COMMANDS pHwCommands;
	INT iAllocationIndex;
	UINT uiPatchOffset;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pOsResource);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);
	GENHW_HW_ASSERT(pOsResource->iAllocationIndex !=
			GENOS_INVALID_ALLOC_INDEX);

	eStatus = GENOS_STATUS_SUCCESS;
	pOsInterface = pHwInterface->pOsInterface;
	pHwCommands = pHwInterface->pHwCommands;
	iAllocationIndex =
	    pOsInterface->pfnGetResourceAllocationIndex(pOsInterface,
							pOsResource);

	uiPatchOffset = pCmdBuffer->iOffset + (2 * sizeof(DWORD));

	GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry(pOsInterface,
							   iAllocationIndex,
							   AllocationOffset,
							   uiPatchOffset));

	PipeControl = *(pHwCommands->pPipeControl_g8);

	switch (FlushMode) {
	case GFX3DFLUSH_WRITE_CACHE:
		PipeControl.DW1.RenderTargetCacheFlushEnable = TRUE;
		PipeControl.DW1.DCFlushEnable = TRUE;
		break;

	case GFX3DFLUSH_READ_CACHE:
		PipeControl.DW1.RenderTargetCacheFlushEnable = FALSE;
		PipeControl.DW1.StateCacheInvalidationEnable = TRUE;
		PipeControl.DW1.ConstantCacheInvalidationEnable = TRUE;
		PipeControl.DW1.VFCacheInvalidationEnable = TRUE;
		PipeControl.DW1.InstructionCacheInvalidateEnable = TRUE;
		break;

	case GFX3DFLUSH_NONE:
	default:
		PipeControl.DW1.RenderTargetCacheFlushEnable = FALSE;
		break;
	}

	PipeControl.DW1.CSStall = TRUE;

	PipeControl.DW1.PostSyncOperation = ControlMode;
	PipeControl.DW1.DestinationAddressType = FALSE;
	PipeControl.DW4.ImmediateData = dwSyncWord;

	PipeControl.DW1.PIPE_CONTROLFlushEnable = TRUE;

	GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand
			    (pCmdBuffer, &PipeControl, sizeof(PipeControl)));

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendStateBaseAddr_g75(PGENHW_HW_INTERFACE
					      pHwInterface,
					      PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	GENOS_STATUS eStatus;
	PGENHW_HW_COMMANDS pHwCommands;
	PGENOS_INTERFACE pOsInterface;
	PGENHW_GSH pGSH;
	INT iAllocationIndex;
	UINT uiPatchOffset;
	UINT uiIndirectStateBase, uiIndirectStateSize;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap);

	eStatus = GENOS_STATUS_SUCCESS;
	pHwCommands = pHwInterface->pHwCommands;
	pGSH = pHwInterface->pGeneralStateHeap;
	pOsInterface = pHwInterface->pOsInterface;
	iAllocationIndex =
	    pOsInterface->pfnGetResourceAllocationIndex(pOsInterface,
							&pGSH->OsResource);

	uiPatchOffset = pCmdBuffer->iOffset + (1 * sizeof(DWORD));
	GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry
			    (pOsInterface, iAllocationIndex, 1, uiPatchOffset));

	if (pOsInterface->bNoParsingAssistanceInKmd) {
		uiPatchOffset = pCmdBuffer->iOffset + (2 * sizeof(DWORD));
		GENHW_HW_CHK_STATUS(pOsInterface->pfnGetIndirectState
				    (pOsInterface, &uiIndirectStateBase,
				     &uiIndirectStateSize));
		GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry(pOsInterface,
								   pOsInterface->pfnGetResourceAllocationIndex
								   (pOsInterface,
								    &pCmdBuffer->
								    OsResource),
								   uiIndirectStateBase
								   + 1,
								   uiPatchOffset));
	}
	uiPatchOffset = pCmdBuffer->iOffset + (3 * sizeof(DWORD));
	GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry
			    (pOsInterface, iAllocationIndex, 1, uiPatchOffset));

	uiPatchOffset = pCmdBuffer->iOffset + (4 * sizeof(DWORD));
	GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry
			    (pOsInterface, iAllocationIndex, 1, uiPatchOffset));

	uiPatchOffset = pCmdBuffer->iOffset + (5 * sizeof(DWORD));
	GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry
			    (pOsInterface, iAllocationIndex, 1, uiPatchOffset));

	GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer, (PVOID)
						  pHwCommands->
						  pStateBaseAddress_g75,
						  sizeof
						  (STATE_BASE_ADDRESS_CMD_G75)));

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendStateBaseAddr_g8(PGENHW_HW_INTERFACE
					     pHwInterface,
					     PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	GENOS_STATUS eStatus;
	PGENHW_HW_COMMANDS pHwCommands;
	PGENOS_INTERFACE pOsInterface;
	PGENHW_GSH pGSH;
	INT iAllocationIndex;
	UINT uiPatchOffset;
	STATE_BASE_ADDRESS_CMD_G8 Cmd;
	UINT uiIndirectStateBase, uiIndirectStateSize;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap);

	eStatus = GENOS_STATUS_SUCCESS;
	pHwCommands = pHwInterface->pHwCommands;
	pGSH = pHwInterface->pGeneralStateHeap;
	pOsInterface = pHwInterface->pOsInterface;
	iAllocationIndex =
	    pOsInterface->pfnGetResourceAllocationIndex(pOsInterface,
							&pGSH->OsResource);

	Cmd = *pHwCommands->pStateBaseAddress_g8;

	Cmd.DW12.GeneralStateBufferSize =
	    Cmd.DW13.DynamicStateBufferSize =
	    Cmd.DW14.IndirectObjBufferSize =
	    Cmd.DW15.InstructionBufferSize =
	    GENOS_ALIGN_CEIL(pHwInterface->pGeneralStateHeap->dwGSHSize,
			     GENHW_PAGE_SIZE) / GENHW_PAGE_SIZE;

	uiPatchOffset = pCmdBuffer->iOffset + (1 * sizeof(DWORD));
	GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry
			    (pOsInterface, iAllocationIndex, 1, uiPatchOffset));

	if (pOsInterface->bNoParsingAssistanceInKmd) {
		uiPatchOffset = pCmdBuffer->iOffset + (4 * sizeof(DWORD));
		GENHW_HW_CHK_STATUS(pOsInterface->pfnGetIndirectState
				    (pOsInterface, &uiIndirectStateBase,
				     &uiIndirectStateSize));
		GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry(pOsInterface,
								   pOsInterface->pfnGetResourceAllocationIndex
								   (pOsInterface,
								    &pCmdBuffer->
								    OsResource),
								   uiIndirectStateBase
								   + 1,
								   uiPatchOffset));
	}
	uiPatchOffset = pCmdBuffer->iOffset + (6 * sizeof(DWORD));
	GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry
			    (pOsInterface, iAllocationIndex, 1, uiPatchOffset));

	uiPatchOffset = pCmdBuffer->iOffset + (8 * sizeof(DWORD));
	GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry
			    (pOsInterface, iAllocationIndex, 1, uiPatchOffset));

	uiPatchOffset = pCmdBuffer->iOffset + (10 * sizeof(DWORD));
	GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry
			    (pOsInterface, iAllocationIndex, 1, uiPatchOffset));

	GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer,
						  (PVOID) & Cmd,
						  sizeof
						  (STATE_BASE_ADDRESS_CMD_G8)));

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendBatchBufferEnd(PGENHW_HW_INTERFACE pHwInterface,
					   PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	MI_BATCH_BUFFER_END_CMD_G5 BatchBufferEndCmd;
	PGENHW_HW_COMMANDS pHwCommands;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);

	pHwCommands = pHwInterface->pHwCommands;

	BatchBufferEndCmd = *(pHwCommands->pBatchBufferEnd);

	GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer,
						  &BatchBufferEndCmd,
						  sizeof(BatchBufferEndCmd)));

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendBatchBufferStart_g75(PGENHW_HW_INTERFACE
						 pHwInterface,
						 PGENOS_COMMAND_BUFFER
						 pCmdBuffer,
						 PGENHW_BATCH_BUFFER
						 pBatchBuffer)
{
	GENOS_STATUS eStatus;
	MI_BATCH_BUFFER_START_CMD_G75 BatchBufferCmd;
	PGENHW_HW_COMMANDS pHwCommands;
	PGENOS_INTERFACE pOsInterface;
	INT iAllocationIndex;
	UINT uiPatchOffset;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pBatchBuffer);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);

	pHwCommands = pHwInterface->pHwCommands;
	pOsInterface = pHwInterface->pOsInterface;
	iAllocationIndex =
	    pOsInterface->pfnGetResourceAllocationIndex(pOsInterface,
							&pBatchBuffer->OsResource);

	uiPatchOffset = pCmdBuffer->iOffset + (1 * sizeof(DWORD));

	GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry(pOsInterface,
							   iAllocationIndex,
							   0, uiPatchOffset));

	BatchBufferCmd = *(pHwCommands->pBatchBufferStart_g75);

	BatchBufferCmd.DW0.SecondLevelBatchBuffer = TRUE;

	GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer,
						  &BatchBufferCmd,
						  sizeof(BatchBufferCmd)));

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendBatchBufferStart_g8(PGENHW_HW_INTERFACE
						pHwInterface,
						PGENOS_COMMAND_BUFFER
						pCmdBuffer,
						PGENHW_BATCH_BUFFER
						pBatchBuffer)
{
	GENOS_STATUS eStatus;
	MI_BATCH_BUFFER_START_CMD_G8 BatchBufferCmd;
	PGENHW_HW_COMMANDS pHwCommands;
	PGENOS_INTERFACE pOsInterface;
	INT iAllocationIndex;
	UINT uiPatchOffset;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pBatchBuffer);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);

	pHwCommands = pHwInterface->pHwCommands;
	pOsInterface = pHwInterface->pOsInterface;
	iAllocationIndex =
	    pOsInterface->pfnGetResourceAllocationIndex(pOsInterface,
							&pBatchBuffer->OsResource);

	uiPatchOffset = pCmdBuffer->iOffset + (1 * sizeof(DWORD));

	GENHW_HW_CHK_STATUS(pOsInterface->pfnSetPatchEntry(pOsInterface,
							   iAllocationIndex,
							   0, uiPatchOffset));

	BatchBufferCmd = *(pHwCommands->pBatchBufferStart_g8);

	BatchBufferCmd.DW0.SecondLevelBatchBuffer = TRUE;

	GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer,
						  &BatchBufferCmd,
						  sizeof(BatchBufferCmd)));

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwInitialize(PGENHW_HW_INTERFACE pHwInterface,
				   PCGENHW_SETTINGS pSettings)
{
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);

	if (pSettings) {
		pHwInterface->GshSettings.iMediaStateHeaps =
		    pSettings->iMediaStates;
	}
	pHwInterface->SshSettings.iSurfaceStateHeaps =
	    pHwInterface->GshSettings.iMediaStateHeaps;

	GENHW_HW_CHK_STATUS(pHwInterface->pfnAllocateCommands(pHwInterface));

	GENHW_HW_CHK_STATUS(pHwInterface->pfnAllocateGSH(pHwInterface,
							 &pHwInterface->
							 GshSettings));

	GENHW_HW_CHK_STATUS(pHwInterface->pfnAllocateSSH(pHwInterface,
							 &pHwInterface->
							 SshSettings));

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwInitInterface(PGENHW_HW_INTERFACE pHwInterface)
{
	GENOS_STATUS eStatus;

	eStatus = GENOS_STATUS_SUCCESS;

	pHwInterface->pfnInitialize = IntelGen_HwInitialize;
	pHwInterface->pfnResetHwStates = IntelGen_HwResetHwStates;
	pHwInterface->pfnAllocateSshBuffer = IntelGen_HwAllocateSshBuffer;
	pHwInterface->pfnFreeSshBuffer = IntelGen_HwFreeSshBuffer;
	pHwInterface->pfnAssignSshInstance = IntelGen_HwAssignSshInstance;
	pHwInterface->pfnBindSurfaceState = IntelGen_HwBindSurfaceState;
	pHwInterface->pfnSendCommandBufferHeader =
	    IntelGen_HwSendCommandBufferHeader;
	pHwInterface->pfnSendBatchBufferEnd = IntelGen_HwSendBatchBufferEnd;

	if (GFX_IS_RENDERCORE(pHwInterface->Platform, IGFX_GEN7_5_CORE)) {
		pHwInterface->pfnSendSyncTag = IntelGen_HwSendSyncTag_g75;
		pHwInterface->pfnSendStateBaseAddr =
		    IntelGen_HwSendStateBaseAddr_g75;
		pHwInterface->pfnSendSurfaces =
		    IntelGen_HwSendSurfaces_g75_PatchList;
		pHwInterface->pfnSendBatchBufferStart =
		    IntelGen_HwSendBatchBufferStart_g75;
		pHwInterface->pfnSendPipeControl =
		    IntelGen_HwSendPipeControl_g75;
		pHwInterface->pfnSendLoadRegImmCmd =
		    IntelGen_HwSendLoadRegImmCmd_g75;
		pHwInterface->pfnSetupSurfaceStateOs =
		    IntelGen_HwSetupSurfaceState_g75;

	} else if (GFX_IS_RENDERCORE(pHwInterface->Platform, IGFX_GEN8_CORE)) {
		pHwInterface->pfnSendSyncTag = IntelGen_HwSendSyncTag_g75;
		pHwInterface->pfnSendStateBaseAddr =
		    IntelGen_HwSendStateBaseAddr_g8;
		pHwInterface->pfnSendSurfaces = IntelGen_HwSendSurfaces_g8;
		pHwInterface->pfnSendBatchBufferStart =
		    IntelGen_HwSendBatchBufferStart_g8;
		pHwInterface->pfnSendPipeControl =
		    IntelGen_HwSendPipeControl_g8;
		pHwInterface->pfnSendLoadRegImmCmd =
		    IntelGen_HwSendLoadRegImmCmd_g75;
		pHwInterface->pfnSetupSurfaceStateOs =
		    IntelGen_HwSetupSurfaceState_g75;

	} else {
		eStatus = GENOS_STATUS_UNKNOWN;
	}

	return eStatus;
}
