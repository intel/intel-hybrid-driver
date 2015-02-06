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
#include "hw_interface_g8.h"

#include <math.h>

CONST GENHW_GSH_SETTINGS g_GSH_Settings_g8 = {
	GENHW_SYNC_SIZE_G75,
	GENHW_MEDIA_STATES_G75,
	GENHW_MEDIA_IDS_G75,
	GENHW_CURBE_SIZE_G8,
	GENHW_KERNEL_COUNT_G75,
	GENHW_KERNEL_HEAP_G75,
	GENHW_KERNEL_BLOCK_SIZE_G75,
	0,
	GENHW_MAX_SIP_SIZE
};

CONST GENHW_SSH_SETTINGS g_SSH_Settings_g8 = {
	GENHW_SSH_INSTANCES,
	GENHW_SSH_BINDING_TABLES,
	GENHW_SSH_SURFACE_STATES,
	GENHW_SSH_SURFACES_PER_BT,
	GENHW_SSH_BINDING_TABLE_ALIGN_G8
};

CONST GENHW_HW_CAPS g_IntelGen_HwCaps_g8_gt1 = {
	GENHW_SSH_SURFACES_PER_BT_MAX - 1,
	GENHW_MEDIA_THREADS_MAX_G8_GT1,
	512,
	GENHW_URB_SIZE_MAX_G75,
	GENHW_URB_ENTRIES_MAX_G75_GT1,
	GENHW_URB_ENTRY_SIZE_MAX_G75,
	GENHW_CURBE_SIZE_MAX_G75,
	GENHW_INTERFACE_DESCRIPTOR_ENTRIES_MAX_G75,
	GENHW_SUBSLICES_MAX_G8_GT1,
	GENHW_EU_INDEX_MAX_G8,
	GENHW_MEDIA_THREADS_PER_EU_MAX_G8,
	GENHW_SIZE_REGISTERS_PER_THREAD_G8
};

CONST GENHW_HW_CAPS g_IntelGen_HwCaps_g8_gt2 = {
	GENHW_SSH_SURFACES_PER_BT_MAX - 1,
	GENHW_MEDIA_THREADS_MAX_G8_GT2,
	512,
	GENHW_URB_SIZE_MAX_G75,
	GENHW_URB_ENTRIES_MAX_G75_GT2,
	GENHW_URB_ENTRY_SIZE_MAX_G75,
	GENHW_CURBE_SIZE_MAX_G75,
	GENHW_INTERFACE_DESCRIPTOR_ENTRIES_MAX_G75,
	GENHW_SUBSLICES_MAX_G8_GT2,
	GENHW_EU_INDEX_MAX_G8,
	GENHW_MEDIA_THREADS_PER_EU_MAX_G8,
	GENHW_SIZE_REGISTERS_PER_THREAD_G8
};

CONST GENHW_HW_CAPS g_IntelGen_HwCaps_g8_gt3 = {
	GENHW_SSH_SURFACES_PER_BT_MAX - 1,
	GENHW_MEDIA_THREADS_MAX_G8_GT3,
	512,
	GENHW_URB_SIZE_MAX_G75,
	GENHW_URB_ENTRIES_MAX_G75_GT3,
	GENHW_URB_ENTRY_SIZE_MAX_G75,
	GENHW_CURBE_SIZE_MAX_G75,
	GENHW_INTERFACE_DESCRIPTOR_ENTRIES_MAX_G75,
	GENHW_SUBSLICES_MAX_G8_GT3,
	GENHW_EU_INDEX_MAX_G8,
	GENHW_MEDIA_THREADS_PER_EU_MAX_G8,
	GENHW_SIZE_REGISTERS_PER_THREAD_G8
};

CONST GENHW_HW_CAPS g_IntelGen_HwCaps_g8_lcia = {
	GENHW_SSH_SURFACES_PER_BT_MAX - 1,
	GENHW_MEDIA_THREADS_MAX_G8_LCIA,
	512,
	GENHW_URB_SIZE_MAX_G75,
	GENHW_URB_ENTRIES_MAX_G75_GT3,
	GENHW_URB_ENTRY_SIZE_MAX_G75,
	GENHW_CURBE_SIZE_MAX_G75,
	GENHW_INTERFACE_DESCRIPTOR_ENTRIES_MAX_G75,
	GENHW_SUBSLICES_MAX_G8_LCIA,
	GENHW_EU_INDEX_MAX_G8,
	GENHW_MEDIA_THREADS_PER_EU_MAX_G8,
	GENHW_SIZE_REGISTERS_PER_THREAD_G8
};

DWORD IntelGen_GetScratchSpaceSize_g8(PGENHW_HW_INTERFACE pHwInterface,
				      DWORD iPerThreadScratchSpaceSize)
{
	DWORD dwScratchSpaceSize =
	    pHwInterface->pHwCaps->dwMaxThreads * iPerThreadScratchSpaceSize;

	return dwScratchSpaceSize;
}

VOID IntelGen_HwInitCommands_g8(PGENHW_HW_INTERFACE pHwInterface)
{
	PGENHW_HW_COMMANDS pHwCommands;

	pHwInterface->pfnInitCommandsCommon(pHwInterface);
	pHwCommands = pHwInterface->pHwCommands;

	pHwCommands->dwMediaObjectHeaderCmdSize =
	    sizeof(MEDIA_OBJECT_HEADER_G6);

	pHwCommands->pPipelineSelectMedia =
	    &g_cInit_PIPELINE_SELECT_CMD_MEDIA_G575;

	pHwCommands->pcPipeControlParam = &g_PipeControlParam_g75;
}

GENOS_STATUS IntelGen_HwAssignBindingTable_g8(PGENHW_HW_INTERFACE pHwInterface,
					      PINT piBindingTable)
{
	PGENHW_SSH pSSH;
	PGENHW_HW_COMMANDS pHwCommands;
	PBINDING_TABLE_STATE_G8 pBindingTableEntry;
	DWORD dwOffset;
	INT i;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(piBindingTable);
	GENHW_HW_ASSERT(pHwInterface->pSurfaceStateHeap);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);

	*piBindingTable = -1;
	pSSH = pHwInterface->pSurfaceStateHeap;
	pHwCommands = pHwInterface->pHwCommands;
	eStatus = GENOS_STATUS_UNKNOWN;

	if (pSSH->iCurrentBindingTable >=
	    pHwInterface->SshSettings.iBindingTables) {
		GENHW_HW_ASSERTMESSAGE
		    ("Unable to allocate Binding Table. Exceeds Maximum.");
		goto finish;
	}

	*piBindingTable = pSSH->iCurrentBindingTable;

	dwOffset = IntelGen_HwGetCurBindingTableBase(pSSH) +
	    (*piBindingTable * pSSH->iBindingTableSize);

	pBindingTableEntry =
	    (PBINDING_TABLE_STATE_G8) (pSSH->pSshBuffer + dwOffset);

	for (i = pHwInterface->SshSettings.iSurfacesPerBT; i > 0;
	     i--, pBindingTableEntry++) {
		*pBindingTableEntry = *(pHwCommands->pBindingTableState_g8);
	}

	++pSSH->iCurrentBindingTable;

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

DWORD IntelGen_HwGetSurfaceMemoryObjectControl_g8(PGENHW_HW_INTERFACE
						  pHwInterface,
						  PGENHW_SURFACE_STATE_PARAMS
						  pParams)
{
	return pParams->MemObjCtl;
}

GENOS_STATUS IntelGen_HwSetupBufferSurfaceState_g8(PGENHW_HW_INTERFACE
						   pHwInterface,
						   PGENHW_SURFACE pSurface,
						   PGENHW_SURFACE_STATE_PARAMS
						   pParams,
						   PGENHW_SURFACE_STATE_ENTRY *
						   ppSurfaceEntry)
{
	GENOS_STATUS eStatus;
	PGENHW_SSH pSSH;
	PSURFACE_STATE_G8 pState_g8;
	PGENHW_SURFACE_STATE_ENTRY pSurfaceEntry;
	BYTE iWidth;
	USHORT iHeight;
	USHORT iDepth;
	DWORD bufferSize;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pSurface);
	GENHW_HW_ASSERT(ppSurfaceEntry);
	GENHW_HW_ASSERT(pSurface->dwWidth > 0);

	eStatus = GENOS_STATUS_SUCCESS;
	pSSH = pHwInterface->pSurfaceStateHeap;

	pParams->Type = GENHW_SURFACE_TYPE_G8;

	GENHW_HW_CHK_STATUS(pHwInterface->pfnAssignSurfaceState(pHwInterface,
								GENHW_SURFACE_TYPE_G8,
								ppSurfaceEntry));

	GENHW_HW_ASSERT(*ppSurfaceEntry);
	pSurfaceEntry = *ppSurfaceEntry;

	switch (pSurface->Format) {
	case Format_Buffer:
		{
			pSurfaceEntry->dwFormat =
			    GFX3DSTATE_SURFACEFORMAT_L8_UNORM;
			break;
		}
	case Format_RAW:
		{
			pSurfaceEntry->dwFormat = GFX3DSTATE_SURFACEFORMAT_RAW;
			break;
		}
	default:
		{
			GENHW_HW_ASSERTMESSAGE
			    ("Invalid Buffer Surface Format.");
			break;
		}
	}

	pSurfaceEntry->dwSurfStateOffset =
	    IntelGen_HwGetCurSurfaceStateBase(pSSH) +
	    (pSurfaceEntry->iSurfStateID * sizeof(SURFACE_STATE_G8));

	bufferSize = pSurface->dwWidth - 1;

	iWidth = (BYTE) (bufferSize & GFX_MASK(0, 6));
	iHeight = (USHORT) ((bufferSize & GFX_MASK(7, 20)) >> 7);
	if (Format_RAW == pSurface->Format) {
		iDepth = (USHORT) ((bufferSize & GFX_MASK(21, 30)) >> 21);
	} else {
		iDepth = (USHORT) ((bufferSize & GFX_MASK(21, 26)) >> 21);
	}

	pState_g8 =
	    &pSurfaceEntry->pSurfaceState->PacketSurfaceState_g8.
	    cmdSurfaceState_g8;

	*pState_g8 = *(pHwInterface->pHwCommands->pSurfaceState_g8);

	pState_g8->DW0.SurfaceFormat = pSurfaceEntry->dwFormat;
	pState_g8->DW0.TileMode = 0;
	pState_g8->DW0.SurfaceType = GFX3DSTATE_SURFACETYPE_BUFFER;
	pState_g8->DW1.SurfaceMemObjCtrlState =
	    pHwInterface->pfnGetSurfaceMemoryObjectControl(pHwInterface,
							   pParams);
	pState_g8->DW2.Width = iWidth;
	pState_g8->DW2.Height = iHeight;
	pState_g8->DW3.Depth = iDepth;
	pState_g8->DW3.SurfacePitch = 0;
	pState_g8->DW8.SurfaceBaseAddress = 0;

	GENHW_HW_CHK_STATUS(pHwInterface->pfnSetupSurfaceStateOs(pHwInterface,
								 pSurface,
								 pParams,
								 pSurfaceEntry));

 finish:
	return eStatus;

}

GENOS_STATUS IntelGen_HwSetupSurfaceState_g8(PGENHW_HW_INTERFACE pHwInterface,
					     PGENHW_SURFACE pSurface,
					     PGENHW_SURFACE_STATE_PARAMS
					     pParams, PINT piNumEntries,
					     PGENHW_SURFACE_STATE_ENTRY *
					     ppSurfaceEntries)
{
	PGENHW_HW_COMMANDS pHwCommands;
	PGENHW_SSH pSSH;
	PSURFACE_STATE_G8 pSurfaceState_g8;
	PGENHW_SURFACE_STATE_ENTRY pSurfaceEntry;
	PGENHW_PLANE_OFFSET pPlaneOffset;
	INT i;
	DWORD dwPixelsPerSampleUV;
	UINT uSurfPitch;
	UINT uBytesPerPixelShift;
	UINT uYPlaneTopLvlIndexY;
	UINT uYPlane2ndLvlIndexY;
	UINT uYPlaneTopLvlIndexX;
	UINT uYPlane2ndLvlIndexX;
	UINT uUVPlaneTopLvlIndexY;
	UINT uUVPlane2ndLvlIndexY;
	UINT uUVPlaneTopLvlIndexX;
	UINT uUVPlane2ndLvlIndexX;
	RECT tempRect;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pSurface);
	GENHW_HW_ASSERT(pParams);
	GENHW_HW_ASSERT(ppSurfaceEntries);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);
	GENHW_HW_ASSERT(pHwInterface->pSurfaceStateHeap);

	eStatus = GENOS_STATUS_UNKNOWN;
	pSSH = pHwInterface->pSurfaceStateHeap;
	pHwCommands = pHwInterface->pHwCommands;
	uSurfPitch = 0;

	uYPlaneTopLvlIndexY = uYPlaneTopLvlIndexX = 0;
	uYPlane2ndLvlIndexY = uYPlane2ndLvlIndexX = 0;
	uUVPlaneTopLvlIndexY = uUVPlaneTopLvlIndexX = 0;
	uUVPlane2ndLvlIndexY = uUVPlane2ndLvlIndexX = 0;

	if (pParams->b32MWColorFillKernWA == TRUE) {
		tempRect = pSurface->rcDst;
		uSurfPitch = pSurface->dwPitch;

		pSurface->YPlaneOffset.iXOffset = tempRect.left;
		pSurface->YPlaneOffset.iYOffset = tempRect.top;

		pSurface->rcDst.left =
		    tempRect.left & (GENHW_MACROBLOCK_SIZE - 1);
		pSurface->rcDst.top =
		    tempRect.top & (GENHW_MACROBLOCK_SIZE - 1);

		pSurface->dwWidth = pSurface->rcDst.right =
		    tempRect.right - (tempRect.left & ~0xf);
		pSurface->dwHeight = pSurface->rcDst.bottom =
		    tempRect.bottom - (tempRect.top & ~0xf);

		switch (pSurface->Format) {
		case Format_A8B8G8R8:
		case Format_X8B8G8R8:
		case Format_A8R8G8B8:
		case Format_X8R8G8B8:
			uBytesPerPixelShift = 2;
			break;
		case Format_YUY2:
		case Format_YUYV:
		case Format_YVYU:
		case Format_UYVY:
		case Format_VYUY:
			uBytesPerPixelShift = 1;
			break;
		case Format_NV12:
			uBytesPerPixelShift = 0;
			break;
		default:
			uBytesPerPixelShift = 0;
			break;
		}

		uYPlaneTopLvlIndexY = tempRect.top >> GENHW_YTILE_H_SHIFTBITS;

		uYPlane2ndLvlIndexY =
		    (tempRect.top & (GENHW_YTILE_H_ALIGNMENT -
				     1)) & ~(GENHW_MACROBLOCK_SIZE - 1);

		uYPlaneTopLvlIndexX =
		    tempRect.left >> (GENHW_YTILE_W_SHIFTBITS -
				      uBytesPerPixelShift);

		uYPlane2ndLvlIndexX = ((tempRect.left &
					((GENHW_YTILE_W_ALIGNMENT >>
					  uBytesPerPixelShift) -
					 1)) & ~(GENHW_MACROBLOCK_SIZE -
						 1)) >> (2 -
							 uBytesPerPixelShift);

		if (pSurface->Format == Format_NV12) {
			uUVPlaneTopLvlIndexY =
			    tempRect.top >> (GENHW_YTILE_H_SHIFTBITS + 1);
			uUVPlane2ndLvlIndexY =
			    ((tempRect.top & ((GENHW_YTILE_H_ALIGNMENT << 1) -
					      1)) & ~(GENHW_MACROBLOCK_SIZE -
						      1)) >> 1;
			uUVPlaneTopLvlIndexX = uYPlaneTopLvlIndexX;
			uUVPlane2ndLvlIndexX = uYPlane2ndLvlIndexX;
		}
	}

	GENHW_HW_CHK_STATUS(pHwInterface->pfnGetSurfaceStateEntries
			    (pHwInterface, pSurface, pParams, piNumEntries,
			     ppSurfaceEntries));

#define GET_SURFACE_STATE_G8_TILEMODE(_pSurfaceEntry)   ((_pSurfaceEntry->bTiledSurface) ? ((_pSurfaceEntry->bTileWalk == 0) ? 2 /*x-tile*/: 3 /*y-tile*/) : 0 /*linear*/ )

	for (i = 0; i < *piNumEntries; i++) {
		pSurfaceEntry = ppSurfaceEntries[i];

		pSurfaceEntry->dwSurfStateOffset =
		    IntelGen_HwGetCurSurfaceStateBase(pSSH) +
		    (pSurfaceEntry->iSurfStateID * sizeof(SURFACE_STATE_G8));

		if (pSurfaceEntry->bAVS) {

		} else {
			pSurfaceState_g8 =
			    &pSurfaceEntry->pSurfaceState->
			    PacketSurfaceState_g8.cmdSurfaceState_g8;

			*pSurfaceState_g8 = *(pHwCommands->pSurfaceState_g8);

			if (pSurfaceEntry->YUVPlane == GENHW_U_PLANE ||
			    pSurfaceEntry->YUVPlane == GENHW_V_PLANE) {
				pPlaneOffset =
				    (pSurfaceEntry->YUVPlane ==
				     GENHW_U_PLANE) ? &pSurface->UPlaneOffset :
				    &pSurface->VPlaneOffset;

				if (pParams->b32MWColorFillKernWA == TRUE) {
					pPlaneOffset->iSurfaceOffset +=
					    uUVPlaneTopLvlIndexY *
					    (uSurfPitch >>
					     GENHW_YTILE_W_SHIFTBITS) *
					    GENHW_PAGE_SIZE;

					pPlaneOffset->iSurfaceOffset +=
					    uUVPlaneTopLvlIndexX *
					    GENHW_PAGE_SIZE;

					pSurfaceState_g8->DW5.YOffset =
					    uUVPlane2ndLvlIndexY >> 2;
				} else {
					pSurfaceState_g8->DW5.YOffset =
					    pPlaneOffset->iYOffset >> 2;
				}

				if (pParams->bWidthInDword_UV) {
					IntelGen_GetPixelsPerSample
					    (pSurface->Format,
					     &dwPixelsPerSampleUV);
				} else {
					dwPixelsPerSampleUV = 1;
				}

				if (dwPixelsPerSampleUV == 1) {
					pSurfaceState_g8->DW5.XOffset =
					    pPlaneOffset->iXOffset >> 2;
				} else {
					if (pParams->b32MWColorFillKernWA ==
					    TRUE) {
						pSurfaceState_g8->DW5.XOffset =
						    uUVPlane2ndLvlIndexX >> 2;
					} else {
						pSurfaceState_g8->DW5.XOffset =
						    (pPlaneOffset->iXOffset /
						     (DWORD) sizeof(DWORD)) >>
						    2;
					}
				}
			} else {
				if (pParams->b32MWColorFillKernWA == TRUE) {
					pSurface->dwOffset +=
					    uYPlaneTopLvlIndexY * (uSurfPitch /
								   GENHW_YTILE_W_ALIGNMENT)
					    * GENHW_PAGE_SIZE;

					pSurface->dwOffset +=
					    uYPlaneTopLvlIndexX *
					    GENHW_PAGE_SIZE;

					pSurfaceState_g8->DW5.XOffset =
					    uYPlane2ndLvlIndexX >> 2;

					pSurfaceState_g8->DW5.YOffset =
					    uYPlane2ndLvlIndexY >> 2;
				} else {
					pSurfaceState_g8->DW5.XOffset =
					    (pSurface->YPlaneOffset.iXOffset /
					     (DWORD) sizeof(DWORD)) >> 2;
					pSurfaceState_g8->DW5.YOffset =
					    pSurface->YPlaneOffset.
					    iYOffset >> 2;
				}

				if ((pSurfaceEntry->YUVPlane == GENHW_Y_PLANE)
				    && (pSurfaceEntry->dwFormat ==
					GFX3DSTATE_SURFACEFORMAT_PLANAR_420_8))
				{
					pSurfaceState_g8->DW6.XOffsetUVPlane =
					    0;
					pSurfaceState_g8->DW6.YOffsetUVPlane =
					    (WORD) pSurface->dwHeight;
				}
			}

			pSurfaceState_g8->DW0.SurfaceType =
			    (pSurface->dwDepth >
			     1) ? GFX3DSTATE_SURFACETYPE_3D :
			    GFX3DSTATE_SURFACETYPE_2D;

			pSurfaceState_g8->DW0.VerticalLineStrideOffset =
			    pSurfaceEntry->bVertStrideOffs;
			pSurfaceState_g8->DW0.VerticalLineStride =
			    pSurfaceEntry->bVertStride;
			pSurfaceState_g8->DW0.TileMode =
			    GET_SURFACE_STATE_G8_TILEMODE(pSurfaceEntry);
			pSurfaceState_g8->DW0.SurfaceFormat =
			    pSurfaceEntry->dwFormat;
			pSurfaceState_g8->DW1.SurfaceMemObjCtrlState =
			    pHwInterface->pfnGetSurfaceMemoryObjectControl
			    (pHwInterface, pParams);

			if (pParams->bWAUseSrcWidth) {
				pSurfaceState_g8->DW2.Width =
				    pSurface->rcSrc.right - 1;
			} else {
				pSurfaceState_g8->DW2.Width =
				    pSurfaceEntry->dwWidth - 1;
			}

			if (pParams->bWAUseSrcHeight) {
				pSurfaceState_g8->DW2.Height =
				    pSurface->rcSrc.bottom - 1;
			} else {
				pSurfaceState_g8->DW2.Height =
				    pSurfaceEntry->dwHeight - 1;
			}
			pSurfaceState_g8->DW3.SurfacePitch =
			    pSurfaceEntry->dwPitch - 1;
			pSurfaceState_g8->DW3.Depth =
			    MAX(1, pSurface->dwDepth) - 1;
			pSurfaceState_g8->DW8.SurfaceBaseAddress = 0;
			pSurfaceState_g8->DW9.SurfaceBaseAddress64 = 0;
		}

		GENHW_HW_CHK_STATUS(pHwInterface->pfnSetupSurfaceStateOs
				    (pHwInterface, pSurface, pParams,
				     pSurfaceEntry));
	}

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

INT IntelGen_HwLoadCurbeData_g8(PGENHW_HW_INTERFACE pHwInterface,
				PGENHW_MEDIA_STATE pCurMediaState,
				PVOID pData, INT iSize)
{
	INT iOffset;
	INT iCurbeSize;
	PBYTE pPtrCurbe;
	PGENHW_GSH pGSH;

	iOffset = -1;
	pGSH = (pHwInterface) ? pHwInterface->pGeneralStateHeap : NULL;
	if (pGSH && pCurMediaState) {
		iCurbeSize =
		    GENOS_ALIGN_CEIL(iSize, GENHW_CURBE_BLOCK_ALIGN_G8);
		if (pCurMediaState->iCurbeOffset + iCurbeSize <=
		    (int)pGSH->dwSizeCurbe) {
			iOffset = pCurMediaState->iCurbeOffset;
			pCurMediaState->iCurbeOffset += iCurbeSize;

			if (pData) {
				pPtrCurbe = pGSH->pGSH +
				    pGSH->pCurMediaState->dwOffset +
				    pGSH->dwOffsetCurbe + iOffset;

				GENOS_SecureMemcpy(pPtrCurbe, iSize, pData,
						   iSize);

				iCurbeSize -= iSize;
				if (iCurbeSize > 0) {
					GENOS_ZeroMemory(pPtrCurbe + iSize,
							 iCurbeSize);
				}
			}
		}
	}

	return iOffset;
}

GENOS_STATUS IntelGen_HwSendVfeState_g8(PGENHW_HW_INTERFACE pHwInterface,
					PGENOS_COMMAND_BUFFER pCmdBuffer,
					BOOL blGpGpuWalkerMode)
{
	PMEDIA_VFE_STATE_CMD_G8 pVideoFrontEnd;
	PGENHW_HW_COMMANDS pHwCommands;
	PGENHW_GSH pGSH;
	PCGENHW_HW_CAPS pHwCaps;
	DWORD dwMaxURBSize;
	DWORD dwCURBEAllocationSize;
	DWORD dwActualCURBESizeNeeded;
	DWORD dwURBEntryAllocationSize;
	DWORD dwNumberofURBEntries;
	DWORD dwMaxInterfaceDescriptorEntries;
	GENOS_STATUS eStatus;
	INT iSize;
	DWORD dwCmdSize;
	INT iRemain;
	INT iPerThreadScratchSize;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);
	GENHW_HW_ASSERT(pHwInterface->pWaTable);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap);

	eStatus = GENOS_STATUS_SUCCESS;
	pHwCommands = pHwInterface->pHwCommands;
	pGSH = pHwInterface->pGeneralStateHeap;
	pHwCaps = pHwInterface->pHwCaps;
	dwCmdSize = sizeof(MEDIA_VFE_STATE_CMD_G8);

	pVideoFrontEnd =
	    (PMEDIA_VFE_STATE_CMD_G8) IntelGen_OsGetCmdBufferSpace(pCmdBuffer,
								   dwCmdSize);
	GENHW_HW_CHK_NULL(pVideoFrontEnd);

	*pVideoFrontEnd = *(pHwCommands->pVideoFrontEnd_g8);

	if (pHwInterface->GshSettings.iPerThreadScratchSize > 0) {
		GENHW_HW_ASSERT(pHwInterface->GshSettings.
				iPerThreadScratchSize ==
				GENOS_ALIGN_CEIL(pHwInterface->GshSettings.
						 iPerThreadScratchSize, 1024));

		iPerThreadScratchSize =
		    pHwInterface->GshSettings.iPerThreadScratchSize >> 10;
		iRemain = iPerThreadScratchSize % 2;
		iPerThreadScratchSize = iPerThreadScratchSize / 2;
		iSize = 0;

		while (!iRemain && (iPerThreadScratchSize / 2)) {
			iSize++;
			iRemain = iPerThreadScratchSize % 2;
			iPerThreadScratchSize = iPerThreadScratchSize / 2;
		}

		GENHW_HW_ASSERT(!iRemain && iPerThreadScratchSize);
		GENHW_HW_ASSERT(iSize < 12);

		pVideoFrontEnd->DW1.PerThreadScratchSpace = iSize;

		pVideoFrontEnd->DW1.ScratchSpaceBasePointer =
		    pGSH->dwScratchSpaceBase >> 10;
		pVideoFrontEnd->DW2.ScratchSpaceBasePointer64 = 0;
	}

	dwMaxURBSize = pHwCaps->dwMaxURBSize;
	dwMaxInterfaceDescriptorEntries =
	    pHwCaps->dwMaxInterfaceDescriptorEntries;

	dwActualCURBESizeNeeded =
	    GFX_MAX(pHwInterface->VfeStateParams.dwCURBEAllocationSize,
		    (DWORD) pGSH->pCurMediaState->iCurbeOffset);

	dwCURBEAllocationSize = GENOS_ROUNDUP_SHIFT(dwActualCURBESizeNeeded, 5);

	dwURBEntryAllocationSize =
	    GENOS_ROUNDUP_SHIFT(pHwInterface->VfeStateParams.
				dwURBEntryAllocationSize, 5);
	dwURBEntryAllocationSize = GFX_MAX(1, dwURBEntryAllocationSize);

	dwNumberofURBEntries =
	    (dwMaxURBSize - dwCURBEAllocationSize -
	     dwMaxInterfaceDescriptorEntries) / dwURBEntryAllocationSize;
	dwNumberofURBEntries = GFX_CLAMP_MIN_MAX(dwNumberofURBEntries, 1, 32);

	pVideoFrontEnd->DW3.DebugCounterControl =
	    pHwInterface->VfeStateParams.dwDebugCounterControl;
	pVideoFrontEnd->DW3.NumberofURBEntries = dwNumberofURBEntries;
	pVideoFrontEnd->DW3.MaximumNumberofThreads =
	    pHwInterface->VfeStateParams.dwMaximumNumberofThreads - 1;
	pVideoFrontEnd->DW5.CURBEAllocationSize = dwCURBEAllocationSize;
	pVideoFrontEnd->DW5.URBEntryAllocationSize = dwURBEntryAllocationSize;

	if (pHwInterface->VfeScoreboard.ScoreboardEnable) {
		pVideoFrontEnd->DW6.ScoreboardEnable = 1;
		pVideoFrontEnd->DW6.ScoreboardMask =
		    pHwInterface->VfeScoreboard.ScoreboardMask;
		pVideoFrontEnd->DW6.ScoreboardType =
		    pHwInterface->VfeScoreboard.ScoreboardType;
		pVideoFrontEnd->DW7.Value =
		    pHwInterface->VfeScoreboard.Value[0];
		pVideoFrontEnd->DW8.Value =
		    pHwInterface->VfeScoreboard.Value[1];
	}

	if (blGpGpuWalkerMode) {
		pVideoFrontEnd->DW3.GPGPUMode = MEDIASTATE_GPGPU_MODE;
		pVideoFrontEnd->DW3.BypassGatewayControl = 1;
	}

	GENHW_HW_ASSERT(pVideoFrontEnd->DW3.NumberofURBEntries <=
			pHwCaps->dwMaxURBEntries);
	GENHW_HW_ASSERT(pVideoFrontEnd->DW5.CURBEAllocationSize <=
			pHwCaps->dwMaxCURBEAllocationSize);
	GENHW_HW_ASSERT(pVideoFrontEnd->DW5.URBEntryAllocationSize <=
			pHwCaps->dwMaxURBEntryAllocationSize);
	GENHW_HW_ASSERT(pVideoFrontEnd->DW3.NumberofURBEntries *
			pVideoFrontEnd->DW5.URBEntryAllocationSize +
			pVideoFrontEnd->DW5.CURBEAllocationSize +
			dwMaxInterfaceDescriptorEntries <= dwMaxURBSize);

	IntelGen_OsAdjustCmdBufferFreeSpace(pCmdBuffer, dwCmdSize);

 finish:
	return eStatus;
}

VOID IntelGen_HwInitInterfaceDescriptor_g8(PGENHW_HW_INTERFACE pHwInterface,
					   PBYTE pBase,
					   DWORD dwBase, DWORD dwOffsetID)
{
	PINTERFACE_DESCRIPTOR_DATA_G8 pInterfaceDescriptor;

	pInterfaceDescriptor =
	    (INTERFACE_DESCRIPTOR_DATA_G8 *) (pBase + dwOffsetID);
	*pInterfaceDescriptor =
	    *(pHwInterface->pHwCommands->pInterfaceDescriptor_g8);
}

VOID IntelGen_HwSetupInterfaceDescriptor_g8(PGENHW_HW_INTERFACE pHwInterface,
					    PGENHW_MEDIA_STATE pMediaState,
					    PGENHW_KRN_ALLOCATION
					    pKernelAllocation,
					    PGENHW_INTERFACE_DESCRIPTOR_PARAMS
					    pInterfaceDescriptorParams,
					    PGENHW_GPGPU_WALKER_PARAMS
					    pGpGpuWalkerParams)
{
	PGENHW_GSH pGSH;
	PGENHW_SSH pSSH;
	PINTERFACE_DESCRIPTOR_DATA_G8 pInterfaceDescriptor;
	DWORD dwBTOffset;

	pGSH = pHwInterface->pGeneralStateHeap;
	pSSH = pHwInterface->pSurfaceStateHeap;

	dwBTOffset = IntelGen_HwGetCurBindingTableBase(pSSH) +
	    (pInterfaceDescriptorParams->iBindingTableID *
	     pSSH->iBindingTableSize);

	pInterfaceDescriptor = (PINTERFACE_DESCRIPTOR_DATA_G8)
	    (pGSH->pGSH +
	     pMediaState->dwOffset +
	     pGSH->dwOffsetMediaID +
	     (pInterfaceDescriptorParams->iMediaID * pGSH->dwSizeMediaID));

	pInterfaceDescriptor->DW0.KernelStartPointer =
	    pKernelAllocation->dwOffset >> 6;
	pInterfaceDescriptor->DW4.BindingTablePointer = dwBTOffset >> 5;
	pInterfaceDescriptor->DW5.ConstantURBEntryReadOffset =
	    pInterfaceDescriptorParams->iCurbeOffset >> 5;
	pInterfaceDescriptor->DW5.ConstantURBEntryReadLength =
	    pInterfaceDescriptorParams->iCurbeLength >> 5;
	pInterfaceDescriptor->DW7.CrsThdConDataRdLn =
	    pInterfaceDescriptorParams->iCrsThrdConstDataLn >> 5;

	if (pGpGpuWalkerParams && pGpGpuWalkerParams->GpGpuEnable) {
		pInterfaceDescriptor->DW6.BarrierEnable = 1;
		pInterfaceDescriptor->DW6.NumberofThreadsInGPGPUGroup =
		    pGpGpuWalkerParams->ThreadWidth *
		    pGpGpuWalkerParams->ThreadHeight;
		pInterfaceDescriptor->DW6.SharedLocalMemorySize =
		    GENOS_ALIGN_CEIL(pGpGpuWalkerParams->SLMSize, 4) >> 2;
	}
}

GENOS_STATUS IntelGen_HwSendGpGpuWalkerState_g8(PGENHW_HW_INTERFACE
						pHwInterface,
						PGENOS_COMMAND_BUFFER
						pCmdBuffer,
						PGENHW_GPGPU_WALKER_PARAMS
						pGpGpuWalkerParams)
{
	GENOS_STATUS eStatus;
	PGPGPU_WALKER_CMD_G8 pGpuWalker;
	DWORD dwCmdSize;

	eStatus = GENOS_STATUS_SUCCESS;
	dwCmdSize = sizeof(GPGPU_WALKER_CMD_G8);

	pGpuWalker =
	    (PGPGPU_WALKER_CMD_G8) IntelGen_OsGetCmdBufferSpace(pCmdBuffer,
								dwCmdSize);
	GENHW_HW_CHK_NULL(pGpuWalker);

	*pGpuWalker = *(pHwInterface->pHwCommands->pGpGpuWalker_g8);
	pGpuWalker->DW1.InterfaceDescriptorOffset =
	    pGpGpuWalkerParams->InterfaceDescriptorOffset;
	pGpuWalker->DW4.SIMDSize = 2;
	pGpuWalker->DW4.ThreadWidthCounterMax =
	    pGpGpuWalkerParams->ThreadWidth - 1;
	pGpuWalker->DW4.ThreadHeightCounterMax =
	    pGpGpuWalkerParams->ThreadHeight - 1;
	pGpuWalker->DW4.ThreadDepthCounterMax = 0;
	pGpuWalker->DW5.ThreadGroupIDStartingX = 0;
	pGpuWalker->DW7.ThreadGroupIDDimensionX =
	    pGpGpuWalkerParams->GroupWidth;
	pGpuWalker->DW8.ThreadGroupIDStartingY = 0;
	pGpuWalker->DW10.ThreadGroupIDDimensionY =
	    pGpGpuWalkerParams->GroupHeight;
	pGpuWalker->DW12.ThreadGroupIDDimensionZ = 1;
	pGpuWalker->DW13.RightExecutionMask = 0xffffffff;
	pGpuWalker->DW14.BottomExecutionMask = 0xffffffff;

	IntelGen_OsAdjustCmdBufferFreeSpace(pCmdBuffer, dwCmdSize);

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendWalkerState_g8(PGENHW_HW_INTERFACE pHwInterface,
					   PGENOS_COMMAND_BUFFER pCmdBuffer,
					   PGENHW_WALKER_PARAMS pWalkerParams)
{
	PMEDIA_OBJECT_WALKER_CMD_G6 pMediaObjWalker;
	GENOS_STATUS eStatus;
	DWORD dwCmdSize;

	eStatus = GENOS_STATUS_SUCCESS;
	dwCmdSize = sizeof(MEDIA_OBJECT_WALKER_CMD_G6);

	pMediaObjWalker = (PMEDIA_OBJECT_WALKER_CMD_G6)
	    IntelGen_OsGetCmdBufferSpace(pCmdBuffer, dwCmdSize);
	GENHW_HW_CHK_NULL(pMediaObjWalker);

	*pMediaObjWalker = *(pHwInterface->pHwCommands->pMediaWalker_g75);
	pMediaObjWalker->DW0.Length =
	    OP_LENGTH(SIZE32(MEDIA_OBJECT_WALKER_CMD_G6)) +
	    pWalkerParams->InlineDataLength / sizeof(DWORD);
	pMediaObjWalker->DW1.InterfaceDescriptorOffset =
	    pWalkerParams->InterfaceDescriptorOffset;
	pMediaObjWalker->DW2.UseScoreboard =
	    pHwInterface->VfeScoreboard.ScoreboardEnable;
	pMediaObjWalker->DW5.ScoreboardMask =
	    pHwInterface->VfeScoreboard.ScoreboardMask;

	pMediaObjWalker->DW6.ColorCountMinusOne =
	    pWalkerParams->ColorCountMinusOne;

	pMediaObjWalker->DW6.MidLoopUnitX = pWalkerParams->MidLoopUnitX;
	pMediaObjWalker->DW6.MidLoopUnitY = pWalkerParams->MidLoopUnitY;
	pMediaObjWalker->DW6.MidLoopExtraSteps =
	    pWalkerParams->MiddleLoopExtraSteps;

	pMediaObjWalker->DW7.Value = pWalkerParams->LoopExecCount.value;
	pMediaObjWalker->DW8.Value = pWalkerParams->BlockResolution.value;
	pMediaObjWalker->DW9.Value = pWalkerParams->LocalStart.value;
	pMediaObjWalker->DW10.Value = pWalkerParams->LocalEnd.value;
	pMediaObjWalker->DW11.Value = pWalkerParams->LocalOutLoopStride.value;
	pMediaObjWalker->DW12.Value = pWalkerParams->LocalInnerLoopUnit.value;
	pMediaObjWalker->DW13.Value = pWalkerParams->GlobalResolution.value;
	pMediaObjWalker->DW14.Value = pWalkerParams->GlobalStart.value;
	pMediaObjWalker->DW15.Value =
	    pWalkerParams->GlobalOutlerLoopStride.value;
	pMediaObjWalker->DW16.Value = pWalkerParams->GlobalInnerLoopUnit.value;

	IntelGen_OsAdjustCmdBufferFreeSpace(pCmdBuffer, dwCmdSize);

	if (pWalkerParams->InlineDataLength) {
		GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer,
							  pWalkerParams->pInlineData,
							  pWalkerParams->
							  InlineDataLength));
	}
 finish:
	return eStatus;
}

VOID IntelGen_HwInitInterface_g8(PGENHW_HW_INTERFACE pHwInterface)
{
	if (GFX_IS_PRODUCT(pHwInterface->Platform, IGFX_CHERRYVIEW)) {
		pHwInterface->pHwCaps = &g_IntelGen_HwCaps_g8_lcia;
	} else if (pHwInterface->Platform.GtType == GTTYPE_GT1) {
		pHwInterface->pHwCaps = &g_IntelGen_HwCaps_g8_gt1;
	} else if (pHwInterface->Platform.GtType == GTTYPE_GT2) {
		pHwInterface->pHwCaps = &g_IntelGen_HwCaps_g8_gt2;
	} else {
		pHwInterface->pHwCaps = &g_IntelGen_HwCaps_g8_gt3;
	}

	pHwInterface->GshSettings = g_GSH_Settings_g8;

	pHwInterface->SshSettings = g_SSH_Settings_g8;

	pHwInterface->SurfaceTypeDefault = GENHW_SURFACE_TYPE_G8;
	pHwInterface->bUsesPatchList = TRUE;

	pHwInterface->iSizeBindingTableState = sizeof(BINDING_TABLE_STATE_G8);
	pHwInterface->iSizeInstructionCache = GENHW_INSTRUCTION_CACHE_G75;
	pHwInterface->iSizeInterfaceDescriptor =
	    sizeof(INTERFACE_DESCRIPTOR_DATA_G8);
	pHwInterface->bEnableYV12SinglePass = FALSE;

	pHwInterface->VfeStateParams.dwDebugCounterControl = 0;
	pHwInterface->VfeStateParams.dwMaximumNumberofThreads =
	    pHwInterface->pHwCaps->dwMaxThreads;

	pHwInterface->pfnInitCommands = IntelGen_HwInitCommands_g8;
	pHwInterface->pfnAssignBindingTable = IntelGen_HwAssignBindingTable_g8;
	pHwInterface->pfnSetupSurfaceState = IntelGen_HwSetupSurfaceState_g8;
	pHwInterface->pfnSetupBufferSurfaceState =
	    IntelGen_HwSetupBufferSurfaceState_g8;
	pHwInterface->pfnLoadCurbeData = IntelGen_HwLoadCurbeData_g8;
	pHwInterface->pfnInitInterfaceDescriptor =
	    IntelGen_HwInitInterfaceDescriptor_g8;
	pHwInterface->pfnSetupInterfaceDescriptor =
	    IntelGen_HwSetupInterfaceDescriptor_g8;
	pHwInterface->pfnGetMediaWalkerStatus =
	    IntelGen_HwGetMediaWalkerStatus_g75;
	pHwInterface->pfnGetMediaWalkerBlockSize =
	    IntelGen_HwGetMediaWalkerBlockSize_g75;
	pHwInterface->pfnSendVfeState = IntelGen_HwSendVfeState_g8;
	pHwInterface->pfnSendCurbeLoad = IntelGen_HwSendCurbeLoad_g75;
	pHwInterface->pfnSendIDLoad = IntelGen_HwSendIDLoad_g75;
	pHwInterface->pfnAddMediaObjectCmdBb =
	    IntelGen_HwAddMediaObjectCmdBb_g75;
	pHwInterface->pfnSendMediaStateFlush =
	    IntelGen_HwSendMediaStateFlush_g75;
	pHwInterface->pfnGetSurfaceMemoryObjectControl =
	    IntelGen_HwGetSurfaceMemoryObjectControl_g8;
	pHwInterface->pfnSendMediaObjectWalker = IntelGen_HwSendWalkerState_g8;
	pHwInterface->pfnSendPipelineSelectCmd =
	    IntelGen_HwSendPipelineSelectCmd_g75;
	pHwInterface->pfnGetScratchSpaceSize = IntelGen_GetScratchSpaceSize_g8;
	pHwInterface->pfnSendMISetPredicateCmd =
	    IntelGen_HwSendMISetPredicateCmd_g75;
	pHwInterface->pfnSendGpGpuWalkerState =
	    IntelGen_HwSendGpGpuWalkerState_g8;
	pHwInterface->pfnConvertToNanoSeconds =
	    IntelGen_HwConvertToNanoSeconds_g75;
	pHwInterface->pfnSkipPipeControlCmdBb =
	    IntelGen_HwSkipPipeControlCmdBb_g75;
	pHwInterface->pfnAddPipeControlCmdBb =
	    IntelGen_HwAddPipeControlCmdBb_g75;

	pHwInterface->pfnIs2PlaneNV12Needed = IntelGen_HwIs2PlaneNV12Needed_g75;
}
