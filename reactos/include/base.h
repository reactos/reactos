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

/*
 * Definitions needed for the ddk includes (we miss out win32 only stuff to
 * cut down on the compile time)					    
 */
typedef unsigned char UCHAR;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef unsigned short WCHAR;
typedef unsigned short WORD;
typedef int WINBOOL;
typedef unsigned char BOOLEAN;
typedef unsigned int DWORD; /* was unsigned long */
typedef unsigned short *LPWSTR;
typedef unsigned short *PWSTR;
typedef unsigned char *PUCHAR;
typedef unsigned int *PUINT;
typedef unsigned long *PULONG;
typedef unsigned short *PUSHORT;
typedef void *PVOID;
typedef unsigned char BYTE;
typedef void *LPVOID;

/* Check VOID before defining CHAR, SHORT, and LONG */
#ifndef VOID
#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
#endif

typedef CHAR *PCHAR;
typedef CHAR *PCH;
typedef void *HANDLE;
typedef char CCHAR;

typedef enum _SECURITY_IMPERSONATION_LEVEL {
    SecurityAnonymous, 
    SecurityIdentification, 
    SecurityImpersonation, 
    SecurityDelegation 
} SECURITY_IMPERSONATION_LEVEL; 

typedef enum tagTOKEN_TYPE {
    TokenPrimary = 1, 
    TokenImpersonation 
} TOKEN_TYPE; 

#define FALSE 0
#define TRUE 1

   typedef const unsigned short *PCWSTR;

typedef char* PCSZ;
   
#define CONST const

#ifdef i386
#define STDCALL     __attribute__ ((stdcall))
#define CDECL       __attribute((cdecl))
#define CALLBACK    WINAPI
#define PASCAL      WINAPI
#else
#define STDCALL
#define CDECL
#define CALLBACK
#define PASCAL
#endif
#define WINAPI      STDCALL
#define APIENTRY    STDCALL
#define WINGDIAPI

typedef BYTE *PBOOLEAN;
typedef HANDLE *PHANDLE;
   
typedef DWORD CALLBACK (*PTHREAD_START_ROUTINE) (LPVOID);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

   typedef unsigned short ATOM;

   #ifdef UNICODE
typedef unsigned short *LPTCH;
typedef unsigned short *LPTSTR;
#else
typedef char *LPTCH;
typedef char *LPTSTR;
#endif /* UNICODE */

typedef long *PLONG;
typedef unsigned short *PWCHAR;
typedef char *LPSTR;
typedef double LONGLONG, *PLONGLONG;

   typedef enum _MEDIA_TYPE { 
  Unknown,                
  F5_1Pt2_512,            
  F3_1Pt44_512,           
  F3_2Pt88_512,           
  F3_20Pt8_512,           
  F3_720_512,             
  F5_360_512,             
  F5_320_512,             
  F5_320_1024,            
  F5_180_512,             
  F5_160_512,             
  RemovableMedia,         
  FixedMedia              
} MEDIA_TYPE; 

#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b)) 
#endif

#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b)) 
#endif

   
#ifndef WIN32_LEAN_AND_MEAN
   

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
typedef HANDLE HLOCAL;
typedef HANDLE HMENU;
typedef HANDLE HMETAFILE;
typedef HANDLE HMODULE;
typedef HANDLE HPALETTE;
typedef HANDLE HPEN;
typedef HANDLE HRASCONN;
typedef long HRESULT;
typedef HANDLE HRGN;
typedef HANDLE HRSRC;
typedef HANDLE HSTMT;
typedef HANDLE HSZ;
typedef HANDLE HWINSTA;
typedef HANDLE HWND;
typedef int INT;
typedef unsigned short LANGID;
typedef DWORD LCID;
typedef DWORD LCTYPE;
/* typedef LOCALHANDLE */
typedef unsigned short *LP;
typedef long LPARAM;
typedef WINBOOL *LPBOOL;
typedef BYTE *LPBYTE;
typedef CONST CHAR *LPCCH;
typedef CHAR *LPCH;
typedef COLORREF *LPCOLORREF;
typedef const char *LPCSTR;
   
#ifdef UNICODE
typedef const unsigned short *LPCTSTR;
#else
typedef const char *LPCTSTR;
#endif /* UNICODE */

typedef const unsigned short *LPCWCH;
typedef const unsigned short *LPCWSTR;
typedef DWORD *LPDWORD;
/* typedef LPFRHOOKPROC; */
typedef HANDLE *LPHANDLE;
/* typedef LPHANDLER_FUNCTION; */
typedef int *LPINT;
typedef long *LPLONG;

typedef long LRESULT;
typedef const void *LPCVOID;
typedef unsigned short *LPWCH;
typedef unsigned short *LPWORD;
/* typedef NPSTR; */
typedef unsigned short *NWPSTR;
typedef WINBOOL *PWINBOOL;
typedef BYTE *PBYTE;
typedef const CHAR *PCCH;
typedef const char *PCSTR;
typedef const unsigned short *PCWCH;
typedef DWORD *PDWORD;
typedef float *PFLOAT;
/* typedef PHKEY; */
typedef int *PINT;
/* typedef LCID *PLCID; */
typedef short *PSHORT;
/* typedef PSID; */
typedef char *PSTR;
typedef char *PSZ;

#ifdef UNICODE
typedef unsigned short *PTBYTE;
typedef unsigned short *PTCH;
typedef unsigned short *PTCHAR;
typedef unsigned short *PTSTR;
#else
typedef unsigned char *PTBYTE;
typedef char *PTCH;
typedef char *PTCHAR;
typedef char *PTSTR;
#endif /* UNICODE */

typedef unsigned short *PWCH;
typedef unsigned short *PWORD;
/*
typedef PWSTR;
typedef REGSAM;
*/

typedef short RETCODE;

typedef HANDLE SC_HANDLE;
typedef LPVOID  SC_LOCK;
typedef SC_HANDLE *LPSC_HANDLE;
typedef DWORD SERVICE_STATUS_HANDLE;
/* typedef SPHANDLE; */

#ifdef UNICODE
typedef unsigned short TBYTE;
typedef unsigned short TCHAR;
typedef unsigned short BCHAR;
#else
typedef unsigned char TBYTE;
typedef char TCHAR;
typedef BYTE BCHAR;
#endif /* UNICODE */

typedef unsigned int WPARAM;
/* typedef YIELDPROC; */

/* Only use __stdcall under WIN32 compiler */

#define _export

/*
  Enumerations
*/
typedef enum _ACL_INFORMATION_CLASS {
  AclRevisionInformation = 1,   
  AclSizeInformation            
} ACL_INFORMATION_CLASS; 
 
 
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
 
typedef enum _TOKEN_INFORMATION_CLASS {
    TokenUser = 1, 
    TokenGroups, 
    TokenPrivileges, 
    TokenOwner, 
    TokenPrimaryGroup, 
    TokenDefaultDacl, 
    TokenSource, 
    TokenType, 
    TokenImpersonationLevel, 
    TokenStatistics 
} TOKEN_INFORMATION_CLASS; 
  
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
#define LOBYTE(w)   ((BYTE) (w)) 
#define LOWORD(l)   ((WORD) (l)) 
#define MAKELONG(a, b) ((LONG) (((WORD) (a)) | ((DWORD) ((WORD) (b))) << 16)) 
#define MAKEWORD(a, b) ((WORD) (((BYTE) (a)) | ((WORD) ((BYTE) (b))) << 8)) 

/* original Cygnus headers also had the following defined: */
#define SEXT_HIWORD(l)     ((((int)l) >> 16))
#define ZEXT_HIWORD(l)     ((((unsigned int)l) >> 16))
#define SEXT_LOWORD(l)     ((int)(short)l)

#define INDEXTOOVERLAYMASK(i) ((i) << 8) 
#define INDEXTOSTATEIMAGEMASK(i) ((i) << 12) 

#define MAKEINTATOM(i)   (LPTSTR) ((DWORD) ((WORD) (i))) 
#define MAKEINTRESOURCE(i)  (LPTSTR) ((DWORD) ((WORD) (i)))

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

#ifdef UNICODE
#define TEXT(quote) L##quote 
#else
#define TEXT(quote) quote
#endif

#ifndef RC_INVOKED

/*
   Definitions for callback procedures
*/
typedef int CALLBACK (*BFFCALLBACK) (HWND, UINT, LPARAM, LPARAM);
typedef UINT CALLBACK (*LPCCHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef UINT CALLBACK (*LPCFHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef DWORD CALLBACK (*EDITSTREAMCALLBACK) (DWORD, LPBYTE, LONG, LONG);
typedef UINT CALLBACK (*LPFRHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef UINT CALLBACK (*LPOFNHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef UINT CALLBACK (*LPPRINTHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef UINT CALLBACK (*LPSETUPHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef WINBOOL CALLBACK (*DLGPROC) (HWND, UINT, WPARAM, LPARAM);
typedef int CALLBACK (*PFNPROPSHEETCALLBACK) (HWND, UINT, LPARAM);
typedef VOID CALLBACK (*LPSERVICE_MAIN_FUNCTION) (DWORD, LPTSTR);
typedef int CALLBACK (*PFNTVCOMPARE) (LPARAM, LPARAM, LPARAM);
typedef LRESULT CALLBACK (*WNDPROC) (HWND, UINT, WPARAM, LPARAM);
typedef int CALLBACK (*FARPROC)(void);
typedef FARPROC PROC;
typedef WINBOOL CALLBACK (*ENUMRESTYPEPROC) (HANDLE, LPTSTR, LONG);
typedef WINBOOL CALLBACK (*ENUMRESNAMEPROC) (HANDLE, LPCTSTR, LPTSTR, LONG);
typedef WINBOOL CALLBACK (*ENUMRESLANGPROC) (HANDLE, LPCTSTR, LPCTSTR, WORD, LONG);
typedef FARPROC DESKTOPENUMPROC;
typedef WINBOOL CALLBACK (*ENUMWINDOWSPROC) (HWND, LPARAM);
typedef WINBOOL CALLBACK (*ENUMWINDOWSTATIONPROC) (LPTSTR, LPARAM);
typedef VOID CALLBACK (*SENDASYNCPROC) (HWND, UINT, DWORD, LRESULT);
typedef VOID CALLBACK (*TIMERPROC) (HWND, UINT, UINT, DWORD);
typedef FARPROC GRAYSTRINGPROC;
typedef WINBOOL CALLBACK (*DRAWSTATEPROC) (HDC, LPARAM, WPARAM, int, int);
typedef WINBOOL CALLBACK (*PROPENUMPROCEX) (HWND, LPCTSTR, HANDLE, DWORD);
typedef WINBOOL CALLBACK (*PROPENUMPROC) (HWND, LPCTSTR, HANDLE);
typedef LRESULT CALLBACK (*HOOKPROC) (int, WPARAM, LPARAM);
typedef VOID CALLBACK (*ENUMOBJECTSPROC) (LPVOID, LPARAM);
typedef VOID CALLBACK (*LINEDDAPROC) (int, int, LPARAM);
typedef WINBOOL CALLBACK (*ABORTPROC) (HDC, int);
typedef UINT CALLBACK (*LPPAGEPAINTHOOK) (HWND, UINT, WPARAM, LPARAM );
typedef UINT CALLBACK (*LPPAGESETUPHOOK) (HWND, UINT, WPARAM, LPARAM );
typedef int CALLBACK (*ICMENUMPROC) (LPTSTR, LPARAM);
typedef LONG (*EDITWORDBREAKPROCEX) (char *, LONG, BYTE, INT);
typedef int CALLBACK (*PFNLVCOMPARE) (LPARAM, LPARAM, LPARAM);
typedef WINBOOL CALLBACK (*LOCALE_ENUMPROC) (LPTSTR);
typedef WINBOOL CALLBACK (*CODEPAGE_ENUMPROC) (LPTSTR);
typedef WINBOOL CALLBACK (*DATEFMT_ENUMPROC) (LPTSTR);
typedef WINBOOL CALLBACK (*TIMEFMT_ENUMPROC) (LPTSTR);
typedef WINBOOL CALLBACK (*CALINFO_ENUMPROC) (LPTSTR);
typedef WINBOOL CALLBACK (*PHANDLER_ROUTINE) (DWORD);
typedef WINBOOL CALLBACK (*LPHANDLER_FUNCTION) (DWORD);
typedef UINT CALLBACK (*PFNGETPROFILEPATH) (LPCTSTR, LPSTR, UINT);
typedef UINT CALLBACK (*PFNRECONCILEPROFILE) (LPCTSTR, LPCTSTR, DWORD);
typedef WINBOOL CALLBACK (*PFNPROCESSPOLICIES) (HWND, LPCTSTR, LPCTSTR, LPCTSTR, DWORD);

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

#define SERVICES_ACTIVE_DATABASEW      L"ServicesActive"
#define SERVICES_FAILED_DATABASEW      L"ServicesFailed"
#define SERVICES_ACTIVE_DATABASEA      "ServicesActive"
#define SERVICES_FAILED_DATABASEA      "ServicesFailed"
#define SC_GROUP_IDENTIFIERW           L'+'
#define SC_GROUP_IDENTIFIERA           '+'

#ifdef UNICODE
#define SERVICES_ACTIVE_DATABASE       SERVICES_ACTIVE_DATABASEW
#define SERVICES_FAILED_DATABASE       SERVICES_FAILED_DATABASEW
#define SC_GROUP_IDENTIFIER            SC_GROUP_IDENTIFIERW
#else 
#define SERVICES_ACTIVE_DATABASE       SERVICES_ACTIVE_DATABASEA
#define SERVICES_FAILED_DATABASE       SERVICES_FAILED_DATABASEA
#define SC_GROUP_IDENTIFIER            SC_GROUP_IDENTIFIERA
#endif /* UNICODE */

/* ---------------------------------- */
/* From ddeml.h in old Cygnus headers */

typedef void (*CALLB) (void);
typedef CALLB PFNCALLBACK;

typedef WINBOOL SECURITY_CONTEXT_TRACKING_MODE;

/* End of stuff from ddeml.h in old Cygnus headers */
/* ----------------------------------------------- */

typedef FARPROC WNDENUMPROC;
typedef FARPROC ENHMFENUMPROC;
typedef DWORD CCSTYLE, *PCCSTYLE, *LPCCSTYLE;
typedef DWORD CCSTYLEFLAGA, *PCCSTYLEFLAGA, *LPCCSTYLEFLAGA;
#define DECLARE_HANDLE(s) typedef HANDLE s

#endif /* ! defined (RC_INVOKED) */

#endif /* WIN32_LEAN_AND_MEAN */
   
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GNU_H_WINDOWS32_BASE */
