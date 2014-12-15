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

class CmDevice;

class CmProgram {
 public:
	static INT Create(CmDevice * pCmDev, void *pCISACode,
			  const UINT uiCISACodeSize, void *pGenCode,
			  const UINT uiGenCodeSize, CmProgram * &pProgram,
			  const char *options, const UINT programId);
	static INT Destroy(CmProgram * &pProgram);

	INT GetCommonISACode(void *&pCommonISACode, UINT & size);
	INT GetKernelCount(UINT & kernelCount);
	INT GetKernelInfo(UINT index, CM_KERNEL_INFO * &pKernelInfo);
	INT GetIsaFileName(char *&kernelName);
	INT GetKernelOptions(char *&kernelOptions);

	UINT GetSurfaceCount(void);
	INT SetSurfaceCount(UINT count);

	BOOL IsJitterEnabled(void) {
		return m_IsJitterEnabled;
	} BOOL IsHwDebugEnabled(void) {
		return m_IsHwDebugEnabled;
	}

	UINT AcquireKernelInfo(UINT index);
	UINT ReleaseKernelInfo(UINT index);
	INT GetKernelInfoRefCount(UINT index, UINT & refCount);

	INT GetCISAVersion(UINT & majorVersion, UINT & minorVersion);

	INT Acquire(void);
	INT SafeRelease(void);

	UINT GetProgramIndex();

 protected:
	CmProgram(CmDevice * pCmDev, UINT programId);
	~CmProgram(void);

	INT Initialize(void *pCISACode, const UINT uiCISACodeSize,
		       void *pGenCode, const UINT uiGenCodeSize,
		       const char *options);

	CmDevice *m_pCmDev;

	UINT m_ProgramCodeSize;
	BYTE *m_pProgramCode;
	char *m_Options;
	char m_IsaFileName[CM_MAX_ISA_FILE_NAME_SIZE_IN_BYTE];
	UINT m_SurfaceCount;

	UINT m_KernelCount;
	CmDynamicArray m_pKernelInfo;

	BOOL m_IsJitterEnabled;
	BOOL m_IsHwDebugEnabled;

	UINT m_refCount;

	UINT m_programIndex;

	pJITCompile m_fJITCompile;
	pFreeBlock m_fFreeBlock;
	pJITVersion m_fJITVersion;

 public:
	DWORD m_CISA_magicNumber;
	BYTE m_CISA_majorVersion;
	BYTE m_CISA_minorVersion;

 private:
	CmProgram(const CmProgram & other);
	CmProgram & operator=(const CmProgram & other);
};
