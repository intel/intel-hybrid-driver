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

#ifndef __GENHW_H__
#define __GENHW_H__

#include "gen_hw_common.h"
#include "os_utilities.h"
#include "os_util_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GENHW_HW_ASSERT(_expr)                                                       \
    GENOS_ASSERT(GENOS_COMPONENT_HW, GENOS_HW_SUBCOMP_HW, _expr)

#define GENHW_HW_ASSERTMESSAGE(_message, ...)                                        \
    GENOS_ASSERTMESSAGE(GENOS_COMPONENT_HW, GENOS_HW_SUBCOMP_HW, _message, ##__VA_ARGS__)

#define GENHW_HW_NORMALMESSAGE(_message, ...)                                        \
    GENOS_NORMALMESSAGE(GENOS_COMPONENT_HW, GENOS_HW_SUBCOMP_HW, _message, ##__VA_ARGS__)

#define GENHW_HW_CHK_NULL(_ptr)                                                      \
    GENOS_CHK_NULL(GENOS_COMPONENT_HW, GENOS_HW_SUBCOMP_HW, _ptr)

#define GENHW_HW_CHK_STATUS(_stmt)                                                   \
    GENOS_CHK_STATUS(GENOS_COMPONENT_HW, GENOS_HW_SUBCOMP_HW, _stmt)

#define GENHW_PUBLIC_ASSERT(_expr)                                                   \
    GENOS_ASSERT(GENOS_COMPONENT_HW, GENOS_HW_SUBCOMP_PUBLIC, _expr)

#define GENHW_PUBLIC_ASSERTMESSAGE(_message, ...)                                    \
    GENOS_ASSERTMESSAGE(GENOS_COMPONENT_HW, GENOS_HW_SUBCOMP_PUBLIC, _message, ##__VA_ARGS__)

	typedef struct _GENHW_HW_INTERFACE *PGENHW_HW_INTERFACE;

	typedef struct _GENHW_SETTINGS {
		INT iMediaStates;
	} GENHW_SETTINGS, *PGENHW_SETTINGS;

	typedef CONST GENHW_SETTINGS CGENHW_SETTINGS, *PCGENHW_SETTINGS;

	typedef struct _GENHW_SURFACE {
		RECT rcSrc;
		RECT rcDst;
		RECT rcMaxSrc;

		DWORD dwWidth;
		DWORD dwHeight;
		DWORD dwPitch;
		GENOS_TILE_TYPE TileType;

		GENHW_PLANE_OFFSET YPlaneOffset;
		GENHW_PLANE_OFFSET UPlaneOffset;
		GENHW_PLANE_OFFSET VPlaneOffset;

		GENOS_FORMAT Format;
		GENHW_SURFACE_TYPE SurfType;
		DWORD dwDepth;
		DWORD dwOffset;
		GENOS_RESOURCE OsResource;

	} GENHW_SURFACE, *PGENHW_SURFACE;

	typedef struct {
		PCGENHW_SETTINGS pcGenHwSettings;
	} GENHW_INITIALIZERS, *PGENHW_INITIALIZERS;

	GENOS_STATUS IntelGen_GetSurfaceInfo(PGENOS_INTERFACE pOsInterface,
					     PGENHW_SURFACE pSurface);

#ifdef __cplusplus
}
#endif
#endif
