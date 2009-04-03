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
#include "wine/port.h"

#define COBJMACROS

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml.h"
#include "msxml2.h"

#include "wine/debug.h"
#include "wine/library.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("\n");
    return S_FALSE;
}


void* libxslt_handle = NULL;
#ifdef SONAME_LIBXSLT
# define DECL_FUNCPTR(f) typeof(f) * p##f = NULL
DECL_FUNCPTR(xsltApplyStylesheet);
DECL_FUNCPTR(xsltCleanupGlobals);
DECL_FUNCPTR(xsltFreeStylesheet);
DECL_FUNCPTR(xsltParseStylesheetDoc);
# undef MAKE_FUNCPTR
#endif

static void init_libxslt(void)
{
#ifdef SONAME_LIBXSLT
    void (*pxsltInit)(void); /* Missing in libxslt <= 1.1.14 */

    libxslt_handle = wine_dlopen(SONAME_LIBXSLT, RTLD_NOW, NULL, 0);
    if (!libxslt_handle)
        return;

#define LOAD_FUNCPTR(f, needed) if ((p##f = wine_dlsym(libxslt_handle, #f, NULL, 0)) == NULL && needed) { WARN("Can't find symbol %s\n", #f); goto sym_not_found; }
    LOAD_FUNCPTR(xsltInit, 0);
    LOAD_FUNCPTR(xsltApplyStylesheet, 1);
    LOAD_FUNCPTR(xsltCleanupGlobals, 1);
    LOAD_FUNCPTR(xsltFreeStylesheet, 1);
    LOAD_FUNCPTR(xsltParseStylesheetDoc, 1);
#undef LOAD_FUNCPTR

    if (pxsltInit)
        pxsltInit();
    return;

 sym_not_found:
    wine_dlclose(libxslt_handle, NULL, 0);
    libxslt_handle = NULL;
#endif
}


BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
#ifdef HAVE_LIBXML2
        xmlInitParser();

        /* Set the default indent character to a single tab,
           for this thread and as default for new threads */
        xmlTreeIndentString = "\t";
        xmlThrDefTreeIndentString("\t");
#endif
        init_libxslt();
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
#ifdef SONAME_LIBXSLT
        if (libxslt_handle)
        {
            pxsltCleanupGlobals();
            wine_dlclose(libxslt_handle, NULL, 0);
            libxslt_handle = NULL;
        }
#endif
#ifdef HAVE_LIBXML2
        xmlCleanupParser();
#endif
        release_typelib();
        break;
    }
    return TRUE;
}
