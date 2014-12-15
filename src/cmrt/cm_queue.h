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
 *     Lina Sun<lina.sun@intel.com>
 */

#pragma once

#include "cm_def.h"
#include "cm_basic_queue.h"
#include "cm_array.h"
#include "cm_program.h"
#include "cm_surface_2d.h"

class CmDevice;
class CmKernel;
class CmTask;
class CmTaskInternal;
class CmEvent;
class CmThreadSpace;
class CQueue;
class CmThreadGroupSpace;

typedef struct _CM_GPUCOPY_KERNEL {
	CmKernel *pKernel;
	CM_GPUCOPY_KERNEL_ID KernelID;
	BOOL bLocked;
} CM_GPUCOPY_KERNEL, *PCM_GPUCOPY_KERNEL;

class CmQueue:public CmDynamicArray {
 public:
	static INT Create(CmDevice * pDevice, CmQueue * &pQueue);
	static INT Destroy(CmQueue * &pQueue);

	CM_RT_API INT Enqueue(CmTask * pTask, CmEvent * &pEvent,
			      const CmThreadSpace * pTS = NULL);
	CM_RT_API INT DestroyEvent(CmEvent * &pEvent);
	CM_RT_API INT EnqueueWithGroup(CmTask * pTask, CmEvent * &pEvent,
				       const CmThreadGroupSpace * pTGS = NULL);
	CM_RT_API INT EnqueueWithHints(CmTask * pTask, CmEvent * &pEvent,
				       UINT hints = 0);

	INT FlushTaskWithoutSync(bool bIfFlushBlock = FALSE);
	INT GetTaskCount(UINT & numTasks);

	INT TouchFlushedTasks(void);

	INT GetTaskHasThreadArg(CmKernel * pKernelArray[], UINT numKernels,
				BOOLEAN & threadArgExists);
	void AcquireQueueLock(void);
	void ReleaseQueueLock(void);

 protected:
	 CmQueue(CmDevice * pDevice);
	~CmQueue(void);

	INT Initialize(void);

	INT Enqueue_RT(CmKernel * pKernelArray[], const UINT uiKernelCount,
		       const UINT uiTotalThreadCount, CmEvent * &pEvent,
		       const CmThreadSpace * pTS =
		       NULL, const UINT64 uiSyncBitmap =
		       0, PCM_HAL_POWER_OPTION_PARAM pPowerOption = NULL);
	INT Enqueue_RT(CmKernel * pKernelArray[], const UINT uiKernelCount,
		       const UINT uiTotalThreadCount, CmEvent * &pEvent,
		       const CmThreadGroupSpace * pTGS =
		       NULL, const UINT64 uiSyncBitmap =
		       0, CM_HAL_PREEMPTION_MODE preemptionMode =
		       UN_PREEMPTABLE_MODE);
	INT Enqueue_RT(CmKernel * pKernelArray[], CmEvent * &pEvent,
		       UINT numTaskGenerated, BOOLEAN isLastTask, UINT hints =
		       0, PCM_HAL_POWER_OPTION_PARAM pPowerOption = NULL);

	INT QueryFlushedTasks(void);
	INT CleanQueue(void);

	INT FlushGeneralTask(CmTaskInternal * pTask);
	INT FlushGroupTask(CmTaskInternal * pTask);
	INT FlushEnqueueWithHintsTask(CmTaskInternal * pTask);

	INT UpdateSurfaceStateOnPop(CmTaskInternal * pTask);
	INT UpdateSurfaceStateOnPush(CmTaskInternal * pTask);
	void PopTaskFromFlushedQueue();

	INT CreateEvent(CmTaskInternal * pTask, BOOL bIsVisible,
			INT & taskDriverId, CmEvent * &pEvent);

	CmDevice *m_pDevice;
	CQueue m_EnqueuedTasks;
	CQueue m_FlushedTasks;

	CmDynamicArray m_EventArray;
	CSync m_CriticalSection_Event;
	CSync m_CriticalSection_HalExecute;
	CSync m_CriticalSection_Queue;

	UINT m_EventCount;

	CM_HAL_MAX_VALUES *m_pHalMaxValues;

};
