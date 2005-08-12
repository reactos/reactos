/*
 * RichEdit - structures and constant
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __EDITSTR_H
#define __EDITSTR_H

#ifndef _WIN32_IE
#define _WIN32_IE 0x0400
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winnt.h>
#include <wingdi.h>
#include <winuser.h>
#include <richedit.h>
#include <commctrl.h>

#include "wine/debug.h"

typedef struct tagME_String
{
  WCHAR *szData;
  int nLen, nBuffer;
} ME_String;

typedef struct tagME_Style
{
  CHARFORMAT2W fmt;

  HFONT hFont; /* cached font for the style */
  TEXTMETRICW tm; /* cached font metrics for the style */
  int nRefs; /* reference count */
  int nSequence; /* incremented when cache needs to be rebuilt, ie. every screen redraw */
} ME_Style;

typedef enum {
  diTextStart, /* start of the text buffer */
  diParagraph, /* paragraph start */
  diRun, /* run (sequence of chars with the same character format) */
  diStartRow, /* start of the row (line of text on the screen) */
  diTextEnd, /* end of the text buffer */
  
  /********************* these below are meant for finding only *********************/
  diStartRowOrParagraph, /* 5 */
  diStartRowOrParagraphOrEnd,
  diRunOrParagraph,
  diRunOrStartRow,
  diParagraphOrEnd,
  diRunOrParagraphOrEnd, /* 10 */
  
  diUndoInsertRun, /* 11 */
  diUndoDeleteRun, /* 12 */
  diUndoJoinParagraphs, /* 13 */
  diUndoSplitParagraph, /* 14 */
  diUndoSetParagraphFormat, /* 15 */
  diUndoSetCharFormat, /* 16 */
  diUndoEndTransaction, /* 17 */
  diUndoSetDefaultCharFormat, /* 18 */
} ME_DIType;

/******************************** run flags *************************/
#define MERF_STYLEFLAGS 0x0FFF
/* run contains non-text content, which has its own rules for wrapping, sizing etc */
#define MERF_GRAPHICS 1
/* run is a tab (or, in future, any kind of content whose size is dependent on run position) */
#define MERF_TAB 2

/* run is splittable (contains white spaces in the middle or end) */
#define MERF_SPLITTABLE 0x001000
/* run starts with whitespaces */
#define MERF_STARTWHITE 0x002000
/* run ends with whitespaces */
#define MERF_ENDWHITE   0x004000
/* run is completely made of whitespaces */
#define MERF_WHITESPACE 0x008000
/* run is a last (dummy) run in the paragraph */
#define MERF_SKIPPED    0x010000
/* flags that are calculated during text wrapping */
#define MERF_CALCBYWRAP 0x0F0000
/* the "end of paragraph" run, contains 1 character */
#define MERF_ENDPARA    0x100000

/* runs with any of these flags set cannot be joined */
#define MERF_NOJOIN (MERF_GRAPHICS|MERF_TAB|MERF_ENDPARA)
/* runs that don't contain real text */
#define MERF_NOTEXT (MERF_GRAPHICS|MERF_TAB|MERF_ENDPARA)

/* those flags are kept when the row is split */
#define MERF_SPLITMASK (~(0))

/******************************** para flags *************************/

/* this paragraph was already wrapped and hasn't changed, every change resets that flag */
#define MEPF_REWRAP 1
#define MEPF_REPAINT 2

/******************************** structures *************************/

struct tagME_DisplayItem;

typedef struct tagME_Run
{
  ME_String *strText;
  ME_Style *style;
  int nCharOfs; /* relative to para's offset */
  int nWidth; /* width of full run, width of leading&trailing ws */
  int nFlags;
  int nAscent, nDescent; /* pixels above/below baseline */
  POINT pt; /* relative to para's position */
} ME_Run;

typedef struct tagME_Document {
  struct tagME_DisplayItem *def_char_style;
  struct tagME_DisplayItem *def_para_style;
  int last_wrapped_line;
} ME_Document;

typedef struct tagME_Paragraph
{
  PARAFORMAT2 *pFmt;
  int nLeftMargin, nRightMargin, nFirstMargin;
  int nCharOfs;
  int nFlags;
  int nYPos, nHeight;
  int nLastPaintYPos, nLastPaintHeight;
  int nRows;
  struct tagME_DisplayItem *prev_para, *next_para, *document;
} ME_Paragraph;

typedef struct tagME_Row
{
  int nHeight;
  int nBaseline;
  int nWidth;
  int nLMargin;
  int nRMargin;
  int nYPos;
} ME_Row;

typedef struct tagME_DisplayItem
{
  ME_DIType type;
  struct tagME_DisplayItem *prev, *next;
  union {
    ME_Run run;
    ME_Row row;
    ME_Paragraph para;
    ME_Document doc; /* not used */
    ME_Style *ustyle; /* used by diUndoSetCharFormat */
  } member;
} ME_DisplayItem;

typedef struct tagME_UndoItem
{
  ME_DisplayItem di;
  int nStart, nLen;
} ME_UndoItem;

typedef struct tagME_TextBuffer
{
  ME_DisplayItem *pFirst, *pLast;
  ME_Style *pCharStyle;
  ME_Style *pDefaultStyle;
} ME_TextBuffer;

typedef struct tagME_Cursor
{
  ME_DisplayItem *pRun;
  int nOffset;
} ME_Cursor;

typedef enum {
  umAddToUndo,
  umAddToRedo,
  umIgnore,
  umAddBackToUndo
} ME_UndoMode;

typedef struct tagME_FontTableItem {
  BYTE bCharSet;
  WCHAR *szFaceName;
} ME_FontTableItem;


#define STREAMIN_BUFFER_SIZE 4096 /* M$ compatibility */

struct tagME_InStream {
  EDITSTREAM *editstream;
  DWORD dwSize;
  DWORD dwUsed;
  BYTE buffer[STREAMIN_BUFFER_SIZE];
};
typedef struct tagME_InStream ME_InStream;


#define STREAMOUT_BUFFER_SIZE 4096
#define STREAMOUT_FONTTBL_SIZE 8192
#define STREAMOUT_COLORTBL_SIZE 1024

typedef struct tagME_OutStream {
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
} ME_OutStream;

typedef struct tagME_FontCacheItem
{
  LOGFONTW lfSpecs;
  HFONT hFont;
  int nRefs;
  int nAge;
} ME_FontCacheItem;

#define HFONT_CACHE_SIZE 10

typedef struct tagME_TextEditor
{
  HWND hWnd;
  BOOL bEmulateVersion10;
  BOOL bCaretShown;
  ME_TextBuffer *pBuffer;
  ME_Cursor *pCursors;
  int nCursors;
  SIZE sizeWindow;
  int nTotalLength, nLastTotalLength;
  int nUDArrowX;
  int nSequence;
  int nOldSelFrom, nOldSelTo;
  COLORREF rgbBackColor;
  BOOL bCaretAtEnd;
  int nEventMask;
  int nModifyStep;
  ME_DisplayItem *pUndoStack, *pRedoStack;
  ME_UndoMode nUndoMode;
  int nParagraphs;
  int nLastSelStart, nLastSelEnd;
  ME_FontCacheItem pFontCache[HFONT_CACHE_SIZE];
  ME_OutStream *pStream;
  BOOL bScrollX, bScrollY;
  int nScrollPosY;
  int nZoomNumerator, nZoomDenominator;
} ME_TextEditor;

typedef struct tagME_Context
{
  HDC hDC;
  POINT pt;
  POINT ptRowOffset;
  RECT rcView;
  HBRUSH hbrMargin;

  /* those are valid inside ME_WrapTextParagraph and related */
  POINT ptFirstRun;
  ME_TextEditor *editor;
  int nSequence;
} ME_Context;

typedef struct tagME_WrapContext
{
  ME_Style *style;
  ME_Context *context;
  int nLeftMargin, nRightMargin, nFirstMargin;
  int nTotalWidth, nAvailWidth;
  int nRow;
  POINT pt;
  BOOL bOverflown;
  ME_DisplayItem *pRowStart;
  
  ME_DisplayItem *pLastSplittableRun;
  POINT ptLastSplittableRun;
} ME_WrapContext;  

#endif
