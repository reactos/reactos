#include <ddk/ntddk.h>
#include <windows.h>
#include <stdarg.h>

BOOL KERNEL32_AnsiToUnicode(PWSTR DestStr,
			    LPCSTR SrcStr,
			    ULONG MaxLen)
{
   ULONG i=0;
   
   while (SrcStr[i] != 0 && i < MaxLen)
     {
	DestStr[i] = (WCHAR)SrcStr[i];
	i++;
     }
   if (i == MaxLen && SrcStr[i] != 0)
     {
	return(FALSE);
     }
   return(TRUE);
}
