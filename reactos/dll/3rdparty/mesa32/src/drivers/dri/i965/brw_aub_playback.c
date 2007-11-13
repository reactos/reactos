
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "brw_aub.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "intel_ioctl.h"
#include "bufmgr.h"

struct aub_state {
   struct intel_context *intel;
   const char *map;
   unsigned int csr;
   unsigned int sz;
};


static int gobble( struct aub_state *s, int size )
{
   if (s->csr + size > s->sz) {
      _mesa_printf("EOF in %s\n", __FUNCTION__);
      return 1;
   }

   s->csr += size;
   return 0;
}

static void flush_and_fence( struct aub_state *s )
{
   struct intel_context *intel = s->intel;
   GLuint buf[2];

   buf[0] = intel->vtbl.flush_cmd();
   buf[1] = 0;

   intel_cmd_ioctl(intel, (char *)&buf, sizeof(buf));
      
   intelWaitIrq( intel, intelEmitIrqLocked( intel ));
}

static void flush_cmds( struct aub_state *s,
			const void *data,
			int len )
{
   DBG("%s %d\n", __FUNCTION__, len);

   if (len & 0x4) {
      unsigned int *tmp = malloc(len + 4);
      DBG("padding to octword\n");
      memcpy(tmp, data, len);
      tmp[len/4] = MI_NOOP;
      flush_cmds(s, tmp, len+4);
      free(tmp);
      return;
   }

   /* For ring data, just send off immediately via an ioctl.
    * This differs slightly from how the stream was executed
    * initially as this would have been a batchbuffer.
    */
   intel_cmd_ioctl(s->intel, (void *)data, len);

   if (1)
      flush_and_fence(s);
}

static const char *pstrings[] = {
   "none",
   "POINTLIST",
   "LINELIST",
   "LINESTRIP",
   "TRILIST",
   "TRISTRIP",
   "TRIFAN",
   "QUADLIST",
   "QUADSTRIP",
   "LINELIST_ADJ",
   "LINESTRIP_ADJ",
   "TRILIST_ADJ",
   "TRISTRIP_ADJ",
   "TRISTRIP_REVERSE",
   "POLYGON",
   "RECTLIST",
   "LINELOOP",
   "POINTLIST_BF",
   "LINESTRIP_CONT",
   "LINESTRIP_BF",
   "LINESTRIP_CONT_BF",
   "TRIFAN_NOSTIPPLE",
};

static void do_3d_prim( struct aub_state *s,
			const void *data,
			int len )
{
   struct brw_3d_primitive prim;
   const struct brw_3d_primitive *orig = data;
   int i;

   assert(len == sizeof(prim));
   memcpy(&prim, data, sizeof(prim));

#define START 0
#define BLOCK (12*28)

   if (orig->verts_per_instance < BLOCK)
      flush_cmds(s, &prim, sizeof(prim));
   else {
      for (i = START; i + BLOCK < orig->verts_per_instance; i += BLOCK/2) {
	 prim.start_vert_location = i;
	 prim.verts_per_instance = BLOCK;
	 _mesa_printf("%sprim %d/%s verts %d..%d (of %d)\n", 
		      prim.header.indexed ? "INDEXED " : "",
		      prim.header.topology, pstrings[prim.header.topology%16],
		      prim.start_vert_location, 
		      prim.start_vert_location + prim.verts_per_instance,
		      orig->verts_per_instance);
	 flush_cmds(s, &prim, sizeof(prim));
      }
   }
}



static struct {
   int cmd;
   const char *name;
   int has_length;
} cmd_info[] = {
   { 0, "NOOP", 0 },
   { 0x5410, "XY_COLOR_BLT_RGB", 1 },
   { 0x5430, "XY_COLOR_BLT_RGBA", 1 },
   { 0x54d0, "XY_SRC_COPY_BLT_RGB", 1 },
   { 0x54f0, "XY_SRC_COPY_BLT_RGBA", 1 },
   { CMD_URB_FENCE, "URB_FENCE",  1 },
   { CMD_CONST_BUFFER_STATE, "CONST_BUFFER_STATE",  1 },
   { CMD_CONST_BUFFER, "CONST_BUFFER",  1 },
   { CMD_STATE_BASE_ADDRESS, "STATE_BASE_ADDRESS",  1 },
   { CMD_STATE_INSN_POINTER, "STATE_INSN_POINTER",  1 },
   { CMD_PIPELINE_SELECT, "PIPELINE_SELECT", 0, },
   { CMD_PIPELINED_STATE_POINTERS, "PIPELINED_STATE_POINTERS", 1 },
   { CMD_BINDING_TABLE_PTRS, "BINDING_TABLE_PTRS", 1 },
   { CMD_VERTEX_BUFFER, "VERTEX_BUFFER", 1 },
   { CMD_VERTEX_ELEMENT, "VERTEX_ELEMENT", 1 },
   { CMD_INDEX_BUFFER, "INDEX_BUFFER", 1 },
   { CMD_VF_STATISTICS, "VF_STATISTICS", 0 },
   { CMD_DRAW_RECT, "DRAW_RECT", 1 },
   { CMD_BLEND_CONSTANT_COLOR, "BLEND_CONSTANT_COLOR", 1 },
   { CMD_CHROMA_KEY, "CHROMA_KEY", 1 },
   { CMD_DEPTH_BUFFER, "DEPTH_BUFFER", 1 },
   { CMD_POLY_STIPPLE_OFFSET, "POLY_STIPPLE_OFFSET", 1 },
   { CMD_POLY_STIPPLE_PATTERN, "POLY_STIPPLE_PATTERN", 1 },
   { CMD_LINE_STIPPLE_PATTERN, "LINE_STIPPLE_PATTERN", 1 },
   { CMD_GLOBAL_DEPTH_OFFSET_CLAMP, "GLOBAL_DEPTH_OFFSET_CLAMP", 1 },
   { CMD_PIPE_CONTROL, "PIPE_CONTROL", 1 },
   { CMD_MI_FLUSH, "MI_FLUSH", 0 },
   { CMD_3D_PRIM, "3D_PRIM", 1 },
};

#define NR_CMDS (sizeof(cmd_info)/sizeof(cmd_info[0]))


static int find_command( unsigned int cmd )
{
   int i;

   for (i = 0; i < NR_CMDS; i++) 
      if (cmd == cmd_info[i].cmd) 
	 return i;

   return -1;
}



static int parse_commands( struct aub_state *s,
			   const unsigned int *data,
			   int len )
{
   while (len) {
      int cmd = data[0] >> 16;
      int dwords;
      int i;

      i = find_command(cmd);

      if (i < 0) {
	 _mesa_printf("couldn't find info for cmd %x\n", cmd);
	 return 1;
      }

      if (cmd_info[i].has_length)
	 dwords = (data[0] & 0xff) + 2;
      else
	 dwords = 1;

      _mesa_printf("%s (%d dwords) 0x%x\n", cmd_info[i].name, dwords, data[0]);

      if (len < dwords * 4) {
	 _mesa_printf("EOF in %s (%d bytes)\n", __FUNCTION__, len);
	 return 1;
      }


      if (0 && cmd == CMD_3D_PRIM)
	 do_3d_prim(s, data, dwords * 4);
      else
	 flush_cmds(s, data, dwords * 4);

      data += dwords;
      len -= dwords * 4;
   }

   return 0;
}



static void parse_data_write( struct aub_state *s,
			     const struct aub_block_header *bh,
			     void *dest,
			     const unsigned int *data,
			     int len )
{
   switch (bh->type) {
   case DW_GENERAL_STATE:
      switch (bh->general_state_type) {
      case DWGS_VERTEX_SHADER_STATE: {
	 struct brw_vs_unit_state vs;
	 assert(len == sizeof(vs));

	 _mesa_printf("DWGS_VERTEX_SHADER_STATE\n");
	 memcpy(&vs, data, sizeof(vs));

/* 	 vs.vs6.vert_cache_disable = 1;  */
/*  	 vs.thread4.max_threads = 4;  */

	 memcpy(dest, &vs, sizeof(vs));
	 return;
      }
      case DWGS_CLIPPER_STATE: {
	 struct brw_clip_unit_state clip;
	 assert(len == sizeof(clip));

	 _mesa_printf("DWGS_CLIPPER_STATE\n");
	 memcpy(&clip, data, sizeof(clip));

/* 	 clip.thread4.max_threads = 0; */
/*   	 clip.clip5.clip_mode = BRW_CLIPMODE_REJECT_ALL;   */

	 memcpy(dest, &clip, sizeof(clip));
	 return;
      }

      case DWGS_NOTYPE:
      case DWGS_GEOMETRY_SHADER_STATE:
      case DWGS_STRIPS_FANS_STATE:
	 break;

      case DWGS_WINDOWER_IZ_STATE: {
	    struct brw_wm_unit_state wm;
	    assert(len == sizeof(wm));

	    _mesa_printf("DWGS_WINDOWER_IZ_STATE\n");
	    memcpy(&wm, data, sizeof(wm));

/* 	    wm.wm5.max_threads = 10; */

	    memcpy(dest, &wm, sizeof(wm));
	    return;
	 }

      case DWGS_COLOR_CALC_STATE:
      case DWGS_CLIPPER_VIEWPORT_STATE:
      case DWGS_STRIPS_FANS_VIEWPORT_STATE:
      case DWGS_COLOR_CALC_VIEWPORT_STATE:
      case DWGS_SAMPLER_STATE:
      case DWGS_KERNEL_INSTRUCTIONS:
      case DWGS_SCRATCH_SPACE:
      case DWGS_SAMPLER_DEFAULT_COLOR:
      case DWGS_INTERFACE_DESCRIPTOR:
      case DWGS_VLD_STATE:
      case DWGS_VFE_STATE:
      default:
	 break;
      }
      break;
   case DW_SURFACE_STATE:
      break;
   case DW_1D_MAP:
   case DW_2D_MAP:
   case DW_CUBE_MAP:
   case DW_VOLUME_MAP:
   case DW_CONSTANT_BUFFER:
   case DW_CONSTANT_URB_ENTRY:
   case DW_VERTEX_BUFFER:
   case DW_INDEX_BUFFER:
   default:
      break;
   }

   memcpy(dest, data, len);
}


/* In order to work, the memory layout has to be the same as the X
 * server which created the aubfile.
 */
static int parse_block_header( struct aub_state *s )
{
   struct aub_block_header *bh = (struct aub_block_header *)(s->map + s->csr);
   void *data = (void *)(bh + 1);
   unsigned int len = (bh->length + 3) & ~3;

   _mesa_printf("block header at 0x%x\n", s->csr);

   if (s->csr + len + sizeof(*bh) > s->sz) {
      _mesa_printf("EOF in data in %s\n", __FUNCTION__);
      return 1;
   }

   if (bh->address_space == ADDR_GTT) {

      switch (bh->operation)
      {
      case BH_DATA_WRITE: {
	 void *dest = bmFindVirtual( s->intel, bh->address, len );
	 if (dest == NULL) {
	    _mesa_printf("Couldn't find virtual address for offset %x\n", bh->address);
	    return 1;
	 }

#if 1
	 parse_data_write(s, bh, dest, data, len);
#else
	 memcpy(dest, data, len);
#endif
	 break;
      }
      case BH_COMMAND_WRITE:
#if 0
	 intel_cmd_ioctl(s->intel, (void *)data, len);
#else
	 if (parse_commands(s, data, len) != 0)
	    _mesa_printf("parse_commands failed\n");
#endif
	 break;
      default:
	 break;
      }
   }

   s->csr += sizeof(*bh) + len;
   return 0;
}


#define AUB_FILE_HEADER 0xe085000b
#define AUB_BLOCK_HEADER 0xe0c10003
#define AUB_DUMP_BMP 0xe09e0004

int brw_playback_aubfile(struct brw_context *brw,
			 const char *filename)
{
   struct intel_context *intel = &brw->intel;
   struct aub_state state;
   struct stat sb;
   int fd;
   int retval = 0;

   state.intel = intel;

   fd = open(filename, O_RDONLY, 0);
   if (fd < 0) {
      _mesa_printf("couldn't open aubfile: %s\n", filename);
      return 1;
   }

   if (fstat(fd, &sb) != 0) {
      _mesa_printf("couldn't open %s\n", filename);
      return 1;
   }

   state.csr = 0;
   state.sz = sb.st_size;
   state.map = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
   
   if (state.map == NULL) {
      _mesa_printf("couldn't mmap %s\n", filename);
      return 1;
   }

   LOCK_HARDWARE(intel); 
   {
      /* Make sure we don't confuse anything that might happen to be
       * going on with the hardware:
       */
/*       bmEvictAll(intel); */
/*       intel->vtbl.lost_hardware(intel); */
      

      /* Replay the aubfile item by item: 
       */
      while (retval == 0 && 
	     state.csr != state.sz) {
	 unsigned int insn = *(unsigned int *)(state.map + state.csr);

	 switch (insn) {
	 case AUB_FILE_HEADER:
	    retval = gobble(&state, sizeof(struct aub_file_header));
	    break;
	 
	 case AUB_BLOCK_HEADER:   
	    retval = parse_block_header(&state);
	    break;
	 
	 case AUB_DUMP_BMP:
	    retval = gobble(&state, sizeof(struct aub_dump_bmp));
	    break;
	 
	 default:
	    _mesa_printf("unknown instruction %x\n", insn);
	    retval = 1;
	    break;
	 }
      }
   }
   UNLOCK_HARDWARE(intel);
   return retval;
}






		  
