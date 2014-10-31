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

#ifndef _MEDIA__DRIVER_HWCMDS_G8_H
#define _MEDIA__DRIVER_HWCMDS_G8_H

#include "media_drv_batchbuffer.h"
#define CMD_PIPE_CONTROL_WC_FLUSH               (1 << 12)

STATUS media_object_walker_cmd_g8 (MEDIA_BATCH_BUFFER * batch,MEDIA_OBJ_WALKER_PARAMS * params);
STATUS mediadrv_gen_pipe_ctrl_cmd_g8 (MEDIA_BATCH_BUFFER * batch,PIPE_CONTROL_PARAMS * params);
STATUS mediadrv_gen_state_base_address_cmd_g8 (MEDIA_BATCH_BUFFER * batch,STATE_BASE_ADDR_PARAMS * params);
STATUS mediadrv_gen_media_vfe_state_cmd_g8 (MEDIA_BATCH_BUFFER * batch,VFE_STATE_PARAMS * params);

#endif
