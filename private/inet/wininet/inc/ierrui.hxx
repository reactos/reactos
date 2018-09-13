/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ierrui.hxx

Abstract:

    Contains Function Headers for internal Error UI 
    functions.

Author:

    Arthur L Bierer (arthurbi) 04-Apr-1996

Revision History:

    04-Apr-1996 arthurbi
        Created

--*/


DWORD
LaunchAuthPlugInDlg(
    AUTHCTX * pAuthCtx,
    HWND hWnd,
    DWORD dwError,             
    DWORD dwFlags,
    InvalidPassType *pipAuthUIInfo
    );

DWORD
LaunchDlg(
    IN HWND      hWnd,
    IN LPVOID    lpParam,
    IN DWORD     dwDlgResource,
    IN DLGPROC   pDlgProc
    );

DWORD
LaunchW95ErrorDlg(
    HWND hWnd,
    InvalidPassType *pipAuthUIInfo,
    BOOL fSilent
    );

DWORD
MapWininetErrorToDlgId(
    IN  DWORD        dwError,
    OUT LPDWORD     lpdwDlgId,
    OUT LPDWORD     lpdwDlgFlags,
    OUT DLGPROC     *ppDlgProc
    );

INT_PTR
CALLBACK
CertPickDialogProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
    );

INT_PTR
CALLBACK
CookieDialogProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
    );


#define MAX_CERT_TEXT_SIZE          (3*1024)

#define DLG_FLAGS_CAN_HAVE_CERT_INFO            0x0001
#define DLG_FLAGS_HAS_CERT_INFO                 0x0002
#define DLG_FLAGS_HAS_TELL_ME_ABOUT             0x0004
#define DLG_FLAGS_HAS_DISABLED_SELECTION        0x0010
#define DLG_FLAGS_HAS_CERT_TEXT_IN_VOID         0x0020
#define DLG_FLAGS_BRING_TO_FOREGROUND           0x0040

#define DLG_FLAGS_IGNORE_CERT_CN_INVALID        INTERNET_FLAG_IGNORE_CERT_CN_INVALID
#define DLG_FLAGS_IGNORE_CERT_DATE_INVALID      INTERNET_FLAG_IGNORE_CERT_DATE_INVALID
#define DLG_FLAGS_IGNORE_REDIRECT_TO_HTTPS      INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS
#define DLG_FLAGS_IGNORE_REDIRECT_TO_HTTP       INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP
#define DLG_FLAGS_IGNORE_INVALID_CA             SECURITY_FLAG_IGNORE_UNKNOWN_CA
#define DLG_FLAGS_IGNORE_FAILED_REVOCATION      SECURITY_FLAG_IGNORE_REVOCATION

// These flags should not use the same bits as any other SECURITY_FLAGS. 
// These are for internal use only but maintained on the same DWORD 
// as the SECURITY_FLAG bits.
#define DLG_FLAGS_INVALID_CA                    0x01000000
#define DLG_FLAGS_SEC_CERT_CN_INVALID           0x02000000
#define DLG_FLAGS_SEC_CERT_DATE_INVALID         0x04000000
#define DLG_FLAGS_SEC_CERT_REV_FAILED           0x00800000

//
// help topic that the user gets when he clicks on
// "tell me about Internet Security" Button 
//

#define HELP_TOPIC_SECURITY                             0x104f


typedef struct {
    DWORD       dwDlgFlags;
    DWORD       dwDlgId;
    HINTERNET   hInternetMapped;        
    LPVOID      lpVoid;
} ERRORINFODLGTYPE, *PERRORINFODLGTYPE;

typedef struct _COOKIE_DLG_INFO {
    LPWSTR pszServer;
    PINTERNET_COOKIE pic;
    DWORD   dwStopWarning;
    INT     cx;
    INT     cy;
} COOKIE_DLG_INFO, *PCOOKIE_DLG_INFO;

