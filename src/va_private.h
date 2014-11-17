/*
 * Copyright (C) 2012 Intel Corporation. All Rights Reserved.
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
 */

#ifndef __VA_PRIVATE_H__
#define __VA_PRIVATE_H__

#define VAEntrypointHybridEncSlice      -1
#define VAEncMiscParameterTypePrivate   -2
#define VAEncMbDataBufferType	        -4
#define VAEncMiscParameterTypeVP8HybridFrameUpdate      -3
#define VAEncMiscParameterTypeVP8SegmentMapParams	-4

typedef struct _VAEncMbDataLayout
{
    unsigned char MbCodeSize;
    unsigned int  MbCodeOffset;
    unsigned int  MbCodeStride;
    unsigned char MvNumber;
    unsigned int  MvOffset;
    unsigned int  MvStride;

} VAEncMbDataLayout;

typedef struct _VAEncMiscParameterVP8HybridFrameUpdate
{
    unsigned int    prev_frame_size;
    bool            two_prev_frame_size;
    unsigned short  ref_frame_cost[4];
    unsigned short  intra_mode_cost[4][4];
    unsigned short  inter_mode_cost[4];
    unsigned char   intra_non_dc_penalty_16x16[4];
    unsigned char   intra_non_dc_penalty_4x4[4];
    unsigned char   ref_q_index[3];
} VAEncMiscParameterVP8HybridFrameUpdate;

typedef struct _VAEncMiscParameterVP8FrameRate
{
    unsigned int    frame_rate;
} VAEncMiscParameterVP8FrameRate;

#endif

