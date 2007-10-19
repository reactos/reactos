/*
 * RichEdit style management functions
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
WINE_DECLARE_DEBUG_CHANNEL(richedit_style);

static int all_refs = 0;

CHARFORMAT2W *ME_ToCF2W(CHARFORMAT2W *to, CHARFORMAT2W *from)
{
  if (from->cbSize == sizeof(CHARFORMATA))
  {
    CHARFORMATA *f = (CHARFORMATA *)from;
    CopyMemory(to, f, sizeof(*f)-sizeof(f->szFaceName));
    to->cbSize = sizeof(CHARFORMAT2W);
    if (f->dwMask & CFM_FACE) {
      MultiByteToWideChar(0, 0, f->szFaceName, -1, to->szFaceName, sizeof(to->szFaceName)/sizeof(WCHAR));
    }
    return to;
  }
  if (from->cbSize == sizeof(CHARFORMATW))
  {
    CHARFORMATW *f = (CHARFORMATW *)from;
    CopyMemory(to, f, sizeof(*f));
    /* theoretically, we don't need to zero the remaining memory */
    ZeroMemory(((CHARFORMATW *)to)+1, sizeof(CHARFORMAT2W)-sizeof(CHARFORMATW));
    to->cbSize = sizeof(CHARFORMAT2W);
    return to;
  }
  if (from->cbSize == sizeof(CHARFORMAT2A))
  {
    CHARFORMATA *f = (CHARFORMATA *)from;
    /* copy the A structure without face name */
    CopyMemory(to, f, sizeof(CHARFORMATA)-sizeof(f->szFaceName));
    /* convert face name */
    if (f->dwMask & CFM_FACE)
      MultiByteToWideChar(0, 0, f->szFaceName, -1, to->szFaceName, sizeof(to->szFaceName)/sizeof(WCHAR));
    /* copy the rest of the 2A structure to 2W */
    CopyMemory(1+((CHARFORMATW *)to), f+1, sizeof(CHARFORMAT2A)-sizeof(CHARFORMATA));
    to->cbSize = sizeof(CHARFORMAT2W);
    return to;
  }

  assert(from->cbSize >= sizeof(CHARFORMAT2W));
  return from;
}

void ME_CopyToCF2W(CHARFORMAT2W *to, CHARFORMAT2W *from)
{
  if (ME_ToCF2W(to, from) == from)
    CopyMemory(to, from, sizeof(*from));
}

CHARFORMAT2W *ME_ToCFAny(CHARFORMAT2W *to, CHARFORMAT2W *from)
{
  assert(from->cbSize == sizeof(CHARFORMAT2W));
  if (to->cbSize == sizeof(CHARFORMATA))
  {
    CHARFORMATA *t = (CHARFORMATA *)to;
    CopyMemory(t, from, sizeof(*t)-sizeof(t->szFaceName));
    WideCharToMultiByte(0, 0, from->szFaceName, -1, t->szFaceName, sizeof(t->szFaceName), 0, 0);
    t->cbSize = sizeof(*t); /* it was overwritten by CopyMemory */
    return to;
  }
  if (to->cbSize == sizeof(CHARFORMATW))
  {
    CHARFORMATW *t = (CHARFORMATW *)to;
    CopyMemory(t, from, sizeof(*t));
    t->cbSize = sizeof(*t); /* it was overwritten by CopyMemory */
    return to;
  }
  if (to->cbSize == sizeof(CHARFORMAT2A))
  {
    CHARFORMAT2A *t = (CHARFORMAT2A *)to;
    /* copy the A structure without face name */
    CopyMemory(t, from, sizeof(CHARFORMATA)-sizeof(t->szFaceName));
    /* convert face name */
    WideCharToMultiByte(0, 0, from->szFaceName, -1, t->szFaceName, sizeof(t->szFaceName), 0, 0);
    /* copy the rest of the 2A structure to 2W */
    CopyMemory(&t->wWeight, &from->wWeight, sizeof(CHARFORMAT2A)-sizeof(CHARFORMATA));
    t->cbSize = sizeof(*t); /* it was overwritten by CopyMemory */
    return to;
  }
  assert(to->cbSize >= sizeof(CHARFORMAT2W));
  return from;
}

void ME_CopyToCFAny(CHARFORMAT2W *to, CHARFORMAT2W *from)
{
  if (ME_ToCFAny(to, from) == from)
    CopyMemory(to, from, to->cbSize);
}

ME_Style *ME_MakeStyle(CHARFORMAT2W *style) {
  CHARFORMAT2W styledata;
  ME_Style *s = ALLOC_OBJ(ME_Style);

  style = ME_ToCF2W(&styledata, style);
  memset(s, 0, sizeof(ME_Style));
  if (style->cbSize <= sizeof(CHARFORMAT2W))
    CopyMemory(&s->fmt, style, style->cbSize);
  else
    CopyMemory(&s->fmt, style, sizeof(CHARFORMAT2W));
  s->fmt.cbSize = sizeof(CHARFORMAT2W);

  s->nSequence = -2;
  s->nRefs = 1;
  s->hFont = NULL;
  s->tm.tmAscent = -1;
  all_refs++;
  return s;
}

#define COPY_STYLE_ITEM(mask, member) \
  if (style->dwMask & mask) { \
    s->fmt.dwMask |= mask;\
    s->fmt.member = style->member;\
  }

#define COPY_STYLE_ITEM_MEMCPY(mask, member) \
  if (style->dwMask & mask) { \
    s->fmt.dwMask |= mask;\
    CopyMemory(s->fmt.member, style->member, sizeof(style->member));\
  }

void ME_InitCharFormat2W(CHARFORMAT2W *pFmt)
{
  ZeroMemory(pFmt, sizeof(CHARFORMAT2W));
  pFmt->cbSize = sizeof(CHARFORMAT2W);
}

ME_Style *ME_ApplyStyle(ME_Style *sSrc, CHARFORMAT2W *style)
{
  CHARFORMAT2W styledata;
  ME_Style *s = ME_MakeStyle(&sSrc->fmt);
  style = ME_ToCF2W(&styledata, style);
  COPY_STYLE_ITEM(CFM_ANIMATION, bAnimation);
  COPY_STYLE_ITEM(CFM_BACKCOLOR, crBackColor);
  COPY_STYLE_ITEM(CFM_CHARSET, bCharSet);
  COPY_STYLE_ITEM(CFM_COLOR, crTextColor);
  COPY_STYLE_ITEM_MEMCPY(CFM_FACE, szFaceName);
  COPY_STYLE_ITEM(CFM_KERNING, wKerning);
  COPY_STYLE_ITEM(CFM_LCID, lcid);
  COPY_STYLE_ITEM(CFM_OFFSET, yOffset);
  COPY_STYLE_ITEM(CFM_REVAUTHOR, bRevAuthor);
  COPY_STYLE_ITEM(CFM_SIZE, yHeight);
  COPY_STYLE_ITEM(CFM_SPACING, sSpacing);
  COPY_STYLE_ITEM(CFM_STYLE, sStyle);
  COPY_STYLE_ITEM(CFM_UNDERLINETYPE, bUnderlineType);
  COPY_STYLE_ITEM(CFM_WEIGHT, wWeight);

  s->fmt.dwEffects &= ~(style->dwMask);
  s->fmt.dwEffects |= style->dwEffects & style->dwMask;
  s->fmt.dwMask |= style->dwMask;
  if (style->dwMask & CFM_COLOR)
  {
    if (style->dwEffects & CFE_AUTOCOLOR)
      s->fmt.dwEffects |= CFE_AUTOCOLOR;
    else
      s->fmt.dwEffects &= ~CFE_AUTOCOLOR;
  }
  return s;
}

void ME_CopyCharFormat(CHARFORMAT2W *pDest, CHARFORMAT2W *pSrc)
{
  /* using this with non-2W structs is forbidden */
  assert(pSrc->cbSize == sizeof(CHARFORMAT2W));
  assert(pDest->cbSize == sizeof(CHARFORMAT2W));
  CopyMemory(pDest, pSrc, sizeof(CHARFORMAT2W));
}

static void ME_DumpStyleEffect(char **p, const char *name, CHARFORMAT2W *fmt, int mask)
{
  *p += sprintf(*p, "%-22s%s\n", name, (fmt->dwMask & mask) ? ((fmt->dwEffects & mask) ? "YES" : "no") : "N/A");
}

void ME_DumpStyle(ME_Style *s)
{
  char buf[2048];
  ME_DumpStyleToBuf(&s->fmt, buf);
  TRACE_(richedit_style)("%s\n", buf);
}

void ME_DumpStyleToBuf(CHARFORMAT2W *pFmt, char buf[2048])
{
  /* FIXME only CHARFORMAT styles implemented */
  /* this function sucks, doesn't check for buffer overruns but it's "good enough" as for debug code */
  char *p;
  p = buf;
  p += sprintf(p, "Font face:            ");
  if (pFmt->dwMask & CFM_FACE) {
    WCHAR *q = pFmt->szFaceName;
    while(*q) {
      *p++ = (*q > 255) ? '?' : *q;
      q++;
    }
  } else
    p += sprintf(p, "N/A");

  if (pFmt->dwMask & CFM_SIZE)
    p += sprintf(p, "\nFont size:            %d\n", (int)pFmt->yHeight);
  else
    p += sprintf(p, "\nFont size:            N/A\n");

  if (pFmt->dwMask & CFM_OFFSET)
    p += sprintf(p, "Char offset:          %d\n", (int)pFmt->yOffset);
  else
    p += sprintf(p, "Char offset:          N/A\n");

  if (pFmt->dwMask & CFM_CHARSET)
    p += sprintf(p, "Font charset:         %d\n", (int)pFmt->bCharSet);
  else
    p += sprintf(p, "Font charset:         N/A\n");

  /* I'm assuming CFM_xxx and CFE_xxx are the same values, fortunately it IS true wrt used flags*/
  ME_DumpStyleEffect(&p, "Font bold:", pFmt, CFM_BOLD);
  ME_DumpStyleEffect(&p, "Font italic:", pFmt, CFM_ITALIC);
  ME_DumpStyleEffect(&p, "Font underline:", pFmt, CFM_UNDERLINE);
  ME_DumpStyleEffect(&p, "Font strikeout:", pFmt, CFM_STRIKEOUT);
  ME_DumpStyleEffect(&p, "Hidden text:", pFmt, CFM_HIDDEN);
  p += sprintf(p, "Text color:           ");
  if (pFmt->dwMask & CFM_COLOR)
  {
    if (pFmt->dwEffects & CFE_AUTOCOLOR)
      p += sprintf(p, "auto\n");
    else
      p += sprintf(p, "%06x\n", (int)pFmt->crTextColor);
  }
  else
    p += sprintf(p, "N/A\n");
  ME_DumpStyleEffect(&p, "Text protected:", pFmt, CFM_PROTECTED);
}


static void
ME_LogFontFromStyle(HDC hDC, LOGFONTW *lf, ME_Style *s, int nZoomNumerator, int nZoomDenominator)
{
  int rx, ry;
  rx = GetDeviceCaps(hDC, LOGPIXELSX);
  ry = GetDeviceCaps(hDC, LOGPIXELSY);
  ZeroMemory(lf, sizeof(LOGFONTW));
  lstrcpyW(lf->lfFaceName, s->fmt.szFaceName);

  if (nZoomNumerator == 0)
  {
    nZoomNumerator = 1;
    nZoomDenominator = 1;
  }
  lf->lfHeight = -s->fmt.yHeight*ry*nZoomNumerator/nZoomDenominator/1440;

  lf->lfWeight = 400;
  if (s->fmt.dwEffects & s->fmt.dwMask & CFM_BOLD)
    lf->lfWeight = 700;
  if (s->fmt.dwEffects & s->fmt.dwMask & CFM_WEIGHT)
    lf->lfWeight = s->fmt.wWeight;
  if (s->fmt.dwEffects & s->fmt.dwMask & CFM_ITALIC)
    lf->lfItalic = 1;
  if (s->fmt.dwEffects & s->fmt.dwMask & (CFM_UNDERLINE | CFE_LINK))
    lf->lfUnderline = 1;
  if (s->fmt.dwEffects & s->fmt.dwMask & CFM_STRIKEOUT)
    lf->lfStrikeOut = 1;
  if (s->fmt.dwEffects & s->fmt.dwMask & (CFM_SUBSCRIPT|CFM_SUPERSCRIPT))
    lf->lfHeight = (lf->lfHeight*2)/3;
/*lf.lfQuality = PROOF_QUALITY; */
  lf->lfPitchAndFamily = s->fmt.bPitchAndFamily;
  lf->lfCharSet = s->fmt.bCharSet;
}

void ME_CharFormatFromLogFont(HDC hDC, LOGFONTW *lf, CHARFORMAT2W *fmt)
{
  int rx, ry;

  ME_InitCharFormat2W(fmt);
  rx = GetDeviceCaps(hDC, LOGPIXELSX);
  ry = GetDeviceCaps(hDC, LOGPIXELSY);
  lstrcpyW(fmt->szFaceName, lf->lfFaceName);
  fmt->dwEffects = 0;
  fmt->dwMask = CFM_WEIGHT|CFM_BOLD|CFM_ITALIC|CFM_UNDERLINE|CFM_STRIKEOUT|CFM_SIZE|CFM_FACE|CFM_CHARSET;
  fmt->wWeight = lf->lfWeight;
  fmt->yHeight = -lf->lfHeight*1440/ry;
  if (lf->lfWeight>400) fmt->dwEffects |= CFM_BOLD;
  if (lf->lfItalic) fmt->dwEffects |= CFM_ITALIC;
  if (lf->lfUnderline) fmt->dwEffects |= CFM_UNDERLINE;
  /* notice that if a logfont was created with underline due to CFM_LINK, this
      would add an erronious CFM_UNDERLINE. This isn't currently ever a problem */
  if (lf->lfStrikeOut) fmt->dwEffects |= CFM_STRIKEOUT;
  fmt->bPitchAndFamily = lf->lfPitchAndFamily;
  fmt->bCharSet = lf->lfCharSet;
}

static BOOL ME_IsFontEqual(LOGFONTW *p1, LOGFONTW *p2)
{
  if (memcmp(p1, p2, sizeof(LOGFONTW)-sizeof(p1->lfFaceName)))
    return FALSE;
  if (lstrcmpW(p1->lfFaceName, p2->lfFaceName))
    return FALSE;
  return TRUE;
}

HFONT ME_SelectStyleFont(ME_TextEditor *editor, HDC hDC, ME_Style *s)
{
  HFONT hOldFont;
  LOGFONTW lf;
  int i, nEmpty, nAge = 0x7FFFFFFF;
  ME_FontCacheItem *item;
  assert(hDC);
  assert(s);

  ME_LogFontFromStyle(hDC, &lf, s, editor->nZoomNumerator, editor->nZoomDenominator);

  for (i=0; i<HFONT_CACHE_SIZE; i++)
    editor->pFontCache[i].nAge++;
  for (i=0, nEmpty=-1, nAge=0; i<HFONT_CACHE_SIZE; i++)
  {
    item = &editor->pFontCache[i];
    if (!item->nRefs)
    {
      if (item->nAge > nAge)
        nEmpty = i, nAge = item->nAge;
    }
    if (item->hFont && ME_IsFontEqual(&item->lfSpecs, &lf))
      break;
  }
  if (i < HFONT_CACHE_SIZE) /* found */
  {
    item = &editor->pFontCache[i];
    TRACE_(richedit_style)("font reused %d\n", i);

    s->hFont = item->hFont;
    item->nRefs++;
  }
  else
  {
    item = &editor->pFontCache[nEmpty]; /* this legal even when nEmpty == -1, as we don't dereference it */

    assert(nEmpty != -1); /* otherwise we leak cache entries or get too many fonts at once*/
    if (item->hFont) {
      TRACE_(richedit_style)("font deleted %d\n", nEmpty);
      DeleteObject(item->hFont);
      item->hFont = NULL;
    }
    s->hFont = CreateFontIndirectW(&lf);
    assert(s->hFont);
    TRACE_(richedit_style)("font created %d\n", nEmpty);
    item->hFont = s->hFont;
    item->nRefs = 1;
    memcpy(&item->lfSpecs, &lf, sizeof(LOGFONTW));
  }
  hOldFont = SelectObject(hDC, s->hFont);
  /* should be cached too, maybe ? */
  GetTextMetricsW(hDC, &s->tm);
  return hOldFont;
}

void ME_UnselectStyleFont(ME_TextEditor *editor, HDC hDC, ME_Style *s, HFONT hOldFont)
{
  int i;

  assert(hDC);
  assert(s);
  SelectObject(hDC, hOldFont);
  for (i=0; i<HFONT_CACHE_SIZE; i++)
  {
    ME_FontCacheItem *pItem = &editor->pFontCache[i];
    if (pItem->hFont == s->hFont && pItem->nRefs > 0)
    {
      pItem->nRefs--;
      pItem->nAge = 0;
      s->hFont = NULL;
      return;
    }
  }
  assert(0 == "UnselectStyleFont without SelectStyleFont");
}

static void ME_DestroyStyle(ME_Style *s) {
  if (s->hFont)
  {
    DeleteObject(s->hFont);
    s->hFont = NULL;
  }
  FREE_OBJ(s);
}

void ME_AddRefStyle(ME_Style *s)
{
  assert(s->nRefs>0); /* style with 0 references isn't supposed to exist */
  s->nRefs++;
  all_refs++;
}

void ME_ReleaseStyle(ME_Style *s)
{
  s->nRefs--;
  all_refs--;
  if (s->nRefs==0)
    TRACE_(richedit_style)("destroy style %p, total refs=%d\n", s, all_refs);
  else
    TRACE_(richedit_style)("release style %p, new refs=%d, total refs=%d\n", s, s->nRefs, all_refs);
  if (!all_refs) TRACE("all style references freed (good!)\n");
  assert(s->nRefs>=0);
  if (!s->nRefs)
    ME_DestroyStyle(s);
}

ME_Style *ME_GetInsertStyle(ME_TextEditor *editor, int nCursor) {
  if (ME_IsSelection(editor))
  {
    ME_Cursor c;
    int from, to;

    ME_GetSelection(editor, &from, &to);
    ME_CursorFromCharOfs(editor, from, &c);
    ME_AddRefStyle(c.pRun->member.run.style);
    return c.pRun->member.run.style;
  }
  if (editor->pBuffer->pCharStyle) {
    ME_AddRefStyle(editor->pBuffer->pCharStyle);
    return editor->pBuffer->pCharStyle;
  }
  else
  {
    ME_Cursor *pCursor = &editor->pCursors[nCursor];
    ME_DisplayItem *pRunItem = pCursor->pRun;
    ME_DisplayItem *pPrevItem = NULL;
    if (pCursor->nOffset) {
      ME_Run *pRun = &pRunItem->member.run;
      ME_AddRefStyle(pRun->style);
      return pRun->style;
    }
    pPrevItem = ME_FindItemBack(pRunItem, diRunOrParagraph);
    if (pPrevItem->type == diRun)
    {
      ME_AddRefStyle(pPrevItem->member.run.style);
      return pPrevItem->member.run.style;
    }
    else
    {
      ME_AddRefStyle(pRunItem->member.run.style);
      return pRunItem->member.run.style;
    }
  }
}

void ME_SaveTempStyle(ME_TextEditor *editor)
{
  ME_Style *old_style = editor->pBuffer->pCharStyle;
  editor->pBuffer->pCharStyle = ME_GetInsertStyle(editor, 0);
  if (old_style)
    ME_ReleaseStyle(old_style);
}

void ME_ClearTempStyle(ME_TextEditor *editor)
{
  if (!editor->pBuffer->pCharStyle) return;
  ME_ReleaseStyle(editor->pBuffer->pCharStyle);
  editor->pBuffer->pCharStyle = NULL;
}
