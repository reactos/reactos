/*
 *	TYPELIB
 *
 *	Copyright 1997	Marcus Meissner
 *		      1999  Rein Klazes
 *		      2000  Francois Jacques
 *		      2001  Huw D M Davies for CodeWeavers
 *		      2005  Robert Shearman, for CodeWeavers
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winuser.h"
#include "lzexpand.h"

#include "wine/winbase16.h"
#include "wine/unicode.h"
#include "objbase.h"
#include "typelib.h"
#include "wine/debug.h"
#include "variant.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);
WINE_DECLARE_DEBUG_CHANNEL(typelib);

static HRESULT typedescvt_to_variantvt(ITypeInfo *tinfo, const TYPEDESC *tdesc, VARTYPE *vt);
static HRESULT TLB_AllocAndInitVarDesc(const VARDESC *src, VARDESC **dest_ptr);

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
static BOOL find_typelib_key( REFGUID guid, WORD wMaj, WORD *wMin )
{
    static const WCHAR typelibW[] = {'T','y','p','e','l','i','b','\\',0};
    WCHAR buffer[60];
    char key_name[16];
    DWORD len, i;
    INT best_min = -1;
    HKEY hkey;

    memcpy( buffer, typelibW, sizeof(typelibW) );
    StringFromGUID2( guid, buffer + strlenW(buffer), 40 );

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

            if (wMaj == v_maj)
            {
                if (*wMin == v_min)
                {
                    best_min = v_min;
                    break; /* exact match */
                }
                if (v_min > best_min) best_min = v_min;
            }
        }
        len = sizeof(key_name);
    }
    RegCloseKey( hkey );
    if (best_min >= 0)
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
    static const WCHAR TypelibW[] = {'T','y','p','e','l','i','b','\\',0};
    static const WCHAR VersionFormatW[] = {'\\','%','x','.','%','x',0};

    memcpy( buffer, TypelibW, sizeof(TypelibW) );
    StringFromGUID2( guid, buffer + strlenW(buffer), 40 );
    sprintfW( buffer + strlenW(buffer), VersionFormatW, wMaj, wMin );
    return buffer;
}

/* get the path of an interface key, in the form "Interface\\<guid>" */
/* buffer must be at least 50 characters long */
static WCHAR *get_interface_key( REFGUID guid, WCHAR *buffer )
{
    static const WCHAR InterfaceW[] = {'I','n','t','e','r','f','a','c','e','\\',0};

    memcpy( buffer, InterfaceW, sizeof(InterfaceW) );
    StringFromGUID2( guid, buffer + strlenW(buffer), 40 );
    return buffer;
}

/* get the lcid subkey for a typelib, in the form "<lcid>\\<syskind>" */
/* buffer must be at least 16 characters long */
static WCHAR *get_lcid_subkey( LCID lcid, SYSKIND syskind, WCHAR *buffer )
{
    static const WCHAR LcidFormatW[] = {'%','l','x','\\',0};
    static const WCHAR win16W[] = {'w','i','n','1','6',0};
    static const WCHAR win32W[] = {'w','i','n','3','2',0};

    sprintfW( buffer, LcidFormatW, lcid );
    switch(syskind)
    {
    case SYS_WIN16: strcatW( buffer, win16W ); break;
    case SYS_WIN32: strcatW( buffer, win32W ); break;
    default:
        TRACE("Typelib is for unsupported syskind %i\n", syskind);
        return NULL;
    }
    return buffer;
}

static HRESULT TLB_ReadTypeLib(LPCWSTR pszFileName, LPWSTR pszPath, UINT cchPath, ITypeLib2 **ppTypeLib);


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
HRESULT WINAPI QueryPathOfRegTypeLib(
	REFGUID guid,
	WORD wMaj,
	WORD wMin,
	LCID lcid,
	LPBSTR path )
{
    HRESULT hr = TYPE_E_LIBNOTREGISTERED;
    LCID myLCID = lcid;
    HKEY hkey;
    WCHAR buffer[60];
    WCHAR Path[MAX_PATH];
    LONG res;

    TRACE_(typelib)("(%s, %x.%x, 0x%x, %p)\n", debugstr_guid(guid), wMaj, wMin, lcid, path);

    if (!find_typelib_key( guid, wMaj, &wMin )) return TYPE_E_LIBNOTREGISTERED;
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

        get_lcid_subkey( myLCID, SYS_WIN32, buffer );

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
    TRACE_(typelib)("-- 0x%08x\n", hr);
    return hr;
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
                    IUnknown_Release(*pptLib);
                    *pptLib = 0;
                }
                break;
            case REGKIND_NONE:
                break;
        }

    TRACE(" returns %08x\n",res);
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
    }

    TRACE("(IID: %s) load %s (%p)\n",debugstr_guid(rguid), SUCCEEDED(res)? "SUCCESS":"FAILED", *ppTLib);

    return res;
}


/* some string constants shared between RegisterTypeLib and UnRegisterTypeLib */
static const WCHAR TypeLibW[] = {'T','y','p','e','L','i','b',0};
static const WCHAR FLAGSW[] = {'F','L','A','G','S',0};
static const WCHAR HELPDIRW[] = {'H','E','L','P','D','I','R',0};
static const WCHAR ProxyStubClsidW[] = {'P','r','o','x','y','S','t','u','b','C','l','s','i','d',0};
static const WCHAR ProxyStubClsid32W[] = {'P','r','o','x','y','S','t','u','b','C','l','s','i','d','3','2',0};

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
HRESULT WINAPI RegisterTypeLib(
     ITypeLib * ptlib,     /* [in] Pointer to the library*/
     OLECHAR * szFullPath, /* [in] full Path of the library*/
     OLECHAR * szHelpDir)  /* [in] dir to the helpfile for the library,
							 may be NULL*/
{
    static const WCHAR PSOA[] = {'{','0','0','0','2','0','4','2','4','-',
                                 '0','0','0','0','-','0','0','0','0','-','C','0','0','0','-',
                                 '0','0','0','0','0','0','0','0','0','0','4','6','}',0};
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

        /* Set the human-readable name of the typelib */
        if (SUCCEEDED(ITypeLib_GetDocumentation(ptlib, -1, NULL, &doc, NULL, NULL)))
        {
            if (RegSetValueExW(key, NULL, 0, REG_SZ,
                (BYTE *)doc, (lstrlenW(doc)+1) * sizeof(OLECHAR)) != ERROR_SUCCESS)
                res = E_FAIL;

            SysFreeString(doc);
        }
        else
            res = E_FAIL;

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
        if (res == S_OK && RegCreateKeyExW(key, FLAGSW, 0, NULL, 0,
            KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS)
        {
            /* FIXME: is %u correct? */
            static const WCHAR formatW[] = {'%','u',0};
            WCHAR buf[20];
            sprintfW(buf, formatW, attr->wLibFlags);
            if (RegSetValueExW(subKey, NULL, 0, REG_SZ,
                               (BYTE *)buf, (strlenW(buf) + 1)*sizeof(WCHAR) ) != ERROR_SUCCESS)
                res = E_FAIL;

            RegCloseKey(subKey);
        }
        else
            res = E_FAIL;

        /* create the helpdir subkey */
        if (res == S_OK && RegCreateKeyExW(key, HELPDIRW, 0, NULL, 0,
            KEY_WRITE, NULL, &subKey, &disposition) == ERROR_SUCCESS)
        {
            BOOL freeHelpDir = FALSE;
            OLECHAR* pIndexStr;

            /* if we created a new key, and helpDir was null, set the helpdir
               to the directory which contains the typelib. However,
               if we just opened an existing key, we leave the helpdir alone */
            if ((disposition == REG_CREATED_NEW_KEY) && (szHelpDir == NULL)) {
                szHelpDir = SysAllocString(szFullPath);
                pIndexStr = strrchrW(szHelpDir, '\\');
                if (pIndexStr) {
                    *pIndexStr = 0;
                }
                freeHelpDir = TRUE;
            }

            /* if we have an szHelpDir, set it! */
            if (szHelpDir != NULL) {
                if (RegSetValueExW(subKey, NULL, 0, REG_SZ,
                    (BYTE *)szHelpDir, (lstrlenW(szHelpDir)+1) * sizeof(OLECHAR)) != ERROR_SUCCESS) {
                    res = E_FAIL;
                }
            }

            /* tidy up */
            if (freeHelpDir) SysFreeString(szHelpDir);
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

		    if (tattr->wTypeFlags & (TYPEFLAG_FOLEAUTOMATION|TYPEFLAG_FDUAL|TYPEFLAG_FDISPATCHABLE))
		    {
			/* register interface<->typelib coupling */
			get_interface_key( &tattr->guid, keyName );
			if (RegCreateKeyExW(HKEY_CLASSES_ROOT, keyName, 0, NULL, 0,
					    KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
			{
			    if (name)
				RegSetValueExW(key, NULL, 0, REG_SZ,
					       (BYTE *)name, (strlenW(name)+1) * sizeof(OLECHAR));

			    if (RegCreateKeyExW(key, ProxyStubClsidW, 0, NULL, 0,
				KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS) {
				RegSetValueExW(subKey, NULL, 0, REG_SZ,
					       (const BYTE *)PSOA, sizeof PSOA);
				RegCloseKey(subKey);
			    }

			    if (RegCreateKeyExW(key, ProxyStubClsid32W, 0, NULL, 0,
				KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS) {
				RegSetValueExW(subKey, NULL, 0, REG_SZ,
					       (const BYTE *)PSOA, sizeof PSOA);
				RegCloseKey(subKey);
			    }

			    if (RegCreateKeyExW(key, TypeLibW, 0, NULL, 0,
				KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS)
			    {
				WCHAR buffer[40];
				static const WCHAR fmtver[] = {'%','x','.','%','x',0 };
				static const WCHAR VersionW[] = {'V','e','r','s','i','o','n',0};

				StringFromGUID2(&attr->guid, buffer, 40);
				RegSetValueExW(subKey, NULL, 0, REG_SZ,
					       (BYTE *)buffer, (strlenW(buffer)+1) * sizeof(WCHAR));
				sprintfW(buffer, fmtver, attr->wMajorVerNum, attr->wMinorVerNum);
				RegSetValueExW(subKey, VersionW, 0, REG_SZ,
					       (BYTE*)buffer, (strlenW(buffer)+1) * sizeof(WCHAR));
				RegCloseKey(subKey);
			    }

			    RegCloseKey(key);
			}
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
    HKEY subKey = NULL;
    TYPEATTR* typeAttr = NULL;
    TYPEKIND kind;
    ITypeInfo* typeInfo = NULL;
    ITypeLib* typeLib = NULL;
    int numTypes;

    TRACE("(IID: %s)\n",debugstr_guid(libid));

    /* Create the path to the key */
    get_typelib_key( libid, wVerMajor, wVerMinor, keyName );

    if (syskind != SYS_WIN16 && syskind != SYS_WIN32)
    {
        TRACE("Unsupported syskind %i\n", syskind);
        result = E_INVALIDARG;
        goto end;
    }

    /* get the path to the typelib on disk */
    if (QueryPathOfRegTypeLib(libid, wVerMajor, wVerMinor, lcid, &tlibPath) != S_OK) {
        result = E_INVALIDARG;
        goto end;
    }

    /* Try and open the key to the type library. */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, keyName, 0, KEY_READ | KEY_WRITE, &key) != ERROR_SUCCESS) {
        result = E_INVALIDARG;
        goto end;
    }

    /* Try and load the type library */
    if (LoadTypeLibEx(tlibPath, REGKIND_NONE, &typeLib)) {
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

        /* the path to the type */
        get_interface_key( &typeAttr->guid, subKeyName );

        /* Delete its bits */
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, subKeyName, 0, KEY_WRITE, &subKey) != ERROR_SUCCESS) {
            goto enddeleteloop;
        }
        RegDeleteKeyW(subKey, ProxyStubClsidW);
        RegDeleteKeyW(subKey, ProxyStubClsid32W);
        RegDeleteKeyW(subKey, TypeLibW);
        RegCloseKey(subKey);
        subKey = NULL;
        RegDeleteKeyW(HKEY_CLASSES_ROOT, subKeyName);

enddeleteloop:
        if (typeAttr) ITypeInfo_ReleaseTypeAttr(typeInfo, typeAttr);
        typeAttr = NULL;
        if (typeInfo) ITypeInfo_Release(typeInfo);
        typeInfo = NULL;
    }

    /* Now, delete the type library path subkey */
    get_lcid_subkey( lcid, syskind, subKeyName );
    RegDeleteKeyW(key, subKeyName);
    *strrchrW( subKeyName, '\\' ) = 0;  /* remove last path component */
    RegDeleteKeyW(key, subKeyName);

    /* check if there is anything besides the FLAGS/HELPDIR keys.
       If there is, we don't delete them */
    tmpLength = sizeof(subKeyName)/sizeof(WCHAR);
    deleteOtherStuff = TRUE;
    i = 0;
    while(RegEnumKeyExW(key, i++, subKeyName, &tmpLength, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        tmpLength = sizeof(subKeyName)/sizeof(WCHAR);

        /* if its not FLAGS or HELPDIR, then we must keep the rest of the key */
        if (!strcmpW(subKeyName, FLAGSW)) continue;
        if (!strcmpW(subKeyName, HELPDIRW)) continue;
        deleteOtherStuff = FALSE;
        break;
    }

    /* only delete the other parts of the key if we're absolutely sure */
    if (deleteOtherStuff) {
        RegDeleteKeyW(key, FLAGSW);
        RegDeleteKeyW(key, HELPDIRW);
        RegCloseKey(key);
        key = NULL;

        RegDeleteKeyW(HKEY_CLASSES_ROOT, keyName);
        *strrchrW( keyName, '\\' ) = 0;  /* remove last path component */
        RegDeleteKeyW(HKEY_CLASSES_ROOT, keyName);
    }

end:
    SysFreeString(tlibPath);
    if (typeLib) ITypeLib_Release(typeLib);
    if (subKey) RegCloseKey(subKey);
    if (key) RegCloseKey(key);
    return result;
}

/*======================= ITypeLib implementation =======================*/

typedef struct tagTLBCustData
{
    GUID guid;
    VARIANT data;
    struct tagTLBCustData* next;
} TLBCustData;

/* data structure for import typelibs */
typedef struct tagTLBImpLib
{
    int offset;                 /* offset in the file (MSFT)
				   offset in nametable (SLTG)
				   just used to identify library while reading
				   data from file */
    GUID guid;                  /* libid */
    BSTR name;                  /* name */

    LCID lcid;                  /* lcid of imported typelib */

    WORD wVersionMajor;         /* major version number */
    WORD wVersionMinor;         /* minor version number */

    struct tagITypeLibImpl *pImpTypeLib; /* pointer to loaded typelib, or
					    NULL if not yet loaded */
    struct tagTLBImpLib * next;
} TLBImpLib;

/* internal ITypeLib data */
typedef struct tagITypeLibImpl
{
    const ITypeLib2Vtbl *lpVtbl;
    const ITypeCompVtbl *lpVtblTypeComp;
    LONG ref;
    TLIBATTR LibAttr;            /* guid,lcid,syskind,version,flags */

    /* strings can be stored in tlb as multibyte strings BUT they are *always*
     * exported to the application as a UNICODE string.
     */
    BSTR Name;
    BSTR DocString;
    BSTR HelpFile;
    BSTR HelpStringDll;
    unsigned long  dwHelpContext;
    int TypeInfoCount;          /* nr of typeinfo's in librarry */
    struct tagITypeInfoImpl *pTypeInfo;   /* linked list of type info data */
    int ctCustData;             /* number of items in cust data list */
    TLBCustData * pCustData;    /* linked list to cust data */
    TLBImpLib   * pImpLibs;     /* linked list to all imported typelibs */
    int ctTypeDesc;             /* number of items in type desc array */
    TYPEDESC * pTypeDesc;       /* array of TypeDescriptions found in the
				   library. Only used while reading MSFT
				   typelibs */
    struct list ref_list;       /* list of ref types in this typelib */
    HREFTYPE dispatch_href;     /* reference to IDispatch, -1 if unused */


    /* typelibs are cached, keyed by path and index, so store the linked list info within them */
    struct tagITypeLibImpl *next, *prev;
    WCHAR *path;
    INT index;
} ITypeLibImpl;

static const ITypeLib2Vtbl tlbvt;
static const ITypeCompVtbl tlbtcvt;

static inline ITypeLibImpl *impl_from_ITypeComp( ITypeComp *iface )
{
    return (ITypeLibImpl *)((char*)iface - FIELD_OFFSET(ITypeLibImpl, lpVtblTypeComp));
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

    GUID guid;              /* guid of the referenced type */
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
    BSTR Name;
    int ctCustData;
    TLBCustData * pCustData;        /* linked list to cust data */
} TLBParDesc;

/* internal Function data */
typedef struct tagTLBFuncDesc
{
    FUNCDESC funcdesc;      /* lots of info on the function and its attributes. */
    BSTR Name;             /* the name of this function */
    TLBParDesc *pParamDesc; /* array with param names and custom data */
    int helpcontext;
    int HelpStringContext;
    BSTR HelpString;
    BSTR Entry;            /* if its Hiword==0, it numeric; -1 is not present*/
    int ctCustData;
    TLBCustData * pCustData;        /* linked list to cust data; */
    struct tagTLBFuncDesc * next;
} TLBFuncDesc;

/* internal Variable data */
typedef struct tagTLBVarDesc
{
    VARDESC vardesc;        /* lots of info on the variable and its attributes. */
    BSTR Name;             /* the name of this variable */
    int HelpContext;
    int HelpStringContext;  /* FIXME: where? */
    BSTR HelpString;
    int ctCustData;
    TLBCustData * pCustData;/* linked list to cust data; */
    struct tagTLBVarDesc * next;
} TLBVarDesc;

/* internal implemented interface data */
typedef struct tagTLBImplType
{
    HREFTYPE hRef;          /* hRef of interface */
    int implflags;          /* IMPLFLAG_*s */
    int ctCustData;
    TLBCustData * pCustData;/* linked list to custom data; */
    struct tagTLBImplType *next;
} TLBImplType;

/* internal TypeInfo data */
typedef struct tagITypeInfoImpl
{
    const ITypeInfo2Vtbl *lpVtbl;
    const ITypeCompVtbl  *lpVtblTypeComp;
    LONG ref;
    BOOL no_free_data; /* don't free data structures */
    TYPEATTR TypeAttr ;         /* _lots_ of type information. */
    ITypeLibImpl * pTypeLib;        /* back pointer to typelib */
    int index;                  /* index in this typelib; */
    HREFTYPE hreftype;          /* hreftype for app object binding */
    /* type libs seem to store the doc strings in ascii
     * so why should we do it in unicode?
     */
    BSTR Name;
    BSTR DocString;
    BSTR DllName;
    unsigned long  dwHelpContext;
    unsigned long  dwHelpStringContext;

    /* functions  */
    TLBFuncDesc * funclist;     /* linked list with function descriptions */

    /* variables  */
    TLBVarDesc * varlist;       /* linked list with variable descriptions */

    /* Implemented Interfaces  */
    TLBImplType * impltypelist;

    int ctCustData;
    TLBCustData * pCustData;        /* linked list to cust data; */
    struct tagITypeInfoImpl * next;
} ITypeInfoImpl;

static inline ITypeInfoImpl *info_impl_from_ITypeComp( ITypeComp *iface )
{
    return (ITypeInfoImpl *)((char*)iface - FIELD_OFFSET(ITypeInfoImpl, lpVtblTypeComp));
}

static const ITypeInfo2Vtbl tinfvt;
static const ITypeCompVtbl  tcompvt;

static ITypeInfo2 * ITypeInfo_Constructor(void);

typedef struct tagTLBContext
{
	unsigned int oStart;  /* start of TLB in file */
	unsigned int pos;     /* current pos */
	unsigned int length;  /* total length */
	void *mapping;        /* memory mapping */
	MSFT_SegDir * pTblDir;
	ITypeLibImpl* pLibInfo;
} TLBContext;


static void MSFT_DoRefType(TLBContext *pcx, ITypeLibImpl *pTL, int offset);

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
    case VT_USERDEFINED: sprintf(szVarType, "VT_USERDEFINED ref = %x",
				 pTD->u.hreftype); break;
    case VT_LPSTR: sprintf(szVarType, "VT_LPSTR"); break;
    case VT_LPWSTR: sprintf(szVarType, "VT_LPWSTR"); break;
    case VT_PTR: sprintf(szVarType, "ptr to ");
      dump_TypeDesc(pTD->u.lptdesc, szVarType + 7);
      break;
    case VT_SAFEARRAY: sprintf(szVarType, "safearray of ");
      dump_TypeDesc(pTD->u.lptdesc, szVarType + 13);
      break;
    case VT_CARRAY: sprintf(szVarType, "%d dim array of ",
			    pTD->u.lpadesc->cDims); /* FIXME print out sizes */
      dump_TypeDesc(&pTD->u.lpadesc->tdescElem, szVarType + strlen(szVarType));
      break;

    default: sprintf(szVarType, "unknown(%d)", pTD->vt & VT_TYPEMASK); break;
    }
}

static void dump_ELEMDESC(const ELEMDESC *edesc) {
  char buf[200];
  USHORT flags = edesc->u.paramdesc.wParamFlags;
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
  MESSAGE("\n\t\tu.paramdesc.lpex %p\n",edesc->u.paramdesc.pparamdescex);
}
static void dump_FUNCDESC(const FUNCDESC *funcdesc) {
  int i;
  MESSAGE("memid is %08x\n",funcdesc->memid);
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
  MESSAGE("%s(%u)\n", debugstr_w(pfd->Name), pfd->funcdesc.cParams);
  for (i=0;i<pfd->funcdesc.cParams;i++)
      MESSAGE("\tparm%d: %s\n",i,debugstr_w(pfd->pParamDesc[i].Name));


  dump_FUNCDESC(&(pfd->funcdesc));

  MESSAGE("\thelpstring: %s\n", debugstr_w(pfd->HelpString));
  MESSAGE("\tentry: %s\n", (pfd->Entry == (void *)-1) ? "invalid" : debugstr_w(pfd->Entry));
}
static void dump_TLBFuncDesc(const TLBFuncDesc * pfd)
{
	while (pfd)
	{
	  dump_TLBFuncDescOne(pfd);
	  pfd = pfd->next;
	};
}
static void dump_TLBVarDesc(const TLBVarDesc * pvd)
{
	while (pvd)
	{
	  TRACE_(typelib)("%s\n", debugstr_w(pvd->Name));
	  pvd = pvd->next;
	};
}

static void dump_TLBImpLib(const TLBImpLib *import)
{
    TRACE_(typelib)("%s %s\n", debugstr_guid(&(import->guid)),
		    debugstr_w(import->name));
    TRACE_(typelib)("v%d.%d lcid=%x offset=%x\n", import->wVersionMajor,
		    import->wVersionMinor, import->lcid, import->offset);
}

static void dump_TLBRefType(const ITypeLibImpl *pTL)
{
    TLBRefType *ref;

    LIST_FOR_EACH_ENTRY(ref, &pTL->ref_list, TLBRefType, entry)
    {
        TRACE_(typelib)("href:0x%08x\n", ref->reference);
        if(ref->index == -1)
	    TRACE_(typelib)("%s\n", debugstr_guid(&(ref->guid)));
        else
	    TRACE_(typelib)("type no: %d\n", ref->index);

        if(ref->pImpTLInfo != TLB_REF_INTERNAL && ref->pImpTLInfo != TLB_REF_NOT_FOUND)
        {
            TRACE_(typelib)("in lib\n");
            dump_TLBImpLib(ref->pImpTLInfo);
        }
    }
}

static void dump_TLBImplType(const TLBImplType * impl)
{
    while (impl) {
        TRACE_(typelib)(
		"implementing/inheriting interface hRef = %x implflags %x\n",
		impl->hRef, impl->implflags);
	impl = impl->next;
    }
}

static void dump_Variant(const VARIANT * pvar)
{
    SYSTEMTIME st;

    TRACE("%p->{%s%s", pvar, debugstr_VT(pvar), debugstr_VF(pvar));

    if (pvar)
    {
      if (V_ISBYREF(pvar) || V_TYPE(pvar) == VT_UNKNOWN ||
          V_TYPE(pvar) == VT_DISPATCH || V_TYPE(pvar) == VT_RECORD)
      {
        TRACE(",%p", V_BYREF(pvar));
      }
      else if (V_ISARRAY(pvar) || V_ISVECTOR(pvar))
      {
        TRACE(",%p", V_ARRAY(pvar));
      }
      else switch (V_TYPE(pvar))
      {
      case VT_I1:   TRACE(",%d", V_I1(pvar)); break;
      case VT_UI1:  TRACE(",%d", V_UI1(pvar)); break;
      case VT_I2:   TRACE(",%d", V_I2(pvar)); break;
      case VT_UI2:  TRACE(",%d", V_UI2(pvar)); break;
      case VT_INT:
      case VT_I4:   TRACE(",%d", V_I4(pvar)); break;
      case VT_UINT:
      case VT_UI4:  TRACE(",%d", V_UI4(pvar)); break;
      case VT_I8:   TRACE(",0x%08x,0x%08x", (ULONG)(V_I8(pvar) >> 32),
                          (ULONG)(V_I8(pvar) & 0xffffffff)); break;
      case VT_UI8:  TRACE(",0x%08x,0x%08x", (ULONG)(V_UI8(pvar) >> 32),
                          (ULONG)(V_UI8(pvar) & 0xffffffff)); break;
      case VT_R4:   TRACE(",%3.3e", V_R4(pvar)); break;
      case VT_R8:   TRACE(",%3.3e", V_R8(pvar)); break;
      case VT_BOOL: TRACE(",%s", V_BOOL(pvar) ? "TRUE" : "FALSE"); break;
      case VT_BSTR: TRACE(",%s", debugstr_w(V_BSTR(pvar))); break;
      case VT_CY:   TRACE(",0x%08x,0x%08x", V_CY(pvar).s.Hi,
                           V_CY(pvar).s.Lo); break;
      case VT_DATE:
        if(!VariantTimeToSystemTime(V_DATE(pvar), &st))
          TRACE(",<invalid>");
        else
          TRACE(",%04d/%02d/%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond);
        break;
      case VT_ERROR:
      case VT_VOID:
      case VT_USERDEFINED:
      case VT_EMPTY:
      case VT_NULL:  break;
      default:       TRACE(",?"); break;
      }
    }
    TRACE("}\n");
}

static void dump_DispParms(const DISPPARAMS * pdp)
{
    unsigned int index;

    TRACE("args=%u named args=%u\n", pdp->cArgs, pdp->cNamedArgs);

    if (pdp->cNamedArgs && pdp->rgdispidNamedArgs)
    {
        TRACE("named args:\n");
        for (index = 0; index < pdp->cNamedArgs; index++)
            TRACE( "\t0x%x\n", pdp->rgdispidNamedArgs[index] );
    }

    if (pdp->cArgs && pdp->rgvarg)
    {
        TRACE("args:\n");
        for (index = 0; index < pdp->cArgs; index++)
            dump_Variant( &pdp->rgvarg[index] );
    }
}

static void dump_TypeInfo(const ITypeInfoImpl * pty)
{
    TRACE("%p ref=%u\n", pty, pty->ref);
    TRACE("%s %s\n", debugstr_w(pty->Name), debugstr_w(pty->DocString));
    TRACE("attr:%s\n", debugstr_guid(&(pty->TypeAttr.guid)));
    TRACE("kind:%s\n", typekind_desc[pty->TypeAttr.typekind]);
    TRACE("fct:%u var:%u impl:%u\n",
      pty->TypeAttr.cFuncs, pty->TypeAttr.cVars, pty->TypeAttr.cImplTypes);
    TRACE("wTypeFlags: 0x%04x\n", pty->TypeAttr.wTypeFlags);
    TRACE("parent tlb:%p index in TLB:%u\n",pty->pTypeLib, pty->index);
    if (pty->TypeAttr.typekind == TKIND_MODULE) TRACE("dllname:%s\n", debugstr_w(pty->DllName));
    if (TRACE_ON(ole))
        dump_TLBFuncDesc(pty->funclist);
    dump_TLBVarDesc(pty->varlist);
    dump_TLBImplType(pty->impltypelist);
}

static void dump_VARDESC(const VARDESC *v)
{
    MESSAGE("memid %d\n",v->memid);
    MESSAGE("lpstrSchema %s\n",debugstr_w(v->lpstrSchema));
    MESSAGE("oInst %d\n",v->u.oInst);
    dump_ELEMDESC(&(v->elemdescVar));
    MESSAGE("wVarFlags %x\n",v->wVarFlags);
    MESSAGE("varkind %d\n",v->varkind);
}

static TYPEDESC stndTypeDesc[VT_LPWSTR+1]=
{
    /* VT_LPWSTR is largest type that */
    /* may appear in type description*/
    {{0}, 0},{{0}, 1},{{0}, 2},{{0}, 3},{{0}, 4},
    {{0}, 5},{{0}, 6},{{0}, 7},{{0}, 8},{{0}, 9},
    {{0},10},{{0},11},{{0},12},{{0},13},{{0},14},
    {{0},15},{{0},16},{{0},17},{{0},18},{{0},19},
    {{0},20},{{0},21},{{0},22},{{0},23},{{0},24},
    {{0},25},{{0},26},{{0},27},{{0},28},{{0},29},
    {{0},30},{{0},31}
};

static void TLB_abort(void)
{
    DebugBreak();
}

static void * TLB_Alloc(unsigned size) __WINE_ALLOC_SIZE(1);
static void * TLB_Alloc(unsigned size)
{
    void * ret;
    if((ret=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size))==NULL){
        /* FIXME */
        ERR("cannot allocate memory\n");
    }
    return ret;
}

static void TLB_Free(void * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
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
        size += TLB_SizeTypeDesc(tdesc->u.lptdesc, TRUE);
        break;
    case VT_CARRAY:
        size += FIELD_OFFSET(ARRAYDESC, rgbounds[tdesc->u.lpadesc->cDims]);
        size += TLB_SizeTypeDesc(&tdesc->u.lpadesc->tdescElem, FALSE);
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
        dest->u.lptdesc = buffer;
        buffer = TLB_CopyTypeDesc(NULL, src->u.lptdesc, buffer);
        break;
    case VT_CARRAY:
        dest->u.lpadesc = buffer;
        memcpy(dest->u.lpadesc, src->u.lpadesc, FIELD_OFFSET(ARRAYDESC, rgbounds[src->u.lpadesc->cDims]));
        buffer = (char *)buffer + FIELD_OFFSET(ARRAYDESC, rgbounds[src->u.lpadesc->cDims]);
        buffer = TLB_CopyTypeDesc(&dest->u.lpadesc->tdescElem, &src->u.lpadesc->tdescElem, buffer);
        break;
    }
    return buffer;
}

/* free custom data allocated by MSFT_CustData */
static inline void TLB_FreeCustData(TLBCustData *pCustData)
{
    TLBCustData *pCustDataNext;
    for (; pCustData; pCustData = pCustDataNext)
    {
        VariantClear(&pCustData->data);

        pCustDataNext = pCustData->next;
        TLB_Free(pCustData);
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

/**********************************************************************
 *
 *  Functions for reading MSFT typelibs (those created by CreateTypeLib2)
 */
static inline unsigned int MSFT_Tell(const TLBContext *pcx)
{
    return pcx->pos;
}

static inline void MSFT_Seek(TLBContext *pcx, long where)
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
static DWORD MSFT_Read(void *buffer,  DWORD count, TLBContext *pcx, long where )
{
    TRACE_(typelib)("pos=0x%08x len=0x%08x 0x%08x 0x%08x 0x%08lx\n",
       pcx->pos, count, pcx->oStart, pcx->length, where);

    MSFT_Seek(pcx, where);
    if (pcx->pos + count > pcx->length) count = pcx->length - pcx->pos;
    memcpy( buffer, (char *)pcx->mapping + pcx->pos, count );
    pcx->pos += count;
    return count;
}

static DWORD MSFT_ReadLEDWords(void *buffer,  DWORD count, TLBContext *pcx,
			       long where )
{
  DWORD ret;

  ret = MSFT_Read(buffer, count, pcx, where);
  FromLEDWords(buffer, ret);

  return ret;
}

static DWORD MSFT_ReadLEWords(void *buffer,  DWORD count, TLBContext *pcx,
			      long where )
{
  DWORD ret;

  ret = MSFT_Read(buffer, count, pcx, where);
  FromLEWords(buffer, ret);

  return ret;
}

static void MSFT_ReadGuid( GUID *pGuid, int offset, TLBContext *pcx)
{
    if(offset<0 || pcx->pTblDir->pGuidTab.offset <0){
        memset(pGuid,0, sizeof(GUID));
        return;
    }
    MSFT_Read(pGuid, sizeof(GUID), pcx, pcx->pTblDir->pGuidTab.offset+offset );
    pGuid->Data1 = FromLEDWord(pGuid->Data1);
    pGuid->Data2 = FromLEWord(pGuid->Data2);
    pGuid->Data3 = FromLEWord(pGuid->Data3);
    TRACE_(typelib)("%s\n", debugstr_guid(pGuid));
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

static BSTR MSFT_ReadName( TLBContext *pcx, int offset)
{
    char * name;
    MSFT_NameIntro niName;
    int lengthInChars;
    BSTR bstrName = NULL;

    if (offset < 0)
    {
        ERR_(typelib)("bad offset %d\n", offset);
        return NULL;
    }
    MSFT_ReadLEDWords(&niName, sizeof(niName), pcx,
		      pcx->pTblDir->pNametab.offset+offset);
    niName.namelen &= 0xFF; /* FIXME: correct ? */
    name=TLB_Alloc((niName.namelen & 0xff) +1);
    MSFT_Read(name, (niName.namelen & 0xff), pcx, DO_NOT_SEEK);
    name[niName.namelen & 0xff]='\0';

    lengthInChars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                        name, -1, NULL, 0);

    /* no invalid characters in string */
    if (lengthInChars)
    {
        bstrName = SysAllocStringByteLen(NULL, lengthInChars * sizeof(WCHAR));

        /* don't check for invalid character since this has been done previously */
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, -1, bstrName, lengthInChars);
    }
    TLB_Free(name);

    TRACE_(typelib)("%s %d\n", debugstr_w(bstrName), lengthInChars);
    return bstrName;
}

static BSTR MSFT_ReadString( TLBContext *pcx, int offset)
{
    char * string;
    INT16 length;
    int lengthInChars;
    BSTR bstr = NULL;

    if(offset<0) return NULL;
    MSFT_ReadLEWords(&length, sizeof(INT16), pcx, pcx->pTblDir->pStringtab.offset+offset);
    if(length <= 0) return 0;
    string=TLB_Alloc(length +1);
    MSFT_Read(string, length, pcx, DO_NOT_SEEK);
    string[length]='\0';

    lengthInChars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                        string, -1, NULL, 0);

    /* no invalid characters in string */
    if (lengthInChars)
    {
        bstr = SysAllocStringByteLen(NULL, lengthInChars * sizeof(WCHAR));

        /* don't check for invalid character since this has been done previously */
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, string, -1, bstr, lengthInChars);
    }
    TLB_Free(string);

    TRACE_(typelib)("%s %d\n", debugstr_w(bstr), lengthInChars);
    return bstr;
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
	    if(size < 0) {
                char next;
                DWORD origPos = MSFT_Tell(pcx), nullPos;

                do {
                    MSFT_Read(&next, 1, pcx, DO_NOT_SEEK);
                } while (next);
                nullPos = MSFT_Tell(pcx);
                size = nullPos - origPos;
                MSFT_Seek(pcx, origPos);
	    }
            ptr=TLB_Alloc(size);/* allocate temp buffer */
            MSFT_Read(ptr, size, pcx, DO_NOT_SEEK);/* read string (ANSI) */
            V_BSTR(pVar)=SysAllocStringLen(NULL,size);
            /* FIXME: do we need a AtoW conversion here? */
            V_UNION(pVar, bstrVal[size])='\0';
            while(size--) V_UNION(pVar, bstrVal[size])=ptr[size];
            TLB_Free(ptr);
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
static int MSFT_CustData( TLBContext *pcx, int offset, TLBCustData** ppCustData )
{
    MSFT_CDGuid entry;
    TLBCustData* pNew;
    int count=0;

    TRACE_(typelib)("\n");

    while(offset >=0){
        count++;
        pNew=TLB_Alloc(sizeof(TLBCustData));
        MSFT_ReadLEDWords(&entry, sizeof(entry), pcx, pcx->pTblDir->pCDGuids.offset+offset);
        MSFT_ReadGuid(&(pNew->guid), entry.GuidOffset , pcx);
        MSFT_ReadValue(&(pNew->data), entry.DataOffset, pcx);
        /* add new custom data at head of the list */
        pNew->next=*ppCustData;
        *ppCustData=pNew;
        offset = entry.next;
    }
    return count;
}

static void MSFT_GetTdesc(TLBContext *pcx, INT type, TYPEDESC *pTd,
			  ITypeInfoImpl *pTI)
{
    if(type <0)
        pTd->vt=type & VT_TYPEMASK;
    else
        *pTd=pcx->pLibInfo->pTypeDesc[type/(2*sizeof(INT))];

    if(pTd->vt == VT_USERDEFINED)
      MSFT_DoRefType(pcx, pTI->pTypeLib, pTd->u.hreftype);

    TRACE_(typelib)("vt type = %X\n", pTd->vt);
}

static void MSFT_ResolveReferencedTypes(TLBContext *pcx, ITypeInfoImpl *pTI, TYPEDESC *lpTypeDesc)
{
    /* resolve referenced type if any */
    while (lpTypeDesc)
    {
        switch (lpTypeDesc->vt)
        {
        case VT_PTR:
            lpTypeDesc = lpTypeDesc->u.lptdesc;
            break;

        case VT_CARRAY:
            lpTypeDesc = & (lpTypeDesc->u.lpadesc->tdescElem);
            break;

        case VT_USERDEFINED:
            MSFT_DoRefType(pcx, pTI->pTypeLib,
                           lpTypeDesc->u.hreftype);

            lpTypeDesc = NULL;
            break;

        default:
            lpTypeDesc = NULL;
        }
    }
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

    int infolen, nameoffset, reclength, nrattributes, i;
    int recoffset = offset + sizeof(INT);

    char *recbuf = HeapAlloc(GetProcessHeap(), 0, 0xffff);
    MSFT_FuncRecord * pFuncRec=(MSFT_FuncRecord *) recbuf;
    TLBFuncDesc *ptfd_prev = NULL;

    TRACE_(typelib)("\n");

    MSFT_ReadLEDWords(&infolen, sizeof(INT), pcx, offset);

    for ( i = 0; i < cFuncs ; i++ )
    {
        *pptfd = TLB_Alloc(sizeof(TLBFuncDesc));

        /* name, eventually add to a hash table */
        MSFT_ReadLEDWords(&nameoffset, sizeof(INT), pcx,
                          offset + infolen + (cFuncs + cVars + i + 1) * sizeof(INT));

        /* nameoffset is sometimes -1 on the second half of a propget/propput
         * pair of functions */
        if ((nameoffset == -1) && (i > 0))
            (*pptfd)->Name = SysAllocString(ptfd_prev->Name);
        else
            (*pptfd)->Name = MSFT_ReadName(pcx, nameoffset);

        /* read the function information record */
        MSFT_ReadLEDWords(&reclength, sizeof(INT), pcx, recoffset);

        reclength &= 0xffff;

        MSFT_ReadLEDWords(pFuncRec, reclength - sizeof(INT), pcx, DO_NOT_SEEK);

        /* do the attributes */
        nrattributes = (reclength - pFuncRec->nrargs * 3 * sizeof(int) - 0x18)
                       / sizeof(int);

        if ( nrattributes > 0 )
        {
            (*pptfd)->helpcontext = pFuncRec->OptAttr[0] ;

            if ( nrattributes > 1 )
            {
                (*pptfd)->HelpString = MSFT_ReadString(pcx,
                                                      pFuncRec->OptAttr[1]) ;

                if ( nrattributes > 2 )
                {
                    if ( pFuncRec->FKCCIC & 0x2000 )
                    {
                       if (HIWORD(pFuncRec->OptAttr[2]) != 0)
                           ERR("ordinal 0x%08x invalid, HIWORD != 0\n", pFuncRec->OptAttr[2]);
                       (*pptfd)->Entry = (BSTR)pFuncRec->OptAttr[2];
                    }
                    else
                    {
                        (*pptfd)->Entry = MSFT_ReadString(pcx,
                                                         pFuncRec->OptAttr[2]);
                    }
                    if( nrattributes > 5 )
                    {
                        (*pptfd)->HelpStringContext = pFuncRec->OptAttr[5] ;

                        if ( nrattributes > 6 && pFuncRec->FKCCIC & 0x80 )
                        {
                            MSFT_CustData(pcx,
					  pFuncRec->OptAttr[6],
					  &(*pptfd)->pCustData);
                        }
                    }
                }
                else
                {
                    (*pptfd)->Entry = (BSTR)-1;
                }
            }
        }

        /* fill the FuncDesc Structure */
        MSFT_ReadLEDWords( & (*pptfd)->funcdesc.memid, sizeof(INT), pcx,
                           offset + infolen + ( i + 1) * sizeof(INT));

        (*pptfd)->funcdesc.funckind   =  (pFuncRec->FKCCIC)      & 0x7;
        (*pptfd)->funcdesc.invkind    =  (pFuncRec->FKCCIC) >> 3 & 0xF;
        (*pptfd)->funcdesc.callconv   =  (pFuncRec->FKCCIC) >> 8 & 0xF;
        (*pptfd)->funcdesc.cParams    =   pFuncRec->nrargs  ;
        (*pptfd)->funcdesc.cParamsOpt =   pFuncRec->nroargs ;
        (*pptfd)->funcdesc.oVft       =   (pFuncRec->VtableOffset * sizeof(void *))/4;
        (*pptfd)->funcdesc.wFuncFlags =   LOWORD(pFuncRec->Flags) ;

        MSFT_GetTdesc(pcx,
		      pFuncRec->DataType,
		      &(*pptfd)->funcdesc.elemdescFunc.tdesc,
		      pTI);
        MSFT_ResolveReferencedTypes(pcx, pTI, &(*pptfd)->funcdesc.elemdescFunc.tdesc);

        /* do the parameters/arguments */
        if(pFuncRec->nrargs)
        {
            int j = 0;
            MSFT_ParameterInfo paraminfo;

            (*pptfd)->funcdesc.lprgelemdescParam =
                TLB_Alloc(pFuncRec->nrargs * sizeof(ELEMDESC));

            (*pptfd)->pParamDesc =
                TLB_Alloc(pFuncRec->nrargs * sizeof(TLBParDesc));

            MSFT_ReadLEDWords(&paraminfo, sizeof(paraminfo), pcx,
                              recoffset + reclength - pFuncRec->nrargs * sizeof(MSFT_ParameterInfo));

            for ( j = 0 ; j < pFuncRec->nrargs ; j++ )
            {
                ELEMDESC *elemdesc = &(*pptfd)->funcdesc.lprgelemdescParam[j];

                MSFT_GetTdesc(pcx,
			      paraminfo.DataType,
			      &elemdesc->tdesc,
			      pTI);

                elemdesc->u.paramdesc.wParamFlags = paraminfo.Flags;

                /* name */
                if (paraminfo.oName == -1)
                    /* this occurs for [propput] or [propget] methods, so
                     * we should just set the name of the parameter to the
                     * name of the method. */
                    (*pptfd)->pParamDesc[j].Name = SysAllocString((*pptfd)->Name);
                else
                    (*pptfd)->pParamDesc[j].Name =
                        MSFT_ReadName( pcx, paraminfo.oName );
                TRACE_(typelib)("param[%d] = %s\n", j, debugstr_w((*pptfd)->pParamDesc[j].Name));

                MSFT_ResolveReferencedTypes(pcx, pTI, &elemdesc->tdesc);

                /* default value */
                if ( (elemdesc->u.paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT) &&
                     (pFuncRec->FKCCIC & 0x1000) )
                {
                    INT* pInt = (INT *)((char *)pFuncRec +
                                   reclength -
                                   (pFuncRec->nrargs * 4 + 1) * sizeof(INT) );

                    PARAMDESC* pParamDesc = &elemdesc->u.paramdesc;

                    pParamDesc->pparamdescex = TLB_Alloc(sizeof(PARAMDESCEX));
                    pParamDesc->pparamdescex->cBytes = sizeof(PARAMDESCEX);

		    MSFT_ReadValue(&(pParamDesc->pparamdescex->varDefaultValue),
                        pInt[j], pcx);
                }
                else
                    elemdesc->u.paramdesc.pparamdescex = NULL;
                /* custom info */
                if ( nrattributes > 7 + j && pFuncRec->FKCCIC & 0x80 )
                {
                    MSFT_CustData(pcx,
				  pFuncRec->OptAttr[7+j],
				  &(*pptfd)->pParamDesc[j].pCustData);
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
        (*pptfd)->funcdesc.cScodes   = 0 ;
        (*pptfd)->funcdesc.lprgscode = NULL ;

        ptfd_prev = *pptfd;
        pptfd      = & ((*pptfd)->next);
        recoffset += reclength;
    }
    HeapFree(GetProcessHeap(), 0, recbuf);
}

static void MSFT_DoVars(TLBContext *pcx, ITypeInfoImpl *pTI, int cFuncs,
		       int cVars, int offset, TLBVarDesc ** pptvd)
{
    int infolen, nameoffset, reclength;
    char recbuf[256];
    MSFT_VarRecord * pVarRec=(MSFT_VarRecord *) recbuf;
    int i;
    int recoffset;

    TRACE_(typelib)("\n");

    MSFT_ReadLEDWords(&infolen,sizeof(INT), pcx, offset);
    MSFT_ReadLEDWords(&recoffset,sizeof(INT), pcx, offset + infolen +
                      ((cFuncs+cVars)*2+cFuncs + 1)*sizeof(INT));
    recoffset += offset+sizeof(INT);
    for(i=0;i<cVars;i++){
        *pptvd=TLB_Alloc(sizeof(TLBVarDesc));
    /* name, eventually add to a hash table */
        MSFT_ReadLEDWords(&nameoffset, sizeof(INT), pcx,
                          offset + infolen + (2*cFuncs + cVars + i + 1) * sizeof(INT));
        (*pptvd)->Name=MSFT_ReadName(pcx, nameoffset);
    /* read the variable information record */
        MSFT_ReadLEDWords(&reclength, sizeof(INT), pcx, recoffset);
        reclength &=0xff;
        MSFT_ReadLEDWords(pVarRec, reclength - sizeof(INT), pcx, DO_NOT_SEEK);
    /* Optional data */
        if(reclength >(6*sizeof(INT)) )
            (*pptvd)->HelpContext=pVarRec->HelpContext;
        if(reclength >(7*sizeof(INT)) )
            (*pptvd)->HelpString = MSFT_ReadString(pcx, pVarRec->oHelpString) ;
        if(reclength >(8*sizeof(INT)) )
        if(reclength >(9*sizeof(INT)) )
            (*pptvd)->HelpStringContext=pVarRec->HelpStringContext;
    /* fill the VarDesc Structure */
        MSFT_ReadLEDWords(&(*pptvd)->vardesc.memid, sizeof(INT), pcx,
                          offset + infolen + (cFuncs + i + 1) * sizeof(INT));
        (*pptvd)->vardesc.varkind = pVarRec->VarKind;
        (*pptvd)->vardesc.wVarFlags = pVarRec->Flags;
        MSFT_GetTdesc(pcx, pVarRec->DataType,
            &(*pptvd)->vardesc.elemdescVar.tdesc, pTI);
/*   (*pptvd)->vardesc.lpstrSchema; is reserved (SDK) FIXME?? */
        if(pVarRec->VarKind == VAR_CONST ){
            (*pptvd)->vardesc.u.lpvarValue=TLB_Alloc(sizeof(VARIANT));
            MSFT_ReadValue((*pptvd)->vardesc.u.lpvarValue,
                pVarRec->OffsValue, pcx);
        } else
            (*pptvd)->vardesc.u.oInst=pVarRec->OffsValue;
        MSFT_ResolveReferencedTypes(pcx, pTI, &(*pptvd)->vardesc.elemdescVar.tdesc);
        pptvd=&((*pptvd)->next);
        recoffset += reclength;
    }
}
/* fill in data for a hreftype (offset). When the referenced type is contained
 * in the typelib, it's just an (file) offset in the type info base dir.
 * If comes from import, it's an offset+1 in the ImpInfo table
 * */
static void MSFT_DoRefType(TLBContext *pcx, ITypeLibImpl *pTL,
                          int offset)
{
    TLBRefType *ref;

    TRACE_(typelib)("TLB context %p, TLB offset %x\n", pcx, offset);

    LIST_FOR_EACH_ENTRY(ref, &pTL->ref_list, TLBRefType, entry)
    {
        if(ref->reference == offset) return;
    }

    ref = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ref));
    list_add_tail(&pTL->ref_list, &ref->entry);

    if(!MSFT_HREFTYPE_INTHISFILE( offset)) {
        /* external typelib */
        MSFT_ImpInfo impinfo;
        TLBImpLib *pImpLib=(pcx->pLibInfo->pImpLibs);

        TRACE_(typelib)("offset %x, masked offset %x\n", offset, offset + (offset & 0xfffffffc));

        MSFT_ReadLEDWords(&impinfo, sizeof(impinfo), pcx,
                          pcx->pTblDir->pImpInfo.offset + (offset & 0xfffffffc));
        while (pImpLib){   /* search the known offsets of all import libraries */
            if(pImpLib->offset==impinfo.oImpFile) break;
            pImpLib=pImpLib->next;
        }
        if(pImpLib){
            ref->reference = offset;
            ref->pImpTLInfo = pImpLib;
            if(impinfo.flags & MSFT_IMPINFO_OFFSET_IS_GUID) {
                MSFT_ReadGuid(&ref->guid, impinfo.oGuid, pcx);
                TRACE("importing by guid %s\n", debugstr_guid(&ref->guid));
                ref->index = TLB_REF_USE_GUID;
            } else
                ref->index = impinfo.oGuid;
        }else{
            ERR("Cannot find a reference\n");
            ref->reference = -1;
            ref->pImpTLInfo = TLB_REF_NOT_FOUND;
        }
    }else{
        /* in this typelib */
        ref->index = MSFT_HREFTYPE_INDEX(offset);
        ref->reference = offset;
        ref->pImpTLInfo = TLB_REF_INTERNAL;
    }
}

/* process Implemented Interfaces of a com class */
static void MSFT_DoImplTypes(TLBContext *pcx, ITypeInfoImpl *pTI, int count,
			    int offset)
{
    int i;
    MSFT_RefRecord refrec;
    TLBImplType **ppImpl = &pTI->impltypelist;

    TRACE_(typelib)("\n");

    for(i=0;i<count;i++){
        if(offset<0) break; /* paranoia */
        *ppImpl=TLB_Alloc(sizeof(**ppImpl));
        MSFT_ReadLEDWords(&refrec,sizeof(refrec),pcx,offset+pcx->pTblDir->pRefTab.offset);
        MSFT_DoRefType(pcx, pTI->pTypeLib, refrec.reftype);
	(*ppImpl)->hRef = refrec.reftype;
	(*ppImpl)->implflags=refrec.flags;
        (*ppImpl)->ctCustData=
            MSFT_CustData(pcx, refrec.oCustData, &(*ppImpl)->pCustData);
        offset=refrec.onext;
        ppImpl=&((*ppImpl)->next);
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

    ptiRet = (ITypeInfoImpl*) ITypeInfo_Constructor();
    MSFT_ReadLEDWords(&tiBase, sizeof(tiBase) ,pcx ,
                      pcx->pTblDir->pTypeInfoTab.offset+count*sizeof(tiBase));

/* this is where we are coming from */
    ptiRet->pTypeLib = pLibInfo;
    ptiRet->index=count;
/* fill in the typeattr fields */

    MSFT_ReadGuid(&ptiRet->TypeAttr.guid, tiBase.posguid, pcx);
    ptiRet->TypeAttr.lcid=pLibInfo->LibAttr.lcid;   /* FIXME: correct? */
    ptiRet->TypeAttr.lpstrSchema=NULL;              /* reserved */
    ptiRet->TypeAttr.cbSizeInstance=tiBase.size;
    ptiRet->TypeAttr.typekind=tiBase.typekind & 0xF;
    ptiRet->TypeAttr.cFuncs=LOWORD(tiBase.cElement);
    ptiRet->TypeAttr.cVars=HIWORD(tiBase.cElement);
    ptiRet->TypeAttr.cbAlignment=(tiBase.typekind >> 11 )& 0x1F; /* there are more flags there */
    ptiRet->TypeAttr.wTypeFlags=tiBase.flags;
    ptiRet->TypeAttr.wMajorVerNum=LOWORD(tiBase.version);
    ptiRet->TypeAttr.wMinorVerNum=HIWORD(tiBase.version);
    ptiRet->TypeAttr.cImplTypes=tiBase.cImplTypes;
    ptiRet->TypeAttr.cbSizeVft=(tiBase.cbSizeVft * sizeof(void *))/4; /* FIXME: this is only the non inherited part */
    if(ptiRet->TypeAttr.typekind == TKIND_ALIAS)
        MSFT_GetTdesc(pcx, tiBase.datatype1,
            &ptiRet->TypeAttr.tdescAlias, ptiRet);

/*  FIXME: */
/*    IDLDESC  idldescType; *//* never saw this one != zero  */

/* name, eventually add to a hash table */
    ptiRet->Name=MSFT_ReadName(pcx, tiBase.NameOffset);
    ptiRet->hreftype = MSFT_ReadHreftype(pcx, tiBase.NameOffset);
    TRACE_(typelib)("reading %s\n", debugstr_w(ptiRet->Name));
    /* help info */
    ptiRet->DocString=MSFT_ReadString(pcx, tiBase.docstringoffs);
    ptiRet->dwHelpStringContext=tiBase.helpstringcontext;
    ptiRet->dwHelpContext=tiBase.helpcontext;

    if (ptiRet->TypeAttr.typekind == TKIND_MODULE)
        ptiRet->DllName = MSFT_ReadString(pcx, tiBase.datatype1);

/* note: InfoType's Help file and HelpStringDll come from the containing
 * library. Further HelpString and Docstring appear to be the same thing :(
 */
    /* functions */
    if(ptiRet->TypeAttr.cFuncs >0 )
        MSFT_DoFuncs(pcx, ptiRet, ptiRet->TypeAttr.cFuncs,
		    ptiRet->TypeAttr.cVars,
		    tiBase.memoffset, & ptiRet->funclist);
    /* variables */
    if(ptiRet->TypeAttr.cVars >0 )
        MSFT_DoVars(pcx, ptiRet, ptiRet->TypeAttr.cFuncs,
		   ptiRet->TypeAttr.cVars,
		   tiBase.memoffset, & ptiRet->varlist);
    if(ptiRet->TypeAttr.cImplTypes >0 ) {
        switch(ptiRet->TypeAttr.typekind)
        {
        case TKIND_COCLASS:
            MSFT_DoImplTypes(pcx, ptiRet, ptiRet->TypeAttr.cImplTypes ,
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
                ptiRet->impltypelist = TLB_Alloc(sizeof(TLBImplType));
                ptiRet->impltypelist->hRef = tiBase.datatype1;
                MSFT_DoRefType(pcx, pLibInfo, tiBase.datatype1);
            }
          break;
        default:
            ptiRet->impltypelist=TLB_Alloc(sizeof(TLBImplType));
            MSFT_DoRefType(pcx, pLibInfo, tiBase.datatype1);
	    ptiRet->impltypelist->hRef = tiBase.datatype1;
            break;
       }
    }
    ptiRet->ctCustData=
        MSFT_CustData(pcx, tiBase.oCustData, &ptiRet->pCustData);

    TRACE_(typelib)("%s guid: %s kind:%s\n",
       debugstr_w(ptiRet->Name),
       debugstr_guid(&ptiRet->TypeAttr.guid),
       typekind_desc[ptiRet->TypeAttr.typekind]);
    if (TRACE_ON(typelib))
      dump_TypeInfo(ptiRet);

    return ptiRet;
}

/* Because type library parsing has some degree of overhead, and some apps repeatedly load the same
 * typelibs over and over, we cache them here. According to MSDN Microsoft have a similar scheme in
 * place. This will cause a deliberate memory leak, but generally losing RAM for cycles is an acceptable
 * tradeoff here.
 */
static ITypeLibImpl *tlb_cache_first;
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
    const IUnknownVtbl *lpvtbl;
    LONG refs;
    HMODULE dll;
    HRSRC typelib_resource;
    HGLOBAL typelib_global;
    LPVOID typelib_base;
} TLB_PEFile;

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
    TLB_PEFile *This = (TLB_PEFile *)iface;
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI TLB_PEFile_Release(IUnknown *iface)
{
    TLB_PEFile *This = (TLB_PEFile *)iface;
    ULONG refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        if (This->typelib_global)
            FreeResource(This->typelib_global);
        if (This->dll)
            FreeLibrary(This->dll);
        HeapFree(GetProcessHeap(), 0, This);
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

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->lpvtbl = &TLB_PEFile_Vtable;
    This->refs = 1;
    This->dll = NULL;
    This->typelib_resource = NULL;
    This->typelib_global = NULL;
    This->typelib_base = NULL;

    This->dll = LoadLibraryExW(path, 0, DONT_RESOLVE_DLL_REFERENCES |
                    LOAD_LIBRARY_AS_DATAFILE | LOAD_WITH_ALTERED_SEARCH_PATH);

    if (This->dll)
    {
        static const WCHAR TYPELIBW[] = {'T','Y','P','E','L','I','B',0};
        This->typelib_resource = FindResourceW(This->dll, MAKEINTRESOURCEW(index), TYPELIBW);
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
                    *ppFile = (IUnknown *)&This->lpvtbl;
                    return S_OK;
                }
            }
        }
    }

    TLB_PEFile_Release((IUnknown *)&This->lpvtbl);
    return TYPE_E_CANTLOADLIBRARY;
}

typedef struct TLB_NEFile
{
    const IUnknownVtbl *lpvtbl;
    LONG refs;
    LPVOID typelib_base;
} TLB_NEFile;

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
    TLB_NEFile *This = (TLB_NEFile *)iface;
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI TLB_NEFile_Release(IUnknown *iface)
{
    TLB_NEFile *This = (TLB_NEFile *)iface;
    ULONG refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        HeapFree(GetProcessHeap(), 0, This->typelib_base);
        HeapFree(GetProcessHeap(), 0, This);
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
    if ( sizeof(nehd) != LZRead( lzfd, (LPSTR)&nehd, sizeof(nehd) ) ) return 0;

    resTabSize = nehd.ne_restab - nehd.ne_rsrctab;
    if ( !resTabSize )
    {
        TRACE("No resources in NE dll\n" );
        return FALSE;
    }

    /* Read in resource table */
    resTab = HeapAlloc( GetProcessHeap(), 0, resTabSize );
    if ( !resTab ) return FALSE;

    LZSeek( lzfd, nehd.ne_rsrctab + nehdoffset, SEEK_SET );
    if ( resTabSize != LZRead( lzfd, (char*)resTab, resTabSize ) )
    {
        HeapFree( GetProcessHeap(), 0, resTab );
        return FALSE;
    }

    /* Find resource */
    typeInfo = (NE_TYPEINFO *)(resTab + 2);

    if (HIWORD(typeid) != 0)  /* named type */
    {
        BYTE len = strlen( typeid );
        while (typeInfo->type_id)
        {
            if (!(typeInfo->type_id & 0x8000))
            {
                BYTE *p = resTab + typeInfo->type_id;
                if ((*p == len) && !strncasecmp( (char*)p+1, typeid, len )) goto found_type;
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
    HeapFree( GetProcessHeap(), 0, resTab );
    return FALSE;

 found_type:
    nameInfo = (NE_NAMEINFO *)(typeInfo + 1);

    if (HIWORD(resid) != 0)  /* named resource */
    {
        BYTE len = strlen( resid );
        for (count = typeInfo->count; count > 0; count--, nameInfo++)
        {
            BYTE *p = resTab + nameInfo->id;
            if (nameInfo->id & 0x8000) continue;
            if ((*p == len) && !strncasecmp( (char*)p+1, resid, len )) goto found_name;
        }
    }
    else  /* numeric resource id */
    {
        WORD id = LOWORD(resid) | 0x8000;
        for (count = typeInfo->count; count > 0; count--, nameInfo++)
            if (nameInfo->id == id) goto found_name;
    }
    TRACE("No resid entry found for %p\n", typeid );
    HeapFree( GetProcessHeap(), 0, resTab );
    return FALSE;

 found_name:
    /* Return resource data */
    if ( resLen ) *resLen = nameInfo->length << *(WORD *)resTab;
    if ( resOff ) *resOff = nameInfo->offset << *(WORD *)resTab;

    HeapFree( GetProcessHeap(), 0, resTab );
    return TRUE;
}

static HRESULT TLB_NEFile_Open(LPCWSTR path, INT index, LPVOID *ppBase, DWORD *pdwTLBLength, IUnknown **ppFile){

    HFILE lzfd = -1;
    OFSTRUCT ofs;
    HRESULT hr = TYPE_E_CANTLOADLIBRARY;
    TLB_NEFile *This = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->lpvtbl = &TLB_NEFile_Vtable;
    This->refs = 1;
    This->typelib_base = NULL;

    lzfd = LZOpenFileW( (LPWSTR)path, &ofs, OF_READ );
    if ( lzfd >= 0 && read_xx_header( lzfd ) == IMAGE_OS2_SIGNATURE )
    {
        DWORD reslen, offset;
        if( find_ne_resource( lzfd, "TYPELIB", MAKEINTRESOURCEA(index), &reslen, &offset ) )
        {
            This->typelib_base = HeapAlloc(GetProcessHeap(), 0, reslen);
            if( !This->typelib_base )
                hr = E_OUTOFMEMORY;
            else
            {
                LZSeek( lzfd, offset, SEEK_SET );
                reslen = LZRead( lzfd, This->typelib_base, reslen );
                LZClose( lzfd );
                *ppBase = This->typelib_base;
                *pdwTLBLength = reslen;
                *ppFile = (IUnknown *)&This->lpvtbl;
                return S_OK;
            }
        }
    }

    if( lzfd >= 0) LZClose( lzfd );
    TLB_NEFile_Release((IUnknown *)&This->lpvtbl);
    return hr;
}

typedef struct TLB_Mapping
{
    const IUnknownVtbl *lpvtbl;
    LONG refs;
    HANDLE file;
    HANDLE mapping;
    LPVOID typelib_base;
} TLB_Mapping;

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
    TLB_Mapping *This = (TLB_Mapping *)iface;
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI TLB_Mapping_Release(IUnknown *iface)
{
    TLB_Mapping *This = (TLB_Mapping *)iface;
    ULONG refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        if (This->typelib_base)
            UnmapViewOfFile(This->typelib_base);
        if (This->mapping)
            CloseHandle(This->mapping);
        if (This->file != INVALID_HANDLE_VALUE)
            CloseHandle(This->file);
        HeapFree(GetProcessHeap(), 0, This);
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

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->lpvtbl = &TLB_Mapping_Vtable;
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
                *ppFile = (IUnknown *)&This->lpvtbl;
                return S_OK;
            }
        }
    }

    IUnknown_Release((IUnknown *)&This->lpvtbl);
    return TYPE_E_CANTLOADLIBRARY;
}

/****************************************************************************
 *	TLB_ReadTypeLib
 *
 * find the type of the typelib file and map the typelib resource into
 * the memory
 */
#define MSFT_SIGNATURE 0x5446534D /* "MSFT" */
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

    *ppTypeLib = NULL;

    index_str = strrchrW(pszFileName, '\\');
    if(index_str && *++index_str != '\0')
    {
        LPWSTR end_ptr;
        long idx = strtolW(index_str, &end_ptr, 10);
        if(*end_ptr == '\0')
        {
            int str_len = index_str - pszFileName - 1;
            index = idx;
            file = HeapAlloc(GetProcessHeap(), 0, (str_len + 1) * sizeof(WCHAR));
            memcpy(file, pszFileName, str_len * sizeof(WCHAR));
            file[str_len] = 0;
        }
    }

    if(!SearchPathW(NULL, file, NULL, cchPath, pszPath, NULL))
    {
        if(strchrW(file, '\\'))
        {
            lstrcpyW(pszPath, file);
        }
        else
        {
            int len = GetSystemDirectoryW(pszPath, cchPath);
            pszPath[len] = '\\';
            memcpy(pszPath + len + 1, file, (strlenW(file) + 1) * sizeof(WCHAR));
        }
    }

    if(file != pszFileName) HeapFree(GetProcessHeap(), 0, file);

    TRACE_(typelib)("File %s index %d\n", debugstr_w(pszPath), index);

    /* We look the path up in the typelib cache. If found, we just addref it, and return the pointer. */
    EnterCriticalSection(&cache_section);
    for (entry = tlb_cache_first; entry != NULL; entry = entry->next)
    {
        if (!strcmpiW(entry->path, pszPath) && entry->index == index)
        {
            TRACE("cache hit\n");
            *ppTypeLib = (ITypeLib2*)entry;
            ITypeLib_AddRef(*ppTypeLib);
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
                FIXME("Header type magic 0x%08x not supported.\n",dwSignature);
                ret = TYPE_E_CANTLOADLIBRARY;
            }
        }
        else
            ret = TYPE_E_CANTLOADLIBRARY;
        IUnknown_Release(pFile);
    }

    if(*ppTypeLib) {
	ITypeLibImpl *impl = (ITypeLibImpl*)*ppTypeLib;

	TRACE("adding to cache\n");
	impl->path = HeapAlloc(GetProcessHeap(), 0, (strlenW(pszPath)+1) * sizeof(WCHAR));
	lstrcpyW(impl->path, pszPath);
	/* We should really canonicalise the path here. */
        impl->index = index;

        /* FIXME: check if it has added already in the meantime */
        EnterCriticalSection(&cache_section);
        if ((impl->next = tlb_cache_first) != NULL) impl->next->prev = impl;
        impl->prev = NULL;
        tlb_cache_first = impl;
        LeaveCriticalSection(&cache_section);
        ret = S_OK;
    } else
	ERR("Loading of typelib %s failed with error %d\n", debugstr_w(pszFileName), GetLastError());

    return ret;
}

/*================== ITypeLib(2) Methods ===================================*/

static ITypeLibImpl* TypeLibImpl_Constructor(void)
{
    ITypeLibImpl* pTypeLibImpl;

    pTypeLibImpl = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ITypeLibImpl));
    if (!pTypeLibImpl) return NULL;

    pTypeLibImpl->lpVtbl = &tlbvt;
    pTypeLibImpl->lpVtblTypeComp = &tlbtcvt;
    pTypeLibImpl->ref = 1;

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
    long lPSegDir;
    MSFT_Header tlbHeader;
    MSFT_SegDir tlbSegDir;
    ITypeLibImpl * pTypeLibImpl;

    TRACE("%p, TLB length = %d\n", pLib, dwTLBLength);

    pTypeLibImpl = TypeLibImpl_Constructor();
    if (!pTypeLibImpl) return NULL;

    /* get pointer to beginning of typelib data */
    cx.pos = 0;
    cx.oStart=0;
    cx.mapping = pLib;
    cx.pLibInfo = pTypeLibImpl;
    cx.length = dwTLBLength;

    /* read header */
    MSFT_ReadLEDWords((void*)&tlbHeader, sizeof(tlbHeader), &cx, 0);
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
        ERR("cannot find the table directory, ptr=0x%lx\n",lPSegDir);
	HeapFree(GetProcessHeap(),0,pTypeLibImpl);
	return NULL;
    }

    /* now fill our internal data */
    /* TLIBATTR fields */
    MSFT_ReadGuid(&pTypeLibImpl->LibAttr.guid, tlbHeader.posguid, &cx);

    /*    pTypeLibImpl->LibAttr.lcid = tlbHeader.lcid;*/
    /* Windows seems to have zero here, is this correct? */
    if(SUBLANGID(tlbHeader.lcid) == SUBLANG_NEUTRAL)
      pTypeLibImpl->LibAttr.lcid = MAKELCID(MAKELANGID(PRIMARYLANGID(tlbHeader.lcid),0),0);
    else
      pTypeLibImpl->LibAttr.lcid = 0;

    pTypeLibImpl->LibAttr.syskind = tlbHeader.varflags & 0x0f; /* check the mask */
    pTypeLibImpl->LibAttr.wMajorVerNum = LOWORD(tlbHeader.version);
    pTypeLibImpl->LibAttr.wMinorVerNum = HIWORD(tlbHeader.version);
    pTypeLibImpl->LibAttr.wLibFlags = (WORD) tlbHeader.flags & 0xffff;/* check mask */

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
        pTypeLibImpl->ctCustData = MSFT_CustData(&cx, tlbHeader.CustomDataOffset, &pTypeLibImpl->pCustData);
    }

    /* fill in type descriptions */
    if(tlbSegDir.pTypdescTab.length > 0)
    {
        int i, j, cTD = tlbSegDir.pTypdescTab.length / (2*sizeof(INT));
        INT16 td[4];
        pTypeLibImpl->ctTypeDesc = cTD;
        pTypeLibImpl->pTypeDesc = TLB_Alloc( cTD * sizeof(TYPEDESC));
        MSFT_ReadLEWords(td, sizeof(td), &cx, tlbSegDir.pTypdescTab.offset);
        for(i=0; i<cTD; )
	{
            /* FIXME: add several sanity checks here */
            pTypeLibImpl->pTypeDesc[i].vt = td[0] & VT_TYPEMASK;
            if(td[0] == VT_PTR || td[0] == VT_SAFEARRAY)
	    {
	        /* FIXME: check safearray */
                if(td[3] < 0)
                    pTypeLibImpl->pTypeDesc[i].u.lptdesc= & stndTypeDesc[td[2]];
                else
                    pTypeLibImpl->pTypeDesc[i].u.lptdesc= & pTypeLibImpl->pTypeDesc[td[2]/8];
            }
	    else if(td[0] == VT_CARRAY)
            {
	        /* array descr table here */
	        pTypeLibImpl->pTypeDesc[i].u.lpadesc = (void *)((int) td[2]);  /* temp store offset in*/
            }
            else if(td[0] == VT_USERDEFINED)
	    {
                pTypeLibImpl->pTypeDesc[i].u.hreftype = MAKELONG(td[2],td[3]);
            }
	    if(++i<cTD) MSFT_ReadLEWords(td, sizeof(td), &cx, DO_NOT_SEEK);
        }

        /* second time around to fill the array subscript info */
        for(i=0;i<cTD;i++)
	{
            if(pTypeLibImpl->pTypeDesc[i].vt != VT_CARRAY) continue;
            if(tlbSegDir.pArrayDescriptions.offset>0)
	    {
                MSFT_ReadLEWords(td, sizeof(td), &cx, tlbSegDir.pArrayDescriptions.offset + (int) pTypeLibImpl->pTypeDesc[i].u.lpadesc);
                pTypeLibImpl->pTypeDesc[i].u.lpadesc = TLB_Alloc(sizeof(ARRAYDESC)+sizeof(SAFEARRAYBOUND)*(td[3]-1));

                if(td[1]<0)
                    pTypeLibImpl->pTypeDesc[i].u.lpadesc->tdescElem.vt = td[0] & VT_TYPEMASK;
                else
                    pTypeLibImpl->pTypeDesc[i].u.lpadesc->tdescElem = stndTypeDesc[td[0]/8];

                pTypeLibImpl->pTypeDesc[i].u.lpadesc->cDims = td[2];

                for(j = 0; j<td[2]; j++)
		{
                    MSFT_ReadLEDWords(& pTypeLibImpl->pTypeDesc[i].u.lpadesc->rgbounds[j].cElements,
                                      sizeof(INT), &cx, DO_NOT_SEEK);
                    MSFT_ReadLEDWords(& pTypeLibImpl->pTypeDesc[i].u.lpadesc->rgbounds[j].lLbound,
                                      sizeof(INT), &cx, DO_NOT_SEEK);
                }
            }
	    else
	    {
                pTypeLibImpl->pTypeDesc[i].u.lpadesc = NULL;
                ERR("didn't find array description data\n");
            }
        }
    }

    /* imported type libs */
    if(tlbSegDir.pImpFiles.offset>0)
    {
        TLBImpLib **ppImpLib = &(pTypeLibImpl->pImpLibs);
        int oGuid, offset = tlbSegDir.pImpFiles.offset;
        UINT16 size;

        while(offset < tlbSegDir.pImpFiles.offset +tlbSegDir.pImpFiles.length)
	{
            char *name;

            *ppImpLib = TLB_Alloc(sizeof(TLBImpLib));
            (*ppImpLib)->offset = offset - tlbSegDir.pImpFiles.offset;
            MSFT_ReadLEDWords(&oGuid, sizeof(INT), &cx, offset);

            MSFT_ReadLEDWords(&(*ppImpLib)->lcid,         sizeof(LCID),   &cx, DO_NOT_SEEK);
            MSFT_ReadLEWords(&(*ppImpLib)->wVersionMajor, sizeof(WORD),   &cx, DO_NOT_SEEK);
            MSFT_ReadLEWords(&(*ppImpLib)->wVersionMinor, sizeof(WORD),   &cx, DO_NOT_SEEK);
            MSFT_ReadLEWords(& size,                      sizeof(UINT16), &cx, DO_NOT_SEEK);

            size >>= 2;
            name = TLB_Alloc(size+1);
            MSFT_Read(name, size, &cx, DO_NOT_SEEK);
            (*ppImpLib)->name = TLB_MultiByteToBSTR(name);

            MSFT_ReadGuid(&(*ppImpLib)->guid, oGuid, &cx);
            offset = (offset + sizeof(INT) + sizeof(DWORD) + sizeof(LCID) + sizeof(UINT16) + size + 3) & ~3;

            ppImpLib = &(*ppImpLib)->next;
        }
    }

    pTypeLibImpl->dispatch_href = tlbHeader.dispatchpos;
    if(pTypeLibImpl->dispatch_href != -1)
        MSFT_DoRefType(&cx, pTypeLibImpl, pTypeLibImpl->dispatch_href);

    /* type info's */
    if(tlbHeader.nrtypeinfos >= 0 )
    {
        /*pTypeLibImpl->TypeInfoCount=tlbHeader.nrtypeinfos; */
        ITypeInfoImpl **ppTI = &(pTypeLibImpl->pTypeInfo);
        int i;

        for(i = 0; i < tlbHeader.nrtypeinfos; i++)
        {
            *ppTI = MSFT_DoTypeInfo(&cx, i, pTypeLibImpl);

            ppTI = &((*ppTI)->next);
            (pTypeLibImpl->TypeInfoCount)++;
        }
    }

    TRACE("(%p)\n", pTypeLibImpl);
    return (ITypeLib2*) pTypeLibImpl;
}


static BOOL TLB_GUIDFromString(const char *str, GUID *guid)
{
  char b[3];
  int i;
  short s;

  if(sscanf(str, "%x-%hx-%hx-%hx", &guid->Data1, &guid->Data2, &guid->Data3, &s) != 4) {
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

static WORD SLTG_ReadString(const char *ptr, BSTR *pBstr)
{
    WORD bytelen;
    DWORD len;

    *pBstr = NULL;
    bytelen = *(const WORD*)ptr;
    if(bytelen == 0xffff) return 2;
    len = MultiByteToWideChar(CP_ACP, 0, ptr + 2, bytelen, NULL, 0);
    *pBstr = SysAllocStringLen(NULL, len - 1);
    if (*pBstr)
        len = MultiByteToWideChar(CP_ACP, 0, ptr + 2, bytelen, *pBstr, len);
    return bytelen + 2;
}

static WORD SLTG_ReadStringA(const char *ptr, char **str)
{
    WORD bytelen;

    *str = NULL;
    bytelen = *(const WORD*)ptr;
    if(bytelen == 0xffff) return 2;
    *str = HeapAlloc(GetProcessHeap(), 0, bytelen + 1);
    memcpy(*str, ptr + 2, bytelen);
    (*str)[bytelen] = '\0';
    return bytelen + 2;
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
        FIXME("LibBlk.res06 = %04x. Assumung string and skipping\n", w);
        ptr += w;
    }
    ptr += 2;

    ptr += SLTG_ReadString(ptr, &pTypeLibImpl->DocString);

    ptr += SLTG_ReadString(ptr, &pTypeLibImpl->HelpFile);

    pTypeLibImpl->dwHelpContext = *(DWORD*)ptr;
    ptr += 4;

    pTypeLibImpl->LibAttr.syskind = *(WORD*)ptr;
    ptr += 2;

    if(SUBLANGID(*(WORD*)ptr) == SUBLANG_NEUTRAL)
        pTypeLibImpl->LibAttr.lcid = MAKELCID(MAKELANGID(PRIMARYLANGID(*(WORD*)ptr),0),0);
    else
        pTypeLibImpl->LibAttr.lcid = 0;
    ptr += 2;

    ptr += 4; /* skip res12 */

    pTypeLibImpl->LibAttr.wLibFlags = *(WORD*)ptr;
    ptr += 2;

    pTypeLibImpl->LibAttr.wMajorVerNum = *(WORD*)ptr;
    ptr += 2;

    pTypeLibImpl->LibAttr.wMinorVerNum = *(WORD*)ptr;
    ptr += 2;

    memcpy(&pTypeLibImpl->LibAttr.guid, ptr, sizeof(GUID));
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
    if(typeinfo_ref < table->num)
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
	    pTD->u.lptdesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				       sizeof(TYPEDESC));
	    pTD = pTD->u.lptdesc;
	}
	switch(*pType & 0x3f) {
	case VT_PTR:
	    pTD->vt = VT_PTR;
	    pTD->u.lptdesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				       sizeof(TYPEDESC));
	    pTD = pTD->u.lptdesc;
	    break;

	case VT_USERDEFINED:
	    pTD->vt = VT_USERDEFINED;
            sltg_get_typelib_ref(ref_lookup, *(++pType) / 4, &pTD->u.hreftype);
	    done = TRUE;
	    break;

	case VT_CARRAY:
	  {
	    /* *(pType+1) is offset to a SAFEARRAY, *(pType+2) is type of
	       array */

	    SAFEARRAY *pSA = (SAFEARRAY *)(pBlk + *(++pType));

	    pTD->vt = VT_CARRAY;
	    pTD->u.lpadesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			        sizeof(ARRAYDESC) +
				(pSA->cDims - 1) * sizeof(SAFEARRAYBOUND));
	    pTD->u.lpadesc->cDims = pSA->cDims;
	    memcpy(pTD->u.lpadesc->rgbounds, pSA->rgsabound,
		   pSA->cDims * sizeof(SAFEARRAYBOUND));

	    pTD = &pTD->u.lpadesc->tdescElem;
	    break;
	  }

	case VT_SAFEARRAY:
	  {
	    /* FIXME: *(pType+1) gives an offset to SAFEARRAY, is this
	       useful? */

	    pType++;
	    pTD->vt = VT_SAFEARRAY;
	    pTD->u.lptdesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				       sizeof(TYPEDESC));
	    pTD = pTD->u.lptdesc;
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
        pElem->u.paramdesc.wParamFlags = PARAMFLAG_NONE;
    else if(*pType & 0x8000)
        pElem->u.paramdesc.wParamFlags = PARAMFLAG_FIN | PARAMFLAG_FOUT;
    else if(*pType & 0x4000)
        pElem->u.paramdesc.wParamFlags = PARAMFLAG_FOUT;
    else
        pElem->u.paramdesc.wParamFlags = PARAMFLAG_FIN;

    if(*pType & 0x2000)
        pElem->u.paramdesc.wParamFlags |= PARAMFLAG_FLCID;

    if(*pType & 0x80)
        pElem->u.paramdesc.wParamFlags |= PARAMFLAG_FRETVAL;

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

    table = HeapAlloc(GetProcessHeap(), 0, sizeof(*table) + ((pRef->number >> 3) - 1) * sizeof(table->refs[0]));
    table->num = pRef->number >> 3;

    /* FIXME should scan the existing list and reuse matching refs added by previous typeinfos */

    /* We don't want the first href to be 0 */
    typelib_ref = (list_count(&pTL->ref_list) + 1) << 2;

    for(ref = 0; ref < pRef->number >> 3; ref++) {
        char *refname;
	unsigned int lib_offs, type_num;

	ref_type = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ref_type));

	name += SLTG_ReadStringA(name, &refname);
	if(sscanf(refname, "*\\R%x*#%x", &lib_offs, &type_num) != 2)
	    FIXME_(typelib)("Can't sscanf ref\n");
	if(lib_offs != 0xffff) {
	    TLBImpLib **import = &pTL->pImpLibs;

	    while(*import) {
	        if((*import)->offset == lib_offs)
		    break;
		import = &(*import)->next;
	    }
	    if(!*import) {
	        char fname[MAX_PATH+1];
		int len;

		*import = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				    sizeof(**import));
		(*import)->offset = lib_offs;
		TLB_GUIDFromString( pNameTable + lib_offs + 4,
				    &(*import)->guid);
		if(sscanf(pNameTable + lib_offs + 40, "}#%hd.%hd#%x#%s",
			  &(*import)->wVersionMajor,
			  &(*import)->wVersionMinor,
			  &(*import)->lcid, fname) != 4) {
		  FIXME_(typelib)("can't sscanf ref %s\n",
			pNameTable + lib_offs + 40);
		}
		len = strlen(fname);
		if(fname[len-1] != '#')
		    FIXME("fname = %s\n", fname);
		fname[len-1] = '\0';
		(*import)->name = TLB_MultiByteToBSTR(fname);
	    }
	    ref_type->pImpTLInfo = *import;

            /* Store a reference to IDispatch */
            if(pTL->dispatch_href == -1 && IsEqualGUID(&(*import)->guid, &IID_StdOle) && type_num == 4)
                pTL->dispatch_href = typelib_ref;

	} else { /* internal ref */
	  ref_type->pImpTLInfo = TLB_REF_INTERNAL;
	}
	ref_type->reference = typelib_ref;
	ref_type->index = type_num;

	HeapFree(GetProcessHeap(), 0, refname);
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
    TLBImplType **ppImplType = &pTI->impltypelist;
    /* I don't really get this structure, usually it's 0x16 bytes
       long, but iuser.tlb contains some that are 0x18 bytes long.
       That's ok because we can use the next ptr to jump to the next
       one. But how do we know the length of the last one?  The WORD
       at offs 0x8 might be the clue.  For now I'm just assuming that
       the last one is the regular 0x16 bytes. */

    info = (SLTG_ImplInfo*)pBlk;
    while(1) {
	*ppImplType = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				sizeof(**ppImplType));
        sltg_get_typelib_ref(ref_lookup, info->ref, &(*ppImplType)->hRef);
	(*ppImplType)->implflags = info->impltypeflags;
	pTI->TypeAttr.cImplTypes++;
	ppImplType = &(*ppImplType)->next;

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
  TLBVarDesc **ppVarDesc = &pTI->varlist;
  BSTR bstrPrevName = NULL;
  SLTG_Variable *pItem;
  unsigned short i;
  WORD *pType;

  for(pItem = (SLTG_Variable *)pFirstItem, i = 0; i < cVars;
      pItem = (SLTG_Variable *)(pBlk + pItem->next), i++) {

      *ppVarDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			     sizeof(**ppVarDesc));
      (*ppVarDesc)->vardesc.memid = pItem->memid;

      if (pItem->magic != SLTG_VAR_MAGIC &&
          pItem->magic != SLTG_VAR_WITH_FLAGS_MAGIC) {
	  FIXME_(typelib)("var magic = %02x\n", pItem->magic);
	  return;
      }

      if (pItem->name == 0xfffe)
        (*ppVarDesc)->Name = SysAllocString(bstrPrevName);
      else
        (*ppVarDesc)->Name = TLB_MultiByteToBSTR(pItem->name + pNameTable);

      TRACE_(typelib)("name: %s\n", debugstr_w((*ppVarDesc)->Name));
      TRACE_(typelib)("byte_offs = 0x%x\n", pItem->byte_offs);
      TRACE_(typelib)("memid = 0x%x\n", pItem->memid);

      if(pItem->flags & 0x02)
	  pType = &pItem->type;
      else
	  pType = (WORD*)(pBlk + pItem->type);

      if (pItem->flags & ~0xda)
        FIXME_(typelib)("unhandled flags = %02x\n", pItem->flags & ~0xda);

      SLTG_DoElem(pType, pBlk,
		  &(*ppVarDesc)->vardesc.elemdescVar, ref_lookup);

      if (TRACE_ON(typelib)) {
          char buf[300];
          dump_TypeDesc(&(*ppVarDesc)->vardesc.elemdescVar.tdesc, buf);
          TRACE_(typelib)("elemdescVar: %s\n", buf);
      }

      if (pItem->flags & 0x40) {
        TRACE_(typelib)("VAR_DISPATCH\n");
        (*ppVarDesc)->vardesc.varkind = VAR_DISPATCH;
      }
      else if (pItem->flags & 0x10) {
        TRACE_(typelib)("VAR_CONST\n");
        (*ppVarDesc)->vardesc.varkind = VAR_CONST;
        (*ppVarDesc)->vardesc.u.lpvarValue = HeapAlloc(GetProcessHeap(), 0,
						       sizeof(VARIANT));
        V_VT((*ppVarDesc)->vardesc.u.lpvarValue) = VT_INT;
        if (pItem->flags & 0x08)
          V_INT((*ppVarDesc)->vardesc.u.lpvarValue) = pItem->byte_offs;
        else {
          switch ((*ppVarDesc)->vardesc.elemdescVar.tdesc.vt)
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
              V_VT((*ppVarDesc)->vardesc.u.lpvarValue) = VT_BSTR;
              V_BSTR((*ppVarDesc)->vardesc.u.lpvarValue) = str;
              break;
            }
            case VT_I2:
            case VT_UI2:
            case VT_I4:
            case VT_UI4:
            case VT_INT:
            case VT_UINT:
              V_INT((*ppVarDesc)->vardesc.u.lpvarValue) =
                *(INT*)(pBlk + pItem->byte_offs);
              break;
            default:
              FIXME_(typelib)("VAR_CONST unimplemented for type %d\n", (*ppVarDesc)->vardesc.elemdescVar.tdesc.vt);
          }
        }
      }
      else {
        TRACE_(typelib)("VAR_PERINSTANCE\n");
        (*ppVarDesc)->vardesc.u.oInst = pItem->byte_offs;
        (*ppVarDesc)->vardesc.varkind = VAR_PERINSTANCE;
      }

      if (pItem->magic == SLTG_VAR_WITH_FLAGS_MAGIC)
        (*ppVarDesc)->vardesc.wVarFlags = pItem->varflags;

      if (pItem->flags & 0x80)
        (*ppVarDesc)->vardesc.wVarFlags |= VARFLAG_FREADONLY;

      bstrPrevName = (*ppVarDesc)->Name;
      ppVarDesc = &((*ppVarDesc)->next);
  }
  pTI->TypeAttr.cVars = cVars;
}

static void SLTG_DoFuncs(char *pBlk, char *pFirstItem, ITypeInfoImpl *pTI,
			 unsigned short cFuncs, char *pNameTable, const sltg_ref_lookup_t *ref_lookup)
{
    SLTG_Function *pFunc;
    unsigned short i;
    TLBFuncDesc **ppFuncDesc = &pTI->funclist;

    for(pFunc = (SLTG_Function*)pFirstItem, i = 0; i < cFuncs;
	pFunc = (SLTG_Function*)(pBlk + pFunc->next), i++) {

        int param;
	WORD *pType, *pArg;

	*ppFuncDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				sizeof(**ppFuncDesc));

        switch (pFunc->magic & ~SLTG_FUNCTION_FLAGS_PRESENT) {
        case SLTG_FUNCTION_MAGIC:
            (*ppFuncDesc)->funcdesc.funckind = FUNC_PUREVIRTUAL;
            break;
        case SLTG_DISPATCH_FUNCTION_MAGIC:
            (*ppFuncDesc)->funcdesc.funckind = FUNC_DISPATCH;
            break;
        case SLTG_STATIC_FUNCTION_MAGIC:
            (*ppFuncDesc)->funcdesc.funckind = FUNC_STATIC;
            break;
        default:
	    FIXME("unimplemented func magic = %02x\n", pFunc->magic & ~SLTG_FUNCTION_FLAGS_PRESENT);
            HeapFree(GetProcessHeap(), 0, *ppFuncDesc);
            *ppFuncDesc = NULL;
	    return;
	}
	(*ppFuncDesc)->Name = TLB_MultiByteToBSTR(pFunc->name + pNameTable);

	(*ppFuncDesc)->funcdesc.memid = pFunc->dispid;
	(*ppFuncDesc)->funcdesc.invkind = pFunc->inv >> 4;
	(*ppFuncDesc)->funcdesc.callconv = pFunc->nacc & 0x7;
	(*ppFuncDesc)->funcdesc.cParams = pFunc->nacc >> 3;
	(*ppFuncDesc)->funcdesc.cParamsOpt = (pFunc->retnextopt & 0x7e) >> 1;
	(*ppFuncDesc)->funcdesc.oVft = pFunc->vtblpos;

	if(pFunc->magic & SLTG_FUNCTION_FLAGS_PRESENT)
	    (*ppFuncDesc)->funcdesc.wFuncFlags = pFunc->funcflags;

	if(pFunc->retnextopt & 0x80)
	    pType = &pFunc->rettype;
	else
	    pType = (WORD*)(pBlk + pFunc->rettype);

	SLTG_DoElem(pType, pBlk, &(*ppFuncDesc)->funcdesc.elemdescFunc, ref_lookup);

	(*ppFuncDesc)->funcdesc.lprgelemdescParam =
	  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		    (*ppFuncDesc)->funcdesc.cParams * sizeof(ELEMDESC));
	(*ppFuncDesc)->pParamDesc =
	  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		    (*ppFuncDesc)->funcdesc.cParams * sizeof(TLBParDesc));

	pArg = (WORD*)(pBlk + pFunc->arg_off);

	for(param = 0; param < (*ppFuncDesc)->funcdesc.cParams; param++) {
	    char *paramName = pNameTable + *pArg;
	    BOOL HaveOffs;
	    /* If arg type follows then paramName points to the 2nd
	       letter of the name, else the next WORD is an offset to
	       the arg type and paramName points to the first letter.
	       So let's take one char off paramName and see if we're
	       pointing at an alpha-numeric char.  However if *pArg is
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
			    &(*ppFuncDesc)->funcdesc.lprgelemdescParam[param], ref_lookup);
		pArg++;
	    } else {
		if(paramName)
		  paramName--;
		pArg = SLTG_DoElem(pArg, pBlk,
                                   &(*ppFuncDesc)->funcdesc.lprgelemdescParam[param], ref_lookup);
	    }

	    /* Are we an optional param ? */
	    if((*ppFuncDesc)->funcdesc.cParams - param <=
	       (*ppFuncDesc)->funcdesc.cParamsOpt)
	      (*ppFuncDesc)->funcdesc.lprgelemdescParam[param].u.paramdesc.wParamFlags |= PARAMFLAG_FOPT;

	    if(paramName) {
	        (*ppFuncDesc)->pParamDesc[param].Name =
		  TLB_MultiByteToBSTR(paramName);
	    } else {
	        (*ppFuncDesc)->pParamDesc[param].Name =
                  SysAllocString((*ppFuncDesc)->Name);
	    }
	}

	ppFuncDesc = &((*ppFuncDesc)->next);
	if(pFunc->next == 0xffff) break;
    }
    pTI->TypeAttr.cFuncs = cFuncs;
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
    HeapFree(GetProcessHeap(), 0, ref_lookup);
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

    HeapFree(GetProcessHeap(), 0, ref_lookup);

    if (TRACE_ON(typelib))
        dump_TLBFuncDesc(pTI->funclist);
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
    pTI->TypeAttr.tdescAlias.vt = pTITail->tdescalias_vt;
    return;
  }

  if(pTIHeader->href_table != 0xffffffff) {
      ref_lookup = SLTG_DoRefs((SLTG_RefInfo*)((char *)pTIHeader + pTIHeader->href_table), pTI->pTypeLib,
		  pNameTable);
  }

  /* otherwise it is an offset to a type */
  pType = (WORD *)(pBlk + pTITail->tdescalias_vt);

  SLTG_DoType(pType, pBlk, &pTI->TypeAttr.tdescAlias, ref_lookup);

  HeapFree(GetProcessHeap(), 0, ref_lookup);
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

  /* this is necessary to cope with MSFT typelibs that set cFuncs to the number
   * of dispinterface functions including the IDispatch ones, so
   * ITypeInfo::GetFuncDesc takes the real value for cFuncs from cbSizeVft */
  pTI->TypeAttr.cbSizeVft = pTI->TypeAttr.cFuncs * sizeof(void *);

  HeapFree(GetProcessHeap(), 0, ref_lookup);
  if (TRACE_ON(typelib))
      dump_TLBFuncDesc(pTI->funclist);
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
  HeapFree(GetProcessHeap(), 0, ref_lookup);
  if (TRACE_ON(typelib))
    dump_TypeInfo(pTI);
}

/* Because SLTG_OtherTypeInfo is such a painful struct, we make a more
   managable copy of it into this */
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

    TRACE_(typelib)("%p, TLB length = %d\n", pLib, dwTLBLength);


    pTypeLibImpl = TypeLibImpl_Constructor();
    if (!pTypeLibImpl) return NULL;

    pHeader = pLib;

    TRACE_(typelib)("header:\n");
    TRACE_(typelib)("\tmagic=0x%08x, file blocks = %d\n", pHeader->SLTG_magic,
	  pHeader->nrOfFileBlks );
    if (pHeader->SLTG_magic != SLTG_SIGNATURE) {
	FIXME_(typelib)("Header type magic 0x%08x not supported.\n",
	      pHeader->SLTG_magic);
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

    for(pBlk = pFirstBlk, order = pHeader->first_blk - 1, i = 0;
	  pBlkEntry[order].next != 0;
	  order = pBlkEntry[order].next - 1, i++) {
       pBlk = (char*)pBlk + pBlkEntry[order].len;
    }
    pLibBlk = pBlk;

    len = SLTG_ReadLibBlk(pLibBlk, pTypeLibImpl);

    /* Now there's 0x40 bytes of 0xffff with the numbers 0 to TypeInfoCount
       interspersed */

    len += 0x40;

    /* And now TypeInfoCount of SLTG_OtherTypeInfo */

    pOtherTypeInfoBlks = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				   sizeof(*pOtherTypeInfoBlks) *
				   pTypeLibImpl->TypeInfoCount);


    ptr = (char*)pLibBlk + len;

    for(i = 0; i < pTypeLibImpl->TypeInfoCount; i++) {
	WORD w, extra;
	len = 0;

	pOtherTypeInfoBlks[i].small_no = *(WORD*)ptr;

	w = *(WORD*)(ptr + 2);
	if(w != 0xffff) {
	    len += w;
	    pOtherTypeInfoBlks[i].index_name = HeapAlloc(GetProcessHeap(),0,
							 w+1);
	    memcpy(pOtherTypeInfoBlks[i].index_name, ptr + 4, w);
	    pOtherTypeInfoBlks[i].index_name[w] = '\0';
	}
	w = *(WORD*)(ptr + 4 + len);
	if(w != 0xffff) {
	    TRACE_(typelib)("\twith %s\n", debugstr_an(ptr + 6 + len, w));
	    len += w;
	    pOtherTypeInfoBlks[i].other_name = HeapAlloc(GetProcessHeap(),0,
							 w+1);
	    memcpy(pOtherTypeInfoBlks[i].other_name, ptr + 6 + len, w);
	    pOtherTypeInfoBlks[i].other_name[w] = '\0';
	}
	pOtherTypeInfoBlks[i].res1a = *(WORD*)(ptr + len + 6);
	pOtherTypeInfoBlks[i].name_offs = *(WORD*)(ptr + len + 8);
	extra = pOtherTypeInfoBlks[i].more_bytes = *(WORD*)(ptr + 10 + len);
	if(extra) {
	    pOtherTypeInfoBlks[i].extra = HeapAlloc(GetProcessHeap(),0,
						    extra);
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

    pTypeLibImpl->Name = TLB_MultiByteToBSTR(pNameTable + pLibBlk->name);


    /* Hopefully we now have enough ptrs set up to actually read in
       some TypeInfos.  It's not clear which order to do them in, so
       I'll just follow the links along the BlkEntry chain and read
       them in the order in which they are in the file */

    ppTypeInfoImpl = &(pTypeLibImpl->pTypeInfo);

    for(pBlk = pFirstBlk, order = pHeader->first_blk - 1, i = 0;
	pBlkEntry[order].next != 0;
	order = pBlkEntry[order].next - 1, i++) {

      SLTG_TypeInfoHeader *pTIHeader;
      SLTG_TypeInfoTail *pTITail;
      SLTG_MemberHeader *pMemHeader;

      if(strcmp(pBlkEntry[order].index_string + (char*)pMagic,
		pOtherTypeInfoBlks[i].index_name)) {
	FIXME_(typelib)("Index strings don't match\n");
	return NULL;
      }

      pTIHeader = pBlk;
      if(pTIHeader->magic != SLTG_TIHEADER_MAGIC) {
	FIXME_(typelib)("TypeInfoHeader magic = %04x\n", pTIHeader->magic);
	return NULL;
      }
      TRACE_(typelib)("pTIHeader->res06 = %x, pTIHeader->res0e = %x, "
        "pTIHeader->res16 = %x, pTIHeader->res1e = %x\n",
        pTIHeader->res06, pTIHeader->res0e, pTIHeader->res16, pTIHeader->res1e);

      *ppTypeInfoImpl = (ITypeInfoImpl*)ITypeInfo_Constructor();
      (*ppTypeInfoImpl)->pTypeLib = pTypeLibImpl;
      (*ppTypeInfoImpl)->index = i;
      (*ppTypeInfoImpl)->Name = TLB_MultiByteToBSTR(
					     pOtherTypeInfoBlks[i].name_offs +
					     pNameTable);
      (*ppTypeInfoImpl)->dwHelpContext = pOtherTypeInfoBlks[i].helpcontext;
      (*ppTypeInfoImpl)->TypeAttr.guid = pOtherTypeInfoBlks[i].uuid;
      (*ppTypeInfoImpl)->TypeAttr.typekind = pTIHeader->typekind;
      (*ppTypeInfoImpl)->TypeAttr.wMajorVerNum = pTIHeader->major_version;
      (*ppTypeInfoImpl)->TypeAttr.wMinorVerNum = pTIHeader->minor_version;
      (*ppTypeInfoImpl)->TypeAttr.wTypeFlags =
	(pTIHeader->typeflags1 >> 3) | (pTIHeader->typeflags2 << 5);

      if((pTIHeader->typeflags1 & 7) != 2)
	FIXME_(typelib)("typeflags1 = %02x\n", pTIHeader->typeflags1);
      if(pTIHeader->typeflags3 != 2)
	FIXME_(typelib)("typeflags3 = %02x\n", pTIHeader->typeflags3);

      TRACE_(typelib)("TypeInfo %s of kind %s guid %s typeflags %04x\n",
	    debugstr_w((*ppTypeInfoImpl)->Name),
	    typekind_desc[pTIHeader->typekind],
	    debugstr_guid(&(*ppTypeInfoImpl)->TypeAttr.guid),
	    (*ppTypeInfoImpl)->TypeAttr.wTypeFlags);

      pMemHeader = (SLTG_MemberHeader*)((char *)pBlk + pTIHeader->elem_table);

      pTITail = (SLTG_TypeInfoTail*)((char *)(pMemHeader + 1) + pMemHeader->cbExtra);

      (*ppTypeInfoImpl)->TypeAttr.cbAlignment = pTITail->cbAlignment;
      (*ppTypeInfoImpl)->TypeAttr.cbSizeInstance = pTITail->cbSizeInstance;
      (*ppTypeInfoImpl)->TypeAttr.cbSizeVft = (pTITail->cbSizeVft * sizeof(void *))/4;

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
      ppTypeInfoImpl = &((*ppTypeInfoImpl)->next);
      pBlk = (char*)pBlk + pBlkEntry[order].len;
    }

    if(i != pTypeLibImpl->TypeInfoCount) {
      FIXME("Somehow processed %d TypeInfos\n", i);
      return NULL;
    }

    HeapFree(GetProcessHeap(), 0, pOtherTypeInfoBlks);
    return (ITypeLib2*)pTypeLibImpl;
}

/* ITypeLib::QueryInterface
 */
static HRESULT WINAPI ITypeLib2_fnQueryInterface(
	ITypeLib2 * iface,
	REFIID riid,
	VOID **ppvObject)
{
    ITypeLibImpl *This = (ITypeLibImpl *)iface;

    TRACE("(%p)->(IID: %s)\n",This,debugstr_guid(riid));

    *ppvObject=NULL;
    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid,&IID_ITypeLib)||
       IsEqualIID(riid,&IID_ITypeLib2))
    {
        *ppvObject = This;
    }

    if(*ppvObject)
    {
        ITypeLib2_AddRef(iface);
        TRACE("-- Interface: (%p)->(%p)\n",ppvObject,*ppvObject);
        return S_OK;
    }
    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/* ITypeLib::AddRef
 */
static ULONG WINAPI ITypeLib2_fnAddRef( ITypeLib2 *iface)
{
    ITypeLibImpl *This = (ITypeLibImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->ref was %u\n",This, ref - 1);

    return ref;
}

/* ITypeLib::Release
 */
static ULONG WINAPI ITypeLib2_fnRelease( ITypeLib2 *iface)
{
    ITypeLibImpl *This = (ITypeLibImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%u)\n",This, ref);

    if (!ref)
    {
      TLBImpLib *pImpLib, *pImpLibNext;
      TLBCustData *pCustData, *pCustDataNext;
      TLBRefType *ref_type;
      void *cursor2;
      int i;

      /* remove cache entry */
      if(This->path)
      {
          TRACE("removing from cache list\n");
          EnterCriticalSection(&cache_section);
          if (This->next) This->next->prev = This->prev;
          if (This->prev) This->prev->next = This->next;
          else tlb_cache_first = This->next;
          LeaveCriticalSection(&cache_section);
          HeapFree(GetProcessHeap(), 0, This->path);
      }
      TRACE(" destroying ITypeLib(%p)\n",This);

      SysFreeString(This->Name);
      This->Name = NULL;

      SysFreeString(This->DocString);
      This->DocString = NULL;

      SysFreeString(This->HelpFile);
      This->HelpFile = NULL;

      SysFreeString(This->HelpStringDll);
      This->HelpStringDll = NULL;

      for (pCustData = This->pCustData; pCustData; pCustData = pCustDataNext)
      {
          VariantClear(&pCustData->data);

          pCustDataNext = pCustData->next;
          TLB_Free(pCustData);
      }

      for (i = 0; i < This->ctTypeDesc; i++)
          if (This->pTypeDesc[i].vt == VT_CARRAY)
              TLB_Free(This->pTypeDesc[i].u.lpadesc);

      TLB_Free(This->pTypeDesc);

      for (pImpLib = This->pImpLibs; pImpLib; pImpLib = pImpLibNext)
      {
          if (pImpLib->pImpTypeLib)
              ITypeLib_Release((ITypeLib *)pImpLib->pImpTypeLib);
          SysFreeString(pImpLib->name);

          pImpLibNext = pImpLib->next;
          TLB_Free(pImpLib);
      }

      LIST_FOR_EACH_ENTRY_SAFE(ref_type, cursor2, &This->ref_list, TLBRefType, entry)
      {
          list_remove(&ref_type->entry);
          TLB_Free(ref_type);
      }

      if (This->pTypeInfo) /* can be NULL */
      	  ITypeInfo_Release((ITypeInfo*) This->pTypeInfo);
      HeapFree(GetProcessHeap(),0,This);
      return 0;
    }

    return ref;
}

/* ITypeLib::GetTypeInfoCount
 *
 * Returns the number of type descriptions in the type library
 */
static UINT WINAPI ITypeLib2_fnGetTypeInfoCount( ITypeLib2 *iface)
{
    ITypeLibImpl *This = (ITypeLibImpl *)iface;
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
    UINT i;

    ITypeLibImpl *This = (ITypeLibImpl *)iface;
    ITypeInfoImpl *pTypeInfo = This->pTypeInfo;

    TRACE("(%p)->(index=%d)\n", This, index);

    if (!ppTInfo) return E_INVALIDARG;

    /* search element n in list */
    for(i=0; i < index; i++)
    {
      pTypeInfo = pTypeInfo->next;
      if (!pTypeInfo)
      {
        TRACE("-- element not found\n");
        return TYPE_E_ELEMENTNOTFOUND;
      }
    }

    *ppTInfo = (ITypeInfo *) pTypeInfo;

    ITypeInfo_AddRef(*ppTInfo);
    TRACE("-- found (%p)\n",*ppTInfo);
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
    ITypeLibImpl *This = (ITypeLibImpl *)iface;
    UINT i;
    ITypeInfoImpl *pTInfo = This->pTypeInfo;

    if (ITypeLib2_fnGetTypeInfoCount(iface) < index + 1)
    	 return TYPE_E_ELEMENTNOTFOUND;

    TRACE("(%p) index %d\n", This, index);

    if(!pTKind) return E_INVALIDARG;

    /* search element n in list */
    for(i=0; i < index; i++)
    {
      if(!pTInfo)
      {
        TRACE("-- element not found\n");
        return TYPE_E_ELEMENTNOTFOUND;
      }
      pTInfo = pTInfo->next;
    }

    *pTKind = pTInfo->TypeAttr.typekind;
    TRACE("-- found Type (%d)\n", *pTKind);
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
    ITypeLibImpl *This = (ITypeLibImpl *)iface;
    ITypeInfoImpl *pTypeInfo = This->pTypeInfo; /* head of list */

    TRACE("(%p)\n\tguid:\t%s)\n",This,debugstr_guid(guid));

    if (!pTypeInfo)
    {
        WARN("-- element not found\n");
        return TYPE_E_ELEMENTNOTFOUND;
    }

    /* search linked list for guid */
    while( !IsEqualIID(guid,&pTypeInfo->TypeAttr.guid) )
    {
      pTypeInfo = pTypeInfo->next;

      if (!pTypeInfo)
      {
        /* end of list reached */
        WARN("-- element not found\n");
        return TYPE_E_ELEMENTNOTFOUND;
      }
    }

    TRACE("-- found (%p, %s)\n",
          pTypeInfo,
          debugstr_w(pTypeInfo->Name));

    *ppTInfo = (ITypeInfo*)pTypeInfo;
    ITypeInfo_AddRef(*ppTInfo);
    return S_OK;
}

/* ITypeLib::GetLibAttr
 *
 * Retrieves the structure that contains the library's attributes.
 *
 */
static HRESULT WINAPI ITypeLib2_fnGetLibAttr(
	ITypeLib2 *iface,
	LPTLIBATTR *ppTLibAttr)
{
    ITypeLibImpl *This = (ITypeLibImpl *)iface;
    TRACE("(%p)\n",This);
    *ppTLibAttr = HeapAlloc(GetProcessHeap(), 0, sizeof(**ppTLibAttr));
    **ppTLibAttr = This->LibAttr;
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
    ITypeLibImpl *This = (ITypeLibImpl *)iface;

    TRACE("(%p)->(%p)\n",This,ppTComp);
    *ppTComp = (ITypeComp *)&This->lpVtblTypeComp;
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
    ITypeLibImpl *This = (ITypeLibImpl *)iface;

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
                if(!(*pBstrName = SysAllocString(This->Name)))
                    goto memerr1;
            }
            else
                *pBstrName = NULL;
        }
        if(pBstrDocString)
        {
            if (This->DocString)
            {
                if(!(*pBstrDocString = SysAllocString(This->DocString)))
                    goto memerr2;
            }
            else if (This->Name)
            {
                if(!(*pBstrDocString = SysAllocString(This->Name)))
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
                if(!(*pBstrHelpFile = SysAllocString(This->HelpFile)))
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
    ITypeLibImpl *This = (ITypeLibImpl *)iface;
    ITypeInfoImpl *pTInfo;
    TLBFuncDesc *pFInfo;
    TLBVarDesc *pVInfo;
    int i;
    UINT nNameBufLen = (lstrlenW(szNameBuf)+1)*sizeof(WCHAR);

    TRACE("(%p)->(%s,%08x,%p)\n", This, debugstr_w(szNameBuf), lHashVal,
	  pfName);

    *pfName=TRUE;
    for(pTInfo=This->pTypeInfo;pTInfo;pTInfo=pTInfo->next){
        if(!memcmp(szNameBuf,pTInfo->Name, nNameBufLen)) goto ITypeLib2_fnIsName_exit;
        for(pFInfo=pTInfo->funclist;pFInfo;pFInfo=pFInfo->next) {
            if(!memcmp(szNameBuf,pFInfo->Name, nNameBufLen)) goto ITypeLib2_fnIsName_exit;
            for(i=0;i<pFInfo->funcdesc.cParams;i++)
                if(!memcmp(szNameBuf,pFInfo->pParamDesc[i].Name, nNameBufLen))
                    goto ITypeLib2_fnIsName_exit;
        }
        for(pVInfo=pTInfo->varlist;pVInfo;pVInfo=pVInfo->next)
            if(!memcmp(szNameBuf,pVInfo->Name, nNameBufLen)) goto ITypeLib2_fnIsName_exit;

    }
    *pfName=FALSE;

ITypeLib2_fnIsName_exit:
    TRACE("(%p)slow! search for %s: %s found!\n", This,
          debugstr_w(szNameBuf), *pfName?"NOT":"");

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
	LPOLESTR szNameBuf,
	ULONG lHashVal,
	ITypeInfo **ppTInfo,
	MEMBERID *rgMemId,
	UINT16 *pcFound)
{
    ITypeLibImpl *This = (ITypeLibImpl *)iface;
    ITypeInfoImpl *pTInfo;
    TLBFuncDesc *pFInfo;
    TLBVarDesc *pVInfo;
    int i,j = 0;
    UINT nNameBufLen = (lstrlenW(szNameBuf)+1)*sizeof(WCHAR);

    for(pTInfo=This->pTypeInfo;pTInfo && j<*pcFound; pTInfo=pTInfo->next){
        if(!memcmp(szNameBuf,pTInfo->Name, nNameBufLen)) goto ITypeLib2_fnFindName_exit;
        for(pFInfo=pTInfo->funclist;pFInfo;pFInfo=pFInfo->next) {
            if(!memcmp(szNameBuf,pFInfo->Name,nNameBufLen)) goto ITypeLib2_fnFindName_exit;
            for(i=0;i<pFInfo->funcdesc.cParams;i++) {
                if(!memcmp(szNameBuf,pFInfo->pParamDesc[i].Name,nNameBufLen))
                    goto ITypeLib2_fnFindName_exit;
	    }
        }
        for(pVInfo=pTInfo->varlist;pVInfo;pVInfo=pVInfo->next)
            if(!memcmp(szNameBuf,pVInfo->Name, nNameBufLen)) goto ITypeLib2_fnFindName_exit;
        continue;
ITypeLib2_fnFindName_exit:
        ITypeInfo_AddRef((ITypeInfo*)pTInfo);
        ppTInfo[j]=(LPTYPEINFO)pTInfo;
        j++;
    }
    TRACE("(%p)slow! search for %d with %s: found %d TypeInfo's!\n",
          This, *pcFound, debugstr_w(szNameBuf), j);

    *pcFound=j;

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
    ITypeLibImpl *This = (ITypeLibImpl *)iface;
    TRACE("freeing (%p)\n",This);
    HeapFree(GetProcessHeap(),0,pTLibAttr);

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
    ITypeLibImpl *This = (ITypeLibImpl *)iface;
    TLBCustData *pCData;

    for(pCData=This->pCustData; pCData; pCData = pCData->next)
    {
      if( IsEqualIID(guid, &pCData->guid)) break;
    }

    TRACE("(%p) guid %s %s found!x)\n", This, debugstr_guid(guid), pCData? "" : "NOT");

    if(pCData)
    {
        VariantInit( pVarVal);
        VariantCopy( pVarVal, &pCData->data);
        return S_OK;
    }
    return E_INVALIDARG;  /* FIXME: correct? */
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
    ITypeLibImpl *This = (ITypeLibImpl *)iface;

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
    ITypeLibImpl *This = (ITypeLibImpl *)iface;
    HRESULT result;
    ITypeInfo *pTInfo;

    FIXME("(%p) index %d lcid %d half implemented stub!\n", This, index, lcid);

    /* the help string should be obtained from the helpstringdll,
     * using the _DLLGetDocumentation function, based on the supplied
     * lcid. Nice to do sometime...
     */
    if(index<0)
    {
      /* documentation for the typelib */
      if(pbstrHelpString)
        *pbstrHelpString=SysAllocString(This->DocString);
      if(pdwHelpStringContext)
        *pdwHelpStringContext=This->dwHelpContext;
      if(pbstrHelpStringDll)
        *pbstrHelpStringDll=SysAllocString(This->HelpStringDll);

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

/* ITypeLib2::GetAllCustData
 *
 * Gets all custom data items for the library.
 *
 */
static HRESULT WINAPI ITypeLib2_fnGetAllCustData(
	ITypeLib2 * iface,
        CUSTDATA *pCustData)
{
    ITypeLibImpl *This = (ITypeLibImpl *)iface;
    TLBCustData *pCData;
    int i;
    TRACE("(%p) returning %d items\n", This, This->ctCustData);
    pCustData->prgCustData = TLB_Alloc(This->ctCustData * sizeof(CUSTDATAITEM));
    if(pCustData->prgCustData ){
        pCustData->cCustData=This->ctCustData;
        for(i=0, pCData=This->pCustData; pCData; i++, pCData = pCData->next){
            pCustData->prgCustData[i].guid=pCData->guid;
            VariantCopy(& pCustData->prgCustData[i].varValue, & pCData->data);
        }
    }else{
        ERR(" OUT OF MEMORY!\n");
        return E_OUTOFMEMORY;
    }
    return S_OK;
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

    return ITypeLib2_QueryInterface((ITypeLib *)This, riid, ppv);
}

static ULONG WINAPI ITypeLibComp_fnAddRef(ITypeComp * iface)
{
    ITypeLibImpl *This = impl_from_ITypeComp(iface);

    return ITypeLib2_AddRef((ITypeLib2 *)This);
}

static ULONG WINAPI ITypeLibComp_fnRelease(ITypeComp * iface)
{
    ITypeLibImpl *This = impl_from_ITypeComp(iface);

    return ITypeLib2_Release((ITypeLib2 *)This);
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
    ITypeInfoImpl *pTypeInfo;

    TRACE("(%s, 0x%x, 0x%x, %p, %p, %p)\n", debugstr_w(szName), lHash, wFlags, ppTInfo, pDescKind, pBindPtr);

    *pDescKind = DESCKIND_NONE;
    pBindPtr->lptcomp = NULL;
    *ppTInfo = NULL;

    for (pTypeInfo = This->pTypeInfo; pTypeInfo; pTypeInfo = pTypeInfo->next)
    {
        TRACE("testing %s\n", debugstr_w(pTypeInfo->Name));

        /* FIXME: check wFlags here? */
        /* FIXME: we should use a hash table to look this info up using lHash
         * instead of an O(n) search */
        if ((pTypeInfo->TypeAttr.typekind == TKIND_ENUM) ||
            (pTypeInfo->TypeAttr.typekind == TKIND_MODULE))
        {
            if (pTypeInfo->Name && !strcmpW(pTypeInfo->Name, szName))
            {
                *pDescKind = DESCKIND_TYPECOMP;
                pBindPtr->lptcomp = (ITypeComp *)&pTypeInfo->lpVtblTypeComp;
                ITypeComp_AddRef(pBindPtr->lptcomp);
                TRACE("module or enum: %s\n", debugstr_w(szName));
                return S_OK;
            }
        }

        if ((pTypeInfo->TypeAttr.typekind == TKIND_MODULE) ||
            (pTypeInfo->TypeAttr.typekind == TKIND_ENUM))
        {
            ITypeComp *pSubTypeComp = (ITypeComp *)&pTypeInfo->lpVtblTypeComp;
            HRESULT hr;

            hr = ITypeComp_Bind(pSubTypeComp, szName, lHash, wFlags, ppTInfo, pDescKind, pBindPtr);
            if (SUCCEEDED(hr) && (*pDescKind != DESCKIND_NONE))
            {
                TRACE("found in module or in enum: %s\n", debugstr_w(szName));
                return S_OK;
            }
        }

        if ((pTypeInfo->TypeAttr.typekind == TKIND_COCLASS) &&
            (pTypeInfo->TypeAttr.wTypeFlags & TYPEFLAG_FAPPOBJECT))
        {
            ITypeComp *pSubTypeComp = (ITypeComp *)&pTypeInfo->lpVtblTypeComp;
            HRESULT hr;
            ITypeInfo *subtypeinfo;
            BINDPTR subbindptr;
            DESCKIND subdesckind;

            hr = ITypeComp_Bind(pSubTypeComp, szName, lHash, wFlags,
                &subtypeinfo, &subdesckind, &subbindptr);
            if (SUCCEEDED(hr) && (subdesckind != DESCKIND_NONE))
            {
                TYPEDESC tdesc_appobject =
                {
                    {
                        (TYPEDESC *)pTypeInfo->hreftype
                    },
                    VT_USERDEFINED
                };
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
                *ppTInfo = (ITypeInfo *)pTypeInfo;
                ITypeInfo_AddRef(*ppTInfo);
                return S_OK;
            }
        }
    }

    TRACE("name not found %s\n", debugstr_w(szName));
    return S_OK;
}

static HRESULT WINAPI ITypeLibComp_fnBindType(
    ITypeComp * iface,
    OLECHAR * szName,
    ULONG lHash,
    ITypeInfo ** ppTInfo,
    ITypeComp ** ppTComp)
{
    FIXME("(%s, %x, %p, %p): stub\n", debugstr_w(szName), lHash, ppTInfo, ppTComp);
    return E_NOTIMPL;
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
static ITypeInfo2 * ITypeInfo_Constructor(void)
{
    ITypeInfoImpl * pTypeInfoImpl;

    pTypeInfoImpl = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ITypeInfoImpl));
    if (pTypeInfoImpl)
    {
      pTypeInfoImpl->lpVtbl = &tinfvt;
      pTypeInfoImpl->lpVtblTypeComp = &tcompvt;
      pTypeInfoImpl->ref=1;
      pTypeInfoImpl->hreftype = -1;
      pTypeInfoImpl->TypeAttr.memidConstructor = MEMBERID_NIL;
      pTypeInfoImpl->TypeAttr.memidDestructor = MEMBERID_NIL;
    }
    TRACE("(%p)\n", pTypeInfoImpl);
    return (ITypeInfo2*) pTypeInfoImpl;
}

/* ITypeInfo::QueryInterface
 */
static HRESULT WINAPI ITypeInfo_fnQueryInterface(
	ITypeInfo2 *iface,
	REFIID riid,
	VOID **ppvObject)
{
    ITypeLibImpl *This = (ITypeLibImpl *)iface;

    TRACE("(%p)->(IID: %s)\n",This,debugstr_guid(riid));

    *ppvObject=NULL;
    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid,&IID_ITypeInfo)||
            IsEqualIID(riid,&IID_ITypeInfo2))
        *ppvObject = This;

    if(*ppvObject){
        ITypeInfo_AddRef(iface);
        TRACE("-- Interface: (%p)->(%p)\n",ppvObject,*ppvObject);
        return S_OK;
    }
    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/* ITypeInfo::AddRef
 */
static ULONG WINAPI ITypeInfo_fnAddRef( ITypeInfo2 *iface)
{
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    ITypeLib2_AddRef((ITypeLib2*)This->pTypeLib);

    TRACE("(%p)->ref is %u\n",This, ref);
    return ref;
}

/* ITypeInfo::Release
 */
static ULONG WINAPI ITypeInfo_fnRelease(ITypeInfo2 *iface)
{
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%u)\n",This, ref);

    if (ref)   {
      /* We don't release ITypeLib when ref=0 because
         it means that function is called by ITypeLib2_Release */
      ITypeLib2_Release((ITypeLib2*)This->pTypeLib);
    } else   {
      TLBFuncDesc *pFInfo, *pFInfoNext;
      TLBVarDesc *pVInfo, *pVInfoNext;
      TLBImplType *pImpl, *pImplNext;

      TRACE("destroying ITypeInfo(%p)\n",This);

      if (This->no_free_data)
          goto finish_free;

      SysFreeString(This->Name);
      This->Name = NULL;

      SysFreeString(This->DocString);
      This->DocString = NULL;

      SysFreeString(This->DllName);
      This->DllName = NULL;

      for (pFInfo = This->funclist; pFInfo; pFInfo = pFInfoNext)
      {
          INT i;
          for(i = 0;i < pFInfo->funcdesc.cParams; i++)
          {
              ELEMDESC *elemdesc = &pFInfo->funcdesc.lprgelemdescParam[i];
              if (elemdesc->u.paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT)
              {
                  VariantClear(&elemdesc->u.paramdesc.pparamdescex->varDefaultValue);
                  TLB_Free(elemdesc->u.paramdesc.pparamdescex);
              }
              SysFreeString(pFInfo->pParamDesc[i].Name);
          }
          TLB_Free(pFInfo->funcdesc.lprgelemdescParam);
          TLB_Free(pFInfo->pParamDesc);
          TLB_FreeCustData(pFInfo->pCustData);
          if (HIWORD(pFInfo->Entry) != 0 && pFInfo->Entry != (BSTR)-1) 
              SysFreeString(pFInfo->Entry);
          SysFreeString(pFInfo->HelpString);
          SysFreeString(pFInfo->Name);

          pFInfoNext = pFInfo->next;
          TLB_Free(pFInfo);
      }
      for (pVInfo = This->varlist; pVInfo; pVInfo = pVInfoNext)
      {
          if (pVInfo->vardesc.varkind == VAR_CONST)
          {
              VariantClear(pVInfo->vardesc.u.lpvarValue);
              TLB_Free(pVInfo->vardesc.u.lpvarValue);
          }
          TLB_FreeCustData(pVInfo->pCustData);
          SysFreeString(pVInfo->Name);
          pVInfoNext = pVInfo->next;
          TLB_Free(pVInfo);
      }
      for(pImpl = This->impltypelist; pImpl; pImpl = pImplNext)
      {
          TLB_FreeCustData(pImpl->pCustData);
          pImplNext = pImpl->next;
          TLB_Free(pImpl);
      }
      TLB_FreeCustData(This->pCustData);

finish_free:
      if (This->next)
      {
        ITypeInfo_Release((ITypeInfo*)This->next);
      }

      HeapFree(GetProcessHeap(),0,This);
      return 0;
    }
    return ref;
}

/* ITypeInfo::GetTypeAttr
 *
 * Retrieves a TYPEATTR structure that contains the attributes of the type
 * description.
 *
 */
static HRESULT WINAPI ITypeInfo_fnGetTypeAttr( ITypeInfo2 *iface,
        LPTYPEATTR  *ppTypeAttr)
{
    const ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    SIZE_T size;

    TRACE("(%p)\n",This);

    size = sizeof(**ppTypeAttr);
    if (This->TypeAttr.typekind == TKIND_ALIAS)
        size += TLB_SizeTypeDesc(&This->TypeAttr.tdescAlias, FALSE);

    *ppTypeAttr = HeapAlloc(GetProcessHeap(), 0, size);
    if (!*ppTypeAttr)
        return E_OUTOFMEMORY;

    **ppTypeAttr = This->TypeAttr;

    if (This->TypeAttr.typekind == TKIND_ALIAS)
        TLB_CopyTypeDesc(&(*ppTypeAttr)->tdescAlias,
            &This->TypeAttr.tdescAlias, *ppTypeAttr + 1);

    if((*ppTypeAttr)->typekind == TKIND_DISPATCH) {
        /* This should include all the inherited funcs */
        (*ppTypeAttr)->cFuncs = (*ppTypeAttr)->cbSizeVft / sizeof(void *);
        (*ppTypeAttr)->cbSizeVft = 7 * sizeof(void *); /* This is always the size of IDispatch's vtbl */
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;

    TRACE("(%p)->(%p)\n", This, ppTComp);

    *ppTComp = (ITypeComp *)&This->lpVtblTypeComp;
    ITypeComp_AddRef(*ppTComp);
    return S_OK;
}

static SIZE_T TLB_SizeElemDesc( const ELEMDESC *elemdesc )
{
    SIZE_T size = TLB_SizeTypeDesc(&elemdesc->tdesc, FALSE);
    if (elemdesc->u.paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT)
        size += sizeof(*elemdesc->u.paramdesc.pparamdescex);
    return size;
}

static HRESULT TLB_CopyElemDesc( const ELEMDESC *src, ELEMDESC *dest, char **buffer )
{
    *dest = *src;
    *buffer = TLB_CopyTypeDesc(&dest->tdesc, &src->tdesc, *buffer);
    if (src->u.paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT)
    {
        const PARAMDESCEX *pparamdescex_src = src->u.paramdesc.pparamdescex;
        PARAMDESCEX *pparamdescex_dest = dest->u.paramdesc.pparamdescex = (PARAMDESCEX *)*buffer;
        *buffer += sizeof(PARAMDESCEX);
        *pparamdescex_dest = *pparamdescex_src;
        VariantInit(&pparamdescex_dest->varDefaultValue);
        return VariantCopy(&pparamdescex_dest->varDefaultValue, 
                           (VARIANTARG *)&pparamdescex_src->varDefaultValue);
    }
    else
        dest->u.paramdesc.pparamdescex = NULL;
    return S_OK;
}

static void TLB_FreeElemDesc( ELEMDESC *elemdesc )
{
    if (elemdesc->u.paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT)
        VariantClear(&elemdesc->u.paramdesc.pparamdescex->varDefaultValue);
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

    dest->lprgscode = (SCODE *)buffer;
    memcpy(dest->lprgscode, src->lprgscode, sizeof(*src->lprgscode) * src->cScodes);
    buffer += sizeof(*src->lprgscode) * src->cScodes;

    hr = TLB_CopyElemDesc(&src->elemdescFunc, &dest->elemdescFunc, &buffer);
    if (FAILED(hr))
    {
        SysFreeString((BSTR)dest);
        return hr;
    }

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

    /* special treatment for dispinterfaces: this makes functions appear
     * to return their [retval] value when it is really returning an
     * HRESULT */
    if (dispinterface && dest->elemdescFunc.tdesc.vt == VT_HRESULT)
    {
        if (dest->cParams &&
            (dest->lprgelemdescParam[dest->cParams - 1].u.paramdesc.wParamFlags & PARAMFLAG_FRETVAL))
        {
            ELEMDESC *elemdesc = &dest->lprgelemdescParam[dest->cParams - 1];
            if (elemdesc->tdesc.vt != VT_PTR)
            {
                ERR("elemdesc should have started with VT_PTR instead of:\n");
                if (ERR_ON(ole))
                    dump_ELEMDESC(elemdesc);
                return E_UNEXPECTED;
            }

            /* copy last parameter to the return value. we are using a flat
             * buffer so there is no danger of leaking memory in
             * elemdescFunc */
            dest->elemdescFunc.tdesc = *elemdesc->tdesc.u.lptdesc;

            /* remove the last parameter */
            dest->cParams--;
        }
        else
            /* otherwise this function is made to appear to have no return
             * value */
            dest->elemdescFunc.tdesc.vt = VT_VOID;

    }

    *dest_ptr = dest;
    return S_OK;
}

HRESULT ITypeInfoImpl_GetInternalFuncDesc( ITypeInfo *iface, UINT index, const FUNCDESC **ppFuncDesc )
{
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    const TLBFuncDesc *pFDesc;
    UINT i;

    for(i=0, pFDesc=This->funclist; i!=index && pFDesc; i++, pFDesc=pFDesc->next)
        ;

    if (pFDesc)
    {
        *ppFuncDesc = &pFDesc->funcdesc;
        return S_OK;
    }

    return TYPE_E_ELEMENTNOTFOUND;
}

/* internal function to make the inherited interfaces' methods appear
 * part of the interface */
static HRESULT ITypeInfoImpl_GetInternalDispatchFuncDesc( ITypeInfo *iface,
    UINT index, const FUNCDESC **ppFuncDesc, UINT *funcs, UINT *hrefoffset)
{
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    HRESULT hr;
    UINT implemented_funcs = 0;

    if (funcs)
        *funcs = 0;
    else
        *hrefoffset = DISPATCH_HREF_OFFSET;

    if(This->impltypelist)
    {
        ITypeInfo *pSubTypeInfo;
        UINT sub_funcs;

        hr = ITypeInfo_GetRefTypeInfo(iface, This->impltypelist->hRef, &pSubTypeInfo);
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
        *funcs = implemented_funcs + This->TypeAttr.cFuncs;
    else
        *hrefoffset = 0;
    
    if (index < implemented_funcs)
        return E_INVALIDARG;
    return ITypeInfoImpl_GetInternalFuncDesc(iface, index - implemented_funcs,
                                             ppFuncDesc);
}

static inline void ITypeInfoImpl_ElemDescAddHrefOffset( LPELEMDESC pElemDesc, UINT hrefoffset)
{
    TYPEDESC *pTypeDesc = &pElemDesc->tdesc;
    while (TRUE)
    {
        switch (pTypeDesc->vt)
        {
        case VT_USERDEFINED:
            pTypeDesc->u.hreftype += hrefoffset;
            return;
        case VT_PTR:
        case VT_SAFEARRAY:
            pTypeDesc = pTypeDesc->u.lptdesc;
            break;
        case VT_CARRAY:
            pTypeDesc = &pTypeDesc->u.lpadesc->tdescElem;
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    const FUNCDESC *internal_funcdesc;
    HRESULT hr;
    UINT hrefoffset = 0;

    TRACE("(%p) index %d\n", This, index);

    if (This->TypeAttr.typekind == TKIND_DISPATCH)
        hr = ITypeInfoImpl_GetInternalDispatchFuncDesc((ITypeInfo *)iface, index,
                                                       &internal_funcdesc, NULL,
                                                       &hrefoffset);
    else
        hr = ITypeInfoImpl_GetInternalFuncDesc((ITypeInfo *)iface, index,
                                               &internal_funcdesc);
    if (FAILED(hr))
    {
        WARN("description for function %d not found\n", index);
        return hr;
    }

    hr = TLB_AllocAndInitFuncDesc(
        internal_funcdesc,
        ppFuncDesc,
        This->TypeAttr.typekind == TKIND_DISPATCH);

    if ((This->TypeAttr.typekind == TKIND_DISPATCH) && hrefoffset)
        ITypeInfoImpl_FuncDescAddHrefOffset(*ppFuncDesc, hrefoffset);

    TRACE("-- 0x%08x\n", hr);
    return hr;
}

static HRESULT TLB_AllocAndInitVarDesc( const VARDESC *src, VARDESC **dest_ptr )
{
    VARDESC *dest;
    char *buffer;
    SIZE_T size = sizeof(*src);
    HRESULT hr;

    if (src->lpstrSchema) size += (strlenW(src->lpstrSchema) + 1) * sizeof(WCHAR);
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
        len = strlenW(src->lpstrSchema);
        memcpy(dest->lpstrSchema, src->lpstrSchema, (len + 1) * sizeof(WCHAR));
        buffer += (len + 1) * sizeof(WCHAR);
    }

    if (src->varkind == VAR_CONST)
    {
        HRESULT hr;

        dest->u.lpvarValue = (VARIANT *)buffer;
        *dest->u.lpvarValue = *src->u.lpvarValue;
        buffer += sizeof(VARIANT);
        VariantInit(dest->u.lpvarValue);
        hr = VariantCopy(dest->u.lpvarValue, src->u.lpvarValue);
        if (FAILED(hr))
        {
            SysFreeString((BSTR)dest_ptr);
            return hr;
        }
    }
    hr = TLB_CopyElemDesc(&src->elemdescVar, &dest->elemdescVar, &buffer);
    if (FAILED(hr))
    {
        if (src->varkind == VAR_CONST)
            VariantClear(dest->u.lpvarValue);
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    UINT i;
    const TLBVarDesc *pVDesc;

    TRACE("(%p) index %d\n", This, index);

    for(i=0, pVDesc=This->varlist; i!=index && pVDesc; i++, pVDesc=pVDesc->next)
        ;

    if (pVDesc)
        return TLB_AllocAndInitVarDesc(&pVDesc->vardesc, ppVarDesc);

    return E_INVALIDARG;
}

/* ITypeInfo_GetNames
 *
 * Retrieves the variable with the specified member ID (or the name of the
 * property or method and its parameters) that correspond to the specified
 * function ID.
 */
static HRESULT WINAPI ITypeInfo_fnGetNames( ITypeInfo2 *iface, MEMBERID memid,
        BSTR  *rgBstrNames, UINT cMaxNames, UINT  *pcNames)
{
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    const TLBFuncDesc *pFDesc;
    const TLBVarDesc *pVDesc;
    int i;
    TRACE("(%p) memid=0x%08x Maxname=%d\n", This, memid, cMaxNames);
    for(pFDesc=This->funclist; pFDesc && pFDesc->funcdesc.memid != memid; pFDesc=pFDesc->next);
    if(pFDesc)
    {
      /* function found, now return function and parameter names */
      for(i=0; i<cMaxNames && i <= pFDesc->funcdesc.cParams; i++)
      {
        if(!i)
	  *rgBstrNames=SysAllocString(pFDesc->Name);
        else
          rgBstrNames[i]=SysAllocString(pFDesc->pParamDesc[i-1].Name);
      }
      *pcNames=i;
    }
    else
    {
      for(pVDesc=This->varlist; pVDesc && pVDesc->vardesc.memid != memid; pVDesc=pVDesc->next);
      if(pVDesc)
      {
        *rgBstrNames=SysAllocString(pVDesc->Name);
        *pcNames=1;
      }
      else
      {
        if(This->impltypelist &&
	   (This->TypeAttr.typekind==TKIND_INTERFACE || This->TypeAttr.typekind==TKIND_DISPATCH)) {
          /* recursive search */
          ITypeInfo *pTInfo;
          HRESULT result;
          result=ITypeInfo_GetRefTypeInfo(iface, This->impltypelist->hRef,
					  &pTInfo);
          if(SUCCEEDED(result))
	  {
            result=ITypeInfo_GetNames(pTInfo, memid, rgBstrNames, cMaxNames, pcNames);
            ITypeInfo_Release(pTInfo);
            return result;
          }
          WARN("Could not search inherited interface!\n");
        }
        else
	{
          WARN("no names found\n");
	}
        *pcNames=0;
        return TYPE_E_ELEMENTNOTFOUND;
      }
    }
    return S_OK;
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    UINT i;
    HRESULT hr = S_OK;
    const TLBImplType *pImpl = This->impltypelist;

    TRACE("(%p) index %d\n", This, index);
    if (TRACE_ON(ole)) dump_TypeInfo(This);

    if(index==(UINT)-1)
    {
      /* only valid on dual interfaces;
         retrieve the associated TKIND_INTERFACE handle for the current TKIND_DISPATCH
      */
      if( This->TypeAttr.typekind != TKIND_DISPATCH) return E_INVALIDARG;

      if (This->TypeAttr.wTypeFlags & TYPEFLAG_FDISPATCHABLE &&
          This->TypeAttr.wTypeFlags & TYPEFLAG_FDUAL )
      {
        *pRefType = -1;
      }
      else
      {
        hr = TYPE_E_ELEMENTNOTFOUND;
      }
    }
    else if(index == 0 && This->TypeAttr.typekind == TKIND_DISPATCH)
    {
      /* All TKIND_DISPATCHs are made to look like they inherit from IDispatch */
      *pRefType = This->pTypeLib->dispatch_href;
    }
    else
    {
      /* get element n from linked list */
      for(i=0; pImpl && i<index; i++)
      {
        pImpl = pImpl->next;
      }

      if (pImpl)
        *pRefType = pImpl->hRef;
      else
        hr = TYPE_E_ELEMENTNOTFOUND;
    }

    if(TRACE_ON(ole))
    {
        if(SUCCEEDED(hr))
            TRACE("SUCCESS -- hRef = 0x%08x\n", *pRefType );
        else
            TRACE("FAILURE -- hresult = 0x%08x\n", hr);
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    UINT i;
    TLBImplType *pImpl;

    TRACE("(%p) index %d\n", This, index);
    for(i=0, pImpl=This->impltypelist; i<index && pImpl;
	i++, pImpl=pImpl->next)
        ;
    if(i==index && pImpl){
        *pImplTypeFlags=pImpl->implflags;
        return S_OK;
    }
    *pImplTypeFlags=0;
    return TYPE_E_ELEMENTNOTFOUND;
}

/* GetIDsOfNames
 * Maps between member names and member IDs, and parameter names and
 * parameter IDs.
 */
static HRESULT WINAPI ITypeInfo_fnGetIDsOfNames( ITypeInfo2 *iface,
        LPOLESTR  *rgszNames, UINT cNames, MEMBERID  *pMemId)
{
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    const TLBFuncDesc *pFDesc;
    const TLBVarDesc *pVDesc;
    HRESULT ret=S_OK;
    UINT i;

    TRACE("(%p) Name %s cNames %d\n", This, debugstr_w(*rgszNames),
            cNames);

    /* init out parameters in case of failure */
    for (i = 0; i < cNames; i++)
        pMemId[i] = MEMBERID_NIL;

    for(pFDesc=This->funclist; pFDesc; pFDesc=pFDesc->next) {
        int j;
        if(!lstrcmpiW(*rgszNames, pFDesc->Name)) {
            if(cNames) *pMemId=pFDesc->funcdesc.memid;
            for(i=1; i < cNames; i++){
                for(j=0; j<pFDesc->funcdesc.cParams; j++)
                    if(!lstrcmpiW(rgszNames[i],pFDesc->pParamDesc[j].Name))
                            break;
                if( j<pFDesc->funcdesc.cParams)
                    pMemId[i]=j;
                else
                   ret=DISP_E_UNKNOWNNAME;
            };
            TRACE("-- 0x%08x\n", ret);
            return ret;
        }
    }
    for(pVDesc=This->varlist; pVDesc; pVDesc=pVDesc->next) {
        if(!lstrcmpiW(*rgszNames, pVDesc->Name)) {
            if(cNames) *pMemId=pVDesc->vardesc.memid;
            return ret;
        }
    }
    /* not found, see if it can be found in an inherited interface */
    if(This->impltypelist) {
        /* recursive search */
        ITypeInfo *pTInfo;
        ret=ITypeInfo_GetRefTypeInfo(iface,
                This->impltypelist->hRef, &pTInfo);
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

/* ITypeInfo::Invoke
 *
 * Invokes a method, or accesses a property of an object, that implements the
 * interface described by the type description.
 */
DWORD
_invoke(FARPROC func,CALLCONV callconv, int nrargs, DWORD *args) {
    DWORD res;

    if (TRACE_ON(ole)) {
	int i;
	TRACE("Calling %p(",func);
	for (i=0;i<nrargs;i++) TRACE("%08x,",args[i]);
	TRACE(")\n");
    }

    switch (callconv) {
    case CC_STDCALL:

	switch (nrargs) {
	case 0:
		res = func();
		break;
	case 1:
		res = func(args[0]);
		break;
	case 2:
		res = func(args[0],args[1]);
		break;
	case 3:
		res = func(args[0],args[1],args[2]);
		break;
	case 4:
		res = func(args[0],args[1],args[2],args[3]);
		break;
	case 5:
		res = func(args[0],args[1],args[2],args[3],args[4]);
		break;
	case 6:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5]);
		break;
	case 7:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
		break;
	case 8:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
		break;
	case 9:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8]);
		break;
	case 10:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9]);
		break;
	case 11:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10]);
		break;
	case 12:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11]);
		break;
	case 13:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12]);
		break;
	case 14:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13]);
		break;
	case 15:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14]);
		break;
	case 16:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15]);
		break;
	case 17:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16]);
		break;
	case 18:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16],args[17]);
		break;
	case 19:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16],args[17],args[18]);
		break;
	case 20:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16],args[17],args[18],args[19]);
		break;
	case 21:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16],args[17],args[18],args[19],args[20]);
		break;
	case 22:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16],args[17],args[18],args[19],args[20],args[21]);
		break;
	case 23:
		res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16],args[17],args[18],args[19],args[20],args[21],args[22]);
		break;
	case 24:
                res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16],args[17],args[18],args[19],args[20],args[21],args[22],args[23]);
                break;
	case 25:
                res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16],args[17],args[18],args[19],args[20],args[21],args[22],args[23],args[24]);
                break;
	case 26:
                res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16],args[17],args[18],args[19],args[20],args[21],args[22],args[23],args[24],args[25]);
                break;
	case 27:
                res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16],args[17],args[18],args[19],args[20],args[21],args[22],args[23],args[24],args[25],args[26]);
                break;
	case 28:
                res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16],args[17],args[18],args[19],args[20],args[21],args[22],args[23],args[24],args[25],args[26],args[27]);
                break;
	case 29:
                res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16],args[17],args[18],args[19],args[20],args[21],args[22],args[23],args[24],args[25],args[26],args[27],args[28]);
                break;
	case 30:
                res = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15],args[16],args[17],args[18],args[19],args[20],args[21],args[22],args[23],args[24],args[25],args[26],args[27],args[28],args[29]);
                break;
	default:
		FIXME("unsupported number of arguments %d in stdcall\n",nrargs);
		res = -1;
		break;
	}
	break;
    default:
	FIXME("unsupported calling convention %d\n",callconv);
	res = -1;
	break;
    }
    TRACE("returns %08x\n",res);
    return res;
}

/* The size of the argument on the stack in DWORD units (in all x86 call
 * convetions the arguments on the stack are DWORD-aligned)
 */
static int _dispargsize(VARTYPE vt)
{
    switch (vt) {
    case VT_I8:
    case VT_UI8:
	return 8/sizeof(DWORD);
    case VT_R8:
        return sizeof(double)/sizeof(DWORD);
    case VT_DECIMAL:
        return (sizeof(DECIMAL)+3)/sizeof(DWORD);
    case VT_CY:
        return sizeof(CY)/sizeof(DWORD);
    case VT_DATE:
	return sizeof(DATE)/sizeof(DWORD);
    case VT_VARIANT:
	return (sizeof(VARIANT)+3)/sizeof(DWORD);
    case VT_RECORD:
        FIXME("VT_RECORD not implemented\n");
        return 1;
    default:
	return 1;
    }
}

static HRESULT userdefined_to_variantvt(ITypeInfo *tinfo, const TYPEDESC *tdesc, VARTYPE *vt)
{
    HRESULT hr = S_OK;
    ITypeInfo *tinfo2 = NULL;
    TYPEATTR *tattr = NULL;

    hr = ITypeInfo_GetRefTypeInfo(tinfo, tdesc->u.hreftype, &tinfo2);
    if (hr)
    {
        ERR("Could not get typeinfo of hreftype %x for VT_USERDEFINED, "
            "hr = 0x%08x\n",
              tdesc->u.hreftype, hr);
        return hr;
    }
    hr = ITypeInfo_GetTypeAttr(tinfo2, &tattr);
    if (hr)
    {
        ERR("ITypeInfo_GetTypeAttr failed, hr = 0x%08x\n", hr);
        ITypeInfo_Release(tinfo2);
        return hr;
    }

    switch (tattr->typekind)
    {
    case TKIND_ENUM:
        *vt |= VT_I4;
        break;

    case TKIND_ALIAS:
        tdesc = &tattr->tdescAlias;
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
        tdesc = tdesc->u.lptdesc;

        /* munch VT_PTR -> VT_USERDEFINED(interface) into VT_UNKNOWN or
         * VT_DISPATCH and VT_PTR -> VT_PTR -> VT_USERDEFINED(interface) into 
         * VT_BYREF|VT_DISPATCH or VT_BYREF|VT_UNKNOWN */
        if ((tdesc->vt == VT_USERDEFINED) ||
            ((tdesc->vt == VT_PTR) && (tdesc->u.lptdesc->vt == VT_USERDEFINED)))
        {
            VARTYPE vt_userdefined = 0;
            const TYPEDESC *tdesc_userdefined = tdesc;
            if (tdesc->vt == VT_PTR)
            {
                vt_userdefined = VT_BYREF;
                tdesc_userdefined = tdesc->u.lptdesc;
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
        hr = typedescvt_to_variantvt(tinfo, tdesc->u.lptdesc, vt);
        break;
    default:
        *vt |= tdesc->vt;
        break;
    }
    return hr;
}

/***********************************************************************
 *		DispCallFunc (OLEAUT32.@)
 *
 * Invokes a function of the specified calling convention, passing the
 * specified arguments and returns the result.
 *
 * PARAMS
 *  pvInstance  [I] Optional pointer to the instance whose function to invoke.
 *  oVft        [I] The offset in the vtable. See notes.
 *  cc          [I] Calling convention of the function to call.
 *  vtReturn    [I] The return type of the function.
 *  cActuals    [I] Number of parameters.
 *  prgvt       [I] The types of the parameters to pass. This is used for sizing only.
 *  prgpvarg    [I] The arguments to pass.
 *  pvargResult [O] The return value of the function. Can be NULL.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * NOTES
 *  The HRESULT return value of this function is not affected by the return
 *  value of the user supplied function, which is returned in pvargResult.
 *
 *  If pvInstance is NULL then a non-object function is to be called and oVft
 *  is the address of the function to call.
 *
 * The cc parameter can be one of the following values:
 *|CC_FASTCALL
 *|CC_CDECL
 *|CC_PASCAL
 *|CC_STDCALL
 *|CC_FPFASTCALL
 *|CC_SYSCALL
 *|CC_MPWCDECL
 *|CC_MPWPASCAL
 *
 */
HRESULT WINAPI
DispCallFunc(
    void* pvInstance, ULONG_PTR oVft, CALLCONV cc, VARTYPE vtReturn, UINT cActuals,
    VARTYPE* prgvt, VARIANTARG** prgpvarg, VARIANT* pvargResult)
{
    int argsize, argspos;
    UINT i;
    DWORD *args;
    HRESULT hres;

    TRACE("(%p, %ld, %d, %d, %d, %p, %p, %p (vt=%d))\n",
        pvInstance, oVft, cc, vtReturn, cActuals, prgvt, prgpvarg,
        pvargResult, V_VT(pvargResult));

    argsize = 0;
    if (pvInstance)
        argsize++; /* for This pointer */

    for (i=0;i<cActuals;i++)
    {
        TRACE("arg %u: type %d, size %d\n",i,prgvt[i],_dispargsize(prgvt[i]));
        dump_Variant(prgpvarg[i]);
        argsize += _dispargsize(prgvt[i]);
    }
    args = HeapAlloc(GetProcessHeap(),0,sizeof(DWORD)*argsize);

    argspos = 0;
    if (pvInstance)
    {
        args[0] = (DWORD)pvInstance; /* the This pointer is always the first parameter */
        argspos++;
    }

    for (i=0;i<cActuals;i++)
    {
        VARIANT *arg = prgpvarg[i];
        TRACE("Storing arg %u (%d as %d)\n",i,V_VT(arg),prgvt[i]);
        if (prgvt[i] == VT_VARIANT)
            memcpy(&args[argspos], arg, _dispargsize(prgvt[i]) * sizeof(DWORD));
        else
            memcpy(&args[argspos], &V_NONE(arg), _dispargsize(prgvt[i]) * sizeof(DWORD));
        argspos += _dispargsize(prgvt[i]);
    }

    if (pvInstance)
    {
        FARPROC *vtable = *(FARPROC**)pvInstance;
        hres = _invoke(vtable[oVft/sizeof(void *)], cc, argsize, args);
    }
    else
        /* if we aren't invoking an object then the function pointer is stored
         * in oVft */
        hres = _invoke((FARPROC)oVft, cc, argsize, args);

    if (pvargResult && (vtReturn != VT_EMPTY))
    {
        TRACE("Method returned 0x%08x\n",hres);
        V_VT(pvargResult) = vtReturn;
        V_UI4(pvargResult) = hres;
    }

    HeapFree(GetProcessHeap(),0,args);
    return S_OK;
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    int i;
    unsigned int var_index;
    TYPEKIND type_kind;
    HRESULT hres;
    const TLBFuncDesc *pFuncInfo;

    TRACE("(%p)(%p,id=%d,flags=0x%08x,%p,%p,%p,%p)\n",
      This,pIUnk,memid,wFlags,pDispParams,pVarResult,pExcepInfo,pArgErr
    );

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
    for (pFuncInfo = This->funclist; pFuncInfo; pFuncInfo=pFuncInfo->next)
        if ((memid == pFuncInfo->funcdesc.memid) &&
            (wFlags & pFuncInfo->funcdesc.invkind))
            break;

    if (pFuncInfo) {
        const FUNCDESC *func_desc = &pFuncInfo->funcdesc;

        if (TRACE_ON(ole))
        {
            TRACE("invoking:\n");
            dump_TLBFuncDescOne(pFuncInfo);
        }
        
	switch (func_desc->funckind) {
	case FUNC_PUREVIRTUAL:
	case FUNC_VIRTUAL: {
            void *buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INVBUF_ELEMENT_SIZE * func_desc->cParams);
            VARIANT varresult;
            VARIANT retval; /* pointer for storing byref retvals in */
            VARIANTARG **prgpvarg = INVBUF_GET_ARG_PTR_ARRAY(buffer, func_desc->cParams);
            VARIANTARG *rgvarg = INVBUF_GET_ARG_ARRAY(buffer, func_desc->cParams);
            VARTYPE *rgvt = INVBUF_GET_ARG_TYPE_ARRAY(buffer, func_desc->cParams);
            UINT cNamedArgs = pDispParams->cNamedArgs;
            DISPID *rgdispidNamedArgs = pDispParams->rgdispidNamedArgs;

            hres = S_OK;

            if (func_desc->invkind & (INVOKE_PROPERTYPUT|INVOKE_PROPERTYPUTREF))
            {
                if (!cNamedArgs || (rgdispidNamedArgs[0] != DISPID_PROPERTYPUT))
                {
                    ERR("first named arg for property put invocation must be DISPID_PROPERTYPUT\n");
                    hres = DISP_E_PARAMNOTFOUND;
                    goto func_fail;
                }
                /* ignore the DISPID_PROPERTYPUT named argument from now on */
                cNamedArgs--;
                rgdispidNamedArgs++;
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
                USHORT wParamFlags = func_desc->lprgelemdescParam[i].u.paramdesc.wParamFlags;
                VARIANTARG *src_arg;

                if (cNamedArgs)
                {
                    USHORT j;
                    src_arg = NULL;
                    for (j = 0; j < cNamedArgs; j++)
                        if (rgdispidNamedArgs[j] == i)
                        {
                            src_arg = &pDispParams->rgvarg[j];
                            break;
                        }
                }
                else
                    src_arg = i < pDispParams->cArgs ? &pDispParams->rgvarg[pDispParams->cArgs - 1 - i] : NULL;

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
                        VARIANTARG *arg;
                        arg = prgpvarg[i] = &rgvarg[i];
                        memset(arg, 0, sizeof(*arg));
                        V_VT(arg) = rgvt[i];
                        memset(&retval, 0, sizeof(retval));
                        V_BYREF(arg) = &retval;
                    }
                    else
                    {
                        ERR("[retval] parameter must be the last parameter of the method (%d/%d)\n", i, func_desc->cParams);
                        hres = E_UNEXPECTED;
                        break;
                    }
                }
                else if (src_arg)
                {
                    dump_Variant(src_arg);

                    if (rgvt[i] == VT_VARIANT)
                        hres = VariantCopy(&rgvarg[i], src_arg);
                    else if (rgvt[i] == (VT_VARIANT | VT_BYREF))
                    {
                        if (rgvt[i] == V_VT(src_arg))
                            V_VARIANTREF(&rgvarg[i]) = V_VARIANTREF(src_arg);
                        else
                        {
                            VARIANTARG *missing_arg = INVBUF_GET_MISSING_ARG_ARRAY(buffer, func_desc->cParams);
                            hres = VariantCopy(&missing_arg[i], src_arg);
                            V_VARIANTREF(&rgvarg[i]) = &missing_arg[i];
                        }
                        V_VT(&rgvarg[i]) = rgvt[i];
                    }
                    else if (rgvt[i] == (VT_VARIANT | VT_ARRAY) && func_desc->cParamsOpt < 0 && i == func_desc->cParams-1)
                    {
                        SAFEARRAY *a;
                        SAFEARRAYBOUND bound;
                        VARIANT *v;
                        LONG j;
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
                            ERR("SafeArrayAccessData failed with %x\n", hres);
                            break;
                        }
                        for (j = 0; j < bound.cElements; j++)
                            VariantCopy(&v[j], &pDispParams->rgvarg[pDispParams->cArgs - 1 - i - j]);
                        hres = SafeArrayUnaccessData(a);
                        if (hres != S_OK)
                        {
                            ERR("SafeArrayUnaccessData failed with %x\n", hres);
                            break;
                        }
                        V_ARRAY(&rgvarg[i]) = a;
                        V_VT(&rgvarg[i]) = rgvt[i];
                    }
                    else if ((rgvt[i] & VT_BYREF) && !V_ISBYREF(src_arg))
                    {
                        VARIANTARG *missing_arg = INVBUF_GET_MISSING_ARG_ARRAY(buffer, func_desc->cParams);
                        V_VT(&missing_arg[i]) = V_VT(src_arg);
                        hres = VariantChangeType(&missing_arg[i], src_arg, 0, rgvt[i] & ~VT_BYREF);
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
                        ERR("failed to convert param %d to %s%s from %s%s\n", i,
                            debugstr_vt(rgvt[i]), debugstr_vf(rgvt[i]),
                            debugstr_VT(src_arg), debugstr_VF(src_arg));
                        break;
                    }
                    prgpvarg[i] = &rgvarg[i];
                }
                else if (wParamFlags & PARAMFLAG_FOPT)
                {
                    VARIANTARG *arg;
                    arg = prgpvarg[i] = &rgvarg[i];
                    if (wParamFlags & PARAMFLAG_FHASDEFAULT)
                    {
                        hres = VariantCopy(arg, &func_desc->lprgelemdescParam[i].u.paramdesc.pparamdescex->varDefaultValue);
                        if (FAILED(hres))
                            break;
                    }
                    else
                    {
                        VARIANTARG *missing_arg;
                        /* if the function wants a pointer to a variant then
                         * set that up, otherwise just pass the VT_ERROR in
                         * the argument by value */
                        if (rgvt[i] & VT_BYREF)
                        {
                            missing_arg = INVBUF_GET_MISSING_ARG_ARRAY(buffer, func_desc->cParams) + i;
                            V_VT(arg) = VT_VARIANT | VT_BYREF;
                            V_VARIANTREF(arg) = missing_arg;
                        }
                        else
                            missing_arg = arg;
                        V_VT(missing_arg) = VT_ERROR;
                        V_ERROR(missing_arg) = DISP_E_PARAMNOTFOUND;
                    }
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

            hres = DispCallFunc(pIUnk, func_desc->oVft, func_desc->callconv,
                                V_VT(&varresult), func_desc->cParams, rgvt,
                                prgpvarg, &varresult);

            for (i = 0; i < func_desc->cParams; i++)
            {
                USHORT wParamFlags = func_desc->lprgelemdescParam[i].u.paramdesc.wParamFlags;
                if (wParamFlags & PARAMFLAG_FRETVAL)
                {
                    if (TRACE_ON(ole))
                    {
                        TRACE("[retval] value: ");
                        dump_Variant(prgpvarg[i]);
                    }

                    if (pVarResult)
                    {
                        VariantInit(pVarResult);
                        /* deref return value */
                        hres = VariantCopyInd(pVarResult, prgpvarg[i]);
                    }

                    /* free data stored in varresult. Note that
                     * VariantClear doesn't do what we want because we are
                     * working with byref types. */
                    /* FIXME: clear safearrays, bstrs, records and
                     * variants here too */
                    if ((V_VT(prgpvarg[i]) == (VT_UNKNOWN | VT_BYREF)) ||
                         (V_VT(prgpvarg[i]) == (VT_DISPATCH | VT_BYREF)))
                    {
                        if(*V_UNKNOWNREF(prgpvarg[i]))
                            IUnknown_Release(*V_UNKNOWNREF(prgpvarg[i]));
                    }
                    break;
                }
                else if (i < pDispParams->cArgs)
                {
                    if (wParamFlags & PARAMFLAG_FOUT)
                    {
                        VARIANTARG *arg = &pDispParams->rgvarg[pDispParams->cArgs - 1 - i];

                        if ((rgvt[i] == VT_BYREF) && (V_VT(arg) != VT_BYREF))
                            hres = VariantChangeType(arg, &rgvarg[i], 0, V_VT(arg));

                        if (FAILED(hres))
                        {
                            ERR("failed to convert param %d to vt %d\n", i,
                                V_VT(&pDispParams->rgvarg[pDispParams->cArgs - 1 - i]));
                            break;
                        }
                    }
                    else if (V_VT(prgpvarg[i]) == (VT_VARIANT | VT_ARRAY) &&
                             func_desc->cParamsOpt < 0 &&
                             i == func_desc->cParams-1)
                    {
                        SAFEARRAY *a = V_ARRAY(prgpvarg[i]);
                        LONG j, ubound;
                        VARIANT *v;
                        hres = SafeArrayGetUBound(a, 1, &ubound);
                        if (hres != S_OK)
                        {
                            ERR("SafeArrayGetUBound failed with %x\n", hres);
                            break;
                        }
                        hres = SafeArrayAccessData(a, (LPVOID)&v);
                        if (hres != S_OK)
                        {
                            ERR("SafeArrayAccessData failed with %x\n", hres);
                            break;
                        }
                        for (j = 0; j <= ubound; j++)
                            VariantClear(&v[j]);
                        hres = SafeArrayUnaccessData(a);
                        if (hres != S_OK)
                        {
                            ERR("SafeArrayUnaccessData failed with %x\n", hres);
                            break;
                        }
                    }
                    VariantClear(&rgvarg[i]);
                }
                else if (wParamFlags & PARAMFLAG_FOPT)
                {
                    if (wParamFlags & PARAMFLAG_FHASDEFAULT)
                        VariantClear(&rgvarg[i]);
                }
            }

            if ((V_VT(&varresult) == VT_ERROR) && FAILED(V_ERROR(&varresult)))
            {
                WARN("invoked function failed with error 0x%08x\n", V_ERROR(&varresult));
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
                TRACE("varresult value: ");
                dump_Variant(&varresult);

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
                (func_desc->lprgelemdescParam[0].u.paramdesc.wParamFlags & PARAMFLAG_FRETVAL) &&
                (pDispParams->cArgs != 0))
            {
                if (V_VT(pVarResult) == VT_DISPATCH)
                {
                    IDispatch *pDispatch = V_DISPATCH(pVarResult);
                    /* Note: not VariantClear; we still need the dispatch
                     * pointer to be valid */
                    VariantInit(pVarResult);
                    hres = IDispatch_Invoke(pDispatch, DISPID_VALUE, &IID_NULL,
                        GetSystemDefaultLCID(), INVOKE_PROPERTYGET,
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
            HeapFree(GetProcessHeap(), 0, buffer);
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
                   FIXME("IDispatch::Invoke failed with %08x. (Could be not a real error?)\n", hres);
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

        TRACE("-- 0x%08x\n", hres);
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

    /* not found, look for it in inherited interfaces */
    ITypeInfo2_GetTypeKind(iface, &type_kind);
    if(type_kind == TKIND_INTERFACE || type_kind == TKIND_DISPATCH) {
        if(This->impltypelist) {
            /* recursive search */
            ITypeInfo *pTInfo;
            hres = ITypeInfo_GetRefTypeInfo(iface, This->impltypelist->hRef, &pTInfo);
            if(SUCCEEDED(hres)){
                hres = ITypeInfo_Invoke(pTInfo,pIUnk,memid,wFlags,pDispParams,pVarResult,pExcepInfo,pArgErr);
                ITypeInfo_Release(pTInfo);
                return hres;
            }
            WARN("Could not search inherited interface!\n");
        }
    }
    ERR("did not find member id %d, flags 0x%x!\n", memid, wFlags);
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    const TLBFuncDesc *pFDesc;
    const TLBVarDesc *pVDesc;
    TRACE("(%p) memid %d Name(%p) DocString(%p)"
          " HelpContext(%p) HelpFile(%p)\n",
        This, memid, pBstrName, pBstrDocString, pdwHelpContext, pBstrHelpFile);
    if(memid==MEMBERID_NIL){ /* documentation for the typeinfo */
        if(pBstrName)
            *pBstrName=SysAllocString(This->Name);
        if(pBstrDocString)
            *pBstrDocString=SysAllocString(This->DocString);
        if(pdwHelpContext)
            *pdwHelpContext=This->dwHelpContext;
        if(pBstrHelpFile)
            *pBstrHelpFile=SysAllocString(This->DocString);/* FIXME */
        return S_OK;
    }else {/* for a member */
    for(pFDesc=This->funclist; pFDesc; pFDesc=pFDesc->next)
        if(pFDesc->funcdesc.memid==memid){
	  if(pBstrName)
	    *pBstrName = SysAllocString(pFDesc->Name);
	  if(pBstrDocString)
            *pBstrDocString=SysAllocString(pFDesc->HelpString);
	  if(pdwHelpContext)
            *pdwHelpContext=pFDesc->helpcontext;
	  return S_OK;
        }
    for(pVDesc=This->varlist; pVDesc; pVDesc=pVDesc->next)
        if(pVDesc->vardesc.memid==memid){
	    if(pBstrName)
	      *pBstrName = SysAllocString(pVDesc->Name);
	    if(pBstrDocString)
	      *pBstrDocString=SysAllocString(pVDesc->HelpString);
	    if(pdwHelpContext)
	      *pdwHelpContext=pVDesc->HelpContext;
	    return S_OK;
        }
    }

    if(This->impltypelist &&
       (This->TypeAttr.typekind==TKIND_INTERFACE || This->TypeAttr.typekind==TKIND_DISPATCH)) {
        /* recursive search */
        ITypeInfo *pTInfo;
        HRESULT result;
        result = ITypeInfo_GetRefTypeInfo(iface, This->impltypelist->hRef,
                                        &pTInfo);
        if(SUCCEEDED(result)) {
            result = ITypeInfo_GetDocumentation(pTInfo, memid, pBstrName,
                pBstrDocString, pdwHelpContext, pBstrHelpFile);
            ITypeInfo_Release(pTInfo);
            return result;
        }
        WARN("Could not search inherited interface!\n");
    }

    WARN("member %d not found\n", memid);
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    const TLBFuncDesc *pFDesc;

    TRACE("(%p)->(memid %x, %d, %p, %p, %p)\n", This, memid, invKind, pBstrDllName, pBstrName, pwOrdinal);

    if (pBstrDllName) *pBstrDllName = NULL;
    if (pBstrName) *pBstrName = NULL;
    if (pwOrdinal) *pwOrdinal = 0;

    if (This->TypeAttr.typekind != TKIND_MODULE)
        return TYPE_E_BADMODULEKIND;

    for(pFDesc=This->funclist; pFDesc; pFDesc=pFDesc->next)
        if(pFDesc->funcdesc.memid==memid){
	    dump_TypeInfo(This);
	    if (TRACE_ON(ole))
		dump_TLBFuncDescOne(pFDesc);

	    if (pBstrDllName)
		*pBstrDllName = SysAllocString(This->DllName);

	    if (HIWORD(pFDesc->Entry) && (pFDesc->Entry != (void*)-1)) {
		if (pBstrName)
		    *pBstrName = SysAllocString(pFDesc->Entry);
		if (pwOrdinal)
		    *pwOrdinal = -1;
		return S_OK;
	    }
	    if (pBstrName)
		*pBstrName = NULL;
	    if (pwOrdinal)
		*pwOrdinal = (DWORD)pFDesc->Entry;
	    return S_OK;
        }
    return TYPE_E_ELEMENTNOTFOUND;
}

/* internal function to make the inherited interfaces' methods appear
 * part of the interface */
static HRESULT ITypeInfoImpl_GetDispatchRefTypeInfo( ITypeInfo *iface,
    HREFTYPE *hRefType, ITypeInfo  **ppTInfo)
{
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    HRESULT hr;

    TRACE("%p, 0x%x\n", iface, *hRefType);

    if (This->impltypelist && (*hRefType & DISPATCH_HREF_MASK))
    {
        ITypeInfo *pSubTypeInfo;

        hr = ITypeInfo_GetRefTypeInfo(iface, This->impltypelist->hRef, &pSubTypeInfo);
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    HRESULT result = E_FAIL;

    if ((This->hreftype != -1) && (This->hreftype == hRefType))
    {
        *ppTInfo = (ITypeInfo *)&This->lpVtbl;
        ITypeInfo_AddRef(*ppTInfo);
        result = S_OK;
    }
    else if (hRefType == -1 &&
	(This->TypeAttr.typekind   == TKIND_DISPATCH) &&
	(This->TypeAttr.wTypeFlags &  TYPEFLAG_FDUAL))
    {
	  /* when we meet a DUAL dispinterface, we must create the interface
	  * version of it.
	  */
	  ITypeInfoImpl* pTypeInfoImpl = (ITypeInfoImpl*) ITypeInfo_Constructor();


	  /* the interface version contains the same information as the dispinterface
	   * copy the contents of the structs.
	   */
	  *pTypeInfoImpl = *This;
	  pTypeInfoImpl->ref = 0;

	  /* change the type to interface */
	  pTypeInfoImpl->TypeAttr.typekind = TKIND_INTERFACE;

	  *ppTInfo = (ITypeInfo*) pTypeInfoImpl;

	  /* we use data structures from This, so we need to keep a reference
	   * to it to stop it being destroyed and signal to the new instance to
	   * not free its data structures when it is destroyed */
	  pTypeInfoImpl->no_free_data = TRUE;
	  pTypeInfoImpl->next = This;
	  ITypeInfo_AddRef((ITypeInfo*) This);

	  ITypeInfo_AddRef(*ppTInfo);

	  result = S_OK;

    } else if ((hRefType != -1) && (hRefType & DISPATCH_HREF_MASK) &&
        (This->TypeAttr.typekind   == TKIND_DISPATCH) &&
	(This->TypeAttr.wTypeFlags &  TYPEFLAG_FDUAL))
    {
        HREFTYPE href_dispatch = hRefType;
        result = ITypeInfoImpl_GetDispatchRefTypeInfo((ITypeInfo *)iface, &href_dispatch, ppTInfo);
    } else {
        TLBRefType *ref_type;
        LIST_FOR_EACH_ENTRY(ref_type, &This->pTypeLib->ref_list, TLBRefType, entry)
        {
            if(ref_type->reference == hRefType)
                break;
        }
        if(&ref_type->entry == &This->pTypeLib->ref_list)
        {
            FIXME("Can't find pRefType for ref %x\n", hRefType);
            goto end;
        }
        if(hRefType != -1) {
            ITypeLib *pTLib = NULL;

            if(ref_type->pImpTLInfo == TLB_REF_INTERNAL) {
	        UINT Index;
		result = ITypeInfo_GetContainingTypeLib(iface, &pTLib, &Index);
	    } else {
                if(ref_type->pImpTLInfo->pImpTypeLib) {
		    TRACE("typeinfo in imported typelib that is already loaded\n");
                    pTLib = (ITypeLib*)ref_type->pImpTLInfo->pImpTypeLib;
		    ITypeLib2_AddRef(pTLib);
		    result = S_OK;
		} else {
		    TRACE("typeinfo in imported typelib that isn't already loaded\n");
                    result = LoadRegTypeLib( &ref_type->pImpTLInfo->guid,
                                             ref_type->pImpTLInfo->wVersionMajor,
                                             ref_type->pImpTLInfo->wVersionMinor,
                                             ref_type->pImpTLInfo->lcid,
					     &pTLib);

                    if(FAILED(result)) {
                        BSTR libnam=SysAllocString(ref_type->pImpTLInfo->name);
			result=LoadTypeLib(libnam, &pTLib);
			SysFreeString(libnam);
		    }
		    if(SUCCEEDED(result)) {
                        ref_type->pImpTLInfo->pImpTypeLib = (ITypeLibImpl*)pTLib;
			ITypeLib2_AddRef(pTLib);
		    }
		}
	    }
	    if(SUCCEEDED(result)) {
                if(ref_type->index == TLB_REF_USE_GUID)
		    result = ITypeLib2_GetTypeInfoOfGuid(pTLib,
                                                         &ref_type->guid,
							 ppTInfo);
		else
                    result = ITypeLib2_GetTypeInfo(pTLib, ref_type->index,
						   ppTInfo);
	    }
	    if (pTLib != NULL)
		ITypeLib2_Release(pTLib);
	}
    }

end:
    TRACE("(%p) hreftype 0x%04x loaded %s (%p)\n", This, hRefType,
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    HRESULT hr;
    BSTR dll, entry;
    WORD ordinal;
    HMODULE module;

    TRACE("(%p)->(0x%x, 0x%x, %p)\n", This, memid, invKind, ppv);

    hr = ITypeInfo_GetDllEntry(iface, memid, invKind, &dll, &entry, &ordinal);
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
        entryA = HeapAlloc(GetProcessHeap(), 0, len);
        WideCharToMultiByte(CP_ACP, 0, entry, -1, entryA, len, NULL, NULL);

        *ppv = GetProcAddress(module, entryA);
        if (!*ppv)
            ERR("function not found %s\n", debugstr_a(entryA));

        HeapFree(GetProcessHeap(), 0, entryA);
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    HRESULT hr;
    TYPEATTR *pTA;

    TRACE("(%p)->(%p, %s, %p)\n", This, pOuterUnk, debugstr_guid(riid), ppvObj);

    *ppvObj = NULL;

    if(pOuterUnk)
    {
        WARN("Not able to aggregate\n");
        return CLASS_E_NOAGGREGATION;
    }

    hr = ITypeInfo_GetTypeAttr(iface, &pTA);
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
        TRACE("GetActiveObject rets %08x\n", hr);
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
    ITypeInfo_ReleaseTypeAttr(iface, pTA);
    return hr;
}

/* ITypeInfo::GetMops
 *
 * Retrieves marshalling information.
 */
static HRESULT WINAPI ITypeInfo_fnGetMops( ITypeInfo2 *iface, MEMBERID memid,
				BSTR  *pBstrMops)
{
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    FIXME("(%p) stub!\n", This);
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    
    /* If a pointer is null, we simply ignore it, the ATL in particular passes pIndex as 0 */
    if (pIndex) {
      *pIndex=This->index;
      TRACE("returning pIndex=%d\n", *pIndex);
    }
    
    if (ppTLib) {
      *ppTLib=(LPTYPELIB )(This->pTypeLib);
      ITypeLib2_AddRef(*ppTLib);
      TRACE("returning ppTLib=%p\n", *ppTLib);
    }
    
    return S_OK;
}

/* ITypeInfo::ReleaseTypeAttr
 *
 * Releases a TYPEATTR previously returned by GetTypeAttr.
 *
 */
static void WINAPI ITypeInfo_fnReleaseTypeAttr( ITypeInfo2 *iface,
        TYPEATTR* pTypeAttr)
{
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    TRACE("(%p)->(%p)\n", This, pTypeAttr);
    HeapFree(GetProcessHeap(), 0, pTypeAttr);
}

/* ITypeInfo::ReleaseFuncDesc
 *
 * Releases a FUNCDESC previously returned by GetFuncDesc. *
 */
static void WINAPI ITypeInfo_fnReleaseFuncDesc(
	ITypeInfo2 *iface,
        FUNCDESC *pFuncDesc)
{
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    TRACE("(%p)->(%p)\n", This, pVarDesc);

    TLB_FreeElemDesc(&pVarDesc->elemdescVar);
    if (pVarDesc->varkind == VAR_CONST)
        VariantClear(pVarDesc->u.lpvarValue);
    SysFreeString((BSTR)pVarDesc);
}

/* ITypeInfo2::GetTypeKind
 *
 * Returns the TYPEKIND enumeration quickly, without doing any allocations.
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetTypeKind( ITypeInfo2 * iface,
    TYPEKIND *pTypeKind)
{
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    *pTypeKind=This->TypeAttr.typekind;
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    *pTypeFlags=This->TypeAttr.wTypeFlags;
    TRACE("(%p) flags 0x%x\n", This,*pTypeFlags);
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    const TLBFuncDesc *pFuncInfo;
    int i;
    HRESULT result;

    for(i = 0, pFuncInfo = This->funclist; pFuncInfo; i++, pFuncInfo=pFuncInfo->next)
        if(memid == pFuncInfo->funcdesc.memid && (invKind & pFuncInfo->funcdesc.invkind))
            break;
    if(pFuncInfo) {
        *pFuncIndex = i;
        result = S_OK;
    } else
        result = TYPE_E_ELEMENTNOTFOUND;

    TRACE("(%p) memid 0x%08x invKind 0x%04x -> %s\n", This,
          memid, invKind, SUCCEEDED(result) ? "SUCCESS" : "FAILED");
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    TLBVarDesc *pVarInfo;
    int i;
    HRESULT result;
    for(i=0, pVarInfo=This->varlist; pVarInfo &&
            memid != pVarInfo->vardesc.memid; i++, pVarInfo=pVarInfo->next)
        ;
    if(pVarInfo) {
        *pVarIndex = i;
        result = S_OK;
    } else
        result = TYPE_E_ELEMENTNOTFOUND;

    TRACE("(%p) memid 0x%08x -> %s\n", This,
          memid, SUCCEEDED(result) ? "SUCCESS" : "FAILED");
    return result;
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    TLBCustData *pCData;

    for(pCData=This->pCustData; pCData; pCData = pCData->next)
        if( IsEqualIID(guid, &pCData->guid)) break;

    TRACE("(%p) guid %s %s found!x)\n", This, debugstr_guid(guid), pCData? "" : "NOT");

    if(pCData)
    {
        VariantInit( pVarVal);
        VariantCopy( pVarVal, &pCData->data);
        return S_OK;
    }
    return E_INVALIDARG;  /* FIXME: correct? */
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    TLBCustData *pCData=NULL;
    TLBFuncDesc * pFDesc;
    UINT i;
    for(i=0, pFDesc=This->funclist; i!=index && pFDesc; i++,
            pFDesc=pFDesc->next);

    if(pFDesc)
        for(pCData=pFDesc->pCustData; pCData; pCData = pCData->next)
            if( IsEqualIID(guid, &pCData->guid)) break;

    TRACE("(%p) guid %s %s found!x)\n", This, debugstr_guid(guid), pCData? "" : "NOT");

    if(pCData){
        VariantInit( pVarVal);
        VariantCopy( pVarVal, &pCData->data);
        return S_OK;
    }
    return E_INVALIDARG;  /* FIXME: correct? */
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    TLBCustData *pCData=NULL;
    TLBFuncDesc * pFDesc;
    UINT i;

    for(i=0, pFDesc=This->funclist; i!=indexFunc && pFDesc; i++,pFDesc=pFDesc->next);

    if(pFDesc && indexParam<pFDesc->funcdesc.cParams)
        for(pCData=pFDesc->pParamDesc[indexParam].pCustData; pCData;
                pCData = pCData->next)
            if( IsEqualIID(guid, &pCData->guid)) break;

    TRACE("(%p) guid %s %s found!x)\n", This, debugstr_guid(guid), pCData? "" : "NOT");

    if(pCData)
    {
        VariantInit( pVarVal);
        VariantCopy( pVarVal, &pCData->data);
        return S_OK;
    }
    return E_INVALIDARG;  /* FIXME: correct? */
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    TLBCustData *pCData=NULL;
    TLBVarDesc * pVDesc;
    UINT i;

    for(i=0, pVDesc=This->varlist; i!=index && pVDesc; i++, pVDesc=pVDesc->next);

    if(pVDesc)
    {
      for(pCData=pVDesc->pCustData; pCData; pCData = pCData->next)
      {
        if( IsEqualIID(guid, &pCData->guid)) break;
      }
    }

    TRACE("(%p) guid %s %s found!x)\n", This, debugstr_guid(guid), pCData? "" : "NOT");

    if(pCData)
    {
        VariantInit( pVarVal);
        VariantCopy( pVarVal, &pCData->data);
        return S_OK;
    }
    return E_INVALIDARG;  /* FIXME: correct? */
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    TLBCustData *pCData=NULL;
    TLBImplType * pRDesc;
    UINT i;

    for(i=0, pRDesc=This->impltypelist; i!=index && pRDesc; i++, pRDesc=pRDesc->next);

    if(pRDesc)
    {
      for(pCData=pRDesc->pCustData; pCData; pCData = pCData->next)
      {
        if( IsEqualIID(guid, &pCData->guid)) break;
      }
    }

    TRACE("(%p) guid %s %s found!x)\n", This, debugstr_guid(guid), pCData? "" : "NOT");

    if(pCData)
    {
        VariantInit( pVarVal);
        VariantCopy( pVarVal, &pCData->data);
        return S_OK;
    }
    return E_INVALIDARG;  /* FIXME: correct? */
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    const TLBFuncDesc *pFDesc;
    const TLBVarDesc *pVDesc;
    TRACE("(%p) memid %d lcid(0x%x)  HelpString(%p) "
          "HelpStringContext(%p) HelpStringDll(%p)\n",
          This, memid, lcid, pbstrHelpString, pdwHelpStringContext,
          pbstrHelpStringDll );
    /* the help string should be obtained from the helpstringdll,
     * using the _DLLGetDocumentation function, based on the supplied
     * lcid. Nice to do sometime...
     */
    if(memid==MEMBERID_NIL){ /* documentation for the typeinfo */
        if(pbstrHelpString)
            *pbstrHelpString=SysAllocString(This->Name);
        if(pdwHelpStringContext)
            *pdwHelpStringContext=This->dwHelpStringContext;
        if(pbstrHelpStringDll)
            *pbstrHelpStringDll=
                SysAllocString(This->pTypeLib->HelpStringDll);/* FIXME */
        return S_OK;
    }else {/* for a member */
    for(pFDesc=This->funclist; pFDesc; pFDesc=pFDesc->next)
        if(pFDesc->funcdesc.memid==memid){
             if(pbstrHelpString)
                *pbstrHelpString=SysAllocString(pFDesc->HelpString);
            if(pdwHelpStringContext)
                *pdwHelpStringContext=pFDesc->HelpStringContext;
            if(pbstrHelpStringDll)
                *pbstrHelpStringDll=
                    SysAllocString(This->pTypeLib->HelpStringDll);/* FIXME */
        return S_OK;
    }
    for(pVDesc=This->varlist; pVDesc; pVDesc=pVDesc->next)
        if(pVDesc->vardesc.memid==memid){
             if(pbstrHelpString)
                *pbstrHelpString=SysAllocString(pVDesc->HelpString);
            if(pdwHelpStringContext)
                *pdwHelpStringContext=pVDesc->HelpStringContext;
            if(pbstrHelpStringDll)
                *pbstrHelpStringDll=
                    SysAllocString(This->pTypeLib->HelpStringDll);/* FIXME */
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    TLBCustData *pCData;
    int i;

    TRACE("(%p) returning %d items\n", This, This->ctCustData);

    pCustData->prgCustData = TLB_Alloc(This->ctCustData * sizeof(CUSTDATAITEM));
    if(pCustData->prgCustData ){
        pCustData->cCustData=This->ctCustData;
        for(i=0, pCData=This->pCustData; pCData; i++, pCData = pCData->next){
            pCustData->prgCustData[i].guid=pCData->guid;
            VariantCopy(& pCustData->prgCustData[i].varValue, & pCData->data);
        }
    }else{
        ERR(" OUT OF MEMORY!\n");
        return E_OUTOFMEMORY;
    }
    return S_OK;
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    TLBCustData *pCData;
    TLBFuncDesc * pFDesc;
    UINT i;
    TRACE("(%p) index %d\n", This, index);
    for(i=0, pFDesc=This->funclist; i!=index && pFDesc; i++,
            pFDesc=pFDesc->next)
        ;
    if(pFDesc){
        pCustData->prgCustData =
            TLB_Alloc(pFDesc->ctCustData * sizeof(CUSTDATAITEM));
        if(pCustData->prgCustData ){
            pCustData->cCustData=pFDesc->ctCustData;
            for(i=0, pCData=pFDesc->pCustData; pCData; i++,
                    pCData = pCData->next){
                pCustData->prgCustData[i].guid=pCData->guid;
                VariantCopy(& pCustData->prgCustData[i].varValue,
                        & pCData->data);
            }
        }else{
            ERR(" OUT OF MEMORY!\n");
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
    return TYPE_E_ELEMENTNOTFOUND;
}

/* ITypeInfo2::GetAllParamCustData
 *
 * Gets all custom data items for the Functions
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllParamCustData( ITypeInfo2 * iface,
    UINT indexFunc, UINT indexParam, CUSTDATA *pCustData)
{
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    TLBCustData *pCData=NULL;
    TLBFuncDesc * pFDesc;
    UINT i;
    TRACE("(%p) index %d\n", This, indexFunc);
    for(i=0, pFDesc=This->funclist; i!=indexFunc && pFDesc; i++,
            pFDesc=pFDesc->next)
        ;
    if(pFDesc && indexParam<pFDesc->funcdesc.cParams){
        pCustData->prgCustData =
            TLB_Alloc(pFDesc->pParamDesc[indexParam].ctCustData *
                    sizeof(CUSTDATAITEM));
        if(pCustData->prgCustData ){
            pCustData->cCustData=pFDesc->pParamDesc[indexParam].ctCustData;
            for(i=0, pCData=pFDesc->pParamDesc[indexParam].pCustData;
                    pCData; i++, pCData = pCData->next){
                pCustData->prgCustData[i].guid=pCData->guid;
                VariantCopy(& pCustData->prgCustData[i].varValue,
                        & pCData->data);
            }
        }else{
            ERR(" OUT OF MEMORY!\n");
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
    return TYPE_E_ELEMENTNOTFOUND;
}

/* ITypeInfo2::GetAllVarCustData
 *
 * Gets all custom data items for the specified Variable
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllVarCustData( ITypeInfo2 * iface,
    UINT index, CUSTDATA *pCustData)
{
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    TLBCustData *pCData;
    TLBVarDesc * pVDesc;
    UINT i;
    TRACE("(%p) index %d\n", This, index);
    for(i=0, pVDesc=This->varlist; i!=index && pVDesc; i++,
            pVDesc=pVDesc->next)
        ;
    if(pVDesc){
        pCustData->prgCustData =
            TLB_Alloc(pVDesc->ctCustData * sizeof(CUSTDATAITEM));
        if(pCustData->prgCustData ){
            pCustData->cCustData=pVDesc->ctCustData;
            for(i=0, pCData=pVDesc->pCustData; pCData; i++,
                    pCData = pCData->next){
                pCustData->prgCustData[i].guid=pCData->guid;
                VariantCopy(& pCustData->prgCustData[i].varValue,
                        & pCData->data);
            }
        }else{
            ERR(" OUT OF MEMORY!\n");
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
    return TYPE_E_ELEMENTNOTFOUND;
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
    ITypeInfoImpl *This = (ITypeInfoImpl *)iface;
    TLBCustData *pCData;
    TLBImplType * pRDesc;
    UINT i;
    TRACE("(%p) index %d\n", This, index);
    for(i=0, pRDesc=This->impltypelist; i!=index && pRDesc; i++,
            pRDesc=pRDesc->next)
        ;
    if(pRDesc){
        pCustData->prgCustData =
            TLB_Alloc(pRDesc->ctCustData * sizeof(CUSTDATAITEM));
        if(pCustData->prgCustData ){
            pCustData->cCustData=pRDesc->ctCustData;
            for(i=0, pCData=pRDesc->pCustData; pCData; i++,
                    pCData = pCData->next){
                pCustData->prgCustData[i].guid=pCData->guid;
                VariantCopy(& pCustData->prgCustData[i].varValue,
                        & pCData->data);
            }
        }else{
            ERR(" OUT OF MEMORY!\n");
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
    return TYPE_E_ELEMENTNOTFOUND;
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
    TLBFuncDesc **ppFuncDesc;
    TLBRefType *ref;

    TRACE("\n");
    pTypeLibImpl = TypeLibImpl_Constructor();
    if (!pTypeLibImpl) return E_FAIL;

    pTIIface = (ITypeInfoImpl*)ITypeInfo_Constructor();
    pTIIface->pTypeLib = pTypeLibImpl;
    pTIIface->index = 0;
    pTIIface->Name = NULL;
    pTIIface->dwHelpContext = -1;
    memset(&pTIIface->TypeAttr.guid, 0, sizeof(GUID));
    pTIIface->TypeAttr.lcid = lcid;
    pTIIface->TypeAttr.typekind = TKIND_INTERFACE;
    pTIIface->TypeAttr.wMajorVerNum = 0;
    pTIIface->TypeAttr.wMinorVerNum = 0;
    pTIIface->TypeAttr.cbAlignment = 2;
    pTIIface->TypeAttr.cbSizeInstance = -1;
    pTIIface->TypeAttr.cbSizeVft = -1;
    pTIIface->TypeAttr.cFuncs = 0;
    pTIIface->TypeAttr.cImplTypes = 0;
    pTIIface->TypeAttr.cVars = 0;
    pTIIface->TypeAttr.wTypeFlags = 0;

    ppFuncDesc = &pTIIface->funclist;
    for(func = 0; func < pidata->cMembers; func++) {
        METHODDATA *md = pidata->pmethdata + func;
        *ppFuncDesc = HeapAlloc(GetProcessHeap(), 0, sizeof(**ppFuncDesc));
        (*ppFuncDesc)->Name = SysAllocString(md->szName);
        (*ppFuncDesc)->funcdesc.memid = md->dispid;
        (*ppFuncDesc)->funcdesc.lprgscode = NULL;
        (*ppFuncDesc)->funcdesc.funckind = FUNC_VIRTUAL;
        (*ppFuncDesc)->funcdesc.invkind = md->wFlags;
        (*ppFuncDesc)->funcdesc.callconv = md->cc;
        (*ppFuncDesc)->funcdesc.cParams = md->cArgs;
        (*ppFuncDesc)->funcdesc.cParamsOpt = 0;
        (*ppFuncDesc)->funcdesc.oVft = md->iMeth << 2;
        (*ppFuncDesc)->funcdesc.cScodes = 0;
        (*ppFuncDesc)->funcdesc.wFuncFlags = 0;
        (*ppFuncDesc)->funcdesc.elemdescFunc.tdesc.vt = md->vtReturn;
        (*ppFuncDesc)->funcdesc.elemdescFunc.u.paramdesc.wParamFlags = PARAMFLAG_NONE;
        (*ppFuncDesc)->funcdesc.elemdescFunc.u.paramdesc.pparamdescex = NULL;
        (*ppFuncDesc)->funcdesc.lprgelemdescParam = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                              md->cArgs * sizeof(ELEMDESC));
        (*ppFuncDesc)->pParamDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                              md->cArgs * sizeof(TLBParDesc));
        for(param = 0; param < md->cArgs; param++) {
            (*ppFuncDesc)->funcdesc.lprgelemdescParam[param].tdesc.vt = md->ppdata[param].vt;
            (*ppFuncDesc)->pParamDesc[param].Name = SysAllocString(md->ppdata[param].szName);
        }
        (*ppFuncDesc)->helpcontext = 0;
        (*ppFuncDesc)->HelpStringContext = 0;
        (*ppFuncDesc)->HelpString = NULL;
        (*ppFuncDesc)->Entry = NULL;
        (*ppFuncDesc)->ctCustData = 0;
        (*ppFuncDesc)->pCustData = NULL;
        (*ppFuncDesc)->next = NULL;
        pTIIface->TypeAttr.cFuncs++;
        ppFuncDesc = &(*ppFuncDesc)->next;
    }

    dump_TypeInfo(pTIIface);

    pTypeLibImpl->pTypeInfo = pTIIface;
    pTypeLibImpl->TypeInfoCount++;

    pTIClass = (ITypeInfoImpl*)ITypeInfo_Constructor();
    pTIClass->pTypeLib = pTypeLibImpl;
    pTIClass->index = 1;
    pTIClass->Name = NULL;
    pTIClass->dwHelpContext = -1;
    memset(&pTIClass->TypeAttr.guid, 0, sizeof(GUID));
    pTIClass->TypeAttr.lcid = lcid;
    pTIClass->TypeAttr.typekind = TKIND_COCLASS;
    pTIClass->TypeAttr.wMajorVerNum = 0;
    pTIClass->TypeAttr.wMinorVerNum = 0;
    pTIClass->TypeAttr.cbAlignment = 2;
    pTIClass->TypeAttr.cbSizeInstance = -1;
    pTIClass->TypeAttr.cbSizeVft = -1;
    pTIClass->TypeAttr.cFuncs = 0;
    pTIClass->TypeAttr.cImplTypes = 1;
    pTIClass->TypeAttr.cVars = 0;
    pTIClass->TypeAttr.wTypeFlags = 0;

    pTIClass->impltypelist = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pTIClass->impltypelist));
    pTIClass->impltypelist->hRef = 0;

    ref = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ref));
    ref->index = 0;
    ref->reference = 0;
    ref->pImpTLInfo = TLB_REF_INTERNAL;
    list_add_head(&pTypeLibImpl->ref_list, &ref->entry);

    dump_TypeInfo(pTIClass);

    pTIIface->next = pTIClass;
    pTypeLibImpl->TypeInfoCount++;

    *pptinfo = (ITypeInfo*)pTIClass;

    ITypeInfo_AddRef(*pptinfo);
    ITypeLib_Release((ITypeLib *)&pTypeLibImpl->lpVtbl);

    return S_OK;

}

static HRESULT WINAPI ITypeComp_fnQueryInterface(ITypeComp * iface, REFIID riid, LPVOID * ppv)
{
    ITypeInfoImpl *This = info_impl_from_ITypeComp(iface);

    return ITypeInfo_QueryInterface((ITypeInfo *)This, riid, ppv);
}

static ULONG WINAPI ITypeComp_fnAddRef(ITypeComp * iface)
{
    ITypeInfoImpl *This = info_impl_from_ITypeComp(iface);

    return ITypeInfo_AddRef((ITypeInfo *)This);
}

static ULONG WINAPI ITypeComp_fnRelease(ITypeComp * iface)
{
    ITypeInfoImpl *This = info_impl_from_ITypeComp(iface);

    return ITypeInfo_Release((ITypeInfo *)This);
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

    TRACE("(%s, %x, 0x%x, %p, %p, %p)\n", debugstr_w(szName), lHash, wFlags, ppTInfo, pDescKind, pBindPtr);

    *pDescKind = DESCKIND_NONE;
    pBindPtr->lpfuncdesc = NULL;
    *ppTInfo = NULL;

    for(pFDesc = This->funclist; pFDesc; pFDesc = pFDesc->next)
        if (!strcmpiW(pFDesc->Name, szName)) {
            if (!wFlags || (pFDesc->funcdesc.invkind & wFlags))
                break;
            else
                /* name found, but wrong flags */
                hr = TYPE_E_TYPEMISMATCH;
        }

    if (pFDesc)
    {
        HRESULT hr = TLB_AllocAndInitFuncDesc(
            &pFDesc->funcdesc,
            &pBindPtr->lpfuncdesc,
            This->TypeAttr.typekind == TKIND_DISPATCH);
        if (FAILED(hr))
            return hr;
        *pDescKind = DESCKIND_FUNCDESC;
        *ppTInfo = (ITypeInfo *)&This->lpVtbl;
        ITypeInfo_AddRef(*ppTInfo);
        return S_OK;
    } else {
        for(pVDesc = This->varlist; pVDesc; pVDesc = pVDesc->next) {
            if (!strcmpiW(pVDesc->Name, szName)) {
                HRESULT hr = TLB_AllocAndInitVarDesc(&pVDesc->vardesc, &pBindPtr->lpvardesc);
                if (FAILED(hr))
                    return hr;
                *pDescKind = DESCKIND_VARDESC;
                *ppTInfo = (ITypeInfo *)&This->lpVtbl;
                ITypeInfo_AddRef(*ppTInfo);
                return S_OK;
            }
        }
    }
    /* FIXME: search each inherited interface, not just the first */
    if (hr == DISP_E_MEMBERNOTFOUND && This->impltypelist) {
        /* recursive search */
        ITypeInfo *pTInfo;
        ITypeComp *pTComp;
        HRESULT hr;
        hr=ITypeInfo_GetRefTypeInfo((ITypeInfo *)&This->lpVtbl, This->impltypelist->hRef, &pTInfo);
        if (SUCCEEDED(hr))
        {
            hr = ITypeInfo_GetTypeComp(pTInfo,&pTComp);
            ITypeInfo_Release(pTInfo);
        }
        if (SUCCEEDED(hr))
        {
            hr = ITypeComp_Bind(pTComp, szName, lHash, wFlags, ppTInfo, pDescKind, pBindPtr);
            ITypeComp_Release(pTComp);
            return hr;
        }
        WARN("Could not search inherited interface!\n");
    }
    WARN("did not find member with name %s, flags 0x%x!\n", debugstr_w(szName), wFlags);
    return hr;
}

static HRESULT WINAPI ITypeComp_fnBindType(
    ITypeComp * iface,
    OLECHAR * szName,
    ULONG lHash,
    ITypeInfo ** ppTInfo,
    ITypeComp ** ppTComp)
{
    TRACE("(%s, %x, %p, %p)\n", debugstr_w(szName), lHash, ppTInfo, ppTComp);

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
