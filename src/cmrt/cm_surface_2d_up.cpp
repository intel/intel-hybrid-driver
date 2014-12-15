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

#include "cm_surface_2d_up.h"
#include "cm_device.h"
#include "cm_surface_manager.h"

INT CmSurface2DUP::Create(UINT index, UINT handle, UINT width, UINT height,
			  CM_SURFACE_FORMAT format,
			  CmSurfaceManager * pSurfaceManager,
			  CmSurface2DUP * &pSurface)
{
	INT result = CM_SUCCESS;

	pSurface =
	    new(std::nothrow) CmSurface2DUP(handle, width, height, format,
					    pSurfaceManager);
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

 CmSurface2DUP::CmSurface2DUP(UINT handle, UINT width, UINT height, CM_SURFACE_FORMAT format, CmSurfaceManager * pSurfaceManager):
CmSurface(pSurfaceManager, TRUE),
m_Handle(handle), m_Width(width), m_Height(height), m_Format(format)
{
	CmSurface::SetMemoryObjectControl(MEMORY_OBJECT_CONTROL_UNKNOW,
					  CM_USE_PTE, 0);
}

CmSurface2DUP::~CmSurface2DUP(void)
{

}

INT CmSurface2DUP::Initialize(UINT index)
{
	return CmSurface::Initialize(index);

}

INT CmSurface2DUP::GetHandle(UINT & handle)
{
	handle = m_Handle;
	return CM_SUCCESS;
}

CM_RT_API INT CmSurface2DUP::GetIndex(SurfaceIndex * &pIndex)
{
	pIndex = m_pIndex;
	return CM_SUCCESS;
}

CM_RT_API INT
    CmSurface2DUP::SetMemoryObjectControl(MEMORY_OBJECT_CONTROL mem_ctrl,
					  MEMORY_TYPE mem_type, UINT age)
{
	CmSurface::SetMemoryObjectControl(mem_ctrl, mem_type, age);

	return CM_SUCCESS;
}

CM_RT_API INT
    CmSurface2DUP::GetSurfaceDesc(UINT & width, UINT & height,
				  CM_SURFACE_FORMAT & format,
				  UINT & sizeperpixel)
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
