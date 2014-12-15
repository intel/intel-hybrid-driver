/* * Copyright © 2014 Intel Corporation
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

#ifndef __OS_UTIL_DEBUG_H__
#define __OS_UTIL_DEBUG_H__

#include "os_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#if GENOS_MESSAGES_ENABLED

#define GENOS_MAX_SUBCOMPONENT_COUNT  16

#define GENOS_MAX_MSG_BUF_SIZE        1024

#define GENOS_MAX_HLT_FILENAME_LEN    260

	typedef enum {
		GENOS_MESSAGE_LVL_DISABLED = 0,
		GENOS_MESSAGE_LVL_CRITICAL = 1,
		GENOS_MESSAGE_LVL_NORMAL = 2,
		GENOS_MESSAGE_LVL_VERBOSE = 3,
		GENOS_MESSAGE_LVL_FUNCTION_ENTRY = 4,
		GENOS_MESSAGE_LVL_FUNCTION_EXIT = 5,
		GENOS_MESSAGE_LVL_FUNCTION_ENTRY_VERBOSE = 6,
		GENOS_MESSAGE_LVL_FUNCTION_EXIT_VERBOSE = 7,
		GENOS_MESSAGE_LVL_COUNT
	} GENOS_MESSAGE_LEVEL;

	typedef enum {
		GENOS_COMPONENT_OS,
		GENOS_COMPONENT_HW,
		GENOS_COMPONENT_DDI,
		GENOS_COMPONENT_CM,
		GENOS_COMPONENT_COUNT
	} GENOS_COMPONENT_ID;

	extern const PCCHAR GENOS_ComponentName[GENOS_COMPONENT_COUNT];

	extern const PCCHAR GENOS_LogLevelName[GENOS_MESSAGE_LVL_COUNT];

	typedef enum {
		GENOS_HW_SUBCOMP_DDI = 0,
		GENOS_HW_SUBCOMP_HW = 1,
		GENOS_HW_SUBCOMP_PUBLIC = 2,
		GENOS_HW_SUBCOMP_COUNT
	} GENOS_VP_SUBCOMP_ID;

	typedef enum {
		GENOS_CM_SUBCOMP_DDI = 0,
		GENOS_CM_SUBCOMP_SELF = 1,
		GENOS_CM_SUBCOMP_COUNT
	} GENOS_CM_SUBCOMP_ID;

	typedef struct _GENOS_DEBUG_PARAMS {
		BOOL bAssertEnabled;
		GENOS_MESSAGE_LEVEL uiMessageLevel;
	} GENOS_DEBUG_PARAMS;

	typedef struct _GENOS_MESSAGE_PARAMS {
		PFILE pLogFile;
		PFILE pTraceFile;
		UINT32 uiCounter;
		BOOL bUseHybridLogTrace;
		BOOL bUseOutputDebugString;
		UINT32 bEnableMaps;
		GENOS_COMPONENT_DEBUG_PARAMS components[GENOS_COMPONENT_COUNT];
		CHAR g_GenOsMsgBuffer[GENOS_MAX_MSG_BUF_SIZE];
	} GENOS_MESSAGE_PARAMS;

	VOID GENOS_MessageInit();

	VOID GENOS_MessageClose();

	VOID GENOS_HltpPreface(PFILE pFile);

	VOID GENOS_Message(GENOS_MESSAGE_LEVEL level,
			   const PCCHAR logtag,
			   GENOS_COMPONENT_ID compID,
			   UINT8 subCompID, const PCCHAR message, ...);

	VOID GENOS_Message_DDI_Dump(const PCCHAR message, ...);

	GENOS_STATUS GENOS_LogFileNamePrefix(PCHAR fileNamePrefix);

#ifndef LOG_TAG
#define LOG_TAG "IntelGenOs"
#endif

#if USE_PRETTY_FUNCTION

	PCCHAR GENOS_getClassMethod(PCCHAR pcPrettyFunction);

#define GENOS_FUNCTION GENOS_getClassMethod(__PRETTY_FUNCTION__)

#else
#define GENOS_FUNCTION __FUNCTION__
#endif

#define GENOS_DEBUGMESSAGE(_level, _compID, _subCompID, _message, ...)                        \
    GENOS_Message(_level, LOG_TAG, _compID, _subCompID, "%s%s - %s:%d: " _message "\n",       \
        GENOS_ComponentName[_compID], GENOS_LogLevelName[_level], GENOS_FUNCTION, __LINE__, ##__VA_ARGS__)

#define GENOS_DEBUGMESSAGE_NOLINE(_level, _compID, _subCompID, _message, ...)                 \
    GENOS_Message(_level, LOG_TAG, _compID, _subCompID, "%s%s - %s" _message "\n",            \
        GENOS_ComponentName[_compID], GENOS_LogLevelName[_level], GENOS_FUNCTION, ##__VA_ARGS__)

#define GENOS_FUNCTION_ENTER(_compID, _subCompID)                                             \
    GENOS_DEBUGMESSAGE_NOLINE(GENOS_MESSAGE_LVL_FUNCTION_ENTRY, _compID, _subCompID, "")

#define GENOS_FUNCTION_EXIT(_compID, _subCompID, hr)                                           \
    GENOS_DEBUGMESSAGE_NOLINE(GENOS_MESSAGE_LVL_FUNCTION_EXIT, _compID, _subCompID, ": hr = 0x%x", hr)

#define GENOS_FUNCTION_ENTER_VERBOSE(_compID, _subCompID)                                     \
    GENOS_DEBUGMESSAGE(GENOS_MESSAGE_LVL_FUNCTION_ENTRY_VERBOSE, _compID, _subCompID, "")

#define GENOS_FUNCTION_EXIT_VERBOSE(_compID, _subCompID)                                      \
    GENOS_DEBUGMESSAGE(GENOS_MESSAGE_LVL_FUNCTION_EXIT_VERBOSE, _compID, _subCompID, "")

#define GENOS_ASSERTMESSAGE(_compID, _subCompID, _message, ...)                               \
    GENOS_CRITICALMESSAGE(_compID, _subCompID, _message, ##__VA_ARGS__);                      \
    GENOS_ASSERT(_compID, _subCompID, FALSE);

#define GENOS_NORMALMESSAGE(_compID, _subCompID, _message, ...)                               \
    GENOS_DEBUGMESSAGE(GENOS_MESSAGE_LVL_NORMAL, _compID, _subCompID, _message, ##__VA_ARGS__)

#define GENOS_VERBOSEMESSAGE(_compID, _subCompID, _message, ...)                              \
    GENOS_DEBUGMESSAGE(GENOS_MESSAGE_LVL_VERBOSE, _compID, _subCompID, _message, ##__VA_ARGS__)

#define GENOS_CRITICALMESSAGE(_compID, _subCompID, _message, ...)                             \
    GENOS_DEBUGMESSAGE(GENOS_MESSAGE_LVL_CRITICAL, _compID, _subCompID, _message, ##__VA_ARGS__)

#define GENOS_DEBUGMESSAGE_IF(_cond, _level, _compID, _subCompID, _message, ...)              \
    if (_cond)                                                                              \
    {                                                                                       \
        GENOS_DEBUGMESSAGE(_level, _compID, _subCompID, _message, ##__VA_ARGS__);             \
    }

#else

#define GENOS_MessageInit()
#define GENOS_MessageClose()

#define GENOS_FUNCTION_ENTER(_compID, _subCompID)
#define GENOS_FUNCTION_EXIT(_compID, _subCompID, hr)
#define GENOS_FUNCTION_ENTER_VERBOSE(_compID, _subCompID)
#define GENOS_FUNCTION_EXIT_VERBOSE(_compID, _subCompID)
#define GENOS_ASSERTMESSAGE(_compID, _subCompID, _message, ...)
#define GENOS_NORMALMESSAGE(_compID, _subCompID, _message, ...)
#define GENOS_VERBOSEMESSAGE(_compID, _subCompID, _message, ...)
#define GENOS_DEBUGMESSAGE_IF(_cond, _level, _compID, _subCompID, _message, ...)
#define GENOS_DEBUGMESSAGE(_compID, _subCompID, _message, ...)

#endif

#if GENOS_ASSERT_ENABLED

#define GENOS_ASSERT(_compID, _subCompID, _expr)                   \
    if(!(_expr))                                                 \
    {                                                            \
        _GENOS_Assert(_compID, _subCompID);                        \
    }

	VOID _GENOS_Assert(GENOS_COMPONENT_ID compID, UINT8 subCompID);

#else

#define GENOS_ASSERT(_compID, _subCompID, _expr)

#endif

#define GENOS_CHK_STATUS(_compID, _subCompID, _stmt)                                          \
{                                                                                           \
    eStatus = (GENOS_STATUS)(_stmt);                                                          \
    if (eStatus != GENOS_STATUS_SUCCESS)                                                      \
    {                                                                                       \
        GENOS_ASSERTMESSAGE(_compID, _subCompID, "IntelGenOs returned error.");                      \
        goto finish;                                                                        \
    }                                                                                       \
}

#define GENOS_CHK_STATUS_MESSAGE(_compID, _subCompID, _stmt, _message, ...)                   \
{                                                                                           \
    eStatus = (GENOS_STATUS)(_stmt);                                                          \
    if (eStatus != GENOS_STATUS_SUCCESS)                                                      \
    {                                                                                       \
        GENOS_ASSERTMESSAGE(_compID, _subCompID, _message, ##__VA_ARGS__);                    \
        goto finish;                                                                        \
    }                                                                                       \
}

#define GENOS_CHK_STATUS_SAFE(_stmt)                                                          \
{                                                                                           \
    eStatus = (GENOS_STATUS)(_stmt);                                                          \
    if (eStatus != GENOS_STATUS_SUCCESS)                                                      \
    {                                                                                       \
        goto finish;                                                                        \
    }                                                                                       \
}

#define GENOS_CHK_NULL(_compID, _subCompID, _ptr)                                             \
{                                                                                           \
    if ((_ptr) == NULL)                                                                     \
    {                                                                                       \
        GENOS_ASSERTMESSAGE(_compID, _subCompID, "Invalid (NULL) Pointer.");                  \
        eStatus = GENOS_STATUS_NULL_POINTER;                                                  \
        goto finish;                                                                        \
    }                                                                                       \
}

#define GENOS_CHK_NULL_NO_STATUS(_compID, _subCompID, _ptr)                                   \
{                                                                                           \
    if ((_ptr) == NULL)                                                                     \
    {                                                                                       \
        GENOS_ASSERTMESSAGE(_compID, _subCompID, "Invalid (NULL) Pointer.");                  \
        goto finish;                                                                        \
    }                                                                                       \
}

#define GENOS_CHK_HR(_compID, _subCompID, _stmt)                                              \
{                                                                                           \
    hr = (_stmt);                                                                           \
    if (hr != S_OK)                                                                         \
    {                                                                                       \
        GENOS_ASSERTMESSAGE(_compID, _subCompID, "hr check failed.");                         \
        goto finish;                                                                        \
    }                                                                                       \
}

#define GENOS_CHK_HR_MESSAGE(_compID, _subCompID, _stmt, _message, ...)                       \
{                                                                                           \
    hr = (_stmt);                                                                           \
    if (hr != S_OK)                                                                         \
    {                                                                                       \
        GENOS_ASSERTMESSAGE(_compID, _subCompID, _message, ##__VA_ARGS__);                    \
        goto finish;                                                                        \
    }                                                                                       \
}

#define GENOS_CHK_NULL_WITH_HR(_compID, _subCompID, _ptr)                                     \
{                                                                                           \
    if ((_ptr) == NULL)                                                                     \
    {                                                                                       \
        GENOS_ASSERTMESSAGE(_compID, _subCompID, "Invalid (NULL) Pointer.");                  \
        hr = E_FAIL;                                                                        \
        goto finish;                                                                        \
    }                                                                                       \
}

#define GENOS_CHECK_CONDITION(_compID, _subCompID, _condition, _str, _ret)                    \
{                                                                                           \
    if (_condition)                                                                         \
    {                                                                                       \
        GENOS_ASSERTMESSAGE(_compID, _subCompID, _str);                                       \
        return _ret;                                                                        \
    }                                                                                       \
}

#define GENOS_OS_CHK_STATUS(_stmt)                                                            \
    GENOS_CHK_STATUS(GENOS_COMPONENT_OS, GENOS_SUBCOMP_SELF, _stmt)

#define GENOS_OS_CHK_NULL(_ptr)                                                               \
    GENOS_CHK_NULL(GENOS_COMPONENT_OS, GENOS_SUBCOMP_SELF, _ptr)

#define GENOS_OS_CHK_HR(_ptr)                                                                 \
    GENOS_CHK_HR(GENOS_COMPONENT_OS, GENOS_SUBCOMP_SELF, _ptr)

#define GENOS_OS_CHK_HR_MESSAGE(_ptr, _message, ...)                                          \
    GENOS_CHK_HR_MESSAGE(GENOS_COMPONENT_OS, GENOS_SUBCOMP_SELF, _ptr, _message, ##__VA_ARGS__)

#define GENOS_OS_CHK_NULL_WITH_HR(_ptr)                                                       \
    GENOS_CHK_NULL_WITH_HR(GENOS_COMPONENT_OS, GENOS_SUBCOMP_SELF, _ptr)

#define GENOS_OS_CHECK_CONDITION(_condition, _str, _ret)                                      \
   GENOS_CHECK_CONDITION(GENOS_COMPONENT_OS, GENOS_SUBCOMP_SELF, _condition, _str, _ret)

#define GENOS_OS_ASSERT(_expr)                                                                \
    GENOS_ASSERT(GENOS_COMPONENT_OS, GENOS_SUBCOMP_SELF, _expr)

#define GENOS_OS_ASSERTMESSAGE(_message, ...)                                                 \
    GENOS_ASSERTMESSAGE(GENOS_COMPONENT_OS, GENOS_SUBCOMP_SELF, _message, ##__VA_ARGS__)

#define GENOS_OS_NORMALMESSAGE(_message, ...)                                                 \
    GENOS_NORMALMESSAGE(GENOS_COMPONENT_OS, GENOS_SUBCOMP_SELF, _message, ##__VA_ARGS__)

#define GENOS_OS_VERBOSEMESSAGE(_message, ...)                                                \
    GENOS_VERBOSEMESSAGE(GENOS_COMPONENT_OS, GENOS_SUBCOMP_SELF, _message, ##__VA_ARGS__)

#define GENOS_OS_FUNCTION_ENTER                                                               \
    GENOS_FUNCTION_ENTER(GENOS_COMPONENT_OS, GENOS_SUBCOMP_SELF)

#ifdef __cplusplus
}
#endif
#endif
