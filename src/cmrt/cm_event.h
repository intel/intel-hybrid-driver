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

#include "cm_array.h"
#include "cm_device.h"

class CmTaskInternal;

class CmEvent:public CmDynamicArray {
 public:

	static INT Create(UINT index, CmTaskInternal * pTask, INT taskDriverId,
			  CmDevice * pCmDev, BOOL isVisible, CmEvent * &pEvent);
	static INT Destroy(CmEvent * &pEvent);

	CM_RT_API INT GetStatus(CM_STATUS & status);
	CM_RT_API INT GetExecutionTime(UINT64 & time);
	CM_RT_API INT GetSubmitTime(LARGE_INTEGER & time);
	CM_RT_API INT GetHWStartTime(LARGE_INTEGER & time);
	CM_RT_API INT GetHWEndTime(LARGE_INTEGER & time);
	CM_RT_API INT GetCompleteTime(LARGE_INTEGER & time);
	CM_RT_API UINT GetKernelCount();
	CM_RT_API INT GetKernelName(UINT index, char *&KernelName);
	CM_RT_API INT GetKernelThreadSpace(UINT index, UINT & localWidth,
					   UINT & localHeight,
					   UINT & globalWidth,
					   UINT & globalHeight);
	INT GetIndex(UINT & index);

	INT SetTaskDriverId(INT id);
	INT GetTaskDriverId(INT & id);
	INT SetTaskOsData(PVOID data);
	CM_RT_API INT WaitForTaskFinished(DWORD dwTimeOutMs =
					  CM_MAX_TIMEOUT_MS);

	INT Acquire(void);
	INT SafeRelease(void);

	INT SetKernelNames(CmTask * pTask, CmThreadSpace * pThreadSpace,
			   CmThreadGroupSpace * pThreadGroupSpace);

 protected:
	 CmEvent(UINT index, CmTaskInternal * pTask, INT taskDriverId,
		 CmDevice * pCmDev, BOOL isVisible);
	~CmEvent(void);
	INT Initialize(void);
	INT Query(void);

	UINT m_Index;
	INT m_TaskDriverId;
	PVOID m_OsData;

	CM_STATUS m_Status;
	UINT64 m_Time;

	LARGE_INTEGER m_GlobalCMSubmitTime;
	LARGE_INTEGER m_CMSubmitTimeStamp;
	LARGE_INTEGER m_HWStartTimeStamp;
	LARGE_INTEGER m_HWEndTimeStamp;
	LARGE_INTEGER m_CompleteTime;

	char **m_KernelNames;
	UINT *m_ThreadSpace;
	UINT m_KernelCount;

	CmDevice *m_pDevice;
	CmQueue *m_pQueue;

	INT m_RefCount;

	BOOL isVisible;

	CM_HAL_SURFACE_ENTRY_INFO_ARRAYS m_SurEntryInfoArrays;
	CmTaskInternal *m_pTask;
};
