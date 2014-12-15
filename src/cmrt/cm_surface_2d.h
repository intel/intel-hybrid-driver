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
#include "cm_surface.h"

class CmEvent;
class CmSurfaceManager;

class CmSurface2D:public CmSurface {
 public:
	static INT Create(UINT index, UINT handle, UINT width, UINT height,
			  UINT pitch, CM_SURFACE_FORMAT format,
			  BOOL isCmCreated, CmSurfaceManager * pSurfaceManager,
			  CmSurface2D * &pSurface);

	CM_RT_API INT ReadSurface(unsigned char *pSysMem, CmEvent * pEvent,
				  UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL);
	CM_RT_API INT WriteSurface(const unsigned char *pSysMem,
				   CmEvent * pEvent, UINT64 sysMemSize =
				   0xFFFFFFFFFFFFFFFFULL);
	CM_RT_API INT ReadSurfaceStride(unsigned char *pSysMem,
					CmEvent * pEvent, const UINT stride,
					UINT64 sysMemSize =
					0xFFFFFFFFFFFFFFFFULL);
	CM_RT_API INT WriteSurfaceStride(const unsigned char *pSysMem,
					 CmEvent * pEvent, const UINT stride,
					 UINT64 sysMemSize =
					 0xFFFFFFFFFFFFFFFFULL);

	CM_RT_API INT GetIndex(SurfaceIndex * &pIndex);
	CM_RT_API INT GetSurfaceDesc(UINT & width, UINT & height,
				     CM_SURFACE_FORMAT & format,
				     UINT & sizeperpixel);
	CM_RT_API INT InitSurface(const DWORD initValue, CmEvent * pEvent);
	CM_RT_API INT SetSurfaceStateDimensions(UINT iWidth, UINT iHeight,
						SurfaceIndex * pSurfIndex =
						NULL);
	CM_RT_API INT ReadSurfaceFullStride(unsigned char *pSysMem,
					    CmEvent * pEvent,
					    const UINT iWidthStride,
					    const UINT iHeightStride,
					    UINT64 sysMemSize);
	CM_RT_API INT WriteSurfaceFullStride(const unsigned char *pSysMem,
					     CmEvent * pEvent,
					     const UINT iWidthStride,
					     const UINT iHeightStride,
					     UINT64 sysMemSize);

	INT GetIndexFor2D(UINT & index);
	INT GetHandle(UINT & handle);
	INT SetSurfaceProperties(UINT width, UINT height,
				 CM_SURFACE_FORMAT format);
	CM_ENUM_CLASS_TYPE Type() const {
		return CM_ENUM_CLASS_TYPE_CMSURFACE2D;
	};
	CM_RT_API INT SetMemoryObjectControl(MEMORY_OBJECT_CONTROL mem_ctrl,
					     MEMORY_TYPE mem_type, UINT age);

	INT GetSubResourceIndex(UINT & nIndex);
	INT SetSubResourceIndex(UINT nIndex);
	INT SetReadSyncFlag();

 protected:
	 CmSurface2D(UINT handle, UINT width, UINT height, UINT pitch,
		     CM_SURFACE_FORMAT format,
		     CmSurfaceManager * pSurfaceManager, BOOL isCmCreated);
	~CmSurface2D(void);

	INT Initialize(UINT index);

	UINT m_Width;
	UINT m_Height;
	UINT m_Handle;
	UINT m_Pitch;
	CM_SURFACE_FORMAT m_Format;

	UINT m_SubResourceIndex;
};
