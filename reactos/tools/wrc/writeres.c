/*
 * Write .res file from a resource-tree
 *
 * Copyright 1998 Bertho A. Stultiens
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wine/unicode.h"
#include "wrc.h"
#include "genres.h"
#include "newstruc.h"
#include "utils.h"

/*
 *****************************************************************************
 * Function	: write_resfile
 * Syntax	: void write_resfile(char *outname, resource_t *top)
 * Input	:
 *	outname	- Filename to write to
 *	top	- The resource-tree to convert
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
void write_resfile(char *outname, resource_t *top)
{
	FILE *fo;
	unsigned int ret;
	char zeros[3] = {0, 0, 0};

	fo = fopen(outname, "wb");
	if(!fo)
	{
		error("Could not open %s\n", outname);
	}

	if(win32)
	{
		/* Put an empty resource first to signal win32 format */
		res_t *res = new_res();
		put_dword(res, 0);		/* ResSize */
		put_dword(res, 0x00000020);	/* HeaderSize */
		put_word(res, 0xffff);		/* ResType */
		put_word(res, 0);
		put_word(res, 0xffff);		/* ResName */
		put_word(res, 0);
		put_dword(res, 0);		/* DataVersion */
		put_word(res, 0);		/* Memory options */
		put_word(res, 0);		/* Language */
		put_dword(res, 0);		/* Version */
		put_dword(res, 0);		/* Characteristics */
		ret = fwrite(res->data, 1, res->size, fo);
		if(ret != res->size)
		{
			fclose(fo);
			error("Error writing %s\n", outname);
		}
		free(res);
	}

	for(; top; top = top->next)
	{
		if(!top->binres)
			continue;

		ret = fwrite(top->binres->data, 1, top->binres->size, fo);
		if(ret != top->binres->size)
		{
			fclose(fo);
			error("Error writing %s\n", outname);
		}
		if(win32 && (top->binres->size & 0x03))
		{
			/* Write padding */
			ret = fwrite(zeros, 1, 4 - (top->binres->size & 0x03), fo);
			if(ret != 4 - (top->binres->size & 0x03))
			{
				fclose(fo);
				error("Error writing %s\n", outname);
			}
		}
	}
	fclose(fo);
}
