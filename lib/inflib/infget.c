/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * PROGRAMMER: Royce Mitchell III
 *             Eric Kohl
 *             Ge van Geldorp <gvg@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "inflib.h"

#define NDEBUG
#include <debug.h>


INFSTATUS
InfpFindFirstLine(PINFCACHE Cache,
                  PCTSTR Section,
                  PCTSTR Key,
                  PINFCONTEXT *Context)
{
  PINFCACHESECTION CacheSection;
  PINFCACHELINE CacheLine;

  if (Cache == NULL || Section == NULL || Context == NULL)
    {
      DPRINT1("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  CacheSection = InfpFindSection(Cache, Section);
  if (NULL == CacheSection)
    {
      DPRINT("Section not found\n");
      return INF_STATUS_NOT_FOUND;
    }

  if (Key != NULL)
    {
      CacheLine = InfpFindKeyLine(CacheSection, Key);
    }
  else
    {
      CacheLine = CacheSection->FirstLine;
    }

  if (NULL == CacheLine)
    {
      DPRINT("Key not found\n");
      return INF_STATUS_NOT_FOUND;
    }

  *Context = MALLOC(sizeof(INFCONTEXT));
  if (NULL == *Context)
    {
      DPRINT1("MALLOC() failed\n");
      return INF_STATUS_NO_MEMORY;
    }
  (*Context)->Inf = (PVOID)Cache;
  (*Context)->Section = (PVOID)CacheSection;
  (*Context)->Line = (PVOID)CacheLine;

  return INF_STATUS_SUCCESS;
}


INFSTATUS
InfpFindNextLine(PINFCONTEXT ContextIn,
                 PINFCONTEXT ContextOut)
{
  PINFCACHELINE CacheLine;

  if (ContextIn == NULL || ContextOut == NULL)
    return INF_STATUS_INVALID_PARAMETER;

  if (ContextIn->Line == NULL)
    return INF_STATUS_INVALID_PARAMETER;

  CacheLine = (PINFCACHELINE)ContextIn->Line;
  if (CacheLine->Next == NULL)
    return INF_STATUS_NOT_FOUND;

  if (ContextIn != ContextOut)
    {
      ContextOut->Inf = ContextIn->Inf;
      ContextOut->Section = ContextIn->Section;
    }
  ContextOut->Line = (PVOID)(CacheLine->Next);

  return INF_STATUS_SUCCESS;
}


INFSTATUS
InfpFindFirstMatchLine(PINFCONTEXT ContextIn,
                       PCTSTR Key,
                       PINFCONTEXT ContextOut)
{
  PINFCACHELINE CacheLine;

  if (ContextIn == NULL || ContextOut == NULL || Key == NULL || *Key == 0)
    return INF_STATUS_INVALID_PARAMETER;

  if (ContextIn->Inf == NULL || ContextIn->Section == NULL)
    return INF_STATUS_INVALID_PARAMETER;

  CacheLine = ((PINFCACHESECTION)(ContextIn->Section))->FirstLine;
  while (CacheLine != NULL)
    {
      if (CacheLine->Key != NULL && _tcsicmp (CacheLine->Key, Key) == 0)
        {

          if (ContextIn != ContextOut)
            {
              ContextOut->Inf = ContextIn->Inf;
              ContextOut->Section = ContextIn->Section;
            }
          ContextOut->Line = (PVOID)CacheLine;

          return INF_STATUS_SUCCESS;
        }

      CacheLine = CacheLine->Next;
    }

  return INF_STATUS_NOT_FOUND;
}


INFSTATUS
InfpFindNextMatchLine(PINFCONTEXT ContextIn,
                      PCTSTR Key,
                      PINFCONTEXT ContextOut)
{
  PINFCACHELINE CacheLine;

  if (ContextIn == NULL || ContextOut == NULL || Key == NULL || *Key == 0)
    return INF_STATUS_INVALID_PARAMETER;

  if (ContextIn->Inf == NULL || ContextIn->Section == NULL || ContextIn->Line == NULL)
    return INF_STATUS_INVALID_PARAMETER;

  CacheLine = (PINFCACHELINE)ContextIn->Line;
  while (CacheLine != NULL)
    {
      if (CacheLine->Key != NULL && _tcsicmp (CacheLine->Key, Key) == 0)
        {

          if (ContextIn != ContextOut)
            {
              ContextOut->Inf = ContextIn->Inf;
              ContextOut->Section = ContextIn->Section;
            }
          ContextOut->Line = (PVOID)CacheLine;

          return INF_STATUS_SUCCESS;
        }

      CacheLine = CacheLine->Next;
    }

  return INF_STATUS_NOT_FOUND;
}


LONG
InfpGetLineCount(HINF InfHandle,
                 PCTSTR Section)
{
  PINFCACHE Cache;
  PINFCACHESECTION CacheSection;

  if (InfHandle == NULL || Section == NULL)
    {
      DPRINT("Invalid parameter\n");
      return -1;
    }

  Cache = (PINFCACHE)InfHandle;

  /* Iterate through list of sections */
  CacheSection = Cache->FirstSection;
  while (CacheSection != NULL)
    {
      /* Are the section names the same? */
      if (_tcsicmp(CacheSection->Name, Section) == 0)
        {
          return CacheSection->LineCount;
        }

      /* Get the next section */
      CacheSection = CacheSection->Next;
    }

  DPRINT("Section not found\n");

  return -1;
}


/* InfpGetLineText */


LONG
InfpGetFieldCount(PINFCONTEXT Context)
{
  if (Context == NULL || Context->Line == NULL)
    return 0;

  return ((PINFCACHELINE)Context->Line)->FieldCount;
}


INFSTATUS
InfpGetBinaryField(PINFCONTEXT Context,
                   ULONG FieldIndex,
                   PUCHAR ReturnBuffer,
                   ULONG ReturnBufferSize,
                   PULONG RequiredSize)
{
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  ULONG Index;
  ULONG Size;
  PUCHAR Ptr;

  if (Context == NULL || Context->Line == NULL || FieldIndex == 0)
    {
      DPRINT("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  if (RequiredSize != NULL)
    *RequiredSize = 0;

  CacheLine = (PINFCACHELINE)Context->Line;

  if (FieldIndex > (ULONG)CacheLine->FieldCount)
    return INF_STATUS_NOT_FOUND;

  CacheField = CacheLine->FirstField;
  for (Index = 1; Index < FieldIndex; Index++)
    CacheField = CacheField->Next;

  Size = CacheLine->FieldCount - FieldIndex + 1;

  if (RequiredSize != NULL)
    *RequiredSize = Size;

  if (ReturnBuffer != NULL)
    {
      if (ReturnBufferSize < Size)
        return INF_STATUS_BUFFER_OVERFLOW;

      /* Copy binary data */
      Ptr = ReturnBuffer;
      while (CacheField != NULL)
        {
          *Ptr = (UCHAR)_tcstoul (CacheField->Data, NULL, 16);

          Ptr++;
          CacheField = CacheField->Next;
        }
    }

  return INF_STATUS_SUCCESS;
}


INFSTATUS
InfpGetIntField(PINFCONTEXT Context,
                ULONG FieldIndex,
                PLONG IntegerValue)
{
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  ULONG Index;
  PTCHAR Ptr;

  if (Context == NULL || Context->Line == NULL || IntegerValue == NULL)
    {
      DPRINT("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  CacheLine = (PINFCACHELINE)Context->Line;

  if (FieldIndex > (ULONG)CacheLine->FieldCount)
    {
      DPRINT("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  if (FieldIndex == 0)
    {
      Ptr = CacheLine->Key;
    }
  else
    {
      CacheField = CacheLine->FirstField;
      for (Index = 1; Index < FieldIndex; Index++)
        CacheField = CacheField->Next;

      Ptr = CacheField->Data;
    }

  *IntegerValue = _tcstol(Ptr, NULL, 0);

  return INF_STATUS_SUCCESS;
}


INFSTATUS
InfpGetMultiSzField(PINFCONTEXT Context,
                    ULONG FieldIndex,
                    PTSTR ReturnBuffer,
                    ULONG ReturnBufferSize,
                    PULONG RequiredSize)
{
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  PINFCACHEFIELD FieldPtr;
  ULONG Index;
  ULONG Size;
  PTCHAR Ptr;

  if (Context == NULL || Context->Line == NULL || FieldIndex == 0)
    {
      DPRINT("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  if (RequiredSize != NULL)
    *RequiredSize = 0;

  CacheLine = (PINFCACHELINE)Context->Line;

  if (FieldIndex > (ULONG)CacheLine->FieldCount)
    return INF_STATUS_INVALID_PARAMETER;

  CacheField = CacheLine->FirstField;
  for (Index = 1; Index < FieldIndex; Index++)
    CacheField = CacheField->Next;

  /* Calculate the required buffer size */
  FieldPtr = CacheField;
  Size = 0;
  while (FieldPtr != NULL)
    {
      Size += (_tcslen (FieldPtr->Data) + 1);
      FieldPtr = FieldPtr->Next;
    }
  Size++;

  if (RequiredSize != NULL)
    *RequiredSize = Size;

  if (ReturnBuffer != NULL)
    {
      if (ReturnBufferSize < Size)
        return INF_STATUS_BUFFER_OVERFLOW;

      /* Copy multi-sz string */
      Ptr = ReturnBuffer;
      FieldPtr = CacheField;
      while (FieldPtr != NULL)
        {
          Size = _tcslen (FieldPtr->Data) + 1;

          _tcscpy (Ptr, FieldPtr->Data);

          Ptr = Ptr + Size;
          FieldPtr = FieldPtr->Next;
        }
      *Ptr = 0;
    }

  return INF_STATUS_SUCCESS;
}


INFSTATUS
InfpGetStringField(PINFCONTEXT Context,
                   ULONG FieldIndex,
                   PTSTR ReturnBuffer,
                   ULONG ReturnBufferSize,
                   PULONG RequiredSize)
{
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  ULONG Index;
  PTCHAR Ptr;
  ULONG Size;

  if (Context == NULL || Context->Line == NULL || FieldIndex == 0)
    {
      DPRINT("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  if (RequiredSize != NULL)
    *RequiredSize = 0;

  CacheLine = (PINFCACHELINE)Context->Line;

  if (FieldIndex > (ULONG)CacheLine->FieldCount)
    return INF_STATUS_INVALID_PARAMETER;

  if (FieldIndex == 0)
    {
      Ptr = CacheLine->Key;
    }
  else
    {
      CacheField = CacheLine->FirstField;
      for (Index = 1; Index < FieldIndex; Index++)
        CacheField = CacheField->Next;

      Ptr = CacheField->Data;
    }

  Size = _tcslen (Ptr) + 1;

  if (RequiredSize != NULL)
    *RequiredSize = Size;

  if (ReturnBuffer != NULL)
    {
      if (ReturnBufferSize < Size)
        return INF_STATUS_BUFFER_OVERFLOW;

      _tcscpy (ReturnBuffer, Ptr);
    }

  return INF_STATUS_SUCCESS;
}


INFSTATUS
InfpGetData(PINFCONTEXT Context,
            PTCHAR *Key,
            PTCHAR *Data)
{
  PINFCACHELINE CacheKey;

  if (Context == NULL || Context->Line == NULL || Data == NULL)
    {
      DPRINT("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  CacheKey = (PINFCACHELINE)Context->Line;
  if (Key != NULL)
    *Key = CacheKey->Key;

  if (Data != NULL)
    {
      if (CacheKey->FirstField == NULL)
        {
          *Data = NULL;
        }
      else
        {
          *Data = CacheKey->FirstField->Data;
        }
    }

  return INF_STATUS_SUCCESS;
}


INFSTATUS
InfpGetDataField(PINFCONTEXT Context,
                 ULONG FieldIndex,
                 PTCHAR *Data)
{
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  ULONG Index;

  if (Context == NULL || Context->Line == NULL || Data == NULL)
    {
      DPRINT("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  CacheLine = (PINFCACHELINE)Context->Line;

  if (FieldIndex > (ULONG)CacheLine->FieldCount)
    return INF_STATUS_INVALID_PARAMETER;

  if (FieldIndex == 0)
    {
      *Data = CacheLine->Key;
    }
  else
    {
      CacheField = CacheLine->FirstField;
      for (Index = 1; Index < FieldIndex; Index++)
        CacheField = CacheField->Next;

      *Data = CacheField->Data;
    }

  return INF_STATUS_SUCCESS;
}

VOID
InfpFreeContext(PINFCONTEXT Context)
{
  FREE(Context);
}

/* EOF */
