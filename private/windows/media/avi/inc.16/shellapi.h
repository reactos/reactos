/*****************************************************************************\
*                                                                             *
* shellapi.h -  SHELL.DLL functions, types, and definitions                   *
*                                                                             *
* Copyright (c) 1992-1994, Microsoft Corp.	All rights reserved	      *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_SHELLAPI
#define _INC_SHELLAPI

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

DECLARE_HANDLE(HDROP);

#ifdef WIN32
#ifdef UNICODE
#define ShellExecute ShellExecuteW
#define FindExecutable FindExecutableW
#define ShellAbout ShellAboutW
#define ExtractAssociatedIcon ExtractAssociatedIconW
#define ExtractIcon ExtractIconW
#define DragQueryFile DragQueryFileW
#define InternalExtractIcon InternalExtractIconW
#define DoEnvironmentSubst DoEnvironmentSubstW
#define FindEnvironmentString FindEnvironmentStringW
#else
#define ShellExecute ShellExecuteA
#define FindExecutable FindExecutableA
#define ShellAbout ShellAboutA
#define ExtractAssociatedIcon ExtractAssociatedIconA
#define ExtractIcon ExtractIconA
#define DragQueryFile DragQueryFileA
#define InternalExtractIcon InternalExtractIconA
#define DoEnvironmentSubst DoEnvironmentSubstA
#define FindEnvironmentString FindEnvironmentStringA
#endif  // UNICODE
#endif  // WIN32

UINT  WINAPI DragQueryFile(HDROP, UINT, LPSTR, UINT);
BOOL  WINAPI DragQueryPoint(HDROP, POINT FAR*);
void  WINAPI DragFinish(HDROP);
void  WINAPI DragAcceptFiles(HWND, BOOL);

#ifdef WIN32

typedef struct _DRAGINFO {
    UINT  uSize;			/* init with sizeof(DRAGINFO) */
    POINT pt;
    BOOL  fNC;
    LPSTR lpFileList;
    DWORD grfKeyState;
} DRAGINFO, FAR* LPDRAGINFO;

BOOL WINAPI DragQueryInfo(HDROP, LPDRAGINFO);	/* get extra info about a drop */



// AppBar stuff
#define ABM_NEW           0x00000000
#define ABM_REMOVE        0x00000001
#define ABM_QUERYPOS      0x00000002
#define ABM_SETPOS        0x00000003
#define ABM_GETSTATE      0x00000004
#define ABM_GETTASKBARPOS 0x00000005

// these are put in the wparam of callback messages 
#define ABN_STATECHANGE    0x0000000
#define ABN_POSCHANGED     0x0000001
#define ABN_FULLSCREENAPP  0x0000002
#define ABN_WINDOWARRANGE  0x0000003 // lParam == TRUE means hide

// flags for get state
#define ABS_AUTOHIDE    0x0000001
#define ABS_ALWAYSONTOP 0x0000002

#define ABE_LEFT        0
#define ABE_TOP         1
#define ABE_RIGHT       2
#define ABE_BOTTOM      3

typedef struct _AppBarData
{
    DWORD cbSize;
    HWND hWnd;
    UINT uCallbackMessage;
    UINT uEdge;
    RECT rc;
} APPBARDATA, *PAPPBARDATA;

UINT WINAPI SHAppBarMessage(DWORD dwMessage, PAPPBARDATA pData);
    

#endif


HICON WINAPI ExtractIcon(HINSTANCE hInst, LPCSTR lpszFile, UINT nIconIndex);


/* ShellExecute() and ShellExecuteEx() error codes */

/* regular WinExec() codes */
#define SE_ERR_FNF              2	// file not found
#define SE_ERR_PNF              3	// path not found
#define SE_ERR_OOM              8	// out of memory

/* values beyond the regular WinExec() codes */
#define SE_ERR_SHARE            26
#define SE_ERR_ASSOCINCOMPLETE  27
#define SE_ERR_DDETIMEOUT       28
#define SE_ERR_DDEFAIL          29
#define SE_ERR_DDEBUSY          30
#define SE_ERR_NOASSOC          31
#define SE_ERR_DLLNOTFOUND      32

HINSTANCE WINAPI FindExecutable(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult);	
HINSTANCE WINAPI ShellExecute(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, int iShowCmd);

int     WINAPI ShellAbout(HWND hWnd, LPCSTR szApp, LPCSTR szOtherStuff, HICON hIcon);
DWORD   WINAPI DoEnvironmentSubst(LPSTR szString, UINT cbString);                                  
LPSTR 	WINAPI FindEnvironmentString(LPSTR szEnvVar);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* RC_INVOKED */

#endif  /* _INC_SHELLAPI */
