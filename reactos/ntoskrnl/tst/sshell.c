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
#include <internal/ctype.h>

#define NDEBUG
#include <internal/debug.h>

int ShellChangeDir(char* args);
int ShellListDir(char* args);
VOID TstReadLineInit(VOID);
VOID TstReadLine(ULONG Length, PCHAR Buffer);

/* GLOBALS ******************************************************************/

static HANDLE CurrentDirHandle = NULL;
static UNICODE_STRING CurrentDirName = {NULL,0,0};
static char current_dir_name[255] = {0,};

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

char* eat_white_space(char* s)
{
   while (isspace(*s))
     {
	s++;
     }
   return(s);
}

int ShellChangeDir(char* args)
{
   char* end;
   ANSI_STRING astr;
   OBJECT_ATTRIBUTES attr;
   
   DPRINT("ShellChangeDir(args %s)\n",args);
   
   args = eat_white_space(args);
   end = strchr(args,' ');
   if (end!=NULL)
     {
	*end=0;
     }
   strcat(current_dir_name,args);
   
   DPRINT("current_dir_name %s\n",current_dir_name);
   
   RtlInitAnsiString(&astr,current_dir_name);
   RtlAnsiStringToUnicodeString(&CurrentDirName,&astr,TRUE);
   InitializeObjectAttributes(&attr,&CurrentDirName,0,NULL,NULL);
   ZwClose(CurrentDirHandle);
   ZwOpenDirectoryObject(&CurrentDirHandle,0,&attr);
}

int ShellListDir(char* args)
{
   OBJDIR_INFORMATION DirObj[50];
   ULONG Idx;
   ULONG Length;
   ULONG i;
   
   DbgPrint("ShellListDir(args %s)\n",args);
   
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
   DbgPrint("Processing cmd '%s'\n",cmd);
   while (commands[i].name!=NULL)
     {
	DbgPrint("Scanning %s i %d\n",commands[i].name,i);
	if (strncmp(cmd,commands[i].name,strlen(commands[i].name))==0)
	  {
	     commands[i].fn(cmd+strlen(commands[i].name));
             return;
	  }
        i++;
     }
   DbgPrint("Unknown command\n");
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
   strcpy(current_dir_name,"\\");
   
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
