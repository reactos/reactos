/*
 * isotest - display cdrom information
 */

#include <ddk/ntddk.h>
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>


void HexDump(char *buffer, ULONG size)
{
  ULONG offset = 0;
  unsigned char *ptr;

  while (offset < (size & ~15))
    {
      ptr = (unsigned char*)((ULONG)buffer + offset);
      printf("%08lx  %02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx-%02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx",
	     offset,
	     ptr[0],
	     ptr[1],
	     ptr[2],
	     ptr[3],
	     ptr[4],
	     ptr[5],
	     ptr[6],
	     ptr[7],
	     ptr[8],
	     ptr[9],
	     ptr[10],
	     ptr[11],
	     ptr[12],
	     ptr[13],
	     ptr[14],
	     ptr[15]);

      printf("  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
	     isprint(ptr[0])?ptr[0]:'.',
	     isprint(ptr[1])?ptr[1]:'.',
	     isprint(ptr[2])?ptr[2]:'.',
	     isprint(ptr[3])?ptr[3]:'.',
	     isprint(ptr[4])?ptr[4]:'.',
	     isprint(ptr[5])?ptr[5]:'.',
	     isprint(ptr[6])?ptr[6]:'.',
	     isprint(ptr[7])?ptr[7]:'.',
	     isprint(ptr[8])?ptr[8]:'.',
	     isprint(ptr[9])?ptr[9]:'.',
	     isprint(ptr[10])?ptr[10]:'.',
	     isprint(ptr[11])?ptr[11]:'.',
	     isprint(ptr[12])?ptr[12]:'.',
	     isprint(ptr[13])?ptr[13]:'.',
	     isprint(ptr[14])?ptr[14]:'.',
	     isprint(ptr[15])?ptr[15]:'.');

      offset += 16;
    }

  ptr = (unsigned char*)((ULONG)buffer + offset);
  if (offset < size)
    {
      printf("%08lx ", offset);
      while (offset < size)
	{
	  printf(" %02hx", *ptr);
	  offset++;
	  ptr++;
	}
    }

  printf("\n\n");
}


#ifndef EVENT_ALL_ACCESS
#define EVENT_ALL_ACCESS	(0x1f0003L)
#endif

BOOL
ReadBlock(HANDLE FileHandle,
	  PVOID Buffer,
	  PLARGE_INTEGER Offset,
	  ULONG Length,
	  PULONG BytesRead)
{
  IO_STATUS_BLOCK IoStatusBlock;
  OBJECT_ATTRIBUTES ObjectAttributes;
  NTSTATUS Status;
  HANDLE EventHandle;

  InitializeObjectAttributes(&ObjectAttributes,
			     NULL, 0, NULL, NULL);

  Status = NtCreateEvent(&EventHandle,
			 EVENT_ALL_ACCESS,
			 &ObjectAttributes,
			 TRUE,
			 FALSE);
  if (!NT_SUCCESS(Status))
    {
      printf("NtCreateEvent() failed\n");
      return(FALSE);
    }

  Status = NtReadFile(FileHandle,
		      EventHandle,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      Buffer,
		      Length,
		      Offset,
		      NULL);
  if (Status == STATUS_PENDING)
    {
      NtWaitForSingleObject(EventHandle, FALSE, NULL);
      Status = IoStatusBlock.Status;
    }

  NtClose(EventHandle);

  if (Status != STATUS_PENDING && BytesRead != NULL)
    {
      *BytesRead = IoStatusBlock.Information;
    }
  if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
    {
      printf("ReadBlock() failed (Status: %lx)\n", Status);
      return(FALSE);
    }

  return(TRUE);
}



int main (int argc, char *argv[])
{
  HANDLE hDisk;
  DWORD dwRead;
  DWORD i;
  char *Buffer;
  CHAR Filename[80];
  LARGE_INTEGER FilePosition;

  if (argc != 2)
    {
      printf("Usage: isotest [Drive:]\n");
      return 0;
    }

  strcpy(Filename, "\\\\.\\");
  strcat(Filename, argv[1]);

  hDisk = CreateFile(Filename,
		     GENERIC_READ,
		     FILE_SHARE_READ | FILE_SHARE_WRITE,
		     NULL,
		     OPEN_EXISTING,
		     0,
		     NULL);
  if (hDisk == INVALID_HANDLE_VALUE)
    {
      printf("CreateFile(): Invalid disk handle!\n");
      return 0;
    }

  Buffer = (char*)malloc(2048);
  if (Buffer == NULL)
    {
      CloseHandle(hDisk);
      printf("Out of memory!\n");
      return 0;
    }
  memset(Buffer, 0, 2048);


  FilePosition.QuadPart = 16 * 2048;
#if 0
  SetLastError(NO_ERROR);
  SetFilePointer(hDisk,
		 FilePosition.u.LowPart,
		 &FilePosition.u.HighPart,
		 FILE_BEGIN);
  if (GetLastError() != NO_ERROR)
    {
      CloseHandle(hDisk);
      free(Buffer);
      printf("SetFilePointer() failed!\n");
      return 0;
    }

  if (ReadFile(hDisk,
	       Buffer,
	       2048,
	       &dwRead,
	       NULL) == FALSE)
    {
      CloseHandle(hDisk);
      free(Buffer);
      printf("ReadFile() failed!\n");
      return 0;
    }
#endif

  if (ReadBlock(hDisk,
		Buffer,
		&FilePosition,
		2048,
		&dwRead) == FALSE)
    {
      CloseHandle(hDisk);
      free(Buffer);
#if 0
      printf("ReadBlock() failed!\n");
#endif
      return 0;
    }

  HexDump(Buffer, 128);

  CloseHandle(hDisk);

  free(Buffer);

  return 0;
}

/* EOF */
