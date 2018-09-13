/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  ISVWOW.H
 *  WOW32 ISV Support. Public Functions and Macros for Multi-Media extensions
 *  to the WOW thunking mechanism.
 *
 *  History:
 *  Created 18-Feb-1992 by Stephen Estrop (StephenE)
--*/


/*
** Public functions that allow for extensions to the WOW thunking
** system.  These two functions enable extension thunk dlls such as
** Multi-Media video recording to callback into 16 bit code to simulate
** a hardware interrupt callback and to use the same handle mapping that
** WOW uses.
*/
LPVOID
WOW32ResolveMemory(
    VPVOID  vp
    );

BOOL APIENTRY
WOW32DriverCallback(
    DWORD dwCallback,
    DWORD dwFlags,
    WORD wID,
    WORD wMsg,
    DWORD dwUser,
    DWORD dw1,
    DWORD dw2
    );

BOOL APIENTRY
WOW32ResolveHandle(
    UINT uHandleType,
    UINT uMappingDirection,
    WORD wHandle16_In,
    LPWORD lpwHandle16_Out,
    DWORD dwHandle32_In,
    LPDWORD lpdwHandle32_Out
    );


/*
** Constants for use with WOW32ResolveHandle
*/

#define WOW32_DIR_16IN_32OUT        0x0001
#define WOW32_DIR_32IN_16OUT        0x0002

#define WOW32_USER_HANDLE           0x0001  // Generic user handle
#define WOW32_GDI_HANDLE            0x0002  // Generic gdi handle
                                            // Kernel handles are not mapped

#define WOW32_WAVEIN_HANDLE         0x0003
#define WOW32_WAVEOUT_HANDLE        0x0004
#define WOW32_MIDIOUT_HANDLE        0x0005
#define WOW32_MIDIIN_HANDLE         0x0006



/*
** These MultiMedia messages expect dwParam1 to be a generic pointer and
** dwParam2 to be a generic DWORD.  auxOutMessage, waveInMessage,
** waveOutMessage, midiInMessage and midiOutMessage all respect this
** convention and are thunked accordingly on WOW.
*/
#define DRV_BUFFER_LOW      (DRV_USER - 0x1000)     // 0x3000
#define DRV_BUFFER_USER     (DRV_USER - 0x0800)     // 0x3800
#define DRV_BUFFER_HIGH     (DRV_USER - 0x0001)     // 0x3FFF


/*
** The flags are extensions to those normally used with GetWindowFlags,
** they allow 16 bit applications to detect if they are running on NT
** and if the Intel cpu is being emulated.
*/
#define WF1_WINNT   0x40    // You are running on NT WOW
#define WF1_CPUEM   0x01    // NT WOW on MIPS or Alpha
