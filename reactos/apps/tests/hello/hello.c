#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>


void main()
{  
   NtDisplayString("Shell Starting...\n");
   ExitThread(0);
}
