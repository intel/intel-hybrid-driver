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

#include "cm_thread_space.h"
#include "cm_kernel.h"
#include "cm_task.h"

static CM_DEPENDENCY WavefrontPattern = {
	3,
	{-1, -1, 0},
	{0, -1, -1}
};

static CM_DEPENDENCY Wavefront26Pattern = {
	4,
	{-1, -1, 0, 1},
	{0, -1, -1, -1}
};

static CM_DEPENDENCY Wavefront26ZPattern = {
	5,
	{-1, -1, -1, 0, 1},
	{1, 0, -1, -1, -1}
};

static CM_DEPENDENCY Wavefront26ZIPattern = {
	7,
	{-1, -2, -1, -1, 0, 1, 1},
	{1, 0, 0, -1, -1, -1, 0}
};

static CM_DEPENDENCY HorizontalPattern = {
	1,
	{0},
	{-1}
};

static CM_DEPENDENCY VerticalPattern = {
	1,
	{-1},
	{0}
};

INT CmThreadSpace::Create(CmDevice * pDevice, UINT indexTsArray, UINT width,
			  UINT height, CmThreadSpace * &pTS)
{
	if ((0 == width) || (0 == height) || (width > CM_MAX_THREADSPACE_WIDTH)
	    || (height > CM_MAX_THREADSPACE_HEIGHT)) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	INT result = CM_SUCCESS;
	pTS =
	    new(std::nothrow) CmThreadSpace(pDevice, indexTsArray, width,
					    height);
	if (pTS) {
		result = pTS->Initialize();
		if (result != CM_SUCCESS) {
			CmThreadSpace::Destroy(pTS);
		}
	} else {
		CM_ASSERT(0);
		result = CM_OUT_OF_HOST_MEMORY;
	}
	return result;
}

INT CmThreadSpace::Destroy(CmThreadSpace * &pTS)
{
	if (pTS) {
		delete pTS;
		pTS = NULL;
	}
	return CM_SUCCESS;
}

 CmThreadSpace::CmThreadSpace(CmDevice * pDevice, UINT indexTsArray, UINT width, UINT height):
m_pDevice(pDevice),
m_Width(width),
m_Height(height),
m_ColorCountMinusOne(0),
m_26ZIBlockWidth(CM_26ZI_BLOCK_WIDTH),
m_26ZIBlockHeight(CM_26ZI_BLOCK_HEIGHT),
m_pThreadSpaceUnit(NULL),
m_ThreadAssociated(FALSE),
m_NeedSetKernelPointer(FALSE),
m_ppKernel(NULL),
m_DependencyPatternType(CM_DEPENDENCY_NONE),
m_CurrentDependencyPattern(CM_DEPENDENCY_NONE),
m_26ZIDispatchPattern(VVERTICAL_HVERTICAL_26),
m_Current26ZIDispatchPattern(VVERTICAL_HVERTICAL_26),
m_pBoardFlag(NULL),
m_pBoardOrderList(NULL),
m_IndexInList(0),
m_IndexInTsArray(indexTsArray),
m_WalkingPattern(CM_WALK_DEFAULT),
m_MediaWalkerParamsSet(FALSE),
m_DependencyVectorsSet(FALSE), m_pDirtyStatus(NULL)
{
	CmSafeMemSet(&m_Dependency, 0, sizeof(CM_DEPENDENCY));
	CmSafeMemSet(&m_Wavefront26ZDispatchInfo, 0,
		     sizeof(CM_HAL_WAVEFRONT26Z_DISPATCH_INFO));
	CmSafeMemSet(&m_WalkingParameters, 0, sizeof(m_WalkingParameters));
	CmSafeMemSet(&m_DependencyVectors, 0, sizeof(m_DependencyVectors));
}

CmThreadSpace::~CmThreadSpace(void)
{
	CmSafeDeleteArray(m_pThreadSpaceUnit);
	CmSafeDeleteArray(m_pBoardFlag);
	CmSafeDeleteArray(m_pBoardOrderList);
	CmSafeDelete(m_pDirtyStatus);
	CmSafeDelete(m_ppKernel);

	if (m_Wavefront26ZDispatchInfo.pNumThreadsInWave)
		GENOS_FreeMemory(m_Wavefront26ZDispatchInfo.pNumThreadsInWave);
}

INT CmThreadSpace::Initialize(void)
{
	m_pDirtyStatus = new(std::nothrow) CM_THREAD_SPACE_DIRTY_STATUS;
	if (m_pDirtyStatus == NULL) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}
	*m_pDirtyStatus = CM_THREAD_SPACE_CLEAN;

	m_ppKernel = new(std::nothrow) CmKernel *;
	if (m_ppKernel == NULL) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}
	*m_ppKernel = NULL;

	return CM_SUCCESS;
}

CM_RT_API INT
    CmThreadSpace::AssociateThread(UINT x, UINT y, CmKernel * pKernel,
				   UINT threadId, BYTE dependencyMask =
				   CM_DEFAULT_THREAD_DEPENDENCY_MASK)
{
	if ((x >= m_Width) || (y >= m_Height) || (pKernel == NULL)) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}
	if (m_pThreadSpaceUnit == NULL) {
		m_pThreadSpaceUnit =
		    new(std::nothrow) CM_THREAD_SPACE_UNIT[m_Height * m_Width];
		if (m_pThreadSpaceUnit) {
			CmSafeMemSet(m_pThreadSpaceUnit, 0,
				     sizeof(CM_THREAD_SPACE_UNIT) * m_Height *
				     m_Width);
		} else {
			CM_ASSERT(0);
			return CM_OUT_OF_HOST_MEMORY;
		}
	}

	UINT linear_offset = y * m_Width + x;
	if ((m_pThreadSpaceUnit[linear_offset].pKernel == pKernel) &&
	    (m_pThreadSpaceUnit[linear_offset].threadId == threadId) &&
	    (m_pThreadSpaceUnit[linear_offset].scoreboardCoordinates.x ==
	     (INT) x)
	    && (m_pThreadSpaceUnit[linear_offset].scoreboardCoordinates.y ==
		(INT) y)) {
		if (m_pThreadSpaceUnit[linear_offset].dependencyMask ==
		    dependencyMask) {
			m_pThreadSpaceUnit[linear_offset].reset =
			    CM_REUSE_DEPENDENCY_MASK;
		} else {
			m_pThreadSpaceUnit[linear_offset].dependencyMask =
			    dependencyMask;
			m_pThreadSpaceUnit[linear_offset].reset =
			    CM_RESET_DEPENDENCY_MASK;
		}
		*m_pDirtyStatus = CM_THREAD_SPACE_DEPENDENCY_MASK_DIRTY;
	} else {
		m_pThreadSpaceUnit[linear_offset].pKernel = pKernel;
		m_pThreadSpaceUnit[linear_offset].threadId = threadId;
		m_pThreadSpaceUnit[linear_offset].scoreboardCoordinates.x = x;
		m_pThreadSpaceUnit[linear_offset].scoreboardCoordinates.y = y;
		m_pThreadSpaceUnit[linear_offset].dependencyMask =
		    dependencyMask;
		m_pThreadSpaceUnit[linear_offset].reset =
		    CM_NO_BATCH_BUFFER_REUSE;
		*m_pDirtyStatus = CM_THREAD_SPACE_DATA_DIRTY;
	}

	if (!m_ThreadAssociated) {
		m_ThreadAssociated = TRUE;
	}

	pKernel->SetAssociatedToTSFlag(TRUE);

	return CM_SUCCESS;
}

CM_RT_API INT
    CmThreadSpace::SetThreadDependencyPattern(UINT count, INT * deltaX,
					      INT * deltaY)
{
	if (count > CM_MAX_DEPENDENCY_COUNT) {
		CM_ASSERTMESSAGE
		    ("Exceed dependency count limitation, which is 8.");
		return CM_FAILURE;
	}

	m_Dependency.count = count;

	CmSafeMemCopy(m_Dependency.deltaX, deltaX, sizeof(INT) * count);
	CmSafeMemCopy(m_Dependency.deltaY, deltaY, sizeof(INT) * count);

	return CM_SUCCESS;
}

CM_RT_API INT
    CmThreadSpace::SelectThreadDependencyPattern(CM_HAL_DEPENDENCY_PATTERN
						 pattern)
{
	int result = CM_SUCCESS;

	if (m_pBoardFlag == NULL) {
		m_pBoardFlag = new(std::nothrow) UINT[m_Height * m_Width];
		if (m_pBoardFlag) {
			CmSafeMemSet(m_pBoardFlag, 0,
				     sizeof(UINT) * m_Height * m_Width);
		} else {
			CM_ASSERT(0);
			return CM_OUT_OF_HOST_MEMORY;
		}
	}
	if (m_pBoardOrderList == NULL) {
		m_pBoardOrderList = new(std::nothrow) UINT[m_Height * m_Width];
		if (m_pBoardOrderList) {
			CmSafeMemSet(m_pBoardOrderList, 0,
				     sizeof(UINT) * m_Height * m_Width);
		} else {
			CM_ASSERT(0);
			CmSafeDeleteArray(m_pBoardFlag);
			return CM_OUT_OF_HOST_MEMORY;
		}
	}

	if ((pattern != CM_DEPENDENCY_NONE)
	    && (m_WalkingPattern != CM_WALK_DEFAULT)) {
		CM_ASSERT(0);
		return CM_INVALID_DEPENDENCY_WITH_WALKING_PATTERN;
	}

	switch (pattern) {
	case CM_DEPENDENCY_VERTICAL:
		m_DependencyPatternType = CM_DEPENDENCY_VERTICAL;
		result =
		    SetThreadDependencyPattern(VerticalPattern.count,
					       VerticalPattern.deltaX,
					       VerticalPattern.deltaY);
		break;

	case CM_DEPENDENCY_HORIZONTAL:
		m_DependencyPatternType = CM_DEPENDENCY_HORIZONTAL;
		result =
		    SetThreadDependencyPattern(HorizontalPattern.count,
					       HorizontalPattern.deltaX,
					       HorizontalPattern.deltaY);
		break;

	case CM_DEPENDENCY_WAVEFRONT:
		m_DependencyPatternType = CM_DEPENDENCY_WAVEFRONT;
		result =
		    SetThreadDependencyPattern(WavefrontPattern.count,
					       WavefrontPattern.deltaX,
					       WavefrontPattern.deltaY);
		break;

	case CM_DEPENDENCY_WAVEFRONT26:
		m_DependencyPatternType = CM_DEPENDENCY_WAVEFRONT26;
		result =
		    SetThreadDependencyPattern(Wavefront26Pattern.count,
					       Wavefront26Pattern.deltaX,
					       Wavefront26Pattern.deltaY);
		break;

	case CM_DEPENDENCY_WAVEFRONT26Z:
		m_DependencyPatternType = CM_DEPENDENCY_WAVEFRONT26Z;
		result =
		    SetThreadDependencyPattern(Wavefront26ZPattern.count,
					       Wavefront26ZPattern.deltaX,
					       Wavefront26ZPattern.deltaY);
		m_Wavefront26ZDispatchInfo.pNumThreadsInWave =
		    (PUINT) GENOS_AllocAndZeroMemory(sizeof(UINT) * m_Width *
						     m_Height);
		break;

	case CM_DEPENDENCY_WAVEFRONT26ZI:
		m_DependencyPatternType = CM_DEPENDENCY_WAVEFRONT26ZI;
		result =
		    SetThreadDependencyPattern(Wavefront26ZIPattern.count,
					       Wavefront26ZIPattern.deltaX,
					       Wavefront26ZIPattern.deltaY);
		if (m_pThreadSpaceUnit == NULL) {
			m_pThreadSpaceUnit =
			    new(std::nothrow) CM_THREAD_SPACE_UNIT[m_Height *
								   m_Width];
			if (m_pThreadSpaceUnit) {
				CmSafeMemSet(m_pThreadSpaceUnit, 0,
					     sizeof(CM_THREAD_SPACE_UNIT) *
					     m_Height * m_Width);
			} else {
				return CM_OUT_OF_HOST_MEMORY;
			}
			UINT threadId = 0;
			UINT linear_offset = 0;
			for (UINT y = 0; y < m_Height; ++y) {
				for (UINT x = 0; x < m_Width; ++x) {
					linear_offset = y * m_Width + x;
					m_pThreadSpaceUnit
					    [linear_offset].threadId =
					    threadId++;
					m_pThreadSpaceUnit
					    [linear_offset].scoreboardCoordinates.
					    x = x;
					m_pThreadSpaceUnit
					    [linear_offset].scoreboardCoordinates.
					    y = y;
					m_pThreadSpaceUnit
					    [linear_offset].dependencyMask =
					    (1 << Wavefront26ZIPattern.count) -
					    1;
					m_pThreadSpaceUnit[linear_offset].reset
					    = CM_NO_BATCH_BUFFER_REUSE;
				}
			}

			*m_pDirtyStatus = CM_THREAD_SPACE_DATA_DIRTY;
			m_ThreadAssociated = TRUE;
			m_NeedSetKernelPointer = TRUE;
		}
		break;

	case CM_DEPENDENCY_NONE:
		m_DependencyPatternType = CM_DEPENDENCY_NONE;
		result = CM_SUCCESS;
		break;

	default:
		result = CM_FAILURE;
		break;
	}

	if (m_DependencyPatternType != m_CurrentDependencyPattern) {
		*m_pDirtyStatus = CM_THREAD_SPACE_DATA_DIRTY;
	}

	return result;
}

CM_RT_API INT
    CmThreadSpace::SelectMediaWalkingPattern(CM_HAL_WALKING_PATTERN pattern)
{
	int result = CM_SUCCESS;

	if (m_DependencyPatternType != CM_DEPENDENCY_NONE) {
		CM_ASSERT(0);
		return CM_INVALID_DEPENDENCY_WITH_WALKING_PATTERN;
	}

	switch (pattern) {
	case CM_WALK_DEFAULT:
		m_WalkingPattern = CM_WALK_DEFAULT;
		break;
	case CM_WALK_HORIZONTAL:
		m_WalkingPattern = CM_WALK_HORIZONTAL;
		break;
	case CM_WALK_VERTICAL:
		m_WalkingPattern = CM_WALK_VERTICAL;
		break;
	case CM_WALK_WAVEFRONT:
		m_WalkingPattern = CM_WALK_WAVEFRONT;
		break;
	case CM_WALK_WAVEFRONT26:
		m_WalkingPattern = CM_WALK_WAVEFRONT26;
		break;
	default:
		CM_ASSERT(0);
		result = CM_INVALID_MEDIA_WALKING_PATTERN;
		break;
	}

	return result;
}

CM_RT_API INT
    CmThreadSpace::SelectMediaWalkingParameters(CM_HAL_WALKING_PARAMETERS
						parameters)
{
	if (CmSafeMemCompare
	    (&m_WalkingParameters, &parameters,
	     sizeof(m_WalkingParameters)) != 0) {
		CmSafeMemCopy(&m_WalkingParameters, &parameters,
			      sizeof(m_WalkingParameters));
		*m_pDirtyStatus = CM_THREAD_SPACE_DATA_DIRTY;
	}

	m_MediaWalkerParamsSet = TRUE;

	return CM_SUCCESS;
}

CM_RT_API INT
    CmThreadSpace::SelectThreadDependencyVectors(CM_HAL_DEPENDENCY
						 dependencyVectors)
{
	if (CmSafeMemCompare
	    (&m_DependencyVectors, &dependencyVectors,
	     sizeof(m_DependencyVectors)) != 0) {
		CmSafeMemCopy(&m_DependencyVectors, &dependencyVectors,
			      sizeof(m_DependencyVectors));
		*m_pDirtyStatus = CM_THREAD_SPACE_DATA_DIRTY;
	}

	m_DependencyVectorsSet = TRUE;

	return CM_SUCCESS;
}

CM_RT_API INT CmThreadSpace::SetThreadSpaceColorCount(UINT colorCount)
{
	if (colorCount == CM_INVALID_COLOR_COUNT
	    || colorCount > CM_THREADSPACE_MAX_COLOR_COUNT) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	m_ColorCountMinusOne = colorCount - 1;

	return CM_SUCCESS;
}

CM_RT_API INT
    CmThreadSpace::Set26ZIDispatchPattern(CM_HAL_26ZI_DISPATCH_PATTERN pattern)
{
	int result = CM_SUCCESS;

	switch (pattern) {
	case VVERTICAL_HVERTICAL_26:
		m_26ZIDispatchPattern = VVERTICAL_HVERTICAL_26;
		break;
	case VVERTICAL_HHORIZONTAL_26:
		m_26ZIDispatchPattern = VVERTICAL_HHORIZONTAL_26;
		break;
	case VVERTICAL26_HHORIZONTAL26:
		m_26ZIDispatchPattern = VVERTICAL26_HHORIZONTAL26;
		break;
	case VVERTICAL1X26_HHORIZONTAL1X26:
		m_26ZIDispatchPattern = VVERTICAL1X26_HHORIZONTAL1X26;
		break;
	default:
		result = CM_FAILURE;
		break;
	}

	if (m_26ZIDispatchPattern != m_Current26ZIDispatchPattern) {
		*m_pDirtyStatus = CM_THREAD_SPACE_DATA_DIRTY;
	}

	return result;
}

CM_RT_API INT CmThreadSpace::Set26ZIMacroBlockSize(UINT width, UINT height)
{
	m_26ZIBlockWidth = width;
	m_26ZIBlockHeight = height;

	return CM_SUCCESS;
}

INT CmThreadSpace::GetColorCountMinusOne(UINT & colorCount)
{
	colorCount = m_ColorCountMinusOne;

	return CM_SUCCESS;
}

INT CmThreadSpace::GetThreadSpaceSize(UINT & width, UINT & height)
{
	width = m_Width;
	height = m_Height;

	return CM_SUCCESS;
}

INT CmThreadSpace::GetThreadSpaceUnit(CM_THREAD_SPACE_UNIT * &pThreadSpaceUnit)
{
	pThreadSpaceUnit = m_pThreadSpaceUnit;
	return CM_SUCCESS;
}

INT CmThreadSpace::GetDependency(CM_DEPENDENCY * &pDependency)
{
	pDependency = &m_Dependency;
	return CM_SUCCESS;
}

INT CmThreadSpace::GetDependencyPatternType(CM_HAL_DEPENDENCY_PATTERN &
					    DependencyPatternType)
{
	DependencyPatternType = m_DependencyPatternType;

	return CM_SUCCESS;
}

INT CmThreadSpace::Get26ZIDispatchPattern(CM_HAL_26ZI_DISPATCH_PATTERN &
					  pattern)
{
	pattern = m_26ZIDispatchPattern;

	return CM_SUCCESS;
}

INT CmThreadSpace::GetWalkingPattern(CM_HAL_WALKING_PATTERN & pWalkingPattern)
{
	pWalkingPattern = m_WalkingPattern;
	return CM_SUCCESS;
}

INT CmThreadSpace::GetWalkingParameters(CM_HAL_WALKING_PARAMETERS &
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

BOOLEAN CmThreadSpace::CheckWalkingParametersSet()
{
	return m_MediaWalkerParamsSet;
}

INT CmThreadSpace::GetDependencyVectors(CM_HAL_DEPENDENCY & pDependencyVectors)
{
	if (&pDependencyVectors == NULL) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	CmSafeMemCopy(&pDependencyVectors, &m_DependencyVectors,
		      sizeof(m_DependencyVectors));
	return CM_SUCCESS;
}

BOOLEAN CmThreadSpace::CheckDependencyVectorsSet()
{
	return m_DependencyVectorsSet;
}

INT CmThreadSpace::GetWavefront26ZDispatchInfo(CM_HAL_WAVEFRONT26Z_DISPATCH_INFO
					       & dispatchInfo)
{
	dispatchInfo = m_Wavefront26ZDispatchInfo;
	return CM_SUCCESS;
}

BOOLEAN CmThreadSpace::IntegrityCheck(CmTask * pTask)
{
	CmKernel *pKernel_RT = NULL;
	UINT i;
	UINT KernelCount = 0;
	UINT ThreadNumber = 0;
	UINT KernelIndex = 0;
	UINT unassociated = 0;
	INT hr = CM_SUCCESS;

	BOOLEAN **pTSMapping = NULL;
	BOOLEAN *pKernelInScoreboard = NULL;

	KernelCount = pTask->GetKernelCount();
	if (KernelCount > 1) {
		CM_ASSERTMESSAGE
		    ("pTS->IntegrityCheck Failed: ThreadSpace is not allowed in multi-kernel task.");
		return FALSE;
	}

	pKernel_RT = pTask->GetKernelPointer(0);
	CMCHK_NULL(pKernel_RT);

	pKernel_RT->GetThreadCount(ThreadNumber);

	if (ThreadNumber !=
	    (this->m_Width * this->m_Height * (this->m_ColorCountMinusOne + 1)))
	{
		CM_ASSERTMESSAGE
		    ("pTS->IntegrityCheck Failed: ThreadSpace size (width * height) is not matched with thread count.");
		return FALSE;
	}
	if (this->IsThreadAssociated()) {
		pTSMapping = new(std::nothrow) BOOLEAN *[KernelCount];
		pKernelInScoreboard = new(std::nothrow) BOOLEAN[KernelCount];

		CMCHK_NULL(pTSMapping);
		CMCHK_NULL(pKernelInScoreboard);

		CmSafeMemSet(pTSMapping, 0, KernelCount * sizeof(BOOLEAN *));
		CmSafeMemSet(pKernelInScoreboard, 0,
			     KernelCount * sizeof(BOOLEAN));

		for (i = 0; i < KernelCount; i++) {
			pKernel_RT = pTask->GetKernelPointer(i);
			CMCHK_NULL(pKernel_RT);
			pKernel_RT->GetThreadCount(ThreadNumber);

			pTSMapping[i] = new(std::nothrow) BOOLEAN[ThreadNumber];
			CMCHK_NULL(pTSMapping[i]);
			CmSafeMemSet(pTSMapping[i], 0,
				     ThreadNumber * sizeof(BOOLEAN));
			pKernelInScoreboard[i] = FALSE;
		}

		for (i = 0; i < m_Width * m_Height; i++) {
			pKernel_RT =
			    static_cast <
			    CmKernel * >(m_pThreadSpaceUnit[i].pKernel);
			if (pKernel_RT == NULL) {
				if (m_NeedSetKernelPointer) {
					pKernel_RT = *m_ppKernel;
				}
			}
			CMCHK_NULL(pKernel_RT);

			KernelIndex = pKernel_RT->GetIndexInTask();
			pTSMapping[KernelIndex][m_pThreadSpaceUnit[i].threadId]
			    = TRUE;
			pKernelInScoreboard[KernelIndex] = TRUE;
		}

		for (i = 0; i < KernelCount; i++) {
			if (pKernelInScoreboard[i] == TRUE) {
				pKernel_RT = pTask->GetKernelPointer(i);
				CMCHK_NULL(pKernel_RT);

				pKernel_RT->GetThreadCount(ThreadNumber);
				pKernel_RT->SetAssociatedToTSFlag(TRUE);
				for (UINT j = 0; j < ThreadNumber; j++) {
					if (pTSMapping[i][j] == FALSE) {
						unassociated++;
						break;
					}
				}
			}
			CmSafeDeleteArray(pTSMapping[i]);
		}

		if (unassociated != 0) {
			CM_ASSERT(0);
			hr = CM_FAILURE;
		}
	}

 finish:

	CmSafeDeleteArray(pTSMapping);
	CmSafeDeleteArray(pKernelInScoreboard);

	return (hr == CM_SUCCESS) ? TRUE : FALSE;
}

INT CmThreadSpace::Wavefront45Sequence()
{
	if (m_CurrentDependencyPattern == CM_DEPENDENCY_WAVEFRONT) {
		return CM_SUCCESS;
	}
	m_CurrentDependencyPattern = CM_DEPENDENCY_WAVEFRONT;

	CmSafeMemSet(m_pBoardFlag, WHITE, m_Width * m_Height * sizeof(UINT));
	m_IndexInList = 0;

	for (UINT y = 0; y < m_Height; y++) {
		for (UINT x = 0; x < m_Width; x++) {
			CM_COORDINATE temp_xy;
			INT linear_offset = y * m_Width + x;
			if (m_pBoardFlag[linear_offset] == WHITE) {
				m_pBoardOrderList[m_IndexInList++] =
				    linear_offset;
				m_pBoardFlag[linear_offset] = BLACK;
				temp_xy.x = x - 1;
				temp_xy.y = y + 1;
				while ((temp_xy.x >= 0) && (temp_xy.y >= 0) &&
				       (temp_xy.x < (INT) m_Width)
				       && (temp_xy.y < (INT) m_Height)) {
					if (m_pBoardFlag
					    [temp_xy.y * m_Width + temp_xy.x] ==
					    WHITE) {
						m_pBoardOrderList
						    [m_IndexInList++] =
						    temp_xy.y * m_Width +
						    temp_xy.x;
						m_pBoardFlag[temp_xy.y *
							     m_Width +
							     temp_xy.x] = BLACK;
					}
					temp_xy.x = temp_xy.x - 1;
					temp_xy.y = temp_xy.y + 1;
				}
			}
		}
	}

	return CM_SUCCESS;
}

INT CmThreadSpace::Wavefront26Sequence()
{
	if (m_CurrentDependencyPattern == CM_DEPENDENCY_WAVEFRONT26) {
		return CM_SUCCESS;
	}
	m_CurrentDependencyPattern = CM_DEPENDENCY_WAVEFRONT26;

	CmSafeMemSet(m_pBoardFlag, WHITE, m_Width * m_Height * sizeof(UINT));
	m_IndexInList = 0;

	for (UINT y = 0; y < m_Height; y++) {
		for (UINT x = 0; x < m_Width; x++) {
			CM_COORDINATE temp_xy;
			INT linear_offset = y * m_Width + x;
			if (m_pBoardFlag[linear_offset] == WHITE) {
				m_pBoardOrderList[m_IndexInList++] =
				    linear_offset;
				m_pBoardFlag[linear_offset] = BLACK;
				temp_xy.x = x - 2;
				temp_xy.y = y + 1;
				while ((temp_xy.x >= 0) && (temp_xy.y >= 0) &&
				       (temp_xy.x < (INT) m_Width)
				       && (temp_xy.y < (INT) m_Height)) {
					if (m_pBoardFlag
					    [temp_xy.y * m_Width + temp_xy.x] ==
					    WHITE) {
						m_pBoardOrderList
						    [m_IndexInList++] =
						    temp_xy.y * m_Width +
						    temp_xy.x;
						m_pBoardFlag[temp_xy.y *
							     m_Width +
							     temp_xy.x] = BLACK;
					}
					temp_xy.x = temp_xy.x - 2;
					temp_xy.y = temp_xy.y + 1;
				}
			}
		}
	}

	return CM_SUCCESS;
}

INT CmThreadSpace::Wavefront26ZSequence()
{
	if (m_CurrentDependencyPattern == CM_DEPENDENCY_WAVEFRONT26Z) {
		return CM_SUCCESS;
	}
	m_CurrentDependencyPattern = CM_DEPENDENCY_WAVEFRONT26Z;

	UINT threadsInWave = 0;
	UINT numWaves = 0;

	if ((m_Height % 2 != 0) || (m_Width % 2 != 0)) {
		return CM_INVALID_ARG_SIZE;
	}
	CmSafeMemSet(m_pBoardFlag, WHITE, m_Width * m_Height * sizeof(UINT));
	m_IndexInList = 0;

	UINT iX, iY, nOffset;
	iX = iY = nOffset = 0;

	UINT *pWaveFrontPos = new(std::nothrow) UINT[m_Width];
	UINT *pWaveFrontOffset = new(std::nothrow) UINT[m_Width];
	if ((pWaveFrontPos == NULL) || (pWaveFrontOffset == NULL)) {
		CmSafeDeleteArray(pWaveFrontPos);
		CmSafeDeleteArray(pWaveFrontOffset);
		return CM_FAILURE;
	}
	CmSafeMemSet(pWaveFrontPos, 0, m_Width * sizeof(int));

	m_pBoardFlag[0] = BLACK;
	m_pBoardOrderList[0] = 0;
	pWaveFrontPos[0] = 1;
	m_IndexInList = 0;

	CM_COORDINATE pMask[8];
	UINT nMaskNumber = 0;

	m_Wavefront26ZDispatchInfo.pNumThreadsInWave[numWaves] = 1;
	numWaves++;

	while (m_IndexInList < m_Width * m_Height - 1) {

		CmSafeMemSet(pWaveFrontOffset, 0, m_Width * sizeof(int));
		for (iX = 0; iX < m_Width; ++iX) {
			iY = pWaveFrontPos[iX];
			nOffset = iY * m_Width + iX;
			CmSafeMemSet(pMask, 0, sizeof(pMask));

			if (m_pBoardFlag[nOffset] == WHITE) {
				if ((iX % 2 == 0) && (iY % 2 == 0)) {
					if (iX == 0) {
						pMask[0].x = 0;
						pMask[0].y = -1;
						pMask[1].x = 1;
						pMask[1].y = -1;
						nMaskNumber = 2;
					} else if (iY == 0) {
						pMask[0].x = -1;
						pMask[0].y = 1;
						pMask[1].x = -1;
						pMask[1].y = 0;
						nMaskNumber = 2;
					} else {
						pMask[0].x = -1;
						pMask[0].y = 1;
						pMask[1].x = -1;
						pMask[1].y = 0;
						pMask[2].x = 0;
						pMask[2].y = -1;
						pMask[3].x = 1;
						pMask[3].y = -1;
						nMaskNumber = 4;
					}
				} else if ((iX % 2 == 0) && (iY % 2 == 1)) {
					if (iX == 0) {
						pMask[0].x = 0;
						pMask[0].y = -1;
						pMask[1].x = 1;
						pMask[1].y = -1;
						nMaskNumber = 2;
					} else {
						pMask[0].x = -1;
						pMask[0].y = 0;
						pMask[1].x = 0;
						pMask[1].y = -1;
						pMask[2].x = 1;
						pMask[2].y = -1;
						nMaskNumber = 3;
					}
				} else if ((iX % 2 == 1) && (iY % 2 == 0)) {
					if (iY == 0) {
						pMask[0].x = -1;
						pMask[0].y = 0;
						nMaskNumber = 1;
					} else if (iX == m_Width - 1) {
						pMask[0].x = -1;
						pMask[0].y = 0;
						pMask[1].x = 0;
						pMask[1].y = -1;
						nMaskNumber = 2;
					} else {
						pMask[0].x = -1;
						pMask[0].y = 0;
						pMask[1].x = 0;
						pMask[1].y = -1;
						pMask[2].x = 1;
						pMask[2].y = -1;
						nMaskNumber = 3;
					}
				} else {
					pMask[0].x = -1;
					pMask[0].y = 0;
					pMask[1].x = 0;
					pMask[1].y = -1;
					nMaskNumber = 2;
				}

				BOOL bAllInQueue = true;
				for (UINT i = 0; i < nMaskNumber; ++i) {
					if (m_pBoardFlag
					    [nOffset + pMask[i].x +
					     pMask[i].y * m_Width] == WHITE) {
						bAllInQueue = false;
						break;
					}
				}
				if (bAllInQueue) {
					pWaveFrontOffset[iX] = nOffset;
					if (pWaveFrontPos[iX] < m_Height - 1) {
						pWaveFrontPos[iX]++;
					}
				}
			}
		}

		for (UINT iX = 0; iX < m_Width; ++iX) {
			if ((m_pBoardFlag[pWaveFrontOffset[iX]] == WHITE)
			    && (pWaveFrontOffset[iX] != 0)) {
				m_IndexInList++;
				m_pBoardOrderList[m_IndexInList] =
				    pWaveFrontOffset[iX];
				m_pBoardFlag[pWaveFrontOffset[iX]] = BLACK;
				threadsInWave++;
			}
		}

		m_Wavefront26ZDispatchInfo.pNumThreadsInWave[numWaves] =
		    threadsInWave;
		threadsInWave = 0;
		numWaves++;
	}

	CmSafeDeleteArray(pWaveFrontPos);
	CmSafeDeleteArray(pWaveFrontOffset);

	m_Wavefront26ZDispatchInfo.numWaves = numWaves;

	return CM_SUCCESS;
}

INT CmThreadSpace::Wavefront26ZISeqVVHV26()
{
	if (m_CurrentDependencyPattern == CM_DEPENDENCY_WAVEFRONT26ZI &&
	    (m_Current26ZIDispatchPattern == VVERTICAL_HVERTICAL_26)) {
		return CM_SUCCESS;
	}

	m_CurrentDependencyPattern = CM_DEPENDENCY_WAVEFRONT26ZI;
	m_Current26ZIDispatchPattern = VVERTICAL_HVERTICAL_26;

	CmSafeMemSet(m_pBoardFlag, WHITE, m_Width * m_Height * sizeof(UINT));
	m_IndexInList = 0;

	for (UINT y = 0; y < m_Height; y = y + m_26ZIBlockHeight) {
		for (UINT x = 0; x < m_Width; x = x + m_26ZIBlockWidth) {
			CM_COORDINATE temp_xyFor26;
			temp_xyFor26.x = x;
			temp_xyFor26.y = y;

			do {
				if (m_pBoardFlag
				    [temp_xyFor26.y * m_Width +
				     temp_xyFor26.x] == WHITE) {
					m_pBoardOrderList[m_IndexInList++] =
					    temp_xyFor26.y * m_Width +
					    temp_xyFor26.x;
					m_pBoardFlag[temp_xyFor26.y * m_Width +
						     temp_xyFor26.x] = BLACK;

					for (UINT widthCount = 0;
					     widthCount < m_26ZIBlockWidth;
					     widthCount = widthCount + 2) {
						CM_COORDINATE temp_xy;
						UINT localHeightCounter = 0;

						temp_xy.x =
						    temp_xyFor26.x + widthCount;
						temp_xy.y = temp_xyFor26.y;
						while ((temp_xy.x >= 0)
						       && (temp_xy.y >= 0)
						       && (temp_xy.x <
							   (INT) m_Width)
						       && (temp_xy.y <
							   (INT) m_Height)
						       && (localHeightCounter <
							   m_26ZIBlockHeight)) {
							if (m_pBoardFlag
							    [temp_xy.y *
							     m_Width +
							     temp_xy.x] ==
							    WHITE) {
								m_pBoardOrderList
								    [m_IndexInList++]
								    =
								    temp_xy.y *
								    m_Width +
								    temp_xy.x;
								m_pBoardFlag
								    [temp_xy.y *
								     m_Width +
								     temp_xy.x]
								    = BLACK;
							}
							temp_xy.y =
							    temp_xy.y + 1;
							localHeightCounter++;
						}
					}

					for (UINT widthCount = 1;
					     widthCount < m_26ZIBlockWidth;
					     widthCount = widthCount + 2) {
						CM_COORDINATE temp_xy;
						UINT localHeightCounter = 0;

						temp_xy.x =
						    temp_xyFor26.x + widthCount;
						temp_xy.y = temp_xyFor26.y;
						while ((temp_xy.x >= 0)
						       && (temp_xy.y >= 0)
						       && (temp_xy.x <
							   (INT) m_Width)
						       && (temp_xy.y <
							   (INT) m_Height)
						       && (localHeightCounter <
							   m_26ZIBlockHeight)) {
							if (m_pBoardFlag
							    [temp_xy.y *
							     m_Width +
							     temp_xy.x] ==
							    WHITE) {
								m_pBoardOrderList
								    [m_IndexInList++]
								    =
								    temp_xy.y *
								    m_Width +
								    temp_xy.x;
								m_pBoardFlag
								    [temp_xy.y *
								     m_Width +
								     temp_xy.x]
								    = BLACK;
							}
							temp_xy.y =
							    temp_xy.y + 1;
							localHeightCounter++;
						}
					}
				}

				temp_xyFor26.x =
				    temp_xyFor26.x - (2 * m_26ZIBlockWidth);
				temp_xyFor26.y =
				    temp_xyFor26.y + (1 * m_26ZIBlockHeight);

			}
			while ((temp_xyFor26.x >= 0) && (temp_xyFor26.y >= 0)
			       && (temp_xyFor26.x < (INT) m_Width)
			       && (temp_xyFor26.y < (INT) m_Height));
		}
	}

	return CM_SUCCESS;
}

INT CmThreadSpace::Wavefront26ZISeqVVHH26()
{
	if (m_CurrentDependencyPattern == CM_DEPENDENCY_WAVEFRONT26ZI &&
	    (m_Current26ZIDispatchPattern == VVERTICAL_HHORIZONTAL_26)) {
		return CM_SUCCESS;
	}

	m_CurrentDependencyPattern = CM_DEPENDENCY_WAVEFRONT26ZI;
	m_Current26ZIDispatchPattern = VVERTICAL_HHORIZONTAL_26;

	CmSafeMemSet(m_pBoardFlag, WHITE, m_Width * m_Height * sizeof(UINT));
	m_IndexInList = 0;

	for (UINT y = 0; y < m_Height; y = y + m_26ZIBlockHeight) {
		for (UINT x = 0; x < m_Width; x = x + m_26ZIBlockWidth) {
			CM_COORDINATE temp_xyFor26;
			temp_xyFor26.x = x;
			temp_xyFor26.y = y;

			do {
				if (m_pBoardFlag
				    [temp_xyFor26.y * m_Width +
				     temp_xyFor26.x] == WHITE) {
					m_pBoardOrderList[m_IndexInList++] =
					    temp_xyFor26.y * m_Width +
					    temp_xyFor26.x;
					m_pBoardFlag[temp_xyFor26.y * m_Width +
						     temp_xyFor26.x] = BLACK;

					for (UINT widthCount = 0;
					     widthCount < m_26ZIBlockWidth;
					     widthCount = widthCount + 2) {
						CM_COORDINATE temp_xy;
						UINT localHeightCounter = 0;

						temp_xy.x =
						    temp_xyFor26.x + widthCount;
						temp_xy.y = temp_xyFor26.y;
						while ((temp_xy.x >= 0)
						       && (temp_xy.y >= 0)
						       && (temp_xy.x <
							   (INT) m_Width)
						       && (temp_xy.y <
							   (INT) m_Height)
						       && (localHeightCounter <
							   m_26ZIBlockHeight)) {
							if (m_pBoardFlag
							    [temp_xy.y *
							     m_Width +
							     temp_xy.x] ==
							    WHITE) {
								m_pBoardOrderList
								    [m_IndexInList++]
								    =
								    temp_xy.y *
								    m_Width +
								    temp_xy.x;
								m_pBoardFlag
								    [temp_xy.y *
								     m_Width +
								     temp_xy.x]
								    = BLACK;
							}
							temp_xy.y =
							    temp_xy.y + 1;
							localHeightCounter++;
						}
					}

					for (UINT heightCount = 0;
					     heightCount < m_26ZIBlockHeight;
					     ++heightCount) {
						CM_COORDINATE temp_xy;
						UINT localWidthCounter = 0;

						temp_xy.x = temp_xyFor26.x + 1;
						temp_xy.y =
						    temp_xyFor26.y +
						    heightCount;
						while ((temp_xy.x >= 0)
						       && (temp_xy.y >= 0)
						       && (temp_xy.x <
							   (INT) m_Width)
						       && (temp_xy.y <
							   (INT) m_Height)
						       && (localWidthCounter <
							   (m_26ZIBlockWidth /
							    2))) {
							if (m_pBoardFlag
							    [temp_xy.y *
							     m_Width +
							     temp_xy.x] ==
							    WHITE) {
								m_pBoardOrderList
								    [m_IndexInList++]
								    =
								    temp_xy.y *
								    m_Width +
								    temp_xy.x;
								m_pBoardFlag
								    [temp_xy.y *
								     m_Width +
								     temp_xy.x]
								    = BLACK;
							}

							temp_xy.x =
							    temp_xy.x + 2;
							localWidthCounter++;
						}
					}
				}

				temp_xyFor26.x =
				    temp_xyFor26.x - (2 * m_26ZIBlockWidth);
				temp_xyFor26.y =
				    temp_xyFor26.y + (1 * m_26ZIBlockHeight);

			}
			while ((temp_xyFor26.x >= 0) && (temp_xyFor26.y >= 0)
			       && (temp_xyFor26.x < (INT) m_Width)
			       && (temp_xyFor26.y < (INT) m_Height));
		}
	}

	return CM_SUCCESS;
}

INT CmThreadSpace::Wavefront26ZISeqVV26HH26()
{
	if ((m_CurrentDependencyPattern == CM_DEPENDENCY_WAVEFRONT26ZI) &&
	    (m_Current26ZIDispatchPattern == VVERTICAL26_HHORIZONTAL26)) {
		return CM_SUCCESS;
	}

	m_CurrentDependencyPattern = CM_DEPENDENCY_WAVEFRONT26ZI;
	m_Current26ZIDispatchPattern = VVERTICAL26_HHORIZONTAL26;

	CmSafeMemSet(m_pBoardFlag, WHITE, m_Width * m_Height * sizeof(UINT));
	m_IndexInList = 0;

	UINT waveFrontNum = 0;
	UINT waveFrontStartX = 0;
	UINT waveFrontStartY = 0;

	UINT adjustHeight = 0;

	CM_COORDINATE temp_xyFor26;
	temp_xyFor26.x = 0;
	temp_xyFor26.y = 0;

	while ((temp_xyFor26.x >= 0) && (temp_xyFor26.y >= 0) &&
	       (temp_xyFor26.x < (INT) m_Width)
	       && (temp_xyFor26.y < (INT) m_Height)) {
		CM_COORDINATE temp_xyForHorz;
		temp_xyForHorz.x = temp_xyFor26.x;
		temp_xyForHorz.y = temp_xyFor26.y;

		do {
			CM_COORDINATE temp_xyForVer;

			for (UINT widthCount = 0; widthCount < m_26ZIBlockWidth;
			     widthCount += 2) {
				UINT localHeightCounter = 0;
				temp_xyForVer.x = temp_xyFor26.x + widthCount;
				temp_xyForVer.y = temp_xyFor26.y;

				while ((temp_xyForVer.x < (INT) m_Width)
				       && (temp_xyForVer.y < (INT) m_Height)
				       && (temp_xyForVer.x >= 0)
				       && (temp_xyForVer.y >= 0)
				       && (localHeightCounter <
					   m_26ZIBlockHeight)) {
					if (m_pBoardFlag
					    [temp_xyForVer.y * m_Width +
					     temp_xyForVer.x] == WHITE) {
						m_pBoardOrderList
						    [m_IndexInList++] =
						    temp_xyForVer.y * m_Width +
						    temp_xyForVer.x;
						m_pBoardFlag[temp_xyForVer.y *
							     m_Width +
							     temp_xyForVer.x] =
						    BLACK;
					}
					temp_xyForVer.y += 1;
					localHeightCounter++;
				}
			}

			temp_xyFor26.x =
			    temp_xyFor26.x + (2 * m_26ZIBlockWidth);
			temp_xyFor26.y =
			    temp_xyFor26.y - (1 * m_26ZIBlockHeight);

		}
		while ((temp_xyFor26.x >= 0) && (temp_xyFor26.y >= 0) &&
		       (temp_xyFor26.x < (INT) m_Width)
		       && (temp_xyFor26.y < (INT) m_Height));

		temp_xyFor26.x = temp_xyForHorz.x;
		temp_xyFor26.y = temp_xyForHorz.y;

		do {
			for (UINT heightCount = 0;
			     heightCount < m_26ZIBlockHeight; ++heightCount) {
				UINT localWidthCounter = 0;
				temp_xyForHorz.x = temp_xyFor26.x + 1;
				temp_xyForHorz.y = temp_xyFor26.y + heightCount;
				while ((temp_xyForHorz.x >= 0)
				       && (temp_xyForHorz.y >= 0)
				       && (temp_xyForHorz.x < (INT) m_Width)
				       && (temp_xyForHorz.y < (INT) m_Height)
				       && (localWidthCounter <
					   (m_26ZIBlockWidth / 2))) {
					if (m_pBoardFlag
					    [temp_xyForHorz.y * m_Width +
					     temp_xyForHorz.x] == WHITE) {
						m_pBoardOrderList
						    [m_IndexInList++] =
						    temp_xyForHorz.y * m_Width +
						    temp_xyForHorz.x;
						m_pBoardFlag[temp_xyForHorz.y *
							     m_Width +
							     temp_xyForHorz.x] =
						    BLACK;
					}

					temp_xyForHorz.x += 2;
					localWidthCounter++;
				}
			}

			temp_xyFor26.x =
			    temp_xyFor26.x + (2 * m_26ZIBlockWidth);
			temp_xyFor26.y =
			    temp_xyFor26.y - (1 * m_26ZIBlockHeight);

		}
		while ((temp_xyFor26.x >= 0) && (temp_xyFor26.y >= 0) &&
		       (temp_xyFor26.x < (INT) m_Width)
		       && (temp_xyFor26.y < (INT) m_Height));

		if (m_Width <= m_26ZIBlockWidth) {
			temp_xyFor26.x = 0;
			temp_xyFor26.y = temp_xyForHorz.y + m_26ZIBlockHeight;
		} else {
			waveFrontNum++;
			adjustHeight =
			    (UINT) ceil((double)m_Height / m_26ZIBlockHeight);
			if (waveFrontNum < (2 * adjustHeight)) {
				waveFrontStartX = waveFrontNum & 1;
				waveFrontStartY =
				    (UINT) floor((double)waveFrontNum / 2);
			} else {
				waveFrontStartX =
				    (waveFrontNum - 2 * adjustHeight) + 2;
				waveFrontStartY = (adjustHeight) - 1;
			}
			temp_xyFor26.x = waveFrontStartX * m_26ZIBlockWidth;
			temp_xyFor26.y = waveFrontStartY * m_26ZIBlockHeight;
		}
	}

	return CM_SUCCESS;
}

INT CmThreadSpace::Wavefront26ZISeqVV1x26HH1x26()
{
	if ((m_CurrentDependencyPattern == CM_DEPENDENCY_WAVEFRONT26ZI) &&
	    (m_Current26ZIDispatchPattern == VVERTICAL1X26_HHORIZONTAL1X26)) {
		return CM_SUCCESS;
	}

	m_CurrentDependencyPattern = CM_DEPENDENCY_WAVEFRONT26ZI;
	m_Current26ZIDispatchPattern = VVERTICAL1X26_HHORIZONTAL1X26;

	CmSafeMemSet(m_pBoardFlag, WHITE, m_Width * m_Height * sizeof(UINT));
	m_IndexInList = 0;

	UINT waveFrontNum = 0;
	UINT waveFrontStartX = 0;
	UINT waveFrontStartY = 0;

	UINT adjustHeight = 0;

	CM_COORDINATE temp_xyFor26;
	temp_xyFor26.x = 0;
	temp_xyFor26.y = 0;

	CM_COORDINATE saveTemp_xyFor26;
	saveTemp_xyFor26.x = 0;
	saveTemp_xyFor26.y = 0;

	CM_COORDINATE temp_xyForVer;
	CM_COORDINATE temp_xyForHorz;

	while ((temp_xyFor26.x >= 0) && (temp_xyFor26.y >= 0) &&
	       (temp_xyFor26.x < (INT) m_Width)
	       && (temp_xyFor26.y < (INT) m_Height)) {
		saveTemp_xyFor26.x = temp_xyFor26.x;
		saveTemp_xyFor26.y = temp_xyFor26.y;

		for (UINT widthCount = 0; widthCount < m_26ZIBlockWidth;
		     widthCount += 2) {
			temp_xyFor26.x = saveTemp_xyFor26.x;
			temp_xyFor26.y = saveTemp_xyFor26.y;

			do {
				UINT localHeightCounter = 0;
				temp_xyForVer.x = temp_xyFor26.x + widthCount;
				temp_xyForVer.y = temp_xyFor26.y;
				while ((temp_xyForVer.x < (INT) m_Width)
				       && (temp_xyForVer.y < (INT) m_Height)
				       && (temp_xyForVer.x >= 0)
				       && (temp_xyForVer.y >= 0)
				       && (localHeightCounter <
					   m_26ZIBlockHeight)) {
					if (m_pBoardFlag
					    [temp_xyForVer.y * m_Width +
					     temp_xyForVer.x] == WHITE) {
						m_pBoardOrderList
						    [m_IndexInList++] =
						    temp_xyForVer.y * m_Width +
						    temp_xyForVer.x;
						m_pBoardFlag[temp_xyForVer.y *
							     m_Width +
							     temp_xyForVer.x] =
						    BLACK;
					}
					temp_xyForVer.y += 1;
					localHeightCounter++;
				}

				temp_xyFor26.x =
				    temp_xyFor26.x + (2 * m_26ZIBlockWidth);
				temp_xyFor26.y =
				    temp_xyFor26.y - (1 * m_26ZIBlockHeight);

			}
			while ((temp_xyFor26.x >= 0) && (temp_xyFor26.y >= 0) &&
			       (temp_xyFor26.x < (INT) m_Width)
			       && (temp_xyFor26.y < (INT) m_Height));
		}

		temp_xyFor26.x = saveTemp_xyFor26.x;
		temp_xyFor26.y = saveTemp_xyFor26.y;

		for (UINT heightCount = 0; heightCount < m_26ZIBlockHeight;
		     ++heightCount) {
			temp_xyFor26.x = saveTemp_xyFor26.x;
			temp_xyFor26.y = saveTemp_xyFor26.y;

			do {
				UINT localWidthCounter = 0;
				temp_xyForHorz.x = temp_xyFor26.x + 1;
				temp_xyForHorz.y = temp_xyFor26.y + heightCount;
				while ((temp_xyForHorz.x >= 0)
				       && (temp_xyForHorz.y >= 0)
				       && (temp_xyForHorz.x < (INT) m_Width)
				       && (temp_xyForHorz.y < (INT) m_Height)
				       && (localWidthCounter <
					   (m_26ZIBlockWidth / 2))) {
					if (m_pBoardFlag
					    [temp_xyForHorz.y * m_Width +
					     temp_xyForHorz.x] == WHITE) {
						m_pBoardOrderList
						    [m_IndexInList++] =
						    temp_xyForHorz.y * m_Width +
						    temp_xyForHorz.x;
						m_pBoardFlag[temp_xyForHorz.y *
							     m_Width +
							     temp_xyForHorz.x] =
						    BLACK;
					}

					temp_xyForHorz.x += 2;
					localWidthCounter++;
				}

				temp_xyFor26.x =
				    temp_xyFor26.x + (2 * m_26ZIBlockWidth);
				temp_xyFor26.y =
				    temp_xyFor26.y - (1 * m_26ZIBlockHeight);

			}
			while ((temp_xyFor26.x >= 0) && (temp_xyFor26.y >= 0) &&
			       (temp_xyFor26.x < (INT) m_Width)
			       && (temp_xyFor26.y < (INT) m_Height));

		}
		if (m_Width <= m_26ZIBlockWidth) {
			temp_xyFor26.x = 0;
			temp_xyFor26.y = saveTemp_xyFor26.y + m_26ZIBlockHeight;
		} else {
			waveFrontNum++;
			adjustHeight =
			    (UINT) ceil((double)m_Height / m_26ZIBlockHeight);
			if (waveFrontNum < (2 * adjustHeight)) {
				waveFrontStartX = waveFrontNum & 1;
				waveFrontStartY =
				    (UINT) floor((double)waveFrontNum / 2);
			} else {
				waveFrontStartX =
				    (waveFrontNum - 2 * adjustHeight) + 2;
				waveFrontStartY = (adjustHeight) - 1;
			}
			temp_xyFor26.x = waveFrontStartX * m_26ZIBlockWidth;
			temp_xyFor26.y = waveFrontStartY * m_26ZIBlockHeight;
		}
	}

	return CM_SUCCESS;
}

INT CmThreadSpace::VerticalSequence()
{
	if (m_CurrentDependencyPattern == CM_DEPENDENCY_VERTICAL) {
		return CM_SUCCESS;
	}
	m_CurrentDependencyPattern = CM_DEPENDENCY_VERTICAL;

	CmSafeMemSet(m_pBoardFlag, WHITE, m_Width * m_Height * sizeof(UINT));
	m_IndexInList = 0;

	for (UINT x = 0; x < m_Width; x++) {
		for (UINT y = 0; y < m_Height; y++) {
			CM_COORDINATE temp_xy;
			INT linear_offset = y * m_Width + x;
			if (m_pBoardFlag[linear_offset] == WHITE) {
				m_pBoardOrderList[m_IndexInList++] =
				    linear_offset;
				m_pBoardFlag[linear_offset] = BLACK;
				temp_xy.x = x;
				temp_xy.y = y + 1;
				while ((temp_xy.x >= 0) && (temp_xy.y >= 0) &&
				       (temp_xy.x < (INT) m_Width)
				       && (temp_xy.y < (INT) m_Height)) {
					if (m_pBoardFlag
					    [temp_xy.y * m_Width + temp_xy.x] ==
					    WHITE) {
						m_pBoardOrderList
						    [m_IndexInList++] =
						    temp_xy.y * m_Width +
						    temp_xy.x;
						m_pBoardFlag[temp_xy.y *
							     m_Width +
							     temp_xy.x] = BLACK;
					}
					temp_xy.y = temp_xy.y + 1;
				}
			}
		}
	}

	return CM_SUCCESS;
}

INT CmThreadSpace::HorizentalSequence()
{
	if (m_CurrentDependencyPattern == CM_DEPENDENCY_HORIZONTAL) {
		return CM_SUCCESS;
	}
	m_CurrentDependencyPattern = CM_DEPENDENCY_HORIZONTAL;

	CmSafeMemSet(m_pBoardFlag, WHITE, m_Width * m_Height * sizeof(UINT));
	m_IndexInList = 0;

	for (UINT y = 0; y < m_Height; y++) {
		for (UINT x = 0; x < m_Width; x++) {
			CM_COORDINATE temp_xy;
			INT linear_offset = y * m_Width + x;
			if (m_pBoardFlag[linear_offset] == WHITE) {
				m_pBoardOrderList[m_IndexInList++] =
				    linear_offset;
				m_pBoardFlag[linear_offset] = BLACK;
				temp_xy.x = x + 1;
				temp_xy.y = y;
				while ((temp_xy.x >= 0) && (temp_xy.y >= 0) &&
				       (temp_xy.x < (INT) m_Width)
				       && (temp_xy.y < (INT) m_Height)) {
					if (m_pBoardFlag
					    [temp_xy.y * m_Width + temp_xy.x] ==
					    WHITE) {
						m_pBoardOrderList
						    [m_IndexInList++] =
						    temp_xy.y * m_Width +
						    temp_xy.x;
						m_pBoardFlag[temp_xy.y *
							     m_Width +
							     temp_xy.x] = BLACK;
					}
					temp_xy.x = temp_xy.x + 1;
				}
			}
		}
	}

	return CM_SUCCESS;
}

INT CmThreadSpace::GetBoardOrder(UINT * &pBoardOrder)
{
	pBoardOrder = m_pBoardOrderList;
	return CM_SUCCESS;
}

BOOLEAN CmThreadSpace::IsThreadAssociated() const {
	return m_ThreadAssociated;
} BOOLEAN CmThreadSpace::IsDependencySet()
{
	return ((m_DependencyPatternType != CM_DEPENDENCY_NONE) ? TRUE : FALSE);
}

BOOLEAN CmThreadSpace::GetNeedSetKernelPointer() const {
	return m_NeedSetKernelPointer;
} INT CmThreadSpace::SetKernelPointer(CmKernel * pKernel) const {
	*m_ppKernel = pKernel;
	return CM_SUCCESS;
} BOOLEAN CmThreadSpace::KernelPointerIsNULL() const {
	if (*m_ppKernel == NULL)
{
return TRUE;
	} else {
		return FALSE;
	}
}

CmKernel *CmThreadSpace::GetKernelPointer() const {
	return *m_ppKernel;
} UINT CmThreadSpace::GetIndexInTsArray()
{
	return m_IndexInTsArray;
}

CM_THREAD_SPACE_DIRTY_STATUS CmThreadSpace::GetDirtyStatus() const {
	return *m_pDirtyStatus;
} UINT CmThreadSpace::SetDirtyStatus(CM_THREAD_SPACE_DIRTY_STATUS DirtyStatus) const {
	*m_pDirtyStatus = DirtyStatus;
	return CM_SUCCESS;
}
