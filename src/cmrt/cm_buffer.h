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
class CmSurface;
class CmEvent;

class CmBufferUP {
 public:
	CM_RT_API virtual INT GetIndex(SurfaceIndex * &pIndex) = 0;
	CM_RT_API virtual CM_ENUM_CLASS_TYPE Type() const = 0;
	CM_RT_API virtual INT SetMemoryObjectControl(MEMORY_OBJECT_CONTROL
						     mem_ctrl,
						     MEMORY_TYPE mem_type,
						     UINT age) = 0;

	 virtual ~ CmBufferUP() {
	};
};

class CmBuffer {
 public:

	CM_RT_API virtual INT GetIndex(SurfaceIndex * &pIndex) = 0;
	CM_RT_API virtual INT ReadSurface(unsigned char *pSysMem,
					  CmEvent * pEvent, UINT64 sysMemSize =
					  0xFFFFFFFFFFFFFFFFULL) = 0;
	CM_RT_API virtual INT WriteSurface(const unsigned char *pSysMem,
					   CmEvent * pEvent, UINT64 sysMemSize =
					   0xFFFFFFFFFFFFFFFFULL) = 0;

	CM_RT_API virtual CM_ENUM_CLASS_TYPE Type() const = 0;

	 virtual ~ CmBuffer() {
	};

	CM_RT_API virtual INT InitSurface(const DWORD initValue,
					  CmEvent * pEvent) = 0;
	CM_RT_API virtual INT SetMemoryObjectControl(MEMORY_OBJECT_CONTROL
						     mem_ctrl,
						     MEMORY_TYPE mem_type,
						     UINT age) = 0;
};

class CmBuffer_RT:public CmBuffer, public CmBufferUP, public CmSurface {
 public:
	static INT Create(UINT index, UINT handle, UINT size, BOOL bIsCmCreated,
			  CmSurfaceManager * pSurfaceManager, UINT uiBufferType,
			  void *pSysMem, CmBuffer_RT * &pSurface);
	CM_RT_API INT ReadSurface(unsigned char *pSysMem, CmEvent * pEvent,
				  UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL);
	CM_RT_API INT WriteSurface(const unsigned char *pSysMem,
				   CmEvent * pEvent, UINT64 sysMemSize =
				   0xFFFFFFFFFFFFFFFFULL);
	CM_RT_API INT InitSurface(const DWORD initValue, CmEvent * pEvent);

	CM_RT_API INT GetIndex(SurfaceIndex * &pIndex);
	INT GetHandle(UINT & handle);
	CM_RT_API CM_ENUM_CLASS_TYPE Type() const {
		return CM_ENUM_CLASS_TYPE_CMBUFFER_RT;
	};
	CM_RT_API INT SetMemoryObjectControl(MEMORY_OBJECT_CONTROL mem_ctrl,
					     MEMORY_TYPE mem_type, UINT age);
	CM_RT_API INT GetAddress(VOID * &pAddr);

	INT GetSize(UINT & size);
	INT SetSize(UINT size);
	BOOL IsUpSurface();

	UINT GetBufferType() {
		return m_uiBufferType;
 } protected:
	 CmBuffer_RT(UINT handle, UINT size, BOOL bIsCmCreated,
		     CmSurfaceManager * pSurfaceManager, UINT uiBufferType,
		     VOID * pSysMem);
	~CmBuffer_RT(void);

	INT Initialize(UINT index);

	UINT m_Handle;
	UINT m_Size;
	UINT m_uiBufferType;
	PVOID m_pSysMem;
};
