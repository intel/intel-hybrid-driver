/*
 * Copyright ©  2014 Intel Corporation
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
 *     Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 *
 */

#include "media_drv_encoder.h"
#include "media_drv_encoder_vp8.h"
#include "media_drv_driver.h"
#include "media_drv_hw_g8.h"
#include "media_drv_hw_g75.h"
#include "media_drv_hw_g7.h"
#include "media_drv_hwcmds.h"

static UINT16 pak_qp_input_table[160 * 18] =
{
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x4000, 0x0004, 0x0010, 0x4000, 0x0004, 0x0010,
  0x3333, 0x0005, 0x0010, 0x3333, 0x0005, 0x0010, 0x2000, 0x0008, 0x0010, 0x1999, 0x000a, 0x0010, 0x3333, 0x0005, 0x0010, 0x3333, 0x0005, 0x0010,
  0x2aaa, 0x0006, 0x0010, 0x2aaa, 0x0006, 0x0010, 0x1c71, 0x0009, 0x0010, 0x1555, 0x000c, 0x0010, 0x2aaa, 0x0006, 0x0010, 0x2aaa, 0x0006, 0x0010,
  0x2492, 0x0007, 0x0010, 0x2492, 0x0007, 0x0010, 0x1999, 0x000a, 0x0010, 0x1249, 0x000e, 0x0010, 0x2492, 0x0007, 0x0010, 0x2492, 0x0007, 0x0010,
  0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010, 0x1555, 0x000c, 0x0010, 0x1000, 0x0010, 0x0010, 0x2000, 0x0008, 0x0010, 0x2000, 0x0008, 0x0010,
  0x1c71, 0x0009, 0x0010, 0x1c71, 0x0009, 0x0010, 0x13b1, 0x000d, 0x0010, 0x0e38, 0x0012, 0x0010, 0x1c71, 0x0009, 0x0010, 0x1c71, 0x0009, 0x0010,
  0x1999, 0x000a, 0x0010, 0x1999, 0x000a, 0x0010, 0x1111, 0x000f, 0x0010, 0x0ccc, 0x0014, 0x0010, 0x1999, 0x000a, 0x0010, 0x1999, 0x000a, 0x0010,
  0x1745, 0x000b, 0x0010, 0x1999, 0x000a, 0x0010, 0x0f0f, 0x0011, 0x0010, 0x0ccc, 0x0014, 0x0010, 0x1745, 0x000b, 0x0010, 0x1999, 0x000a, 0x0010,
  0x1555, 0x000c, 0x0010, 0x1745, 0x000b, 0x0010, 0x0e38, 0x0012, 0x0010, 0x0ba2, 0x0016, 0x0010, 0x1555, 0x000c, 0x0010, 0x1745, 0x000b, 0x0010,
  0x13b1, 0x000d, 0x0010, 0x1555, 0x000c, 0x0010, 0x0ccc, 0x0014, 0x0010, 0x0aaa, 0x0018, 0x0010, 0x13b1, 0x000d, 0x0010, 0x1555, 0x000c, 0x0010,
  0x1249, 0x000e, 0x0010, 0x13b1, 0x000d, 0x0010, 0x0c30, 0x0015, 0x0010, 0x09d8, 0x001a, 0x0010, 0x1249, 0x000e, 0x0010, 0x13b1, 0x000d, 0x0010,
  0x1111, 0x000f, 0x0010, 0x1249, 0x000e, 0x0010, 0x0b21, 0x0017, 0x0010, 0x0924, 0x001c, 0x0010, 0x1111, 0x000f, 0x0010, 0x1249, 0x000e, 0x0010,
  0x1000, 0x0010, 0x0010, 0x1111, 0x000f, 0x0010, 0x0aaa, 0x0018, 0x0010, 0x0888, 0x001e, 0x0010, 0x1000, 0x0010, 0x0010, 0x1111, 0x000f, 0x0010,
  0x0f0f, 0x0011, 0x0010, 0x1000, 0x0010, 0x0010, 0x09d8, 0x001a, 0x0010, 0x0800, 0x0020, 0x0010, 0x0f0f, 0x0011, 0x0010, 0x1000, 0x0010, 0x0010,
  0x0e38, 0x0012, 0x0010, 0x0f0f, 0x0011, 0x0010, 0x097b, 0x001b, 0x0010, 0x0787, 0x0022, 0x0010, 0x0e38, 0x0012, 0x0010, 0x0f0f, 0x0011, 0x0010,
  0x0d79, 0x0013, 0x0010, 0x0f0f, 0x0011, 0x0010, 0x08d3, 0x001d, 0x0010, 0x0787, 0x0022, 0x0010, 0x0d79, 0x0013, 0x0010, 0x0f0f, 0x0011, 0x0010,
  0x0ccc, 0x0014, 0x0010, 0x0e38, 0x0012, 0x0010, 0x0842, 0x001f, 0x0010, 0x071c, 0x0024, 0x0010, 0x0ccc, 0x0014, 0x0010, 0x0e38, 0x0012, 0x0010,
  0x0c30, 0x0015, 0x0010, 0x0d79, 0x0013, 0x0010, 0x0800, 0x0020, 0x0010, 0x06bc, 0x0026, 0x0010, 0x0c30, 0x0015, 0x0010, 0x0d79, 0x0013, 0x0010,
  0x0ba2, 0x0016, 0x0010, 0x0ccc, 0x0014, 0x0010, 0x0787, 0x0022, 0x0010, 0x0666, 0x0028, 0x0010, 0x0ba2, 0x0016, 0x0010, 0x0ccc, 0x0014, 0x0010,
  0x0b21, 0x0017, 0x0010, 0x0ccc, 0x0014, 0x0010, 0x0750, 0x0023, 0x0010, 0x0666, 0x0028, 0x0010, 0x0b21, 0x0017, 0x0010, 0x0ccc, 0x0014, 0x0010,
  0x0aaa, 0x0018, 0x0010, 0x0c30, 0x0015, 0x0010, 0x06eb, 0x0025, 0x0010, 0x0618, 0x002a, 0x0010, 0x0aaa, 0x0018, 0x0010, 0x0c30, 0x0015, 0x0010,
  0x0a3d, 0x0019, 0x0010, 0x0c30, 0x0015, 0x0010, 0x06bc, 0x0026, 0x0010, 0x0618, 0x002a, 0x0010, 0x0a3d, 0x0019, 0x0010, 0x0c30, 0x0015, 0x0010,
  0x09d8, 0x001a, 0x0010, 0x0ba2, 0x0016, 0x0010, 0x0666, 0x0028, 0x0010, 0x05d1, 0x002c, 0x0010, 0x09d8, 0x001a, 0x0010, 0x0ba2, 0x0016, 0x0010,
  0x097b, 0x001b, 0x0010, 0x0ba2, 0x0016, 0x0010, 0x063e, 0x0029, 0x0010, 0x05d1, 0x002c, 0x0010, 0x097b, 0x001b, 0x0010, 0x0ba2, 0x0016, 0x0010,
  0x0924, 0x001c, 0x0010, 0x0b21, 0x0017, 0x0010, 0x05f4, 0x002b, 0x0010, 0x0590, 0x002e, 0x0010, 0x0924, 0x001c, 0x0010, 0x0b21, 0x0017, 0x0010,
  0x08d3, 0x001d, 0x0010, 0x0b21, 0x0017, 0x0010, 0x05d1, 0x002c, 0x0010, 0x0590, 0x002e, 0x0010, 0x08d3, 0x001d, 0x0010, 0x0b21, 0x0017, 0x0010,
  0x0888, 0x001e, 0x0010, 0x0aaa, 0x0018, 0x0010, 0x0590, 0x002e, 0x0010, 0x0555, 0x0030, 0x0010, 0x0888, 0x001e, 0x0010, 0x0aaa, 0x0018, 0x0010,
  0x0842, 0x001f, 0x0010, 0x0a3d, 0x0019, 0x0010, 0x0555, 0x0030, 0x0010, 0x051e, 0x0032, 0x0010, 0x0842, 0x001f, 0x0010, 0x0a3d, 0x0019, 0x0010,
  0x0800, 0x0020, 0x0010, 0x0a3d, 0x0019, 0x0010, 0x0539, 0x0031, 0x0010, 0x051e, 0x0032, 0x0010, 0x0800, 0x0020, 0x0010, 0x0a3d, 0x0019, 0x0010,
  0x07c1, 0x0021, 0x0010, 0x09d8, 0x001a, 0x0010, 0x0505, 0x0033, 0x0010, 0x04ec, 0x0034, 0x0010, 0x07c1, 0x0021, 0x0010, 0x09d8, 0x001a, 0x0010,
  0x0787, 0x0022, 0x0010, 0x097b, 0x001b, 0x0010, 0x04ec, 0x0034, 0x0010, 0x04bd, 0x0036, 0x0010, 0x0787, 0x0022, 0x0010, 0x097b, 0x001b, 0x0010,
  0x0750, 0x0023, 0x0010, 0x0924, 0x001c, 0x0010, 0x04bd, 0x0036, 0x0010, 0x0492, 0x0038, 0x0010, 0x0750, 0x0023, 0x0010, 0x0924, 0x001c, 0x0010,
  0x071c, 0x0024, 0x0010, 0x08d3, 0x001d, 0x0010, 0x04a7, 0x0037, 0x0010, 0x0469, 0x003a, 0x0010, 0x071c, 0x0024, 0x0010, 0x08d3, 0x001d, 0x0010,
  0x06eb, 0x0025, 0x0010, 0x0888, 0x001e, 0x0010, 0x047d, 0x0039, 0x0010, 0x0444, 0x003c, 0x0010, 0x06eb, 0x0025, 0x0010, 0x0888, 0x001e, 0x0010,
  0x06bc, 0x0026, 0x0010, 0x0842, 0x001f, 0x0010, 0x0469, 0x003a, 0x0010, 0x0421, 0x003e, 0x0010, 0x06bc, 0x0026, 0x0010, 0x0842, 0x001f, 0x0010,
  0x0690, 0x0027, 0x0010, 0x0800, 0x0020, 0x0010, 0x0444, 0x003c, 0x0010, 0x0400, 0x0040, 0x0010, 0x0690, 0x0027, 0x0010, 0x0800, 0x0020, 0x0010,
  0x0666, 0x0028, 0x0010, 0x07c1, 0x0021, 0x0010, 0x0421, 0x003e, 0x0010, 0x03e0, 0x0042, 0x0010, 0x0666, 0x0028, 0x0010, 0x07c1, 0x0021, 0x0010,
  0x063e, 0x0029, 0x0010, 0x0787, 0x0022, 0x0010, 0x0410, 0x003f, 0x0010, 0x03c3, 0x0044, 0x0010, 0x063e, 0x0029, 0x0010, 0x0787, 0x0022, 0x0010,
  0x0618, 0x002a, 0x0010, 0x0750, 0x0023, 0x0010, 0x03f0, 0x0041, 0x0010, 0x03a8, 0x0046, 0x0010, 0x0618, 0x002a, 0x0010, 0x0750, 0x0023, 0x0010,
  0x05f4, 0x002b, 0x0010, 0x071c, 0x0024, 0x0010, 0x03e0, 0x0042, 0x0010, 0x038e, 0x0048, 0x0010, 0x05f4, 0x002b, 0x0010, 0x071c, 0x0024, 0x0010,
  0x05d1, 0x002c, 0x0010, 0x06eb, 0x0025, 0x0010, 0x03c3, 0x0044, 0x0010, 0x0375, 0x004a, 0x0010, 0x05d1, 0x002c, 0x0010, 0x06eb, 0x0025, 0x0010,
  0x05b0, 0x002d, 0x0010, 0x06eb, 0x0025, 0x0010, 0x03b5, 0x0045, 0x0010, 0x0375, 0x004a, 0x0010, 0x05b0, 0x002d, 0x0010, 0x06eb, 0x0025, 0x0010,
  0x0590, 0x002e, 0x0010, 0x06bc, 0x0026, 0x0010, 0x039b, 0x0047, 0x0010, 0x035e, 0x004c, 0x0010, 0x0590, 0x002e, 0x0010, 0x06bc, 0x0026, 0x0010,
  0x0572, 0x002f, 0x0010, 0x0690, 0x0027, 0x0010, 0x038e, 0x0048, 0x0010, 0x0348, 0x004e, 0x0010, 0x0572, 0x002f, 0x0010, 0x0690, 0x0027, 0x0010,
  0x0555, 0x0030, 0x0010, 0x0666, 0x0028, 0x0010, 0x0375, 0x004a, 0x0010, 0x0333, 0x0050, 0x0010, 0x0555, 0x0030, 0x0010, 0x0666, 0x0028, 0x0010,
  0x0539, 0x0031, 0x0010, 0x063e, 0x0029, 0x0010, 0x0369, 0x004b, 0x0010, 0x031f, 0x0052, 0x0010, 0x0539, 0x0031, 0x0010, 0x063e, 0x0029, 0x0010,
  0x051e, 0x0032, 0x0010, 0x0618, 0x002a, 0x0010, 0x0353, 0x004d, 0x0010, 0x030c, 0x0054, 0x0010, 0x051e, 0x0032, 0x0010, 0x0618, 0x002a, 0x0010,
  0x0505, 0x0033, 0x0010, 0x05f4, 0x002b, 0x0010, 0x033d, 0x004f, 0x0010, 0x02fa, 0x0056, 0x0010, 0x0505, 0x0033, 0x0010, 0x05f4, 0x002b, 0x0010,
  0x04ec, 0x0034, 0x0010, 0x05d1, 0x002c, 0x0010, 0x0333, 0x0050, 0x0010, 0x02e8, 0x0058, 0x0010, 0x04ec, 0x0034, 0x0010, 0x05d1, 0x002c, 0x0010,
  0x04d4, 0x0035, 0x0010, 0x05b0, 0x002d, 0x0010, 0x031f, 0x0052, 0x0010, 0x02d8, 0x005a, 0x0010, 0x04d4, 0x0035, 0x0010, 0x05b0, 0x002d, 0x0010,
  0x04bd, 0x0036, 0x0010, 0x0590, 0x002e, 0x0010, 0x0315, 0x0053, 0x0010, 0x02c8, 0x005c, 0x0010, 0x04bd, 0x0036, 0x0010, 0x0590, 0x002e, 0x0010,
  0x04a7, 0x0037, 0x0010, 0x0590, 0x002e, 0x0010, 0x0303, 0x0055, 0x0010, 0x02c8, 0x005c, 0x0010, 0x04a7, 0x0037, 0x0010, 0x0590, 0x002e, 0x0010,
  0x0492, 0x0038, 0x0010, 0x0572, 0x002f, 0x0010, 0x02fa, 0x0056, 0x0010, 0x02b9, 0x005e, 0x0010, 0x0492, 0x0038, 0x0010, 0x0572, 0x002f, 0x0010,
  0x047d, 0x0039, 0x0010, 0x0555, 0x0030, 0x0010, 0x02e8, 0x0058, 0x0010, 0x02aa, 0x0060, 0x0010, 0x047d, 0x0039, 0x0010, 0x0555, 0x0030, 0x0010,
  0x0469, 0x003a, 0x0010, 0x0539, 0x0031, 0x0010, 0x02e0, 0x0059, 0x0010, 0x029c, 0x0062, 0x0010, 0x0469, 0x003a, 0x0010, 0x0539, 0x0031, 0x0010,
  0x0444, 0x003c, 0x0010, 0x051e, 0x0032, 0x0010, 0x02c0, 0x005d, 0x0010, 0x028f, 0x0064, 0x0010, 0x0444, 0x003c, 0x0010, 0x051e, 0x0032, 0x0010,
  0x0421, 0x003e, 0x0010, 0x0505, 0x0033, 0x0010, 0x02aa, 0x0060, 0x0010, 0x0282, 0x0066, 0x0010, 0x0421, 0x003e, 0x0010, 0x0505, 0x0033, 0x0010,
  0x0400, 0x0040, 0x0010, 0x04ec, 0x0034, 0x0010, 0x0295, 0x0063, 0x0010, 0x0276, 0x0068, 0x0010, 0x0400, 0x0040, 0x0010, 0x04ec, 0x0034, 0x0010,
  0x03e0, 0x0042, 0x0010, 0x04d4, 0x0035, 0x0010, 0x0282, 0x0066, 0x0010, 0x026a, 0x006a, 0x0010, 0x03e0, 0x0042, 0x0010, 0x04d4, 0x0035, 0x0010,
  0x03c3, 0x0044, 0x0010, 0x04bd, 0x0036, 0x0010, 0x0270, 0x0069, 0x0010, 0x025e, 0x006c, 0x0010, 0x03c3, 0x0044, 0x0010, 0x04bd, 0x0036, 0x0010,
  0x03a8, 0x0046, 0x0010, 0x04a7, 0x0037, 0x0010, 0x025e, 0x006c, 0x0010, 0x0253, 0x006e, 0x0010, 0x03a8, 0x0046, 0x0010, 0x04a7, 0x0037, 0x0010,
  0x038e, 0x0048, 0x0010, 0x0492, 0x0038, 0x0010, 0x024e, 0x006f, 0x0010, 0x0249, 0x0070, 0x0010, 0x038e, 0x0048, 0x0010, 0x0492, 0x0038, 0x0010,
  0x0375, 0x004a, 0x0010, 0x047d, 0x0039, 0x0010, 0x023e, 0x0072, 0x0010, 0x023e, 0x0072, 0x0010, 0x0375, 0x004a, 0x0010, 0x047d, 0x0039, 0x0010,
  0x035e, 0x004c, 0x0010, 0x0469, 0x003a, 0x0010, 0x0230, 0x0075, 0x0010, 0x0234, 0x0074, 0x0010, 0x035e, 0x004c, 0x0010, 0x0469, 0x003a, 0x0010,
  0x0348, 0x004e, 0x0010, 0x0456, 0x003b, 0x0010, 0x0222, 0x0078, 0x0010, 0x022b, 0x0076, 0x0010, 0x0348, 0x004e, 0x0010, 0x0456, 0x003b, 0x0010,
  0x0333, 0x0050, 0x0010, 0x0444, 0x003c, 0x0010, 0x0210, 0x007c, 0x0010, 0x0222, 0x0078, 0x0010, 0x0333, 0x0050, 0x0010, 0x0444, 0x003c, 0x0010,
  0x031f, 0x0052, 0x0010, 0x0432, 0x003d, 0x0010, 0x0204, 0x007f, 0x0010, 0x0219, 0x007a, 0x0010, 0x031f, 0x0052, 0x0010, 0x0432, 0x003d, 0x0010,
  0x030c, 0x0054, 0x0010, 0x0421, 0x003e, 0x0010, 0x01f8, 0x0082, 0x0010, 0x0210, 0x007c, 0x0010, 0x030c, 0x0054, 0x0010, 0x0421, 0x003e, 0x0010,
  0x02fa, 0x0056, 0x0010, 0x0410, 0x003f, 0x0010, 0x01ec, 0x0085, 0x0010, 0x0208, 0x007e, 0x0010, 0x02fa, 0x0056, 0x0010, 0x0410, 0x003f, 0x0010,
  0x02e8, 0x0058, 0x0010, 0x0400, 0x0040, 0x0010, 0x01e1, 0x0088, 0x0010, 0x0200, 0x0080, 0x0010, 0x02e8, 0x0058, 0x0010, 0x0400, 0x0040, 0x0010,
  0x02d8, 0x005a, 0x0010, 0x03f0, 0x0041, 0x0010, 0x01d7, 0x008b, 0x0010, 0x01f8, 0x0082, 0x0010, 0x02d8, 0x005a, 0x0010, 0x03f0, 0x0041, 0x0010,
  0x02c8, 0x005c, 0x0010, 0x03e0, 0x0042, 0x0010, 0x01cd, 0x008e, 0x0010, 0x01f0, 0x0084, 0x0010, 0x02c8, 0x005c, 0x0010, 0x03e0, 0x0042, 0x0010,
  0x02b9, 0x005e, 0x0010, 0x03d2, 0x0043, 0x0010, 0x01c3, 0x0091, 0x0010, 0x01e9, 0x0086, 0x0010, 0x02b9, 0x005e, 0x0010, 0x03d2, 0x0043, 0x0010,
  0x02aa, 0x0060, 0x0010, 0x03c3, 0x0044, 0x0010, 0x01ba, 0x0094, 0x0010, 0x01e1, 0x0088, 0x0010, 0x02aa, 0x0060, 0x0010, 0x03c3, 0x0044, 0x0010,
  0x029c, 0x0062, 0x0010, 0x03b5, 0x0045, 0x0010, 0x01b2, 0x0097, 0x0010, 0x01da, 0x008a, 0x0010, 0x029c, 0x0062, 0x0010, 0x03b5, 0x0045, 0x0010,
  0x028f, 0x0064, 0x0010, 0x03a8, 0x0046, 0x0010, 0x01a6, 0x009b, 0x0010, 0x01d4, 0x008c, 0x0010, 0x028f, 0x0064, 0x0010, 0x03a8, 0x0046, 0x0010,
  0x0282, 0x0066, 0x0010, 0x039b, 0x0047, 0x0010, 0x019e, 0x009e, 0x0010, 0x01cd, 0x008e, 0x0010, 0x0282, 0x0066, 0x0010, 0x039b, 0x0047, 0x0010,
  0x0276, 0x0068, 0x0010, 0x038e, 0x0048, 0x0010, 0x0197, 0x00a1, 0x0010, 0x01c7, 0x0090, 0x0010, 0x0276, 0x0068, 0x0010, 0x038e, 0x0048, 0x0010,
  0x026a, 0x006a, 0x0010, 0x0381, 0x0049, 0x0010, 0x018f, 0x00a4, 0x0010, 0x01c0, 0x0092, 0x0010, 0x026a, 0x006a, 0x0010, 0x0381, 0x0049, 0x0010,
  0x025e, 0x006c, 0x0010, 0x0375, 0x004a, 0x0010, 0x0188, 0x00a7, 0x0010, 0x01ba, 0x0094, 0x0010, 0x025e, 0x006c, 0x0010, 0x0375, 0x004a, 0x0010,
  0x0253, 0x006e, 0x0010, 0x0369, 0x004b, 0x0010, 0x0181, 0x00aa, 0x0010, 0x01b4, 0x0096, 0x0010, 0x0253, 0x006e, 0x0010, 0x0369, 0x004b, 0x0010,
  0x0249, 0x0070, 0x0010, 0x035e, 0x004c, 0x0010, 0x017a, 0x00ad, 0x0010, 0x01af, 0x0098, 0x0010, 0x0249, 0x0070, 0x0010, 0x035e, 0x004c, 0x0010,
  0x023e, 0x0072, 0x0010, 0x035e, 0x004c, 0x0010, 0x0174, 0x00b0, 0x0010, 0x01af, 0x0098, 0x0010, 0x023e, 0x0072, 0x0010, 0x035e, 0x004c, 0x0010,
  0x0234, 0x0074, 0x0010, 0x0353, 0x004d, 0x0010, 0x016e, 0x00b3, 0x0010, 0x01a9, 0x009a, 0x0010, 0x0234, 0x0074, 0x0010, 0x0353, 0x004d, 0x0010,
  0x0226, 0x0077, 0x0010, 0x0348, 0x004e, 0x0010, 0x0164, 0x00b8, 0x0010, 0x01a4, 0x009c, 0x0010, 0x0226, 0x0077, 0x0010, 0x0348, 0x004e, 0x0010,
  0x0219, 0x007a, 0x0010, 0x033d, 0x004f, 0x0010, 0x015a, 0x00bd, 0x0010, 0x019e, 0x009e, 0x0010, 0x0219, 0x007a, 0x0010, 0x033d, 0x004f, 0x0010,
  0x020c, 0x007d, 0x0010, 0x0333, 0x0050, 0x0010, 0x0153, 0x00c1, 0x0010, 0x0199, 0x00a0, 0x0010, 0x020c, 0x007d, 0x0010, 0x0333, 0x0050, 0x0010,
  0x0200, 0x0080, 0x0010, 0x0329, 0x0051, 0x0010, 0x014a, 0x00c6, 0x0010, 0x0194, 0x00a2, 0x0010, 0x0200, 0x0080, 0x0010, 0x0329, 0x0051, 0x0010,
  0x01f4, 0x0083, 0x0010, 0x031f, 0x0052, 0x0010, 0x0142, 0x00cb, 0x0010, 0x018f, 0x00a4, 0x0010, 0x01f4, 0x0083, 0x0010, 0x031f, 0x0052, 0x0010,
  0x01e9, 0x0086, 0x0010, 0x0315, 0x0053, 0x0010, 0x013c, 0x00cf, 0x0010, 0x018a, 0x00a6, 0x0010, 0x01e9, 0x0086, 0x0010, 0x0315, 0x0053, 0x0010,
  0x01de, 0x0089, 0x0010, 0x030c, 0x0054, 0x0010, 0x0135, 0x00d4, 0x0010, 0x0186, 0x00a8, 0x0010, 0x01de, 0x0089, 0x0010, 0x030c, 0x0054, 0x0010,
  0x01d4, 0x008c, 0x0010, 0x0303, 0x0055, 0x0010, 0x012e, 0x00d9, 0x0010, 0x0181, 0x00aa, 0x0010, 0x01d4, 0x008c, 0x0010, 0x0303, 0x0055, 0x0010,
  0x01ca, 0x008f, 0x0010, 0x02fa, 0x0056, 0x0010, 0x0128, 0x00dd, 0x0010, 0x017d, 0x00ac, 0x0010, 0x01ca, 0x008f, 0x0010, 0x02fa, 0x0056, 0x0010,
  0x01c0, 0x0092, 0x0010, 0x02f1, 0x0057, 0x0010, 0x0121, 0x00e2, 0x0010, 0x0178, 0x00ae, 0x0010, 0x01c0, 0x0092, 0x0010, 0x02f1, 0x0057, 0x0010,
  0x01b7, 0x0095, 0x0010, 0x02e8, 0x0058, 0x0010, 0x011c, 0x00e6, 0x0010, 0x0174, 0x00b0, 0x0010, 0x01b7, 0x0095, 0x0010, 0x02e8, 0x0058, 0x0010,
  0x01af, 0x0098, 0x0010, 0x02e0, 0x0059, 0x0010, 0x0116, 0x00eb, 0x0010, 0x0170, 0x00b2, 0x0010, 0x01af, 0x0098, 0x0010, 0x02e0, 0x0059, 0x0010,
  0x01a6, 0x009b, 0x0010, 0x02d0, 0x005b, 0x0010, 0x0111, 0x00f0, 0x0010, 0x0168, 0x00b6, 0x0010, 0x01a6, 0x009b, 0x0010, 0x02d0, 0x005b, 0x0010,
  0x019e, 0x009e, 0x0010, 0x02c0, 0x005d, 0x0010, 0x010c, 0x00f4, 0x0010, 0x0160, 0x00ba, 0x0010, 0x019e, 0x009e, 0x0010, 0x02c0, 0x005d, 0x0010,
  0x0197, 0x00a1, 0x0010, 0x02b1, 0x005f, 0x0010, 0x0107, 0x00f9, 0x0010, 0x0158, 0x00be, 0x0010, 0x0197, 0x00a1, 0x0010, 0x02b1, 0x005f, 0x0010,
  0x018f, 0x00a4, 0x0010, 0x02aa, 0x0060, 0x0010, 0x0102, 0x00fe, 0x0010, 0x0155, 0x00c0, 0x0010, 0x018f, 0x00a4, 0x0010, 0x02aa, 0x0060, 0x0010,
  0x0188, 0x00a7, 0x0010, 0x029c, 0x0062, 0x0010, 0x00fe, 0x0102, 0x0010, 0x014e, 0x00c4, 0x0010, 0x0188, 0x00a7, 0x0010, 0x029c, 0x0062, 0x0010,
  0x0181, 0x00aa, 0x0010, 0x028f, 0x0064, 0x0010, 0x00f9, 0x0107, 0x0010, 0x0147, 0x00c8, 0x0010, 0x0181, 0x00aa, 0x0010, 0x028f, 0x0064, 0x0010,
  0x017a, 0x00ad, 0x0010, 0x0288, 0x0065, 0x0010, 0x00f4, 0x010c, 0x0010, 0x0144, 0x00ca, 0x0010, 0x017a, 0x00ad, 0x0010, 0x0288, 0x0065, 0x0010,
  0x0172, 0x00b1, 0x0010, 0x0282, 0x0066, 0x0010, 0x00ef, 0x0112, 0x0010, 0x0141, 0x00cc, 0x0010, 0x0172, 0x00b1, 0x0010, 0x0282, 0x0066, 0x0010,
  0x016a, 0x00b5, 0x0010, 0x0276, 0x0068, 0x0010, 0x00ea, 0x0118, 0x0010, 0x013b, 0x00d0, 0x0010, 0x016a, 0x00b5, 0x0010, 0x0276, 0x0068, 0x0010,
  0x0162, 0x00b9, 0x0010, 0x026a, 0x006a, 0x0010, 0x00e5, 0x011e, 0x0010, 0x0135, 0x00d4, 0x0010, 0x0162, 0x00b9, 0x0010, 0x026a, 0x006a, 0x0010,
  0x015a, 0x00bd, 0x0010, 0x025e, 0x006c, 0x0010, 0x00e0, 0x0124, 0x0010, 0x012f, 0x00d8, 0x0010, 0x015a, 0x00bd, 0x0010, 0x025e, 0x006c, 0x0010,
  0x0153, 0x00c1, 0x0010, 0x0253, 0x006e, 0x0010, 0x00db, 0x012b, 0x0010, 0x0129, 0x00dc, 0x0010, 0x0153, 0x00c1, 0x0010, 0x0253, 0x006e, 0x0010,
  0x014c, 0x00c5, 0x0010, 0x0249, 0x0070, 0x0010, 0x00d6, 0x0131, 0x0010, 0x0124, 0x00e0, 0x0010, 0x014c, 0x00c5, 0x0010, 0x0249, 0x0070, 0x0010,
  0x0146, 0x00c9, 0x0010, 0x023e, 0x0072, 0x0010, 0x00d2, 0x0137, 0x0010, 0x011f, 0x00e4, 0x0010, 0x0146, 0x00c9, 0x0010, 0x023e, 0x0072, 0x0010,
  0x013f, 0x00cd, 0x0010, 0x0234, 0x0074, 0x0010, 0x00ce, 0x013d, 0x0010, 0x011a, 0x00e8, 0x0010, 0x013f, 0x00cd, 0x0010, 0x0234, 0x0074, 0x0010,
  0x0139, 0x00d1, 0x0010, 0x022b, 0x0076, 0x0010, 0x00ca, 0x0143, 0x0010, 0x0115, 0x00ec, 0x0010, 0x0139, 0x00d1, 0x0010, 0x022b, 0x0076, 0x0010,
  0x0133, 0x00d5, 0x0010, 0x0219, 0x007a, 0x0010, 0x00c6, 0x014a, 0x0010, 0x010c, 0x00f4, 0x0010, 0x0133, 0x00d5, 0x0010, 0x0219, 0x007a, 0x0010,
  0x012e, 0x00d9, 0x0010, 0x0210, 0x007c, 0x0010, 0x00c3, 0x0150, 0x0010, 0x0108, 0x00f8, 0x0010, 0x012e, 0x00d9, 0x0010, 0x0210, 0x007c, 0x0010,
  0x0128, 0x00dd, 0x0010, 0x0208, 0x007e, 0x0010, 0x00bf, 0x0156, 0x0010, 0x0104, 0x00fc, 0x0010, 0x0128, 0x00dd, 0x0010, 0x0208, 0x007e, 0x0010,
  0x0123, 0x00e1, 0x0010, 0x0200, 0x0080, 0x0010, 0x00bc, 0x015c, 0x0010, 0x0100, 0x0100, 0x0010, 0x0123, 0x00e1, 0x0010, 0x0200, 0x0080, 0x0010,
  0x011e, 0x00e5, 0x0010, 0x01f8, 0x0082, 0x0010, 0x00b9, 0x0162, 0x0010, 0x00fc, 0x0104, 0x0010, 0x011e, 0x00e5, 0x0010, 0x01f8, 0x0082, 0x0010,
  0x0118, 0x00ea, 0x0010, 0x01f0, 0x0084, 0x0010, 0x00b5, 0x016a, 0x0010, 0x00f8, 0x0108, 0x0010, 0x0118, 0x00ea, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x0112, 0x00ef, 0x0010, 0x01e9, 0x0086, 0x0010, 0x00b1, 0x0172, 0x0010, 0x00f4, 0x010c, 0x0010, 0x0112, 0x00ef, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x010b, 0x00f5, 0x0010, 0x01e1, 0x0088, 0x0010, 0x00ac, 0x017b, 0x0010, 0x00f0, 0x0110, 0x0010, 0x010b, 0x00f5, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x0107, 0x00f9, 0x0010, 0x01da, 0x008a, 0x0010, 0x00aa, 0x0181, 0x0010, 0x00ed, 0x0114, 0x0010, 0x0107, 0x00f9, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x0102, 0x00fe, 0x0010, 0x01d4, 0x008c, 0x0010, 0x00a6, 0x0189, 0x0010, 0x00ea, 0x0118, 0x0010, 0x0102, 0x00fe, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00fd, 0x0103, 0x0010, 0x01ca, 0x008f, 0x0010, 0x00a3, 0x0191, 0x0010, 0x00e5, 0x011e, 0x0010, 0x00fd, 0x0103, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00f8, 0x0108, 0x0010, 0x01c3, 0x0091, 0x0010, 0x00a0, 0x0199, 0x0010, 0x00e1, 0x0122, 0x0010, 0x00f8, 0x0108, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00f3, 0x010d, 0x0010, 0x01ba, 0x0094, 0x0010, 0x009d, 0x01a0, 0x0010, 0x00dd, 0x0128, 0x0010, 0x00f3, 0x010d, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00ef, 0x0112, 0x0010, 0x01b2, 0x0097, 0x0010, 0x009a, 0x01a8, 0x0010, 0x00d9, 0x012e, 0x0010, 0x00ef, 0x0112, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00ea, 0x0117, 0x0010, 0x01a9, 0x009a, 0x0010, 0x0097, 0x01b0, 0x0010, 0x00d4, 0x0134, 0x0010, 0x00ea, 0x0117, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010,
  0x00e6, 0x011c, 0x0010, 0x01a1, 0x009d, 0x0010, 0x0094, 0x01b8, 0x0010, 0x00d0, 0x013a, 0x0010, 0x00e6, 0x011c, 0x0010, 0x01f0, 0x0084, 0x0010
};

VOID
gpe_context_vfe_scoreboardinit_vp8 (MEDIA_GPE_CTX * gpe_context)
{
  gpe_context->vfe_state.vfe_desc5.scoreboard0.mask = 0xFF;
  gpe_context->vfe_state.vfe_desc5.scoreboard0.type = SCOREBOARD_NON_STALLING;
  gpe_context->vfe_state.vfe_desc5.scoreboard0.enable = 1;

  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x0 = 0xF;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y0 = 0;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x1 = 0;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y1 = 0xF;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x2 = 1;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y2 = 0xF;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x3 = 0xF;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y3 = 0xF;

  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x4 = 0xF;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y4 = 1;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x5 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y5 = 0xE;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x6 = 1;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y6 = 0xE;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x7 = 0xF;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y7 = 0xE;
}

VOID
gpe_context_vfe_scoreboardinit_pak_vp8 (MEDIA_GPE_CTX * gpe_context)
{
  gpe_context->vfe_state.vfe_desc5.scoreboard0.mask = 0x07;
  gpe_context->vfe_state.vfe_desc5.scoreboard0.type = SCOREBOARD_NON_STALLING;
  gpe_context->vfe_state.vfe_desc5.scoreboard0.enable = 1;

  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x0 = 0xF;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y0 = 0;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x1 = 0;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y1 = 0xF;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x2 = 1;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y2 = 0xE;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_x3 = 0x0;
  gpe_context->vfe_state.vfe_desc6.scoreboard1.delta_y3 = 0x0;

  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x4 = 0x0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y4 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x5 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y5 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x6 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y6 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_x7 = 0;
  gpe_context->vfe_state.vfe_desc7.scoreboard2.delta_y7 = 0;
}

VOID
media_alloc_resource_mbpak (VADriverContextP ctx,
			    MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  MBPAK_CONTEXT *mbpak_context = &encoder_context->mbpak_context;
  /*UINT pic_w_h_in_mb =
     (encoder_context->picture_width_in_mbs *
     ((encoder_context->picture_height_in_mbs + 1) >> 1)) * 2; */
  //row buffer y
  mbpak_context->row_buffer_y.surface_array_spacing = 0x1;
  mbpak_context->row_buffer_y.tiling = I915_TILING_NONE;
  mbpak_context->row_buffer_y.bo_size =
    encoder_context->picture_width_in_mbs * 16;
  media_allocate_resource (&mbpak_context->row_buffer_y,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "row_buffer y",
			   mbpak_context->row_buffer_y.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->row_buffer_y.bo);

  //row buffer uv
  mbpak_context->row_buffer_uv.surface_array_spacing = 0x1;
  mbpak_context->row_buffer_uv.tiling = I915_TILING_NONE;
  mbpak_context->row_buffer_uv.bo_size =
    encoder_context->picture_width_in_mbs * 16;
  media_allocate_resource (&mbpak_context->row_buffer_uv,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "row_buffer uv",
			   mbpak_context->row_buffer_uv.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->row_buffer_uv.bo);

  //column buffer y
  mbpak_context->column_buffer_y.surface_array_spacing = 0x1;
  mbpak_context->column_buffer_y.tiling = I915_TILING_NONE;
  mbpak_context->column_buffer_y.bo_size =
    encoder_context->picture_height_in_mbs * 16;
  media_allocate_resource (&mbpak_context->column_buffer_y,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "column buffer y",
			   mbpak_context->column_buffer_y.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->column_buffer_y.bo);

  //column buffer uv
  mbpak_context->column_buffer_uv.surface_array_spacing = 0x1;
  mbpak_context->column_buffer_uv.tiling = I915_TILING_NONE;
  mbpak_context->column_buffer_uv.bo_size =
    encoder_context->picture_height_in_mbs * 16;
  media_allocate_resource (&mbpak_context->column_buffer_uv,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "column buffer uv",
			   mbpak_context->column_buffer_uv.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->column_buffer_uv.bo);

  mbpak_context->kernel_dump_buffer.surface_array_spacing = 0x1;
  mbpak_context->kernel_dump_buffer.tiling = I915_TILING_NONE;
  mbpak_context->kernel_dump_buffer.bo_size = KERNEL_DUMP_SIZE_VP8;	//pic_w_h_in_mb;
  media_allocate_resource (&mbpak_context->kernel_dump_buffer,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "kernel dump buffer mbpak",
			   mbpak_context->kernel_dump_buffer.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbpak_context->kernel_dump_buffer.bo);
}

VOID
media_alloc_resource_mbenc (VADriverContextP ctx,
			    MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  MBENC_CONTEXT *mbenc_context = &encoder_context->mbenc_context;
  UINT sz = 0;
  UINT pic_w_h_in_mb;
  UINT num_of_mb_blocks =
    encoder_context->picture_width_in_mbs *
    encoder_context->picture_height_in_mbs;
  pic_w_h_in_mb = num_of_mb_blocks;	/*(encoder_context->picture_width_in_mbs *
					   ((encoder_context->picture_height_in_mbs + 1) >> 1)) * 2; */
  if (encoder_context->brc_enabled)
    {
      encoder_context->mb_data_offset = ALIGN((MB_CODE_SIZE_VP8 * sizeof(UINT) + MB_MV_CODE_SIZE_VP8), 32);
    }
  else
    {
      encoder_context->mb_data_offset = 0;
    }

  encoder_context->mb_data_in_bytes = pic_w_h_in_mb * MB_CODE_SIZE_VP8 * sizeof(UINT);
  encoder_context->mv_offset = ALIGN((encoder_context->mb_data_offset + encoder_context->mb_data_in_bytes), MB_MV_ALIGNMENT);
  encoder_context->mv_in_bytes = pic_w_h_in_mb * MB_MV_CODE_SIZE_VP8;

  mbenc_context->mb_mode_cost_luma_buffer.width =
    ALIGN ((sizeof (UINT) * 10), 64);
  mbenc_context->mb_mode_cost_luma_buffer.height = 1;
  mbenc_context->mb_mode_cost_luma_buffer.surface_array_spacing = 0x1;
  mbenc_context->mb_mode_cost_luma_buffer.tiling = I915_TILING_NONE;
  mbenc_context->mb_mode_cost_luma_buffer.pitch =
    mbenc_context->mb_mode_cost_luma_buffer.width;

  media_allocate_resource (&mbenc_context->mb_mode_cost_luma_buffer,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "mb mode cost luma buffer", 0x1000,
			   4096);
  MEDIA_DRV_ASSERT (mbenc_context->mb_mode_cost_luma_buffer.bo);

  mbenc_context->block_mode_cost_buffer.width =
    ALIGN ((sizeof (UINT16)) * 10 * 10 * 10, 64);
  mbenc_context->block_mode_cost_buffer.height = 1;
  mbenc_context->block_mode_cost_buffer.surface_array_spacing = 0x1;
  mbenc_context->block_mode_cost_buffer.tiling = I915_TILING_NONE;
  mbenc_context->block_mode_cost_buffer.pitch =
    mbenc_context->block_mode_cost_buffer.width;
  media_allocate_resource (&mbenc_context->block_mode_cost_buffer,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "block mode cost buffer", 0x1000,
			   4096);
  MEDIA_DRV_ASSERT (mbenc_context->block_mode_cost_buffer.bo);

  mbenc_context->chroma_reconst_buffer.width = 64;
  mbenc_context->chroma_reconst_buffer.height = num_of_mb_blocks;
  mbenc_context->chroma_reconst_buffer.surface_array_spacing = 0x1;
  mbenc_context->chroma_reconst_buffer.tiling = I915_TILING_NONE;
  mbenc_context->chroma_reconst_buffer.pitch =
    mbenc_context->chroma_reconst_buffer.width;
  sz =
    mbenc_context->chroma_reconst_buffer.width *
    mbenc_context->chroma_reconst_buffer.height;
  media_allocate_resource (&mbenc_context->chroma_reconst_buffer,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "chrome reconst buffer", sz, 4096);
  MEDIA_DRV_ASSERT (mbenc_context->chroma_reconst_buffer.bo);


  mbenc_context->histogram_buffer.surface_array_spacing = 0x1;
  mbenc_context->histogram_buffer.tiling = I915_TILING_NONE;
  mbenc_context->histogram_buffer.bo_size = HISTOGRAM_SIZE;
  media_allocate_resource (&mbenc_context->histogram_buffer,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "histogram buffer", HISTOGRAM_SIZE,
			   4096);
  MEDIA_DRV_ASSERT (mbenc_context->histogram_buffer.bo);

  mbenc_context->kernel_dump_buffer.surface_array_spacing = 0x1;
  mbenc_context->kernel_dump_buffer.tiling = I915_TILING_NONE;
  mbenc_context->kernel_dump_buffer.bo_size = KERNEL_DUMP_SIZE_VP8;	// pic_w_h_in_mb;
  media_allocate_resource (&mbenc_context->kernel_dump_buffer,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "kernel dump buffer",
			   mbenc_context->kernel_dump_buffer.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbenc_context->kernel_dump_buffer.bo);

  mbenc_context->ref_frm_count_surface.surface_array_spacing = 0x1;
  mbenc_context->ref_frm_count_surface.tiling = I915_TILING_NONE;
  mbenc_context->ref_frm_count_surface.bo_size = 32;
  media_allocate_resource (&mbenc_context->ref_frm_count_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "reference frame mb count surface",
			   mbenc_context->ref_frm_count_surface.bo_size,
			   4096);
  MEDIA_DRV_ASSERT (mbenc_context->ref_frm_count_surface.bo);

  mbenc_context->pred_mv_data_surface.surface_array_spacing = 0x1;
  mbenc_context->pred_mv_data_surface.tiling = I915_TILING_NONE;
  mbenc_context->pred_mv_data_surface.bo_size =
    4 * encoder_context->picture_width_in_mbs *
    encoder_context->picture_height_in_mbs * sizeof (UINT);
  media_allocate_resource (&mbenc_context->pred_mv_data_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "pred mv data surface",
			   mbenc_context->pred_mv_data_surface.bo_size, 4096);
  MEDIA_DRV_ASSERT (mbenc_context->pred_mv_data_surface.bo);

  mbenc_context->mode_cost_update_surface.surface_array_spacing = 0x1;
  mbenc_context->mode_cost_update_surface.tiling = I915_TILING_NONE;
  mbenc_context->mode_cost_update_surface.bo_size = 16 * sizeof (UINT);
  media_allocate_resource (&mbenc_context->mode_cost_update_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "mode cost update surface",
			   mbenc_context->mode_cost_update_surface.bo_size,
			   4096);
  MEDIA_DRV_ASSERT (mbenc_context->mode_cost_update_surface.bo);

  mbenc_context->pred_mb_quant_data_surface.surface_array_spacing = 0x1;
  mbenc_context->pred_mb_quant_data_surface.tiling = I915_TILING_NONE;
  mbenc_context->pred_mb_quant_data_surface.width =
    ALIGN ((encoder_context->picture_width_in_mbs * 4), 64);
  mbenc_context->pred_mb_quant_data_surface.height =
    encoder_context->picture_height_in_mbs;
  mbenc_context->pred_mb_quant_data_surface.pitch =
    mbenc_context->pred_mb_quant_data_surface.width;
  sz =
    mbenc_context->pred_mb_quant_data_surface.width *
    mbenc_context->pred_mb_quant_data_surface.height;
  media_allocate_resource (&mbenc_context->pred_mb_quant_data_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "pred mb quant data surface", sz,
			   4096);
  MEDIA_DRV_ASSERT (mbenc_context->pred_mb_quant_data_surface.bo);
}

VOID
media_alloc_resource_me (VADriverContextP ctx,
			 MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  ME_CONTEXT *me_context = &encoder_context->me_context;

  me_context->mv_data_surface_4x_me.width =
    ALIGN ((encoder_context->down_scaled_width_mb4x * 32), 64);
  me_context->mv_data_surface_4x_me.height =
    encoder_context->down_scaled_height_mb4x * 4;
  me_context->mv_data_surface_4x_me.surface_array_spacing = 0x1;
  me_context->mv_data_surface_4x_me.tiling = I915_TILING_NONE;
  me_context->mv_data_surface_4x_me.pitch =
    me_context->mv_data_surface_4x_me.width;
  media_allocate_resource (&me_context->mv_data_surface_4x_me,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "mv data surface 4x_me",
			   (me_context->mv_data_surface_4x_me.width *
			    me_context->mv_data_surface_4x_me.height), 4096);
  MEDIA_DRV_ASSERT (me_context->mv_data_surface_4x_me.bo);


  me_context->mv_distortion_surface_4x_me.width =
    encoder_context->down_scaled_width_mb4x * 8;
  me_context->mv_distortion_surface_4x_me.height =
    ALIGN ((encoder_context->down_scaled_height_mb4x * 4 * 4), 8);
  me_context->mv_distortion_surface_4x_me.surface_array_spacing = 0x1;
  me_context->mv_distortion_surface_4x_me.tiling = I915_TILING_NONE;
  me_context->mv_distortion_surface_4x_me.pitch =
    ALIGN (me_context->mv_distortion_surface_4x_me.width, 64);
  media_allocate_resource (&me_context->mv_distortion_surface_4x_me,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "mv distortion surface 4x_me",
			   0x1000, 4096);
  MEDIA_DRV_ASSERT (me_context->mv_distortion_surface_4x_me.bo);

  me_context->mv_data_surface_16x_me.width =
    encoder_context->down_scaled_width_mb16x * 32;
  me_context->mv_data_surface_16x_me.height =
    encoder_context->down_scaled_height_mb16x * 4;
  me_context->mv_data_surface_16x_me.surface_array_spacing = 0x1;
  me_context->mv_data_surface_16x_me.tiling = I915_TILING_NONE;
  me_context->mv_data_surface_16x_me.pitch =
    ALIGN(me_context->mv_data_surface_16x_me.width, 64);
  media_allocate_resource (&me_context->mv_data_surface_16x_me,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "mv data surface 16x_me", 0x1000,
			   4096);
  MEDIA_DRV_ASSERT (me_context->mv_data_surface_16x_me.bo);

}

static VOID
media_encoder_context_params_init (MEDIA_DRV_CONTEXT * drv_ctx,
				   MEDIA_ENCODER_CTX * encoder_context)
{
  if (IS_HSW_GT1 (drv_ctx->drv_data.device_id)
      || (IS_IVYBRIDGE (drv_ctx->drv_data.device_id)))
    {
      encoder_context->walker_mode = SINGLE_MODE;
    }
  else if (IS_HSW_GT2 (drv_ctx->drv_data.device_id))
    {
      encoder_context->walker_mode = DUAL_MODE;
    }
  else if (IS_HSW_GT3 (drv_ctx->drv_data.device_id))
    {
      encoder_context->walker_mode = QUAD_MODE;
    }

  encoder_context->kernel_mode = 0;
  encoder_context->frame_num = 0;
  encoder_context->use_hw_scoreboard = 1;
  encoder_context->scaling_enabled = 0;
  encoder_context->me_16x_supported = 0;
  encoder_context->hme_supported = 0;
  encoder_context->kernel_dump_enable = 0;
  encoder_context->brc_enabled = (encoder_context->internal_rate_mode == HB_BRC_CBR ||
                                  encoder_context->internal_rate_mode == HB_BRC_VBR);
  encoder_context->brc_initted = 0;
  encoder_context->frame_rate = 3000; /* in case user doesn't set frame rate */

  if (encoder_context->hme_supported == 1)
    {
      encoder_context->scaling_enabled = 1;
      encoder_context->me_16x_supported = 1;
    }
  encoder_context->mbenc_chroma_kernel = TRUE;
  encoder_context->mbenc_curbe_set_brc_update = FALSE;
  encoder_context->mbpak_curbe_set_brc_update = FALSE;
  encoder_context->frame_width = encoder_context->picture_width;
  encoder_context->frame_height = encoder_context->picture_height;

  encoder_context->picture_width_in_mbs =
    (UINT) WIDTH_IN_MACROBLOCKS (encoder_context->picture_width);
  encoder_context->picture_height_in_mbs =
    (UINT) HEIGHT_IN_MACROBLOCKS (encoder_context->picture_height);
  encoder_context->down_scaled_width_mb4x =
    WIDTH_IN_MACROBLOCKS (encoder_context->picture_width / SCALE_FACTOR_4x);
  encoder_context->down_scaled_height_mb4x =
    HEIGHT_IN_MACROBLOCKS (encoder_context->picture_height / SCALE_FACTOR_4x);
  encoder_context->down_scaled_width_mb16x =
    WIDTH_IN_MACROBLOCKS (encoder_context->picture_width / SCALE_FACTOR_16x);
  encoder_context->down_scaled_height_mb16x =
    HEIGHT_IN_MACROBLOCKS (encoder_context->picture_height /
			   SCALE_FACTOR_16x);
  encoder_context->down_scaled_width_mb32x =
    WIDTH_IN_MACROBLOCKS (encoder_context->picture_width / SCALE_FACTOR_32x);
  encoder_context->down_scaled_height_mb32x =
    HEIGHT_IN_MACROBLOCKS (encoder_context->picture_height /
			   SCALE_FACTOR_32x);
}

VOID
media_mbpak_context_init_vp8 (VADriverContextP ctx,
			      MEDIA_ENCODER_CTX * encoder_context)
{
  MBPAK_CONTEXT *mbpak_context = &encoder_context->mbpak_context;
  MEDIA_GPE_CTX *gpe_context = &mbpak_context->gpe_context;
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = /* 0xc0; */ CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 280 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 120;	//16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE - 1;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels[7], 2);

  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	// * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;	//0xco
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_mbpak (gpe_context);

  gpe_context = &mbpak_context->gpe_context2;
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = /* 0xc0; */ CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 280 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 120;	//16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE - 1;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels[7], 2);

  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	// * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;	//0xco
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_mbpak (gpe_context);

  media_alloc_resource_mbpak (ctx, encoder_context);
  return;
}

VOID
media_mbenc_context_init (VADriverContextP ctx,
			  MEDIA_ENCODER_CTX * encoder_context)
{
  MBENC_CONTEXT *mbenc_context = &encoder_context->mbenc_context;
  MEDIA_GPE_CTX *gpe_context = &mbenc_context->gpe_context;
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 280 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 120;	//121;    //16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE - 1;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels[0], 5);
  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	//* MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_mbenc (encoder_context);
  media_alloc_resource_mbenc (ctx, encoder_context);
  return;
}

VOID
media_me_context_init (VADriverContextP ctx,
		       MEDIA_ENCODER_CTX * encoder_context)
{
  ME_CONTEXT *me_context = &encoder_context->me_context;
  MEDIA_GPE_CTX *gpe_context = &me_context->gpe_context;

  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 280 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 121;	//16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE - 1;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  /*media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels[3], 1); */
#if 0
  gpe_context->surface_state_binding_table.res.bo_size =
    (SURFACE_STATE_PADDED_SIZE + sizeof (UINT)) * MAX_MEDIA_SURFACES_GEN6;
  gpe_context->surface_state_binding_table.table_name = "binding table me";
#endif
  gpe_context->idrt_size =
    sizeof (struct media_interface_descriptor_data) * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);

  media_alloc_resource_me (ctx, encoder_context);
}

VOID
media_alloc_resource_brc_init_reset (VADriverContextP ctx,
                                     MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  BRC_INIT_RESET_CONTEXT *brc_init_reset_context = &encoder_context->brc_init_reset_context;
  BYTE *pbuffer;
  int size, i;
  BOOL status;

  brc_init_reset_context->brc_history.surface_array_spacing = 0x1;
  brc_init_reset_context->brc_history.tiling = I915_TILING_NONE;
  brc_init_reset_context->brc_history.bo_size = 544; // 17 GRF registers
  media_allocate_resource (&brc_init_reset_context->brc_history,
			   drv_ctx->drv_data.bufmgr,(const BYTE*) "BRC history buffer",
			   HISTOGRAM_SIZE, 4096);
  MEDIA_DRV_ASSERT (brc_init_reset_context->brc_history.bo);

  brc_init_reset_context->brc_distortion.width =
    ALIGN ((encoder_context->down_scaled_width_mb4x * 8), 64);
  brc_init_reset_context->brc_distortion.height =
    2 * ALIGN ((encoder_context->down_scaled_height_mb4x * 4), 8);
  brc_init_reset_context->brc_distortion.surface_array_spacing = 0x1;
  brc_init_reset_context->brc_distortion.tiling = I915_TILING_NONE;
  brc_init_reset_context->brc_distortion.pitch = brc_init_reset_context->brc_distortion.width;
  brc_init_reset_context->brc_distortion.bo_size = brc_init_reset_context->brc_distortion.pitch *
    brc_init_reset_context->brc_distortion.height;
  media_allocate_resource (&brc_init_reset_context->brc_distortion,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE*)"BRC distortion surface",
			   brc_init_reset_context->brc_distortion.bo_size,
			   4096);
  MEDIA_DRV_ASSERT (brc_init_reset_context->brc_distortion.bo);

  size = 160 * 18 * sizeof(UINT16);

  brc_init_reset_context->brc_pak_qp_input_table.surface_array_spacing = 0x1;
  brc_init_reset_context->brc_pak_qp_input_table.tiling = I915_TILING_NONE;
  brc_init_reset_context->brc_pak_qp_input_table.bo_size = size;
  media_allocate_resource (&brc_init_reset_context->brc_pak_qp_input_table,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE*) "BRC PAK QP Input Table",
			   size,
			   4096);
  MEDIA_DRV_ASSERT (brc_init_reset_context->brc_pak_qp_input_table.bo);

  pbuffer =
    (BYTE *) media_map_buffer_obj (brc_init_reset_context->brc_pak_qp_input_table.bo);
  MEDIA_DRV_ASSERT (pbuffer);

  status = media_drv_memcpy (pbuffer,
			     size,
			     (VOID *) pak_qp_input_table,
			     size);

  if (status != TRUE) {
    media_unmap_buffer_obj (brc_init_reset_context->brc_pak_qp_input_table.bo);
    MEDIA_DRV_ASSERT ("media_drv_memcpy failed");
  }

  media_unmap_buffer_obj (brc_init_reset_context->brc_pak_qp_input_table.bo);

  size = 2880;
  brc_init_reset_context->brc_constant_data.surface_array_spacing = 0x1;
  brc_init_reset_context->brc_constant_data.tiling = I915_TILING_NONE;
  brc_init_reset_context->brc_constant_data.bo_size = size;
  media_allocate_resource (&brc_init_reset_context->brc_constant_data,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE*) "BRC Constant Data Buffer",
			   size,
			   4096);
  MEDIA_DRV_ASSERT (brc_init_reset_context->brc_constant_data.bo);

  pbuffer =
    (BYTE *) media_map_buffer_obj (brc_init_reset_context->brc_constant_data.bo);
  MEDIA_DRV_ASSERT (pbuffer);

  media_drv_memset (pbuffer, size);

  media_unmap_buffer_obj (brc_init_reset_context->brc_constant_data.bo);

  for (i = 0; i < NUM_BRC_CONSTANT_DATA_BUFFERS; i++) {
    brc_init_reset_context->brc_constant_buffer[i].width = BRC_CONSTANTSURFACE_WIDTH_G75;
    brc_init_reset_context->brc_constant_buffer[i].height = BRC_CONSTANTSURFACE_HEIGHT_G75;
    brc_init_reset_context->brc_constant_buffer[i].surface_array_spacing = 0x1;
    brc_init_reset_context->brc_constant_buffer[i].tiling = I915_TILING_NONE;
    brc_init_reset_context->brc_constant_buffer[i].pitch = BRC_CONSTANTSURFACE_WIDTH_G75;
    media_allocate_resource (&brc_init_reset_context->brc_constant_buffer[i],
			     drv_ctx->drv_data.bufmgr,
			     (const BYTE*)"BRC Constant Data surface",
			     0x1000,
			     4096);
    MEDIA_DRV_ASSERT (brc_init_reset_context->brc_constant_buffer[i].bo);
  }
}

VOID
media_brc_init_reset_context_init(VADriverContextP ctx,
                                  MEDIA_ENCODER_CTX * encoder_context)
{
  BRC_INIT_RESET_CONTEXT *brc_init_reset_context = &encoder_context->brc_init_reset_context;
  MEDIA_GPE_CTX *gpe_context = &brc_init_reset_context->gpe_context;
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 280 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 120;	//121;    //16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE - 1;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels[9], 2);
  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	//* MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_interface_setup_brc_init_reset (encoder_context);
  media_alloc_resource_brc_init_reset (ctx, encoder_context);
  return;
}

VOID
media_alloc_resource_brc_update (VADriverContextP ctx,
                                 MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  BRC_UPDATE_CONTEXT *distortion_ctx = &encoder_context->brc_update_context;
}

VOID
media_brc_update_context_init(VADriverContextP ctx,
                              MEDIA_ENCODER_CTX * encoder_context)
{
  BRC_UPDATE_CONTEXT *distortion_ctx = &encoder_context->brc_update_context;
  MEDIA_GPE_CTX *gpe_context = &distortion_ctx->gpe_context;
  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 280 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 120;	//121;    //16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE - 1;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);
  media_gpe_load_kernels (ctx, gpe_context, &media_hybrid_vp8_kernels[11], 1);
  gpe_context->idrt_size = sizeof (struct media_interface_descriptor_data);	//* MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = 0;

  media_gpe_context_init (ctx, gpe_context);

  encoder_context->brc_distortion_buffer_supported = 1;
  encoder_context->brc_constant_buffer_supported = 1;

  media_interface_setup_brc_update (encoder_context);
  media_alloc_resource_brc_update (ctx, encoder_context);
  return;
}

VOID
media_alloc_resource_scaling (VADriverContextP ctx,
			      MEDIA_ENCODER_CTX * encoder_context)
{
  UINT down_scaled_width4x, downscaled_height_4x;
  UINT down_scaled_width16x, downscaled_height_16x;
  UINT down_scaled_width32x, downscaled_height_32x;
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  SCALING_CONTEXT *scaling_context = &encoder_context->scaling_context;
  down_scaled_width4x = encoder_context->frame_width / 4;
  downscaled_height_4x = encoder_context->frame_height / 4;
  down_scaled_width4x = ALIGN (down_scaled_width4x, 16);
  downscaled_height_4x = (downscaled_height_4x + 1) >> 1;
  downscaled_height_4x = 2 * ALIGN (downscaled_height_4x, 32);

  scaling_context->scaled_4x_surface.width = down_scaled_width4x;
  scaling_context->scaled_4x_surface.height = downscaled_height_4x;
  scaling_context->scaled_4x_surface.surface_array_spacing = 0x1;
  scaling_context->scaled_4x_surface.tiling = I915_TILING_NONE;
  scaling_context->scaled_4x_surface.pitch = 0x80;	//hardcoded..need to fix this later
  media_allocate_resource (&scaling_context->scaled_4x_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "scaled surface 4x", 0x1000, 4096);
  MEDIA_DRV_ASSERT (scaling_context->scaled_4x_surface.bo);

  down_scaled_width16x = encoder_context->frame_width / 16;
  downscaled_height_16x = encoder_context->frame_height / 16;
  down_scaled_width16x = ALIGN (down_scaled_width16x, 16);
  downscaled_height_16x = (downscaled_height_16x + 1) >> 1;
  downscaled_height_16x = 2 * ALIGN (downscaled_height_16x, 32);

  scaling_context->scaled_16x_surface.width = down_scaled_width16x;
  scaling_context->scaled_16x_surface.height = downscaled_height_16x;
  scaling_context->scaled_16x_surface.surface_array_spacing = 0x1;
  scaling_context->scaled_16x_surface.tiling = I915_TILING_NONE;
  scaling_context->scaled_16x_surface.pitch = 0x80;	//hardcoded..need to fix this later
  media_allocate_resource (&scaling_context->scaled_16x_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "scaled surface 16x", 0x1000, 4096);
  MEDIA_DRV_ASSERT (scaling_context->scaled_16x_surface.bo);

  down_scaled_width32x = encoder_context->frame_width / 32;
  downscaled_height_32x = encoder_context->frame_height / 32;
  down_scaled_width32x = ALIGN (down_scaled_width32x, 16);
  downscaled_height_32x = (downscaled_height_32x + 1) >> 1;
  downscaled_height_32x = 2 * ALIGN (downscaled_height_32x, 32);

  scaling_context->scaled_32x_surface.width = down_scaled_width32x;
  scaling_context->scaled_32x_surface.height = downscaled_height_32x;
  scaling_context->scaled_32x_surface.surface_array_spacing = 0x1;
  scaling_context->scaled_32x_surface.tiling = I915_TILING_NONE;
  scaling_context->scaled_32x_surface.pitch = 0x80;	//hardcoded..need to fix this later
  media_allocate_resource (&scaling_context->scaled_32x_surface,
			   drv_ctx->drv_data.bufmgr,
			   (const BYTE *) "scaled surface 32x", 0x1000, 4096);
  MEDIA_DRV_ASSERT (scaling_context->scaled_32x_surface.bo);

}

VOID
media_scaling_context_init (VADriverContextP ctx,
			    MEDIA_ENCODER_CTX * encoder_context)
{
  SCALING_CONTEXT *scaling_context = &encoder_context->scaling_context;
  MEDIA_GPE_CTX *gpe_context = &scaling_context->gpe_context;

  gpe_context->idrt.max_entries = MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe.length = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->vfe_state.max_num_threads = 280 - 1;	//60 - 1;
  gpe_context->vfe_state.num_urb_entries = 16;	//64;
  gpe_context->vfe_state.gpgpu_mode = 0;
  gpe_context->vfe_state.urb_entry_size = 121;	//16;
  gpe_context->vfe_state.curbe_allocation_size = CURBE_ALLOCATION_SIZE - 1;
  gpe_context_vfe_scoreboardinit_vp8 (gpe_context);

  /*media_gpe_load_kernels (ctx, gpe_context, media_hybrid_vp8_kernels, 1); */
#if 0
  gpe_context->surface_state_binding_table.res.bo_size =
    (SURFACE_STATE_PADDED_SIZE + sizeof (UINT)) * MAX_MEDIA_SURFACES_GEN6;
#endif
  gpe_context->idrt_size =
    sizeof (struct media_interface_descriptor_data) * MAX_INTERFACE_DESC_GEN6;
  gpe_context->curbe_size = CURBE_TOTAL_DATA_LENGTH;
  gpe_context->sampler_size = 0;
  media_gpe_context_init (ctx, gpe_context);
  media_alloc_resource_scaling (ctx, encoder_context);
}

void
media_encoder_init_vp8 (VADriverContextP ctx,
			MEDIA_ENCODER_CTX * encoder_context)
{
  MEDIA_DRV_CONTEXT *drv_ctx = (MEDIA_DRV_CONTEXT *) (ctx->pDriverData);
  encoder_context->codec = CODEC_VP8;
  //encoder_context->picture_width = picture_width;
  //encoder_context->picture_height = picture_height;

  if (IS_HASWELL (drv_ctx->drv_data.device_id))
    {
      encoder_context->num_of_kernels =
	sizeof (media_hybrid_vp8_kernels) / sizeof (MEDIA_KERNEL);
      encoder_context->disable_multi_ref = 0;
      media_encoder_context_params_init (drv_ctx, encoder_context);
      media_scaling_context_init (ctx, encoder_context);
      media_me_context_init (ctx, encoder_context);
      media_mbenc_context_init (ctx, encoder_context);
      media_mbpak_context_init_vp8 (ctx, encoder_context);
      media_brc_init_reset_context_init(ctx, encoder_context);
      media_brc_update_context_init(ctx, encoder_context);
      encoder_context->set_curbe_i_vp8_mbenc = media_set_curbe_i_vp8_mbenc;
      encoder_context->set_curbe_p_vp8_mbenc = media_set_curbe_p_vp8_mbenc;
      encoder_context->set_curbe_vp8_mbpak = media_set_curbe_vp8_mbpak;
      encoder_context->surface_state_vp8_mbenc =
	media_surface_state_vp8_mbenc;
      encoder_context->surface_state_vp8_mbpak =
	media_surface_state_vp8_mbpak;
       encoder_context->media_add_surface_state =
        media_add_surface_state;
    }
  else if (IS_GEN7 (drv_ctx->drv_data.device_id))
    {
      encoder_context->num_of_kernels =
	sizeof (media_hybrid_vp8_kernels_g7) / sizeof (MEDIA_KERNEL);
      encoder_context->disable_multi_ref = 1;
      media_encoder_context_params_init (drv_ctx, encoder_context);
      media_scaling_context_init (ctx, encoder_context);
      media_me_context_init (ctx, encoder_context);
      media_mbenc_context_init_g7 (ctx, encoder_context);
      media_mbpak_context_init_vp8_g7 (ctx, encoder_context);
      encoder_context->set_curbe_i_vp8_mbenc = media_set_curbe_i_vp8_mbenc_g7;
      encoder_context->set_curbe_p_vp8_mbenc = media_set_curbe_p_vp8_mbenc_g7;
      encoder_context->set_curbe_vp8_mbpak = media_set_curbe_vp8_mbpak_g7;
      encoder_context->surface_state_vp8_mbenc =
	media_surface_state_vp8_mbenc_g7;
      encoder_context->surface_state_vp8_mbpak =
	media_surface_state_vp8_mbpak_g7;
     encoder_context->media_add_surface_state =
        media_add_surface_state;
   }
  else if (IS_GEN8 (drv_ctx->drv_data.device_id))
    {
      encoder_context->num_of_kernels =
        sizeof (media_hybrid_vp8_kernels_g8) / sizeof (MEDIA_KERNEL);
      encoder_context->disable_multi_ref = 0;
      media_encoder_context_params_init (drv_ctx, encoder_context);
      media_scaling_context_init (ctx, encoder_context);
      media_me_context_init (ctx, encoder_context);
      media_mbenc_context_init_vp8_g8 (ctx, encoder_context);
      media_mbpak_context_init_vp8_g8 (ctx, encoder_context);
      encoder_context->set_curbe_i_vp8_mbenc = media_set_curbe_i_vp8_mbenc;
      encoder_context->set_curbe_p_vp8_mbenc = media_set_curbe_p_vp8_mbenc;
      encoder_context->set_curbe_vp8_mbpak = media_set_curbe_vp8_mbpak;
      encoder_context->surface_state_vp8_mbenc =
        media_surface_state_vp8_mbenc;
      encoder_context->surface_state_vp8_mbpak =
        media_surface_state_vp8_mbpak;
      encoder_context->media_add_surface_state =
        media_add_surface_state_g8;
    }
  else {
   printf("Platform not supported");
   MEDIA_DRV_ASSERT(0);
  }
}
