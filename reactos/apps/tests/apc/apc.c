#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <ddk/ntddk.h>

HANDLE OutputHandle;
HANDLE InputHandle;

VOID STDCALL 
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
   UNICODE_STRING FileName;
   IO_STATUS_BLOCK IoStatus;
   CHAR Buffer[256];
   HANDLE EventHandle;
   
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
   RtlInitUnicodeString(&FileName,
			      L"\\C:\\a.txt");			      
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
	printf("Open failed\n");
	return 0;
     }
   printf("Reading file\n");
   Status = ZwReadFile(FileHandle,
			NULL,
			(PIO_APC_ROUTINE)ApcRoutine,
			(PVOID)0xdeadbeef,
			&IoStatus,
			Buffer,
			256,
			NULL,
			NULL);
   if (!NT_SUCCESS(Status))
     {
	printf("Read failed\n");
     }
   printf("Waiting\n");
   WaitForSingleObjectEx(EventHandle, INFINITE, TRUE);
   printf("Returned from wait\n");
   ZwClose(FileHandle);
   printf("Program finished\n");
   return 0;
}

