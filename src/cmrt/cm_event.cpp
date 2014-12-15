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

#include "cm_event.h"
#include "cm_device.h"
#include "cm_queue.h"
#include "cm_mem.h"
#include "cm_task.h"
#include "cm_kernel.h"
#include "cm_thread_space.h"
#include "cm_group_space.h"
#include "hal_cm.h"
#include "cm_surface_manager.h"

INT CmEvent::Create(UINT index, CmTaskInternal * pTask, INT taskDriverId,
		    CmDevice * pCmDev, BOOL isVisible, CmEvent * &pEvent)
{
	INT result = CM_SUCCESS;
	pEvent =
	    new(std::nothrow) CmEvent(index, pTask, taskDriverId, pCmDev,
				      isVisible);
	if (pEvent) {
		if (isVisible) {
			pEvent->Acquire();
		}
		result = pEvent->Initialize();
		if (result != CM_SUCCESS) {
			CmEvent::Destroy(pEvent);
		}
	} else {
		CM_ASSERT(0);
		result = CM_OUT_OF_HOST_MEMORY;
	}
	return result;
}

INT CmEvent::Destroy(CmEvent * &pEvent)
{
	long refCount = pEvent->SafeRelease();
	if (refCount == 0) {
		pEvent = NULL;
	}

	return CM_SUCCESS;
}

 CmEvent::CmEvent(UINT index, CmTaskInternal * pTask, INT taskDriverId, CmDevice * pCmDev, BOOL isVisible):
m_Index(index),
m_TaskDriverId(taskDriverId),
m_OsData(NULL),
m_Status(CM_STATUS_QUEUED),
m_Time(0),
m_pDevice(pCmDev),
m_pQueue(NULL), m_RefCount(0), isVisible(isVisible), m_pTask(pTask)
{
}

INT CmEvent::Acquire(void)
{
	++m_RefCount;
	return m_RefCount;
}

INT CmEvent::SafeRelease(void)
{
	--m_RefCount;
	if (m_RefCount == 0) {
		delete this;
		return 0;
	} else {
		return m_RefCount;
	}
}

CmEvent::~CmEvent(void)
{
	if (m_SurEntryInfoArrays.pSurfEntryInfosArray != NULL) {

		for (UINT i = 0; i < m_SurEntryInfoArrays.dwKrnNum; i++) {
			if (m_SurEntryInfoArrays.
			    pSurfEntryInfosArray[i].pSurfEntryInfos != NULL) {
				CmSafeDelete
				    (m_SurEntryInfoArrays.pSurfEntryInfosArray
				     [i].pSurfEntryInfos);
			}
			if (m_SurEntryInfoArrays.
			    pSurfEntryInfosArray[i].pGlobalSurfInfos != NULL) {
				CmSafeDelete
				    (m_SurEntryInfoArrays.pSurfEntryInfosArray
				     [i].pGlobalSurfInfos);
			}
		}
		CmSafeDelete(m_SurEntryInfoArrays.pSurfEntryInfosArray);
	}

	if (m_KernelNames != NULL) {
		for (UINT i = 0; i < m_KernelCount; i++) {
			CmSafeDeleteArray(m_KernelNames[i]);
		}
		CmSafeDeleteArray(m_KernelNames);
		CmSafeDeleteArray(m_ThreadSpace);
	}
}

INT CmEvent::Initialize(void)
{
	CmSafeMemSet(&m_SurEntryInfoArrays, 0,
		     sizeof(CM_HAL_SURFACE_ENTRY_INFO_ARRAYS));
	if (m_TaskDriverId == -1) {
		m_Status = CM_STATUS_QUEUED;
	} else {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	m_KernelNames = NULL;
	m_KernelCount = 0;

	m_pDevice->GetQueue(m_pQueue);

	return CM_SUCCESS;
}

CM_RT_API INT CmEvent::GetStatus(CM_STATUS & status)
{

	if ((m_Status == CM_STATUS_FLUSHED) || (m_Status == CM_STATUS_STARTED)) {
		Query();
	} else if (m_Status == CM_STATUS_QUEUED) {
		m_pQueue->FlushTaskWithoutSync();

	} else if (m_Status == CM_STATUS_FINISHED) {
	} else {
		CM_ASSERT(0);
	}

	status = m_Status;
	return CM_SUCCESS;
}

CM_RT_API INT CmEvent::GetExecutionTime(UINT64 & time)
{
	CM_STATUS EventStatus = CM_STATUS_QUEUED;
	m_pQueue->AcquireQueueLock();
	GetStatus(EventStatus);
	m_pQueue->ReleaseQueueLock();

	if (EventStatus == CM_STATUS_FINISHED) {
		time = m_Time;
		return CM_SUCCESS;
	} else {
		return CM_FAILURE;
	}
}

CM_RT_API INT CmEvent::GetSubmitTime(LARGE_INTEGER & time)
{

	CM_STATUS EventStatus = CM_STATUS_QUEUED;

	m_pQueue->AcquireQueueLock();
	GetStatus(EventStatus);
	m_pQueue->ReleaseQueueLock();

	if (EventStatus == CM_STATUS_FINISHED) {
		time = m_GlobalCMSubmitTime;
		return CM_SUCCESS;
	} else {
		CM_ASSERT(0);
		return CM_FAILURE;
	}
}

CM_RT_API INT CmEvent::GetHWStartTime(LARGE_INTEGER & time)
{
	CM_STATUS EventStatus = CM_STATUS_QUEUED;

	m_pQueue->AcquireQueueLock();
	GetStatus(EventStatus);
	m_pQueue->ReleaseQueueLock();

	if (EventStatus == CM_STATUS_FINISHED) {
		time.QuadPart =
		    m_GlobalCMSubmitTime.QuadPart +
		    m_HWStartTimeStamp.QuadPart - m_CMSubmitTimeStamp.QuadPart;
		return CM_SUCCESS;
	} else {
		return CM_FAILURE;
	}

}

CM_RT_API INT CmEvent::GetHWEndTime(LARGE_INTEGER & time)
{

	CM_STATUS EventStatus = CM_STATUS_QUEUED;

	m_pQueue->AcquireQueueLock();
	GetStatus(EventStatus);
	m_pQueue->ReleaseQueueLock();

	if (EventStatus == CM_STATUS_FINISHED) {
		time.QuadPart =
		    m_GlobalCMSubmitTime.QuadPart + m_HWEndTimeStamp.QuadPart -
		    m_CMSubmitTimeStamp.QuadPart;
		return CM_SUCCESS;
	} else {
		return CM_FAILURE;
	}
}

CM_RT_API INT CmEvent::GetCompleteTime(LARGE_INTEGER & time)
{

	CM_STATUS EventStatus = CM_STATUS_QUEUED;

	m_pQueue->AcquireQueueLock();
	GetStatus(EventStatus);
	m_pQueue->ReleaseQueueLock();

	if (EventStatus == CM_STATUS_FINISHED) {
		time = m_CompleteTime;
		return CM_SUCCESS;
	} else {
		return CM_FAILURE;
	}

}

CM_RT_API UINT CmEvent::GetKernelCount()
{
	return m_KernelCount;
}

CM_RT_API INT CmEvent::GetKernelName(UINT index, char *&KernelName)
{
	if (index < m_KernelCount) {
		KernelName = m_KernelNames[index];
		return CM_SUCCESS;
	}
	return CM_FAILURE;
}

CM_RT_API INT
    CmEvent::GetKernelThreadSpace(UINT index, UINT & localWidth,
				  UINT & localHeight, UINT & globalWidth,
				  UINT & globalHeight)
{
	if (index < m_KernelCount) {
		localWidth = m_ThreadSpace[4 * index];
		localHeight = m_ThreadSpace[4 * index + 1];
		globalWidth = m_ThreadSpace[4 * index + 2];
		globalHeight = m_ThreadSpace[4 * index + 3];
		return CM_SUCCESS;
	}
	return CM_FAILURE;
}

INT CmEvent::SetKernelNames(CmTask * pTask, CmThreadSpace * pThreadSpace,
			    CmThreadGroupSpace * pThreadGroupSpace)
{
	UINT i = 0;
	INT hr = CM_SUCCESS;
	CmThreadSpace *pThreadSpace_RT =
	    dynamic_cast < CmThreadSpace * >(pThreadSpace);
	UINT ThreadCount;
	m_KernelCount = pTask->GetKernelCount();

	m_KernelNames = new(std::nothrow) char *[m_KernelCount];
	m_ThreadSpace = new(std::nothrow) UINT[4 * m_KernelCount];
	CMCHK_NULL_RETURN(m_KernelNames, CM_OUT_OF_HOST_MEMORY);
	CmSafeMemSet(m_KernelNames, 0, m_KernelCount * sizeof(char *));
	CMCHK_NULL_RETURN(m_ThreadSpace, CM_OUT_OF_HOST_MEMORY);

	for (i = 0; i < m_KernelCount; i++) {
		m_KernelNames[i] =
		    new(std::nothrow) char[CM_MAX_KERNEL_NAME_SIZE_IN_BYTE];
		CMCHK_NULL_RETURN(m_KernelNames[i], CM_OUT_OF_HOST_MEMORY);
		CmKernel *pKernel = pTask->GetKernelPointer(i);
		strcpy_s(m_KernelNames[i], CM_MAX_KERNEL_NAME_SIZE_IN_BYTE,
			 pKernel->GetName());

		pKernel->GetThreadCount(ThreadCount);
		m_ThreadSpace[4 * i] = ThreadCount;
		m_ThreadSpace[4 * i + 1] = 1;
		m_ThreadSpace[4 * i + 2] = ThreadCount;
		m_ThreadSpace[4 * i + 3] = 1;
	}

	if (pThreadSpace) {
		UINT ThreadWidth, ThreadHeight;
		pThreadSpace_RT->GetThreadSpaceSize(ThreadWidth, ThreadHeight);
		m_ThreadSpace[0] = ThreadWidth;
		m_ThreadSpace[1] = ThreadHeight;
		m_ThreadSpace[2] = ThreadWidth;
		m_ThreadSpace[3] = ThreadHeight;
	} else if (pThreadGroupSpace) {
		UINT ThreadWidth, ThreadHeight, GroupWidth, GroupHeight;
		pThreadGroupSpace->GetThreadGroupSpaceSize(ThreadWidth,
							   ThreadHeight,
							   GroupWidth,
							   GroupHeight);
		m_ThreadSpace[0] = ThreadWidth;
		m_ThreadSpace[1] = ThreadHeight;
		m_ThreadSpace[2] = ThreadWidth * GroupWidth;
		m_ThreadSpace[3] = ThreadHeight * GroupHeight;
	}

 finish:
	if (hr == CM_OUT_OF_HOST_MEMORY) {
		if (m_KernelNames != NULL) {
			for (UINT j = 0; j < m_KernelCount; j++) {
				CmSafeDeleteArray(m_KernelNames[j]);
			}
		}
		CmSafeDeleteArray(m_KernelNames);
		CmSafeDeleteArray(m_ThreadSpace);
	}
	return hr;
}

INT CmEvent::GetIndex(UINT & index)
{
	index = m_Index;
	return CM_SUCCESS;
}

INT CmEvent::SetTaskDriverId(INT id)
{
	m_TaskDriverId = id;
	if (m_TaskDriverId > -1) {
		m_Status = CM_STATUS_FLUSHED;
	} else if (m_TaskDriverId == -1) {
		m_Status = CM_STATUS_QUEUED;
	} else {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	return CM_SUCCESS;
}

INT CmEvent::SetTaskOsData(PVOID data)
{
	m_OsData = data;
	return CM_SUCCESS;
}

INT CmEvent::GetTaskDriverId(INT & id)
{
	id = m_TaskDriverId;
	return CM_SUCCESS;
}

INT CmEvent::Query(void)
{
	CM_RETURN_CODE hr = CM_SUCCESS;

	if ((m_Status != CM_STATUS_FLUSHED) && (m_Status != CM_STATUS_STARTED)) {
		return CM_FAILURE;
	}

	CM_ASSERT(m_TaskDriverId > -1);
	CM_HAL_QUERY_TASK_PARAM param;
	param.iTaskId = m_TaskDriverId;

	PCM_CONTEXT pCmData = (PCM_CONTEXT) m_pDevice->GetAccelData();

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnQueryTask(pCmData->pCmHalState,
						    &param));

	if (param.status == CM_TASK_FINISHED) {
		CmQueue *pQueue = NULL;

		m_Time = param.iTaskDuration;
		m_Status = CM_STATUS_FINISHED;
		m_GlobalCMSubmitTime = param.iTaskGlobalCMSubmitTime;
		m_CMSubmitTimeStamp = param.iTaskCMSubmitTimeStamp;
		m_HWStartTimeStamp = param.iTaskHWStartTimeStamp;
		m_HWEndTimeStamp = param.iTaskHWEndTimeStamp;
		m_CompleteTime = param.iTaskCompleteTime;

		m_pDevice->GetQueue(pQueue);
		if (!pQueue) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}

		if (m_OsData) {
			drm_intel_bo_unreference((drm_intel_bo *) m_OsData);
		}

		m_GlobalCMSubmitTime = param.iTaskGlobalCMSubmitTime;
		m_CMSubmitTimeStamp = param.iTaskCMSubmitTimeStamp;
		m_HWStartTimeStamp = param.iTaskHWStartTimeStamp;
		m_HWEndTimeStamp = param.iTaskHWEndTimeStamp;
		m_CompleteTime = param.iTaskCompleteTime;

	} else if (param.status == CM_TASK_IN_PROGRESS) {
		m_Status = CM_STATUS_STARTED;
	}

 finish:
	return hr;
}

CM_RT_API INT CmEvent::WaitForTaskFinished(DWORD dwTimeOutMs)
{
	INT result = CM_SUCCESS;
	m_pQueue->AcquireQueueLock();

	while (m_Status == CM_STATUS_QUEUED) {
		m_pQueue->FlushTaskWithoutSync();
	}

	Query();

	while (m_Status != CM_STATUS_FINISHED) {
		if (m_OsData) {
			drm_intel_bo_wait_rendering((drm_intel_bo *) m_OsData);
			drm_intel_gem_bo_clear_relocs((drm_intel_bo *) m_OsData,
						      0);
		}

		Query();
	}

	m_pQueue->ReleaseQueueLock();
	return result;
}
