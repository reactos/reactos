#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


void debug_printf(char* fmt, ...)
{
   va_list args;
   char buffer[255];
   HANDLE OutputHandle;
   
   AllocConsole();
   OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
   va_start(args,fmt);
   vsprintf(buffer,fmt,args);
   WriteConsoleA(OutputHandle, buffer, strlen(buffer), NULL, NULL);
   va_end(args);
}
