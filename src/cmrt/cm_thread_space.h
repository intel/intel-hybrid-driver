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

class CmDevice;
class CmKernel;
class CmTask;

class CmThreadSpace {
 public:
	static INT Create(CmDevice * pDevice, UINT indexTsArray, UINT width,
			  UINT height, CmThreadSpace * &pTS);
	static INT Destroy(CmThreadSpace * &pTS);

	CM_RT_API INT AssociateThread(UINT x, UINT y, CmKernel * pKernel,
				      UINT threadId, BYTE dependencyMask);
	CM_RT_API INT SetThreadDependencyPattern(UINT count, INT * deltaX,
						 INT * deltaY);
	CM_RT_API INT SelectThreadDependencyPattern(CM_HAL_DEPENDENCY_PATTERN
						    pattern);
	CM_RT_API INT SetThreadSpaceColorCount(UINT colorCount);
	CM_RT_API INT SelectMediaWalkingPattern(CM_HAL_WALKING_PATTERN pattern);
	CM_RT_API INT Set26ZIDispatchPattern(CM_HAL_26ZI_DISPATCH_PATTERN
					     pattern);
	CM_RT_API INT Set26ZIMacroBlockSize(UINT width, UINT height);
	CM_RT_API INT SelectMediaWalkingParameters(CM_HAL_WALKING_PARAMETERS
						   parameters);
	CM_RT_API INT SelectThreadDependencyVectors(CM_HAL_DEPENDENCY
						    dependencyVectors);

	INT GetThreadSpaceSize(UINT & width, UINT & height);
	INT GetThreadSpaceUnit(CM_THREAD_SPACE_UNIT * &pThreadSpaceUnit);
	INT GetDependency(CM_DEPENDENCY * &pDependency);
	INT GetDependencyPatternType(CM_HAL_DEPENDENCY_PATTERN &
				     DependencyPatternType);
	INT GetWalkingPattern(CM_HAL_WALKING_PATTERN & pWalkingPattern);
	INT Get26ZIDispatchPattern(CM_HAL_26ZI_DISPATCH_PATTERN & pattern);
	INT GetWalkingParameters(CM_HAL_WALKING_PARAMETERS &
				 pWalkingParameters);
	INT GetDependencyVectors(CM_HAL_DEPENDENCY & pDependencyVectors);
	BOOLEAN CheckWalkingParametersSet();
	BOOLEAN CheckDependencyVectorsSet();
	INT GetColorCountMinusOne(UINT & colorCount);
	INT GetWavefront26ZDispatchInfo(CM_HAL_WAVEFRONT26Z_DISPATCH_INFO &
					dispatchInfo);
	BOOLEAN IntegrityCheck(CmTask * pTask);
	INT GetBoardOrder(UINT * &pBoardOrder);
	INT Wavefront45Sequence();
	INT Wavefront26Sequence();
	INT Wavefront26ZSequence();
	INT Wavefront26ZISeqVVHV26();
	INT Wavefront26ZISeqVVHH26();
	INT Wavefront26ZISeqVV26HH26();
	INT Wavefront26ZISeqVV1x26HH1x26();
	INT HorizentalSequence();
	INT VerticalSequence();

	BOOLEAN IsThreadAssociated() const;
	BOOLEAN IsDependencySet();

	UINT GetIndexInTsArray();
	CM_THREAD_SPACE_DIRTY_STATUS GetDirtyStatus() const;
	UINT SetDirtyStatus(CM_THREAD_SPACE_DIRTY_STATUS DirtyStatus) const;

	BOOLEAN GetNeedSetKernelPointer() const;
	INT SetKernelPointer(CmKernel * pKernel) const;
	BOOLEAN KernelPointerIsNULL() const;
	CmKernel *GetKernelPointer() const;

 protected:
	 CmThreadSpace(CmDevice * pDevice, UINT indexTsArray, UINT width,
		       UINT height);
	~CmThreadSpace(void);

	INT Initialize(void);

	CmDevice *m_pDevice;

	UINT m_Width;
	UINT m_Height;
	UINT m_ColorCountMinusOne;

	UINT m_26ZIBlockWidth;
	UINT m_26ZIBlockHeight;

	CM_THREAD_SPACE_UNIT *m_pThreadSpaceUnit;
	BOOLEAN m_ThreadAssociated;

	BOOLEAN m_NeedSetKernelPointer;
	CmKernel **m_ppKernel;

	CM_HAL_DEPENDENCY_PATTERN m_DependencyPatternType;
	CM_HAL_DEPENDENCY_PATTERN m_CurrentDependencyPattern;
	CM_DEPENDENCY m_Dependency;
	CM_HAL_26ZI_DISPATCH_PATTERN m_26ZIDispatchPattern;
	CM_HAL_26ZI_DISPATCH_PATTERN m_Current26ZIDispatchPattern;

	UINT *m_pBoardFlag;
	UINT *m_pBoardOrderList;
	UINT m_IndexInList;
	UINT m_IndexInTsArray;

	CM_HAL_WALKING_PATTERN m_WalkingPattern;
	DWORD m_WalkingParameters[CM_NUM_DWORD_FOR_MW_PARAM];
	BOOLEAN m_MediaWalkerParamsSet;
	CM_HAL_DEPENDENCY m_DependencyVectors;
	BOOLEAN m_DependencyVectorsSet;

 private:
	 CmThreadSpace(const CmThreadSpace & other);
	 CmThreadSpace & operator=(const CmThreadSpace & other);

	PCM_THREAD_SPACE_DIRTY_STATUS m_pDirtyStatus;

	CM_HAL_WAVEFRONT26Z_DISPATCH_INFO m_Wavefront26ZDispatchInfo;
};
