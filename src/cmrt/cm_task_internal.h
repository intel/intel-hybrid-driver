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
#pragma once
#include "cm_def.h"
#include "cm_array.h"

class CmKernel;
class CmEvent;
class CmKernelData;
class CmThreadSpace;
class CmThreadGroupSpace;
class CmDevice;

class CmTaskInternal:public CmDynamicArray {
 public:

	static INT Create(const UINT kernelCount, const UINT totalThreadCount,
			  CmKernel * pKernelArray[], const CmThreadSpace * pTS,
			  CmDevice * pCmDevice, const UINT64 uiSyncBitmap,
			  CmTaskInternal * &pTask);
	static INT Destroy(CmTaskInternal * &pTask);
	static INT Create(const UINT kernelCount, const UINT totalThreadCount,
			  CmKernel * pKernelArray[],
			  const CmThreadGroupSpace * pTGS, CmDevice * pCmDevice,
			  const UINT64 uiSyncBitmap, CmTaskInternal * &pTask);
	static INT Create(const UINT kernelCount, const UINT totalThreadCount,
			  CmKernel * pKernelArray[], CmTaskInternal * &pTask,
			  UINT numTasksGenerated, BOOLEAN isLastTask,
			  UINT hints, CmDevice * pCmDevice);

	INT GetKernelCount(UINT & count);
	INT GetKernel(const UINT index, CmKernel * &pKernel);
	INT GetKernelData(const UINT index, CmKernelData * &pKernelData);
	INT GetKernelDataSize(const UINT index, UINT & size);
	UINT GetKernelCurbeOffset(const UINT index);
	INT GetTotalThreadCount(UINT & totalThreadCount);

	INT SetTaskEvent(CmEvent * pEvent);
	INT GetTaskEvent(CmEvent * &pEvent);

	INT CreateThreadSpaceData(const CmThreadSpace * pTS);
	INT GetKernelCoordinates(const UINT index, VOID * &pKernelCoordinates);
	INT GetKernelDependencyMasks(const UINT index,
				     VOID * &pKernelDependencyMasks);
	INT GetDependencyPattern(CM_HAL_DEPENDENCY_PATTERN &
				 pDependencyPattern);
	INT GetWalkingPattern(CM_HAL_WALKING_PATTERN & pWalkingPattern);
	INT GetWalkingParameters(CM_HAL_WALKING_PARAMETERS &
				 pWalkingParameters);
	INT GetDependencyVectors(CM_HAL_DEPENDENCY & pDependencyVector);
	BOOLEAN CheckWalkingParametersSet();
	BOOLEAN CheckDependencyVectorsSet();

	INT GetThreadSpaceSize(UINT & width, UINT & height);
	INT GetThreadGroupSpaceSize(UINT & trdSpaceWidth, UINT & trdSpaceHeight,
				    UINT & grpSpaceWidth,
				    UINT & grpSpaceHeight);
	INT GetSLMSize(UINT & iSLMSize);
	INT GetColorCountMinusOne(UINT & colorCount);
	INT GetHints(UINT & hints);
	INT GetNumTasksGenerated(UINT & numTasksGenerated);
	INT GetLastTask(BOOLEAN & isLastTask);

	BOOLEAN IsThreadGroupSpaceCreated(void);
	BOOLEAN IsThreadSpaceCreated(void);
	BOOLEAN IsThreadCoordinatesExisted(void);

	INT GetTaskType(UINT & taskType);
	INT GetTaskSurfaces(BOOL * &surfArray);

	UINT64 GetSyncBitmap();
	INT ReleaseKernel();

	INT SetPowerOption(PCM_HAL_POWER_OPTION_PARAM pPowerOption);
	PCM_HAL_POWER_OPTION_PARAM GetPowerOption();
	INT SetPreemptionMode(CM_HAL_PREEMPTION_MODE mode);
	CM_HAL_PREEMPTION_MODE GetPreemptionMode();

	INT GetTaskStatus(CM_STATUS & TaskStatus);

 protected:

	 CmTaskInternal(const UINT kernelCount, const UINT totalThreadCount,
			CmKernel * pKernelArray[], CmDevice * pCmDevice,
			const UINT64 uiSyncBitmap);
	~CmTaskInternal(void);

	INT Initialize(const CmThreadSpace * pTS, BOOL isWithHints);
	INT Initialize(const CmThreadGroupSpace * pTGS);

	INT Initialize(UINT hints, UINT numTasksGenerated, BOOLEAN isLastTask);

	CmDynamicArray m_Kernels;
	CmDynamicArray m_KernelData;
	PUINT m_pKernelCurbeOffsetArray;
	UINT m_KernelCount;

	UINT m_TotalThreadCount;

	CmEvent *m_pTaskEvent;

	BOOLEAN m_IsThreadSpaceCreated;
	BOOLEAN m_IsThreadCoordinatesExisted;
	UINT m_ThreadSpaceWidth;
	UINT m_ThreadSpaceHeight;
	PCM_COORDINATE *m_pThreadCoordinates;
	CM_HAL_DEPENDENCY_PATTERN m_DependencyPattern;
	CM_HAL_WALKING_PATTERN m_WalkingPattern;
	DWORD m_WalkingParameters[CM_NUM_DWORD_FOR_MW_PARAM];
	BOOLEAN m_MediaWalkerParamsSet;
	CM_HAL_DEPENDENCY m_DependencyVectors;
	BOOLEAN m_DependencyVectorsSet;
	PCM_HAL_MASK_AND_RESET *m_pDependencyMasks;

	BOOLEAN m_IsThreadGroupSpaceCreated;
	UINT m_GroupSpaceWidth;
	UINT m_GroupSpaceHeight;
	UINT m_SLMSize;

	UINT m_ColorCountMinusOne;
	UINT m_Hints;
	UINT m_NumTasksGenerated;
	BOOLEAN m_IsLastTask;

	UINT64 m_ui64SyncBitmap;

	CmDevice *m_pCmDevice;
	BOOL *m_SurfaceArray;

	UINT m_TaskType;

	CM_HAL_POWER_OPTION_PARAM m_PowerOption;
	CM_HAL_PREEMPTION_MODE m_PreemptionMode;
 private:
	 CmTaskInternal(const CmTaskInternal & other);
	 CmTaskInternal & operator=(const CmTaskInternal & other);
};
