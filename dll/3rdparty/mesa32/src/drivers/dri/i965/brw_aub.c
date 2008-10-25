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

#include "brw_context.h"
#include "brw_aub.h"
#include "intel_regions.h"
#include <stdio.h>

extern char *__progname;


/* Registers to control page table
 */
#define PGETBL_CTL       0x2020
#define PGETBL_ENABLED   0x1

#define NR_GTT_ENTRIES  65536	/* 256 mb */

#define FAIL										\
do {											\
   fprintf(stderr, "failed to write aub data at %s/%d\n", __FUNCTION__, __LINE__);	\
   exit(1);										\
} while (0)


/* Emit the headers at the top of each aubfile.  Initialize the GTT.
 */
static void init_aubfile( FILE *aub_file )
{   
   struct aub_file_header fh;
   struct aub_block_header bh;
   unsigned int data;

   static int nr;
   
   nr++;

   /* Emit the aub header:
    */
   memset(&fh, 0, sizeof(fh));

   fh.instruction_type = AUB_FILE_HEADER;
   fh.minor = 0x0;
   fh.major = 0x7;
   memcpy(fh.application, __progname, sizeof(fh.application));
   fh.day = (nr>>24) & 0xff;
   fh.month = 0x0;
   fh.year = 0x0;
   fh.timezone = 0x0;
   fh.second = nr & 0xff;
   fh.minute = (nr>>8) & 0xff;
   fh.hour = (nr>>16) & 0xff;
   fh.comment_length = 0x0;   

   if (fwrite(&fh, sizeof(fh), 1, aub_file) < 1) 
      FAIL;
         
   /* Setup the GTT starting at main memory address zero (!):
    */
   memset(&bh, 0, sizeof(bh));
   
   bh.instruction_type = AUB_BLOCK_HEADER;
   bh.operation = BH_MMI0_WRITE32;
   bh.type = 0x0;
   bh.address_space = ADDR_GTT;	/* ??? */
   bh.general_state_type = 0x0;
   bh.surface_state_type = 0x0;
   bh.address = PGETBL_CTL;
   bh.length = 0x4;

   if (fwrite(&bh, sizeof(bh), 1, aub_file) < 1) 
      FAIL;

   data = 0x0 | PGETBL_ENABLED;

   if (fwrite(&data, sizeof(data), 1, aub_file) < 1) 
      FAIL;
}


static void init_aub_gtt( struct brw_context *brw,
			  GLuint start_offset, 
			  GLuint size )
{
   FILE *aub_file = brw->intel.aub_file;
   struct aub_block_header bh;
   unsigned int i;

   assert(start_offset + size < NR_GTT_ENTRIES * 4096);


   memset(&bh, 0, sizeof(bh));
   
   bh.instruction_type = AUB_BLOCK_HEADER;
   bh.operation = BH_DATA_WRITE;
   bh.type = 0x0;
   bh.address_space = ADDR_MAIN;
   bh.general_state_type = 0x0;
   bh.surface_state_type = 0x0;
   bh.address =  start_offset / 4096 * 4;
   bh.length = size / 4096 * 4;

   if (fwrite(&bh, sizeof(bh), 1, aub_file) < 1) 
      FAIL;

   for (i = 0; i < size / 4096; i++) {
      GLuint data = brw->next_free_page | 1;

      brw->next_free_page += 4096;

      if (fwrite(&data, sizeof(data), 1, aub_file) < 1) 
	 FAIL;
   }

}

static void write_block_header( FILE *aub_file,
				struct aub_block_header *bh,
				const GLuint *data,
				GLuint sz )
{
   sz = (sz + 3) & ~3;

   if (fwrite(bh, sizeof(*bh), 1, aub_file) < 1) 
      FAIL;

   if (fwrite(data, sz, 1, aub_file) < 1) 
      FAIL;

   fflush(aub_file);
}


static void write_dump_bmp( FILE *aub_file,
			    struct aub_dump_bmp *db )
{
   if (fwrite(db, sizeof(*db), 1, aub_file) < 1) 
      FAIL;

   fflush(aub_file);
}



static void brw_aub_gtt_data( struct intel_context *intel,
			      GLuint offset,
			      const void *data,
			      GLuint sz,
			      GLuint type,
			      GLuint state_type )
{
   struct aub_block_header bh;

   bh.instruction_type = AUB_BLOCK_HEADER;
   bh.operation = BH_DATA_WRITE;
   bh.type = type;
   bh.address_space = ADDR_GTT;
   bh.pad0 = 0;

   if (type == DW_GENERAL_STATE) {
      bh.general_state_type = state_type;
      bh.surface_state_type = 0;
   }
   else {
      bh.general_state_type = 0;
      bh.surface_state_type = state_type;
   }

   bh.pad1 = 0;
   bh.address = offset;
   bh.length = sz;

   write_block_header(intel->aub_file, &bh, data, sz);
}



static void brw_aub_gtt_cmds( struct intel_context *intel,
			      GLuint offset,
			      const void *data,
			      GLuint sz )
{
   struct brw_context *brw = brw_context(&intel->ctx);
   struct aub_block_header bh;   
   GLuint type = CW_PRIMARY_RING_A;
   

   bh.instruction_type = AUB_BLOCK_HEADER;
   bh.operation = BH_COMMAND_WRITE;
   bh.type = type;
   bh.address_space = ADDR_GTT;
   bh.pad0 = 0;
   bh.general_state_type = 0;
   bh.surface_state_type = 0;
   bh.pad1 = 0;
   bh.address = offset;
   bh.length = sz;

   write_block_header(brw->intel.aub_file, &bh, data, sz);
}

static void brw_aub_dump_bmp( struct intel_context *intel,
			      GLuint buffer )
{
   struct brw_context *brw = brw_context(&intel->ctx);
   intelScreenPrivate *intelScreen = brw->intel.intelScreen;
   struct aub_dump_bmp db;
   GLuint format;

   if (intelScreen->cpp == 4)
      format = 0x7;
   else
      format = 0x3;


   if (buffer == 0) {
      db.instruction_type = AUB_DUMP_BMP;
      db.xmin = 0;
      db.ymin = 0;
      db.format = format;
      db.bpp = intelScreen->cpp * 8;
      db.pitch = intelScreen->front.pitch / intelScreen->cpp;
      db.xsize = intelScreen->width;
      db.ysize = intelScreen->height;
      db.addr = intelScreen->front.offset;
      db.unknown = 0x0;		/* 4: xmajor tiled, 0: not tiled */

      write_dump_bmp(brw->intel.aub_file, &db);
   }
   else {
      db.instruction_type = AUB_DUMP_BMP;
      db.xmin = 0;
      db.ymin = 0;
      db.format = format;
      db.bpp = intel->back_region->cpp * 8;
      db.pitch = intel->back_region->pitch;
      db.xsize = intel->back_region->pitch;
      db.ysize = intel->back_region->height;
      db.addr = intelScreen->back.offset;
      db.unknown = intel->back_region->tiled ? 0x4 : 0x0;

      write_dump_bmp(brw->intel.aub_file, &db);
   }
}

/* Attempt to prevent monster aubfiles by closing and reopening when
 * the state pools wrap.
 */
static void brw_aub_wrap( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);   
   if (intel->aub_file) {
      brw_aub_destroy(brw);
      brw_aub_init(brw);
   }
   brw->wrap = 1;		/* ??? */
}


int brw_aub_init( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;
   intelScreenPrivate *intelScreen = intel->intelScreen;
   char filename[80];
   int val;
   static int i = 0;

   i++;

   if (_mesa_getenv("INTEL_REPLAY"))
      return 0;

   if (_mesa_getenv("INTEL_AUBFILE")) {
      val = snprintf(filename, sizeof(filename), "%s%d.aub", _mesa_getenv("INTEL_AUBFILE"), i%4);
      _mesa_printf("--> Aub file: %s\n", filename);
      brw->intel.aub_file = fopen(filename, "w");
   }
   else if (_mesa_getenv("INTEL_AUB")) {
      val = snprintf(filename, sizeof(filename), "%s.aub", __progname);
      if (val < 0 || val > sizeof(filename)) 
	 strcpy(filename, "default.aub");   
   
      _mesa_printf("--> Aub file: %s\n", filename);
      brw->intel.aub_file = fopen(filename, "w");
   }
   else {
      return 0;
   }

   if (!brw->intel.aub_file) {
      _mesa_printf("couldn't open aubfile\n");
      exit(1);
   }

   brw->intel.vtbl.aub_commands = brw_aub_gtt_cmds;
   brw->intel.vtbl.aub_dump_bmp = brw_aub_dump_bmp;
   brw->intel.vtbl.aub_gtt_data = brw_aub_gtt_data;
   brw->intel.vtbl.aub_wrap = brw_aub_wrap;
   
   init_aubfile(brw->intel.aub_file);

   /* The GTT is located starting address zero in main memory.  Pages
    * to populate the gtt start after this point.
    */
   brw->next_free_page = (NR_GTT_ENTRIES * 4 + 4095) & ~4095;

   /* More or less correspond with all the agp regions mapped by the
    * driver:
    */
   init_aub_gtt(brw, 0, 4096*4); /* so new fulsim doesn't crash */
   init_aub_gtt(brw, intelScreen->front.offset, intelScreen->back.size);
   init_aub_gtt(brw, intelScreen->back.offset, intelScreen->back.size);
   init_aub_gtt(brw, intelScreen->depth.offset, intelScreen->back.size);
   init_aub_gtt(brw, intelScreen->tex.offset, intelScreen->tex.size);

   return 0;
}

void brw_aub_destroy( struct brw_context *brw )
{
   if (brw->intel.aub_file) {
      fclose(brw->intel.aub_file);
      brw->intel.aub_file = NULL;
   }
}
