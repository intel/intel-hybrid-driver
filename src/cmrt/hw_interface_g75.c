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

#include "hw_interface_g75.h"
#include <math.h>

#define GENHW_NS_PER_TICK_RENDER_G75        80

CONST GENHW_SSH_SETTINGS g_SSH_Settings_g75 = {
	GENHW_SSH_INSTANCES,
	GENHW_SSH_BINDING_TABLES,
	GENHW_SSH_SURFACE_STATES,
	GENHW_SSH_SURFACES_PER_BT,
	GENHW_SSH_BINDING_TABLE_ALIGN
};

CONST GENHW_PIPECONTROL_PARAM g_PipeControlParam_g75 = {
	NULL,
	0,
	GFX3DCONTROLOP_NOWRITE,
	0,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	TRUE,
	FALSE,
	FALSE
};

CONST GENHW_INDIRECT_PATCH_PARAM g_IndirectPatchParam_g75 = {
	CMD_INDIRECT_INVALID,
	NULL,
	0,
	NULL,
	0
};

extern CONST GENHW_SURFACE_PLANES
    g_cInitSurfacePlanes_g75[GENHW_PLANES_DEFINITION_COUNT] = {
	// GENHW_PLANES_PL3
	{3,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_U_PLANE, 2, 2, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_V_PLANE, 2, 2, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 },
	// GENHW_PLANES_NV12
	{1,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_PLANAR_420_8}
	  }
	 },
	// GENHW_PLANES_YUY2
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_YCRCB_NORMAL}
	  }
	 },
	// GENHW_PLANES_UYVY
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_YCRCB_SWAPY}
	  }
	 },
	// GENHW_PLANES_YVYU
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_YCRCB_SWAPUV}
	  }
	 },
	// GENHW_PLANES_VYUY
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_YCRCB_SWAPUVY}
	  }
	 },
	// GENHW_PLANES_ARGB
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_B8G8R8A8_UNORM}
	  }
	 },
	// GENHW_PLANES_XRGB
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_B8G8R8X8_UNORM}
	  }
	 },
	// GENHW_PLANES_ABGR
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R8G8B8A8_UNORM}
	  }
	 },
	// GENHW_PLANES_XBGR
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R8G8B8X8_UNORM}
	  }
	 },
	// GENHW_PLANES_RGB16
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_B5G6R5_UNORM}
	  }
	 },
	// GENHW_PLANES_R16U
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_R16_UINT}
	  }
	 },
	// GENHW_PLANES_R16S
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_R16_SINT}
	  }
	 },
	// GENHW_PLANES_R32U
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R32_UINT}
	  }
	 },
	// GENHW_PLANES_R32S
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R32_SINT}
	  }
	 },
	// GENHW_PLANES_R32F
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R32_FLOAT}
	  }
	 },
	// GENHW_PLANES_V8U8
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_R8G8_SNORM}
	  }
	 },
	// GENHW_PLANES_R8G8_UNORM
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_R8G8_UNORM}
	  }
	 },
	// GENHW_PLANES_411P
	{3,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_U_PLANE, 4, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_V_PLANE, 4, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 },
	// GENHW_PLANES_411R
	{3,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_U_PLANE, 1, 4, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_V_PLANE, 1, 4, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 },
	// GENHW_PLANES_422H
	{3,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_U_PLANE, 2, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_V_PLANE, 2, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 },
	// GENHW_PLANES_422V
	{3,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_U_PLANE, 1, 2, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_V_PLANE, 1, 2, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 },
	// GENHW_PLANES_444P
	{3,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_U_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_V_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 },
	// GENHW_PLANES_RGBP
	{3,
	 {
	  {GENHW_U_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_V_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 },
	// GENHW_PLANES_BGRP
	{3,
	 {
	  {GENHW_U_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_V_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 },
	// GENHW_PLANES_AI44_PALLETE_0
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_P4A4_UNORM_PALETTE_0}
	  }
	 },
	// GENHW_PLANES_IA44_PALLETE_0
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_A4P4_UNORM_PALETTE_0}
	  }
	 },
	// GENHW_PLANES_P8_PALLETE_0
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_P8_UNORM_PALETTE_0}
	  }
	 },
	// GENHW_PLANES_A8P8_PALLETE_0
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_P8A8_UNORM_PALETTE_0}
	  }
	 },
	// GENHW_PLANES_AI44_PALLETE_1
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_P4A4_UNORM_PALETTE_1}
	  }
	 },
	// GENHW_PLANES_IA44_PALLETE_1
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_A4P4_UNORM_PALETTE_1}
	  }
	 },
	// GENHW_PLANES_P8_PALLETE_1
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_P8_UNORM_PALETTE_1}
	  }
	 },
	// GENHW_PLANES_A8P8_PALLETE_1
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_P8A8_UNORM_PALETTE_1}
	  }
	 },
	// GENHW_PLANES_AYUV
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R8B8G8A8_UNORM}
	  }
	 },
	// GENHW_PLANES_STMM
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_L8_UNORM}
	  }
	 },
	// GENHW_PLANES_L8
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_L8_UNORM}
	  }
	 },
	// GENHW_PLANES_A8
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_A8_UNORM}
	  }
	 },
	// GENHW_PLANES_R8
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 },
	// GENHW_PLANES_NV12_2PLANES
	{2,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_U_PLANE, 2, 2, 1, 1, 2, 0, GFX3DSTATE_SURFACEFORMAT_R8G8_UNORM}
	  }
	 },
	// GENHW_PLANES_R16_UNORM
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_R16_UNORM}
	  }
	 },
	// GENHW_PLANES_A16B16G16R16
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 0, 0,
	   GFX3DSTATE_SURFACEFORMAT_R16G16B16A16_UNORM}
	  }
	 },
	// GENHW_PLANES_R10G10B10A2
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R10G10B10A2_UNORM}
	  }
	 },
	// GENHW_PLANES_L16
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_L16_UNORM}
	  }
	 },
	// GENHW_PLANES_NV21
	{2,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM},
	  {GENHW_U_PLANE, 2, 2, 1, 1, 2, 0, GFX3DSTATE_SURFACEFORMAT_R8G8_UNORM}
	  }
	 },
	// GENHW_PLANES_YV12
	{1,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_PLANAR_420_8}
	  }
	 },
	// GENHW_PLANES_P016
	{2,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 2, 0, GFX3DSTATE_SURFACEFORMAT_R16_UNORM},
	  {GENHW_U_PLANE, 2, 2, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R16G16_UNORM}
	  }
	 },
	// GENHW_PLANES_P010
	{2,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 2, 0, GFX3DSTATE_SURFACEFORMAT_R16_UNORM},
	  {GENHW_U_PLANE, 2, 2, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R16G16_UNORM}
	  }
	 }
};

CONST GENHW_HW_CAPS g_IntelGen_HwCaps_g75_gt1 = {
	GENHW_SSH_SURFACES_PER_BT_MAX - 1,
	GENHW_MEDIA_THREADS_MAX_G75_GT1,
	512,
	GENHW_URB_SIZE_MAX_G75,
	GENHW_URB_ENTRIES_MAX_G75_GT1,
	GENHW_URB_ENTRY_SIZE_MAX_G75,
	GENHW_CURBE_SIZE_MAX_G75,
	GENHW_INTERFACE_DESCRIPTOR_ENTRIES_MAX_G75,
	GENHW_SUBSLICES_MAX_G75_GT1,
	GENHW_EU_INDEX_MAX_G75,
	GENHW_MEDIA_THREADS_PER_EU_MAX_G75,
	GENHW_SIZE_REGISTERS_PER_THREAD_G75
};

CONST GENHW_HW_CAPS g_IntelGen_HwCaps_g75_gt2 = {
	GENHW_SSH_SURFACES_PER_BT_MAX - 1,
	GENHW_MEDIA_THREADS_MAX_G75_GT2,
	512,
	GENHW_URB_SIZE_MAX_G75,
	GENHW_URB_ENTRIES_MAX_G75_GT2,
	GENHW_URB_ENTRY_SIZE_MAX_G75,
	GENHW_CURBE_SIZE_MAX_G75,
	GENHW_INTERFACE_DESCRIPTOR_ENTRIES_MAX_G75,
	GENHW_SUBSLICES_MAX_G75_GT2,
	GENHW_EU_INDEX_MAX_G75,
	GENHW_MEDIA_THREADS_PER_EU_MAX_G75,
	GENHW_SIZE_REGISTERS_PER_THREAD_G75
};

CONST GENHW_HW_CAPS g_IntelGen_HwCaps_g75_gt3 = {
	GENHW_SSH_SURFACES_PER_BT_MAX - 1,
	GENHW_MEDIA_THREADS_MAX_G75_GT3,
	512,
	GENHW_URB_SIZE_MAX_G75,
	GENHW_URB_ENTRIES_MAX_G75_GT2,
	GENHW_URB_ENTRY_SIZE_MAX_G75,
	GENHW_CURBE_SIZE_MAX_G75,
	GENHW_INTERFACE_DESCRIPTOR_ENTRIES_MAX_G75,
	GENHW_SUBSLICES_MAX_G75_GT3,
	GENHW_EU_INDEX_MAX_G75,
	GENHW_MEDIA_THREADS_PER_EU_MAX_G75,
	GENHW_SIZE_REGISTERS_PER_THREAD_G75
};

extern CONST GENHW_GSH_SETTINGS g_GSH_Settings_g75 = {
	GENHW_SYNC_SIZE_G75,
	GENHW_MEDIA_STATES_G75,
	GENHW_MEDIA_IDS_G75,
	GENHW_CURBE_SIZE_G75,
	GENHW_KERNEL_COUNT_G75,
	GENHW_KERNEL_HEAP_G75,
	GENHW_KERNEL_BLOCK_SIZE_G75,
	0,
	GENHW_MAX_SIP_SIZE
};

DWORD IntelGen_HwGetSurfaceMemoryObjectControl_g75(PGENHW_HW_INTERFACE
						   pHwInterface,
						   PGENHW_SURFACE_STATE_PARAMS
						   pParams)
{
	return pParams->MemObjCtl;
}

GENOS_STATUS IntelGen_HwAssignBindingTable_g75(PGENHW_HW_INTERFACE pHwInterface,
					       PINT piBindingTable)
{
	PGENHW_SSH pSSH;
	PGENHW_HW_COMMANDS pHwCommands;
	PBINDING_TABLE_STATE_G5 pBindingTableEntry;
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
	    (PBINDING_TABLE_STATE_G5) (pSSH->pSshBuffer + dwOffset);

	for (i = pHwInterface->SshSettings.iSurfacesPerBT; i > 0;
	     i--, pBindingTableEntry++) {
		*pBindingTableEntry = *(pHwCommands->pBindingTableState_g75);
	}

	++pSSH->iCurrentBindingTable;

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSetupSurfaceState_g75(PGENHW_HW_INTERFACE pHwInterface,
					      PGENHW_SURFACE pSurface,
					      PGENHW_SURFACE_STATE_PARAMS
					      pParams, PINT piNumEntries,
					      PGENHW_SURFACE_STATE_ENTRY *
					      ppSurfaceEntries)
{
	PGENHW_HW_COMMANDS pHwCommands;
	PGENHW_SSH pSSH;
	PSURFACE_STATE_G7 pSurfaceState_g75;
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

	for (i = 0; i < *piNumEntries; i++) {
		pSurfaceEntry = ppSurfaceEntries[i];

		pSurfaceEntry->dwSurfStateOffset =
		    IntelGen_HwGetCurSurfaceStateBase(pSSH) +
		    (pSurfaceEntry->iSurfStateID * sizeof(SURFACE_STATE_G7));

		pSurfaceState_g75 =
		    &pSurfaceEntry->pSurfaceState->
		    PacketSurfaceState_g75.cmdSurfaceState_g75;

		*pSurfaceState_g75 = *(pHwCommands->pSurfaceState_g75);

		if (pSurfaceEntry->YUVPlane == GENHW_U_PLANE ||
		    pSurfaceEntry->YUVPlane == GENHW_V_PLANE) {
			pPlaneOffset =
			    (pSurfaceEntry->YUVPlane == GENHW_U_PLANE)
			    ? &pSurface->UPlaneOffset : &pSurface->VPlaneOffset;

			if (pParams->b32MWColorFillKernWA == TRUE) {
				pPlaneOffset->iSurfaceOffset +=
				    uUVPlaneTopLvlIndexY *
				    (uSurfPitch >>
				     GENHW_YTILE_W_SHIFTBITS) * GENHW_PAGE_SIZE;

				pPlaneOffset->iSurfaceOffset +=
				    uUVPlaneTopLvlIndexX * GENHW_PAGE_SIZE;

				pSurfaceState_g75->DW5.YOffset =
				    uUVPlane2ndLvlIndexY >> 1;
			} else {
				pSurfaceState_g75->DW5.YOffset =
				    pPlaneOffset->iYOffset >> 1;
			}

			if (pParams->bWidthInDword_UV) {
				IntelGen_GetPixelsPerSample
				    (pSurface->Format, &dwPixelsPerSampleUV);
			} else {
				dwPixelsPerSampleUV = 1;
			}

			if (dwPixelsPerSampleUV == 1) {
				pSurfaceState_g75->DW5.XOffset =
				    pPlaneOffset->iXOffset >> 2;
			} else {
				if (pParams->b32MWColorFillKernWA == TRUE) {
					pSurfaceState_g75->DW5.XOffset =
					    uUVPlane2ndLvlIndexX >> 2;
				} else {
					pSurfaceState_g75->DW5.XOffset =
					    (pPlaneOffset->iXOffset /
					     (DWORD) sizeof(DWORD)) >> 2;
				}
			}
		} else {
			if (pParams->b32MWColorFillKernWA == TRUE) {
				pSurface->dwOffset +=
				    uYPlaneTopLvlIndexY * (uSurfPitch /
							   GENHW_YTILE_W_ALIGNMENT)
				    * GENHW_PAGE_SIZE;

				pSurface->dwOffset +=
				    uYPlaneTopLvlIndexX * GENHW_PAGE_SIZE;

				pSurfaceState_g75->DW5.XOffset =
				    uYPlane2ndLvlIndexX >> 2;

				pSurfaceState_g75->DW5.YOffset =
				    uYPlane2ndLvlIndexY >> 1;
			} else {
				pSurfaceState_g75->DW5.XOffset =
				    (pSurface->YPlaneOffset.iXOffset /
				     (DWORD) sizeof(DWORD)) >> 2;
				pSurfaceState_g75->DW5.YOffset =
				    pSurface->YPlaneOffset.iYOffset >> 1;
			}

			if (pSurfaceEntry->dwFormat ==
			    GFX3DSTATE_SURFACEFORMAT_PLANAR_420_8) {
				pSurfaceState_g75->DW6.XOffsetUVPlane = 0;
				pSurfaceState_g75->DW6.YOffsetUVPlane =
				    (WORD) pSurface->dwHeight;
			}
		}

		pSurfaceState_g75->DW0.SurfaceType =
		    (pSurface->dwDepth >
		     1) ? GFX3DSTATE_SURFACETYPE_3D : GFX3DSTATE_SURFACETYPE_2D;

		pSurfaceState_g75->DW0.VerticalLineStrideOffset =
		    pSurfaceEntry->bVertStrideOffs;
		pSurfaceState_g75->DW0.VerticalLineStride =
		    pSurfaceEntry->bVertStride;
		pSurfaceState_g75->DW0.TileWalk = pSurfaceEntry->bTileWalk;
		pSurfaceState_g75->DW0.TiledSurface =
		    pSurfaceEntry->bTiledSurface;
		pSurfaceState_g75->DW0.SurfaceFormat = pSurfaceEntry->dwFormat;

		pSurfaceState_g75->DW1.SurfaceBaseAddress = 0;
		pSurfaceState_g75->DW2.Width = pSurfaceEntry->dwWidth - 1;
		pSurfaceState_g75->DW2.Height = pSurfaceEntry->dwHeight - 1;
		pSurfaceState_g75->DW3.SurfacePitch =
		    pSurfaceEntry->dwPitch - 1;
		pSurfaceState_g75->DW3.Depth = MAX(1, pSurface->dwDepth) - 1;
		pSurfaceState_g75->DW5.SurfaceObjectControlState =
		    pHwInterface->pfnGetSurfaceMemoryObjectControl
		    (pHwInterface, pParams);

		GENHW_HW_CHK_STATUS(pHwInterface->pfnSetupSurfaceStateOs
				    (pHwInterface, pSurface, pParams,
				     pSurfaceEntry));
	}

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSetupBufferSurfaceState_g75(PGENHW_HW_INTERFACE
						    pHwInterface,
						    PGENHW_SURFACE pSurface,
						    PGENHW_SURFACE_STATE_PARAMS
						    pParams,
						    PGENHW_SURFACE_STATE_ENTRY *
						    ppSurfaceEntry)
{
	GENOS_STATUS eStatus;
	PGENHW_SSH pSSH;
	PSURFACE_STATE_G7 pState_g75;
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

	pParams->Type = GENHW_SURFACE_TYPE_G7;

	GENHW_HW_CHK_STATUS(pHwInterface->pfnAssignSurfaceState(pHwInterface,
								GENHW_SURFACE_TYPE_G7,
								ppSurfaceEntry));

	pSurfaceEntry = *ppSurfaceEntry;
	GENHW_HW_ASSERT(pSurfaceEntry);

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
	    (pSurfaceEntry->iSurfStateID * sizeof(SURFACE_STATE_G7));

	bufferSize = pSurface->dwWidth - 1;

	iWidth = (BYTE) (bufferSize & GFX_MASK(0, 6));
	iHeight = (USHORT) ((bufferSize & GFX_MASK(7, 20)) >> 7);
	if (Format_RAW == pSurface->Format) {
		iDepth = (USHORT) ((bufferSize & GFX_MASK(21, 30)) >> 21);
	} else {
		iDepth = (USHORT) ((bufferSize & GFX_MASK(21, 26)) >> 21);
	}

	pState_g75 =
	    &pSurfaceEntry->pSurfaceState->PacketSurfaceState_g75.
	    cmdSurfaceState_g75;

	*pState_g75 = *(pHwInterface->pHwCommands->pSurfaceState_g75);

	pState_g75->DW0.SurfaceFormat = pSurfaceEntry->dwFormat;
	pState_g75->DW0.TileWalk = 0;
	pState_g75->DW0.TiledSurface = FALSE;
	pState_g75->DW0.SurfaceType = GFX3DSTATE_SURFACETYPE_BUFFER;
	pState_g75->DW1.SurfaceBaseAddress = 0;
	pState_g75->DW2.Width = iWidth;
	pState_g75->DW2.Height = iHeight;
	pState_g75->DW3.Depth = iDepth;
	pState_g75->DW3.SurfacePitch = 0;
	pState_g75->DW5.SurfaceObjectControlState =
	    pHwInterface->pfnGetSurfaceMemoryObjectControl(pHwInterface,
							   pParams);

	GENHW_HW_CHK_STATUS(pHwInterface->pfnSetupSurfaceStateOs(pHwInterface,
								 pSurface,
								 pParams,
								 pSurfaceEntry));

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendPipelineSelectCmd_g75(PGENHW_HW_INTERFACE
						  pHwInterface,
						  PGENOS_COMMAND_BUFFER
						  pCmdBuffer,
						  DWORD dwGfxPipelineSelect)
{
	GENOS_STATUS eStatus;
	PPIPELINE_SELECT_CMD_G5 pPipelineSelectCmd;
	DWORD dwCmdSize;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);

	eStatus = GENOS_STATUS_SUCCESS;
	dwCmdSize = sizeof(PIPELINE_SELECT_CMD_G5);

	pPipelineSelectCmd =
	    (PPIPELINE_SELECT_CMD_G5) IntelGen_OsGetCmdBufferSpace(pCmdBuffer,
								   dwCmdSize);
	GENHW_HW_CHK_NULL(pPipelineSelectCmd);

	*pPipelineSelectCmd =
	    *(pHwInterface->pHwCommands->pPipelineSelectMedia);
	pPipelineSelectCmd->DW0.PipelineSelect = dwGfxPipelineSelect;

	IntelGen_OsAdjustCmdBufferFreeSpace(pCmdBuffer, dwCmdSize);

 finish:
	return eStatus;
}

VOID IntelGen_HwSkipPipeControlCmdBb_g75(PGENHW_HW_INTERFACE pHwInterface,
					 PGENHW_BATCH_BUFFER pBatchBuffer,
					 PGENHW_PIPECONTROL_PARAM pParam)
{
	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pBatchBuffer);
	GENHW_HW_ASSERT(pParam);
	GENHW_HW_ASSERT((pBatchBuffer->iSize - pBatchBuffer->iCurrent) >=
			sizeof(PIPE_CONTROL_CMD_G7));

	pBatchBuffer->iCurrent += sizeof(PIPE_CONTROL_CMD_G7);
}

INT IntelGen_HwLoadCurbeData_g75(PGENHW_HW_INTERFACE pHwInterface,
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
		    GENOS_ALIGN_CEIL(iSize, GENHW_CURBE_BLOCK_ALIGN_G7);
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

VOID IntelGen_HwInitInterfaceDescriptor_g75(PGENHW_HW_INTERFACE pHwInterface,
					    PBYTE pBase,
					    DWORD dwBase, DWORD dwOffsetID)
{
	PINTERFACE_DESCRIPTOR_DATA_G6 pInterfaceDescriptor;

	pInterfaceDescriptor =
	    (INTERFACE_DESCRIPTOR_DATA_G6 *) (pBase + dwOffsetID);
	*pInterfaceDescriptor =
	    *(pHwInterface->pHwCommands->pInterfaceDescriptor_g75);
}

VOID IntelGen_HwSetupInterfaceDescriptor_g75(PGENHW_HW_INTERFACE pHwInterface,
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
	PINTERFACE_DESCRIPTOR_DATA_G6 pInterfaceDescriptor;
	DWORD dwBTOffset;

	pGSH = pHwInterface->pGeneralStateHeap;
	pSSH = pHwInterface->pSurfaceStateHeap;

	dwBTOffset = IntelGen_HwGetCurBindingTableBase(pSSH) +
	    (pInterfaceDescriptorParams->iBindingTableID *
	     pSSH->iBindingTableSize);

	pInterfaceDescriptor = (PINTERFACE_DESCRIPTOR_DATA_G6)
	    (pGSH->pGSH +
	     pMediaState->dwOffset +
	     pGSH->dwOffsetMediaID +
	     (pInterfaceDescriptorParams->iMediaID * pGSH->dwSizeMediaID));

	pInterfaceDescriptor->DW0.KernelStartPointer =
	    pKernelAllocation->dwOffset >> 6;
	pInterfaceDescriptor->DW3.BindingTablePointer = dwBTOffset >> 5;
	pInterfaceDescriptor->DW4.ConstantURBEntryReadOffset =
	    pInterfaceDescriptorParams->iCurbeOffset >> 5;
	pInterfaceDescriptor->DW4.ConstantURBEntryReadLength =
	    pInterfaceDescriptorParams->iCurbeLength >> 5;

	if (pGpGpuWalkerParams && pGpGpuWalkerParams->GpGpuEnable) {
		pInterfaceDescriptor->DW5.BarrierEnable = 1;
		pInterfaceDescriptor->DW5.NumberofThreadsInGPGPUGroup =
		    pGpGpuWalkerParams->ThreadWidth *
		    pGpGpuWalkerParams->ThreadHeight;
		pInterfaceDescriptor->DW5.SharedLocalMemorySize =
		    GENOS_ALIGN_CEIL(pGpGpuWalkerParams->SLMSize, 4) >> 2;
		pInterfaceDescriptor->DW6.CrsThdConDataRdLn =
		    pInterfaceDescriptorParams->iCrsThrdConstDataLn >> 5;
	} else {
		pInterfaceDescriptor->DW5.Value = 0;
		pInterfaceDescriptor->DW6.Value = 0;
	}
}

GENOS_STATUS IntelGen_HwSendCurbeLoad_g75(PGENHW_HW_INTERFACE pHwInterface,
					  PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	PMEDIA_CURBE_LOAD_CMD_G6 pMediaCurbeLoad;
	PGENHW_HW_COMMANDS pHwCommands;
	PGENHW_GSH pGSH;
	GENOS_STATUS eStatus;
	DWORD dwCmdSize;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap->pCurMediaState);

	eStatus = GENOS_STATUS_SUCCESS;
	dwCmdSize = sizeof(MEDIA_CURBE_LOAD_CMD_G6);
	pHwCommands = pHwInterface->pHwCommands;
	pGSH = pHwInterface->pGeneralStateHeap;

	pMediaCurbeLoad =
	    (PMEDIA_CURBE_LOAD_CMD_G6) IntelGen_OsGetCmdBufferSpace(pCmdBuffer,
								    dwCmdSize);
	GENHW_HW_CHK_NULL(pMediaCurbeLoad);

	*pMediaCurbeLoad = *(pHwCommands->pMediaCurbeLoad_g75);

	if (pGSH->pCurMediaState->iCurbeOffset != 0) {
		pMediaCurbeLoad->DW2.CURBETotalDataLength =
		    pGSH->pCurMediaState->iCurbeOffset;
	} else {
		goto finish;
	}

	pMediaCurbeLoad->DW3.CURBEDataStartAddress =
	    pGSH->pCurMediaState->dwOffset + pGSH->dwOffsetCurbe;

	IntelGen_OsAdjustCmdBufferFreeSpace(pCmdBuffer, dwCmdSize);

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendIDLoad_g75(PGENHW_HW_INTERFACE pHwInterface,
				       PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	PMEDIA_INTERFACE_DESCRIPTOR_LOAD_CMD_G6 pMediaIDLoad;
	PGENHW_HW_COMMANDS pHwCommands;
	PGENHW_GSH pGSH;
	GENOS_STATUS eStatus;
	DWORD dwCmdSize;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap->pCurMediaState);

	eStatus = GENOS_STATUS_SUCCESS;
	dwCmdSize = sizeof(MEDIA_INTERFACE_DESCRIPTOR_LOAD_CMD_G6);
	pHwCommands = pHwInterface->pHwCommands;
	pGSH = pHwInterface->pGeneralStateHeap;

	pMediaIDLoad = (PMEDIA_INTERFACE_DESCRIPTOR_LOAD_CMD_G6)
	    IntelGen_OsGetCmdBufferSpace(pCmdBuffer, dwCmdSize);
	GENHW_HW_CHK_NULL(pMediaIDLoad);

	*pMediaIDLoad = *(pHwCommands->pMediaIDLoad_g75);

	pMediaIDLoad->DW2.InterfaceDescriptorLength =
	    pHwInterface->GshSettings.iMediaIDs * pGSH->dwSizeMediaID;

	pMediaIDLoad->DW3.InterfaceDescriptorStartAddress =
	    pGSH->pCurMediaState->dwOffset + pGSH->dwOffsetMediaID;

	IntelGen_OsAdjustCmdBufferFreeSpace(pCmdBuffer, dwCmdSize);

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendGpGpuWalkerState_g75(PGENHW_HW_INTERFACE
						 pHwInterface,
						 PGENOS_COMMAND_BUFFER
						 pCmdBuffer,
						 PGENHW_GPGPU_WALKER_PARAMS
						 pGpGpuWalkerParams)
{
	GENOS_STATUS eStatus;
	PGPGPU_WALKER_CMD_G75 pGpuWalker;
	DWORD dwCmdSize;

	eStatus = GENOS_STATUS_SUCCESS;
	dwCmdSize = sizeof(GPGPU_WALKER_CMD_G75);

	pGpuWalker =
	    (PGPGPU_WALKER_CMD_G75) IntelGen_OsGetCmdBufferSpace(pCmdBuffer,
								 dwCmdSize);
	GENHW_HW_CHK_NULL(pGpuWalker);

	*pGpuWalker = *(pHwInterface->pHwCommands->pGpGpuWalker_g75);

	pGpuWalker->DW1.InterfaceDescriptorOffset =
	    pGpGpuWalkerParams->InterfaceDescriptorOffset;
	pGpuWalker->DW2.SIMDSize = 2;
	pGpuWalker->DW2.ThreadWidthCounterMax =
	    pGpGpuWalkerParams->ThreadWidth - 1;
	pGpuWalker->DW2.ThreadHeightCounterMax =
	    pGpGpuWalkerParams->ThreadHeight - 1;
	pGpuWalker->DW2.ThreadDepthCounterMax = 0;
	pGpuWalker->DW3.ThreadGroupIDStartingX = 0;
	pGpuWalker->DW4.ThreadGroupIDDimensionX =
	    pGpGpuWalkerParams->GroupWidth;
	pGpuWalker->DW5.ThreadGroupIDStartingY = 0;
	pGpuWalker->DW6.ThreadGroupIDDimensionY =
	    pGpGpuWalkerParams->GroupHeight;
	pGpuWalker->DW7.ThreadGroupIDStartingZ = 0;
	pGpuWalker->DW8.ThreadGroupIDDimensionZ = 1;
	pGpuWalker->DW9.RightExecutionMask = 0xffffffff;
	pGpuWalker->DW10.BottomExecutionMask = 0xffffffff;

	IntelGen_OsAdjustCmdBufferFreeSpace(pCmdBuffer, dwCmdSize);

 finish:
	return eStatus;
}

VOID IntelGen_HwAddMediaObjectCmdBb_g75(PGENHW_HW_INTERFACE pHwInterface,
					PGENHW_BATCH_BUFFER pBatchBuffer,
					PGENHW_HW_MEDIAOBJECT_PARAM pParam)
{
	PBYTE pBuffer;
	PMEDIA_OBJECT_HEADER_G6 pCmd;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pBatchBuffer);
	GENHW_HW_ASSERT(pBatchBuffer->pData);
	GENHW_HW_ASSERT(pParam);
	GENHW_HW_ASSERT(pParam->dwMediaObjectSize >=
			sizeof(MEDIA_OBJECT_HEADER_G6));
	GENHW_HW_ASSERT((pBatchBuffer->iSize - pBatchBuffer->iCurrent) >=
			sizeof(MEDIA_OBJECT_HEADER_G6));

	pBuffer = pBatchBuffer->pData + pBatchBuffer->iCurrent;
	pCmd = (PMEDIA_OBJECT_HEADER_G6) pBuffer;
	*pCmd = g_cInit_MEDIA_OBJECT_HEADER_G6;

	pCmd->DW0.DWordLength =
	    OP_LENGTH(SIZE_IN_DW(pParam->dwMediaObjectSize));
	pCmd->DW1.InterfaceDescriptorOffset = pParam->dwIDOffset;

	pBatchBuffer->iCurrent += sizeof(MEDIA_OBJECT_HEADER_G6);
}

VOID IntelGen_HwConvertToNanoSeconds_g75(PGENHW_HW_INTERFACE pHwInterface,
					 UINT64 iTicks, PUINT64 piNs)
{
	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(piNs);

	*piNs = iTicks * GENHW_NS_PER_TICK_RENDER_G75;
}

VOID IntelGen_HwInitCommands_g75(PGENHW_HW_INTERFACE pHwInterface)
{
	PGENHW_HW_COMMANDS pHwCommands;

	pHwInterface->pfnInitCommandsCommon(pHwInterface);
	pHwCommands = pHwInterface->pHwCommands;

	pHwCommands->dwMediaObjectHeaderCmdSize =
	    sizeof(MEDIA_OBJECT_HEADER_G6);

	pHwCommands->pPipelineSelectMedia =
	    &g_cInit_PIPELINE_SELECT_CMD_MEDIA_G575;

	pHwCommands->pcPipeControlParam = &g_PipeControlParam_g75;
	pHwCommands->pcPatchParam = &g_IndirectPatchParam_g75;
}

BOOL IntelGen_HwGetMediaWalkerStatus_g75(PGENHW_HW_INTERFACE pHwInterface)
{
	if (pHwInterface->MediaWalkerMode == GENHW_MEDIA_WALKER_DISABLED) {
		return FALSE;
	}

	return TRUE;
}

UINT IntelGen_HwGetMediaWalkerBlockSize_g75(PGENHW_HW_INTERFACE pHwInterface)
{
	return 32;
}

VOID IntelGen_HwAddPipeControlCmdBb_g75(PGENHW_HW_INTERFACE pHwInterface,
					PGENHW_BATCH_BUFFER pBatchBuffer,
					PGENHW_PIPECONTROL_PARAM pParam)
{
	PBYTE pBuffer;
	PPIPE_CONTROL_CMD_G7 pCmd;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pBatchBuffer);
	GENHW_HW_ASSERT(pParam);
	GENHW_HW_ASSERT((pBatchBuffer->iSize - pBatchBuffer->iCurrent) >=
			sizeof(PIPE_CONTROL_CMD_G7));

	pBuffer = pBatchBuffer->pData + pBatchBuffer->iCurrent;
	pCmd = (PPIPE_CONTROL_CMD_G7) pBuffer;
	*pCmd = g_cInit_PIPE_CONTROL_CMD_G7;

	pCmd->DW1.StateCacheInvalidationEnable = pParam->dwInvalidateStateCache;
	pCmd->DW1.ConstantCacheInvalidationEnable =
	    pParam->dwInvaliateConstantCache;
	pCmd->DW1.VFCacheInvalidationEnable = pParam->dwInvalidateVFECache;
	pCmd->DW1.InstructionCacheInvalidateEnable =
	    pParam->dwInvalidateInstructionCache;
	pCmd->DW1.RenderTargetCacheFlushEnable =
	    pParam->dwFlushRenderTargetCache;
	pCmd->DW1.TLBInvalidate = pParam->dwTlbInvalidate;
	pCmd->DW1.PostSyncOperation = pParam->Operation;
	pCmd->DW1.CSStall = pParam->dwCSStall;
	pCmd->DW3.ImmediateData = pParam->dwImmData;
	pCmd->DW1.DestinationAddressType = 0;

	pCmd->DW1.DCFlushEnable = TRUE;
	pCmd->DW1.PIPE_CONTROLFlushEnable = TRUE;

	pBatchBuffer->iCurrent += sizeof(PIPE_CONTROL_CMD_G7);
}

GENOS_STATUS IntelGen_HwSendMISetPredicateCmd_g75(PGENHW_HW_INTERFACE
						  pHwInterface,
						  PGENOS_COMMAND_BUFFER
						  pCmdBuffer,
						  DWORD PredicateEnable)
{
	GENOS_STATUS eStatus;
	PMI_SET_PREDICATE_CMD_G75 pMiSetPredicate;
	DWORD dwCmdSize;

	eStatus = GENOS_STATUS_SUCCESS;
	dwCmdSize = sizeof(MI_SET_PREDICATE_CMD_G75);

	pMiSetPredicate =
	    (PMI_SET_PREDICATE_CMD_G75) IntelGen_OsGetCmdBufferSpace(pCmdBuffer,
								     dwCmdSize);
	GENHW_HW_CHK_NULL(pMiSetPredicate);

	*pMiSetPredicate = g_cInit_MI_SET_PREDICATE_CMD_G75;
	pMiSetPredicate->DW0.Enable = PredicateEnable;

	IntelGen_OsAdjustCmdBufferFreeSpace(pCmdBuffer, dwCmdSize);

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendMIArbCheck_g75(PGENHW_HW_INTERFACE pHwInterface,
					   PGENOS_COMMAND_BUFFER pCmdBuffer)
{
	GENOS_STATUS eStatus;
	PMI_ARB_CHECK_CMD_G75 pArbChkCmd;
	DWORD dwCmdSize;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);

	eStatus = GENOS_STATUS_SUCCESS;
	dwCmdSize = sizeof(MI_ARB_CHECK_CMD_G75);
	pArbChkCmd =
	    (PMI_ARB_CHECK_CMD_G75) IntelGen_OsGetCmdBufferSpace(pCmdBuffer,
								 dwCmdSize);

	GENHW_HW_CHK_NULL(pArbChkCmd);

	*pArbChkCmd = *pHwInterface->pHwCommands->pArbCheck_g75;

	IntelGen_OsAdjustCmdBufferFreeSpace(pCmdBuffer, dwCmdSize);

 finish:
	return eStatus;
}

GENHW_MEDIA_WALKER_MODE IntelGen_HwSelectWalkerStateMode_g75(PGENHW_HW_INTERFACE
							     pHwInterface)
{
	GENHW_MEDIA_WALKER_MODE Mode;

	Mode = pHwInterface->MediaWalkerMode;

	if (pHwInterface->Platform.GtType == GTTYPE_GT1) {
		Mode = GENHW_MEDIA_WALKER_REPEL_MODE;
	} else if (pHwInterface->Platform.GtType == GTTYPE_GT2) {
		if (pHwInterface->MediaWalkerMode !=
		    GENHW_MEDIA_WALKER_REPEL_MODE) {
			Mode = GENHW_MEDIA_WALKER_DUAL_MODE;
		}
	} else if ((pHwInterface->Platform.GtType == GTTYPE_GT3)) {
		if ((pHwInterface->MediaWalkerMode !=
		     GENHW_MEDIA_WALKER_REPEL_MODE)
		    && (pHwInterface->MediaWalkerMode !=
			GENHW_MEDIA_WALKER_DUAL_MODE)) {
			Mode = GENHW_MEDIA_WALKER_QUAD_MODE;
		}
	}

	return Mode;
}

GENOS_STATUS IntelGen_HwSendWalkerState_g75(PGENHW_HW_INTERFACE pHwInterface,
					    PGENOS_COMMAND_BUFFER pCmdBuffer,
					    PGENHW_WALKER_PARAMS pWalkerParams)
{
	MEDIA_OBJECT_WALKER_CMD_G6 MediaObjWalker;
	PMEDIA_OBJECT_WALKER_CMD_G6 pMediaObjWalker;
	GENHW_MEDIA_WALKER_MODE Mode;
	GENOS_STATUS eStatus;

	eStatus = GENOS_STATUS_SUCCESS;
	Mode = IntelGen_HwSelectWalkerStateMode_g75(pHwInterface);
	pMediaObjWalker = &MediaObjWalker;
	*pMediaObjWalker = *pHwInterface->pHwCommands->pMediaWalker_g75;

	pMediaObjWalker->DW0.Length =
	    OP_LENGTH(SIZE32(MEDIA_OBJECT_WALKER_CMD_G6)) +
	    pWalkerParams->InlineDataLength / sizeof(DWORD);
	pMediaObjWalker->DW1.InterfaceDescriptorOffset =
	    pWalkerParams->InterfaceDescriptorOffset;
	pMediaObjWalker->DW2.UseScoreboard =
	    pHwInterface->VfeScoreboard.ScoreboardEnable;
	pMediaObjWalker->DW5.ScoreboardMask =
	    pHwInterface->VfeScoreboard.ScoreboardMask;
	pMediaObjWalker->DW6.Value = 0;

	pMediaObjWalker->DW6.ColorCountMinusOne =
	    pWalkerParams->ColorCountMinusOne;

	switch (Mode) {
	case GENHW_MEDIA_WALKER_REPEL_MODE:
		pMediaObjWalker->DW6.Repel = 1;
		break;
	case GENHW_MEDIA_WALKER_DUAL_MODE:
		pMediaObjWalker->DW6.DualMode = 1;

		break;
	case GENHW_MEDIA_WALKER_QUAD_MODE:
		pMediaObjWalker->DW6.QuadMode = 1;
		break;
	default:
		GENHW_HW_ASSERTMESSAGE
		    ("Unsupported mode of media walker for G75");
		eStatus = GENOS_STATUS_UNKNOWN;
		goto finish;
	}

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

	if ((Mode == GENHW_MEDIA_WALKER_QUAD_MODE)
	    && ((pHwInterface->Platform.GtType == GTTYPE_GT3)
		|| pHwInterface->bRequestSingleSlice)) {
		pHwInterface->pfnSendMISetPredicateCmd(pHwInterface, pCmdBuffer,
						       MI_SET_PREDICATE_ENABLE_ON_CLEAR);

		GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer,
							  pMediaObjWalker,
							  sizeof
							  (MEDIA_OBJECT_WALKER_CMD_G6)));

		if (pWalkerParams->InlineDataLength) {
			GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer,
								  pWalkerParams->
								  pInlineData,
								  pWalkerParams->
								  InlineDataLength));
		}

		pHwInterface->pfnSendMISetPredicateCmd(pHwInterface,
						       pCmdBuffer,
						       MI_SET_PREDICATE_ENABLE_ON_SET);

		pMediaObjWalker->DW6.Repel = 0;
		pMediaObjWalker->DW6.DualMode = 1;
		pMediaObjWalker->DW6.QuadMode = 0;
		GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer,
							  pMediaObjWalker,
							  sizeof
							  (MEDIA_OBJECT_WALKER_CMD_G6)));

		if (pWalkerParams->InlineDataLength) {
			GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer,
								  pWalkerParams->
								  pInlineData,
								  pWalkerParams->
								  InlineDataLength));
		}

		pHwInterface->pfnSendMISetPredicateCmd(pHwInterface,
						       pCmdBuffer,
						       MI_SET_PREDICATE_DISABLE);
	} else {
		GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer,
							  pMediaObjWalker,
							  sizeof
							  (MEDIA_OBJECT_WALKER_CMD_G6)));

		if (pWalkerParams->InlineDataLength) {
			GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer,
								  pWalkerParams->
								  pInlineData,
								  pWalkerParams->
								  InlineDataLength));
		}
	}

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwSendVfeState_g75(PGENHW_HW_INTERFACE pHwInterface,
					 PGENOS_COMMAND_BUFFER pCmdBuffer,
					 BOOL blGpGpuWalkerMode)
{
	MEDIA_VFE_STATE_CMD_G6 VideoFrontEnd;
	PGENHW_HW_COMMANDS pHwCommands;
	PGENHW_GSH pGSH;
	PCGENHW_HW_CAPS pHwCaps;
	DWORD dwMaxURBSize;
	DWORD dwCURBEAllocationSize;
	DWORD dwURBEntryAllocationSize;
	DWORD dwNumberofURBEntries;
	DWORD dwMaxInterfaceDescriptorEntries;
	GENOS_STATUS eStatus;
	INT iSize;
	INT iRemain;
	INT iPerThreadScratchSize;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap);

	eStatus = GENOS_STATUS_SUCCESS;
	pHwCommands = pHwInterface->pHwCommands;
	pGSH = pHwInterface->pGeneralStateHeap;
	pHwCaps = pHwInterface->pHwCaps;

	VideoFrontEnd = *(pHwCommands->pVideoFrontEnd_g75);

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
		VideoFrontEnd.DW1.PerThreadScratchSpace = iSize;

		VideoFrontEnd.DW1.ScratchSpaceBasePointer =
		    pGSH->dwScratchSpaceBase >> 10;
	}
	dwMaxURBSize = pHwCaps->dwMaxURBSize;
	dwMaxInterfaceDescriptorEntries =
	    pHwCaps->dwMaxInterfaceDescriptorEntries;

	dwCURBEAllocationSize =
	    GENOS_ROUNDUP_SHIFT(pHwInterface->VfeStateParams.
				dwCURBEAllocationSize, 5);

	dwURBEntryAllocationSize =
	    GENOS_ROUNDUP_SHIFT(pHwInterface->VfeStateParams.
				dwURBEntryAllocationSize, 5);
	dwURBEntryAllocationSize = GFX_MAX(1, dwURBEntryAllocationSize);

	dwNumberofURBEntries =
	    (dwMaxURBSize - dwCURBEAllocationSize -
	     dwMaxInterfaceDescriptorEntries) / dwURBEntryAllocationSize;
	dwNumberofURBEntries = GFX_CLAMP_MIN_MAX(dwNumberofURBEntries, 1, 32);

	VideoFrontEnd.DW2.DebugCounterControl =
	    pHwInterface->VfeStateParams.dwDebugCounterControl;
	VideoFrontEnd.DW2.NumberofURBEntries = dwNumberofURBEntries;
	VideoFrontEnd.DW2.MaximumNumberofThreads =
	    pHwInterface->VfeStateParams.dwMaximumNumberofThreads - 1;
	VideoFrontEnd.DW4.CURBEAllocationSize = dwCURBEAllocationSize;
	VideoFrontEnd.DW4.URBEntryAllocationSize = dwURBEntryAllocationSize;

	if (pHwInterface->VfeScoreboard.ScoreboardEnable) {
		VideoFrontEnd.DW5.ScoreboardEnable = 1;
		VideoFrontEnd.DW5.ScoreboardMask =
		    pHwInterface->VfeScoreboard.ScoreboardMask;
		VideoFrontEnd.DW5.ScoreboardType =
		    pHwInterface->VfeScoreboard.ScoreboardType;
		VideoFrontEnd.DW6.Value = pHwInterface->VfeScoreboard.Value[0];
		VideoFrontEnd.DW7.Value = pHwInterface->VfeScoreboard.Value[1];
	}

	if (blGpGpuWalkerMode) {
		VideoFrontEnd.DW2.GPGPUMode = MEDIASTATE_GPGPU_MODE;
		VideoFrontEnd.DW2.BypassGatewayControl = 1;
	}

	GENHW_HW_ASSERT(VideoFrontEnd.DW2.NumberofURBEntries <=
			pHwCaps->dwMaxURBEntries);
	GENHW_HW_ASSERT(VideoFrontEnd.DW4.CURBEAllocationSize <=
			pHwCaps->dwMaxCURBEAllocationSize);
	GENHW_HW_ASSERT(VideoFrontEnd.DW4.URBEntryAllocationSize <=
			pHwCaps->dwMaxURBEntryAllocationSize);
	GENHW_HW_ASSERT(VideoFrontEnd.DW2.NumberofURBEntries *
			VideoFrontEnd.DW4.URBEntryAllocationSize +
			VideoFrontEnd.DW4.CURBEAllocationSize +
			dwMaxInterfaceDescriptorEntries <= dwMaxURBSize);

	GENHW_HW_CHK_STATUS(IntelGen_OsAddCommand(pCmdBuffer,
						  &VideoFrontEnd,
						  sizeof(VideoFrontEnd)));

 finish:
	return eStatus;
}

BOOL IntelGen_HwIs2PlaneNV12Needed_g75(PGENHW_HW_INTERFACE pHwInterface,
				       PGENHW_SURFACE pSurface)
{
	WORD wWidthAlignUnit;
	WORD wHeightAlignUnit;
	DWORD dwSurfaceHeight;
	DWORD dwSurfaceWidth;

	pHwInterface->pfnGetAlignUnit(&wWidthAlignUnit, &wHeightAlignUnit,
				      pSurface);

	dwSurfaceHeight =
	    GENOS_ALIGN_CEIL(pSurface->dwHeight, wHeightAlignUnit);
	dwSurfaceWidth = GENOS_ALIGN_CEIL(pSurface->dwWidth, wWidthAlignUnit);

	return (!GENOS_IS_ALIGNED(dwSurfaceHeight, 4)
		|| !GENOS_IS_ALIGNED(dwSurfaceWidth, 4));
}

DWORD IntelGen_GetScratchSpaceSize_g75(PGENHW_HW_INTERFACE pHwInterface,
				       DWORD iPerThreadScratchSpaceSize)
{
	DWORD dwScratchSpaceSize = (GENHW_MEDIA_THREADS_MAX_G75_GT1 +
				    (pHwInterface->pHwCaps->dwMaxThreads /
				     GENHW_MEDIA_THREADS_MAX_G75_GT1 -
				     1) *
				    GENHW_MEDIAL_SUBSLICE_SCRATCH_DISTANCE_G75)
	    * iPerThreadScratchSpaceSize;

	return dwScratchSpaceSize;
}

GENOS_STATUS IntelGen_HwSendMediaStateFlush_g75(PGENHW_HW_INTERFACE
						pHwInterface,
						PGENOS_COMMAND_BUFFER
						pCmdBuffer)
{
	GENOS_STATUS eStatus;
	PMEDIA_STATE_FLUSH_CMD_G75 pMediaStateFlush;
	DWORD dwCmdSize;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pCmdBuffer);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);

	eStatus = GENOS_STATUS_SUCCESS;
	dwCmdSize = sizeof(MEDIA_STATE_FLUSH_CMD_G75);
	pMediaStateFlush = (PMEDIA_STATE_FLUSH_CMD_G75)
	    IntelGen_OsGetCmdBufferSpace(pCmdBuffer, dwCmdSize);

	GENHW_HW_CHK_NULL(pMediaStateFlush);

	*pMediaStateFlush = *pHwInterface->pHwCommands->pMediaStateFlush_g75;

	IntelGen_OsAdjustCmdBufferFreeSpace(pCmdBuffer, dwCmdSize);

 finish:
	return eStatus;
}

VOID IntelGen_HwInitInterface_g75(PGENHW_HW_INTERFACE pHwInterface)
{
	if (pHwInterface->Platform.GtType == GTTYPE_GT1) {
		pHwInterface->pHwCaps = &g_IntelGen_HwCaps_g75_gt1;
	} else if (pHwInterface->Platform.GtType == GTTYPE_GT2) {
		pHwInterface->pHwCaps = &g_IntelGen_HwCaps_g75_gt2;
	} else {
		pHwInterface->pHwCaps = &g_IntelGen_HwCaps_g75_gt3;
	}

	pHwInterface->GshSettings = g_GSH_Settings_g75;

	pHwInterface->SshSettings = g_SSH_Settings_g75;

	pHwInterface->SurfaceTypeDefault = GENHW_SURFACE_TYPE_G7;
	pHwInterface->bUsesPatchList = TRUE;

	pHwInterface->pPlaneDefinitions = g_cInitSurfacePlanes_g75;
	pHwInterface->iSizeBindingTableState = sizeof(BINDING_TABLE_STATE_G5);
	pHwInterface->iSizeInstructionCache = GENHW_INSTRUCTION_CACHE_G75;
	pHwInterface->iSizeInterfaceDescriptor =
	    sizeof(INTERFACE_DESCRIPTOR_DATA_G6);
	pHwInterface->bEnableYV12SinglePass = FALSE;

	pHwInterface->VfeStateParams.dwDebugCounterControl = 0;
	pHwInterface->VfeStateParams.dwMaximumNumberofThreads =
	    pHwInterface->pHwCaps->dwMaxThreads;

	pHwInterface->pfnInitCommands = IntelGen_HwInitCommands_g75;
	pHwInterface->pfnAssignBindingTable = IntelGen_HwAssignBindingTable_g75;
	pHwInterface->pfnSetupSurfaceState = IntelGen_HwSetupSurfaceState_g75;
	pHwInterface->pfnSetupBufferSurfaceState =
	    IntelGen_HwSetupBufferSurfaceState_g75;
	pHwInterface->pfnLoadCurbeData = IntelGen_HwLoadCurbeData_g75;
	pHwInterface->pfnInitInterfaceDescriptor =
	    IntelGen_HwInitInterfaceDescriptor_g75;
	pHwInterface->pfnSetupInterfaceDescriptor =
	    IntelGen_HwSetupInterfaceDescriptor_g75;
	pHwInterface->pfnGetMediaWalkerStatus =
	    IntelGen_HwGetMediaWalkerStatus_g75;
	pHwInterface->pfnGetMediaWalkerBlockSize =
	    IntelGen_HwGetMediaWalkerBlockSize_g75;

	pHwInterface->pfnSendVfeState = IntelGen_HwSendVfeState_g75;
	pHwInterface->pfnSendCurbeLoad = IntelGen_HwSendCurbeLoad_g75;
	pHwInterface->pfnSendIDLoad = IntelGen_HwSendIDLoad_g75;
	pHwInterface->pfnAddMediaObjectCmdBb =
	    IntelGen_HwAddMediaObjectCmdBb_g75;
	pHwInterface->pfnGetSurfaceMemoryObjectControl =
	    IntelGen_HwGetSurfaceMemoryObjectControl_g75;
	pHwInterface->pfnSendMediaObjectWalker = IntelGen_HwSendWalkerState_g75;
	pHwInterface->pfnSendMISetPredicateCmd =
	    IntelGen_HwSendMISetPredicateCmd_g75;
	pHwInterface->pfnSendPipelineSelectCmd =
	    IntelGen_HwSendPipelineSelectCmd_g75;
	pHwInterface->pfnGetScratchSpaceSize = IntelGen_GetScratchSpaceSize_g75;
	pHwInterface->pfnConvertToNanoSeconds =
	    IntelGen_HwConvertToNanoSeconds_g75;
	pHwInterface->pfnSendGpGpuWalkerState =
	    IntelGen_HwSendGpGpuWalkerState_g75;
	pHwInterface->pfnAddPipeControlCmdBb =
	    IntelGen_HwAddPipeControlCmdBb_g75;
	pHwInterface->pfnSkipPipeControlCmdBb =
	    IntelGen_HwSkipPipeControlCmdBb_g75;
	pHwInterface->pfnSendMediaStateFlush =
	    IntelGen_HwSendMediaStateFlush_g75;
	pHwInterface->pfnSendMIArbCheckCmd = IntelGen_HwSendMIArbCheck_g75;

	pHwInterface->pfnIs2PlaneNV12Needed = IntelGen_HwIs2PlaneNV12Needed_g75;
}
