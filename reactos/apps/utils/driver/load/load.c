/*
 * Load a device driver
 */ 
#include <windows.h>
#include <ntos/zw.h>

int
main(int argc, char *argv[])
{
   NTSTATUS Status;
   UNICODE_STRING ServiceName;

   if (argc != 2)
   {
      printf("Usage: load <ServiceName>\n");
      return 0;
   }
   ServiceName.Length = (strlen(argv[1]) + 52) * sizeof(WCHAR);
   ServiceName.Buffer = (LPWSTR)malloc(ServiceName.Length + sizeof(UNICODE_NULL));
   wsprintf(ServiceName.Buffer,
      L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\%S",
      argv[1]);
   wprintf(L"%s %d %d\n", ServiceName.Buffer, ServiceName.Length, wcslen(ServiceName.Buffer));
   Status = NtLoadDriver(&ServiceName);
   free(ServiceName.Buffer);
   if (!NT_SUCCESS(Status))
   {
      printf("Failed: %X\n", Status);
      return 1;
   }
   return 0;
}
