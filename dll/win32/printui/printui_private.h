/*
 * Implementation of the Printer User Interface Dialogs: private Header
 *
 * Copyright 2007 Detlef Riekenberg
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

#ifndef __WINE_PRINTUI_PRIVATE__
#define __WINE_PRINTUI_PRIVATE__

/* Index for Options with an argument */
/* Must be in order with optionsW     */
typedef enum _OPT_INDEX {
    OPT_A = 0,
    OPT_B,
    OPT_C,
    OPT_F,
    OPT_H,
    OPT_J,
    OPT_L,
    OPT_M,
    OPT_N,
    OPT_R,
    OPT_T,
    OPT_V,
    OPT_MAX
} OPT_INDEX;

/* Index for Flags without an argument */
/* Must be in order with flagsW        */
typedef enum _FLAG_INDEX {
    FLAG_Q = 0,
    FLAG_W,
    FLAG_Y,
    FLAG_Z,
    FLAG_ZZ,
    FLAG_MAX
} FLAG_INDEX;


typedef struct tag_context {
    HWND    hWnd;
    DWORD   nCmdShow;
    LPWSTR  * argv;
    LPWSTR  pNextCharW;
    int     argc;
    int     next_arg;
    WCHAR   command;
    WCHAR   subcommand;
    LPWSTR  options[OPT_MAX];
    BOOL    flags[FLAG_MAX];
} context_t;
#endif
