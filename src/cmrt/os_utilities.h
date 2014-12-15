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

#ifndef __GENOS_UTILITIES_H__
#define __GENOS_UTILITIES_H__

#include "os_defs.h"

#include <va/va.h>
typedef VAStatus GENOS_OSRESULT;
#include <stdarg.h>

#define GENOS_USER_CONTROL_MIN_DATA_SIZE         128
#define GENOS_USER_CONTROL_MAX_DATA_SIZE         2048
#define REG_FILE                            "/etc/igfx_registry.txt"

#ifdef __cplusplus
extern "C" {
#endif

	PVOID GENOS_AlignedAllocMemory(SIZE_T size, SIZE_T alignment);

	VOID GENOS_AlignedFreeMemory(PVOID ptr);

	PVOID GENOS_AllocMemory(SIZE_T size);

	PVOID GENOS_AllocAndZeroMemory(SIZE_T size);

	VOID GENOS_FreeMemory(PVOID ptr);

#define GENOS_FreeMemAndSetNull(ptr)            \
    GENOS_FreeMemory(ptr);                      \
    ptr = NULL;

#define GENOS_SafeFreeMemory(ptr)               \
    if (ptr) GENOS_FreeMemory(ptr);             \

	VOID GENOS_ZeroMemory(PVOID pDestination, SIZE_T stLength);

	VOID GENOS_FillMemory(PVOID pDestination, SIZE_T stLength, UINT8 bFill);

	GENOS_STATUS GENOS_GetFileSize(HANDLE hFile,
				       PUINT32 lpFileSizeLow,
				       PUINT32 lpFileSizeHigh);

	GENOS_STATUS GENOS_CreateDirectory(const PCHAR lpPathName);

	GENOS_STATUS GENOS_CreateFile(PHANDLE pHandle,
				      const PCHAR lpFileName, UINT32 iOpenFlag);

	GENOS_STATUS GENOS_ReadFile(HANDLE hFile,
				    PVOID lpBuffer,
				    UINT32 bytesToRead,
				    PUINT32 pbytesRead, PVOID lpOverlapped);

	GENOS_STATUS GENOS_WriteFile(HANDLE hFile,
				     PVOID lpBuffer,
				     UINT32 bytesToWrite,
				     PUINT32 pbytesWritten, PVOID lpOverlapped);

	GENOS_STATUS GENOS_SetFilePointer(HANDLE hFile,
					  INT32 lDistanceToMove,
					  PINT32 lpDistanceToMoveHigh,
					  INT32 dwMoveMethod);

	BOOL GENOS_CloseHandle(HANDLE hObject);

	GENOS_STATUS GENOS_AppendFileFromPtr(PCCHAR pFilename,
					     PVOID pData, DWORD dwSize);

	GENOS_STATUS GENOS_SecureStrcat(PCHAR strDestination,
					SIZE_T numberOfElements,
					const PCCHAR strSource);

	PCHAR GENOS_SecureStrtok(PCHAR strToken,
				 PCCHAR strDelimit, PCHAR * contex);

	GENOS_STATUS GENOS_SecureStrcpy(PCHAR strDestination,
					SIZE_T numberOfElements,
					const PCCHAR strSource);

	GENOS_STATUS GENOS_SecureMemcpy(PVOID pDestination,
					SIZE_T dstLength,
					PCVOID pSource, SIZE_T srcLength);

	GENOS_STATUS GENOS_SecureFileOpen(PPFILE ppFile,
					  PCCHAR filename, PCCHAR mode);

	INT32 GENOS_SecureStringPrint(PCHAR buffer,
				      SIZE_T bufSize,
				      SIZE_T length, const PCCHAR format, ...);

	GENOS_STATUS GENOS_SecureVStringPrint(PCHAR buffer,
					      SIZE_T bufSize,
					      SIZE_T length,
					      const PCCHAR format,
					      va_list var_args);
	GENOS_STATUS GENOS_LoadLibrary(const PCCHAR lpLibFileName,
				       PHMODULE phModule);

	BOOL GENOS_FreeLibrary(HMODULE hLibModule);

	PVOID GENOS_GetProcAddress(HMODULE hModule, PCCHAR lpProcName);

	INT32 GENOS_GetPid();

	BOOL GENOS_QueryPerformanceFrequency(PUINT64 pFrequency);

	BOOL GENOS_QueryPerformanceCounter(PUINT64 pPerformanceCount);

	VOID GENOS_Sleep(UINT32 mSec);

	HANDLE GENOS_CreateEventEx(PVOID lpEventAttributes,
				   PCHAR lpName, UINT32 dwFlags);

	BOOL GENOS_RegisterWaitForSingleObject(PHANDLE phNewWaitObject,
					       HANDLE hObject,
					       PVOID Callback, PVOID Context);

	BOOL GENOS_UnregisterWaitEx(HANDLE hWaitHandle);

	DWORD GENOS_GetLogicalCoreNumber();

	GENOS_THREADHANDLE GENOS_CreateThread(PVOID ThreadFunction,
					      PVOID ThreadData);

	UINT GENOS_GetThreadId(GENOS_THREADHANDLE hThread);

	UINT GENOS_GetCurrentThreadId();

	PGENOS_MUTEX GENOS_CreateMutex();

	HRESULT GENOS_DestroyMutex(PGENOS_MUTEX pMutex);

	HRESULT GENOS_LockMutex(PGENOS_MUTEX pMutex);

	HRESULT GENOS_UnlockMutex(PGENOS_MUTEX pMutex);

	PGENOS_SEMAPHORE GENOS_CreateSemaphore(UINT uiInitialCount,
					       UINT uiMaximumCount);

	HRESULT GENOS_DestroySemaphore(PGENOS_SEMAPHORE pSemaphore);

	HRESULT GENOS_WaitSemaphore(PGENOS_SEMAPHORE pSemaphore,
				    UINT uiMilliseconds);

	HRESULT GENOS_PostSemaphore(PGENOS_SEMAPHORE pSemaphore,
				    UINT uiPostCount);

	UINT GENOS_WaitForSingleObject(PVOID pObject, UINT uiMilliseconds);

	UINT GENOS_WaitForMultipleObjects(UINT uiThreadCount,
					  VOID ** ppObjects,
					  UINT bWaitAll, UINT uiMilliseconds);

	GENOS_OSRESULT GENOS_StatusToOsResult(GENOS_STATUS eStatus);

	GENOS_STATUS OsResultToGENOS_Status(GENOS_OSRESULT eResult);

#ifdef __cplusplus
}
#endif
#endif
