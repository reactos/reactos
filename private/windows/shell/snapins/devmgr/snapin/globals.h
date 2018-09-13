/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    globals.h

Abstract:

    header file for globals.cpp

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#ifndef __GLOBALS_H_
#define __GLOBALS_H_


class CMachineList;
class String;
class CMemoryException;

extern CMachineList g_MachineList;
extern HINSTANCE g_hInstance;
extern String       g_strStartupMachineName;
extern String       g_strStartupDeviceId;
extern String       g_strStartupCommand;
extern String       g_strDevMgr;
extern BOOL     g_HasLoadDriverNamePrivilege;
extern CPrintDialog g_PrintDlg;

extern CMemoryException g_MemoryException;

extern const CLSID CLSID_DEVMGR;
extern const CLSID CLSID_DEVMGR_EXTENSION;
extern const CLSID CLSID_SYSTOOLS;
extern const CLSID CLSID_DEVMGR_ABOUT;

extern const IID IID_IDMTVOCX;
extern const IID IID_ISnapinCallback;

extern const TCHAR*  const CLSID_STRING_DEVMGR;
extern const TCHAR*  const CLSID_STRING_DEVMGR_EXTENSION;
extern const TCHAR*  const CLSID_STRING_SYSTOOLS;
extern const TCHAR*  const CLSID_STRING_DEVMGR_ABOUT;

extern const TCHAR*  const MMC_SNAPIN_MACHINE_NAME;
extern const TCHAR*  const SNAPIN_INTERNAL;
extern const TCHAR*  const DEVMGR_SNAPIN_CLASS_GUID;
extern const TCHAR*  const DEVMGR_SNAPIN_DEVICE_ID;
extern const TCHAR*  const REG_PATH_DEVICE_MANAGER;
extern const TCHAR*  const REG_VAL_DEVMGR_LOGGING_MASK;
extern const TCHAR*  const REG_STR_BUS_TYPES;
extern const TCHAR*  const REG_STR_TROUBLESHOOTERS;
extern const TCHAR*  const DEVMGR_COMMAND_PROPERTY;
extern const TCHAR*  const DEVMGR_HELP_FILE_NAME;

extern const TCHAR*  const PROGID_DEVMGR;
extern const TCHAR*  const PROGID_DEVMGREXT;
extern const TCHAR*  const PROGID_DEVMGR_ABOUT;
extern const TCHAR*  const DEVMGR_HTML_HELP_FILE_NAME;
extern const TCHAR*  const ENV_NAME_SYSTEMDRIVE;
#if DEVL
extern const TCHAR*  const REG_VAL_DEVMGR_DEBUGBREAKOPTIONS;
extern  DWORD g_DebugBreakOptions;
#define DEBUG_OPTIONS_BREAKON_ENABLEDISABLE     0x00000001
#define DEBUG_OPTIONS_BREAKON_INITDRIVER        0x00000002
#define DEBUG_OPTIONS_BREAKON_REMOVEDEVICE      0x00000004
#define DEBUG_OPTIONS_BREAKON_INITMACHINE       0x00000100
#define DEBUG_OPTIONS_BREAKON_CREATETREEBYTYPE      0x00001000
#define DEBUG_OPTIONS_BREAKON_CREATETREEBYCONN      0x00002000
#define DEBUG_OPTIONS_BREAKON_CREATERESTREE     0x00004000
#define DEBUG_OPTIONS_BREAKON_SHOWDEVTREE       0x00008000
#define DEBUG_OPTIONS_BREAKON_SHOWRESTREE       0x00010000
#define DEBUG_OPTIONS_BREAKON_SHOWRESLIST       0x00020000
#define DEBUG_OPTIONS_BREAKON_PRINTALL          0x00100000
#define DEBUG_OPTIONS_BREAKON_PRINTSUMMARY      0x00200000
#define DEBUG_OPTIONS_BREAKON_PRINTCLASSDEVICE      0x00400000
#define DEBUG_OPTIONS_BREAKON_CHECKDRIVERS      0x01000000
#define DEBUGBREAK_ON(mask) if (g_DebugBreakOptions & mask) DebugBreak()
#else
#define DEBUGBREAK_ON(mask)
#endif

extern LPOLESTR AllocOleTaskString(LPCTSTR str);
extern void FreeOleTaskString(LPOLESTR str);

extern  const NODEINFO NodeInfo[];

//
// global functions declaration
//
extern HRESULT AddMenuItem(LPCONTEXTMENUCALLBACK pCallback, int iNameStringId,
               int iStatusBarStringId, long lCommandId,
               long InsertionPointId, long  Flags,
               long SpecialFlags = 0);
extern CONFIGRET VerifyMachineName(LPCTSTR MachineName);
extern UINT LoadResourceString(int StringId, LPTSTR Buffer, UINT BufferSize);
extern UINT GetDeviceProblemText(HMACHINE hMachine, DEVNODE DevNode,  ULONG ProblemNumber,  LPTSTR Buffer, UINT BufferSize);
extern int MsgBoxParam(HWND hwnd, int MsgId, int CaptionId, DWORD Type, ...);
extern int MsgBoxWinError(HWND hwndParent, int CaptionId, DWORD Type, DWORD Error = ERROR_SUCCESS);
extern LRESULT dmNotifyWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern void PromptForRestart(HWND hwndParent, DWORD RestartFlags, int ResId = 0);

extern "C" {

// imported from setupapi
extern BOOL
DoesUserHavePrivilege(
    PCTSTR PrivilegeName
    );

}
extern BOOL LoadEnumPropPage32(LPCTSTR RegString, HMODULE* pdll, FARPROC* pProcAddress);
extern BOOL IsBlankChar(TCHAR ch);
extern LPTSTR SkipBlankChars(LPTSTR psz);
extern BOOL GuidFromString(LPCTSTR GuidString, LPGUID pGuid);
extern BOOL GuidToString(LPGUID pGuid, LPTSTR Buffer, DWORD BufferLen);
extern PCTSTR MyGetFileTitle(IN PCTSTR FilePath);
extern BOOL AddToolTips(HWND hDlg, UINT id, LPCTSTR pszText, HWND *phwnd);
extern BOOL AddPropPageCallback(HPROPSHEETPAGE hPage, LPARAM lParam);
extern BOOL EnablePrivilege(PCTSTR PrivilegeName, BOOL Enable);
const int MAX_PROP_PAGES = 64;

#if DBG
extern void Trace(LPCTSTR format, ...);

#define TRACE(text)     Trace text
#define TRACE0(text)    OutputDebugString(text)
#define TRACE1(text, arg1)  Trace(text, arg1)
#define TRACE2(text, arg1, arg2) Trace(text, arg1, arg2)
#define TRACE3(text, arg1, arg2, arg3) Trace(text, arg1, arg2, arg3)
#define TRACE4(text, arg1, arg2, arg3, arg4) Trace(text, arg1, arg2, arg3, arg4)
#else

#define TRACE(text)
#define TRACE0(text)
#define TRACE1(text, arg1)
#define TRACE2(text, arg1, arg2)
#define TRACE3(text, arg1, agr2, agr3)

#endif

#define DI_NEEDPOWERCYCLE   0x400000L

#endif      // __GLOBALS_H_
