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

#include "cm_array.h"
CmDynamicArray::CmDynamicArray(const DWORD initSize)
{
	m_pArrayBuffer = NULL;

	m_UsedSize = 0;
	m_ActualSize = 0;

	CreateArray(initSize);
}

CmDynamicArray::CmDynamicArray()
{
	m_pArrayBuffer = NULL;
	m_UsedSize = 0;
	m_ActualSize = 0;
}

CmDynamicArray::~CmDynamicArray(void)
{
	Delete();
}

void *CmDynamicArray::GetElement(const DWORD index)
{

	void *element;

	if (m_pArrayBuffer && IsValidIndex(index)) {
		element = m_pArrayBuffer[index];
	} else {
		CM_ASSERT(0);
		CmSafeMemSet(&element, 0, sizeof(void *));
	}
	return element;
}

bool CmDynamicArray::SetElement(const DWORD index, const void *element)
{

	bool success = false;

	if (!IsValidIndex(index)) {
		CreateArray(index + 1);
	}

	if (m_pArrayBuffer && IsValidIndex(index)) {
		m_pArrayBuffer[index] = (void *)element;
		success = true;
	}

	CM_ASSERT(success);
	return success;
}

DWORD CmDynamicArray::GetSize(void)
{
	const DWORD size = m_UsedSize;
	return size;
}

void CmDynamicArray::Delete(void)
{
	DeleteArray();
	m_UsedSize = 0;
}

CmDynamicArray & CmDynamicArray::operator=(const CmDynamicArray & array)
{

	if (array.m_pArrayBuffer) {
		if (m_UsedSize < array.m_UsedSize) {
			CreateArray(array.m_UsedSize);
		}

		if (m_pArrayBuffer && (m_UsedSize >= array.m_UsedSize)) {
			for (DWORD i = 0; i < array.m_UsedSize; i++) {
				m_pArrayBuffer[i] = array.m_pArrayBuffer[i];
			}
		}
	}

	return *this;
}

void CmDynamicArray::CreateArray(const DWORD size)
{
	if (size) {
		if (size > GetMaxSize()) {
			DWORD actualSize = GetMaxSize() * 2;

			if (size > actualSize) {
				actualSize = (DWORD) Round(Max(size, 32), 32);
			}

			CM_ASSERT(actualSize >= size);
			CM_ASSERT(actualSize > m_ActualSize);

			const DWORD allocSize = actualSize * sizeof(void *);

			void **pArrayBuffer =
			    new(std::nothrow) void *[allocSize];

			if (pArrayBuffer) {
				CmSafeMemSet(pArrayBuffer, 0, allocSize);

				if (m_pArrayBuffer) {
					for (DWORD i = 0; i < m_UsedSize; i++) {
						pArrayBuffer[i] =
						    m_pArrayBuffer[i];
					}

					DeleteArray();
				}

				m_pArrayBuffer = pArrayBuffer;
				m_ActualSize = actualSize;
				m_UsedSize = size;
			} else {
				CM_ASSERT(0);
				return;
			}
		} else {
			m_UsedSize = size;
		}
	}
}

void CmDynamicArray::DeleteArray(void)
{

	if (m_pArrayBuffer) {
		delete[]m_pArrayBuffer;
		m_pArrayBuffer = NULL;
	}

	m_ActualSize = 0;
}

DWORD CmDynamicArray::GetMaxSize(void)
{
	return m_ActualSize;
}

bool CmDynamicArray::IsValidIndex(const DWORD index)
{
	return (index < GetSize());
}

DWORD CmDynamicArray::GetFirstFreeIndex()
{
	DWORD index = 0;
	for (index = 0; index < GetMaxSize(); index++) {
		if (m_pArrayBuffer[index] == NULL) {
			return index;
		}
	}
	return index;
}

bool CmDynamicArray::SetElementIntoFreeSlot(const void *element)
{
	DWORD index = GetFirstFreeIndex();

	return SetElement(index, element);
}
