/*
 * Generate .res format from a resource-tree
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 * 05-May-2000 BS	- Added code to support endian conversions. The
 * 			  extra functions also aid unaligned access, but
 * 			  this is not yet implemented.
 * 25-May-1998 BS	- Added simple unicode -> char conversion for resource
 *			  names in .s and .h files.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "wrc.h"
#include "genres.h"
#include "utils.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/unicode.h"

/* Fix 64-bit host, re: put_dword */
#if defined(unix) && defined(__x86_64__)
typedef unsigned int HOST_DWORD;
#else
typedef unsigned long HOST_DWORD;
#endif

#define SetResSize(res, tag)	set_dword((res), (tag), (res)->size - get_dword((res), (tag)))

res_t *new_res(void)
{
	res_t *r;
	r = (res_t *)xmalloc(sizeof(res_t));
	r->allocsize = RES_BLOCKSIZE;
	r->size = 0;
	r->data = (char *)xmalloc(RES_BLOCKSIZE);
	return r;
}

res_t *grow_res(res_t *r, unsigned int add)
{
	r->allocsize += add;
	r->data = (char *)xrealloc(r->data, r->allocsize);
	return r;
}

/*
 *****************************************************************************
 * Function	: put_byte
 *		  put_word
 *		  put_dword
 * Syntax	: void put_byte(res_t *res, unsigned c)
 *		  void put_word(res_t *res, unsigned w)
 *		  void put_dword(res_t *res, unsigned d)
 * Input	:
 *	res	- Binary resource to put the data in
 *	c, w, d	- Data to put
 * Output	: nop
 * Description	: Put primitives that put an item in the binary resource.
 *		  The data array grows automatically.
 * Remarks	:
 *****************************************************************************
*/
void put_byte(res_t *res, unsigned c)
{
	if(res->allocsize - res->size < sizeof(char))
		grow_res(res, RES_BLOCKSIZE);
	res->data[res->size] = (char)c;
	res->size += sizeof(char);
}

void put_word(res_t *res, unsigned w)
{
	if(res->allocsize - res->size < sizeof(WORD))
		grow_res(res, RES_BLOCKSIZE);
	switch(byteorder)
	{
#ifdef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_BIG:
		res->data[res->size+0] = HIBYTE(w);
		res->data[res->size+1] = LOBYTE(w);
		break;

#ifndef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_LITTLE:
		res->data[res->size+1] = HIBYTE(w);
		res->data[res->size+0] = LOBYTE(w);
		break;
	}
	res->size += sizeof(WORD);
}

void put_dword(res_t *res, unsigned d)
{
	assert(sizeof(HOST_DWORD) == 4);

	if(res->allocsize - res->size < sizeof(HOST_DWORD))
		grow_res(res, RES_BLOCKSIZE);
	switch(byteorder)
	{
#ifdef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_BIG:
		res->data[res->size+0] = HIBYTE(HIWORD(d));
		res->data[res->size+1] = LOBYTE(HIWORD(d));
		res->data[res->size+2] = HIBYTE(LOWORD(d));
		res->data[res->size+3] = LOBYTE(LOWORD(d));
		break;

#ifndef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_LITTLE:
		res->data[res->size+3] = HIBYTE(HIWORD(d));
		res->data[res->size+2] = LOBYTE(HIWORD(d));
		res->data[res->size+1] = HIBYTE(LOWORD(d));
		res->data[res->size+0] = LOBYTE(LOWORD(d));
		break;
	}
	res->size += sizeof(HOST_DWORD);
}

static void put_pad(res_t *res)
{
	while(res->size & 0x3)
		put_byte(res, 0);
}

/*
 *****************************************************************************
 * Function	: set_word
 *		  set_dword
 * Syntax	: void set_word(res_t *res, int ofs, unsigned w)
 *		  void set_dword(res_t *res, int ofs, unsigned d)
 * Input	:
 *	res	- Binary resource to put the data in
 *	ofs	- Byte offset in data-array
 *	w, d	- Data to put
 * Output	: nop
 * Description	: Set the value of a binary resource data array to a
 * 		  specific value.
 * Remarks	:
 *****************************************************************************
*/
static void set_word(res_t *res, int ofs, unsigned w)
{
	switch(byteorder)
	{
#ifdef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_BIG:
		res->data[ofs+0] = HIBYTE(w);
		res->data[ofs+1] = LOBYTE(w);
		break;

#ifndef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_LITTLE:
		res->data[ofs+1] = HIBYTE(w);
		res->data[ofs+0] = LOBYTE(w);
		break;
	}
}

static void set_dword(res_t *res, int ofs, unsigned d)
{
	switch(byteorder)
	{
#ifdef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_BIG:
		res->data[ofs+0] = HIBYTE(HIWORD(d));
		res->data[ofs+1] = LOBYTE(HIWORD(d));
		res->data[ofs+2] = HIBYTE(LOWORD(d));
		res->data[ofs+3] = LOBYTE(LOWORD(d));
		break;

#ifndef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_LITTLE:
		res->data[ofs+3] = HIBYTE(HIWORD(d));
		res->data[ofs+2] = LOBYTE(HIWORD(d));
		res->data[ofs+1] = HIBYTE(LOWORD(d));
		res->data[ofs+0] = LOBYTE(LOWORD(d));
		break;
	}
}

/*
 *****************************************************************************
 * Function	: get_dword
 * Input	:
 *	res	- Binary resource to put the data in
 *	ofs	- Byte offset in data-array
 * Output	: The data in native endian
 * Description	: Get the value of a binary resource data array in native
 * 		  endian.
 * Remarks	:
 *****************************************************************************
*/
static HOST_DWORD get_dword(res_t *res, int ofs)
{
	switch(byteorder)
	{
#ifdef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_BIG:
		return   (res->data[ofs+0] << 24)
		       | (res->data[ofs+1] << 16)
		       | (res->data[ofs+2] <<  8)
		       |  res->data[ofs+3];

#ifndef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_LITTLE:
		return   (res->data[ofs+3] << 24)
		       | (res->data[ofs+2] << 16)
		       | (res->data[ofs+1] <<  8)
		       |  res->data[ofs+0];
	}
}

/*
 *****************************************************************************
 * Function	: string_to_upper
 * Syntax	: void string_to_upper(string_t *str)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	: FIXME: codepages...
 *****************************************************************************
*/
static void string_to_upper(string_t *str)
{
    int i;

    if(str->type == str_char)
    {
        for (i = 0; i < str->size; i++) str->str.cstr[i] = toupper((unsigned char)str->str.cstr[i]);
    }
    else if(str->type == str_unicode)
    {
        for (i = 0; i < str->size; i++) str->str.wstr[i] = toupperW(str->str.wstr[i]);
    }
    else
    {
        internal_error(__FILE__, __LINE__, "Invalid string type %d", str->type);
    }
}

/*
 *****************************************************************************
 * Function	: put_string
 * Syntax	: void put_string(res_t *res, string_t *str, enum str_e type,
 *				  int isterm, const language_t *lang)
 * Input	:
 *	res	- Binary resource to put the data in
 *	str	- String to put
 *	type	- Data has to be written in either str_char or str_unicode
 *	isterm	- The string is '\0' terminated (disregard the string's
 *		  size member)
 * Output	: nop
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static void put_string(res_t *res, const string_t *str, enum str_e type, int isterm,
                       const language_t *lang)
{
    int cnt, codepage;
    string_t *newstr;

    assert(res != NULL);
    assert(str != NULL);

    if (lang) codepage = get_language_codepage( lang->id, lang->sub );
    else codepage = get_language_codepage( 0, 0 );

    assert( codepage != -1 );

    newstr = convert_string(str, type, codepage);
    if (type == str_unicode)
    {
        if (str->type == str_char)
        {
            if (!check_unicode_conversion( str, newstr, codepage ))
                error( "String %s does not convert identically to Unicode and back in codepage %d. "
                       "Try using a Unicode string instead.", str->str.cstr, codepage );
        }
        if (!isterm) put_word(res, newstr->size);
        for(cnt = 0; cnt < newstr->size; cnt++)
        {
            WCHAR c = newstr->str.wstr[cnt];
            if (isterm && !c) break;
            put_word(res, c);
        }
        if (isterm) put_word(res, 0);
    }
    else  /* str_char */
    {
        if (!isterm) put_byte(res, newstr->size);
        for(cnt = 0; cnt < newstr->size; cnt++)
        {
            char c = newstr->str.cstr[cnt];
            if (isterm && !c) break;
            put_byte(res, c);
        }
        if (isterm) put_byte(res, 0);
    }
    free_string(newstr);
}

/*
 *****************************************************************************
 * Function	: put_name_id
 * Syntax	: void put_name_id(res_t *res, name_id_t *nid, int upcase, const language_t *lang)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static void put_name_id(res_t *res, name_id_t *nid, int upcase, const language_t *lang)
{
	if(nid->type == name_ord)
	{
		if(win32)
			put_word(res, 0xffff);
		else
			put_byte(res, 0xff);
		put_word(res, (WORD)nid->name.i_name);
	}
	else if(nid->type == name_str)
	{
		if(upcase)
			string_to_upper(nid->name.s_name);
		put_string(res, nid->name.s_name, win32 ? str_unicode : str_char, TRUE, lang);
	}
	else
	{
		internal_error(__FILE__, __LINE__, "Invalid name_id type %d", nid->type);
	}
}

/*
 *****************************************************************************
 * Function	: put_lvc
 * Syntax	: void put_lvc(res_t *res, lvc_t *lvc)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static void put_lvc(res_t *res, lvc_t *lvc)
{
	if(lvc && lvc->language)
		put_word(res, MAKELANGID(lvc->language->id, lvc->language->sub));
	else
		put_word(res, 0);	/* Neutral */
	if(lvc && lvc->version)
		put_dword(res, *(lvc->version));
	else
		put_dword(res, 0);
	if(lvc && lvc->characts)
		put_dword(res, *(lvc->characts));
	else
		put_dword(res, 0);
}

/*
 *****************************************************************************
 * Function	: put_raw_data
 * Syntax	: void put_raw_data(res_t *res, raw_data_t *raw, int offset)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static void put_raw_data(res_t *res, raw_data_t *raw, int offset)
{
	unsigned int wsize = raw->size - offset;
	if(res->allocsize - res->size < wsize)
		grow_res(res, wsize);
	memcpy(&(res->data[res->size]), raw->data + offset, wsize);
	res->size += wsize;
}

/*
 *****************************************************************************
 * Function	: put_res_header
 * Syntax	: intput_res_header(res_t *res, int type, name_id_t *ntype,
 *				    name_id_t *name, DWORD memopt, lvc_t *lvc)
 *
 * Input	:
 *	res	- Binary resource descriptor to write to
 *	type	- Resource identifier (if ntype == NULL)
 *	ntype	- Name id of type
 *	name	- Resource's name
 *	memopt	- Resource's memory options to write
 *	lvc	- Language, version and characteristics (win32 only)
 * Output	: An index to the resource size field. The resource size field
 *		  contains the header size upon exit.
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static int put_res_header(res_t *res, int type, name_id_t *ntype, name_id_t *name,
                          HOST_DWORD memopt, lvc_t *lvc)
{
	if(win32)
	{
		put_dword(res, 0);		/* We will overwrite these later */
		put_dword(res, 0);
		if(!ntype)
		{
			put_word(res, 0xffff);		/* ResType */
			put_word(res, type);
		}
		else
			put_name_id(res, ntype, TRUE, lvc->language);
		put_name_id(res, name, TRUE, lvc->language); /* ResName */
		put_pad(res);
		put_dword(res, 0);		/* DataVersion */
		put_word(res, memopt);		/* Memory options */
		put_lvc(res, lvc);		/* Language, version and characts */
		set_dword(res, 0*sizeof(HOST_DWORD), res->size);	/* Set preliminary resource */
		set_dword(res, 1*sizeof(HOST_DWORD), res->size);	/* Set HeaderSize */
		res->dataidx = res->size;
		return 0;
	}
	else /* win16 */
	{
		int tag;
		if(!ntype)
		{
			put_byte(res, 0xff);		/* ResType */
			put_word(res, type);
		}
		else
			put_name_id(res, ntype, TRUE, NULL);
		put_name_id(res, name, TRUE, NULL); /* ResName */
		put_word(res, memopt);		/* Memory options */
		tag = res->size;
		put_dword(res, 0);		/* ResSize overwritten later*/
		set_dword(res, tag, res->size);
		res->dataidx = res->size;
		return tag;
	}
}

/*
 *****************************************************************************
 * Function	: accelerator2res
 * Syntax	: res_t *accelerator2res(name_id_t *name, accelerator_t *acc)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	acc	- The accelerator descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *accelerator2res(name_id_t *name, accelerator_t *acc)
{
	int restag;
	res_t *res;
	event_t *ev;
	assert(name != NULL);
	assert(acc != NULL);

	ev = acc->events;
	res = new_res();
	if(win32)
	{
		restag = put_res_header(res, WRC_RT_ACCELERATOR, NULL, name, acc->memopt, &(acc->lvc));
		while(ev)
		{
			put_word(res, ev->flags | (ev->next ? 0 : 0x80));
			put_word(res, ev->key);
			put_word(res, ev->id);
			put_word(res, 0);	/* Padding */
			ev = ev->next;
		}
		put_pad(res);
	}
	else /* win16 */
	{
		restag = put_res_header(res, WRC_RT_ACCELERATOR, NULL, name, acc->memopt, NULL);
		while(ev)
		{
			put_byte(res, ev->flags | (ev->next ? 0 : 0x80));
			put_word(res, ev->key);
			put_word(res, ev->id);
			ev = ev->next;
		}
	}
	/* Set ResourceSize */
	SetResSize(res, restag);
	return res;
}

/*
 *****************************************************************************
 * Function	: dialog2res
 * Syntax	: res_t *dialog2res(name_id_t *name, dialog_t *dlg)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	dlg	- The dialog descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *dialog2res(name_id_t *name, dialog_t *dlg)
{
	int restag;
	res_t *res;
	control_t *ctrl;
	int tag_nctrl;
	int nctrl = 0;
	assert(name != NULL);
	assert(dlg != NULL);

	ctrl = dlg->controls;
	res = new_res();
	if(win32)
	{
		restag = put_res_header(res, WRC_RT_DIALOG, NULL, name, dlg->memopt, &(dlg->lvc));

		put_dword(res, dlg->style->or_mask);
		put_dword(res, dlg->gotexstyle ? dlg->exstyle->or_mask : 0);
		tag_nctrl = res->size;
		put_word(res, 0);		/* Number of controls */
		put_word(res, dlg->x);
		put_word(res, dlg->y);
		put_word(res, dlg->width);
		put_word(res, dlg->height);
		if(dlg->menu)
			put_name_id(res, dlg->menu, TRUE, dlg->lvc.language);
		else
			put_word(res, 0);
		if(dlg->dlgclass)
			put_name_id(res, dlg->dlgclass, TRUE, dlg->lvc.language);
		else
			put_word(res, 0);
		if(dlg->title)
			put_string(res, dlg->title, str_unicode, TRUE, dlg->lvc.language);
		else
			put_word(res, 0);
		if(dlg->font)
		{
			put_word(res, dlg->font->size);
			put_string(res, dlg->font->name, str_unicode, TRUE, dlg->lvc.language);
		}

		put_pad(res);
		while(ctrl)
		{
			/* FIXME: what is default control style? */
			put_dword(res, ctrl->gotstyle ? ctrl->style->or_mask: WS_CHILD);
			put_dword(res, ctrl->gotexstyle ? ctrl->exstyle->or_mask : 0);
			put_word(res, ctrl->x);
			put_word(res, ctrl->y);
			put_word(res, ctrl->width);
			put_word(res, ctrl->height);
			put_word(res, ctrl->id);
			if(ctrl->ctlclass)
				put_name_id(res, ctrl->ctlclass, TRUE, dlg->lvc.language);
			else
				internal_error(__FILE__, __LINE__, "Control has no control-class");
			if(ctrl->title)
				put_name_id(res, ctrl->title, FALSE, dlg->lvc.language);
			else
				put_word(res, 0);
			if(ctrl->extra)
			{
				put_word(res, ctrl->extra->size+2);
				put_pad(res);
				put_raw_data(res, ctrl->extra, 0);
			}
			else
				put_word(res, 0);

			if(ctrl->next)
				put_pad(res);
			nctrl++;
			ctrl = ctrl->next;
		}
		/* Set number of controls */
		set_word(res, tag_nctrl, (WORD)nctrl);
	}
	else /* win16 */
	{
		restag = put_res_header(res, WRC_RT_DIALOG, NULL, name, dlg->memopt, NULL);

		put_dword(res, dlg->gotstyle ? dlg->style->or_mask : WS_POPUPWINDOW);
		tag_nctrl = res->size;
		put_byte(res, 0);		/* Number of controls */
		put_word(res, dlg->x);
		put_word(res, dlg->y);
		put_word(res, dlg->width);
		put_word(res, dlg->height);
		if(dlg->menu)
			put_name_id(res, dlg->menu, TRUE, NULL);
		else
			put_byte(res, 0);
		if(dlg->dlgclass)
			put_name_id(res, dlg->dlgclass, TRUE, NULL);
		else
			put_byte(res, 0);
		if(dlg->title)
			put_string(res, dlg->title, str_char, TRUE, NULL);
		else
			put_byte(res, 0);
		if(dlg->font)
		{
			put_word(res, dlg->font->size);
			put_string(res, dlg->font->name, str_char, TRUE, NULL);
		}

		while(ctrl)
		{
			put_word(res, ctrl->x);
			put_word(res, ctrl->y);
			put_word(res, ctrl->width);
			put_word(res, ctrl->height);
			put_word(res, ctrl->id);
			put_dword(res, ctrl->gotstyle ? ctrl->style->or_mask: WS_CHILD);
			if(ctrl->ctlclass)
			{
				if(ctrl->ctlclass->type == name_ord
				&& ctrl->ctlclass->name.i_name >= 0x80
				&& ctrl->ctlclass->name.i_name <= 0x85)
					put_byte(res, ctrl->ctlclass->name.i_name);
				else if(ctrl->ctlclass->type == name_str)
					put_name_id(res, ctrl->ctlclass, FALSE, NULL);
				else
					error("Unknown control-class %04x", ctrl->ctlclass->name.i_name);
			}
			else
				internal_error(__FILE__, __LINE__, "Control has no control-class");
			if(ctrl->title)
				put_name_id(res, ctrl->title, FALSE, NULL);
			else
				put_byte(res, 0);

			/* FIXME: What is this extra byte doing here? */
			put_byte(res, 0);

			nctrl++;
			ctrl = ctrl->next;
		}
		/* Set number of controls */
		((char *)res->data)[tag_nctrl] = (char)nctrl;
	}
	/* Set ResourceSize */
	SetResSize(res, restag);
	return res;
}

/*
 *****************************************************************************
 * Function	: dialogex2res
 * Syntax	: res_t *dialogex2res(name_id_t *name, dialogex_t *dlgex)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	dlgex	- The dialogex descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *dialogex2res(name_id_t *name, dialogex_t *dlgex)
{
	int restag;
	res_t *res;
	control_t *ctrl;
	int tag_nctrl;
	int nctrl = 0;
	assert(name != NULL);
	assert(dlgex != NULL);

	ctrl = dlgex->controls;
	res = new_res();
	if(win32)
	{
		restag = put_res_header(res, WRC_RT_DIALOG, NULL, name, dlgex->memopt, &(dlgex->lvc));

		/* FIXME: MS doc says thet the first word must contain 0xffff
		 * and the second 0x0001 to signal a DLGTEMPLATEEX. Borland's
		 * compiler reverses the two words.
		 * I don't know which one to choose, but I write it as Mr. B
		 * writes it.
		 */
		put_word(res, 1);		/* Signature */
		put_word(res, 0xffff);		/* DlgVer */
		put_dword(res, dlgex->gothelpid ? dlgex->helpid : 0);
		put_dword(res, dlgex->gotexstyle ? dlgex->exstyle->or_mask : 0);
		put_dword(res, dlgex->gotstyle ? dlgex->style->or_mask : WS_POPUPWINDOW);
		tag_nctrl = res->size;
		put_word(res, 0);		/* Number of controls */
		put_word(res, dlgex->x);
		put_word(res, dlgex->y);
		put_word(res, dlgex->width);
		put_word(res, dlgex->height);
		if(dlgex->menu)
			put_name_id(res, dlgex->menu, TRUE, dlgex->lvc.language);
		else
			put_word(res, 0);
		if(dlgex->dlgclass)
			put_name_id(res, dlgex->dlgclass, TRUE, dlgex->lvc.language);
		else
			put_word(res, 0);
		if(dlgex->title)
			put_string(res, dlgex->title, str_unicode, TRUE, dlgex->lvc.language);
		else
			put_word(res, 0);
		if(dlgex->font)
		{
			put_word(res, dlgex->font->size);
			put_word(res, dlgex->font->weight);
			/* FIXME: ? TRUE should be sufficient to say that its
			 * italic, but Borland's compiler says its 0x0101.
			 * I just copy it here, and hope for the best.
			 */
			put_word(res, dlgex->font->italic ? 0x0101 : 0);
			put_string(res, dlgex->font->name, str_unicode, TRUE, dlgex->lvc.language);
		}

		put_pad(res);
		while(ctrl)
		{
			put_dword(res, ctrl->gothelpid ? ctrl->helpid : 0);
			put_dword(res, ctrl->gotexstyle ? ctrl->exstyle->or_mask : 0);
			/* FIXME: what is default control style? */
			put_dword(res, ctrl->gotstyle ? ctrl->style->or_mask : WS_CHILD | WS_VISIBLE);
			put_word(res, ctrl->x);
			put_word(res, ctrl->y);
			put_word(res, ctrl->width);
			put_word(res, ctrl->height);
			put_dword(res, ctrl->id);
			if(ctrl->ctlclass)
				put_name_id(res, ctrl->ctlclass, TRUE, dlgex->lvc.language);
			else
				internal_error(__FILE__, __LINE__, "Control has no control-class");
			if(ctrl->title)
				put_name_id(res, ctrl->title, FALSE, dlgex->lvc.language);
			else
				put_word(res, 0);
			if(ctrl->extra)
			{
				put_pad(res);
				put_word(res, ctrl->extra->size);
				put_raw_data(res, ctrl->extra, 0);
			}
			else
				put_word(res, 0);

			put_pad(res);
			nctrl++;
			ctrl = ctrl->next;
		}
		/* Set number of controls */
		set_word(res, tag_nctrl, (WORD)nctrl);
		/* Set ResourceSize */
		SetResSize(res, restag);
		put_pad(res);
	}
	else /* win16 */
	{
		/* Do not generate anything in 16-bit mode */
		free(res->data);
		free(res);
		return NULL;
	}
	return res;
}

/*
 *****************************************************************************
 * Function	: menuitem2res
 * Syntax	: void menuitem2res(res_t *res, menu_item_t *item)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	: Self recursive
 *****************************************************************************
*/
static void menuitem2res(res_t *res, menu_item_t *menitem, const language_t *lang)
{
	menu_item_t *itm = menitem;
	if(win32)
	{
		while(itm)
		{
			put_word(res, itm->state | (itm->popup ? MF_POPUP : 0) | (!itm->next ? MF_END : 0));
			if(!itm->popup)
				put_word(res, itm->id);
			if(itm->name)
				put_string(res, itm->name, str_unicode, TRUE, lang);
			else
				put_word(res, 0);
			if(itm->popup)
				menuitem2res(res, itm->popup, lang);
			itm = itm->next;
		}
	}
	else /* win16 */
	{
		while(itm)
		{
			put_word(res, itm->state | (itm->popup ? MF_POPUP : 0) | (!itm->next ? MF_END : 0));
			if(!itm->popup)
				put_word(res, itm->id);
			if(itm->name)
				put_string(res, itm->name, str_char, TRUE, lang);
			else
				put_byte(res, 0);
			if(itm->popup)
				menuitem2res(res, itm->popup, lang);
			itm = itm->next;
		}
	}

}

/*
 *****************************************************************************
 * Function	: menu2res
 * Syntax	: res_t *menu2res(name_id_t *name, menu_t *men)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	men	- The menu descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *menu2res(name_id_t *name, menu_t *men)
{
	int restag;
	res_t *res;
	assert(name != NULL);
	assert(men != NULL);

	res = new_res();
	restag = put_res_header(res, WRC_RT_MENU, NULL, name, men->memopt, win32 ? &(men->lvc) : NULL);

	put_dword(res, 0);		/* Menuheader: Version and HeaderSize */
	menuitem2res(res, men->items, win32 ? men->lvc.language : NULL);
	/* Set ResourceSize */
	SetResSize(res, restag);
	if(win32)
		put_pad(res);
	return res;
}

/*
 *****************************************************************************
 * Function	: menuexitem2res
 * Syntax	: void menuexitem2res(res_t *res, menuex_item_t *item)
 * Input	:
 * Output	: nop
 * Description	:
 * Remarks	: Self recursive
 *****************************************************************************
*/
static void menuexitem2res(res_t *res, menuex_item_t *menitem, const language_t *lang)
{
	menuex_item_t *itm = menitem;
	assert(win32 != 0);
	while(itm)
	{
		put_dword(res, itm->gottype ? itm->type : 0);
		put_dword(res, itm->gotstate ? itm->state : 0);
		put_dword(res, itm->gotid ? itm->id : 0);	/* FIXME: Docu. says word */
		put_word(res, (itm->popup ? 0x01 : 0) | (!itm->next ? MF_END : 0));
		if(itm->name)
			put_string(res, itm->name, str_unicode, TRUE, lang);
		else
			put_word(res, 0);
		put_pad(res);
		if(itm->popup)
		{
			put_dword(res, itm->gothelpid ? itm->helpid : 0);
			menuexitem2res(res, itm->popup, lang);
		}
		itm = itm->next;
	}

}

/*
 *****************************************************************************
 * Function	: menuex2res
 * Syntax	: res_t *menuex2res(name_id_t *name, menuex_t *menex)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	menex	- The menuex descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *menuex2res(name_id_t *name, menuex_t *menex)
{
	int restag;
	res_t *res;
	assert(name != NULL);
	assert(menex != NULL);

	res = new_res();
	if(win32)
	{
		restag = put_res_header(res, WRC_RT_MENU, NULL, name, menex->memopt, &(menex->lvc));

		put_word(res, 1);		/* Menuheader: Version */
		put_word(res, 4);		/* Offset */
		put_dword(res, 0);		/* HelpId */
		put_pad(res);
		menuexitem2res(res, menex->items, menex->lvc.language);
		/* Set ResourceSize */
		SetResSize(res, restag);
		put_pad(res);
	}
	else /* win16 */
	{
		/* Do not generate anything in 16-bit mode */
		free(res->data);
		free(res);
		return NULL;
	}
	return res;
}

/*
 *****************************************************************************
 * Function	: cursorgroup2res
 * Syntax	: res_t *cursorgroup2res(name_id_t *name, cursor_group_t *curg)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	curg	- The cursor descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *cursorgroup2res(name_id_t *name, cursor_group_t *curg)
{
	int restag;
	res_t *res;
	cursor_t *cur;
	assert(name != NULL);
	assert(curg != NULL);

	res = new_res();
	restag = put_res_header(res, WRC_RT_GROUP_CURSOR, NULL, name, curg->memopt, &(curg->lvc));
	if(win32)
	{
		put_word(res, 0);	/* Reserved */
		/* FIXME: The ResType in the NEWHEADER structure should
		 * contain 14 according to the MS win32 doc. This is
		 * not the case with the BRC compiler and I really doubt
		 * the latter. Putting one here is compliant to win16 spec,
		 * but who knows the true value?
		 */
		put_word(res, 2);	/* ResType */
		put_word(res, curg->ncursor);
#if 0
		for(cur = curg->cursorlist; cur; cur = cur->next)
#else
		cur = curg->cursorlist;
		while(cur->next)
			cur = cur->next;
		for(; cur; cur = cur->prev)
#endif
		{
			put_word(res, cur->width);
			/* FIXME: The height of a cursor is half the size of
			 * the bitmap's height. BRC puts the height from the
			 * BITMAPINFOHEADER here instead of the cursorfile's
			 * height. MS doesn't seem to care...
			 */
			put_word(res, cur->height);
			/* FIXME: The next two are reversed in BRC and I don't
			 * know why. Probably a bug. But, we can safely ignore
			 * it because win16 does not support color cursors.
			 * A warning should have been generated by the parser.
			 */
			put_word(res, cur->planes);
			put_word(res, cur->bits);
			/* FIXME: The +4 is the hotspot in the cursor resource.
			 * However, I cound not find this in the documentation.
			 * The hotspot bytes must either be included or MS
			 * doesn't care.
			 */
			put_dword(res, cur->data->size +4);
			put_word(res, cur->id);
		}
	}
	else /* win16 */
	{
		put_word(res, 0);	/* Reserved */
		put_word(res, 2);	/* ResType */
		put_word(res, curg->ncursor);
#if 0
		for(cur = curg->cursorlist; cur; cur = cur->next)
#else
		cur = curg->cursorlist;
		while(cur->next)
			cur = cur->next;
		for(; cur; cur = cur->prev)
#endif
		{
			put_word(res, cur->width);
			/* FIXME: The height of a cursor is half the size of
			 * the bitmap's height. BRC puts the height from the
			 * BITMAPINFOHEADER here instead of the cursorfile's
			 * height. MS doesn't seem to care...
			 */
			put_word(res, cur->height);
			/* FIXME: The next two are reversed in BRC and I don't
			 * know why. Probably a bug. But, we can safely ignore
			 * it because win16 does not support color cursors.
			 * A warning should have been generated by the parser.
			 */
			put_word(res, cur->planes);
			put_word(res, cur->bits);
			/* FIXME: The +4 is the hotspot in the cursor resource.
			 * However, I cound not find this in the documentation.
			 * The hotspot bytes must either be included or MS
			 * doesn't care.
			 */
			put_dword(res, cur->data->size +4);
			put_word(res, cur->id);
		}
	}
	SetResSize(res, restag);	/* Set ResourceSize */
	if(win32)
		put_pad(res);

	return res;
}

/*
 *****************************************************************************
 * Function	: cursor2res
 * Syntax	: res_t *cursor2res(cursor_t *cur)
 * Input	:
 *	cur	- The cursor descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *cursor2res(cursor_t *cur)
{
	int restag;
	res_t *res;
	name_id_t name;

	assert(cur != NULL);

	res = new_res();
	name.type = name_ord;
	name.name.i_name = cur->id;
	restag = put_res_header(res, WRC_RT_CURSOR, NULL, &name, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, &(cur->lvc));
	put_word(res, cur->xhot);
	put_word(res, cur->yhot);
	put_raw_data(res, cur->data, 0);

	SetResSize(res, restag);	/* Set ResourceSize */
	if(win32)
		put_pad(res);

	return res;
}

/*
 *****************************************************************************
 * Function	: icongroup2res
 * Syntax	: res_t *icongroup2res(name_id_t *name, icon_group_t *icog)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	icog	- The icon group descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *icongroup2res(name_id_t *name, icon_group_t *icog)
{
	int restag;
	res_t *res;
	icon_t *ico;
	assert(name != NULL);
	assert(icog != NULL);

	res = new_res();
	restag = put_res_header(res, WRC_RT_GROUP_ICON, NULL, name, icog->memopt, &(icog->lvc));
	if(win32)
	{
		put_word(res, 0);	/* Reserved */
		/* FIXME: The ResType in the NEWHEADER structure should
		 * contain 14 according to the MS win32 doc. This is
		 * not the case with the BRC compiler and I really doubt
		 * the latter. Putting one here is compliant to win16 spec,
		 * but who knows the true value?
		 */
		put_word(res, 1);	/* ResType */
		put_word(res, icog->nicon);
		for(ico = icog->iconlist; ico; ico = ico->next)
		{
			put_byte(res, ico->width);
			put_byte(res, ico->height);
			put_byte(res, ico->nclr);
			put_byte(res, 0);	/* Reserved */
			put_word(res, ico->planes);
			put_word(res, ico->bits);
			put_dword(res, ico->data->size);
			put_word(res, ico->id);
		}
	}
	else /* win16 */
	{
		put_word(res, 0);	/* Reserved */
		put_word(res, 1);	/* ResType */
		put_word(res, icog->nicon);
		for(ico = icog->iconlist; ico; ico = ico->next)
		{
			put_byte(res, ico->width);
			put_byte(res, ico->height);
			put_byte(res, ico->nclr);
			put_byte(res, 0);	/* Reserved */
			put_word(res, ico->planes);
			put_word(res, ico->bits);
			put_dword(res, ico->data->size);
			put_word(res, ico->id);
		}
	}
	SetResSize(res, restag);	/* Set ResourceSize */
	if(win32)
		put_pad(res);

	return res;
}

/*
 *****************************************************************************
 * Function	: icon2res
 * Syntax	: res_t *icon2res(icon_t *ico)
 * Input	:
 *	ico	- The icon descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *icon2res(icon_t *ico)
{
	int restag;
	res_t *res;
	name_id_t name;

	assert(ico != NULL);

	res = new_res();
	name.type = name_ord;
	name.name.i_name = ico->id;
	restag = put_res_header(res, WRC_RT_ICON, NULL, &name, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, &(ico->lvc));
	put_raw_data(res, ico->data, 0);

	SetResSize(res, restag);	/* Set ResourceSize */
	if(win32)
		put_pad(res);

	return res;
}

/*
 *****************************************************************************
 * Function	: anicurico2res
 * Syntax	: res_t *anicurico2res(name_id_t *name, ani_curico_t *ani)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	ani	- The animated object descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	: The endian of the object's structures have been converted
 * 		  by the loader.
 * 		  There are rumors that win311 could handle animated stuff.
 * 		  That is why they are generated for both win16 and win32
 * 		  compile.
 *****************************************************************************
*/
static res_t *anicurico2res(name_id_t *name, ani_curico_t *ani, enum res_e type)
{
	int restag;
	res_t *res;
	assert(name != NULL);
	assert(ani != NULL);

	res = new_res();
	restag = put_res_header(res, type == res_anicur ? WRC_RT_ANICURSOR : WRC_RT_ANIICON,
				NULL, name, ani->memopt, NULL);
	put_raw_data(res, ani->data, 0);
	/* Set ResourceSize */
	SetResSize(res, restag);
	if(win32)
		put_pad(res);
	return res;
}

/*
 *****************************************************************************
 * Function	: bitmap2res
 * Syntax	: res_t *bitmap2res(name_id_t *name, bitmap_t *bmp)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	bmp	- The bitmap descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	: The endian of the bitmap structures have been converted
 * 		  by the loader.
 *****************************************************************************
*/
static res_t *bitmap2res(name_id_t *name, bitmap_t *bmp)
{
	int restag;
	res_t *res;
	assert(name != NULL);
	assert(bmp != NULL);

	res = new_res();
	restag = put_res_header(res, WRC_RT_BITMAP, NULL, name, bmp->memopt, &(bmp->data->lvc));
	if(bmp->data->data[0] == 'B'
	&& bmp->data->data[1] == 'M'
	&& ((BITMAPFILEHEADER *)bmp->data->data)->bfSize == bmp->data->size
	&& bmp->data->size >= sizeof(BITMAPFILEHEADER))
	{
		/* The File header is still attached, don't write it */
		put_raw_data(res, bmp->data, sizeof(BITMAPFILEHEADER));
	}
	else
	{
		put_raw_data(res, bmp->data, 0);
	}
	/* Set ResourceSize */
	SetResSize(res, restag);
	if(win32)
		put_pad(res);
	return res;
}

/*
 *****************************************************************************
 * Function	: font2res
 * Syntax	: res_t *font2res(name_id_t *name, font_t *fnt)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	fnt	- The font descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	: The data has been prepared just after parsing.
 *****************************************************************************
*/
static res_t *font2res(name_id_t *name, font_t *fnt)
{
	int restag;
	res_t *res;
	assert(name != NULL);
	assert(fnt != NULL);

	res = new_res();
	restag = put_res_header(res, WRC_RT_FONT, NULL, name, fnt->memopt, &(fnt->data->lvc));
	put_raw_data(res, fnt->data, 0);
	/* Set ResourceSize */
	SetResSize(res, restag);
	if(win32)
		put_pad(res);
	return res;
}

/*
 *****************************************************************************
 * Function	: fontdir2res
 * Syntax	: res_t *fontdir2res(name_id_t *name, fontdir_t *fnd)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	fntdir	- The fontdir descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	: The data has been prepared just after parsing.
 *****************************************************************************
*/
static res_t *fontdir2res(name_id_t *name, fontdir_t *fnd)
{
	int restag;
	res_t *res;
	assert(name != NULL);
	assert(fnd != NULL);

	res = new_res();
	restag = put_res_header(res, WRC_RT_FONTDIR, NULL, name, fnd->memopt, &(fnd->data->lvc));
	put_raw_data(res, fnd->data, 0);
	/* Set ResourceSize */
	SetResSize(res, restag);
	if(win32)
		put_pad(res);
	return res;
}

/*
 *****************************************************************************
 * Function	: html2res
 * Syntax	: res_t *html2res(name_id_t *name, html_t *html)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	rdt	- The html descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *html2res(name_id_t *name, html_t *html)
{
	int restag;
	res_t *res;
	assert(name != NULL);
	assert(html != NULL);

	res = new_res();
	restag = put_res_header(res, WRC_RT_HTML, NULL, name, html->memopt, &(html->data->lvc));
	put_raw_data(res, html->data, 0);
	/* Set ResourceSize */
	SetResSize(res, restag);
	if(win32)
		put_pad(res);
	return res;
}

/*
 *****************************************************************************
 * Function	: rcdata2res
 * Syntax	: res_t *rcdata2res(name_id_t *name, rcdata_t *rdt)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	rdt	- The rcdata descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *rcdata2res(name_id_t *name, rcdata_t *rdt)
{
	int restag;
	res_t *res;
	assert(name != NULL);
	assert(rdt != NULL);

	res = new_res();
	restag = put_res_header(res, WRC_RT_RCDATA, NULL, name, rdt->memopt, &(rdt->data->lvc));
	put_raw_data(res, rdt->data, 0);
	/* Set ResourceSize */
	SetResSize(res, restag);
	if(win32)
		put_pad(res);
	return res;
}

/*
 *****************************************************************************
 * Function	: messagetable2res
 * Syntax	: res_t *messagetable2res(name_id_t *name, messagetable_t *msg)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	msg	- The messagetable descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	: The data has been converted to the appropriate endian
 *		  after is was parsed.
 *****************************************************************************
*/
static res_t *messagetable2res(name_id_t *name, messagetable_t *msg)
{
	int restag;
	res_t *res;
	assert(name != NULL);
	assert(msg != NULL);

	res = new_res();
	restag = put_res_header(res, WRC_RT_MESSAGETABLE, NULL, name, msg->memopt, &(msg->data->lvc));
	put_raw_data(res, msg->data, 0);
	/* Set ResourceSize */
	SetResSize(res, restag);
	if(win32)
		put_pad(res);
	return res;
}

/*
 *****************************************************************************
 * Function	: stringtable2res
 * Syntax	: res_t *stringtable2res(stringtable_t *stt)
 * Input	:
 *	stt	- The stringtable descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *stringtable2res(stringtable_t *stt)
{
	res_t *res;
	name_id_t name;
	int i;
	int restag;
	HOST_DWORD lastsize = 0;

	assert(stt != NULL);
	res = new_res();

	for(; stt; stt = stt->next)
	{
		if(!stt->nentries)
		{
			warning("Empty internal stringtable");
			continue;
		}
		name.type = name_ord;
		name.name.i_name = (stt->idbase >> 4) + 1;
		restag = put_res_header(res, WRC_RT_STRING, NULL, &name, stt->memopt, win32 ? &(stt->lvc) : NULL);
		for(i = 0; i < stt->nentries; i++)
		{
			if(stt->entries[i].str && stt->entries[i].str->size)
			{
				put_string(res, stt->entries[i].str, win32 ? str_unicode : str_char,
                                           FALSE, win32 ? stt->lvc.language : NULL);
			}
			else
			{
				if (win32)
					put_word(res, 0);
				else
					put_byte(res, 0);
			}
		}
		/* Set ResourceSize */
		SetResSize(res, restag - lastsize);
		if(win32)
			put_pad(res);
		lastsize = res->size;
	}
	return res;
}

/*
 *****************************************************************************
 * Function	: user2res
 * Syntax	: res_t *user2res(name_id_t *name, user_t *usr)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	usr	- The userresource descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *user2res(name_id_t *name, user_t *usr)
{
	int restag;
	res_t *res;
	assert(name != NULL);
	assert(usr != NULL);

	res = new_res();
	restag = put_res_header(res, 0, usr->type, name, usr->memopt, &(usr->data->lvc));
	put_raw_data(res, usr->data, 0);
	/* Set ResourceSize */
	SetResSize(res, restag);
	if(win32)
		put_pad(res);
	return res;
}

/*
 *****************************************************************************
 * Function	: versionblock2res
 * Syntax	: void versionblock2res(res_t *res, ver_block_t *blk)
 * Input	:
 *	res	- Binary resource to write to
 *	blk	- The version block to be written
 * Output	:
 * Description	:
 * Remarks	: Self recursive
 *****************************************************************************
*/
static void versionblock2res(res_t *res, ver_block_t *blk, int level, const language_t *lang)
{
	ver_value_t *val;
	int blksizetag;
	int valblksizetag;
	int valvalsizetag;
	int tag;
	int i;

	blksizetag = res->size;
	put_word(res, 0);	/* Will be overwritten later */
	put_word(res, 0);
	if(win32)
		put_word(res, 0);	/* level ? */
	put_string(res, blk->name, win32 ? str_unicode : str_char, TRUE, lang);
	put_pad(res);
	for(val = blk->values; val; val = val->next)
	{
		if(val->type == val_str)
		{
			valblksizetag = res->size;
			put_word(res, 0);	/* Will be overwritten later */
			valvalsizetag = res->size;
			put_word(res, 0);	/* Will be overwritten later */
			if(win32)
			{
				put_word(res, level);
			}
			put_string(res, val->key, win32 ? str_unicode : str_char, TRUE, lang);
			put_pad(res);
			tag = res->size;
			put_string(res, val->value.str, win32 ? str_unicode : str_char, TRUE, lang);
			if(win32)
				set_word(res, valvalsizetag, (WORD)((res->size - tag) >> 1));
			else
				set_word(res, valvalsizetag, (WORD)(res->size - tag));
			set_word(res, valblksizetag, (WORD)(res->size - valblksizetag));
			put_pad(res);
		}
		else if(val->type == val_words)
		{
			valblksizetag = res->size;
			put_word(res, 0);	/* Will be overwritten later */
			valvalsizetag = res->size;
			put_word(res, 0);	/* Will be overwritten later */
			if(win32)
			{
				put_word(res, level);
			}
			put_string(res, val->key, win32 ? str_unicode : str_char, TRUE, lang);
			put_pad(res);
			tag = res->size;
			for(i = 0; i < val->value.words->nwords; i++)
			{
				put_word(res, val->value.words->words[i]);
			}
			set_word(res, valvalsizetag, (WORD)(res->size - tag));
			set_word(res, valblksizetag, (WORD)(res->size - valblksizetag));
			put_pad(res);
		}
		else if(val->type == val_block)
		{
			versionblock2res(res, val->value.block, level+1, lang);
		}
		else
		{
			internal_error(__FILE__, __LINE__, "Invalid value indicator %d in VERSIONINFO", val->type);
		}
	}

	/* Set blocksize */
	set_word(res, blksizetag, (WORD)(res->size - blksizetag));
}

/*
 *****************************************************************************
 * Function	: versioninfo2res
 * Syntax	: res_t *versioninfo2res(name_id_t *name, versioninfo_t *ver)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	ver	- The versioninfo descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *versioninfo2res(name_id_t *name, versioninfo_t *ver)
{
	int restag;
	int rootblocksizetag;
	int valsizetag;
	int tag;
	res_t *res;
	string_t vsvi;
	ver_block_t *blk;

	assert(name != NULL);
	assert(ver != NULL);

	vsvi.type = str_char;
	vsvi.str.cstr = xstrdup("VS_VERSION_INFO");
	vsvi.size = 15; /* Excl. termination */

	res = new_res();
	restag = put_res_header(res, WRC_RT_VERSION, NULL, name, ver->memopt, &(ver->lvc));
	rootblocksizetag = res->size;
	put_word(res, 0);	/* BlockSize filled in later */
	valsizetag = res->size;
	put_word(res, 0);	/* ValueSize filled in later*/
	if(win32)
		put_word(res, 0);	/* Tree-level ? */
	put_string(res, &vsvi, win32 ? str_unicode : str_char,
                   TRUE, win32 ? ver->lvc.language : NULL);
	if(win32)
		put_pad(res);
	tag = res->size;
	put_dword(res, VS_FFI_SIGNATURE);
	put_dword(res, VS_FFI_STRUCVERSION);
	put_dword(res, (ver->filever_maj1 << 16) + (ver->filever_maj2 & 0xffff));
	put_dword(res, (ver->filever_min1 << 16) + (ver->filever_min2 & 0xffff));
	put_dword(res, (ver->prodver_maj1 << 16) + (ver->prodver_maj2 & 0xffff));
	put_dword(res, (ver->prodver_min1 << 16) + (ver->prodver_min2 & 0xffff));
	put_dword(res, ver->fileflagsmask);
	put_dword(res, ver->fileflags);
	put_dword(res, ver->fileos);
	put_dword(res, ver->filetype);
	put_dword(res, ver->filesubtype);
	put_dword(res, 0);		/* FileDateMS */
	put_dword(res, 0);		/* FileDateLS */
	/* Set ValueSize */
	set_word(res, valsizetag, (WORD)(res->size - tag));
	/* Descend into the blocks */
	for(blk = ver->blocks; blk; blk = blk->next)
		versionblock2res(res, blk, 0, win32 ? ver->lvc.language : NULL);
	/* Set root block's size */
	set_word(res, rootblocksizetag, (WORD)(res->size - rootblocksizetag));

	SetResSize(res, restag);
	if(win32)
		put_pad(res);

	free(vsvi.str.cstr);
	return res;
}

/*
 *****************************************************************************
 * Function	: toolbaritem2res
 * Syntax	: void toolbaritem2res(res_t *res, toolbar_item_t *tbitem)
 * Input	:
 * Output	: nop
 * Description	:
 * Remarks	: Self recursive
 *****************************************************************************
*/
static void toolbaritem2res(res_t *res, toolbar_item_t *tbitem)
{
	toolbar_item_t *itm = tbitem;
	assert(win32 != 0);
	while(itm)
	{
		put_word(res, itm->id);
		itm = itm->next;
	}

}

/*
 *****************************************************************************
 * Function	: toolbar2res
 * Syntax	: res_t *toolbar2res(name_id_t *name, toolbar_t *toolbar)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	toolbar	- The toolbar descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *toolbar2res(name_id_t *name, toolbar_t *toolbar)
{
	int restag;
	res_t *res;
	assert(name != NULL);
	assert(toolbar != NULL);

	res = new_res();
	if(win32)
	{
		restag = put_res_header(res, WRC_RT_TOOLBAR, NULL, name, toolbar->memopt, &(toolbar->lvc));

		put_word(res, 1);		/* Menuheader: Version */
		put_word(res, toolbar->button_width); /* (in pixels?) */
		put_word(res, toolbar->button_height); /* (in pixels?) */
		put_word(res, toolbar->nitems);
		put_pad(res);
		toolbaritem2res(res, toolbar->items);
		/* Set ResourceSize */
		SetResSize(res, restag);
		put_pad(res);
	}
	else /* win16 */
	{
		/* Do not generate anything in 16-bit mode */
		free(res->data);
		free(res);
		return NULL;
	}
	return res;
}

/*
 *****************************************************************************
 * Function	: dlginit2res
 * Syntax	: res_t *dlginit2res(name_id_t *name, dlginit_t *dit)
 * Input	:
 *	name	- Name/ordinal of the resource
 *	rdt	- The dlginit descriptor
 * Output	: New .res format structure
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_t *dlginit2res(name_id_t *name, dlginit_t *dit)
{
	int restag;
	res_t *res;
	assert(name != NULL);
	assert(dit != NULL);

	res = new_res();
	restag = put_res_header(res, WRC_RT_DLGINIT, NULL, name, dit->memopt, &(dit->data->lvc));
	put_raw_data(res, dit->data, 0);
	/* Set ResourceSize */
	SetResSize(res, restag);
	if(win32)
		put_pad(res);
	return res;
}

/*
 *****************************************************************************
 * Function	: prep_nid_for_label
 * Syntax	: char *prep_nid_for_label(const name_id_t *nid)
 * Input	:
 * Output	:
 * Description	: Converts a resource name into the first 32 (or less)
 *		  characters of the name with conversions.
 * Remarks	:
 *****************************************************************************
*/
#define MAXNAMELEN	32
char *prep_nid_for_label(const name_id_t *nid)
{
	static char buf[MAXNAMELEN+1];

	assert(nid != NULL);

	if(nid->type == name_str && nid->name.s_name->type == str_unicode)
	{
		WCHAR *sptr;
		int i;
		sptr = nid->name.s_name->str.wstr;
		buf[0] = '\0';
		for(i = 0; *sptr && i < MAXNAMELEN; i++)
		{
			if((unsigned)*sptr < 0x80 && isprint(*sptr & 0xff))
				buf[i] = *sptr++;
			else
				warning("Resourcename (str_unicode) contain unprintable characters or invalid translation, ignored");
		}
		buf[i] = '\0';
	}
	else if(nid->type == name_str && nid->name.s_name->type == str_char)
	{
		char *cptr;
		int i;
		cptr = nid->name.s_name->str.cstr;
		buf[0] = '\0';
		for(i = 0; *cptr && i < MAXNAMELEN; i++)
		{
			if((unsigned)*cptr < 0x80 && isprint(*cptr & 0xff))
				buf[i] = *cptr++;
			else
				warning("Resourcename (str_char) contain unprintable characters, ignored");
		}
		buf[i] = '\0';
	}
	else if(nid->type == name_ord)
	{
		sprintf(buf, "%u", nid->name.i_name);
	}
	else
	{
		internal_error(__FILE__, __LINE__, "Resource name_id with invalid type %d", nid->type);
	}
	return buf;
}
#undef MAXNAMELEN

/*
 *****************************************************************************
 * Function	: make_c_name
 * Syntax	: char *make_c_name(const char *base, const name_id_t *nid, const language_t *lan)
 * Input	:
 * Output	:
 * Description	: Converts a resource name into a valid c-identifier in the
 *		  form "_base_nid".
 * Remarks	:
 *****************************************************************************
*/
char *make_c_name(const char *base, const name_id_t *nid, const language_t *lan)
{
	int nlen;
	char *buf;
	char *ret;
	char lanbuf[6];

	sprintf(lanbuf, "%d", lan ? MAKELANGID(lan->id, lan->sub) : 0);
	buf = prep_nid_for_label(nid);
	nlen = strlen(buf) + strlen(lanbuf);
	nlen += strlen(base) + 4; /* three time '_' and '\0' */
	ret = (char *)xmalloc(nlen);
	strcpy(ret, "_");
	strcat(ret, base);
	strcat(ret, "_");
	strcat(ret, buf);
	strcat(ret, "_");
	strcat(ret, lanbuf);
	return ret;
}

/*
 *****************************************************************************
 * Function	: get_c_typename
 * Syntax	: const char *get_c_typename(enum res_e type)
 * Input	:
 * Output	:
 * Description	: Convert resource enum to char string to be used in c-name
 *		  creation.
 * Remarks	:
 *****************************************************************************
*/
const char *get_c_typename(enum res_e type)
{
	switch(type)
	{
	case res_acc:	return "Acc";
	case res_anicur:return "AniCur";
	case res_aniico:return "AniIco";
	case res_bmp:	return "Bmp";
	case res_cur:	return "Cur";
	case res_curg:	return "CurGrp";
	case res_dlg:
	case res_dlgex:	return "Dlg";
	case res_fnt:	return "Fnt";
	case res_fntdir:return "FntDir";
	case res_ico:	return "Ico";
	case res_icog:	return "IcoGrp";
	case res_men:
	case res_menex:	return "Men";
	case res_rdt:	return "RCDat";
	case res_stt:	return "StrTab";
	case res_usr:	return "Usr";
	case res_msg:	return "MsgTab";
	case res_ver:	return "VerInf";
	case res_toolbar:	return "TlBr";
	case res_dlginit: return "DlgInit";
	default:	return "Oops";
	}
}

/*
 *****************************************************************************
 * Function	: resources2res
 * Syntax	: void resources2res(resource_t *top)
 * Input	:
 *	top	- The resource-tree to convert
 * Output	:
 * Description	: Convert logical resource descriptors into binary data
 * Remarks	:
 *****************************************************************************
*/
void resources2res(resource_t *top)
{
	while(top)
	{
		switch(top->type)
		{
		case res_acc:
			if(!top->binres)
				top->binres = accelerator2res(top->name, top->res.acc);
			break;
		case res_bmp:
			if(!top->binres)
				top->binres = bitmap2res(top->name, top->res.bmp);
			break;
		case res_cur:
			if(!top->binres)
				top->binres = cursor2res(top->res.cur);
			break;
		case res_curg:
			if(!top->binres)
				top->binres = cursorgroup2res(top->name, top->res.curg);
			break;
		case res_dlg:
			if(!top->binres)
				top->binres = dialog2res(top->name, top->res.dlg);
			break;
		case res_dlgex:
			if(!top->binres)
				top->binres = dialogex2res(top->name, top->res.dlgex);
			break;
		case res_fnt:
			if(!top->binres)
				top->binres = font2res(top->name, top->res.fnt);
			break;
		case res_fntdir:
			if(!top->binres)
				top->binres = fontdir2res(top->name, top->res.fnd);
			break;
		case res_ico:
			if(!top->binres)
				top->binres = icon2res(top->res.ico);
			break;
		case res_icog:
			if(!top->binres)
				top->binres = icongroup2res(top->name, top->res.icog);
			break;
		case res_men:
			if(!top->binres)
				top->binres = menu2res(top->name, top->res.men);
			break;
		case res_menex:
			if(!top->binres)
				top->binres = menuex2res(top->name, top->res.menex);
			break;
		case res_html:
			if(!top->binres)
				top->binres = html2res(top->name, top->res.html);
			break;
		case res_rdt:
			if(!top->binres)
				top->binres = rcdata2res(top->name, top->res.rdt);
			break;
		case res_stt:
			if(!top->binres)
				top->binres = stringtable2res(top->res.stt);
			break;
		case res_usr:
			if(!top->binres)
				top->binres = user2res(top->name, top->res.usr);
			break;
		case res_msg:
			if(!top->binres)
				top->binres = messagetable2res(top->name, top->res.msg);
			break;
		case res_ver:
			if(!top->binres)
				top->binres = versioninfo2res(top->name, top->res.ver);
			break;
		case res_toolbar:
			if(!top->binres)
				top->binres = toolbar2res(top->name, top->res.tbt);
			break;
		case res_dlginit:
			if(!top->binres)
			    top->binres = dlginit2res(top->name, top->res.dlgi);
			break;
		case res_anicur:
		case res_aniico:
			if(!top->binres)
			    top->binres = anicurico2res(top->name, top->res.ani, top->type);
			break;
		default:
			internal_error(__FILE__, __LINE__, "Unknown resource type encountered %d in binary res generation", top->type);
		}
		top->c_name = make_c_name(get_c_typename(top->type), top->name, top->lan);
		top = top->next;
	}
}
