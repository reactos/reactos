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

#define NDEBUG
#include <internal/debug.h>

#include <in.h>

/* GLOBALS ******************************************************************/

static KEVENT event = {};
//static KEVENT event2;

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
   for (;;)
     {     
	KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
	printk("AAA ");
        KeSetEvent(&event,IO_NO_INCREMENT,FALSE);   
     }
}

NTSTATUS TstSecondThread(PVOID start)
{
   printk("Beginning Thread B\n");
   for (;;)
     {
        KeSetEvent(&event,IO_NO_INCREMENT,FALSE);
	KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
	printk("BBB ");
     }
}

NTSTATUS TstThreadSupport()
{
   HANDLE th1, th2;
   
   KeInitializeEvent(&event,SynchronizationEvent,FALSE);
   PsCreateSystemThread(&th1,0,NULL,NULL,NULL,TstFirstThread,NULL);
   PsCreateSystemThread(&th2,0,NULL,NULL,NULL,TstSecondThread,NULL);
   printk("Ending main thread\n");
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
   OBJECT_ATTRIBUTES attr;
   HANDLE hfile;
   ANSI_STRING afilename;
   UNICODE_STRING ufilename;
   KEY_EVENT_RECORD key[2];
   
   DbgPrint("Opening keyboard\n");
   RtlInitAnsiString(&afilename,"\\Device\\Keyboard");
   RtlAnsiStringToUnicodeString(&ufilename,&afilename,TRUE);
   InitializeObjectAttributes(&attr,&ufilename,0,NULL,NULL);
   ZwOpenFile(&hfile,0,&attr,NULL,0,0);
   if (hfile==NULL)
     {
	DbgPrint("Failed to open keyboard\n");
        return;
     }
   for(;;)
     {
	ZwReadFile(hfile,
		   NULL,
		   NULL,
		   NULL,
		   NULL,
		   &key[0],
		   sizeof(KEY_EVENT_RECORD)*2,
		   0,
		   0);
	DbgPrint("%c",key[0].AsciiChar);
//	DbgPrint("%c",key[1].AsciiChar);
     }
 }



void TstBegin()
{
//   TstGeneralWrite();
//   TstThreadSupport();
   TstKeyboardRead();
}

