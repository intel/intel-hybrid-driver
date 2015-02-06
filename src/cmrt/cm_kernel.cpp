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

#include "cm_kernel.h"
#include "cm_program.h"
#include "cm_device.h"
#include "cm_surface_manager.h"
#include "cm_buffer.h"
#include "cm_mov_inst.h"
#include "cm_kernel_data.h"
#include "cm_thread_space.h"
#include "cm_def.h"
#include "cm_debug.h"
#include "cm_group_space.h"
#include "hal_cm.h"

#define GENERATE_GLOBAL_SURFACE_INDEX

#define READ_FIELD_FROM_BUF( dst, type ) \
    dst = *((type *) &buf[byte_pos]); \
    byte_pos += sizeof(type);

#define PER_ARG_SIZE_IN_DWORD 3
#define KERNEL_INFO_SIZE_IN_DWORD 4

#define DW_ALIGNMENT( byte_address ) \
    if( byte_address % 4 ) \
    byte_address = ( byte_address / 4 + 1 ) * 4;

#define GRF_ALIGNMENT( byte_address ) \
    if( byte_address % 32 ) \
    byte_address = ( byte_address / 32 + 1 ) * 32;

#define CHECK_SURFACE_TYPE( nType, ... )  ( _CheckSurfaceType( nType, __VA_ARGS__, -1 ) )

#define IsKernelArg(arg)    ((arg).iUnitCount == 1)

typedef CM_ARG *PCM_ARG;

INT Partition(PCM_ARG * ppArg, INT p, INT r)
{
	WORD x = ppArg[p]->unitOffsetInPayload;
	INT i = p - 1;
	INT j = r + 1;
	while (1) {
		do {
			j--;
		}
		while (ppArg[j]->unitOffsetInPayload > x);

		do {
			i++;
		}
		while (ppArg[i]->unitOffsetInPayload < x);

		if (i < j) {
			PCM_ARG tmpP = ppArg[i];
			ppArg[i] = ppArg[j];
			ppArg[j] = tmpP;
		} else {
			return j;
		}
	}
}

BOOL _CheckSurfaceType(int nType, ...)
{
	BOOL bMatch = FALSE;
	va_list ap;
	va_start(ap, nType);
	int iType = 0;

	while ((iType = va_arg(ap, int)) >= 0) {
		if (iType == nType) {
			bMatch = TRUE;
			break;
		}
	}
	va_end(ap);

	return bMatch;
}

void QuickSort(PCM_ARG * ppArg, INT p, INT r)
{
	if (p < r) {
		INT q = Partition(ppArg, p, r);
		QuickSort(ppArg, p, q);
		QuickSort(ppArg, q + 1, r);
	}
}

INT CmKernel::Create(CmDevice * pCmDev, CmProgram * pProgram,
		     const char *kernelName, UINT KernelIndex,
		     UINT KernelSeqNum, CmKernel * &pKernel,
		     const char *options)
{
	INT result = CM_SUCCESS;
	pKernel =
	    new(std::nothrow) CmKernel(pCmDev, pProgram, KernelIndex,
				       KernelSeqNum);
	if (pKernel) {
		pKernel->Acquire();
		result = pKernel->Initialize(kernelName, options);
		if (result != CM_SUCCESS) {
			CmKernel::Destroy(pKernel, pProgram);
			return result;
		}
	} else {
		CM_ASSERT(0);
		return CM_OUT_OF_HOST_MEMORY;
	}

	return result;
}

INT CmKernel::Destroy(CmKernel * &pKernel, CmProgram * &pProgram)
{
	UINT refCount = pKernel->SafeRelease();
	if (refCount == 0) {
		pKernel = NULL;
	}

	refCount = pProgram->SafeRelease();
	if (refCount == 0) {
		pProgram = NULL;
	}
	return CM_SUCCESS;
}

INT CmKernel::Acquire(void)
{
	m_refcount++;
	return m_refcount;
}

INT CmKernel::SafeRelease(void)
{
	--m_refcount;
	if (m_refcount == 0) {
		delete this;
		return 0;
	}
	return m_refcount;
}

 CmKernel::CmKernel(CmDevice * pCmDev, CmProgram * pProgram, UINT KernelIndex, UINT KernelSeqNum):
m_pCmDev(pCmDev),
m_pProgram(pProgram),
m_Options(NULL),
m_pBinary(NULL),
m_uiBinarySize(0),
m_ThreadCount(0),
m_LastThreadCount(0),
m_SizeInCurbe(0),
m_SizeInPayload(0),
m_ArgCount(0),
m_Args(NULL),
m_pKernelInfo(NULL),
m_kernelIndexInProgram(CM_INVALID_KERNEL_INDEX),
m_CurbeEnable(TRUE),
m_NonstallingScoreboardEnable(FALSE),
m_Dirty(CM_KERNEL_DATA_CLEAN),
m_pLastKernelData(NULL),
m_LastKernelDataSize(0),
m_IndexInTask(0),
m_AssociatedToTS(FALSE),
m_blPerThreadArgExists(FALSE),
m_blPerKernelArgExists(FALSE),
m_pThreadSpace(NULL),
m_adjustScoreboardY(0),
m_LastAdjustScoreboardY(0),
m_usKernelPayloadDataSize(0),
m_pKernelPayloadData(NULL),
m_usKernelPayloadSurfaceCount(0),
m_refcount(0),
m_pHalMaxValues(NULL),
m_pHalMaxValuesEx(NULL), m_SurfaceArray(NULL), m_pThreadGroupSpace(NULL)
{
	pProgram->Acquire();
	m_pProgram = pProgram;

	m_Id = KernelSeqNum;
	m_Id <<= 32;
	m_kernelIndex = KernelIndex;

	for (int i = 0; i < CM_GLOBAL_SURFACE_NUMBER; i++) {
		m_GlobalSurfaces[i] = NULL;
		m_GlobalCmIndex[i] = 0;
	}

	m_blhwDebugEnable = pProgram->IsHwDebugEnabled();

	CmSafeMemSet(m_pKernelPayloadSurfaceArray, 0,
		     sizeof(m_pKernelPayloadSurfaceArray));
	CmSafeMemSet(m_IndirectSurfaceInfoArray, 0,
		     sizeof(m_IndirectSurfaceInfoArray));
	ResetKernelSurfaces();
}

CmKernel::~CmKernel(void)
{
	CmSafeDeleteArray(m_Options);

	DestroyArgs();

	if (m_pLastKernelData) {
		CmKernelData::Destroy(m_pLastKernelData);
	}

	if (CM_INVALID_KERNEL_INDEX != m_kernelIndexInProgram) {
		m_pProgram->ReleaseKernelInfo(m_kernelIndexInProgram);
	}

	for (int i = 0; i < CM_GLOBAL_SURFACE_NUMBER; i++) {
		SurfaceIndex *pSurfIndex = m_GlobalSurfaces[i];
		CmSafeDelete(pSurfIndex);
	}

	CmSafeDeleteArray(m_pKernelPayloadData);
	CmSafeDeleteArray(m_SurfaceArray);
}

INT CmKernel::Initialize(const char *kernelName, const char *options)
{
	if (kernelName == NULL) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	size_t length = strnlen(kernelName, CM_MAX_KERNEL_NAME_SIZE_IN_BYTE);
	if (length >= CM_MAX_KERNEL_NAME_SIZE_IN_BYTE) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	UINT kernelCount = 0;
	m_pProgram->GetKernelCount(kernelCount);

	CM_KERNEL_INFO *kernelInfo = NULL;
	UINT i = 0;
	for (i = 0; i < kernelCount; i++) {
		m_pProgram->GetKernelInfo(i, kernelInfo);
		if (!kernelInfo) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}
		if (strcmp(kernelInfo->kernelName, kernelName) == 0) {
			break;
		}
	}

	if (i == kernelCount) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	m_pCmDev->GetHalMaxValues(m_pHalMaxValues, m_pHalMaxValuesEx);

	m_pProgram->AcquireKernelInfo(i);
	m_pKernelInfo = kernelInfo;
	m_kernelIndexInProgram = i;

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

			char *pTmp = strstr(m_Options, "nocurbe");
			if (pTmp) {
				m_CurbeEnable = FALSE;
			}
		}
	}

	if ((m_pProgram->m_CISA_majorVersion > 2)
	    || (m_pProgram->m_CISA_majorVersion >= 2
		&& m_pProgram->m_CISA_minorVersion >= 2)) {
		m_NonstallingScoreboardEnable = TRUE;
	} else {
		m_NonstallingScoreboardEnable = FALSE;
	}

	void *pCommonISACode = NULL;
	UINT commonISACodeSize = 0;
	m_pProgram->GetCommonISACode(pCommonISACode, commonISACodeSize);
	if ((pCommonISACode == NULL) || (commonISACodeSize <= 0)) {
		CM_ASSERT(0);
		return CM_INVALID_COMMON_ISA;
	}
	BYTE *buf = (BYTE *) pCommonISACode;
	UINT byte_pos = m_pKernelInfo->kernelIsaOffset;

	UINT kernelInfoRefCount = 0;
	m_pProgram->GetKernelInfoRefCount(m_kernelIndexInProgram,
					  kernelInfoRefCount);
	if (kernelInfoRefCount <= 2) {
		READ_FIELD_FROM_BUF(m_pKernelInfo->globalStringCount,
				    unsigned short);
		m_pKernelInfo->globalStrings =
		    (const char **)malloc(m_pKernelInfo->globalStringCount *
					  sizeof(char *));
		if (m_pKernelInfo->globalStrings == NULL) {
			CM_ASSERT(0);
			return CM_OUT_OF_HOST_MEMORY;
		}
		CmSafeMemSet(m_pKernelInfo->globalStrings, 0,
			     m_pKernelInfo->globalStringCount * sizeof(char *));
		for (int i = 0; i < m_pKernelInfo->globalStringCount; i++) {
			char *string =
			    (char *)malloc(CM_MAX_KERNEL_STRING_IN_BYTE + 1);
			if (string == NULL) {
				CM_ASSERT(0);
				return CM_OUT_OF_HOST_MEMORY;
			}
			int j = 0;
			while (buf[byte_pos] != '\0'
			       && j < CM_MAX_KERNEL_STRING_IN_BYTE) {
				string[j++] = buf[byte_pos++];
			}
			string[j] = '\0';
			byte_pos++;
			m_pKernelInfo->globalStrings[i] = string;
		}

		byte_pos += sizeof(WORD);

		READ_FIELD_FROM_BUF(m_pKernelInfo->variable_count, WORD);
		for (UINT i = 0; i < m_pKernelInfo->variable_count; i++) {
			gen_var_info_t g_v;
			attribute_info_t a;

			READ_FIELD_FROM_BUF(g_v.name_index, WORD);
			READ_FIELD_FROM_BUF(g_v.bit_properties, BYTE);
			READ_FIELD_FROM_BUF(g_v.num_elements, WORD);
			READ_FIELD_FROM_BUF(g_v.alias_index, WORD);
			READ_FIELD_FROM_BUF(g_v.alias_offset, WORD);

			if (m_pProgram->m_CISA_majorVersion >= 3)
				byte_pos += sizeof(BYTE);

			READ_FIELD_FROM_BUF(g_v.attribute_count, BYTE);
			for (UINT k = 0; k < g_v.attribute_count; k++) {
				READ_FIELD_FROM_BUF(a.name_index, WORD);
				READ_FIELD_FROM_BUF(a.size, BYTE);
				byte_pos += a.size;
			}
		}

		READ_FIELD_FROM_BUF(m_pKernelInfo->address_count, WORD);
		for (UINT i = 0; i < m_pKernelInfo->address_count; i++) {
			spec_var_info_t s_v;
			attribute_info_t a;
			READ_FIELD_FROM_BUF(s_v.name_index, WORD);
			READ_FIELD_FROM_BUF(s_v.num_elements, WORD);
			READ_FIELD_FROM_BUF(s_v.attribute_count, BYTE);
			for (UINT k = 0; k < s_v.attribute_count; k++) {
				READ_FIELD_FROM_BUF(a.name_index, WORD);
				READ_FIELD_FROM_BUF(a.size, BYTE);
				byte_pos += a.size;
			}
		}

		READ_FIELD_FROM_BUF(m_pKernelInfo->predicte_count, WORD);
		for (UINT i = 0; i < m_pKernelInfo->predicte_count; i++) {
			spec_var_info_t s_v;
			attribute_info_t a;
			READ_FIELD_FROM_BUF(s_v.name_index, WORD);
			READ_FIELD_FROM_BUF(s_v.num_elements, WORD);
			READ_FIELD_FROM_BUF(s_v.attribute_count, BYTE);
			for (UINT k = 0; k < s_v.attribute_count; k++) {
				READ_FIELD_FROM_BUF(a.name_index, WORD);
				READ_FIELD_FROM_BUF(a.size, BYTE);
				byte_pos += a.size;
			}
		}

		READ_FIELD_FROM_BUF(m_pKernelInfo->label_count, WORD);
		for (UINT i = 0; i < m_pKernelInfo->label_count; i++) {
			label_info_t s_v;
			attribute_info_t a;
			READ_FIELD_FROM_BUF(s_v.name_index, WORD);
			READ_FIELD_FROM_BUF(s_v.kind, BYTE);
			READ_FIELD_FROM_BUF(s_v.attribute_count, BYTE);
			for (UINT k = 0; k < s_v.attribute_count; k++) {
				READ_FIELD_FROM_BUF(a.name_index, WORD);
				READ_FIELD_FROM_BUF(a.size, BYTE);
				byte_pos += a.size;
			}
		}

		if ((m_pProgram->m_CISA_majorVersion > 2)
		    || (m_pProgram->m_CISA_majorVersion >= 2
			&& m_pProgram->m_CISA_minorVersion >= 1)) {

			READ_FIELD_FROM_BUF(m_pKernelInfo->surface_count, BYTE);
			if (m_pKernelInfo->surface_count) {
				m_pKernelInfo->surface = (spec_var_info_t *)
				    malloc(sizeof(spec_var_info_t) *
					   m_pKernelInfo->surface_count);
				if (m_pKernelInfo->surface == NULL) {
					CM_ASSERT(0);
					return CM_OUT_OF_HOST_MEMORY;
				}

				CmSafeMemSet(m_pKernelInfo->surface, 0,
					     sizeof(spec_var_info_t) *
					     m_pKernelInfo->surface_count);
				for (UINT i = 0;
				     i < m_pKernelInfo->surface_count; i++) {
					READ_FIELD_FROM_BUF
					    (m_pKernelInfo->surface
					     [i].name_index, WORD);
					READ_FIELD_FROM_BUF
					    (m_pKernelInfo->surface
					     [i].num_elements, WORD);
					READ_FIELD_FROM_BUF
					    (m_pKernelInfo->surface[i].
					     attribute_count, BYTE);
					if (m_pKernelInfo->
					    surface[i].attribute_count) {
						m_pKernelInfo->
						    surface[i].attributes =
						    (attribute_info_t *)
						    malloc(sizeof
							   (attribute_info_t)
							   *
							   m_pKernelInfo->
							   surface[i].
							   attribute_count);
						CmSafeMemSet
						    (m_pKernelInfo->surface
						     [i].attributes, 0,
						     sizeof(attribute_info_t)
						     *
						     m_pKernelInfo->surface[i].
						     attribute_count);
						for (UINT k = 0;
						     k <
						     m_pKernelInfo->
						     surface[i].attribute_count;
						     k++) {
							READ_FIELD_FROM_BUF
							    (m_pKernelInfo->surface
							     [i].
							     attributes
							     [k].name_index,
							     WORD);
							READ_FIELD_FROM_BUF
							    (m_pKernelInfo->surface
							     [i].
							     attributes[k].size,
							     BYTE);
							CM_ASSERT
							    (m_pKernelInfo->surface
							     [i].attributes[k].
							     size == 1);
							m_pKernelInfo->surface
							    [i].attributes
							    [k].values =
							    (unsigned char
							     *)((unsigned
								 long)((UINT) ((*(BYTE *) & buf[byte_pos]))));
							byte_pos +=
							    sizeof(BYTE);
						}
					}
				}
			}
		}
	}

	byte_pos = m_pKernelInfo->inputCountOffset;

	BYTE count;
	READ_FIELD_FROM_BUF(count, BYTE);

	if (count > m_pHalMaxValues->iMaxArgsPerKernel) {
		CM_ASSERT(0);
		return CM_EXCEED_KERNEL_ARG_AMOUNT;
	}

	m_Args = new(std::nothrow) CM_ARG[count];
	if ((!m_Args) && (count != 0)) {
		CM_ASSERT(0);
		CmSafeDeleteArray(m_Options);
		return CM_OUT_OF_HOST_MEMORY;
	}
	CmSafeMemSet(m_Args, 0, sizeof(CM_ARG) * count);
	m_ArgCount = count;

	UINT preDefinedSurfNum;
	if ((m_pProgram->m_CISA_majorVersion > 3)
	    || ((m_pProgram->m_CISA_majorVersion == 3)
		&& (m_pProgram->m_CISA_minorVersion >= 1))) {
		preDefinedSurfNum = COMMON_ISA_NUM_PREDEFINED_SURF_VER_3_1;
	} else if (((m_pProgram->m_CISA_majorVersion == 3)
		    && (m_pProgram->m_CISA_minorVersion == 0))
		   || ((m_pProgram->m_CISA_majorVersion == 2)
		       && (m_pProgram->m_CISA_minorVersion >= 1))) {
		preDefinedSurfNum = COMMON_ISA_NUM_PREDEFINED_SURF_VER_2_1;
	} else {
		preDefinedSurfNum = COMMON_ISA_NUM_PREDEFINED_SURF_VER_2;
	}

	UINT argSize = 0;
	UINT kernelSurfaceCount = 0;
	for (UINT i = 0; i < m_ArgCount; i++) {
		BYTE kind;
		READ_FIELD_FROM_BUF(kind, BYTE);
		if (kind == 0x2) {
			kind = ARG_KIND_SURFACE;

			kernelSurfaceCount++;
		}

		m_Args[i].unitKind = kind;
		m_Args[i].unitKindOrig = kind;

		WORD tmpW;
		READ_FIELD_FROM_BUF(tmpW, WORD);

		if (kind == ARG_KIND_SURFACE && m_pKernelInfo->surface_count) {
			if (m_pKernelInfo->surface[tmpW - preDefinedSurfNum].
			    attribute_count) {
				m_Args[i].s_k =
				    (SURFACE_KIND) (UINT) (unsigned long)
				    m_pKernelInfo->surface[tmpW -
							   preDefinedSurfNum].attributes
				    [0].values;
			} else {
				m_Args[i].s_k = DATA_PORT_SURF;
			}
		}

		READ_FIELD_FROM_BUF(tmpW, WORD);
		m_Args[i].unitOffsetInPayload = tmpW;
		m_Args[i].unitOffsetInPayloadOrig = tmpW;

		READ_FIELD_FROM_BUF(tmpW, WORD);
		m_Args[i].unitSize = tmpW;
		m_Args[i].unitSizeOrig = tmpW;

		argSize += tmpW;
	}

	if (kernelInfoRefCount <= 2) {
		byte_pos += 8;

		WORD attribute_count;
		READ_FIELD_FROM_BUF(attribute_count, WORD);
		for (int i = 0; i < attribute_count; i++) {
			WORD name_index;
			BYTE size;
			READ_FIELD_FROM_BUF(name_index, WORD);
			READ_FIELD_FROM_BUF(size, BYTE);
			if (strcmp
			    (m_pKernelInfo->globalStrings[name_index],
			     "AsmName") == 0) {
				CmFastMemCopy(m_pKernelInfo->kernelASMName,
					      &buf[byte_pos], size);
				byte_pos += size;
			} else
			    if (strcmp
				(m_pKernelInfo->globalStrings[name_index],
				 "SLMSize")
				== 0) {
				READ_FIELD_FROM_BUF
				    (m_pKernelInfo->kernelSLMSize, BYTE);

				if ((m_pProgram->m_CISA_majorVersion < 3)
				    || ((m_pProgram->m_CISA_majorVersion == 3)
					&& (m_pProgram->m_CISA_minorVersion <=
					    1))) {
					m_pKernelInfo->kernelSLMSize =
					    m_pKernelInfo->kernelSLMSize * 4;
				}
			} else {
				byte_pos += size;
			}
		}
	}

	if (argSize > m_pHalMaxValues->iMaxArgByteSizePerKernel) {
		CM_ASSERT(0);
		return CM_EXCEED_KERNEL_ARG_SIZE_IN_BYTE;
	}

	buf = (BYTE *) pCommonISACode;

	if (m_pProgram->IsJitterEnabled()) {
		char *programOptions;
		m_pProgram->GetKernelOptions(programOptions);
		{
			m_pBinary = (char *)m_pKernelInfo->jitBinaryCode;
			m_uiBinarySize = m_pKernelInfo->jitBinarySize;
			m_pKernelInfo->pOrigBinary =
			    m_pKernelInfo->jitBinaryCode;
			m_pKernelInfo->uiOrigBinarySize =
			    m_pKernelInfo->jitBinarySize;
		}
	} else {
		char *pBinary = (char *)(buf + m_pKernelInfo->genxBinaryOffset);

		m_pBinary = pBinary;
		m_uiBinarySize = m_pKernelInfo->genxBinarySize;

		m_pKernelInfo->pOrigBinary = pBinary;
		m_pKernelInfo->uiOrigBinarySize = m_pKernelInfo->genxBinarySize;
	}

	return CM_SUCCESS;
}

INT CmKernel::GetBinary(void *&pBinary, UINT & size)
{
	pBinary = m_pBinary;

	size = m_uiBinarySize;

	return CM_SUCCESS;
}

CM_RT_API INT CmKernel::SetThreadCount(UINT count)
{
	if ((int)count <= 0)
		return CM_INVALID_ARG_VALUE;

	if (m_ThreadCount) {
		if (m_ThreadCount != count) {
			Reset();
			m_ThreadCount = count;
			m_Dirty |= CM_KERNEL_DATA_THREAD_COUNT_DIRTY;
		}
	} else {
		m_ThreadCount = count;
	}

	return CM_SUCCESS;
}

INT CmKernel::GetThreadCount(UINT & count)
{
	count = m_ThreadCount;
	return CM_SUCCESS;
}

INT CmKernel::GetKernelSurfaces(BOOL * &surfArray)
{
	surfArray = m_SurfaceArray;
	return CM_SUCCESS;
}

INT CmKernel::ResetKernelSurfaces()
{
	CmSurfaceManager *pSurfaceMgr = NULL;
	m_pCmDev->GetSurfaceManager(pSurfaceMgr);
	if (!pSurfaceMgr) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}
	UINT surfacePoolSize = pSurfaceMgr->GetSurfacePoolSize();
	if (!m_SurfaceArray) {
		m_SurfaceArray = new(std::nothrow) BOOL[surfacePoolSize];
		if (!m_SurfaceArray) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}
	}
	CmSafeMemSet(m_SurfaceArray, 0, surfacePoolSize * sizeof(BOOL));

	return CM_SUCCESS;
}

#define SET_MEMORY_OBJECT_CONTROL(x, memCtl) \
           x = ((WORD)(memCtl.mem_ctrl<< 8 | memCtl.mem_type << 4 | memCtl.age)) << 16 | (x);

INT CmKernel::SetArgsInternal(CM_KERNEL_INTERNAL_ARG_TYPE nArgType, UINT index,
			      size_t size, const void *pValue, UINT nThreadID)
{
	UINT surfRegTableIndex = 0;
	UINT handle = 0;
	CmSurfaceManager *pSurfaceMgr = NULL;
	WORD *pSurfIndexValue = NULL;
	UINT surfaces[CM_MAX_ARGS_PER_KERNEL];
	WORD surfIndexArray[CM_MAX_ARGS_PER_KERNEL];

	m_pCmDev->GetSurfaceManager(pSurfaceMgr);
	if (!pSurfaceMgr) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}
	pSurfaceMgr->GetSurfacePoolSize();

	m_Args[index].bIsSet = FALSE;

	if (m_Args[index].unitKind == ARG_KIND_GENERAL) {
		if (size != m_Args[index].unitSize) {
			CM_ASSERT(0);
			return CM_INVALID_ARG_SIZE;
		}
	} else if (CHECK_SURFACE_TYPE(m_Args[index].unitKind,
				      ARG_KIND_SURFACE,
				      ARG_KIND_SURFACE_1D,
				      ARG_KIND_SURFACE_2D,
				      ARG_KIND_SURFACE_2D_UP,
				      ARG_KIND_SURFACE_2D_DUAL)) {
		int signature_size = m_Args[index].unitSize;
		int num_surfaces = signature_size / sizeof(int);
		SurfaceIndex *pSurfIndex = (SurfaceIndex *) pValue;
		CM_SURFACE_MEM_OBJ_CTRL memCtl;

		if (size != sizeof(SurfaceIndex) * num_surfaces) {
			CM_ASSERT(0);
			return CM_INVALID_ARG_SIZE;
		}

		UINT surfIndex = pSurfIndex->get_data();
		int i = 0;
		while (!surfIndex && (i < num_surfaces)) {
			surfaces[i] = CM_NULL_SURFACE;
			surfIndexArray[i] = 0;
			i++;
			if (i >= num_surfaces)
				break;
			surfIndex = pSurfIndex[i].get_data();
		}

		if (i >= num_surfaces) {
			m_Args[index].unitKind = ARG_KIND_SURFACE_2D;
			pValue = surfaces;
			size = (size / sizeof(SurfaceIndex)) * sizeof(UINT);
			m_Args[index].unitSize = (WORD) size;
			goto finish;
		}
		CmSurface *pSurface = NULL;
		pSurfaceMgr->GetSurface(surfIndex, pSurface);
		if (NULL == pSurface) {
			CM_ASSERT(0);
			return CM_FAILURE;
		}

		if (SurfTypeToArgKind(pSurface->Type()) !=
		    m_Args[index].unitKind) {
			m_Args[index].bIsDirty = TRUE;
			m_Dirty |= CM_KERNEL_DATA_KERNEL_ARG_DIRTY;
		}

		UINT CISA_majorVersion, CISA_minorVersion;
		m_pProgram->GetCISAVersion(CISA_majorVersion,
					   CISA_minorVersion);

		switch (pSurface->Type()) {
		case CM_ENUM_CLASS_TYPE_CMSURFACE2D:
			{
				CmSurface2D *pSurf2D =
				    static_cast < CmSurface2D * >(pSurface);
				if (CISA_majorVersion >= 2
				    && CISA_minorVersion >= 4) {
					if (m_Args[index].s_k == DUAL_SURF) {
						CM_ASSERTMESSAGE
						    ("Warning: Possible dual use of 2D surface.");
						return CM_INVALID_ARG_VALUE;
					}
				}

				while (i < num_surfaces) {
					pSurf2D->GetIndexFor2D
					    (surfRegTableIndex);
					pSurface->GetMemoryObjectCtrl(&memCtl);
					SET_MEMORY_OBJECT_CONTROL
					    (surfRegTableIndex, memCtl);

					surfaces[i] = surfRegTableIndex;
					surfIndexArray[i] = (WORD) surfIndex;
					i++;
					if (i >= num_surfaces)
						break;
					surfIndex = pSurfIndex[i].get_data();
					while (!surfIndex && (i < num_surfaces)) {
						surfaces[i] = CM_NULL_SURFACE;
						surfIndexArray[i] = 0;
						i++;
						if (i >= num_surfaces)
							break;
						surfIndex =
						    pSurfIndex[i].get_data();
					}

					if (i < num_surfaces) {
						pSurfaceMgr->GetSurface
						    (surfIndex, pSurface);
						pSurf2D =
						    static_cast <
						    CmSurface2D * >(pSurface);
					}

					if (pSurf2D == NULL) {
						CM_ASSERT(0);
						return CM_FAILURE;
					}
				}
				pValue = surfaces;
				pSurfIndexValue = surfIndexArray;

				size =
				    (size / sizeof(SurfaceIndex)) *
				    sizeof(UINT);
				m_Args[index].unitSize = (WORD) size;

				if ((m_Args[index].unitKind == ARG_KIND_SURFACE)
				    || (m_Args[index].unitKind ==
					ARG_KIND_SURFACE_2D_UP)) {
					m_Args[index].unitKind =
					    ARG_KIND_SURFACE_2D;
					if (m_Args[index].s_k == DUAL_SURF)
						m_Args[index].unitKind =
						    ARG_KIND_SURFACE_2D_DUAL;
				} else if (m_Args[index].unitKind !=
					   ARG_KIND_SURFACE_2D
					   && m_Args[index].unitKind !=
					   ARG_KIND_SURFACE_2D_DUAL) {
					CM_ASSERTMESSAGE
					    ("Assign a 2D surface to the arg which is previously assigned 1D surface.");
					return CM_INVALID_ARG_VALUE;
				}
				break;
			}
		case CM_ENUM_CLASS_TYPE_CMBUFFER_RT:
			{
				CmBuffer_RT *pSurf1D =
				    static_cast < CmBuffer_RT * >(pSurface);
				while (i < num_surfaces) {
					pSurf1D->GetHandle(handle);
					pSurface->GetMemoryObjectCtrl(&memCtl);
					SET_MEMORY_OBJECT_CONTROL(handle,
								  memCtl);
					surfaces[i] = handle;
					surfIndexArray[i] = (WORD) surfIndex;
					i++;
					if (i >= num_surfaces)
						break;
					surfIndex = pSurfIndex[i].get_data();

					while (!surfIndex) {
						surfaces[i] = CM_NULL_SURFACE;
						surfIndexArray[i] = 0;
						i++;
						if (i >= num_surfaces)
							break;
						surfIndex =
						    pSurfIndex[i].get_data();
					}

					if (i < num_surfaces) {
						pSurfaceMgr->GetSurface
						    (surfIndex, pSurface);
						pSurf1D =
						    static_cast <
						    CmBuffer_RT * >(pSurface);
					}

					if (pSurf1D == NULL) {
						CM_ASSERT(0);
						return CM_FAILURE;
					}
				}
				pValue = surfaces;
				pSurfIndexValue = surfIndexArray;

				size =
				    (size / sizeof(SurfaceIndex)) *
				    sizeof(UINT);
				m_Args[index].unitSize = (WORD) size;

				if (m_Args[index].unitKind == ARG_KIND_SURFACE) {
					m_Args[index].unitKind =
					    ARG_KIND_SURFACE_1D;
				} else if (m_Args[index].unitKind !=
					   ARG_KIND_SURFACE_1D) {
					CM_ASSERTMESSAGE
					    ("Assign a 1D surface to the arg which is previously assigned 2D surface.");
					return CM_INVALID_ARG_VALUE;
				}
				break;
			}
		case CM_ENUM_CLASS_TYPE_CMSURFACE2DUP:
			{
				CmSurface2DUP *pSurf2DUP =
				    static_cast < CmSurface2DUP * >(pSurface);
				int i = 0;

				while (i < num_surfaces) {
					pSurf2DUP->GetHandle(handle);
					pSurface->GetMemoryObjectCtrl(&memCtl);
					SET_MEMORY_OBJECT_CONTROL(handle,
								  memCtl);
					surfaces[i] = handle;
					surfIndexArray[i] = (WORD) surfIndex;
					i++;
					if (i >= num_surfaces)
						break;
					surfIndex = pSurfIndex[i].get_data();
					pSurfaceMgr->GetSurface(surfIndex,
								pSurface);
					pSurf2DUP =
					    static_cast <
					    CmSurface2DUP * >(pSurface);
					if (pSurf2DUP == NULL) {
						CM_ASSERT(0);
						return CM_FAILURE;
					}
				}
				pValue = surfaces;
				pSurfIndexValue = surfIndexArray;

				size =
				    (size / sizeof(SurfaceIndex)) *
				    sizeof(UINT);
				m_Args[index].unitSize = (WORD) size;

				if ((m_Args[index].unitKind == ARG_KIND_SURFACE)
				    || (m_Args[index].unitKind ==
					ARG_KIND_SURFACE_2D)) {
					m_Args[index].unitKind =
					    ARG_KIND_SURFACE_2D_UP;
				} else if (m_Args[index].unitKind !=
					   ARG_KIND_SURFACE_2D_UP) {
					CM_ASSERTMESSAGE
					    ("Assign a 2D surface UP to the arg which is previously assigned other surfaces.");
					return CM_INVALID_ARG_VALUE;
				}
				break;
			}
		default:
			{
				CM_ASSERT(0);
				return CM_INVALID_ARG_VALUE;
			}
		}
	}

 finish:
	if (nArgType == CM_KERNEL_INTERNEL_ARG_PERKERNEL) {
		CM_ARG & arg = m_Args[index];

		if (!arg.pValue) {
			UINT tempUnitSize = m_Args[index].unitSize;
			m_SizeInCurbe += tempUnitSize;

			arg.pValue = new(std::nothrow) BYTE[size];
			if (!arg.pValue) {
				CM_ASSERT(0);
				return CM_OUT_OF_HOST_MEMORY;
			}

			arg.unitCount = 1;

			CmFastMemCopy((void *)arg.pValue, pValue, size);

			if (((m_Args[index].unitKind == ARG_KIND_SURFACE) ||
			     (m_Args[index].unitKind == ARG_KIND_SURFACE_1D) ||
			     (m_Args[index].unitKind == ARG_KIND_SURFACE_2D) ||
			     (m_Args[index].unitKind == ARG_KIND_SURFACE_2D_UP)
			     || (m_Args[index].unitKind ==
				 ARG_KIND_SURFACE_2D_DUAL))
			    && pSurfIndexValue) {
				arg.surfIndex =
				    new(std::nothrow) WORD[size / sizeof(INT)];
				if (!arg.surfIndex) {
					CM_ASSERT(0);
					CmSafeDeleteArray(arg.pValue);
					return CM_FAILURE;
				}
				CmSafeMemSet((void *)arg.surfIndex, 0,
					     size / sizeof(INT) * sizeof(WORD));
				if (pSurfIndexValue == NULL) {
					CM_ASSERT(0);
					return CM_FAILURE;
				}
				CmFastMemCopy((void *)arg.surfIndex,
					      pSurfIndexValue,
					      size / sizeof(INT) *
					      sizeof(WORD));
			}

			m_Dirty |= CM_KERNEL_DATA_KERNEL_ARG_DIRTY;
			arg.bIsDirty = TRUE;
		} else {
			if (arg.unitCount != 1) {
				CM_ASSERT(0);
				return CM_FAILURE;
			}
			if (memcmp((void *)arg.pValue, pValue, size) != 0) {
				CmFastMemCopy((void *)arg.pValue, pValue, size);
				m_Dirty |= CM_KERNEL_DATA_KERNEL_ARG_DIRTY;
				arg.bIsDirty = TRUE;
			}
			if (((m_Args[index].unitKind == ARG_KIND_SURFACE) ||
			     (m_Args[index].unitKind == ARG_KIND_SURFACE_1D) ||
			     (m_Args[index].unitKind == ARG_KIND_SURFACE_2D) ||
			     (m_Args[index].unitKind == ARG_KIND_SURFACE_2D_UP)
			     || (m_Args[index].unitKind ==
				 ARG_KIND_SURFACE_2D_DUAL))
			    && pSurfIndexValue) {
				CmSafeMemSet((void *)arg.surfIndex, 0,
					     size / sizeof(INT) * sizeof(WORD));
				if (pSurfIndexValue == NULL) {
					CM_ASSERT(0);
					return CM_FAILURE;
				}
				CmFastMemCopy((void *)arg.surfIndex,
					      pSurfIndexValue,
					      size / sizeof(INT) *
					      sizeof(WORD));
			}
		}

		m_blPerKernelArgExists = TRUE;
	} else {
		CM_ARG & arg = m_Args[index];

		if (!arg.pValue) {
			m_SizeInPayload += arg.unitSize;
			DW_ALIGNMENT(m_SizeInPayload);

			arg.pValue =
			    new(std::nothrow) BYTE[size * m_ThreadCount];
			CmSafeMemSet((void *)arg.pValue, 0,
				     size * m_ThreadCount);
			if (!arg.pValue) {
				CM_ASSERT(0);
				return CM_OUT_OF_HOST_MEMORY;

			}
			arg.unitCount = m_ThreadCount;

			UINT offset = size * nThreadID;
			BYTE *pThreadValue = (BYTE *) arg.pValue;
			pThreadValue += offset;

			CmFastMemCopy(pThreadValue, pValue, size);
			if (((m_Args[index].unitKind == ARG_KIND_SURFACE) ||
			     (m_Args[index].unitKind == ARG_KIND_SURFACE_1D) ||
			     (m_Args[index].unitKind == ARG_KIND_SURFACE_2D) ||
			     (m_Args[index].unitKind == ARG_KIND_SURFACE_2D_UP)
			     || (m_Args[index].unitKind ==
				 ARG_KIND_SURFACE_2D_DUAL))
			    && pSurfIndexValue) {
				arg.surfIndex =
				    new(std::nothrow) WORD[size / sizeof(UINT) *
							   m_ThreadCount];
				if (!arg.surfIndex) {
					CM_ASSERT(0);
					CmSafeDeleteArray(arg.pValue);
					return CM_OUT_OF_HOST_MEMORY;
				}
				CmSafeMemSet((void *)arg.surfIndex, 0,
					     size / sizeof(UINT) *
					     sizeof(WORD) * m_ThreadCount);
				if (pSurfIndexValue == NULL) {
					CM_ASSERT(0);
					return CM_FAILURE;
				}
				CmFastMemCopy((void *)(arg.surfIndex +
						       size / sizeof(UINT) *
						       nThreadID),
					      pSurfIndexValue,
					      size / sizeof(UINT) *
					      sizeof(WORD));
			}
			m_blPerThreadArgExists = TRUE;
		} else {
			if (arg.unitCount != m_ThreadCount) {
				CM_ASSERT(0);
				return CM_FAILURE;

			}
			UINT offset = size * nThreadID;
			BYTE *pThreadValue = (BYTE *) arg.pValue;
			pThreadValue += offset;

			if (memcmp(pThreadValue, pValue, size) != 0) {
				CmFastMemCopy(pThreadValue, pValue, size);
				m_Dirty |= CM_KERNEL_DATA_THREAD_ARG_DIRTY;
				arg.bIsDirty = TRUE;
			}
			if (((m_Args[index].unitKind == ARG_KIND_SURFACE) ||
			     (m_Args[index].unitKind == ARG_KIND_SURFACE_1D) ||
			     (m_Args[index].unitKind == ARG_KIND_SURFACE_2D) ||
			     (m_Args[index].unitKind == ARG_KIND_SURFACE_2D_UP)
			     || (m_Args[index].unitKind ==
				 ARG_KIND_SURFACE_2D_DUAL))
			    && pSurfIndexValue) {
				if (pSurfIndexValue == NULL) {
					CM_ASSERT(0);
					return CM_FAILURE;
				}
				CmFastMemCopy((void *)(arg.surfIndex +
						       size / sizeof(UINT) *
						       nThreadID),
					      pSurfIndexValue,
					      size / sizeof(UINT) *
					      sizeof(WORD));
			}
		}
	}

	m_Args[index].bIsSet = TRUE;

	return CM_SUCCESS;
}

CM_RT_API INT
    CmKernel::SetKernelArg(UINT index, size_t size, const void *pValue)
{
	if (m_pKernelPayloadData) {
		CM_ASSERT(0);
		return CM_KERNELPAYLOAD_PERKERNELARG_MUTEX_FAIL;
	}

	if (index >= m_ArgCount) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_INDEX;

	}

	if (!pValue) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	if (size == 0) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_SIZE;
	}

	INT nRetVal = 0;
	if ((nRetVal =
	     SetArgsInternal(CM_KERNEL_INTERNEL_ARG_PERKERNEL, index, size,
			     pValue)) != CM_SUCCESS) {
		return nRetVal;
	}

	return CM_SUCCESS;
}

CM_RT_API INT CmKernel::SetStaticBuffer(UINT index, const void *pValue)
{
	if (index >= CM_GLOBAL_SURFACE_NUMBER) {
		CM_ASSERT(0);
		return CM_INVALID_GLOBAL_BUFFER_INDEX;
	}

	if (!pValue) {
		CM_ASSERT(0);
		return CM_INVALID_BUFFER_HANDLER;
	}

	SurfaceIndex *pSurfIndex = (SurfaceIndex *) pValue;
	UINT surfIndex = pSurfIndex->get_data();
	UINT handle = 0;
	CmSurfaceManager *pSurfaceMgr = NULL;
	m_pCmDev->GetSurfaceManager(pSurfaceMgr);
	if (!pSurfaceMgr) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}
	CmSurface *pSurface = NULL;
	pSurfaceMgr->GetSurface(surfIndex, pSurface);
	if (pSurface == NULL) {
		CM_ASSERT(0);
		return CM_INVALID_BUFFER_HANDLER;
	}

	CmBuffer_RT *pSurf1D = NULL;
	if (pSurface->Type() == CM_ENUM_CLASS_TYPE_CMBUFFER_RT) {
		CM_SURFACE_MEM_OBJ_CTRL memCtl;
		pSurf1D = static_cast < CmBuffer_RT * >(pSurface);
		pSurf1D->GetHandle(handle);
		pSurface->GetMemoryObjectCtrl(&memCtl);
		SET_MEMORY_OBJECT_CONTROL(handle, memCtl);
		m_GlobalSurfaces[index] = new(std::nothrow) SurfaceIndex(0);
		if (!m_GlobalSurfaces[index]) {
			CM_ASSERT(0);
			return CM_OUT_OF_HOST_MEMORY;
		}
		*m_GlobalSurfaces[index] = handle;
		m_GlobalCmIndex[index] = surfIndex;
		m_Dirty |= CM_KERNEL_DATA_GLOBAL_SURFACE_DIRTY;
	} else {
		CM_ASSERT(0);
		return CM_INVALID_BUFFER_HANDLER;
	}
	return CM_SUCCESS;
}

CM_RT_API INT
    CmKernel::SetThreadArg(UINT threadId, UINT index, size_t size,
			   const void *pValue)
{
	if (m_pKernelPayloadData) {
		CM_ASSERTMESSAGE
		    ("SetThredArg should be mutual exclusive with indirect data.");
		return CM_KERNELPAYLOAD_PERTHREADARG_MUTEX_FAIL;
	}

	if (m_ThreadCount > m_pHalMaxValues->iMaxUserThreadsPerTask
	    || m_ThreadCount <= 0) {
		CM_ASSERTMESSAGE
		    ("Minimum or Maximum number of threads exceeded.");
		return CM_FAILURE;
	}

	if (index >= m_ArgCount) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_INDEX;

	}

	if (threadId >= m_ThreadCount) {
		CM_ASSERT(0);
		return CM_INVALID_THREAD_INDEX;

	}

	if (!pValue) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	if (size == 0) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_SIZE;
	}

	INT nRetVal = 0;
	if ((nRetVal =
	     SetArgsInternal(CM_KERNEL_INTERNEL_ARG_PERTHREAD, index, size,
			     pValue, threadId)) != CM_SUCCESS) {
		return nRetVal;
	}

	return CM_SUCCESS;
}

INT CmKernel::CalcKernelDataSize(UINT MovInsNum,
				 UINT NumArgs,
				 UINT ArgSize, UINT & TotalKernelDataSize)
{
	INT hr = CM_SUCCESS;

	UINT headSize =
	    (KERNEL_INFO_SIZE_IN_DWORD +
	     NumArgs * PER_ARG_SIZE_IN_DWORD) * sizeof(UINT);
	UINT totalSize =
	    headSize + MovInsNum * CM_MOVE_INSTRUCTION_SIZE + m_uiBinarySize +
	    ArgSize;

	totalSize += 4;
	totalSize += 8;

	totalSize += 16;
	totalSize += 12;

	totalSize += sizeof(WORD);
	if (m_usKernelPayloadDataSize) {
		totalSize += m_usKernelPayloadDataSize;
	}
	totalSize += sizeof(WORD);
	if (m_usKernelPayloadSurfaceCount) {
		totalSize +=
		    m_usKernelPayloadSurfaceCount *
		    sizeof(CM_INDIRECT_SURFACE_INFO);
	}

	TotalKernelDataSize = totalSize;

	return hr;
}

INT CmKernel::CreateMovInstructions(UINT & movInstNum, PBYTE & pCodeDst,
				    CM_ARG * pTempArgs, UINT NumArgs)
{
	CmDynamicArray movInsts(NumArgs);
	UINT platform = IGFX_UNKNOWN_CORE;

	movInstNum = 0;

	if (m_CurbeEnable && (m_blPerThreadArgExists || m_blPerKernelArgExists)) {
		m_pCmDev->GetGenPlatform(platform);

		if ((m_ArgCount > 0) && (m_ThreadCount > 1)) {
			PCM_ARG *ppArgSorted =
			    new(std::nothrow) PCM_ARG[NumArgs];
			if (!ppArgSorted) {
				CM_ASSERT(0);
				return CM_OUT_OF_HOST_MEMORY;
			}
			for (UINT j = 0; j < NumArgs; j++) {
				ppArgSorted[j] = pTempArgs + j;
			}
			QuickSort(ppArgSorted, 0, NumArgs - 1);

			WORD *unitOffsetInPayloadSorted =
			    new(std::nothrow) WORD[NumArgs];
			if (!unitOffsetInPayloadSorted) {
				CM_ASSERT(0);
				CmSafeDeleteArray(ppArgSorted);
				return CM_OUT_OF_HOST_MEMORY;
			}
			for (UINT j = 0; j < NumArgs; j++) {
				unitOffsetInPayloadSorted[j] =
				    ppArgSorted[j]->unitOffsetInPayload;
			}

			WORD kernelArgEnd = 32;
			bool beforeFirstThreadArg = true;
			for (UINT j = 0; j < NumArgs; j++) {
				if (ppArgSorted[j]->unitCount == 1) {
					if (beforeFirstThreadArg) {
						kernelArgEnd =
						    ppArgSorted
						    [j]->unitOffsetInPayload +
						    ppArgSorted[j]->unitSize;
					} else {
						DW_ALIGNMENT(kernelArgEnd);
						ppArgSorted
						    [j]->unitOffsetInPayload =
						    kernelArgEnd;
						kernelArgEnd +=
						    ppArgSorted[j]->unitSize;
					}
				} else {
					if (beforeFirstThreadArg) {
						beforeFirstThreadArg = false;
					}
				}
			}

			GRF_ALIGNMENT(kernelArgEnd);
			UINT threadArgStart = kernelArgEnd;

			for (UINT j = 0; j < NumArgs; j++) {
				if (ppArgSorted[j]->unitCount > 1) {
					ppArgSorted[j]->unitOffsetInPayload =
					    (WORD) threadArgStart;
					threadArgStart +=
					    ppArgSorted[j]->unitSize;
					DW_ALIGNMENT(threadArgStart);
				}
			}

			GRF_ALIGNMENT(threadArgStart);
			UINT threadArgEnd = threadArgStart;
			UINT size = threadArgEnd - 32;
			CM_ASSERT((size % 32) == 0);

			UINT nextIndex = 0;
			bool bdw = platform == IGFX_GEN8_CORE;

			nextIndex +=
			    MovInst_RT::CreateMoves(R64_OFFSET, 32, size,
						    movInsts, nextIndex,
						    bdw, m_blhwDebugEnable);

			beforeFirstThreadArg = true;
			for (UINT j = 0; j < NumArgs; j++) {
				if (ppArgSorted[j]->unitCount == 1) {
					if (beforeFirstThreadArg == false) {
						nextIndex +=
						    MovInst_RT::CreateMoves
						    (unitOffsetInPayloadSorted
						     [j],
						     R64_OFFSET +
						     ppArgSorted
						     [j]->unitOffsetInPayload -
						     32,
						     ppArgSorted[j]->unitSize,
						     movInsts, nextIndex,
						     bdw, m_blhwDebugEnable);
					}

				} else {
					if (beforeFirstThreadArg) {
						beforeFirstThreadArg = false;
					}
					nextIndex +=
					    MovInst_RT::CreateMoves
					    (unitOffsetInPayloadSorted[j],
					     R64_OFFSET +
					     ppArgSorted[j]->unitOffsetInPayload
					     - CM_PAYLOAD_OFFSET,
					     ppArgSorted[j]->unitSize, movInsts,
					     nextIndex, bdw, m_blhwDebugEnable);

				}
			}

			movInstNum = nextIndex;

			CmSafeDeleteArray(ppArgSorted);
			CmSafeDeleteArray(unitOffsetInPayloadSorted);
		}
	}

	DWORD addInstDW[4];
	GENOS_ZeroMemory(addInstDW, CM_MOVE_INSTRUCTION_SIZE);
	UINT addInstNum = 0;

	if (m_pThreadSpace && m_adjustScoreboardY) {
		addInstNum = 1;

		if (platform == IGFX_GEN7_5_CORE) {
			addInstDW[0] = CM_IVB_HSW_ADJUST_Y_SCOREBOARD_DW0;
			addInstDW[1] = CM_IVB_HSW_ADJUST_Y_SCOREBOARD_DW1;
			addInstDW[2] = CM_IVB_HSW_ADJUST_Y_SCOREBOARD_DW2;

		} else if (platform == IGFX_GEN8_CORE) {
			addInstDW[0] = CM_BDW_ADJUST_Y_SCOREBOARD_DW0;
			addInstDW[1] = CM_BDW_ADJUST_Y_SCOREBOARD_DW1;
			addInstDW[2] = CM_BDW_ADJUST_Y_SCOREBOARD_DW2;
		} else {
			CM_ASSERT(0);
			return CM_FAILURE;
		}

		USHORT tmp = -(INT) (m_adjustScoreboardY);
		addInstDW[3] = (tmp << 16) + tmp;

	}

	pCodeDst =
	    new(std::nothrow) BYTE[(movInstNum + addInstNum) *
				   CM_MOVE_INSTRUCTION_SIZE];
	if (!pCodeDst) {
		return CM_OUT_OF_HOST_MEMORY;
	}

	for (UINT j = 0; j < movInstNum; j++) {
		MovInst_RT *movInst = (MovInst_RT *) movInsts.GetElement(j);
		if (!movInst) {
			CM_ASSERT(0);
			CmSafeDeleteArray(pCodeDst);
			return CM_FAILURE;
		}
		if (j != 0) {
			movInst->ClearDebug();
		}
		CmFastMemCopy(pCodeDst + j * CM_MOVE_INSTRUCTION_SIZE,
			      movInst->GetBinary(), CM_MOVE_INSTRUCTION_SIZE);
		CmSafeDelete(movInst);
	}
	movInsts.Delete();

	if (addInstNum != 0) {
		CmFastMemCopy(pCodeDst + movInstNum * CM_MOVE_INSTRUCTION_SIZE,
			      addInstDW, CM_MOVE_INSTRUCTION_SIZE);

		movInstNum += addInstNum;
	}

	return CM_SUCCESS;
}

INT CmKernel::CreateKernelArgDataGroup(PBYTE & pData, UINT Value)
{
	pData = new(std::nothrow) BYTE[sizeof(UINT)];
	if (!pData) {
		return CM_OUT_OF_HOST_MEMORY;
	}
	*(UINT *) pData = Value;
	return CM_SUCCESS;
}

INT CmKernel::CreateThreadArgData(PCM_HAL_KERNEL_ARG_PARAM pKernelArg,
				  UINT ThreadArgIndex,
				  CmThreadSpace * pThreadSpace,
				  BOOL isKernelThreadSpace, CM_ARG * pCmArgs)
{
	INT hr = CM_SUCCESS;
	UINT ThreadArgCount = pCmArgs[ThreadArgIndex].unitCount;
	UINT ThreadArgSize = pCmArgs[ThreadArgIndex].unitSize;
	CmThreadSpace *pTS_RT = pThreadSpace;

	if (pKernelArg->pFirstValue == NULL) {
		pKernelArg->pFirstValue =
		    new(std::nothrow) BYTE[pCmArgs[ThreadArgIndex].unitCount *
					   pCmArgs[ThreadArgIndex].unitSize];
		if (!pKernelArg->pFirstValue) {
			hr = CM_OUT_OF_HOST_MEMORY;
			goto finish;
		}
	}

	if (pKernelArg->iUnitCount == 1) {
		CmFastMemCopy(pKernelArg->pFirstValue,
			      pCmArgs[ThreadArgIndex].pValue,
			      ThreadArgCount * ThreadArgSize);
		goto finish;
	}

	if (pTS_RT != NULL) {
		CM_HAL_DEPENDENCY_PATTERN DependencyPatternType =
		    CM_DEPENDENCY_NONE;
		pTS_RT->GetDependencyPatternType(DependencyPatternType);

		if ((m_AssociatedToTS == TRUE)
		    && (DependencyPatternType != CM_DEPENDENCY_NONE)) {
			CM_THREAD_SPACE_UNIT *pThreadSpaceUnit = NULL;
			pTS_RT->GetThreadSpaceUnit(pThreadSpaceUnit);

			UINT *pBoardOrder = NULL;
			pTS_RT->GetBoardOrder(pBoardOrder);

			for (UINT index = 0; index < ThreadArgCount; index++) {
				UINT offset =
				    pThreadSpaceUnit[pBoardOrder
						     [index]].threadId;
				BYTE *pArgSrc =
				    (BYTE *) pCmArgs[ThreadArgIndex].pValue +
				    offset * ThreadArgSize;
				BYTE *pArgDst =
				    pKernelArg->pFirstValue +
				    index * ThreadArgSize;
				CmFastMemCopy(pArgDst, pArgSrc, ThreadArgSize);
			}
		} else {
			CmFastMemCopy(pKernelArg->pFirstValue,
				      pCmArgs[ThreadArgIndex].pValue,
				      ThreadArgCount * ThreadArgSize);
		}
	} else {
		CmFastMemCopy(pKernelArg->pFirstValue,
			      pCmArgs[ThreadArgIndex].pValue,
			      ThreadArgCount * ThreadArgSize);
	}

 finish:
	return hr;
}

INT CmKernel::SortThreadSpace(CmThreadSpace * pThreadSpace)
{
	INT hr = CM_SUCCESS;
	CM_HAL_DEPENDENCY_PATTERN DependencyPatternType = CM_DEPENDENCY_NONE;

	CMCHK_NULL(pThreadSpace);

	pThreadSpace->GetDependencyPatternType(DependencyPatternType);

	if (!pThreadSpace->IsThreadAssociated()) {
		return CM_SUCCESS;
	}

	switch (DependencyPatternType) {
	case CM_DEPENDENCY_WAVEFRONT:
		pThreadSpace->Wavefront45Sequence();
		break;

	case CM_DEPENDENCY_WAVEFRONT26:
		pThreadSpace->Wavefront26Sequence();
		break;

	case CM_DEPENDENCY_WAVEFRONT26Z:
		pThreadSpace->Wavefront26ZSequence();
		break;

	case CM_DEPENDENCY_WAVEFRONT26ZI:
		CM_HAL_26ZI_DISPATCH_PATTERN dispatchPattern;
		pThreadSpace->Get26ZIDispatchPattern(dispatchPattern);
		switch (dispatchPattern) {
		case VVERTICAL_HVERTICAL_26:
			pThreadSpace->Wavefront26ZISeqVVHV26();
			break;
		case VVERTICAL_HHORIZONTAL_26:
			pThreadSpace->Wavefront26ZISeqVVHH26();
			break;
		case VVERTICAL26_HHORIZONTAL26:
			pThreadSpace->Wavefront26ZISeqVV26HH26();
			break;
		case VVERTICAL1X26_HHORIZONTAL1X26:
			pThreadSpace->Wavefront26ZISeqVV1x26HH1x26();
			break;
		default:
			pThreadSpace->Wavefront26ZISeqVVHV26();
			break;
		}
		break;

	case CM_DEPENDENCY_HORIZONTAL:
		pThreadSpace->HorizentalSequence();
		break;

	case CM_DEPENDENCY_VERTICAL:
		pThreadSpace->VerticalSequence();
		break;

	case CM_DEPENDENCY_NONE:
		break;

	default:
		CM_ASSERT(0);
		hr = CM_FAILURE;
		break;
	}

 finish:
	return hr;
}

INT CmKernel::CreateTempArgs(UINT NumofArgs, CM_ARG * &pTempArgs)
{
	INT hr = CM_SUCCESS;
	INT num_surfaces = 0;
	INT increased_args = 0;

	if (NumofArgs < m_ArgCount || pTempArgs != NULL) {
		CM_ASSERT(0);
		hr = CM_FAILURE;
		goto finish;
	}

	pTempArgs = new(std::nothrow) CM_ARG[NumofArgs];
	CMCHK_NULL_RETURN(pTempArgs, CM_OUT_OF_HOST_MEMORY);
	CmSafeMemSet(pTempArgs, 0, NumofArgs * sizeof(CM_ARG));

	for (UINT j = 0; j < m_ArgCount; j++) {
		if (CHECK_SURFACE_TYPE(m_Args[j].unitKind,
				       ARG_KIND_SURFACE,
				       ARG_KIND_SURFACE_1D,
				       ARG_KIND_SURFACE_2D,
				       ARG_KIND_SURFACE_2D_UP,
				       ARG_KIND_SURFACE_2D_DUAL)) {
			num_surfaces = m_Args[j].unitSize / sizeof(int);

			if (num_surfaces > 1) {
				if (m_Args[j].unitCount == 1) {
					for (INT k = 0; k < num_surfaces; k++) {
						pTempArgs[j + increased_args +
							  k] = m_Args[j];
						pTempArgs[j + increased_args +
							  k].unitSize =
						    sizeof(INT);
						pTempArgs[j + increased_args +
							  k].unitSizeOrig =
						    sizeof(INT);
						pTempArgs[j + increased_args +
							  k].pValue =
						    (BYTE *) ((UINT *)
							      m_Args[j].pValue +
							      k);
						pTempArgs[j + increased_args +
							  k].unitOffsetInPayload
						    =
						    m_Args
						    [j].unitOffsetInPayload +
						    4 * k;
						pTempArgs[j + increased_args +
							  k].unitOffsetInPayloadOrig
						    =
						    pTempArgs[j +
							      increased_args +
							      k].unitOffsetInPayload;
					}
				} else {
					UINT *surfaces =
					    (UINT *) new(std::nothrow)
					    BYTE[sizeof(INT) *
						 m_Args[j].unitCount];
					CMCHK_NULL_RETURN(surfaces,
							  CM_OUT_OF_HOST_MEMORY);
					for (INT k = 0; k < num_surfaces; k++) {
						pTempArgs[j + increased_args +
							  k] = m_Args[j];
						pTempArgs[j + increased_args +
							  k].unitSize =
						    sizeof(INT);
						pTempArgs[j + increased_args +
							  k].unitSizeOrig =
						    sizeof(INT);
						pTempArgs[j + increased_args +
							  k].pValue =
						    new(std::nothrow)
						    BYTE[sizeof(INT) *
							 m_Args[j].unitCount];
						if (pTempArgs
						    [j + increased_args +
						     k].pValue == NULL) {
							CM_ASSERT(0);
							hr = CM_OUT_OF_HOST_MEMORY;
							CmSafeDeleteArray
							    (surfaces);
							goto finish;
						}
						for (UINT s = 0;
						     s < m_Args[j].unitCount;
						     s++) {
							surfaces[s] =
							    *(UINT *) ((UINT *)
								       m_Args
								       [j].pValue
								       + k +
								       num_surfaces
								       * s);
						}
						CmFastMemCopy(pTempArgs
							      [j +
							       increased_args +
							       k].pValue,
							      surfaces,
							      sizeof(INT) *
							      m_Args
							      [j].unitCount);
						pTempArgs[j + increased_args +
							  k].unitOffsetInPayload
						    =
						    m_Args
						    [j].unitOffsetInPayload +
						    4 * k;
						pTempArgs[j + increased_args +
							  k].unitOffsetInPayloadOrig
						    = (WORD) - 1;
					}
					CmSafeDeleteArray(surfaces);
				}
				increased_args += num_surfaces - 1;
			} else {
				pTempArgs[j + increased_args] = m_Args[j];
			}
		} else {
			pTempArgs[j + increased_args] = m_Args[j];
		}
	}

 finish:
	if (hr == CM_OUT_OF_HOST_MEMORY) {
		if (pTempArgs) {
			for (UINT j = 0; j < NumofArgs; j++) {
				CmSafeDeleteArray(pTempArgs[j].pValue);
			}
		}
		CmSafeDeleteArray(pTempArgs);
	}
	return hr;
}

INT CmKernel::GetArgCountPlusSurfArray(UINT & ArgSize, UINT & ArgCountPlus)
{
	UINT extra_surfaces = 0;

	ArgCountPlus = m_ArgCount;
	ArgSize = 0;

	if (m_usKernelPayloadDataSize) {
		ArgCountPlus = 0;
		ArgSize = 0;
		return CM_SUCCESS;
	}
	if (m_ArgCount != 0) {
		if ((m_blPerThreadArgExists == FALSE)
		    && (m_blPerKernelArgExists == FALSE)
		    && (m_usKernelPayloadDataSize == 0)) {
			CM_ASSERTMESSAGE("Kernel arguments is not set.");
			return CM_NOT_SET_KERNEL_ARGUMENT;
		}

		if (m_blPerThreadArgExists || m_blPerKernelArgExists) {
			for (UINT j = 0; j < m_ArgCount; j++) {
				if (!m_Args[j].bIsSet) {
					CM_ASSERTMESSAGE
					    ("One Kernel arguments is not set.");
					return CM_KERNEL_ARG_SETTING_FAILED;
				}

				ArgSize +=
				    m_Args[j].unitSize * m_Args[j].unitCount;

				if (CHECK_SURFACE_TYPE(m_Args[j].unitKind,
						       ARG_KIND_SURFACE,
						       ARG_KIND_SURFACE_1D,
						       ARG_KIND_SURFACE_2D,
						       ARG_KIND_SURFACE_2D_UP,
						       ARG_KIND_SURFACE_2D_DUAL))
				{
					int num_surfaces =
					    m_Args[j].unitSize / sizeof(int);
					if (num_surfaces > 1) {
						extra_surfaces +=
						    num_surfaces - 1;
					}
				}
			}

			ArgCountPlus = m_ArgCount + extra_surfaces;
		}
	}
	return CM_SUCCESS;
}

INT CmKernel::CreateThreadSpaceParam(PCM_HAL_KERNEL_THREADSPACE_PARAM
				     pCmKernelThreadSpaceParam,
				     CmThreadSpace * pThreadSpace)
{
	INT hr = CM_SUCCESS;
	CM_DEPENDENCY *pDependency = NULL;
	UINT TsWidth = 0;
	UINT TsHeight = 0;
	CM_THREAD_SPACE_UNIT *pThreadSpaceUnit = NULL;
	CM_THREAD_SPACE_DIRTY_STATUS dirtyStatus = CM_THREAD_SPACE_CLEAN;

	if (pCmKernelThreadSpaceParam == NULL || pThreadSpace == NULL) {
		CM_ASSERT(0);
		hr = CM_FAILURE;
		goto finish;
	}

	pThreadSpace->GetThreadSpaceSize(TsWidth, TsHeight);
	pCmKernelThreadSpaceParam->iThreadSpaceWidth = (WORD) TsWidth;
	pCmKernelThreadSpaceParam->iThreadSpaceHeight = (WORD) TsHeight;

	pThreadSpace->GetDependencyPatternType(pCmKernelThreadSpaceParam->
					       patternType);
	pThreadSpace->GetDependency(pDependency);

	if (pDependency != NULL) {
		CmFastMemCopy(&pCmKernelThreadSpaceParam->dependencyInfo,
			      pDependency, sizeof(CM_HAL_DEPENDENCY));
	}

	if (pThreadSpace->CheckWalkingParametersSet()) {
		pCmKernelThreadSpaceParam->walkingParamsValid = 1;
		CMCHK_HR(pThreadSpace->GetWalkingParameters
			 (pCmKernelThreadSpaceParam->walkingParams));
	} else {
		pCmKernelThreadSpaceParam->walkingParamsValid = 0;
	}

	if (pThreadSpace->CheckDependencyVectorsSet()) {
		pCmKernelThreadSpaceParam->dependencyVectorsValid = 1;
		CMCHK_HR(pThreadSpace->GetDependencyVectors
			 (pCmKernelThreadSpaceParam->dependencyVectors));
	} else {
		pCmKernelThreadSpaceParam->dependencyVectorsValid = 0;
	}

	pThreadSpace->GetThreadSpaceUnit(pThreadSpaceUnit);

	if (pThreadSpaceUnit) {
		pCmKernelThreadSpaceParam->pThreadCoordinates =
		    new(std::nothrow) CM_HAL_SCOREBOARD_XY_MASK[TsWidth *
								TsHeight];
		CMCHK_NULL_RETURN(pCmKernelThreadSpaceParam->pThreadCoordinates,
				  CM_OUT_OF_HOST_MEMORY);
		CmSafeMemSet(pCmKernelThreadSpaceParam->pThreadCoordinates, 0,
			     TsHeight * TsWidth *
			     sizeof(CM_HAL_SCOREBOARD_XY_MASK));

		UINT *pBoardOrder = NULL;
		pThreadSpace->GetBoardOrder(pBoardOrder);
		CMCHK_NULL(pBoardOrder);

		pCmKernelThreadSpaceParam->reuseBBUpdateMask = 0;
		for (UINT i = 0; i < TsWidth * TsHeight; i++) {
			pCmKernelThreadSpaceParam->pThreadCoordinates[i].x =
			    pThreadSpaceUnit[pBoardOrder
					     [i]].scoreboardCoordinates.x;
			pCmKernelThreadSpaceParam->pThreadCoordinates[i].y =
			    pThreadSpaceUnit[pBoardOrder
					     [i]].scoreboardCoordinates.y;
			pCmKernelThreadSpaceParam->pThreadCoordinates[i].mask =
			    pThreadSpaceUnit[pBoardOrder[i]].dependencyMask;
			pCmKernelThreadSpaceParam->
			    pThreadCoordinates[i].resetMask =
			    pThreadSpaceUnit[pBoardOrder[i]].reset;

			pCmKernelThreadSpaceParam->reuseBBUpdateMask |=
			    pThreadSpaceUnit[pBoardOrder[i]].reset;
		}

		if (pCmKernelThreadSpaceParam->patternType ==
		    CM_DEPENDENCY_WAVEFRONT26Z) {
			CM_HAL_WAVEFRONT26Z_DISPATCH_INFO dispatchInfo;
			pThreadSpace->GetWavefront26ZDispatchInfo(dispatchInfo);

			pCmKernelThreadSpaceParam->dispatchInfo.numWaves =
			    dispatchInfo.numWaves;
			pCmKernelThreadSpaceParam->
			    dispatchInfo.pNumThreadsInWave =
			    new(std::nothrow) UINT[dispatchInfo.numWaves];

			CMCHK_NULL_RETURN
			    (pCmKernelThreadSpaceParam->dispatchInfo.
			     pNumThreadsInWave, CM_OUT_OF_HOST_MEMORY);
			CmFastMemCopy(pCmKernelThreadSpaceParam->dispatchInfo.
				      pNumThreadsInWave,
				      dispatchInfo.pNumThreadsInWave,
				      dispatchInfo.numWaves * sizeof(UINT));

		}
	}

	dirtyStatus = pThreadSpace->GetDirtyStatus();
	switch (dirtyStatus) {
	case CM_THREAD_SPACE_CLEAN:
		pCmKernelThreadSpaceParam->BBdirtyStatus = CM_HAL_BB_CLEAN;
		break;
	default:
		pCmKernelThreadSpaceParam->BBdirtyStatus = CM_HAL_BB_DIRTY;
		break;
	}

 finish:
	if (hr == CM_OUT_OF_HOST_MEMORY) {
		if (pCmKernelThreadSpaceParam) {
			CmSafeDeleteArray
			    (pCmKernelThreadSpaceParam->dispatchInfo.
			     pNumThreadsInWave);
			CmSafeDeleteArray
			    (pCmKernelThreadSpaceParam->pThreadCoordinates);
		}
	}

	return hr;
}

INT CmKernel::DestroyArgs(void)
{
	for (UINT i = 0; i < m_ArgCount; i++) {
		CM_ARG & arg = m_Args[i];
		CmSafeDeleteArray(arg.pValue);
		CmSafeDeleteArray(arg.surfIndex);
		arg.unitCount = 0;
		arg.unitSize = 0;
		arg.unitKind = 0;
		arg.unitOffsetInPayload = 0;
		arg.bIsDirty = TRUE;
		arg.bIsSet = FALSE;
	}

	CmSafeDeleteArray(m_Args);

	m_AssociatedToTS = FALSE;
	m_pThreadSpace = NULL;

	m_blPerThreadArgExists = FALSE;
	m_blPerKernelArgExists = FALSE;

	m_SizeInCurbe = 0;
	m_CurbeEnable = true;

	m_SizeInPayload = 0;
	m_adjustScoreboardY = 0;

	ResetKernelSurfaces();

	return CM_SUCCESS;
}

INT CmKernel::Reset(void)
{
	for (UINT i = 0; i < m_ArgCount; i++) {
		CM_ARG & arg = m_Args[i];
		CmSafeDeleteArray(arg.pValue);
		CmSafeDeleteArray(arg.surfIndex);
		arg.pValue = NULL;
		arg.unitCount = 0;

		arg.unitSize = arg.unitSizeOrig;
		arg.unitKind = arg.unitKindOrig;
		arg.unitOffsetInPayload = arg.unitOffsetInPayloadOrig;

		arg.bIsDirty = TRUE;
		arg.bIsSet = FALSE;
	}

	m_ThreadCount = 0;

	m_IndexInTask = 0;

	m_blPerThreadArgExists = FALSE;
	m_blPerKernelArgExists = FALSE;

	m_SizeInCurbe = 0;
	m_CurbeEnable = true;

	m_SizeInPayload = 0;

	m_AssociatedToTS = FALSE;
	m_pThreadSpace = NULL;
	m_adjustScoreboardY = 0;

	m_pThreadGroupSpace = NULL;

	CmSafeDeleteArray(m_pKernelPayloadData);
	m_usKernelPayloadDataSize = 0;

	if (m_usKernelPayloadSurfaceCount) {
		CmSafeMemSet(m_pKernelPayloadSurfaceArray, 0,
			     m_usKernelPayloadSurfaceCount *
			     sizeof(SurfaceIndex *));
		CmSafeMemSet(m_IndirectSurfaceInfoArray, 0,
			     m_usKernelPayloadSurfaceCount *
			     sizeof(CM_INDIRECT_SURFACE_INFO));
		m_usKernelPayloadSurfaceCount = 0;
	}

	ResetKernelSurfaces();

	return CM_SUCCESS;
}

INT CmKernel::GetArgs(CM_ARG * &pArg)
{
	pArg = m_Args;
	return CM_SUCCESS;
}

INT CmKernel::GetArgCount(UINT & argCount)
{
	argCount = m_ArgCount;
	return CM_SUCCESS;
}

CM_RT_API INT CmKernel::SetThreadDependencyMask(UINT threadId, BYTE mask)
{
	CM_ASSERT(0);
	return CM_NOT_IMPLEMENTED;
}

INT CmKernel::GetCurbeEnable(BOOL & b)
{
	b = m_CurbeEnable;
	return CM_SUCCESS;
}

INT CmKernel::SetCurbeEnable(BOOL b)
{
	m_CurbeEnable = b;
	return CM_SUCCESS;
}

INT CmKernel::GetSizeInCurbe(UINT & size)
{
	size = m_SizeInCurbe;
	return CM_SUCCESS;
}

INT CmKernel::GetSizeInPayload(UINT & size)
{
	size = m_SizeInPayload;
	return CM_SUCCESS;
}

INT CmKernel::GetCmDevice(CmDevice * &pCmDev)
{
	pCmDev = m_pCmDev;
	return CM_SUCCESS;
}

INT CmKernel::GetCmProgram(CmProgram * &pProgram)
{
	pProgram = m_pProgram;
	return CM_SUCCESS;
}

INT CmKernel::CollectKernelSurface()
{
	for (UINT j = 0; j < m_ArgCount; j++) {
		if ((m_Args[j].unitKind == ARG_KIND_SURFACE) ||
		    (m_Args[j].unitKind == ARG_KIND_SURFACE_1D) ||
		    (m_Args[j].unitKind == ARG_KIND_SURFACE_2D) ||
		    (m_Args[j].unitKind == ARG_KIND_SURFACE_2D_UP) ||
		    (m_Args[j].unitKind == ARG_KIND_SURFACE_2D_DUAL)) {
			int num_surfaces = m_Args[j].unitSize / sizeof(int);
			int num_valid_surfaces = 0;

			for (UINT k = 0; k < num_surfaces * m_Args[j].unitCount;
			     k++) {
				WORD surfIndex = m_Args[j].surfIndex[k];
				if (surfIndex != 0
				    && surfIndex != CM_NULL_SURFACE) {
					m_SurfaceArray[surfIndex] = TRUE;
					num_valid_surfaces++;
				}
			}
		}
	}

	for (INT i = 0; i < CM_GLOBAL_SURFACE_NUMBER; ++i) {
		if (m_GlobalSurfaces[i] != NULL) {
			UINT surfIndex = m_GlobalCmIndex[i];
			m_SurfaceArray[surfIndex] = TRUE;
		}
	}

	for (INT i = 0; i < m_usKernelPayloadSurfaceCount; i++) {
		if (m_pKernelPayloadSurfaceArray[i] != NULL) {
			UINT surfIndex =
			    m_pKernelPayloadSurfaceArray[i]->get_data();
			m_SurfaceArray[surfIndex] = TRUE;
		}
	}

	return CM_SUCCESS;
}

INT CmKernel::IsKernelDataReusable(CmThreadSpace * pTS)
{
	if (pTS) {
		if (pTS->IsThreadAssociated()
		    && (pTS->GetDirtyStatus() != CM_THREAD_SPACE_CLEAN)) {
			return FALSE;
		}
	}

	if (m_pThreadSpace) {
		if (m_pThreadSpace->GetDirtyStatus() != CM_THREAD_SPACE_CLEAN) {
			return FALSE;
		}
	}

	if (m_Dirty != CM_KERNEL_DATA_CLEAN) {
		return FALSE;
	}

	return TRUE;
}

INT CmKernel::CreateKernelData(CmKernelData * &pKernelData,
			       UINT & kernelDataSize, const CmThreadSpace * pTS)
{
	INT hr = CM_SUCCESS;
	PCM_HAL_KERNEL_PARAM pHalKernelParam = NULL;

	if ((pTS != NULL) && (m_pThreadSpace != NULL)) {
		return CM_INVALID_THREAD_SPACE;
	}

	if (m_pLastKernelData == NULL) {
		CMCHK_HR(CreateKernelDataInternal
			 (pKernelData, kernelDataSize, pTS));
		pKernelData->AcquireKernel();
		CMCHK_HR(UpdateLastKernelData(pKernelData));
	} else {
		if (IsKernelDataReusable((CmThreadSpace *) pTS)) {
			pKernelData = m_pLastKernelData;
			pKernelData->Acquire();
			pKernelData->AcquireKernel();
			kernelDataSize = pKernelData->GetKernelDataSize();
			if (m_pThreadSpace) {
				pHalKernelParam =
				    pKernelData->GetHalCmKernelData();
				CMCHK_NULL(pHalKernelParam);
				pHalKernelParam->
				    CmKernelThreadSpaceParam.BBdirtyStatus =
				    CM_HAL_BB_CLEAN;
			}

		} else {
			if (m_pLastKernelData->IsInUse()) {
				CMCHK_HR(CreateKernelDataInternal
					 (pKernelData, kernelDataSize, pTS));
				pKernelData->AcquireKernel();
				CMCHK_HR(UpdateLastKernelData(pKernelData));
			} else if (pTS && pTS->IsThreadAssociated()
				   && (pTS->GetDirtyStatus() !=
				       CM_THREAD_SPACE_CLEAN)) {
				CMCHK_HR(CreateKernelDataInternal
					 (pKernelData, kernelDataSize, pTS));
				pKernelData->AcquireKernel();
				CMCHK_HR(UpdateLastKernelData(pKernelData));
			} else if (m_Dirty < CM_KERNEL_DATA_THREAD_COUNT_DIRTY
				   || (m_pThreadSpace
				       && m_pThreadSpace->GetDirtyStatus() ==
				       CM_THREAD_SPACE_DEPENDENCY_MASK_DIRTY)) {
				CMCHK_HR(UpdateKernelData
					 (m_pLastKernelData, pTS));
				m_pLastKernelData->Acquire();
				m_pLastKernelData->AcquireKernel();
				pKernelData = m_pLastKernelData;
				kernelDataSize =
				    pKernelData->GetKernelDataSize();

			} else {
				CMCHK_HR(CreateKernelDataInternal
					 (pKernelData, kernelDataSize, pTS));
				pKernelData->AcquireKernel();
				CMCHK_HR(UpdateLastKernelData(pKernelData));
			}
		}
	}

	CleanArgDirtyFlag();
	if (pTS) {
		pTS->SetDirtyStatus(CM_THREAD_SPACE_CLEAN);
	}
	if (m_pThreadSpace) {
		m_pThreadSpace->SetDirtyStatus(CM_THREAD_SPACE_CLEAN);
	}

 finish:
	return hr;
}

INT CmKernel::CreateKernelData(CmKernelData * &pKernelData,
			       UINT & kernelDataSize,
			       const CmThreadGroupSpace * pTGS)
{
	INT hr = CM_SUCCESS;
	CmThreadGroupSpace *pUsedTGS = NULL;

	if (m_pThreadGroupSpace) {
		pUsedTGS = m_pThreadGroupSpace;
	} else {
		pUsedTGS = const_cast < CmThreadGroupSpace * >(pTGS);
	}

	if (m_pLastKernelData == NULL) {
		CMCHK_HR(CreateKernelDataInternal
			 (pKernelData, kernelDataSize, pUsedTGS));
		pKernelData->AcquireKernel();
		CMCHK_HR(UpdateLastKernelData(pKernelData));
	} else {
		if (!(m_Dirty & CM_KERNEL_DATA_KERNEL_ARG_DIRTY)) {
			pKernelData = m_pLastKernelData;
			pKernelData->Acquire();
			pKernelData->AcquireKernel();
			kernelDataSize = pKernelData->GetKernelDataSize();
		} else {
			if (m_pLastKernelData->IsInUse()) {
				CMCHK_HR(CreateKernelDataInternal
					 (pKernelData, kernelDataSize,
					  pUsedTGS));
				pKernelData->AcquireKernel();
				CMCHK_HR(UpdateLastKernelData(pKernelData));
			} else {
				CMCHK_HR(UpdateKernelData
					 (m_pLastKernelData, pUsedTGS));
				m_pLastKernelData->Acquire();
				m_pLastKernelData->AcquireKernel();
				pKernelData = m_pLastKernelData;
				kernelDataSize =
				    pKernelData->GetKernelDataSize();
			}
		}
	}

	CleanArgDirtyFlag();

 finish:
	return hr;
}

INT CmKernel::CleanArgDirtyFlag()
{

	for (UINT i = 0; i < m_ArgCount; i++) {
		m_Args[i].bIsDirty = FALSE;
	}

	if (m_pThreadSpace && m_pThreadSpace->GetDirtyStatus()) {
		m_pThreadSpace->SetDirtyStatus(CM_THREAD_SPACE_CLEAN);
	}

	m_Dirty = CM_KERNEL_DATA_CLEAN;

	return CM_SUCCESS;
}

INT CmKernel::UpdateKernelDataGlobalSurfaceInfo(PCM_HAL_KERNEL_PARAM
						pHalKernelParam)
{
	INT hr = CM_SUCCESS;

	for (UINT j = 0; j < CM_GLOBAL_SURFACE_NUMBER; j++) {
		if (m_GlobalSurfaces[j] != NULL) {
			pHalKernelParam->globalSurface[j] =
			    m_GlobalSurfaces[j]->get_data();
			pHalKernelParam->bGlobalSurfaceUsed = TRUE;
		} else {
			pHalKernelParam->globalSurface[j] = CM_NULL_SURFACE;
		}
	}

	return hr;
}

INT CmKernel::CreateKernelDataInternal(CmKernelData * &pKernelData,
				       UINT & kernelDataSize,
				       const CmThreadGroupSpace * pTGS)
{
	PCM_HAL_KERNEL_PARAM pHalKernelParam = NULL;
	INT hr = CM_SUCCESS;
	UINT movInstNum = 0;
	UINT KrnCurbeSize = 0;
	UINT NumArgs = 0;
	CM_ARG *pTempArgs = NULL;
	UINT ArgSize = 0;
	UINT surfNum = 0;

	CMCHK_HR(CmKernelData::Create(this, pKernelData));
	pHalKernelParam = pKernelData->GetHalCmKernelData();
	CMCHK_NULL(pHalKernelParam);

	CMCHK_HR(GetArgCountPlusSurfArray(ArgSize, NumArgs));

	CMCHK_HR(CreateTempArgs(NumArgs, pTempArgs));

	CMCHK_HR(CreateMovInstructions
		 (movInstNum, pHalKernelParam->pMovInsData, pTempArgs,
		  NumArgs));
	CMCHK_HR(CalcKernelDataSize
		 (movInstNum, NumArgs, ArgSize, kernelDataSize));
	CMCHK_HR(pKernelData->SetKernelDataSize(kernelDataSize));

	pHalKernelParam->uiKernelId = m_Id++;
	pHalKernelParam->iNumArgs = NumArgs + CM_GPUWALKER_IMPLICIT_ARG_NUM;
	pHalKernelParam->iNumThreads = m_ThreadCount;
	pHalKernelParam->iKernelBinarySize =
	    m_uiBinarySize + movInstNum * CM_MOVE_INSTRUCTION_SIZE;
	pHalKernelParam->iKernelDataSize = kernelDataSize;
	pHalKernelParam->iMovInsDataSize =
	    movInstNum * CM_MOVE_INSTRUCTION_SIZE;
	pHalKernelParam->bKernelDebugEnabled = m_blhwDebugEnable;

	pHalKernelParam->dwCmFlags = m_CurbeEnable ? CM_FLAG_CURBE_ENABLED : 0;
	pHalKernelParam->dwCmFlags |=
	    m_NonstallingScoreboardEnable ?
	    CM_FLAG_NONSTALLING_SCOREBOARD_ENABLED : 0;

	pHalKernelParam->pKernelBinary = (PBYTE) m_pBinary;

	for (UINT i = 0; i < NumArgs; i++) {
		pHalKernelParam->CmArgParams[i].iUnitCount =
		    pTempArgs[i].unitCount;
		pHalKernelParam->CmArgParams[i].Kind =
		    (CM_HAL_KERNEL_ARG_KIND) (pTempArgs[i].unitKind);
		pHalKernelParam->CmArgParams[i].iUnitSize =
		    pTempArgs[i].unitSize;
		pHalKernelParam->CmArgParams[i].iPayloadOffset =
		    pTempArgs[i].unitOffsetInPayload;
		pHalKernelParam->CmArgParams[i].bPerThread = FALSE;
		pHalKernelParam->CmArgParams[i].nCustomValue =
		    pTempArgs[i].nCustomValue;

		CreateThreadArgData(&pHalKernelParam->CmArgParams[i], i, NULL,
				    FALSE, pTempArgs);

		if (pHalKernelParam->dwCmFlags & CM_KERNEL_FLAGS_CURBE) {
			if (IsKernelArg(pHalKernelParam->CmArgParams[i])) {
				pHalKernelParam->
				    CmArgParams[i].iPayloadOffset -=
				    CM_PAYLOAD_OFFSET;

				if (pHalKernelParam->
				    CmArgParams[i].iPayloadOffset +
				    pHalKernelParam->CmArgParams[i].iUnitSize >
				    KrnCurbeSize) {
					KrnCurbeSize =
					    pHalKernelParam->
					    CmArgParams[i].iPayloadOffset +
					    pHalKernelParam->
					    CmArgParams[i].iUnitSize;
				}
			}
		}
	}

	for (UINT i = NumArgs; i < NumArgs + CM_GPUWALKER_IMPLICIT_ARG_NUM; i++) {
		pHalKernelParam->CmArgParams[i].iUnitCount = 1;
		pHalKernelParam->CmArgParams[i].Kind = CM_ARGUMENT_GENERAL;
		pHalKernelParam->CmArgParams[i].iUnitSize = 4;
		pHalKernelParam->CmArgParams[i].iPayloadOffset =
		    GENOS_ALIGN_CEIL(KrnCurbeSize,
				     4) + (i - NumArgs) * sizeof(DWORD);
		pHalKernelParam->CmArgParams[i].bPerThread = FALSE;
	}

	UINT thrdSpaceWidth, thrdSpaceHeight, grpSpaceWidth, grpSpaceHeight;
	pTGS->GetThreadGroupSpaceSize(thrdSpaceWidth, thrdSpaceHeight,
				      grpSpaceWidth, grpSpaceHeight);

	CMCHK_HR(CreateKernelArgDataGroup
		 (pHalKernelParam->CmArgParams[NumArgs + 0].pFirstValue,
		  thrdSpaceWidth));
	CMCHK_HR(CreateKernelArgDataGroup
		 (pHalKernelParam->CmArgParams[NumArgs + 1].pFirstValue,
		  thrdSpaceHeight));
	CMCHK_HR(CreateKernelArgDataGroup
		 (pHalKernelParam->CmArgParams[NumArgs + 2].pFirstValue,
		  grpSpaceWidth));
	CMCHK_HR(CreateKernelArgDataGroup
		 (pHalKernelParam->CmArgParams[NumArgs + 3].pFirstValue,
		  grpSpaceHeight));
	CMCHK_HR(CreateKernelArgDataGroup
		 (pHalKernelParam->CmArgParams[NumArgs + 4].pFirstValue,
		  thrdSpaceWidth));
	CMCHK_HR(CreateKernelArgDataGroup
		 (pHalKernelParam->CmArgParams[NumArgs + 5].pFirstValue,
		  thrdSpaceHeight));

	pHalKernelParam->GpGpuWalkerParams.CmGpGpuEnable = TRUE;
	pHalKernelParam->GpGpuWalkerParams.GroupWidth = grpSpaceWidth;
	pHalKernelParam->GpGpuWalkerParams.GroupHeight = grpSpaceHeight;
	pHalKernelParam->GpGpuWalkerParams.ThreadHeight = thrdSpaceHeight;
	pHalKernelParam->GpGpuWalkerParams.ThreadWidth = thrdSpaceWidth;
	pHalKernelParam->GpGpuWalkerParams.SLMSize = GetSLMSize();

	KrnCurbeSize =
	    GENOS_ALIGN_CEIL(KrnCurbeSize,
			     4) + CM_GPUWALKER_IMPLICIT_ARG_NUM * sizeof(DWORD);
	if ((KrnCurbeSize % 32) == 4) {
		pHalKernelParam->iCurbeSizePerThread = 64;
	} else {
		pHalKernelParam->iCurbeSizePerThread = 32;
	}
	pHalKernelParam->iKrnCurbeSize =
	    GENOS_ALIGN_CEIL(KrnCurbeSize,
			     32) - pHalKernelParam->iCurbeSizePerThread +
	    pHalKernelParam->iCurbeSizePerThread * thrdSpaceWidth *
	    thrdSpaceHeight;

	pHalKernelParam->iCrsThrdConstDataLn =
	    GENOS_ALIGN_CEIL(KrnCurbeSize,
			     32) - pHalKernelParam->iCurbeSizePerThread;
	pHalKernelParam->iPayloadSize = 0;

	CMCHK_HR(CreateKernelIndirectData
		 (&pHalKernelParam->CmIndirectDataParam));

	CalculateKernelSurfacesNum(surfNum, pHalKernelParam->iNumSurfaces);

	UpdateKernelDataGlobalSurfaceInfo(pHalKernelParam);

	for (UINT j = 0; j < NumArgs; j++) {
		if (pTempArgs[j].unitOffsetInPayloadOrig == (UINT) - 1) {
			CmSafeDeleteArray(pTempArgs[j].pValue);
		}
	}
	CmSafeDeleteArray(pTempArgs);

 finish:
	if (hr != CM_SUCCESS) {
		for (UINT i = 0; i < NumArgs + CM_GPUWALKER_IMPLICIT_ARG_NUM;
		     i++) {
			if (pHalKernelParam) {
				if (pHalKernelParam->CmArgParams[i].pFirstValue) {
					CmSafeDeleteArray
					    (pHalKernelParam->CmArgParams[i].
					     pFirstValue);
				}
			}
		}
	}
	return hr;
}

BOOL CmKernel::IsBatchBufferReusable(CmThreadSpace * pTaskThreadSpace)
{
	BOOL Reusable = TRUE;
	if (m_Dirty & CM_KERNEL_DATA_THREAD_ARG_DIRTY) {
		Reusable = FALSE;
	} else if ((m_Dirty & CM_KERNEL_DATA_KERNEL_ARG_DIRTY)
		   && (m_CurbeEnable == FALSE)) {
		Reusable = FALSE;
	} else if (m_Dirty & CM_KERNEL_DATA_THREAD_COUNT_DIRTY) {
		Reusable = FALSE;
	} else if (m_pThreadSpace) {
		if (m_pThreadSpace->GetDirtyStatus() ==
		    CM_THREAD_SPACE_DATA_DIRTY) {
			Reusable = FALSE;
		}
	} else if (pTaskThreadSpace) {
		if (pTaskThreadSpace->GetDirtyStatus() ==
		    CM_THREAD_SPACE_DATA_DIRTY) {
			Reusable = FALSE;
		}
	}
	return Reusable;

}

BOOL CmKernel::IsPrologueDirty(void)
{
	BOOL prologueDirty = FALSE;

	if (m_ThreadCount != m_LastThreadCount) {
		if (m_LastThreadCount) {
			if (m_ThreadCount == 1 || m_LastThreadCount == 1) {
				prologueDirty = TRUE;
			}
		}
		m_LastThreadCount = m_ThreadCount;
	}

	if (m_adjustScoreboardY != m_LastAdjustScoreboardY) {
		if (m_LastAdjustScoreboardY) {
			prologueDirty = TRUE;
		}
		m_LastAdjustScoreboardY = m_adjustScoreboardY;
	}

	return prologueDirty;
}

INT CmKernel::CreateKernelDataInternal(CmKernelData * &pKernelData,
				       UINT & kernelDataSize,
				       const CmThreadSpace * pTS)
{
	PCM_HAL_KERNEL_PARAM pHalKernelParam = NULL;
	INT hr = CM_SUCCESS;
	UINT movInstNum = 0;
	UINT KrnCurbeSize = 0;
	UINT NumArgs = 0;
	UINT dwBottomRange = 1024;
	UINT dwUpRange = 0;
	UINT UnitSize = 0;
	BOOL hasThreadArg = FALSE;
	CmThreadSpace *pCmThreadSpace = NULL;
	BOOL isKernelThreadSpace = FALSE;
	CM_ARG *pTempArgs = NULL;
	UINT ArgSize = 0;
	UINT surfNum = 0;

	if (pTS == NULL && m_pThreadSpace != NULL) {
		pCmThreadSpace = m_pThreadSpace;
		isKernelThreadSpace = TRUE;
	} else {
		pCmThreadSpace = const_cast < CmThreadSpace * >(pTS);
	}

	CMCHK_HR(CmKernelData::Create(this, pKernelData));
	pHalKernelParam = pKernelData->GetHalCmKernelData();
	CMCHK_NULL(pHalKernelParam);

	CMCHK_HR(GetArgCountPlusSurfArray(ArgSize, NumArgs));

	if (NumArgs > 0) {
		CMCHK_HR(CreateTempArgs(NumArgs, pTempArgs));
		CMCHK_HR(CreateMovInstructions
			 (movInstNum, pHalKernelParam->pMovInsData, pTempArgs,
			  NumArgs));
	}

	CMCHK_HR(CalcKernelDataSize
		 (movInstNum, NumArgs, ArgSize, kernelDataSize));
	CMCHK_HR(pKernelData->SetKernelDataSize(kernelDataSize));

	if (!IsBatchBufferReusable(const_cast < CmThreadSpace * >(pTS))) {
		m_Id++;
	}

	if (IsPrologueDirty()) {
		UINT64 tempID = m_Id;
		tempID >>= 48;
		tempID++;
		tempID <<= 48;
		m_Id <<= 16;
		m_Id >>= 16;
		m_Id |= tempID;
	}
	pHalKernelParam->uiKernelId = m_Id;
	pHalKernelParam->iNumArgs = NumArgs;
	pHalKernelParam->iNumThreads = m_ThreadCount;
	pHalKernelParam->iKernelBinarySize =
	    m_uiBinarySize + movInstNum * CM_MOVE_INSTRUCTION_SIZE;
	pHalKernelParam->iKernelDataSize = kernelDataSize;
	pHalKernelParam->iMovInsDataSize =
	    movInstNum * CM_MOVE_INSTRUCTION_SIZE;

	pHalKernelParam->dwCmFlags = m_CurbeEnable ? CM_FLAG_CURBE_ENABLED : 0;
	pHalKernelParam->dwCmFlags |=
	    m_NonstallingScoreboardEnable ?
	    CM_FLAG_NONSTALLING_SCOREBOARD_ENABLED : 0;
	pHalKernelParam->bKernelDebugEnabled = m_blhwDebugEnable;

	pHalKernelParam->pKernelBinary = (PBYTE) m_pBinary;

	if (pCmThreadSpace) {
		CMCHK_HR(SortThreadSpace(pCmThreadSpace));
	}

	for (UINT i = 0; i < NumArgs; i++) {
		pHalKernelParam->CmArgParams[i].iUnitCount =
		    pTempArgs[i].unitCount;
		pHalKernelParam->CmArgParams[i].Kind =
		    (CM_HAL_KERNEL_ARG_KIND) (pTempArgs[i].unitKind);
		pHalKernelParam->CmArgParams[i].iUnitSize =
		    pTempArgs[i].unitSize;
		pHalKernelParam->CmArgParams[i].iPayloadOffset =
		    pTempArgs[i].unitOffsetInPayload;
		pHalKernelParam->CmArgParams[i].bPerThread =
		    (pTempArgs[i].unitCount > 1) ? TRUE : FALSE;
		pHalKernelParam->CmArgParams[i].nCustomValue =
		    pTempArgs[i].nCustomValue;

		CreateThreadArgData(&pHalKernelParam->CmArgParams[i], i,
				    pCmThreadSpace, isKernelThreadSpace,
				    pTempArgs);

		UnitSize = pHalKernelParam->CmArgParams[i].iUnitSize;

		if (pHalKernelParam->dwCmFlags & CM_KERNEL_FLAGS_CURBE) {
			if (IsKernelArg(pHalKernelParam->CmArgParams[i])) {
				DWORD dwOffset =
				    pHalKernelParam->
				    CmArgParams[i].iPayloadOffset -
				    CM_PAYLOAD_OFFSET;
				if (dwOffset >= KrnCurbeSize) {
					KrnCurbeSize = dwOffset + UnitSize;
				}
				pHalKernelParam->
				    CmArgParams[i].iPayloadOffset -=
				    CM_PAYLOAD_OFFSET;
			}
		}

		if (!IsKernelArg(pHalKernelParam->CmArgParams[i])) {
			hasThreadArg = TRUE;
			pHalKernelParam->CmArgParams[i].iPayloadOffset -=
			    CM_PAYLOAD_OFFSET;

			if (pHalKernelParam->CmArgParams[i].iPayloadOffset <
			    dwBottomRange) {
				dwBottomRange =
				    pHalKernelParam->
				    CmArgParams[i].iPayloadOffset;
			}
			if (pHalKernelParam->CmArgParams[i].iPayloadOffset >=
			    dwUpRange) {
				dwUpRange =
				    pHalKernelParam->
				    CmArgParams[i].iPayloadOffset + UnitSize;
			}
		}
	}

	pHalKernelParam->iPayloadSize =
	    hasThreadArg ? GENOS_ALIGN_CEIL(dwUpRange - dwBottomRange, 4) : 0;
	pHalKernelParam->iKrnCurbeSize = GENOS_ALIGN_CEIL(KrnCurbeSize, 32);
	pHalKernelParam->iCurbeSizePerThread = pHalKernelParam->iKrnCurbeSize;
	pHalKernelParam->bPerThreadArgExisted = hasThreadArg;

	if (pHalKernelParam->dwCmFlags & CM_KERNEL_FLAGS_CURBE) {
		for (UINT i = 0; i < NumArgs; i++) {
			if (!IsKernelArg(pHalKernelParam->CmArgParams[i])) {
				pHalKernelParam->
				    CmArgParams[i].iPayloadOffset -=
				    pHalKernelParam->iCurbeSizePerThread;
			}
		}
	}
	CMCHK_HR(CreateKernelIndirectData
		 (&pHalKernelParam->CmIndirectDataParam));

	CalculateKernelSurfacesNum(surfNum, pHalKernelParam->iNumSurfaces);

	if (m_pThreadSpace) {
		CMCHK_HR(CreateThreadSpaceParam
			 (&pHalKernelParam->CmKernelThreadSpaceParam,
			  m_pThreadSpace));
	}

	CMCHK_HR(UpdateKernelDataGlobalSurfaceInfo(pHalKernelParam));

	for (UINT j = 0; j < NumArgs; j++) {
		if (pTempArgs[j].unitOffsetInPayloadOrig == (UINT) - 1) {
			CmSafeDeleteArray(pTempArgs[j].pValue);
		}
	}
	CmSafeDeleteArray(pTempArgs);

 finish:
	if (hr != CM_SUCCESS) {
		if (pHalKernelParam) {
			for (UINT i = 0; i < NumArgs; i++) {
				if (pHalKernelParam->CmArgParams[i].pFirstValue) {
					CmSafeDeleteArray
					    (pHalKernelParam->CmArgParams[i].
					     pFirstValue);
				}
			}
		}
	}
	return hr;
}

INT CmKernel::UpdateKernelData(CmKernelData * pKernelData,
			       const CmThreadSpace * pTS)
{
	INT hr = CM_SUCCESS;
	PCM_HAL_KERNEL_PARAM pHalKernelParam = NULL;
	CmThreadSpace *pCmThreadSpace = NULL;
	BOOL isKernelThreadSpace = FALSE;
	UINT ArgIndexStep = 0;
	UINT ArgIndex = 0;
	UINT surfNum = 0;

	if (pTS == NULL && m_pThreadSpace != NULL) {
		pCmThreadSpace = m_pThreadSpace;
		isKernelThreadSpace = TRUE;
	} else {
		pCmThreadSpace = const_cast < CmThreadSpace * >(pTS);
	}

	CMCHK_NULL(pKernelData);
	CM_ASSERT(pKernelData->IsInUse() == FALSE);

	pHalKernelParam = pKernelData->GetHalCmKernelData();
	CMCHK_NULL(pHalKernelParam);

	if (!IsBatchBufferReusable(const_cast < CmThreadSpace * >(pTS))) {
		m_Id++;
		pHalKernelParam->uiKernelId = m_Id;
	}
	for (UINT OrgArgIndex = 0; OrgArgIndex < m_ArgCount; OrgArgIndex++) {
		ArgIndexStep = 1;

		if (CHECK_SURFACE_TYPE(m_Args[OrgArgIndex].unitKind,
				       ARG_KIND_SURFACE,
				       ARG_KIND_SURFACE_1D,
				       ARG_KIND_SURFACE_2D,
				       ARG_KIND_SURFACE_2D_UP,
				       ARG_KIND_SURFACE_2D_DUAL)) {
			ArgIndexStep =
			    m_Args[OrgArgIndex].unitSize / sizeof(int);
		}

		if (m_Args[OrgArgIndex].bIsDirty) {
			if (CHECK_SURFACE_TYPE
			    (m_Args[OrgArgIndex].unitKind, ARG_KIND_SURFACE,
			     ARG_KIND_SURFACE_1D, ARG_KIND_SURFACE_2D,
			     ARG_KIND_SURFACE_2D_UP,
			     ARG_KIND_SURFACE_2D_DUAL)) {

				UINT num_surfaces =
				    m_Args[OrgArgIndex].unitSize / sizeof(int);
				if (m_Args[OrgArgIndex].unitCount == 1) {
					for (UINT kk = 0; kk < num_surfaces;
					     kk++) {
						CM_ASSERT
						    (pHalKernelParam->CmArgParams
						     [ArgIndex +
						      kk].pFirstValue != NULL);
						CmFastMemCopy(pHalKernelParam->
							      CmArgParams
							      [ArgIndex +
							       kk].pFirstValue,
							      m_Args
							      [OrgArgIndex].pValue
							      +
							      kk * sizeof(UINT),
							      sizeof(UINT));

						pHalKernelParam->CmArgParams
						    [ArgIndex + kk].Kind =
						    (CM_HAL_KERNEL_ARG_KIND)
						    m_Args
						    [OrgArgIndex].unitKind;

					}
				} else {
					UINT num_surfaces =
					    m_Args[OrgArgIndex].unitSize /
					    sizeof(int);
					UINT *surfaces =
					    (UINT *) new(std::nothrow)
					    BYTE[sizeof(UINT) *
						 m_Args[OrgArgIndex].unitCount];
					CMCHK_NULL_RETURN(surfaces,
							  CM_OUT_OF_HOST_MEMORY);
					for (UINT kk = 0; kk < num_surfaces;
					     kk++) {
						for (UINT s = 0;
						     s <
						     m_Args
						     [OrgArgIndex].unitCount;
						     s++) {
							surfaces[s] =
							    *(UINT *) ((UINT *)
								       m_Args
								       [OrgArgIndex].pValue
								       + kk +
								       num_surfaces
								       * s);
						}
						CmFastMemCopy
						    (pHalKernelParam->
						     CmArgParams[ArgIndex +
								 kk].
						     pFirstValue, surfaces,
						     sizeof(UINT) *
						     m_Args
						     [OrgArgIndex].unitCount);

						pHalKernelParam->CmArgParams
						    [ArgIndex + kk].Kind =
						    (CM_HAL_KERNEL_ARG_KIND)
						    m_Args
						    [OrgArgIndex].unitKind;

					}
					CmSafeDeleteArray(surfaces);
				}

			} else {
				CMCHK_HR(CreateThreadArgData
					 (&pHalKernelParam->CmArgParams
					  [ArgIndex], OrgArgIndex,
					  pCmThreadSpace, isKernelThreadSpace,
					  m_Args));
			}
		}
		ArgIndex += ArgIndexStep;
	}

	if (m_pThreadSpace && m_pThreadSpace->GetDirtyStatus()) {

		CMCHK_HR(SortThreadSpace(m_pThreadSpace));

		UINT TsWidth, TsHeight;
		PCM_HAL_KERNEL_THREADSPACE_PARAM pCmKernelThreadSpaceParam =
		    &pHalKernelParam->CmKernelThreadSpaceParam;
		m_pThreadSpace->GetThreadSpaceSize(TsWidth, TsHeight);

		pCmKernelThreadSpaceParam->iThreadSpaceWidth = (WORD) TsWidth;
		pCmKernelThreadSpaceParam->iThreadSpaceHeight = (WORD) TsHeight;
		m_pThreadSpace->GetDependencyPatternType
		    (pCmKernelThreadSpaceParam->patternType);

		CM_DEPENDENCY *pDependency;
		m_pThreadSpace->GetDependency(pDependency);

		if (pDependency != NULL) {
			CmFastMemCopy
			    (&pCmKernelThreadSpaceParam->dependencyInfo,
			     pDependency, sizeof(CM_HAL_DEPENDENCY));
		}

		if (m_pThreadSpace->CheckWalkingParametersSet()) {
			CMCHK_HR(m_pThreadSpace->GetWalkingParameters
				 (pCmKernelThreadSpaceParam->walkingParams));
		}

		if (m_pThreadSpace->CheckDependencyVectorsSet()) {
			CMCHK_HR(m_pThreadSpace->GetDependencyVectors
				 (pCmKernelThreadSpaceParam->
				  dependencyVectors));
		}

		if (m_pThreadSpace->IsThreadAssociated()) {
			UINT *pBoardOrder = NULL;
			m_pThreadSpace->GetBoardOrder(pBoardOrder);
			CMCHK_NULL(pBoardOrder);

			CM_THREAD_SPACE_UNIT *pThreadSpaceUnit = NULL;
			m_pThreadSpace->GetThreadSpaceUnit(pThreadSpaceUnit);
			CMCHK_NULL(pThreadSpaceUnit);

			pCmKernelThreadSpaceParam->reuseBBUpdateMask = 0;
			for (UINT i = 0; i < TsWidth * TsHeight; i++) {
				pCmKernelThreadSpaceParam->pThreadCoordinates
				    [i].x =
				    pThreadSpaceUnit[pBoardOrder
						     [i]].scoreboardCoordinates.
				    x;
				pCmKernelThreadSpaceParam->pThreadCoordinates
				    [i].y =
				    pThreadSpaceUnit[pBoardOrder
						     [i]].scoreboardCoordinates.
				    y;
				pCmKernelThreadSpaceParam->pThreadCoordinates
				    [i].mask =
				    pThreadSpaceUnit[pBoardOrder
						     [i]].dependencyMask;
				pCmKernelThreadSpaceParam->pThreadCoordinates
				    [i].resetMask =
				    pThreadSpaceUnit[pBoardOrder[i]].reset;

				pCmKernelThreadSpaceParam->reuseBBUpdateMask |=
				    pThreadSpaceUnit[pBoardOrder[i]].reset;
			}

			if (pCmKernelThreadSpaceParam->patternType ==
			    CM_DEPENDENCY_WAVEFRONT26Z) {
				CM_HAL_WAVEFRONT26Z_DISPATCH_INFO dispatchInfo;
				m_pThreadSpace->GetWavefront26ZDispatchInfo
				    (dispatchInfo);

				if (pCmKernelThreadSpaceParam->
				    dispatchInfo.numWaves >=
				    dispatchInfo.numWaves) {
					pCmKernelThreadSpaceParam->
					    dispatchInfo.numWaves =
					    dispatchInfo.numWaves;
					CmFastMemCopy
					    (pCmKernelThreadSpaceParam->dispatchInfo.
					     pNumThreadsInWave,
					     dispatchInfo.pNumThreadsInWave,
					     dispatchInfo.numWaves *
					     sizeof(UINT));
				} else {
					pCmKernelThreadSpaceParam->
					    dispatchInfo.numWaves =
					    dispatchInfo.numWaves;
					CmSafeDeleteArray
					    (pCmKernelThreadSpaceParam->dispatchInfo.
					     pNumThreadsInWave);
					pCmKernelThreadSpaceParam->
					    dispatchInfo.pNumThreadsInWave =
					    new(std::
						nothrow)
					    UINT[dispatchInfo.numWaves];
					CMCHK_NULL_RETURN
					    (pCmKernelThreadSpaceParam->dispatchInfo.
					     pNumThreadsInWave,
					     CM_OUT_OF_HOST_MEMORY);
					CmFastMemCopy
					    (pCmKernelThreadSpaceParam->dispatchInfo.
					     pNumThreadsInWave,
					     dispatchInfo.pNumThreadsInWave,
					     dispatchInfo.numWaves *
					     sizeof(UINT));
				}
			}
		}
	}
	if (m_Dirty & CM_KERNEL_DATA_PAYLOAD_DATA_DIRTY) {
		pHalKernelParam->CmIndirectDataParam.iIndirectDataSize =
		    m_usKernelPayloadDataSize;
		pHalKernelParam->CmIndirectDataParam.iSurfaceCount =
		    m_usKernelPayloadSurfaceCount;

		if (m_usKernelPayloadDataSize != 0) {
			if (m_Dirty & CM_KERNEL_DATA_PAYLOAD_DATA_SIZE_DIRTY) {
				CmSafeDeleteArray
				    (pHalKernelParam->CmIndirectDataParam.
				     pIndirectData);
				pHalKernelParam->
				    CmIndirectDataParam.pIndirectData =
				    new(std::nothrow)
				    BYTE[m_usKernelPayloadDataSize];
				CMCHK_NULL_RETURN
				    (pHalKernelParam->CmIndirectDataParam.
				     pIndirectData, CM_OUT_OF_HOST_MEMORY);
			}
			CmFastMemCopy(pHalKernelParam->
				      CmIndirectDataParam.pIndirectData,
				      (void *)m_pKernelPayloadData,
				      m_usKernelPayloadDataSize);
		}

		if (m_usKernelPayloadSurfaceCount != 0) {
			if (m_Dirty & CM_KERNEL_DATA_PAYLOAD_DATA_SIZE_DIRTY) {
				CmSafeDeleteArray
				    (pHalKernelParam->CmIndirectDataParam.
				     pSurfaceInfo);
				pHalKernelParam->
				    CmIndirectDataParam.pSurfaceInfo =
				    new(std::nothrow)
				    CM_INDIRECT_SURFACE_INFO
				    [m_usKernelPayloadSurfaceCount];
				CMCHK_NULL_RETURN
				    (pHalKernelParam->CmIndirectDataParam.
				     pSurfaceInfo, CM_OUT_OF_HOST_MEMORY);

			}
			CmFastMemCopy((void *)
				      pHalKernelParam->CmIndirectDataParam.
				      pSurfaceInfo,
				      (void *)m_IndirectSurfaceInfoArray,
				      m_usKernelPayloadSurfaceCount *
				      sizeof(CM_INDIRECT_SURFACE_INFO));
		}
	}

	CMCHK_HR(UpdateKernelDataGlobalSurfaceInfo(pHalKernelParam));

	CMCHK_HR(CalculateKernelSurfacesNum
		 (surfNum, pHalKernelParam->iNumSurfaces));

 finish:
	if (hr != CM_SUCCESS) {
		if (pHalKernelParam) {
			CmSafeDeleteArray(pHalKernelParam->CmIndirectDataParam.
					  pIndirectData);
			CmSafeDeleteArray(pHalKernelParam->CmIndirectDataParam.
					  pSurfaceInfo);
		}
	}
	return hr;
}

INT CmKernel::UpdateKernelData(CmKernelData * pKernelData,
			       const CmThreadGroupSpace * pTGS)
{
	INT hr = CM_SUCCESS;
	PCM_HAL_KERNEL_PARAM pHalKernelParam = NULL;
	UINT ArgIndexStep = 0;
	UINT ArgIndex = 0;
	UINT surfNum = 0;

	CMCHK_NULL(pKernelData);
	CM_ASSERT(pKernelData->IsInUse() == FALSE);

	pHalKernelParam = pKernelData->GetHalCmKernelData();
	CMCHK_NULL(pHalKernelParam);

	CMCHK_NULL(pTGS);

	for (UINT OrgArgIndex = 0; OrgArgIndex < m_ArgCount; OrgArgIndex++) {
		ArgIndexStep = 1;

		if (CHECK_SURFACE_TYPE(m_Args[OrgArgIndex].unitKind,
				       ARG_KIND_SURFACE,
				       ARG_KIND_SURFACE_1D,
				       ARG_KIND_SURFACE_2D,
				       ARG_KIND_SURFACE_2D_UP,
				       ARG_KIND_SURFACE_2D_DUAL)) {
			ArgIndexStep =
			    m_Args[OrgArgIndex].unitSize / sizeof(int);
		}

		if (m_Args[OrgArgIndex].bIsDirty) {
			if (m_Args[OrgArgIndex].unitCount > 1) {
				CM_ASSERT(0);
				hr = CM_FAILURE;
				goto finish;
			}

			if (CHECK_SURFACE_TYPE
			    (m_Args[OrgArgIndex].unitKind, ARG_KIND_SURFACE,
			     ARG_KIND_SURFACE_1D, ARG_KIND_SURFACE_2D,
			     ARG_KIND_SURFACE_2D_UP,
			     ARG_KIND_SURFACE_2D_DUAL)) {
				UINT num_surfaces =
				    m_Args[OrgArgIndex].unitSize / sizeof(int);
				if (m_Args[OrgArgIndex].unitCount == 1) {
					for (UINT kk = 0; kk < num_surfaces;
					     kk++) {
						CM_ASSERT
						    (pHalKernelParam->CmArgParams
						     [ArgIndex +
						      kk].pFirstValue != NULL);
						CmFastMemCopy(pHalKernelParam->
							      CmArgParams
							      [ArgIndex +
							       kk].pFirstValue,
							      m_Args
							      [OrgArgIndex].pValue
							      +
							      kk * sizeof(UINT),
							      sizeof(UINT));

						pHalKernelParam->CmArgParams
						    [ArgIndex + kk].Kind =
						    (CM_HAL_KERNEL_ARG_KIND)
						    m_Args
						    [OrgArgIndex].unitKind;
					}
				}
			} else {
				CMCHK_HR(CreateThreadArgData
					 (&pHalKernelParam->CmArgParams
					  [ArgIndex], OrgArgIndex, NULL, FALSE,
					  m_Args));
			}
		}
		ArgIndex += ArgIndexStep;
	}

	CMCHK_HR(UpdateKernelDataGlobalSurfaceInfo(pHalKernelParam));

	CMCHK_HR(CalculateKernelSurfacesNum
		 (surfNum, pHalKernelParam->iNumSurfaces));

	UINT thrdSpaceWidth, thrdSpaceHeight, grpSpaceWidth, grpSpaceHeight;
	pTGS->GetThreadGroupSpaceSize(thrdSpaceWidth, thrdSpaceHeight,
				      grpSpaceWidth, grpSpaceHeight);

	CMCHK_HR(CreateKernelArgDataGroup
		 (pHalKernelParam->CmArgParams[ArgIndex + 0].pFirstValue,
		  thrdSpaceWidth));
	CMCHK_HR(CreateKernelArgDataGroup
		 (pHalKernelParam->CmArgParams[ArgIndex + 1].pFirstValue,
		  thrdSpaceHeight));
	CMCHK_HR(CreateKernelArgDataGroup
		 (pHalKernelParam->CmArgParams[ArgIndex + 2].pFirstValue,
		  grpSpaceWidth));
	CMCHK_HR(CreateKernelArgDataGroup
		 (pHalKernelParam->CmArgParams[ArgIndex + 3].pFirstValue,
		  grpSpaceHeight));
	CMCHK_HR(CreateKernelArgDataGroup
		 (pHalKernelParam->CmArgParams[ArgIndex + 4].pFirstValue,
		  thrdSpaceWidth));
	CMCHK_HR(CreateKernelArgDataGroup
		 (pHalKernelParam->CmArgParams[ArgIndex + 5].pFirstValue,
		  thrdSpaceHeight));

 finish:
	return hr;
}

INT CmKernel::CreateKernelIndirectData(PCM_HAL_INDIRECT_DATA_PARAM
				       pHalIndreictData)
{
	INT hr = CM_SUCCESS;

	pHalIndreictData->iIndirectDataSize = m_usKernelPayloadDataSize;
	pHalIndreictData->iSurfaceCount = m_usKernelPayloadSurfaceCount;

	if (pHalIndreictData->pIndirectData == NULL
	    && m_usKernelPayloadDataSize != 0) {
		pHalIndreictData->pIndirectData =
		    new(std::nothrow) BYTE[pHalIndreictData->iIndirectDataSize];
		CMCHK_NULL_RETURN(pHalIndreictData->pIndirectData,
				  CM_OUT_OF_HOST_MEMORY);
	}
	if (pHalIndreictData->pSurfaceInfo == NULL
	    && m_usKernelPayloadSurfaceCount != 0) {
		pHalIndreictData->pSurfaceInfo = new(std::nothrow)
		    CM_INDIRECT_SURFACE_INFO[pHalIndreictData->iSurfaceCount];
		CMCHK_NULL_RETURN(pHalIndreictData->pSurfaceInfo,
				  CM_OUT_OF_HOST_MEMORY);
	}

	if (m_usKernelPayloadDataSize != 0) {
		CmFastMemCopy(pHalIndreictData->pIndirectData,
			      (void *)m_pKernelPayloadData,
			      m_usKernelPayloadDataSize);
	}

	if (m_usKernelPayloadSurfaceCount != 0) {
		CmFastMemCopy((void *)pHalIndreictData->pSurfaceInfo,
			      (void *)m_IndirectSurfaceInfoArray,
			      m_usKernelPayloadSurfaceCount *
			      sizeof(CM_INDIRECT_SURFACE_INFO));
	}
 finish:
	if (hr != CM_SUCCESS) {
		if (pHalIndreictData->pIndirectData)
			CmSafeDeleteArray(pHalIndreictData->pIndirectData);
		if (pHalIndreictData->pSurfaceInfo)
			CmSafeDeleteArray(pHalIndreictData->pSurfaceInfo);
	}
	return hr;
}

INT CmKernel::UpdateLastKernelData(CmKernelData * &pKernelData)
{
	INT hr = CM_SUCCESS;

	if (pKernelData == NULL || m_pLastKernelData == pKernelData) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	if (m_pLastKernelData) {
		CmKernelData::Destroy(m_pLastKernelData);
	}

	m_pLastKernelData = pKernelData;
	m_pLastKernelData->Acquire();
	m_LastKernelDataSize = m_pLastKernelData->GetKernelDataSize();

	return hr;
}

INT CmKernel::SetIndexInTask(UINT index)
{
	m_IndexInTask = index;
	return CM_SUCCESS;
}

UINT CmKernel::GetIndexInTask(void)
{
	return m_IndexInTask;
}

INT CmKernel::SetAssociatedToTSFlag(BOOLEAN b)
{
	m_AssociatedToTS = b;
	return CM_SUCCESS;
}

INT CmKernel::AssociateThreadSpace(CmThreadSpace * &pThreadSpace)
{
	if (pThreadSpace == NULL) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}
	if (m_pThreadGroupSpace != NULL) {
		CM_ASSERT(0);
		return CM_INVALID_KERNEL_THREADSPACE;
	}

	bool TSChanged = FALSE;
	if (m_pThreadSpace) {
		if (m_pThreadSpace != pThreadSpace) {
			TSChanged = TRUE;
		}
	}

	m_pThreadSpace = pThreadSpace;

	if (TSChanged) {
		m_pThreadSpace->SetDirtyStatus(CM_THREAD_SPACE_DATA_DIRTY);
	}

	return CM_SUCCESS;
}

INT CmKernel::AssociateThreadGroupSpace(CmThreadGroupSpace * &pTGS)
{
	if (pTGS == NULL) {
		CM_ASSERT(0);
		return CM_INVALID_ARG_VALUE;
	}

	if (m_pThreadSpace != NULL) {
		CM_ASSERT(0);
		return CM_INVALID_KERNEL_THREADGROUPSPACE;
	}

	m_pThreadGroupSpace = pTGS;

	return CM_SUCCESS;
}

BOOLEAN CmKernel::IsThreadArgExisted()
{
	return (BOOLEAN) m_blPerThreadArgExists;
}

UINT CmKernel::GetSLMSize()
{
	return (UINT) m_pKernelInfo->kernelSLMSize;
}

CM_RT_API INT CmKernel::SetKernelPayloadData(size_t size, const void *pValue)
{
	if (m_blPerThreadArgExists) {
		CM_ASSERT(0);
		return CM_KERNELPAYLOAD_PERTHREADARG_MUTEX_FAIL;
	}

	if (m_blPerKernelArgExists) {
		CM_ASSERT(0);
		return CM_KERNELPAYLOAD_PERKERNELARG_MUTEX_FAIL;
	}
	if (size > m_pHalMaxValuesEx->iMaxIndirectDataSizePerKernel) {
		CM_ASSERT(0);
		return CM_KERNELPAYLOAD_SETTING_FAILURE;
	}
	if (pValue == NULL) {
		CM_ASSERT(0);
		return CM_KERNELPAYLOAD_SETTING_FAILURE;
	}
	if (m_pKernelPayloadData) {
		if (size == m_usKernelPayloadDataSize) {
			if (CmSafeMemCompare
			    (pValue, (const void *)m_pKernelPayloadData,
			     size) != 0) {
				m_Dirty |= CM_KERNEL_DATA_PAYLOAD_DATA_DIRTY;
			} else {
				return CM_SUCCESS;
			}
		} else {
			CmSafeDeleteArray(m_pKernelPayloadData);
			m_usKernelPayloadDataSize = 0;
			m_Dirty |=
			    (CM_KERNEL_DATA_PAYLOAD_DATA_DIRTY |
			     CM_KERNEL_DATA_PAYLOAD_DATA_SIZE_DIRTY);
		}
	}

	if (m_pKernelPayloadData == NULL) {
		m_pKernelPayloadData = new(std::nothrow) BYTE[size];
		if (!m_pKernelPayloadData) {
			CM_ASSERT(0);
			return CM_OUT_OF_HOST_MEMORY;
		}
	}

	CmFastMemCopy(m_pKernelPayloadData, pValue, size);
	m_usKernelPayloadDataSize = (WORD) size;

	return CM_SUCCESS;
}

CM_RT_API INT
    CmKernel::SetKernelPayloadSurface(UINT surfaceCount,
				      SurfaceIndex ** pSurfaces)
{
	if (m_blPerThreadArgExists) {
		CM_ASSERT(0);
		return CM_KERNELPAYLOAD_PERTHREADARG_MUTEX_FAIL;
	}

	if (m_blPerKernelArgExists) {
		CM_ASSERT(0);
		return CM_KERNELPAYLOAD_PERKERNELARG_MUTEX_FAIL;
	}
	if (surfaceCount > m_pCmDev->MaxIndirectSurfaceCount()) {
		CM_ASSERT(0);
		return CM_KERNELPAYLOAD_SETTING_FAILURE;
	}
	if (pSurfaces == NULL) {
		CM_ASSERT(0);
		return CM_KERNELPAYLOAD_SETTING_FAILURE;
	}
	if (surfaceCount == m_usKernelPayloadSurfaceCount) {
		if (CmSafeMemCompare
		    (pSurfaces, m_pKernelPayloadSurfaceArray,
		     surfaceCount * sizeof(SurfaceIndex *)) != 0) {
			m_Dirty |= CM_KERNEL_DATA_PAYLOAD_DATA_DIRTY;

		} else {
			return CM_SUCCESS;
		}
	} else {
		if (m_usKernelPayloadSurfaceCount) {
			CmSafeMemSet(m_pKernelPayloadSurfaceArray, 0,
				     m_usKernelPayloadSurfaceCount *
				     sizeof(SurfaceIndex *));
			CmSafeMemSet(m_IndirectSurfaceInfoArray, 0,
				     m_usKernelPayloadSurfaceCount *
				     sizeof(CM_INDIRECT_SURFACE_INFO));
			m_usKernelPayloadSurfaceCount = 0;
		}

		m_Dirty |=
		    (CM_KERNEL_DATA_PAYLOAD_DATA_DIRTY |
		     CM_KERNEL_DATA_PAYLOAD_DATA_SIZE_DIRTY);

	}

	CmFastMemCopy(m_pKernelPayloadSurfaceArray, pSurfaces,
		      surfaceCount * sizeof(SurfaceIndex *));

	CmSurfaceManager *pSurfaceMgr = NULL;
	m_pCmDev->GetSurfaceManager(pSurfaceMgr);
	CM_ASSERT(pSurfaceMgr);

	UINT index = 0;
	UINT handle = 0;
	CmSurface *pSurface = NULL;

	for (UINT i = 0; i < surfaceCount; i++) {
		index = m_pKernelPayloadSurfaceArray[i]->get_data();
		if (m_pCmDev->IsValidSurfaceIndex(index)) {
			m_IndirectSurfaceInfoArray[i].iBindingTableIndex =
			    (WORD) index;
		} else {
			CmSafeMemSet(m_pKernelPayloadSurfaceArray, 0,
				     surfaceCount * sizeof(SurfaceIndex *));
			CmSafeMemSet(m_IndirectSurfaceInfoArray, 0,
				     surfaceCount *
				     sizeof(CM_INDIRECT_SURFACE_INFO));
			m_usKernelPayloadSurfaceCount = 0;
			return CM_KERNELPAYLOAD_SURFACE_INVALID_BTINDEX;
		}

		pSurfaceMgr->GetSurface(index, pSurface);
		if (pSurface == NULL) {
			CM_ASSERT(pSurface);
			return CM_KERNELPAYLOAD_SETTING_FAILURE;
		}

		CmSurface2D *pSurf2D = NULL;
		if (pSurface->Type() == CM_ENUM_CLASS_TYPE_CMSURFACE2D) {
			pSurf2D = static_cast < CmSurface2D * >(pSurface);
			m_IndirectSurfaceInfoArray[i].iKind =
			    ARG_KIND_SURFACE_2D;
			pSurf2D->GetHandle(handle);
			m_IndirectSurfaceInfoArray[i].iSurfaceIndex =
			    (WORD) handle;
		} else {
			CmBuffer_RT *pCmBuffer = NULL;
			if (pSurface->Type() == CM_ENUM_CLASS_TYPE_CMBUFFER_RT) {
				pCmBuffer =
				    static_cast < CmBuffer_RT * >(pSurface);
				m_IndirectSurfaceInfoArray[i].iKind =
				    ARG_KIND_SURFACE_1D;
				pCmBuffer->GetHandle(handle);
				m_IndirectSurfaceInfoArray[i].iSurfaceIndex =
				    (WORD) handle;
			} else {
				CmSurface2DUP *pSurf2DUP = NULL;
				if (pSurface->Type() ==
				    CM_ENUM_CLASS_TYPE_CMSURFACE2DUP) {
					pSurf2DUP =
					    static_cast <
					    CmSurface2DUP * >(pSurface);
					m_IndirectSurfaceInfoArray[i].iKind =
					    ARG_KIND_SURFACE_2D_UP;
					pSurf2DUP->GetHandle(handle);
					m_IndirectSurfaceInfoArray
					    [i].iSurfaceIndex = (WORD) handle;
				} else {
					CmSafeMemSet
					    (m_pKernelPayloadSurfaceArray, 0,
					     surfaceCount *
					     sizeof(SurfaceIndex *));
					CmSafeMemSet(m_IndirectSurfaceInfoArray,
						     0,
						     surfaceCount *
						     sizeof
						     (CM_INDIRECT_SURFACE_INFO));
					m_usKernelPayloadSurfaceCount = 0;
					return CM_KERNELPAYLOAD_SETTING_FAILURE;
				}
			}
		}
	}

	m_usKernelPayloadSurfaceCount = (WORD) surfaceCount;

	return CM_SUCCESS;
}

CM_RT_API INT CmKernel::SetSurfaceBTI(SurfaceIndex * pSurface, UINT BTIndex)
{
	if (pSurface == NULL) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}
	if (!m_pCmDev->IsValidSurfaceIndex(BTIndex)) {
		CM_ASSERT(0);
		return CM_KERNELPAYLOAD_SURFACE_INVALID_BTINDEX;
	}

	CmSurfaceManager *pSurfaceMgr = NULL;
	m_pCmDev->GetSurfaceManager(pSurfaceMgr);
	CM_ASSERT(pSurfaceMgr);

	UINT index = pSurface->get_data();
	UINT handle = 0;

	CmSurface *pSurface_RT = NULL;
	pSurfaceMgr->GetSurface(index, pSurface_RT);
	if (pSurface_RT == NULL) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	CmSurface2D *pSurf2D = NULL;
	if (pSurface_RT->Type() == CM_ENUM_CLASS_TYPE_CMSURFACE2D) {
		pSurf2D = static_cast < CmSurface2D * >(pSurface_RT);
		m_IndirectSurfaceInfoArray[m_usKernelPayloadSurfaceCount].iKind
		    = ARG_KIND_SURFACE_2D;
		pSurf2D->GetHandle(handle);
		m_IndirectSurfaceInfoArray
		    [m_usKernelPayloadSurfaceCount].iSurfaceIndex =
		    (WORD) handle;
	} else {
		CmBuffer_RT *pCmBuffer = NULL;
		if (pSurface_RT->Type() == CM_ENUM_CLASS_TYPE_CMBUFFER_RT) {
			pCmBuffer = static_cast < CmBuffer_RT * >(pSurface_RT);
			m_IndirectSurfaceInfoArray
			    [m_usKernelPayloadSurfaceCount].iKind =
			    ARG_KIND_SURFACE_1D;
			pCmBuffer->GetHandle(handle);
			m_IndirectSurfaceInfoArray
			    [m_usKernelPayloadSurfaceCount].iSurfaceIndex =
			    (WORD) handle;
		} else {
			CmSurface2DUP *pSurf2DUP = NULL;
			if (pSurface_RT->Type() ==
			    CM_ENUM_CLASS_TYPE_CMSURFACE2DUP) {
				pSurf2DUP =
				    static_cast <
				    CmSurface2DUP * >(pSurface_RT);
				m_IndirectSurfaceInfoArray
				    [m_usKernelPayloadSurfaceCount].iKind =
				    ARG_KIND_SURFACE_2D_UP;
				pSurf2DUP->GetHandle(handle);
				m_IndirectSurfaceInfoArray
				    [m_usKernelPayloadSurfaceCount].
				    iSurfaceIndex = (WORD) handle;
			} else {
				return CM_FAILURE;
			}
		}
	}

	m_IndirectSurfaceInfoArray
	    [m_usKernelPayloadSurfaceCount].iBindingTableIndex = (WORD) BTIndex;

	if (m_pKernelPayloadSurfaceArray[m_usKernelPayloadSurfaceCount] == NULL) {
		m_pKernelPayloadSurfaceArray[m_usKernelPayloadSurfaceCount] =
		    pSurface;
	}

	m_usKernelPayloadSurfaceCount++;

	return CM_SUCCESS;
}

UINT CmKernel::GetKernelIndex()
{
	return m_kernelIndex;
}

UINT CmKernel::GetKernelGenxBinarySize(void)
{
	if (m_pKernelInfo == NULL) {
		CM_ASSERT(0);
		return 0;
	} else {
		return m_pKernelInfo->genxBinarySize;
	}
}

CM_ARG_KIND CmKernel::SurfTypeToArgKind(CM_ENUM_CLASS_TYPE SurfType)
{
	switch (SurfType) {
	case CM_ENUM_CLASS_TYPE_CMBUFFER_RT:
		return ARG_KIND_SURFACE_1D;
	case CM_ENUM_CLASS_TYPE_CMSURFACE2D:
		return ARG_KIND_SURFACE_2D;
	case CM_ENUM_CLASS_TYPE_CMSURFACE2DUP:
		return ARG_KIND_SURFACE_2D_UP;
	default:
		CM_ASSERT(0);
		break;
	}
	return ARG_KIND_GENERAL;
}

INT CmKernel::CalculateKernelSurfacesNum(UINT & kernelSurfaceNum,
					 UINT & neededBTEntryNum)
{
	UINT surfaceArraySize = 0;
	CmSurfaceManager *pSurfaceMgr = NULL;
	CmSurface *pSurf = NULL;
	CmSurface2D *pSurf2D = NULL;
	CmSurface2DUP *pSurf2D_UP = NULL;
	CM_SURFACE_FORMAT format;
	UINT width, height, bytesPerPixel;
	UINT uiMaxBTIndex = 0;

	kernelSurfaceNum = 0;
	neededBTEntryNum = 0;

	m_pCmDev->GetSurfaceManager(pSurfaceMgr);
	if (pSurfaceMgr == NULL) {
		CM_ASSERT(0);
		return CM_FAILURE;
	}

	surfaceArraySize = pSurfaceMgr->GetSurfacePoolSize();

	for (UINT surfIndex = 0; surfIndex < surfaceArraySize; surfIndex++) {
		if (m_SurfaceArray[surfIndex]) {
			pSurf = NULL;
			pSurfaceMgr->GetSurface(surfIndex, pSurf);
			if (pSurf) {
				switch (pSurf->Type()) {
				case CM_ENUM_CLASS_TYPE_CMBUFFER_RT:
					kernelSurfaceNum++;
					neededBTEntryNum++;
					break;

				case CM_ENUM_CLASS_TYPE_CMSURFACE2D:
					kernelSurfaceNum++;
					pSurf2D =
					    static_cast <
					    CmSurface2D * >(pSurf);
					pSurf2D->GetSurfaceDesc(width, height,
								format,
								bytesPerPixel);
					if (format == CM_SURFACE_FORMAT_NV12) {
						neededBTEntryNum += 2;
					} else {
						neededBTEntryNum += 1;
					}
					break;

				case CM_ENUM_CLASS_TYPE_CMSURFACE2DUP:
					kernelSurfaceNum++;
					pSurf2D_UP =
					    static_cast <
					    CmSurface2DUP * >(pSurf);
					pSurf2D_UP->GetSurfaceDesc(width,
								   height,
								   format,
								   bytesPerPixel);
					if (format == CM_SURFACE_FORMAT_NV12) {
						neededBTEntryNum += 2;
					} else {
						neededBTEntryNum += 1;
					}
					break;
				default:
					break;
				}
			}
		}
	}

	if ((uiMaxBTIndex + 1) > neededBTEntryNum) {
		neededBTEntryNum = uiMaxBTIndex + 1;
	}

	return CM_SUCCESS;
}

UINT CmKernel::GetAlignedCurbeSize(UINT value)
{
	UINT platform = IGFX_UNKNOWN_CORE;
	UINT CurbeBlockAlignment = 0;
	UINT CurbeAlignedSize = 0;

	m_pCmDev->GetGenPlatform(platform);

	switch (platform) {
	case IGFX_GEN7_5_CORE:
		CurbeBlockAlignment = GENHW_CURBE_BLOCK_ALIGN_G7;
		break;

	case IGFX_GEN8_CORE:
		CurbeBlockAlignment = GENHW_CURBE_BLOCK_ALIGN_G8;
		break;

	default:
		CurbeBlockAlignment = GENHW_CURBE_BLOCK_ALIGN_G7;
		break;
	}

	CurbeAlignedSize = GENOS_ALIGN_CEIL(value, CurbeBlockAlignment);
	return CurbeAlignedSize;
}
