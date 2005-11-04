/* $Id: genkgi_mode.c,v 1.4 2000/01/07 08:34:44 jtaylor Exp $
******************************************************************************

   display-fbdev-kgicon-generic-mesa

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

#include <ggi/internal/ggi-dl.h>
#include <ggi/mesa/ggimesa_int.h>
#include <ggi/mesa/debug.h>
#include "genkgi.h"

int GGIMesa_genkgi_getapi(ggi_visual *vis, int num, char *apiname, char *arguments)
{
	struct genkgi_priv_mesa *priv = GENKGI_PRIV_MESA(vis);
	
	GGIMESADPRINT_CORE("Entered mesa_genkgi_getapi, num=%d\n", num);
	
	strcpy(arguments, "");

	switch(num) 
	{
		case 0:
		if (priv->have_accel)
		{
			strcpy(apiname, priv->accel);
			return 0;
		}
		break;
	}
	return -1;
}

int GGIMesa_genkgi_flush(ggi_visual *vis, int x, int y, int w, int h, int tryflag)
{
	struct genkgi_priv_mesa *priv = GENKGI_PRIV_MESA(vis);
	int junkval; 

	priv->oldpriv->kgicommand_ptr += getpagesize(); 
	(kgiu32)(priv->oldpriv->kgicommand_ptr) &= 0xfffff000;
	junkval = *((int *)(priv->oldpriv->kgicommand_ptr));
	
	/* Check if we are now in the last page, and reset the
	 * FIFO if so.  We can't use the last page to send
	 * more commands, since there's no page after it that
	 * we can touch to fault in the last page's commands.
	 * 
	 * FIXME: This will be replaced with a flush-and-reset handler
	 * on the end-of-buffer pagefault at some point....
	 * 
	 */
	if ((priv->oldpriv->kgicommand_ptr - priv->oldpriv->mapped_kgicommand)
	    >= (priv->oldpriv->kgicommand_buffersize - getpagesize()))
	{
		munmap(priv->oldpriv->mapped_kgicommand, priv->oldpriv->kgicommand_buffersize);
		if ((priv->oldpriv->mapped_kgicommand = 
		     mmap(NULL, 
			  priv->oldpriv->kgicommand_buffersize, 
			  PROT_READ | PROT_WRITE,
			  MAP_SHARED,
			  priv->oldpriv->fd_kgicommand, 
			  0)) == MAP_FAILED)
		{
			ggiPanic("Failed to remap kgicommand!");
		}
		priv->oldpriv->kgicommand_ptr = priv->oldpriv->mapped_kgicommand;
	}
	return 0;
}
