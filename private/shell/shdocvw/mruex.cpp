#include "priv.h"

// SHGetViewStream needs this and the web browser OC needs SHGetViewStream,
// so this either lives here or shlwapi.
#include "mruex.h"

// this is swiped from comctl32\mru.c

#define MRU_ORDERDIRTY 0x1000

#define DM_MRULAZY  0

typedef struct tagMRUDATA
{
    UINT fFlags;
    UINT uMax;
    MRUCMPPROC lpfnCompare;
    HKEY hKey;
#ifdef DEBUG
    TCHAR szSubKey[32];
#endif
    LPDWORD cOrder;
} MRUDATA, *PMRUDATA;

#define szMRUEX         TEXT("MRUListEx")
#define szMRUEX_OLD     TEXT("MRUList")
#define c_szShell       TEXT("Shell")


#define NTHSTRING(p, n) (*((LPTSTR FAR *)((LPBYTE)p+sizeof(MRUDATA))+n))
#define NTHDATA(p, n) (*((LPBYTE FAR *)((LPBYTE)p+sizeof(MRUDATA))+n))
#define NUM_OVERHEAD 3
#define MAX_MRU_INDEXSTR        15


//----------------------------------------------------------------------------
//  For binary data we stick the size of the data at the begining and store the
//  whole thing in one go.
// Use this macro to get the original size of the data.
#define DATASIZE(p)  (*((LPDWORD)p))
// And this to get a pointer to the original data.
#define DATAPDATA(p) (p + sizeof(DWORD))

#define DATAPDATAEX(p) ((LPDWORD) ((DWORD_PTR)p + sizeof(DWORD)))


HRESULT GetIndexStrFromIndex(DWORD dwIndex, LPTSTR pszIndexStr, DWORD cchIndexStrSize)
{
    wnsprintf(pszIndexStr, cchIndexStrSize, TEXT("%d"), dwIndex);

    return S_OK;
}

#define MAX_CHAR 126
#define BASE_CHAR TEXT('a')

DWORD ConvertOldIndexToNewIndex(TCHAR chOldMRUIndex)
{
    //  limit to 126 so that we don't use extended chars
    ASSERT(chOldMRUIndex <= MAX_CHAR);

    return (chOldMRUIndex - BASE_CHAR);
}


LPDWORD GetMRUValue(HKEY hkeySubKey, LPCTSTR pszRegValue)
{
    LPDWORD pResult = NULL;
    DWORD cbVal;

    // Get the size
    if (ERROR_SUCCESS == RegQueryValueEx(hkeySubKey, pszRegValue, NULL, NULL, NULL, &cbVal))
    {
        // Binary data has the size at the begining so we'll need a little extra room.
        pResult = (LPDWORD)Alloc(cbVal + sizeof(DWORD));
        if (pResult)
        {
            DATASIZE(pResult) = cbVal;     // Set the size.

            // Can we successfully get the data?
            if (ERROR_SUCCESS != RegQueryValueEx(hkeySubKey, pszRegValue, NULL, NULL, (LPBYTE) DATAPDATAEX(pResult), &cbVal))
            {
                // No, so free the buffer and make sure we return NULL.
                Free((HLOCAL)pResult);
                pResult = NULL;
            }
        }
    }

    return pResult;
}

HRESULT SetMRUValue(HKEY hkeySubKey, LPCTSTR pszRegValue, LPDWORD pData)
{
    HRESULT hr = E_FAIL;

    // The first DWORD in pData is the size of the rest of pData.
    // The real data is stored starting in the second DWORD.
    if (ERROR_SUCCESS == RegSetValueEx(hkeySubKey, pszRegValue, NULL, REG_BINARY, (LPBYTE)DATAPDATAEX(pData), DATASIZE(pData)))
        hr = S_OK;

    return hr;
}

HRESULT ImportOldMRU(HKEY hkeySubKey, LPDWORD pOrder, LPDWORD pcbVal)
{
    HRESULT hr = E_FAIL;
    TCHAR szOldMRU[MAX_PATH];   // The old MRU where chars and less than MAX_PATH
    DWORD cbOldMRUStr = sizeof(szOldMRU);

    if (ERROR_SUCCESS == RegQueryValueEx(hkeySubKey, (LPTSTR)szMRUEX_OLD, NULL, NULL, (LPBYTE)szOldMRU, &cbOldMRUStr))
    {
        DWORD dwIndex = 0;
        TCHAR szOldIndex[2];
        szOldIndex[1] = TEXT('\0'); // Terminate the 2 char string.

        while (TEXT('\0') != szOldMRU[dwIndex]) // While we aren't at the end of the list
        {
            DWORD dwNewIndex = ConvertOldIndexToNewIndex(szOldMRU[dwIndex]);
            LPDWORD pData;

            szOldIndex[0] = szOldMRU[dwIndex]; // Create the Old MRU Index in the form of a string.
            pData = GetMRUValue(hkeySubKey, szOldIndex);
            if (pData)
            {
                TCHAR szNewIndexStr[MAX_MRU_INDEXSTR];

                GetIndexStrFromIndex(dwNewIndex, szNewIndexStr, ARRAYSIZE(szNewIndexStr));
                SetMRUValue(hkeySubKey, szNewIndexStr, pData);

                Free((HLOCAL)pData);
            }

            pOrder[dwIndex] = dwNewIndex;    // Copy from the old Index to the new one.
            dwIndex++;
        }

        pOrder[dwIndex] = -1;    // Terminate the index.
        *pcbVal = sizeof(*pOrder) * dwIndex;    // Set the size.

        hr = S_OK;
    }

    return hr;
}



//----------------------------------------------------------------------------
// Internal memcmp - saves loading crt's, cdecl so we can use
// as MRUCMPDATAPROC

int CDECL _mymemcmp(const void *pBuf1, const void *pBuf2, size_t cb)
{
    UINT i;
    const BYTE *lpb1, *lpb2;

    ASSERT(pBuf1);
    ASSERT(pBuf2);

    lpb1 = (const BYTE *)pBuf1; lpb2 = (const BYTE *)pBuf2;

    for (i=0; i < cb; i++)
    {
        if (*lpb1 > *lpb2)
            return 1;
        else if (*lpb1 < *lpb2)
            return -1;

        lpb1++;
        lpb2++;
    }

    return 0;
}

BOOL MRUIsSameData(PMRUDATA pMRU, BYTE FAR* pVal, const void FAR *lpData, UINT cbData)
{
    int cbUseSize;
    MRUCMPDATAPROC lpfnCompare;

    if (!pVal)
        return FALSE;

    lpfnCompare = (MRUCMPDATAPROC)(pMRU->lpfnCompare);
    // if there's something other than a mem compare,
    // don't require the sizes to be equal in order for the
    // data to be equivalent.

    if ((LPVOID)(pMRU->lpfnCompare) == (LPVOID)(_mymemcmp))
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
HANDLE CreateMRUListLazyEx(LPMRUINFO lpmi, const void FAR *lpData, UINT cbData, LPINT lpiSlot)
{
    HANDLE hMRU = NULL;
    LPDWORD pOrder, pNewOrder, pTemp;
    LPBYTE pVal;
    DWORD cbVal;
#ifdef WIN32
    DWORD dwDisposition;
#endif
    DWORD dwType;
    PMRUDATA pMRU = NULL;
    HKEY hkeySubKey = NULL;
    TCHAR szTemp[10];
    UINT uMax = lpmi->uMax;
    HKEY hKey = lpmi->hKey;
    LPCTSTR lpszSubKey = lpmi->lpszSubKey;
    MRUCMPPROC lpfnCompare = lpmi->lpfnCompare;
    int cb;

#ifdef DEBUG
    DWORD dwStart = GetTickCount();
#endif
    if (!lpfnCompare) {
        lpfnCompare = (lpmi->fFlags & MRU_BINARY) ? (MRUCMPPROC)_mymemcmp : (MRUCMPPROC)StrCmpI;
    }

    if (RegCreateKeyEx(hKey, lpszSubKey, 0L, (LPTSTR)c_szShell, REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkeySubKey, &dwDisposition) != ERROR_SUCCESS)
        goto Error1;

    cbVal = ((LONG)uMax + 1) * sizeof(DWORD);
    pOrder = (LPDWORD)Alloc(cbVal);
    if (!pOrder) {
        goto Error1;
    }

    // Do we already have the new MRU Index?
    if (RegQueryValueEx(hkeySubKey, (LPTSTR)szMRUEX, NULL, &dwType, (LPBYTE)pOrder, &cbVal) == ERROR_SUCCESS)
    {
        // Then validate it.  You can never trust the registry not to be
        // corrupted.

        // Must be at least the size of a DWORD
        if (cbVal < sizeof(DWORD)) goto NotInRegistry;

        // Must be a multiple of DWORD in length
        if (cbVal % sizeof(DWORD)) goto NotInRegistry;

        // Must end in a -1
        if (pOrder[cbVal/sizeof(DWORD) - 1] != (DWORD)-1) goto NotInRegistry;

        // If any interior values are corrupted, we will detect that
        // during the traversal loop.
    }
    else
    {
NotInRegistry:
        // The new MRU didn't exist, so look for an old one to import.
        if (FAILED(ImportOldMRU(hkeySubKey, pOrder, &cbVal)))
        {
            // There wasn't an old MRU to import, so start fresh
            *pOrder = (DWORD)-1;
        }
    }

    // We allocate room for the MRUDATA structure, plus the order list,
    // and the list of strings.
    cb = (lpmi->fFlags & MRU_BINARY) ? sizeof(LPBYTE) : sizeof(LPTSTR);
    pMRU = (PMRUDATA)Alloc(sizeof(MRUDATA)+(uMax*cb));
    if (!pMRU) {
        goto Error2;
    }

    // Allocate space for the order list
    pMRU->cOrder = (LPDWORD)Alloc((uMax+1)*sizeof(DWORD));
    if (!pMRU->cOrder) {
        Free(pMRU);
        pMRU = NULL;
        goto Error2;
    }

    pMRU->fFlags = lpmi->fFlags;
    pMRU->uMax = uMax;
    pMRU->lpfnCompare = lpfnCompare;
    pMRU->hKey = hkeySubKey;
#ifdef DEBUG
    StrCpyN(pMRU->szSubKey, lpszSubKey, ARRAYSIZE(pMRU->szSubKey));
#endif

    // Traverse through the MRU list, adding strings to the end of the
    // list.
    for (pTemp = pOrder, pNewOrder = pMRU->cOrder; *pTemp != (DWORD)-1; ++pTemp)
    {
        GetIndexStrFromIndex(*pTemp, szTemp, ARRAYSIZE(szTemp));

        if (lpmi->fFlags & MRU_BINARY) {
            // Check if in range and if we have already used this letter.
            if (*pTemp>=uMax || NTHDATA(pMRU, *pTemp)) {
                continue;
            }

            // BUGBUG: Convert to use GetMRUValue();

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
            NTHDATA(pMRU, *pTemp) = pVal;
            *pNewOrder++ = *pTemp;

            //
            // OPTIMIZATION
            //   If lpData and lpiSlot are specified, we stop the enumeratation
            //  when we find the item. 
            //
            if (lpData && lpiSlot) {
                // Check if we have the specified one or not.
                if (MRUIsSameData(pMRU, pVal, lpData, cbData)) {
                    // Found it. 
                    *lpiSlot = (int) (pNewOrder-pMRU->cOrder);

                    TraceMsg(DM_MRULAZY, "CreateMRUListLazy found it. Copying %d", *pTemp);

                    pMRU->fFlags |= MRU_LAZY;
                    //
                    // Copy the rest of slot. Notice that we don't load
                    // data for those slot.
                    //
                    for (pTemp++; -1 != *pTemp; pTemp++) {
                        *pNewOrder++ = *pTemp;
                    }
                    break;
                }
            }
        } else {
            AssertMsg(0, TEXT("Functionality NOT IMPLEMENTED"));
        }
    }
    // terminate the order list
    *pNewOrder = (DWORD)-1;

    if (lpData && lpiSlot) {
        TraceMsg(DM_MRULAZY, "CreateMRUListLazy. End of loop. %c", pMRU->cOrder);
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

HANDLE CreateMRUListEx(LPMRUINFO lpmi)
{
    return CreateMRUListLazyEx(lpmi, NULL, 0, NULL);
}

LPDWORD OrderStrChr(LPDWORD pdwOrder, DWORD dwVal)
{
    while (*pdwOrder != (DWORD)-1)
    {
        if (*pdwOrder == dwVal)
            return pdwOrder;

        pdwOrder++;
    }
    return NULL;
}

int OrderLength(LPDWORD pdwOrder)
{
    int len = 0;
    do {
        len++;
    } while (*pdwOrder++ != (DWORD)-1);

    return len;
}

void SaveMRUOrder(PMRUDATA pMRU)
{
    if (EVAL(pMRU->cOrder)) // See BryanSt if this assert.
    {
        RegSetValueEx(pMRU->hKey, szMRUEX, 0L,
                  REG_BINARY, (CONST BYTE *)pMRU->cOrder, sizeof(DWORD) * OrderLength(pMRU->cOrder));
    }

    pMRU->fFlags &= ~MRU_ORDERDIRTY;
}


#define pMRU ((PMRUDATA)hMRU)
//----------------------------------------------------------------------------
void FreeMRUListEx(HANDLE hMRU)
{
    int i;
    LPBYTE *pTemp;

    pTemp = (pMRU->fFlags & MRU_BINARY) ?
        &NTHDATA(pMRU, 0) : (LPBYTE FAR *)&NTHSTRING(pMRU, 0);

    if (pMRU->fFlags & MRU_ORDERDIRTY)
    {
        SaveMRUOrder(pMRU);
    }                      

    for (i=pMRU->uMax-1; i>=0; --i, ++pTemp)
    {
        if (*pTemp) {
            if (pMRU->fFlags & MRU_BINARY) {
                Free(*pTemp);
                *pTemp = NULL;
            } else {
                Str_SetPtr((LPTSTR FAR *)pTemp, NULL);
            }
        }
    }
    RegCloseKey(pMRU->hKey);
    Free(pMRU->cOrder);
    Free((HLOCAL)pMRU);
}


//----------------------------------------------------------------------------
// Add data to an MRU list.
int AddMRUDataEx(HANDLE hMRU, const void FAR *lpData, UINT cbData)
{
    DWORD cFirst;
    int iSlot = -1;
    LPDWORD lpTemp;
    LPBYTE FAR *ppData;
    int i;
    UINT uMax;
    MRUCMPDATAPROC lpfnCompare;
    BOOL fShouldWrite = !(pMRU->fFlags & MRU_CACHEWRITE);

#ifdef DEBUG
    DWORD dwStart = GetTickCount();
#endif
    if (hMRU == NULL)
        return(-1);     // Error

    uMax = pMRU->uMax;
    lpfnCompare = (MRUCMPDATAPROC)pMRU->lpfnCompare;

    // Check if the data already exists in the list.
    for (i=0, ppData=&NTHDATA(pMRU, 0); (UINT)i<uMax; ++i, ++ppData)
    {
        if (*ppData && MRUIsSameData(pMRU, *ppData, lpData, cbData))
        {
            // found it, so don't do the write out
            cFirst = i;
            iSlot = i;
            goto FoundEntry;
        }
    }

    //
    // When created "lazy", we are not supposed to add a new item.
    //
    if (pMRU->fFlags & MRU_LAZY) {
        ASSERT(0);
        return -1;
    }

    // Attempt to find an unused entry.  Count up the used entries at the
    // same time.
    for (i=0, ppData=&NTHDATA(pMRU, 0); ; ++i, ++ppData)
    {
        // If we got to the end of the list.
        if ((UINT)i >= uMax)
        {
            // use the entry at the end of the cOrder list
            cFirst = pMRU->cOrder[uMax-1];
            ppData = &NTHDATA(pMRU, cFirst);
            break;
        }

        // If the entry is not used.
        if (!*ppData)
        {
            cFirst = i;
            break;
        }
    }

    *ppData = (LPBYTE)ReAlloc(*ppData, cbData+sizeof(DWORD));
    if (*ppData)
    {
        TCHAR szTemp[MAX_MRU_INDEXSTR];

        *((LPDWORD)(*ppData)) = cbData;
        hmemcpy(DATAPDATA(*ppData), lpData, cbData);

        iSlot = (int)(cFirst);

        GetIndexStrFromIndex(cFirst, szTemp, ARRAYSIZE(szTemp));

        RegSetValueEx(pMRU->hKey, szTemp, 0L, REG_BINARY, (LPBYTE)lpData, cbData);
        fShouldWrite = TRUE;
    }
    else
    {
        // Since iSlot == -1, we will remove the reference to cFirst
        // below.
    }

FoundEntry:
    // Remove any previous reference to cFirst.
    lpTemp = OrderStrChr(pMRU->cOrder, cFirst);
    if (lpTemp)
    {
        MoveMemory(lpTemp, lpTemp+1, (pMRU->uMax - (lpTemp-pMRU->cOrder))*sizeof(DWORD));
    }

    if (iSlot != -1)
    {
        // shift everything over and put cFirst at the front
        hmemcpy(pMRU->cOrder+1, pMRU->cOrder, pMRU->uMax*sizeof(DWORD));
        pMRU->cOrder[0] = cFirst;
    }

    if (fShouldWrite) {
        SaveMRUOrder(pMRU);
    } else
        pMRU->fFlags |= MRU_ORDERDIRTY;

#ifdef DEBUG
    // DebugMsg(DM_TRACE, TEXT("AddMRU: %d msec"), LOWORD(GetTickCount()-dwStart));
#endif
    return(iSlot);
}

