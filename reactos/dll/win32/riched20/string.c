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

int ME_GetOptimalBuffer(int nLen)
{
  return ((2*nLen+1)+128)&~63;
}

ME_String *ME_MakeString(LPCWSTR szText)
{
  ME_String *s = ALLOC_OBJ(ME_String);
  s->nLen = lstrlenW(szText);
  s->nBuffer = ME_GetOptimalBuffer(s->nLen+1);
  s->szData = ALLOC_N_OBJ(WCHAR, s->nBuffer);
  lstrcpyW(s->szData, szText);
  return s;
}

ME_String *ME_MakeStringN(LPCWSTR szText, int nMaxChars)
{
  ME_String *s = ALLOC_OBJ(ME_String);
  
  s->nLen = nMaxChars;
  s->nBuffer = ME_GetOptimalBuffer(s->nLen+1);
  s->szData = ALLOC_N_OBJ(WCHAR, s->nBuffer);
  /* Native allows NUL chars */
  memmove(s->szData, szText, s->nLen * sizeof(WCHAR));
  s->szData[s->nLen] = 0;
  return s;
}

ME_String *ME_MakeStringR(WCHAR cRepeat, int nMaxChars)
{ /* Make a string by repeating a char nMaxChars times */
  int i;
   ME_String *s = ALLOC_OBJ(ME_String);
  
  s->nLen = nMaxChars;
  s->nBuffer = ME_GetOptimalBuffer(s->nLen+1);
  s->szData = ALLOC_N_OBJ(WCHAR, s->nBuffer);

  for (i = 0;i<nMaxChars;i++)
    s->szData[i] = cRepeat;
  s->szData[s->nLen] = 0;
  return s;
}

ME_String *ME_MakeStringB(int nMaxChars)
{ /* Create a buffer (uninitialized string) of size nMaxChars */
  ME_String *s = ALLOC_OBJ(ME_String);
  
  s->nLen = nMaxChars;
  s->nBuffer = ME_GetOptimalBuffer(s->nLen+1);
  s->szData = ALLOC_N_OBJ(WCHAR, s->nBuffer);
  s->szData[s->nLen] = 0;
  return s;
}

ME_String *ME_StrDup(const ME_String *s)
{
  return ME_MakeStringN(s->szData, s->nLen);
}

void ME_DestroyString(ME_String *s)
{
  FREE_OBJ(s->szData);
  FREE_OBJ(s);
}

void ME_AppendString(ME_String *s1, const ME_String *s2)
{
  if (s1->nLen+s2->nLen+1 <= s1->nBuffer) {
    lstrcpyW(s1->szData+s1->nLen, s2->szData);
    s1->nLen += s2->nLen;
  }
  else
  {
    WCHAR *buf;
    s1->nBuffer = ME_GetOptimalBuffer(s1->nLen+s2->nLen+1);

    buf = ALLOC_N_OBJ(WCHAR, s1->nBuffer); 
    lstrcpyW(buf, s1->szData);
    lstrcpyW(buf+s1->nLen, s2->szData);
    FREE_OBJ(s1->szData);
    s1->szData = buf;
    s1->nLen += s2->nLen;
  }
}

ME_String *ME_ConcatString(const ME_String *s1, const ME_String *s2)
{
  ME_String *s = ALLOC_OBJ(ME_String);
  s->nLen = s1->nLen+s2->nLen;
  s->nBuffer = ME_GetOptimalBuffer(s1->nLen+s2->nLen+1);
  s->szData = ALLOC_N_OBJ(WCHAR, s->nBuffer);
  lstrcpyW(s->szData, s1->szData);
  lstrcpyW(s->szData+s1->nLen, s2->szData);
  return s;  
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

int ME_IsWhitespaces(const ME_String *s)
{
  /* FIXME multibyte */
  WCHAR *pos = s->szData;
  while(ME_IsWSpace(*pos++))
    ;
  pos--;
  if (*pos)
    return 0;
  else
    return 1;
}

int ME_IsSplitable(const ME_String *s)
{
  WCHAR *pos = s->szData;
  WCHAR ch;
  while(ME_IsWSpace(*pos++))
    ;
  pos--;
  while((ch = *pos++) != 0)
  {
    if (ME_IsWSpace(ch))
      return 1;
  }
  return 0;
}

/* FIXME multibyte */
/*
int ME_CalcSkipChars(ME_String *s)
{
  int cnt = 0;
  while(cnt < s->nLen && s->szData[s->nLen-1-cnt]==' ')
    cnt++;
  return cnt;
}
*/

int ME_StrLen(const ME_String *s) {
  return s->nLen;
}

int ME_StrVLen(const ME_String *s) {
  return s->nLen;
}

int ME_StrRelPos(const ME_String *s, int nVChar, int *pRelChars)
{
  int nRelChars = *pRelChars;

  TRACE("%s,%d,&%d\n", debugstr_w(s->szData), nVChar, *pRelChars);

  assert(*pRelChars);
  if (!nRelChars)
    return nVChar;
  
  if (nRelChars>0)
    nRelChars = min(*pRelChars, s->nLen - nVChar);
  else
    nRelChars = max(*pRelChars, -nVChar);
  nVChar += nRelChars;
  *pRelChars -= nRelChars;
  return nVChar;
}

int ME_StrRelPos2(const ME_String *s, int nVChar, int nRelChars)
{
  return ME_StrRelPos(s, nVChar, &nRelChars);
}

int ME_VPosToPos(ME_String *s, int nVPos)
{
  return nVPos;
  /*
  int i = 0, len = 0;
  if (!nVPos)
    return 0;
  while (i < s->nLen)
  {
    if (i == nVPos)
      return len;
    if (s->szData[i]=='\\') i++;
    i++;
    len++;
  }
  return len;
  */
}

int ME_PosToVPos(const ME_String *s, int nPos)
{
  if (!nPos)
    return 0;
  return ME_StrRelPos2(s, 0, nPos);
}

void ME_StrDeleteV(ME_String *s, int nVChar, int nChars)
{
  int end_ofs;
  
  assert(nVChar >=0 && nVChar <= s->nLen);
  assert(nChars >= 0);
  assert(nVChar+nChars <= s->nLen);
  
  end_ofs = ME_StrRelPos2(s, nVChar, nChars);
  assert(end_ofs <= s->nLen);
  memmove(s->szData+nVChar, s->szData+end_ofs, 2*(s->nLen+1-end_ofs));
  s->nLen -= (end_ofs - nVChar);
}

int ME_GetCharFwd(const ME_String *s, int nPos)
{
  int nVPos = 0;

  assert(nPos < ME_StrLen(s));
  if (nPos)
    nVPos = ME_StrRelPos2(s, nVPos, nPos);
  
  if (nVPos < s->nLen)
    return s->szData[nVPos];
  return -1;
}

int ME_GetCharBack(const ME_String *s, int nPos)
{
  int nVPos = ME_StrVLen(s);

  assert(nPos < ME_StrLen(s));
  if (nPos)
    nVPos = ME_StrRelPos2(s, nVPos, -nPos);
  
  if (nVPos < s->nLen)
    return s->szData[nVPos];
  return -1;
}

int ME_FindNonWhitespaceV(const ME_String *s, int nVChar) {
  int i;
  for (i = nVChar; i<s->nLen && ME_IsWSpace(s->szData[i]); i++)
    ;
    
  return i;
}

/* note: returns offset of the first trailing whitespace */
int ME_ReverseFindNonWhitespaceV(const ME_String *s, int nVChar) {
  int i;
  for (i = nVChar; i>0 && ME_IsWSpace(s->szData[i-1]); i--)
    ;
    
  return i;
}

/* note: returns offset of the first trailing nonwhitespace */
int ME_ReverseFindWhitespaceV(const ME_String *s, int nVChar) {
  int i;
  for (i = nVChar; i>0 && !ME_IsWSpace(s->szData[i-1]); i--)
    ;
    
  return i;
}


static int
ME_WordBreakProc(LPWSTR s, INT start, INT len, INT code)
{
  /* FIXME: Native also knows about punctuation */
  TRACE("s==%s, start==%d, len==%d, code==%d\n",
        debugstr_wn(s, len), start, len, code);
  /* convert number of bytes to number of characters. */
  len /= sizeof(WCHAR);
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
ME_CallWordBreakProc(ME_TextEditor *editor, ME_String *str, INT start, INT code)
{
  if (!editor->pfnWordBreak) {
    return ME_WordBreakProc(str->szData, start, str->nLen*sizeof(WCHAR), code);
  } else if (!editor->bEmulateVersion10) {
    /* MSDN lied about the third parameter for EditWordBreakProc being the number
     * of characters, it is actually the number of bytes of the string. */
    return editor->pfnWordBreak(str->szData, start, str->nLen*sizeof(WCHAR), code);
  } else {
    int result;
    int buffer_size = WideCharToMultiByte(CP_ACP, 0, str->szData, str->nLen,
                                          NULL, 0, NULL, NULL);
    char *buffer = (char*)heap_alloc(buffer_size);
    WideCharToMultiByte(CP_ACP, 0, str->szData, str->nLen,
                        buffer, buffer_size, NULL, NULL);
    result = editor->pfnWordBreak(str->szData, start, str->nLen, code);
    heap_free(buffer);
    return result;
  }
}

LPWSTR ME_ToUnicode(BOOL unicode, LPVOID psz)
{
  assert(psz != NULL);

  if (unicode)
    return (LPWSTR)psz;
  else {
    WCHAR *tmp;
    int nChars = MultiByteToWideChar(CP_ACP, 0, (char *)psz, -1, NULL, 0);
    if((tmp = ALLOC_N_OBJ(WCHAR, nChars)) != NULL)
      MultiByteToWideChar(CP_ACP, 0, (char *)psz, -1, tmp, nChars);
    return tmp;
  }
}

void ME_EndToUnicode(BOOL unicode, LPVOID psz)
{
  if (!unicode)
    FREE_OBJ(psz);
}
