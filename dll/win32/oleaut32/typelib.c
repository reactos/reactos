/*
 *	TYPELIB
 *
 *	Copyright 1997	Marcus Meissner
 *		      1999  Rein Klazes
 *		      2000  Francois Jacques
 *		      2001  Huw D M Davies for CodeWeavers
 *		      2004  Alastair Bridgewater
 *		      2005  Robert Shearman, for CodeWeavers
 *		      2013  Andrew Eikum for CodeWeavers
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
 *
 * --------------------------------------------------------------------------------------
 * Known problems (2000, Francois Jacques)
 *
 * - Tested using OLEVIEW (Platform SDK tool) only.
 *
 * - dual interface dispinterfaces. vtable-interface ITypeInfo instances are
 *   creating by doing a straight copy of the dispinterface instance and just changing
 *   its typekind. Pointed structures aren't copied - only the address of the pointers.
 *
 * - locale stuff is partially implemented but hasn't been tested.
 *
 * - typelib file is still read in its entirety, but it is released now.
 *
 * --------------------------------------------------------------------------------------
 *  Known problems left from previous implementation (1999, Rein Klazes) :
 *
 * -. Data structures are straightforward, but slow for look-ups.
 * -. (related) nothing is hashed
 * -. Most error return values are just guessed not checked with windows
 *      behaviour.
 * -. lousy fatal error handling
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#define COBJMACROS
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winuser.h"
#include "winternl.h"
#include "lzexpand.h"

#include "objbase.h"
#include "typelib.h"
#include "wine/debug.h"
#include "variant.h"
#include "wine/asm.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);
WINE_DECLARE_DEBUG_CHANNEL(typelib);

static const BOOL is_win64 = sizeof(void *) > sizeof(int);

typedef struct
{
    WORD     offset;
    WORD     length;
    WORD     flags;
    WORD     id;
    WORD     handle;
    WORD     usage;
} NE_NAMEINFO;

typedef struct
{
    WORD        type_id;   /* Type identifier */
    WORD        count;     /* Number of resources of this type */
    DWORD       resloader; /* SetResourceHandler() */
    /*
     * Name info array.
     */
} NE_TYPEINFO;

static HRESULT typedescvt_to_variantvt(ITypeInfo *tinfo, const TYPEDESC *tdesc, VARTYPE *vt);
static HRESULT TLB_AllocAndInitVarDesc(const VARDESC *src, VARDESC **dest_ptr);
static void TLB_FreeVarDesc(VARDESC*);

/****************************************************************************
 *              FromLExxx
 *
 * Takes p_iVal (which is in little endian) and returns it
 *   in the host machine's byte order.
 */
#ifdef WORDS_BIGENDIAN
static WORD FromLEWord(WORD p_iVal)
{
  return (((p_iVal & 0x00FF) << 8) |
	  ((p_iVal & 0xFF00) >> 8));
}


static DWORD FromLEDWord(DWORD p_iVal)
{
  return (((p_iVal & 0x000000FF) << 24) |
	  ((p_iVal & 0x0000FF00) <<  8) |
	  ((p_iVal & 0x00FF0000) >>  8) |
	  ((p_iVal & 0xFF000000) >> 24));
}
#else
#define FromLEWord(X)  (X)
#define FromLEDWord(X) (X)
#endif

#define DISPATCH_HREF_OFFSET 0x01000000
#define DISPATCH_HREF_MASK   0xff000000

/****************************************************************************
 *              FromLExxx
 *
 * Fix byte order in any structure if necessary
 */
#ifdef WORDS_BIGENDIAN
static void FromLEWords(void *p_Val, int p_iSize)
{
  WORD *Val = p_Val;

  p_iSize /= sizeof(WORD);

  while (p_iSize) {
    *Val = FromLEWord(*Val);
    Val++;
    p_iSize--;
  }
}


static void FromLEDWords(void *p_Val, int p_iSize)
{
  DWORD *Val = p_Val;

  p_iSize /= sizeof(DWORD);

  while (p_iSize) {
    *Val = FromLEDWord(*Val);
    Val++;
    p_iSize--;
  }
}
#else
#define FromLEWords(X,Y) /*nothing*/
#define FromLEDWords(X,Y) /*nothing*/
#endif

/*
 * Find a typelib key which matches a requested maj.min version.
 */
static BOOL find_typelib_key( REFGUID guid, WORD *wMaj, WORD *wMin )
{
    WCHAR buffer[60];
    char key_name[16];
    DWORD len, i;
    INT best_maj = -1, best_min = -1;
    HKEY hkey;

    lstrcpyW( buffer, L"Typelib\\" );
    StringFromGUID2( guid, buffer + lstrlenW(buffer), 40 );

    if (RegOpenKeyExW( HKEY_CLASSES_ROOT, buffer, 0, KEY_READ, &hkey ) != ERROR_SUCCESS)
        return FALSE;

    len = sizeof(key_name);
    i = 0;
    while (RegEnumKeyExA(hkey, i++, key_name, &len, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        INT v_maj, v_min;

        if (sscanf(key_name, "%x.%x", &v_maj, &v_min) == 2)
        {
            TRACE("found %s: %x.%x\n", debugstr_w(buffer), v_maj, v_min);

            if (*wMaj == 0xffff && *wMin == 0xffff)
            {
                if (v_maj > best_maj) best_maj = v_maj;
                if (v_min > best_min) best_min = v_min;
            }
            else if (*wMaj == v_maj)
            {
                best_maj = v_maj;

                if (*wMin == v_min)
                {
                    best_min = v_min;
                    break; /* exact match */
                }
                if (*wMin != 0xffff && v_min >= *wMin && v_min > best_min) best_min = v_min;
            }
        }
        len = sizeof(key_name);
    }
    RegCloseKey( hkey );

    TRACE("found best_maj %d, best_min %d\n", best_maj, best_min);

    if (*wMaj == 0xffff && *wMin == 0xffff)
    {
        if (best_maj >= 0 && best_min >= 0)
        {
            *wMaj = best_maj;
            *wMin = best_min;
            return TRUE;
        }
    }

    if (*wMaj == best_maj && best_min >= 0)
    {
        *wMin = best_min;
        return TRUE;
    }
    return FALSE;
}

/* get the path of a typelib key, in the form "Typelib\\<guid>\\<maj>.<min>" */
/* buffer must be at least 60 characters long */
static WCHAR *get_typelib_key( REFGUID guid, WORD wMaj, WORD wMin, WCHAR *buffer )
{
    lstrcpyW( buffer, L"Typelib\\" );
    StringFromGUID2( guid, buffer + lstrlenW(buffer), 40 );
    swprintf( buffer + lstrlenW(buffer), 20, L"\\%x.%x", wMaj, wMin );
    return buffer;
}

/* get the path of an interface key, in the form "Interface\\<guid>" */
/* buffer must be at least 50 characters long */
static WCHAR *get_interface_key( REFGUID guid, WCHAR *buffer )
{
    lstrcpyW( buffer, L"Interface\\" );
    StringFromGUID2( guid, buffer + lstrlenW(buffer), 40 );
    return buffer;
}

/* get the lcid subkey for a typelib, in the form "<lcid>\\<syskind>" */
/* buffer must be at least 16 characters long */
static WCHAR *get_lcid_subkey( LCID lcid, SYSKIND syskind, WCHAR *buffer )
{
    swprintf( buffer, 16, L"%lx\\", lcid );
    switch(syskind)
    {
    case SYS_WIN16: lstrcatW( buffer, L"win16" ); break;
    case SYS_WIN32: lstrcatW( buffer, L"win32" ); break;
    case SYS_WIN64: lstrcatW( buffer, L"win64" ); break;
    default:
        TRACE("Typelib is for unsupported syskind %i\n", syskind);
        return NULL;
    }
    return buffer;
}

static HRESULT TLB_ReadTypeLib(LPCWSTR pszFileName, LPWSTR pszPath, UINT cchPath, ITypeLib2 **ppTypeLib);

struct tlibredirect_data
{
    ULONG  size;
    DWORD  res;
    ULONG  name_len;
    ULONG  name_offset;
    LANGID langid;
    WORD   flags;
    ULONG  help_len;
    ULONG  help_offset;
    WORD   major_version;
    WORD   minor_version;
};

/* Get the path to a registered type library. Helper for QueryPathOfRegTypeLib. */
static HRESULT query_typelib_path( REFGUID guid, WORD wMaj, WORD wMin,
                                   SYSKIND syskind, LCID lcid, BSTR *path, BOOL redir )
{
    HRESULT hr = TYPE_E_LIBNOTREGISTERED;
    LCID myLCID = lcid;
    HKEY hkey;
    WCHAR buffer[60];
    WCHAR Path[MAX_PATH];
    LONG res;

    TRACE_(typelib)("%s, %x.%x, %#lx, %p\n", debugstr_guid(guid), wMaj, wMin, lcid, path);

    if (redir)
    {
        ACTCTX_SECTION_KEYED_DATA data;

        data.cbSize = sizeof(data);
        if (FindActCtxSectionGuid( 0, NULL, ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION, guid, &data ))
        {
            struct tlibredirect_data *tlib = (struct tlibredirect_data*)data.lpData;
            WCHAR *nameW;
            DWORD len;

            if ((wMaj != 0xffff || wMin != 0xffff) && (tlib->major_version != wMaj || tlib->minor_version < wMin))
                return TYPE_E_LIBNOTREGISTERED;

            nameW = (WCHAR*)((BYTE*)data.lpSectionBase + tlib->name_offset);
            len = SearchPathW( NULL, nameW, NULL, ARRAY_SIZE( Path ), Path, NULL );
            if (!len) return TYPE_E_LIBNOTREGISTERED;

            TRACE_(typelib)("got path from context %s\n", debugstr_w(Path));
            *path = SysAllocString( Path );
            return S_OK;
        }
    }

    if (!find_typelib_key( guid, &wMaj, &wMin )) return TYPE_E_LIBNOTREGISTERED;
    get_typelib_key( guid, wMaj, wMin, buffer );

    res = RegOpenKeyExW( HKEY_CLASSES_ROOT, buffer, 0, KEY_READ, &hkey );
    if (res == ERROR_FILE_NOT_FOUND)
    {
        TRACE_(typelib)("%s not found\n", debugstr_w(buffer));
        return TYPE_E_LIBNOTREGISTERED;
    }
    else if (res != ERROR_SUCCESS)
    {
        TRACE_(typelib)("failed to open %s for read access\n", debugstr_w(buffer));
        return TYPE_E_REGISTRYACCESS;
    }

    while (hr != S_OK)
    {
        LONG dwPathLen = sizeof(Path);

        get_lcid_subkey( myLCID, syskind, buffer );

        if (RegQueryValueW(hkey, buffer, Path, &dwPathLen))
        {
            if (!lcid)
                break;
            else if (myLCID == lcid)
            {
                /* try with sub-langid */
                myLCID = SUBLANGID(lcid);
            }
            else if ((myLCID == SUBLANGID(lcid)) && myLCID)
            {
                /* try with system langid */
                myLCID = 0;
            }
            else
            {
                break;
            }
        }
        else
        {
            *path = SysAllocString( Path );
            hr = S_OK;
        }
    }
    RegCloseKey( hkey );
    TRACE_(typelib)("-- %#lx\n", hr);
    return hr;
}

/****************************************************************************
 *		QueryPathOfRegTypeLib	[OLEAUT32.164]
 *
 * Gets the path to a registered type library.
 *
 * PARAMS
 *  guid [I] referenced guid
 *  wMaj [I] major version
 *  wMin [I] minor version
 *  lcid [I] locale id
 *  path [O] path of typelib
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: If the type library is not registered then TYPE_E_LIBNOTREGISTERED
 *  or TYPE_E_REGISTRYACCESS if the type library registration key couldn't be
 *  opened.
 */
HRESULT WINAPI QueryPathOfRegTypeLib( REFGUID guid, WORD wMaj, WORD wMin, LCID lcid, LPBSTR path )
{
    BOOL redir = TRUE;
    HRESULT hres = query_typelib_path( guid, wMaj, wMin, is_win64 ? SYS_WIN64 : SYS_WIN32, lcid, path, TRUE );
    if(SUCCEEDED(hres))
        return hres;
    redir = FALSE;
    return query_typelib_path( guid, wMaj, wMin, is_win64 ? SYS_WIN32 : SYS_WIN64, lcid, path, redir );
}

/******************************************************************************
 * CreateTypeLib [OLEAUT32.160]  creates a typelib
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI CreateTypeLib(
	SYSKIND syskind, LPCOLESTR szFile, ICreateTypeLib** ppctlib
) {
    FIXME("(%d,%s,%p), stub!\n",syskind,debugstr_w(szFile),ppctlib);
    return E_FAIL;
}

/******************************************************************************
 *		LoadTypeLib	[OLEAUT32.161]
 *
 * Loads a type library
 *
 * PARAMS
 *  szFile [I] Name of file to load from.
 *  pptLib [O] Pointer that receives ITypeLib object on success.
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 *
 * SEE
 *  LoadTypeLibEx, LoadRegTypeLib, CreateTypeLib.
 */
HRESULT WINAPI LoadTypeLib(const OLECHAR *szFile, ITypeLib * *pptLib)
{
    TRACE("(%s,%p)\n",debugstr_w(szFile), pptLib);
    return LoadTypeLibEx(szFile, REGKIND_DEFAULT, pptLib);
}

/******************************************************************************
 *		LoadTypeLibEx	[OLEAUT32.183]
 *
 * Loads and optionally registers a type library
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI LoadTypeLibEx(
    LPCOLESTR szFile,  /* [in] Name of file to load from */
    REGKIND  regkind,  /* [in] Specify kind of registration */
    ITypeLib **pptLib) /* [out] Pointer to pointer to loaded type library */
{
    WCHAR szPath[MAX_PATH+1];
    HRESULT res;

    TRACE("(%s,%d,%p)\n",debugstr_w(szFile), regkind, pptLib);

    if (!szFile || !pptLib)
        return E_INVALIDARG;

    *pptLib = NULL;

    res = TLB_ReadTypeLib(szFile, szPath, MAX_PATH + 1, (ITypeLib2**)pptLib);

    if (SUCCEEDED(res))
        switch(regkind)
        {
            case REGKIND_DEFAULT:
                /* don't register typelibs supplied with full path. Experimentation confirms the following */
                if (((szFile[0] == '\\') && (szFile[1] == '\\')) ||
                    (szFile[0] && (szFile[1] == ':'))) break;
                /* else fall-through */

            case REGKIND_REGISTER:
                if (FAILED(res = RegisterTypeLib(*pptLib, szPath, NULL)))
                {
                    ITypeLib_Release(*pptLib);
                    *pptLib = 0;
                }
                break;
            case REGKIND_NONE:
                break;
        }

    TRACE(" returns %#lx\n",res);
    return res;
}

/******************************************************************************
 *		LoadRegTypeLib	[OLEAUT32.162]
 *
 * Loads a registered type library.
 *
 * PARAMS
 *  rguid     [I] GUID of the registered type library.
 *  wVerMajor [I] major version.
 *  wVerMinor [I] minor version.
 *  lcid      [I] locale ID.
 *  ppTLib    [O] pointer that receives an ITypeLib object on success.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: Any HRESULT code returned from QueryPathOfRegTypeLib or
 *  LoadTypeLib.
 */
HRESULT WINAPI LoadRegTypeLib(
	REFGUID rguid,
	WORD wVerMajor,
	WORD wVerMinor,
	LCID lcid,
	ITypeLib **ppTLib)
{
    BSTR bstr=NULL;
    HRESULT res;

    *ppTLib = NULL;

    res = QueryPathOfRegTypeLib( rguid, wVerMajor, wVerMinor, lcid, &bstr);

    if(SUCCEEDED(res))
    {
        res= LoadTypeLib(bstr, ppTLib);
        SysFreeString(bstr);

        if ((wVerMajor!=0xffff || wVerMinor!=0xffff) && *ppTLib)
        {
            TLIBATTR *attr;

            res = ITypeLib_GetLibAttr(*ppTLib, &attr);
            if (res == S_OK)
            {
                BOOL mismatch = attr->wMajorVerNum != wVerMajor || attr->wMinorVerNum < wVerMinor;
                ITypeLib_ReleaseTLibAttr(*ppTLib, attr);

                if (mismatch)
                {
                    ITypeLib_Release(*ppTLib);
                    *ppTLib = NULL;
                    res = TYPE_E_LIBNOTREGISTERED;
                }
            }
        }
    }

    TRACE("(IID: %s) load %s (%p)\n",debugstr_guid(rguid), SUCCEEDED(res)? "SUCCESS":"FAILED", *ppTLib);

    return res;
}

static void TLB_register_interface(TLIBATTR *libattr, LPOLESTR name, TYPEATTR *tattr, DWORD flag)
{
    WCHAR keyName[60];
    HKEY key, subKey;

    get_interface_key( &tattr->guid, keyName );
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, keyName, 0, NULL, 0,
                        KEY_WRITE | flag, NULL, &key, NULL) == ERROR_SUCCESS)
    {
        const WCHAR *proxy_clsid;

        if (tattr->typekind == TKIND_INTERFACE || (tattr->wTypeFlags & TYPEFLAG_FDUAL))
            proxy_clsid = L"{00020424-0000-0000-C000-000000000046}";
        else
            proxy_clsid = L"{00020420-0000-0000-C000-000000000046}";

        if (name)
            RegSetValueExW(key, NULL, 0, REG_SZ,
                           (BYTE *)name, (lstrlenW(name)+1) * sizeof(OLECHAR));

        if (!RegCreateKeyExW(key, L"ProxyStubClsid", 0, NULL, 0, KEY_WRITE | flag, NULL, &subKey, NULL))
        {
            RegSetValueExW(subKey, NULL, 0, REG_SZ, (const BYTE *)proxy_clsid, (lstrlenW(proxy_clsid) + 1) * sizeof(WCHAR));
            RegCloseKey(subKey);
        }

        if (!RegCreateKeyExW(key, L"ProxyStubClsid32", 0, NULL, 0, KEY_WRITE | flag, NULL, &subKey, NULL))
        {
            RegSetValueExW(subKey, NULL, 0, REG_SZ, (const BYTE *)proxy_clsid, (lstrlenW(proxy_clsid) + 1) * sizeof(WCHAR));
            RegCloseKey(subKey);
        }

        if (RegCreateKeyExW(key, L"TypeLib", 0, NULL, 0,
            KEY_WRITE | flag, NULL, &subKey, NULL) == ERROR_SUCCESS)
        {
            WCHAR buffer[40];

            StringFromGUID2(&libattr->guid, buffer, 40);
            RegSetValueExW(subKey, NULL, 0, REG_SZ,
                           (BYTE *)buffer, (lstrlenW(buffer)+1) * sizeof(WCHAR));
            swprintf(buffer, ARRAY_SIZE(buffer), L"%x.%x", libattr->wMajorVerNum, libattr->wMinorVerNum);
            RegSetValueExW(subKey, L"Version", 0, REG_SZ, (BYTE *)buffer, (lstrlenW(buffer)+1) * sizeof(WCHAR));
            RegCloseKey(subKey);
        }

        RegCloseKey(key);
    }
}

/******************************************************************************
 *		RegisterTypeLib	[OLEAUT32.163]
 * Adds information about a type library to the System Registry
 * NOTES
 *    Docs: ITypeLib FAR * ptlib
 *    Docs: OLECHAR FAR* szFullPath
 *    Docs: OLECHAR FAR* szHelpDir
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI RegisterTypeLib(ITypeLib *ptlib, const WCHAR *szFullPath, const WCHAR *szHelpDir)
{
    HRESULT res;
    TLIBATTR *attr;
    WCHAR keyName[60];
    WCHAR tmp[16];
    HKEY key, subKey;
    UINT types, tidx;
    TYPEKIND kind;
    DWORD disposition;

    if (ptlib == NULL || szFullPath == NULL)
        return E_INVALIDARG;

    if (FAILED(ITypeLib_GetLibAttr(ptlib, &attr)))
        return E_FAIL;

    get_typelib_key( &attr->guid, attr->wMajorVerNum, attr->wMinorVerNum, keyName );

    res = S_OK;
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, keyName, 0, NULL, 0,
        KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
    {
        LPOLESTR doc;
        LPOLESTR libName;

        /* Set the human-readable name of the typelib to
           the typelib's doc, if it exists, else to the typelib's name. */
        if (FAILED(ITypeLib_GetDocumentation(ptlib, -1, &libName, &doc, NULL, NULL)))
            res = E_FAIL;
        else if (doc || libName)
        {
            WCHAR *name = doc ? doc : libName;

            if (RegSetValueExW(key, NULL, 0, REG_SZ,
                (BYTE *)name, (lstrlenW(name)+1) * sizeof(OLECHAR)) != ERROR_SUCCESS)
                res = E_FAIL;

            SysFreeString(doc);
            SysFreeString(libName);
        }

        /* Make up the name of the typelib path subkey */
        if (!get_lcid_subkey( attr->lcid, attr->syskind, tmp )) res = E_FAIL;

        /* Create the typelib path subkey */
        if (res == S_OK && RegCreateKeyExW(key, tmp, 0, NULL, 0,
            KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS)
        {
            if (RegSetValueExW(subKey, NULL, 0, REG_SZ,
                (BYTE *)szFullPath, (lstrlenW(szFullPath)+1) * sizeof(OLECHAR)) != ERROR_SUCCESS)
                res = E_FAIL;

            RegCloseKey(subKey);
        }
        else
            res = E_FAIL;

        /* Create the flags subkey */
        if (res == S_OK && RegCreateKeyExW(key, L"FLAGS", 0, NULL, 0,
            KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS)
        {
            WCHAR buf[20];

            /* FIXME: is %u correct? */
            swprintf(buf, ARRAY_SIZE(buf), L"%u", attr->wLibFlags);
            if (RegSetValueExW(subKey, NULL, 0, REG_SZ,
                               (BYTE *)buf, (lstrlenW(buf) + 1)*sizeof(WCHAR) ) != ERROR_SUCCESS)
                res = E_FAIL;

            RegCloseKey(subKey);
        }
        else
            res = E_FAIL;

        /* create the helpdir subkey */
        if (res == S_OK && RegCreateKeyExW(key, L"HELPDIR", 0, NULL, 0,
            KEY_WRITE, NULL, &subKey, &disposition) == ERROR_SUCCESS)
        {
            BSTR freeHelpDir = NULL;
            WCHAR *file_name;

            /* if we created a new key, and helpDir was null, set the helpdir
               to the directory which contains the typelib. However,
               if we just opened an existing key, we leave the helpdir alone */
            if ((disposition == REG_CREATED_NEW_KEY) && (szHelpDir == NULL)) {
                szHelpDir = freeHelpDir = SysAllocString(szFullPath);
                file_name = wcsrchr(szHelpDir, '\\');
                if (file_name && file_name[1]) {
                    /* possible remove a numeric \index (resource-id) */
                    WCHAR *end_ptr = file_name + 1;
                    while ('0' <= *end_ptr && *end_ptr <= '9') end_ptr++;
                    if (!*end_ptr)
                    {
                        *file_name = 0;
                        file_name = wcsrchr(szHelpDir, '\\');
                    }
                }
                if (file_name)
                    *file_name = 0;
            }

            /* if we have an szHelpDir, set it! */
            if (szHelpDir != NULL) {
                if (RegSetValueExW(subKey, NULL, 0, REG_SZ,
                    (BYTE *)szHelpDir, (lstrlenW(szHelpDir)+1) * sizeof(OLECHAR)) != ERROR_SUCCESS) {
                    res = E_FAIL;
                }
            }

            SysFreeString(freeHelpDir);
            RegCloseKey(subKey);
        } else {
            res = E_FAIL;
        }

        RegCloseKey(key);
    }
    else
        res = E_FAIL;

    /* register OLE Automation-compatible interfaces for this typelib */
    types = ITypeLib_GetTypeInfoCount(ptlib);
    for (tidx=0; tidx<types; tidx++) {
	if (SUCCEEDED(ITypeLib_GetTypeInfoType(ptlib, tidx, &kind))) {
	    LPOLESTR name = NULL;
	    ITypeInfo *tinfo = NULL;

	    ITypeLib_GetDocumentation(ptlib, tidx, &name, NULL, NULL, NULL);

	    switch (kind) {
	    case TKIND_INTERFACE:
		TRACE_(typelib)("%d: interface %s\n", tidx, debugstr_w(name));
		ITypeLib_GetTypeInfo(ptlib, tidx, &tinfo);
		break;

	    case TKIND_DISPATCH:
		TRACE_(typelib)("%d: dispinterface %s\n", tidx, debugstr_w(name));
                ITypeLib_GetTypeInfo(ptlib, tidx, &tinfo);
		break;

	    default:
		TRACE_(typelib)("%d: %s\n", tidx, debugstr_w(name));
		break;
	    }

	    if (tinfo) {
		TYPEATTR *tattr = NULL;
		ITypeInfo_GetTypeAttr(tinfo, &tattr);

		if (tattr) {
		    TRACE_(typelib)("guid=%s, flags=%04x (",
				    debugstr_guid(&tattr->guid),
				    tattr->wTypeFlags);

		    if (TRACE_ON(typelib)) {
#define XX(x) if (TYPEFLAG_##x & tattr->wTypeFlags) MESSAGE(#x"|");
			XX(FAPPOBJECT);
			XX(FCANCREATE);
			XX(FLICENSED);
			XX(FPREDECLID);
			XX(FHIDDEN);
			XX(FCONTROL);
			XX(FDUAL);
			XX(FNONEXTENSIBLE);
			XX(FOLEAUTOMATION);
			XX(FRESTRICTED);
			XX(FAGGREGATABLE);
			XX(FREPLACEABLE);
			XX(FDISPATCHABLE);
			XX(FREVERSEBIND);
			XX(FPROXY);
#undef XX
			MESSAGE("\n");
		    }

                    /* Register all dispinterfaces (which includes dual interfaces) and
                       oleautomation interfaces */
		    if ((kind == TKIND_INTERFACE && (tattr->wTypeFlags & TYPEFLAG_FOLEAUTOMATION)) ||
                        kind == TKIND_DISPATCH)
		    {
                        BOOL is_wow64;
                        DWORD opposite = (sizeof(void*) == 8 ? KEY_WOW64_32KEY : KEY_WOW64_64KEY);

                        /* register interface<->typelib coupling */
                        TLB_register_interface(attr, name, tattr, 0);

                        /* register TLBs into the opposite registry view, too */
                        if(opposite == KEY_WOW64_32KEY ||
                                 (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64))
                            TLB_register_interface(attr, name, tattr, opposite);
		    }

		    ITypeInfo_ReleaseTypeAttr(tinfo, tattr);
		}

		ITypeInfo_Release(tinfo);
	    }

	    SysFreeString(name);
	}
    }

    ITypeLib_ReleaseTLibAttr(ptlib, attr);

    return res;
}

static void TLB_unregister_interface(GUID *guid, REGSAM flag)
{
    WCHAR subKeyName[50];
    HKEY subKey;

    /* the path to the type */
    get_interface_key( guid, subKeyName );

    /* Delete its bits */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, subKeyName, 0, KEY_WRITE | flag, &subKey) != ERROR_SUCCESS)
        return;

    RegDeleteKeyW(subKey, L"ProxyStubClsid");
    RegDeleteKeyW(subKey, L"ProxyStubClsid32");
    RegDeleteKeyW(subKey, L"TypeLib");
    RegCloseKey(subKey);
    RegDeleteKeyExW(HKEY_CLASSES_ROOT, subKeyName, flag, 0);
}

/******************************************************************************
 *	UnRegisterTypeLib	[OLEAUT32.186]
 * Removes information about a type library from the System Registry
 * NOTES
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI UnRegisterTypeLib(
    REFGUID libid,	/* [in] Guid of the library */
	WORD wVerMajor,	/* [in] major version */
	WORD wVerMinor,	/* [in] minor version */
	LCID lcid,	/* [in] locale id */
	SYSKIND syskind)
{
    BSTR tlibPath = NULL;
    DWORD tmpLength;
    WCHAR keyName[60];
    WCHAR subKeyName[50];
    int result = S_OK;
    DWORD i = 0;
    BOOL deleteOtherStuff;
    HKEY key = NULL;
    TYPEATTR* typeAttr = NULL;
    TYPEKIND kind;
    ITypeInfo* typeInfo = NULL;
    ITypeLib* typeLib = NULL;
    int numTypes;

    TRACE("(IID: %s)\n",debugstr_guid(libid));

    /* Create the path to the key */
    get_typelib_key( libid, wVerMajor, wVerMinor, keyName );

    if (syskind != SYS_WIN16 && syskind != SYS_WIN32 && syskind != SYS_WIN64)
    {
        TRACE("Unsupported syskind %i\n", syskind);
        result = E_INVALIDARG;
        goto end;
    }

    /* get the path to the typelib on disk */
    if (query_typelib_path(libid, wVerMajor, wVerMinor, syskind, lcid, &tlibPath, FALSE) != S_OK) {
        result = E_INVALIDARG;
        goto end;
    }

    /* Try and open the key to the type library. */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, keyName, 0, KEY_READ | KEY_WRITE, &key) != ERROR_SUCCESS) {
        result = E_INVALIDARG;
        goto end;
    }

    /* Try and load the type library */
    if (LoadTypeLibEx(tlibPath, REGKIND_NONE, &typeLib) != S_OK) {
        result = TYPE_E_INVALIDSTATE;
        goto end;
    }

    /* remove any types registered with this typelib */
    numTypes = ITypeLib_GetTypeInfoCount(typeLib);
    for (i=0; i<numTypes; i++) {
        /* get the kind of type */
        if (ITypeLib_GetTypeInfoType(typeLib, i, &kind) != S_OK) {
            goto enddeleteloop;
        }

        /* skip non-interfaces, and get type info for the type */
        if ((kind != TKIND_INTERFACE) && (kind != TKIND_DISPATCH)) {
            goto enddeleteloop;
        }
        if (ITypeLib_GetTypeInfo(typeLib, i, &typeInfo) != S_OK) {
            goto enddeleteloop;
        }
        if (ITypeInfo_GetTypeAttr(typeInfo, &typeAttr) != S_OK) {
            goto enddeleteloop;
        }

        if ((kind == TKIND_INTERFACE && (typeAttr->wTypeFlags & TYPEFLAG_FOLEAUTOMATION)) ||
            kind == TKIND_DISPATCH)
        {
            BOOL is_wow64;
            REGSAM opposite = (sizeof(void*) == 8 ? KEY_WOW64_32KEY : KEY_WOW64_64KEY);

            TLB_unregister_interface(&typeAttr->guid, 0);

            /* unregister TLBs into the opposite registry view, too */
            if(opposite == KEY_WOW64_32KEY ||
               (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)) {
                TLB_unregister_interface(&typeAttr->guid, opposite);
            }
        }

enddeleteloop:
        if (typeAttr) ITypeInfo_ReleaseTypeAttr(typeInfo, typeAttr);
        typeAttr = NULL;
        if (typeInfo) ITypeInfo_Release(typeInfo);
        typeInfo = NULL;
    }

    /* Now, delete the type library path subkey */
    get_lcid_subkey( lcid, syskind, subKeyName );
    RegDeleteKeyW(key, subKeyName);
    *wcsrchr( subKeyName, '\\' ) = 0;  /* remove last path component */
    RegDeleteKeyW(key, subKeyName);

    /* check if there is anything besides the FLAGS/HELPDIR keys.
       If there is, we don't delete them */
    tmpLength = ARRAY_SIZE(subKeyName);
    deleteOtherStuff = TRUE;
    i = 0;
    while(RegEnumKeyExW(key, i++, subKeyName, &tmpLength, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        tmpLength = ARRAY_SIZE(subKeyName);

        /* if its not FLAGS or HELPDIR, then we must keep the rest of the key */
        if (!wcscmp(subKeyName, L"FLAGS")) continue;
        if (!wcscmp(subKeyName, L"HELPDIR")) continue;
        deleteOtherStuff = FALSE;
        break;
    }

    /* only delete the other parts of the key if we're absolutely sure */
    if (deleteOtherStuff) {
        RegDeleteKeyW(key, L"FLAGS");
        RegDeleteKeyW(key, L"HELPDIR");
        RegCloseKey(key);
        key = NULL;

        RegDeleteKeyW(HKEY_CLASSES_ROOT, keyName);
        *wcsrchr( keyName, '\\' ) = 0;  /* remove last path component */
        RegDeleteKeyW(HKEY_CLASSES_ROOT, keyName);
    }

end:
    SysFreeString(tlibPath);
    if (typeLib) ITypeLib_Release(typeLib);
    if (key) RegCloseKey(key);
    return result;
}

/******************************************************************************
 *		RegisterTypeLibForUser	[OLEAUT32.442]
 * Adds information about a type library to the user registry
 * NOTES
 *    Docs: ITypeLib FAR * ptlib
 *    Docs: OLECHAR FAR* szFullPath
 *    Docs: OLECHAR FAR* szHelpDir
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI RegisterTypeLibForUser(
     ITypeLib * ptlib,     /* [in] Pointer to the library*/
     OLECHAR * szFullPath, /* [in] full Path of the library*/
     OLECHAR * szHelpDir)  /* [in] dir to the helpfile for the library,
							 may be NULL*/
{
    FIXME("(%p, %s, %s) registering the typelib system-wide\n", ptlib,
          debugstr_w(szFullPath), debugstr_w(szHelpDir));
    return RegisterTypeLib(ptlib, szFullPath, szHelpDir);
}

/******************************************************************************
 *	UnRegisterTypeLibForUser	[OLEAUT32.443]
 * Removes information about a type library from the user registry
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI UnRegisterTypeLibForUser(
    REFGUID libid,	/* [in] GUID of the library */
    WORD wVerMajor,	/* [in] major version */
    WORD wVerMinor,	/* [in] minor version */
    LCID lcid,	/* [in] locale id */
    SYSKIND syskind)
{
    FIXME("%s, %u, %u, %#lx, %u unregistering the typelib system-wide\n",
          debugstr_guid(libid), wVerMajor, wVerMinor, lcid, syskind);
    return UnRegisterTypeLib(libid, wVerMajor, wVerMinor, lcid, syskind);
}

/*======================= ITypeLib implementation =======================*/

typedef struct tagTLBGuid {
    GUID guid;
    INT hreftype;
    UINT offset;
    struct list entry;
} TLBGuid;

typedef struct tagTLBCustData
{
    TLBGuid *guid;
    VARIANT data;
    struct list entry;
} TLBCustData;

/* data structure for import typelibs */
typedef struct tagTLBImpLib
{
    int offset;                 /* offset in the file (MSFT)
				   offset in nametable (SLTG)
				   just used to identify library while reading
				   data from file */
    TLBGuid *guid;                  /* libid */
    BSTR name;                  /* name */

    LCID lcid;                  /* lcid of imported typelib */

    WORD wVersionMajor;         /* major version number */
    WORD wVersionMinor;         /* minor version number */

    struct tagITypeLibImpl *pImpTypeLib; /* pointer to loaded typelib, or
					    NULL if not yet loaded */
    struct list entry;
} TLBImpLib;

typedef struct tagTLBString {
    BSTR str;
    UINT offset;
    struct list entry;
} TLBString;

/* internal ITypeLib data */
typedef struct tagITypeLibImpl
{
    ITypeLib2 ITypeLib2_iface;
    ITypeComp ITypeComp_iface;
    ICreateTypeLib2 ICreateTypeLib2_iface;
    LONG ref;
    TLBGuid *guid;
    LCID lcid;
    SYSKIND syskind;
    int ptr_size;
    WORD ver_major;
    WORD ver_minor;
    WORD libflags;
    LCID set_lcid;

    /* strings can be stored in tlb as multibyte strings BUT they are *always*
     * exported to the application as a UNICODE string.
     */
    struct list string_list;
    struct list name_list;
    struct list guid_list;

    const TLBString *Name;
    const TLBString *DocString;
    const TLBString *HelpFile;
    const TLBString *HelpStringDll;
    DWORD dwHelpContext;
    int TypeInfoCount;          /* nr of typeinfo's in librarry */
    struct tagITypeInfoImpl **typeinfos;
    struct list custdata_list;
    struct list implib_list;
    int ctTypeDesc;             /* number of items in type desc array */
    TYPEDESC * pTypeDesc;       /* array of TypeDescriptions found in the
				   library. Only used while reading MSFT
				   typelibs */
    struct list ref_list;       /* list of ref types in this typelib */
    HREFTYPE dispatch_href;     /* reference to IDispatch, -1 if unused */


    /* typelibs are cached, keyed by path and index, so store the linked list info within them */
    struct list entry;
    WCHAR *path;
    INT index;
} ITypeLibImpl;

static const ITypeLib2Vtbl tlbvt;
static const ITypeCompVtbl tlbtcvt;
static const ICreateTypeLib2Vtbl CreateTypeLib2Vtbl;

static inline ITypeLibImpl *impl_from_ITypeLib2(ITypeLib2 *iface)
{
    return CONTAINING_RECORD(iface, ITypeLibImpl, ITypeLib2_iface);
}

static inline ITypeLibImpl *impl_from_ITypeLib(ITypeLib *iface)
{
    return impl_from_ITypeLib2((ITypeLib2*)iface);
}

static inline ITypeLibImpl *impl_from_ITypeComp( ITypeComp *iface )
{
    return CONTAINING_RECORD(iface, ITypeLibImpl, ITypeComp_iface);
}

static inline ITypeLibImpl *impl_from_ICreateTypeLib2( ICreateTypeLib2 *iface )
{
    return CONTAINING_RECORD(iface, ITypeLibImpl, ICreateTypeLib2_iface);
}

/* ITypeLib methods */
static ITypeLib2* ITypeLib2_Constructor_MSFT(LPVOID pLib, DWORD dwTLBLength);
static ITypeLib2* ITypeLib2_Constructor_SLTG(LPVOID pLib, DWORD dwTLBLength);

/*======================= ITypeInfo implementation =======================*/

/* data for referenced types */
typedef struct tagTLBRefType
{
    INT index;              /* Type index for internal ref or for external ref
			       it the format is SLTG.  -2 indicates to
			       use guid */

    TYPEKIND tkind;
    TLBGuid *guid;              /* guid of the referenced type */
                            /* if index == TLB_REF_USE_GUID */

    HREFTYPE reference;     /* The href of this ref */
    TLBImpLib *pImpTLInfo;  /* If ref is external ptr to library data
			       TLB_REF_INTERNAL for internal refs
			       TLB_REF_NOT_FOUND for broken refs */

    struct list entry;
} TLBRefType;

#define TLB_REF_USE_GUID -2

#define TLB_REF_INTERNAL (void*)-2
#define TLB_REF_NOT_FOUND (void*)-1

/* internal Parameter data */
typedef struct tagTLBParDesc
{
    const TLBString *Name;
    struct list custdata_list;
} TLBParDesc;

/* internal Function data */
typedef struct tagTLBFuncDesc
{
    FUNCDESC funcdesc;      /* lots of info on the function and its attributes. */
    const TLBString *Name;             /* the name of this function */
    TLBParDesc *pParamDesc; /* array with param names and custom data */
    int helpcontext;
    int HelpStringContext;
    const TLBString *HelpString;
    const TLBString *Entry;            /* if IS_INTRESOURCE true, it's numeric; if -1 it isn't present */
    struct list custdata_list;
} TLBFuncDesc;

/* internal Variable data */
typedef struct tagTLBVarDesc
{
    VARDESC vardesc;                /* lots of info on the variable and its attributes. */
    VARDESC *vardesc_create;        /* additional data needed for storing VARDESC */
    const TLBString *Name;          /* the name of this variable */
    int HelpContext;
    int HelpStringContext;
    const TLBString *HelpString;
    struct list custdata_list;
} TLBVarDesc;

/* internal implemented interface data */
typedef struct tagTLBImplType
{
    HREFTYPE hRef;          /* hRef of interface */
    int implflags;          /* IMPLFLAG_*s */
    struct list custdata_list;
} TLBImplType;

/* internal TypeInfo data */
typedef struct tagITypeInfoImpl
{
    ITypeInfo2 ITypeInfo2_iface;
    ITypeComp ITypeComp_iface;
    ICreateTypeInfo2 ICreateTypeInfo2_iface;
    LONG ref;
    BOOL not_attached_to_typelib;
    BOOL needs_layout;

    TLBGuid *guid;
    TYPEATTR typeattr;
    TYPEDESC *tdescAlias;

    ITypeLibImpl * pTypeLib;        /* back pointer to typelib */
    int index;                  /* index in this typelib; */
    HREFTYPE hreftype;          /* hreftype for app object binding */
    /* type libs seem to store the doc strings in ascii
     * so why should we do it in unicode?
     */
    const TLBString *Name;
    const TLBString *DocString;
    const TLBString *DllName;
    const TLBString *Schema;
    DWORD dwHelpContext;
    DWORD dwHelpStringContext;

    /* functions  */
    TLBFuncDesc *funcdescs;

    /* variables  */
    TLBVarDesc *vardescs;

    /* Implemented Interfaces  */
    TLBImplType *impltypes;

    struct list *pcustdata_list;
    struct list custdata_list;
} ITypeInfoImpl;

static inline ITypeInfoImpl *info_impl_from_ITypeComp( ITypeComp *iface )
{
    return CONTAINING_RECORD(iface, ITypeInfoImpl, ITypeComp_iface);
}

static inline ITypeInfoImpl *impl_from_ITypeInfo2( ITypeInfo2 *iface )
{
    return CONTAINING_RECORD(iface, ITypeInfoImpl, ITypeInfo2_iface);
}

static inline ITypeInfoImpl *impl_from_ITypeInfo( ITypeInfo *iface )
{
    return impl_from_ITypeInfo2((ITypeInfo2*)iface);
}

static inline ITypeInfoImpl *info_impl_from_ICreateTypeInfo2( ICreateTypeInfo2 *iface )
{
    return CONTAINING_RECORD(iface, ITypeInfoImpl, ICreateTypeInfo2_iface);
}

static const ITypeInfo2Vtbl tinfvt;
static const ITypeCompVtbl  tcompvt;
static const ICreateTypeInfo2Vtbl CreateTypeInfo2Vtbl;

static ITypeInfoImpl* ITypeInfoImpl_Constructor(void);
static void ITypeInfoImpl_Destroy(ITypeInfoImpl *This);

typedef struct tagTLBContext
{
	unsigned int oStart;  /* start of TLB in file */
	unsigned int pos;     /* current pos */
	unsigned int length;  /* total length */
	void *mapping;        /* memory mapping */
	MSFT_SegDir * pTblDir;
	ITypeLibImpl* pLibInfo;
} TLBContext;


static inline BSTR TLB_get_bstr(const TLBString *str)
{
    return str != NULL ? str->str : NULL;
}

static inline int TLB_str_memcmp(void *left, const TLBString *str, DWORD len)
{
    if(!str)
        return 1;
    return memcmp(left, str->str, len);
}

static inline const GUID *TLB_get_guidref(const TLBGuid *guid)
{
    return guid != NULL ? &guid->guid : NULL;
}

static inline const GUID *TLB_get_guid_null(const TLBGuid *guid)
{
    return guid != NULL ? &guid->guid : &GUID_NULL;
}

static int get_ptr_size(SYSKIND syskind)
{
    switch(syskind){
    case SYS_WIN64:
        return 8;
    case SYS_WIN32:
    case SYS_MAC:
    case SYS_WIN16:
        return 4;
    }
    WARN("Unhandled syskind: 0x%x\n", syskind);
    return 4;
}

/*
 debug
*/
static void dump_TypeDesc(const TYPEDESC *pTD,char *szVarType) {
    if (pTD->vt & VT_RESERVED)
	szVarType += strlen(strcpy(szVarType, "reserved | "));
    if (pTD->vt & VT_BYREF)
	szVarType += strlen(strcpy(szVarType, "ref to "));
    if (pTD->vt & VT_ARRAY)
	szVarType += strlen(strcpy(szVarType, "array of "));
    if (pTD->vt & VT_VECTOR)
	szVarType += strlen(strcpy(szVarType, "vector of "));
    switch(pTD->vt & VT_TYPEMASK) {
    case VT_UI1: sprintf(szVarType, "VT_UI1"); break;
    case VT_I2: sprintf(szVarType, "VT_I2"); break;
    case VT_I4: sprintf(szVarType, "VT_I4"); break;
    case VT_R4: sprintf(szVarType, "VT_R4"); break;
    case VT_R8: sprintf(szVarType, "VT_R8"); break;
    case VT_BOOL: sprintf(szVarType, "VT_BOOL"); break;
    case VT_ERROR: sprintf(szVarType, "VT_ERROR"); break;
    case VT_CY: sprintf(szVarType, "VT_CY"); break;
    case VT_DATE: sprintf(szVarType, "VT_DATE"); break;
    case VT_BSTR: sprintf(szVarType, "VT_BSTR"); break;
    case VT_UNKNOWN: sprintf(szVarType, "VT_UNKNOWN"); break;
    case VT_DISPATCH: sprintf(szVarType, "VT_DISPATCH"); break;
    case VT_I1: sprintf(szVarType, "VT_I1"); break;
    case VT_UI2: sprintf(szVarType, "VT_UI2"); break;
    case VT_UI4: sprintf(szVarType, "VT_UI4"); break;
    case VT_INT: sprintf(szVarType, "VT_INT"); break;
    case VT_UINT: sprintf(szVarType, "VT_UINT"); break;
    case VT_VARIANT: sprintf(szVarType, "VT_VARIANT"); break;
    case VT_VOID: sprintf(szVarType, "VT_VOID"); break;
    case VT_HRESULT: sprintf(szVarType, "VT_HRESULT"); break;
    case VT_USERDEFINED: sprintf(szVarType, "VT_USERDEFINED ref = %lx", pTD->hreftype); break;
    case VT_LPSTR: sprintf(szVarType, "VT_LPSTR"); break;
    case VT_LPWSTR: sprintf(szVarType, "VT_LPWSTR"); break;
    case VT_PTR: sprintf(szVarType, "ptr to ");
      dump_TypeDesc(pTD->lptdesc, szVarType + 7);
      break;
    case VT_SAFEARRAY: sprintf(szVarType, "safearray of ");
      dump_TypeDesc(pTD->lptdesc, szVarType + 13);
      break;
    case VT_CARRAY: sprintf(szVarType, "%d dim array of ",
			    pTD->lpadesc->cDims); /* FIXME print out sizes */
      dump_TypeDesc(&pTD->lpadesc->tdescElem, szVarType + strlen(szVarType));
      break;

    default: sprintf(szVarType, "unknown(%d)", pTD->vt & VT_TYPEMASK); break;
    }
}

static void dump_ELEMDESC(const ELEMDESC *edesc) {
  char buf[200];
  USHORT flags = edesc->paramdesc.wParamFlags;
  dump_TypeDesc(&edesc->tdesc,buf);
  MESSAGE("\t\ttdesc.vartype %d (%s)\n",edesc->tdesc.vt,buf);
  MESSAGE("\t\tu.paramdesc.wParamFlags");
  if (!flags) MESSAGE(" PARAMFLAGS_NONE");
  if (flags & PARAMFLAG_FIN) MESSAGE(" PARAMFLAG_FIN");
  if (flags & PARAMFLAG_FOUT) MESSAGE(" PARAMFLAG_FOUT");
  if (flags & PARAMFLAG_FLCID) MESSAGE(" PARAMFLAG_FLCID");
  if (flags & PARAMFLAG_FRETVAL) MESSAGE(" PARAMFLAG_FRETVAL");
  if (flags & PARAMFLAG_FOPT) MESSAGE(" PARAMFLAG_FOPT");
  if (flags & PARAMFLAG_FHASDEFAULT) MESSAGE(" PARAMFLAG_FHASDEFAULT");
  if (flags & PARAMFLAG_FHASCUSTDATA) MESSAGE(" PARAMFLAG_FHASCUSTDATA");
  MESSAGE("\n\t\tu.paramdesc.lpex %p\n",edesc->paramdesc.pparamdescex);
}
static void dump_FUNCDESC(const FUNCDESC *funcdesc) {
  int i;
  MESSAGE("memid is %#lx\n", funcdesc->memid);
  for (i=0;i<funcdesc->cParams;i++) {
      MESSAGE("Param %d:\n",i);
      dump_ELEMDESC(funcdesc->lprgelemdescParam+i);
  }
  MESSAGE("\tfunckind: %d (",funcdesc->funckind);
  switch (funcdesc->funckind) {
  case FUNC_VIRTUAL: MESSAGE("virtual");break;
  case FUNC_PUREVIRTUAL: MESSAGE("pure virtual");break;
  case FUNC_NONVIRTUAL: MESSAGE("nonvirtual");break;
  case FUNC_STATIC: MESSAGE("static");break;
  case FUNC_DISPATCH: MESSAGE("dispatch");break;
  default: MESSAGE("unknown");break;
  }
  MESSAGE(")\n\tinvkind: %d (",funcdesc->invkind);
  switch (funcdesc->invkind) {
  case INVOKE_FUNC: MESSAGE("func");break;
  case INVOKE_PROPERTYGET: MESSAGE("property get");break;
  case INVOKE_PROPERTYPUT: MESSAGE("property put");break;
  case INVOKE_PROPERTYPUTREF: MESSAGE("property put ref");break;
  }
  MESSAGE(")\n\tcallconv: %d (",funcdesc->callconv);
  switch (funcdesc->callconv) {
  case CC_CDECL: MESSAGE("cdecl");break;
  case CC_PASCAL: MESSAGE("pascal");break;
  case CC_STDCALL: MESSAGE("stdcall");break;
  case CC_SYSCALL: MESSAGE("syscall");break;
  default:break;
  }
  MESSAGE(")\n\toVft: %d\n", funcdesc->oVft);
  MESSAGE("\tcParamsOpt: %d\n", funcdesc->cParamsOpt);
  MESSAGE("\twFlags: %x\n", funcdesc->wFuncFlags);

  MESSAGE("\telemdescFunc (return value type):\n");
  dump_ELEMDESC(&funcdesc->elemdescFunc);
}

static const char * const typekind_desc[] =
{
	"TKIND_ENUM",
	"TKIND_RECORD",
	"TKIND_MODULE",
	"TKIND_INTERFACE",
	"TKIND_DISPATCH",
	"TKIND_COCLASS",
	"TKIND_ALIAS",
	"TKIND_UNION",
	"TKIND_MAX"
};

static void dump_TLBFuncDescOne(const TLBFuncDesc * pfd)
{
  int i;
  MESSAGE("%s(%u)\n", debugstr_w(TLB_get_bstr(pfd->Name)), pfd->funcdesc.cParams);
  for (i=0;i<pfd->funcdesc.cParams;i++)
      MESSAGE("\tparm%d: %s\n",i,debugstr_w(TLB_get_bstr(pfd->pParamDesc[i].Name)));


  dump_FUNCDESC(&(pfd->funcdesc));

  MESSAGE("\thelpstring: %s\n", debugstr_w(TLB_get_bstr(pfd->HelpString)));
  if(pfd->Entry == NULL)
      MESSAGE("\tentry: (null)\n");
  else if(pfd->Entry == (void*)-1)
      MESSAGE("\tentry: invalid\n");
  else if(IS_INTRESOURCE(pfd->Entry))
      MESSAGE("\tentry: %p\n", pfd->Entry);
  else
      MESSAGE("\tentry: %s\n", debugstr_w(TLB_get_bstr(pfd->Entry)));
}
static void dump_TLBFuncDesc(const TLBFuncDesc * pfd, UINT n)
{
	while (n)
	{
	  dump_TLBFuncDescOne(pfd);
	  ++pfd;
	  --n;
	}
}
static void dump_TLBVarDesc(const TLBVarDesc * pvd, UINT n)
{
	while (n)
	{
	  TRACE_(typelib)("%s\n", debugstr_w(TLB_get_bstr(pvd->Name)));
	  ++pvd;
	  --n;
	}
}

static void dump_TLBImpLib(const TLBImpLib *import)
{
    TRACE_(typelib)("%s %s\n", debugstr_guid(TLB_get_guidref(import->guid)),
		    debugstr_w(import->name));
    TRACE_(typelib)("v%d.%d lcid %#lx offset=%x\n", import->wVersionMajor, import->wVersionMinor, import->lcid, import->offset);
}

static void dump_TLBRefType(const ITypeLibImpl *pTL)
{
    TLBRefType *ref;

    LIST_FOR_EACH_ENTRY(ref, &pTL->ref_list, TLBRefType, entry)
    {
        TRACE_(typelib)("href:%#lx\n", ref->reference);
        if(ref->index == -1)
	    TRACE_(typelib)("%s\n", debugstr_guid(TLB_get_guidref(ref->guid)));
        else
	    TRACE_(typelib)("type no: %d\n", ref->index);

        if(ref->pImpTLInfo != TLB_REF_INTERNAL && ref->pImpTLInfo != TLB_REF_NOT_FOUND)
        {
            TRACE_(typelib)("in lib\n");
            dump_TLBImpLib(ref->pImpTLInfo);
        }
    }
}

static void dump_TLBImplType(const TLBImplType * impl, UINT n)
{
    if(!impl)
        return;
    while (n) {
        TRACE_(typelib)("implementing/inheriting interface hRef = %lx implflags %x\n",
            impl->hRef, impl->implflags);
        ++impl;
        --n;
    }
}

static void dump_DispParms(const DISPPARAMS * pdp)
{
    unsigned int index;

    TRACE("args=%u named args=%u\n", pdp->cArgs, pdp->cNamedArgs);

    if (pdp->cNamedArgs && pdp->rgdispidNamedArgs)
    {
        TRACE("named args:\n");
        for (index = 0; index < pdp->cNamedArgs; index++)
            TRACE( "\t0x%lx\n", pdp->rgdispidNamedArgs[index] );
    }

    if (pdp->cArgs && pdp->rgvarg)
    {
        TRACE("args:\n");
        for (index = 0; index < pdp->cArgs; index++)
            TRACE("  [%d] %s\n", index, debugstr_variant(pdp->rgvarg+index));
    }
}

static void dump_TypeInfo(const ITypeInfoImpl * pty)
{
    TRACE("%p ref %lu\n", pty, pty->ref);
    TRACE("%s %s\n", debugstr_w(TLB_get_bstr(pty->Name)), debugstr_w(TLB_get_bstr(pty->DocString)));
    TRACE("attr:%s\n", debugstr_guid(TLB_get_guidref(pty->guid)));
    TRACE("kind:%s\n", typekind_desc[pty->typeattr.typekind]);
    TRACE("fct:%u var:%u impl:%u\n", pty->typeattr.cFuncs, pty->typeattr.cVars, pty->typeattr.cImplTypes);
    TRACE("wTypeFlags: 0x%04x\n", pty->typeattr.wTypeFlags);
    TRACE("parent tlb:%p index in TLB:%u\n",pty->pTypeLib, pty->index);
    if (pty->typeattr.typekind == TKIND_MODULE) TRACE("dllname:%s\n", debugstr_w(TLB_get_bstr(pty->DllName)));
    if (TRACE_ON(ole))
        dump_TLBFuncDesc(pty->funcdescs, pty->typeattr.cFuncs);
    dump_TLBVarDesc(pty->vardescs, pty->typeattr.cVars);
    dump_TLBImplType(pty->impltypes, pty->typeattr.cImplTypes);
}

static void dump_VARDESC(const VARDESC *v)
{
    MESSAGE("memid %ld\n",v->memid);
    MESSAGE("lpstrSchema %s\n",debugstr_w(v->lpstrSchema));
    MESSAGE("oInst %ld\n", v->oInst);
    dump_ELEMDESC(&(v->elemdescVar));
    MESSAGE("wVarFlags %x\n",v->wVarFlags);
    MESSAGE("varkind %d\n",v->varkind);
}

static TYPEDESC std_typedesc[VT_LPWSTR+1] =
{
    /* VT_LPWSTR is largest type that, may appear in type description */
    {{0}, VT_EMPTY},  {{0}, VT_NULL},        {{0}, VT_I2},      {{0}, VT_I4},
    {{0}, VT_R4},     {{0}, VT_R8},          {{0}, VT_CY},      {{0}, VT_DATE},
    {{0}, VT_BSTR},   {{0}, VT_DISPATCH},    {{0}, VT_ERROR},   {{0}, VT_BOOL},
    {{0}, VT_VARIANT},{{0}, VT_UNKNOWN},     {{0}, VT_DECIMAL}, {{0}, 15}, /* unused in VARENUM */
    {{0}, VT_I1},     {{0}, VT_UI1},         {{0}, VT_UI2},     {{0}, VT_UI4},
    {{0}, VT_I8},     {{0}, VT_UI8},         {{0}, VT_INT},     {{0}, VT_UINT},
    {{0}, VT_VOID},   {{0}, VT_HRESULT},     {{0}, VT_PTR},     {{0}, VT_SAFEARRAY},
    {{0}, VT_CARRAY}, {{0}, VT_USERDEFINED}, {{0}, VT_LPSTR},   {{0}, VT_LPWSTR}
};

static void TLB_abort(void)
{
    DebugBreak();
}

/* returns the size required for a deep copy of a typedesc into a
 * flat buffer */
static SIZE_T TLB_SizeTypeDesc( const TYPEDESC *tdesc, BOOL alloc_initial_space )
{
    SIZE_T size = 0;

    if (alloc_initial_space)
        size += sizeof(TYPEDESC);

    switch (tdesc->vt)
    {
    case VT_PTR:
    case VT_SAFEARRAY:
        size += TLB_SizeTypeDesc(tdesc->lptdesc, TRUE);
        break;
    case VT_CARRAY:
        size += FIELD_OFFSET(ARRAYDESC, rgbounds[tdesc->lpadesc->cDims]);
        size += TLB_SizeTypeDesc(&tdesc->lpadesc->tdescElem, FALSE);
        break;
    }
    return size;
}

/* deep copy a typedesc into a flat buffer */
static void *TLB_CopyTypeDesc( TYPEDESC *dest, const TYPEDESC *src, void *buffer )
{
    if (!dest)
    {
        dest = buffer;
        buffer = (char *)buffer + sizeof(TYPEDESC);
    }

    *dest = *src;

    switch (src->vt)
    {
    case VT_PTR:
    case VT_SAFEARRAY:
        dest->lptdesc = buffer;
        buffer = TLB_CopyTypeDesc(NULL, src->lptdesc, buffer);
        break;
    case VT_CARRAY:
        dest->lpadesc = buffer;
        memcpy(dest->lpadesc, src->lpadesc, FIELD_OFFSET(ARRAYDESC, rgbounds[src->lpadesc->cDims]));
        buffer = (char *)buffer + FIELD_OFFSET(ARRAYDESC, rgbounds[src->lpadesc->cDims]);
        buffer = TLB_CopyTypeDesc(&dest->lpadesc->tdescElem, &src->lpadesc->tdescElem, buffer);
        break;
    }
    return buffer;
}

/* free custom data allocated by MSFT_CustData */
static inline void TLB_FreeCustData(struct list *custdata_list)
{
    TLBCustData *cd, *cdn;
    LIST_FOR_EACH_ENTRY_SAFE(cd, cdn, custdata_list, TLBCustData, entry)
    {
        list_remove(&cd->entry);
        VariantClear(&cd->data);
        free(cd);
    }
}

static BSTR TLB_MultiByteToBSTR(const char *ptr)
{
    DWORD len;
    BSTR ret;

    len = MultiByteToWideChar(CP_ACP, 0, ptr, -1, NULL, 0);
    ret = SysAllocStringLen(NULL, len - 1);
    if (!ret) return ret;
    MultiByteToWideChar(CP_ACP, 0, ptr, -1, ret, len);
    return ret;
}

static inline TLBFuncDesc *TLB_get_funcdesc_by_memberid(ITypeInfoImpl *typeinfo, MEMBERID memid)
{
    int i;

    for (i = 0; i < typeinfo->typeattr.cFuncs; ++i)
    {
        if (typeinfo->funcdescs[i].funcdesc.memid == memid)
            return &typeinfo->funcdescs[i];
    }

    return NULL;
}

static inline TLBFuncDesc *TLB_get_funcdesc_by_memberid_invkind(ITypeInfoImpl *typeinfo, MEMBERID memid, INVOKEKIND invkind)
{
    int i;

    for (i = 0; i < typeinfo->typeattr.cFuncs; ++i)
    {
        if (typeinfo->funcdescs[i].funcdesc.memid == memid && typeinfo->funcdescs[i].funcdesc.invkind == invkind)
            return &typeinfo->funcdescs[i];
    }

    return NULL;
}

static inline TLBVarDesc *TLB_get_vardesc_by_memberid(ITypeInfoImpl *typeinfo, MEMBERID memid)
{
    int i;

    for (i = 0; i < typeinfo->typeattr.cVars; ++i)
    {
        if (typeinfo->vardescs[i].vardesc.memid == memid)
            return &typeinfo->vardescs[i];
    }

    return NULL;
}

static inline TLBVarDesc *TLB_get_vardesc_by_name(ITypeInfoImpl *typeinfo, const OLECHAR *name)
{
    int i;

    for (i = 0; i < typeinfo->typeattr.cVars; ++i)
    {
        if (!lstrcmpiW(TLB_get_bstr(typeinfo->vardescs[i].Name), name))
            return &typeinfo->vardescs[i];
    }

    return NULL;
}

static inline TLBCustData *TLB_get_custdata_by_guid(const struct list *custdata_list, REFGUID guid)
{
    TLBCustData *cust_data;
    LIST_FOR_EACH_ENTRY(cust_data, custdata_list, TLBCustData, entry)
        if(IsEqualIID(TLB_get_guid_null(cust_data->guid), guid))
            return cust_data;
    return NULL;
}

static inline ITypeInfoImpl *TLB_get_typeinfo_by_name(ITypeLibImpl *typelib, const OLECHAR *name)
{
    int i;

    for (i = 0; i < typelib->TypeInfoCount; ++i)
    {
        if (!lstrcmpiW(TLB_get_bstr(typelib->typeinfos[i]->Name), name))
            return typelib->typeinfos[i];
    }

    return NULL;
}

static void TLBVarDesc_Constructor(TLBVarDesc *var_desc)
{
    list_init(&var_desc->custdata_list);
}

static TLBVarDesc *TLBVarDesc_Alloc(UINT n)
{
    TLBVarDesc *ret;

    ret = calloc(n, sizeof(TLBVarDesc));
    if(!ret)
        return NULL;

    while(n){
        TLBVarDesc_Constructor(&ret[n-1]);
        --n;
    }

    return ret;
}

static TLBParDesc *TLBParDesc_Constructor(UINT n)
{
    TLBParDesc *ret;

    ret = calloc(n, sizeof(TLBParDesc));
    if(!ret)
        return NULL;

    while(n){
        list_init(&ret[n-1].custdata_list);
        --n;
    }

    return ret;
}

static void TLBFuncDesc_Constructor(TLBFuncDesc *func_desc)
{
    list_init(&func_desc->custdata_list);
}

static TLBFuncDesc *TLBFuncDesc_Alloc(UINT n)
{
    TLBFuncDesc *ret;

    ret = calloc(n, sizeof(TLBFuncDesc));
    if(!ret)
        return NULL;

    while(n){
        TLBFuncDesc_Constructor(&ret[n-1]);
        --n;
    }

    return ret;
}

static void TLBImplType_Constructor(TLBImplType *impl)
{
    list_init(&impl->custdata_list);
}

static TLBImplType *TLBImplType_Alloc(UINT n)
{
    TLBImplType *ret;

    ret = calloc(n, sizeof(TLBImplType));
    if(!ret)
        return NULL;

    while(n){
        TLBImplType_Constructor(&ret[n-1]);
        --n;
    }

    return ret;
}

static TLBGuid *TLB_append_guid(struct list *guid_list,
        const GUID *new_guid, HREFTYPE hreftype)
{
    TLBGuid *guid;

    LIST_FOR_EACH_ENTRY(guid, guid_list, TLBGuid, entry) {
        if (IsEqualGUID(&guid->guid, new_guid))
            return guid;
    }

    guid = malloc(sizeof(TLBGuid));
    if (!guid)
        return NULL;

    memcpy(&guid->guid, new_guid, sizeof(GUID));
    guid->hreftype = hreftype;

    list_add_tail(guid_list, &guid->entry);

    return guid;
}

static HRESULT TLB_set_custdata(struct list *custdata_list, TLBGuid *tlbguid, VARIANT *var)
{
    TLBCustData *cust_data;

    switch(V_VT(var)){
    case VT_I4:
    case VT_R4:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_HRESULT:
    case VT_BSTR:
        break;
    default:
        return DISP_E_BADVARTYPE;
    }

    cust_data = TLB_get_custdata_by_guid(custdata_list, TLB_get_guid_null(tlbguid));

    if (!cust_data) {
        cust_data = malloc(sizeof(TLBCustData));
        if (!cust_data)
            return E_OUTOFMEMORY;

        cust_data->guid = tlbguid;
        VariantInit(&cust_data->data);

        list_add_tail(custdata_list, &cust_data->entry);
    }else
        VariantClear(&cust_data->data);

    return VariantCopy(&cust_data->data, var);
}

/* Used to update list pointers after list itself was moved. */
static void TLB_relink_custdata(struct list *custdata_list)
{
    if (custdata_list->prev == custdata_list->next)
        list_init(custdata_list);
    else
    {
        custdata_list->prev->next = custdata_list;
        custdata_list->next->prev = custdata_list;
    }
}

static TLBString *TLB_append_str(struct list *string_list, BSTR new_str)
{
    TLBString *str;

    if(!new_str)
        return NULL;

    LIST_FOR_EACH_ENTRY(str, string_list, TLBString, entry) {
        if (wcscmp(str->str, new_str) == 0)
            return str;
    }

    str = malloc(sizeof(TLBString));
    if (!str)
        return NULL;

    str->str = SysAllocString(new_str);
    if (!str->str) {
        free(str);
        return NULL;
    }

    list_add_tail(string_list, &str->entry);

    return str;
}

static HRESULT TLB_get_size_from_hreftype(ITypeInfoImpl *info, HREFTYPE href,
        ULONG *size, WORD *align)
{
    ITypeInfo *other;
    TYPEATTR *attr;
    HRESULT hr;

    hr = ITypeInfo2_GetRefTypeInfo(&info->ITypeInfo2_iface, href, &other);
    if(FAILED(hr))
        return hr;

    hr = ITypeInfo_GetTypeAttr(other, &attr);
    if(FAILED(hr)){
        ITypeInfo_Release(other);
        return hr;
    }

    if(size)
        *size = attr->cbSizeInstance;
    if(align)
        *align = attr->cbAlignment;

    ITypeInfo_ReleaseTypeAttr(other, attr);
    ITypeInfo_Release(other);

    return S_OK;
}

static HRESULT TLB_size_instance(ITypeInfoImpl *info, SYSKIND sys,
        TYPEDESC *tdesc, ULONG *size, WORD *align)
{
    ULONG i, sub, ptr_size;
    HRESULT hr;

    ptr_size = get_ptr_size(sys);

    switch(tdesc->vt){
    case VT_VOID:
        *size = 0;
        break;
    case VT_I1:
    case VT_UI1:
        *size = 1;
        break;
    case VT_I2:
    case VT_BOOL:
    case VT_UI2:
        *size = 2;
        break;
    case VT_I4:
    case VT_R4:
    case VT_ERROR:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_HRESULT:
        *size = 4;
        break;
    case VT_R8:
    case VT_I8:
    case VT_UI8:
        *size = 8;
        break;
    case VT_BSTR:
    case VT_DISPATCH:
    case VT_UNKNOWN:
    case VT_PTR:
    case VT_SAFEARRAY:
    case VT_LPSTR:
    case VT_LPWSTR:
        *size = ptr_size;
        break;
    case VT_DATE:
        *size = sizeof(DATE);
        break;
    case VT_VARIANT:
        *size = sizeof(VARIANT);
        if(get_ptr_size(sys) != sizeof(void*))
            *size += is_win64 ? -8 : 8; /* 32-bit VARIANT is 8 bytes smaller than 64-bit VARIANT */
        break;
    case VT_DECIMAL:
        *size = sizeof(DECIMAL);
        break;
    case VT_CY:
        *size = sizeof(CY);
        break;
    case VT_CARRAY:
        *size = 0;
        for(i = 0; i < tdesc->lpadesc->cDims; ++i)
            *size += tdesc->lpadesc->rgbounds[i].cElements;
        hr = TLB_size_instance(info, sys, &tdesc->lpadesc->tdescElem, &sub, align);
        if(FAILED(hr))
            return hr;
        *size *= sub;
        return S_OK;
    case VT_USERDEFINED:
        return TLB_get_size_from_hreftype(info, tdesc->hreftype, size, align);
    default:
        FIXME("Unsized VT: 0x%x\n", tdesc->vt);
        return E_FAIL;
    }

    if(align){
        if(*size < 4)
            *align = *size;
        else
            *align = 4;
    }

    return S_OK;
}

/**********************************************************************
 *
 *  Functions for reading MSFT typelibs (those created by CreateTypeLib2)
 */

static inline void MSFT_Seek(TLBContext *pcx, LONG where)
{
    if (where != DO_NOT_SEEK)
    {
        where += pcx->oStart;
        if (where > pcx->length)
        {
            /* FIXME */
            ERR("seek beyond end (%ld/%d)\n", where, pcx->length );
            TLB_abort();
        }
        pcx->pos = where;
    }
}

/* read function */
static DWORD MSFT_Read(void *buffer,  DWORD count, TLBContext *pcx, LONG where )
{
    TRACE_(typelib)("pos=0x%08x len %#lx, %u, %u, %#lx\n",
       pcx->pos, count, pcx->oStart, pcx->length, where);

    MSFT_Seek(pcx, where);
    if (pcx->pos + count > pcx->length) count = pcx->length - pcx->pos;
    memcpy( buffer, (char *)pcx->mapping + pcx->pos, count );
    pcx->pos += count;
    return count;
}

static DWORD MSFT_ReadLEDWords(void *buffer,  DWORD count, TLBContext *pcx,
                               LONG where )
{
  DWORD ret;

  ret = MSFT_Read(buffer, count, pcx, where);
  FromLEDWords(buffer, ret);

  return ret;
}

static DWORD MSFT_ReadLEWords(void *buffer,  DWORD count, TLBContext *pcx,
                              LONG where )
{
  DWORD ret;

  ret = MSFT_Read(buffer, count, pcx, where);
  FromLEWords(buffer, ret);

  return ret;
}

static HRESULT MSFT_ReadAllGuids(TLBContext *pcx)
{
    TLBGuid *guid;
    MSFT_GuidEntry entry;
    int offs = 0;

    MSFT_Seek(pcx, pcx->pTblDir->pGuidTab.offset);
    while (1) {
        if (offs >= pcx->pTblDir->pGuidTab.length)
            return S_OK;

        MSFT_ReadLEWords(&entry, sizeof(MSFT_GuidEntry), pcx, DO_NOT_SEEK);

        guid = malloc(sizeof(TLBGuid));

        guid->offset = offs;
        guid->guid = entry.guid;
        guid->hreftype = entry.hreftype;

        list_add_tail(&pcx->pLibInfo->guid_list, &guid->entry);

        offs += sizeof(MSFT_GuidEntry);
    }
}

static TLBGuid *MSFT_ReadGuid( int offset, TLBContext *pcx)
{
    TLBGuid *ret;

    LIST_FOR_EACH_ENTRY(ret, &pcx->pLibInfo->guid_list, TLBGuid, entry){
        if(ret->offset == offset){
            TRACE_(typelib)("%s\n", debugstr_guid(&ret->guid));
            return ret;
        }
    }

    return NULL;
}

static HREFTYPE MSFT_ReadHreftype( TLBContext *pcx, int offset )
{
    MSFT_NameIntro niName;

    if (offset < 0)
    {
        ERR_(typelib)("bad offset %d\n", offset);
        return -1;
    }

    MSFT_ReadLEDWords(&niName, sizeof(niName), pcx,
		      pcx->pTblDir->pNametab.offset+offset);

    return niName.hreftype;
}

static HRESULT MSFT_ReadAllNames(TLBContext *pcx)
{
    char *string;
    MSFT_NameIntro intro;
    INT16 len_piece;
    int offs = 0, lengthInChars;

    MSFT_Seek(pcx, pcx->pTblDir->pNametab.offset);
    while (1) {
        TLBString *tlbstr;

        if (offs >= pcx->pTblDir->pNametab.length)
            return S_OK;

        MSFT_ReadLEWords(&intro, sizeof(MSFT_NameIntro), pcx, DO_NOT_SEEK);
        intro.namelen &= 0xFF;
        len_piece = intro.namelen + sizeof(MSFT_NameIntro);
        if(len_piece % 4)
            len_piece = (len_piece + 4) & ~0x3;
        if(len_piece < 8)
            len_piece = 8;

        string = malloc(len_piece + 1);
        MSFT_Read(string, len_piece - sizeof(MSFT_NameIntro), pcx, DO_NOT_SEEK);
        string[intro.namelen] = '\0';

        lengthInChars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                            string, -1, NULL, 0);
        if (!lengthInChars) {
            free(string);
            return E_UNEXPECTED;
        }

        tlbstr = malloc(sizeof(TLBString));

        tlbstr->offset = offs;
        tlbstr->str = SysAllocStringByteLen(NULL, lengthInChars * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, string, -1, tlbstr->str, lengthInChars);

        free(string);

        list_add_tail(&pcx->pLibInfo->name_list, &tlbstr->entry);

        offs += len_piece;
    }
}

static TLBString *MSFT_ReadName( TLBContext *pcx, int offset)
{
    TLBString *tlbstr;

    LIST_FOR_EACH_ENTRY(tlbstr, &pcx->pLibInfo->name_list, TLBString, entry) {
        if (tlbstr->offset == offset) {
            TRACE_(typelib)("%s\n", debugstr_w(tlbstr->str));
            return tlbstr;
        }
    }

    return NULL;
}

static TLBString *MSFT_ReadString( TLBContext *pcx, int offset)
{
    TLBString *tlbstr;

    LIST_FOR_EACH_ENTRY(tlbstr, &pcx->pLibInfo->string_list, TLBString, entry) {
        if (tlbstr->offset == offset) {
            TRACE_(typelib)("%s\n", debugstr_w(tlbstr->str));
            return tlbstr;
        }
    }

    return NULL;
}

/*
 * read a value and fill a VARIANT structure
 */
static void MSFT_ReadValue( VARIANT * pVar, int offset, TLBContext *pcx )
{
    int size;

    TRACE_(typelib)("\n");

    if(offset <0) { /* data are packed in here */
        V_VT(pVar) = (offset & 0x7c000000 )>> 26;
        V_I4(pVar) = offset & 0x3ffffff;
        return;
    }
    MSFT_ReadLEWords(&(V_VT(pVar)), sizeof(VARTYPE), pcx,
                     pcx->pTblDir->pCustData.offset + offset );
    TRACE_(typelib)("Vartype = %x\n", V_VT(pVar));
    switch (V_VT(pVar)){
        case VT_EMPTY:  /* FIXME: is this right? */
        case VT_NULL:   /* FIXME: is this right? */
        case VT_I2  :   /* this should not happen */
        case VT_I4  :
        case VT_R4  :
        case VT_ERROR   :
        case VT_BOOL    :
        case VT_I1  :
        case VT_UI1 :
        case VT_UI2 :
        case VT_UI4 :
        case VT_INT :
        case VT_UINT    :
        case VT_VOID    : /* FIXME: is this right? */
        case VT_HRESULT :
            size=4; break;
        case VT_R8  :
        case VT_CY  :
        case VT_DATE    :
        case VT_I8  :
        case VT_UI8 :
        case VT_DECIMAL :  /* FIXME: is this right? */
        case VT_FILETIME :
            size=8;break;
            /* pointer types with known behaviour */
        case VT_BSTR    :{
            char * ptr;
            MSFT_ReadLEDWords(&size, sizeof(INT), pcx, DO_NOT_SEEK );
            if(size == -1){
                V_BSTR(pVar) = NULL;
            }else{
                int len;
                ptr = calloc(1, size);
                MSFT_Read(ptr, size, pcx, DO_NOT_SEEK);
                len = MultiByteToWideChar(CP_ACP, 0, ptr, size, NULL, 0 );
                V_BSTR(pVar)=SysAllocStringLen(NULL,len);
                MultiByteToWideChar(CP_ACP, 0, ptr, size, V_BSTR(pVar), len );
                free(ptr);
            }
	}
	size=-4; break;
    /* FIXME: this will not work AT ALL when the variant contains a pointer */
        case VT_DISPATCH :
        case VT_VARIANT :
        case VT_UNKNOWN :
        case VT_PTR :
        case VT_SAFEARRAY :
        case VT_CARRAY  :
        case VT_USERDEFINED :
        case VT_LPSTR   :
        case VT_LPWSTR  :
        case VT_BLOB    :
        case VT_STREAM  :
        case VT_STORAGE :
        case VT_STREAMED_OBJECT :
        case VT_STORED_OBJECT   :
        case VT_BLOB_OBJECT :
        case VT_CF  :
        case VT_CLSID   :
        default:
            size=0;
            FIXME("VARTYPE %d is not supported, setting pointer to NULL\n",
                V_VT(pVar));
    }

    if(size>0) /* (big|small) endian correct? */
        MSFT_Read(&(V_I2(pVar)), size, pcx, DO_NOT_SEEK );
    return;
}
/*
 * create a linked list with custom data
 */
static int MSFT_CustData( TLBContext *pcx, int offset, struct list *custdata_list)
{
    MSFT_CDGuid entry;
    TLBCustData* pNew;
    int count=0;

    TRACE_(typelib)("\n");

    if (pcx->pTblDir->pCDGuids.offset < 0) return 0;

    while(offset >=0){
        count++;
        pNew = calloc(1, sizeof(TLBCustData));
        MSFT_ReadLEDWords(&entry, sizeof(entry), pcx, pcx->pTblDir->pCDGuids.offset+offset);
        pNew->guid = MSFT_ReadGuid(entry.GuidOffset, pcx);
        MSFT_ReadValue(&(pNew->data), entry.DataOffset, pcx);
        list_add_head(custdata_list, &pNew->entry);
        offset = entry.next;
    }
    return count;
}

static void MSFT_GetTdesc(TLBContext *pcx, INT type, TYPEDESC *pTd)
{
    if(type <0)
        pTd->vt=type & VT_TYPEMASK;
    else
        *pTd=pcx->pLibInfo->pTypeDesc[type/(2*sizeof(INT))];

    TRACE_(typelib)("vt type = %X\n", pTd->vt);
}

static BOOL TLB_is_propgetput(INVOKEKIND invkind)
{
    return (invkind == INVOKE_PROPERTYGET ||
        invkind == INVOKE_PROPERTYPUT ||
        invkind == INVOKE_PROPERTYPUTREF);
}

static void
MSFT_DoFuncs(TLBContext*     pcx,
	    ITypeInfoImpl*  pTI,
            int             cFuncs,
            int             cVars,
            int             offset,
            TLBFuncDesc**   pptfd)
{
    /*
     * member information is stored in a data structure at offset
     * indicated by the memoffset field of the typeinfo structure
     * There are several distinctive parts.
     * The first part starts with a field that holds the total length
     * of this (first) part excluding this field. Then follow the records,
     * for each member there is one record.
     *
     * The first entry is always the length of the record (including this
     * length word).
     * The rest of the record depends on the type of the member. If there is
     * a field indicating the member type (function, variable, interface, etc)
     * I have not found it yet. At this time we depend on the information
     * in the type info and the usual order how things are stored.
     *
     * Second follows an array sized nrMEM*sizeof(INT) with a member id
     * for each member;
     *
     * Third is an equal sized array with file offsets to the name entry
     * of each member.
     *
     * The fourth and last (?) part is an array with offsets to the records
     * in the first part of this file segment.
     */

    int infolen, nameoffset, reclength, i;
    int recoffset = offset + sizeof(INT);

    char *recbuf = malloc(0xffff);
    MSFT_FuncRecord *pFuncRec = (MSFT_FuncRecord*)recbuf;
    TLBFuncDesc *ptfd_prev = NULL, *ptfd;

    TRACE_(typelib)("\n");

    MSFT_ReadLEDWords(&infolen, sizeof(INT), pcx, offset);

    *pptfd = TLBFuncDesc_Alloc(cFuncs);
    ptfd = *pptfd;
    for ( i = 0; i < cFuncs ; i++ )
    {
        int optional;

        /* name, eventually add to a hash table */
        MSFT_ReadLEDWords(&nameoffset, sizeof(INT), pcx,
                          offset + infolen + (cFuncs + cVars + i + 1) * sizeof(INT));

        /* read the function information record */
        MSFT_ReadLEDWords(&reclength, sizeof(pFuncRec->Info), pcx, recoffset);

        reclength &= 0xffff;

        MSFT_ReadLEDWords(&pFuncRec->DataType, reclength - FIELD_OFFSET(MSFT_FuncRecord, DataType), pcx, DO_NOT_SEEK);

        /* size without argument data */
        optional = reclength - pFuncRec->nrargs*sizeof(MSFT_ParameterInfo);
        if (pFuncRec->FKCCIC & 0x1000)
            optional -= pFuncRec->nrargs * sizeof(INT);

        if (optional > FIELD_OFFSET(MSFT_FuncRecord, HelpContext))
            ptfd->helpcontext = pFuncRec->HelpContext;

        if (optional > FIELD_OFFSET(MSFT_FuncRecord, oHelpString))
            ptfd->HelpString = MSFT_ReadString(pcx, pFuncRec->oHelpString);

        if (optional > FIELD_OFFSET(MSFT_FuncRecord, oEntry))
        {
            if (pFuncRec->FKCCIC & 0x2000 )
            {
                if (!IS_INTRESOURCE(pFuncRec->oEntry))
                    ERR("ordinal 0x%08x invalid, IS_INTRESOURCE is false\n", pFuncRec->oEntry);
                ptfd->Entry = (TLBString*)(DWORD_PTR)LOWORD(pFuncRec->oEntry);
            }
            else
                ptfd->Entry = MSFT_ReadString(pcx, pFuncRec->oEntry);
        }
        else
            ptfd->Entry = (TLBString*)-1;

        if (optional > FIELD_OFFSET(MSFT_FuncRecord, HelpStringContext))
            ptfd->HelpStringContext = pFuncRec->HelpStringContext;

        if (optional > FIELD_OFFSET(MSFT_FuncRecord, oCustData) && pFuncRec->FKCCIC & 0x80)
            MSFT_CustData(pcx, pFuncRec->oCustData, &ptfd->custdata_list);

        /* fill the FuncDesc Structure */
        MSFT_ReadLEDWords( & ptfd->funcdesc.memid, sizeof(INT), pcx,
                           offset + infolen + ( i + 1) * sizeof(INT));

        ptfd->funcdesc.funckind   =  (pFuncRec->FKCCIC)      & 0x7;
        ptfd->funcdesc.invkind    =  (pFuncRec->FKCCIC) >> 3 & 0xF;
        ptfd->funcdesc.callconv   =  (pFuncRec->FKCCIC) >> 8 & 0xF;
        ptfd->funcdesc.cParams    =   pFuncRec->nrargs  ;
        ptfd->funcdesc.cParamsOpt =   pFuncRec->nroargs ;
        if (ptfd->funcdesc.funckind == FUNC_DISPATCH)
            ptfd->funcdesc.oVft   =   0;
        else
            ptfd->funcdesc.oVft   =   (unsigned short)(pFuncRec->VtableOffset & ~1) * sizeof(void *) / pTI->pTypeLib->ptr_size;
        ptfd->funcdesc.wFuncFlags =   LOWORD(pFuncRec->Flags) ;

        /* nameoffset is sometimes -1 on the second half of a propget/propput
         * pair of functions */
        if ((nameoffset == -1) && (i > 0) &&
                TLB_is_propgetput(ptfd_prev->funcdesc.invkind) &&
                TLB_is_propgetput(ptfd->funcdesc.invkind))
            ptfd->Name = ptfd_prev->Name;
        else
            ptfd->Name = MSFT_ReadName(pcx, nameoffset);

        MSFT_GetTdesc(pcx,
		      pFuncRec->DataType,
		      &ptfd->funcdesc.elemdescFunc.tdesc);

        /* do the parameters/arguments */
        if(pFuncRec->nrargs)
        {
            int j = 0;
            MSFT_ParameterInfo paraminfo;

            ptfd->funcdesc.lprgelemdescParam =
                calloc(pFuncRec->nrargs, sizeof(ELEMDESC) + sizeof(PARAMDESCEX));

            ptfd->pParamDesc = TLBParDesc_Constructor(pFuncRec->nrargs);

            MSFT_ReadLEDWords(&paraminfo, sizeof(paraminfo), pcx,
                              recoffset + reclength - pFuncRec->nrargs * sizeof(MSFT_ParameterInfo));

            for ( j = 0 ; j < pFuncRec->nrargs ; j++ )
            {
                ELEMDESC *elemdesc = &ptfd->funcdesc.lprgelemdescParam[j];

                MSFT_GetTdesc(pcx,
			      paraminfo.DataType,
			      &elemdesc->tdesc);

                elemdesc->paramdesc.wParamFlags = paraminfo.Flags;

                /* name */
                if (paraminfo.oName != -1)
                    ptfd->pParamDesc[j].Name =
                        MSFT_ReadName( pcx, paraminfo.oName );
                TRACE_(typelib)("param[%d] = %s\n", j, debugstr_w(TLB_get_bstr(ptfd->pParamDesc[j].Name)));

                /* default value */
                if ( (elemdesc->paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT) &&
                     (pFuncRec->FKCCIC & 0x1000) )
                {
                    INT* pInt = (INT *)((char *)pFuncRec +
                                   reclength -
                                   (pFuncRec->nrargs * 4) * sizeof(INT) );

                    PARAMDESC* pParamDesc = &elemdesc->paramdesc;

                    pParamDesc->pparamdescex = (PARAMDESCEX*)(ptfd->funcdesc.lprgelemdescParam+pFuncRec->nrargs)+j;
                    pParamDesc->pparamdescex->cBytes = sizeof(PARAMDESCEX);

		    MSFT_ReadValue(&(pParamDesc->pparamdescex->varDefaultValue),
                        pInt[j], pcx);
                }
                else
                    elemdesc->paramdesc.pparamdescex = NULL;

                /* custom info */
                if (optional > (FIELD_OFFSET(MSFT_FuncRecord, oArgCustData) +
                                j*sizeof(pFuncRec->oArgCustData[0])) &&
                    pFuncRec->FKCCIC & 0x80 )
                {
                    MSFT_CustData(pcx,
				  pFuncRec->oArgCustData[j],
				  &ptfd->pParamDesc[j].custdata_list);
                }

                /* SEEK value = jump to offset,
                 * from there jump to the end of record,
                 * go back by (j-1) arguments
                 */
                MSFT_ReadLEDWords( &paraminfo ,
			   sizeof(MSFT_ParameterInfo), pcx,
			   recoffset + reclength - ((pFuncRec->nrargs - j - 1)
					       * sizeof(MSFT_ParameterInfo)));
            }
        }

        /* scode is not used: archaic win16 stuff FIXME: right? */
        ptfd->funcdesc.cScodes   = 0 ;
        ptfd->funcdesc.lprgscode = NULL ;

        ptfd_prev = ptfd;
        ++ptfd;
        recoffset += reclength;
    }
    free(recbuf);
}

static void MSFT_DoVars(TLBContext *pcx, ITypeInfoImpl *pTI, int cFuncs,
		       int cVars, int offset, TLBVarDesc ** pptvd)
{
    int infolen, nameoffset, reclength;
    char recbuf[256];
    MSFT_VarRecord *pVarRec = (MSFT_VarRecord*)recbuf;
    TLBVarDesc *ptvd;
    int i;
    int recoffset;

    TRACE_(typelib)("\n");

    ptvd = *pptvd = TLBVarDesc_Alloc(cVars);
    MSFT_ReadLEDWords(&infolen,sizeof(INT), pcx, offset);
    MSFT_ReadLEDWords(&recoffset,sizeof(INT), pcx, offset + infolen +
                      ((cFuncs+cVars)*2+cFuncs + 1)*sizeof(INT));
    recoffset += offset+sizeof(INT);
    for(i=0;i<cVars;i++, ++ptvd){
    /* name, eventually add to a hash table */
        MSFT_ReadLEDWords(&nameoffset, sizeof(INT), pcx,
                          offset + infolen + (2*cFuncs + cVars + i + 1) * sizeof(INT));
        ptvd->Name=MSFT_ReadName(pcx, nameoffset);
    /* read the variable information record */
        MSFT_ReadLEDWords(&reclength, sizeof(pVarRec->Info), pcx, recoffset);
        reclength &= 0xff;
        MSFT_ReadLEDWords(&pVarRec->DataType, reclength - FIELD_OFFSET(MSFT_VarRecord, DataType), pcx, DO_NOT_SEEK);

        /* optional data */
        if(reclength > FIELD_OFFSET(MSFT_VarRecord, HelpContext))
            ptvd->HelpContext = pVarRec->HelpContext;

        if(reclength > FIELD_OFFSET(MSFT_VarRecord, HelpString))
            ptvd->HelpString = MSFT_ReadString(pcx, pVarRec->HelpString);

        if (reclength > FIELD_OFFSET(MSFT_VarRecord, oCustData))
            MSFT_CustData(pcx, pVarRec->oCustData, &ptvd->custdata_list);

        if(reclength > FIELD_OFFSET(MSFT_VarRecord, HelpStringContext))
            ptvd->HelpStringContext = pVarRec->HelpStringContext;

    /* fill the VarDesc Structure */
        MSFT_ReadLEDWords(&ptvd->vardesc.memid, sizeof(INT), pcx,
                          offset + infolen + (cFuncs + i + 1) * sizeof(INT));
        ptvd->vardesc.varkind = pVarRec->VarKind;
        ptvd->vardesc.wVarFlags = pVarRec->Flags;
        MSFT_GetTdesc(pcx, pVarRec->DataType,
            &ptvd->vardesc.elemdescVar.tdesc);
/*   ptvd->vardesc.lpstrSchema; is reserved (SDK) FIXME?? */
        if(pVarRec->VarKind == VAR_CONST ){
            ptvd->vardesc.lpvarValue = calloc(1, sizeof(VARIANT));
            MSFT_ReadValue(ptvd->vardesc.lpvarValue,
                pVarRec->OffsValue, pcx);
        } else
            ptvd->vardesc.oInst=pVarRec->OffsValue;
        recoffset += reclength;
    }
}

/* process Implemented Interfaces of a com class */
static void MSFT_DoImplTypes(TLBContext *pcx, ITypeInfoImpl *pTI, int count,
			    int offset)
{
    int i;
    MSFT_RefRecord refrec;
    TLBImplType *pImpl;

    TRACE_(typelib)("\n");

    pTI->impltypes = TLBImplType_Alloc(count);
    pImpl = pTI->impltypes;
    for(i=0;i<count;i++){
        if(offset<0) break; /* paranoia */
        MSFT_ReadLEDWords(&refrec,sizeof(refrec),pcx,offset+pcx->pTblDir->pRefTab.offset);
        pImpl->hRef = refrec.reftype;
        pImpl->implflags=refrec.flags;
        MSFT_CustData(pcx, refrec.oCustData, &pImpl->custdata_list);
        offset=refrec.onext;
        ++pImpl;
    }
}

/* when a typelib is loaded in a different 32/64-bit mode, we need to resize pointers
 * and some structures, and fix the alignment */
static void TLB_fix_typeinfo_ptr_size(ITypeInfoImpl *info)
{
    if(info->typeattr.typekind == TKIND_ALIAS){
        switch(info->tdescAlias->vt){
        case VT_BSTR:
        case VT_DISPATCH:
        case VT_UNKNOWN:
        case VT_PTR:
        case VT_SAFEARRAY:
        case VT_LPSTR:
        case VT_LPWSTR:
            info->typeattr.cbSizeInstance = sizeof(void*);
            info->typeattr.cbAlignment = sizeof(void*);
            break;
        case VT_CARRAY:
        case VT_USERDEFINED:
            TLB_size_instance(info, is_win64 ? SYS_WIN64 : SYS_WIN32, info->tdescAlias,
                              &info->typeattr.cbSizeInstance, &info->typeattr.cbAlignment);
            break;
        case VT_VARIANT:
            info->typeattr.cbSizeInstance = sizeof(VARIANT);
            info->typeattr.cbAlignment = sizeof(void *);
            break;
        default:
            if(info->typeattr.cbSizeInstance < sizeof(void*))
                info->typeattr.cbAlignment = info->typeattr.cbSizeInstance;
            else
                info->typeattr.cbAlignment = sizeof(void*);
            break;
        }
    }else if(info->typeattr.typekind == TKIND_INTERFACE ||
            info->typeattr.typekind == TKIND_DISPATCH ||
            info->typeattr.typekind == TKIND_COCLASS){
        info->typeattr.cbSizeInstance = sizeof(void*);
        info->typeattr.cbAlignment = sizeof(void*);
    }
}

/*
 * process a typeinfo record
 */
static ITypeInfoImpl * MSFT_DoTypeInfo(
    TLBContext *pcx,
    int count,
    ITypeLibImpl * pLibInfo)
{
    MSFT_TypeInfoBase tiBase;
    ITypeInfoImpl *ptiRet;

    TRACE_(typelib)("count=%u\n", count);

    ptiRet = ITypeInfoImpl_Constructor();
    MSFT_ReadLEDWords(&tiBase, sizeof(tiBase) ,pcx ,
                      pcx->pTblDir->pTypeInfoTab.offset+count*sizeof(tiBase));

/* this is where we are coming from */
    ptiRet->pTypeLib = pLibInfo;
    ptiRet->index=count;

    ptiRet->guid = MSFT_ReadGuid(tiBase.posguid, pcx);
    ptiRet->typeattr.lcid = pLibInfo->set_lcid;   /* FIXME: correct? */
    ptiRet->typeattr.lpstrSchema = NULL;              /* reserved */
    ptiRet->typeattr.cbSizeInstance = tiBase.size;
    ptiRet->typeattr.typekind = tiBase.typekind & 0xF;
    ptiRet->typeattr.cFuncs = LOWORD(tiBase.cElement);
    ptiRet->typeattr.cVars = HIWORD(tiBase.cElement);
    ptiRet->typeattr.cbAlignment = (tiBase.typekind >> 11 )& 0x1F; /* there are more flags there */
    ptiRet->typeattr.wTypeFlags = tiBase.flags;
    ptiRet->typeattr.wMajorVerNum = LOWORD(tiBase.version);
    ptiRet->typeattr.wMinorVerNum = HIWORD(tiBase.version);
    ptiRet->typeattr.cImplTypes = tiBase.cImplTypes;
    ptiRet->typeattr.cbSizeVft = tiBase.cbSizeVft;
    if (ptiRet->typeattr.typekind == TKIND_ALIAS) {
        TYPEDESC tmp;
        MSFT_GetTdesc(pcx, tiBase.datatype1, &tmp);
        ptiRet->tdescAlias = malloc(TLB_SizeTypeDesc(&tmp, TRUE));
        TLB_CopyTypeDesc(NULL, &tmp, ptiRet->tdescAlias);
    }

/*  FIXME: */
/*    IDLDESC  idldescType; *//* never saw this one != zero  */

/* name, eventually add to a hash table */
    ptiRet->Name=MSFT_ReadName(pcx, tiBase.NameOffset);
    ptiRet->hreftype = MSFT_ReadHreftype(pcx, tiBase.NameOffset);
    TRACE_(typelib)("reading %s\n", debugstr_w(TLB_get_bstr(ptiRet->Name)));
    /* help info */
    ptiRet->DocString=MSFT_ReadString(pcx, tiBase.docstringoffs);
    ptiRet->dwHelpStringContext=tiBase.helpstringcontext;
    ptiRet->dwHelpContext=tiBase.helpcontext;

    if (ptiRet->typeattr.typekind == TKIND_MODULE)
        ptiRet->DllName = MSFT_ReadString(pcx, tiBase.datatype1);

/* note: InfoType's Help file and HelpStringDll come from the containing
 * library. Further HelpString and Docstring appear to be the same thing :(
 */
    /* functions */
    if(ptiRet->typeattr.cFuncs >0 )
        MSFT_DoFuncs(pcx, ptiRet, ptiRet->typeattr.cFuncs,
		    ptiRet->typeattr.cVars,
		    tiBase.memoffset, &ptiRet->funcdescs);
    /* variables */
    if(ptiRet->typeattr.cVars >0 )
        MSFT_DoVars(pcx, ptiRet, ptiRet->typeattr.cFuncs,
		   ptiRet->typeattr.cVars,
		   tiBase.memoffset, &ptiRet->vardescs);
    if(ptiRet->typeattr.cImplTypes >0 ) {
        switch(ptiRet->typeattr.typekind)
        {
        case TKIND_COCLASS:
            MSFT_DoImplTypes(pcx, ptiRet, ptiRet->typeattr.cImplTypes,
                tiBase.datatype1);
            break;
        case TKIND_DISPATCH:
            /* This is not -1 when the interface is a non-base dual interface or
               when a dispinterface wraps an interface, i.e., the idl 'dispinterface x {interface y;};'.
               Note however that GetRefTypeOfImplType(0) always returns a ref to IDispatch and
               not this interface.
            */

            if (tiBase.datatype1 != -1)
            {
                ptiRet->impltypes = TLBImplType_Alloc(1);
                ptiRet->impltypes[0].hRef = tiBase.datatype1;
            }
            break;
        default:
            ptiRet->impltypes = TLBImplType_Alloc(1);
            ptiRet->impltypes[0].hRef = tiBase.datatype1;
            break;
       }
    }
    MSFT_CustData(pcx, tiBase.oCustData, ptiRet->pcustdata_list);

    TRACE_(typelib)("%s guid: %s kind:%s\n",
       debugstr_w(TLB_get_bstr(ptiRet->Name)),
       debugstr_guid(TLB_get_guidref(ptiRet->guid)),
       typekind_desc[ptiRet->typeattr.typekind]);
    if (TRACE_ON(typelib))
      dump_TypeInfo(ptiRet);

    return ptiRet;
}

static HRESULT MSFT_ReadAllStrings(TLBContext *pcx)
{
    char *string;
    INT16 len_str, len_piece;
    int offs = 0, lengthInChars;

    MSFT_Seek(pcx, pcx->pTblDir->pStringtab.offset);
    while (1) {
        TLBString *tlbstr;

        if (offs >= pcx->pTblDir->pStringtab.length)
            return S_OK;

        MSFT_ReadLEWords(&len_str, sizeof(INT16), pcx, DO_NOT_SEEK);
        len_piece = len_str + sizeof(INT16);
        if(len_piece % 4)
            len_piece = (len_piece + 4) & ~0x3;
        if(len_piece < 8)
            len_piece = 8;

        string = malloc(len_piece + 1);
        MSFT_Read(string, len_piece - sizeof(INT16), pcx, DO_NOT_SEEK);
        string[len_str] = '\0';

        lengthInChars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                            string, -1, NULL, 0);
        if (!lengthInChars) {
            free(string);
            return E_UNEXPECTED;
        }

        tlbstr = malloc(sizeof(TLBString));

        tlbstr->offset = offs;
        tlbstr->str = SysAllocStringByteLen(NULL, lengthInChars * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, string, -1, tlbstr->str, lengthInChars);

        free(string);

        list_add_tail(&pcx->pLibInfo->string_list, &tlbstr->entry);

        offs += len_piece;
    }
}

static HRESULT MSFT_ReadAllRefs(TLBContext *pcx)
{
    TLBRefType *ref;
    int offs = 0;

    MSFT_Seek(pcx, pcx->pTblDir->pImpInfo.offset);
    while (offs < pcx->pTblDir->pImpInfo.length) {
        MSFT_ImpInfo impinfo;
        TLBImpLib *pImpLib;

        MSFT_ReadLEDWords(&impinfo, sizeof(impinfo), pcx, DO_NOT_SEEK);

        ref = calloc(1, sizeof(TLBRefType));
        list_add_tail(&pcx->pLibInfo->ref_list, &ref->entry);

        LIST_FOR_EACH_ENTRY(pImpLib, &pcx->pLibInfo->implib_list, TLBImpLib, entry)
            if(pImpLib->offset==impinfo.oImpFile)
                break;

        if(&pImpLib->entry != &pcx->pLibInfo->implib_list){
            ref->reference = offs;
            ref->pImpTLInfo = pImpLib;
            if(impinfo.flags & MSFT_IMPINFO_OFFSET_IS_GUID) {
                ref->guid = MSFT_ReadGuid(impinfo.oGuid, pcx);
                TRACE("importing by guid %s\n", debugstr_guid(TLB_get_guidref(ref->guid)));
                ref->index = TLB_REF_USE_GUID;
            } else
                ref->index = impinfo.oGuid;
        }else{
            ERR("Cannot find a reference\n");
            ref->reference = -1;
            ref->pImpTLInfo = TLB_REF_NOT_FOUND;
        }

        offs += sizeof(impinfo);
    }

    return S_OK;
}

/* Because type library parsing has some degree of overhead, and some apps repeatedly load the same
 * typelibs over and over, we cache them here. According to MSDN Microsoft have a similar scheme in
 * place. This will cause a deliberate memory leak, but generally losing RAM for cycles is an acceptable
 * tradeoff here.
 */
static struct list tlb_cache = LIST_INIT(tlb_cache);
static CRITICAL_SECTION cache_section;
static CRITICAL_SECTION_DEBUG cache_section_debug =
{
    0, 0, &cache_section,
    { &cache_section_debug.ProcessLocksList, &cache_section_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": typelib loader cache") }
};
static CRITICAL_SECTION cache_section = { &cache_section_debug, -1, 0, 0, 0, 0 };


typedef struct TLB_PEFile
{
    IUnknown IUnknown_iface;
    LONG refs;
    HMODULE dll;
    HRSRC typelib_resource;
    HGLOBAL typelib_global;
    LPVOID typelib_base;
} TLB_PEFile;

static inline TLB_PEFile *pefile_impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, TLB_PEFile, IUnknown_iface);
}

static HRESULT WINAPI TLB_PEFile_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI TLB_PEFile_AddRef(IUnknown *iface)
{
    TLB_PEFile *This = pefile_impl_from_IUnknown(iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI TLB_PEFile_Release(IUnknown *iface)
{
    TLB_PEFile *This = pefile_impl_from_IUnknown(iface);
    ULONG refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        if (This->typelib_global)
            FreeResource(This->typelib_global);
        if (This->dll)
            FreeLibrary(This->dll);
        free(This);
    }
    return refs;
}

static const IUnknownVtbl TLB_PEFile_Vtable =
{
    TLB_PEFile_QueryInterface,
    TLB_PEFile_AddRef,
    TLB_PEFile_Release
};

static HRESULT TLB_PEFile_Open(LPCWSTR path, INT index, LPVOID *ppBase, DWORD *pdwTLBLength, IUnknown **ppFile)
{
    TLB_PEFile *This;
    HRESULT hr = TYPE_E_CANTLOADLIBRARY;

    This = malloc(sizeof(TLB_PEFile));
    if (!This)
        return E_OUTOFMEMORY;

    This->IUnknown_iface.lpVtbl = &TLB_PEFile_Vtable;
    This->refs = 1;
    This->dll = NULL;
    This->typelib_resource = NULL;
    This->typelib_global = NULL;
    This->typelib_base = NULL;

    This->dll = LoadLibraryExW(path, 0, DONT_RESOLVE_DLL_REFERENCES |
                    LOAD_LIBRARY_AS_DATAFILE | LOAD_WITH_ALTERED_SEARCH_PATH);

    if (This->dll)
    {
        This->typelib_resource = FindResourceW(This->dll, MAKEINTRESOURCEW(index), L"TYPELIB");
        if (This->typelib_resource)
        {
            This->typelib_global = LoadResource(This->dll, This->typelib_resource);
            if (This->typelib_global)
            {
                This->typelib_base = LockResource(This->typelib_global);

                if (This->typelib_base)
                {
                    *pdwTLBLength = SizeofResource(This->dll, This->typelib_resource);
                    *ppBase = This->typelib_base;
                    *ppFile = &This->IUnknown_iface;
                    return S_OK;
                }
            }
        }

        TRACE("No TYPELIB resource found\n");
        hr = E_FAIL;
    }

    TLB_PEFile_Release(&This->IUnknown_iface);
    return hr;
}

typedef struct TLB_NEFile
{
    IUnknown IUnknown_iface;
    LONG refs;
    LPVOID typelib_base;
} TLB_NEFile;

static inline TLB_NEFile *nefile_impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, TLB_NEFile, IUnknown_iface);
}

static HRESULT WINAPI TLB_NEFile_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI TLB_NEFile_AddRef(IUnknown *iface)
{
    TLB_NEFile *This = nefile_impl_from_IUnknown(iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI TLB_NEFile_Release(IUnknown *iface)
{
    TLB_NEFile *This = nefile_impl_from_IUnknown(iface);
    ULONG refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        free(This->typelib_base);
        free(This);
    }
    return refs;
}

static const IUnknownVtbl TLB_NEFile_Vtable =
{
    TLB_NEFile_QueryInterface,
    TLB_NEFile_AddRef,
    TLB_NEFile_Release
};

/***********************************************************************
 *           read_xx_header         [internal]
 */
static int read_xx_header( HFILE lzfd )
{
    IMAGE_DOS_HEADER mzh;
    char magic[3];

    LZSeek( lzfd, 0, SEEK_SET );
    if ( sizeof(mzh) != LZRead( lzfd, (LPSTR)&mzh, sizeof(mzh) ) )
        return 0;
    if ( mzh.e_magic != IMAGE_DOS_SIGNATURE )
        return 0;

    LZSeek( lzfd, mzh.e_lfanew, SEEK_SET );
    if ( 2 != LZRead( lzfd, magic, 2 ) )
        return 0;

    LZSeek( lzfd, mzh.e_lfanew, SEEK_SET );

    if ( magic[0] == 'N' && magic[1] == 'E' )
        return IMAGE_OS2_SIGNATURE;
    if ( magic[0] == 'P' && magic[1] == 'E' )
        return IMAGE_NT_SIGNATURE;

    magic[2] = '\0';
    WARN("Can't handle %s files.\n", magic );
    return 0;
}


/***********************************************************************
 *           find_ne_resource         [internal]
 */
static BOOL find_ne_resource( HFILE lzfd, LPCSTR typeid, LPCSTR resid,
                                DWORD *resLen, DWORD *resOff )
{
    IMAGE_OS2_HEADER nehd;
    NE_TYPEINFO *typeInfo;
    NE_NAMEINFO *nameInfo;
    DWORD nehdoffset;
    LPBYTE resTab;
    DWORD resTabSize;
    int count;

    /* Read in NE header */
    nehdoffset = LZSeek( lzfd, 0, SEEK_CUR );
    if ( sizeof(nehd) != LZRead( lzfd, (LPSTR)&nehd, sizeof(nehd) ) ) return FALSE;

    resTabSize = nehd.ne_restab - nehd.ne_rsrctab;
    if ( !resTabSize )
    {
        TRACE("No resources in NE dll\n" );
        return FALSE;
    }

    /* Read in resource table */
    resTab = malloc( resTabSize );
    if ( !resTab ) return FALSE;

    LZSeek( lzfd, nehd.ne_rsrctab + nehdoffset, SEEK_SET );
    if ( resTabSize != LZRead( lzfd, (char*)resTab, resTabSize ) )
    {
        free( resTab );
        return FALSE;
    }

    /* Find resource */
    typeInfo = (NE_TYPEINFO *)(resTab + 2);

    if (!IS_INTRESOURCE(typeid))  /* named type */
    {
        BYTE len = strlen( typeid );
        while (typeInfo->type_id)
        {
            if (!(typeInfo->type_id & 0x8000))
            {
                BYTE *p = resTab + typeInfo->type_id;
                if ((*p == len) && !_strnicmp( (char*)p+1, typeid, len )) goto found_type;
            }
            typeInfo = (NE_TYPEINFO *)((char *)(typeInfo + 1) +
                                       typeInfo->count * sizeof(NE_NAMEINFO));
        }
    }
    else  /* numeric type id */
    {
        WORD id = LOWORD(typeid) | 0x8000;
        while (typeInfo->type_id)
        {
            if (typeInfo->type_id == id) goto found_type;
            typeInfo = (NE_TYPEINFO *)((char *)(typeInfo + 1) +
                                       typeInfo->count * sizeof(NE_NAMEINFO));
        }
    }
    TRACE("No typeid entry found for %p\n", typeid );
    free( resTab );
    return FALSE;

 found_type:
    nameInfo = (NE_NAMEINFO *)(typeInfo + 1);

    if (!IS_INTRESOURCE(resid))  /* named resource */
    {
        BYTE len = strlen( resid );
        for (count = typeInfo->count; count > 0; count--, nameInfo++)
        {
            BYTE *p = resTab + nameInfo->id;
            if (nameInfo->id & 0x8000) continue;
            if ((*p == len) && !_strnicmp( (char*)p+1, resid, len )) goto found_name;
        }
    }
    else  /* numeric resource id */
    {
        WORD id = LOWORD(resid) | 0x8000;
        for (count = typeInfo->count; count > 0; count--, nameInfo++)
            if (nameInfo->id == id) goto found_name;
    }
    TRACE("No resid entry found for %p\n", typeid );
    free( resTab );
    return FALSE;

 found_name:
    /* Return resource data */
    if ( resLen ) *resLen = nameInfo->length << *(WORD *)resTab;
    if ( resOff ) *resOff = nameInfo->offset << *(WORD *)resTab;

    free( resTab );
    return TRUE;
}

static HRESULT TLB_NEFile_Open(LPCWSTR path, INT index, LPVOID *ppBase, DWORD *pdwTLBLength, IUnknown **ppFile){

    HFILE lzfd = -1;
    OFSTRUCT ofs;
    HRESULT hr = TYPE_E_CANTLOADLIBRARY;
    TLB_NEFile *This;

    This = malloc(sizeof(TLB_NEFile));
    if (!This) return E_OUTOFMEMORY;

    This->IUnknown_iface.lpVtbl = &TLB_NEFile_Vtable;
    This->refs = 1;
    This->typelib_base = NULL;

    lzfd = LZOpenFileW( (LPWSTR)path, &ofs, OF_READ );
    if ( lzfd >= 0 && read_xx_header( lzfd ) == IMAGE_OS2_SIGNATURE )
    {
        DWORD reslen, offset;
        if( find_ne_resource( lzfd, "TYPELIB", MAKEINTRESOURCEA(index), &reslen, &offset ) )
        {
            This->typelib_base = malloc(reslen);
            if( !This->typelib_base )
                hr = E_OUTOFMEMORY;
            else
            {
                LZSeek( lzfd, offset, SEEK_SET );
                reslen = LZRead( lzfd, This->typelib_base, reslen );
                LZClose( lzfd );
                *ppBase = This->typelib_base;
                *pdwTLBLength = reslen;
                *ppFile = &This->IUnknown_iface;
                return S_OK;
            }
        }
    }

    if( lzfd >= 0) LZClose( lzfd );
    TLB_NEFile_Release(&This->IUnknown_iface);
    return hr;
}

typedef struct TLB_Mapping
{
    IUnknown IUnknown_iface;
    LONG refs;
    HANDLE file;
    HANDLE mapping;
    LPVOID typelib_base;
} TLB_Mapping;

static inline TLB_Mapping *mapping_impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, TLB_Mapping, IUnknown_iface);
}

static HRESULT WINAPI TLB_Mapping_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI TLB_Mapping_AddRef(IUnknown *iface)
{
    TLB_Mapping *This = mapping_impl_from_IUnknown(iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI TLB_Mapping_Release(IUnknown *iface)
{
    TLB_Mapping *This = mapping_impl_from_IUnknown(iface);
    ULONG refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        if (This->typelib_base)
            UnmapViewOfFile(This->typelib_base);
        if (This->mapping)
            CloseHandle(This->mapping);
        if (This->file != INVALID_HANDLE_VALUE)
            CloseHandle(This->file);
        free(This);
    }
    return refs;
}

static const IUnknownVtbl TLB_Mapping_Vtable =
{
    TLB_Mapping_QueryInterface,
    TLB_Mapping_AddRef,
    TLB_Mapping_Release
};

static HRESULT TLB_Mapping_Open(LPCWSTR path, LPVOID *ppBase, DWORD *pdwTLBLength, IUnknown **ppFile)
{
    TLB_Mapping *This;

    This = malloc(sizeof(TLB_Mapping));
    if (!This)
        return E_OUTOFMEMORY;

    This->IUnknown_iface.lpVtbl = &TLB_Mapping_Vtable;
    This->refs = 1;
    This->file = INVALID_HANDLE_VALUE;
    This->mapping = NULL;
    This->typelib_base = NULL;

    This->file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    if (INVALID_HANDLE_VALUE != This->file)
    {
        This->mapping = CreateFileMappingW(This->file, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
        if (This->mapping)
        {
            This->typelib_base = MapViewOfFile(This->mapping, FILE_MAP_READ, 0, 0, 0);
            if(This->typelib_base)
            {
                /* retrieve file size */
                *pdwTLBLength = GetFileSize(This->file, NULL);
                *ppBase = This->typelib_base;
                *ppFile = &This->IUnknown_iface;
                return S_OK;
            }
        }
    }

    IUnknown_Release(&This->IUnknown_iface);
    return TYPE_E_CANTLOADLIBRARY;
}

/****************************************************************************
 *	TLB_ReadTypeLib
 *
 * find the type of the typelib file and map the typelib resource into
 * the memory
 */

#define SLTG_SIGNATURE 0x47544c53 /* "SLTG" */
static HRESULT TLB_ReadTypeLib(LPCWSTR pszFileName, LPWSTR pszPath, UINT cchPath, ITypeLib2 **ppTypeLib)
{
    ITypeLibImpl *entry;
    HRESULT ret;
    INT index = 1;
    LPWSTR index_str, file = (LPWSTR)pszFileName;
    LPVOID pBase = NULL;
    DWORD dwTLBLength = 0;
    IUnknown *pFile = NULL;
    HANDLE h;

    *ppTypeLib = NULL;

    index_str = wcsrchr(pszFileName, '\\');
    if(index_str && *++index_str != '\0')
    {
        LPWSTR end_ptr;
        LONG idx = wcstol(index_str, &end_ptr, 10);
        if(*end_ptr == '\0')
        {
            int str_len = index_str - pszFileName - 1;
            index = idx;
            file = malloc((str_len + 1) * sizeof(WCHAR));
            memcpy(file, pszFileName, str_len * sizeof(WCHAR));
            file[str_len] = 0;
        }
    }

    if(!SearchPathW(NULL, file, NULL, cchPath, pszPath, NULL))
    {
        if(wcschr(file, '\\'))
        {
            lstrcpyW(pszPath, file);
        }
        else
        {
            int len = GetSystemDirectoryW(pszPath, cchPath);
            pszPath[len] = '\\';
            memcpy(pszPath + len + 1, file, (lstrlenW(file) + 1) * sizeof(WCHAR));
        }
    }

    if(file != pszFileName) free(file);

    h = CreateFileW(pszPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(h != INVALID_HANDLE_VALUE){
        GetFinalPathNameByHandleW(h, pszPath, cchPath, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        CloseHandle(h);
    }

    TRACE_(typelib)("File %s index %d\n", debugstr_w(pszPath), index);

    /* We look the path up in the typelib cache. If found, we just addref it, and return the pointer. */
    EnterCriticalSection(&cache_section);
    LIST_FOR_EACH_ENTRY(entry, &tlb_cache, ITypeLibImpl, entry)
    {
        if (!wcsicmp(entry->path, pszPath) && entry->index == index)
        {
            TRACE("cache hit\n");
            *ppTypeLib = &entry->ITypeLib2_iface;
            ITypeLib2_AddRef(*ppTypeLib);
            LeaveCriticalSection(&cache_section);
            return S_OK;
        }
    }
    LeaveCriticalSection(&cache_section);

    /* now actually load and parse the typelib */

    ret = TLB_PEFile_Open(pszPath, index, &pBase, &dwTLBLength, &pFile);
    if (ret == TYPE_E_CANTLOADLIBRARY)
        ret = TLB_NEFile_Open(pszPath, index, &pBase, &dwTLBLength, &pFile);
    if (ret == TYPE_E_CANTLOADLIBRARY)
        ret = TLB_Mapping_Open(pszPath, &pBase, &dwTLBLength, &pFile);
    if (SUCCEEDED(ret))
    {
        if (dwTLBLength >= 4)
        {
            DWORD dwSignature = FromLEDWord(*((DWORD*) pBase));
            if (dwSignature == MSFT_SIGNATURE)
                *ppTypeLib = ITypeLib2_Constructor_MSFT(pBase, dwTLBLength);
            else if (dwSignature == SLTG_SIGNATURE)
                *ppTypeLib = ITypeLib2_Constructor_SLTG(pBase, dwTLBLength);
            else
            {
                FIXME("Header type magic %#lx not supported.\n", dwSignature);
                ret = TYPE_E_CANTLOADLIBRARY;
            }
        }
        else
            ret = TYPE_E_CANTLOADLIBRARY;
        IUnknown_Release(pFile);
    }

    if(*ppTypeLib) {
	ITypeLibImpl *impl = impl_from_ITypeLib2(*ppTypeLib);

	TRACE("adding to cache\n");
	impl->path = wcsdup(pszPath);
	/* We should really canonicalise the path here. */
        impl->index = index;

        /* FIXME: check if it has added already in the meantime */
        EnterCriticalSection(&cache_section);
        list_add_head(&tlb_cache, &impl->entry);
        LeaveCriticalSection(&cache_section);
        ret = S_OK;
    }
    else
    {
        if(ret != E_FAIL)
            ERR("Loading of typelib %s failed with error %ld\n", debugstr_w(pszFileName), GetLastError());

        ret = TYPE_E_CANTLOADLIBRARY;
    }


    return ret;
}

/*================== ITypeLib(2) Methods ===================================*/

static ITypeLibImpl* TypeLibImpl_Constructor(void)
{
    ITypeLibImpl* pTypeLibImpl;

    pTypeLibImpl = calloc(1, sizeof(ITypeLibImpl));
    if (!pTypeLibImpl) return NULL;

    pTypeLibImpl->ITypeLib2_iface.lpVtbl = &tlbvt;
    pTypeLibImpl->ITypeComp_iface.lpVtbl = &tlbtcvt;
    pTypeLibImpl->ICreateTypeLib2_iface.lpVtbl = &CreateTypeLib2Vtbl;
    pTypeLibImpl->ref = 1;

    list_init(&pTypeLibImpl->implib_list);
    list_init(&pTypeLibImpl->custdata_list);
    list_init(&pTypeLibImpl->name_list);
    list_init(&pTypeLibImpl->string_list);
    list_init(&pTypeLibImpl->guid_list);
    list_init(&pTypeLibImpl->ref_list);
    pTypeLibImpl->dispatch_href = -1;

    return pTypeLibImpl;
}

/****************************************************************************
 *	ITypeLib2_Constructor_MSFT
 *
 * loading an MSFT typelib from an in-memory image
 */
static ITypeLib2* ITypeLib2_Constructor_MSFT(LPVOID pLib, DWORD dwTLBLength)
{
    TLBContext cx;
    LONG lPSegDir;
    MSFT_Header tlbHeader;
    MSFT_SegDir tlbSegDir;
    ITypeLibImpl * pTypeLibImpl;
    int i;

    TRACE("%p, TLB length = %ld\n", pLib, dwTLBLength);

    pTypeLibImpl = TypeLibImpl_Constructor();
    if (!pTypeLibImpl) return NULL;

    /* get pointer to beginning of typelib data */
    cx.pos = 0;
    cx.oStart=0;
    cx.mapping = pLib;
    cx.pLibInfo = pTypeLibImpl;
    cx.length = dwTLBLength;

    /* read header */
    MSFT_ReadLEDWords(&tlbHeader, sizeof(tlbHeader), &cx, 0);
    TRACE_(typelib)("header:\n");
    TRACE_(typelib)("\tmagic1=0x%08x ,magic2=0x%08x\n",tlbHeader.magic1,tlbHeader.magic2 );
    if (tlbHeader.magic1 != MSFT_SIGNATURE) {
	FIXME("Header type magic 0x%08x not supported.\n",tlbHeader.magic1);
	return NULL;
    }
    TRACE_(typelib)("\tdispatchpos = 0x%x\n", tlbHeader.dispatchpos);

    /* there is a small amount of information here until the next important
     * part:
     * the segment directory . Try to calculate the amount of data */
    lPSegDir = sizeof(tlbHeader) + (tlbHeader.nrtypeinfos)*4 + ((tlbHeader.varflags & HELPDLLFLAG)? 4 :0);

    /* now read the segment directory */
    TRACE("read segment directory (at %ld)\n",lPSegDir);
    MSFT_ReadLEDWords(&tlbSegDir, sizeof(tlbSegDir), &cx, lPSegDir);
    cx.pTblDir = &tlbSegDir;

    /* just check two entries */
    if ( tlbSegDir.pTypeInfoTab.res0c != 0x0F || tlbSegDir.pImpInfo.res0c != 0x0F)
    {
        ERR("cannot find the table directory, ptr %#lx\n",lPSegDir);
	free(pTypeLibImpl);
	return NULL;
    }

    MSFT_ReadAllNames(&cx);
    MSFT_ReadAllStrings(&cx);
    MSFT_ReadAllGuids(&cx);

    /* now fill our internal data */
    /* TLIBATTR fields */
    pTypeLibImpl->guid = MSFT_ReadGuid(tlbHeader.posguid, &cx);

    pTypeLibImpl->syskind = tlbHeader.varflags & 0x0f; /* check the mask */
    pTypeLibImpl->ptr_size = get_ptr_size(pTypeLibImpl->syskind);
    pTypeLibImpl->ver_major = LOWORD(tlbHeader.version);
    pTypeLibImpl->ver_minor = HIWORD(tlbHeader.version);
    pTypeLibImpl->libflags = ((WORD) tlbHeader.flags & 0xffff) /* check mask */ | LIBFLAG_FHASDISKIMAGE;

    pTypeLibImpl->set_lcid = tlbHeader.lcid2;
    pTypeLibImpl->lcid = tlbHeader.lcid;

    /* name, eventually add to a hash table */
    pTypeLibImpl->Name = MSFT_ReadName(&cx, tlbHeader.NameOffset);

    /* help info */
    pTypeLibImpl->DocString = MSFT_ReadString(&cx, tlbHeader.helpstring);
    pTypeLibImpl->HelpFile = MSFT_ReadString(&cx, tlbHeader.helpfile);

    if( tlbHeader.varflags & HELPDLLFLAG)
    {
            int offset;
            MSFT_ReadLEDWords(&offset, sizeof(offset), &cx, sizeof(tlbHeader));
            pTypeLibImpl->HelpStringDll = MSFT_ReadString(&cx, offset);
    }

    pTypeLibImpl->dwHelpContext = tlbHeader.helpstringcontext;

    /* custom data */
    if(tlbHeader.CustomDataOffset >= 0)
    {
        MSFT_CustData(&cx, tlbHeader.CustomDataOffset, &pTypeLibImpl->custdata_list);
    }

    /* fill in type descriptions */
    if(tlbSegDir.pTypdescTab.length > 0)
    {
        int i, j, cTD = tlbSegDir.pTypdescTab.length / (2*sizeof(INT));
        INT16 td[4];
        pTypeLibImpl->ctTypeDesc = cTD;
        pTypeLibImpl->pTypeDesc = calloc(cTD, sizeof(TYPEDESC));
        MSFT_ReadLEWords(td, sizeof(td), &cx, tlbSegDir.pTypdescTab.offset);
        for(i=0; i<cTD; )
	{
            /* FIXME: add several sanity checks here */
            pTypeLibImpl->pTypeDesc[i].vt = td[0] & VT_TYPEMASK;
            if(td[0] == VT_PTR || td[0] == VT_SAFEARRAY)
	    {
	        /* FIXME: check safearray */
                if(td[3] < 0)
                    pTypeLibImpl->pTypeDesc[i].lptdesc = &std_typedesc[td[2]];
                else
                    pTypeLibImpl->pTypeDesc[i].lptdesc = &pTypeLibImpl->pTypeDesc[td[2]/8];
            }
	    else if(td[0] == VT_CARRAY)
            {
	        /* array descr table here */
	        pTypeLibImpl->pTypeDesc[i].lpadesc = (void *)(INT_PTR)td[2];  /* temp store offset in*/
            }
            else if(td[0] == VT_USERDEFINED)
	    {
                pTypeLibImpl->pTypeDesc[i].hreftype = MAKELONG(td[2],td[3]);
            }
	    if(++i<cTD) MSFT_ReadLEWords(td, sizeof(td), &cx, DO_NOT_SEEK);
        }

        /* second time around to fill the array subscript info */
        for(i=0;i<cTD;i++)
	{
            if(pTypeLibImpl->pTypeDesc[i].vt != VT_CARRAY) continue;
            if(tlbSegDir.pArrayDescriptions.offset>0)
	    {
                MSFT_ReadLEWords(td, sizeof(td), &cx, tlbSegDir.pArrayDescriptions.offset + (INT_PTR)pTypeLibImpl->pTypeDesc[i].lpadesc);
                pTypeLibImpl->pTypeDesc[i].lpadesc = calloc(1, sizeof(ARRAYDESC) + sizeof(SAFEARRAYBOUND) * (td[3] - 1));

                if(td[1]<0)
                    pTypeLibImpl->pTypeDesc[i].lpadesc->tdescElem.vt = td[0] & VT_TYPEMASK;
                else
                    pTypeLibImpl->pTypeDesc[i].lpadesc->tdescElem = cx.pLibInfo->pTypeDesc[td[0]/(2*sizeof(INT))];

                pTypeLibImpl->pTypeDesc[i].lpadesc->cDims = td[2];

                for(j = 0; j<td[2]; j++)
		{
                    MSFT_ReadLEDWords(& pTypeLibImpl->pTypeDesc[i].lpadesc->rgbounds[j].cElements,
                                      sizeof(INT), &cx, DO_NOT_SEEK);
                    MSFT_ReadLEDWords(& pTypeLibImpl->pTypeDesc[i].lpadesc->rgbounds[j].lLbound,
                                      sizeof(INT), &cx, DO_NOT_SEEK);
                }
            }
	    else
	    {
                pTypeLibImpl->pTypeDesc[i].lpadesc = NULL;
                ERR("didn't find array description data\n");
            }
        }
    }

    /* imported type libs */
    if(tlbSegDir.pImpFiles.offset>0)
    {
        TLBImpLib *pImpLib;
        int oGuid, offset = tlbSegDir.pImpFiles.offset;
        UINT16 size;

        while(offset < tlbSegDir.pImpFiles.offset +tlbSegDir.pImpFiles.length)
	{
            char *name;

            pImpLib = calloc(1, sizeof(TLBImpLib));
            pImpLib->offset = offset - tlbSegDir.pImpFiles.offset;
            MSFT_ReadLEDWords(&oGuid, sizeof(INT), &cx, offset);

            MSFT_ReadLEDWords(&pImpLib->lcid,         sizeof(LCID),   &cx, DO_NOT_SEEK);
            MSFT_ReadLEWords(&pImpLib->wVersionMajor, sizeof(WORD),   &cx, DO_NOT_SEEK);
            MSFT_ReadLEWords(&pImpLib->wVersionMinor, sizeof(WORD),   &cx, DO_NOT_SEEK);
            MSFT_ReadLEWords(& size,                      sizeof(UINT16), &cx, DO_NOT_SEEK);

            size >>= 2;
            name = calloc(1, size + 1);
            MSFT_Read(name, size, &cx, DO_NOT_SEEK);
            pImpLib->name = TLB_MultiByteToBSTR(name);
            free(name);

            pImpLib->guid = MSFT_ReadGuid(oGuid, &cx);
            offset = (offset + sizeof(INT) + sizeof(DWORD) + sizeof(LCID) + sizeof(UINT16) + size + 3) & ~3;

            list_add_tail(&pTypeLibImpl->implib_list, &pImpLib->entry);
        }
    }

    MSFT_ReadAllRefs(&cx);

    pTypeLibImpl->dispatch_href = tlbHeader.dispatchpos;

    /* type infos */
    if(tlbHeader.nrtypeinfos >= 0 )
    {
        ITypeInfoImpl **ppTI;

        ppTI = pTypeLibImpl->typeinfos = calloc(tlbHeader.nrtypeinfos, sizeof(ITypeInfoImpl*));

        for(i = 0; i < tlbHeader.nrtypeinfos; i++)
        {
            *ppTI = MSFT_DoTypeInfo(&cx, i, pTypeLibImpl);

            ++ppTI;
            (pTypeLibImpl->TypeInfoCount)++;
        }
    }

    if (pTypeLibImpl->ptr_size != sizeof(void *))
    {
        for(i = 0; i < pTypeLibImpl->TypeInfoCount; ++i)
            TLB_fix_typeinfo_ptr_size(pTypeLibImpl->typeinfos[i]);
    }

    TRACE("(%p)\n", pTypeLibImpl);
    return &pTypeLibImpl->ITypeLib2_iface;
}


static BOOL TLB_GUIDFromString(const char *str, GUID *guid)
{
  char b[3];
  int i;
  short s;

  if(sscanf(str, "%lx-%hx-%hx-%hx", &guid->Data1, &guid->Data2, &guid->Data3, &s) != 4) {
    FIXME("Can't parse guid %s\n", debugstr_guid(guid));
    return FALSE;
  }

  guid->Data4[0] = s >> 8;
  guid->Data4[1] = s & 0xff;

  b[2] = '\0';
  for(i = 0; i < 6; i++) {
    memcpy(b, str + 24 + 2 * i, 2);
    guid->Data4[i + 2] = strtol(b, NULL, 16);
  }
  return TRUE;
}

static WORD SLTG_ReadString(const char *ptr, const TLBString **pStr, ITypeLibImpl *lib)
{
    WORD bytelen;
    DWORD len;
    BSTR tmp_str;

    *pStr = NULL;
    bytelen = *(const WORD*)ptr;
    if(bytelen == 0xffff) return 2;

    len = MultiByteToWideChar(CP_ACP, 0, ptr + 2, bytelen, NULL, 0);
    tmp_str = SysAllocStringLen(NULL, len);
    if (tmp_str) {
        MultiByteToWideChar(CP_ACP, 0, ptr + 2, bytelen, tmp_str, len);
        *pStr = TLB_append_str(&lib->string_list, tmp_str);
        SysFreeString(tmp_str);
    }
    return bytelen + 2;
}

static WORD SLTG_ReadStringA(const char *ptr, char **str)
{
    WORD bytelen;

    *str = NULL;
    bytelen = *(const WORD*)ptr;
    if(bytelen == 0xffff) return 2;
    *str = malloc(bytelen + 1);
    memcpy(*str, ptr + 2, bytelen);
    (*str)[bytelen] = '\0';
    return bytelen + 2;
}

static TLBString *SLTG_ReadName(const char *pNameTable, int offset, ITypeLibImpl *lib)
{
    BSTR tmp_str;
    TLBString *tlbstr;

    LIST_FOR_EACH_ENTRY(tlbstr, &lib->name_list, TLBString, entry) {
        if (tlbstr->offset == offset)
            return tlbstr;
    }

    tmp_str = TLB_MultiByteToBSTR(pNameTable + offset);
    tlbstr = TLB_append_str(&lib->name_list, tmp_str);
    SysFreeString(tmp_str);

    return tlbstr;
}

static DWORD SLTG_ReadLibBlk(LPVOID pLibBlk, ITypeLibImpl *pTypeLibImpl)
{
    char *ptr = pLibBlk;
    WORD w;

    if((w = *(WORD*)ptr) != SLTG_LIBBLK_MAGIC) {
        FIXME("libblk magic = %04x\n", w);
	return 0;
    }

    ptr += 6;
    if((w = *(WORD*)ptr) != 0xffff) {
        FIXME("LibBlk.res06 = %04x. Assuming string and skipping\n", w);
        ptr += w;
    }
    ptr += 2;

    ptr += SLTG_ReadString(ptr, &pTypeLibImpl->DocString, pTypeLibImpl);

    ptr += SLTG_ReadString(ptr, &pTypeLibImpl->HelpFile, pTypeLibImpl);

    pTypeLibImpl->dwHelpContext = *(DWORD*)ptr;
    ptr += 4;

    pTypeLibImpl->syskind = *(WORD*)ptr;
    pTypeLibImpl->ptr_size = get_ptr_size(pTypeLibImpl->syskind);
    ptr += 2;

    if(SUBLANGID(*(WORD*)ptr) == SUBLANG_NEUTRAL)
        pTypeLibImpl->lcid = pTypeLibImpl->set_lcid = MAKELCID(MAKELANGID(PRIMARYLANGID(*(WORD*)ptr),0),0);
    else
        pTypeLibImpl->lcid = pTypeLibImpl->set_lcid = 0;
    ptr += 2;

    ptr += 4; /* skip res12 */

    pTypeLibImpl->libflags = *(WORD*)ptr;
    ptr += 2;

    pTypeLibImpl->ver_major = *(WORD*)ptr;
    ptr += 2;

    pTypeLibImpl->ver_minor = *(WORD*)ptr;
    ptr += 2;

    pTypeLibImpl->guid = TLB_append_guid(&pTypeLibImpl->guid_list, (GUID*)ptr, -2);
    ptr += sizeof(GUID);

    return ptr - (char*)pLibBlk;
}

/* stores a mapping between the sltg typeinfo's references and the typelib's HREFTYPEs */
typedef struct
{
    unsigned int num;
    HREFTYPE refs[1];
} sltg_ref_lookup_t;

static HRESULT sltg_get_typelib_ref(const sltg_ref_lookup_t *table, DWORD typeinfo_ref,
				    HREFTYPE *typelib_ref)
{
    if(table && typeinfo_ref < table->num)
    {
        *typelib_ref = table->refs[typeinfo_ref];
        return S_OK;
    }

    ERR_(typelib)("Unable to find reference\n");
    *typelib_ref = -1;
    return E_FAIL;
}

static WORD *SLTG_DoType(WORD *pType, char *pBlk, TYPEDESC *pTD, const sltg_ref_lookup_t *ref_lookup)
{
    BOOL done = FALSE;

    while(!done) {
        if((*pType & 0xe00) == 0xe00) {
	    pTD->vt = VT_PTR;
	    pTD->lptdesc = calloc(1, sizeof(TYPEDESC));
	    pTD = pTD->lptdesc;
	}
	switch(*pType & 0x3f) {
	case VT_PTR:
	    pTD->vt = VT_PTR;
	    pTD->lptdesc = calloc(1, sizeof(TYPEDESC));
	    pTD = pTD->lptdesc;
	    break;

	case VT_USERDEFINED:
	    pTD->vt = VT_USERDEFINED;
            sltg_get_typelib_ref(ref_lookup, *(++pType) / 4, &pTD->hreftype);
	    done = TRUE;
	    break;

	case VT_CARRAY:
	  {
	    /* *(pType+1) is offset to a SAFEARRAY, *(pType+2) is type of
	       array */

	    SAFEARRAY *pSA = (SAFEARRAY *)(pBlk + *(++pType));

	    pTD->vt = VT_CARRAY;
	    pTD->lpadesc = calloc(1, sizeof(ARRAYDESC) + (pSA->cDims - 1) * sizeof(SAFEARRAYBOUND));
	    pTD->lpadesc->cDims = pSA->cDims;
	    memcpy(pTD->lpadesc->rgbounds, pSA->rgsabound,
		   pSA->cDims * sizeof(SAFEARRAYBOUND));

	    pTD = &pTD->lpadesc->tdescElem;
	    break;
	  }

	case VT_SAFEARRAY:
	  {
	    /* FIXME: *(pType+1) gives an offset to SAFEARRAY, is this
	       useful? */

	    pType++;
	    pTD->vt = VT_SAFEARRAY;
	    pTD->lptdesc = calloc(1, sizeof(TYPEDESC));
	    pTD = pTD->lptdesc;
	    break;
	  }
	default:
	    pTD->vt = *pType & 0x3f;
	    done = TRUE;
	    break;
	}
	pType++;
    }
    return pType;
}

static WORD *SLTG_DoElem(WORD *pType, char *pBlk,
			 ELEMDESC *pElem, const sltg_ref_lookup_t *ref_lookup)
{
    /* Handle [in/out] first */
    if((*pType & 0xc000) == 0xc000)
        pElem->paramdesc.wParamFlags = PARAMFLAG_NONE;
    else if(*pType & 0x8000)
        pElem->paramdesc.wParamFlags = PARAMFLAG_FIN | PARAMFLAG_FOUT;
    else if(*pType & 0x4000)
        pElem->paramdesc.wParamFlags = PARAMFLAG_FOUT;
    else
        pElem->paramdesc.wParamFlags = PARAMFLAG_FIN;

    if(*pType & 0x2000)
        pElem->paramdesc.wParamFlags |= PARAMFLAG_FLCID;

    if(*pType & 0x80)
        pElem->paramdesc.wParamFlags |= PARAMFLAG_FRETVAL;

    return SLTG_DoType(pType, pBlk, &pElem->tdesc, ref_lookup);
}


static sltg_ref_lookup_t *SLTG_DoRefs(SLTG_RefInfo *pRef, ITypeLibImpl *pTL,
			char *pNameTable)
{
    unsigned int ref;
    char *name;
    TLBRefType *ref_type;
    sltg_ref_lookup_t *table;
    HREFTYPE typelib_ref;

    if(pRef->magic != SLTG_REF_MAGIC) {
        FIXME("Ref magic = %x\n", pRef->magic);
	return NULL;
    }
    name = ( (char*)pRef->names + pRef->number);

    table = malloc(sizeof(*table) + ((pRef->number >> 3) - 1) * sizeof(table->refs[0]));
    table->num = pRef->number >> 3;

    /* FIXME should scan the existing list and reuse matching refs added by previous typeinfos */

    /* We don't want the first href to be 0 */
    typelib_ref = (list_count(&pTL->ref_list) + 1) << 2;

    for(ref = 0; ref < pRef->number >> 3; ref++) {
        char *refname;
	unsigned int lib_offs, type_num;

	ref_type = calloc(1, sizeof(TLBRefType));

	name += SLTG_ReadStringA(name, &refname);
	if(sscanf(refname, "*\\R%x*#%x", &lib_offs, &type_num) != 2)
	    FIXME_(typelib)("Can't sscanf ref\n");
	if(lib_offs != 0xffff) {
	    TLBImpLib *import;

            LIST_FOR_EACH_ENTRY(import, &pTL->implib_list, TLBImpLib, entry)
                if(import->offset == lib_offs)
                    break;

            if(&import->entry == &pTL->implib_list) {
	        char fname[MAX_PATH+1];
		int len;
                GUID tmpguid;

		import = calloc(1, sizeof(*import));
		import->offset = lib_offs;
		TLB_GUIDFromString( pNameTable + lib_offs + 4, &tmpguid);
                import->guid = TLB_append_guid(&pTL->guid_list, &tmpguid, 2);
		if(sscanf(pNameTable + lib_offs + 40, "}#%hd.%hd#%lx#%s",
			  &import->wVersionMajor,
			  &import->wVersionMinor,
			  &import->lcid, fname) != 4) {
		  FIXME_(typelib)("can't sscanf ref %s\n",
			pNameTable + lib_offs + 40);
		}
		len = strlen(fname);
		if(fname[len-1] != '#')
		    FIXME("fname = %s\n", fname);
		fname[len-1] = '\0';
		import->name = TLB_MultiByteToBSTR(fname);
		list_add_tail(&pTL->implib_list, &import->entry);
	    }
	    ref_type->pImpTLInfo = import;

            /* Store a reference to IDispatch */
            if(pTL->dispatch_href == -1 && IsEqualGUID(&import->guid->guid, &IID_StdOle) && type_num == 4)
                pTL->dispatch_href = typelib_ref;

	} else { /* internal ref */
	  ref_type->pImpTLInfo = TLB_REF_INTERNAL;
	}
	ref_type->reference = typelib_ref;
	ref_type->index = type_num;

        free(refname);
        list_add_tail(&pTL->ref_list, &ref_type->entry);

        table->refs[ref] = typelib_ref;
        typelib_ref += 4;
    }
    if((BYTE)*name != SLTG_REF_MAGIC)
      FIXME_(typelib)("End of ref block magic = %x\n", *name);
    dump_TLBRefType(pTL);
    return table;
}

static char *SLTG_DoImpls(char *pBlk, ITypeInfoImpl *pTI,
			  BOOL OneOnly, const sltg_ref_lookup_t *ref_lookup)
{
    SLTG_ImplInfo *info;
    TLBImplType *pImplType;
    /* I don't really get this structure, usually it's 0x16 bytes
       long, but iuser.tlb contains some that are 0x18 bytes long.
       That's ok because we can use the next ptr to jump to the next
       one. But how do we know the length of the last one?  The WORD
       at offs 0x8 might be the clue.  For now I'm just assuming that
       the last one is the regular 0x16 bytes. */

    info = (SLTG_ImplInfo*)pBlk;
    while(1){
        pTI->typeattr.cImplTypes++;
        if(info->next == 0xffff)
            break;
        info = (SLTG_ImplInfo*)(pBlk + info->next);
    }

    info = (SLTG_ImplInfo*)pBlk;
    pTI->impltypes = TLBImplType_Alloc(pTI->typeattr.cImplTypes);
    pImplType = pTI->impltypes;
    while(1) {
	sltg_get_typelib_ref(ref_lookup, info->ref, &pImplType->hRef);
	pImplType->implflags = info->impltypeflags;
	++pImplType;

	if(info->next == 0xffff)
	    break;
	if(OneOnly)
	    FIXME_(typelib)("Interface inheriting more than one interface\n");
	info = (SLTG_ImplInfo*)(pBlk + info->next);
    }
    info++; /* see comment at top of function */
    return (char*)info;
}

static void SLTG_DoVars(char *pBlk, char *pFirstItem, ITypeInfoImpl *pTI, unsigned short cVars,
			const char *pNameTable, const sltg_ref_lookup_t *ref_lookup)
{
  TLBVarDesc *pVarDesc;
  const TLBString *prevName = NULL;
  SLTG_Variable *pItem;
  unsigned short i;
  WORD *pType;

  pVarDesc = pTI->vardescs = TLBVarDesc_Alloc(cVars);

  for(pItem = (SLTG_Variable *)pFirstItem, i = 0; i < cVars;
      pItem = (SLTG_Variable *)(pBlk + pItem->next), i++, ++pVarDesc) {

      pVarDesc->vardesc.memid = pItem->memid;

      if (pItem->magic != SLTG_VAR_MAGIC &&
          pItem->magic != SLTG_VAR_WITH_FLAGS_MAGIC) {
	  FIXME_(typelib)("var magic = %02x\n", pItem->magic);
	  return;
      }

      if (pItem->name == 0xfffe)
        pVarDesc->Name = prevName;
      else
        pVarDesc->Name = SLTG_ReadName(pNameTable, pItem->name, pTI->pTypeLib);

      TRACE_(typelib)("name: %s\n", debugstr_w(TLB_get_bstr(pVarDesc->Name)));
      TRACE_(typelib)("byte_offs = 0x%x\n", pItem->byte_offs);
      TRACE_(typelib)("memid = %#lx\n", pItem->memid);

      if(pItem->flags & 0x02)
	  pType = &pItem->type;
      else
	  pType = (WORD*)(pBlk + pItem->type);

      if (pItem->flags & ~0xda)
        FIXME_(typelib)("unhandled flags = %02x\n", pItem->flags & ~0xda);

      SLTG_DoElem(pType, pBlk,
		  &pVarDesc->vardesc.elemdescVar, ref_lookup);

      if (TRACE_ON(typelib)) {
          char buf[300];
          dump_TypeDesc(&pVarDesc->vardesc.elemdescVar.tdesc, buf);
          TRACE_(typelib)("elemdescVar: %s\n", buf);
      }

      if (pItem->flags & 0x40) {
        TRACE_(typelib)("VAR_DISPATCH\n");
        pVarDesc->vardesc.varkind = VAR_DISPATCH;
      }
      else if (pItem->flags & 0x10) {
        TRACE_(typelib)("VAR_CONST\n");
        pVarDesc->vardesc.varkind = VAR_CONST;
        pVarDesc->vardesc.lpvarValue = malloc(sizeof(VARIANT));
        V_VT(pVarDesc->vardesc.lpvarValue) = VT_INT;
        if (pItem->flags & 0x08)
          V_INT(pVarDesc->vardesc.lpvarValue) = pItem->byte_offs;
        else {
          switch (pVarDesc->vardesc.elemdescVar.tdesc.vt)
          {
            case VT_LPSTR:
            case VT_LPWSTR:
            case VT_BSTR:
            {
              WORD len = *(WORD *)(pBlk + pItem->byte_offs);
              BSTR str;
              TRACE_(typelib)("len = %u\n", len);
              if (len == 0xffff) {
                str = NULL;
              } else {
                INT alloc_len = MultiByteToWideChar(CP_ACP, 0, pBlk + pItem->byte_offs + 2, len, NULL, 0);
                str = SysAllocStringLen(NULL, alloc_len);
                MultiByteToWideChar(CP_ACP, 0, pBlk + pItem->byte_offs + 2, len, str, alloc_len);
              }
              V_VT(pVarDesc->vardesc.lpvarValue) = VT_BSTR;
              V_BSTR(pVarDesc->vardesc.lpvarValue) = str;
              break;
            }
            case VT_I2:
            case VT_UI2:
            case VT_I4:
            case VT_UI4:
            case VT_INT:
            case VT_UINT:
              V_INT(pVarDesc->vardesc.lpvarValue) =
                *(INT*)(pBlk + pItem->byte_offs);
              break;
            default:
              FIXME_(typelib)("VAR_CONST unimplemented for type %d\n", pVarDesc->vardesc.elemdescVar.tdesc.vt);
          }
        }
      }
      else {
        TRACE_(typelib)("VAR_PERINSTANCE\n");
        pVarDesc->vardesc.oInst = pItem->byte_offs;
        pVarDesc->vardesc.varkind = VAR_PERINSTANCE;
      }

      if (pItem->magic == SLTG_VAR_WITH_FLAGS_MAGIC)
        pVarDesc->vardesc.wVarFlags = pItem->varflags;

      if (pItem->flags & 0x80)
        pVarDesc->vardesc.wVarFlags |= VARFLAG_FREADONLY;

      prevName = pVarDesc->Name;
  }
  pTI->typeattr.cVars = cVars;
}

static void SLTG_DoFuncs(char *pBlk, char *pFirstItem, ITypeInfoImpl *pTI,
			 unsigned short cFuncs, char *pNameTable, const sltg_ref_lookup_t *ref_lookup)
{
    SLTG_Function *pFunc;
    unsigned short i;
    TLBFuncDesc *pFuncDesc;

    pTI->funcdescs = TLBFuncDesc_Alloc(cFuncs);

    pFuncDesc = pTI->funcdescs;
    for(pFunc = (SLTG_Function*)pFirstItem, i = 0; i < cFuncs && pFunc != (SLTG_Function*)0xFFFF;
	pFunc = (SLTG_Function*)(pBlk + pFunc->next), i++, ++pFuncDesc) {

        int param;
	WORD *pType, *pArg;

        switch (pFunc->magic & ~SLTG_FUNCTION_FLAGS_PRESENT) {
        case SLTG_FUNCTION_MAGIC:
            pFuncDesc->funcdesc.funckind = FUNC_PUREVIRTUAL;
            break;
        case SLTG_DISPATCH_FUNCTION_MAGIC:
            pFuncDesc->funcdesc.funckind = FUNC_DISPATCH;
            break;
        case SLTG_STATIC_FUNCTION_MAGIC:
            pFuncDesc->funcdesc.funckind = FUNC_STATIC;
            break;
        default:
	    FIXME("unimplemented func magic = %02x\n", pFunc->magic & ~SLTG_FUNCTION_FLAGS_PRESENT);
	    continue;
	}
	pFuncDesc->Name = SLTG_ReadName(pNameTable, pFunc->name, pTI->pTypeLib);

	pFuncDesc->funcdesc.memid = pFunc->dispid;
	pFuncDesc->funcdesc.invkind = pFunc->inv >> 4;
	pFuncDesc->funcdesc.callconv = pFunc->nacc & 0x7;
	pFuncDesc->funcdesc.cParams = pFunc->nacc >> 3;
	pFuncDesc->funcdesc.cParamsOpt = (pFunc->retnextopt & 0x7e) >> 1;
	if (pFuncDesc->funcdesc.funckind == FUNC_DISPATCH)
	    pFuncDesc->funcdesc.oVft = 0;
        else
	    pFuncDesc->funcdesc.oVft = (unsigned short)(pFunc->vtblpos & ~1) * sizeof(void *) / pTI->pTypeLib->ptr_size;

	if(pFunc->magic & SLTG_FUNCTION_FLAGS_PRESENT)
	    pFuncDesc->funcdesc.wFuncFlags = pFunc->funcflags;

	if(pFunc->retnextopt & 0x80)
	    pType = &pFunc->rettype;
	else
	    pType = (WORD*)(pBlk + pFunc->rettype);

	SLTG_DoElem(pType, pBlk, &pFuncDesc->funcdesc.elemdescFunc, ref_lookup);

	pFuncDesc->funcdesc.lprgelemdescParam =
	  calloc(pFuncDesc->funcdesc.cParams, sizeof(ELEMDESC));
	pFuncDesc->pParamDesc = TLBParDesc_Constructor(pFuncDesc->funcdesc.cParams);

	pArg = (WORD*)(pBlk + pFunc->arg_off);

	for(param = 0; param < pFuncDesc->funcdesc.cParams; param++) {
	    char *paramName = pNameTable + *pArg;
	    BOOL HaveOffs;
	    /* If arg type follows then paramName points to the 2nd
	       letter of the name, else the next WORD is an offset to
	       the arg type and paramName points to the first letter.
	       So let's take one char off paramName and see if we're
	       pointing at an alphanumeric char.  However if *pArg is
	       0xffff or 0xfffe then the param has no name, the former
	       meaning that the next WORD is the type, the latter
	       meaning that the next WORD is an offset to the type. */

	    HaveOffs = FALSE;
	    if(*pArg == 0xffff)
	        paramName = NULL;
	    else if(*pArg == 0xfffe) {
	        paramName = NULL;
		HaveOffs = TRUE;
	    }
	    else if(paramName[-1] && !isalnum(paramName[-1]))
	        HaveOffs = TRUE;

	    pArg++;

	    if(HaveOffs) { /* the next word is an offset to type */
	        pType = (WORD*)(pBlk + *pArg);
		SLTG_DoElem(pType, pBlk,
			    &pFuncDesc->funcdesc.lprgelemdescParam[param], ref_lookup);
		pArg++;
	    } else {
		if(paramName)
		  paramName--;
		pArg = SLTG_DoElem(pArg, pBlk,
                                   &pFuncDesc->funcdesc.lprgelemdescParam[param], ref_lookup);
	    }

	    /* Are we an optional param ? */
	    if(pFuncDesc->funcdesc.cParams - param <=
	       pFuncDesc->funcdesc.cParamsOpt)
	      pFuncDesc->funcdesc.lprgelemdescParam[param].paramdesc.wParamFlags |= PARAMFLAG_FOPT;

	    if(paramName) {
	        pFuncDesc->pParamDesc[param].Name = SLTG_ReadName(pNameTable,
	                paramName - pNameTable, pTI->pTypeLib);
	    } else {
	        pFuncDesc->pParamDesc[param].Name = pFuncDesc->Name;
	    }
	}
    }
    pTI->typeattr.cFuncs = cFuncs;
}

static void SLTG_ProcessCoClass(char *pBlk, ITypeInfoImpl *pTI,
				char *pNameTable, SLTG_TypeInfoHeader *pTIHeader,
				SLTG_TypeInfoTail *pTITail)
{
    char *pFirstItem;
    sltg_ref_lookup_t *ref_lookup = NULL;

    if(pTIHeader->href_table != 0xffffffff) {
        ref_lookup = SLTG_DoRefs((SLTG_RefInfo*)((char *)pTIHeader + pTIHeader->href_table), pTI->pTypeLib,
		    pNameTable);
    }

    pFirstItem = pBlk;

    if(*(WORD*)pFirstItem == SLTG_IMPL_MAGIC) {
        SLTG_DoImpls(pFirstItem, pTI, FALSE, ref_lookup);
    }
    free(ref_lookup);
}


static void SLTG_ProcessInterface(char *pBlk, ITypeInfoImpl *pTI,
				  char *pNameTable, SLTG_TypeInfoHeader *pTIHeader,
				  const SLTG_TypeInfoTail *pTITail)
{
    char *pFirstItem;
    sltg_ref_lookup_t *ref_lookup = NULL;

    if(pTIHeader->href_table != 0xffffffff) {
        ref_lookup = SLTG_DoRefs((SLTG_RefInfo*)((char *)pTIHeader + pTIHeader->href_table), pTI->pTypeLib,
		    pNameTable);
    }

    pFirstItem = pBlk;

    if(*(WORD*)pFirstItem == SLTG_IMPL_MAGIC) {
        SLTG_DoImpls(pFirstItem, pTI, TRUE, ref_lookup);
    }

    if (pTITail->funcs_off != 0xffff)
        SLTG_DoFuncs(pBlk, pBlk + pTITail->funcs_off, pTI, pTITail->cFuncs, pNameTable, ref_lookup);

    free(ref_lookup);

    if (TRACE_ON(typelib))
        dump_TLBFuncDesc(pTI->funcdescs, pTI->typeattr.cFuncs);
}

static void SLTG_ProcessRecord(char *pBlk, ITypeInfoImpl *pTI,
			       const char *pNameTable, SLTG_TypeInfoHeader *pTIHeader,
			       const SLTG_TypeInfoTail *pTITail)
{
  SLTG_DoVars(pBlk, pBlk + pTITail->vars_off, pTI, pTITail->cVars, pNameTable, NULL);
}

static void SLTG_ProcessAlias(char *pBlk, ITypeInfoImpl *pTI,
			      char *pNameTable, SLTG_TypeInfoHeader *pTIHeader,
			      const SLTG_TypeInfoTail *pTITail)
{
  WORD *pType;
  sltg_ref_lookup_t *ref_lookup = NULL;

  if (pTITail->simple_alias) {
      /* if simple alias, no more processing required */
      pTI->tdescAlias = calloc(1, sizeof(TYPEDESC));
      pTI->tdescAlias->vt = pTITail->tdescalias_vt;
      return;
  }

  if(pTIHeader->href_table != 0xffffffff) {
      ref_lookup = SLTG_DoRefs((SLTG_RefInfo*)((char *)pTIHeader + pTIHeader->href_table), pTI->pTypeLib,
		  pNameTable);
  }

  /* otherwise it is an offset to a type */
  pType = (WORD *)(pBlk + pTITail->tdescalias_vt);

  pTI->tdescAlias = malloc(sizeof(TYPEDESC));
  SLTG_DoType(pType, pBlk, pTI->tdescAlias, ref_lookup);

  free(ref_lookup);
}

static void SLTG_ProcessDispatch(char *pBlk, ITypeInfoImpl *pTI,
				 char *pNameTable, SLTG_TypeInfoHeader *pTIHeader,
				 const SLTG_TypeInfoTail *pTITail)
{
  sltg_ref_lookup_t *ref_lookup = NULL;
  if (pTIHeader->href_table != 0xffffffff)
      ref_lookup = SLTG_DoRefs((SLTG_RefInfo*)((char *)pTIHeader + pTIHeader->href_table), pTI->pTypeLib,
                                  pNameTable);

  if (pTITail->vars_off != 0xffff)
    SLTG_DoVars(pBlk, pBlk + pTITail->vars_off, pTI, pTITail->cVars, pNameTable, ref_lookup);

  if (pTITail->funcs_off != 0xffff)
    SLTG_DoFuncs(pBlk, pBlk + pTITail->funcs_off, pTI, pTITail->cFuncs, pNameTable, ref_lookup);

  if (pTITail->impls_off != 0xffff)
    SLTG_DoImpls(pBlk + pTITail->impls_off, pTI, FALSE, ref_lookup);

  /* this is necessary to cope with MSFT typelibs that set cFuncs to the number
   * of dispinterface functions including the IDispatch ones, so
   * ITypeInfo::GetFuncDesc takes the real value for cFuncs from cbSizeVft */
  pTI->typeattr.cbSizeVft = pTI->typeattr.cFuncs * pTI->pTypeLib->ptr_size;

  free(ref_lookup);
  if (TRACE_ON(typelib))
      dump_TLBFuncDesc(pTI->funcdescs, pTI->typeattr.cFuncs);
}

static void SLTG_ProcessEnum(char *pBlk, ITypeInfoImpl *pTI,
			     const char *pNameTable, SLTG_TypeInfoHeader *pTIHeader,
			     const SLTG_TypeInfoTail *pTITail)
{
  SLTG_DoVars(pBlk, pBlk + pTITail->vars_off, pTI, pTITail->cVars, pNameTable, NULL);
}

static void SLTG_ProcessModule(char *pBlk, ITypeInfoImpl *pTI,
			       char *pNameTable, SLTG_TypeInfoHeader *pTIHeader,
			       const SLTG_TypeInfoTail *pTITail)
{
  sltg_ref_lookup_t *ref_lookup = NULL;
  if (pTIHeader->href_table != 0xffffffff)
      ref_lookup = SLTG_DoRefs((SLTG_RefInfo*)((char *)pTIHeader + pTIHeader->href_table), pTI->pTypeLib,
                                  pNameTable);

  if (pTITail->vars_off != 0xffff)
    SLTG_DoVars(pBlk, pBlk + pTITail->vars_off, pTI, pTITail->cVars, pNameTable, ref_lookup);

  if (pTITail->funcs_off != 0xffff)
    SLTG_DoFuncs(pBlk, pBlk + pTITail->funcs_off, pTI, pTITail->cFuncs, pNameTable, ref_lookup);
  free(ref_lookup);
  if (TRACE_ON(typelib))
    dump_TypeInfo(pTI);
}

/* Because SLTG_OtherTypeInfo is such a painful struct, we make a more
   manageable copy of it into this */
typedef struct {
  WORD small_no;
  char *index_name;
  char *other_name;
  WORD res1a;
  WORD name_offs;
  WORD more_bytes;
  char *extra;
  WORD res20;
  DWORD helpcontext;
  WORD res26;
  GUID uuid;
} SLTG_InternalOtherTypeInfo;

/****************************************************************************
 *	ITypeLib2_Constructor_SLTG
 *
 * loading a SLTG typelib from an in-memory image
 */
static ITypeLib2* ITypeLib2_Constructor_SLTG(LPVOID pLib, DWORD dwTLBLength)
{
    ITypeLibImpl *pTypeLibImpl;
    SLTG_Header *pHeader;
    SLTG_BlkEntry *pBlkEntry;
    SLTG_Magic *pMagic;
    SLTG_Index *pIndex;
    SLTG_Pad9 *pPad9;
    LPVOID pBlk, pFirstBlk;
    SLTG_LibBlk *pLibBlk;
    SLTG_InternalOtherTypeInfo *pOtherTypeInfoBlks;
    char *pAfterOTIBlks = NULL;
    char *pNameTable, *ptr;
    int i;
    DWORD len, order;
    ITypeInfoImpl **ppTypeInfoImpl;

    TRACE_(typelib)("%p, TLB length = %ld\n", pLib, dwTLBLength);


    pTypeLibImpl = TypeLibImpl_Constructor();
    if (!pTypeLibImpl) return NULL;

    pHeader = pLib;

    TRACE_(typelib)("header:\n");
    TRACE_(typelib)("\tmagic %#lx, file blocks = %d\n", pHeader->SLTG_magic,
	  pHeader->nrOfFileBlks );
    if (pHeader->SLTG_magic != SLTG_SIGNATURE)
    {
        FIXME_(typelib)("Header type magic %#lx not supported.\n", pHeader->SLTG_magic);
	return NULL;
    }

    /* There are pHeader->nrOfFileBlks - 2 TypeInfo records in this typelib */
    pTypeLibImpl->TypeInfoCount = pHeader->nrOfFileBlks - 2;

    /* This points to pHeader->nrOfFileBlks - 1 of SLTG_BlkEntry */
    pBlkEntry = (SLTG_BlkEntry*)(pHeader + 1);

    /* Next we have a magic block */
    pMagic = (SLTG_Magic*)(pBlkEntry + pHeader->nrOfFileBlks - 1);

    /* Let's see if we're still in sync */
    if(memcmp(pMagic->CompObj_magic, SLTG_COMPOBJ_MAGIC,
	      sizeof(SLTG_COMPOBJ_MAGIC))) {
        FIXME_(typelib)("CompObj magic = %s\n", pMagic->CompObj_magic);
	return NULL;
    }
    if(memcmp(pMagic->dir_magic, SLTG_DIR_MAGIC,
	      sizeof(SLTG_DIR_MAGIC))) {
        FIXME_(typelib)("dir magic = %s\n", pMagic->dir_magic);
	return NULL;
    }

    pIndex = (SLTG_Index*)(pMagic+1);

    pPad9 = (SLTG_Pad9*)(pIndex + pTypeLibImpl->TypeInfoCount);

    pFirstBlk = pPad9 + 1;

    /* We'll set up a ptr to the main library block, which is the last one. */

    for(pBlk = pFirstBlk, order = pHeader->first_blk - 1;
	  pBlkEntry[order].next != 0;
	  order = pBlkEntry[order].next - 1) {
       pBlk = (char*)pBlk + pBlkEntry[order].len;
    }
    pLibBlk = pBlk;

    len = SLTG_ReadLibBlk(pLibBlk, pTypeLibImpl);

    /* Now there are 0x40 bytes of 0xffff with the numbers 0 to TypeInfoCount
       interspersed */

    len += 0x40;

    /* And now TypeInfoCount of SLTG_OtherTypeInfo */

    pOtherTypeInfoBlks = calloc(pTypeLibImpl->TypeInfoCount, sizeof(*pOtherTypeInfoBlks));


    ptr = (char*)pLibBlk + len;

    for(i = 0; i < pTypeLibImpl->TypeInfoCount; i++) {
	WORD w, extra;
	len = 0;

	pOtherTypeInfoBlks[i].small_no = *(WORD*)ptr;

	w = *(WORD*)(ptr + 2);
	if(w != 0xffff) {
	    len += w;
	    pOtherTypeInfoBlks[i].index_name = malloc(w + 1);
	    memcpy(pOtherTypeInfoBlks[i].index_name, ptr + 4, w);
	    pOtherTypeInfoBlks[i].index_name[w] = '\0';
	}
	w = *(WORD*)(ptr + 4 + len);
	if(w != 0xffff) {
	    TRACE_(typelib)("\twith %s\n", debugstr_an(ptr + 6 + len, w));
	    len += w;
	    pOtherTypeInfoBlks[i].other_name = malloc(w + 1);
	    memcpy(pOtherTypeInfoBlks[i].other_name, ptr + 6 + len, w);
	    pOtherTypeInfoBlks[i].other_name[w] = '\0';
	}
	pOtherTypeInfoBlks[i].res1a = *(WORD*)(ptr + len + 6);
	pOtherTypeInfoBlks[i].name_offs = *(WORD*)(ptr + len + 8);
	extra = pOtherTypeInfoBlks[i].more_bytes = *(WORD*)(ptr + 10 + len);
	if(extra) {
	    pOtherTypeInfoBlks[i].extra = malloc(extra);
	    memcpy(pOtherTypeInfoBlks[i].extra, ptr + 12, extra);
	    len += extra;
	}
	pOtherTypeInfoBlks[i].res20 = *(WORD*)(ptr + 12 + len);
	pOtherTypeInfoBlks[i].helpcontext = *(DWORD*)(ptr + 14 + len);
	pOtherTypeInfoBlks[i].res26 = *(WORD*)(ptr + 18 + len);
	memcpy(&pOtherTypeInfoBlks[i].uuid, ptr + 20 + len, sizeof(GUID));
	len += sizeof(SLTG_OtherTypeInfo);
	ptr += len;
    }

    pAfterOTIBlks = ptr;

    /* Skip this WORD and get the next DWORD */
    len = *(DWORD*)(pAfterOTIBlks + 2);

    /* Now add this to pLibBLk look at what we're pointing at and
       possibly add 0x20, then add 0x216, sprinkle a bit a magic
       dust and we should be pointing at the beginning of the name
       table */

    pNameTable = (char*)pLibBlk + len;

   switch(*(WORD*)pNameTable) {
   case 0xffff:
       break;
   case 0x0200:
       pNameTable += 0x20;
       break;
   default:
       FIXME_(typelib)("pNameTable jump = %x\n", *(WORD*)pNameTable);
       break;
   }

    pNameTable += 0x216;

    pNameTable += 2;

    TRACE_(typelib)("Library name is %s\n", pNameTable + pLibBlk->name);

    pTypeLibImpl->Name = SLTG_ReadName(pNameTable, pLibBlk->name, pTypeLibImpl);


    /* Hopefully we now have enough ptrs set up to actually read in
       some TypeInfos.  It's not clear which order to do them in, so
       I'll just follow the links along the BlkEntry chain and read
       them in the order in which they are in the file */

    pTypeLibImpl->typeinfos = calloc(pTypeLibImpl->TypeInfoCount, sizeof(ITypeInfoImpl*));
    ppTypeInfoImpl = pTypeLibImpl->typeinfos;

    for(pBlk = pFirstBlk, order = pHeader->first_blk - 1, i = 0;
	pBlkEntry[order].next != 0;
	order = pBlkEntry[order].next - 1, i++) {

      SLTG_TypeInfoHeader *pTIHeader;
      SLTG_TypeInfoTail *pTITail;
      SLTG_MemberHeader *pMemHeader;

      if(strcmp(pBlkEntry[order].index_string + (char*)pMagic, pOtherTypeInfoBlks[i].index_name)) {
        FIXME_(typelib)("Index strings don't match\n");
        free(pOtherTypeInfoBlks);
        return NULL;
      }

      pTIHeader = pBlk;
      if(pTIHeader->magic != SLTG_TIHEADER_MAGIC) {
	FIXME_(typelib)("TypeInfoHeader magic = %04x\n", pTIHeader->magic);
       free(pOtherTypeInfoBlks);
	return NULL;
      }
      TRACE_(typelib)("pTIHeader->res06 = %lx, pTIHeader->res0e = %lx, "
        "pTIHeader->res16 = %lx, pTIHeader->res1e = %lx\n",
        pTIHeader->res06, pTIHeader->res0e, pTIHeader->res16, pTIHeader->res1e);

      *ppTypeInfoImpl = ITypeInfoImpl_Constructor();
      (*ppTypeInfoImpl)->pTypeLib = pTypeLibImpl;
      (*ppTypeInfoImpl)->index = i;
      (*ppTypeInfoImpl)->Name = SLTG_ReadName(pNameTable, pOtherTypeInfoBlks[i].name_offs, pTypeLibImpl);
      (*ppTypeInfoImpl)->dwHelpContext = pOtherTypeInfoBlks[i].helpcontext;
      (*ppTypeInfoImpl)->guid = TLB_append_guid(&pTypeLibImpl->guid_list, &pOtherTypeInfoBlks[i].uuid, 2);
      (*ppTypeInfoImpl)->typeattr.typekind = pTIHeader->typekind;
      (*ppTypeInfoImpl)->typeattr.wMajorVerNum = pTIHeader->major_version;
      (*ppTypeInfoImpl)->typeattr.wMinorVerNum = pTIHeader->minor_version;
      (*ppTypeInfoImpl)->typeattr.wTypeFlags =
	(pTIHeader->typeflags1 >> 3) | (pTIHeader->typeflags2 << 5);

      if((*ppTypeInfoImpl)->typeattr.wTypeFlags & TYPEFLAG_FDUAL)
	(*ppTypeInfoImpl)->typeattr.typekind = TKIND_DISPATCH;

      if((pTIHeader->typeflags1 & 7) != 2)
	FIXME_(typelib)("typeflags1 = %02x\n", pTIHeader->typeflags1);
      if(pTIHeader->typeflags3 != 2)
	FIXME_(typelib)("typeflags3 = %02x\n", pTIHeader->typeflags3);

      TRACE_(typelib)("TypeInfo %s of kind %s guid %s typeflags %04x\n",
	    debugstr_w(TLB_get_bstr((*ppTypeInfoImpl)->Name)),
	    typekind_desc[pTIHeader->typekind],
	    debugstr_guid(TLB_get_guidref((*ppTypeInfoImpl)->guid)),
	    (*ppTypeInfoImpl)->typeattr.wTypeFlags);

      pMemHeader = (SLTG_MemberHeader*)((char *)pBlk + pTIHeader->elem_table);

      pTITail = (SLTG_TypeInfoTail*)((char *)(pMemHeader + 1) + pMemHeader->cbExtra);

      (*ppTypeInfoImpl)->typeattr.cbAlignment = pTITail->cbAlignment;
      (*ppTypeInfoImpl)->typeattr.cbSizeInstance = pTITail->cbSizeInstance;
      (*ppTypeInfoImpl)->typeattr.cbSizeVft = pTITail->cbSizeVft;

      switch(pTIHeader->typekind) {
      case TKIND_ENUM:
	SLTG_ProcessEnum((char *)(pMemHeader + 1), *ppTypeInfoImpl, pNameTable,
                         pTIHeader, pTITail);
	break;

      case TKIND_RECORD:
	SLTG_ProcessRecord((char *)(pMemHeader + 1), *ppTypeInfoImpl, pNameTable,
                           pTIHeader, pTITail);
	break;

      case TKIND_INTERFACE:
	SLTG_ProcessInterface((char *)(pMemHeader + 1), *ppTypeInfoImpl, pNameTable,
                              pTIHeader, pTITail);
	break;

      case TKIND_COCLASS:
	SLTG_ProcessCoClass((char *)(pMemHeader + 1), *ppTypeInfoImpl, pNameTable,
                            pTIHeader, pTITail);
	break;

      case TKIND_ALIAS:
	SLTG_ProcessAlias((char *)(pMemHeader + 1), *ppTypeInfoImpl, pNameTable,
                          pTIHeader, pTITail);
	break;

      case TKIND_DISPATCH:
	SLTG_ProcessDispatch((char *)(pMemHeader + 1), *ppTypeInfoImpl, pNameTable,
                             pTIHeader, pTITail);
	break;

      case TKIND_MODULE:
	SLTG_ProcessModule((char *)(pMemHeader + 1), *ppTypeInfoImpl, pNameTable,
                           pTIHeader, pTITail);
	break;

      default:
	FIXME("Not processing typekind %d\n", pTIHeader->typekind);
	break;

      }

      /* could get cFuncs, cVars and cImplTypes from here
		       but we've already set those */
#define X(x) TRACE_(typelib)("tt "#x": %x\n",pTITail->res##x);
      X(06);
      X(16);
      X(18);
      X(1a);
      X(1e);
      X(24);
      X(26);
      X(2a);
      X(2c);
      X(2e);
      X(30);
      X(32);
      X(34);
#undef X
      ++ppTypeInfoImpl;
      pBlk = (char*)pBlk + pBlkEntry[order].len;
    }

    if(i != pTypeLibImpl->TypeInfoCount) {
      FIXME("Somehow processed %d TypeInfos\n", i);
      free(pOtherTypeInfoBlks);
      return NULL;
    }

    free(pOtherTypeInfoBlks);
    return &pTypeLibImpl->ITypeLib2_iface;
}

static HRESULT WINAPI ITypeLib2_fnQueryInterface(ITypeLib2 *iface, REFIID riid, void **ppv)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);

    TRACE("(%p)->(IID: %s)\n",This,debugstr_guid(riid));

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid,&IID_ITypeLib)||
       IsEqualIID(riid,&IID_ITypeLib2))
    {
        *ppv = &This->ITypeLib2_iface;
    }
    else if(IsEqualIID(riid, &IID_ICreateTypeLib) ||
             IsEqualIID(riid, &IID_ICreateTypeLib2))
    {
        *ppv = &This->ICreateTypeLib2_iface;
    }
    else
    {
        *ppv = NULL;
        TRACE("-- Interface: E_NOINTERFACE\n");
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ITypeLib2_fnAddRef( ITypeLib2 *iface)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI ITypeLib2_fnRelease( ITypeLib2 *iface)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);
    ULONG ref;

    EnterCriticalSection(&cache_section);
    ref = InterlockedDecrement(&This->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
      TLBImpLib *pImpLib, *pImpLibNext;
      TLBRefType *ref_type, *ref_type_next;
      TLBString *tlbstr, *tlbstr_next;
      TLBGuid *tlbguid, *tlbguid_next;
      int i;

      /* remove cache entry */
      if(This->path)
      {
          TRACE("removing from cache list\n");
          if(This->entry.next)
              list_remove(&This->entry);
          free(This->path);
      }
      TRACE(" destroying ITypeLib(%p)\n",This);

      LIST_FOR_EACH_ENTRY_SAFE(tlbstr, tlbstr_next, &This->string_list, TLBString, entry) {
          list_remove(&tlbstr->entry);
          SysFreeString(tlbstr->str);
          free(tlbstr);
      }

      LIST_FOR_EACH_ENTRY_SAFE(tlbstr, tlbstr_next, &This->name_list, TLBString, entry) {
          list_remove(&tlbstr->entry);
          SysFreeString(tlbstr->str);
          free(tlbstr);
      }

      LIST_FOR_EACH_ENTRY_SAFE(tlbguid, tlbguid_next, &This->guid_list, TLBGuid, entry) {
          list_remove(&tlbguid->entry);
          free(tlbguid);
      }

      TLB_FreeCustData(&This->custdata_list);

      for (i = 0; i < This->ctTypeDesc; i++)
          if (This->pTypeDesc[i].vt == VT_CARRAY)
              free(This->pTypeDesc[i].lpadesc);

      free(This->pTypeDesc);

      LIST_FOR_EACH_ENTRY_SAFE(pImpLib, pImpLibNext, &This->implib_list, TLBImpLib, entry)
      {
          if (pImpLib->pImpTypeLib)
              ITypeLib2_Release(&pImpLib->pImpTypeLib->ITypeLib2_iface);
          SysFreeString(pImpLib->name);

          list_remove(&pImpLib->entry);
          free(pImpLib);
      }

      LIST_FOR_EACH_ENTRY_SAFE(ref_type, ref_type_next, &This->ref_list, TLBRefType, entry)
      {
          list_remove(&ref_type->entry);
          free(ref_type);
      }

      for (i = 0; i < This->TypeInfoCount; ++i){
          free(This->typeinfos[i]->tdescAlias);
          ITypeInfoImpl_Destroy(This->typeinfos[i]);
      }
      free(This->typeinfos);
      free(This);
    }

    LeaveCriticalSection(&cache_section);
    return ref;
}

/* ITypeLib::GetTypeInfoCount
 *
 * Returns the number of type descriptions in the type library
 */
static UINT WINAPI ITypeLib2_fnGetTypeInfoCount( ITypeLib2 *iface)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);
    TRACE("(%p)->count is %d\n",This, This->TypeInfoCount);
    return This->TypeInfoCount;
}

/* ITypeLib::GetTypeInfo
 *
 * retrieves the specified type description in the library.
 */
static HRESULT WINAPI ITypeLib2_fnGetTypeInfo(
    ITypeLib2 *iface,
    UINT index,
    ITypeInfo **ppTInfo)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);

    TRACE("%p %u %p\n", This, index, ppTInfo);

    if(!ppTInfo)
        return E_INVALIDARG;

    if(index >= This->TypeInfoCount)
        return TYPE_E_ELEMENTNOTFOUND;

    *ppTInfo = (ITypeInfo *)&This->typeinfos[index]->ITypeInfo2_iface;
    ITypeInfo_AddRef(*ppTInfo);

    return S_OK;
}


/* ITypeLibs::GetTypeInfoType
 *
 * Retrieves the type of a type description.
 */
static HRESULT WINAPI ITypeLib2_fnGetTypeInfoType(
    ITypeLib2 *iface,
    UINT index,
    TYPEKIND *pTKind)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);

    TRACE("(%p, %d, %p)\n", This, index, pTKind);

    if(!pTKind)
        return E_INVALIDARG;

    if(index >= This->TypeInfoCount)
        return TYPE_E_ELEMENTNOTFOUND;

    *pTKind = This->typeinfos[index]->typeattr.typekind;

    return S_OK;
}

/* ITypeLib::GetTypeInfoOfGuid
 *
 * Retrieves the type description that corresponds to the specified GUID.
 *
 */
static HRESULT WINAPI ITypeLib2_fnGetTypeInfoOfGuid(
    ITypeLib2 *iface,
    REFGUID guid,
    ITypeInfo **ppTInfo)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);
    int i;

    TRACE("%p %s %p\n", This, debugstr_guid(guid), ppTInfo);

    for(i = 0; i < This->TypeInfoCount; ++i){
        if(IsEqualIID(TLB_get_guid_null(This->typeinfos[i]->guid), guid)){
            *ppTInfo = (ITypeInfo *)&This->typeinfos[i]->ITypeInfo2_iface;
            ITypeInfo_AddRef(*ppTInfo);
            return S_OK;
        }
    }

    return TYPE_E_ELEMENTNOTFOUND;
}

/* ITypeLib::GetLibAttr
 *
 * Retrieves the structure that contains the library's attributes.
 *
 */
static HRESULT WINAPI ITypeLib2_fnGetLibAttr(
	ITypeLib2 *iface,
	LPTLIBATTR *attr)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);

    TRACE("(%p, %p)\n", This, attr);

    if (!attr) return E_INVALIDARG;

    *attr = malloc(sizeof(**attr));
    if (!*attr) return E_OUTOFMEMORY;

    (*attr)->guid = *TLB_get_guid_null(This->guid);
    (*attr)->lcid = This->set_lcid;
    (*attr)->syskind = This->syskind;
    (*attr)->wMajorVerNum = This->ver_major;
    (*attr)->wMinorVerNum = This->ver_minor;
    (*attr)->wLibFlags = This->libflags;

    return S_OK;
}

/* ITypeLib::GetTypeComp
 *
 * Enables a client compiler to bind to a library's types, variables,
 * constants, and global functions.
 *
 */
static HRESULT WINAPI ITypeLib2_fnGetTypeComp(
	ITypeLib2 *iface,
	ITypeComp **ppTComp)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);

    TRACE("(%p)->(%p)\n",This,ppTComp);
    *ppTComp = &This->ITypeComp_iface;
    ITypeComp_AddRef(*ppTComp);

    return S_OK;
}

/* ITypeLib::GetDocumentation
 *
 * Retrieves the library's documentation string, the complete Help file name
 * and path, and the context identifier for the library Help topic in the Help
 * file.
 *
 * On a successful return all non-null BSTR pointers will have been set,
 * possibly to NULL.
 */
static HRESULT WINAPI ITypeLib2_fnGetDocumentation(
    ITypeLib2 *iface,
    INT index,
    BSTR *pBstrName,
    BSTR *pBstrDocString,
    DWORD *pdwHelpContext,
    BSTR *pBstrHelpFile)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);
    HRESULT result = E_INVALIDARG;
    ITypeInfo *pTInfo;

    TRACE("(%p) index %d Name(%p) DocString(%p) HelpContext(%p) HelpFile(%p)\n",
        This, index,
        pBstrName, pBstrDocString,
        pdwHelpContext, pBstrHelpFile);

    if(index<0)
    {
        /* documentation for the typelib */
        if(pBstrName)
        {
            if (This->Name)
            {
                if(!(*pBstrName = SysAllocString(TLB_get_bstr(This->Name))))
                    goto memerr1;
            }
            else
                *pBstrName = NULL;
        }
        if(pBstrDocString)
        {
            if (This->DocString)
            {
                if(!(*pBstrDocString = SysAllocString(TLB_get_bstr(This->DocString))))
                    goto memerr2;
            }
            else
                *pBstrDocString = NULL;
        }
        if(pdwHelpContext)
        {
            *pdwHelpContext = This->dwHelpContext;
        }
        if(pBstrHelpFile)
        {
            if (This->HelpFile)
            {
                if(!(*pBstrHelpFile = SysAllocString(TLB_get_bstr(This->HelpFile))))
                    goto memerr3;
            }
            else
                *pBstrHelpFile = NULL;
        }

        result = S_OK;
    }
    else
    {
        /* for a typeinfo */
        result = ITypeLib2_fnGetTypeInfo(iface, index, &pTInfo);

        if(SUCCEEDED(result))
        {
            result = ITypeInfo_GetDocumentation(pTInfo,
                                          MEMBERID_NIL,
                                          pBstrName,
                                          pBstrDocString,
                                          pdwHelpContext, pBstrHelpFile);

            ITypeInfo_Release(pTInfo);
        }
    }
    return result;
memerr3:
    if (pBstrDocString) SysFreeString (*pBstrDocString);
memerr2:
    if (pBstrName) SysFreeString (*pBstrName);
memerr1:
    return STG_E_INSUFFICIENTMEMORY;
}

/* ITypeLib::IsName
 *
 * Indicates whether a passed-in string contains the name of a type or member
 * described in the library.
 *
 */
static HRESULT WINAPI ITypeLib2_fnIsName(
	ITypeLib2 *iface,
	LPOLESTR szNameBuf,
	ULONG lHashVal,
	BOOL *pfName)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);
    int tic;
    UINT nNameBufLen = (lstrlenW(szNameBuf)+1)*sizeof(WCHAR), fdc, vrc;

    TRACE("%p, %s, %#lx, %p.\n", iface, debugstr_w(szNameBuf), lHashVal, pfName);

    *pfName=TRUE;
    for(tic = 0; tic < This->TypeInfoCount; ++tic){
        ITypeInfoImpl *pTInfo = This->typeinfos[tic];
        if(!TLB_str_memcmp(szNameBuf, pTInfo->Name, nNameBufLen)) goto ITypeLib2_fnIsName_exit;
        for(fdc = 0; fdc < pTInfo->typeattr.cFuncs; ++fdc) {
            TLBFuncDesc *pFInfo = &pTInfo->funcdescs[fdc];
            int pc;
            if(!TLB_str_memcmp(szNameBuf, pFInfo->Name, nNameBufLen)) goto ITypeLib2_fnIsName_exit;
            for(pc=0; pc < pFInfo->funcdesc.cParams; pc++){
                if(!TLB_str_memcmp(szNameBuf, pFInfo->pParamDesc[pc].Name, nNameBufLen))
                    goto ITypeLib2_fnIsName_exit;
            }
        }
        for(vrc = 0; vrc < pTInfo->typeattr.cVars; ++vrc){
            TLBVarDesc *pVInfo = &pTInfo->vardescs[vrc];
            if(!TLB_str_memcmp(szNameBuf, pVInfo->Name, nNameBufLen)) goto ITypeLib2_fnIsName_exit;
        }

    }
    *pfName=FALSE;

ITypeLib2_fnIsName_exit:
    TRACE("(%p)slow! search for %s: %sfound!\n", This,
          debugstr_w(szNameBuf), *pfName ? "" : "NOT ");

    return S_OK;
}

/* ITypeLib::FindName
 *
 * Finds occurrences of a type description in a type library. This may be used
 * to quickly verify that a name exists in a type library.
 *
 */
static HRESULT WINAPI ITypeLib2_fnFindName(
	ITypeLib2 *iface,
	LPOLESTR name,
	ULONG hash,
	ITypeInfo **ppTInfo,
	MEMBERID *memid,
	UINT16 *found)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);
    int tic;
    UINT count = 0;
    UINT len;

    TRACE("%p, %s %#lx, %p, %p, %p.\n", iface, debugstr_w(name), hash, ppTInfo, memid, found);

    if ((!name && hash == 0) || !ppTInfo || !memid || !found)
        return E_INVALIDARG;

    len = (lstrlenW(name) + 1)*sizeof(WCHAR);
    for(tic = 0; count < *found && tic < This->TypeInfoCount; ++tic) {
        ITypeInfoImpl *pTInfo = This->typeinfos[tic];
        TLBVarDesc *var;
        UINT fdc;

        if(!TLB_str_memcmp(name, pTInfo->Name, len)) {
            memid[count] = MEMBERID_NIL;
            goto ITypeLib2_fnFindName_exit;
        }

        for(fdc = 0; fdc < pTInfo->typeattr.cFuncs; ++fdc) {
            TLBFuncDesc *func = &pTInfo->funcdescs[fdc];

            if(!TLB_str_memcmp(name, func->Name, len)) {
                memid[count] = func->funcdesc.memid;
                goto ITypeLib2_fnFindName_exit;
            }
        }

        var = TLB_get_vardesc_by_name(pTInfo, name);
        if (var) {
            memid[count] = var->vardesc.memid;
            goto ITypeLib2_fnFindName_exit;
        }

        continue;
ITypeLib2_fnFindName_exit:
        ITypeInfo2_AddRef(&pTInfo->ITypeInfo2_iface);
        ppTInfo[count] = (ITypeInfo *)&pTInfo->ITypeInfo2_iface;
        count++;
    }
    TRACE("found %d typeinfos\n", count);

    *found = count;

    return S_OK;
}

/* ITypeLib::ReleaseTLibAttr
 *
 * Releases the TLIBATTR originally obtained from ITypeLib::GetLibAttr.
 *
 */
static VOID WINAPI ITypeLib2_fnReleaseTLibAttr(
	ITypeLib2 *iface,
	TLIBATTR *pTLibAttr)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);
    TRACE("(%p)->(%p)\n", This, pTLibAttr);
    free(pTLibAttr);
}

/* ITypeLib2::GetCustData
 *
 * gets the custom data
 */
static HRESULT WINAPI ITypeLib2_fnGetCustData(
	ITypeLib2 * iface,
	REFGUID guid,
        VARIANT *pVarVal)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);
    TLBCustData *pCData;

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(guid), pVarVal);

    pCData = TLB_get_custdata_by_guid(&This->custdata_list, guid);
    if(!pCData)
        return TYPE_E_ELEMENTNOTFOUND;

    VariantInit(pVarVal);
    VariantCopy(pVarVal, &pCData->data);

    return S_OK;
}

/* ITypeLib2::GetLibStatistics
 *
 * Returns statistics about a type library that are required for efficient
 * sizing of hash tables.
 *
 */
static HRESULT WINAPI ITypeLib2_fnGetLibStatistics(
	ITypeLib2 * iface,
        ULONG *pcUniqueNames,
	ULONG *pcchUniqueNames)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);

    FIXME("(%p): stub!\n", This);

    if(pcUniqueNames) *pcUniqueNames=1;
    if(pcchUniqueNames) *pcchUniqueNames=1;
    return S_OK;
}

/* ITypeLib2::GetDocumentation2
 *
 * Retrieves the library's documentation string, the complete Help file name
 * and path, the localization context to use, and the context ID for the
 * library Help topic in the Help file.
 *
 */
static HRESULT WINAPI ITypeLib2_fnGetDocumentation2(
	ITypeLib2 * iface,
        INT index,
	LCID lcid,
	BSTR *pbstrHelpString,
        DWORD *pdwHelpStringContext,
	BSTR *pbstrHelpStringDll)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);
    HRESULT result;
    ITypeInfo *pTInfo;

    FIXME("%p, %d, %#lx, partially implemented stub!\n", iface, index, lcid);

    /* the help string should be obtained from the helpstringdll,
     * using the _DLLGetDocumentation function, based on the supplied
     * lcid. Nice to do sometime...
     */
    if(index<0)
    {
      /* documentation for the typelib */
      if(pbstrHelpString)
        *pbstrHelpString=SysAllocString(TLB_get_bstr(This->DocString));
      if(pdwHelpStringContext)
        *pdwHelpStringContext=This->dwHelpContext;
      if(pbstrHelpStringDll)
        *pbstrHelpStringDll=SysAllocString(TLB_get_bstr(This->HelpStringDll));

      result = S_OK;
    }
    else
    {
      /* for a typeinfo */
      result=ITypeLib2_GetTypeInfo(iface, index, &pTInfo);

      if(SUCCEEDED(result))
      {
        ITypeInfo2 * pTInfo2;
        result = ITypeInfo_QueryInterface(pTInfo,
                                          &IID_ITypeInfo2,
                                          (LPVOID*) &pTInfo2);

        if(SUCCEEDED(result))
        {
          result = ITypeInfo2_GetDocumentation2(pTInfo2,
                                           MEMBERID_NIL,
                                           lcid,
                                           pbstrHelpString,
                                           pdwHelpStringContext,
                                           pbstrHelpStringDll);

          ITypeInfo2_Release(pTInfo2);
        }

        ITypeInfo_Release(pTInfo);
      }
    }
    return result;
}

static HRESULT TLB_copy_all_custdata(const struct list *custdata_list, CUSTDATA *pCustData)
{
    TLBCustData *pCData;
    unsigned int ct;
    CUSTDATAITEM *cdi;
    HRESULT hr = S_OK;

    ct = list_count(custdata_list);

    pCustData->prgCustData = CoTaskMemAlloc(ct * sizeof(CUSTDATAITEM));
    if(!pCustData->prgCustData)
        return E_OUTOFMEMORY;

    pCustData->cCustData = ct;

    cdi = pCustData->prgCustData;
    LIST_FOR_EACH_ENTRY(pCData, custdata_list, TLBCustData, entry){
        cdi->guid = *TLB_get_guid_null(pCData->guid);
        VariantInit(&cdi->varValue);
        hr = VariantCopy(&cdi->varValue, &pCData->data);
        if(FAILED(hr)) break;
        ++cdi;
    }

    return hr;
}


/* ITypeLib2::GetAllCustData
 *
 * Gets all custom data items for the library.
 *
 */
static HRESULT WINAPI ITypeLib2_fnGetAllCustData(
	ITypeLib2 * iface,
        CUSTDATA *pCustData)
{
    ITypeLibImpl *This = impl_from_ITypeLib2(iface);
    TRACE("(%p)->(%p)\n", This, pCustData);
    return TLB_copy_all_custdata(&This->custdata_list, pCustData);
}

static const ITypeLib2Vtbl tlbvt = {
    ITypeLib2_fnQueryInterface,
    ITypeLib2_fnAddRef,
    ITypeLib2_fnRelease,
    ITypeLib2_fnGetTypeInfoCount,
    ITypeLib2_fnGetTypeInfo,
    ITypeLib2_fnGetTypeInfoType,
    ITypeLib2_fnGetTypeInfoOfGuid,
    ITypeLib2_fnGetLibAttr,
    ITypeLib2_fnGetTypeComp,
    ITypeLib2_fnGetDocumentation,
    ITypeLib2_fnIsName,
    ITypeLib2_fnFindName,
    ITypeLib2_fnReleaseTLibAttr,

    ITypeLib2_fnGetCustData,
    ITypeLib2_fnGetLibStatistics,
    ITypeLib2_fnGetDocumentation2,
    ITypeLib2_fnGetAllCustData
 };


static HRESULT WINAPI ITypeLibComp_fnQueryInterface(ITypeComp * iface, REFIID riid, LPVOID * ppv)
{
    ITypeLibImpl *This = impl_from_ITypeComp(iface);

    return ITypeLib2_QueryInterface(&This->ITypeLib2_iface, riid, ppv);
}

static ULONG WINAPI ITypeLibComp_fnAddRef(ITypeComp * iface)
{
    ITypeLibImpl *This = impl_from_ITypeComp(iface);

    return ITypeLib2_AddRef(&This->ITypeLib2_iface);
}

static ULONG WINAPI ITypeLibComp_fnRelease(ITypeComp * iface)
{
    ITypeLibImpl *This = impl_from_ITypeComp(iface);

    return ITypeLib2_Release(&This->ITypeLib2_iface);
}

static HRESULT WINAPI ITypeLibComp_fnBind(
    ITypeComp * iface,
    OLECHAR * szName,
    ULONG lHash,
    WORD wFlags,
    ITypeInfo ** ppTInfo,
    DESCKIND * pDescKind,
    BINDPTR * pBindPtr)
{
    ITypeLibImpl *This = impl_from_ITypeComp(iface);
    BOOL typemismatch = FALSE;
    int i;

    TRACE("%p, %s, %#lx, %#x, %p, %p, %p.\n", iface, debugstr_w(szName), lHash, wFlags, ppTInfo, pDescKind, pBindPtr);

    *pDescKind = DESCKIND_NONE;
    pBindPtr->lptcomp = NULL;
    *ppTInfo = NULL;

    for(i = 0; i < This->TypeInfoCount; ++i){
        ITypeInfoImpl *pTypeInfo = This->typeinfos[i];
        TRACE("testing %s\n", debugstr_w(TLB_get_bstr(pTypeInfo->Name)));

        /* FIXME: check wFlags here? */
        /* FIXME: we should use a hash table to look this info up using lHash
         * instead of an O(n) search */
        if ((pTypeInfo->typeattr.typekind == TKIND_ENUM) ||
            (pTypeInfo->typeattr.typekind == TKIND_MODULE))
        {
            if (pTypeInfo->Name && !wcscmp(pTypeInfo->Name->str, szName))
            {
                *pDescKind = DESCKIND_TYPECOMP;
                pBindPtr->lptcomp = &pTypeInfo->ITypeComp_iface;
                ITypeComp_AddRef(pBindPtr->lptcomp);
                TRACE("module or enum: %s\n", debugstr_w(szName));
                return S_OK;
            }
        }

        if ((pTypeInfo->typeattr.typekind == TKIND_MODULE) ||
            (pTypeInfo->typeattr.typekind == TKIND_ENUM))
        {
            ITypeComp *pSubTypeComp = &pTypeInfo->ITypeComp_iface;
            HRESULT hr;

            hr = ITypeComp_Bind(pSubTypeComp, szName, lHash, wFlags, ppTInfo, pDescKind, pBindPtr);
            if (SUCCEEDED(hr) && (*pDescKind != DESCKIND_NONE))
            {
                TRACE("found in module or in enum: %s\n", debugstr_w(szName));
                return S_OK;
            }
            else if (hr == TYPE_E_TYPEMISMATCH)
                typemismatch = TRUE;
        }

        if ((pTypeInfo->typeattr.typekind == TKIND_COCLASS) &&
            (pTypeInfo->typeattr.wTypeFlags & TYPEFLAG_FAPPOBJECT))
        {
            ITypeComp *pSubTypeComp = &pTypeInfo->ITypeComp_iface;
            HRESULT hr;
            ITypeInfo *subtypeinfo;
            BINDPTR subbindptr;
            DESCKIND subdesckind;

            hr = ITypeComp_Bind(pSubTypeComp, szName, lHash, wFlags,
                &subtypeinfo, &subdesckind, &subbindptr);
            if (SUCCEEDED(hr) && (subdesckind != DESCKIND_NONE))
            {
                TYPEDESC tdesc_appobject;
                const VARDESC vardesc_appobject =
                {
                    -2,         /* memid */
                    NULL,       /* lpstrSchema */
                    {
                        0       /* oInst */
                    },
                    {
                                /* ELEMDESC */
                        {
                                /* TYPEDESC */
                                {
                                    &tdesc_appobject
                                },
                                VT_PTR
                        },
                    },
                    0,          /* wVarFlags */
                    VAR_STATIC  /* varkind */
                };

                tdesc_appobject.hreftype = pTypeInfo->hreftype;
                tdesc_appobject.vt = VT_USERDEFINED;

                TRACE("found in implicit app object: %s\n", debugstr_w(szName));

                /* cleanup things filled in by Bind call so we can put our
                 * application object data in there instead */
                switch (subdesckind)
                {
                case DESCKIND_FUNCDESC:
                    ITypeInfo_ReleaseFuncDesc(subtypeinfo, subbindptr.lpfuncdesc);
                    break;
                case DESCKIND_VARDESC:
                    ITypeInfo_ReleaseVarDesc(subtypeinfo, subbindptr.lpvardesc);
                    break;
                default:
                    break;
                }
                if (subtypeinfo) ITypeInfo_Release(subtypeinfo);

                if (pTypeInfo->hreftype == -1)
                    FIXME("no hreftype for interface %p\n", pTypeInfo);

                hr = TLB_AllocAndInitVarDesc(&vardesc_appobject, &pBindPtr->lpvardesc);
                if (FAILED(hr))
                    return hr;

                *pDescKind = DESCKIND_IMPLICITAPPOBJ;
                *ppTInfo = (ITypeInfo *)&pTypeInfo->ITypeInfo2_iface;
                ITypeInfo_AddRef(*ppTInfo);
                return S_OK;
            }
            else if (hr == TYPE_E_TYPEMISMATCH)
                typemismatch = TRUE;
        }
    }

    if (typemismatch)
    {
        TRACE("type mismatch %s\n", debugstr_w(szName));
        return TYPE_E_TYPEMISMATCH;
    }
    else
    {
        TRACE("name not found %s\n", debugstr_w(szName));
        return S_OK;
    }
}

static HRESULT WINAPI ITypeLibComp_fnBindType(
    ITypeComp * iface,
    OLECHAR * szName,
    ULONG lHash,
    ITypeInfo ** ppTInfo,
    ITypeComp ** ppTComp)
{
    ITypeLibImpl *This = impl_from_ITypeComp(iface);
    ITypeInfoImpl *info;

    TRACE("%p, %s, %#lx, %p, %p.\n", iface, debugstr_w(szName), lHash, ppTInfo, ppTComp);

    if(!szName || !ppTInfo || !ppTComp)
        return E_INVALIDARG;

    info = TLB_get_typeinfo_by_name(This, szName);
    if(!info){
        *ppTInfo = NULL;
        *ppTComp = NULL;
        return S_OK;
    }

    *ppTInfo = (ITypeInfo *)&info->ITypeInfo2_iface;
    ITypeInfo_AddRef(*ppTInfo);
    *ppTComp = &info->ITypeComp_iface;
    ITypeComp_AddRef(*ppTComp);

    return S_OK;
}

static const ITypeCompVtbl tlbtcvt =
{

    ITypeLibComp_fnQueryInterface,
    ITypeLibComp_fnAddRef,
    ITypeLibComp_fnRelease,

    ITypeLibComp_fnBind,
    ITypeLibComp_fnBindType
};

/*================== ITypeInfo(2) Methods ===================================*/
static ITypeInfoImpl* ITypeInfoImpl_Constructor(void)
{
    ITypeInfoImpl *pTypeInfoImpl;

    pTypeInfoImpl = calloc(1, sizeof(ITypeInfoImpl));
    if (pTypeInfoImpl)
    {
      pTypeInfoImpl->ITypeInfo2_iface.lpVtbl = &tinfvt;
      pTypeInfoImpl->ITypeComp_iface.lpVtbl = &tcompvt;
      pTypeInfoImpl->ICreateTypeInfo2_iface.lpVtbl = &CreateTypeInfo2Vtbl;
      pTypeInfoImpl->ref = 0;
      pTypeInfoImpl->hreftype = -1;
      pTypeInfoImpl->typeattr.memidConstructor = MEMBERID_NIL;
      pTypeInfoImpl->typeattr.memidDestructor = MEMBERID_NIL;
      pTypeInfoImpl->pcustdata_list = &pTypeInfoImpl->custdata_list;
      list_init(pTypeInfoImpl->pcustdata_list);
    }
    TRACE("(%p)\n", pTypeInfoImpl);
    return pTypeInfoImpl;
}

/* ITypeInfo::QueryInterface
 */
static HRESULT WINAPI ITypeInfo_fnQueryInterface(
	ITypeInfo2 *iface,
	REFIID riid,
	VOID **ppvObject)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);

    TRACE("(%p)->(IID: %s)\n",This,debugstr_guid(riid));

    *ppvObject=NULL;
    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid,&IID_ITypeInfo)||
            IsEqualIID(riid,&IID_ITypeInfo2))
        *ppvObject = &This->ITypeInfo2_iface;
    else if(IsEqualIID(riid, &IID_ICreateTypeInfo) ||
             IsEqualIID(riid, &IID_ICreateTypeInfo2))
        *ppvObject = &This->ICreateTypeInfo2_iface;
    else if(IsEqualIID(riid, &IID_ITypeComp))
        *ppvObject = &This->ITypeComp_iface;

    if(*ppvObject){
        IUnknown_AddRef((IUnknown*)*ppvObject);
        TRACE("-- Interface: (%p)->(%p)\n",ppvObject,*ppvObject);
        return S_OK;
    }
    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ITypeInfo_fnAddRef( ITypeInfo2 *iface)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (ref == 1 /* incremented from 0 */)
        ITypeLib2_AddRef(&This->pTypeLib->ITypeLib2_iface);

    return ref;
}

static void typeinfo_release_funcdesc(TLBFuncDesc *func)
{
    unsigned int i;

    for (i = 0; i < func->funcdesc.cParams; ++i)
    {
        ELEMDESC *elemdesc = &func->funcdesc.lprgelemdescParam[i];
        if (elemdesc->paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT)
            VariantClear(&elemdesc->paramdesc.pparamdescex->varDefaultValue);
        TLB_FreeCustData(&func->pParamDesc[i].custdata_list);
    }
    free(func->funcdesc.lprgelemdescParam);
    free(func->pParamDesc);
    TLB_FreeCustData(&func->custdata_list);
}

static void ITypeInfoImpl_Destroy(ITypeInfoImpl *This)
{
    UINT i;

    TRACE("destroying ITypeInfo(%p)\n",This);

    for (i = 0; i < This->typeattr.cFuncs; ++i)
    {
        typeinfo_release_funcdesc(&This->funcdescs[i]);
    }
    free(This->funcdescs);

    for(i = 0; i < This->typeattr.cVars; ++i)
    {
        TLBVarDesc *pVInfo = &This->vardescs[i];
        if (pVInfo->vardesc_create) {
            TLB_FreeVarDesc(pVInfo->vardesc_create);
        } else if (pVInfo->vardesc.varkind == VAR_CONST) {
            VariantClear(pVInfo->vardesc.lpvarValue);
            free(pVInfo->vardesc.lpvarValue);
        }
        TLB_FreeCustData(&pVInfo->custdata_list);
    }
    free(This->vardescs);

    if(This->impltypes){
        for (i = 0; i < This->typeattr.cImplTypes; ++i){
            TLBImplType *pImpl = &This->impltypes[i];
            TLB_FreeCustData(&pImpl->custdata_list);
        }
        free(This->impltypes);
    }

    TLB_FreeCustData(&This->custdata_list);

    free(This);
}

static ULONG WINAPI ITypeInfo_fnRelease(ITypeInfo2 *iface)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        BOOL not_attached_to_typelib = This->not_attached_to_typelib;
        ITypeLib2_Release(&This->pTypeLib->ITypeLib2_iface);
        if (not_attached_to_typelib)
            free(This);
        /* otherwise This will be freed when typelib is freed */
    }

    return ref;
}

static HRESULT WINAPI ITypeInfo_fnGetTypeAttr( ITypeInfo2 *iface,
        LPTYPEATTR  *ppTypeAttr)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    SIZE_T size;

    TRACE("(%p)\n",This);

    size = sizeof(**ppTypeAttr);
    if (This->typeattr.typekind == TKIND_ALIAS && This->tdescAlias)
        size += TLB_SizeTypeDesc(This->tdescAlias, FALSE);

    *ppTypeAttr = malloc(size);
    if (!*ppTypeAttr)
        return E_OUTOFMEMORY;

    **ppTypeAttr = This->typeattr;
    (*ppTypeAttr)->guid = *TLB_get_guid_null(This->guid);

    if (This->tdescAlias)
        TLB_CopyTypeDesc(&(*ppTypeAttr)->tdescAlias, This->tdescAlias, *ppTypeAttr + 1);

    if((*ppTypeAttr)->typekind == TKIND_DISPATCH) {
        /* This should include all the inherited funcs */
        (*ppTypeAttr)->cFuncs = (*ppTypeAttr)->cbSizeVft / This->pTypeLib->ptr_size;
        /* This is always the size of IDispatch's vtbl */
        (*ppTypeAttr)->cbSizeVft = sizeof(IDispatchVtbl);
        (*ppTypeAttr)->wTypeFlags &= ~TYPEFLAG_FOLEAUTOMATION;
    }
    return S_OK;
}

/* ITypeInfo::GetTypeComp
 *
 * Retrieves the ITypeComp interface for the type description, which enables a
 * client compiler to bind to the type description's members.
 *
 */
static HRESULT WINAPI ITypeInfo_fnGetTypeComp( ITypeInfo2 *iface,
        ITypeComp  * *ppTComp)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);

    TRACE("(%p)->(%p)\n", This, ppTComp);

    *ppTComp = &This->ITypeComp_iface;
    ITypeComp_AddRef(*ppTComp);
    return S_OK;
}

static SIZE_T TLB_SizeElemDesc( const ELEMDESC *elemdesc )
{
    SIZE_T size = TLB_SizeTypeDesc(&elemdesc->tdesc, FALSE);
    if (elemdesc->paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT)
        size += sizeof(*elemdesc->paramdesc.pparamdescex);
    return size;
}

static HRESULT TLB_CopyElemDesc( const ELEMDESC *src, ELEMDESC *dest, char **buffer )
{
    *dest = *src;
    *buffer = TLB_CopyTypeDesc(&dest->tdesc, &src->tdesc, *buffer);
    if (src->paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT)
    {
        const PARAMDESCEX *pparamdescex_src = src->paramdesc.pparamdescex;
        PARAMDESCEX *pparamdescex_dest = dest->paramdesc.pparamdescex = (PARAMDESCEX *)*buffer;
        *buffer += sizeof(PARAMDESCEX);
        *pparamdescex_dest = *pparamdescex_src;
        pparamdescex_dest->cBytes = sizeof(PARAMDESCEX);
        VariantInit(&pparamdescex_dest->varDefaultValue);
        return VariantCopy(&pparamdescex_dest->varDefaultValue, 
                           (VARIANTARG *)&pparamdescex_src->varDefaultValue);
    }
    else
        dest->paramdesc.pparamdescex = NULL;
    return S_OK;
}

static HRESULT TLB_SanitizeVariant(VARIANT *var)
{
    if (V_VT(var) == VT_INT)
        return VariantChangeType(var, var, 0, VT_I4);
    else if (V_VT(var) == VT_UINT)
        return VariantChangeType(var, var, 0, VT_UI4);

    return S_OK;
}

static void TLB_FreeElemDesc( ELEMDESC *elemdesc )
{
    if (elemdesc->paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT)
        VariantClear(&elemdesc->paramdesc.pparamdescex->varDefaultValue);
}

static HRESULT TLB_AllocAndInitFuncDesc( const FUNCDESC *src, FUNCDESC **dest_ptr, BOOL dispinterface )
{
    FUNCDESC *dest;
    char *buffer;
    SIZE_T size = sizeof(*src);
    SHORT i;
    HRESULT hr;

    size += sizeof(*src->lprgscode) * src->cScodes;
    size += TLB_SizeElemDesc(&src->elemdescFunc);
    for (i = 0; i < src->cParams; i++)
    {
        size += sizeof(ELEMDESC);
        size += TLB_SizeElemDesc(&src->lprgelemdescParam[i]);
    }

    dest = (FUNCDESC *)SysAllocStringByteLen(NULL, size);
    if (!dest) return E_OUTOFMEMORY;

    *dest = *src;
    if (dispinterface)    /* overwrite funckind */
        dest->funckind = FUNC_DISPATCH;
    buffer = (char *)(dest + 1);

    dest->oVft = dest->oVft & 0xFFFC;

    if (dest->cScodes) {
        dest->lprgscode = (SCODE *)buffer;
        memcpy(dest->lprgscode, src->lprgscode, sizeof(*src->lprgscode) * src->cScodes);
        buffer += sizeof(*src->lprgscode) * src->cScodes;
    } else
        dest->lprgscode = NULL;

    hr = TLB_CopyElemDesc(&src->elemdescFunc, &dest->elemdescFunc, &buffer);
    if (FAILED(hr))
    {
        SysFreeString((BSTR)dest);
        return hr;
    }

    if (dest->cParams) {
        dest->lprgelemdescParam = (ELEMDESC *)buffer;
        buffer += sizeof(ELEMDESC) * src->cParams;
        for (i = 0; i < src->cParams; i++)
        {
            hr = TLB_CopyElemDesc(&src->lprgelemdescParam[i], &dest->lprgelemdescParam[i], &buffer);
            if (FAILED(hr))
                break;
        }
        if (FAILED(hr))
        {
            /* undo the above actions */
            for (i = i - 1; i >= 0; i--)
                TLB_FreeElemDesc(&dest->lprgelemdescParam[i]);
            TLB_FreeElemDesc(&dest->elemdescFunc);
            SysFreeString((BSTR)dest);
            return hr;
        }
    } else
        dest->lprgelemdescParam = NULL;

    /* special treatment for dispinterface FUNCDESC based on an interface FUNCDESC.
     * This accounts for several arguments that are separate in the signature of
     * IDispatch::Invoke, rather than passed in DISPPARAMS::rgvarg[] */
    if (dispinterface && (src->funckind != FUNC_DISPATCH))
    {
        /* functions that have a [retval] parameter return this value into pVarResult.
         * [retval] is always the last parameter (if present) */
        if (dest->cParams &&
            (dest->lprgelemdescParam[dest->cParams - 1].paramdesc.wParamFlags & PARAMFLAG_FRETVAL))
        {
            ELEMDESC *elemdesc = &dest->lprgelemdescParam[dest->cParams - 1];
            if (elemdesc->tdesc.vt != VT_PTR)
            {
                ERR("elemdesc should have started with VT_PTR instead of:\n");
                if (ERR_ON(ole))
                    dump_ELEMDESC(elemdesc);
                return E_UNEXPECTED;
            }

            /* the type pointed to by this [retval] becomes elemdescFunc,
             * i.e. the function signature's return type.
             * We are using a flat buffer so there is no danger of leaking memory */
            dest->elemdescFunc.tdesc = *elemdesc->tdesc.lptdesc;

            /* remove the last parameter */
            dest->cParams--;
        }
        else if (dest->elemdescFunc.tdesc.vt == VT_HRESULT)
            /* Even if not otherwise replaced HRESULT is returned in pExcepInfo->scode,
             * not pVarResult.  So the function signature should show no return value. */
            dest->elemdescFunc.tdesc.vt = VT_VOID;

        /* The now-last (except [retval], removed above) parameter might be labeled [lcid].
         * If so it will be supplied from Invoke(lcid), so also not via DISPPARAMS::rgvarg */
        if (dest->cParams && (dest->lprgelemdescParam[dest->cParams - 1].paramdesc.wParamFlags & PARAMFLAG_FLCID))
            dest->cParams--;
    }

    *dest_ptr = dest;
    return S_OK;
}

static void TLB_FreeVarDesc(VARDESC *var_desc)
{
    TLB_FreeElemDesc(&var_desc->elemdescVar);
    if (var_desc->varkind == VAR_CONST)
        VariantClear(var_desc->lpvarValue);
    SysFreeString((BSTR)var_desc);
}

/* internal function to make the inherited interfaces' methods appear
 * part of the interface */
static HRESULT ITypeInfoImpl_GetInternalDispatchFuncDesc( ITypeInfo *iface,
    UINT index, const TLBFuncDesc **ppFuncDesc, UINT *funcs, UINT *hrefoffset)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo(iface);
    HRESULT hr;
    UINT implemented_funcs = 0;

    if (funcs)
        *funcs = 0;
    else
        *hrefoffset = DISPATCH_HREF_OFFSET;

    if(This->impltypes)
    {
        ITypeInfo *pSubTypeInfo;
        UINT sub_funcs;

        hr = ITypeInfo_GetRefTypeInfo(iface, This->impltypes[0].hRef, &pSubTypeInfo);
        if (FAILED(hr))
            return hr;

        hr = ITypeInfoImpl_GetInternalDispatchFuncDesc(pSubTypeInfo,
                                                       index,
                                                       ppFuncDesc,
                                                       &sub_funcs, hrefoffset);
        implemented_funcs += sub_funcs;
        ITypeInfo_Release(pSubTypeInfo);
        if (SUCCEEDED(hr))
            return hr;
        *hrefoffset += DISPATCH_HREF_OFFSET;
    }

    if (funcs)
        *funcs = implemented_funcs + This->typeattr.cFuncs;
    else
        *hrefoffset = 0;
    
    if (index < implemented_funcs)
        return E_INVALIDARG;
    index -= implemented_funcs;

    if (index >= This->typeattr.cFuncs)
        return TYPE_E_ELEMENTNOTFOUND;

    *ppFuncDesc = &This->funcdescs[index];
    return S_OK;
}

static HRESULT ITypeInfoImpl_GetInternalFuncDesc( ITypeInfo *iface, UINT index, const TLBFuncDesc **func_desc, UINT *hrefoffset )
{
    ITypeInfoImpl *This = impl_from_ITypeInfo(iface);

    if (This->typeattr.typekind == TKIND_DISPATCH)
        return ITypeInfoImpl_GetInternalDispatchFuncDesc(iface, index, func_desc, NULL, hrefoffset);

    if (index >= This->typeattr.cFuncs)
        return TYPE_E_ELEMENTNOTFOUND;

    *func_desc = &This->funcdescs[index];
    return S_OK;
}

static inline void ITypeInfoImpl_ElemDescAddHrefOffset( LPELEMDESC pElemDesc, UINT hrefoffset)
{
    TYPEDESC *pTypeDesc = &pElemDesc->tdesc;
    while (TRUE)
    {
        switch (pTypeDesc->vt)
        {
        case VT_USERDEFINED:
            pTypeDesc->hreftype += hrefoffset;
            return;
        case VT_PTR:
        case VT_SAFEARRAY:
            pTypeDesc = pTypeDesc->lptdesc;
            break;
        case VT_CARRAY:
            pTypeDesc = &pTypeDesc->lpadesc->tdescElem;
            break;
        default:
            return;
        }
    }
}

static inline void ITypeInfoImpl_FuncDescAddHrefOffset( LPFUNCDESC pFuncDesc, UINT hrefoffset)
{
    SHORT i;
    for (i = 0; i < pFuncDesc->cParams; i++)
        ITypeInfoImpl_ElemDescAddHrefOffset(&pFuncDesc->lprgelemdescParam[i], hrefoffset);
    ITypeInfoImpl_ElemDescAddHrefOffset(&pFuncDesc->elemdescFunc, hrefoffset);
}

/* ITypeInfo::GetFuncDesc
 *
 * Retrieves the FUNCDESC structure that contains information about a
 * specified function.
 *
 */
static HRESULT WINAPI ITypeInfo_fnGetFuncDesc( ITypeInfo2 *iface, UINT index,
        LPFUNCDESC  *ppFuncDesc)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    const TLBFuncDesc *internal_funcdesc;
    HRESULT hr;
    UINT hrefoffset = 0;

    TRACE("(%p) index %d\n", This, index);

    if (!ppFuncDesc)
        return E_INVALIDARG;

    if (This->needs_layout)
        ICreateTypeInfo2_LayOut(&This->ICreateTypeInfo2_iface);

    hr = ITypeInfoImpl_GetInternalFuncDesc((ITypeInfo *)iface, index,
                                           &internal_funcdesc, &hrefoffset);
    if (FAILED(hr))
    {
        WARN("description for function %d not found\n", index);
        return hr;
    }

    hr = TLB_AllocAndInitFuncDesc(
        &internal_funcdesc->funcdesc,
        ppFuncDesc,
        This->typeattr.typekind == TKIND_DISPATCH);

    if ((This->typeattr.typekind == TKIND_DISPATCH) && hrefoffset)
        ITypeInfoImpl_FuncDescAddHrefOffset(*ppFuncDesc, hrefoffset);

    TRACE("-- %#lx.\n", hr);
    return hr;
}

static HRESULT TLB_AllocAndInitVarDesc( const VARDESC *src, VARDESC **dest_ptr )
{
    VARDESC *dest;
    char *buffer;
    SIZE_T size = sizeof(*src);
    HRESULT hr;

    if (src->lpstrSchema) size += (lstrlenW(src->lpstrSchema) + 1) * sizeof(WCHAR);
    if (src->varkind == VAR_CONST)
        size += sizeof(VARIANT);
    size += TLB_SizeElemDesc(&src->elemdescVar);

    dest = (VARDESC *)SysAllocStringByteLen(NULL, size);
    if (!dest) return E_OUTOFMEMORY;

    *dest = *src;
    buffer = (char *)(dest + 1);
    if (src->lpstrSchema)
    {
        int len;
        dest->lpstrSchema = (LPOLESTR)buffer;
        len = lstrlenW(src->lpstrSchema);
        memcpy(dest->lpstrSchema, src->lpstrSchema, (len + 1) * sizeof(WCHAR));
        buffer += (len + 1) * sizeof(WCHAR);
    }

    if (src->varkind == VAR_CONST)
    {
        HRESULT hr;

        dest->lpvarValue = (VARIANT *)buffer;
        *dest->lpvarValue = *src->lpvarValue;
        buffer += sizeof(VARIANT);
        VariantInit(dest->lpvarValue);
        hr = VariantCopy(dest->lpvarValue, src->lpvarValue);
        if (FAILED(hr))
        {
            SysFreeString((BSTR)dest);
            return hr;
        }
    }
    hr = TLB_CopyElemDesc(&src->elemdescVar, &dest->elemdescVar, &buffer);
    if (FAILED(hr))
    {
        if (src->varkind == VAR_CONST)
            VariantClear(dest->lpvarValue);
        SysFreeString((BSTR)dest);
        return hr;
    }
    *dest_ptr = dest;
    return S_OK;
}

/* ITypeInfo::GetVarDesc
 *
 * Retrieves a VARDESC structure that describes the specified variable.
 *
 */
static HRESULT WINAPI ITypeInfo_fnGetVarDesc( ITypeInfo2 *iface, UINT index,
        LPVARDESC  *ppVarDesc)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    const TLBVarDesc *pVDesc = &This->vardescs[index];

    TRACE("(%p) index %d\n", This, index);

    if(index >= This->typeattr.cVars)
        return TYPE_E_ELEMENTNOTFOUND;

    if (This->needs_layout)
        ICreateTypeInfo2_LayOut(&This->ICreateTypeInfo2_iface);

    return TLB_AllocAndInitVarDesc(&pVDesc->vardesc, ppVarDesc);
}

/* internal function to make the inherited interfaces' methods appear
 * part of the interface, remembering if the top-level was dispinterface */
static HRESULT typeinfo_getnames( ITypeInfo *iface, MEMBERID memid, BSTR *names,
                                  UINT max_names, UINT *num_names, BOOL dispinterface)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo(iface);
    const TLBFuncDesc *func_desc;
    const TLBVarDesc *var_desc;
    int i;

    *num_names = 0;

    func_desc = TLB_get_funcdesc_by_memberid(This, memid);
    if (func_desc)
    {
        UINT params = func_desc->funcdesc.cParams;
        if (!max_names || !func_desc->Name)
            return S_OK;

        *names = SysAllocString(TLB_get_bstr(func_desc->Name));
        ++(*num_names);

        if (dispinterface && (func_desc->funcdesc.funckind != FUNC_DISPATCH))
        {
            /* match the rewriting of special trailing parameters in TLB_AllocAndInitFuncDesc */
            if ((params > 0) && (func_desc->funcdesc.lprgelemdescParam[params - 1].paramdesc.wParamFlags & PARAMFLAG_FRETVAL))
                --params; /* Invoke(pVarResult) supplies the [retval] parameter, so it's hidden from DISPPARAMS */
            if ((params > 0) && (func_desc->funcdesc.lprgelemdescParam[params - 1].paramdesc.wParamFlags & PARAMFLAG_FLCID))
                --params; /* Invoke(lcid) supplies the [lcid] parameter, so it's hidden from DISPPARAMS */
        }

        for (i = 0; i < params; i++)
        {
            if (*num_names >= max_names || !func_desc->pParamDesc[i].Name)
                return S_OK;
            names[*num_names] = SysAllocString(TLB_get_bstr(func_desc->pParamDesc[i].Name));
            ++(*num_names);
        }
        return S_OK;
    }

    var_desc = TLB_get_vardesc_by_memberid(This, memid);
    if (var_desc)
    {
        *names = SysAllocString(TLB_get_bstr(var_desc->Name));
        *num_names = 1;
    }
    else
    {
        if (This->impltypes &&
            (This->typeattr.typekind == TKIND_INTERFACE || This->typeattr.typekind == TKIND_DISPATCH))
        {
            /* recursive search */
            ITypeInfo *parent;
            HRESULT result;
            result = ITypeInfo_GetRefTypeInfo(iface, This->impltypes[0].hRef, &parent);
            if (SUCCEEDED(result))
            {
                result = typeinfo_getnames(parent, memid, names, max_names, num_names, dispinterface);
                ITypeInfo_Release(parent);
                return result;
            }
            WARN("Could not search inherited interface!\n");
        }
        else
	{
            WARN("no names found\n");
	}
        *num_names = 0;
        return TYPE_E_ELEMENTNOTFOUND;
    }
    return S_OK;
}

/* ITypeInfo_GetNames
 *
 * Retrieves the variable with the specified member ID (or the name of the
 * property or method and its parameters) that correspond to the specified
 * function ID.
 */
static HRESULT WINAPI ITypeInfo_fnGetNames( ITypeInfo2 *iface, MEMBERID memid,
                                            BSTR *names, UINT max_names, UINT *num_names)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);

    TRACE("%p, %#lx, %p, %d, %p\n", iface, memid, names, max_names, num_names);

    if (!names) return E_INVALIDARG;

    return typeinfo_getnames((ITypeInfo *)iface, memid, names, max_names, num_names,
                             This->typeattr.typekind == TKIND_DISPATCH);
}

/* ITypeInfo::GetRefTypeOfImplType
 *
 * If a type description describes a COM class, it retrieves the type
 * description of the implemented interface types. For an interface,
 * GetRefTypeOfImplType returns the type information for inherited interfaces,
 * if any exist.
 *
 */
static HRESULT WINAPI ITypeInfo_fnGetRefTypeOfImplType(
	ITypeInfo2 *iface,
        UINT index,
	HREFTYPE  *pRefType)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    HRESULT hr = S_OK;

    TRACE("(%p) index %d\n", This, index);
    if (TRACE_ON(ole)) dump_TypeInfo(This);

    if(index==(UINT)-1)
    {
      /* only valid on dual interfaces;
         retrieve the associated TKIND_INTERFACE handle for the current TKIND_DISPATCH
      */

      if (This->typeattr.wTypeFlags & TYPEFLAG_FDUAL)
      {
          *pRefType = -2;
      }
      else
      {
        hr = TYPE_E_ELEMENTNOTFOUND;
      }
    }
    else if(index == 0 && This->typeattr.typekind == TKIND_DISPATCH)
    {
      /* All TKIND_DISPATCHs are made to look like they inherit from IDispatch */
      *pRefType = This->pTypeLib->dispatch_href;
    }
    else
    {
        if(index >= This->typeattr.cImplTypes)
            hr = TYPE_E_ELEMENTNOTFOUND;
        else{
            *pRefType = This->impltypes[index].hRef;
            if (This->typeattr.typekind == TKIND_INTERFACE)
                *pRefType |= 0x2;
        }
    }

    if(TRACE_ON(ole))
    {
        if(SUCCEEDED(hr))
            TRACE("SUCCESS -- hRef %#lx.\n", *pRefType );
        else
            TRACE("FAILURE -- hresult %#lx.\n", hr);
    }

    return hr;
}

/* ITypeInfo::GetImplTypeFlags
 *
 * Retrieves the IMPLTYPEFLAGS enumeration for one implemented interface
 * or base interface in a type description.
 */
static HRESULT WINAPI ITypeInfo_fnGetImplTypeFlags( ITypeInfo2 *iface,
        UINT index, INT  *pImplTypeFlags)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);

    TRACE("(%p) index %d\n", This, index);

    if(!pImplTypeFlags)
        return E_INVALIDARG;

    if(This->typeattr.typekind == TKIND_DISPATCH && index == 0){
        *pImplTypeFlags = 0;
        return S_OK;
    }

    if(index >= This->typeattr.cImplTypes)
        return TYPE_E_ELEMENTNOTFOUND;

    *pImplTypeFlags = This->impltypes[index].implflags;

    return S_OK;
}

/* GetIDsOfNames
 * Maps between member names and member IDs, and parameter names and
 * parameter IDs.
 */
static HRESULT WINAPI ITypeInfo_fnGetIDsOfNames( ITypeInfo2 *iface,
        LPOLESTR  *rgszNames, UINT cNames, MEMBERID  *pMemId)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    const TLBVarDesc *pVDesc;
    HRESULT ret=S_OK;
    UINT i, fdc;

    TRACE("%p, %s, %d.\n", iface, debugstr_w(*rgszNames), cNames);

    /* init out parameters in case of failure */
    for (i = 0; i < cNames; i++)
        pMemId[i] = MEMBERID_NIL;

    for (fdc = 0; fdc < This->typeattr.cFuncs; ++fdc) {
        int j;
        const TLBFuncDesc *pFDesc = &This->funcdescs[fdc];
        if(!lstrcmpiW(*rgszNames, TLB_get_bstr(pFDesc->Name))) {
            if(cNames) *pMemId=pFDesc->funcdesc.memid;
            for(i=1; i < cNames; i++){
                for(j=0; j<pFDesc->funcdesc.cParams; j++)
                    if(!lstrcmpiW(rgszNames[i],TLB_get_bstr(pFDesc->pParamDesc[j].Name)))
                            break;
                if( j<pFDesc->funcdesc.cParams)
                    pMemId[i]=j;
                else
                   ret=DISP_E_UNKNOWNNAME;
            };
            TRACE("-- %#lx.\n", ret);
            return ret;
        }
    }
    pVDesc = TLB_get_vardesc_by_name(This, *rgszNames);
    if(pVDesc){
        if(cNames)
            *pMemId = pVDesc->vardesc.memid;
        return ret;
    }
    /* not found, see if it can be found in an inherited interface */
    if(This->impltypes) {
        /* recursive search */
        ITypeInfo *pTInfo;
        ret = ITypeInfo2_GetRefTypeInfo(iface, This->impltypes[0].hRef, &pTInfo);
        if(SUCCEEDED(ret)){
            ret=ITypeInfo_GetIDsOfNames(pTInfo, rgszNames, cNames, pMemId );
            ITypeInfo_Release(pTInfo);
            return ret;
        }
        WARN("Could not search inherited interface!\n");
    } else
        WARN("no names found\n");
    return DISP_E_UNKNOWNNAME;
}


#ifdef __i386__

extern LONGLONG call_method( void *func, int nb_args, const DWORD *args, int *stack_offset );
extern double call_double_method( void *func, int nb_args, const DWORD *args, int *stack_offset );

HRESULT WINAPI DispCallFunc( void* pvInstance, ULONG_PTR oVft, CALLCONV cc, VARTYPE vtReturn,
                             UINT cActuals, VARTYPE* prgvt, VARIANTARG** prgpvarg, VARIANT* pvargResult )
{
    int argspos = 0, stack_offset;
    void *func;
    UINT i;
    DWORD *args;

    TRACE("(%p, %Id, %d, %d, %d, %p, %p, %p (vt=%d))\n",
        pvInstance, oVft, cc, vtReturn, cActuals, prgvt, prgpvarg,
        pvargResult, V_VT(pvargResult));

    if (cc != CC_STDCALL && cc != CC_CDECL)
    {
        FIXME("unsupported calling convention %d\n",cc);
        return E_INVALIDARG;
    }

    /* maximum size for an argument is sizeof(VARIANT) */
    args = malloc( sizeof(VARIANT) * cActuals + sizeof(DWORD) * 2 );

    if (pvInstance)
    {
        const FARPROC *vtable = *(FARPROC **)pvInstance;
        func = vtable[oVft/sizeof(void *)];
        args[argspos++] = (DWORD)pvInstance; /* the This pointer is always the first parameter */
    }
    else func = (void *)oVft;

    switch (vtReturn)
    {
    case VT_DECIMAL:
    case VT_VARIANT:
        args[argspos++] = (DWORD)pvargResult;  /* arg 0 is a pointer to the result */
        break;
    case VT_HRESULT:
        WARN("invalid return type %u\n", vtReturn);
        free( args );
        return E_INVALIDARG;
    default:
        break;
    }

    for (i = 0; i < cActuals; i++)
    {
        VARIANT *arg = prgpvarg[i];

        switch (prgvt[i])
        {
        case VT_EMPTY:
            break;
        case VT_I8:
        case VT_UI8:
        case VT_R8:
        case VT_DATE:
        case VT_CY:
            memcpy( &args[argspos], &V_I8(arg), sizeof(V_I8(arg)) );
            argspos += sizeof(V_I8(arg)) / sizeof(DWORD);
            break;
        case VT_DECIMAL:
        case VT_VARIANT:
            memcpy( &args[argspos], arg, sizeof(*arg) );
            argspos += sizeof(*arg) / sizeof(DWORD);
            break;
        case VT_BOOL:  /* VT_BOOL is 16-bit but BOOL is 32-bit, needs to be extended */
            args[argspos++] = V_BOOL(arg);
            break;
        default:
            args[argspos++] = V_UI4(arg);
            break;
        }
        TRACE("arg %u: type %s %s\n", i, debugstr_vt(prgvt[i]), debugstr_variant(arg));
    }

    switch (vtReturn)
    {
    case VT_EMPTY:
    case VT_DECIMAL:
    case VT_VARIANT:
        call_method( func, argspos, args, &stack_offset );
        break;
    case VT_R4:
        V_R4(pvargResult) = call_double_method( func, argspos, args, &stack_offset );
        break;
    case VT_R8:
    case VT_DATE:
        V_R8(pvargResult) = call_double_method( func, argspos, args, &stack_offset );
        break;
    case VT_I8:
    case VT_UI8:
    case VT_CY:
        V_UI8(pvargResult) = call_method( func, argspos, args, &stack_offset );
        break;
    default:
        V_UI4(pvargResult) = call_method( func, argspos, args, &stack_offset );
        break;
    }
    free( args );
    if (stack_offset && cc == CC_STDCALL)
    {
        WARN( "stack pointer off by %d\n", stack_offset );
        return DISP_E_BADCALLEE;
    }
    if (vtReturn != VT_VARIANT) V_VT(pvargResult) = vtReturn;
    TRACE("retval: %s\n", debugstr_variant(pvargResult));
    return S_OK;
}

#elif defined(__x86_64__)

extern DWORD_PTR CDECL call_method( void *func, int nb_args, const DWORD_PTR *args );
extern double CDECL call_double_method( void *func, int nb_args, const DWORD_PTR *args );

HRESULT WINAPI DispCallFunc( void* pvInstance, ULONG_PTR oVft, CALLCONV cc, VARTYPE vtReturn,
                             UINT cActuals, VARTYPE* prgvt, VARIANTARG** prgpvarg, VARIANT* pvargResult )
{
    int argspos = 0;
    UINT i;
    DWORD_PTR *args;
    void *func;

    TRACE("%p, %Id, %d, %d, %d, %p, %p, %p (vt=%d).\n",
          pvInstance, oVft, cc, vtReturn, cActuals, prgvt, prgpvarg,
          pvargResult, V_VT(pvargResult));

    if (cc != CC_STDCALL && cc != CC_CDECL)
    {
	FIXME("unsupported calling convention %d\n",cc);
        return E_INVALIDARG;
    }

    /* maximum size for an argument is sizeof(DWORD_PTR) */
    args = malloc( sizeof(DWORD_PTR) * (cActuals + 2) );

    if (pvInstance)
    {
        const FARPROC *vtable = *(FARPROC **)pvInstance;
        func = vtable[oVft/sizeof(void *)];
        args[argspos++] = (DWORD_PTR)pvInstance; /* the This pointer is always the first parameter */
    }
    else func = (void *)oVft;

    switch (vtReturn)
    {
    case VT_DECIMAL:
    case VT_VARIANT:
        args[argspos++] = (DWORD_PTR)pvargResult;  /* arg 0 is a pointer to the result */
        break;
    case VT_HRESULT:
        WARN("invalid return type %u\n", vtReturn);
        free( args );
        return E_INVALIDARG;
    default:
        break;
    }

    for (i = 0; i < cActuals; i++)
    {
        VARIANT *arg = prgpvarg[i];

        switch (prgvt[i])
        {
        case VT_DECIMAL:
        case VT_VARIANT:
            args[argspos++] = (ULONG_PTR)arg;
            break;
        case VT_BOOL:  /* VT_BOOL is 16-bit but BOOL is 32-bit, needs to be extended */
            args[argspos++] = V_BOOL(arg);
            break;
        default:
            args[argspos++] = V_UI8(arg);
            break;
        }
        TRACE("arg %u: type %s %s\n", i, debugstr_vt(prgvt[i]), debugstr_variant(arg));
    }

    switch (vtReturn)
    {
    case VT_R4:
        V_R4(pvargResult) = call_double_method( func, argspos, args );
        break;
    case VT_R8:
    case VT_DATE:
        V_R8(pvargResult) = call_double_method( func, argspos, args );
        break;
    case VT_DECIMAL:
    case VT_VARIANT:
        call_method( func, argspos, args );
        break;
    default:
        V_UI8(pvargResult) = call_method( func, argspos, args );
        break;
    }
    free( args );
    if (vtReturn != VT_VARIANT) V_VT(pvargResult) = vtReturn;
    TRACE("retval: %s\n", debugstr_variant(pvargResult));
    return S_OK;
}

#elif defined(__arm__)

extern LONGLONG CDECL call_method( void *func, int nb_stk_args, const DWORD *stk_args, const DWORD *reg_args );
extern float CDECL call_float_method( void *func, int nb_stk_args, const DWORD *stk_args, const DWORD *reg_args );
extern double CDECL call_double_method( void *func, int nb_stk_args, const DWORD *stk_args, const DWORD *reg_args );

HRESULT WINAPI DispCallFunc( void* pvInstance, ULONG_PTR oVft, CALLCONV cc, VARTYPE vtReturn,
                             UINT cActuals, VARTYPE* prgvt, VARIANTARG** prgpvarg, VARIANT* pvargResult )
{
    int argspos;
    void *func;
    UINT i;
    DWORD *args;
    struct {
        union {
            float s[16];
            double d[8];
        } sd;
        DWORD r[4];
    } regs;
    int rcount;     /* 32-bit register index count */
    int scount = 0; /* single-precision float register index count */
    int dcount = 0; /* double-precision float register index count */

    TRACE("(%p, %Id, %d, %d, %d, %p, %p, %p (vt=%d))\n",
        pvInstance, oVft, cc, vtReturn, cActuals, prgvt, prgpvarg, pvargResult, V_VT(pvargResult));

    if (cc != CC_STDCALL && cc != CC_CDECL)
    {
        FIXME("unsupported calling convention %d\n",cc);
        return E_INVALIDARG;
    }

    argspos = 0;
    rcount = 0;

    if (pvInstance)
    {
        const FARPROC *vtable = *(FARPROC **)pvInstance;
        func = vtable[oVft/sizeof(void *)];
        regs.r[rcount++] = (DWORD)pvInstance; /* the This pointer is always the first parameter */
    }
    else func = (void *)oVft;

    /* Determine if we need to pass a pointer for the return value as arg 0.  If so, do that */
    /*  first as it will need to be in the 'r' registers:                                    */
    switch (vtReturn)
    {
    case VT_DECIMAL:
    case VT_VARIANT:
        regs.r[rcount++] = (DWORD)pvargResult;  /* arg 0 is a pointer to the result */
        break;
    case VT_HRESULT:
        WARN("invalid return type %u\n", vtReturn);
        return E_INVALIDARG;
    default:                    /* And all others are in 'r', 's', or 'd' registers or have no return value */
        break;
    }

    /* maximum size for an argument is sizeof(VARIANT).  Also allow for return pointer and stack alignment. */
    args = malloc( sizeof(VARIANT) * cActuals + sizeof(DWORD) * 4 );

    for (i = 0; i < cActuals; i++)
    {
        VARIANT *arg = prgpvarg[i];
        DWORD *pdwarg = (DWORD *)(arg);     /* a reinterpret_cast of the variant, used for copying structures when they are split between registers and stack */
        int ntemp;              /* Used for counting words split between registers and stack */

        switch (prgvt[i])
        {
        case VT_R8:             /* these must be 8-byte aligned, and put in 'd' regs or stack, as they are double-floats */
        case VT_DATE:
            dcount = max( (scount + 1) / 2, dcount );
            if (dcount < 8)
            {
                regs.sd.d[dcount++] = V_R8(arg);
            }
            else
            {
                argspos += (argspos % 2);   /* align argspos to 8-bytes */
                memcpy( &args[argspos], &V_R8(arg), sizeof(V_R8(arg)) );
                argspos += sizeof(V_R8(arg)) / sizeof(DWORD);
            }
            break;
        case VT_I8:             /* these must be 8-byte aligned, and put in 'r' regs or stack, as they are long-longs */
        case VT_UI8:
        case VT_CY:
            if (rcount < 3)
            {
                rcount += (rcount % 2);     /* align rcount to 8-byte register pair */
                memcpy( &regs.r[rcount], &V_UI8(arg), sizeof(V_UI8(arg)) );
                rcount += sizeof(V_UI8(arg)) / sizeof(DWORD);
            }
            else
            {
                rcount = 4;                 /* Make sure we flag that all 'r' regs are full */
                argspos += (argspos % 2);   /* align argspos to 8-bytes */
                memcpy( &args[argspos], &V_UI8(arg), sizeof(V_UI8(arg)) );
                argspos += sizeof(V_UI8(arg)) / sizeof(DWORD);
            }
            break;
        case VT_DECIMAL:        /* these structures are 8-byte aligned, and put in 'r' regs or stack, can be split between the two */
        case VT_VARIANT:
            /* 8-byte align 'r' and/or stack: */
            if (rcount < 3)
                rcount += (rcount % 2);
            else
            {
                rcount = 4;
                argspos += (argspos % 2);
            }
            ntemp = sizeof(*arg) / sizeof(DWORD);
            while (ntemp > 0)
            {
                if (rcount < 4)
                    regs.r[rcount++] = *pdwarg++;
                else
                    args[argspos++] = *pdwarg++;
                --ntemp;
            }
            break;
        case VT_R4:             /* these must be 4-byte aligned, and put in 's' regs or stack, as they are single-floats */
            if (!(scount % 2)) scount = max( scount, dcount * 2 );
            if (scount < 16)
                regs.sd.s[scount++] = V_R4(arg);
            else
                args[argspos++] = V_UI4(arg);
            break;
        /* extend parameters to 32 bits */
        case VT_I1:
            if (rcount < 4) regs.r[rcount++] = V_I1(arg);
            else args[argspos++] = V_I1(arg);
            break;
        case VT_UI1:
            if (rcount < 4) regs.r[rcount++] = V_UI1(arg);
            else args[argspos++] = V_UI1(arg);
            break;
        case VT_I2:
            if (rcount < 4) regs.r[rcount++] = V_I2(arg);
            else args[argspos++] = V_I2(arg);
            break;
        case VT_UI2:
            if (rcount < 4) regs.r[rcount++] = V_UI2(arg);
            else args[argspos++] = V_UI2(arg);
            break;
        case VT_BOOL:
            if (rcount < 4) regs.r[rcount++] = V_BOOL(arg);
            else args[argspos++] = V_BOOL(arg);
            break;
        default:
            if (rcount < 4) regs.r[rcount++] = V_UI4(arg);
            else args[argspos++] = V_UI4(arg);
            break;
        }
        TRACE("arg %u: type %s %s\n", i, debugstr_vt(prgvt[i]), debugstr_variant(arg));
    }

    argspos += (argspos % 2);   /* Make sure stack function alignment is 8-byte */

    switch (vtReturn)
    {
    case VT_DECIMAL:    /* DECIMAL and VARIANT already have a pointer argument passed (see above) */
    case VT_VARIANT:
        call_method( func, argspos, args, (DWORD*)&regs );
        break;
    case VT_R4:
        V_R4(pvargResult) = call_float_method( func, argspos, args, (DWORD*)&regs );
        break;
    case VT_R8:
    case VT_DATE:
        V_R8(pvargResult) = call_double_method( func, argspos, args, (DWORD*)&regs );
        break;
    case VT_I8:
    case VT_UI8:
    case VT_CY:
        V_UI8(pvargResult) = call_method( func, argspos, args, (DWORD*)&regs );
        break;
    default:
        V_UI4(pvargResult) = call_method( func, argspos, args, (DWORD*)&regs );
        break;
    }
    free( args );
    if (vtReturn != VT_VARIANT) V_VT(pvargResult) = vtReturn;
    TRACE("retval: %s\n", debugstr_variant(pvargResult));
    return S_OK;
}

#elif defined(__aarch64__)

extern DWORD_PTR CDECL call_method( void *func, int nb_stk_args, const DWORD_PTR *stk_args, const DWORD_PTR *reg_args );
extern float CDECL call_float_method( void *func, int nb_stk_args, const DWORD_PTR *stk_args, const DWORD_PTR *reg_args );
extern double CDECL call_double_method( void *func, int nb_stk_args, const DWORD_PTR *stk_args, const DWORD_PTR *reg_args );

HRESULT WINAPI DispCallFunc( void *instance, ULONG_PTR offset, CALLCONV cc, VARTYPE ret_type, UINT count,
                             VARTYPE *types, VARIANTARG **vargs, VARIANT *result )
{
    int argspos;
    void *func;
    UINT i;
    DWORD_PTR *args;
    struct
    {
        union
        {
            float f;
            double d;
        } fp[8];
        DWORD_PTR x[9];
    } regs;
    int rcount;      /* 64-bit register index count */
    int fpcount = 0; /* float register index count */

    TRACE("(%p, %Id, %d, %d, %d, %p, %p, %p (vt=%d))\n",
          instance, offset, cc, ret_type, count, types, vargs, result, V_VT(result));

    if (cc != CC_STDCALL && cc != CC_CDECL)
    {
        FIXME("unsupported calling convention %d\n",cc);
        return E_INVALIDARG;
    }

    argspos = 0;
    rcount = 0;

    if (instance)
    {
        const FARPROC *vtable = *(FARPROC **)instance;
        func = vtable[offset/sizeof(void *)];
        regs.x[rcount++] = (DWORD_PTR)instance; /* the This pointer is always the first parameter */
    }
    else func = (void *)offset;

    /* maximum size for an argument is 16 */
    args = malloc( 16 * count );

    for (i = 0; i < count; i++)
    {
        VARIANT *arg = vargs[i];

        switch (types[i])
        {
        case VT_R4:
            if (fpcount < 8) regs.fp[fpcount++].f = V_R4(arg);
            else *(float *)&args[argspos++] = V_R4(arg);
            break;
        case VT_R8:
        case VT_DATE:
            if (fpcount < 8) regs.fp[fpcount++].d = V_R8(arg);
            else *(double *)&args[argspos++] = V_R8(arg);
            break;
        case VT_DECIMAL:
            if (rcount < 7)
            {
                memcpy( &regs.x[rcount], arg, sizeof(*arg) );
                rcount += 2;
            }
            else
            {
                memcpy( &args[argspos], arg, sizeof(*arg) );
                argspos += 2;
            }
            break;
        case VT_VARIANT:
            if (rcount < 8) regs.x[rcount++] = (DWORD_PTR)arg;
            else args[argspos++] = (DWORD_PTR)arg;
            break;
        case VT_BOOL:  /* VT_BOOL is 16-bit but BOOL is 32-bit, needs to be extended */
            if (rcount < 8) regs.x[rcount++] = V_BOOL(arg);
            else args[argspos++] = V_BOOL(arg);
            break;
        default:
            if (rcount < 8) regs.x[rcount++] = V_UI8(arg);
            else args[argspos++] = V_UI8(arg);
            break;
        }
        TRACE("arg %u: type %s %s\n", i, debugstr_vt(types[i]), debugstr_variant(arg));
    }

    argspos += (argspos % 2);   /* Make sure stack function alignment is 16-byte */

    switch (ret_type)
    {
    case VT_HRESULT:
        free( args );
        return E_INVALIDARG;
    case VT_DECIMAL:
    case VT_VARIANT:
        regs.x[8] = (DWORD_PTR)result;  /* x8 is a pointer to the result */
        call_method( func, argspos, args, (DWORD_PTR *)&regs );
        break;
    case VT_R4:
        V_R4(result) = call_float_method( func, argspos, args, (DWORD_PTR *)&regs );
        break;
    case VT_R8:
    case VT_DATE:
        V_R8(result) = call_double_method( func, argspos, args, (DWORD_PTR *)&regs );
        break;
    default:
        V_UI8(result) = call_method( func, argspos, args, (DWORD_PTR *)&regs );
        break;
    }
    free( args );
    if (ret_type != VT_VARIANT) V_VT(result) = ret_type;
    TRACE("retval: %s\n", debugstr_variant(result));
    return S_OK;
}

#else  /* __aarch64__ */

HRESULT WINAPI DispCallFunc( void* pvInstance, ULONG_PTR oVft, CALLCONV cc, VARTYPE vtReturn,
                             UINT cActuals, VARTYPE* prgvt, VARIANTARG** prgpvarg, VARIANT* pvargResult )
{
    FIXME( "(%p, %ld, %d, %d, %d, %p, %p, %p (vt=%d)): not implemented for this CPU\n",
           pvInstance, oVft, cc, vtReturn, cActuals, prgvt, prgpvarg, pvargResult, V_VT(pvargResult));
    return E_NOTIMPL;
}

#endif

static HRESULT userdefined_to_variantvt(ITypeInfo *tinfo, const TYPEDESC *tdesc, VARTYPE *vt)
{
    HRESULT hr = S_OK;
    ITypeInfo *tinfo2 = NULL;
    TYPEATTR *tattr = NULL;

    hr = ITypeInfo_GetRefTypeInfo(tinfo, tdesc->hreftype, &tinfo2);
    if (hr)
    {
        ERR("Could not get typeinfo of hreftype %lx for VT_USERDEFINED, hr %#lx.\n", tdesc->hreftype, hr);
        return hr;
    }
    hr = ITypeInfo_GetTypeAttr(tinfo2, &tattr);
    if (hr)
    {
        ERR("ITypeInfo_GetTypeAttr failed, hr %#lx.\n", hr);
        ITypeInfo_Release(tinfo2);
        return hr;
    }

    switch (tattr->typekind)
    {
    case TKIND_ENUM:
        *vt |= VT_I4;
        break;

    case TKIND_ALIAS:
        hr = typedescvt_to_variantvt(tinfo2, &tattr->tdescAlias, vt);
        break;

    case TKIND_INTERFACE:
        if (tattr->wTypeFlags & TYPEFLAG_FDISPATCHABLE)
           *vt |= VT_DISPATCH;
        else
           *vt |= VT_UNKNOWN;
        break;

    case TKIND_DISPATCH:
        *vt |= VT_DISPATCH;
        break;

    case TKIND_COCLASS:
        *vt |= VT_DISPATCH;
        break;

    case TKIND_RECORD:
        FIXME("TKIND_RECORD unhandled.\n");
        hr = E_NOTIMPL;
        break;

    case TKIND_UNION:
        FIXME("TKIND_UNION unhandled.\n");
        hr = E_NOTIMPL;
        break;

    default:
        FIXME("TKIND %d unhandled.\n",tattr->typekind);
        hr = E_NOTIMPL;
        break;
    }
    ITypeInfo_ReleaseTypeAttr(tinfo2, tattr);
    ITypeInfo_Release(tinfo2);
    return hr;
}

static HRESULT typedescvt_to_variantvt(ITypeInfo *tinfo, const TYPEDESC *tdesc, VARTYPE *vt)
{
    HRESULT hr = S_OK;

    /* enforce only one level of pointer indirection */
    if (!(*vt & VT_BYREF) && !(*vt & VT_ARRAY) && (tdesc->vt == VT_PTR))
    {
        tdesc = tdesc->lptdesc;

        /* munch VT_PTR -> VT_USERDEFINED(interface) into VT_UNKNOWN or
         * VT_DISPATCH and VT_PTR -> VT_PTR -> VT_USERDEFINED(interface) into 
         * VT_BYREF|VT_DISPATCH or VT_BYREF|VT_UNKNOWN */
        if ((tdesc->vt == VT_USERDEFINED) ||
            ((tdesc->vt == VT_PTR) && (tdesc->lptdesc->vt == VT_USERDEFINED)))
        {
            VARTYPE vt_userdefined = 0;
            const TYPEDESC *tdesc_userdefined = tdesc;
            if (tdesc->vt == VT_PTR)
            {
                vt_userdefined = VT_BYREF;
                tdesc_userdefined = tdesc->lptdesc;
            }
            hr = userdefined_to_variantvt(tinfo, tdesc_userdefined, &vt_userdefined);
            if ((hr == S_OK) && 
                (((vt_userdefined & VT_TYPEMASK) == VT_UNKNOWN) ||
                 ((vt_userdefined & VT_TYPEMASK) == VT_DISPATCH)))
            {
                *vt |= vt_userdefined;
                return S_OK;
            }
        }
        *vt = VT_BYREF;
    }

    switch (tdesc->vt)
    {
    case VT_HRESULT:
        *vt |= VT_ERROR;
        break;
    case VT_USERDEFINED:
        hr = userdefined_to_variantvt(tinfo, tdesc, vt);
        break;
    case VT_VOID:
    case VT_CARRAY:
    case VT_PTR:
    case VT_LPSTR:
    case VT_LPWSTR:
        ERR("cannot convert type %d into variant VT\n", tdesc->vt);
        hr = DISP_E_BADVARTYPE;
        break;
    case VT_SAFEARRAY:
        *vt |= VT_ARRAY;
        hr = typedescvt_to_variantvt(tinfo, tdesc->lptdesc, vt);
        break;
    case VT_INT:
        *vt |= VT_I4;
        break;
    case VT_UINT:
        *vt |= VT_UI4;
        break;
    default:
        *vt |= tdesc->vt;
        break;
    }
    return hr;
}

static HRESULT get_iface_guid(ITypeInfo *tinfo, HREFTYPE href, GUID *guid)
{
    ITypeInfo *tinfo2;
    TYPEATTR *tattr;
    HRESULT hres;
    int flags, i;

    hres = ITypeInfo_GetRefTypeInfo(tinfo, href, &tinfo2);
    if(FAILED(hres))
        return hres;

    hres = ITypeInfo_GetTypeAttr(tinfo2, &tattr);
    if(FAILED(hres)) {
        ITypeInfo_Release(tinfo2);
        return hres;
    }

    switch(tattr->typekind) {
    case TKIND_ALIAS:
        hres = get_iface_guid(tinfo2, tattr->tdescAlias.hreftype, guid);
        break;

    case TKIND_INTERFACE:
    case TKIND_DISPATCH:
        *guid = tattr->guid;
        break;

    case TKIND_COCLASS:
        for (i = 0; i < tattr->cImplTypes; i++)
        {
            ITypeInfo_GetImplTypeFlags(tinfo2, i, &flags);
            if (flags & IMPLTYPEFLAG_FDEFAULT)
                break;
        }

        if (i == tattr->cImplTypes)
            i = 0;

        hres = ITypeInfo_GetRefTypeOfImplType(tinfo2, i, &href);
        if (SUCCEEDED(hres))
            hres = get_iface_guid(tinfo2, href, guid);
        break;

    default:
        ERR("Unexpected typekind %d\n", tattr->typekind);
        hres = E_UNEXPECTED;
    }

    ITypeInfo_ReleaseTypeAttr(tinfo2, tattr);
    ITypeInfo_Release(tinfo2);
    return hres;
}

static inline BOOL func_restricted( const FUNCDESC *desc )
{
    return (desc->wFuncFlags & FUNCFLAG_FRESTRICTED) && (desc->memid >= 0);
}

#define INVBUF_ELEMENT_SIZE \
    (sizeof(VARIANTARG) + sizeof(VARIANTARG) + sizeof(VARIANTARG *) + sizeof(VARTYPE))
#define INVBUF_GET_ARG_ARRAY(buffer, params) (buffer)
#define INVBUF_GET_MISSING_ARG_ARRAY(buffer, params) \
    ((VARIANTARG *)((char *)(buffer) + sizeof(VARIANTARG) * (params)))
#define INVBUF_GET_ARG_PTR_ARRAY(buffer, params) \
    ((VARIANTARG **)((char *)(buffer) + (sizeof(VARIANTARG) + sizeof(VARIANTARG)) * (params)))
#define INVBUF_GET_ARG_TYPE_ARRAY(buffer, params) \
    ((VARTYPE *)((char *)(buffer) + (sizeof(VARIANTARG) + sizeof(VARIANTARG) + sizeof(VARIANTARG *)) * (params)))

static HRESULT WINAPI ITypeInfo_fnInvoke(
    ITypeInfo2 *iface,
    VOID  *pIUnk,
    MEMBERID memid,
    UINT16 wFlags,
    DISPPARAMS  *pDispParams,
    VARIANT  *pVarResult,
    EXCEPINFO  *pExcepInfo,
    UINT  *pArgErr)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    int i, j;
    unsigned int var_index;
    TYPEKIND type_kind;
    HRESULT hres;
    const TLBFuncDesc *pFuncInfo;
    UINT fdc;

    TRACE("%p, %p, %ld, %#x, %p, %p, %p, %p.\n", iface, pIUnk, memid, wFlags, pDispParams,
            pVarResult, pExcepInfo, pArgErr);

    if( This->typeattr.wTypeFlags & TYPEFLAG_FRESTRICTED )
        return DISP_E_MEMBERNOTFOUND;

    if (!pDispParams)
    {
        ERR("NULL pDispParams not allowed\n");
        return E_INVALIDARG;
    }

    dump_DispParms(pDispParams);

    if (pDispParams->cNamedArgs > pDispParams->cArgs)
    {
        ERR("named argument array cannot be bigger than argument array (%d/%d)\n",
            pDispParams->cNamedArgs, pDispParams->cArgs);
        return E_INVALIDARG;
    }

    /* we do this instead of using GetFuncDesc since it will return a fake
     * FUNCDESC for dispinterfaces and we want the real function description */
    for (fdc = 0; fdc < This->typeattr.cFuncs; ++fdc){
        pFuncInfo = &This->funcdescs[fdc];
        if ((memid == pFuncInfo->funcdesc.memid) &&
            (wFlags & pFuncInfo->funcdesc.invkind) &&
            !func_restricted( &pFuncInfo->funcdesc ))
            break;
    }

    if (fdc < This->typeattr.cFuncs) {
        const FUNCDESC *func_desc = &pFuncInfo->funcdesc;

        if (TRACE_ON(ole))
        {
            TRACE("invoking:\n");
            dump_TLBFuncDescOne(pFuncInfo);
        }
        
	switch (func_desc->funckind) {
	case FUNC_PUREVIRTUAL:
	case FUNC_VIRTUAL: {
            void *buffer = calloc(func_desc->cParams, INVBUF_ELEMENT_SIZE);
            VARIANT varresult;
            VARIANT retval = {{{0}}}; /* pointer for storing byref retvals in */
            VARIANTARG **prgpvarg = INVBUF_GET_ARG_PTR_ARRAY(buffer, func_desc->cParams);
            VARIANTARG *rgvarg = INVBUF_GET_ARG_ARRAY(buffer, func_desc->cParams);
            VARTYPE *rgvt = INVBUF_GET_ARG_TYPE_ARRAY(buffer, func_desc->cParams);
            VARIANTARG *missing_arg = INVBUF_GET_MISSING_ARG_ARRAY(buffer, func_desc->cParams);
            UINT cNamedArgs = pDispParams->cNamedArgs;
            DISPID *rgdispidNamedArgs = pDispParams->rgdispidNamedArgs;
            UINT vargs_converted=0;
            SAFEARRAY *a;

            hres = S_OK;

            if (func_desc->invkind & (INVOKE_PROPERTYPUT|INVOKE_PROPERTYPUTREF))
            {
                if (!cNamedArgs || (rgdispidNamedArgs[0] != DISPID_PROPERTYPUT))
                {
                    ERR("first named arg for property put invocation must be DISPID_PROPERTYPUT\n");
                    hres = DISP_E_PARAMNOTFOUND;
                    goto func_fail;
                }
            }

            if (func_desc->cParamsOpt < 0 && cNamedArgs)
            {
                ERR("functions with the vararg attribute do not support named arguments\n");
                hres = DISP_E_NONAMEDARGS;
                goto func_fail;
            }

            for (i = 0; i < func_desc->cParams; i++)
            {
                TYPEDESC *tdesc = &func_desc->lprgelemdescParam[i].tdesc;
                hres = typedescvt_to_variantvt((ITypeInfo *)iface, tdesc, &rgvt[i]);
                if (FAILED(hres))
                    goto func_fail;
            }

            TRACE("changing args\n");
            for (i = 0; i < func_desc->cParams; i++)
            {
                USHORT wParamFlags = func_desc->lprgelemdescParam[i].paramdesc.wParamFlags;
                TYPEDESC *tdesc = &func_desc->lprgelemdescParam[i].tdesc;
                VARIANTARG *src_arg;

                if (wParamFlags & PARAMFLAG_FLCID)
                {
                    prgpvarg[i] = &rgvarg[i];
                    V_VT(prgpvarg[i]) = VT_I4;
                    V_I4(prgpvarg[i]) = This->pTypeLib->lcid;
                    continue;
                }

                src_arg = NULL;

                for (j = 0; j < cNamedArgs; j++)
                {
                    if (rgdispidNamedArgs[j] == i || (i == func_desc->cParams-1 && rgdispidNamedArgs[j] == DISPID_PROPERTYPUT))
                    {
                        src_arg = &pDispParams->rgvarg[j];
                        break;
                    }
                }

                if (!src_arg && vargs_converted + cNamedArgs < pDispParams->cArgs)
                {
                    src_arg = &pDispParams->rgvarg[pDispParams->cArgs - 1 - vargs_converted];
                    vargs_converted++;
                }

                if (wParamFlags & PARAMFLAG_FRETVAL)
                {
                    /* under most conditions the caller is not allowed to
                     * pass in a dispparam arg in the index of what would be
                     * the retval parameter. however, there is an exception
                     * where the extra parameter is used in an extra
                     * IDispatch::Invoke below */
                    if ((i < pDispParams->cArgs) &&
                        ((func_desc->cParams != 1) || !pVarResult ||
                         !(func_desc->invkind & INVOKE_PROPERTYGET)))
                    {
                        hres = DISP_E_BADPARAMCOUNT;
                        break;
                    }

                    /* note: this check is placed so that if the caller passes
                     * in a VARIANTARG for the retval we just ignore it, like
                     * native does */
                    if (i == func_desc->cParams - 1)
                    {
                        prgpvarg[i] = &rgvarg[i];
                        V_BYREF(prgpvarg[i]) = &retval;
                        V_VT(prgpvarg[i]) = rgvt[i];
                    }
                    else
                    {
                        ERR("[retval] parameter must be the last parameter of the method (%d/%d)\n", i, func_desc->cParams);
                        hres = E_UNEXPECTED;
                        break;
                    }
                }
                else if (src_arg && !((wParamFlags & PARAMFLAG_FOPT) &&
                         V_VT(src_arg) == VT_ERROR && V_ERROR(src_arg) == DISP_E_PARAMNOTFOUND))
                {
                    TRACE("%s\n", debugstr_variant(src_arg));

                    if(rgvt[i]!=V_VT(src_arg))
                    {
                        if (rgvt[i] == VT_VARIANT)
                            hres = VariantCopy(&rgvarg[i], src_arg);
                        else if (rgvt[i] == (VT_VARIANT | VT_BYREF))
                        {
                            if (rgvt[i] == V_VT(src_arg))
                                V_VARIANTREF(&rgvarg[i]) = V_VARIANTREF(src_arg);
                            else
                            {
                                if (wParamFlags & PARAMFLAG_FIN)
                                    hres = VariantCopy(&missing_arg[i], src_arg);
                                V_VARIANTREF(&rgvarg[i]) = &missing_arg[i];
                            }
                            V_VT(&rgvarg[i]) = rgvt[i];
                        }
                        else if ((rgvt[i] == (VT_VARIANT | VT_ARRAY) || rgvt[i] == (VT_VARIANT | VT_ARRAY | VT_BYREF)) && func_desc->cParamsOpt < 0)
                        {
                            SAFEARRAYBOUND bound;
                            VARIANT *v;

                            bound.lLbound = 0;
                            bound.cElements = pDispParams->cArgs-i;
                            if (!(a = SafeArrayCreate(VT_VARIANT, 1, &bound)))
                            {
                                ERR("SafeArrayCreate failed\n");
                                break;
                            }
                            hres = SafeArrayAccessData(a, (LPVOID)&v);
                            if (hres != S_OK)
                            {
                                ERR("SafeArrayAccessData failed with %#lx.\n", hres);
                                SafeArrayDestroy(a);
                                break;
                            }
                            for (j = 0; j < bound.cElements; j++)
                                VariantCopy(&v[j], &pDispParams->rgvarg[pDispParams->cArgs - 1 - i - j]);
                            hres = SafeArrayUnaccessData(a);
                            if (hres != S_OK)
                            {
                                ERR("SafeArrayUnaccessData failed with %#lx.\n", hres);
                                SafeArrayDestroy(a);
                                break;
                            }
                            if (rgvt[i] & VT_BYREF)
                                V_BYREF(&rgvarg[i]) = &a;
                            else
                                V_ARRAY(&rgvarg[i]) = a;
                            V_VT(&rgvarg[i]) = rgvt[i];
                        }
                        else if ((rgvt[i] & VT_BYREF) && !V_ISBYREF(src_arg))
                        {
                            if (wParamFlags & PARAMFLAG_FIN)
                                hres = VariantChangeType(&missing_arg[i], src_arg, 0, rgvt[i] & ~VT_BYREF);
                            else
                                V_VT(&missing_arg[i]) = rgvt[i] & ~VT_BYREF;
                            V_BYREF(&rgvarg[i]) = &V_NONE(&missing_arg[i]);
                            V_VT(&rgvarg[i]) = rgvt[i];
                        }
                        else if ((rgvt[i] & VT_BYREF) && (rgvt[i] == V_VT(src_arg)))
                        {
                            V_BYREF(&rgvarg[i]) = V_BYREF(src_arg);
                            V_VT(&rgvarg[i]) = rgvt[i];
                        }
                        else
                        {
                            /* FIXME: this doesn't work for VT_BYREF arguments if
                             * they are not the same type as in the paramdesc */
                            V_VT(&rgvarg[i]) = V_VT(src_arg);
                            hres = VariantChangeType(&rgvarg[i], src_arg, 0, rgvt[i]);
                            V_VT(&rgvarg[i]) = rgvt[i];
                        }

                        if (FAILED(hres))
                        {
                            ERR("failed to convert param %d to %s from %s\n", i,
                                debugstr_vt(rgvt[i]), debugstr_variant(src_arg));
                            break;
                        }
                        prgpvarg[i] = &rgvarg[i];
                    }
                    else
                    {
                        prgpvarg[i] = src_arg;
                    }

                    if((tdesc->vt == VT_USERDEFINED || (tdesc->vt == VT_PTR && tdesc->lptdesc->vt == VT_USERDEFINED))
                       && (V_VT(prgpvarg[i]) == VT_DISPATCH || V_VT(prgpvarg[i]) == VT_UNKNOWN)
                       && V_UNKNOWN(prgpvarg[i])) {
                        IUnknown *userdefined_iface;
                        GUID guid;

                        if (tdesc->vt == VT_PTR)
                            tdesc = tdesc->lptdesc;

                        hres = get_iface_guid((ITypeInfo*)iface, tdesc->hreftype, &guid);
                        if(FAILED(hres))
                            break;

                        hres = IUnknown_QueryInterface(V_UNKNOWN(prgpvarg[i]), &guid, (void**)&userdefined_iface);
                        if(FAILED(hres)) {
                            ERR("argument does not support %s interface\n", debugstr_guid(&guid));
                            break;
                        }

                        IUnknown_Release(V_UNKNOWN(prgpvarg[i]));
                        V_UNKNOWN(prgpvarg[i]) = userdefined_iface;
                    }
                }
                else if (wParamFlags & PARAMFLAG_FOPT)
                {
                    VARIANTARG *arg;
                    arg = prgpvarg[i] = &rgvarg[i];
                    if (wParamFlags & PARAMFLAG_FHASDEFAULT)
                    {
                        hres = VariantCopy(arg, &func_desc->lprgelemdescParam[i].paramdesc.pparamdescex->varDefaultValue);
                        if (FAILED(hres))
                            break;
                    }
                    else
                    {
                        /* if the function wants a pointer to a variant then
                         * set that up, otherwise just pass the VT_ERROR in
                         * the argument by value */
                        if (rgvt[i] & VT_BYREF)
                        {
                            V_VT(&missing_arg[i]) = VT_ERROR;
                            V_ERROR(&missing_arg[i]) = DISP_E_PARAMNOTFOUND;

                            V_VT(arg) = VT_VARIANT | VT_BYREF;
                            V_VARIANTREF(arg) = &missing_arg[i];
                        }
                        else
                        {
                            V_VT(arg) = VT_ERROR;
                            V_ERROR(arg) = DISP_E_PARAMNOTFOUND;
                        }
                    }
                }
                else if (func_desc->cParamsOpt < 0 && ((rgvt[i] & ~VT_BYREF) == (VT_VARIANT | VT_ARRAY)))
                {
                    hres = SafeArrayAllocDescriptorEx( VT_EMPTY, 1, &a );
                    if (FAILED(hres)) break;
                    if (rgvt[i] & VT_BYREF)
                        V_BYREF(&rgvarg[i]) = &a;
                    else
                        V_ARRAY(&rgvarg[i]) = a;
                    V_VT(&rgvarg[i]) = rgvt[i];
                    prgpvarg[i] = &rgvarg[i];
                }
                else
                {
                    hres = DISP_E_BADPARAMCOUNT;
                    break;
                }
            }
            if (FAILED(hres)) goto func_fail; /* FIXME: we don't free changed types here */

            /* VT_VOID is a special case for return types, so it is not
             * handled in the general function */
            if (func_desc->elemdescFunc.tdesc.vt == VT_VOID)
                V_VT(&varresult) = VT_EMPTY;
            else
            {
                V_VT(&varresult) = 0;
                hres = typedescvt_to_variantvt((ITypeInfo *)iface, &func_desc->elemdescFunc.tdesc, &V_VT(&varresult));
                if (FAILED(hres)) goto func_fail; /* FIXME: we don't free changed types here */
            }

            hres = DispCallFunc(pIUnk, func_desc->oVft & 0xFFFC, func_desc->callconv,
                                V_VT(&varresult), func_desc->cParams, rgvt,
                                prgpvarg, &varresult);

            vargs_converted = 0;

            for (i = 0; i < func_desc->cParams; i++)
            {
                USHORT wParamFlags = func_desc->lprgelemdescParam[i].paramdesc.wParamFlags;

                if (wParamFlags & PARAMFLAG_FLCID)
                    continue;
                else if (wParamFlags & PARAMFLAG_FRETVAL)
                {
                    TRACE("[retval] value: %s\n", debugstr_variant(prgpvarg[i]));

                    if (pVarResult)
                    {
                        VariantInit(pVarResult);
                        /* deref return value */
                        hres = VariantCopyInd(pVarResult, prgpvarg[i]);
                    }

                    VARIANT_ClearInd(prgpvarg[i]);
                }
                else if (vargs_converted < pDispParams->cArgs)
                {
                    VARIANTARG *arg = &pDispParams->rgvarg[pDispParams->cArgs - 1 - vargs_converted];
                    if (wParamFlags & PARAMFLAG_FOUT)
                    {
                        if ((rgvt[i] & VT_BYREF) && !(V_VT(arg) & VT_BYREF))
                        {
                            hres = VariantChangeType(arg, &rgvarg[i], 0, V_VT(arg));

                            if (FAILED(hres))
                            {
                                ERR("failed to convert param %d to vt %d\n", i,
                                    V_VT(&pDispParams->rgvarg[pDispParams->cArgs - 1 - vargs_converted]));
                                break;
                            }
                        }
                    }
                    else if (V_VT(prgpvarg[i]) == (VT_VARIANT | VT_ARRAY) &&
                             func_desc->cParamsOpt < 0 &&
                             i == func_desc->cParams-1)
                    {
                        SAFEARRAY *a = V_ARRAY(prgpvarg[i]);
                        LONG ubound;
                        VARIANT *v;
                        hres = SafeArrayGetUBound(a, 1, &ubound);
                        if (hres != S_OK)
                        {
                            ERR("SafeArrayGetUBound failed with %#lx.\n", hres);
                            break;
                        }
                        hres = SafeArrayAccessData(a, (LPVOID)&v);
                        if (hres != S_OK)
                        {
                            ERR("SafeArrayAccessData failed with %#lx.\n", hres);
                            break;
                        }
                        for (j = 0; j <= ubound; j++)
                            VariantClear(&v[j]);
                        hres = SafeArrayUnaccessData(a);
                        if (hres != S_OK)
                        {
                            ERR("SafeArrayUnaccessData failed with %#lx.\n", hres);
                            break;
                        }
                    }
                    VariantClear(&rgvarg[i]);
                    vargs_converted++;
                }
                else if (wParamFlags & PARAMFLAG_FOPT)
                {
                    if (wParamFlags & PARAMFLAG_FHASDEFAULT)
                        VariantClear(&rgvarg[i]);
                }

                VariantClear(&missing_arg[i]);
            }

            if ((V_VT(&varresult) == VT_ERROR) && FAILED(V_ERROR(&varresult)))
            {
                WARN("invoked function failed with error %#lx.\n", V_ERROR(&varresult));
                hres = DISP_E_EXCEPTION;
                if (pExcepInfo)
                {
                    IErrorInfo *pErrorInfo;
                    pExcepInfo->scode = V_ERROR(&varresult);
                    if (GetErrorInfo(0, &pErrorInfo) == S_OK)
                    {
                        IErrorInfo_GetDescription(pErrorInfo, &pExcepInfo->bstrDescription);
                        IErrorInfo_GetHelpFile(pErrorInfo, &pExcepInfo->bstrHelpFile);
                        IErrorInfo_GetSource(pErrorInfo, &pExcepInfo->bstrSource);
                        IErrorInfo_GetHelpContext(pErrorInfo, &pExcepInfo->dwHelpContext);

                        IErrorInfo_Release(pErrorInfo);
                    }
                }
            }
            if (V_VT(&varresult) != VT_ERROR)
            {
                TRACE("varresult value: %s\n", debugstr_variant(&varresult));

                if (pVarResult)
                {
                    VariantClear(pVarResult);
                    *pVarResult = varresult;
                }
                else
                    VariantClear(&varresult);
            }

            if (SUCCEEDED(hres) && pVarResult && (func_desc->cParams == 1) &&
                (func_desc->invkind & INVOKE_PROPERTYGET) &&
                (func_desc->lprgelemdescParam[0].paramdesc.wParamFlags & PARAMFLAG_FRETVAL) &&
                (pDispParams->cArgs != 0))
            {
                if (V_VT(pVarResult) == VT_DISPATCH)
                {
                    IDispatch *pDispatch = V_DISPATCH(pVarResult);
                    /* Note: not VariantClear; we still need the dispatch
                     * pointer to be valid */
                    VariantInit(pVarResult);
                    hres = IDispatch_Invoke(pDispatch, DISPID_VALUE, &IID_NULL,
                        GetSystemDefaultLCID(), wFlags,
                        pDispParams, pVarResult, pExcepInfo, pArgErr);
                    IDispatch_Release(pDispatch);
                }
                else
                {
                    VariantClear(pVarResult);
                    hres = DISP_E_NOTACOLLECTION;
                }
            }

func_fail:
            free(buffer);
            break;
        }
	case FUNC_DISPATCH:  {
	   IDispatch *disp;

	   hres = IUnknown_QueryInterface((LPUNKNOWN)pIUnk,&IID_IDispatch,(LPVOID*)&disp);
	   if (SUCCEEDED(hres)) {
               FIXME("Calling Invoke in IDispatch iface. untested!\n");
               hres = IDispatch_Invoke(
                                     disp,memid,&IID_NULL,LOCALE_USER_DEFAULT,wFlags,pDispParams,
                                     pVarResult,pExcepInfo,pArgErr
                                     );
               if (FAILED(hres))
                   FIXME("IDispatch::Invoke failed with %#lx. (Could be not a real error?)\n", hres);
               IDispatch_Release(disp);
           } else
	       FIXME("FUNC_DISPATCH used on object without IDispatch iface?\n");
           break;
	}
	default:
            FIXME("Unknown function invocation type %d\n", func_desc->funckind);
            hres = E_FAIL;
            break;
        }

        TRACE("-- %#lx\n", hres);
        return hres;

    } else if(SUCCEEDED(hres = ITypeInfo2_GetVarIndexOfMemId(iface, memid, &var_index))) {
        VARDESC *var_desc;

        hres = ITypeInfo2_GetVarDesc(iface, var_index, &var_desc);
        if(FAILED(hres)) return hres;
        
        FIXME("varseek: Found memid, but variable-based invoking not supported\n");
        dump_VARDESC(var_desc);
        ITypeInfo2_ReleaseVarDesc(iface, var_desc);
        return E_NOTIMPL;
    }

    /* not found, check for special error cases */
    for (fdc = 0; fdc < This->typeattr.cFuncs; ++fdc)
    {
        const FUNCDESC *func_desc = &This->funcdescs[fdc].funcdesc;
        if (memid == func_desc->memid)
        {
            if ((wFlags & INVOKE_PROPERTYPUT) && (func_desc->invkind & INVOKE_PROPERTYGET))
            {
                int count_inputs = 0;
                for (i = 0; i < func_desc->cParams; i++)
                {
                    USHORT wParamFlags = func_desc->lprgelemdescParam[i].paramdesc.wParamFlags;
                    if (!(wParamFlags & PARAMFLAG_FRETVAL))
                        count_inputs++;
                }

                if (count_inputs == 0 || pDispParams->cArgs == count_inputs + 1)
                    return DISP_E_BADPARAMCOUNT;
            }
        }
    }

    /* not found, look for it in inherited interfaces */
    ITypeInfo2_GetTypeKind(iface, &type_kind);
    if(type_kind == TKIND_INTERFACE || type_kind == TKIND_DISPATCH) {
        if(This->impltypes) {
            /* recursive search */
            ITypeInfo *pTInfo;
            hres = ITypeInfo2_GetRefTypeInfo(iface, This->impltypes[0].hRef, &pTInfo);
            if(SUCCEEDED(hres)){
                hres = ITypeInfo_Invoke(pTInfo,pIUnk,memid,wFlags,pDispParams,pVarResult,pExcepInfo,pArgErr);
                ITypeInfo_Release(pTInfo);
                return hres;
            }
            WARN("Could not search inherited interface!\n");
        }
    }
    WARN("did not find member id %ld, flags 0x%x!\n", memid, wFlags);
    return DISP_E_MEMBERNOTFOUND;
}

/* ITypeInfo::GetDocumentation
 *
 * Retrieves the documentation string, the complete Help file name and path,
 * and the context ID for the Help topic for a specified type description.
 *
 * (Can be tested by the Visual Basic Editor in Word for instance.)
 */
static HRESULT WINAPI ITypeInfo_fnGetDocumentation( ITypeInfo2 *iface,
        MEMBERID memid, BSTR  *pBstrName, BSTR  *pBstrDocString,
        DWORD  *pdwHelpContext, BSTR  *pBstrHelpFile)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    const TLBFuncDesc *pFDesc;
    const TLBVarDesc *pVDesc;
    TRACE("%p, %ld, %p, %p, %p, %p.\n",
        iface, memid, pBstrName, pBstrDocString, pdwHelpContext, pBstrHelpFile);
    if(memid==MEMBERID_NIL){ /* documentation for the typeinfo */
        if(pBstrName)
            *pBstrName=SysAllocString(TLB_get_bstr(This->Name));
        if(pBstrDocString)
            *pBstrDocString=SysAllocString(TLB_get_bstr(This->DocString));
        if(pdwHelpContext)
            *pdwHelpContext=This->dwHelpContext;
        if(pBstrHelpFile)
            *pBstrHelpFile=SysAllocString(TLB_get_bstr(This->pTypeLib->HelpFile));
        return S_OK;
    }else {/* for a member */
        pFDesc = TLB_get_funcdesc_by_memberid(This, memid);
        if(pFDesc){
            if(pBstrName)
              *pBstrName = SysAllocString(TLB_get_bstr(pFDesc->Name));
            if(pBstrDocString)
              *pBstrDocString=SysAllocString(TLB_get_bstr(pFDesc->HelpString));
            if(pdwHelpContext)
              *pdwHelpContext=pFDesc->helpcontext;
            if(pBstrHelpFile)
              *pBstrHelpFile = SysAllocString(TLB_get_bstr(This->pTypeLib->HelpFile));
            return S_OK;
        }
        pVDesc = TLB_get_vardesc_by_memberid(This, memid);
        if(pVDesc){
            if(pBstrName)
              *pBstrName = SysAllocString(TLB_get_bstr(pVDesc->Name));
            if(pBstrDocString)
              *pBstrDocString=SysAllocString(TLB_get_bstr(pVDesc->HelpString));
            if(pdwHelpContext)
              *pdwHelpContext=pVDesc->HelpContext;
            if(pBstrHelpFile)
              *pBstrHelpFile = SysAllocString(TLB_get_bstr(This->pTypeLib->HelpFile));
            return S_OK;
        }
    }

    if(This->impltypes &&
       (This->typeattr.typekind == TKIND_INTERFACE || This->typeattr.typekind == TKIND_DISPATCH)) {
        /* recursive search */
        ITypeInfo *pTInfo;
        HRESULT result;
        result = ITypeInfo2_GetRefTypeInfo(iface, This->impltypes[0].hRef, &pTInfo);
        if(SUCCEEDED(result)) {
            result = ITypeInfo_GetDocumentation(pTInfo, memid, pBstrName,
                pBstrDocString, pdwHelpContext, pBstrHelpFile);
            ITypeInfo_Release(pTInfo);
            return result;
        }
        WARN("Could not search inherited interface!\n");
    }

    WARN("member %ld not found\n", memid);
    return TYPE_E_ELEMENTNOTFOUND;
}

/*  ITypeInfo::GetDllEntry
 *
 * Retrieves a description or specification of an entry point for a function
 * in a DLL.
 */
static HRESULT WINAPI ITypeInfo_fnGetDllEntry( ITypeInfo2 *iface, MEMBERID memid,
        INVOKEKIND invKind, BSTR  *pBstrDllName, BSTR  *pBstrName,
        WORD  *pwOrdinal)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    const TLBFuncDesc *pFDesc;

    TRACE("%p, %#lx, %d, %p, %p, %p.\n", iface, memid, invKind, pBstrDllName, pBstrName, pwOrdinal);

    if (pBstrDllName) *pBstrDllName = NULL;
    if (pBstrName) *pBstrName = NULL;
    if (pwOrdinal) *pwOrdinal = 0;

    if (This->typeattr.typekind != TKIND_MODULE)
        return TYPE_E_BADMODULEKIND;

    pFDesc = TLB_get_funcdesc_by_memberid_invkind(This, memid, invKind);
    if (!pFDesc) return TYPE_E_ELEMENTNOTFOUND;

    dump_TypeInfo(This);
    if (TRACE_ON(ole)) dump_TLBFuncDescOne(pFDesc);

    if (pBstrDllName) *pBstrDllName = SysAllocString(TLB_get_bstr(This->DllName));

    if (!IS_INTRESOURCE(pFDesc->Entry) && (pFDesc->Entry != (void*)-1))
    {
        if (pBstrName) *pBstrName = SysAllocString(TLB_get_bstr(pFDesc->Entry));
        if (pwOrdinal) *pwOrdinal = -1;
    }
    else
    {
        if (pBstrName) *pBstrName = NULL;
        if (pwOrdinal) *pwOrdinal = LOWORD(pFDesc->Entry);
    }
    return S_OK;
}

/* internal function to make the inherited interfaces' methods appear
 * part of the interface */
static HRESULT ITypeInfoImpl_GetDispatchRefTypeInfo( ITypeInfo *iface,
    HREFTYPE *hRefType, ITypeInfo  **ppTInfo)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo(iface);
    HRESULT hr;

    TRACE("%p, %#lx.\n", iface, *hRefType);

    if (This->impltypes && (*hRefType & DISPATCH_HREF_MASK))
    {
        ITypeInfo *pSubTypeInfo;

        hr = ITypeInfo_GetRefTypeInfo(iface, This->impltypes[0].hRef, &pSubTypeInfo);
        if (FAILED(hr))
            return hr;

        hr = ITypeInfoImpl_GetDispatchRefTypeInfo(pSubTypeInfo,
                                                  hRefType, ppTInfo);
        ITypeInfo_Release(pSubTypeInfo);
        if (SUCCEEDED(hr))
            return hr;
    }
    *hRefType -= DISPATCH_HREF_OFFSET;

    if (!(*hRefType & DISPATCH_HREF_MASK))
        return ITypeInfo_GetRefTypeInfo(iface, *hRefType, ppTInfo);
    else
        return E_FAIL;
}

/* ITypeInfo::GetRefTypeInfo
 *
 * If a type description references other type descriptions, it retrieves
 * the referenced type descriptions.
 */
static HRESULT WINAPI ITypeInfo_fnGetRefTypeInfo(
	ITypeInfo2 *iface,
        HREFTYPE hRefType,
	ITypeInfo  **ppTInfo)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    ITypeInfo *type_info = NULL;
    HRESULT result = E_FAIL;
    TLBRefType *ref_type;
    UINT i;

    if(!ppTInfo)
        return E_INVALIDARG;

    if ((INT)hRefType < 0) {
        ITypeInfoImpl *pTypeInfoImpl;

        if (!(This->typeattr.wTypeFlags & TYPEFLAG_FDUAL) ||
                !(This->typeattr.typekind == TKIND_INTERFACE ||
                    This->typeattr.typekind == TKIND_DISPATCH))
            return TYPE_E_ELEMENTNOTFOUND;

        /* when we meet a DUAL typeinfo, we must create the alternate
        * version of it.
        */
        pTypeInfoImpl = ITypeInfoImpl_Constructor();

        *pTypeInfoImpl = *This;
        pTypeInfoImpl->ref = 0;
        list_init(&pTypeInfoImpl->custdata_list);

        if (This->typeattr.typekind == TKIND_INTERFACE)
            pTypeInfoImpl->typeattr.typekind = TKIND_DISPATCH;
        else
            pTypeInfoImpl->typeattr.typekind = TKIND_INTERFACE;

        *ppTInfo = (ITypeInfo *)&pTypeInfoImpl->ITypeInfo2_iface;
        /* the AddRef implicitly adds a reference to the parent typelib, which
         * stops the copied data from being destroyed until the new typeinfo's
         * refcount goes to zero, but we need to signal to the new instance to
         * not free its data structures when it is destroyed */
        pTypeInfoImpl->not_attached_to_typelib = TRUE;
        ITypeInfo_AddRef(*ppTInfo);

        TRACE("got dual interface %p\n", *ppTInfo);
        return S_OK;
    }

    if ((hRefType & DISPATCH_HREF_MASK) && (This->typeattr.typekind == TKIND_DISPATCH))
        return ITypeInfoImpl_GetDispatchRefTypeInfo((ITypeInfo *)iface, &hRefType, ppTInfo);

    if(!(hRefType & 0x1))
    {
        for(i = 0; i < This->pTypeLib->TypeInfoCount; ++i)
        {
            if (This->pTypeLib->typeinfos[i]->hreftype == (hRefType&(~0x3)))
            {
                result = S_OK;
                type_info = (ITypeInfo*)&This->pTypeLib->typeinfos[i]->ITypeInfo2_iface;
                ITypeInfo_AddRef(type_info);
                break;
            }
        }
    }

    if (!type_info)
    {
        ITypeLib *pTLib = NULL;

        LIST_FOR_EACH_ENTRY(ref_type, &This->pTypeLib->ref_list, TLBRefType, entry)
        {
            if(ref_type->reference == (hRefType & (~0x3)))
                break;
        }
        if(&ref_type->entry == &This->pTypeLib->ref_list)
        {
            FIXME("Can't find pRefType for ref %lx\n", hRefType);
            return E_FAIL;
        }

        if(ref_type->pImpTLInfo == TLB_REF_INTERNAL) {
            UINT Index;
            TRACE("internal reference\n");
            result = ITypeInfo2_GetContainingTypeLib(iface, &pTLib, &Index);
        } else {
            if(ref_type->pImpTLInfo->pImpTypeLib) {
                TRACE("typeinfo in imported typelib that is already loaded\n");
                pTLib = (ITypeLib*)&ref_type->pImpTLInfo->pImpTypeLib->ITypeLib2_iface;
                ITypeLib_AddRef(pTLib);
                result = S_OK;
            } else {
                /* Search in cached typelibs */
                ITypeLibImpl *entry;

                EnterCriticalSection(&cache_section);
                LIST_FOR_EACH_ENTRY(entry, &tlb_cache, ITypeLibImpl, entry)
                {
                    if (entry->guid
                        && IsEqualIID(&entry->guid->guid, TLB_get_guid_null(ref_type->pImpTLInfo->guid))
                        && entry->ver_major == ref_type->pImpTLInfo->wVersionMajor
                        && entry->ver_minor == ref_type->pImpTLInfo->wVersionMinor
                        && entry->set_lcid == ref_type->pImpTLInfo->lcid)
                    {
                        TRACE("got cached %p\n", entry);
                        pTLib = (ITypeLib*)&entry->ITypeLib2_iface;
                        ITypeLib_AddRef(pTLib);
                        result = S_OK;
                        break;
                    }
                }
                LeaveCriticalSection(&cache_section);

                if (!pTLib)
                {
                    BSTR libnam;

                    /* Search on disk */
                    result = query_typelib_path(TLB_get_guid_null(ref_type->pImpTLInfo->guid),
                            ref_type->pImpTLInfo->wVersionMajor,
                            ref_type->pImpTLInfo->wVersionMinor,
                            This->pTypeLib->syskind,
                            ref_type->pImpTLInfo->lcid, &libnam, TRUE);
                    if (FAILED(result))
                        libnam = SysAllocString(ref_type->pImpTLInfo->name);

                    result = LoadTypeLib(libnam, &pTLib);
                    SysFreeString(libnam);
                }

                if(SUCCEEDED(result)) {
                    ref_type->pImpTLInfo->pImpTypeLib = impl_from_ITypeLib(pTLib);
                    ITypeLib_AddRef(pTLib);
                }
            }
        }
        if(SUCCEEDED(result)) {
            if(ref_type->index == TLB_REF_USE_GUID)
                result = ITypeLib_GetTypeInfoOfGuid(pTLib, TLB_get_guid_null(ref_type->guid), &type_info);
            else
                result = ITypeLib_GetTypeInfo(pTLib, ref_type->index, &type_info);
        }
        if (pTLib != NULL)
            ITypeLib_Release(pTLib);
        if (FAILED(result))
        {
            WARN("(%p) failed hreftype %#lx.\n", iface, hRefType);
            return result;
        }
    }

    if ((hRefType & 0x2) && SUCCEEDED(ITypeInfo_GetRefTypeInfo(type_info, -2, ppTInfo)))
        ITypeInfo_Release(type_info);
    else *ppTInfo = type_info;

    TRACE("%p, hreftype %#lx, loaded %s (%p)\n", iface, hRefType,
          SUCCEEDED(result)? "SUCCESS":"FAILURE", *ppTInfo);
    return result;
}

/* ITypeInfo::AddressOfMember
 *
 * Retrieves the addresses of static functions or variables, such as those
 * defined in a DLL.
 */
static HRESULT WINAPI ITypeInfo_fnAddressOfMember( ITypeInfo2 *iface,
        MEMBERID memid, INVOKEKIND invKind, PVOID *ppv)
{
    HRESULT hr;
    BSTR dll, entry;
    WORD ordinal;
    HMODULE module;

    TRACE("%p, %lx, %#x, %p.\n", iface, memid, invKind, ppv);

    hr = ITypeInfo2_GetDllEntry(iface, memid, invKind, &dll, &entry, &ordinal);
    if (FAILED(hr))
        return hr;

    module = LoadLibraryW(dll);
    if (!module)
    {
        ERR("couldn't load %s\n", debugstr_w(dll));
        SysFreeString(dll);
        SysFreeString(entry);
        return STG_E_FILENOTFOUND;
    }
    /* FIXME: store library somewhere where we can free it */

    if (entry)
    {
        LPSTR entryA;
        INT len = WideCharToMultiByte(CP_ACP, 0, entry, -1, NULL, 0, NULL, NULL);
        entryA = malloc(len);
        WideCharToMultiByte(CP_ACP, 0, entry, -1, entryA, len, NULL, NULL);

        *ppv = GetProcAddress(module, entryA);
        if (!*ppv)
            ERR("function not found %s\n", debugstr_a(entryA));

        free(entryA);
    }
    else
    {
        *ppv = GetProcAddress(module, MAKEINTRESOURCEA(ordinal));
        if (!*ppv)
            ERR("function not found %d\n", ordinal);
    }

    SysFreeString(dll);
    SysFreeString(entry);

    if (!*ppv)
        return TYPE_E_DLLFUNCTIONNOTFOUND;

    return S_OK;
}

/* ITypeInfo::CreateInstance
 *
 * Creates a new instance of a type that describes a component object class
 * (coclass).
 */
static HRESULT WINAPI ITypeInfo_fnCreateInstance( ITypeInfo2 *iface,
        IUnknown *pOuterUnk, REFIID riid, VOID  **ppvObj)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    HRESULT hr;
    TYPEATTR *pTA;

    TRACE("(%p)->(%p, %s, %p)\n", This, pOuterUnk, debugstr_guid(riid), ppvObj);

    *ppvObj = NULL;

    if(pOuterUnk)
    {
        WARN("Not able to aggregate\n");
        return CLASS_E_NOAGGREGATION;
    }

    hr = ITypeInfo2_GetTypeAttr(iface, &pTA);
    if(FAILED(hr)) return hr;

    if(pTA->typekind != TKIND_COCLASS)
    {
        WARN("CreateInstance on typeinfo of type %x\n", pTA->typekind);
        hr = E_INVALIDARG;
        goto end;
    }

    hr = S_FALSE;
    if(pTA->wTypeFlags & TYPEFLAG_FAPPOBJECT)
    {
        IUnknown *pUnk;
        hr = GetActiveObject(&pTA->guid, NULL, &pUnk);
        TRACE("GetActiveObject rets %#lx.\n", hr);
        if(hr == S_OK)
        {
            hr = IUnknown_QueryInterface(pUnk, riid, ppvObj);
            IUnknown_Release(pUnk);
        }
    }

    if(hr != S_OK)
        hr = CoCreateInstance(&pTA->guid, NULL,
                              CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                              riid, ppvObj);

end:
    ITypeInfo2_ReleaseTypeAttr(iface, pTA);
    return hr;
}

/* ITypeInfo::GetMops
 *
 * Retrieves marshalling information.
 */
static HRESULT WINAPI ITypeInfo_fnGetMops( ITypeInfo2 *iface, MEMBERID memid, BSTR *pBstrMops)
{
    FIXME("%p, %ld stub!\n", iface, memid);
    *pBstrMops = NULL;
    return S_OK;
}

/* ITypeInfo::GetContainingTypeLib
 *
 * Retrieves the containing type library and the index of the type description
 * within that type library.
 */
static HRESULT WINAPI ITypeInfo_fnGetContainingTypeLib( ITypeInfo2 *iface,
        ITypeLib  * *ppTLib, UINT  *pIndex)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);

    /* If a pointer is null, we simply ignore it, the ATL in particular passes pIndex as 0 */
    if (pIndex) {
      *pIndex=This->index;
      TRACE("returning pIndex=%d\n", *pIndex);
    }

    if (ppTLib) {
      *ppTLib = (ITypeLib *)&This->pTypeLib->ITypeLib2_iface;
      ITypeLib_AddRef(*ppTLib);
      TRACE("returning ppTLib=%p\n", *ppTLib);
    }

    return S_OK;
}

/* ITypeInfo::ReleaseTypeAttr
 *
 * Releases a TYPEATTR previously returned by Get
 *
 */
static void WINAPI ITypeInfo_fnReleaseTypeAttr( ITypeInfo2 *iface,
        TYPEATTR* pTypeAttr)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    TRACE("(%p)->(%p)\n", This, pTypeAttr);
    free(pTypeAttr);
}

/* ITypeInfo::ReleaseFuncDesc
 *
 * Releases a FUNCDESC previously returned by GetFuncDesc. *
 */
static void WINAPI ITypeInfo_fnReleaseFuncDesc(
	ITypeInfo2 *iface,
        FUNCDESC *pFuncDesc)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    SHORT i;

    TRACE("(%p)->(%p)\n", This, pFuncDesc);

    for (i = 0; i < pFuncDesc->cParams; i++)
        TLB_FreeElemDesc(&pFuncDesc->lprgelemdescParam[i]);
    TLB_FreeElemDesc(&pFuncDesc->elemdescFunc);

    SysFreeString((BSTR)pFuncDesc);
}

/* ITypeInfo::ReleaseVarDesc
 *
 * Releases a VARDESC previously returned by GetVarDesc.
 */
static void WINAPI ITypeInfo_fnReleaseVarDesc( ITypeInfo2 *iface,
        VARDESC *pVarDesc)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    TRACE("(%p)->(%p)\n", This, pVarDesc);

    TLB_FreeVarDesc(pVarDesc);
}

/* ITypeInfo2::GetTypeKind
 *
 * Returns the TYPEKIND enumeration quickly, without doing any allocations.
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetTypeKind( ITypeInfo2 * iface,
    TYPEKIND *pTypeKind)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    *pTypeKind = This->typeattr.typekind;
    TRACE("(%p) type 0x%0x\n", This,*pTypeKind);
    return S_OK;
}

/* ITypeInfo2::GetTypeFlags
 *
 * Returns the type flags without any allocations. This returns a DWORD type
 * flag, which expands the type flags without growing the TYPEATTR (type
 * attribute).
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetTypeFlags( ITypeInfo2 *iface, ULONG *pTypeFlags)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    TRACE("%p, %p.\n", iface, pTypeFlags);
    *pTypeFlags=This->typeattr.wTypeFlags;
    return S_OK;
}

/* ITypeInfo2::GetFuncIndexOfMemId
 * Binds to a specific member based on a known DISPID, where the member name
 * is not known (for example, when binding to a default member).
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetFuncIndexOfMemId( ITypeInfo2 * iface,
    MEMBERID memid, INVOKEKIND invKind, UINT *pFuncIndex)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    UINT fdc;
    HRESULT result;

    for (fdc = 0; fdc < This->typeattr.cFuncs; ++fdc){
        const TLBFuncDesc *pFuncInfo = &This->funcdescs[fdc];
        if(memid == pFuncInfo->funcdesc.memid && (invKind & pFuncInfo->funcdesc.invkind))
            break;
    }
    if(fdc < This->typeattr.cFuncs) {
        *pFuncIndex = fdc;
        result = S_OK;
    } else
        result = TYPE_E_ELEMENTNOTFOUND;

    TRACE("%p, %#lx, %#x, hr %#lx.\n", iface, memid, invKind, result);
    return result;
}

/* TypeInfo2::GetVarIndexOfMemId
 *
 * Binds to a specific member based on a known DISPID, where the member name
 * is not known (for example, when binding to a default member).
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetVarIndexOfMemId( ITypeInfo2 * iface,
    MEMBERID memid, UINT *pVarIndex)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    TLBVarDesc *pVarInfo;

    TRACE("%p, %ld, %p.\n", iface, memid, pVarIndex);

    pVarInfo = TLB_get_vardesc_by_memberid(This, memid);
    if(!pVarInfo)
        return TYPE_E_ELEMENTNOTFOUND;

    *pVarIndex = (pVarInfo - This->vardescs);

    return S_OK;
}

/* ITypeInfo2::GetCustData
 *
 * Gets the custom data
 */
static HRESULT WINAPI ITypeInfo2_fnGetCustData(
	ITypeInfo2 * iface,
	REFGUID guid,
	VARIANT *pVarVal)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    TLBCustData *pCData;

    TRACE("%p %s %p\n", This, debugstr_guid(guid), pVarVal);

    if(!guid || !pVarVal)
        return E_INVALIDARG;

    pCData = TLB_get_custdata_by_guid(This->pcustdata_list, guid);

    VariantInit( pVarVal);
    if (pCData)
        VariantCopy( pVarVal, &pCData->data);
    else
        VariantClear( pVarVal );
    return S_OK;
}

/* ITypeInfo2::GetFuncCustData
 *
 * Gets the custom data
 */
static HRESULT WINAPI ITypeInfo2_fnGetFuncCustData(
	ITypeInfo2 * iface,
	UINT index,
	REFGUID guid,
	VARIANT *pVarVal)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    const TLBFuncDesc *desc;
    TLBCustData *data;
    UINT hrefoffset;
    HRESULT hr;

    TRACE("%p %u %s %p\n", This, index, debugstr_guid(guid), pVarVal);

    hr = ITypeInfoImpl_GetInternalFuncDesc((ITypeInfo *)iface, index, &desc, &hrefoffset);
    if (FAILED(hr))
    {
        WARN("description for function %d not found\n", index);
        return hr;
    }

    VariantInit(pVarVal);
    data = TLB_get_custdata_by_guid(&desc->custdata_list, guid);
    return data ? VariantCopy(pVarVal, &data->data) : S_OK;
}

/* ITypeInfo2::GetParamCustData
 *
 * Gets the custom data
 */
static HRESULT WINAPI ITypeInfo2_fnGetParamCustData(
	ITypeInfo2 * iface,
	UINT indexFunc,
	UINT indexParam,
	REFGUID guid,
	VARIANT *pVarVal)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    const TLBFuncDesc *pFDesc;
    TLBCustData *pCData;
    UINT hrefoffset;
    HRESULT hr;

    TRACE("%p %u %u %s %p\n", This, indexFunc, indexParam,
            debugstr_guid(guid), pVarVal);

    hr = ITypeInfoImpl_GetInternalFuncDesc((ITypeInfo *)iface, indexFunc, &pFDesc, &hrefoffset);
    if (FAILED(hr))
        return hr;

    if(indexParam >= pFDesc->funcdesc.cParams)
        return TYPE_E_ELEMENTNOTFOUND;

    pCData = TLB_get_custdata_by_guid(&pFDesc->pParamDesc[indexParam].custdata_list, guid);
    if(!pCData)
        return TYPE_E_ELEMENTNOTFOUND;

    VariantInit(pVarVal);
    VariantCopy(pVarVal, &pCData->data);

    return S_OK;
}

/* ITypeInfo2::GetVarCustData
 *
 * Gets the custom data
 */
static HRESULT WINAPI ITypeInfo2_fnGetVarCustData(
	ITypeInfo2 * iface,
	UINT index,
	REFGUID guid,
	VARIANT *pVarVal)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    TLBCustData *pCData;
    TLBVarDesc *pVDesc = &This->vardescs[index];

    TRACE("%p %s %p\n", This, debugstr_guid(guid), pVarVal);

    if(index >= This->typeattr.cVars)
        return TYPE_E_ELEMENTNOTFOUND;

    pCData = TLB_get_custdata_by_guid(&pVDesc->custdata_list, guid);
    if(!pCData)
        return TYPE_E_ELEMENTNOTFOUND;

    VariantInit(pVarVal);
    VariantCopy(pVarVal, &pCData->data);

    return S_OK;
}

/* ITypeInfo2::GetImplCustData
 *
 * Gets the custom data
 */
static HRESULT WINAPI ITypeInfo2_fnGetImplTypeCustData(
	ITypeInfo2 * iface,
	UINT index,
	REFGUID guid,
	VARIANT *pVarVal)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    TLBCustData *pCData;
    TLBImplType *pRDesc = &This->impltypes[index];

    TRACE("%p %u %s %p\n", This, index, debugstr_guid(guid), pVarVal);

    if(index >= This->typeattr.cImplTypes)
        return TYPE_E_ELEMENTNOTFOUND;

    pCData = TLB_get_custdata_by_guid(&pRDesc->custdata_list, guid);
    if(!pCData)
        return TYPE_E_ELEMENTNOTFOUND;

    VariantInit(pVarVal);
    VariantCopy(pVarVal, &pCData->data);

    return S_OK;
}

/* ITypeInfo2::GetDocumentation2
 *
 * Retrieves the documentation string, the complete Help file name and path,
 * the localization context to use, and the context ID for the library Help
 * topic in the Help file.
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetDocumentation2(
	ITypeInfo2 * iface,
	MEMBERID memid,
	LCID lcid,
	BSTR *pbstrHelpString,
	DWORD *pdwHelpStringContext,
	BSTR *pbstrHelpStringDll)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    const TLBFuncDesc *pFDesc;
    const TLBVarDesc *pVDesc;
    TRACE("%p, %ld, %#lx, %p, %p, %p.\n",
          iface, memid, lcid, pbstrHelpString, pdwHelpStringContext,
          pbstrHelpStringDll );
    /* the help string should be obtained from the helpstringdll,
     * using the _DLLGetDocumentation function, based on the supplied
     * lcid. Nice to do sometime...
     */
    if(memid==MEMBERID_NIL){ /* documentation for the typeinfo */
        if(pbstrHelpString)
            *pbstrHelpString=SysAllocString(TLB_get_bstr(This->Name));
        if(pdwHelpStringContext)
            *pdwHelpStringContext=This->dwHelpStringContext;
        if(pbstrHelpStringDll)
            *pbstrHelpStringDll=
                SysAllocString(TLB_get_bstr(This->pTypeLib->HelpStringDll));/* FIXME */
        return S_OK;
    }else {/* for a member */
        pFDesc = TLB_get_funcdesc_by_memberid(This, memid);
        if(pFDesc){
            if(pbstrHelpString)
                *pbstrHelpString=SysAllocString(TLB_get_bstr(pFDesc->HelpString));
            if(pdwHelpStringContext)
                *pdwHelpStringContext=pFDesc->HelpStringContext;
            if(pbstrHelpStringDll)
                *pbstrHelpStringDll=
                    SysAllocString(TLB_get_bstr(This->pTypeLib->HelpStringDll));/* FIXME */
            return S_OK;
        }
        pVDesc = TLB_get_vardesc_by_memberid(This, memid);
        if(pVDesc){
            if(pbstrHelpString)
                *pbstrHelpString=SysAllocString(TLB_get_bstr(pVDesc->HelpString));
            if(pdwHelpStringContext)
                *pdwHelpStringContext=pVDesc->HelpStringContext;
            if(pbstrHelpStringDll)
                *pbstrHelpStringDll=
                    SysAllocString(TLB_get_bstr(This->pTypeLib->HelpStringDll));/* FIXME */
            return S_OK;
        }
    }
    return TYPE_E_ELEMENTNOTFOUND;
}

/* ITypeInfo2::GetAllCustData
 *
 * Gets all custom data items for the Type info.
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllCustData(
	ITypeInfo2 * iface,
	CUSTDATA *pCustData)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);

    TRACE("%p %p\n", This, pCustData);

    return TLB_copy_all_custdata(This->pcustdata_list, pCustData);
}

/* ITypeInfo2::GetAllFuncCustData
 *
 * Gets all custom data items for the specified Function
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllFuncCustData(
	ITypeInfo2 * iface,
	UINT index,
	CUSTDATA *pCustData)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    const TLBFuncDesc *pFDesc;
    UINT hrefoffset;
    HRESULT hr;

    TRACE("%p %u %p\n", This, index, pCustData);

    hr = ITypeInfoImpl_GetInternalFuncDesc((ITypeInfo *)iface, index, &pFDesc, &hrefoffset);
    if (FAILED(hr))
        return hr;

    return TLB_copy_all_custdata(&pFDesc->custdata_list, pCustData);
}

/* ITypeInfo2::GetAllParamCustData
 *
 * Gets all custom data items for the Functions
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllParamCustData( ITypeInfo2 * iface,
    UINT indexFunc, UINT indexParam, CUSTDATA *pCustData)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    const TLBFuncDesc *pFDesc;
    UINT hrefoffset;
    HRESULT hr;

    TRACE("%p %u %u %p\n", This, indexFunc, indexParam, pCustData);

    hr = ITypeInfoImpl_GetInternalFuncDesc((ITypeInfo *)iface, indexFunc, &pFDesc, &hrefoffset);
    if (FAILED(hr))
        return hr;

    if(indexParam >= pFDesc->funcdesc.cParams)
        return TYPE_E_ELEMENTNOTFOUND;

    return TLB_copy_all_custdata(&pFDesc->pParamDesc[indexParam].custdata_list, pCustData);
}

/* ITypeInfo2::GetAllVarCustData
 *
 * Gets all custom data items for the specified Variable
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllVarCustData( ITypeInfo2 * iface,
    UINT index, CUSTDATA *pCustData)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    TLBVarDesc * pVDesc = &This->vardescs[index];

    TRACE("%p %u %p\n", This, index, pCustData);

    if(index >= This->typeattr.cVars)
        return TYPE_E_ELEMENTNOTFOUND;

    return TLB_copy_all_custdata(&pVDesc->custdata_list, pCustData);
}

/* ITypeInfo2::GetAllImplCustData
 *
 * Gets all custom data items for the specified implementation type
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllImplTypeCustData(
	ITypeInfo2 * iface,
	UINT index,
	CUSTDATA *pCustData)
{
    ITypeInfoImpl *This = impl_from_ITypeInfo2(iface);
    TLBImplType *pRDesc = &This->impltypes[index];

    TRACE("%p %u %p\n", This, index, pCustData);

    if(index >= This->typeattr.cImplTypes)
        return TYPE_E_ELEMENTNOTFOUND;

    return TLB_copy_all_custdata(&pRDesc->custdata_list, pCustData);
}

static const ITypeInfo2Vtbl tinfvt =
{

    ITypeInfo_fnQueryInterface,
    ITypeInfo_fnAddRef,
    ITypeInfo_fnRelease,

    ITypeInfo_fnGetTypeAttr,
    ITypeInfo_fnGetTypeComp,
    ITypeInfo_fnGetFuncDesc,
    ITypeInfo_fnGetVarDesc,
    ITypeInfo_fnGetNames,
    ITypeInfo_fnGetRefTypeOfImplType,
    ITypeInfo_fnGetImplTypeFlags,
    ITypeInfo_fnGetIDsOfNames,
    ITypeInfo_fnInvoke,
    ITypeInfo_fnGetDocumentation,
    ITypeInfo_fnGetDllEntry,
    ITypeInfo_fnGetRefTypeInfo,
    ITypeInfo_fnAddressOfMember,
    ITypeInfo_fnCreateInstance,
    ITypeInfo_fnGetMops,
    ITypeInfo_fnGetContainingTypeLib,
    ITypeInfo_fnReleaseTypeAttr,
    ITypeInfo_fnReleaseFuncDesc,
    ITypeInfo_fnReleaseVarDesc,

    ITypeInfo2_fnGetTypeKind,
    ITypeInfo2_fnGetTypeFlags,
    ITypeInfo2_fnGetFuncIndexOfMemId,
    ITypeInfo2_fnGetVarIndexOfMemId,
    ITypeInfo2_fnGetCustData,
    ITypeInfo2_fnGetFuncCustData,
    ITypeInfo2_fnGetParamCustData,
    ITypeInfo2_fnGetVarCustData,
    ITypeInfo2_fnGetImplTypeCustData,
    ITypeInfo2_fnGetDocumentation2,
    ITypeInfo2_fnGetAllCustData,
    ITypeInfo2_fnGetAllFuncCustData,
    ITypeInfo2_fnGetAllParamCustData,
    ITypeInfo2_fnGetAllVarCustData,
    ITypeInfo2_fnGetAllImplTypeCustData,
};

/******************************************************************************
 * CreateDispTypeInfo [OLEAUT32.31]
 *
 * Build type information for an object so it can be called through an
 * IDispatch interface.
 *
 * RETURNS
 *  Success: S_OK. pptinfo contains the created ITypeInfo object.
 *  Failure: E_INVALIDARG, if one or more arguments is invalid.
 *
 * NOTES
 *  This call allows an objects methods to be accessed through IDispatch, by
 *  building an ITypeInfo object that IDispatch can use to call through.
 */
HRESULT WINAPI CreateDispTypeInfo(
	INTERFACEDATA *pidata, /* [I] Description of the interface to build type info for */
	LCID lcid, /* [I] Locale Id */
	ITypeInfo **pptinfo) /* [O] Destination for created ITypeInfo object */
{
    ITypeInfoImpl *pTIClass, *pTIIface;
    ITypeLibImpl *pTypeLibImpl;
    unsigned int param, func;
    TLBFuncDesc *pFuncDesc;
    TLBRefType *ref;

    TRACE("\n");
    pTypeLibImpl = TypeLibImpl_Constructor();
    if (!pTypeLibImpl) return E_FAIL;

    pTypeLibImpl->TypeInfoCount = 2;
    pTypeLibImpl->typeinfos = calloc(pTypeLibImpl->TypeInfoCount, sizeof(ITypeInfoImpl*));

    pTIIface = pTypeLibImpl->typeinfos[0] = ITypeInfoImpl_Constructor();
    pTIIface->pTypeLib = pTypeLibImpl;
    pTIIface->index = 0;
    pTIIface->Name = NULL;
    pTIIface->dwHelpContext = -1;
    pTIIface->guid = NULL;
    pTIIface->typeattr.lcid = lcid;
    pTIIface->typeattr.typekind = TKIND_INTERFACE;
    pTIIface->typeattr.wMajorVerNum = 0;
    pTIIface->typeattr.wMinorVerNum = 0;
    pTIIface->typeattr.cbAlignment = 2;
    pTIIface->typeattr.cbSizeInstance = -1;
    pTIIface->typeattr.cbSizeVft = -1;
    pTIIface->typeattr.cFuncs = 0;
    pTIIface->typeattr.cImplTypes = 0;
    pTIIface->typeattr.cVars = 0;
    pTIIface->typeattr.wTypeFlags = 0;
    pTIIface->hreftype = 0;

    pTIIface->funcdescs = TLBFuncDesc_Alloc(pidata->cMembers);
    pFuncDesc = pTIIface->funcdescs;
    for(func = 0; func < pidata->cMembers; func++) {
        METHODDATA *md = pidata->pmethdata + func;
        pFuncDesc->Name = TLB_append_str(&pTypeLibImpl->name_list, md->szName);
        pFuncDesc->funcdesc.memid = md->dispid;
        pFuncDesc->funcdesc.lprgscode = NULL;
        pFuncDesc->funcdesc.funckind = FUNC_VIRTUAL;
        pFuncDesc->funcdesc.invkind = md->wFlags;
        pFuncDesc->funcdesc.callconv = md->cc;
        pFuncDesc->funcdesc.cParams = md->cArgs;
        pFuncDesc->funcdesc.cParamsOpt = 0;
        pFuncDesc->funcdesc.oVft = md->iMeth * sizeof(void *);
        pFuncDesc->funcdesc.cScodes = 0;
        pFuncDesc->funcdesc.wFuncFlags = 0;
        pFuncDesc->funcdesc.elemdescFunc.tdesc.vt = md->vtReturn;
        pFuncDesc->funcdesc.elemdescFunc.paramdesc.wParamFlags = PARAMFLAG_NONE;
        pFuncDesc->funcdesc.elemdescFunc.paramdesc.pparamdescex = NULL;
        pFuncDesc->funcdesc.lprgelemdescParam = calloc(md->cArgs, sizeof(ELEMDESC));
        pFuncDesc->pParamDesc = TLBParDesc_Constructor(md->cArgs);
        for(param = 0; param < md->cArgs; param++) {
            pFuncDesc->funcdesc.lprgelemdescParam[param].tdesc.vt = md->ppdata[param].vt;
            pFuncDesc->pParamDesc[param].Name = TLB_append_str(&pTypeLibImpl->name_list, md->ppdata[param].szName);
        }
        pFuncDesc->helpcontext = 0;
        pFuncDesc->HelpStringContext = 0;
        pFuncDesc->HelpString = NULL;
        pFuncDesc->Entry = NULL;
        list_init(&pFuncDesc->custdata_list);
        pTIIface->typeattr.cFuncs++;
        ++pFuncDesc;
    }

    dump_TypeInfo(pTIIface);

    pTIClass = pTypeLibImpl->typeinfos[1] = ITypeInfoImpl_Constructor();
    pTIClass->pTypeLib = pTypeLibImpl;
    pTIClass->index = 1;
    pTIClass->Name = NULL;
    pTIClass->dwHelpContext = -1;
    pTIClass->guid = NULL;
    pTIClass->typeattr.lcid = lcid;
    pTIClass->typeattr.typekind = TKIND_COCLASS;
    pTIClass->typeattr.wMajorVerNum = 0;
    pTIClass->typeattr.wMinorVerNum = 0;
    pTIClass->typeattr.cbAlignment = 2;
    pTIClass->typeattr.cbSizeInstance = -1;
    pTIClass->typeattr.cbSizeVft = -1;
    pTIClass->typeattr.cFuncs = 0;
    pTIClass->typeattr.cImplTypes = 1;
    pTIClass->typeattr.cVars = 0;
    pTIClass->typeattr.wTypeFlags = 0;
    pTIClass->hreftype = sizeof(MSFT_TypeInfoBase);

    pTIClass->impltypes = TLBImplType_Alloc(1);

    ref = calloc(1, sizeof(*ref));
    ref->pImpTLInfo = TLB_REF_INTERNAL;
    list_add_head(&pTypeLibImpl->ref_list, &ref->entry);

    dump_TypeInfo(pTIClass);

    *pptinfo = (ITypeInfo *)&pTIClass->ITypeInfo2_iface;

    ITypeInfo_AddRef(*pptinfo);
    ITypeLib2_Release(&pTypeLibImpl->ITypeLib2_iface);

    return S_OK;

}

static HRESULT WINAPI ITypeComp_fnQueryInterface(ITypeComp * iface, REFIID riid, LPVOID * ppv)
{
    ITypeInfoImpl *This = info_impl_from_ITypeComp(iface);

    return ITypeInfo2_QueryInterface(&This->ITypeInfo2_iface, riid, ppv);
}

static ULONG WINAPI ITypeComp_fnAddRef(ITypeComp * iface)
{
    ITypeInfoImpl *This = info_impl_from_ITypeComp(iface);

    return ITypeInfo2_AddRef(&This->ITypeInfo2_iface);
}

static ULONG WINAPI ITypeComp_fnRelease(ITypeComp * iface)
{
    ITypeInfoImpl *This = info_impl_from_ITypeComp(iface);

    return ITypeInfo2_Release(&This->ITypeInfo2_iface);
}

static HRESULT WINAPI ITypeComp_fnBind(
    ITypeComp * iface,
    OLECHAR * szName,
    ULONG lHash,
    WORD wFlags,
    ITypeInfo ** ppTInfo,
    DESCKIND * pDescKind,
    BINDPTR * pBindPtr)
{
    ITypeInfoImpl *This = info_impl_from_ITypeComp(iface);
    const TLBFuncDesc *pFDesc;
    const TLBVarDesc *pVDesc;
    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    UINT fdc;

    TRACE("%p, %s, %#lx, 0x%x, %p, %p, %p.\n", iface, debugstr_w(szName), lHash, wFlags, ppTInfo, pDescKind, pBindPtr);

    *pDescKind = DESCKIND_NONE;
    pBindPtr->lpfuncdesc = NULL;
    *ppTInfo = NULL;

    for(fdc = 0; fdc < This->typeattr.cFuncs; ++fdc){
        pFDesc = &This->funcdescs[fdc];
        if (!lstrcmpiW(TLB_get_bstr(pFDesc->Name), szName)) {
            if (!wFlags || (pFDesc->funcdesc.invkind & wFlags))
                break;
            else
                /* name found, but wrong flags */
                hr = TYPE_E_TYPEMISMATCH;
        }
    }

    if (fdc < This->typeattr.cFuncs)
    {
        HRESULT hr = TLB_AllocAndInitFuncDesc(
            &pFDesc->funcdesc,
            &pBindPtr->lpfuncdesc,
            This->typeattr.typekind == TKIND_DISPATCH);
        if (FAILED(hr))
            return hr;
        *pDescKind = DESCKIND_FUNCDESC;
        *ppTInfo = (ITypeInfo *)&This->ITypeInfo2_iface;
        ITypeInfo_AddRef(*ppTInfo);
        return S_OK;
    } else {
        pVDesc = TLB_get_vardesc_by_name(This, szName);
        if(pVDesc){
            HRESULT hr = TLB_AllocAndInitVarDesc(&pVDesc->vardesc, &pBindPtr->lpvardesc);
            if (FAILED(hr))
                return hr;
            *pDescKind = DESCKIND_VARDESC;
            *ppTInfo = (ITypeInfo *)&This->ITypeInfo2_iface;
            ITypeInfo_AddRef(*ppTInfo);
            return S_OK;
        }
    }

    if (hr == DISP_E_MEMBERNOTFOUND && This->impltypes) {
        /* recursive search */
        ITypeInfo *pTInfo;
        ITypeComp *pTComp;
        HRESULT hr;
        hr=ITypeInfo2_GetRefTypeInfo(&This->ITypeInfo2_iface, This->impltypes[0].hRef, &pTInfo);
        if (SUCCEEDED(hr))
        {
            hr = ITypeInfo_GetTypeComp(pTInfo,&pTComp);
            ITypeInfo_Release(pTInfo);
        }
        if (SUCCEEDED(hr))
        {
            hr = ITypeComp_Bind(pTComp, szName, lHash, wFlags, ppTInfo, pDescKind, pBindPtr);
            ITypeComp_Release(pTComp);
            if (SUCCEEDED(hr) && *pDescKind == DESCKIND_FUNCDESC &&
                    This->typeattr.typekind == TKIND_DISPATCH)
            {
                FUNCDESC *tmp = pBindPtr->lpfuncdesc;
                hr = TLB_AllocAndInitFuncDesc(tmp, &pBindPtr->lpfuncdesc, TRUE);
                SysFreeString((BSTR)tmp);
            }
            return hr;
        }
        WARN("Could not search inherited interface!\n");
    }
    if (hr == DISP_E_MEMBERNOTFOUND)
        hr = S_OK;
    TRACE("did not find member with name %s, flags 0x%x\n", debugstr_w(szName), wFlags);
    return hr;
}

static HRESULT WINAPI ITypeComp_fnBindType(
    ITypeComp * iface,
    OLECHAR * szName,
    ULONG lHash,
    ITypeInfo ** ppTInfo,
    ITypeComp ** ppTComp)
{
    TRACE("%s, %#lx, %p, %p.\n", debugstr_w(szName), lHash, ppTInfo, ppTComp);

    /* strange behaviour (does nothing) but like the
     * original */

    if (!ppTInfo || !ppTComp)
        return E_POINTER;

    *ppTInfo = NULL;
    *ppTComp = NULL;

    return S_OK;
}

static const ITypeCompVtbl tcompvt =
{

    ITypeComp_fnQueryInterface,
    ITypeComp_fnAddRef,
    ITypeComp_fnRelease,

    ITypeComp_fnBind,
    ITypeComp_fnBindType
};

HRESULT WINAPI CreateTypeLib2(SYSKIND syskind, LPCOLESTR szFile,
        ICreateTypeLib2** ppctlib)
{
    ITypeLibImpl *This;
    HRESULT hres;

    TRACE("(%d,%s,%p)\n", syskind, debugstr_w(szFile), ppctlib);

    if (!szFile) return E_INVALIDARG;

    This = TypeLibImpl_Constructor();
    if (!This)
        return E_OUTOFMEMORY;

    This->lcid = GetSystemDefaultLCID();
    This->syskind = syskind;
    This->ptr_size = get_ptr_size(syskind);

    This->path = wcsdup(szFile);
    if (!This->path) {
        ITypeLib2_Release(&This->ITypeLib2_iface);
        return E_OUTOFMEMORY;
    }

    hres = ITypeLib2_QueryInterface(&This->ITypeLib2_iface, &IID_ICreateTypeLib2, (LPVOID*)ppctlib);
    ITypeLib2_Release(&This->ITypeLib2_iface);
    return hres;
}

static HRESULT WINAPI ICreateTypeLib2_fnQueryInterface(ICreateTypeLib2 *iface,
        REFIID riid, void **object)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);

    return ITypeLib2_QueryInterface(&This->ITypeLib2_iface, riid, object);
}

static ULONG WINAPI ICreateTypeLib2_fnAddRef(ICreateTypeLib2 *iface)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);

    return ITypeLib2_AddRef(&This->ITypeLib2_iface);
}

static ULONG WINAPI ICreateTypeLib2_fnRelease(ICreateTypeLib2 *iface)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);

    return ITypeLib2_Release(&This->ITypeLib2_iface);
}

static HRESULT WINAPI ICreateTypeLib2_fnCreateTypeInfo(ICreateTypeLib2 *iface,
        LPOLESTR name, TYPEKIND kind, ICreateTypeInfo **ctinfo)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);
    ITypeInfoImpl *info;
    HRESULT hres;

    TRACE("%p %s %d %p\n", This, wine_dbgstr_w(name), kind, ctinfo);

    if (!ctinfo || !name)
        return E_INVALIDARG;

    info = TLB_get_typeinfo_by_name(This, name);
    if (info)
        return TYPE_E_NAMECONFLICT;

    This->typeinfos = realloc(This->typeinfos, sizeof(ITypeInfoImpl*) * (This->TypeInfoCount + 1));

    info = This->typeinfos[This->TypeInfoCount] = ITypeInfoImpl_Constructor();

    info->pTypeLib = This;
    info->Name = TLB_append_str(&This->name_list, name);
    info->index = This->TypeInfoCount;
    info->typeattr.typekind = kind;
    info->typeattr.cbAlignment = 4;

    switch (info->typeattr.typekind) {
    case TKIND_ENUM:
    case TKIND_INTERFACE:
    case TKIND_DISPATCH:
    case TKIND_COCLASS:
        info->typeattr.cbSizeInstance = This->ptr_size;
        break;
    case TKIND_RECORD:
    case TKIND_UNION:
        info->typeattr.cbSizeInstance = 0;
        break;
    case TKIND_MODULE:
        info->typeattr.cbSizeInstance = 2;
        break;
    case TKIND_ALIAS:
        info->typeattr.cbSizeInstance = -0x75;
        break;
    default:
        FIXME("unrecognized typekind %d\n", info->typeattr.typekind);
        info->typeattr.cbSizeInstance = 0xdeadbeef;
        break;
    }

    hres = ITypeInfo2_QueryInterface(&info->ITypeInfo2_iface,
            &IID_ICreateTypeInfo, (void **)ctinfo);
    if (FAILED(hres)) {
        ITypeInfo2_Release(&info->ITypeInfo2_iface);
        return hres;
    }

    info->hreftype = info->index * sizeof(MSFT_TypeInfoBase);

    ++This->TypeInfoCount;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeLib2_fnSetName(ICreateTypeLib2 *iface,
        LPOLESTR name)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);

    TRACE("%p %s\n", This, wine_dbgstr_w(name));

    if (!name)
        return E_INVALIDARG;

    This->Name = TLB_append_str(&This->name_list, name);

    return S_OK;
}

static HRESULT WINAPI ICreateTypeLib2_fnSetVersion(ICreateTypeLib2 *iface,
        WORD majorVerNum, WORD minorVerNum)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);

    TRACE("%p %d %d\n", This, majorVerNum, minorVerNum);

    This->ver_major = majorVerNum;
    This->ver_minor = minorVerNum;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeLib2_fnSetGuid(ICreateTypeLib2 *iface,
        REFGUID guid)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);

    TRACE("%p %s\n", This, debugstr_guid(guid));

    This->guid = TLB_append_guid(&This->guid_list, guid, -2);

    return S_OK;
}

static HRESULT WINAPI ICreateTypeLib2_fnSetDocString(ICreateTypeLib2 *iface,
        LPOLESTR doc)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);

    TRACE("%p %s\n", This, wine_dbgstr_w(doc));

    if (!doc)
        return E_INVALIDARG;

    This->DocString = TLB_append_str(&This->string_list, doc);

    return S_OK;
}

static HRESULT WINAPI ICreateTypeLib2_fnSetHelpFileName(ICreateTypeLib2 *iface,
        LPOLESTR helpFileName)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);

    TRACE("%p %s\n", This, wine_dbgstr_w(helpFileName));

    if (!helpFileName)
        return E_INVALIDARG;

    This->HelpFile = TLB_append_str(&This->string_list, helpFileName);

    return S_OK;
}

static HRESULT WINAPI ICreateTypeLib2_fnSetHelpContext(ICreateTypeLib2 *iface,
        DWORD helpContext)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);

    TRACE("%p, %ld.\n", iface, helpContext);

    This->dwHelpContext = helpContext;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeLib2_fnSetLcid(ICreateTypeLib2 *iface,
        LCID lcid)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);

    TRACE("%p, %#lx.\n", iface, lcid);

    This->set_lcid = lcid;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeLib2_fnSetLibFlags(ICreateTypeLib2 *iface,
        UINT libFlags)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);

    TRACE("%p %x\n", This, libFlags);

    This->libflags = libFlags;

    return S_OK;
}

typedef struct tagWMSFT_SegContents {
    DWORD len;
    void *data;
} WMSFT_SegContents;

typedef struct tagWMSFT_TLBFile {
    MSFT_Header header;
    WMSFT_SegContents typeinfo_seg;
    WMSFT_SegContents impfile_seg;
    WMSFT_SegContents impinfo_seg;
    WMSFT_SegContents ref_seg;
    WMSFT_SegContents guidhash_seg;
    WMSFT_SegContents guid_seg;
    WMSFT_SegContents namehash_seg;
    WMSFT_SegContents name_seg;
    WMSFT_SegContents string_seg;
    WMSFT_SegContents typdesc_seg;
    WMSFT_SegContents arraydesc_seg;
    WMSFT_SegContents custdata_seg;
    WMSFT_SegContents cdguids_seg;
    MSFT_SegDir segdir;
    WMSFT_SegContents aux_seg;
} WMSFT_TLBFile;

static HRESULT WMSFT_compile_strings(ITypeLibImpl *This,
        WMSFT_TLBFile *file)
{
    TLBString *str;
    UINT last_offs;
    char *data;

    file->string_seg.len = 0;
    LIST_FOR_EACH_ENTRY(str, &This->string_list, TLBString, entry) {
        int size;

        size = WideCharToMultiByte(CP_ACP, 0, str->str, lstrlenW(str->str), NULL, 0, NULL, NULL);
        if (size == 0)
            return E_UNEXPECTED;

        size += sizeof(INT16);
        if (size % 4)
            size = (size + 4) & ~0x3;
        if (size < 8)
            size = 8;

        file->string_seg.len += size;

        /* temporarily use str->offset to store the length of the aligned,
         * converted string */
        str->offset = size;
    }

    file->string_seg.data = data = malloc(file->string_seg.len);

    last_offs = 0;
    LIST_FOR_EACH_ENTRY(str, &This->string_list, TLBString, entry) {
        int size;

        size = WideCharToMultiByte(CP_ACP, 0, str->str, lstrlenW(str->str),
                data + sizeof(INT16), file->string_seg.len - last_offs - sizeof(INT16), NULL, NULL);
        if (size == 0) {
            free(file->string_seg.data);
            file->string_seg.data = NULL;
            return E_UNEXPECTED;
        }

        *((INT16*)data) = size;

        memset(data + sizeof(INT16) + size, 0x57, str->offset - size - sizeof(INT16));

        size = str->offset;
        data += size;
        str->offset = last_offs;
        last_offs += size;
    }

    return S_OK;
}

static HRESULT WMSFT_compile_names(ITypeLibImpl *This,
        WMSFT_TLBFile *file)
{
    TLBString *str;
    UINT last_offs;
    char *data;
    MSFT_NameIntro *last_intro = NULL;

    file->header.nametablecount = 0;
    file->header.nametablechars = 0;

    file->name_seg.len = 0;
    LIST_FOR_EACH_ENTRY(str, &This->name_list, TLBString, entry) {
        int size;

        size = lstrlenW(str->str);
        file->header.nametablechars += size;
        file->header.nametablecount++;

        size = WideCharToMultiByte(CP_ACP, 0, str->str, size, NULL, 0, NULL, NULL);
        if (size == 0)
            return E_UNEXPECTED;

        size += sizeof(MSFT_NameIntro);
        if (size % 4)
            size = (size + 4) & ~0x3;
        if (size < 8)
            size = 8;

        file->name_seg.len += size;

        /* temporarily use str->offset to store the length of the aligned,
         * converted string */
        str->offset = size;
    }

    /* Allocate bigger buffer so we can temporarily NULL terminate the name */
    file->name_seg.data = data = malloc(file->name_seg.len + 1);

    last_offs = 0;
    LIST_FOR_EACH_ENTRY(str, &This->name_list, TLBString, entry) {
        int size, hash;
        MSFT_NameIntro *intro = (MSFT_NameIntro*)data;

        size = WideCharToMultiByte(CP_ACP, 0, str->str, lstrlenW(str->str),
                data + sizeof(MSFT_NameIntro),
                file->name_seg.len - last_offs - sizeof(MSFT_NameIntro), NULL, NULL);
        if (size == 0) {
            free(file->name_seg.data);
            return E_UNEXPECTED;
        }
        data[sizeof(MSFT_NameIntro) + size] = '\0';

        intro->hreftype = -1; /* TODO? */
        intro->namelen = size & 0xFF;
        /* TODO: namelen & 0xFF00 == ??? maybe HREF type indicator? */
        hash = LHashValOfNameSysA(This->syskind, This->lcid, data + sizeof(MSFT_NameIntro));
        intro->namelen |= hash << 16;
        intro->next_hash = ((DWORD*)file->namehash_seg.data)[hash & 0x7f];
        ((DWORD*)file->namehash_seg.data)[hash & 0x7f] = last_offs;

        memset(data + sizeof(MSFT_NameIntro) + size, 0x57,
                str->offset - size - sizeof(MSFT_NameIntro));

        /* update str->offset to actual value to use in other
         * compilation functions that require positions within
         * the string table */
        last_intro = intro;
        size = str->offset;
        data += size;
        str->offset = last_offs;
        last_offs += size;
    }

    if(last_intro)
        last_intro->hreftype = 0; /* last one is 0? */

    return S_OK;
}

static inline int hash_guid(GUID *guid)
{
    int i, hash = 0;

    for (i = 0; i < 8; i ++)
        hash ^= ((const short *)guid)[i];

    return hash & 0x1f;
}

static HRESULT WMSFT_compile_guids(ITypeLibImpl *This, WMSFT_TLBFile *file)
{
    TLBGuid *guid;
    MSFT_GuidEntry *entry;
    DWORD offs;
    int hash_key, *guidhashtab;

    file->guid_seg.len = sizeof(MSFT_GuidEntry) * list_count(&This->guid_list);
    file->guid_seg.data = malloc(file->guid_seg.len);

    entry = file->guid_seg.data;
    offs = 0;
    guidhashtab = file->guidhash_seg.data;
    LIST_FOR_EACH_ENTRY(guid, &This->guid_list, TLBGuid, entry){
        memcpy(&entry->guid, &guid->guid, sizeof(GUID));
        entry->hreftype = guid->hreftype;

        hash_key = hash_guid(&guid->guid);
        entry->next_hash = guidhashtab[hash_key];
        guidhashtab[hash_key] = offs;

        guid->offset = offs;
        offs += sizeof(MSFT_GuidEntry);
        ++entry;
    }

    return S_OK;
}

static DWORD WMSFT_encode_variant(VARIANT *value, WMSFT_TLBFile *file)
{
    VARIANT v = *value;
    VARTYPE arg_type = V_VT(value);
    int mask = 0;
    HRESULT hres;
    DWORD ret = file->custdata_seg.len;

    if(arg_type == VT_INT)
        arg_type = VT_I4;
    if(arg_type == VT_UINT)
        arg_type = VT_UI4;

    v = *value;
    if(V_VT(value) != arg_type) {
        hres = VariantChangeType(&v, value, 0, arg_type);
        if(FAILED(hres)){
            ERR("VariantChangeType failed: %#lx.\n", hres);
            return -1;
        }
    }

    /* Check if default value can be stored in-place */
    switch(arg_type){
    case VT_I4:
    case VT_UI4:
        mask = 0x3ffffff;
        if(V_UI4(&v) > 0x3ffffff)
            break;
        /* fall through */
    case VT_I1:
    case VT_UI1:
    case VT_BOOL:
        if(!mask)
            mask = 0xff;
        /* fall through */
    case VT_I2:
    case VT_UI2:
        if(!mask)
            mask = 0xffff;
        return ((0x80 + 0x4 * V_VT(value)) << 24) | (V_UI4(&v) & mask);
    }

    /* have to allocate space in custdata_seg */
    switch(arg_type) {
    case VT_I4:
    case VT_R4:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_HRESULT:
    case VT_PTR: {
        /* Construct the data to be allocated */
        int *data;

        if(file->custdata_seg.data){
            file->custdata_seg.data = realloc(file->custdata_seg.data, file->custdata_seg.len + sizeof(int) * 2);
            data = (int *)(((char *)file->custdata_seg.data) + file->custdata_seg.len);
            file->custdata_seg.len += sizeof(int) * 2;
        }else{
            file->custdata_seg.len = sizeof(int) * 2;
            data = file->custdata_seg.data = malloc(file->custdata_seg.len);
        }

        data[0] = V_VT(value) + (V_UI4(&v) << 16);
        data[1] = (V_UI4(&v) >> 16) + 0x57570000;

        /* TODO: Check if the encoded data is already present in custdata_seg */

        return ret;
    }

    case VT_BSTR: {
        int mb_len = WideCharToMultiByte(CP_ACP, 0, V_BSTR(&v), SysStringLen(V_BSTR(&v)), NULL, 0, NULL, NULL );
        int i, len = (6 + mb_len + 3) & ~0x3;
        char *data;

        if(file->custdata_seg.data){
            file->custdata_seg.data = realloc(file->custdata_seg.data, file->custdata_seg.len + len);
            data = ((char *)file->custdata_seg.data) + file->custdata_seg.len;
            file->custdata_seg.len += len;
        }else{
            file->custdata_seg.len = len;
            data = file->custdata_seg.data = malloc(file->custdata_seg.len);
        }

        *((unsigned short *)data) = V_VT(value);
        *((unsigned int *)(data+2)) = mb_len;
        WideCharToMultiByte(CP_ACP, 0, V_BSTR(&v), SysStringLen(V_BSTR(&v)), &data[6], mb_len, NULL, NULL);
        for (i = 6 + mb_len; i < len; i++)
            data[i] = 0x57;

        /* TODO: Check if the encoded data is already present in custdata_seg */

        return ret;
    }
    default:
        FIXME("Argument type not yet handled\n");
        return -1;
    }
}

static DWORD WMSFT_append_typedesc(TYPEDESC *desc, WMSFT_TLBFile *file, DWORD *out_mix, INT16 *out_size);

static DWORD WMSFT_append_arraydesc(ARRAYDESC *desc, WMSFT_TLBFile *file)
{
    DWORD offs = file->arraydesc_seg.len;
    DWORD *encoded;
    USHORT i;

    /* TODO: we should check for duplicates, but that's harder because each
     * chunk is variable length (really we should store TYPEDESC and ARRAYDESC
     * at the library-level) */

    file->arraydesc_seg.len += (2 + desc->cDims * 2) * sizeof(DWORD);
    file->arraydesc_seg.data = realloc(file->arraydesc_seg.data, file->arraydesc_seg.len);
    encoded = (DWORD*)((char *)file->arraydesc_seg.data + offs);

    encoded[0] = WMSFT_append_typedesc(&desc->tdescElem, file, NULL, NULL);
    encoded[1] = desc->cDims | ((desc->cDims * 2 * sizeof(DWORD)) << 16);
    for(i = 0; i < desc->cDims; ++i){
        encoded[2 + i * 2] =  desc->rgbounds[i].cElements;
        encoded[2 + i * 2 + 1] = desc->rgbounds[i].lLbound;
    }

    return offs;
}

static DWORD WMSFT_append_typedesc(TYPEDESC *desc, WMSFT_TLBFile *file, DWORD *out_mix, INT16 *out_size)
{
    DWORD junk;
    INT16 junk2;
    DWORD offs = 0;
    DWORD encoded[2];
    VARTYPE vt, subtype;
    char *data;

    if(!desc)
        return -1;

    if(!out_mix)
        out_mix = &junk;
    if(!out_size)
        out_size = &junk2;

    vt = desc->vt & VT_TYPEMASK;

    if(vt == VT_PTR || vt == VT_SAFEARRAY){
        DWORD mix;
        encoded[1] = WMSFT_append_typedesc(desc->lptdesc, file, &mix, out_size);
        encoded[0] = desc->vt | ((mix | VT_BYREF) << 16);
        *out_mix = 0x7FFF;
        *out_size += 2 * sizeof(DWORD);
    }else if(vt == VT_CARRAY){
        encoded[0] = desc->vt | (0x7FFE << 16);
        encoded[1] = WMSFT_append_arraydesc(desc->lpadesc, file);
        *out_mix = 0x7FFE;
    }else if(vt == VT_USERDEFINED){
        encoded[0] = desc->vt | (0x7FFF << 16);
        encoded[1] = desc->hreftype;
        *out_mix = 0x7FFF; /* FIXME: Should get TYPEKIND of the hreftype, e.g. TKIND_ENUM => VT_I4 */
    }else{
        TRACE("Mixing in-place, VT: 0x%x\n", desc->vt);

        switch(vt){
        case VT_INT:
            subtype = VT_I4;
            break;
        case VT_UINT:
            subtype = VT_UI4;
            break;
        case VT_VOID:
            subtype = VT_EMPTY;
            break;
        default:
            subtype = vt;
            break;
        }

        *out_mix = subtype;
        return 0x80000000 | (subtype << 16) | desc->vt;
    }

    data = file->typdesc_seg.data;
    while(offs < file->typdesc_seg.len){
        if(!memcmp(&data[offs], encoded, sizeof(encoded)))
            return offs;
        offs += sizeof(encoded);
    }

    file->typdesc_seg.len += sizeof(encoded);
    data = file->typdesc_seg.data = realloc(file->typdesc_seg.data, file->typdesc_seg.len);

    memcpy(&data[offs], encoded, sizeof(encoded));

    return offs;
}

static DWORD WMSFT_compile_custdata(struct list *custdata_list, WMSFT_TLBFile *file)
{
    WMSFT_SegContents *cdguids_seg = &file->cdguids_seg;
    DWORD ret = cdguids_seg->len, offs;
    MSFT_CDGuid *cdguid;
    TLBCustData *cd;

    if(list_empty(custdata_list))
        return -1;

    cdguids_seg->len += sizeof(MSFT_CDGuid) * list_count(custdata_list);
    cdguids_seg->data = realloc(cdguids_seg->data, cdguids_seg->len);
    cdguid = (MSFT_CDGuid*)((char*)cdguids_seg->data + ret);

    offs = ret + sizeof(MSFT_CDGuid);
    LIST_FOR_EACH_ENTRY(cd, custdata_list, TLBCustData, entry){
        cdguid->GuidOffset = cd->guid->offset;
        cdguid->DataOffset = WMSFT_encode_variant(&cd->data, file);
        cdguid->next = offs;
        offs += sizeof(MSFT_CDGuid);
        ++cdguid;
    }

    --cdguid;
    cdguid->next = -1;

    return ret;
}

static DWORD WMSFT_compile_typeinfo_aux(ITypeInfoImpl *info,
        WMSFT_TLBFile *file)
{
    WMSFT_SegContents *aux_seg = &file->aux_seg;
    DWORD ret = aux_seg->len, i, j, recorded_size = 0, extra_size = 0;
    MSFT_VarRecord *varrecord;
    MSFT_FuncRecord *funcrecord;
    MEMBERID *memid;
    DWORD *name, *offsets, offs;

    for(i = 0; i < info->typeattr.cFuncs; ++i){
        TLBFuncDesc *desc = &info->funcdescs[i];

        recorded_size += 6 * sizeof(INT); /* mandatory fields */

        /* optional fields */
        /* TODO: oArgCustData - FuncSetCustData not impl yet */
        if(!list_empty(&desc->custdata_list))
            recorded_size += 7 * sizeof(INT);
        else if(desc->HelpStringContext != 0)
            recorded_size += 6 * sizeof(INT);
        /* res9? resA? */
        else if(desc->Entry)
            recorded_size += 3 * sizeof(INT);
        else if(desc->HelpString)
            recorded_size += 2 * sizeof(INT);
        else if(desc->helpcontext)
            recorded_size += sizeof(INT);

        recorded_size += desc->funcdesc.cParams * sizeof(MSFT_ParameterInfo);

        for(j = 0; j < desc->funcdesc.cParams; ++j){
            if(desc->funcdesc.lprgelemdescParam[j].paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT){
                recorded_size += desc->funcdesc.cParams * sizeof(INT);
                break;
            }
        }

        extra_size += 2 * sizeof(INT); /* memberid, name offs */
    }

    for(i = 0; i < info->typeattr.cVars; ++i){
        TLBVarDesc *desc = &info->vardescs[i];

        recorded_size += 5 * sizeof(INT); /* mandatory fields */

        /* optional fields */
        if(desc->HelpStringContext != 0)
            recorded_size += 5 * sizeof(INT);
        else if(!list_empty(&desc->custdata_list))
            recorded_size += 4 * sizeof(INT);
        /* res9? */
        else if(desc->HelpString)
            recorded_size += 2 * sizeof(INT);
        else if(desc->HelpContext != 0)
            recorded_size += sizeof(INT);

        extra_size += 2 * sizeof(INT); /* memberid, name offs */
    }

    if(!recorded_size && !extra_size)
        return ret;

    extra_size += sizeof(INT); /* total aux size for this typeinfo */

    aux_seg->len += recorded_size + extra_size;

    aux_seg->len += sizeof(INT) * (info->typeattr.cVars + info->typeattr.cFuncs); /* offsets at the end */

    aux_seg->data = realloc(aux_seg->data, aux_seg->len);

    *((DWORD*)((char *)aux_seg->data + ret)) = recorded_size;

    offsets = (DWORD*)((char *)aux_seg->data + ret + recorded_size + extra_size);
    offs = 0;

    funcrecord = (MSFT_FuncRecord*)(((char *)aux_seg->data) + ret + sizeof(INT));
    for(i = 0; i < info->typeattr.cFuncs; ++i){
        TLBFuncDesc *desc = &info->funcdescs[i];
        DWORD size = 6 * sizeof(INT), paramdefault_size = 0, *paramdefault;

        funcrecord->funcdescsize = sizeof(desc->funcdesc) + desc->funcdesc.cParams * sizeof(ELEMDESC);
        funcrecord->DataType = WMSFT_append_typedesc(&desc->funcdesc.elemdescFunc.tdesc, file, NULL, &funcrecord->funcdescsize);
        funcrecord->Flags = desc->funcdesc.wFuncFlags;
        funcrecord->VtableOffset = desc->funcdesc.oVft;

        /* FKCCIC:
         * XXXX XXXX XXXX XXXX  XXXX XXXX XXXX XXXX
         *                                      ^^^funckind
         *                                 ^^^ ^invkind
         *                                ^has_cust_data
         *                           ^^^^callconv
         *                         ^has_param_defaults
         *                        ^oEntry_is_intresource
         */
        funcrecord->FKCCIC =
            desc->funcdesc.funckind |
            (desc->funcdesc.invkind << 3) |
            (list_empty(&desc->custdata_list) ? 0 : 0x80) |
            (desc->funcdesc.callconv << 8);

        if(desc->Entry && desc->Entry != (TLBString*)-1 && IS_INTRESOURCE(desc->Entry))
            funcrecord->FKCCIC |= 0x2000;

        for(j = 0; j < desc->funcdesc.cParams; ++j){
            if(desc->funcdesc.lprgelemdescParam[j].paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT){
                paramdefault_size = sizeof(INT) * desc->funcdesc.cParams;
                funcrecord->funcdescsize += sizeof(PARAMDESCEX);
            }
        }
        if(paramdefault_size > 0)
            funcrecord->FKCCIC |= 0x1000;

        funcrecord->nrargs = desc->funcdesc.cParams;
        funcrecord->nroargs = desc->funcdesc.cParamsOpt;

        /* optional fields */
        /* res9? resA? */
        if(!list_empty(&desc->custdata_list)){
            size += 7 * sizeof(INT);
            funcrecord->HelpContext = desc->helpcontext;
            if(desc->HelpString)
                funcrecord->oHelpString = desc->HelpString->offset;
            else
                funcrecord->oHelpString = -1;
            if(!desc->Entry)
                funcrecord->oEntry = -1;
            else if(IS_INTRESOURCE(desc->Entry))
                funcrecord->oEntry = LOWORD(desc->Entry);
            else
                funcrecord->oEntry = desc->Entry->offset;
            funcrecord->res9 = -1;
            funcrecord->resA = -1;
            funcrecord->HelpStringContext = desc->HelpStringContext;
            funcrecord->oCustData = WMSFT_compile_custdata(&desc->custdata_list, file);
        }else if(desc->HelpStringContext != 0){
            size += 6 * sizeof(INT);
            funcrecord->HelpContext = desc->helpcontext;
            if(desc->HelpString)
                funcrecord->oHelpString = desc->HelpString->offset;
            else
                funcrecord->oHelpString = -1;
            if(!desc->Entry)
                funcrecord->oEntry = -1;
            else if(IS_INTRESOURCE(desc->Entry))
                funcrecord->oEntry = LOWORD(desc->Entry);
            else
                funcrecord->oEntry = desc->Entry->offset;
            funcrecord->res9 = -1;
            funcrecord->resA = -1;
            funcrecord->HelpStringContext = desc->HelpStringContext;
        }else if(desc->Entry){
            size += 3 * sizeof(INT);
            funcrecord->HelpContext = desc->helpcontext;
            if(desc->HelpString)
                funcrecord->oHelpString = desc->HelpString->offset;
            else
                funcrecord->oHelpString = -1;
            if(!desc->Entry)
                funcrecord->oEntry = -1;
            else if(IS_INTRESOURCE(desc->Entry))
                funcrecord->oEntry = LOWORD(desc->Entry);
            else
                funcrecord->oEntry = desc->Entry->offset;
        }else if(desc->HelpString){
            size += 2 * sizeof(INT);
            funcrecord->HelpContext = desc->helpcontext;
            funcrecord->oHelpString = desc->HelpString->offset;
        }else if(desc->helpcontext){
            size += sizeof(INT);
            funcrecord->HelpContext = desc->helpcontext;
        }

        paramdefault = (DWORD*)((char *)funcrecord + size);
        size += paramdefault_size;

        for(j = 0; j < desc->funcdesc.cParams; ++j){
            MSFT_ParameterInfo *info = (MSFT_ParameterInfo*)(((char *)funcrecord) + size);

            info->DataType = WMSFT_append_typedesc(&desc->funcdesc.lprgelemdescParam[j].tdesc, file, NULL, &funcrecord->funcdescsize);
            if(desc->pParamDesc[j].Name)
                info->oName = desc->pParamDesc[j].Name->offset;
            else
                info->oName = -1;
            info->Flags = desc->funcdesc.lprgelemdescParam[j].paramdesc.wParamFlags;

            if(paramdefault_size){
                if(desc->funcdesc.lprgelemdescParam[j].paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT)
                    *paramdefault = WMSFT_encode_variant(&desc->funcdesc.lprgelemdescParam[j].paramdesc.pparamdescex->varDefaultValue, file);
                else if(paramdefault_size)
                    *paramdefault = -1;
                ++paramdefault;
            }

            size += sizeof(MSFT_ParameterInfo);
        }

        funcrecord->Info = size | (i << 16); /* is it just the index? */

        *offsets = offs;
        offs += size;
        ++offsets;

        funcrecord = (MSFT_FuncRecord*)(((char*)funcrecord) + size);
    }

    varrecord = (MSFT_VarRecord*)funcrecord;
    for(i = 0; i < info->typeattr.cVars; ++i){
        TLBVarDesc *desc = &info->vardescs[i];
        DWORD size = 5 * sizeof(INT);

        varrecord->vardescsize = sizeof(desc->vardesc);
        varrecord->DataType = WMSFT_append_typedesc(&desc->vardesc.elemdescVar.tdesc, file, NULL, &varrecord->vardescsize);
        varrecord->Flags = desc->vardesc.wVarFlags;
        varrecord->VarKind = desc->vardesc.varkind;

        if(desc->vardesc.varkind == VAR_CONST){
            varrecord->vardescsize += sizeof(VARIANT);
            varrecord->OffsValue = WMSFT_encode_variant(desc->vardesc.lpvarValue, file);
        }else
            varrecord->OffsValue = desc->vardesc.oInst;

        /* res9? */
        if(desc->HelpStringContext != 0){
            size += 5 * sizeof(INT);
            varrecord->HelpContext = desc->HelpContext;
            if(desc->HelpString)
                varrecord->HelpString = desc->HelpString->offset;
            else
                varrecord->HelpString = -1;
            varrecord->res9 = -1;
            varrecord->oCustData = WMSFT_compile_custdata(&desc->custdata_list, file);
            varrecord->HelpStringContext = desc->HelpStringContext;
        }else if(!list_empty(&desc->custdata_list)){
            size += 4 * sizeof(INT);
            varrecord->HelpContext = desc->HelpContext;
            if(desc->HelpString)
                varrecord->HelpString = desc->HelpString->offset;
            else
                varrecord->HelpString = -1;
            varrecord->res9 = -1;
            varrecord->oCustData = WMSFT_compile_custdata(&desc->custdata_list, file);
        }else if(desc->HelpString){
            size += 2 * sizeof(INT);
            varrecord->HelpContext = desc->HelpContext;
            if(desc->HelpString)
                varrecord->HelpString = desc->HelpString->offset;
            else
                varrecord->HelpString = -1;
        }else if(desc->HelpContext != 0){
            size += sizeof(INT);
            varrecord->HelpContext = desc->HelpContext;
        }

        varrecord->Info = size | (i << 16);

        *offsets = offs;
        offs += size;
        ++offsets;

        varrecord = (MSFT_VarRecord*)(((char*)varrecord) + size);
    }

    memid = (MEMBERID*)varrecord;
    for(i = 0; i < info->typeattr.cFuncs; ++i){
        TLBFuncDesc *desc = &info->funcdescs[i];
        *memid = desc->funcdesc.memid;
        ++memid;
    }
    for(i = 0; i < info->typeattr.cVars; ++i){
        TLBVarDesc *desc = &info->vardescs[i];
        *memid = desc->vardesc.memid;
        ++memid;
    }

    name = (DWORD*)memid;
    for(i = 0; i < info->typeattr.cFuncs; ++i){
        TLBFuncDesc *desc = &info->funcdescs[i];
        if(desc->Name)
            *name = desc->Name->offset;
        else
            *name = -1;
        ++name;
    }
    for(i = 0; i < info->typeattr.cVars; ++i){
        TLBVarDesc *desc = &info->vardescs[i];
        if(desc->Name)
            *name = desc->Name->offset;
        else
            *name = -1;
        ++name;
    }

    return ret;
}

typedef struct tagWMSFT_RefChunk {
    DWORD href;
    DWORD res04;
    DWORD res08;
    DWORD next;
} WMSFT_RefChunk;

static DWORD WMSFT_compile_typeinfo_ref(ITypeInfoImpl *info, WMSFT_TLBFile *file)
{
    DWORD offs = file->ref_seg.len, i;
    WMSFT_RefChunk *chunk;

    file->ref_seg.len += info->typeattr.cImplTypes * sizeof(WMSFT_RefChunk);
    file->ref_seg.data = realloc(file->ref_seg.data, file->ref_seg.len);

    chunk = (WMSFT_RefChunk*)((char*)file->ref_seg.data + offs);

    for(i = 0; i < info->typeattr.cImplTypes; ++i){
        chunk->href = info->impltypes[i].hRef;
        chunk->res04 = info->impltypes[i].implflags;
        chunk->res08 = -1;
        if(i < info->typeattr.cImplTypes - 1)
            chunk->next = offs + sizeof(WMSFT_RefChunk) * (i + 1);
        else
            chunk->next = -1;
        ++chunk;
    }

    return offs;
}

static DWORD WMSFT_compile_typeinfo(ITypeInfoImpl *info, INT16 index, WMSFT_TLBFile *file, char *data)
{
    DWORD size;

    size = sizeof(MSFT_TypeInfoBase);

    if(data){
        MSFT_TypeInfoBase *base = (MSFT_TypeInfoBase*)data;
        if(info->typeattr.wTypeFlags & TYPEFLAG_FDUAL)
            base->typekind = TKIND_DISPATCH;
        else
            base->typekind = info->typeattr.typekind;
        base->typekind |= index << 16; /* TODO: There are some other flags here */
        base->typekind |= (info->typeattr.cbAlignment << 11) | (info->typeattr.cbAlignment << 6);
        base->memoffset = WMSFT_compile_typeinfo_aux(info, file);
        base->res2 = 0;
        base->res3 = 0;
        base->res4 = 3;
        base->res5 = 0;
        base->cElement = (info->typeattr.cVars << 16) | info->typeattr.cFuncs;
        base->res7 = 0;
        base->res8 = 0;
        base->res9 = 0;
        base->resA = 0;
        if(info->guid)
            base->posguid = info->guid->offset;
        else
            base->posguid = -1;
        base->flags = info->typeattr.wTypeFlags;
        if(info->Name) {
            base->NameOffset = info->Name->offset;

            ((unsigned char*)file->name_seg.data)[info->Name->offset+9] = 0x38;
            *(HREFTYPE*)((unsigned char*)file->name_seg.data+info->Name->offset) = info->hreftype;
        }else {
            base->NameOffset = -1;
        }
        base->version = (info->typeattr.wMinorVerNum << 16) | info->typeattr.wMajorVerNum;
        if(info->DocString)
            base->docstringoffs = info->DocString->offset;
        else
            base->docstringoffs = -1;
        base->helpstringcontext = info->dwHelpStringContext;
        base->helpcontext = info->dwHelpContext;
        base->oCustData = WMSFT_compile_custdata(info->pcustdata_list, file);
        base->cImplTypes = info->typeattr.cImplTypes;
        base->cbSizeVft = info->typeattr.cbSizeVft;
        base->size = info->typeattr.cbSizeInstance;
        if(info->typeattr.typekind == TKIND_COCLASS){
            base->datatype1 = WMSFT_compile_typeinfo_ref(info, file);
        }else if(info->typeattr.typekind == TKIND_ALIAS){
            base->datatype1 = WMSFT_append_typedesc(info->tdescAlias, file, NULL, NULL);
        }else if(info->typeattr.typekind == TKIND_MODULE){
            if(info->DllName)
                base->datatype1 = info->DllName->offset;
            else
                base->datatype1 = -1;
        }else{
            if(info->typeattr.cImplTypes > 0)
                base->datatype1 = info->impltypes[0].hRef;
            else
                base->datatype1 = -1;
        }
        base->datatype2 = index; /* FIXME: i think there's more here */
        base->res18 = 0;
        base->res19 = -1;
    }

    return size;
}

static void WMSFT_compile_typeinfo_seg(ITypeLibImpl *This, WMSFT_TLBFile *file, DWORD *junk)
{
    UINT i;

    file->typeinfo_seg.len = 0;
    for(i = 0; i < This->TypeInfoCount; ++i){
        ITypeInfoImpl *info = This->typeinfos[i];
        *junk = file->typeinfo_seg.len;
        ++junk;
        file->typeinfo_seg.len += WMSFT_compile_typeinfo(info, i, NULL, NULL);
    }

    file->typeinfo_seg.data = malloc(file->typeinfo_seg.len);
    memset(file->typeinfo_seg.data, 0x96, file->typeinfo_seg.len);

    file->aux_seg.len = 0;
    file->aux_seg.data = NULL;

    file->typeinfo_seg.len = 0;
    for(i = 0; i < This->TypeInfoCount; ++i){
        ITypeInfoImpl *info = This->typeinfos[i];
        file->typeinfo_seg.len += WMSFT_compile_typeinfo(info, i, file,
                ((char *)file->typeinfo_seg.data) + file->typeinfo_seg.len);
    }
}

typedef struct tagWMSFT_ImpFile {
    INT guid_offs;
    LCID lcid;
    DWORD version;
} WMSFT_ImpFile;

static void WMSFT_compile_impfile(ITypeLibImpl *This, WMSFT_TLBFile *file)
{
    TLBImpLib *implib;
    WMSFT_ImpFile *impfile;
    char *data;
    DWORD last_offs = 0;

    file->impfile_seg.len = 0;
    LIST_FOR_EACH_ENTRY(implib, &This->implib_list, TLBImpLib, entry){
        int size = 0;

        if(implib->name){
            WCHAR *path = wcsrchr(implib->name, '\\');
            if(path)
                ++path;
            else
                path = implib->name;
            size = WideCharToMultiByte(CP_ACP, 0, path, lstrlenW(path), NULL, 0, NULL, NULL);
            if (size == 0)
                ERR("failed to convert wide string: %s\n", debugstr_w(path));
        }

        size += sizeof(INT16);
        if (size % 4)
            size = (size + 4) & ~0x3;
        if (size < 8)
            size = 8;

        file->impfile_seg.len += sizeof(WMSFT_ImpFile) + size;
    }

    data = file->impfile_seg.data = malloc(file->impfile_seg.len);

    LIST_FOR_EACH_ENTRY(implib, &This->implib_list, TLBImpLib, entry){
        int strlen = 0, size;

        impfile = (WMSFT_ImpFile*)data;
        impfile->guid_offs = implib->guid->offset;
        impfile->lcid = implib->lcid;
        impfile->version = (implib->wVersionMinor << 16) | implib->wVersionMajor;

        data += sizeof(WMSFT_ImpFile);

        if(implib->name){
            WCHAR *path= wcsrchr(implib->name, '\\');
            if(path)
                ++path;
            else
                path = implib->name;
            strlen = WideCharToMultiByte(CP_ACP, 0, path, lstrlenW(path),
                    data + sizeof(INT16), file->impfile_seg.len - last_offs - sizeof(INT16), NULL, NULL);
            if (strlen == 0)
                ERR("failed to convert wide string: %s\n", debugstr_w(path));
        }

        *((INT16*)data) = (strlen << 2) | 1; /* FIXME: is that a flag, or what? */

        size = strlen + sizeof(INT16);
        if (size % 4)
            size = (size + 4) & ~0x3;
        if (size < 8)
            size = 8;
        memset(data + sizeof(INT16) + strlen, 0x57, size - strlen - sizeof(INT16));

        data += size;
        implib->offset = last_offs;
        last_offs += size + sizeof(WMSFT_ImpFile);
    }
}

static void WMSFT_compile_impinfo(ITypeLibImpl *This, WMSFT_TLBFile *file)
{
    MSFT_ImpInfo *info;
    TLBRefType *ref_type;
    UINT i = 0;

    WMSFT_compile_impfile(This, file);

    file->impinfo_seg.len = sizeof(MSFT_ImpInfo) * list_count(&This->ref_list);
    info = file->impinfo_seg.data = malloc(file->impinfo_seg.len);

    LIST_FOR_EACH_ENTRY(ref_type, &This->ref_list, TLBRefType, entry){
        info->flags = i | ((ref_type->tkind & 0xFF) << 24);
        if(ref_type->index == TLB_REF_USE_GUID){
            info->flags |= MSFT_IMPINFO_OFFSET_IS_GUID;
            info->oGuid = ref_type->guid->offset;
        }else
            info->oGuid = ref_type->index;
        info->oImpFile = ref_type->pImpTLInfo->offset;
        ++i;
        ++info;
    }
}

static void WMSFT_compile_guidhash(ITypeLibImpl *This, WMSFT_TLBFile *file)
{
    file->guidhash_seg.len = 0x80;
    file->guidhash_seg.data = malloc(file->guidhash_seg.len);
    memset(file->guidhash_seg.data, 0xFF, file->guidhash_seg.len);
}

static void WMSFT_compile_namehash(ITypeLibImpl *This, WMSFT_TLBFile *file)
{
    file->namehash_seg.len = 0x200;
    file->namehash_seg.data = malloc(file->namehash_seg.len);
    memset(file->namehash_seg.data, 0xFF, file->namehash_seg.len);
}

static void tmp_fill_segdir_seg(MSFT_pSeg *segdir, WMSFT_SegContents *contents, DWORD *running_offset)
{
    if(contents && contents->len){
        segdir->offset = *running_offset;
        segdir->length = contents->len;
        *running_offset += segdir->length;
    }else{
        segdir->offset = -1;
        segdir->length = 0;
    }

    /* TODO: do these ever change? */
    segdir->res08 = -1;
    segdir->res0c = 0xf;
}

static void WMSFT_write_segment(HANDLE outfile, WMSFT_SegContents *segment)
{
    DWORD written;
    if(segment)
        WriteFile(outfile, segment->data, segment->len, &written, NULL);
}

static HRESULT WMSFT_fixup_typeinfos(ITypeLibImpl *This, WMSFT_TLBFile *file,
        DWORD file_len)
{
    DWORD i;
    MSFT_TypeInfoBase *base = (MSFT_TypeInfoBase *)file->typeinfo_seg.data;

    for(i = 0; i < This->TypeInfoCount; ++i){
        base->memoffset += file_len;
        ++base;
    }

    return S_OK;
}

static void WMSFT_free_file(WMSFT_TLBFile *file)
{
    free(file->typeinfo_seg.data);
    free(file->guidhash_seg.data);
    free(file->guid_seg.data);
    free(file->ref_seg.data);
    free(file->impinfo_seg.data);
    free(file->impfile_seg.data);
    free(file->namehash_seg.data);
    free(file->name_seg.data);
    free(file->string_seg.data);
    free(file->typdesc_seg.data);
    free(file->arraydesc_seg.data);
    free(file->custdata_seg.data);
    free(file->cdguids_seg.data);
    free(file->aux_seg.data);
}

static HRESULT WINAPI ICreateTypeLib2_fnSaveAllChanges(ICreateTypeLib2 *iface)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);
    WMSFT_TLBFile file;
    DWORD written, junk_size, junk_offs, running_offset;
    BOOL br;
    HANDLE outfile;
    HRESULT hres;
    DWORD *junk;
    UINT i;

    TRACE("%p\n", This);

    for(i = 0; i < This->TypeInfoCount; ++i)
        if(This->typeinfos[i]->needs_layout)
            ICreateTypeInfo2_LayOut(&This->typeinfos[i]->ICreateTypeInfo2_iface);

    memset(&file, 0, sizeof(file));

    file.header.magic1 = 0x5446534D;
    file.header.magic2 = 0x00010002;
    file.header.lcid = This->set_lcid ? This->set_lcid : MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    file.header.lcid2 = This->set_lcid;
    file.header.varflags = 0x40 | This->syskind;
    if (This->HelpFile)
        file.header.varflags |= 0x10;
    if (This->HelpStringDll)
        file.header.varflags |= HELPDLLFLAG;
    file.header.version = (This->ver_minor << 16) | This->ver_major;
    file.header.flags = This->libflags;
    file.header.helpstringcontext = 0; /* TODO - SetHelpStringContext not implemented yet */
    file.header.helpcontext = This->dwHelpContext;
    file.header.res44 = 0x20;
    file.header.res48 = 0x80;
    file.header.dispatchpos = This->dispatch_href;

    WMSFT_compile_namehash(This, &file);
    /* do name and string compilation to get offsets for other compilations */
    hres = WMSFT_compile_names(This, &file);
    if (FAILED(hres)){
        WMSFT_free_file(&file);
        return hres;
    }

    hres = WMSFT_compile_strings(This, &file);
    if (FAILED(hres)){
        WMSFT_free_file(&file);
        return hres;
    }

    WMSFT_compile_guidhash(This, &file);
    hres = WMSFT_compile_guids(This, &file);
    if (FAILED(hres)){
        WMSFT_free_file(&file);
        return hres;
    }

    if(This->HelpFile)
        file.header.helpfile = This->HelpFile->offset;
    else
        file.header.helpfile = -1;

    if(This->DocString)
        file.header.helpstring = This->DocString->offset;
    else
        file.header.helpstring = -1;

    /* do some more segment compilation */
    file.header.nimpinfos = list_count(&This->ref_list);
    file.header.nrtypeinfos = This->TypeInfoCount;

    if(This->Name)
        file.header.NameOffset = This->Name->offset;
    else
        file.header.NameOffset = -1;

    file.header.CustomDataOffset = WMSFT_compile_custdata(&This->custdata_list, &file);

    if(This->guid)
        file.header.posguid = This->guid->offset;
    else
        file.header.posguid = -1;

    junk_size = file.header.nrtypeinfos * sizeof(DWORD);
    if(file.header.varflags & HELPDLLFLAG)
        junk_size += sizeof(DWORD);
    if(junk_size){
        junk = calloc(1, junk_size);
        if(file.header.varflags & HELPDLLFLAG){
            *junk = This->HelpStringDll->offset;
            junk_offs = 1;
        }else
            junk_offs = 0;
    }else{
        junk = NULL;
        junk_offs = 0;
    }

    WMSFT_compile_typeinfo_seg(This, &file, junk + junk_offs);
    WMSFT_compile_impinfo(This, &file);

    running_offset = 0;

    TRACE("header at: 0x%lx\n", running_offset);
    running_offset += sizeof(file.header);

    TRACE("junk at: 0x%lx\n", running_offset);
    running_offset += junk_size;

    TRACE("segdir at: 0x%lx\n", running_offset);
    running_offset += sizeof(file.segdir);

    TRACE("typeinfo at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.pTypeInfoTab, &file.typeinfo_seg, &running_offset);

    TRACE("guidhashtab at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.pGuidHashTab, &file.guidhash_seg, &running_offset);

    TRACE("guidtab at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.pGuidTab, &file.guid_seg, &running_offset);

    TRACE("reftab at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.pRefTab, &file.ref_seg, &running_offset);

    TRACE("impinfo at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.pImpInfo, &file.impinfo_seg, &running_offset);

    TRACE("impfiles at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.pImpFiles, &file.impfile_seg, &running_offset);

    TRACE("namehashtab at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.pNameHashTab, &file.namehash_seg, &running_offset);

    TRACE("nametab at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.pNametab, &file.name_seg, &running_offset);

    TRACE("stringtab at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.pStringtab, &file.string_seg, &running_offset);

    TRACE("typdesc at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.pTypdescTab, &file.typdesc_seg, &running_offset);

    TRACE("arraydescriptions at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.pArrayDescriptions, &file.arraydesc_seg, &running_offset);

    TRACE("custdata at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.pCustData, &file.custdata_seg, &running_offset);

    TRACE("cdguids at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.pCDGuids, &file.cdguids_seg, &running_offset);

    TRACE("res0e at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.res0e, NULL, &running_offset);

    TRACE("res0f at: 0x%lx\n", running_offset);
    tmp_fill_segdir_seg(&file.segdir.res0f, NULL, &running_offset);

    TRACE("aux_seg at: 0x%lx\n", running_offset);

    WMSFT_fixup_typeinfos(This, &file, running_offset);

    outfile = CreateFileW(This->path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, 0);
    if (outfile == INVALID_HANDLE_VALUE){
        WMSFT_free_file(&file);
        free(junk);
        return TYPE_E_IOERROR;
    }

    br = WriteFile(outfile, &file.header, sizeof(file.header), &written, NULL);
    if (!br) {
        WMSFT_free_file(&file);
        CloseHandle(outfile);
        free(junk);
        return TYPE_E_IOERROR;
    }

    br = WriteFile(outfile, junk, junk_size, &written, NULL);
    free(junk);
    if (!br) {
        WMSFT_free_file(&file);
        CloseHandle(outfile);
        return TYPE_E_IOERROR;
    }

    br = WriteFile(outfile, &file.segdir, sizeof(file.segdir), &written, NULL);
    if (!br) {
        WMSFT_free_file(&file);
        CloseHandle(outfile);
        return TYPE_E_IOERROR;
    }

    WMSFT_write_segment(outfile, &file.typeinfo_seg);
    WMSFT_write_segment(outfile, &file.guidhash_seg);
    WMSFT_write_segment(outfile, &file.guid_seg);
    WMSFT_write_segment(outfile, &file.ref_seg);
    WMSFT_write_segment(outfile, &file.impinfo_seg);
    WMSFT_write_segment(outfile, &file.impfile_seg);
    WMSFT_write_segment(outfile, &file.namehash_seg);
    WMSFT_write_segment(outfile, &file.name_seg);
    WMSFT_write_segment(outfile, &file.string_seg);
    WMSFT_write_segment(outfile, &file.typdesc_seg);
    WMSFT_write_segment(outfile, &file.arraydesc_seg);
    WMSFT_write_segment(outfile, &file.custdata_seg);
    WMSFT_write_segment(outfile, &file.cdguids_seg);
    WMSFT_write_segment(outfile, &file.aux_seg);

    WMSFT_free_file(&file);

    CloseHandle(outfile);

    return S_OK;
}

static HRESULT WINAPI ICreateTypeLib2_fnDeleteTypeInfo(ICreateTypeLib2 *iface,
        LPOLESTR name)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);
    FIXME("%p %s - stub\n", This, wine_dbgstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeLib2_fnSetCustData(ICreateTypeLib2 *iface,
        REFGUID guid, VARIANT *varVal)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);
    TLBGuid *tlbguid;

    TRACE("%p %s %p\n", This, debugstr_guid(guid), varVal);

    if (!guid || !varVal)
        return E_INVALIDARG;

    tlbguid = TLB_append_guid(&This->guid_list, guid, -1);

    return TLB_set_custdata(&This->custdata_list, tlbguid, varVal);
}

static HRESULT WINAPI ICreateTypeLib2_fnSetHelpStringContext(ICreateTypeLib2 *iface,
        ULONG helpStringContext)
{
    FIXME("%p, %lu - stub\n", iface, helpStringContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeLib2_fnSetHelpStringDll(ICreateTypeLib2 *iface,
        LPOLESTR filename)
{
    ITypeLibImpl *This = impl_from_ICreateTypeLib2(iface);
    TRACE("%p %s\n", This, wine_dbgstr_w(filename));

    if (!filename)
        return E_INVALIDARG;

    This->HelpStringDll = TLB_append_str(&This->string_list, filename);

    return S_OK;
}

static const ICreateTypeLib2Vtbl CreateTypeLib2Vtbl = {
    ICreateTypeLib2_fnQueryInterface,
    ICreateTypeLib2_fnAddRef,
    ICreateTypeLib2_fnRelease,
    ICreateTypeLib2_fnCreateTypeInfo,
    ICreateTypeLib2_fnSetName,
    ICreateTypeLib2_fnSetVersion,
    ICreateTypeLib2_fnSetGuid,
    ICreateTypeLib2_fnSetDocString,
    ICreateTypeLib2_fnSetHelpFileName,
    ICreateTypeLib2_fnSetHelpContext,
    ICreateTypeLib2_fnSetLcid,
    ICreateTypeLib2_fnSetLibFlags,
    ICreateTypeLib2_fnSaveAllChanges,
    ICreateTypeLib2_fnDeleteTypeInfo,
    ICreateTypeLib2_fnSetCustData,
    ICreateTypeLib2_fnSetHelpStringContext,
    ICreateTypeLib2_fnSetHelpStringDll
};

static HRESULT WINAPI ICreateTypeInfo2_fnQueryInterface(ICreateTypeInfo2 *iface,
        REFIID riid, void **object)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    return ITypeInfo2_QueryInterface(&This->ITypeInfo2_iface, riid, object);
}

static ULONG WINAPI ICreateTypeInfo2_fnAddRef(ICreateTypeInfo2 *iface)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    return ITypeInfo2_AddRef(&This->ITypeInfo2_iface);
}

static ULONG WINAPI ICreateTypeInfo2_fnRelease(ICreateTypeInfo2 *iface)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    return ITypeInfo2_Release(&This->ITypeInfo2_iface);
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetGuid(ICreateTypeInfo2 *iface,
        REFGUID guid)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    TRACE("%p %s\n", This, debugstr_guid(guid));

    This->guid = TLB_append_guid(&This->pTypeLib->guid_list, guid, This->hreftype);

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetTypeFlags(ICreateTypeInfo2 *iface,
        UINT typeFlags)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    WORD old_flags;
    HRESULT hres;

    TRACE("%p %x\n", This, typeFlags);

    if (typeFlags & TYPEFLAG_FDUAL) {
        ITypeLib *stdole;
        ITypeInfo *dispatch;
        HREFTYPE hreftype;
        HRESULT hres;

        hres = LoadTypeLib(L"stdole2.tlb", &stdole);
        if(FAILED(hres))
            return hres;

        hres = ITypeLib_GetTypeInfoOfGuid(stdole, &IID_IDispatch, &dispatch);
        ITypeLib_Release(stdole);
        if(FAILED(hres))
            return hres;

        hres = ICreateTypeInfo2_AddRefTypeInfo(iface, dispatch, &hreftype);
        ITypeInfo_Release(dispatch);
        if(FAILED(hres))
            return hres;
    }

    old_flags = This->typeattr.wTypeFlags;
    This->typeattr.wTypeFlags = typeFlags;

    hres = ICreateTypeInfo2_LayOut(iface);
    if (FAILED(hres)) {
        This->typeattr.wTypeFlags = old_flags;
        return hres;
    }

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetDocString(ICreateTypeInfo2 *iface,
        LPOLESTR doc)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    TRACE("%p %s\n", This, wine_dbgstr_w(doc));

    if (!doc)
        return E_INVALIDARG;

    This->DocString = TLB_append_str(&This->pTypeLib->string_list, doc);

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetHelpContext(ICreateTypeInfo2 *iface,
        DWORD helpContext)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    TRACE("%p, %ld.\n", iface, helpContext);

    This->dwHelpContext = helpContext;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetVersion(ICreateTypeInfo2 *iface,
        WORD majorVerNum, WORD minorVerNum)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    TRACE("%p %d %d\n", This, majorVerNum, minorVerNum);

    This->typeattr.wMajorVerNum = majorVerNum;
    This->typeattr.wMinorVerNum = minorVerNum;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnAddRefTypeInfo(ICreateTypeInfo2 *iface,
        ITypeInfo *typeInfo, HREFTYPE *refType)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    UINT index;
    ITypeLib *container;
    TLBRefType *ref_type;
    TLBImpLib *implib;
    TYPEATTR *typeattr;
    TLIBATTR *libattr;
    HRESULT hres;

    TRACE("%p %p %p\n", This, typeInfo, refType);

    if (!typeInfo || !refType)
        return E_INVALIDARG;

    hres = ITypeInfo_GetContainingTypeLib(typeInfo, &container, &index);
    if (FAILED(hres))
        return hres;

    if (container == (ITypeLib*)&This->pTypeLib->ITypeLib2_iface) {
        ITypeInfoImpl *target = impl_from_ITypeInfo(typeInfo);

        ITypeLib_Release(container);

        *refType = target->hreftype;

        return S_OK;
    }

    hres = ITypeLib_GetLibAttr(container, &libattr);
    if (FAILED(hres)) {
        ITypeLib_Release(container);
        return hres;
    }

    LIST_FOR_EACH_ENTRY(implib, &This->pTypeLib->implib_list, TLBImpLib, entry){
        if(IsEqualGUID(&implib->guid->guid, &libattr->guid) &&
                implib->lcid == libattr->lcid &&
                implib->wVersionMajor == libattr->wMajorVerNum &&
                implib->wVersionMinor == libattr->wMinorVerNum)
            break;
    }

    if(&implib->entry == &This->pTypeLib->implib_list){
        implib = calloc(1, sizeof(TLBImpLib));

        if((ITypeLib2Vtbl*)container->lpVtbl == &tlbvt){
            const ITypeLibImpl *our_container = impl_from_ITypeLib2((ITypeLib2*)container);
            implib->name = SysAllocString(our_container->path);
        }else{
            hres = QueryPathOfRegTypeLib(&libattr->guid, libattr->wMajorVerNum,
                    libattr->wMinorVerNum, libattr->lcid, &implib->name);
            if(FAILED(hres)){
                implib->name = NULL;
                TRACE("QueryPathOfRegTypeLib failed, no name stored: %#lx.\n", hres);
            }
        }

        implib->guid = TLB_append_guid(&This->pTypeLib->guid_list, &libattr->guid, 2);
        implib->lcid = libattr->lcid;
        implib->wVersionMajor = libattr->wMajorVerNum;
        implib->wVersionMinor = libattr->wMinorVerNum;

        list_add_tail(&This->pTypeLib->implib_list, &implib->entry);
    }

    ITypeLib_ReleaseTLibAttr(container, libattr);
    ITypeLib_Release(container);

    hres = ITypeInfo_GetTypeAttr(typeInfo, &typeattr);
    if (FAILED(hres))
        return hres;

    index = 0;
    LIST_FOR_EACH_ENTRY(ref_type, &This->pTypeLib->ref_list, TLBRefType, entry){
        if(ref_type->index == TLB_REF_USE_GUID &&
                IsEqualGUID(&ref_type->guid->guid, &typeattr->guid) &&
                ref_type->tkind == typeattr->typekind)
            break;
        ++index;
    }

    if(&ref_type->entry == &This->pTypeLib->ref_list){
        ref_type = calloc(1, sizeof(TLBRefType));

        ref_type->tkind = typeattr->typekind;
        ref_type->pImpTLInfo = implib;
        ref_type->reference = index * sizeof(MSFT_ImpInfo);

        ref_type->index = TLB_REF_USE_GUID;

        ref_type->guid = TLB_append_guid(&This->pTypeLib->guid_list, &typeattr->guid, ref_type->reference+1);

        list_add_tail(&This->pTypeLib->ref_list, &ref_type->entry);
    }

    ITypeInfo_ReleaseTypeAttr(typeInfo, typeattr);

    *refType = ref_type->reference | 0x1;

    if(IsEqualGUID(&ref_type->guid->guid, &IID_IDispatch))
        This->pTypeLib->dispatch_href = *refType;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnAddFuncDesc(ICreateTypeInfo2 *iface,
        UINT index, FUNCDESC *funcDesc)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    TLBFuncDesc tmp_func_desc, *func_desc;
    int buf_size, i;
    char *buffer;
    HRESULT hres;

    TRACE("%p %u %p\n", This, index, funcDesc);

    if (!funcDesc || funcDesc->oVft & 3)
        return E_INVALIDARG;

    switch (This->typeattr.typekind) {
    case TKIND_MODULE:
        if (funcDesc->funckind != FUNC_STATIC)
            return TYPE_E_BADMODULEKIND;
        break;
    case TKIND_DISPATCH:
        if (funcDesc->funckind != FUNC_DISPATCH)
            return TYPE_E_BADMODULEKIND;
        break;
    default:
        if (funcDesc->funckind != FUNC_PUREVIRTUAL)
            return TYPE_E_BADMODULEKIND;
    }

    if (index > This->typeattr.cFuncs)
        return TYPE_E_ELEMENTNOTFOUND;

    if (funcDesc->invkind & (INVOKE_PROPERTYPUT | INVOKE_PROPERTYPUTREF) &&
            !funcDesc->cParams)
        return TYPE_E_INCONSISTENTPROPFUNCS;

    if(This->pTypeLib->syskind == SYS_WIN64 &&
            funcDesc->oVft % 8 != 0)
        return E_INVALIDARG;

    memset(&tmp_func_desc, 0, sizeof(tmp_func_desc));
    TLBFuncDesc_Constructor(&tmp_func_desc);

    tmp_func_desc.funcdesc = *funcDesc;

    if (tmp_func_desc.funcdesc.oVft != 0)
        tmp_func_desc.funcdesc.oVft |= 1;

    if (funcDesc->cScodes && funcDesc->lprgscode) {
        tmp_func_desc.funcdesc.lprgscode = malloc(sizeof(SCODE) * funcDesc->cScodes);
        memcpy(tmp_func_desc.funcdesc.lprgscode, funcDesc->lprgscode, sizeof(SCODE) * funcDesc->cScodes);
    } else {
        tmp_func_desc.funcdesc.lprgscode = NULL;
        tmp_func_desc.funcdesc.cScodes = 0;
    }

    buf_size = TLB_SizeElemDesc(&funcDesc->elemdescFunc);
    for (i = 0; i < funcDesc->cParams; ++i) {
        buf_size += sizeof(ELEMDESC);
        buf_size += TLB_SizeElemDesc(funcDesc->lprgelemdescParam + i);
    }
    tmp_func_desc.funcdesc.lprgelemdescParam = malloc(buf_size);
    buffer = (char*)(tmp_func_desc.funcdesc.lprgelemdescParam + funcDesc->cParams);

    hres = TLB_CopyElemDesc(&funcDesc->elemdescFunc, &tmp_func_desc.funcdesc.elemdescFunc, &buffer);
    if (FAILED(hres)) {
        free(tmp_func_desc.funcdesc.lprgelemdescParam);
        free(tmp_func_desc.funcdesc.lprgscode);
        return hres;
    }

    for (i = 0; i < funcDesc->cParams; ++i) {
        hres = TLB_CopyElemDesc(funcDesc->lprgelemdescParam + i,
                tmp_func_desc.funcdesc.lprgelemdescParam + i, &buffer);
        if (FAILED(hres)) {
            free(tmp_func_desc.funcdesc.lprgelemdescParam);
            free(tmp_func_desc.funcdesc.lprgscode);
            return hres;
        }
        if (tmp_func_desc.funcdesc.lprgelemdescParam[i].paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT &&
                tmp_func_desc.funcdesc.lprgelemdescParam[i].tdesc.vt != VT_VARIANT &&
                tmp_func_desc.funcdesc.lprgelemdescParam[i].tdesc.vt != VT_USERDEFINED){
            hres = TLB_SanitizeVariant(&tmp_func_desc.funcdesc.lprgelemdescParam[i].paramdesc.pparamdescex->varDefaultValue);
            if (FAILED(hres)) {
                free(tmp_func_desc.funcdesc.lprgelemdescParam);
                free(tmp_func_desc.funcdesc.lprgscode);
                return hres;
            }
        }
    }

    tmp_func_desc.pParamDesc = TLBParDesc_Constructor(funcDesc->cParams);

    if (This->funcdescs) {
        This->funcdescs = realloc(This->funcdescs, sizeof(TLBFuncDesc) * (This->typeattr.cFuncs + 1));

        if (index < This->typeattr.cFuncs) {
            memmove(This->funcdescs + index + 1, This->funcdescs + index,
                    (This->typeattr.cFuncs - index) * sizeof(TLBFuncDesc));
            func_desc = This->funcdescs + index;
        } else
            func_desc = This->funcdescs + This->typeattr.cFuncs;

        /* move custdata lists to the new memory location */
        for(i = 0; i < This->typeattr.cFuncs + 1; ++i){
            if(index != i)
                TLB_relink_custdata(&This->funcdescs[i].custdata_list);
        }
    } else
        func_desc = This->funcdescs = malloc(sizeof(TLBFuncDesc));

    memcpy(func_desc, &tmp_func_desc, sizeof(tmp_func_desc));
    list_init(&func_desc->custdata_list);

    ++This->typeattr.cFuncs;

    This->needs_layout = TRUE;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnAddImplType(ICreateTypeInfo2 *iface,
        UINT index, HREFTYPE refType)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    TLBImplType *impl_type;
    HRESULT hres;

    TRACE("%p, %u, %ld.\n", iface, index, refType);

    switch(This->typeattr.typekind){
        case TKIND_COCLASS: {
            if (index == -1) {
                FIXME("Unhandled index: -1\n");
                return E_NOTIMPL;
            }

            if(index != This->typeattr.cImplTypes)
                return TYPE_E_ELEMENTNOTFOUND;

            break;
        }
        case TKIND_INTERFACE:
        case TKIND_DISPATCH:
            if (index != 0 || This->typeattr.cImplTypes)
                return TYPE_E_ELEMENTNOTFOUND;
            break;
        default:
            FIXME("Unimplemented typekind: %d\n", This->typeattr.typekind);
            return E_NOTIMPL;
    }

    if (This->impltypes){
        UINT i;

        This->impltypes = realloc(This->impltypes, sizeof(TLBImplType) * (This->typeattr.cImplTypes + 1));

        if (index < This->typeattr.cImplTypes) {
            memmove(This->impltypes + index + 1, This->impltypes + index,
                    (This->typeattr.cImplTypes - index) * sizeof(TLBImplType));
            impl_type = This->impltypes + index;
        } else
            impl_type = This->impltypes + This->typeattr.cImplTypes;

        /* move custdata lists to the new memory location */
        for(i = 0; i < This->typeattr.cImplTypes + 1; ++i){
            if(index != i)
                TLB_relink_custdata(&This->impltypes[i].custdata_list);
        }
    } else
        impl_type = This->impltypes = malloc(sizeof(TLBImplType));

    memset(impl_type, 0, sizeof(TLBImplType));
    TLBImplType_Constructor(impl_type);
    impl_type->hRef = refType;

    ++This->typeattr.cImplTypes;

    if((refType & (~0x3)) == (This->pTypeLib->dispatch_href & (~0x3)))
        This->typeattr.wTypeFlags |= TYPEFLAG_FDISPATCHABLE;

    hres = ICreateTypeInfo2_LayOut(iface);
    if (FAILED(hres))
        return hres;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetImplTypeFlags(ICreateTypeInfo2 *iface,
        UINT index, INT implTypeFlags)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    TLBImplType *impl_type = &This->impltypes[index];

    TRACE("%p %u %x\n", This, index, implTypeFlags);

    if (This->typeattr.typekind != TKIND_COCLASS)
        return TYPE_E_BADMODULEKIND;

    if (index >= This->typeattr.cImplTypes)
        return TYPE_E_ELEMENTNOTFOUND;

    impl_type->implflags = implTypeFlags;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetAlignment(ICreateTypeInfo2 *iface,
        WORD alignment)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    TRACE("%p %d\n", This, alignment);

    This->typeattr.cbAlignment = alignment;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetSchema(ICreateTypeInfo2 *iface,
        LPOLESTR schema)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    TRACE("%p %s\n", This, wine_dbgstr_w(schema));

    if (!schema)
        return E_INVALIDARG;

    This->Schema = TLB_append_str(&This->pTypeLib->string_list, schema);

    This->typeattr.lpstrSchema = This->Schema->str;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnAddVarDesc(ICreateTypeInfo2 *iface,
        UINT index, VARDESC *varDesc)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    TLBVarDesc *var_desc;
    HRESULT hr;

    TRACE("%p %u %p\n", This, index, varDesc);

    if (This->vardescs){
        UINT i;

        This->vardescs = realloc(This->vardescs, sizeof(TLBVarDesc) * (This->typeattr.cVars + 1));

        if (index < This->typeattr.cVars) {
            memmove(This->vardescs + index + 1, This->vardescs + index,
                    (This->typeattr.cVars - index) * sizeof(TLBVarDesc));
            var_desc = This->vardescs + index;
        } else {
            var_desc = This->vardescs + This->typeattr.cVars;
            memset(var_desc, 0, sizeof(TLBVarDesc));
        }

        /* move custdata lists to the new memory location */
        for(i = 0; i < This->typeattr.cVars + 1; ++i){
            if(index != i)
                TLB_relink_custdata(&This->vardescs[i].custdata_list);
        }
    } else
        var_desc = This->vardescs = calloc(1, sizeof(TLBVarDesc));

    TLBVarDesc_Constructor(var_desc);
    hr = TLB_AllocAndInitVarDesc(varDesc, &var_desc->vardesc_create);
    if (FAILED(hr))
        return hr;
    var_desc->vardesc = *var_desc->vardesc_create;

    ++This->typeattr.cVars;

    This->needs_layout = TRUE;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetFuncAndParamNames(ICreateTypeInfo2 *iface,
        UINT index, LPOLESTR *names, UINT numNames)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    TLBFuncDesc *func_desc = &This->funcdescs[index];
    int i;

    TRACE("%p %u %p %u\n", This, index, names, numNames);

    if (!names)
        return E_INVALIDARG;

    if (index >= This->typeattr.cFuncs || numNames == 0)
        return TYPE_E_ELEMENTNOTFOUND;

    if (func_desc->funcdesc.invkind & (INVOKE_PROPERTYPUT | INVOKE_PROPERTYPUTREF)){
        if(numNames > func_desc->funcdesc.cParams)
            return TYPE_E_ELEMENTNOTFOUND;
    } else
        if(numNames > func_desc->funcdesc.cParams + 1)
            return TYPE_E_ELEMENTNOTFOUND;

    for(i = 0; i < This->typeattr.cFuncs; ++i) {
        TLBFuncDesc *iter = &This->funcdescs[i];
        if (iter->Name && !wcscmp(TLB_get_bstr(iter->Name), *names)) {
            if (iter->funcdesc.invkind & (INVOKE_PROPERTYPUT | INVOKE_PROPERTYPUTREF | INVOKE_PROPERTYGET) &&
                    func_desc->funcdesc.invkind & (INVOKE_PROPERTYPUT | INVOKE_PROPERTYPUTREF | INVOKE_PROPERTYGET) &&
                    func_desc->funcdesc.invkind != iter->funcdesc.invkind)
                continue;
            return TYPE_E_AMBIGUOUSNAME;
        }
    }

    func_desc->Name = TLB_append_str(&This->pTypeLib->name_list, *names);

    for (i = 1; i < numNames; ++i) {
        TLBParDesc *par_desc = func_desc->pParamDesc + i - 1;
        par_desc->Name = TLB_append_str(&This->pTypeLib->name_list, *(names + i));
    }

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetVarName(ICreateTypeInfo2 *iface,
        UINT index, LPOLESTR name)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    TRACE("%p %u %s\n", This, index, wine_dbgstr_w(name));

    if(!name)
        return E_INVALIDARG;

    if(index >= This->typeattr.cVars)
        return TYPE_E_ELEMENTNOTFOUND;

    This->vardescs[index].Name = TLB_append_str(&This->pTypeLib->name_list, name);
    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetTypeDescAlias(ICreateTypeInfo2 *iface,
        TYPEDESC *tdescAlias)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    HRESULT hr;

    TRACE("%p %p\n", This, tdescAlias);

    if(!tdescAlias)
        return E_INVALIDARG;

    if(This->typeattr.typekind != TKIND_ALIAS)
        return TYPE_E_BADMODULEKIND;

    hr = TLB_size_instance(This, This->pTypeLib->syskind, tdescAlias, &This->typeattr.cbSizeInstance, &This->typeattr.cbAlignment);
    if(FAILED(hr))
        return hr;

    free(This->tdescAlias);
    This->tdescAlias = malloc(TLB_SizeTypeDesc(tdescAlias, TRUE));
    TLB_CopyTypeDesc(NULL, tdescAlias, This->tdescAlias);

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnDefineFuncAsDllEntry(ICreateTypeInfo2 *iface,
        UINT index, LPOLESTR dllName, LPOLESTR procName)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    FIXME("%p %u %s %s - stub\n", This, index, wine_dbgstr_w(dllName), wine_dbgstr_w(procName));
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetFuncDocString(ICreateTypeInfo2 *iface,
        UINT index, LPOLESTR docString)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    TLBFuncDesc *func_desc = &This->funcdescs[index];

    TRACE("%p %u %s\n", This, index, wine_dbgstr_w(docString));

    if(!docString)
        return E_INVALIDARG;

    if(index >= This->typeattr.cFuncs)
        return TYPE_E_ELEMENTNOTFOUND;

    func_desc->HelpString = TLB_append_str(&This->pTypeLib->string_list, docString);

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetVarDocString(ICreateTypeInfo2 *iface,
        UINT index, LPOLESTR docString)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    TLBVarDesc *var_desc = &This->vardescs[index];

    TRACE("%p %u %s\n", This, index, wine_dbgstr_w(docString));

    if(!docString)
        return E_INVALIDARG;

    if(index >= This->typeattr.cVars)
        return TYPE_E_ELEMENTNOTFOUND;

    var_desc->HelpString = TLB_append_str(&This->pTypeLib->string_list, docString);

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetFuncHelpContext(ICreateTypeInfo2 *iface,
        UINT index, DWORD helpContext)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    TLBFuncDesc *func_desc = &This->funcdescs[index];

    TRACE("%p, %u, %ld.\n", iface, index, helpContext);

    if(index >= This->typeattr.cFuncs)
        return TYPE_E_ELEMENTNOTFOUND;

    func_desc->helpcontext = helpContext;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetVarHelpContext(ICreateTypeInfo2 *iface,
        UINT index, DWORD helpContext)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    TLBVarDesc *var_desc = &This->vardescs[index];

    TRACE("%p, %u, %ld.\n", iface, index, helpContext);

    if(index >= This->typeattr.cVars)
        return TYPE_E_ELEMENTNOTFOUND;

    var_desc->HelpContext = helpContext;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetMops(ICreateTypeInfo2 *iface,
        UINT index, BSTR bstrMops)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    FIXME("%p %u %s - stub\n", This, index, wine_dbgstr_w(bstrMops));
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetTypeIdldesc(ICreateTypeInfo2 *iface,
        IDLDESC *idlDesc)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    TRACE("%p %p\n", This, idlDesc);

    if (!idlDesc)
        return E_INVALIDARG;

    This->typeattr.idldescType.dwReserved = idlDesc->dwReserved;
    This->typeattr.idldescType.wIDLFlags = idlDesc->wIDLFlags;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnLayOut(ICreateTypeInfo2 *iface)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    ITypeInfo2 *tinfo = &This->ITypeInfo2_iface;
    TLBFuncDesc *func_desc;
    UINT user_vft = 0, i, depth = 0;
    HRESULT hres = S_OK;

    TRACE("%p\n", This);

    This->needs_layout = FALSE;

    if (This->typeattr.typekind == TKIND_INTERFACE) {
        ITypeInfo *inh;
        TYPEATTR *attr;
        HREFTYPE inh_href;

        hres = ITypeInfo2_GetRefTypeOfImplType(tinfo, 0, &inh_href);

        if (SUCCEEDED(hres)) {
            hres = ITypeInfo2_GetRefTypeInfo(tinfo, inh_href, &inh);

            if (SUCCEEDED(hres)) {
                hres = ITypeInfo_GetTypeAttr(inh, &attr);
                if (FAILED(hres)) {
                    ITypeInfo_Release(inh);
                    return hres;
                }
                This->typeattr.cbSizeVft = attr->cbSizeVft;
                ITypeInfo_ReleaseTypeAttr(inh, attr);

                do{
                    ++depth;
                    hres = ITypeInfo_GetRefTypeOfImplType(inh, 0, &inh_href);
                    if(SUCCEEDED(hres)){
                        ITypeInfo *next;
                        hres = ITypeInfo_GetRefTypeInfo(inh, inh_href, &next);
                        if(SUCCEEDED(hres)){
                            ITypeInfo_Release(inh);
                            inh = next;
                        }
                    }
                }while(SUCCEEDED(hres));
                hres = S_OK;

                ITypeInfo_Release(inh);
            } else if (hres == TYPE_E_ELEMENTNOTFOUND) {
                This->typeattr.cbSizeVft = 0;
                hres = S_OK;
            } else
                return hres;
        } else if (hres == TYPE_E_ELEMENTNOTFOUND) {
            This->typeattr.cbSizeVft = 0;
            hres = S_OK;
        } else
            return hres;
    } else if (This->typeattr.typekind == TKIND_DISPATCH)
        This->typeattr.cbSizeVft = 7 * This->pTypeLib->ptr_size;
    else
        This->typeattr.cbSizeVft = 0;

    func_desc = This->funcdescs;
    i = 0;
    while (i < This->typeattr.cFuncs) {
        if (!(func_desc->funcdesc.oVft & 0x1))
            func_desc->funcdesc.oVft = This->typeattr.cbSizeVft;

        if ((func_desc->funcdesc.oVft & 0xFFFC) > user_vft)
            user_vft = func_desc->funcdesc.oVft & 0xFFFC;

        This->typeattr.cbSizeVft += This->pTypeLib->ptr_size;

        if (func_desc->funcdesc.memid == MEMBERID_NIL) {
            TLBFuncDesc *iter;
            UINT j = 0;
            BOOL reset = FALSE;

            func_desc->funcdesc.memid = 0x60000000 + (depth << 16) + i;

            iter = This->funcdescs;
            while (j < This->typeattr.cFuncs) {
                if (iter != func_desc && iter->funcdesc.memid == func_desc->funcdesc.memid) {
                    if (!reset) {
                        func_desc->funcdesc.memid = 0x60000000 + (depth << 16) + This->typeattr.cFuncs;
                        reset = TRUE;
                    } else
                        ++func_desc->funcdesc.memid;
                    iter = This->funcdescs;
                    j = 0;
                } else {
                    ++iter;
                    ++j;
                }
            }
        }

        ++func_desc;
        ++i;
    }

    if (user_vft > This->typeattr.cbSizeVft)
        This->typeattr.cbSizeVft = user_vft + This->pTypeLib->ptr_size;

    for(i = 0; i < This->typeattr.cVars; ++i){
        TLBVarDesc *var_desc = &This->vardescs[i];
        if(var_desc->vardesc.memid == MEMBERID_NIL){
            UINT j = 0;
            BOOL reset = FALSE;
            TLBVarDesc *iter;

            var_desc->vardesc.memid = 0x40000000 + (depth << 16) + i;

            iter = This->vardescs;
            while (j < This->typeattr.cVars) {
                if (iter != var_desc && iter->vardesc.memid == var_desc->vardesc.memid) {
                    if (!reset) {
                        var_desc->vardesc.memid = 0x40000000 + (depth << 16) + This->typeattr.cVars;
                        reset = TRUE;
                    } else
                        ++var_desc->vardesc.memid;
                    iter = This->vardescs;
                    j = 0;
                } else {
                    ++iter;
                    ++j;
                }
            }
        }
    }

    return hres;
}

static HRESULT WINAPI ICreateTypeInfo2_fnDeleteFuncDesc(ICreateTypeInfo2 *iface,
        UINT index)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    unsigned int i;

    TRACE("%p %u\n", This, index);

    if (index >= This->typeattr.cFuncs)
        return TYPE_E_ELEMENTNOTFOUND;

    typeinfo_release_funcdesc(&This->funcdescs[index]);

    --This->typeattr.cFuncs;
    if (index != This->typeattr.cFuncs)
    {
        memmove(This->funcdescs + index, This->funcdescs + index + 1,
                sizeof(*This->funcdescs) * (This->typeattr.cFuncs - index));
        for (i = index; i < This->typeattr.cFuncs; ++i)
            TLB_relink_custdata(&This->funcdescs[i].custdata_list);
    }

    This->needs_layout = TRUE;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnDeleteFuncDescByMemId(ICreateTypeInfo2 *iface,
        MEMBERID memid, INVOKEKIND invKind)
{
    FIXME("%p, %#lx, %d - stub\n", iface, memid, invKind);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeInfo2_fnDeleteVarDesc(ICreateTypeInfo2 *iface,
        UINT index)
{
    FIXME("%p, %u - stub\n", iface, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeInfo2_fnDeleteVarDescByMemId(ICreateTypeInfo2 *iface,
        MEMBERID memid)
{
    FIXME("%p, %#lx - stub\n", iface, memid);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeInfo2_fnDeleteImplType(ICreateTypeInfo2 *iface,
        UINT index)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    int i;

    TRACE("%p %u\n", This, index);

    if (index >= This->typeattr.cImplTypes)
        return TYPE_E_ELEMENTNOTFOUND;

    TLB_FreeCustData(&This->impltypes[index].custdata_list);
    --This->typeattr.cImplTypes;

    if (index < This->typeattr.cImplTypes)
    {
        memmove(This->impltypes + index, This->impltypes + index + 1, (This->typeattr.cImplTypes - index) *
                sizeof(*This->impltypes));
        for (i = index; i < This->typeattr.cImplTypes; ++i)
            TLB_relink_custdata(&This->impltypes[i].custdata_list);
    }

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetCustData(ICreateTypeInfo2 *iface,
        REFGUID guid, VARIANT *varVal)
{
    TLBGuid *tlbguid;

    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(guid), varVal);

    if (!guid || !varVal)
        return E_INVALIDARG;

    tlbguid = TLB_append_guid(&This->pTypeLib->guid_list, guid, -1);

    return TLB_set_custdata(This->pcustdata_list, tlbguid, varVal);
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetFuncCustData(ICreateTypeInfo2 *iface,
        UINT index, REFGUID guid, VARIANT *varVal)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    FIXME("%p %u %s %p - stub\n", This, index, debugstr_guid(guid), varVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetParamCustData(ICreateTypeInfo2 *iface,
        UINT funcIndex, UINT paramIndex, REFGUID guid, VARIANT *varVal)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    FIXME("%p %u %u %s %p - stub\n", This, funcIndex, paramIndex, debugstr_guid(guid), varVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetVarCustData(ICreateTypeInfo2 *iface,
        UINT index, REFGUID guid, VARIANT *varVal)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    FIXME("%p %u %s %p - stub\n", This, index, debugstr_guid(guid), varVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetImplTypeCustData(ICreateTypeInfo2 *iface,
        UINT index, REFGUID guid, VARIANT *varVal)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);
    FIXME("%p %u %s %p - stub\n", This, index, debugstr_guid(guid), varVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetHelpStringContext(ICreateTypeInfo2 *iface,
        ULONG helpStringContext)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    TRACE("%p, %lu.\n", iface, helpStringContext);

    This->dwHelpStringContext = helpStringContext;

    return S_OK;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetFuncHelpStringContext(ICreateTypeInfo2 *iface,
        UINT index, ULONG helpStringContext)
{
    FIXME("%p, %u, %lu - stub\n", iface, index, helpStringContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetVarHelpStringContext(ICreateTypeInfo2 *iface,
        UINT index, ULONG helpStringContext)
{
    FIXME("%p, %u, %lu - stub\n", iface, index, helpStringContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeInfo2_fnInvalidate(ICreateTypeInfo2 *iface)
{
    FIXME("%p - stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICreateTypeInfo2_fnSetName(ICreateTypeInfo2 *iface,
        LPOLESTR name)
{
    ITypeInfoImpl *This = info_impl_from_ICreateTypeInfo2(iface);

    TRACE("%p %s\n", This, wine_dbgstr_w(name));

    if (!name)
        return E_INVALIDARG;

    This->Name = TLB_append_str(&This->pTypeLib->name_list, name);

    return S_OK;
}

static const ICreateTypeInfo2Vtbl CreateTypeInfo2Vtbl = {
    ICreateTypeInfo2_fnQueryInterface,
    ICreateTypeInfo2_fnAddRef,
    ICreateTypeInfo2_fnRelease,
    ICreateTypeInfo2_fnSetGuid,
    ICreateTypeInfo2_fnSetTypeFlags,
    ICreateTypeInfo2_fnSetDocString,
    ICreateTypeInfo2_fnSetHelpContext,
    ICreateTypeInfo2_fnSetVersion,
    ICreateTypeInfo2_fnAddRefTypeInfo,
    ICreateTypeInfo2_fnAddFuncDesc,
    ICreateTypeInfo2_fnAddImplType,
    ICreateTypeInfo2_fnSetImplTypeFlags,
    ICreateTypeInfo2_fnSetAlignment,
    ICreateTypeInfo2_fnSetSchema,
    ICreateTypeInfo2_fnAddVarDesc,
    ICreateTypeInfo2_fnSetFuncAndParamNames,
    ICreateTypeInfo2_fnSetVarName,
    ICreateTypeInfo2_fnSetTypeDescAlias,
    ICreateTypeInfo2_fnDefineFuncAsDllEntry,
    ICreateTypeInfo2_fnSetFuncDocString,
    ICreateTypeInfo2_fnSetVarDocString,
    ICreateTypeInfo2_fnSetFuncHelpContext,
    ICreateTypeInfo2_fnSetVarHelpContext,
    ICreateTypeInfo2_fnSetMops,
    ICreateTypeInfo2_fnSetTypeIdldesc,
    ICreateTypeInfo2_fnLayOut,
    ICreateTypeInfo2_fnDeleteFuncDesc,
    ICreateTypeInfo2_fnDeleteFuncDescByMemId,
    ICreateTypeInfo2_fnDeleteVarDesc,
    ICreateTypeInfo2_fnDeleteVarDescByMemId,
    ICreateTypeInfo2_fnDeleteImplType,
    ICreateTypeInfo2_fnSetCustData,
    ICreateTypeInfo2_fnSetFuncCustData,
    ICreateTypeInfo2_fnSetParamCustData,
    ICreateTypeInfo2_fnSetVarCustData,
    ICreateTypeInfo2_fnSetImplTypeCustData,
    ICreateTypeInfo2_fnSetHelpStringContext,
    ICreateTypeInfo2_fnSetFuncHelpStringContext,
    ICreateTypeInfo2_fnSetVarHelpStringContext,
    ICreateTypeInfo2_fnInvalidate,
    ICreateTypeInfo2_fnSetName
};

/******************************************************************************
 * ClearCustData (OLEAUT32.171)
 *
 * Clear a custom data type's data.
 *
 * PARAMS
 *  lpCust [I] The custom data type instance
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI ClearCustData(CUSTDATA *lpCust)
{
    if (lpCust && lpCust->cCustData)
    {
        if (lpCust->prgCustData)
        {
            DWORD i;

            for (i = 0; i < lpCust->cCustData; i++)
                VariantClear(&lpCust->prgCustData[i].varValue);

            CoTaskMemFree(lpCust->prgCustData);
            lpCust->prgCustData = NULL;
        }
        lpCust->cCustData = 0;
    }
}
