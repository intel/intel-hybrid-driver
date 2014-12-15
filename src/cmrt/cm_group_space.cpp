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
 */

#include "cm_kernel.h"
#include "cm_group_space.h"
#include "cm_device.h"

INT CmThreadGroupSpace::Create(CmDevice * pDevice, UINT index,
			       UINT thrdSpaceWidth, UINT thrdSpaceHeight,
			       UINT grpSpaceWidth, UINT grpSpaceHeight,
			       CmThreadGroupSpace * &pTGS)
{
	CM_HAL_MAX_VALUES *pHalMaxValues = NULL;
	CM_HAL_MAX_VALUES_EX *pHalMaxValuesEx = NULL;
	pDevice->GetHalMaxValues(pHalMaxValues, pHalMaxValuesEx);
	if ((thrdSpaceWidth == 0) || (thrdSpaceHeight == 0)
	    || (grpSpaceWidth == 0)
	    || (grpSpaceHeight == 0)
	    || (thrdSpaceHeight > MAX_THREAD_SPACE_HEIGHT_PERGROUP)
	    || (thrdSpaceWidth > MAX_THREAD_SPACE_WIDTH_PERGROUP)
	    || (thrdSpaceHeight * thrdSpaceWidth >
		pHalMaxValuesEx->iMaxUserThreadsPerThreadGroup)) {
		CM_ASSERTMESSAGE("Exceed thread group size limitation.");
		return CM_INVALID_THREAD_GROUP_SPACE;
	}

	INT result = CM_SUCCESS;
	pTGS =
	    new(std::nothrow) CmThreadGroupSpace(pDevice, index, thrdSpaceWidth,
						 thrdSpaceHeight, grpSpaceWidth,
						 grpSpaceHeight);
	if (pTGS) {
		result = pTGS->Initialize();
		if (result != CM_SUCCESS) {
			CmThreadGroupSpace::Destroy(pTGS);
		}
	} else {
		CM_ASSERT(0);
		result = CM_OUT_OF_HOST_MEMORY;
	}
	return result;
}

INT CmThreadGroupSpace::Destroy(CmThreadGroupSpace * &pTGS)
{
	CmSafeDelete(pTGS);
	pTGS = NULL;
	return CM_SUCCESS;
}

INT CmThreadGroupSpace::GetThreadGroupSpaceSize(UINT & thrdSpaceWidth,
						UINT & thrdSpaceHeight,
						UINT & grpSpaceWidth,
						UINT & grpSpaceHeight)
    const
{
	thrdSpaceWidth = m_threadSpaceWidth;
	thrdSpaceHeight = m_threadSpaceHeight;
	grpSpaceWidth = m_groupSpaceWidth;
	grpSpaceHeight = m_groupSpaceHeight;

	return CM_SUCCESS;
}

 CmThreadGroupSpace::CmThreadGroupSpace(CmDevice * pCmDev, UINT index, UINT thrdSpaceWidth, UINT thrdSpaceHeight, UINT grpSpaceWidth, UINT grpSpaceHeight):
m_pCmDev(pCmDev),
m_threadSpaceWidth(thrdSpaceWidth),
m_threadSpaceHeight(thrdSpaceHeight),
m_groupSpaceWidth(grpSpaceWidth),
m_groupSpaceHeight(grpSpaceHeight), m_IndexInTGSArray(index)
{
}

CmThreadGroupSpace::~CmThreadGroupSpace(void)
{
}

INT CmThreadGroupSpace::Initialize(void)
{
	return CM_SUCCESS;
}

UINT CmThreadGroupSpace::GetIndexInTGsArray(void)
{
	return m_IndexInTGSArray;
}
