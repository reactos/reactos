#include <windows.h>
#include <ddk/ntddk.h>
#include <stdarg.h>
#include <stdio.h>

VOID STDCALL OutputDebugStringA(LPCSTR lpOutputString)
{
   WCHAR DebugStringW[161];
   int i,j;
   i = 0;
   j = 0;
   while ( lpOutputString[i] != 0 )
     {
	while ( j < 160 && lpOutputString[i] != 0 )
	  {
	     DebugStringW[j] = (WCHAR)lpOutputString[i];		
	     i++;
	     j++;
	  }
	DebugStringW[j] = 0;
	OutputDebugStringW(DebugStringW);
	j = 0;
     }   
   return;
}

VOID STDCALL OutputDebugStringW(LPCWSTR lpOutputString)
{
   UNICODE_STRING UnicodeOutput;

   UnicodeOutput.Buffer = (WCHAR *)lpOutputString;
   UnicodeOutput.Length = lstrlenW(lpOutputString)*sizeof(WCHAR);
   UnicodeOutput.MaximumLength = UnicodeOutput.Length;
	
   NtDisplayString(&UnicodeOutput);
}

void dprintf(char* fmt, ...)
{
   va_list va_args;
   char buffer[255];
   
   va_start(va_args,fmt);
   vsprintf(buffer,fmt,va_args);
   OutputDebugStringA(buffer);
   va_end(fmt);
}

void aprintf(char* fmt, ...)
{
   va_list va_args;
   char buffer[255];
   
   va_start(va_args,fmt);
   vsprintf(buffer,fmt,va_args);
   OutputDebugStringA(buffer);
   va_end(fmt);
}
