/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/initpage.c
 * PURPOSE:         Paging file support.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>

#define GIGABYTE (1024 * 1024 * 1024) /* One Gigabyte */

static NTSTATUS NTAPI
SmpPagingFilesQueryRoutine(PWSTR ValueName,
                           ULONG ValueType,
                           PVOID ValueData,
                           ULONG ValueLength,
                           PVOID Context,
                           PVOID EntryContext)
{
  UNICODE_STRING FileName;
  LARGE_INTEGER InitialSize = {{0, 0}};
  LARGE_INTEGER MaximumSize = {{0, 0}};
  NTSTATUS Status = STATUS_SUCCESS;
  PWSTR p, ValueString = (PWSTR)ValueData;
  WCHAR RootDriveLetter[5] = {0};

  if (ValueLength > 3 * sizeof(WCHAR) &&
      (ValueLength % sizeof(WCHAR) != 0 ||
      ValueString[(ValueLength / sizeof(WCHAR)) - 1] != L'\0'))
    {
      return STATUS_INVALID_PARAMETER;
    }

  if (ValueType != REG_SZ)
    {
      return STATUS_INVALID_PARAMETER_2;
    }

  /*
  * Format: "<path>[ <initial_size>[ <maximum_size>]]"
  */
  if ((p = wcschr(ValueString, L' ')) != NULL)
    {
      *p = L'\0';
      InitialSize.QuadPart = wcstoul(p + 1, &p, 0) * 256 * 4096;
      if (*p == ' ')
        {
          MaximumSize.QuadPart = wcstoul(p + 1, NULL, 0) * 256 * 4096;
        }
      else
        {
          MaximumSize = InitialSize;
        }
    }

  if (!RtlDosPathNameToNtPathName_U (ValueString,
      &FileName,
      NULL,
      NULL))
    {
      return STATUS_OBJECT_PATH_INVALID;
    }

  /* If there is only a file name or if initial and max are both 0
   * the system will pick the sizes.  Then it makes intial the size of phyical memory
   * and makes max the size of 1.5 * initial.  If there isnt enough free space then it will
   * fall back to intial 20% of free space and max 25%.  There is a max of 1 gig before
   * it doesnt make it bigger. */
  if ((InitialSize.QuadPart == 0 && MaximumSize.QuadPart == 0) || p == NULL)
    {
      FILE_FS_SIZE_INFORMATION FileFsSize;
      IO_STATUS_BLOCK IoStatusBlock;
      HANDLE hFile;
      SYSTEM_BASIC_INFORMATION SysBasicInfo;
      UNICODE_STRING NtPathU;
      LARGE_INTEGER FreeBytes = {{0, 0}};
      OBJECT_ATTRIBUTES ObjectAttributes;

      DPRINT("System managed pagefile...\n");
      /* Make sure the path that is given for the file actually has the drive in it.
      At this point if there is not file name, no sizes will be set therefore no page
      file will be created */
      if (wcslen(ValueString) <= 3 ||
          ValueString[1] != L':' ||
          ValueString[2] != L'\\')
        {
          DPRINT1("Invalid path for pagefile.\n");
          goto Cleanup;
        }

      Status = NtQuerySystemInformation(SystemBasicInformation,
                                        &SysBasicInfo,
                                        sizeof(SysBasicInfo),
                                        NULL);
      if (!NT_SUCCESS(Status))
        {
          DPRINT1("Could not query for physical memory size.\n");
          goto Cleanup;
        }
      DPRINT("PageSize: %d, PhysicalPages: %d, TotalMem: %d\n", SysBasicInfo.PageSize, SysBasicInfo.NumberOfPhysicalPages, (SysBasicInfo.NumberOfPhysicalPages * SysBasicInfo.PageSize) / 1024);

      InitialSize.QuadPart = SysBasicInfo.NumberOfPhysicalPages *
                             SysBasicInfo.PageSize;
      MaximumSize.QuadPart = InitialSize.QuadPart * 2;

      DPRINT("InitialSize: %I64d PhysicalPages: %lu PageSize: %lu\n",InitialSize.QuadPart,SysBasicInfo.NumberOfPhysicalPages,SysBasicInfo.PageSize);

      /* copy the drive letter, the colon and the slash,
      tack a null on the end */
      RootDriveLetter[0] = ValueString[0];
      RootDriveLetter[1] = L':';
      RootDriveLetter[2] = L'\\';
      RootDriveLetter[3] = L'\0';
      DPRINT("Root drive X:\\...\"%S\"\n",RootDriveLetter);

      if (!RtlDosPathNameToNtPathName_U(RootDriveLetter,
          &NtPathU,
          NULL,
          NULL))
        {
          DPRINT1("Invalid path to root of drive\n");
          Status = STATUS_OBJECT_PATH_INVALID;
          goto Cleanup;
        }

      InitializeObjectAttributes(&ObjectAttributes,
                                 &NtPathU,
                                 OBJ_CASE_INSENSITIVE,
                                 NULL,
                                 NULL);

      /* Get a handle to the root to find the free space on the drive */
      Status = NtCreateFile(&hFile,
                            0,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            NULL,
                            0,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_OPEN,
                            0,
                            NULL,
                            0);

      RtlFreeHeap(RtlGetProcessHeap(),
                  0,
                  NtPathU.Buffer);

      if (!NT_SUCCESS(Status))
        {
          DPRINT1("Could not open a handle to the volume.\n");
          goto Cleanup;
        }

      Status = NtQueryVolumeInformationFile(hFile,
                                            &IoStatusBlock,
                                            &FileFsSize,
                                            sizeof(FILE_FS_SIZE_INFORMATION),
                                            FileFsSizeInformation);

      NtClose(hFile);

      if (!NT_SUCCESS(Status))
        {
          DPRINT1("Querying the volume free space failed!\n");
          goto Cleanup;
        }

      FreeBytes.QuadPart = FileFsSize.BytesPerSector *
                           FileFsSize.SectorsPerAllocationUnit *
                           FileFsSize.AvailableAllocationUnits.QuadPart;

      DPRINT("Free bytes: %I64d   Inital Size based on memory: %I64d \n",FreeBytes.QuadPart,InitialSize.QuadPart);


      if (InitialSize.QuadPart > (FreeBytes.QuadPart / 4) || InitialSize.QuadPart == 0)
        {
          DPRINT("Inital Size took more then 25%% of free space\n");
          /* Set by percentage of free space
          * intial is 20%, and max is 25% */
          InitialSize.QuadPart = FreeBytes.QuadPart / 5;
          MaximumSize.QuadPart = FreeBytes.QuadPart / 4;
          /* The page file is more then a gig, size it down */
          if (InitialSize.QuadPart > GIGABYTE)
            {
              InitialSize.QuadPart = GIGABYTE;
              MaximumSize.QuadPart = GIGABYTE * 3 / 2;
            }
        }


    }

  /* Make sure that max is not smaller then initial */
  if (InitialSize.QuadPart > MaximumSize.QuadPart)
    {
      DPRINT("Max page file size was bigger then inital.\n");
      MaximumSize.QuadPart = InitialSize.QuadPart;
    }

  DPRINT("Creating paging file %wZ with size %I64d KB\n",
         &FileName, InitialSize.QuadPart / 1024);

  Status = NtCreatePagingFile(&FileName,
                              &InitialSize,
                              &MaximumSize,
                              0);
  if (! NT_SUCCESS(Status))
    {
      PrintString("Creation of paging file %wZ with size %I64d KB failed (status 0x%x)\n",
                  &FileName, InitialSize.QuadPart / 1024, Status);
    }

Cleanup:
  RtlFreeHeap(RtlGetProcessHeap(),
              0,
              FileName.Buffer);

  return STATUS_SUCCESS;
}


static NTSTATUS
SmpGetFreeDiskSpace(IN PWSTR PageFileName,
                    OUT PLARGE_INTEGER FreeDiskSpaceInMB)
{
    FILE_FS_SIZE_INFORMATION FileFsSize;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE hFile;
    UNICODE_STRING NtPathU;
    LARGE_INTEGER FreeBytes;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR RootPath[5];
    NTSTATUS Status;

    /*
     * copy the drive letter, the colon and the slash,
     * tack a null on the end
     */
    RootPath[0] = PageFileName[0];
    RootPath[1] = L':';
    RootPath[2] = L'\\';
    RootPath[3] = L'\0';

    DPRINT("Root drive X:\\...\"%S\"\n",RootPath);

    if (!RtlDosPathNameToNtPathName_U(RootPath,
                                      &NtPathU,
                                      NULL,
                                      NULL))
    {
        DPRINT1("Invalid path to root of drive\n");
        return STATUS_OBJECT_PATH_INVALID;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathU,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Get a handle to the root to find the free space on the drive */
    Status = NtCreateFile(&hFile,
                          0,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          0,
                          NULL,
                          0);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                NtPathU.Buffer);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not open a handle to the volume.\n");
        return Status;
    }

    Status = NtQueryVolumeInformationFile(hFile,
                                          &IoStatusBlock,
                                          &FileFsSize,
                                          sizeof(FILE_FS_SIZE_INFORMATION),
                                          FileFsSizeInformation);

    NtClose(hFile);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Querying the volume free space failed!\n");
        return Status;
    }

    FreeBytes.QuadPart = FileFsSize.BytesPerSector *
                         FileFsSize.SectorsPerAllocationUnit *
                         FileFsSize.AvailableAllocationUnits.QuadPart;

    FreeDiskSpaceInMB->QuadPart = FreeBytes.QuadPart >> 20;

    return STATUS_SUCCESS;
}


static NTSTATUS
SmpSetDefaultPageFileData(IN PWSTR PageFileName,
                          IN PLARGE_INTEGER InitialSizeInMB,
                          IN PLARGE_INTEGER MaximumSizeInMB)
{
    WCHAR ValueString[MAX_PATH * 2];
    ULONG ValueLength;

    /* Format the value string */
    swprintf(ValueString,
             L"%s %I64u %I64u",
             PageFileName,
             InitialSizeInMB->QuadPart,
             MaximumSizeInMB->QuadPart);

    /*
     * Append another zero character because it is a multi string value
     * (REG_MULTI_SZ) and calculate the total string length.
     */
    ValueLength = wcslen(ValueString) + 1;
    ValueString[ValueLength] = 0;
    ValueLength++;

    /* Write the page file data */
    return RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                 L"\\Session Manager\\Memory Management",
                                 L"PagingFiles",
                                 REG_MULTI_SZ,
                                 ValueString,
                                 ValueLength * sizeof(WCHAR));
}


static NTSTATUS
SmpCreatePageFile(IN PWSTR PageFileName,
                  IN PLARGE_INTEGER InitialSizeInMB,
                  IN PLARGE_INTEGER MaximumSizeInMB)
{
    LARGE_INTEGER InitialSize;
    LARGE_INTEGER MaximumSize;
    UNICODE_STRING FileName;
    NTSTATUS Status;

    /* Get the NT path name of the page file */
    if (!RtlDosPathNameToNtPathName_U(PageFileName,
                                      &FileName,
                                      NULL,
                                      NULL))
    {
        return STATUS_OBJECT_PATH_INVALID;
    }

    /* Convert sizes in megabytes to sizes in bytes */
    InitialSize.QuadPart = InitialSizeInMB->QuadPart << 20;
    MaximumSize.QuadPart = MaximumSizeInMB->QuadPart << 20;

    /* Create the pageing file */
    Status = NtCreatePagingFile(&FileName,
                                &InitialSize,
                                &MaximumSize,
                                0);
    if (! NT_SUCCESS(Status))
    {
        DPRINT("Creation of paging file %wZ with size %I64d MB failed (status 0x%x)\n",
               &FileName, InitialSizeInMB->QuadPart, Status);
    }

    /* Release the file name */
    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                FileName.Buffer);

    return Status;
}


static NTSTATUS
SmpCreateDefaultPagingFile(VOID)
{
    SYSTEM_BASIC_INFORMATION SysBasicInfo;
    LARGE_INTEGER MemorySizeInMB;
    LARGE_INTEGER FreeDiskSpaceInMB;
    LARGE_INTEGER InitialSizeInMB;
    LARGE_INTEGER MaximumSizeInMB;
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR PageFileName[MAX_PATH];

    DPRINT("Creating a default paging file\n");

    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &SysBasicInfo,
                                      sizeof(SysBasicInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not query for physical memory size.\n");
        return Status;
    }

    DPRINT("PageSize: %d, PhysicalPages: %d, TotalMem: %d\n",
           SysBasicInfo.PageSize,
           SysBasicInfo.NumberOfPhysicalPages,
           (SysBasicInfo.NumberOfPhysicalPages * SysBasicInfo.PageSize) / 1024);

    MemorySizeInMB.QuadPart = (SysBasicInfo.NumberOfPhysicalPages * SysBasicInfo.PageSize) >> 20;

    DPRINT("MemorySize %I64u MB\n",
           MemorySizeInMB.QuadPart);

    /* Build the default page file name */
    PageFileName[0] = SharedUserData->NtSystemRoot[0];
    PageFileName[1] = 0;
    wcscat(PageFileName, L":\\pagefile.sys");

    Status = SmpGetFreeDiskSpace(PageFileName,
                                 &FreeDiskSpaceInMB);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    DPRINT("FreeDiskSpace %I64u MB\n",
           FreeDiskSpaceInMB.QuadPart);

    InitialSizeInMB.QuadPart = MemorySizeInMB.QuadPart + (MemorySizeInMB.QuadPart / 2);
    MaximumSizeInMB.QuadPart = InitialSizeInMB.QuadPart * 2;

    if (InitialSizeInMB.QuadPart > (FreeDiskSpaceInMB.QuadPart / 4))
    {
        DPRINT("Inital Size took more then 25%% of free disk space\n");

        /*
         * Set by percentage of free space
         * intial is 20%, and max is 25%
         */
        InitialSizeInMB.QuadPart = FreeDiskSpaceInMB.QuadPart / 5;
        MaximumSizeInMB.QuadPart = FreeDiskSpaceInMB.QuadPart / 4;

        /* The page file is more then a gig, size it down */
        if (InitialSizeInMB.QuadPart > 1024)
        {
            InitialSizeInMB.QuadPart = 1024; /* 1GB */
            MaximumSizeInMB.QuadPart = 1536; /* 1.5GB */
        }
    }

    DPRINT("InitialSize %I64u MB   MaximumSize %I64u MB\n",
           InitialSizeInMB.QuadPart,
           MaximumSizeInMB.QuadPart);

    Status = SmpSetDefaultPageFileData(PageFileName,
                                       &InitialSizeInMB,
                                       &MaximumSizeInMB);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return SmpCreatePageFile(PageFileName,
                             &InitialSizeInMB,
                             &MaximumSizeInMB);
}


NTSTATUS
SmCreatePagingFiles(VOID)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    NTSTATUS Status;

    DPRINT("creating system paging files\n");

    /* Disable paging file on MiniNT/Live CD. */
    if (RtlCheckRegistryKey(RTL_REGISTRY_CONTROL, L"MiniNT") == STATUS_SUCCESS)
    {
        return STATUS_SUCCESS;
    }

    RtlZeroMemory(&QueryTable,
                  sizeof(QueryTable));

    QueryTable[0].Name = L"PagingFiles";
    QueryTable[0].QueryRoutine = SmpPagingFilesQueryRoutine;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                    L"\\Session Manager\\Memory Management",
                                    QueryTable,
                                    NULL,
                                    NULL);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        Status = SmpCreateDefaultPagingFile();
    }

    return Status;
}


/* EOF */
