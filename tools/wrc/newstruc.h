/*
 * Create dynamic new structures of various types
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

#ifndef __WRC_NEWSTRUC_H
#define __WRC_NEWSTRUC_H

#include "wrctypes.h"

dialog_t *new_dialog(void);
name_id_t *new_name_id(void);
menu_t *new_menu(void);
menu_item_t *new_menu_item(void);
control_t *new_control(void);
icon_t *new_icon(void);
cursor_t *new_cursor(void);
versioninfo_t *new_versioninfo(void);
ver_value_t *new_ver_value(void);
ver_block_t *new_ver_block(void);
stt_entry_t *new_stt_entry(void);
accelerator_t *new_accelerator(void);
event_t *new_event(void);
raw_data_t *new_raw_data(void);
lvc_t *new_lvc(void);
res_count_t *new_res_count(void);
string_t *new_string(void);
toolbar_item_t *new_toolbar_item(void);
ani_any_t *new_ani_any(void);
resource_t *new_resource(enum res_e t, void *res, int memopt, language_t *lan);
version_t *new_version(DWORD v);
characts_t *new_characts(DWORD c);
language_t *new_language(int id, int sub);
language_t *dup_language(language_t *l);
version_t *dup_version(version_t *v);
characts_t *dup_characts(characts_t *c);
html_t *new_html(raw_data_t *rd, int *memopt);
rcdata_t *new_rcdata(raw_data_t *rd, int *memopt);
font_id_t *new_font_id(int size, string_t *face, int weight, int italic);
user_t *new_user(name_id_t *type, raw_data_t *rd, int *memopt);
font_t *new_font(raw_data_t *rd, int *memopt);
fontdir_t *new_fontdir(raw_data_t *rd, int *memopt);
icon_group_t *new_icon_group(raw_data_t *rd, int *memopt);
cursor_group_t *new_cursor_group(raw_data_t *rd, int *memopt);
ani_curico_t *new_ani_curico(enum res_e type, raw_data_t *rd, int *memopt);
bitmap_t *new_bitmap(raw_data_t *rd, int *memopt);
ver_words_t *new_ver_words(int i);
ver_words_t *add_ver_words(ver_words_t *w, int i);
messagetable_t *new_messagetable(raw_data_t *rd, int *memopt);
dlginit_t *new_dlginit(raw_data_t *rd, int *memopt);
void copy_raw_data(raw_data_t *dst, raw_data_t *src, unsigned int offs, int len);
int *new_int(int i);
stringtable_t *new_stringtable(lvc_t *lvc);
toolbar_t *new_toolbar(int button_width, int button_Height, toolbar_item_t *items, int nitems);
style_pair_t *new_style_pair(style_t *style, style_t *exstyle);
style_t *new_style(DWORD or_mask, DWORD and_mask);

#endif
