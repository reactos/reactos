#include <stdio.h>
#include <string.h>
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

HANDLE OutputHandle;
HANDLE InputHandle;

VOID WINAPI
ApcRoutine(PVOID Context,
	   PIO_STATUS_BLOCK IoStatus,
	   ULONG Reserved)
{
   printf("(apc.exe) ApcRoutine(Context %p)\n", Context);
}


int main(int argc, char* argv[])
{
   NTSTATUS Status;
   HANDLE FileHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING FileName = RTL_CONSTANT_STRING(L"\\C:\\a.txt");
   IO_STATUS_BLOCK IoStatus;
   CHAR Buffer[256];
   HANDLE EventHandle;
   LARGE_INTEGER off;

   AllocConsole();
   InputHandle = GetStdHandle(STD_INPUT_HANDLE);
   OutputHandle =  GetStdHandle(STD_OUTPUT_HANDLE);

   printf("APC test program\n");

   EventHandle = CreateEventW(NULL,
			      FALSE,
			      FALSE,
			      NULL);
   if (EventHandle == INVALID_HANDLE_VALUE)
     {
	printf("Failed to create event\n");
	return 0;
     }

   printf("Opening file\n");
   InitializeObjectAttributes(&ObjectAttributes,
			      &FileName,
			      0,
			      NULL,
			      NULL);

   printf("Creating file\n");
   FileHandle = CreateFileW(L"C:\\a.txt",
			    FILE_GENERIC_READ | FILE_GENERIC_WRITE,
			    0,
			    NULL,
			    OPEN_EXISTING,
			    FILE_FLAG_OVERLAPPED,
			    NULL);

   if (FileHandle == INVALID_HANDLE_VALUE)
     {

  printf("Open failed last err 0x%lx\n",GetLastError());
	return 0;
     }

     off.QuadPart = 0;

   printf("Reading file\n");
   Status = ZwReadFile(FileHandle,
			NULL,
			(PIO_APC_ROUTINE)ApcRoutine,
			(PVOID) 0xdeadbeef,
			&IoStatus,
			Buffer,
			256,//len
			&off ,//offset must exist if file was opened for asynch. i/o aka. OVERLAPPED
			NULL);

   if (!NT_SUCCESS(Status))
     {
	printf("Read failed status 0x%lx\n",Status);
     }
   printf("Waiting\n");
   WaitForSingleObjectEx(EventHandle, INFINITE, TRUE);
   printf("Returned from wait\n");
   ZwClose(FileHandle);
   printf("Program finished\n");
   return 0;
}

