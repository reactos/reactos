/*
 * RichEdit - string operations
 *
 * Copyright 2004 by Krzysztof Foltman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "editor.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

static int ME_GetOptimalBuffer(int nLen)
{
  /* FIXME: This seems wasteful for tabs and end of lines strings,
   *        since they have a small fixed length. */
  return ((sizeof(WCHAR) * nLen) + 128) & ~63;
}

/* Create a buffer (uninitialized string) of size nMaxChars */
static ME_String *ME_MakeStringB(int nMaxChars)
{
  ME_String *s = ALLOC_OBJ(ME_String);

  s->nLen = nMaxChars;
  s->nBuffer = ME_GetOptimalBuffer(s->nLen + 1);
  s->szData = ALLOC_N_OBJ(WCHAR, s->nBuffer);
  s->szData[s->nLen] = 0;
  return s;
}

ME_String *ME_MakeStringN(LPCWSTR szText, int nMaxChars)
{
  ME_String *s = ME_MakeStringB(nMaxChars);
  /* Native allows NULL chars */
  memcpy(s->szData, szText, s->nLen * sizeof(WCHAR));
  return s;
}

/* Make a string by repeating a char nMaxChars times */
ME_String *ME_MakeStringR(WCHAR cRepeat, int nMaxChars)
{
  int i;
  ME_String *s = ME_MakeStringB(nMaxChars);
  for (i = 0; i < nMaxChars; i++)
    s->szData[i] = cRepeat;
  return s;
}

void ME_DestroyString(ME_String *s)
{
  if (!s) return;
  FREE_OBJ(s->szData);
  FREE_OBJ(s);
}

BOOL ME_InsertString(ME_String *s, int ofs, const WCHAR *insert, int len)
{
    DWORD new_len = s->nLen + len + 1;
    assert( ofs <= s->nLen );

    if( new_len > s->nBuffer )
    {
        s->nBuffer = ME_GetOptimalBuffer( new_len );
        s->szData = heap_realloc( s->szData, s->nBuffer * sizeof(WCHAR) );
        if (!s->szData) return FALSE;
    }

    memmove( s->szData + ofs + len, s->szData + ofs, (s->nLen - ofs + 1) * sizeof(WCHAR) );
    memcpy( s->szData + ofs, insert, len * sizeof(WCHAR) );
    s->nLen += len;

    return TRUE;
}

BOOL ME_AppendString(ME_String *s, const WCHAR *append, int len)
{
    return ME_InsertString( s, s->nLen, append, len );
}

ME_String *ME_VSplitString(ME_String *orig, int charidx)
{
  ME_String *s;

  /*if (charidx<0) charidx = 0;
  if (charidx>orig->nLen) charidx = orig->nLen;
  */
  assert(charidx>=0);
  assert(charidx<=orig->nLen);

  s = ME_MakeStringN(orig->szData+charidx, orig->nLen-charidx);
  orig->nLen = charidx;
  orig->szData[charidx] = '\0';
  return s;
}

void ME_StrDeleteV(ME_String *s, int nVChar, int nChars)
{
  int end_ofs = nVChar + nChars;

  assert(nChars >= 0);
  assert(nVChar >= 0);
  assert(end_ofs <= s->nLen);

  memmove(s->szData + nVChar, s->szData + end_ofs,
          (s->nLen - end_ofs + 1) * sizeof(WCHAR));
  s->nLen -= nChars;
}

static int
ME_WordBreakProc(LPWSTR s, INT start, INT len, INT code)
{
  /* FIXME: Native also knows about punctuation */
  TRACE("s==%s, start==%d, len==%d, code==%d\n",
        debugstr_wn(s, len), start, len, code);

  switch (code)
  {
    case WB_ISDELIMITER:
      return ME_IsWSpace(s[start]);
    case WB_LEFT:
    case WB_MOVEWORDLEFT:
      while (start && ME_IsWSpace(s[start - 1]))
        start--;
      while (start && !ME_IsWSpace(s[start - 1]))
        start--;
      return start;
    case WB_RIGHT:
    case WB_MOVEWORDRIGHT:
      while (start < len && !ME_IsWSpace(s[start]))
        start++;
      while (start < len && ME_IsWSpace(s[start]))
        start++;
      return start;
  }
  return 0;
}


int
ME_CallWordBreakProc(ME_TextEditor *editor, WCHAR *str, INT len, INT start, INT code)
{
  if (!editor->pfnWordBreak) {
    return ME_WordBreakProc(str, start, len, code);
  } else if (!editor->bEmulateVersion10) {
    /* MSDN lied about the third parameter for EditWordBreakProc being the number
     * of characters, it is actually the number of bytes of the string. */
    return editor->pfnWordBreak(str, start, len * sizeof(WCHAR), code);
  } else {
    int result;
    int buffer_size = WideCharToMultiByte(CP_ACP, 0, str, len,
                                          NULL, 0, NULL, NULL);
    char *buffer = heap_alloc(buffer_size);
    WideCharToMultiByte(CP_ACP, 0, str, len,
                        buffer, buffer_size, NULL, NULL);
    result = editor->pfnWordBreak((WCHAR*)buffer, start, buffer_size, code);
    heap_free(buffer);
    return result;
  }
}

LPWSTR ME_ToUnicode(LONG codepage, LPVOID psz, INT *len)
{
  *len = 0;
  if (!psz) return NULL;

  if (codepage == CP_UNICODE)
  {
    *len = lstrlenW(psz);
    return psz;
  }
  else {
    WCHAR *tmp;
    int nChars = MultiByteToWideChar(codepage, 0, psz, -1, NULL, 0);

    if(!nChars) return NULL;

    if((tmp = ALLOC_N_OBJ(WCHAR, nChars)) != NULL)
      *len = MultiByteToWideChar(codepage, 0, psz, -1, tmp, nChars) - 1;
    return tmp;
  }
}

void ME_EndToUnicode(LONG codepage, LPVOID psz)
{
  if (codepage != CP_UNICODE)
    FREE_OBJ(psz);
}
