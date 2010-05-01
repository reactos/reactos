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

static unsigned int
InfpSubstituteString(PINFCACHE Inf,
                     const WCHAR *text,
                     WCHAR *buffer,
                     unsigned int size);

/* retrieve the string substitution for a given string, or NULL if not found */
/* if found, len is set to the substitution length */
static PCWSTR
InfpGetSubstitutionString(PINFCACHE Inf,
                          PCWSTR str,
                          unsigned int *len,
                          BOOL no_trailing_slash)
{
    static const WCHAR percent = '%';

    INFSTATUS Status = INF_STATUS_NOT_FOUND;
    PINFCONTEXT Context = NULL;
    PWCHAR Data = NULL;
    WCHAR ValueName[MAX_INF_STRING_LENGTH +1];
    WCHAR StringLangId[13];

    if (!*len)  /* empty string (%%) is replaced by single percent */
    {
        *len = 1;
        return &percent;
    }

    strncpyW(ValueName, str, *len);
    ValueName[*len] = 0;

    DPRINT("Value name: %S\n", ValueName);

    if (Inf->LanguageId != 0)
    {
        swprintf(StringLangId,
                 L"Strings.%04hx",
                 Inf->LanguageId);

        Status = InfpFindFirstLine(Inf,
                                   StringLangId,
                                   ValueName,
                                   &Context);
        if (Status != INF_STATUS_SUCCESS)
        {
            swprintf(StringLangId,
                     L"Strings.%04hx",
                     MAKELANGID(PRIMARYLANGID(Inf->LanguageId), SUBLANG_NEUTRAL));

            Status = InfpFindFirstLine(Inf,
                                       StringLangId,
                                       ValueName,
                                       &Context);
            if (Status != INF_STATUS_SUCCESS)
            {
                Status = InfpFindFirstLine(Inf,
                                           L"Strings",
                                           ValueName,
                                           &Context);
            }
        }
    }
    else
    {
        Status = InfpFindFirstLine(Inf,
                                   L"Strings",
                                   ValueName,
                                   &Context);
    }

    if (Status != INF_STATUS_SUCCESS || Context == NULL)
        return NULL;

    Status = InfpGetData(Context,
                         NULL,
                         &Data);

    InfpFreeContext(Context);

    if (Status == STATUS_SUCCESS)
    {
        *len = strlenW(Data);
        DPRINT("Substitute: %S  Length: %ul\n", Data, *len);
        return Data;
    }

    return NULL;
}


/* do string substitutions on the specified text */
/* the buffer is assumed to be large enough */
/* returns necessary length not including terminating null */
static unsigned int
InfpSubstituteString(PINFCACHE Inf,
                     PCWSTR text,
                     PWSTR buffer,
                     unsigned int size)
{
    const WCHAR *start, *subst, *p;
    unsigned int len, total = 0;
    int inside = 0;

    if (!buffer) size = MAX_INF_STRING_LENGTH + 1;
    for (p = start = text; *p; p++)
    {
        if (*p != '%') continue;
        inside = !inside;
        if (inside)  /* start of a %xx% string */
        {
            len = (unsigned int)(p - start);
            if (len > size - 1) len = size - 1;
            if (buffer) memcpy( buffer + total, start, len * sizeof(WCHAR) );
            total += len;
            size -= len;
            start = p;
        }
        else /* end of the %xx% string, find substitution */
        {
            len = (unsigned int)(p - start - 1);
            subst = InfpGetSubstitutionString( Inf, start + 1, &len, p[1] == '\\' );
            if (!subst)
            {
                subst = start;
                len = (unsigned int)(p - start + 1);
            }
            if (len > size - 1) len = size - 1;
            if (buffer) memcpy( buffer + total, subst, len * sizeof(WCHAR) );
            total += len;
            size -= len;
            start = p + 1;
        }
    }

    if (start != p) /* unfinished string, copy it */
    {
        len = (unsigned int)(p - start);
        if (len > size - 1) len = size - 1;
        if (buffer) memcpy( buffer + total, start, len * sizeof(WCHAR) );
        total += len;
    }
    if (buffer && size) buffer[total] = 0;
    return total;
}


INFSTATUS
InfpFindFirstLine(PINFCACHE Cache,
                  PCWSTR Section,
                  PCWSTR Key,
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
                       PCWSTR Key,
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
      if (CacheLine->Key != NULL && strcmpiW (CacheLine->Key, Key) == 0)
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
                      PCWSTR Key,
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
      if (CacheLine->Key != NULL && strcmpiW (CacheLine->Key, Key) == 0)
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
                 PCWSTR Section)
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
      if (strcmpiW(CacheSection->Name, Section) == 0)
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

  Size = (ULONG)CacheLine->FieldCount - FieldIndex + 1;

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
          *Ptr = (UCHAR)strtoulW(CacheField->Data, NULL, 16);

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
  PWCHAR Ptr;

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

  *IntegerValue = (LONG)strtolW(Ptr, NULL, 0);

  return INF_STATUS_SUCCESS;
}


INFSTATUS
InfpGetMultiSzField(PINFCONTEXT Context,
                    ULONG FieldIndex,
                    PWSTR ReturnBuffer,
                    ULONG ReturnBufferSize,
                    PULONG RequiredSize)
{
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  PINFCACHEFIELD FieldPtr;
  ULONG Index;
  ULONG Size;
  PWCHAR Ptr;

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
      Size += ((ULONG)strlenW(FieldPtr->Data) + 1);
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
          Size = (ULONG)strlenW(FieldPtr->Data) + 1;

          strcpyW(Ptr, FieldPtr->Data);

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
                   PWSTR ReturnBuffer,
                   ULONG ReturnBufferSize,
                   PULONG RequiredSize)
{
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  ULONG Index;
  PWCHAR Ptr;
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

//  Size = (ULONG)strlenW(Ptr) + 1;
  Size = InfpSubstituteString(Context->Inf,
                              Ptr,
                              NULL,
                              0);

  if (RequiredSize != NULL)
    *RequiredSize = Size + 1;

  if (ReturnBuffer != NULL)
    {
      if (ReturnBufferSize <= Size)
        return INF_STATUS_BUFFER_OVERFLOW;

//      strcpyW(ReturnBuffer, Ptr);
      InfpSubstituteString(Context->Inf,
                           Ptr,
                           ReturnBuffer,
                           ReturnBufferSize);
    }

  return INF_STATUS_SUCCESS;
}


INFSTATUS
InfpGetData(PINFCONTEXT Context,
            PWCHAR *Key,
            PWCHAR *Data)
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
                 PWCHAR *Data)
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
