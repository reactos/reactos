#include <ddk/ntddk.h>
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

/*
 * NOTE: Don't call DbgService()!
 *       It's a ntdll internal function and is NOT exported!
 */

VOID STDCALL OutputDebugStringA(LPCSTR lpOutputString)
{
   DbgPrint( (PSTR)lpOutputString );
}

VOID STDCALL OutputDebugStringW(LPCWSTR lpOutputString)
{
   UNICODE_STRING UnicodeOutput;
   ANSI_STRING AnsiString;
   char buff[512];

   UnicodeOutput.Buffer = (WCHAR *)lpOutputString;
   UnicodeOutput.Length = lstrlenW(lpOutputString)*sizeof(WCHAR);
   UnicodeOutput.MaximumLength = UnicodeOutput.Length;
   AnsiString.Buffer = buff;
   AnsiString.MaximumLength = 512;
   AnsiString.Length = 0;
   if( UnicodeOutput.Length > 512 )
     UnicodeOutput.Length = 512;
   if( NT_SUCCESS( RtlUnicodeStringToAnsiString( &AnsiString, &UnicodeOutput, FALSE ) ) )
     DbgPrint( AnsiString.Buffer );
}






