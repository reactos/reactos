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

   ServiceName.Length = (USHORT)((52 + wcslen(argv[1])) * sizeof(WCHAR));
   ServiceName.MaximumLength = ServiceName.Length + sizeof(UNICODE_NULL);
   ServiceName.Buffer = malloc(ServiceName.MaximumLength);
   wsprintf(ServiceName.Buffer,
      L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\%s",
      argv[1]);
   wprintf(L"Loading %wZ\n", &ServiceName);

   Status = NtLoadDriver(&ServiceName);
   free(ServiceName.Buffer);
   if (!NT_SUCCESS(Status))
   {
      wprintf(L"Failed: 0x%08lx\n", Status);
      return 1;
   }

   return 0;
}
