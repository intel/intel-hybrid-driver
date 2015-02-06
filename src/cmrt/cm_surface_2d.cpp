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
#include "cm_surface_manager.h"
#include "cm_device.h"
#include "cm_perf.h"
#include "cm_mem.h"
#include "hal_cm.h"
#include "cm_queue.h"


INT CmSurface2D::Create(UINT index,
			UINT handle,
			UINT width,
			UINT height,
			UINT pitch,
			CM_SURFACE_FORMAT format,
			BOOL isCmCreated,
			CmSurfaceManager * pSurfaceManager,
			CmSurface2D * &pSurface)
{
	INT result = CM_SUCCESS;

	pSurface =
	    new(std::nothrow) CmSurface2D(handle, width, height, pitch, format,
					  pSurfaceManager, isCmCreated);
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

 CmSurface2D::CmSurface2D(UINT handle, UINT width, UINT height, UINT pitch, CM_SURFACE_FORMAT format, CmSurfaceManager * pSurfaceManager, BOOL isCmCreated):
CmSurface(pSurfaceManager, isCmCreated),
m_Width(width),
m_Height(height),
m_Handle(handle), m_Pitch(pitch), m_Format(format), m_SubResourceIndex(0)
{
	CmSurface::SetMemoryObjectControl(MEMORY_OBJECT_CONTROL_UNKNOW,
					  CM_USE_PTE, 0);
}

CmSurface2D::~CmSurface2D(void)
{
}

INT CmSurface2D::Initialize(UINT index)
{
	return CmSurface::Initialize(index);
}

CM_RT_API INT
    CmSurface2D::WriteSurface(const unsigned char *pSysMem, CmEvent * pEvent,
			      UINT64 sysMemSize)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	BYTE *pDst = NULL;
	BYTE *pSurf = NULL;
	UINT sizePerPixel = 0;
	UINT updatedHeight = 0;
	UINT size = 0;
	UINT pitch = 0;
	UINT row = 0;

	if (pSysMem == NULL) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	if (pEvent) {
		FlushDeviceQueue(pEvent);
	}
	WaitForReferenceFree();

	CmDevice *pCmDevice = NULL;
	m_SurfaceMgr->GetCmDevice(pCmDevice);

	CSync *pSurfaceLock = pCmDevice->GetSurfaceLock();
	CM_ASSERT(pSurfaceLock);
	CLock locker(*pSurfaceLock);

	PCM_CONTEXT pCmData = (PCM_CONTEXT) pCmDevice->GetAccelData();

	CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM inParam;
	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM));
	inParam.iWidth = m_Width;
	inParam.iHeight = m_Height;
	inParam.dwHandle = m_Handle;
	inParam.iLockFlag = CM_HAL_LOCKFLAG_WRITEONLY;

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnLock2DResource(pCmData->pCmHalState,
							 &inParam));
	CMCHK_NULL(inParam.pData);

	pDst = (BYTE *) (inParam.pData);
	pSurf = (BYTE *) pSysMem;

	CMCHK_HR(m_SurfaceMgr->GetPixelBytesAndHeight
		 (m_Width, m_Height, m_Format, sizePerPixel, updatedHeight));

	size = m_Width * sizePerPixel;
	pitch = m_Pitch;
	if (pitch != size) {
		for (row = 0; row < updatedHeight; row++) {
			CmFastMemCopyWC(pDst, pSurf, size);

			pSurf += size;
			pDst += pitch;
		}
	} else {
		CmFastMemCopyWC(pDst, pSurf, pitch * updatedHeight);
	}

	inParam.pData = NULL;
	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->pfnUnlock2DResource
				       (pCmData->pCmHalState, &inParam));

 finish:
	return hr;
}

CM_RT_API INT
    CmSurface2D::ReadSurface(unsigned char *pSysMem, CmEvent * pEvent,
			     UINT64 sysMemSize)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	BYTE *pDst = NULL;
	BYTE *pSurf = NULL;
	UINT sizePerPixel = 0;
	UINT updatedHeight = 0;
	UINT widthInByte = 0;
	UINT pitch = 0;
	UINT row = 0;

	if (pSysMem == NULL) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	if (pEvent) {
		FlushDeviceQueue(pEvent);
	}

	WaitForReferenceFree();

	CmDevice *pCmDevice = NULL;
	m_SurfaceMgr->GetCmDevice(pCmDevice);

	CSync *pSurfaceLock = pCmDevice->GetSurfaceLock();
	CM_ASSERT(pSurfaceLock);
	CLock locker(*pSurfaceLock);

	PCM_CONTEXT pCmData = (PCM_CONTEXT) pCmDevice->GetAccelData();

	CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM inParam;
	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM));
	inParam.iWidth = m_Width;
	inParam.iHeight = m_Height;
	inParam.dwHandle = m_Handle;
	inParam.iLockFlag = CM_HAL_LOCKFLAG_READONLY;

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnLock2DResource(pCmData->pCmHalState,
							 &inParam));
	CMCHK_NULL(inParam.pData);

	pDst = (BYTE *) pSysMem;
	pSurf = (BYTE *) (inParam.pData);

	CMCHK_HR(m_SurfaceMgr->GetPixelBytesAndHeight
		 (m_Width, m_Height, m_Format, sizePerPixel, updatedHeight));

	widthInByte = m_Width * sizePerPixel;
	pitch = m_Pitch;
	if (pitch != widthInByte) {
		for (row = 0; row < updatedHeight; row++) {
			CmFastMemCopyFromWC(pDst, pSurf, widthInByte,
					    GetCpuInstructionLevel());
			pSurf += pitch;
			pDst += widthInByte;
		}
	} else {
		CmFastMemCopyFromWC(pDst, pSurf, pitch * updatedHeight,
				    GetCpuInstructionLevel());
	}

	inParam.pData = NULL;
	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->pfnUnlock2DResource
				       (pCmData->pCmHalState, &inParam));

 finish:
	return hr;
}

CM_RT_API INT CmSurface2D::GetIndex(SurfaceIndex * &pIndex)
{
	pIndex = m_pIndex;
	return CM_SUCCESS;
}

CM_RT_API INT
    CmSurface2D::WriteSurfaceStride(const unsigned char *pSysMem,
				    CmEvent * pEvent, const UINT stride,
				    UINT64 sysMemSize)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	BYTE *pDst = NULL;
	BYTE *pSrc = NULL;
	UINT sizePerPixel = 0;
	UINT updatedHeight = 0;
	UINT widthInByte = 0;
	UINT pitch = 0;
	UINT row = 0;

	if (pSysMem == NULL) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	if (pEvent) {
		FlushDeviceQueue(pEvent);
	}

	WaitForReferenceFree();

	CmDevice *pCmDevice = NULL;
	m_SurfaceMgr->GetCmDevice(pCmDevice);

	CSync *pSurfaceLock = pCmDevice->GetSurfaceLock();
	CM_ASSERT(pSurfaceLock);
	CLock locker(*pSurfaceLock);

	PCM_CONTEXT pCmData = (PCM_CONTEXT) pCmDevice->GetAccelData();

	CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM inParam;
	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM));
	inParam.iWidth = m_Width;
	inParam.iHeight = m_Height;
	inParam.dwHandle = m_Handle;
	inParam.iLockFlag = CM_HAL_LOCKFLAG_WRITEONLY;

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnLock2DResource(pCmData->pCmHalState,
							 &inParam));
	CMCHK_NULL(inParam.pData);

	pDst = (BYTE *) (inParam.pData);
	pSrc = (BYTE *) pSysMem;

	CMCHK_HR(m_SurfaceMgr->GetPixelBytesAndHeight
		 (m_Width, m_Height, m_Format, sizePerPixel, updatedHeight));

	widthInByte = m_Width * sizePerPixel;
	pitch = m_Pitch;
	if ((pitch != widthInByte) || (stride != pitch)) {
		for (row = 0; row < updatedHeight; row++) {
			CmFastMemCopyWC(pDst, pSrc, widthInByte);
			pSrc += stride;
			pDst += pitch;
		}
	} else {
		CmFastMemCopyWC(pDst, pSrc, pitch * updatedHeight);
	}

	inParam.pData = NULL;
	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->pfnUnlock2DResource
				       (pCmData->pCmHalState, &inParam));

 finish:
	return hr;
}

CM_RT_API INT
    CmSurface2D::ReadSurfaceStride(unsigned char *pSysMem, CmEvent * pEvent,
				   const UINT stride, UINT64 sysMemSize)
{
	return ReadSurfaceFullStride(pSysMem, pEvent, stride, m_Height,
				     sysMemSize);
}

CM_RT_API INT
    CmSurface2D::ReadSurfaceFullStride(unsigned char *pSysMem, CmEvent * pEvent,
				       const UINT iWidthStride,
				       const UINT iHeightStride,
				       UINT64 sysMemSize)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	BYTE *pDst = NULL;
	BYTE *pSrc = NULL;
	UINT sizePerPixel = 0;
	UINT updatedHeight = 0;

	if (pSysMem == NULL) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	if (pEvent) {
		FlushDeviceQueue(pEvent);
	}

	WaitForReferenceFree();

	CmDevice *pCmDevice = NULL;
	m_SurfaceMgr->GetCmDevice(pCmDevice);

	CSync *pSurfaceLock = pCmDevice->GetSurfaceLock();
	CM_ASSERT(pSurfaceLock);
	CLock locker(*pSurfaceLock);

	PCM_CONTEXT pCmData = (PCM_CONTEXT) pCmDevice->GetAccelData();

	CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM inParam;
	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM));
	inParam.iWidth = m_Width;
	inParam.iHeight = m_Height;
	inParam.dwHandle = m_Handle;
	inParam.iLockFlag = CM_HAL_LOCKFLAG_READONLY;

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnLock2DResource(pCmData->pCmHalState,
							 &inParam));
	CMCHK_NULL(inParam.pData);

	pDst = (BYTE *) pSysMem;
	pSrc = (BYTE *) (inParam.pData);

	CMCHK_HR(m_SurfaceMgr->GetPixelBytesAndHeight
		 (m_Width, m_Height, m_Format, sizePerPixel, updatedHeight));

	if (m_Format == CM_SURFACE_FORMAT_NV12) {
		for (UINT i = 0; i < m_Height; i++) {
			CmFastMemCopyFromWC(pDst, pSrc, m_Width,
					    GetCpuInstructionLevel());
			pSrc += m_Pitch;
			pDst += iWidthStride;
		}

		pSrc = (BYTE *) (inParam.pData) + m_Height * m_Pitch;
		pDst = (BYTE *) pSysMem + iWidthStride * iHeightStride;

		for (UINT i = 0; i < (m_Height + 1) / 2; i++) {
			CmFastMemCopyFromWC(pDst, pSrc, m_Width,
					    GetCpuInstructionLevel());
			pSrc += m_Pitch;
			pDst += iWidthStride;
		}
	} else {
		UINT size = m_Width * sizePerPixel;
		UINT pitch = m_Pitch;
		if ((pitch != size) || (iWidthStride != size)) {
			for (UINT i = 0; i < updatedHeight; i++) {
				CmFastMemCopyFromWC(pDst, pSrc, size,
						    GetCpuInstructionLevel());
				pSrc += pitch;
				pDst += iWidthStride;
			}
		} else {
			CmFastMemCopyFromWC(pDst, pSrc, pitch * updatedHeight,
					    GetCpuInstructionLevel());
		}
	}

	inParam.pData = NULL;
	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->pfnUnlock2DResource
				       (pCmData->pCmHalState, &inParam));

 finish:
	return hr;
}

CM_RT_API INT
    CmSurface2D::WriteSurfaceFullStride(const unsigned char *pSysMem,
					CmEvent * pEvent,
					const UINT iWidthStride,
					const UINT iHeightStride,
					UINT64 sysMemSize)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	BYTE *pDst = NULL;
	BYTE *pSrc = NULL;
	UINT sizePerPixel = 0;
	UINT updatedHeight = 0;

	if (pSysMem == NULL) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	if (pEvent) {
		FlushDeviceQueue(pEvent);
	}

	WaitForReferenceFree();

	CmDevice *pCmDevice = NULL;
	m_SurfaceMgr->GetCmDevice(pCmDevice);

	CSync *pSurfaceLock = pCmDevice->GetSurfaceLock();
	CM_ASSERT(pSurfaceLock);
	CLock locker(*pSurfaceLock);

	PCM_CONTEXT pCmData = (PCM_CONTEXT) pCmDevice->GetAccelData();

	CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM inParam;
	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM));
	inParam.iWidth = m_Width;
	inParam.iHeight = m_Height;
	inParam.dwHandle = m_Handle;
	inParam.iLockFlag = CM_HAL_LOCKFLAG_WRITEONLY;

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnLock2DResource(pCmData->pCmHalState,
							 &inParam));
	CMCHK_NULL(inParam.pData);

	pDst = (BYTE *) (inParam.pData);
	pSrc = (BYTE *) pSysMem;

	CMCHK_HR(m_SurfaceMgr->GetPixelBytesAndHeight
		 (m_Width, m_Height, m_Format, sizePerPixel, updatedHeight));

	if (m_Format == VA_CM_FMT_NV12) {
		for (UINT i = 0; i < m_Height; i++) {
			CmFastMemCopyFromWC(pDst, pSrc, m_Width,
					    GetCpuInstructionLevel());
			pSrc += iWidthStride;
			pDst += m_Pitch;
		}

		pDst = (BYTE *) (inParam.pData) + m_Height * m_Pitch;
		pSrc = (BYTE *) pSysMem + iWidthStride * iHeightStride;

		for (UINT i = 0; i < (m_Height + 1) / 2; i++) {
			CmFastMemCopyFromWC(pDst, pSrc, m_Width,
					    GetCpuInstructionLevel());
			pSrc += iWidthStride;
			pDst += m_Pitch;
		}
	} else {
		UINT size = m_Width * sizePerPixel;
		UINT pitch = m_Pitch;
		if ((pitch != size) || (iWidthStride != pitch)) {
			for (UINT i = 0; i < updatedHeight; i++) {
				CmFastMemCopyWC(pDst, pSrc, size);
				pSrc += iWidthStride;
				pDst += pitch;
			}
		} else {
			CmFastMemCopyWC(pDst, pSrc, pitch * updatedHeight);
		}
	}

	inParam.pData = NULL;
	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->pfnUnlock2DResource
				       (pCmData->pCmHalState, &inParam));

 finish:
	return hr;
}

INT CmSurface2D::GetHandle(UINT & handle)
{
	handle = m_Handle;
	return CM_SUCCESS;
}

INT CmSurface2D::GetIndexFor2D(UINT & index)
{
	index = m_Handle;
	return CM_SUCCESS;
}

INT CmSurface2D::SetSurfaceProperties(UINT width, UINT height,
				      CM_SURFACE_FORMAT format)
{
	if (format == (CM_SURFACE_FORMAT) FOURCC_NV12) {
		m_Pitch = GENOS_ALIGN_CEIL(width * m_Pitch / m_Width, 2);
	}
	m_Width = width;
	m_Height = height;
	m_Format = format;

	return CM_SUCCESS;
}

CM_RT_API INT
    CmSurface2D::GetSurfaceDesc(UINT & width, UINT & height,
				CM_SURFACE_FORMAT & format, UINT & sizeperpixel)
{
	int ret = CM_SUCCESS;
	UINT updatedHeight = 0;

	width = m_Width;
	height = m_Height;
	format = m_Format;

	ret =
	    m_SurfaceMgr->GetPixelBytesAndHeight(width, height, format,
						 sizeperpixel, updatedHeight);

	return ret;
}

CM_RT_API INT CmSurface2D::InitSurface(const DWORD pInitValue, CmEvent * pEvent)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	CmDevice *pCmDevice = NULL;
	PCM_CONTEXT pCmData = NULL;
	CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM inParam;
	UINT pitch = 0;
	DWORD *pSurf = NULL;
	UINT widthInBytes = 0;

	if (pEvent) {
		FlushDeviceQueue(pEvent);
	}

	WaitForReferenceFree();

	UINT sizePerPixel = 0;
	UINT updatedHeight = 0;
	CMCHK_HR(m_SurfaceMgr->GetPixelBytesAndHeight
		 (m_Width, m_Height, m_Format, sizePerPixel, updatedHeight));

	m_SurfaceMgr->GetCmDevice(pCmDevice);
	pCmData = (PCM_CONTEXT) pCmDevice->GetAccelData();

	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_SURFACE2D_LOCK_UNLOCK_PARAM));
	inParam.iWidth = m_Width;
	inParam.iHeight = m_Height;
	inParam.dwHandle = m_Handle;
	inParam.iLockFlag = CM_HAL_LOCKFLAG_WRITEONLY;

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnLock2DResource(pCmData->pCmHalState,
							 &inParam));
	CMCHK_NULL(inParam.pData);

	pitch = inParam.iPitch;
	pSurf = (DWORD *) inParam.pData;

	widthInBytes = m_Width * sizePerPixel;
	if (widthInBytes != pitch) {
		for (UINT i = 0; i < updatedHeight; i++) {
			CmDwordMemSet(pSurf, pInitValue, widthInBytes);
			pSurf += (pitch >> 2);
		}
	} else {
		CmDwordMemSet(pSurf, pInitValue, pitch * updatedHeight);
	}

	inParam.pData = NULL;
	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->pfnUnlock2DResource
				       (pCmData->pCmHalState, &inParam));

 finish:
	return hr;
}

CM_RT_API INT
    CmSurface2D::SetMemoryObjectControl(MEMORY_OBJECT_CONTROL mem_ctrl,
					MEMORY_TYPE mem_type, UINT age)
{
	CmSurface::SetMemoryObjectControl(mem_ctrl, mem_type, age);

	return CM_SUCCESS;
}

INT CmSurface2D::GetSubResourceIndex(UINT & nIndex)
{
	nIndex = m_SubResourceIndex;
	return CM_SUCCESS;
}

INT CmSurface2D::SetSubResourceIndex(UINT nIndex)
{
	m_SubResourceIndex = nIndex;
	return CM_SUCCESS;
}

CM_RT_API INT
    CmSurface2D::SetSurfaceStateDimensions(UINT iWidth, UINT iHeight,
					   SurfaceIndex * pSurfIndex)
{
	CM_RETURN_CODE hr = CM_SUCCESS;

	if ((iWidth > m_Width) || (iHeight > m_Height)) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	if (pSurfIndex != NULL) {
		CM_ASSERT(0);
		return CM_NOT_IMPLEMENTED;
	}

	CmDevice *pCmDevice = NULL;
	m_SurfaceMgr->GetCmDevice(pCmDevice);
	PCM_CONTEXT pCmData = (PCM_CONTEXT) pCmDevice->GetAccelData();

	CM_HAL_SURFACE2D_SURFACE_STATE_DIMENSIONS_PARAM inParam;
	CmSafeMemSet(&inParam, 0, sizeof(inParam));
	inParam.iWidth = iWidth;
	inParam.iHeight = iHeight;
	inParam.dwHandle = m_Handle;

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->pCmHalState->
				       pfnSet2DSurfaceStateDimensions(pCmData->
								      pCmHalState,
								      &inParam));

 finish:
	return hr;
}

INT CmSurface2D::SetReadSyncFlag()
{
	HRESULT hr = CM_SUCCESS;

	CmDevice *pCmDevice = NULL;
	m_SurfaceMgr->GetCmDevice(pCmDevice);
	PCM_CONTEXT pCmData = (PCM_CONTEXT) pCmDevice->GetAccelData();

	hr = pCmData->pCmHalState->pfnSetSurfaceReadFlag(pCmData->pCmHalState,
							 m_Handle);

	if (FAILED(hr)) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	return hr;
}
