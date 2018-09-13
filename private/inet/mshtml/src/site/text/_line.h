/*
 *  _LINE.H
 *
 *  Purpose:
 *      CLine class
 *
 *  Authors:
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Murray Sargent
 *
 *  Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
 */

#ifndef I__LINE_H_
#define I__LINE_H_
#pragma INCMSG("--- Beg '_line.h'")

#ifndef X__RUNPTR_H_
#define X__RUNPTR_H_
#include "_runptr.h"
#endif

class CDisplay;
class CLSMeasurer;

enum JUSTIFY
{
    JUSTIFY_LEAD,
    JUSTIFY_CENTER,
    JUSTIFY_TRAIL,
    JUSTIFY_FULL
};

// ============================  CLine  =====================================
// line - keeps track of a line of text
// All metrics are in rendering device units

MtExtern(CLine)

class CLine : public CTxtRun
{
public:
    LONG    _xLeft;         // text line left position (line indent + line shift)
    LONG    _xWidth;        // text line width - does not include line left and
                            // trailing whitespace
    LONG    _xLeftMargin;   // margin on the left
    LONG    _xRightMargin;  // margin on the right
    LONG    _xLineWidth;    // width of line from margin to margin (possibly > view width)
    LONG    _yHeight;       // line height (amount y coord is advanced for this line).
    LONG    _yExtent;       // Actual extent of the line (before line-height style).
    LONG    _xRight;        // Right indent (for blockquotes).

#if !defined(MW_MSCOMPATIBLE_STRUCT)

    // Line flags.
    union
    {
        DWORD _dwFlagsVar;   // To access them all at once.
        struct
        {
#endif
            //
            unsigned int _fCanBlastToScreen : 1;
            unsigned int _fHasBulletOrNum : 1;    // Set if the line has a bullet
            unsigned int _fFirstInPara : 1;
            unsigned int _fForceNewLine : 1;      // line forces a new line (adds vertical space)

            //
            unsigned int _fLeftAligned : 1;       // line is left aligned
            unsigned int _fRightAligned : 1;      // line is right aligned
            unsigned int _fClearBefore : 1;       // clear line created by a line after the cur line(clear on a p)
            unsigned int _fClearAfter : 1;        // clear line created by a line after the cur line(clear on a br)

            //
            unsigned int _fHasAligned : 1;        // line contains a embeded char's for
            unsigned int _fHasBreak : 1;          // Specifies that the line ends in a break character.
            unsigned int _fHasEOP : 1;            // set if ends in paragraph mark
            unsigned int _fHasEmbedOrWbr : 1;     // has embedding or wbr char

            //
            unsigned int _fHasBackground : 1;     // has bg color or bg image
            unsigned int _fHasNBSPs : 1;          // has nbsp (might need help rendering)
            unsigned int _fHasNestedRunOwner : 1; // has runs owned by a nested element (e.g., a CTable)
            unsigned int _fHidden:1;              // Is this line hidden?

            //
            unsigned int _fEatMargin : 1;         // Line should act as bottom margin.
            unsigned int _fPartOfRelChunk : 1;    // Part of a relative line chunk
            unsigned int _fFrameBeforeText : 1;   // this means this frame belongs to the
                                                  // next line of text.
            unsigned int _fDummyLine : 1;         // dummy line

            //
            unsigned int _fHasAbsoluteElt : 1;    // has site(s) with position:absolute
            unsigned int _fAddsFrameMargin : 1;   // line adds frame margin space to adjoining lines
            unsigned int _fSingleSite : 1;        // Set if the line contains one of our
                                                  // sites that always lives on its own line,
                                                  // but still in the text stream.(like tables and HR's)
            unsigned int _fHasParaBorder : 1;     // TRUE if this line has a paragraph border around it.

            //
            unsigned int _fRelative : 1;          // relatively positioned line
            unsigned int _fFirstFragInLine : 1;   // first fragment or chunk of one screen line
            unsigned int _fRTL : 1;               // TRUE if the line has RTL direction orientation.
            unsigned int _fPageBreakBefore : 1;   // TRUE if this line has an element w/ page-break-before attribute

            //
            unsigned int _fPageBreakAfter   : 1;  // TRUE if this line has an element w/ page-break-after attribute
            unsigned int _fJustified        : 2;  // current line is justified.
                                                  // 00 - left/notset       -   01 - center justified
                                                  // 10 - right justified   -   11 - full justified

            unsigned short _fLookaheadForGlyphing : 1;  // We need to look beyond the current
                                                        // run to determine if glyphing is needed.

#if !defined(MW_MSCOMPATIBLE_STRUCT)
        };
    };

    DWORD& _dwFlags() { return _dwFlagsVar; }
#else

    DWORD& _dwFlags() { return *(DWORD*)(&_xRight + 1); }

#endif

#if !defined(MW_MSCOMPATIBLE_STRUCT)
    union
    {
        struct
        {
#endif
            SHORT   _yTxtDescent;   // max text descent for the line
            SHORT   _yDescent;      // distance from baseline to bottom of line

#if !defined(MW_MSCOMPATIBLE_STRUCT)
        };
        CTreeNode * _pNodeLayout;
    };

#else
    CTreeNode * _pNodeLayout;
#endif

    void    operator =(const CLine& li)
        { if(&li != this) memcpy(this, &li, sizeof(CLine)); }


    SHORT   _xLineOverhang; // Overhang for the line.
    SHORT   _cchWhite;      // number of white character at end of line
    SHORT   _xWhite;        // width of white characters at end of line
    SHORT   _yBeforeSpace;  // Pixels of extra space before the line.

    SHORT   _yBulletHeight; // Height of the bullet -- valid only for lines
                            //   having _fBulletOrNum = TRUE

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLine))

    VOID Init()
    {
        // !!!!! CLine should not have any virtual method !!!!!!
        ZeroMemory(this, sizeof(CLine));
    }

    LONG CchFromXpos(CLSMeasurer& me, LONG x, LONG y, LONG *pdx, BOOL fRTLDisplay,
                     BOOL fExactfit=FALSE, LONG *pyHeightRubyBase = NULL) const;

    BOOL IsLeftAligned() const  { return _fLeftAligned; }
    BOOL IsRightAligned() const { return _fRightAligned; }
    BOOL HasMargins() const     { return _xLeftMargin || _xRightMargin; }
    BOOL HasAligned() const     { return _fHasAligned; }
    BOOL IsFrame() const        { return _fRightAligned || _fLeftAligned; }
    BOOL IsClear() const        { return _fClearBefore  || _fClearAfter; }
    BOOL IsBlankLine() const    { return IsClear() || IsFrame(); }
    BOOL IsTextLine() const     { return !IsBlankLine(); }
    BOOL IsNextLineFirstInPara(){ return (_fHasEOP || (!_fForceNewLine && _fFirstInPara)); }
    BOOL IsRTL() const          { return _fRTL; }

    void ClearAlignment() { _fRightAligned = _fLeftAligned = FALSE; }
    void SetLeftAligned() { _fLeftAligned = TRUE; }
    void SetRightAligned() { _fRightAligned = TRUE; }

    LONG GetTextLeft() const { return _xLeftMargin + _xLeft; }

    LONG GetTextRight(BOOL fLastLine=FALSE) const { return (long(_fJustified) == JUSTIFY_FULL && !_fHasEOP && !_fHasBreak && !fLastLine
                                        ? _xLeftMargin + _xLineWidth - _xRight
                                        : _xLeftMargin + _xLeft + _xWidth + _xLineOverhang); }
    LONG GetRTLTextRight() const { return _xRightMargin + _xRight; }
    LONG GetRTLTextLeft() const { return (long(_fJustified) == JUSTIFY_FULL && !_fHasEOP && !_fHasBreak
                                        ? _xRightMargin + _xLineWidth - _xLeft
                                        : _xRightMargin + _xRight + _xWidth + _xLineOverhang); }

    // Amount to advance the y coordinate for this line.
    LONG GetYHeight() const
    {
        return _yHeight;
    }

    // Offset to add to top of line for hit testing.
    // This takes into account line heights smaller than natural.
    LONG GetYHeightTopOff() const
    {
        return (((_yHeight - _yBeforeSpace) - _yExtent) / 2);
    }

    // Offset to add to bottom of line for hit testing.
    LONG GetYHeightBottomOff() const
    {
        return (_yExtent - (_yHeight - _yBeforeSpace)) + GetYHeightTopOff();
    }

    // Total to add to the top of the line space to get the actual
    // top of the display part of the line.
    LONG GetYTop() const
    {
        return GetYHeightTopOff() + _yBeforeSpace;
    }

    LONG GetYBottom() const
    {
        return GetYHeight() + GetYHeightBottomOff();
    }

    LONG GetYMostTop() const
    {
        return min(GetYTop(), GetYHeight());
    }

    LONG GetYLineTop() const
    {
        return min(0L, GetYMostTop());
    }

    LONG GetYLineBottom() const
    {
        return max(GetYBottom(), _yHeight);
    }

    BOOL TryToKeepTogether();

    void RcFromLine(RECT & rcLine, LONG yTop)
    {
        rcLine.top      = yTop + GetYTop();
        rcLine.bottom   = yTop + GetYBottom();
        rcLine.left     = _xLeftMargin;
        rcLine.right    = _xLeftMargin + _xLineWidth;
    }
};


// ==========================  CLineArray  ===================================
// Array of lines

MtExtern(CLineArray)
MtExtern(CLineArray_pv)

class CLineArray : public CArray<CLine>
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CLineArray))
    CLineArray() : CArray<CLine>(Mt(CLineArray_pv)) {};
};

// ==========================  CLinePtr  ===================================
// Maintains position in a array of lines

MtExtern(CLinePtr)

class CLinePtr : public CRunPtr<CLine>
{
protected:
    CDisplay   *_pdp;

public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CLinePtr))
    CLinePtr (CDisplay *pdp) {Hijack(pdp);}
    CLinePtr (CLinePtr& rp) : CRunPtr<CLine> (rp)   {}

    void Init ( CLineArray & );

    CDisplay *GetPdp() { return _pdp;}

    // The new display hijack's this line ptr
    void    Hijack(CDisplay *pdp);

    // Alternate initializer
    void    RpSet(LONG iRun, LONG ich)  { CRunPtr<CLine>::SetRun(iRun, ich); }


    // Direct cast to a run index
    operator LONG() const { return GetIRun(); }

    // Get the run index (line number)
    LONG GetLineIndex () { return GetIRun(); }
    LONG GetAdjustedLineLength();


    CLine * operator -> ( ) const
    {
        return CurLine();
    }

    CLine * CurLine() const
    {
        return (CLine *)_prgRun->Elem( GetIRun() );
    }

    CLine & operator * ( ) const
    {
        return *((CLine *)_prgRun->Elem( GetIRun() ));
    }

    CLine & operator [ ] ( long dRun );

    BOOL    NextLine(BOOL fSkipFrame, BOOL fSkipEmptyLines); // skip frames
    BOOL    PrevLine(BOOL fSkipFrame, BOOL fSkipEmptyLines); // skip frames

    // Character position control
    LONG    RpGetIch ( ) const { return GetIch(); }
    BOOL    RpAdvanceCp(LONG cch, BOOL fSkipFrame = TRUE);
    BOOL    RpSetCp(LONG cp, BOOL fAtEnd, BOOL fSkipFrame = TRUE);
    LONG    RpBeginLine(void);
    LONG    RpEndLine(void);

    void RemoveRel (LONG cRun, ArrayFlag flag)
    {
        CRunPtr<CLine>::RemoveRel(cRun, flag);
    }

    BOOL Replace(LONG cRun, CLineArray *parLine)
    {
        return CRunPtr<CLine>::Replace(cRun,(CRunArray *)parLine);
    }

    // Assignment from a run index
    CRunPtrBase& operator =(LONG iRun) {SetRun(iRun, 0); return *this;}

    LONG    FindParagraph(BOOL fForward);

    // returns TRUE if the ptr is *after* the *last* character in the line
    BOOL IsAfterEOL() { return GetIch() == CurLine()->_cch; }

    BOOL IsLastTextLine();
};

#pragma INCMSG("--- End '_line.h'")
#else
#pragma INCMSG("*** Dup '_line.h'")
#endif
