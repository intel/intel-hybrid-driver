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
#include "cm_array.h"
#include "cm_kernel.h"

class CmKernelData:public CmDynamicArray {
 public:

	static INT Create(CmKernel * pCmKernel, CmKernelData * &pKernelData);
	static INT Destroy(CmKernelData * &pKernelData);

	INT GetCmKernel(CmKernel * &pCmKernel);
	INT SetKernelDataSize(INT value);
	INT GetKernelDataSize();
	UINT Acquire(void);
	UINT SafeRelease(void);

	PCM_HAL_KERNEL_PARAM GetHalCmKernelData();
	UINT AcquireKernel(void);
	INT ReleaseKernel(void);
	BOOL IsInUse(void);
	UINT GetKernelCurbeSize(void);

 protected:

	 CmKernelData(CmKernel * pCmKernel);
	~CmKernelData(void);

	INT Initialize(void);

	UINT m_kerneldatasize;
	CmKernel *m_pCmKernel;
	UINT m_RefCount;
	CM_HAL_KERNEL_PARAM m_HalKernelParam;

	UINT m_KernelRef;

	BOOL m_IsInUse;

 private:
	 CmKernelData(const CmKernelData & other);
	 CmKernelData & operator=(const CmKernelData & other);
};
