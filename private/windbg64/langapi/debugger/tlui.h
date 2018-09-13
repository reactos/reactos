#ifndef __TLUI_H__
#define __TLUI_H__

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

// Function prototype for the help callback function
typedef VOID (PASCAL * LPFNINVOKEHELP)(DWORD dwHelpId);

// Transport Layer UI Structure
typedef struct _TLUI
{
	HWND hwnd;
	HINSTANCE hmod;
	LPFNINVOKEHELP lpfnInvokeHelp;	// should be NULL if no help support available.
} TLUI;

// Transport Layer Command Line Structure
typedef struct _TLCL
{
	HWND hwnd;	// should be NULL to specify command line
	LPSTR *argv;
	LONG argc;
} TLCL;

// This declaration comes from afxpriv.h, the value for WM_COMMANDHELP should match the
// one in that file. We also need these values in some of the C only transport dlls
// so I am duplicating it here.

#ifndef __AFXPRIV_H__
#define WM_COMMANDHELP  0x365
#endif

#define IDD_TL_WIN32_SERIAL			0x0f80
#define IDD_TL_MAC_SERIAL			0x0f81
#define IDD_TL_PMAC_SERIAL			0x0f82

//#define IDD_TL_WIN32_ADSP			0x0f83
#define IDD_TL_MAC_ADSP				0x0f84
#define IDD_TL_PMAC_ADSP			0x0f85

#define IDD_TL_WIN32_TCPIP			0x0f86
#define IDD_TL_MAC_TCPIP			0x0f87
#define IDD_TL_PMAC_TCPIP			0x0f88

#define IDD_EM_MAC_SETUP			0x0F91

#endif // __TLUI_H__
