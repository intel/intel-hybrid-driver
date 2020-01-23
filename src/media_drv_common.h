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
 *    Midhunchandra Kodiyath <midhunchandra.kodiyath@intel.com>
 *
 */

#ifndef _MEDIA__DRIVER_COMMON_H
#define _MEDIA__DRIVER_COMMON_H

// BRC Flag in BRC Init Kernel
#define BRC_INIT_CBR                            0x0010
#define BRC_INIT_VBR                            0x0020
#define BRC_INIT_AVBR                           0x0040
#define BRC_INIT_FIELD_PIC                      0x0100
#define BRC_INIT_ICQ                            0x0200
#define BRC_INIT_VCM                            0x0400
#define BRC_INIT_IGNORE_PICTURE_HEADER_SIZE     0x2000
#define BRC_INIT_DISABLE_MBBRC                  0x8000

extern UINT SEARCH_PATH_TABLE[2][8][16];
extern UINT ME_CURBE_INIT_DATA[30];
#endif
