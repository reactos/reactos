#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>


void main()
{  
   NtDisplayString("Hello world\n");
   ExitProcess(0);
}
