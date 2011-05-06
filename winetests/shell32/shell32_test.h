/*
 * Unit test suite for shell32 functions
 *
 * Copyright 2005 Francois Gougett for CodeWeavers
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


/* Helper function for creating .lnk files */
typedef struct
{
    const char* description;
    const char* workdir;
    const char* path;
    LPITEMIDLIST pidl;
    const char* arguments;
    int   showcmd;
    const char* icon;
    int   icon_id;
    WORD  hotkey;
} lnk_desc_t;

#define create_lnk(a,b,c)     create_lnk_(__LINE__, (a), (b), (c))
void create_lnk_(int,const WCHAR*,lnk_desc_t*,int);
