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
 *     Zhao Yakui <yakui.zhao@intel.com>
 *
 */


#ifndef __INTEL_HOSTVLD_VP9_PARSER_H__
#define __INTEL_HOSTVLD_VP9_PARSER_H__

#include "intel_hybrid_hostvld_vp9_internal.h"
#include "intel_hybrid_common_vp9.h"

VAStatus Intel_HostvldVp9_ParseCompressedHeader(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState);

VAStatus Intel_HostvldVp9_ParseTiles(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState);

VAStatus Intel_HostvldVp9_PreParseTiles(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState);

VAStatus Intel_HostvldVp9_ParseTileColumn(
    PINTEL_HOSTVLD_VP9_TILE_STATE   pTileState, 
    DWORD                              dwTileX);

VAStatus Intel_HostvldVp9_PostParseTiles(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState);

#endif // __INTEL_HOSTVLD_VP9_PARSER_H__
