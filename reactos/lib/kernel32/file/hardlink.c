/* $Id: hardlink.c,v 1.1 2004/03/14 09:21:42 weiden Exp $
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
#include <ddk/ntifs.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS ****************************************************************/


/*
 * @implemented
 */
BOOL STDCALL
CreateHardLinkW(
  LPCWSTR lpFileName,
  LPCWSTR lpExistingFileName,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes
)
{
  UNICODE_STRING LinkTarget, LinkName, CheckDrive, LinkDrive, LanManager;
  LPWSTR lpNtLinkTarget, lpFilePart;
  ULONG NeededSize;
  LPVOID lpSecurityDescriptor;
  WCHAR wCheckDrive[10];
  OBJECT_ATTRIBUTES ObjectAttribues;
  PFILE_LINK_INFORMATION LinkInformation;
  IO_STATUS_BLOCK IoStatus;
  HANDLE hFile, hTarget;
  NTSTATUS Status;
  
  if(!lpFileName || !lpExistingFileName)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  lpSecurityDescriptor = (lpSecurityAttributes ? lpSecurityAttributes->lpSecurityDescriptor : NULL);
  
  if(RtlDetermineDosPathNameType_U((LPWSTR)lpFileName) == 1 ||
     RtlDetermineDosPathNameType_U((LPWSTR)lpExistingFileName) == 1)
  {
    DPRINT1("CreateHardLinkW() cannot handle UNC Paths!\n");
    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
  }
  
  if(!RtlDosPathNameToNtPathName_U((LPWSTR)lpExistingFileName, &LinkTarget, NULL, NULL))
  {
    SetLastError(ERROR_PATH_NOT_FOUND);
    return FALSE;
  }
  
  if(!(NeededSize = RtlGetFullPathName_U((LPWSTR)lpExistingFileName, 0, NULL, NULL)))
  {
    RtlFreeUnicodeString(&LinkTarget);
    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
  }
  
  NeededSize += 2;
  if(!(lpNtLinkTarget = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, NeededSize * sizeof(WCHAR))))
  {
    RtlFreeUnicodeString(&LinkTarget);
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  
  if(!RtlGetFullPathName_U((LPWSTR)lpExistingFileName, NeededSize, lpNtLinkTarget, &lpFilePart))
  {
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpNtLinkTarget);
    RtlFreeUnicodeString(&LinkTarget);
    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
  }
  
  swprintf(wCheckDrive, L"\\??\\%c:", (WCHAR)(*lpNtLinkTarget));
  RtlInitUnicodeString(&CheckDrive, wCheckDrive);
  
  RtlZeroMemory(&LinkDrive, sizeof(UNICODE_STRING));
  if(!(LinkDrive.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, 
                                          (MAX_PATH + 1) * sizeof(WCHAR))))
  {
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpNtLinkTarget);
    RtlFreeUnicodeString(&LinkTarget);
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  
  InitializeObjectAttributes(&ObjectAttribues,
                             &CheckDrive,
                             OBJ_CASE_INSENSITIVE,
                             NULL,
                             NULL);
  
  Status = ZwOpenSymbolicLinkObject(&hFile, 1, &ObjectAttribues);
  if(!NT_SUCCESS(Status))
  {
    RtlFreeHeap(RtlGetProcessHeap(), 0, LinkDrive.Buffer);
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpNtLinkTarget);
    RtlFreeUnicodeString(&LinkTarget);
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  
  RtlInitUnicodeString(&LanManager, L"\\Device\\LanmanRedirector\\");
  
  ZwQuerySymbolicLinkObject(hFile, &LinkDrive, NULL);
  
  if(RtlPrefixUnicodeString(&LanManager, &LinkDrive, TRUE))
  {
    ZwClose(hFile);
    RtlFreeHeap(RtlGetProcessHeap(), 0, LinkDrive.Buffer);
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpNtLinkTarget);
    RtlFreeUnicodeString(&LinkTarget);
    DPRINT1("Path \"%wZ\" must not be a mapped drive!\n", &LinkDrive);
    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
  }
  
  InitializeObjectAttributes(&ObjectAttribues,
                             &LinkTarget,
                             OBJ_CASE_INSENSITIVE,
                             NULL,
                             lpSecurityDescriptor);
  
  Status = ZwOpenFile(&hTarget, SYNCHRONIZE | DELETE, &ObjectAttribues, &IoStatus,
                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT);
  if(!NT_SUCCESS(Status))
  {
    ZwClose(hFile);
    RtlFreeHeap(RtlGetProcessHeap(), 0, LinkDrive.Buffer);
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpNtLinkTarget);
    RtlFreeUnicodeString(&LinkTarget);
    DPRINT1("Unable to open link destination \"%wZ\"!\n", &LinkTarget);
    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
  }
  
  if(!RtlDosPathNameToNtPathName_U((LPWSTR)lpFileName, &LinkName, NULL, NULL))
  {
    ZwClose(hTarget);
    ZwClose(hFile);
    RtlFreeHeap(RtlGetProcessHeap(), 0, LinkDrive.Buffer);
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpNtLinkTarget);
    RtlFreeUnicodeString(&LinkTarget);
    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
  }
  
  NeededSize = sizeof(FILE_LINK_INFORMATION) + LinkName.Length + sizeof(WCHAR);
  if(!(LinkInformation = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, NeededSize)))
  {
    ZwClose(hTarget);
    ZwClose(hFile);
    RtlFreeHeap(RtlGetProcessHeap(), 0, LinkDrive.Buffer);
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpNtLinkTarget);
    RtlFreeUnicodeString(&LinkTarget);
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  
  LinkInformation->ReplaceIfExists = FALSE;
  LinkInformation->RootDirectory = 0;
  LinkInformation->FileNameLength = LinkName.Length;
  RtlCopyMemory(LinkInformation->FileName, LinkName.Buffer, LinkName.Length);
  
  Status = ZwSetInformationFile(hTarget, &IoStatus, LinkInformation, NeededSize, FileLinkInformation);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
  }
  
  ZwClose(hTarget);
  ZwClose(hFile);
  RtlFreeHeap(RtlGetProcessHeap(), 0, LinkInformation);
  RtlFreeHeap(RtlGetProcessHeap(), 0, LinkDrive.Buffer);
  RtlFreeHeap(RtlGetProcessHeap(), 0, lpNtLinkTarget);
  RtlFreeUnicodeString(&LinkTarget);
  return NT_SUCCESS(Status);
}


/*
 * @implemented
 */
BOOL STDCALL
CreateHardLinkA(
  LPCSTR lpFileName,
  LPCSTR lpExistingFileName,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes
)
{
  ANSI_STRING FileNameA, ExistingFileNameA;
  UNICODE_STRING FileName, ExistingFileName;
  NTSTATUS Status;
  BOOL Ret;
  
  if(!lpFileName || !lpExistingFileName)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  RtlInitAnsiString(&FileNameA, (LPSTR)lpFileName);
  RtlInitAnsiString(&ExistingFileNameA, (LPSTR)lpExistingFileName);
  
  if(bIsFileApiAnsi)
    Status = RtlAnsiStringToUnicodeString(&FileName, &FileNameA, TRUE);
  else
    Status = RtlOemStringToUnicodeString(&FileName, &FileNameA, TRUE);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  
  if(bIsFileApiAnsi)
    Status = RtlAnsiStringToUnicodeString(&ExistingFileName, &ExistingFileNameA, TRUE);
  else
    Status = RtlOemStringToUnicodeString(&ExistingFileName, &ExistingFileNameA, TRUE);
  if(!NT_SUCCESS(Status))
  {
    RtlFreeUnicodeString(&FileName);
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  
  Ret = CreateHardLinkW(FileName.Buffer, ExistingFileName.Buffer, lpSecurityAttributes);
  
  RtlFreeUnicodeString(&FileName);
  RtlFreeUnicodeString(&ExistingFileName);
  
  return Ret;
}

/* EOF */
