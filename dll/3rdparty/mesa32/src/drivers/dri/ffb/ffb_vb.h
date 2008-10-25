/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_vb.h,v 1.2 2002/02/22 21:32:59 dawes Exp $ */

#ifndef _FFB_VB_H
#define _FFB_VB_H

#include "mtypes.h"
#include "macros.h"
#include "tnl/t_context.h"
#include "swrast/swrast.h"

#define __FFB_2_30_FIXED_SCALE		1073741824.0f
#define FFB_2_30_FLOAT_TO_FIXED(X)	\
	(IROUND((X) * fmesa->ffb_2_30_fixed_scale))
#define FFB_2_30_FIXED_TO_FLOAT(X)	\
	(((GLfloat)(X)) * fmesa->ffb_one_over_2_30_fixed_scale)

#define __FFB_16_16_FIXED_SCALE		65536.0f
#define FFB_16_16_FLOAT_TO_FIXED(X)	\
	(IROUND((X) * fmesa->ffb_16_16_fixed_scale))
#define FFB_16_16_FIXED_TO_FLOAT(X)	\
	(((GLfloat)(X)) * fmesa->ffb_one_over_16_16_fixed_scale)

#define FFB_Z_FROM_FLOAT(VAL)	  FFB_2_30_FLOAT_TO_FIXED(VAL)
#define FFB_Z_TO_FLOAT(VAL)	  FFB_2_30_FIXED_TO_FLOAT(VAL)
#define FFB_XY_FROM_FLOAT(VAL)	  FFB_16_16_FLOAT_TO_FIXED(VAL)
#define FFB_XY_TO_FLOAT(VAL)	  FFB_16_16_FIXED_TO_FLOAT(VAL)

#define FFB_UBYTE_FROM_COLOR(VAL) ((IROUND((VAL) * fmesa->ffb_ubyte_color_scale)))

#define FFB_PACK_CONST_UBYTE_ARGB_COLOR(C) 		\
        ((FFB_UBYTE_FROM_COLOR(C.alpha) << 24) |	\
         (FFB_UBYTE_FROM_COLOR(C.blue)  << 16) |	\
         (FFB_UBYTE_FROM_COLOR(C.green) <<  8) |	\
         (FFB_UBYTE_FROM_COLOR(C.red)   <<  0))

#define FFB_COLOR_FROM_FLOAT(VAL) FFB_2_30_FLOAT_TO_FIXED(VAL)

#define _FFB_NEW_VERTEX (_DD_NEW_TRI_LIGHT_TWOSIDE)

extern void ffbDDSetupInit(void);
extern void ffbChooseVertexState(GLcontext *);
extern void ffbInitVB( GLcontext *ctx );
extern void ffbFreeVB( GLcontext *ctx );

#endif /* !(_FFB_VB_H) */
