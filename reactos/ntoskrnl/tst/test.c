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
#include <internal/kernel.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

#include <in.h>

/* GLOBALS ******************************************************************/

static KEVENT event;

/* FUNCTIONS ****************************************************************/

NTSTATUS TstPlaySound(void)
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
   printk("Beginning Thread A\n");
   KeClearEvent(&event);
   KeSetEvent(&event,IO_NO_INCREMENT,TRUE);
}

NTSTATUS TstSecondThread(PVOID start)
{
   printk("Beginning Thread B\n");  
   KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
   printk("Ending Thread B\n");
}

NTSTATUS TstThreadSupport()
{
   HANDLE th1, th2;
   
   KeInitializeEvent(&event,SynchronizationEvent,FALSE);
//   PsCreateSystemThread(&th1,0,NULL,NULL,NULL,TstFirstThread,NULL);
   KeClearEvent(&event);
   PsCreateSystemThread(&th2,0,NULL,NULL,NULL,TstSecondThread,NULL);
   for(;;);
}

void TstGeneralWrite()
{
   OBJECT_ATTRIBUTES attr;
   HANDLE hfile;
   char buf[256];
   ANSI_STRING afilename;
   UNICODE_STRING ufilename;
   
   DbgPrint("Opening test device\n");
   RtlInitAnsiString(&afilename,"\\Device\\Test");
   RtlAnsiStringToUnicodeString(&ufilename,&afilename,TRUE);
   InitializeObjectAttributes(&attr,&ufilename,0,NULL,NULL);
   ZwOpenFile(&hfile,0,&attr,NULL,0,0);
   if (hfile==NULL)
     {
	DbgPrint("Failed to open test device\n");
        return;
     }
   strcpy(buf,"hello world");
   ZwWriteFile(hfile,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       buf,
	       strlen(buf),
	       0,
	       0);
 }

void TstParallelPortWrite()
{
   HANDLE hfile;
   
   DbgPrint("Opening parallel port\n");
//   hfile = CreateFile("\\Device\\Parallel",0,0,0,0,0,0);
   if (hfile==NULL)
     {
	DbgPrint("Failed to open parallel port\n");
     }
 //  WriteFile(hfile,"hello world",strlen("hello world"),NULL,NULL);
 }

void TstKeyboardRead()
{
   KEY_EVENT_RECORD key;
   HANDLE hfile;
   
//   hfile = CreateFile("\\Device\\Keyboard",0,0,0,0,0,0);
   if (hfile == NULL)
     {
	printk("Failed to open keyboard\n");
	return;
     }
   for (;;)
     {
//	ReadFile(hfile,&key,sizeof(KEY_EVENT_RECORD),NULL,NULL);
	printk("%c",key.AsciiChar);
	for(;;);
     }
}

void TstBegin()
{
   TstGeneralWrite();
}

