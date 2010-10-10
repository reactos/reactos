/*
 * Copyright 2003 Vincent Béron
 * Copyright 2007, 2008 Mikolaj Zalewski
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dumpres.h"
#include "utils.h"
#include "wrc.h"

#define MASTER_LANGUAGE LANG_ENGLISH
#define MASTER_SUBLANGUAGE SUBLANG_ENGLISH_US
#define NB_LANG 0x94

enum lang_type_e {
	lang_type_master = 0,
	lang_type_neutral,
	lang_type_normal
};

static language_t get_language(resource_t *resource) {
	switch(resource->type) {
		case res_acc:
			return *resource->res.acc->lvc.language;
		case res_bmp:
			return *resource->res.bmp->data->lvc.language;
		case res_cur:
			return *resource->res.cur->lvc.language;
		case res_curg:
			return *resource->res.curg->lvc.language;
		case res_dlg:
			return *resource->res.dlg->lvc.language;
		case res_dlgex:
			return *resource->res.dlgex->lvc.language;
		case res_fnt:
			return *resource->res.fnt->data->lvc.language;
		case res_fntdir:
			return *resource->res.fnd->data->lvc.language;
		case res_ico:
			return *resource->res.ico->lvc.language;
		case res_icog:
			return *resource->res.icog->lvc.language;
		case res_men:
			return *resource->res.men->lvc.language;
		case res_menex:
			return *resource->res.menex->lvc.language;
		case res_rdt:
			return *resource->res.rdt->data->lvc.language;
		case res_stt:
			return *resource->res.stt->lvc.language;
		case res_usr:
			return *resource->res.usr->data->lvc.language;
		case res_msg:
			return *resource->res.msg->data->lvc.language;
		case res_ver:
			return *resource->res.ver->lvc.language;
		case res_dlginit:
			return *resource->res.dlgi->data->lvc.language;
		case res_toolbar:
			return *resource->res.tbt->lvc.language;
		case res_anicur:
		case res_aniico:
			return *resource->res.ani->data->lvc.language;
                case res_html:
                        return *resource->res.html->data->lvc.language;
		default:
			/* Not supposed to reach here */
			fprintf(stderr, "Not supposed to reach here (get_language_id())\n");
			abort();
	}
}

static int get_language_id(resource_t *resource) {
    return get_language(resource).id;
}

static int compare_lang(language_t lang1, language_t lang2)
{
    return memcmp(&lang1, &lang2, sizeof(language_t));
}

#if 0

#define PRETTYPRINTLANG(langid) \
	if(LANG_##langid == lid) { \
		return #langid; \
	}

static const char *get_language_name(int lid) {
	PRETTYPRINTLANG(NEUTRAL)
	PRETTYPRINTLANG(AFRIKAANS)
	PRETTYPRINTLANG(ALBANIAN)
	PRETTYPRINTLANG(ARABIC)
	PRETTYPRINTLANG(ARMENIAN)
	PRETTYPRINTLANG(ASSAMESE)
	PRETTYPRINTLANG(AZERI)
	PRETTYPRINTLANG(BASQUE)
	PRETTYPRINTLANG(BELARUSIAN)
	PRETTYPRINTLANG(BENGALI)
	PRETTYPRINTLANG(BULGARIAN)
	PRETTYPRINTLANG(CATALAN)
	PRETTYPRINTLANG(CHINESE)
	PRETTYPRINTLANG(CROATIAN)
	PRETTYPRINTLANG(CZECH)
	PRETTYPRINTLANG(DANISH)
	PRETTYPRINTLANG(DIVEHI)
	PRETTYPRINTLANG(DUTCH)
	PRETTYPRINTLANG(ENGLISH)
	PRETTYPRINTLANG(ESTONIAN)
	PRETTYPRINTLANG(FAEROESE)
	PRETTYPRINTLANG(FARSI)
	PRETTYPRINTLANG(FINNISH)
	PRETTYPRINTLANG(FRENCH)
	PRETTYPRINTLANG(GALICIAN)
	PRETTYPRINTLANG(GEORGIAN)
	PRETTYPRINTLANG(GERMAN)
	PRETTYPRINTLANG(GREEK)
	PRETTYPRINTLANG(GUJARATI)
	PRETTYPRINTLANG(HEBREW)
	PRETTYPRINTLANG(HINDI)
	PRETTYPRINTLANG(HUNGARIAN)
	PRETTYPRINTLANG(ICELANDIC)
	PRETTYPRINTLANG(INDONESIAN)
	PRETTYPRINTLANG(ITALIAN)
	PRETTYPRINTLANG(JAPANESE)
	PRETTYPRINTLANG(KANNADA)
	PRETTYPRINTLANG(KASHMIRI)
	PRETTYPRINTLANG(KAZAK)
	PRETTYPRINTLANG(KONKANI)
	PRETTYPRINTLANG(KOREAN)
	PRETTYPRINTLANG(KYRGYZ)
	PRETTYPRINTLANG(LATVIAN)
	PRETTYPRINTLANG(LITHUANIAN)
	PRETTYPRINTLANG(MACEDONIAN)
	PRETTYPRINTLANG(MALAY)
	PRETTYPRINTLANG(MALAYALAM)
	PRETTYPRINTLANG(MANIPURI)
	PRETTYPRINTLANG(MARATHI)
	PRETTYPRINTLANG(MONGOLIAN)
	PRETTYPRINTLANG(NEPALI)
	PRETTYPRINTLANG(NORWEGIAN)
	PRETTYPRINTLANG(ORIYA)
	PRETTYPRINTLANG(POLISH)
	PRETTYPRINTLANG(PORTUGUESE)
	PRETTYPRINTLANG(PUNJABI)
	PRETTYPRINTLANG(ROMANIAN)
	PRETTYPRINTLANG(RUSSIAN)
	PRETTYPRINTLANG(SANSKRIT)
	PRETTYPRINTLANG(SERBIAN)
	PRETTYPRINTLANG(SINDHI)
	PRETTYPRINTLANG(SLOVAK)
	PRETTYPRINTLANG(SLOVENIAN)
	PRETTYPRINTLANG(SPANISH)
	PRETTYPRINTLANG(SWAHILI)
	PRETTYPRINTLANG(SWEDISH)
	PRETTYPRINTLANG(SYRIAC)
	PRETTYPRINTLANG(TAMIL)
	PRETTYPRINTLANG(TATAR)
	PRETTYPRINTLANG(TELUGU)
	PRETTYPRINTLANG(THAI)
	PRETTYPRINTLANG(TURKISH)
	PRETTYPRINTLANG(UKRAINIAN)
	PRETTYPRINTLANG(URDU)
	PRETTYPRINTLANG(UZBEK)
	PRETTYPRINTLANG(VIETNAMESE)
	PRETTYPRINTLANG(GAELIC)
	PRETTYPRINTLANG(MALTESE)
	PRETTYPRINTLANG(MAORI)
	PRETTYPRINTLANG(RHAETO_ROMANCE)
	PRETTYPRINTLANG(SAAMI)
	PRETTYPRINTLANG(SORBIAN)
	PRETTYPRINTLANG(SUTU)
	PRETTYPRINTLANG(TSONGA)
	PRETTYPRINTLANG(TSWANA)
	PRETTYPRINTLANG(VENDA)
	PRETTYPRINTLANG(XHOSA)
	PRETTYPRINTLANG(ZULU)
	PRETTYPRINTLANG(ESPERANTO)
	PRETTYPRINTLANG(WALON)
	PRETTYPRINTLANG(CORNISH)
	PRETTYPRINTLANG(WELSH)
	PRETTYPRINTLANG(BRETON)
	return "Unknown language";
}
#endif

static int compare_accelerator(accelerator_t *accelerator1, accelerator_t *accelerator2) {
	int different = 0;
	event_t *ev1 = NULL, *ev2 = NULL;
	if(!different &&
	   ((accelerator1->memopt != accelerator2->memopt) ||
	   (accelerator1->lvc.version != accelerator2->lvc.version) ||
	   (accelerator1->lvc.characts != accelerator2->lvc.characts)))
		different = 1;
	ev1 = accelerator1->events;
	ev2 = accelerator2->events;
	while(!different && ev1 && ev2) {
		if(!different &&
		   ((ev1->id != ev2->id) ||
		   (ev1->flags != ev2->flags)))
			different = 1;
		ev1 = ev1->next;
		ev2 = ev2->next;
	}
	if(!different &&
	   ((ev1 && !ev2) || (!ev1 && ev2)))
		different = 1;
	return different;
}

static int compare_bitmap(bitmap_t *bitmap1, bitmap_t *bitmap2) {
	int different = 0;
	if(!different &&
	   ((bitmap1->memopt != bitmap2->memopt) ||
	   (bitmap1->data->lvc.version != bitmap2->data->lvc.version) ||
	   (bitmap1->data->lvc.characts != bitmap2->data->lvc.characts)))
		different = 1;
	return different;
}

static int compare_cursor(cursor_t *cursor1, cursor_t *cursor2) {
	int different = 0;
	if(!different &&
	   ((cursor1->id != cursor2->id) ||
	   (cursor1->width != cursor2->width) ||
	   (cursor1->height != cursor2->height) ||
	   (cursor1->xhot != cursor2->xhot) ||
	   (cursor1->yhot != cursor2->yhot)))
		different = 1;
	if(!different &&
	   ((cursor1->lvc.version != cursor2->lvc.version) ||
	   (cursor1->lvc.characts != cursor2->lvc.characts)))
		different = 1;
	return different;
}

static int compare_cursor_group(cursor_group_t *cursor_group1, cursor_group_t *cursor_group2) {
	int different = 0;
	cursor_t *cursor1 = NULL, *cursor2 = NULL;
	if(!different &&
	   ((cursor_group1->memopt != cursor_group2->memopt) ||
	   (cursor_group1->lvc.version != cursor_group2->lvc.version) ||
	   (cursor_group1->lvc.characts != cursor_group2->lvc.characts)))
		different = 1;
	if(!different &&
	   (cursor_group1->ncursor != cursor_group2->ncursor))
		different = 1;
	if(!different) {
		cursor1 = cursor_group1->cursorlist;
		cursor2 = cursor_group2->cursorlist;
		while(!different && cursor1 && cursor2) {
			different = compare_cursor(cursor1, cursor2);
			cursor1 = cursor1->next;
			cursor2 = cursor2->next;
		}
		if(!different &&
		   ((cursor1 && !cursor2) ||
		   (!cursor1 && cursor2)))
			different = 1;
	}
	return different;
}

static int compare_control(control_t *control1, control_t *control2) {
	int different = 0;
	char *nameid = NULL;
	int ignore_style;
	if(!different &&
		((control1 && !control2) ||
		(!control1 && control2)))
			different = 1;
	if(different || !control1 || !control2)
		return different;
	nameid = strdup(get_nameid_str(control1->ctlclass));
	if(!different && strcmp(nameid, get_nameid_str(control2->ctlclass)))
		different = 1;
	free(nameid);
        if (different)
            return different;

        /* allow the translators to set some styles */
        ignore_style = 0;
        if (control1->ctlclass->type == name_ord && control1->ctlclass->name.i_name == CT_BUTTON)
            ignore_style = 0x2000;          /* BS_MULTILINE*/

	if(!different && 
	   (control1->id != control2->id))
		different = 1;
	if(!different && control1->gotstyle && control2->gotstyle) {
		if((!control1->style || !control2->style) ||
		   (control1->style->and_mask || control2->style->and_mask) ||
		   ((control1->style->or_mask & ~ignore_style) != (control2->style->or_mask & ~ignore_style)))
			different = 1;
	} else if(!different &&
		  ((control1->gotstyle && !control2->gotstyle) ||
		  (!control1->gotstyle && control2->gotstyle)))
			different = 1;
	if(!different && control1->gotexstyle && control2->gotexstyle) {
		if((!control1->exstyle || !control2->exstyle) ||
		   (control1->exstyle->and_mask || control2->exstyle->and_mask) ||
		   (control1->exstyle->or_mask != control2->exstyle->or_mask))
			different = 1;
	} else if(!different &&
		  ((control1->gotexstyle && !control2->gotexstyle) ||
		  (!control1->gotexstyle && control2->gotexstyle)))
			different = 1;
	if(!different && control1->gothelpid && control2->gothelpid) {
		if(control1->helpid != control2->helpid)
			different = 1;
	} else if(!different &&
		  ((control1->gothelpid && !control2->gothelpid) ||
		  (!control1->gothelpid && control2->gothelpid)))
			different = 1;
	return different;
}

static int compare_dialog(dialog_t *dialog1, dialog_t *dialog2) {
	int different = 0;
	char *nameid = NULL;
	control_t *ctrl1, *ctrl2;
	if(!different &&
	   ((dialog1->memopt != dialog2->memopt) ||
	   (dialog1->lvc.version != dialog2->lvc.version) ||
	   (dialog1->lvc.characts != dialog2->lvc.characts)))
		different = 1;
	if(!different && dialog1->gotstyle && dialog2->gotstyle) {
		if((!dialog1->style || !dialog2->style) ||
		   (dialog1->style->and_mask || dialog2->style->and_mask) ||
		   (dialog1->style->or_mask != dialog2->style->or_mask))
			different = 1;
	} else if(!different &&
		  ((dialog1->gotstyle && !dialog2->gotstyle) ||
		  (!dialog1->gotstyle && dialog2->gotstyle)))
			different = 1;
	if(!different && dialog1->gotexstyle && dialog2->gotexstyle) {
		if((!dialog1->exstyle || !dialog2->exstyle) ||
		   (dialog1->exstyle->and_mask || dialog2->exstyle->and_mask) ||
		   (dialog1->exstyle->or_mask != dialog2->exstyle->or_mask))
			different = 1;
	} else if(!different &&
		  ((dialog1->gotexstyle && !dialog2->gotexstyle) ||
		  (!dialog1->gotexstyle && dialog2->gotexstyle)))
			different = 1;
	nameid = strdup(get_nameid_str(dialog1->menu));
	if(!different && strcmp(nameid, get_nameid_str(dialog2->menu)))
		different = 1;
	free(nameid);
	nameid = strdup(get_nameid_str(dialog1->dlgclass));
	if(!different && strcmp(nameid, get_nameid_str(dialog2->dlgclass)))
		different = 1;
	free(nameid);

        ctrl1 = dialog1->controls;
        ctrl2 = dialog2->controls;
        while(!different && (ctrl1 || ctrl2))
        {
            different = compare_control(ctrl1, ctrl2);
            if (ctrl1) ctrl1 = ctrl1->next;
            if (ctrl2) ctrl2 = ctrl2->next;
        }
	return different;
}

static int compare_dialogex(dialogex_t *dialogex1, dialogex_t *dialogex2) {
	int different = 0;
	char *nameid = NULL;
	control_t *ctrl1, *ctrl2;
	if(!different &&
	   ((dialogex1->memopt != dialogex2->memopt) ||
	   (dialogex1->lvc.version != dialogex2->lvc.version) ||
	   (dialogex1->lvc.characts != dialogex2->lvc.characts)))
		different = 1;
	if(!different && dialogex1->gotstyle && dialogex2->gotstyle) {
		if((!dialogex1->style || !dialogex2->style) ||
		   (dialogex1->style->and_mask || dialogex2->style->and_mask) ||
		   (dialogex1->style->or_mask != dialogex2->style->or_mask))
			different = 1;
	} else if(!different &&
		  ((dialogex1->gotstyle && !dialogex2->gotstyle) ||
		  (!dialogex1->gotstyle && dialogex2->gotstyle)))
			different = 1;
	if(!different && dialogex1->gotexstyle && dialogex2->gotexstyle) {
		if((!dialogex1->exstyle || !dialogex2->exstyle) ||
		   (dialogex1->exstyle->and_mask || dialogex2->exstyle->and_mask) ||
		   (dialogex1->exstyle->or_mask != dialogex2->exstyle->or_mask))
			different = 1;
	} else if(!different &&
		  ((dialogex1->gotexstyle && !dialogex2->gotexstyle) ||
		  (!dialogex1->gotexstyle && dialogex2->gotexstyle)))
			different = 1;
	if(!different && dialogex1->gothelpid && dialogex2->gothelpid) {
		if(dialogex1->helpid != dialogex2->helpid)
			different = 1;
	} else if(!different &&
		  ((dialogex1->gothelpid && !dialogex2->gothelpid) ||
		  (!dialogex1->gothelpid && dialogex2->gothelpid)))
			different = 1;
	nameid = strdup(get_nameid_str(dialogex1->menu));
	if(!different && strcmp(nameid, get_nameid_str(dialogex2->menu)))
		different = 1;
	free(nameid);
	nameid = strdup(get_nameid_str(dialogex1->dlgclass));
	if(!different && strcmp(nameid, get_nameid_str(dialogex2->dlgclass)))
		different = 1;
	free(nameid);

        ctrl1 = dialogex1->controls;
        ctrl2 = dialogex2->controls;
        while(!different && (ctrl1 || ctrl2))
        {
            different = compare_control(ctrl1, ctrl2);
            if (ctrl1) ctrl1 = ctrl1->next;
            if (ctrl2) ctrl2 = ctrl2->next;
        }
	return different;
}

static int compare_font(font_t *font1, font_t *font2) {
	int different = 0;
	if(!different &&
	   ((font1->memopt != font2->memopt) ||
	   (font1->data->lvc.version != font2->data->lvc.version) ||
	   (font1->data->lvc.characts != font2->data->lvc.characts)))
		different = 1;
	return different;
}

static int compare_fontdir(fontdir_t *fontdir1, fontdir_t *fontdir2) {
	int different = 0;
	if(!different &&
	   ((fontdir1->memopt != fontdir2->memopt) ||
	   (fontdir1->data->lvc.version != fontdir2->data->lvc.version) ||
	   (fontdir1->data->lvc.characts != fontdir2->data->lvc.characts)))
		different = 1;
	return different;
}

static int compare_icon(icon_t *icon1, icon_t *icon2) {
	int different = 0;
	if(!different &&
	   ((icon1->id != icon2->id) ||
	   (icon1->width != icon2->width) ||
	   (icon1->height != icon2->height)))
		different = 1;
	if(!different &&
	   ((icon1->lvc.version != icon2->lvc.version) ||
	   (icon1->lvc.characts != icon2->lvc.characts)))
		different = 1;
	return different;
}

static int compare_icon_group(icon_group_t *icon_group1, icon_group_t *icon_group2) {
	int different = 0;
	icon_t *icon1 = NULL, *icon2 = NULL;
	if(!different &&
	   ((icon_group1->memopt != icon_group2->memopt) ||
	   (icon_group1->lvc.version != icon_group2->lvc.version) ||
	   (icon_group1->lvc.characts != icon_group2->lvc.characts)))
		different = 1;
	if(!different &&
	   (icon_group1->nicon != icon_group2->nicon))
		different = 1;
	if(!different) {
		icon1 = icon_group1->iconlist;
		icon2 = icon_group2->iconlist;
		while(!different && icon1 && icon2) {
			different = compare_icon(icon1, icon2);
			icon1 = icon1->next;
			icon2 = icon2->next;
		}
		if(!different &&
		   ((icon1 && !icon2) ||
		   (!icon1 && icon2)))
			different = 1;
	}
	return different;
}

static int compare_menu_item(menu_item_t *menu_item1, menu_item_t *menu_item2) {
	int different = 0;
	while(!different && menu_item1 && menu_item2) {
		if(menu_item1->popup && menu_item2->popup)
			different = compare_menu_item(menu_item1->popup, menu_item2->popup);
		else if(!menu_item1->popup && !menu_item2->popup) {
			if(menu_item1->name && menu_item2->name) {
				if((menu_item1->id != menu_item2->id) ||
				   (menu_item1->state != menu_item2->state))
					different = 1;
			} else if((menu_item1->name && !menu_item2->name) ||
				  (!menu_item1->name && menu_item2->name))
					different = 1;
		} else
			different = 1;
		menu_item1 = menu_item1->next;
		menu_item2 = menu_item2->next;
	}
	if(!different &&
	   ((menu_item1 && !menu_item2) ||
	   (!menu_item1 && menu_item2)))
		different = 1;
	return different;
}

static int compare_menu(menu_t *menu1, menu_t *menu2) {
	int different = 0;
	if(!different &&
	   ((menu1->memopt != menu2->memopt) ||
	   (menu1->lvc.version != menu2->lvc.version) ||
	   (menu1->lvc.characts != menu2->lvc.characts)))
		different = 1;
	if(!different)
		different = compare_menu_item(menu1->items, menu2->items);
	return different;
}

static int compare_menuex_item(menuex_item_t *menuex_item1, menuex_item_t *menuex_item2) {
	int different = 0;
	while(!different && menuex_item1 && menuex_item2) {
		if(menuex_item1->popup && menuex_item2->popup) {
			if(!different && menuex_item1->gotid && menuex_item2->gotid) {
				if(menuex_item1->id != menuex_item2->id)
					different = 1;
			} else if(!different &&
				  ((menuex_item1->gotid && !menuex_item2->gotid) ||
				  (!menuex_item2->gotid && menuex_item2->gotid)))
					different = 1;
			if(!different && menuex_item1->gottype && menuex_item2->gottype) {
				if(menuex_item1->type != menuex_item2->type)
					different = 1;
			} else if(!different &&
				  ((menuex_item1->gottype && !menuex_item2->gottype) ||
				  (!menuex_item2->gottype && menuex_item2->gottype)))
					different = 1;
			if(!different && menuex_item1->gotstate && menuex_item2->gotstate) {
				if(menuex_item1->state != menuex_item2->state)
					different = 1;
			} else if(!different &&
				  ((menuex_item1->gotstate && !menuex_item2->gotstate) ||
				  (!menuex_item2->gotstate && menuex_item2->gotstate)))
					different = 1;
			if(!different && menuex_item1->gothelpid && menuex_item2->gothelpid) {
				if(menuex_item1->helpid != menuex_item2->helpid)
					different = 1;
			} else if(!different &&
				  ((menuex_item1->gothelpid && !menuex_item2->gothelpid) ||
				  (!menuex_item2->gothelpid && menuex_item2->gothelpid)))
					different = 1;
			if(!different)
				different = compare_menuex_item(menuex_item1->popup, menuex_item2->popup);
		} else if(!menuex_item1->popup && !menuex_item2->popup) {
			if(menuex_item1->name && menuex_item2->name) {
				if(!different && menuex_item1->gotid && menuex_item2->gotid) {
					if(menuex_item1->id != menuex_item2->id)
						different = 1;
				} else if(!different &&
					  ((menuex_item1->gotid && !menuex_item2->gotid) ||
					  (!menuex_item2->gotid && menuex_item2->gotid)))
						different = 1;
				if(!different && menuex_item1->gottype && menuex_item2->gottype) {
					if(menuex_item1->type != menuex_item2->type)
						different = 1;
				} else if(!different &&
					  ((menuex_item1->gottype && !menuex_item2->gottype) ||
					  (!menuex_item2->gottype && menuex_item2->gottype)))
						different = 1;
				if(!different && menuex_item1->gotstate && menuex_item2->gotstate) {
					if(menuex_item1->state != menuex_item2->state)
						different = 1;
				} else if(!different &&
					  ((menuex_item1->gotstate && !menuex_item2->gotstate) ||
					  (!menuex_item2->gotstate && menuex_item2->gotstate)))
						different = 1;
				if(!different && menuex_item1->gothelpid && menuex_item2->gothelpid) {
					if(menuex_item1->helpid != menuex_item2->helpid)
						different = 1;
				} else if(!different &&
					  ((menuex_item1->gothelpid && !menuex_item2->gothelpid) ||
					  (!menuex_item2->gothelpid && menuex_item2->gothelpid)))
						different = 1;
			} else if((menuex_item1->name && !menuex_item2->name) ||
				  (!menuex_item1->name && menuex_item2->name))
					different = 1;
		} else
			different = 1;
		menuex_item1 = menuex_item1->next;
		menuex_item2 = menuex_item2->next;
	}
	if(!different &&
	   ((menuex_item1 && !menuex_item2) ||
	   (!menuex_item1 && menuex_item2)))
		different = 1;
	return different;
}

static int compare_menuex(menuex_t *menuex1, menuex_t *menuex2) {
	int different = 0;
	if(!different &&
	   ((menuex1->memopt != menuex2->memopt) ||
	   (menuex1->lvc.version != menuex2->lvc.version) ||
	   (menuex1->lvc.characts != menuex2->lvc.characts)))
		different = 1;
	if(!different)
		different = compare_menuex_item(menuex1->items, menuex2->items);
	return different;
}

static int compare_rcdata(rcdata_t *rcdata1, rcdata_t *rcdata2) {
	int different = 0;
	if(!different &&
	   ((rcdata1->memopt != rcdata2->memopt) ||
	   (rcdata1->data->lvc.version != rcdata2->data->lvc.version) ||
	   (rcdata1->data->lvc.characts != rcdata2->data->lvc.characts)))
		different = 1;
	return different;
}

static int compare_html(html_t *rcdata1, html_t *rcdata2) {
        int different = 0;
        if(!different &&
           ((rcdata1->memopt != rcdata2->memopt) ||
           (rcdata1->data->lvc.version != rcdata2->data->lvc.version) ||
           (rcdata1->data->lvc.characts != rcdata2->data->lvc.characts)))
                different = 1;
        return different;
}

static int compare_stringtable(stringtable_t *stringtable1, stringtable_t *stringtable2) {
	int different = 0;
	int i;
	while(!different && stringtable1 && stringtable2) {
		if((stringtable1->memopt != stringtable2->memopt) ||
		   (stringtable1->lvc.version != stringtable2->lvc.version) ||
		   (stringtable1->lvc.characts != stringtable2->lvc.characts))
			different = 1;
		if(!different) {
			if((stringtable1->nentries != stringtable2->nentries) ||
			   (stringtable1->idbase != stringtable2->idbase))
				different = 1;
			else
				for(i = 0 ; i < stringtable1->nentries; i++)
					if((stringtable1->entries[i].id != stringtable2->entries[i].id) ||
					   (stringtable1->entries[i].memopt != stringtable2->entries[i].memopt) ||
					   (stringtable1->entries[i].str && !stringtable2->entries[i].str) ||
					   (!stringtable1->entries[i].str && stringtable2->entries[i].str)) {
						different = 1;
						break;
					}
		}
		stringtable1 = stringtable1->next;
		stringtable2 = stringtable2->next;
	}
	return different;
}

static int compare_user(user_t *user1, user_t *user2) {
	int different = 0;
	char *nameid = NULL;
	if(!different &&
	   ((user1->memopt != user2->memopt) ||
	   (user1->data->lvc.version != user2->data->lvc.version) ||
	   (user1->data->lvc.characts != user2->data->lvc.characts)))
		different = 1;
	nameid = strdup(get_nameid_str(user1->type));
	if(!different && strcmp(nameid, get_nameid_str(user2->type)))
		different = 1;
	free(nameid);
	return different;
}

static int compare_messagetable(messagetable_t *messagetable1, messagetable_t *messagetable2) {
	int different = 0;
	if(!different &&
	   ((messagetable1->memopt != messagetable2->memopt) ||
	   (messagetable1->data->lvc.version != messagetable2->data->lvc.version) ||
	   (messagetable1->data->lvc.characts != messagetable2->data->lvc.characts)))
		different = 1;
	return different;
}

static int compare_string(string_t *string1, string_t *string2) {
	int different = 0;
	if(!different &&
	   ((string1->size != string2->size) ||
	   (string1->type != string2->type)))
		different = 1;
	if(!different) {
		if(string1->type == str_char)
			different = memcmp(string1->str.cstr, string2->str.cstr, string1->size);
		else if(string1->type == str_unicode)
			different = memcmp(string1->str.wstr, string2->str.wstr, string1->size*sizeof(WCHAR));
		else
			different = 1;
	}
	return different;
}

static int compare_ver_block(ver_block_t *ver_block1, ver_block_t *ver_block2);

static int compare_ver_value(ver_value_t *ver_value1, ver_value_t *ver_value2) {
	int different = 0;
	int i = 0;
	if(!different &&
	   (ver_value1->type == ver_value2->type)) {
		switch(ver_value1->type) {
			case val_str:
				if(!different && ver_value1->key && ver_value2->key)
					different = compare_string(ver_value1->key, ver_value2->key);
				else if(!different &&
					((ver_value1->key && !ver_value2->key) ||
					(!ver_value1->key && ver_value2->key)))
						different = 1;
				break;
			case val_words:
				if(!different && ver_value1->key && ver_value2->key)
					different = compare_string(ver_value1->key, ver_value2->key);
				else if(!different &&
					((ver_value1->key && !ver_value2->key) ||
					(!ver_value1->key && ver_value2->key)))
						different = 1;
				if(!different && ver_value1->value.words && ver_value2->value.words) {
					if(!different &&
					   (ver_value1->value.words->nwords != ver_value2->value.words->nwords))
						different = 1;
					if(!different)
						for(i = 0; i < ver_value1->value.words->nwords; i++) {
							if(ver_value1->value.words->words[i] != ver_value2->value.words->words[i]) {
								different = 1;
								break;
							}
						}
				} else if(!different &&
					  ((ver_value1->value.words && !ver_value2->value.words) ||
					  (!ver_value1->value.words && ver_value2->value.words)))
						different = 1;
				break;
			case val_block:
				if(!different && ver_value1->value.block && ver_value2->value.block)
					different = compare_ver_block(ver_value1->value.block, ver_value2->value.block);
				else if(!different &&
					((ver_value1->value.block && !ver_value2->value.block) ||
					(!ver_value1->value.block && ver_value2->value.block)))
						different = 1;
				break;
			default:
				different = 1;
		}
	} else
		different = 1;
	return different;
}

static int compare_ver_block(ver_block_t *ver_block1, ver_block_t *ver_block2) {
	int different = 0;
	ver_value_t *ver_value1 = NULL, *ver_value2 = NULL;
	if(!different) {
		ver_value1 = ver_block1->values;
		ver_value2 = ver_block2->values;
		while(!different && ver_value1 && ver_value2) {
			different = compare_ver_value(ver_value1, ver_value2);
			ver_value1 = ver_value1->next;
			ver_value2 = ver_value2->next;
		}
		if(!different &&
		   ((ver_value1 && !ver_value2) ||
		   (!ver_value1 && ver_value2)))
			different = 1;
	}
	return different;
}

static int compare_versioninfo(versioninfo_t *versioninfo1, versioninfo_t *versioninfo2) {
	int different = 0;
	ver_block_t *ver_block1 = NULL, *ver_block2 = NULL;
	if(!different &&
	   ((versioninfo1->memopt != versioninfo2->memopt) ||
	   (versioninfo1->lvc.version != versioninfo2->lvc.version) ||
	   (versioninfo1->lvc.characts != versioninfo2->lvc.characts)))
		different = 1;
	if(!different && versioninfo1->gotit.fv && versioninfo2->gotit.fv) {
		if((versioninfo1->filever_maj1 != versioninfo2->filever_maj1) ||
		   (versioninfo1->filever_maj2 != versioninfo2->filever_maj2) ||
		   (versioninfo1->filever_min1 != versioninfo2->filever_min1) ||
		   (versioninfo1->filever_min2 != versioninfo2->filever_min2))
			different = 1;
	} else if(!different &&
		  ((versioninfo1->gotit.fv && !versioninfo2->gotit.fv) ||
		  (!versioninfo1->gotit.fv && versioninfo2->gotit.fv)))
			different = 1;
	if(!different && versioninfo1->gotit.pv && versioninfo2->gotit.pv) {
		if((versioninfo1->prodver_maj1 != versioninfo2->prodver_maj1) ||
		   (versioninfo1->prodver_maj2 != versioninfo2->prodver_maj2) ||
		   (versioninfo1->prodver_min1 != versioninfo2->prodver_min1) ||
		   (versioninfo1->prodver_min2 != versioninfo2->prodver_min2))
			different = 1;
	} else if(!different &&
		  ((versioninfo1->gotit.pv && !versioninfo2->gotit.pv) ||
		  (!versioninfo1->gotit.pv && versioninfo2->gotit.pv)))
			different = 1;
	if(!different && versioninfo1->gotit.fo && versioninfo2->gotit.fo) {
		if(versioninfo1->fileos != versioninfo2->fileos)
			different = 1;
	} else if(!different &&
		  ((versioninfo1->gotit.fo && !versioninfo2->gotit.fo) ||
		  (!versioninfo1->gotit.fo && versioninfo2->gotit.fo)))
			different = 1;
	if(!different && versioninfo1->gotit.ff && versioninfo2->gotit.ff) {
		if(versioninfo1->fileflags != versioninfo2->fileflags)
			different = 1;
	} else if(!different &&
		  ((versioninfo1->gotit.ff && !versioninfo2->gotit.ff) ||
		  (!versioninfo1->gotit.ff && versioninfo2->gotit.ff)))
			different = 1;
	if(!different && versioninfo1->gotit.ffm && versioninfo2->gotit.ffm) {
		if(versioninfo1->fileflagsmask != versioninfo2->fileflagsmask)
			different = 1;
	} else if(!different &&
		  ((versioninfo1->gotit.ffm && !versioninfo2->gotit.ffm) ||
		  (!versioninfo1->gotit.ffm && versioninfo2->gotit.ffm)))
			different = 1;
	if(!different && versioninfo1->gotit.ft && versioninfo2->gotit.ft) {
		if(versioninfo1->filetype != versioninfo2->filetype)
			different = 1;
	} else if(!different &&
		  ((versioninfo1->gotit.ft && !versioninfo2->gotit.ft) ||
		  (!versioninfo1->gotit.ft && versioninfo2->gotit.ft)))
			different = 1;
	if(!different && versioninfo1->gotit.fst && versioninfo2->gotit.fst) {
		if(versioninfo1->filesubtype != versioninfo2->filesubtype)
			different = 1;
	} else if(!different &&
		  ((versioninfo1->gotit.fst && !versioninfo2->gotit.fst) ||
		  (!versioninfo1->gotit.fst && versioninfo2->gotit.fst)))
			different = 1;
	if(!different) {
		ver_block1 = versioninfo1->blocks;
		ver_block2 = versioninfo2->blocks;
		while(!different && ver_block1 && ver_block2) {
			different = compare_ver_block(ver_block1, ver_block2);
			ver_block1 = ver_block1->next;
			ver_block2 = ver_block2->next;
		}
		if(!different &&
		   ((ver_block1 && !ver_block2) ||
		   (!ver_block1 && ver_block2)))
			different = 1;
	}
	return different;
}

static int compare_dlginit(dlginit_t *dlginit1, dlginit_t *dlginit2) {
	int different = 0;
	if(!different &&
	   ((dlginit1->memopt != dlginit2->memopt) ||
	   (dlginit1->data->lvc.version != dlginit2->data->lvc.version) ||
	   (dlginit1->data->lvc.characts != dlginit2->data->lvc.characts)))
		different = 1;
	return different;
}

static int compare_toolbar_item(toolbar_item_t *toolbar_item1, toolbar_item_t *toolbar_item2) {
	int different = 0;
	while(!different && toolbar_item1 && toolbar_item2) {
		if((toolbar_item1->id && !toolbar_item2->id) ||
		   (!toolbar_item1->id && toolbar_item2->id))
			different = 1;
		toolbar_item1 = toolbar_item1->next;
		toolbar_item2 = toolbar_item2->next;
	}
	if(!different &&
	   ((toolbar_item1 && !toolbar_item2) ||
	   (!toolbar_item1 && toolbar_item2)))
		different = 1;
	return different;
}

static int compare_toolbar(toolbar_t *toolbar1, toolbar_t *toolbar2) {
	int different = 0;
	if(!different &&
	   ((toolbar1->memopt != toolbar2->memopt) ||
	   (toolbar1->lvc.version != toolbar2->lvc.version) ||
	   (toolbar1->lvc.characts != toolbar2->lvc.characts)))
		different = 1;
	if(!different)
		different = compare_toolbar_item(toolbar1->items, toolbar2->items);
	return different;
}

static int compare_ani_curico(ani_curico_t *ani_curico1, ani_curico_t *ani_curico2) {
	int different = 0;
	if(!different &&
	   ((ani_curico1->memopt != ani_curico2->memopt) ||
	   (ani_curico1->data->lvc.version != ani_curico2->data->lvc.version) ||
	   (ani_curico1->data->lvc.characts != ani_curico2->data->lvc.characts)))
		different = 1;
	return different;
}

static int compare(resource_t *resource1, resource_t *resource2) {
	switch(resource1->type) {
		case res_acc:
			return compare_accelerator(resource1->res.acc, resource2->res.acc);
		case res_bmp:
			return compare_bitmap(resource1->res.bmp, resource2->res.bmp);
		case res_cur:
			return compare_cursor(resource1->res.cur, resource2->res.cur);
		case res_curg:
			return compare_cursor_group(resource1->res.curg, resource2->res.curg);
		case res_dlg:
			return compare_dialog(resource1->res.dlg, resource2->res.dlg);
		case res_dlgex:
			return compare_dialogex(resource1->res.dlgex, resource2->res.dlgex);
		case res_fnt:
			return compare_font(resource1->res.fnt, resource2->res.fnt);
		case res_fntdir:
			return compare_fontdir(resource1->res.fnd, resource2->res.fnd);
		case res_ico:
			return compare_icon(resource1->res.ico, resource2->res.ico);
		case res_icog:
			return compare_icon_group(resource1->res.icog, resource2->res.icog);
		case res_men:
			return compare_menu(resource1->res.men, resource2->res.men);
		case res_menex:
			return compare_menuex(resource1->res.menex, resource2->res.menex);
		case res_rdt:
			return compare_rcdata(resource1->res.rdt, resource2->res.rdt);
		case res_stt:
			return compare_stringtable(resource1->res.stt, resource2->res.stt);
		case res_usr:
			return compare_user(resource1->res.usr, resource2->res.usr);
		case res_html:
		        return compare_html(resource1->res.html, resource2->res.html);
		case res_msg:
			return compare_messagetable(resource1->res.msg, resource2->res.msg);
		case res_ver:
			return compare_versioninfo(resource1->res.ver, resource2->res.ver);
		case res_dlginit:
			return compare_dlginit(resource1->res.dlgi, resource2->res.dlgi);
		case res_toolbar:
			return compare_toolbar(resource1->res.tbt, resource2->res.tbt);
		case res_anicur:
		case res_aniico:
			return compare_ani_curico(resource1->res.ani, resource2->res.ani);
		default:
			/* Not supposed to reach here */
			fprintf(stderr, "Not supposed to reach here (compare())\n");
			abort();
			return -1;
	}
}

typedef struct resource_lang_node
{
    language_t lang;
    resource_t *res;
    struct resource_lang_node *next;
} resource_lang_node_t;

typedef struct resource_id_node
{
    name_id_t *id;
    resource_lang_node_t *langs;
    struct resource_id_node *next;
} resource_id_node_t;

struct
{
    int enabled;
    struct resource_id_node *ids;
} verify_tab[res_usr+1];

static void add_resource(resource_t *res)
{
    resource_id_node_t *idnode;
    resource_lang_node_t *langnode;
    if (!verify_tab[res->type].enabled)
    {
	fprintf(stderr, "ERR: Report this: unknown resource type parsed %08x\n", res->type);
	return;
    }

    for (idnode = verify_tab[res->type].ids; idnode; idnode = idnode->next)
        if (compare_name_id(idnode->id, res->name) == 0)
            break;

    if (idnode == NULL)
    {
        idnode = xmalloc(sizeof(resource_id_node_t));
        idnode->id = res->name;
        idnode->langs = NULL;
        idnode->next = verify_tab[res->type].ids;
        verify_tab[res->type].ids = idnode;
    }

    for (langnode = idnode->langs; langnode; langnode = langnode->next)
        if (compare_lang(langnode->lang, get_language(res)) == 0)
        {
            fprintf(stderr, "ERR: resource %s [type %x] language %03x:%02x duplicated!\n",
                get_nameid_str(res->name), res->type, langnode->lang.id, langnode->lang.sub);
            return;
        }

    langnode = xmalloc(sizeof(resource_lang_node_t));
    langnode->res = res;
    langnode->lang = get_language(res);
    langnode->next = idnode->langs;
    idnode->langs = langnode;
}

static void setup_tabs(void)
{
    int i;

    for (i = 0; i <= res_usr; i++)
	switch(i) {
		case res_acc:
		case res_bmp:
		case res_cur:
		case res_curg:
		case res_dlg:
		case res_dlgex:
		case res_fnt:
		case res_fntdir:
		case res_ico:
		case res_icog:
		case res_men:
		case res_menex:
		case res_rdt:
		case res_stt:
		case res_usr:
		case res_msg:
		case res_ver:
		case res_dlginit:
		case res_toolbar:
		case res_anicur:
	        case res_aniico:
		case res_html:
		    verify_tab[i].enabled = 1;
		    break;
	}
}

static const char *get_typename_for_int(int type) {
    resource_t res;
    res.type = type;
    return get_typename(&res);
}

static resource_t *find_main(int type, name_id_t *id, resource_lang_node_t *langnode)
{
    resource_t *neutral = NULL, *en = NULL, *en_US = NULL;
    for (; langnode; langnode = langnode->next)
    {
        if (langnode->lang.id == LANG_NEUTRAL && langnode->lang.sub == SUBLANG_NEUTRAL)
            neutral = langnode->res;
        if (langnode->lang.id == MASTER_LANGUAGE && langnode->lang.sub == SUBLANG_NEUTRAL)
            en = langnode->res;
        if (langnode->lang.id == MASTER_LANGUAGE && langnode->lang.sub == MASTER_SUBLANGUAGE)
            en_US = langnode->res;
    }

    if (neutral != NULL && (en != NULL || en_US != NULL))
    {
        fprintf(stderr, "INFO: Resource %04x/%s has both NEUTRAL and MASTER language translarion\n",
            type, get_nameid_str(id));
    }

    if (en_US != NULL) return en_US;
    if (en != NULL) return en;
    return neutral;
}

void verify_translations(resource_t *top) {
    resource_t *curr = top;
    resource_id_node_t *idnode;
    resource_lang_node_t *langnode;
    int type;

    setup_tabs();
    while (curr)
    {
        add_resource(curr);
        curr = curr->next;
    }

    for (type = 0; type <= res_usr; type++)
    {
        printf("TYPE NEXT [%s]\n", get_typename_for_int(type));
        for (idnode = verify_tab[type].ids; idnode; idnode = idnode->next)
        {
            resource_t *mainres;
            printf("RESOURCE [%s]\n", get_nameid_str(idnode->id));

            mainres = find_main(type, idnode->id, idnode->langs);
            if (!mainres)
            {
                fprintf(stderr, "ERR: resource %04x/%s has translation(s) but not available in NEUTRAL or MASTER language\n",
                    type, get_nameid_str(idnode->id));
                for (langnode = idnode->langs; langnode; langnode = langnode->next)
                    printf("EXTRA %03x:%02x\n", langnode->lang.id, langnode->lang.sub);
                continue;
            }

            if (get_language_id(mainres) == LANG_NEUTRAL && idnode->langs->next == NULL) {
                printf("NOTRANSL\n");
                continue;
            }

            for (langnode = idnode->langs; langnode; langnode = langnode->next)
            {
                printf("EXIST %03x:%02x\n", langnode->lang.id, langnode->lang.sub);
                if (compare(langnode->res, mainres))
                {
                    printf("DIFF %03x:%02x\n", langnode->lang.id, langnode->lang.sub);
                }
            }
        }
    }
}
