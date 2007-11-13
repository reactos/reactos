/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#ifndef S3VVB_INC
#define S3VVB_INC

#include "mtypes.h"
#include "swrast/swrast.h"

#define _S3V_NEW_VERTEX (_NEW_TEXTURE |		\
			   _DD_NEW_TRI_UNFILLED |	\
			   _DD_NEW_TRI_LIGHT_TWOSIDE)


extern void s3vChooseVertexState( GLcontext *ctx );
extern void s3vCheckTexSizes( GLcontext *ctx );
extern void s3vBuildVertices( GLcontext *ctx, 
				GLuint start, 
				GLuint count,
				GLuint newinputs );


extern void s3v_import_float_colors( GLcontext *ctx );
extern void s3v_import_float_spec_colors( GLcontext *ctx );

extern void s3v_translate_vertex( GLcontext *ctx, 
				    const s3vVertex *src, 
				    SWvertex *dst );

extern void s3vInitVB( GLcontext *ctx );
extern void s3vFreeVB( GLcontext *ctx );

extern void s3v_print_vertex( GLcontext *ctx, const s3vVertex *v );
#if 0
extern void s3vPrintSetupFlags(char *msg, GLuint flags );
#endif

#endif
