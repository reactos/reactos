#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

HANDLE OutputHandle;
HANDLE InputHandle;

void debug_printf(char* fmt, ...)
{
   va_list args;
   char buffer[255];

   va_start(args,fmt);
   vsprintf(buffer,fmt,args);
   WriteConsoleA(OutputHandle, buffer, strlen(buffer), NULL, NULL);
   va_end(args);
}

VOID STDCALL ApcRoutine(PVOID Context,
			PIO_STATUS_BLOCK IoStatus,
			ULONG Reserved)
{
   printf("(apc.exe) ApcRoutine(Context %x)\n", Context);
}

void main(int argc, char* argv[])
{
   int i;
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
	return;
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
	return;
     }
   printf("Reading file\n");
   Status = ZwReadFile(FileHandle,
			NULL,
			ApcRoutine,
			0xdeadbeef,
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
   for(;;);
}

