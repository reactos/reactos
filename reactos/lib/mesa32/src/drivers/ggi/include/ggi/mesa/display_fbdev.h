#ifndef _GGIMESA_DISPLAY_FBDEV_H
#define _GGIMESA_DISPLAY_FBDEV_H

#include <ggi/internal/ggi-dl.h>
#include <ggi/display/fbdev.h>

ggifunc_setmode GGIMesa_fbdev_setmode;
ggifunc_getapi GGIMesa_fbdev_getapi;

#define FBDEV_PRIV_MESA(vis) ((struct fbdev_priv_mesa *)(FBDEV_PRIV(vis)->accelpriv))

struct fbdev_priv_mesa
{
	char *accel;
	int have_accel;
	void *accelpriv;
	ggi_fbdev_priv *oldpriv;	/* Hooks back to the LibGGI fbdev target's private data */
};

#endif /* _GGIMESA_DISPLAY_FBDEV_H */
