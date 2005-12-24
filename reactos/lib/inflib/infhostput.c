/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * COPYRIGHT:  Copyright 2005 Ge van Geldorp <gvg@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "inflib.h"
#include "infhost.h"

#define NDEBUG
#include <debug.h>

int
InfHostWriteFile(HINF InfHandle, const char *FileName,
                 const char *HeaderComment)
{
  char *Buffer;
  unsigned long BufferSize;
  INFSTATUS Status;
  FILE *File;

  Status = InfpBuildFileBuffer((PINFCACHE) InfHandle, &Buffer, &BufferSize);
  if (! INF_SUCCESS(Status))
    {
      errno = Status;
      return -1;
    }

  File = fopen(FileName, "wb");
  if (NULL == File)
    {
      FREE(Buffer);
      DPRINT1("fopen() failed (errno %d)\n", errno);
      return -1;
    }

  DPRINT("fopen() successful\n");

  if (NULL != HeaderComment && '\0' != *HeaderComment)
    {
      fprintf(File, "; %s\r\n\r\n", HeaderComment);
    }

  if (BufferSize != fwrite(Buffer, 1, BufferSize, File))
    {
      DPRINT1("fwrite() failed (errno %d)\n", errno);
      fclose(File);
      FREE(Buffer);
      return -1;
    }

  fclose(File);

  FREE(Buffer);

  return 0;
}

int
InfHostFindOrAddSection(HINF InfHandle,
                        const char *Section,
                        PINFCONTEXT *Context)
{
  INFSTATUS Status;

  Status = InfpFindOrAddSection((PINFCACHE) InfHandle, Section, Context);
  if (INF_SUCCESS(Status))
    {
      return 0;
    }
  else
    {
      errno = Status;
      return -1;
    }
}

int
InfHostAddLine(PINFCONTEXT Context, const char *Key)
{
  INFSTATUS Status;

  Status = InfpAddLineWithKey(Context, Key);
  if (INF_SUCCESS(Status))
    {
      return 0;
    }
  else
    {
      errno = Status;
      return -1;
    }
}

int
InfHostAddField(PINFCONTEXT Context, const char *Data)
{
  INFSTATUS Status;

  Status = InfpAddField(Context, Data);
  if (INF_SUCCESS(Status))
    {
      return 0;
    }
  else
    {
      errno = Status;
      return -1;
    }
}

/* EOF */
