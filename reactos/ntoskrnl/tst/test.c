/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/tst/test.c
 * PURPOSE:         Kernel regression tests
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                28/05/98: Created
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/string.h>

//#define NDEBUG
#include <internal/debug.h>

#include <in.h>

/* GLOBALS ******************************************************************/

static KEVENT event = {};
//static KEVENT event2;
NTSTATUS TstShell(VOID);

/* FUNCTIONS ****************************************************************/
NTSTATUS TstPlaySound(VOID)
{
   HANDLE hfile;
   
//    * Open the parallel port
   printk("Opening Waveout\n");
//   hfile = CreateFile("\\Device\\WaveOut",0,0,0,0,0,0);
   if (hfile == NULL)
     {
	printk("File open failed\n");
     }
   else
     {
//	WriteFile(hfile,wave,wavelength,NULL,NULL);
     }
}

NTSTATUS TstFirstThread(PVOID start)
{
   int i;
   
   printk("Beginning Thread A\n");
   for (;;)
//   for (i=0;i<10;i++)
     {     
	KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
	printk("AAA ");
        KeSetEvent(&event,IO_NO_INCREMENT,FALSE);
	for (i=0;i<10000;i++);
     }
}

NTSTATUS TstSecondThread(PVOID start)
{
   int i;
   
   printk("Beginning Thread B\n");
   for(;;)
//   for (i=0;i<10;i++)
     {
	KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
	printk("BBB ");
        KeSetEvent(&event,IO_NO_INCREMENT,FALSE);
	for (i=0;i<100000;i++);
     }
}

NTSTATUS TstThreadSupport()
{
   HANDLE th1, th2;
   
   KeInitializeEvent(&event,SynchronizationEvent,TRUE);
   PsCreateSystemThread(&th1,0,NULL,NULL,NULL,TstFirstThread,NULL);
   PsCreateSystemThread(&th2,0,NULL,NULL,NULL,TstSecondThread,NULL);
   printk("Ending main thread\n");
   for(;;);
}

void TstGeneralWrite(VOID)
{
   OBJECT_ATTRIBUTES attr;
   HANDLE hfile;
   char buf[512];
   ANSI_STRING afilename;
   UNICODE_STRING ufilename;
   
   DbgPrint("Opening test device\n");
   RtlInitAnsiString(&afilename,"\\Device\\SDisk");
   RtlAnsiStringToUnicodeString(&ufilename,&afilename,TRUE);
   InitializeObjectAttributes(&attr,&ufilename,0,NULL,NULL);
   ZwOpenFile(&hfile,0,&attr,NULL,0,0);
   if (hfile==NULL)
     {
	DbgPrint("Failed to open test device\n");
        return;
     }
   ZwReadFile(hfile,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       buf,
	       512,
	       0,
	       0);
   DbgPrint("buf %s\n",buf);
 }

void TstParallelPortWrite(VOID)
{
   HANDLE hfile;
   
   DbgPrint("Opening parallel port\n");
//   hfile = CreateFile("\\Device\\Parallel",0,0,0,0,0,0);
   if (hfile==NULL)
     {
	DbgPrint("Failed to open parallel port\n");
     }
 //  WriteFile(hfile,"hello world",strlen("hello world"),NULL,NULL);
}

void TstKeyboardRead(VOID)
{
   OBJECT_ATTRIBUTES attr;
   HANDLE hfile;
   ANSI_STRING afilename;
   UNICODE_STRING ufilename;
   KEY_EVENT_RECORD key[2];
   
   DbgPrint("Opening keyboard\n");
   RtlInitAnsiString(&afilename,"\\Device\\Keyboard");
   RtlAnsiStringToUnicodeString(&ufilename,&afilename,TRUE);
   InitializeObjectAttributes(&attr,&ufilename,0,NULL,NULL);
   ZwOpenFile(&hfile,0,&attr,NULL,0,0);
   if (hfile==NULL)
     {
	DbgPrint("Failed to open keyboard\n");
        return;
     }
   for(;;)
     {
	ZwReadFile(hfile,
		   NULL,
		   NULL,
		   NULL,
		   NULL,
		   &key[0],
		   sizeof(KEY_EVENT_RECORD)*2,
		   0,
		   0);
	DbgPrint("%c",key[0].AsciiChar);
//	DbgPrint("%c",key[1].AsciiChar);
     }
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

void TstFileRead(VOID)
{
   OBJECT_ATTRIBUTES attr;
   HANDLE hfile;
   ANSI_STRING afilename;
   UNICODE_STRING ufilename;
   char ch;
   IO_STATUS_BLOCK IoStatusBlock;
   
   DbgPrint("Opening file\n");
   RtlInitAnsiString(&afilename,"\\??\\C:\\my_other_directory\\..\\"
		     "my_valid_directory\\apc.txt");
   RtlAnsiStringToUnicodeString(&ufilename,&afilename,TRUE);
   InitializeObjectAttributes(&attr,&ufilename,0,NULL,NULL);
   ZwOpenFile(&hfile,0,&attr,NULL,0,0);
   if (hfile==NULL)
     {
	DbgPrint("Failed to open file\n");
        return;
     }
   while (1)
     {
//	CHECKPOINT;
	ZwReadFile(hfile,
		     NULL,
		     NULL,
		     NULL,
		     &IoStatusBlock,
		     &ch,
		     1,
		     NULL,
		     NULL);
	DbgPrint("%c",ch);
     }
   CHECKPOINT;
 }

void TstIDERead(void)
{
  BOOLEAN TestFailed;
  int Entry, i, j;
  HANDLE FileHandle;
  NTSTATUS Status;
  LARGE_INTEGER BlockOffset;
  ANSI_STRING AnsiDeviceName;
  UNICODE_STRING UnicodeDeviceName;
  OBJECT_ATTRIBUTES ObjectAttributes;
// static  char SectorBuffer2[512 ];
static  char SectorBuffer[512 * 10];
  PBOOT_BLOCK BootBlock;
  ROOT_DIR_ENTRY DirectoryBlock[ENTRIES_PER_BLOCK];

  DbgPrint("IDE Read Test\n");
  TestFailed = FALSE;

    /*  open the first partition  */
  DbgPrint("Opening Partition1\n");
  RtlInitAnsiString(&AnsiDeviceName, "\\Device\\HardDrive0\\Partition1");
  RtlAnsiStringToUnicodeString(&UnicodeDeviceName, &AnsiDeviceName, TRUE);
  InitializeObjectAttributes(&ObjectAttributes,
                             &UnicodeDeviceName, 
                             0,
                             NULL,
                             NULL);
  Status = ZwOpenFile(&FileHandle, 0, &ObjectAttributes, NULL, 0, 0);
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
      BlockOffset.HighPart = 0;
      BlockOffset.LowPart = BootBlock->BootParameters.ReservedSectorCount * 512 +
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
      RtlFillMemory(SectorBuffer, sizeof(SectorBuffer), 0xea);
      BlockOffset.HighPart = 0;
      BlockOffset.LowPart = 10000 * 512;
      Status = ZwReadFile(FileHandle,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          SectorBuffer,
                          512 * 5,
                          &BlockOffset,
                          0);
      if (!NT_SUCCESS(Status))
        {
          DbgPrint("Failed to read blocks 10000-4 from partition1 status:%x\n", 
                   Status);
          TestFailed = TRUE;
        }
      else
        {
          for (j = 0; j < 10; j++)
            {
              DbgPrint("%04x: ", j * 256);
              for (i = 0; i < 16; i++)
                {
                  DbgPrint("%02x ", (unsigned char)SectorBuffer[j * 256 + i]);
                  SectorBuffer[j * 256 + i]++;
                }
              DbgPrint("\n");
            }
//for(;;);
          Status = ZwWriteFile(FileHandle,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               SectorBuffer,
                               512 * 5,
                               &BlockOffset,
                               0);
          if (!NT_SUCCESS(Status))
            {
              DbgPrint("Failed to write blocks 10000-4 to partition1 status:%x\n", 
                       Status);
              TestFailed = TRUE;
            }
          else
            {
              DbgPrint("Block written\n");
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
//   TstFileRead();
//   TstGeneralWrite();
//   TstThreadSupport();
//   TstKeyboardRead();
   TstIDERead();
//   TstKeyboardRead();
//   TstShell();
}

