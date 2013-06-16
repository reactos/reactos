/*
 * Copyright 2005 Jacek Caban
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

#include <windef.h>
#include <winuser.h>
#include <commctrl.h>

#define IDS_HTMLDOCUMENT    7501

#define IDS_STATUS_FIRST            7550
#define IDS_STATUS_DOWNLOADINGFROM  IDS_STATUS_FIRST
#define IDS_STATUS_DONE             (IDS_STATUS_FIRST+1)
#define IDS_STATUS_LAST             IDS_STATUS_DONE

#define ID_PROMPT_DIALOG    7700
#define ID_PROMPT_PROMPT    7701
#define ID_PROMPT_EDIT      7702

#define IDS_MESSAGE_BOX_TITLE  2213

#define IDS_PRINT_HEADER_TEMPLATE  8403
#define IDS_PRINT_FOOTER_TEMPLATE  8404

#define IDR_BROWSE_CONTEXT_MENU  24641

#define IDD_HYPERLINK           8000

#define IDC_URL                 9001
#define IDC_TYPE                9002
