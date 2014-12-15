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

#include "cm_mem.h"
#include "cm_basic_list.h"

CLinkedList::CNode::CNode(void)
{
	CmSafeMemSet(&m_Element, 0, sizeof(void *));

	m_Next = this;
	m_Previous = this;
}

CLinkedList::CNode::CNode(void *&element)
{
	m_Element = (element);
	m_Next = this;
	m_Previous = this;
}

CLinkedList::CNode::~CNode(void)
{
	CM_ASSERT(m_Next == this);
	CM_ASSERT(m_Previous == this);
}

void CLinkedList::CNode::Insert(CNode * pNext)
{
	m_Next = pNext;
	m_Previous = m_Next->m_Previous;

	m_Previous->m_Next = this;
	m_Next->m_Previous = this;
}

void CLinkedList::CNode::Remove(void)
{
	m_Previous->m_Next = m_Next;
	m_Next->m_Previous = m_Previous;

	m_Previous = this;
	m_Next = this;

	CmSafeMemSet(&m_Element, 0, sizeof(void *));
}

void CLinkedList::CNode::SetElement(void *&element)
{
	m_Element = (element);
}

void *&CLinkedList::CNode::GetElement(void)
{
	return m_Element;
}

void CLinkedList::CNode::SetPrevious(CNode * pNode)
{
	m_Previous = pNode;
}

void CLinkedList::CNode::SetNext(CNode * pNode)
{
	m_Next = pNode;
}

CLinkedList::CNode * CLinkedList::CNode::GetPrevious(void)
{
	return m_Previous;
}

CLinkedList::CNode * CLinkedList::CNode::GetNext(void)
{
	return m_Next;
}

CLinkedList::CLinkedList(void)
{
	m_Count = 0;
	m_FreePoolCount = 0;
	m_DummyTail.SetNext(&m_DummyTail);
	m_DummyTail.SetPrevious(&m_DummyTail);

}

CLinkedList::~CLinkedList(void)
{
	CM_ASSERT(IsEmpty());

	Clear();

	DeleteFreePool();

}

CLinkedList::CIterator::CIterator(void)
{
	m_Ptr = NULL;
}

CLinkedList::CIterator::CIterator(CNode * ptr)
{
	CM_ASSERT(ptr);
	m_Ptr = ptr;
}

CLinkedList::CIterator::CIterator(const CIterator & iterator)
{
	m_Ptr = iterator.m_Ptr;
}

CLinkedList::CIterator & CLinkedList::CIterator::operator--(void)
{
	m_Ptr = m_Ptr->GetPrevious();
	CM_ASSERT(m_Ptr);
	return *this;
}

CLinkedList::CIterator & CLinkedList::CIterator::operator++(void)
{
	m_Ptr = m_Ptr->GetNext();
	CM_ASSERT(m_Ptr);
	return *this;
}

bool CLinkedList::CIterator::operator==(const CIterator & iterator)
{
	return m_Ptr == iterator.m_Ptr;
}

bool CLinkedList::CIterator::operator!=(const CIterator & iterator)
{
	return m_Ptr != iterator.m_Ptr;
}

CLinkedList::CIterator & CLinkedList::CIterator::
operator=(const CIterator & iterator)
{
	if (m_Ptr != iterator.m_Ptr)
		m_Ptr = iterator.m_Ptr;
	return *this;
}

void *&CLinkedList::CIterator::operator*(void)
{
	return m_Ptr->GetElement();
}

bool CLinkedList::CIterator::IsNull(void)
{
	return (m_Ptr == NULL);
}

void CLinkedList::CIterator::SetNull(void)
{
	m_Ptr = NULL;
}

bool CLinkedList::IsEmpty(void)
{
	return (m_DummyTail.GetNext() == &m_DummyTail);
}

DWORD CLinkedList::GetCount(void)
{
	return m_Count;
}

CLinkedList & CLinkedList::operator=(CLinkedList & llist)
{
	CNode *pDummyTail = &llist.m_DummyTail;
	CNode *pNode = pDummyTail->GetPrevious();

	while (pNode != pDummyTail) {
		Add(pNode->GetElement());
		pNode = pNode->GetPrevious();
	}

	return *this;
}

CLinkedList::CIterator CLinkedList::Begin(void)
{
	return CIterator(m_DummyTail.GetNext());
}

CLinkedList::CIterator CLinkedList::Get(DWORD index)
{

	CIterator iterator;

	if (index <= (m_Count - 1)) {
		if (index <= (m_Count / 2)) {
			iterator = Begin();

			for (DWORD i = 0; i < index; i++) {
				++iterator;
			}
		} else {
			iterator = End();

			for (DWORD i = m_Count; i > index; i--) {
				--iterator;
			}
		}
	} else {
		iterator = End();
	}

	return iterator;
}

CLinkedList::CIterator CLinkedList::End(void)
{
	return CIterator(&m_DummyTail);
}

CLinkedList::CIterator CLinkedList::Find(void *&element)
{

	CNode *pDummyTail = &m_DummyTail;
	CNode *pNode = m_DummyTail.GetNext();

	while (pNode != pDummyTail) {
		if (pNode->GetElement() == element) {
			return CIterator(pNode);
		}
		pNode = pNode->GetNext();
	}

	return End();
}

bool CLinkedList::Add(void *&element)
{
	bool success = Add(Begin(), element);
	return success;
}

bool CLinkedList::Remove(void *&element)
{
	CIterator iterator = Find(element);
	bool success = Remove(iterator);
	return success;
}

bool CLinkedList::Add(const CIterator & location, void *&element)
{
	CM_ASSERT(Find(element) == End());

	bool success = false;

	CNode *pNode = CreateNode(element);

	if (pNode) {
		pNode->Insert(location.m_Ptr);

		m_Count++;

		success = true;
	} else {
		CM_ASSERT(0);

		success = false;
	}

	return success;
}

bool CLinkedList::Remove(const CIterator & location)
{

	bool success = false;

	if (End() == location) {
		success = false;
	} else {
		CNode *pNode = location.m_Ptr;

		success = Remove(pNode);
	}

	return success;
}

void CLinkedList::Clear(void)
{
	while (!IsEmpty()) {
		Remove(Begin());
	}
}

void CLinkedList::Splice(CIterator & dstLocation,
			 CIterator & srcLocation, CLinkedList & srcList)
{

	if (this != &srcList) {
		if (!srcList.IsEmpty()) {
			CIterator countIterator = srcLocation;
			DWORD count = 0;
			while (countIterator != srcList.End()) {
				count++;
				++countIterator;
			}

			m_Count += count;
			srcList.m_Count -= count;

			CNode *pDstNode = dstLocation.m_Ptr;

			CNode *pSrcStartNode = srcLocation.m_Ptr;
			CNode *pSrcEndNode = srcList.m_DummyTail.GetPrevious();

			pSrcStartNode->GetPrevious()->
			    SetNext(&srcList.m_DummyTail);
			srcList.m_DummyTail.
			    SetPrevious(pSrcStartNode->GetPrevious());

			pDstNode->GetPrevious()->SetNext(pSrcStartNode);
			pSrcStartNode->SetPrevious(pDstNode->GetPrevious());

			pSrcEndNode->SetNext(pDstNode);
			pDstNode->SetPrevious(pSrcEndNode);
		}
	}

}

void CLinkedList::Set(CIterator & location, void *&element)
{

	if (location.m_Ptr) {
		location.m_Ptr->SetElement(element);
	}

}

void CLinkedList::DeleteFreePool(void)
{
	while (m_FreePoolDummyTail.GetNext() != &m_FreePoolDummyTail) {
		CNode *pNode = m_FreePoolDummyTail.GetNext();
		if (pNode) {
			pNode->Remove();
			delete pNode;
		}
	}

	m_FreePoolCount = 0;
}

bool CLinkedList::Remove(CNode * &pNode)
{
	bool success = false;

	if (pNode) {
		pNode->Remove();

		DeleteNode(pNode);
		m_Count--;

		success = true;
	}

	return success;
}

CLinkedList::CNode * CLinkedList::CreateNode(void *&element)
{
	CNode *pNode = NULL;

	if (m_FreePoolDummyTail.GetNext() != &m_FreePoolDummyTail) {
		pNode = m_FreePoolDummyTail.GetNext();
		pNode->Remove();
		--m_FreePoolCount;

		pNode->SetElement(element);
	} else {
		pNode = new(std::nothrow) CNode(element);
		if (pNode == NULL) {
			CM_ASSERT(0);
			return NULL;
		}
	}

	return pNode;
}

void CLinkedList::DeleteNode(CNode * &pNode)
{
	DWORD cMaxFreePoolCount = 32;
	if (m_FreePoolCount <= cMaxFreePoolCount) {
		pNode->Insert(m_FreePoolDummyTail.GetNext());
		++m_FreePoolCount;
	} else {
		if (pNode)
			delete pNode;
	}
}
