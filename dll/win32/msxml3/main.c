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

#define COBJMACROS

#include <stdarg.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlsave.h>
#include <libxslt/pattern.h>
#include <libxslt/transform.h>
#include <libxslt/imports.h>
#include <libxslt/xsltutils.h>
#include <libxslt/variables.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/documents.h>
#include <libxslt/extensions.h>
#include <libxslt/extra.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "rpcproxy.h"
#include "msxml.h"
#include "msxml2.h"
#include "msxml6.h"

#include "wine/debug.h"

#include "msxml_private.h"

HINSTANCE MSXML_hInstance = NULL;

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

void wineXmlCallbackError(char const* caller, const xmlError* err)
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

    TRACE("Read %ld bytes.\n", dwBytesRead);

    return dwBytesRead;
}

static int wineXmlFileCloseCallback (void * context)
{
    return CloseHandle(context) ? 0 : -1;
}

static void init_libxslt(void)
{
    xsltInit();
    xsltSetLoaderFunc(xslt_doc_default_loader);
    xsltRegisterExtModuleFunction(
        (const xmlChar *)"node-set",
        (const xmlChar *)"urn:schemas-microsoft-com:xslt",
        xsltFunctionNodeSet);
}

static int to_utf8(int cp, unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    WCHAR *tmp;
    int len = 0;

    if (!in || !inlen || !*inlen) goto done;

    len = MultiByteToWideChar(cp, 0, (const char *)in, *inlen, NULL, 0);
    tmp = malloc(len * sizeof(WCHAR));
    if (!tmp) return -1;
    MultiByteToWideChar(cp, 0, (const char *)in, *inlen, tmp, len);

    len = WideCharToMultiByte(CP_UTF8, 0, tmp, len, (char *)out, *outlen, NULL, NULL);
    free(tmp);
    if (!len) return -1;
done:
    *outlen = len;
    return len;
}

static int from_utf8(int cp, unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    WCHAR *tmp;
    int len = 0;

    if (!in || !inlen || !*inlen) goto done;

    len = MultiByteToWideChar(CP_UTF8, 0, (const char *)in, *inlen, NULL, 0);
    tmp = malloc(len * sizeof(WCHAR));
    if (!tmp) return -1;
    MultiByteToWideChar(CP_UTF8, 0, (const char *)in, *inlen, tmp, len);

    len = WideCharToMultiByte(cp, 0, tmp, len, (char *)out, *outlen, NULL, NULL);
    free(tmp);
    if (!len) return -1;
done:
    *outlen = len;
    return len;
}

static int gbk_to_utf8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return to_utf8(936, out, outlen, in, inlen);
}

static int utf8_to_gbk(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return from_utf8(936, out, outlen, in, inlen);
}

static int iso8859_1_to_utf8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return to_utf8(28591, out, outlen, in, inlen);
}

static int utf8_to_iso8859_1(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
    return from_utf8(28591, out, outlen, in, inlen);
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
        { "gbk",          gbk_to_utf8,       utf8_to_gbk       },
        { "iso8859-1",    iso8859_1_to_utf8, utf8_to_iso8859_1 },
        { "windows-1250", win1250_to_utf8,   utf8_to_win1250   },
        { "windows-1251", win1251_to_utf8,   utf8_to_win1251   },
        { "windows-1252", win1252_to_utf8,   utf8_to_win1252   },
        { "windows-1253", win1253_to_utf8,   utf8_to_win1253   },
        { "windows-1254", win1254_to_utf8,   utf8_to_win1254   },
        { "windows-1255", win1255_to_utf8,   utf8_to_win1255   },
        { "windows-1256", win1256_to_utf8,   utf8_to_win1256   },
        { "windows-1257", win1257_to_utf8,   utf8_to_win1257   },
        { "windows-1258", win1258_to_utf8,   utf8_to_win1258   }
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(encoder); i++)
    {
        if (!xmlFindCharEncodingHandler(encoder[i].encoding))
        {
            TRACE("Adding %s encoding handler\n", encoder[i].encoding);
            xmlNewCharEncodingHandler(encoder[i].encoding, encoder[i].input, encoder[i].output);
        }
    }
}

const CLSID* DOMDocument_version(MSXML_VERSION v)
{
    switch (v)
    {
    default:
    case MSXML_DEFAULT: return &CLSID_DOMDocument;
    case MSXML3: return &CLSID_DOMDocument30;
    case MSXML4: return &CLSID_DOMDocument40;
    case MSXML6: return &CLSID_DOMDocument60;
    }
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID reserved)
{
    MSXML_hInstance = hInstDLL;

    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
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
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        xsltCleanupGlobals();
        /* Restore default Callbacks */
        xmlCleanupInputCallbacks();
        xmlRegisterDefaultInputCallbacks();

        xmlCleanupParser();
        schemasCleanup();
        release_typelib();
        break;
    }
    return TRUE;
}
