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
                        unsigned long BufferSize,
                        unsigned long *ErrorLine)
{
  INFSTATUS Status;
  PINFCACHE Cache;
  char *FileBuffer;

  *InfHandle = NULL;
  *ErrorLine = (unsigned long)-1;

  /* Allocate file buffer */
  FileBuffer = MALLOC(BufferSize + 1);
  if (FileBuffer == NULL)
    {
      DPRINT1("MALLOC() failed\n");
      return(INF_STATUS_INSUFFICIENT_RESOURCES);
    }

  MEMCPY(FileBuffer, Buffer, BufferSize);

  /* Append string terminator */
  FileBuffer[BufferSize] = 0;

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

  /* Parse the inf buffer */
  Status = InfpParseBuffer (Cache,
			    FileBuffer,
			    FileBuffer + BufferSize,
			    ErrorLine);
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
                const char *FileName,
                unsigned long *ErrorLine)
{
  FILE *File;
  char *FileBuffer;
  unsigned long FileLength;
  PINFCACHE Cache;
  INFSTATUS Status;

  *InfHandle = NULL;
  *ErrorLine = (unsigned long)-1;

  /* Open the inf file */
  File = fopen(FileName, "rb");
  if (NULL == File)
    {
      DPRINT1("fopen() failed (errno %d)\n", errno);
      return -1;
    }

  DPRINT("fopen() successful\n");

  /* Query file size */
  if (fseek(File, 0, SEEK_END))
    {
      DPRINT1("fseek() to EOF failed (errno %d)\n", errno);
      fclose(File);
      return -1;
    }

  FileLength = ftell(File);
  if ((unsigned long) -1 == FileLength)
    {
      DPRINT1("ftell() failed (errno %d)\n", errno);
      fclose(File);
      return -1;
    }
  DPRINT("File size: %lu\n", FileLength);

  /* Rewind */
  if (fseek(File, 0, SEEK_SET))
    {
      DPRINT1("fseek() to BOF failed (errno %d)\n", errno);
      fclose(File);
      return -1;
    }

  /* Allocate file buffer */
  FileBuffer = MALLOC(FileLength + 1);
  if (FileBuffer == NULL)
    {
      DPRINT1("MALLOC() failed\n");
      fclose(File);
      return -1;
    }

  /* Read file */
  if (FileLength != fread(FileBuffer, 1, FileLength, File))
    {
      DPRINT1("fread() failed (errno %d)\n", errno);
      fclose(File);
      return -1;
    }

  fclose(File);

  /* Append string terminator */
  FileBuffer[FileLength] = 0;

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

  /* Parse the inf buffer */
  Status = InfpParseBuffer (Cache,
			    FileBuffer,
			    FileBuffer + FileLength,
			    ErrorLine);
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
