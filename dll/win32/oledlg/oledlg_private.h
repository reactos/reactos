/*
 * oledlg internal header file
 *
 * Copyright (C) 2006 Huw Davies
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

#ifndef __OLEDLG_PRIVATE_H__
#define __OLEDLG_PRIVATE_H__

extern HINSTANCE OLEDLG_hInstance;

extern UINT cf_embed_source;
extern UINT cf_embedded_object;
extern UINT cf_link_source;
extern UINT cf_object_descriptor;
extern UINT cf_link_src_descriptor;
extern UINT cf_ownerlink;
extern UINT cf_filename;
extern UINT cf_filenamew;

extern UINT oleui_msg_help;
extern UINT oleui_msg_enddialog;

#endif /* __OLEDLG_PRIVATE_H__ */
