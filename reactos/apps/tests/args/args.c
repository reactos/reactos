#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>

HANDLE OutputHandle;
HANDLE InputHandle;

void debug_printf(char* fmt, ...)
{
   va_list args;
   char buffer[255];
   
   va_start(args,fmt);
   vsprintf(buffer,fmt,args);
   WriteConsoleA(OutputHandle, buffer, strlen(buffer), NULL, NULL);
   va_end(args);
}


void main(int argc, char* argv[])
{
   int i;
   
   AllocConsole();
   InputHandle = GetStdHandle(STD_INPUT_HANDLE);
   OutputHandle =  GetStdHandle(STD_OUTPUT_HANDLE);

   debug_printf("GetCommandLineA() %s\n",GetCommandLineA());
   for (i=0; i<argc; i++)
     {
	debug_printf("Args: %x\n",argv[i]);
	debug_printf("Args: '%s'\n",argv[i]);
     }
}

