#include <windows.h>
#include <stdarg.h>

VOID STDCALL OutputDebugStringA(LPCSTR lpOutputString)
{
   NtDisplayString(lpOutputString);
}

void dprintf(char* fmt, ...)
{
   va_list va_args;
   char buffer[255];
   
   va_start(va_args,fmt);
   vsprintf(buffer,fmt,va_args);
   OutputDebugString(buffer);
   va_end(fmt);
}

void aprintf(char* fmt, ...)
{
   va_list va_args;
   char buffer[255];
   
   va_start(va_args,fmt);
   vsprintf(buffer,fmt,va_args);
   OutputDebugString(buffer);
   va_end(fmt);
}
