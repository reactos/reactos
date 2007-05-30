#include <user32.h>

#include <wine/debug.h>

/* FIXME: Currently IsBadWritePtr is implemented using VirtualQuery which
          does not seem to work properly for stack address space. */
/* kill `left-hand operand of comma expression has no effect' warning */
#define IsBadWritePtr(lp, n) ((DWORD)lp==n?0:0)

BOOL STDCALL _InternalLoadString
(
 HINSTANCE hInstance,
 UINT uID,
 PUNICODE_STRING pwstrDest
)
{
 HRSRC hrsStringTable;
 HGLOBAL hResource;
 PWCHAR pStringTable;
 unsigned i;
 unsigned l = uID % 16; /* (1) */

 /* parameter validation */
 if(IsBadWritePtr(pwstrDest, sizeof(UNICODE_STRING)))
 {
  SetLastError(ERROR_INVALID_PARAMETER);
  return FALSE;
 }

 /*
  find the string table. String tables are created by grouping, 16 by 16, string
  resources whose identifiers, divided by 16, have the same integer quotient.
  Holes in the numbering are filled with zero-length strings. String table ids
  (actual resource ids) start from 1. See (1) and (2)
 */
 /* TODO: some sort of cache, here, would be great */
 hrsStringTable = FindResourceW
 (
  (HMODULE)hInstance,
  MAKEINTRESOURCEW((uID / 16) + 1), /* (2) */
  RT_STRING
 );

 /* failure */
 if(hrsStringTable == NULL) return FALSE;

 /* load the string table into memory */
 hResource = LoadResource((HMODULE)hInstance, hrsStringTable);

 /* failure */
 if(hResource == NULL) return FALSE;

 /* lock the resource into memory */
 pStringTable = LockResource(hResource);

 /* failure */
 if(pStringTable == NULL) return FALSE;

 /*
  string tables are packed Unicode Pascal strings. The first WCHAR contains the
  length, in characters, of the current string. Zero-length strings, if any, are
  placeholders for unused slots, and should therefore be considered non-present.
  See also (3). Here, we walk all the strings before that of interest
 */
 for(i = 0; i < l; ++ i)
 {
  /* skip the length and the current string */
  pStringTable += 1 + (*pStringTable);
 }

 /* we've reached the string of interest */
 if((*pStringTable) == 0)
 {
  /* the string is empty (unallocated) */
  SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
  return FALSE; /* 3 */
 }

 /* string length in bytes */
 pwstrDest->Length = pwstrDest->MaximumLength = (*pStringTable) * sizeof(WCHAR);

 /* string */
 pwstrDest->Buffer = pStringTable + 1;

 /* success */
 return TRUE;
}


/*
 * @implemented
 */
int STDCALL LoadStringA
(
 HINSTANCE hInstance,
 UINT uID,
 LPSTR lpBuffer,
 int nBufferMax
)
{
  UNICODE_STRING wstrResStr;
  ANSI_STRING strBuf;
  INT retSize;

  /* parameter validation */
  if
  (
    (nBufferMax < 1) ||
    (IsBadWritePtr(lpBuffer, nBufferMax * sizeof(lpBuffer[0])))
  )
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }

  /* get the UNICODE_STRING descriptor of the in-memory image of the string */
  if(!_InternalLoadString(hInstance, uID, &wstrResStr))
  {
    /* failure */
    return 0;
  }

  /*
   convert the string. The Unicode string may be in UTF-16 (multi-byte), so we
   don't alter wstrResStr.Length, and let RtlUnicodeStringToAnsiString truncate
   it, if necessary
  */
  strBuf.Length = 0;
  strBuf.MaximumLength = nBufferMax * sizeof(CHAR);
  strBuf.Buffer = lpBuffer;

  retSize = WideCharToMultiByte(CP_ACP, 0, wstrResStr.Buffer,
                                wstrResStr.Length / sizeof(WCHAR),
                                strBuf.Buffer, strBuf.MaximumLength, NULL, NULL);

  if(!retSize)
  {
    /* failure */
    return 0;
  }
  else
  {
    strBuf.Length = retSize;
  }

  /* the ANSI string may not be null-terminated */
  if(strBuf.Length >= strBuf.MaximumLength)
  {
    /* length greater than the buffer? whatever */
    int nStringLen = strBuf.MaximumLength / sizeof(CHAR) - 1;

    /* zero the last character in the buffer */
    strBuf.Buffer[nStringLen] = 0;

    /* success */
    return nStringLen;
  }
  else
  {
    /* zero the last character in the string */
    strBuf.Buffer[strBuf.Length / sizeof(CHAR)] = 0;

    /* success */
    return strBuf.Length / sizeof(CHAR);
  }
}


/*
 * @implemented
 */
int STDCALL LoadStringW
(
 HINSTANCE hInstance,
 UINT uID,
 LPWSTR lpBuffer,
 int nBufferMax
)
{
  UNICODE_STRING wstrResStr;
  int nStringLen;

  /* parameter validation */
  if
  (
    (nBufferMax < 1) ||
    ((nBufferMax > 0)  && IsBadWritePtr(lpBuffer, nBufferMax * sizeof(lpBuffer[0]))) ||
    /* undocumented: If nBufferMax is 0, LoadStringW will copy a pointer to the 
       in-memory image of the string to the specified buffer and return the length 
       of the string in WCHARs */
    ((nBufferMax == 0) && IsBadWritePtr(lpBuffer, sizeof(lpBuffer)))
  )
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }

  /* get the UNICODE_STRING descriptor of the in-memory image of the string */
  if(!_InternalLoadString(hInstance, uID, &wstrResStr))
  {
    /* failure */
    return 0;
  }

  /* get the length in characters */
  nStringLen = wstrResStr.Length / sizeof(WCHAR);

  if (nBufferMax > 0)
  {
    /* the buffer must be enough to contain the string and the null terminator */
    if(nBufferMax < (nStringLen + 1))
    {
      /* otherwise, the string is truncated */
      nStringLen = nBufferMax - 1;
    }

    /* copy the string */
    memcpy(lpBuffer, wstrResStr.Buffer, nStringLen * sizeof(WCHAR));

    /* null-terminate it */
    lpBuffer[nStringLen] = 0;
  }
  else
  {
    *((LPWSTR*)lpBuffer) = wstrResStr.Buffer;
  }
  /* success */
  return nStringLen;
}

/* EOF */
