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

int ShellChangeDirOb(char* args);
int ShellListDirOb(char* args);
int ShellChangeDirFile(char* args);
int ShellListDirFile(char* args);
VOID TstReadLineInit(VOID);
VOID TstReadLine(ULONG Length, PCHAR Buffer);

/* GLOBALS ******************************************************************/

static HANDLE CurrentDirHandle = NULL;
static UNICODE_STRING CurrentDirName = {0,0,NULL};
static char current_dir_name[255] = {0,};
static HANDLE CurrentDirHandleF = NULL;
static UNICODE_STRING CurrentDirNameF = {0,0,NULL};
static char current_dir_name_f[255] = {0,};

typedef struct
{
   char* name;
   int (*fn)(char* args);
} command;

command commands[]=
{
     {"fcd",ShellChangeDirFile},
     {"fdir",ShellListDirFile},
     {"ocd",ShellChangeDirOb},
     {"odir",ShellListDirOb},
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

int ShellChangeDirOb(char* args)
{
 char* end;
 ANSI_STRING astr;
 OBJECT_ATTRIBUTES attr;
 NTSTATUS Status;
   
   DPRINT("ShellChangeDir(args %s)\n",args);
   
   args = eat_white_space(args);
   end = strchr(args,' ');
   if (end!=NULL)
     {
	*end=0;
     }
   if (args[0]=='\\') strcpy(current_dir_name,args);
   else strcat(current_dir_name,args);
   
   DPRINT("current_dir_name %s\n",current_dir_name);
   
   RtlInitAnsiString(&astr,current_dir_name);
   RtlAnsiStringToUnicodeString(&CurrentDirName,&astr,TRUE);
   InitializeObjectAttributes(&attr,&CurrentDirName,0,NULL,NULL);
   ZwClose(CurrentDirHandle);
   Status=ZwOpenDirectoryObject(&CurrentDirHandle,0,&attr);
   DbgPrint("Status=%x,dirhandle=%d\n",Status,CurrentDirHandle);
   return 0;
}

int ShellChangeDirFile(char* args)
{
 char* end;
 ANSI_STRING astr;
 OBJECT_ATTRIBUTES attr;
 NTSTATUS Status;
   DPRINT("ShellChangeDir(args %s)\n",args);
   
   args = eat_white_space(args);
   end = strchr(args,' ');
   if (end!=NULL)
     {
	*end=0;
     }
   if (args[0]=='\\') strcpy(current_dir_name_f,args);
   else if (args[0])
   {
       strcat(current_dir_name_f,"\\");
       strcat(current_dir_name_f,args);
   }

   
   DPRINT("current_dir_name_f %s\n",current_dir_name_f);
   
   RtlInitAnsiString(&astr,current_dir_name_f);
   RtlAnsiStringToUnicodeString(&CurrentDirNameF,&astr,TRUE);
   InitializeObjectAttributes(&attr,&CurrentDirNameF,0,NULL,NULL);
   ZwClose(CurrentDirHandleF);
   Status=ZwOpenFile(&CurrentDirHandleF,FILE_ALL_ACCESS,&attr,NULL,0,0);
   DbgPrint("Status=%x,dirhandle=%d\n",Status,CurrentDirHandleF);
   return 0;
}

int ShellListDirOb(char* args)
{
   OBJDIR_INFORMATION DirObj[50];
   ULONG Idx=0;
   ULONG Length=0;
   ULONG i;
   
   if (args) args = eat_white_space(args);
   DbgPrint("ShellListDir(args %s)\n",args);
   
   ZwQueryDirectoryObject(CurrentDirHandle,
			  &(DirObj[0]),
			  sizeof(DirObj),
              TRUE,
			  TRUE,
			  &Idx,
			  &Length);
   DbgPrint("read %d bytes,Idx=%d\n",Length,Idx);
   for (i=0;i<(Length/sizeof(OBJDIR_INFORMATION));i++)
     {
    DbgPrint("i=%d ; ",i);
    DbgPrint("Scanning %w\n",DirObj[i].ObjectName.Buffer);
     }
   return 0;
}

int ShellListDirFile(char* args)
{
 PFILE_BOTH_DIRECTORY_INFORMATION dirInfo,dirInfo2;
 NTSTATUS Status;
 ANSI_STRING afilename;
 UNICODE_STRING ToFind;
 WCHAR Name[251];
 short i;
 int Length=512;
   dirInfo=ExAllocatePool(NonPagedPool,Length);
   if (args) args = eat_white_space(args);
   if(args && args[0])
      RtlInitAnsiString(&afilename,args);
   else
      RtlInitAnsiString(&afilename,"*");
   RtlAnsiStringToUnicodeString(&ToFind,&afilename,TRUE);
   DbgPrint("ShellListDir(args %s)\n",args);
   DPRINT("before ZwQueryDirectoryFile\n");
   Status=ZwQueryDirectoryFile(CurrentDirHandleF,NULL,NULL,NULL
             ,NULL
             , &(dirInfo[0])
             ,Length
             ,FileBothDirectoryInformation
             ,TRUE
             ,&ToFind
             ,TRUE);
   DPRINT("after ZwQueryDirectoryFile\n");
   while(NT_SUCCESS(Status))
   {
     CHECKPOINT;
     dirInfo2=dirInfo;
     while(1)
     {
       memcpy(Name,dirInfo2[0].ShortName,12*sizeof(WCHAR));
       Name[(int)(dirInfo2[0].ShortNameLength)]=0;
       DbgPrint("  %w",Name);
       for (i=dirInfo2[0].ShortNameLength; i<14;i++) DbgPrint(" ");
       memcpy(Name,dirInfo2[0].FileName,dirInfo2[0].FileNameLength*sizeof(WCHAR));
       Name[dirInfo2[0].FileNameLength]=0;
       DbgPrint("  %w\n",Name);
       if(!dirInfo2->NextEntryOffset) break;
       dirInfo2=(PFILE_BOTH_DIRECTORY_INFORMATION)
                 (((char *)dirInfo2)+dirInfo2->NextEntryOffset);
     }
     CHECKPOINT;
     Status=ZwQueryDirectoryFile(CurrentDirHandleF,NULL,NULL,NULL
             ,NULL
             , &(dirInfo[0])
             ,Length
             ,FileBothDirectoryInformation
             , FALSE
             ,&ToFind
             ,FALSE);
     CHECKPOINT;
   }
   ExFreePool(dirInfo);
   return 0;
}

VOID ShellDisplayPrompt(VOID)
{
   printk("%w# ",CurrentDirName.Buffer);
}

VOID ShellProcessCommand(char* cmd)
{
   unsigned int i=0;
   DbgPrint("Processing cmd :%s.\n",cmd);
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

  ShellChangeDirFile("\\??\\C:\\");
   
   TstReadLineInit();
   
   for(;;)
     {
	ShellDisplayPrompt();
	TstReadLine(255,cmd);
	ShellProcessCommand(cmd);
     }
}
