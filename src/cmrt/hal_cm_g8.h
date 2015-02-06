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
#ifndef __GENHW_CM_G8__
#define __GENHW_CM_G8__

#include "gen_hw.h"
#include "os_interface.h"
#include "hw_interface.h"

#define CM_PREMP_DBG_ADDRESS_OFFSET             (0x2248)
#define CM_PREMP_DEFAULT_VALUE                  (0x00000000)
#define CM_PREMP_ON_MI_ARB_CHECK_ONLY           (0x00000100)

#define GENHW_CM_THREADS_PER_EU_BDW_GT1             CM_GEN8_HW_THREADS_PER_EU
#define GENHW_CM_THREADS_PER_EU_BDW_GT2             CM_GEN8_HW_THREADS_PER_EU
#define GENHW_CM_THREADS_PER_EU_BDW_GT3             CM_GEN8_HW_THREADS_PER_EU
#define GENHW_CM_THREADS_PER_EU_CHV                 CM_GEN8LP_HW_THREADS_PER_EU

#define GENHW_CM_EU_PER_SUBSLICE_BDW_GT1               CM_GEN8_GT1_EUS_PER_SUBSLICE
#define GENHW_CM_EU_PER_SUBSLICE_BDW_GT2               CM_GEN8_GT2_EUS_PER_SUBSLICE
#define GENHW_CM_EU_PER_SUBSLICE_BDW_GT3               CM_GEN8_GT3_EUS_PER_SUBSLICE
#define GENHW_CM_EU_PER_SUBSLICE_CHV                   CM_GEN8LP_EUS_PER_SUBSLICE

#define CM_CONFIG_CNTLREG_VALUE_G8_BDW_SLM	0x00808021
#define CM_CONFIG_CNTLREG_VALUE_G8_BDW_NONSLM	0x80000040

#define CM_CONFIG_CNTLREG_VALUE_G8_CHV_SLM	0x00410011
#define CM_CONFIG_CNTLREG_VALUE_G8_CHV_NONSLM	0x00418020

GENOS_STATUS HalCm_SubmitCommands_g8(PCM_HAL_STATE pState,
				     PGENHW_BATCH_BUFFER pBatchBuffer,
				     INT iTaskId,
				     PCM_HAL_KERNEL_PARAM * pKernels,
				     PVOID * PPCmdBuffer);

GENOS_STATUS HalCm_HwSetSurfaceMemoryObjectControl_g8(PCM_HAL_STATE pState,
						      WORD wMemObjCtl,
						      PGENHW_SURFACE_STATE_PARAMS
						      pParams);

VOID HalCm_HwSendL3CacheConfig_g8(PCM_HAL_STATE pState,
				  PGENOS_COMMAND_BUFFER pCmdBuffer);

INT HalCm_GetCurbeBlockAlignSize_g8();

GENOS_STATUS HalCm_AddMediaStateFlushBb_g8(PGENHW_HW_INTERFACE pHwInterface,
					   PGENHW_BATCH_BUFFER pBatchBuffer,
					   PMEDIA_STATE_FLUSH_CMD_G75
					   pMediaStateFlush);

GENOS_STATUS HalCm_SkipMediaStateFlushBb_g8(PGENHW_HW_INTERFACE pHwInterface,
					    PGENHW_BATCH_BUFFER pBatchBuffer);

GENOS_STATUS HalCm_GetUserDefinedThreadCountPerThreadGroup_g8(PCM_HAL_STATE
							      pState,
							      UINT *
							      pThreadsPerThreadGroup);

#endif
