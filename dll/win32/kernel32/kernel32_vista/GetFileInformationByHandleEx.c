
#include "k32_vista.h"

#include <ndk/rtlfuncs.h>
#include <ndk/iofuncs.h>
#include <winioctl.h>

/* Taken from Wine kernel32/file.c */

/***********************************************************************
*             GetFileInformationByHandleEx (KERNEL32.@)
*
*  Copyright 2023, Dibyamartanda Samanta <dibya.samanta@neverseen.de>
*  License: GPL 2.1/ Apache 2.0 
*/
typedef struct _IO_STATUS_BLOCK
{
   union {
      NTSTATUS Status;
      PVOID Pointer;
   } DUMMYUNIONNAME;

   ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef NTSTATUS (CALLBACK *pfnNtQueryDirectoryFile)(
   HANDLE                 FileHandle,
   HANDLE                 Event,
   PVOID                  ApcRoutine,
   PVOID                  ApcContext,
   PIO_STATUS_BLOCK       IoStatusBlock,
   PVOID                  FileInformation,
   ULONG                  Length,
   FILE_INFORMATION_CLASS FileInformationClass,
   BOOLEAN                ReturnSingleEntry,
   WCHAR*                 FileName,
   BOOLEAN                RestartScan);

static pfnNtQueryDirectoryFile fnNtQueryDirectoryFile = nullptr;

typedef NTSTATUS (CALLBACK *pfnNtQueryInformationFile)(
   HANDLE                 FileHandle,
   PIO_STATUS_BLOCK       IoStatusBlock,
   PVOID                  FileInformation,
   ULONG                  Length,
   FILE_INFORMATION_CLASS FileInformationClass);

static pfnNtQueryInformationFile fnNtQueryInformationFile = nullptr;

static BOOL bNtdllInited = false;

static BOOL WINAPI GetFileInformationByHandleEx(
   _In_  HANDLE hFile,
   _In_  FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
   _Out_writes_bytes_(dwBufferSize) LPVOID lpFileInformation,
   _In_  DWORD dwBufferSize)
{
   FILE_INFORMATION_CLASS NtFileInformationClass;
   DWORD cbMinBufferSize;
   BOOLEAN RestartScan = false;

   switch (FileInformationClass) // Purosham Sahanu InformationClass 
   {
   case FileBasicInfo:
      NtFileInformationClass = FileBasicInformation;
      cbMinBufferSize = sizeof(FILE_BASIC_INFO);
      break;
   case FileStandardInfo:
      NtFileInformationClass = FileStandardInformation;
      cbMinBufferSize = sizeof(FILE_STANDARD_INFO);
      break;
   case FileNameInfo:
      NtFileInformationClass = FileNameInformation;
      cbMinBufferSize = sizeof(FILE_NAME_INFO);
      break;
   case FileStreamInfo:
      NtFileInformationClass = FileStreamInformation;
      cbMinBufferSize = sizeof(FILE_STREAM_INFO);
      break;
   case FileCompressionInfo:
      NtFileInformationClass = FileCompressionInformation;
      cbMinBufferSize = sizeof(FILE_COMPRESSION_INFO);
      break;
   default:
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
      break;
   }

   if (cbMinBufferSize > dwBufferSize) {
      SetLastError(ERROR_BAD_LENGTH);
      return FALSE;
   }

   if (!bNtdllInited) {
      bNtdllInited = true;

      HINSTANCE hdll = LoadLibraryA("ntdll.dll");
      fnNtQueryDirectoryFile = (pfnNtQueryDirectoryFile)GetProcAddress(hdll, "NtQueryDirectoryFile");
      fnNtQueryInformationFile = (pfnNtQueryInformationFile)GetProcAddress(hdll, "NtQueryInformationFile");
   }

   int Status = ERROR_SUCCESS;
   IO_STATUS_BLOCK IoStatusBlock;
   if (fnNtQueryDirectoryFile) {
      Status = fnNtQueryDirectoryFile(
         hFile,
         nullptr,
         nullptr,
         nullptr,
         &IoStatusBlock,
         lpFileInformation,
         dwBufferSize,
         NtFileInformationClass,
         false,
         nullptr,
         RestartScan
      );

      if (STATUS_PENDING == Status) {
         if (WaitForSingleObjectEx(hFile, 0, FALSE) == WAIT_FAILED) {
            return FALSE;
         }

         Status = IoStatusBlock.Status;
      }
      else if (Status == STATUS_INVALID_INFO_CLASS)
         goto LBL_CheckFile;
   }
   else {
LBL_CheckFile:
      if (!fnNtQueryInformationFile) {
         SetLastError(ERROR_INVALID_FUNCTION);
         return FALSE;
      }

      Status = fnNtQueryInformationFile(hFile, &IoStatusBlock, lpFileInformation, dwBufferSize, NtFileInformationClass);
   }

   if (Status >= ERROR_SUCCESS) {
      if (FileStreamInfo == FileInformationClass && IoStatusBlock.Information == 0) {
         SetLastError(ERROR_HANDLE_EOF);
         return FALSE;
      }
      else {
         return TRUE;
      }
   }
   else {
      SetLastError(Status);
      return FALSE;
   }
}

   
