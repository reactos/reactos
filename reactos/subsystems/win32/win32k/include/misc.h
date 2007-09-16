#ifndef __WIN32K_MISC_H
#define __WIN32K_MISC_H

/* W32PROCESS flags */
#define W32PF_NOWINDOWGHOSTING	(0x0001)
#define W32PF_MANUALGUICHECK	(0x0002)
#define W32PF_CREATEDWINORDC	(0x0004)

ULONG FASTCALL IntSystemParametersInfo(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);

/* Window Client Information structure */
typedef struct _W32CLTINFO_TEB
{
    ULONG Win32ClientInfo0[2];
    ULONG ulWindowsVersion;
    ULONG ulAppCompatFlags;
    ULONG ulAppCompatFlags2;
    ULONG Win32ClientInfo1[5];
    HWND  hWND;
    PVOID pvWND;
    ULONG Win32ClientInfo2[50];
} W32CLTINFO_TEB, *PW32CLTINFO_TEB;

#define GetWin32ClientInfo() (PW32CLTINFO_TEB)(NtCurrentTeb()->Win32ClientInfo)

#endif /* __WIN32K_MISC_H */
