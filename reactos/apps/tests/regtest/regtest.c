#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

HANDLE OutputHandle;
HANDLE InputHandle;

void dprintf(char* fmt, ...)
{
   va_list args;
   char buffer[255];

   va_start(args,fmt);
   vsprintf(buffer,fmt,args);
   WriteConsoleA(OutputHandle, buffer, strlen(buffer), NULL, NULL);
   va_end(args);
}


int main(int argc, char* argv[])
{
   HKEY hKey = NULL;
   DWORD dwDisposition;

   AllocConsole();
   InputHandle = GetStdHandle(STD_INPUT_HANDLE);
   OutputHandle =  GetStdHandle(STD_OUTPUT_HANDLE);

   dprintf ("RegCreateKeyExW:\n");
   RegCreateKeyExW (HKEY_LOCAL_MACHINE,
                    L"Test",
                    0,
                    L"",
                    REG_OPTION_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKey,
                    &dwDisposition);


   dprintf ("\nTests done...\n");

   return 0;
}

