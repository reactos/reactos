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


#ifndef BRWCONTEXT_INC
#define BRWCONTEXT_INC

#include "intel_context.h"
#include "brw_structs.h"
#include "imports.h"


/* Glossary:
 *
 * URB - uniform resource buffer.  A mid-sized buffer which is
 * partitioned between the fixed function units and used for passing
 * values (vertices, primitives, constants) between them.
 *
 * CURBE - constant URB entry.  An urb region (entry) used to hold
 * constant values which the fixed function units can be instructed to
 * preload into the GRF when spawining a thread.
 *
 * VUE - vertex URB entry.  An urb entry holding a vertex and usually
 * a vertex header.  The header contains control information and
 * things like primitive type, Begin/end flags and clip codes.  
 *
 * PUE - primitive URB entry.  An urb entry produced by the setup (SF)
 * unit holding rasterization and interpolation parameters.
 *
 * GRF - general register file.  One of several register files
 * addressable by programmed threads.  The inputs (r0, payload, curbe,
 * urb) of the thread are preloaded to this area before the thread is
 * spawned.  The registers are individually 8 dwords wide and suitable
 * for general usage.  Registers holding thread input values are not
 * special and may be overwritten.
 *
 * MRF - message register file.  Threads communicate (and terminate)
 * by sending messages.  Message parameters are placed in contigous
 * MRF registers.  All program output is via these messages.  URB
 * entries are populated by sending a message to the shared URB
 * function containing the new data, together with a control word,
 * often an unmodified copy of R0.
 *
 * R0 - GRF register 0.  Typically holds control information used when
 * sending messages to other threads.
 *
 * EU or GEN4 EU: The name of the programmable subsystem of the
 * i965 hardware.  Threads are executed by the EU, the registers
 * described above are part of the EU architecture.
 *
 * Fixed function units:
 *
 * CS - Command streamer.  Notional first unit, little software
 * interaction.  Holds the URB entries used for constant data, ie the
 * CURBEs.
 *
 * VF/VS - Vertex Fetch / Vertex Shader.  The fixed function part of
 * this unit is responsible for pulling vertices out of vertex buffers
 * in vram and injecting them into the processing pipe as VUEs.  If
 * enabled, it first passes them to a VS thread which is a good place
 * for the driver to implement any active vertex shader.
 *
 * GS - Geometry Shader.  This corresponds to a new DX10 concept.  If
 * enabled, incoming strips etc are passed to GS threads in individual
 * line/triangle/point units.  The GS thread may perform arbitary
 * computation and emit whatever primtives with whatever vertices it
 * chooses.  This makes GS an excellent place to implement GL's
 * unfilled polygon modes, though of course it is capable of much
 * more.  Additionally, GS is used to translate away primitives not
 * handled by latter units, including Quads and Lineloops.
 *
 * CS - Clipper.  Mesa's clipping algorithms are imported to run on
 * this unit.  The fixed function part performs cliptesting against
 * the 6 fixed clipplanes and makes descisions on whether or not the
 * incoming primitive needs to be passed to a thread for clipping.
 * User clip planes are handled via cooperation with the VS thread.
 *
 * SF - Strips Fans or Setup: Triangles are prepared for
 * rasterization.  Interpolation coefficients are calculated.
 * Flatshading and two-side lighting usually performed here.
 *
 * WM - Windower.  Interpolation of vertex attributes performed here.
 * Fragment shader implemented here.  SIMD aspects of EU taken full
 * advantage of, as pixels are processed in blocks of 16.
 *
 * CC - Color Calculator.  No EU threads associated with this unit.
 * Handles blending and (presumably) depth and stencil testing.
 */

#define BRW_FALLBACK_TEXTURE		 0x1
#define BRW_MAX_CURBE                    (32*16)

struct brw_context;

#define BRW_NEW_URB_FENCE               0x1
#define BRW_NEW_FRAGMENT_PROGRAM        0x2
#define BRW_NEW_VERTEX_PROGRAM          0x4
#define BRW_NEW_INPUT_DIMENSIONS        0x8
#define BRW_NEW_CURBE_OFFSETS           0x10
#define BRW_NEW_REDUCED_PRIMITIVE       0x20
#define BRW_NEW_PRIMITIVE               0x40
#define BRW_NEW_CONTEXT                 0x80
#define BRW_NEW_WM_INPUT_DIMENSIONS     0x100
#define BRW_NEW_INPUT_VARYING           0x200
#define BRW_NEW_TNL_PROGRAM             0x400
#define BRW_NEW_PSP                     0x800
#define BRW_NEW_METAOPS                 0x1000
#define BRW_NEW_FENCE                   0x2000
#define BRW_NEW_LOCK                    0x4000



struct brw_state_flags {
   GLuint mesa;
   GLuint cache;
   GLuint brw;
};

struct brw_vertex_program {
   struct gl_vertex_program program;
   GLuint id;
   GLuint param_state;		/* flags indicating state tracked by params */
};



struct brw_fragment_program {
   struct gl_fragment_program program;
   GLuint id;
   GLuint param_state;		/* flags indicating state tracked by params */
};




/* Data about a particular attempt to compile a program.  Note that
 * there can be many of these, each in a different GL state
 * corresponding to a different brw_wm_prog_key struct, with different
 * compiled programs:
 */
struct brw_wm_prog_data {
   GLuint curb_read_length;
   GLuint urb_read_length;

   GLuint first_curbe_grf;
   GLuint total_grf;
   GLuint total_scratch;

   GLuint nr_params;
   GLboolean error;

   /* Pointer to tracked values (only valid once
    * _mesa_load_state_parameters has been called at runtime).
    */
   const GLfloat *param[BRW_MAX_CURBE];
};

struct brw_sf_prog_data {
   GLuint urb_read_length;
   GLuint total_grf;

   /* Each vertex may have upto 12 attributes, 4 components each,
    * except WPOS which requires only 2.  (11*4 + 2) == 44 ==> 11
    * rows.
    *
    * Actually we use 4 for each, so call it 12 rows.
    */
   GLuint urb_entry_size;
};

struct brw_clip_prog_data {
   GLuint curb_read_length;	/* user planes? */
   GLuint clip_mode;
   GLuint urb_read_length;
   GLuint total_grf;
};

struct brw_gs_prog_data {
   GLuint urb_read_length;
   GLuint total_grf;
};

struct brw_vs_prog_data {
   GLuint curb_read_length;
   GLuint urb_read_length;
   GLuint total_grf;
   GLuint outputs_written;

   GLuint inputs_read;

   /* Used for calculating urb partitions:
    */
   GLuint urb_entry_size;
};


/* Size == 0 if output either not written, or always [0,0,0,1]
 */
struct brw_vs_ouput_sizes {
   GLubyte output_size[VERT_RESULT_MAX];
};


#define BRW_MAX_TEX_UNIT 8
#define BRW_WM_MAX_SURF BRW_MAX_TEX_UNIT + 1

/* Create a fixed sized struct for caching binding tables:
 */
struct brw_surface_binding_table {
   GLuint surf_ss_offset[BRW_WM_MAX_SURF];
};


struct brw_cache;

struct brw_mem_pool {
   struct buffer *buffer;

   GLuint size;
   GLuint offset;		/* offset of first free byte */

   struct brw_context *brw;
};

struct brw_cache_item {
   GLuint hash;
   GLuint key_size;		/* for variable-sized keys */
   const void *key;

   GLuint offset;		/* offset within pool's buffer */
   GLuint data_size;

   struct brw_cache_item *next;
};   



struct brw_cache {
   GLuint id;

   const char *name;

   struct brw_context *brw;
   struct brw_mem_pool *pool;

   struct brw_cache_item **items;
   GLuint size, n_items;
   
   GLuint key_size;		/* for fixed-size keys */
   GLuint aux_size;

   GLuint aub_type;
   GLuint aub_sub_type;
   
   GLuint last_addr;			/* offset of active item */
};



struct brw_state_pointers {
   struct gl_colorbuffer_attrib	*Color;
   struct gl_depthbuffer_attrib	*Depth;
   struct gl_fog_attrib		*Fog;
   struct gl_hint_attrib	*Hint;
   struct gl_light_attrib	*Light;
   struct gl_line_attrib	*Line;
   struct gl_point_attrib	*Point;
   struct gl_polygon_attrib	*Polygon;
   GLuint                       *PolygonStipple;
   struct gl_scissor_attrib	*Scissor;
   struct gl_stencil_attrib	*Stencil;
   struct gl_texture_attrib	*Texture;
   struct gl_transform_attrib	*Transform;
   struct gl_viewport_attrib	*Viewport;
   struct gl_vertex_program_state *VertexProgram; 
   struct gl_fragment_program_state *FragmentProgram;
};

/* Considered adding a member to this struct to document which flags
 * an update might raise so that ordering of the state atoms can be
 * checked or derived at runtime.  Dropped the idea in favor of having
 * a debug mode where the state is monitored for flags which are
 * raised that have already been tested against.
 */
struct brw_tracked_state {
   struct brw_state_flags dirty;
   void (*update)( struct brw_context *brw );
};


enum brw_cache_id {
   BRW_CC_VP,
   BRW_CC_UNIT,
   BRW_WM_PROG,
   BRW_SAMPLER_DEFAULT_COLOR,
   BRW_SAMPLER,
   BRW_WM_UNIT,
   BRW_SF_PROG,
   BRW_SF_VP,
   BRW_SF_UNIT,
   BRW_VS_UNIT,
   BRW_VS_PROG,
   BRW_GS_UNIT,
   BRW_GS_PROG,
   BRW_CLIP_VP,
   BRW_CLIP_UNIT,
   BRW_CLIP_PROG,

   /* These two are in the SS pool:
    */
   BRW_SS_SURFACE,
   BRW_SS_SURF_BIND,

   BRW_MAX_CACHE
};

/* Flags for brw->state.cache.
 */
#define CACHE_NEW_CC_VP                  (1<<BRW_CC_VP)
#define CACHE_NEW_CC_UNIT                (1<<BRW_CC_UNIT)
#define CACHE_NEW_WM_PROG                (1<<BRW_WM_PROG)
#define CACHE_NEW_SAMPLER_DEFAULT_COLOR  (1<<BRW_SAMPLER_DEFAULT_COLOR)
#define CACHE_NEW_SAMPLER                (1<<BRW_SAMPLER)
#define CACHE_NEW_WM_UNIT                (1<<BRW_WM_UNIT)
#define CACHE_NEW_SF_PROG                (1<<BRW_SF_PROG)
#define CACHE_NEW_SF_VP                  (1<<BRW_SF_VP)
#define CACHE_NEW_SF_UNIT                (1<<BRW_SF_UNIT)
#define CACHE_NEW_VS_UNIT                (1<<BRW_VS_UNIT)
#define CACHE_NEW_VS_PROG                (1<<BRW_VS_PROG)
#define CACHE_NEW_GS_UNIT                (1<<BRW_GS_UNIT)
#define CACHE_NEW_GS_PROG                (1<<BRW_GS_PROG)
#define CACHE_NEW_CLIP_VP                (1<<BRW_CLIP_VP)
#define CACHE_NEW_CLIP_UNIT              (1<<BRW_CLIP_UNIT)
#define CACHE_NEW_CLIP_PROG              (1<<BRW_CLIP_PROG)
#define CACHE_NEW_SURFACE                (1<<BRW_SS_SURFACE)
#define CACHE_NEW_SURF_BIND              (1<<BRW_SS_SURF_BIND)




enum brw_mempool_id {
   BRW_GS_POOL,
   BRW_SS_POOL,
   BRW_MAX_POOL
};


struct brw_cached_batch_item {
   struct header *header;
   GLuint sz;
   struct brw_cached_batch_item *next;
};
   


/* Protect against a future where VERT_ATTRIB_MAX > 32.  Wouldn't life
 * be easier if C allowed arrays of packed elements?
 */
#define ATTRIB_BIT_DWORDS  ((VERT_ATTRIB_MAX+31)/32)

struct brw_vertex_element {
   const struct gl_client_array *glarray;

   struct brw_vertex_element_state *vep;

   GLuint index;
   GLuint element_size;
   GLuint count;
   GLuint vbo_rebase_offset;
};



struct brw_vertex_info {
   GLuint varying;  /* varying:1[VERT_ATTRIB_MAX] */
   GLuint sizes[ATTRIB_BIT_DWORDS * 2]; /* sizes:2[VERT_ATTRIB_MAX] */
};




/* Cache for TNL programs.
 */
struct brw_tnl_cache_item {
   GLuint hash;
   void *key;
   void *data;
   struct brw_tnl_cache_item *next;
};

struct brw_tnl_cache {
   struct brw_tnl_cache_item **items;
   GLuint size, n_items;
};



struct brw_context 
{
   struct intel_context intel;
   GLuint primitive;

   GLboolean emit_state_always;
   GLboolean wrap;
   GLboolean tmp_fallback;

   struct {
      struct brw_state_flags dirty;
      struct brw_tracked_state **atoms;
      GLuint nr_atoms;


      struct intel_region *draw_region;
      struct intel_region *depth_region;
   } state;

   struct brw_state_pointers attribs;
   struct brw_mem_pool pool[BRW_MAX_POOL];
   struct brw_cache cache[BRW_MAX_CACHE];
   struct brw_cached_batch_item *cached_batch_items;

   struct {

      /* Arrays with buffer objects to copy non-bufferobj arrays into
       * for upload:
       */
      struct gl_client_array vbo_array[VERT_ATTRIB_MAX];

      struct brw_vertex_element inputs[VERT_ATTRIB_MAX];

#define BRW_NR_UPLOAD_BUFS 17
#define BRW_UPLOAD_INIT_SIZE (128*1024)

      struct {
	 struct gl_buffer_object *vbo[BRW_NR_UPLOAD_BUFS];
	 GLuint buf;
	 GLuint offset;
	 GLuint size;
	 GLuint wrap;
      } upload;

      /* Summary of size and varying of active arrays, so we can check
       * for changes to this state:
       */
      struct brw_vertex_info info;
   } vb;

   struct {
      /* Will be allocated on demand if needed.   
       */
      struct brw_state_pointers attribs;
      struct gl_vertex_program *vp;
      struct gl_fragment_program *fp, *fp_tex;

      struct gl_buffer_object *vbo;

      struct intel_region *saved_draw_region;
      struct intel_region *saved_depth_region;

      GLuint restore_draw_mask;
      struct gl_fragment_program *restore_fp;
      
      GLboolean active;
   } metaops;

   /* Track fixed function t&l in a vertex program:
    */
   struct gl_vertex_program *tnl_program;
   struct brw_tnl_cache tnl_program_cache;

   /* Active vertex program: 
    */
   const struct gl_vertex_program *vertex_program;
   const struct gl_fragment_program *fragment_program;


   /* For populating the gtt:
    */
   GLuint next_free_page;


   /* BRW_NEW_URB_ALLOCATIONS:
    */
   struct {
      GLuint vsize;		/* vertex size plus header in urb registers */
      GLuint csize;		/* constant buffer size in urb registers */
      GLuint sfsize;		/* setup data size in urb registers */

      GLboolean constrained;

      GLuint nr_vs_entries;
      GLuint nr_gs_entries;
      GLuint nr_clip_entries;
      GLuint nr_sf_entries;
      GLuint nr_cs_entries;

/*       GLuint vs_size; */
/*       GLuint gs_size; */
/*       GLuint clip_size; */
/*       GLuint sf_size; */
/*       GLuint cs_size; */

      GLuint vs_start;
      GLuint gs_start;
      GLuint clip_start;
      GLuint sf_start;
      GLuint cs_start;
   } urb;

   
   /* BRW_NEW_CURBE_OFFSETS: 
    */
   struct {
      GLuint wm_start;
      GLuint wm_size;
      GLuint clip_start;
      GLuint clip_size;
      GLuint vs_start;
      GLuint vs_size;
      GLuint total_size;

      /* Dynamic tracker which changes to reflect the state referenced
       * by active fp and vp program parameters:
       */
      struct brw_tracked_state tracked_state;

      GLuint gs_offset;

      GLfloat *last_buf;
      GLuint last_bufsz;
   } curbe;

   struct {
      struct brw_vs_prog_data *prog_data;

      GLuint prog_gs_offset;
      GLuint state_gs_offset;	
   } vs;

   struct {
      struct brw_gs_prog_data *prog_data;

      GLboolean prog_active;
      GLuint prog_gs_offset;
      GLuint state_gs_offset;	
   } gs;

   struct {
      struct brw_clip_prog_data *prog_data;

      GLuint prog_gs_offset;
      GLuint vp_gs_offset;
      GLuint state_gs_offset;	
   } clip;


   struct {
      struct brw_sf_prog_data *prog_data;

      GLuint prog_gs_offset;
      GLuint vp_gs_offset;
      GLuint state_gs_offset;
   } sf;

   struct {
      struct brw_wm_prog_data *prog_data;
      struct brw_wm_compile *compile_data;

      /* Input sizes, calculated from active vertex program:
       */
      GLuint input_size_masks[4];


      /* State structs
       */
      struct brw_sampler_default_color sdc[BRW_MAX_TEX_UNIT];
      struct brw_sampler_state sampler[BRW_MAX_TEX_UNIT];

      GLuint render_surf;
      GLuint nr_surfaces;      

      GLuint max_threads;
      struct buffer *scratch_buffer;
      GLuint scratch_buffer_size;

      GLuint sampler_count;
      GLuint sampler_gs_offset;

      struct brw_surface_binding_table bind;
      GLuint bind_ss_offset;

      GLuint prog_gs_offset;
      GLuint state_gs_offset;
   } wm;


   struct {
      GLuint vp_gs_offset;
      GLuint state_gs_offset;
   } cc;

   
   /* Used to give every program string a unique id
    */
   GLuint program_id;
};


#define BRW_PACKCOLOR8888(r,g,b,a)  ((r<<24) | (g<<16) | (b<<8) | a)



/*======================================================================
 * brw_vtbl.c
 */
void brwInitVtbl( struct brw_context *brw );
void brw_do_flush( struct brw_context *brw, 
		   GLuint flags );

/*======================================================================
 * brw_context.c
 */
GLboolean brwCreateContext( const __GLcontextModes *mesaVis,
			    __DRIcontextPrivate *driContextPriv,
			    void *sharedContextPrivate);



/*======================================================================
 * brw_state.c
 */
void brw_validate_state( struct brw_context *brw );
void brw_init_state( struct brw_context *brw );
void brw_destroy_state( struct brw_context *brw );



/*======================================================================
 * brw_tex.c
 */
void brwUpdateTextureState( struct intel_context *intel );
void brwInitTextureFuncs( struct dd_function_table *functions );
void brw_FrameBufferTexInit( struct brw_context *brw );
void brw_FrameBufferTexDestroy( struct brw_context *brw );

/*======================================================================
 * brw_metaops.c
 */

void brw_init_metaops( struct brw_context *brw );
void brw_destroy_metaops( struct brw_context *brw );


/*======================================================================
 * brw_program.c
 */
void brwInitFragProgFuncs( struct dd_function_table *functions );


/* brw_urb.c
 */
void brw_upload_urb_fence(struct brw_context *brw);

void brw_upload_constant_buffer_state(struct brw_context *brw);


/*======================================================================
 * Inline conversion functions.  These are better-typed than the
 * macros used previously:
 */
static inline struct brw_context *
brw_context( GLcontext *ctx )
{
   return (struct brw_context *)ctx;
}

#endif

