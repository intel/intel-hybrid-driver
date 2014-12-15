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

#ifndef __GENHW_COMMON_H__
#define __GENHW_COMMON_H__

#include "fourcc.h"
#include "os_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MIN
#define MIN( x, y ) (((x)<=(y))?(x):(y))
#endif

#ifndef MAX
#define MAX( x, y ) (((x)>=(y))?(x):(y))
#endif

	typedef enum _GENHW_SURFACE_TYPE {
		SURF_NONE = 0,
		SURF_OUT_RENDERTARGET,
		SURF_TYPE_COUNT
	} GENHW_SURFACE_TYPE;
	 C_ASSERT(SURF_TYPE_COUNT == 2);

	typedef struct _GENHW_PLANE_OFFSET {
		int iLockSurfaceOffset;
		int iSurfaceOffset;
		int iXOffset;
		int iYOffset;
	} GENHW_PLANE_OFFSET, *PGENHW_PLANE_OFFSET;

#ifdef __cplusplus
}
#endif
#endif
