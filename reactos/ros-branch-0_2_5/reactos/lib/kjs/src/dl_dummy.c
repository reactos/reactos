/*
 * Dummy dynamic loading functions for systems that do not support dload.
 * Copyright (c) 1998 New Generation Software (NGS) Oy
 *
 * Author: Markku Rossi <mtr@ngs.fi>
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 */

/*
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/dl_dummy.c,v $
 * $Id: dl_dummy.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "jsint.h"

/*
 * Prototypes for static functions.
 */

static void set_error (char *error_return, unsigned int error_return_len);


/*
 * Global functions.
 */

void *
js_dl_open (const char *filename, char *error_return,
	    unsigned int error_return_len)
{
  set_error (error_return, error_return_len);
  return NULL;
}

void *
js_dl_sym (void *library, char *symbol, char *error_return,
	   unsigned int error_return_len)
{
  set_error (error_return, error_return_len);
  return NULL;
}


/*
 * Static functions.
 */

static void
set_error (char *error_return, unsigned int error_return_len)
{
  char msg[512];
  unsigned int msg_len;

  sprintf (msg, "dynamic loading is not supported on %s", CANONICAL_HOST);
  msg_len = strlen (msg);
  if (msg_len > error_return_len - 1)
    msg_len = error_return_len - 1;

  memcpy (error_return, msg, msg_len);
  error_return[msg_len] = '\0';
}
