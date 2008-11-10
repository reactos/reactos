/*
 * Read a .res file and create a resource-tree
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

#include "wrc.h"
#include "readres.h"
#include "newstruc.h"
#include "utils.h"
#include "genres.h"

static const struct resheader32 {
	DWORD	ressize;	/* 0 */
	DWORD	hdrsize;	/* 0x20 */
	WORD	restype1;	/* 0xffff */
	WORD	restype2;	/* 0 */
	WORD	resname1;	/* 0xffff */
	WORD	resname2;	/* 0 */
	DWORD	dversion;	/* 0 */
	WORD	memopt;		/* 0 */
	WORD	language;	/* 0 */
	DWORD	version;	/* 0 */
	DWORD	characts;	/* 0 */
} emptyheader		= {0, 0x20, 0xffff, 0, 0xffff, 0, 0, 0, 0, 0, 0},
  emptyheaderSWAPPED	= {0, BYTESWAP_DWORD(0x20), 0xffff, 0, 0xffff, 0, 0, 0, 0, 0, 0};

/*
 *****************************************************************************
 * Function	:
 * Syntax	:
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
/*
 *****************************************************************************
 * Function	:
 * Syntax	:
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
/*
 *****************************************************************************
 * Function	:
 * Syntax	:
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static int read_data(FILE *fp, size_t size, void *buf)
{
	unsigned int r;
	int pos = ftell(fp);
	r = fread(buf, 1, size, fp);
	if(r == size)
		return 0;
	if(r == 0 && ftell(fp) - pos > 0)
		return 1;
	else
		return -1;
}

/*
 *****************************************************************************
 * Function	:
 * Syntax	:
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static enum res_e res_type_from_id(const name_id_t *nid)
{
	if(nid->type == name_str)
		return res_usr;

	if(nid->type != name_ord)
		internal_error(__FILE__, __LINE__, "Invalid name_id descriptor %d\n", nid->type);

	switch(nid->name.i_name)
	{
	case WRC_RT_CURSOR:		return res_cur;
	case WRC_RT_BITMAP:		return res_bmp;
	case WRC_RT_ICON:		return res_ico;
	case WRC_RT_MENU:		return res_men;
	case WRC_RT_DIALOG:		return res_dlg;
	case WRC_RT_STRING:		return res_stt;
	case WRC_RT_FONTDIR:		return res_fntdir;
	case WRC_RT_FONT:		return res_fnt;
	case WRC_RT_ACCELERATOR:	return res_acc;
	case WRC_RT_RCDATA:		return res_rdt;
	case WRC_RT_MESSAGETABLE:	return res_msg;
	case WRC_RT_GROUP_CURSOR:	return res_curg;
	case WRC_RT_GROUP_ICON:		return res_icog;
	case WRC_RT_VERSION:		return res_ver;
	case WRC_RT_TOOLBAR:		return res_toolbar;

	default:
	case WRC_RT_DLGINCLUDE:
	case WRC_RT_PLUGPLAY:
	case WRC_RT_VXD:
	case WRC_RT_ANICURSOR:
	case WRC_RT_ANIICON:
		warning("Cannot be sure of resource type, using usertype settings\n");
		return res_usr;
	}
}

/*
 *****************************************************************************
 * Function	:
 * Syntax	:
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
#define get_word(idx)	(*((WORD *)(&res->data[idx])))
#define get_dword(idx)	(*((DWORD *)(&res->data[idx])))

static resource_t *read_res32(FILE *fp)
{
	static const char wrong_format[] = "Wrong resfile format (32bit)";
	DWORD ressize;
	DWORD hdrsize;
	DWORD totsize;
	WORD memopt;
	WORD language;
	int err;
	res_t *res;
	resource_t *rsc;
	resource_t *tail = NULL;
	resource_t *list = NULL;
	name_id_t *type = NULL;
	name_id_t *name = NULL;
	int idx;
	enum res_e res_type;
	user_t *usrres;

	while(1)
	{
		/* Get headersize and resource size */
		err = read_data(fp, sizeof(ressize), &ressize);
		if(err < 0)
			break;
		else if(err > 0)
			error(wrong_format);
		err = read_data(fp, sizeof(hdrsize), &hdrsize);
		if(err)
			error(wrong_format);

		/* Align sizes and compute total size */
		totsize = hdrsize;
		if(hdrsize & 3)
		{
			warning("Hu? .res header needed alignment (anything can happen now)\n");
			totsize += 4 - (hdrsize & 3);
		}
		totsize += ressize;
		if(ressize & 3)
			totsize += 4 - (ressize & 3);

		/* Read in entire data-block */
		fseek(fp, -8, SEEK_CUR);
		res = new_res();
		if(res->allocsize < totsize)
			grow_res(res, totsize - res->allocsize + 8);
		err = read_data(fp, totsize, res->data);
		if(err)
			error(wrong_format);

		res->dataidx = hdrsize;
		res->size = hdrsize + ressize;

		/* Analyse the content of the header */
		idx = 8;
		/* Get restype */
		if(get_word(idx) == 0xffff)
		{
			idx += sizeof(WORD);
			type = new_name_id();
			type->type = name_ord;
			type->name.i_name = get_word(idx);
			idx += sizeof(WORD);
		}
		else if(get_word(idx) == 0)
		{
			error("ResType name has zero length (32 bit)\n");
		}
		else
		{
			int tag = idx;
			string_t *str;
			while(1)
			{
				idx += sizeof(WORD);
				if(!get_word(idx))
					break;
			}
			idx += sizeof(WORD);
			str = new_string();
			str->type = str_unicode;
			str->size = (idx - tag) / 2;
			str->str.wstr = xmalloc(idx-tag+2);
			memcpy(str->str.wstr, &res->data[tag], idx-tag);
			str->str.wstr[str->size] = 0;
			type = new_name_id();
			type->type = name_str;
			type->name.s_name = str;
		}
		/* Get resname */
		if(get_word(idx) == 0xffff)
		{
			idx += sizeof(WORD);
			name = new_name_id();
			name->type = name_ord;
			name->name.i_name = get_word(idx);
			idx += sizeof(WORD);
		}
		else if(get_word(idx) == 0)
		{
			error("ResName name has zero length (32 bit)\n");
		}
		else
		{
			int tag = idx;
			string_t *str;
			while(1)
			{
				idx += sizeof(WORD);
				if(!get_word(idx))
					break;
			}
			idx += sizeof(WORD);
			str = new_string();
			str->type = str_unicode;
			str->size = (idx - tag) / 2;
			str->str.wstr = xmalloc(idx-tag+2);
			memcpy(str->str.wstr, &res->data[tag], idx-tag);
			str->str.wstr[str->size] = 0;
			name = new_name_id();
			name->type = name_str;
			name->name.s_name = str;
		}

		/* align */
		if(idx & 0x3)
			idx += 4 - (idx & 3);

		idx += sizeof(DWORD);	/* Skip DataVersion */
		memopt = get_word(idx);
		idx += sizeof(WORD);
		language = get_word(idx);

		/* Build a resource_t list */
		res_type = res_type_from_id(type);
		if(res_type == res_usr)
		{
			/* User-type has custom ResType for .[s|h] generation */
			usrres = new_user(type, NULL, new_int(memopt));
		}
		else
			usrres = NULL;
		rsc = new_resource(res_type,
				   usrres,
				   memopt,
				   new_language(PRIMARYLANGID(language),
						SUBLANGID(language)));
		rsc->binres = res;
		rsc->name = name;
		rsc->c_name = make_c_name(get_c_typename(res_type), name, rsc->lan);
		if(!list)
		{
			list = rsc;
			tail = rsc;
		}
		else
		{
			rsc->prev = tail;
			tail->next = rsc;
			tail = rsc;
		}
	}
	return list;
}

/*
 *****************************************************************************
 * Function	:
 * Syntax	:
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static resource_t *read_res16(FILE *fp)
{
	internal_error(__FILE__, __LINE__, "Can't yet read 16 bit .res files\n");
	return NULL;
}

/*
 *****************************************************************************
 * Function	: read_resfile
 * Syntax	: resource_t *read_resfile(char *inname)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
resource_t *read_resfile(char *inname)
{
	FILE *fp;
	struct resheader32 rh;
	int is32bit = 1;
	resource_t *top;

	fp = fopen(inname, "rb");
	if(!fp)
		error("Could not open inputfile %s\n", inname);

	/* Determine 16 or 32 bit .res file */
	if(fread(&rh, 1, sizeof(rh), fp) != sizeof(rh))
		is32bit = 0;
	else
	{
		if(!memcmp(&emptyheader, &rh, sizeof(rh)))
			is32bit = 1;
		else if(!memcmp(&emptyheaderSWAPPED, &rh, sizeof(rh)))
			error("Binary .res-file has its byteorder swapped\n");
		else
			is32bit = 0;
	}

	if(is32bit && !win32)
		error("Cannot convert 32-bit .res-file into 16-bit resources (and will, hopefully never, implement it)\n");

	if(!is32bit && win32)
		error("Cannot (yet) convert 16-bit .res-file into 32-bit resources\n");

	if(!is32bit)
	{
		fseek(fp, 0, SEEK_SET);
		top = read_res16(fp);
	}
	else
	{
		top = read_res32(fp);
	}

	fclose(fp);

	return top;
}
