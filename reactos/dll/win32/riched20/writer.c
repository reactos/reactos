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
ME_StreamOutRTFText(ME_OutStream *pStream, WCHAR *text, LONG nChars);


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
  return pStream;
}


static BOOL
ME_StreamOutFlush(ME_OutStream *pStream)
{
  LONG nStart = 0;
  LONG nWritten = 0;
  LONG nRemaining = 0;
  EDITSTREAM *stream = pStream->stream;

  do {
    TRACE("sending %u bytes\n", pStream->pos - nStart);
    /* Some apps seem not to set *pcb unless a problem arises, relying
      on initial random nWritten value, which is usually >STREAMOUT_BUFFER_SIZE */
    nRemaining = pStream->pos - nStart;
    nWritten = 0xDEADBEEF;
    stream->dwError = stream->pfnCallback(stream->dwCookie, (LPBYTE)pStream->buffer + nStart,
                                          pStream->pos - nStart, &nWritten);
    TRACE("error=%u written=%u\n", stream->dwError, nWritten);
    if (nWritten > (pStream->pos - nStart) || nWritten<0) {
      FIXME("Invalid returned written size *pcb: 0x%x (%d) instead of %d\n", 
            (unsigned)nWritten, nWritten, nRemaining);
      nWritten = nRemaining;
    }
    if (nWritten == 0 || stream->dwError)
      return FALSE;
    pStream->written += nWritten;
    nStart += nWritten;
  } while (nStart < pStream->pos);
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

    TRACE("%u:%u:%.*s\n", pStream->pos, fit, fit, buffer);
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
ME_StreamOutRTFFontAndColorTbl(ME_OutStream *pStream, ME_DisplayItem *pFirstRun, ME_DisplayItem *pLastRun)
{
  ME_DisplayItem *item = pFirstRun;
  ME_FontTableItem *table = pStream->fonttbl;
  int i;
  
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
      if (i == pStream->nFontTblLen) {
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
      if (i == pStream->nColorTblLen) {
        pStream->colortbl[i] = crColor;
        pStream->nColorTblLen++;
      }
    }
    if (fmt->dwMask & CFM_BACKCOLOR && !(fmt->dwEffects & CFE_AUTOBACKCOLOR)) {
      crColor = fmt->crBackColor;
      for (i = 1; i < pStream->nColorTblLen; i++)
        if (pStream->colortbl[i] == crColor)
          break;
      if (i == pStream->nColorTblLen) {
        pStream->colortbl[i] = crColor;
        pStream->nColorTblLen++;
      }
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
    if (!ME_StreamOutPrint(pStream, ";}\r\n"))
      return FALSE;
  }
  if (!ME_StreamOutPrint(pStream, "}"))
    return FALSE;

  /* Output colors table if not empty */
  if (pStream->nColorTblLen > 1) {
    if (!ME_StreamOutPrint(pStream, "{\\colortbl;"))
      return FALSE;
    for (i = 1; i < pStream->nColorTblLen; i++) {
      if (!ME_StreamOutPrint(pStream, "\\red%u\\green%u\\blue%u;",
                             pStream->colortbl[i] & 0xFF,
                             (pStream->colortbl[i] >> 8) & 0xFF,
                             (pStream->colortbl[i] >> 16) & 0xFF))
        return FALSE;
    }
    if (!ME_StreamOutPrint(pStream, "}"))
      return FALSE;
  }

  return TRUE;
}


static BOOL
ME_StreamOutRTFParaProps(ME_OutStream *pStream, ME_DisplayItem *para)
{
  PARAFORMAT2 *fmt = para->member.para.pFmt;
  char props[STREAMOUT_BUFFER_SIZE] = "";
  int i;

  if (para->member.para.pCells)
  {
    ME_TableCell *cell = para->member.para.pCells;
    
    if (!ME_StreamOutPrint(pStream, "\\trowd"))
      return FALSE;
    do {
      sprintf(props, "\\cellx%d", cell->nRightBoundary);
      if (!ME_StreamOutPrint(pStream, props))
        return FALSE;
      cell = cell->next;
    } while (cell);
    props[0] = '\0';
  }
  
  /* TODO: Don't emit anything if the last PARAFORMAT2 is inherited */
  if (!ME_StreamOutPrint(pStream, "\\pard"))
    return FALSE;

  if (para->member.para.bTable)
    strcat(props, "\\intbl");
  
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
  if (fmt->dwMask & PFM_TABLE && fmt->dwMask & PFE_TABLE)
    strcat(props, "\\intbl");
  
  if (fmt->dwMask & PFM_OFFSET)
    sprintf(props + strlen(props), "\\li%d", fmt->dxOffset);
  if (fmt->dwMask & PFM_OFFSETINDENT || fmt->dwMask & PFM_STARTINDENT)
    sprintf(props + strlen(props), "\\fi%d", fmt->dxStartIndent);
  if (fmt->dwMask & PFM_RIGHTINDENT)
    sprintf(props + strlen(props), "\\ri%d", fmt->dxRightIndent);
  if (fmt->dwMask & PFM_SPACEAFTER)
    sprintf(props + strlen(props), "\\sa%d", fmt->dySpaceAfter);
  if (fmt->dwMask & PFM_SPACEBEFORE)
    sprintf(props + strlen(props), "\\sb%d", fmt->dySpaceBefore);
  if (fmt->dwMask & PFM_STYLE)
    sprintf(props + strlen(props), "\\s%d", fmt->sStyle);

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
  int i;

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
          strcat(props, "\\ul0");
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
ME_StreamOutRTFText(ME_OutStream *pStream, WCHAR *text, LONG nChars)
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


static BOOL
ME_StreamOutRTF(ME_TextEditor *editor, ME_OutStream *pStream, int nStart, int nChars, int dwFormat)
{
  ME_DisplayItem *p, *pEnd, *pPara;
  int nOffset, nEndLen; 
  
  ME_RunOfsFromCharOfs(editor, nStart, &p, &nOffset);
  ME_RunOfsFromCharOfs(editor, nStart+nChars, &pEnd, &nEndLen);
  
  pPara = ME_GetParagraph(p);
  
  if (!ME_StreamOutRTFHeader(pStream, dwFormat))
    return FALSE;

  if (!ME_StreamOutRTFFontAndColorTbl(pStream, p, pEnd))
    return FALSE;
  
  /* TODO: stylesheet table */
  
  /* FIXME: maybe emit something smarter for the generator? */
  if (!ME_StreamOutPrint(pStream, "{\\*\\generator Wine Riched20 2.0.????;}"))
    return FALSE;

  /* TODO: information group */

  /* TODO: document formatting properties */

  /* FIXME: We have only one document section */

  /* TODO: section formatting properties */

  if (!ME_StreamOutRTFParaProps(pStream, ME_GetParagraph(p)))
    return FALSE;

  while(1)
  {
    switch(p->type)
    {
      case diParagraph:
        if (!ME_StreamOutRTFParaProps(pStream, p))
          return FALSE;
        pPara = p;
        break;
      case diRun:
        if (p == pEnd && !nEndLen)
          break;
        TRACE("flags %xh\n", p->member.run.nFlags);
        /* TODO: emit embedded objects */
        if (p->member.run.nFlags & MERF_GRAPHICS)
          FIXME("embedded objects are not handled\n");
        if (p->member.run.nFlags & MERF_CELL) {
          if (!ME_StreamOutPrint(pStream, "\\cell "))
            return FALSE;
          nChars--;
        } else if (p->member.run.nFlags & MERF_ENDPARA) {
          if (pPara->member.para.bTable) {
            if (!ME_StreamOutPrint(pStream, "\\row \r\n"))
              return FALSE;
          } else {
            if (!ME_StreamOutPrint(pStream, "\r\n\\par"))
              return FALSE;
          }
          nChars--;
          if (editor->bEmulateVersion10 && nChars)
            nChars--;
        } else {
          int nEnd;
          
          if (!ME_StreamOutPrint(pStream, "{"))
            return FALSE;
          TRACE("style %p\n", p->member.run.style);
          if (!ME_StreamOutRTFCharProps(pStream, &p->member.run.style->fmt))
            return FALSE;
        
          nEnd = (p == pEnd) ? nEndLen : ME_StrLen(p->member.run.strText);
          if (!ME_StreamOutRTFText(pStream, p->member.run.strText->szData + nOffset, nEnd - nOffset))
            return FALSE;
          nOffset = 0;
          if (!ME_StreamOutPrint(pStream, "}"))
            return FALSE;
        }
        break;
      default: /* we missed the last item */
        assert(0);
    }
    if (p == pEnd)
      break;
    p = ME_FindItemFwd(p, diRunOrParagraphOrEnd);
  }
  if (!ME_StreamOutPrint(pStream, "}"))
    return FALSE;
  return TRUE;
}


static BOOL
ME_StreamOutText(ME_TextEditor *editor, ME_OutStream *pStream, int nStart, int nChars, DWORD dwFormat)
{
  /* FIXME: use ME_RunOfsFromCharOfs */
  ME_DisplayItem *item = ME_FindItemAtOffset(editor, diRun, nStart, &nStart);
  int nLen;
  UINT nCodePage = CP_ACP;
  char *buffer = NULL;
  int nBufLen = 0;
  BOOL success = TRUE;

  if (!item)
    return FALSE;
   
  if (dwFormat & SF_USECODEPAGE)
    nCodePage = HIWORD(dwFormat);

  /* TODO: Handle SF_TEXTIZED */
  
  while (success && nChars && item) {
    nLen = ME_StrLen(item->member.run.strText) - nStart;
    if (nLen > nChars)
      nLen = nChars;

    if (item->member.run.nFlags & MERF_ENDPARA) {
      static const WCHAR szEOL[2] = { '\r', '\n' };
      
      if (dwFormat & SF_UNICODE)
        success = ME_StreamOutMove(pStream, (const char *)szEOL, sizeof(szEOL));
      else
        success = ME_StreamOutMove(pStream, "\r\n", 2);
    } else {
      if (dwFormat & SF_UNICODE)
        success = ME_StreamOutMove(pStream, (const char *)(item->member.run.strText->szData + nStart),
                                   sizeof(WCHAR) * nLen);
      else {
        int nSize;

        nSize = WideCharToMultiByte(nCodePage, 0, item->member.run.strText->szData + nStart,
                                    nLen, NULL, 0, NULL, NULL);
        if (nSize > nBufLen) {
          FREE_OBJ(buffer);
          buffer = ALLOC_N_OBJ(char, nSize);
          nBufLen = nSize;
        }
        WideCharToMultiByte(nCodePage, 0, item->member.run.strText->szData + nStart,
                            nLen, buffer, nSize, NULL, NULL);
        success = ME_StreamOutMove(pStream, buffer, nSize);
      }
    }
    
    nChars -= nLen;
    if (editor->bEmulateVersion10 && nChars && item->member.run.nFlags & MERF_ENDPARA)
      nChars--;
    nStart = 0;
    item = ME_FindItemFwd(item, diRun);
  }
  
  FREE_OBJ(buffer);
  return success;
}


LRESULT
ME_StreamOutRange(ME_TextEditor *editor, DWORD dwFormat, int nStart, int nTo, EDITSTREAM *stream)
{
  ME_OutStream *pStream = ME_StreamOutInit(editor, stream);

  if (nTo == -1)
  {
    nTo = ME_GetTextLength(editor);
    /* Generate an end-of-paragraph at the end of SCF_ALL RTF output */
    if (dwFormat & SF_RTF)
      nTo++;
  }
  TRACE("from %d to %d\n", nStart, nTo);

  if (dwFormat & SF_RTF)
    ME_StreamOutRTF(editor, pStream, nStart, nTo - nStart, dwFormat);
  else if (dwFormat & SF_TEXT || dwFormat & SF_TEXTIZED)
    ME_StreamOutText(editor, pStream, nStart, nTo - nStart, dwFormat);
  if (!pStream->stream->dwError)
    ME_StreamOutFlush(pStream);
  return ME_StreamOutFree(pStream);
}

LRESULT
ME_StreamOut(ME_TextEditor *editor, DWORD dwFormat, EDITSTREAM *stream)
{
  int nStart, nTo;

  if (dwFormat & SFF_SELECTION)
    ME_GetSelection(editor, &nStart, &nTo);
  else {
    nStart = 0;
    nTo = -1;
  }
  return ME_StreamOutRange(editor, dwFormat, nStart, nTo, stream);
}
