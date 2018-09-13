//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       wvtver1.cpp
//
//  Contents:   Microsoft Internet Security WinVerifyTrust v1 support
//
//  Functions:  WintrustIsVersion1ActionID
//              ConvertDataFromVersion1
//
//              *** local functions ***
//
//  History:    30-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "wvtver1.h"

BOOL WintrustIsVersion1ActionID(GUID *pgActionID)
{
    GUID    gV1UISup    = V1_WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
    GUID    gV1UINoBad  = V1_WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI;

    if ((memcmp(pgActionID, &gV1UISup, sizeof(GUID)) == 0) ||
        (memcmp(pgActionID, &gV1UINoBad, sizeof(GUID)) == 0))
    {
        return(TRUE);
    }

    return(FALSE);
}

#ifndef WIN_SPUB_ACTION_TRUSTED_PUBLISHER   // still referenced in winbase.h?
//
//  old v1 structures
//
typedef struct _WIN_TRUST_SUBJECT_FILE 
{
    HANDLE  hFile;
    LPCWSTR lpPath;
} WIN_TRUST_SUBJECT_FILE;

typedef struct _WIN_TRUST_ACTDATA_CONTEXT_WITH_SUBJECT 
{
    HANDLE                  hClientToken;
    GUID *                  SubjectType;
    WIN_TRUST_SUBJECT_FILE  *Subject;
} WIN_TRUST_ACTDATA_CONTEXT_WITH_SUBJECT;

#endif // BUGBUG

WINTRUST_DATA *ConvertDataFromVersion1(HWND hWnd,
                                       GUID *pgActionID,
                                       WINTRUST_DATA *pWTDNew, 
                                       WINTRUST_FILE_INFO *pWTFINew, 
                                       LPVOID pWTDOld)
{
    GUID                                    gV1UINoBad  = V1_WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI;

    WCHAR                                   *pwszFile;
    WIN_TRUST_ACTDATA_CONTEXT_WITH_SUBJECT  *pActData;

    pActData    = (WIN_TRUST_ACTDATA_CONTEXT_WITH_SUBJECT *)pWTDOld;

    memset(pWTDNew, 0x00, sizeof(WINTRUST_DATA));
    pWTDNew->cbStruct           = sizeof(WINTRUST_DATA);
    pWTDNew->dwUnionChoice      = WTD_CHOICE_FILE;
    pWTDNew->pFile              = pWTFINew;

    memset(pWTFINew, 0x00, sizeof(WINTRUST_FILE_INFO));
    pWTFINew->cbStruct          = sizeof(WINTRUST_FILE_INFO);

    if (!(pWTDOld))
    {
        return(pWTDNew);
    }

    pWTDNew->dwUIChoice             = WTD_UI_ALL;
    pWTDNew->pPolicyCallbackData    = pActData->hClientToken;
    pWTFINew->hFile                 = ((WIN_TRUST_SUBJECT_FILE *)pActData->Subject)->hFile;

    if (memcmp(&gV1UINoBad, pgActionID, sizeof(GUID)) == 0)
    {
        pWTDNew->dwUIChoice     = WTD_UI_NOBAD;
    }

    if (hWnd == (HWND)(-1))
    {
        pWTDNew->dwUIChoice     = WTD_UI_NONE;
    }

    pwszFile                        = (WCHAR *)((WIN_TRUST_SUBJECT_FILE  *)pActData->Subject)->lpPath;

    while ((*pwszFile) && (*pwszFile != '|'))
    {
        ++pwszFile;
    }

    if (*pwszFile)
    {
        *pwszFile = NULL;
    }
    
    pWTFINew->pcwszFilePath = (WCHAR *)((WIN_TRUST_SUBJECT_FILE  *)pActData->Subject)->lpPath;

    return(pWTDNew);
}



//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  the following code implements the version 1 style of calling trust providers.
//
//  this code is ONLY implemented when a trust provider registers itself in the
//  old location!
//
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////


#define WIN_TRUST_MAJOR_REVISION_MASK       0xFFFF0000
#define WIN_TRUST_MINOR_REVISION_MASK       0x0000FFFF
#define WIN_TRUST_REVISION_1_0              0x00010000

#define REGISTRY_TRUSTPROVIDERS TEXT("System\\CurrentControlSet\\Services\\WinTrust\\TrustProviders")
#define REGISTRY_ROOT           HKEY_LOCAL_MACHINE

#define ACTION_IDS              TEXT("$ActionIDs")
#define DLL_NAME                TEXT("$DLL")

#define IsEqualActionID( id1, id2)    (!memcmp(id1, id2, sizeof(GUID))) 

typedef struct _WINTRUST_CLIENT_TP_INFO {
    DWORD                                   dwRevision;
    //LPWINTRUST_CLIENT_TP_DISPATCH_TABLE     lpServices;
    LPVOID                                  lpServices;
} WINTRUST_CLIENT_TP_INFO,  *LPWINTRUST_CLIENT_TP_INFO;

typedef LONG
(*LPWINTRUST_PROVIDER_VERIFY_TRUST) (
    IN     HWND                             hwnd,
    IN     GUID *                           ActionID,
    IN     LPVOID                           ActionData
    );

typedef VOID
(*LPWINTRUST_PROVIDER_SUBMIT_CERTIFICATE) (
    IN     LPWIN_CERTIFICATE                lpCertificate
    );

typedef VOID
(*LPWINTRUST_PROVIDER_CLIENT_UNLOAD) (
    IN     LPVOID                           lpTrustProviderInfo
    );

typedef BOOL
(*LPWINTRUST_PROVIDER_CLIENT_INITIALIZE)(
    IN     DWORD                                dwWinTrustRevision,
    IN     LPWINTRUST_CLIENT_TP_INFO            lpWinTrustInfo,
    IN     LPWSTR                               lpProviderName,
    LPVOID                                      *lpTrustProviderInfo
//    OUT    LPWINTRUST_PROVIDER_CLIENT_INFO      *lpTrustProviderInfo
    );

typedef struct _WINTRUST_PROVIDER_CLIENT_SERVICES
{
    LPWINTRUST_PROVIDER_CLIENT_UNLOAD       Unload;
    LPWINTRUST_PROVIDER_VERIFY_TRUST        VerifyTrust;
    LPWINTRUST_PROVIDER_SUBMIT_CERTIFICATE  SubmitCertificate;
    
} WINTRUST_PROVIDER_CLIENT_SERVICES, *LPWINTRUST_PROVIDER_CLIENT_SERVICES;

typedef struct _WINTRUST_PROVIDER_CLIENT_INFO {
    DWORD                                   dwRevision;
    LPWINTRUST_PROVIDER_CLIENT_SERVICES     lpServices;
    DWORD                                   dwActionIdCount;
    GUID *                                  lpActionIdArray;
} WINTRUST_PROVIDER_CLIENT_INFO, *LPWINTRUST_PROVIDER_CLIENT_INFO;

typedef struct _LOADED_PROVIDER_V1 {

    struct _LOADED_PROVIDER_V1      *Next;
    struct _LOADED_PROVIDER_V1      *Prev;
    HANDLE                          ModuleHandle;
    LPTSTR                          ModuleName;
    LPTSTR                          SubKeyName;
    LPWINTRUST_PROVIDER_CLIENT_INFO ClientInfo;
    DWORD                           RefCount;
    DWORD                           ProviderInitialized;                            

} LOADED_PROVIDER_V1, *PLOADED_PROVIDER_V1;


#define PROVIDER_INITIALIZATION_SUCCESS        (1)
#define PROVIDER_INITIALIZATION_IN_PROGRESS    (2) 
#define PROVIDER_INITIALIZATION_FAILED         (3)

PLOADED_PROVIDER_V1 WinTrustFindActionID(IN GUID * dwActionID);
PLOADED_PROVIDER_V1 Version1_RegLoadProvider(HKEY hKey, LPTSTR KeyName, GUID *ActionID);
PLOADED_PROVIDER_V1 Version1_LoadProvider(GUID *pgActionID);
PLOADED_PROVIDER_V1 Version1_TestProviderForAction(HKEY hKey, LPTSTR KeyName, GUID * ActionID);
void Version1_UnloadProvider(PLOADED_PROVIDER_V1 Provider);

LONG Version1_WinVerifyTrust(HWND hwnd, GUID *ActionID, LPVOID ActionData)

{
    PLOADED_PROVIDER_V1 Provider;
    HRESULT rc;


    if (!(Provider = Version1_LoadProvider(ActionID)))
    {
        return( TRUST_E_PROVIDER_UNKNOWN );
    }

    rc = (*Provider->ClientInfo->lpServices->VerifyTrust)( hwnd,                             
                                                           ActionID,                         
                                                           ActionData                        
                                                           );

    Version1_UnloadProvider(Provider);

    return( rc );
}


PLOADED_PROVIDER_V1 Version1_LoadProvider(GUID *pgActionID)
{
    HKEY    hKey;             // Handle to the base of the provider information.
    HKEY    hSubKey;          // Handle to the provider currently being examined.
    LONG    Result;           // Returned by registry API.
    DWORD   cSubKeys;         // Number of providers under the root key.
    DWORD   cbMaxSubKeyLen;   // Maximum provider name length.
    ULONG   i;              // Indicies for iterating through providers and action IDs.
    LPTSTR  SubKeyName;       // Points to the name of the current provider.
    GUID *  ActionIds;
    PLOADED_PROVIDER_V1 FoundProvider = NULL;

    //
    // Open the registry and get a list of installed trust providers
    //

    Result = RegOpenKeyEx(
                 REGISTRY_ROOT,
                 REGISTRY_TRUSTPROVIDERS,
                 0L,
                 GENERIC_READ,
                 &hKey
                 );

    if (Result != ERROR_SUCCESS) {
        return( NULL );
    }

    //
    // Find out how many subkeys there are.
    //

    Result = RegQueryInfoKey (  hKey,               // handle of key to query
                                NULL,               // address of buffer for class string
                                NULL,               // address of size of class string buffer
                                NULL,               // reserved
                                &cSubKeys,          // address of buffer for number of subkeys
                                &cbMaxSubKeyLen,    // address of buffer for longest subkey name length
                                NULL,               // address of buffer for longest class string length
                                NULL,               // address of buffer for number of value entries
                                NULL,               // address of buffer for longest value name length
                                NULL,               // address of buffer for longest value data length
                                NULL,               // address of buffer for security descriptor length
                                NULL                // address of buffer for last write time
                                );

    if (ERROR_SUCCESS != Result) {
        RegCloseKey( hKey );
        return( NULL );
    }

    //
    // Iterate through the subkeys, looking for ones with hint information.
    //

    cbMaxSubKeyLen += sizeof( WCHAR );

    SubKeyName = new char[cbMaxSubKeyLen + 1];

    if (NULL == SubKeyName) {
        RegCloseKey( hKey );
        return(NULL);
    }

    for (i=0; i<cSubKeys; i++) {

        DWORD KeyNameLength;

        KeyNameLength = cbMaxSubKeyLen;

        Result = RegEnumKeyEx( hKey,               // handle of key to enumerate
                               i,                  // index of subkey to enumerate
                               SubKeyName,         // address of buffer for subkey name
                               &KeyNameLength,     // address for size of subkey buffer
                               NULL,               // reserved
                               NULL,               // address of buffer for class string
                               NULL,               // address for size of class buffer
                               NULL                // address for time key last written to
                               );

        //
        // Not much to do if this fails, try enumerating the rest of them and see
        // what happens.
        //

        if (Result != ERROR_SUCCESS) {
            continue;
        }

        Result = RegOpenKeyEx(
                     hKey,
                     SubKeyName,
                     0L,
                     GENERIC_READ | MAXIMUM_ALLOWED,
                     &hSubKey
                     );

        if (ERROR_SUCCESS != Result) 
        {
            continue;
        }

        FoundProvider = Version1_TestProviderForAction( hSubKey, SubKeyName, pgActionID );

        RegCloseKey( hSubKey );

        if (NULL != FoundProvider) 
        {
            
            //
            // Got one.  Clean up and return.
            //

            delete SubKeyName;
            RegCloseKey( hKey );
            return( FoundProvider );
        }

        continue;
    }

    delete SubKeyName;
    RegCloseKey( hKey );
    return( NULL );
}

WINTRUST_CLIENT_TP_INFO WinTrustClientTPInfo = {
                            WIN_TRUST_REVISION_1_0,
                            NULL
                            };

PLOADED_PROVIDER_V1 Version1_TestProviderForAction(HKEY hKey, LPTSTR KeyName, GUID * ActionID)
{
    PLOADED_PROVIDER_V1 Provider;
    LPWINTRUST_PROVIDER_CLIENT_INFO ClientInfo;
    GUID * ActionIds;
    DWORD i;

    Provider = Version1_RegLoadProvider( hKey, KeyName, ActionID);

    if (NULL == Provider) {
        return( NULL );
    }

    ClientInfo = Provider->ClientInfo;

    ActionIds = ClientInfo->lpActionIdArray;

    for (i=0; i<ClientInfo->dwActionIdCount; i++) {

        if (IsEqualActionID(ActionID, &ActionIds[i])) {
            return( Provider );
        }
    }

    return(NULL);
}

PLOADED_PROVIDER_V1 Version1_RegLoadProvider(HKEY hKey, LPTSTR KeyName, GUID *ActionID)
{
    LPTSTR ModuleName                           = NULL;
    HINSTANCE LibraryHandle                     = NULL;
    LPWINTRUST_PROVIDER_CLIENT_INFO ClientInfo  = NULL;
    PLOADED_PROVIDER_V1 Provider                   = NULL;
    PLOADED_PROVIDER_V1 FoundProvider              = NULL;
    LPWSTR ProviderName                         = NULL;
    LPTSTR SubKeyName                           = NULL;

    GUID    gBuffer[10];       // Assume no more than 10 action ids in a provider
    DWORD Type;
    DWORD cbData = 0;
    LONG Result;
    LPWINTRUST_PROVIDER_CLIENT_INITIALIZE ProcAddr;
    BOOL Referenced = FALSE;
    DWORD size;
    BOOL Inited;


    //
    //  get the guids
    //
    cbData = sizeof(GUID) * 10;
    Result = RegQueryValueEx(   hKey,    // handle of key to query
                                TEXT("$ActionIDs"),
                                NULL,       // reserved
                                &Type, // address of buffer for value type
                                (BYTE *)&gBuffer[0],
                                &cbData     // address of data buffer size
                                );

    if (Result != ERROR_SUCCESS)
    {
        return(NULL);
    }

    //
    //  check the guids
    //
    Inited = FALSE;
    for (int j = 0; j < (int)(cbData / sizeof(GUID)); j++) 
    {
        if (memcmp(&gBuffer[j], ActionID, sizeof(GUID)) == 0)
        {
            Inited = TRUE;
            break;
        }
    }

    if (!(Inited))
    {
        return(NULL);
    }


    //
    // Extract the dll name from the $DLL value
    //

    Result = RegQueryValueEx( hKey,           // handle of key to query
                              TEXT("$DLL"),   // address of name of value to query
                              NULL,           // reserved
                              &Type,          // address of buffer for value type
                              NULL,           // address of data buffer
                              &cbData         // address of data buffer size
                              );

//    if (ERROR_MORE_DATA != Result) {
//        goto error_cleanup;
//    }

    if (ERROR_SUCCESS != Result) {
        goto error_cleanup;
    }

    cbData += sizeof( TCHAR );

    ModuleName = new char[cbData];

    if (NULL == ModuleName) {
        goto error_cleanup;
    }

    ModuleName[cbData - 1] = TEXT('\0');

    Result = RegQueryValueEx( hKey,           // handle of key to query
                              TEXT("$DLL"),   // address of name of value to query
                              NULL,           // reserved
                              &Type,          // address of buffer for value type
                              (LPBYTE)ModuleName,   // address of data buffer
                              &cbData         // address of data buffer size
                              );

    if (ERROR_SUCCESS != Result) {
        goto error_cleanup;
    }

    //
    // Expand environment strings if necessary
    //

    if (Type == REG_EXPAND_SZ) {

        DWORD ExpandedLength = 0;
        LPTSTR ExpandedModuleName = NULL;

        ExpandedLength = ExpandEnvironmentStrings( ModuleName, NULL, 0 );

        if (0 == ExpandedLength) {
            goto error_cleanup;
        }

        ExpandedModuleName = new char[ExpandedLength];

        if (NULL == ExpandedModuleName) {
            goto error_cleanup;
        }

        ExpandedLength = ExpandEnvironmentStrings( ModuleName, ExpandedModuleName, ExpandedLength );

        if (0 == ExpandedLength) {
            delete ExpandedModuleName;
            goto error_cleanup;
        }

        //
        // Free the old module name, use the new one
        //

        delete ModuleName;

        ModuleName = ExpandedModuleName;
    }

    size = (lstrlen( KeyName ) + 1) * sizeof( WCHAR );

    ProviderName = new WCHAR[size / sizeof(WCHAR)];

    if (NULL == ProviderName) {
        goto error_cleanup;
    }


#ifdef UNICODE

    //
    // If we've been compiled as unicode, the KeyName we got from
    // the registry consists of WCHARs, so we can just copy it into
    // the Name buffer.
    //

    lstrcpy( ProviderName, KeyName );

#else

    //
    // If we've been compiled as ANSI, then KeyName is an ANSI string,
    // and we need to convert it to WCHARs.
    //

    MultiByteToWideChar ( CP_ACP, 0, KeyName, -1, ProviderName, size );

#endif // !UNICODE

    //
    // ModuleName now contains the module name, attempt to load it
    // and ask it to initialize itself.
    //

    LibraryHandle = LoadLibrary( (LPTSTR)ModuleName );

    if (NULL == LibraryHandle) {
        DWORD Error;

        Error = GetLastError();

        goto error_cleanup;
    }

    ProcAddr = (LPWINTRUST_PROVIDER_CLIENT_INITIALIZE) GetProcAddress( LibraryHandle, (LPCSTR)"WinTrustProviderClientInitialize");

    if (NULL == ProcAddr) {
        goto error_cleanup;
    }

    SubKeyName = new char[(lstrlen(KeyName) + 1) * sizeof(TCHAR)];

    if (NULL == SubKeyName) {
        goto error_cleanup;
    }

    lstrcpy( SubKeyName, KeyName );

    Provider = new LOADED_PROVIDER_V1;

    if (NULL == Provider) {
        delete SubKeyName;
        goto error_cleanup;
    }

    //
    // Ready to call init routine.
    //

    Provider->RefCount = 1;
    Provider->ProviderInitialized = PROVIDER_INITIALIZATION_IN_PROGRESS;

    //
    // Set the subkey name so anyone else looking for this provider will
    // find this one and wait.
    //
    // Note that we don't want to use the ProviderName as will be passed into
    // the init routine here, because we've forced that to WCHARs regardless
    // of whether we're ANSI or Unicode, and we want this string to reflect
    // the base system for efficiency.
    //

    Provider->SubKeyName = SubKeyName;

    Provider->Next = NULL;
    Provider->Prev = NULL;

    Inited = (*ProcAddr)( WIN_TRUST_REVISION_1_0, &WinTrustClientTPInfo, ProviderName, (void **)&ClientInfo );

    if (TRUE != Inited) {

        Provider->ProviderInitialized = PROVIDER_INITIALIZATION_FAILED;

        //
        // We could release the lock now, because we're either going to
        // do nothing to this provider, or we've removed it from
        // the list and no one else can get to it.
        //

        goto error_cleanup;
    }

    //
    // Since we have a write lock, it doesn't matter what order we
    // do this in, since there are no readers.  Just be sure to signal
    // the event under the write lock.
    //

    Provider->ProviderInitialized = PROVIDER_INITIALIZATION_SUCCESS;
    Provider->ModuleHandle = LibraryHandle;
    Provider->ModuleName = ModuleName;
    Provider->ClientInfo = ClientInfo;

    return( Provider );

error_cleanup:

    if (NULL != LibraryHandle) {
        FreeLibrary( LibraryHandle );
    }

    if (NULL != ModuleName) {
        delete ModuleName;
    }

    if (NULL != ProviderName) {
        delete ProviderName;
    }

    if (NULL != Provider)
    {
        delete Provider;
    }

    return( NULL );
}

void Version1_UnloadProvider(PLOADED_PROVIDER_V1 Provider)
{
    if (Provider)
    {
        if (Provider->ModuleHandle)
        {
            FreeLibrary((HINSTANCE)Provider->ModuleHandle);
        }
        if (Provider->ModuleName)
        {
            delete Provider->ModuleName;
        }
    }
    
    delete Provider;
}



