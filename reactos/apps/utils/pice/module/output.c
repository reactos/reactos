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

#include <linux/sched.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <linux/utsname.h>
#include <linux/sched.h>
#include <linux/console.h>
#include <asm/delay.h>

char tempOutput[1024],tempOutput2[1024];

ULONG ulPrintk=0;
BOOLEAN bInPrintk = FALSE;
BOOLEAN bIsDebugPrint = FALSE;

ULONG ulCountTimerEvents = 0;
struct timer_list sPiceRunningTimer;

asmlinkage int printk(const char *fmt, ...);

EXPORT_SYMBOL(printk);

//************************************************************************* 
// printk() 
// 
// this function overrides printk() in the kernel
//************************************************************************* 
asmlinkage int printk(const char *fmt, ...)
{
	ULONG len,ulRingBufferLock;
    static ULONG ulOldJiffies = 0;
	va_list args;
	va_start(args, fmt);

	if((len = PICE_strlen((LPSTR)fmt)) )
	{
	    save_flags(ulRingBufferLock);
	    cli();

		PICE_vsprintf(tempOutput, fmt, args);
		bIsDebugPrint = TRUE;
        // if the last debug print was longer than 5 timer ticks ago
        // directly print it, else just add it to the ring buffer
        // and let the timer process it.
        if( (jiffies-ulOldJiffies) > (1*wWindow[OUTPUT_WINDOW].cy)/2)
        {
            ulOldJiffies = jiffies;
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

//************************************************************************* 
// PrintkCallback() 
// 
// called from RealIsr() when processing INT3 placed
//************************************************************************* 
void PrintkCallback(void)
{
	LPSTR fmt,args;
	ULONG ulAddress;
	ULONG countArgs,i,len;

	bInPrintk = TRUE;

	// get the linear address of stack where string resides
	ulAddress = GetLinearAddress(CurrentSS,CurrentESP);
	if(ulAddress)
	{
		if(IsAddressValid(ulAddress+sizeof(char *)) )
		{
			fmt = (LPSTR)*(PULONG)(ulAddress+sizeof(char *));

			// validate format string
			if((len = PICE_strlen(fmt)) )
			{
				// skip debug prefix if present
				if(len>=3 && *fmt=='<' && *(fmt+2)=='>')
					fmt += 3;

				if((countArgs = CountArgs(fmt))>0)
				{
					
					args = (LPSTR)(ulAddress+2*sizeof(char *));
					if(IsAddressValid((ULONG)args))
					{
						// validate passed in args
						for(i=0;i<countArgs;i++)
						{
							if(!IsRangeValid((ULONG)(args+i*sizeof(ULONG)),sizeof(ULONG)) )
							{
								PICE_sprintf(tempOutput,"printk(%s): argument #%u is not valid!\n",(LPSTR)fmt,i);
								Print(OUTPUT_WINDOW,tempOutput);
								bInPrintk = FALSE;
								return;
							}
						}
						PICE_vsprintf(tempOutput2, fmt, args);
					}
					else
					{
						Print(OUTPUT_WINDOW,"printk(): ARGS are passed in but not valid!\n");
					}
				}
				else
				{
					PICE_strcpy(tempOutput2, fmt);
				}
				Print(OUTPUT_WINDOW,tempOutput2);
			}
		}
	}
	bInPrintk = FALSE;
}

//************************************************************************* 
// PiceRunningTimer() 
// 
//************************************************************************* 
void PiceRunningTimer(unsigned long param)
{
	mod_timer(&sPiceRunningTimer,jiffies + HZ/10);

   	CheckRingBuffer();

    if(ulCountTimerEvents++ > 10)
    {
        ulCountTimerEvents = 0;

        SetForegroundColor(COLOR_TEXT);
	    SetBackgroundColor(COLOR_CAPTION);
        PICE_sprintf(tempOutput,"jiffies = %.8X\n",jiffies);
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
	init_timer(&sPiceRunningTimer);
	sPiceRunningTimer.data = 0;
	sPiceRunningTimer.function = PiceRunningTimer;
	sPiceRunningTimer.expires = jiffies + HZ;
	add_timer(&sPiceRunningTimer);
}

//************************************************************************* 
// RemovePiceRunningTimer() 
// 
//************************************************************************* 
void RemovePiceRunningTimer(void)
{
	del_timer(&sPiceRunningTimer);
}

//************************************************************************* 
// InstallPrintkHook() 
// 
//************************************************************************* 
void InstallPrintkHook(void)
{
    ENTER_FUNC();
    DPRINT((0,"enter InstallPrintk()\n"));

    ScanExports("printk",(PULONG)&ulPrintk);
    if(ulPrintk)
    {
        InstallSWBreakpoint(ulPrintk,TRUE,PrintkCallback);
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

    if(ulPrintk)
    {
		// will be done on exit debugger
        DeInstallSWBreakpoint(ulPrintk);
    }


    LEAVE_FUNC();
}
