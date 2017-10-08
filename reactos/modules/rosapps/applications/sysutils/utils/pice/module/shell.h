/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    shell.h

Abstract:

    HEADER for shell.c

Environment:

    LINUX 2.2.X
    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/
//void InstallKeyboardHook(void);
//void DeInstallKeyboardHook(void);
void InstallGlobalKeyboardHook(void);
void DeInstallGlobalKeyboardHook(void);

void RealIsr(ULONG dwReasonForBreak);
void NewInt31Handler(void);

extern volatile BOOLEAN bNotifyToExit;
extern volatile BOOLEAN bSingleStep;
extern volatile UCHAR ucKeyPressedWhileIdle;
extern volatile BOOLEAN bInDebuggerShell;

extern ULONG CurrentEIP,CurrentEFL;
extern ULONG CurrentEAX,CurrentEBX,CurrentECX,CurrentEDX;
extern ULONG CurrentESP,CurrentEBP,CurrentESI,CurrentEDI;
extern ULONG CurrentDR0,CurrentDR1,CurrentDR2,CurrentDR3,CurrentDR6,CurrentDR7;
extern ULONG CurrentCR0,CurrentCR2,CurrentCR3;
extern USHORT CurrentCS,CurrentDS,CurrentES,CurrentFS,CurrentGS,CurrentSS;
extern volatile BOOLEAN bControl; // TRUE when CTRL key was pressed
extern volatile BOOLEAN bShift; // TRUE when SHIFT key was pressed
extern volatile BOOLEAN bAlt; // TRUE when SHIFT key was pressed

// previous context
extern ULONG OldEIP,OldEFL;
extern ULONG OldEAX,OldEBX,OldECX,OldEDX;
extern ULONG OldESP,OldEBP,OldESI,OldEDI;
extern USHORT OldCS,OldDS,OldES,OldFS,OldGS,OldSS;

extern ULONG CurrentProcess;

extern USHORT OldSelector;
extern ULONG OldOffset;

extern ULONG ulRealStackPtr; // serves as current process pointer too!!

extern ULONG g_ulLineNumberStart;
extern BOOLEAN bStepThroughSource;
extern BOOLEAN bStepInto;

#define REASON_INT3         (0)
#define REASON_SINGLESTEP   (1)
#define REASON_CTRLF        (2)
#define REASON_PAGEFAULT    (3)
#define REASON_GP_FAULT     (4)
#define REASON_HARDWARE_BP  (5)
#define REASON_DOUBLE_FAULT (6)
#define REASON_MODULE_LOAD  (7)
#define REASON_INTERNAL_ERROR (8)

extern volatile BOOLEAN bEnterNow;

// keyboard controller defines
#define I8042_PHYSICAL_BASE           0x60
#define I8042_DATA_REGISTER_OFFSET    0
#define I8042_COMMAND_REGISTER_OFFSET 4
#define I8042_STATUS_REGISTER_OFFSET  4

void ShowStatusLine(void);

#define KEYBOARD_IRQ       1
