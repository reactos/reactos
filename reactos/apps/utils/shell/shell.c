
#include <internal/mmhal.h>

#include <ddk/ntddk.h>
#include <stdarg.h>

void debug_printf(char* fmt, ...)
{
   va_list args;
   char buffer[255];
   
   va_start(args,fmt);
   vsprintf(buffer,fmt,args);
   OutputDebugString(buffer);
   va_end(args);
}

void main()
{
   KEY_EVENT_RECORD KeyEvent[2];
   HANDLE FileHandle;
   DWORD Result;
   HANDLE DefaultHeap;
   PVOID Buffer;

   NtDisplayString("Simple Shell Starting...\n");
   
//   DefaultHeap = HeapCreate(0,1024*1024,1024*1024);
//   Buffer = HeapAlloc(DefaultHeap,0,1024);
   
   FileHandle = CreateFile("\\Device\\Keyboard",
			   FILE_GENERIC_READ,
			   0,
			   NULL,
			   OPEN_EXISTING,
			   0,
			   NULL);
   
NtDisplayString("#1\n");
   debug_printf("C:\\");
NtDisplayString("#2\n");
   for(;;)
     {
	ReadFile(FileHandle,
		 &KeyEvent,
		 sizeof(KeyEvent),
		 &Result,
		 NULL);
NtDisplayString("#3\n");
	debug_printf("%c",KeyEvent[0].AsciiChar);
NtDisplayString("#4\n");
     }
}
