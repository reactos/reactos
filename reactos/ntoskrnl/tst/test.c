/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/tst/test.c
 * PURPOSE:         Kernel regression tests
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                28/05/98: Created
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/string.h>
#include <internal/mm.h>
#include <internal/mmhal.h>
#include <internal/i386/segment.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

#include <in.h>

#define IDE_SECTOR_SZ 512

/* FUNCTIONS ****************************************************************/


VOID ExExecuteShell(VOID)
{
   HANDLE ShellHandle;
   HANDLE ThreadHandle;   
   PVOID BaseAddress;
   HANDLE SectionHandle;
   OBJECT_ATTRIBUTES attr;
   HANDLE hfile;
   ANSI_STRING afilename;
   UNICODE_STRING ufilename;
   LARGE_INTEGER SectionOffset;
   ULONG Size, StackSize;
   CONTEXT Context;
   NTSTATUS Status;
   ULONG Temp,BytesWritten;
   
   ZwCreateProcess(&ShellHandle,
		   PROCESS_ALL_ACCESS,
		   NULL,
		   SystemProcessHandle,
		   FALSE,
		   NULL,
		   NULL,
		   NULL);

   RtlInitAnsiString(&afilename,"\\??\\C:\\reactos\\system\\shell.bin");
   RtlAnsiStringToUnicodeString(&ufilename,&afilename,TRUE);
   InitializeObjectAttributes(&attr,&ufilename,0,NULL,NULL);
   Status = ZwOpenFile(&hfile,FILE_ALL_ACCESS,&attr,NULL,0,0);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Failed to open file\n");
        return;
     }
   
  ZwCreateSection(&SectionHandle,
		   SECTION_ALL_ACCESS,
		   NULL,
		   NULL,
		   PAGE_READWRITE,
		   MEM_COMMIT,
		   hfile);
   
   BaseAddress = (PVOID)0x10000;
   LARGE_INTEGER_QUAD_PART(SectionOffset) = 0;
   Size = 0x20000;
   ZwMapViewOfSection(SectionHandle,
		      ShellHandle,
		      &BaseAddress,
		      0,
                      Size,
		      &SectionOffset,
		      &Size,
		      0,
		      MEM_COMMIT,
		      PAGE_READWRITE);
   
   memset(&Context,0,sizeof(CONTEXT));
   
   Context.SegSs = USER_DS;
   Context.Esp = 0xf000 - 12;
   Context.EFlags = 0x202;
   Context.SegCs = USER_CS;
   Context.Eip = 0x10000;
   Context.SegDs = USER_DS;
   Context.SegEs = USER_DS;
   Context.SegFs = USER_DS;
   Context.SegGs = USER_DS;
   
   BaseAddress = 0x1000;
   StackSize = 0xe000;
   ZwAllocateVirtualMemory(ShellHandle,
			   &BaseAddress,
			   0,
			   &StackSize,
			   MEM_COMMIT,
			   PAGE_READWRITE);
   
   Temp = 0xf000 - 4;
   ZwWriteVirtualMemory(ShellHandle,
			0xf000 - 8,
			&Temp,
			sizeof(Temp),
			&BytesWritten);
   
   ZwCreateThread(&ThreadHandle,
		  THREAD_ALL_ACCESS,
		  NULL,
		  ShellHandle,
		  NULL,
		  &Context,
		  NULL,
		  FALSE);
}

/* IDE TEST STUFF ***********************************************************/

typedef struct _BOOT_PARAMETERS {
  WORD BytesBerSector;
  BYTE SectorsPerAllocationUnit;
  WORD ReservedSectorCount;
  BYTE FATCount;
  WORD RootDirEntryCount;
  WORD TotalSectorCount;
  BYTE MediaDescriptor;
  WORD SectorsPerFAT;
  WORD SectorsPerTrack;
  WORD HeadCount;
  DWORD HiddenSectorCount;
  DWORD TotalSectorsInLogicalVolume;
} __attribute__ ((packed)) BOOT_PARAMETERS, __attribute__ ((packed)) *PBOOT_PARAMETERS;

typedef struct _BOOT_BLOCK {
  char JumpInstruction[3];
  char OEMName[8];
  BOOT_PARAMETERS BootParameters;
  BYTE DriveNumber;
  BYTE Reserved1;
  BYTE ExtBootSignatureRecord;
  DWORD BinaryVolumeID;
  char VolumeLabel[11];
  char Reserved2[8]; 
  char Bootstrap[512 - 61];
} __attribute__ ((packed)) BOOT_BLOCK, __attribute__ ((packed)) *PBOOT_BLOCK;

typedef struct _ROOT_DIR_ENTRY {
  char Filename[8];
  char Extension[3];
  char FileAttribute;
  char Reserved1[10];
  WORD ModifiedTime;
  WORD ModifiedDate;
  WORD StartingCluster;
  DWORD FileSize;
} __attribute__ ((packed)) ROOT_DIR_ENTRY;

#define ENTRIES_PER_BLOCK (512 / sizeof(ROOT_DIR_ENTRY))

void TstIDERead(void)
{
  BOOLEAN TestFailed;
  int Entry, i, j, BufferSize;
  HANDLE FileHandle;
  NTSTATUS Status;
  LARGE_INTEGER BlockOffset;
  ANSI_STRING AnsiDeviceName;
  UNICODE_STRING UnicodeDeviceName;
  OBJECT_ATTRIBUTES ObjectAttributes;
  char *SectorBuffer;
  PBOOT_BLOCK BootBlock;
  ROOT_DIR_ENTRY DirectoryBlock[ENTRIES_PER_BLOCK];

  DbgPrint("IDE Read Test\n");
  TestFailed = FALSE;
  BufferSize = IDE_SECTOR_SZ * 300;
  SectorBuffer = ExAllocatePool(NonPagedPool, BufferSize);

    /*  open the first partition  */
  DbgPrint("Opening Partition1\n");
  RtlInitAnsiString(&AnsiDeviceName, "\\Device\\HardDrive0\\Partition1");
  RtlAnsiStringToUnicodeString(&UnicodeDeviceName, &AnsiDeviceName, TRUE);
  InitializeObjectAttributes(&ObjectAttributes,
                             &UnicodeDeviceName, 
                             0,
                             NULL,
                             NULL);
  Status = ZwOpenFile(&FileHandle, 
                      FILE_ALL_ACCESS, 
                      &ObjectAttributes, 
                      NULL, 
                      0, 
                      FILE_SYNCHRONOUS_IO_ALERT);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to open partition1\n");
      TestFailed = TRUE;
    }

    /*  Read the boot block  */
  if (!TestFailed)
    {
      DbgPrint("Reading boot block from Partition1\n");
      RtlZeroMemory(SectorBuffer, sizeof(SectorBuffer));
      Status = ZwReadFile(FileHandle,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          SectorBuffer,
                          512,
                          0,
                          0);
      if (!NT_SUCCESS(Status))
        {
          DbgPrint("Failed to read book block from partition1 status:%x\n", Status);
          TestFailed = TRUE;
        }
    }  

    /* %%% Check for valid boot block signature  */

    /*  Spew info about boot block  */
  if (!TestFailed)
    {
      BootBlock = (PBOOT_BLOCK) SectorBuffer;
      DbgPrint("boot block on Partition1:\n");
      DbgPrint("  OEM Name: %.8s  Bytes/Sector:%d Sectors/Cluster:%d\n",
               BootBlock->OEMName,
               BootBlock->BootParameters.BytesBerSector,
               BootBlock->BootParameters.SectorsPerAllocationUnit);
      DbgPrint("  ReservedSectors:%d FATs:%d RootDirEntries:%d\n",
               BootBlock->BootParameters.ReservedSectorCount,
               BootBlock->BootParameters.FATCount,
               BootBlock->BootParameters.RootDirEntryCount);
      DbgPrint("  TotalSectors:%d MediaDescriptor:%d Sectors/FAT:%d\n",
               BootBlock->BootParameters.TotalSectorCount,
               BootBlock->BootParameters.MediaDescriptor,
               BootBlock->BootParameters.SectorsPerFAT);
      DbgPrint("  Sectors/Track:%d Heads:%d HiddenSectors:%d\n",
               BootBlock->BootParameters.SectorsPerTrack,
               BootBlock->BootParameters.HeadCount,
               BootBlock->BootParameters.HiddenSectorCount);
      DbgPrint("  VolumeLabel:%.11s\n", BootBlock->VolumeLabel);
    }

    /*  Read the first root directory block */
  if (!TestFailed)
    {
      DbgPrint("Reading rootdir block from Partition1\n");
      LARGE_INTEGER_QUAD_PART(BlockOffset) =  
        BootBlock->BootParameters.ReservedSectorCount * 512 +
        BootBlock->BootParameters.FATCount * 
        BootBlock->BootParameters.SectorsPerFAT * 512;
      Status = ZwReadFile(FileHandle,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          DirectoryBlock,
                          sizeof(DirectoryBlock),
                          &BlockOffset,
                          0);
      if (!NT_SUCCESS(Status))
        {
          DbgPrint("Failed to read root directory block from partition1\n");
          TestFailed = TRUE;
        }
    }  

    /*  Print the contents  */
  if (!TestFailed)
    {
      for (Entry = 0; Entry < ENTRIES_PER_BLOCK; Entry++)
        {
          switch (DirectoryBlock[Entry].Filename[0])
            {
            case 0x00:
              DbgPrint("End of Directory.\n");
              Entry = ENTRIES_PER_BLOCK;
              break;

            case 0x05:
              DbgPrint("  FILE: %c%.7s.%.3s ATTR:%x Time:%04x Date:%04x offset:%d size:%d\n",
                       0xe5,
                       &DirectoryBlock[Entry].Filename[1],
                       DirectoryBlock[Entry].Extension,
                       DirectoryBlock[Entry].FileAttribute,
                       DirectoryBlock[Entry].ModifiedTime,
                       DirectoryBlock[Entry].ModifiedDate,
                       DirectoryBlock[Entry].StartingCluster,
                       DirectoryBlock[Entry].FileSize);
              break;

            case 0x2e:
              DbgPrint("  ALIAS: %.8s ATTR:%x Time:%04x Date:%04x offset:%d size:%d\n",
                       &DirectoryBlock[Entry].Filename[1],
                       DirectoryBlock[Entry].FileAttribute,
                       DirectoryBlock[Entry].ModifiedTime,
                       DirectoryBlock[Entry].ModifiedDate,
                       DirectoryBlock[Entry].StartingCluster,
                       DirectoryBlock[Entry].FileSize);
              break;

            case 0xe5:
              break;

            default:
              DbgPrint("  FILE: %.8s.%.3s ATTR:%x Time:%04x Date:%04x offset:%d size:%d\n",
                       DirectoryBlock[Entry].Filename,
                       DirectoryBlock[Entry].Extension,
                       DirectoryBlock[Entry].FileAttribute,
                       DirectoryBlock[Entry].ModifiedTime,
                       DirectoryBlock[Entry].ModifiedDate,
                       DirectoryBlock[Entry].StartingCluster,
                       DirectoryBlock[Entry].FileSize);
              break;
            }
        }
    }

    /*  Execute a multiblock disk read/write test  */
  if (!TestFailed)
    {
      DbgPrint("Reading data from blocks 10000-4 from Partition1\n");
      RtlFillMemory(SectorBuffer, BufferSize, 0xea);
      LARGE_INTEGER_QUAD_PART(BlockOffset) = 10000 * IDE_SECTOR_SZ;
      Status = ZwReadFile(FileHandle,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          SectorBuffer,
                          BufferSize,
                          &BlockOffset,
                          0);
      if (!NT_SUCCESS(Status))
        {
          DbgPrint("Failed to read %d bytes of data to offset 10000 from partition1 status:%x\n", 
                   BufferSize,
                   Status);
          TestFailed = TRUE;
        }
      else
        {
          DbgPrint("%d bytes read from offset 10000 of partition1\n", BufferSize);
          for (j = 0; j < BufferSize; j += IDE_SECTOR_SZ)
            {
              DbgPrint("%02x", (unsigned char)SectorBuffer[j]);
              SectorBuffer[j]++;
              if (((j / IDE_SECTOR_SZ + 1) % 30) == 0)
                {
                  DbgPrint("\n");
                }
            }
          DbgPrint("\n");
//RtlZeroMemory(SectorBuffer, BufferSize);
          Status = ZwWriteFile(FileHandle,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               SectorBuffer,
                               BufferSize,
                               &BlockOffset,
                               0);
          if (!NT_SUCCESS(Status))
            {
              DbgPrint("Failed to write %d bytes of data to offset 10000 of partition1 status:%x\n", 
                       BufferSize, 
                       Status);
              TestFailed = TRUE;
            }
          else
            {
              DbgPrint("%d bytes written\n", BufferSize);
            }
        }
    }  

  if (FileHandle != NULL)
    {
      ZwClose(FileHandle);
    }
}

void TstBegin()
{
   ExExecuteShell();
}

