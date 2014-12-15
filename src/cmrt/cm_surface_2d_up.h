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

class CmSurface2DUP:public CmSurface {
 public:
	static INT Create(UINT index, UINT handle, UINT width, UINT height,
			  CM_SURFACE_FORMAT format,
			  CmSurfaceManager * pSurfaceManager,
			  CmSurface2DUP * &pSurface);
	CM_RT_API INT GetIndex(SurfaceIndex * &pIndex);
	INT GetHandle(UINT & handle);

	CM_ENUM_CLASS_TYPE Type() const {
		return CM_ENUM_CLASS_TYPE_CMSURFACE2DUP;
	};
	CM_RT_API INT SetMemoryObjectControl(MEMORY_OBJECT_CONTROL mem_ctrl,
					     MEMORY_TYPE mem_type, UINT age);

	CM_RT_API INT GetSurfaceDesc(UINT & width, UINT & height,
				     CM_SURFACE_FORMAT & format,
				     UINT & sizeperpixel);

 protected:
	 CmSurface2DUP(UINT handle, UINT width, UINT height,
		       CM_SURFACE_FORMAT format,
		       CmSurfaceManager * pSurfaceManager);
	~CmSurface2DUP(void);

	INT Initialize(UINT index);

	UINT m_Handle;
	UINT m_Width;
	UINT m_Height;
	CM_SURFACE_FORMAT m_Format;
};
