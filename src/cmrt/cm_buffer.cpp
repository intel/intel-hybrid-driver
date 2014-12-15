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

#include "cm_buffer.h"
#include "cm_surface_manager.h"
#include "cm_event.h"
#include "cm_device.h"
#include "cm_mem.h"
#include "cm_def.h"
#include "hal_cm.h"

INT CmBuffer_RT::Create(UINT index, UINT handle, UINT size, BOOL bIsCmCreated,
			CmSurfaceManager * pSurfaceManager, UINT uiBufferType,
			VOID * pSysMem, CmBuffer_RT * &pSurface)
{
	INT result = CM_SUCCESS;

	pSurface =
	    new(std::nothrow) CmBuffer_RT(handle, size, bIsCmCreated,
					  pSurfaceManager, uiBufferType,
					  pSysMem);
	if (pSurface) {
		result = pSurface->Initialize(index);
		if (result != CM_SUCCESS) {
			CmSurface *pBaseSurface = pSurface;
			CmSurface::Destroy(pBaseSurface);
		}
	} else {
		CM_ASSERT(0);
		result = CM_OUT_OF_HOST_MEMORY;
	}

	return result;
}

 CmBuffer_RT::CmBuffer_RT(UINT handle, UINT size, BOOL bIsCmCreated, CmSurfaceManager * pSurfaceManager, UINT uiBufferType, VOID * pSysMem):
CmSurface(pSurfaceManager, bIsCmCreated),
m_Handle(handle), m_Size(size), m_uiBufferType(uiBufferType), m_pSysMem(pSysMem)
{
	CmSurface::SetMemoryObjectControl(MEMORY_OBJECT_CONTROL_UNKNOW,
					  CM_USE_PTE, 0);
}

CmBuffer_RT::~CmBuffer_RT(void)
{
}

INT CmBuffer_RT::Initialize(UINT index)
{
	return CmSurface::Initialize(index);
}

INT CmBuffer_RT::GetHandle(UINT & handle)
{
	handle = m_Handle;
	return CM_SUCCESS;
}

CM_RT_API INT
    CmBuffer_RT::WriteSurface(const unsigned char *pSysMem, CmEvent * pEvent,
			      UINT64 sysMemSize)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	BYTE *pDst = NULL;
	BYTE *pSurf = NULL;

	if (pSysMem == NULL) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	if (sysMemSize < m_Size) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}
	if (pEvent) {
		FlushDeviceQueue(pEvent);
	}

	WaitForReferenceFree();

	CmDevice *pCmDevice = NULL;
	m_SurfaceMgr->GetCmDevice(pCmDevice);
	PCM_CONTEXT pCmData = (PCM_CONTEXT) pCmDevice->GetAccelData();

	CM_HAL_BUFFER_PARAM inParam;
	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_BUFFER_PARAM));
	inParam.iLockFlag = CM_HAL_LOCKFLAG_WRITEONLY;
	inParam.dwHandle = m_Handle;

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnLockBuffer(pCmData->pCmHalState,
						     &inParam));
	CMCHK_NULL(inParam.pData);

	pDst = (BYTE *) (inParam.pData);
	pSurf = (BYTE *) pSysMem;

	CmFastMemCopyWC(pDst, pSurf, m_Size);

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnUnlockBuffer(pCmData->pCmHalState,
						       &inParam));

 finish:
	return hr;
}

CM_RT_API INT
    CmBuffer_RT::ReadSurface(unsigned char *pSysMem, CmEvent * pEvent,
			     UINT64 sysMemSize)
{
	CM_RETURN_CODE hr = CM_SUCCESS;

	if (pSysMem == NULL) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	if (sysMemSize < m_Size) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}
	if (pEvent) {
		FlushDeviceQueue(pEvent);
	}

	WaitForReferenceFree();

	CmDevice *pCmDevice = NULL;
	m_SurfaceMgr->GetCmDevice(pCmDevice);
	PCM_CONTEXT pCmData = (PCM_CONTEXT) pCmDevice->GetAccelData();

	CM_HAL_BUFFER_PARAM inParam;
	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_BUFFER_PARAM));
	inParam.iLockFlag = CM_HAL_LOCKFLAG_READONLY;
	inParam.dwHandle = m_Handle;

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnLockBuffer(pCmData->pCmHalState,
						     &inParam));
	CMCHK_NULL(inParam.pData);

	CmFastMemCopyFromWC(pSysMem, inParam.pData, m_Size,
			    GetCpuInstructionLevel());

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnUnlockBuffer(pCmData->pCmHalState,
						       &inParam));

 finish:
	return hr;
}

CM_RT_API INT CmBuffer_RT::GetIndex(SurfaceIndex * &pIndex)
{
	pIndex = m_pIndex;
	return CM_SUCCESS;
}

CM_RT_API INT CmBuffer_RT::InitSurface(const DWORD initValue, CmEvent * pEvent)
{
	CM_RETURN_CODE hr = CM_SUCCESS;

	if (pEvent) {
		FlushDeviceQueue(pEvent);
	}

	CmDevice *pCmDevice = NULL;
	m_SurfaceMgr->GetCmDevice(pCmDevice);
	CM_ASSERT(pCmDevice);
	PCM_CONTEXT pCmData = (PCM_CONTEXT) pCmDevice->GetAccelData();

	CM_HAL_BUFFER_PARAM inParam;
	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_BUFFER_PARAM));
	inParam.dwHandle = m_Handle;
	inParam.iLockFlag = CM_HAL_LOCKFLAG_WRITEONLY;

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnLockBuffer(pCmData->pCmHalState,
						     &inParam));
	CMCHK_NULL(inParam.pData);

	CmDwordMemSet(inParam.pData, initValue, m_Size);

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnUnlockBuffer(pCmData->pCmHalState,
						       &inParam));

 finish:
	return hr;
}

CM_RT_API INT
    CmBuffer_RT::SetMemoryObjectControl(MEMORY_OBJECT_CONTROL mem_ctrl,
					MEMORY_TYPE mem_type, UINT age)
{
	CmSurface::SetMemoryObjectControl(mem_ctrl, mem_type, age);

	return CM_SUCCESS;
}

INT CmBuffer_RT::GetSize(UINT & size)
{
	size = m_Size;
	return CM_SUCCESS;
}

INT CmBuffer_RT::SetSize(UINT size)
{
	m_Size = size;
	return CM_SUCCESS;
}

BOOL CmBuffer_RT::IsUpSurface()
{
	return (m_uiBufferType == CM_BUFFER_UP);
}

CM_RT_API INT CmBuffer_RT::GetAddress(VOID * &pAddr)
{
	pAddr = m_pSysMem;
	return CM_SUCCESS;
}
