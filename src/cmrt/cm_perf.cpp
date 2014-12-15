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

#if (PERFORMANCE_MEASUREMENT)
#include "cm_def.h"
#include "cm_perf.h"
#include "cm_perf_internal.h"

INT64 g_CPUFrequency = 0.0;

static double g_time_ws_LockRect = 0.0;
static double g_time_ws_MemCopy = 0.0;
static double g_time_ws_UnlockRect = 0.0;
static double g_time_ws_UpdateSurface = 0.0;
static double g_time_ws_FlushDeviceQueue = 0.0;

static double g_time_rs_LockRect = 0.0;
static double g_time_rs_MemCopy = 0.0;
static double g_time_rs_UnlockRect = 0.0;
static double g_time_rs_GetRenderTargetData = 0.0;
static double g_time_rs_FlushDeviceQueue = 0.0;

static double g_time_eq_Enqueue_RT = 0.0;
static double g_time_eqrt_FlushWithoutSync = 0.0;

static double g_time_flush_QueryFlushedTasks = 0.0;
static double g_time_flush_Execute = 0.0;
static double g_time_flush_Execute_sum = 0.0;
static double g_time_flush_Execute_avg = 0.0;
static int g_Execute_times = 0;

static double g_time_jit_Compile = 0.0;

static char *g_func_name_str[] = {
	"WS_FlushDeviceQueue",	//0
	"WS_LockRect",		//1
	"WS_MemCopy",		//2
	"WS_UnlockRect",	//3
	"WS_UpdateSurface",	//4

	"RS_FlushDeviceQueue",	//5
	"RS_GetRenderTargetData",	//6
	"RS_LockRect",		//7
	"RS_MemCopy",		//8
	"RS_UnlockRect",	//9

	"EQ_Enqueue_RT",	//10
	"EQRT_FlushWithoutSync",	//11
	"FLUSH_QueryFlushedTasks",	//12
	"FLUSH_Execute",	//13
	"JIT_Compile",		//14
	"ERROR"
};

extern "C" inline INT
UpdateFuncTime(FUNC_TIME_TYPE FunctionName, const double value)
{
	INT ret = CM_SUCCESS;
	switch (FunctionName) {
	case FUNC_TIME_WS_LOCKRECT:
		g_time_ws_LockRect = value;
		break;

	case FUNC_TIME_WS_MEMCOPY:
		g_time_ws_MemCopy = value;
		break;

	case FUNC_TIME_WS_UNLOCKRECT:
		g_time_ws_UnlockRect = value;
		break;

	case FUNC_TIME_WS_UPDATESURFACE:
		g_time_ws_UpdateSurface = value;
		break;

	case FUNC_TIME_WS_FLUSHDEVICEQUEUE:
		g_time_ws_FlushDeviceQueue = value;
		break;

	case FUNC_TIME_EQ_ENQUEUERT:
		g_time_eq_Enqueue_RT = value;
		break;

	case FUNC_TIME_RS_GETRENDERTARGETDATA:
		g_time_rs_GetRenderTargetData = value;
		break;

	case FUNC_TIME_RS_LOCKRECT:
		g_time_rs_LockRect = value;
		break;

	case FUNC_TIME_RS_MEMCOPY:
		g_time_rs_MemCopy = value;
		break;

	case FUNC_TIME_RS_UNLOCKRECT:
		g_time_rs_UnlockRect = value;
		break;

	case FUNC_TIME_RS_FLUSHDEVICEQUEUE:
		g_time_rs_FlushDeviceQueue = value;
		break;

	case FUNC_TIME_EQRT_FLUSHWITHOUTSYNC:
		g_time_eqrt_FlushWithoutSync = value;
		break;

	case FUNC_TIME_FLUSH_QUERYFLUSHEDTASKS:
		g_time_flush_QueryFlushedTasks = value;
		break;

	case FUNC_TIME_FLUSH_EXECUTE:
		g_time_flush_Execute = value;
		g_time_flush_Execute_sum += value;
		g_Execute_times++;
		g_time_flush_Execute_avg =
		    g_time_flush_Execute_sum / g_Execute_times;
		break;

	case FUNC_TIME_JIT_COMPILE:
		g_time_jit_Compile = value;
		break;

	default:
		ret = CM_FAILURE;
		break;
	}
	return ret;
}

extern "C" CM_RT_API INT
QueryFuncTime(FUNC_TIME_TYPE FunctionNameIndex, double *pValue)
{
	int ret = CM_SUCCESS;
	switch (FunctionNameIndex) {
	case FUNC_TIME_WS_LOCKRECT:
		*pValue = g_time_ws_LockRect;
		break;

	case FUNC_TIME_WS_MEMCOPY:
		*pValue = g_time_ws_MemCopy;
		break;

	case FUNC_TIME_WS_UNLOCKRECT:
		*pValue = g_time_ws_UnlockRect;
		break;

	case FUNC_TIME_WS_UPDATESURFACE:
		*pValue = g_time_ws_UpdateSurface;
		break;

	case FUNC_TIME_WS_FLUSHDEVICEQUEUE:
		*pValue = g_time_ws_FlushDeviceQueue;
		break;

	case FUNC_TIME_EQ_ENQUEUERT:
		*pValue = g_time_eq_Enqueue_RT;
		break;

	case FUNC_TIME_RS_GETRENDERTARGETDATA:
		*pValue = g_time_rs_GetRenderTargetData;
		break;

	case FUNC_TIME_RS_LOCKRECT:
		*pValue = g_time_rs_LockRect;
		break;

	case FUNC_TIME_RS_MEMCOPY:
		*pValue = g_time_rs_MemCopy;
		break;

	case FUNC_TIME_RS_UNLOCKRECT:
		*pValue = g_time_rs_UnlockRect;
		break;

	case FUNC_TIME_RS_FLUSHDEVICEQUEUE:
		*pValue = g_time_rs_FlushDeviceQueue;
		break;

	case FUNC_TIME_EQRT_FLUSHWITHOUTSYNC:
		*pValue = g_time_eqrt_FlushWithoutSync;
		break;

	case FUNC_TIME_FLUSH_QUERYFLUSHEDTASKS:
		*pValue = g_time_flush_QueryFlushedTasks;
		break;

	case FUNC_TIME_FLUSH_EXECUTE:
		*pValue = g_time_flush_Execute;
		break;

	case FUNC_TIME_JIT_COMPILE:
		*pValue = g_time_jit_Compile;
		break;

	default:
		*pValue = 0.0;
		ret = CM_FAILURE;
		break;
	}
	return ret;
}

extern "C" CM_RT_API INT PrintFuncTime(FUNC_TIME_TYPE FunctionNameIndex)
{
	double value;
	int ret = CM_SUCCESS;

	switch (FunctionNameIndex) {
	case FUNC_TIME_WS_LOCKRECT:
		value = g_time_ws_LockRect;
		break;

	case FUNC_TIME_WS_MEMCOPY:
		value = g_time_ws_MemCopy;
		break;

	case FUNC_TIME_WS_UNLOCKRECT:
		value = g_time_ws_UnlockRect;
		break;

	case FUNC_TIME_WS_UPDATESURFACE:
		value = g_time_ws_UpdateSurface;
		break;

	case FUNC_TIME_WS_FLUSHDEVICEQUEUE:
		value = g_time_ws_FlushDeviceQueue;
		break;

	case FUNC_TIME_EQ_ENQUEUERT:
		value = g_time_eq_Enqueue_RT;
		break;

	case FUNC_TIME_RS_GETRENDERTARGETDATA:
		value = g_time_rs_GetRenderTargetData;
		break;

	case FUNC_TIME_RS_LOCKRECT:
		value = g_time_rs_LockRect;
		break;

	case FUNC_TIME_RS_MEMCOPY:
		value = g_time_rs_MemCopy;
		break;

	case FUNC_TIME_RS_UNLOCKRECT:
		value = g_time_rs_UnlockRect;
		break;

	case FUNC_TIME_RS_FLUSHDEVICEQUEUE:
		value = g_time_rs_FlushDeviceQueue;
		break;

	case FUNC_TIME_EQRT_FLUSHWITHOUTSYNC:
		value = g_time_eqrt_FlushWithoutSync;
		break;

	case FUNC_TIME_FLUSH_QUERYFLUSHEDTASKS:
		value = g_time_flush_QueryFlushedTasks;
		break;

	case FUNC_TIME_FLUSH_EXECUTE:
		value = g_time_flush_Execute;
		break;

	case FUNC_TIME_JIT_COMPILE:
		value = g_time_jit_Compile;
		break;

	default:
		value = 0.0;
		ret = CM_FAILURE;
		break;
	}
	CM_NORMALMESSAGE("\tCallee %s took %4.2f ms.",
			 g_func_name_str[FunctionNameIndex], value);
	if (FunctionNameIndex == FUNC_TIME_FLUSH_EXECUTE) {
		CM_NORMALMESSAGE
		    ("\t\tExecute called times is %d, and average time is %4.2f ms.",
		     g_Execute_times, g_time_flush_Execute_avg);
	}
	return ret;
}

extern "C" CM_RT_API double GetFuncTime(FUNC_TIME_TYPE FunctionNameIndex)
{
	double value;
	int ret = CM_SUCCESS;

	switch (FunctionNameIndex) {
	case FUNC_TIME_WS_LOCKRECT:
		value = g_time_ws_LockRect;
		break;

	case FUNC_TIME_WS_MEMCOPY:
		value = g_time_ws_MemCopy;
		break;

	case FUNC_TIME_WS_UNLOCKRECT:
		value = g_time_ws_UnlockRect;
		break;

	case FUNC_TIME_WS_UPDATESURFACE:
		value = g_time_ws_UpdateSurface;
		break;

	case FUNC_TIME_WS_FLUSHDEVICEQUEUE:
		value = g_time_ws_FlushDeviceQueue;
		break;

	case FUNC_TIME_EQ_ENQUEUERT:
		value = g_time_eq_Enqueue_RT;
		break;

	case FUNC_TIME_RS_GETRENDERTARGETDATA:
		value = g_time_rs_GetRenderTargetData;
		break;

	case FUNC_TIME_RS_LOCKRECT:
		value = g_time_rs_LockRect;
		break;

	case FUNC_TIME_RS_MEMCOPY:
		value = g_time_rs_MemCopy;
		break;

	case FUNC_TIME_RS_UNLOCKRECT:
		value = g_time_rs_UnlockRect;
		break;

	case FUNC_TIME_RS_FLUSHDEVICEQUEUE:
		value = g_time_rs_FlushDeviceQueue;
		break;

	case FUNC_TIME_EQRT_FLUSHWITHOUTSYNC:
		value = g_time_eqrt_FlushWithoutSync;
		break;

	case FUNC_TIME_FLUSH_QUERYFLUSHEDTASKS:
		value = g_time_flush_QueryFlushedTasks;
		break;

	case FUNC_TIME_FLUSH_EXECUTE:
		value = g_time_flush_Execute;
		break;

	case FUNC_TIME_JIT_COMPILE:
		value = g_time_jit_Compile;
		break;

	default:
		value = 0.0;
		ret = CM_FAILURE;
		break;
	}

	return value;
}

extern "C" CM_RT_API INT64 QueryCPUPerformanceFrequency()
{
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	return Freq.QuadPart;
}

extern "C" CM_RT_API INT UpdateCMRTCPUFrequencyVar()
{
	g_CPUFrequency = QueryCPUPerformanceFrequency();
	return CM_SUCCESS;
}
#endif
