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

#ifndef __HWINFO_LINUX_H__
#define __HWINFO_LINUX_H__

typedef enum __GTTYPE {
	GTTYPE_GT1 = 0x0,
	GTTYPE_GT2,
	GTTYPE_GT2_FUSED_TO_GT1,
	GTTYPE_GT2_FUSED_TO_GT1_6,
	GTTYPE_GTL,
	GTTYPE_GTM,
	GTTYPE_GTH,
	GTTYPE_GT1_75,
	GTTYPE_GT3,
	GTTYPE_GT0,
	GTTYPE_UNDEFINED,
} GTTYPE, *PGTTYPE;

typedef enum {
	IGFX_UNKNOWN = 0,
	IGFX_HASWELL,
	IGFX_BROADWELL,
	IGFX_CHERRYVIEW,

	IGFX_GENNEXT = 0x7ffffffe,
	PRODUCT_FAMILY_FORCE_ULONG = 0x7fffffff
} PRODUCT_FAMILY;

typedef enum {
	PCH_UNKNOWN = 0,
	PCH_IBX,
	PCH_CPT,
	PCH_CPTR,
	PCH_PPT,
	PCH_LPT,
	PCH_LPTR,
	PCH_WPT,
	PCH_PRODUCT_FAMILY_FORCE_ULONG = 0x7fffffff
} PCH_PRODUCT_FAMILY;

typedef enum {
	IGFX_UNKNOWN_CORE = 0,
	IGFX_GEN5_CORE = 5,
	IGFX_GEN7_5_CORE = 10,
	IGFX_GEN8_CORE = 11,

	IGFX_GENNEXT_CORE = 0x7ffffffe,
	GFXCORE_FAMILY_FORCE_ULONG = 0x7fffffff
} GFXCORE_FAMILY;

typedef enum {
	PLATFORM_NONE = 0x00,
	PLATFORM_DESKTOP = 0x01,
	PLATFORM_MOBILE = 0x02,
	PLATFORM_TABLET = 0X03,
	PLATFORM_ALL = 0xff,
} PLATFORM_TYPE;

typedef struct PLATFORM_STR {
	PRODUCT_FAMILY eProductFamily;
	PCH_PRODUCT_FAMILY ePCHProductFamily;
	GFXCORE_FAMILY eDisplayCoreFamily;
	GFXCORE_FAMILY eRenderCoreFamily;
	PLATFORM_TYPE ePlatformType;

	unsigned short usRevId;
	unsigned short usRevId_PCH;
	unsigned int pchDeviceID;
	GTTYPE GtType;
} PLATFORM;

#define PCH_IS_PRODUCT(p, r)            ( (p).ePCHProductFamily == r )
#define PCH_GET_CURRENT_PRODUCT(p)      ( (p).ePCHProductFamily )

#define GFX_IS_PRODUCT(p, r)           ( (p).eProductFamily == r )
#define GFX_IS_DISPLAYCORE(p, d)       ( (p).eDisplayCoreFamily == d )
#define GFX_IS_RENDERCORE(p, r)        ( (p).eRenderCoreFamily == r )

#define GFX_GET_CURRENT_PRODUCT(p)     ( (p).eProductFamily )
#define GFX_GET_CURRENT_DISPLAYCORE(p) ( (p).eDisplayCoreFamily )
#define GFX_GET_CURRENT_RENDERCORE(p)  ( (p).eRenderCoreFamily )

#define IHSW_GTH_DESK_DEVICE_F0_ID       0x0090
#define IHSW_GTM_DESK_DEVICE_F0_ID       0x0091
#define IHSW_GTL_DESK_DEVICE_F0_ID       0x0092
#define IHSW_DESK_DEV_F0_ID              0x0C02
#define IHSW_MOBL_DEV_F0_ID              0x0C06
#define IHSW_DESK_DEV_F0_M_ID            0x0C12
#define IHSW_MOBL_DEV_F0_M_ID            0x0C16
#define IHSW_DESK_DEV_F0_H_ID            0x0C22
#define IHSW_MOBL_DEV_F0_H_ID            0x0C26
#define IHSW_VA_DEV_F0_ID                0x0C0B

#define IHSW_MOBL_DEVICE_F0_ID          0x0094

#define IHSW_CL_DESK_GT1_DEV_ID              0x402
#define IHSW_CL_MOBL_GT1_DEV_ID              0x406
#define IHSW_CL_SERV_GT1_DEV_ID              0x40A
#define IHSW_CL_DESK_GT2_DEV_ID              0x412
#define IHSW_CL_MOBL_GT2_DEV_ID              0x416
#define IHSW_CL_WS_GT2_DEV_ID                0x41B
#define IHSW_CL_SERV_GT2_DEV_ID              0x41A

#define IHSW_ULT_MOBL_GT1_DEV_ID              0xA06
#define IHSW_ULT_MOBL_GT2_DEV_ID              0xA16
#define IHSW_ULT_MOBL_GT3_DEV_ID              0xA26
#define IHSW_ULT_MRKT_GT3_DEV_ID              0xA2E

#define IHSW_ULX_MOBL_GT1_DEV_ID              0xA0E
#define IHSW_ULX_MOBL_GT2_DEV_ID              0xA1E

#define IHSW_CRW_DESK_GT2_DEV_ID              0xD12
#define IHSW_CRW_MOBL_GT2_DEV_ID              0xD16
#define IHSW_CRW_DESK_GT3_DEV_ID              0xD22
#define IHSW_CRW_MOBL_GT3_DEV_ID              0xD26
#define IHSW_CRW_SERV_GT3_DEV_ID              0xD2A

#define IVLV_PLUS_MOBL_DEVICE_F0_ID      0x0F31

#define ICHV_MOBL_DEVICE_F0_ID           0x22B0
#define ICHV_PLUS_MOBL_DEVICE_F0_ID      0x22B1
#define ICHV_DESK_DEVICE_F0_ID           0x22B2
#define ICHV_PLUS_DESK_DEVICE_F0_ID      0x22B3

#define IBDW_GT0_DESK_DEVICE_F0_ID              0x0BD0
#define IBDW_GT1_DESK_DEVICE_F0_ID              0x0BD1
#define IBDW_GT2_DESK_DEVICE_F0_ID              0x0BD2
#define IBDW_GT3_DESK_DEVICE_F0_ID              0x0BD3

#define IBDW_GT1_HALO_MOBL_DEVICE_F0_ID         0x1602
#define IBDW_GT1_ULT_MOBL_DEVICE_F0_ID          0x1606
#define IBDW_GT1_RSVD_DEVICE_F0_ID              0x160B
#define IBDW_GT1_SERV_DEVICE_F0_ID              0x160A
#define IBDW_GT1_WRK_DEVICE_F0_ID               0x160D
#define IBDW_GT1_ULX_DEVICE_F0_ID               0x160E
#define IBDW_GT2_HALO_MOBL_DEVICE_F0_ID         0x1612
#define IBDW_GT2_ULT_MOBL_DEVICE_F0_ID          0x1616
#define IBDW_GT2_RSVD_DEVICE_F0_ID              0x161B
#define IBDW_GT2_SERV_DEVICE_F0_ID              0x161A
#define IBDW_GT2_WRK_DEVICE_F0_ID               0x161D
#define IBDW_GT2_ULX_DEVICE_F0_ID               0x161E
#define IBDW_GT3_HALO_MOBL_DEVICE_F0_ID         0x1622
#define IBDW_GT3_ULT_MOBL_DEVICE_F0_ID          0x1626
#define IBDW_GT3_ULT25W_MOBL_DEVICE_F0_ID       0x162B
#define IBDW_GT3_SERV_DEVICE_F0_ID              0x162A
#define IBDW_GT3_WRK_DEVICE_F0_ID               0x162D
#define IBDW_GT3_ULX_DEVICE_F0_ID               0x162E
#define IBDW_RSVD_MRKT_DEVICE_F0_ID             0x1632
#define IBDW_RSVD_ULT_MOBL_DEVICE_F0_ID         0x1636
#define IBDW_RSVD_HALO_MOBL_DEVICE_F0_ID        0x163B
#define IBDW_RSVD_SERV_DEVICE_F0_ID             0x163A
#define IBDW_RSVD_WRK_DEVICE_F0_ID              0x163D
#define IBDW_RSVD_ULX_DEVICE_F0_ID              0x163E

#define IS_GEN7_5(device_id) ( device_id == IHSW_GTH_DESK_DEVICE_F0_ID    || \
                               device_id == IHSW_GTM_DESK_DEVICE_F0_ID    || \
                               device_id == IHSW_GTL_DESK_DEVICE_F0_ID    || \
                               device_id == IHSW_DESK_DEV_F0_ID           || \
                               device_id == IHSW_MOBL_DEV_F0_ID           || \
                               device_id == IHSW_DESK_DEV_F0_M_ID         || \
                               device_id == IHSW_MOBL_DEV_F0_M_ID         || \
                               device_id == IHSW_DESK_DEV_F0_H_ID         || \
                               device_id == IHSW_MOBL_DEV_F0_H_ID         || \
                               device_id == IHSW_VA_DEV_F0_ID             || \
                               device_id == IHSW_MOBL_DEVICE_F0_ID        || \
                               device_id == IHSW_CL_DESK_GT1_DEV_ID       || \
                               device_id == IHSW_CL_MOBL_GT1_DEV_ID       || \
                               device_id == IHSW_CL_SERV_GT1_DEV_ID       || \
                               device_id == IHSW_CL_DESK_GT2_DEV_ID       || \
                               device_id == IHSW_CL_MOBL_GT2_DEV_ID       || \
                               device_id == IHSW_CL_WS_GT2_DEV_ID         || \
                               device_id == IHSW_CL_SERV_GT2_DEV_ID       || \
                               device_id == IHSW_ULT_MOBL_GT1_DEV_ID      || \
                               device_id == IHSW_ULT_MOBL_GT2_DEV_ID      || \
                               device_id == IHSW_ULT_MOBL_GT3_DEV_ID      || \
                               device_id == IHSW_ULT_MRKT_GT3_DEV_ID      || \
                               device_id == IHSW_ULX_MOBL_GT1_DEV_ID      || \
                               device_id == IHSW_ULX_MOBL_GT2_DEV_ID      || \
                               device_id == IHSW_CRW_DESK_GT2_DEV_ID      || \
                               device_id == IHSW_CRW_MOBL_GT2_DEV_ID      || \
                               device_id == IHSW_CRW_DESK_GT3_DEV_ID      || \
                               device_id == IHSW_CRW_MOBL_GT3_DEV_ID      || \
                               device_id == IHSW_CRW_SERV_GT3_DEV_ID )

#define IS_BROADWELL(device_id)  ( device_id == IBDW_GT0_DESK_DEVICE_F0_ID        || \
                                   device_id == IBDW_GT1_DESK_DEVICE_F0_ID        || \
                                   device_id == IBDW_GT2_DESK_DEVICE_F0_ID        || \
                                   device_id == IBDW_GT3_DESK_DEVICE_F0_ID        || \
                                   device_id == IBDW_GT1_RSVD_DEVICE_F0_ID        || \
                                   device_id == IBDW_GT1_ULT_MOBL_DEVICE_F0_ID    || \
                                   device_id == IBDW_GT1_HALO_MOBL_DEVICE_F0_ID   || \
                                   device_id == IBDW_GT1_SERV_DEVICE_F0_ID        || \
                                   device_id == IBDW_GT1_WRK_DEVICE_F0_ID         || \
                                   device_id == IBDW_GT1_ULX_DEVICE_F0_ID         || \
                                   device_id == IBDW_GT2_RSVD_DEVICE_F0_ID        || \
                                   device_id == IBDW_GT2_ULT_MOBL_DEVICE_F0_ID    || \
                                   device_id == IBDW_GT2_HALO_MOBL_DEVICE_F0_ID   || \
                                   device_id == IBDW_GT2_SERV_DEVICE_F0_ID        || \
                                   device_id == IBDW_GT2_WRK_DEVICE_F0_ID         || \
                                   device_id == IBDW_GT2_ULX_DEVICE_F0_ID         || \
                                   device_id == IBDW_GT3_ULT25W_MOBL_DEVICE_F0_ID || \
                                   device_id == IBDW_GT3_ULT_MOBL_DEVICE_F0_ID    || \
                                   device_id == IBDW_GT3_HALO_MOBL_DEVICE_F0_ID   || \
                                   device_id == IBDW_GT3_SERV_DEVICE_F0_ID        || \
                                   device_id == IBDW_GT3_WRK_DEVICE_F0_ID         || \
                                   device_id == IBDW_GT3_ULX_DEVICE_F0_ID         || \
                                   device_id == IBDW_RSVD_MRKT_DEVICE_F0_ID       || \
                                   device_id == IBDW_RSVD_ULT_MOBL_DEVICE_F0_ID   || \
                                   device_id == IBDW_RSVD_HALO_MOBL_DEVICE_F0_ID  || \
                                   device_id == IBDW_RSVD_SERV_DEVICE_F0_ID       || \
                                   device_id == IBDW_RSVD_WRK_DEVICE_F0_ID        || \
                                   device_id == IBDW_RSVD_ULX_DEVICE_F0_ID )

#define IS_CHERRYVIEW(device_id) ( device_id == ICHV_DESK_DEVICE_F0_ID      || \
                                   device_id == ICHV_MOBL_DEVICE_F0_ID      || \
                                   device_id == ICHV_PLUS_DESK_DEVICE_F0_ID || \
                                   device_id == ICHV_PLUS_MOBL_DEVICE_F0_ID )

#define IS_GEN8(device_id)       ( IS_BROADWELL(device_id) || IS_CHERRYVIEW(device_id) )

#define IS_SUPPORTED_GEN(device_id) ( \
                                      IS_GEN7_5(device_id)        || \
                                      IS_GEN8(device_id)          || \
                                      IS_VALLEYVIEW_A0(device_id) )

#define IS_GEN7_5_PLUS(device_id)   ( IS_GEN7_5(device_id) || \
                                      IS_GEN8(device_id) )

#define IS_GEN8_PLUS(device_id)     ( IS_GEN8(device_id) )

#endif
