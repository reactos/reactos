/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * PROGRAMMER: Royce Mitchell III
 *             Eric Kohl
 *             Ge van Geldorp <gvg@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "inflib.h"
#include "infhost.h"

#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS *********************************************************/

int
InfHostOpenBufferedFile(PHINF InfHandle,
                        void *Buffer,
                        ULONG BufferSize,
                        LANGID LanguageId,
                        ULONG *ErrorLine)
{
  INFSTATUS Status;
  PINFCACHE Cache;
  WCHAR *FileBuffer;
  ULONG FileBufferSize;

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
      DPRINT1("MALLOC() failed\n");
      FREE(FileBuffer);
      return(INF_STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Initialize inicache header */
  ZEROMEMORY(Cache,
             sizeof(INFCACHE));

    Cache->LanguageId = LanguageId;

  /* Parse the inf buffer */
    if (!RtlIsTextUnicode(FileBuffer, (INT)FileBufferSize, NULL))
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

  return INF_SUCCESS(Status) ? 0 : -1;
}


int
InfHostOpenFile(PHINF InfHandle,
                const CHAR *FileName,
                LANGID LanguageId,
                ULONG *ErrorLine)
{
  FILE *File;
  CHAR *FileBuffer;
  ULONG FileLength;
  ULONG FileBufferLength;
  PINFCACHE Cache;
  INFSTATUS Status = INF_STATUS_SUCCESS;

  *InfHandle = NULL;
  *ErrorLine = (ULONG)-1;

  /* Open the inf file */
  File = fopen(FileName, "rb");
  if (NULL == File)
    {
      DPRINT1("fopen() failed (errno %d)\n", errno);
      return -1;
    }

  DPRINT("fopen() successful\n");

  /* Query file size */
  if (fseek(File, (size_t)0, SEEK_END))
    {
      DPRINT1("fseek() to EOF failed (errno %d)\n", errno);
      fclose(File);
      return -1;
    }

  FileLength = (ULONG)ftell(File);
  if ((ULONG) -1 == FileLength)
    {
      DPRINT1("ftell() failed (errno %d)\n", errno);
      fclose(File);
      return -1;
    }
  DPRINT("File size: %u\n", (UINT)FileLength);

  /* Rewind */
  if (fseek(File, (size_t)0, SEEK_SET))
    {
      DPRINT1("fseek() to BOF failed (errno %d)\n", errno);
      fclose(File);
      return -1;
    }

  /* Allocate file buffer */
  FileBufferLength = FileLength + 2;
  FileBuffer = MALLOC(FileBufferLength);
  if (FileBuffer == NULL)
    {
      DPRINT1("MALLOC() failed\n");
      fclose(File);
      return -1;
    }

  /* Read file */
  if (FileLength != fread(FileBuffer, (size_t)1, (size_t)FileLength, File))
    {
      DPRINT1("fread() failed (errno %d)\n", errno);
      fclose(File);
      return -1;
    }

  fclose(File);

  /* Append string terminator */
  FileBuffer[FileLength] = 0;
  FileBuffer[FileLength + 1] = 0;

  /* Allocate infcache header */
  Cache = (PINFCACHE)MALLOC(sizeof(INFCACHE));
  if (Cache == NULL)
    {
      DPRINT1("MALLOC() failed\n");
      FREE(FileBuffer);
      return -1;
    }

  /* Initialize inicache header */
  ZEROMEMORY(Cache,
             sizeof(INFCACHE));

    Cache->LanguageId = LanguageId;

  /* Parse the inf buffer */
    if (!RtlIsTextUnicode(FileBuffer, (INT)FileBufferLength, NULL))
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

  return INF_SUCCESS(Status) ? 0 : -1;
}


void
InfHostCloseFile(HINF InfHandle)
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
}

/* EOF */
