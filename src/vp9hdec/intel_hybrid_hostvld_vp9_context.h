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

#ifndef __INTEL_HOSTVLD_VP9_CONTEXT_H__
#define __INTEL_HOSTVLD_VP9_CONTEXT_H__

#include "intel_hybrid_common_vp9.h"
#include "intel_hybrid_hostvld_vp9_internal.h"

typedef UINT8 PROBABILITY, *PPROBABILITY;

VAStatus Intel_HostvldVp9_GetCurrFrameContext(
    PINTEL_HOSTVLD_VP9_CONTEXT       pContext, 
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo);

VAStatus Intel_HostvldVp9_ResetContext(
    PINTEL_HOSTVLD_VP9_CONTEXT       pContext, 
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo);

VAStatus Intel_HostvldVp9_SetupSegmentationProbs(
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext,
    PUCHAR                              pSegTreeProb,
    PUCHAR                              pSegPredProb);

VAStatus Intel_HostvldVp9_ReadProbabilities(
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext, 
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo, 
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine);

VAStatus Intel_HostvldVp9_AdaptProbabilities(
    PINTEL_HOSTVLD_VP9_CONTEXT       pContext, 
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo);

VAStatus Intel_HostvldVp9_RefreshFrameContext(
    PINTEL_HOSTVLD_VP9_CONTEXT       pContext, 
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo);

#endif // __INTEL_HOSTVLD_VP9_CONTEXT_H__
