/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#ifndef BRW_AUB_H
#define BRW_AUB_H

struct aub_file_header {
   unsigned int instruction_type;
   unsigned int pad0:16;
   unsigned int minor:8;
   unsigned int major:8;
   unsigned char application[8*4];
   unsigned int day:8;
   unsigned int month:8;
   unsigned int year:16;
   unsigned int timezone:8;
   unsigned int second:8;
   unsigned int minute:8;
   unsigned int hour:8;
   unsigned int comment_length:16;   
   unsigned int pad1:16;
};

struct aub_block_header {
   unsigned int instruction_type;
   unsigned int operation:8;
   unsigned int type:8;
   unsigned int address_space:8;
   unsigned int pad0:8;
   unsigned int general_state_type:8;
   unsigned int surface_state_type:8;
   unsigned int pad1:16;
   unsigned int address;
   unsigned int length;
};

struct aub_dump_bmp {
   unsigned int instruction_type;
   unsigned int xmin:16;
   unsigned int ymin:16;
   unsigned int pitch:16;
   unsigned int bpp:8;
   unsigned int format:8;
   unsigned int xsize:16;
   unsigned int ysize:16;
   unsigned int addr;
   unsigned int unknown;
};

enum bh_operation {
   BH_COMMENT,
   BH_DATA_WRITE,
   BH_COMMAND_WRITE,
   BH_MMI0_WRITE32,
   BH_END_SCENE,
   BH_CONFIG_MEMORY_MAP,
   BH_MAX_OPERATION
};

enum command_write_type {
   CW_HWB_RING = 1,
   CW_PRIMARY_RING_A,
   CW_PRIMARY_RING_B,		/* XXX - disagreement with listaub! */
   CW_PRIMARY_RING_C,
   CW_MAX_TYPE
};

enum data_write_type {
   DW_NOTYPE,
   DW_BATCH_BUFFER,
   DW_BIN_BUFFER,
   DW_BIN_POINTER_LIST,
   DW_SLOW_STATE_BUFFER,
   DW_VERTEX_BUFFER,
   DW_2D_MAP,
   DW_CUBE_MAP,
   DW_INDIRECT_STATE_BUFFER,
   DW_VOLUME_MAP,
   DW_1D_MAP,
   DW_CONSTANT_BUFFER,
   DW_CONSTANT_URB_ENTRY,
   DW_INDEX_BUFFER,
   DW_GENERAL_STATE,
   DW_SURFACE_STATE,
   DW_MEDIA_OBJECT_INDIRECT_DATA,
   DW_MAX_TYPE
};

enum data_write_general_state_type {
   DWGS_NOTYPE,
   DWGS_VERTEX_SHADER_STATE,
   DWGS_GEOMETRY_SHADER_STATE ,
   DWGS_CLIPPER_STATE,
   DWGS_STRIPS_FANS_STATE,
   DWGS_WINDOWER_IZ_STATE,
   DWGS_COLOR_CALC_STATE,
   DWGS_CLIPPER_VIEWPORT_STATE,	/* was 0x7 */
   DWGS_STRIPS_FANS_VIEWPORT_STATE,
   DWGS_COLOR_CALC_VIEWPORT_STATE, /* was 0x9 */
   DWGS_SAMPLER_STATE,
   DWGS_KERNEL_INSTRUCTIONS,
   DWGS_SCRATCH_SPACE,
   DWGS_SAMPLER_DEFAULT_COLOR,
   DWGS_INTERFACE_DESCRIPTOR,
   DWGS_VLD_STATE,
   DWGS_VFE_STATE,
   DWGS_MAX_TYPE
};

enum data_write_surface_state_type {
   DWSS_NOTYPE,
   DWSS_BINDING_TABLE_STATE,
   DWSS_SURFACE_STATE,
   DWSS_MAX_TYPE
};

enum memory_map_type {
   MM_DEFAULT,
   MM_DYNAMIC,
   MM_MAX_TYPE
};

enum address_space {
   ADDR_GTT,
   ADDR_LOCAL,
   ADDR_MAIN,
   ADDR_MAX
};


#define AUB_FILE_HEADER 0xe085000b
#define AUB_BLOCK_HEADER 0xe0c10003
#define AUB_DUMP_BMP 0xe09e0004

struct brw_context;
struct intel_context;

int brw_aub_init( struct brw_context *brw );
void brw_aub_destroy( struct brw_context *brw );

int brw_playback_aubfile(struct brw_context *brw,
			 const char *filename);

#endif
