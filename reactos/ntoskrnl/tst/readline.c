/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/tst/readline.c
 * PURPOSE:         Simple readline library
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

/* GLOBALS ******************************************************************/

static HANDLE KeyboardHandle = NULL;

/* FUNCTIONS ****************************************************************/

static unsigned char TstReadLineReadChar(VOID)
{
   KEY_EVENT_RECORD key[2];
   IO_STATUS_BLOCK IoStatusBlock;
   
   ZwReadFile(KeyboardHandle,
	      NULL,
	      NULL,
	      NULL,
	      &IoStatusBlock,
	      &key[0],
	      sizeof(KEY_EVENT_RECORD)*2,
	      0,
	      0);
//   DbgPrint("%c",key[0].AsciiChar);
   return(key[0].AsciiChar);
}

VOID TstReadLine(ULONG Length, PCHAR Buffer)
{
   char* current = Buffer;
   char tmp;
   unsigned int i;
   
   for (i=0;i<Length;i++)
     {
	tmp = TstReadLineReadChar();
//	DbgPrint("%x %x ",tmp,'\n');
	switch (tmp)
	  {
	   case 0xd:
	     *current = 0;
	     DbgPrint("\n");
	     return;
	     	     
	   default:
             DbgPrint("%c",tmp);
	     *current = tmp;
	     current++;
	  }
     }
   *current=0;
   DbgPrint("\n");
}


VOID TstReadLineInit(VOID)
{
   OBJECT_ATTRIBUTES attr;
   HANDLE hfile;
   ANSI_STRING afilename;
   UNICODE_STRING ufilename;
   
   DbgPrint("Opening keyboard\n");
   RtlInitAnsiString(&afilename,"\\Device\\Keyboard");
   RtlAnsiStringToUnicodeString(&ufilename,&afilename,TRUE);
   InitializeObjectAttributes(&attr,&ufilename,0,NULL,NULL);
   ZwOpenFile(&KeyboardHandle,
	      FILE_GENERIC_READ,
	      &attr,
	      NULL,
	      0,
	      FILE_SYNCHRONOUS_IO_NONALERT);
   if (KeyboardHandle==NULL)
     {
	DbgPrint("Failed to open keyboard\n");
        return;
     }
}


