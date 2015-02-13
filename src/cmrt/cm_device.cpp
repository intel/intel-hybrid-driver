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
 *     Zhao Yakui <yakui.zhao@intel.com>
 */

#include "cm_device.h"
#include "cm_queue.h"
#include "cm_surface_manager.h"
#include "cm_program.h"
#include "cm_kernel.h"
#include "cm_task.h"
#include "cm_buffer.h"
#include "cm_thread_space.h"
#include "cm_debug.h"
#include "cm_def.h"
#include "cm_group_space.h"
#include "cm_surface_2d.h"
#include "hal_cm.h"

CSync CmDevice::GlobalCriticalSection_Surf2DUserDataLock = CSync();

INT CmDevice::Create(CmDriverContext * pDriverContext, CmDevice * &pDevice,
		     UINT DevCreateOption)
{
	INT result = CM_FAILURE;

	if (pDevice != NULL) {
		pDevice->Acquire();
		return CM_SUCCESS;
	}

	pDevice = new(std::nothrow) CmDevice(DevCreateOption);
	if (pDevice) {
		pDevice->Acquire();
		result = pDevice->Initialize(pDriverContext);
		if (result != CM_SUCCESS) {
			CM_ASSERT(0);
			CmDevice::Destroy(pDevice);
			pDevice = NULL;
		}
	} else {
		CM_ASSERT(0);
		result = CM_OUT_OF_HOST_MEMORY;
	}

	return result;
}

INT CmDevice::Acquire(void)
{
	CLock locker(m_CriticalSection_DeviceRefCount);

	m_CmDeviceRefCount++;
	return CM_SUCCESS;
}

INT CmDevice::Release(void)
{
	CLock locker(m_CriticalSection_DeviceRefCount);

	m_CmDeviceRefCount--;

	return m_CmDeviceRefCount;
}

INT CmDevice::Destroy(CmDevice * &pDevice)
{
	INT result = CM_SUCCESS;

	INT refCount = pDevice->Release();

	if (refCount == 0) {
		CmSafeDelete(pDevice);
	}

	return result;
}

 CmDevice::CmDevice(UINT DevCreateOption):
m_pUmdContext(NULL),
m_pAccelData(NULL),
m_AccelSize(0),
m_pSurfaceMgr(NULL),
m_pQueue(NULL),
m_ProgramArray(CM_INIT_PROGRAM_COUNT),
m_ProgramCount(0),
m_KernelArray(CM_INIT_KERNEL_COUNT),
m_KernelCount(0),
m_ThreadSpaceArray(CM_INIT_THREADSPACE_COUNT),
m_ThreadSpaceCount(0),
m_hJITDll(NULL),
m_fJITCompile(NULL),
m_fFreeBlock(NULL),
m_fJITVersion(NULL),
m_DDIVersion(0),
m_Platform(IGFX_UNKNOWN_CORE),
m_CmDeviceRefCount(0),
m_ThreadGroupSpaceArray(CM_INIT_THREADGROUPSPACE_COUNT),
m_ThreadGroupSpaceCount(0), m_TaskArray(CM_INIT_TASK_COUNT), m_TaskCount(0)
{
	InitDevCreateOption(m_DevCreateOption, DevCreateOption);

}

CmDevice::~CmDevice(void)
{
	for (UINT i = 0; i < m_KernelCount; i++) {
		CmKernel *pKernel = (CmKernel *) m_KernelArray.GetElement(i);
		if (pKernel) {
			CmProgram *pProgram = NULL;
			pKernel->GetCmProgram(pProgram);
			UINT indexInProgramArray;
			for (indexInProgramArray = 0;
			     indexInProgramArray < m_ProgramCount;
			     indexInProgramArray++) {
				if (pProgram ==
				    m_ProgramArray.GetElement
				    (indexInProgramArray)) {
					break;
				}
			}
			CmKernel::Destroy(pKernel, pProgram);
			if ((pProgram == NULL)
			    && (indexInProgramArray < m_ProgramCount)) {
				m_ProgramArray.SetElement(indexInProgramArray,
							  NULL);
			}
		}
	}
	m_KernelArray.Delete();

	for (UINT i = 0; i < m_ProgramCount; i++) {
		CmProgram *pProgram =
		    (CmProgram *) m_ProgramArray.GetElement(i);
		while (pProgram) {
			CmProgram::Destroy(pProgram);
		}
	}
	m_ProgramArray.Delete();

	UINT ThreadSpaceArrayUsedSize = m_ThreadSpaceArray.GetMaxSize();
	for (UINT i = 0; i < ThreadSpaceArrayUsedSize; i++) {
		CmThreadSpace *pTS_RT =
		    (CmThreadSpace *) m_ThreadSpaceArray.GetElement(i);
		if (pTS_RT) {
			CmThreadSpace::Destroy(pTS_RT);
		}
	}
	m_ThreadSpaceArray.Delete();

	for (UINT i = 0; i < m_ThreadGroupSpaceCount; i++) {
		CmThreadGroupSpace *pTGS = (CmThreadGroupSpace *)
		    m_ThreadGroupSpaceArray.GetElement(i);
		if (pTGS) {
			CmThreadGroupSpace::Destroy(pTGS);
		}
	}
	m_ThreadGroupSpaceArray.Delete();

	UINT TaskArrayUsedSize = m_TaskArray.GetMaxSize();
	for (UINT i = 0; i < TaskArrayUsedSize; i++) {
		CmTask *pTask = (CmTask *) m_TaskArray.GetElement(i);
		if (pTask) {
			CmTask::Destroy(pTask);
		}
	}
	m_TaskArray.Delete();

	CmSurfaceManager::Destroy(m_pSurfaceMgr);
	DestroyQueue(m_pQueue);

	if (m_hJITDll) {
		FreeLibrary(m_hJITDll);
	}

	DestroyAuxDevice();
};

INT CmDevice::Initialize(CmDriverContext * pDriverContext)
{
	INT result = CreateAuxDevice(pDriverContext);

	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}

	m_pSurfaceMgr = NULL;
	result = CmSurfaceManager::Create(this,
					  m_HalMaxValues,
					  m_HalMaxValuesEx, m_pSurfaceMgr);

	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}

	result = CreateQueue_Internel();
	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}

	return result;
}

INT CmDevice::CreateAuxDevice(CmDriverContext * pDriverContext)
{
	INT hr = CM_SUCCESS;
	PCM_HAL_STATE pCmHalState;
	PCM_CONTEXT pCmCtx;
	PGENOS_CONTEXT pOsContext;

	pOsContext =
	    (PGENOS_CONTEXT) GENOS_AllocAndZeroMemory(sizeof(GENOS_CONTEXT));
	CMCHK_NULL(pOsContext);

	if (pDriverContext) {
		pOsContext->wDeviceID = pDriverContext->deviceid;
		pOsContext->wRevision = pDriverContext->device_rev;
		pOsContext->bufmgr = pDriverContext->bufmgr;
	}

	m_pUmdContext = pOsContext;

	CHK_GENOSSTATUS_RETURN_CMERROR(HalCm_Create
				       (pOsContext, &m_DevCreateOption,
					&pCmHalState));

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmHalState->pfnCmAllocate(pCmHalState));

	pCmCtx = (PCM_CONTEXT) GENOS_AllocAndZeroMemory(sizeof(CM_CONTEXT));
	CMCHK_NULL(pCmCtx);
	pCmCtx->GenHwDrvCtx = *pOsContext;
	pCmCtx->pCmHalState = pCmHalState;

	m_pAccelData = (PVOID) pCmCtx;

	CMCHK_HR_MESSAGE(GetMaxValueFromCaps(m_HalMaxValues, m_HalMaxValuesEx),
			 "Failed to get Max values.");
	CMCHK_HR_MESSAGE(GetGenPlatform(m_Platform), "Failed to get GPU type.");

	m_DDIVersion = VA_CM_VERSION;

 finish:
	return hr;
}

INT CmDevice::DestroyAuxDevice()
{
	PCM_CONTEXT pCmData = (PCM_CONTEXT) m_pAccelData;

	if (pCmData && pCmData->pCmHalState) {
		HalCm_Destroy(pCmData->pCmHalState);
		GENOS_FreeMemory(pCmData);
	}

	if (m_pUmdContext) {
		GENOS_FreeMemAndSetNull(m_pUmdContext);
	}

	return CM_SUCCESS;
}

CM_RT_API INT CmDevice::CreateBuffer(UINT size, CmBuffer * &pSurface)
{
	if ((size < CM_MIN_SURF_WIDTH) || (size > CM_MAX_1D_SURF_WIDTH)) {
		CM_ASSERT(0);
		return CM_INVALID_WIDTH;
	}

	CLock locker(m_CriticalSection_Surface);

	CmBuffer_RT *p = NULL;
	VOID *pSysMem = NULL;
	int result = m_pSurfaceMgr->CreateBuffer(size, CM_BUFFER_N, p, NULL,
						 pSysMem);
	pSurface = static_cast < CmBuffer * >(p);

	return result;
}

CM_RT_API INT
    CmDevice::CreateBuffer(CmOsResource * pCmOsResource, CmBuffer * &pSurface)
{
	INT result = CM_SUCCESS;
	if (pCmOsResource == NULL) {
		return CM_INVALID_GENOS_RESOURCE_HANDLE;
	}

	CLock locker(m_CriticalSection_Surface);
	CmBuffer_RT *pBufferRT = NULL;
	VOID *pSysMem = NULL;
	result =
	    m_pSurfaceMgr->CreateBuffer(pCmOsResource->orig_width, CM_BUFFER_N,
					pBufferRT, pCmOsResource, pSysMem);

	pSurface = static_cast < CmBuffer * >(pBufferRT);

	return result;
}

CM_RT_API INT
    CmDevice::CreateBufferUP(UINT size, void *pSysMem, CmBufferUP * &pSurface)
{
	if ((size < CM_MIN_SURF_WIDTH) || (size > CM_MAX_1D_SURF_WIDTH)) {
		CM_ASSERT(0);
		return CM_INVALID_WIDTH;
	}

	CLock locker(m_CriticalSection_Surface);

	CmBuffer_RT *p = NULL;
	int result = m_pSurfaceMgr->CreateBuffer(size, CM_BUFFER_UP, p, NULL,
						 pSysMem);
	pSurface = static_cast < CmBufferUP * >(p);

	return result;
}

CM_RT_API INT CmDevice::DestroyBufferUP(CmBufferUP * &pSurface)
{
	CmBuffer_RT *temp = NULL;
	if (pSurface && (pSurface->Type() == CM_ENUM_CLASS_TYPE_CMBUFFER_RT)) {
		temp = static_cast < CmBuffer_RT * >(pSurface);
	} else {
		return CM_FAILURE;
	}

	CLock locker(m_CriticalSection_Surface);

	INT status = m_pSurfaceMgr->DestroySurface(temp, APP_DESTROY);

	if (status != CM_FAILURE) {
		pSurface = NULL;
		return CM_SUCCESS;
	} else {
		return CM_FAILURE;
	}
	return status;
}

CM_RT_API INT CmDevice::ForceDestroyBufferUP(CmBufferUP * &pSurface)
{
	CmBuffer_RT *temp = NULL;
	if (pSurface && (pSurface->Type() == CM_ENUM_CLASS_TYPE_CMBUFFER_RT)) {
		temp = static_cast < CmBuffer_RT * >(pSurface);
	} else {
		return CM_FAILURE;
	}

	CLock locker(m_CriticalSection_Surface);

	INT status = m_pSurfaceMgr->DestroySurface(temp, FORCE_DESTROY);

	if (status == CM_SUCCESS) {
		pSurface = NULL;
	}
	return status;
}

INT CmDevice::DestroyBufferUP(CmBufferUP * &pSurface, INT iIndexInPool,
			      INT iSurfaceID, SURFACE_DESTROY_KIND kind)
{
	CmBuffer_RT *temp = NULL;

	CLock locker(m_CriticalSection_Surface);
	INT currentID = m_pSurfaceMgr->GetSurfaceIdInPool(iIndexInPool);
	if (currentID > iSurfaceID) {
		return CM_SUCCESS;
	}

	if (pSurface && (pSurface->Type() == CM_ENUM_CLASS_TYPE_CMBUFFER_RT)) {
		temp = static_cast < CmBuffer_RT * >(pSurface);
	} else {
		return CM_FAILURE;
	}

	INT status = m_pSurfaceMgr->DestroySurface(temp, kind);

	if (status == CM_SUCCESS) {
		pSurface = NULL;
	}

	return status;
}

CM_RT_API INT
    CmDevice::CreateSurface2DUP(UINT width, UINT height,
				CM_SURFACE_FORMAT format, void *pSysMem,
				CmSurface2DUP * &pSurface)
{
	INT result = m_pSurfaceMgr->Surface2DSanityCheck(width, height, format);
	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}

	CLock locker(m_CriticalSection_Surface);
	return m_pSurfaceMgr->CreateSurface2DUP(width, height, format, pSysMem,
						pSurface);
}

CM_RT_API INT
    CmDevice::CreateSurface2D(UINT width, UINT height, CM_SURFACE_FORMAT format,
			      CmSurface2D * &pSurface)
{
	CLock locker(m_CriticalSection_Surface);

	return m_pSurfaceMgr->CreateSurface2D(width, height, 0, TRUE, format,
					      pSurface);
}

CM_RT_API INT
    CmDevice::CreateSurface2D(CmOsResource * pCmOsResource,
			      CmSurface2D * &pSurface)
{
	if (pCmOsResource == NULL) {
		return CM_INVALID_GENOS_RESOURCE_HANDLE;
	}

	CLock locker(m_CriticalSection_Surface);

	return m_pSurfaceMgr->CreateSurface2D(pCmOsResource, FALSE, pSurface);
}

INT CmDevice::DestroySurface(CmBuffer * &pSurface, INT iIndexInPool,
			     INT iSurfaceID, SURFACE_DESTROY_KIND kind)
{
	CLock locker(m_CriticalSection_Surface);
	INT currentID = m_pSurfaceMgr->GetSurfaceIdInPool(iIndexInPool);
	if (currentID > iSurfaceID) {
		return CM_SUCCESS;
	}

	CmBuffer_RT *temp = NULL;
	if (pSurface && (pSurface->Type() == CM_ENUM_CLASS_TYPE_CMBUFFER_RT)) {
		temp = static_cast < CmBuffer_RT * >(pSurface);
	}

	if (temp == NULL) {
		return CM_FAILURE;
	}

	INT status = m_pSurfaceMgr->DestroySurface(temp, kind);

	if (status == CM_SUCCESS) {
		pSurface = NULL;
	}

	return status;
}

CM_RT_API INT CmDevice::DestroySurface(CmBuffer * &pSurface)
{
	CmBuffer_RT *temp = NULL;
	if (pSurface && (pSurface->Type() == CM_ENUM_CLASS_TYPE_CMBUFFER_RT)) {
		temp = static_cast < CmBuffer_RT * >(pSurface);
	} else {
		return CM_FAILURE;
	}

	CLock locker(m_CriticalSection_Surface);

	INT status = m_pSurfaceMgr->DestroySurface(temp, APP_DESTROY);

	if (status != CM_FAILURE) {
		pSurface = NULL;
		return CM_SUCCESS;
	} else {
		return CM_FAILURE;
	}
}

CM_RT_API INT CmDevice::DestroySurface(CmSurface2DUP * &pSurface)
{
	CLock locker(m_CriticalSection_Surface);

	INT status = m_pSurfaceMgr->DestroySurface(pSurface, APP_DESTROY);

	if (status != CM_FAILURE) {
		pSurface = NULL;
		return CM_SUCCESS;
	} else {
		return CM_FAILURE;
	}
}

INT CmDevice::DestroySurface(CmSurface2DUP * &pSurface, INT iIndexInPool,
			     INT iSurfaceID, SURFACE_DESTROY_KIND kind)
{
	CLock locker(m_CriticalSection_Surface);

	INT currentID = m_pSurfaceMgr->GetSurfaceIdInPool(iIndexInPool);
	if (currentID > iSurfaceID) {
		return CM_SUCCESS;
	}

	INT status = m_pSurfaceMgr->DestroySurface(pSurface, kind);

	if (status == CM_SUCCESS) {
		pSurface = NULL;
	}
	return status;
}

INT CmDevice::DestroySurface(CmSurface2D * &pSurface, INT iIndexInPool,
			     INT iSurfaceID, SURFACE_DESTROY_KIND kind)
{
	CLock locker(m_CriticalSection_Surface);

	INT currentID = m_pSurfaceMgr->GetSurfaceIdInPool(iIndexInPool);
	if (currentID > iSurfaceID) {
		return CM_SUCCESS;
	}

	INT status = m_pSurfaceMgr->DestroySurface(pSurface, kind);

	if (status == CM_SUCCESS) {
		pSurface = NULL;
	}

	return status;
}

CM_RT_API INT CmDevice::DestroySurface(CmSurface2D * &pSurface)
{
	CLock locker(m_CriticalSection_Surface);

	INT status = m_pSurfaceMgr->DestroySurface(pSurface, APP_DESTROY);

	if (status != CM_FAILURE) {
		pSurface = NULL;
		return CM_SUCCESS;
	} else {
		return CM_FAILURE;
	}

	return status;
}

INT CmDevice::GetJITCompileFnt(pJITCompile & fJITCompile)
{
	if (m_fJITCompile) {
		fJITCompile = m_fJITCompile;
	} else {
		if (!m_hJITDll) {
			if (sizeof(void *) == 4) {
				m_hJITDll = dlopen("igfxcmjit32.so", RTLD_LAZY);
			} else {
				m_hJITDll = dlopen("igfxcmjit64.so", RTLD_LAZY);
			}

			if (NULL == m_hJITDll) {
				CM_ASSERT(0);
				return CM_JITDLL_LOAD_FAILURE;
			}
		}

		m_fJITCompile =
		    (pJITCompile) GetProcAddress(m_hJITDll,
						 JITCOMPILE_FUNCTION_STR);
		if (NULL == m_fJITCompile) {
			CM_ASSERT(0);
			return CM_JITDLL_LOAD_FAILURE;
		}
		fJITCompile = m_fJITCompile;
	}
	return CM_SUCCESS;
}

INT CmDevice::GetFreeBlockFnt(pFreeBlock & fFreeBlock)
{
	if (m_fFreeBlock) {
		fFreeBlock = m_fFreeBlock;
	} else {
		if (!m_hJITDll) {
			if (sizeof(void *) == 4) {
				m_hJITDll = dlopen("igfxcmjit32.so", RTLD_LAZY);
			} else {
				m_hJITDll = dlopen("igfxcmjit64.so", RTLD_LAZY);
			}

			if (NULL == m_hJITDll) {
				CM_ASSERT(0);
				return CM_JITDLL_LOAD_FAILURE;
			}
		}

		m_fFreeBlock =
		    (pFreeBlock) GetProcAddress(m_hJITDll,
						FREEBLOCK_FUNCTION_STR);
		if (NULL == m_fFreeBlock) {
			CM_ASSERT(0);
			return CM_JITDLL_LOAD_FAILURE;
		}
		fFreeBlock = m_fFreeBlock;
	}
	return CM_SUCCESS;
}

INT CmDevice::GetJITVersionFnt(pJITVersion & fJITVersion)
{
	if (m_fJITVersion) {
		fJITVersion = m_fJITVersion;
	} else {
		if (!m_hJITDll) {
			if (sizeof(void *) == 4) {
				m_hJITDll = dlopen("igfxcmjit32.so", RTLD_LAZY);
			} else {
				m_hJITDll = dlopen("igfxcmjit64.so", RTLD_LAZY);
			}

			if (NULL == m_hJITDll) {
				CM_ASSERT(0);
				return CM_JITDLL_LOAD_FAILURE;
			}
		}

		m_fJITVersion =
		    (pJITVersion) GetProcAddress(m_hJITDll,
						 JITVERSION_FUNCTION_STR);
		if (NULL == m_fJITVersion) {
			CM_ASSERT(0);
			return CM_JITDLL_LOAD_FAILURE;
		}
		fJITVersion = m_fJITVersion;
	}
	return CM_SUCCESS;
}

INT CmDevice::LoadJITDll(void)
{
	int result = 0;

	if (NULL == m_hJITDll) {
		if (sizeof(void *) == 4) {
			m_hJITDll = dlopen("igfxcmjit32.so", RTLD_LAZY);
		} else {
			m_hJITDll = dlopen("igfxcmjit64.so", RTLD_LAZY);
		}

		if (NULL == m_hJITDll) {
			result = CM_JITDLL_LOAD_FAILURE;
			CM_ASSERT(0);
			return result;
		}
		if (NULL == m_fJITCompile) {
			m_fJITCompile =
			    (pJITCompile) GetProcAddress(m_hJITDll,
							 JITCOMPILE_FUNCTION_STR);
			m_fFreeBlock =
			    (pFreeBlock) GetProcAddress(m_hJITDll,
							FREEBLOCK_FUNCTION_STR);
			m_fJITVersion =
			    (pJITVersion) GetProcAddress(m_hJITDll,
							 JITVERSION_FUNCTION_STR);
		}

		if ((NULL == m_fJITCompile) || (NULL == m_fFreeBlock)
		    || (NULL == m_fJITVersion)) {
			result = CM_JITDLL_LOAD_FAILURE;
			CM_ASSERT(0);
			return result;
		}
	}

	return result;
}

CM_RT_API INT CmDevice::GetGenPlatform(UINT & platform)
{
	if (m_Platform != IGFX_UNKNOWN_CORE) {
		platform = m_Platform;
		return CM_SUCCESS;
	}

	platform = IGFX_UNKNOWN_CORE;

	INT hr = 0;
	DXVA_CM_QUERY_CAPS queryCaps;
	UINT querySize = sizeof(DXVA_CM_QUERY_CAPS);

	CmSafeMemSet(&queryCaps, 0, sizeof(queryCaps));
	queryCaps.Type = DXVA_CM_QUERY_GPU;

	hr = GetCapsInternal(&queryCaps, &querySize);
	if (FAILED(hr)) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}
	if (queryCaps.iVersion) {
		platform = queryCaps.iVersion;
	}

	return CM_SUCCESS;
}

CM_RT_API INT
    CmDevice::GetSurface2DInfo(UINT width, UINT height,
			       CM_SURFACE_FORMAT format, UINT & pitch,
			       UINT & physicalSize)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	CM_HAL_SURFACE2D_UP_PARAM inParam;
	PCM_CONTEXT pCmData;
	PCM_HAL_STATE pCmHalState;

	CMCHK_HR(m_pSurfaceMgr->Surface2DSanityCheck(width, height, format));

	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_SURFACE2D_UP_PARAM));
	inParam.iWidth = width;
	inParam.iHeight = height;
	inParam.format = m_pSurfaceMgr->CmFmtToGenHwFmt(format);

	pCmData = (PCM_CONTEXT) GetAccelData();
	pCmHalState = pCmData->pCmHalState;
	CHK_GENOSSTATUS_RETURN_CMERROR(pCmHalState->pfnGetSurface2DPitchAndSize
				       (pCmHalState, &inParam));

	pitch = inParam.iPitch;
	physicalSize = inParam.iPhysicalSize;

 finish:
	return hr;
}

INT CmDevice::CreateQueue_Internel(void)
{
	if (m_pQueue) {
		CM_ASSERTMESSAGE("Failed to create more than one queue.");
		return CM_FAILURE;
	}

	INT result = CmQueue::Create(this, m_pQueue);
	if (result != CM_SUCCESS) {
		CM_ASSERTMESSAGE("Failed to create queue.");
		return CM_FAILURE;
	}

	return result;
}

INT CmDevice::GetSurfaceManager(CmSurfaceManager * &pSurfaceMgr)
{
	pSurfaceMgr = m_pSurfaceMgr;
	return CM_SUCCESS;
}

CSync *CmDevice::GetSurfaceLock()
{
	return &m_CriticalSection_ReadWriteSurface2D;
}

CSync *CmDevice::GetSurfaceCreationLock()
{
	return &m_CriticalSection_Surface;
}

CSync *CmDevice::GetProgramKernelLock()
{
	return &m_CriticalSection_Program_Kernel;
}

INT CmDevice::GetQueue(CmQueue * &pQueue)
{
	pQueue = m_pQueue;
	return CM_SUCCESS;
}

INT CmDevice::GetHalMaxValues(CM_HAL_MAX_VALUES * &pHalMaxValues,
			      CM_HAL_MAX_VALUES_EX * &pHalMaxValuesEx)
{
	pHalMaxValues = &m_HalMaxValues;
	pHalMaxValuesEx = &m_HalMaxValuesEx;

	return CM_SUCCESS;
}

INT CmDevice::GetMaxValueFromCaps(CM_HAL_MAX_VALUES & MaxValues,
				  CM_HAL_MAX_VALUES_EX & MaxValuesEx)
{
	DXVA_CM_QUERY_CAPS queryCaps;
	UINT querySize = sizeof(DXVA_CM_QUERY_CAPS);
	CmSafeMemSet(&queryCaps, 0, sizeof(DXVA_CM_QUERY_CAPS));
	queryCaps.Type = DXVA_CM_MAX_VALUES;

	INT hr = GetCapsInternal(&queryCaps, &querySize);
	if (FAILED(hr)) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	MaxValues = queryCaps.MaxValues;
	MaxValues.iMaxArgsPerKernel =
	    (queryCaps.MaxValues.iMaxArgsPerKernel >
	     CM_MAX_ARGS_PER_KERNEL) ? (CM_MAX_ARGS_PER_KERNEL) :
	    queryCaps.MaxValues.iMaxArgsPerKernel;

	CmSafeMemSet(&queryCaps, 0, sizeof(DXVA_CM_QUERY_CAPS));
	queryCaps.Type = DXVA_CM_MAX_VALUES_EX;

	hr = GetCapsInternal(&queryCaps, &querySize);
	if (FAILED(hr)) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}
	MaxValuesEx = queryCaps.MaxValuesEx;

	return CM_SUCCESS;
}

INT CmDevice::GetCapsInternal(PVOID pCaps, PUINT puSize)
{
	PDXVA_CM_QUERY_CAPS pQueryCaps;
	PCM_CONTEXT pCmData;
	PCM_HAL_STATE pCmHalState;

	CM_RETURN_CODE hr = CM_SUCCESS;

	if ((!puSize) || (!pCaps) || (*puSize < sizeof(DXVA_CM_QUERY_CAPS))) {
		CM_ASSERTMESSAGE("Invalid Arguments.");
		hr = CM_FAILURE;
		goto finish;
	}

	pQueryCaps = (PDXVA_CM_QUERY_CAPS) pCaps;
	*puSize = sizeof(DXVA_CM_QUERY_CAPS);

	if (pQueryCaps->Type == DXVA_CM_QUERY_VERSION) {
		pQueryCaps->iVersion = DXVA_CM_VERSION;
		hr = CM_SUCCESS;
		goto finish;
	}

	pCmData = (PCM_CONTEXT) GetAccelData();
	CMCHK_NULL(pCmData);

	pCmHalState = pCmData->pCmHalState;
	CMCHK_NULL(pCmHalState);

	switch (pQueryCaps->Type) {
	case DXVA_CM_QUERY_REG_HANDLE:
		pQueryCaps->hRegistration =
		    (HANDLE *) & pCmHalState->SurfaceRegTable;
		break;

	case DXVA_CM_MAX_VALUES:
		CHK_GENOSSTATUS_RETURN_CMERROR(pCmHalState->pfnGetMaxValues
					       (pCmHalState,
						&pQueryCaps->MaxValues));
		break;

	case DXVA_CM_MAX_VALUES_EX:
		CHK_GENOSSTATUS_RETURN_CMERROR(pCmHalState->pfnGetMaxValuesEx
					       (pCmHalState,
						&pQueryCaps->MaxValuesEx));
		break;

	case DXVA_CM_QUERY_GPU:
		pQueryCaps->genCore =
		    pCmHalState->pHwInterface->Platform.eRenderCoreFamily;
		break;

	case DXVA_CM_QUERY_GT:
		if (GFX_IS_PRODUCT
		    (pCmHalState->pHwInterface->Platform, IGFX_CHERRYVIEW)) {
			pQueryCaps->genGT = PLATFORM_INTEL_GTCHV;
		} else if (pCmHalState->pHwInterface->Platform.GtType ==
			   GTTYPE_GT1) {
			pQueryCaps->genGT = PLATFORM_INTEL_GT1;
		} else if (pCmHalState->pHwInterface->Platform.GtType ==
			   GTTYPE_GT2) {
			pQueryCaps->genGT = PLATFORM_INTEL_GT2;
		} else if (pCmHalState->pHwInterface->Platform.GtType ==
			   GTTYPE_GT3) {
			pQueryCaps->genGT = PLATFORM_INTEL_GT3;
		}
		break;

	case DXVA_CM_QUERY_STEP:
		pQueryCaps->genStepId = pCmHalState->Platform.usRevId;
		break;

	case DXVA_CM_QUERY_GPU_FREQ:
		CHK_GENOSSTATUS_RETURN_CMERROR
		    (pCmHalState->pfnGetGPUCurrentFrequency
		     (pCmHalState, &pQueryCaps->GPUCurrentFreq));
		break;

	case DXVA_CM_QUERY_SURFACE2D_FORMAT_COUNT:
		pQueryCaps->Surface2DCount =
		    CM_MAX_SURFACE2D_FORMAT_COUNT_INTERNAL;
		break;

	case DXVA_CM_QUERY_SURFACE2D_FORMATS:
		if (pQueryCaps->pSurface2DFormats) {
			CM_SURFACE_FORMAT
			    formats[CM_MAX_SURFACE2D_FORMAT_COUNT_INTERNAL] = {
			CM_SURFACE_FORMAT_X8R8G8B8,
				    CM_SURFACE_FORMAT_A8R8G8B8,
				    CM_SURFACE_FORMAT_R32F,
				    CM_SURFACE_FORMAT_V8U8,
				    CM_SURFACE_FORMAT_P8,
				    CM_SURFACE_FORMAT_YUY2,
				    CM_SURFACE_FORMAT_A8,
				    CM_SURFACE_FORMAT_NV12,
				    CM_SURFACE_FORMAT_UYVY,
				    CM_SURFACE_FORMAT_R8_UINT,
				    CM_SURFACE_FORMAT_R16_UINT,
				    CM_SURFACE_FORMAT_411P,
				    CM_SURFACE_FORMAT_422H,
				    CM_SURFACE_FORMAT_444P,
				    CM_SURFACE_FORMAT_IMC3,
				    CM_SURFACE_FORMAT_422V,
				    CM_SURFACE_FORMAT_YV12,};
			CmSafeMemCopy(pQueryCaps->pSurface2DFormats, formats,
				      CM_MAX_SURFACE2D_FORMAT_COUNT_INTERNAL *
				      sizeof(CM_SURFACE_FORMAT));
			break;
		} else {
			hr = CM_FAILURE;
			goto finish;
		}

	default:
		hr = CM_FAILURE;
		goto finish;
	}

 finish:
	return hr;
}

CM_RT_API INT
    CmDevice::GetCaps(CM_DEVICE_CAP_NAME capName, size_t & capValueSize,
		      void *pCapValue)
{
	PCM_CONTEXT pCmData;
	PCM_HAL_STATE pCmHalState;

	if (pCapValue == NULL) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	pCmData = (PCM_CONTEXT) GetAccelData();
	if (pCmData == NULL) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	pCmHalState = pCmData->pCmHalState;
	if (pCmHalState == NULL) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	switch (capName) {
	case CAP_KERNEL_COUNT_PER_TASK:
		if (capValueSize >= sizeof(m_HalMaxValues.iMaxKernelsPerTask)) {
			capValueSize =
			    sizeof(m_HalMaxValues.iMaxKernelsPerTask);
			CmSafeMemCopy(pCapValue,
				      &m_HalMaxValues.iMaxKernelsPerTask,
				      capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_KERNEL_BINARY_SIZE:
		if (capValueSize >= sizeof(m_HalMaxValues.iMaxKernelBinarySize)) {
			capValueSize =
			    sizeof(m_HalMaxValues.iMaxKernelBinarySize);
			CmSafeMemCopy(pCapValue,
				      &m_HalMaxValues.iMaxKernelBinarySize,
				      capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_BUFFER_COUNT:
		if (capValueSize >= sizeof(m_HalMaxValues.iMaxBufferTableSize)) {
			capValueSize =
			    sizeof(m_HalMaxValues.iMaxBufferTableSize);
			CmSafeMemCopy(pCapValue,
				      &m_HalMaxValues.iMaxBufferTableSize,
				      capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_SURFACE2D_COUNT:
		if (capValueSize >=
		    sizeof(m_HalMaxValues.iMax2DSurfaceTableSize)) {
			capValueSize =
			    sizeof(m_HalMaxValues.iMax2DSurfaceTableSize);
			CmSafeMemCopy(pCapValue,
				      &m_HalMaxValues.iMax2DSurfaceTableSize,
				      capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_SURFACE_COUNT_PER_KERNEL:
		if (capValueSize >=
		    sizeof(m_HalMaxValues.iMaxSurfacesPerKernel)) {
			capValueSize =
			    sizeof(m_HalMaxValues.iMaxSurfacesPerKernel);
			CmSafeMemCopy(pCapValue,
				      &m_HalMaxValues.iMaxSurfacesPerKernel,
				      capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_ARG_COUNT_PER_KERNEL:
		if (capValueSize >= sizeof(m_HalMaxValues.iMaxArgsPerKernel)) {
			capValueSize = sizeof(m_HalMaxValues.iMaxArgsPerKernel);
			CmSafeMemCopy(pCapValue,
				      &m_HalMaxValues.iMaxArgsPerKernel,
				      capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_ARG_SIZE_PER_KERNEL:
		if (capValueSize >=
		    sizeof(m_HalMaxValues.iMaxArgByteSizePerKernel)) {
			capValueSize =
			    sizeof(m_HalMaxValues.iMaxArgByteSizePerKernel);
			CmSafeMemCopy(pCapValue,
				      &m_HalMaxValues.iMaxArgByteSizePerKernel,
				      capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_USER_DEFINED_THREAD_COUNT_PER_TASK:
		if (capValueSize >=
		    sizeof(m_HalMaxValues.iMaxUserThreadsPerTask)) {
			capValueSize =
			    sizeof(m_HalMaxValues.iMaxUserThreadsPerTask);
			CmSafeMemCopy(pCapValue,
				      &m_HalMaxValues.iMaxUserThreadsPerTask,
				      capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_USER_DEFINED_THREAD_COUNT_PER_MEDIA_WALKER:
		if (capValueSize >=
		    sizeof(m_HalMaxValuesEx.iMaxUserThreadsPerMediaWalker)) {
			capValueSize =
			    sizeof
			    (m_HalMaxValuesEx.iMaxUserThreadsPerMediaWalker);
			CmSafeMemCopy(pCapValue,
				      &m_HalMaxValuesEx.
				      iMaxUserThreadsPerMediaWalker,
				      capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_USER_DEFINED_THREAD_COUNT_PER_THREAD_GROUP:
		if (capValueSize >=
		    sizeof(m_HalMaxValuesEx.iMaxUserThreadsPerThreadGroup)) {
			capValueSize =
			    sizeof
			    (m_HalMaxValuesEx.iMaxUserThreadsPerThreadGroup);
			CmSafeMemCopy(pCapValue,
				      &m_HalMaxValuesEx.
				      iMaxUserThreadsPerThreadGroup,
				      capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_USER_DEFINED_THREAD_COUNT_PER_TASK_NO_THREAD_ARG:
		if (capValueSize >=
		    sizeof(m_HalMaxValues.iMaxUserThreadsPerTaskNoThreadArg)) {
			capValueSize =
			    sizeof
			    (m_HalMaxValues.iMaxUserThreadsPerTaskNoThreadArg);
			CmSafeMemCopy(pCapValue,
				      &m_HalMaxValues.
				      iMaxUserThreadsPerTaskNoThreadArg,
				      capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_HW_THREAD_COUNT:
		if (capValueSize >= sizeof(m_HalMaxValues.iMaxHwThreads)) {
			capValueSize = sizeof(m_HalMaxValues.iMaxHwThreads);
			CmSafeMemCopy(pCapValue, &m_HalMaxValues.iMaxHwThreads,
				      capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_SURFACE2D_FORMAT_COUNT:
		if (capValueSize >= sizeof(UINT)) {
			capValueSize = sizeof(UINT);
			UINT formatCount = CM_MAX_SURFACE2D_FORMAT_COUNT;
			CmSafeMemCopy(pCapValue, &formatCount, capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_SURFACE2D_FORMATS:
		if (capValueSize >=
		    CM_MAX_SURFACE2D_FORMAT_COUNT * sizeof(CM_SURFACE_FORMAT)) {
			capValueSize =
			    CM_MAX_SURFACE2D_FORMAT_COUNT *
			    sizeof(CM_SURFACE_FORMAT);
			CM_SURFACE_FORMAT formats[CM_MAX_SURFACE2D_FORMAT_COUNT]
			    = {
				CM_SURFACE_FORMAT_X8R8G8B8,
				CM_SURFACE_FORMAT_A8R8G8B8,
				CM_SURFACE_FORMAT_R32F,
				CM_SURFACE_FORMAT_V8U8,
				CM_SURFACE_FORMAT_P8,
				CM_SURFACE_FORMAT_YUY2,
				CM_SURFACE_FORMAT_A8,
				CM_SURFACE_FORMAT_NV12,
				CM_SURFACE_FORMAT_UYVY,
				CM_SURFACE_FORMAT_R8_UINT,
				CM_SURFACE_FORMAT_R16_UINT,
				CM_SURFACE_FORMAT_411P,
				CM_SURFACE_FORMAT_422H,
				CM_SURFACE_FORMAT_444P,
				CM_SURFACE_FORMAT_IMC3,
				CM_SURFACE_FORMAT_422V,
				CM_SURFACE_FORMAT_YV12,
			};
			CmSafeMemCopy(pCapValue, formats, capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_GPU_PLATFORM:
		if (capValueSize >= sizeof(UINT)) {
			UINT platform = PLATFORM_INTEL_UNKNOWN;
			capValueSize = sizeof(UINT);
			switch (m_Platform) {
			case IGFX_GEN7_5_CORE:
				platform = PLATFORM_INTEL_HSW;
				break;

			case IGFX_GEN8_CORE:
				if (GFX_IS_PRODUCT
				    (pCmHalState->pHwInterface->Platform,
				     IGFX_CHERRYVIEW)) {
					platform = PLATFORM_INTEL_CHV;
				} else {
					platform = PLATFORM_INTEL_BDW;
				}
				break;
			}
			CmSafeMemCopy(pCapValue, &platform, capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_GT_PLATFORM:
		if (capValueSize >= sizeof(UINT)) {
			DXVA_CM_QUERY_CAPS queryCaps;
			queryCaps.Type = DXVA_CM_QUERY_GT;
			UINT queryCapsSize = sizeof(DXVA_CM_QUERY_CAPS);
			GetCapsInternal(&queryCaps, &queryCapsSize);
			capValueSize = sizeof(UINT);
			UINT gtPlatform = queryCaps.genGT;
			CmSafeMemCopy(pCapValue, &gtPlatform, capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}
	case CAP_MIN_FREQUENCY:
		if (capValueSize >= sizeof(UINT)) {
			DXVA_CM_QUERY_CAPS queryCaps;
			queryCaps.Type = DXVA_CM_MIN_RENDER_FREQ;
			UINT queryCapsSize = sizeof(DXVA_CM_QUERY_CAPS);
			GetCapsInternal(&queryCaps, &queryCapsSize);
			UINT frequency = queryCaps.MinRenderFreq;
			capValueSize = sizeof(UINT);
			CmSafeMemCopy(pCapValue, &frequency, capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_MAX_FREQUENCY:
		if (capValueSize >= sizeof(UINT)) {
			DXVA_CM_QUERY_CAPS queryCaps;
			queryCaps.Type = DXVA_CM_MAX_RENDER_FREQ;
			UINT queryCapsSize = sizeof(DXVA_CM_QUERY_CAPS);
			GetCapsInternal(&queryCaps, &queryCapsSize);
			UINT frequency = queryCaps.MaxRenderFreq;
			capValueSize = sizeof(UINT);
			CmSafeMemCopy(pCapValue, &frequency, capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	case CAP_GPU_CURRENT_FREQUENCY:
		if (capValueSize >= sizeof(UINT)) {
			DXVA_CM_QUERY_CAPS queryCaps;
			queryCaps.Type = DXVA_CM_QUERY_GPU_FREQ;
			UINT queryCapsSize = sizeof(DXVA_CM_QUERY_CAPS);
			GetCapsInternal(&queryCaps, &queryCapsSize);
			UINT frequency = queryCaps.GPUCurrentFreq;
			capValueSize = sizeof(UINT);
			CmSafeMemCopy(pCapValue, &frequency, capValueSize);
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}

	default:
		return CM_FAILURE;
	}
}

CM_RT_API INT
    CmDevice::LoadProgram(void *pCommonISACode, const UINT size,
			  CmProgram * &pProgram, const char *options)
{
	INT result;

	if ((pCommonISACode == NULL) || (size == 0)) {
		CM_ASSERT(0);
		return CM_INVALID_COMMON_ISA;
	}

	CLock locker(m_CriticalSection_Program_Kernel);

	UINT firstfreeslot = m_ProgramArray.GetFirstFreeIndex();

	result =
	    CmProgram::Create(this, pCommonISACode, size, NULL, 0, pProgram,
			      options, firstfreeslot);
	if (result == CM_SUCCESS) {
		m_ProgramArray.SetElement(firstfreeslot, pProgram);
		m_ProgramCount++;
	}
	return result;
}

CM_RT_API INT CmDevice::DestroyProgram(CmProgram * &pProgram)
{
	if (pProgram == NULL) {
		return CM_FAILURE;
	}

	CLock locker(m_CriticalSection_Program_Kernel);

	UINT indexInProgramArrary = pProgram->GetProgramIndex();
	if (pProgram == m_ProgramArray.GetElement(indexInProgramArrary)) {
		CmProgram::Destroy(pProgram);
		if (pProgram == NULL) {
			m_ProgramArray.SetElement(indexInProgramArrary, NULL);
		}
		return CM_SUCCESS;
	} else {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

}

CM_RT_API INT
    CmDevice::CreateKernel(CmProgram * pProgram, const char *kernelName,
			   CmKernel * &pKernel, const char *options)
{
	if (pProgram == NULL) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	CLock locker(m_CriticalSection_Program_Kernel);

	UINT freeSlotInKernelArray = m_KernelArray.GetFirstFreeIndex();
	INT result =
	    CmKernel::Create(this, pProgram, kernelName, freeSlotInKernelArray,
			     m_KernelCount, pKernel, options);
	if (result == CM_SUCCESS) {
		m_KernelArray.SetElement(freeSlotInKernelArray, pKernel);
		m_KernelCount++;
	}

	return result;
}

CM_RT_API INT CmDevice::DestroyKernel(CmKernel * &pKernel)
{
	if (pKernel == NULL) {
		return CM_FAILURE;
	}

	CLock locker(m_CriticalSection_Program_Kernel);

	UINT indexInKernelArrary = pKernel->GetKernelIndex();
	if (pKernel == m_KernelArray.GetElement(indexInKernelArrary)) {
		CmProgram *pProgram = NULL;
		pKernel->GetCmProgram(pProgram);
		if (pProgram == NULL) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}

		UINT indexInProgramArray = pProgram->GetProgramIndex();

		if (pProgram == m_ProgramArray.GetElement(indexInProgramArray)) {
			CmKernel::Destroy(pKernel, pProgram);

			if (pKernel == NULL) {
				m_KernelArray.SetElement(indexInKernelArrary,
							 NULL);
			}

			if (pProgram == NULL) {
				m_ProgramArray.SetElement(indexInProgramArray,
							  NULL);
			}
			return CM_SUCCESS;
		} else {
			CM_ASSERT(0);
			return CM_FAILURE;
		}
	} else {
		CM_ASSERT(0);
		return CM_FAILURE;
	}
	return CM_SUCCESS;
}

CM_RT_API INT CmDevice::CreateQueue(CmQueue * &pQueue)
{
	pQueue = m_pQueue;
	return CM_SUCCESS;
}

CM_RT_API INT CmDevice::CreateTask(CmTask * &pTask)
{
	CLock locker(m_CriticalSection_Task);

	UINT freeSlotInTaskArray = m_TaskArray.GetFirstFreeIndex();
	INT result = CmTask::Create(this, freeSlotInTaskArray,
				    m_HalMaxValues.iMaxKernelsPerTask, pTask);
	if (result == CM_SUCCESS) {
		m_TaskArray.SetElement(freeSlotInTaskArray, pTask);
		m_TaskCount++;
	}
	return result;
}

INT CmDevice::DestroyQueue(CmQueue * &pQueue)
{
	if (pQueue == NULL) {
		return CM_FAILURE;
	}

	return CmQueue::Destroy(pQueue);
}

CM_RT_API INT CmDevice::DestroyTask(CmTask * &pTask)
{

	CLock locker(m_CriticalSection_Task);

	if (pTask == NULL) {
		return CM_FAILURE;
	}

	UINT index = pTask->GetIndexInTaskArray();

	if (pTask == (CmTask *) m_TaskArray.GetElement(index)) {
		INT status = CmTask::Destroy(pTask);
		if (status == CM_SUCCESS) {
			m_TaskArray.SetElement(index, NULL);
			pTask = NULL;
			return CM_SUCCESS;
		} else {
			CM_ASSERT(0);
			return status;
		}
	} else {
		CM_ASSERT(0);
		return CM_FAILURE;
	}
}

CM_RT_API INT
    CmDevice::CreateThreadSpace(UINT width, UINT height, CmThreadSpace * &pTS)
{
	CLock locker(m_CriticalSection_ThreadSpace);

	UINT freeSlotInThreadSpaceArray =
	    m_ThreadSpaceArray.GetFirstFreeIndex();
	INT result =
	    CmThreadSpace::Create(this, freeSlotInThreadSpaceArray, width,
				  height,
				  pTS);
	if (result == CM_SUCCESS) {
		m_ThreadSpaceArray.SetElement(freeSlotInThreadSpaceArray, pTS);
		m_ThreadSpaceCount++;
	}

	return result;
}

CM_RT_API INT CmDevice::DestroyThreadSpace(CmThreadSpace * &pTS)
{
	if (pTS == NULL) {
		return CM_FAILURE;
	}

	UINT indexTs = pTS->GetIndexInTsArray();

	CLock locker(m_CriticalSection_ThreadSpace);
	if (pTS == m_ThreadSpaceArray.GetElement(indexTs)) {
		INT status = CmThreadSpace::Destroy(pTS);
		if (status == CM_SUCCESS) {
			m_ThreadSpaceArray.SetElement(indexTs, NULL);
			pTS = NULL;
			return CM_SUCCESS;
		} else {
			CM_ASSERT(0);
			return status;
		}
	} else {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

}

INT CmDevice::GetDDIVersion(UINT & DDIVersion)
{
	DDIVersion = m_DDIVersion;
	return CM_SUCCESS;
}

CM_RT_API INT
    CmDevice::CreateThreadGroupSpace(UINT thrdSpaceWidth, UINT thrdSpaceHeight,
				     UINT grpSpaceWidth, UINT grpSpaceHeight,
				     CmThreadGroupSpace * &pTGS)
{
	CLock locker(m_CriticalSection_ThreadGroupSpace);

	UINT firstfreeslot = m_ThreadGroupSpaceArray.GetFirstFreeIndex();
	INT result =
	    CmThreadGroupSpace::Create(this, firstfreeslot, thrdSpaceWidth,
				       thrdSpaceHeight, grpSpaceWidth,
				       grpSpaceHeight, pTGS);
	if (result == CM_SUCCESS) {
		m_ThreadGroupSpaceArray.SetElement(firstfreeslot, pTGS);
		m_ThreadGroupSpaceCount++;
	}
	return result;
}

CM_RT_API INT CmDevice::DestroyThreadGroupSpace(CmThreadGroupSpace * &pTGS)
{
	if (pTGS == NULL) {
		return CM_FAILURE;
	}

	UINT indexTGs = pTGS->GetIndexInTGsArray();

	CLock locker(m_CriticalSection_ThreadGroupSpace);

	if (pTGS == static_cast <
	    CmThreadGroupSpace *
	    >(m_ThreadGroupSpaceArray.GetElement(indexTGs))) {
		INT status = CmThreadGroupSpace::Destroy(pTGS);
		if (status == CM_SUCCESS) {
			m_ThreadGroupSpaceArray.SetElement(indexTGs, NULL);
			pTGS = NULL;
			return CM_SUCCESS;
		}
	} else {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	return CM_FAILURE;
}

INT CmDevice::GetGenStepInfo(UINT platform, char *&stepinfostr)
{
	INT hr;

	DXVA_CM_QUERY_CAPS queryCaps;
	CmSafeMemSet(&queryCaps, 0, sizeof(queryCaps));
	queryCaps.Type = DXVA_CM_QUERY_STEP;
	UINT queryCapsSize = sizeof(queryCaps);

	if (platform < IGFX_GEN7_5_CORE) {
		stepinfostr = NULL;
		return CM_SUCCESS;
	}

	hr = GetCapsInternal(&queryCaps, &queryCapsSize);
	if (FAILED(hr)) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	UINT stepid = queryCaps.genStepId;
	UINT ulStepId = (1 << stepid);

	switch (ulStepId) {
	case SIWA_ONLY_BDW_A0:
		stepinfostr = (char *)HW_GT_STEPPING_A0;
		break;

	case SIWA_ONLY_HSW_A1:
		stepinfostr = (char *)HW_GT_STEPPING_A1;
		break;

	case SIWA_ONLY_HSW_B0:
		stepinfostr = (char *)HW_GT_STEPPING_B0;
		break;

	case SIWA_ONLY_HSW_C0:
		stepinfostr = (char *)HW_GT_STEPPING_C0;
		break;

	default:
		stepinfostr = NULL;
	}

	return CM_SUCCESS;
}

INT CmDevice::SetCapsInternal(CM_DEVICE_CAP_NAME capName, size_t capValueSize,
			      void *pCapValue)
{
	CM_RETURN_CODE hr = CM_SUCCESS;

	DXVA_CM_SET_CAPS setCaps;
	UINT maxValue;
	size_t size = sizeof(maxValue);
	CmSafeMemSet(&setCaps, 0, sizeof(setCaps));

	switch (capName) {
	case CAP_HW_THREAD_COUNT:
		if (capValueSize != sizeof(UINT)) {
			CM_ASSERT(0);
			return CM_INVALID_HARDWARE_THREAD_NUMBER;
		}

		if (*(UINT *) pCapValue <= 0) {
			CM_ASSERT(0);
			return CM_INVALID_HARDWARE_THREAD_NUMBER;
		}

		GetCaps(CAP_HW_THREAD_COUNT, size, &maxValue);
		if (*(UINT *) pCapValue > maxValue) {
			CM_ASSERT(0);
			return CM_INVALID_HARDWARE_THREAD_NUMBER;
		}

		setCaps.Type = DXVA_CM_MAX_HW_THREADS;
		setCaps.MaxValue = *(UINT *) pCapValue;
		break;

	default:
		return CM_FAILURE;
	}

	PCM_CONTEXT pCmData = (PCM_CONTEXT) this->GetAccelData();
	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnSetCaps(pCmData->pCmHalState,
						  (PCM_HAL_MAX_SET_CAPS_PARAM)
						  & setCaps));

 finish:
	return hr;
}

INT CmDevice::GetSurf2DLookUpEntry(UINT index, PCMLOOKUP_ENTRY & pLookupEntry)
{
	PCM_CONTEXT pCmData = (PCM_CONTEXT) GetAccelData();
	if (pCmData) {
		pLookupEntry = &(pCmData->pCmHalState->pSurf2DTable[index]);
	} else {
		return CM_FAILURE;
	}

	return CM_SUCCESS;
}

INT CmDevice::LoadProgramWithGenCode(void *pCISACode, const UINT uiCISACodeSize,
				     void *pGenCode, const UINT uiGenCodeSize,
				     CmProgram * &pProgram, const char *options)
{
	INT result;
	CLock locker(m_CriticalSection_Program_Kernel);

	UINT firstfreeslot = m_ProgramArray.GetFirstFreeIndex();
	result =
	    CmProgram::Create(this, pCISACode, uiCISACodeSize, pGenCode,
			      uiGenCodeSize, pProgram, options, firstfreeslot);
	if (result == CM_SUCCESS) {
		m_ProgramArray.SetElement(firstfreeslot, pProgram);
		m_ProgramCount++;
	}
	return result;
}

INT CmDevice::GetSurface2DInPool(UINT width, UINT height,
				 CM_SURFACE_FORMAT format,
				 CmSurface2D * &pSurface)
{
	CLock locker(m_CriticalSection_Surface);

	INT result =
	    m_pSurfaceMgr->GetSurface2dInPool(width, height, format, pSurface);
	return result;
}

INT CmDevice::GetSurfaceIDInPool(INT iIndex)
{
	CLock locker(m_CriticalSection_Surface);
	INT result = m_pSurfaceMgr->GetSurfaceIdInPool(iIndex);
	return result;
}

INT CmDevice::DestroySurfaceInPool(UINT & freeSurfNum)
{
	CLock locker(m_CriticalSection_Surface);

	freeSurfNum = m_pSurfaceMgr->TouchSurfaceInPoolForDestroy();
	if ((INT) freeSurfNum < 0) {
		freeSurfNum = 0;
		return CM_FAILURE;
	}

	return CM_SUCCESS;
}

INT CmDevice::InitDevCreateOption(CM_HAL_CREATE_PARAM & DevCreateParam,
				  UINT DevCreateOption)
{
	UINT MaxTaskNumber = 0;

	DevCreateParam.DisableScratchSpace =
	    (DevCreateOption & CM_DEVICE_CREATE_OPTION_SCRATCH_SPACE_MASK);

	DevCreateParam.EnableSurfaceReuse =
	    (DevCreateOption & CM_DEVICE_CREATE_OPTION_SURFACE_REUSE_ENABLE);

	if (DevCreateParam.DisableScratchSpace) {
		DevCreateParam.ScratchSpaceSize = 0;
	} else {
		DevCreateParam.ScratchSpaceSize =
		    (DevCreateOption & CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_MASK)
		    >> CM_DEVICE_CONFIG_SCRATCH_SPACE_SIZE_OFFSET;
	}

	MaxTaskNumber =
	    (DevCreateOption & CM_DEVICE_CONFIG_TASK_NUM_MASK) >>
	    CM_DEVICE_CONFIG_TASK_NUM_OFFSET;

	DevCreateParam.MaxTaskNumber =
	    (MaxTaskNumber + 1) * CM_DEVICE_CONFIG_TASK_NUM_STEP;

	DevCreateParam.bMediaReset = FALSE;

	MaxTaskNumber =
	    (DevCreateOption & CM_DEVICE_CONFIG_EXTRA_TASK_NUM_MASK) >>
	    CM_DEVICE_CONFIG_EXTRA_TASK_NUM_OFFSET;

	DevCreateParam.MaxTaskNumber =
	    (MaxTaskNumber + 1) * DevCreateParam.MaxTaskNumber;

	DevCreateParam.bRequestSliceShutdown =
	    (DevCreateOption & CM_DEVICE_CONFIG_SLICESHUTDOWN_ENABLE) ? TRUE :
	    FALSE;

	DevCreateParam.bRequestCustomGpuContext =
	    (DevCreateOption & CM_DEVICE_CONFIG_GPUCONTEXT_ENABLE) ? TRUE :
	    FALSE;

	DevCreateParam.bSLMMode =
	    (DevCreateOption & CM_DEVICE_CONFIG_SLM_MODE_ENABLE) ? TRUE : FALSE;

	return CM_SUCCESS;
}

BOOL CmDevice::IsScratchSpaceDisabled()
{
	return m_DevCreateOption.DisableScratchSpace ? TRUE : FALSE;
}

BOOL CmDevice::IsSurfaceReuseEnabled()
{
	return m_DevCreateOption.EnableSurfaceReuse ? TRUE : FALSE;
}

UINT CmDevice::ValidSurfaceIndexStart()
{
	return (CM_NULL_SURFACE_BINDING_INDEX + 1);
}

UINT CmDevice::MaxIndirectSurfaceCount()
{
	return (GT_RESERVED_INDEX_START - CM_GLOBAL_SURFACE_NUMBER - 1);
}

BOOL CmDevice::IsCmReservedSurfaceIndex(UINT surfBTI)
{
	if (surfBTI >= CM_GLOBAL_SURFACE_INDEX_START
	    && surfBTI <
	    (CM_GLOBAL_SURFACE_INDEX_START + CM_GLOBAL_SURFACE_NUMBER))
		return TRUE;
	else
		return FALSE;
}

BOOL CmDevice::IsValidSurfaceIndex(UINT surfBTI)
{
	UINT genid;
	GetGenPlatform(genid);

	if (surfBTI > CM_NULL_SURFACE_BINDING_INDEX
	    && surfBTI < CM_GLOBAL_SURFACE_INDEX_START)
		return TRUE;
	else
		return FALSE;
}

EXTERN_C INT
CreateCmDevice(CmDevice * &pDevice, UINT & version,
	       CmDriverContext * pDriverContext, UINT DevCreateOption)
{
	INT result = CM_SUCCESS;
	pDevice = NULL;

	result = CmDevice::Create(pDriverContext, pDevice, DevCreateOption);
	if (result == CM_SUCCESS) {
		version = CURRENT_CM_VERSION;
	} else {
		version = 0;
	}

	return result;
}

EXTERN_C INT DestroyCmDevice(CmDevice * &pDevice)
{
	INT result = CM_SUCCESS;

	result = CmDevice::Destroy(pDevice);
	if (result == CM_SUCCESS) {
		pDevice = NULL;
	}

	return result;
}
