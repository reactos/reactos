/*
 * Load a device driver
 */
#define WIN32_NO_STATUS
#include <windows.h>
#include <stdlib.h>
#include <ntndk.h>

int wmain(int argc, WCHAR * argv[])
{
   NTSTATUS Status;
   UNICODE_STRING ServiceName;

   if (argc != 2)
   {
      wprintf(L"Usage: load <ServiceName>\n");
      return 0;
   }
   ServiceName.Length = (wcslen(argv[1]) + 52) * sizeof(WCHAR);
   ServiceName.Buffer = (LPWSTR)malloc(ServiceName.Length + sizeof(UNICODE_NULL));
   wsprintf(ServiceName.Buffer,
      L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\%S",
      argv[1]);
   wprintf(L"%s %u %d\n", ServiceName.Buffer, ServiceName.Length, wcslen(ServiceName.Buffer));
   Status = NtLoadDriver(&ServiceName);
   free(ServiceName.Buffer);
   if (!NT_SUCCESS(Status))
   {
      wprintf(L"Failed: %x\n", Status);
      return 1;
   }
   return 0;
}
