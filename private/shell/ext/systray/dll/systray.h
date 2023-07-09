#include "stresid.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)   (sizeof((x))/sizeof((x)[0]))
#endif


#define STWM_NOTIFYHOTPLUG  STWM_NOTIFYPCMCIA
#define STSERVICE_HOTPLUG   STSERVICE_PCMCIA
#define HOTPLUG_REGFLAG_NOWARN PCMCIA_REGFLAG_NOWARN

void SysTray_RunProperties(UINT RunStringID);

VOID
PASCAL
SysTray_NotifyIcon(
    HWND hWnd,
    UINT uCallbackMessage,
    DWORD Message,
    HICON hIcon,
    LPCTSTR lpTip
    );

LPTSTR
NEAR CDECL
LoadDynamicString(
    UINT StringID,
    ...
    );

UINT EnableService(UINT uNewSvcMask, BOOL fEnable);
BOOL PASCAL GenericGetSet(HKEY hKey, LPCTSTR pszValue, LPVOID pData,
                          ULONG  cbSize, BOOL   bSet);

VOID
PASCAL
SysTray_AppendMenuString(
    HMENU hmenu,
    UINT item,
    LPTSTR lpszMenuItem
    );

//  Wrapper for LocalFree to make the code a little easier to read.
#define DeleteDynamicString(x)          LocalFree((HLOCAL) (x))

#define HOTPLUG_TIMER_ID                2
#define VOLUME_TIMER_ID                 3
#define POWER_TIMER_ID                  4
#define HOTPLUG_DEVICECHANGE_TIMERID    5
#define USBUI_TIMER_ID                  6
#define FAX_STARTUP_TIMER_ID            7


void    Power_Timer(HWND hWnd);
BOOL    Power_CheckEnable(HWND hWnd, BOOL bSvcEnabled);
void    Power_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam);
void    Power_OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
void    Power_OnPowerBroadcast(HWND hWnd, WPARAM wParam, LPARAM lParam);
void    Power_OnDeviceChange(HWND hWnd, WPARAM wParam, LPARAM lParam);
void    Power_WmDestroy(HWND hWnd);
BOOLEAN Power_OnActivate(HWND hWnd, WPARAM wParam, LPARAM lParam);
void    Update_PowerFlags(DWORD dwMask, BOOL bEnable);
DWORD   Get_PowerFlags(void);
VOID    PASCAL Power_UpdateStatus(HWND, DWORD, BOOL);

void CloseIfOpen(LPHANDLE);

BOOL Volume_Init(HWND hWnd);
BOOL Volume_CheckEnable(HWND hWnd, BOOL bEnabled);
void Volume_DeviceChange(HWND hWnd, WPARAM wParam, LPARAM lParam);
void Volume_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam);
void Volume_Timer(HWND hWnd);
void Volume_LineChange(HWND hWnd, HMIXER hmx, DWORD dwID );
void Volume_ControlChange(HWND hWnd, HMIXER hmx, DWORD dwID );
void Volume_Shutdown(HWND hWnd);
void Volume_WinMMDeviceChange(HWND hWnd);
void Volume_HandlePowerBroadcast(HWND hWnd, WPARAM wParam, LPARAM lParam);
void Volume_DeviceChange_Cleanup(void);
void Volume_WmDestroy(HWND hWnd);

BOOL HotPlug_CheckEnable(HWND hWnd, BOOL bEnabled);
void HotPlug_DeviceChange(HWND hWnd, WPARAM wParam, LPARAM lParam);
void HotPlug_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam);
void HotPlug_Timer(HWND hWnd);
int  HotPlug_DeviceChangeTimer(HWND hWnd);
void HotPlug_WmDestroy(HWND HWnd);

BOOL StickyKeys_CheckEnable(HWND hWnd);
void StickyKeys_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam);

BOOL MouseKeys_CheckEnable(HWND hWnd);
void MouseKeys_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam);

BOOL CSC_CheckEnable(HWND hWnd, BOOL bSvcEnabled);
BOOL CSC_MsgProcess(LPMSG pMsg);

void USBUI_Notify(HWND hwnd, WPARAM wParam, LPARAM lParam);
//HMENU USBUI_CreateMenu();
void USBUI_Menu(HWND hwnd, UINT uMenuNum, UINT uButton);
BOOL USBUI_Init(HWND hWnd);
void USBUI_UpdateStatus(HWND hWnd, BOOL bShowIcon);
BOOL USBUI_CheckEnable(HWND hWnd, BOOL bSvcEnabled);
void USBUI_Toggle();
BOOL USBUI_SetState(BOOL On);
void USBUI_Timer(HWND hwnd);

BOOL FilterKeys_CheckEnable(HWND hWnd);
void FilterKeys_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam);

VOID Fax_StartupTimer(HWND hWnd);
BOOL Fax_CheckEnable(HWND hWnd);
BOOL Fax_MsgProcess(LPMSG Msg);
HANDLE Fax_EventSignal(DWORD);

VOID
SetIconFocus(
    HWND hwnd,
    UINT uiIcon
    );
