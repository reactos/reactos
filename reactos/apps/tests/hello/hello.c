#include <stdio.h>
#include <ddk/ntddk.h>

int main(int argc, char* argv[])
{  
   UNICODE_STRING UnicodeString;
   RtlInitUnicodeString(&UnicodeString,L"Hello world\n");
   NtDisplayString(&UnicodeString);
   return(0);
}
