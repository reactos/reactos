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
//   for (i=0;i<10;i++)
     {     
	KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
	printk("AAA ");
        KeSetEvent(&event,IO_NO_INCREMENT,FALSE);
	for (i=0;i<10000;i++);
     }
}

NTSTATUS TstSecondThread(PVOID start)
{
   int i;
   
   printk("Beginning Thread B\n");
   for(;;)
//   for (i=0;i<10;i++)
     {
	KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
	printk("BBB ");
        KeSetEvent(&event,IO_NO_INCREMENT,FALSE);
	for (i=0;i<100000;i++);
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

void TstGeneralWrite(VOID)
{
   OBJECT_ATTRIBUTES attr;
   HANDLE hfile;
   char buf[512];
   ANSI_STRING afilename;
   UNICODE_STRING ufilename;
   
   DbgPrint("Opening test device\n");
   RtlInitAnsiString(&afilename,"\\Device\\SDisk");
   RtlAnsiStringToUnicodeString(&ufilename,&afilename,TRUE);
   InitializeObjectAttributes(&attr,&ufilename,0,NULL,NULL);
   ZwOpenFile(&hfile,0,&attr,NULL,0,0);
   if (hfile==NULL)
     {
	DbgPrint("Failed to open test device\n");
        return;
     }
   ZwReadFile(hfile,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       buf,
	       512,
	       0,
	       0);
   DbgPrint("buf %s\n",buf);
 }

void TstParallelPortWrite(VOID)
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

void TstKeyboardRead(VOID)
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

void TstFileRead(VOID)
{
   OBJECT_ATTRIBUTES attr;
   HANDLE hfile;
   ANSI_STRING afilename;
   UNICODE_STRING ufilename;
   char ch;
   IO_STATUS_BLOCK IoStatusBlock;
   
   DbgPrint("Opening file\n");
   RtlInitAnsiString(&afilename,"\\??\\C:\\my_other_directory\\..\\"
		     "my_valid_directory\\apc.txt");
   RtlAnsiStringToUnicodeString(&ufilename,&afilename,TRUE);
   InitializeObjectAttributes(&attr,&ufilename,0,NULL,NULL);
   ZwOpenFile(&hfile,0,&attr,NULL,0,0);
   if (hfile==NULL)
     {
	DbgPrint("Failed to open file\n");
        return;
     }
   while (1)
     {
//	CHECKPOINT;
	ZwReadFile(hfile,
		     NULL,
		     NULL,
		     NULL,
		     &IoStatusBlock,
		     &ch,
		     1,
		     NULL,
		     NULL);
	DbgPrint("%c",ch);
     }
   CHECKPOINT;
 }



void TstBegin()
{
   TstFileRead();
//   TstGeneralWrite();
//   TstThreadSupport();
//   TstKeyboardRead();
//   TstShell();
}

