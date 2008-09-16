/******************************************************************************

   display-fbdev-mesa: visual handling

   Copyright (C) 1999 Jon Taylor [taylorj@ggi-project.org]

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************
*/

#include <ggi/internal/ggi-dl.h>
#include <ggi/mesa/ggimesa_int.h>
#include <ggi/mesa/display_fbdev.h>
#include <ggi/mesa/debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


#ifdef HAVE_SYS_VT_H
#include <sys/vt.h>
#else
#include <linux/vt.h>
#endif
#ifdef HAVE_LINUX_KDEV_T_H
#include <linux/kdev_t.h>
#endif
#include <linux/tty.h>

#define MAX_DEV_LEN	63
#define DEFAULT_FBNUM	0

static char accel_prefix[] = "tgt-fbdev-";
#define PREFIX_LEN	(sizeof(accel_prefix))

typedef struct {
	int   async;
	char *str;
} accel_info;

static accel_info accel_strings[] = {
	{ 0, "kgicon-generic",},	/* no accel - check for KGIcon	*/
	{ 0, NULL },			/* Atari Blitter		*/
};

#define NUM_ACCELS	(sizeof(accel_strings)/sizeof(accel_info))



static int GGIopen(ggi_visual *vis, struct ggi_dlhandle *dlh,
                   const char *args, void *argptr, uint32 *dlret)
{
	int err;
	struct fbdev_priv_mesa *priv;
	ggifunc_getapi *oldgetapi;


	priv->oldpriv = LIBGGI_PRIVATE(vis);  /* Hook back */

	GGIMESA_PRIV(vis) = priv = malloc(sizeof(struct fbdev_priv_mesa));
	if (priv == NULL) {
		fprintf(stderr, "GGIMesa: Failed to allocate fbdev private data\n");
		return GGI_ENOMEM;
	}
	
	oldgetapi = vis->opdisplay->getapi;
	vis->opdisplay->getapi = GGIMesa_fbdev_getapi;
	changed(vis, GGI_CHG_APILIST);	

	/* If the accel sublibs didn't sucessfuly hook a driver,
	 * back up and keep looking */
	if ((LIBGGI_MESAEXT(vis)->update_state == NULL) ||
	    (LIBGGI_MESAEXT(vis)->setup_driver == NULL))
	{
		vis->opdisplay->getapi = oldgetapi;
	}

	*dlret = GGI_DL_EXTENSION;
	return 0;
}


static int GGIclose(ggi_visual *vis, struct ggi_dlhandle *dlh)
{
	struct fbdev_priv_mesa *priv = GGIMESA_PRIV(vis);

	if (priv) {
		LIBGGI_PRIVATE(vis) = priv->oldpriv;
		free(priv);
	}

	return 0;
}


int MesaGGIdl_fbdev_mesa(int func, void **funcptr)
{
	switch (func) {
	case GGIFUNC_open:
		*funcptr = GGIopen;
		return 0;
	case GGIFUNC_exit:
		*funcptr = NULL;
		return 0;
	case GGIFUNC_close:
		*funcptr = GGIclose;
		return 0;
	default:
		*funcptr = NULL;
	}
        
	return GGI_ENOTFOUND;
}


#include <ggi/internal/ggidlinit.h>
