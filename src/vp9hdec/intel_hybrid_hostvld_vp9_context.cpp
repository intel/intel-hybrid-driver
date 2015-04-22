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

#include "intel_hybrid_hostvld_vp9_context.h"
#include "intel_hybrid_hostvld_vp9_context_tables.h"
#include "intel_hybrid_hostvld_vp9_engine.h"

#define VP9_DIFF_UPDATE_PROB 252
#define VP9_NMV_UPDATE_PROB  252
#define VP9_SUBEXP_PARAM     4   /* Subexponential code parameter */

#define VP9_COEFF_COUNT_SAT                     24
#define VP9_COEFF_MAX_UPDATE_FACTOR             112
#define VP9_COEFF_COUNT_SAT_KEY                 24
#define VP9_COEFF_MAX_UPDATE_FACTOR_KEY         112
#define VP9_COEFF_COUNT_SAT_AFTER_KEY           24
#define VP9_COEFF_MAX_UPDATE_FACTOR_AFTER_KEY   128

#define VP9_NODE_LEFT                           0
#define VP9_NODE_RIGHT                          1

#define VP9_COUNT_SAT 20
#define VP9_MAX_UPDATE_FACTOR 128

#define INTEL_VP9_RECENTER(v, m)          (((v) > ((m) << 1)) ? (v) : ((v) % 2 ? (m) - (((v) + 1) >> 1) : (m) + ((v) >> 1)))
#define INTEL_VP9_GET_PROB(num, den)      (((den) == 0) ? 128u : INTEL_VP9_CLAMP(((num) * 256 + ((den) >> 1)) / (den), 1, 255))
#define INTEL_VP9_GET_BINARY_PROB(n0, n1) INTEL_VP9_GET_PROB(n0, n0 + n1)

#define INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(pProbs, Count) \
    Intel_HostvldVp9_MergeProb(pProbs, Count, VP9_COUNT_SAT, VP9_MAX_UPDATE_FACTOR)


static const INT g_Vp9InterModeLookup[INTER_MODE_COUNT] = 
{
    PRED_MD_ZEROMV - PRED_MD_NEARESTMV, 
    PRED_MD_NEARESTMV - PRED_MD_NEARESTMV, 
    PRED_MD_NEARMV - PRED_MD_NEARESTMV, 
    PRED_MD_NEWMV - PRED_MD_NEARESTMV
};

/********************************************************************/
/*********************** INTERNAL FUNCTIONS *************************/
/********************************************************************/

static INT Intel_HostvldVp9_GetUnsignedBits(UINT uiNumValues)
{
    INT cat = 0;

    if (uiNumValues <= 1)
    {
        return 0;
    }

    uiNumValues--;
    while (uiNumValues > 0)
    {
        cat++;
        uiNumValues >>= 1;
    }

    return cat;
}

static PROBABILITY Intel_HostvldVp9_InverseMap(INT v, INT m) {
    // The clamp is not necessary for conforming VP9 stream, it is added to
    // prevent out of bound access for bad input data
    v = INTEL_VP9_CLAMP(v, 0, 253);
    v = g_Vp9InverseMapTable[v];
    m--;

    if ((m << 1) <= VP9_MAX_PROB) 
    {
        return 1 + INTEL_VP9_RECENTER(v, m);
    } 
    else 
    {
        return VP9_MAX_PROB - INTEL_VP9_RECENTER(v, VP9_MAX_PROB - 1 - m);
    }
}

static INT Intel_HostvldVp9_DecodeUniform(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine, 
    INT                              n) 
{
    INT v;
    const INT l = Intel_HostvldVp9_GetUnsignedBits(n);
    const INT m = (1 << l) - n;

    if (!l)
    {
        return 0;
    }

    v = INTEL_HOSTVLD_VP9_READ_BITS(l - 1);

    return v < m ?  v : (v << 1) - m + INTEL_HOSTVLD_VP9_READ_ONE_BIT;
}

static INT Intel_HostvldVp9_DecodeSubExponential(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine) 
{
    INT i = 0, mk = 0, iSubExp, iBits, a;

    while (1) 
    {
        iBits = i ? VP9_SUBEXP_PARAM + i - 1 : VP9_SUBEXP_PARAM;
        a = 1 << iBits;

        if (255 <= mk + (3 << iBits)) 
        {
            iSubExp = Intel_HostvldVp9_DecodeUniform(pBacEngine, 255 - mk) + mk;
            break;
        } else {
            if (INTEL_HOSTVLD_VP9_READ_ONE_BIT)
            {
                i++;
                mk += a;
            } 
            else 
            {
                iSubExp = INTEL_HOSTVLD_VP9_READ_BITS(iBits) + mk;
                break;
            }
        }
    }

    return iSubExp;
}

VOID Intel_HostvldVp9_UpdateProb(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine, 
    PPROBABILITY                     pProb) 
{
    BOOL bUpdate = INTEL_HOSTVLD_VP9_READ_BIT(VP9_DIFF_UPDATE_PROB);

    if (bUpdate)
    {
        const int delp = Intel_HostvldVp9_DecodeSubExponential(pBacEngine);
        *pProb = (PROBABILITY)Intel_HostvldVp9_InverseMap(delp, *pProb);
    }
}

VOID Intel_HostvldVp9_UpdateMvProb(
    PINTEL_HOSTVLD_VP9_BAC_ENGINE pBacEngine, 
    PPROBABILITY                     pProb, 
    DWORD                            n) 
{
    DWORD i;

    for (i = 0; i < n; i++)
    {
        if (INTEL_HOSTVLD_VP9_READ_BIT(VP9_NMV_UPDATE_PROB))
        {
            pProb[i] = (INTEL_HOSTVLD_VP9_READ_BITS(7) << 1) | 1;
        }
    }
}

static VOID Intel_HostvldVp9_GetBranchCount(
    UINT BranchCount[VP9_UNCONSTRAINED_NODES][2], 
    UINT CoeffCounts[VP9_UNCONSTRAINED_NODES+1])
{
    BranchCount[2][VP9_NODE_LEFT]   = CoeffCounts[VP9_ONE_TOKEN];
    BranchCount[2][VP9_NODE_RIGHT]  = CoeffCounts[VP9_TWO_TOKEN];
    BranchCount[1][VP9_NODE_LEFT]   = CoeffCounts[VP9_ZERO_TOKEN];
    BranchCount[1][VP9_NODE_RIGHT]  = BranchCount[2][VP9_NODE_LEFT] + BranchCount[2][VP9_NODE_RIGHT];
    BranchCount[0][VP9_NODE_LEFT]   = CoeffCounts[VP9_DCT_EOB_MODEL_TOKEN];
    BranchCount[0][VP9_NODE_RIGHT]  = BranchCount[1][VP9_NODE_LEFT] + BranchCount[1][VP9_NODE_RIGHT];
}

static PROBABILITY Intel_HostvldVp9_MergeProb(
    PROBABILITY PrevProb, 
    UINT        Count[2], 
    UINT        uiCountSat, 
    UINT        uiUpdateFactor)
{
    PROBABILITY Prob     = INTEL_VP9_GET_BINARY_PROB(Count[VP9_NODE_LEFT], Count[VP9_NODE_RIGHT]);
    UINT        uiCount  = MIN(Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT], uiCountSat);
    UINT        uiFactor = uiUpdateFactor * uiCount / uiCountSat;
    return INTEL_VP9_ROUND_POWER_OF_TWO(PrevProb * (256 - uiFactor) + Prob * uiFactor, 8);;
}

static VOID Intel_HostvldVp9_AdaptProbsIntraMode(
    INTEL_HOSTVLD_VP9_TKN_TREE CurrTree, 
    INTEL_HOSTVLD_VP9_TKN_TREE PrevTree, 
    UINT  CurrCounts[INTRA_MODE_COUNT])
{
    UINT    Count[2], RightNodeCount;

    Count[VP9_NODE_RIGHT] = CurrCounts[PRED_MD_D207];
    Count[VP9_NODE_LEFT]  = CurrCounts[PRED_MD_D153];
    CurrTree[16].ui8Prob = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevTree[16].ui8Prob, Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[PRED_MD_D63];
    CurrTree[14].ui8Prob = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevTree[14].ui8Prob, Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[PRED_MD_D45];
    CurrTree[8].ui8Prob = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevTree[8].ui8Prob, Count);
    RightNodeCount = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];

    Count[VP9_NODE_RIGHT] = CurrCounts[PRED_MD_D117];
    Count[VP9_NODE_LEFT]  = CurrCounts[PRED_MD_D135];
    CurrTree[10].ui8Prob = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevTree[10].ui8Prob, Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[PRED_MD_H];
    CurrTree[7].ui8Prob = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevTree[7].ui8Prob, Count);

    Count[VP9_NODE_LEFT]  = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_RIGHT] = RightNodeCount;
    CurrTree[6].ui8Prob = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevTree[6].ui8Prob, Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[PRED_MD_V];
    CurrTree[4].ui8Prob = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevTree[4].ui8Prob, Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[PRED_MD_TM];
    CurrTree[2].ui8Prob = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevTree[2].ui8Prob, Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[PRED_MD_DC];
    CurrTree[0].ui8Prob = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevTree[0].ui8Prob, Count);
}

static VOID Intel_HostvldVp9_AdaptProbsMvClass(
    UINT8     CurrProbs[VP9_MV_CLASSES - 1], 
    UINT8     PrevProbs[VP9_MV_CLASSES - 1], 
    UINT      CurrCounts[VP9_MV_CLASSES])
{
    UINT    Count[2], RightNodeCount;

    Count[VP9_NODE_RIGHT] = CurrCounts[VP9_MV_CLASS_10];
    Count[VP9_NODE_LEFT]  = CurrCounts[VP9_MV_CLASS_9];
    CurrProbs[9] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[9], Count);
    RightNodeCount = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];

    Count[VP9_NODE_RIGHT] = CurrCounts[VP9_MV_CLASS_8];
    Count[VP9_NODE_LEFT]  = CurrCounts[VP9_MV_CLASS_7];
    CurrProbs[8] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[8], Count);

    Count[VP9_NODE_LEFT]  = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_RIGHT] = RightNodeCount;
    CurrProbs[7] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[7], Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[VP9_MV_CLASS_6];
    CurrProbs[6] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[6], Count);
    RightNodeCount = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];

    Count[VP9_NODE_RIGHT] = CurrCounts[VP9_MV_CLASS_5];
    Count[VP9_NODE_LEFT]  = CurrCounts[VP9_MV_CLASS_4];
    CurrProbs[5] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[5], Count);

    Count[VP9_NODE_LEFT]  = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_RIGHT] = RightNodeCount;
    CurrProbs[4] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[4], Count);
    RightNodeCount = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];

    Count[VP9_NODE_RIGHT] = CurrCounts[VP9_MV_CLASS_3];
    Count[VP9_NODE_LEFT]  = CurrCounts[VP9_MV_CLASS_2];
    CurrProbs[3] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[3], Count);

    Count[VP9_NODE_LEFT]  = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_RIGHT] = RightNodeCount;
    CurrProbs[2] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[2], Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[VP9_MV_CLASS_1];
    CurrProbs[1] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[1], Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[VP9_MV_CLASS_0];
    CurrProbs[0] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[0], Count);
}

static VOID Intel_HostvldVp9_AdaptProbs3NodeTreeWithIndex(
    UINT8     CurrProbs[3], 
    UINT8     PrevProbs[3], 
    UINT      CurrCounts[4], 
    const INT Index[4])
{
    UINT    Count[2];

    Count[VP9_NODE_RIGHT] = CurrCounts[Index[3]];
    Count[VP9_NODE_LEFT]  = CurrCounts[Index[2]];
    CurrProbs[2] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[2], Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[Index[1]];
    CurrProbs[1] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[1], Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[Index[0]];
    CurrProbs[0] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[0], Count);
}

static VOID Intel_HostvldVp9_AdaptProbs3NodeTree(
    UINT8 CurrProbs[3], 
    UINT8 PrevProbs[3], 
    UINT  CurrCounts[4])
{
    UINT    Count[2];

    Count[VP9_NODE_RIGHT] = CurrCounts[3];
    Count[VP9_NODE_LEFT]  = CurrCounts[2];
    CurrProbs[2] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[2], Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[1];
    CurrProbs[1] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[1], Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[0];
    CurrProbs[0] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[0], Count);
}

static VOID Intel_HostvldVp9_AdaptProbs2NodeTree(
    UINT8 CurrProbs[2], 
    UINT8 PrevProbs[2], 
    UINT  CurrCounts[3])
{
    UINT    Count[2];

    Count[VP9_NODE_RIGHT] = CurrCounts[2];
    Count[VP9_NODE_LEFT]  = CurrCounts[1];
    CurrProbs[1] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[1], Count);

    Count[VP9_NODE_RIGHT] = Count[VP9_NODE_LEFT] + Count[VP9_NODE_RIGHT];
    Count[VP9_NODE_LEFT]  = CurrCounts[0];
    CurrProbs[0] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(PrevProbs[0], Count);
}

static VAStatus Intel_HostvldVp9_AdaptCoeffProbs(
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pCurrContext, 
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pPrevContext, 
    PINTEL_HOSTVLD_VP9_COUNT         pCount,
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo)
{
    UINT        uiCountSat, uiUpdateFactor;
    INT         i, j, k, l, m, n;
    UINT        BranchCount[VP9_UNCONSTRAINED_NODES][2];
    VAStatus  eStatus     = VA_STATUS_SUCCESS;

    if (pFrameInfo->bIsIntraOnly)
    {
        uiCountSat      = VP9_COEFF_COUNT_SAT_KEY;
        uiUpdateFactor  = VP9_COEFF_MAX_UPDATE_FACTOR_KEY;
    } 
    else if (pFrameInfo->LastFrameType == KEY_FRAME) 
    {
        uiCountSat      = VP9_COEFF_COUNT_SAT_AFTER_KEY;
        uiUpdateFactor  = VP9_COEFF_MAX_UPDATE_FACTOR_AFTER_KEY;
    } 
    else 
    {
        uiCountSat      = VP9_COEFF_COUNT_SAT;
        uiUpdateFactor  = VP9_COEFF_MAX_UPDATE_FACTOR;
    }

    // Coefficient probabilities
    for (n = TX_4X4; n < TX_SIZES; n++)
    {
        for (i = 0; i < INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER; i++)
        {
            for (j = 0; j < REF_TYPES; j++)
            {
                for (k = 0; k < VP9_COEF_BANDS; k++)
                {
                    for (l = 0; l < VP9_PREV_COEF_CONTEXTS; l++)
                    {
                        if (k == 0 && l >= 3)
                        {
                            continue;
                        }

                        Intel_HostvldVp9_GetBranchCount(BranchCount, pCount->CoeffCounts[n][i][j][k][l]);
                        BranchCount[0][1] = pCount->EobBranchCounts[n][i][j][k][l] - BranchCount[0][0];

                        for (m = 0; m < VP9_UNCONSTRAINED_NODES; m++)
                        {
                            pCurrContext->CoeffProbs[n][i][j][k][l][m] = Intel_HostvldVp9_MergeProb(
                                pPrevContext->CoeffProbs[n][i][j][k][l][m],
                                BranchCount[m],
                                uiCountSat, 
                                uiUpdateFactor);
                        }
                    }
                }
            }
        }
    }

finish:
    return eStatus;
}

static VAStatus Intel_HostvldVp9_AdaptModeProbs(
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pCurrContext, 
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pPrevContext, 
    PINTEL_HOSTVLD_VP9_COUNT         pCount,
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo)
{
    INT         i, j;
    VAStatus  eStatus     = VA_STATUS_SUCCESS;

    for (i = 0; i < VP9_INTRA_INTER_CONTEXTS; i++)
    {
        pCurrContext->IntraInterProbs[i] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(
            pPrevContext->IntraInterProbs[i],
            pCount->IntraInterCounts[i]);
    }

    for (i = 0; i < VP9_COMPOUND_INTER_CONTEXTS; i++)
    {
        pCurrContext->CompoundInterProbs[i] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(
            pPrevContext->CompoundInterProbs[i],
            pCount->CompoundInterCounts[i]);
    }

    for (i = 0; i < VP9_REF_CONTEXTS; i++)
    {
        pCurrContext->CompoundRefProbs[i] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(
            pPrevContext->CompoundRefProbs[i],
            pCount->CompoundRefCounts[i]);
    }

    // adapt inter mode contexts
    for (i = 0; i < VP9_REF_CONTEXTS; i++)
    {
        for (j = 0; j < 2; j++)
        {
            pCurrContext->SingleRefProbs[i][j] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(
                pPrevContext->SingleRefProbs[i][j],
                pCount->SingleRefCounts[i][j]);
        }
    }

    for (i = 0; i < VP9_INTER_MODE_CONTEXTS; i++)
    {
        Intel_HostvldVp9_AdaptProbs3NodeTreeWithIndex(
            pCurrContext->InterModeProbs[i], 
            pPrevContext->InterModeProbs[i], 
            pCount->InterModeCounts[i], 
            g_Vp9InterModeLookup);
    }

    for (i = 0; i < VP9_BLK_SIZE_GROUPS; i++)
    {
        Intel_HostvldVp9_AdaptProbsIntraMode(
            pCurrContext->ModeTree_Y[i], 
            pPrevContext->ModeTree_Y[i], 
            pCount->IntraModeCounts_Y[i]);
    }

    for (i = 0; i < INTRA_MODE_COUNT; i++)
    {
        Intel_HostvldVp9_AdaptProbsIntraMode(
            pCurrContext->ModeTree_UV[i], 
            pPrevContext->ModeTree_UV[i], 
            pCount->IntraModeCounts_UV[i]);
    }

    // adapt partition contexts
    for (i = 0; i < VP9_PARTITION_CONTEXTS; i++)
    {
        Intel_HostvldVp9_AdaptProbs3NodeTree(
            pCurrContext->PartitionProbs[i].Prob, 
            pPrevContext->PartitionProbs[i].Prob, 
            pCount->PartitionCounts[i]);
    }

    // adapt switchable interpolation contexts
    if (pFrameInfo->bIsSwitchableInterpolation) 
    {
        for (i = 0; i < VP9_SWITCHABLE_FILTER_CONTEXTS; i++)
        {
            Intel_HostvldVp9_AdaptProbs2NodeTree(
                pCurrContext->SwitchableInterpProbs[i], 
                pPrevContext->SwitchableInterpProbs[i], 
                pCount->SwitchableInterpCounts[i]);
        }
    }

    if (pFrameInfo->TxMode == TX_MODE_SELECT) 
    {
        PUINT   puiTxCounts;
        UINT    uiBranchCounts[3][2];

        for (i = 0; i < VP9_TX_SIZE_CONTEXTS; i++) 
        {
            // 8x8 or less transform
            puiTxCounts = pCount->TxCountSet.Tx_8X8[i];
            uiBranchCounts[0][0] = puiTxCounts[TX_4X4];
            uiBranchCounts[0][1] = puiTxCounts[TX_8X8];
            pCurrContext->TxProbTableSet.Tx_8X8[i][0] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(
                pPrevContext->TxProbTableSet.Tx_8X8[i][0],
                uiBranchCounts[0]);

            // 16x16 transform
            puiTxCounts = pCount->TxCountSet.Tx_16X16[i];
            uiBranchCounts[0][0] = puiTxCounts[TX_4X4];
            uiBranchCounts[0][1] = puiTxCounts[TX_8X8] + puiTxCounts[TX_16X16];
            uiBranchCounts[1][0] = puiTxCounts[TX_8X8];
            uiBranchCounts[1][1] = puiTxCounts[TX_16X16];
            for (j = 0; j < TX_SIZES - 2; j++)
            {
                pCurrContext->TxProbTableSet.Tx_16X16[i][j] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(
                    pPrevContext->TxProbTableSet.Tx_16X16[i][j],
                    uiBranchCounts[j]);
            }

            // 32x32 transform
            puiTxCounts = pCount->TxCountSet.Tx_32X32[i];
            uiBranchCounts[0][0] = puiTxCounts[TX_4X4];
            uiBranchCounts[0][1] = puiTxCounts[TX_8X8] + puiTxCounts[TX_16X16] + puiTxCounts[TX_32X32];
            uiBranchCounts[1][0] = puiTxCounts[TX_8X8];
            uiBranchCounts[1][1] = puiTxCounts[TX_16X16] + puiTxCounts[TX_32X32];
            uiBranchCounts[2][0] = puiTxCounts[TX_16X16];
            uiBranchCounts[2][1] = puiTxCounts[TX_32X32];
            for (j = 0; j < TX_SIZES - 1; j++)
            {
                pCurrContext->TxProbTableSet.Tx_32X32[i][j] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(
                    pPrevContext->TxProbTableSet.Tx_32X32[i][j],
                    uiBranchCounts[j]);
            }
        }
    }

    for (i = 0; i < VP9_MBSKIP_CONTEXTS; i++)
    {
        pCurrContext->MbSkipProbs[i] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(
            pPrevContext->MbSkipProbs[i],
            pCount->MbSkipCounts[i]);
    }

finish:
    return eStatus;
}

static VAStatus Intel_HostvldVp9_AdaptMvProbs(
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pCurrContext, 
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pPrevContext, 
    PINTEL_HOSTVLD_VP9_COUNT         pCount,
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo)
{
    INT         i, j;
    VAStatus  eStatus     = VA_STATUS_SUCCESS;


    Intel_HostvldVp9_AdaptProbs3NodeTree(
        pCurrContext->MvJointProbs, 
        pPrevContext->MvJointProbs, 
        pCount->MvJointCounts);

    for (i = 0; i < VP9_MV_COMPONENTS; i++) 
    {
        PINTEL_HOSTVLD_VP9_MV_PROB_SET  pCurrMvProbs = pCurrContext->MvProbSet + i;
        PINTEL_HOSTVLD_VP9_MV_PROB_SET  pPrevMvProbs = pPrevContext->MvProbSet + i;
        PINTEL_HOSTVLD_VP9_MV_COUNT_SET pMvCounts = pCount->MvCountSet + i;

        pCurrMvProbs->MvSignProbs = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(
            pPrevMvProbs->MvSignProbs,
            pMvCounts->MvSignCounts);

        Intel_HostvldVp9_AdaptProbsMvClass(
            pCurrMvProbs->MvClassProbs, 
            pPrevMvProbs->MvClassProbs, 
            pMvCounts->MvClassCounts);

        pCurrMvProbs->MvClass0Probs[0] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(
            pPrevMvProbs->MvClass0Probs[0], 
            pMvCounts->MvClass0Counts);

        for (j = 0; j < VP9_MV_OFFSET_BITS; j++)
        {
            pCurrMvProbs->MvBitsProbs[j] = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(
                pPrevMvProbs->MvBitsProbs[j],
                pMvCounts->MvBitsCounts[j]);
        }

        for (j = 0; j < VP9_MV_CLASS0_SIZE; j++)
        {
            Intel_HostvldVp9_AdaptProbs3NodeTree(
                pCurrMvProbs->MvClass0FpProbs[j], 
                pPrevMvProbs->MvClass0FpProbs[j], 
                pMvCounts->MvClass0FpCounts[j]);
        }

        Intel_HostvldVp9_AdaptProbs3NodeTree(
            pCurrMvProbs->MvFpProbs, 
            pPrevMvProbs->MvFpProbs, 
            pMvCounts->MvFpCounts);

        if (pFrameInfo->bAllowHighPrecisionMv) 
        {
            pCurrMvProbs->MvClass0HpProbs = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(
                pPrevMvProbs->MvClass0HpProbs, 
                pMvCounts->MvClass0HpCounts);

            pCurrMvProbs->MvHpProbs = INTEL_HOSTVLD_VP9_MERGE_PROB_MAX(
                pPrevMvProbs->MvHpProbs, 
                pMvCounts->MvHpCounts);
        }
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_InitializeProbabilities(
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext)
{
    size_t      Size;
    VAStatus  eStatus = VA_STATUS_SUCCESS;


    Size = sizeof(pContext->TxProbTableSet);
    memcpy(
        &pContext->TxProbTableSet, 
        &g_Vp9DefaultTxProbs,
        Size);
    Size = VP9_MBSKIP_CONTEXTS * sizeof(pContext->MbSkipProbs[0]);
    memcpy(
        pContext->MbSkipProbs, 
        g_Vp9DefaultMbSkipProbs,
        Size);
    Size = VP9_PARTITION_CONTEXTS * sizeof(pContext->PartitionProbs[0]);
    memcpy(
        pContext->PartitionProbs, 
        g_Vp9DefaultPartitionProbs,
        Size);

    // inter probs
    Size = VP9_BLK_SIZE_GROUPS * sizeof(pContext->ModeTree_Y[0]);
    memcpy(
        pContext->ModeTree_Y, 
        g_Vp9DefaultIntraInInterProbTreeY,
        Size);
    Size = INTRA_MODE_COUNT * sizeof(pContext->ModeTree_UV[0]);
    memcpy(
        pContext->ModeTree_UV, 
        g_Vp9DefaultIntraInInterProbTreeUV,
        Size);
    Size = VP9_INTER_MODE_CONTEXTS * (INTER_MODE_COUNT - 1) * 
        sizeof(pContext->InterModeProbs[0][0]);
    memcpy(
        pContext->InterModeProbs, 
        g_Vp9DefaultInterModeProbs,
        Size);
    Size = VP9_SWITCHABLE_FILTER_CONTEXTS * (VP9_SWITCHABLE_FILTERS - 1) * 
        sizeof(pContext->SwitchableInterpProbs[0][0]);
    memcpy(
        pContext->SwitchableInterpProbs, 
        g_Vp9DefaultSwitchableInterpProbs,
        Size);
    Size = VP9_INTRA_INTER_CONTEXTS * sizeof(pContext->IntraInterProbs[0]);
    memcpy(
        &pContext->IntraInterProbs[0], 
        g_Vp9DefaultIntraInterProbs,
        Size);
    Size = VP9_COMPOUND_INTER_CONTEXTS * sizeof(pContext->CompoundInterProbs[0]);
    memcpy(
        &pContext->CompoundInterProbs[0], 
        g_Vp9DefaultCompoundInterProbs,
        Size);
    Size = VP9_REF_CONTEXTS * 2 * sizeof(pContext->SingleRefProbs[0][0]);
    memcpy(
        pContext->SingleRefProbs, 
        g_Vp9DefaultSingleRefProbs,
        Size);
    Size = VP9_REF_CONTEXTS * sizeof(pContext->CompoundRefProbs[0]);
    memcpy(
        &pContext->CompoundRefProbs[0], 
        g_Vp9DefaultCompoundRefProbs,
        Size);

    // mv probs
    Size = (VP9_MV_JOINTS - 1) * sizeof(pContext->MvJointProbs[0]);
    memcpy(
        &pContext->MvJointProbs[0], 
        g_Vp9DefaultMvJointProbs,
        Size);
    Size = VP9_MV_COMPONENTS * sizeof(pContext->MvProbSet[0]);
    memcpy(
        &pContext->MvProbSet[0], 
        g_Vp9DefaultMvProbSet,
        Size);

    // coefficient probs
    Size = TX_SIZES * INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER * 
        REF_TYPES * VP9_COEF_BANDS * VP9_PREV_COEF_CONTEXTS * VP9_UNCONSTRAINED_NODES * 
        sizeof(pContext->CoeffProbs[0][0][0][0][0][0]);
    memcpy(
        pContext->CoeffProbs, 
        g_Vp9DefaultCoeffProbs,
        Size);

finish:
    return eStatus;
}

/******************************************************************/
/*********************** PUBLIC FUNCTIONS *************************/
/******************************************************************/

VAStatus Intel_HostvldVp9_GetCurrFrameContext(
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT  pCtxTable,
    PINTEL_HOSTVLD_VP9_FRAME_INFO     pFrameInfo)
{
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext;
    VAStatus  eStatus     = VA_STATUS_SUCCESS;

    if (pFrameInfo->uiFrameContextIndex >= 4)
    {
        eStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        goto finish;
    }
    memcpy(
        pFrameInfo->pContext,
        pCtxTable + pFrameInfo->uiFrameContextIndex,
        sizeof(*(pCtxTable + pFrameInfo->uiFrameContextIndex)));

    //reset TxProbTables pointers.
    pContext                                        = pFrameInfo->pContext;
    pContext->TxProbTables[TX_8X8].pui8ProbTable    = &pContext->TxProbTableSet.Tx_8X8[0][0];
    pContext->TxProbTables[TX_8X8].uiStride         = TX_8X8;
    pContext->TxProbTables[TX_16X16].pui8ProbTable  = &pContext->TxProbTableSet.Tx_16X16[0][0];
    pContext->TxProbTables[TX_16X16].uiStride       = TX_16X16;
    pContext->TxProbTables[TX_32X32].pui8ProbTable  = &pContext->TxProbTableSet.Tx_32X32[0][0];
    pContext->TxProbTables[TX_32X32].uiStride       = TX_32X32;

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_ResetContext(
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT  pCtxTable,
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo)
{
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext;
    BOOL       bResetAll, bResetSpecified;
    VAStatus eStatus = VA_STATUS_SUCCESS;


    bResetAll =
        (pFrameInfo->bIsKeyFrame || 
         pFrameInfo->bErrorResilientMode || 
         (pFrameInfo->uiResetFrameContext == 3));

    bResetSpecified = (pFrameInfo->uiResetFrameContext == 2);

    pFrameInfo->bResetContext = (bResetAll || bResetSpecified);

    if (bResetAll)
    {
            Intel_HostvldVp9_InitializeProbabilities(
            pFrameInfo->pContext);
        //all 4 context tables updating will be done in postparser thread
    }
    else if (bResetSpecified)
    {
        if (pFrameInfo->uiFrameContextIndex >= 4)
        {
            eStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
            goto finish;
        }
            Intel_HostvldVp9_InitializeProbabilities(
            pCtxTable + pFrameInfo->uiFrameContextIndex);

        memcpy(
            pFrameInfo->pContext,
            pCtxTable,
            sizeof(*pCtxTable));
    }

    //reset TxProbTables pointers.
    pContext                                        = pFrameInfo->pContext;
    pContext->TxProbTables[TX_8X8].pui8ProbTable    = &pContext->TxProbTableSet.Tx_8X8[0][0];
    pContext->TxProbTables[TX_8X8].uiStride         = TX_8X8;
    pContext->TxProbTables[TX_16X16].pui8ProbTable  = &pContext->TxProbTableSet.Tx_16X16[0][0];
    pContext->TxProbTables[TX_16X16].uiStride       = TX_16X16;
    pContext->TxProbTables[TX_32X32].pui8ProbTable  = &pContext->TxProbTableSet.Tx_32X32[0][0];
    pContext->TxProbTables[TX_32X32].uiStride       = TX_32X32;

    pFrameInfo->uiFrameContextIndex = 0;

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_UpdateContextTables(
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT  pCtxTable,
    PINTEL_HOSTVLD_VP9_FRAME_INFO     pFrameInfo)
{
    VAStatus eStatus = VA_STATUS_SUCCESS;

    if (pFrameInfo->bIsKeyFrame ||
         pFrameInfo->bErrorResilientMode ||
         (pFrameInfo->uiResetFrameContext == 3))
    {
        INT i;
            Intel_HostvldVp9_InitializeProbabilities(
            pCtxTable);

        for (i = 1; i < 4; i++)
        {
            memcpy(
                pCtxTable + i,
                pCtxTable,
                sizeof(*pCtxTable));
        }
    }

    return eStatus;
}

VAStatus Intel_HostvldVp9_SetupSegmentationProbs(
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext,
    PUCHAR                              pSegTreeProb,
    PUCHAR                              pSegPredProb)
{
    size_t      Size;
    VAStatus  eStatus = VA_STATUS_SUCCESS;

    INTEL_HOSTVLD_VP9_SEGMENT_TREE SegTree = INTEL_HOSTVLD_VP9_SEGMENT_PROB_TREE(
        pSegTreeProb[0],
        pSegTreeProb[1],
        pSegTreeProb[2],
        pSegTreeProb[3],
        pSegTreeProb[4],
        pSegTreeProb[5],
        pSegTreeProb[6]);

    Size = sizeof(pContext->SegmentTree);
    memcpy(
        &pContext->SegmentTree,
        &SegTree,
        Size);
    Size = sizeof(pContext->SegPredProbs);
    memcpy(
        pContext->SegPredProbs,
        pSegPredProb,
        Size);

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_ReadProbabilitiesInter(
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext, 
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo, 
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine)
{
    PINTEL_VP9_PIC_PARAMS    pPicParams;
    INT                         i, j;
    PBOOL                       pbRefFrameSignBias;
    DWORD                       dwPredictionMode;
    VAStatus                  eStatus     = VA_STATUS_SUCCESS;


    pPicParams          = pFrameInfo->pPicParams;
    pbRefFrameSignBias  = pFrameInfo->RefFrameSignBias;

    // inter mode probs
    for (i = 0; i < VP9_INTER_MODE_CONTEXTS; i++)
    {
        for (j = 0; j < INTER_MODE_COUNT - 1; j++)
        {
            Intel_HostvldVp9_UpdateProb(pBacEngine, pContext->InterModeProbs[i] + j);
        }
    }

    if (pFrameInfo->bIsSwitchableInterpolation)
    {
        // switchable interpolation probs
        for (i = 0; i < VP9_SWITCHABLE_FILTER_CONTEXTS; i++)
        {
            for (j = 0; j < VP9_SWITCHABLE_FILTERS - 1; j++)
            {
                Intel_HostvldVp9_UpdateProb(pBacEngine, pContext->SwitchableInterpProbs[i] + j);
            }
        }
    }

    // intra inter probs
    for (i = 0; i < VP9_INTRA_INTER_CONTEXTS; ++i)
    {
        Intel_HostvldVp9_UpdateProb(pBacEngine, pContext->IntraInterProbs + i);
    }

    // compound prediction probs
    dwPredictionMode = VP9_SINGLE_PREDICTION_ONLY;
    if ((pbRefFrameSignBias[VP9_REF_FRAME_GOLDEN] != pbRefFrameSignBias[VP9_REF_FRAME_LAST]) || 
        (pbRefFrameSignBias[VP9_REF_FRAME_ALTREF] != pbRefFrameSignBias[VP9_REF_FRAME_LAST]))
    {
        // compound allowed, read prediction mode
        if (INTEL_HOSTVLD_VP9_READ_ONE_BIT)
        {
            dwPredictionMode = INTEL_HOSTVLD_VP9_READ_ONE_BIT ? VP9_HYBRID_PREDICTION : VP9_COMPOUND_PREDICTION_ONLY;
        }

        // setup frame level compound prediction reference frame
        if (pbRefFrameSignBias[VP9_REF_FRAME_LAST] == pbRefFrameSignBias[VP9_REF_FRAME_GOLDEN]) 
        {
                pFrameInfo->CompondFixedRef = VP9_REF_FRAME_ALTREF;
                pFrameInfo->CompondVarRef[0] = VP9_REF_FRAME_LAST;
                pFrameInfo->CompondVarRef[1] = VP9_REF_FRAME_GOLDEN;
        } 
        else if (pbRefFrameSignBias[VP9_REF_FRAME_LAST] == pbRefFrameSignBias[VP9_REF_FRAME_ALTREF]) 
        {
                pFrameInfo->CompondFixedRef = VP9_REF_FRAME_GOLDEN;
                pFrameInfo->CompondVarRef[0] = VP9_REF_FRAME_LAST;
                pFrameInfo->CompondVarRef[1] = VP9_REF_FRAME_ALTREF;
        } 
        else 
        {
            pFrameInfo->CompondFixedRef = VP9_REF_FRAME_LAST;
            pFrameInfo->CompondVarRef[0] = VP9_REF_FRAME_GOLDEN;
            pFrameInfo->CompondVarRef[1] = VP9_REF_FRAME_ALTREF;
        }
    }

    pFrameInfo->dwPredictionMode = dwPredictionMode;

    // compound inter probs
    if (dwPredictionMode == VP9_HYBRID_PREDICTION)
    {
        for (i = 0; i < VP9_COMPOUND_INTER_CONTEXTS; i++)
        {
            Intel_HostvldVp9_UpdateProb(pBacEngine, pContext->CompoundInterProbs + i);
        }
    }

    // single reference probs
    if (dwPredictionMode != VP9_COMPOUND_PREDICTION_ONLY)
    {
        for (i = 0; i < VP9_REF_CONTEXTS; i++)
        {
            Intel_HostvldVp9_UpdateProb(pBacEngine, pContext->SingleRefProbs[i]);
            Intel_HostvldVp9_UpdateProb(pBacEngine, pContext->SingleRefProbs[i] + 1);
        }
    }

    // compound reference probs
    if (dwPredictionMode != VP9_SINGLE_PREDICTION_ONLY)
    {
        for (i = 0; i < VP9_REF_CONTEXTS; i++)
        {
            Intel_HostvldVp9_UpdateProb(pBacEngine, pContext->CompoundRefProbs + i);
        }
    }

    // Luma intra mode probs
    for (i = 0; i < VP9_BLK_SIZE_GROUPS; i++)
    {
        Intel_HostvldVp9_UpdateProb(pBacEngine, &(pContext->ModeTree_Y[i][0].ui8Prob));
        Intel_HostvldVp9_UpdateProb(pBacEngine, &(pContext->ModeTree_Y[i][2].ui8Prob));
        Intel_HostvldVp9_UpdateProb(pBacEngine, &(pContext->ModeTree_Y[i][4].ui8Prob));
        Intel_HostvldVp9_UpdateProb(pBacEngine, &(pContext->ModeTree_Y[i][6].ui8Prob));
        Intel_HostvldVp9_UpdateProb(pBacEngine, &(pContext->ModeTree_Y[i][7].ui8Prob));
        Intel_HostvldVp9_UpdateProb(pBacEngine, &(pContext->ModeTree_Y[i][10].ui8Prob));
        Intel_HostvldVp9_UpdateProb(pBacEngine, &(pContext->ModeTree_Y[i][8].ui8Prob));
        Intel_HostvldVp9_UpdateProb(pBacEngine, &(pContext->ModeTree_Y[i][14].ui8Prob));
        Intel_HostvldVp9_UpdateProb(pBacEngine, &(pContext->ModeTree_Y[i][16].ui8Prob));
    }

    // partition probs
    for (i = 0; i < VP9_PARTITION_CONTEXTS; i++)
    {
        for (j = 0; j < PARTITION_TYPES - 1; j++)
        {
            Intel_HostvldVp9_UpdateProb(pBacEngine, pContext->PartitionProbs[i].Prob + j);
        }
    }

    // MV probs
    Intel_HostvldVp9_UpdateMvProb(pBacEngine, pContext->MvJointProbs, VP9_MV_JOINTS - 1);

    for (i = 0; i < VP9_MV_COMPONENTS; i++) 
    {
        PINTEL_HOSTVLD_VP9_MV_PROB_SET pMvProbSet = pContext->MvProbSet + i;

        if (INTEL_HOSTVLD_VP9_READ_BIT(VP9_NMV_UPDATE_PROB))
        {
            pMvProbSet->MvSignProbs = (UINT8)(INTEL_HOSTVLD_VP9_READ_BITS(7) << 1) | 1;
        }

        Intel_HostvldVp9_UpdateMvProb(pBacEngine, pMvProbSet->MvClassProbs, VP9_MV_CLASSES - 1);
        Intel_HostvldVp9_UpdateMvProb(pBacEngine, pMvProbSet->MvClass0Probs, VP9_MV_CLASS0_SIZE - 1);
        Intel_HostvldVp9_UpdateMvProb(pBacEngine, pMvProbSet->MvBitsProbs, VP9_MV_OFFSET_BITS);
    }

    for (i = 0; i < VP9_MV_COMPONENTS; i++) 
    {
        PINTEL_HOSTVLD_VP9_MV_PROB_SET pMvProbSet = pContext->MvProbSet + i;

        for (j = 0; j < VP9_MV_CLASS0_SIZE; ++j)
        {
            Intel_HostvldVp9_UpdateMvProb(pBacEngine, pMvProbSet->MvClass0FpProbs[j], 3);
        }

        Intel_HostvldVp9_UpdateMvProb(pBacEngine, pMvProbSet->MvFpProbs, 3);
    }

    if (pFrameInfo->bAllowHighPrecisionMv) 
    {
        for (i = 0; i < VP9_MV_COMPONENTS; i++) 
        {
            PINTEL_HOSTVLD_VP9_MV_PROB_SET pMvProbSet = pContext->MvProbSet + i;

            if (INTEL_HOSTVLD_VP9_READ_BIT(VP9_NMV_UPDATE_PROB))
            {
                pMvProbSet->MvClass0HpProbs = (UINT8)(INTEL_HOSTVLD_VP9_READ_BITS(7) << 1) | 1;
            }

            if (INTEL_HOSTVLD_VP9_READ_BIT(VP9_NMV_UPDATE_PROB))
            {
                pMvProbSet->MvHpProbs = (UINT8)(INTEL_HOSTVLD_VP9_READ_BITS(7) << 1) | 1;
            }
        }
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_ReadProbabilities(
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pContext, 
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo, 
    PINTEL_HOSTVLD_VP9_BAC_ENGINE    pBacEngine)
{
    PINTEL_VP9_PIC_PARAMS    pPicParams;
    INT                         i, j, k, l, m, n;
    VAStatus                  eStatus     = VA_STATUS_SUCCESS;


    pPicParams  = pFrameInfo->pPicParams;

    // Read probabilities
    if (pFrameInfo->TxMode == TX_MODE_SELECT)
    {
        // transform probabilities
        PINTEL_HOSTVLD_VP9_TX_PROB_TABLE_SET pTxProbs = &pContext->TxProbTableSet;

        // 8x8
        for (i = 0; i < VP9_TX_SIZE_CONTEXTS; i++)
        {
            for (j = 0; j < TX_SIZES - 3; j++)
            {
                Intel_HostvldVp9_UpdateProb(pBacEngine, pTxProbs->Tx_8X8[i] + j);
            }
        }
        // 16x16
        for (i = 0; i < VP9_TX_SIZE_CONTEXTS; i++)
        {
            for (j = 0; j < TX_SIZES - 2; ++j)
            {
                Intel_HostvldVp9_UpdateProb(pBacEngine, pTxProbs->Tx_16X16[i] + j);
            }
        }
        // 32x32
        for (i = 0; i < VP9_TX_SIZE_CONTEXTS; i++)
        {
            for (j = 0; j < TX_SIZES - 1; ++j)
            {
                Intel_HostvldVp9_UpdateProb(pBacEngine, pTxProbs->Tx_32X32[i] + j);
            }
        }
    }

    // Coefficient probabilities
    for (n = TX_4X4; n <= g_Vp9TxMode2MaxTxSize[pFrameInfo->TxMode]; n++)
    {
        if (INTEL_HOSTVLD_VP9_READ_ONE_BIT)
        {
            for (i = 0; i < INTEL_HOSTVLD_VP9_YUV_PLANE_NUMBER; i++)
            {
                for (j = 0; j < REF_TYPES; j++)
                {
                    for (k = 0; k < VP9_COEF_BANDS; k++)
                    {
                        for (l = 0; l < VP9_PREV_COEF_CONTEXTS; l++)
                        {
                            if (k == 0 && l >= 3)
                            {
                                continue;
                            }

                            for (m = 0; m < VP9_UNCONSTRAINED_NODES; m++)
                            {
                                Intel_HostvldVp9_UpdateProb(pBacEngine, &pContext->CoeffProbs[n][i][j][k][l][m]);
                            }
                        }
                    }
                }
            }
        }
    }

    // Skip Flag probabilities
    for (i = 0; i < VP9_MBSKIP_CONTEXTS; i++)
    {
        Intel_HostvldVp9_UpdateProb(pBacEngine, &pContext->MbSkipProbs[i]);
    }

    if (!pFrameInfo->bIsIntraOnly)
    {
        Intel_HostvldVp9_ReadProbabilitiesInter(
            pContext, pFrameInfo, pBacEngine);
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_AdaptProbabilities(
    PINTEL_HOSTVLD_VP9_FRAME_STATE   pFrameState)
{
    PINTEL_HOSTVLD_VP9_FRAME_INFO    pFrameInfo;
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT pCurrContext, pPrevContext;
    PINTEL_HOSTVLD_VP9_COUNT         pCount;
    VAStatus                  eStatus     = VA_STATUS_SUCCESS;

    pFrameInfo      = &pFrameState->FrameInfo;
    pCurrContext    = pFrameInfo->pContext;
    pPrevContext    = &(pFrameState->pVp9HostVld->ContextTable[pFrameInfo->uiFrameContextIndex]);
    pCount          = &pFrameState->pTileStateBase->Count;

    if (!pFrameInfo->bErrorResilientMode && pFrameInfo->bFrameParallelDisabled)
    {
        Intel_HostvldVp9_AdaptCoeffProbs(
            pCurrContext, pPrevContext, pCount, pFrameInfo);

        if (!pFrameInfo->bIsIntraOnly)
        {
            Intel_HostvldVp9_AdaptModeProbs(
                pCurrContext, pPrevContext, pCount, pFrameInfo);
            Intel_HostvldVp9_AdaptMvProbs(
                pCurrContext, pPrevContext, pCount, pFrameInfo);
        }
    }

finish:
    return eStatus;
}

VAStatus Intel_HostvldVp9_RefreshFrameContext(
    PINTEL_HOSTVLD_VP9_FRAME_CONTEXT  pCtxTable,
    PINTEL_HOSTVLD_VP9_FRAME_INFO     pFrameInfo)
{
    VAStatus                  eStatus     = VA_STATUS_SUCCESS;


    if (pFrameInfo->pPicParams->PicFlags.fields.refresh_frame_context)
    {
        if (pFrameInfo->uiFrameContextIndex >= 4)
        {
            eStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
            goto finish;
        }
        memcpy(
            pCtxTable + pFrameInfo->uiFrameContextIndex,
            pFrameInfo->pContext, 
            sizeof(*pFrameInfo->pContext));
    }

finish:
    return eStatus;
}

