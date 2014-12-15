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

extern INT64 g_CPUFrequency;
extern "C" inline INT UpdateFuncTime(FUNC_TIME_TYPE FunctionName,
				     const double value);

#define RECORD_TIMING_CHK_RESULT(_function_, _func_index_, _CPUFrequency_) \
{                                               \
    INT64 _start = __rdtsc();                   \
    int _result_ = _function_;                  \
    INT64 _time = __rdtsc() - _start;           \
    if(_result_ != CM_SUCCESS)                  \
    {                                           \
        CM_ASSERTMESSAGE("Error %d!", _result_);\
        CM_ASSERT( 0 );                         \
        return _result_;                        \
    }                                           \
    else                                        \
    {                                           \
        UpdateFuncTime(_func_index_, (double)(_time * 1000.00)/(double)_CPUFrequency_); \
    }                                           \
}

#define RECORD_TIMING(_function_, _name_, _CPUFrequency_) \
{                                               \
    INT64 _start = __rdtsc();                   \
    _function_;                                 \
    INT64 _time = __rdtsc() - _start;           \
    UpdateFuncTime(_name_, (double)(_time * 1000.00)/(double)_CPUFrequency_); \
}

#define RECORD_TIMING_RET_RESULT(_function_, _name_, _CPUFrequency_, _result_) \
{                                               \
    INT64 _start = __rdtsc();                   \
    _result_ = _function_;                      \
    INT64 _time = __rdtsc() - _start;           \
    UpdateFuncTime(_name_, (double)(_time * 1000.00)/(double)_CPUFrequency_); \
}
