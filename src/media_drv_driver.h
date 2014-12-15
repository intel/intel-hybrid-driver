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

#ifndef _MEDIA__DRIVER_H
#define _MEDIA__DRIVER_H
#include <va/va_backend.h>
#include "media_drv_util.h"
#include "media_drv_defines.h"
#include "media_drv_init.h"
#include "va_backend_compat.h"


#define PCI_CHIP_IVYBRIDGE_GT1          0x0152	/* Desktop */
#define PCI_CHIP_IVYBRIDGE_GT2          0x0162
#define PCI_CHIP_IVYBRIDGE_M_GT1        0x0156	/* Mobile */
#define PCI_CHIP_IVYBRIDGE_M_GT2        0x0166
#define PCI_CHIP_IVYBRIDGE_S_GT1        0x015a	/* Server */
#define PCI_CHIP_IVYBRIDGE_S_GT2        0x016a
#define PCI_CHIP_BAYTRAIL_M_1           0x0F31
#define PCI_CHIP_BAYTRAIL_M_2           0x0F32
#define PCI_CHIP_BAYTRAIL_M_3           0x0F33
#define PCI_CHIP_BAYTRAIL_M_4           0x0157
#define PCI_CHIP_BAYTRAIL_D             0x0155


#define CONFIG_ID_OFFSET                0x01000000
#define CONTEXT_ID_OFFSET               0x02000000
#define SURFACE_ID_OFFSET               0x04000000
#define BUFFER_ID_OFFSET                0x08000000
#define IMAGE_ID_OFFSET                 0x0a000000
#define SUBPIC_ID_OFFSET                0x10000000
#define PCI_CHIP_HASWELL_GT1            0x0402	/* Desktop */
#define PCI_CHIP_HASWELL_GT2            0x0412
#define PCI_CHIP_HASWELL_GT3            0x0422
#define PCI_CHIP_HASWELL_M_GT1          0x0406	/* Mobile */
#define PCI_CHIP_HASWELL_M_GT2          0x0416
#define PCI_CHIP_HASWELL_M_GT3          0x0426
#define PCI_CHIP_HASWELL_S_GT1          0x040a	/* Server */
#define PCI_CHIP_HASWELL_S_GT2          0x041a
#define PCI_CHIP_HASWELL_S_GT3          0x042a
#define PCI_CHIP_HASWELL_B_GT1          0x040b	/* Reserved */
#define PCI_CHIP_HASWELL_B_GT2          0x041b
#define PCI_CHIP_HASWELL_B_GT3          0x042b
#define PCI_CHIP_HASWELL_E_GT1          0x040e	/* Reserved */
#define PCI_CHIP_HASWELL_E_GT2          0x041e
#define PCI_CHIP_HASWELL_E_GT3          0x042e

#define	PCI_CHIP_HASWELL_SDV_GT1		0x0c02	/* Desktop */
#define	PCI_CHIP_HASWELL_SDV_GT2		0x0c12
#define	PCI_CHIP_HASWELL_SDV_GT3		0x0c22
#define	PCI_CHIP_HASWELL_SDV_M_GT1		0x0c06	/* Mobile */
#define	PCI_CHIP_HASWELL_SDV_M_GT2		0x0c16
#define	PCI_CHIP_HASWELL_SDV_M_GT3		0x0c26
#define	PCI_CHIP_HASWELL_SDV_S_GT1		0x0c0a	/* Server */
#define	PCI_CHIP_HASWELL_SDV_S_GT2		0x0c1a
#define	PCI_CHIP_HASWELL_SDV_S_GT3		0x0c2a
#define PCI_CHIP_HASWELL_SDV_B_GT1              0x0c0b	/* Reserved */
#define PCI_CHIP_HASWELL_SDV_B_GT2              0x0c1b
#define PCI_CHIP_HASWELL_SDV_B_GT3              0x0c2b
#define PCI_CHIP_HASWELL_SDV_E_GT1              0x0c0e	/* Reserved */
#define PCI_CHIP_HASWELL_SDV_E_GT2              0x0c1e
#define PCI_CHIP_HASWELL_SDV_E_GT3              0x0c2e

#define	PCI_CHIP_HASWELL_ULT_GT1		0x0A02	/* Desktop */
#define	PCI_CHIP_HASWELL_ULT_GT2		0x0A12
#define	PCI_CHIP_HASWELL_ULT_GT3		0x0A22
#define	PCI_CHIP_HASWELL_ULT_M_GT1		0x0A06	/* Mobile */
#define	PCI_CHIP_HASWELL_ULT_M_GT2		0x0A16
#define	PCI_CHIP_HASWELL_ULT_M_GT3		0x0A26
#define	PCI_CHIP_HASWELL_ULT_S_GT1		0x0A0A	/* Server */
#define	PCI_CHIP_HASWELL_ULT_S_GT2		0x0A1A
#define	PCI_CHIP_HASWELL_ULT_S_GT3		0x0A2A
#define PCI_CHIP_HASWELL_ULT_B_GT1              0x0A0B	/* Reserved */
#define PCI_CHIP_HASWELL_ULT_B_GT2              0x0A1B
#define PCI_CHIP_HASWELL_ULT_B_GT3              0x0A2B
#define PCI_CHIP_HASWELL_ULT_E_GT1              0x0A0E	/* Reserved */
#define PCI_CHIP_HASWELL_ULT_E_GT2              0x0A1E
#define PCI_CHIP_HASWELL_ULT_E_GT3              0x0A2E

#define	PCI_CHIP_HASWELL_CRW_GT1		0x0D02	/* Desktop */
#define	PCI_CHIP_HASWELL_CRW_GT2		0x0D12
#define	PCI_CHIP_HASWELL_CRW_GT3		0x0D22
#define	PCI_CHIP_HASWELL_CRW_M_GT1		0x0D06	/* Mobile */
#define	PCI_CHIP_HASWELL_CRW_M_GT2		0x0D16
#define	PCI_CHIP_HASWELL_CRW_M_GT3		0x0D26
#define	PCI_CHIP_HASWELL_CRW_S_GT1		0x0D0A	/* Server */
#define	PCI_CHIP_HASWELL_CRW_S_GT2		0x0D1A
#define	PCI_CHIP_HASWELL_CRW_S_GT3		0x0D2A
#define PCI_CHIP_HASWELL_CRW_B_GT1              0x0D0B	/* Reserved */
#define PCI_CHIP_HASWELL_CRW_B_GT2              0x0D1B
#define PCI_CHIP_HASWELL_CRW_B_GT3              0x0D2B
#define PCI_CHIP_HASWELL_CRW_E_GT1              0x0D0E	/* Reserved */
#define PCI_CHIP_HASWELL_CRW_E_GT2              0x0D1E
#define PCI_CHIP_HASWELL_CRW_E_GT3              0x0D2E

#define IS_BAYTRAIL_M1(devid)    (devid == PCI_CHIP_BAYTRAIL_M_1)
#define IS_BAYTRAIL_M2(devid)    (devid == PCI_CHIP_BAYTRAIL_M_2)
#define IS_BAYTRAIL_M3(devid)    (devid == PCI_CHIP_BAYTRAIL_M_3)
#define IS_BAYTRAIL_D(devid)     (devid == PCI_CHIP_BAYTRAIL_D)
#define IS_BAYTRAIL(devid)       (IS_BAYTRAIL_M1(devid) || \
                                  IS_BAYTRAIL_M2(devid) || \
                                  IS_BAYTRAIL_M3(devid) || \
                                  IS_BAYTRAIL_D(devid) )

#define IS_IVB_GT1(devid)       (devid == PCI_CHIP_IVYBRIDGE_GT1 ||     \
                                 devid == PCI_CHIP_IVYBRIDGE_M_GT1 ||   \
                                 devid == PCI_CHIP_IVYBRIDGE_S_GT1)

#define IS_IVB_GT2(devid)       (devid == PCI_CHIP_IVYBRIDGE_GT2 ||     \
                                 devid == PCI_CHIP_IVYBRIDGE_M_GT2 ||   \
                                 devid == PCI_CHIP_IVYBRIDGE_S_GT2)

#define IS_IVYBRIDGE(devid)     (IS_IVB_GT1(devid) ||   \
                                 IS_IVB_GT2(devid) ||   \
                                 IS_BAYTRAIL(devid) )

#define IS_HSW_GT1(devid)   	(devid == PCI_CHIP_HASWELL_GT1		|| \
                                 devid == PCI_CHIP_HASWELL_M_GT1	|| \
                                 devid == PCI_CHIP_HASWELL_S_GT1	|| \
				 devid == PCI_CHIP_HASWELL_B_GT1        || \
				 devid == PCI_CHIP_HASWELL_E_GT1        || \
                                 devid == PCI_CHIP_HASWELL_SDV_GT1	|| \
                                 devid == PCI_CHIP_HASWELL_SDV_M_GT1	|| \
                                 devid == PCI_CHIP_HASWELL_SDV_S_GT1	|| \
				 devid == PCI_CHIP_HASWELL_SDV_B_GT1    || \
				 devid == PCI_CHIP_HASWELL_SDV_E_GT1    || \
                                 devid == PCI_CHIP_HASWELL_CRW_GT1	|| \
                                 devid == PCI_CHIP_HASWELL_CRW_M_GT1	|| \
                                 devid == PCI_CHIP_HASWELL_CRW_S_GT1    || \
				 devid == PCI_CHIP_HASWELL_CRW_B_GT1    || \
				 devid == PCI_CHIP_HASWELL_CRW_E_GT1    || \
                                 devid == PCI_CHIP_HASWELL_ULT_GT1	|| \
                                 devid == PCI_CHIP_HASWELL_ULT_M_GT1	|| \
                                 devid == PCI_CHIP_HASWELL_ULT_S_GT1    || \
				 devid == PCI_CHIP_HASWELL_ULT_B_GT1    || \
				 devid == PCI_CHIP_HASWELL_ULT_E_GT1)


#define IS_HSW_GT2(devid)   	(devid == PCI_CHIP_HASWELL_GT2||        \
                                 devid == PCI_CHIP_HASWELL_M_GT2||      \
                                 devid == PCI_CHIP_HASWELL_S_GT2||      \
				 devid == PCI_CHIP_HASWELL_B_GT2 || \
				 devid == PCI_CHIP_HASWELL_E_GT2 || \
                                 devid == PCI_CHIP_HASWELL_SDV_GT2||    \
                                 devid == PCI_CHIP_HASWELL_SDV_M_GT2||  \
                                 devid == PCI_CHIP_HASWELL_SDV_S_GT2||  \
				 devid == PCI_CHIP_HASWELL_SDV_B_GT2 || \
				 devid == PCI_CHIP_HASWELL_SDV_E_GT2 || \
                                 devid == PCI_CHIP_HASWELL_CRW_GT2||    \
                                 devid == PCI_CHIP_HASWELL_CRW_M_GT2||  \
                                 devid == PCI_CHIP_HASWELL_CRW_S_GT2||  \
				 devid == PCI_CHIP_HASWELL_CRW_B_GT2|| \
				 devid == PCI_CHIP_HASWELL_CRW_E_GT2|| \
                                 devid == PCI_CHIP_HASWELL_ULT_GT2||    \
                                 devid == PCI_CHIP_HASWELL_ULT_M_GT2||  \
                                 devid == PCI_CHIP_HASWELL_ULT_S_GT2||  \
				 devid == PCI_CHIP_HASWELL_ULT_B_GT2 || \
				 devid == PCI_CHIP_HASWELL_ULT_E_GT2)

#define VA_INTEL_HYBRID_PRE_DUMP	(1 << 2)
#define VA_INTEL_HYBRID_POST_DUMP	(1 << 3)

#define IS_HSW_GT3(devid)   	(devid == PCI_CHIP_HASWELL_GT3          || \
                                 devid == PCI_CHIP_HASWELL_M_GT3        || \
                                 devid == PCI_CHIP_HASWELL_S_GT3        || \
				 devid == PCI_CHIP_HASWELL_B_GT3        || \
				 devid == PCI_CHIP_HASWELL_E_GT3        || \
                                 devid == PCI_CHIP_HASWELL_SDV_GT3      || \
                                 devid == PCI_CHIP_HASWELL_SDV_M_GT3    || \
                                 devid == PCI_CHIP_HASWELL_SDV_S_GT3    || \
				 devid == PCI_CHIP_HASWELL_SDV_B_GT3    || \
				 devid == PCI_CHIP_HASWELL_SDV_E_GT3    || \
                                 devid == PCI_CHIP_HASWELL_CRW_GT3      || \
                                 devid == PCI_CHIP_HASWELL_CRW_M_GT3    || \
                                 devid == PCI_CHIP_HASWELL_CRW_S_GT3    || \
				 devid == PCI_CHIP_HASWELL_CRW_B_GT3    || \
				 devid == PCI_CHIP_HASWELL_CRW_E_GT3    || \
                                 devid == PCI_CHIP_HASWELL_ULT_GT3      || \
                                 devid == PCI_CHIP_HASWELL_ULT_M_GT3    || \
                                 devid == PCI_CHIP_HASWELL_ULT_S_GT3    || \
				 devid == PCI_CHIP_HASWELL_ULT_B_GT3    || \
				 devid == PCI_CHIP_HASWELL_ULT_E_GT3)

#define IS_HASWELL(devid)       (IS_HSW_GT1(devid) || \
                                 IS_HSW_GT2(devid) || \
                                 IS_HSW_GT3(devid))

#define IS_GEN75(devid)          (IS_HASWELL(devid))

#define IS_GEN7(devid)          (IS_IVYBRIDGE(devid))

#define IS_BROADWELL(devid)             (devid == 0x1602 || \
	                                 devid == 0x1606 || \
	                                 devid == 0x160A || \
	                                 devid == 0x160B || \
	                                 devid == 0x160D || \
	                                 devid == 0x160E || \
	                                 devid == 0x1612 || \
	                                 devid == 0x1616 || \
	                                 devid == 0x161A || \
	                                 devid == 0x161B || \
	                                 devid == 0x161D || \
	                                 devid == 0x161E || \
	                                 devid == 0x1622 || \
	                                 devid == 0x1626 || \
	                                 devid == 0x162A || \
	                                 devid == 0x162B || \
	                                 devid == 0x162D || \
	                                 devid == 0x162E )

#define IS_GEN8(devid)  (IS_BROADWELL(devid))

struct region
{
  INT x;
  INT y;
  UINT width;
  UINT height;
  UINT cpp;
  UINT pitch;
  UINT tiling;
  UINT swizzle;
  dri_bo *bo;
};

VOID media_driver_terminate (VADriverContextP ctx);
VOID media_driver_data_terminate (VADriverContextP ctx);

BOOL media_driver_init (VADriverContextP ctx);

BOOL media_driver_data_init (VADriverContextP ctx);
VOID media_destroy_config (struct object_heap *heap, struct object_base *obj);
VOID
media_destroy_context (struct object_heap *heap, struct object_base *obj);
VOID media_destroy_buffer (struct object_heap *heap, struct object_base *obj);
VOID media_release_buffer_store (struct buffer_store **ptr);

void media_destroy_subpic (struct object_heap *heap, struct object_base *obj);

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t g_intel_debug_option_flags;

#ifdef __cplusplus
}
#endif

#endif
