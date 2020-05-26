/*
 * RichEdit - RTF writer module
 *
 * Copyright 2005 by Phil Krylov
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

#define NONAMELESSUNION

#include "editor.h"
#include "rtf.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

#define STREAMOUT_BUFFER_SIZE 4096
#define STREAMOUT_FONTTBL_SIZE 8192
#define STREAMOUT_COLORTBL_SIZE 1024

typedef struct tagME_OutStream
{
    EDITSTREAM *stream;
    char buffer[STREAMOUT_BUFFER_SIZE];
    UINT pos, written;
    UINT nCodePage;
    UINT nFontTblLen;
    ME_FontTableItem fonttbl[STREAMOUT_FONTTBL_SIZE];
    UINT nColorTblLen;
    COLORREF colortbl[STREAMOUT_COLORTBL_SIZE];
    UINT nDefaultFont;
    UINT nDefaultCodePage;
    /* nNestingLevel = 0 means we aren't in a cell, 1 means we are in a cell,
     * an greater numbers mean we are in a cell nested within a cell. */
    UINT nNestingLevel;
    CHARFORMAT2W cur_fmt; /* current character format */
} ME_OutStream;

static BOOL
ME_StreamOutRTFText(ME_OutStream *pStream, const WCHAR *text, LONG nChars);


static ME_OutStream*
ME_StreamOutInit(ME_TextEditor *editor, EDITSTREAM *stream)
{
  ME_OutStream *pStream = heap_alloc_zero(sizeof(*pStream));

  pStream->stream = stream;
  pStream->stream->dwError = 0;
  pStream->nColorTblLen = 1;
  pStream->cur_fmt.dwEffects = CFE_AUTOCOLOR | CFE_AUTOBACKCOLOR;
  pStream->cur_fmt.bUnderlineType = CFU_UNDERLINE;
  return pStream;
}


static BOOL
ME_StreamOutFlush(ME_OutStream *pStream)
{
  LONG nWritten = 0;
  EDITSTREAM *stream = pStream->stream;

  if (pStream->pos) {
    TRACE("sending %u bytes\n", pStream->pos);
    nWritten = pStream->pos;
    stream->dwError = stream->pfnCallback(stream->dwCookie, (LPBYTE)pStream->buffer,
                                          pStream->pos, &nWritten);
    TRACE("error=%u written=%u\n", stream->dwError, nWritten);
    if (nWritten == 0 || stream->dwError)
      return FALSE;
    /* Don't resend partial chunks if nWritten < pStream->pos */
  }
  if (nWritten == pStream->pos)
      pStream->written += nWritten;
  pStream->pos = 0;
  return TRUE;
}


static LONG
ME_StreamOutFree(ME_OutStream *pStream)
{
  LONG written = pStream->written;
  TRACE("total length = %u\n", written);

  heap_free(pStream);
  return written;
}


static BOOL
ME_StreamOutMove(ME_OutStream *pStream, const char *buffer, int len)
{
  while (len) {
    int space = STREAMOUT_BUFFER_SIZE - pStream->pos;
    int fit = min(space, len);

    TRACE("%u:%u:%s\n", pStream->pos, fit, debugstr_an(buffer,fit));
    memmove(pStream->buffer + pStream->pos, buffer, fit);
    len -= fit;
    buffer += fit;
    pStream->pos += fit;
    if (pStream->pos == STREAMOUT_BUFFER_SIZE) {
      if (!ME_StreamOutFlush(pStream))
        return FALSE;
    }
  }
  return TRUE;
}


static BOOL WINAPIV
ME_StreamOutPrint(ME_OutStream *pStream, const char *format, ...)
{
  char string[STREAMOUT_BUFFER_SIZE]; /* This is going to be enough */
  int len;
  __ms_va_list valist;

  __ms_va_start(valist, format);
  len = vsnprintf(string, sizeof(string), format, valist);
  __ms_va_end(valist);

  return ME_StreamOutMove(pStream, string, len);
}

#define HEX_BYTES_PER_LINE 40

static BOOL
ME_StreamOutHexData(ME_OutStream *stream, const BYTE *data, UINT len)
{

    char line[HEX_BYTES_PER_LINE * 2 + 1];
    UINT size, i;
    static const char hex[] = "0123456789abcdef";

    while (len)
    {
        size = min( len, HEX_BYTES_PER_LINE );
        for (i = 0; i < size; i++)
        {
            line[i * 2] = hex[(*data >> 4) & 0xf];
            line[i * 2 + 1] = hex[*data & 0xf];
            data++;
        }
        line[size * 2] = '\n';
        if (!ME_StreamOutMove( stream, line, size * 2 + 1 ))
            return FALSE;
        len -= size;
    }
    return TRUE;
}

static BOOL
ME_StreamOutRTFHeader(ME_OutStream *pStream, int dwFormat)
{
  const char *cCharSet = NULL;
  UINT nCodePage;
  LANGID language;
  BOOL success;
  
  if (dwFormat & SF_USECODEPAGE) {
    CPINFOEXW info;
    
    switch (HIWORD(dwFormat)) {
      case CP_ACP:
        cCharSet = "ansi";
        nCodePage = GetACP();
        break;
      case CP_OEMCP:
        nCodePage = GetOEMCP();
        if (nCodePage == 437)
          cCharSet = "pc";
        else if (nCodePage == 850)
          cCharSet = "pca";
        else
          cCharSet = "ansi";
        break;
      case CP_UTF8:
        nCodePage = CP_UTF8;
        break;
      default:
        if (HIWORD(dwFormat) == CP_MACCP) {
          cCharSet = "mac";
          nCodePage = 10000; /* MacRoman */
        } else {
          cCharSet = "ansi";
          nCodePage = 1252; /* Latin-1 */
        }
        if (GetCPInfoExW(HIWORD(dwFormat), 0, &info))
          nCodePage = info.CodePage;
    }
  } else {
    cCharSet = "ansi";
    /* TODO: If the original document contained an \ansicpg value, retain it.
     * Otherwise, M$ richedit emits a codepage number determined from the
     * charset of the default font here. Anyway, this value is not used by
     * the reader... */
    nCodePage = GetACP();
  }
  if (nCodePage == CP_UTF8)
    success = ME_StreamOutPrint(pStream, "{\\urtf");
  else
    success = ME_StreamOutPrint(pStream, "{\\rtf1\\%s\\ansicpg%u\\uc1", cCharSet, nCodePage);

  if (!success)
    return FALSE;

  pStream->nDefaultCodePage = nCodePage;
  
  /* FIXME: This should be a document property */
  /* TODO: handle SFF_PLAINRTF */
  language = GetUserDefaultLangID(); 
  if (!ME_StreamOutPrint(pStream, "\\deff0\\deflang%u\\deflangfe%u", language, language))
    return FALSE;

  /* FIXME: This should be a document property */
  pStream->nDefaultFont = 0;

  return TRUE;
}

static void add_font_to_fonttbl( ME_OutStream *stream, ME_Style *style )
{
    ME_FontTableItem *table = stream->fonttbl;
    CHARFORMAT2W *fmt = &style->fmt;
    WCHAR *face = fmt->szFaceName;
    BYTE charset = (fmt->dwMask & CFM_CHARSET) ? fmt->bCharSet : DEFAULT_CHARSET;
    int i;

    if (fmt->dwMask & CFM_FACE)
    {
        for (i = 0; i < stream->nFontTblLen; i++)
            if (table[i].bCharSet == charset
                && (table[i].szFaceName == face || !wcscmp(table[i].szFaceName, face)))
                break;

        if (i == stream->nFontTblLen && i < STREAMOUT_FONTTBL_SIZE)
        {
            table[i].bCharSet = charset;
            table[i].szFaceName = face;
            stream->nFontTblLen++;
        }
    }
}

static BOOL find_font_in_fonttbl( ME_OutStream *stream, CHARFORMAT2W *fmt, unsigned int *idx )
{
    WCHAR *facename;
    int i;

    *idx = 0;
    if (fmt->dwMask & CFM_FACE)
        facename = fmt->szFaceName;
    else
        facename = stream->fonttbl[0].szFaceName;
    for (i = 0; i < stream->nFontTblLen; i++)
    {
        if (facename == stream->fonttbl[i].szFaceName
            || !wcscmp(facename, stream->fonttbl[i].szFaceName))
            if (!(fmt->dwMask & CFM_CHARSET)
                || fmt->bCharSet == stream->fonttbl[i].bCharSet)
            {
                *idx = i;
                break;
            }
    }

    return i < stream->nFontTblLen;
}

static void add_color_to_colortbl( ME_OutStream *stream, COLORREF color )
{
    int i;

    for (i = 1; i < stream->nColorTblLen; i++)
        if (stream->colortbl[i] == color)
            break;

    if (i == stream->nColorTblLen && i < STREAMOUT_COLORTBL_SIZE)
    {
        stream->colortbl[i] = color;
        stream->nColorTblLen++;
    }
}

static BOOL find_color_in_colortbl( ME_OutStream *stream, COLORREF color, unsigned int *idx )
{
    int i;

    *idx = 0;
    for (i = 1; i < stream->nColorTblLen; i++)
    {
        if (stream->colortbl[i] == color)
        {
            *idx = i;
            break;
        }
    }

    return i < stream->nFontTblLen;
}

static BOOL
ME_StreamOutRTFFontAndColorTbl(ME_OutStream *pStream, ME_DisplayItem *pFirstRun,
                               ME_DisplayItem *pLastRun)
{
  ME_DisplayItem *item = pFirstRun;
  ME_FontTableItem *table = pStream->fonttbl;
  unsigned int i;
  ME_DisplayItem *pCell = NULL;
  ME_Paragraph *prev_para = NULL;

  do {
    CHARFORMAT2W *fmt = &item->member.run.style->fmt;

    add_font_to_fonttbl( pStream, item->member.run.style );

    if (fmt->dwMask & CFM_COLOR && !(fmt->dwEffects & CFE_AUTOCOLOR))
      add_color_to_colortbl( pStream, fmt->crTextColor );
    if (fmt->dwMask & CFM_BACKCOLOR && !(fmt->dwEffects & CFE_AUTOBACKCOLOR))
      add_color_to_colortbl( pStream, fmt->crBackColor );

    if (item->member.run.para != prev_para)
    {
      /* check for any para numbering text */
      if (item->member.run.para->fmt.wNumbering)
        add_font_to_fonttbl( pStream, item->member.run.para->para_num.style );

      if ((pCell = item->member.para.pCell))
      {
        ME_Border* borders[4] = { &pCell->member.cell.border.top,
                                  &pCell->member.cell.border.left,
                                  &pCell->member.cell.border.bottom,
                                  &pCell->member.cell.border.right };
        for (i = 0; i < 4; i++)
          if (borders[i]->width > 0)
            add_color_to_colortbl( pStream, borders[i]->colorRef );
      }

      prev_para = item->member.run.para;
    }

    if (item == pLastRun)
      break;
    item = ME_FindItemFwd(item, diRun);
  } while (item);

  if (!ME_StreamOutPrint(pStream, "{\\fonttbl"))
    return FALSE;
  
  for (i = 0; i < pStream->nFontTblLen; i++) {
    if (table[i].bCharSet != DEFAULT_CHARSET) {
      if (!ME_StreamOutPrint(pStream, "{\\f%u\\fcharset%u ", i, table[i].bCharSet))
        return FALSE;
    } else {
      if (!ME_StreamOutPrint(pStream, "{\\f%u ", i))
        return FALSE;
    }
    if (!ME_StreamOutRTFText(pStream, table[i].szFaceName, -1))
      return FALSE;
    if (!ME_StreamOutPrint(pStream, ";}"))
      return FALSE;
  }
  if (!ME_StreamOutPrint(pStream, "}\r\n"))
    return FALSE;

  /* Output the color table */
  if (!ME_StreamOutPrint(pStream, "{\\colortbl;")) return FALSE; /* first entry is auto-color */
  for (i = 1; i < pStream->nColorTblLen; i++)
  {
    if (!ME_StreamOutPrint(pStream, "\\red%u\\green%u\\blue%u;", pStream->colortbl[i] & 0xFF,
                           (pStream->colortbl[i] >> 8) & 0xFF, (pStream->colortbl[i] >> 16) & 0xFF))
      return FALSE;
  }
  if (!ME_StreamOutPrint(pStream, "}\r\n")) return FALSE;

  return TRUE;
}

static BOOL
ME_StreamOutRTFTableProps(ME_TextEditor *editor, ME_OutStream *pStream,
                          ME_DisplayItem *para)
{
  ME_DisplayItem *cell;
  char props[STREAMOUT_BUFFER_SIZE] = "";
  int i;
  const char sideChar[4] = {'t','l','b','r'};

  if (!ME_StreamOutPrint(pStream, "\\trowd"))
    return FALSE;
  if (!editor->bEmulateVersion10) { /* v4.1 */
    PARAFORMAT2 *pFmt = &ME_GetTableRowEnd(para)->member.para.fmt;
    para = ME_GetTableRowStart(para);
    cell = para->member.para.next_para->member.para.pCell;
    assert(cell);
    if (pFmt->dxOffset)
      sprintf(props + strlen(props), "\\trgaph%d", pFmt->dxOffset);
    if (pFmt->dxStartIndent)
      sprintf(props + strlen(props), "\\trleft%d", pFmt->dxStartIndent);
    do {
      ME_Border* borders[4] = { &cell->member.cell.border.top,
                                &cell->member.cell.border.left,
                                &cell->member.cell.border.bottom,
                                &cell->member.cell.border.right };
      for (i = 0; i < 4; i++)
      {
        if (borders[i]->width)
        {
          unsigned int idx;
          COLORREF crColor = borders[i]->colorRef;
          sprintf(props + strlen(props), "\\clbrdr%c", sideChar[i]);
          sprintf(props + strlen(props), "\\brdrs");
          sprintf(props + strlen(props), "\\brdrw%d", borders[i]->width);
          if (find_color_in_colortbl( pStream, crColor, &idx ))
            sprintf(props + strlen(props), "\\brdrcf%u", idx);
        }
      }
      sprintf(props + strlen(props), "\\cellx%d", cell->member.cell.nRightBoundary);
      cell = cell->member.cell.next_cell;
    } while (cell->member.cell.next_cell);
  } else { /* v1.0 - 3.0 */
    const ME_Border* borders[4] = { &para->member.para.border.top,
                                    &para->member.para.border.left,
                                    &para->member.para.border.bottom,
                                    &para->member.para.border.right };
    PARAFORMAT2 *pFmt = &para->member.para.fmt;

    assert(!(para->member.para.nFlags & (MEPF_ROWSTART|MEPF_ROWEND|MEPF_CELL)));
    if (pFmt->dxOffset)
      sprintf(props + strlen(props), "\\trgaph%d", pFmt->dxOffset);
    if (pFmt->dxStartIndent)
      sprintf(props + strlen(props), "\\trleft%d", pFmt->dxStartIndent);
    for (i = 0; i < 4; i++)
    {
      if (borders[i]->width)
      {
        unsigned int idx;
        COLORREF crColor = borders[i]->colorRef;
        sprintf(props + strlen(props), "\\trbrdr%c", sideChar[i]);
        sprintf(props + strlen(props), "\\brdrs");
        sprintf(props + strlen(props), "\\brdrw%d", borders[i]->width);
        if (find_color_in_colortbl( pStream, crColor, &idx ))
          sprintf(props + strlen(props), "\\brdrcf%u", idx);
      }
    }
    for (i = 0; i < pFmt->cTabCount; i++)
    {
      sprintf(props + strlen(props), "\\cellx%d", pFmt->rgxTabs[i] & 0x00FFFFFF);
    }
  }
  if (!ME_StreamOutPrint(pStream, props))
    return FALSE;
  props[0] = '\0';
  return TRUE;
}

static BOOL stream_out_para_num( ME_OutStream *stream, ME_Paragraph *para, BOOL pn_dest )
{
    static const char fmt_label[] = "{\\*\\pn\\pnlvlbody\\pnf%u\\pnindent%d\\pnstart%d%s%s}";
    static const char fmt_bullet[] = "{\\*\\pn\\pnlvlblt\\pnf%u\\pnindent%d{\\pntxtb\\'b7}}";
    static const char dec[] = "\\pndec";
    static const char lcltr[] = "\\pnlcltr";
    static const char ucltr[] = "\\pnucltr";
    static const char lcrm[] = "\\pnlcrm";
    static const char ucrm[] = "\\pnucrm";
    static const char period[] = "{\\pntxta.}";
    static const char paren[] = "{\\pntxta)}";
    static const char parens[] = "{\\pntxtb(}{\\pntxta)}";
    const char *type, *style = "";
    unsigned int idx;

    find_font_in_fonttbl( stream, &para->para_num.style->fmt, &idx );

    if (!ME_StreamOutPrint( stream, "{\\pntext\\f%u ", idx )) return FALSE;
    if (!ME_StreamOutRTFText( stream, para->para_num.text->szData, para->para_num.text->nLen ))
        return FALSE;
    if (!ME_StreamOutPrint( stream, "\\tab}" )) return FALSE;

    if (!pn_dest) return TRUE;

    if (para->fmt.wNumbering == PFN_BULLET)
    {
        if (!ME_StreamOutPrint( stream, fmt_bullet, idx, para->fmt.wNumberingTab ))
            return FALSE;
    }
    else
    {
        switch (para->fmt.wNumbering)
        {
        case PFN_ARABIC:
        default:
            type = dec;
            break;
        case PFN_LCLETTER:
            type = lcltr;
            break;
        case PFN_UCLETTER:
            type = ucltr;
            break;
        case PFN_LCROMAN:
            type = lcrm;
            break;
        case PFN_UCROMAN:
            type = ucrm;
            break;
        }
        switch (para->fmt.wNumberingStyle & 0xf00)
        {
        case PFNS_PERIOD:
            style = period;
            break;
        case PFNS_PAREN:
            style = paren;
            break;
        case PFNS_PARENS:
            style = parens;
            break;
        }

        if (!ME_StreamOutPrint( stream, fmt_label, idx, para->fmt.wNumberingTab,
                                para->fmt.wNumberingStart, type, style ))
            return FALSE;
    }
    return TRUE;
}

static BOOL
ME_StreamOutRTFParaProps(ME_TextEditor *editor, ME_OutStream *pStream,
                         ME_DisplayItem *para)
{
  PARAFORMAT2 *fmt = &para->member.para.fmt;
  char props[STREAMOUT_BUFFER_SIZE] = "";
  int i;
  ME_Paragraph *prev_para = NULL;

  if (para->member.para.prev_para->type == diParagraph)
      prev_para = &para->member.para.prev_para->member.para;

  if (!editor->bEmulateVersion10) { /* v4.1 */
    if (para->member.para.nFlags & MEPF_ROWSTART) {
      pStream->nNestingLevel++;
      if (pStream->nNestingLevel == 1) {
        if (!ME_StreamOutRTFTableProps(editor, pStream, para))
          return FALSE;
      }
      return TRUE;
    } else if (para->member.para.nFlags & MEPF_ROWEND) {
      pStream->nNestingLevel--;
      if (pStream->nNestingLevel >= 1) {
        if (!ME_StreamOutPrint(pStream, "{\\*\\nesttableprops"))
          return FALSE;
        if (!ME_StreamOutRTFTableProps(editor, pStream, para))
          return FALSE;
        if (!ME_StreamOutPrint(pStream, "\\nestrow}{\\nonesttables\\par}\r\n"))
          return FALSE;
      } else {
        if (!ME_StreamOutPrint(pStream, "\\row\r\n"))
          return FALSE;
      }
      return TRUE;
    }
  } else { /* v1.0 - 3.0 */
    if (para->member.para.fmt.dwMask & PFM_TABLE &&
        para->member.para.fmt.wEffects & PFE_TABLE)
    {
      if (!ME_StreamOutRTFTableProps(editor, pStream, para))
        return FALSE;
    }
  }

  if (prev_para && !memcmp( fmt, &prev_para->fmt, sizeof(*fmt) ))
  {
    if (fmt->wNumbering)
      return stream_out_para_num( pStream, &para->member.para, FALSE );
    return TRUE;
  }

  if (!ME_StreamOutPrint(pStream, "\\pard"))
    return FALSE;

  if (fmt->wNumbering)
    if (!stream_out_para_num( pStream, &para->member.para, TRUE )) return FALSE;

  if (!editor->bEmulateVersion10) { /* v4.1 */
    if (pStream->nNestingLevel > 0)
      strcat(props, "\\intbl");
    if (pStream->nNestingLevel > 1)
      sprintf(props + strlen(props), "\\itap%d", pStream->nNestingLevel);
  } else { /* v1.0 - 3.0 */
    if (fmt->dwMask & PFM_TABLE && fmt->wEffects & PFE_TABLE)
      strcat(props, "\\intbl");
  }

  /* TODO: PFM_BORDER. M$ does not emit any keywords for these properties, and
   * when streaming border keywords in, PFM_BORDER is set, but wBorder field is
   * set very different from the documentation.
   * (Tested with RichEdit 5.50.25.0601) */
  
  if (fmt->dwMask & PFM_ALIGNMENT) {
    switch (fmt->wAlignment) {
      case PFA_LEFT:
        /* Default alignment: not emitted */
        break;
      case PFA_RIGHT:
        strcat(props, "\\qr");
        break;
      case PFA_CENTER:
        strcat(props, "\\qc");
        break;
      case PFA_JUSTIFY:
        strcat(props, "\\qj");
        break;
    }
  }
  
  if (fmt->dwMask & PFM_LINESPACING) {
    /* FIXME: MSDN says that the bLineSpacingRule field is controlled by the
     * PFM_SPACEAFTER flag. Is that true? I don't believe so. */
    switch (fmt->bLineSpacingRule) {
      case 0: /* Single spacing */
        strcat(props, "\\sl-240\\slmult1");
        break;
      case 1: /* 1.5 spacing */
        strcat(props, "\\sl-360\\slmult1");
        break;
      case 2: /* Double spacing */
        strcat(props, "\\sl-480\\slmult1");
        break;
      case 3:
        sprintf(props + strlen(props), "\\sl%d\\slmult0", fmt->dyLineSpacing);
        break;
      case 4:
        sprintf(props + strlen(props), "\\sl-%d\\slmult0", fmt->dyLineSpacing);
        break;
      case 5:
        sprintf(props + strlen(props), "\\sl-%d\\slmult1", fmt->dyLineSpacing * 240 / 20);
        break;
    }
  }

  if (fmt->dwMask & PFM_DONOTHYPHEN && fmt->wEffects & PFE_DONOTHYPHEN)
    strcat(props, "\\hyph0");
  if (fmt->dwMask & PFM_KEEP && fmt->wEffects & PFE_KEEP)
    strcat(props, "\\keep");
  if (fmt->dwMask & PFM_KEEPNEXT && fmt->wEffects & PFE_KEEPNEXT)
    strcat(props, "\\keepn");
  if (fmt->dwMask & PFM_NOLINENUMBER && fmt->wEffects & PFE_NOLINENUMBER)
    strcat(props, "\\noline");
  if (fmt->dwMask & PFM_NOWIDOWCONTROL && fmt->wEffects & PFE_NOWIDOWCONTROL)
    strcat(props, "\\nowidctlpar");
  if (fmt->dwMask & PFM_PAGEBREAKBEFORE && fmt->wEffects & PFE_PAGEBREAKBEFORE)
    strcat(props, "\\pagebb");
  if (fmt->dwMask & PFM_RTLPARA && fmt->wEffects & PFE_RTLPARA)
    strcat(props, "\\rtlpar");
  if (fmt->dwMask & PFM_SIDEBYSIDE && fmt->wEffects & PFE_SIDEBYSIDE)
    strcat(props, "\\sbys");
  
  if (!(editor->bEmulateVersion10 && /* v1.0 - 3.0 */
        fmt->dwMask & PFM_TABLE && fmt->wEffects & PFE_TABLE))
  {
    if (fmt->dxOffset)
      sprintf(props + strlen(props), "\\li%d", fmt->dxOffset);
    if (fmt->dxStartIndent)
      sprintf(props + strlen(props), "\\fi%d", fmt->dxStartIndent);
    if (fmt->dxRightIndent)
      sprintf(props + strlen(props), "\\ri%d", fmt->dxRightIndent);
    if (fmt->dwMask & PFM_TABSTOPS) {
      static const char * const leader[6] = { "", "\\tldot", "\\tlhyph", "\\tlul", "\\tlth", "\\tleq" };

      for (i = 0; i < fmt->cTabCount; i++) {
        switch ((fmt->rgxTabs[i] >> 24) & 0xF) {
          case 1:
            strcat(props, "\\tqc");
            break;
          case 2:
            strcat(props, "\\tqr");
            break;
          case 3:
            strcat(props, "\\tqdec");
            break;
          case 4:
            /* Word bar tab (vertical bar). Handled below */
            break;
        }
        if (fmt->rgxTabs[i] >> 28 <= 5)
          strcat(props, leader[fmt->rgxTabs[i] >> 28]);
        sprintf(props+strlen(props), "\\tx%d", fmt->rgxTabs[i]&0x00FFFFFF);
      }
    }
  }
  if (fmt->dySpaceAfter)
    sprintf(props + strlen(props), "\\sa%d", fmt->dySpaceAfter);
  if (fmt->dySpaceBefore)
    sprintf(props + strlen(props), "\\sb%d", fmt->dySpaceBefore);
  if (fmt->sStyle != -1)
    sprintf(props + strlen(props), "\\s%d", fmt->sStyle);
  
  if (fmt->dwMask & PFM_SHADING) {
    static const char * const style[16] = { "", "\\bgdkhoriz", "\\bgdkvert", "\\bgdkfdiag",
                                     "\\bgdkbdiag", "\\bgdkcross", "\\bgdkdcross",
                                     "\\bghoriz", "\\bgvert", "\\bgfdiag",
                                     "\\bgbdiag", "\\bgcross", "\\bgdcross",
                                     "", "", "" };
    if (fmt->wShadingWeight)
      sprintf(props + strlen(props), "\\shading%d", fmt->wShadingWeight);
    if (fmt->wShadingStyle & 0xF)
      strcat(props, style[fmt->wShadingStyle & 0xF]);
    if ((fmt->wShadingStyle >> 4) & 0xf)
      sprintf(props + strlen(props), "\\cfpat%d", (fmt->wShadingStyle >> 4) & 0xf);
    if ((fmt->wShadingStyle >> 8) & 0xf)
      sprintf(props + strlen(props), "\\cbpat%d", (fmt->wShadingStyle >> 8) & 0xf);
  }
  if (*props)
    strcat(props, " ");
  
  if (*props && !ME_StreamOutPrint(pStream, props))
    return FALSE;

  return TRUE;
}


static BOOL
ME_StreamOutRTFCharProps(ME_OutStream *pStream, CHARFORMAT2W *fmt)
{
  char props[STREAMOUT_BUFFER_SIZE] = "";
  unsigned int i;
  CHARFORMAT2W *old_fmt = &pStream->cur_fmt;
  static const struct
  {
      DWORD effect;
      const char *on, *off;
  } effects[] =
  {
      { CFE_ALLCAPS,     "\\caps",     "\\caps0"     },
      { CFE_BOLD,        "\\b",        "\\b0"        },
      { CFE_DISABLED,    "\\disabled", "\\disabled0" },
      { CFE_EMBOSS,      "\\embo",     "\\embo0"     },
      { CFE_HIDDEN,      "\\v",        "\\v0"        },
      { CFE_IMPRINT,     "\\impr",     "\\impr0"     },
      { CFE_ITALIC,      "\\i",        "\\i0"        },
      { CFE_OUTLINE,     "\\outl",     "\\outl0"     },
      { CFE_PROTECTED,   "\\protect",  "\\protect0"  },
      { CFE_SHADOW,      "\\shad",     "\\shad0"     },
      { CFE_SMALLCAPS,   "\\scaps",    "\\scaps0"    },
      { CFE_STRIKEOUT,   "\\strike",   "\\strike0"   },
  };

  for (i = 0; i < ARRAY_SIZE( effects ); i++)
  {
      if ((old_fmt->dwEffects ^ fmt->dwEffects) & effects[i].effect)
          strcat( props, fmt->dwEffects & effects[i].effect ? effects[i].on : effects[i].off );
  }

  if ((old_fmt->dwEffects ^ fmt->dwEffects) & CFE_AUTOBACKCOLOR ||
      (!(fmt->dwEffects & CFE_AUTOBACKCOLOR) && old_fmt->crBackColor != fmt->crBackColor))
  {
      if (fmt->dwEffects & CFE_AUTOBACKCOLOR) i = 0;
      else find_color_in_colortbl( pStream, fmt->crBackColor, &i );
      sprintf(props + strlen(props), "\\highlight%u", i);
  }
  if ((old_fmt->dwEffects ^ fmt->dwEffects) & CFE_AUTOCOLOR ||
      (!(fmt->dwEffects & CFE_AUTOCOLOR) && old_fmt->crTextColor != fmt->crTextColor))
  {
      if (fmt->dwEffects & CFE_AUTOCOLOR) i = 0;
      else find_color_in_colortbl( pStream, fmt->crTextColor, &i );
      sprintf(props + strlen(props), "\\cf%u", i);
  }

  if (old_fmt->bAnimation != fmt->bAnimation)
    sprintf(props + strlen(props), "\\animtext%u", fmt->bAnimation);
  if (old_fmt->wKerning != fmt->wKerning)
    sprintf(props + strlen(props), "\\kerning%u", fmt->wKerning);

  if (old_fmt->lcid != fmt->lcid)
  {
    /* TODO: handle SFF_PLAINRTF */
    if (LOWORD(fmt->lcid) == 1024)
      strcat(props, "\\noproof\\lang1024\\langnp1024\\langfe1024\\langfenp1024");
    else
      sprintf(props + strlen(props), "\\lang%u", LOWORD(fmt->lcid));
  }

  if (old_fmt->yOffset != fmt->yOffset)
  {
    if (fmt->yOffset >= 0)
      sprintf(props + strlen(props), "\\up%d", fmt->yOffset);
    else
      sprintf(props + strlen(props), "\\dn%d", -fmt->yOffset);
  }
  if (old_fmt->yHeight != fmt->yHeight)
    sprintf(props + strlen(props), "\\fs%d", fmt->yHeight / 10);
  if (old_fmt->sSpacing != fmt->sSpacing)
    sprintf(props + strlen(props), "\\expnd%u\\expndtw%u", fmt->sSpacing / 5, fmt->sSpacing);
  if ((old_fmt->dwEffects ^ fmt->dwEffects) & (CFM_SUBSCRIPT | CFM_SUPERSCRIPT))
  {
    if (fmt->dwEffects & CFE_SUBSCRIPT)
      strcat(props, "\\sub");
    else if (fmt->dwEffects & CFE_SUPERSCRIPT)
      strcat(props, "\\super");
    else
      strcat(props, "\\nosupersub");
  }
  if ((old_fmt->dwEffects ^ fmt->dwEffects) & CFE_UNDERLINE ||
      old_fmt->bUnderlineType != fmt->bUnderlineType)
  {
      BYTE type = (fmt->dwEffects & CFE_UNDERLINE) ? fmt->bUnderlineType : CFU_UNDERLINENONE;
      switch (type)
      {
      case CFU_UNDERLINE:
          strcat(props, "\\ul");
          break;
      case CFU_UNDERLINEDOTTED:
          strcat(props, "\\uld");
          break;
      case CFU_UNDERLINEDOUBLE:
          strcat(props, "\\uldb");
          break;
      case CFU_UNDERLINEWORD:
          strcat(props, "\\ulw");
          break;
      case CFU_CF1UNDERLINE:
      case CFU_UNDERLINENONE:
      default:
          strcat(props, "\\ulnone");
          break;
      }
  }
  
  if (wcscmp(old_fmt->szFaceName, fmt->szFaceName) ||
      old_fmt->bCharSet != fmt->bCharSet)
  {
    if (find_font_in_fonttbl( pStream, fmt, &i ))
    {
      sprintf(props + strlen(props), "\\f%u", i);

      /* In UTF-8 mode, charsets/codepages are not used */
      if (pStream->nDefaultCodePage != CP_UTF8)
      {
        if (pStream->fonttbl[i].bCharSet == DEFAULT_CHARSET)
          pStream->nCodePage = pStream->nDefaultCodePage;
        else
          pStream->nCodePage = RTFCharSetToCodePage(NULL, pStream->fonttbl[i].bCharSet);
      }
    }
  }
  if (*props)
    strcat(props, " ");
  if (!ME_StreamOutPrint(pStream, props))
    return FALSE;
  *old_fmt = *fmt;
  return TRUE;
}


static BOOL
ME_StreamOutRTFText(ME_OutStream *pStream, const WCHAR *text, LONG nChars)
{
  char buffer[STREAMOUT_BUFFER_SIZE];
  int pos = 0;
  int fit, nBytes, i;

  if (nChars == -1)
    nChars = lstrlenW(text);
  
  while (nChars) {
    /* In UTF-8 mode, font charsets are not used. */
    if (pStream->nDefaultCodePage == CP_UTF8) {
      /* 6 is the maximum character length in UTF-8 */
      fit = min(nChars, STREAMOUT_BUFFER_SIZE / 6);
      nBytes = WideCharToMultiByte(CP_UTF8, 0, text, fit, buffer,
                                   STREAMOUT_BUFFER_SIZE, NULL, NULL);
      nChars -= fit;
      text += fit;
      for (i = 0; i < nBytes; i++)
        if (buffer[i] == '{' || buffer[i] == '}' || buffer[i] == '\\') {
          if (!ME_StreamOutPrint(pStream, "%.*s\\", i - pos, buffer + pos))
            return FALSE;
          pos = i;
        }
      if (pos < nBytes)
        if (!ME_StreamOutMove(pStream, buffer + pos, nBytes - pos))
          return FALSE;
      pos = 0;
    } else if (*text < 128) {
      if (*text == '{' || *text == '}' || *text == '\\')
        buffer[pos++] = '\\';
      buffer[pos++] = (char)(*text++);
      nChars--;
    } else {
      BOOL unknown = FALSE;
      char letter[3];

      /* FIXME: In the MS docs for WideCharToMultiByte there is a big list of
       * codepages including CP_SYMBOL for which the last parameter must be set
       * to NULL for the function to succeed. But in Wine we need to care only
       * about CP_SYMBOL */
      nBytes = WideCharToMultiByte(pStream->nCodePage, 0, text, 1,
                                   letter, 3, NULL,
                                   (pStream->nCodePage == CP_SYMBOL) ? NULL : &unknown);
      if (unknown)
        pos += sprintf(buffer + pos, "\\u%d?", (short)*text);
      else if ((BYTE)*letter < 128) {
        if (*letter == '{' || *letter == '}' || *letter == '\\')
          buffer[pos++] = '\\';
        buffer[pos++] = *letter;
      } else {
         for (i = 0; i < nBytes; i++)
           pos += sprintf(buffer + pos, "\\'%02x", (BYTE)letter[i]);
      }
      text++;
      nChars--;
    }
    if (pos >= STREAMOUT_BUFFER_SIZE - 11) {
      if (!ME_StreamOutMove(pStream, buffer, pos))
        return FALSE;
      pos = 0;
    }
  }
  return ME_StreamOutMove(pStream, buffer, pos);
}

static BOOL stream_out_graphics( ME_TextEditor *editor, ME_OutStream *stream,
                                 ME_Run *run )
{
    IDataObject *data;
    HRESULT hr;
    FORMATETC fmt = { CF_ENHMETAFILE, NULL, DVASPECT_CONTENT, -1, TYMED_ENHMF };
    STGMEDIUM med = { TYMED_NULL };
    BOOL ret = FALSE;
    ENHMETAHEADER *emf_bits = NULL;
    UINT size;
    SIZE goal, pic;
    ME_Context c;

    hr = IOleObject_QueryInterface( run->reobj->obj.poleobj, &IID_IDataObject, (void **)&data );
    if (FAILED(hr)) return FALSE;

    ME_InitContext( &c, editor, ITextHost_TxGetDC( editor->texthost ) );
    hr = IDataObject_QueryGetData( data, &fmt );
    if (hr != S_OK) goto done;

    hr = IDataObject_GetData( data, &fmt, &med );
    if (FAILED(hr)) goto done;
    if (med.tymed != TYMED_ENHMF) goto done;

    size = GetEnhMetaFileBits( med.u.hEnhMetaFile, 0, NULL );
    if (size < FIELD_OFFSET(ENHMETAHEADER, cbPixelFormat)) goto done;

    emf_bits = HeapAlloc( GetProcessHeap(), 0, size );
    if (!emf_bits) goto done;

    size = GetEnhMetaFileBits( med.u.hEnhMetaFile, size, (BYTE *)emf_bits );
    if (size < FIELD_OFFSET(ENHMETAHEADER, cbPixelFormat)) goto done;

    /* size_in_pixels = (frame_size / 100) * szlDevice / szlMillimeters
       pic = size_in_pixels * 2540 / dpi */
    pic.cx = MulDiv( emf_bits->rclFrame.right - emf_bits->rclFrame.left, emf_bits->szlDevice.cx * 254,
                     emf_bits->szlMillimeters.cx * c.dpi.cx * 10 );
    pic.cy = MulDiv( emf_bits->rclFrame.bottom - emf_bits->rclFrame.top, emf_bits->szlDevice.cy * 254,
                     emf_bits->szlMillimeters.cy * c.dpi.cy * 10 );

    /* convert goal size to twips */
    goal.cx = MulDiv( run->reobj->obj.sizel.cx, 144, 254 );
    goal.cy = MulDiv( run->reobj->obj.sizel.cy, 144, 254 );

    if (!ME_StreamOutPrint( stream, "{\\*\\shppict{\\pict\\emfblip\\picw%d\\pich%d\\picwgoal%d\\pichgoal%d\n",
                            pic.cx, pic.cy, goal.cx, goal.cy ))
        goto done;

    if (!ME_StreamOutHexData( stream, (BYTE *)emf_bits, size ))
        goto done;

    if (!ME_StreamOutPrint( stream, "}}\n" ))
        goto done;

    ret = TRUE;

done:
    ME_DestroyContext( &c );
    HeapFree( GetProcessHeap(), 0, emf_bits );
    ReleaseStgMedium( &med );
    IDataObject_Release( data );
    return ret;
}

static BOOL ME_StreamOutRTF(ME_TextEditor *editor, ME_OutStream *pStream,
                            const ME_Cursor *start, int nChars, int dwFormat)
{
  ME_Cursor cursor = *start;
  ME_DisplayItem *prev_para = NULL;
  ME_Cursor endCur = cursor;

  ME_MoveCursorChars(editor, &endCur, nChars, TRUE);

  if (!ME_StreamOutRTFHeader(pStream, dwFormat))
    return FALSE;

  if (!ME_StreamOutRTFFontAndColorTbl(pStream, cursor.pRun, endCur.pRun))
    return FALSE;

  /* TODO: stylesheet table */

  if (!ME_StreamOutPrint(pStream, "{\\*\\generator Wine Riched20 2.0;}\r\n"))
    return FALSE;

  /* TODO: information group */

  /* TODO: document formatting properties */

  /* FIXME: We have only one document section */

  /* TODO: section formatting properties */

  do {
    if (cursor.pPara != prev_para)
    {
      prev_para = cursor.pPara;
      if (!ME_StreamOutRTFParaProps(editor, pStream, cursor.pPara))
        return FALSE;
    }

    if (cursor.pRun == endCur.pRun && !endCur.nOffset)
      break;
    TRACE("flags %xh\n", cursor.pRun->member.run.nFlags);
    /* TODO: emit embedded objects */
    if (cursor.pPara->member.para.nFlags & (MEPF_ROWSTART|MEPF_ROWEND))
      continue;
    if (cursor.pRun->member.run.nFlags & MERF_GRAPHICS) {
      if (!stream_out_graphics(editor, pStream, &cursor.pRun->member.run))
        return FALSE;
    } else if (cursor.pRun->member.run.nFlags & MERF_TAB) {
      if (editor->bEmulateVersion10 && /* v1.0 - 3.0 */
          cursor.pPara->member.para.fmt.dwMask & PFM_TABLE &&
          cursor.pPara->member.para.fmt.wEffects & PFE_TABLE)
      {
        if (!ME_StreamOutPrint(pStream, "\\cell "))
          return FALSE;
      } else {
        if (!ME_StreamOutPrint(pStream, "\\tab "))
          return FALSE;
      }
    } else if (cursor.pRun->member.run.nFlags & MERF_ENDCELL) {
      if (pStream->nNestingLevel > 1) {
        if (!ME_StreamOutPrint(pStream, "\\nestcell "))
          return FALSE;
      } else {
        if (!ME_StreamOutPrint(pStream, "\\cell "))
          return FALSE;
      }
      nChars--;
    } else if (cursor.pRun->member.run.nFlags & MERF_ENDPARA) {
      if (!ME_StreamOutRTFCharProps(pStream, &cursor.pRun->member.run.style->fmt))
        return FALSE;

      if (cursor.pPara->member.para.fmt.dwMask & PFM_TABLE &&
          cursor.pPara->member.para.fmt.wEffects & PFE_TABLE &&
          !(cursor.pPara->member.para.nFlags & (MEPF_ROWSTART|MEPF_ROWEND|MEPF_CELL)))
      {
        if (!ME_StreamOutPrint(pStream, "\\row\r\n"))
          return FALSE;
      } else {
        if (!ME_StreamOutPrint(pStream, "\\par\r\n"))
          return FALSE;
      }
      /* Skip as many characters as required by current line break */
      nChars = max(0, nChars - cursor.pRun->member.run.len);
    } else if (cursor.pRun->member.run.nFlags & MERF_ENDROW) {
      if (!ME_StreamOutPrint(pStream, "\\line\r\n"))
        return FALSE;
      nChars--;
    } else {
      int nEnd;

      TRACE("style %p\n", cursor.pRun->member.run.style);
      if (!ME_StreamOutRTFCharProps(pStream, &cursor.pRun->member.run.style->fmt))
        return FALSE;

      nEnd = (cursor.pRun == endCur.pRun) ? endCur.nOffset : cursor.pRun->member.run.len;
      if (!ME_StreamOutRTFText(pStream, get_text( &cursor.pRun->member.run, cursor.nOffset ),
                               nEnd - cursor.nOffset))
        return FALSE;
      cursor.nOffset = 0;
    }
  } while (cursor.pRun != endCur.pRun && ME_NextRun(&cursor.pPara, &cursor.pRun, TRUE));

  if (!ME_StreamOutMove(pStream, "}\0", 2))
    return FALSE;
  return TRUE;
}


static BOOL ME_StreamOutText(ME_TextEditor *editor, ME_OutStream *pStream,
                             const ME_Cursor *start, int nChars, DWORD dwFormat)
{
  ME_Cursor cursor = *start;
  int nLen;
  UINT nCodePage = CP_ACP;
  char *buffer = NULL;
  int nBufLen = 0;
  BOOL success = TRUE;

  if (!cursor.pRun)
    return FALSE;

  if (dwFormat & SF_USECODEPAGE)
    nCodePage = HIWORD(dwFormat);

  /* TODO: Handle SF_TEXTIZED */

  while (success && nChars && cursor.pRun) {
    nLen = min(nChars, cursor.pRun->member.run.len - cursor.nOffset);

    if (!editor->bEmulateVersion10 && cursor.pRun->member.run.nFlags & MERF_ENDPARA)
    {
      static const WCHAR szEOL[] = { '\r', '\n' };

      /* richedit 2.0 - all line breaks are \r\n */
      if (dwFormat & SF_UNICODE)
        success = ME_StreamOutMove(pStream, (const char *)szEOL, sizeof(szEOL));
      else
        success = ME_StreamOutMove(pStream, "\r\n", 2);
    } else {
      if (dwFormat & SF_UNICODE)
        success = ME_StreamOutMove(pStream, (const char *)(get_text( &cursor.pRun->member.run, cursor.nOffset )),
                                   sizeof(WCHAR) * nLen);
      else {
        int nSize;

        nSize = WideCharToMultiByte(nCodePage, 0, get_text( &cursor.pRun->member.run, cursor.nOffset ),
                                    nLen, NULL, 0, NULL, NULL);
        if (nSize > nBufLen) {
          buffer = heap_realloc(buffer, nSize);
          nBufLen = nSize;
        }
        WideCharToMultiByte(nCodePage, 0, get_text( &cursor.pRun->member.run, cursor.nOffset ),
                            nLen, buffer, nSize, NULL, NULL);
        success = ME_StreamOutMove(pStream, buffer, nSize);
      }
    }

    nChars -= nLen;
    cursor.nOffset = 0;
    cursor.pRun = ME_FindItemFwd(cursor.pRun, diRun);
  }

  heap_free(buffer);
  return success;
}


LRESULT ME_StreamOutRange(ME_TextEditor *editor, DWORD dwFormat,
                          const ME_Cursor *start,
                          int nChars, EDITSTREAM *stream)
{
  ME_OutStream *pStream = ME_StreamOutInit(editor, stream);

  if (dwFormat & SF_RTF)
    ME_StreamOutRTF(editor, pStream, start, nChars, dwFormat);
  else if (dwFormat & SF_TEXT || dwFormat & SF_TEXTIZED)
    ME_StreamOutText(editor, pStream, start, nChars, dwFormat);
  if (!pStream->stream->dwError)
    ME_StreamOutFlush(pStream);
  return ME_StreamOutFree(pStream);
}

LRESULT
ME_StreamOut(ME_TextEditor *editor, DWORD dwFormat, EDITSTREAM *stream)
{
  ME_Cursor start;
  int nChars;

  if (dwFormat & SFF_SELECTION) {
    int nStart, nTo;
    start = editor->pCursors[ME_GetSelectionOfs(editor, &nStart, &nTo)];
    nChars = nTo - nStart;
  } else {
    ME_SetCursorToStart(editor, &start);
    nChars = ME_GetTextLength(editor);
    /* Generate an end-of-paragraph at the end of SCF_ALL RTF output */
    if (dwFormat & SF_RTF)
      nChars++;
  }
  return ME_StreamOutRange(editor, dwFormat, &start, nChars, stream);
}
