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

#define GGI_SYMNAME_PREFIX  "MesaGGIdl_"

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

/* FIXME: These should really be defined in the make system */
#define CONF_FILE "/usr/local/etc/ggi/mesa/targets/fbdev.conf"
void *_configHandle;
char confstub[512] = CONF_FILE;
char *conffile = confstub;

static int changed(ggi_visual_t vis, int whatchanged)
{
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
				fname = ggMatchConfig(_configHandle, api, NULL);
				if (fname == NULL)
				{
					/* No special implementation for this sublib */
					continue;
				}
				
				lib = ggiExtensionLoadDL(vis, fname, args, NULL, GGI_SYMNAME_PREFIX);
			}
		}
		break;
	}
	return 0;
}

int GGIdlinit(ggi_visual *vis, const char *args, void *argptr)
{
	struct fbdev_priv_mesa *priv;
	int err;
	ggifunc_getapi *oldgetapi;

	GGIMESA_PRIVATE(vis) = priv = malloc(sizeof(struct fbdev_priv_mesa));
	if (priv == NULL) {
		fprintf(stderr, "Failed to allocate fbdev private data\n");
		return GGI_DL_ERROR;
	}
	
	priv->oldpriv = LIBGGI_PRIVATE(vis);  /* Hook back */
	
	err = ggLoadConfig(conffile, &_configHandle);
	if (err != GGI_OK)
	{
		GGIMESADPRINT_CORE("display-fbdev: Couldn't open %s\n", conffile);
		return err;
	}
	
	LIBGGI_MESAEXT(vis)->update_state = NULL;
	LIBGGI_MESAEXT(vis)->setup_driver = NULL;
	
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

	return 0;
}

int GGIdlcleanup(ggi_visual *vis)
{
	return 0;
}

#include <ggi/internal/ggidlinit.h>
