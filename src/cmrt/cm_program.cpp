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

#include "cm_device.h"
#include "cm_program.h"
#include "hal_cm.h"

namespace {

	class SharedLibrary {
 public:
		SharedLibrary(const char *path):handle_(NULL), path_(path) {
		} ~SharedLibrary();

		bool open();
		void *symbolAddress(const char *symbol);

 private:
		void *handle_;
		const char *path_;
	};

	inline SharedLibrary::~SharedLibrary() {
		if (handle_)
			dlclose(handle_);
	}

	inline bool SharedLibrary::open() {
		handle_ = dlopen((path_), RTLD_NOW);
		return handle_ != NULL;
	}

	inline void *SharedLibrary::symbolAddress(const char *symbol) {
		return dlsym(handle_, symbol);
	}

	static const char g_soName32[] = "libigfxdbgxchg32.so";
	static const char g_soName64[] = "libigfxdbgxchg64.so";

	static const char notifyKernelBinaryName[] = "notifyKernelBinary";
	static const char requestSipBinaryName[] = "requestSipBinary";

	class SharedLibraryHolder {
 public:
		SharedLibraryHolder() {
			if (!sl_) {
				const char *soName =
				    sizeof(void *) ==
				    4 ? g_soName32 : g_soName64;

				 sl_ = new SharedLibrary(soName);
				if (!sl_->open()) {
					delete sl_;
					 sl_ = NULL;
				}

			}
		}
		bool isEnabled() const {
			return sl_ != NULL;
		}
		SharedLibrary *operator->() {
			return sl_;
		}

 private:
		static SharedLibrary *sl_;
	};

	SharedLibrary *SharedLibraryHolder::sl_ = NULL;

	typedef void *Handle;
	typedef Handle CmUmdDeviceHandle;
	typedef Handle CmUmdProgramHandle;

	const unsigned int IGFX_DBG_CURRENT_VERSION = 1;
}

#define READ_FIELD_FROM_BUF( dst, type ) \
    dst = *((type *) &buf[byte_pos]); \
    byte_pos += sizeof(type);

INT CmProgram::Create(CmDevice * pCmDev, void *pCISACode,
		      const UINT uiCISACodeSize, void *pGenCode,
		      const UINT uiGenCodeSize, CmProgram * &pProgram,
		      const char *options, const UINT programId)
{
	INT result = CM_SUCCESS;
	pProgram = new(std::nothrow) CmProgram(pCmDev, programId);
	if (pProgram) {
		pProgram->Acquire();
		result =
		    pProgram->Initialize(pCISACode, uiCISACodeSize, pGenCode,
					 uiGenCodeSize, options);
		if (result != CM_SUCCESS) {
			CmProgram::Destroy(pProgram);
		}
	} else {
		CM_ASSERT(0);
		result = CM_OUT_OF_HOST_MEMORY;
	}
	return result;
}

INT CmProgram::Destroy(CmProgram * &pProgram)
{
	long refCount = pProgram->SafeRelease();
	if (refCount == 0) {
		pProgram = NULL;
	}
	return CM_SUCCESS;
}

INT CmProgram::Acquire(void)
{
	m_refCount++;
	return CM_SUCCESS;
}

INT CmProgram::SafeRelease(void)
{
	--m_refCount;
	if (m_refCount == 0) {
		delete this;
		return 0;
	}
	return m_refCount;
}

 CmProgram::CmProgram(CmDevice * pCmDev, UINT programId):
m_pCmDev(pCmDev),
m_ProgramCodeSize(0),
m_pProgramCode(NULL),
m_Options(NULL),
m_SurfaceCount(0),
m_KernelCount(0),
m_pKernelInfo(CM_INIT_KERNEL_PER_PROGRAM),
m_IsJitterEnabled(false),
m_refCount(0),
m_programIndex(programId),
m_fJITCompile(NULL),
m_fFreeBlock(NULL),
m_fJITVersion(NULL),
m_CISA_magicNumber(0), m_CISA_majorVersion(0), m_CISA_minorVersion(0)
{
	CmSafeMemSet(m_IsaFileName, 0,
		     sizeof(char) * CM_MAX_ISA_FILE_NAME_SIZE_IN_BYTE);
}

CmProgram::~CmProgram(void)
{
	CmSafeDeleteArray(m_Options);
	CmSafeDeleteArray(m_pProgramCode);
	for (UINT i = 0; i < m_KernelCount; i++) {
		this->ReleaseKernelInfo(i);
	}
	m_pKernelInfo.Delete();
}

INT CmProgram::Initialize(void *pCISACode, const UINT uiCISACodeSize,
			  void *pGenCode, const UINT uiGenCodeSize,
			  const char *options)
{
	BOOL bLoadingGPUCopyKernel = false;
	UINT genid = IGFX_UNKNOWN_CORE;
	INT result = CM_SUCCESS;
	INT hr = CM_FAILURE;

	m_IsJitterEnabled = true;

	int numJitFlags = 0;
	const char *jitFlags[CM_RT_JITTER_MAX_NUM_FLAGS];
	CmSafeMemSet(jitFlags, 0, sizeof(char *) * CM_RT_JITTER_MAX_NUM_FLAGS);

	char *pFlagStepInfo = NULL;

	if (options) {
		size_t length = strnlen(options, CM_MAX_OPTION_SIZE_IN_BYTE);
		if (length >= CM_MAX_OPTION_SIZE_IN_BYTE) {
			CM_ASSERT(0);
			return CM_INVALID_ARG_VALUE;
		} else {
			m_Options = new(std::nothrow) char[length + 1];
			if (!m_Options) {
				CM_ASSERT(0);
				return CM_OUT_OF_HOST_MEMORY;

			}
			CmFastMemCopy(m_Options, options, length);
			m_Options[length] = '\0';

			if (strstr(options, "nojitter"))
				m_IsJitterEnabled = false;

			if (!strcmp(m_Options, "PredefinedGPUCopyKernel"))
				bLoadingGPUCopyKernel = true;
			if ((m_IsJitterEnabled == TRUE)
			    && (bLoadingGPUCopyKernel == FALSE)) {
				char *token = NULL;
				char *next_token = NULL;
				char *ptr = m_Options;

				while (NULL !=
				       (token =
					strtok_s(ptr, " ", &next_token))) {
					if (numJitFlags >=
					    CM_RT_JITTER_MAX_NUM_USER_FLAGS) {
						CM_ASSERT(0);
						CmSafeDeleteArray(m_Options);
						return CM_FAILURE;
					}

					jitFlags[numJitFlags] = token;
					numJitFlags++;
					ptr = next_token;
				}
			}
		}
	}

	BYTE *buf = (BYTE *) pCISACode;
	UINT byte_pos = 0;

	READ_FIELD_FROM_BUF(m_CISA_magicNumber, DWORD);
	READ_FIELD_FROM_BUF(m_CISA_majorVersion, BYTE);
	READ_FIELD_FROM_BUF(m_CISA_minorVersion, BYTE);

	if ((m_CISA_majorVersion >= 2) && (m_CISA_minorVersion >= 4)) {
		if (m_CISA_magicNumber != CISA_MAGIC_NUMBER) {
			CM_ASSERT(0);
			CmSafeDeleteArray(m_Options);
			return CM_INVALID_COMMON_ISA;
		}
	}

	if (bLoadingGPUCopyKernel) {
		m_IsJitterEnabled = false;
	}

	m_pCmDev->GetGenPlatform(genid);
	const char *platform = NULL;
	UINT CISAGenID = -1;

	PCM_HAL_STATE pCmHalState;
	PCM_CONTEXT pCmData;

	pCmData = (PCM_CONTEXT) m_pCmDev->GetAccelData();
	CMCHK_NULL(pCmData);

	pCmHalState = pCmData->pCmHalState;
	CMCHK_NULL(pCmHalState);

	switch (genid) {
	case IGFX_GEN7_5_CORE:
		platform = "HSW";
		CISAGenID = GENX_HSW;
		break;
	case IGFX_GEN8_CORE:
		if (GFX_IS_PRODUCT
		    (pCmHalState->pHwInterface->Platform, IGFX_CHERRYVIEW)) {
			platform = "CHV";
			CISAGenID = GENX_CHV;
		} else {
			platform = "BDW";
			CISAGenID = GENX_BDW;
		}
		break;
	default:
		m_IsJitterEnabled = false;
	}

	if (m_IsJitterEnabled) {

		result = m_pCmDev->LoadJITDll();
		if (result != CM_SUCCESS) {
			CM_ASSERT(0);
			CmSafeDeleteArray(m_Options);
			return result;
		}

		m_pCmDev->GetJITCompileFnt(m_fJITCompile);
		m_pCmDev->GetFreeBlockFnt(m_fFreeBlock);
		m_pCmDev->GetJITVersionFnt(m_fJITVersion);

		UINT jitMajor = 0;
		UINT jitMinor = 0;
		m_fJITVersion(jitMajor, jitMinor);
		if ((jitMajor < m_CISA_majorVersion)
		    || (jitMajor == m_CISA_majorVersion
			&& jitMinor < m_CISA_minorVersion))
			return CM_JITDLL_OLDER_THAN_ISA;

		if (genid == IGFX_GEN8_CORE) {
			char *stepstr = NULL;
			m_pCmDev->GetGenStepInfo(genid, stepstr);
			if (stepstr != NULL) {
				pFlagStepInfo =
				    new(std::nothrow) char[CM_JIT_FLAG_SIZE];
				if (pFlagStepInfo) {
					jitFlags[numJitFlags] = "-stepping";
					numJitFlags++;

					CmSafeMemSet(pFlagStepInfo, 0,
						     CM_JIT_FLAG_SIZE);
					sprintf_s(pFlagStepInfo,
						  CM_JIT_FLAG_SIZE, "%s",
						  stepstr);
					jitFlags[numJitFlags] = pFlagStepInfo;
					numJitFlags++;
				} else {
					CM_ASSERT(0);
					CmSafeDeleteArray(m_Options);
					return CM_OUT_OF_HOST_MEMORY;
				}
			}
		}
	}

	byte_pos = 4 + 1 + 1;

	unsigned short num_kernels;
	READ_FIELD_FROM_BUF(num_kernels, unsigned short);

	m_KernelCount = num_kernels;

	for (UINT i = 0; i < m_KernelCount; i++) {
		BYTE name_len;
		READ_FIELD_FROM_BUF(name_len, BYTE);

		if (name_len > CM_MAX_KERNEL_NAME_SIZE_IN_BYTE) {
			CM_ASSERT(0);
			hr = CM_FAILURE;
			goto finish;
		}

		CM_KERNEL_INFO *pKernInfo = new(std::nothrow) CM_KERNEL_INFO;
		if (!pKernInfo) {
			CM_ASSERT(0);
			CmSafeDeleteArray(pFlagStepInfo);
			CmSafeDeleteArray(m_Options);
			hr = CM_OUT_OF_HOST_MEMORY;
			goto finish;
		}

		CmSafeMemSet(pKernInfo, 0, sizeof(CM_KERNEL_INFO));
		CmFastMemCopy(pKernInfo->kernelName, buf + byte_pos, name_len);

		byte_pos += name_len;

		if (m_IsJitterEnabled) {
			UINT kernelIsaOffset;
			UINT kernelIsaSize;
			UINT inputCountOffset;

			READ_FIELD_FROM_BUF(kernelIsaOffset, UINT);
			READ_FIELD_FROM_BUF(kernelIsaSize, UINT);
			READ_FIELD_FROM_BUF(inputCountOffset, UINT);

			pKernInfo->kernelIsaOffset = kernelIsaOffset;
			pKernInfo->kernelIsaSize = kernelIsaSize;
			pKernInfo->inputCountOffset = inputCountOffset;

			if (m_CISA_majorVersion < 3) {
				byte_pos += 2 * 4;
			}
			if (m_CISA_majorVersion >= 3) {
				unsigned short num_var_syms = 0, num_func_syms =
				    0;
				unsigned char num_gen_binaries = 0;

				READ_FIELD_FROM_BUF(num_var_syms, WORD);
				byte_pos += sizeof(WORD) * num_var_syms;
				byte_pos += sizeof(WORD) * num_var_syms;

				READ_FIELD_FROM_BUF(num_func_syms, WORD);
				byte_pos += sizeof(WORD) * num_func_syms;
				byte_pos += sizeof(WORD) * num_func_syms;

				READ_FIELD_FROM_BUF(num_gen_binaries, BYTE);
				byte_pos += sizeof(BYTE) * num_gen_binaries;
				byte_pos += sizeof(UINT) * num_gen_binaries;
				byte_pos += sizeof(UINT) * num_gen_binaries;

			}
		} else {
			UINT kernelIsaOffset;
			READ_FIELD_FROM_BUF(kernelIsaOffset, UINT);
			pKernInfo->kernelIsaOffset = kernelIsaOffset;

			byte_pos += 4;

			UINT inputCountOffset;
			UINT genxBinaryOffset = 0;
			UINT genxBinarySize = 0;

			READ_FIELD_FROM_BUF(inputCountOffset, UINT);
			pKernInfo->inputCountOffset = inputCountOffset;

			if (m_CISA_majorVersion < 3) {
				READ_FIELD_FROM_BUF(genxBinaryOffset, UINT);
				READ_FIELD_FROM_BUF(genxBinarySize, UINT);
			} else {
				unsigned short num_var_syms = 0, num_func_syms =
				    0;
				unsigned char num_gen_binaries = 0;

				READ_FIELD_FROM_BUF(num_var_syms, WORD);
				byte_pos += sizeof(WORD) * num_var_syms;
				byte_pos += sizeof(WORD) * num_var_syms;

				READ_FIELD_FROM_BUF(num_func_syms, WORD);
				byte_pos += sizeof(WORD) * num_func_syms;
				byte_pos += sizeof(WORD) * num_func_syms;

				READ_FIELD_FROM_BUF(num_gen_binaries, BYTE);

				for (int j = 0; j < num_gen_binaries; j++) {
					unsigned char gen_platform;
					unsigned int offset, size;

					READ_FIELD_FROM_BUF(gen_platform, BYTE);

					READ_FIELD_FROM_BUF(offset, UINT);
					READ_FIELD_FROM_BUF(size, UINT);

					if (gen_platform == CISAGenID ||
					    ((CISAGenID == GENX_CHV)
					     && (gen_platform == GENX_BDW))) {
						genxBinaryOffset = offset;
						genxBinarySize = size;
					}
				}
			}

			if (pGenCode == NULL) {
				pKernInfo->genxBinaryOffset = genxBinaryOffset;
				pKernInfo->genxBinarySize = genxBinarySize;
			} else {
				pKernInfo->genxBinaryOffset = uiCISACodeSize;
				pKernInfo->genxBinarySize = uiGenCodeSize;
			}

			if (pKernInfo->genxBinarySize == 0
			    || pKernInfo->genxBinaryOffset == 0) {
				CM_ASSERT(0);
				CmSafeDeleteArray(pFlagStepInfo);
				CmSafeDeleteArray(m_Options);
				CmSafeDelete(pKernInfo);
				return CM_INVALID_GENX_BINARY;
			}
		}

		if (m_IsJitterEnabled) {
			pKernInfo->jitBinaryCode = 0;
			void *jitBinary = 0;
			UINT jitBinarySize = 0;
			char *errorMsg =
			    (char *)malloc(CM_JIT_ERROR_MESSAGE_SIZE);
			if (errorMsg == NULL) {
				CM_ASSERT(0);
				CmSafeDelete(pKernInfo);
				hr = CM_OUT_OF_HOST_MEMORY;
				goto finish;
			}
			CmSafeMemSet(errorMsg, 0, CM_JIT_ERROR_MESSAGE_SIZE);

			CM_JIT_INFO *jitProfInfo =
			    (CM_JIT_INFO *) malloc(CM_JIT_PROF_INFO_SIZE);
			if (jitProfInfo == NULL) {
				CM_ASSERT(0);
				free(errorMsg);
				CmSafeDelete(pKernInfo);
				hr = CM_OUT_OF_HOST_MEMORY;
				goto finish;
			}
			CmSafeMemSet(jitProfInfo, 0, CM_JIT_PROF_INFO_SIZE);

			if (m_CISA_majorVersion >= 3) {
				result =
				    m_fJITCompile(pKernInfo->kernelName, buf,
						  uiCISACodeSize, jitBinary,
						  jitBinarySize, platform,
						  m_CISA_majorVersion,
						  m_CISA_minorVersion,
						  numJitFlags, jitFlags,
						  errorMsg, jitProfInfo);
			} else {
				result =
				    m_fJITCompile(pKernInfo->kernelName,
						  buf +
						  pKernInfo->kernelIsaOffset,
						  pKernInfo->kernelIsaSize,
						  jitBinary, jitBinarySize,
						  platform, m_CISA_majorVersion,
						  m_CISA_minorVersion,
						  numJitFlags, jitFlags,
						  errorMsg, jitProfInfo);
			}

			if (result != CM_SUCCESS) {
				CM_NORMALMESSAGE("%s.", errorMsg);
				free(errorMsg);
				CmSafeDelete(pKernInfo);
				hr = CM_JIT_COMPILE_FAILURE;
				goto finish;
			}
			if (jitProfInfo->isSpill == TRUE
			    && m_pCmDev->IsScratchSpaceDisabled()) {
				CmSafeDelete(pKernInfo);
				free(errorMsg);
				return CM_INVALID_KERNEL_SPILL_CODE;
			}

			free(errorMsg);

			pKernInfo->jitBinaryCode = jitBinary;
			pKernInfo->jitBinarySize = jitBinarySize;
			pKernInfo->jitInfo = jitProfInfo;

		}

		m_pKernelInfo.SetElement(i, pKernInfo);
		this->AcquireKernelInfo(i);
	}

	m_ProgramCodeSize = uiCISACodeSize + uiGenCodeSize;
	m_pProgramCode = new(std::nothrow) BYTE[m_ProgramCodeSize];
	if (!m_pProgramCode) {
		CM_ASSERT(0);
		hr = CM_OUT_OF_HOST_MEMORY;
		goto finish;
	}
	CmFastMemCopy((void *)m_pProgramCode, pCISACode, uiCISACodeSize);

	if (pGenCode) {
		CmFastMemCopy(m_pProgramCode + uiCISACodeSize, pGenCode,
			      uiGenCodeSize);
	}

	hr = CM_SUCCESS;

 finish:
	if (hr != CM_SUCCESS) {
		CmSafeDeleteArray(m_Options);
		CmSafeDeleteArray(pFlagStepInfo);
		CmSafeDeleteArray(m_pProgramCode);
	}
	return hr;
}

INT CmProgram::GetCommonISACode(void *&pCommonISACode, UINT & size)
{
	pCommonISACode = (void *)m_pProgramCode;
	size = m_ProgramCodeSize;

	return CM_SUCCESS;
}

INT CmProgram::GetKernelCount(UINT & kernelCount)
{
	kernelCount = m_KernelCount;
	return CM_SUCCESS;
}

INT CmProgram::GetKernelInfo(UINT index, CM_KERNEL_INFO * &pKernelInfo)
{
	if (index < m_KernelCount) {
		pKernelInfo =
		    (CM_KERNEL_INFO *) m_pKernelInfo.GetElement(index);
		return CM_SUCCESS;
	} else {
		pKernelInfo = NULL;
		return CM_FAILURE;
	}
}

INT CmProgram::GetIsaFileName(char *&isaFileName)
{
	isaFileName = m_IsaFileName;
	return CM_SUCCESS;
}

INT CmProgram::GetKernelOptions(char *&kernelOptions)
{
	kernelOptions = m_Options;
	return CM_SUCCESS;
}

UINT CmProgram::GetSurfaceCount(void)
{
	return m_SurfaceCount;
}

INT CmProgram::SetSurfaceCount(UINT count)
{
	m_SurfaceCount = count;
	return CM_SUCCESS;
}

UINT CmProgram::AcquireKernelInfo(UINT index)
{
	CM_KERNEL_INFO *pKernelInfo = NULL;

	if (index < m_KernelCount) {
		pKernelInfo =
		    (CM_KERNEL_INFO *) m_pKernelInfo.GetElement(index);
		if (pKernelInfo) {
			CM_ASSERT((INT) pKernelInfo->kernelInfoRefCount >= 0);
			CM_ASSERT(pKernelInfo->kernelInfoRefCount < UINT_MAX);

			++pKernelInfo->kernelInfoRefCount;
			return pKernelInfo->kernelInfoRefCount;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

UINT CmProgram::ReleaseKernelInfo(UINT index)
{
	CM_KERNEL_INFO *pKernelInfo = NULL;

	if (index < m_KernelCount) {
		pKernelInfo =
		    (CM_KERNEL_INFO *) m_pKernelInfo.GetElement(index);
		if (pKernelInfo) {
			CM_ASSERT(pKernelInfo->kernelInfoRefCount > 0);

			--pKernelInfo->kernelInfoRefCount;

			if (pKernelInfo->kernelInfoRefCount == 1) {
				for (int i = 0;
				     i < pKernelInfo->globalStringCount; i++) {
					if (pKernelInfo->globalStrings[i]) {
						free((void *)
						     pKernelInfo->globalStrings
						     [i]);
					}
				}
				if (pKernelInfo->globalStrings) {
					free((void *)
					     pKernelInfo->globalStrings);
					pKernelInfo->globalStrings = NULL;
					pKernelInfo->globalStringCount = 0;
				}

				for (UINT i = 0; i < pKernelInfo->surface_count;
				     i++) {
					if (pKernelInfo->
					    surface[i].attribute_count
					    && pKernelInfo->
					    surface[i].attributes) {
						free(pKernelInfo->
						     surface[i].attributes);
					}
				}
				if (pKernelInfo->surface) {
					free(pKernelInfo->surface);
					pKernelInfo->surface = NULL;
					pKernelInfo->surface_count = 0;
				}

				return 1;
			}

			else if (pKernelInfo->kernelInfoRefCount == 0) {
				if (m_IsJitterEnabled) {
					if (pKernelInfo
					    && pKernelInfo->jitBinaryCode)
						m_fFreeBlock
						    (pKernelInfo->jitBinaryCode);
					if (pKernelInfo && pKernelInfo->jitInfo)
						free(pKernelInfo->jitInfo);
				}
				for (int i = 0;
				     i < pKernelInfo->globalStringCount; i++) {
					if (pKernelInfo->globalStrings[i]) {
						free((void *)
						     pKernelInfo->globalStrings
						     [i]);
					}
				}
				if (pKernelInfo->globalStrings) {
					free((void *)
					     pKernelInfo->globalStrings);
				}

				for (UINT i = 0; i < pKernelInfo->surface_count;
				     i++) {
					if (pKernelInfo->
					    surface[i].attribute_count
					    && pKernelInfo->
					    surface[i].attributes) {
						free(pKernelInfo->
						     surface[i].attributes);
					}
				}
				if (pKernelInfo->surface) {
					free(pKernelInfo->surface);
				}

				CmSafeDelete(pKernelInfo);

				m_pKernelInfo.SetElement(index, NULL);

				return 0;
			} else {
				return pKernelInfo->kernelInfoRefCount;
			}
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

INT CmProgram::GetKernelInfoRefCount(UINT index, UINT & refCount)
{
	CM_KERNEL_INFO *pKernelInfo = NULL;

	refCount = 0;

	if (index < m_KernelCount) {
		pKernelInfo =
		    (CM_KERNEL_INFO *) m_pKernelInfo.GetElement(index);
		if (pKernelInfo) {
			refCount = pKernelInfo->kernelInfoRefCount;
			return CM_SUCCESS;
		} else {
			return CM_FAILURE;
		}
	} else {
		return CM_FAILURE;
	}
}

INT CmProgram::GetCISAVersion(UINT & majorVersion, UINT & minorVersion)
{
	majorVersion = m_CISA_majorVersion;
	minorVersion = m_CISA_minorVersion;

	return CM_SUCCESS;
}

UINT CmProgram::GetProgramIndex()
{
	return m_programIndex;
}
