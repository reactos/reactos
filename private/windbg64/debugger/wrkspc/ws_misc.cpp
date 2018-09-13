#include "precomp.hxx"
#pragma hdrstop


//
// Used by windbg & windbgrm
//  Serial transport layers
//

LPSTR 
rgszSerialTransportLayers[ROWS_SERIAL_TRANSPORT_LAYERS][COLS_SERIAL_TRANSPORT_LAYERS] = {
    { "SER12",  "Serial - COM1, 1200 baud",     "tlser.dll",    "com1:1200" },
    { "SER24",  "Serial - COM1, 2400 baud",     "tlser.dll",    "com1:2400" },
    { "SER96",  "Serial - COM1, 9600 baud",     "tlser.dll",    "com1:9600" },
    { "SER192", "Serial - COM1, 19200 baud",    "tlser.dll",    "com1:19200" },
    { "SER57K", "Serial - COM1, 57600 baud",    "tlser.dll",    "com1:57600" }
};


#ifdef _CPPRTTI
BOOL
RttiTypesEqual(
    const type_info & t1, 
    const type_info & t2
    )
{
    return t1 == t2;
}
#endif


UINT
WKSP_MultiStrSize(
    PCSTR psz
    )
/*++
Routine Description:
    Returns the total size in char of a multistring, including all terminating zereos.

Arguments:
    psz      -   Supplies the multistring

Return Value:

    UINT - Total size in char. (strlen is UINT) To maintain compatibility with strlen.

--*/
{
    if (NULL == psz) {
        return 0;
    }

    DWORD dwSum = 0;
    DWORD dwCur;

    do {
        dwCur = strlen(psz) +1;
        psz += dwCur;
        dwSum += dwCur;
    } while (*psz);

    return dwSum +1;
}




UINT
WKSP_StrSize(
    PCSTR psz
    )
/*++
Routine Description:
    Returns the total size in char of a string, including the terminating zereo.

Arguments:
    psz      -   Supplies the multistring

Return Value:

    UINT - Total size in char. (strlen is UINT) To maintain compatibility with strlen.

--*/
{
    if (NULL == psz) {
        return 0;
    } else {
        return strlen(psz) + 1;
    }
}


//
// Copied from windbg. Minor modifications.
//


int
CDECL 
WKSP_DisplayLastErrorMsgBox()
{
    PSTR pszErr = WKSP_FormatLastErrorMessage();

    if (pszErr) {
        return MessageBox(NULL, pszErr, NULL, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
    } else {
        Assert(0);
        return 0;
    }
}

/*
**  Description:
**      if pszTitle is is NULL, the default title Error is used. 
**
**      Display a message box with a title, an OK
**      button and a Exclamation Icon. First parameter is a
**      reference string in the ressource file.  The string
**      can contain printf formatting chars, the arguments
**      follow from the second parameter onwards.
*/
int
CDECL
WKSP_MsgBox(
    PSTR pszTitle,
    WORD wErrorFormat,
    ...
    )
{
    int nRes;
    va_list vargs;
    
    va_start(vargs, wErrorFormat);
    nRes = WKSP_VargsCustMsgBox(MB_OK | MB_ICONINFORMATION | MB_TASKMODAL, pszTitle, 
        wErrorFormat, vargs);
    va_end(vargs);

    return nRes;
}

int
CDECL
WKSP_VargsMsgBox(
    PSTR pszTitle,
    WORD wErrorFormat,
    va_list vargs
    )
{
    return WKSP_VargsCustMsgBox(MB_OK | MB_ICONINFORMATION | MB_TASKMODAL, pszTitle, 
        wErrorFormat, vargs);
}

int
CDECL
WKSP_CustMsgBox(
    UINT uStyle, 
    PSTR pszTitle,
    WORD wErrorFormat,
    ...
    )
{
    int nRes;
    va_list vargs;
    
    va_start(vargs, wErrorFormat);
    nRes = WKSP_VargsCustMsgBox(uStyle, pszTitle, wErrorFormat, vargs);
    va_end(vargs);

    return nRes;
}

int
CDECL
WKSP_VargsCustMsgBox(
    UINT uStyle, 
    PSTR pszTitle,
    WORD wErrorFormat,
    va_list vargs
    )
{
    extern HINSTANCE g_hInst;

    char szErrorFormat[MAX_MSG_TXT];
    char szErrorText[MAX_VAR_MSG_TXT];  // size is as big as considered necessary

    // load format string from resource file
    Dbg(LoadString(g_hInst, wErrorFormat, (LPSTR)szErrorFormat, MAX_MSG_TXT));

    vsprintf(szErrorText, szErrorFormat, vargs);

    return MessageBox(NULL, szErrorText, pszTitle, uStyle);
}

BOOL
WKSP_RegKeyValueInfo(
    HKEY hkey,
    PDWORD pdwNumSubKeys,
    PDWORD pdwNumValues,
    PDWORD pdwMaxKeyNameLen,
    PDWORD pdwMaxValueNameLen
    )
{
    Assert(hkey);

    LONG lres;

    lres = RegQueryInfoKey(hkey,               // handle of key to query
                           NULL,               // address of buffer for class string
                           NULL,               // address of size of class string buffer
                           0,                  // reserved
                           pdwNumSubKeys,      // address of buffer for number of subkeys
                           pdwMaxKeyNameLen,   // address of buffer for longest subkey name length
                           NULL,               // address of buffer for longest class string length
                           pdwNumValues,       // address of buffer for number of value entries
                           pdwMaxValueNameLen, // address of buffer for longest value name length
                           NULL,               // address of buffer for longest value data length
                           NULL,               // address of buffer for security descriptor length
                           NULL                // address of buffer for last write time);
                           );
#ifdef DBG
    if (ERROR_SUCCESS != lres) {
        WKSP_DisplayLastErrorMsgBox();
    }
#endif

    return ERROR_SUCCESS == lres;
}

BOOL
WKSP_RegGetKeyName(
    HKEY hkey, 
    DWORD dwIndex, 
    PSTR pszName, 
    PDWORD pcbName
    )
{
    Assert(hkey);

    LONG lres;

    lres = RegEnumKeyEx(hkey, 
                        dwIndex, 
                        pszName, 
                        pcbName, 
                        0, 
                        NULL, 
                        NULL, 
                        NULL
                        );

#ifdef DBG
    if (ERROR_SUCCESS != lres) {
        WKSP_DisplayLastErrorMsgBox();
    }
#endif

    return (ERROR_SUCCESS == lres); 

}

BOOL
WKSP_GetValueName(
    HKEY hkey, 
    DWORD dwIndex, 
    PSTR pszName, 
    PDWORD pcbName,
    PDWORD pdwType
    )
{
    Assert(hkey);

    LONG lres;

    lres = RegEnumValue(hkey, 
                        dwIndex, 
                        pszName, 
                        pcbName, 
                        0, 
                        pdwType, 
                        NULL, 
                        NULL
                        );

#ifdef DBG
    if (ERROR_SUCCESS != lres) {
        WKSP_DisplayLastErrorMsgBox();
    }
#endif

    return (ERROR_SUCCESS == lres);
}

BOOL
WKSP_RegEnumerate(
    HKEY hkey,
    TList< CRegEntry * >   *plistRegKeys,
    TList< CRegEntry * >   *plistRegVals,
    BOOL                    fOpenRegKeys
    )
/*++
Description
    Enumerates all of the name of the key's values or sub-keys.

Arguments:
    hkey - Can't be NULL. Must be an open registry key.

    plistRegKeys - List where the enumerated key names will be kept

    plistRegValues - List where the enumerated values names will be kept

    fOpenRegKeys - TRUE open the handle to the reg key in the plistRegKeys
    
Returns:
    TRUE - Success
    FALSE - Error occured
--*/
{
    Assert(hkey);
    
    LONG        lres;
    DWORD       dwKeys_Num;
    DWORD       dwKeys_MaxNameLen;
    DWORD       dwVals_Num;
    DWORD       dwVals_MaxNameLen;
    PTSTR       pszNameBuf;
    DWORD       dwIdx;
    DWORD       dwMaxIdx;
    DWORD       dwLen;
    CRegEntry  *pRegEntry;
    int         nFlag;
    TList< CRegEntry * > * plist;


    if ( !hkey ) {
        return FALSE;
    }

    if ( !plistRegKeys && !plistRegVals ) {
        return FALSE;
    }

    //
    // Get the number of keys and values
    //
    if ( !WKSP_RegKeyValueInfo(hkey,
                             &dwKeys_Num, 
                             &dwVals_Num, 
                             &dwKeys_MaxNameLen, 
                             &dwVals_MaxNameLen
                             )) {
        return FALSE;
    }

    // Reserve space for the zero terminator
    dwKeys_MaxNameLen++;
    dwVals_MaxNameLen++;

    pszNameBuf = (PTSTR) malloc( max( dwKeys_MaxNameLen, dwVals_MaxNameLen ) );
    if ( !pszNameBuf ) {
        return FALSE;
    }

    //
    // nFlag signifies the following values
    //      0 - Enumerate keys
    //      1 - Enumerate values
    //      2 - finished
    //
    for (nFlag=0; nFlag < 2; nFlag++) {

        // If this is not set to something other
        // than 0, then the for loop falls thru.
        dwMaxIdx = 0;

        // Figure out which list to fill
        if (0 == nFlag && plistRegKeys) {
            dwMaxIdx = dwKeys_Num;
            plist = plistRegKeys;
        } else if (1 == nFlag && plistRegVals) {
            dwMaxIdx = dwVals_Num;
            plist = plistRegVals;
        }

        for (dwIdx=0; dwIdx < dwMaxIdx; dwIdx++) {
    
            dwLen = dwKeys_MaxNameLen;
    
            WKSP_RegGetKeyName(hkey, dwIdx, pszNameBuf, &dwLen);

            pRegEntry = new CRegEntry;
            if ( !pRegEntry ) {
                goto ERROR_CLEANUP;
            }

            pRegEntry->m_pszName = _strdup(pszNameBuf);

            // Open reg key?
            if (0 == nFlag && fOpenRegKeys) {
                
                if (ERROR_SUCCESS != RegOpenKeyEx(hkey, 
                                                  pRegEntry->m_pszName, 
                                                  0, 
                                                  KEY_ALL_ACCESS, 
                                                  &pRegEntry->m_hkey
                                                  )) {
                    goto ERROR_CLEANUP;
                }
            }

            plist->InsertTail( pRegEntry );
        }
    }

    return TRUE;


ERROR_CLEANUP:

    //
    // Delete the list of keys
    //
    if (plistRegKeys) {
        while ( !plistRegKeys->IsEmpty() ) {
            
            pRegEntry = plistRegKeys->GetHeadData();
            
            plistRegKeys->RemoveHead();
            
            pRegEntry->CleanUp();
            delete pRegEntry;
        }
    }

    //
    // Delete the list of values
    //
    if (plistRegVals) {
        while ( !plistRegVals->IsEmpty() ) {
            
            pRegEntry = plistRegVals->GetHeadData();

            plistRegVals->RemoveHead();
            
            pRegEntry->CleanUp();
            delete pRegEntry;
        }
    }

    return FALSE;
}


void
WKSP_RegDeleteValues(
    HKEY hkey
    )
/*++
Routine Description:
    Helper function. Deletes all of the values contained by a registry key.

    Does not delete any keys or the specified key.

Arguments:
    hkey - Cannot be NULL.  Must point to an open registry key.
--*/
{
    Assert(hkey);

    LONG lres;
    DWORD dwMaxNumValues;
    DWORD dwMaxNameSize;

    WKSP_RegKeyValueInfo(hkey, NULL, &dwMaxNumValues, NULL, &dwMaxNameSize);

    // Include space for the null terminator.
    dwMaxNameSize++;
    PSTR pszBuffer = (PSTR) calloc(dwMaxNameSize, 1);
    Assert(pszBuffer);

    for (DWORD dwCounter = 0; dwCounter < dwMaxNumValues; dwCounter++) {
        DWORD dwLen = dwMaxNameSize;

        lres = WKSP_GetValueName(hkey, 0, pszBuffer, &dwLen, NULL);
#ifdef DBG
        if (ERROR_SUCCESS != lres) {
            WKSP_DisplayLastErrorMsgBox();
        }
#endif

        lres = RegDeleteValue(hkey, pszBuffer);
#ifdef DBG
        if (ERROR_SUCCESS != lres) {
            WKSP_DisplayLastErrorMsgBox();
        }
#endif
    }

    free(pszBuffer);
}





void
WKSP_RegDeleteSubKeys(
    HKEY hkey
    )
/*++
Routine Description:
    Recursively deletes subkeys.

    Helper function. Will only delete the subkeys within the specified key. The values
    within the specified key will not be deleted.

    The specified key is not deleted.

    See docs for "RegDeleteKey".

Arguments:
    hkey - Can't be NULL. Must be an open registry key.
--*/
{
    Assert(hkey);

    LONG lres;
    DWORD dwMaxNameSize, dwMaxNumSubKeys;

    WKSP_RegKeyValueInfo(hkey, &dwMaxNumSubKeys, NULL, &dwMaxNameSize, NULL);

    // Include space for the null terminator.
    dwMaxNameSize++;
    PSTR pszBuffer = (PSTR) calloc(dwMaxNameSize, 1);
    Assert(pszBuffer);

    // Treat the reg keys as a list, by simply deleting the first key, the
    //  required amount of times.
    for (DWORD dwCounter = 0; dwCounter < dwMaxNumSubKeys; dwCounter++) {
        DWORD dwLen = dwMaxNameSize;
        HKEY hkeyChild;

        // Get the first subkey
        WKSP_RegGetKeyName(hkey, 0, pszBuffer, &dwLen);

        lres = RegOpenKeyEx(hkey, pszBuffer, 0, KEY_ALL_ACCESS, &hkeyChild);
#ifdef DBG
        if (ERROR_SUCCESS != lres) {
            WKSP_DisplayLastErrorMsgBox();
        }
#endif

        // Delete any subkeys it may have
        WKSP_RegDeleteSubKeys(hkeyChild);

        // Delete this subkey
        lres = RegCloseKey(hkeyChild);
#ifdef DBG
        if (ERROR_SUCCESS != lres) {
            WKSP_DisplayLastErrorMsgBox();
        }
#endif

        lres = RegDeleteKey(hkey, pszBuffer);
#ifdef DBG
        if (ERROR_SUCCESS != lres) {
            WKSP_DisplayLastErrorMsgBox();
        }
#endif
    }

    free(pszBuffer);
}


void
WKSP_RegDeleteContents(
    HKEY hkey
    )
/*++
Routine Description:
    Deletes all of the values within the specified key.

    Recursively deletes all subkeys. However, the specified key is not deleted.

Arguments:
    hkey - Can't be NULL. Must be an open registry key.
--*/
{
    Assert(hkey);
    WKSP_RegDeleteValues(hkey);
    WKSP_RegDeleteSubKeys(hkey);
}

BOOL
WKSP_RegDeleteKey(
    HKEY hkeyParent,
    PSTR pszKeyName
    )
/*++
Routine Description:
    Deletes the key specified byt pszKeyName and all of its children.

    Does not matter if it contains values and subkeys.


Arguments:
    hkey - Can't be NULL. Must be an open registry key.
    
    pszKeyName - Name of subkey to delete
--*/
{
    Assert(hkeyParent);
    Assert(pszKeyName);


    HKEY hkeyChild;
    long lres;

    lres = RegOpenKeyEx(hkeyParent, 
                        pszKeyName,
                        0, 
                        KEY_ALL_ACCESS, 
                        &hkeyChild
                        );

#ifdef DBG
    if (ERROR_SUCCESS != lres) {
        WKSP_DisplayLastErrorMsgBox();
    }
#endif

    if (ERROR_SUCCESS != lres) {
        return FALSE;
    }

    WKSP_RegDeleteContents(hkeyChild);

    lres = RegCloseKey(hkeyChild);
#ifdef DBG
    if (ERROR_SUCCESS != lres) {
        WKSP_DisplayLastErrorMsgBox();
    }
#endif
    if (ERROR_SUCCESS != lres) {
        return FALSE;
    }

    lres = RegDeleteKey(hkeyParent, pszKeyName);
#ifdef DBG
    if (ERROR_SUCCESS != lres) {
        WKSP_DisplayLastErrorMsgBox();
    }
#endif
    if (ERROR_SUCCESS != lres) {
        return FALSE;
    }

    return TRUE;
}

BOOL
WKSP_RegKeyExist(
    HKEY hkeyParent, 
    PCSTR pszKeyName
    )
/*++
Routine Description:
    Queries the registry as to whether the key exists.

    The query consists of simply trying to open the reg key. This
    can fail even though the key exists, but the user has insufficient
    rights.

Arguments:
    hkeyParent - Can't be NULL. Must be an open registry key.

    pszKeyName - Name of registry key to open.

Returns:
    TRUE - if it was possible to open the reg key.
    FALSE - if it was not possible to open the reg key.
--*/
{
    Assert(hkeyParent);
    Assert(pszKeyName);

    HKEY hkeyResult;

    long lres = RegOpenKeyEx(hkeyParent, pszKeyName,
                                0, KEY_ALL_ACCESS, &hkeyResult);

    if (ERROR_SUCCESS == lres) {
        lres = RegCloseKey(hkeyResult);
#ifdef DBG
        if (ERROR_SUCCESS != lres) {
            WKSP_DisplayLastErrorMsgBox();
        }
#endif
    }

    return ERROR_SUCCESS == lres;
}


HKEY
WKSP_RegKeyOpenCreate(
    HKEY    hkeyParent,
    PCSTR   pszKeyName,
    PBOOL   pbRegKeyCreated
    )
/*++
Routine Description:
    Queries the registry as to whether the key exists.

    The query consists of simply trying to open the reg key. This
    can fail even though the key exists, but the user has insufficient
    rights.

Arguments:
    hkeyParent - Can't be NULL. Must be an open registry key.

    pszKeyName -  Name of key that is to be created and or opened.
    
    pbRegKeyCreated - Can be NULL. If not NULL, it is assigned TRUE if the
        reg key was created, FALSE if it already existed.

Returns:
    An HKEY to the newly opened/created reg key.
--*/
{
    HKEY hkeyResult = 0;
    long lresult = 0;
    BOOL bExist;

    bExist = WKSP_RegKeyExist(hkeyParent, pszKeyName);
    if (bExist) {
        lresult = RegOpenKeyEx(hkeyParent, 
                               pszKeyName,
                               0, 
                               KEY_ALL_ACCESS, 
                               &hkeyResult
                               );
    } else {
        DWORD dwDisposition;
        lresult = RegCreateKeyEx(hkeyParent, 
                                 pszKeyName,
                                 0, 
                                 NULL, 
                                 REG_OPTION_NON_VOLATILE, 
                                 KEY_ALL_ACCESS, 
                                 NULL,
                                 &hkeyResult, 
                                 &dwDisposition
                                 );
    }

    if (ERROR_SUCCESS != lresult) {
        MessageBox(NULL, 
                   "An error occurred while trying to write to registry. "
                   "Please make sure your user account has permission to modify it.",
                   NULL,
                   MB_OK | MB_ICONINFORMATION | MB_TASKMODAL
                   );
    }

    if (pbRegKeyCreated) {
        *pbRegKeyCreated = !bExist;
    }

    return hkeyResult;
}


BOOL
WKSP_CmpRegName(
    const void * const pv, 
    CGenInterface_WKSP * pGenInt
    )
{
    Assert(pv);
    Assert(pGenInt);

    return 0 == strcmp( (PSTR) pv, pGenInt->m_pszRegistryName);
}




PSTR
WKSP_DynaLoadStringWithArgs(  
    HINSTANCE hInstance,
    UINT uID,
    ...
    )
{
    va_list va_arg;
    va_start(va_arg, uID);

    __declspec( thread ) static char szBuffer[1024 * 10] = {0};
    PSTR psz = WKSP_DynaLoadString(hInstance, uID);
    Assert(psz);

    if (0x7c == uID) {
        if (strlen(psz) > 48) {
            DebugBreak();
        }
    }

    vsprintf(szBuffer, psz, va_arg) ;
    
    va_end(va_arg);

    free(psz);
    Assert(strlen(szBuffer) < sizeof(szBuffer));
    
    return _strdup(szBuffer);
}


PSTR
WKSP_DynaLoadString(  
    HINSTANCE hInstance,
    UINT uID
    )
{
    HRSRC hrsrc = NULL;
    DWORD dwSize = 0;
    PSTR pszStr = NULL;

    // The strings are stored in groups of 16
    hrsrc = FindResource(hInstance, MAKEINTRESOURCE(uID/16 +1), RT_STRING);
    Assert(hrsrc);

    dwSize = SizeofResource(hInstance, hrsrc);
    if (dwSize) {
        PSTR pszTmp = (PSTR) calloc(dwSize, 1);
        if (pszTmp) {
            // +2 to include the null terminators if it is terminated with 2 zeros
            int nCharsCopied = LoadString(hInstance, uID, pszTmp, dwSize) +2;
            Assert(nCharsCopied);

            pszStr = (PSTR) malloc(nCharsCopied);
            memcpy(pszStr, pszTmp, nCharsCopied);

            free(pszTmp);
        }
    }

    return pszStr;
}


PTSTR
WKSP_FormatLastErrorMessage()
{
    PTSTR pszRetString = NULL;
    PTSTR pszMsgBuf = NULL;
    
    if (FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (PTSTR) &pszMsgBuf,
        0,
        NULL)) {

        pszRetString = _strdup(pszMsgBuf);
        LocalFree(pszMsgBuf);
    }

    return pszRetString;
}

void
WKSP_AppendStrToMultiStr(
    PTSTR & pszOriginal,
    PTSTR pszNewStrToAppend
    )
{
    if (!pszOriginal) {
        // Alocate space for the double terminator
        pszOriginal = (PSTR) calloc(strlen(pszNewStrToAppend) +2, sizeof(TCHAR));
        strcpy(pszOriginal, pszNewStrToAppend);
        return;
    }
        
    // +1 need to include the space for the NULL
    UINT uSize = WKSP_MultiStrSize(pszOriginal);
    // terminator of the new string
    PSTR pszNewBuf = (PSTR) realloc(pszOriginal, 
        (uSize + strlen(pszNewStrToAppend) +1) * sizeof(TCHAR) );

    if (pszNewBuf) {
        // Add the new string
        pszOriginal = pszNewBuf;

        if (uSize) {
            // Move the pointer to the end
            pszNewBuf += uSize -1;
        }
        strcpy(pszNewBuf, pszNewStrToAppend);
        pszNewBuf += strlen(pszNewStrToAppend) +1;
        *pszNewBuf = NULL;
    }
}
