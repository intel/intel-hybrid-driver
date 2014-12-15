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

#include "cm_surface_manager.h"
#include "cm_debug.h"
#include "hal_cm.h"
#include "cm_queue.h"

INT CmSurfaceManager::UPDATE_STATE_FOR_DELAYED_DESTROY(SURFACE_DESTROY_KIND
						       destroyKind, UINT i)
{
	switch (destroyKind) {
	case DELAYED_DESTROY:
	case GC_DESTROY:
		if (m_SurfaceCached[i]) {
			return CM_SURFACE_CACHED;
		}

		if (!m_SurfaceReleased[i] || m_SurfaceState[i]) {
			return CM_SURFACE_IN_USE;
		}
		break;

	case APP_DESTROY:
		m_SurfaceReleased[i] = TRUE;
		if (m_SurfaceState[i]) {
			return CM_SURFACE_IN_USE;
		}
		break;

	default:
		CM_ASSERT(0);
		return CM_FAILURE;

	}

	return CM_SUCCESS;
}

INT CmSurfaceManager::UPDATE_STATE_FOR_REUSE_DESTROY(SURFACE_DESTROY_KIND
						     destroyKind, UINT i)
{
	switch (destroyKind) {
	case DELAYED_DESTROY:
		if (m_SurfaceCached[i]) {
			return CM_SURFACE_CACHED;
		}

		if (!m_SurfaceReleased[i] || m_SurfaceState[i]) {
			return CM_SURFACE_IN_USE;
		}
		break;

	case APP_DESTROY:
		m_SurfaceReleased[i] = TRUE;

		if (m_SurfaceReleased[i] && !m_SurfaceCached[i]) {
			m_SurfaceCached[i] = TRUE;
		}
		return CM_SURFACE_CACHED;

	case GC_DESTROY:
		if (!m_SurfaceReleased[i] || m_SurfaceState[i]) {
			return CM_SURFACE_IN_USE;
		}
		break;

	default:
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	return CM_SUCCESS;
}

INT CmSurfaceManager::UPDATE_STATE_FOR_REAL_DESTROY(UINT i,
						    CM_ENUM_CLASS_TYPE kind)
{
	m_SurfaceReleased[i] = FALSE;
	m_SurfaceCached[i] = FALSE;
	m_SurfaceArray[i] = NULL;
	m_SurfaceDestroyID[i]++;
	m_SurfaceSizes[i] = 0;

	switch (kind) {
	case CM_ENUM_CLASS_TYPE_CMBUFFER_RT:
		m_bufferCount--;
		break;
	case CM_ENUM_CLASS_TYPE_CMSURFACE2D:
		m_2DSurfaceCount--;
		break;
	case CM_ENUM_CLASS_TYPE_CMSURFACE2DUP:
		m_2DUPSurfaceCount--;
		break;
	default:
		CM_ASSERT(0);
		break;
	}

	return CM_SUCCESS;
}

INT CmSurfaceManager::UPDATE_STATE_FOR_SURFACE_REUSE(UINT i)
{
	m_SurfaceCached[i] = FALSE;
	m_SurfaceReleased[i] = FALSE;

	return CM_SUCCESS;
}

INT CmSurfaceManager::UPDATE_PROFILE_FOR_2D_SURFACE(UINT index, UINT width,
						    UINT height,
						    CM_SURFACE_FORMAT format,
						    BOOL reuse)
{
	UINT size = 0;
	UINT sizeperpixel = 1;

	GetFormatSize(format, sizeperpixel);
	size = width * height * sizeperpixel;

	m_2DSurfaceAllCount++;
	m_2DSurfaceAllSize += size;
	if (reuse) {
		m_2DSurfaceReuseCount++;
		m_2DSurfaceReuseSize += size;
	} else {
		m_2DSurfaceCount++;
		m_SurfaceSizes[index] = size;
	}

	return CM_SUCCESS;
}

INT CmSurfaceManager::UPDATE_PROFILE_FOR_1D_SURFACE(UINT index, UINT size,
						    BOOL reuse)
{
	m_bufferAllCount++;
	m_bufferAllSize += size;
	if (reuse) {
		m_bufferReuseSize += size;
		m_bufferReuseCount++;
	} else {
		m_bufferCount++;
		m_SurfaceSizes[index] = size;
	}

	return CM_SUCCESS;
}

INT CmSurfaceManager::Create(CmDevice * pCmDevice,
			     CM_HAL_MAX_VALUES HalMaxValues,
			     CM_HAL_MAX_VALUES_EX HalMaxValuesEx,
			     CmSurfaceManager * &pManager)
{
	INT result = CM_SUCCESS;

	pManager = new(std::nothrow) CmSurfaceManager(pCmDevice);
	if (pManager) {
		result = pManager->Initialize(HalMaxValues, HalMaxValuesEx);
		if (result != CM_SUCCESS) {
			CmSurfaceManager::Destroy(pManager);
		}
	} else {
		CM_ASSERT(0);
		result = CM_OUT_OF_HOST_MEMORY;
	}

	return result;
}

INT CmSurfaceManager::Destroy(CmSurfaceManager * &pManager)
{
	if (pManager) {
		delete pManager;
		pManager = NULL;
	}

	return CM_SUCCESS;
}

 CmSurfaceManager::CmSurfaceManager(CmDevice * pCmDevice):
m_pCmDevice(pCmDevice),
m_SurfaceArraySize(0),
m_SurfaceArray(NULL),
m_SurfaceState(NULL),
m_SurfaceCached(NULL),
m_SurfaceReleased(NULL),
m_SurfaceDestroyID(NULL),
m_SurfaceSizes(NULL),
m_maxBufferCount(0),
m_bufferCount(0),
m_max2DSurfaceCount(0),
m_2DSurfaceCount(0),
m_max2DUPSurfaceCount(0),
m_2DUPSurfaceCount(0),
m_bufferAllCount(0),
m_2DSurfaceAllCount(0),
m_bufferAllSize(0),
m_2DSurfaceAllSize(0),
m_bufferReuseCount(0),
m_2DSurfaceReuseCount(0),
m_bufferReuseSize(0),
m_2DSurfaceReuseSize(0),
m_GCTriggerTimes(0), m_GCCollected1DSize(0), m_GCCollected2DSize(0)
{
};

CmSurfaceManager::~CmSurfaceManager(void)
{
	for (UINT i = m_pCmDevice->ValidSurfaceIndexStart();
	     i < m_SurfaceArraySize; i++) {
		DestroySurfaceArrayElement(i);
	}

#ifdef SURFACE_MANAGE_PROFILE
	printf("\n\n");
	printf("Total %d 1D buffers, with size: %d\n", m_bufferAllCount,
	       m_bufferAllSize);
	printf("Total %d 2D surfaces, with size: %d\n", m_2DSurfaceAllCount,
	       m_2DSurfaceAllSize);

	printf("\nReused %d 1D buffers, with size: %d\n", m_bufferReuseCount,
	       m_bufferReuseSize);
	printf("Reused %d 2D surfaces, with size: %d\n", m_2DSurfaceReuseCount,
	       m_2DSurfaceReuseSize);

	printf("\nGC trigger times: %d\n", m_GCTriggerTimes);
	printf("GC collected 1D surface size: %d\n", m_GCCollected1DSize);
	printf("GC collected 2D surface size: %d\n", m_GCCollected2DSize);

	printf("\n\n");
#endif

	CmSafeDeleteArray(m_SurfaceState);
	CmSafeDeleteArray(m_SurfaceCached);
	CmSafeDeleteArray(m_SurfaceReleased);
	CmSafeDeleteArray(m_SurfaceDestroyID);
	CmSafeDeleteArray(m_SurfaceSizes);
	CmSafeDeleteArray(m_SurfaceArray);
}

INT CmSurfaceManager::DestroySurfaceArrayElement(UINT index)
{
	UINT i = index;

	if (i >= m_SurfaceArraySize)
		return CM_FAILURE;

	CmSurface *pSurface = m_SurfaceArray[i];

	if (pSurface) {
		CmSurface2D *pSurf2D = NULL;
		CmBuffer_RT *pSurf1D = NULL;
		CmSurface2DUP *pSurf2DUP = NULL;

		switch (pSurface->Type()) {
		case CM_ENUM_CLASS_TYPE_CMSURFACE2D:
			pSurf2D = static_cast < CmSurface2D * >(pSurface);
			if (pSurf2D) {
				DestroySurface(pSurf2D, FORCE_DESTROY);
			}
			break;

		case CM_ENUM_CLASS_TYPE_CMBUFFER_RT:
			pSurf1D = static_cast < CmBuffer_RT * >(pSurface);
			if (pSurf1D) {
				DestroySurface(pSurf1D, FORCE_DESTROY);
			}
			break;

		case CM_ENUM_CLASS_TYPE_CMSURFACE2DUP:
			pSurf2DUP = static_cast < CmSurface2DUP * >(pSurface);
			if (pSurf2DUP) {
				DestroySurface(pSurf2DUP, FORCE_DESTROY);
			}
			break;
		default:
			break;
		}
	}

	return CM_SUCCESS;
}

INT CmSurfaceManager::Initialize(CM_HAL_MAX_VALUES HalMaxValues,
				 CM_HAL_MAX_VALUES_EX HalMaxValuesEx)
{
	UINT totalSurfaceCount =
	    HalMaxValues.iMaxBufferTableSize +
	    HalMaxValues.iMax2DSurfaceTableSize +
	    HalMaxValues.iMax3DSurfaceTableSize +
	    HalMaxValuesEx.iMax2DUPSurfaceTableSize;
	m_SurfaceArraySize = totalSurfaceCount;

	m_maxBufferCount = HalMaxValues.iMaxBufferTableSize;
	m_max2DSurfaceCount = HalMaxValues.iMax2DSurfaceTableSize;
	m_max2DUPSurfaceCount = HalMaxValuesEx.iMax2DUPSurfaceTableSize;

	typedef CmSurface *PCMSURFACE;
	m_SurfaceArray = new(std::nothrow) PCMSURFACE[m_SurfaceArraySize];
	m_SurfaceState = new(std::nothrow) INT[m_SurfaceArraySize];
	m_SurfaceCached = new(std::nothrow) BOOL[m_SurfaceArraySize];
	m_SurfaceReleased = new(std::nothrow) BOOL[m_SurfaceArraySize];
	m_SurfaceDestroyID = new(std::nothrow) INT[m_SurfaceArraySize];
	m_SurfaceSizes = new(std::nothrow) INT[m_SurfaceArraySize];
	if (m_SurfaceArray == NULL ||
	    m_SurfaceState == NULL ||
	    m_SurfaceCached == NULL ||
	    m_SurfaceReleased == NULL ||
	    m_SurfaceDestroyID == NULL || m_SurfaceSizes == NULL) {
		CmSafeDeleteArray(m_SurfaceState);
		CmSafeDeleteArray(m_SurfaceCached);
		CmSafeDeleteArray(m_SurfaceReleased);
		CmSafeDeleteArray(m_SurfaceDestroyID);
		CmSafeDeleteArray(m_SurfaceSizes);
		CmSafeDeleteArray(m_SurfaceArray);

		CM_ASSERT(0);
		return CM_OUT_OF_HOST_MEMORY;
	}

	CmSafeMemSet(m_SurfaceArray, 0,
		     m_SurfaceArraySize * sizeof(CmSurface *));
	CmSafeMemSet(m_SurfaceState, 0, m_SurfaceArraySize * sizeof(INT));
	CmSafeMemSet(m_SurfaceCached, 0, m_SurfaceArraySize * sizeof(BOOL));
	CmSafeMemSet(m_SurfaceReleased, 0, m_SurfaceArraySize * sizeof(BOOL));
	CmSafeMemSet(m_SurfaceDestroyID, 0, m_SurfaceArraySize * sizeof(INT));
	CmSafeMemSet(m_SurfaceSizes, 0, m_SurfaceArraySize * sizeof(INT));
	return CM_SUCCESS;
}

INT CmSurfaceManager::DestroySurfaceInPool(UINT & freeSurfNum,
					   SURFACE_DESTROY_KIND destroyKind)
{
	CmSurface *pSurface = NULL;
	CmBuffer_RT *pSurf1D = NULL;
	CmSurface2D *pSurf2D = NULL;
	CmSurface2DUP *pSurf2DUP = NULL;
	INT status = CM_FAILURE;
	UINT index = m_pCmDevice->ValidSurfaceIndexStart();

	freeSurfNum = 0;

	while (index < m_SurfaceArraySize) {
		pSurface = m_SurfaceArray[index];
		if (!pSurface) {
			index++;
			continue;
		}

		status = CM_FAILURE;

		switch (pSurface->Type()) {
		case CM_ENUM_CLASS_TYPE_CMSURFACE2D:
			pSurf2D = static_cast < CmSurface2D * >(pSurface);
			if (pSurf2D) {
				status = DestroySurface(pSurf2D, destroyKind);
			}
			break;

		case CM_ENUM_CLASS_TYPE_CMBUFFER_RT:
			pSurf1D = static_cast < CmBuffer_RT * >(pSurface);
			if (pSurf1D) {
				status = DestroySurface(pSurf1D, destroyKind);
			}
			break;

		case CM_ENUM_CLASS_TYPE_CMSURFACE2DUP:
			pSurf2DUP = static_cast < CmSurface2DUP * >(pSurface);
			if (pSurf2DUP) {
				status = DestroySurface(pSurf2DUP, destroyKind);
			}
			break;

		default:
			CM_ASSERT(0);
			break;
		}

		if (status == CM_SUCCESS) {
			freeSurfNum++;
		}
		index++;
	}

	return CM_SUCCESS;
}

INT CmSurfaceManager::TouchSurfaceInPoolForDestroy()
{
	UINT freeNum = 0;
	CmQueue *pCmQueue = NULL;

	m_pCmDevice->GetQueue(pCmQueue);

	DestroySurfaceInPool(freeNum, GC_DESTROY);
	while (!freeNum) {
		if (FAILED(pCmQueue->TouchFlushedTasks())) {
			CM_ASSERT(0);
			return CM_FAILURE;
		} else {
			DestroySurfaceInPool(freeNum, GC_DESTROY);
		}
	}

	m_GCTriggerTimes++;

	return freeNum;
}

INT CmSurfaceManager::GetFreeSurfaceIndexFromPool(UINT & freeIndex)
{
	UINT index = m_pCmDevice->ValidSurfaceIndexStart();

	while ((index < m_SurfaceArraySize) && m_SurfaceArray[index]) {
		index++;
	}

	if (index >= m_SurfaceArraySize) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	freeIndex = index;

	return CM_SUCCESS;
}

INT CmSurfaceManager::GetFreeSurfaceIndex(UINT & freeIndex)
{
	UINT index = 0;

	if (GetFreeSurfaceIndexFromPool(index) != CM_SUCCESS) {
		if (!TouchSurfaceInPoolForDestroy()) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}
		if (GetFreeSurfaceIndexFromPool(index) != CM_SUCCESS) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}
	}

	freeIndex = index;

	return CM_SUCCESS;
}

INT CmSurfaceManager::GetFormatSize(CM_SURFACE_FORMAT format,
				    UINT & sizePerPixel)
{
	switch (format) {
	case CM_SURFACE_FORMAT_X8R8G8B8:
	case CM_SURFACE_FORMAT_A8R8G8B8:
	case CM_SURFACE_FORMAT_R32F:
		sizePerPixel = 4;
		break;

	case CM_SURFACE_FORMAT_V8U8:
	case CM_SURFACE_FORMAT_UYVY:
	case CM_SURFACE_FORMAT_YUY2:
	case CM_SURFACE_FORMAT_R16_UINT:
		sizePerPixel = 2;
		break;

	case CM_SURFACE_FORMAT_A8:
	case CM_SURFACE_FORMAT_P8:
	case CM_SURFACE_FORMAT_R8_UINT:
		sizePerPixel = 1;
		break;

	case CM_SURFACE_FORMAT_NV12:
	case CM_SURFACE_FORMAT_YV12:
	case CM_SURFACE_FORMAT_444P:
	case CM_SURFACE_FORMAT_422H:
	case CM_SURFACE_FORMAT_411P:
	case CM_SURFACE_FORMAT_422V:
	case CM_SURFACE_FORMAT_IMC3:
		sizePerPixel = 1;
		break;

	default:
		CM_ASSERT(0);
		return CM_SURFACE_FORMAT_NOT_SUPPORTED;
	}

	return CM_SUCCESS;
}

inline INT CmSurfaceManager::GetMemorySizeOfSurfaces()
{
	UINT index = m_pCmDevice->ValidSurfaceIndexStart();
	UINT MemSize = 0;

	while ((index < m_SurfaceArraySize)) {
		if (!m_SurfaceArray[index]) {
			index++;
			continue;
		}

		MemSize += m_SurfaceSizes[index];
		index++;
	}

	return MemSize;
}

#define REUSE_SIZE_FACTOR 1.5
INT CmSurfaceManager::GetReuseSurfaceIndex(UINT width, UINT height, UINT depth,
					   CM_SURFACE_FORMAT format)
{
	UINT index = m_pCmDevice->ValidSurfaceIndexStart();
	UINT s_width = 0;
	UINT s_height = 0;
	CM_SURFACE_FORMAT s_format = CM_SURFACE_FORMAT_UNKNOWN;
	UINT minSize = 0;
	UINT t_index = 0;
	UINT s_sizeperpixel = 0;

	while (index < m_SurfaceArraySize) {
		if (!m_SurfaceArray[index]) {
			index++;
			continue;
		}

		if (m_SurfaceCached[index]) {
			CmSurface *pSurface = m_SurfaceArray[index];
			if (width && height) {
				CmSurface2D *pSurf2D = NULL;
				if (pSurface
				    && (pSurface->Type() ==
					CM_ENUM_CLASS_TYPE_CMSURFACE2D)) {
					pSurf2D =
					    static_cast <
					    CmSurface2D * >(pSurface);
				}

				if (!pSurf2D) {
					index++;
					continue;
				}
				pSurf2D->GetSurfaceDesc(s_width, s_height,
							s_format,
							s_sizeperpixel);
				if (width <= s_width && height <= s_height
				    && s_format == format
				    && (width * s_sizeperpixel % 4 == 0)) {
					UINT t_size;
					t_size =
					    s_width * s_height * s_sizeperpixel;
					if (!minSize || minSize > t_size) {
						minSize = t_size;
						t_index = index;
					}
				}
			} else if (width && !height) {
				CmBuffer_RT *pBuffer = NULL;
				if (pSurface
				    && (pSurface->Type() ==
					CM_ENUM_CLASS_TYPE_CMBUFFER_RT)) {
					pBuffer =
					    static_cast <
					    CmBuffer_RT * >(pSurface);
				}

				if (!pBuffer) {
					index++;
					continue;
				}
				pBuffer->GetSize(s_width);
				if (width <= s_width) {
					if (!minSize || minSize > s_width) {
						minSize = s_width;
						t_index = index;
					}
				}
			}
		}

		index++;
	}

	if (t_index) {
		if (minSize >=
		    width * (height == 0 ? 1 : height) * (depth ==
							  0 ? 1 : depth) *
		    REUSE_SIZE_FACTOR) {
			index = 0;
		} else {
			index = t_index;
		}
	} else {
		index = 0;
	}

	return index;
}

INT CmSurfaceManager::GetSurface2dInPool(UINT width, UINT height,
					 CM_SURFACE_FORMAT format,
					 CmSurface2D * &pSurface2D)
{
	UINT index = m_pCmDevice->ValidSurfaceIndexStart();

	pSurface2D = NULL;

	if (m_2DSurfaceCount >= m_max2DSurfaceCount) {
		if (!TouchSurfaceInPoolForDestroy()) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}
	}

	if (m_pCmDevice->IsSurfaceReuseEnabled()) {
		index = GetReuseSurfaceIndex(width, height, 0, format);
		if (index) {
			CmSurface *pSurface = m_SurfaceArray[index];
			if (pSurface
			    && (pSurface->Type() ==
				CM_ENUM_CLASS_TYPE_CMSURFACE2D)) {
				pSurface2D =
				    static_cast < CmSurface2D * >(pSurface);
			} else {
				CM_ASSERT(0);
				return CM_FAILURE;
			}
			UpdateSurface2D(pSurface2D, width, height, format);
			UPDATE_STATE_FOR_SURFACE_REUSE(index);
			UPDATE_PROFILE_FOR_2D_SURFACE(index, width, height,
						      format, TRUE);

			return CM_SUCCESS;
		}
	}

	return CM_FAILURE;
}

INT CmSurfaceManager::AllocateSurfaceIndex(UINT width, UINT height, UINT depth,
					   CM_SURFACE_FORMAT format,
					   UINT & freeIndex,
					   BOOL & useNewSurface, void *pSysMem)
{
	UINT index = m_pCmDevice->ValidSurfaceIndexStart();

	if ((m_bufferCount >= m_maxBufferCount && width && !height && !depth) ||
	    (m_2DSurfaceCount >= m_max2DSurfaceCount && width && height
	     && !depth)) {
		if (!TouchSurfaceInPoolForDestroy()) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}
	}

	if (m_pCmDevice->IsSurfaceReuseEnabled() && !pSysMem) {
		index = GetReuseSurfaceIndex(width, height, depth, format);
		if (index) {
			useNewSurface = FALSE;
			freeIndex = index;
			m_SurfaceReleased[index] = FALSE;
			UPDATE_STATE_FOR_SURFACE_REUSE(index);
			return CM_SUCCESS;
		}
	}

	if (GetFreeSurfaceIndex(index) != CM_SUCCESS) {
		return CM_FAILURE;
	}

	useNewSurface = TRUE;
	freeIndex = index;
	m_SurfaceReleased[index] = FALSE;

	return CM_SUCCESS;
}

INT CmSurfaceManager::CreateBuffer(UINT size, CM_BUFFER_TYPE type,
				   CmBuffer_RT * &pSurface1D,
				   CmOsResource * pCmOsResource, void *&pSysMem)
{
	UINT index = m_pCmDevice->ValidSurfaceIndexStart();
	pSurface1D = NULL;

	if (pCmOsResource) {
		if (GetFreeSurfaceIndex(index) != CM_SUCCESS) {
			return CM_EXCEED_SURFACE_AMOUNT;
		}
	} else {
		BOOL useNewSurface = TRUE;

		if (AllocateSurfaceIndex
		    (size, 0, 0, CM_SURFACE_FORMAT_UNKNOWN, index,
		     useNewSurface, pSysMem) != CM_SUCCESS) {
			return CM_EXCEED_SURFACE_AMOUNT;
		}

		if (!useNewSurface) {
			CmSurface *pSurface = m_SurfaceArray[index];
			if (pSurface
			    && (pSurface->Type() ==
				CM_ENUM_CLASS_TYPE_CMBUFFER_RT)) {
				pSurface1D =
				    static_cast < CmBuffer_RT * >(pSurface);
			} else {
				return CM_FAILURE;
			}
			UpdateBuffer(pSurface1D, size);
			UPDATE_PROFILE_FOR_1D_SURFACE(index, size, TRUE);

			return CM_SUCCESS;
		}
	}

	if (m_bufferCount >= m_maxBufferCount) {
		CM_ASSERT(0);
		return CM_EXCEED_SURFACE_AMOUNT;
	}

	UINT handle = 0;
	INT result = AllocateBuffer(size, type, handle, pCmOsResource, pSysMem);
	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}

	result =
	    CmBuffer_RT::Create(index, handle, size, pCmOsResource == NULL,
				this, type, pSysMem, pSurface1D);
	if (result != CM_SUCCESS) {
		FreeBuffer(handle);
		CM_ASSERT(0);
		return result;
	}

	m_SurfaceArray[index] = pSurface1D;
	UPDATE_PROFILE_FOR_1D_SURFACE(index, size, FALSE);

	return CM_SUCCESS;
}

INT CmSurfaceManager::AllocateBuffer(UINT size, CM_BUFFER_TYPE type,
				     UINT & handle,
				     CmOsResource * pCmOsResource,
				     void *pSysMem)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	GENOS_STATUS genos_status = GENOS_STATUS_SUCCESS;

	PCM_CONTEXT pCmData = (PCM_CONTEXT) m_pCmDevice->GetAccelData();

	handle = 0;
	CM_HAL_BUFFER_PARAM inParam;
	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_BUFFER_PARAM));
	inParam.iSize = size;

	inParam.type = type;

	if (pCmOsResource) {
		inParam.pCmOsResource = pCmOsResource;
		inParam.isAllocatedbyCmrtUmd = FALSE;
	} else {
		inParam.pCmOsResource = NULL;
		inParam.isAllocatedbyCmrtUmd = TRUE;
	}
	if (pSysMem) {
		inParam.pData = pSysMem;
	}

	genos_status =
	    pCmData->pCmHalState->pfnAllocateBuffer(pCmData->pCmHalState,
						    &inParam);
	while (genos_status == GENOS_STATUS_NO_SPACE) {
		if (!TouchSurfaceInPoolForDestroy()) {
			CM_ASSERT(0);
			return CM_SURFACE_ALLOCATION_FAILURE;
		}
		genos_status =
		    pCmData->pCmHalState->pfnAllocateBuffer(pCmData->
							    pCmHalState,
							    &inParam);
	}
	GENOSSTATUS2CM_AND_CHECK(genos_status, hr);

	handle = inParam.dwHandle;

 finish:
	return hr;
}

INT CmSurfaceManager::FreeBuffer(UINT handle)
{
	CM_RETURN_CODE hr = CM_SUCCESS;

	PCM_CONTEXT pCmData = (PCM_CONTEXT) m_pCmDevice->GetAccelData();
	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->
				       pCmHalState->pfnFreeBuffer(pCmData->
								  pCmHalState,
								  (DWORD)
								  handle));

 finish:
	return hr;
}

INT CmSurfaceManager::UpdateBuffer(CmBuffer_RT * pSurface1D, UINT size)
{
	CM_RETURN_CODE hr = CM_SUCCESS;

	UINT handle = 0;
	pSurface1D->GetHandle(handle);
	pSurface1D->SetSize(size);

	PCM_CONTEXT pCmData = (PCM_CONTEXT) m_pCmDevice->GetAccelData();
	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->
				       pCmHalState->pfnUpdateBuffer(pCmData->
								    pCmHalState,
								    (DWORD)
								    handle,
								    size));

 finish:
	return hr;
}

INT CmSurfaceManager::UpdateSurface2D(CmSurface2D * pSurface2D, UINT width,
				      UINT height, CM_SURFACE_FORMAT format)
{
	CM_RETURN_CODE hr = CM_SUCCESS;

	UINT handle = 0;
	pSurface2D->GetHandle(handle);
	pSurface2D->SetSurfaceProperties(width, height, format);

	PCM_CONTEXT pCmData = (PCM_CONTEXT) m_pCmDevice->GetAccelData();

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->
				       pCmHalState->pfnUpdateSurface2D(pCmData->
								       pCmHalState,
								       (DWORD)
								       handle,
								       width,
								       height));

 finish:
	return hr;
}

INT CmSurfaceManager::CreateSurface2DUP(UINT width, UINT height,
					CM_SURFACE_FORMAT format, void *pSysMem,
					CmSurface2DUP * &pSurface2D)
{
	pSurface2D = NULL;
	UINT index = m_pCmDevice->ValidSurfaceIndexStart();

	if (GetFreeSurfaceIndex(index) != CM_SUCCESS) {
		return CM_EXCEED_SURFACE_AMOUNT;
	}

	if (m_2DUPSurfaceCount >= m_max2DUPSurfaceCount) {
		CM_ASSERT(0);
		return CM_EXCEED_SURFACE_AMOUNT;
	}

	UINT handle = 0;
	INT result =
	    AllocateSurface2DUP(width, height, format, pSysMem, handle);
	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}

	result =
	    CmSurface2DUP::Create(index, handle, width, height, format, this,
				  pSurface2D);
	if (result != CM_SUCCESS) {
		FreeSurface2DUP(handle);
		CM_ASSERT(0);
		return result;
	}

	m_SurfaceArray[index] = pSurface2D;
	m_2DUPSurfaceCount++;
	UINT sizeperpixel = 1;
	GetFormatSize(format, sizeperpixel);
	m_SurfaceSizes[index] = width * height * sizeperpixel;

	return CM_SUCCESS;
}

INT CmSurfaceManager::AllocateSurface2DUP(UINT width, UINT height,
					  CM_SURFACE_FORMAT format,
					  void *pSysMem, UINT & handle)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	GENOS_STATUS genos_status = GENOS_STATUS_SUCCESS;

	handle = 0;
	PCM_CONTEXT pCmData = (PCM_CONTEXT) m_pCmDevice->GetAccelData();

	CM_HAL_SURFACE2D_UP_PARAM inParam;
	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_SURFACE2D_UP_PARAM));
	inParam.iWidth = width;
	inParam.iHeight = height;
	inParam.format = CmFmtToGenHwFmt(format);
	inParam.pData = pSysMem;

	genos_status =
	    pCmData->pCmHalState->pfnAllocateSurface2DUP(pCmData->pCmHalState,
							 &inParam);
	while (genos_status == GENOS_STATUS_NO_SPACE) {
		if (!TouchSurfaceInPoolForDestroy()) {
			CM_ASSERT(0);
			return CM_SURFACE_ALLOCATION_FAILURE;
		}
		genos_status =
		    pCmData->pCmHalState->pfnAllocateSurface2DUP(pCmData->
								 pCmHalState,
								 &inParam);
	}
	GENOSSTATUS2CM_AND_CHECK(genos_status, hr);

	handle = inParam.dwHandle;

 finish:
	return hr;
}

INT CmSurfaceManager::FreeSurface2DUP(UINT handle)
{
	CM_RETURN_CODE hr = CM_SUCCESS;

	PCM_CONTEXT pCmData = (PCM_CONTEXT) m_pCmDevice->GetAccelData();

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->
				       pCmHalState->pfnFreeSurface2DUP(pCmData->
								       pCmHalState,
								       (DWORD)
								       handle));

 finish:
	return hr;
}

INT CmSurfaceManager::CreateSurface2D(UINT width, UINT height, UINT pitch,
				      BOOL bIsCmCreated,
				      CM_SURFACE_FORMAT format,
				      CmSurface2D * &pSurface2D)
{
	UINT handle = 0;
	UINT index = m_pCmDevice->ValidSurfaceIndexStart();
	INT result = 0;

	pSurface2D = NULL;

	result = Surface2DSanityCheck(width, height, format);
	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}

	if (bIsCmCreated) {
		BOOL useNewSurface = TRUE;

		if (AllocateSurfaceIndex
		    (width, height, 0, format, index, useNewSurface,
		     NULL) != CM_SUCCESS) {
			return CM_EXCEED_SURFACE_AMOUNT;
		}

		if (!useNewSurface) {
			CmSurface *pSurface = m_SurfaceArray[index];
			if (pSurface
			    && (pSurface->Type() ==
				CM_ENUM_CLASS_TYPE_CMSURFACE2D)) {
				pSurface2D =
				    static_cast < CmSurface2D * >(pSurface);
			} else {
				return CM_FAILURE;
			}

			UpdateSurface2D(pSurface2D, width, height, format);
			UPDATE_PROFILE_FOR_2D_SURFACE(index, width, height,
						      format, TRUE);

			return CM_SUCCESS;
		}
	} else {
		if (GetFreeSurfaceIndex(index) != CM_SUCCESS) {
			return CM_EXCEED_SURFACE_AMOUNT;
		}
	}

	if (m_2DSurfaceCount >= m_max2DSurfaceCount) {
		CM_ASSERT(0);
		return CM_EXCEED_SURFACE_AMOUNT;
	}

	result = AllocateSurface2D(width, height, format, handle, pitch);
	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}

	result =
	    CmSurface2D::Create(index, handle, width, height, pitch, format,
				TRUE, this, pSurface2D);
	if (result != CM_SUCCESS) {
		FreeSurface2D(handle);
		CM_ASSERT(0);
		return result;
	}

	m_SurfaceArray[index] = pSurface2D;
	UPDATE_PROFILE_FOR_2D_SURFACE(index, width, height, format, FALSE);

	return CM_SUCCESS;
}

INT CmSurfaceManager::CreateSurface2D(CmOsResource * pCmOsResource,
				      BOOL bIsCmCreated,
				      CmSurface2D * &pSurface2D)
{
	UINT handle = 0;
	UINT index = m_pCmDevice->ValidSurfaceIndexStart();
	INT result = 0;
	UINT width = 0;
	UINT height = 0;
	UINT pitch = 0;
	CM_SURFACE_FORMAT format = CM_SURFACE_FORMAT_UNKNOWN;

	if (pCmOsResource == NULL) {
		return CM_INVALID_GENOS_RESOURCE_HANDLE;
	}

	pSurface2D = NULL;

	result = GetSurfaceInfo(pCmOsResource, width, height, pitch, format);
	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}
	result = Surface2DSanityCheck(width, height, format);
	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}
	if (GetFreeSurfaceIndex(index) != CM_SUCCESS) {
		CM_ASSERT(0);
		return CM_EXCEED_SURFACE_AMOUNT;
	}

	if (m_2DSurfaceCount >= m_max2DSurfaceCount) {
		CM_ASSERT(0);
		return CM_EXCEED_SURFACE_AMOUNT;
	}
	result =
	    AllocateSurface2D(width, height, format, pCmOsResource, handle);
	if (result != CM_SUCCESS) {
		CM_ASSERT(0);
		return result;
	}

	result =
	    CmSurface2D::Create(index, handle, width, height, pitch, format,
				bIsCmCreated, this, pSurface2D);
	if (result != CM_SUCCESS) {
		FreeSurface2D(handle);
		CM_ASSERT(0);
		return result;
	}

	m_SurfaceArray[index] = pSurface2D;
	UPDATE_PROFILE_FOR_2D_SURFACE(index, width, height, format, FALSE);

	return CM_SUCCESS;
}

INT CmSurfaceManager::AllocateSurface2D(UINT width, UINT height,
					CM_SURFACE_FORMAT format, UINT & handle,
					UINT & pitch)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	GENOS_STATUS genos_status = GENOS_STATUS_SUCCESS;
	PCM_CONTEXT pCmData = (PCM_CONTEXT) m_pCmDevice->GetAccelData();

	CM_HAL_SURFACE2D_PARAM inParam;
	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_SURFACE2D_PARAM));
	inParam.iWidth = width;
	inParam.iHeight = height;
	inParam.format = CmFmtToGenHwFmt(format);
	inParam.pData = NULL;
	inParam.isAllocatedbyCmrtUmd = TRUE;

	genos_status =
	    pCmData->pCmHalState->pfnAllocateSurface2D(pCmData->pCmHalState,
						       &inParam);
	while (genos_status == GENOS_STATUS_NO_SPACE) {
		if (!TouchSurfaceInPoolForDestroy()) {
			CM_ASSERT(0);
			return CM_SURFACE_ALLOCATION_FAILURE;
		}
		genos_status =
		    pCmData->pCmHalState->pfnAllocateSurface2D(pCmData->
							       pCmHalState,
							       &inParam);
	}
	GENOSSTATUS2CM_AND_CHECK(genos_status, hr);

	handle = inParam.dwHandle;

	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->
				       pCmHalState->pfnGetSurface2DTileYPitch
				       (pCmData->pCmHalState, &inParam));

	pitch = inParam.iPitch;

 finish:
	return hr;
}

INT CmSurfaceManager::AllocateSurface2D(UINT width, UINT height,
					CM_SURFACE_FORMAT format,
					CmOsResource * pCmOsResource,
					UINT & handle)
{
	CM_RETURN_CODE hr = CM_SUCCESS;
	GENOS_STATUS genos_status = GENOS_STATUS_SUCCESS;

	PCM_CONTEXT pCmData = (PCM_CONTEXT) m_pCmDevice->GetAccelData();

	CM_HAL_SURFACE2D_PARAM inParam;
	CmSafeMemSet(&inParam, 0, sizeof(CM_HAL_SURFACE2D_PARAM));
	inParam.iWidth = width;
	inParam.iHeight = height;
	inParam.format = CmFmtToGenHwFmt(format);
	inParam.pCmOsResource = pCmOsResource;
	inParam.isAllocatedbyCmrtUmd = FALSE;

	genos_status =
	    pCmData->pCmHalState->pfnAllocateSurface2D(pCmData->pCmHalState,
						       &inParam);
	while (genos_status == GENOS_STATUS_NO_SPACE) {
		if (!TouchSurfaceInPoolForDestroy()) {
			CM_ASSERT(0);
			return CM_SURFACE_ALLOCATION_FAILURE;
		}
		genos_status =
		    pCmData->pCmHalState->pfnAllocateSurface2D(pCmData->
							       pCmHalState,
							       &inParam);
	}
	GENOSSTATUS2CM_AND_CHECK(genos_status, hr);

	handle = inParam.dwHandle;

 finish:
	return hr;
}

INT CmSurfaceManager::FreeSurface2D(UINT handle)
{
	CM_RETURN_CODE hr = CM_SUCCESS;

	PCM_CONTEXT pCmData = (PCM_CONTEXT) m_pCmDevice->GetAccelData();
	CHK_GENOSSTATUS_RETURN_CMERROR(pCmData->
				       pCmHalState->pfnFreeSurface2D(pCmData->
								     pCmHalState,
								     handle));

 finish:
	return hr;
}

INT CmSurfaceManager::DestroySurface(CmBuffer_RT * &pSurface1D,
				     SURFACE_DESTROY_KIND destroyKind)
{
	UINT handle = 0;
	INT result = CM_SUCCESS;

	if (!pSurface1D) {
		return CM_FAILURE;
	}
	SurfaceIndex *pIndex = NULL;
	pSurface1D->GetIndex(pIndex);
	CM_ASSERT(pIndex);
	UINT index = pIndex->get_data();
	CM_ASSERT(m_SurfaceArray[index] == pSurface1D);

	if (destroyKind == FORCE_DESTROY) {
		pSurface1D->WaitForReferenceFree();
	} else {
		if (m_pCmDevice->IsSurfaceReuseEnabled()
		    && !pSurface1D->IsUpSurface()) {
			result =
			    UPDATE_STATE_FOR_REUSE_DESTROY(destroyKind, index);
		} else {
			result =
			    UPDATE_STATE_FOR_DELAYED_DESTROY(destroyKind,
							     index);
		}

		if (result != CM_SUCCESS) {
			return result;
		}
	}

	result = pSurface1D->GetHandle(handle);
	if (result != CM_SUCCESS) {
		return result;
	}

	result = FreeBuffer(handle);
	if (result != CM_SUCCESS) {
		return result;
	}

	CmSurface *pSurface = pSurface1D;
	CmSurface::Destroy(pSurface);

	UPDATE_STATE_FOR_REAL_DESTROY(index, CM_ENUM_CLASS_TYPE_CMBUFFER_RT);

	return CM_SUCCESS;
}

INT CmSurfaceManager::DestroySurface(CmSurface2DUP * &pSurface2D,
				     SURFACE_DESTROY_KIND destroyKind)
{
	UINT handle = 0;
	INT result = CM_SUCCESS;

	if (!pSurface2D) {
		return CM_FAILURE;
	}

	SurfaceIndex *pIndex = NULL;
	pSurface2D->GetIndex(pIndex);
	CM_ASSERT(pIndex);
	UINT index = pIndex->get_data();

	CM_ASSERT(m_SurfaceArray[index] == pSurface2D);

	if (destroyKind == FORCE_DESTROY) {
		pSurface2D->WaitForReferenceFree();
	} else {
		result = UPDATE_STATE_FOR_DELAYED_DESTROY(destroyKind, index);
		if (result != CM_SUCCESS) {
			return result;
		}
	}

	result = pSurface2D->GetHandle(handle);
	if (result != CM_SUCCESS) {
		return result;
	}

	result = FreeSurface2DUP(handle);
	if (result != CM_SUCCESS) {
		return result;
	}

	CmSurface *pSurface = pSurface2D;
	CmSurface::Destroy(pSurface);

	UPDATE_STATE_FOR_REAL_DESTROY(index, CM_ENUM_CLASS_TYPE_CMSURFACE2DUP);

	return CM_SUCCESS;
}

INT CmSurfaceManager::DestroySurface(CmSurface2D * &pSurface2D,
				     SURFACE_DESTROY_KIND destroyKind)
{
	UINT handle = 0;
	SurfaceIndex *pIndex = NULL;
	pSurface2D->GetIndex(pIndex);
	CM_ASSERT(pIndex);
	UINT index = pIndex->get_data();
	INT result = CM_SUCCESS;

	CM_ASSERT(m_SurfaceArray[index] == pSurface2D);

	if (destroyKind == FORCE_DESTROY) {
		pSurface2D->WaitForReferenceFree();
	} else {
		if (m_pCmDevice->IsSurfaceReuseEnabled()
		    && pSurface2D->IsCmCreated()) {
			result =
			    UPDATE_STATE_FOR_REUSE_DESTROY(destroyKind, index);
		} else {
			result =
			    UPDATE_STATE_FOR_DELAYED_DESTROY(destroyKind,
							     index);
		}

		if (result != CM_SUCCESS) {
			return result;
		}
	}

	result = pSurface2D->GetHandle(handle);
	if (result != CM_SUCCESS) {
		return result;
	}

	result = FreeSurface2D(handle);
	if (result != CM_SUCCESS) {
		return result;
	}

	CmSurface *pSurface = pSurface2D;
	CmSurface::Destroy(pSurface);

	UPDATE_STATE_FOR_REAL_DESTROY(index, CM_ENUM_CLASS_TYPE_CMSURFACE2D);

	return CM_SUCCESS;
}

INT CmSurfaceManager::GetSurface(const UINT index, CmSurface * &pSurface)
{
	if (index < m_SurfaceArraySize) {
		pSurface = m_SurfaceArray[index];
		return CM_SUCCESS;
	} else {
		pSurface = NULL;
		return CM_FAILURE;
	}
}

INT CmSurfaceManager::GetCmDevice(CmDevice * &pCmDevice)
{
	pCmDevice = m_pCmDevice;
	return CM_SUCCESS;
}

INT CmSurfaceManager::GetPixelBytesAndHeight(UINT width, UINT height,
					     CM_SURFACE_FORMAT format,
					     UINT & sizePerPixel,
					     UINT & updatedHeight)
{
	updatedHeight = height;
	switch (format) {
	case CM_SURFACE_FORMAT_X8R8G8B8:
	case CM_SURFACE_FORMAT_A8R8G8B8:
	case CM_SURFACE_FORMAT_R32F:
		sizePerPixel = 4;
		break;

	case CM_SURFACE_FORMAT_V8U8:
	case CM_SURFACE_FORMAT_UYVY:
	case CM_SURFACE_FORMAT_YUY2:
	case CM_SURFACE_FORMAT_R16_UINT:
		sizePerPixel = 2;
		break;

	case CM_SURFACE_FORMAT_A8:
	case CM_SURFACE_FORMAT_P8:
	case CM_SURFACE_FORMAT_R8_UINT:
		sizePerPixel = 1;
		break;

	case CM_SURFACE_FORMAT_NV12:
		sizePerPixel = 1;
		updatedHeight += (updatedHeight + 1) / 2;
		break;

	case CM_SURFACE_FORMAT_411P:
	case CM_SURFACE_FORMAT_422H:
	case CM_SURFACE_FORMAT_444P:
		sizePerPixel = 1;
		updatedHeight = height * 3;
		break;

	case CM_SURFACE_FORMAT_IMC3:
		sizePerPixel = 1;
		updatedHeight = height * 2;
		break;

	case CM_SURFACE_FORMAT_422V:
		sizePerPixel = 1;
		updatedHeight = height * 2;
		break;

	case CM_SURFACE_FORMAT_YV12:
		sizePerPixel = 1;
		updatedHeight += updatedHeight / 2;
		break;

	default:
		CM_ASSERT(0);
		return CM_SURFACE_FORMAT_NOT_SUPPORTED;
	}

	return CM_SUCCESS;
}

INT CmSurfaceManager::Surface2DSanityCheck(UINT width, UINT height,
					   CM_SURFACE_FORMAT format)
{
	UINT MaxSurf2DWidth = CM_MAX_2D_SURF_WIDTH_IVB_PLUS;
	UINT MaxSurf2DHeight = CM_MAX_2D_SURF_HEIGHT_IVB_PLUS;

	if ((width < CM_MIN_SURF_WIDTH) || (width > MaxSurf2DWidth)) {
		CM_ASSERT(0);
		return CM_INVALID_WIDTH;
	}

	if ((height < CM_MIN_SURF_HEIGHT) || (height > MaxSurf2DHeight)) {
		CM_ASSERT(0);
		return CM_INVALID_HEIGHT;
	}

	switch (format) {
	case CM_SURFACE_FORMAT_X8R8G8B8:
	case CM_SURFACE_FORMAT_A8R8G8B8:
	case CM_SURFACE_FORMAT_R32F:
	case CM_SURFACE_FORMAT_A8:
	case CM_SURFACE_FORMAT_P8:
	case CM_SURFACE_FORMAT_V8U8:
	case CM_SURFACE_FORMAT_R16_UINT:
		break;

	case CM_SURFACE_FORMAT_R8_UINT:
		break;

	case CM_SURFACE_FORMAT_UYVY:
	case CM_SURFACE_FORMAT_YUY2:

		if (width & 0x1) {
			CM_ASSERT(0);
			return CM_INVALID_WIDTH;
		}
		break;

	case CM_SURFACE_FORMAT_NV12:
	case CM_SURFACE_FORMAT_YV12:
		if (width & 0x1) {
			CM_ASSERT(0);
			return CM_INVALID_WIDTH;
		}

		if (height & 0x1) {
			CM_ASSERT(0);
			return CM_INVALID_HEIGHT;
		}
		break;

	case CM_SURFACE_FORMAT_411P:
	case CM_SURFACE_FORMAT_IMC3:
	case CM_SURFACE_FORMAT_422H:
	case CM_SURFACE_FORMAT_422V:
	case CM_SURFACE_FORMAT_444P:
		if (width & 0x1) {
			CM_ASSERT(0);
			return CM_INVALID_WIDTH;
		}
		if (height & 0x1) {
			CM_ASSERT(0);
			return CM_INVALID_HEIGHT;
		}
		break;

	default:
		CM_ASSERT(0);
		return CM_SURFACE_FORMAT_NOT_SUPPORTED;
	}

	return CM_SUCCESS;
}

GENOS_FORMAT CmSurfaceManager::CmFmtToGenHwFmt(CM_SURFACE_FORMAT format)
{
	switch (format) {
	case CM_SURFACE_FORMAT_A8R8G8B8:
		return Format_A8R8G8B8;
	case CM_SURFACE_FORMAT_X8R8G8B8:
		return Format_X8R8G8B8;
	case CM_SURFACE_FORMAT_R32F:
		return Format_R32F;
	case CM_SURFACE_FORMAT_YUY2:
		return Format_YUY2;
	case CM_SURFACE_FORMAT_P8:
		return Format_P8;
	case CM_SURFACE_FORMAT_A8:
		return Format_A8;
	case CM_SURFACE_FORMAT_UYVY:
		return Format_UYVY;
	case CM_SURFACE_FORMAT_NV12:
		return Format_NV12;
	case CM_SURFACE_FORMAT_V8U8:
		return Format_V8U8;
	case CM_SURFACE_FORMAT_R8_UINT:
		return Format_R8U;
	case CM_SURFACE_FORMAT_R16_UINT:
		return Format_R16U;
	case CM_SURFACE_FORMAT_411P:
		return Format_411P;
	case CM_SURFACE_FORMAT_422H:
		return Format_422H;
	case CM_SURFACE_FORMAT_444P:
		return Format_444P;
	case CM_SURFACE_FORMAT_IMC3:
		return Format_IMC3;
	case CM_SURFACE_FORMAT_422V:
		return Format_422V;
	case CM_SURFACE_FORMAT_YV12:
		return Format_YV12;
	default:
		return Format_Invalid;
	}
}

UINT CmSurfaceManager::GetSurfacePoolSize()
{
	return m_SurfaceArraySize;
}

UINT CmSurfaceManager::GetSurfaceState(INT * &pSurfState)
{
	pSurfState = m_SurfaceState;

	return CM_SUCCESS;
}

INT CmSurfaceManager::GetSurfaceIdInPool(INT iIndex)
{
	return m_SurfaceDestroyID[iIndex];
}

INT CmSurfaceManager::GetSurfaceInfo(CmOsResource * pCmOsResource, UINT & width,
				     UINT & height, UINT & pitch,
				     CM_SURFACE_FORMAT & format)
{
	if (pCmOsResource == NULL) {
		return CM_INVALID_GENOS_RESOURCE_HANDLE;
	}

	width = pCmOsResource->aligned_width;
	height = pCmOsResource->aligned_height;
	format = pCmOsResource->format;
	pitch = pCmOsResource->pitch;
	return CM_SUCCESS;
}
