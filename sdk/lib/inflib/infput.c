/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * COPYRIGHT:  Copyright 2005 Ge van Geldorp <gvg@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "inflib.h"

#define NDEBUG
#include <debug.h>

#define EOL      L"\r\n"
#define SIZE_INC 1024

typedef struct _OUTPUTBUFFER
{
  PWCHAR Buffer;
  PWCHAR Current;
  ULONG TotalSize;
  ULONG FreeSize;
  INFSTATUS Status;
} OUTPUTBUFFER, *POUTPUTBUFFER;

static void
Output(POUTPUTBUFFER OutBuf, PCWSTR Text)
{
  ULONG Length;
  PWCHAR NewBuf;
  ULONG NewSize;

  /* Skip mode? */
  if (! INF_SUCCESS(OutBuf->Status))
    {
      return;
    }

  /* Doesn't fit? */
  Length = (ULONG)strlenW(Text) * sizeof(WCHAR);
  if (OutBuf->FreeSize < Length + 1 && INF_SUCCESS(OutBuf->Status))
    {
      DPRINT("Out of free space. TotalSize %u FreeSize %u Length %u\n",
             (UINT)OutBuf->TotalSize, (UINT)OutBuf->FreeSize, (UINT)Length);
      /* Round up to next SIZE_INC */
      NewSize = OutBuf->TotalSize +
                (((Length + 1) - OutBuf->FreeSize + (SIZE_INC - 1)) /
                 SIZE_INC) * SIZE_INC;
      DPRINT("NewSize %u\n", (UINT)NewSize);
      NewBuf = MALLOC(NewSize);
      /* Abort if failed */
      if (NULL == NewBuf)
        {
          DPRINT1("MALLOC() failed\n");
          OutBuf->Status = INF_STATUS_NO_MEMORY;
          return;
        }

      /* Need to copy old contents? */
      if (NULL != OutBuf->Buffer)
        {
          DPRINT("Copying %u bytes from old content\n",
                 (UINT)(OutBuf->TotalSize - OutBuf->FreeSize));
          MEMCPY(NewBuf, OutBuf->Buffer, OutBuf->TotalSize - OutBuf->FreeSize);
          OutBuf->Current = NewBuf + (OutBuf->Current - OutBuf->Buffer);
          FREE(OutBuf->Buffer);
        }
      else
        {
          OutBuf->Current = NewBuf;
        }
      OutBuf->Buffer = NewBuf;
      OutBuf->FreeSize += NewSize - OutBuf->TotalSize;
      OutBuf->TotalSize = NewSize;
      DPRINT("After reallocation TotalSize %u FreeSize %u\n",
             (UINT)OutBuf->TotalSize, (UINT)OutBuf->FreeSize);
    }

  /* We're guaranteed to have enough room now. Copy char by char because of
     possible "conversion" from Unicode to Ansi */
  while (Length--)
    {
      *OutBuf->Current++ = *Text++;
      OutBuf->FreeSize--;
    }
  *OutBuf->Current = '\0';
}

INFSTATUS
InfpBuildFileBuffer(PINFCACHE Cache,
                    PWCHAR *Buffer,
                    PULONG BufferSize)
{
  OUTPUTBUFFER OutBuf;
  PINFCACHESECTION CacheSection;
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  PWCHAR p;
  BOOLEAN NeedQuotes;

  OutBuf.Buffer = NULL;
  OutBuf.Current = NULL;
  OutBuf.FreeSize = 0;
  OutBuf.TotalSize = 0;
  OutBuf.Status = INF_STATUS_SUCCESS;

  /* Iterate through list of sections */
  CacheSection = Cache->FirstSection;
  while (CacheSection != NULL)
    {
      DPRINT("Processing section %S\n", CacheSection->Name);
      if (CacheSection != Cache->FirstSection)
        {
          Output(&OutBuf, EOL);
        }
      Output(&OutBuf, L"[");
      Output(&OutBuf, CacheSection->Name);
      Output(&OutBuf, L"]");
      Output(&OutBuf, EOL);

      /* Iterate through list of lines */
      CacheLine = CacheSection->FirstLine;
      while (CacheLine != NULL)
        {
          if (NULL != CacheLine->Key)
            {
              DPRINT("Line with key %S\n", CacheLine->Key);
              Output(&OutBuf, CacheLine->Key);
              Output(&OutBuf, L" = ");
            }
          else
            {
              DPRINT("Line without key\n");
            }

          /* Iterate through list of lines */
          CacheField = CacheLine->FirstField;
          while (CacheField != NULL)
            {
              if (CacheField != CacheLine->FirstField)
                {
                  Output(&OutBuf, L",");
                }
              p = CacheField->Data;
              NeedQuotes = FALSE;
              while (L'\0' != *p && ! NeedQuotes)
                {
                  NeedQuotes = (BOOLEAN)(L',' == *p || L';' == *p ||
                                         L'\\' == *p);
                  p++;
                }
              if (NeedQuotes)
                {
                  Output(&OutBuf, L"\"");
                  Output(&OutBuf, CacheField->Data);
                  Output(&OutBuf, L"\"");
                }
              else
                {
                  Output(&OutBuf, CacheField->Data);
                }

              /* Get the next field */
              CacheField = CacheField->Next;
            }

          Output(&OutBuf, EOL);
          /* Get the next line */
          CacheLine = CacheLine->Next;
        }

      /* Get the next section */
      CacheSection = CacheSection->Next;
    }

  if (INF_SUCCESS(OutBuf.Status))
    {
      *Buffer = OutBuf.Buffer;
      *BufferSize = OutBuf.TotalSize - OutBuf.FreeSize;
    }
  else if (NULL != OutBuf.Buffer)
    {
      FREE(OutBuf.Buffer);
    }

  return INF_STATUS_SUCCESS;
}

INFSTATUS
InfpFindOrAddSection(PINFCACHE Cache,
                     PCWSTR Section,
                     PINFCONTEXT *Context)
{
  PINFCACHESECTION CacheSection;
  DPRINT("InfpFindOrAddSection section %S\n", Section);

  *Context = MALLOC(sizeof(INFCONTEXT));
  if (NULL == *Context)
    {
      DPRINT1("MALLOC() failed\n");
      return INF_STATUS_NO_MEMORY;
    }

  (*Context)->Inf = Cache;
  (*Context)->Line = 0;
  CacheSection = InfpFindSection(Cache, Section);
  if (NULL == CacheSection)
    {
      DPRINT("Section not found, creating it\n");
      CacheSection = InfpAddSection(Cache, Section);
      if (NULL == CacheSection)
        {
          DPRINT("Failed to create section\n");
          FREE(*Context);
          return INF_STATUS_NO_MEMORY;
        }
    }

  (*Context)->Section = CacheSection->Id;
  return INF_STATUS_SUCCESS;
}

INFSTATUS
InfpAddLineWithKey(PINFCONTEXT Context, PCWSTR Key)
{
  PINFCACHESECTION Section;
  PINFCACHELINE Line;

  if (NULL == Context)
    {
      DPRINT1("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  Section = InfpGetSectionForContext(Context);
  Line = InfpAddLine(Section);
  if (NULL == Line)
    {
      DPRINT("Failed to create line\n");
      return INF_STATUS_NO_MEMORY;
    }
  Context->Line = Line->Id;

  if (NULL != Key && NULL == InfpAddKeyToLine(Line, Key))
    {
      DPRINT("Failed to add key\n");
      return INF_STATUS_NO_MEMORY;
    }

  return INF_STATUS_SUCCESS;
}

INFSTATUS
InfpAddField(PINFCONTEXT Context, PCWSTR Data)
{
  PINFCACHELINE Line;

  if (NULL == Context)
    {
      DPRINT1("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  Line = InfpGetLineForContext(Context);
  if (NULL == InfpAddFieldToLine(Line, Data))
    {
      DPRINT("Failed to add field\n");
      return INF_STATUS_NO_MEMORY;
    }

  return INF_STATUS_SUCCESS;
}

/* EOF */
