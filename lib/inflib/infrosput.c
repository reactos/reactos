/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * COPYRIGHT:  Copyright 2005 Ge van Geldorp <gvg@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "inflib.h"
#include "infros.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
InfWriteFile(HINF InfHandle,
             PUNICODE_STRING FileName,
             PUNICODE_STRING HeaderComment)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  NTSTATUS Status;
  INFSTATUS InfStatus;
  PCHAR Buffer;
  ULONG BufferSize;
  PCHAR HeaderBuffer;
  ULONG HeaderBufferSize;
  UINT Index;

  InfStatus = InfpBuildFileBuffer((PINFCACHE) InfHandle, &Buffer, &BufferSize);
  if (! INF_SUCCESS(InfStatus))
    {
      DPRINT("Failed to create buffer (Status 0x%lx)\n", InfStatus);
      return InfStatus;
    }

  /* Open the inf file */
  InitializeObjectAttributes(&ObjectAttributes,
                             FileName,
                             0,
                             NULL,
                             NULL);

  Status = NtOpenFile(&FileHandle,
                      GENERIC_WRITE | SYNCHRONIZE,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      0,
                      FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
  if (!INF_SUCCESS(Status))
    {
      DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
      FREE(Buffer);
      return Status;
    }

  DPRINT("NtOpenFile() successful\n");

  if (NULL != HeaderComment && 0 != HeaderComment->Length)
    {
      /* This is just a comment header, don't abort on errors here */
      HeaderBufferSize = HeaderComment->Length / sizeof(WCHAR) + 7;
      HeaderBuffer = MALLOC(HeaderBufferSize);
      if (NULL != HeaderBuffer)
        {
          strcpy(HeaderBuffer, "; ");
          for (Index = 0; Index < HeaderComment->Length / sizeof(WCHAR); Index++)
            {
              HeaderBuffer[2 + Index] = (CHAR) HeaderComment->Buffer[Index];
            }
          strcpy(HeaderBuffer + (2 + HeaderComment->Length / sizeof(WCHAR)),
                 "\r\n\r\n");
          NtWriteFile(FileHandle,
                      NULL,
                      NULL,
                      NULL,
                      &IoStatusBlock,
                      HeaderBuffer,
                      HeaderBufferSize,
                      NULL,
                      NULL);
          FREE(HeaderBuffer);
        }
    }

  /* Write main contents */
  Status = NtWriteFile(FileHandle,
                       NULL,
                       NULL,
                       NULL,
                       &IoStatusBlock,
                       Buffer,
                       BufferSize,
                       NULL,
                       NULL);

  NtClose(FileHandle);
  FREE(Buffer);

  if (!INF_SUCCESS(Status))
    {
      DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
      FREE(Buffer);
      return(Status);
    }

  return STATUS_SUCCESS;
}

BOOLEAN
InfFindOrAddSection(HINF InfHandle,
                    PCWSTR Section,
                    PINFCONTEXT *Context)
{
  return INF_SUCCESS(InfpFindOrAddSection((PINFCACHE) InfHandle,
                                          Section, Context));
}

BOOLEAN
InfHostAddLine(PINFCONTEXT Context, PCWSTR Key)
{
  return INF_SUCCESS(InfpAddLineWithKey(Context, Key));
}

BOOLEAN
InfHostAddField(PINFCONTEXT Context, PCWSTR Data)
{
  return INF_SUCCESS(InfpAddField(Context, Data));
}

/* EOF */
