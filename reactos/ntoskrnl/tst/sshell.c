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

#include <in.h>

int ShellChangeDir(char* args);
int ShellListDir(char* args);

/* GLOBALS ******************************************************************/

static HANDLE CurrentDirHandle;
static UNICODE_STRING CurrentDirName;
static HANDLE KeyboardHandle;

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
}

VOID ShellDisplayPrompt()
{
   printk("%w# ",CurrentDirName->Buffer);
}

VOID ShellGetCommand(char* cmd)
{
   do
     {
	ZwReadFile(hfile,NULL,NULL,NULL,NULL,cmd,1,0,0);
	printk("%c",*cmd);
	cmd++;
     } while((*cmd)!='\n');
   *cmd=0;
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
   
   RtlInitAnsiString(&afilename,"\\Device\\Keyboard");
   RtlAnsiStringToUnicodeString(&ufilename,&afilename,TRUE);
   InitializeObjectAttributes(&attr,&ufilename,0,NULL,NULL);
   ZwOpenFile(&KeyboardHandle,0,&attr,NULL,0,0);

   
   for(;;)
     {
	ShellDisplayPrompt();
	ShellGetCommand(cmd);
	ShellProcessCommand(cmd);
     }
}
