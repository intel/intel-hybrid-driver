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

class CmSurfaceManager;
class CmEvent;

class CmSurface {
 public:
	static INT Destroy(CmSurface * &pSurface);
	BOOL IsCmCreated(void) {
		return m_IsCmCreated;
	}
	virtual CM_ENUM_CLASS_TYPE Type() const = 0;
	INT TouchDeviceQueue();
	INT WaitForReferenceFree();
	INT SetMemoryObjectControl(MEMORY_OBJECT_CONTROL mem_ctrl,
				   MEMORY_TYPE mem_type, UINT age);
	VOID GetMemoryObjectCtrl(CM_SURFACE_MEM_OBJ_CTRL * mem_ctrl);

 protected:
	CmSurface(CmSurfaceManager * Mgr, BOOL IsCmCreated);
	virtual ~ CmSurface(void);
	INT Initialize(UINT index);

	INT FlushDeviceQueue(CmEvent * pEvent);
	BOOL MemoryObjectCtrlPolicyCheck(MEMORY_OBJECT_CONTROL mem_ctrl,
					 MEMORY_TYPE mem_type, UINT age);

	SurfaceIndex *m_pIndex;

	CmSurfaceManager *m_SurfaceMgr;

	UINT m_IsCmCreated;

	CM_SURFACE_MEM_OBJ_CTRL m_MemObjCtrl;

 private:
	CmSurface(const CmSurface & other);
	CmSurface & operator=(const CmSurface & other);
};
