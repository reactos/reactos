/* $Id: print.c,v 1.3 2002/10/29 04:45:54 rex Exp $ */
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
