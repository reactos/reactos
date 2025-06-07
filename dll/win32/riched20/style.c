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

BOOL cfany_to_cf2w(CHARFORMAT2W *to, const CHARFORMAT2W *from)
{
  if (from->cbSize == sizeof(CHARFORMATA))
  {
    CHARFORMATA *f = (CHARFORMATA *)from;
    CopyMemory(to, f, FIELD_OFFSET(CHARFORMATA, szFaceName));
    to->cbSize = sizeof(CHARFORMAT2W);
    if (f->dwMask & CFM_FACE) {
      MultiByteToWideChar(CP_ACP, 0, f->szFaceName, -1, to->szFaceName, ARRAY_SIZE(to->szFaceName));
    }
    return TRUE;
  }
  if (from->cbSize == sizeof(CHARFORMATW))
  {
    CHARFORMATW *f = (CHARFORMATW *)from;
    CopyMemory(to, f, sizeof(*f));
    /* theoretically, we don't need to zero the remaining memory */
    ZeroMemory(&to->wWeight, sizeof(CHARFORMAT2W)-FIELD_OFFSET(CHARFORMAT2W, wWeight));
    to->cbSize = sizeof(CHARFORMAT2W);
    return TRUE;
  }
  if (from->cbSize == sizeof(CHARFORMAT2A))
  {
    CHARFORMAT2A *f = (CHARFORMAT2A *)from;
    /* copy the A structure without face name */
    CopyMemory(to, f, FIELD_OFFSET(CHARFORMATA, szFaceName));
    /* convert face name */
    if (f->dwMask & CFM_FACE)
      MultiByteToWideChar(CP_ACP, 0, f->szFaceName, -1, to->szFaceName, ARRAY_SIZE(to->szFaceName));
    /* copy the rest of the 2A structure to 2W */
    CopyMemory(&to->wWeight, &f->wWeight, sizeof(CHARFORMAT2A)-FIELD_OFFSET(CHARFORMAT2A, wWeight));
    to->cbSize = sizeof(CHARFORMAT2W);
    return TRUE;
  }
  if (from->cbSize == sizeof(CHARFORMAT2W))
  {
    CopyMemory(to, from, sizeof(CHARFORMAT2W));
    return TRUE;
  }

  return FALSE;
}

BOOL cf2w_to_cfany(CHARFORMAT2W *to, const CHARFORMAT2W *from)
{
  assert(from->cbSize == sizeof(CHARFORMAT2W));
  if (to->cbSize == sizeof(CHARFORMATA))
  {
    CHARFORMATA *t = (CHARFORMATA *)to;
    CopyMemory(t, from, FIELD_OFFSET(CHARFORMATA, szFaceName));
    WideCharToMultiByte(CP_ACP, 0, from->szFaceName, -1, t->szFaceName, sizeof(t->szFaceName), NULL, NULL);
    t->cbSize = sizeof(*t); /* it was overwritten by CopyMemory */
    t->dwMask &= CFM_ALL;
    t->dwEffects &= CFM_EFFECTS;
    return TRUE;
  }
  if (to->cbSize == sizeof(CHARFORMATW))
  {
    CHARFORMATW *t = (CHARFORMATW *)to;
    CopyMemory(t, from, sizeof(*t));
    t->cbSize = sizeof(*t); /* it was overwritten by CopyMemory */
    t->dwMask &= CFM_ALL;
    t->dwEffects &= CFM_EFFECTS;
    return TRUE;
  }
  if (to->cbSize == sizeof(CHARFORMAT2A))
  {
    CHARFORMAT2A *t = (CHARFORMAT2A *)to;
    /* copy the A structure without face name */
    CopyMemory(t, from, FIELD_OFFSET(CHARFORMATA, szFaceName));
    /* convert face name */
    WideCharToMultiByte(CP_ACP, 0, from->szFaceName, -1, t->szFaceName, sizeof(t->szFaceName), NULL, NULL);
    /* copy the rest of the 2A structure to 2W */
    CopyMemory(&t->wWeight, &from->wWeight, sizeof(CHARFORMAT2W)-FIELD_OFFSET(CHARFORMAT2W,wWeight));
    t->cbSize = sizeof(*t); /* it was overwritten by CopyMemory */
    return TRUE;
  }
  if (to->cbSize == sizeof(CHARFORMAT2W))
  {
    CopyMemory(to, from, sizeof(CHARFORMAT2W));
    return TRUE;
  }
  return FALSE;
}

ME_Style *ME_MakeStyle(CHARFORMAT2W *style)
{
  ME_Style *s = malloc(sizeof(*s));

  assert(style->cbSize == sizeof(CHARFORMAT2W));
  s->fmt = *style;
  s->nRefs = 1;
  s->font_cache = NULL;
  memset(&s->tm, 0, sizeof(s->tm));
  s->tm.tmAscent = -1;
  s->script_cache = NULL;
  list_init(&s->entry);
  all_refs++;
  TRACE_(richedit_style)("ME_MakeStyle %p, total refs=%d\n", s, all_refs);
  return s;
}

#define COPY_STYLE_ITEM(mask, member) \
  if (mod->dwMask & mask) { \
    fmt.dwMask |= mask;\
    fmt.member = mod->member;\
  }

#define COPY_STYLE_ITEM_MEMCPY(mask, member) \
  if (mod->dwMask & mask) { \
    fmt.dwMask |= mask;\
    CopyMemory(fmt.member, mod->member, sizeof(mod->member));\
  }

void ME_InitCharFormat2W(CHARFORMAT2W *pFmt)
{
  ZeroMemory(pFmt, sizeof(CHARFORMAT2W));
  pFmt->cbSize = sizeof(CHARFORMAT2W);
}

ME_Style *ME_ApplyStyle(ME_TextEditor *editor, ME_Style *sSrc, CHARFORMAT2W *mod)
{
  CHARFORMAT2W fmt = sSrc->fmt;
  ME_Style *s;

  assert(mod->cbSize == sizeof(CHARFORMAT2W));
  COPY_STYLE_ITEM(CFM_ANIMATION, bAnimation);
  COPY_STYLE_ITEM(CFM_BACKCOLOR, crBackColor);
  COPY_STYLE_ITEM(CFM_CHARSET, bCharSet);
  COPY_STYLE_ITEM(CFM_COLOR, crTextColor);
  COPY_STYLE_ITEM_MEMCPY(CFM_FACE, szFaceName);
  COPY_STYLE_ITEM(CFM_KERNING, wKerning);
  COPY_STYLE_ITEM(CFM_LCID, lcid);
  COPY_STYLE_ITEM(CFM_OFFSET, yOffset);
  COPY_STYLE_ITEM(CFM_REVAUTHOR, bRevAuthor);
  if (mod->dwMask & CFM_SIZE) {
    fmt.dwMask |= CFM_SIZE;
    fmt.yHeight = min(mod->yHeight, yHeightCharPtsMost * 20);
  }
  COPY_STYLE_ITEM(CFM_SPACING, sSpacing);
  COPY_STYLE_ITEM(CFM_STYLE, sStyle);
  COPY_STYLE_ITEM(CFM_WEIGHT, wWeight);
  /* FIXME: this is not documented this way, but that's the more logical */
  COPY_STYLE_ITEM(CFM_FACE, bPitchAndFamily);

  fmt.dwEffects &= ~(mod->dwMask);
  fmt.dwEffects |= mod->dwEffects & mod->dwMask;
  fmt.dwMask |= mod->dwMask;
  if (mod->dwMask & CFM_COLOR)
  {
    if (mod->dwEffects & CFE_AUTOCOLOR)
      fmt.dwEffects |= CFE_AUTOCOLOR;
    else
      fmt.dwEffects &= ~CFE_AUTOCOLOR;
  }

  COPY_STYLE_ITEM(CFM_UNDERLINETYPE, bUnderlineType);
  /* If the CFM_UNDERLINE effect is not specified, set it appropriately */
  if ((mod->dwMask & CFM_UNDERLINETYPE) && !(mod->dwMask & CFM_UNDERLINE))
  {
      fmt.dwMask |= CFM_UNDERLINE;
      if (mod->bUnderlineType == CFU_UNDERLINENONE)
          fmt.dwEffects &= ~CFE_UNDERLINE;
      else
          fmt.dwEffects |= CFE_UNDERLINE;
  }

  if (mod->dwMask & CFM_BOLD && !(mod->dwMask & CFM_WEIGHT))
  {
      fmt.wWeight = (mod->dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL;
  } else if (mod->dwMask & CFM_WEIGHT && !(mod->dwMask & CFM_BOLD)) {
      if (mod->wWeight > FW_NORMAL)
          fmt.dwEffects |= CFE_BOLD;
      else
          fmt.dwEffects &= ~CFE_BOLD;
  }

  LIST_FOR_EACH_ENTRY(s, &editor->style_list, ME_Style, entry)
  {
      if (!memcmp( &s->fmt, &fmt, sizeof(fmt) ))
      {
          TRACE_(richedit_style)("found existing style %p\n", s);
          ME_AddRefStyle( s );
          return s;
      }
  }

  s = ME_MakeStyle( &fmt );
  if (s)
      list_add_head( &editor->style_list, &s->entry );
  TRACE_(richedit_style)("created new style %p\n", s);
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
    p += sprintf(p, "\nFont size:            %ld\n", pFmt->yHeight);
  else
    p += sprintf(p, "\nFont size:            N/A\n");

  if (pFmt->dwMask & CFM_OFFSET)
    p += sprintf(p, "Char offset:          %ld\n", pFmt->yOffset);
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
  if ((s->fmt.dwEffects & s->fmt.dwMask & CFM_UNDERLINE) &&
      !(s->fmt.dwEffects & CFE_LINK) &&
      s->fmt.bUnderlineType == CFU_CF1UNDERLINE)
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
  fmt->dwMask = CFM_WEIGHT|CFM_BOLD|CFM_ITALIC|CFM_UNDERLINE|CFM_UNDERLINETYPE|CFM_STRIKEOUT|CFM_SIZE|CFM_FACE|CFM_CHARSET;
  fmt->wWeight = lf->lfWeight;
  fmt->yHeight = -lf->lfHeight*1440/ry;
  if (lf->lfWeight > FW_NORMAL) fmt->dwEffects |= CFM_BOLD;
  if (lf->lfItalic) fmt->dwEffects |= CFM_ITALIC;
  if (lf->lfUnderline) fmt->dwEffects |= CFM_UNDERLINE;
  fmt->bUnderlineType = CFU_UNDERLINE;
  if (lf->lfStrikeOut) fmt->dwEffects |= CFM_STRIKEOUT;
  fmt->bPitchAndFamily = lf->lfPitchAndFamily;
  fmt->bCharSet = lf->lfCharSet;
}

static BOOL ME_IsFontEqual(const LOGFONTW *p1, const LOGFONTW *p2)
{
  if (memcmp(p1, p2, sizeof(LOGFONTW)-sizeof(p1->lfFaceName)))
    return FALSE;
  if (wcscmp(p1->lfFaceName, p2->lfFaceName))
    return FALSE;
  return TRUE;
}

static void release_font_cache(ME_FontCacheItem *item)
{
    if (item->nRefs > 0)
    {
        item->nRefs--;
        item->nAge = 0;
    }
}

void select_style( ME_Context *c, ME_Style *s )
{
    HFONT old_font;
    LOGFONTW lf;
    int i, empty, age = 0x7FFFFFFF;
    ME_FontCacheItem *item;

    if (c->current_style == s) return;

    if (s)
    {
        ME_LogFontFromStyle( c, &lf, s );

        for (i = 0; i < HFONT_CACHE_SIZE; i++)
            c->editor->pFontCache[i].nAge++;
        for (i = 0, empty = -1, age = 0; i < HFONT_CACHE_SIZE; i++)
        {
            item = &c->editor->pFontCache[i];
            if (!item->nRefs)
            {
                if (item->nAge > age)
                {
                    empty = i;
                    age = item->nAge;
                }
            }

            if (item->hFont && ME_IsFontEqual( &item->lfSpecs, &lf ))
                break;
        }

        if (i < HFONT_CACHE_SIZE) /* found */
        {
            item = &c->editor->pFontCache[i];
            TRACE_(richedit_style)( "font reused %d\n", i );
            item->nRefs++;
        }
        else
        {
            assert(empty != -1);
            item = &c->editor->pFontCache[empty];
            if (item->hFont)
            {
                TRACE_(richedit_style)( "font deleted %d\n", empty );
                DeleteObject(item->hFont);
                item->hFont = NULL;
            }
            item->hFont = CreateFontIndirectW( &lf );
            TRACE_(richedit_style)( "font created %d\n", empty );
            item->nRefs = 1;
            item->lfSpecs = lf;
        }
        s->font_cache = item;
        old_font = SelectObject( c->hDC, item->hFont );
        GetTextMetricsW( c->hDC, &s->tm );
        if (!c->orig_font) c->orig_font = old_font;
    }
    else
    {
        SelectObject( c->hDC, c->orig_font );
        c->orig_font = NULL;
    }

    if (c->current_style && c->current_style->font_cache)
    {
        release_font_cache( c->current_style->font_cache );
        c->current_style->font_cache = NULL;
    }
    c->current_style = s;

    return;
}

void ME_DestroyStyle(ME_Style *s)
{
  list_remove( &s->entry );
  if (s->font_cache)
  {
    release_font_cache( s->font_cache );
    s->font_cache = NULL;
  }
  ScriptFreeCache( &s->script_cache );
  free(s);
}

void ME_AddRefStyle(ME_Style *s)
{
  assert(s->nRefs>0); /* style with 0 references isn't supposed to exist */
  s->nRefs++;
  all_refs++;
  TRACE_(richedit_style)("ME_AddRefStyle %p, new refs=%d, total refs=%d\n", s, s->nRefs, all_refs);
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

ME_Style *style_get_insert_style( ME_TextEditor *editor, ME_Cursor *cursor )
{
    ME_Style *style;
    ME_Cursor *from, *to;
    ME_Run *prev;

    if (ME_IsSelection( editor ))
    {
        ME_GetSelection( editor, &from, &to );
        style = from->run->style;
    }
    else if (editor->pBuffer->pCharStyle)
        style = editor->pBuffer->pCharStyle;
    else if (!cursor->nOffset && (prev = run_prev( cursor->run )))
        style = prev->style;
    else
        style = cursor->run->style;

    ME_AddRefStyle( style );
    return style;
}

void ME_SaveTempStyle(ME_TextEditor *editor, ME_Style *style)
{
  ME_Style *old_style = editor->pBuffer->pCharStyle;

  if (style) ME_AddRefStyle( style );
  editor->pBuffer->pCharStyle = style;
  if (old_style) ME_ReleaseStyle( old_style );
}

void ME_ClearTempStyle(ME_TextEditor *editor)
{
  if (!editor->pBuffer->pCharStyle) return;
  ME_ReleaseStyle(editor->pBuffer->pCharStyle);
  editor->pBuffer->pCharStyle = NULL;
}

/******************************************************************************
 * ME_SetDefaultCharFormat
 *
 * Applies a style change to the default character style.
 *
 * The default style is special in that it is mutable - runs
 * in the document that have this style should change if the
 * default style changes.  That means we need to fix up this
 * style manually.
 */
void ME_SetDefaultCharFormat(ME_TextEditor *editor, CHARFORMAT2W *mod)
{
    ME_Style *style, *def = editor->pBuffer->pDefaultStyle;

    assert(mod->cbSize == sizeof(CHARFORMAT2W));
    style = ME_ApplyStyle(editor, def, mod);
    def->fmt = style->fmt;
    def->tm = style->tm;
    if (def->font_cache)
    {
        release_font_cache( def->font_cache );
        def->font_cache = NULL;
    }
    ScriptFreeCache( &def->script_cache );
    ME_ReleaseStyle( style );
    editor_mark_rewrap_all( editor );
}
