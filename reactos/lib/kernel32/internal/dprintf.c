#include <ddk/ntddk.h>
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

ULONG DbgService( ULONG Service, PVOID Context1, PVOID Context2 );

VOID STDCALL OutputDebugStringA(LPCSTR lpOutputString)
{
  ANSI_STRING AnsiString;
  AnsiString.Buffer = lpOutputString;
  AnsiString.Length = AnsiString.MaximumLength = lstrlenA( lpOutputString );
  DbgService( 1, &AnsiString, NULL );
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
     DbgService( 1, &AnsiString, NULL );
}






