/*
 * Copyright (C) 2003 Michael Günnewig
 * Copyright (C) 2003 CodeWeavers Inc. (Ulrich Czekalla)
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "objbase.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "initguid.h"
#include "dmo.h"

WINE_DEFAULT_DEBUG_CHANNEL(msdmo);

#define MSDMO_MAJOR_VERSION 6

static const WCHAR szDMORootKey[] = 
{
    'D','i','r','e','c','t','S','h','o','w','\\',
    'M','e','d','i','a','O','b','j','e','c','t','s',0
}; 

static const WCHAR szDMOInputType[] =
{
    'I','n','p','u','t','T','y','p','e','s',0
};

static const WCHAR szDMOOutputType[] =
{
    'O','u','t','p','u','t','T','y','p','e','s',0
};

static const WCHAR szDMOKeyed[] =
{
    'K','e','y','e','d',0
};

static const WCHAR szDMOCategories[] =
{
    'C','a','t','e','g','o','r','i','e','s',0
};

static const WCHAR szGUIDFmt[] =
{
    '%','0','8','X','-','%','0','4','X','-','%','0','4','X','-','%','0',
    '2','X','%','0','2','X','-','%','0','2','X','%','0','2','X','%','0','2',
    'X','%','0','2','X','%','0','2','X','%','0','2','X',0
};

static const WCHAR szCat3Fmt[] =
{
    '%','s','\\','%','s','\\','%','s',0
};

static const WCHAR szCat2Fmt[] =
{
    '%','s','\\','%','s',0
};

static const WCHAR szToGuidFmt[] =
{
    '{','%','s','}',0
};


typedef struct
{
    const IEnumDMOVtbl         *lpVtbl;
    LONG			ref;
    DWORD			index;
    const GUID*                 guidCategory;
    DWORD                       dwFlags;
    DWORD                       cInTypes;
    DMO_PARTIAL_MEDIATYPE       *pInTypes;
    DWORD                       cOutTypes;
    DMO_PARTIAL_MEDIATYPE       *pOutTypes;
    HKEY                        hkey;
} IEnumDMOImpl;

static HRESULT read_types(HKEY root, LPCWSTR key, ULONG *supplied, ULONG requested, DMO_PARTIAL_MEDIATYPE* types);

static const IEnumDMOVtbl edmovt;

static LPWSTR GUIDToString(LPWSTR lpwstr, REFGUID lpcguid)
{
    wsprintfW(lpwstr, szGUIDFmt, lpcguid->Data1, lpcguid->Data2,
        lpcguid->Data3, lpcguid->Data4[0], lpcguid->Data4[1],
        lpcguid->Data4[2], lpcguid->Data4[3], lpcguid->Data4[4],
        lpcguid->Data4[5], lpcguid->Data4[6], lpcguid->Data4[7]);

    return lpwstr;
}

static BOOL IsMediaTypeEqual(const DMO_PARTIAL_MEDIATYPE* mt1, const DMO_PARTIAL_MEDIATYPE* mt2)
{

    return (IsEqualCLSID(&mt1->type, &mt2->type) ||
            IsEqualCLSID(&mt2->type, &GUID_NULL) ||
            IsEqualCLSID(&mt1->type, &GUID_NULL)) &&
            (IsEqualCLSID(&mt1->subtype, &mt2->subtype) ||
            IsEqualCLSID(&mt2->subtype, &GUID_NULL) ||
            IsEqualCLSID(&mt1->subtype, &GUID_NULL));
}

static HRESULT write_types(HKEY hkey, LPCWSTR name, const DMO_PARTIAL_MEDIATYPE* types, DWORD count)
{
    HRESULT hres = S_OK;
    if (MSDMO_MAJOR_VERSION > 5)
    {
        hres = RegSetValueExW(hkey, name, 0, REG_BINARY, (const BYTE*) types,
                          count* sizeof(DMO_PARTIAL_MEDIATYPE));
    }
    else
    {
        HKEY skey1,skey2,skey3;
        DWORD index = 0;
        WCHAR szGuidKey[64];

        hres = RegCreateKeyExW(hkey, name, 0, NULL, REG_OPTION_NON_VOLATILE,
                               KEY_WRITE, NULL, &skey1, NULL);
        while (index < count)
        {
            GUIDToString(szGuidKey,&types[index].type);
            hres = RegCreateKeyExW(skey1, szGuidKey, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &skey2, NULL);
            GUIDToString(szGuidKey,&types[index].subtype);
            hres = RegCreateKeyExW(skey2, szGuidKey, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &skey3, NULL);
            RegCloseKey(skey3);
            RegCloseKey(skey2);
            index ++;
        }
        RegCloseKey(skey1);
    }

    return hres;
}

/***************************************************************
 * DMORegister (MSDMO.@)
 *
 * Register a DirectX Media Object.
 */
HRESULT WINAPI DMORegister(
   LPCWSTR szName,
   REFCLSID clsidDMO,
   REFGUID guidCategory,
   DWORD dwFlags,
   DWORD cInTypes,
   const DMO_PARTIAL_MEDIATYPE *pInTypes,
   DWORD cOutTypes,
   const DMO_PARTIAL_MEDIATYPE *pOutTypes
)
{
    WCHAR szguid[64];
    HRESULT hres;
    HKEY hrkey = 0;
    HKEY hkey = 0;
    HKEY hckey = 0;
    HKEY hclskey = 0;

    TRACE("%s\n", debugstr_w(szName));

    hres = RegCreateKeyExW(HKEY_CLASSES_ROOT, szDMORootKey, 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hrkey, NULL);
    if (ERROR_SUCCESS != hres)
        goto lend;

    /* Create clsidDMO key under MediaObjects */ 
    hres = RegCreateKeyExW(hrkey, GUIDToString(szguid, clsidDMO), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);
    if (ERROR_SUCCESS != hres)
        goto lend;

    /* Set default Name value */
    hres = RegSetValueExW(hkey, NULL, 0, REG_SZ, (const BYTE*) szName, 
        (strlenW(szName) + 1) * sizeof(WCHAR));

    /* Set InputTypes */
    hres = write_types(hkey, szDMOInputType, pInTypes, cInTypes);

    /* Set OutputTypes */
    hres = write_types(hkey, szDMOOutputType, pOutTypes, cOutTypes);

    if (dwFlags & DMO_REGISTERF_IS_KEYED)
    {
        /* Create Keyed key */ 
        hres = RegCreateKeyExW(hkey, szDMOKeyed, 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hckey, NULL);
        if (ERROR_SUCCESS != hres)
            goto lend;
        RegCloseKey(hckey);
    }

    /* Register the category */
    hres = RegCreateKeyExW(hrkey, szDMOCategories, 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hckey, NULL);
    if (ERROR_SUCCESS != hres)
        goto lend;

    RegCloseKey(hkey);

    hres = RegCreateKeyExW(hckey, GUIDToString(szguid, guidCategory), 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);
    if (ERROR_SUCCESS != hres)
        goto lend;
    hres = RegCreateKeyExW(hkey, GUIDToString(szguid, clsidDMO), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hclskey, NULL);
    if (ERROR_SUCCESS != hres)
        goto lend;

lend:
    if (hkey)
        RegCloseKey(hkey);
    if (hckey)
        RegCloseKey(hckey);
    if (hclskey)
        RegCloseKey(hclskey);
    if (hrkey)
        RegCloseKey(hrkey);

    TRACE(" hresult=0x%08x\n", hres);
    return hres;
}


/***************************************************************
 * DMOUnregister (MSDMO.@)
 *
 * Unregister a DirectX Media Object.
 */
HRESULT WINAPI DMOUnregister(REFCLSID clsidDMO, REFGUID guidCategory)
{
    HRESULT hres;
    WCHAR szguid[64];
    HKEY hrkey = 0;
    HKEY hckey = 0;

    GUIDToString(szguid, clsidDMO);

    TRACE("%s %p\n", debugstr_w(szguid), guidCategory);

    hres = RegOpenKeyExW(HKEY_CLASSES_ROOT, szDMORootKey, 0, KEY_WRITE, &hrkey);
    if (ERROR_SUCCESS != hres)
        goto lend;

    hres = RegDeleteKeyW(hrkey, szguid);
    if (ERROR_SUCCESS != hres)
        goto lend;

    hres = RegOpenKeyExW(hrkey, szDMOCategories, 0, KEY_WRITE, &hckey);
    if (ERROR_SUCCESS != hres)
        goto lend;

    hres = RegDeleteKeyW(hckey, szguid);
    if (ERROR_SUCCESS != hres)
        goto lend;

lend:
    if (hckey)
        RegCloseKey(hckey);
    if (hrkey)
        RegCloseKey(hrkey);

    return hres;
}


/***************************************************************
 * DMOGetName (MSDMO.@)
 *
 * Get DMP Name from the registry
 */
HRESULT WINAPI DMOGetName(REFCLSID clsidDMO, WCHAR szName[])
{
    WCHAR szguid[64];
    HRESULT hres;
    HKEY hrkey = 0;
    HKEY hkey = 0;
    static const INT max_name_len = 80;
    DWORD count;

    TRACE("%s\n", debugstr_guid(clsidDMO));

    hres = RegOpenKeyExW(HKEY_CLASSES_ROOT, szDMORootKey, 
        0, KEY_READ, &hrkey);
    if (ERROR_SUCCESS != hres)
        goto lend;

    hres = RegOpenKeyExW(hrkey, GUIDToString(szguid, clsidDMO),
        0, KEY_READ, &hkey);
    if (ERROR_SUCCESS != hres)
        goto lend;

    count = max_name_len * sizeof(WCHAR);
    hres = RegQueryValueExW(hkey, NULL, NULL, NULL, 
        (LPBYTE) szName, &count); 

    TRACE(" szName=%s\n", debugstr_w(szName));
lend:
    if (hkey)
        RegCloseKey(hrkey);
    if (hkey)
        RegCloseKey(hkey);

    return hres;
}


/**************************************************************************
*   IEnumDMO_Destructor
*/
static BOOL IEnumDMO_Destructor(IEnumDMO* iface)
{
    IEnumDMOImpl *This = (IEnumDMOImpl *)iface;

    TRACE("%p\n", This);

    if (This->hkey)
        RegCloseKey(This->hkey);

    HeapFree(GetProcessHeap(), 0, This->pInTypes);
    HeapFree(GetProcessHeap(), 0, This->pOutTypes);

    return TRUE;
}


/**************************************************************************
 *  IEnumDMO_Constructor
 */
static IEnumDMO * IEnumDMO_Constructor(
    REFGUID guidCategory,
    DWORD dwFlags,
    DWORD cInTypes,
    const DMO_PARTIAL_MEDIATYPE *pInTypes,
    DWORD cOutTypes,
    const DMO_PARTIAL_MEDIATYPE *pOutTypes)
{
    UINT size;
    IEnumDMOImpl* lpedmo;
    BOOL ret = FALSE;

    lpedmo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IEnumDMOImpl));

    if (lpedmo)
    {
        lpedmo->ref = 1;
        lpedmo->lpVtbl = &edmovt;
        lpedmo->index = -1;
	lpedmo->guidCategory = guidCategory;
	lpedmo->dwFlags = dwFlags;

        if (cInTypes > 0)
        {
            size = cInTypes * sizeof(DMO_PARTIAL_MEDIATYPE);
            lpedmo->pInTypes = HeapAlloc(GetProcessHeap(), 0, size);
            if (!lpedmo->pInTypes)
                goto lerr;
            memcpy(lpedmo->pInTypes, pInTypes, size);
            lpedmo->cInTypes = cInTypes;
        }

        if (cOutTypes > 0)
        {
            size = cOutTypes * sizeof(DMO_PARTIAL_MEDIATYPE);
            lpedmo->pOutTypes = HeapAlloc(GetProcessHeap(), 0, size);
            if (!lpedmo->pOutTypes)
                goto lerr;
            memcpy(lpedmo->pOutTypes, pOutTypes, size);
            lpedmo->cOutTypes = cOutTypes;
        }

        /* If not filtering by category enum from media objects root */
        if (IsEqualGUID(guidCategory, &GUID_NULL))
        {
            if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_CLASSES_ROOT, szDMORootKey, 
                0, KEY_READ, &lpedmo->hkey))
                ret = TRUE;
        }
        else
        {
            WCHAR szguid[64];
            WCHAR szKey[MAX_PATH];

            wsprintfW(szKey, szCat3Fmt, szDMORootKey, szDMOCategories, 
                GUIDToString(szguid, guidCategory));
            if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_CLASSES_ROOT, szKey, 
                0, KEY_READ, &lpedmo->hkey))
                ret = TRUE;
        }

lerr:
        if(!ret)
        {
            IEnumDMO_Destructor((IEnumDMO*)lpedmo);
            HeapFree(GetProcessHeap(),0,lpedmo);
            lpedmo = NULL;
        }
    }

    TRACE("returning %p\n", lpedmo);

    return (IEnumDMO*)lpedmo;
}


/******************************************************************************
 * IEnumDMO_fnAddRef
 */
static ULONG WINAPI IEnumDMO_fnAddRef(IEnumDMO * iface)
{
    IEnumDMOImpl *This = (IEnumDMOImpl *)iface;
    return InterlockedIncrement(&This->ref);
}


/**************************************************************************
 *  EnumDMO_QueryInterface
 */
static HRESULT WINAPI IEnumDMO_fnQueryInterface(
    IEnumDMO* iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    IEnumDMOImpl *This = (IEnumDMOImpl *)iface;

    *ppvObj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown))
        *ppvObj = This;
    else if(IsEqualIID(riid, &IID_IEnumDMO))
        *ppvObj = This;

    if(*ppvObj)
    {
        IEnumDMO_fnAddRef(*ppvObj);
        return S_OK;
    }

    return E_NOINTERFACE;
}


/******************************************************************************
 * IEnumDMO_fnRelease
 */
static ULONG WINAPI IEnumDMO_fnRelease(IEnumDMO * iface)
{
    IEnumDMOImpl *This = (IEnumDMOImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount)
    {
        IEnumDMO_Destructor((IEnumDMO*)This);
        HeapFree(GetProcessHeap(),0,This);
    }
    return refCount;
}


/******************************************************************************
 * IEnumDMO_fnNext
 */
static HRESULT WINAPI IEnumDMO_fnNext(
    IEnumDMO * iface, 
    DWORD cItemsToFetch,
    CLSID * pCLSID,
    WCHAR ** Names,
    DWORD * pcItemsFetched)
{
    FILETIME ft;
    HKEY hkey;
    WCHAR szNextKey[MAX_PATH];
    WCHAR szGuidKey[64];
    WCHAR szKey[MAX_PATH];
    WCHAR szValue[MAX_PATH];
    DWORD len;
    UINT count = 0;
    HRESULT hres = S_OK;

    IEnumDMOImpl *This = (IEnumDMOImpl *)iface;

    TRACE("--> (%p) %d %p %p %p\n", iface, cItemsToFetch, pCLSID, Names, pcItemsFetched);

    if (!pCLSID || !Names || !pcItemsFetched)
        return E_POINTER;

    while (count < cItemsToFetch)
    {
        This->index++;

        len = MAX_PATH;
        hres = RegEnumKeyExW(This->hkey, This->index, szNextKey, &len, NULL, NULL, NULL, &ft);
        if (hres != ERROR_SUCCESS)
            break;

        TRACE("found %s\n", debugstr_w(szNextKey));

        if (!(This->dwFlags & DMO_ENUMF_INCLUDE_KEYED))
        {
            wsprintfW(szKey, szCat3Fmt, szDMORootKey, szNextKey, szDMOKeyed);
            hres = RegOpenKeyExW(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hkey);
            if (ERROR_SUCCESS == hres)
            {
                RegCloseKey(hkey);
                /* Skip Keyed entries */
                continue;
            }
        }

        wsprintfW(szKey, szCat2Fmt, szDMORootKey, szNextKey);
        hres = RegOpenKeyExW(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hkey);

        if (This->pInTypes)
        {
            UINT i, j;
            DWORD cInTypes;
            DMO_PARTIAL_MEDIATYPE* pInTypes;

            hres = read_types(hkey, szDMOInputType, &cInTypes,
                    sizeof(szValue)/sizeof(DMO_PARTIAL_MEDIATYPE),
                    (DMO_PARTIAL_MEDIATYPE*)szValue);

            if (ERROR_SUCCESS != hres)
            {
                RegCloseKey(hkey);
                continue;
            }

	    pInTypes = (DMO_PARTIAL_MEDIATYPE*) szValue;

            for (i = 0; i < This->cInTypes; i++)
            {
                for (j = 0; j < cInTypes; j++) 
                {
                    if (IsMediaTypeEqual(&pInTypes[j], &This->pInTypes[i]))
		        break;
                }

		if (j >= cInTypes)
                    break;
            }

            if (i < This->cInTypes)
            {
                RegCloseKey(hkey);
                continue;
            }
        }

        if (This->pOutTypes)
        {
            UINT i, j;
            DWORD cOutTypes;
            DMO_PARTIAL_MEDIATYPE* pOutTypes;

            hres = read_types(hkey, szDMOOutputType, &cOutTypes,
                    sizeof(szValue)/sizeof(DMO_PARTIAL_MEDIATYPE),
                    (DMO_PARTIAL_MEDIATYPE*)szValue);

	    if (ERROR_SUCCESS != hres)
            {
                RegCloseKey(hkey);
                continue;
            }

	    pOutTypes = (DMO_PARTIAL_MEDIATYPE*) szValue;

            for (i = 0; i < This->cOutTypes; i++)
            {
                for (j = 0; j < cOutTypes; j++) 
                {
                    if (IsMediaTypeEqual(&pOutTypes[j], &This->pOutTypes[i]))
		        break;
                }

		if (j >= cOutTypes)
                    break;
            }

            if (i < This->cOutTypes)
            {
                RegCloseKey(hkey);
                continue;
            }
        }

	/* Media object wasn't filtered so add it to return list */
        Names[count] = NULL;
	len = MAX_PATH * sizeof(WCHAR);
        hres = RegQueryValueExW(hkey, NULL, NULL, NULL, (LPBYTE) szValue, &len); 
        if (ERROR_SUCCESS == hres)
	{
            Names[count] = HeapAlloc(GetProcessHeap(), 0, strlenW(szValue) + 1);
	    if (Names[count])
                strcmpW(Names[count], szValue);
	}
        wsprintfW(szGuidKey,szToGuidFmt,szNextKey);
        CLSIDFromString(szGuidKey, &pCLSID[count]);

        TRACE("found match %s %s\n", debugstr_w(szValue), debugstr_w(szNextKey));
        RegCloseKey(hkey);
	count++;
    }

    *pcItemsFetched = count;
    if (*pcItemsFetched < cItemsToFetch)
        hres = S_FALSE;

    TRACE("<-- %i found\n",count);
    return hres;
}
 

/******************************************************************************
 * IEnumDMO_fnSkip
 */
static HRESULT WINAPI IEnumDMO_fnSkip(IEnumDMO * iface, DWORD cItemsToSkip)
{
    IEnumDMOImpl *This = (IEnumDMOImpl *)iface;

    This->index += cItemsToSkip;

    return S_OK;
}


/******************************************************************************
 * IEnumDMO_fnReset
 */
static HRESULT WINAPI IEnumDMO_fnReset(IEnumDMO * iface)
{
    IEnumDMOImpl *This = (IEnumDMOImpl *)iface;

    This->index = -1;

    return S_OK;
}
 

/******************************************************************************
 * IEnumDMO_fnClone
 */
static HRESULT WINAPI IEnumDMO_fnClone(IEnumDMO * iface, IEnumDMO **ppEnum)
{
    IEnumDMOImpl *This = (IEnumDMOImpl *)iface;

    FIXME("(%p)->() to (%p)->() E_NOTIMPL\n", This, ppEnum);

  return E_NOTIMPL;
}


/***************************************************************
 * DMOEnum (MSDMO.@)
 *
 * Enumerate DirectX Media Objects in the registry.
 */
HRESULT WINAPI DMOEnum(
    REFGUID guidCategory,
    DWORD dwFlags,
    DWORD cInTypes,
    const DMO_PARTIAL_MEDIATYPE *pInTypes,
    DWORD cOutTypes,
    const DMO_PARTIAL_MEDIATYPE *pOutTypes,
    IEnumDMO **ppEnum)
{
    HRESULT hres = E_FAIL;

    TRACE("guidCategory=%p dwFlags=0x%08x cInTypes=%d cOutTypes=%d\n",
        guidCategory, dwFlags, cInTypes, cOutTypes);

    *ppEnum = IEnumDMO_Constructor(guidCategory, dwFlags, cInTypes,
        pInTypes, cOutTypes, pOutTypes);
    if (*ppEnum)
        hres = S_OK;

    return hres;
}


static const IEnumDMOVtbl edmovt =
{
	IEnumDMO_fnQueryInterface,
	IEnumDMO_fnAddRef,
	IEnumDMO_fnRelease,
	IEnumDMO_fnNext,
	IEnumDMO_fnSkip,
	IEnumDMO_fnReset,
	IEnumDMO_fnClone,
};


HRESULT read_types(HKEY root, LPCWSTR key, ULONG *supplied, ULONG requested, DMO_PARTIAL_MEDIATYPE* types )
{
    HRESULT ret = S_OK;
    if (MSDMO_MAJOR_VERSION > 5)
    {
        DWORD len;
        len = requested * sizeof(DMO_PARTIAL_MEDIATYPE);
        ret = RegQueryValueExW(root, key, NULL, NULL, (LPBYTE) types, &len);
        *supplied = len / sizeof(DMO_PARTIAL_MEDIATYPE);
    }
    else
    {
        HKEY hkey;
        WCHAR szGuidKey[64];

        *supplied = 0;
        if (ERROR_SUCCESS == RegOpenKeyExW(root, key, 0, KEY_READ, &hkey))
        {
          int index = 0;
          WCHAR szNextKey[MAX_PATH];
          DWORD len;
          LONG rc = ERROR_SUCCESS;

          while (rc == ERROR_SUCCESS)
          {
            len = MAX_PATH;
            rc = RegEnumKeyExW(hkey, index, szNextKey, &len, NULL, NULL, NULL, NULL);
            if (rc == ERROR_SUCCESS)
            {
              HKEY subk;
              int sub_index = 0;
              LONG rcs = ERROR_SUCCESS;
              WCHAR szSubKey[MAX_PATH];

              RegOpenKeyExW(hkey, szNextKey, 0, KEY_READ, &subk);
              while (rcs == ERROR_SUCCESS)
              {
                len = MAX_PATH;
                rcs = RegEnumKeyExW(subk, sub_index, szSubKey, &len, NULL, NULL, NULL, NULL);
                if (rcs == ERROR_SUCCESS)
                {
                  if (*supplied >= requested)
                  {
                    /* Bailing */
                    ret = S_FALSE;
                    rc = ERROR_MORE_DATA;
                    rcs = ERROR_MORE_DATA;
                    break;
                  }

                  wsprintfW(szGuidKey,szToGuidFmt,szNextKey);
                  CLSIDFromString(szGuidKey, &types[*supplied].type);
                  wsprintfW(szGuidKey,szToGuidFmt,szSubKey);
                  CLSIDFromString(szGuidKey, &types[*supplied].subtype);
                  TRACE("Adding type %s subtype %s at index %i\n",
                    debugstr_guid(&types[*supplied].type),
                    debugstr_guid(&types[*supplied].subtype),
                    *supplied);
                  (*supplied)++;
                }
                sub_index++;
              }
              index++;
            }
          }
          RegCloseKey(hkey);
        }
    }
    return ret;
}

/***************************************************************
 * DMOGetTypes (MSDMO.@)
 */
HRESULT WINAPI DMOGetTypes(REFCLSID clsidDMO,
               unsigned long ulInputTypesRequested,
               unsigned long* pulInputTypesSupplied,
               DMO_PARTIAL_MEDIATYPE* pInputTypes,
               unsigned long ulOutputTypesRequested,
               unsigned long* pulOutputTypesSupplied,
               DMO_PARTIAL_MEDIATYPE* pOutputTypes)
{
  HKEY root,hkey;
  HRESULT ret = S_OK;
  WCHAR szguid[64];

  TRACE ("(%s,%u,%p,%p,%u,%p,%p),stub!\n", debugstr_guid(clsidDMO),
        ulInputTypesRequested, pulInputTypesSupplied, pInputTypes,
        ulOutputTypesRequested, pulOutputTypesSupplied, pOutputTypes);

  if (ERROR_SUCCESS != RegOpenKeyExW(HKEY_CLASSES_ROOT, szDMORootKey, 0,
                                     KEY_READ, &root))
    return E_FAIL;

  if (ERROR_SUCCESS != RegOpenKeyExW(root,GUIDToString(szguid,clsidDMO) , 0,
                                     KEY_READ, &hkey))
  {
    RegCloseKey(root);
    return E_FAIL;
  }

  if (ulInputTypesRequested > 0)
  {
    ret = read_types(hkey, szDMOInputType, (ULONG*)pulInputTypesSupplied, ulInputTypesRequested, pInputTypes );
  }
  else
    *pulInputTypesSupplied = 0;

  if (ulOutputTypesRequested > 0)
  {
    HRESULT ret2;
    ret2 = read_types(hkey, szDMOOutputType, (ULONG*)pulOutputTypesSupplied, ulOutputTypesRequested, pOutputTypes );

    if (ret == S_OK)
        ret = ret2;
  }
  else
    *pulOutputTypesSupplied = 0;

  return ret;
}
