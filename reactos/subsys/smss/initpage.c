/* $Id$
 *
 * initpage.c - 
 * 
 * ReactOS Operating System
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include "smss.h"

#define NDEBUG
#include <debug.h>

#define GIGABYTE (1024 * 1024 * 1024) /* One Gigabyte */

static NTSTATUS STDCALL
SmpPagingFilesQueryRoutine(PWSTR ValueName,
			                  ULONG ValueType,
			                  PVOID ValueData,
			                  ULONG ValueLength,
			                  PVOID Context,
			                  PVOID EntryContext)
{
   UNICODE_STRING FileName;
   LARGE_INTEGER InitialSize;
   LARGE_INTEGER MaximumSize;
   NTSTATUS Status = STATUS_SUCCESS;
   LPWSTR p;
   WCHAR RootDriveLetter[5];


   DPRINT("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
   DPRINT("ValueData '%S'\n", (PWSTR)ValueData);

   if (ValueType != REG_SZ)
   {
      return STATUS_INVALID_PARAMETER_2;
   }

   if (!RtlDosPathNameToNtPathName_U ((LPWSTR)ValueData,
      &FileName,
      NULL,
      NULL))
   {
      return STATUS_OBJECT_PATH_NOT_FOUND;
   }
   /*
   * Format: "<path>[ <initial_size>[ <maximum_size>]]"
   */
   if ((p = wcschr(ValueData, ' ')) != NULL)
   {
      *p = L'\0';
      InitialSize.QuadPart = wcstoul(p + 1, &p, 0) * 256 * 4096;
      if (*p == ' ')
      {
         MaximumSize.QuadPart = wcstoul(p + 1, NULL, 0) * 256 * 4096;
      }
      else
         MaximumSize = InitialSize;
   }

   /* If there is only a file name or if initial and max are both 0
      the system will pick the sizes.  10% and 20% for initial and max
      respectivly.  There is a max of 1 gig before it doesnt make it
      bigger. */
   if((InitialSize.QuadPart == 0 && MaximumSize.QuadPart == 0) || p == NULL)
   {
      FILE_FS_SIZE_INFORMATION FileFsSize;
      IO_STATUS_BLOCK IoStatusBlock;
      HANDLE hFile;
      UNICODE_STRING NtPathU;
      LARGE_INTEGER FreeBytes;
      OBJECT_ATTRIBUTES ObjectAttributes;

      DPRINT("System managed pagefile...\n");
      /* Make sure the path that is given for the file actually has the drive in it.
         At this point if there is not file name, no sizes will be set therefore no page
         file will be created */
      if (wcslen(((PWSTR)ValueData)) > 2 && 
          ((PWSTR)ValueData)[1] == L':' && 
          ((PWSTR)ValueData)[2] == L'\\')
      {
         /* copy the drive letter, the colon and the slash,
            tack a null on the end */
         p = ((PWSTR)ValueData);
         wcsncpy(RootDriveLetter,p,3);
         wcscat(RootDriveLetter,L"\0");
         DPRINT("At least X:\\...%S\n",RootDriveLetter);

         if (!RtlDosPathNameToNtPathName_U((LPWSTR)RootDriveLetter,
                                           &NtPathU,
                                           NULL,
                                           NULL))
         {
            DPRINT("Invalid path\n");
            return STATUS_OBJECT_PATH_NOT_FOUND;
         }

         InitializeObjectAttributes(&ObjectAttributes,
                                    &NtPathU,
                                    FILE_READ_ATTRIBUTES,
                                    NULL,
                                    NULL);

         /* Get a handle to the root to find the free space on the drive */
         Status = NtCreateFile (&hFile,
                                FILE_GENERIC_READ,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                NULL,
                                0,
                                FILE_SHARE_READ,
                                FILE_OPEN,
                                0,
                                NULL,
                                0);

         RtlFreeUnicodeString(&NtPathU);

         if (!NT_SUCCESS(Status))
         {
             DPRINT ("Invalid handle\n");
             return Status;
         }

         Status = NtQueryVolumeInformationFile(hFile,
                                               &IoStatusBlock,
                                               &FileFsSize,
                                               sizeof(FILE_FS_SIZE_INFORMATION),
                                               FileFsSizeInformation);

         FreeBytes.QuadPart = FileFsSize.BytesPerSector * 
                              FileFsSize.SectorsPerAllocationUnit * 
                              FileFsSize.AvailableAllocationUnits.QuadPart;

         DPRINT ("Free:%d\n",FreeBytes.QuadPart);

         /* Set by percentage */
         InitialSize.QuadPart = FreeBytes.QuadPart / 10;
         MaximumSize.QuadPart = FreeBytes.QuadPart / 5;

         /* The page file is more then a gig, size it down */
         if (InitialSize.QuadPart > GIGABYTE)
         {
             InitialSize.QuadPart = GIGABYTE;
             MaximumSize.QuadPart = GIGABYTE;
         }

         ZwClose(hFile);
      }
      else
      {
         /* No page file will be created, 
            but we return succes because that 
            is what the setting wants */
         return (STATUS_SUCCESS);
      }
   }

   DPRINT("SMSS: Created paging file %wZ with size %dKB\n",
          &FileName, InitialSize.QuadPart / 1024);

   Status = NtCreatePagingFile(&FileName,
                               &InitialSize,
                               &MaximumSize,
                               0);

   RtlFreeUnicodeString(&FileName);

   return Status;
}


NTSTATUS
SmCreatePagingFiles(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;

  DPRINT("SM: creating system paging files\n");
  /*
   * Disable paging file on MiniNT/Live CD.
   */
  if (RtlCheckRegistryKey(RTL_REGISTRY_CONTROL, L"MiniNT") == STATUS_SUCCESS)
    {
      return STATUS_SUCCESS;
    }

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"PagingFiles";
  QueryTable[0].QueryRoutine = SmpPagingFilesQueryRoutine;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"\\Session Manager\\Memory Management",
				  QueryTable,
				  NULL,
				  NULL);

  return(Status);
}


/* EOF */
