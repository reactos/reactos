/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/tst/sshell.c
 * PURPOSE:         Simple kernel mode shell
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

int ShellChangeDir(char* args);
int ShellListDir(char* args);
VOID TstReadLineInit(VOID);
VOID TstReadLine(ULONG Length, PCHAR Buffer);

/* GLOBALS ******************************************************************/

static HANDLE CurrentDirHandle = NULL;
static UNICODE_STRING CurrentDirName = {NULL,0,0};

typedef struct
{
   char* name;
   int (*fn)(char* args);
} command;

command commands[]=
{
     {"cd",ShellChangeDir},
     {"dir",ShellListDir},
     {NULL,NULL},
};

/* FUNCTIONS ****************************************************************/

int ShellChangeDir(char* args)
{
}

int ShellListDir(char* args)
{
   OBJDIR_INFORMATION DirObj[50];
   ULONG Idx;
   ULONG Length;
   ULONG i;
   
   ZwQueryDirectoryObject(CurrentDirHandle,
			  &(DirObj[0]),
			  sizeof(DirObj),
			  TRUE,
			  TRUE,
			  &Idx,
			  &Length);
   
   for (i=0;i<(Length/sizeof(OBJDIR_INFORMATION));i++)
     {
	DbgPrint("Scanning %w\n",DirObj[i].ObjectName.Buffer);
     }
}

VOID ShellDisplayPrompt()
{
   printk("%w# ",CurrentDirName.Buffer);
}

VOID ShellProcessCommand(char* cmd)
{
   unsigned int i=0;
   while (commands[i].name!=NULL)
     {
	if (strncmp(cmd,commands[i].name,strlen(commands[i].name))==0)
	  {
	     commands[i].fn(cmd+strlen(commands[i].name));
	  }
     }
}

NTSTATUS TstShell(VOID)
{
   ANSI_STRING astr;
   ANSI_STRING afilename;
   UNICODE_STRING ufilename;
   OBJECT_ATTRIBUTES attr;
   char cmd[255];
   
   
   RtlInitAnsiString(&astr,"\\");
   RtlAnsiStringToUnicodeString(&CurrentDirName,&astr,TRUE);
   
   RtlInitAnsiString(&afilename,"\\");
   RtlAnsiStringToUnicodeString(&ufilename,&afilename,TRUE);
   InitializeObjectAttributes(&attr,&ufilename,0,NULL,NULL);
   ZwOpenDirectoryObject(&CurrentDirHandle,0,&attr);

   
   TstReadLineInit();
   
   for(;;)
     {
	ShellDisplayPrompt();
	TstReadLine(255,cmd);
	ShellProcessCommand(cmd);
     }
}
