/*
 * partinfo - partition info program
 */

#include <windows.h>
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

int main (int argc, char *argv[])
{
  HANDLE hDisk;
  DWORD dwRead;
  DWORD i;
  char *Buffer;
  DRIVE_LAYOUT_INFORMATION *LayoutBuffer;
  DISK_GEOMETRY DiskGeometry;

  hDisk = CreateFile("\\\\.\\PHYSICALDRIVE0",
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


  /* get drive geometry */
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
      printf("DeviceIoControl failed! Error: %u\n",
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

#ifdef DUMP_SIZE_INFO
  printf("\nsizeof(PARTITION_INFORMATION): %lu\n", sizeof(PARTITION_INFORMATION));
#endif

  free(Buffer);

  return 0;
}
