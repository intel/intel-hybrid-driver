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
#include "cm_surface.h"
#include "cm_surface_manager.h"
#include "cm_queue.h"
#include "cm_device.h"
#include "cm_event.h"
#include <sys/time.h>

INT CmSurface::Destroy(CmSurface * &pSurface)
{
	CmSafeDelete(pSurface);

	return CM_SUCCESS;
}

 CmSurface::CmSurface(CmSurfaceManager * surfMgr, BOOL isCmCreated):
m_pIndex(NULL), m_SurfaceMgr(surfMgr), m_IsCmCreated(isCmCreated)
{

}

CmSurface::~CmSurface(void)
{
	if (m_pIndex) {
		delete m_pIndex;
	}
}

INT CmSurface::Initialize(UINT index)
{
	m_pIndex = new(std::nothrow) SurfaceIndex(index);
	if (m_pIndex) {
		return CM_SUCCESS;
	} else {
		return CM_OUT_OF_HOST_MEMORY;
	}
}

INT CmSurface::FlushDeviceQueue(CmEvent * pEvent)
{
	if (pEvent == NULL) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	CmDevice *pCmDev = NULL;
	m_SurfaceMgr->GetCmDevice(pCmDev);
	CM_ASSERT(pCmDev);

	CmQueue *pCmQueue = NULL;
	pCmDev->GetQueue(pCmQueue);
	pCmQueue->AcquireQueueLock();

	UINT num_tasks;
	pCmQueue->GetTaskCount(num_tasks);
	struct timeval start;
	gettimeofday(&start, NULL);
	UINT64 timeout_usec;
	timeout_usec = CM_MAX_TIMEOUT * num_tasks * 1000000;

	CM_STATUS status;
	pEvent->GetStatus(status);
	while (status == CM_STATUS_QUEUED) {
		struct timeval current;
		gettimeofday(&current, NULL);
		UINT64 timeuse_usec;
		timeuse_usec =
		    1000000 * (current.tv_sec - start.tv_sec) +
		    current.tv_usec - start.tv_usec;
		if (timeuse_usec > timeout_usec)
			return CM_EXCEED_MAX_TIMEOUT;

		pEvent->GetStatus(status);
	}

	pCmQueue->ReleaseQueueLock();
	return CM_SUCCESS;
}

INT CmSurface::TouchDeviceQueue()
{
	CmDevice *pCmDev = NULL;
	CmQueue *pCmQueue = NULL;

	m_SurfaceMgr->GetCmDevice(pCmDev);
	CM_ASSERT(pCmDev);
	pCmDev->GetQueue(pCmQueue);

	return pCmQueue->TouchFlushedTasks();
}

INT CmSurface::WaitForReferenceFree()
{
	INT *pSurfState = NULL;
	m_SurfaceMgr->GetSurfaceState(pSurfState);
	while (pSurfState[m_pIndex->get_data()]) {
		if (FAILED(TouchDeviceQueue())) {
			CM_ASSERT(0);
			return CM_FAILURE;
		};
	};

	return CM_SUCCESS;
}

BOOL CmSurface::MemoryObjectCtrlPolicyCheck(MEMORY_OBJECT_CONTROL mem_ctrl,
					    MEMORY_TYPE mem_type, UINT age)
{
	CmDevice *pCmDevice = NULL;
	UINT platform = IGFX_UNKNOWN_CORE;

	if (mem_ctrl == MEMORY_OBJECT_CONTROL_UNKNOW) {
		return TRUE;
	}

	m_SurfaceMgr->GetCmDevice(pCmDevice);
	pCmDevice->GetGenPlatform(platform);

	switch (platform) {

	case IGFX_GEN7_5_CORE:
		if (mem_ctrl > MEMORY_OBJECT_CONTROL_L3_ELLC)
			return FALSE;
		break;
	case IGFX_GEN8_CORE:
		if (mem_ctrl > MEMORY_OBJECT_CONTROL_BDW_L3_LLC_ELLC_ALLOWED)
			return FALSE;
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

INT CmSurface::SetMemoryObjectControl(MEMORY_OBJECT_CONTROL mem_ctrl,
				      MEMORY_TYPE mem_type, UINT age)
{
	if (!MemoryObjectCtrlPolicyCheck(mem_ctrl, mem_type, age)) {
		return CM_FAILURE;
	}

	m_MemObjCtrl.mem_ctrl = mem_ctrl;
	m_MemObjCtrl.mem_type = mem_type;
	m_MemObjCtrl.age = age;

	return CM_SUCCESS;
}

VOID CmSurface::GetMemoryObjectCtrl(CM_SURFACE_MEM_OBJ_CTRL * mem_ctrl)
{
	if (!mem_ctrl) {
		CM_ASSERT(0);
		return;
	}
	mem_ctrl->mem_ctrl = m_MemObjCtrl.mem_ctrl;
	mem_ctrl->mem_type = m_MemObjCtrl.mem_type;
	mem_ctrl->age = m_MemObjCtrl.age;

	return;
}
