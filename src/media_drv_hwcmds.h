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
#ifndef _MEDIA__DRIVER_HWCMDS_H
#define _MEDIA__DRIVER_HWCMDS_H
#include "media_drv_hw.h"
#include "media_drv_init.h"
#define CMD(pipeline,op,sub_op)		((3 << 29) | \
                                           	((pipeline) << 27) | \
                                           	((op) << 24) | \
                                           	((sub_op) << 16))
#define CMD_MI                                  (0x0 << 29)
#define CMD_2D                                  (0x2 << 29)
#define CMD_3D                                  (0x3 << 29)
#define MI_BATCH_BUFFER_END                     (CMD_MI | (0xA << 23))
#define MI_BATCH_BUFFER_START                   (CMD_MI | (0x31 << 23))
#define BATCH_SIZE      0x80000
#define BATCH_RESERVED  0x10
#define MAX_BATCH_SIZE		0x400000
//PIPELINE CONTROL
#define CMD_PIPE_CONTROL                        (CMD_3D | (3 << 27) | (2 << 24) | (0 << 16))
#define CMD_PIPE_CONTROL_DEST_ADDR_TYPE         (1 << 24)
#define CMD_PIPE_CONTROL_CS_STALL               (1 << 20)
#define CMD_PIPE_CONTROL_NOWRITE                (0 << 14)
#define CMD_PIPE_CONTROL_WRITE_QWORD            (1 << 14)
#define CMD_PIPE_CONTROL_WRITE_DEPTH            (2 << 14)
#define CMD_PIPE_CONTROL_WRITE_TIME             (3 << 14)
#define CMD_PIPE_CONTROL_DEPTH_STALL            (1 << 13)
#define CMD_PIPE_CONTROL_RT_FLUSH_ENABLE               (1 << 12)
#define CMD_PIPE_CONTROL_INSTR_CI_ENABLE               (1 << 11)
#define CMD_PIPE_CONTROL_TC_FLUSH               (1 << 10)
#define CMD_PIPE_CONTROL_NOTIFY_ENABLE          (1 << 8)
#define CMD_PIPE_CONTROL_FLUSH_ENABLE           (1 << 7)
#define CMD_PIPE_CONTROL_DC_FLUSH               (1 << 5)
#define CMD_PIPE_CONTROL_VF_CI_ENABLE     (1 << 4)
#define CMD_PIPE_CONTROL_CONSTANT_CI_ENABLE     (1 << 3)
#define CMD_PIPE_CONTROL_STATE_CI_ENABLE        (1 << 2)
#define CMD_PIPE_CONTROL_GLOBAL_GTT             (1 << 2)
#define CMD_PIPE_CONTROL_LOCAL_PGTT             (0 << 2)
#define CMD_PIPE_CONTROL_STALL_AT_SCOREBOARD    (1 << 1)
#define CMD_PIPE_CONTROL_DEPTH_CACHE_FLUSH      (1 << 0)
#define CMD_PIPE_CONTROL_DWORD_LEN   3
//STATE_BASE_ADDRESS
#define BASE_ADDRESS_MODIFY             (1 << 0)
#define CMD_STATE_BASE_ADDRESS_LEN  12//10
#define CMD_STATE_BASE_ADDRESS CMD(0,1,1)
//PIPELINE SELECT
#define CMD_PIPELINE_SELECT                     CMD(1, 1, 4)
#define PIPELINE_SELECT_3D              0
#define PIPELINE_SELECT_MEDIA           1


//MEDIA VFE STATE
#define CMD_MEDIA_VFE_STATE_LEN 8
#define CMD_MEDIA_VFE_STATE                     CMD(2, 0, 0)

//MEDIA CURBE LOAD
#define CMD_MEDIA_CURBE_LOAD                    CMD(2, 0, 1)

//ID LOAD
#define CMD_MEDIA_INTERFACE_LOAD                CMD(2, 0, 2)

//MI PREDICATE
#define CMD_MI_SET_PREDICATE    (CMD_MI | (1<<23))

//MEDIA_OBJECT_WALKER
#define CMD_MEDIA_OBJECT_WALKER_LEN 17
#define CMD_MEDIA_OBJECT_WALKER                 CMD(2, 1, 3)

//MEDIA_OBJECT
#define CMD_MEDIA_OBJECT_LEN                    6
#define CMD_MEDIA_OBJECT                        CMD(2, 1, 0)

//MI STORE DATA IMM
#define CMD_MI_STORE_DATA_IMM   (CMD_MI | (1<<28))

#define FLUSH_NONE      0x00
#define FLUSH_WRITE_CACHE  0x01
#define FLUSH_READ_CACHE   0x02

#define I_FRM          1
#define P_FRM          2
#define B_FRM          3
#define SINGLE_MODE    1
#define DUAL_MODE      2
#define TRI_MODE       3
#define QUAD_MODE 4

#define  SCALE_SRC_Y 0
#define  SCALE_DST_Y 1

//#define DEGREE_46 3
typedef struct media_curbe_scaling_data
{
  UINT input_pic_width:16;
  UINT input_pic_height:16;
  UINT src_planar_y:32;
  UINT dest_planar_y:32;
} CURBE_SCALING_DATA;

STATUS mediadrv_set_curbe_scaling (MEDIA_GPE_CTX * gpe_context,
				   SCALING_CURBE_PARAMS * params);
STATUS mediadrv_mi_store_data_imm_cmd (MEDIA_BATCH_BUFFER * batch,
				       MI_STORE_DATA_IMM_PARAMS * params);
STATUS media_object_walker_cmd (MEDIA_BATCH_BUFFER * batch,
				MEDIA_OBJ_WALKER_PARAMS * params);
STATUS mediadrv_media_mi_set_predicate_cmd (MEDIA_BATCH_BUFFER * batch,
					    MI_SET_PREDICATE_PARAMS * params);
STATUS mediadrv_gen_media_curbe_load_cmd (MEDIA_BATCH_BUFFER * batch,
					  CURBE_LOAD_PARAMS * params);
STATUS mediadrv_gen_media_id_load_cmd (MEDIA_BATCH_BUFFER * batch,
				       ID_LOAD_PARAMS * params);
STATUS mediadrv_gen_media_vfe_state_cmd (MEDIA_BATCH_BUFFER * batch,
					 VFE_STATE_PARAMS * params);
STATUS mediadrv_gen_state_base_address_cmd (MEDIA_BATCH_BUFFER * batch,
					    STATE_BASE_ADDR_PARAMS * params);
STATUS mediadrv_gen_pipeline_select_cmd (MEDIA_BATCH_BUFFER * batch);
STATUS mediadrv_gen_pipe_ctrl_cmd (MEDIA_BATCH_BUFFER * batch,
				   PIPE_CONTROL_PARAMS * params);

STATUS
media_object_cmd (MEDIA_BATCH_BUFFER *batch,
                  MEDIA_OBJECT_PARAMS *params);


#define MI_FLUSH                                (CMD_MI | (0x4 << 23))
#define   MI_FLUSH_STATE_INSTRUCTION_CACHE_INVALIDATE   (0x1 << 0)
#define MI_FLUSH_DW                             (CMD_MI | (0x26 << 23) | 0x2)
#define   MI_FLUSH_DW_VIDEO_PIPELINE_CACHE_INVALIDATE   (0x1 << 7)

#define CMD_PIPE_CONTROL_WC_FLUSH               (1 << 12)

#endif
