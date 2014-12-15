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

#ifndef __HW_INTERFACE_G75_H__
#define __HW_INTERFACE_G75_H__

#include "os_interface.h"
#include "hw_interface.h"

#define GENHW_REG_L3_CACHE_CNTLREG2_G75          0xB020
#define GENHW_REG_L3_CACHE_CNTLREG3_G75          0xB024

#define GENHW_REG_L3_CACHE_L3LRA1_G75               0x4040

extern CONST GENHW_PIPECONTROL_PARAM g_PipeControlParam_g75;
extern CONST GENHW_SSH_SETTINGS g_SSH_Settings_g75;
extern CONST GENHW_STORE_DATA_IMM_PARAM g_StoreDataImmParam_g75;
extern CONST GENHW_INDIRECT_PATCH_PARAM g_IndirectPatchParam_g75;

VOID IntelGen_HwSetupMediaObjectHeader_g75(PMEDIA_OBJECT_KA2_CMD pMediaCmd,
					   INT iMediaID, INT iSize);

INT IntelGen_HwGetMediaObjectSize_g75(PGENHW_HW_INTERFACE pHwInterface,
				      BOOL bNLAS);

GENOS_STATUS IntelGen_HwSendCurbeLoad_g75(PGENHW_HW_INTERFACE pHwInterface,
					  PGENOS_COMMAND_BUFFER pCmdBuffer);

GENOS_STATUS IntelGen_HwSendIDLoad_g75(PGENHW_HW_INTERFACE pHwInterface,
				       PGENOS_COMMAND_BUFFER pCmdBuffer);

VOID IntelGen_HwAddMediaObjectCmdBb_g75(PGENHW_HW_INTERFACE pHwInterface,
					PGENHW_BATCH_BUFFER pBatchBuffer,
					PGENHW_HW_MEDIAOBJECT_PARAM pParam);

VOID IntelGen_HwAddPipeControlCmdBb_g75(PGENHW_HW_INTERFACE pHwInterface,
					PGENHW_BATCH_BUFFER pBatchBuffer,
					PGENHW_PIPECONTROL_PARAM pParam);

VOID IntelGen_HwSkipPipeControlCmdBb_g75(PGENHW_HW_INTERFACE pHwInterface,
					 PGENHW_BATCH_BUFFER pBatchBuffer,
					 PGENHW_PIPECONTROL_PARAM pParam);

VOID IntelGen_HwConvertToNanoSeconds_g75(PGENHW_HW_INTERFACE pHwInterface,
					 UINT64 iTicks, PUINT64 piNs);

BOOL IntelGen_HwIsCSCCoeffPatchMode_g75(PGENHW_HW_INTERFACE pHwInterface);

GENOS_STATUS IntelGen_HwSendPipelineSelectCmd_g75(PGENHW_HW_INTERFACE
						  pHwInterface,
						  PGENOS_COMMAND_BUFFER
						  pCmdBuffer,
						  DWORD dwGfxPipelineSelect);

DWORD IntelGen_GetScratchSpaceSize_g8(PGENHW_HW_INTERFACE pHwInterface,
				      DWORD iPerThreadScratchSpaceSize);
BOOL IntelGen_HwGetMediaWalkerStatus_g75(PGENHW_HW_INTERFACE pHwInterface);

BOOL IntelGen_HwGetVDIWalkerStatus_g75(PGENHW_HW_INTERFACE pHwInterface,
				       PGENHW_SURFACE pSurface,
				       BOOL bDn, BOOL bDiVariance);

GENOS_STATUS IntelGen_HwSubmitBuffer_g75(PGENHW_HW_INTERFACE pHwInterface,
					 PGENHW_BATCH_BUFFER pBatchBuffer,
					 BOOL bNullRendering,
					 PGENHW_WALKER_PARAMS pWalkerParams,
					 PGENHW_GPGPU_WALKER_PARAMS
					 pGpGpuWalkerParams);

GENOS_STATUS IntelGen_HwSendMediaStateFlush_g75(PGENHW_HW_INTERFACE
						pHwInterface,
						PGENOS_COMMAND_BUFFER
						pCmdBuffer);

VOID IntelGen_HwAddGpuStatusWriteTagCmdBb_g75(PGENHW_HW_INTERFACE pHwInterface,
					      PGENHW_BATCH_BUFFER pBatchBuffer);

GENOS_STATUS IntelGen_HwSendMISetPredicateCmd_g75(PGENHW_HW_INTERFACE
						  pHwInterface,
						  PGENOS_COMMAND_BUFFER
						  pCmdBuffer,
						  DWORD PredicateEnable);

GENHW_MEDIA_WALKER_MODE IntelGen_HwSelectWalkerStateMode_g75(PGENHW_HW_INTERFACE
							     pHwInterface);

GENOS_STATUS IntelGen_HwSendWalkerState_g75(PGENHW_HW_INTERFACE pHwInterface,
					    PGENOS_COMMAND_BUFFER pCmdBuffer);

BOOL IntelGen_HwIs2PlaneNV12Needed_g75(PGENHW_HW_INTERFACE pHwInterface,
				       PGENHW_SURFACE pSurface);

DWORD IntelGen_GetScratchSpaceSize_g75(PGENHW_HW_INTERFACE pHwInterface,
				       DWORD iPerThreadScratchSpaceSize);

UINT IntelGen_HwGetMediaWalkerBlockSize_g75(PGENHW_HW_INTERFACE pHwInterface);

#endif
