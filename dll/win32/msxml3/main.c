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
#ifdef HAVE_LIBXML2
# include <libxml/parser.h>
# include <libxml/xmlerror.h>
# ifdef SONAME_LIBXSLT
#  ifdef HAVE_LIBXSLT_PATTERN_H
#   include <libxslt/pattern.h>
#  endif
#  ifdef HAVE_LIBXSLT_TRANSFORM_H
#   include <libxslt/transform.h>
#  endif
#  include <libxslt/imports.h>
#  include <libxslt/xsltutils.h>
#  include <libxslt/variables.h>
#  include <libxslt/xsltInternals.h>
#  include <libxslt/documents.h>
#  include <libxslt/extensions.h>
#  include <libxslt/extra.h>
# endif
#endif

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "rpcproxy.h"
#include "msxml.h"
#include "msxml6.h"

#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/library.h"

#include "msxml_private.h"

HINSTANCE MSXML_hInstance = NULL;

#ifdef HAVE_LIBXML2

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

void wineXmlCallbackLog(char const* caller, xmlErrorLevel lvl, char const* msg, va_list ap)
{
    enum __wine_debug_class dbcl;
    char buff[200];
    const int max_size = ARRAY_SIZE(buff);
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

void* libxslt_handle = NULL;
#ifdef SONAME_LIBXSLT
# define DECL_FUNCPTR(f) typeof(f) * p##f = NULL
DECL_FUNCPTR(xsltApplyStylesheet);
DECL_FUNCPTR(xsltApplyStylesheetUser);
DECL_FUNCPTR(xsltCleanupGlobals);
DECL_FUNCPTR(xsltFreeStylesheet);
DECL_FUNCPTR(xsltFreeTransformContext);
DECL_FUNCPTR(xsltFunctionNodeSet);
DECL_FUNCPTR(xsltNewTransformContext);
DECL_FUNCPTR(xsltNextImport);
DECL_FUNCPTR(xsltParseStylesheetDoc);
DECL_FUNCPTR(xsltQuoteUserParams);
DECL_FUNCPTR(xsltRegisterExtModuleFunction);
DECL_FUNCPTR(xsltSaveResultTo);
DECL_FUNCPTR(xsltSetLoaderFunc);
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
    LOAD_FUNCPTR(xsltApplyStylesheetUser, 1);
    LOAD_FUNCPTR(xsltCleanupGlobals, 1);
    LOAD_FUNCPTR(xsltFreeStylesheet, 1);
    LOAD_FUNCPTR(xsltFreeTransformContext, 1);
    LOAD_FUNCPTR(xsltFunctionNodeSet, 1);
    LOAD_FUNCPTR(xsltNewTransformContext, 1);
    LOAD_FUNCPTR(xsltNextImport, 1);
    LOAD_FUNCPTR(xsltParseStylesheetDoc, 1);
    LOAD_FUNCPTR(xsltQuoteUserParams, 1);
    LOAD_FUNCPTR(xsltRegisterExtModuleFunction, 1);
    LOAD_FUNCPTR(xsltSaveResultTo, 1);
    LOAD_FUNCPTR(xsltSetLoaderFunc, 1);
#undef LOAD_FUNCPTR

    if (pxsltInit)
        pxsltInit();

    pxsltSetLoaderFunc(xslt_doc_default_loader);
    pxsltRegisterExtModuleFunction(
        (const xmlChar *)"node-set",
        (const xmlChar *)"urn:schemas-microsoft-com:xslt",
        pxsltFunctionNodeSet);

    return;

 sym_not_found:
    wine_dlclose(libxslt_handle, NULL, 0);
    libxslt_handle = NULL;
#endif
}

static int to_utf8(int cp, unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    WCHAR *tmp;
    int len;

    if (!in || !inlen) return 0;

    len = MultiByteToWideChar(cp, 0, (const char *)in, *inlen, NULL, 0);
    tmp = heap_alloc(len * sizeof(WCHAR));
    if (!tmp) return -1;
    MultiByteToWideChar(cp, 0, (const char *)in, *inlen, tmp, len);

    len = WideCharToMultiByte(CP_UTF8, 0, tmp, len, (char *)out, *outlen, NULL, NULL);
    heap_free(tmp);
    if (!len) return -1;

    *outlen = len;
    return len;
}

static int from_utf8(int cp, unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    WCHAR *tmp;
    int len;

    if (!in || !inlen) return 0;

    len = MultiByteToWideChar(CP_UTF8, 0, (const char *)in, *inlen, NULL, 0);
    tmp = heap_alloc(len * sizeof(WCHAR));
    if (!tmp) return -1;
    MultiByteToWideChar(CP_UTF8, 0, (const char *)in, *inlen, tmp, len);

    len = WideCharToMultiByte(cp, 0, tmp, len, (char *)out, *outlen, NULL, NULL);
    heap_free(tmp);
    if (!len) return -1;

    *outlen = len;
    return len;
}

static int win1250_to_utf8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return to_utf8(1250, out, outlen, in, inlen);
}

static int utf8_to_win1250(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return from_utf8(1250, out, outlen, in, inlen);
}

static int win1251_to_utf8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return to_utf8(1251, out, outlen, in, inlen);
}

static int utf8_to_win1251(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return from_utf8(1251, out, outlen, in, inlen);
}

static int win1252_to_utf8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return to_utf8(1252, out, outlen, in, inlen);
}

static int utf8_to_win1252(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return from_utf8(1252, out, outlen, in, inlen);
}

static int win1253_to_utf8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return to_utf8(1253, out, outlen, in, inlen);
}

static int utf8_to_win1253(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return from_utf8(1253, out, outlen, in, inlen);
}
static int win1254_to_utf8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return to_utf8(1254, out, outlen, in, inlen);
}

static int utf8_to_win1254(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return from_utf8(1254, out, outlen, in, inlen);
}

static int win1255_to_utf8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return to_utf8(1255, out, outlen, in, inlen);
}

static int utf8_to_win1255(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return from_utf8(1255, out, outlen, in, inlen);
}

static int win1256_to_utf8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return to_utf8(1256, out, outlen, in, inlen);
}

static int utf8_to_win1256(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return from_utf8(1256, out, outlen, in, inlen);
}

static int win1257_to_utf8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return to_utf8(1257, out, outlen, in, inlen);
}

static int utf8_to_win1257(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return from_utf8(1257, out, outlen, in, inlen);
}

static int win1258_to_utf8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return to_utf8(1258, out, outlen, in, inlen);
}

static int utf8_to_win1258(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return from_utf8(1258, out, outlen, in, inlen);
}

static void init_char_encoders(void)
{
    static const struct
    {
        const char *encoding;
        xmlCharEncodingInputFunc input;
        xmlCharEncodingOutputFunc output;
    } encoder[] =
    {
        { "windows-1250", win1250_to_utf8, utf8_to_win1250 },
        { "windows-1251", win1251_to_utf8, utf8_to_win1251 },
        { "windows-1252", win1252_to_utf8, utf8_to_win1252 },
        { "windows-1253", win1253_to_utf8, utf8_to_win1253 },
        { "windows-1254", win1254_to_utf8, utf8_to_win1254 },
        { "windows-1255", win1255_to_utf8, utf8_to_win1255 },
        { "windows-1256", win1256_to_utf8, utf8_to_win1256 },
        { "windows-1257", win1257_to_utf8, utf8_to_win1257 },
        { "windows-1258", win1258_to_utf8, utf8_to_win1258 }
    };
    int i;

    xmlInitCharEncodingHandlers();

    for (i = 0; i < ARRAY_SIZE(encoder); i++)
    {
        if (!xmlFindCharEncodingHandler(encoder[i].encoding))
        {
            TRACE("Adding %s encoding handler\n", encoder[i].encoding);
            xmlNewCharEncodingHandler(encoder[i].encoding, encoder[i].input, encoder[i].output);
        }
    }
}

#endif  /* HAVE_LIBXML2 */


HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}


BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID reserved)
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

        init_char_encoders();

        schemasInit();
        init_libxslt();
#endif
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
#ifdef HAVE_LIBXML2
#ifdef SONAME_LIBXSLT
        if (libxslt_handle)
        {
            pxsltCleanupGlobals();
            wine_dlclose(libxslt_handle, NULL, 0);
        }
#endif
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
