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
   DWORD dwError;

   AllocConsole();
   InputHandle = GetStdHandle(STD_INPUT_HANDLE);
   OutputHandle =  GetStdHandle(STD_OUTPUT_HANDLE);

   dprintf ("RegCreateKeyExW:\n");
   dwError = RegCreateKeyExW (HKEY_LOCAL_MACHINE,
                              L"Test",
                              0,
                              L"",
                              REG_OPTION_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKey,
                              &dwDisposition);

   dprintf ("dwError %x\n", dwError);
   if (dwError != ERROR_SUCCESS)
      return 0;

   dprintf ("dwDisposition %x\n", dwDisposition);

   dprintf ("RegSetValueExW:\n");
   dwError = RegSetValueExW (hKey,
                             L"TestValue",
                             0,
                             REG_SZ,
                             L"TestString",
                             20);

   dprintf ("dwError %x\n", dwError);
   if (dwError != ERROR_SUCCESS)
      return 0;

   dprintf ("RegCloseKey:\n");
   dwError = RegCloseKey (hKey);
   dprintf ("dwError %x\n", dwError);
   if (dwError != ERROR_SUCCESS)
      return 0;

   dprintf ("\n\n");

   hKey = NULL;

   dprintf ("RegCreateKeyExW:\n");
   dwError = RegCreateKeyExW (HKEY_LOCAL_MACHINE,
                              L"Test",
                              0,
                              L"",
                              REG_OPTION_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKey,
                              &dwDisposition);

   dprintf ("dwError %x\n", dwError);
   if (dwError != ERROR_SUCCESS)
      return 0;

   dprintf ("dwDisposition %x\n", dwDisposition);

#if 0
   dprintf ("RegQueryKeyExW:\n");

#endif

   dprintf ("RegCloseKey:\n");
   dwError = RegCloseKey (hKey);
   dprintf ("dwError %x\n", dwError);
   if (dwError != ERROR_SUCCESS)
      return 0;

   dprintf ("\nTests done...\n");

   return 0;
}

