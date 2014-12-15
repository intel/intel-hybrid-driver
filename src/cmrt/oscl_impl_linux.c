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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>

#include "oscl_impl_linux.h"
#include "os_utilities.h"

#define SUCCESS              1
#define ERROR                0

static DWORD dwLastError = ERROR_SUCCESS;
static VOID SetErrorNum(DWORD dwLastError);
static VOID TranslateLinuxErrno(INT iError);

HMODULE LoadLibrary(LPCSTR lpLibFileName)
{
	VOID *hModule;

	hModule = dlopen((CONST CHAR *) lpLibFileName, RTLD_LAZY);
	if (hModule == NULL) {
		TranslateLinuxErrno(errno);
	} else {
		SetErrorNum(ERROR_SUCCESS);
	}
	return (HMODULE) hModule;
}

BOOL FreeLibrary(HMODULE hLibModule)
{
	if (dlclose((VOID *) hLibModule) != 0) {
		SetErrorNum(ERROR_UNKNOWN);
		return ERROR;
	}

	SetErrorNum(ERROR_SUCCESS);
	return SUCCESS;
}

FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	VOID *pSym;

	pSym = dlsym((VOID *) hModule, (CHAR *) lpProcName);
	SetErrorNum(ERROR_SUCCESS);
	return (FARPROC) pSym;
}

static VOID SetErrorNum(DWORD Error)
{
	dwLastError = Error;
}

static VOID TranslateLinuxErrno(INT iErr)
{
	switch (iErr) {
	case 0:
		SetErrorNum(ERROR_SUCCESS);
		break;
	case ENOENT:
		SetErrorNum(ERROR_FILE_NOTFOUND);
		break;
	case EEXIST:
		SetErrorNum(ERROR_FILE_EXISTS);
		break;
	case EROFS:
		SetErrorNum(ERROR_FILE_READONLY);
		break;
	case EINVAL:
		SetErrorNum(ERROR_INVALID_PARAMETER);
		break;
	default:
		SetErrorNum(ERROR_UNKNOWN);
	}
	return;
}

extern "C" BOOL QueryPerformanceFrequency(LARGE_INTEGER * lpFrequency)
{
	struct timespec Res;
	INT iRet;

	if ((iRet = clock_getres(CLOCK_MONOTONIC, &Res)) != 0) {
		return ERROR;
	}
	if (Res.tv_sec != 0) {
		return ERROR;
	}
	lpFrequency->QuadPart = (1000 * 1000 * 1000) / Res.tv_nsec;

	SetErrorNum(ERROR_SUCCESS);
	return SUCCESS;
}

extern "C" BOOL QueryPerformanceCounter(LARGE_INTEGER * lpPerformanceCount)
{
	struct timespec Res;
	struct timespec t;
	INT iRet;

	if ((iRet = clock_getres(CLOCK_MONOTONIC, &Res)) != 0) {
		return ERROR;
	}
	if (Res.tv_sec != 0) {
		return ERROR;
	}
	if ((iRet = clock_gettime(CLOCK_MONOTONIC, &t)) != 0) {
		return ERROR;
	}
	lpPerformanceCount->QuadPart = (1000 * 1000 * 1000 * t.tv_sec +
					t.tv_nsec) / Res.tv_nsec;

	SetErrorNum(ERROR_SUCCESS);
	return SUCCESS;
}

INT strcpy_s(CHAR * pDestination, SIZE_T DstLength, CONST CHAR * pSource)
{
	if ((pDestination == NULL) || (pSource == NULL)) {
		return S_FALSE;
	}

	if (DstLength <= strlen(pSource)) {
		return S_FALSE;
	}

	strcpy(pDestination, pSource);

	return S_OK;
}
