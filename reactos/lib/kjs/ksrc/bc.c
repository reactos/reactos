/*
 * Byte code handling routines.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/ksrc/bc.c,v $
 * $Id: bc.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#include "jsint.h"

/*
 * Global functions.
 */

JSByteCode *
js_bc_read_file (FILE *fp)
{
  return NULL;
}


JSByteCode *
js_bc_read_data (unsigned char *data, unsigned int datalen)
{
  JSUInt32 ui;
  unsigned int pos = 0;
  int i;
  JSByteCode *bc = NULL;

  if (data[pos] == '#')
    {
      /* Skip the first line. */
      for (; pos < datalen && data[pos] != '\n'; pos++)
	;
      if (pos >= datalen)
	goto format_error;
    }

  if (datalen - pos < 8)
    goto format_error;

  JS_BC_READ_INT32 (data + pos, ui);
  if (ui != JS_BC_FILE_MAGIC)
    goto format_error;
  pos += 4;

  bc = js_calloc (NULL, 1, sizeof (*bc));
  if (bc == NULL)
    return NULL;

  JS_BC_READ_INT32 (data + pos, ui);
  bc->num_sects = (unsigned int) ui;
  pos += 4;

  bc->sects = js_calloc (NULL, bc->num_sects, sizeof (JSBCSect));
  if (bc->sects == NULL)
    {
      js_free (bc);
      return NULL;
    }

  /* Read sections. */
  for (i = 0; i < bc->num_sects; i++)
    {
      if (datalen - pos < 8)
	goto format_error;

      /* Get type. */
      JS_BC_READ_INT32 (data + pos, ui);
      bc->sects[i].type = (int) ui;
      pos += 4;

      /* Get section length. */
      JS_BC_READ_INT32 (data + pos, ui);
      bc->sects[i].length = (unsigned int) ui;
      pos += 4;

      bc->sects[i].data = js_malloc (NULL, bc->sects[i].length + 1
				     /* +1 to avoid zero allocations */);
      if (bc->sects[i].data == NULL)
	{
	  for (i--; i >= 0; i--)
	    js_free (bc->sects[i].data);

	  js_free (bc->sects);
	  js_free (bc);
	  return NULL;
	}

      /* Read section's data. */
      if (datalen - pos < bc->sects[i].length)
	goto format_error;

      memcpy (bc->sects[i].data, data + pos, bc->sects[i].length);
      pos += bc->sects[i].length;
    }

  if (pos != datalen)
    goto format_error;

  return bc;


 format_error:

  if (bc)
    js_bc_free (bc);

  return NULL;
}


void
js_bc_free (JSByteCode *bc)
{
}
