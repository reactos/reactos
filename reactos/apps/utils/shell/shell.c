
#include <internal/mmhal.h>

#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>

void debug_printf(char* fmt, ...)
{
   va_list args;
   char buffer[255];
   
   va_start(args,fmt);
   vsprintf(buffer,fmt,args);
   OutputDebugStringA(buffer);
   va_end(args);
}

HANDLE KeyboardHandle;

void ExecuteCd(char* cmdline)
{
   UCHAR Buffer[255];
   
   debug_printf("ExecuteCd(cmdline %s)\n",cmdline);
   
   if (cmdline[0] != '\\' &&
       cmdline[1] != ':')
     {
	GetCurrentDirectoryA(255,Buffer);
     }
   else
     {
	Buffer[0] = 0;
     }
   debug_printf("Buffer %s\n",Buffer);
   
   lstrcatA(Buffer,cmdline);
   
   debug_printf("Buffer %s\n",Buffer);
   
   if (Buffer[lstrlenA(Buffer)-1] != '\\')
     {
	lstrcatA(Buffer,"\\");
     }
   debug_printf("Buffer %s\n",Buffer);
   
   SetCurrentDirectoryA(Buffer);
}

void ExecuteDir(char* cmdline)
{
        HANDLE shandle;
        WIN32_FIND_DATA FindData;

        shandle = FindFirstFile("*",&FindData);

        if (shandle==INVALID_HANDLE_VALUE)
        {
                return;
        }
        do
        {
                debug_printf("Scanning %s\n",FindData.cFileName);
        } while(FindNextFile(shandle,&FindData));
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
   while (ReadFile(FileHandle,
		   &c,
		   1,
		   &Result,
		   NULL))
     {
	debug_printf("%c",c);
	c = 0;
     }
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
   
   if (line[1] == ':' && line[2] == 0)
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
   
//   debug_printf("cmd '%s' tail '%s'\n",cmd,tail);
   
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
   if (ExecuteProcess(cmd,tail))
     {
        debug_printf("Done ExecuteProcess\n");
	return;
     }
   debug_printf("Unknown command\n");
}

void ReadLine(char* line)
{
   KEY_EVENT_RECORD KeyEvent;
   DWORD Result;
   UCHAR CurrentDir[255];
   
   GetCurrentDirectoryA(255,CurrentDir);
   debug_printf(CurrentDir);
   
   do
     {
	ReadFile(KeyboardHandle,
		 &KeyEvent,
		 sizeof(KEY_EVENT_RECORD),
		 &Result,
		 NULL);
	if (KeyEvent.bKeyDown && KeyEvent.AsciiChar != 0)
	  {
	     debug_printf("%c",KeyEvent.AsciiChar);
	     *line = KeyEvent.AsciiChar;
	     line++;
	  }
     } while (!(KeyEvent.bKeyDown && KeyEvent.AsciiChar == '\n'));
   ReadFile(KeyboardHandle,
	    &KeyEvent,
	    sizeof(KEY_EVENT_RECORD),
	    &Result,
	    NULL);
   line--;
   *line = 0;
}

void main()
{
   static char line[255];
   
   KERNEL32_Init();
   
   NtDisplayString("Shell Starting...\n");
      
   KeyboardHandle = CreateFile("Keyboard",
			       FILE_GENERIC_READ,
			       0,
			       NULL,
			       OPEN_EXISTING,
			       0,
			       NULL);
   
   SetCurrentDirectoryA("C:\\");
   
   for(;;)
     {
	ReadLine(line);
	ExecuteCommand(line);
     }
}
