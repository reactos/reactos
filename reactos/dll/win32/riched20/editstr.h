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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winnt.h>
#include <wingdi.h>
#include <winuser.h>
#include <richedit.h>
#include <commctrl.h>
#include <ole2.h>
#include <richole.h>

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
  diInvalid,
  diTextStart, /* start of the text buffer */
  diParagraph, /* paragraph start */
  diCell, /* cell start */
  diRun, /* run (sequence of chars with the same character format) */
  diStartRow, /* start of the row (line of text on the screen) */
  diTextEnd, /* end of the text buffer */
  
  /********************* these below are meant for finding only *********************/
  diStartRowOrParagraph, /* 7 */
  diStartRowOrParagraphOrEnd,
  diRunOrParagraph,
  diRunOrStartRow,
  diParagraphOrEnd,
  diRunOrParagraphOrEnd, /* 12 */
  
  diUndoInsertRun, /* 13 */
  diUndoDeleteRun, /* 14 */
  diUndoJoinParagraphs, /* 15 */
  diUndoSplitParagraph, /* 16 */
  diUndoSetParagraphFormat, /* 17 */
  diUndoSetCharFormat, /* 18 */
  diUndoEndTransaction, /* 19 - marks the end of a group of changes for undo */
  diUndoPotentialEndTransaction, /* 20 - allows grouping typed chars for undo */
} ME_DIType;

#define SELECTIONBAR_WIDTH 9

/******************************** run flags *************************/
#define MERF_STYLEFLAGS 0x0FFF
/* run contains non-text content, which has its own rules for wrapping, sizing etc */
#define MERF_GRAPHICS   0x001
/* run is a tab (or, in future, any kind of content whose size is dependent on run position) */
#define MERF_TAB        0x002
/* run is a cell boundary */
#define MERF_ENDCELL    0x004 /* v4.1 */

#define MERF_NONTEXT (MERF_GRAPHICS | MERF_TAB | MERF_ENDCELL)

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
/* forcing the "end of row" run, contains 1 character */
#define MERF_ENDROW     0x200000
/* run is hidden */
#define MERF_HIDDEN     0x400000
/* start of a table row has an empty paragraph that should be skipped over. */
#define MERF_TABLESTART 0x800000 /* v4.1 */

/* runs with any of these flags set cannot be joined */
#define MERF_NOJOIN (MERF_GRAPHICS|MERF_TAB|MERF_ENDPARA|MERF_ENDROW)
/* runs that don't contain real text */
#define MERF_NOTEXT (MERF_GRAPHICS|MERF_TAB|MERF_ENDPARA|MERF_ENDROW)

/* those flags are kept when the row is split */
#define MERF_SPLITMASK (~(0))

/******************************** para flags *************************/

/* this paragraph was already wrapped and hasn't changed, every change resets that flag */
#define MEPF_REWRAP   0x01
#define MEPF_REPAINT  0x02
/* v4.1 */
#define MEPF_CELL     0x04 /* The paragraph is nested in a cell */
#define MEPF_ROWSTART 0x08 /* Hidden empty paragraph at the start of the row */
#define MEPF_ROWEND   0x10 /* Visible empty paragraph at the end of the row */

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
  REOBJECT *ole_obj; /* FIXME: should be a union with strText (at least) */
  int nCR; int nLF;  /* for MERF_ENDPARA: number of \r and \n characters encoded by run */
} ME_Run;

typedef struct tagME_Document {
  struct tagME_DisplayItem *def_char_style;
  struct tagME_DisplayItem *def_para_style;
  int last_wrapped_line;
} ME_Document;

typedef struct tagME_Border
{
  int width;
  COLORREF colorRef;
} ME_Border;

typedef struct tagME_BorderRect
{
  ME_Border top;
  ME_Border left;
  ME_Border bottom;
  ME_Border right;
} ME_BorderRect;

typedef struct tagME_Paragraph
{
  PARAFORMAT2 *pFmt;

  struct tagME_DisplayItem *pCell; /* v4.1 */
  ME_BorderRect border;

  int nCharOfs;
  int nFlags;
  POINT pt;
  int nHeight;
  int nLastPaintYPos, nLastPaintHeight;
  int nRows;
  struct tagME_DisplayItem *prev_para, *next_para, *document;
} ME_Paragraph;

typedef struct tagME_Cell /* v4.1 */
{
  int nNestingLevel; /* 0 for normal cells, and greater for nested cells */
  int nRightBoundary;
  ME_BorderRect border;
  POINT pt;
  int nHeight, nWidth;
  int yTextOffset; /* The text offset is caused by the largest top border. */
  struct tagME_DisplayItem *prev_cell, *next_cell, *parent_cell;
} ME_Cell;

typedef struct tagME_Row
{
  int nHeight;
  int nBaseline;
  int nWidth;
  int nLMargin;
  int nRMargin;
  POINT pt;
} ME_Row;

/* the display item list layout is like this:
 * - the document consists of paragraphs
 * - each paragraph contains at least one run, the last run in the paragraph
 *   is an end-of-paragraph run
 * - each formatted paragraph contains at least one row, which corresponds
 *   to a screen line (that's why there are no rows in an unformatted
 *   paragraph
 * - the paragraphs contain "shortcut" pointers to the previous and the next
 *   paragraph, that makes iteration over paragraphs faster 
 * - the list starts with diTextStart and ends with diTextEnd
 */

typedef struct tagME_DisplayItem
{
  ME_DIType type;
  struct tagME_DisplayItem *prev, *next;
  union {
    ME_Run run;
    ME_Row row;
    ME_Cell cell;
    ME_Paragraph para;
    ME_Document doc; /* not used */
    ME_Style *ustyle; /* used by diUndoSetCharFormat */
  } member;
} ME_DisplayItem;

typedef struct tagME_UndoItem
{
  ME_DisplayItem di;
  int nStart, nLen;
  int nCR, nLF;      /* used by diUndoSplitParagraph */
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

typedef enum {
  stPosition = 0,
  stWord,
  stLine,
  stParagraph,
  stDocument
} ME_SelectionType;

typedef struct tagME_FontTableItem {
  BYTE bCharSet;
  WCHAR *szFaceName;
} ME_FontTableItem;


#define STREAMIN_BUFFER_SIZE 4096 /* M$ compatibility */

struct tagME_InStream {
  EDITSTREAM *editstream;
  DWORD dwSize;
  DWORD dwUsed;
  char buffer[STREAMIN_BUFFER_SIZE];
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
  /* nNestingLevel = 0 means we aren't in a cell, 1 means we are in a cell,
   * an greater numbers mean we are in a cell nested within a cell. */
  UINT nNestingLevel;
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
  ME_TextBuffer *pBuffer;
  ME_Cursor *pCursors;
  int nCursors;
  SIZE sizeWindow;
  int nTotalLength, nLastTotalLength;
  int nHeight;
  int nUDArrowX;
  int nSequence;
  COLORREF rgbBackColor;
  HBRUSH hbrBackground;
  BOOL bCaretAtEnd;
  int nEventMask;
  int nModifyStep;
  ME_DisplayItem *pUndoStack, *pRedoStack, *pUndoStackBottom;
  int nUndoStackSize;
  int nUndoLimit;
  ME_UndoMode nUndoMode;
  int nParagraphs;
  int nLastSelStart, nLastSelEnd;
  ME_DisplayItem *pLastSelStartPara, *pLastSelEndPara;
  ME_FontCacheItem pFontCache[HFONT_CACHE_SIZE];
  int nZoomNumerator, nZoomDenominator;
  RECT rcFormat;
  BOOL bRedraw;
  BOOL bWordWrap;
  int nInvalidOfs;
  int nTextLimit;
  EDITWORDBREAKPROCW pfnWordBreak;
  LPRICHEDITOLECALLBACK lpOleCallback;
  /*TEXTMODE variable; contains only one of each of the following options:
   *TM_RICHTEXT or TM_PLAINTEXT
   *TM_SINGLELEVELUNDO or TM_MULTILEVELUNDO
   *TM_SINGLECODEPAGE or TM_MULTICODEPAGE*/
  int mode;
  BOOL bHideSelection;
  BOOL AutoURLDetect_bEnable;
  WCHAR cPasswordMask;
  BOOL bHaveFocus;
  /*for IME */
  int imeStartIndex;
  DWORD selofs; /* The size of the selection bar on the left side of control */
  ME_SelectionType nSelectionType;

  /* Track previous notified selection */
  CHARRANGE notified_cr;

  /* Cache previously set vertical scrollbar info */
  SCROLLINFO vert_si;
} ME_TextEditor;

typedef struct tagME_Context
{
  HDC hDC;
  POINT pt;
  POINT ptRowOffset;
  RECT rcView;
  HBRUSH hbrMargin;
  SIZE dpi;

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
  int nAvailWidth;
  int nRow;
  POINT pt;
  BOOL bOverflown;
  ME_DisplayItem *pRowStart;
  
  ME_DisplayItem *pLastSplittableRun;
  POINT ptLastSplittableRun;
} ME_WrapContext;  

#endif
