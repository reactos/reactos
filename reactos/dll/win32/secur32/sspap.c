/*
 * Copyright (C) 2011 Samuel Serapión
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

#include <precomp.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(secur32);

BOOL LoadSSPAPProvider(PWSTR moduleName)
{
    HMODULE lib = LoadLibraryW(moduleName);

    if (lib)
    {
        SpUserModeInitializeFn pSpUserModeInitialize =
        (SpUserModeInitializeFn)GetProcAddress(lib,SECPKG_USERMODEINIT_NAME);
        ERR("loaded %s, SpUserModeInitializeFn is %p\n", debugstr_w(moduleName),
            pSpUserModeInitialize);

            return TRUE;
    }
    return FALSE;
}

