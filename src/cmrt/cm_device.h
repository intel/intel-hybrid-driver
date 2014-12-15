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
#include <time.h>
#include <i915_drm.h>

#include "cm_common.h"
#include "os_interface.h"
#include "cm_buffer.h"
#include "cm_array.h"
#include "cm_program.h"
#include "cm_surface_2d.h"
#include "cm_surface_2d_up.h"

class CmSurfaceManager;
class CmQueue;
class CmKernel;
class CmTask;
class CmThreadSpace;
class CmThreadGroupSpace;

class CmDevice {
 public:

	static INT Create(CmDriverContext * pUmdContext, CmDevice * &pDevice,
			  UINT DevCreateOption =
			  CM_DEVICE_CREATE_OPTION_DEFAULT);
	static INT Destroy(CmDevice * &pDevice);

	CM_RT_API INT CreateBuffer(UINT size, CmBuffer * &pSurface);
	CM_RT_API INT CreateBuffer(CmOsResource * pOsResource,
				   CmBuffer * &pSurface);
	CM_RT_API INT DestroySurface(CmBuffer * &pSurface);
	CM_RT_API INT GetSurface2DInfo(UINT width, UINT height,
				       CM_SURFACE_FORMAT format, UINT & pitch,
				       UINT & physicalSize);
	CM_RT_API INT CreateSurface2DUP(UINT width, UINT height,
					CM_SURFACE_FORMAT format, void *pSysMem,
					CmSurface2DUP * &pSurface);
	CM_RT_API INT DestroySurface(CmSurface2DUP * &pSurface);

	CM_RT_API INT CreateSurface2D(UINT width, UINT height,
				      CM_SURFACE_FORMAT format,
				      CmSurface2D * &pSurface);
	CM_RT_API INT CreateSurface2D(CmOsResource * pCmOsResource,
				      CmSurface2D * &pSurface);

	CM_RT_API INT DestroySurface(CmSurface2D * &pSurface);

	CM_RT_API INT LoadProgram(void *pCommonISACode, const UINT size,
				  CmProgram * &pProgram, const char *options =
				  NULL);
	CM_RT_API INT DestroyProgram(CmProgram * &pProgram);

	CM_RT_API INT CreateBufferUP(UINT size, void *pSystMem,
				     CmBufferUP * &pSurface);
	CM_RT_API INT DestroyBufferUP(CmBufferUP * &pSurface);
	CM_RT_API INT ForceDestroyBufferUP(CmBufferUP * &pSurface);

	CM_RT_API INT CreateKernel(CmProgram * pProgram, const char *kernelName,
				   CmKernel * &pKernel, const char *options =
				   NULL);
	CM_RT_API INT DestroyKernel(CmKernel * &pKernel);

	CM_RT_API INT CreateQueue(CmQueue * &pQueue);
	CM_RT_API INT CreateTask(CmTask * &pTask);
	CM_RT_API INT DestroyTask(CmTask * &pTask);

	CM_RT_API INT CreateThreadSpace(UINT width, UINT height,
					CmThreadSpace * &pTS);
	CM_RT_API INT DestroyThreadSpace(CmThreadSpace * &pTS);

	CM_RT_API INT CreateThreadGroupSpace(UINT thrdSpaceWidth,
					     UINT thrdSpaceHeight,
					     UINT grpSpaceWidth,
					     UINT grpSpaceHeight,
					     CmThreadGroupSpace * &pTGS);
	CM_RT_API INT DestroyThreadGroupSpace(CmThreadGroupSpace * &pTGS);

	void *GetAccelData(void) {
		return m_pAccelData;
	}
	GENOS_CONTEXT *GetUMDCtx(void) {
		return m_pUmdContext;
	}
	DWORD GetAccelsize(void) {
		return m_AccelSize;
	}
	INT GetHalMaxValues(CM_HAL_MAX_VALUES * &pHalMaxValues,
			    CM_HAL_MAX_VALUES_EX * &pHalMaxValuesEx);
	INT GetGenPlatform(UINT & platform);

	INT GetSurfaceManager(CmSurfaceManager * &pSurfaceMgr);
	INT GetQueue(CmQueue * &pQueue);
	CSync *GetSurfaceLock();
	CSync *GetSurfaceCreationLock();
	CSync *GetProgramKernelLock();

	INT GetJITCompileFnt(pJITCompile & fJITCompile);
	INT GetJITCompileSimFnt(pJITCompileSim & fJITCompileSim);
	INT GetFreeBlockFnt(pFreeBlock & fFreeBlock);
	INT GetJITVersionFnt(pJITVersion & fJITVersion);
	INT LoadJITDll(void);

	INT GetDDIVersion(UINT & DDIVersion);

	CM_RT_API INT GetCaps(CM_DEVICE_CAP_NAME capName, size_t & capValueSize,
			      void *pCapValue);
	INT GetGenStepInfo(UINT platform, char *&stepinfostr);
	INT GetCapsInternal(PVOID pCaps, PUINT puSize);
	INT SetCapsInternal(CM_DEVICE_CAP_NAME capName, size_t capValueSize,
			    void *pCapValue);

	INT GetOSSyncEventHandle(PVOID & hOSSyncEvent);

	INT Acquire(void);
	INT Release(void);

	INT GetSurf2DLookUpEntry(UINT index, PCMLOOKUP_ENTRY & pLookupEntry);

	INT LoadProgramWithGenCode(void *pCISACode, const UINT uiCISACodeSize,
				   void *pGenCode, const UINT uiGenCodeSize,
				   CmProgram * &pProgram, const char *options =
				   NULL);

	INT GetSurface2DInPool(UINT width, UINT height,
			       CM_SURFACE_FORMAT format,
			       CmSurface2D * &pSurface);
	INT GetSurfaceIDInPool(INT iIndex);
	INT DestroySurfaceInPool(UINT & freeSurfNum);
	INT DestroySurface(CmBuffer * &pSurface, INT iIndexInPool,
			   INT iSurfaceID, SURFACE_DESTROY_KIND kind);
	INT DestroySurface(CmSurface2D * &pSurface, INT iIndexInPool,
			   INT iSurfaceID, SURFACE_DESTROY_KIND kind);
	INT DestroyBufferUP(CmBufferUP * &pSurface, INT iIndexInPool,
			    INT iSurfaceID, SURFACE_DESTROY_KIND kind);
	INT DestroySurface(CmSurface2DUP * &pSurface, INT iIndexInPool,
			   INT iSurfaceID, SURFACE_DESTROY_KIND kind);

	INT CreateSurface2D(UINT width, UINT height, BOOL bIsCmCreated,
			    CM_SURFACE_FORMAT format, CmSurface2D * &pSurface);

	BOOL IsScratchSpaceDisabled();
	BOOL IsSurfaceReuseEnabled();

	UINT ValidSurfaceIndexStart();
	UINT MaxIndirectSurfaceCount();
	BOOL IsCmReservedSurfaceIndex(UINT surfBTI);
	BOOL IsValidSurfaceIndex(UINT surfBTI);

 protected:
	INT Initialize(CmDriverContext * pUmdContext);

	INT CreateAuxDevice(CmDriverContext * pUmdContext);
	INT DestroyAuxDevice(void);

	INT CreateQueue_Internel(void);
	INT DestroyQueue(CmQueue * &pQueue);

	INT GetMaxValueFromCaps(CM_HAL_MAX_VALUES & MaxValues,
				CM_HAL_MAX_VALUES_EX & MaxValuesEx);

	INT InitDevCreateOption(CM_HAL_CREATE_PARAM & DevCreateParam,
				UINT DevCreateOption);

	CmDevice(UINT DevCreateOption);
	~CmDevice(void);

	GENOS_CONTEXT *m_pUmdContext;

	void *m_pAccelData;
	DWORD m_AccelSize;

	CM_HAL_MAX_VALUES m_HalMaxValues;
	CM_HAL_MAX_VALUES_EX m_HalMaxValuesEx;

	CmSurfaceManager *m_pSurfaceMgr;
	CmQueue *m_pQueue;

	CmDynamicArray m_ProgramArray;
	UINT m_ProgramCount;

	CmDynamicArray m_KernelArray;
	UINT m_KernelCount;

	CmDynamicArray m_ThreadSpaceArray;
	UINT m_ThreadSpaceCount;

	HMODULE m_hJITDll;
	pJITCompile m_fJITCompile;
	pFreeBlock m_fFreeBlock;
	pJITVersion m_fJITVersion;

	UINT m_DDIVersion;
	UINT m_Platform;
	UINT m_CmDeviceRefCount;

	CSync m_CriticalSection_Program_Kernel;
	CSync m_CriticalSection_Surface;
	CSync m_CriticalSection_ReadWriteSurface2D;
	CSync m_CriticalSection_ThreadSpace;
	CSync m_CriticalSection_DeviceRefCount;
	CSync m_CriticalSection_ThreadGroupSpace;
	CSync m_CriticalSection_Task;

 public:
	static CSync GlobalCriticalSection_Surf2DUserDataLock;

 protected:
	CmDynamicArray m_ThreadGroupSpaceArray;
	UINT m_ThreadGroupSpaceCount;

	CmDynamicArray m_TaskArray;
	UINT m_TaskCount;

	CM_HAL_CREATE_PARAM m_DevCreateOption;

 private:
	CmDevice(const CmDevice & other);
	CmDevice & operator=(const CmDevice & other);
};

EXTERN_C INT CreateCmDevice(CmDevice * &pDevice, UINT & version,
			    CmDriverContext * drivercontext,
			    UINT DevCreateOption =
			    CM_DEVICE_CREATE_OPTION_DEFAULT);

EXTERN_C INT DestroyCmDevice(CmDevice * &pDevice);
