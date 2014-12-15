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

#include "cm_task_internal.h"
#include "cm_kernel.h"
#include "cm_event.h"
#include "cm_device.h"
#include "cm_kernel_data.h"
#include "cm_thread_space.h"
#include "cm_group_space.h"
#include "cm_queue.h"
#include "cm_surface_manager.h"

INT CmTaskInternal::Create(const UINT kernelCount, const UINT totalThreadCount,
			   CmKernel * pKernelArray[], const CmThreadSpace * pTS,
			   CmDevice * pCmDevice, const UINT64 uiSyncBitmap,
			   CmTaskInternal * &pTask)
{
	INT result = CM_SUCCESS;
	pTask =
	    new(std::nothrow) CmTaskInternal(kernelCount, totalThreadCount,
					     pKernelArray, pCmDevice,
					     uiSyncBitmap);
	if (pTask) {
		result = pTask->Initialize(pTS, FALSE);
		if (result != CM_SUCCESS) {
			CmTaskInternal::Destroy(pTask);
		}
	} else {
		CM_ASSERT(0);
		result = CM_OUT_OF_HOST_MEMORY;
	}
	return result;
}

INT CmTaskInternal::Create(const UINT kernelCount, const UINT totalThreadCount,
			   CmKernel * pKernelArray[],
			   const CmThreadGroupSpace * pTGS,
			   CmDevice * pCmDevice, const UINT64 uiSyncBitmap,
			   CmTaskInternal * &pTask)
{
	INT result = CM_SUCCESS;
	pTask =
	    new(std::nothrow) CmTaskInternal(kernelCount, totalThreadCount,
					     pKernelArray, pCmDevice,
					     uiSyncBitmap);

	if (pTask) {
		result = pTask->Initialize(pTGS);
		if (result != CM_SUCCESS) {
			CmTaskInternal::Destroy(pTask);
		}
	} else {
		CM_ASSERT(0);
		result = CM_OUT_OF_HOST_MEMORY;
	}
	return result;
}

INT CmTaskInternal::Create(const UINT kernelCount, const UINT totalThreadCount,
			   CmKernel * pKernelArray[], CmTaskInternal * &pTask,
			   UINT numTasksGenerated, BOOLEAN isLastTask,
			   UINT hints, CmDevice * pCmDevice)
{
	INT result = CM_SUCCESS;
	pTask =
	    new(std::nothrow) CmTaskInternal(kernelCount, totalThreadCount,
					     pKernelArray, pCmDevice,
					     CM_NO_KERNEL_SYNC);
	if (pTask) {
		result =
		    pTask->Initialize(hints, numTasksGenerated, isLastTask);
		if (result != CM_SUCCESS) {
			CmTaskInternal::Destroy(pTask);
		}
	} else {
		CM_ASSERT(0);
		result = CM_OUT_OF_HOST_MEMORY;
	}
	return result;
}

INT CmTaskInternal::Destroy(CmTaskInternal * &pTask)
{
	CmSafeDelete(pTask);
	return CM_SUCCESS;
}

 CmTaskInternal::CmTaskInternal(const UINT kernelCount, const UINT totalThreadCount, CmKernel * pKernelArray[], CmDevice * pCmDevice, const UINT64 uiSyncBitmap):
m_Kernels(kernelCount),
m_KernelData(kernelCount),
m_KernelCount(kernelCount),
m_TotalThreadCount(totalThreadCount),
m_pTaskEvent(NULL),
m_IsThreadSpaceCreated(FALSE),
m_IsThreadCoordinatesExisted(FALSE),
m_ThreadSpaceWidth(0),
m_ThreadSpaceHeight(0),
m_pThreadCoordinates(NULL),
m_DependencyPattern(CM_DEPENDENCY_NONE),
m_WalkingPattern(CM_WALK_DEFAULT),
m_MediaWalkerParamsSet(FALSE),
m_DependencyVectorsSet(FALSE),
m_pDependencyMasks(NULL),
m_IsThreadGroupSpaceCreated(FALSE),
m_GroupSpaceWidth(0),
m_GroupSpaceHeight(0),
m_SLMSize(0),
m_ColorCountMinusOne(0),
m_Hints(0),
m_NumTasksGenerated(0),
m_IsLastTask(FALSE),
m_ui64SyncBitmap(uiSyncBitmap),
m_pCmDevice(pCmDevice), m_SurfaceArray(NULL), m_TaskType(CM_TASK_TYPE_DEFAULT)
{
	m_pKernelCurbeOffsetArray = new(std::nothrow) UINT[kernelCount];
	CM_ASSERT(m_pKernelCurbeOffsetArray != NULL);

	for (UINT i = 0; i < kernelCount; i++) {
		m_Kernels.SetElement(i, pKernelArray[i]);
		m_KernelData.SetElement(i, NULL);
	}

	CmSafeMemSet(&m_WalkingParameters, 0, sizeof(m_WalkingParameters));
	CmSafeMemSet(&m_DependencyVectors, 0, sizeof(m_DependencyVectors));
	if (m_pKernelCurbeOffsetArray != NULL) {
		CmSafeMemSet(m_pKernelCurbeOffsetArray, 0,
			     kernelCount * sizeof(UINT));
	}
}

CmTaskInternal::~CmTaskInternal(void)
{

	m_Kernels.Delete();
	for (UINT i = 0; i < m_KernelCount; i++) {
		CmKernelData *p = (CmKernelData *) m_KernelData.GetElement(i);
		CmKernelData::Destroy(p);
	}
	m_KernelData.Delete();

	CmSafeDeleteArray(m_pKernelCurbeOffsetArray);

	if (m_pTaskEvent) {
		CmQueue *pCmQueue;
		m_pCmDevice->GetQueue(pCmQueue);
		pCmQueue->DestroyEvent(m_pTaskEvent);
	}

	if (m_pThreadCoordinates) {
		for (UINT i = 0; i < m_KernelCount; i++) {
			if (m_pThreadCoordinates[i]) {
				CmSafeDeleteArray(m_pThreadCoordinates[i]);
			}
		}
	}

	CmSafeDeleteArray(m_pThreadCoordinates);

	if (m_pDependencyMasks) {
		for (UINT i = 0; i < m_KernelCount; ++i) {
			CmSafeDeleteArray(m_pDependencyMasks[i]);
		}
	}

	CmSafeDeleteArray(m_pDependencyMasks);
	CmSafeDeleteArray(m_SurfaceArray);
}

INT CmTaskInternal::Initialize(const CmThreadSpace * pTS, BOOL isWithHints)
{
	UINT totalCurbeSize = 0;
	UINT surfacePoolSize = 0;
	UINT totalKernelBinarySize = 0;
	UINT kernelCurbeSize = 0;
	UINT kernelPayloadSize = 0;
	CmSurfaceManager *pSurfaceMgr = NULL;

	CM_HAL_MAX_VALUES *pHalMaxValues = NULL;
	CM_HAL_MAX_VALUES_EX *pHalMaxValuesEx = NULL;
	m_pCmDevice->GetHalMaxValues(pHalMaxValues, pHalMaxValuesEx);

	m_pCmDevice->GetSurfaceManager(pSurfaceMgr);
	surfacePoolSize = pSurfaceMgr->GetSurfacePoolSize();

	m_SurfaceArray = new(std::nothrow) BOOL[surfacePoolSize];
	if (!m_SurfaceArray) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}
	CmSafeMemSet(m_SurfaceArray, 0, surfacePoolSize * sizeof(BOOL));

	for (UINT i = 0; i < m_KernelCount; i++) {

		CmKernel *pKernel = (CmKernel *) m_Kernels.GetElement(i);
		if (pKernel == NULL) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}

		pKernel->GetSizeInPayload(kernelPayloadSize);
		pKernel->GetSizeInCurbe(kernelCurbeSize);

		if ((kernelCurbeSize + kernelPayloadSize) >
		    pHalMaxValues->iMaxArgByteSizePerKernel) {
			CM_ASSERT(0);
			return CM_EXCEED_KERNEL_ARG_SIZE_IN_BYTE;
		} else {
			kernelCurbeSize =
			    pKernel->GetAlignedCurbeSize(kernelCurbeSize);
			totalCurbeSize += kernelCurbeSize;
		}
		m_pKernelCurbeOffsetArray[i] = totalCurbeSize - kernelCurbeSize;

		UINT totalSize = 0;
		CmKernelData *pKernelData = NULL;

		if (isWithHints) {
			CmThreadSpace *pKTS = NULL;
			pKernel->GetThreadSpace(pKTS);
			if (pKTS) {
				for (UINT j = i; j > 0; --j) {
					UINT width, height, myAdjY;
					CmKernel *pTmpKern =
					    (CmKernel *) m_Kernels.GetElement(j
									      -
									      1);
					if (!pTmpKern) {
						CM_ASSERT(0);
						return CM_FAILURE;
					}
					pTmpKern->GetThreadSpace(pKTS);
					pKTS->GetThreadSpaceSize(width, height);
					myAdjY = pKernel->GetAdjustedYCoord();
					pKernel->SetAdjustedYCoord(myAdjY +
								   height);
				}
			}
		}

		pKernel->CollectKernelSurface();
		INT result =
		    pKernel->CreateKernelData(pKernelData, totalSize, pTS);
		if ((pKernelData == NULL) || (result != CM_SUCCESS)) {
			CM_ASSERT(0);
			CmKernelData::Destroy(pKernelData);
			return result;
		}

		m_KernelData.SetElement(i, pKernelData);

		totalKernelBinarySize += pKernel->GetKernelGenxBinarySize();

		BOOL *surfArray = NULL;
		pKernel->GetKernelSurfaces(surfArray);
		for (UINT j = 0; j < surfacePoolSize; j++) {
			m_SurfaceArray[j] |= surfArray[j];
		}
		pKernel->ResetKernelSurfaces();
	}

	if (pTS) {
		if (FAILED(this->CreateThreadSpaceData(pTS))) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}
		m_IsThreadSpaceCreated = TRUE;
	}

	m_TaskType = CM_INTERNAL_TASK_WITH_THREADSPACE;

	if (totalKernelBinarySize >
	    pHalMaxValues->iMaxKernelBinarySize *
	    pHalMaxValues->iMaxKernelsPerTask) {
		CM_ASSERT(0);
		return CM_EXCEED_MAX_KERNEL_SIZE_IN_BYTE;
	}

	return CM_SUCCESS;
}

INT CmTaskInternal::Initialize(const CmThreadGroupSpace * pTGS)
{
	UINT totalCurbeSize = 0;
	UINT surfacePoolSize = 0;
	UINT totalKernelBinarySize = 0;
	UINT kernelCurbeSize = 0;
	UINT kernelPayloadSize = 0;

	CmSurfaceManager *pSurfaceMgr = NULL;
	CM_HAL_MAX_VALUES *pHalMaxValues = NULL;
	CM_HAL_MAX_VALUES_EX *pHalMaxValuesEx = NULL;
	m_pCmDevice->GetHalMaxValues(pHalMaxValues, pHalMaxValuesEx);

	m_pCmDevice->GetSurfaceManager(pSurfaceMgr);
	CM_ASSERT(pSurfaceMgr);
	surfacePoolSize = pSurfaceMgr->GetSurfacePoolSize();
	m_SurfaceArray = new(std::nothrow) BOOL[surfacePoolSize];
	if (!m_SurfaceArray) {
		CM_ASSERT(0);
		return CM_OUT_OF_HOST_MEMORY;
	}
	CmSafeMemSet(m_SurfaceArray, 0, surfacePoolSize * sizeof(BOOL));

	for (UINT i = 0; i < m_KernelCount; i++) {
		CmKernel *pKernel = (CmKernel *) m_Kernels.GetElement(i);
		if (pKernel == NULL) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}

		pKernel->CollectKernelSurface();

		UINT totalSize = 0;
		CmKernelData *pKernelData = NULL;

		INT result =
		    pKernel->CreateKernelData(pKernelData, totalSize, pTGS);
		if (result != CM_SUCCESS) {
			CM_ASSERT(0);
			CmKernelData::Destroy(pKernelData);
			return result;
		}

		pKernelData->SetKernelDataSize(totalSize);

		pKernel->GetSizeInPayload(kernelPayloadSize);
		pKernel->GetSizeInCurbe(kernelCurbeSize);

		if (kernelCurbeSize + kernelPayloadSize >
		    pHalMaxValues->iMaxArgByteSizePerKernel) {
			CM_ASSERT(0);
			return CM_EXCEED_KERNEL_ARG_SIZE_IN_BYTE;
		} else {
			kernelCurbeSize =
			    pKernel->GetAlignedCurbeSize(kernelCurbeSize);
			totalCurbeSize += kernelCurbeSize;
		}

		m_pKernelCurbeOffsetArray[i] = totalCurbeSize - kernelCurbeSize;

		m_KernelData.SetElement(i, pKernelData);

		m_SLMSize = pKernel->GetSLMSize();

		totalKernelBinarySize += pKernel->GetKernelGenxBinarySize();

		BOOL *surfArray = NULL;
		pKernel->GetKernelSurfaces(surfArray);
		for (UINT j = 0; j < surfacePoolSize; j++) {
			m_SurfaceArray[j] |= surfArray[j];
		}
		pKernel->ResetKernelSurfaces();
	}

	if (totalKernelBinarySize >
	    pHalMaxValues->iMaxKernelBinarySize *
	    pHalMaxValues->iMaxKernelsPerTask) {
		CM_ASSERT(0);
		return CM_EXCEED_MAX_KERNEL_SIZE_IN_BYTE;
	}

	m_TaskType = CM_INTERNAL_TASK_WITH_THREADGROUPSPACE;

	if (pTGS) {
		pTGS->GetThreadGroupSpaceSize(m_ThreadSpaceWidth,
					      m_ThreadSpaceHeight,
					      m_GroupSpaceWidth,
					      m_GroupSpaceHeight);
		m_IsThreadGroupSpaceCreated = TRUE;
	}

	return CM_SUCCESS;
}

INT CmTaskInternal::Initialize(UINT hints, UINT numTasksGenerated,
			       BOOLEAN isLastTask)
{
	CmThreadSpace *pTS = NULL;
	INT result = CM_SUCCESS;

	result = this->Initialize(pTS, TRUE);

	m_Hints = hints;

	m_NumTasksGenerated = numTasksGenerated;
	m_IsLastTask = isLastTask;

	m_TaskType = CM_INTERNAL_TASK_ENQUEUEWITHHINTS;

	return result;
}

INT CmTaskInternal::GetKernelCount(UINT & count)
{
	count = m_KernelCount;
	return CM_SUCCESS;
}

INT CmTaskInternal::GetTaskSurfaces(BOOL * &surfArray)
{
	surfArray = m_SurfaceArray;
	return CM_SUCCESS;
}

INT CmTaskInternal::GetKernel(const UINT index, CmKernel * &pKernel)
{
	pKernel = NULL;
	if (index < m_Kernels.GetSize()) {
		pKernel = (CmKernel *) m_Kernels.GetElement(index);
		return CM_SUCCESS;
	} else {
		return CM_FAILURE;
	}
}

INT CmTaskInternal::GetKernelData(const UINT index, CmKernelData * &pKernelData)
{
	pKernelData = NULL;
	if (index < m_KernelData.GetSize()) {
		pKernelData = (CmKernelData *) m_KernelData.GetElement(index);
		return CM_SUCCESS;
	} else {
		return CM_FAILURE;
	}
}

INT CmTaskInternal::GetKernelDataSize(const UINT index, UINT & size)
{
	size = 0;
	CmKernelData *pKernelData = NULL;
	if (index < m_KernelData.GetSize()) {
		pKernelData = (CmKernelData *) m_KernelData.GetElement(index);
		if (pKernelData == NULL) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}
		size = pKernelData->GetKernelDataSize();
		return CM_SUCCESS;
	} else {
		return CM_FAILURE;
	}
}

UINT CmTaskInternal::GetKernelCurbeOffset(const UINT index)
{
	return (UINT) m_pKernelCurbeOffsetArray[index];
}

INT CmTaskInternal::SetTaskEvent(CmEvent * pEvent)
{
	m_pTaskEvent = pEvent;
	m_pTaskEvent->Acquire();
	return CM_SUCCESS;
}

INT CmTaskInternal::GetTaskEvent(CmEvent * &pEvent)
{
	pEvent = m_pTaskEvent;
	return CM_SUCCESS;
}

INT CmTaskInternal::GetTaskStatus(CM_STATUS & TaskStatus)
{
	if (m_pTaskEvent == NULL) {
		return CM_FAILURE;
	}

	return m_pTaskEvent->GetStatus(TaskStatus);
}

INT CmTaskInternal::ReleaseKernel()
{

	INT hr = CM_SUCCESS;

	for (UINT KrnDataIndex = 0; KrnDataIndex < m_KernelCount;
	     KrnDataIndex++) {
		CmKernelData *pKernelData;
		CMCHK_HR(GetKernelData(KrnDataIndex, pKernelData));
		CMCHK_NULL(pKernelData);
		CMCHK_HR(pKernelData->ReleaseKernel());
	}

 finish:
	return hr;
}

INT CmTaskInternal::CreateThreadSpaceData(const CmThreadSpace * pTS)
{
	UINT i;
	UINT width, height;
	UINT *pKernelCoordinateIndex = NULL;
	int hr = CM_SUCCESS;
	CmThreadSpace *pTS_RT = const_cast < CmThreadSpace * >(pTS);

	CmKernel *pKernel_inTS = NULL;
	CmKernel *pKernel_inTask = NULL;

	if (pTS_RT->IsThreadAssociated()) {
		m_pThreadCoordinates =
		    new(std::nothrow) PCM_COORDINATE[m_KernelCount];
		CMCHK_NULL_RETURN(m_pThreadCoordinates, CM_FAILURE);
		CmSafeMemSet(m_pThreadCoordinates, 0,
			     m_KernelCount * sizeof(PCM_COORDINATE));

		m_pDependencyMasks =
		    new(std::nothrow) PCM_HAL_MASK_AND_RESET[m_KernelCount];
		CMCHK_NULL_RETURN(m_pDependencyMasks, CM_FAILURE);
		CmSafeMemSet(m_pDependencyMasks, 0,
			     m_KernelCount * sizeof(PCM_HAL_MASK_AND_RESET));

		pKernelCoordinateIndex = new(std::nothrow) UINT[m_KernelCount];
		if (m_pThreadCoordinates && pKernelCoordinateIndex
		    && m_pDependencyMasks) {
			CmSafeMemSet(pKernelCoordinateIndex, 0,
				     m_KernelCount * sizeof(UINT));
			for (i = 0; i < m_KernelCount; i++) {
				pKernelCoordinateIndex[i] = 0;
				UINT threadCount;
				this->GetKernel(i, pKernel_inTask);

				if (pKernel_inTask == NULL) {
					CM_ASSERT(0);
					hr = CM_FAILURE;
					goto finish;
				}

				pKernel_inTask->GetThreadCount(threadCount);
				m_pThreadCoordinates[i] = new(std::nothrow)
				    CM_COORDINATE[threadCount];
				if (m_pThreadCoordinates[i]) {
					CmSafeMemSet(m_pThreadCoordinates[i], 0,
						     sizeof(CM_COORDINATE) *
						     threadCount);
				} else {
					CM_ASSERT(0);
					hr = CM_FAILURE;
					goto finish;
				}

				m_pDependencyMasks[i] = new(std::nothrow)
				    CM_HAL_MASK_AND_RESET[threadCount];
				if (m_pDependencyMasks[i]) {
					CmSafeMemSet(m_pDependencyMasks[i], 0,
						     sizeof
						     (CM_HAL_MASK_AND_RESET) *
						     threadCount);
				} else {
					CM_ASSERT(0);
					hr = CM_FAILURE;
					goto finish;
				}
			}

			CM_THREAD_SPACE_UNIT *pThreadSpaceUnit = NULL;
			pTS_RT->GetThreadSpaceSize(width, height);
			pTS_RT->GetThreadSpaceUnit(pThreadSpaceUnit);

			UINT *pBoardOrder = NULL;
			pTS_RT->GetBoardOrder(pBoardOrder);
			for (UINT tIndex = 0; tIndex < height * width; tIndex++) {
				pKernel_inTS =
				    static_cast <
				    CmKernel *
				    >(pThreadSpaceUnit
				      [pBoardOrder[tIndex]].pKernel);
				if (pKernel_inTS == NULL) {
					if (pTS_RT->GetNeedSetKernelPointer()) {
						pKernel_inTS =
						    pTS_RT->GetKernelPointer();
					}
					if (pKernel_inTS == NULL) {
						CM_ASSERT(0);
						hr = CM_FAILURE;
						goto finish;
					}
				}
				UINT kIndex = pKernel_inTS->GetIndexInTask();

				m_pThreadCoordinates[kIndex]
				    [pKernelCoordinateIndex[kIndex]].x =
				    pThreadSpaceUnit[pBoardOrder
						     [tIndex]].scoreboardCoordinates.
				    x;
				m_pThreadCoordinates[kIndex]
				    [pKernelCoordinateIndex[kIndex]].y =
				    pThreadSpaceUnit[pBoardOrder
						     [tIndex]].scoreboardCoordinates.
				    y;
				m_pDependencyMasks[kIndex]
				    [pKernelCoordinateIndex[kIndex]].mask =
				    pThreadSpaceUnit[pBoardOrder
						     [tIndex]].dependencyMask;
				m_pDependencyMasks[kIndex]
				    [pKernelCoordinateIndex[kIndex]].resetMask =
				    pThreadSpaceUnit[pBoardOrder[tIndex]].reset;
				pKernelCoordinateIndex[kIndex]++;
			}

			CmSafeDeleteArray(pKernelCoordinateIndex);
		} else {
			CM_ASSERT(0);
			hr = CM_FAILURE;
			goto finish;
		}

		m_IsThreadCoordinatesExisted = TRUE;
	} else {
		m_pThreadCoordinates = NULL;
		m_pDependencyMasks = NULL;
		m_IsThreadCoordinatesExisted = FALSE;
	}

	if (pTS_RT->IsDependencySet()) {
		pTS_RT->GetDependencyPatternType(m_DependencyPattern);
	}

	pTS_RT->GetThreadSpaceSize(m_ThreadSpaceWidth, m_ThreadSpaceHeight);

	pTS_RT->GetColorCountMinusOne(m_ColorCountMinusOne);

	pTS_RT->GetWalkingPattern(m_WalkingPattern);

	m_MediaWalkerParamsSet = pTS_RT->CheckWalkingParametersSet();
	if (m_MediaWalkerParamsSet) {
		CM_HAL_WALKING_PARAMETERS tmpMWParams;
		CMCHK_HR(pTS_RT->GetWalkingParameters(tmpMWParams));
		CmSafeMemCopy(&m_WalkingParameters, &tmpMWParams,
			      sizeof(tmpMWParams));
	}

	m_DependencyVectorsSet = pTS_RT->CheckDependencyVectorsSet();
	if (m_DependencyVectorsSet) {
		CM_HAL_DEPENDENCY tmpDepVectors;
		CMCHK_HR(pTS_RT->GetDependencyVectors(tmpDepVectors));
		CmSafeMemCopy(&m_DependencyVectors, &tmpDepVectors,
			      sizeof(tmpDepVectors));
	}

 finish:
	if (hr != CM_SUCCESS) {
		if (m_pThreadCoordinates) {
			for (i = 0; i < m_KernelCount; i++) {
				CmSafeDeleteArray(m_pThreadCoordinates[i]);
			}
		}

		if (m_pDependencyMasks) {
			for (i = 0; i < m_KernelCount; i++) {
				CmSafeDeleteArray(m_pDependencyMasks[i]);
			}
		}
		CmSafeDeleteArray(m_pThreadCoordinates);
		CmSafeDeleteArray(m_pDependencyMasks);
		CmSafeDeleteArray(pKernelCoordinateIndex);
	}
	return hr;
}

INT CmTaskInternal::GetKernelCoordinates(const UINT index,
					 VOID * &pKernelCoordinates)
{
	if (m_pThreadCoordinates != NULL) {
		pKernelCoordinates = (PVOID) m_pThreadCoordinates[index];
	} else {
		pKernelCoordinates = NULL;
	}

	return CM_SUCCESS;
}

INT CmTaskInternal::GetKernelDependencyMasks(const UINT index,
					     VOID * &pKernelDependencyMasks)
{
	if (m_pDependencyMasks != NULL) {
		pKernelDependencyMasks = (PVOID) m_pDependencyMasks[index];
	} else {
		pKernelDependencyMasks = NULL;
	}

	return CM_SUCCESS;
}

INT CmTaskInternal::GetDependencyPattern(CM_HAL_DEPENDENCY_PATTERN &
					 DependencyPattern)
{
	DependencyPattern = m_DependencyPattern;
	return CM_SUCCESS;
}

INT CmTaskInternal::GetWalkingPattern(CM_HAL_WALKING_PATTERN & WalkingPattern)
{
	WalkingPattern = m_WalkingPattern;
	return CM_SUCCESS;
}

INT CmTaskInternal::GetWalkingParameters(CM_HAL_WALKING_PARAMETERS &
					 pWalkingParameters)
{
	if (&pWalkingParameters == NULL) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	CmSafeMemCopy(&pWalkingParameters, &m_WalkingParameters,
		      sizeof(m_WalkingParameters));
	return CM_SUCCESS;
}

BOOLEAN CmTaskInternal::CheckWalkingParametersSet()
{
	return m_MediaWalkerParamsSet;
}

INT CmTaskInternal::GetDependencyVectors(CM_HAL_DEPENDENCY & pDependencyVectors)
{
	if (&pDependencyVectors == NULL) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	CmSafeMemCopy(&pDependencyVectors, &m_DependencyVectors,
		      sizeof(m_DependencyVectors));
	return CM_SUCCESS;
}

BOOLEAN CmTaskInternal::CheckDependencyVectorsSet()
{
	return m_DependencyVectorsSet;
}

INT CmTaskInternal::GetTotalThreadCount(UINT & totalThreadCount)
{
	totalThreadCount = m_TotalThreadCount;

	return CM_SUCCESS;
}

INT CmTaskInternal::GetThreadSpaceSize(UINT & width, UINT & height)
{
	width = m_ThreadSpaceWidth;
	height = m_ThreadSpaceHeight;

	return CM_SUCCESS;
}

INT CmTaskInternal::GetColorCountMinusOne(UINT & colorCount)
{
	colorCount = m_ColorCountMinusOne;

	return CM_SUCCESS;
}

BOOLEAN CmTaskInternal::IsThreadSpaceCreated(void)
{
	return m_IsThreadSpaceCreated;
}

BOOLEAN CmTaskInternal::IsThreadCoordinatesExisted(void)
{
	return m_IsThreadCoordinatesExisted;
}

INT CmTaskInternal::GetThreadGroupSpaceSize(UINT & trdSpaceWidth,
					    UINT & trdSpaceHeight,
					    UINT & grpSpaceWidth,
					    UINT & grpSpaceHeight)
{
	trdSpaceWidth = m_ThreadSpaceWidth;
	trdSpaceHeight = m_ThreadSpaceHeight;
	grpSpaceWidth = m_GroupSpaceWidth;
	grpSpaceHeight = m_GroupSpaceHeight;

	return CM_SUCCESS;
}

INT CmTaskInternal::GetSLMSize(UINT & iSLMSize)
{
	iSLMSize = m_SLMSize;
	return CM_SUCCESS;
}

INT CmTaskInternal::GetHints(UINT & hints)
{
	hints = m_Hints;
	return CM_SUCCESS;
}

INT CmTaskInternal::GetNumTasksGenerated(UINT & numTasksGenerated)
{
	numTasksGenerated = m_NumTasksGenerated;
	return CM_SUCCESS;
}

INT CmTaskInternal::GetLastTask(BOOLEAN & isLastTask)
{
	isLastTask = m_IsLastTask;
	return CM_SUCCESS;
}

BOOLEAN CmTaskInternal::IsThreadGroupSpaceCreated(void)
{
	return m_IsThreadGroupSpaceCreated;
}

INT CmTaskInternal::GetTaskType(UINT & taskType)
{
	taskType = m_TaskType;

	return CM_SUCCESS;
}

UINT64 CmTaskInternal::GetSyncBitmap()
{
	return m_ui64SyncBitmap;
}

INT CmTaskInternal::SetPowerOption(PCM_HAL_POWER_OPTION_PARAM pPowerOption)
{
	CmFastMemCopy(&m_PowerOption, pPowerOption, sizeof(m_PowerOption));
	return CM_SUCCESS;
}

PCM_HAL_POWER_OPTION_PARAM CmTaskInternal::GetPowerOption()
{
	return &m_PowerOption;
}

INT CmTaskInternal::SetPreemptionMode(CM_HAL_PREEMPTION_MODE mode)
{
	m_PreemptionMode = mode;

	return CM_SUCCESS;
}

CM_HAL_PREEMPTION_MODE CmTaskInternal::GetPreemptionMode()
{
	return m_PreemptionMode;
}
