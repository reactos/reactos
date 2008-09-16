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

/* the following routines assume that:
 * - char2[AW] extends char[AW] by adding fields at the end of the charA form)
 * - szFaceName is the last field of char[AW] form, and wWeight the first of 2[AW]
 * - the difference between A and W form is the szFaceName as Ansi vs Unicode string
 * - because of alignment, offset of wWeight field in 2[AW] structure *IS NOT*
 *   sizeof(char[AW])
 */

CHARFORMAT2W *ME_ToCF2W(CHARFORMAT2W *to, CHARFORMAT2W *from)
{
  if (from->cbSize == sizeof(CHARFORMATA))
  {
    CHARFORMATA *f = (CHARFORMATA *)from;
    CopyMemory(to, f, FIELD_OFFSET(CHARFORMATA, szFaceName));
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
    ZeroMemory(&to->wWeight, sizeof(CHARFORMAT2W)-FIELD_OFFSET(CHARFORMAT2W, wWeight));
    to->cbSize = sizeof(CHARFORMAT2W);
    return to;
  }
  if (from->cbSize == sizeof(CHARFORMAT2A))
  {
    CHARFORMAT2A *f = (CHARFORMAT2A *)from;
    /* copy the A structure without face name */
    CopyMemory(to, f, FIELD_OFFSET(CHARFORMATA, szFaceName));
    /* convert face name */
    if (f->dwMask & CFM_FACE)
      MultiByteToWideChar(0, 0, f->szFaceName, -1, to->szFaceName, sizeof(to->szFaceName)/sizeof(WCHAR));
    /* copy the rest of the 2A structure to 2W */
    CopyMemory(&to->wWeight, &f->wWeight, sizeof(CHARFORMAT2A)-FIELD_OFFSET(CHARFORMAT2A, wWeight));
    to->cbSize = sizeof(CHARFORMAT2W);
    return to;
  }

  return (from->cbSize >= sizeof(CHARFORMAT2W)) ? from : NULL;
}

void ME_CopyToCF2W(CHARFORMAT2W *to, CHARFORMAT2W *from)
{
  if (ME_ToCF2W(to, from) == from)
    *to = *from;
}

CHARFORMAT2W *ME_ToCFAny(CHARFORMAT2W *to, CHARFORMAT2W *from)
{
  assert(from->cbSize == sizeof(CHARFORMAT2W));
  if (to->cbSize == sizeof(CHARFORMATA))
  {
    CHARFORMATA *t = (CHARFORMATA *)to;
    CopyMemory(t, from, FIELD_OFFSET(CHARFORMATA, szFaceName));
    WideCharToMultiByte(0, 0, from->szFaceName, -1, t->szFaceName, sizeof(t->szFaceName), 0, 0);
    if (from->dwMask & CFM_UNDERLINETYPE)
    {
        switch (from->bUnderlineType)
        {
        case CFU_CF1UNDERLINE:
            to->dwMask |= CFM_UNDERLINE;
            to->dwEffects |= CFE_UNDERLINE;
            break;
        case CFU_UNDERLINENONE:
            to->dwMask |= CFM_UNDERLINE;
            to->dwEffects &= ~CFE_UNDERLINE;
            break;
        }
    }
    t->cbSize = sizeof(*t); /* it was overwritten by CopyMemory */
    return to;
  }
  if (to->cbSize == sizeof(CHARFORMATW))
  {
    CHARFORMATW *t = (CHARFORMATW *)to;
    CopyMemory(t, from, sizeof(*t));
    if (from->dwMask & CFM_UNDERLINETYPE)
    {
        switch (from->bUnderlineType)
        {
        case CFU_CF1UNDERLINE:
            to->dwMask |= CFM_UNDERLINE;
            to->dwEffects |= CFE_UNDERLINE;
            break;
        case CFU_UNDERLINENONE:
            to->dwMask |= CFM_UNDERLINE;
            to->dwEffects &= ~CFE_UNDERLINE;
            break;
        }
    }
    t->cbSize = sizeof(*t); /* it was overwritten by CopyMemory */
    return to;
  }
  if (to->cbSize == sizeof(CHARFORMAT2A))
  {
    CHARFORMAT2A *t = (CHARFORMAT2A *)to;
    /* copy the A structure without face name */
    CopyMemory(t, from, FIELD_OFFSET(CHARFORMATA, szFaceName));
    /* convert face name */
    WideCharToMultiByte(0, 0, from->szFaceName, -1, t->szFaceName, sizeof(t->szFaceName), 0, 0);
    /* copy the rest of the 2A structure to 2W */
    CopyMemory(&t->wWeight, &from->wWeight, sizeof(CHARFORMAT2W)-FIELD_OFFSET(CHARFORMAT2W,wWeight));
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
    s->fmt = *style;
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
  if (style->dwMask & CFM_SIZE) {
    s->fmt.dwMask |= CFM_SIZE;
    s->fmt.yHeight = min(style->yHeight, yHeightCharPtsMost * 20);
  }
  COPY_STYLE_ITEM(CFM_SPACING, sSpacing);
  COPY_STYLE_ITEM(CFM_STYLE, sStyle);
  COPY_STYLE_ITEM(CFM_UNDERLINETYPE, bUnderlineType);
  COPY_STYLE_ITEM(CFM_WEIGHT, wWeight);
  /* FIXME: this is not documented this way, but that's the more logical */
  COPY_STYLE_ITEM(CFM_FACE, bPitchAndFamily);

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
  if (style->dwMask & CFM_UNDERLINE)
  {
      s->fmt.dwMask |= CFM_UNDERLINETYPE;
      s->fmt.bUnderlineType = (style->dwEffects & CFM_UNDERLINE) ?
          CFU_CF1UNDERLINE : CFU_UNDERLINENONE;
  }
  if (style->dwMask & CFM_BOLD && !(style->dwMask & CFM_WEIGHT))
  {
      s->fmt.wWeight = (style->dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL;
  } else if (style->dwMask & CFM_WEIGHT && !(style->dwMask & CFM_BOLD)) {
      if (style->wWeight > FW_NORMAL)
          s->fmt.dwEffects |= CFE_BOLD;
      else
          s->fmt.dwEffects &= ~CFE_BOLD;
  }
  return s;
}

void ME_CopyCharFormat(CHARFORMAT2W *pDest, const CHARFORMAT2W *pSrc)
{
  /* using this with non-2W structs is forbidden */
  assert(pSrc->cbSize == sizeof(CHARFORMAT2W));
  assert(pDest->cbSize == sizeof(CHARFORMAT2W));
  *pDest = *pSrc;
}

static void ME_DumpStyleEffect(char **p, const char *name, const CHARFORMAT2W *fmt, int mask)
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
    p += sprintf(p, "\nFont size:            %d\n", pFmt->yHeight);
  else
    p += sprintf(p, "\nFont size:            N/A\n");

  if (pFmt->dwMask & CFM_OFFSET)
    p += sprintf(p, "Char offset:          %d\n", pFmt->yOffset);
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
ME_LogFontFromStyle(ME_Context* c, LOGFONTW *lf, const ME_Style *s)
{
  ZeroMemory(lf, sizeof(LOGFONTW));
  lstrcpyW(lf->lfFaceName, s->fmt.szFaceName);

  lf->lfHeight = ME_twips2pointsY(c, -s->fmt.yHeight);
  
  lf->lfWeight = FW_NORMAL;
  if (s->fmt.dwEffects & s->fmt.dwMask & CFM_BOLD)
    lf->lfWeight = FW_BOLD;
  if (s->fmt.dwMask & CFM_WEIGHT)
    lf->lfWeight = s->fmt.wWeight;
  if (s->fmt.dwEffects & s->fmt.dwMask & CFM_ITALIC)
    lf->lfItalic = 1;
  if (s->fmt.dwEffects & s->fmt.dwMask & (CFM_UNDERLINE | CFE_LINK))
    lf->lfUnderline = 1;
  if (s->fmt.dwMask & CFM_UNDERLINETYPE && s->fmt.bUnderlineType == CFU_CF1UNDERLINE)
    lf->lfUnderline = 1;
  if (s->fmt.dwEffects & s->fmt.dwMask & CFM_STRIKEOUT)
    lf->lfStrikeOut = 1;
  if (s->fmt.dwEffects & s->fmt.dwMask & (CFM_SUBSCRIPT|CFM_SUPERSCRIPT))
    lf->lfHeight = (lf->lfHeight*2)/3;
/*lf.lfQuality = PROOF_QUALITY; */
  if (s->fmt.dwMask & CFM_FACE)
    lf->lfPitchAndFamily = s->fmt.bPitchAndFamily;
  if (s->fmt.dwMask & CFM_CHARSET)
    lf->lfCharSet = s->fmt.bCharSet;
}

void ME_CharFormatFromLogFont(HDC hDC, const LOGFONTW *lf, CHARFORMAT2W *fmt)
{
  int ry;

  ME_InitCharFormat2W(fmt);
  ry = GetDeviceCaps(hDC, LOGPIXELSY);
  lstrcpyW(fmt->szFaceName, lf->lfFaceName);
  fmt->dwEffects = 0;
  fmt->dwMask = CFM_WEIGHT|CFM_BOLD|CFM_ITALIC|CFM_UNDERLINE|CFM_STRIKEOUT|CFM_SIZE|CFM_FACE|CFM_CHARSET;
  fmt->wWeight = lf->lfWeight;
  fmt->yHeight = -lf->lfHeight*1440/ry;
  if (lf->lfWeight > FW_NORMAL) fmt->dwEffects |= CFM_BOLD;
  if (lf->lfItalic) fmt->dwEffects |= CFM_ITALIC;
  if (lf->lfUnderline) fmt->dwEffects |= CFM_UNDERLINE;
  /* notice that if a logfont was created with underline due to CFM_LINK, this
      would add an erroneous CFM_UNDERLINE. This isn't currently ever a problem. */
  if (lf->lfStrikeOut) fmt->dwEffects |= CFM_STRIKEOUT;
  fmt->bPitchAndFamily = lf->lfPitchAndFamily;
  fmt->bCharSet = lf->lfCharSet;
}

static BOOL ME_IsFontEqual(const LOGFONTW *p1, const LOGFONTW *p2)
{
  if (memcmp(p1, p2, sizeof(LOGFONTW)-sizeof(p1->lfFaceName)))
    return FALSE;
  if (lstrcmpW(p1->lfFaceName, p2->lfFaceName))
    return FALSE;
  return TRUE;
}

HFONT ME_SelectStyleFont(ME_Context *c, ME_Style *s)
{
  HFONT hOldFont;
  LOGFONTW lf;
  int i, nEmpty, nAge = 0x7FFFFFFF;
  ME_FontCacheItem *item;
  assert(c->hDC);
  assert(s);
  
  ME_LogFontFromStyle(c, &lf, s);
  
  for (i=0; i<HFONT_CACHE_SIZE; i++)
    c->editor->pFontCache[i].nAge++;
  for (i=0, nEmpty=-1, nAge=0; i<HFONT_CACHE_SIZE; i++)
  {
    item = &c->editor->pFontCache[i];
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
    item = &c->editor->pFontCache[i];
    TRACE_(richedit_style)("font reused %d\n", i);

    s->hFont = item->hFont;
    item->nRefs++;
  }
  else
  {
    item = &c->editor->pFontCache[nEmpty]; /* this legal even when nEmpty == -1, as we don't dereference it */

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
    item->lfSpecs = lf;
  }
  hOldFont = SelectObject(c->hDC, s->hFont);
  /* should be cached too, maybe ? */
  GetTextMetricsW(c->hDC, &s->tm);
  return hOldFont;
}

void ME_UnselectStyleFont(ME_Context *c, ME_Style *s, HFONT hOldFont)
{
  int i;
  
  assert(c->hDC);
  assert(s);
  SelectObject(c->hDC, hOldFont);
  for (i=0; i<HFONT_CACHE_SIZE; i++)
  {
    ME_FontCacheItem *pItem = &c->editor->pFontCache[i];
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
