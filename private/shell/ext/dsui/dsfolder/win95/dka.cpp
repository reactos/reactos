//
// These APIs were moved from shell32 to stocklib.
//

#include "pch.h"
#include "dlshell.h"
#pragma hdrstop


#define CCH_KEYMAX      64          // DOC: max size of a reg key (under shellex)

//===========================================================================
// DKA stuff (moved from filemenu.c)
//===========================================================================

typedef struct _DKAITEM {       // dkai
    TCHAR    _szKey[CCH_KEYMAX];
} DKAITEM, *PDKAITEM;
typedef const DKAITEM * PCDKAITEM;

typedef struct _DKA {           // dka
    HDSA    _hdsa;
    HKEY    _hkey;
} DKA, *PDKA;



//
//  This function creates a dynamic registration key array from the
// specified location of the registration base.
//
// Arguments:
//  hkey      -- Identifies a currently open key (which can be HKEY_CLASSES_ROOT).
//  pszSubKey -- Points to a null-terminated string specifying the name of the
//               subkey from which we enumerate the list of subkeys.
//  fDefault  -- If true, it will only load the keys that are enumerated in
//               pszSubKey's value
//
// Returns:
//   The return value is non-zero handle to the created dynamic key array
//  if the function is successful. Otherwise, NULL.
//
// History:
//  05-06-93 SatoNa     Created
//
// Notes:
//  The dynamic key array should be destroyed by calling DKA_Destroy function.
//
HDKA DKA_Create(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszFirst, LPCTSTR pszDefOrder, BOOL fDefault)
{
    ASSERT(IS_VALID_HANDLE(hkey, KEY));
    ASSERT(NULL == pszSubKey || IS_VALID_STRING_PTR(pszSubKey, -1));
    ASSERT(NULL == pszFirst || IS_VALID_STRING_PTR(pszFirst, -1));
    ASSERT(NULL == pszDefOrder || IS_VALID_STRING_PTR(pszDefOrder, -1));
    
    PDKA pdka = (PDKA)LocalAlloc(LPTR, SIZEOF(DKA));
    DKAITEM dkai;

    if (pdka)
    {
        pdka->_hdsa = DSA_Create(SIZEOF(DKAITEM), 4);
        if (pdka->_hdsa)
        {
            if (ERROR_SUCCESS != RegOpenKeyEx(hkey, pszSubKey, 0L, KEY_READ, &pdka->_hkey))
            {
                DSA_Destroy(pdka->_hdsa);
                pdka->_hdsa = NULL;
            }
        }
        
        // Check if the creation succceeded
        if (pdka->_hdsa)
        {
            // Yes, add keys
            TCHAR szValue[MAX_PATH*2+CCH_KEYMAX];
            LONG cchValue=ARRAYSIZE(szValue)-CCH_KEYMAX;
            LONG cbValue;
            LPTSTR lpszValue = szValue;
            TCHAR szKey[CCH_KEYMAX];
            int i;
            LPTSTR psz;
            HKEY hkeyCmd;

            *szValue = TEXT('\0');

            // if there's something we need to add first, do it now.
            if (pszFirst) {
                lstrcpy(szValue, pszFirst);
                lstrcat(szValue, TEXT(" "));
                i = lstrlen(szValue);
                cchValue -= i;
                lpszValue += i;
            }

            // First, add the subkeys from the value of the specified key
            // This should never fail, since we just opened this key

            cbValue = cchValue * SIZEOF(TCHAR);
            RegQueryValue(pdka->_hkey, NULL, lpszValue, &cbValue);
            if (!*szValue && pszDefOrder)
            {
                // If there is no value, default to open for 3.1 compatibility
                lstrcpy(szValue, pszDefOrder);
            }

            psz = szValue;
            do
            {
                // skip the space or comma characters
                while(*psz==TEXT(' ') || *psz==TEXT(','))
                    psz++;          // NLS Notes: OK to ++

                if (*psz)
                {
                    // Search for the space or comma character
                    LPTSTR pszNext = psz + StrCSpn(psz, TEXT(" ,"));
                    if (*pszNext) {
                        *pszNext++=TEXT('\0');    // NLS Notes: OK to ++
                    }

                    // Verify that the key exists before adding it to the list
                    if (RegOpenKeyEx(pdka->_hkey, psz, 0L, KEY_READ, &hkeyCmd) == ERROR_SUCCESS)
                    {
                        lstrcpy(dkai._szKey, psz);
                        DSA_AppendItem(pdka->_hdsa, &dkai);
                        RegCloseKey(hkeyCmd);
                    }

                    psz = pszNext;
                }
            } while (psz && *psz);


            if (!fDefault) {
                // Then, append the rest if they are not in the list yet.
                for (i=0;
                     RegEnumKey(pdka->_hkey, i, szKey, ARRAYSIZE(szKey))==ERROR_SUCCESS;
                     i++)
                {
                    int idsa;
                    //
                    // Check if the key is already in the list.
                    //
                    for (idsa = 0; idsa < DSA_GetItemCount(pdka->_hdsa); idsa++)
                    {
                        PDKAITEM pdkai = (PDKAITEM)DSA_GetItemPtr(pdka->_hdsa, idsa);
                        if (lstrcmpi(szKey, pdkai->_szKey)==0)
                            break;
                    }

                    if (idsa == DSA_GetItemCount(pdka->_hdsa))
                    {
                        //
                        // No, append it.
                        //
                        lstrcpy(dkai._szKey, szKey);
                        DSA_AppendItem(pdka->_hdsa, &dkai);
                    }
                }
            }
        }
        else
        {
            // No, free the memory and return NULL.
            LocalFree((HLOCAL)pdka);
            pdka=NULL;
        }
    }
    return (HDKA)pdka;
}


#ifdef DECLARE_ONCE

int DKA_GetItemCount(HDKA pdka)
{
    return DSA_GetItemCount(pdka->_hdsa);
}


//
//  This function destroys the dynamic key array.
// Arguments:
//  hdka     -- Specifies the dynamic key array
//
// History:
//  05-06-93 SatoNa     Created
//
void DKA_Destroy(HDKA pdka)
{
    if (pdka)
    {
        RegCloseKey(pdka->_hkey);
        DSA_Destroy(pdka->_hdsa);
        LocalFree((HLOCAL)pdka);
    }
}

#endif // DECLARE_ONCE


LPCTSTR DKA_GetKey(HDKA pdka, int iItem)
{
    PDKAITEM pdkai = (PDKAITEM)DSA_GetItemPtr(pdka->_hdsa, iItem);
    return pdkai->_szKey;
}


//
//  This function returns the value of specified sub-key.
//
// Arguments:
//  hdka     -- Specifies the dynamic key array
//  iItem    -- Specifies the index to the sub-key
//  pszValue -- Points to a buffefr that contains the text string when
//              the function returns.
//  pcb      -- Points to a variable specifying the sixze, in bytes, of the buffer
//              pointer by the pszValue parameter. When the function returns,
//              this variable contains the size of the string copied to pszVlaue,
//              including the null-terminating character.
//
// History:
//  05-06-93 SatoNa     Created
//
LONG DKA_QueryValue(HDKA pdka, int iItem, LPTSTR pszValue, LONG * pcb)
{
    PCDKAITEM pdkai = (PCDKAITEM) DSA_GetItemPtr(pdka->_hdsa, iItem);
    if (pdkai) {
        return RegQueryValue(pdka->_hkey, pdkai->_szKey, pszValue, pcb);
    }
    return ERROR_INVALID_PARAMETER;
}


/*----------------------------------------------------------
Purpose: Return a value from under the given sub-key.

Returns: win32 error
*/
DWORD
DKA_QueryOtherValue(
    IN  HDKA    pdka,
    IN  int     iItem,
    IN  LPCTSTR pszName,
    IN  LPTSTR  pszValue,
    OUT LONG *  pcb)
{
    DWORD dwRet = ERROR_INVALID_PARAMETER;
    PCDKAITEM pdkai = (PCDKAITEM) DSA_GetItemPtr(pdka->_hdsa, iItem);

    if (pdkai)
    {
        HKEY hkey;

        dwRet = RegOpenKeyEx(pdka->_hkey, pdkai->_szKey, 0, KEY_QUERY_VALUE,
                             &hkey);
        if (NO_ERROR == dwRet)
        {
            dwRet = RegQueryValueEx(hkey, pszName, 0, NULL, (LPBYTE)pszValue, (DWORD *) pcb);
            RegCloseKey(hkey);
        }
    }

    return dwRet;
}


//===========================================================================
// DCA stuff - Dynamic CLSID array
// 
//  This is a dynamic array of CLSIDs that you can obtain from 
//  a registry key or add individually.  Use DCA_CreateInstance
//  to actually CoCreateInstance the element.
//
//===========================================================================


#ifdef DECLARE_ONCE

HDCA DCA_Create()
{
    HDSA hdsa = DSA_Create(SIZEOF(CLSID), 4);
    return (HDCA)hdsa;
}

void DCA_Destroy(HDCA hdca)
{
    DSA_Destroy((HDSA)hdca);
}

int  DCA_GetItemCount(HDCA hdca)
{
    ASSERT(hdca);
    
    return DSA_GetItemCount((HDSA)hdca);
}

const CLSID * DCA_GetItem(HDCA hdca, int i)
{
    ASSERT(hdca);
    
    return (const CLSID *)DSA_GetItemPtr((HDSA)hdca, i);
}


BOOL DCA_AddItem(HDCA hdca, REFCLSID rclsid)
{
    ASSERT(hdca);
    
    int ccls = DCA_GetItemCount(hdca);
    int icls;
    for (icls = 0; icls < ccls; icls++)
    {
        if (IsEqualGUID(rclsid, *DCA_GetItem(hdca,icls))) 
            return FALSE;
    }

{
    TCHAR szGUID[GUIDSTR_MAX];
    GetStringFromGUID(rclsid, szGUID, ARRAYSIZE(szGUID));
    Trace(TEXT("GUID added was %s"), szGUID);
}    

    DSA_AppendItem((HDSA)hdca, (LPVOID) &rclsid);
    return TRUE;
}


HRESULT DCA_CreateInstance(HDCA hdca, int iItem, REFIID riid, LPVOID FAR* ppv)
{
    const CLSID * pclsid = DCA_GetItem(hdca, iItem);
    if (pclsid) {
        return SHCoCreateInstance(NULL, pclsid, NULL, riid, ppv);
    }
    return E_INVALIDARG;
}

#endif // DECLARE_ONCE


void DCA_AddItemsFromKey(HDCA hdca, HKEY hkey, LPCTSTR pszSubKey)
{
    HDKA hdka = DKA_Create(hkey, pszSubKey, NULL, NULL, FALSE);
    if (hdka)
    {
        int ikey;
        int ckey = DKA_GetItemCount(hdka);
        for (ikey = 0; ikey < ckey; ikey++)
        {
            HRESULT hres;
            CLSID clsid;

            //
            // First, check if the key itself is a CLSID
            //          
            Trace(TEXT("Trying the key name %s"), DKA_GetKey(hdka, ikey));
            hres = GetGUIDFromString(DKA_GetKey(hdka, ikey), &clsid) ? NOERROR : CO_E_CLASSSTRING;
            if (FAILED(hres))
            {
                //
                // If not, try its value
                //
                TCHAR szCLSID[MAX_PATH];
                LONG cb = SIZEOF(szCLSID);
                if (DKA_QueryValue(hdka, ikey, szCLSID, &cb)==ERROR_SUCCESS)
                {
                    Trace(TEXT("Trying the value %s"), szCLSID);
                    hres = GetGUIDFromString(szCLSID, &clsid) ? NOERROR : CO_E_CLASSSTRING;
                }
            }

            //
            // Add the CLSID if we successfully got the CLSID.
            //
            if (SUCCEEDED(hres))
            {
                DCA_AddItem(hdca, clsid);
            }
        }
        DKA_Destroy(hdka);
    }
}


