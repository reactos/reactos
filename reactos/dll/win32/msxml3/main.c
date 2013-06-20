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

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#include <config.h>
#include <wine/port.h>

#define COBJMACROS

//#include <stdarg.h>
#ifdef HAVE_LIBXML2
//# include <libxml/parser.h>
//# include <libxml/xmlerror.h>
# ifdef SONAME_LIBXSLT
#  ifdef HAVE_LIBXSLT_PATTERN_H
#   include <libxslt/pattern.h>
#  endif
#  ifdef HAVE_LIBXSLT_TRANSFORM_H
#   include <libxslt/transform.h>
#  endif
#  include <libxslt/xsltutils.h>
#  include <libxslt/xsltInternals.h>
# endif
#endif

#include <windef.h>
#include <winbase.h>
//#include "winuser.h"
#include <ole2.h>
#include <rpcproxy.h>
//#include "msxml.h"
#include <msxml6.h>

//#include "wine/unicode.h"
#include <wine/debug.h>
#include <wine/library.h>

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

HINSTANCE MSXML_hInstance = NULL;

#ifdef HAVE_LIBXML2

void wineXmlCallbackLog(char const* caller, xmlErrorLevel lvl, char const* msg, va_list ap)
{
    enum __wine_debug_class dbcl;
    char buff[200];
    static const int max_size = sizeof(buff) / sizeof(buff[0]);
    int len;

    switch (lvl)
    {
        case XML_ERR_NONE:
            dbcl = __WINE_DBCL_TRACE;
            break;
        case XML_ERR_WARNING:
            dbcl = __WINE_DBCL_WARN;
            break;
        default:
            dbcl = __WINE_DBCL_ERR;
            break;
    }

    len = vsnprintf(buff, max_size, msg, ap);
    if (len == -1 || len >= max_size) buff[max_size-1] = 0;

    wine_dbg_log(dbcl, &__wine_dbch_msxml, caller, "%s", buff);
}

void wineXmlCallbackError(char const* caller, xmlErrorPtr err)
{
    enum __wine_debug_class dbcl;

    switch (err->level)
    {
    case XML_ERR_NONE:    dbcl = __WINE_DBCL_TRACE; break;
    case XML_ERR_WARNING: dbcl = __WINE_DBCL_WARN; break;
    default:              dbcl = __WINE_DBCL_ERR; break;
    }

    wine_dbg_log(dbcl, &__wine_dbch_msxml, caller, "error code %d", err->code);
    if (err->message)
        wine_dbg_log(dbcl, &__wine_dbch_msxml, caller, ": %s", err->message);
    else
        wine_dbg_log(dbcl, &__wine_dbch_msxml, caller, "\n");
}

/* Support for loading xml files from a Wine Windows drive */
static int wineXmlMatchCallback (char const * filename)
{
    int nRet = 0;

    TRACE("%s\n", filename);

    /*
     * We will deal with loading XML files from the file system
     *   We only care about files that linux cannot find.
     *    e.g. C:,D: etc
     */
    if(isalpha(filename[0]) && filename[1] == ':')
        nRet = 1;

    return nRet;
}

static void *wineXmlOpenCallback (char const * filename)
{
    BSTR sFilename = bstr_from_xmlChar( (const xmlChar*)filename);
    HANDLE hFile;

    TRACE("%s\n", debugstr_w(sFilename));

    hFile = CreateFileW(sFilename, GENERIC_READ,FILE_SHARE_READ, NULL,
                       OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE) hFile = 0;
    SysFreeString(sFilename);
    return hFile;
}

static int wineXmlReadCallback(void * context, char * buffer, int len)
{
    DWORD dwBytesRead;

    TRACE("%p %s %d\n", context, buffer, len);

    if ((context == NULL) || (buffer == NULL))
        return(-1);

    if(!ReadFile( context, buffer,len, &dwBytesRead, NULL))
    {
        ERR("Failed to read file\n");
        return -1;
    }

    TRACE("Read %d\n", dwBytesRead);

    return dwBytesRead;
}

static int wineXmlFileCloseCallback (void * context)
{
    return CloseHandle(context) ? 0 : -1;
}

#endif


HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}


void* libxslt_handle = NULL;
#ifdef SONAME_LIBXSLT
# define DECL_FUNCPTR(f) typeof(f) * p##f = NULL
DECL_FUNCPTR(xsltApplyStylesheet);
DECL_FUNCPTR(xsltCleanupGlobals);
DECL_FUNCPTR(xsltFreeStylesheet);
DECL_FUNCPTR(xsltParseStylesheetDoc);
# undef DECL_FUNCPTR
#endif

static void init_libxslt(void)
{
#ifdef SONAME_LIBXSLT
    void (*pxsltInit)(void); /* Missing in libxslt <= 1.1.14 */

    libxslt_handle = wine_dlopen(SONAME_LIBXSLT, RTLD_NOW, NULL, 0);
    if (!libxslt_handle)
        return;

#define LOAD_FUNCPTR(f, needed) \
    if ((p##f = wine_dlsym(libxslt_handle, #f, NULL, 0)) == NULL) \
        if (needed) { WARN("Can't find symbol %s\n", #f); goto sym_not_found; }
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
    MSXML_hInstance = hInstDLL;

    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
#ifdef HAVE_LIBXML2
        xmlInitParser();

        /* Set the default indent character to a single tab,
           for this thread and as default for new threads */
        xmlTreeIndentString = "\t";
        xmlThrDefTreeIndentString("\t");

         /* Register callbacks for loading XML files */
        if(xmlRegisterInputCallbacks(wineXmlMatchCallback, wineXmlOpenCallback,
                            wineXmlReadCallback, wineXmlFileCloseCallback) == -1)
            WARN("Failed to register callbacks\n");

        schemasInit();
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
        /* Restore default Callbacks */
        xmlCleanupInputCallbacks();
        xmlRegisterDefaultInputCallbacks();

        xmlCleanupParser();
        schemasCleanup();
#endif
        release_typelib();
        break;
    }
    return TRUE;
}

const char *debugstr_variant(const VARIANT *v)
{
    if(!v)
        return "(null)";

    switch(V_VT(v)) {
    case VT_EMPTY:
        return "{VT_EMPTY}";
    case VT_NULL:
        return "{VT_NULL}";
    case VT_I1:
        return wine_dbg_sprintf("{VT_I1: %d}", V_I1(v));
    case VT_I2:
        return wine_dbg_sprintf("{VT_I2: %d}", V_I2(v));
    case VT_I4:
        return wine_dbg_sprintf("{VT_I4: %d}", V_I4(v));
    case VT_INT:
        return wine_dbg_sprintf("{VT_INT: %d}", V_INT(v));
    case VT_R8:
        return wine_dbg_sprintf("{VT_R8: %lf}", V_R8(v));
    case VT_BSTR:
        return wine_dbg_sprintf("{VT_BSTR: %s}", debugstr_w(V_BSTR(v)));
    case VT_DISPATCH:
        return wine_dbg_sprintf("{VT_DISPATCH: %p}", V_DISPATCH(v));
    case VT_BOOL:
        return wine_dbg_sprintf("{VT_BOOL: %x}", V_BOOL(v));
    case VT_UNKNOWN:
        return wine_dbg_sprintf("{VT_UNKNOWN: %p}", V_UNKNOWN(v));
    case VT_UINT:
        return wine_dbg_sprintf("{VT_UINT: %u}", V_UINT(v));
    case VT_BSTR|VT_BYREF:
        return wine_dbg_sprintf("{VT_BSTR|VT_BYREF: ptr %p, data %s}",
            V_BSTRREF(v), debugstr_w(V_BSTRREF(v) ? *V_BSTRREF(v) : NULL));
    case VT_ERROR:
        return wine_dbg_sprintf("{VT_ERROR: 0x%08x}", V_ERROR(v));
    case VT_VARIANT|VT_BYREF:
        return wine_dbg_sprintf("{VT_VARIANT|VT_BYREF: %s}", debugstr_variant(V_VARIANTREF(v)));
    case VT_UI1|VT_ARRAY:
        return "{VT_UI1|VT_ARRAY}";
    default:
        return wine_dbg_sprintf("{vt %d}", V_VT(v));
    }
}

/***********************************************************************
 *		DllRegisterServer (MSXML3.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( MSXML_hInstance );
}

/***********************************************************************
 *		DllUnregisterServer (MSXML3.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( MSXML_hInstance );
}
