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

#ifndef __OSCL_PLATFORM_DEF_H__
#define __OSCL_PLATFORM_DEF_H__

#include "os_defs.h"

#define UINT_MAX            0xffffffff
#define INFINITE            0xFFFFFFFF
#define MAX_PATH            128

#define NO_ERROR                            0L
#define ERROR_SUCCESS                       0L
#define ERROR_NO_SPACE                      1L
#define ERROR_INVALID_PARAMETER             2L
#define ERROR_INVALID_HANDLE                3L
#define ERROR_FILE_EXISTS                   4L
#define ERROR_FILE_NOTFOUND                 5L
#define ERROR_FILE_READONLY                 6L
#define ERROR_UNKNOWN                       16L
#define ERROR_MORE_DATA                     234L

#define _snprintf                                           snprintf
#define _vsnprintf                                          vsnprintf
#define _stprintf                                           sprintf
#define _sntprintf                                          snprintf

#define vsprintf_s(pBuffer, size, format, arg)              vsprintf(pBuffer, format, arg)
#define sprintf_s(pBuffer, size, format, arg)               sprintf(pBuffer, format, arg)

#define FAILED(hr)                                          (((HRESULT)(hr)) < 0)

#endif
