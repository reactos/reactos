#include <windows.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


HANDLE InputHandle, OutputHandle;

void debug_printf(char* fmt, ...)
{
   va_list args;
   char buffer[255];

   va_start(args,fmt);
   vsprintf(buffer,fmt,args);
   WriteConsoleA(OutputHandle, buffer, strlen(buffer), NULL, NULL);
   va_end(args);
}

void ExecuteVer(void)
{
    debug_printf(
	"Reactos Simple Shell\n(compiled on %s, at %s)\n",
	__DATE__,
	__TIME__
	);
}

void ExecuteCd(char* cmdline)
{
   if (!SetCurrentDirectoryA(cmdline))
     {
	debug_printf("Invalid directory\n");
     }
}

void ExecuteDir(char* cmdline)
{
   HANDLE shandle;
   WIN32_FIND_DATA FindData;
   int nFile=0, nRep=0;
   FILETIME fTime;
   SYSTEMTIME sTime;

   shandle = FindFirstFile("*",&FindData);

   if (shandle==INVALID_HANDLE_VALUE)
     {
	debug_printf("Invalid directory\n");
	return;
     }
   do
     {
	debug_printf("%-15.15s",FindData.cAlternateFileName);
	if(FindData.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY)
          debug_printf("<DIR>       "),nRep++;
	else
	  debug_printf(" %10d ",FindData.nFileSizeLow),nFile++;

        FileTimeToLocalFileTime(&FindData.ftLastWriteTime ,&fTime);
        FileTimeToSystemTime(&fTime, &sTime);
        debug_printf("%02d/%02d/%04d %02d:%02d:%02d "
            ,sTime.wMonth,sTime.wDay,sTime.wYear
            ,sTime.wHour,sTime.wMinute,sTime.wSecond);

	debug_printf("%s\n",FindData.cFileName);
     } while(FindNextFile(shandle,&FindData));
   debug_printf("\n    %d files\n    %d directories\n\n",nFile,nRep);
   FindClose(shandle);
}

void ExecuteType(char* cmdline)
{
   HANDLE FileHandle;
   char c;
   DWORD Result;

   FileHandle = CreateFile(cmdline,
			   FILE_GENERIC_READ,
			   0,
			   NULL,
			   OPEN_EXISTING,
			   0,
			   NULL);
   if (FileHandle == NULL)
     {
	debug_printf("Unknown file\n");
	return;
     }
   while (ReadFile(FileHandle,
		   &c,
		   1,
		   &Result,
		   NULL))
     {
	debug_printf("%c",c);
	c = 0;
     }
   CloseHandle(FileHandle);
}

int ExecuteProcess(char* name, char* cmdline, BOOL detached)
{
	PROCESS_INFORMATION	ProcessInformation;
	STARTUPINFO		StartupInfo;
//	char			arguments;
	BOOL			ret;

	memset(
		& StartupInfo,
		0,
		(sizeof StartupInfo)
		);
	StartupInfo.cb = sizeof (STARTUPINFO);
	StartupInfo.lpTitle = name;

	ret = CreateProcessA(
		name,
		cmdline,
		NULL,
		NULL,
		TRUE,
		(	(TRUE == detached)
			? DETACHED_PROCESS
			: CREATE_NEW_CONSOLE
			),
		NULL,
		NULL,
		& StartupInfo,
		& ProcessInformation
		);
	if (TRUE == detached)
	{
		if (ret)
		{
			debug_printf(
"%s detached:\n\
\thProcess = %08X\n\
\thThread  = %08X\n\
\tPID      = %d\n\
\tTID      = %d\n\n",
				name,
				ProcessInformation.hProcess,
				ProcessInformation.hThread,
				ProcessInformation.dwProcessId,
				ProcessInformation.dwThreadId
				);
		}
		else
		{
			debug_printf(
				"Could not detach %s\n",
				name
				);
		}
	}
	else
	{
		if (ret)
		{
			WaitForSingleObject(
				ProcessInformation.hProcess,
				INFINITE
				);
		}
		CloseHandle(ProcessInformation.hProcess);
	}
	return(ret);
}

void
ExecuteStart(
	char	* CommandLine
	)
{
	   char *ImageName = CommandLine;

	   for (	;
			(	(*CommandLine)
				&& (*CommandLine != ' ')
				&& (*CommandLine != '\t')
				);
			CommandLine++
		);
	   *CommandLine++ = '\0';
	   while (	(*CommandLine)
			&& (	(*CommandLine == ' ')
				|| (*CommandLine == '\t')
				)
			);
	   ExecuteProcess(
		ImageName,
		CommandLine,
		TRUE
		);
	   return;
}

void
ExecuteKill(char * lpPid)
{
	HANDLE	hProcess;
	DWORD	dwProcessId;

	dwProcessId = (DWORD) atol(lpPid);
   debug_printf("dwProcessId %d\n",dwProcessId);
	hProcess = OpenProcess(
			PROCESS_TERMINATE,
			FALSE,
			dwProcessId
			);
	if (NULL == hProcess)
	{
		debug_printf(
			"Could not open the process with PID = %d\n",
			dwProcessId
			);
		return;
	}
	if (FALSE == TerminateProcess(
			hProcess,
			0
			)
	) {
		debug_printf(
			"Could not terminate the process with PID = %d\n",
			dwProcessId
			);
	}
	CloseHandle(hProcess);
	return;
}

void ExecuteCommand(char* line)
{
   char* cmd;
   char* tail;

   if (isalpha(line[0]) && line[1] == ':' && line[2] == 0)
     {
	line[2] = '\\';
	line[3] = 0;
	SetCurrentDirectoryA(line);
	return;
     }

   tail = line;
   while ((*tail)!=' ' && (*tail)!=0)
     {
	tail++;	
     }
   if ((*tail)==' ')
     {
	*tail = 0;
	tail++;
     }
   cmd = line;


   if (cmd==NULL)
     {
	return;
     }
   if (strcmp(cmd,"cd")==0)
     {
	ExecuteCd(tail);
	return;
     }
   if (strcmp(cmd,"dir")==0)
     {
	ExecuteDir(tail);
	return;
     }
   if (strcmp(cmd,"kill")==0)
     {
	ExecuteKill(tail);
	return;
     }
   if (strcmp(cmd,"type")==0)
     {
	ExecuteType(tail);	
	return;
     }
   if (strcmp(cmd,"ver")==0)
     {
        ExecuteVer(); 
	return;
     }
   if (strcmp(cmd,"validate")==0)
     {
	debug_printf("Validating heap...");
	if (HeapValidate(GetProcessHeap(),0,NULL))
	  {
	     debug_printf("succeeded\n");
	  }
	else
	  {
	     debug_printf("failed\n");
	  }
	return;
     }
   if (strcmp(cmd,"start") == 0)
   {
	   ExecuteStart(tail);
	   return;
   }
   if (strcmp(cmd,"exit")==0)
     {
	ExitProcess(0);
	return;
     }
   if (ExecuteProcess(cmd,tail,FALSE))
     {
	return;
     }
   debug_printf("Unknown command\n");
}

void ReadLine(char* line)
{
//   KEY_EVENT_RECORD KeyEvent;
   DWORD Result;
   UCHAR CurrentDir[255];
   char  ch;
   int   length = 0;

   GetCurrentDirectoryA(255,CurrentDir);
   debug_printf("%s>", CurrentDir);

   do
     {
	if (!ReadConsoleA(InputHandle,
			  &ch,
			  1,
			  &Result,
			  NULL))
	  {
	     debug_printf("Failed to read from console\n");
	     for(;;);
	  }
        switch (ch)
          {
            case '\b':
              if (length > 0)
              {
                debug_printf("\b \b");
                line--;
                length--;
              }
              break;

            default:
              debug_printf("%c", ch);
              *line = ch; 
              line++;
              length++;
          }
     } while (ch != '\n');
   line--;
   *line = 0;
}

int main(void)
{
   static char line[255];

   AllocConsole();
   InputHandle = GetStdHandle(STD_INPUT_HANDLE);
   OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

   debug_printf("Shell Starting...\n");
     
   SetCurrentDirectoryA("C:\\");

   for(;;)
     {
	ReadLine(line);
	ExecuteCommand(line);
     }

   return 0;
}

