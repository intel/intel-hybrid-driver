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

#ifndef __INTEL_HOSTVLD_VP9_ENGINE_H__
#define __INTEL_HOSTVLD_VP9_ENGINE_H__

#include "intel_hybrid_hostvld_vp9_internal.h"

#define BYTE_BITS               8
#define BAC_ENG_MAX_RANGE       255
#define BAC_ENG_VALUE_BITS      ( BYTE_BITS * sizeof(INTEL_HOSTVLD_VP9_BAC_VALUE) )
#define BAC_ENG_MASSIVE_BITS    0x40000000
#define BAC_ENG_VALUE_HEAD_RSRV BYTE_BITS
#define BAC_ENG_VALUE_TAIL_RSRV BYTE_BITS
#define BAC_ENG_PROB_BITS       8
#define BAC_ENG_PROB_RANGE      ( 1 << BAC_ENG_PROB_BITS )
#define BAC_ENG_PROB_HALF       ( 1 << (BAC_ENG_PROB_BITS-1) )

extern const UCHAR g_Vp9NormTable[BAC_ENG_MAX_RANGE+1];

INT Intel_HostvldVp9_BacEngineInit(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine,
    PUCHAR                           pBuf,
    DWORD                            dwBufSize);

VOID Intel_HostvldVp9_BacEngineFill(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine);

INT Intel_HostvldVp9_BacEngineReadBit(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine,
    INT                              iProb);

INT Intel_HostvldVp9_BacEngineReadSingleBit(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine);

INT Intel_HostvldVp9_BacEngineReadMultiBits(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine,
    register INT                     iNumBits);

INT Intel_HostvldVp9_BacEngineReadTree(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine,
    INTEL_HOSTVLD_VP9_TKN_TREE    TknTree);

#endif // __INTEL_HOSTVLD_VP9_ENGINE_H__
