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

#include "cm_queue.h"
#include "cm_device.h"
#include "cm_event.h"
#include "cm_task.h"
#include "cm_task_internal.h"
#include "cm_thread_space.h"
#include "cm_kernel.h"
#include "cm_kernel_data.h"
#include "cm_buffer.h"
#include "cm_group_space.h"
#include "cm_def.h"
#include "hal_cm.h"
#include "cm_surface_manager.h"
#include <sys/time.h>

INT CmQueue::Create(CmDevice * pDevice, CmQueue * &pQueue)
{
	INT result = CM_SUCCESS;
	pQueue = new(std::nothrow) CmQueue(pDevice);
	if (pQueue) {
		result = pQueue->Initialize();
		if (result != CM_SUCCESS) {
			CmQueue::Destroy(pQueue);
		}
	} else {
		CM_ASSERT(0);
		result = CM_OUT_OF_HOST_MEMORY;
	}
	return result;
}

INT CmQueue::Destroy(CmQueue * &pQueue)
{
	if (pQueue == NULL) {
		return CM_FAILURE;
	}

	UINT result = pQueue->CleanQueue();
	CmSafeDelete(pQueue);

	return result;
}

 CmQueue::CmQueue(CmDevice * pDevice):
m_pDevice(pDevice),
m_EventArray(CM_INIT_EVENT_COUNT), m_EventCount(0), m_pHalMaxValues(NULL)
{

}

CmQueue::~CmQueue(void)
{
	UINT EventReleaseTimes = 0;

	m_FlushedTasks.DeleteFreePool();

	UINT EventArrayUsedSize = m_EventArray.GetMaxSize();
	for (UINT i = 0; i < EventArrayUsedSize; i++) {
		CmEvent *pEvent = (CmEvent *) m_EventArray.GetElement(i);
		EventReleaseTimes = 0;
		while (pEvent) {
			if (EventReleaseTimes > 2) {
				CM_ASSERT(0);
				break;
			}
			CmEvent::Destroy(pEvent);
			EventReleaseTimes++;
		}
	}
	m_EventArray.Delete();

}

INT CmQueue::Initialize(void)
{
	CM_HAL_MAX_VALUES_EX *pHalMaxValuesEx = NULL;
	m_pDevice->GetHalMaxValues(m_pHalMaxValues, pHalMaxValuesEx);
	return CM_SUCCESS;
}

INT CmQueue::GetTaskHasThreadArg(CmKernel * pKernelArray[], UINT numKernels,
				 BOOLEAN & threadArgExists)
{
	threadArgExists = FALSE;

	for (UINT iKrn = 0; iKrn < numKernels; iKrn++) {
		if (!pKernelArray[iKrn]) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}

		if (pKernelArray[iKrn]->IsThreadArgExisted()) {
			threadArgExists = TRUE;
			break;
		}
	}

	return CM_SUCCESS;
}

CM_RT_API INT
    CmQueue::Enqueue(CmTask * pKernelArray,
		     CmEvent * &pEvent, const CmThreadSpace * pTS)
{
	INT result;

	if (pKernelArray == NULL) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	UINT KernelCount = 0;
	KernelCount = pKernelArray->GetKernelCount();
	if (KernelCount == 0) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	if (KernelCount > m_pHalMaxValues->iMaxKernelsPerTask) {
		CM_ASSERT(0);
		return CM_EXCEED_MAX_KERNEL_PER_ENQUEUE;
	}

	if (pTS && pTS->IsThreadAssociated()) {
		if (pTS->GetNeedSetKernelPointer()
		    && pTS->KernelPointerIsNULL()) {
			CmKernel *pTmp = NULL;
			pTmp = pKernelArray->GetKernelPointer(0);
			pTS->SetKernelPointer(pTmp);
		}
	}

	typedef CmKernel *pCmKernel;
	CmKernel **pTmp = new(std::nothrow) pCmKernel[KernelCount + 1];
	if (pTmp == NULL) {
		CM_ASSERT(0);
		return CM_OUT_OF_HOST_MEMORY;
	}

	UINT totalThreadNumber = 0;
	for (UINT i = 0; i < KernelCount; i++) {
		pTmp[i] = pKernelArray->GetKernelPointer(i);

		UINT singleThreadNumber = 0;
		pTmp[i]->GetThreadCount(singleThreadNumber);
		totalThreadNumber += singleThreadNumber;
	}
	pTmp[KernelCount] = NULL;

	result =
	    Enqueue_RT(pTmp, KernelCount, totalThreadNumber, pEvent, pTS,
		       pKernelArray->GetSyncBitmap(),
		       pKernelArray->GetPowerOption());

	if (pEvent) {
		pEvent->SetKernelNames(pKernelArray,
				       const_cast < CmThreadSpace * >(pTS),
				       NULL);
	}

	CmSafeDeleteArray(pTmp);

	return result;
}

INT CmQueue::Enqueue_RT(CmKernel * pKernelArray[],
			const UINT uiKernelCount,
			const UINT uiTotalThreadCount,
			CmEvent * &pEvent,
			const CmThreadSpace * pTS,
			UINT64 uiSyncBitmap,
			PCM_HAL_POWER_OPTION_PARAM pPowerOption)
{

	if (pKernelArray == NULL) {
		CM_ASSERTMESSAGE("Kernel array is NULL.");
		return CM_INVALID_ARG_VALUE;
	}

	if (uiKernelCount == 0) {
		CM_ASSERTMESSAGE("There are no valid kernels.");
		return CM_INVALID_ARG_VALUE;
	}

	BOOL bIsEventVisible = (pEvent == CM_NO_EVENT) ? FALSE : TRUE;

	CmTaskInternal *pTask = NULL;
	INT result = CmTaskInternal::Create(uiKernelCount, uiTotalThreadCount,
					    pKernelArray,
					    pTS, m_pDevice, uiSyncBitmap,
					    pTask);
	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}

	m_CriticalSection_Queue.Acquire();

	if (!m_EnqueuedTasks.Push(pTask)) {
		m_CriticalSection_Queue.Release();
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	INT taskDriverId = -1;

	result = CreateEvent(pTask, bIsEventVisible, taskDriverId, pEvent);
	if (result != CM_SUCCESS) {
		m_CriticalSection_Queue.Release();
		CM_ASSERT(0);
		return result;
	}

	pTask->SetPowerOption(pPowerOption);
	UpdateSurfaceStateOnPush(pTask);
	result = FlushTaskWithoutSync();

	m_CriticalSection_Queue.Release();

	return result;
}

INT CmQueue::Enqueue_RT(CmKernel * pKernelArray[],
			const UINT uiKernelCount,
			const UINT uiTotalThreadCount,
			CmEvent * &pEvent,
			const CmThreadGroupSpace * pTGS,
			UINT64 uiSyncBitmap,
			CM_HAL_PREEMPTION_MODE preemptionMode)
{
	if (pKernelArray == NULL) {
		CM_ASSERTMESSAGE("Kernel array is NULL.");
		return CM_INVALID_ARG_VALUE;
	}

	if (uiKernelCount == 0) {
		CM_ASSERTMESSAGE("There are no valid kernels.");
		return CM_INVALID_ARG_VALUE;
	}

	CmTaskInternal *pTask = NULL;
	INT result = CmTaskInternal::Create(uiKernelCount, uiTotalThreadCount,
					    pKernelArray,
					    pTGS, m_pDevice, uiSyncBitmap,
					    pTask);
	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}

	m_CriticalSection_Queue.Acquire();

	pTask->SetPreemptionMode(preemptionMode);

	if (!m_EnqueuedTasks.Push(pTask)) {
		m_CriticalSection_Queue.Release();
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	INT taskDriverId = -1;

	result =
	    CreateEvent(pTask, !(pEvent == CM_NO_EVENT), taskDriverId, pEvent);
	if (result != CM_SUCCESS) {
		m_CriticalSection_Queue.Release();
		CM_ASSERT(0);
		return result;
	}

	UpdateSurfaceStateOnPush(pTask);

	result = FlushTaskWithoutSync();
	m_CriticalSection_Queue.Release();

	return result;
}

INT CmQueue::Enqueue_RT(CmKernel * pKernelArray[],
			CmEvent * &pEvent,
			UINT numTasksGenerated,
			BOOLEAN isLastTask,
			UINT hints, PCM_HAL_POWER_OPTION_PARAM pPowerOption)
{
	INT result = CM_FAILURE;
	UINT kernelCount = 0;
	CmTaskInternal *pTask = NULL;
	INT taskDriverId = -1;
	BOOL bIsEventVisible = (pEvent == CM_NO_EVENT) ? FALSE : TRUE;
	BOOLEAN threadArgExists = FALSE;

	if (pKernelArray == NULL) {
		CM_ASSERTMESSAGE("Kernel array is NULL.");
		return CM_INVALID_ARG_VALUE;
	}
	while (pKernelArray[kernelCount]) {
		kernelCount++;
	}

	if (kernelCount < CM_MINIMUM_NUM_KERNELS_ENQWHINTS) {
		CM_ASSERTMESSAGE
		    ("EnqueueWithHints requires at least 2 kernels.");
		return CM_FAILURE;
	}

	UINT totalThreadCount = 0;
	for (UINT i = 0; i < kernelCount; i++) {
		UINT threadCount = 0;
		pKernelArray[i]->GetThreadCount(threadCount);
		totalThreadCount += threadCount;
	}

	if (GetTaskHasThreadArg(pKernelArray, kernelCount, threadArgExists) !=
	    CM_SUCCESS) {
		CM_ASSERTMESSAGE
		    ("Error checking if Task has any thread arguments.");
		return CM_FAILURE;
	}

	if (!threadArgExists) {
		if (totalThreadCount >
		    m_pHalMaxValues->iMaxUserThreadsPerTaskNoThreadArg) {
			CM_ASSERTMESSAGE
			    ("Maximum number of threads per task exceeded.");
			return CM_EXCEED_MAX_THREAD_AMOUNT_PER_ENQUEUE;
		}
	} else {
		if (totalThreadCount > m_pHalMaxValues->iMaxUserThreadsPerTask) {
			CM_ASSERTMESSAGE
			    ("Maximum number of threads per task exceeded.");
			return CM_EXCEED_MAX_THREAD_AMOUNT_PER_ENQUEUE;
		}
	}

	result =
	    CmTaskInternal::Create(kernelCount, totalThreadCount, pKernelArray,
				   pTask, numTasksGenerated, isLastTask, hints,
				   m_pDevice);
	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}

	m_CriticalSection_Queue.Acquire();
	if (!m_EnqueuedTasks.Push(pTask)) {
		m_CriticalSection_Queue.Release();
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	result = CreateEvent(pTask, bIsEventVisible, taskDriverId, pEvent);
	if (result != CM_SUCCESS) {
		m_CriticalSection_Queue.Release();
		CM_ASSERT(0);
		return result;
	}

	for (UINT i = 0; i < kernelCount; ++i) {
		CmKernel *pKernel = NULL;
		pTask->GetKernel(i, pKernel);
		if (pKernel != NULL) {
			pKernel->SetAdjustedYCoord(0);
		}
	}

	pTask->SetPowerOption(pPowerOption);
	UpdateSurfaceStateOnPush(pTask);

	result = FlushTaskWithoutSync();
	m_CriticalSection_Queue.Release();

	return result;
}

CM_RT_API INT
    CmQueue::EnqueueWithGroup(CmTask * pTask, CmEvent * &pEvent,
			      const CmThreadGroupSpace * pTGS)
{
	INT result;

	if (pTask == NULL) {
		CM_ASSERTMESSAGE("Kernel array is NULL.");
		return CM_INVALID_ARG_VALUE;
	}

	UINT count = 0;
	count = pTask->GetKernelCount();

	if (count == 0) {
		CM_ASSERTMESSAGE("There are no valid kernels.");
		return CM_FAILURE;
	}

	typedef CmKernel *pCmKernel;
	CmKernel **pTmp = new(std::nothrow) pCmKernel[count + 1];
	if (pTmp == NULL) {
		CM_ASSERT(0);
		return CM_OUT_OF_HOST_MEMORY;
	}

	UINT totalThreadNumber = 0;
	for (UINT i = 0; i < count; i++) {
		UINT singleThreadNumber = 0;
		pTmp[i] = pTask->GetKernelPointer(i);

		if (pTmp[i]->IsThreadArgExisted()) {
			CM_ASSERTMESSAGE
			    ("No thread Args allowed when using group space");
			CmSafeDeleteArray(pTmp);
			return CM_THREAD_ARG_NOT_ALLOWED;
		}

		pTmp[i]->GetThreadCount(singleThreadNumber);
		totalThreadNumber += singleThreadNumber;
	}
	pTmp[count] = NULL;

	result =
	    Enqueue_RT(pTmp, count, totalThreadNumber, pEvent, pTGS,
		       pTask->GetSyncBitmap(), pTask->GetPreemptionMode());

	if (pEvent) {
		pEvent->SetKernelNames(pTask, NULL,
				       const_cast <
				       CmThreadGroupSpace * >(pTGS));
	}

	CmSafeDeleteArray(pTmp);

	return result;
}

CM_RT_API INT
    CmQueue::EnqueueWithHints(CmTask * pKernelArray,
			      CmEvent * &pEvent, UINT hints)
{
	INT hr = CM_FAILURE;
	UINT count = 0;
	UINT index = 0;
	CmKernel **pKernels = NULL;
	UINT numTasks = 0;
	BOOLEAN splitTask = FALSE;
	BOOLEAN lastTask = FALSE;
	UINT numTasksGenerated = 0;

	CMCHK_NULL_RETURN(pKernelArray, CM_INVALID_ARG_VALUE);

	count = pKernelArray->GetKernelCount();
	if (count == 0) {
		CM_ASSERT(0);
		hr = CM_FAILURE;
		goto finish;
	}

	if (count > m_pHalMaxValues->iMaxKernelsPerTask) {
		CM_ASSERT(0);
		hr = CM_EXCEED_MAX_KERNEL_PER_ENQUEUE;
		goto finish;
	}

	numTasks =
	    (hints & CM_HINTS_MASK_NUM_TASKS) >> CM_HINTS_NUM_BITS_TASK_POS;
	if (numTasks > 1) {
		splitTask = TRUE;
	}

	pKernels = new(std::nothrow) CmKernel *[count + 1];
	CMCHK_NULL(pKernels);

	do {
		for (index = 0; index < count; ++index) {
			pKernels[index] = pKernelArray->GetKernelPointer(index);
		}

		pKernels[count] = NULL;

		if (splitTask) {
			if (numTasksGenerated == (numTasks - 1)) {
				lastTask = TRUE;
			}
		} else {
			lastTask = TRUE;
		}

		CMCHK_HR(Enqueue_RT
			 (pKernels, pEvent, numTasksGenerated, lastTask, hints,
			  pKernelArray->GetPowerOption()));

		numTasksGenerated++;

	}
	while (numTasksGenerated < numTasks);

 finish:
	CmSafeDeleteArray(pKernels);

	return hr;
}

INT CmQueue::UpdateSurfaceStateOnPop(CmTaskInternal * pTask)
{
	CmSurfaceManager *pSurfaceMgr = NULL;
	INT *pSurfState = NULL;
	BOOL *surfArray = NULL;

	m_pDevice->GetSurfaceManager(pSurfaceMgr);
	if (!pSurfaceMgr) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	UINT poolSize = pSurfaceMgr->GetSurfacePoolSize();
	pSurfaceMgr->GetSurfaceState(pSurfState);

	pTask->GetTaskSurfaces(surfArray);
	for (UINT i = 0; i < poolSize; i++) {
		if (surfArray[i]) {
			pSurfState[i]--;
		}
	}

	return CM_SUCCESS;
}

INT CmQueue::UpdateSurfaceStateOnPush(CmTaskInternal * pTask)
{
	INT *pSurfState = NULL;
	BOOL *surfArray = NULL;
	CmSurfaceManager *pSurfaceMgr = NULL;
	UINT freeSurfNum = 0;

	m_pDevice->GetSurfaceManager(pSurfaceMgr);
	if (!pSurfaceMgr) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	CSync *pSurfaceLock = m_pDevice->GetSurfaceCreationLock();
	pSurfaceLock->Acquire();

	UINT poolSize = pSurfaceMgr->GetSurfacePoolSize();
	pSurfaceMgr->GetSurfaceState(pSurfState);

	pTask->GetTaskSurfaces(surfArray);
	for (UINT i = 0; i < poolSize; i++) {
		if (surfArray[i]) {
			pSurfState[i]++;
		}
	}

	pSurfaceMgr->DestroySurfaceInPool(freeSurfNum, DELAYED_DESTROY);

	pSurfaceLock->Release();

	return CM_SUCCESS;
}

void CmQueue::PopTaskFromFlushedQueue()
{
	CmTaskInternal *pTopTask = (CmTaskInternal *) m_FlushedTasks.Pop();

	UpdateSurfaceStateOnPop(pTopTask);

	CmTaskInternal::Destroy(pTopTask);

	return;
}

INT CmQueue::TouchFlushedTasks(void)
{
	INT hr = CM_SUCCESS;

	m_CriticalSection_Queue.Acquire();
	if (m_FlushedTasks.IsEmpty()) {
		if (!m_EnqueuedTasks.IsEmpty()) {
			hr = FlushTaskWithoutSync();
			if (FAILED(hr)) {
				m_CriticalSection_Queue.Release();
				return hr;
			}
		} else {
			m_CriticalSection_Queue.Release();
			return CM_FAILURE;
		}
	}

	hr = QueryFlushedTasks();
	m_CriticalSection_Queue.Release();
	return hr;
}

INT CmQueue::QueryFlushedTasks(void)
{
	INT hr = CM_SUCCESS;

	while (!m_FlushedTasks.IsEmpty()) {
		CmTaskInternal *pTask = (CmTaskInternal *) m_FlushedTasks.Top();
		CMCHK_NULL(pTask);

		CM_STATUS status = CM_STATUS_FLUSHED;
		pTask->GetTaskStatus(status);
		if (status == CM_STATUS_FINISHED) {
			PopTaskFromFlushedQueue();
		} else {
			if (status == CM_STATUS_STARTED) {
				PCM_CONTEXT pCmData =
				    (PCM_CONTEXT) m_pDevice->GetAccelData();
				if (pCmData->pCmHalState->
				    pHwInterface->bMediaReset) {
					CmTaskInternal *pNextTask =
					    (CmTaskInternal *)
					    m_FlushedTasks.GetNext(pTask);
					if (pNextTask == NULL) {
						continue;
					}

					CM_STATUS nextTaskStatus =
					    CM_STATUS_FLUSHED;
					pNextTask->GetTaskStatus
					    (nextTaskStatus);

					if (nextTaskStatus == CM_STATUS_STARTED
					    || nextTaskStatus ==
					    CM_STATUS_FINISHED) {
						pTask->GetTaskStatus(status);
						if (status == CM_STATUS_STARTED) {
							INT iTaskId;
							CmEvent *pTopTaskEvent;
							pTask->GetTaskEvent
							    (pTopTaskEvent);
							CMCHK_NULL
							    (pTopTaskEvent);

							pTopTaskEvent->GetTaskDriverId
							    (iTaskId);
							pCmData->
							    pCmHalState->pTaskStatusTable
							    [iTaskId] =
							    CM_INVALID_INDEX;

							PopTaskFromFlushedQueue
							    ();
						}
					}
				}
			}
			break;
		}
	}

 finish:

	return hr;
}

CM_RT_API INT CmQueue::DestroyEvent(CmEvent * &pEvent)
{
	if (pEvent == NULL) {
		return CM_FAILURE;
	}

	UINT index = 0;
	pEvent->GetIndex(index);
	CM_ASSERT(m_EventArray.GetElement(index) == pEvent);

	INT status = CmEvent::Destroy(pEvent);
	if (status == CM_SUCCESS && pEvent == NULL) {
		m_EventArray.SetElement(index, NULL);
		pEvent = NULL;
	}
	return status;
}

INT CmQueue::CleanQueue(void)
{

	INT status = CM_SUCCESS;
	m_CriticalSection_Queue.Acquire();

	if (!m_EnqueuedTasks.IsEmpty()) {
		FlushTaskWithoutSync(TRUE);
	}
	CM_ASSERT(m_EnqueuedTasks.IsEmpty());
	m_EnqueuedTasks.DeleteFreePool();

	struct timeval start;
	gettimeofday(&start, NULL);
	UINT64 timeout_usec;
	timeout_usec = CM_MAX_TIMEOUT * m_FlushedTasks.GetCount() * 1000000;

	while (!m_FlushedTasks.IsEmpty() && status != CM_EXCEED_MAX_TIMEOUT) {
		QueryFlushedTasks();

		struct timeval current;
		gettimeofday(&current, NULL);
		UINT64 timeuse_usec;
		timeuse_usec =
		    1000000 * (current.tv_sec - start.tv_sec) +
		    current.tv_usec - start.tv_usec;
		if (timeuse_usec > timeout_usec)
			status = CM_EXCEED_MAX_TIMEOUT;
	}

	m_FlushedTasks.DeleteFreePool();
	m_CriticalSection_Queue.Release();

	return status;
}

INT CmQueue::GetTaskCount(UINT & numTasks)
{
	numTasks = m_EnqueuedTasks.GetCount() + m_FlushedTasks.GetCount();
	return CM_SUCCESS;
}

INT CmQueue::FlushGeneralTask(CmTaskInternal * pTask)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	CM_HAL_EXEC_TASK_PARAM param;
	CmKernelData *pKernelData = NULL;
	UINT kernelDataSize = 0;
	PCM_CONTEXT pCmData = NULL;
	CmEvent *pEvent = NULL;
	UINT totalThreadCount = 0;
	UINT count = 0;
	PCM_HAL_KERNEL_PARAM pTempData = NULL;

	CmSafeMemSet(&param, 0, sizeof(CM_HAL_EXEC_TASK_PARAM));

	pTask->GetKernelCount(count);
	param.iNumKernels = count;

	param.pKernels = new(std::nothrow) PCM_HAL_KERNEL_PARAM[count];
	param.piKernelSizes = new(std::nothrow) UINT[count];
	param.piKernelCurbeOffset = new(std::nothrow) UINT[count];

	CMCHK_NULL_RETURN(param.pKernels, CM_OUT_OF_HOST_MEMORY);
	CMCHK_NULL_RETURN(param.piKernelSizes, CM_OUT_OF_HOST_MEMORY);
	CMCHK_NULL_RETURN(param.piKernelCurbeOffset, CM_OUT_OF_HOST_MEMORY);

	for (UINT i = 0; i < count; i++) {
		pTask->GetKernelData(i, pKernelData);
		CMCHK_NULL(pKernelData);

		pTask->GetKernelDataSize(i, kernelDataSize);
		if (kernelDataSize == 0) {
			CM_ASSERT(0);
			hr = CM_FAILURE;
			goto finish;
		}

		pTempData = pKernelData->GetHalCmKernelData();

		param.pKernels[i] = pTempData;
		param.piKernelSizes[i] = kernelDataSize;
		param.piKernelCurbeOffset[i] = pTask->GetKernelCurbeOffset(i);
		param.bGlobalSurfaceUsed |= pTempData->bGlobalSurfaceUsed;
		param.bKernelDebugEnabled |= pTempData->bKernelDebugEnabled;
	}

	pTask->GetTotalThreadCount(totalThreadCount);

	param.threadSpaceWidth =
	    (totalThreadCount >
	     CM_MAX_THREADSPACE_WIDTH) ? CM_MAX_THREADSPACE_WIDTH :
	    totalThreadCount;
	if (totalThreadCount % CM_MAX_THREADSPACE_WIDTH) {
		param.threadSpaceHeight =
		    totalThreadCount / CM_MAX_THREADSPACE_WIDTH + 1;
	} else {
		param.threadSpaceHeight =
		    totalThreadCount / CM_MAX_THREADSPACE_WIDTH;
	}
	param.DependencyPattern = CM_DEPENDENCY_NONE;

	if (pTask->IsThreadSpaceCreated()) {
		if (pTask->IsThreadCoordinatesExisted()) {
			param.ppThreadCoordinates =
			    new(std::nothrow) PCM_HAL_SCOREBOARD_XY[count];
			param.ppDependencyMasks =
			    new(std::nothrow) PCM_HAL_MASK_AND_RESET[count];
			CMCHK_NULL_RETURN(param.ppThreadCoordinates,
					  CM_OUT_OF_HOST_MEMORY);
			CMCHK_NULL_RETURN(param.ppDependencyMasks,
					  CM_OUT_OF_HOST_MEMORY);
			for (UINT i = 0; i < count; i++) {
				void *pKernelCoordinates = NULL;
				void *pDependencyMasks = NULL;
				pTask->GetKernelCoordinates(i,
							    pKernelCoordinates);
				pTask->GetKernelDependencyMasks(i,
								pDependencyMasks);
				param.ppThreadCoordinates[i] =
				    (PCM_HAL_SCOREBOARD_XY) pKernelCoordinates;
				param.ppDependencyMasks[i] =
				    (PCM_HAL_MASK_AND_RESET) pDependencyMasks;
			}
		} else {
			param.ppThreadCoordinates = NULL;
		}

		pTask->GetDependencyPattern(param.DependencyPattern);

		pTask->GetThreadSpaceSize(param.threadSpaceWidth,
					  param.threadSpaceHeight);

		pTask->GetWalkingPattern(param.WalkingPattern);

		if (pTask->CheckWalkingParametersSet()) {
			param.walkingParamsValid = 1;
			CMCHK_HR(pTask->GetWalkingParameters
				 (param.walkingParams));
		} else {
			param.walkingParamsValid = 0;
		}

		if (pTask->CheckDependencyVectorsSet()) {
			param.dependencyVectorsValid = 1;
			CMCHK_HR(pTask->GetDependencyVectors
				 (param.dependencyVectors));
		} else {
			param.dependencyVectorsValid = 0;
		}
	}

	pTask->GetColorCountMinusOne(param.ColorCountMinusOne);

	param.uiSyncBitmap = pTask->GetSyncBitmap();

	pCmData = (PCM_CONTEXT) m_pDevice->GetAccelData();

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnSetPowerOption(pCmData->pCmHalState,
							 pTask->GetPowerOption
							 ()));

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnExecuteTask(pCmData->pCmHalState,
						      &param));

	if (param.iTaskIdOut < 0) {
		CM_ASSERT(0);
		hr = CM_FAILURE;
		goto finish;
	}

	pTask->GetTaskEvent(pEvent);
	CMCHK_NULL(pEvent);
	CMCHK_HR(pEvent->SetTaskDriverId(param.iTaskIdOut));
	CMCHK_HR(pEvent->SetTaskOsData(param.OsData));
	CMCHK_HR(pTask->ReleaseKernel());

 finish:
	CmSafeDeleteArray(param.pKernels);
	CmSafeDeleteArray(param.piKernelSizes);
	CmSafeDeleteArray(param.ppThreadCoordinates);
	CmSafeDeleteArray(param.ppDependencyMasks);
	CmSafeDeleteArray(param.piKernelCurbeOffset);

	return hr;
}

INT CmQueue::FlushGroupTask(CmTaskInternal * pTask)
{
	CM_RETURN_CODE hr = CM_SUCCESS;

	CM_HAL_EXEC_TASK_GROUP_PARAM param;
	CmKernelData *pKernelData = NULL;
	UINT kernelDataSize = 0;
	UINT count = 0;
	PCM_CONTEXT pCmData = NULL;
	CmEvent *pEvent = NULL;
	PCM_HAL_KERNEL_PARAM pTempData = NULL;

	CmSafeMemSet(&param, 0, sizeof(CM_HAL_EXEC_TASK_GROUP_PARAM));

	pTask->GetKernelCount(count);
	param.iNumKernels = count;

	param.pKernels = new(std::nothrow) PCM_HAL_KERNEL_PARAM[count];
	param.piKernelSizes = new(std::nothrow) UINT[count];
	param.piKernelCurbeOffset = new(std::nothrow) UINT[count];
	param.iPreemptionMode = pTask->GetPreemptionMode();

	CMCHK_NULL(param.pKernels);
	CMCHK_NULL(param.piKernelSizes);
	CMCHK_NULL(param.piKernelCurbeOffset);

	for (UINT i = 0; i < count; i++) {
		pTask->GetKernelData(i, pKernelData);
		CMCHK_NULL(pKernelData);

		pTask->GetKernelDataSize(i, kernelDataSize);
		if (kernelDataSize == 0) {
			CM_ASSERT(0);
			hr = CM_FAILURE;
			goto finish;
		}

		pTempData = pKernelData->GetHalCmKernelData();

		param.pKernels[i] = pTempData;
		param.piKernelSizes[i] = kernelDataSize;
		param.piKernelCurbeOffset[i] = pTask->GetKernelCurbeOffset(i);
		param.bGlobalSurfaceUsed |= pTempData->bGlobalSurfaceUsed;
		param.bKernelDebugEnabled |= pTempData->bKernelDebugEnabled;
	}

	pTask->GetSLMSize(param.iSLMSize);
	if (param.iSLMSize > MAX_SLM_SIZE_PER_GROUP_IN_1K) {
		CM_ASSERT(0);
		hr = CM_EXCEED_MAX_SLM_SIZE;
		goto finish;
	}

	if (pTask->IsThreadGroupSpaceCreated()) {
		pTask->GetThreadGroupSpaceSize(param.threadSpaceWidth,
					       param.threadSpaceHeight,
					       param.groupSpaceWidth,
					       param.groupSpaceHeight);
	}

	param.uiSyncBitmap = pTask->GetSyncBitmap();

	pCmData = (PCM_CONTEXT) m_pDevice->GetAccelData();
	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->pfnExecuteGroupTask
				       (pCmData->pCmHalState, &param));

	if (param.iTaskIdOut < 0) {
		CM_ASSERT(0);
		hr = CM_FAILURE;
		goto finish;
	}

	pTask->GetTaskEvent(pEvent);
	CMCHK_NULL(pEvent);
	CMCHK_HR(pEvent->SetTaskDriverId(param.iTaskIdOut));
	CMCHK_HR(pEvent->SetTaskOsData(param.OsData));
	CMCHK_HR(pTask->ReleaseKernel());

 finish:
	CmSafeDeleteArray(param.pKernels);
	CmSafeDeleteArray(param.piKernelSizes);
	CmSafeDeleteArray(param.piKernelCurbeOffset);

	return hr;
}

INT CmQueue::FlushEnqueueWithHintsTask(CmTaskInternal * pTask)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	CM_HAL_EXEC_HINTS_TASK_PARAM param;
	PCM_CONTEXT pCmData = NULL;
	CmKernelData *pKernelData = NULL;
	UINT kernelDataSize = 0;
	UINT count = 0;
	CmEvent *pEvent = NULL;
	PCM_HAL_KERNEL_PARAM pTempData = NULL;

	CmSafeMemSet(&param, 0, sizeof(CM_HAL_EXEC_HINTS_TASK_PARAM));

	pTask->GetKernelCount(count);
	param.iNumKernels = count;

	param.pKernels = new(std::nothrow) PCM_HAL_KERNEL_PARAM[count];
	param.piKernelSizes = new(std::nothrow) UINT[count];
	param.piKernelCurbeOffset = new(std::nothrow) UINT[count];

	CMCHK_NULL(param.pKernels);
	CMCHK_NULL(param.piKernelSizes);
	CMCHK_NULL(param.piKernelCurbeOffset);

	pTask->GetHints(param.iHints);
	pTask->GetNumTasksGenerated(param.iNumTasksGenerated);
	pTask->GetLastTask(param.isLastTask);

	for (UINT i = 0; i < count; i++) {
		pTask->GetKernelData(i, pKernelData);
		CMCHK_NULL(pKernelData);

		pTask->GetKernelDataSize(i, kernelDataSize);
		if (kernelDataSize == 0) {
			CM_ASSERT(0);
			hr = CM_FAILURE;
			goto finish;
		}

		pTempData = pKernelData->GetHalCmKernelData();

		param.pKernels[i] = pTempData;
		param.piKernelSizes[i] = kernelDataSize;
		param.piKernelCurbeOffset[i] = pTask->GetKernelCurbeOffset(i);
	}

	pCmData = (PCM_CONTEXT) m_pDevice->GetAccelData();
	CMCHK_NULL(pCmData);

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnSetPowerOption(pCmData->pCmHalState,
							 pTask->GetPowerOption
							 ()));

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->pfnExecuteHintsTask
				       (pCmData->pCmHalState, &param));

	if (param.iTaskIdOut < 0) {
		CM_ASSERT(0);
		hr = CM_FAILURE;
		goto finish;
	}

	pTask->GetTaskEvent(pEvent);
	CMCHK_NULL(pEvent);
	CMCHK_HR(pEvent->SetTaskDriverId(param.iTaskIdOut));
	CMCHK_HR(pEvent->SetTaskOsData(param.OsData));
	CMCHK_HR(pTask->ReleaseKernel());

 finish:

	CmSafeDeleteArray(param.pKernels);
	CmSafeDeleteArray(param.piKernelSizes);
	CmSafeDeleteArray(param.piKernelCurbeOffset);

	return hr;
}

INT CmQueue::FlushTaskWithoutSync(bool bIfFlushBlock)
{
	INT hr = CM_SUCCESS;
	CmTaskInternal *pTask = NULL;
	UINT uiTaskType = CM_TASK_TYPE_DEFAULT;

	while (!m_EnqueuedTasks.IsEmpty()) {
		UINT flushedTaskCount = m_FlushedTasks.GetCount();
		if (bIfFlushBlock) {
			while (flushedTaskCount >= m_pHalMaxValues->iMaxTasks) {
				QueryFlushedTasks();
				flushedTaskCount = m_FlushedTasks.GetCount();
			}
		} else {
			if (flushedTaskCount >= m_pHalMaxValues->iMaxTasks) {
				QueryFlushedTasks();
				flushedTaskCount = m_FlushedTasks.GetCount();
				if (flushedTaskCount >=
				    m_pHalMaxValues->iMaxTasks) {
					break;
				}
			}
		}

		pTask = (CmTaskInternal *) m_EnqueuedTasks.Pop();
		CMCHK_NULL(pTask);

		pTask->GetTaskType(uiTaskType);

		m_CriticalSection_HalExecute.Acquire();
		switch (uiTaskType) {
		case CM_INTERNAL_TASK_WITH_THREADSPACE:
			hr = FlushGeneralTask(pTask);
			break;

		case CM_INTERNAL_TASK_WITH_THREADGROUPSPACE:
			hr = FlushGroupTask(pTask);
			break;

		case CM_INTERNAL_TASK_ENQUEUEWITHHINTS:
			hr = FlushEnqueueWithHintsTask(pTask);
			break;

		default:
			hr = FlushGeneralTask(pTask);
			break;
		}
		m_CriticalSection_HalExecute.Release();

		m_FlushedTasks.Push(pTask);

	}

 finish:
	return hr;
}

INT CmQueue::CreateEvent(CmTaskInternal * pTask, BOOL bIsVisible,
			 INT & taskDriverId, CmEvent * &pEvent)
{
	INT hr = CM_SUCCESS;

	m_CriticalSection_Event.Acquire();
	UINT freeSlotInEventArray = m_EventArray.GetFirstFreeIndex();
	m_CriticalSection_Event.Release();

	hr = CmEvent::Create(freeSlotInEventArray, pTask, taskDriverId,
			     m_pDevice, bIsVisible, pEvent);

	if (hr == CM_SUCCESS) {
		m_CriticalSection_Event.Acquire();

		m_EventArray.SetElement(freeSlotInEventArray, pEvent);
		m_EventCount++;

		m_CriticalSection_Event.Release();

		pTask->SetTaskEvent(pEvent);

		if (bIsVisible == FALSE) {
			pEvent = NULL;
		}

	}

	return hr;
}

void CmQueue::AcquireQueueLock(void)
{
	m_CriticalSection_Queue.Acquire();
}

void CmQueue::ReleaseQueueLock(void)
{
	m_CriticalSection_Queue.Release();
}
