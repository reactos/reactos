/*
 *    MSXML Class Factory
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2005 Mike McCormack
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

#include "config.h"

#define COBJMACROS

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml.h"
#include "msxml2.h"

#include "wine/debug.h"

#include "msxml_private.h"

#ifdef HAVE_LIBXSLT
#include <libxslt/xslt.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("\n");
    return S_FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
#ifdef HAVE_LIBXML2
        xmlInitParser();

        /* Set the default indent character to a single tab. */
        xmlThrDefTreeIndentString("\t");
#endif
#ifdef HAVE_XSLTINIT
        xsltInit();
#endif

#ifdef HAVE_LIBXML2
        /* Set the current ident to the default */
        xmlTreeIndentString = "\t";
#endif
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
#ifdef HAVE_LIBXSLT
        xsltCleanupGlobals();
#endif
#ifdef HAVE_LIBXML2
        xmlCleanupParser();
#endif
        release_typelib();
        break;
    }
    return TRUE;
}
