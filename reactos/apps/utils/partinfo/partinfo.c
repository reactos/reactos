/*
 * partinfo - partition info program
 */

#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>
//#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>

//#define DUMP_DATA
#define DUMP_SIZE_INFO


#ifdef DUMP_DATA
void HexDump(char *buffer, ULONG size)
{
  ULONG offset = 0;
  unsigned char *ptr;

  while (offset < (size & ~15))
    {
      ptr = (unsigned char*)((ULONG)buffer + offset);
      printf("%08lx  %02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx-%02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx\n",
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
      offset += 16;
    }

  ptr = (unsigned char*)((ULONG)buffer + offset);
  printf("%08lx ", offset);
  while (offset < size)
    {
      printf(" %02hx", *ptr);
      offset++;
      ptr++;
    }

  printf("\n\n\n");
}
#endif


void Usage(void)
{
  puts("Usage: partinfo <drive number>");
}


int main (int argc, char *argv[])
{
  HANDLE hDisk;
  DWORD dwRead;
  DWORD i;
  char *Buffer;
  DRIVE_LAYOUT_INFORMATION *LayoutBuffer;
  DISK_GEOMETRY DiskGeometry;
  ULONG ulDrive;
  CHAR DriveName[40];
  SYSTEM_DEVICE_INFORMATION DeviceInfo;
  NTSTATUS Status;

  if (argc != 2)
    {
      Usage();
      return(0);
    }

  ulDrive = strtoul(argv[1], NULL, 10);
  if (errno != 0)
    {
      printf("Error: Malformed drive number\n", errno);
      return(0);
    }

  /* Check drive number */
  Status = NtQuerySystemInformation(SystemDeviceInformation,
				    &DeviceInfo,
				    sizeof(SYSTEM_DEVICE_INFORMATION),
				    &i);
  if (!NT_SUCCESS(Status))
    {
      printf("NtQuerySystemInformation() failed (Status %lx)\n", Status);
      return(0);
    }

  if (DeviceInfo.NumberOfDisks == 0)
    {
      printf("No disk drive installed!\n");
      return(0);
    }

  if (ulDrive >= DeviceInfo.NumberOfDisks)
    {
      printf("Invalid disk drive number! Valid drive numbers [0-%lu]\n",
	     DeviceInfo.NumberOfDisks-1);
      return(0);
    }

  /* Build full drive name */
  sprintf(DriveName, "\\\\.\\PHYSICALDRIVE%lu", ulDrive);

  /* Open drive */
  hDisk = CreateFile(DriveName,
		     GENERIC_READ,
		     FILE_SHARE_READ | FILE_SHARE_WRITE,
		     NULL,
		     OPEN_EXISTING,
		     0,
		     NULL);
  if (hDisk == INVALID_HANDLE_VALUE)
    {
      printf("Invalid disk handle!");
      return 0;
    }

  /* Get drive geometry */
  if (!DeviceIoControl(hDisk,
		       IOCTL_DISK_GET_DRIVE_GEOMETRY,
		       NULL,
		       0,
		       &DiskGeometry,
		       sizeof(DISK_GEOMETRY),
		       &dwRead,
		       NULL))
    {
      CloseHandle(hDisk);
      printf("DeviceIoControl failed! Error: %u\n",
	     GetLastError());
      free(Buffer);
      return 0;
    }

#ifdef DUMP_DATA
  HexDump((char*)&DiskGeometry, dwRead);
#endif
  printf("Drive number: %lu\n", ulDrive);
  printf("Cylinders: %I64u\nMediaType: %lx\nTracksPerCylinder: %lu\n"
	 "SectorsPerTrack: %lu\nBytesPerSector: %lu\n\n",
	 DiskGeometry.Cylinders.QuadPart,
	 DiskGeometry.MediaType,
	 DiskGeometry.TracksPerCylinder,
	 DiskGeometry.SectorsPerTrack,
	 DiskGeometry.BytesPerSector);


  Buffer = (char*)malloc(8192);
  if (Buffer == NULL)
    {
      CloseHandle(hDisk);
      printf("Out of memory!");
      return 0;
    }
  memset(Buffer, 0, 8192);

  if (!DeviceIoControl(hDisk,
		       IOCTL_DISK_GET_DRIVE_LAYOUT,
		       NULL,
		       0,
		       Buffer,
		       8192,
		       &dwRead,
		       NULL))
    {
      CloseHandle(hDisk);
      printf("DeviceIoControl(IOCTL_DISK_GET_DRIVE_LAYOUT) failed! Error: %u\n",
	     GetLastError());
      free(Buffer);
      return 0;
    }

  CloseHandle(hDisk);

#ifdef DUMP_DATA
  HexDump(Buffer, dwRead);
#endif

  LayoutBuffer = (DRIVE_LAYOUT_INFORMATION*)Buffer;

  printf("Partitions %u  Signature %x\n",
	 LayoutBuffer->PartitionCount,
	 LayoutBuffer->Signature);

  for (i = 0; i < LayoutBuffer->PartitionCount; i++)
    {
      printf(" %d: nr: %d boot: %1x type: %x start: 0x%I64x count: 0x%I64x\n",
	     i,
	     LayoutBuffer->PartitionEntry[i].PartitionNumber,
	     LayoutBuffer->PartitionEntry[i].BootIndicator,
	     LayoutBuffer->PartitionEntry[i].PartitionType,
	     LayoutBuffer->PartitionEntry[i].StartingOffset.QuadPart,
	     LayoutBuffer->PartitionEntry[i].PartitionLength.QuadPart);
    }

  free(Buffer);

  return 0;
}
