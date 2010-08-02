/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/hardlink.c
 * PURPOSE:         Hardlink functions
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *                  Created 13/03/2004
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32file);

/* FUNCTIONS ****************************************************************/


/*
 * @implemented
 */
BOOL WINAPI
CreateHardLinkW(LPCWSTR lpFileName,
                LPCWSTR lpExistingFileName,
                LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
  UNICODE_STRING LinkTarget, LinkName;
  LPVOID lpSecurityDescriptor;
  PFILE_LINK_INFORMATION LinkInformation;
  IO_STATUS_BLOCK IoStatus;
  NTSTATUS Status;
  BOOL Ret = FALSE;

  if(!lpFileName || !lpExistingFileName)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  lpSecurityDescriptor = (lpSecurityAttributes ? lpSecurityAttributes->lpSecurityDescriptor : NULL);

  if(RtlDetermineDosPathNameType_U((LPWSTR)lpFileName) == 1 ||
     RtlDetermineDosPathNameType_U((LPWSTR)lpExistingFileName) == 1)
  {
    WARN("CreateHardLinkW() cannot handle UNC Paths!\n");
    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
  }

  if(RtlDosPathNameToNtPathName_U(lpExistingFileName, &LinkTarget, NULL, NULL))
  {
    ULONG NeededSize = RtlGetFullPathName_U((LPWSTR)lpExistingFileName, 0, NULL, NULL);
    if(NeededSize > 0)
    {
      LPWSTR lpNtLinkTarget = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, NeededSize * sizeof(WCHAR));
      if(lpNtLinkTarget != NULL)
      {
        LPWSTR lpFilePart;

        if(RtlGetFullPathName_U((LPWSTR)lpExistingFileName, NeededSize, lpNtLinkTarget, &lpFilePart) &&
           (*lpNtLinkTarget) != L'\0')
        {
          UNICODE_STRING CheckDrive, LinkDrive;
          WCHAR wCheckDrive[10];

          swprintf(wCheckDrive, L"\\??\\%c:", (WCHAR)(*lpNtLinkTarget));
          RtlInitUnicodeString(&CheckDrive, wCheckDrive);

          RtlZeroMemory(&LinkDrive, sizeof(UNICODE_STRING));

          LinkDrive.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, (MAX_PATH + 1) * sizeof(WCHAR));
          if(LinkDrive.Buffer != NULL)
          {
            HANDLE hFile, hTarget;
            OBJECT_ATTRIBUTES ObjectAttributes;

            InitializeObjectAttributes(&ObjectAttributes,
                                       &CheckDrive,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            Status = NtOpenSymbolicLinkObject(&hFile, 1, &ObjectAttributes);
            if(NT_SUCCESS(Status))
            {
              UNICODE_STRING LanManager;

              RtlInitUnicodeString(&LanManager, L"\\Device\\LanmanRedirector\\");

              NtQuerySymbolicLinkObject(hFile, &LinkDrive, NULL);

              if(!RtlPrefixUnicodeString(&LanManager, &LinkDrive, TRUE))
              {
                InitializeObjectAttributes(&ObjectAttributes,
                                           &LinkTarget,
                                           OBJ_CASE_INSENSITIVE,
                                           NULL,
                                           lpSecurityDescriptor);
                Status = NtOpenFile(&hTarget,
                                    SYNCHRONIZE | DELETE,
                                    &ObjectAttributes,
                                    &IoStatus,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT);
                if(NT_SUCCESS(Status))
                {
                  if(RtlDosPathNameToNtPathName_U(lpFileName, &LinkName, NULL, NULL))
                  {
                    NeededSize = sizeof(FILE_LINK_INFORMATION) + LinkName.Length + sizeof(WCHAR);
                    LinkInformation = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, NeededSize);
                    if(LinkInformation != NULL)
                    {
                      LinkInformation->ReplaceIfExists = FALSE;
                      LinkInformation->RootDirectory = 0;
                      LinkInformation->FileNameLength = LinkName.Length;
                      RtlCopyMemory(LinkInformation->FileName, LinkName.Buffer, LinkName.Length);

                      Status = NtSetInformationFile(hTarget, &IoStatus, LinkInformation, NeededSize, FileLinkInformation);
                      if(NT_SUCCESS(Status))
                      {
                        Ret = TRUE;
                      }
                      else
                      {
                        SetLastErrorByStatus(Status);
                      }

                      RtlFreeHeap(RtlGetProcessHeap(), 0, LinkInformation);
                    }
                    else
                    {
                      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    }
                  }
                  else
                  {
                    SetLastError(ERROR_PATH_NOT_FOUND);
                  }
                  NtClose(hTarget);
                }
                else
                {
                  WARN("Unable to open link destination \"%wZ\"!\n", &LinkTarget);
                  SetLastErrorByStatus(Status);
                }
              }
              else
              {
                WARN("Path \"%wZ\" must not be a mapped drive!\n", &LinkDrive);
                SetLastError(ERROR_INVALID_NAME);
              }

              NtClose(hFile);
            }
            else
            {
              SetLastErrorByStatus(Status);
            }
          }
          else
          {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          }
        }
        else
        {
          SetLastError(ERROR_INVALID_NAME);
        }
        RtlFreeHeap(RtlGetProcessHeap(), 0, lpNtLinkTarget);
      }
      else
      {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      }
    }
    else
    {
      SetLastError(ERROR_INVALID_NAME);
    }
    RtlFreeHeap(RtlGetProcessHeap(), 0, LinkTarget.Buffer);
  }
  else
  {
    SetLastError(ERROR_PATH_NOT_FOUND);
  }

  return Ret;
}


/*
 * @implemented
 */
BOOL WINAPI
CreateHardLinkA(LPCSTR lpFileName,
                LPCSTR lpExistingFileName,
                LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
  PWCHAR FileNameW, ExistingFileNameW;
  BOOL Ret;

  if(!lpFileName || !lpExistingFileName)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  if (!(FileNameW = FilenameA2W(lpFileName, FALSE)))
    return FALSE;

  if (!(ExistingFileNameW = FilenameA2W(lpExistingFileName, TRUE)))
    return FALSE;

  Ret = CreateHardLinkW(FileNameW , ExistingFileNameW , lpSecurityAttributes);

  RtlFreeHeap(RtlGetProcessHeap(), 0, ExistingFileNameW);

  return Ret;
}

/* EOF */
