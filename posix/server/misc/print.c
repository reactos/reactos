/* $Id: print.c,v 1.1 2002/04/10 21:30:22 ea Exp $ */
#define UNICODE
#include <windows.h>
VOID STDCALL debug_print (LPWSTR Template, ...)
{
   WCHAR   Buffer [1024];
   va_list ArgumentPointer;
   
   va_start(ArgumentPointer, Template);
   vswprintf(Buffer, Template, ArgumentPointer);
   va_end(ArgumentPointer);
#ifdef __PSXSS_ON_W32__   
   _putws (Buffer);
#else
#error TODO
#endif
}
/* EOF */
