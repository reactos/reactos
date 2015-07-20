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

#include "editor.h"
#include "rtf.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);


static BOOL
ME_StreamOutRTFText(ME_OutStream *pStream, const WCHAR *text, LONG nChars);


static ME_OutStream*
ME_StreamOutInit(ME_TextEditor *editor, EDITSTREAM *stream)
{
  ME_OutStream *pStream = ALLOC_OBJ(ME_OutStream);
  pStream->stream = stream;
  pStream->stream->dwError = 0;
  pStream->pos = 0;
  pStream->written = 0;
  pStream->nFontTblLen = 0;
  pStream->nColorTblLen = 1;
  pStream->nNestingLevel = 0;
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
  pStream->pos = 0;
  return TRUE;
}


static LONG
ME_StreamOutFree(ME_OutStream *pStream)
{
  LONG written = pStream->written;
  TRACE("total length = %u\n", written);

  FREE_OBJ(pStream);
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


static BOOL
ME_StreamOutPrint(ME_OutStream *pStream, const char *format, ...)
{
  char string[STREAMOUT_BUFFER_SIZE]; /* This is going to be enough */
  int len;
  va_list valist;

  va_start(valist, format);
  len = vsnprintf(string, sizeof(string), format, valist);
  va_end(valist);
  
  return ME_StreamOutMove(pStream, string, len);
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


static BOOL
ME_StreamOutRTFFontAndColorTbl(ME_OutStream *pStream, ME_DisplayItem *pFirstRun,
                               ME_DisplayItem *pLastRun)
{
  ME_DisplayItem *item = pFirstRun;
  ME_FontTableItem *table = pStream->fonttbl;
  unsigned int i;
  ME_DisplayItem *pLastPara = ME_GetParagraph(pLastRun);
  ME_DisplayItem *pCell = NULL;
  
  do {
    CHARFORMAT2W *fmt = &item->member.run.style->fmt;
    COLORREF crColor;

    if (fmt->dwMask & CFM_FACE) {
      WCHAR *face = fmt->szFaceName;
      BYTE bCharSet = (fmt->dwMask & CFM_CHARSET) ? fmt->bCharSet : DEFAULT_CHARSET;
  
      for (i = 0; i < pStream->nFontTblLen; i++)
        if (table[i].bCharSet == bCharSet
            && (table[i].szFaceName == face || !lstrcmpW(table[i].szFaceName, face)))
          break;
      if (i == pStream->nFontTblLen && i < STREAMOUT_FONTTBL_SIZE) {
        table[i].bCharSet = bCharSet;
        table[i].szFaceName = face;
        pStream->nFontTblLen++;
      }
    }
    
    if (fmt->dwMask & CFM_COLOR && !(fmt->dwEffects & CFE_AUTOCOLOR)) {
      crColor = fmt->crTextColor;
      for (i = 1; i < pStream->nColorTblLen; i++)
        if (pStream->colortbl[i] == crColor)
          break;
      if (i == pStream->nColorTblLen && i < STREAMOUT_COLORTBL_SIZE) {
        pStream->colortbl[i] = crColor;
        pStream->nColorTblLen++;
      }
    }
    if (fmt->dwMask & CFM_BACKCOLOR && !(fmt->dwEffects & CFE_AUTOBACKCOLOR)) {
      crColor = fmt->crBackColor;
      for (i = 1; i < pStream->nColorTblLen; i++)
        if (pStream->colortbl[i] == crColor)
          break;
      if (i == pStream->nColorTblLen && i < STREAMOUT_COLORTBL_SIZE) {
        pStream->colortbl[i] = crColor;
        pStream->nColorTblLen++;
      }
    }

    if (item == pLastRun)
      break;
    item = ME_FindItemFwd(item, diRun);
  } while (item);
  item = ME_GetParagraph(pFirstRun);
  do {
    if ((pCell = item->member.para.pCell))
    {
        ME_Border* borders[4] = { &pCell->member.cell.border.top,
                                  &pCell->member.cell.border.left,
                                  &pCell->member.cell.border.bottom,
                                  &pCell->member.cell.border.right };
        for (i = 0; i < 4; i++)
        {
          if (borders[i]->width > 0)
          {
            unsigned int j;
            COLORREF crColor = borders[i]->colorRef;
            for (j = 1; j < pStream->nColorTblLen; j++)
              if (pStream->colortbl[j] == crColor)
                break;
            if (j == pStream->nColorTblLen && j < STREAMOUT_COLORTBL_SIZE) {
              pStream->colortbl[j] = crColor;
              pStream->nColorTblLen++;
            }
          }
        }
    }
    if (item == pLastPara)
      break;
    item = item->member.para.next_para;
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

  /* It seems like Open Office ignores \deff0 tag at RTF-header.
     As result it can't correctly parse text before first \fN tag,
     so we can put \f0 immediately after font table. This forces
     parser to use the same font, that \deff0 specifies.
     It makes OOffice happy */
  if (!ME_StreamOutPrint(pStream, "\\f0"))
    return FALSE;

  /* Output the color table */
  if (!ME_StreamOutPrint(pStream, "{\\colortbl;")) return FALSE; /* first entry is auto-color */
  for (i = 1; i < pStream->nColorTblLen; i++)
  {
    if (!ME_StreamOutPrint(pStream, "\\red%u\\green%u\\blue%u;", pStream->colortbl[i] & 0xFF,
                           (pStream->colortbl[i] >> 8) & 0xFF, (pStream->colortbl[i] >> 16) & 0xFF))
      return FALSE;
  }
  if (!ME_StreamOutPrint(pStream, "}")) return FALSE;

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
    PARAFORMAT2 *pFmt = ME_GetTableRowEnd(para)->member.para.pFmt;
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
          unsigned int j;
          COLORREF crColor = borders[i]->colorRef;
          sprintf(props + strlen(props), "\\clbrdr%c", sideChar[i]);
          sprintf(props + strlen(props), "\\brdrs");
          sprintf(props + strlen(props), "\\brdrw%d", borders[i]->width);
          for (j = 1; j < pStream->nColorTblLen; j++) {
            if (pStream->colortbl[j] == crColor) {
              sprintf(props + strlen(props), "\\brdrcf%u", j);
              break;
            }
          }
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
    PARAFORMAT2 *pFmt = para->member.para.pFmt;

    assert(!(para->member.para.nFlags & (MEPF_ROWSTART|MEPF_ROWEND|MEPF_CELL)));
    if (pFmt->dxOffset)
      sprintf(props + strlen(props), "\\trgaph%d", pFmt->dxOffset);
    if (pFmt->dxStartIndent)
      sprintf(props + strlen(props), "\\trleft%d", pFmt->dxStartIndent);
    for (i = 0; i < 4; i++)
    {
      if (borders[i]->width)
      {
        unsigned int j;
        COLORREF crColor = borders[i]->colorRef;
        sprintf(props + strlen(props), "\\trbrdr%c", sideChar[i]);
        sprintf(props + strlen(props), "\\brdrs");
        sprintf(props + strlen(props), "\\brdrw%d", borders[i]->width);
        for (j = 1; j < pStream->nColorTblLen; j++) {
          if (pStream->colortbl[j] == crColor) {
            sprintf(props + strlen(props), "\\brdrcf%u", j);
            break;
          }
        }
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

static BOOL
ME_StreamOutRTFParaProps(ME_TextEditor *editor, ME_OutStream *pStream,
                         ME_DisplayItem *para)
{
  PARAFORMAT2 *fmt = para->member.para.pFmt;
  char props[STREAMOUT_BUFFER_SIZE] = "";
  int i;

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
        if (!ME_StreamOutPrint(pStream, "\\row \r\n"))
          return FALSE;
      }
      return TRUE;
    }
  } else { /* v1.0 - 3.0 */
    if (para->member.para.pFmt->dwMask & PFM_TABLE &&
        para->member.para.pFmt->wEffects & PFE_TABLE)
    {
      if (!ME_StreamOutRTFTableProps(editor, pStream, para))
        return FALSE;
    }
  }

  /* TODO: Don't emit anything if the last PARAFORMAT2 is inherited */
  if (!ME_StreamOutPrint(pStream, "\\pard"))
    return FALSE;

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
    if (fmt->dwMask & PFM_OFFSET)
      sprintf(props + strlen(props), "\\li%d", fmt->dxOffset);
    if (fmt->dwMask & PFM_OFFSETINDENT || fmt->dwMask & PFM_STARTINDENT)
      sprintf(props + strlen(props), "\\fi%d", fmt->dxStartIndent);
    if (fmt->dwMask & PFM_RIGHTINDENT)
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
  if (fmt->dwMask & PFM_SPACEAFTER)
    sprintf(props + strlen(props), "\\sa%d", fmt->dySpaceAfter);
  if (fmt->dwMask & PFM_SPACEBEFORE)
    sprintf(props + strlen(props), "\\sb%d", fmt->dySpaceBefore);
  if (fmt->dwMask & PFM_STYLE)
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
    sprintf(props + strlen(props), "\\cfpat%d\\cbpat%d",
            (fmt->wShadingStyle >> 4) & 0xF, (fmt->wShadingStyle >> 8) & 0xF);
  }
  
  if (*props && !ME_StreamOutPrint(pStream, props))
    return FALSE;

  return TRUE;
}


static BOOL
ME_StreamOutRTFCharProps(ME_OutStream *pStream, CHARFORMAT2W *fmt)
{
  char props[STREAMOUT_BUFFER_SIZE] = "";
  unsigned int i;

  if (fmt->dwMask & CFM_ALLCAPS && fmt->dwEffects & CFE_ALLCAPS)
    strcat(props, "\\caps");
  if (fmt->dwMask & CFM_ANIMATION)
    sprintf(props + strlen(props), "\\animtext%u", fmt->bAnimation);
  if (fmt->dwMask & CFM_BACKCOLOR) {
    if (!(fmt->dwEffects & CFE_AUTOBACKCOLOR)) {
      for (i = 1; i < pStream->nColorTblLen; i++)
        if (pStream->colortbl[i] == fmt->crBackColor) {
          sprintf(props + strlen(props), "\\cb%u", i);
          break;
        }
    }
  }
  if (fmt->dwMask & CFM_BOLD && fmt->dwEffects & CFE_BOLD)
    strcat(props, "\\b");
  if (fmt->dwMask & CFM_COLOR) {
    if (!(fmt->dwEffects & CFE_AUTOCOLOR)) {
      for (i = 1; i < pStream->nColorTblLen; i++)
        if (pStream->colortbl[i] == fmt->crTextColor) {
          sprintf(props + strlen(props), "\\cf%u", i);
          break;
        }
    }
  }
  /* TODO: CFM_DISABLED */
  if (fmt->dwMask & CFM_EMBOSS && fmt->dwEffects & CFE_EMBOSS)
    strcat(props, "\\embo");
  if (fmt->dwMask & CFM_HIDDEN && fmt->dwEffects & CFE_HIDDEN)
    strcat(props, "\\v");
  if (fmt->dwMask & CFM_IMPRINT && fmt->dwEffects & CFE_IMPRINT)
    strcat(props, "\\impr");
  if (fmt->dwMask & CFM_ITALIC && fmt->dwEffects & CFE_ITALIC)
    strcat(props, "\\i");
  if (fmt->dwMask & CFM_KERNING)
    sprintf(props + strlen(props), "\\kerning%u", fmt->wKerning);
  if (fmt->dwMask & CFM_LCID) {
    /* TODO: handle SFF_PLAINRTF */
    if (LOWORD(fmt->lcid) == 1024)
      strcat(props, "\\noproof\\lang1024\\langnp1024\\langfe1024\\langfenp1024");
    else
      sprintf(props + strlen(props), "\\lang%u", LOWORD(fmt->lcid));
  }
  /* CFM_LINK is not streamed out by M$ */
  if (fmt->dwMask & CFM_OFFSET) {
    if (fmt->yOffset >= 0)
      sprintf(props + strlen(props), "\\up%d", fmt->yOffset);
    else
      sprintf(props + strlen(props), "\\dn%d", -fmt->yOffset);
  }
  if (fmt->dwMask & CFM_OUTLINE && fmt->dwEffects & CFE_OUTLINE)
    strcat(props, "\\outl");
  if (fmt->dwMask & CFM_PROTECTED && fmt->dwEffects & CFE_PROTECTED)
    strcat(props, "\\protect");
  /* TODO: CFM_REVISED CFM_REVAUTHOR - probably using rsidtbl? */
  if (fmt->dwMask & CFM_SHADOW && fmt->dwEffects & CFE_SHADOW)
    strcat(props, "\\shad");
  if (fmt->dwMask & CFM_SIZE)
    sprintf(props + strlen(props), "\\fs%d", fmt->yHeight / 10);
  if (fmt->dwMask & CFM_SMALLCAPS && fmt->dwEffects & CFE_SMALLCAPS)
    strcat(props, "\\scaps");
  if (fmt->dwMask & CFM_SPACING)
    sprintf(props + strlen(props), "\\expnd%u\\expndtw%u", fmt->sSpacing / 5, fmt->sSpacing);
  if (fmt->dwMask & CFM_STRIKEOUT && fmt->dwEffects & CFE_STRIKEOUT)
    strcat(props, "\\strike");
  if (fmt->dwMask & CFM_STYLE) {
    sprintf(props + strlen(props), "\\cs%u", fmt->sStyle);
    /* TODO: emit style contents here */
  }
  if (fmt->dwMask & (CFM_SUBSCRIPT | CFM_SUPERSCRIPT)) {
    if (fmt->dwEffects & CFE_SUBSCRIPT)
      strcat(props, "\\sub");
    else if (fmt->dwEffects & CFE_SUPERSCRIPT)
      strcat(props, "\\super");
  }
  if (fmt->dwMask & CFM_UNDERLINE || fmt->dwMask & CFM_UNDERLINETYPE) {
    if (fmt->dwMask & CFM_UNDERLINETYPE)
      switch (fmt->bUnderlineType) {
        case CFU_CF1UNDERLINE:
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
        case CFU_UNDERLINENONE:
        default:
          strcat(props, "\\ulnone");
          break;
      }
    else if (fmt->dwEffects & CFE_UNDERLINE)
      strcat(props, "\\ul");
  }
  /* FIXME: How to emit CFM_WEIGHT? */
  
  if (fmt->dwMask & CFM_FACE || fmt->dwMask & CFM_CHARSET) {
    WCHAR *szFaceName;
    
    if (fmt->dwMask & CFM_FACE)
      szFaceName = fmt->szFaceName;
    else
      szFaceName = pStream->fonttbl[0].szFaceName;
    for (i = 0; i < pStream->nFontTblLen; i++) {
      if (szFaceName == pStream->fonttbl[i].szFaceName
          || !lstrcmpW(szFaceName, pStream->fonttbl[i].szFaceName))
        if (!(fmt->dwMask & CFM_CHARSET)
            || fmt->bCharSet == pStream->fonttbl[i].bCharSet)
          break;
    }
    if (i < pStream->nFontTblLen)
    {
      if (i != pStream->nDefaultFont)
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


static BOOL ME_StreamOutRTF(ME_TextEditor *editor, ME_OutStream *pStream,
                            const ME_Cursor *start, int nChars, int dwFormat)
{
  ME_Cursor cursor = *start;
  ME_DisplayItem *prev_para = cursor.pPara;
  ME_Cursor endCur = cursor;
  int actual_chars;

  actual_chars = ME_MoveCursorChars(editor, &endCur, nChars);
  /* Include the final \r which MoveCursorChars will ignore. */
  if (actual_chars != nChars) endCur.nOffset++;

  if (!ME_StreamOutRTFHeader(pStream, dwFormat))
    return FALSE;

  if (!ME_StreamOutRTFFontAndColorTbl(pStream, cursor.pRun, endCur.pRun))
    return FALSE;

  /* TODO: stylesheet table */

  /* FIXME: maybe emit something smarter for the generator? */
  if (!ME_StreamOutPrint(pStream, "{\\*\\generator Wine Riched20 2.0.????;}"))
    return FALSE;

  /* TODO: information group */

  /* TODO: document formatting properties */

  /* FIXME: We have only one document section */

  /* TODO: section formatting properties */

  if (!ME_StreamOutRTFParaProps(editor, pStream, cursor.pPara))
    return FALSE;

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
      FIXME("embedded objects are not handled\n");
    } else if (cursor.pRun->member.run.nFlags & MERF_TAB) {
      if (editor->bEmulateVersion10 && /* v1.0 - 3.0 */
          cursor.pPara->member.para.pFmt->dwMask & PFM_TABLE &&
          cursor.pPara->member.para.pFmt->wEffects & PFE_TABLE)
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
      if (cursor.pPara->member.para.pFmt->dwMask & PFM_TABLE &&
          cursor.pPara->member.para.pFmt->wEffects & PFE_TABLE &&
          !(cursor.pPara->member.para.nFlags & (MEPF_ROWSTART|MEPF_ROWEND|MEPF_CELL)))
      {
        if (!ME_StreamOutPrint(pStream, "\\row \r\n"))
          return FALSE;
      } else {
        if (!ME_StreamOutPrint(pStream, "\r\n\\par"))
          return FALSE;
      }
      /* Skip as many characters as required by current line break */
      nChars = max(0, nChars - cursor.pRun->member.run.len);
    } else if (cursor.pRun->member.run.nFlags & MERF_ENDROW) {
      if (!ME_StreamOutPrint(pStream, "\\line \r\n"))
        return FALSE;
      nChars--;
    } else {
      int nEnd;

      if (!ME_StreamOutPrint(pStream, "{"))
        return FALSE;
      TRACE("style %p\n", cursor.pRun->member.run.style);
      if (!ME_StreamOutRTFCharProps(pStream, &cursor.pRun->member.run.style->fmt))
        return FALSE;

      nEnd = (cursor.pRun == endCur.pRun) ? endCur.nOffset : cursor.pRun->member.run.len;
      if (!ME_StreamOutRTFText(pStream, get_text( &cursor.pRun->member.run, cursor.nOffset ),
                               nEnd - cursor.nOffset))
        return FALSE;
      cursor.nOffset = 0;
      if (!ME_StreamOutPrint(pStream, "}"))
        return FALSE;
    }
  } while (cursor.pRun != endCur.pRun && ME_NextRun(&cursor.pPara, &cursor.pRun));

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
          FREE_OBJ(buffer);
          buffer = ALLOC_N_OBJ(char, nSize);
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

  FREE_OBJ(buffer);
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
