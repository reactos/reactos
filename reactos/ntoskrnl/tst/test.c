/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/test/test.c
 * PURPOSE:         Kernel regression tests
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                28/05/98: Created
 */

/* INCLUDES *****************************************************************/

#include <windows.h>

#include <internal/kernel.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static KEVENT event;

/* FUNCTIONS ****************************************************************/

NTSTATUS TstFirstThread(PVOID start)
{
   printk("Beginning Thread A\n");
   for(;;)
     {
	KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
	KeClearEvent(&event);
	printk("AAA ");
	KeSetEvent(&event,IO_NO_INCREMENT,TRUE);
     }
}

NTSTATUS TstSecondThread(PVOID start)
{
   printk("Beginning Thread B\n");  
   for(;;)
     {
	KeSetEvent(&event,IO_NO_INCREMENT,TRUE);
	KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
	KeClearEvent(&event);
	printk("BBB ");
     }
}

NTSTATUS TstThreadSupport()
{
   HANDLE th1, th2;
   
   KeInitializeEvent(&event,SynchronizationEvent,FALSE);
   PsCreateSystemThread(&th1,0,NULL,NULL,NULL,TstFirstThread,NULL);
   PsCreateSystemThread(&th2,0,NULL,NULL,NULL,TstSecondThread,NULL);
   for(;;);
}

void TstParallelPortWrite()
{
   HANDLE hfile;
   
   DbgPrint("Opening parallel port\n");
   hfile = CreateFile("\\Device\\Parallel",0,0,0,0,0,0);
   if (hfile==NULL)
     {
	DbgPrint("Failed to open parallel port\n");
     }
   WriteFile(hfile,"hello world",strlen("hello world"),NULL,NULL);
 }

void TstKeyboardRead()
{
   KEY_EVENT_RECORD key;
   HANDLE hfile;
   
   hfile = CreateFile("\\Device\\Keyboard",0,0,0,0,0,0);
   if (hfile == NULL)
     {
	printk("Failed to open keyboard\n");
	return;
     }
   for (;;)
     {
	ReadFile(hfile,&key,sizeof(KEY_EVENT_RECORD),NULL,NULL);
	printk("%c",key.AsciiChar);
	for(;;);
     }
}

void TstBegin()
{
   TstKeyboardRead();
}

