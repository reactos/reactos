/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    output.c

Abstract:

    catch debugging outputs

Environment:

    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    14-Nov-1999:	created
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include "remods.h"
#include "precomp.h"
/*
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <linux/utsname.h>
#include <linux/sched.h>
#include <linux/console.h>
#include <asm/delay.h>
*/

char tempOutput[1024],tempOutput2[1024];

//ULONG ulPrintk=0;

ULONG (*ulPrintk) (PANSI_STRING String);

BOOLEAN bInPrintk = FALSE;
BOOLEAN bIsDebugPrint = FALSE;
BOOLEAN bIsPrintkPatched = FALSE;

ULONG ulCountTimerEvents = 0;

#ifdef __cplusplus
#define CPP_ASMLINKAGE extern "C"
#else
#define CPP_ASMLINKAGE
#endif
#define asmlinkage CPP_ASMLINKAGE __attribute__((regparm(0)))

asmlinkage int printk(const char *fmt, ...);

//EXPORT_SYMBOL(printk);

//*************************************************************************
// printk()
//
// this function overrides printk() in the kernel
//*************************************************************************
asmlinkage int printk(const char *fmt, ...)
{
	ULONG len,ulRingBufferLock;
    static LONGLONG ulOldJiffies = 0;
	LARGE_INTEGER jiffies;

	va_list args;
	va_start(args, fmt);

	if((len = PICE_strlen((LPSTR)fmt)) )
	{
	    save_flags(ulRingBufferLock);
	    cli();

		PICE_vsprintf(tempOutput, fmt, args);
		bIsDebugPrint = TRUE;
        // if the last debug print was longer than 50 ms ago
        // directly print it, else just add it to the ring buffer
        // and let the timer process it.
		KeQuerySystemTime(&jiffies);
        if( (jiffies.QuadPart-ulOldJiffies) > 10000*(1*wWindow[OUTPUT_WINDOW].cy)/2)
        {
            ulOldJiffies = jiffies.QuadPart;
		    Print(OUTPUT_WINDOW,tempOutput);
        }
        else
        {
		    AddToRingBuffer(tempOutput);
        }

		bIsDebugPrint = FALSE;
        restore_flags(ulRingBufferLock);
	}
	va_end(args);

    return 0;
}

//*************************************************************************
// CountArgs()
//
// count occurrence of '%' in format string (except %%)
// validity of whole format string must have been enforced
//*************************************************************************
ULONG CountArgs(LPSTR fmt)
{
	ULONG count=0;

	while(*fmt)
	{
		if(*fmt=='%' && *(fmt+1)!='%')
			count++;
		fmt++;
	}
	return count;
}

//***********************************************************************************
//	Our replacement of kernel function.
//	Must not make any calls to KdpPrintString (e.g. by calling DbgPrint).
//***********************************************************************************
ULONG PICE_KdpPrintString(PANSI_STRING String)
{
	//dummy function
	DPRINT((0,"PICE_KdpPrintString\n\n\n"));
}
//*************************************************************************
// PrintkCallback()
//
// called from RealIsr() when processing INT3 placed
// Must not make any calls to KdpPrintString (e.g. by calling DbgPrint).
//*************************************************************************
void PrintkCallback(void)
{
	LPSTR fmt,args;
	ULONG ulAddress;
	ULONG countArgs,i,len;
	PANSI_STRING temp;

	DPRINT((2,"In PrintkCallback:1\n"));

	bInPrintk = TRUE;
	DPRINT((2,"In PrintkCallback:2\n"));

	// get the linear address of stack where string resides
	ulAddress = GetLinearAddress(CurrentSS,CurrentESP);
	if(ulAddress)
	{
		DPRINT((2,"In PrintkCallback: ulAddress: %x\n", ulAddress));
		if(IsAddressValid(ulAddress+sizeof(char *)) )
		{
			//KdpPrintString has PANSI_STRING as a parameter
			temp = (PANSI_STRING)*(PULONG)(ulAddress+sizeof(char *));
			DPRINT((2,"temp: %x\n", temp));
			fmt = temp->Buffer;

			Print(OUTPUT_WINDOW,fmt);
			DPRINT((2,"%s\n", fmt));
			CurrentEIP = (ULONG)PICE_KdpPrintString;
		}
	}
	bInPrintk = FALSE;
}

//*************************************************************************
// PiceRunningTimer()
//
//*************************************************************************

KTIMER PiceTimer;
KDPC PiceTimerDPC;

// do I need it here? Have to keep DPC memory resident #pragma code_seg()
VOID PiceRunningTimer(IN PKDPC Dpc,
                       IN PVOID DeferredContext,
                       IN PVOID SystemArgument1,
                       IN PVOID SystemArgument2)
{
   	CheckRingBuffer();

    if(ulCountTimerEvents++ > 10)
    {
		LARGE_INTEGER jiffies;

		ulCountTimerEvents = 0;

		KeQuerySystemTime(&jiffies);
        SetForegroundColor(COLOR_TEXT);
	    SetBackgroundColor(COLOR_CAPTION);
        PICE_sprintf(tempOutput,"jiffies = %.8X\n",jiffies.u.LowPart);
	    PutChar(tempOutput,GLOBAL_SCREEN_WIDTH-strlen(tempOutput),GLOBAL_SCREEN_HEIGHT-1);
        ResetColor();
    }
}

//*************************************************************************
// InitPiceRunningTimer()
//
//*************************************************************************
void InitPiceRunningTimer(void)
{
	LARGE_INTEGER   Interval;

	ENTER_FUNC();
#if 0  //won't work. we have to intercept timer interrupt so dpc will never fire while we are in pice
	KeInitializeTimer( &PiceTimer );
	KeInitializeDpc( &PiceTimerDPC, PiceRunningTimer, NULL );

	Interval.QuadPart=-1000000L;  // 100 millisec. (unit is 100 nanosec.)

    KeSetTimerEx(&PiceTimer,
                        Interval, 1000000L,
                        &PiceTimerDpc);
#endif
    LEAVE_FUNC();
}

//*************************************************************************
// RemovePiceRunningTimer()
//
//*************************************************************************
void RemovePiceRunningTimer(void)
{
	KeCancelTimer( &PiceTimer );
}

//*************************************************************************
// InstallPrintkHook()
//
//*************************************************************************
void InstallPrintkHook(void)
{

	ENTER_FUNC();
	return;
	if( bIsPrintkPatched )
			return;

    DPRINT((0,"installing PrintString hook\n"));
	ScanExports("_KdpPrintString",(PULONG)&ulPrintk);

	DPRINT((0,"_KdpPrintString @ %x\n", ulPrintk));
	ASSERT( ulPrintk );                 // temporary
    if(ulPrintk)
    {
        bIsPrintkPatched = InstallSWBreakpoint(ulPrintk,TRUE,PrintkCallback);
		DPRINT((0,"KdpPrintStringTest breakpoint installed? %d\n", bIsPrintkPatched));
    }
	LEAVE_FUNC();
}

//*************************************************************************
// DeInstallPrintkHook()
//
//*************************************************************************
void DeInstallPrintkHook(void)
{
    ENTER_FUNC();
    DPRINT((0,"enter DeInstallPrintkHook()\n"));
    if(bIsPrintkPatched && ulPrintk)
    {
		// will be done on exit debugger
        if( DeInstallSWBreakpoint(ulPrintk) )
				bIsPrintkPatched = FALSE;
    }
    LEAVE_FUNC();
}
