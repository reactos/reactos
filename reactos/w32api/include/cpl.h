#ifndef _CPL_H
#define _CPL_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define WM_CPL_LAUNCH (WM_USER+1000)
#define WM_CPL_LAUNCHED (WM_USER+1001)
#define CPL_DYNAMIC_RES 0
#define CPL_INIT 1
#define CPL_GETCOUNT 2
#define CPL_INQUIRE 3
#define CPL_SELECT 4
#define CPL_DBLCLK 5
#define CPL_STOP 6
#define CPL_EXIT 7
#define CPL_NEWINQUIRE 8
#define CPL_STARTWPARMSA 9
#define CPL_STARTWPARMSW 10
#define CPL_SETUP 200
typedef LONG(APIENTRY *APPLET_PROC)(HWND,UINT,LONG,LONG);
typedef struct tagCPLINFO {
	int idIcon;
	int idName;
	int idInfo;
	LONG lData;
} CPLINFO,*LPCPLINFO;
typedef struct tagNEWCPLINFOA {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwHelpContext;
	LONG lData;
	HICON hIcon;
	CHAR szName[32];
	CHAR szInfo[64];
	CHAR szHelpFile[128];
} NEWCPLINFOA,*LPNEWCPLINFOA;
typedef struct tagNEWCPLINFOW {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwHelpContext;
	LONG lData;
	HICON hIcon;
	WCHAR szName[32];
	WCHAR szInfo[64];
	WCHAR szHelpFile[128];
} NEWCPLINFOW,*LPNEWCPLINFOW;
#ifdef UNICODE
#define CPL_STARTWPARMS CPL_STARTWPARMSW
typedef NEWCPLINFOW NEWCPLINFO,*LPNEWCPLINFO;
#else
#define CPL_STARTWPARMS CPL_STARTWPARMSA
typedef NEWCPLINFOA NEWCPLINFO,*LPNEWCPLINFO;
#endif
#ifdef __cplusplus
}
#endif
#endif
