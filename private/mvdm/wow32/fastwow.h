/*++ BUILD Version: 0003
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, 1992, 1993 Microsoft Corporation
 *
 *  FASTWOW.H
 *  WOW32 x86 fast callback/API support
 *
 *  History:
 *  Created 4-Dec-1992  by barry bradie (barryb)
--*/

#if defined(i386) && !defined(DEBUG_OR_WOWPROFILE)

#define FASTBOPPING     1

#else

#define FASTBOPPING     0

#endif

#if FASTBOPPING
extern BYTE fKernelCSIPFixed;
// Used by the monitor to dispatch interupts. Updated for callbacks and bops
extern DECLSPEC_IMPORT PVOID CurrentMonitorTeb;
#endif

#if FASTBOPPING
VOID    WOWBopEntry(VOID);
VPVOID  FastBopVDMStack(void);
VOID    FastBopSetVDMStack(VPVOID vp);
VOID    FastWOWCallbackCall(VOID);
VOID    FastWOWCallbackRet(VOID);
#define FASTVDMSTACK()      FastBopVDMStack()
#define SETFASTVDMSTACK(vp) FastBopSetVDMStack(vp)
#endif

//#if FASTBOPPING
//
// Used to put lock prefixes in appropriate places for MP machines
//
extern VOID FastWowFirstCode(VOID);
extern VOID FixLocks(VOID);
//#endif

#if 0

How to set a 16-bit register from the 32-bit side:

WOW16Call is called by all API thunks.  this routine sets up a stack
frame (the VDMFRAME) before getting over to WOW32.  the VDMFRAME is where
all the registers are stored - whenever a task has crossed over to WOW32
either via an API call or by returning from a callback, it saves
its registers in the frame.  immediately upon returning from WOW32
it pops the stuff off the stack back into the registers.  the way
to get a value into a specific register when the task starts executing
16-bit code again is to put it in the frame.

 ***   do not use the setAX(), setDX(), etc.  functions for this purpose  ***

those routines update a context block.  when the app re-enters 16-bit
code the registers will have the requested values but will be
immediately overwritten by the values on the stack.

similarly, to retrieve a value that was in a register at the time
of the API call you should fetch it out of the frame.
note that this is only valid for a few registers, because the
validation layer may modify the general-purpose registers.

upon returning from a callback everything on the 16-bit stack
should be valid.  to get the value that was in a register
after a callback, pull it out of the callback frame.  note that
there are general-purpose words in the callback frame for passing
extra data from the 16-bit callback routine to WOW32.


  ***  do not use the getAX(), getDX(), etc. functions for this purpose ***

#endif
