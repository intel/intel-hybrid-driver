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

#ifndef __CM_DEBUG_H__
#define __CM_DEBUG_H__

#include "os_util_debug.h"

#define CM_ASSERT(_expr)                                               \
    GENOS_ASSERT(GENOS_COMPONENT_CM, GENOS_CM_SUBCOMP_SELF, _expr)

#define CM_ASSERTMESSAGE(_message, ...)                                \
    GENOS_ASSERTMESSAGE(GENOS_COMPONENT_CM, GENOS_CM_SUBCOMP_SELF, _message, ##__VA_ARGS__)

#define CM_NORMALMESSAGE(_message, ...)                                \
    GENOS_NORMALMESSAGE(GENOS_COMPONENT_CM, GENOS_CM_SUBCOMP_SELF, _message, ##__VA_ARGS__)

#define CM_ERROR_ASSERT(message, ...)                                  \
    CM_ASSERTMESSAGE(message, ##__VA_ARGS__);                          \
    hr = GENOS_STATUS_UNKNOWN;

#define CM_ERROR_ASSERT_RETURN(_ret, message, ...)                     \
    CM_ASSERTMESSAGE(message, ##__VA_ARGS__);                          \
    hr = _ret;

#define CM_DDI_ASSERT(_expr)                                           \
    GENOS_ASSERT(GENOS_COMPONENT_CM, GENOS_CM_SUBCOMP_DDI, _expr)

#define CM_DDI_ASSERTMESSAGE(_message, ...)                            \
    GENOS_ASSERTMESSAGE(GENOS_COMPONENT_CM, GENOS_CM_SUBCOMP_DDI, _message, ##__VA_ARGS__)

#define CM_DDI_NORMALMESSAGE(_message, ...)                            \
    GENOS_NORMALMESSAGE(GENOS_COMPONENT_CM, GENOS_CM_SUBCOMP_DDI, _message, ##__VA_ARGS__)

#define CM_DDI_FUNCTION_ENTER                                          \
    GENOS_FUNCTION_ENTER(GENOS_COMPONENT_CM, GENOS_DDI_SUBCOMP_SELF)

#define CM_DDI_CHK_CONDITION(condition, _str, _ret)                    \
    if (condition) {                                                   \
        CM_DDI_ASSERTMESSAGE(_str);                                    \
        return _ret;                                                   \
    }

#define CM_DDI_CHK_NULL(_ptr, _str, _ret)                              \
    CM_DDI_CHK_CONDITION((NULL == (_ptr)), _str, _ret)

#define CM_CHK_LESS(p, upper, str, ret)                                \
    CM_DDI_CHK_CONDITION((p >= upper),str,ret)

#define CM_FAILED(_cmstatus)                ((CM_RETURN_CODE)(_cmstatus) != CM_SUCCESS)
#define GENOSSTATUS2CM(_genosstatus, _cmstatus)                            \
{                                                                      \
    switch((GENOS_STATUS)_genosstatus) {                                   \
        case GENOS_STATUS_SUCCESS:                                       \
            _cmstatus = CM_SUCCESS;                                    \
            break;                                                     \
        case GENOS_STATUS_NULL_POINTER:                                  \
            _cmstatus = CM_NULL_POINTER;                               \
            break;                                                     \
        case GENOS_STATUS_EXCEED_MAX_BB_SIZE:                            \
            _cmstatus = CM_TOO_MUCH_THREADS;                           \
            break;                                                     \
        default:                                                       \
            _cmstatus = (CM_RETURN_CODE)(0 - _genosstatus + CM_GENOS_STATUS_CONVERTED_CODE_OFFSET);   \
            break;                                                     \
    }                                                                  \
}

#define GENOSSTATUS2CM_AND_CHECK(_genosstatus, _cmstatus)                  \
{                                                                      \
    GENOSSTATUS2CM(_genosstatus, _cmstatus);                               \
    if (_cmstatus != CM_SUCCESS)                                       \
    {                                                                  \
        CM_ASSERT(0);                                                  \
        goto finish;                                                   \
    }                                                                  \
}

#ifndef CHK_GENOSSTATUS_RETURN_CMERROR
#define CHK_GENOSSTATUS_RETURN_CMERROR(_stmt)                             \
{                                                                       \
	GENOS_STATUS hr_genos = (GENOS_STATUS)(_stmt);                        \
	if (hr_genos != GENOS_STATUS_SUCCESS)                               \
	{                                                               \
		CM_ASSERT(0);						\
		GENOSSTATUS2CM(hr_genos, hr);				\
		goto finish;                                            \
	}                                                               \
}
#endif

#define CMCHK_HR(stmt)                                                  \
{                                                                       \
    hr = (CM_RETURN_CODE)(stmt);                                        \
    if (hr != CM_SUCCESS)                                               \
    {                                                                   \
    CM_ASSERT(0);                                                       \
    goto finish;                                                        \
    }                                                                   \
}

#define CMCHK_HR_MESSAGE(stmt, _str)                                    \
{                                                                       \
    hr = (stmt);                                                        \
    if (hr != CM_SUCCESS)                                               \
    {                                                                   \
    CM_DDI_ASSERTMESSAGE("%s [%d].", _str, hr);                         \
    goto finish;                                                        \
    }                                                                   \
}

#define CMCHK_NULL(ptr)                                                \
{                                                                      \
    if ((ptr) == NULL)                                                 \
    {                                                                  \
        CM_ASSERT(0);                                                  \
        CM_ASSERTMESSAGE("Invalid (NULL) Pointer.");                   \
        hr = CM_NULL_POINTER;                                          \
        goto finish;                                                   \
    }                                                                  \
}

#define CMCHK_NULL_RETURN(ptr, returnValue)                            \
{                                                                      \
    if ((ptr) == NULL)                                                 \
    {                                                                  \
        CM_ASSERT(0);                                                  \
        CM_ASSERTMESSAGE("Invalid (NULL) Pointer.");                   \
        hr = returnValue;                                              \
        goto finish;                                                   \
    }                                                                  \
}

#define CHK_HRESULT(stmt)                                              \
{                                                                      \
    hr = (stmt);                                                       \
    if (hr != S_OK)                                                    \
    {                                                                  \
        CM_ASSERTMESSAGE("hr check failed.");                          \
        goto finish;                                                   \
    }                                                                  \
}

#define CHK_NULL_RETURN_HRESULT(ptr)                                   \
{                                                                      \
    if ((ptr) == NULL)                                                 \
    {                                                                  \
        CM_ASSERTMESSAGE("Invalid (NULL) Pointer.");                   \
        hr = E_POINTER;                                                \
        goto finish;                                                   \
    }                                                                  \
}

#ifndef CHK_HR
#define CHK_HR(_stmt)       CHK_HRESULT(_stmt)
#endif

#ifndef CHK_NULL
#define CHK_NULL(_ptr)      CHK_NULL_RETURN_HRESULT(_ptr)
#endif

#endif
