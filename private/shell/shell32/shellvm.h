/******************************************************************************
 *
 *  (C) Copyright MICROSOFT Corp., 1988-1990
 *
 *  Title:      SHELL header file
 *
 *  Version:    2.00
 *
 *  Date:       2-06-89
 *
 *  Author:     ARR
 *
 *******************************************************************************
 *
 *  Change log:
 *
 *    DATE    REVISION                  DESCRIPTION
 *  --------  --------  -------------------------------------------------------
 *  06-Feb-1989 ARR     Original
 *  28-Feb-1992 rjc     Partially converted from inc to h file. BUGBUG chg
 *  25-Sep-1992 rjc     Change old-style ULONG/USHORT into modern DWORD/WORD
 *
 *==============================================================================
 */

/*
 * All directed VM hot keys are PriorityNotify.  This is the timeout for them.
 * If it takes longer than this many milliseconds, the key is discarded.
 *
 *   0 means no time out
 */
#define KEYTIMEOUT      4000    /* 4 sec, long enough for floppy operations */

/*
 * This is the structure of the PIF file hot key information.
 *
 * BUGBUG -- Is this MICROSOFT CONFIDENTIAL information? (raymondc, 20-Apr-92)
 *
 *  The bits of Val are as follows:
 *
 *      bit 0 (value 1) = key is an extended key
 *
 *          This tells us whether or not to look for an E0 prefix.
 *
 *      bit 1 (value 2) = key requires NumLock down (new for 4.0)
 *
 *          This tells us whether to treat scan code 047h as
 *          VK_HOME or VK_NUMPAD7.
 */

#define MAXHKYINFOSIZE          16

/* H2INCSWITCHES -t */
typedef struct PIFKEY { /* PIF_Ky */
        WORD    Scan;   /* Scan code in lower byte */
        WORD    ShVal;  /* Shift state */
        WORD    ShMsk;  /* Mask for shift states interested in */
        BYTE    Val;    /* Enhanced key stuff */
        BYTE    Pad[9]; /* Pad PIF struc to MAXHKYINFOSIZE bytes */
} PIFKEY;
typedef UNALIGNED PIFKEY *LPPIFKEY;
typedef const UNALIGNED PIFKEY *LPCPIFKEY;

/* ASM
.errnz  SIZE PIFKEY - MAXHKYINFOSIZE
 */

/*
 * PIF_Ky_Val
 *      = 1, if extended code   (key is an extended code only)
 *      = 0FFh, if either       (key is EITHER extended or not extended)
 *      = 0  if not extended    (key is not extended only)
 *
 *          bit 15 - Ins depressed
 *          bit 14 - Caps Lock depressed
 *          bit 13 - Num Lock depressed
 *          bit 12 - Scroll Lock depressed
 *          bit 11 - hold state active(Ctrl-Num Lock)
 *          bit 10 - 0
 *          bit  9 - 0
 *          bit  8 - 0
 *          bit  7 - Insert state active
 *          bit  6 - Caps Lock state active
 *          bit  5 - Num Lock state active
 *          bit  4 - Scroll Lock state active
 *          bit  3 - Alt shift depressed
 *          bit  2 - Ctrl shift depressed
 *          bit  1 - left shift depressed
 *          bit  0 - right shift depressed
 */
#define fPIFSh_RShf     0x0001          /* Right shift key */
#define fPIFSh_RShfBit  0

#define fPIFSh_LShf     0x0002          /* Left shift key */
#define fPIFSh_LShfBit  1

#define fPIFSh_Ctrl     0x0004          /* Either Control shift key */
#define fPIFSh_CtrlBit  2

#define fPIFSh_Alt      0x0008          /* Either Alt shift key */
#define fPIFSh_AltBit   3

#define fPIFSh_ScLok    0x0010          /* Scroll lock active */
#define fPIFSh_ScLokBit 4

#define fPIFSh_NmLok    0x0020          /* Num lock active */
#define fPIFSh_NmLokBit 5

#define fPIFSh_CpLok    0x0040          /* Caps lock active */
#define fPIFSh_CpLokBit 6

#define fPIFSh_Insrt    0x0080          /* Insert active */
#define fPIFSh_InsrtBit 7

#define fPIFSh_Ext0     0x0400          /* Extended K/B shift */
#define fPIFSh_Ext0Bit  10

#define fPIFSh_Hold     0x0800          /* Ctrl-Num-Lock/Pause active */
#define fPIFSh_HoldBit  11

#define fPIFSh_LAlt     0x1000          /* Left Alt key is down */
#define fPIFSh_LAltBit  12

#define fPIFSh_RAlt     0x2000          /* Right Alt key is down */
#define fPIFSh_RAltBit  13

#define fPIFSh_LCtrl    0x4000          /* Left Ctrl key is down */
#define fPIFSh_LCtrlBit 14

#define fPIFSh_RCtrl    0x8000          /* Right Ctrl key is down */
#define fPIFSh_RCtrlBit 15

#define MAXVMTITLELENGTH        32      /* Size of name buffer */
#define PIFNAMESIZE     30              /* Amount of buffer actually used */

/*
 * VM descriptor structure used to create and modify VM attributes.
 *
 */
/* H2INCSWITCHES -t- */
struct VM_Descriptor {
        DWORD   VD_Flags;       /* Flags                                */
        DWORD   VD_Flags2;      /* More Flags                           */
        DWORD   VD_ProgName;    /* Pointer to program name              */
        WORD    VD_ProgNameSeg; /*                                      */
        DWORD   VD_CmdLine;     /* Command line ptr                     */
        WORD    VD_CmdLineSeg;  /*                                      */
        DWORD   VD_DrivePath;   /* Current drive and dir ptr            */
        WORD    VD_DrivePathSeg;/*                                      */
        WORD    VD_MaxMem;      /* Maximum VM memory in Kb              */
        WORD    VD_MinMem;      /* Minimum VM memory in Kb              */
        WORD    VD_FPriority;   /* Priority of process when FOCUS       */
        WORD    VD_BPriority;   /* Priority of process when Not FOCUS   */
        WORD    VD_MaxEMSMem;   /* Maximum EMS memory in Kb             */
        WORD    VD_MinEMSMem;   /* Minimum EMS memory in Kb             */
        WORD    VD_MaxXMSMem;   /* Maximum XMS (extended) memory in Kb  */
        WORD    VD_MinXMSMem;   /* Minimum XMS (extended) memory in Kb  */
        WORD    VD_WindHand;    /* Window handle of VMDOSAPP instance   */
        WORD    VD_InstHand;    /* Instance handle of VMDOSAPP instance */
        BYTE    VD_Title[MAXVMTITLELENGTH]; /* Title of app             */
        PIFKEY  VD_HotKeyBuf;   /* Buffer for Hot key spec  */
};

/* ASM
;
; Access the MaxMem and MinMem fields as a single DWORD.
;
VD_VMSize       equ     DWORD PTR VD_MaxMem

*/

/* ASM
ifndef MASM6
IF2
    IFDEF VMStat_Exclusive_Bit
    .erre VD_F_ExclusiveBit    EQ VMStat_Exclusive_Bit
    .erre VD_F_BackgroundBit   EQ VMStat_Background_Bit
    ENDIF
ENDIF
else  ; MASM6
    IFDEF VMStat_Exclusive_Bit
    .erre VD_F_ExclusiveBit    EQ VMStat_Exclusive_Bit
    .erre VD_F_BackgroundBit   EQ VMStat_Background_Bit
    ENDIF
endif ; MASM6
*/

/*
 * Masks and bit numbers for VD_Flags
 *
 */
#define VD_F_ExclusiveBit       0       /* This bit indicates that when the VM
                                         * has the focus it has Exclusive use
                                         * of the CPU.
                                         */
#define VD_F_Exclusive          (1L << VD_F_ExclusiveBit)

#define VD_F_BackgroundBit      1       /* This bit indicates that when the VM
                                         * does not have the focus it continues
                                         * to get CPU time. If this bit is
                                         * clear the VM is effectively
                                         * suspended unless it has the
                                         * input focus.
                                         */
#define VD_F_Background         (1L << VD_F_BackgroundBit)

#define VD_F_WindowBit          2       /* This bit indicates that the VM
                                         * runs in a window.
                                         * If the bit is clear the VM is a
                                         * full screen VM.
                                         */
#define VD_F_Window             (1L << VD_F_WindowBit)

/* VD_F_Window2 is set if the VDD thinks that the VM is         ;Internal
 * running in a window.  This bit is only if...                 ;Internal
 *      1.  The VM is running in a window.                      ;Internal
 *      2.  The VM is not minimized.                            ;Internal
 *      3.  The VM's client area is visible.                    ;Internal
 * This is different from VD_F_Window, which is set iff the     ;Internal
 * VM is running in a window, possible minimized or obscured.   ;Internal
 * We lie to the VDD so that it won't try to track the VM's     ;Internal
 * display when there is no reason to do so.                    ;Internal
 */                                                          /* ;Internal */
#define VD_F_Window2Bit         3                            /* ;Internal */
#define VD_F_Window2            (1L << VD_F_Window2Bit)      /* ;Internal */
                                                             /* ;Internal */
#define VD_F_SuspendedBit       4       /* Set if app suspended by VMDOSAPP */
#define VD_F_Suspended          (1L << VD_F_SuspendedBit)

#define VD_F_ALTTABdisBit       5       /* Set if the Standard
                                         * ALT-TAB Hot key is disabled for
                                         * this VM.
                                         */
#define VD_F_ALTTABdis          (1L << VD_F_ALTTABdisBit)

#define VD_F_ALTESCdisBit       6       /* Set if the Standard
                                         * ALT-ESC Hot key is disabled for
                                         * this VM.
                                         */
#define VD_F_ALTESCdis          (1L << VD_F_ALTESCdisBit)

#define VD_F_ALTSPACEdisBit     7       /* Set if the Standard
                                         * ALT-SPACE Hot key is disabled for
                                         * this VM.
                                         */
#define VD_F_ALTSPACEdis        (1L << VD_F_ALTSPACEdisBit)

#define VD_F_ALTENTERdisBit     8       /* Set if the Standard
                                         * ALT-ENTER Hot key is disabled for
                                         * this VM.
                                         */
#define VD_F_ALTENTERdis        (1L << VD_F_ALTENTERdisBit)

#define VD_F_ALTPRTSCdisBit     9       /* Set if the Standard
                                         * ALT-PRTSC Hot key is disabled for
                                         * this VM.
                                         */
#define VD_F_ALTPRTSCdis        (1L << VD_F_ALTPRTSCdisBit)

#define VD_F_PRTSCdisBit        10      /* Set if the Standard
                                         * PRTSC Hot key is disabled for
                                         * this VM.
                                         */
#define VD_F_PRTSCdis           (1L << VD_F_PRTSCdisBit)

#define VD_F_CTRLESCdisBit      11      /* Set if the Standard
                                         * CTRL-ESC Hot key is disabled for
                                         * this VM.
                                         */
#define VD_F_CTRLESCdis         (1L << VD_F_CTRLESCdisBit)

#define VD_F_PollingDetectBit   12      /* Set if the polling
                                         * detection is allowed for this VM
                                         */
#define VD_F_PollingDetect      (1L << VD_F_PollingDetectBit)

#define VD_F_NoHMABit           13      /* Set if access to
                                         * the XMS HMA is disallowed in
                                         * this VM.
                                         */
#define VD_F_NoHMA              (1L << VD_F_NoHMABit)

#define VD_F_HasHotKeyBit       14      /* Set if this app
                                         * has specified a hot key in
                                         * this VM.
                                         */
#define VD_F_HasHotKey          (1L << VD_F_HasHotKeyBit)

#define VD_F_EMS_Is_LockBit     15      /* Set if XMS memory
                                         * should be locked in this VM.
                                         */
#define VD_F_EMS_Is_Lock        (1L << VD_F_EMS_Is_LockBit)

#define VD_F_XMS_Is_LockBit     16      /* Set if EMS memory
                                         * should be locked in this VM.
                                         */
#define VD_F_XMS_Is_Lock        (1L << VD_F_XMS_Is_LockBit)

#define VD_F_INT16PasteBit      17      /* Paste via INT 16 is OK */
#define VD_F_INT16Paste         (1L << VD_F_INT16PasteBit)

#define VD_F_VMLockedBit        18      /* VM is to be Always LOCKED. */
#define VD_F_VMLocked           (1L << VD_F_VMLockedBit)

#define VD_F_IsBatchBit         19      /* VM is a .BAT file run */
#define VD_F_IsBatch            (1L << VD_F_IsBatchBit)

                                                                        /* ;Internal */
#define VD_F_VolumeLockBit      27      /* Owns a level 3 volume lock *//* ;Internal */
#define VD_F_VolumeLock         (1L << VD_F_VolumeLockBit)              /* ;Internal */
                                                                        /* ;Internal */
#define VD_F_IsTextBit          28      /* Am in a text mode */         /* ;Internal */
#define VD_F_IsText             (1L << VD_F_IsTextBit)                  /* ;Internal */
                                                                        /* ;Internal */
#define VD_F_DynaWindowBit      29      /* Auto switch when in graphics mode */
#define VD_F_DynaWindow         (1L << VD_F_DynaWindowBit)

#define VD_F_ExitCloseBit       30      /* VM is to be closed on exit */
#define VD_F_ExitClose          (1L << VD_F_ExitCloseBit)

#define VD_F_PastingBit         31      /* VM is pasting also used for hot key
                                         * flag
                                         */
#define VD_F_Pasting            (1L << VD_F_PastingBit)

/*
 * Following bits are bits for the VDD to interpret in VD_Flags2
 */

#define VD_F2_VDDPrivMask       0x0000FFFF
#define VD_F2_VDDPrivMinBit     0
#define VD_F2_VDDPrivMaxBit     15

/* Validate the above three defines */

#if ((1 << (1+VD_F2_VDDPrivMaxBit))-1) - ((1 << VD_F2_VDDPrivMinBit)-1) != VD_F2_VDDPrivMask
/* XLATOFF */
#error VD_F2_VDDPrivMask conflicts with MaxBit and MinBit.
/* XLATON */
/* ASM
%OUT   VD_F2_VDDPrivMask conflicts with MaxBit and MinBit.
.err
 */
#endif

#define VD_F2_DynaWindowingBit  16      /* Internal semaphore */
#define VD_F2_DynaWindowIng     (1L << VD_F2_DynaWindowingBit)
#define VD_F2_DynaWaitingBit    17      /* Internal semaphore */
#define VD_F2_DynaWaiting       (1L << VD_F2_DynaWaitingBit)

/*
 * These are the bits of VD_Flags which are "exported" to the outside world
 *   via the SHELL_GetVMInfo service
 */
/* XLATOFF */
#define VD_Flags_Exported (VD_F_Window+VD_F_ALTTABdis+VD_F_ALTESCdis+\
                          VD_F_ALTSPACEdis+VD_F_ALTENTERdis+VD_F_ALTPRTSCdis+\
                          VD_F_PRTSCdis+VD_F_CTRLESCdis+VD_F_PollingDetect+\
                          VD_F_NoHMA+VD_F_HasHotKey+VD_F_EMS_Is_Lock+\
                          VD_F_XMS_Is_Lock+VD_F_INT16Paste+VD_F_VMLocked+\
                          VD_F_ExitClose)
/* XLATON */
/* ASM
VD_Flags_Exported   equ   (VD_F_Window+VD_F_ALTTABdis+VD_F_ALTESCdis+\
                          VD_F_ALTSPACEdis+VD_F_ALTENTERdis+VD_F_ALTPRTSCdis+\
                          VD_F_PRTSCdis+VD_F_CTRLESCdis+VD_F_PollingDetect+\
                          VD_F_NoHMA+VD_F_HasHotKey+VD_F_EMS_Is_Lock+\
                          VD_F_XMS_Is_Lock+VD_F_INT16Paste+VD_F_VMLocked+\
                          VD_F_ExitClose)
*/

#ifdef  DOS7                                                            /* ;Internal */
#define VD_Flags2_Exported VD_F2_CreateVisible                          /* ;Internal */
#else                                                                   /* ;Internal */
#define VD_Flags2_Exported 0
#endif                                                                  /* ;Internal */

/*
 * Special exit codes
 *
 *  VMDA_EXIT_ExecFail can trigger for the following reasons:
 *
 *      An ugly TSR is present in the system.
 *      You are in clean-boot mode.
 *      Administration restrictions forbid DOS boxes.
 *                                                                  ;Internal
 *  NOTE!  If you add a new exit code, make sure to adjust the call ;Internal
 *  to WOAAbort in ttywin.asm accordingly!                          ;Internal
 */
#define VMDA_EXIT_NoFile        0x81    /* File not found */
#define VMDA_EXIT_NoMem         0x82    /* Insufficient memory */
#define VMDA_EXIT_Crash         0x83    /* Application terminated abnormally */
#define VMDA_EXIT_BadVer        0x84    /* Mismatched system components */
#define VMDA_EXIT_ExecFail      0x85    /* Could not run due to incompatible
                                         * system configuration.
                                         */

/*
 * These are the ordinals for the Shell-exported V86-mode and protect-mode
 * services.
 *
 */

#define SHSV_Get_Version                0x0100
#define SHSV_Install_New_Task_Manager   0x0101
#define SHSV_ShellExecute               0x0102
#define SHSV_WinExecWait                0x0103
#define SHSV_Enumerate_Properties       0x0104
#define SHSV_Update_Properties          0x0105
#define SHSV_Set_ScreenSaver_Info       0x0106
#define SHSV_Get_VxD_Version            0x0107

#define NUMSHELLSERVICES                8

/* H2INCSWITCHES -t */
typedef struct TaskManagerDescriptorBlock { /* TMDB */
        DWORD   TaskManNameOffs;
        WORD    TaskManNameSeg;
        DWORD   ProgNameOffs;
        WORD    ProgNameSeg;
        DWORD   CmdLineOffs;
        WORD    CmdLineSeg;
        DWORD   DrivePathOffs;
        WORD    DrivePathSeg;
} TMDB;

#define TMDBSTRINGCOUNT           4
#define MAXTMDBSTRINGLENGTH     128     /* 127 chars + terminating 0 */

/*
 * Error values for WinExecWait.
 */
#define ERR_WEW_FIRST           0x0100
#define ERR_WEW_INSMEM          0x0100  /* Insufficient memory */
#define ERR_WEW_NOTASK          0x0101  /* No such task, or task is Win32 */
#define ERR_WEW_TOOLHELP        0x0102  /* Could not load TOOLHELP */
#define ERR_WEW_LAST            0x0102  /* ;Internal */

/*
 * Definitions for the VM Close API.
 */
#define VMCLFL_ENABLECLOSE      1

#ifndef V86MODE

#ifdef  TASKMAN
/*
 * Structure for installable task managers to notify the Shell VxD
 * which services they want to hook.
 */
typedef struct TMHandlers {             /* TMH */
        DWORD   Len;
        PVOID   Resolve_Contention_Pre;
        PVOID   Resolve_Contention_Post;
        PVOID   Event;
        PVOID   Sysmodal_Message_Pre;
        PVOID   Sysmodal_Message_Post;
        PVOID   Message_Pre;
        PVOID   Message_Post;
        PVOID   Svc_Call;
        PVOID   Clipboard;
        PVOID   Not_Executeable;
        PVOID   Set_Focus;
        PVOID   HotKey;
        PVOID   PostMessage;
        PVOID   ShellExecute;
        PVOID   CallDll;
        PVOID   VmTitle;
        PVOID   VmClose;
        PVOID   QueryAppyTimeAvailable;
        PVOID   CallAtAppyTime;
        PVOID   CancelAppyTimeEvent;
        PVOID   BroadcastSystemMessage;
        PVOID   HookSystemBroadcast;
        PVOID   UnhookSystemBroadcast;
        PVOID   LocalAllocEx;
        PVOID   LocalFree;
        PVOID   LoadLibrary;
        PVOID   FreeLibrary;
        PVOID   GetProcAddress;
        PVOID   DispatchRing0AppyEvents;
} TMH;

/*
 * This is the structure the SHELL_Install_TaskMan_Hooks returns a pointer to.
 *
 * It is an array of pointers to helper functions provided by the Shell VxD
 * to task managers.  These helper functions are not exported as services
 * because they expose the sensitive innards of the Shell VxD.
 */
typedef struct SHHandlers {             /* SHH */
        PVOID   GetVmDescriptor;
        PVOID   RegisterVMHotKey;
        PVOID   DisplayMessage;
} SHH;
#endif

/*
 * When expanding this structure, make sure to PREPEND fields, so as to
 * retain backwards compatibility.
 */
typedef struct VM_Desc2 {               /* VD2 */
        DWORD   phvmOwner;              /* Back-pointer to owner */
        BYTE    Err_Code;               /* Error code for Exec failure */
        BYTE    Exit_Code;              /* Exit code */
} VM_Desc2;

/* ASM
;
; Potentially handy abbreviation so you can say
;
;       call    [SHELL_GetVmDescriptor]
;       mov     eax, [eax.MVM_Desc2.VD2_HotKeyHandle]
;
MVM_Desc2       =       - SIZE VM_Desc2

 */


#endif /* ifndef V86MODE */

#define MAX_DLL_NAME    80
#ifndef MAX_GROUP_NAME
#define MAX_GROUP_NAME  16
#else
#if     MAX_GROUP_NAME != 16
#error  Invalid definition of MAX_GROUP_NAME.
#endif
#endif

/* H2INCSWITCHES -t- */
struct PropID {
        DWORD   ordGroup;
        BYTE    achGroup[MAX_GROUP_NAME];
        BYTE    achDLL[MAX_DLL_NAME];
};
