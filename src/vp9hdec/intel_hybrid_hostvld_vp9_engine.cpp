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

/*
 * Copyright (c) 2010, The WebM Project authors. All rights reserved.
 *
 * An additional intellectual property rights grant can be found
 * in the file LIBVPX_PATENTS.  All contributing project authors may
 * be found in the LIBVPX_AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.

 * Neither the name of Google, nor the WebM Project, nor the names
 * of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "intel_hybrid_hostvld_vp9_engine.h"

const UCHAR g_Vp9NormTable[BAC_ENG_MAX_RANGE+1] =
{
    0, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

VOID Intel_HostvldVp9_BacEngineFill(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine)
{
    INTEL_HOSTVLD_VP9_BAC_VALUE iRegOp;
    INT iShift = BAC_ENG_VALUE_BITS - BAC_ENG_VALUE_TAIL_RSRV - pBacEngine->iCount;
    register INT iBitsLeft = (INT)(pBacEngine->pBufEnd - pBacEngine->pBuf) * BYTE_BITS;
    INT iMargin   = iShift + BAC_ENG_VALUE_TAIL_RSRV - iBitsLeft;
    INT iLoopEnd  = 0;
    uint8_t *buffer;
    register INTEL_HOSTVLD_VP9_BAC_VALUE BacValue;

    if (iMargin >= 0)
    {
        pBacEngine->iCount += BAC_ENG_MASSIVE_BITS;
        iLoopEnd = iMargin;
    }

    buffer = pBacEngine->pBuf;
    BacValue = pBacEngine->BacValue;
    if (iMargin < 0 || iBitsLeft)
    {

        while (iShift >= iLoopEnd) {
            pBacEngine->iCount += BYTE_BITS;
            iRegOp = *buffer++;
            BacValue |= (iRegOp << iShift);
            iShift -= BYTE_BITS;
        }
        pBacEngine->BacValue = BacValue;
        pBacEngine->pBuf = buffer;
    }
}

INT Intel_HostvldVp9_BacEngineInit(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine,
    PUCHAR                           pBuf,
    DWORD                            dwBufSize)
{
    pBacEngine->pBuf     = pBuf;
    pBacEngine->pBufEnd  = pBuf + dwBufSize;
    pBacEngine->BacValue = 0;
    pBacEngine->iCount   = 0;
    pBacEngine->uiRange  = BAC_ENG_MAX_RANGE;

    Intel_HostvldVp9_BacEngineFill(pBacEngine);
    return Intel_HostvldVp9_BacEngineReadSingleBit(pBacEngine);
}

INT Intel_HostvldVp9_BacEngineReadBit(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine,
    INT                              iProb)
{
    INTEL_HOSTVLD_VP9_BAC_VALUE BacValue;
    INT  iCount;
    INT  iBit = 0;
    UINT uiSplit = ( (pBacEngine->uiRange * iProb) + (BAC_ENG_PROB_RANGE - iProb) ) >> BAC_ENG_PROB_BITS;
    register UINT uiRange = uiSplit;
    INTEL_HOSTVLD_VP9_BAC_VALUE BacSplitValue = (INTEL_HOSTVLD_VP9_BAC_VALUE)uiSplit << (BAC_ENG_VALUE_BITS - BAC_ENG_PROB_BITS);

    if (pBacEngine->iCount < BAC_ENG_VALUE_HEAD_RSRV)
    {
        Intel_HostvldVp9_BacEngineFill(pBacEngine);
    }
    BacValue = pBacEngine->BacValue;
    iCount   = pBacEngine->iCount;

    if (BacValue >= BacSplitValue)
    {
        uiRange  = pBacEngine->uiRange - uiSplit;
        BacValue = BacValue - BacSplitValue;
        iBit     = 1;
    }

    {
        register UINT uiShift = g_Vp9NormTable[uiRange];
        uiRange  <<= uiShift;
        BacValue <<= uiShift;
        iCount -= uiShift;
    }

    pBacEngine->BacValue = BacValue;
    pBacEngine->iCount   = iCount;
    pBacEngine->uiRange  = uiRange;

    return iBit;
}

INT Intel_HostvldVp9_BacEngineReadSingleBit(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine)
{
    return Intel_HostvldVp9_BacEngineReadBit(pBacEngine, BAC_ENG_PROB_HALF);
}

INT Intel_HostvldVp9_BacEngineReadMultiBits(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine,
    register INT                    iNumBits)
{
    register INT iBits = 0;

    while((iNumBits--) > 0)
    {
        iBits ^= (Intel_HostvldVp9_BacEngineReadSingleBit(pBacEngine) << iNumBits);
    }

    return iBits;
}

INT Intel_HostvldVp9_BacEngineReadTree(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine,
    INTEL_HOSTVLD_VP9_TKN_TREE    TknTree)
{
    register PINTEL_HOSTVLD_VP9_TKN_TREE_NODE pNode = TknTree;
    register INT8 i8Offset = pNode->i8Offset;

    do
    {
        pNode += Intel_HostvldVp9_BacEngineReadBit(pBacEngine, pNode->ui8Prob) - i8Offset;
    } while((i8Offset = pNode->i8Offset) < 0);

    return pNode->i8Token;
}
