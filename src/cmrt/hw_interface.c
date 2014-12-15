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
#define GFX_MIN(a,b)             (((a) < (b)) ? (a) : (b))
#define GFX_MAX(a,b)             (((a) > (b)) ? (a) : (b))

CONST GENHW_SURFACE_PLANES g_cInitSurfacePlanes[GENHW_PLANES_DEFINITION_COUNT] = {
	// GENHW_PLANES_PL3
	{3,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_U_PLANE, 2, 2, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_V_PLANE, 2, 2, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_NV12
	{2,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_U_PLANE, 2, 2, 1, 1, 2, 0, GFX3DSTATE_SURFACEFORMAT_R8G8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_YUY2
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_YCRCB_NORMAL}
	  }
	 }
	,
	// GENHW_PLANES_UYVY
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_YCRCB_SWAPY}
	  }
	 }
	,
	// GENHW_PLANES_YVYU
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_YCRCB_SWAPUV}
	  }
	 }
	,
	// GENHW_PLANES_VYUY
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_YCRCB_SWAPUVY}
	  }
	 }
	,
	// GENHW_PLANES_ARGB
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_B8G8R8A8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_XRGB
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_B8G8R8X8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_ABGR
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R8G8B8A8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_XBGR
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R8G8B8X8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_RGB16
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_B5G6R5_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_R16U
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_R16_UINT}
	  }
	 }
	,
	// GENHW_PLANES_R16S
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_R16_SINT}
	  }
	 }
	,
	// GENHW_PLANES_R32U
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R32_UINT}
	  }
	 }
	,
	// GENHW_PLANES_R32S
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R32_SINT}
	  }
	 }
	,
	// GENHW_PLANES_R32F
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_R32_FLOAT}
	  }
	 }
	,
	// GENHW_PLANES_V8U8
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_R8G8_SNORM}
	  }
	 }
	,
	// GENHW_PLANES_R8G8_UNORM
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_R8G8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_411P
	{3,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_U_PLANE, 4, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_V_PLANE, 4, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_411R
	{3,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_U_PLANE, 1, 4, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_V_PLANE, 1, 4, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_422H
	{3,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_U_PLANE, 2, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_V_PLANE, 2, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_422V
	{3,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_U_PLANE, 1, 2, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_V_PLANE, 1, 2, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_444P
	{3,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_U_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_V_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_RGBP
	{3,
	 {
	  {GENHW_U_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_V_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_BGRP
	{3,
	 {
	  {GENHW_U_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_V_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_AI44_PALLETE_0
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_P4A4_UNORM_PALETTE_0}
	  }
	 }
	,
	// GENHW_PLANES_IA44_PALLETE_0
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_A4P4_UNORM_PALETTE_0}
	  }
	 }
	,
	// GENHW_PLANES_P8_PALLETE_0
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_P8_UNORM_PALETTE_0}
	  }
	 }
	,
	// GENHW_PLANES_A8P8_PALLETE_0
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_P8A8_UNORM_PALETTE_0}
	  }
	 }
	,
	// GENHW_PLANES_AI44_PALLETE_1
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_P4A4_UNORM_PALETTE_1}
	  }
	 }
	,
	// GENHW_PLANES_IA44_PALLETE_1
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_A4P4_UNORM_PALETTE_1}
	  }
	 }
	,
	// GENHW_PLANES_P8_PALLETE_1
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_P8_UNORM_PALETTE_1}
	  }
	 }
	,
	// GENHW_PLANES_A8P8_PALLETE_1
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_P8A8_UNORM_PALETTE_1}
	  }
	 }
	,
	// GENHW_PLANES_AYUV
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 1, 0,
	   GFX3DSTATE_SURFACEFORMAT_B8G8R8A8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_STMM
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_L8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_L8
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_L8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_A8
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_A8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_R8
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 4, 0,
	   GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_NV12_2PLANES
	{2,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_U_PLANE, 2, 2, 1, 1, 2, 0, GFX3DSTATE_SURFACEFORMAT_R8G8_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_R16_UNORM
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_R16_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_L16
	{1,
	 {
	  {GENHW_GENERIC_PLANE, 1, 1, 1, 1, 2, 0,
	   GFX3DSTATE_SURFACEFORMAT_L16_UNORM}
	  }
	 }
	,
	// GENHW_PLANES_NV21
	{2,
	 {
	  {GENHW_Y_PLANE, 1, 1, 1, 1, 4, 0, GFX3DSTATE_SURFACEFORMAT_R8_UNORM}
	  ,
	  {GENHW_U_PLANE, 2, 2, 1, 1, 2, 0, GFX3DSTATE_SURFACEFORMAT_R8G8_UNORM}
	  }
	 }
};

CONST GENHW_SURFACE_STATE_ENTRY g_cInitSurfaceStateEntry = {
	GENHW_SURFACE_TYPE_INVALID,
	NULL,
	NULL,
	-1,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};

VOID IntelGen_HwInitInterface_g75(PGENHW_HW_INTERFACE pHwInterface);

VOID IntelGen_HwInitInterface_g8(PGENHW_HW_INTERFACE pHwInterface);

GENOS_STATUS IntelGen_HwInitInterface(PGENHW_HW_INTERFACE pHwInterface);

VOID IntelGen_HwGetAlignUnit(PWORD pwWidthAlignUnit,
			     PWORD pwHeightAlignUnit, PGENHW_SURFACE pSurface)
{
	switch (pSurface->Format) {
	case Format_YUY2:
	case Format_UYVY:
	case Format_YVYU:
	case Format_VYUY:
	case Format_P208:
		*pwWidthAlignUnit = 1;
		*pwHeightAlignUnit = 2;
		break;

	default:
		*pwWidthAlignUnit = 1;
		*pwHeightAlignUnit = 1;
		break;
	}
}

VOID IntelGen_HwAdjustBoundary(PGENHW_HW_INTERFACE pHwInterface,
			       PGENHW_SURFACE pSurface,
			       PDWORD pdwSurfaceWidth, PDWORD pdwSurfaceHeight)
{
	WORD wWidthAlignUnit;
	WORD wHeightAlignUnit;

	pHwInterface->pfnGetAlignUnit(&wWidthAlignUnit, &wHeightAlignUnit,
				      pSurface);

	*pdwSurfaceHeight =
	    GENOS_ALIGN_CEIL(pSurface->dwHeight, wHeightAlignUnit);
	*pdwSurfaceWidth = GENOS_ALIGN_CEIL(pSurface->dwWidth, wWidthAlignUnit);
}

VOID IntelGen_GetPixelsPerSample(GENOS_FORMAT format,
				 PDWORD pdwPixelsPerSampleUV)
{
	*pdwPixelsPerSampleUV = 0;
	switch (format) {
 CASE_PL3_FORMAT:
 CASE_PL3_RGB_FORMAT:
		*pdwPixelsPerSampleUV = 4;
		break;

 CASE_PL2_FORMAT:
	case Format_400P:
		*pdwPixelsPerSampleUV = 2;
		break;

	default:
		*pdwPixelsPerSampleUV = 1;
		GENHW_HW_ASSERTMESSAGE("Incorrect Filter Format.");
		break;
	}
}

DWORD IntelGen_HwGetCurBindingTableBase(PGENHW_SSH pSSH)
{
	DWORD dwOffset;

	GENHW_HW_ASSERT(pSSH);

	dwOffset = (pSSH->iCurSshBufferIndex * pSSH->dwSshIntanceSize) +
	    (pSSH->iBindingTableOffset);

	return dwOffset;
}

DWORD IntelGen_HwGetCurSurfaceStateBase(PGENHW_SSH pSSH)
{
	DWORD dwOffset;

	GENHW_HW_ASSERT(pSSH);

	dwOffset = (pSSH->iCurSshBufferIndex * pSSH->dwSshIntanceSize) +
	    (pSSH->iSurfaceStateOffset);

	return dwOffset;
}

PGENOS_INTERFACE IntelGen_HwGetOsInterface(PGENHW_HW_INTERFACE pHwInterface)
{
	return pHwInterface->pOsInterface;
}

GENOS_STATUS IntelGen_HwAllocateCommands(PGENHW_HW_INTERFACE pHwInterface)
{
	PGENHW_HW_COMMANDS pCommands;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);

	eStatus = GENOS_STATUS_UNKNOWN;

	if (pHwInterface->pHwCommands != NULL) {
		GENHW_HW_NORMALMESSAGE("HW commands already allocated.");
		goto finish;
	}

	pCommands = (PGENHW_HW_COMMANDS)
	    GENOS_AllocAndZeroMemory(sizeof(GENHW_HW_COMMANDS));
	GENHW_HW_CHK_NULL(pCommands);

	pHwInterface->pHwCommands = pCommands;

	pHwInterface->pfnInitCommands(pHwInterface);

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

VOID IntelGen_HwInitCommandsCommon(PGENHW_HW_INTERFACE pHwInterface)
{
	PGENHW_HW_COMMANDS pHwCommands;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pHwCommands);

	pHwCommands = pHwInterface->pHwCommands;

	pHwCommands->Platform = pHwInterface->Platform;

	pHwCommands->pSurfaceState_g75 = &g_cInit_SURFACE_STATE_G7;
	pHwCommands->pSurfaceState_g8 = &g_cInit_SURFACE_STATE_G8;

	pHwCommands->pBindingTableState_g75 = &g_cInit_BINDING_TABLE_STATE_G5;
	pHwCommands->pBindingTableState_g8 = &g_cInit_BINDING_TABLE_STATE_G8;

	pHwCommands->pBatchBufferEnd = &g_cInit_MI_BATCH_BUFFER_END_CMD_G5;

	pHwCommands->pVideoFrontEnd_g75 = &g_cInit_MEDIA_VFE_STATE_CMD_G6;
	pHwCommands->pMediaCurbeLoad_g75 = &g_cInit_MEDIA_CURBE_LOAD_CMD_G6;
	pHwCommands->pMediaIDLoad_g75 =
	    &g_cInit_MEDIA_INTERFACE_DESCRIPTOR_LOAD_CMD_G6;
	pHwCommands->pMediaWalker_g75 = &g_cInit_MEDIA_OBJECT_WALKER_CMD_G6;
	pHwCommands->pGpGpuWalker_g75 = &g_cInit_GPGPU_WALKER_CMD_G75;
	pHwCommands->pInterfaceDescriptor_g75 =
	    &g_cInit_INTERFACE_DESCRIPTOR_DATA_G6;
	pHwCommands->pLoadRegImm_g75 = &g_cInit_MI_LOAD_REGISTER_IMM_CMD_G6;

	pHwCommands->pSurfaceStateToken_g75 = &g_cInit_SURFACE_STATE_TOKEN_G75;

	pHwCommands->pPipeControl_g75 = &g_cInit_PIPE_CONTROL_CMD_G7;

	pHwCommands->pStateBaseAddress_g75 =
	    &g_cInit_STATE_BASE_ADDRESS_CMD_G75;
	pHwCommands->pBatchBufferStart_g75 =
	    &g_cInit_MI_BATCH_BUFFER_START_CMD_G75;

	pHwCommands->pMediaStateFlush_g75 = &g_cInit_MEDIA_STATE_FLUSH_CMD_G75;

	pHwCommands->pPipeControl_g8 = &g_cInit_PIPE_CONTROL_CMD_G8;
	pHwCommands->pStateBaseAddress_g8 = &g_cInit_STATE_BASE_ADDRESS_CMD_G8;
	pHwCommands->pInterfaceDescriptor_g8 =
	    &g_cInit_INTERFACE_DESCRIPTOR_DATA_G8;
	pHwCommands->pBatchBufferStart_g8 =
	    &g_cInit_MI_BATCH_BUFFER_START_CMD_G8;
	pHwCommands->pVideoFrontEnd_g8 = &g_cInit_MEDIA_VFE_STATE_CMD_G8;
	pHwCommands->pGpGpuWalker_g8 = &g_cInit_GPGPU_WALKER_CMD_G8;
}

VOID IntelGen_HwFreeCommands(PGENHW_HW_INTERFACE pHwInterface)
{
	GENHW_HW_ASSERT(pHwInterface);

	if (pHwInterface->pHwCommands) {
		GENOS_FreeMemory(pHwInterface->pHwCommands);
		pHwInterface->pHwCommands = NULL;
	}
}

GENOS_STATUS IntelGen_HwAllocateGSH(PGENHW_HW_INTERFACE pHwInterface,
				    PCGENHW_GSH_SETTINGS pSettings)
{
	PGENOS_INTERFACE pOsInterface;
	PGENHW_GSH pGSH;
	PINT pAllocations;
	PBYTE pData;
	DWORD dwSizeGSH;
	DWORD dwSizeMedia;
	INT i;
	INT j;
	PBYTE pBase;
	DWORD dwBase;
	DWORD dwOffsetID;
	GENOS_ALLOC_GFXRES_PARAMS AllocParams;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pOsInterface);
	GENHW_HW_ASSERT(pSettings);
	GENHW_HW_ASSERT((pSettings->iSyncSize % GENHW_SYNC_BLOCK_ALIGN) == 0);
	GENHW_HW_ASSERT((pSettings->iCurbeSize % GENHW_URB_BLOCK_ALIGN) == 0);
	GENHW_HW_ASSERT((pSettings->iKernelHeapSize %
			 GENHW_KERNEL_BLOCK_ALIGN) == 0);
	GENHW_HW_ASSERT((pSettings->iKernelBlockSize %
			 GENHW_KERNEL_BLOCK_ALIGN) == 0);

	eStatus = GENOS_STATUS_UNKNOWN;
	pGSH = NULL;
	pData = NULL;
	dwSizeGSH = 0;
	dwSizeMedia = 0;
	pOsInterface = pHwInterface->pOsInterface;

	dwSizeGSH = sizeof(GENHW_GSH);
	dwSizeGSH +=
	    pSettings->iKernelCount * sizeof(GENHW_KRN_ALLOCATION) + 16;
	dwSizeGSH +=
	    pSettings->iMediaStateHeaps * sizeof(GENHW_MEDIA_STATE) + 16;
	dwSizeGSH +=
	    pSettings->iMediaStateHeaps * pSettings->iMediaIDs * sizeof(INT) +
	    16;

	pGSH = (PGENHW_GSH) GENOS_AllocAndZeroMemory(dwSizeGSH);
	GENHW_HW_CHK_NULL(pGSH);

	pGSH->pKernelAllocation = (PGENHW_KRN_ALLOCATION) (pGSH + 1);
	pGSH->pMediaStates =
	    (PGENHW_MEDIA_STATE) (pGSH->pKernelAllocation +
				  pSettings->iKernelCount);
	pAllocations =
	    (PINT) (pGSH->pMediaStates + pSettings->iMediaStateHeaps);

	dwSizeGSH = 0;

	pGSH->dwOffsetSync = dwSizeGSH;
	pGSH->dwSizeSync = pSettings->iSyncSize;
	pGSH->dwNextTag = 0;
	pGSH->dwSyncTag = 0;
	dwSizeGSH += pGSH->dwSizeSync;

	pGSH->iCurMediaState = 0;
	pGSH->iNextMediaState = 0;

	pGSH->dwOffsetCurbe = dwSizeMedia;
	pGSH->dwSizeCurbe = pSettings->iCurbeSize;
	dwSizeMedia += pGSH->dwSizeCurbe;

	pGSH->dwOffsetMediaID = dwSizeMedia;
	pGSH->dwSizeMediaID = pHwInterface->iSizeInterfaceDescriptor;
	dwSizeMedia += pSettings->iMediaIDs * pGSH->dwSizeMediaID;

	for (i = 0; i < pSettings->iMediaStateHeaps; i++) {
		pGSH->pMediaStates[i].dwOffset = dwSizeGSH;
		pGSH->pMediaStates[i].piAllocation = pAllocations;
		dwSizeGSH = GENOS_ALIGN_CEIL(dwSizeGSH + dwSizeMedia, 128);
		pAllocations += pSettings->iMediaIDs;
	}

	pGSH->dwKernelBase = dwSizeGSH;
	dwSizeGSH =
	    GENOS_ALIGN_CEIL(dwSizeGSH + pSettings->iKernelHeapSize, 64);

	if (pSettings->iPerThreadScratchSize > 0) {
		dwSizeGSH = GENOS_ALIGN_CEIL(dwSizeGSH, 1024);

		GENHW_HW_ASSERT(pSettings->iPerThreadScratchSize ==
				GENOS_ALIGN_CEIL
				(pSettings->iPerThreadScratchSize, 1024));

		pGSH->dwScratchSpaceSize =
		    pHwInterface->pfnGetScratchSpaceSize(pHwInterface,
							 pSettings->
							 iPerThreadScratchSize);
		pGSH->dwScratchSpaceBase = dwSizeGSH;
		dwSizeGSH += pGSH->dwScratchSpaceSize;
	}

	dwSizeGSH = GENOS_ALIGN_CEIL(dwSizeGSH, 16);
	pGSH->dwSipBase = dwSizeGSH;
	dwSizeGSH += pSettings->iSipSize;

	pGSH->dwGSHSize = dwSizeGSH;
	pGSH->bGSHLocked = FALSE;
	pGSH->pGSH = NULL;

	GENOS_ZeroMemory(&AllocParams, sizeof(GENOS_ALLOC_GFXRES_PARAMS));
	AllocParams.Type = GENOS_GFXRES_BUFFER;
	AllocParams.TileType = GENOS_TILE_LINEAR;
	AllocParams.Format = Format_Buffer;
	AllocParams.dwBytes = dwSizeGSH;
	AllocParams.pBufName = "GenHwGSH";

	GENHW_HW_CHK_STATUS(pOsInterface->pfnAllocateResource(pOsInterface,
							      &AllocParams,
							      &pGSH->
							      OsResource));

	pHwInterface->pGeneralStateHeap = pGSH;
	GENHW_HW_CHK_STATUS(pHwInterface->pfnLockGSH(pHwInterface));

	pData = (PBYTE) (pGSH->pGSH);
	GENOS_ZeroMemory(pData, dwSizeGSH);

	GENOS_ZeroMemory(pGSH->pSync, pGSH->dwSizeSync);

	for (i = 0; i < pSettings->iMediaStateHeaps; i++) {
		dwBase = pGSH->pMediaStates[i].dwOffset;
		pBase = pData + dwBase;

		dwOffsetID = pGSH->dwOffsetMediaID;
		for (j = 0; j < pSettings->iMediaIDs; j++) {
			pHwInterface->pfnInitInterfaceDescriptor(pHwInterface,
								 pBase,
								 dwBase,
								 dwOffsetID);

			dwOffsetID += pGSH->dwSizeMediaID;
		}
	}

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	if (eStatus != GENOS_STATUS_SUCCESS) {
		if (pGSH) {
			pOsInterface->pfnFreeResource(pOsInterface,
						      &pGSH->OsResource);
			GENOS_FreeMemory(pGSH);
			pHwInterface->pGeneralStateHeap = NULL;
		}
	}

	return eStatus;
}

GENOS_STATUS IntelGen_HwFreeGSH(PGENHW_HW_INTERFACE pHwInterface)
{
	PGENOS_INTERFACE pOsInterface;
	PGENHW_GSH pGSH;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pOsInterface);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap);

	eStatus = GENOS_STATUS_UNKNOWN;

	pOsInterface = pHwInterface->pOsInterface;
	pGSH = pHwInterface->pGeneralStateHeap;

	if (pGSH->bGSHLocked && !IntelGen_OsResourceIsNull(&pGSH->OsResource)) {
		GENHW_HW_CHK_STATUS(pOsInterface->pfnUnlockResource
				    (pOsInterface, &pGSH->OsResource));
	}
	if (!IntelGen_OsResourceIsNull(&pGSH->OsResource)) {
		pOsInterface->pfnFreeResource(pOsInterface, &pGSH->OsResource);
	}
	GENOS_FreeMemory(pGSH);
	pHwInterface->pGeneralStateHeap = NULL;

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwLockGSH(PGENHW_HW_INTERFACE pHwInterface)
{
	PGENOS_INTERFACE pOsInterface;
	PGENHW_GSH pGSH;
	GENOS_LOCK_PARAMS LockFlags;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pOsInterface);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap);

	eStatus = GENOS_STATUS_SUCCESS;
	pOsInterface = pHwInterface->pOsInterface;
	pGSH = pHwInterface->pGeneralStateHeap;

	if (pGSH->bGSHLocked) {
		GENHW_HW_ASSERTMESSAGE("GSH already locked.");
		eStatus = GENOS_STATUS_UNKNOWN;
		goto finish;
	}
	GENOS_ZeroMemory(&LockFlags, sizeof(GENOS_LOCK_PARAMS));

	LockFlags.NoOverWrite = 1;

	pGSH->pGSH = (PBYTE) pOsInterface->pfnLockResource(pOsInterface,
							   &pGSH->OsResource,
							   &LockFlags);
	GENHW_HW_CHK_NULL(pGSH->pGSH);

	pGSH->bGSHLocked = TRUE;

	pGSH->pSync = (PDWORD) (pGSH->pGSH + pGSH->dwOffsetSync);

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwUnlockGSH(PGENHW_HW_INTERFACE pHwInterface)
{
	PGENOS_INTERFACE pOsInterface;
	PGENHW_GSH pGSH;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pOsInterface);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap);

	eStatus = GENOS_STATUS_UNKNOWN;
	pOsInterface = pHwInterface->pOsInterface;
	pGSH = pHwInterface->pGeneralStateHeap;

	if (pGSH->bGSHLocked == TRUE) {
		GENHW_HW_CHK_STATUS(pOsInterface->pfnUnlockResource
				    (pOsInterface, &pGSH->OsResource));

		pGSH->bGSHLocked = FALSE;
		pGSH->pGSH = NULL;
		pGSH->pSync = NULL;
	}

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwResetGSH(PGENHW_HW_INTERFACE pHwInterface)
{
	return GENOS_STATUS_SUCCESS;
}

GENOS_STATUS IntelGen_HwAllocateBB(PGENHW_HW_INTERFACE pHwInterface,
				   PGENHW_BATCH_BUFFER pBatchBuffer, INT iSize)
{
	PGENOS_INTERFACE pOsInterface;
	GENOS_RESOURCE OsResource;
	GENOS_ALLOC_GFXRES_PARAMS AllocParams;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pOsInterface);
	GENHW_HW_ASSERT(pBatchBuffer);

	eStatus = GENOS_STATUS_UNKNOWN;
	pOsInterface = pHwInterface->pOsInterface;

	GENOS_ZeroMemory(&OsResource, sizeof(OsResource));

	GENOS_ZeroMemory(&AllocParams, sizeof(GENOS_ALLOC_GFXRES_PARAMS));

	AllocParams.Type = GENOS_GFXRES_BUFFER;
	AllocParams.TileType = GENOS_TILE_LINEAR;
	AllocParams.Format = Format_Buffer;
	AllocParams.dwBytes = iSize;
	AllocParams.pBufName = "GenHwBB";

	GENHW_HW_CHK_STATUS(pOsInterface->pfnAllocateResource(pOsInterface,
							      &AllocParams,
							      &OsResource));

	pOsInterface->pfnResetResourceAllocationIndex(pOsInterface,
						      &OsResource);

	pBatchBuffer->OsResource = OsResource;
	pBatchBuffer->iSize = iSize;
	pBatchBuffer->iCurrent = 0;
	pBatchBuffer->bLocked = FALSE;

	pBatchBuffer->bBusy = FALSE;
	pBatchBuffer->dwSyncTag = 0;
	pBatchBuffer->pPrev = NULL;
	pBatchBuffer->pNext = pHwInterface->pBatchBufferList;
	pHwInterface->pBatchBufferList = pBatchBuffer;
	if (pBatchBuffer->pNext) {
		pBatchBuffer->pNext->pPrev = pBatchBuffer;
	}

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwFreeBB(PGENHW_HW_INTERFACE pHwInterface,
			       PGENHW_BATCH_BUFFER pBatchBuffer)
{
	PGENOS_INTERFACE pOsInterface;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pOsInterface);
	GENHW_HW_ASSERT(pBatchBuffer);

	eStatus = GENOS_STATUS_UNKNOWN;
	pOsInterface = pHwInterface->pOsInterface;

	if (pBatchBuffer->bLocked) {
		GENHW_HW_CHK_STATUS(pHwInterface->pfnUnlockBB
				    (pHwInterface, pBatchBuffer));
	}

	pOsInterface->pfnFreeResource(pOsInterface, &pBatchBuffer->OsResource);

	pBatchBuffer->dwSyncTag = 0;
	pBatchBuffer->iSize = 0;
	pBatchBuffer->iCurrent = 0;

	if (pBatchBuffer->pNext) {
		pBatchBuffer->pNext->pPrev = pBatchBuffer->pPrev;
	}

	if (pBatchBuffer->pPrev) {
		pBatchBuffer->pPrev->pNext = pBatchBuffer->pNext;
	} else {
		pHwInterface->pBatchBufferList = pBatchBuffer->pNext;
	}

	pBatchBuffer->pPrev = pBatchBuffer->pNext = NULL;

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwLockBB(PGENHW_HW_INTERFACE pHwInterface,
			       PGENHW_BATCH_BUFFER pBatchBuffer)
{
	PGENOS_INTERFACE pOsInterface;
	GENOS_LOCK_PARAMS LockFlags;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pBatchBuffer);
	GENHW_HW_ASSERT(pHwInterface->pOsInterface);

	eStatus = GENOS_STATUS_UNKNOWN;
	pOsInterface = pHwInterface->pOsInterface;

	if (pBatchBuffer->bLocked) {
		GENHW_HW_ASSERTMESSAGE("Batch Buffer is already locked.");
		goto finish;
	}

	GENOS_ZeroMemory(&LockFlags, sizeof(GENOS_LOCK_PARAMS));

	LockFlags.WriteOnly = 1;

	pBatchBuffer->pData =
	    (PBYTE) pOsInterface->pfnLockResource(pOsInterface,
						  &pBatchBuffer->OsResource,
						  &LockFlags);

	GENHW_HW_CHK_NULL(pBatchBuffer->pData);

	pBatchBuffer->bLocked = TRUE;
	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwUnlockBB(PGENHW_HW_INTERFACE pHwInterface,
				 PGENHW_BATCH_BUFFER pBatchBuffer)
{
	PGENOS_INTERFACE pOsInterface;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pBatchBuffer);
	GENHW_HW_ASSERT(pHwInterface->pOsInterface);

	eStatus = GENOS_STATUS_UNKNOWN;
	pOsInterface = pHwInterface->pOsInterface;

	if (!pBatchBuffer->bLocked) {
		GENHW_HW_ASSERTMESSAGE("Batch buffer is locked.");
		goto finish;
	}

	GENHW_HW_CHK_STATUS(pOsInterface->pfnUnlockResource(pOsInterface,
							    &pBatchBuffer->
							    OsResource));

	pBatchBuffer->bLocked = FALSE;
	pBatchBuffer->pData = NULL;

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwRefreshSync(PGENHW_HW_INTERFACE pHwInterface)
{
	PGENHW_GSH pGSH;
	PGENHW_MEDIA_STATE pCurMediaState;
	PGENHW_BATCH_BUFFER pBatchBuffer;
	DWORD dwCurrentTag;
	INT i;
	INT iStatesInUse;
	INT iBuffersInUse;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pGeneralStateHeap);

	eStatus = GENOS_STATUS_UNKNOWN;

	pGSH = pHwInterface->pGeneralStateHeap;
	if (!pGSH->bGSHLocked) {
		goto finish;
	}
	dwCurrentTag = pGSH->pSync[0];
	pGSH->dwSyncTag = dwCurrentTag - 1;

	iBuffersInUse = 0;
	pBatchBuffer = pHwInterface->pBatchBufferList;

	for (; pBatchBuffer != NULL; pBatchBuffer = pBatchBuffer->pNext) {
		if (!pBatchBuffer->bBusy)
			continue;

		if ((INT) (dwCurrentTag - pBatchBuffer->dwSyncTag) > 0) {
			pBatchBuffer->bBusy = FALSE;
		} else {
			iBuffersInUse++;
		}
	}

	pCurMediaState = pGSH->pMediaStates;
	iStatesInUse = 0;
	for (i = pHwInterface->GshSettings.iMediaStateHeaps; i > 0;
	     i--, pCurMediaState++) {
		if (!pCurMediaState->bBusy)
			continue;

		if ((INT) (dwCurrentTag - pCurMediaState->dwSyncTag) > 0) {
			pCurMediaState->bBusy = FALSE;
		} else {
			iStatesInUse++;
		}
	}

	pHwInterface->iBuffersInUse = iBuffersInUse;
	pHwInterface->iMediaStatesInUse = iStatesInUse;

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwAssignSurfaceState(PGENHW_HW_INTERFACE pHwInterface,
					   GENHW_SURFACE_STATE_TYPE Type,
					   PGENHW_SURFACE_STATE_ENTRY *
					   ppSurfaceEntry)
{
	PGENHW_SSH pSSH;
	INT iSurfaceEntry;
	PGENHW_SURFACE_STATE_ENTRY pSurfaceEntry;
	DWORD dwOffset;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(ppSurfaceEntry);
	GENHW_HW_ASSERT(pHwInterface->pSurfaceStateHeap);

	eStatus = GENOS_STATUS_UNKNOWN;
	pSSH = pHwInterface->pSurfaceStateHeap;

	if (pSSH->iCurrentSurfaceState >=
	    pHwInterface->SshSettings.iSurfaceStates) {
		GENHW_HW_ASSERTMESSAGE
		    ("Unable to allocate Surface State. Exceeds Maximum.");
		goto finish;
	}
	dwOffset = IntelGen_HwGetCurSurfaceStateBase(pSSH) +
	    (pSSH->iCurrentSurfaceState * sizeof(GENHW_SURFACE_STATE));

	iSurfaceEntry = pSSH->iCurrentSurfaceState;
	pSurfaceEntry = &pSSH->pSurfaceEntry[iSurfaceEntry];
	*pSurfaceEntry = g_cInitSurfaceStateEntry;

	pSurfaceEntry->iSurfStateID = iSurfaceEntry;
	pSurfaceEntry->Type = Type;
	pSurfaceEntry->dwSurfStateOffset = (DWORD) - 1;
	pSurfaceEntry->pSurfaceState =
	    (PGENHW_SURFACE_STATE) (pSSH->pSshBuffer + dwOffset);
	*ppSurfaceEntry = pSurfaceEntry;

	++pSSH->iCurrentSurfaceState;

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

GENOS_STATUS IntelGen_HwGetSurfaceStateEntries(PGENHW_HW_INTERFACE pHwInterface,
					       PGENHW_SURFACE pSurface,
					       PGENHW_SURFACE_STATE_PARAMS
					       pParams, PINT piNumEntries,
					       PGENHW_SURFACE_STATE_ENTRY *
					       ppSurfaceEntries)
{
	GENOS_STATUS eStatus;
	PCGENHW_PLANE_SETTING pPlane;
	GENHW_PLANE_DEFINITION PlaneDefinition;
	PGENHW_SURFACE_STATE_ENTRY pSurfaceEntry;
	DWORD dwSurfaceWidth;
	DWORD dwSurfaceHeight;
	DWORD dwUVPitch;
	INT i;
	BOOL bHalfPitchForChroma;
	BOOL bInterleaveChroma;
	BOOL bWidthInDword;
	BYTE Direction;
	WORD wUXOffset;
	WORD wUYOffset;
	WORD wVXOffset;
	WORD wVYOffset;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pSurface);
	GENHW_HW_ASSERT(pParams);
	GENHW_HW_ASSERT(piNumEntries);
	GENHW_HW_ASSERT(ppSurfaceEntries);

	eStatus = GENOS_STATUS_UNKNOWN;

	dwUVPitch = pSurface->dwPitch;
	bHalfPitchForChroma = FALSE;
	bInterleaveChroma = FALSE;
	Direction = 2;
	wUXOffset = 0;
	wUYOffset = 0;
	wVXOffset = 0;
	wVYOffset = 0;
	*piNumEntries = -1;
	PlaneDefinition = GENHW_PLANES_DEFINITION_COUNT;

	if (pSurface->Format == Format_I420 ||
	    pSurface->Format == Format_IYUV ||
	    pSurface->Format == Format_YV12 ||
	    pSurface->Format == Format_NV11) {
		dwUVPitch >>= 1;
	} else if (pSurface->Format == Format_YVU9) {
		dwUVPitch >>= 2;
	}

	if (pParams->Type == GENHW_SURFACE_TYPE_G5 ||
	    pParams->Type == GENHW_SURFACE_TYPE_G7 ||
	    pParams->Type == GENHW_SURFACE_TYPE_G8) {
		switch (pSurface->Format) {
		case Format_IMC1:
		case Format_IMC2:
		case Format_IMC3:
		case Format_IMC4:
		case Format_I420:
		case Format_IYUV:
		case Format_YVU9:
		case Format_YV12:
			PlaneDefinition = GENHW_PLANES_PL3;
			break;

		case Format_400P:
			PlaneDefinition = GENHW_PLANES_NV12;
			break;

		case Format_411P:
			PlaneDefinition = GENHW_PLANES_411P;
			break;

		case Format_411R:
			PlaneDefinition = GENHW_PLANES_411R;
			break;

		case Format_422H:
			PlaneDefinition = GENHW_PLANES_422H;
			break;

		case Format_422V:
			PlaneDefinition = GENHW_PLANES_422V;
			break;

		case Format_444P:
			PlaneDefinition = GENHW_PLANES_444P;
			break;

		case Format_RGBP:
			PlaneDefinition = GENHW_PLANES_RGBP;
			break;

		case Format_BGRP:
			PlaneDefinition = GENHW_PLANES_BGRP;
			break;

		case Format_NV12:
			if (pSurface->SurfType == SURF_OUT_RENDERTARGET ||
			    (pParams->bWidthInDword_Y
			     && pParams->bWidthInDword_UV)
			    || pParams->b2PlaneNV12NeededByKernel
			    || pHwInterface->pfnIs2PlaneNV12Needed(pHwInterface,
								   pSurface)) {
				PlaneDefinition = GENHW_PLANES_NV12_2PLANES;
			} else {
				PlaneDefinition = GENHW_PLANES_NV12;
			}
			break;

		case Format_YUYV:
		case Format_YUY2:
			PlaneDefinition = GENHW_PLANES_YUY2;
			break;

		case Format_G8R8_G8B8:
		case Format_UYVY:
			PlaneDefinition = GENHW_PLANES_UYVY;
			break;

		case Format_YVYU:
			PlaneDefinition = GENHW_PLANES_YVYU;
			break;

		case Format_VYUY:
			PlaneDefinition = GENHW_PLANES_VYUY;
			break;

		case Format_A8R8G8B8:
			PlaneDefinition = GENHW_PLANES_ARGB;
			break;

		case Format_R32U:
			PlaneDefinition = GENHW_PLANES_R32U;
			break;

		case Format_R32S:
			PlaneDefinition = GENHW_PLANES_R32S;
			break;

		case Format_R32F:
			PlaneDefinition = GENHW_PLANES_R32F;
			break;

		case Format_R8G8SN:
		case Format_V8U8:
			PlaneDefinition = GENHW_PLANES_V8U8;
			break;

		case Format_R16U:
			PlaneDefinition = GENHW_PLANES_R16U;
			break;

		case Format_R16S:
			PlaneDefinition = GENHW_PLANES_R16S;
			break;

		case Format_R8G8UN:
			PlaneDefinition = GENHW_PLANES_R8G8_UNORM;
			break;

		case Format_X8R8G8B8:
			PlaneDefinition =
			    (pParams->bRenderTarget) ? GENHW_PLANES_ARGB :
			    GENHW_PLANES_XRGB;
			break;

		case Format_A8B8G8R8:
			PlaneDefinition = GENHW_PLANES_ABGR;
			break;

		case Format_X8B8G8R8:
			PlaneDefinition =
			    (pParams->bRenderTarget) ? GENHW_PLANES_ABGR :
			    GENHW_PLANES_XBGR;
			break;

		case Format_R5G6B5:
			PlaneDefinition = GENHW_PLANES_RGB16;
			break;

		case Format_AYUV:
			PlaneDefinition = GENHW_PLANES_AYUV;
			break;

		case Format_AI44:
			PlaneDefinition = GENHW_PLANES_AI44_PALLETE_0;
			break;

		case Format_IA44:
			PlaneDefinition = GENHW_PLANES_IA44_PALLETE_0;
			break;

		case Format_P8:
			PlaneDefinition = GENHW_PLANES_P8_PALLETE_0;
			break;

		case Format_A8P8:
			PlaneDefinition = GENHW_PLANES_A8P8_PALLETE_0;
			break;

		case Format_STMM:
			PlaneDefinition = GENHW_PLANES_STMM;
			break;

		case Format_L8:
			PlaneDefinition = GENHW_PLANES_L8;
			break;

		case Format_A8:
			PlaneDefinition = GENHW_PLANES_A8;
			break;

		case Format_R8U:
			PlaneDefinition = GENHW_PLANES_R8;
			break;
		case Format_R16UN:
			PlaneDefinition = GENHW_PLANES_R16_UNORM;
			break;

		case Format_NV21:
			PlaneDefinition = GENHW_PLANES_NV21;
			break;
		case Format_L16:
		case Format_D16:
			PlaneDefinition = GENHW_PLANES_L16;
			break;

		default:
			goto finish;
		}
	}
	GENHW_HW_ASSERT(PlaneDefinition < GENHW_PLANES_DEFINITION_COUNT);
	*piNumEntries =
	    pHwInterface->pPlaneDefinitions[PlaneDefinition].NumPlanes;
	pPlane = pHwInterface->pPlaneDefinitions[PlaneDefinition].Plane;
	if (*piNumEntries == 0) {
		goto finish;
	}
	for (i = 0; i < *piNumEntries; i++, pPlane++) {
		GENHW_HW_CHK_STATUS(pHwInterface->pfnAssignSurfaceState
				    (pHwInterface, pParams->Type,
				     &pSurfaceEntry));

		ppSurfaceEntries[i] = pSurfaceEntry;

		pHwInterface->pfnAdjustBoundary(pHwInterface,
						pSurface,
						&dwSurfaceWidth,
						&dwSurfaceHeight);

		dwSurfaceHeight =
		    (dwSurfaceHeight + pPlane->ScaleHeight -
		     1) / pPlane->ScaleHeight;
		dwSurfaceWidth = dwSurfaceWidth / pPlane->ScaleWidth;

		if (pPlane->PlaneID == GENHW_U_PLANE ||
		    pPlane->PlaneID == GENHW_V_PLANE) {
			bWidthInDword = pParams->bWidthInDword_UV;
		} else {
			bWidthInDword = pParams->bWidthInDword_Y;
		}

		if (bWidthInDword) {
			dwSurfaceWidth =
			    (dwSurfaceWidth + pPlane->PixelsPerDword -
			     1) / pPlane->PixelsPerDword;
		}

		if (pParams->bVertStride) {
			dwSurfaceHeight /= 2;
			dwSurfaceHeight = MAX(dwSurfaceHeight, 1);
		}

		dwSurfaceHeight =
		    GENOS_ALIGN_FLOOR(dwSurfaceHeight, pPlane->AlignHeight);
		dwSurfaceWidth =
		    GENOS_ALIGN_FLOOR(dwSurfaceWidth, pPlane->AlignWidth);

		pSurfaceEntry->pGenHwSurface = pSurface;
		pSurfaceEntry->dwFormat = pPlane->dwFormat;
		pSurfaceEntry->dwWidth = MAX(1, dwSurfaceWidth);
		pSurfaceEntry->dwHeight = MAX(1, dwSurfaceHeight);
		pSurfaceEntry->bWidthInDword = bWidthInDword;

		if (pPlane->PlaneID == GENHW_U_PLANE ||
		    pPlane->PlaneID == GENHW_V_PLANE) {
			pSurfaceEntry->dwPitch = dwUVPitch;
		} else {
			pSurfaceEntry->dwPitch = pSurface->dwPitch;
		}

		pSurfaceEntry->YUVPlane = pPlane->PlaneID;
		pSurfaceEntry->bAVS = pPlane->bAdvanced;
		pSurfaceEntry->bRenderTarget = pParams->bRenderTarget;
		pSurfaceEntry->bVertStride = pParams->bVertStride;
		pSurfaceEntry->bVertStrideOffs = pParams->bVertStrideOffs;
		pSurfaceEntry->bTiledSurface =
		    (pSurface->TileType != GENOS_TILE_LINEAR)
		    ? TRUE : FALSE;
		pSurfaceEntry->bTileWalk = (pSurface->TileType == GENOS_TILE_Y)
		    ? GFX3DSTATE_TILEWALK_YMAJOR : GFX3DSTATE_TILEWALK_XMAJOR;

		pSurfaceEntry->bHalfPitchChroma = bHalfPitchForChroma;
		pSurfaceEntry->bInterleaveChroma = bInterleaveChroma;
		pSurfaceEntry->DirectionV = Direction & 0x7;
		pSurfaceEntry->DirectionU = Direction >> 0x3;
		pSurfaceEntry->wUXOffset = wUXOffset;
		pSurfaceEntry->wUYOffset = wUYOffset;
		pSurfaceEntry->wVXOffset = wVXOffset;
		pSurfaceEntry->wVYOffset = wVYOffset;
		pSurfaceEntry->AddressControl = pParams->AddressControl;
	}

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	return eStatus;
}

PGENHW_MEDIA_STATE IntelGen_HwAssignMediaState(PGENHW_HW_INTERFACE pHwInterface)
{
	DWORD dwWaitMs, dwWaitTag;
	PGENOS_INTERFACE pOsInterface = NULL;
	PGENHW_HW_COMMANDS pHwCommands = NULL;
	PGENHW_GSH pGSH = NULL;
	PGENHW_MEDIA_STATE pCurMediaState;

	pCurMediaState = NULL;

	if (pHwInterface) {
		pOsInterface = pHwInterface->pOsInterface;
		pHwCommands = pHwInterface->pHwCommands;
		pGSH = pHwInterface->pGeneralStateHeap;
	}
	if (pHwInterface == NULL ||
	    pOsInterface == NULL ||
	    pHwCommands == NULL || pGSH == NULL || pGSH->bGSHLocked == FALSE) {
		GENHW_HW_ASSERTMESSAGE("Invalid state.");
		goto finish;
	}
	pHwInterface->pfnRefreshSync(pHwInterface);

	pCurMediaState = &pGSH->pMediaStates[pGSH->iNextMediaState];

	if (pCurMediaState->bBusy) {
		dwWaitTag = pCurMediaState->dwSyncTag;

		for (dwWaitMs = pHwInterface->dwTimeoutMs; dwWaitMs > 0;
		     dwWaitMs--) {
			if ((INT) (pGSH->pSync[0] - dwWaitTag) > 0)
				break;
		}

		if (dwWaitMs == 0) {
			GENHW_HW_ASSERTMESSAGE
			    ("Timeout for waiting free media state.");
			pGSH->pCurMediaState = pCurMediaState = NULL;
			goto finish;
		}
	}

	pGSH->pCurMediaState = pCurMediaState;
	pGSH->iCurMediaState = pGSH->iNextMediaState;

	pGSH->iNextMediaState = (pGSH->iNextMediaState + 1) %
	    (pHwInterface->GshSettings.iMediaStateHeaps);

	pCurMediaState->dwSyncTag = pGSH->dwNextTag;
	pCurMediaState->dwSyncCount = 0;
	pCurMediaState->iCurbeOffset = 0;
	GENOS_FillMemory(pCurMediaState->piAllocation,
			 pHwInterface->GshSettings.iMediaIDs * sizeof(INT), -1);

 finish:
	return pCurMediaState;
}

GENOS_STATUS IntelGen_HwAllocateSSH(PGENHW_HW_INTERFACE pHwInterface,
				    PCGENHW_SSH_SETTINGS pSshSettings)
{
	PGENHW_SSH pSSH;
	DWORD dwSize;
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pSshSettings);
	GENHW_HW_ASSERT(pSshSettings->iBindingTables >=
			GENHW_SSH_BINDING_TABLES_MIN);
	GENHW_HW_ASSERT(pSshSettings->iBindingTables <=
			GENHW_SSH_BINDING_TABLES_MAX);
	GENHW_HW_ASSERT(pSshSettings->iSurfaceStates >=
			GENHW_SSH_SURFACE_STATES_MIN);
	GENHW_HW_ASSERT(pSshSettings->iSurfaceStates <=
			GENHW_SSH_SURFACE_STATES_MAX);
	GENHW_HW_ASSERT(pSshSettings->iSurfacesPerBT >=
			GENHW_SSH_SURFACES_PER_BT_MIN);
	GENHW_HW_ASSERT(pSshSettings->iSurfacesPerBT <=
			GENHW_SSH_SURFACES_PER_BT_MAX);

	eStatus = GENOS_STATUS_UNKNOWN;
	pSSH = NULL;

	dwSize = sizeof(GENHW_SSH);
	dwSize +=
	    pSshSettings->iSurfaceStates * sizeof(GENHW_SURFACE_STATE_ENTRY);

	pSSH = (PGENHW_SSH) GENOS_AllocAndZeroMemory(dwSize);
	GENHW_HW_CHK_NULL(pSSH);

	pSSH->pSurfaceEntry = (PGENHW_SURFACE_STATE_ENTRY) (pSSH + 1);

	pSSH->pSshBuffer = NULL;
	pSSH->dwSshSize = 0;

	GENHW_HW_CHK_STATUS(pHwInterface->pfnAllocateSshBuffer
			    (pHwInterface, pSSH));

	pHwInterface->pSurfaceStateHeap = pSSH;

	eStatus = GENOS_STATUS_SUCCESS;

 finish:
	if (eStatus != GENOS_STATUS_SUCCESS) {
		if (pSSH) {
			if (!IntelGen_OsResourceIsNull(&pSSH->OsResource)) {
				pHwInterface->pfnFreeSshBuffer(pHwInterface,
							       pSSH);
			}
			GENOS_FreeMemory(pSSH);

			pHwInterface->pSurfaceStateHeap = NULL;
		}
	}

	return eStatus;
}

VOID IntelGen_HwFreeSSH(PGENHW_HW_INTERFACE pHwInterface)
{
	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pHwInterface->pSurfaceStateHeap);

	pHwInterface->pfnFreeSshBuffer(pHwInterface,
				       pHwInterface->pSurfaceStateHeap);

	GENOS_FreeMemory(pHwInterface->pSurfaceStateHeap);

	pHwInterface->pSurfaceStateHeap = NULL;
}

VOID IntelGen_HwDestroy(PGENHW_HW_INTERFACE pHwInterface)
{
	GENHW_HW_ASSERT(pHwInterface);

	if (pHwInterface->pGeneralStateHeap) {
		pHwInterface->pfnFreeGSH(pHwInterface);
	}

	if (pHwInterface->pSurfaceStateHeap) {
		pHwInterface->pfnFreeSSH(pHwInterface);
	}
	pHwInterface->pfnFreeCommands(pHwInterface);
}

VOID IntelGen_HwSetVfeStateParams(PGENHW_HW_INTERFACE pHwInterface,
				  DWORD dwDebugCounterControl,
				  DWORD dwMaximumNumberofThreads,
				  DWORD dwCURBEAllocationSize,
				  DWORD dwURBEntryAllocationSize,
				  PGENHW_SCOREBOARD_PARAMS pScoreboardParams)
{
	UINT i;

	if (pHwInterface) {
		pHwInterface->VfeStateParams.dwDebugCounterControl =
		    dwDebugCounterControl;

		if (dwMaximumNumberofThreads == GENHW_USE_MEDIA_THREADS_MAX) {
			pHwInterface->VfeStateParams.dwMaximumNumberofThreads =
			    pHwInterface->pHwCaps->dwMaxThreads;
		} else {
			pHwInterface->VfeStateParams.dwMaximumNumberofThreads =
			    GFX_MIN(dwMaximumNumberofThreads,
				    pHwInterface->pHwCaps->dwMaxThreads);
		}

		if (pScoreboardParams) {
			pHwInterface->VfeScoreboard.ScoreboardEnable = TRUE;
			pHwInterface->VfeScoreboard.ScoreboardMask =
			    (1 << pScoreboardParams->numMask) - 1;
			pHwInterface->VfeScoreboard.ScoreboardType =
			    pScoreboardParams->ScoreboardType;
			for (i = 0; i < pScoreboardParams->numMask; i++) {
				pHwInterface->VfeScoreboard.ScoreboardDelta[i].
				    x = pScoreboardParams->ScoreboardDelta[i].x;
				pHwInterface->VfeScoreboard.ScoreboardDelta[i].
				    y = pScoreboardParams->ScoreboardDelta[i].y;
			}
		} else {
			pHwInterface->VfeScoreboard.ScoreboardEnable = TRUE;
			pHwInterface->VfeScoreboard.ScoreboardMask = 0x0;
		}

		pHwInterface->VfeStateParams.dwCURBEAllocationSize =
		    dwCURBEAllocationSize;
		pHwInterface->VfeStateParams.dwURBEntryAllocationSize =
		    dwURBEntryAllocationSize;
	}
}

VOID IntelGen_HwAddBatchBufferEndCmdBb(PGENHW_HW_INTERFACE pHwInterface,
				       PGENHW_BATCH_BUFFER pBatchBuffer)
{
	PBYTE pBuffer;
	PMI_BATCH_BUFFER_END_CMD_G5 pCmd;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pBatchBuffer);
	GENHW_HW_ASSERT((pBatchBuffer->iSize - pBatchBuffer->iCurrent) >=
			sizeof(MI_BATCH_BUFFER_END_CMD_G5));

	pBuffer = pBatchBuffer->pData + pBatchBuffer->iCurrent;
	pCmd = (PMI_BATCH_BUFFER_END_CMD_G5) pBuffer;
	*pCmd = *(pHwInterface->pHwCommands->pBatchBufferEnd);

	pBatchBuffer->iCurrent += sizeof(MI_BATCH_BUFFER_END_CMD_G5);
}

VOID IntelGen_HwSkipBatchBufferEndCmdBb(PGENHW_HW_INTERFACE pHwInterface,
					PGENHW_BATCH_BUFFER pBatchBuffer)
{
	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pBatchBuffer);
	GENHW_HW_ASSERT((pBatchBuffer->iSize - pBatchBuffer->iCurrent) >=
			sizeof(MI_BATCH_BUFFER_END_CMD_G5));

	pBatchBuffer->iCurrent += sizeof(MI_BATCH_BUFFER_END_CMD_G5);
}

GENOS_STATUS IntelGen_HwInitInterface(PGENHW_HW_INTERFACE pHwInterface,
				      PGENOS_INTERFACE pOsInterface)
{
	GENOS_STATUS eStatus;

	GENHW_HW_ASSERT(pHwInterface);
	GENHW_HW_ASSERT(pOsInterface);

	pHwInterface->pOsInterface = pOsInterface;

	pOsInterface->pfnGetPlatform(pOsInterface, &pHwInterface->Platform);

	pHwInterface->pPlaneDefinitions = g_cInitSurfacePlanes;

	pHwInterface->dwTimeoutMs = GENHW_TIMEOUT_MS_DEFAULT;

	pHwInterface->pfnDestroy = IntelGen_HwDestroy;
	pHwInterface->pfnGetOsInterface = IntelGen_HwGetOsInterface;
	pHwInterface->pfnAllocateCommands = IntelGen_HwAllocateCommands;
	pHwInterface->pfnFreeCommands = IntelGen_HwFreeCommands;
	pHwInterface->pfnInitCommandsCommon = IntelGen_HwInitCommandsCommon;

	pHwInterface->pfnAllocateSSH = IntelGen_HwAllocateSSH;
	pHwInterface->pfnFreeSSH = IntelGen_HwFreeSSH;

	pHwInterface->pfnGetSurfaceStateEntries =
	    IntelGen_HwGetSurfaceStateEntries;
	pHwInterface->pfnAssignSurfaceState = IntelGen_HwAssignSurfaceState;
	pHwInterface->pfnGetAlignUnit = IntelGen_HwGetAlignUnit;
	pHwInterface->pfnAdjustBoundary = IntelGen_HwAdjustBoundary;

	pHwInterface->pfnAllocateGSH = IntelGen_HwAllocateGSH;
	pHwInterface->pfnFreeGSH = IntelGen_HwFreeGSH;
	pHwInterface->pfnLockGSH = IntelGen_HwLockGSH;
	pHwInterface->pfnUnlockGSH = IntelGen_HwUnlockGSH;
	pHwInterface->pfnResetGSH = IntelGen_HwResetGSH;
	pHwInterface->pfnRefreshSync = IntelGen_HwRefreshSync;

	pHwInterface->pfnAllocateBB = IntelGen_HwAllocateBB;
	pHwInterface->pfnFreeBB = IntelGen_HwFreeBB;
	pHwInterface->pfnLockBB = IntelGen_HwLockBB;
	pHwInterface->pfnUnlockBB = IntelGen_HwUnlockBB;
	pHwInterface->pfnAssignMediaState = IntelGen_HwAssignMediaState;

	pHwInterface->pfnSetVfeStateParams = IntelGen_HwSetVfeStateParams;

	pHwInterface->pfnAddBatchBufferEndCmdBb =
	    IntelGen_HwAddBatchBufferEndCmdBb;
	pHwInterface->pfnSkipBatchBufferEndCmdBb =
	    IntelGen_HwSkipBatchBufferEndCmdBb;

	if (pHwInterface->pOsInterface->bUsesGfxAddress) {
	} else if (pHwInterface->pOsInterface->bUsesPatchList) {
	} else {
		eStatus = GENOS_STATUS_UNKNOWN;
		goto finish;
	}

	switch (pOsInterface->OS) {
	case GENOS_OS_LINUX:
		GENHW_HW_CHK_STATUS(IntelGen_HwInitInterface(pHwInterface));
		break;

	default:
		GENHW_HW_ASSERTMESSAGE("OS not recognized.");
		eStatus = GENOS_STATUS_UNKNOWN;
		goto finish;
	}

	if (GFX_IS_RENDERCORE(pHwInterface->Platform, IGFX_GEN7_5_CORE)) {
		IntelGen_HwInitInterface_g75(pHwInterface);
	} else if (GFX_IS_RENDERCORE(pHwInterface->Platform, IGFX_GEN8_CORE)) {
		IntelGen_HwInitInterface_g8(pHwInterface);
	} else {
		GENHW_HW_ASSERTMESSAGE("Platform not recognized.");
		eStatus = GENOS_STATUS_UNKNOWN;
	}

 finish:
	return eStatus;
}
