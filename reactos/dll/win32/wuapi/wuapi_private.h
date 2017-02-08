/*
 * Copyright 2008 Hans Leidekker for CodeWeavers
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

#ifndef _WUAPI_PRIVATE_H_
#define _WUAPI_PRIVATE_H_

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <wuapi.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(wuapi);

extern HRESULT AutomaticUpdates_create( LPVOID *ppObj ) DECLSPEC_HIDDEN;
extern HRESULT UpdateSession_create( LPVOID *ppObj ) DECLSPEC_HIDDEN;
extern HRESULT UpdateSearcher_create( LPVOID *ppObj ) DECLSPEC_HIDDEN;
extern HRESULT UpdateDownloader_create( LPVOID *ppObj ) DECLSPEC_HIDDEN;
extern HRESULT UpdateInstaller_create( LPVOID *ppObj ) DECLSPEC_HIDDEN;
extern HRESULT SystemInformation_create( LPVOID *ppObj ) DECLSPEC_HIDDEN;

#endif /* _WUAPI_PRIVATE_H_ */
