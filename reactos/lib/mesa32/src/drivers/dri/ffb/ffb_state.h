/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_state.h,v 1.2 2002/02/22 21:32:59 dawes Exp $ */

#ifndef _FFB_STATE_H
#define _FFB_STATE_H

extern void ffbDDInitStateFuncs(GLcontext *);
extern void ffbDDInitContextHwState(GLcontext *);

extern void ffbCalcViewport(GLcontext *);
extern void ffbXformAreaPattern(ffbContextPtr, const GLubyte *);
extern void ffbSyncHardware(ffbContextPtr fmesa);

#endif /* !(_FFB_STATE_H) */
