/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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

#pragma once

#include <stdarg.h>

#define COBJMACROS

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "hlink.h"

#include "wine/heap.h"

extern HRESULT HLink_Constructor(IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;
extern HRESULT HLinkBrowseContext_Constructor(IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;

static inline LPWSTR hlink_strdupW(LPCWSTR str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (lstrlenW(str)+1)*sizeof(WCHAR);
        ret = heap_alloc(size);
        memcpy(ret, str, size);
    }

    return ret;
}

static inline LPWSTR hlink_co_strdupW(LPCWSTR str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (lstrlenW(str)+1)*sizeof(WCHAR);
        ret = CoTaskMemAlloc(size);
        memcpy(ret, str, size);
    }

    return ret;
}
