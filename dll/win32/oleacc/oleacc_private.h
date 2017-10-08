/*
 * Copyright 2014 Piotr Caban for CodeWeavers
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

#ifndef _OLEACC_PRIVATE_H_
#define _OLEACC_PRIVATE_H_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <ole2.h>
#include <oleacc_classes.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(oleacc);

HRESULT create_client_object(HWND, const IID*, void**) DECLSPEC_HIDDEN;
HRESULT create_window_object(HWND, const IID*, void**) DECLSPEC_HIDDEN;
HRESULT get_accpropservices_factory(REFIID, void**) DECLSPEC_HIDDEN;

int convert_child_id(VARIANT *v) DECLSPEC_HIDDEN;

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

#endif /* _OLEACC_PRIVATE_H_ */
