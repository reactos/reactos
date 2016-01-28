/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * PROGRAMMER: Royce Mitchell III
 *             Eric Kohl
 *             Ge van Geldorp <gvg@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "inflib.h"
#include "infros.h"

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS ********************************************************/

static int InfpHeapRefCount;

static VOID
CheckHeap(VOID)
{
  if (NULL == InfpHeap)
    {
      InfpHeap = RtlCreateHeap(HEAP_GROWABLE, NULL, 0, 0, NULL, NULL);
    }
  if (0 <= InfpHeapRefCount)
    {
      InfpHeapRefCount++;
    }
}


/* PUBLIC FUNCTIONS *********************************************************/

PVOID InfpHeap;

VOID
InfSetHeap(PVOID Heap)
{
  if (NULL == InfpHeap)
    {
      InfpHeap = Heap;
      InfpHeapRefCount = -1;
    }
}


NTSTATUS
InfOpenBufferedFile(PHINF InfHandle,
                    PVOID Buffer,
                    ULONG BufferSize,
                    LANGID LanguageId,
                    PULONG ErrorLine)
{
  INFSTATUS Status;
  PINFCACHE Cache;
  PCHAR FileBuffer;
  ULONG FileBufferSize;

  CheckHeap();

  *InfHandle = NULL;
  *ErrorLine = (ULONG)-1;

  /* Allocate file buffer */
  FileBufferSize = BufferSize + 2;
  FileBuffer = MALLOC(FileBufferSize);
  if (FileBuffer == NULL)
    {
      DPRINT1("MALLOC() failed\n");
      return(INF_STATUS_INSUFFICIENT_RESOURCES);
    }

  MEMCPY(FileBuffer, Buffer, BufferSize);

  /* Append string terminator */
  FileBuffer[BufferSize] = 0;
  FileBuffer[BufferSize + 1] = 0;

  /* Allocate infcache header */
  Cache = (PINFCACHE)MALLOC(sizeof(INFCACHE));
  if (Cache == NULL)
    {
      DPRINT("MALLOC() failed\n");
      FREE(FileBuffer);
      return(INF_STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Initialize inicache header */
  ZEROMEMORY(Cache,
             sizeof(INFCACHE));

    Cache->LanguageId = LanguageId;

    /* Parse the inf buffer */
    if (!RtlIsTextUnicode(FileBuffer, FileBufferSize, NULL))
    {
//        static const BYTE utf8_bom[3] = { 0xef, 0xbb, 0xbf };
        WCHAR *new_buff;
//        UINT codepage = CP_ACP;
        UINT offset = 0;

//        if (BufferSize > sizeof(utf8_bom) && !memcmp(FileBuffer, utf8_bom, sizeof(utf8_bom) ))
//        {
//            codepage = CP_UTF8;
//            offset = sizeof(utf8_bom);
//        }

        new_buff = MALLOC(FileBufferSize * sizeof(WCHAR));
        if (new_buff != NULL)
        {
            ULONG len;
            Status = RtlMultiByteToUnicodeN(new_buff,
                                            FileBufferSize * sizeof(WCHAR),
                                            &len,
                                            (char *)FileBuffer + offset,
                                            FileBufferSize - offset);

            Status = InfpParseBuffer(Cache,
                                     new_buff,
                                     new_buff + len / sizeof(WCHAR),
                                     ErrorLine);
            FREE(new_buff);
        }
        else
            Status = INF_STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        WCHAR *new_buff = (WCHAR *)FileBuffer;
        /* UCS-16 files should start with the Unicode BOM; we should skip it */
        if (*new_buff == 0xfeff)
        {
            new_buff++;
            FileBufferSize -= sizeof(WCHAR);
        }
        Status = InfpParseBuffer(Cache,
                                 new_buff,
                                 (WCHAR*)((char*)new_buff + FileBufferSize),
                                 ErrorLine);
    }

  if (!INF_SUCCESS(Status))
    {
      FREE(Cache);
      Cache = NULL;
    }

  /* Free file buffer */
  FREE(FileBuffer);

  *InfHandle = (HINF)Cache;

  return(Status);
}


NTSTATUS
InfOpenFile(PHINF InfHandle,
	    PUNICODE_STRING FileName,
	    LANGID LanguageId,
	    PULONG ErrorLine)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  FILE_STANDARD_INFORMATION FileInfo;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  NTSTATUS Status;
  PCHAR FileBuffer;
  ULONG FileLength;
  ULONG FileBufferLength;
  LARGE_INTEGER FileOffset;
  PINFCACHE Cache;

  CheckHeap();

  *InfHandle = NULL;
  *ErrorLine = (ULONG)-1;

  /* Open the inf file */
  InitializeObjectAttributes(&ObjectAttributes,
			     FileName,
			     0,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      GENERIC_READ | SYNCHRONIZE,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      FILE_SHARE_READ,
		      FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
  if (!INF_SUCCESS(Status))
    {
      DPRINT("NtOpenFile() failed (Status %lx)\n", Status);
      return(Status);
    }

  DPRINT("NtOpenFile() successful\n");

  /* Query file size */
  Status = NtQueryInformationFile(FileHandle,
				  &IoStatusBlock,
				  &FileInfo,
				  sizeof(FILE_STANDARD_INFORMATION),
				  FileStandardInformation);
  if (!INF_SUCCESS(Status))
    {
      DPRINT("NtQueryInformationFile() failed (Status %lx)\n", Status);
      NtClose(FileHandle);
      return(Status);
    }

  FileLength = FileInfo.EndOfFile.u.LowPart;

  DPRINT("File size: %lu\n", FileLength);

  /* Allocate file buffer */
  FileBufferLength = FileLength + 2;
  FileBuffer = MALLOC(FileBufferLength);
  if (FileBuffer == NULL)
    {
      DPRINT1("MALLOC() failed\n");
      NtClose(FileHandle);
      return(INF_STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Read file */
  FileOffset.QuadPart = 0ULL;
  Status = NtReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      FileBuffer,
		      FileLength,
		      &FileOffset,
		      NULL);

  /* Append string terminator */
  FileBuffer[FileLength] = 0;
  FileBuffer[FileLength + 1] = 0;

  NtClose(FileHandle);

  if (!INF_SUCCESS(Status))
    {
      DPRINT("NtReadFile() failed (Status %lx)\n", Status);
      FREE(FileBuffer);
      return(Status);
    }

  /* Allocate infcache header */
  Cache = (PINFCACHE)MALLOC(sizeof(INFCACHE));
  if (Cache == NULL)
    {
      DPRINT("MALLOC() failed\n");
      FREE(FileBuffer);
      return(INF_STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Initialize inicache header */
  ZEROMEMORY(Cache,
             sizeof(INFCACHE));

    Cache->LanguageId = LanguageId;

    /* Parse the inf buffer */
    if (!RtlIsTextUnicode(FileBuffer, FileBufferLength, NULL))
    {
//        static const BYTE utf8_bom[3] = { 0xef, 0xbb, 0xbf };
        WCHAR *new_buff;
//        UINT codepage = CP_ACP;
        UINT offset = 0;

//        if (FileLength > sizeof(utf8_bom) && !memcmp(FileBuffer, utf8_bom, sizeof(utf8_bom) ))
//        {
//            codepage = CP_UTF8;
//            offset = sizeof(utf8_bom);
//        }

        new_buff = MALLOC(FileBufferLength * sizeof(WCHAR));
        if (new_buff != NULL)
        {
            ULONG len;
            Status = RtlMultiByteToUnicodeN(new_buff,
                                            FileBufferLength * sizeof(WCHAR),
                                            &len,
                                            (char *)FileBuffer + offset,
                                            FileBufferLength - offset);

            Status = InfpParseBuffer(Cache,
                                     new_buff,
                                     new_buff + len / sizeof(WCHAR),
                                     ErrorLine);
            FREE(new_buff);
        }
        else
            Status = INF_STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        WCHAR *new_buff = (WCHAR *)FileBuffer;
        /* UCS-16 files should start with the Unicode BOM; we should skip it */
        if (*new_buff == 0xfeff)
        {
            new_buff++;
            FileBufferLength -= sizeof(WCHAR);
        }
        Status = InfpParseBuffer(Cache,
                                 new_buff,
                                 (WCHAR*)((char*)new_buff + FileBufferLength),
                                 ErrorLine);
    }

  if (!INF_SUCCESS(Status))
    {
      FREE(Cache);
      Cache = NULL;
    }

  /* Free file buffer */
  FREE(FileBuffer);

  *InfHandle = (HINF)Cache;

  return(Status);
}


VOID
InfCloseFile(HINF InfHandle)
{
  PINFCACHE Cache;

  Cache = (PINFCACHE)InfHandle;

  if (Cache == NULL)
    {
      return;
    }

  while (Cache->FirstSection != NULL)
    {
      Cache->FirstSection = InfpFreeSection(Cache->FirstSection);
    }
  Cache->LastSection = NULL;

  FREE(Cache);

  if (0 < InfpHeapRefCount)
    {
      InfpHeapRefCount--;
      if (0 == InfpHeapRefCount)
        {
          RtlDestroyHeap(InfpHeap);
          InfpHeap = NULL;
        }
    }
}


/* EOF */
