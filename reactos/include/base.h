/* 
   Base.h

   Base definitions

   Copyright (C) 1996, 1997 Free Software Foundation, Inc.

   Author: Scott Christley <scottc@net-community.com>

   This file is part of the Windows32 API Library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   If you are interested in a warranty or support for this source code,
   contact Scott Christley <scottc@net-community.com> for more information.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; see the file COPYING.LIB.
   If not, write to the Free Software Foundation, 
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/ 

#ifndef _GNU_H_WINDOWS32_BASE
#define _GNU_H_WINDOWS32_BASE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef NULL
#ifdef __cplusplus
#define NULL  (0)
#else
#define NULL  ((void *)0)
#endif
#endif /* !NULL */

#include <ntos/types.h>

/* Check VOID before defining CHAR, SHORT, and LONG */
#ifndef VOID
#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
#endif


#define CONST const

#define WINAPI      STDCALL
#define APIENTRY    STDCALL
#define WINGDIAPI


#ifndef _DISABLE_TIDENTS
#ifdef UNICODE
typedef wchar_t *LPTCH;
typedef wchar_t *LPTSTR;
#else
typedef char *LPTCH;
typedef char *LPTSTR;
#endif /* UNICODE */
#endif /* _DISABLE_TIDENTS */

#ifndef RC_INVOKED

/* typedef ACMDRIVERENUMCB;
typedef ACMDRIVERPROC;
typedef ACMFILERCHOOSEHOOKPROC;
typedef ACMFILTERENUMCB;
typedef ACMFILTERTAGENUMCB;
typedef ACMFORMATCHOOSEHOOKPROC;
typedef ACMFORMATENUMCB;
typedef ACMFORMATTAGENUMCB;
typedef APPLET_PROC;
*/
/* Changed from BOOL to WINBOOL to avoid Objective-C conflict */
typedef unsigned long CALTYPE;
typedef unsigned long CALID;
typedef unsigned long COLORREF;

/*
typedef CTRYID;
typedef DLGPROC;
*/
typedef double DWORDLONG, *PDWORDLONG;
/*
typedef EDITWORDBREAKPROC;
typedef ENHMFENUMPROC;
typedef ENUMRESLANGPROC;
typedef ENUMRESNAMEPROC;
typedef ENUMRESTYPEPROC;
*/
typedef float FLOAT;
/* typedef GLOBALHANDLE; */
typedef HANDLE HACCEL;
typedef HANDLE HBITMAP;
typedef HANDLE HBRUSH;
typedef HANDLE HCOLORSPACE;
typedef HANDLE HCONV;
typedef HANDLE HCONVLIST;
typedef HANDLE HCURSOR;
typedef HANDLE HDBC;
typedef HANDLE HDC;
typedef HANDLE HDDEDATA;
typedef HANDLE HDESK;
typedef HANDLE HDROP;
typedef HANDLE HDWP;
typedef HANDLE HENHMETAFILE;
typedef HANDLE HENV;
typedef USHORT COLOR16;
typedef int HFILE;
typedef HANDLE HFONT;
typedef HANDLE HGDIOBJ;
typedef HANDLE HGLOBAL;
typedef HANDLE HGLRC;
typedef HANDLE HHOOK;
typedef HANDLE HICON;
typedef HANDLE HIMAGELIST;
typedef HANDLE HINSTANCE;
typedef HANDLE HKEY, *PHKEY;
typedef HANDLE HKL;
typedef LONG    GEOID;
typedef DWORD   GEOTYPE;
typedef DWORD   GEOCLASS;
typedef HANDLE HLOCAL;
typedef HANDLE HMENU;
typedef HANDLE HMETAFILE;
typedef HANDLE HMODULE;
typedef HANDLE HPALETTE;
typedef HANDLE HPEN;
typedef HANDLE HRASCONN;
typedef DWORD LGRPID;
typedef long HRESULT;
typedef HANDLE HRGN;
typedef HANDLE HRSRC;
typedef HANDLE HSTMT;
typedef HANDLE HSZ;
typedef HANDLE HWINSTA;
typedef HANDLE HWND;
typedef HANDLE HRAWINPUT;
typedef HANDLE HTASK;
typedef HANDLE HWINEVENTHOOK;
typedef VOID (CALLBACK *WINEVENTPROC)(HWINEVENTHOOK hWinEventHook,DWORD event,HWND hwnd,LONG idObject,LONG idChild,DWORD idEventThread,DWORD dwmsEventTime);
typedef unsigned short LANGID;
/*typedef DWORD LCID; */
typedef DWORD LCTYPE;
/* typedef LOCALHANDLE */
typedef unsigned short *LP;
typedef long LPARAM;
typedef int WINBOOL;
typedef WINBOOL *LPBOOL;
typedef CONST CHAR *LPCCH;
typedef CHAR *LPCH;
typedef COLORREF *LPCOLORREF;

#ifndef _DISABLE_TIDENTS
#ifdef UNICODE
typedef const wchar_t *LPCTSTR;
#else
typedef const char *LPCTSTR;
#endif /* UNICODE */
#endif /* _DISABLE_TIDENTS */

typedef const wchar_t *LPCWCH;
typedef DWORD *LPDWORD;
/* typedef LPFRHOOKPROC; */
typedef HANDLE *LPHANDLE;
typedef DWORD FULLSCREENCONTROL;
typedef DWORD UNIVERSAL_FONT_ID;
typedef UNIVERSAL_FONT_ID *PUNIVERSAL_FONT_ID;
typedef DWORD REALIZATION_INFO;
typedef REALIZATION_INFO *PREALIZATION_INFO;
typedef DWORD SHAREDHANDLETABLE;
typedef SHAREDHANDLETABLE *PSHAREDHANDLETABLE;
typedef DWORD CHWIDTHINFO;
typedef CHWIDTHINFO *PCHWIDTHINFO;
/* typedef LPHANDLER_FUNCTION; */
typedef PINT LPINT;
typedef PLONG LPLONG;

typedef long LRESULT;
typedef wchar_t *LPWCH;
typedef unsigned short *LPWORD;
/* typedef NPSTR; */
typedef wchar_t *NWPSTR;
typedef WINBOOL *PWINBOOL;
typedef const CHAR *PCCH;
typedef const char *PCSTR;
typedef const wchar_t *PCWCH;
/* typedef PHKEY; */
/* typedef LCID *PLCID; */
typedef short *PSHORT;
/* typedef PSID; */
typedef char *PSTR;
typedef char *PSZ;

#ifndef _DISABLE_TIDENTS
#ifdef UNICODE
typedef wchar_t *PTBYTE;
typedef wchar_t *PTCH;
typedef wchar_t *PTCHAR;
typedef wchar_t *PTSTR;
#else
typedef unsigned char *PTBYTE;
typedef char *PTCH;
typedef char *PTCHAR;
typedef char *PTSTR;
#endif /* UNICODE */
#endif /* _DISABLE_TIDENTS */

/*
 typedef PWSTR;
 */
typedef DWORD REGSAM;


typedef short RETCODE;

typedef HANDLE SC_HANDLE;
typedef LPVOID  SC_LOCK;
typedef SC_HANDLE *LPSC_HANDLE;
typedef DWORD SERVICE_STATUS_HANDLE;
/* typedef SPHANDLE; */

#ifndef _DISABLE_TIDENTS
#ifdef UNICODE
typedef wchar_t TBYTE;
#ifndef _TCHAR_DEFINED
#define _TCHAR_DEFINED
typedef wchar_t TCHAR;
#endif /* _TCHAR_DEFINED */
typedef wchar_t BCHAR;
#else
typedef unsigned char TBYTE;
#ifndef _TCHAR_DEFINED
#define _TCHAR_DEFINED
typedef char TCHAR;
#endif /* _TCHAR_DEFINED */
typedef BYTE BCHAR;
#endif /* UNICODE */
#endif /* _DISABLE_TIDENTS */

typedef unsigned int WPARAM;
/* typedef YIELDPROC; */

/* Only use __stdcall under WIN32 compiler */

#define _export

/*
  Enumerations
*/


#define GEOID_NOT_AVAILABLE (-1)
/*
  GEO information types for clients to query
*/

enum SYSGEOTYPE {
    GEO_NATION            = 0x01,
    GEO_LATITUDE          = 0x02,
    GEO_LONGITUDE         = 0x03,
    GEO_ISO2              = 0x04,
    GEO_ISO3              = 0x05,
    GEO_RFC1766           = 0x06,
    GEO_LCID              = 0x07,
    GEO_FRIENDLYNAME      = 0x08,
    GEO_OFFICIALNAME      = 0x09,
    GEO_TIMEZONES         = 0x0A,
    GEO_OFFICIALLANGUAGES = 0x0B,
};

/*
  More GEOCLASS defines will be listed here
*/

enum SYSGEOCLASS {
    GEOCLASS_NATION       = 16,
    GEOCLASS_REGION       = 14,
};
 
 
#define RASCS_DONE 0x2000
#define RASCS_PAUSED 0x1000
typedef enum _RASCONNSTATE { 
    RASCS_OpenPort = 0, 
    RASCS_PortOpened, 
    RASCS_ConnectDevice, 
    RASCS_DeviceConnected, 
    RASCS_AllDevicesConnected, 
    RASCS_Authenticate, 
    RASCS_AuthNotify, 
    RASCS_AuthRetry, 
    RASCS_AuthCallback, 
    RASCS_AuthChangePassword, 
    RASCS_AuthProject, 
    RASCS_AuthLinkSpeed, 
    RASCS_AuthAck, 
    RASCS_ReAuthenticate, 
    RASCS_Authenticated, 
    RASCS_PrepareForCallback, 
    RASCS_WaitForModemReset, 
    RASCS_WaitForCallback,
    RASCS_Projected, 
 
    RASCS_StartAuthentication,  
    RASCS_CallbackComplete,     
    RASCS_LogonNetwork,         
 
    RASCS_Interactive = RASCS_PAUSED, 
    RASCS_RetryAuthentication, 
    RASCS_CallbackSetByCaller, 
    RASCS_PasswordExpired, 
 
    RASCS_Connected = RASCS_DONE, 
    RASCS_Disconnected 
} RASCONNSTATE ; 
 
typedef enum _RASPROJECTION {  
    RASP_Amb = 0x10000, 
    RASP_PppNbf = 0x803F, 
    RASP_PppIpx = 0x802B, 
    RASP_PppIp = 0x8021 
} RASPROJECTION ; 
  
typedef enum _SID_NAME_USE { 
    SidTypeUser = 1, 
    SidTypeGroup, 
    SidTypeDomain, 
    SidTypeAlias, 
    SidTypeWellKnownGroup, 
    SidTypeDeletedAccount, 
    SidTypeInvalid, 
    SidTypeUnknown 
} SID_NAME_USE, *PSID_NAME_USE; 
 
#endif /* ! defined (RC_INVOKED) */

/*
  Macros
*/
#define FORWARD_WM_NOTIFY(hwnd, idFrom, pnmhdr, fn)  (void)(fn)((hwnd), WM_NOTIFY, (WPARAM)(int)(id),  (LPARAM)(NMHDR FAR*)(pnmhdr)) 

#define GetBValue(rgb)   ((BYTE) ((rgb) >> 16)) 
#define GetGValue(rgb)   ((BYTE) (((WORD) (rgb)) >> 8)) 
#define GetRValue(rgb)   ((BYTE) (rgb)) 
#define RGB(r, g ,b)  ((DWORD) (((BYTE) (r) | ((WORD) (g) << 8)) | (((DWORD) (BYTE) (b)) << 16))) 

#define HANDLE_WM_NOTIFY(hwnd, wParam, lParam, fn) (fn)((hwnd), (int)(wParam), (NMHDR FAR*)(lParam)) 

#define HIBYTE(w)   ((BYTE) (((WORD) (w) >> 8) & 0xFF)) 
#define HIWORD(l)   ((WORD) (((DWORD) (l) >> 16) & 0xFFFF)) 
#define SHIWORD(l)   ((INT16) (((DWORD) (l) >> 16) & 0xFFFF)) 
#define LOBYTE(w)   ((BYTE) (w)) 
#define LOWORD(l)   ((WORD) (l)) 
#define SLOWORD(l)   ((INT16) (l)) 
#define MAKELONG(a, b) ((LONG) (((WORD) (a)) | ((DWORD) ((WORD) (b))) << 16)) 
#define MAKEWORD(a, b) ((WORD) (((BYTE) (a)) | ((WORD) ((BYTE) (b))) << 8)) 

/* original Cygnus headers also had the following defined: */
#define SEXT_HIWORD(l)     ((((int)l) >> 16))
#define ZEXT_HIWORD(l)     ((((unsigned int)l) >> 16))
#define SEXT_LOWORD(l)     ((int)(short)l)

#define INDEXTOOVERLAYMASK(i) ((i) << 8) 
#define INDEXTOSTATEIMAGEMASK(i) ((i) << 12) 

#define MAKEINTATOM(i)   (LPTSTR) ((DWORD) ((WORD) (i))) 
#ifndef _DISABLE_TIDENTS
#define MAKEINTRESOURCE(i)   (LPTSTR) ((ULONG_PTR) ((WORD) (i)))
#endif /* _DISABLE_TIDENTS */
#define MAKEINTRESOURCEA(i)  (LPSTR)  ((ULONG_PTR) ((WORD) (i)))
#define MAKEINTRESOURCEW(i)  (LPWSTR) ((ULONG_PTR) ((WORD) (i)))
#define IS_INTRESOURCE(n)    ((((ULONG_PTR) (n)) >> 16) == 0)

#define MAKELANGID(p, s) ((((WORD) (s)) << 10) | (WORD) (p)) 
#define PRIMARYLANGID(lgid)    ((WORD  )(lgid) & 0x3ff) 
#define SUBLANGID(lgid)        ((WORD  )(lgid) >> 10) 

#define LANGIDFROMLCID(lcid)   ((WORD) (lcid)) 
#define SORTIDFROMLCID(lcid) ((WORD  )((((DWORD)(lcid)) & 0x000FFFFF) >> 16)) 
#define MAKELCID(lgid, srtid)  ((DWORD)((((DWORD)((WORD)(srtid))) << 16) |  ((DWORD)((WORD)(lgid))))) 
#define MAKELPARAM(l, h)   ((LPARAM) MAKELONG(l, h)) 
#define MAKELRESULT(l, h)   ((LRESULT) MAKELONG(l, h)) 
#define MAKEPOINTS(l)   (*((POINTS FAR *) & (l))) 
#define MAKEROP4(fore,back) (DWORD)((((back) << 8) & 0xFF000000) | (fore)) 
#define MAKEWPARAM(l, h)   ((WPARAM) MAKELONG(l, h)) 


#define PALETTEINDEX(i) ((COLORREF) (0x01000000 | (DWORD) (WORD) (i))) 
#define PALETTERGB(r, g, b)  (0x02000000 | RGB(r, g, b)) 
#define POINTSTOPOINT(pt, pts) {(pt).x = (SHORT) LOWORD(pts); (pt).y = (SHORT) HIWORD(pts);} 
#define POINTTOPOINTS(pt) (MAKELONG((short) ((pt).x), (short) ((pt).y))) 

#define INDEXTOOVERLAYMASK(i) ((i) << 8)  
#define INDEXTOSTATEIMAGEMASK(i) ((i) << 12)  

#ifndef _DISABLE_TIDENTS
#  ifdef UNICODE
#    ifndef _T
#      define _T(quote)   L##quote 
#    endif /* _T */
#    ifndef _TEXT
#      define TEXT(quote) L##quote 
#    endif /* _TEXT */
#  else /* UNICODE */
#    ifndef _T
#      define _T(quote)   quote 
#    endif /* _T */
#    ifndef _TEXT
#      define TEXT(quote) quote
#    endif /* _TEXT */
#  endif /* UNICODE */
#endif /* _DISABLE_TIDENTS */

#ifndef RC_INVOKED

/*
   Definitions for callback procedures
*/
typedef enum {

    WinNullSid                                  = 0,
    WinWorldSid                                 = 1,
    WinLocalSid                                 = 2,
    WinCreatorOwnerSid                          = 3,
    WinCreatorGroupSid                          = 4,
    WinCreatorOwnerServerSid                    = 5,
    WinCreatorGroupServerSid                    = 6,
    WinNtAuthoritySid                           = 7,
    WinDialupSid                                = 8,
    WinNetworkSid                               = 9,
    WinBatchSid                                 = 10,
    WinInteractiveSid                           = 11,
    WinServiceSid                               = 12,
    WinAnonymousSid                             = 13,
    WinProxySid                                 = 14,
    WinEnterpriseControllersSid                 = 15,
    WinSelfSid                                  = 16,
    WinAuthenticatedUserSid                     = 17,
    WinRestrictedCodeSid                        = 18,
    WinTerminalServerSid                        = 19,
    WinRemoteLogonIdSid                         = 20,
    WinLogonIdsSid                              = 21,
    WinLocalSystemSid                           = 22,
    WinLocalServiceSid                          = 23,
    WinNetworkServiceSid                        = 24,
    WinBuiltinDomainSid                         = 25,
    WinBuiltinAdministratorsSid                 = 26,
    WinBuiltinUsersSid                          = 27,
    WinBuiltinGuestsSid                         = 28,
    WinBuiltinPowerUsersSid                     = 29,
    WinBuiltinAccountOperatorsSid               = 30,
    WinBuiltinSystemOperatorsSid                = 31,
    WinBuiltinPrintOperatorsSid                 = 32,
    WinBuiltinBackupOperatorsSid                = 33,
    WinBuiltinReplicatorSid                     = 34,
    WinBuiltinPreWindows2000CompatibleAccessSid = 35,
    WinBuiltinRemoteDesktopUsersSid             = 36,
    WinBuiltinNetworkConfigurationOperatorsSid  = 37,
    WinAccountAdministratorSid                  = 38,
    WinAccountGuestSid                          = 39,
    WinAccountKrbtgtSid                         = 40,
    WinAccountDomainAdminsSid                   = 41,
    WinAccountDomainUsersSid                    = 42,
    WinAccountDomainGuestsSid                   = 43,
    WinAccountComputersSid                      = 44,
    WinAccountControllersSid                    = 45,
    WinAccountCertAdminsSid                     = 46,
    WinAccountSchemaAdminsSid                   = 47,
    WinAccountEnterpriseAdminsSid               = 48,
    WinAccountPolicyAdminsSid                   = 49,
    WinAccountRasAndIasServersSid               = 50,
    WinNTLMAuthenticationSid                    = 51,
    WinDigestAuthenticationSid                  = 52,
    WinSChannelAuthenticationSid                = 53,
    WinThisOrganizationSid                      = 54,
    WinOtherOrganizationSid                     = 55,
    WinBuiltinIncomingForestTrustBuildersSid    = 56,
    WinBuiltinPerfMonitoringUsersSid            = 57,
    WinBuiltinPerfLoggingUsersSid               = 58,
    WinBuiltinAuthorizationAccessSid            = 59,
    WinBuiltinTerminalServerLicenseServersSid   = 60,

} WELL_KNOWN_SID_TYPE;
typedef enum _AUDIT_EVENT_TYPE {
    AuditEventObjectAccess,
    AuditEventDirectoryServiceAccess
} AUDIT_EVENT_TYPE, *PAUDIT_EVENT_TYPE;
typedef int (CALLBACK *BFFCALLBACK) (HWND, UINT, LPARAM, LPARAM);
typedef UINT (CALLBACK *LPCCHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef UINT (CALLBACK *LPCFHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef DWORD (CALLBACK *EDITSTREAMCALLBACK) (DWORD, LPBYTE, LONG, LONG);
typedef UINT (CALLBACK *LPFRHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef UINT (CALLBACK *LPOFNHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef UINT (CALLBACK *LPPRINTHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef UINT (CALLBACK *LPSETUPHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef WINBOOL (CALLBACK *DLGPROC) (HWND, UINT, WPARAM, LPARAM);
typedef int (CALLBACK *PFNPROPSHEETCALLBACK) (HWND, UINT, LPARAM);
typedef VOID (CALLBACK *LPSERVICE_MAIN_FUNCTIONA) (DWORD, LPSTR*);
typedef VOID (CALLBACK *LPSERVICE_MAIN_FUNCTIONW) (DWORD, LPWSTR*);
typedef int (CALLBACK *PFNTVCOMPARE) (LPARAM, LPARAM, LPARAM);
typedef LRESULT (CALLBACK *WNDPROC) (HWND, UINT, WPARAM, LPARAM);
typedef int (CALLBACK *FARPROC)(void);
typedef FARPROC PROC;
typedef WINBOOL (CALLBACK *ENUMRESTYPEPROCA) (HANDLE, LPSTR, LONG);
typedef WINBOOL (CALLBACK *ENUMRESTYPEPROCW) (HANDLE, LPWSTR, LONG);
typedef WINBOOL (CALLBACK *ENUMRESNAMEPROCA) (HANDLE, LPCSTR, LPSTR, LONG);
typedef WINBOOL (CALLBACK *ENUMRESNAMEPROCW) (HANDLE, LPCWSTR, LPWSTR, LONG);
typedef WINBOOL (CALLBACK *ENUMRESLANGPROCA) (HANDLE, LPCSTR, LPCSTR, WORD, LONG);
typedef WINBOOL (CALLBACK *ENUMRESLANGPROCW) (HANDLE, LPCWSTR, LPCWSTR, WORD, LONG);
typedef WINBOOL (CALLBACK *DESKTOPENUMPROCA) (LPSTR,  LPARAM);
typedef WINBOOL (CALLBACK *DESKTOPENUMPROCW) (LPWSTR, LPARAM);
typedef WINBOOL (CALLBACK *ENUMWINDOWSPROC) (HWND, LPARAM);
typedef WINBOOL (CALLBACK *ENUMWINDOWSTATIONPROCA) (LPSTR, LPARAM);
typedef WINBOOL (CALLBACK *ENUMWINDOWSTATIONPROCW) (LPWSTR, LPARAM);
typedef VOID (CALLBACK *SENDASYNCPROC) (HWND, UINT, DWORD, LRESULT);
typedef VOID (CALLBACK *TIMERPROC) (HWND, UINT, UINT, DWORD);
typedef WINBOOL (CALLBACK *GRAYSTRINGPROC) (HDC, LPARAM, int);
typedef WINBOOL (CALLBACK *DRAWSTATEPROC) (HDC, LPARAM, WPARAM, int, int);
typedef WINBOOL (CALLBACK *PROPENUMPROCEXA) (HWND, LPCSTR, HANDLE, DWORD);
typedef WINBOOL (CALLBACK *PROPENUMPROCEXW) (HWND, LPCWSTR, HANDLE, DWORD);
typedef WINBOOL (CALLBACK *PROPENUMPROCA) (HWND, LPCSTR, HANDLE);
typedef WINBOOL (CALLBACK *PROPENUMPROCW) (HWND, LPCWSTR, HANDLE);
typedef LRESULT (CALLBACK *HOOKPROC) (int, WPARAM, LPARAM);
typedef VOID (CALLBACK *ENUMOBJECTSPROC) (LPVOID, LPARAM);
typedef VOID (CALLBACK *LINEDDAPROC) (int, int, LPARAM);
typedef WINBOOL (CALLBACK *ABORTPROC) (HDC, int);
typedef UINT (CALLBACK *LPPAGEPAINTHOOK) (HWND, UINT, WPARAM, LPARAM );
typedef UINT (CALLBACK *LPPAGESETUPHOOK) (HWND, UINT, WPARAM, LPARAM );
typedef int (CALLBACK *ICMENUMPROCA) (LPSTR, LPARAM);
typedef int (CALLBACK *ICMENUMPROCW) (LPWSTR, LPARAM);
typedef LONG (*EDITWORDBREAKPROCEX) (char *, LONG, BYTE, INT);
typedef int (CALLBACK *PFNLVCOMPARE) (LPARAM, LPARAM, LPARAM);
typedef WINBOOL (CALLBACK *LOCALE_ENUMPROCA) (LPSTR);
typedef WINBOOL (CALLBACK *LOCALE_ENUMPROCW) (LPWSTR);
typedef WINBOOL (CALLBACK *CODEPAGE_ENUMPROCA) (LPSTR);
typedef WINBOOL (CALLBACK *CODEPAGE_ENUMPROCW) (LPWSTR);
typedef WINBOOL (CALLBACK *DATEFMT_ENUMPROCA) (LPSTR);
typedef WINBOOL (CALLBACK *DATEFMT_ENUMPROCW) (LPWSTR);
typedef WINBOOL (CALLBACK *TIMEFMT_ENUMPROCA) (LPSTR);
typedef WINBOOL (CALLBACK *TIMEFMT_ENUMPROCW) (LPWSTR);
typedef WINBOOL (CALLBACK *CALINFO_ENUMPROCA) (LPSTR);
typedef int (CALLBACK *EMFPLAYPROC)( HDC hdc, INT iFunction, HANDLE hPageQuery );
typedef WINBOOL (CALLBACK *CALINFO_ENUMPROCW) (LPWSTR);
typedef WINBOOL (CALLBACK *PHANDLER_ROUTINE) (DWORD);
typedef VOID (CALLBACK *LPHANDLER_FUNCTION) (DWORD);
typedef DWORD (CALLBACK *LPHANDLER_FUNCTION_EX)(DWORD,DWORD,LPVOID,LPVOID);
typedef UINT (CALLBACK *PFNGETPROFILEPATHA) (LPCSTR, LPSTR, UINT);
typedef UINT (CALLBACK *PFNGETPROFILEPATHW) (LPCWSTR, LPWSTR, UINT);
typedef UINT (CALLBACK *PFNRECONCILEPROFILEA) (LPCSTR, LPCSTR, DWORD);
typedef UINT (CALLBACK *PFNRECONCILEPROFILEW) (LPCWSTR, LPCWSTR, DWORD);
typedef WINBOOL (CALLBACK *PFNPROCESSPOLICIESA) (HWND, LPCSTR, LPCSTR, LPCSTR, DWORD);
typedef WINBOOL (CALLBACK *PFNPROCESSPOLICIESW) (HWND, LPCWSTR, LPCWSTR, LPCWSTR, DWORD);

#ifndef _DISABLE_TIDENTS
#ifdef UNICODE
#define LPSERVICE_MAIN_FUNCTION LPSERVICE_MAIN_FUNCTIONW
#define ENUMRESTYPEPROC ENUMRESTYPEPROCW
#define ENUMRESNAMEPROC ENUMRESNAMEPROCW
#define ENUMRESLANGPROC ENUMRESLANGPROCW
#define DESKTOPENUMPROC DESKTOPENUMPROCW
#define ENUMWINDOWSTATIONPROC ENUMWINDOWSTATIONPROCW
#define PROPENUMPROCEX PROPENUMPROCEXW
#define PROPENUMPROC PROPENUMPROCW
#define ICMENUMPROC ICMENUMPROCW
#define LOCALE_ENUMPROC LOCALE_ENUMPROCW
#define CODEPAGE_ENUMPROC CODEPAGE_ENUMPROCW
#define DATEFMT_ENUMPROC DATEFMT_ENUMPROCW
#define TIMEFMT_ENUMPROC TIMEFMT_ENUMPROCW
#define CALINFO_ENUMPROC CALINFO_ENUMPROCW
#define PFNGETPROFILEPATH PFNGETPROFILEPATHW
#define PFNRECONCILEPROFILE PFNRECONCILEPROFILEW
#define PFNPROCESSPOLICIES PFNPROCESSPOLICIESW
#else /* UNICODE */
#define LPSERVICE_MAIN_FUNCTION LPSERVICE_MAIN_FUNCTIONA
#define ENUMRESTYPEPROC ENUMRESTYPEPROCA
#define ENUMRESNAMEPROC ENUMRESNAMEPROCA
#define ENUMRESLANGPROC ENUMRESLANGPROCA
#define DESKTOPENUMPROC DESKTOPENUMPROCA
#define ENUMWINDOWSTATIONPROC ENUMWINDOWSTATIONPROCA
#define PROPENUMPROCEX PROPENUMPROCEXA
#define PROPENUMPROC PROPENUMPROCA
#define ICMENUMPROC ICMENUMPROCA
#define LOCALE_ENUMPROC LOCALE_ENUMPROCA
#define CODEPAGE_ENUMPROC CODEPAGE_ENUMPROCA
#define DATEFMT_ENUMPROC DATEFMT_ENUMPROCA
#define TIMEFMT_ENUMPROC TIMEFMT_ENUMPROCA
#define CALINFO_ENUMPROC CALINFO_ENUMPROCA
#define PFNGETPROFILEPATH PFNGETPROFILEPATHA
#define PFNRECONCILEPROFILE PFNRECONCILEPROFILEA
#define PFNPROCESSPOLICIES PFNPROCESSPOLICIESA
#endif /* UNICODE */
#endif /* _DISABLE_TIDENTS */

#define SECURITY_NULL_SID_AUTHORITY     {0,0,0,0,0,0}
#define SECURITY_WORLD_SID_AUTHORITY    {0,0,0,0,0,1}
#define SECURITY_LOCAL_SID_AUTHORITY    {0,0,0,0,0,2}
#define SECURITY_CREATOR_SID_AUTHORITY  {0,0,0,0,0,3}
#define SECURITY_NON_UNIQUE_AUTHORITY   {0,0,0,0,0,4}
#define SECURITY_NT_AUTHORITY           {0,0,0,0,0,5}

#define SE_CREATE_TOKEN_NAME              TEXT("SeCreateTokenPrivilege")
#define SE_ASSIGNPRIMARYTOKEN_NAME        TEXT("SeAssignPrimaryTokenPrivilege")
#define SE_LOCK_MEMORY_NAME               TEXT("SeLockMemoryPrivilege")
#define SE_INCREASE_QUOTA_NAME            TEXT("SeIncreaseQuotaPrivilege")
#define SE_UNSOLICITED_INPUT_NAME         TEXT("SeUnsolicitedInputPrivilege")
#define SE_MACHINE_ACCOUNT_NAME           TEXT("SeMachineAccountPrivilege")
#define SE_TCB_NAME                       TEXT("SeTcbPrivilege")
#define SE_SECURITY_NAME                  TEXT("SeSecurityPrivilege")
#define SE_TAKE_OWNERSHIP_NAME            TEXT("SeTakeOwnershipPrivilege")
#define SE_LOAD_DRIVER_NAME               TEXT("SeLoadDriverPrivilege")
#define SE_SYSTEM_PROFILE_NAME            TEXT("SeSystemProfilePrivilege")
#define SE_SYSTEMTIME_NAME                TEXT("SeSystemtimePrivilege")
#define SE_PROF_SINGLE_PROCESS_NAME       TEXT("SeProfileSingleProcessPrivilege")
#define SE_INC_BASE_PRIORITY_NAME         TEXT("SeIncreaseBasePriorityPrivilege")
#define SE_CREATE_PAGEFILE_NAME           TEXT("SeCreatePagefilePrivilege")
#define SE_CREATE_PERMANENT_NAME          TEXT("SeCreatePermanentPrivilege")
#define SE_BACKUP_NAME                    TEXT("SeBackupPrivilege")
#define SE_RESTORE_NAME                   TEXT("SeRestorePrivilege")
#define SE_SHUTDOWN_NAME                  TEXT("SeShutdownPrivilege")
#define SE_DEBUG_NAME                     TEXT("SeDebugPrivilege")
#define SE_AUDIT_NAME                     TEXT("SeAuditPrivilege")
#define SE_SYSTEM_ENVIRONMENT_NAME        TEXT("SeSystemEnvironmentPrivilege")
#define SE_CHANGE_NOTIFY_NAME             TEXT("SeChangeNotifyPrivilege")
#define SE_REMOTE_SHUTDOWN_NAME           TEXT("SeRemoteShutdownPrivilege")

typedef BOOL (CALLBACK *LANGUAGEGROUP_ENUMPROCA)(LGRPID, LPSTR, LPSTR, DWORD, LONG_PTR);
typedef BOOL (CALLBACK *LANGGROUPLOCALE_ENUMPROCA)(LGRPID, LCID, LPSTR, LONG_PTR);
typedef BOOL (CALLBACK *UILANGUAGE_ENUMPROCA)(LPSTR, LONG_PTR);
typedef BOOL (CALLBACK *DATEFMT_ENUMPROCEXA)(LPSTR, CALID);
typedef BOOL (CALLBACK *CALINFO_ENUMPROCEXA)(LPSTR, CALID);

typedef BOOL (CALLBACK *LANGUAGEGROUP_ENUMPROCW)(LGRPID, LPWSTR, LPWSTR, DWORD, LONG_PTR);
typedef BOOL (CALLBACK *LANGGROUPLOCALE_ENUMPROCW)(LGRPID, LCID, LPWSTR, LONG_PTR);
typedef BOOL (CALLBACK *UILANGUAGE_ENUMPROCW)(LPWSTR, LONG_PTR);
typedef BOOL (CALLBACK *DATEFMT_ENUMPROCEXW)(LPWSTR, CALID);
typedef BOOL (CALLBACK *CALINFO_ENUMPROCEXW)(LPWSTR, CALID);
typedef BOOL (CALLBACK *GEO_ENUMPROC)(GEOID);

#define SERVICES_ACTIVE_DATABASEW      L"ServicesActive"
#define SERVICES_FAILED_DATABASEW      L"ServicesFailed"
#define SERVICES_ACTIVE_DATABASEA      "ServicesActive"
#define SERVICES_FAILED_DATABASEA      "ServicesFailed"
#define SC_GROUP_IDENTIFIERW           L'+'
#define SC_GROUP_IDENTIFIERA           '+'

#ifndef _DISABLE_TIDENTS
#ifdef UNICODE
#define SERVICES_ACTIVE_DATABASE       SERVICES_ACTIVE_DATABASEW
#define SERVICES_FAILED_DATABASE       SERVICES_FAILED_DATABASEW
#define SC_GROUP_IDENTIFIER            SC_GROUP_IDENTIFIERW
#else 
#define SERVICES_ACTIVE_DATABASE       SERVICES_ACTIVE_DATABASEA
#define SERVICES_FAILED_DATABASE       SERVICES_FAILED_DATABASEA
#define SC_GROUP_IDENTIFIER            SC_GROUP_IDENTIFIERA
#endif /* UNICODE */
#endif /* _DISABLE_TIDENTS */

#define MM_MAX_NUMAXES      16

/* ---------------------------------- */
/* From ddeml.h in old Cygnus headers */

typedef void (*CALLB) (void);
typedef CALLB PFNCALLBACK;


typedef enum _COMPUTER_NAME_FORMAT {
    ComputerNameNetBIOS,
    ComputerNameDnsHostname,
    ComputerNameDnsDomain,
    ComputerNameDnsFullyQualified,
    ComputerNamePhysicalNetBIOS,
    ComputerNamePhysicalDnsHostname,
    ComputerNamePhysicalDnsDomain,
    ComputerNamePhysicalDnsFullyQualified,
    ComputerNameMax
} COMPUTER_NAME_FORMAT ;

typedef enum _HEAP_INFORMATION_CLASS {

    HeapCompatibilityInformation

} HEAP_INFORMATION_CLASS;

typedef enum {
    LT_DONT_CARE,
    LT_LOWEST_LATENCY
} LATENCY_TIME;

typedef LONG (CALLBACK *PVECTORED_EXCEPTION_HANDLER)(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );

typedef
VOID
(CALLBACK *PAPCFUNC)(
    ULONG_PTR dwParam
    );

#ifdef __cplusplus
#define REFGUID const GUID &
#else
#define REFGUID const GUID *
#endif

typedef DWORD (CALLBACK *PFE_EXPORT_FUNC)(PBYTE pbData,PVOID pvCallbackContext,ULONG ulLength);

typedef DWORD (CALLBACK *PFE_IMPORT_FUNC)(PBYTE pbData,PVOID pvCallbackContext,PULONG ulLength);
typedef VOID (CALLBACK *PFIBER_START_ROUTINE)(
    LPVOID lpFiberParameter
    );
typedef PFIBER_START_ROUTINE LPFIBER_START_ROUTINE;

typedef enum _MEMORY_RESOURCE_NOTIFICATION_TYPE {
    LowMemoryResourceNotification,
    HighMemoryResourceNotification
} MEMORY_RESOURCE_NOTIFICATION_TYPE;

typedef VOID (CALLBACK *WAITORTIMERCALLBACKFUNC) (PVOID, BOOLEAN );
typedef WAITORTIMERCALLBACKFUNC WAITORTIMERCALLBACK;
/* End of stuff from ddeml.h in old Cygnus headers */
/* ----------------------------------------------- */

typedef FARPROC WNDENUMPROC;
typedef FARPROC MFENUMPROC;
typedef FARPROC ENHMFENUMPROC;
typedef DWORD CCSTYLE, *PCCSTYLE, *LPCCSTYLE;
typedef DWORD CCSTYLEFLAGA, *PCCSTYLEFLAGA, *LPCCSTYLEFLAGA;
#define DECLARE_HANDLE(s) typedef HANDLE s
typedef LANGID *PLANGID;

#endif /* ! defined (RC_INVOKED) */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GNU_H_WINDOWS32_BASE */
