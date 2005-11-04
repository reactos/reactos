/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include "glheader.h"
#include "mtypes.h"
#include "macros.h"
#include "colormac.h"

#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"
#include "tnl/tnl.h"

#include "s3v_context.h"
#include "s3v_vb.h"
#include "s3v_tris.h"

#define S3V_XYZW_BIT       0x1
#define S3V_RGBA_BIT       0x2
#define S3V_TEX0_BIT       0x4
#define S3V_PTEX_BIT       0x8
#define S3V_FOG_BIT        0x10
#define S3V_MAX_SETUP      0x20

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void *, GLuint );
   tnl_interp_func	interp;
   tnl_copy_pv_func	copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_stride_shift;
   GLuint               vertex_format;
} setup_tab[S3V_MAX_SETUP];


/* Only one vertex format, atm, so no need to give them names:
 */
#define TINY_VERTEX_FORMAT      1
#define NOTEX_VERTEX_FORMAT     0
#define TEX0_VERTEX_FORMAT      0
#define TEX1_VERTEX_FORMAT      0
#define PROJ_TEX1_VERTEX_FORMAT 0
#define TEX2_VERTEX_FORMAT      0
#define TEX3_VERTEX_FORMAT      0
#define PROJ_TEX3_VERTEX_FORMAT 0

#define DO_XYZW (IND & S3V_XYZW_BIT)
#define DO_RGBA (IND & S3V_RGBA_BIT)
#define DO_SPEC 0
#define DO_FOG  (IND & S3V_FOG_BIT)
#define DO_TEX0 (IND & S3V_TEX0_BIT)
#define DO_TEX1 0
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & S3V_PTEX_BIT)
			       
#define VERTEX s3vVertex
#define LOCALVARS /* s3vContextPtr vmesa = S3V_CONTEXT(ctx); */
#define GET_VIEWPORT_MAT() 0 /* vmesa->hw_viewport */
#define GET_TEXSOURCE(n)  n
#define GET_VERTEX_FORMAT() 0
#define GET_VERTEX_SIZE() S3V_CONTEXT(ctx)->vertex_size * sizeof(GLuint)
#define GET_VERTEX_STORE() S3V_CONTEXT(ctx)->verts
#define GET_VERTEX_STRIDE_SHIFT() S3V_CONTEXT(ctx)->vertex_stride_shift
#define INVALIDATE_STORED_VERTICES()
#define GET_UBYTE_COLOR_STORE() &S3V_CONTEXT(ctx)->UbyteColor
#define GET_UBYTE_SPEC_COLOR_STORE() &S3V_CONTEXT(ctx)->UbyteSecondaryColor

#define HAVE_HW_VIEWPORT    1	/* FIXME */
#define HAVE_HW_DIVIDE      1
#define HAVE_RGBA_COLOR     0 	/* we're BGRA */
#define HAVE_TINY_VERTICES  1
#define HAVE_NOTEX_VERTICES 0
#define HAVE_TEX0_VERTICES  0
#define HAVE_TEX1_VERTICES  0
#define HAVE_TEX2_VERTICES  0
#define HAVE_TEX3_VERTICES  0
#define HAVE_PTEX_VERTICES  1

/*
#define SUBPIXEL_X -.5
#define SUBPIXEL_Y -.5
#define UNVIEWPORT_VARS  GLfloat h = S3V_CONTEXT(ctx)->driDrawable->h
#define UNVIEWPORT_X(x)  x - SUBPIXEL_X
#define UNVIEWPORT_Y(y)  - y + h + SUBPIXEL_Y
#define UNVIEWPORT_Z(z)  z / vmesa->depth_scale
*/

#define PTEX_FALLBACK()		/* never needed */

#define IMPORT_QUALIFIER
#define IMPORT_FLOAT_COLORS s3v_import_float_colors
#define IMPORT_FLOAT_SPEC_COLORS s3v_import_float_spec_colors

#define INTERP_VERTEX setup_tab[S3V_CONTEXT(ctx)->SetupIndex].interp
#define COPY_PV_VERTEX setup_tab[S3V_CONTEXT(ctx)->SetupIndex].copy_pv



/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#define TAG(x) s3v_##x
#include "tnl_dd/t_dd_vb.c"

/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/


#define IND (S3V_XYZW_BIT|S3V_RGBA_BIT)
#define TAG(x) x##_wg
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (S3V_XYZW_BIT|S3V_RGBA_BIT|S3V_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (S3V_XYZW_BIT|S3V_RGBA_BIT|S3V_TEX0_BIT|S3V_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (S3V_TEX0_BIT)
#define TAG(x) x##_t0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (S3V_RGBA_BIT)
#define TAG(x) x##_g
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (S3V_RGBA_BIT|S3V_TEX0_BIT)
#define TAG(x) x##_gt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (S3V_XYZW_BIT|S3V_RGBA_BIT|S3V_FOG_BIT)
#define TAG(x) x##_wgf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (S3V_XYZW_BIT|S3V_RGBA_BIT|S3V_FOG_BIT|S3V_TEX0_BIT)
#define TAG(x) x##_wgft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (S3V_XYZW_BIT|S3V_RGBA_BIT|S3V_FOG_BIT|S3V_TEX0_BIT|S3V_PTEX_BIT)
#define TAG(x) x##_wgfpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (S3V_FOG_BIT)
#define TAG(x) x##_f
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (S3V_RGBA_BIT | S3V_FOG_BIT)
#define TAG(x) x##_gf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (S3V_RGBA_BIT | S3V_FOG_BIT | S3V_TEX0_BIT)
#define TAG(x) x##_gft0
#include "tnl_dd/t_dd_vbtmp.h"

static void init_setup_tab( void )
{
   init_wg();		/* pos + col */
   init_wgt0();		/* pos + col + tex0 */
   init_wgpt0();	/* pos + col + p-tex0 (?) */
   init_t0();		/* tex0 */
   init_g();		/* col */
   init_gt0();		/* col + tex */
   init_wgf();
   init_wgft0();
   init_wgfpt0();
   init_f();
   init_gf();
   init_gft0();
}


#if 0
void s3vPrintSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s(%x): %s%s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & S3V_XYZW_BIT)      ? " xyzw," : "", 
	   (flags & S3V_RGBA_BIT)     ? " rgba," : "",
	   (flags & S3V_SPEC_BIT)     ? " spec," : "",
	   (flags & S3V_FOG_BIT)      ? " fog," : "",
	   (flags & S3V_TEX0_BIT)     ? " tex-0," : "",
	   (flags & S3V_TEX1_BIT)     ? " tex-1," : "");
}
#endif


void s3vCheckTexSizes( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   s3vContextPtr vmesa = S3V_CONTEXT( ctx );

   if (!setup_tab[vmesa->SetupIndex].check_tex_sizes(ctx)) {

      vmesa->SetupIndex |= (S3V_PTEX_BIT|S3V_RGBA_BIT);

      if (1 || !(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
         tnl->Driver.Render.Interp = setup_tab[vmesa->SetupIndex].interp;
         tnl->Driver.Render.CopyPV = setup_tab[vmesa->SetupIndex].copy_pv;
      }
   }
}

void s3vBuildVertices( GLcontext *ctx, 
			 GLuint start, 
			 GLuint count,
			 GLuint newinputs )
{
	s3vContextPtr vmesa = S3V_CONTEXT( ctx );
	GLubyte *v = ((GLubyte *)vmesa->verts +
		(start<<vmesa->vertex_stride_shift));
	GLuint stride = 1<<vmesa->vertex_stride_shift;

	DEBUG(("*** s3vBuildVertices ***\n"));
	DEBUG(("vmesa->SetupNewInputs = 0x%x\n", vmesa->SetupNewInputs));
	DEBUG(("vmesa->SetupIndex = 0x%x\n", vmesa->SetupIndex));

#if 1
	setup_tab[vmesa->SetupIndex].emit( ctx, start, count, v, stride );
#else
	newinputs |= vmesa->SetupNewInputs;
	vmesa->SetupNewInputs = 0;

	DEBUG(("newinputs is 0x%x\n", newinputs));

	if (!newinputs) {
		DEBUG(("!newinputs\n"));
		return;
	}

	if (newinputs & VERT_CLIP) {
	setup_tab[vmesa->SetupIndex].emit( ctx, start, count, v, stride );
	DEBUG(("newinputs & VERT_CLIP\n"));
	return;
	} /* else { */
/*      GLuint ind = 0; */

	if (newinputs & VERT_RGBA) {
		DEBUG(("newinputs & VERT_RGBA\n"));
		ind |= S3V_RGBA_BIT;
	}
 
	if (newinputs & VERT_TEX0) {
		DEBUG(("newinputs & VERT_TEX0\n"));
		ind |= S3V_TEX0_BIT;
	}

    if (newinputs & VERT_FOG_COORD)
        ind |= S3V_FOG_BIT;

	if (vmesa->SetupIndex & S3V_PTEX_BIT)
		ind = ~0;

    ind &= vmesa->SetupIndex;

	DEBUG(("vmesa->SetupIndex = 0x%x\n", vmesa->SetupIndex));
	DEBUG(("ind = 0x%x\n", ind));
	DEBUG(("ind & vmesa->SetupIndex = 0x%x\n", (ind & vmesa->SetupIndex)));

	if (ind) {
		setup_tab[ind].emit( ctx, start, count, v, stride );   
	}
#endif
}

void s3vChooseVertexState( GLcontext *ctx )
{
   s3vContextPtr vmesa = S3V_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   GLuint ind = S3V_XYZW_BIT | S3V_RGBA_BIT;

   /* FIXME: will segv in tnl_dd/t_dd_vbtmp.h (line 196) on some demos */
/*
   if (ctx->Fog.Enabled)
      ind |= S3V_FOG_BIT;
*/


   if (ctx->Texture.Unit[0]._ReallyEnabled) {
      _tnl_need_projected_coords( ctx, GL_FALSE );
      ind |= S3V_TEX0_BIT;
   } else {
      _tnl_need_projected_coords( ctx, GL_TRUE );
   }

   vmesa->SetupIndex = ind;

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = s3v_interp_extras;
      tnl->Driver.Render.CopyPV = s3v_copy_pv_extras;
   } else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }
}


void s3vInitVB( GLcontext *ctx )
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;

   vmesa->verts = (char *)ALIGN_MALLOC(size * 4 * 16, 32);

   {
      static int firsttime = 1;
      if (firsttime) {
	 init_setup_tab();
	 firsttime = 0;
	 vmesa->vertex_stride_shift = 6 /* 4 */; /* FIXME - only one vertex setup */
      }
   }
}


void s3vFreeVB( GLcontext *ctx )
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);
   if (vmesa->verts) {
      ALIGN_FREE(vmesa->verts);
      vmesa->verts = 0;
   }

   if (vmesa->UbyteSecondaryColor.Ptr) {
      ALIGN_FREE((void *)vmesa->UbyteSecondaryColor.Ptr);
      vmesa->UbyteSecondaryColor.Ptr = 0;
   }

   if (vmesa->UbyteColor.Ptr) {
      ALIGN_FREE((void *)vmesa->UbyteColor.Ptr);
      vmesa->UbyteColor.Ptr = 0;
   }
}
