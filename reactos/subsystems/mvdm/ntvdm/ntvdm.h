/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/ntvdm.h
 * PURPOSE:         Header file to define commonly used stuff
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _NTVDM_H_
#define _NTVDM_H_

/* INCLUDES *******************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <wincon.h>
#include <winnls.h>
#include <winreg.h>
#include <winuser.h>
#include <commdlg.h>

#include <subsys/win/vdm.h>

// Do not include stuff that is only defined
// for backwards compatibility in nt_vdd.h
#define NO_NTVDD_COMPAT
#include <vddsvc.h>

DWORD WINAPI SetLastConsoleEventActive(VOID);

#define NTOS_MODE_USER
#include <ndk/kefuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/rtltypes.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

#include <reactos/buildno.h>
#include <reactos/version.h>

#include <fast486.h>
#include <isvbop.h>

#include "bios/bios32/bios32.h"
#include "bios/bios32/dskbios32.h"
#include "bios/bios32/kbdbios32.h"
#include "bios/bios32/moubios32.h"
#include "bios/bios32/vidbios32.h"
#include "bios/vidbios.h"
#include "bios/bios.h"
#include "bios/kbdbios.h"
#include "bios/umamgr.h"
#include "cpu/callback.h"
#include "bios/rom.h"
#include "clock.h"
#include "cpu/bop.h"
#include "cpu/cpu.h"
#include "dos/dem.h"
#include "dos/mouse32.h"
#include "dos/dos32krnl/country.h"
#include "dos/dos32krnl/device.h"
#include "dos/dos32krnl/dos.h"
#include "dos/dos32krnl/dosfiles.h"
#include "dos/dos32krnl/emsdrv.h"
#include "dos/dos32krnl/handle.h"
#include "dos/dos32krnl/himem.h"
#include "dos/dos32krnl/process.h"
#include "emulator.h"
#include "hardware/cmos.h"
#include "hardware/disk.h"
#include "hardware/dma.h"
#include "hardware/keyboard.h"
#include "hardware/mouse.h"
#include "hardware/pic.h"
#include "hardware/pit.h"
#include "hardware/ppi.h"
#include "hardware/ps2.h"
#include "hardware/sound/speaker.h"
#include "hardware/video/svga.h"
#include "bios/bios32/vbe.h"
#include "int32.h"
#include "bios/bios32/bios32p.h"
#include "io.h"
#include "utils.h"
#include "vddsup.h"

#include "memory.h"
#include "dos/dos32krnl/memory.h"

/*
 * Activate this line if you want to run NTVDM in standalone mode with:
 * ntvdm.exe <program>
 */
// #define STANDALONE

/*
 * Activate this line for Win2k compliancy
 */
// #define WIN2K_COMPLIANT

/*
 * Activate this line if you want advanced hardcoded debug facilities
 * (called interrupts, etc...), that break PC-AT compatibility.
 * USE AT YOUR OWN RISK! (disabled by default)
 */
// #define ADVANCED_DEBUGGING

#ifdef ADVANCED_DEBUGGING
#define ADVANCED_DEBUGGING_LEVEL    1
#endif


/* VARIABLES ******************************************************************/

typedef struct _NTVDM_SETTINGS
{
    ANSI_STRING BiosFileName;
    ANSI_STRING RomFiles;
    ANSI_STRING FloppyDisks[2];
    ANSI_STRING HardDisks[4];
} NTVDM_SETTINGS, *PNTVDM_SETTINGS;

extern NTVDM_SETTINGS GlobalSettings;

// Command line of NTVDM
extern INT     NtVdmArgc;
extern WCHAR** NtVdmArgv;

extern HWND hConsoleWnd;


/* FUNCTIONS ******************************************************************/

/*
 * Interface functions
 */
typedef VOID (*CHAR_PRINT)(IN CHAR Character);
VOID DisplayMessage(IN LPCWSTR Format, ...);
VOID PrintMessageAnsi(IN CHAR_PRINT CharPrint,
                      IN LPCSTR Format, ...);

/*static*/ VOID
UpdateVdmMenuDisks(VOID);

BOOL ConsoleAttach(VOID);
VOID ConsoleDetach(VOID);
VOID ConsoleReattach(HANDLE ConOutHandle);
VOID MenuEventHandler(PMENU_EVENT_RECORD MenuEvent);
VOID FocusEventHandler(PFOCUS_EVENT_RECORD FocusEvent);

#endif /* _NTVDM_H_ */
