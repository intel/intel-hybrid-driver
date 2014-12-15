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

#define PERFORMANCE_MEASUREMENT     0

typedef enum _FUNC_TIME_TYPE {
	FUNC_TIME_WS_FLUSHDEVICEQUEUE,
	FUNC_TIME_WS_LOCKRECT,
	FUNC_TIME_WS_MEMCOPY,
	FUNC_TIME_WS_UNLOCKRECT,
	FUNC_TIME_WS_UPDATESURFACE,

	FUNC_TIME_RS_FLUSHDEVICEQUEUE,
	FUNC_TIME_RS_GETRENDERTARGETDATA,
	FUNC_TIME_RS_LOCKRECT,
	FUNC_TIME_RS_MEMCOPY,
	FUNC_TIME_RS_UNLOCKRECT,

	FUNC_TIME_EQ_ENQUEUERT,
	FUNC_TIME_EQRT_FLUSHWITHOUTSYNC,

	FUNC_TIME_FLUSH_QUERYFLUSHEDTASKS,
	FUNC_TIME_FLUSH_EXECUTE,

	FUNC_TIME_JIT_COMPILE,

	FUNC_TIME_ERROR
} FUNC_TIME_TYPE;

#define CHK_RESULT_TIMING(_function_, _name_, _CPUFrequency_) \
    {                                                         \
        INT64 _start = __rdtsc();                             \
        int _result_ = _function_;                            \
        INT64 _time = __rdtsc() - _start;                     \
        if(_result_ != CM_SUCCESS)                            \
        {                                                     \
            CM_ASSERTMESSAGE("Error %d!", _result_);          \
            return _result_;                                  \
        }                                                     \
        else                                                  \
        {                                                     \
            CM_NORMALMESSAGE("\Callee %s took %4.2f ms.", _name_, (double)(_time * 1000.00)/(double)_CPUFrequency_);   \
        }                                                     \
    }

#define CHK_RESULT(_function_, _name_)                        \
    {                                                         \
        int _result_ = _function_;                            \
        if(_result_ != CM_SUCCESS)                            \
        {                                                     \
            CM_ASSERTMESSAGE("Error %d!", _result_);          \
            return _result_;                                  \
        }                                                     \
    }

#define CHK_TIMING(_function_, _name_, _CPUFrequency_)        \
    {                                                         \
    INT64 _start = __rdtsc();                                 \
    _function_;                                               \
    INT64 _time = __rdtsc() - _start;                         \
    CM_NORMALMESSAGE("Function %s took %4.2f ms.", _name_, (double)(_time * 1000.00)/(double)_CPUFrequency_);   \
    }

#define CHK_TIMING_RETURN(_function_, _name_, _CPUFrequency_, _result_) \
    {                                                         \
    INT64 _start = __rdtsc();                                 \
    _result_ = _function_;                                    \
    INT64 _time = __rdtsc() - _start;                         \
    CM_NORMALMESSAGE("Function %s took %4.2f ms.", _name_, (double)(_time * 1000.00)/(double)_CPUFrequency_);   \
    }

#define CHK_RESULT_RETURN_TIMING(_function_, _name_, _CPUFrequency_, _time_) \
    {                                                         \
    INT64 _start = __rdtsc();                                 \
    int _result_ = _function_;                                \
    INT64 _counter = __rdtsc() - _start;                      \
    if(_result_ != CM_SUCCESS)                                \
        {                                                     \
        CM_NORMALMESSAGE("Error %d!", _result_);              \
        return _result_;                                      \
        }                                                     \
        else                                                  \
        {                                                     \
        _time_ = (double)(_counter * 1000.00)/(double)_CPUFrequency_; \
        }                                                     \
    }

#define RETURN_TIMING(_function_, _name_, _CPUFrequency_, _time_) \
    {                                                         \
    INT64 _start = __rdtsc();                                 \
    _function_;                                               \
    INT64 _counter = __rdtsc() - _start;                      \
    _time_ = (double)(_counter * 1000.00)/(double)_CPUFrequency_;   \
    }

extern "C" CM_RT_API INT64 QueryCPUPerformanceFrequency();
extern "C" CM_RT_API INT UpdateCMRTCPUFrequencyVar();
extern "C" CM_RT_API INT QueryFuncTime(FUNC_TIME_TYPE FunctionNameIndex,
				       double *pValue);
extern "C" CM_RT_API INT PrintFuncTime(FUNC_TIME_TYPE FunctionNameIndex);
extern "C" CM_RT_API double GetFuncTime(FUNC_TIME_TYPE FunctionNameIndex);
