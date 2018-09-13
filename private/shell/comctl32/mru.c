#include "ctlspriv.h"
#include <memory.h>

//// BUGBUG:  cpls's main is the only 16 bit guy to use this.  punt him

#define MRU_ORDERDIRTY 0x1000

#define DM_MRULAZY  DM_TRACE

#define MAX_CHAR    126
#define BASE_CHAR   TEXT('a')

typedef struct tagMRUDATA
{
    UINT fFlags;
    UINT uMax;
    LPVOID lpfnCompare;
    HKEY hKey;
#ifdef DEBUG
    TCHAR szSubKey[32];
#endif
    LPTSTR cOrder;
} MRUDATA, *PMRUDATA;

#define c_szMRU     TEXT("MRUList")

#define NTHSTRING(p, n) (*((LPTSTR FAR *)((LPBYTE)p+sizeof(MRUDATA))+n))
#define NTHDATA(p, n) (*((LPBYTE FAR *)((LPBYTE)p+sizeof(MRUDATA))+n))
#define NUM_OVERHEAD 3


#ifdef VSTF

/*----------------------------------------------------------
Purpose: Validate the MRU structure
*/
BOOL IsValidPMRUDATA(PMRUDATA pmru)
{
    return (IS_VALID_WRITE_PTR(pmru, MRUDATA) &&
            (NULL == pmru->lpfnCompare || IS_VALID_CODE_PTR(pmru->lpfnCompare, void)));
}

#endif // VSTF

//----------------------------------------------------------------------------
// Internal memcmp - saves loading crt's, cdecl so we can use
// as MRUCMPDATAPROC

int CDECL _mymemcmp(const void *pBuf1, const void *pBuf2, size_t cb)
{
    // Take advantage of the intrinsic version from crtfree.h
    return memcmp(pBuf1, pBuf2, cb);
}


// Use this macro to get the original size of the data.
#define DATASIZE(p)     (*((LPDWORD)p))
// And this to get a pointer to the original data.
#define DATAPDATA(p)    (p+sizeof(DWORD))

//----------------------------------------------------------------------------
//  For binary data we stick the size of the data at the begining and store the
//  whole thing in one go.
BOOL MRUIsSameData(PMRUDATA pMRU, BYTE FAR* pVal, const void FAR *lpData, UINT cbData)
{
    int cbUseSize;
    MRUCMPDATAPROC lpfnCompare;

    ASSERT(IS_VALID_STRUCT_PTR(pMRU, MRUDATA));

    lpfnCompare = pMRU->lpfnCompare;

    ASSERT(IS_VALID_CODE_PTR(lpfnCompare, MRUCMPDATAPROC));

    // if there's something other than a mem compare,
    // don't require the sizes to be equal in order for the
    // data to be equivalent.

    if (pMRU->lpfnCompare == _mymemcmp)
    {
        if (DATASIZE(pVal) != cbData)
            return FALSE;

        cbUseSize = cbData;
    }
    else
        cbUseSize = min(DATASIZE(pVal), cbData);

    return ((*lpfnCompare)(lpData, DATAPDATA(pVal), cbUseSize) == 0);
}


//----------------------------------------------------------------------------
HANDLE WINAPI CreateMRUListLazy(LPMRUINFO lpmi, const void FAR *lpData, UINT cbData, LPINT lpiSlot)
{
    HANDLE hMRU = NULL;
    PTSTR pOrder, pNewOrder, pTemp;
    LPBYTE pVal;
    LONG cbVal;
    DWORD dwDisposition;
    DWORD dwType;
    PMRUDATA pMRU = NULL;
    HKEY hkeySubKey = NULL;
    TCHAR szTemp[2];
    UINT uMax = lpmi->uMax;
    HKEY hKey = lpmi->hKey;
    LPCTSTR lpszSubKey = lpmi->lpszSubKey;
    MRUCMPPROC lpfnCompare = lpmi->lpfnCompare;
    int cb;

#ifdef DEBUG
    DWORD dwStart = GetTickCount();
#endif
    if (!lpfnCompare) {
        lpfnCompare = (lpmi->fFlags & MRU_BINARY) ? (MRUCMPPROC)_mymemcmp :
                      (
#ifdef UNICODE
                       (lpmi->fFlags & MRU_ANSI) ? (MRUCMPPROC)lstrcmpiA :
#endif
                       (MRUCMPPROC)lstrcmpi
                      );
    }

    //  limit to 126 so that we don't use extended chars
    if (uMax > MAX_CHAR-BASE_CHAR)
        uMax = MAX_CHAR-BASE_CHAR;

    if (RegCreateKeyEx(hKey, lpszSubKey, 0L, (LPTSTR)c_szShell, REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkeySubKey, &dwDisposition) != ERROR_SUCCESS)
        goto Error1;

    pOrder = (PTSTR)Alloc((uMax + 1) * sizeof(TCHAR));
    if (!pOrder)
        goto Error1;

    cbVal = ((LONG)uMax + 1) * sizeof(TCHAR);

    if (RegQueryValueEx(hkeySubKey, (LPTSTR)c_szMRU, NULL, &dwType, (LPBYTE)pOrder, &cbVal) != ERROR_SUCCESS)
    {
        // if not already in the registry, then start fresh
        *pOrder = 0;
    }

    // Uppercase is not allowed
#ifdef UNICODE
    CharLower(pOrder);
#else
    AnsiLower(pOrder);
#endif

    // We allocate room for the MRUDATA structure, plus the order list,
    // and the list of strings.
    cb = (lpmi->fFlags & MRU_BINARY) ? sizeof(LPBYTE) : sizeof(LPTSTR);
    pMRU = (PMRUDATA)Alloc(sizeof(MRUDATA)+(uMax*cb));
    if (!pMRU) {
        goto Error2;
    }

    // Allocate space for the order list
    pMRU->cOrder = (LPTSTR)Alloc((uMax+1)*sizeof(TCHAR));
    if (!pMRU->cOrder) {
        Free(pMRU);
        goto Error2;
    }

    pMRU->fFlags = lpmi->fFlags;
    pMRU->uMax = uMax;
    pMRU->lpfnCompare = lpfnCompare;
    pMRU->hKey = hkeySubKey;
#ifdef DEBUG
    lstrcpyn(pMRU->szSubKey, lpszSubKey, ARRAYSIZE(pMRU->szSubKey));
#endif

    // Traverse through the MRU list, adding strings to the end of the
    // list.
    szTemp[1] = TEXT('\0');
    for (pTemp = pOrder, pNewOrder = pMRU->cOrder; ; ++pTemp)
    {
        // Stop when we get to the end of the list.
        szTemp[0] = *pTemp;
        if (!szTemp[0]) {
            break;
        }

        if (lpmi->fFlags & MRU_BINARY) {
            // Check if in range and if we have already used this letter.
            if ((UINT)(szTemp[0]-BASE_CHAR)>=uMax || NTHDATA(pMRU, szTemp[0]-BASE_CHAR)) {
                continue;
            }
            // Get the value from the registry
            cbVal = 0;
            // first find the size
            if ((RegQueryValueEx(hkeySubKey, szTemp, NULL, &dwType, NULL, &cbVal)
                 != ERROR_SUCCESS) || (dwType != REG_BINARY))
                continue;

            // Binary data has the size at the begining so we'll need a little extra room.
            pVal = (LPBYTE)Alloc(cbVal + sizeof(DWORD));

            if (!pVal) {
                // BUGBUG perhaps sort of error is in order.
                continue;
            }

            // now really get it
            DATASIZE(pVal) = cbVal;
            if (RegQueryValueEx(hkeySubKey, szTemp, NULL, &dwType, pVal+sizeof(DWORD),
                                (LPDWORD)pVal) != ERROR_SUCCESS)
                continue;

            // Note that blank elements ARE allowed in the list.
            NTHDATA(pMRU, szTemp[0]-BASE_CHAR) = pVal;
            *pNewOrder++ = szTemp[0];

            //
            // OPTIMIZATION
            //   If lpData and lpiSlot are specified, we stop the enumeratation
            //  when we find the item.
            //
            if (lpData && lpiSlot) {
                // Check if we have the specified one or not.
                if (MRUIsSameData(pMRU, pVal, lpData, cbData)) {
                    // Found it.
                    *lpiSlot = (INT) (pNewOrder - pMRU->cOrder);

                    TraceMsg(DM_MRULAZY, "CreateMRUListLazy found it. Copying %s", pTemp);

                    pMRU->fFlags |= MRU_LAZY;
                    //
                    // Copy the rest of slot. Notice that we don't load
                    // data for those slot.
                    //
                    for (pTemp++; *pTemp; pTemp++) {
                        *pNewOrder++ = *pTemp;
                    }
                    break;
                }
            }
        } else {

            // Check if in range and if we have already used this letter.
            if ((UINT)(szTemp[0]-BASE_CHAR)>=uMax || NTHSTRING(pMRU, szTemp[0]-BASE_CHAR)) {
                continue;
            }
            // Get the value from the registry
            cbVal = 0;
            // first find the size
            if ((RegQueryValueEx(hkeySubKey, szTemp, NULL, &dwType, NULL, &cbVal)
                 != ERROR_SUCCESS) || (dwType != REG_SZ))
                continue;

            cbVal *= sizeof(TCHAR);
            pVal = (LPBYTE)Alloc(cbVal);

            if (!pVal) {
                // BUGBUG perhaps sort of error is in order.
                continue;
            }
            // now really get it
            if (RegQueryValueEx(hkeySubKey, szTemp, NULL, &dwType, (LPBYTE)pVal, &cbVal) != ERROR_SUCCESS)
                continue;

            // Note that blank elements are not allowed in the list.
            if (*((LPTSTR)pVal)) {
                NTHSTRING(pMRU, szTemp[0]-BASE_CHAR) = (LPTSTR)pVal;
                *pNewOrder++ = szTemp[0];
            } else {
                Free(pVal);
            }
        }
    }
    /* NULL terminate the order list so we can tell how many strings there
     * are.
     */
    *pNewOrder = TEXT('\0');

    if (lpData && lpiSlot) {
        TraceMsg(DM_MRULAZY, "CreateMRUListLazy. End of loop. %s", pMRU->cOrder);
        // If we failed to find, put -1 in it.
        if (!(pMRU->fFlags & MRU_LAZY)) {
            *lpiSlot = -1;
        }
    }

    /* Actually, this is success rather than an error.
     */
    goto Error2;

Error2:
    if (pOrder)
        Free((HLOCAL)pOrder);

Error1:
    if (!pMRU && hkeySubKey)
        RegCloseKey(hkeySubKey);

#ifdef DEBUG
    //DebugMsg(DM_TRACE, TEXT("CreateMRU: %d msec"), LOWORD(GetTickCount()-dwStart));
#endif
    return((HANDLE)pMRU);
}

HANDLE WINAPI CreateMRUList(LPMRUINFO lpmi)
{
    return CreateMRUListLazy(lpmi, NULL, 0, NULL);
}

#ifdef UNICODE

//
// ANSI thunk
//

HANDLE WINAPI CreateMRUListLazyA(LPMRUINFOA lpmi, const void FAR *lpData, UINT cbData, LPINT lpiSlot)
{
    MRUINFOW MRUInfoW;
    HANDLE hMRU;

    MRUInfoW.cbSize       = sizeof (MRUINFOW);
    MRUInfoW.uMax         = lpmi->uMax;
    MRUInfoW.fFlags       = lpmi->fFlags;
    MRUInfoW.hKey         = lpmi->hKey;
    MRUInfoW.lpszSubKey   = ProduceWFromA(CP_ACP, lpmi->lpszSubKey);
    MRUInfoW.lpfnCompare  = (MRUCMPPROCW)lpmi->lpfnCompare;

    MRUInfoW.fFlags |= MRU_ANSI;

    hMRU = CreateMRUListLazy(&MRUInfoW, lpData, cbData, lpiSlot);

    FreeProducedString((LPWSTR)MRUInfoW.lpszSubKey);

    return hMRU;
}

HANDLE WINAPI CreateMRUListA(LPMRUINFOA lpmi)
{
    return CreateMRUListLazyA(lpmi, NULL, 0, NULL);
}
#else

//
// Unicode stub when this code is built ANSI
//

HANDLE WINAPI CreateMRUListW(LPMRUINFOW lpmi)
{
    SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
    return NULL;
}

HANDLE WINAPI CreateMRUListLazyW(LPMRUINFOW lpmi, const void FAR *lpData, UINT cbData, LPINT lpiSlot)
{
    SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
    return NULL;
}
#endif


//----------------------------------------------------------------------------
STDAPI_(void) FreeMRUList(HANDLE hMRU)
{
    int i;
    LPVOID FAR *pTemp;
    PMRUDATA pMRU = (PMRUDATA)hMRU;

    ASSERT(IS_VALID_STRUCT_PTR(pMRU, MRUDATA));

    if (pMRU)
    {
        pTemp = (pMRU->fFlags & MRU_BINARY) ?
            &NTHDATA(pMRU, 0) : (LPBYTE FAR *)&NTHSTRING(pMRU, 0);

        if (pMRU->fFlags & MRU_ORDERDIRTY)
        {
            RegSetValueEx(pMRU->hKey, c_szMRU, 0L, REG_SZ, (CONST BYTE *)pMRU->cOrder,
                          sizeof(TCHAR) * (lstrlen(pMRU->cOrder) + 1));
        }

        for (i=pMRU->uMax-1; i>=0; --i, ++pTemp)
        {
            if (*pTemp)
            {
                if (pMRU->fFlags & MRU_BINARY)
                {
                    Free((LPBYTE)*pTemp);
                    *pTemp = NULL;
                }
                else
                    Str_SetPtr((LPTSTR FAR *)pTemp, NULL);
            }
        }
        RegCloseKey(pMRU->hKey);
        Free(pMRU->cOrder);
        Free((HLOCAL)pMRU);
    }
}


/* Add a string to an MRU list.
 */
STDAPI_(int) AddMRUString(HANDLE hMRU, LPCTSTR szString)
{
    /* The extra +1 is so that the list is NULL terminated.
    */
    TCHAR cFirst;
    int iSlot = -1;
    LPTSTR lpTemp;
    LPTSTR FAR * pTemp;
    int i;
    UINT uMax;
    MRUCMPPROC lpfnCompare;
    BOOL fShouldWrite;
    PMRUDATA pMRU = (PMRUDATA)hMRU;

#ifdef DEBUG
    DWORD dwStart = GetTickCount();
#endif

    ASSERT(IS_VALID_STRUCT_PTR(pMRU, MRUDATA));

    if (hMRU == NULL)
        return(-1);     // Error

    fShouldWrite = !(pMRU->fFlags & MRU_CACHEWRITE);
    uMax = pMRU->uMax;
    lpfnCompare = (MRUCMPPROC)pMRU->lpfnCompare;

    /* Check if the string already exists in the list.
    */
    for (i=0, pTemp=&NTHSTRING(pMRU, 0); (UINT)i<uMax; ++i, ++pTemp)
    {
        if (*pTemp)
        {
            int iResult;

#ifdef UNICODE
            if (pMRU->fFlags & MRU_ANSI)
            {
                LPSTR lpStringA, lpTempA;

                lpStringA = ProduceAFromW (CP_ACP, szString);
                lpTempA = ProduceAFromW (CP_ACP, (LPWSTR)*pTemp);

                iResult = (*lpfnCompare)((const void FAR *)lpStringA, (const void FAR *)lpTempA);

                FreeProducedString (lpStringA);
                FreeProducedString (lpTempA);
            }
            else
#endif
            {
                iResult = (*lpfnCompare)((const void FAR *)szString, (const void FAR *)*pTemp);
            }

            if (!iResult)
            {
                // found it, so don't do the write out
                cFirst = i + BASE_CHAR;
                iSlot = i;
                goto FoundEntry;
            }
        }
    }

    /* Attempt to find an unused entry.  Count up the used entries at the
    * same time.
    */
    for (i=0, pTemp=&NTHSTRING(pMRU, 0); ; ++i, ++pTemp)
    {
        if ((UINT)i >= uMax)    // If we got to the end of the list.
        {
            // use the entry at the end of the cOrder list
            cFirst = pMRU->cOrder[uMax-1];
            pTemp = &NTHSTRING(pMRU, cFirst-BASE_CHAR);
            break;
        }

        // Is the entry not used?
        if (!*pTemp)
        {
            // yes
            cFirst = i+BASE_CHAR;
            break;
        }
    }

    if (Str_SetPtr(pTemp, szString))
    {
        TCHAR szTemp[2];

        iSlot = (int)(cFirst-BASE_CHAR);

        szTemp[0] = cFirst;
        szTemp[1] = TEXT('\0');

        RegSetValueEx(pMRU->hKey, szTemp, 0L, REG_SZ, (CONST BYTE *)szString,
            sizeof(TCHAR) * (lstrlen(szString) + 1));

        fShouldWrite = TRUE;
    }
    else
    {
        /* Since iSlot == -1, we will remove the reference to cFirst
        * below.
        */
    }

FoundEntry:
    /* Remove any previous reference to cFirst.
    */
    lpTemp = StrChr(pMRU->cOrder, cFirst);
    if (lpTemp)
    {
        lstrcpy(lpTemp, lpTemp+1);
    }

    if (iSlot != -1)
    {
        // shift everything over and put cFirst at the front
        hmemcpy(pMRU->cOrder+1, pMRU->cOrder, pMRU->uMax*sizeof(TCHAR));
        pMRU->cOrder[0] = cFirst;
    }

    if (fShouldWrite)
    {
        RegSetValueEx(pMRU->hKey, c_szMRU, 0L, REG_SZ, (CONST BYTE *)pMRU->cOrder,
            sizeof(TCHAR) * (lstrlen(pMRU->cOrder) + 1));
        pMRU->fFlags &= ~MRU_ORDERDIRTY;
    } else
        pMRU->fFlags |= MRU_ORDERDIRTY;

#ifdef DEBUG
    // DebugMsg(DM_TRACE, TEXT("AddMRU: %d msec"), LOWORD(GetTickCount()-dwStart));
#endif
    return(iSlot);
}


#ifdef UNICODE
//
// ANSI thunk
//

STDAPI_(int) AddMRUStringA(HANDLE hMRU, LPCSTR szString)
{
    LPWSTR lpStringW;
    INT    iResult;

    lpStringW = ProduceWFromA(CP_ACP, szString);

    iResult = AddMRUString(hMRU, lpStringW);

    FreeProducedString (lpStringW);

    return iResult;
}

#else

//
// Unicode stub when this code is build ANSI
//

STDAPI_(int) AddMRUStringW(HANDLE hMRU, LPCWSTR szString)
{
    SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
    return -1;
}

#endif


/* Remove a string from an MRU list.
 */
STDAPI_(int) DelMRUString(HANDLE hMRU, int nItem)
{
    BOOL bRet = FALSE;
    LPTSTR lpTemp;
    PMRUDATA pMRU = (PMRUDATA)hMRU;

    ASSERT(IS_VALID_STRUCT_PTR(pMRU, MRUDATA));

    if (pMRU)
    {
        //
        // Make sure the index value is within the length of
        // the string so we don't pick up some random value.
        //
        if (!InRange(nItem, 0, pMRU->uMax) || nItem >= lstrlen(pMRU->cOrder))
            return FALSE;

        // Be easy -- just remove the entry from the cOrder list
        lpTemp = &pMRU->cOrder[nItem];
        if (lpTemp)
        {
            int iSlot = *lpTemp - BASE_CHAR;
            if (iSlot >= 0 && iSlot < MAX_CHAR - BASE_CHAR)
                Str_SetPtr(&NTHSTRING(pMRU, iSlot), NULL);
            lstrcpy(lpTemp, lpTemp+1);

            if (!(pMRU->fFlags & MRU_CACHEWRITE))
            {
                RegSetValueEx(pMRU->hKey, c_szMRU, 0L, REG_SZ, (CONST BYTE *)pMRU->cOrder,
                              sizeof(TCHAR) * (lstrlen(pMRU->cOrder) + 1));
                pMRU->fFlags &= ~MRU_ORDERDIRTY;
            }
            else
            {
                pMRU->fFlags |= MRU_ORDERDIRTY;
            }

            bRet = TRUE;
        }
    }

    return bRet;
}


//----------------------------------------------------------------------------
// Add data to an MRU list.
STDAPI_(int) AddMRUData(HANDLE hMRU, const void FAR *lpData, UINT cbData)
{
    TCHAR cFirst;
    int iSlot = -1;
    LPTSTR lpTemp;
    LPBYTE FAR *ppData;
    int i;
    UINT uMax;
    MRUCMPDATAPROC lpfnCompare;
    PMRUDATA pMRU = (PMRUDATA)hMRU;
    BOOL fShouldWrite;

#ifdef DEBUG
    DWORD dwStart = GetTickCount();
#endif

    ASSERT(IS_VALID_STRUCT_PTR(pMRU, MRUDATA));

    if (hMRU == NULL)
        return(-1);     // Error

    fShouldWrite = !(pMRU->fFlags & MRU_CACHEWRITE);

    uMax = pMRU->uMax;
    lpfnCompare = (MRUCMPDATAPROC)pMRU->lpfnCompare;

    // Check if the data already exists in the list.
    for (i=0, ppData=&NTHDATA(pMRU, 0); (UINT)i<uMax; ++i, ++ppData)
    {
        if (*ppData && MRUIsSameData(pMRU, *ppData, lpData, cbData))
        {
            // found it, so don't do the write out
            cFirst = i + BASE_CHAR;
            iSlot = i;
            goto FoundEntry;
        }
    }

    //
    // When created "lazy", we are not supposed to add a new item.
    //
    if (pMRU->fFlags & MRU_LAZY)
    {
        ASSERT(0);
        return -1;
    }

    // Attempt to find an unused entry.  Count up the used entries at the
    // same time.
    for (i=0, ppData=&NTHDATA(pMRU, 0); ; ++i, ++ppData)
    {
        if ((UINT)i >= uMax)
            // If we got to the end of the list.
        {
            // use the entry at the end of the cOrder list
            cFirst = pMRU->cOrder[uMax-1];
            ppData = &NTHDATA(pMRU, cFirst-BASE_CHAR);
            break;
        }

        if (!*ppData)
            // If the entry is not used.
        {
            cFirst = i+BASE_CHAR;
            break;
        }
    }

    *ppData = ReAlloc(*ppData, cbData+sizeof(DWORD));
    if (*ppData)
    {
        TCHAR szTemp[2];

        *((LPDWORD)(*ppData)) = cbData;
        hmemcpy(DATAPDATA(*ppData), lpData, cbData);

        iSlot = (int)(cFirst-BASE_CHAR);

        szTemp[0] = cFirst;
        szTemp[1] = TEXT('\0');

        RegSetValueEx(pMRU->hKey, szTemp, 0L, REG_BINARY, (LPVOID)lpData, cbData);
        fShouldWrite = TRUE;
    }
    else
    {
        // Since iSlot == -1, we will remove the reference to cFirst
        // below.
    }

FoundEntry:
    // Remove any previous reference to cFirst.
    lpTemp = StrChr(pMRU->cOrder, cFirst);
    if (lpTemp)
    {
        lstrcpy(lpTemp, lpTemp+1);
    }

    if (iSlot != -1)
    {
        // shift everything over and put cFirst at the front
        hmemcpy(pMRU->cOrder+1, pMRU->cOrder, pMRU->uMax*sizeof(TCHAR));
        pMRU->cOrder[0] = cFirst;
    }

    if (fShouldWrite)
    {
        RegSetValueEx(pMRU->hKey, c_szMRU, 0L, REG_SZ, (CONST BYTE *)pMRU->cOrder,
            sizeof(TCHAR) * (lstrlen(pMRU->cOrder) + 1));
        pMRU->fFlags &= ~MRU_ORDERDIRTY;
    } else
        pMRU->fFlags |= MRU_ORDERDIRTY;

#ifdef DEBUG
    // DebugMsg(DM_TRACE, TEXT("AddMRU: %d msec"), LOWORD(GetTickCount()-dwStart));
#endif
    return(iSlot);
}


//----------------------------------------------------------------------------
// Find data in an MRU list.
// Returns the slot number.
STDAPI_(int) FindMRUData(HANDLE hMRU, const void FAR *lpData, UINT cbData, LPINT lpiSlot)
{
    TCHAR cFirst;
    int iSlot = -1;
    LPTSTR lpTemp;
    LPBYTE FAR *ppData;
    int i;
    UINT uMax;
    PMRUDATA pMRU = (PMRUDATA)hMRU;

#ifdef DEBUG
    DWORD dwStart = GetTickCount();
#endif

    ASSERT(IS_VALID_STRUCT_PTR(pMRU, MRUDATA));

    if (hMRU == NULL)
        return(-1); // Error state.

    // Can't call this API when it's created lazily.
    if (pMRU->fFlags & MRU_LAZY)
    {
        ASSERT(0);
        return -1;
    }

    uMax = pMRU->uMax;

    /* Find the item in the list.
    */
    for (i=0, ppData=&NTHDATA(pMRU, 0); (UINT)i<uMax; ++i, ++ppData)
    {
        if (!*ppData)
            continue;

        if (MRUIsSameData(pMRU, *ppData, lpData, cbData))
        {
            // So i now has the slot number in it.
            if (lpiSlot != NULL)
                *lpiSlot = i;

            // Now convert the slot number into an index number
            cFirst = i + BASE_CHAR;
            lpTemp = StrChr(pMRU->cOrder, cFirst);
            ASSERT(lpTemp);
            return((lpTemp == NULL)? -1 : (int)(lpTemp - (LPTSTR)pMRU->cOrder));
        }
    }

    return -1;
}


/* Find a string in an MRU list.
 */
STDAPI_(int) FindMRUString(HANDLE hMRU, LPCTSTR szString, LPINT lpiSlot)
{
    /* The extra +1 is so that the list is NULL terminated.
    */
    TCHAR cFirst;
    int iSlot = -1;
    LPTSTR lpTemp;
    LPTSTR FAR *pTemp;
    int i;
    UINT uMax;
    MRUCMPPROC lpfnCompare;
    PMRUDATA pMRU = (PMRUDATA)hMRU;

#ifdef DEBUG
    DWORD dwStart = GetTickCount();
#endif

    ASSERT(IS_VALID_STRUCT_PTR(pMRU, MRUDATA));

    if (hMRU == NULL)
        return(-1); // Error state.

    uMax = pMRU->uMax;
    lpfnCompare = (MRUCMPPROC)pMRU->lpfnCompare;

    /* Find the item in the list.
    */
    for (i=0, pTemp=&NTHSTRING(pMRU, 0); (UINT)i<uMax; ++i, ++pTemp)
    {
        if (*pTemp)
        {
            int iResult;

#ifdef UNICODE
            if (pMRU->fFlags & MRU_ANSI)
            {
                LPSTR lpStringA, lpTempA;

                lpStringA = ProduceAFromW (CP_ACP, szString);
                lpTempA = ProduceAFromW (CP_ACP, (LPWSTR)*pTemp);

                iResult = (*lpfnCompare)((const void FAR *)lpStringA, (const void FAR *)lpTempA);

                FreeProducedString (lpStringA);
                FreeProducedString (lpTempA);
            }
            else
#endif
            {
                iResult = (*lpfnCompare)((CONST VOID FAR *)szString, (CONST VOID FAR *)*pTemp);
            }

            if (!iResult)
            {
                // So i now has the slot number in it.
                if (lpiSlot != NULL)
                    *lpiSlot = i;

                // Now convert the slot number into an index number
                cFirst = i + BASE_CHAR;
                lpTemp = StrChr(pMRU->cOrder, cFirst);
                return((lpTemp == NULL)? -1 : (int)(lpTemp - (LPTSTR)pMRU->cOrder));
            }
        }
    }

    return(-1);
}


#ifdef UNICODE
//
// ANSI thunk
//

int WINAPI FindMRUStringA(HANDLE hMRU, LPCSTR szString, LPINT lpiSlot)
{
    LPWSTR lpStringW;
    INT    iResult;

    lpStringW = ProduceWFromA(CP_ACP, szString);

    iResult = FindMRUString(hMRU, lpStringW, lpiSlot);

    FreeProducedString (lpStringW);

    return iResult;
}

#else

//
// Unicode stub when build ANSI
//

int WINAPI FindMRUStringW(HANDLE hMRU, LPCWSTR szString, LPINT lpiSlot)
{
    SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
    return -1;
}

#endif

/* If lpszString is NULL, then this returns the number of MRU items or less than
 * 0 on error.
 * if nItem < 0, we'll return the number of items currently in the MRU.
 * Otherwise, fill in as much of the buffer as possible (uLen includes the
 * terminating NULL) and return the actual length of the string (including the
 * terminating NULL) or less than 0 on error.
 */
STDAPI_(int) EnumMRUList(HANDLE hMRU, int nItem, LPVOID lpData, UINT uLen)
{
    PMRUDATA pMRU = (PMRUDATA)hMRU;
    int nItems = -1;
    LPTSTR pTemp;
    LPBYTE pData;

    ASSERT(IS_VALID_STRUCT_PTR(pMRU, MRUDATA));

    if (pMRU)
    {
        nItems = lstrlen(pMRU->cOrder);

        if (nItem < 0 || !lpData)
            return nItems;

        if (nItem < nItems)
        {
            if (pMRU->fFlags & MRU_BINARY)
            {
                pData = NTHDATA(pMRU, pMRU->cOrder[nItem]-BASE_CHAR);
                if (!pData)
                    return -1;

                uLen = min((UINT)DATASIZE(pData), uLen);
                hmemcpy(lpData, DATAPDATA(pData), uLen);

                nItems = uLen;

            }
            else
            {
                pTemp = NTHSTRING(pMRU, pMRU->cOrder[nItem]-BASE_CHAR);
                if (!pTemp)
                    return -1;

                lstrcpyn((LPTSTR)lpData, pTemp, uLen);

                nItems = lstrlen(pTemp);
            }
        }
        else  // revert to error condition
            nItems = -1;
    }

    return nItems;
}


#ifdef UNICODE

STDAPI_(int) EnumMRUListA(HANDLE hMRU, int nItem, LPVOID lpData, UINT uLen)
{
    int iResult = -1;
    PMRUDATA pMRU = (PMRUDATA)hMRU;

    ASSERT(IS_VALID_STRUCT_PTR(pMRU, MRUDATA));

    if (pMRU)
    {
        LPVOID lpDataW;
        BOOL bAllocatedMemory = FALSE;

        //
        //  we need a temp buffer if the data is a string.
        //  but if it is binary, then we trust the callers buffer.
        //
        if (!(pMRU->fFlags & MRU_BINARY) && uLen && lpData)
        {
            lpDataW = LocalAlloc(LPTR, uLen * sizeof(TCHAR));

            if (!lpDataW)
                return -1;

            bAllocatedMemory = TRUE;
        }
        else
            lpDataW = lpData;

        //  call the real thing
        iResult = EnumMRUList(hMRU, nItem, lpDataW, uLen);

        //
        //  if the buffer was a string that we allocated
        //  then we need to thunk the string into the callers buffer
        //
        if (!(pMRU->fFlags & MRU_BINARY) && lpData && uLen && (iResult != -1))
        {
            WideCharToMultiByte(CP_ACP, 0, (LPWSTR)lpDataW, -1,
                (LPSTR)lpData, uLen, NULL, NULL);
        }

        if (bAllocatedMemory)
            LocalFree(lpDataW);
    }

    return iResult;
}

#else

STDAPI_(int) EnumMRUListW(HANDLE hMRU, int nItem, LPVOID lpData, UINT uLen)
{
    SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
    return 0;
}

#endif // UNICODE
