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

#ifndef _INTEL_COMMON_VP9_H_
#define _INTEL_COMMON_VP9_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "media_drv_driver.h"

// VP9 Picture Parameters Buffer
typedef struct _INTEL_VP9_PIC_PARAMS_
{
    unsigned short              FrameHeightMinus1;               // [0..65535]
    unsigned short              FrameWidthMinus1;                // [0..65535]

    union
    {
        struct
        {
            uint32_t        frame_type                      : 1;        // [0..1]
            uint32_t        show_frame                      : 1;        // [0..1]
            uint32_t        error_resilient_mode            : 1;        // [0..1]
            uint32_t        intra_only                      : 1;        // [0..1]
            uint32_t        LastRefIdx                      : 3;        // [0..7]
            uint32_t        LastRefSignBias                 : 1;        // [0..1]
            uint32_t        GoldenRefIdx                    : 3;        // [0..7]
            uint32_t        GoldenRefSignBias               : 1;        // [0..1]
            uint32_t        AltRefIdx                       : 3;        // [0..7]
            uint32_t        AltRefSignBias                  : 1;        // [0..1]
            uint32_t        allow_high_precision_mv         : 1;        // [0..1]
            uint32_t        mcomp_filter_type               : 3;        // [0..7]
            uint32_t        frame_parallel_decoding_mode    : 1;        // [0..1]
            uint32_t        segmentation_enabled            : 1;        // [0..1]
            uint32_t        segmentation_temporal_update    : 1;        // [0..1]
            uint32_t        segmentation_update_map         : 1;        // [0..1]
            uint32_t        reset_frame_context             : 2;        // [0..3]
            uint32_t        refresh_frame_context           : 1;        // [0..1]
            uint32_t        frame_context_idx               : 2;        // [0..3]
            uint32_t        LosslessFlag                    : 1;        // [0..1]
            uint32_t        ReservedField                   : 2;        // [0]
        } fields;
        uint32_t            value;
    } PicFlags;

    VASurfaceID    RefFrameList[8];                            // [0..127, 0xFF]
    VASurfaceID    CurrPic;                                    // [0..127]
    uint8_t               filter_level;                               // [0..63]
    uint8_t               sharpness_level;                            // [0..7]
    uint8_t               log2_tile_rows;                             // [0..2]
    uint8_t               log2_tile_columns;                          // [0..5]
    uint8_t               UncompressedHeaderLengthInBytes;            // [0..255]
    uint16_t              FirstPartitionSize;                         // [0..65535]
    uint8_t               SegTreeProbs[7];
    uint8_t               SegPredProbs[3];
    
    uint32_t                BSBytesInBuffer;

    uint32_t                StatusReportFeedbackNumber;
} INTEL_VP9_PIC_PARAMS, *PINTEL_VP9_PIC_PARAMS;

typedef struct _INTEL_VP9_SEG_PARAMS_
{
    union
    {
        struct
        {
            uint32_t      SegmentReferenceEnabled         : 1;        // [0..1]
            uint32_t      SegmentReference                : 2;        // [0..3]
            uint32_t      SegmentReferenceSkipped         : 1;        // [0..1]
            uint32_t      ReservedField3                  : 12;       // [0]
        } fields;
        uint32_t            value;
    } SegmentFlags;

    uint8_t               FilterLevel[4][2];                          // [0..63]
    uint16_t              LumaACQuantScale;                           // 
    uint16_t              LumaDCQuantScale;                           // 
    uint16_t              ChromaACQuantScale;                         // 
    uint16_t              ChromaDCQuantScale;                         // 
} INTEL_VP9_SEG_PARAMS, *PINTEL_VP9_SEG_PARAMS;


typedef struct _INTEL_VP9_SEGMENT_PARAMS_
{
    INTEL_VP9_SEG_PARAMS     SegData[8];
} INTEL_VP9_SEGMENT_PARAMS, *PINTEL_VP9_SEGMENT_PARAMS;

typedef void                    VOID, *PVOID, *LPVOID, *FARPROC;
typedef sem_t           MOS_SEMAPHORE, *PMOS_SEMAPHORE;
typedef pthread_mutex_t MOS_MUTEX, *PMOS_MUTEX;
typedef pthread_t       MOS_THREAD, *PMOS_THREAD;
typedef uint32_t		BOOL;
typedef unsigned int            *PBOOL, BOOLEAN;
typedef unsigned int            UINT, *PUINT, UINT32, *PUINT32;
typedef unsigned char           BYTE, *PBYTE, *LPBYTE;
typedef unsigned char           UINT8, *PUINT8, UCHAR, *PUCHAR;
typedef short                   INT16, *PINT16, SHORT, *PSHORT;
typedef unsigned short          UINT16, *PUINT16, WORD, *PWORD, USHORT;
typedef unsigned int            DWORD, *PDWORD, *LPDWORD;
typedef int                     INT, *PINT, INT32, *PINT32;

typedef uint64_t      uint64, UINT64, *PUINT64, ULONGLONG;
typedef float                   FLOAT, *PFLOAT;
typedef double                  DOUBLE;
typedef void**                  PHANDLE;

typedef char                    *PINT8;

#endif
