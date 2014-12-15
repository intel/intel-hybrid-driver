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
#include "cm_array.h"
#include "cm_program.h"

class CmDevice;
class CmKernelData;
class CmThreadSpace;
class CmThreadGroupSpace;

class CmKernel {
 public:

	static INT Create(CmDevice * pCmDev, CmProgram * pProgram,
			  const char *kernelName, UINT KernelIndex,
			  UINT KernelSeqNum, CmKernel * &pKernel,
			  const char *options);
	static INT Destroy(CmKernel * &pKernel, CmProgram * &pProgram);

	INT GetBinary(void *&pBinary, UINT & size);
	INT GetThreadCount(UINT & count);

	CM_RT_API INT SetThreadCount(UINT count);
	CM_RT_API INT SetKernelArg(UINT index, size_t size, const void *pValue);

	CM_RT_API INT SetThreadArg(UINT threadId, UINT index, size_t size,
				   const void *pValue);
	CM_RT_API INT SetThreadDependencyMask(UINT threadId, BYTE mask);

	CM_RT_API INT SetStaticBuffer(UINT index, const void *pValue);
	CM_RT_API INT SetKernelPayloadData(size_t size, const void *pValue);
	CM_RT_API INT SetKernelPayloadSurface(UINT surfaceCount,
					      SurfaceIndex ** pSurfaces);

	CM_RT_API INT SetSurfaceBTI(SurfaceIndex * pSurface, UINT BTIndex);
	CM_RT_API INT AssociateThreadSpace(CmThreadSpace * &pThreadSpace);
	CM_RT_API INT AssociateThreadGroupSpace(CmThreadGroupSpace * &pTGS);

	INT GetArgs(CM_ARG * &pArg);
	INT GetArgCount(UINT & argCount);

	INT GetCurbeEnable(BOOL & b);
	INT SetCurbeEnable(BOOL b);
	INT GetSizeInCurbe(UINT & size);
	UINT GetAlignedCurbeSize(UINT value);
	INT GetCmDevice(CmDevice * &);
	INT GetCmProgram(CmProgram * &);
	INT GetSizeInPayload(UINT & size);

	INT CreateKernelData(CmKernelData * &pKernelData, UINT & kernelDataSize,
			     const CmThreadSpace * pTS);
	INT CreateKernelData(CmKernelData * &pKernelData, UINT & kernelDataSize,
			     const CmThreadGroupSpace * pTGS);

	char *GetName(void) {
		return (char *)m_pKernelInfo->kernelName;
	} INT SetIndexInTask(UINT index);
	UINT GetIndexInTask();
	INT SetAssociatedToTSFlag(BOOLEAN b);
	BOOLEAN IsThreadArgExisted();
	UINT GetKernelIndex();

	INT GetThreadSpace(CmThreadSpace * &pThreadSpace) {
		pThreadSpace = m_pThreadSpace;
		return CM_SUCCESS;
	}
	INT SetAdjustedYCoord(UINT value) {
		m_adjustScoreboardY = value;
		return CM_SUCCESS;
	}
	INT GetAdjustedYCoord() {
		return m_adjustScoreboardY;
	}

	UINT GetSLMSize(void);

	INT Acquire(void);
	INT SafeRelease(void);
	INT CollectKernelSurface();
	INT GetKernelSurfaces(BOOL * &surfArray);
	INT ResetKernelSurfaces();
	INT CalculateKernelSurfacesNum(UINT & kernelSurfaceNum,
				       UINT & neededBTEntryNum);

	UINT GetKernelGenxBinarySize(void);

 protected:
	CmKernel(CmDevice * pCmDev, CmProgram * pProgram, UINT KernelIndex,
		 UINT KernelSeqNum);
	~CmKernel(void);

	INT SetArgsInternal(CM_KERNEL_INTERNAL_ARG_TYPE nArgType, UINT index,
			    size_t size, const void *pValue, UINT nThreadID =
			    0);
	INT Initialize(const char *kernelName, const char *options);
	INT DestroyArgs(void);
	INT Reset(void);
	INT IsKernelDataReusable(CmThreadSpace * pTS);
	INT CreateKernelArgDataGroup(PBYTE & pData, UINT Value);

	INT CreateMovInstructions(UINT & movInstNum, PBYTE & pCodeDst,
				  CM_ARG * pTempArgs, UINT NumArgs);
	INT CalcKernelDataSize(UINT MovInsNum, UINT NumArgs, UINT ArgSize,
			       UINT & TotalKernelDataSize);
	INT GetArgCountPlusSurfArray(UINT & ArgSize, UINT & ArgCountPlus);

	INT CreateKernelDataInternal(CmKernelData * &pKernelData,
				     UINT & kernelDataSize,
				     const CmThreadSpace * pTS);
	INT CreateKernelDataInternal(CmKernelData * &pKernelData,
				     UINT & kernelDataSize,
				     const CmThreadGroupSpace * pTGS);

	INT UpdateKernelData(CmKernelData * pKernelData,
			     const CmThreadSpace * pTS);
	INT UpdateKernelData(CmKernelData * pKernelData,
			     const CmThreadGroupSpace * pTGS);

	INT CreateThreadArgData(PCM_HAL_KERNEL_ARG_PARAM pKernelArg,
				UINT ThreadArgIndex,
				CmThreadSpace * pThreadSpace,
				BOOL isKernelThreadSpace, CM_ARG * pCmArgs);

	INT UpdateLastKernelData(CmKernelData * &pKernelData);
	INT CreateKernelIndirectData(PCM_HAL_INDIRECT_DATA_PARAM
				     pHalIndreictData);

	INT CreateThreadSpaceParam(PCM_HAL_KERNEL_THREADSPACE_PARAM
				   pCmKernelThreadSpaceParam,
				   CmThreadSpace * pThreadSpace);
	INT CreateTempArgs(UINT NumofArgs, CM_ARG * &pTempArgs);

	INT SortThreadSpace(CmThreadSpace * pThreadSpace);
	INT CleanArgDirtyFlag();

	BOOL IsBatchBufferReusable(CmThreadSpace * pTaskThreadSpace);
	BOOL IsPrologueDirty(void);
	void DumpKernelData(CmKernelData * pKernelData);

	INT UpdateKernelDataGlobalSurfaceInfo(PCM_HAL_KERNEL_PARAM
					      pHalKernelParam);
	CM_ARG_KIND SurfTypeToArgKind(CM_ENUM_CLASS_TYPE SurfType);

	CmDevice *m_pCmDev;
	CmProgram *m_pProgram;
	char *m_Options;
	char *m_pBinary;
	UINT m_uiBinarySize;

	UINT m_ThreadCount;
	UINT m_LastThreadCount;
	UINT m_SizeInCurbe;
	UINT m_SizeInPayload;
	UINT m_ArgCount;

	CM_ARG *m_Args;
	SurfaceIndex *m_GlobalSurfaces[CM_GLOBAL_SURFACE_NUMBER];
	UINT m_GlobalCmIndex[CM_GLOBAL_SURFACE_NUMBER];
	CM_KERNEL_INFO *m_pKernelInfo;
	UINT m_kernelIndexInProgram;

	BOOL m_CurbeEnable;
	BOOL m_NonstallingScoreboardEnable;

	UINT64 m_Id;
	UINT m_Dirty;
	CmKernelData *m_pLastKernelData;
	UINT m_LastKernelDataSize;

	UINT m_IndexInTask;
	BOOL m_AssociatedToTS;
	BOOL m_blPerThreadArgExists;
	BOOL m_blPerKernelArgExists;

	CmThreadSpace *m_pThreadSpace;
	UINT m_adjustScoreboardY;
	UINT m_LastAdjustScoreboardY;

	BOOL m_blhwDebugEnable;

	USHORT m_usKernelPayloadDataSize;
	BYTE *m_pKernelPayloadData;

	USHORT m_usKernelPayloadSurfaceCount;
	SurfaceIndex
	    * m_pKernelPayloadSurfaceArray[CM_MAX_STATIC_SURFACE_STATES_PER_BT];
	CM_INDIRECT_SURFACE_INFO
	    m_IndirectSurfaceInfoArray[CM_MAX_STATIC_SURFACE_STATES_PER_BT];

	UINT m_refcount;

	CM_HAL_MAX_VALUES *m_pHalMaxValues;
	CM_HAL_MAX_VALUES_EX *m_pHalMaxValuesEx;
	BOOL *m_SurfaceArray;

	UINT m_kernelIndex;

	CmThreadGroupSpace *m_pThreadGroupSpace;

 private:
	CmKernel(const CmKernel & other);
	CmKernel & operator=(const CmKernel & other);
};
