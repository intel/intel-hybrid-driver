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

#include <string.h>
#include <iostream>

#include <mmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include "cm_debug.h"
#include "os_utilities.h"

#include <smmintrin.h>

typedef uintptr_t UINT_PTR;
#define __fastcall
#define __noop

#ifdef __try
#undef __try
#endif
#define __try           try

#ifdef __except
#undef __except
#endif

#define  EXCEPTION_EXECUTE_HANDLER std::exception const& e

#define __except(e)     catch(e)

#ifndef __EXCEPTIONS
#define NO_EXCEPTION_HANDLING  1
#endif

#if NO_EXCEPTION_HANDLING
#define try         if (true)

#ifndef catch
#define catch(x)    if (false)
#endif

#define throw(...)
#endif

enum CPU_INSTRUCTION_LEVEL {
	CPU_INSTRUCTION_LEVEL_UNKNOWN,
	CPU_INSTRUCTION_LEVEL_MMX,
	CPU_INSTRUCTION_LEVEL_SSE,
	CPU_INSTRUCTION_LEVEL_SSE2,
	CPU_INSTRUCTION_LEVEL_SSE3,
	CPU_INSTRUCTION_LEVEL_SSE4,
	CPU_INSTRUCTION_LEVEL_SSE4_1,
	NUM_CPU_INSTRUCTION_LEVELS
};

typedef __m128 DQWORD;
typedef DWORD PREFETCH[8];
typedef DWORD CACHELINE[8];
typedef WORD DHWORD[32];

#define CmSafeDeleteArray(_ptr) {if(_ptr) {delete[] (_ptr); (_ptr)=0;}}
#define CmSafeDelete(_ptr)      {if(_ptr) {delete (_ptr); (_ptr)=0;}}
inline void CmSafeMemSet(void *dst, const int data, const size_t bytes);
inline int CmSafeMemCompare(const void *, const void *, const size_t);

inline bool IsPowerOfTwo(const size_t number);
inline void *Align(void *const ptr, const size_t alignment);
inline size_t GetAlignmentOffset(void *const ptr, const size_t alignSize);
inline bool IsAligned(void *ptr, const size_t alignSize);
inline size_t Round(const size_t value, const size_t size);

inline void CmFastMemCopy(void *dst, const void *src, const size_t bytes);
inline void CmFastMemCopyWC(void *dst, const void *src, const size_t bytes);
inline void CmFastMemCopyFromWC(void *dst, const void *src, const size_t bytes,
				CPU_INSTRUCTION_LEVEL cpuInstructionLevel);

inline void Prefetch(const void *ptr);
inline void FastMemCopy_SSE2(void *dst, void *src,
			     const size_t doubleQuadWords);

inline void FastMemCopy_SSE2_movntdq_movdqa(void *dst, void *src,
					    const size_t doubleQuadWords);
inline void FastMemCopy_SSE2_movdqu_movdqa(void *dst, void *src,
					   const size_t doubleQuadWords);
inline void FastMemCopy_SSE2_movntdq_movdqu(void *dst, const void *src,
					    const size_t doubleQuadWords);
#ifndef BIT
#define BIT( n )    ( 1 << (n) )
#endif

inline void CmSafeMemSet(void *dst, const int data, const size_t bytes)
{
	::memset(dst, data, bytes);
}

inline void CmDwordMemSet(void *dst, const DWORD data, const size_t bytes)
{
	DWORD *ptr = reinterpret_cast < DWORD * >(dst);
	DWORD size_in_dwords = (DWORD) (bytes >> 2);
	DWORD *maxPtr = ptr + size_in_dwords;
	while (ptr < maxPtr)
		*ptr++ = data;
}

inline void CmSafeMemCopy(void *pdst, const void *psrc, const size_t bytes)
{
	BYTE *p_dst = (BYTE *) pdst;
	BYTE *p_src = (BYTE *) psrc;

	GENOS_SecureMemcpy(p_dst, bytes, p_src, bytes);
}

inline int CmSafeMemCompare(const void *dst, const void *src,
			    const size_t bytes)
{
	return::memcmp(dst, src, bytes);
}

inline void Prefetch(const void *ptr)
{
	__asm__("prefetchnta 0(%0)"::"r"(ptr));
}

inline bool TestSSE4_1(void)
{
	bool success = true;

#ifndef NO_EXCEPTION_HANDLING
	__try {
#endif

		if (sizeof(void *) == 4) {
			__asm__ __volatile__(".byte 0x66");
			__asm__ __volatile__(".byte 0x0f");
			__asm__ __volatile__(".byte 0x38");
			__asm__ __volatile__(".byte 0x17");
			__asm__ __volatile__(".byte 0xc1");
		} else {
			success = true;
		}

#ifndef NO_EXCEPTION_HANDLING
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		success = false;
	}
#endif
	return success;
}

inline void GetCPUID(int CPUInfo[4], int InfoType)
{
#ifndef NO_EXCEPTION_HANDLING
	__try {
#endif

		if (sizeof(void *) == 4) {
			unsigned int local_eax, local_ebx, local_ecx, local_edx;
			__asm__ __volatile__("pushl %%ebx      \n\t" "cpuid  \n\t" "movl %%ebx, %1   \n\t" "popl %%ebx \n\t"	/* restore the old %ebx */
					     :"=a"(local_eax), "=r"(local_ebx),
					     "=c"(local_ecx), "=d"(local_edx)
					     :"a"(InfoType)
					     :"cc");
			CPUInfo[0] = local_eax;
			CPUInfo[1] = local_ebx;
			CPUInfo[2] = local_ecx;
			CPUInfo[3] = local_edx;
		} else {
			uint64_t local_rax, local_rbx, local_rcx, local_rdx;
			__asm__ __volatile__("push %%rbx      \n\t" "cpuid           \n\t" "mov %%rbx, %1   \n\t" "pop %%rbx       \n\t"	/* restore the old %ebx */
					     :"=a"(local_rax), "=r"(local_rbx),
					     "=c"(local_rcx), "=d"(local_rdx)
					     :"a"(InfoType)
					     :"cc");
			CPUInfo[0] = local_rax;
			CPUInfo[1] = local_rbx;
			CPUInfo[2] = local_rcx;
			CPUInfo[3] = local_rdx;
		}

#ifndef NO_EXCEPTION_HANDLING
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		return;
	}
#endif
}

inline CPU_INSTRUCTION_LEVEL GetCpuInstructionLevel(void)
{
	int CpuInfo[4];
	memset(CpuInfo, 0, 4 * sizeof(int));

	GetCPUID(CpuInfo, 1);

	CPU_INSTRUCTION_LEVEL CpuInstructionLevel =
	    CPU_INSTRUCTION_LEVEL_UNKNOWN;
	if ((CpuInfo[2] & BIT(19)) && TestSSE4_1()) {
		CpuInstructionLevel = CPU_INSTRUCTION_LEVEL_SSE4_1;
	} else if (CpuInfo[2] & BIT(1)) {
		CpuInstructionLevel = CPU_INSTRUCTION_LEVEL_SSE3;
	} else if (CpuInfo[3] & BIT(26)) {
		CpuInstructionLevel = CPU_INSTRUCTION_LEVEL_SSE2;
	} else if (CpuInfo[3] & BIT(25)) {
		CpuInstructionLevel = CPU_INSTRUCTION_LEVEL_SSE;
	} else if (CpuInfo[3] & BIT(23)) {
		CpuInstructionLevel = CPU_INSTRUCTION_LEVEL_MMX;
	}

	return CpuInstructionLevel;
}

inline size_t Round(const size_t value, const size_t size)
{

	CM_ASSERT(IsPowerOfTwo(size));
	size_t mask = size - 1;
	size_t roundedValue = (value + mask) & ~(mask);
	return roundedValue;

}

__inline size_t Max(size_t var0, size_t var1)
{
	return (var0 >= var1) ? var0 : var1;
}

inline bool IsAligned(void *ptr, const size_t alignSize)
{
	return (((size_t) ptr % alignSize) == 0);
}

inline bool IsPowerOfTwo(const size_t number)
{
	return ((number & (number - 1)) == 0);
}

inline size_t GetAlignmentOffset(void *const ptr, const size_t alignSize)
{
	CM_ASSERT(alignSize);

	DWORD offset = 0;

	if (IsPowerOfTwo(alignSize)) {
		offset =
		    DWORD(UINT_PTR(Align(ptr, alignSize)) - (UINT_PTR) (ptr));
	} else {
		const DWORD modulo = (DWORD) (UINT_PTR(ptr) % alignSize);

		if (modulo) {
			offset = (DWORD) alignSize - modulo;
		}
	}

	return offset;
}

inline void *Align(void *const ptr, const size_t alignment)
{
	CM_ASSERT(IsPowerOfTwo(alignment));

	return (void *)((((size_t) ptr) + alignment - 1) & ~(alignment - 1));
}

inline void FastMemCopy_SSE2_movntdq_movdqa(void *dst,
					    void *src,
					    const size_t doubleQuadWords)
{
	CM_ASSERT(IsAligned(dst, sizeof(DQWORD)));
	CM_ASSERT(IsAligned(src, sizeof(DQWORD)));

	const size_t DoubleQuadWordsPerPrefetch =
	    sizeof(PREFETCH) / sizeof(DQWORD);

	Prefetch((BYTE *) src);
	Prefetch((BYTE *) src + sizeof(PREFETCH));

	__m128i *dst128i = (__m128i *) dst;
	__m128i *src128i = (__m128i *) src;

	size_t count = doubleQuadWords;

	while (count >= DoubleQuadWordsPerPrefetch) {
		Prefetch((BYTE *) src128i + 2 * sizeof(PREFETCH));

		count -= DoubleQuadWordsPerPrefetch;

		for (size_t i = 0; i < DoubleQuadWordsPerPrefetch; i++) {
			_mm_stream_si128(dst128i++, _mm_load_si128(src128i++));
		}
	}

	while (count--) {
		_mm_stream_si128(dst128i++, _mm_load_si128(src128i++));
	}
}

inline void FastMemCopy_SSE2_movdqu_movdqa(void *dst,
					   void *src,
					   const size_t doubleQuadWords)
{
	CM_ASSERT(IsAligned(src, sizeof(DQWORD)));

	const size_t DoubleQuadWordsPerPrefetch =
	    sizeof(PREFETCH) / sizeof(DQWORD);

	Prefetch((BYTE *) src);
	Prefetch((BYTE *) src + sizeof(PREFETCH));

	__m128i *dst128i = (__m128i *) dst;
	__m128i *src128i = (__m128i *) src;

	size_t count = doubleQuadWords;

	while (count >= DoubleQuadWordsPerPrefetch) {
		Prefetch((BYTE *) src128i + 2 * sizeof(PREFETCH));

		count -= DoubleQuadWordsPerPrefetch;

		for (size_t i = 0; i < DoubleQuadWordsPerPrefetch; i++) {
			_mm_storeu_si128(dst128i++, _mm_load_si128(src128i++));
		}
	}

	while (count--) {
		_mm_storeu_si128(dst128i++, _mm_load_si128(src128i++));
	}
}

inline void FastMemCopy_SSE2_movntdq_movdqu(void *dst,
					    const void *src,
					    const size_t doubleQuadWords)
{
	CM_ASSERT(IsAligned(dst, sizeof(DQWORD)));

	const size_t DoubleQuadWordsPerPrefetch =
	    sizeof(PREFETCH) / sizeof(DQWORD);

	Prefetch((BYTE *) src);
	Prefetch((BYTE *) src + sizeof(PREFETCH));

	__m128i *dst128i = (__m128i *) dst;
	__m128i *src128i = (__m128i *) src;

	size_t count = doubleQuadWords;

	while (count >= DoubleQuadWordsPerPrefetch) {
		Prefetch((BYTE *) src128i + 2 * sizeof(PREFETCH));

		count -= DoubleQuadWordsPerPrefetch;

		for (size_t i = 0; i < DoubleQuadWordsPerPrefetch; i++) {
			_mm_stream_si128(dst128i++, _mm_loadu_si128(src128i++));
		}
	}

	while (count--) {
		_mm_stream_si128(dst128i++, _mm_loadu_si128(src128i++));
	}
}

inline void FastMemCopy_SSE2_movdqu_movdqu(void *dst,
					   const void *src,
					   const size_t doubleQuadWords)
{
	const size_t DoubleQuadWordsPerPrefetch =
	    sizeof(PREFETCH) / sizeof(DQWORD);

	Prefetch((BYTE *) src);
	Prefetch((BYTE *) src + sizeof(PREFETCH));

	__m128i *dst128i = (__m128i *) dst;
	__m128i *src128i = (__m128i *) src;

	size_t count = doubleQuadWords;

	while (count >= DoubleQuadWordsPerPrefetch) {
		Prefetch((BYTE *) src128i + 2 * sizeof(PREFETCH));

		count -= DoubleQuadWordsPerPrefetch;

		for (size_t i = 0; i < DoubleQuadWordsPerPrefetch; i++) {
			_mm_storeu_si128(dst128i++, _mm_loadu_si128(src128i++));
		}
	}

	while (count--) {
		_mm_storeu_si128(dst128i++, _mm_loadu_si128(src128i++));
	}
}

inline void FastMemCopy_SSE2(void *dst, void *src, const size_t doubleQuadWords)
{
	const bool isDstDoubleQuadWordAligned = IsAligned(dst, sizeof(DQWORD));
	const bool isSrcDoubleQuadWordAligned = IsAligned(src, sizeof(DQWORD));

	if (isSrcDoubleQuadWordAligned && isDstDoubleQuadWordAligned) {
		FastMemCopy_SSE2_movntdq_movdqa(dst, src, doubleQuadWords);
	} else if (isDstDoubleQuadWordAligned) {
		FastMemCopy_SSE2_movntdq_movdqu(dst, src, doubleQuadWords);
	} else if (isSrcDoubleQuadWordAligned) {
		FastMemCopy_SSE2_movdqu_movdqa(dst, src, doubleQuadWords);
	} else {
		FastMemCopy_SSE2_movdqu_movdqu(dst, src, doubleQuadWords);
	}
}

inline void CmFastMemCopy(void *dst, const void *src, const size_t bytes)
{
	BYTE *p_dst = (BYTE *) dst;
	BYTE *p_src = (BYTE *) src;

	size_t count = bytes;

	const size_t doubleQuadWords = count / sizeof(DQWORD);

	if (doubleQuadWords) {
		FastMemCopy_SSE2(p_dst, p_src, doubleQuadWords);

		p_dst += doubleQuadWords * sizeof(DQWORD);
		p_src += doubleQuadWords * sizeof(DQWORD);
		count -= doubleQuadWords * sizeof(DQWORD);
	}

	if (count) {
		GENOS_SecureMemcpy(p_dst, count, p_src, count);
	}
}

inline void CmFastMemCopyFromWC(void *dst, const void *src, const size_t bytes,
				CPU_INSTRUCTION_LEVEL cpuInstructionLevel)
{
	if (cpuInstructionLevel >= CPU_INSTRUCTION_LEVEL_SSE4_1) {
		BYTE *p_dst = (BYTE *) dst;
		BYTE *p_src = (BYTE *) src;

		size_t count = bytes;

		if (count >= sizeof(DHWORD)) {
			const size_t doubleHexWordAlignBytes =
			    GetAlignmentOffset(p_src, sizeof(DHWORD));

			if (doubleHexWordAlignBytes) {
				CmSafeMemCopy(p_dst, p_src,
					      doubleHexWordAlignBytes);

				p_dst += doubleHexWordAlignBytes;
				p_src += doubleHexWordAlignBytes;
				count -= doubleHexWordAlignBytes;
			}

			CM_ASSERT(IsAligned(p_src, sizeof(DHWORD)) == true);

			const size_t DoubleHexWordsToCopy =
			    count / sizeof(DHWORD);

			if (DoubleHexWordsToCopy) {
				const bool isDstDoubleQuadWordAligned =
				    IsAligned(p_dst, sizeof(DQWORD));

				__m128i *pMMSrc = (__m128i *) (p_src);
				__m128i *pMMDest =
				    reinterpret_cast < __m128i * >(p_dst);
				__m128i xmm0, xmm1, xmm2, xmm3;

				if (isDstDoubleQuadWordAligned) {
					for (size_t i = 0;
					     i < DoubleHexWordsToCopy; i++) {
						xmm0 =
						    _mm_stream_load_si128
						    (pMMSrc);
						xmm1 =
						    _mm_stream_load_si128(pMMSrc
									  + 1);
						xmm2 =
						    _mm_stream_load_si128(pMMSrc
									  + 2);
						xmm3 =
						    _mm_stream_load_si128(pMMSrc
									  + 3);
						pMMSrc += 4;

						_mm_store_si128(pMMDest, xmm0);
						_mm_store_si128(pMMDest + 1,
								xmm1);
						_mm_store_si128(pMMDest + 2,
								xmm2);
						_mm_store_si128(pMMDest + 3,
								xmm3);
						pMMDest += 4;

						p_dst += sizeof(DHWORD);
						p_src += sizeof(DHWORD);
						count -= sizeof(DHWORD);
					}
				} else {
					for (size_t i = 0;
					     i < DoubleHexWordsToCopy; i++) {
						xmm0 =
						    _mm_stream_load_si128
						    (pMMSrc);
						xmm1 =
						    _mm_stream_load_si128(pMMSrc
									  + 1);
						xmm2 =
						    _mm_stream_load_si128(pMMSrc
									  + 2);
						xmm3 =
						    _mm_stream_load_si128(pMMSrc
									  + 3);
						pMMSrc += 4;

						_mm_storeu_si128(pMMDest, xmm0);
						_mm_storeu_si128(pMMDest + 1,
								 xmm1);
						_mm_storeu_si128(pMMDest + 2,
								 xmm2);
						_mm_storeu_si128(pMMDest + 3,
								 xmm3);
						pMMDest += 4;

						p_dst += sizeof(DHWORD);
						p_src += sizeof(DHWORD);
						count -= sizeof(DHWORD);
					}
				}
			}
		}
		if (count) {
			CmSafeMemCopy(p_dst, p_src, count);
		}
	} else {
		CmFastMemCopy(dst, src, bytes);
	}
}

inline void CmFastMemCopyWC(void *dst, const void *src, const size_t bytes)
{
	BYTE *p_dst = (BYTE *) dst;
	BYTE *p_src = (BYTE *) src;

	size_t count = bytes;

	if (count >= sizeof(DQWORD)) {
		const size_t doubleQuadwordAlignBytes =
		    GetAlignmentOffset(p_dst, sizeof(DQWORD));

		if (doubleQuadwordAlignBytes) {
			GENOS_SecureMemcpy(p_dst, doubleQuadwordAlignBytes,
					   p_src, doubleQuadwordAlignBytes);

			p_dst += doubleQuadwordAlignBytes;
			p_src += doubleQuadwordAlignBytes;
			count -= doubleQuadwordAlignBytes;
		}

		const size_t doubleQuadWords = count / sizeof(DQWORD);

		if (doubleQuadWords) {
			CM_ASSERT(IsAligned(p_dst, sizeof(DQWORD)));

			const bool isSrcDoubleQuadWordAligned =
			    IsAligned(p_src, sizeof(DQWORD));

			if (isSrcDoubleQuadWordAligned) {
				FastMemCopy_SSE2_movntdq_movdqa(p_dst, p_src,
								doubleQuadWords);
			} else {
				FastMemCopy_SSE2_movntdq_movdqu(p_dst, p_src,
								doubleQuadWords);
			}

			p_dst += doubleQuadWords * sizeof(DQWORD);
			p_src += doubleQuadWords * sizeof(DQWORD);
			count -= doubleQuadWords * sizeof(DQWORD);
		}
	}
	if (count) {
		GENOS_SecureMemcpy(p_dst, count, p_src, count);
	}
}
