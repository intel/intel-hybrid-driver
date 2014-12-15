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
#include "cm_kernel_data.h"
#include "cm_device.h"

INT CmKernelData::Create(CmKernel * pCmKernel, CmKernelData * &pKernelData)
{
	if (!pCmKernel) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	INT result = CM_SUCCESS;
	pKernelData = new(std::nothrow) CmKernelData(pCmKernel);
	if (pKernelData) {
		pKernelData->Acquire();
		result = pKernelData->Initialize();
		if (result != CM_SUCCESS) {
			CmKernelData::Destroy(pKernelData);
		}
	} else {
		CM_ASSERT(0);
		result = CM_OUT_OF_HOST_MEMORY;
	}
	return result;
}

INT CmKernelData::Destroy(CmKernelData * &pKernelData)
{
	if (pKernelData) {
		UINT refCount;
		refCount = pKernelData->SafeRelease();
		if (refCount == 0) {
			pKernelData = NULL;
		}
	}

	return CM_SUCCESS;
}

 CmKernelData::CmKernelData(CmKernel * pCmKernel):
m_kerneldatasize(0),
m_pCmKernel(pCmKernel), m_RefCount(0), m_KernelRef(0), m_IsInUse(TRUE)
{
	CmSafeMemSet(&m_HalKernelParam, 0, sizeof(CM_HAL_KERNEL_PARAM));
}

CmKernelData::~CmKernelData(void)
{
	for (UINT i = 0; i < m_HalKernelParam.iNumArgs; i++) {
		CmSafeDeleteArray(m_HalKernelParam.CmArgParams[i].pFirstValue);
	}

	CmSafeDeleteArray(m_HalKernelParam.CmIndirectDataParam.pIndirectData);
	CmSafeDeleteArray(m_HalKernelParam.CmIndirectDataParam.pSurfaceInfo);

	CmSafeDeleteArray(m_HalKernelParam.
			  CmKernelThreadSpaceParam.dispatchInfo.
			  pNumThreadsInWave);
	CmSafeDeleteArray(m_HalKernelParam.CmKernelThreadSpaceParam.
			  pThreadCoordinates);

	CmSafeDeleteArray(m_HalKernelParam.pMovInsData);

}

INT CmKernelData::Initialize(void)
{

	return CM_SUCCESS;
}

INT CmKernelData::GetCmKernel(CmKernel * &pCmKernel)
{
	pCmKernel = m_pCmKernel;
	return CM_SUCCESS;
}

INT CmKernelData::SetKernelDataSize(INT value)
{
	m_kerneldatasize = value;
	return CM_SUCCESS;
}

INT CmKernelData::GetKernelDataSize()
{
	return m_kerneldatasize;
}

UINT CmKernelData::Acquire(void)
{
	++m_RefCount;

	m_IsInUse = TRUE;

	return m_RefCount;
}

UINT CmKernelData::AcquireKernel(void)
{
	CmDevice *pDevice = NULL;
	m_pCmKernel->GetCmDevice(pDevice);

	CSync *pKernelLock = pDevice->GetProgramKernelLock();
	CLock locker(*pKernelLock);

	m_pCmKernel->Acquire();
	CmProgram *pCmProgram = NULL;
	m_pCmKernel->GetCmProgram(pCmProgram);
	pCmProgram->Acquire();

	m_KernelRef++;

	return m_KernelRef;
}

UINT CmKernelData::SafeRelease()
{

	--m_RefCount;
	if (m_RefCount == 0) {
		delete this;
		return 0;
	} else {
		return m_RefCount;
	}
}

PCM_HAL_KERNEL_PARAM CmKernelData::GetHalCmKernelData()
{
	return &m_HalKernelParam;
}

INT CmKernelData::ReleaseKernel()
{
	if (m_pCmKernel && m_KernelRef > 0) {
		CmDevice *pDevice = NULL;
		m_pCmKernel->GetCmDevice(pDevice);

		pDevice->DestroyKernel(m_pCmKernel);

		m_KernelRef--;
		m_IsInUse = FALSE;
	}
	return CM_SUCCESS;
}

BOOL CmKernelData::IsInUse()
{
	return m_IsInUse;
}

UINT CmKernelData::GetKernelCurbeSize(void)
{
	return m_HalKernelParam.iKrnCurbeSize;
}
