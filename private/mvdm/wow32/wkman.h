/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WKMAN.H
 *  WOW32 16-bit Kernel API support (manually-coded thunks)
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
 *  30-Apr-91 mattfe added WK32CheckLoadModuleDrv
 *  26-Aug-91 mattfe added FileIO routines
 *  19-JAN-92 mattfe added getnextvdm routine
 *   4-MAR-92 mattfe added KillProcess
 *  12-mar-92 mattfe added w32notifythread
 *   5-may-92 mattfe added HungAppSupport
--*/

ULONG FASTCALL   WK32DirectedYield(PVDMFRAME pFrame);
ULONG FASTCALL   WK32InitTask(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WOWKernelTrace(PVDMFRAME pFrame);
ULONG FASTCALL   WK32ExitKernel(PVDMFRAME pFrame);
ULONG FASTCALL   WK32FatalExit(PVDMFRAME pFrame);
ULONG FASTCALL   WK32KillRemoteTask(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WOWKillTask(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WOWLoadModule32(PVDMFRAME pFrame);
ULONG FASTCALL   WK32RegisterShellWindowHandle(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WOWInitTask(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WOWNotifyWOW32(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WOWOutputDebugString(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WOWQueryPerformanceCounter(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WaitEvent(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WowCloseComPort(PVDMFRAME pFrame);
DWORD FASTCALL   WK32WowDelFile(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WowFailedExec(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WowFailedExec(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WowGetNextVdmCommand (PVDMFRAME pFrame);
ULONG FASTCALL   WK32WowIsKnownDLL(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WowSetIdleHook(PVDMFRAME pFrame);
ULONG FASTCALL   WK32Yield(PVDMFRAME pFrame);
ULONG FASTCALL   WK32OldYield(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WowWaitForMsgAndEvent(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WowMsgBox(PVDMFRAME pFrame);
ULONG FASTCALL   WK32DosWowInit(PVDMFRAME pFrame);
ULONG FASTCALL   WK32CheckUserGdi(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WowPartyByNumber(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WowGetModuleHandle(PVDMFRAME pFrame);
ULONG FASTCALL   WK32FindAndReleaseDib(PVDMFRAME pvf); /* in wdib.c */
ULONG FASTCALL   WK32WowReserveHtask(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WOWLFNEntry(PVDMFRAME pFrame); /* in wkman.c */
ULONG FASTCALL   WK32WowShutdownTimer(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WowTrimWorkingSet(PVDMFRAME pFrame);
ULONG FASTCALL   WK32SetAppCompatFlags(PVDMFRAME pFrame);



BOOL    WK32InitializeHungAppSupport(VOID);
DWORD   W32HungAppNotifyThread(UINT hKillUniqueID);
DWORD   W32RemoteThread(VOID);
DWORD   W32Thread(LPVOID vpInitialSSSP);
VOID    W32DestroyTask( PTD ptd);
VOID    W32EndTask(VOID);
ULONG   W32GetAppCompatFlags(HTASK16 hTask16);
ULONG   W32ReadWOWCompatFlags(HTASK16 htask16, DWORD *pdwWOWXCompatFlagsEx);
#ifdef FE_SB
ULONG   W32ReadWOWCompatFlags2(HTASK16 htask16);
#endif // FE_SB
VOID    WK32DeleteTask(PTD ptdDelete);
VOID    WK32InitWowIsKnownDLL(HANDLE hKeyWow);
LRESULT CALLBACK WK32ForegroundIdleHook(int code, WPARAM wParam, LPARAM lParam);
VOID    W32RefreshCurrentDirectories (PCHAR lpszzEnv);
BOOL FASTCALL WowGetProductNameVersion(PSZ pszExePath, PSZ pszProductName,
                                       DWORD cbProductName, PSZ pszProductVersion,
                                       DWORD cbProductVersion,
                                       PSZ pszParamName, PSZ pszParamValue,
                                       DWORD cbParamValue);
BOOL FASTCALL WowDoNameVersionMatch(PSZ pszExePath, PSZ pszProductName,
                                    PSZ pszProductVersion);

VOID W32InitWOWSetupNames(VOID);
BOOL W32IsSetupProgram(PSZ pszModName, PSZ pszFilePath);

// SoftPC Routines
HANDLE  RegisterWOWIdle(void);
VOID BlockWOWIdle(BOOL Blocking);

// User32 Routines
VOID    ShowStartGlass (DWORD GLASSTIME);

typedef struct _HMODCACHE {         /* hmodcache */
    HAND16  hInst16;
    HAND16  hMod16;
} HMODCACHE, *PHMODCACHE;

extern HMODCACHE ghModCache[];
#define CHMODCACHE      4       // size of cache table

VOID RemoveHmodFromCache(HAND16 hmod16);

typedef struct _CMDSHOW {           /* cmdshow */
    WORD    nTwo;
    WORD    nCmdShow;
} CMDSHOW, *PCMDSHOW;

typedef struct _LOAD_MODULE_PARAMS {        /* loadmoduleparms32 */
    LPVOID lpEnvAddress;
    LPSTR lpCmdLine;
    PCMDSHOW lpCmdShow;
    DWORD dwReserved;
} LOAD_MODULE_PARAMS, *PLOAD_MODULE_PARAMS;

typedef struct _WINOLDAP_THREAD_PARAMS {
    HANDLE hProcess;
    HWND   hwndWinOldAp;
} WINOLDAP_THREAD_PARAMS, *PWINOLDAP_THREAD_PARAMS;

DWORD W32WinOldApThread(PWINOLDAP_THREAD_PARAMS pParams);

// Globals

extern INT busycount;               // Used to detect if WOW is hung
extern HAND16 gKillTaskID;      // 16 bit tdb of task to kill
extern HAND16 ghShellTDB;       // TDB of WOWEXEC
extern HWND ghwndShell;       // Needed for ExitWindowsExec

#define CMS_WAITWOWEXECTIMEOUT 60*1000     // Wait for WOWEXEC to respond
#define CMS_WAITTASKEXIT       5*1000     // Hung App Wait TimeOut
#define CMS_FOREVER            0xffffffff  // wait for ever
#define ALL_TASKS              0xffffffff  // for exitvdm

// IRQ:     Interrupt: ICA: Line: Description:
// -------------------------------------------------------------------
// IRQ1     0x09       0    1     Keyboard service required.
#define KEYBOARD_LINE         1
#define KEYBOARD_ICA          0

extern HANDLE  hSharedTaskMemory;
extern DWORD   dwSharedProcessOffset;
extern VPVOID  vpDebugWOW;
extern VPVOID  vptopPDB;

VOID CleanseSharedList( VOID );
VOID AddProcessSharedList( VOID );
VOID RemoveProcessSharedList( VOID );
WORD AddTaskSharedList( HTASK16, HAND16, PSZ, PSZ );
VOID RemoveTaskSharedList( VOID );
