/*
 * RichEdit - functions working on paragraphs of text (diParagraph).
 * 
 * Copyright 2004 by Krzysztof Foltman
 * Copyright 2006 by Phil Krylov
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

void mark_para_rewrap(ME_TextEditor *editor, ME_DisplayItem *para)
{
    para->member.para.nFlags |= MEPF_REWRAP;
    add_marked_para(editor, para);
}

ME_DisplayItem *get_di_from_para(ME_Paragraph *para)
{
    return (ME_DisplayItem *)((ptrdiff_t)para - offsetof(ME_DisplayItem, member));
}

static ME_DisplayItem *make_para(ME_TextEditor *editor)
{
    ME_DisplayItem *item = ME_MakeDI(diParagraph);

    ME_SetDefaultParaFormat(editor, &item->member.para.fmt);
    item->member.para.nFlags = MEPF_REWRAP;
    item->member.para.next_marked = item->member.para.prev_marked = NULL;

    return item;
}

void destroy_para(ME_TextEditor *editor, ME_DisplayItem *item)
{
    assert(item->type == diParagraph);

    if (item->member.para.nWidth == editor->nTotalWidth)
    {
        item->member.para.nWidth = 0;
        editor->nTotalWidth = get_total_width(editor);
    }
    editor->total_rows -= item->member.para.nRows;
    ME_DestroyString(item->member.para.text);
    para_num_clear( &item->member.para.para_num );
    remove_marked_para(editor, item);
    ME_DestroyDisplayItem(item);
}

int get_total_width(ME_TextEditor *editor)
{
    ME_Paragraph *para;
    int total_width = 0;

    if (editor->pBuffer->pFirst && editor->pBuffer->pLast)
    {
        para = &editor->pBuffer->pFirst->next->member.para;
        while (para != &editor->pBuffer->pLast->member.para && para->next_para)
        {
            total_width = max(total_width, para->nWidth);
            para = &para->next_para->member.para;
        }
    }

    return total_width;
}

void remove_marked_para(ME_TextEditor *editor, ME_DisplayItem *di)
{
    ME_DisplayItem *head = editor->first_marked_para;

    assert(di->type == diParagraph);
    if (!di->member.para.next_marked && !di->member.para.prev_marked)
    {
        if (di == head)
            editor->first_marked_para = NULL;
    }
    else if (di->member.para.next_marked && di->member.para.prev_marked)
    {
        di->member.para.prev_marked->member.para.next_marked = di->member.para.next_marked;
        di->member.para.next_marked->member.para.prev_marked = di->member.para.prev_marked;
        di->member.para.prev_marked = di->member.para.next_marked = NULL;
    }
    else if (di->member.para.next_marked)
    {
        assert(di == editor->first_marked_para);
        editor->first_marked_para = di->member.para.next_marked;
        di->member.para.next_marked->member.para.prev_marked = NULL;
        di->member.para.next_marked = NULL;
    }
    else
    {
        di->member.para.prev_marked->member.para.next_marked = NULL;
        di->member.para.prev_marked = NULL;
    }
}

void add_marked_para(ME_TextEditor *editor, ME_DisplayItem *di)
{
    ME_DisplayItem *iter = editor->first_marked_para;

    if (!iter)
    {
        editor->first_marked_para = di;
        return;
    }
    while (iter)
    {
        if (iter == di)
            return;
        else if (di->member.para.nCharOfs < iter->member.para.nCharOfs)
        {
            if (iter == editor->first_marked_para)
                editor->first_marked_para = di;
            di->member.para.next_marked = iter;
            iter->member.para.prev_marked = di;
            break;
        }
        else if (di->member.para.nCharOfs >= iter->member.para.nCharOfs)
        {
            if (!iter->member.para.next_marked || di->member.para.nCharOfs < iter->member.para.next_marked->member.para.nCharOfs)
            {
                if (iter->member.para.next_marked)
                {
                    di->member.para.next_marked = iter->member.para.next_marked;
                    iter->member.para.next_marked->member.para.prev_marked = di;
                }
                di->member.para.prev_marked = iter;
                iter->member.para.next_marked = di;
                break;
            }
        }
        iter = iter->member.para.next_marked;
    }
}

void ME_MakeFirstParagraph(ME_TextEditor *editor)
{
  static const WCHAR cr_lf[] = {'\r','\n',0};
  ME_Context c;
  CHARFORMAT2W cf;
  const CHARFORMATW *host_cf;
  LOGFONTW lf;
  HFONT hf;
  ME_TextBuffer *text = editor->pBuffer;
  ME_DisplayItem *para = make_para(editor);
  ME_DisplayItem *run;
  ME_Style *style;
  int eol_len;

  ME_InitContext(&c, editor, ITextHost_TxGetDC(editor->texthost));

  hf = GetStockObject(SYSTEM_FONT);
  assert(hf);
  GetObjectW(hf, sizeof(LOGFONTW), &lf);
  ZeroMemory(&cf, sizeof(cf));
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_ANIMATION|CFM_BACKCOLOR|CFM_CHARSET|CFM_COLOR|CFM_FACE|CFM_KERNING|CFM_LCID|CFM_OFFSET;
  cf.dwMask |= CFM_REVAUTHOR|CFM_SIZE|CFM_SPACING|CFM_STYLE|CFM_UNDERLINETYPE|CFM_WEIGHT;
  cf.dwMask |= CFM_ALLCAPS|CFM_BOLD|CFM_DISABLED|CFM_EMBOSS|CFM_HIDDEN;
  cf.dwMask |= CFM_IMPRINT|CFM_ITALIC|CFM_LINK|CFM_OUTLINE|CFM_PROTECTED;
  cf.dwMask |= CFM_REVISED|CFM_SHADOW|CFM_SMALLCAPS|CFM_STRIKEOUT;
  cf.dwMask |= CFM_SUBSCRIPT|CFM_UNDERLINE;
  
  cf.dwEffects = CFE_AUTOCOLOR | CFE_AUTOBACKCOLOR;
  lstrcpyW(cf.szFaceName, lf.lfFaceName);
  /* Convert system font height from logical units to twips for cf.yHeight */
  cf.yHeight = (lf.lfHeight * 72 * 1440) / (c.dpi.cy * c.dpi.cy);
  if (lf.lfWeight > FW_NORMAL) cf.dwEffects |= CFE_BOLD;
  cf.wWeight = lf.lfWeight;
  if (lf.lfItalic) cf.dwEffects |= CFE_ITALIC;
  if (lf.lfUnderline) cf.dwEffects |= CFE_UNDERLINE;
  cf.bUnderlineType = CFU_UNDERLINE;
  if (lf.lfStrikeOut) cf.dwEffects |= CFE_STRIKEOUT;
  cf.bPitchAndFamily = lf.lfPitchAndFamily;
  cf.bCharSet = lf.lfCharSet;
  cf.lcid = GetSystemDefaultLCID();

  style = ME_MakeStyle(&cf);
  text->pDefaultStyle = style;

  if (ITextHost_TxGetCharFormat(editor->texthost, &host_cf) == S_OK)
  {
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cfany_to_cf2w(&cf, (CHARFORMAT2W *)host_cf);
    ME_SetDefaultCharFormat(editor, &cf);
  }

  eol_len = editor->bEmulateVersion10 ? 2 : 1;
  para->member.para.text = ME_MakeStringN( cr_lf, eol_len );

  run = ME_MakeRun(style, MERF_ENDPARA);
  run->member.run.nCharOfs = 0;
  run->member.run.len = eol_len;
  run->member.run.para = &para->member.para;

  para->member.para.eop_run = &run->member.run;

  ME_InsertBefore(text->pLast, para);
  ME_InsertBefore(text->pLast, run);
  para->member.para.prev_para = text->pFirst;
  para->member.para.next_para = text->pLast;
  text->pFirst->member.para.next_para = para;
  text->pLast->member.para.prev_para = para;

  text->pLast->member.para.nCharOfs = editor->bEmulateVersion10 ? 2 : 1;

  add_marked_para(editor, para);
  ME_DestroyContext(&c);
}

static void ME_MarkForWrapping(ME_TextEditor *editor, ME_DisplayItem *first, const ME_DisplayItem *last)
{
  while(first != last)
  {
    mark_para_rewrap(editor, first);
    first = first->member.para.next_para;
  }
}

void ME_MarkAllForWrapping(ME_TextEditor *editor)
{
  ME_MarkForWrapping(editor, editor->pBuffer->pFirst->member.para.next_para, editor->pBuffer->pLast);
}

static void ME_UpdateTableFlags(ME_DisplayItem *para)
{
  para->member.para.fmt.dwMask |= PFM_TABLE|PFM_TABLEROWDELIMITER;
  if (para->member.para.pCell) {
    para->member.para.nFlags |= MEPF_CELL;
  } else {
    para->member.para.nFlags &= ~MEPF_CELL;
  }
  if (para->member.para.nFlags & MEPF_ROWEND) {
    para->member.para.fmt.wEffects |= PFE_TABLEROWDELIMITER;
  } else {
    para->member.para.fmt.wEffects &= ~PFE_TABLEROWDELIMITER;
  }
  if (para->member.para.nFlags & (MEPF_ROWSTART|MEPF_CELL|MEPF_ROWEND))
    para->member.para.fmt.wEffects |= PFE_TABLE;
  else
    para->member.para.fmt.wEffects &= ~PFE_TABLE;
}

static inline BOOL para_num_same_list( const PARAFORMAT2 *item, const PARAFORMAT2 *base )
{
    return item->wNumbering == base->wNumbering &&
        item->wNumberingStart == base->wNumberingStart &&
        item->wNumberingStyle == base->wNumberingStyle &&
        !(item->wNumberingStyle & PFNS_NEWNUMBER);
}

static int para_num_get_num( ME_Paragraph *para )
{
    ME_DisplayItem *prev;
    int num = para->fmt.wNumberingStart;

    for (prev = para->prev_para; prev->type == diParagraph;
         para = &prev->member.para, prev = prev->member.para.prev_para, num++)
    {
        if (!para_num_same_list( &prev->member.para.fmt, &para->fmt )) break;
    }
    return num;
}

static ME_String *para_num_get_str( ME_Paragraph *para, WORD num )
{
    /* max 4 Roman letters (representing '8') / decade + '(' + ')' */
    ME_String *str = ME_MakeStringEmpty( 20 + 2 );
    WCHAR *p;
    static const WCHAR fmtW[] = {'%', 'd', 0};
    static const WORD letter_base[] = { 1, 26, 26 * 26, 26 * 26 * 26 };
    /* roman_base should start on a '5' not a '1', otherwise the 'total' code will need adjusting.
       'N' and 'O' are what MS uses for 5000 and 10000, their version doesn't work well above 30000,
       but we'll use 'P' as the obvious extension, this gets us up to 2^16, which is all we care about. */
    static const struct
    {
        int base;
        char letter;
    }
    roman_base[] =
    {
        {50000, 'P'}, {10000, 'O'}, {5000, 'N'}, {1000, 'M'},
        {500, 'D'}, {100, 'C'}, {50, 'L'}, {10, 'X'}, {5, 'V'}, {1, 'I'}
    };
    int i, len;
    WORD letter, total, char_offset = 0;

    if (!str) return NULL;

    p = str->szData;

    if ((para->fmt.wNumberingStyle & 0xf00) == PFNS_PARENS)
        *p++ = '(';

    switch (para->fmt.wNumbering)
    {
    case PFN_ARABIC:
    default:
        p += swprintf( p, fmtW, num );
        break;

    case PFN_LCLETTER:
        char_offset = 'a' - 'A';
        /* fall through */
    case PFN_UCLETTER:
        if (!num) num = 1;

        /* This is not base-26 (or 27) as zeros don't count unless they are leading zeros.
           It's simplest to start with the least significant letter, so first calculate how many letters are needed. */
        for (i = 0, total = 0; i < ARRAY_SIZE( letter_base ); i++)
        {
            total += letter_base[i];
            if (num < total) break;
        }
        len = i;
        for (i = 0; i < len; i++)
        {
            num -= letter_base[i];
            letter = (num / letter_base[i]) % 26;
            p[len - i - 1] = letter + 'A' + char_offset;
        }
        p += len;
        *p = 0;
        break;

    case PFN_LCROMAN:
        char_offset = 'a' - 'A';
        /* fall through */
    case PFN_UCROMAN:
        if (!num) num = 1;

        for (i = 0; i < ARRAY_SIZE( roman_base ); i++)
        {
            if (i > 0)
            {
                if (i % 2 == 0) /* eg 5000, check for 9000 */
                    total = roman_base[i].base + 4 * roman_base[i + 1].base;
                else  /* eg 1000, check for 4000 */
                    total = 4 * roman_base[i].base;

                if (num / total)
                {
                    *p++ = roman_base[(i & ~1) + 1].letter + char_offset;
                    *p++ = roman_base[i - 1].letter + char_offset;
                    num -= total;
                    continue;
                }
            }

            len = num / roman_base[i].base;
            while (len--)
            {
                *p++ = roman_base[i].letter + char_offset;
                num -= roman_base[i].base;
            }
        }
        *p = 0;
        break;
    }

    switch (para->fmt.wNumberingStyle & 0xf00)
    {
    case PFNS_PARENS:
    case PFNS_PAREN:
        *p++ = ')';
        *p = 0;
        break;

    case PFNS_PERIOD:
        *p++ = '.';
        *p = 0;
        break;
    }

    str->nLen = p - str->szData;
    return str;
}

void para_num_init( ME_Context *c, ME_Paragraph *para )
{
    ME_Style *style;
    CHARFORMAT2W cf;
    static const WCHAR bullet_font[] = {'S','y','m','b','o','l',0};
    static const WCHAR bullet_str[] = {0xb7, 0};
    static const WCHAR spaceW[] = {' ', 0};
    SIZE sz;

    if (!para->fmt.wNumbering) return;
    if (para->para_num.style && para->para_num.text) return;

    if (!para->para_num.style)
    {
        style = para->eop_run->style;

        if (para->fmt.wNumbering == PFN_BULLET)
        {
            cf.cbSize = sizeof(cf);
            cf.dwMask = CFM_FACE | CFM_CHARSET;
            memcpy( cf.szFaceName, bullet_font, sizeof(bullet_font) );
            cf.bCharSet = SYMBOL_CHARSET;
            style = ME_ApplyStyle( c->editor, style, &cf );
        }
        else
        {
            ME_AddRefStyle( style );
        }

        para->para_num.style = style;
    }

    if (!para->para_num.text)
    {
        if (para->fmt.wNumbering != PFN_BULLET)
            para->para_num.text = para_num_get_str( para, para_num_get_num( para ) );
        else
            para->para_num.text = ME_MakeStringConst( bullet_str, 1 );
    }

    select_style( c, para->para_num.style );
    GetTextExtentPointW( c->hDC, para->para_num.text->szData, para->para_num.text->nLen, &sz );
    para->para_num.width = sz.cx;
    GetTextExtentPointW( c->hDC, spaceW, 1, &sz );
    para->para_num.width += sz.cx;
}

void para_num_clear( struct para_num *pn )
{
    if (pn->style)
    {
        ME_ReleaseStyle( pn->style );
        pn->style = NULL;
    }
    ME_DestroyString( pn->text );
    pn->text = NULL;
}

static void para_num_clear_list( ME_TextEditor *editor, ME_Paragraph *para, const PARAFORMAT2 *orig_fmt )
{
    do
    {
        mark_para_rewrap(editor, get_di_from_para(para));
        para_num_clear( &para->para_num );
        if (para->next_para->type != diParagraph) break;
        para = &para->next_para->member.para;
    } while (para_num_same_list( &para->fmt, orig_fmt ));
}

static BOOL ME_SetParaFormat(ME_TextEditor *editor, ME_Paragraph *para, const PARAFORMAT2 *pFmt)
{
  PARAFORMAT2 copy;
  DWORD dwMask;

  assert(para->fmt.cbSize == sizeof(PARAFORMAT2));
  dwMask = pFmt->dwMask;
  if (pFmt->cbSize < sizeof(PARAFORMAT))
    return FALSE;
  else if (pFmt->cbSize < sizeof(PARAFORMAT2))
    dwMask &= PFM_ALL;
  else
    dwMask &= PFM_ALL2;

  add_undo_set_para_fmt( editor, para );

  copy = para->fmt;

#define COPY_FIELD(m, f) \
  if (dwMask & (m)) {                           \
    para->fmt.dwMask |= m;                      \
    para->fmt.f = pFmt->f;                      \
  }

  COPY_FIELD(PFM_NUMBERING, wNumbering);
  COPY_FIELD(PFM_STARTINDENT, dxStartIndent);
  if (dwMask & PFM_OFFSETINDENT)
    para->fmt.dxStartIndent += pFmt->dxStartIndent;
  COPY_FIELD(PFM_RIGHTINDENT, dxRightIndent);
  COPY_FIELD(PFM_OFFSET, dxOffset);
  COPY_FIELD(PFM_ALIGNMENT, wAlignment);
  if (dwMask & PFM_TABSTOPS)
  {
    para->fmt.cTabCount = pFmt->cTabCount;
    memcpy(para->fmt.rgxTabs, pFmt->rgxTabs, pFmt->cTabCount*sizeof(LONG));
  }

#define EFFECTS_MASK (PFM_RTLPARA|PFM_KEEP|PFM_KEEPNEXT|PFM_PAGEBREAKBEFORE| \
                      PFM_NOLINENUMBER|PFM_NOWIDOWCONTROL|PFM_DONOTHYPHEN|PFM_SIDEBYSIDE| \
                      PFM_TABLE)
  /* we take for granted that PFE_xxx is the hiword of the corresponding PFM_xxx */
  if (dwMask & EFFECTS_MASK)
  {
    para->fmt.dwMask |= dwMask & EFFECTS_MASK;
    para->fmt.wEffects &= ~HIWORD(dwMask);
    para->fmt.wEffects |= pFmt->wEffects & HIWORD(dwMask);
  }
#undef EFFECTS_MASK

  COPY_FIELD(PFM_SPACEBEFORE, dySpaceBefore);
  COPY_FIELD(PFM_SPACEAFTER, dySpaceAfter);
  COPY_FIELD(PFM_LINESPACING, dyLineSpacing);
  COPY_FIELD(PFM_STYLE, sStyle);
  COPY_FIELD(PFM_LINESPACING, bLineSpacingRule);
  COPY_FIELD(PFM_SHADING, wShadingWeight);
  COPY_FIELD(PFM_SHADING, wShadingStyle);
  COPY_FIELD(PFM_NUMBERINGSTART, wNumberingStart);
  COPY_FIELD(PFM_NUMBERINGSTYLE, wNumberingStyle);
  COPY_FIELD(PFM_NUMBERINGTAB, wNumberingTab);
  COPY_FIELD(PFM_BORDER, wBorderSpace);
  COPY_FIELD(PFM_BORDER, wBorderWidth);
  COPY_FIELD(PFM_BORDER, wBorders);

  para->fmt.dwMask |= dwMask;
#undef COPY_FIELD

  if (memcmp(&copy, &para->fmt, sizeof(PARAFORMAT2)))
  {
    mark_para_rewrap(editor, get_di_from_para(para));
    if (((dwMask & PFM_NUMBERING)      && (copy.wNumbering != para->fmt.wNumbering)) ||
        ((dwMask & PFM_NUMBERINGSTART) && (copy.wNumberingStart != para->fmt.wNumberingStart)) ||
        ((dwMask & PFM_NUMBERINGSTYLE) && (copy.wNumberingStyle != para->fmt.wNumberingStyle)))
    {
        para_num_clear_list( editor, para, &copy );
    }
  }

  return TRUE;
}

/* split paragraph at the beginning of the run */
ME_DisplayItem *ME_SplitParagraph(ME_TextEditor *editor, ME_DisplayItem *run,
                                  ME_Style *style, const WCHAR *eol_str, int eol_len,
                                  int paraFlags)
{
  ME_DisplayItem *next_para = NULL;
  ME_DisplayItem *run_para = NULL;
  ME_DisplayItem *new_para = make_para(editor);
  ME_DisplayItem *end_run;
  int ofs, i;
  ME_DisplayItem *pp;
  int run_flags = MERF_ENDPARA;

  if (!editor->bEmulateVersion10) { /* v4.1 */
    /* At most 1 of MEPF_CELL, MEPF_ROWSTART, or MEPF_ROWEND should be set. */
    assert(!(paraFlags & ~(MEPF_CELL|MEPF_ROWSTART|MEPF_ROWEND)));
    assert(!(paraFlags & (paraFlags-1)));
    if (paraFlags == MEPF_CELL)
      run_flags |= MERF_ENDCELL;
    else if (paraFlags == MEPF_ROWSTART)
      run_flags |= MERF_TABLESTART|MERF_HIDDEN;
  } else { /* v1.0 - v3.0 */
    assert(!(paraFlags & (MEPF_CELL|MEPF_ROWSTART|MEPF_ROWEND)));
  }
  assert(run->type == diRun);
  run_para = ME_GetParagraph(run);
  assert(run_para->member.para.fmt.cbSize == sizeof(PARAFORMAT2));

  /* Clear any cached para numbering following this paragraph */
  if (run_para->member.para.fmt.wNumbering)
      para_num_clear_list( editor, &run_para->member.para, &run_para->member.para.fmt );

  new_para->member.para.text = ME_VSplitString( run_para->member.para.text, run->member.run.nCharOfs );

  end_run = ME_MakeRun(style, run_flags);
  ofs = end_run->member.run.nCharOfs = run->member.run.nCharOfs;
  end_run->member.run.len = eol_len;
  end_run->member.run.para = run->member.run.para;
  ME_AppendString( run_para->member.para.text, eol_str, eol_len );
  next_para = run_para->member.para.next_para;
  assert(next_para == ME_FindItemFwd(run_para, diParagraphOrEnd));

  add_undo_join_paras( editor, run_para->member.para.nCharOfs + ofs );

  /* Update selection cursors to point to the correct paragraph. */
  for (i = 0; i < editor->nCursors; i++) {
    if (editor->pCursors[i].pPara == run_para &&
        run->member.run.nCharOfs <= editor->pCursors[i].pRun->member.run.nCharOfs)
    {
      editor->pCursors[i].pPara = new_para;
    }
  }

  /* the new paragraph will have a different starting offset, so let's update its runs */
  pp = run;
  while(pp->type == diRun) {
    pp->member.run.nCharOfs -= ofs;
    pp->member.run.para = &new_para->member.para;
    pp = ME_FindItemFwd(pp, diRunOrParagraphOrEnd);
  }
  new_para->member.para.nCharOfs = run_para->member.para.nCharOfs + ofs;
  new_para->member.para.nCharOfs += eol_len;
  new_para->member.para.nFlags = 0;
  mark_para_rewrap(editor, new_para);

  /* FIXME initialize format style and call ME_SetParaFormat blah blah */
  new_para->member.para.fmt = run_para->member.para.fmt;
  new_para->member.para.border = run_para->member.para.border;

  /* insert paragraph into paragraph double linked list */
  new_para->member.para.prev_para = run_para;
  new_para->member.para.next_para = next_para;
  run_para->member.para.next_para = new_para;
  next_para->member.para.prev_para = new_para;

  /* insert end run of the old paragraph, and new paragraph, into DI double linked list */
  ME_InsertBefore(run, new_para);
  ME_InsertBefore(new_para, end_run);

  /* Fix up the paras' eop_run ptrs */
  new_para->member.para.eop_run = run_para->member.para.eop_run;
  run_para->member.para.eop_run = &end_run->member.run;

  if (!editor->bEmulateVersion10) { /* v4.1 */
    if (paraFlags & (MEPF_ROWSTART|MEPF_CELL))
    {
      ME_DisplayItem *cell = ME_MakeDI(diCell);
      ME_InsertBefore(new_para, cell);
      new_para->member.para.pCell = cell;
      cell->member.cell.next_cell = NULL;
      if (paraFlags & MEPF_ROWSTART)
      {
        run_para->member.para.nFlags |= MEPF_ROWSTART;
        cell->member.cell.prev_cell = NULL;
        cell->member.cell.parent_cell = run_para->member.para.pCell;
        if (run_para->member.para.pCell)
          cell->member.cell.nNestingLevel = run_para->member.para.pCell->member.cell.nNestingLevel + 1;
        else
          cell->member.cell.nNestingLevel = 1;
      } else {
        cell->member.cell.prev_cell = run_para->member.para.pCell;
        assert(cell->member.cell.prev_cell);
        cell->member.cell.prev_cell->member.cell.next_cell = cell;
        assert(run_para->member.para.nFlags & MEPF_CELL);
        assert(!(run_para->member.para.nFlags & MEPF_ROWSTART));
        cell->member.cell.nNestingLevel = cell->member.cell.prev_cell->member.cell.nNestingLevel;
        cell->member.cell.parent_cell = cell->member.cell.prev_cell->member.cell.parent_cell;
      }
    } else if (paraFlags & MEPF_ROWEND) {
      run_para->member.para.nFlags |= MEPF_ROWEND;
      run_para->member.para.pCell = run_para->member.para.pCell->member.cell.parent_cell;
      new_para->member.para.pCell = run_para->member.para.pCell;
      assert(run_para->member.para.prev_para->member.para.nFlags & MEPF_CELL);
      assert(!(run_para->member.para.prev_para->member.para.nFlags & MEPF_ROWSTART));
      if (new_para->member.para.pCell != new_para->member.para.next_para->member.para.pCell
          && new_para->member.para.next_para->member.para.pCell
          && !new_para->member.para.next_para->member.para.pCell->member.cell.prev_cell)
      {
        /* Row starts just after the row that was ended. */
        new_para->member.para.nFlags |= MEPF_ROWSTART;
      }
    } else {
      new_para->member.para.pCell = run_para->member.para.pCell;
    }
    ME_UpdateTableFlags(run_para);
    ME_UpdateTableFlags(new_para);
  }

  /* force rewrap of the */
  if (run_para->member.para.prev_para->type == diParagraph)
    mark_para_rewrap(editor, run_para->member.para.prev_para);

  mark_para_rewrap(editor, new_para->member.para.prev_para);

  /* we've added the end run, so we need to modify nCharOfs in the next paragraphs */
  ME_PropagateCharOffset(next_para, eol_len);
  editor->nParagraphs++;

  return new_para;
}

/* join tp with tp->member.para.next_para, keeping tp's style; this
 * is consistent with the original */
ME_DisplayItem *ME_JoinParagraphs(ME_TextEditor *editor, ME_DisplayItem *tp,
                                  BOOL keepFirstParaFormat)
{
  ME_DisplayItem *pNext, *pFirstRunInNext, *pRun, *pTmp, *pCell = NULL;
  int i, shift;
  int end_len;
  CHARFORMAT2W fmt;
  ME_Cursor startCur, endCur;
  ME_String *eol_str;

  assert(tp->type == diParagraph);
  assert(tp->member.para.next_para);
  assert(tp->member.para.next_para->type == diParagraph);

  /* Clear any cached para numbering following this paragraph */
  if (tp->member.para.fmt.wNumbering)
      para_num_clear_list( editor, &tp->member.para, &tp->member.para.fmt );

  pNext = tp->member.para.next_para;

  /* Need to locate end-of-paragraph run here, in order to know end_len */
  pRun = ME_FindItemBack(pNext, diRunOrParagraph);

  assert(pRun);
  assert(pRun->type == diRun);
  assert(pRun->member.run.nFlags & MERF_ENDPARA);

  end_len = pRun->member.run.len;
  eol_str = ME_VSplitString( tp->member.para.text, pRun->member.run.nCharOfs );
  ME_AppendString( tp->member.para.text, pNext->member.para.text->szData, pNext->member.para.text->nLen );

  /* null char format operation to store the original char format for the ENDPARA run */
  ME_InitCharFormat2W(&fmt);
  endCur.pPara = pNext;
  endCur.pRun = ME_FindItemFwd(pNext, diRun);
  endCur.nOffset = 0;
  startCur = endCur;
  ME_PrevRun(&startCur.pPara, &startCur.pRun, TRUE);
  ME_SetCharFormat(editor, &startCur, &endCur, &fmt);

  if (!editor->bEmulateVersion10) { /* v4.1 */
    /* Table cell/row properties are always moved over from the removed para. */
    tp->member.para.nFlags = pNext->member.para.nFlags;
    tp->member.para.pCell = pNext->member.para.pCell;

    /* Remove cell boundary if it is between the end paragraph run and the next
     * paragraph display item. */
    for (pTmp = pRun->next; pTmp != pNext; pTmp = pTmp->next)
    {
      if (pTmp->type == diCell)
      {
        pCell = pTmp;
        break;
      }
    }
  }

  add_undo_split_para( editor, &pNext->member.para, eol_str, pCell ? &pCell->member.cell : NULL );

  if (pCell)
  {
    ME_Remove( pCell );
    if (pCell->member.cell.prev_cell)
      pCell->member.cell.prev_cell->member.cell.next_cell = pCell->member.cell.next_cell;
    if (pCell->member.cell.next_cell)
      pCell->member.cell.next_cell->member.cell.prev_cell = pCell->member.cell.prev_cell;
    ME_DestroyDisplayItem( pCell );
  }

  if (!keepFirstParaFormat)
  {
    add_undo_set_para_fmt( editor, &tp->member.para );
    tp->member.para.fmt = pNext->member.para.fmt;
    tp->member.para.border = pNext->member.para.border;
  }

  shift = pNext->member.para.nCharOfs - tp->member.para.nCharOfs - end_len;

  pFirstRunInNext = ME_FindItemFwd(pNext, diRunOrParagraph);

  assert(pFirstRunInNext->type == diRun);

  /* Update selection cursors so they don't point to the removed end
   * paragraph run, and point to the correct paragraph. */
  for (i=0; i < editor->nCursors; i++) {
    if (editor->pCursors[i].pRun == pRun) {
      editor->pCursors[i].pRun = pFirstRunInNext;
      editor->pCursors[i].nOffset = 0;
    } else if (editor->pCursors[i].pPara == pNext) {
      editor->pCursors[i].pPara = tp;
    }
  }

  pTmp = pNext;
  do {
    pTmp = ME_FindItemFwd(pTmp, diRunOrParagraphOrEnd);
    if (pTmp->type != diRun)
      break;
    TRACE("shifting %s by %d (previous %d)\n", debugstr_run( &pTmp->member.run ), shift, pTmp->member.run.nCharOfs);
    pTmp->member.run.nCharOfs += shift;
    pTmp->member.run.para = &tp->member.para;
  } while(1);

  /* Fix up the para's eop_run ptr */
  tp->member.para.eop_run = pNext->member.para.eop_run;

  ME_Remove(pRun);
  ME_DestroyDisplayItem(pRun);

  if (editor->pLastSelStartPara == pNext)
    editor->pLastSelStartPara = tp;
  if (editor->pLastSelEndPara == pNext)
    editor->pLastSelEndPara = tp;

  tp->member.para.next_para = pNext->member.para.next_para;
  pNext->member.para.next_para->member.para.prev_para = tp;
  ME_Remove(pNext);
  destroy_para(editor, pNext);

  ME_PropagateCharOffset(tp->member.para.next_para, -end_len);

  ME_CheckCharOffsets(editor);

  editor->nParagraphs--;
  mark_para_rewrap(editor, tp);
  return tp;
}

ME_DisplayItem *ME_GetParagraph(ME_DisplayItem *item) {
  return ME_FindItemBackOrHere(item, diParagraph);
}

void ME_DumpParaStyleToBuf(const PARAFORMAT2 *pFmt, char buf[2048])
{
  char *p;
  p = buf;

#define DUMP(mask, name, fmt, field) \
  if (pFmt->dwMask & (mask)) p += sprintf(p, "%-22s" fmt "\n", name, pFmt->field); \
  else p += sprintf(p, "%-22sN/A\n", name);

/* we take for granted that PFE_xxx is the hiword of the corresponding PFM_xxx */
#define DUMP_EFFECT(mask, name) \
  p += sprintf(p, "%-22s%s\n", name, (pFmt->dwMask & (mask)) ? ((pFmt->wEffects & ((mask) >> 16)) ? "yes" : "no") : "N/A");

  DUMP(PFM_NUMBERING,      "Numbering:",         "%u", wNumbering);
  DUMP_EFFECT(PFM_DONOTHYPHEN,     "Disable auto-hyphen:");
  DUMP_EFFECT(PFM_KEEP,            "No page break in para:");
  DUMP_EFFECT(PFM_KEEPNEXT,        "No page break in para & next:");
  DUMP_EFFECT(PFM_NOLINENUMBER,    "No line number:");
  DUMP_EFFECT(PFM_NOWIDOWCONTROL,  "No widow & orphan:");
  DUMP_EFFECT(PFM_PAGEBREAKBEFORE, "Page break before:");
  DUMP_EFFECT(PFM_RTLPARA,         "RTL para:");
  DUMP_EFFECT(PFM_SIDEBYSIDE,      "Side by side:");
  DUMP_EFFECT(PFM_TABLE,           "Table:");
  DUMP(PFM_OFFSETINDENT,   "Offset indent:",     "%d", dxStartIndent);
  DUMP(PFM_STARTINDENT,    "Start indent:",      "%d", dxStartIndent);
  DUMP(PFM_RIGHTINDENT,    "Right indent:",      "%d", dxRightIndent);
  DUMP(PFM_OFFSET,         "Offset:",            "%d", dxOffset);
  if (pFmt->dwMask & PFM_ALIGNMENT) {
    switch (pFmt->wAlignment) {
    case PFA_LEFT   : p += sprintf(p, "Alignment:            left\n"); break;
    case PFA_RIGHT  : p += sprintf(p, "Alignment:            right\n"); break;
    case PFA_CENTER : p += sprintf(p, "Alignment:            center\n"); break;
    case PFA_JUSTIFY: p += sprintf(p, "Alignment:            justify\n"); break;
    default         : p += sprintf(p, "Alignment:            incorrect %d\n", pFmt->wAlignment); break;
    }
  }
  else p += sprintf(p, "Alignment:            N/A\n");
  DUMP(PFM_TABSTOPS,       "Tab Stops:",         "%d", cTabCount);
  if (pFmt->dwMask & PFM_TABSTOPS) {
    int i;
    p += sprintf(p, "\t");
    for (i = 0; i < pFmt->cTabCount; i++) p += sprintf(p, "%x ", pFmt->rgxTabs[i]);
    p += sprintf(p, "\n");
  }
  DUMP(PFM_SPACEBEFORE,    "Space Before:",      "%d", dySpaceBefore);
  DUMP(PFM_SPACEAFTER,     "Space After:",       "%d", dySpaceAfter);
  DUMP(PFM_LINESPACING,    "Line spacing:",      "%d", dyLineSpacing);
  DUMP(PFM_STYLE,          "Text style:",        "%d", sStyle);
  DUMP(PFM_LINESPACING,    "Line spacing rule:", "%u", bLineSpacingRule);
  /* bOutlineLevel should be 0 */
  DUMP(PFM_SHADING,        "Shading Weight:",    "%u", wShadingWeight);
  DUMP(PFM_SHADING,        "Shading Style:",     "%u", wShadingStyle);
  DUMP(PFM_NUMBERINGSTART, "Numbering Start:",   "%u", wNumberingStart);
  DUMP(PFM_NUMBERINGSTYLE, "Numbering Style:",   "0x%x", wNumberingStyle);
  DUMP(PFM_NUMBERINGTAB,   "Numbering Tab:",     "%u", wNumberingStyle);
  DUMP(PFM_BORDER,         "Border Space:",      "%u", wBorderSpace);
  DUMP(PFM_BORDER,         "Border Width:",      "%u", wBorderWidth);
  DUMP(PFM_BORDER,         "Borders:",           "%u", wBorders);

#undef DUMP
#undef DUMP_EFFECT
}

void
ME_GetSelectionParas(ME_TextEditor *editor, ME_DisplayItem **para, ME_DisplayItem **para_end)
{
  ME_Cursor *pEndCursor = &editor->pCursors[1];

  *para = editor->pCursors[0].pPara;
  *para_end = editor->pCursors[1].pPara;
  if (*para == *para_end)
    return;

  if ((*para_end)->member.para.nCharOfs < (*para)->member.para.nCharOfs) {
    ME_DisplayItem *tmp = *para;

    *para = *para_end;
    *para_end = tmp;
    pEndCursor = &editor->pCursors[0];
  }

  /* The paragraph at the end of a non-empty selection isn't included
   * if the selection ends at the start of the paragraph. */
  if (!pEndCursor->pRun->member.run.nCharOfs && !pEndCursor->nOffset)
    *para_end = (*para_end)->member.para.prev_para;
}


BOOL ME_SetSelectionParaFormat(ME_TextEditor *editor, const PARAFORMAT2 *pFmt)
{
  ME_DisplayItem *para, *para_end;

  ME_GetSelectionParas(editor, &para, &para_end);

  do {
    ME_SetParaFormat(editor, &para->member.para, pFmt);
    if (para == para_end)
      break;
    para = para->member.para.next_para;
  } while(1);

  return TRUE;
}

static void ME_GetParaFormat(ME_TextEditor *editor,
                             const ME_DisplayItem *para,
                             PARAFORMAT2 *pFmt)
{
  UINT cbSize = pFmt->cbSize;
  if (pFmt->cbSize >= sizeof(PARAFORMAT2)) {
    *pFmt = para->member.para.fmt;
  } else {
    CopyMemory(pFmt, &para->member.para.fmt, pFmt->cbSize);
    pFmt->dwMask &= PFM_ALL;
  }
  pFmt->cbSize = cbSize;
}

void ME_GetSelectionParaFormat(ME_TextEditor *editor, PARAFORMAT2 *pFmt)
{
  ME_DisplayItem *para, *para_end;
  PARAFORMAT2 *curFmt;

  if (pFmt->cbSize < sizeof(PARAFORMAT)) {
    pFmt->dwMask = 0;
    return;
  }

  ME_GetSelectionParas(editor, &para, &para_end);

  ME_GetParaFormat(editor, para, pFmt);

  /* Invalidate values that change across the selected paragraphs. */
  while (para != para_end)
  {
    para = para->member.para.next_para;
    curFmt = &para->member.para.fmt;

#define CHECK_FIELD(m, f) \
    if (pFmt->f != curFmt->f) pFmt->dwMask &= ~(m);

    CHECK_FIELD(PFM_NUMBERING, wNumbering);
    CHECK_FIELD(PFM_STARTINDENT, dxStartIndent);
    CHECK_FIELD(PFM_RIGHTINDENT, dxRightIndent);
    CHECK_FIELD(PFM_OFFSET, dxOffset);
    CHECK_FIELD(PFM_ALIGNMENT, wAlignment);
    if (pFmt->dwMask & PFM_TABSTOPS) {
      if (pFmt->cTabCount != para->member.para.fmt.cTabCount ||
          memcmp(pFmt->rgxTabs, curFmt->rgxTabs, curFmt->cTabCount*sizeof(int)))
        pFmt->dwMask &= ~PFM_TABSTOPS;
    }

    if (pFmt->dwMask >= sizeof(PARAFORMAT2))
    {
      pFmt->dwMask &= ~((pFmt->wEffects ^ curFmt->wEffects) << 16);
      CHECK_FIELD(PFM_SPACEBEFORE, dySpaceBefore);
      CHECK_FIELD(PFM_SPACEAFTER, dySpaceAfter);
      CHECK_FIELD(PFM_LINESPACING, dyLineSpacing);
      CHECK_FIELD(PFM_STYLE, sStyle);
      CHECK_FIELD(PFM_SPACEAFTER, bLineSpacingRule);
      CHECK_FIELD(PFM_SHADING, wShadingWeight);
      CHECK_FIELD(PFM_SHADING, wShadingStyle);
      CHECK_FIELD(PFM_NUMBERINGSTART, wNumberingStart);
      CHECK_FIELD(PFM_NUMBERINGSTYLE, wNumberingStyle);
      CHECK_FIELD(PFM_NUMBERINGTAB, wNumberingTab);
      CHECK_FIELD(PFM_BORDER, wBorderSpace);
      CHECK_FIELD(PFM_BORDER, wBorderWidth);
      CHECK_FIELD(PFM_BORDER, wBorders);
    }
#undef CHECK_FIELD
  }
}

void ME_SetDefaultParaFormat(ME_TextEditor *editor, PARAFORMAT2 *pFmt)
{
    const PARAFORMAT2 *host_fmt;
    HRESULT hr;

    ZeroMemory(pFmt, sizeof(PARAFORMAT2));
    pFmt->cbSize = sizeof(PARAFORMAT2);
    pFmt->dwMask = PFM_ALL2;
    pFmt->wAlignment = PFA_LEFT;
    pFmt->sStyle = -1;
    pFmt->bOutlineLevel = TRUE;

    hr = ITextHost_TxGetParaFormat( editor->texthost, (const PARAFORMAT **)&host_fmt );
    if (SUCCEEDED(hr))
    {
        /* Just use the alignment for now */
        if (host_fmt->dwMask & PFM_ALIGNMENT)
            pFmt->wAlignment = host_fmt->wAlignment;
        ITextHost_OnTxParaFormatChange( editor->texthost, (PARAFORMAT *)pFmt );
    }
}
