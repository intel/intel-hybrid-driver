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

#include "os_utilities.h"
#include "os_util_debug.h"
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <math.h>

#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <signal.h>
#include <malloc.h>
#include "string.h"
#include <unistd.h>

INT32 GenOsMemAllocCounter;
#define GENOS_MEMNINJA_ALLOC_MESSAGE(ptr, size, functionName, filename, line)                   \
   GENOS_OS_VERBOSEMESSAGE(                                                                     \
       "<MemNinjaSysAllocPtr memPtr = \"%d\" size = \"%d\" memType = \"Sys\"/>.", ptr, size); \
   GENOS_OS_VERBOSEMESSAGE(                                                                     \
       "<MemNinjaSysLastFuncCall memPtr = \"%d\" functionName = \"%s\" filename = \"%s\" "    \
       "memType = \"Sys\" line = \"%d\"/>.", ptr, functionName, filename, line);

#define GENOS_MEMNINJA_FREE_MESSAGE(ptr)                                                        \
   GENOS_OS_VERBOSEMESSAGE("GenOsMemAllocCounter = %d, Addr = 0x%x.", GenOsMemAllocCounter, ptr);   \
   GENOS_OS_VERBOSEMESSAGE("<MemNinjaSysFreePtr memPtr = \"%d\" memType = \"Sys\"/>.", ptr);

#define _aligned_malloc(size, alignment)  memalign(alignment, size)
#define _aligned_free(ptr)                free(ptr)

PVOID GENOS_AlignedAllocMemory(SIZE_T size, SIZE_T alignment)
{
	PVOID ptr;

	ptr = _aligned_malloc(size, alignment);

	GENOS_OS_ASSERT(ptr != NULL);

	if (ptr != NULL) {
		GENOS_MEMNINJA_ALLOC_MESSAGE(ptr, size, functionName, filename,
					     line);
		GenOsMemAllocCounter++;
	}

	return ptr;
}

VOID GENOS_AlignedFreeMemory(PVOID ptr)
{
	GENOS_OS_ASSERT(ptr != NULL);

	if (ptr != NULL) {
		GenOsMemAllocCounter--;

		GENOS_MEMNINJA_FREE_MESSAGE(ptr);

		_aligned_free(ptr);
	}
}

PVOID GENOS_AllocMemory(SIZE_T size)
{
	PVOID ptr;

	ptr = malloc(size);

	GENOS_OS_ASSERT(ptr != NULL);

	if (ptr != NULL) {
		GENOS_MEMNINJA_ALLOC_MESSAGE(ptr, size, functionName, filename,
					     line);
		GenOsMemAllocCounter++;
	}

	return ptr;
}

PVOID GENOS_AllocAndZeroMemory(SIZE_T size)
{
	PVOID ptr;

	ptr = malloc(size);

	GENOS_OS_ASSERT(ptr != NULL);

	if (ptr != NULL) {
		GENOS_ZeroMemory(ptr, size);

		GENOS_MEMNINJA_ALLOC_MESSAGE(ptr, size, functionName, filename,
					     line);

		GenOsMemAllocCounter++;
	}

	return ptr;
}

VOID GENOS_FreeMemory(PVOID ptr)
{
	if (ptr != NULL) {
		GenOsMemAllocCounter--;

		GENOS_MEMNINJA_FREE_MESSAGE(ptr);

		free(ptr);
	}
}

VOID GENOS_ZeroMemory(PVOID pDestination, SIZE_T stLength)
{
	GENOS_OS_ASSERT(pDestination != NULL);

	if (pDestination != NULL) {
		memset(pDestination, 0, stLength);
	}
}

VOID GENOS_FillMemory(PVOID pDestination, SIZE_T stLength, UINT8 bFill)
{
	GENOS_OS_ASSERT(pDestination != NULL);

	if (pDestination != NULL) {
		memset(pDestination, bFill, stLength);
	}
}

GENOS_STATUS GENOS_SecureStrcat(PCHAR strDestination, SIZE_T numberOfElements,
				const PCCHAR strSource)
{
	if ((strDestination == NULL) || (strSource == NULL)) {
		return GENOS_STATUS_INVALID_PARAMETER;
	}

	if (strnlen(strDestination, numberOfElements) == numberOfElements) {
		return GENOS_STATUS_INVALID_PARAMETER;
	}

	if ((strlen(strDestination) + strlen(strSource)) >= numberOfElements) {
		return GENOS_STATUS_INVALID_PARAMETER;
	}

	strcat(strDestination, strSource);
	return GENOS_STATUS_SUCCESS;
}

PCHAR GENOS_SecureStrtok(PCHAR strToken, PCCHAR strDelimit, PCHAR * contex)
{
	return strtok_r(strToken, strDelimit, contex);
}

GENOS_STATUS GENOS_SecureStrcpy(PCHAR strDestination, SIZE_T numberOfElements,
				const PCCHAR strSource)
{
	if ((strDestination == NULL) || (strSource == NULL)) {
		return GENOS_STATUS_INVALID_PARAMETER;
	}

	if (numberOfElements <= strlen(strSource)) {
		return GENOS_STATUS_INVALID_PARAMETER;
	}

	strcpy(strDestination, strSource);

	return GENOS_STATUS_SUCCESS;
}

GENOS_STATUS GENOS_SecureMemcpy(PVOID pDestination, SIZE_T dstLength,
				PCVOID pSource, SIZE_T srcLength)
{
	if ((pDestination == NULL) || (pSource == NULL)) {
		return GENOS_STATUS_INVALID_PARAMETER;
	}

	if (dstLength < srcLength) {
		return GENOS_STATUS_INVALID_PARAMETER;
	}

	memcpy(pDestination, pSource, srcLength);

	return GENOS_STATUS_SUCCESS;
}

INT32 GENOS_SecureStringPrint(PCHAR buffer, SIZE_T bufSize, SIZE_T length,
			      const PCCHAR format, ...)
{
	INT32 iRet = -1;
	va_list var_args;

	if ((buffer == NULL) || (format == NULL) || (bufSize < length)) {
		return iRet;
	}

	va_start(var_args, format);

	iRet = vsnprintf(buffer, length, format, var_args);

	va_end(var_args);

	return iRet;
}

GENOS_STATUS GENOS_SecureVStringPrint(PCHAR buffer, SIZE_T bufSize,
				      SIZE_T length, const PCCHAR format,
				      va_list var_args)
{
	if ((buffer == NULL) || (format == NULL) || (bufSize < length)) {
		return GENOS_STATUS_INVALID_PARAMETER;
	}

	vsnprintf(buffer, length, format, var_args);

	return GENOS_STATUS_SUCCESS;
}

BOOL GENOS_CloseHandle(HANDLE hObject)
{
	BOOL iRet = FALSE;

	if (hObject != 0) {
		close((INT32) hObject);
		iRet = TRUE;
	}

	return iRet;
}

GENOS_STATUS GENOS_LoadLibrary(const PCCHAR lpLibFileName, PHMODULE phModule)
{
	if (lpLibFileName == NULL) {
		return GENOS_STATUS_INVALID_PARAMETER;
	}

	*phModule = dlopen((const PCHAR)lpLibFileName, RTLD_LAZY);

	return ((*phModule !=
		 NULL) ? GENOS_STATUS_SUCCESS :
		GENOS_STATUS_LOAD_LIBRARY_FAILED);
}

BOOL GENOS_FreeLibrary(HMODULE hLibModule)
{
	UINT32 iRet = 10;

	if (hLibModule != NULL) {
		iRet = dlclose(hLibModule);
	}
	return (iRet == 0) ? TRUE : FALSE;
}

PVOID GENOS_GetProcAddress(HMODULE hModule, PCCHAR lpProcName)
{
	PVOID pSym = NULL;

	if (hModule == NULL || lpProcName == NULL) {
		GENOS_OS_ASSERTMESSAGE("Invalid parameter.");
	} else {
		pSym = dlsym(hModule, lpProcName);
	}

	return pSym;
}

BOOL GENOS_QueryPerformanceFrequency(PUINT64 pFrequency)
{
	struct timespec Res;
	INT32 iRet;

	if (pFrequency == NULL) {
		return FALSE;
	}

	if ((iRet = clock_getres(CLOCK_MONOTONIC, &Res)) != 0) {
		return FALSE;
	}
	if (Res.tv_sec != 0) {
		return FALSE;
	}
	*pFrequency = (UINT64) ((1000 * 1000 * 1000) / Res.tv_nsec);

	return TRUE;
}

BOOL GENOS_QueryPerformanceCounter(PUINT64 pPerformanceCount)
{
	struct timespec Res;
	struct timespec t;
	INT32 iRet;

	if (pPerformanceCount == NULL) {
		return FALSE;
	}
	if ((iRet = clock_getres(CLOCK_MONOTONIC, &Res)) != 0) {
		return FALSE;
	}
	if (Res.tv_sec != 0) {
		return FALSE;
	}
	if ((iRet = clock_gettime(CLOCK_MONOTONIC, &t)) != 0) {
		return FALSE;
	}
	*pPerformanceCount =
	    (UINT64) ((1000 * 1000 * 1000 * t.tv_sec +
		       t.tv_nsec) / Res.tv_nsec);

	return TRUE;
}

VOID GENOS_Sleep(UINT32 mSec)
{
	usleep(1000 * mSec);
}

GENOS_THREADHANDLE GENOS_CreateThread(PVOID ThreadFunction, PVOID ThreadData)
{
	GENOS_THREADHANDLE Thread;

	if (0 !=
	    pthread_create(&Thread, NULL, (VOID * (*)(PVOID)) ThreadFunction,
			   ThreadData)) {
		Thread = 0;
	}

	return Thread;
}

PGENOS_MUTEX GENOS_CreateMutex()
{
	PGENOS_MUTEX pMutex;

	pMutex = (PGENOS_MUTEX) GENOS_AllocMemory(sizeof(*pMutex));
	if (pMutex != NULL) {
		if (pthread_mutex_init(pMutex, NULL)) {
			pMutex = NULL;
		}
	}

	return pMutex;
}

HRESULT GENOS_DestroyMutex(PGENOS_MUTEX pMutex)
{
	HRESULT hr = S_OK;

	if (pMutex) {
		if (pthread_mutex_destroy(pMutex)) {
			hr = E_FAIL;
		}
		GENOS_FreeMemory(pMutex);
	}

	return hr;
}

HRESULT GENOS_LockMutex(PGENOS_MUTEX pMutex)
{
	HRESULT hr = S_OK;

	if (pthread_mutex_lock(pMutex)) {
		hr = E_FAIL;
	}

	return hr;
}

HRESULT GENOS_UnlockMutex(PGENOS_MUTEX pMutex)
{
	HRESULT hr = S_OK;

	if (pthread_mutex_unlock(pMutex)) {
		hr = E_FAIL;
	}

	return hr;
}

PGENOS_SEMAPHORE GENOS_CreateSemaphore(UINT uiInitialCount, UINT uiMaximumCount)
{
	PGENOS_SEMAPHORE pSemaphore = NULL;

	pSemaphore = (PGENOS_SEMAPHORE) GENOS_AllocMemory(sizeof(*pSemaphore));
	if (sem_init(pSemaphore, 0, uiInitialCount)) {
		pSemaphore = NULL;
	}

	return pSemaphore;
}

HRESULT GENOS_DestroySemaphore(PGENOS_SEMAPHORE pSemaphore)
{
	GENOS_SafeFreeMemory(pSemaphore);

	return S_OK;
}

HRESULT GENOS_WaitSemaphore(PGENOS_SEMAPHORE pSemaphore, UINT uiMilliseconds)
{
	HRESULT hr = S_OK;

	if (uiMilliseconds == INFINITE) {
		if (sem_wait(pSemaphore)) {
			hr = E_FAIL;
		}
	} else {
		struct timespec time = {
			(LONG) uiMilliseconds / 1000000,
			((LONG) uiMilliseconds % 1000000) * 1000
		};

		if (sem_timedwait(pSemaphore, &time)) {
			hr = E_FAIL;
		}
	}

	return hr;
}

HRESULT GENOS_PostSemaphore(PGENOS_SEMAPHORE pSemaphore, UINT uiPostCount)
{
	HRESULT hr = S_OK;

	if (uiPostCount > 0) {
		while (uiPostCount--) {
			if (sem_post(pSemaphore)) {
				hr = E_FAIL;
				break;
			}
		}
	} else {
		hr = E_FAIL;
	}

	return hr;
}

VAStatus GENOS_StatusToOsResult(GENOS_STATUS eStatus)
{
	switch (eStatus) {
	case GENOS_STATUS_SUCCESS:
		return VA_STATUS_SUCCESS;
	case GENOS_STATUS_NO_SPACE:
		return VA_STATUS_ERROR_ALLOCATION_FAILED;
	case GENOS_STATUS_INVALID_PARAMETER:
		return VA_STATUS_ERROR_INVALID_PARAMETER;
	case GENOS_STATUS_INVALID_HANDLE:
		return VA_STATUS_ERROR_INVALID_BUFFER;
	case GENOS_STATUS_NULL_POINTER:
		return VA_STATUS_ERROR_INVALID_CONTEXT;
	default:
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}

	return VA_STATUS_ERROR_OPERATION_FAILED;
}

GENOS_STATUS OsResultToGENOS_Status(VAStatus eResult)
{
	switch (eResult) {
	case VA_STATUS_SUCCESS:
		return GENOS_STATUS_SUCCESS;
	case VA_STATUS_ERROR_ALLOCATION_FAILED:
		return GENOS_STATUS_NO_SPACE;
	case VA_STATUS_ERROR_INVALID_PARAMETER:
		return GENOS_STATUS_INVALID_PARAMETER;
	case VA_STATUS_ERROR_INVALID_BUFFER:
		return GENOS_STATUS_INVALID_HANDLE;
	case VA_STATUS_ERROR_INVALID_CONTEXT:
		return GENOS_STATUS_NULL_POINTER;
	default:
		return GENOS_STATUS_UNKNOWN;
	}

	return GENOS_STATUS_UNKNOWN;
}
