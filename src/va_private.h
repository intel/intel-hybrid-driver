/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2009-2012
Intel Corporation All Rights Reserved.

The source code contained or described herein and all documents related to the
source code ("Material") are owned by Intel Corporation or its suppliers or
licensors. Title to the Material remains with Intel Corporation or its suppliers
and licensors. The Material contains trade secrets and proprietary and confidential
information of Intel or its suppliers and licensors. The Material is protected by
worldwide copyright and trade secret laws and treaty provisions. No part of the
Material may be used, copied, reproduced, modified, published, uploaded, posted,
transmitted, distributed, or disclosed in any way without Intel’s prior express
written permission.

No license under any patent, copyright, trade secret or other intellectual
property right is granted to or conferred upon you by disclosure or delivery
of the Materials, either expressly, by implication, inducement, estoppel
or otherwise. Any license under such intellectual property rights must be
express and approved by Intel in writing.

File Name: va_private.h
Abstract: libva private API head file

Environment: Linux/Android

Notes:

======================= end_copyright_notice ==================================*/
#ifndef __VA_PRIVATE_H__
#define __VA_PRIVATE_H__

// Private Hybrid Enc Entrypoint
#define VAEntrypointHybridEncSlice      -1
// Misc parameter for encoder
#define  VAEncMiscParameterTypePrivate     -2
// encryption parameters for PAVP
#define VAEncryptionParameterBufferType -3 
// Macroblock data buffer type
#define VAEncMbDataBufferType	        -4

#define EXTERNAL_ENCRYPTED_SURFACE_FLAG   (1<<16)

// Inform driver not to do any aligment for the buffer
#define BUFFER_INFO_BYPASS_FLAG           (1<<15)

//HDCP status report buffer flag
#define HDCP2_CODED_BUF_FLAG               (1<<8)

//0xf is reserved for WiDi
#define WIDI_SESSION_ID					   (0xf)

// LE<->BE translation of DWORD
#define SwapEndian_DW(dw)    ( (((dw) & 0x000000ff) << 24) |  (((dw) & 0x0000ff00) << 8) | (((dw) & 0x00ff0000) >> 8) | (((dw) & 0xff000000) >> 24) )
// LE<->BE translation of 8 byte big number
#define SwapEndian_8B(ptr)                                                    \
{                                                                            \
    unsigned int Temp = 0;                                                    \
    Temp = SwapEndian_DW(((unsigned int*)(ptr))[0]);                        \
    ((unsigned int*)(ptr))[0] = SwapEndian_DW(((unsigned int*)(ptr))[1]);    \
    ((unsigned int*)(ptr))[1] = Temp;                                        \
}
// Private Mb Layout structure
typedef struct _VAEncMbDataLayout
{
    unsigned char MbCodeSize;
    unsigned int  MbCodeOffset;
    unsigned int  MbCodeStride;
    unsigned char MvNumber;
    unsigned int  MvOffset;
    unsigned int  MvStride;

} VAEncMbDataLayout;
#define  VAEncryptionParameterBufferType   -3 

#define EXTERNAL_ENCRYPTED_SURFACE_FLAG   (1<<16)

// Inform driver not to do any aligment for the buffer
#define BUFFER_INFO_BYPASS_FLAG           (1<<15)

//HDCP status report buffer flag
#define HDCP2_CODED_BUF_FLAG               (1<<8)

//HDCP enable flag
#define VA_HDCP_ENABLED                    (1<<12)

//0xf is reserved for WiDi
#define WIDI_SESSION_ID					   (0xf)

#define PAVP_MAX_SESSION_ID                (0x10)

// LE<->BE translation of DWORD
#define SwapEndian_DW(dw)    ( (((dw) & 0x000000ff) << 24) |  (((dw) & 0x0000ff00) << 8) | (((dw) & 0x00ff0000) >> 8) | (((dw) & 0xff000000) >> 24) )
// LE<->BE translation of 8 byte big number
#define SwapEndian_8B(ptr)                                                    \
{                                                                            \
    unsigned int Temp = 0;                                                    \
    Temp = SwapEndian_DW(((unsigned int*)(ptr))[0]);                        \
    ((unsigned int*)(ptr))[0] = SwapEndian_DW(((unsigned int*)(ptr))[1]);    \
    ((unsigned int*)(ptr))[1] = Temp;                                        \
}

typedef struct _VAEncMiscParameterPrivate
{
    unsigned int target_usage; // Valid values 1-7 for AVC & MPEG2.
    unsigned int reserved[7];  // Reserved for future use.
} VAEncMiscParameterPrivate;

/*VAEncrytpionParameterBuffer*/
typedef struct _VAEncryptionParameterBuffer
{
    //Not used currently
    unsigned int encryptionSupport;
    //Not used currently
    unsigned int hostEncryptMode;
    // For IV, Counter input
    unsigned int pavpAesCounter[2][4];
    // not used currently
    unsigned int pavpIndex;
    // not used currently
    unsigned int pavpCounterMode;
    unsigned int pavpEncryptionType;
    // not used currently
    unsigned int pavpInputSize[2];
    // not used currently
    unsigned int pavpBufferSize[2];
    // not used currently
    unsigned int pvap_buf;
    // set to TRUE if protected media
    unsigned int pavpHasBeenEnabled;
    // not used currently
    unsigned int IntermmediatedBufReq;
    // not used currently
    unsigned int uiCounterIncrement;
    // AppId: PAVP sessin Index from application
    unsigned int app_id;

} VAEncryptionParameterBuffer;

typedef struct _VAHDCP2EncryptionStatusBuffer{
    unsigned int       status;
    bool               bEncrypted;
    unsigned int       counter[4];
}VAHDCP2EncryptionStatusBuffer;

#endif

