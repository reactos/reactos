/*
 *	TYPELIB
 *
 *	Copyright 1997	Marcus Meissner
 *		      1999  Rein Klazes
 *		      2000  Francois Jacques
 *		      2001  Huw D M Davies for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * --------------------------------------------------------------------------------------
 * Known problems (2000, Francois Jacques)
 *
 * - Tested using OLEVIEW (Platform SDK tool) only.
 *
 * - dual interface dispinterfaces. vtable-interface ITypeInfo instances are
 *   creating by doing a straight copy of the dispinterface instance and just changing
 *   its typekind. Pointed structures aren't copied - only the address of the pointers.
 *   So when you release the dispinterface, you delete the vtable-interface structures
 *   as well... fortunately, clean up of structures is not implemented.
 *
 * - locale stuff is partially implemented but hasn't been tested.
 *
 * - typelib file is still read in its entirety, but it is released now.
 * - some garbage is read from function names on some very rare occasions.
 *
 * --------------------------------------------------------------------------------------
 *  Known problems left from previous implementation (1999, Rein Klazes) :
 *
 * -. Data structures are straightforward, but slow for look-ups.
 * -. (related) nothing is hashed
 * -. there are a number of stubs in ITypeLib and ITypeInfo interfaces. Most
 *      of them I don't know yet how to implement them.
 * -. Most error return values are just guessed not checked with windows
 *      behaviour.
 * -. didn't bother with a c++ interface
 * -. lousy fatal error handling
 * -. some methods just return pointers to internal data structures, this is
 *      partly laziness, partly I want to check how windows does it.
 *
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winuser.h"

#include "wine/unicode.h"
#include "objbase.h"
#include "heap.h"
#include "ole2disp.h"
#include "typelib.h"
#include "wine/debug.h"
#include "variant.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);
WINE_DECLARE_DEBUG_CHANNEL(typelib);

/* The OLE Automation ProxyStub Interface Class (aka Typelib Marshaler) */
const GUID CLSID_PSOAInterface = { 0x00020424, 0, 0, { 0xC0, 0, 0, 0, 0, 0, 0, 0x46 } };

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


/****************************************************************************
 *		QueryPathOfRegTypeLib	[OLEAUT32.164]
 * RETURNS
 *	path of typelib
 */
HRESULT WINAPI
QueryPathOfRegTypeLib(
	REFGUID guid,	/* [in] referenced guid */
	WORD wMaj,	/* [in] major version */
	WORD wMin,	/* [in] minor version */
	LCID lcid,	/* [in] locale id */
	LPBSTR path )	/* [out] path of typelib */
{
    /* don't need to ZeroMemory those arrays since sprintf and RegQueryValue add
       string termination character on output strings */

    HRESULT hr        = E_FAIL;

    LCID    myLCID    = lcid;

    char    szXGUID[80];
    char    szTypeLibKey[100];
    char    szPath[MAX_PATH];
    DWORD   dwPathLen = sizeof(szPath);

    if ( !HIWORD(guid) )
    {
        sprintf(szXGUID,
            "<guid 0x%08lx>",
            (DWORD) guid);

        FIXME("(%s,%d,%d,0x%04lx,%p),stub!\n", szXGUID, wMaj, wMin, (DWORD)lcid, path);
        return E_FAIL;
    }

    while (hr != S_OK)
    {
        sprintf(szTypeLibKey,
            "SOFTWARE\\Classes\\Typelib\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\%d.%d\\%lx\\win32",
            guid->Data1,    guid->Data2,    guid->Data3,
            guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
            guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7],
            wMaj,
            wMin,
            myLCID);

        if (RegQueryValueA(HKEY_LOCAL_MACHINE, szTypeLibKey, szPath, &dwPathLen))
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
            DWORD len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szPath, dwPathLen, NULL, 0 );
            BSTR bstrPath = SysAllocStringLen(NULL,len);

            MultiByteToWideChar(CP_ACP,
                                MB_PRECOMPOSED,
                                szPath,
                                dwPathLen,
                                bstrPath,
                                len);
           *path = bstrPath;
           hr = S_OK;
        }
    }

    if (hr != S_OK)
		TRACE_(typelib)("%s not found\n", szTypeLibKey);

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
 * Loads and registers a type library
 * NOTES
 *    Docs: OLECHAR FAR* szFile
 *    Docs: iTypeLib FAR* FAR* pptLib
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
int TLB_ReadTypeLib(LPCWSTR file, INT index, ITypeLib2 **ppTypelib);

HRESULT WINAPI LoadTypeLib(
    const OLECHAR *szFile,/* [in] Name of file to load from */
    ITypeLib * *pptLib)   /* [out] Pointer to pointer to loaded type library */
{
    TRACE("\n");
    return LoadTypeLibEx(szFile, REGKIND_DEFAULT, pptLib);
}

/******************************************************************************
 *		LoadTypeLibEx	[OLEAUT32.183]
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
    WCHAR szPath[MAX_PATH+1], szFileCopy[MAX_PATH+1];
    WCHAR *pIndexStr;
    HRESULT res;
    INT index = 1;

    TRACE("(%s,%d,%p)\n",debugstr_w(szFile), regkind, pptLib);

    *pptLib = NULL;
    if(!SearchPathW(NULL,szFile,NULL,sizeof(szPath)/sizeof(WCHAR),szPath,
		    NULL)) {

        /* Look for a trailing '\\' followed by an index */
        pIndexStr = strrchrW(szFile, '\\');
	if(pIndexStr && pIndexStr != szFile && *++pIndexStr != '\0') {
	    index = atoiW(pIndexStr);
	    memcpy(szFileCopy, szFile,
		   (pIndexStr - szFile - 1) * sizeof(WCHAR));
	    szFileCopy[pIndexStr - szFile - 1] = '\0';
	    if(!SearchPathW(NULL,szFileCopy,NULL,sizeof(szPath)/sizeof(WCHAR),
			    szPath,NULL))
	        return TYPE_E_CANTLOADLIBRARY;
	    if (GetFileAttributesW(szFileCopy) & FILE_ATTRIBUTE_DIRECTORY)
		return TYPE_E_CANTLOADLIBRARY;
	} else {
	    WCHAR tstpath[260];
	    WCHAR stdole32tlb[] = { 's','t','d','o','l','e','3','2','.','t','l','b',0 };
	    int i;

	    lstrcpyW(tstpath,szFile);
	    CharLowerW(tstpath);
	    for (i=0;i<strlenW(tstpath);i++) {
		if (tstpath[i] == 's') {
		    if (!strcmpW(tstpath+i,stdole32tlb)) {
		    	MESSAGE("\n");
		    	MESSAGE("**************************************************************************\n");
		    	MESSAGE("You must copy a 'stdole32.tlb' file to your Windows\\System directory!\n");
		    	MESSAGE("You can get one from a Windows installation, or look for the DCOM95 package\n");
		    	MESSAGE("on the Microsoft Download Pages.\n");
		    	MESSAGE("A free download link is on http://sourceforge.net/projects/wine/, look for dcom95.exe.\n");
		    	MESSAGE("**************************************************************************\n");
			break;
		    }
		}
	    }
	    FIXME("Wanted to load %s as typelib, but file was not found.\n",debugstr_w(szFile));
	    return TYPE_E_CANTLOADLIBRARY;
	}
    }

    TRACE("File %s index %d\n", debugstr_w(szPath), index);

    res = TLB_ReadTypeLib(szPath, index, (ITypeLib2**)pptLib);

    if (SUCCEEDED(res))
        switch(regkind)
        {
            case REGKIND_DEFAULT:
                /* don't register typelibs supplied with full path. Experimentation confirms the following */
                if ((!szFile) ||
                    ((szFile[0] == '\\') && (szFile[1] == '\\')) ||
                    (szFile[0] && (szFile[1] == ':'))) break;
                /* else fall-through */

            case REGKIND_REGISTER:
                if (!SUCCEEDED(res = RegisterTypeLib(*pptLib, (LPOLESTR)szPath, NULL)))
                {
                    IUnknown_Release(*pptLib);
                    *pptLib = 0;
                }
                break;
            case REGKIND_NONE:
                break;
        }

    TRACE(" returns %08lx\n",res);
    return res;
}

/******************************************************************************
 *		LoadRegTypeLib	[OLEAUT32.162]
 */
HRESULT WINAPI LoadRegTypeLib(
	REFGUID rguid,		/* [in] referenced guid */
	WORD wVerMajor,		/* [in] major version */
	WORD wVerMinor,		/* [in] minor version */
	LCID lcid,		/* [in] locale id */
	ITypeLib **ppTLib)	/* [out] path of typelib */
{
    BSTR bstr=NULL;
    HRESULT res=QueryPathOfRegTypeLib( rguid, wVerMajor, wVerMinor, lcid, &bstr);

    if(SUCCEEDED(res))
    {
        res= LoadTypeLib(bstr, ppTLib);
        SysFreeString(bstr);
    }

    TRACE("(IID: %s) load %s (%p)\n",debugstr_guid(rguid), SUCCEEDED(res)? "SUCCESS":"FAILED", *ppTLib);

    return res;
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
HRESULT WINAPI RegisterTypeLib(
     ITypeLib * ptlib,     /* [in] Pointer to the library*/
     OLECHAR * szFullPath, /* [in] full Path of the library*/
     OLECHAR * szHelpDir)  /* [in] dir to the helpfile for the library,
							 may be NULL*/
{
    HRESULT res;
    TLIBATTR *attr;
    OLECHAR guid[80];
    LPSTR guidA;
    CHAR keyName[120];
    CHAR tmp[MAX_PATH];
    HKEY key, subKey;
    UINT types, tidx;
    TYPEKIND kind;
    DWORD disposition;
    static const char *PSOA = "{00020424-0000-0000-C000-000000000046}";

    if (ptlib == NULL || szFullPath == NULL)
        return E_INVALIDARG;

    if (!SUCCEEDED(ITypeLib_GetLibAttr(ptlib, &attr)))
        return E_FAIL;

    StringFromGUID2(&attr->guid, guid, 80);
    guidA = HEAP_strdupWtoA(GetProcessHeap(), 0, guid);
    snprintf(keyName, sizeof(keyName), "TypeLib\\%s\\%x.%x",
        guidA, attr->wMajorVerNum, attr->wMinorVerNum);
    HeapFree(GetProcessHeap(), 0, guidA);

    res = S_OK;
    if (RegCreateKeyExA(HKEY_CLASSES_ROOT, keyName, 0, NULL, 0,
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
        sprintf(tmp, "%lu\\", attr->lcid);
        switch(attr->syskind) {
        case SYS_WIN16:
            strcat(tmp, "win16");
            break;

        case SYS_WIN32:
            strcat(tmp, "win32");
            break;

        default:
            TRACE("Typelib is for unsupported syskind %i\n", attr->syskind);
            res = E_FAIL;
            break;
        }

        /* Create the typelib path subkey */
        if (res == S_OK && RegCreateKeyExA(key, tmp, 0, NULL, 0,
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
        if (res == S_OK && RegCreateKeyExA(key, "FLAGS", 0, NULL, 0,
            KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS)
        {
            CHAR buf[20];
            /* FIXME: is %u correct? */
            snprintf(buf, sizeof(buf), "%u", attr->wLibFlags);
            if (RegSetValueExA(subKey, NULL, 0, REG_SZ,
                buf, lstrlenA(buf) + 1) != ERROR_SUCCESS)
                res = E_FAIL;

            RegCloseKey(subKey);
        }
        else
            res = E_FAIL;

        /* create the helpdir subkey */
        if (res == S_OK && RegCreateKeyExA(key, "HELPDIR", 0, NULL, 0,
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

		    /*
		     * FIXME: The 1 is just here until we implement rpcrt4
		     *        stub/proxy handling. Until then it helps IShield
		     *        v6 to work.
		     */
		    if (1 || (tattr->wTypeFlags & TYPEFLAG_FOLEAUTOMATION))
		    {
                        if (!(tattr->wTypeFlags & TYPEFLAG_FOLEAUTOMATION)) {
                            FIXME("Registering non-oleautomation interface!\n");
                        }

			/* register interface<->typelib coupling */
			StringFromGUID2(&tattr->guid, guid, 80);
			guidA = HEAP_strdupWtoA(GetProcessHeap(), 0, guid);
			snprintf(keyName, sizeof(keyName), "Interface\\%s", guidA);
			HeapFree(GetProcessHeap(), 0, guidA);

			if (RegCreateKeyExA(HKEY_CLASSES_ROOT, keyName, 0, NULL, 0,
					    KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS) {
			    if (name)
				RegSetValueExW(key, NULL, 0, REG_SZ,
					       (BYTE *)name, lstrlenW(name) * sizeof(OLECHAR));

			    if (RegCreateKeyExA(key, "ProxyStubClsid", 0, NULL, 0,
				KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS) {
				RegSetValueExA(subKey, NULL, 0, REG_SZ,
					       PSOA, strlen(PSOA));
				RegCloseKey(subKey);
			    }

			    if (RegCreateKeyExA(key, "ProxyStubClsid32", 0, NULL, 0,
				KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS) {
				RegSetValueExA(subKey, NULL, 0, REG_SZ,
					       PSOA, strlen(PSOA));
				RegCloseKey(subKey);
			    }

			    if (RegCreateKeyExA(key, "TypeLib", 0, NULL, 0,
				KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS) {
				CHAR ver[32];
				StringFromGUID2(&attr->guid, guid, 80);
				snprintf(ver, sizeof(ver), "%x.%x",
					 attr->wMajorVerNum, attr->wMinorVerNum);
				RegSetValueExW(subKey, NULL, 0, REG_SZ,
					       (BYTE *)guid, lstrlenW(guid) * sizeof(OLECHAR));
				RegSetValueExA(subKey, "Version", 0, REG_SZ,
					       ver, lstrlenA(ver));
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
    CHAR keyName[MAX_PATH];
    CHAR* syskindName;
    CHAR subKeyName[MAX_PATH];
    LPSTR guidA;
    int result = S_OK;
    DWORD i = 0;
    OLECHAR guid[80];
    BOOL deleteOtherStuff;
    HKEY key = NULL;
    HKEY subKey = NULL;
    TYPEATTR* typeAttr = NULL;
    TYPEKIND kind;
    ITypeInfo* typeInfo = NULL;
    ITypeLib* typeLib = NULL;
    int numTypes;

    TRACE("(IID: %s): stub\n",debugstr_guid(libid));

    /* Create the path to the key */
    StringFromGUID2(libid, guid, 80);
    guidA = HEAP_strdupWtoA(GetProcessHeap(), 0, guid);
    snprintf(keyName, sizeof(keyName), "TypeLib\\%s\\%x.%x",
        guidA, wVerMajor, wVerMinor);
    HeapFree(GetProcessHeap(), 0, guidA);

    /* Work out the syskind name */
    switch(syskind) {
    case SYS_WIN16:
        syskindName = "win16";
        break;

    case SYS_WIN32:
        syskindName = "win32";
        break;

    default:
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
    if (RegOpenKeyExA(HKEY_CLASSES_ROOT, keyName, 0, KEY_READ | KEY_WRITE, &key) != S_OK) {
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
        StringFromGUID2(&typeAttr->guid, guid, 80);
        guidA = HEAP_strdupWtoA(GetProcessHeap(), 0, guid);
        snprintf(subKeyName, sizeof(subKeyName), "Interface\\%s", guidA);
        HeapFree(GetProcessHeap(), 0, guidA);

        /* Delete its bits */
        if (RegOpenKeyExA(HKEY_CLASSES_ROOT, subKeyName, 0, KEY_WRITE, &subKey) != S_OK) {
            goto enddeleteloop;
        }
        RegDeleteKeyA(subKey, "ProxyStubClsid");
        RegDeleteKeyA(subKey, "ProxyStubClsid32");
        RegDeleteKeyA(subKey, "TypeLib");
        RegCloseKey(subKey);
        subKey = NULL;
        RegDeleteKeyA(HKEY_CLASSES_ROOT, subKeyName);

enddeleteloop:
        if (typeAttr) ITypeInfo_ReleaseTypeAttr(typeInfo, typeAttr);
        typeAttr = NULL;
        if (typeInfo) ITypeInfo_Release(typeInfo);
        typeInfo = NULL;
    }

    /* Now, delete the type library path subkey */
    sprintf(subKeyName, "%lu\\%s", lcid, syskindName);
    RegDeleteKeyA(key, subKeyName);
    sprintf(subKeyName, "%lu", lcid);
    RegDeleteKeyA(key, subKeyName);

    /* check if there is anything besides the FLAGS/HELPDIR keys.
       If there is, we don't delete them */
    tmpLength = sizeof(subKeyName);
    deleteOtherStuff = TRUE;
    i = 0;
    while(RegEnumKeyExA(key, i++, subKeyName, &tmpLength, NULL, NULL, NULL, NULL) == S_OK) {
        tmpLength = sizeof(subKeyName);

        /* if its not FLAGS or HELPDIR, then we must keep the rest of the key */
        if (!strcmp(subKeyName, "FLAGS")) continue;
        if (!strcmp(subKeyName, "HELPDIR")) continue;
        deleteOtherStuff = FALSE;
        break;
    }

    /* only delete the other parts of the key if we're absolutely sure */
    if (deleteOtherStuff) {
        RegDeleteKeyA(key, "FLAGS");
        RegDeleteKeyA(key, "HELPDIR");
        RegCloseKey(key);
        key = NULL;

        StringFromGUID2(libid, guid, 80);
        guidA = HEAP_strdupWtoA(GetProcessHeap(), 0, guid);
        sprintf(keyName, "TypeLib\\%s\\%x.%x", guidA, wVerMajor, wVerMinor);
        RegDeleteKeyA(HKEY_CLASSES_ROOT, keyName);
        sprintf(keyName, "TypeLib\\%s", guidA);
        RegDeleteKeyA(HKEY_CLASSES_ROOT, keyName);
        HeapFree(GetProcessHeap(), 0, guidA);
    }

end:
    if (tlibPath) SysFreeString(tlibPath);
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
    ICOM_VFIELD(ITypeLib2);
    ICOM_VTABLE(ITypeComp) * lpVtblTypeComp;
    UINT ref;
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
    TYPEDESC * pTypeDesc;       /* array of TypeDescriptions found in the
				   libary. Only used while read MSFT
				   typelibs */

    /* typelibs are cached, keyed by path, so store the linked list info within them */
    struct tagITypeLibImpl *next, *prev;
    WCHAR *path;
} ITypeLibImpl;

static struct ICOM_VTABLE(ITypeLib2) tlbvt;
static struct ICOM_VTABLE(ITypeComp) tlbtcvt;

#define _ITypeComp_Offset(impl) ((int)(&(((impl*)0)->lpVtblTypeComp)))
#define ICOM_THIS_From_ITypeComp(impl, iface) impl* This = (impl*)(((char*)iface)-_ITypeComp_Offset(impl))

/* ITypeLib methods */
static ITypeLib2* ITypeLib2_Constructor_MSFT(LPVOID pLib, DWORD dwTLBLength);
static ITypeLib2* ITypeLib2_Constructor_SLTG(LPVOID pLib, DWORD dwTLBLength);

/*======================= ITypeInfo implementation =======================*/

/* data for refernced types */
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

    struct tagTLBRefType * next;
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
    ICOM_VFIELD(ITypeInfo2);
    ICOM_VTABLE(ITypeComp) * lpVtblTypeComp;
    UINT ref;
    TYPEATTR TypeAttr ;         /* _lots_ of type information. */
    ITypeLibImpl * pTypeLib;        /* back pointer to typelib */
    int index;                  /* index in this typelib; */
    /* type libs seem to store the doc strings in ascii
     * so why should we do it in unicode?
     */
    BSTR Name;
    BSTR DocString;
    unsigned long  dwHelpContext;
    unsigned long  dwHelpStringContext;

    /* functions  */
    TLBFuncDesc * funclist;     /* linked list with function descriptions */

    /* variables  */
    TLBVarDesc * varlist;       /* linked list with variable descriptions */

    /* Implemented Interfaces  */
    TLBImplType * impltypelist;

    TLBRefType * reflist;
    int ctCustData;
    TLBCustData * pCustData;        /* linked list to cust data; */
    struct tagITypeInfoImpl * next;
} ITypeInfoImpl;

static struct ICOM_VTABLE(ITypeInfo2) tinfvt;
static struct ICOM_VTABLE(ITypeComp)  tcompvt;

static ITypeInfo2 * WINAPI ITypeInfo_Constructor();

typedef struct tagTLBContext
{
	unsigned int oStart;  /* start of TLB in file */
	unsigned int pos;     /* current pos */
	unsigned int length;  /* total length */
	void *mapping;        /* memory mapping */
	MSFT_SegDir * pTblDir;
	ITypeLibImpl* pLibInfo;
} TLBContext;


static void MSFT_DoRefType(TLBContext *pcx, ITypeInfoImpl *pTI, int offset);

/*
 debug
*/
static void dump_TypeDesc(TYPEDESC *pTD,char *szVarType) {
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
    case VT_USERDEFINED: sprintf(szVarType, "VT_USERDEFINED ref = %lx",
				 pTD->u.hreftype); break;
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

void dump_ELEMDESC(ELEMDESC *edesc) {
  char buf[200];
  dump_TypeDesc(&edesc->tdesc,buf);
  MESSAGE("\t\ttdesc.vartype %d (%s)\n",edesc->tdesc.vt,buf);
  MESSAGE("\t\tu.parmadesc.flags %x\n",edesc->u.paramdesc.wParamFlags);
  MESSAGE("\t\tu.parmadesc.lpex %p\n",edesc->u.paramdesc.pparamdescex);
}
void dump_FUNCDESC(FUNCDESC *funcdesc) {
  int i;
  MESSAGE("memid is %08lx\n",funcdesc->memid);
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
}

void dump_IDLDESC(IDLDESC *idl) {
  MESSAGE("\t\twIdlflags: %d\n",idl->wIDLFlags);
}

static const char * typekind_desc[] =
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

void dump_TYPEATTR(TYPEATTR *tattr) {
  char buf[200];
  MESSAGE("\tguid: %s\n",debugstr_guid(&tattr->guid));
  MESSAGE("\tlcid: %ld\n",tattr->lcid);
  MESSAGE("\tmemidConstructor: %ld\n",tattr->memidConstructor);
  MESSAGE("\tmemidDestructor: %ld\n",tattr->memidDestructor);
  MESSAGE("\tschema: %s\n",debugstr_w(tattr->lpstrSchema));
  MESSAGE("\tsizeInstance: %ld\n",tattr->cbSizeInstance);
  MESSAGE("\tkind:%s\n", typekind_desc[tattr->typekind]);
  MESSAGE("\tcFuncs: %d\n", tattr->cFuncs);
  MESSAGE("\tcVars: %d\n", tattr->cVars);
  MESSAGE("\tcImplTypes: %d\n", tattr->cImplTypes);
  MESSAGE("\tcbSizeVft: %d\n", tattr->cbSizeVft);
  MESSAGE("\tcbAlignment: %d\n", tattr->cbAlignment);
  MESSAGE("\twTypeFlags: %d\n", tattr->wTypeFlags);
  MESSAGE("\tVernum: %d.%d\n", tattr->wMajorVerNum,tattr->wMinorVerNum);
  dump_TypeDesc(&tattr->tdescAlias,buf);
  MESSAGE("\ttypedesc: %s\n", buf);
  dump_IDLDESC(&tattr->idldescType);
}

static void dump_TLBFuncDescOne(TLBFuncDesc * pfd)
{
  int i;
  if (!TRACE_ON(typelib))
      return;
  MESSAGE("%s(%u)\n", debugstr_w(pfd->Name), pfd->funcdesc.cParams);
  for (i=0;i<pfd->funcdesc.cParams;i++)
      MESSAGE("\tparm%d: %s\n",i,debugstr_w(pfd->pParamDesc[i].Name));


  dump_FUNCDESC(&(pfd->funcdesc));

  MESSAGE("\thelpstring: %s\n", debugstr_w(pfd->HelpString));
  MESSAGE("\tentry: %s\n", debugstr_w(pfd->Entry));
}
static void dump_TLBFuncDesc(TLBFuncDesc * pfd)
{
	while (pfd)
	{
	  dump_TLBFuncDescOne(pfd);
	  pfd = pfd->next;
	};
}
static void dump_TLBVarDesc(TLBVarDesc * pvd)
{
	while (pvd)
	{
	  TRACE_(typelib)("%s\n", debugstr_w(pvd->Name));
	  pvd = pvd->next;
	};
}

static void dump_TLBImpLib(TLBImpLib *import)
{
    TRACE_(typelib)("%s %s\n", debugstr_guid(&(import->guid)),
		    debugstr_w(import->name));
    TRACE_(typelib)("v%d.%d lcid=%lx offset=%x\n", import->wVersionMajor,
		    import->wVersionMinor, import->lcid, import->offset);
}

static void dump_TLBRefType(TLBRefType * prt)
{
	while (prt)
	{
	  TRACE_(typelib)("href:0x%08lx\n", prt->reference);
	  if(prt->index == -1)
	    TRACE_(typelib)("%s\n", debugstr_guid(&(prt->guid)));
	  else
	    TRACE_(typelib)("type no: %d\n", prt->index);

	  if(prt->pImpTLInfo != TLB_REF_INTERNAL &&
	     prt->pImpTLInfo != TLB_REF_NOT_FOUND) {
	      TRACE_(typelib)("in lib\n");
	      dump_TLBImpLib(prt->pImpTLInfo);
	  }
	  prt = prt->next;
	};
}

static void dump_TLBImplType(TLBImplType * impl)
{
    while (impl) {
        TRACE_(typelib)(
		"implementing/inheriting interface hRef = %lx implflags %x\n",
		impl->hRef, impl->implflags);
	impl = impl->next;
    }
}

void dump_Variant(VARIANT * pvar)
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
        TRACE(",FIXME");
      }
      else switch (V_TYPE(pvar))
      {
      case VT_I1:   TRACE(",%d", V_I1(pvar)); break;
      case VT_UI1:  TRACE(",%d", V_UI1(pvar)); break;
      case VT_I2:   TRACE(",%d", V_I2(pvar)); break;
      case VT_UI2:  TRACE(",%d", V_UI2(pvar)); break;
      case VT_INT:
      case VT_I4:   TRACE(",%ld", V_I4(pvar)); break;
      case VT_UINT:
      case VT_UI4:  TRACE(",%ld", V_UI4(pvar)); break;
      case VT_I8:   TRACE(",0x%08lx,0x%08lx", (ULONG)(V_I8(pvar) >> 32),
                          (ULONG)(V_I8(pvar) & 0xffffffff)); break;
      case VT_UI8:  TRACE(",0x%08lx,0x%08lx", (ULONG)(V_UI8(pvar) >> 32),
                          (ULONG)(V_UI8(pvar) & 0xffffffff)); break;
      case VT_R4:   TRACE(",%3.3e", V_R4(pvar)); break;
      case VT_R8:   TRACE(",%3.3e", V_R8(pvar)); break;
      case VT_BOOL: TRACE(",%s", V_BOOL(pvar) ? "TRUE" : "FALSE"); break;
      case VT_BSTR: TRACE(",%s", debugstr_w(V_BSTR(pvar))); break;
      case VT_CY:   TRACE(",0x%08lx,0x%08lx", V_CY(pvar).s.Hi,
                           V_CY(pvar).s.Lo); break;
#ifndef __REACTOS /*FIXME*/
      case VT_DATE:
        if(!VariantTimeToSystemTime(V_DATE(pvar), &st))
          TRACE(",<invalid>");
        else
          TRACE(",%04d/%02d/%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond);
        break;
#endif
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

static void dump_DispParms(DISPPARAMS * pdp)
{
    int index = 0;

    TRACE("args=%u named args=%u\n", pdp->cArgs, pdp->cNamedArgs);

    while (index < pdp->cArgs)
    {
        dump_Variant( &pdp->rgvarg[index] );
        ++index;
    }
}

static void dump_TypeInfo(ITypeInfoImpl * pty)
{
    TRACE("%p ref=%u\n", pty, pty->ref);
    TRACE("attr:%s\n", debugstr_guid(&(pty->TypeAttr.guid)));
    TRACE("kind:%s\n", typekind_desc[pty->TypeAttr.typekind]);
    TRACE("fct:%u var:%u impl:%u\n",
      pty->TypeAttr.cFuncs, pty->TypeAttr.cVars, pty->TypeAttr.cImplTypes);
    TRACE("parent tlb:%p index in TLB:%u\n",pty->pTypeLib, pty->index);
    TRACE("%s %s\n", debugstr_w(pty->Name), debugstr_w(pty->DocString));
    dump_TLBFuncDesc(pty->funclist);
    dump_TLBVarDesc(pty->varlist);
    dump_TLBImplType(pty->impltypelist);
}

void dump_VARDESC(VARDESC *v)
{
    MESSAGE("memid %ld\n",v->memid);
    MESSAGE("lpstrSchema %s\n",debugstr_w(v->lpstrSchema));
    MESSAGE("oInst %ld\n",v->u.oInst);
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

static void TLB_abort()
{
    DebugBreak();
}
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


/**********************************************************************
 *
 *  Functions for reading MSFT typelibs (those created by CreateTypeLib2)
 */
/* read function */
DWORD MSFT_Read(void *buffer,  DWORD count, TLBContext *pcx, long where )
{
    TRACE_(typelib)("pos=0x%08x len=0x%08lx 0x%08x 0x%08x 0x%08lx\n",
       pcx->pos, count, pcx->oStart, pcx->length, where);

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

BSTR MSFT_ReadName( TLBContext *pcx, int offset)
{
    char * name;
    MSFT_NameIntro niName;
    int lengthInChars;
    WCHAR* pwstring = NULL;
    BSTR bstrName = NULL;

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
        pwstring = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*lengthInChars);

        /* don't check for invalid character since this has been done previously */
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, -1, pwstring, lengthInChars);

        bstrName = SysAllocStringLen(pwstring, lengthInChars);
        lengthInChars = SysStringLen(bstrName);
        HeapFree(GetProcessHeap(), 0, pwstring);
    }

    TRACE_(typelib)("%s %d\n", debugstr_w(bstrName), lengthInChars);
    return bstrName;
}

BSTR MSFT_ReadString( TLBContext *pcx, int offset)
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
        WCHAR* pwstring = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*lengthInChars);

        /* don't check for invalid character since this has been done previously */
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, string, -1, pwstring, lengthInChars);

        bstr = SysAllocStringLen(pwstring, lengthInChars);
        lengthInChars = SysStringLen(bstr);
        HeapFree(GetProcessHeap(), 0, pwstring);
    }

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
        V_UNION(pVar, iVal) = offset & 0xffff;
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
	    if(size <= 0) {
	        FIXME("BSTR length = %d?\n", size);
	    } else {
                ptr=TLB_Alloc(size);/* allocate temp buffer */
		MSFT_Read(ptr, size, pcx, DO_NOT_SEEK);/* read string (ANSI) */
		V_UNION(pVar, bstrVal)=SysAllocStringLen(NULL,size);
		/* FIXME: do we need a AtoW conversion here? */
		V_UNION(pVar, bstrVal[size])=L'\0';
		while(size--) V_UNION(pVar, bstrVal[size])=ptr[size];
		TLB_Free(ptr);
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
        MSFT_Read(&(V_UNION(pVar, iVal)), size, pcx, DO_NOT_SEEK );
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
      MSFT_DoRefType(pcx, pTI, pTd->u.hreftype);

    TRACE_(typelib)("vt type = %X\n", pTd->vt);
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
     * the first part starts with a field that holds the total length
     * of this (first) part excluding this field. Then follow the records,
     * for each member there is one record.
     *
     * First entry is always the length of the record (excluding this
     * length word).
     * Rest of the record depends on the type of the member. If there is
     * a field indicating the member type (function variable intereface etc)
     * I have not found it yet. At this time we depend on the information
     * in the type info and the usual order how things are stored.
     *
     * Second follows an array sized nrMEM*sizeof(INT) with a memeber id
     * for each member;
     *
     * Third is a equal sized array with file offsets to the name entry
     * of each member.
     *
     * Forth and last (?) part is an array with offsets to the records in the
     * first part of this file segment.
     */

    int infolen, nameoffset, reclength, nrattributes, i;
    int recoffset = offset + sizeof(INT);

    char recbuf[512];
    MSFT_FuncRecord * pFuncRec=(MSFT_FuncRecord *) recbuf;

    TRACE_(typelib)("\n");

    MSFT_ReadLEDWords(&infolen, sizeof(INT), pcx, offset);

    for ( i = 0; i < cFuncs ; i++ )
    {
        *pptfd = TLB_Alloc(sizeof(TLBFuncDesc));

        /* name, eventually add to a hash table */
        MSFT_ReadLEDWords(&nameoffset, sizeof(INT), pcx,
                          offset + infolen + (cFuncs + cVars + i + 1) * sizeof(INT));

        (*pptfd)->Name = MSFT_ReadName(pcx, nameoffset);

        /* read the function information record */
        MSFT_ReadLEDWords(&reclength, sizeof(INT), pcx, recoffset);

        reclength &= 0x1ff;

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
                       (*pptfd)->Entry = (WCHAR*) pFuncRec->OptAttr[2] ;
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
        (*pptfd)->funcdesc.oVft       =   pFuncRec->VtableOffset ;
        (*pptfd)->funcdesc.wFuncFlags =   LOWORD(pFuncRec->Flags) ;

        MSFT_GetTdesc(pcx,
		      pFuncRec->DataType,
		      &(*pptfd)->funcdesc.elemdescFunc.tdesc,
		      pTI);

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
                TYPEDESC* lpArgTypeDesc = 0;

                MSFT_GetTdesc(pcx,
			      paraminfo.DataType,
			      &(*pptfd)->funcdesc.lprgelemdescParam[j].tdesc,
			      pTI);

                (*pptfd)->funcdesc.lprgelemdescParam[j].u.paramdesc.wParamFlags = paraminfo.Flags;

                (*pptfd)->pParamDesc[j].Name = (void *) paraminfo.oName;

                /* SEEK value = jump to offset,
                 * from there jump to the end of record,
                 * go back by (j-1) arguments
                 */
                MSFT_ReadLEDWords( &paraminfo ,
			   sizeof(MSFT_ParameterInfo), pcx,
			   recoffset + reclength - ((pFuncRec->nrargs - j - 1)
					       * sizeof(MSFT_ParameterInfo)));
                lpArgTypeDesc =
                    & ((*pptfd)->funcdesc.lprgelemdescParam[j].tdesc);

                while ( lpArgTypeDesc != NULL )
                {
                    switch ( lpArgTypeDesc->vt )
                    {
                    case VT_PTR:
                        lpArgTypeDesc = lpArgTypeDesc->u.lptdesc;
                        break;

                    case VT_CARRAY:
                        lpArgTypeDesc = & (lpArgTypeDesc->u.lpadesc->tdescElem);
                        break;

                    case VT_USERDEFINED:
                        MSFT_DoRefType(pcx, pTI,
				       lpArgTypeDesc->u.hreftype);

                        lpArgTypeDesc = NULL;
                        break;

                    default:
                        lpArgTypeDesc = NULL;
                    }
                }
            }


            /* parameter is the return value! */
            if ( paraminfo.Flags & PARAMFLAG_FRETVAL )
            {
                TYPEDESC* lpArgTypeDesc;

                (*pptfd)->funcdesc.elemdescFunc =
                (*pptfd)->funcdesc.lprgelemdescParam[j];

                lpArgTypeDesc = & ((*pptfd)->funcdesc.elemdescFunc.tdesc) ;

                while ( lpArgTypeDesc != NULL )
                {
                    switch ( lpArgTypeDesc->vt )
                    {
                    case VT_PTR:
                        lpArgTypeDesc = lpArgTypeDesc->u.lptdesc;
                        break;
                    case VT_CARRAY:
                        lpArgTypeDesc =
                        & (lpArgTypeDesc->u.lpadesc->tdescElem);

                        break;

                    case VT_USERDEFINED:
                        MSFT_DoRefType(pcx,
				       pTI,
				       lpArgTypeDesc->u.hreftype);

                        lpArgTypeDesc = NULL;
                        break;

                    default:
                        lpArgTypeDesc = NULL;
                    }
                }
            }

            /* second time around */
            for(j=0;j<pFuncRec->nrargs;j++)
            {
                /* name */
                (*pptfd)->pParamDesc[j].Name =
                    MSFT_ReadName( pcx, (int)(*pptfd)->pParamDesc[j].Name );

                /* default value */
                if ( (PARAMFLAG_FHASDEFAULT &
                      (*pptfd)->funcdesc.lprgelemdescParam[j].u.paramdesc.wParamFlags) &&
                     ((pFuncRec->FKCCIC) & 0x1000) )
                {
                    INT* pInt = (INT *)((char *)pFuncRec +
                                   reclength -
                                   (pFuncRec->nrargs * 4 + 1) * sizeof(INT) );

                    PARAMDESC* pParamDesc = & (*pptfd)->funcdesc.lprgelemdescParam[j].u.paramdesc;

                    pParamDesc->pparamdescex = TLB_Alloc(sizeof(PARAMDESCEX));
                    pParamDesc->pparamdescex->cBytes = sizeof(PARAMDESCEX);

		    MSFT_ReadValue(&(pParamDesc->pparamdescex->varDefaultValue),
                        pInt[j], pcx);
                }
                /* custom info */
                if ( nrattributes > 7 + j && pFuncRec->FKCCIC & 0x80 )
                {
                    MSFT_CustData(pcx,
				  pFuncRec->OptAttr[7+j],
				  &(*pptfd)->pParamDesc[j].pCustData);
                }
           }
        }

        /* scode is not used: archaic win16 stuff FIXME: right? */
        (*pptfd)->funcdesc.cScodes   = 0 ;
        (*pptfd)->funcdesc.lprgscode = NULL ;

        pptfd      = & ((*pptfd)->next);
        recoffset += reclength;
    }
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
                          offset + infolen + (cFuncs + cVars + i + 1) * sizeof(INT));
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
                          offset + infolen + ( i + 1) * sizeof(INT));
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
        pptvd=&((*pptvd)->next);
        recoffset += reclength;
    }
}
/* fill in data for a hreftype (offset). When the refernced type is contained
 * in the typelib, it's just an (file) offset in the type info base dir.
 * If comes from import, it's an offset+1 in the ImpInfo table
 * */
static void MSFT_DoRefType(TLBContext *pcx, ITypeInfoImpl *pTI,
                          int offset)
{
    int j;
    TLBRefType **ppRefType = &pTI->reflist;

    TRACE_(typelib)("TLB context %p, TLB offset %x\n", pcx, offset);

    while(*ppRefType) {
        if((*ppRefType)->reference == offset)
	    return;
	ppRefType = &(*ppRefType)->next;
    }

    *ppRefType = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			   sizeof(**ppRefType));

    if(!MSFT_HREFTYPE_INTHISFILE( offset)) {
        /* external typelib */
        MSFT_ImpInfo impinfo;
        TLBImpLib *pImpLib=(pcx->pLibInfo->pImpLibs);

        TRACE_(typelib)("offset %x, masked offset %x\n", offset, offset + (offset & 0xfffffffc));

        MSFT_ReadLEDWords(&impinfo, sizeof(impinfo), pcx,
                          pcx->pTblDir->pImpInfo.offset + (offset & 0xfffffffc));
        for(j=0;pImpLib;j++){   /* search the known offsets of all import libraries */
            if(pImpLib->offset==impinfo.oImpFile) break;
            pImpLib=pImpLib->next;
        }
        if(pImpLib){
            (*ppRefType)->reference=offset;
            (*ppRefType)->pImpTLInfo = pImpLib;
            MSFT_ReadGuid(&(*ppRefType)->guid, impinfo.oGuid, pcx);
	    (*ppRefType)->index = TLB_REF_USE_GUID;
        }else{
            ERR("Cannot find a reference\n");
            (*ppRefType)->reference=-1;
            (*ppRefType)->pImpTLInfo=TLB_REF_NOT_FOUND;
        }
    }else{
        /* in this typelib */
        (*ppRefType)->index = MSFT_HREFTYPE_INDEX(offset);
        (*ppRefType)->reference=offset;
        (*ppRefType)->pImpTLInfo=TLB_REF_INTERNAL;
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
        MSFT_DoRefType(pcx, pTI, refrec.reftype);
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
ITypeInfoImpl * MSFT_DoTypeInfo(
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
    WARN("Assign constructor/destructor memid\n");

    MSFT_ReadGuid(&ptiRet->TypeAttr.guid, tiBase.posguid, pcx);
    ptiRet->TypeAttr.lcid=pLibInfo->LibAttr.lcid;   /* FIXME: correct? */
    ptiRet->TypeAttr.memidConstructor=MEMBERID_NIL ;/* FIXME */
    ptiRet->TypeAttr.memidDestructor=MEMBERID_NIL ; /* FIXME */
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
    ptiRet->TypeAttr.cbSizeVft=tiBase.cbSizeVft; /* FIXME: this is only the non inherited part */
    if(ptiRet->TypeAttr.typekind == TKIND_ALIAS)
        MSFT_GetTdesc(pcx, tiBase.datatype1,
            &ptiRet->TypeAttr.tdescAlias, ptiRet);

/*  FIXME: */
/*    IDLDESC  idldescType; *//* never saw this one != zero  */

/* name, eventually add to a hash table */
    ptiRet->Name=MSFT_ReadName(pcx, tiBase.NameOffset);
    TRACE_(typelib)("reading %s\n", debugstr_w(ptiRet->Name));
    /* help info */
    ptiRet->DocString=MSFT_ReadString(pcx, tiBase.docstringoffs);
    ptiRet->dwHelpStringContext=tiBase.helpstringcontext;
    ptiRet->dwHelpContext=tiBase.helpcontext;
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
            ptiRet->impltypelist=TLB_Alloc(sizeof(TLBImplType));

            if (tiBase.datatype1 != -1)
            {
              MSFT_DoRefType(pcx, ptiRet, tiBase.datatype1);
	      ptiRet->impltypelist->hRef = tiBase.datatype1;
            }
            else
	    { /* FIXME: This is a really bad hack to add IDispatch */
              const char* szStdOle = "stdole2.tlb\0";
              int   nStdOleLen = strlen(szStdOle);
	      TLBRefType **ppRef = &ptiRet->reflist;

	      while(*ppRef) {
		if((*ppRef)->reference == -1)
		  break;
		ppRef = &(*ppRef)->next;
	      }
	      if(!*ppRef) {
		*ppRef = TLB_Alloc(sizeof(**ppRef));
		(*ppRef)->guid             = IID_IDispatch;
		(*ppRef)->reference        = -1;
		(*ppRef)->index            = TLB_REF_USE_GUID;
		(*ppRef)->pImpTLInfo       = TLB_Alloc(sizeof(TLBImpLib));
		(*ppRef)->pImpTLInfo->guid = IID_StdOle;
		(*ppRef)->pImpTLInfo->name = SysAllocStringLen(NULL,
							      nStdOleLen  + 1);

		MultiByteToWideChar(CP_ACP,
				    MB_PRECOMPOSED,
				    szStdOle,
				    -1,
				    (*ppRef)->pImpTLInfo->name,
				    SysStringLen((*ppRef)->pImpTLInfo->name));

		(*ppRef)->pImpTLInfo->lcid          = 0;
		(*ppRef)->pImpTLInfo->wVersionMajor = 2;
		(*ppRef)->pImpTLInfo->wVersionMinor = 0;
	      }
	    }
            break;
        default:
            ptiRet->impltypelist=TLB_Alloc(sizeof(TLBImplType));
            MSFT_DoRefType(pcx, ptiRet, tiBase.datatype1);
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
      0, 0, { 0, (DWORD)(__FILE__ ": typelib loader cache") }
};
static CRITICAL_SECTION cache_section = { &cache_section_debug, -1, 0, 0, 0, 0 };


/****************************************************************************
 *	TLB_ReadTypeLib
 *
 * find the type of the typelib file and map the typelib resource into
 * the memory
 */
#define MSFT_SIGNATURE 0x5446534D /* "MSFT" */
#define SLTG_SIGNATURE 0x47544c53 /* "SLTG" */
int TLB_ReadTypeLib(LPCWSTR pszFileName, INT index, ITypeLib2 **ppTypeLib)
{
    ITypeLibImpl *entry;
    int ret = TYPE_E_CANTLOADLIBRARY;
    DWORD dwSignature = 0;
    HANDLE hFile;

    TRACE_(typelib)("%s:%d\n", debugstr_w(pszFileName), index);

    *ppTypeLib = NULL;

    /* We look the path up in the typelib cache. If found, we just addref it, and return the pointer. */
    EnterCriticalSection(&cache_section);
    for (entry = tlb_cache_first; entry != NULL; entry = entry->next)
    {
        if (!strcmpiW(entry->path, pszFileName))
        {
            TRACE("cache hit\n");
            *ppTypeLib = (ITypeLib2*)entry;
            ITypeLib_AddRef(*ppTypeLib);
            LeaveCriticalSection(&cache_section);
            return S_OK;
        }
    }
    LeaveCriticalSection(&cache_section);

    /* check the signature of the file */
    hFile = CreateFileW( pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    if (INVALID_HANDLE_VALUE != hFile)
    {
      HANDLE hMapping = CreateFileMappingA( hFile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL );
      if (hMapping)
      {
        LPVOID pBase = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
        if(pBase)
        {
	  /* retrieve file size */
	  DWORD dwTLBLength = GetFileSize(hFile, NULL);

          /* first try to load as *.tlb */
          dwSignature = FromLEDWord(*((DWORD*) pBase));
          if ( dwSignature == MSFT_SIGNATURE)
          {
            *ppTypeLib = ITypeLib2_Constructor_MSFT(pBase, dwTLBLength);
          }
	  else if ( dwSignature == SLTG_SIGNATURE)
          {
            *ppTypeLib = ITypeLib2_Constructor_SLTG(pBase, dwTLBLength);
	  }
          UnmapViewOfFile(pBase);
        }
        CloseHandle(hMapping);
      }
      CloseHandle(hFile);
    }

    if( (WORD)dwSignature == IMAGE_DOS_SIGNATURE )
    {
      /* find the typelibrary resource*/
      HINSTANCE hinstDLL = LoadLibraryExW(pszFileName, 0, DONT_RESOLVE_DLL_REFERENCES|
                                          LOAD_LIBRARY_AS_DATAFILE|LOAD_WITH_ALTERED_SEARCH_PATH);
      if (hinstDLL)
      {
        HRSRC hrsrc = FindResourceA(hinstDLL, MAKEINTRESOURCEA(index),
	  "TYPELIB");
        if (hrsrc)
        {
          HGLOBAL hGlobal = LoadResource(hinstDLL, hrsrc);
          if (hGlobal)
          {
            LPVOID pBase = LockResource(hGlobal);
            DWORD  dwTLBLength = SizeofResource(hinstDLL, hrsrc);

            if (pBase)
            {
              /* try to load as incore resource */
              dwSignature = FromLEDWord(*((DWORD*) pBase));
              if ( dwSignature == MSFT_SIGNATURE)
              {
                  *ppTypeLib = ITypeLib2_Constructor_MSFT(pBase, dwTLBLength);
	      }
	      else if ( dwSignature == SLTG_SIGNATURE)
	      {
                  *ppTypeLib = ITypeLib2_Constructor_SLTG(pBase, dwTLBLength);
	      }
              else
              {
                  FIXME("Header type magic 0x%08lx not supported.\n",dwSignature);
              }
            }
            FreeResource( hGlobal );
          }
        }
        FreeLibrary(hinstDLL);
      }
    }

    if(*ppTypeLib) {
	ITypeLibImpl *impl = (ITypeLibImpl*)*ppTypeLib;

	TRACE("adding to cache\n");
	impl->path = HeapAlloc(GetProcessHeap(), 0, (strlenW(pszFileName)+1) * sizeof(WCHAR));
	lstrcpyW(impl->path, pszFileName);
	/* We should really canonicalise the path here. */

        /* FIXME: check if it has added already in the meantime */
        EnterCriticalSection(&cache_section);
        if ((impl->next = tlb_cache_first) != NULL) impl->next->prev = impl;
        impl->prev = NULL;
        tlb_cache_first = impl;
        LeaveCriticalSection(&cache_section);
        ret = S_OK;
    } else
	ERR("Loading of typelib %s failed with error %ld\n", debugstr_w(pszFileName), GetLastError());

    return ret;
}

/*================== ITypeLib(2) Methods ===================================*/

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

    TRACE("%p, TLB length = %ld\n", pLib, dwTLBLength);

    pTypeLibImpl = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ITypeLibImpl));
    if (!pTypeLibImpl) return NULL;

    pTypeLibImpl->lpVtbl = &tlbvt;
    pTypeLibImpl->lpVtblTypeComp = &tlbtcvt;
    pTypeLibImpl->ref = 1;

    /* get pointer to beginning of typelib data */
    cx.pos = 0;
    cx.oStart=0;
    cx.mapping = pLib;
    cx.pLibInfo = pTypeLibImpl;
    cx.length = dwTLBLength;

    /* read header */
    MSFT_ReadLEDWords((void*)&tlbHeader, sizeof(tlbHeader), &cx, 0);
    TRACE("header:\n");
    TRACE("\tmagic1=0x%08x ,magic2=0x%08x\n",tlbHeader.magic1,tlbHeader.magic2 );
    if (tlbHeader.magic1 != MSFT_SIGNATURE) {
	FIXME("Header type magic 0x%08x not supported.\n",tlbHeader.magic1);
	return NULL;
    }
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

    /* fill in typedescriptions */
    if(tlbSegDir.pTypdescTab.length > 0)
    {
        int i, j, cTD = tlbSegDir.pTypdescTab.length / (2*sizeof(INT));
        INT16 td[4];
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
            DWORD len;

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
            len = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0 );
            (*ppImpLib)->name = TLB_Alloc(len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, name, -1, (*ppImpLib)->name, len );
            TLB_Free(name);

            MSFT_ReadGuid(&(*ppImpLib)->guid, oGuid, &cx);
            offset = (offset + sizeof(INT) + sizeof(DWORD) + sizeof(LCID) + sizeof(UINT16) + size + 3) & ~3;

            ppImpLib = &(*ppImpLib)->next;
        }
    }

    /* type info's */
    if(tlbHeader.nrtypeinfos >= 0 )
    {
        /*pTypeLibImpl->TypeInfoCount=tlbHeader.nrtypeinfos; */
        ITypeInfoImpl **ppTI = &(pTypeLibImpl->pTypeInfo);
        int i;

        for(i = 0; i<(int)tlbHeader.nrtypeinfos; i++)
        {
            *ppTI = MSFT_DoTypeInfo(&cx, i, pTypeLibImpl);

            ITypeInfo_AddRef((ITypeInfo*) *ppTI);
            ppTI = &((*ppTI)->next);
            (pTypeLibImpl->TypeInfoCount)++;
        }
    }

    TRACE("(%p)\n", pTypeLibImpl);
    return (ITypeLib2*) pTypeLibImpl;
}


static BSTR TLB_MultiByteToBSTR(char *ptr)
{
    DWORD len;
    WCHAR *nameW;
    BSTR ret;

    len = MultiByteToWideChar(CP_ACP, 0, ptr, -1, NULL, 0);
    nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, ptr, -1, nameW, len);
    ret = SysAllocString(nameW);
    HeapFree(GetProcessHeap(), 0, nameW);
    return ret;
}

static BOOL TLB_GUIDFromString(char *str, GUID *guid)
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

static WORD SLTG_ReadString(char *ptr, BSTR *pBstr)
{
    WORD bytelen;
    DWORD len;
    WCHAR *nameW;

    *pBstr = NULL;
    bytelen = *(WORD*)ptr;
    if(bytelen == 0xffff) return 2;
    len = MultiByteToWideChar(CP_ACP, 0, ptr + 2, bytelen, NULL, 0);
    nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    len = MultiByteToWideChar(CP_ACP, 0, ptr + 2, bytelen, nameW, len);
    *pBstr = SysAllocStringLen(nameW, len);
    HeapFree(GetProcessHeap(), 0, nameW);
    return bytelen + 2;
}

static WORD SLTG_ReadStringA(char *ptr, char **str)
{
    WORD bytelen;

    *str = NULL;
    bytelen = *(WORD*)ptr;
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

static WORD *SLTG_DoType(WORD *pType, char *pBlk, ELEMDESC *pElem)
{
    BOOL done = FALSE;
    TYPEDESC *pTD = &pElem->tdesc;

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

    while(!done) {
        if((*pType & 0xe00) == 0xe00) {
	    pTD->vt = VT_PTR;
	    pTD->u.lptdesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				       sizeof(TYPEDESC));
	    pTD = pTD->u.lptdesc;
	}
	switch(*pType & 0x7f) {
	case VT_PTR:
	    pTD->vt = VT_PTR;
	    pTD->u.lptdesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				       sizeof(TYPEDESC));
	    pTD = pTD->u.lptdesc;
	    break;

	case VT_USERDEFINED:
	    pTD->vt = VT_USERDEFINED;
	    pTD->u.hreftype = *(++pType) / 4;
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
	    pTD->vt = *pType & 0x7f;
	    done = TRUE;
	    break;
	}
	pType++;
    }
    return pType;
}


static void SLTG_DoRefs(SLTG_RefInfo *pRef, ITypeInfoImpl *pTI,
			char *pNameTable)
{
    int ref;
    char *name;
    TLBRefType **ppRefType;

    if(pRef->magic != SLTG_REF_MAGIC) {
        FIXME("Ref magic = %x\n", pRef->magic);
	return;
    }
    name = ( (char*)(&pRef->names) + pRef->number);

    ppRefType = &pTI->reflist;
    for(ref = 0; ref < pRef->number >> 3; ref++) {
        char *refname;
	unsigned int lib_offs, type_num;

	*ppRefType = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			       sizeof(**ppRefType));

	name += SLTG_ReadStringA(name, &refname);
	if(sscanf(refname, "*\\R%x*#%x", &lib_offs, &type_num) != 2)
	    FIXME("Can't sscanf ref\n");
	if(lib_offs != 0xffff) {
	    TLBImpLib **import = &pTI->pTypeLib->pImpLibs;

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
		if(sscanf(pNameTable + lib_offs + 40, "}#%hd.%hd#%lx#%s",
			  &(*import)->wVersionMajor,
			  &(*import)->wVersionMinor,
			  &(*import)->lcid, fname) != 4) {
		  FIXME("can't sscanf ref %s\n",
			pNameTable + lib_offs + 40);
		}
		len = strlen(fname);
		if(fname[len-1] != '#')
		    FIXME("fname = %s\n", fname);
		fname[len-1] = '\0';
		(*import)->name = TLB_MultiByteToBSTR(fname);
	    }
	    (*ppRefType)->pImpTLInfo = *import;
	} else { /* internal ref */
	  (*ppRefType)->pImpTLInfo = TLB_REF_INTERNAL;
	}
	(*ppRefType)->reference = ref;
	(*ppRefType)->index = type_num;

	HeapFree(GetProcessHeap(), 0, refname);
	ppRefType = &(*ppRefType)->next;
    }
    if((BYTE)*name != SLTG_REF_MAGIC)
      FIXME("End of ref block magic = %x\n", *name);
    dump_TLBRefType(pTI->reflist);
}

static char *SLTG_DoImpls(char *pBlk, ITypeInfoImpl *pTI,
			  BOOL OneOnly)
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
	(*ppImplType)->hRef = info->ref;
	(*ppImplType)->implflags = info->impltypeflags;
	pTI->TypeAttr.cImplTypes++;
	ppImplType = &(*ppImplType)->next;

        if(info->next == 0xffff)
	    break;
	if(OneOnly)
	    FIXME("Interface inheriting more than one interface\n");
	info = (SLTG_ImplInfo*)(pBlk + info->next);
    }
    info++; /* see comment at top of function */
    return (char*)info;
}

static SLTG_TypeInfoTail *SLTG_ProcessCoClass(char *pBlk, ITypeInfoImpl *pTI,
					      char *pNameTable)
{
    SLTG_TypeInfoHeader *pTIHeader = (SLTG_TypeInfoHeader*)pBlk;
    SLTG_MemberHeader *pMemHeader;
    char *pFirstItem, *pNextItem;

    if(pTIHeader->href_table != 0xffffffff) {
        SLTG_DoRefs((SLTG_RefInfo*)(pBlk + pTIHeader->href_table), pTI,
		    pNameTable);
    }


    pMemHeader = (SLTG_MemberHeader*)(pBlk + pTIHeader->elem_table);

    pFirstItem = pNextItem = (char*)(pMemHeader + 1);

    if(*(WORD*)pFirstItem == SLTG_IMPL_MAGIC) {
        pNextItem = SLTG_DoImpls(pFirstItem, pTI, FALSE);
    }

    return (SLTG_TypeInfoTail*)(pFirstItem + pMemHeader->cbExtra);
}


static SLTG_TypeInfoTail *SLTG_ProcessInterface(char *pBlk, ITypeInfoImpl *pTI,
						char *pNameTable)
{
    SLTG_TypeInfoHeader *pTIHeader = (SLTG_TypeInfoHeader*)pBlk;
    SLTG_MemberHeader *pMemHeader;
    SLTG_Function *pFunc;
    char *pFirstItem, *pNextItem;
    TLBFuncDesc **ppFuncDesc = &pTI->funclist;
    int num = 0;

    if(pTIHeader->href_table != 0xffffffff) {
        SLTG_DoRefs((SLTG_RefInfo*)(pBlk + pTIHeader->href_table), pTI,
		    pNameTable);
    }

    pMemHeader = (SLTG_MemberHeader*)(pBlk + pTIHeader->elem_table);

    pFirstItem = pNextItem = (char*)(pMemHeader + 1);

    if(*(WORD*)pFirstItem == SLTG_IMPL_MAGIC) {
        pNextItem = SLTG_DoImpls(pFirstItem, pTI, TRUE);
    }

    for(pFunc = (SLTG_Function*)pNextItem, num = 1; 1;
	pFunc = (SLTG_Function*)(pFirstItem + pFunc->next), num++) {

        int param;
	WORD *pType, *pArg;

	if(pFunc->magic != SLTG_FUNCTION_MAGIC &&
	   pFunc->magic != SLTG_FUNCTION_WITH_FLAGS_MAGIC) {
	    FIXME("func magic = %02x\n", pFunc->magic);
	    return NULL;
	}
	*ppFuncDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				sizeof(**ppFuncDesc));
	(*ppFuncDesc)->Name = TLB_MultiByteToBSTR(pFunc->name + pNameTable);

	(*ppFuncDesc)->funcdesc.memid = pFunc->dispid;
	(*ppFuncDesc)->funcdesc.invkind = pFunc->inv >> 4;
	(*ppFuncDesc)->funcdesc.callconv = pFunc->nacc & 0x7;
	(*ppFuncDesc)->funcdesc.cParams = pFunc->nacc >> 3;
	(*ppFuncDesc)->funcdesc.cParamsOpt = (pFunc->retnextopt & 0x7e) >> 1;
	(*ppFuncDesc)->funcdesc.oVft = pFunc->vtblpos;

	if(pFunc->magic == SLTG_FUNCTION_WITH_FLAGS_MAGIC)
	    (*ppFuncDesc)->funcdesc.wFuncFlags = pFunc->funcflags;

	if(pFunc->retnextopt & 0x80)
	    pType = &pFunc->rettype;
	else
	    pType = (WORD*)(pFirstItem + pFunc->rettype);


	SLTG_DoType(pType, pFirstItem, &(*ppFuncDesc)->funcdesc.elemdescFunc);

	(*ppFuncDesc)->funcdesc.lprgelemdescParam =
	  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		    (*ppFuncDesc)->funcdesc.cParams * sizeof(ELEMDESC));
	(*ppFuncDesc)->pParamDesc =
	  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		    (*ppFuncDesc)->funcdesc.cParams * sizeof(TLBParDesc));

	pArg = (WORD*)(pFirstItem + pFunc->arg_off);

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
	       meaning the the next WORD is an offset to the type. */

	    HaveOffs = FALSE;
	    if(*pArg == 0xffff)
	        paramName = NULL;
	    else if(*pArg == 0xfffe) {
	        paramName = NULL;
		HaveOffs = TRUE;
	    }
	    else if(!isalnum(*(paramName-1)))
	        HaveOffs = TRUE;

	    pArg++;

	    if(HaveOffs) { /* the next word is an offset to type */
	        pType = (WORD*)(pFirstItem + *pArg);
		SLTG_DoType(pType, pFirstItem,
			    &(*ppFuncDesc)->funcdesc.lprgelemdescParam[param]);
		pArg++;
	    } else {
		if(paramName)
		  paramName--;
		pArg = SLTG_DoType(pArg, pFirstItem,
			   &(*ppFuncDesc)->funcdesc.lprgelemdescParam[param]);
	    }

	    /* Are we an optional param ? */
	    if((*ppFuncDesc)->funcdesc.cParams - param <=
	       (*ppFuncDesc)->funcdesc.cParamsOpt)
	      (*ppFuncDesc)->funcdesc.lprgelemdescParam[param].u.paramdesc.wParamFlags |= PARAMFLAG_FOPT;

	    if(paramName) {
	        (*ppFuncDesc)->pParamDesc[param].Name =
		  TLB_MultiByteToBSTR(paramName);
	    }
	}

	ppFuncDesc = &((*ppFuncDesc)->next);
	if(pFunc->next == 0xffff) break;
    }
    pTI->TypeAttr.cFuncs = num;
    dump_TLBFuncDesc(pTI->funclist);
    return (SLTG_TypeInfoTail*)(pFirstItem + pMemHeader->cbExtra);
}

static SLTG_TypeInfoTail *SLTG_ProcessRecord(char *pBlk, ITypeInfoImpl *pTI,
					     char *pNameTable)
{
  SLTG_TypeInfoHeader *pTIHeader = (SLTG_TypeInfoHeader*)pBlk;
  SLTG_MemberHeader *pMemHeader;
  SLTG_RecordItem *pItem;
  char *pFirstItem;
  TLBVarDesc **ppVarDesc = &pTI->varlist;
  int num = 0;
  WORD *pType;
  char buf[300];

  pMemHeader = (SLTG_MemberHeader*)(pBlk + pTIHeader->elem_table);

  pFirstItem = (char*)(pMemHeader + 1);
  for(pItem = (SLTG_RecordItem *)pFirstItem, num = 1; 1;
      pItem = (SLTG_RecordItem *)(pFirstItem + pItem->next), num++) {
      if(pItem->magic != SLTG_RECORD_MAGIC) {
	  FIXME("record magic = %02x\n", pItem->magic);
	  return NULL;
      }
      *ppVarDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			     sizeof(**ppVarDesc));
      (*ppVarDesc)->Name = TLB_MultiByteToBSTR(pItem->name + pNameTable);
      (*ppVarDesc)->vardesc.memid = pItem->memid;
      (*ppVarDesc)->vardesc.u.oInst = pItem->byte_offs;
      (*ppVarDesc)->vardesc.varkind = VAR_PERINSTANCE;

      if(pItem->typepos == 0x02)
	  pType = &pItem->type;
      else if(pItem->typepos == 0x00)
	  pType = (WORD*)(pFirstItem + pItem->type);
      else {
	  FIXME("typepos = %02x\n", pItem->typepos);
	  break;
      }

      SLTG_DoType(pType, pFirstItem,
		  &(*ppVarDesc)->vardesc.elemdescVar);

      /* FIXME("helpcontext, helpstring\n"); */

      dump_TypeDesc(&(*ppVarDesc)->vardesc.elemdescVar.tdesc, buf);

      ppVarDesc = &((*ppVarDesc)->next);
      if(pItem->next == 0xffff) break;
  }
  pTI->TypeAttr.cVars = num;
  return (SLTG_TypeInfoTail*)(pFirstItem + pMemHeader->cbExtra);
}

static SLTG_TypeInfoTail *SLTG_ProcessAlias(char *pBlk, ITypeInfoImpl *pTI,
					   char *pNameTable)
{
  SLTG_TypeInfoHeader *pTIHeader = (SLTG_TypeInfoHeader*)pBlk;
  SLTG_MemberHeader *pMemHeader;
  SLTG_AliasItem *pItem;
  int i, mustbelast;

  pMemHeader = (SLTG_MemberHeader*)(pBlk + pTIHeader->elem_table);
  pItem = (SLTG_AliasItem*)(pMemHeader + 1);

  mustbelast = 0;
  /* This is used for creating a TYPEDESC chain in case of VT_USERDEFINED */
  for (i = 0 ; i<pMemHeader->cbExtra/4 ; i++) {
    if (pItem->vt == 0xffff) {
      if (i<(pMemHeader->cbExtra/4-1))
	FIXME("Endmarker too early in process alias data!\n");
      break;
    }
    if (mustbelast) {
      FIXME("Chain extends over last entry?\n");
      break;
    }
    if (pItem->vt == VT_USERDEFINED) {
      pTI->TypeAttr.tdescAlias.vt = pItem->vt;
      /* guessing here ... */
      FIXME("Guessing TKIND_ALIAS of VT_USERDEFINED with hreftype 0x%x\n",pItem->res02);
      pTI->TypeAttr.tdescAlias.u.hreftype = pItem->res02;
      mustbelast = 1;
    } else {
      FIXME("alias %d: 0x%x\n",i,pItem->vt);
      FIXME("alias %d: 0x%x\n",i,pItem->res02);
    }
    pItem++;
  }
  return (SLTG_TypeInfoTail*)((char*)(pMemHeader + 1)+pMemHeader->cbExtra);
}

static SLTG_TypeInfoTail *SLTG_ProcessDispatch(char *pBlk, ITypeInfoImpl *pTI,
					   char *pNameTable)
{
  SLTG_TypeInfoHeader *pTIHeader = (SLTG_TypeInfoHeader*)pBlk;
  SLTG_MemberHeader *pMemHeader;
  SLTG_AliasItem *pItem;

  pMemHeader = (SLTG_MemberHeader*)(pBlk + pTIHeader->elem_table);
  pItem = (SLTG_AliasItem*)(pMemHeader + 1);
  FIXME("memh.cbExtra is %ld\n",pMemHeader->cbExtra);
  FIXME("offset 0 0x%x\n",*(WORD*)pItem);
  return (SLTG_TypeInfoTail*)((char*)(pMemHeader + 1)+pMemHeader->cbExtra);
}

static SLTG_TypeInfoTail *SLTG_ProcessEnum(char *pBlk, ITypeInfoImpl *pTI,
					   char *pNameTable)
{
  SLTG_TypeInfoHeader *pTIHeader = (SLTG_TypeInfoHeader*)pBlk;
  SLTG_MemberHeader *pMemHeader;
  SLTG_EnumItem *pItem;
  char *pFirstItem;
  TLBVarDesc **ppVarDesc = &pTI->varlist;
  int num = 0;

  pMemHeader = (SLTG_MemberHeader*)(pBlk + pTIHeader->elem_table);

  pFirstItem = (char*)(pMemHeader + 1);
  for(pItem = (SLTG_EnumItem *)pFirstItem, num = 1; 1;
      pItem = (SLTG_EnumItem *)(pFirstItem + pItem->next), num++) {
      if(pItem->magic != SLTG_ENUMITEM_MAGIC) {
	  FIXME("enumitem magic = %04x\n", pItem->magic);
	  return NULL;
      }
      *ppVarDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			     sizeof(**ppVarDesc));
      (*ppVarDesc)->Name = TLB_MultiByteToBSTR(pItem->name + pNameTable);
      (*ppVarDesc)->vardesc.memid = pItem->memid;
      (*ppVarDesc)->vardesc.u.lpvarValue = HeapAlloc(GetProcessHeap(), 0,
						     sizeof(VARIANT));
      V_VT((*ppVarDesc)->vardesc.u.lpvarValue) = VT_INT;
      V_UNION((*ppVarDesc)->vardesc.u.lpvarValue, intVal) =
	*(INT*)(pItem->value + pFirstItem);
      (*ppVarDesc)->vardesc.elemdescVar.tdesc.vt = VT_I4;
      (*ppVarDesc)->vardesc.varkind = VAR_CONST;
      /* FIXME("helpcontext, helpstring\n"); */

      ppVarDesc = &((*ppVarDesc)->next);
      if(pItem->next == 0xffff) break;
  }
  pTI->TypeAttr.cVars = num;
  return (SLTG_TypeInfoTail*)(pFirstItem + pMemHeader->cbExtra);
}

/* Because SLTG_OtherTypeInfo is such a painfull struct, we make a more
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

    TRACE("%p, TLB length = %ld\n", pLib, dwTLBLength);

    pTypeLibImpl = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ITypeLibImpl));
    if (!pTypeLibImpl) return NULL;

    pTypeLibImpl->lpVtbl = &tlbvt;
    pTypeLibImpl->ref = 1;

    pHeader = pLib;

    TRACE("header:\n");
    TRACE("\tmagic=0x%08lx, file blocks = %d\n", pHeader->SLTG_magic,
	  pHeader->nrOfFileBlks );
    if (pHeader->SLTG_magic != SLTG_SIGNATURE) {
	FIXME("Header type magic 0x%08lx not supported.\n",
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
        FIXME("CompObj magic = %s\n", pMagic->CompObj_magic);
	return NULL;
    }
    if(memcmp(pMagic->dir_magic, SLTG_DIR_MAGIC,
	      sizeof(SLTG_DIR_MAGIC))) {
        FIXME("dir magic = %s\n", pMagic->dir_magic);
	return NULL;
    }

    pIndex = (SLTG_Index*)(pMagic+1);

    pPad9 = (SLTG_Pad9*)(pIndex + pTypeLibImpl->TypeInfoCount);

    pFirstBlk = (LPVOID)(pPad9 + 1);

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
	    TRACE("\twith %s\n", debugstr_an(ptr + 6 + len, w));
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
       FIXME("pNameTable jump = %x\n", *(WORD*)pNameTable);
       break;
   }

    pNameTable += 0x216;

    pNameTable += 2;

    TRACE("Library name is %s\n", pNameTable + pLibBlk->name);

    pTypeLibImpl->Name = TLB_MultiByteToBSTR(pNameTable + pLibBlk->name);


    /* Hopefully we now have enough ptrs set up to actually read in
       some TypeInfos.  It's not clear which order to do them in, so
       I'll just follow the links along the BlkEntry chain and read
       them in in the order in which they're in the file */

    ppTypeInfoImpl = &(pTypeLibImpl->pTypeInfo);

    for(pBlk = pFirstBlk, order = pHeader->first_blk - 1, i = 0;
	pBlkEntry[order].next != 0;
	order = pBlkEntry[order].next - 1, i++) {

      SLTG_TypeInfoHeader *pTIHeader;
      SLTG_TypeInfoTail *pTITail;

      if(strcmp(pBlkEntry[order].index_string + (char*)pMagic,
		pOtherTypeInfoBlks[i].index_name)) {
	FIXME("Index strings don't match\n");
	return NULL;
      }

      pTIHeader = pBlk;
      if(pTIHeader->magic != SLTG_TIHEADER_MAGIC) {
	FIXME("TypeInfoHeader magic = %04x\n", pTIHeader->magic);
	return NULL;
      }
      *ppTypeInfoImpl = (ITypeInfoImpl*)ITypeInfo_Constructor();
      (*ppTypeInfoImpl)->pTypeLib = pTypeLibImpl;
      (*ppTypeInfoImpl)->index = i;
      (*ppTypeInfoImpl)->Name = TLB_MultiByteToBSTR(
					     pOtherTypeInfoBlks[i].name_offs +
					     pNameTable);
      (*ppTypeInfoImpl)->dwHelpContext = pOtherTypeInfoBlks[i].helpcontext;
      memcpy(&((*ppTypeInfoImpl)->TypeAttr.guid), &pOtherTypeInfoBlks[i].uuid,
	     sizeof(GUID));
      (*ppTypeInfoImpl)->TypeAttr.typekind = pTIHeader->typekind;
      (*ppTypeInfoImpl)->TypeAttr.wMajorVerNum = pTIHeader->major_version;
      (*ppTypeInfoImpl)->TypeAttr.wMinorVerNum = pTIHeader->minor_version;
      (*ppTypeInfoImpl)->TypeAttr.wTypeFlags =
	(pTIHeader->typeflags1 >> 3) | (pTIHeader->typeflags2 << 5);

      if((pTIHeader->typeflags1 & 7) != 2)
	FIXME("typeflags1 = %02x\n", pTIHeader->typeflags1);
      if(pTIHeader->typeflags3 != 2)
	FIXME("typeflags3 = %02x\n", pTIHeader->typeflags3);

      TRACE("TypeInfo %s of kind %s guid %s typeflags %04x\n",
	    debugstr_w((*ppTypeInfoImpl)->Name),
	    typekind_desc[pTIHeader->typekind],
	    debugstr_guid(&(*ppTypeInfoImpl)->TypeAttr.guid),
	    (*ppTypeInfoImpl)->TypeAttr.wTypeFlags);

      switch(pTIHeader->typekind) {
      case TKIND_ENUM:
	pTITail = SLTG_ProcessEnum(pBlk, *ppTypeInfoImpl, pNameTable);
	break;

      case TKIND_RECORD:
	pTITail = SLTG_ProcessRecord(pBlk, *ppTypeInfoImpl, pNameTable);
	break;

      case TKIND_INTERFACE:
	pTITail = SLTG_ProcessInterface(pBlk, *ppTypeInfoImpl, pNameTable);
	break;

      case TKIND_COCLASS:
	pTITail = SLTG_ProcessCoClass(pBlk, *ppTypeInfoImpl, pNameTable);
	break;

      case TKIND_ALIAS:
	pTITail = SLTG_ProcessAlias(pBlk, *ppTypeInfoImpl, pNameTable);
	if (pTITail->tdescalias_vt)
	  (*ppTypeInfoImpl)->TypeAttr.tdescAlias.vt = pTITail->tdescalias_vt;
	break;

      case TKIND_DISPATCH:
	pTITail = SLTG_ProcessDispatch(pBlk, *ppTypeInfoImpl, pNameTable);
	break;

      default:
	FIXME("Not processing typekind %d\n", pTIHeader->typekind);
	pTITail = NULL;
	break;

      }

      if(pTITail) { /* could get cFuncs, cVars and cImplTypes from here
		       but we've already set those */
	  (*ppTypeInfoImpl)->TypeAttr.cbAlignment = pTITail->cbAlignment;
	  (*ppTypeInfoImpl)->TypeAttr.cbSizeInstance = pTITail->cbSizeInstance;
	  (*ppTypeInfoImpl)->TypeAttr.cbSizeVft = pTITail->cbSizeVft;

#define X(x) TRACE("tt "#x": %x\n",pTITail->res##x);
	  X(06);
	  X(08);
	  X(0a);
	  X(0c);
	  X(0e);
	  X(10);
	  X(12);
	  X(16);
	  X(18);
	  X(1a);
	  X(1c);
	  X(1e);
	  X(24);
	  X(26);
	  X(2a);
	  X(2c);
	  X(2e);
	  X(30);
	  X(32);
	  X(34);
      }
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
    ICOM_THIS( ITypeLibImpl, iface);

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
    ICOM_THIS( ITypeLibImpl, iface);

    TRACE("(%p)->ref was %u\n",This, This->ref);

    return ++(This->ref);
}

/* ITypeLib::Release
 */
static ULONG WINAPI ITypeLib2_fnRelease( ITypeLib2 *iface)
{
    ICOM_THIS( ITypeLibImpl, iface);

    --(This->ref);

    TRACE("(%p)->(%u)\n",This, This->ref);

    if (!This->ref)
    {
      /* remove cache entry */
      TRACE("removing from cache list\n");
      EnterCriticalSection(&cache_section);
      if (This->next) This->next->prev = This->prev;
      if (This->prev) This->prev->next = This->next;
      else tlb_cache_first = This->next;
      LeaveCriticalSection(&cache_section);

      /* FIXME destroy child objects */
      TRACE(" destroying ITypeLib(%p)\n",This);

      if (This->Name)
      {
          SysFreeString(This->Name);
          This->Name = NULL;
      }

      if (This->DocString)
      {
          SysFreeString(This->DocString);
          This->DocString = NULL;
      }

      if (This->HelpFile)
      {
          SysFreeString(This->HelpFile);
          This->HelpFile = NULL;
      }

      if (This->HelpStringDll)
      {
          SysFreeString(This->HelpStringDll);
          This->HelpStringDll = NULL;
      }

      if (This->pTypeInfo) /* can be NULL */
      	  ITypeInfo_Release((ITypeInfo*) This->pTypeInfo);
      HeapFree(GetProcessHeap(),0,This);
      return 0;
    }

    return This->ref;
}

/* ITypeLib::GetTypeInfoCount
 *
 * Returns the number of type descriptions in the type library
 */
static UINT WINAPI ITypeLib2_fnGetTypeInfoCount( ITypeLib2 *iface)
{
    ICOM_THIS( ITypeLibImpl, iface);
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
    int i;

    ICOM_THIS( ITypeLibImpl, iface);
    ITypeInfoImpl *pTypeInfo = This->pTypeInfo;

    TRACE("(%p)->(index=%d) \n", This, index);

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
    ICOM_THIS( ITypeLibImpl, iface);
    int i;
    ITypeInfoImpl *pTInfo = This->pTypeInfo;

    TRACE("(%p) index %d \n",This, index);

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
    ICOM_THIS( ITypeLibImpl, iface);
    ITypeInfoImpl *pTypeInfo = This->pTypeInfo; /* head of list */

    TRACE("(%p)\n\tguid:\t%s)\n",This,debugstr_guid(guid));

    if (!pTypeInfo) return TYPE_E_ELEMENTNOTFOUND;

    /* search linked list for guid */
    while( !IsEqualIID(guid,&pTypeInfo->TypeAttr.guid) )
    {
      pTypeInfo = pTypeInfo->next;

      if (!pTypeInfo)
      {
        /* end of list reached */
        TRACE("-- element not found\n");
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
    ICOM_THIS( ITypeLibImpl, iface);
    TRACE("(%p)\n",This);
    *ppTLibAttr = HeapAlloc(GetProcessHeap(), 0, sizeof(**ppTLibAttr));
    memcpy(*ppTLibAttr, &This->LibAttr, sizeof(**ppTLibAttr));
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
    ICOM_THIS( ITypeLibImpl, iface);

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
    ICOM_THIS( ITypeLibImpl, iface);

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
                if(!(*pBstrName = SysAllocString(This->Name))) goto memerr1;else;
            else
                *pBstrName = NULL;
        }
        if(pBstrDocString)
        {
            if (This->DocString)
                if(!(*pBstrDocString = SysAllocString(This->DocString))) goto memerr2;else;
            else if (This->Name)
                if(!(*pBstrDocString = SysAllocString(This->Name))) goto memerr2;else;
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
                if(!(*pBstrHelpFile = SysAllocString(This->HelpFile))) goto memerr3;else;
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
    ICOM_THIS( ITypeLibImpl, iface);
    ITypeInfoImpl *pTInfo;
    TLBFuncDesc *pFInfo;
    TLBVarDesc *pVInfo;
    int i;
    UINT nNameBufLen = SysStringLen(szNameBuf);

    TRACE("(%p)->(%s,%08lx,%p)\n", This, debugstr_w(szNameBuf), lHashVal,
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
    ICOM_THIS( ITypeLibImpl, iface);
    ITypeInfoImpl *pTInfo;
    TLBFuncDesc *pFInfo;
    TLBVarDesc *pVInfo;
    int i,j = 0;

    UINT nNameBufLen = SysStringLen(szNameBuf);

    for(pTInfo=This->pTypeInfo;pTInfo && j<*pcFound; pTInfo=pTInfo->next){
        if(!memcmp(szNameBuf,pTInfo->Name, nNameBufLen)) goto ITypeLib2_fnFindName_exit;
        for(pFInfo=pTInfo->funclist;pFInfo;pFInfo=pFInfo->next) {
            if(!memcmp(szNameBuf,pFInfo->Name,nNameBufLen)) goto ITypeLib2_fnFindName_exit;
            for(i=0;i<pFInfo->funcdesc.cParams;i++)
                if(!memcmp(szNameBuf,pFInfo->pParamDesc[i].Name,nNameBufLen))
                    goto ITypeLib2_fnFindName_exit;
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
    ICOM_THIS( ITypeLibImpl, iface);
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
    ICOM_THIS( ITypeLibImpl, iface);
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
    ICOM_THIS( ITypeLibImpl, iface);

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
    ICOM_THIS( ITypeLibImpl, iface);
    HRESULT result;
    ITypeInfo *pTInfo;

    FIXME("(%p) index %d lcid %ld half implemented stub!\n", This, index, lcid);

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
    ICOM_THIS( ITypeLibImpl, iface);
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
        ERR(" OUT OF MEMORY! \n");
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

static ICOM_VTABLE(ITypeLib2) tlbvt = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
    ICOM_THIS_From_ITypeComp(ITypeLibImpl, iface);

    return ITypeInfo_QueryInterface((ITypeInfo *)This, riid, ppv);
}

static ULONG WINAPI ITypeLibComp_fnAddRef(ITypeComp * iface)
{
    ICOM_THIS_From_ITypeComp(ITypeLibImpl, iface);

    return ITypeInfo_AddRef((ITypeInfo *)This);
}

static ULONG WINAPI ITypeLibComp_fnRelease(ITypeComp * iface)
{
    ICOM_THIS_From_ITypeComp(ITypeLibImpl, iface);

    return ITypeInfo_Release((ITypeInfo *)This);
}

static HRESULT WINAPI ITypeLibComp_fnBind(
    ITypeComp * iface,
    OLECHAR * szName,
    unsigned long lHash,
    unsigned short wFlags,
    ITypeInfo ** ppTInfo,
    DESCKIND * pDescKind,
    BINDPTR * pBindPtr)
{
    FIXME("(%s, %lx, 0x%x, %p, %p, %p): stub\n", debugstr_w(szName), lHash, wFlags, ppTInfo, pDescKind, pBindPtr);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITypeLibComp_fnBindType(
    ITypeComp * iface,
    OLECHAR * szName,
    unsigned long lHash,
    ITypeInfo ** ppTInfo,
    ITypeComp ** ppTComp)
{
    FIXME("(%s, %lx, %p, %p): stub\n", debugstr_w(szName), lHash, ppTInfo, ppTComp);
    return E_NOTIMPL;
}

static ICOM_VTABLE(ITypeComp) tlbtcvt =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE

    ITypeLibComp_fnQueryInterface,
    ITypeLibComp_fnAddRef,
    ITypeLibComp_fnRelease,

    ITypeLibComp_fnBind,
    ITypeLibComp_fnBindType
};

/*================== ITypeInfo(2) Methods ===================================*/
static ITypeInfo2 * WINAPI ITypeInfo_Constructor(void)
{
    ITypeInfoImpl * pTypeInfoImpl;

    pTypeInfoImpl = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ITypeInfoImpl));
    if (pTypeInfoImpl)
    {
      pTypeInfoImpl->lpVtbl = &tinfvt;
      pTypeInfoImpl->lpVtblTypeComp = &tcompvt;
      pTypeInfoImpl->ref=1;
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
    ICOM_THIS( ITypeLibImpl, iface);

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
    ICOM_THIS( ITypeInfoImpl, iface);

    ++(This->ref);

    TRACE("(%p)->ref is %u\n",This, This->ref);
    return This->ref;
}

/* ITypeInfo::Release
 */
static ULONG WINAPI ITypeInfo_fnRelease( ITypeInfo2 *iface)
{
    ICOM_THIS( ITypeInfoImpl, iface);

    --(This->ref);

    TRACE("(%p)->(%u)\n",This, This->ref);

    if (!This->ref)
    {
      FIXME("destroy child objects\n");

      TRACE("destroying ITypeInfo(%p)\n",This);
      if (This->Name)
      {
          SysFreeString(This->Name);
          This->Name = 0;
      }

      if (This->DocString)
      {
          SysFreeString(This->DocString);
          This->DocString = 0;
      }

      if (This->next)
      {
        ITypeInfo_Release((ITypeInfo*)This->next);
      }

      HeapFree(GetProcessHeap(),0,This);
      return 0;
    }
    return This->ref;
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TRACE("(%p)\n",This);
    /* FIXME: must do a copy here */
    *ppTypeAttr=&This->TypeAttr;
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
    ICOM_THIS( ITypeInfoImpl, iface);

    TRACE("(%p)->(%p) stub!\n", This, ppTComp);

    *ppTComp = (ITypeComp *)&This->lpVtblTypeComp;
    ITypeComp_AddRef(*ppTComp);
    return S_OK;
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
    ICOM_THIS( ITypeInfoImpl, iface);
    int i;
    TLBFuncDesc * pFDesc;
    TRACE("(%p) index %d\n", This, index);
    for(i=0, pFDesc=This->funclist; i!=index && pFDesc; i++, pFDesc=pFDesc->next)
        ;
    if(pFDesc){
        /* FIXME: must do a copy here */
        *ppFuncDesc=&pFDesc->funcdesc;
        return S_OK;
    }
    return E_INVALIDARG;
}

/* ITypeInfo::GetVarDesc
 *
 * Retrieves a VARDESC structure that describes the specified variable.
 *
 */
static HRESULT WINAPI ITypeInfo_fnGetVarDesc( ITypeInfo2 *iface, UINT index,
        LPVARDESC  *ppVarDesc)
{
    ICOM_THIS( ITypeInfoImpl, iface);
    int i;
    TLBVarDesc * pVDesc;
    TRACE("(%p) index %d\n", This, index);
    for(i=0, pVDesc=This->varlist; i!=index && pVDesc; i++, pVDesc=pVDesc->next)
        ;
    if(pVDesc){
        /* FIXME: must do a copy here */
        *ppVarDesc=&pVDesc->vardesc;
        return S_OK;
    }
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBFuncDesc * pFDesc;
    TLBVarDesc * pVDesc;
    int i;
    TRACE("(%p) memid=0x%08lx Maxname=%d\n", This, memid, cMaxNames);
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
        if(This->TypeAttr.cImplTypes &&
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
    ICOM_THIS( ITypeInfoImpl, iface);
    int(i);
    TLBImplType *pImpl = This->impltypelist;

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
        if (!pImpl) return TYPE_E_ELEMENTNOTFOUND;
        *pRefType = pImpl->hRef;
      }
    }
    else
    {
      /* get element n from linked list */
      for(i=0; pImpl && i<index; i++)
      {
        pImpl = pImpl->next;
      }

      if (!pImpl) return TYPE_E_ELEMENTNOTFOUND;

      *pRefType = pImpl->hRef;

      TRACE("-- 0x%08lx\n", pImpl->hRef );
    }

    return S_OK;

}

/* ITypeInfo::GetImplTypeFlags
 *
 * Retrieves the IMPLTYPEFLAGS enumeration for one implemented interface
 * or base interface in a type description.
 */
static HRESULT WINAPI ITypeInfo_fnGetImplTypeFlags( ITypeInfo2 *iface,
        UINT index, INT  *pImplTypeFlags)
{
    ICOM_THIS( ITypeInfoImpl, iface);
    int i;
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBFuncDesc * pFDesc;
    TLBVarDesc * pVDesc;
    HRESULT ret=S_OK;

    TRACE("(%p) Name %s cNames %d\n", This, debugstr_w(*rgszNames),
            cNames);
    for(pFDesc=This->funclist; pFDesc; pFDesc=pFDesc->next) {
        int i, j;
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
            return ret;
        }
    }
    for(pVDesc=This->varlist; pVDesc; pVDesc=pVDesc->next) {
        if(!lstrcmpiW(*rgszNames, pVDesc->Name)) {
            if(cNames) *pMemId=pVDesc->vardesc.memid;
            return ret;
        }
    }
    /* not found, see if this is and interface with an inheritance */
    if(This->TypeAttr.cImplTypes &&
       (This->TypeAttr.typekind==TKIND_INTERFACE || This->TypeAttr.typekind==TKIND_DISPATCH)) {
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
	for (i=0;i<nrargs;i++) TRACE("%08lx,",args[i]);
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
    TRACE("returns %08lx\n",res);
    return res;
}

extern int const _argsize(DWORD vt);

/****************************************************************************
 * Helper functions for Dispcall / Invoke, which copies one variant
 * with target type onto the argument stack.
 */
static HRESULT
_copy_arg(	ITypeInfo2 *tinfo, TYPEDESC *tdesc,
		DWORD *argpos, VARIANT *arg, VARTYPE vt
) {
    UINT arglen = _argsize(vt)*sizeof(DWORD);
    VARTYPE	oldvt;
    VARIANT	va;

    if ((vt==VT_PTR) && tdesc && (tdesc->u.lptdesc->vt == VT_VARIANT)) {
	memcpy(argpos,&arg,sizeof(void*));
	return S_OK;
    }

    if (V_VT(arg) == vt) {
	memcpy(argpos, &V_UNION(arg,lVal), arglen);
	return S_OK;
    }

    if (V_ISARRAY(arg) && (vt == VT_SAFEARRAY)) {
	memcpy(argpos, &V_UNION(arg,parray), sizeof(SAFEARRAY*));
	return S_OK;
    }

    if (vt == VT_VARIANT) {
	memcpy(argpos, arg, arglen);
	return S_OK;
    }
    /* Deref BYREF vars if there is need */
    if(V_ISBYREF(arg) && ((V_VT(arg) & ~VT_BYREF)==vt)) {
        memcpy(argpos,(void*)V_UNION(arg,lVal), arglen);
	return S_OK;
    }
    if (vt==VT_UNKNOWN && V_VT(arg)==VT_DISPATCH) {
    	/* in this context, if the type lib specifies IUnknown*, giving an IDispatch* is correct; so, don't invoke VariantChangeType */
    	memcpy(argpos,&V_UNION(arg,lVal), arglen);
	return S_OK;
    }
    if ((vt == VT_PTR) && tdesc)
	return _copy_arg(tinfo, tdesc->u.lptdesc, argpos, arg, tdesc->u.lptdesc->vt);

    if ((vt == VT_USERDEFINED) && tdesc && tinfo) {
	ITypeInfo	*tinfo2;
	TYPEATTR	*tattr;
	HRESULT		hres;

	hres = ITypeInfo_GetRefTypeInfo(tinfo,tdesc->u.hreftype,&tinfo2);
	if (hres) {
	    FIXME("Could not get typeinfo of hreftype %lx for VT_USERDEFINED, while coercing from vt 0x%x. Copying 4 byte.\n",tdesc->u.hreftype,V_VT(arg));
	    memcpy(argpos, &V_UNION(arg,lVal), 4);
	    return S_OK;
	}
	ITypeInfo_GetTypeAttr(tinfo2,&tattr);
	switch (tattr->typekind) {
	case TKIND_ENUM:
          switch ( V_VT( arg ) ) {
          case VT_I2:
             *argpos = V_UNION(arg,iVal);
             return S_OK;
          case VT_I4:
             memcpy(argpos, &V_UNION(arg,lVal), 4);
             return S_OK;
          default:
             FIXME("vt 0x%x -> TKIND_ENUM unhandled.\n",V_VT(arg));
             break;
          }

	case TKIND_ALIAS:
	    tdesc = &(tattr->tdescAlias);
	    hres = _copy_arg((ITypeInfo2*)tinfo2, tdesc, argpos, arg, tdesc->vt);
	    ITypeInfo_Release(tinfo2);
	    return hres;

	case TKIND_INTERFACE:
	    if (V_VT(arg) == VT_DISPATCH) {
		IDispatch *disp;
		if (IsEqualIID(&IID_IDispatch,&(tattr->guid))) {
		    memcpy(argpos, &V_UNION(arg,pdispVal), 4);
		    return S_OK;
		}
		hres=IUnknown_QueryInterface(V_UNION(arg,pdispVal),&IID_IDispatch,(LPVOID*)&disp);
		if (SUCCEEDED(hres)) {
		    memcpy(argpos,&disp,4);
		    IUnknown_Release(V_UNION(arg,pdispVal));
		    return S_OK;
		}
		FIXME("Failed to query IDispatch interface from %s while converting to VT_DISPATCH!\n",debugstr_guid(&(tattr->guid)));
		return E_FAIL;
	    }
	    if (V_VT(arg) == VT_UNKNOWN) {
		memcpy(argpos, &V_UNION(arg,punkVal), 4);
		return S_OK;
	    }
	    FIXME("vt 0x%x -> TKIND_INTERFACE(%s) unhandled\n",V_VT(arg),debugstr_guid(&(tattr->guid)));
	    break;
	case TKIND_DISPATCH:
	    if (V_VT(arg) == VT_DISPATCH) {
		memcpy(argpos, &V_UNION(arg,pdispVal), 4);
		return S_OK;
	    }
	    FIXME("TKIND_DISPATCH unhandled for target vt 0x%x.\n",V_VT(arg));
	    break;
	case TKIND_RECORD:
	    FIXME("TKIND_RECORD unhandled.\n");
	    break;
	default:
	    FIXME("TKIND %d unhandled.\n",tattr->typekind);
	    break;
	}
	return E_FAIL;
    }

    oldvt = V_VT(arg);
    VariantInit(&va);
    if (VariantChangeType(&va,arg,0,vt)==S_OK) {
	memcpy(argpos,&V_UNION(&va,lVal), arglen);
	FIXME("Should not use VariantChangeType here. (conversion from 0x%x -> 0x%x)\n",
		V_VT(arg), vt
	);
	return S_OK;
    }
    ERR("Set arg to disparg type 0x%x vs 0x%x\n",V_VT(arg),vt);
    return E_FAIL;
}

/***********************************************************************
 *		DispCallFunc (OLEAUT32.@)
 */
HRESULT WINAPI
DispCallFunc(
    void* pvInstance, ULONG oVft, CALLCONV cc, VARTYPE vtReturn, UINT cActuals,
    VARTYPE* prgvt, VARIANTARG** prgpvarg, VARIANT* pvargResult
) {
    int i, argsize, argspos;
    DWORD *args;
    HRESULT hres;

    TRACE("(%p, %ld, %d, %d, %d, %p, %p, %p (vt=%d))\n",
	pvInstance, oVft, cc, vtReturn, cActuals, prgvt, prgpvarg, pvargResult, V_VT(pvargResult)
    );
    /* DispCallFunc is only used to invoke methods belonging to an IDispatch-derived COM interface.
    So we need to add a first parameter to the list of arguments, to supply the interface pointer */
    argsize = 1;
    for (i=0;i<cActuals;i++) {
	TRACE("arg %d: type %d, size %d\n",i,prgvt[i],_argsize(prgvt[i]));
	dump_Variant(prgpvarg[i]);
	argsize += _argsize(prgvt[i]);
    }
    args = (DWORD*)HeapAlloc(GetProcessHeap(),0,sizeof(DWORD)*argsize);
    args[0] = (DWORD)pvInstance;      /* this is the fake IDispatch interface pointer */
    argspos = 1;
    for (i=0;i<cActuals;i++) {
	VARIANT *arg = prgpvarg[i];
	TRACE("Storing arg %d (%d as %d)\n",i,V_VT(arg),prgvt[i]);
	_copy_arg(NULL, NULL, &args[argspos], arg, prgvt[i]);
	argspos += _argsize(prgvt[i]);
    }

    if(pvargResult!=NULL && V_VT(pvargResult)==VT_EMPTY)
    {
        _invoke((*(FARPROC**)pvInstance)[oVft/4],cc,argsize,args);
        hres=S_OK;
    }
    else
    {
        FIXME("Do not know how to handle pvargResult %p. Expect crash ...\n",pvargResult);
        hres = _invoke((*(FARPROC**)pvInstance)[oVft/4],cc,argsize,args);
        FIXME("Method returned %lx\n",hres);
    }
    HeapFree(GetProcessHeap(),0,args);
    return hres;
}

static HRESULT WINAPI ITypeInfo_fnInvoke(
    ITypeInfo2 *iface,
    VOID  *pIUnk,
    MEMBERID memid,
    UINT16 dwFlags,
    DISPPARAMS  *pDispParams,
    VARIANT  *pVarResult,
    EXCEPINFO  *pExcepInfo,
    UINT  *pArgErr)
{
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBFuncDesc * pFDesc;
    TLBVarDesc * pVDesc;
    int i;
    HRESULT hres;

    TRACE("(%p)(%p,id=%ld,flags=0x%08x,%p,%p,%p,%p) partial stub!\n",
      This,pIUnk,memid,dwFlags,pDispParams,pVarResult,pExcepInfo,pArgErr
    );
    dump_DispParms(pDispParams);

    for(pFDesc=This->funclist; pFDesc; pFDesc=pFDesc->next)
	if (pFDesc->funcdesc.memid == memid) {
	    if (pFDesc->funcdesc.invkind & dwFlags)
		break;
	}
    
    if (pFDesc) {
	if (TRACE_ON(typelib)) dump_TLBFuncDescOne(pFDesc);
	/* dump_FUNCDESC(&pFDesc->funcdesc);*/
	switch (pFDesc->funcdesc.funckind) {
	case FUNC_PUREVIRTUAL:
	case FUNC_VIRTUAL: {
	    DWORD res;
	    int   numargs, numargs2, argspos, args2pos;
	    DWORD *args , *args2;


	    numargs = 1; numargs2 = 0;
	    for (i=0;i<pFDesc->funcdesc.cParams;i++) {
		if (i<pDispParams->cArgs)
		    numargs += _argsize(pFDesc->funcdesc.lprgelemdescParam[i].tdesc.vt);
		else {
		    numargs	+= 1; /* sizeof(lpvoid) */
		    numargs2	+= _argsize(pFDesc->funcdesc.lprgelemdescParam[i].tdesc.vt);
		}
	    }

	    args = (DWORD*)HeapAlloc(GetProcessHeap(),0,sizeof(DWORD)*numargs);
	    args2 = (DWORD*)HeapAlloc(GetProcessHeap(),0,sizeof(DWORD)*numargs2);

	    args[0] = (DWORD)pIUnk;
	    argspos = 1; args2pos = 0;
	    for (i=0;i<pFDesc->funcdesc.cParams;i++) {
		int arglen = _argsize(pFDesc->funcdesc.lprgelemdescParam[i].tdesc.vt);
		if (i<pDispParams->cArgs) {
		    VARIANT *arg = &pDispParams->rgvarg[pDispParams->cArgs-i-1];
		    TYPEDESC *tdesc = &pFDesc->funcdesc.lprgelemdescParam[i].tdesc;
		    hres = _copy_arg(iface, tdesc, &args[argspos], arg, tdesc->vt);
		    if (FAILED(hres)) return hres;
		    argspos += arglen;
		} else {
		    TYPEDESC *tdesc = &(pFDesc->funcdesc.lprgelemdescParam[i].tdesc);
		    if (tdesc->vt != VT_PTR)
		    	FIXME("set %d to pointer for get (type is %d)\n",i,tdesc->vt);
		    /*FIXME: give pointers for the rest, so propertyget works*/
		    args[argspos] = (DWORD)&args2[args2pos];

		    /* If pointer to variant, pass reference it. */
		    if ((tdesc->vt == VT_PTR) &&
			(tdesc->u.lptdesc->vt == VT_VARIANT) &&
			pVarResult
		    )
			args[argspos]= (DWORD)pVarResult;
		    argspos	+= 1;
		    args2pos	+= arglen;
		}
	    }
	    if (pFDesc->funcdesc.cParamsOpt)
		FIXME("Does not support optional parameters (%d)\n",
			pFDesc->funcdesc.cParamsOpt
		);

	    res = _invoke((*(FARPROC**)pIUnk)[pFDesc->funcdesc.oVft/4],
		    pFDesc->funcdesc.callconv,
		    numargs,
		    args
	    );
	    if (pVarResult && (dwFlags & (DISPATCH_PROPERTYGET))) {
		args2pos = 0;
		for (i=0;i<pFDesc->funcdesc.cParams-pDispParams->cArgs;i++) {
		    int arglen = _argsize(pFDesc->funcdesc.lprgelemdescParam[i].tdesc.vt);
		    TYPEDESC *tdesc = &(pFDesc->funcdesc.lprgelemdescParam[i+pDispParams->cArgs].tdesc);
                   TYPEDESC i4_tdesc;
                   i4_tdesc.vt = VT_I4;

		    /* If we are a pointer to a variant, we are done already */
		    if ((tdesc->vt==VT_PTR)&&(tdesc->u.lptdesc->vt==VT_VARIANT))
			continue;

		    VariantInit(pVarResult);
		    memcpy(&V_UNION(pVarResult,intVal),&args2[args2pos],arglen*sizeof(DWORD));

		    if (tdesc->vt == VT_PTR)
			tdesc = tdesc->u.lptdesc;
		    if (tdesc->vt == VT_USERDEFINED) {
			ITypeInfo	*tinfo2;
			TYPEATTR	*tattr;

			hres = ITypeInfo_GetRefTypeInfo(iface,tdesc->u.hreftype,&tinfo2);
			if (hres) {
			    FIXME("Could not get typeinfo of hreftype %lx for VT_USERDEFINED, while coercing. Copying 4 byte.\n",tdesc->u.hreftype);
			    return E_FAIL;
			}
			ITypeInfo_GetTypeAttr(tinfo2,&tattr);
			switch (tattr->typekind) {
			case TKIND_ENUM:
                           /* force the return type to be VT_I4 */
                           tdesc = &i4_tdesc;
			    break;
			case TKIND_ALIAS:
			    TRACE("TKIND_ALIAS to vt 0x%x\n",tattr->tdescAlias.vt);
			    tdesc = &(tattr->tdescAlias);
			    break;

			case TKIND_INTERFACE:
			    FIXME("TKIND_INTERFACE unhandled.\n");
			    break;
			case TKIND_DISPATCH:
			    FIXME("TKIND_DISPATCH unhandled.\n");
			    break;
			case TKIND_RECORD:
			    FIXME("TKIND_RECORD unhandled.\n");
			    break;
			default:
			    FIXME("TKIND %d unhandled.\n",tattr->typekind);
			    break;
			}
		        ITypeInfo_Release(tinfo2);
		    }
		    V_VT(pVarResult) = tdesc->vt;

		    /* HACK: VB5 likes this.
		     * I do not know why. There is 1 example in MSDN which uses
		     * this which appears broken (mixes int vals and
		     * IDispatch*.).
		     */
		    if ((tdesc->vt == VT_PTR) && (dwFlags & DISPATCH_METHOD))
			V_VT(pVarResult) = VT_DISPATCH;
		    TRACE("storing into variant:\n");
		    dump_Variant(pVarResult);
		    args2pos += arglen;
		}
	    }
	    HeapFree(GetProcessHeap(),0,args2);
	    HeapFree(GetProcessHeap(),0,args);
	    return S_OK;
	}
	case FUNC_DISPATCH:  {
	   IDispatch *disp;
	   HRESULT hr;

	   hr = IUnknown_QueryInterface((LPUNKNOWN)pIUnk,&IID_IDispatch,(LPVOID*)&disp);
	   if (hr) {
	       FIXME("FUNC_DISPATCH used on object without IDispatch iface?\n");
	       return hr;
	   }
	   FIXME("Calling Invoke in IDispatch iface. untested!\n");
	   hr = IDispatch_Invoke(
	       disp,memid,&IID_NULL,LOCALE_USER_DEFAULT,dwFlags,pDispParams,
	       pVarResult,pExcepInfo,pArgErr
	   );
	   if (hr)
	       FIXME("IDispatch::Invoke failed with %08lx. (Could be not a real error?)\n",hr);
	   IDispatch_Release(disp);
	   return hr;
	}
	default:
	   FIXME("Unknown function invocation type %d\n",pFDesc->funcdesc.funckind);
	   return E_FAIL;
	}
    } else {
	for(pVDesc=This->varlist; pVDesc; pVDesc=pVDesc->next) {
	    if (pVDesc->vardesc.memid == memid) {
		FIXME("varseek: Found memid name %s, but variable-based invoking not supported\n",debugstr_w(((LPWSTR)pVDesc->Name)));
		dump_TLBVarDesc(pVDesc);
		break;
	    }
	}
    }
    /* not found, look for it in inherited interfaces */
    if (This->TypeAttr.cImplTypes &&
	(This->TypeAttr.typekind==TKIND_INTERFACE || This->TypeAttr.typekind==TKIND_DISPATCH)) {
        /* recursive search */
        ITypeInfo *pTInfo;
        HRESULT hr;
        hr=ITypeInfo_GetRefTypeInfo(iface, This->impltypelist->hRef, &pTInfo);
        if(SUCCEEDED(hr)){
            hr=ITypeInfo_Invoke(pTInfo,pIUnk,memid,dwFlags,pDispParams,pVarResult,pExcepInfo,pArgErr);
            ITypeInfo_Release(pTInfo);
            return hr;
        }
        WARN("Could not search inherited interface!\n");
    }
    ERR("did not find member id %d, flags %d!\n", (int)memid, dwFlags);
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBFuncDesc * pFDesc;
    TLBVarDesc * pVDesc;
    TRACE("(%p) memid %ld Name(%p) DocString(%p)"
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBFuncDesc *pFDesc;

    FIXME("(%p, memid %lx, %d, %p, %p, %p), partial stub!\n", This, memid, invKind, pBstrDllName, pBstrName, pwOrdinal);

    for(pFDesc=This->funclist; pFDesc; pFDesc=pFDesc->next)
        if(pFDesc->funcdesc.memid==memid){
	    dump_TypeInfo(This);
	    dump_TLBFuncDescOne(pFDesc);

	    /* FIXME: This is wrong, but how do you find that out? */
	    if (pBstrDllName) {
		const WCHAR oleaut32W[] = {'O','L','E','A','U','T','3','2','.','D','L','L',0};
		*pBstrDllName = SysAllocString(oleaut32W);
	    }

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
    ICOM_THIS( ITypeInfoImpl, iface);
    HRESULT result = E_FAIL;


    if (hRefType == -1 &&
	(((ITypeInfoImpl*) This)->TypeAttr.typekind   == TKIND_DISPATCH) &&
	(((ITypeInfoImpl*) This)->TypeAttr.wTypeFlags &  TYPEFLAG_FDUAL))
    {
	  /* when we meet a DUAL dispinterface, we must create the interface
	  * version of it.
	  */
	  ITypeInfoImpl* pTypeInfoImpl = (ITypeInfoImpl*) ITypeInfo_Constructor();


	  /* the interface version contains the same information as the dispinterface
	   * copy the contents of the structs.
	   */
	  *pTypeInfoImpl = *This;
	  pTypeInfoImpl->ref = 1;

	  /* change the type to interface */
	  pTypeInfoImpl->TypeAttr.typekind = TKIND_INTERFACE;

	  *ppTInfo = (ITypeInfo*) pTypeInfoImpl;

	  ITypeInfo_AddRef((ITypeInfo*) pTypeInfoImpl);

	  result = S_OK;

    } else {
        TLBRefType *pRefType;
        for(pRefType = This->reflist; pRefType; pRefType = pRefType->next) {
	    if(pRefType->reference == hRefType)
	        break;
	}
	if(!pRefType)
	  FIXME("Can't find pRefType for ref %lx\n", hRefType);
	if(pRefType && hRefType != -1) {
            ITypeLib *pTLib = NULL;

	    if(pRefType->pImpTLInfo == TLB_REF_INTERNAL) {
	        int Index;
		result = ITypeInfo_GetContainingTypeLib(iface, &pTLib, &Index);
	    } else {
	        if(pRefType->pImpTLInfo->pImpTypeLib) {
		    TRACE("typeinfo in imported typelib that is already loaded\n");
		    pTLib = (ITypeLib*)pRefType->pImpTLInfo->pImpTypeLib;
		    ITypeLib2_AddRef((ITypeLib*) pTLib);
		    result = S_OK;
		} else {
		    TRACE("typeinfo in imported typelib that isn't already loaded\n");
		    result = LoadRegTypeLib( &pRefType->pImpTLInfo->guid,
					     pRefType->pImpTLInfo->wVersionMajor,
					     pRefType->pImpTLInfo->wVersionMinor,
					     pRefType->pImpTLInfo->lcid,
					     &pTLib);

		    if(!SUCCEEDED(result)) {
		        BSTR libnam=SysAllocString(pRefType->pImpTLInfo->name);
			result=LoadTypeLib(libnam, &pTLib);
			SysFreeString(libnam);
		    }
		    if(SUCCEEDED(result)) {
		        pRefType->pImpTLInfo->pImpTypeLib = (ITypeLibImpl*)pTLib;
			ITypeLib2_AddRef(pTLib);
		    }
		}
	    }
	    if(SUCCEEDED(result)) {
	        if(pRefType->index == TLB_REF_USE_GUID)
		    result = ITypeLib2_GetTypeInfoOfGuid(pTLib,
							 &pRefType->guid,
							 ppTInfo);
		else
		    result = ITypeLib2_GetTypeInfo(pTLib, pRefType->index,
						   ppTInfo);
	    }
	    if (pTLib != NULL)
		ITypeLib2_Release(pTLib);
	}
    }

    TRACE("(%p) hreftype 0x%04lx loaded %s (%p)\n", This, hRefType,
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
    ICOM_THIS( ITypeInfoImpl, iface);
    FIXME("(%p) stub!\n", This);
    return S_OK;
}

/* ITypeInfo::CreateInstance
 *
 * Creates a new instance of a type that describes a component object class
 * (coclass).
 */
static HRESULT WINAPI ITypeInfo_fnCreateInstance( ITypeInfo2 *iface,
        IUnknown *pUnk, REFIID riid, VOID  **ppvObj)
{
    ICOM_THIS( ITypeInfoImpl, iface);
    FIXME("(%p) stub!\n", This);
    return S_OK;
}

/* ITypeInfo::GetMops
 *
 * Retrieves marshalling information.
 */
static HRESULT WINAPI ITypeInfo_fnGetMops( ITypeInfo2 *iface, MEMBERID memid,
				BSTR  *pBstrMops)
{
    ICOM_THIS( ITypeInfoImpl, iface);
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
    ICOM_THIS( ITypeInfoImpl, iface);
    
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TRACE("(%p)->(%p)\n", This, pTypeAttr);
}

/* ITypeInfo::ReleaseFuncDesc
 *
 * Releases a FUNCDESC previously returned by GetFuncDesc. *
 */
static void WINAPI ITypeInfo_fnReleaseFuncDesc(
	ITypeInfo2 *iface,
        FUNCDESC *pFuncDesc)
{
    ICOM_THIS( ITypeInfoImpl, iface);
    TRACE("(%p)->(%p)\n", This, pFuncDesc);
}

/* ITypeInfo::ReleaseVarDesc
 *
 * Releases a VARDESC previously returned by GetVarDesc.
 */
static void WINAPI ITypeInfo_fnReleaseVarDesc( ITypeInfo2 *iface,
        VARDESC *pVarDesc)
{
    ICOM_THIS( ITypeInfoImpl, iface);
    TRACE("(%p)->(%p)\n", This, pVarDesc);
}

/* ITypeInfo2::GetTypeKind
 *
 * Returns the TYPEKIND enumeration quickly, without doing any allocations.
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetTypeKind( ITypeInfo2 * iface,
    TYPEKIND *pTypeKind)
{
    ICOM_THIS( ITypeInfoImpl, iface);
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
    ICOM_THIS( ITypeInfoImpl, iface);
    *pTypeFlags=This->TypeAttr.wTypeFlags;
    TRACE("(%p) flags 0x%lx\n", This,*pTypeFlags);
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBFuncDesc *pFuncInfo;
    int i;
    HRESULT result;
    /* FIXME: should check for invKind??? */
    for(i=0, pFuncInfo=This->funclist;pFuncInfo &&
            memid != pFuncInfo->funcdesc.memid; i++, pFuncInfo=pFuncInfo->next);
    if(pFuncInfo){
        *pFuncIndex=i;
        result= S_OK;
    }else{
        *pFuncIndex=0;
        result=E_INVALIDARG;
    }
    TRACE("(%p) memid 0x%08lx invKind 0x%04x -> %s\n", This,
          memid, invKind, SUCCEEDED(result)? "SUCCES":"FAILED");
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBVarDesc *pVarInfo;
    int i;
    HRESULT result;
    for(i=0, pVarInfo=This->varlist; pVarInfo &&
            memid != pVarInfo->vardesc.memid; i++, pVarInfo=pVarInfo->next)
        ;
    if(pVarInfo){
        *pVarIndex=i;
        result= S_OK;
    }else{
        *pVarIndex=0;
        result=E_INVALIDARG;
    }
    TRACE("(%p) memid 0x%08lx -> %s\n", This,
          memid, SUCCEEDED(result)? "SUCCES":"FAILED");
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
    ICOM_THIS( ITypeInfoImpl, iface);
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBCustData *pCData=NULL;
    TLBFuncDesc * pFDesc;
    int i;
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBCustData *pCData=NULL;
    TLBFuncDesc * pFDesc;
    int i;

    for(i=0, pFDesc=This->funclist; i!=indexFunc && pFDesc; i++,pFDesc=pFDesc->next);

    if(pFDesc && indexParam >=0 && indexParam<pFDesc->funcdesc.cParams)
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBCustData *pCData=NULL;
    TLBVarDesc * pVDesc;
    int i;

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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBCustData *pCData=NULL;
    TLBImplType * pRDesc;
    int i;

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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBFuncDesc * pFDesc;
    TLBVarDesc * pVDesc;
    TRACE("(%p) memid %ld lcid(0x%lx)  HelpString(%p) "
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
    ICOM_THIS( ITypeInfoImpl, iface);
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
        ERR(" OUT OF MEMORY! \n");
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBCustData *pCData;
    TLBFuncDesc * pFDesc;
    int i;
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
            ERR(" OUT OF MEMORY! \n");
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBCustData *pCData=NULL;
    TLBFuncDesc * pFDesc;
    int i;
    TRACE("(%p) index %d\n", This, indexFunc);
    for(i=0, pFDesc=This->funclist; i!=indexFunc && pFDesc; i++,
            pFDesc=pFDesc->next)
        ;
    if(pFDesc && indexParam >=0 && indexParam<pFDesc->funcdesc.cParams){
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
            ERR(" OUT OF MEMORY! \n");
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBCustData *pCData;
    TLBVarDesc * pVDesc;
    int i;
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
            ERR(" OUT OF MEMORY! \n");
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
    ICOM_THIS( ITypeInfoImpl, iface);
    TLBCustData *pCData;
    TLBImplType * pRDesc;
    int i;
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
            ERR(" OUT OF MEMORY! \n");
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
    return TYPE_E_ELEMENTNOTFOUND;
}

static ICOM_VTABLE(ITypeInfo2) tinfvt =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE

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

static HRESULT WINAPI ITypeComp_fnQueryInterface(ITypeComp * iface, REFIID riid, LPVOID * ppv)
{
    ICOM_THIS_From_ITypeComp(ITypeInfoImpl, iface);

    return ITypeInfo_QueryInterface((ITypeInfo *)This, riid, ppv);
}

static ULONG WINAPI ITypeComp_fnAddRef(ITypeComp * iface)
{
    ICOM_THIS_From_ITypeComp(ITypeInfoImpl, iface);

    return ITypeInfo_AddRef((ITypeInfo *)This);
}

static ULONG WINAPI ITypeComp_fnRelease(ITypeComp * iface)
{
    ICOM_THIS_From_ITypeComp(ITypeInfoImpl, iface);

    return ITypeInfo_Release((ITypeInfo *)This);
}

static HRESULT WINAPI ITypeComp_fnBind(
    ITypeComp * iface,
    OLECHAR * szName,
    unsigned long lHash,
    unsigned short wFlags,
    ITypeInfo ** ppTInfo,
    DESCKIND * pDescKind,
    BINDPTR * pBindPtr)
{
    ICOM_THIS_From_ITypeComp(ITypeInfoImpl, iface);
    TLBFuncDesc * pFDesc;
    TLBVarDesc * pVDesc;

    TRACE("(%s, %lx, 0x%x, %p, %p, %p)\n", debugstr_w(szName), lHash, wFlags, ppTInfo, pDescKind, pBindPtr);

    for(pFDesc = This->funclist; pFDesc; pFDesc = pFDesc->next)
        if (pFDesc->funcdesc.invkind & wFlags)
            if (!strcmpW(pFDesc->Name, szName)) {
                break;
            }

    if (pFDesc)
    {
        *pDescKind = DESCKIND_FUNCDESC;
        pBindPtr->lpfuncdesc = &pFDesc->funcdesc;
        *ppTInfo = (ITypeInfo *)&This->lpVtbl;
        return S_OK;
    } else {
        if (!(wFlags & ~(INVOKE_PROPERTYGET)))
        {
            for(pVDesc = This->varlist; pVDesc; pVDesc = pVDesc->next) {
                if (!strcmpW(pVDesc->Name, szName)) {
                    *pDescKind = DESCKIND_VARDESC;
                    pBindPtr->lpvardesc = &pVDesc->vardesc;
                    *ppTInfo = (ITypeInfo *)&This->lpVtbl;
                    return S_OK;
                }
            }
        }
    }
    /* not found, look for it in inherited interfaces */
    if (This->TypeAttr.cImplTypes &&
	(This->TypeAttr.typekind == TKIND_INTERFACE || This->TypeAttr.typekind == TKIND_DISPATCH)) {
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
    ERR("did not find member with name %s, flags 0x%x!\n", debugstr_w(szName), wFlags);
    *pDescKind = DESCKIND_NONE;
    pBindPtr->lpfuncdesc = NULL;
    *ppTInfo = NULL;
    return DISP_E_MEMBERNOTFOUND;
}

static HRESULT WINAPI ITypeComp_fnBindType(
    ITypeComp * iface,
    OLECHAR * szName,
    unsigned long lHash,
    ITypeInfo ** ppTInfo,
    ITypeComp ** ppTComp)
{
    TRACE("(%s, %lx, %p, %p)\n", debugstr_w(szName), lHash, ppTInfo, ppTComp);

    /* strange behaviour (does nothing) but like the
     * original */

    if (!ppTInfo || !ppTComp)
        return E_POINTER;

    *ppTInfo = NULL;
    *ppTComp = NULL;

    return S_OK;
}

static ICOM_VTABLE(ITypeComp) tcompvt =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE

    ITypeComp_fnQueryInterface,
    ITypeComp_fnAddRef,
    ITypeComp_fnRelease,

    ITypeComp_fnBind,
    ITypeComp_fnBindType
};
