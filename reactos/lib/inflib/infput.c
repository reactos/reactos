/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * COPYRIGHT:  Copyright 2005 Ge van Geldorp <gvg@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "inflib.h"

#define NDEBUG
#include <debug.h>

#define EOL      _T("\r\n")
#define SIZE_INC 1024

typedef struct _OUTPUTBUFFER
{
  PCHAR Buffer;
  PCHAR Current;
  ULONG TotalSize;
  ULONG FreeSize;
  INFSTATUS Status;
} OUTPUTBUFFER, *POUTPUTBUFFER;

static void
Output(POUTPUTBUFFER OutBuf, PCTSTR Text)
{
  ULONG Length;
  PCHAR NewBuf;
  ULONG NewSize;

  /* Skip mode? */
  if (! INF_SUCCESS(OutBuf->Status))
    {
      return;
    }

  /* Doesn't fit? */
  Length = _tcslen(Text);
  if (OutBuf->FreeSize < Length + 1 && INF_SUCCESS(OutBuf->Status))
    {
      DPRINT("Out of free space. TotalSize %lu FreeSize %lu Length %u\n",
             OutBuf->TotalSize, OutBuf->FreeSize, Length);
      /* Round up to next SIZE_INC */
      NewSize = OutBuf->TotalSize +
                (((Length + 1) - OutBuf->FreeSize + (SIZE_INC - 1)) /
                 SIZE_INC) * SIZE_INC;
      DPRINT("NewSize %lu\n", NewSize);
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
          DPRINT("Copying %lu bytes from old content\n",
                 OutBuf->TotalSize - OutBuf->FreeSize);
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
      DPRINT("After reallocation TotalSize %lu FreeSize %lu\n",
             OutBuf->TotalSize, OutBuf->FreeSize);
    }

  /* We're guaranteed to have enough room now. Copy char by char because of
     possible "conversion" from Unicode to Ansi */
  while (Length--)
    {
      *OutBuf->Current++ = (char) *Text++;
      OutBuf->FreeSize--;
    }
  *OutBuf->Current = '\0';
}

INFSTATUS
InfpBuildFileBuffer(PINFCACHE Cache,
                    PCHAR *Buffer,
                    PULONG BufferSize)
{
  OUTPUTBUFFER OutBuf;
  PINFCACHESECTION CacheSection;
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  PTCHAR p;
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
      DPRINT("Processing section " STRFMT "\n", CacheSection->Name);
      if (CacheSection != Cache->FirstSection)
        {
          Output(&OutBuf, EOL);
        }
      Output(&OutBuf, _T("["));
      Output(&OutBuf, CacheSection->Name);
      Output(&OutBuf, _T("]"));
      Output(&OutBuf, EOL);

      /* Iterate through list of lines */
      CacheLine = CacheSection->FirstLine;
      while (CacheLine != NULL)
        {
          if (NULL != CacheLine->Key)
            {
              DPRINT("Line with key " STRFMT "\n", CacheLine->Key);
              Output(&OutBuf, CacheLine->Key);
              Output(&OutBuf, _T(" = "));
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
                  Output(&OutBuf, _T(","));
                }
              p = CacheField->Data;
              NeedQuotes = FALSE;
              while (_T('\0') != *p && ! NeedQuotes)
                {
                  NeedQuotes = _T(',') == *p || _T(';') == *p ||
                               _T('\\') == *p;
                  p++;
                }
              if (NeedQuotes)
                {
                  Output(&OutBuf, _T("\""));
                  Output(&OutBuf, CacheField->Data);
                  Output(&OutBuf, _T("\""));
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
                     PCTSTR Section,
                     PINFCONTEXT *Context)
{
  DPRINT("InfpFindOrAddSection section " STRFMT "\n", Section);

  *Context = MALLOC(sizeof(INFCONTEXT));
  if (NULL == *Context)
    {
      DPRINT1("MALLOC() failed\n");
      return INF_STATUS_NO_MEMORY;
    }

  (*Context)->Inf = Cache;
  (*Context)->Section = InfpFindSection(Cache, Section);
  (*Context)->Line = NULL;
  if (NULL == (*Context)->Section)
    {
      DPRINT("Section not found, creating it\n");
      (*Context)->Section = InfpAddSection(Cache, Section);
      if (NULL == (*Context)->Section)
        {
          DPRINT("Failed to create section\n");
          FREE(*Context);
          return INF_STATUS_NO_MEMORY;
        }
    }

  return INF_STATUS_SUCCESS;
}

INFSTATUS
InfpAddLineWithKey(PINFCONTEXT Context, PCTSTR Key)
{
  if (NULL == Context)
    {
      DPRINT1("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  Context->Line = InfpAddLine(Context->Section);
  if (NULL == Context->Line)
    {
      DPRINT("Failed to create line\n");
      return INF_STATUS_NO_MEMORY;
    }

  if (NULL != Key && NULL == InfpAddKeyToLine(Context->Line, Key))
    {
      DPRINT("Failed to add key\n");
      return INF_STATUS_NO_MEMORY;
    }

  return INF_STATUS_SUCCESS;
}

INFSTATUS
InfpAddField(PINFCONTEXT Context, PCTSTR Data)
{
  if (NULL == Context || NULL == Context->Line)
    {
      DPRINT1("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  if (NULL == InfpAddFieldToLine(Context->Line, Data))
    {
      DPRINT("Failed to add field\n");
      return INF_STATUS_NO_MEMORY;
    }

  return INF_STATUS_SUCCESS;
}

/* EOF */
