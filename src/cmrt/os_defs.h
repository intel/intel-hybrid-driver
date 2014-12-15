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
#ifndef __OS_DEFS_H__
#define __OS_DEFS_H__

#define BASIC_TYPES_DEFINED 1
#define BOOL_DEF            1

#include <stdio.h>

#define GENOS_ASSERT_ENABLED   (_DEBUG)

#define GENOS_MESSAGES_ENABLED (_DEBUG || GENOS_ASSERT_ENABLED)

#include <pthread.h>
#include <semaphore.h>

#ifndef WINDEF4LINUX_H
#define WINDEF4LINUX_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

typedef int HANDLE;
typedef void *HINSTANCE;
typedef void *HMODULE;
typedef int *PHANDLE;
typedef void VOID, *PVOID, *LPVOID;

typedef char INT8, *PINT8;
typedef char CHAR, *PCHAR, *LPSTR, TCHAR, *LPTSTR;
typedef const char *PCSTR, *LPCSTR, *LPCTSTR;
typedef unsigned char BYTE, *PBYTE, *LPBYTE;
typedef unsigned char UINT8, *PUINT8, UCHAR, *PUCHAR;
typedef int16_t INT16, *PINT16, SHORT, *PSHORT;
typedef uint16_t UINT16, *PUINT16, WORD, *PWORD, USHORT;

typedef int32_t INT, *PINT, INT32, *PINT32;
typedef int32_t HRESULT, LSTATUS;
typedef uint32_t UINT, *PUINT, UINT32, *PUINT32;
typedef uint32_t DWORD, *PDWORD, *LPDWORD;
typedef uint32_t BOOL, *PBOOL, BOOLEAN;

typedef int32_t LONG, *PLONG;
typedef int64_t INT64, *PINT64, LONGLONG, *PLONGLONG;
typedef uint32_t ULONG, *PULONG;
typedef uint64_t UINT64, *PUINT64, ULONGLONG;
typedef uint64_t ULONG64;
typedef uint64_t QWORD, *PQWORD;

typedef float FLOAT, *PFLOAT;
typedef double DOUBLE;

typedef size_t SIZE_T;

typedef uintptr_t ULONG_PTR;
typedef intptr_t INT_PTR;

typedef union _LARGE_INTEGER {
	struct {
		int32_t LowPart;
		int32_t HighPart;
	} u;
	int64_t QuadPart;
} LARGE_INTEGER;

typedef LARGE_INTEGER *PLARGE_INTEGER;

typedef struct tagRECT {
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
} RECT, *PRECT;

#define TRUE                    1
#define FALSE                   0

#define S_OK                    0
#define S_FALSE                 1
#define E_FAIL                  (0x80004005)
#define E_OUTOFMEMORY           (0x8007000E)

#define CONST const

#define __UNIQUENAME( a1, a2 )  __CONCAT( a1, a2 )
#define UNIQUENAME( __text )    __UNIQUENAME( __text, __COUNTER__ )
#define STATIC_ASSERT(e)  typedef char UNIQUENAME(STATIC_ASSERT_)[(e)?1:-1]
#define C_ASSERT(e) STATIC_ASSERT(e)

typedef INT_PTR(*PROC) ();
typedef INT_PTR(*FARPROC) ();
#endif

typedef pthread_mutex_t GENOS_MUTEX, *PGENOS_MUTEX;
typedef sem_t GENOS_SEMAPHORE, *PGENOS_SEMAPHORE;
typedef pthread_t GENOS_THREADHANDLE;

#define INFINITE                0xFFFFFFFF

typedef FILE *PFILE;
typedef FILE **PPFILE;
typedef HMODULE *PHMODULE;
typedef const void *PCVOID;
typedef void **PPVOID;
typedef const char *PCCHAR;
typedef char **PPCHAR;

#define GENOS_PAGE_SIZE                       4096

#define GENOS_PI                              3.14159265358979324f

#define GENOS_BYTES_TO_PAGES(_b)              ((_b + GENOS_PAGE_SIZE - 1)/(GENOS_PAGE_SIZE))

#define GENOS_PAGES_TO_BYTES(_p)              (_p*GENOS_PAGE_SIZE)

#define GENOS_KB_TO_BYTES(_kb)                (_kb*1024)

#define GENOS_MB_TO_BYTES(_mb)                (_mb*1024*1024)

#define GENOS_IS_ALIGNED(_a, _alignment)      (((_a & (_alignment-1)) == 0) ? 1 : 0)

#define GENOS_ALIGN_CEIL(_a, _alignment)      (((_a) + (_alignment-1)) & (~(_alignment-1)))

#define GENOS_ALIGN_FLOOR(_a, _alignment)     ((_a) & (~(_alignment-1)))

#define GENOS_ROUNDUP_SHIFT(_a, _shift)       (((_a) + (1 << (_shift)) - 1) >> (_shift))

#define GENOS_ROUNDUP_DIVIDE(_x, _divide)     (((_x) + (_divide) - 1) / (_divide))

#define GENOS_NUMBER_OF_ELEMENTS(_A)          (sizeof(_A) / sizeof((_A)[0]))

#define GENOS_MIN(_a, _b)                     (((_a) < (_b)) ? (_a) : (_b))

#define GENOS_MIN3(_a, _b, _c)                GENOS_MIN(GENOS_MIN(_a, _b), _c)

#define GENOS_MAX(_a, _b)                     (((_a) > (_b)) ? (_a) : (_b))

#define GENOS_MAX3(_a, _b, _c)                GENOS_MAX(GENOS_MAX(_a, _b), _c)

#define GENOS_MASKBITS32(_low, _high)         ((((((UINT32)1) << (_high+1)) - 1) >> _low) << _low)

#define GENOS_MASKBITS64(_low, _high)         ((((((UINT64)1) << (_high+1)) - 1) >> _low) << _low)

#define GENOS_BIT_ON(_a, _bit)                ((_a) |= (_bit))

#define GENOS_BIT_OFF(_a, _bit)               ((_a) &= ~(_bit))

#define GENOS_IS_BIT_SET(_a, _bit)            ((_a) & (_bit))

#define GENOS_ABS(_x)                         (((_x) > 0) ? (_x) : -(_x))

#define GENOS_WITHIN_RANGE(_x, _min, _max)  (((_x >= _min) && (_x <= _max)) ? (TRUE) : (FALSE))

#if defined(_MSC_VER)
#define GENOS_ALIGNED(_alignment) __declspec(align(_alignment))
#else
#define GENOS_ALIGNED(_alignment) __attribute__ ((aligned(_alignment)))
#endif

typedef enum _GENOS_STATUS {
	GENOS_STATUS_SUCCESS = 0,
	GENOS_STATUS_NO_SPACE = 1,
	GENOS_STATUS_INVALID_PARAMETER = 2,
	GENOS_STATUS_INVALID_HANDLE = 3,
	GENOS_STATUS_INVALID_FILE_SIZE = 4,
	GENOS_STATUS_NULL_POINTER = 5,
	GENOS_STATUS_FILE_EXISTS = 6,
	GENOS_STATUS_FILE_NOT_FOUND = 7,
	GENOS_STATUS_FILE_OPEN_FAILED = 8,
	GENOS_STATUS_FILE_READ_ONLY = 9,
	GENOS_STATUS_FILE_READ_FAILED = 10,
	GENOS_STATUS_FILE_WRITE_FAILED = 11,
	GENOS_STATUS_DIR_CREATE_FAILED = 12,
	GENOS_STATUS_SET_FILE_POINTER_FAILED = 13,
	GENOS_STATUS_LOAD_LIBRARY_FAILED = 14,
	GENOS_STATUS_MORE_DATA = 15,
	GENOS_STATUS_USER_CONTROL_MAX_NAME_SIZE = 16,
	GENOS_STATUS_USER_CONTROL_MIN_DATA_SIZE = 17,
	GENOS_STATUS_USER_CONTROL_MAX_DATA_SIZE = 18,
	GENOS_STATUS_READ_REGISTRY_FAILED = 19,
	GENOS_STATUS_REG_BADKEY = 20,
	GENOS_STATUS_REG_BADVALUE = 21,
	GENOS_STATUS_REG_NOT_READ = 22,
	GENOS_STATUS_REG_NOT_WRITE = 23,
	GENOS_STATUS_REG_TYPE_UNKNOWN = 24,
	GENOS_STATUS_REG_ITEM_UNKNOWN = 25,
	GENOS_STATUS_REG_KEYNAME_NOT_FOUND = 26,
	GENOS_STATUS_REG_KEYID_NOT_FOUND = 27,
	GENOS_STATUS_EVENT_WAIT_REGISTER_FAILED = 28,
	GENOS_STATUS_EVENT_WAIT_UNREGISTER_FAILED = 29,
	GENOS_STATUS_REG_KEY_CHANGE_NOTIFY_FAILED = 30,
	GENOS_STATUS_HLT_INIT_FAILED = 31,
	GENOS_STATUS_UNIMPLEMENTED = 32,
	GENOS_STATUS_EXCEED_MAX_BB_SIZE = 33,
	GENOS_STATUS_PLATFORM_NOT_SUPPORTED = 34,
	GENOS_STATUS_CLIENT_AR_NO_SPACE = 35,
	GENOS_STATUS_UNKNOWN = 36
} GENOS_STATUS;

#define GENOS_SUCCEEDED(_status)                                                               \
    (_status == GENOS_STATUS_SUCCESS)

#define GENOS_FAILED(_status)                                                                 \
    (_status != GENOS_STATUS_SUCCESS)

typedef enum _GENOS_GPU_CONTEXT {
	GENOS_GPU_CONTEXT_RENDER = 0,
	GENOS_GPU_CONTEXT_RENDER3 = 11,
	GENOS_GPU_CONTEXT_RENDER4 = 12,
	GENOS_GPU_CONTEXT_MAX,
	GENOS_GPU_CONTEXT_INVALID_HANDLE = GENOS_GPU_CONTEXT_MAX
} GENOS_GPU_CONTEXT;

#endif
