/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/tst/test.c
 * PURPOSE:         Kernel regression tests
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                28/05/98: Created
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/string.h>
#include <internal/mm.h>
#include <internal/mmhal.h>
#include <internal/i386/segment.h>
#include <internal/ps.h>

//#define NDEBUG
#include <internal/debug.h>

#include <in.h>

/* GLOBALS ******************************************************************/

static KEVENT event = {};
//static KEVENT event2;
NTSTATUS TstShell(VOID);

/* FUNCTIONS ****************************************************************/

NTSTATUS TstPlaySound(VOID)
{
   HANDLE hfile;
   
   /*
    * Open the parallel port
    */
   printk("Opening Waveout\n");
//   hfile = CreateFile("\\Device\\WaveOut",0,0,0,0,0,0);
   if (hfile == NULL)
     {
	printk("File open failed\n");
     }
   else
     {
//	WriteFile(hfile,wave,wavelength,NULL,NULL);
     }
}

NTSTATUS TstFirstThread(PVOID start)
{
   int i;
   
   printk("Beginning Thread A\n");
   for (;;)
     {     
	KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
	printk("AAA ");
        KeSetEvent(&event,IO_NO_INCREMENT,FALSE);
     }
}

NTSTATUS TstSecondThread(PVOID start)
{
   int i;
   
   printk("Beginning Thread B\n");
   for(;;)
     {
	KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
	printk("BBB ");
        KeSetEvent(&event,IO_NO_INCREMENT,FALSE);
     }
}

NTSTATUS TstThreadSupport()
{
   HANDLE th1, th2;
   
   KeInitializeEvent(&event,SynchronizationEvent,TRUE);
   PsCreateSystemThread(&th1,0,NULL,NULL,NULL,TstFirstThread,NULL);
   PsCreateSystemThread(&th2,0,NULL,NULL,NULL,TstSecondThread,NULL);
   printk("Ending main thread\n");
   for(;;);
}

VOID ExExecuteShell(VOID)
{
   HANDLE ShellHandle;
   HANDLE ThreadHandle;   
   PVOID BaseAddress;
   HANDLE SectionHandle;
   OBJECT_ATTRIBUTES attr;
   HANDLE hfile;
   ANSI_STRING afilename;
   UNICODE_STRING ufilename;
   LARGE_INTEGER SectionOffset;
   ULONG Size;
   CONTEXT Context;
   
   ZwCreateProcess(&ShellHandle,
		   PROCESS_ALL_ACCESS,
		   NULL,
		   SystemProcessHandle,
		   FALSE,
		   NULL,
		   NULL,
		   NULL);

   RtlInitAnsiString(&afilename,"\\??\\C:\\reactos\\system\\shell.bin");
   RtlAnsiStringToUnicodeString(&ufilename,&afilename,TRUE);
   InitializeObjectAttributes(&attr,&ufilename,0,NULL,NULL);
   ZwOpenFile(&hfile,FILE_ALL_ACCESS,&attr,NULL,0,0);
   if (hfile==NULL)
     {
	DbgPrint("Failed to open file\n");
        return;
     }
   
  ZwCreateSection(&SectionHandle,
		   SECTION_ALL_ACCESS,
		   NULL,
		   NULL,
		   PAGE_READWRITE,
		   MEM_COMMIT,
		   hfile);
   
   BaseAddress = (PVOID)0x10000;
   SectionOffset.HighPart = 0;
   SectionOffset.LowPart = 0;
   Size = 0x6000;
   ZwMapViewOfSection(SectionHandle,
		      ShellHandle,
		      &BaseAddress,
		      0,
                      0x6000,
		      &SectionOffset,
		      &Size,
		      0,
		      MEM_COMMIT,
		      PAGE_READWRITE);
   
   memset(&Context,0,sizeof(CONTEXT));
   
   Context.SegSs = USER_DS;
   Context.Esp = 0x2000;
   Context.EFlags = 0x202;
   Context.SegCs = USER_CS;
   Context.Eip = 0x10000;
   Context.SegDs = USER_DS;
   Context.SegEs = USER_DS;
   Context.SegFs = USER_DS;
   Context.SegGs = USER_DS;
   
   BaseAddress = 0x1000;
   ZwAllocateVirtualMemory(ShellHandle,
			   &BaseAddress,
			   0,
			   PAGESIZE,
			   MEM_COMMIT,
			   PAGE_READWRITE);
			   
   
   ZwCreateThread(&ThreadHandle,
		  THREAD_ALL_ACCESS,
		  NULL,
		  ShellHandle,
		  NULL,
		  &Context,
		  NULL,
		  FALSE);
}


void TstBegin()
{
   ExExecuteShell();
//   TstFileRead();
//   TstGeneralWrite();
//   TstThreadSupport();
//   TstKeyboardRead();
//   TstShell();
}

