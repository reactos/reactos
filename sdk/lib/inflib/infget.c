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

static size_t
InfpSubstituteString(PINFCACHE Inf,
                     const WCHAR *text,
                     WCHAR *buffer,
                     size_t size);

static void
ShortToHex(PWCHAR Buffer,
           USHORT Value)
{
    WCHAR HexDigits[] = L"0123456789abcdef";

    Buffer[0] = HexDigits[Value >> 12 & 0xf];
    Buffer[1] = HexDigits[Value >>  8 & 0xf];
    Buffer[2] = HexDigits[Value >>  4 & 0xf];
    Buffer[3] = HexDigits[Value >>  0 & 0xf];
}

/* retrieve the string substitution for a given string, or NULL if not found */
/* if found, len is set to the substitution length */
static PCWSTR
InfpGetSubstitutionString(PINFCACHE Inf,
                          PCWSTR str,
                          size_t *len,
                          BOOL no_trailing_slash)
{
    static const WCHAR percent = '%';

    INFSTATUS Status = INF_STATUS_NOT_FOUND;
    PINFCONTEXT Context = NULL;
    PWCHAR Data = NULL;
    WCHAR ValueName[MAX_INF_STRING_LENGTH +1];
    WCHAR StringLangId[] = L"Strings.XXXX";

    if (!*len)  /* empty string (%%) is replaced by single percent */
    {
        *len = 1;
        return &percent;
    }

    memcpy(ValueName, str, *len * sizeof(WCHAR));
    ValueName[*len] = 0;

    DPRINT("Value name: %S\n", ValueName);

    if (Inf->LanguageId != 0)
    {
        ShortToHex(&StringLangId[sizeof("Strings.") - 1],
                   Inf->LanguageId);

        Status = InfpFindFirstLine(Inf,
                                   StringLangId,
                                   ValueName,
                                   &Context);
        if (Status != INF_STATUS_SUCCESS)
        {
            ShortToHex(&StringLangId[sizeof("Strings.") - 1],
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
        DPRINT("Substitute: %S  Length: %zu\n", Data, *len);
        return Data;
    }

    return NULL;
}


/* do string substitutions on the specified text */
/* the buffer is assumed to be large enough */
/* returns necessary length not including terminating null */
static size_t
InfpSubstituteString(PINFCACHE Inf,
                     PCWSTR text,
                     PWSTR buffer,
                     size_t size)
{
    const WCHAR *start, *subst, *p;
    size_t len, total = 0;
    int inside = 0;

    if (!buffer) size = MAX_INF_STRING_LENGTH + 1;
    for (p = start = text; *p; p++)
    {
        if (*p != '%') continue;
        inside = !inside;
        if (inside)  /* start of a %xx% string */
        {
            len = (p - start);
            if (len > size - 1) len = size - 1;
            if (buffer) memcpy( buffer + total, start, len * sizeof(WCHAR) );
            total += len;
            size -= len;
            start = p;
        }
        else /* end of the %xx% string, find substitution */
        {
            len = (p - start - 1);
            subst = InfpGetSubstitutionString( Inf, start + 1, &len, p[1] == '\\' );
            if (!subst)
            {
                subst = start;
                len = (p - start + 1);
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
  (*Context)->Section = CacheSection->Id;
  (*Context)->Line = CacheLine->Id;

  return INF_STATUS_SUCCESS;
}


INFSTATUS
InfpFindNextLine(PINFCONTEXT ContextIn,
                 PINFCONTEXT ContextOut)
{
  PINFCACHELINE CacheLine;

  if (ContextIn == NULL || ContextOut == NULL)
    return INF_STATUS_INVALID_PARAMETER;

  CacheLine = InfpGetLineForContext(ContextIn);
  if (CacheLine == NULL)
    return INF_STATUS_INVALID_PARAMETER;

  if (CacheLine->Next == NULL)
    return INF_STATUS_NOT_FOUND;

  if (ContextIn != ContextOut)
    {
      ContextOut->Inf = ContextIn->Inf;
      ContextOut->Section = ContextIn->Section;
    }
  ContextOut->Line = CacheLine->Next->Id;

  return INF_STATUS_SUCCESS;
}


INFSTATUS
InfpFindFirstMatchLine(PINFCONTEXT ContextIn,
                       PCWSTR Key,
                       PINFCONTEXT ContextOut)
{
  PINFCACHESECTION Section;
  PINFCACHELINE CacheLine;

  if (ContextIn == NULL || ContextOut == NULL || Key == NULL || *Key == 0)
    return INF_STATUS_INVALID_PARAMETER;

  Section = InfpGetSectionForContext(ContextIn);
  if (Section == NULL)
      return INF_STATUS_INVALID_PARAMETER;

  CacheLine = Section->FirstLine;
  while (CacheLine != NULL)
    {
      if (CacheLine->Key != NULL && strcmpiW (CacheLine->Key, Key) == 0)
        {

          if (ContextIn != ContextOut)
            {
              ContextOut->Inf = ContextIn->Inf;
              ContextOut->Section = ContextIn->Section;
            }
          ContextOut->Line = CacheLine->Id;

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
  PINFCACHESECTION Section;
  PINFCACHELINE CacheLine;

  if (ContextIn == NULL || ContextOut == NULL || Key == NULL || *Key == 0)
    return INF_STATUS_INVALID_PARAMETER;

  Section = InfpGetSectionForContext(ContextIn);
  if (Section == NULL)
      return INF_STATUS_INVALID_PARAMETER;

  CacheLine = InfpGetLineForContext(ContextIn);
  while (CacheLine != NULL)
    {
      if (CacheLine->Key != NULL && strcmpiW (CacheLine->Key, Key) == 0)
        {

          if (ContextIn != ContextOut)
            {
              ContextOut->Inf = ContextIn->Inf;
              ContextOut->Section = ContextIn->Section;
            }
          ContextOut->Line = CacheLine->Id;

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
  PINFCACHELINE Line;

  Line = InfpGetLineForContext(Context);
  if (Line == NULL)
    return 0;
  return Line->FieldCount;
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

  if (Context == NULL || FieldIndex == 0)
    {
      DPRINT("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  if (RequiredSize != NULL)
    *RequiredSize = 0;

  CacheLine = InfpGetLineForContext(Context);

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
                INT *IntegerValue)
{
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  ULONG Index;
  PWCHAR Ptr;

  if (Context == NULL || IntegerValue == NULL)
    {
      DPRINT("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  CacheLine = InfpGetLineForContext(Context);

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

  if (Context == NULL || FieldIndex == 0)
    {
      DPRINT("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  if (RequiredSize != NULL)
    *RequiredSize = 0;

  CacheLine = InfpGetLineForContext(Context);

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
  SIZE_T Size;

  if (Context == NULL)
    {
      DPRINT("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  if (RequiredSize != NULL)
    *RequiredSize = 0;

  CacheLine = InfpGetLineForContext(Context);

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
    *RequiredSize = (ULONG)Size + 1;

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

  if (Context == NULL || Data == NULL)
    {
      DPRINT("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  CacheKey = InfpGetLineForContext(Context);
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

  if (Context == NULL || Data == NULL)
    {
      DPRINT("Invalid parameter\n");
      return INF_STATUS_INVALID_PARAMETER;
    }

  CacheLine = InfpGetLineForContext(Context);

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
