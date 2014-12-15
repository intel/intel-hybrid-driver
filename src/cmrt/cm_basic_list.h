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

#pragma once

#include "cm_mem.h"
class CLinkedList {
 protected:

	class CNode {
 public:
		CNode(void);
		 CNode(void *&element);
		 virtual ~ CNode(void);

		void Insert(CNode * pNext);
		void Remove(void);

		void SetElement(void *&element);
		void *&GetElement(void);

		void SetPrevious(CNode * pNode);
		void SetNext(CNode * pNode);

		CNode *GetPrevious(void);
		CNode *GetNext(void);

 protected:
		void *m_Element;
		CNode *m_Next;
		CNode *m_Previous;
	};

 public:

	CLinkedList(void);
	virtual ~ CLinkedList(void);

	class CIterator {
 public:

		CIterator(void);
		 CIterator(CNode * ptr);
		 CIterator(const CIterator & iterator);

		 CIterator & operator--(void);
		 CIterator & operator++(void);

		bool operator==(const CIterator & iterator);
		bool operator!=(const CIterator & iterator);

		 CIterator & operator=(const CIterator & iterator);
		void *&operator*(void);

		bool IsNull(void);
		void SetNull(void);

		friend class CLinkedList;
		friend class CQueue;

 protected:
		 CNode * m_Ptr;
	};

	bool IsEmpty(void);
	DWORD GetCount(void);

	CLinkedList & operator=(CLinkedList & llist);

	CIterator Begin(void);
	CIterator Get(DWORD index);
	CIterator End(void);
	CIterator Find(void *&element);

	bool Add(void *&element);
	bool Remove(void *&element);
	bool Add(const CIterator & location, void *&element);
	bool Remove(const CIterator & location);
	void Clear(void);
	void Splice(CIterator & dstLocation,
		    CIterator & srcLocation, CLinkedList & srcList);

	void Set(CIterator & location, void *&element);
	void DeleteFreePool(void);

 protected:

	bool Remove(CNode * &pNode);

	CNode *CreateNode(void *&element);
	void DeleteNode(CNode * &pNode);

	CNode m_DummyTail;
	DWORD m_Count;

	CNode m_FreePoolDummyTail;
	DWORD m_FreePoolCount;

};
