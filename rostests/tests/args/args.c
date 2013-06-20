#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

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


int main(int argc, char* argv[])
{
   int i;

   AllocConsole();
   InputHandle = GetStdHandle(STD_INPUT_HANDLE);
   OutputHandle =  GetStdHandle(STD_OUTPUT_HANDLE);

   printf("GetCommandLineA() %s\n",GetCommandLineA());
   debug_printf("GetCommandLineA() %s\n",GetCommandLineA());
   debug_printf("argc %d\n", argc);
   for (i=0; i<argc; i++)
     {
        debug_printf("Argv[%d]: %x\n",i,argv[i]);
        debug_printf("Argv[%d]: '%s'\n",i,argv[i]);
     }
   return 0;
}

