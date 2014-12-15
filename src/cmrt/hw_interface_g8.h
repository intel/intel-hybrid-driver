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

#ifndef __HW_INTERFACE_G8_H__
#define __HW_INTERFACE_G8_H__

#define GENHW_REG_L3_CACHE_CNTLREG_G8           0x7034

extern CONST GENHW_GSH_SETTINGS g_GSH_Settings_g8;
extern CONST GENHW_SSH_SETTINGS g_SSH_Settings_g8;

GENOS_STATUS IntelGen_HwAssignBindingTable_g8(PGENHW_HW_INTERFACE pHwInterface,
					      PINT piBindingTable);

VOID IntelGen_HwInitInterface_g8(PGENHW_HW_INTERFACE pHwInterface);

VOID IntelGen_HwInitInterfaceDescriptor_g8(PGENHW_HW_INTERFACE pHwInterface,
					   PBYTE pBase,
					   DWORD dwBase, DWORD dwOffsetID);

VOID IntelGen_HwSetupInterfaceDescriptor_g8(PGENHW_HW_INTERFACE pHwInterface,
					    PGENHW_MEDIA_STATE pMediaState,
					    PGENHW_KRN_ALLOCATION
					    pKernelAllocation,
					    PGENHW_INTERFACE_DESCRIPTOR_PARAMS
					    pInterfaceDescriptorParams,
					    PGENHW_GPGPU_WALKER_PARAMS
					    pGpGpuWalkerParams);

INT IntelGen_HwLoadCurbeData_g8(PGENHW_HW_INTERFACE pHwInterface,
				PGENHW_MEDIA_STATE pCurMediaState,
				PVOID pData, INT iSize);

DWORD IntelGen_HwGetSurfaceMemoryObjectControl_g8(PGENHW_HW_INTERFACE
						  pHwInterface,
						  PGENHW_SURFACE_STATE_PARAMS
						  pParams);

GENOS_STATUS IntelGen_HwSendGpGpuWalkerState_g8(PGENHW_HW_INTERFACE
						pHwInterface,
						PGENOS_COMMAND_BUFFER
						pCmdBuffer,
						PGENHW_GPGPU_WALKER_PARAMS
						pGpGpuWalkerParams);

GENOS_STATUS IntelGen_HwSetupBufferSurfaceState_g8(PGENHW_HW_INTERFACE
						   pHwInterface,
						   PGENHW_SURFACE pSurface,
						   PGENHW_SURFACE_STATE_PARAMS
						   pParams,
						   PGENHW_SURFACE_STATE_ENTRY *
						   ppSurfaceEntry);

#endif
