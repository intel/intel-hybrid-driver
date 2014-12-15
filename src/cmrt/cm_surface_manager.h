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
#include "cm_device.h"

typedef enum _GENOS_FORMAT GENOS_FORMAT;

class CmDevice;
class CmSurface;

class CmSurfaceManager {
 public:
	static INT Create(CmDevice * pCmDevice,
			  CM_HAL_MAX_VALUES HalMaxValues,
			  CM_HAL_MAX_VALUES_EX HalMaxValuesEx,
			  CmSurfaceManager * &pManager);

	static INT Destroy(CmSurfaceManager * &pManager);

	INT CreateBuffer(UINT size, CM_BUFFER_TYPE type,
			 CmBuffer_RT * &pSurface1D,
			 CmOsResource * pCmOsResource, void *&pSysMem);
	INT DestroySurface(CmBuffer_RT * &pSurface,
			   SURFACE_DESTROY_KIND destroyKind);

	INT CreateSurface2DUP(UINT width, UINT height, CM_SURFACE_FORMAT format,
			      void *pSysMem, CmSurface2DUP * &pSurface2D);
	INT DestroySurface(CmSurface2DUP * &pSurface,
			   SURFACE_DESTROY_KIND destroyKind);

	INT CreateSurface2D(UINT width, UINT height, UINT pitch,
			    BOOL bIsCmCreated, CM_SURFACE_FORMAT format,
			    CmSurface2D * &pSurface);
	INT CreateSurface2D(CmOsResource * pCmOsResource, BOOL bIsCmCreated,
			    CmSurface2D * &pSurface2D);

	INT DestroySurface(CmSurface2D * &pSurface,
			   SURFACE_DESTROY_KIND destroyKind);

	INT GetSurface(const UINT index, CmSurface * &pSurface);
	INT GetCmDevice(CmDevice * &pCmDevice);

	INT GetPixelBytesAndHeight(UINT width, UINT height,
				   CM_SURFACE_FORMAT format,
				   UINT & sizePerPixel, UINT & updatedHeight);
	INT Surface2DSanityCheck(UINT width, UINT height,
				 CM_SURFACE_FORMAT format);

	UINT GetSurfacePoolSize();
	UINT GetSurfaceState(INT * &pSurfState);
	INT IncreaseSurfaceUsage(UINT index);
	INT DecreaseSurfaceUsage(UINT index);
	INT DestroySurfaceInPool(UINT & freeSurfNum,
				 SURFACE_DESTROY_KIND destroyKind);
	INT TouchSurfaceInPoolForDestroy();
	INT GetFreeSurfaceIndexFromPool(UINT & freeIndex);
	INT GetFreeSurfaceIndex(UINT & index);
	INT GetReuseSurfaceIndex(UINT width, UINT height, UINT depth,
				 CM_SURFACE_FORMAT format);
	INT AllocateSurfaceIndex(UINT width, UINT height, UINT depth,
				 CM_SURFACE_FORMAT format, UINT & index,
				 BOOL & useNewSurface, void *pSysMem);

	INT GetSurface2dInPool(UINT width, UINT height,
			       CM_SURFACE_FORMAT format,
			       CmSurface2D * &pSurf2D);

	INT GetSurfaceIdInPool(INT iIndex);

	INT DestroySurfaceArrayElement(UINT index);
	inline INT GetMemorySizeOfSurfaces();

	GENOS_FORMAT CmFmtToGenHwFmt(CM_SURFACE_FORMAT format);

	INT UPDATE_STATE_FOR_REUSE_DESTROY(SURFACE_DESTROY_KIND destroyKind,
					   UINT i);
	INT UPDATE_STATE_FOR_DELAYED_DESTROY(SURFACE_DESTROY_KIND destroyKind,
					     UINT i);
	INT UPDATE_STATE_FOR_SURFACE_REUSE(UINT i);
	INT UPDATE_STATE_FOR_REAL_DESTROY(UINT i, CM_ENUM_CLASS_TYPE kind);
	INT UPDATE_PROFILE_FOR_2D_SURFACE(UINT index, UINT width, UINT height,
					  CM_SURFACE_FORMAT format, BOOL reuse);
	INT UPDATE_PROFILE_FOR_1D_SURFACE(UINT index, UINT size, BOOL reuse);

 protected:
	 CmSurfaceManager(CmDevice * pCmDevice);
	 CmSurfaceManager();

	~CmSurfaceManager(void);
	INT Initialize(CM_HAL_MAX_VALUES HalMaxValues,
		       CM_HAL_MAX_VALUES_EX HalMaxValuesEx);

	INT AllocateBuffer(UINT size, CM_BUFFER_TYPE type, UINT & handle,
			   CmOsResource * pCmOsResource, void *pSysMem = NULL);
	INT FreeBuffer(UINT handle);

	INT AllocateSurface2DUP(UINT width, UINT height,
				CM_SURFACE_FORMAT format, void *pSysMem,
				UINT & handle);
	INT FreeSurface2DUP(UINT handle);

	INT AllocateSurface2D(UINT width, UINT height, CM_SURFACE_FORMAT format,
			      UINT & handle, UINT & pitch);
	INT AllocateSurface2D(UINT width, UINT height, CM_SURFACE_FORMAT format,
			      CmOsResource * pCmOsResource, UINT & handle);

	INT FreeSurface2D(UINT handle);

	INT UpdateBuffer(CmBuffer_RT * pSurface1D, UINT size);
	INT UpdateSurface2D(CmSurface2D * pSurface2D, UINT width, UINT height,
			    CM_SURFACE_FORMAT format);
	INT GetFormatSize(CM_SURFACE_FORMAT format, UINT & sizeperpixel);
	INT GetSurfaceInfo(CmOsResource * pCmOsResource, UINT & width,
			   UINT & height, UINT & pitch,
			   CM_SURFACE_FORMAT & format);

 public:
	static const UINT MAX_DEVICE_FOR_SAME_SURF = 4;

 protected:
	 CmDevice * m_pCmDevice;

	UINT m_SurfaceArraySize;

	CmSurface **m_SurfaceArray;
	INT *m_SurfaceState;
	BOOL *m_SurfaceCached;
	BOOL *m_SurfaceReleased;
	INT *m_SurfaceDestroyID;
	INT *m_SurfaceSizes;

	UINT m_maxBufferCount;
	UINT m_bufferCount;

	UINT m_max2DSurfaceCount;
	UINT m_2DSurfaceCount;

	UINT m_max2DUPSurfaceCount;
	UINT m_2DUPSurfaceCount;

	UINT m_bufferAllCount;
	UINT m_2DSurfaceAllCount;

	UINT m_bufferAllSize;
	UINT m_2DSurfaceAllSize;

	UINT m_bufferReuseCount;
	UINT m_2DSurfaceReuseCount;

	UINT m_bufferReuseSize;
	UINT m_2DSurfaceReuseSize;

	UINT m_GCTriggerTimes;
	UINT m_GCCollected1DSize;
	UINT m_GCCollected2DSize;
 private:
	 CmSurfaceManager(const CmSurfaceManager & other);
	 CmSurfaceManager & operator=(const CmSurfaceManager & other);
};
