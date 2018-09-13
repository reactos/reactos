/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    globals.cxx

Abstract:

    Contains global data items for WININET.DLL and initialization function

    Contents:
        GlobalDllInitialize
        GlobalDllTerminate
        GlobalDataInitialize
        GlobalDataTerminate
        IsHttp1_1
        IsOffline
        SetOfflineUserState
        FetchLocalStrings
        GetWininetUserName
        ChangeGlobalSettings
        RefreshOfflineFromRegistry
        PerformStartupProcessing

Author:

    Richard L Firth (rfirth) 15-Jul-1995

Revision History:

    15-Jul-1995 rfirth
        Created

    07-Oct-1998 joshco
        updated minor version number 1->2

--*/

#include <wininetp.h>
#include <ieverp.h>
#include <autodial.h>       // InitAutodialModule, ExitAutodialModule
#include <schnlsp.h>
#include <persist.h>

//
// WinInet major & minor versions - allow to be defined externally
//

// JOSHCO

#if !defined(WININET_MAJOR_VERSION)
#define WININET_MAJOR_VERSION   1
#endif
#if !defined(WININET_MINOR_VERSION)
#define WININET_MINOR_VERSION   2
#endif

//
// external functions
//

#ifdef UNIX
extern "C"
#endif /* UNIX */
void UrlZonesDetach (void);

#if INET_DEBUG

VOID
InitDebugSock(
    VOID
    );

#endif

//
// private prototypes
//

#if defined(SITARA)

PRIVATE
VOID
OpenIeMainKey(
    VOID
    );

PRIVATE
VOID
CloseIeMainKey(
    VOID
    );

PRIVATE
BOOL
CheckABS(
    VOID
    );

PRIVATE
BOOL
ReadIeMainDwordValue(
    IN LPSTR pszValueName,
    OUT LPDWORD pdwValue
    );

#endif // SITARA

//
// global DLL state data
//

GLOBAL HINSTANCE GlobalDllHandle = NULL;
GLOBAL DWORD GlobalPlatformType;
GLOBAL DWORD GlobalPlatformVersion5;
GLOBAL DWORD GlobalDllState = INTERNET_STATE_ONLINE | INTERNET_STATE_IDLE;
GLOBAL BOOL GlobalDataInitialized = FALSE;

//
// WinInet DLL version information (mainly for diagnostics)
//

#if INET_DEBUG

GLOBAL DWORD InternetMajorVersion = 1;
GLOBAL DWORD InternetMinorVersion = 0;

#endif // INET_DEBUG

#if !defined(VER_PRODUCTBUILD)
#define VER_PRODUCTBUILD    0
#endif

GLOBAL DWORD InternetBuildNumber = VER_PRODUCTBUILD;

//
// transport-based time-outs, etc.
//

//GLOBAL DWORD GlobalConnectTimeout = DEFAULT_CONNECT_TIMEOUT;
#ifndef unix
GLOBAL DWORD GlobalConnectTimeout = 5 * 60 * 1000;
#else
GLOBAL DWORD GlobalConnectTimeout = 1 * 60 * 1000;
#endif /* unix */
GLOBAL DWORD GlobalConnectRetries = DEFAULT_CONNECT_RETRIES;
GLOBAL DWORD GlobalSendTimeout = DEFAULT_SEND_TIMEOUT;
GLOBAL DWORD GlobalReceiveTimeout = DEFAULT_RECEIVE_TIMEOUT;
GLOBAL DWORD GlobalDataSendTimeout = DEFAULT_SEND_TIMEOUT;
GLOBAL DWORD GlobalDataReceiveTimeout = DEFAULT_RECEIVE_TIMEOUT;
GLOBAL DWORD GlobalFromCacheTimeout = (DWORD)-1;
GLOBAL DWORD GlobalFtpAcceptTimeout = DEFAULT_FTP_ACCEPT_TIMEOUT;
GLOBAL DWORD GlobalTransportPacketLength = DEFAULT_TRANSPORT_PACKET_LENGTH;
GLOBAL DWORD GlobalKeepAliveSocketTimeout = DEFAULT_KEEP_ALIVE_TIMEOUT;
GLOBAL DWORD GlobalSocketSendBufferLength = DEFAULT_SOCKET_SEND_BUFFER_LENGTH;
GLOBAL DWORD GlobalSocketReceiveBufferLength = DEFAULT_SOCKET_RECEIVE_BUFFER_LENGTH;
GLOBAL DWORD GlobalMaxHttpRedirects = DEFAULT_MAX_HTTP_REDIRECTS;
GLOBAL DWORD GlobalMaxConnectionsPerServer = DEFAULT_MAX_CONNECTIONS_PER_SERVER;
GLOBAL DWORD GlobalMaxConnectionsPer1_0Server = DEFAULT_MAX_CONS_PER_1_0_SERVER;
GLOBAL DWORD GlobalConnectionInactiveTimeout = DEFAULT_CONNECTION_INACTIVE_TIMEOUT;
GLOBAL DWORD GlobalServerInfoTimeout = DEFAULT_SERVER_INFO_TIMEOUT;
GLOBAL BOOL  GlobalHaveInternetOpened = FALSE;
GLOBAL BOOL  GlobalBypassEditedEntry = FALSE;


//GLOBAL DWORD GlobalServerInfoAllocCount = 0;
//GLOBAL DWORD GlobalServerInfoDeAllocCount = 0;

//
// async worker thread variables
//

//GLOBAL DWORD GlobalMinimumWorkerThreads = DEFAULT_MINIMUM_THREADS;
//GLOBAL DWORD GlobalMaximumWorkerThreads = DEFAULT_MAXIMUM_THREADS;
//GLOBAL DWORD GlobalInitialWorkerThreads = DEFAULT_INITIAL_THREADS;
//GLOBAL DWORD GlobalWorkerThreadIdleTimeout = DEFAULT_THREAD_IDLE_TIMEOUT;
//GLOBAL DWORD GlobalWorkQueueLimit = DEFAULT_WORK_QUEUE_LIMIT;
GLOBAL DWORD GlobalWorkerThreadTimeout = DEFAULT_WORKER_THREAD_TIMEOUT;

//
// switches
//

GLOBAL BOOL InDllCleanup = FALSE;
GLOBAL BOOL GlobalDynaUnload = FALSE;
GLOBAL BOOL GlobalDisableKeepAlive = FALSE;
GLOBAL BOOL GlobalEnableHttp1_1 = FALSE;
GLOBAL BOOL GlobalEnableProxyHttp1_1 = FALSE;
GLOBAL BOOL GlobalDisableReadRange = FALSE;
GLOBAL DWORD GlobalDisableNewCookieDlg = FALSE;

//GLOBAL BOOL GlobalAutoProxyInDeInit = FALSE;
GLOBAL BOOL GlobalIsProcessExplorer = FALSE;
#ifndef UNIX
GLOBAL BOOL GlobalEnableFortezza = TRUE;
#else /* for UNIX */
GLOBAL BOOL GlobalEnableFortezza = FALSE;
#endif /* UNIX */

#if defined(SITARA)

GLOBAL BOOL GlobalEnableSitara = FALSE;
GLOBAL BOOL GlobalHasSitaraModemConn = FALSE;

#endif // SITARA

GLOBAL BOOL GlobalEnableUtf8Encoding = FALSE;
GLOBAL BOOL GlobalEnableRevocation = FALSE;

// SSL Switches  (petesk 7/24/97)
GLOBAL DWORD GlobalSecureProtocols  = DEFAULT_SECURE_PROTOCOLS;

//
// AutoDetect Proxy Globals
//

GLOBAL LONG GlobalInternetOpenHandleCount = -1;
GLOBAL DWORD GlobalProxyVersionCount = 0;
GLOBAL BOOL GlobalAutoProxyInInit = FALSE;
GLOBAL BOOL GlobalAutoProxyCacheEnable = TRUE;
GLOBAL BOOL GlobalDisplayScriptDownloadFailureUI = FALSE;

//
//  Workaround for Novell's Client32
//

GLOBAL BOOL fDontUseDNSLoadBalancing = FALSE;

//
// Workaround for slow RAS enumeration
//
GLOBAL BOOL GlobalDisableNT4RasCheck = FALSE;
GLOBAL BOOL GlobalUseLanSettings = FALSE;

//
// lists
//

GLOBAL SERIALIZED_LIST GlobalObjectList = {0};
GLOBAL SERIALIZED_LIST GlobalServerInfoList = {0};

//
// cache timeouts
//

GLOBAL LONGLONG dwdwHttpDefaultExpiryDelta = 12 * 60 * 60 * (LONGLONG)10000000;  // 12 hours in 100ns units
GLOBAL LONGLONG dwdwFtpDefaultExpiryDelta = 24 * 60 * 60 * (LONGLONG)10000000;  // 24 hours in 100ns units
GLOBAL LONGLONG dwdwGopherDefaultExpiryDelta = 24 * 60 * 60 * (LONGLONG)10000000;  // 24 hours in 100ns units
GLOBAL LONGLONG dwdwSessionStartTime;
GLOBAL LONGLONG dwdwSessionStartTimeDefaultDelta = 0;

GLOBAL DWORD GlobalUrlCacheSyncMode = WININET_SYNC_MODE_DEFAULT;
GLOBAL DWORD GlobalDiskUsageLowerBound = (4*1024*1024);
GLOBAL DWORD GlobalScavengeFileLifeTime = (10*60);

GLOBAL LPSTR vszMimeExclusionList=NULL, vszHeaderExclusionList=NULL;

GLOBAL LPSTR *lpvrgszMimeExclusionTable=NULL, *lpvrgszHeaderExclusionTable=NULL;

GLOBAL DWORD *lpvrgdwMimeExclusionTableOfSizes=NULL;

GLOBAL DWORD vdwMimeExclusionTableCount=0, vdwHeaderExclusionTableCount=0;

//
// SSL globals, for UI.  We need to know
//  whether its ok for us to pop up UI.
//
//

GLOBAL BOOL GlobalWarnOnPost = FALSE;
GLOBAL BOOL GlobalWarnAlways = FALSE;
GLOBAL BOOL GlobalWarnOnZoneCrossing = TRUE;
GLOBAL BOOL GlobalWarnOnBadCertSending = FALSE;
GLOBAL BOOL GlobalWarnOnBadCertRecving = TRUE;
GLOBAL BOOL GlobalDisableSslCaching = FALSE;
GLOBAL BOOL GlobalWarnOnPostRedirect = TRUE;
GLOBAL BOOL GlobalAlwaysDrainOnRedirect = FALSE;

GLOBAL SECURITY_CACHE_LIST GlobalCertCache;

GLOBAL DWORD GlobalSettingsVersion=0; // crossprocess settings versionstamp
GLOBAL BOOL GlobalSettingsLoaded = FALSE;

GLOBAL const char vszSyncMode[] = "SyncMode5";

GLOBAL const char vszDisableSslCaching[] = "DisableCachingOfSSLPages";
GLOBAL BOOL GlobalDisableNTLMPreAuth = FALSE;

GLOBAL char vszCurrentUser[MAX_PATH];
GLOBAL DWORD vdwCurrentUserLen = 0;

//
// critical sections
//

GLOBAL CRITICAL_SECTION AutoProxyDllCritSec = {0};
GLOBAL CRITICAL_SECTION LockRequestFileCritSec = {0};
GLOBAL CRITICAL_SECTION ZoneMgrCritSec = {0};
GLOBAL CRITICAL_SECTION MlangCritSec  = {0};

// cookies info

GLOBAL BOOL vfAllowCookies = COOKIES_ALLOW;
GLOBAL BOOL vfPerUserCookies = TRUE;
const char  vszAnyUserName[]="anyuser";
const char  vszAllowCookies[] = "AllowCookies";
const char  vszPerUserCookies[] = "PerUserCookies";
const char  vszInvalidFilenameChars[] = "<>\\\"/:|?*";


// Mlang related data and functions.
PRIVATE HINSTANCE hInstMlang;
PRIVATE PFNINETMULTIBYTETOUNICODE pfnInetMultiByteToUnicode;
PRIVATE BOOL bFailedMlangLoad;  // So we don't try repeatedly if we fail once.
BOOL LoadMlang( );
BOOL UnloadMlang( );
#define MLANGDLLNAME    "mlang.dll"


// shfolder.dll hmod handle
HMODULE g_HMODSHFolder = NULL;

GLOBAL CUserName GlobalUserName;

//
// novell client32 (hack) "support"
//

GLOBAL BOOL GlobalRunningNovellClient32 = FALSE;
GLOBAL BOOL GlobalNonBlockingClient32 = FALSE;

//
// private data
//

HANDLE g_hAutodialMutex = NULL;

//
// proxy info
//

GLOBAL PROXY_INFO_GLOBAL GlobalProxyInfo;

//
// DLL version info
//

GLOBAL INTERNET_VERSION_INFO InternetVersionInfo = {
    WININET_MAJOR_VERSION,
    WININET_MINOR_VERSION
};

//
// HTTP version info - default 1.0
//

GLOBAL HTTP_VERSION_INFO HttpVersionInfo = {1, 0};


GLOBAL BOOL fCdromDialogActive = FALSE;
GLOBAL DWORD g_dwCredPersistAvail = CRED_PERSIST_UNKNOWN;

GLOBAL BOOL GlobalAutoSelectSingleCert = FALSE;
//
// The following globals are literal strings passed to winsock.
// Do NOT make them const, otherwise they end up in .text section,
// and web release of winsock2 has a bug where it locks and dirties
// send buffers, confusing the win95 vmm and resulting in code
// getting corrupted when it is paged back in.  -RajeevD
//

GLOBAL char gszAt[]   = "@";
GLOBAL char gszBang[] = "!";
GLOBAL char gszCRLF[] = "\r\n";


extern GLOBAL SERIALIZED_LIST BlockedRequestQueue;

//
// functions
//


#ifdef UNIX
extern "C"
#endif /* UNIX */
VOID
GlobalDllInitialize(
    VOID
    )

/*++

Routine Description:

    The set of initializations - critical sections, etc. - that must be done at
    DLL_PROCESS_ATTACH

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GLOBAL,
                 None,
                 "GlobalDllInitialize",
                 NULL
                 ));

    CLEAR_DEBUG_CRIT(szDebugBlankBuffer);

    GetCurrentGmtTime((LPFILETIME)&dwdwSessionStartTime);

    InitializeCriticalSection(&AutoProxyDllCritSec);
    InitializeCriticalSection(&LockRequestFileCritSec);
    InitializeCriticalSection(&ZoneMgrCritSec);
    InitializeCriticalSection(&MlangCritSec);

    InitializeSerializedList(&GlobalObjectList);
    InitializeSerializedList(&GlobalServerInfoList);
    InitializeSerializedList(&BlockedRequestQueue);

    AuthOpen();

    IwinsockInitialize();
    SecurityInitialize();
    FtpInitialize();
    GopherInitialize();

    //
    // initialize cache critical sections etc.
    //

    DLLUrlCacheEntry(DLL_PROCESS_ATTACH);

    OpenInternetSettingsKey();

    DEBUG_LEAVE(0);
}


#ifdef UNIX
extern "C"
#endif /* UNIX */
VOID
GlobalDllTerminate(
    VOID
    )

/*++

Routine Description:

    Undoes the initializations of GlobalDllInitialize

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GLOBAL,
                 None,
                 "GlobalDllTerminate",
                 NULL
                 ));

    //
    // only perform resource clean-up if this DLL is being unloaded due to a
    // FreeLibrary() call. Otherwise, we take the lazy way out and let the
    // system clean up after us
    //

    if (GlobalDynaUnload) {
        TerminateAsyncSupport();
        GopherTerminate();
        FtpTerminate();
        IwinsockTerminate();
        HandleTerminate();
    }

    CHECK_SOCKETS();

    AuthClose();

    TerminateSerializedList(&BlockedRequestQueue);
    TerminateSerializedList(&GlobalServerInfoList);

    //
    //BUGBUG: we can't Terminate the list here because
    //        of a race condition from IE3
    //        (someone still holds the handle)
    //        but we don't want to leak the CritSec
    //        TerminateSerlizedList == DeleteCritSec + some Asserts
    //
    //TerminateSerializedList(&GlobalObjectList);
    DeleteCriticalSection(&(GlobalObjectList.Lock));


    DeleteCriticalSection(&MlangCritSec);
    DeleteCriticalSection(&ZoneMgrCritSec);
    DeleteCriticalSection(&LockRequestFileCritSec);
    DeleteCriticalSection(&AutoProxyDllCritSec);

    DLLUrlCacheEntry(DLL_PROCESS_DETACH);

    SecurityTerminate();

    if (g_HMODSHFolder)
    {
        FreeLibrary(g_HMODSHFolder);
        g_HMODSHFolder = NULL;
    }

    CloseInternetSettingsKey();

    DEBUG_LEAVE(0);
}


DWORD
GlobalDataInitialize(
    VOID
    )

/*++

Routine Description:

    Loads any global data items from the registry

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_GLOBAL,
                 Dword,
                 "GlobalDataInitialize",
                 NULL
                 ));

    static BOOL Initializing = FALSE;
    static BOOL Initialized = FALSE;
    static DWORD error = ERROR_SUCCESS;
    
    //
    // only one thread initializes
    //

    if (InterlockedExchange((LPLONG)&Initializing, TRUE)) {
        while (!Initialized) {
            SleepEx(0, TRUE);
        }
        goto done;
    }

    //
    // we ignore any failure return codes from reading the registry. All the
    // global variables are initialized to default values
    //

    //
    // UseSchannelDirectly - TRUE if we are to bypass SSPI "Secur32/Security"
    //      and directly call into SCHANNEL.  This should give us a perf
    //      improvement since we don't have to load an extra DLL.
    //

//
// BUGBUG - all these need to be per-process. They are intended for IE
//

    InternetReadRegistryDword("FromCacheTimeout",
                              (LPDWORD)&GlobalFromCacheTimeout
                              );

    //InternetReadRegistryDword("UseSchannelDirectly",
    //                          (LPDWORD)&GlobalUseSchannelDirectly
    //                          );

    // also in ChangeGlobalSettings()
    InternetReadRegistryDword("SecureProtocols",
                              (LPDWORD)&GlobalSecureProtocols
                              );

    // also in ChangeGlobalSettings()
    InternetReadRegistryDword("Fortezza",
                              (LPDWORD)&GlobalEnableFortezza
                              );

    // also in ChangeGlobalSettings()
    InternetReadRegistryDword("CertificateRevocation",
                              (LPDWORD)&GlobalEnableRevocation
                              );

    InternetReadRegistryDword("DisableKeepAlive",
                              (LPDWORD)&GlobalDisableKeepAlive
                              );

    // also in ChangeGlobalSettings()
    InternetReadRegistryDword("EnableHttp1_1",
                              (LPDWORD)&GlobalEnableHttp1_1
                              );

    // also in ChangeGlobalSettings()
    InternetReadRegistryDword("ProxyHttp1.1",
                              (LPDWORD)&GlobalEnableProxyHttp1_1
                              );

    DWORD dwType, dwSize;

    dwSize = sizeof(DWORD);

    SHGetValue(HKEY_CURRENT_USER, INTERNET_POLICY_KEY,
        "EnableAutoProxyResultCache", &dwType, &GlobalAutoProxyCacheEnable, &dwSize);

    dwSize = sizeof(DWORD);
    SHGetValue(HKEY_CURRENT_USER, INTERNET_POLICY_KEY,
        "DisplayScriptDownloadFailureUI", &dwType, &GlobalDisplayScriptDownloadFailureUI, &dwSize);

    InternetReadRegistryDword("DisableReadRange",
                              (LPDWORD)&GlobalDisableReadRange
                              );

    InternetReadRegistryDword("SocketSendBufferLength",
                              &GlobalSocketSendBufferLength
                              );

    InternetReadRegistryDword("SocketReceiveBufferLength",
                              &GlobalSocketReceiveBufferLength
                              );

    InternetReadRegistryDword("KeepAliveTimeout",
                              &GlobalKeepAliveSocketTimeout
                              );

    InternetReadRegistryDword("MaxHttpRedirects",
                              &GlobalMaxHttpRedirects
                              );

    InternetReadRegistryDword("MaxConnectionsPerServer",
                              &GlobalMaxConnectionsPerServer
                              );

    InternetReadRegistryDword("MaxConnectionsPer1_0Server",
                              &GlobalMaxConnectionsPer1_0Server
                              );

    InternetReadRegistryDword("ServerInfoTimeout",
                              &GlobalServerInfoTimeout
                              );

    InternetReadRegistryDword("ReceiveTimeOut",
                              (LPDWORD)&GlobalReceiveTimeout
                              );

    InternetReadRegistryDword("DisableNTLMPreAuth",
                              (LPDWORD)&GlobalDisableNTLMPreAuth
                              );

    InternetReadRegistryDword("ScavengeCacheLowerBound",
                              (LPDWORD)&GlobalDiskUsageLowerBound
                              );

    InternetCacheReadRegistryDword("ScavengeCacheFileLifeTime",
                              (LPDWORD)&GlobalScavengeFileLifeTime
                              );

    DWORD dwDefTime;

    if (InternetReadRegistryDword("HttpDefaultExpiryTimeSecs", &dwDefTime) ==
            ERROR_SUCCESS) {
        dwdwHttpDefaultExpiryDelta = dwDefTime * (LONGLONG)10000000;
    }

    if (InternetReadRegistryDword("FtpDefaultExpiryTimeSecs", &dwDefTime) ==
            ERROR_SUCCESS) {
        dwdwFtpDefaultExpiryDelta = dwDefTime * (LONGLONG)10000000;
    }

    if (InternetReadRegistryDword("GopherDefaultExpiryTimeSecs", &dwDefTime) ==
            ERROR_SUCCESS) {
        dwdwGopherDefaultExpiryDelta = dwDefTime * (LONGLONG)10000000;
    }

    InternetReadRegistryDword(vszDisableSslCaching, (LPDWORD)&GlobalDisableSslCaching);

    InternetReadRegistryDword(vszAllowCookies, (LPDWORD)&vfAllowCookies);
    InternetReadRegistryDword(vszPerUserCookies, (LPDWORD)&vfPerUserCookies);
    InternetReadRegistryDword("DisableNewCookieDlg", (LPDWORD)&GlobalDisableNewCookieDlg);

    InternetReadRegistryDword("DisableNT4RasCheck", (LPDWORD)&GlobalDisableNT4RasCheck);
    InternetReadRegistryDword("DialupUseLanSettings", (LPDWORD)&GlobalUseLanSettings);

    InternetReadRegistryDword("AutoSelectSingleCert",
                              (LPDWORD)&GlobalAutoSelectSingleCert
                              );
    //
    //  Work around for Kookaburra's CookiePal application that hooks the old-style
    //   Wininet MessageBox type dlg in order to control what cookies get added
    //   or removed in IE.  We detect if their installed by looking for their registry
    //   value in Local Machine.  See IE B#32908, http://www.kburra.com/
    //

    if ( ! GlobalDisableNewCookieDlg )
    {
        #define COOKIE_PAL_SETTINGS_KEY "SOFTWARE\\Kookaburra\\CookiePal\\Config"

        HKEY clientKey = NULL;
        DWORD dwIECookiePalEnabled = FALSE;

        error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             COOKIE_PAL_SETTINGS_KEY,
                             0, // reserved
                             KEY_QUERY_VALUE,
                             &clientKey
                             );

        if (error == ERROR_SUCCESS) {
            error = ReadRegistryDword(clientKey,
                                      "IEWarn",
                                      &dwIECookiePalEnabled
                                      );
            if (clientKey) {
                REGCLOSEKEY(clientKey);
            }

            if ( dwIECookiePalEnabled )
            {
                GlobalDisableNewCookieDlg = TRUE;
            }
        }

    }


    if ( vfAllowCookies > COOKIES_DENY )
    {
        vfAllowCookies = COOKIES_ALLOW;
    }

    //
    //  fix for Novell's Client32 - zekel 23-jul-96
    //  first check HKLM then HKCU, HKCU takes precdence
    //

    InternetReadRegistryDwordKey(HKEY_LOCAL_MACHINE,
                                 "DontUseDNSLoadBalancing",
                                 (LPDWORD)&fDontUseDNSLoadBalancing
                                 );
    InternetReadRegistryDword("DontUseDNSLoadBalancing",
                              (LPDWORD) &fDontUseDNSLoadBalancing
                              );

    InternetReadRegistryDword("NonBlockingClient32",
                              (LPDWORD)&GlobalNonBlockingClient32
                              );

    //
    // Get the list of MIME types for which caching needs to be disabled
    //

    CreateMimeExclusionTableForCache();

    //
    // Get the list of headers that should be excluded from the cache
    //

    CreateHeaderExclusionTableForCache();


    DEBUG_PRINT(HTTP,
                INFO,
                ("Current wininet user is %s length %d\n",
                vszCurrentUser,
                vdwCurrentUserLen
                ));


    //
    // initialize databases
    //

    InitializeHostentCache();
    GlobalDataReadWarningUIFlags();

    PerformStartupProcessing();

    //
    // create the global keep-alive, cert-cache and proxy lists
    //

    GlobalCertCache.Initialize();
    GlobalProxyInfo.InitializeProxySettings();

    //LoadServerInfoDatabase();

    //
    // initialize the autodial code
    //

    InitAutodialModule(FALSE);

    //
    // initialize offline mode from registry
    //

    RefreshOfflineFromRegistry();

    //
    // read the global (cache) settings version to avoid an unnecessary reload
    // N.B. we rely on the side effect of calling urlcache InitGlobals
    //
    InternetSettingsChanged();

    char buf[MAX_PATH + 1];

    if (GetModuleFileName(NULL, buf, sizeof(buf))) {
        LPSTR p = strrchr(buf, DIR_SEPARATOR_CHAR);
        p = p ? ++p : buf;

        DEBUG_PRINT(INET, INFO, ("process is %q\n", p));

        if (lstrcmpi(p, "EXPLORER.EXE") && lstrcmpi(p, "IEXPLORE.EXE")) {

            //
            // yet another app-hack: AOL's current browser can't understand
            // HTTP 1.1. When they do, they have to call InternetSetOption()
            // with INTERNET_OPTION_HTTP_VERSION
            //

            if (!lstrcmpi(p, "WAOL.EXE")) {
                GlobalEnableHttp1_1 = FALSE;
            }

            //
            // non-IE app must supply this through option
            //

            GlobalFromCacheTimeout = (DWORD)-1;
        } else {
            GlobalIsProcessExplorer = TRUE;
        }
    } else {

        DEBUG_PRINT(INET,
                    INFO,
                    ("GetModuleFileName() returns %d\n",
                    GetLastError()
                    ));

    }

#if defined(SITARA)

    //
    // check if Sitara is loaded (IE5B1). Existence of key means Sitara
    // installed
    //

    HKEY key;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "Software\\Microsoft\\InternetExpressLane",
                     0,
                     KEY_QUERY_VALUE,
                     &key) == ERROR_SUCCESS) {
        REGCLOSEKEY(key);
        OpenIeMainKey();
        GlobalEnableSitara = CheckABS();
    }

#endif // SITARA

    DWORD urlEncoding;

    urlEncoding = 0;
    InternetReadRegistryDwordKey(HKEY_LOCAL_MACHINE,
                                 "UrlEncoding",
                                 &urlEncoding
                                 );
    if (urlEncoding == 0) {
        GlobalEnableUtf8Encoding = TRUE;
    }

    //
    // perform module/package-specific initialization
    //

    error = HandleInitialize();
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // initalize the cookie system
    //

    if (!OpenTheCookieJar()) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // initialize the background task manager
    //
    if( !LoadBackgroundTaskMgr() ) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        //goto quit;
    }


quit:

    //
    // finally, if EnableHttp1_1 was set to non-zero in the registry, enable
    // HTTP 1.1
    //

    if (GlobalEnableHttp1_1) {
        HttpVersionInfo.dwMajorVersion = 1;
        HttpVersionInfo.dwMinorVersion = 1;
    }

    if (error == ERROR_SUCCESS) {
        GlobalDataInitialized = TRUE;
    }

    //
    // irrespective of success or failure, we have attempted global data
    // initialization. If we failed then we assume its something fundamental
    // and fatal: we don't try again
    //

    Initialized = TRUE;

done:

    DEBUG_LEAVE(error);

    return error;
}


VOID
GlobalDataReadWarningUIFlags(
    VOID
    )

/*++

Routine Description:

    Reads Registry values into global data.
    Used to read read the registry on connects.

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_GLOBAL,
                 None,
                 "GlobalDataReadWarningUIFlags",
                 NULL
                 ));

    //
    // Load SSL Values from the Registry on whether to pop up UI on error.
    //

    InternetReadRegistryDword("WarnOnPost", (LPDWORD)&GlobalWarnOnPost);

    GlobalWarnAlways = FALSE;
    InternetReadRegistryDword("WarnAlwaysOnPost", (LPDWORD)&GlobalWarnAlways);

    //
    // If Global Warn On Post is set to "2" we reset the "WarnAlways" to TRUE,
    //  since this is the new way INETCPL reads/writes the registry.
    //

    if ( GlobalWarnOnPost == 2 )
    {
        GlobalWarnAlways = TRUE;
    }

    InternetReadRegistryDword("WarnOnZoneCrossing", (LPDWORD)&GlobalWarnOnZoneCrossing);

    InternetReadRegistryDword("WarnOnBadCertSending", (LPDWORD)&GlobalWarnOnBadCertSending);

    InternetReadRegistryDword("WarnOnBadCertRecving", (LPDWORD)&GlobalWarnOnBadCertRecving);

    InternetReadRegistryDword("WarnOnPostRedirect", (LPDWORD)&GlobalWarnOnPostRedirect);

    InternetReadRegistryDword("AlwaysDrainOnRedirect", (LPDWORD)&GlobalAlwaysDrainOnRedirect);

    DEBUG_LEAVE(0);
}


VOID
GlobalDataTerminate(
    VOID
    )

/*++

Routine Description:

    Undoes work of GlobalDataInitialize()

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GLOBAL,
                 None,
                 "GlobalDataTerminate",
                 NULL
                 ));

    //
    // Release background task manager
    //
    UnloadBackgroundTaskMgr();

    AuthUnload();
    UrlZonesDetach();

    TerminateHostentCache();
    CloseTheCookieJar();

    //
    // destroy lists created from registry
    //

    DestroyMimeExclusionTableForCache();
    DestroyHeaderExclusionTableForCache();

    //
    // terminate the global cert-cache and proxy lists
    //

    GlobalCertCache.Terminate();
    GlobalProxyInfo.TerminateProxySettings();


    //SaveServerInfoDatabase();
    PurgeServerInfoList(TRUE);

    //TerminateAsyncSupport();

    //
    // free up the handle to the startup mutex
    //
    if (g_hAutodialMutex) {
        CloseHandle(g_hAutodialMutex);
    }

#if defined(SITARA)

    CloseIeMainKey();

#endif // SITARA

    UnloadMlang();
    UnloadSecurity();

    GlobalDataInitialized = FALSE;

    DEBUG_LEAVE(0);
}


BOOL
IsHttp1_1(
    VOID
    )

/*++

Routine Description:

    Determine if we are using HTTP 1.1 or greater

Arguments:

    None.

Return Value:

    BOOL

--*/

{
    return (HttpVersionInfo.dwMajorVersion > 1)
            ? TRUE
            : (((HttpVersionInfo.dwMajorVersion == 1)
                && (HttpVersionInfo.dwMajorVersion >= 1))
                ? TRUE
                : FALSE);
}


BOOL
IsOffline(
    VOID
    )

/*++

Routine Description:

    Returns whether we are in (global) offline mode or not. We are offline if
    we went offline because of network failure, or we were put into offline
    mode by the user

Arguments:

    None.

Return Value:

    BOOL
        TRUE    - we are currently offline

        FALSE   - not offline

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 Bool,
                 "IsOffline",
                 NULL
                 ));

    INET_ASSERT(((GlobalDllState & INTERNET_LINE_STATE_MASK) == INTERNET_STATE_ONLINE)
                || ((GlobalDllState & INTERNET_LINE_STATE_MASK) == INTERNET_STATE_OFFLINE));
    INET_ASSERT((GlobalDllState & INTERNET_STATE_OFFLINE_USER)
                ? ((GlobalDllState & INTERNET_LINE_STATE_MASK) == INTERNET_STATE_OFFLINE)
                : TRUE);

    BOOL offline = (GlobalDllState & (INTERNET_STATE_OFFLINE | INTERNET_STATE_OFFLINE_USER))
                    ? TRUE
                    : FALSE;

    DEBUG_LEAVE(offline);

    return offline;
}


DWORD
SetOfflineUserState(
    IN DWORD dwState,
    IN BOOL bForce
    )

/*++

Routine Description:

    If we are in online state, puts Wininet into user-induced offline mode. Stop
    all outstanding requests, if required and set the global scope. If we are in
    offline mode, then change state to online

Arguments:

    dwState - INTERNET_STATE_ONLINE or INTERNET_STATE_OFFLINE

    bForce  - TRUE if we are to forcibly complete all outstanding requests
              (including synchronous requests)

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_GLOBAL,
                 Dword,
                 "SetOfflineUserState",
                 "%s (%d), %B",
                 (dwState == INTERNET_STATE_ONLINE) ? "INTERNET_STATE_ONLINE"
                 : (dwState == INTERNET_STATE_OFFLINE) ? "INTERNET_STATE_OFFLINE"
                 : "???",
                 dwState,
                 bForce
                 ));

    INET_ASSERT((dwState == INTERNET_STATE_ONLINE)
                || (dwState == INTERNET_STATE_OFFLINE));
    INET_ASSERT(((GlobalDllState & INTERNET_LINE_STATE_MASK) == INTERNET_STATE_ONLINE)
                || ((GlobalDllState & INTERNET_LINE_STATE_MASK) == INTERNET_STATE_OFFLINE));

    DWORD error = ERROR_SUCCESS;

    if (dwState == INTERNET_STATE_OFFLINE) {
        GlobalDllState = (GlobalDllState & ~INTERNET_LINE_STATE_MASK)
                       | (INTERNET_STATE_OFFLINE | INTERNET_STATE_OFFLINE_USER);
        if (bForce) {
            //CancelActiveAsyncRequests(ERROR_INTERNET_OFFLINE);
            CancelActiveSyncRequests(ERROR_INTERNET_OFFLINE);
            GlobalProxyInfo.AbortAutoProxy();
        }
    } else {
        GlobalDllState = GlobalDllState
                       & ~(INTERNET_LINE_STATE_MASK | INTERNET_STATE_OFFLINE_USER)
                       | INTERNET_STATE_ONLINE;
    }

    DEBUG_PRINT(GLOBAL,
                INFO,
                ("GlobalDllState = %#x\n",
                GlobalDllState
                ));

    DEBUG_LEAVE(error);

    return error;
}


/*++

FetchLocalStrings:

    This routine fetches the strings necessary to display information in the
    language of the local user.

Arguments:

    None

Return Value:

    The address of a LOCAL_STRINGS structure containing the addresses of the
    localized strings to display.

Author:

    Doug Barlow (dbarlow) 4/25/1996

--*/

PLOCAL_STRINGS
FetchLocalStrings(
    void)
{
    //
    // WARNING!  The order of elements in the following array must match the
    // order of elements in the LOCAL_STRINGS structure.
    //

    static const UINT
        uStringId[]
            = { IDS_LW95_ENTERAUTHINFO,
                IDS_SECERT_CERTINFO,
                IDS_SECERT_STRENGTH_HIGH,
                IDS_SECERT_STRENGTH_MEDIUM,
                IDS_SECERT_STRENGTH_LOW,
                IDS_CERT_SUBJECT,
                IDS_CERT_ISSUER,
                IDS_CERT_EFFECTIVE_DATE,
                IDS_CERT_EXPIRATION_DATE,
                IDS_CERT_PROTOCOL,
                IDS_CERT_USAGE,
                IDS_CERT_ENCRYPT_ALG,
                IDS_CERT_HASH_ALG,
                IDS_CERT_EXCH_ALG,
                IDS_CERT_COMMENT,
                IDS_COMMENT_EXPIRES,
                IDS_COMMENT_NOT_VALID,
                IDS_COMMENT_BAD_CN,
                IDS_COMMENT_BAD_CA,
                IDS_COMMENT_BAD_SIGNATURE,
                IDS_COMMENT_REVOKED,
                IDS_STRING_CIPHMSG,
                IDS_STRING_HASHMSG,
                IDS_STRING_EXCHMSG,
                IDS_CERT_FINGERPRINT,
                IDS_DOMAIN,
                IDS_REALM,
                IDS_SITE,
                IDS_FIREWALL
    };
    static LOCAL_STRINGS
        lszStrings;
    static BOOL fInitialized = FALSE;

    INET_ASSERT(sizeof(uStringId) == offsetof(LOCAL_STRINGS, rgchBuffer));
    EnterCriticalSection(&GeneralInitCritSec);
    __try
    {
        if (!fInitialized)
        {
            LPWSTR szBufEntry;
            LPWSTR *pszName;
            DWORD dwOffset;
            DWORD index, len;


            //
            // It needs to be initialized.
            //

            pszName = (LPWSTR *)&lszStrings;
            dwOffset = 0;
            for (index = 0;
                 index < sizeof(uStringId) / sizeof(UINT);
                 index += 1)
            {
                szBufEntry = &lszStrings.rgchBuffer[dwOffset];
                len = LoadStringWrapW(
                            GlobalDllHandle,
                            uStringId[index],
                            szBufEntry,
                            LOCAL_STRINGS_MAX_BUFFER - dwOffset);
                INET_ASSERT(0 != len);  // Resource missing!
                dwOffset += len;
                lszStrings.rgchBuffer[dwOffset++] = 0;
                *pszName++ = szBufEntry;
            }

            INET_ASSERT(LOCAL_STRINGS_MAX_BUFFER > dwOffset);


            //
            // Make it available to this and future callers.
            //

            fInitialized = TRUE;
        }
    }
    __finally
    {
        LeaveCriticalSection(&GeneralInitCritSec);
    }
    ENDFINALLY
    return &lszStrings;
}



BOOL
GetWininetUserName(
    VOID
)
{
    BOOL fRet = FALSE;
    DWORD dwT;
    CHAR *ptr;

    // Note this critsect could be blocked for a while if RPC gets involved...
    EnterCriticalSection(&GeneralInitCritSec);

    if (vdwCurrentUserLen) {
        fRet = TRUE;
        goto Done;
    }

    dwT = sizeof(vszCurrentUser);

    if (vfPerUserCookies) {

        fRet = GetUserName(vszCurrentUser, &dwT);

        if (!fRet) {

            DEBUG_PRINT(HTTP,
                        ERROR,
                        ("GetUsername returns %d\n",
                        GetLastError()
                        ));
        }

    }

    if (fRet == FALSE){

        strcpy(vszCurrentUser, vszAnyUserName);

        fRet = TRUE;
    }

    // Downcase the username.
    ptr = vszCurrentUser;
    while (*ptr)
    {
        *ptr = tolower(*ptr);
        ptr++;
    }

    INET_ASSERT(fRet == TRUE);

    vdwCurrentUserLen = (DWORD) (ptr - vszCurrentUser);


Done:
    LeaveCriticalSection(&GeneralInitCritSec);
    return (fRet);
}


VOID
ChangeGlobalSettings(
    VOID
    )

/*++

Routine Description:

    Changes global settings

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GLOBAL,
                 None,
                 "ChangeGlobalSettings",
                 NULL
                 ));

    InternetReadRegistryDword(vszSyncMode,
                              &GlobalUrlCacheSyncMode
                              );

    InternetReadRegistryDword(vszDisableSslCaching,
                              (LPDWORD)&GlobalDisableSslCaching
                              );

    InternetReadRegistryDword(vszAllowCookies,
                              (LPDWORD)&vfAllowCookies
                              );

    InternetReadRegistryDword("EnableHttp1_1",
                              (LPDWORD)&GlobalEnableHttp1_1
                              );

    InternetReadRegistryDword("ProxyHttp1.1",
                              (LPDWORD)&GlobalEnableProxyHttp1_1
                              );

    if ( vfAllowCookies > COOKIES_DENY )
    {
        vfAllowCookies = COOKIES_ALLOW;
    }

    InternetReadRegistryDword(vszPerUserCookies,
                              (LPDWORD)&vfPerUserCookies
                              );

    if (!GlobalProxyInfo.IsModifiedInProcess())
    {
        FixProxySettingsForCurrentConnection(
            FALSE
            );
    }

    //
    // update security protocol changes.
    //

    InternetReadRegistryDword("SecureProtocols",
                          (LPDWORD)&GlobalSecureProtocols
                          );

    InternetReadRegistryDword("Fortezza",
                          (LPDWORD)&GlobalEnableFortezza
                          );

    InternetReadRegistryDword("CertificateRevocation",
                              (LPDWORD)&GlobalEnableRevocation
                              );

    //
    // Check autodial settings and hang up if autodial was turned off
    //

    // have autodial module reset itself
    ResetAutodialModule();

    RefreshOfflineFromRegistry();

    DEBUG_LEAVE(0);
}


VOID
RefreshOfflineFromRegistry(
    VOID
    )

/*++

Routine Description:

    Reads offline setting in the registry and sets wininet's offline mode
    accordingly.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GLOBAL,
                 None,
                 "RefreshOfflineFromRegistry",
                 NULL
                 ));

    DWORD dwTemp = 0;

    // note: this won't alter dwTemp if not found and so will default to
    // online
    InternetReadRegistryDword("GlobalUserOffline", &dwTemp);

    // convert to appropriate flags
    if(0 == dwTemp) {
        // online
        dwTemp = INTERNET_STATE_ONLINE;
    } else {
        // offline
        dwTemp = INTERNET_STATE_OFFLINE;
    }
    SetOfflineUserState(dwTemp, FALSE);

    DEBUG_LEAVE(0);
}


VOID
PerformStartupProcessing(
    VOID
    )

/*++

Routine Description:

    Performs actions exactly once on system startup.  Resets offline mode.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GLOBAL,
                 None,
                 "PerformStartupProcessing",
                 NULL
                 ));

    INET_ASSERT(NULL == g_hAutodialMutex);

    g_hAutodialMutex = OpenMutex(SYNCHRONIZE, FALSE, WININET_STARTUP_MUTEX);
    if (g_hAutodialMutex == NULL && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_INVALID_NAME))
    {
        g_hAutodialMutex = CreateMutex(CreateAllAccessSecurityAttributes(NULL,NULL,NULL), FALSE, WININET_STARTUP_MUTEX);
    }

    if(g_hAutodialMutex) {
        DWORD dwLastError = GetLastError();

        if(ERROR_SUCCESS == dwLastError) {
            // GetLastError returns ERROR_ALREADY_EXISTS if the mutex existed
            // before we created it.  If we don't get this error, we're the
            // first so proceed with out startup processing

            if(!GlobalPlatformVersion5)
            {
                // Reset global offline mode on non-Win2K platforms
                InternetWriteRegistryDword("GlobalUserOffline", 0);
            }
        }
    }

    DEBUG_LEAVE(0);
}

#if defined(SITARA)

PRIVATE HKEY IeMainKey = NULL;


PRIVATE
VOID
OpenIeMainKey(
    VOID
    )
{
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     "Software\\Microsoft\\Internet Explorer\\Main",
                     0,
                     KEY_QUERY_VALUE,
                     &IeMainKey
                     ) != ERROR_SUCCESS) {
        IeMainKey = NULL;
    }
}


PRIVATE
VOID
CloseIeMainKey(
    VOID
    )
{
    if (IeMainKey != NULL) {
        REGCLOSEKEY(IeMainKey);
        IeMainKey = NULL;
    }
}


DWORD
GetSitaraProtocol(
    VOID
    )
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Int,
                 "GetSitaraProtocol",
                 NULL
                 ));

    DWORD dwProtocol = IPPROTO_TCP;
    DWORD dwEnabled = 0;

    if (ReadIeMainDwordValue("Use_Express_Lane", &dwEnabled) && (dwEnabled != 0)) {
        dwProtocol = 901;
    }

    DEBUG_LEAVE(dwProtocol);

    return dwProtocol;
}


PRIVATE
BOOL
CheckABS(
    VOID
    )
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "CheckABS",
                 NULL
                 ));

    BOOL on = 0;

    ReadIeMainDwordValue("ABS", (LPDWORD)&on);

    DEBUG_LEAVE(!on);

    return !on;
}


PRIVATE
BOOL
ReadIeMainDwordValue(
    IN LPSTR pszValueName,
    OUT LPDWORD pdwValue
    )
{
    if (IeMainKey != NULL) {

        DWORD value;
        DWORD valueType;
        DWORD valueLength = sizeof(value);
        DWORD err = RegQueryValueEx(IeMainKey,
                                    pszValueName,
                                    NULL,
                                    &valueType,
                                    (LPBYTE)&value,
                                    &valueLength
                                    );

        if ((err == ERROR_SUCCESS)
            && ((valueType == REG_DWORD) || (valueType == REG_BINARY))
            && (valueLength == sizeof(DWORD))) {
            *pdwValue = value;
            return TRUE;
        }
    }
    return FALSE;
}

#endif // SITARA


// Loads Mlang.dll and get the entry point we are interested in.

BOOL LoadMlang( )
{
    EnterCriticalSection(&MlangCritSec);

    if (hInstMlang == NULL && !bFailedMlangLoad)
    {
        INET_ASSERT(pfnInetMultiByteToUnicode == NULL);
        hInstMlang = LoadLibrary(MLANGDLLNAME);

        if (hInstMlang != NULL)
        {
            pfnInetMultiByteToUnicode = (PFNINETMULTIBYTETOUNICODE)GetProcAddress
                                            (hInstMlang,"ConvertINetMultiByteToUnicode");
            if (pfnInetMultiByteToUnicode == NULL)
            {
                INET_ASSERT(FALSE);
                FreeLibrary(hInstMlang);
                hInstMlang = NULL;
            }
        }
        else
        {
            INET_ASSERT(FALSE); // bad news if we can't load mlang.dll
        }

        if (pfnInetMultiByteToUnicode == NULL)
            bFailedMlangLoad = TRUE;
    }

    LeaveCriticalSection(&MlangCritSec);

    return (pfnInetMultiByteToUnicode != NULL);
}

BOOL UnloadMlang( )
{
    EnterCriticalSection(&MlangCritSec);

    if (hInstMlang)
        FreeLibrary(hInstMlang);

    hInstMlang = NULL;
    pfnInetMultiByteToUnicode = NULL;
    bFailedMlangLoad = FALSE;

    LeaveCriticalSection(&MlangCritSec);

    return TRUE;
}

PFNINETMULTIBYTETOUNICODE GetInetMultiByteToUnicode( )
{
    // We are checking for pfnInetMultiByteToUnicode without getting a crit section.
    // This works only because UnloadMlang is called at the Dll unload time.

    if (pfnInetMultiByteToUnicode == NULL)
    {
        LoadMlang( );
    }

    return pfnInetMultiByteToUnicode;
}


