#include <internal/mmhal.h>

#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>

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
 int nFile=0,nRep=0;
 TIME_FIELDS fTime;

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
	  debug_printf("<REP>       "),nRep++;
	else
	  debug_printf(" %10d ",FindData.nFileSizeLow),nFile++;
	//    RtlTimeToTimeFields(&FindData.ftLastWriteTime  ,&fTime);
//    debug_printf("%02d/%02d/%04d %02d:%02d:%02d "
//        ,fTime.Month,fTime.Day,fTime.Year
//        ,fTime.Hour,fTime.Minute,fTime.Second);
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

int ExecuteProcess(char* name, char* cmdline)
{
   PROCESS_INFORMATION ProcessInformation;
   STARTUPINFO StartupInfo;
   char arguments;
   BOOL ret;
   
   memset(&StartupInfo,0,sizeof(StartupInfo));
   StartupInfo.cb = sizeof(STARTUPINFO);
   StartupInfo.lpTitle = name;
  
   ret = CreateProcessA(name,
			 cmdline,
			 NULL,
			 NULL,
			 TRUE,
			 CREATE_NEW_CONSOLE,
			 NULL,
			 NULL,
			 &StartupInfo,
			 &ProcessInformation);
   if (ret)
     {
	WaitForSingleObject(ProcessInformation.hProcess,INFINITE);
     }
   return(ret);
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
   if (strcmp(cmd,"type")==0)
     {
	ExecuteType(tail);	
	return;
     }
   if (strcmp(cmd,"exit")==0)
     {
	ExitProcess(0);
	return;
     }
   if (ExecuteProcess(cmd,tail))
     {
	return;
     }
   debug_printf("Unknown command\n");
}

void ReadLine(char* line)
{
   KEY_EVENT_RECORD KeyEvent;
   DWORD Result;
   UCHAR CurrentDir[255];
   char ch;
   
   GetCurrentDirectoryA(255,CurrentDir);
   debug_printf(CurrentDir);
   
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
	debug_printf("%c", ch);
	*line = ch;	
	line++;
     } while (ch != '\n');
   line--;
   *line = 0;
}

void main()
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
}

