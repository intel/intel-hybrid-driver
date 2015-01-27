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
 *     Zhao Yakui <yakui.zhao@intel.com>
 */

#pragma once
#ifndef _CM_RT_LINUX_H_
#define _CM_RT_LINUX_H_
#include <time.h>
#include <va/va.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <i915_drm.h>
#include <intel_bufmgr.h>

#ifdef __cplusplus
}
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <x86intrin.h>		//__rdtsc
typedef bool BOOL;
typedef int32_t INT;
typedef uint32_t UINT;
typedef signed char INT8;
typedef unsigned char UINT8;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int64_t INT64;
typedef uint64_t UINT64;
typedef unsigned char BYTE;
typedef UINT32 DWORD;

typedef enum _VACMTEXTUREADDRESS {
	VACMTADDRESS_WRAP = 1,
	VACMTADDRESS_MIRROR = 2,
	VACMTADDRESS_CLAMP = 3,
	VACMTADDRESS_BORDER = 4,
	VACMTADDRESS_MIRRORONCE = 5,

	VACMTADDRESS_FORCE_DWORD = 0x7fffffff
} VACMTEXTUREADDRESS;

typedef enum _VACMTEXTUREFILTERTYPE {
	VACMTEXF_NONE = 0,
	VACMTEXF_POINT = 1,
	VACMTEXF_LINEAR = 2,
	VACMTEXF_ANISOTROPIC = 3,
	VACMTEXF_FLATCUBIC = 4,
	VACMTEXF_GAUSSIANCUBIC = 5,
	VACMTEXF_PYRAMIDALQUAD = 6,
	VACMTEXF_GAUSSIANQUAD = 7,
	VACMTEXF_CONVOLUTIONMONO = 8,
	VACMTEXF_FORCE_DWORD = 0x7fffffff
} VACMTEXTUREFILTERTYPE;

#define CM_MAX_TIMEOUT 2

#define CM_ATTRIBUTE(attribute) __attribute__((attribute))

typedef enum _VA_CM_FORMAT {

	VA_CM_FMT_UNKNOWN = 0,

	VA_CM_FMT_BUFFER = 10,
	VA_CM_FMT_A8R8G8B8 = 21,
	VA_CM_FMT_X8R8G8B8 = 22,
	VA_CM_FMT_A8 = 28,
	VA_CM_FMT_A2B10G10R10 = 31,
	VA_CM_FMT_A16B16G16R16 = 36,
	VA_CM_FMT_P8 = 41,
	VA_CM_FMT_L8 = 50,
	VA_CM_FMT_R16U = 57,
	VA_CM_FMT_A8L8 = 51,
	VA_CM_FMT_V8U8 = 60,
	VA_CM_FMT_R8U = 62,
	VA_CM_FMT_D16 = 80,
	VA_CM_FMT_L16 = 81,
	VA_CM_FMT_A16B16G16R16F = 113,
	VA_CM_FMT_R32F = 114,
	VA_CM_FMT_NV12 = VA_FOURCC_NV12,
	VA_CM_FMT_UYVY = VA_FOURCC_UYVY,
	VA_CM_FMT_YUY2 = VA_FOURCC_YUY2,
	VA_CM_FMT_444P = VA_FOURCC_444P,
	VA_CM_FMT_411P = VA_FOURCC_411P,
	VA_CM_FMT_422H = VA_FOURCC_422H,
	VA_CM_FMT_422V = VA_FOURCC_422V,
	VA_CM_FMT_IMC3 = VA_FOURCC_IMC3,
	VA_CM_FMT_YV12 = VA_FOURCC_YV12,

	VA_CM_FMT_MAX = 0xFFFFFFFF
} VA_CM_FORMAT;

#define CM_SURFACE_FORMAT                       VA_CM_FORMAT

#define CM_SURFACE_FORMAT_UNKNOWN               VA_CM_FMT_UNKNOWN
#define CM_SURFACE_FORMAT_A8R8G8B8              VA_CM_FMT_A8R8G8B8
#define CM_SURFACE_FORMAT_X8R8G8B8              VA_CM_FMT_X8R8G8B8
#define CM_SURFACE_FORMAT_A8                    VA_CM_FMT_A8
#define CM_SURFACE_FORMAT_P8                    VA_CM_FMT_P8
#define CM_SURFACE_FORMAT_R32F                  VA_CM_FMT_R32F
#define CM_SURFACE_FORMAT_NV12                  VA_CM_FMT_NV12
#define CM_SURFACE_FORMAT_UYVY                  VA_CM_FMT_UYVY
#define CM_SURFACE_FORMAT_YUY2                  VA_CM_FMT_YUY2
#define CM_SURFACE_FORMAT_V8U8			VA_CM_FMT_V8U8

#define CM_SURFACE_FORMAT_R8_UINT		VA_CM_FMT_R8U
#define CM_SURFACE_FORMAT_R16_UINT		VA_CM_FMT_R16U
#define CM_SURFACE_FORMAT_R16_SINT              VA_CM_FMT_A8L8
#define CM_SURFACE_FORMAT_D16			VA_CM_FMT_D16
#define CM_SURFACE_FORMAT_L16			VA_CM_FMT_L16
#define CM_SURFACE_FORMAT_A16B16G16R16          VA_CM_FMT_A16B16G16R16
#define CM_SURFACE_FORMAT_R10G10B10A2		VA_CM_FMT_A2B10G10R10
#define CM_SURFACE_FORMAT_A16B16G16R16F         VA_CM_FMT_A16B16G16R16F

#define CM_SURFACE_FORMAT_444P                  VA_CM_FMT_444P
#define CM_SURFACE_FORMAT_422H                  VA_CM_FMT_422H
#define CM_SURFACE_FORMAT_422V                  VA_CM_FMT_422V
#define CM_SURFACE_FORMAT_411P                  VA_CM_FMT_411P
#define CM_SURFACE_FORMAT_IMC3                  VA_CM_FMT_IMC3
#define CM_SURFACE_FORMAT_YV12                  VA_CM_FMT_YV12

#define CM_TEXTURE_ADDRESS_TYPE                 VACMTEXTUREADDRESS
#define CM_TEXTURE_ADDRESS_WRAP                 VACMTADDRESS_WRAP
#define CM_TEXTURE_ADDRESS_MIRROR               VACMTADDRESS_MIRROR
#define CM_TEXTURE_ADDRESS_CLAMP                VACMTADDRESS_CLAMP
#define CM_TEXTURE_ADDRESS_BORDER               VACMTADDRESS_BORDER
#define CM_TEXTURE_ADDRESS_MIRRORONCE           VACMTADDRESS_MIRRORONCE

#define CM_TEXTURE_FILTER_TYPE                  VACMTEXTUREFILTERTYPE
#define CM_TEXTURE_FILTER_TYPE_NONE             VACMTEXF_NONE
#define CM_TEXTURE_FILTER_TYPE_POINT            VACMTEXF_POINT
#define CM_TEXTURE_FILTER_TYPE_LINEAR           VACMTEXF_LINEAR
#define CM_TEXTURE_FILTER_TYPE_ANISOTROPIC      VACMTEXF_ANISOTROPIC
#define CM_TEXTURE_FILTER_TYPE_FLATCUBIC        VACMTEXF_FLATCUBIC
#define CM_TEXTURE_FILTER_TYPE_GAUSSIANCUBIC    VACMTEXF_GAUSSIANCUBIC
#define CM_TEXTURE_FILTER_TYPE_PYRAMIDALQUAD    VACMTEXF_PYRAMIDALQUAD
#define CM_TEXTURE_FILTER_TYPE_GAUSSIANQUAD     VACMTEXF_GAUSSIANQUAD
#define CM_TEXTURE_FILTER_TYPE_CONVOLUTIONMONO  VACMTEXF_CONVOLUTIONMONO

/* Surrport for common-used data type */
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef int HKEY;

typedef unsigned int uint;
typedef unsigned int *PUINT;

typedef float FLOAT;
typedef unsigned long long DWORDLONG;

/* Handle Type */
typedef void *HMODULE;
typedef void *HINSTANCE;
typedef int HANDLE;
typedef void *PVOID;
typedef int WINBOOL;
typedef BOOL *PBOOL;
typedef unsigned long ULONG;
typedef ULONG *PULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
typedef double DOUBLE;

#define __int8 char
#define __int16 short
#define __int32 int
#define __int64 long long

typedef unsigned short WORD;
typedef float FLOAT;
typedef FLOAT *PFLOAT;
typedef BYTE *PBYTE;
typedef int *PINT;
typedef WORD *PWORD;
typedef DWORD *PDWORD;
typedef unsigned int *PUINT;
typedef LONG HRESULT;
typedef long long LONGLONG;

typedef union _LARGE_INTEGER {
	struct {
		uint32_t LowPart;
		int32_t HighPart;
	} u;
	int64_t QuadPart;
} LARGE_INTEGER;

extern "C" BOOL QueryPerformanceCounter(LARGE_INTEGER * lpPerformanceCount);
extern "C" BOOL QueryPerformanceFrequency(LARGE_INTEGER * lpFrequency);

typedef struct _CmDriverContext_ {
	int deviceid;
	int device_rev;
	/* indicates whether the CreateBufferUp/CreateSurface2DUP is not supported */
	int userptr_enabled;
	/* indicates whether CM uses the shared bufmgr or not
	 * if it is zero, CM needs to initialize its own bufmgr. bufmgr is meaningless
	 * If it is true, CM will share bufmgr with other components*/
	int shared_bufmgr;
	dri_bufmgr *bufmgr;
} CmDriverContext;

#define DRM_BO_FLINK	0x01
#define DRM_BO_HANDLE	0x02

typedef struct _CmOsResource_ {
	VA_CM_FORMAT format;
	union {
		int aligned_width;
		int buf_bytes;
	};
	int aligned_height;
	/* indicates whether it is BO_FLINK or BO_HANLDE */
	int bo_flags;
	int bo_size;
	union {
		dri_bo *bo;
		UINT bo_flink;
	};
	/*
	 * the pitch/tile_type is only for 2D surface type. It should be ignored
	 * for VA_CM_FORMAT_BUFFER.
	 * pitch is in byte unit
	 * And width/height is in pixel unit.
	 */
	int pitch;
	/*
	 * It directly uses the I915_TILING_XX definition in i915_drm.h
	 * I915_TILING_NONE 0
	 * I915_TILING_X    1
	 * I915_TILIGN_Y    2
	 */
	int tile_type;
	/* orig_width/height represents original width/height of 2D surface */
	/* For the buffer type it can be ignored */
	int orig_width, orig_height;
} CmOsResource;

template < typename T > inline const char *CM_TYPE_NAME_UNMANGLED();

template <> inline const char *CM_TYPE_NAME_UNMANGLED < char >() {
	return "char";
}

template <> inline const char *CM_TYPE_NAME_UNMANGLED < signed char >() {
	return "signed char";
}

template <> inline const char *CM_TYPE_NAME_UNMANGLED < unsigned char >() {
	return "unsigned char";
}

template <> inline const char *CM_TYPE_NAME_UNMANGLED < short >() {
	return "short";
}

template <> inline const char *CM_TYPE_NAME_UNMANGLED < unsigned short >() {
	return "unsigned short";
}

template <> inline const char *CM_TYPE_NAME_UNMANGLED < int >() {
	return "int";
}

template <> inline const char *CM_TYPE_NAME_UNMANGLED < unsigned int >() {
	return "unsigned int";
}

template <> inline const char *CM_TYPE_NAME_UNMANGLED < long >() {
	return "long";
}

template <> inline const char *CM_TYPE_NAME_UNMANGLED < unsigned long >() {
	return "unsigned long";
}

template <> inline const char *CM_TYPE_NAME_UNMANGLED < float >() {
	return "float";
}

template <> inline const char *CM_TYPE_NAME_UNMANGLED < double >() {
	return "double";
}

#define CM_TYPE_NAME(type)   CM_TYPE_NAME_UNMANGLED<type>()

inline void *CM_ALIGNED_MALLOC(size_t size, size_t alignment)
{
	return memalign(alignment, size);
}

inline void CM_ALIGNED_FREE(void *memory)
{
	free(memory);
}

#define THREAD_HANDLE pthread_t
inline void CM_THREAD_CREATE(THREAD_HANDLE * handle, void *start_routine,
			     void *arg)
{
	int err = 0;
	err =
	    pthread_create(handle, NULL, (void *(*)(void *))start_routine, arg);
	if (err) {
		printf(" cm create thread failed! \n");
		exit(-1);
	}
}

inline void CM_THREAD_EXIT(void *retval)
{
	pthread_exit(retval);
}

inline int CM_THREAD_JOIN(THREAD_HANDLE * handle_array, int thread_cnt)
{
	void *tret;
	for (int i = 0; i < thread_cnt; i++) {
		pthread_join(handle_array[i], &tret);
	}
	return 0;
}

#endif				/* _CM_RT_LINUX_H_ */
