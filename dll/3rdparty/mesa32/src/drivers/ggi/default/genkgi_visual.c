/* $Id: genkgi_visual.c,v 1.7 2000/06/11 20:11:55 jtaylor Exp $
******************************************************************************

   genkgi_visual.c: visual handling for the generic KGI helper
   
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
#include "genkgi.h"

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

#define DEFAULT_FBNUM	0

static char accel_prefix[] = "tgt-fbdev-kgicon-";
#define PREFIX_LEN	(sizeof(accel_prefix))

typedef struct {
	int   async;
	char *str;
} accel_info;

static accel_info accel_strings[] = 
{
	{ 0, "d3dim" },		/* Direct3D Immediate Mode     		*/
};

#define NUM_ACCELS	(sizeof(accel_strings)/sizeof(accel_info))

/* FIXME: These should be defined in the makefile system */
#define CONF_FILE "/usr/local/etc/ggi/mesa/targets/genkgi.conf"
void *_configHandle;
char confstub[512] = CONF_FILE;
char *conffile = confstub;

static int changed(ggi_visual_t vis, int whatchanged)
{
	GGIMESADPRINT_CORE("Entered ggimesa_genkgi_changed\n");
	
	switch (whatchanged)
	{
		case GGI_CHG_APILIST:
		{
			char api[256];
			char args[256];
			int i;
			const char *fname;
			ggi_dlhandle *lib;
			
			for (i = 0; ggiGetAPI(vis, i, api, args) == 0; i++)
			{
				strcat(api, "-mesa");
				GGIMESADPRINT_CORE("ggimesa_genkgi_changed: api=%s, i=%d\n", api, i);
				fname = ggMatchConfig(_configHandle, api, NULL);
				if (fname == NULL)
				{
					/* No special implementation for this sublib */
					continue;
				}
				
				lib = ggiExtensionLoadDL(vis, fname, args, NULL);
			}
		} 
		break;
	}
	return 0;
}

static int GGIdlinit(ggi_visual *vis, struct ggi_dlhandle *dlh,
			const char *args, void *argptr, uint32 *dlret)
{
	struct genkgi_priv_mesa *priv;
	char libname[256], libargs[256];
	int id, err;
	struct stat junk;
	ggifunc_getapi *oldgetapi;

	GGIMESADPRINT_CORE("display-fbdev-kgicon-mesa: GGIdlinit start\n");
	
	GENKGI_PRIV_MESA(vis) = priv = malloc(sizeof(struct genkgi_priv_mesa));
	if (priv == NULL) 
	{
		fprintf(stderr, "Failed to allocate genkgi private data\n");
		return GGI_DL_ERROR;
	}
	
	priv->oldpriv = GENKGI_PRIV(vis);
#if 0
	err = ggLoadConfig(conffile, &_configHandle);
	if (err != GGI_OK)
	{
		gl_ggiPrint("display-fbdev-kgicon-mesa: Couldn't open %s\n", conffile);
		return err;
	}

	/* Hack city here.  We need to probe the KGI driver properly for
	 * suggest-strings to discover the acceleration type(s).
	 */
	priv->have_accel = 0;

	if (stat("/proc/gfx0", &junk) == 0)
	{
		sprintf(priv->accel, "%s%s", accel_prefix, "d3dim");
		priv->have_accel = 1;
		GGIMESADPRINT_CORE("display-fbdev-kgicon-mesa: Using accel: \"%s\"\n", priv->accel);
	}

	/* Mode management */
	vis->opdisplay->getapi = GGIMesa_genkgi_getapi;	
	ggiIndicateChange(vis, GGI_CHG_APILIST);
	
	/* Give the accel sublibs a chance to set up a driver */
	if (priv->have_accel == 1)
	{
		oldgetapi = vis->opdisplay->getapi;
		vis->opdisplay->getapi = GGIMesa_genkgi_getapi;
		changed(vis, GGI_CHG_APILIST);
		/* If the accel sublibs didn't produce, back up
		 * and keep looking */
		if ((LIBGGI_MESAEXT(vis)->update_state == NULL) ||
		    (LIBGGI_MESAEXT(vis)->setup_driver == NULL))
		  vis->opdisplay->getapi = oldgetapi;
	}
	
	LIBGGI_MESAEXT(vis)->update_state = genkgi_update_state;
	LIBGGI_MESAEXT(vis)->setup_driver = genkgi_setup_driver;
#endif	
	GGIMESADPRINT_CORE("display-fbdev-kgicon-mesa: GGIdlinit finished\n");

	*dlret = GGI_DL_OPDRAW;
	return 0;
}

int MesaGGIdl_fbdev(int func, void **funcptr)
{
	switch (func) {
		case GGIFUNC_open:
			*funcptr = GGIopen;
			return 0;
		case GGIFUNC_exit:
		case GGIFUNC_close:
			*funcptr = NULL;
			return 0;
		default:
			*funcptr = NULL;
	}
	return GGI_ENOTFOUND;
}

#include <ggi/internal/ggidlinit.h>
