/**************************************************************************

Copyright 2004 Tungsten Graphics Inc., Cedar Park, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Daniel Borca <dborca@yahoo.com>
 */


#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "vtxfmt.h"
#include "dlist.h"
#include "state.h"
#include "light.h"
#include "api_arrayelt.h"
#include "api_noop.h"
#include "t_vtx_api.h"
#include "simple_list.h"


#if defined(USE_X86_ASM) && !defined(HAVE_NONSTANDARD_GLAPIENTRY)

#define EXTERN( FUNC )		\
extern const char FUNC[];	\
extern const char FUNC##_end[]

EXTERN( _tnl_x86_Attribute1fv );
EXTERN( _tnl_x86_Attribute2fv );
EXTERN( _tnl_x86_Attribute3fv );
EXTERN( _tnl_x86_Attribute4fv );
EXTERN( _tnl_x86_Vertex1fv );
EXTERN( _tnl_x86_Vertex2fv );
EXTERN( _tnl_x86_Vertex3fv );
EXTERN( _tnl_x86_Vertex4fv );

EXTERN( _tnl_x86_dispatch_attrf1 );
EXTERN( _tnl_x86_dispatch_attrf2 );
EXTERN( _tnl_x86_dispatch_attrf3 );
EXTERN( _tnl_x86_dispatch_attrf4 );
EXTERN( _tnl_x86_dispatch_attrfv );
EXTERN( _tnl_x86_dispatch_multitexcoordf1 );
EXTERN( _tnl_x86_dispatch_multitexcoordf2 );
EXTERN( _tnl_x86_dispatch_multitexcoordf3 );
EXTERN( _tnl_x86_dispatch_multitexcoordf4 );
EXTERN( _tnl_x86_dispatch_multitexcoordfv );
EXTERN( _tnl_x86_dispatch_vertexattribf1 );
EXTERN( _tnl_x86_dispatch_vertexattribf2 );
EXTERN( _tnl_x86_dispatch_vertexattribf3 );
EXTERN( _tnl_x86_dispatch_vertexattribf4 );
EXTERN( _tnl_x86_dispatch_vertexattribfv );

EXTERN( _tnl_x86_choose_fv );


#define DONT_KNOW_OFFSETS 1


#define DFN( FUNC, CACHE, KEY )				\
   struct _tnl_dynfn *dfn = MALLOC_STRUCT( _tnl_dynfn );\
   const char *start = FUNC;				\
   const char *end = FUNC##_end;			\
   int offset = 0;               			\
   insert_at_head( &CACHE, dfn );			\
   dfn->key = KEY;					\
   dfn->code = ALIGN_MALLOC( end - start, 16 );		\
   memcpy (dfn->code, start, end - start)



#define FIXUP( CODE, KNOWN_OFFSET, CHECKVAL, NEWVAL )	\
do {							\
   GLint subst = 0x10101010 + CHECKVAL;			\
							\
   if (DONT_KNOW_OFFSETS) {				\
      while (*(int *)(CODE+offset) != subst) offset++;	\
      *(int *)(CODE+offset) = (int)(NEWVAL);		\
      if (0) fprintf(stderr, "%s/%d: offset %d, new value: 0x%x\n", __FILE__, __LINE__, offset, (int)(NEWVAL)); \
      offset += 4;					\
   }							\
   else {						\
      int *icode = (int *)(CODE+KNOWN_OFFSET);		\
      assert (*icode == subst);				\
      *icode = (int)NEWVAL;				\
   }							\
} while (0)



#define FIXUPREL( CODE, KNOWN_OFFSET, CHECKVAL, NEWVAL )\
do {							\
   GLint subst = 0x10101010 + CHECKVAL;			\
							\
   if (DONT_KNOW_OFFSETS) {				\
      while (*(int *)(CODE+offset) != subst) offset++;	\
      *(int *)(CODE+offset) = (int)(NEWVAL) - ((int)(CODE)+offset) - 4; \
      if (0) fprintf(stderr, "%s/%d: offset %d, new value: 0x%x\n", __FILE__, __LINE__, offset, (int)(NEWVAL) - ((int)(CODE)+offset) - 4); \
      offset += 4;					\
   }							\
   else {						\
      int *icode = (int *)(CODE+KNOWN_OFFSET);		\
      assert (*icode == subst);				\
      *icode = (int)(NEWVAL) - (int)(icode) - 4;	\
   }							\
} while (0)




/* Build specialized versions of the immediate calls on the fly for
 * the current state.  Generic x86 versions.
 */

static struct _tnl_dynfn *makeX86Vertex1fv( GLcontext *ctx, int vertex_size )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _tnl_x86_Vertex1fv, tnl->vtx.cache.Vertex[1-1], vertex_size );

   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 1, vertex_size - 1);
   FIXUP(dfn->code, 0, 2, (int)&tnl->vtx.vertex[1]);
   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUP(dfn->code, 0, 4, (int)ctx);
   FIXUPREL(dfn->code, 0, 5, (int)&_tnl_wrap_filled_vertex);

   return dfn;
}

static struct _tnl_dynfn *makeX86Vertex2fv( GLcontext *ctx, int vertex_size )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _tnl_x86_Vertex2fv, tnl->vtx.cache.Vertex[2-1], vertex_size );

   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 1, vertex_size - 2);
   FIXUP(dfn->code, 0, 2, (int)&tnl->vtx.vertex[2]);
   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUP(dfn->code, 0, 4, (int)ctx);
   FIXUPREL(dfn->code, 0, 5, (int)&_tnl_wrap_filled_vertex);

   return dfn;
}

static struct _tnl_dynfn *makeX86Vertex3fv( GLcontext *ctx, int vertex_size )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   switch (vertex_size) {
      default: {
         DFN ( _tnl_x86_Vertex3fv, tnl->vtx.cache.Vertex[3-1], vertex_size );

         FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
         FIXUP(dfn->code, 0, 1, vertex_size - 3);
         FIXUP(dfn->code, 0, 2, (int)&tnl->vtx.vertex[3]);
         FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
         FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
         FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
         FIXUP(dfn->code, 0, 4, (int)ctx);
         FIXUPREL(dfn->code, 0, 5, (int)&_tnl_wrap_filled_vertex);
         return dfn;
      }
   }
}

static struct _tnl_dynfn *makeX86Vertex4fv( GLcontext *ctx, int vertex_size )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _tnl_x86_Vertex4fv, tnl->vtx.cache.Vertex[4-1], vertex_size );

   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 1, vertex_size - 4);
   FIXUP(dfn->code, 0, 2, (int)&tnl->vtx.vertex[4]);
   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUP(dfn->code, 0, 4, (int)ctx);
   FIXUPREL(dfn->code, 0, 5, (int)&_tnl_wrap_filled_vertex);

   return dfn;
}


static struct _tnl_dynfn *makeX86Attribute1fv( GLcontext *ctx, int dest )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _tnl_x86_Attribute1fv, tnl->vtx.cache.Attribute[1-1], dest );

   FIXUP(dfn->code, 0, 0, dest);

   return dfn;
}

static struct _tnl_dynfn *makeX86Attribute2fv( GLcontext *ctx, int dest )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _tnl_x86_Attribute2fv, tnl->vtx.cache.Attribute[2-1], dest );

   FIXUP(dfn->code, 0, 0, dest);
   FIXUP(dfn->code, 0, 1, 4+dest);

   return dfn;
}

static struct _tnl_dynfn *makeX86Attribute3fv( GLcontext *ctx, int dest )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _tnl_x86_Attribute3fv, tnl->vtx.cache.Attribute[3-1], dest );

   FIXUP(dfn->code, 0, 0, dest);
   FIXUP(dfn->code, 0, 1, 4+dest);
   FIXUP(dfn->code, 0, 2, 8+dest);

   return dfn;
}

static struct _tnl_dynfn *makeX86Attribute4fv( GLcontext *ctx, int dest )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _tnl_x86_Attribute4fv, tnl->vtx.cache.Attribute[4-1], dest );

   FIXUP(dfn->code, 0, 0, dest);
   FIXUP(dfn->code, 0, 1, 4+dest);
   FIXUP(dfn->code, 0, 2, 8+dest);
   FIXUP(dfn->code, 0, 3, 12+dest);

   return dfn;
}


void _tnl_InitX86Codegen( struct _tnl_dynfn_generators *gen )
{
   gen->Vertex[0] = makeX86Vertex1fv;
   gen->Vertex[1] = makeX86Vertex2fv;
   gen->Vertex[2] = makeX86Vertex3fv;
   gen->Vertex[3] = makeX86Vertex4fv;
   gen->Attribute[0] = makeX86Attribute1fv;
   gen->Attribute[1] = makeX86Attribute2fv;
   gen->Attribute[2] = makeX86Attribute3fv;
   gen->Attribute[3] = makeX86Attribute4fv;
}


#define MKDISP(FUNC, SIZE, ATTR, WARP)					\
do {									\
   char *code;								\
   const char *start = WARP;						\
   const char *end = WARP##_end;					\
   int offset = 0;							\
   code = ALIGN_MALLOC( end - start, 16 );				\
   memcpy (code, start, end - start);					\
   FIXUP(code, 0, 0, (int)&(TNL_CONTEXT(ctx)->vtx.tabfv[ATTR][SIZE-1]));\
   *(void **)&vfmt->FUNC = code;					\
} while (0)


/* Install the codegen'ed versions of the 2nd level dispatch
 * functions.  We should keep a list and free them in the end...
 */
void _tnl_x86_exec_vtxfmt_init( GLcontext *ctx )
{
   GLvertexformat *vfmt = &(TNL_CONTEXT(ctx)->exec_vtxfmt);

   MKDISP(Color3f,             3, _TNL_ATTRIB_COLOR0, _tnl_x86_dispatch_attrf3);
   MKDISP(Color3fv,            3, _TNL_ATTRIB_COLOR0, _tnl_x86_dispatch_attrfv);
   MKDISP(Color4f,             4, _TNL_ATTRIB_COLOR0, _tnl_x86_dispatch_attrf4);
   MKDISP(Color4fv,            4, _TNL_ATTRIB_COLOR0, _tnl_x86_dispatch_attrfv);
   MKDISP(FogCoordfEXT,        1, _TNL_ATTRIB_FOG,    _tnl_x86_dispatch_attrf1);
   MKDISP(FogCoordfvEXT,       1, _TNL_ATTRIB_FOG,    _tnl_x86_dispatch_attrfv);
   MKDISP(Normal3f,            3, _TNL_ATTRIB_NORMAL, _tnl_x86_dispatch_attrf3);
   MKDISP(Normal3fv,           3, _TNL_ATTRIB_NORMAL, _tnl_x86_dispatch_attrfv);
   MKDISP(SecondaryColor3fEXT, 3, _TNL_ATTRIB_COLOR1, _tnl_x86_dispatch_attrf3);
   MKDISP(SecondaryColor3fvEXT,3, _TNL_ATTRIB_COLOR1, _tnl_x86_dispatch_attrfv);
   MKDISP(TexCoord1f,          1, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_attrf1);
   MKDISP(TexCoord1fv,         1, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_attrfv);
   MKDISP(TexCoord2f,          2, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_attrf2);
   MKDISP(TexCoord2fv,         2, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_attrfv);
   MKDISP(TexCoord3f,          3, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_attrf3);
   MKDISP(TexCoord3fv,         3, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_attrfv);
   MKDISP(TexCoord4f,          4, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_attrf4);
   MKDISP(TexCoord4fv,         4, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_attrfv);
   MKDISP(Vertex2f,            2, _TNL_ATTRIB_POS,    _tnl_x86_dispatch_attrf2);
   MKDISP(Vertex2fv,           2, _TNL_ATTRIB_POS,    _tnl_x86_dispatch_attrfv);
   MKDISP(Vertex3f,            3, _TNL_ATTRIB_POS,    _tnl_x86_dispatch_attrf3);
   MKDISP(Vertex3fv,           3, _TNL_ATTRIB_POS,    _tnl_x86_dispatch_attrfv);
   MKDISP(Vertex4f,            4, _TNL_ATTRIB_POS,    _tnl_x86_dispatch_attrf4);
   MKDISP(Vertex4fv,           4, _TNL_ATTRIB_POS,    _tnl_x86_dispatch_attrfv);

   MKDISP(MultiTexCoord1fARB,  1, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_multitexcoordf1);
   MKDISP(MultiTexCoord1fvARB, 1, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_multitexcoordfv);
   MKDISP(MultiTexCoord2fARB,  2, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_multitexcoordf2);
   MKDISP(MultiTexCoord2fvARB, 2, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_multitexcoordfv);
   MKDISP(MultiTexCoord3fARB,  3, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_multitexcoordf3);
   MKDISP(MultiTexCoord3fvARB, 3, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_multitexcoordfv);
   MKDISP(MultiTexCoord4fARB,  4, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_multitexcoordf4);
   MKDISP(MultiTexCoord4fvARB, 4, _TNL_ATTRIB_TEX0,   _tnl_x86_dispatch_multitexcoordfv);

   MKDISP(VertexAttrib1fNV,    1, 0,                  _tnl_x86_dispatch_vertexattribf1);
   MKDISP(VertexAttrib1fvNV,   1, 0,                  _tnl_x86_dispatch_vertexattribfv);
   MKDISP(VertexAttrib2fNV,    2, 0,                  _tnl_x86_dispatch_vertexattribf2);
   MKDISP(VertexAttrib2fvNV,   2, 0,                  _tnl_x86_dispatch_vertexattribfv);
   MKDISP(VertexAttrib3fNV,    3, 0,                  _tnl_x86_dispatch_vertexattribf3);
   MKDISP(VertexAttrib3fvNV,   3, 0,                  _tnl_x86_dispatch_vertexattribfv);
   MKDISP(VertexAttrib4fNV,    4, 0,                  _tnl_x86_dispatch_vertexattribf4);
   MKDISP(VertexAttrib4fvNV,   4, 0,                  _tnl_x86_dispatch_vertexattribfv);
}


/* Install the codegen'ed choosers.
 * We should keep a list and free them in the end...
 */
void _tnl_x86choosers( tnl_attrfv_func (*choose)[4],
		       tnl_attrfv_func (*do_choose)( GLuint attr,
						 GLuint sz ))
{
   int attr, size;

   for (attr = 0; attr < _TNL_MAX_ATTR_CODEGEN; attr++) {
      for (size = 0; size < 4; size++) {
         char *code;
         const char *start = _tnl_x86_choose_fv;
         const char *end = _tnl_x86_choose_fv_end;
         int offset = 0;
         code = ALIGN_MALLOC( end - start, 16 );
         memcpy (code, start, end - start);
         FIXUP(code, 0, 0, attr);
         FIXUP(code, 0, 1, size + 1);
         FIXUPREL(code, 0, 2, do_choose);
         choose[attr][size] = (tnl_attrfv_func)code;
      }
   }
}

#else

void _tnl_InitX86Codegen( struct _tnl_dynfn_generators *gen )
{
   (void) gen;
}


void _tnl_x86_exec_vtxfmt_init( GLcontext *ctx )
{
   (void) ctx;
}


void _tnl_x86choosers( tnl_attrfv_func (*choose)[4],
		       tnl_attrfv_func (*do_choose)( GLuint attr,
						 GLuint sz ))
{
   (void) choose;
   (void) do_choose;
}

#endif
