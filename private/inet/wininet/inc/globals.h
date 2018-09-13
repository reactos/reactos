/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    globals.h

Abstract:

    External definitions for data in dll\globals.c

Author:

    Richard L Firth (rfirth) 15-Jul-1995

Revision History:

    15-Jul-1995 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// macros
//

#define IsGlobalStateOnline() \
    (((GlobalDllState & INTERNET_LINE_STATE_MASK) == INTERNET_STATE_ONLINE) \
        ? TRUE : FALSE)

#define IsGlobalStateOffline() \
    (((GlobalDllState & INTERNET_LINE_STATE_MASK) == INTERNET_STATE_OFFLINE) \
        ? TRUE : FALSE)

#define IsGlobalStateOfflineUser() \
    (((GlobalDllState \
        & (INTERNET_LINE_STATE_MASK | INTERNET_STATE_OFFLINE_USER)) \
        == (INTERNET_STATE_OFFLINE | INTERNET_STATE_OFFLINE_USER)) \
        ? TRUE : FALSE)

#define UPDATE_GLOBAL_PROXY_VERSION() \
    InterlockedIncrement((LPLONG)&GlobalProxyVersionCount)


#define COOKIES_WARN     0 // warn with a dlg if using cookies
#define COOKIES_ALLOW    1 // allow cookies without any warning
#define COOKIES_DENY     2 // disable cookies completely


//
// external variables
//

extern HINSTANCE GlobalDllHandle;
#define GlobalResHandle     GlobalDllHandle  // change for plugable ui
extern DWORD GlobalPlatformType;
extern DWORD GlobalPlatformVersion5;

extern DWORD GlobalDllState;
extern BOOL GlobalDataInitialized;

extern DWORD InternetMajorVersion;
extern DWORD InternetMinorVersion;
extern DWORD InternetBuildNumber;

extern DWORD GlobalConnectTimeout;
extern DWORD GlobalConnectRetries;
extern DWORD GlobalSendTimeout;
extern DWORD GlobalReceiveTimeout;
extern DWORD GlobalDataSendTimeout;
extern DWORD GlobalDataReceiveTimeout;
extern DWORD GlobalFromCacheTimeout;
extern DWORD GlobalFtpAcceptTimeout;
extern DWORD GlobalTransportPacketLength;
extern DWORD GlobalKeepAliveSocketTimeout;
extern DWORD GlobalSocketSendBufferLength;
extern DWORD GlobalSocketReceiveBufferLength;
extern DWORD GlobalMaxHttpRedirects;
extern DWORD GlobalMaxConnectionsPerServer;
extern DWORD GlobalMaxConnectionsPer1_0Server;
extern DWORD GlobalConnectionInactiveTimeout;
extern DWORD GlobalServerInfoTimeout;
extern BOOL  GlobalHaveInternetOpened;

extern BOOL InDllCleanup;
extern BOOL GlobalDynaUnload;
extern BOOL GlobalUseSchannelDirectly;
extern BOOL GlobalDisableKeepAlive;
extern DWORD GlobalSecureProtocols;
extern BOOL GlobalEnableHttp1_1;
extern BOOL GlobalEnableProxyHttp1_1;
extern BOOL GlobalDisableReadRange;
extern BOOL GlobalIsProcessExplorer;
extern BOOL GlobalEnableFortezza;
extern BOOL GlobalEnableRevocation;

#if defined(SITARA)

extern BOOL GlobalEnableSitara;
extern BOOL GlobalHasSitaraModemConn;

#endif // SITARA

extern BOOL GlobalEnableUtf8Encoding;

extern BOOL GlobalBypassEditedEntry;
extern BOOL fDontUseDNSLoadBalancing;
extern BOOL GlobalDisableNT4RasCheck;

extern BOOL GlobalWarnOnPost;
extern BOOL GlobalWarnAlways;
extern BOOL GlobalWarnOnZoneCrossing;
extern BOOL GlobalWarnOnBadCertSending;
extern BOOL GlobalWarnOnBadCertRecving;
extern BOOL GlobalWarnOnPostRedirect;
extern BOOL GlobalAlwaysDrainOnRedirect;

extern LONG GlobalInternetOpenHandleCount;
extern DWORD GlobalProxyVersionCount;
extern BOOL GlobalAutoProxyInInit;
extern BOOL GlobalAutoProxyCacheEnable;
extern BOOL GlobalDisplayScriptDownloadFailureUI;
extern BOOL GlobalUseLanSettings;
//extern BOOL GlobalAutoProxyInDeInit;

//extern DWORD GlobalServerInfoAllocCount;
//extern DWORD GlobalServerInfoDeAllocCount;

extern SERIALIZED_LIST GlobalObjectList;
extern SERIALIZED_LIST GlobalServerInfoList;

extern LONGLONG dwdwHttpDefaultExpiryDelta;
extern LONGLONG dwdwFtpDefaultExpiryDelta;
extern LONGLONG dwdwGopherDefaultExpiryDelta;
extern LONGLONG dwdwSessionStartTime;
extern LONGLONG dwdwSessionStartTimeDefaultDelta;

extern DWORD GlobalUrlCacheSyncMode;
extern DWORD GlobalDiskUsageLowerBound;
extern DWORD GlobalScavengeFileLifeTime;

extern LPSTR vszMimeExclusionList, vszHeaderExclusionList;

extern LPSTR *lpvrgszMimeExclusionTable, *lpvrgszHeaderExclusionTable;

extern DWORD *lpvrgdwMimeExclusionTableOfSizes;

extern DWORD vdwMimeExclusionTableCount, vdwHeaderExclusionTableCount;


extern SECURITY_CACHE_LIST GlobalCertCache;

extern BOOL GlobalDisableSslCaching;
extern BOOL GlobalDisableNTLMPreAuth;

extern CRITICAL_SECTION AuthenticationCritSec;
extern CRITICAL_SECTION GeneralInitCritSec;
extern CRITICAL_SECTION LockRequestFileCritSec;
extern CRITICAL_SECTION AutoProxyDllCritSec;
extern CRITICAL_SECTION ZoneMgrCritSec;
extern CRITICAL_SECTION MlangCritSec;

extern const char vszSyncMode[];

extern const char vszDisableSslCaching[];


// moved to proxysup.hxx
//extern PROXY_INFO GlobalProxyInfo;

extern BOOL vfPerUserCookies;
extern BOOL vfAllowCookies;
extern DWORD GlobalDisableNewCookieDlg;

BOOL GetWininetUserName(VOID);
// BUGBUG: GetWininetUserName must be called before accessing vszCurrentUser.
// Instead, it should return the username ptr and the global not accessed.
extern char vszCurrentUser[];
extern DWORD vdwCurrentUserLen;

extern const char vszAllowCookies[];
extern const char vszPerUserCookies[];

extern INTERNET_VERSION_INFO InternetVersionInfo;
extern HTTP_VERSION_INFO HttpVersionInfo;
extern BOOL fCdromDialogActive;
extern DWORD g_dwCredPersistAvail;

extern CUserName GlobalUserName;

//
// The following globals are literal strings passed to winsock.
// Do NOT make them const, otherwise they end up in .text section,
// and web release of winsock2 has a bug where it locks and dirties
// send buffers, confusing the win95 vmm and resulting in code
// getting corrupted when it is paged back in.  -RajeevD
//

extern char gszAt[];
extern char gszBang[];
extern char gszCRLF[3];

//
// novell client32 (hack) "support"
//

extern BOOL GlobalRunningNovellClient32;
extern BOOL GlobalNonBlockingClient32;


// shfolder.dll hmod handle
extern HMODULE g_HMODSHFolder;


extern BOOL GlobalAutoSelectSingleCert;

//
// Localization Structures
//

//
// This definition must be big enough to hold the largest set of localized
// strings.
//

#define LOCAL_STRINGS_MAX_BUFFER 1024

//
// *WARNING* - The order of elements in the following structure must match the
// order of elements in the uStringId array in the FetchLocalStrings routine in
// dll/Globals.cxx.
//

typedef struct {
    LPWSTR
        szEnterAuthInfo,
        szCertInfo,
        szStrengthHigh,
        szStrengthMedium,
        szStrengthLow,
        szCertSubject,
        szCertIssuer,
        szCertEffectiveDate,
        szCertExpirationDate,
        szCertProtocol,
        szCertUsage,
        szHttpsEncryptAlg,
        szHttpsHashAlg,
        szHttpsExchAlg,
        szCertComment,
        szCommentExpires,
        szCommentNotValid,
        szCommentBadCN,
        szCommentBadCA,
        szCommentBadSignature,
        szCommentRevoked,
        szCiphMsg,
        szHashMsg,
        szExchMsg,
        szFingerprint,
        szDomain,
        szRealm,
        szSite,
        szFirewall;

    WCHAR
        rgchBuffer[LOCAL_STRINGS_MAX_BUFFER];
} LOCAL_STRINGS, *PLOCAL_STRINGS;

//
// prototypes
//

VOID
GlobalDllInitialize(
    VOID
    );

VOID
GlobalDllTerminate(
    VOID
    );

DWORD
GlobalDataInitialize(
    VOID
    );

VOID
GlobalDataTerminate(
    VOID
    );

BOOL
IsHttp1_1(
    VOID
    );

BOOL
IsOffline(
    VOID
    );

DWORD
SetOfflineUserState(
    IN DWORD dwState,
    IN BOOL bForce
    );

VOID
GlobalDataReadWarningUIFlags(
    VOID
    );

PLOCAL_STRINGS
FetchLocalStrings(
    VOID
    );

VOID
ChangeGlobalSettings(
    VOID
    );

VOID
RefreshOfflineFromRegistry(
    VOID
    );

VOID
PerformStartupProcessing(
    VOID
    );

DWORD
GetSitaraProtocol(
    VOID
    );


typedef HRESULT
(STDAPICALLTYPE * PFNINETMULTIBYTETOUNICODE)
(
    LPDWORD  lpdword,
    DWORD    dwSrcEncoding,
    LPCSTR   lpSrcStr,
    LPINT    lpnSrcSize,
    LPWSTR   lpDstStr,
    LPINT    lpDstStrSize
);

// Loads Mlang and returns a pointer to the MultiByte to Unicode converter.
// Could return NULL if mlang.dll couldn't be loaded for some reason. 
PFNINETMULTIBYTETOUNICODE GetInetMultiByteToUnicode( );

#if defined(__cplusplus)
}
#endif
