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

#ifndef _FOURCC_H
#define _FOURCC_H

#define PF_FOURCC          0x0001
#define PF_RGB             0x0002

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)  \
                  ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
                  ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

#define FOURCC_YUY2   0x32595559
#define FOURCC_UYVY   0x59565955
#define FOURCC_VBID   0x44494256
#define FOURCC_YVYU   0x55595659
#define FOURCC_I420   0x30323449
#define FOURCC_IYUV   0x56555949
#define FOURCC_YV12   0x32315659
#define FOURCC_AI44   0x34344941
#define FOURCC_IA44   0x34344149
#define FOURCC_IF09   0x39304649
#define FOURCC_YVU9   0x39555659
#define FOURCC_VYUY   0x59555956
#define FOURCC_Y41P   0x50313459
#define FOURCC_HWFE   0x45465748
#define FOURCC_IMC1   0x31434D49
#define FOURCC_IMC2   0x32434D49
#define FOURCC_IMC3   0x33434D49
#define FOURCC_IMC4   0x34434D49
#define FOURCC_422P   0x50323234
#define FOURCC_NV12   0x3231564e
#define FOURCC_NV11   0x3131564e
#define FOURCC_NV21   0x3132564e
#define FOURCC_P016   0x36313050
#define FOURCC_P010   0x30313050
#define FOURCC_P208   0x38303250
#define FOURCC_AYUV   0x56555941

#define FOURCC_IVAC   0x43415649
#define FOURCC_IVAR   0x52415649
#define FOURCC_IVAT   0x54415649
#define FOURCC_IVAI   0x49415649
#define FOURCC_IVAV   0x56415649
#define FOURCC_IVAM   0x4D415649

#define FOURCC_FXT1   0x31495846

#define FOURCC_PPA8   0x38415050

#define FOURCC_GPAP   INTEL_GPA_FMT_PIPELINE
#define FOURCC_GPAI   INTEL_GPA_FMT_ILK_PERF_COUNT
#define FOURCC_GPAG   INTEL_GPA_FMT_GT_PERF_COUNT
#define FOURCC_GT01   INTEL_GPA_FMT_GT_PERF_COUNT_01
#define FOURCC_GT02   INTEL_GPA_FMT_GT_PERF_COUNT_02
#define FOURCC_GT03   INTEL_GPA_FMT_GT_PERF_COUNT_03
#define FOURCC_IB00   INTEL_GPA_FMT_IVB_PERF_COUNT
#define FOURCC_IB01   INTEL_GPA_FMT_IVB_PERF_COUNT_01
#define FOURCC_IB02   INTEL_GPA_FMT_IVB_PERF_COUNT_02
#define FOURCC_IB03   INTEL_GPA_FMT_IVB_PERF_COUNT_03
#define FOURCC_GTKT   INTEL_GPA_FMT_KERNEL_TRACE
#define FOURCC_GTKD   INTEL_GPA_FMT_KERNEL_DUMP
#define FOURCC_IBOC   INTEL_GPA_FMT_IVB_OVERRIDE_CTRL
#define FOURCC_GTOC   INTEL_GPA_FMT_GT_OVERRIDE_CTRL

#define FOURCC_ATI1   MAKEFOURCC( 'A', 'T', 'I', '1' )
#define FOURCC_ATI2   MAKEFOURCC( 'A', 'T', 'I', '2' )

#define FOURCC_NULL   MAKEFOURCC( 'N', 'U', 'L', 'L' )

#define FOURCC_INTZ   MAKEFOURCC( 'I', 'N', 'T', 'Z' )

#define FOURCC_DF16   MAKEFOURCC( 'D', 'F', '1', '6' )
#define FOURCC_DF24   MAKEFOURCC( 'D', 'F', '2', '4' )

#define FOURCC_RESZ   MAKEFOURCC( 'R', 'E', 'S', 'Z' )
#define RESZ_CODE     0x7fa05000

#define FOURCC_A2M_ENABLE   MAKEFOURCC( 'A', '2', 'M', '1' )
#define FOURCC_A2M_DISABLE  MAKEFOURCC( 'A', '2', 'M', '0' )
#define FOURCC_ATOC         MAKEFOURCC( 'A', 'T', 'O', 'C' )

#define FOURCC_GET4  MAKEFOURCC( 'G', 'E', 'T', '4' )
#define FOURCC_GET1  MAKEFOURCC( 'G', 'E', 'T', '1' )

#define FOURCC_422H    MAKEFOURCC('4','2','2','H')
#define FOURCC_422V    MAKEFOURCC('4','2','2','V')
#define FOURCC_411P    MAKEFOURCC('4','1','1','P')
#define FOURCC_411R    MAKEFOURCC('4','1','1','R')
#define FOURCC_444P    MAKEFOURCC('4','4','4','P')
#define FOURCC_RGBP    MAKEFOURCC('R','G','B','P')
#define FOURCC_BGRP    MAKEFOURCC('B','G','R','P')
#define FOURCC_400P    MAKEFOURCC('4','0','0','P')
#define FOURCC_420O    MAKEFOURCC('4','2','0','O')

#define FOURCC_IRW0    MAKEFOURCC('I','R','W','0')
#define FOURCC_IRW1    MAKEFOURCC('I','R','W','1')
#define FOURCC_IRW2    MAKEFOURCC('I','R','W','2')
#define FOURCC_IRW3    MAKEFOURCC('I','R','W','3')
#define FOURCC_IRW4    MAKEFOURCC('I','R','W','4')
#define FOURCC_IRW5    MAKEFOURCC('I','R','W','5')
#define FOURCC_IRW6    MAKEFOURCC('I','R','W','6')
#define FOURCC_IRW7    MAKEFOURCC('I','R','W','7')

#define FOURCC_Y416    MAKEFOURCC('Y','4','1','6')

#define FOURCC_CMAA    MAKEFOURCC('C','M','A','A')

#endif
