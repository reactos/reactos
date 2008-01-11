#ifndef __WIN32K_MISC_H
#define __WIN32K_MISC_H

/* W32PROCESS flags */
#define W32PF_NOWINDOWGHOSTING	(0x0001)
#define W32PF_MANUALGUICHECK	(0x0002)
#define W32PF_CREATEDWINORDC	(0x0004)

ULONG FASTCALL IntSystemParametersInfo(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);

#endif /* __WIN32K_MISC_H */
