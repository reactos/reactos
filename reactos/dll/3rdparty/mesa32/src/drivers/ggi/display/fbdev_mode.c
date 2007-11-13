/******************************************************************************

   display-fbdev-mesa

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/fb.h>

#include <ggi/internal/ggi-dl.h>
#include <ggi/mesa/ggimesa_int.h>
#include <ggi/mesa/display_fbdev.h>
#include <ggi/mesa/debug.h>

#ifndef MAP_FAILED
#define MAP_FAILED ((void*)-1)
#endif

#define FB_KLUDGE_FONTX  8
#define FB_KLUDGE_FONTY  16
#define FB_KLUDGE_TEXTMODE  13
#define TIMINGFILE "/etc/fb.modes"

int GGIMesa_fbdev_getapi(ggi_visual *vis, int num, char *apiname, char *arguments)
{
	struct fbdev_priv_mesa *priv = GGIMESA_PRIV(vis);
	
	arguments = '\0';

	switch(num) {
	case 0:
		if (priv->oldpriv->have_accel) {
			strcpy(apiname, priv->oldpriv->accel);
			return 0;
		}
		break;
	}

	return -1;
}

static int do_setmode(ggi_visual *vis)
{
	struct fbdev_priv_mesa *priv = GGIMESA_PRIV(vis);
	int err, id;
	char libname[GGI_API_MAXLEN], libargs[GGI_API_MAXLEN];
	ggi_graphtype gt;

	_ggiZapMode(vis, ~GGI_DL_OPDISPLAY);
	priv->have_accel = 0;

	for (id = 1; GGIMesa_fbdev_getapi(vis, id, libname, libargs) == 0; id++) {
		if (_ggiOpenDL(vis, libname, libargs, NULL) == 0) {
			GGIMESADPRINT_LIBS(stderr, "display-fbdev-mesa: Error opening the "
				"%s (%s) library.\n", libname, libargs);
			return GGI_EFATAL;
		}

		GGIMESADPRINT_CORE("Success in loading %s (%s)\n",
			libname, libargs);
	}

	if (priv->oldpriv->accel &&
	    _ggiOpenDL(vis, priv->accel, NULL, NULL) != 0) {
		priv->have_accel = 1;
	} else {
		priv->have_accel = 0;
	}
	vis->accelactive = 0;

	ggiIndicateChange(vis, GGI_CHG_APILIST);

	GGIMESADPRINT_CORE("display-fbdev-mesa: do_setmode SUCCESS\n");

	return 0;
}


int GGIMesa_fbdev_setmode(ggi_visual *vis, ggi_mode *mode)
{ 
	int err;

        if ((err = ggiCheckMode(vis, mode)) != 0) {
		return err;
	}

	GGIMESADPRINT_CORE("display-fbdev-mesa: setmode %dx%d#%dx%dF%d[0x%02x]\n",
		    mode->visible.x, mode->visible.y,
		    mode->virt.x, mode->virt.y, 
		    mode->frames, mode->graphtype);

	memcpy(LIBGGI_MODE(vis), mode, sizeof(ggi_mode));

	/* Now actually set the mode */
	err = do_setmode(vis);
	if (err != 0) {
		return err;
	}

	GGIMESADPRINT_CORE("display-fbdev-mesa: setmode success.\n");

	return 0;
}
