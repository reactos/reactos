#ifndef __WIN32K_MISC_H
#define __WIN32K_MISC_H

/* W32PROCESS flags */
#define W32PF_CONSOLEAPPLICATION      0x00000001
#define W32PF_FORCEOFFFEEDBACK        0x00000002
#define W32PF_STARTGLASS              0x00000004
#define W32PF_WOW                     0x00000008
#define W32PF_READSCREENACCESSGRANTED 0x00000010
#define W32PF_INITIALIZED             0x00000020
#define W32PF_APPSTARTING             0x00000040
#define W32PF_WOW64                   0x00000080
#define W32PF_ALLOWFOREGROUNDACTIVATE 0x00000100
#define W32PF_OWNDCCLEANUP            0x00000200
#define W32PF_SHOWSTARTGLASSCALLED    0x00000400
#define W32PF_FORCEBACKGROUNDPRIORITY 0x00000800
#define W32PF_TERMINATED              0x00001000
#define W32PF_CLASSESREGISTERED       0x00002000
#define W32PF_THREADCONNECTED         0x00004000
#define W32PF_PROCESSCONNECTED        0x00008000
#define W32PF_WAKEWOWEXEC             0x00010000
#define W32PF_WAITFORINPUTIDLE        0x00020000
#define W32PF_IOWINSTA                0x00040000
#define W32PF_CONSOLEFOREGROUND       0x00080000
#define W32PF_OLELOADED               0x00100000
#define W32PF_SCREENSAVER             0x00200000
#define W32PF_IDLESCREENSAVER         0x00400000
// ReactOS
#define W32PF_NOWINDOWGHOSTING       (0x01000000)
#define W32PF_MANUALGUICHECK         (0x02000000)
#define W32PF_CREATEDWINORDC         (0x04000000)

typedef struct INTENG_ENTER_LEAVE_TAG
  {
  /* Contents is private to EngEnter/EngLeave */
  SURFOBJ *DestObj;
  SURFOBJ *OutputObj;
  HBITMAP OutputBitmap;
  CLIPOBJ *TrivialClipObj;
  RECTL DestRect;
  BOOL ReadOnly;
  } INTENG_ENTER_LEAVE, *PINTENG_ENTER_LEAVE;

extern BOOL APIENTRY IntEngEnter(PINTENG_ENTER_LEAVE EnterLeave,
                                SURFOBJ *DestObj,
                                RECTL *DestRect,
                                BOOL ReadOnly,
                                POINTL *Translate,
                                SURFOBJ **OutputObj);

extern BOOL APIENTRY IntEngLeave(PINTENG_ENTER_LEAVE EnterLeave);

extern HGDIOBJ StockObjects[];
extern SHORT gusLanguageID;

SHORT FASTCALL IntGdiGetLanguageID();
DWORD APIENTRY IntGetQueueStatus(BOOL ClearChanges);
VOID FASTCALL IntUserManualGuiCheck(LONG Check);
PVOID APIENTRY HackSecureVirtualMemory(IN PVOID,IN SIZE_T,IN ULONG,OUT PVOID *);
VOID APIENTRY HackUnsecureVirtualMemory(IN PVOID);

BOOL
NTAPI
RegReadUserSetting(
    IN PCWSTR pwszKeyName,
    IN PCWSTR pwszValueName,
    IN ULONG ulType,
    OUT PVOID pvData,
    IN ULONG cbDataSize);

BOOL
NTAPI
RegWriteUserSetting(
    IN PCWSTR pwszKeyName,
    IN PCWSTR pwszValueName,
    IN ULONG ulType,
    OUT PVOID pvData,
    IN ULONG cbDataSize);

#endif /* __WIN32K_MISC_H */
