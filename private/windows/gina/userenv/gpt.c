//*************************************************************
//
//  Group Policy Support
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997-1998
//  All rights reserved
//
//*************************************************************

#include "uenv.h"

#define GPO_LPARAM_FLAG_DELETE         0x00000001


//
// Structures
//

typedef struct _GPEXT {
    LPTSTR         lpDisplayName;            // Display name
    LPTSTR         lpKeyName;                // Extension name
    LPTSTR         lpDllName;                // Dll name
    LPSTR          lpFunctionName;           // Entry point name
    HANDLE         hInstance;                // Handle to dll
    PFNPROCESSGROUPPOLICY  pEntryPoint;      // Entry point for ProcessGPO
    DWORD          dwNoMachPolicy;           // Mach policy setting
    DWORD          dwNoUserPolicy;           // User policy setting
    DWORD          dwNoSlowLink;             // Slow link setting
    DWORD          dwNoBackgroundPolicy;     // Background policy setting
    DWORD          dwNoGPOChanges;           // GPO changes setting
    DWORD          dwUserLocalSetting;       // Per user per machine setting
    DWORD          dwRequireRegistry;        // RequireSuccReg setting
    DWORD          dwEnableAsynch;           // Enable asynchronous processing setting
    DWORD          dwLinkTransition;         // Link speed transition setting
    DWORD          dwMaxChangesInterval;     // Max interval (mins) for which NoGpoChanges is adhered to
    BOOL           bRegistryExt;             // Is this the psuedo reg extension ?
    BOOL           bSkipped;                 // Should processing be skipped for this extension ?
    BOOL           bHistoryProcessing;       // Is processing needed to clean up cached Gpos ?
    DWORD          dwSlowLinkPrev;           // Slow link when policy applied previously ?
    GUID           guid;                     // Guid of extension
    struct _GPEXT *pNext;                    // Singly linked list pointer
} GPEXT, *LPGPEXT;



typedef struct _EXTLIST {
    GUID             guid;                   // Extension guid
    struct _EXTLIST *pNext;                  // Singly linked list pointer
} EXTLIST, *LPEXTLIST;



typedef struct _EXTFILTERLIST {
    PGROUP_POLICY_OBJECT   lpGPO;            // GPO
    LPEXTLIST              lpExtList;        // List of extension guids that apply to lpGPO
    struct _EXTFILTERLIST *pNext;            // Singly linked list pointer
} EXTFILTERLIST, *LPEXTFILTERLIST;



typedef struct _GPOINFO {
    DWORD                    dwFlags;
    INT                      iMachineRole;
    HANDLE                   hToken;
    HANDLE                   hEvent;
    HKEY                     hKeyRoot;
    BOOL                     bXferToExtList;     // Has the ownership been transferred from lpGPOList to lpExtFilterList ?
    LPEXTFILTERLIST          lpExtFilterList;    // List of extensions to be filtered, cardinality is same as GetGPOList's list
    PGROUP_POLICY_OBJECT     lpGPOList;          // Filtered GPO List, can vary from one extension to next
    LPTSTR                   lpwszSidUser;       // Sid of user in string form
    HANDLE                   hTriggerEvent;
    HANDLE                   hNotifyEvent;
    HANDLE                   hCritSection;
    LPGPEXT                  lpExtensions;
    BOOL                     bMemChanged;          // Has security group membership has changed ?
    BOOL                     bUserLocalMemChanged; // Has membership changed on per user local basis ?
    BOOL                     bSidChanged;          // Has the Sid changed since the last policy run?
    PFNSTATUSMESSAGECALLBACK pStatusCallback;
} GPOINFO, *LPGPOINFO;


typedef struct _GPINFOHANDLE
{
    LPGPOINFO pGPOInfo;
} GPINFOHANDLE, *LPGPINFOHANDLE;


typedef struct _DNENTRY {
    LPTSTR                pwszDN;            // Distinguished name
    union {
        PGROUP_POLICY_OBJECT  pDeferredGPO;  // GPO corresponding to this DN
        struct _DNENTRY *     pDeferredOU;   // OU correspdonding to this DN
    };
    PLDAPMessage          pOUMsg;            // Message for evaluating deferred OU
    GPO_LINK              gpoLink;           // Type of GPO
    struct _DNENTRY *     pNext;             // Singly linked list pointer
} DNENTRY;


typedef struct _LDAPQUERY {
    LPTSTR              pwszDomain;          // Domain of subtree search
    LPTSTR              pwszFilter;          // Ldap filter for search
    DWORD               cbAllocLen;          // Allocated size of pwszFilter in bytes
    DWORD               cbLen;               // Size of pwszFilter currently used in bytes
    PLDAP               pLdapHandle;         // Ldap bind handle
    BOOL                bOwnLdapHandle;      // Does this struct own pLdapHandle ?
    PLDAPMessage        pMessage;            // Ldap message handle
    DNENTRY *           pDnEntry;            // Distinguished name entry
    struct _LDAPQUERY * pNext;               // Singly linked list pointer
} LDAPQUERY;


//
// Verison number for the registry file format
//

#define REGISTRY_FILE_VERSION       1


//
// File signature
//

#define REGFILE_SIGNATURE  0x67655250


//
// Default refresh rate (minutes)
//
// Client machines will refresh every 90 minutes
// Domain controllers will refresh every 5 minutes
//

#define GP_DEFAULT_REFRESH_RATE      90
#define GP_DEFAULT_REFRESH_RATE_DC    5


//
// Default refresh rate max offset
//
// To prevent many clients from querying policy at the exact same
// time, a random amount is added to the refresh rate.  In the
// default case, a number between 0 and 30 will be added to
// 180 to determine when the next background refresh will occur
//

#define GP_DEFAULT_REFRESH_RATE_OFFSET    30
#define GP_DEFAULT_REFRESH_RATE_OFFSET_DC  0


//
// Max keyname size
//

#define MAX_KEYNAME_SIZE         2048
#define MAX_VALUENAME_SIZE        512


//
// Max time to wait for the network to start (in ms)
//

#define MAX_WAIT_TIME            120000


//
// Extension registry path
//

#define GP_EXTENSIONS   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions")

//
// Path for extension preference policies
//

#define GP_EXTENSIONS_POLICIES   TEXT("Software\\Policies\\Microsoft\\Windows\\Group Policy\\%s")

//
// Group Policy Object option flags
//
// Note, this was taken from sdk\inc\gpedit.h
//

#define GPO_OPTION_DISABLE_USER     0x00000001  // The user portion of this GPO is disabled
#define GPO_OPTION_DISABLE_MACHINE  0x00000002  // The machine portion of this GPO is disabled


//
// Generic access mappings
//

const
ACCESS_MASK
GENERIC_READ_MAPPING =    ((STANDARD_RIGHTS_READ)     | \
                           (ACTRL_DS_LIST)            | \
                           (ACTRL_DS_READ_PROP)       | \
                           (ACTRL_DS_LIST_OBJECT));

const
ACCESS_MASK
GENERIC_EXECUTE_MAPPING = ((STANDARD_RIGHTS_EXECUTE)  | \
                           (ACTRL_DS_LIST));

const
ACCESS_MASK
GENERIC_WRITE_MAPPING =   ((STANDARD_RIGHTS_WRITE)    | \
                           (ACTRL_DS_SELF)            | \
                           (ACTRL_DS_WRITE_PROP));

const
ACCESS_MASK
GENERIC_ALL_MAPPING =     ((STANDARD_RIGHTS_REQUIRED) | \
                           (ACTRL_DS_CREATE_CHILD)    | \
                           (ACTRL_DS_DELETE_CHILD)    | \
                           (ACTRL_DS_DELETE_TREE)     | \
                           (ACTRL_DS_READ_PROP)       | \
                           (ACTRL_DS_WRITE_PROP)      | \
                           (ACTRL_DS_LIST)            | \
                           (ACTRL_DS_LIST_OBJECT)     | \
                           (ACTRL_DS_CONTROL_ACCESS)  | \
                           (ACTRL_DS_SELF));



//
// DS Object class types
//

TCHAR szDSClassAny[]    = TEXT("(objectClass=*)");
TCHAR szDSClassGPO[]    = TEXT("groupPolicyContainer");
TCHAR szDSClassSite[]   = TEXT("site");
TCHAR szDSClassDomain[] = TEXT("domainDNS");
TCHAR szDSClassOU[]     = TEXT("organizationalUnit");
TCHAR szObjectClass[]   = TEXT("objectClass");

//
// Extension name properties
//
#define GPO_MACHEXTENSION_NAMES   L"gPCMachineExtensionNames"
#define GPO_USEREXTENSION_NAMES   L"gPCUserExtensionNames"
#define GPO_FUNCTIONALITY_VERSION L"gPCFunctionalityVersion"


TCHAR wszKerberos[] = TEXT("Kerberos");

#define POLICY_GUID_PATH            TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\PolicyGuid")

//
// Global flags for Gpo shutdown processing. These are accessed outside
// the lock because its value is either 0 or 1. Even if there is a race,
// all it means is that shutdown will start one iteration later.
//

BOOL g_bStopMachGPOProcessing = FALSE;
BOOL g_bStopUserGPOProcessing = FALSE;

//
// Critical section for handling concurrent, asynchronous completion
//

CRITICAL_SECTION g_GPOCS;

//
// Global pointers for maintaining asynchronous completion context
//

LPGPINFOHANDLE g_pMachGPInfo = 0;
LPGPINFOHANDLE g_pUserGPInfo = 0;


//
// Status UI critical section, callback, and proto-types
//

CRITICAL_SECTION g_StatusCallbackCS;
PFNSTATUSMESSAGECALLBACK g_pStatusMessageCallback = NULL;
DWORD UserPolicyCallback (BOOL bVerbose, LPWSTR lpMessage);
DWORD MachinePolicyCallback (BOOL bVerbose, LPWSTR lpMessage);


//
// Function proto-types
//

DWORD WINAPI GPOThread (LPGPOINFO lpGPOInfo);
BOOL ProcessGPOs (LPGPOINFO lpGPOInfo);
DWORD WINAPI PolicyChangedThread (BOOL bMachine);
BOOL ResetPolicies (LPGPOINFO lpGPOInfo, LPTSTR lpArchive);
BOOL SetupGPOFilter (LPGPOINFO lpGPOInfo );
void FilterGPOs( LPGPEXT lpExt, LPGPOINFO lpGPOInfo );
void FreeLists( LPGPOINFO lpGPOInfo );
void FreeExtList(LPEXTLIST pExtList );
BOOL CheckGPOs (LPGPEXT lpExt, LPGPOINFO lpGPOInfo, DWORD dwTime, BOOL *pbProcessGPOs,
                BOOL *pbNoChanges, PGROUP_POLICY_OBJECT *ppDeletedGPOList);
BOOL CheckForChangedSid( LPGPOINFO lpGPOInfo );
BOOL CheckForSkippedExtensions( LPGPOINFO lpGPOInfo );
BOOL ReadGPExtensions( LPGPOINFO lpGPOInfo );
BOOL LoadGPExtension (LPGPEXT lpExt);
BOOL UnloadGPExtensions (LPGPOINFO lpGPOInfo);
BOOL WriteStatus( TCHAR *lpExtName, LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser, DWORD dwStatus, DWORD dwTime, DWORD dwSlowLink );
BOOL ReadStatus( TCHAR *lpExtName, LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser, DWORD *pdwStatus, DWORD *pdwTime, DWORD *pdwSlowlink );
DWORD ProcessGPOList (LPGPEXT lpExt, LPGPOINFO lpGPOInfo, PGROUP_POLICY_OBJECT pDeletedGPOList,
                     PGROUP_POLICY_OBJECT pChangedGPOList, BOOL bNoChanges,
                     ASYNCCOMPLETIONHANDLE pAsyncHandle );
BOOL ProcessGPORegistryPolicy (LPGPOINFO lpGPOInfo, PGROUP_POLICY_OBJECT pChangedGPOList);
BOOL SaveGPOList (TCHAR *pszExtName, LPGPOINFO lpGPOInfo,
                  HKEY hKeyRootMach, LPTSTR lpwszSidUser, BOOL bShadow, PGROUP_POLICY_OBJECT lpGPOList);
BOOL AddGPO (PGROUP_POLICY_OBJECT * lpGPOList, DWORD dwOptions, DWORD dwVersion,
             LPTSTR lpDSPath, LPTSTR lpFileSysPath, LPTSTR lpDisplayName,
             LPTSTR lpGPOName, LPTSTR lpExtensions, GPO_LINK GPOLink, LPTSTR lpLink, LPARAM lParam, BOOL bFront,
             BOOL bBlock, BOOL bVerbose);
BOOL RefreshDisplay (LPGPOINFO lpGPOInfo);
DWORD IsSlowLink (HKEY hKeyRoot, LPTSTR lpDCAddress, BOOL *bSlow);
BOOL GetGPOInfo (DWORD dwFlags, LPTSTR lpHostName, LPTSTR lpDNName,
                 LPCTSTR lpComputerName, PGROUP_POLICY_OBJECT *lpGPOList,
                 PNETAPI32_API pNetAPI32, BOOL bMachineTokenOk);
void WINAPI ShutdownGPOProcessing( BOOL bMachine );
void DebugPrintGPOList( LPGPOINFO lpGPOInfo );

typedef BOOL (*PFNREGFILECALLBACK)(LPGPOINFO lpGPOInfo, LPTSTR lpKeyName,
                                   LPTSTR lpValueName, DWORD dwType,
                                   DWORD dwDataLength, LPBYTE lpData);
BOOL ParseRegistryFile (LPGPOINFO lpGPOInfo, LPTSTR lpRegistry,
                        PFNREGFILECALLBACK pfnRegFileCallback,
                        HANDLE hArchive);
BOOL ExtensionHasPerUserLocalSetting( LPTSTR pszExtension, HKEY hKeyRoot );
void CheckGroupMembership( LPGPOINFO lpGPOInfo, HANDLE hToken, BOOL *pbMemChanged, BOOL *pbUserLocalMemChanged );
BOOL ReadMembershipList( LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser, PTOKEN_GROUPS pGroups );
void SaveMembershipList( LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser, PTOKEN_GROUPS pGroups );
BOOL GroupInList( LPTSTR lpSid, PTOKEN_GROUPS pGroups );
DWORD GetCurTime();
DWORD GetDomainControllerInfo(  PNETAPI32_API pNetAPI32, LPTSTR szDomainName,
                                ULONG ulFlags, HKEY hKeyRoot, PDOMAIN_CONTROLLER_INFO* ppInfo,
                                BOOL* pfSlow );
PLDAP GetMachineDomainDS( PNETAPI32_API pNetApi32, PLDAP_API pLdapApi );
HANDLE GetMachineToken();
NTSTATUS CallDFS(LPWSTR lpDomainName, LPWSTR lpDCName);

//*************************************************************
//
//  ApplyGroupPolicy()
//
//  Purpose:    Processes group policy
//
//  Parameters: dwFlags         -  Processing flags
//              hToken          -  Token (user or machine)
//              hEvent          -  Termination event for background thread
//              hKeyRoot        -  Root registry key (HKCU or HKLM)
//              pStatusCallback -  Callback function for display status messages
//
//  Return:     Thread handle if successful
//              NULL if an error occurs
//
//*************************************************************

HANDLE WINAPI ApplyGroupPolicy (DWORD dwFlags, HANDLE hToken, HANDLE hEvent,
                                HKEY hKeyRoot, PFNSTATUSMESSAGECALLBACK pStatusCallback)
{
    HANDLE hThread = NULL;
    DWORD dwThreadID;
    LPGPOINFO lpGPOInfo = NULL;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ApplyGroupPolicy: Entering. Flags = %x"), dwFlags));


    //
    // Save the status UI callback function
    //

    EnterCriticalSection (&g_StatusCallbackCS);
    g_pStatusMessageCallback = pStatusCallback;
    LeaveCriticalSection (&g_StatusCallbackCS);


    //
    // Allocate a GPOInfo structure to work with.
    //

    lpGPOInfo = (LPGPOINFO) LocalAlloc (LPTR, sizeof(GPOINFO));

    if (!lpGPOInfo) {
        DebugMsg((DM_WARNING, TEXT("ApplyGroupPolicy: Failed to alloc lpGPOInfo (%d)."),
                 GetLastError()));
        LogEvent (TRUE, IDS_FAILED_ALLOCATION, GetLastError());
        goto Exit;
    }


    lpGPOInfo->dwFlags = dwFlags;
    lpGPOInfo->hToken = hToken;
    lpGPOInfo->hEvent = hEvent;
    lpGPOInfo->hKeyRoot = hKeyRoot;

    if (dwFlags & GP_MACHINE) {
        lpGPOInfo->pStatusCallback = MachinePolicyCallback;
    } else {
        lpGPOInfo->pStatusCallback = UserPolicyCallback;
    }


    //
    // Create an event so other processes can trigger policy
    // to be applied immediately
    //

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

    SetSecurityDescriptorDacl (
                    &sd,
                    TRUE,                           // Dacl present
                    NULL,                           // NULL Dacl
                    FALSE                           // Not defaulted
                    );

    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;
    sa.nLength = sizeof(sa);

    lpGPOInfo->hTriggerEvent = CreateEvent (&sa, FALSE, FALSE,
                                            (dwFlags & GP_MACHINE) ?
                                            MACHINE_POLICY_REFRESH_EVENT : USER_POLICY_REFRESH_EVENT);


    //
    // Create the notification event
    //

    lpGPOInfo->hNotifyEvent = CreateEvent (&sa, TRUE, FALSE,
                                           (dwFlags & GP_MACHINE) ?
                                           MACHINE_POLICY_APPLIED_EVENT : USER_POLICY_APPLIED_EVENT);


    //
    // Initilialize shutdown gpo processing support
    //

    if ( dwFlags & GP_MACHINE )
        g_bStopMachGPOProcessing = FALSE;
    else
        g_bStopUserGPOProcessing = FALSE;

    //
    // Process the GPOs
    //

    ProcessGPOs(lpGPOInfo);

    //
    // If requested, create a background thread to keep updating
    // the profile from the gpos
    //

    if (lpGPOInfo->dwFlags & GP_BACKGROUND_REFRESH) {

        //
        // Create a thread which sleeps and processes GPOs
        //

        hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) GPOThread,
                                (LPVOID) lpGPOInfo, CREATE_SUSPENDED, &dwThreadID);

        if (!hThread) {
            DebugMsg((DM_WARNING, TEXT("ApplyGroupPolicy: Failed to create background thread (%d)."),
                     GetLastError()));
            goto Exit;
        }

        SetThreadPriority (hThread, THREAD_PRIORITY_IDLE);
        lpGPOInfo->pStatusCallback = NULL;
        ResumeThread (hThread);

        //
        // Reset the status UI callback function
        //

        EnterCriticalSection (&g_StatusCallbackCS);
        g_pStatusMessageCallback = NULL;
        LeaveCriticalSection (&g_StatusCallbackCS);

        DebugMsg((DM_VERBOSE, TEXT("ApplyGroupPolicy: Leaving successfully.")));

        return hThread;
    }

    DebugMsg((DM_VERBOSE, TEXT("ApplyGroupPolicy: Background refresh not requested.  Leaving successfully.")));
    hThread = (HANDLE) 1;

Exit:
    EnterCriticalSection( &g_GPOCS );
    if ( dwFlags & GP_MACHINE ) {

        if ( g_pMachGPInfo )
            LocalFree( g_pMachGPInfo );
        g_pMachGPInfo = 0;
    } else {
        if ( g_pUserGPInfo )
            LocalFree( g_pUserGPInfo );
        g_pUserGPInfo = 0;
    }
    LeaveCriticalSection( &g_GPOCS );

    if (lpGPOInfo) {

        if (lpGPOInfo->hTriggerEvent) {
            CloseHandle (lpGPOInfo->hTriggerEvent);
        }

        if (lpGPOInfo->hNotifyEvent) {
            CloseHandle (lpGPOInfo->hNotifyEvent);
        }

        if (lpGPOInfo->lpwszSidUser)
            DeleteSidString( lpGPOInfo->lpwszSidUser );

        LocalFree (lpGPOInfo);
    }

    //
    // Reset the status UI callback function
    //

    EnterCriticalSection (&g_StatusCallbackCS);
    g_pStatusMessageCallback = NULL;
    LeaveCriticalSection (&g_StatusCallbackCS);

    return hThread;
}

//*************************************************************
//
//  GPOThread()
//
//  Purpose:    Background thread for GPO processing.
//
//  Parameters: lpGPOInfo   - GPO info
//
//  Return:     0
//
//*************************************************************

DWORD WINAPI GPOThread (LPGPOINFO lpGPOInfo)
{
    HINSTANCE hInst;
    HKEY hKey;
    HANDLE hHandles[3] = {NULL, NULL, NULL};
    DWORD dwType, dwSize, dwResult;
    DWORD dwTimeout, dwOffset;
    BOOL bSetBkGndFlag;
    TCHAR szEventName[60];
    LARGE_INTEGER DueTime;


    hInst = LoadLibrary (TEXT("userenv.dll"));


    hHandles[0] = lpGPOInfo->hEvent;
    hHandles[1] = lpGPOInfo->hTriggerEvent;


    while (TRUE) {

        //
        // Initialize
        //

        if (lpGPOInfo->dwFlags & GP_MACHINE) {
            if (lpGPOInfo->iMachineRole == 3) {
                dwTimeout = GP_DEFAULT_REFRESH_RATE_DC;
                dwOffset = GP_DEFAULT_REFRESH_RATE_OFFSET_DC;
            } else {
                dwTimeout = GP_DEFAULT_REFRESH_RATE;
                dwOffset = GP_DEFAULT_REFRESH_RATE_OFFSET;
            }
        } else {
            dwTimeout = GP_DEFAULT_REFRESH_RATE;
            dwOffset = GP_DEFAULT_REFRESH_RATE_OFFSET;
        }


        //
        // Query for the refresh timer value and max offset
        //

        if (RegOpenKeyEx (lpGPOInfo->hKeyRoot,
                          SYSTEM_POLICIES_KEY,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {


            if ((lpGPOInfo->iMachineRole == 3) && (lpGPOInfo->dwFlags & GP_MACHINE)) {

                dwSize = sizeof(dwTimeout);
                RegQueryValueEx (hKey,
                                 TEXT("GroupPolicyRefreshTimeDC"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &dwTimeout,
                                 &dwSize);

                dwSize = sizeof(dwOffset);
                RegQueryValueEx (hKey,
                                 TEXT("GroupPolicyRefreshTimeOffsetDC"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &dwOffset,
                                 &dwSize);

            } else {

                dwSize = sizeof(dwTimeout);
                RegQueryValueEx (hKey,
                                 TEXT("GroupPolicyRefreshTime"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &dwTimeout,
                                 &dwSize);

                dwSize = sizeof(dwOffset);
                RegQueryValueEx (hKey,
                                 TEXT("GroupPolicyRefreshTimeOffset"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &dwOffset,
                                 &dwSize);
            }

            RegCloseKey (hKey);
        }


        //
        // Limit the timeout to once every 64800 minutes (45 days)
        //

        if (dwTimeout >= 64800) {
            dwTimeout = 64800;
        }


        //
        // Convert seconds to milliseconds
        //

        dwTimeout =  dwTimeout * 60 * 1000;


        //
        // Limit the offset to 1440 minutes (24 hours)
        //

        if (dwOffset >= 1440) {
            dwOffset = 1440;
        }


        //
        // Special case 0 milliseconds to be 7 seconds
        //

        if (dwTimeout == 0) {
            dwTimeout = 7000;

        } else {

            //
            // If there is an offset, pick a random number
            // from 0 to dwOffset and then add it to the timeout
            //

            if (dwOffset) {
                dwOffset = GetTickCount() % dwOffset;

                dwOffset = dwOffset * 60 * 1000;
                dwTimeout += dwOffset;
            }
        }


        //
        // Setup the timer
        //

        if (dwTimeout >= 60000) {
            DebugMsg((DM_VERBOSE, TEXT("GPOThread:  Next refresh will happen in %d minutes"),
                     ((dwTimeout / 1000) / 60)));
        } else {
            DebugMsg((DM_VERBOSE, TEXT("GPOThread:  Next refresh will happen in %d seconds"),
                     (dwTimeout / 1000)));
        }


        wsprintf (szEventName, TEXT("userenv: refresh timer for %d:%d"),
                  GetCurrentProcessId(), GetCurrentThreadId());
        hHandles[2] = CreateWaitableTimer (NULL, TRUE, szEventName);


        if (hHandles[2] == NULL) {
            DebugMsg((DM_WARNING, TEXT("GPOThread: CreateWaitableTimer failed with error %d"),
                     GetLastError()));
            LogEvent (TRUE, IDS_FAILED_TIMER, TEXT("CreateWaitableTimer"), GetLastError());
            break;
        }

        DueTime.QuadPart = UInt32x32To64(10000, dwTimeout);
        DueTime.QuadPart *= -1;

        if (!SetWaitableTimer (hHandles[2], &DueTime, 0, NULL, 0, FALSE)) {
            DebugMsg((DM_WARNING, TEXT("GPOThread: Failed to set timer with error %d"),
                     GetLastError()));
            LogEvent (TRUE, IDS_FAILED_TIMER, TEXT("SetWaitableTimer"), GetLastError());
            break;
        }


        dwResult = WaitForMultipleObjects (3, hHandles, FALSE, INFINITE);
        
        if ( (dwResult - WAIT_OBJECT_0) == 0 )
        {
            //
            // for machine policy thread, this is a shutdown.
            // for user policy thread, this is a logoff.
            //
            break;
        }
        else if ( dwResult == WAIT_FAILED )
        {
            LogEvent (TRUE, IDS_FAILED_TIMER, TEXT("WaitForMultipleObjects"), GetLastError());
            break;
        }


        //
        // Check if we should set the background flag.  We offer this
        // option for the test team's automation tests.  They need to
        // simulate logon / boot policy without actually logging on or
        // booting the machine.
        //

        bSetBkGndFlag = TRUE;

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                          WINLOGON_KEY,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(bSetBkGndFlag);
            RegQueryValueEx (hKey,
                             TEXT("SetGroupPolicyBackgroundFlag"),
                             NULL,
                             &dwType,
                             (LPBYTE) &bSetBkGndFlag,
                             &dwSize);

            RegCloseKey (hKey);
        }


        lpGPOInfo->dwFlags &= ~GP_REGPOLICY_CPANEL;
        lpGPOInfo->dwFlags &= ~GP_SLOW_LINK;
        lpGPOInfo->dwFlags &= ~GP_VERBOSE;
        lpGPOInfo->dwFlags &= ~GP_BACKGROUND_THREAD;


        //
        // Set the background thread flag so components known
        // when they are being called from the background thread
        // vs the main thread.
        //

        if (bSetBkGndFlag) {
            lpGPOInfo->dwFlags |= GP_BACKGROUND_THREAD;
        }

        ProcessGPOs(lpGPOInfo);

        CloseHandle (hHandles[2]);
        hHandles[2] = NULL;
    }



    //
    // Cleanup
    //

    if (hHandles[2]) {
        CloseHandle (hHandles[2]);
    }

    if (lpGPOInfo->hTriggerEvent) {
        CloseHandle (lpGPOInfo->hTriggerEvent);
    }

    if (lpGPOInfo->hNotifyEvent) {
        CloseHandle (lpGPOInfo->hNotifyEvent);
    }

    EnterCriticalSection( &g_GPOCS );

    if ( lpGPOInfo->dwFlags & GP_MACHINE ) {

       if ( g_pMachGPInfo )
           LocalFree( g_pMachGPInfo );

       g_pMachGPInfo = 0;
    } else {

        if ( g_pUserGPInfo )
            LocalFree( g_pUserGPInfo );

        g_pUserGPInfo = 0;
     }

    LeaveCriticalSection( &g_GPOCS );

    if (lpGPOInfo->lpwszSidUser)
        DeleteSidString( lpGPOInfo->lpwszSidUser );

    LocalFree (lpGPOInfo);

    FreeLibraryAndExitThread (hInst, 0);
    return 0;
}


//*************************************************************
//
//  GPOExceptionFilter()
//
//  Purpose:    Exception filter when procssing GPO extensions
//
//  Parameters: pExceptionPtrs - Pointer to exception pointer
//
//  Returns:    EXCEPTION_EXECUTE_HANDLER
//
//*************************************************************

LONG GPOExceptionFilter( PEXCEPTION_POINTERS pExceptionPtrs )
{
    PEXCEPTION_RECORD pExr = pExceptionPtrs->ExceptionRecord;
    PCONTEXT pCxr = pExceptionPtrs->ContextRecord;

    DebugMsg(( DM_WARNING, L"GPOExceptionFilter: Caught exception 0x%x, exr = 0x%x, cxr = 0x%x\n",
              pExr->ExceptionCode, pExr, pCxr ));

    DmAssert( ! L"Caught unhandled exception when processing group policy extension" );

    return EXCEPTION_EXECUTE_HANDLER;
}


//*************************************************************
//
//  ProcessGPOs()
//
//  Purpose:    Processes GPOs
//
//  Parameters: lpGPOInfo   -   GPO information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ProcessGPOs (LPGPOINFO lpGPOInfo)
{
    BOOL bRetVal = FALSE;
    DWORD dwThreadID;
    HANDLE hThread;
    DWORD dwType, dwSize, dwResult;
    HKEY hKey;
    BOOL bResult;
    PDOMAIN_CONTROLLER_INFO pDCI = NULL;
    LPTSTR lpDomain = NULL;
    LPTSTR lpName = NULL;
    LPTSTR lpDomainDN = NULL;
    LPTSTR lpComputerName;
    PGROUP_POLICY_OBJECT lpGPO = NULL;
    PGROUP_POLICY_OBJECT lpGPOTemp;
    BOOL bAllSkipped;
    LPGPEXT lpExt;
    LPGPINFOHANDLE pGPHandle = NULL;
    ASYNCCOMPLETIONHANDLE pAsyncHandle = 0;
    HANDLE hOldToken;
    UINT uExtensionCount = 0;
    PNETAPI32_API pNetAPI32;
    DWORD dwUserPolicyMode = 0;
    DWORD dwCurrentTime = 0;
    INT iRole;
    BOOL bSlow;

    //
    // Allow debugging level to be changed dynamically between
    // policy refreshes.
    //

    InitDebugSupport( FALSE );


    //
    // Debug spew
    //

    if (lpGPOInfo->dwFlags & GP_MACHINE) {
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:  Starting computer Group Policy processing...")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
    } else {
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs: Starting user Group Policy processing...")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
    }


    //
    // Check if we should be verbose to the event log
    //

    if (CheckForVerbosePolicy()) {
        lpGPOInfo->dwFlags |= GP_VERBOSE;
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs:  Verbose output to eventlog requested.")));
    }

    if (lpGPOInfo->dwFlags & GP_VERBOSE) {
        if (lpGPOInfo->dwFlags & GP_MACHINE) {
            LogEvent (FALSE, IDS_START_MACHINE_POLICY);
        } else {
            LogEvent (FALSE, IDS_START_USER_POLICY);
        }
    }


    //
    // Claim the critical section
    //

    lpGPOInfo->hCritSection = EnterCriticalPolicySection((lpGPOInfo->dwFlags & GP_MACHINE));

    if (!lpGPOInfo->hCritSection) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to claim the policy critical section with %d."),
                 GetLastError()));
        LogEvent (TRUE, IDS_FAILED_CRITICAL_SECTION, GetLastError());
        goto Exit;
    }


    //
    // Set the security on the Group Policy registry key
    //

    if (!MakeRegKeySecure((lpGPOInfo->dwFlags & GP_MACHINE) ? NULL : lpGPOInfo->hToken,
                          lpGPOInfo->hKeyRoot,
                          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy"))) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to secure reg key.")));
    }


    //
    // Load netapi32
    //

    pNetAPI32 = LoadNetAPI32();

    if (!pNetAPI32) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs:  Failed to load netapi32 with %d."),
                 GetLastError()));
        LogEvent (TRUE, IDS_FAILED_NETAPI32, GetLastError());
        goto Exit;
    }



    //
    // Get the role of this computer
    //

    if (!GetMachineRole (&iRole)) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs:  Failed to get the role of the computer.")));
        LogEvent (TRUE, IDS_FAILED_ROLE);
        goto Exit;
    }

    lpGPOInfo->iMachineRole = iRole;

    DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs:  Machine role is %d."), iRole));

    if (lpGPOInfo->dwFlags & GP_VERBOSE) {

        switch (iRole) {
            case 0:
                LogEvent (FALSE, IDS_ROLE_STANDALONE);
                break;

            case 1:
                LogEvent (FALSE, IDS_ROLE_DOWNLEVEL_DOMAIN);
                break;

            default:
                LogEvent (FALSE, IDS_ROLE_DS_DOMAIN);
                break;
        }
    }


    //
    // If we are going to apply policy from the DS, query for the user's
    // DN name, domain name, etc
    //

    if (lpGPOInfo->dwFlags & GP_APPLY_DS_POLICY) {


        //
        // Query for the user's domain name
        //

        if (!ImpersonateUser(lpGPOInfo->hToken, &hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to impersonate user")));
            goto Exit;
        }

        lpDomain = MyGetDomainName ();

        RevertToUser(&hOldToken);

        if (!lpDomain) {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: MyGetDomainName failed with %d."),
                     dwResult));
            goto Exit;
        }


        //
        // Query for the DS server name
        //

        dwResult = GetDomainControllerInfo( pNetAPI32,
                                            lpDomain,
                                            DS_DIRECTORY_SERVICE_REQUIRED | DS_IS_FLAT_NAME
                                            | DS_RETURN_DNS_NAME |
                                            ((lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD) ? DS_BACKGROUND_ONLY : 0),
                                            lpGPOInfo->hKeyRoot,
                                            &pDCI,
                                            &bSlow );

        if (dwResult != ERROR_SUCCESS) {

            if ((dwResult == ERROR_BAD_NETPATH) ||
                (dwResult == ERROR_NETWORK_UNREACHABLE) ||
                (dwResult == ERROR_NO_SUCH_DOMAIN)) {

                //
                // couldn't find DC. Nothing more we can do, abort
                //
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: The DC for domain %s is not available"),
                         lpDomain));

            } else {

                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: DSGetDCName failed with %d."),
                         dwResult));
                LogEvent (TRUE, IDS_FAILED_DSNAME, dwResult);
            }

            goto Exit;
        } else {

            //
            // success, slow link?
            //
            if (bSlow) {
                lpGPOInfo->dwFlags |= GP_SLOW_LINK;
                if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                    LogEvent (FALSE, IDS_SLOWLINK);
                }
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: A slow link was detected.")));
            }
        }


        //
        // Get the user's DN name
        //

        if (!ImpersonateUser(lpGPOInfo->hToken, &hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to impersonate user")));
            goto Exit;
        }

        lpName = MyGetUserName (NameFullyQualifiedDN);

        RevertToUser(&hOldToken);

        if (!lpName) {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: MyGetUserName failed with %d."),
                     dwResult));
            LogEvent (TRUE, IDS_FAILED_USERNAME, dwResult);
            goto Exit;
        }


        lpDomainDN = pDCI->DomainName;

        if (lpGPOInfo->dwFlags & GP_VERBOSE) {
            LogEvent (FALSE, IDS_USERNAME, lpName);
            LogEvent (FALSE, IDS_DOMAINNAME, lpDomain);
            LogEvent (FALSE, IDS_DCNAME, pDCI->DomainControllerName);
        }

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs:  User name is:  %s, Domain name is:  %s"),
             lpName, lpDomain));

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Domain controller is:  %s  Domain DN is %s"),
                 pDCI->DomainControllerName, lpDomainDN));


        if (!(lpGPOInfo->dwFlags & GP_MACHINE)) {
            CallDFS(pDCI->DomainName, pDCI->DomainControllerName);
        }


        //
        // Save the DC name in the registry for future reference
        //

        if (RegOpenKeyEx (lpGPOInfo->hKeyRoot,
                          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\History"),
                          0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {

            dwSize = (lstrlen(pDCI->DomainControllerName) + 1) * sizeof(TCHAR);
            RegSetValueEx (hKey, TEXT("DCName"), 0, REG_SZ,
                           (LPBYTE) pDCI->DomainControllerName, dwSize);

            RegCloseKey (hKey);
        }
    }


    //
    // Read the group policy extensions from the registry
    //

    if ( !ReadGPExtensions( lpGPOInfo ) ) {

        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: ReadGPExtensions failed.")));
        LogEvent (TRUE, IDS_READ_EXT_FAILED );
        goto Exit;

    }


    //
    // Get the user policy mode if appropriate
    //

    if (!(lpGPOInfo->dwFlags & GP_MACHINE)) {

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                          SYSTEM_POLICIES_KEY,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(dwUserPolicyMode);
            RegQueryValueEx (hKey,
                             TEXT("UserPolicyMode"),
                             NULL,
                             &dwType,
                             (LPBYTE) &dwUserPolicyMode,
                             &dwSize);

            RegCloseKey (hKey);
        }

        if (dwUserPolicyMode > 0) {

            if (!(lpGPOInfo->dwFlags & GP_APPLY_DS_POLICY)) {
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Loopback is not allowed for downlevel or local user accounts.  Loopback will be disabled.")));
                LogEvent (FALSE, IDS_LOOPBACK_DISABLED1);
                dwUserPolicyMode = 0;
            }

            if (dwUserPolicyMode > 0) {
                if (lpGPOInfo->iMachineRole < 2) {
                    DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Loopback is not allowed on machines joined to a downlevel domain or running standalone.  Loopback will be disabled.")));
                    LogEvent (TRUE, IDS_LOOPBACK_DISABLED2);
                    dwUserPolicyMode = 0;
                }
            }
        }
    }

    //
    // Check if user's sid has changed
    //

    if ( !CheckForChangedSid( lpGPOInfo ) ) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Check for changed sid failed")));
        goto Exit;
    }

    if (!ImpersonateUser(lpGPOInfo->hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to impersonate user")));
        LogEvent (TRUE, IDS_FAILED_IMPERSONATE, GetLastError() );
        goto Exit;
    }


    //
    // Check if any extensions can be skipped. If there is ever a case where
    // all extensions can be skipped, then exit successfully right after this check.
    // Currently RegistryExtension is always run unless there are no GPO changes,
    // but the GPO changes check is done much later.
    //

    if ( !CheckForSkippedExtensions( lpGPOInfo ) ) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Checking extensions for skipping failed")));
        //
        // LogEvent() is called by CheckForSkippedExtensions()
        //
        goto Exit;
    }


    //
    // Query for the GPO list based upon the mode
    //
    // 0 is normal
    // 1 is merge.  Merge user list + machine list
    // 2 is replace.  use machine list instead of user list
    //

    if (dwUserPolicyMode == 0) {

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Calling GetGPOInfo for normal policy mode")));

        bResult = GetGPOInfo ((lpGPOInfo->dwFlags & GP_MACHINE) ? GPO_LIST_FLAG_MACHINE : 0,
                              lpDomainDN, lpName, NULL, &lpGPOInfo->lpGPOList, pNetAPI32, TRUE);

        if (!bResult) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: GetGPOInfo failed.")));
            LogEvent (TRUE, IDS_GPO_QUERY_FAILED);
        }


    } else if (dwUserPolicyMode == 2) {

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Calling GetGPOInfo for replacement user policy mode")));

        lpComputerName = MyGetComputerName (NameFullyQualifiedDN);
    
        if (lpComputerName) {
    
            PDOMAIN_CONTROLLER_INFO pDCInfo;

            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Using computer name %s for query."), lpComputerName));
    
            dwResult = pNetAPI32->pfnDsGetDcName(   0,
                                                    0,
                                                    0,
                                                    0,
                                                    DS_DIRECTORY_SERVICE_REQUIRED |
                                                    ((lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD) ? DS_BACKGROUND_ONLY : 0),
                                                    &pDCInfo);
            if ( dwResult == 0 )
            {
                    bResult = GetGPOInfo (0, pDCInfo->DomainName, lpComputerName, NULL,
                                          &lpGPOInfo->lpGPOList, pNetAPI32, FALSE);

                    if (!bResult) {
                        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: GetGPOInfo failed.")));
                        LogEvent (TRUE, IDS_GPO_QUERY_FAILED);
                    }
    
                pNetAPI32->pfnNetApiBufferFree( pDCInfo );
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to get the computer domain name with %d"),
                             GetLastError()));
                LogEvent (TRUE, IDS_NO_MACHINE_DOMAIN, lpComputerName, GetLastError() );
                bResult = FALSE;
            }
            LocalFree (lpComputerName);

        } else {
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to get the computer name with %d"),
                         GetLastError()));
            LogEvent (TRUE, IDS_FAILED_MACHINENAME, GetLastError());
            bResult = FALSE;
        }


    } else {

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Calling GetGPOInfo for merging user policy mode")));

        lpComputerName = MyGetComputerName (NameFullyQualifiedDN);

        if (lpComputerName) {

            lpGPOInfo->lpGPOList = NULL;
            bResult = GetGPOInfo (0, lpDomainDN, lpName, NULL,
                                      &lpGPOInfo->lpGPOList, pNetAPI32, FALSE);

            if (bResult) {

                PDOMAIN_CONTROLLER_INFO pDCInfo;
        
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Using computer name %s for query."), lpComputerName));

                lpGPO = NULL;

                dwResult = pNetAPI32->pfnDsGetDcName(   0,
                                                        0,
                                                        0,
                                                        0,
                                                        DS_DIRECTORY_SERVICE_REQUIRED |
                                                        ((lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD) ? DS_BACKGROUND_ONLY : 0),
                                                        &pDCInfo);
                if ( dwResult == 0 )
                {
                    bResult = GetGPOInfo (0, pDCInfo->DomainName, lpComputerName, NULL,
                                      &lpGPO, pNetAPI32, FALSE);

                    if (bResult) {


                        if (lpGPOInfo->lpGPOList && lpGPO) {
    
                            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Both user and machine lists are defined.  Merging them together.")));

                            //
                            // Need to merge the lists together
                            //
    
                            lpGPOTemp = lpGPOInfo->lpGPOList;

                            while (lpGPOTemp->pNext) {
                                lpGPOTemp = lpGPOTemp->pNext;
                            }

                            lpGPOTemp->pNext = lpGPO;

                        } else if (!lpGPOInfo->lpGPOList && lpGPO) {
    
                            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Only machine list is defined.")));
                            lpGPOInfo->lpGPOList = lpGPO;
    
                        } else {
    
                            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Only user list is defined.")));
                        }
    
                    } else {
                        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: GetGPOInfo failed for computer name.")));
                        LogEvent (TRUE, IDS_GPO_QUERY_FAILED);
                    }
                    pNetAPI32->pfnNetApiBufferFree( pDCInfo );

                }
            else
            {
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to get the computer domain name with %d"),
                         GetLastError()));
                LogEvent (TRUE, IDS_NO_MACHINE_DOMAIN, lpComputerName, GetLastError());
                bResult = FALSE;
            }
                    

            } else {
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: GetGPOInfo failed for user name.")));
                LogEvent (TRUE, IDS_GPO_QUERY_FAILED);
            }

            LocalFree (lpComputerName);

        } else {
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to get the computer name with %d"),
                         GetLastError()));
            LogEvent (TRUE, IDS_FAILED_MACHINENAME, GetLastError());
            bResult = FALSE;
        }

    }



    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to revert to self")));
    }


    if (!bResult) {
        goto Exit;
    }

    bResult = SetupGPOFilter( lpGPOInfo );

    if (!bResult) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: SetupGPOFilter failed.")));
        LogEvent (TRUE, IDS_SETUP_GPOFILTER_FAILED);
        goto Exit;
    }

    //
    // Need to check if the security group membership has changed the first time around
    //

    if ( !(lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD) ) {

        if ((lpGPOInfo->dwFlags & GP_MACHINE) && (lpGPOInfo->dwFlags & GP_APPLY_DS_POLICY)) {

        
            HANDLE hLocToken=NULL;

            //
            // if it is machine policy processing, get the machine token so that we can check
            // security group membership using the right token. This causes GetMachineToken to be called twice
            // but moving it to the beginning requires too much change.
            //

            
            hLocToken = GetMachineToken();            

            if (hLocToken) {
                CheckGroupMembership( lpGPOInfo, hLocToken, &lpGPOInfo->bMemChanged, &lpGPOInfo->bUserLocalMemChanged);            
                CloseHandle(hLocToken);
            } 
            else {
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs:  Failed to get the machine token with  %d"), GetLastError()));
                goto Exit;
            }
        }
        else {

            //
            // In the user case just use the token passed in
            //
            
            CheckGroupMembership( lpGPOInfo, lpGPOInfo->hToken, &lpGPOInfo->bMemChanged, &lpGPOInfo->bUserLocalMemChanged);
        }        
    }

    
    
    DebugPrintGPOList( lpGPOInfo );

    //================================================================
    //
    // Now walk through the list of extensions
    //
    //================================================================

    EnterCriticalSection( &g_GPOCS );

    pGPHandle = (LPGPINFOHANDLE) LocalAlloc( LPTR, sizeof(GPINFOHANDLE) );

    //
    // Continue even if pGPHandle is 0, because all it means is that async completions (if any)
    // will fail. Remove old asynch completion context.
    //

    if ( pGPHandle )
        pGPHandle->pGPOInfo = lpGPOInfo;

    if ( lpGPOInfo->dwFlags & GP_MACHINE ) {
        if ( g_pMachGPInfo )
            LocalFree( g_pMachGPInfo );

        g_pMachGPInfo = pGPHandle;
    } else {
        if ( g_pUserGPInfo )
            LocalFree( g_pUserGPInfo );

        g_pUserGPInfo = pGPHandle;
    }

    LeaveCriticalSection( &g_GPOCS );

    pAsyncHandle = (ASYNCCOMPLETIONHANDLE) pGPHandle;

    dwCurrentTime = GetCurTime();

    lpExt = lpGPOInfo->lpExtensions;

    //
    // Before going in, get the thread token and reset the thread token in case
    // one of the extensions hit an exception.
    //
    
    if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ,
                          TRUE, &hOldToken)) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: OpenThreadToken failed with error %d, assuming thread is not impersonating"), GetLastError()));
        hOldToken = NULL;
    }

        
    while ( lpExt ) {

        BOOL bProcessGPOs, bNoChanges, bUsePerUserLocalSetting;
        PGROUP_POLICY_OBJECT pDeletedGPOList;
        DWORD dwRet;

        //
        // Check for early shutdown or user logoff
        //
        if ( (lpGPOInfo->dwFlags & GP_MACHINE) && g_bStopMachGPOProcessing
             || !(lpGPOInfo->dwFlags & GP_MACHINE) && g_bStopUserGPOProcessing ) {

            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Aborting GPO processing due to machine shutdown or logoff")));
            LogEvent (TRUE, IDS_GPO_PROC_STOPPED);
            break;

        }

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: -----------------------")));
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Processing extension %s"),
                 lpExt->lpDisplayName));

        if ( lpExt->bSkipped ) {

            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Extension %s skipped with flags 0x%x."),
                      lpExt->lpDisplayName, lpGPOInfo->dwFlags));
            if (lpGPOInfo->dwFlags & GP_VERBOSE)
                LogEvent (FALSE, IDS_EXT_SKIPPED, lpExt->lpDisplayName, lpGPOInfo->dwFlags);

            lpExt = lpExt->pNext;
            continue;

        }

        //
        // Reset lpGPOInfo->lpGPOList based on extension filter list. If the extension
        // is being called to do delete processing on the history then the current GpoList
        // is null.
        //

        if ( lpExt->bHistoryProcessing ) {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Extension %s is being called to do delete processing on cached history."),
                      lpExt->lpDisplayName));
            lpGPOInfo->lpGPOList = NULL;
        }
        else
            FilterGPOs( lpExt, lpGPOInfo );

        DebugPrintGPOList( lpGPOInfo );

        if ( !CheckGPOs( lpExt, lpGPOInfo, dwCurrentTime,
                         &bProcessGPOs, &bNoChanges, &pDeletedGPOList ) ) {

            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: CheckGPOs failed.")));
            lpExt = lpExt->pNext;
            continue;

        }

        if ( bProcessGPOs ) {

            if ( pDeletedGPOList == NULL && lpGPOInfo->lpGPOList == NULL ) {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Extension %s skipped because both deleted and changed GPO lists are empty."),
                          lpExt->lpDisplayName ));
                if (lpGPOInfo->dwFlags & GP_VERBOSE)
                    LogEvent (FALSE, IDS_EXT_HAS_EMPTY_LISTS, lpExt->lpDisplayName);

                lpExt = lpExt->pNext;
                continue;
            }


            if ( lpExt->dwEnableAsynch ) {

                //
                // Save now to shadow area to avoid race between thread that returns from
                // ProcessGPOList and the thread that does ProcessGroupPolicyCompleted and
                // reads from shadow area.
                //

                SaveGPOList( lpExt->lpKeyName, lpGPOInfo,
                             HKEY_LOCAL_MACHINE,
                             bUsePerUserLocalSetting ? lpGPOInfo->lpwszSidUser : NULL,
                             TRUE, lpGPOInfo->lpGPOList );
            }

            dwRet = E_FAIL;

            __try {

                dwRet = ProcessGPOList( lpExt, lpGPOInfo, pDeletedGPOList,
                                        lpGPOInfo->lpGPOList, bNoChanges, pAsyncHandle );

            }
            __except( GPOExceptionFilter( GetExceptionInformation() ) ) {

                SetThreadToken(NULL, hOldToken);
                
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Extension %s ProcessGroupPolicy threw unhandled exception 0x%x."),
                            lpExt->lpDisplayName, GetExceptionCode() ));
                LogEvent (TRUE, IDS_CAUGHT_EXCEPTION, lpExt->lpDisplayName, GetExceptionCode());
                
            }

            FreeGPOList( pDeletedGPOList );
            pDeletedGPOList = NULL;

            bUsePerUserLocalSetting = lpExt->dwUserLocalSetting && !(lpGPOInfo->dwFlags & GP_MACHINE);

            if ( dwRet == ERROR_SUCCESS || dwRet == ERROR_OVERRIDE_NOCHANGES ) {

                //
                // ERROR_OVERRIDE_NOCHANGES means that extension processed the list and so the cached list
                // must be updated, but the extension will be called the next time even if there are
                // no changes.
                //

                SaveGPOList( lpExt->lpKeyName, lpGPOInfo,
                             HKEY_LOCAL_MACHINE,
                             bUsePerUserLocalSetting ? lpGPOInfo->lpwszSidUser : NULL,
                             FALSE, lpGPOInfo->lpGPOList );
                uExtensionCount++;

            } else if ( dwRet == E_PENDING ) {

                DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Extension %s ProcessGroupPolicy returned e_pending."),
                          lpExt->lpDisplayName));

            } else {

                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Extension %s ProcessGroupPolicy failed, status 0x%x."),
                          lpExt->lpDisplayName, dwRet));
                if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                    LogEvent (FALSE, IDS_CHANGES_FAILED, lpExt->lpDisplayName, dwRet);
                }

            }

            WriteStatus( lpExt->lpKeyName, lpGPOInfo,
                         bUsePerUserLocalSetting ? lpGPOInfo->lpwszSidUser : NULL,
                         dwRet, dwCurrentTime,
                         (lpGPOInfo->dwFlags & GP_SLOW_LINK) != 0 );

        }

        //
        // Process next extension
        //

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: -----------------------")));
        lpExt = lpExt->pNext;
    }


    if (hOldToken)
        CloseHandle(hOldToken);


    //================================================================
    //
    // Success
    //
    //================================================================

    //
    // Set the return value
    //

    bRetVal = TRUE;


Exit:

    //
    // Unload the Group Policy Extensions
    //

    UnloadGPExtensions (lpGPOInfo);

    FreeLists( lpGPOInfo );

    lpGPOInfo->lpGPOList = NULL;
    lpGPOInfo->lpExtFilterList = NULL;

    //
    // Token groups can change only at logon time, so reset to false
    //

    lpGPOInfo->bMemChanged = FALSE;
    lpGPOInfo->bUserLocalMemChanged = FALSE;

    //
    // We migrate the policy data from old sid only at logon time.
    // reset it to false.
    //
    
    lpGPOInfo->bSidChanged = FALSE;

    if (pDCI) {
        pNetAPI32->pfnNetApiBufferFree(pDCI);
    }
    

    if (lpName) {
        LocalFree (lpName);
    }

    if (lpDomain) {
        LocalFree (lpDomain);
    }


    //
    // Release the critical section
    //

    if (lpGPOInfo->hCritSection) {
        LeaveCriticalPolicySection (lpGPOInfo->hCritSection);
        lpGPOInfo->hCritSection = NULL;
    }


    //
    // Announce that policies have changed
    //

    if (bRetVal) {

        if (uExtensionCount) {

            //
            // First, update User with new colors, bitmaps, etc.
            //

            if (lpGPOInfo->dwFlags & GP_REGPOLICY_CPANEL) {

                //
                // Something has changed in the control panel section
                // Start control.exe with the /policy switch so the
                // display is refreshed.
                //

                RefreshDisplay (lpGPOInfo);
            }


            //
            // Notify anyone waiting on an event handle
            //

            if (lpGPOInfo->hNotifyEvent) {
                PulseEvent (lpGPOInfo->hNotifyEvent);
            }


            //
            // Create a thread to broadcase the WM_SETTINGCHANGE message
            //

            hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) PolicyChangedThread,
                                    (LPVOID) ((lpGPOInfo->dwFlags & GP_MACHINE) ? 1 : 0),
                                    CREATE_SUSPENDED, &dwThreadID);

            if (hThread) {
                SetThreadPriority (hThread, THREAD_PRIORITY_IDLE);
                ResumeThread (hThread);
                CloseHandle (hThread);

            } else {
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to create background thread (%d)."),
                         GetLastError()));
                PolicyChangedThread (((lpGPOInfo->dwFlags & GP_MACHINE) ? 1 : 0));
            }
        }

    }

    if (lpGPOInfo->dwFlags & GP_VERBOSE) {
        if (lpGPOInfo->dwFlags & GP_MACHINE) {
            LogEvent (FALSE, IDS_MACHINE_POLICY_APPLIED);
        } else {
            LogEvent (FALSE, IDS_USER_POLICY_APPLIED);
        }
    }

    if (lpGPOInfo->dwFlags & GP_MACHINE) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Computer Group Policy has been applied.")));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: User Group Policy has been applied.")));
    }

    DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Leaving with %d."), bRetVal));

    return bRetVal;
}

//*************************************************************
//
//  PolicyChangedThread()
//
//  Purpose:    Sends the WM_SETTINGCHANGE message announcing
//              that policy has changed.  This is done on a
//              separate thread because this could take many
//              seconds to succeed if an application is hung
//
//  Parameters: BOOL bMachine - machine or user policy has been applied
//
//
//  Return:     0
//
//*************************************************************

DWORD WINAPI PolicyChangedThread (BOOL bMachine)
{
    HINSTANCE hInst;
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    DWORD dwBSM;


    hInst = LoadLibrary (TEXT("userenv.dll"));

    DebugMsg((DM_VERBOSE, TEXT("PolicyChangedThread: Entering with %d."), bMachine));


    //
    // Broadcase the WM_SETTINGCHANGE message
    //

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (NT_SUCCESS(Status)) {

        dwBSM = BSM_ALLDESKTOPS | BSM_APPLICATIONS;
        BroadcastSystemMessage (BSF_IGNORECURRENTTASK | BSF_FORCEIFHUNG,
                                &dwBSM,
                                WM_SETTINGCHANGE,
                                bMachine, (LPARAM) TEXT("Policy"));

        RtlAdjustPrivilege(SE_TCB_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
    }


    DebugMsg((DM_VERBOSE, TEXT("PolicyChangedThread: Leaving")));

    FreeLibraryAndExitThread (hInst, 0);

    return 0;
}


//*************************************************************
//
//  DeleteRegistryValue()
//
//  Purpose:    Callback from ParseRegistryFile that deletes
//              registry policies
//
//  Parameters: lpGPOInfo   -  GPO Information
//              lpKeyName   -  Key name
//              lpValueName -  Value name
//              dwType      -  Registry data type
//              lpData      -  Registry data
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL DeleteRegistryValue (LPGPOINFO lpGPOInfo, LPTSTR lpKeyName,
                          LPTSTR lpValueName, DWORD dwType,
                          DWORD dwDataLength, LPBYTE lpData)
{
    DWORD dwDisp;
    HKEY hSubKey;
    LONG lResult;
    INT iStrLen;
    TCHAR szPolicies1[] = TEXT("Software\\Policies");
    TCHAR szPolicies2[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies");


    //
    // Check if there is a keyname
    //

    if (!lpKeyName || !(*lpKeyName)) {
        return TRUE;
    }


    //
    // Check if the key is in one of the policies keys
    //

    iStrLen = lstrlen(szPolicies1);
    if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE, szPolicies1,
                       iStrLen, lpKeyName, iStrLen) != CSTR_EQUAL) {

        iStrLen = lstrlen(szPolicies2);
        if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE, szPolicies2,
                           iStrLen, lpKeyName, iStrLen) != CSTR_EQUAL) {
            return TRUE;
        }
    }


    //
    // Check if the value name starts with **
    //

    if (lpValueName && (lstrlen(lpValueName) > 1)) {

        if ( (*lpValueName == TEXT('*')) && (*(lpValueName+1) == TEXT('*')) ) {
            return TRUE;
        }
    }


    //
    // We found a value that needs to be deleted
    //

    if (RegCleanUpValue (lpGPOInfo->hKeyRoot, lpKeyName, lpValueName)) {
        DebugMsg((DM_VERBOSE, TEXT("DeleteRegistryValue: Deleted %s\\%s"),
                 lpKeyName, lpValueName));
    } else {
        DebugMsg((DM_WARNING, TEXT("DeleteRegistryValue: Failed to delete %s\\%s"),
                 lpKeyName, lpValueName));
    }


    return TRUE;
}


//*************************************************************
//
//  ResetPolicies()
//
//  Purpose:    Resets the Policies and old Policies key to their
//              original state.
//
//  Parameters: lpGPOInfo   -   GPT information
//              lpArchive   -   Name of archive file
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ResetPolicies (LPGPOINFO lpGPOInfo, LPTSTR lpArchive)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwDisp, dwValue = 0x95;


    DebugMsg((DM_VERBOSE, TEXT("ResetPolicies: Entering.")));


    //
    // Parse the archive file and delete any policies
    //

    if (!ParseRegistryFile (lpGPOInfo, lpArchive,
                            DeleteRegistryValue, NULL)) {
        DebugMsg((DM_WARNING, TEXT("ResetPolicies: Leaving")));
        return FALSE;
    }


    //
    // Recreate the new policies key
    //

    lResult = RegCreateKeyEx (lpGPOInfo->hKeyRoot,
                              TEXT("Software\\Policies"),
                              0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult == ERROR_SUCCESS) {

        //
        // Re-apply security
        //

        RegCloseKey (hKey);

        if (!MakeRegKeySecure((lpGPOInfo->dwFlags & GP_MACHINE) ? NULL : lpGPOInfo->hToken,
                              lpGPOInfo->hKeyRoot,
                              TEXT("Software\\Policies"))) {
            DebugMsg((DM_WARNING, TEXT("ResetPolicies: Failed to secure reg key.")));
        }

    } else {
        DebugMsg((DM_WARNING, TEXT("ResetPolicies: Failed to create reg key with %d."), lResult));
    }


    //
    // Recreate the old policies key
    //

    lResult = RegCreateKeyEx (lpGPOInfo->hKeyRoot,
                              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies"),
                              0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult == ERROR_SUCCESS) {

        //
        // Re-apply security
        //

        RegCloseKey (hKey);

        if (!MakeRegKeySecure((lpGPOInfo->dwFlags & GP_MACHINE) ? NULL : lpGPOInfo->hToken,
                              lpGPOInfo->hKeyRoot,
                              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies"))) {
            DebugMsg((DM_WARNING, TEXT("ResetPolicies: Failed to secure reg key.")));
        }

    } else {
        DebugMsg((DM_WARNING, TEXT("ResetPolicies: Failed to create reg key with %d."), lResult));
    }


    //
    // If this is user policy, reset the NoDriveTypeAutoRun default value
    //

    if (!(lpGPOInfo->dwFlags & GP_MACHINE)) {


        if (RegCreateKeyEx (lpGPOInfo->hKeyRoot,
                          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"),
                          0, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_WRITE, NULL, &hKey, &dwDisp) == ERROR_SUCCESS) {

            RegSetValueEx (hKey, TEXT("NoDriveTypeAutoRun"), 0,
                           REG_DWORD, (LPBYTE) &dwValue, sizeof(dwValue));

            RegCloseKey (hKey);
        }
    }

    DebugMsg((DM_VERBOSE, TEXT("ResetPolicies: Leaving.")));

    return TRUE;
}


//*************************************************************
//
//  SetupGPOFilter()
//
//  Purpose:    Setup up GPO Filter info
//
//  Parameters: lpGPOInfo   - GPO info
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL SetupGPOFilter( LPGPOINFO lpGPOInfo )
{
    //
    // Format is [{ext guid1}{snapin guid1}..{snapin guidn}][{ext guid2}...]...\0
    // Both extension and snapin guids are in ascending order.
    //
    // Note: If the format is corrupt then take the conservative
    //       position and assume that it means that all
    //       extensions need to be applied to the GPO.
    //

    LPEXTFILTERLIST pExtFilterListTail = 0;
    PGROUP_POLICY_OBJECT lpGPO = 0;
    LPEXTFILTERLIST pExtFilterElem = NULL;

    lpGPOInfo->bXferToExtList = FALSE;

    lpGPO = lpGPOInfo->lpGPOList;
    while ( lpGPO ) {

        TCHAR *pchCur = lpGPO->lpExtensions;
        LPEXTLIST pExtListHead = 0;
        LPEXTLIST pExtListTail = 0;

        if ( pchCur ) {

            while ( *pchCur ) {

                GUID guidExt;
                LPEXTLIST pExtElem;

                if ( *pchCur == TEXT('[') )
                    pchCur++;
                else {

                    DebugMsg((DM_WARNING, TEXT("SetupGPOFilter: Corrupt extension name format.")));
                    FreeExtList( pExtListHead );
                    pExtListHead = 0;
                    break;

                }

                if ( ValidateGuid( pchCur ) )
                    StringToGuid( pchCur, &guidExt );
                else {

                    DebugMsg((DM_WARNING, TEXT("SetupGPOFilter: Corrupt extension name format.")));
                    FreeExtList( pExtListHead );
                    pExtListHead = 0;
                    break;

                }

                pExtElem = LocalAlloc( LPTR, sizeof(EXTLIST) );
                if ( pExtElem == 0 ) {

                    DebugMsg((DM_WARNING, TEXT("SetupGPOFilter: Unable to allocate memory.")));
                    FreeExtList( pExtListHead );
                    return FALSE;

                }

                pExtElem->guid = guidExt;
                pExtElem->pNext = 0;

                if ( pExtListTail )
                    pExtListTail->pNext = pExtElem;
                else
                    pExtListHead = pExtElem;

                pExtListTail = pExtElem;

                while ( *pchCur && *pchCur != TEXT('[') )
                    pchCur++;

            } // while *pchcur

        } // if pchcur

        //
        // Append to lpExtFilterList
        //

        pExtFilterElem = LocalAlloc( LPTR, sizeof(EXTFILTERLIST) );
        if ( pExtFilterElem == NULL ) {

             DebugMsg((DM_WARNING, TEXT("SetupGPOFilter: Unable to allocate memory.")));
             FreeExtList( pExtListHead );
             return FALSE;

        }

        pExtFilterElem->lpExtList = pExtListHead;
        pExtFilterElem->lpGPO = lpGPO;
        pExtFilterElem->pNext = NULL;

        if ( pExtFilterListTail == 0 )
            lpGPOInfo->lpExtFilterList = pExtFilterElem;
        else
            pExtFilterListTail->pNext = pExtFilterElem;

        pExtFilterListTail = pExtFilterElem;

        //
        // Advance to next GPO
        //

        lpGPO = lpGPO->pNext;

    } // while lpgpo

    //
    // Transfer ownership from lpGPOList to lpExtFilterList
    //

    lpGPOInfo->bXferToExtList = TRUE;

    return TRUE;
}



//*************************************************************
//
//  FilterGPOs()
//
//  Purpose:    Filter GPOs not relevant to this extension
//
//  Parameters: lpExt        -  Extension
//              lpGPOInfo    -  GPO info
//
//*************************************************************

void FilterGPOs( LPGPEXT lpExt, LPGPOINFO lpGPOInfo )
{


    //
    // lpGPOInfo->lpGPOList will have the filtered list of GPOs
    //

    PGROUP_POLICY_OBJECT pGPOTail = 0;
    LPEXTFILTERLIST pExtFilterList = lpGPOInfo->lpExtFilterList;

    lpGPOInfo->lpGPOList = 0;

    while ( pExtFilterList ) {

        BOOL bFound = FALSE;
        LPEXTLIST pExtList = pExtFilterList->lpExtList;

        if ( pExtList == NULL ) {

            //
            // A null pExtlist means no extensions apply to this GPO
            //

            bFound = FALSE;

        } else {

            while (pExtList) {

                INT iComp = CompareGuid( &lpExt->guid, &pExtList->guid );

                if ( iComp == 0 ) {
                    bFound = TRUE;
                    break;
                } else if ( iComp < 0 ) {
                    //
                    // Guids in pExtList are in ascending order, so we are done
                    //
                    break;
                } else
                    pExtList = pExtList->pNext;

            } // while pextlist

        } // else

        if ( bFound ) {

            //
            // Append pExtFilterList->lpGPO to the filtered GPO list
            //

            pExtFilterList->lpGPO->pNext = 0;
            pExtFilterList->lpGPO->pPrev = pGPOTail;

            if ( pGPOTail == 0 )
                lpGPOInfo->lpGPOList = pExtFilterList->lpGPO;
            else
                pGPOTail->pNext = pExtFilterList->lpGPO;

            pGPOTail = pExtFilterList->lpGPO;

        }  // bFound

        pExtFilterList = pExtFilterList->pNext;

    }  // while pextfilterlist
}



//*************************************************************
//
//  GetDeletedGPOList()
//
//  Purpose:    Get the list of deleted GPOs
//
//  Parameters: lpGPOList        -  List of old GPOs
//              ppDeletedGPOList -  Deleted list returned here
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL GetDeletedGPOList (PGROUP_POLICY_OBJECT lpGPOList,
                        PGROUP_POLICY_OBJECT *ppDeletedGPOList)
{
    *ppDeletedGPOList = NULL;

     //
     // It's possible that lpGPOList could be NULL.  This is ok.
     //

    if (!lpGPOList) {
        DebugMsg((DM_VERBOSE, TEXT("GetDeletedList: No old GPOs.  Leaving.")));
        return TRUE;
    }

    //
    // We need to do any delete operations in reverse order
    // of the way there were applied.
    //

    while ( lpGPOList ) {

        PGROUP_POLICY_OBJECT pCurGPO = lpGPOList;
        lpGPOList = lpGPOList->pNext;

        if ( pCurGPO->lParam & GPO_LPARAM_FLAG_DELETE ) {

            //
            // Prepend to deleted list
            //
            pCurGPO->pNext = *ppDeletedGPOList;
            pCurGPO->pPrev = NULL;

            if ( *ppDeletedGPOList )
                (*ppDeletedGPOList)->pPrev = pCurGPO;

            *ppDeletedGPOList = pCurGPO;

        } else
            LocalFree( pCurGPO );

    }

    DebugMsg((DM_VERBOSE, TEXT("GetDeletedGPOList: Finished.")));

    return TRUE;
}


//*************************************************************
//
//  ReadGPOList()
//
//  Purpose:    Reads the list of Group Policy Objects from
//              the registry
//
//  Parameters: pszExtName -  GP extension
//              hKeyRoot   -  Registry handle
//              hKeyRootMach - Registry handle to hklm
//              lpwszSidUser - Sid of user, if non-null then it means
//                             per user local setting
//              bShadow    -  Read from shadow or from history list
//              lpGPOList  -  pointer to the array of GPOs
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ReadGPOList ( TCHAR * pszExtName, HKEY hKeyRoot,
                   HKEY hKeyRootMach, LPTSTR lpwszSidUser, BOOL bShadow,
                   PGROUP_POLICY_OBJECT * lpGPOList)
{
    INT iIndex = 0;
    LONG lResult;
    HKEY hKey, hSubKey = NULL;
    BOOL bResult = FALSE;
    TCHAR szSubKey[10];
    DWORD dwOptions, dwVersion;
    GPO_LINK GPOLink;
    LPARAM lParam;
    TCHAR szGPOName[50];
    LPTSTR lpDSPath = NULL, lpFileSysPath = NULL, lpDisplayName = NULL, lpExtensions = NULL, lpLink = NULL;
    DWORD dwDisp, dwSize, dwType, dwTemp, dwMaxSize;
    PGROUP_POLICY_OBJECT lpGPO, lpGPOTemp;
    TCHAR szKey[400];


    //
    // Set default
    //

    *lpGPOList = NULL;


    //
    // Open the key that holds the GPO list
    //

    if ( lpwszSidUser == 0 ) {
        wsprintf (szKey,
                  bShadow ? GP_SHADOW_KEY
                            : GP_HISTORY_KEY,
                  pszExtName );

    } else {
        wsprintf (szKey,
                  bShadow ? GP_SHADOW_SID_KEY
                          : GP_HISTORY_SID_KEY,
                          lpwszSidUser, pszExtName );
    }

    lResult = RegOpenKeyEx ( lpwszSidUser ? hKeyRootMach : hKeyRoot,
                            szKey,
                            0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {

        if (lResult == ERROR_FILE_NOT_FOUND) {
            return TRUE;

        } else {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to open reg key with %d."), lResult));
            return FALSE;
        }
    }


    while (TRUE) {

        //
        // Enumerate through the subkeys.  The keys are named by index number
        // eg:  0, 1, 2, 3, etc...
        //

        IntToString (iIndex, szSubKey);

        lResult = RegOpenKeyEx (hKey, szSubKey, 0, KEY_READ, &hSubKey);

        if (lResult != ERROR_SUCCESS) {

            if (lResult == ERROR_FILE_NOT_FOUND) {
                bResult = TRUE;
                goto Exit;

            } else {
                DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to open reg key <%s> with %d."), szSubKey, lResult));
                goto Exit;
            }
        }


        //
        // Read the size of the largest value in this key
        //

        lResult = RegQueryInfoKey (hSubKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   &dwMaxSize, NULL, NULL);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query max size with %d."), lResult));
            goto Exit;
        }


        //
        // Allocate buffers based upon the value above
        //

        lpDSPath = LocalAlloc (LPTR, dwMaxSize);

        if (!lpDSPath) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to allocate memory with %d."), GetLastError()));
            goto Exit;
        }


        lpFileSysPath = LocalAlloc (LPTR, dwMaxSize);

        if (!lpFileSysPath) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to allocate memory with %d."), GetLastError()));
            goto Exit;
        }


        lpDisplayName = LocalAlloc (LPTR, dwMaxSize);

        if (!lpDisplayName) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to allocate memory with %d."), GetLastError()));
            goto Exit;
        }


        lpExtensions = LocalAlloc (LPTR, dwMaxSize);

        if (!lpExtensions) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to allocate memory with %d."), GetLastError()));
            goto Exit;
        }


        lpLink = LocalAlloc (LPTR, dwMaxSize);

        if (!lpLink) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to allocate memory with %d."), GetLastError()));
            goto Exit;
        }


        //
        // Read in the GPO
        //

        dwOptions = 0;
        dwSize = sizeof(dwOptions);
        lResult = RegQueryValueEx (hSubKey, TEXT("Options"), NULL, &dwType,
                                  (LPBYTE) &dwOptions, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query options reg value with %d."), lResult));
        }


        dwVersion = 0;
        dwSize = sizeof(dwVersion);
        lResult = RegQueryValueEx (hSubKey, TEXT("Version"), NULL, &dwType,
                                  (LPBYTE) &dwVersion, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query Version reg value with %d."), lResult));
        }


        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hSubKey, TEXT("DSPath"), NULL, &dwType,
                                  (LPBYTE) lpDSPath, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            if (lResult != ERROR_FILE_NOT_FOUND) {
                DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query DS reg value with %d."), lResult));
                goto Exit;
            }
            LocalFree (lpDSPath);
            lpDSPath = NULL;
        }

        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hSubKey, TEXT("FileSysPath"), NULL, &dwType,
                                  (LPBYTE) lpFileSysPath, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query file sys path reg value with %d."), lResult));
            goto Exit;
        }


        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hSubKey, TEXT("DisplayName"), NULL, &dwType,
                                  (LPBYTE) lpDisplayName, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query display name reg value with %d."), lResult));
            goto Exit;
        }

        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hSubKey, TEXT("Extensions"), NULL, &dwType,
                                  (LPBYTE) lpExtensions, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query extension names reg value with %d."), lResult));

            LocalFree(lpExtensions);
            lpExtensions = NULL;
        }

        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hSubKey, TEXT("Link"), NULL, &dwType,
                                  (LPBYTE) lpLink, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            if (lResult != ERROR_FILE_NOT_FOUND) {
                DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query DS Object reg value with %d."), lResult));
            }
            LocalFree(lpLink);
            lpLink = NULL;
        }

        dwSize = sizeof(szGPOName);
        lResult = RegQueryValueEx (hSubKey, TEXT("GPOName"), NULL, &dwType,
                                  (LPBYTE) szGPOName, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query GPO name reg value with %d."), lResult));
            goto Exit;
        }


        GPOLink = GPLinkUnknown;
        dwSize = sizeof(GPOLink);
        lResult = RegQueryValueEx (hSubKey, TEXT("GPOLink"), NULL, &dwType,
                                  (LPBYTE) &GPOLink, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query reserved reg value with %d."), lResult));
        }


        lParam = 0;
        dwSize = sizeof(lParam);
        lResult = RegQueryValueEx (hSubKey, TEXT("lParam"), NULL, &dwType,
                                  (LPBYTE) &lParam, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query lParam reg value with %d."), lResult));
        }


        //
        // Add the GPO to the list
        //

        if (!AddGPO (lpGPOList, dwOptions, dwVersion, lpDSPath, lpFileSysPath,
                     lpDisplayName, szGPOName, lpExtensions, GPOLink, lpLink, lParam,
                     FALSE, FALSE, FALSE)) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to add GPO to list.")));
            goto Exit;
        }


        //
        // Free the buffers allocated above
        //

        if (lpDSPath) {
            LocalFree (lpDSPath);
            lpDSPath = NULL;
        }

        LocalFree (lpFileSysPath);
        lpFileSysPath = NULL;

        LocalFree (lpDisplayName);
        lpDisplayName = NULL;

        if (lpExtensions) {
            LocalFree(lpExtensions);
            lpExtensions = NULL;
        }

        if (lpLink) {
            LocalFree(lpLink);
            lpLink = NULL;
        }

        //
        // Close the subkey handle
        //

        RegCloseKey (hSubKey);
        hSubKey = NULL;

        iIndex++;
    }

Exit:

    if (lpDSPath) {
        LocalFree (lpDSPath);
    }

    if (lpFileSysPath) {
        LocalFree (lpFileSysPath);
    }

    if (lpDisplayName) {
        LocalFree (lpDisplayName);
    }

    if (lpExtensions) {
        LocalFree(lpExtensions);
    }

    if (lpLink) {
        LocalFree(lpLink);
    }


    if (hSubKey) {
        RegCloseKey (hSubKey);
    }

    RegCloseKey (hKey);

    if (!bResult) {

        //
        // Free any entries in the list
        //

        lpGPO = *lpGPOList;

        while (lpGPO) {
            lpGPOTemp = lpGPO->pNext;
            LocalFree (lpGPO);
            lpGPO = lpGPOTemp;
        }

        *lpGPOList = NULL;
    }


    return bResult;
}

//*************************************************************
//
//  SaveGPOList()
//
//  Purpose:    Saves the list of Group Policy Objects in
//              the registry
//
//  Parameters: pszExtName -  GP extension
//              lpGPOInfo  -  Group policy info
//              hKeyRootMach - Registry handle to hklm
//              lpwszSidUser - Sid of user, if non-null then it means
//                             per user local setting
//              bShadow    -  Save to shadow or to history list
//              lpGPOList  -  Array of GPOs
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL SaveGPOList (TCHAR *pszExtName, LPGPOINFO lpGPOInfo,
                  HKEY hKeyRootMach, LPTSTR lpwszSidUser, BOOL bShadow,
                  PGROUP_POLICY_OBJECT lpGPOList)
{
    INT iIndex = 0;
    LONG lResult;
    HKEY hKey = NULL;
    BOOL bResult = FALSE;
    TCHAR szSubKey[400];
    DWORD dwDisp, dwSize;


    //
    // Start off with an empty key
    //
    if ( lpwszSidUser == 0 ) {
        wsprintf (szSubKey,
                  bShadow ? GP_SHADOW_KEY
                          : GP_HISTORY_KEY,
                  pszExtName);
    } else {
        wsprintf (szSubKey,
                  bShadow ? GP_SHADOW_SID_KEY
                          : GP_HISTORY_SID_KEY,
                  lpwszSidUser, pszExtName);
    }

    if (!RegDelnode (lpwszSidUser ? hKeyRootMach : lpGPOInfo->hKeyRoot,
                     szSubKey)) {
        DebugMsg((DM_VERBOSE, TEXT("SaveGPOList: RegDelnode failed.")));
    }


    //
    // Check if we have any GPOs to store.  It's ok for this to be NULL.
    //

    if (!lpGPOList) {
        return TRUE;
    }

    //
    // Set the proper security on the registry key
    //

    if ( !MakeRegKeySecure( (lpGPOInfo->dwFlags & GP_MACHINE) ? NULL : lpGPOInfo->hToken,
                            lpwszSidUser ? hKeyRootMach : lpGPOInfo->hKeyRoot,
                            szSubKey ) ) {
        DebugMsg((DM_WARNING, TEXT("SaveGpoList: Failed to secure reg key.")));
    }

    //
    // Loop through the GPOs saving them in the registry
    //

    while (lpGPOList) {

        if ( lpwszSidUser == 0 ) {
            wsprintf (szSubKey,
                      bShadow ? TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\Shadow\\%ws\\%d")
                              : TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\History\\%ws\\%d"),
                      pszExtName,
                      iIndex);
        } else {
            wsprintf (szSubKey,
                      bShadow ? TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\Shadow\\%ws\\%d")
                              : TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\History\\%ws\\%d"),
                      lpwszSidUser, pszExtName, iIndex);
        }

        lResult = RegCreateKeyEx (lpwszSidUser ? hKeyRootMach : lpGPOInfo->hKeyRoot,
                                  szSubKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to create reg key with %d."), lResult));
            goto Exit;
        }


        //
        // Save the GPO
        //

        dwSize = sizeof(lpGPOList->dwOptions);
        lResult = RegSetValueEx (hKey, TEXT("Options"), 0, REG_DWORD,
                                 (LPBYTE) &lpGPOList->dwOptions, dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set options reg value with %d."), lResult));
            goto Exit;
        }


        dwSize = sizeof(lpGPOList->dwVersion);
        lResult = RegSetValueEx (hKey, TEXT("Version"), 0, REG_DWORD,
                                 (LPBYTE) &lpGPOList->dwVersion, dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set Version reg value with %d."), lResult));
            goto Exit;
        }


        if (lpGPOList->lpDSPath) {

            dwSize = (lstrlen (lpGPOList->lpDSPath) + 1) * sizeof(TCHAR);
            lResult = RegSetValueEx (hKey, TEXT("DSPath"), 0, REG_SZ,
                                     (LPBYTE) lpGPOList->lpDSPath, dwSize);

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set DS reg value with %d."), lResult));
                goto Exit;
            }
        }

        dwSize = (lstrlen (lpGPOList->lpFileSysPath) + 1) * sizeof(TCHAR);
        lResult = RegSetValueEx (hKey, TEXT("FileSysPath"), 0, REG_SZ,
                                  (LPBYTE) lpGPOList->lpFileSysPath, dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set file sys path reg value with %d."), lResult));
            goto Exit;
        }


        dwSize = (lstrlen (lpGPOList->lpDisplayName) + 1) * sizeof(TCHAR);
        lResult = RegSetValueEx (hKey, TEXT("DisplayName"), 0, REG_SZ,
                                (LPBYTE) lpGPOList->lpDisplayName, dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set display name reg value with %d."), lResult));
            goto Exit;
        }

        if (lpGPOList->lpExtensions) {

            dwSize = (lstrlen (lpGPOList->lpExtensions) + 1) * sizeof(TCHAR);
            lResult = RegSetValueEx (hKey, TEXT("Extensions"), 0, REG_SZ,
                                    (LPBYTE) lpGPOList->lpExtensions, dwSize);

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set extension names reg value with %d."), lResult));
                goto Exit;
            }

        }

        if (lpGPOList->lpLink) {

            dwSize = (lstrlen (lpGPOList->lpLink) + 1) * sizeof(TCHAR);
            lResult = RegSetValueEx (hKey, TEXT("Link"), 0, REG_SZ,
                                    (LPBYTE) lpGPOList->lpLink, dwSize);

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set DSObject reg value with %d."), lResult));
                goto Exit;
            }

        }

        dwSize = (lstrlen (lpGPOList->szGPOName) + 1) * sizeof(TCHAR);
        lResult = RegSetValueEx (hKey, TEXT("GPOName"), 0, REG_SZ,
                                  (LPBYTE) lpGPOList->szGPOName, dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set GPO name reg value with %d."), lResult));
            goto Exit;
        }


        dwSize = sizeof(lpGPOList->GPOLink);
        lResult = RegSetValueEx (hKey, TEXT("GPOLink"), 0, REG_DWORD,
                                 (LPBYTE) &lpGPOList->GPOLink, dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set GPOLink reg value with %d."), lResult));
            goto Exit;
        }


        dwSize = sizeof(lpGPOList->lParam);
        lResult = RegSetValueEx (hKey, TEXT("lParam"), 0, REG_DWORD,
                                 (LPBYTE) &lpGPOList->lParam, dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set lParam reg value with %d."), lResult));
            goto Exit;
        }

        //
        // Close the handle
        //

        RegCloseKey (hKey);
        hKey = NULL;


        //
        // Prep for the next loop
        //

        iIndex++;
        lpGPOList = lpGPOList->pNext;
    }


    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (hKey) {
        RegCloseKey (hKey);
    }

    return bResult;
}


//*************************************************************
//
//  WriteStatus()
//
//  Purpose:    Saves status in the registry
//
//  Parameters: lpGPOInfo  -  GPO info
//              lpExtName  -  GP extension name
//              dwStatus   -  Status to write
//              dwTime     -  Policy time to write
//              dwSlowLink -  Link speed to write
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL WriteStatus( TCHAR *lpExtName, LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser, DWORD dwStatus, DWORD dwTime,
                  DWORD dwSlowLink )
{
    HKEY hKey = NULL;
    DWORD dwDisp, dwSize;
    LONG lResult;
    BOOL bResult = FALSE;
    TCHAR szKey[400];

    if ( lpwszSidUser == 0 ) {
        wsprintf (szKey,
                  GP_EXTENSIONS_KEY,
                  lpExtName);
    } else {
        wsprintf (szKey,
                  GP_EXTENSIONS_SID_KEY,
                  lpwszSidUser, lpExtName);
    }

    lResult = RegCreateKeyEx (lpwszSidUser ? HKEY_LOCAL_MACHINE : lpGPOInfo->hKeyRoot,
                            szKey, 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("WriteStatus: Failed to create reg key with %d."), lResult));
        goto Exit;
    }

    dwSize = sizeof(dwStatus);
    lResult = RegSetValueEx (hKey, TEXT("Status"), 0, REG_DWORD,
                             (LPBYTE) &dwStatus, dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("WriteStatus: Failed to set status reg value with %d."), lResult));
        goto Exit;
    }

    dwSize = sizeof(dwTime);
    lResult = RegSetValueEx (hKey, TEXT("LastPolicyTime"), 0, REG_DWORD,
                             (LPBYTE) &dwTime, dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("WriteStatus: Failed to set time reg value with %d."), lResult));
        goto Exit;
    }

    dwSize = sizeof(dwSlowLink);
    lResult = RegSetValueEx (hKey, TEXT("PrevSlowLink"), 0, REG_DWORD,
                             (LPBYTE) &dwSlowLink, dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("WriteStatus: Failed to set slowlink reg value with %d."), lResult));
        goto Exit;
    }

    bResult = TRUE;

Exit:
    if ( hKey != NULL )
        RegCloseKey( hKey );

    return bResult;

}


//*************************************************************
//
//  ReadStatus()
//
//  Purpose:    Reads status from the registry
//
//  Parameters: lpKeyName  -  Extension name
//              lpGPOInfo  -  GPO info
//              lpwszSidUser - Sid of user, if non-null then it means
//                             per user local setting
//              pdwStatus  -  Status returned here
//              pdwTime    -  Last policy time returned here
//              pdwSlowLink - Previous link speed returned here
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ReadStatus( TCHAR *lpKeyName, LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser, DWORD *pdwStatus, DWORD *pdwTime,
                 DWORD *pdwSlowLink )
{
    HKEY hKey = NULL;
    DWORD dwType, dwSize;
    LONG lResult;
    BOOL bResult = FALSE;
    TCHAR szKey[400];

    if ( lpwszSidUser == 0 ) {
        wsprintf (szKey,
                  GP_EXTENSIONS_KEY,
                  lpKeyName);
    } else {
        wsprintf (szKey,
                  GP_EXTENSIONS_SID_KEY,
                  lpwszSidUser, lpKeyName);
    }

    lResult = RegOpenKeyEx (lpwszSidUser ? HKEY_LOCAL_MACHINE : lpGPOInfo->hKeyRoot,
                            szKey,
                            0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {
        if (lResult != ERROR_FILE_NOT_FOUND) {
            DebugMsg((DM_VERBOSE, TEXT("ReadStatus: Failed to open reg key with %d."), lResult));
        }
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx( hKey, TEXT("Status"), NULL,
                               &dwType, (LPBYTE) pdwStatus,
                               &dwSize );

    if (lResult != ERROR_SUCCESS) {
        if (lResult != ERROR_FILE_NOT_FOUND) {
            DebugMsg((DM_VERBOSE, TEXT("ReadStatus: Failed to read status reg value with %d."), lResult));
        }
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx( hKey, TEXT("LastPolicyTime"), NULL,
                               &dwType, (LPBYTE) pdwTime,
                               &dwSize );

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("ReadStatus: Failed to read time reg value with %d."), lResult));
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx( hKey, TEXT("PrevSlowLink"), NULL,
                               &dwType, (LPBYTE) pdwSlowLink,
                               &dwSize );

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("ReadStatus: Failed to read slowlink reg value with %d."), lResult));
        goto Exit;
    }

    bResult = TRUE;

Exit:
    if ( hKey != NULL )
        RegCloseKey( hKey );

    return bResult;

}

//*************************************************************
//
//  GetCurTime()
//
//  Purpose:    Returns current time in minutes, or 0 if there
//              is a failure
//
//*************************************************************

DWORD GetCurTime()
{
    DWORD dwCurTime = 0;
    LARGE_INTEGER liCurTime;

    if ( NT_SUCCESS( NtQuerySystemTime( &liCurTime) ) ) {

        if ( RtlTimeToSecondsSince1980 ( &liCurTime, &dwCurTime) ) {

            dwCurTime /= 60;   // seconds to minutes
        }
    }

    return dwCurTime;
}



//*************************************************************
//
//  CheckForGPOsToRemove()
//
//  Purpose:    Compares the GPOs in list1 with list 2 to determine
//              if any GPOs need to be removed.
//
//  Parameters: lpGPOList1  -   GPO link list 1
//              lpGPOList2  -   GPO link list 2
//
//  Return:     TRUE if one or more GPOs need to be removed
//              FALSE if not
//
//*************************************************************

BOOL CheckForGPOsToRemove (PGROUP_POLICY_OBJECT lpGPOList1, PGROUP_POLICY_OBJECT lpGPOList2)
{
    PGROUP_POLICY_OBJECT lpGPOSrc, lpGPODest;
    BOOL bFound;
    BOOL bResult = FALSE;


    //
    // First check to see if they are both NULL
    //

    if (!lpGPOList1 && !lpGPOList2) {
        return FALSE;
    }


    //
    // Go through every GPO in list 1, and see if it is still in list 2
    //

    lpGPOSrc = lpGPOList1;

    while (lpGPOSrc) {

        lpGPODest = lpGPOList2;
        bFound = FALSE;

        while (lpGPODest) {

            if (!lstrcmpi (lpGPOSrc->szGPOName, lpGPODest->szGPOName)) {
                bFound = TRUE;
                break;
            }

            lpGPODest = lpGPODest->pNext;
        }

        if (!bFound) {
            DebugMsg((DM_VERBOSE, TEXT("CheckForGPOsToRemove: GPO <%s> needs to be removed"), lpGPOSrc->lpDisplayName));
            lpGPOSrc->lParam |= GPO_LPARAM_FLAG_DELETE;
            bResult = TRUE;
        }

        lpGPOSrc = lpGPOSrc->pNext;
    }


    return bResult;
}

//*************************************************************
//
//  CompareGPOLists()
//
//  Purpose:    Compares one list of GPOs to another
//
//  Parameters: lpGPOList1  -   GPO link list 1
//              lpGPOList2  -   GPO link list 2
//
//  Return:     TRUE if the lists are the same
//              FALSE if not
//
//*************************************************************

BOOL CompareGPOLists (PGROUP_POLICY_OBJECT lpGPOList1, PGROUP_POLICY_OBJECT lpGPOList2)
{

    //
    // Check if one list is empty
    //

    if ((lpGPOList1 && !lpGPOList2) || (!lpGPOList1 && lpGPOList2)) {
        DebugMsg((DM_VERBOSE, TEXT("CompareGPOLists:  One list is empty")));
        return FALSE;
    }


    //
    // Loop through the GPOs
    //

    while (lpGPOList1 && lpGPOList2) {

        //
        // Compare GPO names
        //

        if (lstrcmpi (lpGPOList1->szGPOName, lpGPOList2->szGPOName) != 0) {
            DebugMsg((DM_VERBOSE, TEXT("CompareGPOLists:  Different entries found.")));
            return FALSE;
        }


        //
        // Compare the version numbers
        //

        if (lpGPOList1->dwVersion != lpGPOList2->dwVersion) {
            DebugMsg((DM_VERBOSE, TEXT("CompareGPOLists:  Different version numbers found")));
            return FALSE;
        }


        //
        // Move to the next node
        //

        lpGPOList1 = lpGPOList1->pNext;
        lpGPOList2 = lpGPOList2->pNext;


        //
        // Check if one list has more entries than the other
        //

        if ((lpGPOList1 && !lpGPOList2) || (!lpGPOList1 && lpGPOList2)) {
            DebugMsg((DM_VERBOSE, TEXT("CompareGPOLists:  One list has more entries than the other")));
            return FALSE;
        }
    }


    DebugMsg((DM_VERBOSE, TEXT("CompareGPOLists:  The lists are the same.")));

    return TRUE;
}


//*************************************************************
//
//  HistoryPresent()
//
//  Purpose:    Checks if the current extension has any cached
//              GPOs
//
//  Parameters: lpGPOInfo   -   GPOInfo
//              lpExt       -   Extension
//
//
//  Return:     TRUE if cached GPOs present
//              FALSE otherwise
//
//*************************************************************

BOOL HistoryPresent( LPGPOINFO lpGPOInfo, LPGPEXT lpExt )
{
    TCHAR szKey[400];
    LONG lResult;
    HKEY hKey;

    //
    // Check if history is cached on per user per machine basis
    //

    BOOL bUsePerUserLocalSetting = lpExt->dwUserLocalSetting && !(lpGPOInfo->dwFlags & GP_MACHINE);

    DmAssert( !bUsePerUserLocalSetting || lpGPOInfo->lpwszSidUser != 0 );

    if ( bUsePerUserLocalSetting ) {
        wsprintf( szKey, GP_HISTORY_SID_KEY,
                  lpGPOInfo->lpwszSidUser, lpExt->lpKeyName );
    } else {
        wsprintf( szKey, GP_HISTORY_KEY,
                  lpExt->lpKeyName );
    }

    lResult = RegOpenKeyEx ( bUsePerUserLocalSetting ? HKEY_LOCAL_MACHINE : lpGPOInfo->hKeyRoot,
                             szKey,
                             0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS) {
        RegCloseKey( hKey );
        return TRUE;
    } else
        return FALSE;
}


//*************************************************************
//
//  MigrateMembershipData()
//
//  Purpose:    Moves group membership data from old sid to new
//              sid.
//
//  Parameters: lpwszSidUserNew - New sid
//              lpwszSidUserOld - Old sid
//
//  Return:     TRUE if success
//              FALSE otherwise
//
//*************************************************************

BOOL MigrateMembershipData( LPTSTR lpwszSidUserNew, LPTSTR lpwszSidUserOld )
{
    DWORD dwCount = 0;
    DWORD dwSize, dwType, dwMaxSize, dwDisp;
    DWORD i= 0;
    LONG lResult;
    HKEY hKeyRead = NULL, hKeyWrite = NULL;
    BOOL bResult = TRUE;
    LPTSTR lpSid = NULL;
    TCHAR szKeyRead[250];
    TCHAR szKeyWrite[250];
    TCHAR szGroup[30];

    wsprintf( szKeyRead, GP_MEMBERSHIP_KEY, lpwszSidUserOld );

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szKeyRead, 0, KEY_READ, &hKeyRead);

    if (lResult != ERROR_SUCCESS)
        return TRUE;

    wsprintf( szKeyWrite, GP_MEMBERSHIP_KEY, lpwszSidUserNew );

    if ( !RegDelnode( HKEY_LOCAL_MACHINE, szKeyWrite ) ) {
        DebugMsg((DM_VERBOSE, TEXT("MigrateMembershipData: RegDelnode failed.")));
        bResult = FALSE;
        goto Exit;
    }

    lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE, szKeyWrite, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyWrite, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("MigrateMembershipData: Failed to create key with %d."), lResult));
        bResult = FALSE;
        goto Exit;
    }

    dwSize = sizeof(dwCount);
    lResult = RegQueryValueEx (hKeyRead, TEXT("Count"), NULL, &dwType,
                               (LPBYTE) &dwCount, &dwSize);
    if ( lResult != ERROR_SUCCESS ) {
        DebugMsg((DM_VERBOSE, TEXT("MigrateMembershipData: Failed to read membership count")));
        goto Exit;
    }


    lResult = RegQueryInfoKey (hKeyRead, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               &dwMaxSize, NULL, NULL);
    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("MigrateMembershipData: Failed to query max size with %d."), lResult));
        goto Exit;
    }

    //
    // Allocate buffer based upon the largest value
    //

    lpSid = LocalAlloc (LPTR, dwMaxSize);

    if (!lpSid) {
        DebugMsg((DM_WARNING, TEXT("MigrateMembershipData: Failed to allocate memory with %d."), lResult));
        bResult = FALSE;
        goto Exit;
    }

    for ( i=0; i<dwCount; i++ ) {

        wsprintf( szGroup, TEXT("Group%d"), i );

        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hKeyRead, szGroup, NULL, &dwType, (LPBYTE) lpSid, &dwSize);
        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("MigrateMembershipData: Failed to read value %ws"), szGroup ));
            goto Exit;
        }

        dwSize = (lstrlen(lpSid) + 1) * sizeof(TCHAR);
        lResult = RegSetValueEx (hKeyWrite, szGroup, 0, REG_SZ, (LPBYTE) lpSid, dwSize);

        if (lResult != ERROR_SUCCESS) {
            bResult = FALSE;
            DebugMsg((DM_WARNING, TEXT("MigrateMembershipData: Failed to write value %ws"), szGroup ));
            goto Exit;
        }

    }

    dwSize = sizeof(dwCount);
    lResult = RegSetValueEx (hKeyWrite, TEXT("Count"), 0, REG_DWORD, (LPBYTE) &dwCount, dwSize);

    if (lResult != ERROR_SUCCESS) {
        bResult = FALSE;
        DebugMsg((DM_WARNING, TEXT("MigrateMembershipData: Failed to write count value") ));
        goto Exit;
    }


Exit:

    if ( lpSid )
        LocalFree( lpSid );

    if ( hKeyRead )
        RegCloseKey (hKeyRead);

    if ( hKeyWrite )
        RegCloseKey (hKeyWrite);

    return bResult;
}


//*************************************************************
//
//  MigrateGPOData()
//
//  Purpose:    Moves cached GPOs from old sid to new
//              sid.
//
//  Parameters: lpGPOInfo       -   GPOInfo
//              lpwszSidUserNew - New sid
//              lpwszSidUserOld - Old sid
//
//  Return:     TRUE if success
//              FALSE otherwise
//
//*************************************************************

BOOL MigrateGPOData( LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUserNew, LPTSTR lpwszSidUserOld )
{
    TCHAR szKey[250];
    LONG lResult;
    HKEY hKey = NULL;
    DWORD dwIndex = 0;
    TCHAR szExtension[50];
    DWORD dwSize = 50;
    FILETIME ftWrite;
    PGROUP_POLICY_OBJECT pGPOList, lpGPO, lpGPOTemp;
    BOOL bResult;

    wsprintf( szKey, GP_HISTORY_SID_ROOT_KEY, lpwszSidUserOld );

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hKey);
    if ( lResult != ERROR_SUCCESS )
        return TRUE;

    while (RegEnumKeyEx (hKey, dwIndex, szExtension, &dwSize,
                         NULL, NULL, NULL, &ftWrite) == ERROR_SUCCESS ) {

        if ( ReadGPOList( szExtension, NULL, HKEY_LOCAL_MACHINE,
                         lpwszSidUserOld, FALSE, &pGPOList) ) {

            bResult = SaveGPOList( szExtension, lpGPOInfo, HKEY_LOCAL_MACHINE,
                                   lpwszSidUserNew, FALSE, pGPOList );
            lpGPO = pGPOList;

            while (lpGPO) {
                lpGPOTemp = lpGPO->pNext;
                LocalFree (lpGPO);
                lpGPO = lpGPOTemp;
            }

            if ( !bResult ) {
                DebugMsg((DM_WARNING, TEXT("MigrateGPOData: Failed to save GPO list") ));
                RegCloseKey( hKey );
                return FALSE;
            }

        }

        dwSize = ARRAYSIZE(szExtension);
        dwIndex++;
    }

    RegCloseKey( hKey );
    return TRUE;
}


//*************************************************************
//
//  MigrateStatusData()
//
//  Purpose:    Moves extension status data from old sid to new
//              sid.
//
//  Parameters: lpGPOInfo       -   GPOInfo
//              lpwszSidUserNew - New sid
//              lpwszSidUserOld - Old sid
//
//  Return:     TRUE if success
//              FALSE otherwise
//
//*************************************************************

BOOL MigrateStatusData( LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUserNew, LPTSTR lpwszSidUserOld )
{
    TCHAR szKey[250];
    LONG lResult;
    HKEY hKey = NULL;
    DWORD dwIndex = 0;
    TCHAR szExtension[50];
    DWORD dwSize = 50;
    FILETIME ftWrite;
    BOOL bTemp;
    DWORD dwStatus, dwTime, dwSlowLink;

    wsprintf( szKey, GP_EXTENSIONS_SID_ROOT_KEY, lpwszSidUserOld );

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hKey);
    if ( lResult != ERROR_SUCCESS )
        return TRUE;

    while (RegEnumKeyEx (hKey, dwIndex, szExtension, &dwSize,
                         NULL, NULL, NULL, &ftWrite) == ERROR_SUCCESS ) {

        if ( ReadStatus( szExtension, lpGPOInfo, lpwszSidUserOld, &dwStatus, &dwTime, &dwSlowLink ) ) {

            bTemp = WriteStatus( szExtension, lpGPOInfo, lpwszSidUserNew, dwStatus, dwTime, dwSlowLink );

            if ( !bTemp ) {
                DebugMsg((DM_WARNING, TEXT("MigrateStatusData: Failed to save status") ));
                RegCloseKey( hKey );
                return FALSE;
            }
        }

        dwSize = ARRAYSIZE(szExtension);
        dwIndex++;
    }

    RegCloseKey( hKey );
    return TRUE;

}



//*************************************************************
//
//  CheckForChangedSid()
//
//  Purpose:    Checks if the user's sid has changed and if so,
//              moves history data from old sid to new sid.
//
//  Parameters: lpGPOInfo   -   GPOInfo
//
//  Return:     TRUE if success
//              FALSE otherwise
//
//*************************************************************

BOOL CheckForChangedSid (LPGPOINFO lpGPOInfo)
{
    TCHAR szKey[400];
    LONG lResult;
    HKEY hKey = NULL;
    LPTSTR lpwszSidUserOld = NULL;
    DWORD dwDisp;
    BOOL bCommit = FALSE;      // True, if move of history data should be committed

    //
    // initialize it to FALSE at the beginning and if the Sid has
    // changed we will set it to true later on..
    //
    
    lpGPOInfo->bSidChanged = FALSE;
    
    if ( lpGPOInfo->dwFlags & GP_MACHINE )
        return TRUE;


    if ( lpGPOInfo->lpwszSidUser == 0 ) {

        lpGPOInfo->lpwszSidUser = GetSidString( lpGPOInfo->hToken );
        if ( lpGPOInfo->lpwszSidUser == 0 ) {
            DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: GetSidString failed.")));

            LogEvent(TRUE, IDS_FAILED_GET_SID);
            return FALSE;
        }
    }

    if (!(lpGPOInfo->dwFlags & GP_APPLY_DS_POLICY))
        return TRUE;

    //
    // Check if the key where history is cached exists
    //

    wsprintf( szKey, GP_POLICY_SID_KEY, lpGPOInfo->lpwszSidUser );

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hKey);

    if ( lResult == ERROR_SUCCESS ) {
        RegCloseKey( hKey );
        return TRUE;
    }

    if ( lResult != ERROR_FILE_NOT_FOUND ) {
        DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: Failed to open registry key with %d."),
                  lResult ));
        return FALSE;
    }

    //
    // This is the first time that we are seeing this sid, it can either be a brand new sid or
    // an old sid that has been renamed.
    //

    lpwszSidUserOld =  GetOldSidString( lpGPOInfo->hToken, POLICY_GUID_PATH );

    if ( lpwszSidUserOld == NULL ) {

        //
        // Brand new sid
        //

        if ( !SetOldSidString(lpGPOInfo->hToken, lpGPOInfo->lpwszSidUser, POLICY_GUID_PATH) ) {
             DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: WriteSidMapping failed.") ));

             LogEvent(TRUE, IDS_FAILED_WRITE_SID_MAPPING);
             return FALSE;
        }

        lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE, szKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: RegCreateKey failed.") ));
            return TRUE;
        }

        RegCloseKey( hKey );

        return TRUE;
    }

    //
    // Need to migrate history data from old sid to new sid
    //

    if ( !MigrateMembershipData( lpGPOInfo->lpwszSidUser, lpwszSidUserOld ) ) {
        DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: MigrateMembershipData failed.") ));
        LogEvent(TRUE, IDS_FAILED_MIGRATION);
        goto Exit;
    }

    if ( !MigrateGPOData( lpGPOInfo, lpGPOInfo->lpwszSidUser, lpwszSidUserOld ) ) {
        DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: MigrateGPOData failed.") ));
        LogEvent(TRUE, IDS_FAILED_MIGRATION);
        goto Exit;
    }

    if ( !MigrateStatusData( lpGPOInfo, lpGPOInfo->lpwszSidUser, lpwszSidUserOld ) ) {
        DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: MigrateStatusData failed.") ));
        LogEvent(TRUE, IDS_FAILED_MIGRATION);
        goto Exit;
    }

    bCommit = TRUE;

Exit:

    if ( bCommit ) {

        if ( !SetOldSidString(lpGPOInfo->hToken, lpGPOInfo->lpwszSidUser, POLICY_GUID_PATH) )
             DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: SetOldString failed.") ));

        lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE, szKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);

        if (lResult == ERROR_SUCCESS)
            RegCloseKey( hKey );
        else
            DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: RegCreateKey failed.") ));

        wsprintf( szKey, GP_POLICY_SID_KEY, lpwszSidUserOld );
        RegDelnode( HKEY_LOCAL_MACHINE, szKey );

        wsprintf( szKey, GP_LOGON_SID_KEY, lpwszSidUserOld );
        RegDelnode( HKEY_LOCAL_MACHINE, szKey );


        //
        // if we managed to successfully migrate everything
        //
        
        lpGPOInfo->bSidChanged = TRUE;

        
    } else {

        wsprintf( szKey, GP_POLICY_SID_KEY, lpGPOInfo->lpwszSidUser );
        RegDelnode( HKEY_LOCAL_MACHINE, szKey );

        wsprintf( szKey, GP_LOGON_SID_KEY, lpGPOInfo->lpwszSidUser );
        RegDelnode( HKEY_LOCAL_MACHINE, szKey );

    }

    if ( lpwszSidUserOld )
        LocalFree( lpwszSidUserOld );

    return bCommit;
}


//*************************************************************
//
//  CheckForSkippedExtensions()
//
//  Purpose:    Checks to the current list of extensions to see
//              if any of them have been skipped
//
//  Parameters: lpGPOInfo   -   GPOInfo
//
//
//  Return:     TRUE if success
//              FALSE otherwise
//
//*************************************************************

BOOL CheckForSkippedExtensions (LPGPOINFO lpGPOInfo)
{
    DWORD dwStatus = 0;
    DWORD dwPrevTime = 0;
    DWORD dwSlowLinkPrev = FALSE;

    BOOL dwFlags = lpGPOInfo->dwFlags;

    LPGPEXT lpExt = lpGPOInfo->lpExtensions;

    while ( lpExt ) {

        //
        // Read previous slow link speed
        //

        BOOL bUsePerUserLocalSetting = lpExt->dwUserLocalSetting && !(lpGPOInfo->dwFlags & GP_MACHINE);

        DmAssert( !bUsePerUserLocalSetting || lpGPOInfo->lpwszSidUser != 0 );

        if ( ReadStatus( lpExt->lpKeyName, lpGPOInfo,
                         bUsePerUserLocalSetting ? lpGPOInfo->lpwszSidUser : NULL,
                         &dwStatus, &dwPrevTime, &dwSlowLinkPrev ) ) {
            lpExt->dwSlowLinkPrev = dwSlowLinkPrev;
        } else
            lpExt->dwSlowLinkPrev = FALSE;


        if ( // Check background preference
             lpExt->dwNoBackgroundPolicy && dwFlags & GP_BACKGROUND_THREAD
           ) {

            lpExt->bSkipped = TRUE;

        } else if (  lpExt->dwNoSlowLink && dwFlags & GP_SLOW_LINK) {

            //
            // Slow link preference can be overridden by link transition preference
            //

            DWORD dwSlowLinkCur = (lpGPOInfo->dwFlags & GP_SLOW_LINK) != 0;

            if ( lpExt->dwLinkTransition && ( dwSlowLinkCur != lpExt->dwSlowLinkPrev ) )
                lpExt->bSkipped = FALSE;
            else
                lpExt->bSkipped = TRUE;

        } else {

            //
            // If cached history is present but policy is turned off then still call
            // extension one more time so that cached policies can be passed to extension
            // to do delete processing. If there is no cached history then extension can be skipped.
            //

            BOOL bPolicySkippedPreference = lpExt->dwNoMachPolicy && dwFlags & GP_MACHINE        // mach policy
                                            || lpExt->dwNoUserPolicy && !(dwFlags & GP_MACHINE); // user policy

            if ( bPolicySkippedPreference ) {

                BOOL bHistoryPresent = HistoryPresent( lpGPOInfo, lpExt );
                if ( bHistoryPresent )
                    lpExt->bHistoryProcessing = TRUE;
                else
                    lpExt->bSkipped = TRUE;

            }

        }

        lpExt = lpExt->pNext;

    }

    return TRUE;
}

//*************************************************************
//
//  CheckGPOs()
//
//  Purpose:    Checks to the current list of GPOs with
//              the list stored in the registry to see
//              if policy needs to be flushed.
//
//  Parameters: lpExt            - GP extension
//              lpGPOInfo        - GPOInfo
//              dwTime           - Current time in minutes
//              pbProcessGPOs    - On return set TRUE if GPOs have to be processed
//              pbNoChanges      - On return set to TRUE if no changes, but extension
//                                    has asked for GPOs to be still processed
//              ppDeletedGPOList - On return set to deleted GPO list, if any
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL CheckGPOs (LPGPEXT lpExt,
                LPGPOINFO lpGPOInfo,
                DWORD dwCurrentTime,
                BOOL *pbProcessGPOs,
                BOOL *pbNoChanges,
                PGROUP_POLICY_OBJECT *ppDeletedGPOList)
{
    PGROUP_POLICY_OBJECT lpOldGPOList, lpGPO, lpGPOTemp;
    BOOL bTemp;

    BOOL bUsePerUserLocalSetting = lpExt->dwUserLocalSetting && !(lpGPOInfo->dwFlags & GP_MACHINE);

    *pbProcessGPOs = TRUE;
    *pbNoChanges = FALSE;
    *ppDeletedGPOList = NULL;

    DmAssert( !bUsePerUserLocalSetting || lpGPOInfo->lpwszSidUser != 0 );

    //
    // Read in the old GPO list
    //

    bTemp = ReadGPOList (lpExt->lpKeyName, lpGPOInfo->hKeyRoot,
                         HKEY_LOCAL_MACHINE,
                         bUsePerUserLocalSetting ? lpGPOInfo->lpwszSidUser : NULL,
                         FALSE, &lpOldGPOList);

    if (!bTemp) {
        DebugMsg((DM_WARNING, TEXT("CheckGPOs: ReadGPOList failed.")));
        LogEvent (TRUE, IDS_FAILED_READ_GPO_LIST);

        lpOldGPOList = NULL;
    }


    //
    // Compare with the new GPO list to determine if any GPOs have been
    // removed.
    //

    bTemp = CheckForGPOsToRemove (lpOldGPOList, lpGPOInfo->lpGPOList);


    //
    // If no GPOs need to be removed, then we need to compare the version
    // numbers of the GPOs to see if any have been updated.
    //

    if (bTemp) {

        if (lpGPOInfo->dwFlags & GP_VERBOSE) {
            LogEvent (FALSE, IDS_GPO_LIST_CHANGED);
        }

        if ( !GetDeletedGPOList (lpOldGPOList, ppDeletedGPOList)) {

            DebugMsg((DM_WARNING, TEXT("CheckGPOs: GetDeletedList failed for %s."), lpExt->lpDisplayName));
            LogEvent (TRUE, IDS_FAILED_GETDELETED_LIST, lpExt->lpDisplayName);

        }

    } else {

        BOOL bMembershipChanged = bUsePerUserLocalSetting && lpGPOInfo->bUserLocalMemChanged
                                  || !bUsePerUserLocalSetting && lpGPOInfo->bMemChanged;

        if (CompareGPOLists (lpOldGPOList, lpGPOInfo->lpGPOList) && !bMembershipChanged && (!(lpGPOInfo->bSidChanged))) {

            //
            // The list of GPOs hasn't changed or been updated, and the security group
            // membership has not changed. The default is to not call the extension if
            // it has NoGPOListChanges set. However this can be overridden based on other
            // extension preferences. These are hacks for performance.
            //
            // Exception: Even if nothing has changed but the user's sid changes we need to
            // call the extensions so that they can update their settings
            //

            DWORD dwStatus = 0;
            DWORD dwPrevTime = 0;
            BOOL bSkip = TRUE;      // Start with the default case
            BOOL bNoChanges = TRUE;
            DWORD dwSlowLinkCur = (lpGPOInfo->dwFlags & GP_SLOW_LINK) != 0;
            DWORD dwSlowLinkPrev;


            BOOL bStatusOk = ReadStatus( lpExt->lpKeyName, lpGPOInfo,
                                         bUsePerUserLocalSetting ? lpGPOInfo->lpwszSidUser : NULL,
                                         &dwStatus, &dwPrevTime, &dwSlowLinkPrev );

            if ( !bStatusOk ) {

                //
                // Couldn't read the previous status or time, so the conservative solution is to call
                // extension.
                //

                bSkip = FALSE;
                DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but couldn't read extension %s's status or policy time."),
                              lpExt->lpDisplayName));

            } else if ( bStatusOk && dwStatus == ERROR_OVERRIDE_NOCHANGES ) {

                //
                // When the previous call completed the status code has explicitly asked the framework
                // to disregard the NoGPOListChanges setting.
                //

                bSkip = FALSE;
                bNoChanges = FALSE;
                DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but extension %s had returned ERROR_OVERRIDE_NOCHANGES for previous policy processing call."),
                              lpExt->lpDisplayName));

            } else if ( bStatusOk
                        && dwStatus != ERROR_SUCCESS ) {

                //
                // Extension returned error code, so call the extension again with changes.
                //

                bSkip = FALSE;
                bNoChanges = FALSE;
                DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but extension %s's returned error status %d earlier."),
                              lpExt->lpDisplayName, dwStatus ));


            } else if ( bStatusOk
                        && lpExt->dwLinkTransition
                        && ( lpExt->dwSlowLinkPrev != dwSlowLinkCur ) ) {

                //
                // If there has been a link speed transition then no changes is overridden.
                //

                bSkip = FALSE;
                DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but extension %s's has a link speed transtion from %d to %d."),
                              lpExt->lpDisplayName, lpExt->dwSlowLinkPrev, dwSlowLinkCur ));

            } else if ( bStatusOk
                        && dwStatus == ERROR_SUCCESS
                        && lpExt->dwNoGPOChanges
                        && lpExt->dwMaxChangesInterval != 0 ) {

                if ( dwCurrentTime == 0
                     || dwPrevTime == 0
                     || dwCurrentTime < dwPrevTime ) {

                    //
                    // Handle clock overflow case by assuming that interval has been exceeded
                    //

                    bSkip = FALSE;
                    DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but extension %s's MaxNoGPOListChangesInterval has been exceeded due to clock overflow."),
                              lpExt->lpDisplayName));

                } else if ( (dwCurrentTime - dwPrevTime) > lpExt->dwMaxChangesInterval ) {

                    //
                    // Extension has specified a time interval for which NoGPOListChanges is valid and the time
                    // interval has been exceeded.
                    //

                    bSkip = FALSE;
                    DebugMsg((DM_VERBOSE,
                              TEXT("CheckGPOs: No GPO changes but extension %s's MaxNoGPOListChangesInterval has been exceeded."),
                              lpExt->lpDisplayName));
                }

            }

            if ( bSkip && lpExt->dwNoGPOChanges ) {

                //
                // Case of skipping extension when there are *really* no changes and extension
                // set NoGPOListChanges to true.
                //

                DebugMsg((DM_VERBOSE,
                          TEXT("CheckGPOs: No GPO changes and no security group membership change and extension %s has NoGPOChanges set."),
                          lpExt->lpDisplayName));
                if (lpGPOInfo->dwFlags & GP_VERBOSE)
                    LogEvent (FALSE, IDS_NO_CHANGES, lpExt->lpDisplayName);

                *pbProcessGPOs = FALSE;

            } else
                *pbNoChanges = bNoChanges;

        } // if CompareGpoLists

        //
        // Free the old GPO list
        //

        lpGPO = lpOldGPOList;

        while (lpGPO) {
            lpGPOTemp = lpGPO->pNext;
            LocalFree (lpGPO);
            lpGPO = lpGPOTemp;
        }

    }   // else if bTemp

    return TRUE;
}

//*************************************************************
//
//  ReadGPExtensions()
//
//  Purpose:    Reads the group policy extenions from registry.
//              The actual loading of extension is deferred.
//
//  Parameters: lpGPOInfo   -   GP Information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ReadGPExtensions (LPGPOINFO lpGPOInfo)
{
    TCHAR szSubKey[MAX_PATH];
    DWORD dwType;
    HKEY hKey, hKeyOverride;
    DWORD dwIndex = 0;
    DWORD dwSize = 50;
    TCHAR szDisplayName[50];
    TCHAR szKeyName[50];
    TCHAR szDllName[MAX_PATH+1];
    TCHAR szExpDllName[MAX_PATH+1];
    CHAR  szFunctionName[100];
    FILETIME ftWrite;
    HKEY hKeyExt;
    HINSTANCE hInstDLL;
    LPGPEXT lpExt, lpTemp;

    //
    // Check if any extensions are registered
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      GP_EXTENSIONS,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {


        //
        // Enumerate the keys (each extension has its own key)
        //

        while (RegEnumKeyEx (hKey, dwIndex, szKeyName, &dwSize,
                                NULL, NULL, NULL, &ftWrite) == ERROR_SUCCESS) {


            //
            // Open the extension's key.
            //

            if (RegOpenKeyEx (hKey, szKeyName,
                              0, KEY_READ, &hKeyExt) == ERROR_SUCCESS) {

                if ( ValidateGuid( szKeyName ) ) {

                    if ( lstrcmpi(szKeyName, c_szRegistryExtName) != 0 ) {

                        //
                        // Every extension, other than RegistryExtension is required to have a value called
                        // DllName.  This value can be REG_SZ or REG_EXPAND_SZ type.
                        //

                        dwSize = sizeof(szDllName);
                        if (RegQueryValueEx (hKeyExt, TEXT("DllName"), NULL,
                                             &dwType, (LPBYTE) szDllName,
                                             &dwSize) == ERROR_SUCCESS) {

                            BOOL bFuncFound = FALSE;

                            DWORD dwNoMachPolicy = FALSE;
                            DWORD dwNoUserPolicy = FALSE;
                            DWORD dwNoSlowLink   = FALSE;
                            DWORD dwNoBackgroundPolicy = FALSE;
                            DWORD dwNoGPOChanges = FALSE;
                            DWORD dwUserLocalSetting = FALSE;
                            DWORD dwRequireRegistry = FALSE;
                            DWORD dwEnableAsynch = FALSE;
                            DWORD dwMaxChangesInterval = 0;
                            DWORD dwLinkTransition = FALSE;

                            ExpandEnvironmentStrings (szDllName, szExpDllName, MAX_PATH);

                            //
                            // Read the function name
                            //

                            dwSize = sizeof(szFunctionName);

                            if (RegQueryValueExA (hKeyExt, "ProcessGroupPolicy", NULL,
                                                         &dwType, (LPBYTE) szFunctionName,
                                                         &dwSize) == ERROR_SUCCESS) {

                                //
                                // Read preferences
                                //

                                bFuncFound = TRUE;

                                dwSize = sizeof(szDisplayName);
                                if (RegQueryValueEx (hKeyExt, NULL, NULL,
                                                     &dwType, (LPBYTE) szDisplayName,
                                                     &dwSize) != ERROR_SUCCESS) {
                                    lstrcpyn (szDisplayName, szKeyName, ARRAYSIZE(szDisplayName));
                                }


                                dwSize = sizeof(DWORD);
                                RegQueryValueEx( hKeyExt, TEXT("NoMachinePolicy"), NULL,
                                                 &dwType, (LPBYTE) &dwNoMachPolicy,
                                                 &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("NoUserPolicy"), NULL,
                                                 &dwType, (LPBYTE) &dwNoUserPolicy,
                                                 &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("NoSlowLink"), NULL,
                                                     &dwType, (LPBYTE) &dwNoSlowLink,
                                                     &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("NoGPOListChanges"), NULL,
                                                     &dwType, (LPBYTE) &dwNoGPOChanges,
                                                     &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("NoBackgroundPolicy"), NULL,
                                                     &dwType, (LPBYTE) &dwNoBackgroundPolicy,
                                                     &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("PerUserLocalSettings"), NULL,
                                                 &dwType, (LPBYTE) &dwUserLocalSetting,
                                                 &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("RequiresSuccessfulRegistry"), NULL,
                                                 &dwType, (LPBYTE) &dwRequireRegistry,
                                                 &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("EnableAsynchronousProcessing"), NULL,
                                                 &dwType, (LPBYTE) &dwEnableAsynch,
                                                 &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("MaxNoGPOListChangesInterval"), NULL,
                                                 &dwType, (LPBYTE) &dwMaxChangesInterval,
                                                 &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("NotifyLinkTransition"), NULL,
                                                 &dwType, (LPBYTE) &dwLinkTransition,
                                                 &dwSize );

                                //
                                // Read override policy values, if any
                                //

                                wsprintf (szSubKey, GP_EXTENSIONS_POLICIES, szKeyName );

                                if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                                  szSubKey,
                                                  0, KEY_READ, &hKeyOverride ) == ERROR_SUCCESS) {

                                    dwSize = sizeof(DWORD);
                                    RegQueryValueEx( hKeyOverride, TEXT("NoSlowLink"), NULL,
                                                     &dwType, (LPBYTE) &dwNoSlowLink,
                                                     &dwSize );

                                    RegQueryValueEx( hKeyOverride, TEXT("NoGPOListChanges"), NULL,
                                                     &dwType, (LPBYTE) &dwNoGPOChanges,
                                                     &dwSize );

                                    RegQueryValueEx( hKeyOverride, TEXT("NoBackgroundPolicy"), NULL,
                                                     &dwType, (LPBYTE) &dwNoBackgroundPolicy,
                                                     &dwSize );

                                    RegCloseKey( hKeyOverride );
                                }

                            }

                            if ( bFuncFound ) {

                                lpExt = LocalAlloc (LPTR, sizeof(GPEXT)
                                                          + ((lstrlen(szDisplayName) + 1) * sizeof(TCHAR))
                                                          + ((lstrlen(szKeyName) + 1) * sizeof(TCHAR))
                                                          + ((lstrlen(szExpDllName) + 1) * sizeof(TCHAR))
                                                          + lstrlenA(szFunctionName) + 1 );
                                if (lpExt) {

                                    //
                                    // Set up all fields
                                    //

                                    lpExt->lpDisplayName = (LPTSTR)((LPBYTE)lpExt + sizeof(GPEXT));
                                    lstrcpy( lpExt->lpDisplayName, szDisplayName );

                                    lpExt->lpKeyName = lpExt->lpDisplayName + lstrlen(lpExt->lpDisplayName) + 1;
                                    lstrcpy( lpExt->lpKeyName, szKeyName );

                                    StringToGuid( szKeyName, &lpExt->guid );

                                    lpExt->lpDllName = lpExt->lpKeyName + lstrlen(lpExt->lpKeyName) + 1;
                                    lstrcpy (lpExt->lpDllName, szExpDllName);

                                    lpExt->lpFunctionName = (LPSTR)( (LPBYTE)lpExt->lpDllName + (lstrlen(lpExt->lpDllName) + 1) * sizeof(TCHAR) );
                                    lstrcpyA( lpExt->lpFunctionName, szFunctionName );

                                    lpExt->hInstance = NULL;
                                    lpExt->pEntryPoint = NULL;

                                    lpExt->dwNoMachPolicy = dwNoMachPolicy;
                                    lpExt->dwNoUserPolicy = dwNoUserPolicy;
                                    lpExt->dwNoSlowLink = dwNoSlowLink;
                                    lpExt->dwNoBackgroundPolicy = dwNoBackgroundPolicy;
                                    lpExt->dwNoGPOChanges = dwNoGPOChanges;
                                    lpExt->dwUserLocalSetting = dwUserLocalSetting;
                                    lpExt->dwRequireRegistry = dwRequireRegistry;
                                    lpExt->dwEnableAsynch = dwEnableAsynch;
                                    lpExt->dwMaxChangesInterval = dwMaxChangesInterval;
                                    lpExt->dwLinkTransition = dwLinkTransition;

                                    lpExt->bRegistryExt = FALSE;
                                    lpExt->bSkipped = FALSE;
                                    lpExt->pNext = NULL;

                                    //
                                    // Append to end of extension list
                                    //

                                    if (lpGPOInfo->lpExtensions) {

                                        lpTemp = lpGPOInfo->lpExtensions;

                                        while (TRUE) {
                                            if (lpTemp->pNext) {
                                                lpTemp = lpTemp->pNext;
                                            } else {
                                                break;
                                            }
                                        }

                                        lpTemp->pNext = lpExt;

                                    } else {
                                        lpGPOInfo->lpExtensions = lpExt;
                                    }

                                } else {   // if lpExt
                                    DebugMsg((DM_WARNING, TEXT("ReadGPExtensions: Failed to allocate memory with %d"),
                                              GetLastError()));
                                }
                            } else {       // if bFuncFound
                                DebugMsg((DM_WARNING, TEXT("ReadGPExtensions: Failed to query for the function name.")));
                                LogEvent (TRUE, IDS_EXT_MISSING_FUNC, szExpDllName);
                            }
                        } else {           // if RegQueryValueEx DllName
                            DebugMsg((DM_WARNING, TEXT("ReadGPExtensions: Failed to query DllName value.")));
                            LogEvent (TRUE, IDS_EXT_MISSING_DLLNAME, szKeyName);
                        }
                    } // if lstrcmpi(szKeyName, c_szRegistryExtName)

                }  // if validateguid

                RegCloseKey (hKeyExt);
            }     // if RegOpenKey hKeyExt

            dwSize = ARRAYSIZE(szKeyName);
            dwIndex++;
        }         // while RegEnumKeyEx

        RegCloseKey (hKey);
    }             // if RegOpenKey gpext

    //
    // Add the registry psuedo extension at the beginning
    //

    if ( LoadString (g_hDllInstance, IDS_REGISTRYNAME, szDisplayName, ARRAYSIZE(szDisplayName)) ) {

        lpExt = LocalAlloc (LPTR, sizeof(GPEXT)
                            + ((lstrlen(szDisplayName) + 1) * sizeof(TCHAR))
                            + ((lstrlen(c_szRegistryExtName) + 1) * sizeof(TCHAR)) );
    } else {

        lpExt = 0;
    }

    if (lpExt) {

        DWORD dwNoSlowLink = FALSE;
        DWORD dwNoGPOChanges = TRUE;
        DWORD dwNoBackgroundPolicy = FALSE;

        lpExt->lpDisplayName = (LPTSTR)((LPBYTE)lpExt + sizeof(GPEXT));
        lstrcpy( lpExt->lpDisplayName, szDisplayName );

        lpExt->lpKeyName = lpExt->lpDisplayName + lstrlen(lpExt->lpDisplayName) + 1;
        lstrcpy( lpExt->lpKeyName, c_szRegistryExtName );

        StringToGuid( lpExt->lpKeyName, &lpExt->guid );;

        lpExt->lpDllName = NULL;
        lpExt->lpFunctionName = NULL;
        lpExt->hInstance = NULL;
        lpExt->pEntryPoint = NULL;

        //
        // Read override policy values, if any
        //

        wsprintf (szSubKey, GP_EXTENSIONS_POLICIES, lpExt->lpKeyName );

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                          szSubKey,
                          0, KEY_READ, &hKeyOverride ) == ERROR_SUCCESS) {

            RegQueryValueEx( hKeyOverride, TEXT("NoGPOListChanges"), NULL,
                             &dwType, (LPBYTE) &dwNoGPOChanges,
                             &dwSize );

            RegQueryValueEx( hKeyOverride, TEXT("NoBackgroundPolicy"), NULL,
                             &dwType, (LPBYTE) &dwNoBackgroundPolicy,
                             &dwSize );
            RegCloseKey( hKeyOverride );

        }

        lpExt->dwNoMachPolicy = FALSE;
        lpExt->dwNoUserPolicy = FALSE;
        lpExt->dwNoSlowLink = dwNoSlowLink;
        lpExt->dwNoBackgroundPolicy = dwNoBackgroundPolicy;
        lpExt->dwNoGPOChanges = dwNoGPOChanges;
        lpExt->dwUserLocalSetting = FALSE;
        lpExt->dwRequireRegistry = FALSE;
        lpExt->dwEnableAsynch = FALSE;
        lpExt->dwLinkTransition = FALSE;

        lpExt->bRegistryExt = TRUE;
        lpExt->bSkipped = FALSE;

        lpExt->pNext = lpGPOInfo->lpExtensions;
        lpGPOInfo->lpExtensions = lpExt;

    } else {
        DebugMsg((DM_WARNING, TEXT("ReadGPExtensions: Failed to allocate memory with %d"),
                  GetLastError()));

        return FALSE;

    }

    return TRUE;
}



//*************************************************************
//
//  LoadGPExtension()
//
//  Purpose:    Loads a GP extension.
//
//  Parameters: lpExt -- GP extension
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL LoadGPExtension( LPGPEXT lpExt )
{
    if ( !lpExt->bRegistryExt && lpExt->hInstance == NULL )
    {
        lpExt->hInstance = LoadLibrary( lpExt->lpDllName );
        if ( lpExt->hInstance )
        {
            lpExt->pEntryPoint = (PFNPROCESSGROUPPOLICY)GetProcAddress(lpExt->hInstance,
                                                                       lpExt->lpFunctionName);
            if ( lpExt->pEntryPoint == NULL )
            {
                DebugMsg((DM_WARNING,
                          TEXT("LoadGPExtension: Failed to query ProcessGroupPolicy function entry point in dll <%s> with %d"),
                          lpExt->lpDllName, GetLastError()));
                LogEvent (TRUE, IDS_EXT_FUNC_FAIL, lpExt->lpDllName);

                return FALSE;
            }
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("LoadGPExtension: Failed to load dll <%s> with %d"),
                      lpExt->lpDllName, GetLastError()));
            LogEvent (TRUE, IDS_EXT_LOAD_FAIL, lpExt->lpDllName, GetLastError());

            return FALSE;
        }
    }

    return TRUE;
}




//*************************************************************
//
//  UnloadGPExtensions()
//
//  Purpose:    Unloads the Group Policy extension dlls
//
//  Parameters: lpGPOInfo   -   GP Information
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL UnloadGPExtensions (LPGPOINFO lpGPOInfo)
{
    LPGPEXT lpExt, lpTemp;

    lpExt = lpGPOInfo->lpExtensions;

    while (lpExt) {

        if ( lpExt->hInstance != NULL )
            FreeLibrary (lpExt->hInstance);

        lpTemp = lpExt->pNext;

        LocalFree (lpExt);

        lpExt = lpTemp;
    }

    lpGPOInfo->lpExtensions = NULL;

    return TRUE;
}

//*************************************************************
//
//  ProcessGPOList()
//
//  Purpose:    Calls client side extensions to process gpos
//
//  Parameters: lpExt           - GP extension
//              lpGPOInfo       - GPT Information
//              pDeletedGPOList - Deleted GPOs
//              pChangedGPOList - New/changed GPOs
//              bNoChanges      - True if there are no GPO changes
//                                  and GPO processing is forced
//              pAsyncHandle    - Context for async completion
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

DWORD ProcessGPOList (LPGPEXT lpExt,
                      LPGPOINFO lpGPOInfo,
                      PGROUP_POLICY_OBJECT pDeletedGPOList,
                      PGROUP_POLICY_OBJECT pChangedGPOList,
                      BOOL bNoChanges, ASYNCCOMPLETIONHANDLE pAsyncHandle )
{
    LPTSTR lpGPTPath, lpDSPath;
    INT iStrLen;
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwFlags = 0;
    PGROUP_POLICY_OBJECT lpGPO;
    TCHAR szStatusFormat[50];
    TCHAR szVerbose[100];
    DWORD dwSlowLinkCur = (lpGPOInfo->dwFlags & GP_SLOW_LINK) != 0;

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ProcessGPOList: Entering for extension %s"), lpExt->lpDisplayName));

    if (lpGPOInfo->pStatusCallback) {
        LoadString (g_hDllInstance, IDS_CALLEXTENSION, szStatusFormat, ARRAYSIZE(szStatusFormat));
        wsprintf (szVerbose, szStatusFormat, lpExt->lpDisplayName);
        lpGPOInfo->pStatusCallback(TRUE, szVerbose);
    }

    if (lpGPOInfo->dwFlags & GP_MACHINE) {
        dwFlags |= GPO_INFO_FLAG_MACHINE;
    }

    if (lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD) {
        dwFlags |= GPO_INFO_FLAG_BACKGROUND;
    }

    if (lpGPOInfo->dwFlags & GP_SLOW_LINK) {
        dwFlags |= GPO_INFO_FLAG_SLOWLINK;
    }

    if ( dwSlowLinkCur != lpExt->dwSlowLinkPrev ) {
        dwFlags |= GPO_INFO_FLAG_LINKTRANSITION;
    }

    if (lpGPOInfo->dwFlags & GP_VERBOSE) {
        dwFlags |= GPO_INFO_FLAG_VERBOSE;
    }

    if ( bNoChanges ) {
        dwFlags |= GPO_INFO_FLAG_NOCHANGES;
    }

    dwRet = ERROR_SUCCESS;

    if ( lpExt->bRegistryExt ) {

        //
        // Registry pseudo extension.
        //

        if (!ProcessGPORegistryPolicy (lpGPOInfo, pChangedGPOList)) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPOList: ProcessGPORegistryPolicy failed.")));
            dwRet = E_FAIL;
        }

    } else {    // if lpExt->bRegistryExt

        //
        // Regular extension
        //

        BOOL *pbAbort;
        ASYNCCOMPLETIONHANDLE pAsyncHandleTemp;

        if ( lpExt->dwRequireRegistry ) {

            DWORD dwStatus;
            DWORD dwTime;
            DWORD dwSlowLink;

            if ( !ReadStatus( c_szRegistryExtName, lpGPOInfo, NULL, &dwStatus, &dwTime, &dwSlowLink ) || dwStatus != ERROR_SUCCESS ) {

                DebugMsg((DM_VERBOSE,
                          TEXT("ProcessGPOList: Skipping extension %s due to failed Registry extension."),
                          lpExt->lpDisplayName));
                if (lpGPOInfo->dwFlags & GP_VERBOSE)
                    LogEvent( FALSE, IDS_EXT_SKIPPED_DUETO_FAILED_REG, lpExt->lpDisplayName );
                dwRet = E_FAIL;

                goto Exit;

            }
        }

        if ( !LoadGPExtension( lpExt ) ) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPOList: LoadGPExtension %s failed."), lpExt->lpDisplayName));

            dwRet = E_FAIL;
            goto Exit;
        }

        if ( lpGPOInfo->dwFlags & GP_MACHINE )
            pbAbort = &g_bStopMachGPOProcessing;
        else
            pbAbort = &g_bStopUserGPOProcessing;

        //
        // Check if asynchronous processing is enabled
        //

        if ( lpExt->dwEnableAsynch )
            pAsyncHandleTemp = pAsyncHandle;
        else
            pAsyncHandleTemp = 0;

        dwRet = lpExt->pEntryPoint( dwFlags,
                                    lpGPOInfo->hToken,
                                    lpGPOInfo->hKeyRoot,
                                    pDeletedGPOList,
                                    pChangedGPOList,
                                    pAsyncHandleTemp,
                                    pbAbort,
                                    lpGPOInfo->pStatusCallback );

        RevertToSelf();

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOList: Extension %s returned 0x%x."),
                  lpExt->lpDisplayName, dwRet));

        if ((dwRet != ERROR_SUCCESS) && (dwRet != ERROR_OVERRIDE_NOCHANGES) &&
            (dwRet != E_PENDING))
            LogEvent( TRUE, IDS_EXT_FAILED, lpExt->lpDisplayName, dwFlags, dwRet );

    }   // else of if lpext->bregistryext

Exit:

    return dwRet;
}


//*************************************************************
//
//  CheckGroupMembership()
//
//  Purpose:    Checks if the security groups has changed,
//              and if so saves the new security groups.
//
//  Parameters: lpGPOInfo - LPGPOINFO struct
//              pbMemChanged          - Change status returned here
//              pbUserLocalMemChanged - PerUserLocal change status returned here
//
//*************************************************************

void CheckGroupMembership( LPGPOINFO lpGPOInfo, HANDLE hToken, BOOL *pbMemChanged, BOOL *pbUserLocalMemChanged )
{
    PTOKEN_GROUPS pGroups = 0;
    DWORD dwSize = 0;

    DWORD dwStatus = NtQueryInformationToken( hToken,
                                              TokenGroups,
                                              pGroups,
                                              dwSize,
                                              &dwSize );

    if ( dwStatus ==  STATUS_BUFFER_TOO_SMALL ) {

        pGroups = (PTOKEN_GROUPS) LocalAlloc( LPTR, dwSize );

        if ( pGroups == 0 ) {
            *pbMemChanged = TRUE;
            *pbUserLocalMemChanged = TRUE;

            goto Exit;
        }

        dwStatus = NtQueryInformationToken( hToken,
                                            TokenGroups,
                                            pGroups,
                                            dwSize,
                                            &dwSize );
    }

    if ( dwStatus != STATUS_SUCCESS ) {
        *pbMemChanged = TRUE;
        *pbUserLocalMemChanged = TRUE;

        goto Exit;
    }

    //
    // First do the machine and roaming user case
    //

    *pbMemChanged = ReadMembershipList( lpGPOInfo, NULL, pGroups );
    if ( *pbMemChanged )
        SaveMembershipList( lpGPOInfo, NULL, pGroups );

    //
    // Now the per user local settings case
    //

    if ( lpGPOInfo->dwFlags & GP_MACHINE ) {

        *pbUserLocalMemChanged = *pbMemChanged;

    } else {

        DmAssert( lpGPOInfo->lpwszSidUser != 0 );

        *pbUserLocalMemChanged = ReadMembershipList( lpGPOInfo, lpGPOInfo->lpwszSidUser, pGroups );
        if ( *pbUserLocalMemChanged )
            SaveMembershipList( lpGPOInfo, lpGPOInfo->lpwszSidUser, pGroups );
    }

Exit:

    if ( pGroups != 0 )
        LocalFree( pGroups );

}



//*************************************************************
//
//  ReadMembershipList()
//
//  Purpose:    Reads cached memberhip list and checks if the
//              security groups has changed.
//
//  Parameters: lpGPOInfo - LPGPOINFO struct
//              lpwszSidUser - Sid of user, if non-null then it means
//                             per user local setting
//              pGroups   - List of token groups
//
//  Return:     TRUE if changed
//              FALSE otherwise
//
//*************************************************************

BOOL ReadMembershipList( LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser, PTOKEN_GROUPS pGroupsCur )
{
    DWORD i= 0;
    LONG lResult;
    TCHAR szGroup[30];
    TCHAR szKey[250];
    HKEY hKey = NULL;
    BOOL bDiff = TRUE;
    DWORD dwCountOld = 0;
    DWORD dwSize, dwType, dwMaxSize;
    LPTSTR lpSid = NULL;

    DWORD dwCountCur = 0;

    //
    // Get current count of groups ignoring groups that have
    // the SE_GROUP_LOGON_ID attribute set as this sid will be different
    // for each logon session.
    //

    for ( i=0; i < pGroupsCur->GroupCount; i++) {
        if ( (SE_GROUP_LOGON_ID & pGroupsCur->Groups[i].Attributes) == 0 )
            dwCountCur++;
    }

    //
    // Read from cached group membership list
    //

    if ( lpwszSidUser == 0 )
        wsprintf( szKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\GroupMembership") );
    else
        wsprintf( szKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\GroupMembership"),
                  lpwszSidUser );


    lResult = RegOpenKeyEx ( lpwszSidUser ? HKEY_LOCAL_MACHINE : lpGPOInfo->hKeyRoot,
                             szKey,
                             0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS)
        return TRUE;

    dwSize = sizeof(dwCountOld);
    lResult = RegQueryValueEx (hKey, TEXT("Count"), NULL, &dwType,
                               (LPBYTE) &dwCountOld, &dwSize);

    if ( lResult != ERROR_SUCCESS ) {
        DebugMsg((DM_VERBOSE, TEXT("ReadMembershipList: Failed to read old group count") ));
        goto Exit;
    }

    //
    // Now compare the old and new number of security groups
    //

    if ( dwCountOld != dwCountCur ) {
        DebugMsg((DM_VERBOSE, TEXT("ReadMembershipList: Old count %d is different from current count %d"),
                  dwCountOld, dwCountCur ));
        goto Exit;
    }

    //
    // Total group count is the same, now check that each individual group is the same.
    // First read the size of the largest value in this key.
    //

    lResult = RegQueryInfoKey (hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               &dwMaxSize, NULL, NULL);
    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("ReadMembershipList: Failed to query max size with %d."), lResult));
        goto Exit;
    }

    //
    // account for possible NULL
    // 
    dwMaxSize += sizeof( WCHAR );

    //
    // Allocate buffer based upon the largest value
    //

    lpSid = LocalAlloc (LPTR, dwMaxSize);

    if (!lpSid) {
        DebugMsg((DM_WARNING, TEXT("ReadMembershipList: Failed to allocate memory with %d."), lResult));
        goto Exit;
    }

    for ( i=0; i<dwCountOld; i++ ) {

        wsprintf( szGroup, TEXT("Group%d"), i );

        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hKey, szGroup, NULL, &dwType,
                                   (LPBYTE) lpSid, &dwSize);
        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadMembershipList: Failed to read value %ws"), szGroup ));
            goto Exit;
        }

        if ( !GroupInList( lpSid, pGroupsCur ) ) {
            DebugMsg((DM_WARNING, TEXT("ReadMembershipList: Group %ws not in current list of token groups"), lpSid ));
            goto Exit;
        }

    }

    bDiff = FALSE;

Exit:

    if ( lpSid )
        LocalFree( lpSid );

    if ( hKey )
        RegCloseKey (hKey);

    return bDiff;
}



//*************************************************************
//
//  GroupInList()
//
//  Purpose:    Checks if sid in is list of security groups.
//
//  Parameters: lpSid   - Sid to check
//              pGroups - List of token groups
//
//  Return:     TRUE if sid is in list
//              FALSE otherwise
//
//*************************************************************

BOOL GroupInList( LPTSTR lpSid, PTOKEN_GROUPS pGroups )
{
    PSID    pSid = 0;
    DWORD   dwStatus, i;
    BOOL    bInList = FALSE;

    //
    // Optimize the basic case where the user is an earthling
    //

    if ( 0 == lstrcmpi (lpSid, L"s-1-1-0") )
        return TRUE;

    dwStatus = AllocateAndInitSidFromString (lpSid, &pSid);

    if (ERROR_SUCCESS != dwStatus)
        return FALSE;

    //
    // Cannot match up cached groups with current groups one-by-one because
    // current pGroups can have groups with  SE_GROUP_LOGON_ID attribute
    // set which are different for each logon session.
    //

    for ( i=0; i < pGroups->GroupCount; i++ ) {

        bInList = RtlEqualSid (pSid, pGroups->Groups[i].Sid);
        if ( bInList )
            break;

    }

    RtlFreeSid (pSid);

    return bInList;
}




//*************************************************************
//
//  SavesMembershipList()
//
//  Purpose:    Caches memberhip list
//
//  Parameters: lpGPOInfo - LPGPOINFO struct
//              lpwszSidUser - Sid of user, if non-null then it means
//                             per user local setting
//              pGroups   - List of token groups to cache
//
//  Notes:      The count is saved last because it serves
//              as a commit point for the entire save operation.
//
//*************************************************************

void SaveMembershipList( LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser, PTOKEN_GROUPS pGroups )
{
    TCHAR szKey[250];
    TCHAR szGroup[30];
    DWORD i;
    LONG lResult;
    DWORD dwCount = 0, dwSize, dwDisp;
    NTSTATUS ntStatus;
    UNICODE_STRING unicodeStr;
    HKEY hKey = NULL;

    //
    // Start with clean key
    //

    if ( lpwszSidUser == 0 )
        wsprintf( szKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\GroupMembership") );
    else
        wsprintf( szKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\GroupMembership"),
                  lpwszSidUser );

    if (!RegDelnode ( lpwszSidUser ? HKEY_LOCAL_MACHINE : lpGPOInfo->hKeyRoot, szKey) ) {
        DebugMsg((DM_VERBOSE, TEXT("SaveMembershipList: RegDelnode failed.")));
        return;
    }

    lResult = RegCreateKeyEx ( lpwszSidUser ? HKEY_LOCAL_MACHINE : lpGPOInfo->hKeyRoot,
                               szKey, 0, NULL,
                               REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);
    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("SaveMemberList: Failed to create key with %d."), lResult));
        goto Exit;
    }

    for ( i=0; i < pGroups->GroupCount; i++) {

        if (SE_GROUP_LOGON_ID & pGroups->Groups[i].Attributes )
            continue;

        dwCount++;

        //
        // Convert user SID to a string.
        //

        ntStatus = RtlConvertSidToUnicodeString( &unicodeStr,
                                                 pGroups->Groups[i].Sid,
                                                 (BOOLEAN)TRUE ); // Allocate
        if ( !NT_SUCCESS(ntStatus) ) {
            DebugMsg((DM_WARNING, TEXT("SaveMembershipList: RtlConvertSidToUnicodeString failed, status = 0x%x"),
                      ntStatus));
            goto Exit;
        }

        wsprintf( szGroup, TEXT("Group%d"), dwCount-1 );

        dwSize = (lstrlen (unicodeStr.Buffer) + 1) * sizeof(TCHAR);
        lResult = RegSetValueEx (hKey, szGroup, 0, REG_SZ,
                                 (LPBYTE) unicodeStr.Buffer, dwSize);

        RtlFreeUnicodeString( &unicodeStr );

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveMemberList: Failed to set value %ws with %d."),
                      szGroup, lResult));
            goto Exit;
        }

    }   // for

    //
    // Commit by writing count
    //

    dwSize = sizeof(dwCount);
    lResult = RegSetValueEx (hKey, TEXT("Count"), 0, REG_DWORD,
                             (LPBYTE) &dwCount, dwSize);

Exit:
    if (hKey)
        RegCloseKey (hKey);
}



//*************************************************************
//
//  ArchiveRegistryValue()
//
//  Purpose:    Archives a registry value in the specified file
//
//  Parameters: hFile - File handle of archive file
//              lpKeyName    -  Key name
//              lpValueName  -  Value name
//              dwType       -  Registry value type
//              dwDataLength -  Registry value size
//              lpData       -  Registry value
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ArchiveRegistryValue(HANDLE hFile, LPWSTR lpKeyName,
                          LPWSTR lpValueName, DWORD dwType,
                          DWORD dwDataLength, LPBYTE lpData)
{
    BOOL bResult = FALSE;
    DWORD dwBytesWritten;
    DWORD dwTemp;
    const WCHAR cOpenBracket = L'[';
    const WCHAR cCloseBracket = L']';
    const WCHAR cSemiColon = L';';


    //
    // Write the entry to the text file.
    //
    // Format:
    //
    // [keyname;valuename;type;datalength;data]
    //

    // open bracket
    if (!WriteFile (hFile, &cOpenBracket, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write open bracket with %d"),
                 GetLastError()));
        goto Exit;
    }


    // key name
    dwTemp = (lstrlen (lpKeyName) + 1) * sizeof (WCHAR);
    if (!WriteFile (hFile, lpKeyName, dwTemp, &dwBytesWritten, NULL) ||
        dwBytesWritten != dwTemp)
    {
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write key name with %d"),
                 GetLastError()));
        goto Exit;
    }


    // semicolon
    if (!WriteFile (hFile, &cSemiColon, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write semicolon with %d"),
                 GetLastError()));
        goto Exit;
    }

    // value name
    dwTemp = (lstrlen (lpValueName) + 1) * sizeof (WCHAR);
    if (!WriteFile (hFile, lpValueName, dwTemp, &dwBytesWritten, NULL) ||
        dwBytesWritten != dwTemp)
    {
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write value name with %d"),
                 GetLastError()));
        goto Exit;
    }


    // semicolon
    if (!WriteFile (hFile, &cSemiColon, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write semicolon with %d"),
                 GetLastError()));
        goto Exit;
    }

    // type
    if (!WriteFile (hFile, &dwType, sizeof(DWORD), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(DWORD))
    {
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write data type with %d"),
                 GetLastError()));
        goto Exit;
    }

    // semicolon
    if (!WriteFile (hFile, &cSemiColon, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write semicolon with %d"),
                 GetLastError()));
        goto Exit;
    }

    // data length
    if (!WriteFile (hFile, &dwDataLength, sizeof(DWORD), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(DWORD))
    {
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write data type with %d"),
                 GetLastError()));
        goto Exit;
    }

    // semicolon
    if (!WriteFile (hFile, &cSemiColon, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write semicolon with %d"),
                 GetLastError()));
        goto Exit;
    }

    // data
    if (!WriteFile (hFile, lpData, dwDataLength, &dwBytesWritten, NULL) ||
        dwBytesWritten != dwDataLength)
    {
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write data with %d"),
                 GetLastError()));
        goto Exit;
    }

    // close bracket
    if (!WriteFile (hFile, &cCloseBracket, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write close bracket with %d"),
                 GetLastError()));
        goto Exit;
    }


    //
    // Sucess
    //

    bResult = TRUE;

Exit:

    return bResult;
}


//*************************************************************
//
//  ParseRegistryFile()
//
//  Purpose:    Parses a registry.pol file
//
//  Parameters: lpGPOInfo          -   GPO information
//              lpRegistry         -   Path to registry.pol
//              pfnRegFileCallback - Callback function
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ParseRegistryFile (LPGPOINFO lpGPOInfo, LPTSTR lpRegistry,
                        PFNREGFILECALLBACK pfnRegFileCallback,
                        HANDLE hArchive)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BOOL bResult = FALSE;
    DWORD dwTemp, dwBytesRead, dwType, dwDataLength;
    LPWSTR lpKeyName = 0, lpValueName = 0, lpTemp;
    LPBYTE lpData = NULL;
    WCHAR  chTemp;
    HANDLE hOldToken;


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ParseRegistryFile: Entering with <%s>."),
             lpRegistry));


    //
    // Open the registry file
    //

    if (!ImpersonateUser(lpGPOInfo->hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to impersonate user")));
        goto Exit;
    }


    hFile = CreateFile (lpRegistry, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);

    RevertToUser(&hOldToken);

    if (hFile == INVALID_HANDLE_VALUE) {
        if ((GetLastError() == ERROR_FILE_NOT_FOUND) ||
            (GetLastError() == ERROR_PATH_NOT_FOUND))
        {
            bResult = TRUE;
            goto Exit;
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: CreateFile failed with %d"),
                     GetLastError()));
            LogEvent (TRUE, IDS_NO_REGISTRY, lpRegistry, GetLastError());
            goto Exit;
        }
    }


    //
    // Allocate buffers to hold the keyname, valuename, and data
    //

    lpKeyName = (LPWSTR) LocalAlloc (LPTR, MAX_KEYNAME_SIZE * sizeof(WCHAR));

    if (!lpKeyName)
    {
        DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to allocate memory with %d"),
                 GetLastError()));
        goto Exit;
    }


    lpValueName = (LPWSTR) LocalAlloc (LPTR, MAX_VALUENAME_SIZE * sizeof(WCHAR));

    if (!lpValueName)
    {
        DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to allocate memory with %d"),
                 GetLastError()));
        goto Exit;
    }


    //
    // Read the header block
    //
    // 2 DWORDS, signature (PReg) and version number and 2 newlines
    //

    if (!ReadFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesRead, NULL) ||
        dwBytesRead != sizeof(dwTemp))
    {
        DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read signature with %d"),
                 GetLastError()));
        goto Exit;
    }


    if (dwTemp != REGFILE_SIGNATURE)
    {
        DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Invalid file signature")));
        goto Exit;
    }


    if (!ReadFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesRead, NULL) ||
        dwBytesRead != sizeof(dwTemp))
    {
        DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read version number with %d"),
                 GetLastError()));
        goto Exit;
    }

    if (dwTemp != REGISTRY_FILE_VERSION)
    {
        DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Invalid file version")));
        goto Exit;
    }


    //
    // Read the data
    //

    while (TRUE)
    {

        //
        // Read the first character.  It will either be a [ or the end
        // of the file.
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read first character with %d"),
                         GetLastError()));
                goto Exit;
            }
            break;
        }

        if ((dwBytesRead == 0) || (chTemp != L'['))
        {
            break;
        }


        //
        // Read the keyname
        //

        lpTemp = lpKeyName;
        dwTemp = 0;

        while (dwTemp < MAX_KEYNAME_SIZE)
        {

            if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
            {
                DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read keyname character with %d"),
                         GetLastError()));
                goto Exit;
            }

            *lpTemp++ = chTemp;

            if (chTemp == TEXT('\0'))
                break;

            dwTemp++;
        }

        if (dwTemp >= MAX_KEYNAME_SIZE)
        {
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Keyname exceeded max size")));
            goto Exit;
        }


        //
        // Read the semi-colon
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read first character with %d"),
                         GetLastError()));
                goto Exit;
            }
            break;
        }

        if ((dwBytesRead == 0) || (chTemp != L';'))
        {
            break;
        }


        //
        // Read the valuename
        //

        lpTemp = lpValueName;
        dwTemp = 0;

        while (dwTemp < MAX_VALUENAME_SIZE)
        {

            if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
            {
                DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read valuename character with %d"),
                         GetLastError()));
                goto Exit;
            }

            *lpTemp++ = chTemp;

            if (chTemp == TEXT('\0'))
                break;

            dwTemp++;
        }

        if (dwTemp >= MAX_VALUENAME_SIZE)
        {
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Valuename exceeded max size")));
            goto Exit;
        }


        //
        // Read the semi-colon
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read first character with %d"),
                         GetLastError()));
                goto Exit;
            }
            break;
        }

        if ((dwBytesRead == 0) || (chTemp != L';'))
        {
            break;
        }


        //
        // Read the type
        //

        if (!ReadFile (hFile, &dwType, sizeof(DWORD), &dwBytesRead, NULL))
        {
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read type with %d"),
                     GetLastError()));
            goto Exit;
        }


        //
        // Skip semicolon
        //

        if (!ReadFile (hFile, &dwTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to skip semicolon with %d"),
                     GetLastError()));
            goto Exit;
        }


        //
        // Read the data length
        //

        if (!ReadFile (hFile, &dwDataLength, sizeof(DWORD), &dwBytesRead, NULL))
        {
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to data length with %d"),
                     GetLastError()));
            goto Exit;
        }


        //
        // Skip semicolon
        //

        if (!ReadFile (hFile, &dwTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to skip semicolon with %d"),
                     GetLastError()));
            goto Exit;
        }


        //
        // Allocate memory for data
        //

        lpData = (LPBYTE) LocalAlloc (LPTR, dwDataLength);

        if (!lpData)
        {
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to allocate memory for data with %d"),
                     GetLastError()));
            goto Exit;
        }


        //
        // Read data
        //

        if (!ReadFile (hFile, lpData, dwDataLength, &dwBytesRead, NULL))
        {
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read data with %d"),
                     GetLastError()));
            goto Exit;
        }


        //
        // Skip closing bracket
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to skip closing bracket with %d"),
                     GetLastError()));
            goto Exit;
        }

        if (chTemp != L']')
        {
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Expected to find ], but found %c"),
                     chTemp));
            goto Exit;
        }


        //
        // Call the callback function
        //

        if (!pfnRegFileCallback (lpGPOInfo, lpKeyName, lpValueName,
                                 dwType, dwDataLength, lpData))
        {
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Callback function returned false.")));
            goto Exit;
        }


        //
        // Archive the data if appropriate
        //

        if (hArchive) {
            if (!ArchiveRegistryValue(hArchive, lpKeyName, lpValueName,
                                      dwType, dwDataLength, lpData)) {
                DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: ArchiveRegistryValue returned false.")));
            }
        }

        LocalFree (lpData);
        lpData = NULL;

    }

    bResult = TRUE;

Exit:

    //
    // Finished
    //

    DebugMsg((DM_VERBOSE, TEXT("ParseRegistryFile: Leaving.")));
    if (lpData) {
        LocalFree (lpData);
    }
    if ( hFile != INVALID_HANDLE_VALUE ) {
        CloseHandle (hFile);
    }
    if ( lpKeyName ) {
        LocalFree (lpKeyName);
    }
    if ( lpValueName ) {
        LocalFree (lpValueName);
    }

    return bResult;
}


//*************************************************************
//
//  ResetRegKeySecurity
//
//  Purpose:    Resets the security on a user's key
//
//  Parameters: hKeyRoot    -   Handle to the root of the hive
//              lpKeyName   -   Subkey name
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ResetRegKeySecurity (HKEY hKeyRoot, LPTSTR lpKeyName)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD dwSize = 0;
    LONG lResult;
    HKEY hSubKey;


    RegGetKeySecurity(hKeyRoot, DACL_SECURITY_INFORMATION, pSD, &dwSize);

    if (!dwSize) {
       DebugMsg((DM_WARNING, TEXT("ResetRegKeySecurity: RegGetKeySecurity returned 0")));
       return FALSE;
    }

    pSD = LocalAlloc (LPTR, dwSize);

    if (!pSD) {
       DebugMsg((DM_WARNING, TEXT("ResetRegKeySecurity: Failed to allocate memory")));
       return FALSE;
    }


    lResult = RegGetKeySecurity(hKeyRoot, DACL_SECURITY_INFORMATION, pSD, &dwSize);
    if (lResult != ERROR_SUCCESS) {
       DebugMsg((DM_WARNING, TEXT("ResetRegKeySecurity: Failed to query key security with %d"),
                lResult));
       LocalFree (pSD);
       return FALSE;
    }


    lResult = RegOpenKeyEx(hKeyRoot,
                         lpKeyName,
                         0,
                         WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                         &hSubKey);

    if (lResult != ERROR_SUCCESS) {
       DebugMsg((DM_WARNING, TEXT("ResetRegKeySecurity: Failed to open sub key with %d"),
                lResult));
       LocalFree (pSD);
       return FALSE;
    }

    lResult = RegSetKeySecurity (hSubKey, DACL_SECURITY_INFORMATION, pSD);

    RegCloseKey (hSubKey);
    LocalFree (pSD);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("ResetRegKeySecure: Failed to set security, error = %d"), lResult));
        return FALSE;
    }

    return TRUE;

}


//*************************************************************
//
//  SetRegistryValue()
//
//  Purpose:    Callback from ParseRegistryFile that sets
//              registry policies
//
//  Parameters: lpGPOInfo   -  GPO Information
//              lpKeyName   -  Key name
//              lpValueName -  Value name
//              dwType      -  Registry data type
//              lpData      -  Registry data
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL SetRegistryValue (LPGPOINFO lpGPOInfo, LPTSTR lpKeyName,
                       LPTSTR lpValueName, DWORD dwType,
                       DWORD dwDataLength, LPBYTE lpData)
{
    DWORD dwDisp;
    HKEY hSubKey;
    LONG lResult;


    //
    // Special case some values
    //

    if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**del."), 6, lpValueName, 6) == 2)
    {
        LPTSTR lpRealValueName = lpValueName + 6;

        //
        // Delete one specific value
        //

        lResult = RegOpenKeyEx (lpGPOInfo->hKeyRoot,
                        lpKeyName, 0, KEY_WRITE, &hSubKey);

        if (lResult == ERROR_SUCCESS)
        {
            lResult = RegDeleteValue(hSubKey, lpRealValueName);

            if ((lResult == ERROR_SUCCESS) || (lResult == ERROR_FILE_NOT_FOUND))
            {
                DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Deleted value <%s>."),
                         lpRealValueName));
                if (lpGPOInfo->dwFlags & GP_VERBOSE)
                    LogEvent (FALSE, IDS_DELETED_VALUE, lpRealValueName);
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("SetRegistryValue: Failed to delete value <%s> with %d"),
                         lpRealValueName, lResult));
                LogEvent (TRUE, IDS_FAIL_DELETE_VALUE, lpRealValueName, lResult);
            }

            RegCloseKey (hSubKey);
        }
    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**delvals."), 10, lpValueName, 10) == 2)
    {

        //
        // Delete all values in the destination key
        //

        lResult = RegOpenKeyEx (lpGPOInfo->hKeyRoot,
                        lpKeyName, 0, KEY_WRITE | KEY_READ, &hSubKey);

        if (lResult == ERROR_SUCCESS)
        {

            DeleteAllValues(hSubKey);

            DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Deleted all values in <%s>."),
                     lpKeyName));
            RegCloseKey (hSubKey);
        }
    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**DeleteValues"), 14, lpValueName, 14) == 2)
    {
        TCHAR szValueName[MAX_PATH];
        LPTSTR lpName, lpNameTemp;


        //
        // Delete the values  (semi-colon separated)
        //

        lResult = RegOpenKeyEx (lpGPOInfo->hKeyRoot,
                        lpKeyName, 0, KEY_WRITE, &hSubKey);

        if (lResult == ERROR_SUCCESS)
        {
            lpName = (LPTSTR)lpData;

            while (*lpName) {

                lpNameTemp = szValueName;

                while (*lpName && *lpName == TEXT(' ')) {
                    lpName++;
                }

                while (*lpName && *lpName != TEXT(';')) {
                    *lpNameTemp++ = *lpName++;
                }

                *lpNameTemp= TEXT('\0');

                while (*lpName == TEXT(';')) {
                    lpName++;
                }


                lResult = RegDeleteValue (hSubKey, szValueName);

                if ((lResult == ERROR_SUCCESS) || (lResult == ERROR_FILE_NOT_FOUND))
                {
                    DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Deleted value <%s>."),
                             szValueName));
                    if (lpGPOInfo->dwFlags & GP_VERBOSE)
                        LogEvent (FALSE, IDS_DELETED_VALUE, szValueName);
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("SetRegistryValue: Failed to delete value <%s> with %d"),
                             szValueName, lResult));
                    LogEvent (TRUE, IDS_FAIL_DELETE_VALUE, szValueName, lResult);
                }
            }

            RegCloseKey (hSubKey);
        }

    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**DeleteKeys"), 12, lpValueName, 12) == 2)
    {
        TCHAR szKeyName[MAX_KEYNAME_SIZE];
        LPTSTR lpName, lpNameTemp, lpEnd;


        //
        // Delete keys
        //

        lpName = (LPTSTR)lpData;

        while (*lpName) {

            szKeyName[0] = TEXT('\0');
            lpNameTemp = szKeyName;

            while (*lpName && *lpName == TEXT(' ')) {
                lpName++;
            }

            while (*lpName && *lpName != TEXT(';')) {
                *lpNameTemp++ = *lpName++;
            }

            *lpNameTemp= TEXT('\0');

            while (*lpName == TEXT(';')) {
                lpName++;
            }


            if (RegDelnode (lpGPOInfo->hKeyRoot,
                        szKeyName)) {

                DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Deleted key <%s>."),
                         szKeyName));
                if (lpGPOInfo->dwFlags & GP_VERBOSE)
                    LogEvent (FALSE, IDS_DELETED_KEY, szKeyName);
            }
        }

    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**soft."), 7, lpValueName, 7) == 2)
    {

        //
        // "soft" value, only set this if it doesn't already
        // exist in destination
        //

        lResult = RegOpenKeyEx (lpGPOInfo->hKeyRoot,
                        lpKeyName, 0, KEY_WRITE, &hSubKey);

        if (lResult == ERROR_SUCCESS)
        {
            TCHAR TmpValueData[MAX_PATH+1];
            DWORD dwSize=sizeof(TmpValueData);

            lResult = RegQueryValueEx(hSubKey, lpValueName + 7,
                                      NULL,NULL,(LPBYTE) TmpValueData,
                                      &dwSize);

            RegCloseKey (hSubKey);

            if (lResult != ERROR_SUCCESS) {
                goto SetValue;
            }
        }
    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**SecureKey"), 11, lpValueName, 11) == 2)
    {
        //
        // Secure / unsecure a key (user only)
        //

        if (!(lpGPOInfo->dwFlags & GP_MACHINE))
        {
            if (*((LPDWORD)lpData) == 1)
            {
                DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Securing key <%s>."),
                         lpKeyName));

                MakeRegKeySecure(lpGPOInfo->hToken, lpGPOInfo->hKeyRoot, lpKeyName);
            }
            else
            {

                DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Unsecuring key <%s>."),
                         lpKeyName));

                ResetRegKeySecurity (lpGPOInfo->hKeyRoot, lpKeyName);
            }
        }
    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**Comment:"), 10, lpValueName, 10) == 2)
    {
        //
        // Comment - can be ignored
        //

        DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Found comment %s."),
                 (lpValueName+10)));

    }
    else
    {
SetValue:
        //
        // Save registry value
        //

        lResult = RegCreateKeyEx (lpGPOInfo->hKeyRoot,
                        lpKeyName, 0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_WRITE, NULL, &hSubKey, &dwDisp);

        if (lResult == ERROR_SUCCESS)
        {

            if ((dwType == REG_NONE) && (dwDataLength == 0) &&
                (*lpValueName == L'\0'))
            {
                lResult = ERROR_SUCCESS;
            }
            else
            {
                lResult = RegSetValueEx (hSubKey, lpValueName, 0, dwType,
                                         lpData, dwDataLength);
            }

            RegCloseKey (hSubKey);

            if (lResult == ERROR_SUCCESS)
            {
                switch (dwType) {
                    case REG_SZ:
                    case REG_EXPAND_SZ:
                        DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: %s => %s  [OK]"),
                                 lpValueName, (LPTSTR)lpData));
                        if (lpGPOInfo->dwFlags & GP_VERBOSE)
                            LogEvent (FALSE, IDS_SET_STRING_VALUE, lpValueName, (LPTSTR)lpData);
                        break;

                    case REG_DWORD:
                        DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: %s => %d  [OK]"),
                                 lpValueName, *((LPDWORD)lpData)));
                        if (lpGPOInfo->dwFlags & GP_VERBOSE)
                            LogEvent (FALSE, IDS_SET_DWORD_VALUE, lpValueName, (DWORD)*lpData);
                        break;

                    case REG_NONE:
                        break;

                    default:
                        DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: %s was set successfully"),
                                 lpValueName));
                        if (lpGPOInfo->dwFlags & GP_VERBOSE)
                            LogEvent (FALSE, IDS_SET_UNKNOWN_VALUE, lpValueName);
                        break;
                }


                if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                   TEXT("Control Panel\\Colors"), 20, lpKeyName, 20) == 2) {
                    lpGPOInfo->dwFlags |= GP_REGPOLICY_CPANEL;

                } else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                   TEXT("Control Panel\\Desktop"), 21, lpKeyName, 21) == 2) {
                    lpGPOInfo->dwFlags |= GP_REGPOLICY_CPANEL;
                }


            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("SetRegistryValue: Failed to set value <%s> with %d"),
                         lpValueName, lResult));
                LogEvent (TRUE, IDS_FAILED_SET, lpValueName, lResult);
            }

        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("SetRegistryValue: Failed to open key <%s> with %d"),
                     lpKeyName, lResult));
            LogEvent (TRUE, IDS_FAILED_CREATE, lpKeyName, lResult);
        }
    }


    return TRUE;
}


//*************************************************************
//
//  ProcessGPORegistryPolicy()
//
//  Purpose:    Proceses GPO registry policy
//
//  Parameters: lpGPOInfo       -   GPO information
//              pChangedGPOList - Link list of changed GPOs
//
//  Notes:      This function is called in the context of
//              local system, which allows us to create the
//              directory, write to the file etc.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ProcessGPORegistryPolicy (LPGPOINFO lpGPOInfo,
                               PGROUP_POLICY_OBJECT pChangedGPOList)
{
    PGROUP_POLICY_OBJECT lpGPO;
    TCHAR szPath[MAX_PATH];
    TCHAR szBuffer[MAX_PATH];
    TCHAR szKeyName[100];
    LPTSTR lpEnd, lpGPOComment;
    HANDLE hFile;
    DWORD dwTemp, dwBytesWritten;


    //
    // Get the path name to the appropriate profile
    //

    szPath[0] = TEXT('\0');
    dwTemp = ARRAYSIZE(szPath);

    if (lpGPOInfo->dwFlags & GP_MACHINE) {
        GetAllUsersProfileDirectoryEx(szPath, &dwTemp, TRUE);
    } else {
        GetUserProfileDirectory(lpGPOInfo->hToken, szPath, &dwTemp);
    }

    if (szPath[0] == TEXT('\0')) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: Failed to get path to profile root")));
        return FALSE;
    }


    //
    // Tack on the archive file name
    //

    DmAssert( lstrlen(szPath) + lstrlen(TEXT("\\ntuser.pol")) < MAX_PATH );

    lstrcat (szPath, TEXT("\\ntuser.pol"));


    //
    // Delete any existing policies
    //

    if (!ResetPolicies (lpGPOInfo, szPath)) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: ResetPolicies failed.")));
        return FALSE;
    }


    //
    // Delete the old archive file
    //

    SetFileAttributes (szPath, FILE_ATTRIBUTE_NORMAL);
    DeleteFile (szPath);


    //
    // Recreate the archive file
    //

    hFile = CreateFile (szPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);


    if (hFile == INVALID_HANDLE_VALUE)
    {
        DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: Failed to create archive file with %d"),
                 GetLastError()));
        return FALSE;
    }

    //
    // Set the header information in the archive file
    //

    dwTemp = REGFILE_SIGNATURE;

    if (!WriteFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(dwTemp))
    {
        DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: Failed to write signature with %d"),
                 GetLastError()));
        CloseHandle (hFile);
        return FALSE;
    }


    dwTemp = REGISTRY_FILE_VERSION;

    if (!WriteFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(dwTemp))
    {
        DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: Failed to write version number with %d"),
                 GetLastError()));
        CloseHandle (hFile);
        return FALSE;
    }


    //
    // Now loop through the GPOs applying the registry.pol files
    //

    lpGPO = pChangedGPOList;

    while ( lpGPO ) {

        //
        // Add the source GPO comment
        //

        lpGPOComment = LocalAlloc (LPTR, (lstrlen(lpGPO->lpDisplayName) + 25) * sizeof(TCHAR));

        if (lpGPOComment) {

            lstrcpy (szKeyName, TEXT("Software\\Policies\\Microsoft\\Windows\\Group Policy Objects\\"));
            lstrcat (szKeyName, lpGPO->szGPOName);

            lstrcpy (lpGPOComment, TEXT("**Comment:GPO Name: "));
            lstrcat (lpGPOComment, lpGPO->lpDisplayName);

            if (!ArchiveRegistryValue(hFile, szKeyName, lpGPOComment, REG_SZ, 0, NULL)) {
                DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: ArchiveRegistryValue returned false.")));
            }

            LocalFree (lpGPOComment);
        }


        //
        // Build the path to registry.pol
        //

        DmAssert( lstrlen(lpGPO->lpFileSysPath) + lstrlen(c_szRegistryPol) + 1 < MAX_PATH );

        lstrcpy (szBuffer, lpGPO->lpFileSysPath);
        lpEnd = CheckSlash (szBuffer);
        lstrcpy (lpEnd, c_szRegistryPol);


        if (!ParseRegistryFile (lpGPOInfo, szBuffer, SetRegistryValue, hFile)) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: ParseRegistryFile failed.")));
            CloseHandle (hFile);
            return FALSE;
        }

        lpGPO = lpGPO->pNext;
    }

    CloseHandle (hFile);

#if 0 //bugbug:  need to rethink
    //
    // Set the security on the file
    //

    if (!MakeFileSecure (szPath, 0)) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: Failed to set security on the group policy registry file with %d"),
                 GetLastError()));
    }
#endif

    return TRUE;
}


//*************************************************************
//
//  RefreshDisplay()
//
//  Purpose:    Starts control.exe
//
//  Parameters: lpGPOInfo   -   GPT information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL RefreshDisplay (LPGPOINFO lpGPOInfo)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR szCmdLine[50];
    BOOL Result;
    HANDLE hOldToken;



    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("RefreshDisplay: Starting control.exe")));


    //
    // Initialize process startup info
    //

    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;
    si.lpTitle = NULL;
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.dwFlags = 0;
    si.wShowWindow = SW_HIDE;
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;
    si.lpDesktop = TEXT("");


    //
    // Impersonate the user so we get access checked correctly on
    // the file we're trying to execute
    //

    if (!ImpersonateUser(lpGPOInfo->hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("RefreshDisplay: Failed to impersonate user")));
        return FALSE;
    }


    //
    // Create the app
    //

    lstrcpy (szCmdLine, TEXT("control /policy"));

    Result = CreateProcessAsUser(lpGPOInfo->hToken, NULL, szCmdLine, NULL,
                                 NULL, FALSE, 0, NULL, NULL, &si, &pi);


    //
    // Revert to being 'ourself'
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("RefreshDisplay: Failed to revert to self")));
    }


    if (Result) {
        WaitForSingleObject (pi.hProcess, 120000);
        CloseHandle (pi.hThread);
        CloseHandle (pi.hProcess);

    } else {
        DebugMsg((DM_WARNING, TEXT("RefreshDisplay: Failed to start control.exe with %d"), GetLastError()));
    }

    return(Result);

}


//*************************************************************
//
//  IsSlowLink()
//
//  Purpose:    Determines if the connection to the specified
//              server is a slow link or not
//
//  Parameters: hKeyRoot     -  Registry hive root
//              lpDCAddress  -  Server address in string form
//              bSlow        -  Receives slow link status
//
//  Return:     TRUE if slow link
//              FALSE if not
//
//*************************************************************

DWORD IsSlowLink (HKEY hKeyRoot, LPTSTR lpDCAddress, BOOL *bSlow)
{
    DWORD dwSize, dwType, dwResult;
    HKEY hKey;
    LONG lResult;
    ULONG ulSpeed, ulTransferRate;
    IPAddr ipaddr;
    LPSTR lpDCAddressA, lpTemp;
    PWSOCK32_API pWSock32;


    //
    // Set default
    //

    *bSlow = FALSE;


    //
    // Get the slow link detection flag, and slow link timeout.
    //

    ulTransferRate = SLOW_LINK_TRANSFER_RATE;

    lResult = RegOpenKeyEx(hKeyRoot,
                           WINLOGON_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(ulTransferRate);
        RegQueryValueEx (hKey,
                         TEXT("GroupPolicyMinTransferRate"),
                         NULL,
                         &dwType,
                         (LPBYTE) &ulTransferRate,
                         &dwSize);

        RegCloseKey (hKey);
    }


    lResult = RegOpenKeyEx(hKeyRoot,
                           SYSTEM_POLICIES_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(ulTransferRate);
        RegQueryValueEx (hKey,
                         TEXT("GroupPolicyMinTransferRate"),
                         NULL,
                         &dwType,
                         (LPBYTE) &ulTransferRate,
                         &dwSize);

        RegCloseKey (hKey);
    }


    //
    // If the transfer rate is 0, then always download policy
    //

    if (!ulTransferRate) {
        DebugMsg((DM_VERBOSE, TEXT("IsSlowLink: Slow link transfer rate is 0.  Always download policy.")));
        return ERROR_SUCCESS;
    }


    //
    // Convert the ipaddress from string form to ulong format
    //

    dwSize = lstrlen (lpDCAddress) + 1;

    lpDCAddressA = LocalAlloc (LPTR, dwSize);

    if (!lpDCAddressA) {
        DebugMsg((DM_WARNING, TEXT("IsSlowLink: Failed to allocate memory.")));
        return GetLastError();
    }

#ifdef UNICODE

    if (!WideCharToMultiByte(CP_ACP, 0, lpDCAddress, -1, lpDCAddressA, dwSize, NULL, NULL)) {
        LocalFree(lpDCAddressA);
        DebugMsg((DM_WARNING, TEXT("IsSlowLink: WideCharToMultiByte failed with %d"), GetLastError()));
        return GetLastError();
    }

#else

    lstrcpy (lpDCAddressA, lpDCAddress);

#endif

    pWSock32 = LoadWSock32();

    if ( !pWSock32 ) {
        LocalFree(lpDCAddressA);
        DebugMsg((DM_WARNING, TEXT("IsSlowLink: Failed to load wsock32.dll with %d"), GetLastError()));
        return GetLastError();
    }


    if ((*lpDCAddressA == TEXT('\\')) && (*(lpDCAddressA+1) == TEXT('\\'))) {
        lpTemp = lpDCAddressA+2;
    } else {
        lpTemp = lpDCAddressA;
    }

    ipaddr = pWSock32->pfninet_addr (lpTemp);


    //
    // Ping the computer
    //

    dwResult = PingComputer (ipaddr, &ulSpeed);


    if (dwResult == ERROR_SUCCESS) {

        if (ulSpeed) {

            //
            // If the delta time is greater that the timeout time, then this
            // is a slow link.
            //

            if (ulSpeed < ulTransferRate) {
                *bSlow = TRUE;
            }
        }
    }

    LocalFree (lpDCAddressA);

    return dwResult;
}

//*************************************************************
//
//  RefreshPolicy()
//
//  Purpose:    External api that causes policy to be refreshed now
//
//  Parameters: bMachine -   Machine policy vs user policy
//
//  Return:     TRUE if successful
//              FALSE if not
//
//*************************************************************

BOOL WINAPI RefreshPolicy (BOOL bMachine)
{
    HANDLE hEvent;

    DebugMsg((DM_VERBOSE, TEXT("RefreshPolicy: Entering with %d"), bMachine));

    hEvent = OpenEvent (EVENT_MODIFY_STATE, FALSE,
                        bMachine ? MACHINE_POLICY_REFRESH_EVENT : USER_POLICY_REFRESH_EVENT);

    if (hEvent) {
        SetEvent (hEvent);
        CloseHandle (hEvent);
    } else {
        DebugMsg((DM_VERBOSE, TEXT("RefreshPolicy: Failed to open event with %d"), GetLastError()));
    }

    DebugMsg((DM_VERBOSE, TEXT("RefreshPolicy: Leaving.")));

    return TRUE;
}

//*************************************************************
//
//  EnterCriticalPolicySection()
//
//  Purpose:    External api that causes policy to pause
//              This allows an application to pause policy
//              so that values don't change while it reads
//              the settings.
//
//  Parameters: bMachine -   Pause machine policy or user policy
//
//  Return:     TRUE if successful
//              FALSE if not
//
//*************************************************************

HANDLE WINAPI EnterCriticalPolicySection (BOOL bMachine)
{
    HANDLE hSection;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;


    //
    // Create / open the mutex
    //

    InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );

    SetSecurityDescriptorDacl (
                    &sd,
                    TRUE,                           // Dacl present
                    NULL,                           // NULL Dacl
                    FALSE                           // Not defaulted
                    );

    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;
    sa.nLength = sizeof(sa);

    hSection = CreateMutex (&sa, FALSE,
                           (bMachine ? MACHINE_POLICY_MUTEX : USER_POLICY_MUTEX));

    if (!hSection) {
        DebugMsg((DM_WARNING, TEXT("EnterCriticalPolicySection: Failed to create mutex with %d"),
                 GetLastError()));
        return NULL;
    }


    //
    // Claim the mutex
    //
    // Max wait time is 10 minutes
    //

    if ((WaitForSingleObject (hSection, 600000) == WAIT_FAILED)) {
        DebugMsg((DM_WARNING, TEXT("EnterCriticalPolicySection: Failed to wait on the mutex.  Error = %d."),
                  GetLastError()));
        CloseHandle( hSection );
        return NULL;
    }

    DebugMsg((DM_VERBOSE, TEXT("EnterCriticalPolicySection: %s critical section has been claimed.  Handle = 0x%x"),
             (bMachine ? TEXT("Machine") : TEXT("User")), hSection));

    return hSection;
}


//*************************************************************
//
//  LeaveCriticalPolicySection()
//
//  Purpose:    External api that causes policy to resume
//              This api assumes the app has called
//              EnterCriticalPolicySection first
//
//  Parameters: hSection - mutex handle
//
//  Return:     TRUE if successful
//              FALSE if not
//
//*************************************************************

BOOL WINAPI LeaveCriticalPolicySection (HANDLE hSection)
{

    if (!hSection) {
        DebugMsg((DM_WARNING, TEXT("LeaveCriticalPolicySection: null mutex handle.")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ReleaseMutex (hSection);
    CloseHandle (hSection);

    DebugMsg((DM_VERBOSE, TEXT("LeaveCriticalPolicySection: Critical section 0x%x has been released."),
             hSection));

    return TRUE;
}


//*************************************************************
//
//  AddGPO()
//
//  Purpose:    Adds a GPO to the list
//
//  Parameters: lpGPOList        - list of GPOs
//              dwOptions        - Options
//              dwVersion        - Version number
//              lpDSPath         - DS path
//              lpFileSysPath    - File system path
//              lpDisplayName    - Friendly display name
//              lpGPOName        - GPO name
//              lpExtensions     - Extensions relevant to this GPO
//              GPOLink          - GPO link type
//              lpLink       - SDOU this GPO is linked to
//              lParam           - lParam
//              bFront           - Head or end of list
//              bBlock           - Block from above flag
//              bVerbose         - Verbose output flag
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL AddGPO (PGROUP_POLICY_OBJECT * lpGPOList, DWORD dwOptions,
             DWORD dwVersion, LPTSTR lpDSPath, LPTSTR lpFileSysPath,
             LPTSTR lpDisplayName, LPTSTR lpGPOName, LPTSTR lpExtensions,
             GPO_LINK GPOLink, LPTSTR lpLink, LPARAM lParam,
             BOOL bFront, BOOL bBlock, BOOL bVerbose)
{
    PGROUP_POLICY_OBJECT lpNew, lpTemp;
    DWORD dwSize;


    //
    // Check if this item should be excluded from the list
    //

    if (bBlock) {
        if (!(dwOptions & GPO_FLAG_FORCE)) {
            DebugMsg((DM_VERBOSE, TEXT("AddGPO:  GPO %s will not be added to the list since the Block flag is set and this GPO is not in enforce mode."),
                     lpDisplayName));
            if (bVerbose) LogEvent (FALSE, IDS_SKIP_GPO, lpDisplayName);
            return TRUE;
        }
    }


    //
    // Calculate the size of the new GPO item
    //

    dwSize = sizeof (GROUP_POLICY_OBJECT);

    if (lpDSPath) {
        dwSize += ((lstrlen(lpDSPath) + 1) * sizeof(TCHAR));
    }

    if (lpFileSysPath) {
        dwSize += ((lstrlen(lpFileSysPath) + 1) * sizeof(TCHAR));
    }

    if (lpDisplayName) {
        dwSize += ((lstrlen(lpDisplayName) + 1) * sizeof(TCHAR));
    }

    if (lpExtensions) {
        dwSize += ((lstrlen(lpExtensions) + 1) * sizeof(TCHAR));
    }

    if (lpLink) {
        dwSize += ((lstrlen(lpLink) + 1) * sizeof(TCHAR));
    }


    //
    // Allocate space for it
    //

    lpNew = (PGROUP_POLICY_OBJECT) LocalAlloc (LPTR, dwSize);

    if (!lpNew) {
        DebugMsg((DM_WARNING, TEXT("AddGPO: Failed to allocate memory with %d"),
                 GetLastError()));
        return FALSE;
    }


    //
    // Fill in item
    //

    lpNew->dwOptions = dwOptions;
    lpNew->dwVersion = dwVersion;

    if (lpDSPath) {
        lpNew->lpDSPath = (LPTSTR)(((LPBYTE)lpNew) + sizeof(GROUP_POLICY_OBJECT));
        lstrcpy (lpNew->lpDSPath, lpDSPath);
    }

    if (lpFileSysPath) {
        if (lpDSPath) {
            lpNew->lpFileSysPath = lpNew->lpDSPath + lstrlen (lpNew->lpDSPath) + 1;
        } else {
            lpNew->lpFileSysPath = (LPTSTR)(((LPBYTE)lpNew) + sizeof(GROUP_POLICY_OBJECT));
        }

        lstrcpy (lpNew->lpFileSysPath, lpFileSysPath);
    }


    if (lpDisplayName) {
        if (lpFileSysPath) {
            lpNew->lpDisplayName = lpNew->lpFileSysPath + lstrlen (lpNew->lpFileSysPath) + 1;
        } else {

            if (lpDSPath)
            {
                lpNew->lpDisplayName = lpNew->lpDSPath + lstrlen (lpNew->lpDSPath) + 1;
            }
            else
            {
                lpNew->lpDisplayName = (LPTSTR)(((LPBYTE)lpNew) + sizeof(GROUP_POLICY_OBJECT));
            }
        }

        lstrcpy (lpNew->lpDisplayName, lpDisplayName);
    }


    if (lpGPOName) {
        DmAssert( lstrlen(lpGPOName) < 50 );
        lstrcpy (lpNew->szGPOName, lpGPOName);
    }

    if (lpExtensions) {
        if (lpDisplayName) {
            lpNew->lpExtensions = lpNew->lpDisplayName + lstrlen(lpNew->lpDisplayName) + 1;
        } else {

            if (lpFileSysPath) {
                lpNew->lpExtensions = lpNew->lpFileSysPath + lstrlen(lpNew->lpFileSysPath) + 1;
            } else {

                if (lpDSPath) {
                    lpNew->lpExtensions = lpNew->lpDSPath + lstrlen(lpNew->lpDSPath) + 1;
                } else {
                    lpNew->lpExtensions = (LPTSTR)(((LPBYTE)lpNew) + sizeof(GROUP_POLICY_OBJECT));
                }

            }
        }

        lstrcpy (lpNew->lpExtensions, lpExtensions);
    }

    if (lpLink) {
        if (lpExtensions) {
            lpNew->lpLink = lpNew->lpExtensions + lstrlen(lpNew->lpExtensions) + 1;
        } else {
            if (lpDisplayName) {
                lpNew->lpLink = lpNew->lpDisplayName + lstrlen(lpNew->lpDisplayName) + 1;
            } else {

                if (lpFileSysPath) {
                    lpNew->lpLink = lpNew->lpFileSysPath + lstrlen(lpNew->lpFileSysPath) + 1;
                } else {

                    if (lpDSPath) {
                        lpNew->lpLink = lpNew->lpDSPath + lstrlen(lpNew->lpDSPath) + 1;
                    } else {
                        lpNew->lpLink = (LPTSTR)(((LPBYTE)lpNew) + sizeof(GROUP_POLICY_OBJECT));
                    }
                }
            }
        }

        lstrcpy (lpNew->lpLink, lpLink);
    }


    lpNew->GPOLink = GPOLink;
    lpNew->lParam = lParam;

    //
    // Add item to link list
    //

    if (*lpGPOList) {

        if (bFront) {

            (*lpGPOList)->pPrev = lpNew;
            lpNew->pNext = *lpGPOList;
            *lpGPOList = lpNew;

        } else {

            lpTemp = *lpGPOList;

            while (lpTemp->pNext != NULL) {
                lpTemp = lpTemp->pNext;
            }

            lpTemp->pNext = lpNew;
            lpNew->pPrev = lpTemp;
        }

    } else {

        //
        // First item in the list
        //

        *lpGPOList = lpNew;
    }

    return TRUE;
}


//*************************************************************
//
//  CheckGPOAccess()
//
//  Purpose:    Determines if the user / machine has read access to
//              the GPO and if so, checks the Apply Group Policy
//              extended right to see if the GPO should be applied.
//              Also retrieves GPO attributes.
//
//  Parameters: pld             -  LDAP connection
//              pLDAP           -  LDAP function table pointer
//              pMessage        -  LDAP message
//              lpSDProperty    -  Security descriptor property name
//              dwFlags         -  GetGPOList flags
//              hToken          -  User / machine token
//              pbAccessGranted -  Receives the final yes / no status
//
//  Return:     TRUE if successful
//              FALSE if an error occurs.
//
//*************************************************************

BOOL CheckGPOAccess (PLDAP pld, PLDAP_API pLDAP, HANDLE hToken, PLDAPMessage pMessage,
                     LPTSTR lpSDProperty, DWORD dwFlags,
                     BOOL *pbAccessGranted)
{
    BOOL bResult = FALSE;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PWSTR *ppwszValues = NULL;
    PLDAP_BERVAL *pSize = NULL;
    OBJECT_TYPE_LIST ObjType[2];
    PRIVILEGE_SET PrivSet;
    DWORD PrivSetLength = sizeof(PRIVILEGE_SET);
    DWORD dwGrantedAccess;
    BOOL bAccessStatus = TRUE;
    GUID GroupPolicyContainer = {0x31B2F340, 0x016D, 0x11D2,
                                 0x94, 0x5F, 0x00, 0xC0, 0x4F, 0xB9, 0x84, 0xF9};
    GUID ApplyGroupPolicy = {0xedacfd8f, 0xffb3, 0x11d1,
                             0xb4, 0x1d, 0x00, 0xa0, 0xc9, 0x68, 0xf9, 0x39};
    GENERIC_MAPPING DS_GENERIC_MAPPING = { GENERIC_READ_MAPPING, GENERIC_WRITE_MAPPING,
                                           GENERIC_EXECUTE_MAPPING, GENERIC_ALL_MAPPING };

    //
    // Set the default return value
    //

    *pbAccessGranted = FALSE;


    //
    // Get the security descriptor value
    //

    ppwszValues = pLDAP->pfnldap_get_values(pld, pMessage, lpSDProperty);


    if (!ppwszValues) {
        if (pld->ld_errno == LDAP_NO_SUCH_ATTRIBUTE) {
            DebugMsg((DM_VERBOSE, TEXT("CheckGPOAccess:  Object can not be accessed.")));
            bResult = TRUE;
        }
        else {
            DebugMsg((DM_WARNING, TEXT("CheckGPOAccess:  ldap_get_values failed with 0x%x"),
                 pld->ld_errno));
        }

        goto Exit;
    }


    //
    // Get the length of the security descriptor
    //

    pSize = pLDAP->pfnldap_get_values_len(pld, pMessage, lpSDProperty);

    if (!pSize) {
        DebugMsg((DM_WARNING, TEXT("CheckGPOAccess:  ldap_get_values_len failed with 0x%x"),
                 pld->ld_errno));
        goto Exit;
    }


    //
    // Allocate the memory for the security descriptor
    //

    pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, (*pSize)->bv_len);

    if (!pSD) {
        DebugMsg((DM_WARNING, TEXT("CheckGPOAccess:  Failed to allocate memory for SD with  %d"),
                 GetLastError()));
        goto Exit;
    }


    //
    // Copy the security descriptor
    //

    CopyMemory(pSD, (PBYTE)(*pSize)->bv_val, (*pSize)->bv_len);


    //
    // Now we use AccessCheckByType to determine if the user / machine
    // should have this GPO applied to them
    //
    //
    // Prepare the object type array
    //

    ObjType[0].Level = ACCESS_OBJECT_GUID;
    ObjType[0].Sbz = 0;
    ObjType[0].ObjectType = &GroupPolicyContainer;

    ObjType[1].Level = ACCESS_PROPERTY_SET_GUID;
    ObjType[1].Sbz = 0;
    ObjType[1].ObjectType = &ApplyGroupPolicy;


    //
    // Check access
    //

    if (!AccessCheckByType (pSD, NULL, hToken, MAXIMUM_ALLOWED, ObjType, 2,
                        &DS_GENERIC_MAPPING, &PrivSet, &PrivSetLength,
                        &dwGrantedAccess, &bAccessStatus)) {
        DebugMsg((DM_WARNING, TEXT("CheckGPOAccess:  AccessCheckByType failed with  %d"), GetLastError()));
        goto Exit;
    }


    //
    // Check for the control bit
    //

    if (bAccessStatus && (dwGrantedAccess & ACTRL_DS_CONTROL_ACCESS)) {
        *pbAccessGranted = TRUE;
    }


    bResult = TRUE;

Exit:

    if (pSize) {
        pLDAP->pfnldap_value_free_len(pSize);
    }

    if (ppwszValues) {
        pLDAP->pfnldap_value_free(ppwszValues);
    }

    if (pSD) {
        LocalFree (pSD);
    }

    return bResult;
}

//*************************************************************
//
//  ProcessGPO()
//
//  Purpose:    Processes a specific GPO
//
//  Parameters: lpGPOPath     - Path to the GPO
//              dwFlags       - GetGPOList flags
//              HANDLE        - user or machine aceess token
//              lpGPOList     - List of GPOs
//              dwGPOOptions  - Link options
//              bDeferred     - Should ldap query be deferred ?
//              bVerbose      - Verbose output
//              GPOLink       - GPO link type
//              lpDSObject    - SDOU this gpo is linked to
//              pld           - LDAP info
//              pLDAP         - LDAP api
//              pLdapMsg      - LDAP message
//              bBlock        - Block flag
//              hToken        - User / machine token
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ProcessGPO (LPTSTR lpGPOPath, DWORD dwFlags, HANDLE hToken, PGROUP_POLICY_OBJECT *lpGPOList,
                 DWORD dwGPOOptions, BOOL bDeferred, BOOL bVerbose, GPO_LINK GPOLink, LPTSTR lpDSObject,
                 PLDAP  pld, PLDAP_API pLDAP, PLDAPMessage pMessage, BOOL bBlock)
{
    ULONG ulResult, i;
    BOOL bResult = FALSE;
    BOOL bFound = FALSE;
    BOOL bOwnLdapMsg = FALSE;  // LDAP message owned by us (if true) or caller (if false)
    BOOL bAccessGranted;
    DWORD dwFunctionalityVersion = 2;
    DWORD dwVersion = 0;
    DWORD dwGPOFlags = 0;
    DWORD dwGPTVersion = 0;
    TCHAR szGPOName[80];
    TCHAR *pszGPTPath = 0;
    TCHAR *pszFriendlyName = 0;
    LPTSTR lpPath, lpEnd, lpTemp;
    TCHAR *pszExtensions = 0;
    TCHAR szLDAP[] = TEXT("LDAP://");
    INT iStrLen = lstrlen(szLDAP);
    BYTE berValue[8];
    LDAPControl SeInfoControl = { LDAP_SERVER_SD_FLAGS_OID_W, { 5, (PCHAR)berValue }, TRUE };
    LDAPControl referralControl = { LDAP_SERVER_DOMAIN_SCOPE_OID_W, { 0, NULL}, TRUE };
    PLDAPControl ServerControls[] = { &SeInfoControl, &referralControl, NULL };
    TCHAR szSDProperty[] = TEXT("nTSecurityDescriptor");
    TCHAR szCommonName[] = TEXT("cn");
    TCHAR szDisplayName[] = TEXT("displayName");
    TCHAR szFileSysPath[] = TEXT("gPCFileSysPath");
    TCHAR szVersion[] = TEXT("versionNumber");
    TCHAR szFunctionalityVersion[] = GPO_FUNCTIONALITY_VERSION;
    TCHAR szFlags[] = TEXT("flags");
    PWSTR rgAttribs[11] = { szSDProperty,
                           szFileSysPath,
                           szCommonName,
                           szDisplayName,
                           szVersion,
                           szFunctionalityVersion,
                           szFlags,
                           GPO_MACHEXTENSION_NAMES,
                           GPO_USEREXTENSION_NAMES,
                           szObjectClass,
                           NULL };
    LPTSTR *lpValues;


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  ==============================")));

    //
    // Skip the starting LDAP provider if found
    //

    if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       lpGPOPath, iStrLen, szLDAP, iStrLen) == CSTR_EQUAL)
    {
        lpPath = lpGPOPath + iStrLen;
    }
    else
    {
        lpPath = lpGPOPath;
    }

    if ( bDeferred ) {

        bResult = AddGPO (lpGPOList, dwGPOOptions, 0, lpPath,
                          0, 0, 0, 0, GPOLink, lpDSObject, 0,
                          FALSE, bBlock, bVerbose);
        if (!bResult)
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Failed to add GPO <%s> to the list."), lpPath));

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Deferring search for <%s>"), lpGPOPath));

        return bResult;

    }

    DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Searching <%s>"), lpGPOPath));

    if ( pMessage == NULL ) {

        bOwnLdapMsg = TRUE;

        //
        // Setup the BER encoding
        //

        berValue[0] = 0x30;
        berValue[1] = 0x03;
        berValue[2] = 0x02; // denotes an integer
        berValue[3] = 0x01; // denotes size
        berValue[4] = (BYTE)((DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION) & 0xF);


        //
        // Search for the GPO
        //

        ulResult = pLDAP->pfnldap_search_ext_s(pld, lpPath, LDAP_SCOPE_BASE,
                                               szDSClassAny, rgAttribs, 0,
                                               (PLDAPControl*)ServerControls,
                                               NULL, NULL, 0x10000, &pMessage);


        //
        // If the search fails, store the error code and return
        //

        if (ulResult != LDAP_SUCCESS) {

            if (ulResult == LDAP_NO_SUCH_ATTRIBUTE) {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Object can not be accessed.")));
                if (bVerbose) LogEvent (FALSE, IDS_NO_ACCESS, lpGPOPath);
                bResult = TRUE;

            } else if (ulResult == LDAP_NO_SUCH_OBJECT) {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Object <%s> does not exist."), lpGPOPath));
                if (bVerbose) LogEvent (FALSE, IDS_GPO_DELETED, lpGPOPath);
                bResult = TRUE;

            } else {
                DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Failed to search for <%s> with error %d"), lpGPOPath, ulResult));
                LogEvent (TRUE, IDS_OBJECT_NOT_FOUND, lpGPOPath, ulResult);
            }

            goto Exit;
        }


        //
        // If the search succeeds, but the message is empty,
        // store the error code and return
        //

        if (!pMessage) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Search for <%s> returned and empty message structure.  Error = %d"),
                     lpGPOPath, pld->ld_errno));
            LogEvent (TRUE, IDS_OBJECT_NOT_FOUND, lpGPOPath, pld->ld_errno);
            goto Exit;
        }

    }

    //
    // Check if this user or machine has access to the GPO, and if so,
    // should that GPO be applied to them.
    //

    if (!CheckGPOAccess (pld, pLDAP, hToken, pMessage, szSDProperty, dwFlags, &bAccessGranted)) {

        DebugMsg((DM_WARNING, TEXT("ProcessGPO:  CheckGPOAccess failed for <%s>"), lpGPOPath));
        LogEvent (TRUE, IDS_FAILED_ACCESS_CHECK, lpGPOPath, GetLastError());

        goto Exit;
    }

    if (!bAccessGranted) {
        if (dwFlags & GPO_LIST_FLAG_MACHINE) {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Machine does not have access to the GPO and so will not be applied.")));
        } else {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  User does not have access to the GPO and so will not be applied.")));
        }
        if (bVerbose) LogEvent (FALSE, IDS_NO_ACCESS, lpGPOPath);
        bResult = TRUE;
        goto Exit;
    }


    if (dwFlags & GPO_LIST_FLAG_MACHINE) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Machine has access to this GPO.")));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  User has access to this GPO.")));
    }


    //
    // User has access to this GPO, now retrieve remaining GPO attributes
    //

    //
    // Check if this object is a GPO
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage, szObjectClass);

    if (lpValues) {

        bFound = FALSE;
        for ( i=0; lpValues[i] != NULL; i++) {
            if ( lstrcmp( lpValues[i], szDSClassGPO ) == 0 ) {
                bFound = TRUE;
                break;
            }
        }

        pLDAP->pfnldap_value_free (lpValues);

        if ( !bFound ) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Object <%s> is not a GPO"), lpGPOPath ));
            LogEvent (TRUE, IDS_INCORRECT_CLASS, lpGPOPath, szDSClassGPO);

            goto Exit;
        }

    }

    //
    // In the results, get the values that match the gPCFunctionalityVersion attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage, szFunctionalityVersion);

    if (lpValues) {

        dwFunctionalityVersion = StringToInt (*lpValues);
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found functionality version of:  %d"),
                 dwFunctionalityVersion));
        pLDAP->pfnldap_value_free (lpValues);

    } else {

        ulResult = pLDAP->pfnLdapGetLastError();

        if (ulResult == LDAP_NO_SUCH_ATTRIBUTE) {
            if (dwFlags & GPO_LIST_FLAG_MACHINE) {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Machine does not have access to <%s>"), lpGPOPath));
            } else {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  User does not have access to <%s>"), lpGPOPath));
            }
            if (bVerbose) LogEvent (FALSE, IDS_NO_ACCESS, lpGPOPath);
            bResult = TRUE;

        } else {
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  GPO %s does not have a functionality version number, error = 0x%x."), lpGPOPath, ulResult));
            LogEvent (TRUE, IDS_CORRUPT_GPO_FUNCVERSION, lpGPOPath);
        }
        goto Exit;
    }


    //
    // In the results, get the values that match the gPCFileSystemPath attribute
    //

    lpValues = pLDAP->pfnldap_get_values (pld, pMessage, szFileSysPath);

    if (lpValues) {


        pszGPTPath = LocalAlloc( LPTR, (lstrlen(*lpValues) +lstrlen(TEXT("\\Machine")) +1) * sizeof(TCHAR) );
        if ( pszGPTPath == 0) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Unable to allocate memory")));
            pLDAP->pfnldap_value_free (lpValues);
            goto Exit;
        }

        lstrcpy (pszGPTPath, *lpValues);
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found file system path of:  <%s>"), pszGPTPath));
        pLDAP->pfnldap_value_free (lpValues);

        lpEnd = CheckSlash (pszGPTPath);

        //
        // Get the GPT version number
        //

        lstrcpy (lpEnd, TEXT("gpt.ini"));
        dwGPTVersion = GetPrivateProfileInt(TEXT("General"), TEXT("Version"), 0, pszGPTPath);

        if (dwFlags & GPO_LIST_FLAG_MACHINE) {
            lstrcpy (lpEnd, TEXT("Machine"));
        } else {
            lstrcpy (lpEnd, TEXT("User"));
        }

    } else {
        ulResult = pLDAP->pfnLdapGetLastError();

        if (ulResult == LDAP_NO_SUCH_ATTRIBUTE) {
            if (dwFlags & GPO_LIST_FLAG_MACHINE) {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Machine does not have access to <%s>"), lpGPOPath));
            } else {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  User does not have access to <%s>"), lpGPOPath));
            }
            if (bVerbose) LogEvent (FALSE, IDS_NO_ACCESS, lpGPOPath);
            bResult = TRUE;

        } else {
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  GPO %s does not have a file system path, error = 0x%x."), lpGPOPath, ulResult));
            LogEvent (TRUE, IDS_CORRUPT_GPO_FSPATH, lpGPOPath);
        }
        goto Exit;
    }


    //
    // In the results, get the values that match the common name attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage, szCommonName);

    if (lpValues) {

        DmAssert( lstrlen(*lpValues) < 80 );

        lstrcpy (szGPOName, *lpValues);
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found common name of:  <%s>"), szGPOName));
        pLDAP->pfnldap_value_free (lpValues);

    } else {
        ulResult = pLDAP->pfnLdapGetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPO:  GPO %s does not have a common name (a GUID)."), lpGPOPath));
        LogEvent (TRUE, IDS_CORRUPT_GPO_COMMONNAME, lpGPOPath);
        goto Exit;
    }


    //
    // In the results, get the values that match the display name attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage, szDisplayName);


    if (lpValues) {

        pszFriendlyName = LocalAlloc( LPTR, (lstrlen(*lpValues)+1) * sizeof(TCHAR) );
        if ( pszFriendlyName == 0) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Unable to allocate memory")));
            pLDAP->pfnldap_value_free (lpValues);
            goto Exit;
        }

        lstrcpy (pszFriendlyName, *lpValues);
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found display name of:  <%s>"), pszFriendlyName));
        pLDAP->pfnldap_value_free (lpValues);


    } else {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  No display name for this object.")));

        pszFriendlyName = LocalAlloc( LPTR, 2 * sizeof(TCHAR) );
        if ( pszFriendlyName == 0) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Unable to allocate memory")));
            goto Exit;
        }

        pszFriendlyName[0] = TEXT('\0');
    }


    //
    // In the results, get the values that match the version attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage, szVersion);

    if (lpValues) {

        dwVersion = StringToInt (*lpValues);

        if (dwFlags & GPO_LIST_FLAG_MACHINE) {
            dwVersion = MAKELONG(LOWORD(dwVersion), LOWORD(dwGPTVersion));
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found machine version of:  GPC is %d, GPT is %d"), LOWORD(dwVersion), HIWORD(dwVersion)));

        } else {
            dwVersion = MAKELONG(HIWORD(dwVersion), HIWORD(dwGPTVersion));
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found user version of:  GPC is %d, GPT is %d"), LOWORD(dwVersion), HIWORD(dwVersion)));
        }

        pLDAP->pfnldap_value_free (lpValues);

    } else {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  GPO %s does not have a version number."), lpGPOPath));
    }


    //
    // In the results, get the values that match the flags attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage, szFlags);

    if (lpValues) {

        dwGPOFlags = StringToInt (*lpValues);
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found flags of:  %d"), dwGPOFlags));
        pLDAP->pfnldap_value_free (lpValues);


    } else {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  No flags for this object.")));
    }


    //
    // In the results, get the values that match the extension names attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage,
                                         (dwFlags & GPO_LIST_FLAG_MACHINE) ? GPO_MACHEXTENSION_NAMES
                                                                           : GPO_USEREXTENSION_NAMES );
    if (lpValues) {

        if ( lstrcmpi( *lpValues, TEXT(" ") ) == 0 ) {

            //
            // A blank char is also a null property case, because Adsi doesn't commit null strings
            //
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  No client-side extensions for this object.")));

        } else {

            pszExtensions = LocalAlloc( LPTR, (lstrlen(*lpValues)+1) * sizeof(TCHAR) );
            if ( pszExtensions == 0 ) {

                DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Unable to allocate memory")));
                pLDAP->pfnldap_value_free (lpValues);
                goto Exit;

            }

            lstrcpy( pszExtensions, *lpValues );

            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found extensions:  %s"), pszExtensions));
        }

        pLDAP->pfnldap_value_free (lpValues);

    } else {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  No client-side extensions for this object.")));
    }


    //
    // Log which GPO we found
    //

    if (bVerbose) LogEvent (FALSE, IDS_FOUND_GPO, pszFriendlyName, szGPOName);


    //
    // Check the functionalty version number
    //

    if (dwFunctionalityVersion < 2) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  GPO %s was created by an old version of the Group Policy Editor.  It will be skipped."), pszFriendlyName));
        if (bVerbose) LogEvent (FALSE, IDS_GPO_TOO_OLD, pszFriendlyName);
        bResult = TRUE;
        goto Exit;
    }


    //
    // Check if the GPO is disabled
    //

    if (((dwFlags & GPO_LIST_FLAG_MACHINE) &&
         (dwGPOFlags & GPO_OPTION_DISABLE_MACHINE)) ||
         (!(dwFlags & GPO_LIST_FLAG_MACHINE) &&
         (dwGPOFlags & GPO_OPTION_DISABLE_USER))) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  GPO %s is disabled.  It will be skipped."), pszFriendlyName));
        if (bVerbose) LogEvent (FALSE, IDS_GPO_DISABLED, pszFriendlyName);
        bResult = TRUE;
        goto Exit;
    }

    //
    // Check if the version number is 0, if so there isn't any data
    // in the GPO and we can skip it
    //

    if (dwVersion == 0) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  GPO %s doesn't contain any data since the version number is 0.  It will be skipped."), pszFriendlyName));
        if (bVerbose) LogEvent (FALSE, IDS_GPO_NO_DATA, pszFriendlyName);
        bResult = TRUE;
        goto Exit;
    }

    //
    // Put the correct container name on the front of the LDAP path
    //

    lpTemp = LocalAlloc (LPTR, (lstrlen(lpGPOPath) + 20) * sizeof(TCHAR));

    if (!lpTemp) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Failed to allocate memory with %d"), GetLastError()));
        LogEvent (TRUE, IDS_OUT_OF_MEMORY, GetLastError());
        goto Exit;
    }

    if (dwFlags & GPO_LIST_FLAG_MACHINE) {
        lstrcpy (lpTemp, TEXT("LDAP://CN=Machine,"));
    } else {
        lstrcpy (lpTemp, TEXT("LDAP://CN=User,"));
    }

    DmAssert( lstrlen(TEXT("LDAP://CN=Machine,")) + lstrlen(lpPath) < (lstrlen(lpGPOPath) + 20) );

    lstrcat (lpTemp, lpPath);


    //
    // Add this GPO to the list
    //

    bResult = AddGPO (lpGPOList, dwGPOOptions, dwVersion, lpTemp,
                      pszGPTPath, pszFriendlyName, szGPOName, pszExtensions, GPOLink,
                      lpDSObject, 0, FALSE, bBlock, bVerbose);



    if (!bResult) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Failed to add GPO <%s> to the list."), pszFriendlyName));
    }

    LocalFree (lpTemp);

Exit:

    if ( pszGPTPath )
        LocalFree( pszGPTPath );

    if ( pszFriendlyName )
        LocalFree( pszFriendlyName );

    if ( pszExtensions )
        LocalFree( pszExtensions );

    if (pMessage && bOwnLdapMsg ) {
        pLDAP->pfnldap_msgfree (pMessage);
    }

    DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  ==============================")));

    return bResult;
}


//*************************************************************
//
//  SearchDSObject()
//
//  Purpose:    Searches the specified DS object for GPOs and
//              if found, adds them to the list.
//
//  Parameters: lpDSObject          - DS object to search
//              dwFlags             - GetGPOList flags
//              pGPOForcedList      - List of forced GPOs
//              pGPONonForcedList   - List of non-forced GPOs
//              bVerbose            - Verbose output
//              GPOLink             - GPO link type
//              pld                 - LDAP info
//              pLDAP               - LDAP api
//              bBlock              - Pointer to the block flag
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL SearchDSObject (LPTSTR lpDSObject, DWORD dwFlags, HANDLE hToken, PGROUP_POLICY_OBJECT *pGPOForcedList,
                     PGROUP_POLICY_OBJECT *pGPONonForcedList, BOOL bVerbose,
                     GPO_LINK GPOLink, PLDAP  pld, PLDAP_API pLDAP, PLDAPMessage pLDAPMsg,BOOL *bBlock )
{
    PGROUP_POLICY_OBJECT pForced = NULL, pNonForced = NULL, lpGPO;
    LPTSTR *lpValues;
    ULONG ulResult;
    BOOL bResult = FALSE;
    BOOL bOwnLdapMsg = FALSE;  // LDAP message owned by us (if true) or caller (if false)
    DWORD dwGPOOptions, dwOptions = 0;
    LPTSTR lpTemp, lpList, lpDSClass;
    TCHAR szGPLink[] = TEXT("gPLink");
    TCHAR szGPOPath[512];
    TCHAR szGPOOptions[12];
    TCHAR szGPOptions[] = TEXT("gPOptions");
    ULONG i = 0;
    LPTSTR lpFullDSObject = NULL;
    BOOL bFound = FALSE;
    LPTSTR lpAttr[4] = { szGPLink,
                         szGPOptions,
                         szObjectClass,
                         NULL
                       };

    //
    // Search for the object
    //

    DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  Searching <%s>"), lpDSObject));
    if (bVerbose) LogEvent (FALSE, IDS_SEARCHING, lpDSObject);

    if ( pLDAPMsg == NULL ) {

        bOwnLdapMsg = TRUE;

        ulResult = pLDAP->pfnldap_search_s (pld, lpDSObject, LDAP_SCOPE_BASE, szDSClassAny,
                                            lpAttr, FALSE, &pLDAPMsg);

        if (ulResult != LDAP_SUCCESS) {

            if (ulResult == LDAP_NO_SUCH_ATTRIBUTE) {

                DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  No GPO(s) for this object.")));
                if (bVerbose) LogEvent (FALSE, IDS_NO_GPOS);
                bResult = TRUE;

            } else if (ulResult == LDAP_NO_SUCH_OBJECT) {

                DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  Object not found in DS (this is ok).  Leaving. ")));
                if (bVerbose) LogEvent (FALSE, IDS_NO_DS_OBJECT, lpDSObject);
                bResult = TRUE;

            } else if (ulResult == LDAP_SIZELIMIT_EXCEEDED) {
               DebugMsg((DM_WARNING, TEXT("SearchDSObject:  Too many linked GPOs in search.") ));
               LogEvent (TRUE, IDS_TOO_MANY_GPOS);

            } else {

                DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  Failed to find DS object <%s> due to error %d."),
                         lpDSObject, ulResult));
                LogEvent (TRUE, IDS_OBJECT_NOT_FOUND, lpDSObject, ulResult);
            }

            goto Exit;

        }
    }

    //
    // In the results, get the values that match the gPOptions attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pLDAPMsg, szGPOptions);

    if (lpValues && *lpValues) {
        dwOptions = StringToInt (*lpValues);
        pLDAP->pfnldap_value_free (lpValues);
    }


    //
    // In the results, get the values that match the gPLink attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pLDAPMsg, szGPLink);


    if (lpValues && *lpValues) {

        lpList = *lpValues;

        DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  Found GPO(s):  <%s>"), lpList));

        lpFullDSObject = LocalAlloc (LPTR, (lstrlen(lpDSObject) + 8) * sizeof(TCHAR));

        if (!lpFullDSObject) {
            DebugMsg((DM_WARNING, TEXT("SearchDSObject:  Failed to allocate memory for full DS Object path name with %d"),
                     GetLastError()));
            pLDAP->pfnldap_value_free (lpValues);
            goto Exit;
        }

        lstrcpy (lpFullDSObject, TEXT("LDAP://"));
        lstrcat (lpFullDSObject, lpDSObject);


        while (*lpList) {


            //
            // Pull off the GPO ldap path
            //

            lpTemp = szGPOPath;
            dwGPOOptions = 0;

            while (*lpList && (*lpList != TEXT('['))) {
                lpList++;
            }

            if (!(*lpList)) {
                break;
            }

            lpList++;

            while (*lpList && (*lpList != TEXT(';'))) {
                *lpTemp++ = *lpList++;
            }

            if (!(*lpList)) {
                break;
            }

            *lpTemp = TEXT('\0');


            lpList++;

            lpTemp = szGPOOptions;
            *lpTemp = TEXT('\0');

            while (*lpList && (*lpList != TEXT(']'))) {
                *lpTemp++ = *lpList++;
            }

            if (!(*lpList)) {
                break;
            }

            *lpTemp = TEXT('\0');
            lpList++;

            dwGPOOptions = StringToInt (szGPOOptions);


            //
            // Check if this link is disabled
            //

            if (!(dwGPOOptions & GPO_FLAG_DISABLE)) {

                if (!ProcessGPO (szGPOPath, dwFlags, hToken,
                                 (dwGPOOptions & GPO_FLAG_FORCE) ? &pForced : &pNonForced,
                                 dwGPOOptions, TRUE, bVerbose,
                                 GPOLink, lpFullDSObject, pld, pLDAP, NULL, *bBlock)) {

                    DebugMsg((DM_WARNING, TEXT("SearchDSObject:  ProcessGPO failed.")));
                    pLDAP->pfnldap_value_free (lpValues);
                    goto Exit;
                }

            } else {
                DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  The link to GPO %s is disabled.  It will be skipped."), szGPOPath));
                if (bVerbose) LogEvent (FALSE, IDS_GPO_LINK_DISABLED, szGPOPath);
            }
        }

        pLDAP->pfnldap_value_free (lpValues);


        //
        // Set the block flag now if requested.  This way OU's, domains, etc
        // higher in the namespace will have GPOs removed if appropriate
        //

        if (dwOptions & GPC_BLOCK_POLICY) {
            *bBlock = TRUE;
            DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  <%s> has the Block From Above attribute set"), lpDSObject));
            if (bVerbose) LogEvent (FALSE, IDS_BLOCK_ENABLED, lpDSObject);
        }


    } else {
        DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  No GPO(s) for this object.")));
        if (bVerbose) LogEvent (FALSE, IDS_NO_GPOS);
    }

    //
    // Merge the temp and real lists together
    // First the non-forced lists
    //

    if (pNonForced) {

        lpGPO = pNonForced;

        while (lpGPO->pNext) {
            lpGPO = lpGPO->pNext;
        }

        lpGPO->pNext = *pGPONonForcedList;
        if (*pGPONonForcedList) {
            (*pGPONonForcedList)->pPrev = lpGPO;
        }

        *pGPONonForcedList = pNonForced;
    }

    //
    // Now the forced lists
    //

    if (pForced) {

        lpGPO = *pGPOForcedList;

        if (lpGPO) {
            while (lpGPO->pNext) {
                lpGPO = lpGPO->pNext;
            }

            lpGPO->pNext = pForced;
            pForced->pPrev = lpGPO;

        } else {
            *pGPOForcedList = pForced;
        }
    }

    bResult = TRUE;

Exit:

    if (lpFullDSObject) {
        LocalFree (lpFullDSObject);
    }

    if (pLDAPMsg && bOwnLdapMsg ) {
        pLDAP->pfnldap_msgfree (pLDAPMsg);
    }

    return bResult;
}

//*************************************************************
//
//  AllocDnEntry()
//
//  Purpose:    Allocates a new struct for dn entry
//
//
//  Parameters: pwszDN  - Distinguished name
//
//  Return:     Pointer if successful
//              NULL if an error occurs
//
//*************************************************************

DNENTRY * AllocDnEntry( LPTSTR pwszDN )
{
    DNENTRY *pDnEntry = (DNENTRY *) LocalAlloc (LPTR, sizeof(DNENTRY));

    if ( pDnEntry == NULL ) {
        DebugMsg((DM_WARNING, TEXT("AllocDnEntry: Failed to alloc pDnEntry with 0x%x."),
                  GetLastError()));
        return NULL;
    }

    pDnEntry->pwszDN = (LPTSTR) LocalAlloc (LPTR, (lstrlen(pwszDN) + 1) * sizeof(TCHAR) );

    if ( pDnEntry->pwszDN == NULL ) {
       DebugMsg((DM_WARNING, TEXT("AllocDnEntry: Failed to alloc pwszDN with 0x%x."),
                 GetLastError()));
       LocalFree( pDnEntry );
       return NULL;
    }

    lstrcpy( pDnEntry->pwszDN, pwszDN );

    return pDnEntry;
}


//*************************************************************
//
//  FreeDnEntry()
//
//  Purpose:    Frees dn entry struct
//
//*************************************************************

void FreeDnEntry( DNENTRY *pDnEntry )
{
    if ( pDnEntry ) {
        if ( pDnEntry->pwszDN )
            LocalFree( pDnEntry->pwszDN );

        LocalFree( pDnEntry );
    }
}

//*************************************************************
//
//  AllocLdapQuery()
//
//  Purpose:    Allocates a new struct for ldap query
//
//
//  Parameters: pwszDomain  - Domain of Gpo
//
//  Return:     Pointer if successful
//              NULL if an error occurs
//
//*************************************************************

LDAPQUERY * AllocLdapQuery( LPTSTR pwszDomain )
{
    const INIT_ALLOC_SIZE = 1000;
    LDAPQUERY *pQuery = (LDAPQUERY *) LocalAlloc (LPTR, sizeof(LDAPQUERY));

    if ( pQuery == NULL ) {
        DebugMsg((DM_WARNING, TEXT("AllocLdapQuery: Failed to alloc pQuery with 0x%x."),
                  GetLastError()));
        return NULL;
    }

    pQuery->pwszDomain = (LPTSTR) LocalAlloc (LPTR, (lstrlen(pwszDomain) + 1) * sizeof(TCHAR) );

    if ( pQuery->pwszDomain == NULL ) {
        DebugMsg((DM_WARNING, TEXT("AllocLdapQuery: Failed to alloc pwszDomain with 0x%x."),
                  GetLastError()));
        LocalFree( pQuery );
        return NULL;
    }

    pQuery->pwszFilter = (LPTSTR) LocalAlloc (LPTR, INIT_ALLOC_SIZE );

    if ( pQuery->pwszFilter == NULL ) {
        DebugMsg((DM_WARNING, TEXT("AllocLdapQuery: Failed to alloc pwszFilter with 0x%x."),
                  GetLastError()));
        LocalFree( pQuery->pwszDomain );
        LocalFree( pQuery );
        return NULL;
    }

    lstrcpy( pQuery->pwszDomain, pwszDomain );
    lstrcpy( pQuery->pwszFilter, L"(|)" );
    pQuery->cbLen = 8;           // 8 = (lstrlen(L"(|)") + 1) * sizeof(TCHAR)
    pQuery->cbAllocLen = INIT_ALLOC_SIZE;

    return pQuery;
}


//*************************************************************
//
//  FreeLdapQuery()
//
//  Purpose:    Frees ldap query struct
//
//*************************************************************

void FreeLdapQuery( PLDAP_API pLDAP, LDAPQUERY *pQuery )
{
    DNENTRY *pDnEntry = NULL;

    if ( pQuery ) {

        if ( pQuery->pwszDomain )
            LocalFree( pQuery->pwszDomain );

        if ( pQuery->pwszFilter )
            LocalFree( pQuery->pwszFilter );

        if ( pQuery->pMessage )
            pLDAP->pfnldap_msgfree( pQuery->pMessage );

        if ( pQuery->pLdapHandle && pQuery->bOwnLdapHandle )
            pLDAP->pfnldap_unbind( pQuery->pLdapHandle );

        pDnEntry = pQuery->pDnEntry;

        while ( pDnEntry ) {
            DNENTRY *pTemp = pDnEntry->pNext;
            FreeDnEntry( pDnEntry );
            pDnEntry = pTemp;
        }

        LocalFree( pQuery );

    }
}


//*************************************************************
//
//  MatchDnWithDeferredItems()
//
//  Purpose:    Matches the dns from ldap query with the deferred items
//
//  Parameters: pLDAP         - LDAP function table pointer
//              ppLdapQuery   - LDAP query list
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL MatchDnWithDeferredItems( PLDAP_API pLDAP, LDAPQUERY *pLdapQuery, BOOL bOUProcessing )
{
    PLDAPMessage pMsg = pLDAP->pfnldap_first_entry( pLdapQuery->pLdapHandle, pLdapQuery->pMessage );

    while ( pMsg ) {

        WCHAR *pwszDN = pLDAP->pfnldap_get_dn( pLdapQuery->pLdapHandle, pMsg );

        DNENTRY *pCurPtr = pLdapQuery->pDnEntry;

        while ( pCurPtr ) {

            INT iResult = CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                          pwszDN, -1, pCurPtr->pwszDN, -1 );
            if ( iResult == CSTR_EQUAL ) {

                //
                // Store the pointer to ldap message so that it can be used
                // later to retrieve necessary attributes.
                //
                if ( bOUProcessing )
                    pCurPtr->pDeferredOU->pOUMsg = pMsg;
                else {
                    pCurPtr->pDeferredGPO->lParam = (LPARAM) pMsg;
                    pCurPtr->pDeferredGPO->lParam2 = (LPARAM) pLdapQuery->pLdapHandle;
                }

                pCurPtr = pCurPtr->pNext;

            } else if ( iResult == CSTR_LESS_THAN ) {

                //
                // Since dns are in ascending order,
                // we are done.
                //

                break;

            } else {

                //
                // Advance down the list
                //

                pCurPtr = pCurPtr->pNext;

            } // final else

        }   // while pcurptr

        pLDAP->pfnldap_memfree( pwszDN );

        pMsg = pLDAP->pfnldap_next_entry( pLdapQuery->pLdapHandle, pMsg );

    }   // while pmsg

    return TRUE;
}

//*************************************************************
//
//  AddDnToFilter()
//
//  Purpose:    ORs in the new dn to the ldap filter
//
//  Parameters: ppLdapQuery       - LDAP query list
//              pGPO              - Deferred GPO
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL AddDnToFilter( LDAPQUERY *pLdapQuery, LPTSTR pwszDN )
{
    const DN_SIZE = 20;      // 20 = # chars in "(dis..=)"

    DWORD cbNew = (lstrlen(pwszDN) + DN_SIZE) * sizeof(TCHAR); // + 1 is not needed because \0 is already part of filter string

    DWORD cbSizeRequired = pLdapQuery->cbLen + cbNew;

    if ( cbSizeRequired >= pLdapQuery->cbAllocLen ) {

        //
        // Need to grow buffer because of overflow
        //

        LPTSTR pwszNewFilter = (LPTSTR) LocalAlloc (LPTR, cbSizeRequired * 2);

        if ( pwszNewFilter == NULL ) {
            DebugMsg((DM_WARNING, TEXT("AddDnToFilter: Unable to allocate new filter string") ));
            return FALSE;
        }

        lstrcpy( pwszNewFilter, pLdapQuery->pwszFilter );

        LocalFree( pLdapQuery->pwszFilter );
        pLdapQuery->pwszFilter = pwszNewFilter;

        pLdapQuery->cbAllocLen = cbSizeRequired * 2;
    }

    DmAssert( cbSizeRequired < pLdapQuery->cbAllocLen );

    //
    // Overwrite last ")" and then append the new dn name term
    //

    lstrcpy( &pLdapQuery->pwszFilter[pLdapQuery->cbLen/2 - 2], L"(distinguishedName=" );
    lstrcat( pLdapQuery->pwszFilter, pwszDN );
    lstrcat( pLdapQuery->pwszFilter, L"))" );

    pLdapQuery->cbLen += cbNew;

    return TRUE;
}

//*************************************************************
//
//  InsertDN()
//
//  Purpose:    Adds a distinguished name entry to ldap query's
//              names linked list
//
//  Parameters: ppLdapQuery       - LDAP query list
//              pwszDN            - DN
//              pDeferredOU       - Deferred OU
//              pDeferredGPO      - Deferred GPO
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL InsertDN( LDAPQUERY *pLdapQuery, LPTSTR pwszDN,
               DNENTRY *pDeferredOU, PGROUP_POLICY_OBJECT pDeferredGPO )
{
    DNENTRY *pNewEntry = NULL;
    DNENTRY *pTrailPtr = NULL;
    DNENTRY *pCurPtr = pLdapQuery->pDnEntry;

    DmAssert( !( pDeferredOU && pDeferredGPO ) );

    while ( pCurPtr != NULL ) {

         INT iResult = CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                       pwszDN, -1, pCurPtr->pwszDN, -1 );

         if ( iResult == CSTR_EQUAL || iResult == CSTR_LESS_THAN ) {

             //
             // Duplicate or since dn's are in ascending order, add new entry
             //

             DNENTRY *pNewEntry = AllocDnEntry( pwszDN );
             if ( pNewEntry == NULL )
                 return FALSE;

             if ( !AddDnToFilter( pLdapQuery, pwszDN ) ) {
                 FreeDnEntry( pNewEntry );
                 return FALSE;
             }

             if ( pDeferredOU )
                 pNewEntry->pDeferredOU = pDeferredOU;
             else
                 pNewEntry->pDeferredGPO = pDeferredGPO;

             pNewEntry->pNext = pCurPtr;
             if ( pTrailPtr == NULL )
                 pLdapQuery->pDnEntry = pNewEntry;
             else
                 pTrailPtr->pNext = pNewEntry;

             return TRUE;

         } else {

             //
             // Advance down the list
             //

             pTrailPtr = pCurPtr;
             pCurPtr = pCurPtr->pNext;

         }

    }    // while

    //
    // Null list or end of list case.
    //

    pNewEntry = AllocDnEntry( pwszDN );
    if ( pNewEntry == NULL )
         return FALSE;

    if ( !AddDnToFilter( pLdapQuery, pwszDN ) ) {
        FreeDnEntry( pNewEntry );
        return FALSE;
    }

    if ( pDeferredOU )
        pNewEntry->pDeferredOU = pDeferredOU;
    else
        pNewEntry->pDeferredGPO = pDeferredGPO;

    pNewEntry->pNext = pCurPtr;
    if ( pTrailPtr == NULL )
         pLdapQuery->pDnEntry = pNewEntry;
    else
        pTrailPtr->pNext = pNewEntry;

    return TRUE;
}



//*************************************************************
//
//  AddDN()
//
//  Purpose:    Adds a distinguished name entry to ldap query
//
//  Parameters: ppLdapQuery       - LDAP query list
//              pwszDN            - DN name
//              pDeferredOU       - Deferred OU
//              pDeferredGPO      - Deferred GPO
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL AddDN( PLDAP_API pLDAP, LDAPQUERY **ppLdapQuery,
            LPTSTR pwszDN, DNENTRY *pDeferredOU, PGROUP_POLICY_OBJECT pDeferredGPO )
{
    LPTSTR pwszDomain = NULL;
    LPTSTR pwszTemp = pwszDN;
    LDAPQUERY *pNewQuery = NULL;
    LDAPQUERY *pTrailPtr = NULL;
    LDAPQUERY *pCurPtr = *ppLdapQuery;

    DmAssert( !( pDeferredOU && pDeferredGPO ) );

    //
    // Find the domain to which the GPO belongs
    //

    if ( pwszTemp == NULL ) {
        DebugMsg((DM_WARNING, TEXT("AddDN: Null pwszDN. Exiting.") ));
        return FALSE;
    }

    while ( *pwszTemp ) {

        if (CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                            pwszTemp, 3, TEXT("DC="), 3) == CSTR_EQUAL ) {
            pwszDomain = pwszTemp;
            break;
        }

        //
        // Move to the next chunk of the DN name
        //

        while ( *pwszTemp && (*pwszTemp != TEXT(',')))
            pwszTemp++;

        if ( *pwszTemp == TEXT(','))
            pwszTemp++;

    }

    if ( pwszDomain == NULL ) {
        DebugMsg((DM_WARNING, TEXT("AddDN: Domain not found for <%s>. Exiting."), pwszDN ));
        return FALSE;
    }

    while ( pCurPtr != NULL ) {

        INT iResult = CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                      pwszDomain, -1, pCurPtr->pwszDomain, -1 );
        if ( iResult == CSTR_EQUAL ) {

            BOOL bOk = InsertDN( pCurPtr, pwszDN, pDeferredOU, pDeferredGPO );
            return bOk;

        } else if ( iResult == CSTR_LESS_THAN ) {

            //
            // Since domains are in ascending order,
            // pwszDomain is not in list, so add.
            //

            pNewQuery = AllocLdapQuery( pwszDomain );
            if ( pNewQuery == NULL )
                return FALSE;

            if ( !InsertDN( pNewQuery, pwszDN, pDeferredOU, pDeferredGPO ) ) {
                FreeLdapQuery( pLDAP, pNewQuery );
                return FALSE;
            }

            pNewQuery->pNext = pCurPtr;
            if ( pTrailPtr == NULL )
                *ppLdapQuery = pNewQuery;
            else
                pTrailPtr->pNext = pNewQuery;

            return TRUE;

        } else {

            //
            // Advance down the list
            //

            pTrailPtr = pCurPtr;
            pCurPtr = pCurPtr->pNext;

        }

    }   // while

    //
    // Null list or end of list case.
    //

    pNewQuery = AllocLdapQuery( pwszDomain );

    if ( pNewQuery == NULL )
        return FALSE;

    if ( !InsertDN( pNewQuery, pwszDN, pDeferredOU, pDeferredGPO ) ) {
        FreeLdapQuery( pLDAP, pNewQuery );
        return FALSE;
    }

    pNewQuery->pNext = pCurPtr;

    if ( pTrailPtr == NULL )
        *ppLdapQuery = pNewQuery;
    else
        pTrailPtr->pNext = pNewQuery;

    return TRUE;
}



//*************************************************************
//
//  EvalList()
//
//  Purpose:    Encapsulates common processing functionality for
//              forced and nonforced lists
//
//  Parameters: pLDAP                  - LDAP api
//              dwFlags                - GetGPOList flags
//              bVerbose               - Verbose flag
//              hToken                 - User or machine token
//              pDeferredList          - List of deferred GPOs
//              ppGPOList              - List of evaluated GPOs
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL EvalList(  PLDAP_API pLDAP,
                DWORD dwFlags,
                HANDLE hToken,
                BOOL bVerbose,
                PGROUP_POLICY_OBJECT pDeferredList,
                PGROUP_POLICY_OBJECT *ppGPOList  )
{
    PGROUP_POLICY_OBJECT pGPOTemp = pDeferredList;

    while ( pGPOTemp ) {

        PLDAPMessage pGPOMsg = (PLDAPMessage) pGPOTemp->lParam;

        if ( pGPOMsg == NULL ) {
            DebugMsg((DM_VERBOSE, TEXT("EvalList: Object <%s> cannot be accessed"),
                      pGPOTemp->lpDSPath ));
            if (bVerbose)
                LogEvent (FALSE, IDS_OBJECT_NOT_FOUND, pGPOTemp->lpDSPath, 0);

        } else {

            DmAssert( (PLDAP) pGPOTemp->lParam2 != NULL );

            if ( !ProcessGPO( pGPOTemp->lpDSPath, dwFlags, hToken, ppGPOList, pGPOTemp->dwOptions,
                              FALSE, bVerbose, pGPOTemp->GPOLink, pGPOTemp->lpLink, (PLDAP) pGPOTemp->lParam2, pLDAP,
                              pGPOMsg, FALSE ) ) {
                DebugMsg((DM_WARNING, TEXT("EvalList:  ProcessGPO failed") ));
                return FALSE;
            }

        }

        pGPOTemp = pGPOTemp->pNext;

    }

    return TRUE;
}

//*************************************************************
//
//  EvaluateDeferredGPOs()
//
//  Purpose:    Uses a single ldap query to evaluate deferred
//              GPO lists.
//
//  Parameters: pldBound               - Bound LDAP handle
//              pLDAP                  - LDAP api
//              pwszDomainBound        - Domain already bound to
//              dwFlags                - GetGPOList flags
//              hToken                 - User or machine token
//              pDeferredForcedList    - List of deferred forced GPOs
//              pDeferredNonForcedList - List of deferred non-forced GPOs
//              pGPOForcedList         - List of forced GPOs
//              pGPONonForcedList      - List of non-forced GPOs
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL EvaluateDeferredGPOs (PLDAP pldBound,
                           PLDAP_API pLDAP,
                           LPTSTR pwszDomainBound,
                           DWORD dwFlags,
                           HANDLE hToken,
                           BOOL bVerbose,
                           PGROUP_POLICY_OBJECT pDeferredForcedList,
                           PGROUP_POLICY_OBJECT pDeferredNonForcedList,
                           PGROUP_POLICY_OBJECT *ppForcedList,
                           PGROUP_POLICY_OBJECT *ppNonForcedList )
{
    ULONG ulResult;
    BOOL bResult = FALSE;
    BYTE berValue[8];
    LDAPControl SeInfoControl = { LDAP_SERVER_SD_FLAGS_OID_W, { 5, (PCHAR)berValue }, TRUE };
    LDAPControl referralControl = { LDAP_SERVER_DOMAIN_SCOPE_OID_W, { 0, NULL}, TRUE };
    PLDAPControl ServerControls[] = { &SeInfoControl, &referralControl, NULL };
    TCHAR szSDProperty[] = TEXT("nTSecurityDescriptor");
    TCHAR szCommonName[] = TEXT("cn");
    TCHAR szDisplayName[] = TEXT("displayName");
    TCHAR szFileSysPath[] = TEXT("gPCFileSysPath");
    TCHAR szVersion[] = TEXT("versionNumber");
    TCHAR szFunctionalityVersion[] = GPO_FUNCTIONALITY_VERSION;
    TCHAR szFlags[] = TEXT("flags");
    PWSTR rgAttribs[11] = { szSDProperty,
                           szFileSysPath,
                           szCommonName,
                           szDisplayName,
                           szVersion,
                           szFunctionalityVersion,
                           szFlags,
                           GPO_MACHEXTENSION_NAMES,
                           GPO_USEREXTENSION_NAMES,
                           szObjectClass,
                           NULL };
    PGROUP_POLICY_OBJECT pGPOTemp = pDeferredForcedList;
    LDAPQUERY *pLdapQuery = NULL, *pQuery = NULL;
    VOID *pData;
    PDS_API pdsApi;

    *ppForcedList = NULL;
    *ppNonForcedList = NULL;

    if ( pDeferredForcedList == NULL && pDeferredNonForcedList == NULL )
        return TRUE;

    //
    // Demand load ntdsapi.dll
    //

    pdsApi = LoadDSApi();

    if ( pdsApi == 0 ) {
        DebugMsg((DM_WARNING, TEXT("EvaluateDeferredGpos: Failed to load ntdsapi.dll")));
        goto Exit;
    }

    while ( pGPOTemp ) {

        if ( !AddDN( pLDAP, &pLdapQuery, pGPOTemp->lpDSPath, NULL, pGPOTemp ) )
             goto Exit;
        pGPOTemp = pGPOTemp->pNext;

    }

    pGPOTemp = pDeferredNonForcedList;
    while ( pGPOTemp ) {

        if ( !AddDN( pLDAP, &pLdapQuery, pGPOTemp->lpDSPath, NULL, pGPOTemp ) )
             goto Exit;
        pGPOTemp = pGPOTemp->pNext;

    }

    //
    // Setup the BER encoding
    //

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02; // denotes an integer
    berValue[3] = 0x01; // denotes size
    berValue[4] = (BYTE)((DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION) & 0xF);

    pQuery  = pLdapQuery;
    while ( pQuery ) {

        //
        // Check if this is a cross-domain Gpo and hence needs a new bind
        //

        WCHAR *pDomainString[1];
        PDS_NAME_RESULT pNameResult = NULL;
        PLDAP pLdapHandle = NULL;

        pDomainString[0] = pQuery->pwszDomain;

        ulResult = pdsApi->pfnDsCrackNames( (HANDLE) -1,
                                          DS_NAME_FLAG_SYNTACTICAL_ONLY,
                                          DS_FQDN_1779_NAME,
                                          DS_CANONICAL_NAME,
                                          1,
                                          pDomainString,
                                          &pNameResult );

        if ( ulResult != ERROR_SUCCESS
             || pNameResult->cItems == 0
             || pNameResult->rItems[0].status != ERROR_SUCCESS
             || pNameResult->rItems[0].pDomain == NULL ) {

            DebugMsg((DM_VERBOSE, TEXT("EvaluateDeferredGPOs:  DsCrackNames failed with 0x%x."), ulResult ));
            goto Exit;
        }

        //
        // Optimize same domain Gpo queries by not doing an unnecessary bind
        //

        pQuery->pLdapHandle = pldBound;

        if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                           pwszDomainBound, -1, pNameResult->rItems[0].pDomain, -1) != CSTR_EQUAL) {

            //
            // Cross-domain Gpo query and so need to bind to new domain
            //

            DebugMsg((DM_VERBOSE, TEXT("EvaluateDeferredGPOs: Doing an ldap bind to cross-domain <%s>"),
                      pNameResult->rItems[0].pDomain));

            pLdapHandle = pLDAP->pfnldap_open( pNameResult->rItems[0].pDomain, LDAP_PORT);

            if (!pLdapHandle) {

                DebugMsg((DM_WARNING, TEXT("EvaluateDeferredGPOs:  ldap_open for <%s> failed with = 0x%x or %d"),
                          pNameResult->rItems[0].pDomain, pLDAP->pfnLdapGetLastError(), GetLastError()));
                LogEvent (TRUE, IDS_FAILED_DS_CONNECT, pNameResult->rItems[0].pDomain, pLDAP->pfnLdapGetLastError());

                pdsApi->pfnDsFreeNameResult( pNameResult );

                goto Exit;
            }


            //
            // Turn on Packet integrity flag
            //            

            pData = (VOID *) LDAP_OPT_ON;
            ulResult = pLDAP->pfnldap_set_option(pLdapHandle, LDAP_OPT_SIGN, &pData);

            if (ulResult != LDAP_SUCCESS) {

                DebugMsg((DM_WARNING, TEXT("EvaluateDeferredGPOs:  Failed to turn on LDAP_OPT_SIGN with %d"), ulResult));

                pdsApi->pfnDsFreeNameResult( pNameResult );

                goto Exit;
            }
            
            //
            // Transfer ownerhip of ldap handle to pQuery struct
            //

            pQuery->pLdapHandle = pLdapHandle;
            pQuery->bOwnLdapHandle = TRUE;

            if ( dwFlags & GPO_LIST_FLAG_MACHINE) {

                //
                // For machine policies specifically ask for Kerberos as the only authentication
                // mechanism. Otherwise if Kerberos were to fail for some reason, then NTLM is used
                // and localsystem context has no real credentials, which means that we won't get
                // any GPOs back.
                //

                SEC_WINNT_AUTH_IDENTITY_EXW secIdentity;

                secIdentity.Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
                secIdentity.Length = sizeof(SEC_WINNT_AUTH_IDENTITY_EXW);
                secIdentity.User = 0;
                secIdentity.UserLength = 0;
                secIdentity.Domain = 0;
                secIdentity.DomainLength = 0;
                secIdentity.Password = 0;
                secIdentity.PasswordLength = 0;
                secIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
                secIdentity.PackageList = wszKerberos;
                secIdentity.PackageListLength = lstrlen( wszKerberos );

                ulResult = pLDAP->pfnldap_bind_s (pLdapHandle, NULL, (WCHAR *)&secIdentity, LDAP_AUTH_SSPI);

            } else
                ulResult = pLDAP->pfnldap_bind_s (pLdapHandle, NULL, NULL, LDAP_AUTH_SSPI);

            if (ulResult != LDAP_SUCCESS) {

                DebugMsg((DM_WARNING, TEXT("EvaluateDeferredGPOs:  ldap_bind_s failed with = <%d>"),
                          ulResult));
                LogEvent (TRUE, IDS_FAILED_DS_BIND, pNameResult->rItems[0].pDomain, ulResult);

                pdsApi->pfnDsFreeNameResult( pNameResult );

                goto Exit;
            }

            DebugMsg((DM_VERBOSE, TEXT("EvaluateDeferredGPOs: Bind sucessful")));

        }

        pdsApi->pfnDsFreeNameResult( pNameResult );

        //
        // Turn referrals off because this is a single domain call
        //

        pData = (VOID *) LDAP_OPT_OFF;
        ulResult = pLDAP->pfnldap_set_option( pQuery->pLdapHandle,  LDAP_OPT_REFERRALS, &pData );
        if ( ulResult != LDAP_SUCCESS )
        {
            DebugMsg((DM_WARNING, TEXT("EvalauteDeferredGPOs:  Failed to turn off referrals with error %d"), ulResult));
            goto Exit;
        }

        //
        // Search for GPOs
        //

        DmAssert( pQuery->pwszDomain != NULL && pQuery->pwszFilter != NULL );

        ulResult = pLDAP->pfnldap_search_ext_s(pQuery->pLdapHandle, pQuery->pwszDomain, LDAP_SCOPE_SUBTREE,
                                               pQuery->pwszFilter, rgAttribs, 0,
                                               (PLDAPControl*)ServerControls,
                                               NULL, NULL, 0x10000, &pQuery->pMessage);

        //
        // If the search fails, store the error code and return
        //

        if (ulResult != LDAP_SUCCESS) {

            if (ulResult == LDAP_NO_SUCH_ATTRIBUTE) {
                DebugMsg((DM_VERBOSE, TEXT("EvaluateDeferredGPOs:  All objects can not be accessed.")));
                if (bVerbose) LogEvent (FALSE, IDS_NO_GPOS);
                bResult = TRUE;

            } else if (ulResult == LDAP_NO_SUCH_OBJECT) {
                DebugMsg((DM_VERBOSE, TEXT("EvalateDeferredGPOs:  Objects do not exist.") ));
                if (bVerbose) LogEvent (FALSE, IDS_NO_GPOS);
                bResult = TRUE;

            } else if (ulResult == LDAP_SIZELIMIT_EXCEEDED) {
                DebugMsg((DM_WARNING, TEXT("EvalateDeferredGPOs:  Too many GPOs in search.") ));
                LogEvent (TRUE, IDS_TOO_MANY_GPOS);

            } else {
                DebugMsg((DM_WARNING, TEXT("EvaluteDeferredGPOs:  Failed to search with error 0x%x"), ulResult));
                LogEvent (TRUE, IDS_FAILED_GPO_SEARCH, ulResult);
            }

            goto Exit;
        }

        //
        // If the search succeeds, but the message is empty,
        // store the error code and return
        //

        if ( pQuery->pMessage == NULL ) {
            DebugMsg((DM_WARNING, TEXT("EvaluateDeferredGPOs:  Search returned an empty message structure.  Error = 0x%x"),
                     pQuery->pLdapHandle->ld_errno));
            goto Exit;
        }

        if ( !MatchDnWithDeferredItems( pLDAP, pQuery, FALSE ) )
            goto Exit;

        pQuery = pQuery->pNext;

    }   // while

    if ( !EvalList( pLDAP, dwFlags, hToken, bVerbose,
                    pDeferredForcedList, ppForcedList ) ) {
        goto Exit;
    }

    if ( !EvalList( pLDAP, dwFlags, hToken, bVerbose,
                    pDeferredNonForcedList, ppNonForcedList) ) {
        goto Exit;
    }

    bResult = TRUE;

Exit:

    //
    // Free all resources except for ppForcedList, ppNonForcedList
    // which are owned by caller.
    //

    while ( pLdapQuery ) {
        pQuery = pLdapQuery->pNext;
        FreeLdapQuery( pLDAP, pLdapQuery );
        pLdapQuery = pQuery;
    }

    return bResult;
}


//*************************************************************
//
//  AddOU()
//
//  Purpose:    Appends an OU or domain to deferred list.
//
//  Parameters: ppOUList    - OU list to append to
//              pwszOU      - OU name
//              gpoLink     - Type of Gpo
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL AddOU( DNENTRY **ppOUList, LPTSTR pwszOU, GPO_LINK gpoLink )
{
    DNENTRY *pOUTemp = *ppOUList;
    DNENTRY *pOULast = NULL;

    DNENTRY *pOUNew = AllocDnEntry( pwszOU );
    if ( pOUNew == NULL )
        return FALSE;

    pOUNew->gpoLink = gpoLink;

    while ( pOUTemp ) {
        pOULast = pOUTemp;
        pOUTemp = pOUTemp->pNext;
    }

    if ( pOULast )
        pOULast->pNext = pOUNew;
    else
        *ppOUList = pOUNew;

    return TRUE;
}


//*************************************************************
//
//  EvaluateDeferredOUs()
//
//  Purpose:    Uses a single Ldap query to evaluate all OUs
//
//  Parameters: ppOUList            - OU list to append to
//              dwFlags             - GetGPOList flags
//              pGPOForcedList      - List of forced GPOs
//              pGPONonForcedList   - List of non-forced GPOs
//              bVerbose            - Verbose output
//              pld                 - LDAP info
//              pLDAP               - LDAP api
//              pLDAPMsg            - LDAP message
//              bBlock              - Pointer to the block flag
//              hToken              - User / machine token
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL EvaluateDeferredOUs(   DNENTRY *pOUList,
                            DWORD dwFlags,
                            HANDLE hToken,
                            PGROUP_POLICY_OBJECT *ppDeferredForcedList,
                            PGROUP_POLICY_OBJECT *ppDeferredNonForcedList,
                            BOOL bVerbose,
                            PLDAP  pld,
                            PLDAP_API pLDAP,
                            BOOL *pbBlock)
{
    ULONG ulResult;
    BOOL bResult = FALSE;
    LDAPQUERY *pLdapQuery = NULL;
    TCHAR szGPLink[] = TEXT("gPLink");
    TCHAR szGPOptions[] = TEXT("gPOptions");
    LPTSTR lpAttr[] = { szGPLink,
                        szGPOptions,
                        NULL
                      };
    DNENTRY *pOUTemp = pOUList;
    VOID *pData;
#if 0 //BUGBUG:site
    BYTE berValue[8];
    LDAPControl phantomControl = { LDAP_SERVER_SEARCH_OPTIONS_OID_W, { 5, (PCHAR)berValue }, TRUE };
    PLDAPControl serverControls[] = { &phantomControl, NULL }; // Allows searching for gpLink attribute on site object
#endif

    if ( pOUTemp == NULL )
        return TRUE;

    while ( pOUTemp ) {
        if ( !AddDN( pLDAP, &pLdapQuery, pOUTemp->pwszDN, pOUTemp, NULL ) ) {
            goto Exit;
        }
        pOUTemp = pOUTemp->pNext;
    }

    pLdapQuery->pLdapHandle = pld;

    //
    // Turn referrals off because this is a single domain call
    //

    pData = (VOID *) LDAP_OPT_OFF;
    ulResult = pLDAP->pfnldap_set_option( pLdapQuery->pLdapHandle,  LDAP_OPT_REFERRALS, &pData );
    if ( ulResult != LDAP_SUCCESS ) {
        DebugMsg((DM_WARNING, TEXT("EvaluteDeferredOUs:  Failed to turn off referrals with error %d"), ulResult));
        goto Exit;
    }

#if 0 //BUGBUG:site
    //
    // Setup the BER encoding
    //

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02; // denotes an integer
    berValue[3] = 0x01; // denotes size
    berValue[4] = (BYTE) SERVER_SEARCH_FLAG_PHANTOM_ROOT;

    ulResult = pLDAP->pfnldap_search_ext_s (pld, L"", LDAP_SCOPE_SUBTREE,
                                            pLdapQuery->pwszFilter, lpAttr, FALSE,
                                            (PLDAPControl*)serverControls,
                                            NULL, NULL, 0x10000, &pLdapQuery->pMessage );
#else

    ulResult = pLDAP->pfnldap_search_s (pld, pLdapQuery->pwszDomain, LDAP_SCOPE_SUBTREE,
                                        pLdapQuery->pwszFilter, lpAttr, FALSE, &pLdapQuery->pMessage );
#endif

    //
    // If the search fails, store the error code and return
    //

    if (ulResult != LDAP_SUCCESS) {

        if (ulResult == LDAP_NO_SUCH_ATTRIBUTE) {
            DebugMsg((DM_VERBOSE, TEXT("EvaluateDeferredOUs:  All objects can not be accessed.")));
            bResult = TRUE;

        } else if (ulResult == LDAP_NO_SUCH_OBJECT) {
            DebugMsg((DM_VERBOSE, TEXT("EvalateDeferredOUs:  Objects do not exist.") ));
            bResult = TRUE;

        } else if (ulResult == LDAP_SIZELIMIT_EXCEEDED) {
               DebugMsg((DM_WARNING, TEXT("EvalateDeferredOUs:  Too many linked GPOs in search.") ));
               LogEvent (TRUE, IDS_TOO_MANY_GPOS);

        } else {
            DebugMsg((DM_WARNING, TEXT("EvaluateDeferredOUs:  Failed to search with error %d"), ulResult));
            LogEvent (TRUE, IDS_FAILED_OU_SEARCH, ulResult);
        }

        goto Exit;
    }

    //
    // If the search succeeds, but the message is empty,
    // store the error code and return
    //

    if ( pLdapQuery->pMessage == NULL ) {
        DebugMsg((DM_WARNING, TEXT("EvaluateDeferredOUs:  Search returned an empty message structure.  Error = %d"),
                  pld->ld_errno));
        goto Exit;
    }

    if ( !MatchDnWithDeferredItems( pLDAP, pLdapQuery, TRUE ) )
        goto Exit;

    //
    // Evaluate the OU list
    //

    pOUTemp = pOUList;

    while ( pOUTemp ) {

        PLDAPMessage pOUMsg = pOUTemp->pOUMsg;

        if ( pOUMsg == NULL ) {
            DebugMsg((DM_WARNING, TEXT("EvaluateDeferredOUs: Object <%s> cannot be accessed"),
                      pOUTemp->pwszDN ));
        } else {
               if ( !SearchDSObject( pOUTemp->pwszDN, dwFlags, hToken, ppDeferredForcedList, ppDeferredNonForcedList,
                                     bVerbose, pOUTemp->gpoLink, pld, pLDAP, pOUMsg, pbBlock)) {
                   DebugMsg((DM_WARNING, TEXT("EvaluateDeferredOUs:  SearchDSObject failed") ));
                   goto Exit;
               }
        }

        pOUTemp = pOUTemp->pNext;

    }

    bResult = TRUE;

Exit:

    while ( pLdapQuery ) {
        LDAPQUERY *pQuery = pLdapQuery->pNext;
        FreeLdapQuery( pLDAP, pLdapQuery );
        pLdapQuery = pQuery;
    }

    return bResult;
}


//*************************************************************
//
//  GetGPOInfo()
//
//  Purpose:    Gets the GPO info for this threads token.
//
//  Parameters: dwFlags         -   GPO_LIST_FLAG_* from userenv.h
//              lpHostName      -   Domain DN name or DC server name
//              lpDNName        -   User or Machine DN name
//              lpComputerName  -   Computer name used for site look up
//              lpGPOList       -   Receives the list of GROUP_POLICY_OBJECTs
//              pNetAPI32       -   Netapi32 function table
//              bMachineTokenOK -   Ok to query for the machine token
//
//  Comment:    This is a link list of GROUP_POLICY_OBJECTs.  Each can be
//              free'ed with LocalFree() or calling FreeGPOList()
//
//              The processing sequence is:
//
//              Local Site Domain OrganizationalUnit
//
//              Note that we process this list backwards to get the
//              correct sequencing for the force flag.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL GetGPOInfo (DWORD dwFlags, LPTSTR lpHostName, LPTSTR lpDNName, LPCTSTR lpComputerName,
                 PGROUP_POLICY_OBJECT *lpGPOList, PNETAPI32_API pNetAPI32, BOOL bMachineTokenOk)
{
    PGROUP_POLICY_OBJECT pGPOForcedList = NULL, pGPONonForcedList = NULL;
    PLDAP  pld = NULL;
    ULONG ulResult;
    BOOL bResult = FALSE;
    BOOL bBlock = FALSE;
    LPTSTR lpDSObject, lpTemp;
    LPTSTR lpSiteName = NULL;
    PLDAPMessage pLDAPMsg = NULL;
    TCHAR szGPOPath[MAX_PATH];
    TCHAR szGPOName[50];
    BOOL bVerbose, bDisabled;
    DWORD dwVersion, dwOptions;
    TCHAR szNamingContext[] = TEXT("configurationNamingContext");
    PGROUP_POLICY_OBJECT lpGPO, lpGPOTemp;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    PLDAP_API pldap_api;
    HANDLE hToken = NULL, hTempToken;
    DWORD dwFunctionalityVersion;
    PGROUP_POLICY_OBJECT pDeferredForcedList = NULL, pDeferredNonForcedList = NULL;
    DNENTRY *pDeferredOUList = NULL;    // List of deferred OUs
    TCHAR*  szDN;
    PSECUR32_API pSecur32Api;
    BOOL    bAddedOU = FALSE;
    PLDAP   pldMachine = 0;
    VOID *pData;

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  ********************************")));
    DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  Entering...")));

    //
    // Start with lpGPOList being a pointer to null
    //

    *lpGPOList = NULL;

    //
    // Check if we should be verbose to the event log
    //

    bVerbose = CheckForVerbosePolicy();


    //
    // Load the secur32 api
    //
    
    pSecur32Api = LoadSecur32();
    
    if (!pSecur32Api) {
        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to load secur32 api.")));
        goto Exit;
    }

    //
    // Load the ldap api
    //

    pldap_api = LoadLDAP();

    if (!pldap_api) {
        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to load ldap api.")));
        goto Exit;
    }

    //=========================================================================
    //
    // If we don't have a DS server or user / machine name, we can
    // skip the DS stuff and only check for a local GPO
    //
    //=========================================================================

    if (!lpHostName || !lpDNName) {
        DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  lpHostName or lpDNName is NULL.  Skipping DS stuff.")));
        goto CheckLocal;
    }


    //
    // Get the user or machine's token
    //

    if (bMachineTokenOk && (dwFlags & GPO_LIST_FLAG_MACHINE)) {

        hToken = GetMachineToken();

        if (!hToken) {
            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to get the machine token with  %d"),
                     GetLastError()));
            goto Exit;
        }

    } else {

        if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE,
                              TRUE, &hTempToken)) {
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE,
                                  &hTempToken)) {
                DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to get a token with  %d"),
                         GetLastError()));
                goto Exit;
            }
        }


        //
        // Duplicate it so it can be used for impersonation
        //

        if (!DuplicateTokenEx(hTempToken, TOKEN_IMPERSONATE | TOKEN_QUERY,
                              NULL, SecurityImpersonation, TokenImpersonation,
                              &hToken))
        {
            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to duplicate the token with  %d"),
                     GetLastError()));
            CloseHandle (hTempToken);
            goto Exit;
        }

        CloseHandle (hTempToken);
    }


    //
    // Get a connection to the DS
    //

    if ((lpHostName[0] == TEXT('\\')) && (lpHostName[1] == TEXT('\\')))  {
        lpHostName = lpHostName + 2;
    }

    pld = pldap_api->pfnldap_open( lpHostName, LDAP_PORT);

    if (!pld) {
        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  ldap_open for <%s> failed with = 0x%x or %d"),
                 lpHostName, pldap_api->pfnLdapGetLastError(), GetLastError()));
        LogEvent (TRUE, IDS_FAILED_DS_CONNECT, lpHostName, pldap_api->pfnLdapGetLastError());
        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  Server connection established.")));


    //
    // Turn on Packet integrity flag
    //
    
    pData = (VOID *) LDAP_OPT_ON;
    ulResult = pldap_api->pfnldap_set_option(pld, LDAP_OPT_SIGN, &pData);

    if (ulResult != LDAP_SUCCESS) {

        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to turn on LDAP_OPT_SIGN with %d"), ulResult));
        goto Exit;
    }
            

    //
    // Bind to the DS.
    //

    if ( dwFlags & GPO_LIST_FLAG_MACHINE) {

        //
        // For machine policies specifically ask for Kerberos as the only authentication
        // mechanism. Otherwise if Kerberos were to fail for some reason, then NTLM is used
        // and localsystem context has no real credentials, which means that we won't get
        // any GPOs back.
        //

        SEC_WINNT_AUTH_IDENTITY_EXW secIdentity;

        secIdentity.Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
        secIdentity.Length = sizeof(SEC_WINNT_AUTH_IDENTITY_EXW);
        secIdentity.User = 0;
        secIdentity.UserLength = 0;
        secIdentity.Domain = 0;
        secIdentity.DomainLength = 0;
        secIdentity.Password = 0;
        secIdentity.PasswordLength = 0;
        secIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        secIdentity.PackageList = wszKerberos;
        secIdentity.PackageListLength = lstrlen( wszKerberos );

        ulResult = pldap_api->pfnldap_bind_s (pld, NULL, (WCHAR *)&secIdentity, LDAP_AUTH_SSPI);

    } else
        ulResult = pldap_api->pfnldap_bind_s (pld, NULL, NULL, LDAP_AUTH_SSPI);

    if (ulResult != LDAP_SUCCESS) {
       DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  ldap_bind_s failed with = <%d>"),
                ulResult));
       LogEvent (TRUE, IDS_FAILED_DS_BIND, lpHostName, GetLastError());
       goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  Bound successfully.")));


    //=========================================================================
    //
    // Check the organizational units and domain for policy
    //
    //=========================================================================


    if (!(dwFlags & GPO_LIST_FLAG_SITEONLY)) {

        //
        // Loop through the DN Name to find each OU or the domain
        //

        lpDSObject = lpDNName;

        while (*lpDSObject) {

            //
            // See if the DN name starts with OU=
            //

            if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                               lpDSObject, 3, TEXT("OU="), 3) == CSTR_EQUAL) {
                if ( !AddOU( &pDeferredOUList, lpDSObject, GPLinkOrganizationalUnit ) ) {
                    goto Exit;
                }
            }

            //
            // See if the DN name starts with DC=
            //

            else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                    lpDSObject, 3, TEXT("DC="), 3) == CSTR_EQUAL) {
                if ( !AddOU( &pDeferredOUList, lpDSObject, GPLinkDomain ) ) {
                    goto Exit;
                }


                //
                // Now that we've found a DN name that starts with DC=
                // we exit the loop now.
                //

                break;
            }


            //
            // Move to the next chunk of the DN name
            //

            while (*lpDSObject && (*lpDSObject != TEXT(','))) {
                lpDSObject++;
            }

            if (*lpDSObject == TEXT(',')) {
                lpDSObject++;
            }
        }

        //
        // Evaluate deferred OUs with single Ldap query
        //
        // BUGBUG:site  Used to be located post CheckLocal:
        //

        if ( !EvaluateDeferredOUs(  pDeferredOUList,
                                    dwFlags,
                                    hToken,
                                    &pDeferredForcedList,
                                    &pDeferredNonForcedList,
                                    bVerbose,
                                    pld,
                                    pldap_api,
                                    &bBlock) )
        {
            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  EvaluateDeferredOUs failed. Exiting") ));
            goto Exit;
        }
    }


    //=========================================================================
    //
    // Check the site object for policy
    //
    //=========================================================================

    //
    // Query for the site name
    //

    ulResult = pNetAPI32->pfnDsGetSiteName(lpComputerName,  &lpSiteName);

    if (ulResult != ERROR_SUCCESS) {

        if ( ulResult == ERROR_NO_SITENAME ) {

            DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  No site name defined.  Skipping site policy.")));
            goto CheckLocal;

        } else {

            LogEvent (TRUE, IDS_FAILED_QUERY_SITE, ulResult);
            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  DSGetSiteName failed with %d, exiting."),
                      ulResult));
            goto Exit;

        }
    }

    //
    // Now we need to query for the domain name. 
    //

    pldMachine = GetMachineDomainDS( pNetAPI32, pldap_api );

    if ( pldMachine )
    {
        //
        // Now we need to query for the domain name.  This is done by
        // reading the operational attribute configurationNamingContext
        //
        LPTSTR szNamingContext = TEXT("configurationNamingContext");
        LPTSTR lpAttr[2];
        PLDAPMessage pLDAPMsg = 0;

        lpAttr[0] = szNamingContext;
        lpAttr[1] = 0;

        ulResult = pldap_api->pfnldap_search_s( pldMachine,
                                                TEXT(""),
                                                LDAP_SCOPE_BASE,
                                                TEXT("(objectClass=*)"),
                                                lpAttr,
                                                FALSE,
                                                &pLDAPMsg);


        if ( ulResult == LDAP_SUCCESS )
        {
            LPTSTR* pszValues = pldap_api->pfnldap_get_values( pldMachine, pLDAPMsg, szNamingContext );

            if ( pszValues )
            {
                TCHAR   szSite[512];
                //
                // Combine the domain name + site name to get the full
                // DS object path
                //

                wsprintf( szSite, TEXT("CN=%s,CN=Sites,%s"), lpSiteName, *pszValues );


                if (SearchDSObject (szSite, dwFlags, hToken, &pDeferredForcedList, &pDeferredNonForcedList,
                                    bVerbose, GPLinkSite, pldMachine,
                                    pldap_api, NULL, &bBlock)) {

                    bAddedOU = TRUE;

                } else {

                    DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  SearchDSObject failed.  Exiting.")));
                }

                pldap_api->pfnldap_value_free( pszValues );
            } 
            else
            {
                DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to get values.")));
            }

            pldap_api->pfnldap_msgfree( pLDAPMsg );
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  ldap_search_s failed with = <%d>"), ulResult) );
            LogEvent (TRUE, IDS_FAILED_ROOT_SEARCH, GetLastError());
        }

    }

    if ( !bAddedOU )
    {
        goto Exit;
    }

CheckLocal:

    //
    // BugBug:site  Used to defer OU search to this point and include site
    // information in it.
    //

    //
    // Evaluate all GPOs deferred so far with single Ldap query
    //

    if ( !EvaluateDeferredGPOs( pld,
                                pldap_api,
                                lpHostName,
                                dwFlags,
                                hToken,
                                bVerbose,
                                pDeferredForcedList,
                                pDeferredNonForcedList,
                                &pGPOForcedList,
                                &pGPONonForcedList ) )
    {
        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  EvaluateDeferredGPOs failed. Exiting") ));
        goto Exit;
    }


    //=========================================================================
    //
    // Check if we have a local GPO. If so, add it to the list
    //
    //=========================================================================

    if (!(dwFlags & GPO_LIST_FLAG_SITEONLY)) {

        ExpandEnvironmentStrings (LOCAL_GPO_DIRECTORY, szGPOPath, ARRAYSIZE(szGPOPath));

        if (GetFileAttributesEx (szGPOPath, GetFileExInfoStandard, &fad) &&
            (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

            LoadString (g_hDllInstance, IDS_LOCALGPONAME, szGPOName, ARRAYSIZE(szGPOName));

            DmAssert( lstrlen(szGPOPath) + lstrlen(TEXT("gpt.ini")) + 1 < MAX_PATH );

            lpTemp = CheckSlash (szGPOPath);
            lstrcpy (lpTemp, TEXT("gpt.ini"));

            //
            // Check the functionalty version number
            //

            dwFunctionalityVersion = GetPrivateProfileInt(TEXT("General"), GPO_FUNCTIONALITY_VERSION, 2, szGPOPath);
            if (dwFunctionalityVersion < 2) {

                DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  GPO %s was created by an old version of the Group Policy Editor.  It will be skipped."), szGPOName));
                if (bVerbose)
                    LogEvent (FALSE, IDS_GPO_TOO_OLD, szGPOName);

            } else {

                //
                // Check if this GPO is enabled
                //

                bDisabled = FALSE;

                dwOptions = GetPrivateProfileInt(TEXT("General"), TEXT("Options"), 0, szGPOPath);

                if (((dwFlags & GPO_LIST_FLAG_MACHINE) &&
                     (dwOptions & GPO_OPTION_DISABLE_MACHINE)) ||
                     (!(dwFlags & GPO_LIST_FLAG_MACHINE) &&
                     (dwOptions & GPO_OPTION_DISABLE_USER))) {
                     bDisabled = TRUE;
                }

                //
                // Check if the version number is 0, if so there isn't any data
                // in the GPO and we can skip it
                //

                dwVersion = GetPrivateProfileInt(TEXT("General"), TEXT("Version"), 0, szGPOPath);

                if (dwFlags & GPO_LIST_FLAG_MACHINE) {
                    dwVersion = MAKELONG (LOWORD(dwVersion), LOWORD(dwVersion));
                } else {
                    dwVersion = MAKELONG (HIWORD(dwVersion), HIWORD(dwVersion));
                }

                if (dwVersion == 0) {
                    DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  GPO %s doesn't contain any data since the version number is 0.  It will be skipped."), szGPOName));
                    if (bVerbose)
                        LogEvent (FALSE, IDS_GPO_NO_DATA, szGPOName);
                }

                if (!bDisabled && dwVersion != 0) {

                    DWORD dwSize = MAX_PATH;
                    DWORD dwCount = 0;
                    BOOL bOk = FALSE;
                    TCHAR *pszExtensions = 0;

                    //
                    // Read list of extension guids
                    //

                    pszExtensions = LocalAlloc( LPTR, dwSize * sizeof(TCHAR) );
                    if ( pszExtensions == 0 ) {
                        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to allocate memory.")));
                        goto Exit;
                    }

                    dwCount = GetPrivateProfileString( TEXT("General"),
                                                       dwFlags & GPO_LIST_FLAG_MACHINE ? GPO_MACHEXTENSION_NAMES
                                                                                       : GPO_USEREXTENSION_NAMES,
                                                       TEXT(""),
                                                       pszExtensions,
                                                       dwSize,
                                                       szGPOPath );

                    while ( dwCount == dwSize - 1 )
                    {
                        //
                        // Value has been truncated, so retry with larger buffer
                        //

                        LocalFree( pszExtensions );

                        dwSize *= 2;
                        pszExtensions = LocalAlloc( LPTR, dwSize * sizeof(TCHAR) );
                        if ( pszExtensions == 0 ) {
                            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to allocate memory.")));
                            goto Exit;
                        }

                        dwCount = GetPrivateProfileString( TEXT("General"),
                                                           dwFlags & GPO_LIST_FLAG_MACHINE ? GPO_MACHEXTENSION_NAMES
                                                                                           : GPO_USEREXTENSION_NAMES,
                                                           TEXT(""),
                                                           pszExtensions,
                                                           dwSize,
                                                           szGPOPath );
                    }

                    if ( lstrcmpi( pszExtensions, TEXT("")) == 0 || lstrcmpi( pszExtensions, TEXT(" ")) == 0 ) {
                        //
                        // Extensions property was not found
                        //

                        LocalFree( pszExtensions );
                        pszExtensions = 0;
                    }

                    //
                    // Tack on the correct subdirectory name
                    //

                    DmAssert( lstrlen(szGPOPath) + lstrlen(TEXT("Machine")) + 1 < MAX_PATH );

                    if (dwFlags & GPO_LIST_FLAG_MACHINE) {
                        lstrcpy (lpTemp, TEXT("Machine"));
                    } else {
                        lstrcpy (lpTemp, TEXT("User"));
                    }


                    //
                    // Add this to the list of paths
                    //

                    bOk = AddGPO (&pGPONonForcedList, 0, dwVersion, NULL, szGPOPath,
                                  szGPOName, szGPOName, pszExtensions, GPLinkMachine, TEXT("Local"), 0, TRUE,
                                  FALSE, bVerbose);

                    if ( pszExtensions )
                        LocalFree( pszExtensions );

                    if ( !bOk ) {
                        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to add local group policy object to the list.")));
                        goto Exit;
                    }

                    if (bVerbose) LogEvent (FALSE, IDS_FOUND_LOCAL_GPO);

                } else {
                    if (bVerbose) LogEvent (FALSE, IDS_NO_LOCAL_GPO);
                }
            }   // else - if dwfuncversion

        }
    }

    //
    // Merge the forced and nonforced lists together
    //

    if (pGPOForcedList && !pGPONonForcedList) {

        *lpGPOList = pGPOForcedList;

    } else if (!pGPOForcedList && pGPONonForcedList) {

        *lpGPOList = pGPONonForcedList;

    } else if (pGPOForcedList && pGPONonForcedList) {

        lpGPO = pGPONonForcedList;

        while (lpGPO->pNext) {
            lpGPO = lpGPO->pNext;
        }

        lpGPO->pNext = pGPOForcedList;
        pGPOForcedList->pPrev = lpGPO;

        *lpGPOList = pGPONonForcedList;
    }


    //
    // Success
    //

    bResult = TRUE;

Exit:

    //
    // Free any GPOs we found
    //

    if (!bResult) {
        FreeGPOList( pGPOForcedList );
        FreeGPOList( pGPONonForcedList );
    }

    //
    // Free temporary OU list
    //

    while ( pDeferredOUList ) {
        DNENTRY *pTemp = pDeferredOUList->pNext;
        FreeDnEntry( pDeferredOUList );
        pDeferredOUList = pTemp;
    }

    //
    // Free temporary deferred GPO lists
    //

    FreeGPOList( pDeferredForcedList );
    FreeGPOList( pDeferredNonForcedList );

    if (pld) {
        pldap_api->pfnldap_unbind (pld);
    }

    if ( pldMachine )
    {
        pldap_api->pfnldap_unbind( pldMachine );
    }

    
    if (lpSiteName) {
        pNetAPI32->pfnNetApiBufferFree(lpSiteName);
    }

    DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  Leaving with %d"), bResult));
    DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  ********************************")));

    if ( hToken )
    {
        CloseHandle( hToken );
    }

    return bResult;
}

//*************************************************************
//
//  GetGPOList()
//
//  Purpose:    Retreives the list of GPOs for the specified
//              user or machine
//
//  Parameters:  hToken     - User or machine token, if NULL,
//                            lpName and lpDCName must be supplied
//               lpName     - User or machine name in DN format,
//                            if hToken is supplied, this must be NULL
//               lpHostName - Host name.  This should be a domain's
//                            dn name for best performance.  Otherwise
//                            it can also be a DC name.  If hToken is supplied,
//                            this must be NULL
//               lpComputerName - Computer named used to determine site
//                                information.  Can be NULL which means
//                                use the local machine
//               dwFlags  - Flags field
//               pGPOList - Address of a pointer which receives
//                          the link list of GPOs
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL WINAPI GetGPOList (HANDLE hToken, LPCTSTR lpName, LPCTSTR lpHostName,
                        LPCTSTR lpComputerName, DWORD dwFlags,
                        PGROUP_POLICY_OBJECT *pGPOList)
{
    PDOMAIN_CONTROLLER_INFO pDCI = NULL;
    TCHAR szDomainDN[200];
    BOOL bResult = FALSE;
    LPTSTR lpDomainDN, lpDNName, lpTemp, lpDomain = NULL, lpUserName = NULL;
    DWORD dwResult;
    HANDLE hOldToken = 0;
    PNETAPI32_API pNetAPI32;


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("GetGPOList: Entering.")));
    DebugMsg((DM_VERBOSE, TEXT("GetGPOList:  hToken = 0x%x"), (hToken ? hToken : 0)));
    DebugMsg((DM_VERBOSE, TEXT("GetGPOList:  lpName = <%s>"), (lpName ? lpName : TEXT("NULL"))));
    DebugMsg((DM_VERBOSE, TEXT("GetGPOList:  lpHostName = <%s>"), (lpHostName ? lpHostName : TEXT("NULL"))));
    DebugMsg((DM_VERBOSE, TEXT("GetGPOList:  dwFlags = 0x%x"), dwFlags));


    //
    // Check parameters
    //

    if (hToken) {
        if (lpName || lpHostName) {
            DebugMsg((DM_WARNING, TEXT("GetGPOList: lpName and lpHostName must be NULL")));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    } else {
        if (!lpName || !lpHostName) {
            DebugMsg((DM_WARNING, TEXT("GetGPOList: lpName and lpHostName must be valid")));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    if (!pGPOList) {
        DebugMsg((DM_WARNING, TEXT("GetGPOList: pGPOList is null")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    //
    // Load netapi32
    //

    pNetAPI32 = LoadNetAPI32();

    if (!pNetAPI32) {
        DebugMsg((DM_WARNING, TEXT("GetGPOList:  Failed to load netapi32 with %d."),
                 GetLastError()));
        goto Exit;
    }


    //
    // If an hToken was offered, then we need to get the name and
    // domain DN name
    //

    if (hToken) {

        //
        // Impersonate the user / machine
        //

        if (!ImpersonateUser(hToken, &hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("GetGPOList: Failed to impersonate user")));
            return FALSE;
        }


        //
        // Get the username in DN format
        //

        lpUserName = MyGetUserName (NameFullyQualifiedDN);

        if (!lpUserName) {
            DebugMsg((DM_WARNING, TEXT("GetGPOList:  MyGetUserName failed for DN style name with %d"),
                     GetLastError()));
            LogEvent (TRUE, IDS_FAILED_USERNAME, GetLastError());
            goto Exit;
        }

        lpDNName = lpUserName;
        DebugMsg((DM_VERBOSE, TEXT("GetGPOList:  Queried lpDNName = <%s>"), lpDNName));


        //
        // Get the username in NT4 format
        //

        lpDomain = MyGetUserName (NameSamCompatible);

        if (!lpDomain) {
            DebugMsg((DM_WARNING, TEXT("GetGPOList:  MyGetUserName failed for NT4 style name with %d"),
                     GetLastError()));
            LogEvent (TRUE, IDS_FAILED_USERNAME, GetLastError());
            goto Exit;
        }


        //
        // Look for the \ between the domain and username and replace
        // it with a NULL
        //

        lpTemp = lpDomain;

        while (*lpTemp && ((*lpTemp) != TEXT('\\')))
            lpTemp++;


        if (*lpTemp != TEXT('\\')) {
            DebugMsg((DM_WARNING, TEXT("GetGPOList:  Failed to find slash in NT4 style name:  <%s>"),
                     lpDomain));
            goto Exit;
        }

        *lpTemp = TEXT('\0');


        //
        // Check this domain for a DC
        //

        dwResult = pNetAPI32->pfnDsGetDcName (NULL, lpDomain, NULL, NULL,
                                   DS_DIRECTORY_SERVICE_PREFERRED |
                                   DS_IS_FLAT_NAME |
                                   DS_RETURN_DNS_NAME,
                                   &pDCI);

        if (dwResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("GetGPOList:  DSGetDCName failed with %d for <%s>"),
                     dwResult, lpDomain));
            goto Exit;
        }


        //
        // Found a DC, does it have a DS ?
        //

        if (!(pDCI->Flags & DS_DS_FLAG)) {
            pNetAPI32->pfnNetApiBufferFree(pDCI);
            DebugMsg((DM_WARNING, TEXT("GetGPOList:  The domain <%s> does not have a DS"),
                     lpDomain));
            goto Exit;
        }

        lstrcpyn (szDomainDN, pDCI->DomainName, ARRAYSIZE(szDomainDN));
        lpDomainDN = szDomainDN;
        DebugMsg((DM_VERBOSE, TEXT("GetGPOList:  lpDomainDN = <%s>"), lpDomainDN));

        pNetAPI32->pfnNetApiBufferFree(pDCI);

    } else {

        //
        // Use the server and DN name passed in
        //

        lpDomainDN = (LPTSTR)lpHostName;
        lpDNName = (LPTSTR)lpName;
    }


    //
    // Call to get the list of GPOs
    //

    bResult = GetGPOInfo (dwFlags, lpDomainDN, lpDNName,
                          lpComputerName, pGPOList, pNetAPI32, FALSE);


Exit:

    //
    // Stop impersonating if a hToken was given
    //

    if ( hOldToken ) {
        RevertToUser(&hOldToken);
    }

    if (lpDomain) {
        LocalFree (lpDomain);
    }

    if (lpUserName) {
        LocalFree (lpUserName);
    }


    DebugMsg((DM_VERBOSE, TEXT("GetGPOList: Leaving with %d"), bResult));

    return bResult;
}

//*************************************************************
//
//  FreeGPOList()
//
//  Purpose:    Free's the link list of GPOs
//
//  Parameters: pGPOList - Pointer to the head of the list
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL WINAPI FreeGPOList (PGROUP_POLICY_OBJECT pGPOList)
{
    PGROUP_POLICY_OBJECT pGPOTemp;

    while (pGPOList) {
        pGPOTemp = pGPOList->pNext;
        LocalFree (pGPOList);
        pGPOList = pGPOTemp;
    }

    return TRUE;
}


//*************************************************************
//
//  FreeLists()
//
//  Purpose:    Free's the lpExtFilterList and/or lpGPOList
//
//  Parameters: lpGPOInfo - GPO info
//
//*************************************************************

void FreeLists( LPGPOINFO lpGPOInfo )
{
    LPEXTFILTERLIST pExtFilterList = lpGPOInfo->lpExtFilterList;

    //
    // If bXferToExtList is True then it means that lpGPOInfo->lpExtFilterList
    // owns the list of GPOs. Otherwise lpGPOInfo->lpGPOList owns the list
    // of GPOs.
    //

    while ( pExtFilterList ) {

        LPEXTFILTERLIST pTemp = pExtFilterList->pNext;

        FreeExtList( pExtFilterList->lpExtList );

        if ( lpGPOInfo->bXferToExtList )
            LocalFree( pExtFilterList->lpGPO );

        LocalFree( pExtFilterList );
        pExtFilterList = pTemp;
    }

    if ( !lpGPOInfo->bXferToExtList )
        FreeGPOList( lpGPOInfo->lpGPOList );
}


//*************************************************************
//
//  FreeExtList()
//
//  Purpose:    Free's the lpExtList
//
//  Parameters: pExtList - Extensions list
//
//*************************************************************

void FreeExtList( LPEXTLIST pExtList )
{
    while (pExtList) {

        LPEXTLIST pTemp = pExtList->pNext;
        LocalFree( pExtList );
        pExtList = pTemp;
    }
}



//*************************************************************
//
//  ShutdownGPOProcessing()
//
//  Purpose:    Begins aborting GPO processing
//
//  Parameters: bMachine    -  Shutdown machine or user processing ?
//
//*************************************************************

void WINAPI ShutdownGPOProcessing( BOOL bMachine )
{
    if ( bMachine )
        g_bStopMachGPOProcessing = TRUE;
    else
        g_bStopUserGPOProcessing = TRUE;
}


//*************************************************************
//
//  InitializeGPOCriticalSection, CloseGPOCriticalSection
//
//  Purpose:   Initialization and cleanup routines for critical sections
//
//*************************************************************

void InitializeGPOCriticalSection()
{
    InitializeCriticalSection( &g_GPOCS );
    InitializeCriticalSection( &g_StatusCallbackCS );
}


void CloseGPOCriticalSection()
{
    DeleteCriticalSection( &g_StatusCallbackCS );
    DeleteCriticalSection( &g_GPOCS );
}


//*************************************************************
//
//  ProcessGroupPolicyCompleted()
//
//  Purpose:    Callback for asynchronous completion of an extension
//
//  Parameters: refExtensionId    -  Unique guid of extension
//              pAsyncHandle      -  Completion context
//              dwStatus          -  Asynchronous completion status
//
//  Returns:    Win32 error code
//
//*************************************************************

DWORD ProcessGroupPolicyCompleted( REFGPEXTENSIONID extensionGuid,
                                   ASYNCCOMPLETIONHANDLE pAsyncHandle,
                                   DWORD dwStatus )
{
    DWORD dwRet = E_FAIL;
    TCHAR szExtension[64];
    PGROUP_POLICY_OBJECT pGPOList = NULL;
    LPGPOINFO lpGPOInfo = NULL;
    BOOL bUsePerUserLocalSetting = FALSE;
    LPTSTR lpwszSidUser = NULL;
    DWORD dwCurrentTime = GetCurTime();

    LPGPINFOHANDLE pGPHandle = (LPGPINFOHANDLE) pAsyncHandle;

    if ( extensionGuid == 0 )
        return ERROR_INVALID_PARAMETER;

    GuidToString( extensionGuid, szExtension );

    DebugMsg((DM_VERBOSE, TEXT("ProcessGroupPolicyCompleted: Entering. Extension = %s, dwStatus = 0x%x"),
              szExtension, dwStatus));

    EnterCriticalSection( &g_GPOCS );

    if ( !(pGPHandle == g_pMachGPInfo || pGPHandle == g_pUserGPInfo) ) {
        DebugMsg((DM_WARNING, TEXT("Extension %s asynchronous completion is stale"),
                  szExtension));
        goto Exit;
    }

    DmAssert( pGPHandle->pGPOInfo != NULL );

    if ( pGPHandle->pGPOInfo == NULL ) {
        DebugMsg((DM_WARNING, TEXT("Extension %s asynchronous completion has invalid pGPHandle->pGPOInfo"),
                  szExtension));
        goto Exit;
    }

    lpGPOInfo = pGPHandle->pGPOInfo;

    if ( (lpGPOInfo->dwFlags & GP_MACHINE) && g_bStopMachGPOProcessing
         || !(lpGPOInfo->dwFlags & GP_MACHINE) && g_bStopUserGPOProcessing ) {

        DebugMsg((DM_WARNING, TEXT("Extension %s asynchronous completion, aborting due to machine shutdown or logoff"),
                  szExtension));
        LogEvent (TRUE, IDS_GPO_PROC_STOPPED);
        goto Exit;

    }

    if ( dwStatus != ERROR_SUCCESS ) {

        //
        // Extension returned error code, so no need to update history
        //

        dwRet = ERROR_SUCCESS;
        goto Exit;
    }

    if ( pGPHandle == 0 ) {
         DebugMsg((DM_WARNING, TEXT("Extension %s is using 0 as asynchronous completion handle"),
                   szExtension));
         goto Exit;
    }

    bUsePerUserLocalSetting = !(lpGPOInfo->dwFlags & GP_MACHINE)
                              && ExtensionHasPerUserLocalSetting( szExtension, HKEY_LOCAL_MACHINE );

    if ( bUsePerUserLocalSetting ) {

        lpwszSidUser = GetSidString( lpGPOInfo->hToken );
        if ( lpwszSidUser == NULL ) {
            DebugMsg((DM_WARNING, TEXT("ProcesGroupPolicyCompleted: GetSidString failed.")));
            goto Exit;
        }

    }

    if ( ReadGPOList( szExtension, lpGPOInfo->hKeyRoot,
                      HKEY_LOCAL_MACHINE,
                      lpwszSidUser,
                      TRUE, &pGPOList ) ) {

        if ( SaveGPOList( szExtension, lpGPOInfo,
                          HKEY_LOCAL_MACHINE,
                          lpwszSidUser,
                          FALSE, pGPOList ) )
            dwRet = ERROR_SUCCESS;
        else {
            DebugMsg((DM_WARNING, TEXT("Extension %s asynchronous completion, failed to save GPOList"),
                      szExtension));
        }
    } else {
        DebugMsg((DM_WARNING, TEXT("Extension %s asynchronous completion, failed to read shadow GPOList"),
                  szExtension));
    }

Exit:

    if ( lpwszSidUser )
        DeleteSidString( lpwszSidUser );

    if ( dwRet == ERROR_SUCCESS ) {

        //
        // Override E_PENDING status code with status returned
        //

        BOOL bUsePerUserLocalSetting = !(lpGPOInfo->dwFlags & GP_MACHINE) && lpGPOInfo->lpwszSidUser != NULL;

        WriteStatus( szExtension, lpGPOInfo,
                     bUsePerUserLocalSetting ? lpGPOInfo->lpwszSidUser : NULL,
                     dwStatus, dwCurrentTime,
                     (lpGPOInfo->dwFlags & GP_SLOW_LINK) != 0 );

    }

    LeaveCriticalSection( &g_GPOCS );

    DebugMsg((DM_VERBOSE, TEXT("ProcessGroupPolicyCompleted: Leaving. Extension = %s, Return status dwRet = 0x%x"),
              szExtension, dwRet));

    return dwRet;
}



//*************************************************************
//
//  DebugPrintGPOList()
//
//  Purpose:    Prints GPO list
//
//  Parameters: lpGPOInfo    -  GPO Info
//
//*************************************************************

void DebugPrintGPOList( LPGPOINFO lpGPOInfo )
{
    //
    // If we are in verbose mode, put the list of GPOs in the event log
    //

    PGROUP_POLICY_OBJECT lpGPO = NULL;
    DWORD dwSize;

#if DBG
    if (TRUE) {
#else
    if (lpGPOInfo->dwFlags & GP_VERBOSE) {
#endif
        LPTSTR lpTempList;

        dwSize = 10;
        lpGPO = lpGPOInfo->lpGPOList;
        while (lpGPO) {
            if (lpGPO->lpDisplayName) {
                dwSize += (lstrlen (lpGPO->lpDisplayName) + 4);
            }
            lpGPO = lpGPO->pNext;
        }

        lpTempList = LocalAlloc (LPTR, (dwSize * sizeof(TCHAR)));

        if (lpTempList) {

            lstrcpy (lpTempList, TEXT(""));

            lpGPO = lpGPOInfo->lpGPOList;
            while (lpGPO) {
                if (lpGPO->lpDisplayName) {
                    lstrcat (lpTempList, TEXT("\""));
                    lstrcat (lpTempList, lpGPO->lpDisplayName);
                    lstrcat (lpTempList, TEXT("\" "));
                }
                lpGPO = lpGPO->pNext;
            }

            if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                LogEvent (FALSE, IDS_GPO_LIST, lpTempList);
            }

            DebugMsg((DM_VERBOSE, TEXT("DebugPrintGPOList: List of GPO(s) to process: %s"),
                     lpTempList));

            LocalFree (lpTempList);
        }
    }
}



//*************************************************************
//
//  GetAppliedGPOList()
//
//  Purpose:    Queries for the list of applied Group Policy
//              Objects for the specified user or machine
//              and specified client side extension.
//
//  Parameters: dwFlags    -  User or machine policy, if it is GPO_LIST_FLAG_MACHINE
//                            then machine policy
//              pMachineName  - Name of remote computer in the form \\computername. If null
//                              then local computer is used.
//              pSidUser      - Security id of user (relevant for user policy). If pMachineName is
//                              null and pSidUser is null then it means current logged on user.
//                              If pMachine is null and pSidUser is non-null then it means user
//                              represented by pSidUser on local machine. If pMachineName is non-null
//                              then and if dwFlags specifies user policy, then pSidUser must be
//                              non-null.
//              pGuid      -  Guid of the specified extension
//              ppGPOList  -  Address of a pointer which receives the link list of GPOs
//
//  Returns:    Win32 error code
//
//*************************************************************

DWORD GetAppliedGPOList( DWORD dwFlags,
                         LPCTSTR pMachineName,
                         PSID pSidUser,
                         GUID *pGuidExtension,
                         PGROUP_POLICY_OBJECT *ppGPOList)
{
    DWORD dwRet = E_FAIL;
    TCHAR szExtension[64];
    BOOL bOk;
    BOOL bMachine = dwFlags & GPO_LIST_FLAG_MACHINE;
    NTSTATUS ntStatus;
    UNICODE_STRING  unicodeStr;

    *ppGPOList = 0;

    if ( pGuidExtension == 0 )
        return ERROR_INVALID_PARAMETER;

    GuidToString( pGuidExtension, szExtension );

    DebugMsg((DM_VERBOSE, TEXT("GetAppliedGPOList: Entering. Extension = %s"),
              szExtension));

    if ( pMachineName == NULL ) {

        //
        // Local case
        //

        if ( bMachine ) {

            bOk = ReadGPOList( szExtension,
                               HKEY_LOCAL_MACHINE,
                               HKEY_LOCAL_MACHINE,
                               0,
                               FALSE, ppGPOList );

            return bOk ? ERROR_SUCCESS : E_FAIL;

        } else {

            BOOL bUsePerUserLocalSetting = ExtensionHasPerUserLocalSetting( szExtension, HKEY_LOCAL_MACHINE );
            LPTSTR lpwszSidUser = NULL;

            if ( pSidUser == NULL ) {

                //
                // Current logged on user
                //

                if ( bUsePerUserLocalSetting ) {

                    HANDLE hToken = NULL;
                    if (!OpenThreadToken (GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken)) {
                        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
                            DebugMsg((DM_WARNING, TEXT("GetAppliedGPOList:  Failed to get user token with  %d"),
                                      GetLastError()));
                            return GetLastError();
                        }
                    }

                    lpwszSidUser = GetSidString( hToken );
                    CloseHandle( hToken );

                    if ( lpwszSidUser == NULL ) {
                        DebugMsg((DM_WARNING, TEXT("GetAppliedGPOList: GetSidString failed.")));
                        return E_FAIL;
                    }

                }

                bOk = ReadGPOList( szExtension,
                                   HKEY_CURRENT_USER,
                                   HKEY_LOCAL_MACHINE,
                                   lpwszSidUser,
                                   FALSE, ppGPOList );
                if ( lpwszSidUser )
                    DeleteSidString( lpwszSidUser );

                return bOk ? ERROR_SUCCESS : E_FAIL;

            } else {

                //
                // User represented by pSidUser
                //

                HKEY hSubKey;

                ntStatus = RtlConvertSidToUnicodeString( &unicodeStr,
                                                         pSidUser,
                                                         (BOOLEAN)TRUE  ); // Allocate
                if ( !NT_SUCCESS(ntStatus) )
                    return E_FAIL;

                dwRet = RegOpenKeyEx ( HKEY_USERS, unicodeStr.Buffer, 0, KEY_READ, &hSubKey);

                if (dwRet != ERROR_SUCCESS) {
                    RtlFreeUnicodeString(&unicodeStr);

                    if (dwRet == ERROR_FILE_NOT_FOUND)
                        return ERROR_SUCCESS;
                    else
                        return dwRet;
                }

                bOk = ReadGPOList( szExtension,
                                   hSubKey,
                                   HKEY_LOCAL_MACHINE,
                                   bUsePerUserLocalSetting ? unicodeStr.Buffer : NULL,
                                   FALSE, ppGPOList );

                RtlFreeUnicodeString(&unicodeStr);
                RegCloseKey(hSubKey);

                return bOk ? ERROR_SUCCESS : E_FAIL;

            }  // else if psiduser == null

        }      // else if bmachine

    } else {   // if pmachine == null

        //
        // Remote case
        //

        if ( bMachine ) {

            HKEY hKeyRemote;

            dwRet = RegConnectRegistry( pMachineName,
                                        HKEY_LOCAL_MACHINE,
                                        &hKeyRemote );
            if ( dwRet != ERROR_SUCCESS )
                return dwRet;

            bOk = ReadGPOList( szExtension,
                               hKeyRemote,
                               hKeyRemote,
                               0,
                               FALSE, ppGPOList );
            RegCloseKey( hKeyRemote );

            dwRet = bOk ? ERROR_SUCCESS : E_FAIL;
            return dwRet;

        } else {

            //
            // Remote user
            //

            HKEY hKeyRemoteMach;
            BOOL bUsePerUserLocalSetting;

            if ( pSidUser == NULL )
                return ERROR_INVALID_PARAMETER;

            ntStatus = RtlConvertSidToUnicodeString( &unicodeStr,
                                                     pSidUser,
                                                     (BOOLEAN)TRUE  ); // Allocate
            if ( !NT_SUCCESS(ntStatus) )
                return E_FAIL;

            dwRet = RegConnectRegistry( pMachineName,
                                        HKEY_LOCAL_MACHINE,
                                        &hKeyRemoteMach );

            bUsePerUserLocalSetting = ExtensionHasPerUserLocalSetting( szExtension, hKeyRemoteMach );

            if ( bUsePerUserLocalSetting ) {

                //
                // Account for per user local settings
                //

                bOk = ReadGPOList( szExtension,
                                   hKeyRemoteMach,
                                   hKeyRemoteMach,
                                   unicodeStr.Buffer,
                                   FALSE, ppGPOList );

                RtlFreeUnicodeString(&unicodeStr);
                RegCloseKey(hKeyRemoteMach);

                return bOk ? ERROR_SUCCESS : E_FAIL;

            } else {

                HKEY hKeyRemote, hSubKeyRemote;

                RegCloseKey( hKeyRemoteMach );

                dwRet = RegConnectRegistry( pMachineName,
                                            HKEY_USERS,
                                            &hKeyRemote );
                if ( dwRet != ERROR_SUCCESS ) {
                    RtlFreeUnicodeString(&unicodeStr);
                    return dwRet;
                }

                dwRet = RegOpenKeyEx (hKeyRemote, unicodeStr.Buffer, 0, KEY_READ, &hSubKeyRemote);

                RtlFreeUnicodeString(&unicodeStr);

                if (dwRet != ERROR_SUCCESS) {
                    RegCloseKey(hKeyRemote);

                    if (dwRet == ERROR_FILE_NOT_FOUND)
                        return ERROR_SUCCESS;
                    else
                        return dwRet;
                }

                bOk = ReadGPOList( szExtension,
                                   hSubKeyRemote,
                                   hSubKeyRemote,
                                   0,
                                   FALSE, ppGPOList );

                RegCloseKey(hSubKeyRemote);
                RegCloseKey(hKeyRemote);

                return bOk ? ERROR_SUCCESS : E_FAIL;

            } // else if bUsePerUserLocalSettings

        } // else if bMachine

    }   // else if pMachName == null

    return dwRet;
}




//*************************************************************
//
//  ExtensionHasPerUserLocalSetting()
//
//  Purpose:    Checks registry if extension has per user local setting
//
//  Parameters: pwszExtension - Extension guid
//              hKeyRoot      - Registry root
//
//  Returns:    True if extension has per user local setting
//              False otherwise
//
//*************************************************************

BOOL ExtensionHasPerUserLocalSetting( LPTSTR pszExtension, HKEY hKeyRoot )
{
    TCHAR szKey[200];
    DWORD dwType, dwSetting = 0, dwSize = sizeof(DWORD);
    LONG lResult;
    HKEY hKey;

    wsprintf ( szKey, GP_EXTENSIONS_KEY,
               pszExtension );

    lResult = RegOpenKeyEx ( hKeyRoot, szKey, 0, KEY_READ, &hKey);
    if ( lResult != ERROR_SUCCESS )
        return FALSE;

    lResult = RegQueryValueEx( hKey, TEXT("PerUserLocalSettings"), NULL,
                               &dwType, (LPBYTE) &dwSetting,
                               &dwSize );
    RegCloseKey( hKey );

    if (lResult == ERROR_SUCCESS)
        return dwSetting;
    else
        return FALSE;
}

//*************************************************************
//
//  UserPolicyCallback()
//
//  Purpose:    Callback function for status UI messages
//
//  Parameters: bVerbose  - Verbose message or not
//              lpMessage - Message text
//
//  Return:     ERROR_SUCCESS if successful
//              Win32 error code if an error occurs
//
//*************************************************************

DWORD UserPolicyCallback (BOOL bVerbose, LPWSTR lpMessage)
{
    WCHAR szMsg[100];
    LPWSTR lpMsg;
    DWORD dwResult = ERROR_INVALID_FUNCTION;


    if (lpMessage) {
        lpMsg = lpMessage;
    } else {
        LoadString (g_hDllInstance, IDS_USER_SETTINGS, szMsg, 100);
        lpMsg = szMsg;
    }

    DebugMsg((DM_VERBOSE, TEXT("UserPolicyCallback: Setting status UI to %s"), lpMsg));

    EnterCriticalSection (&g_StatusCallbackCS);

    if (g_pStatusMessageCallback) {
        dwResult = g_pStatusMessageCallback(bVerbose, lpMsg);
    } else {
        DebugMsg((DM_WARNING, TEXT("UserPolicyCallback: Extension requested status UI when status UI is not available.")));
    }

    LeaveCriticalSection (&g_StatusCallbackCS);

    return dwResult;
}

//*************************************************************
//
//  MachinePolicyCallback()
//
//  Purpose:    Callback function for status UI messages
//
//  Parameters: bVerbose  - Verbose message or not
//              lpMessage - Message text
//
//  Return:     ERROR_SUCCESS if successful
//              Win32 error code if an error occurs
//
//*************************************************************

DWORD MachinePolicyCallback (BOOL bVerbose, LPWSTR lpMessage)
{
    WCHAR szMsg[100];
    LPWSTR lpMsg;
    DWORD dwResult = ERROR_INVALID_FUNCTION;


    if (lpMessage) {
        lpMsg = lpMessage;
    } else {
        LoadString (g_hDllInstance, IDS_COMPUTER_SETTINGS, szMsg, 100);
        lpMsg = szMsg;
    }

    DebugMsg((DM_VERBOSE, TEXT("MachinePolicyCallback: Setting status UI to %s"), lpMsg));

    EnterCriticalSection (&g_StatusCallbackCS);

    if (g_pStatusMessageCallback) {
        dwResult = g_pStatusMessageCallback(bVerbose, lpMsg);
    } else {
        DebugMsg((DM_WARNING, TEXT("MachinePolicyCallback: Extension requested status UI when status UI is not available.")));
    }

    LeaveCriticalSection (&g_StatusCallbackCS);

    return dwResult;
}

//*************************************************************
//
//  GetMachineDomainDS()
//
//  Purpose:    Obtain the machine domain DS
//
//  Parameters: pNetApi32	- netapi32.dll
//              pLdapApi	- wldap32.dll
//
//  Return:     valid PLDAP if successful
//              0 if an error occurs
//
//*************************************************************
PLDAP
GetMachineDomainDS( PNETAPI32_API pNetApi32, PLDAP_API pLdapApi )
{
    PLDAP	pld = 0;
	
    DWORD	dwResult = 0;
    PDOMAIN_CONTROLLER_INFO pDCI = 0;
    ULONG ulResult;
    VOID *pData;

    //
    // get the machine domain name
    //

    dwResult = pNetApi32->pfnDsGetDcName(   0,
                                            0,
                                            0,
                                            0,
                                            DS_DIRECTORY_SERVICE_REQUIRED,
                                            &pDCI);
    if ( dwResult == ERROR_SUCCESS )
    {
        SEC_WINNT_AUTH_IDENTITY_EXW secIdentity;
	
        pld = pLdapApi->pfnldap_open( pDCI->DomainName, LDAP_PORT );


        if (!pld) {
            DebugMsg((DM_WARNING, TEXT("GetMachineDomainDS:  ldap_open for <%s> failed with = 0x%x or %d"),
                                 pDCI->DomainName, pLdapApi->pfnLdapGetLastError(), GetLastError()));
            return pld;
        }


        //
        // Turn on Packet integrity flag
        //
        
        pData = (VOID *) LDAP_OPT_ON;
        ulResult = pLdapApi->pfnldap_set_option(pld, LDAP_OPT_SIGN, &pData);

        if (ulResult != LDAP_SUCCESS) {

            DebugMsg((DM_WARNING, TEXT("GetMachineDomainDS:  Failed to turn on LDAP_OPT_SIGN with %d"), ulResult));
            pLdapApi->pfnldap_unbind(pld);
            pld = 0;    
            return pld;
        }
        
	
        //
        // For machine policies specifically ask for Kerberos as the only authentication
        // mechanism. Otherwise if Kerberos were to fail for some reason, then NTLM is used
        // and localsystem context has no real credentials, which means that we won't get
        // any GPOs back.
        //
	
        secIdentity.Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
        secIdentity.Length = sizeof(SEC_WINNT_AUTH_IDENTITY_EXW);
        secIdentity.User = 0;
        secIdentity.UserLength = 0;
        secIdentity.Domain = 0;
        secIdentity.DomainLength = 0;
        secIdentity.Password = 0;
        secIdentity.PasswordLength = 0;
        secIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        secIdentity.PackageList = wszKerberos;
        secIdentity.PackageListLength = lstrlen( wszKerberos );

        if ( pLdapApi->pfnldap_bind_s (pld, 0, (WCHAR *)&secIdentity, LDAP_AUTH_SSPI) != LDAP_SUCCESS )
        {
            pLdapApi->pfnldap_unbind(pld);
            pld = 0;    
        }
	
        pNetApi32->pfnNetApiBufferFree(pDCI);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("GetMachineDomainDS:  The domain does not have a DS")));
    }


    return pld;
}

//*************************************************************
//
//  CallDFS()
//
//  Purpose:    Calls DFS to initialize the domain / DC name
//
//  Parameters:  lpDomainName  - Domain name
//               lpDCName      - DC name
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

#define POINTER_TO_OFFSET(field, buffer)  \
    ( ((PCHAR)field) -= ((ULONG_PTR)buffer) )


NTSTATUS CallDFS(LPWSTR lpDomainName, LPWSTR lpDCName)
{
    HANDLE DfsDeviceHandle = NULL;
    PDFS_SPC_REFRESH_INFO DfsInfo;
    ULONG lpDomainNameLen, lpDCNameLen, sizeNeeded;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;
    UNICODE_STRING unicodeServerName;


    lpDomainNameLen = (wcslen(lpDomainName) + 1) * sizeof(WCHAR);
    lpDCNameLen = (wcslen(lpDCName) + 1) * sizeof(WCHAR);

    sizeNeeded = sizeof(DFS_SPC_REFRESH_INFO) + lpDomainNameLen + lpDCNameLen;

    DfsInfo = (PDFS_SPC_REFRESH_INFO)LocalAlloc(LPTR, sizeNeeded);

    if (DfsInfo == NULL) {
        DebugMsg((DM_WARNING, TEXT("CallDFS:  LocalAlloc failed with %d"), GetLastError()));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DfsInfo->DomainName = (WCHAR *)((PCHAR)DfsInfo + sizeof(DFS_SPC_REFRESH_INFO));
    DfsInfo->DCName = (WCHAR *)((PCHAR)DfsInfo->DomainName + lpDomainNameLen);


    RtlCopyMemory(DfsInfo->DomainName,
                  lpDomainName,
                  lpDomainNameLen);

    RtlCopyMemory(DfsInfo->DCName,
                  lpDCName,
                  lpDCNameLen);

    POINTER_TO_OFFSET(DfsInfo->DomainName, DfsInfo);
    POINTER_TO_OFFSET(DfsInfo->DCName, DfsInfo);

    RtlInitUnicodeString( &unicodeServerName, L"\\Dfs");

    InitializeObjectAttributes(
          &objectAttributes,
          &unicodeServerName,
          OBJ_CASE_INSENSITIVE,
          NULL,
          NULL
          );

    status = NtOpenFile(
                &DfsDeviceHandle,
                SYNCHRONIZE | FILE_WRITE_DATA,
                &objectAttributes,
                &ioStatusBlock,
                0,
                FILE_SYNCHRONOUS_IO_NONALERT
                );



    if (!NT_SUCCESS(status) ) {
        DebugMsg((DM_WARNING, TEXT("CallDFS:  NtOpenFile failed with 0x%x"), status));
        LocalFree(DfsInfo);
        return status;
    }

    status = NtFsControlFile(
                DfsDeviceHandle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                FSCTL_DFS_SPC_REFRESH,
                DfsInfo, sizeNeeded,
                NULL, 0);

    if (!NT_SUCCESS(status) ) {
        DebugMsg((DM_WARNING, TEXT("CallDFS:  NtFsControlFile failed with 0x%x"), status));
    }

    
    LocalFree(DfsInfo);
    NtClose(DfsDeviceHandle);
    return status;
}
