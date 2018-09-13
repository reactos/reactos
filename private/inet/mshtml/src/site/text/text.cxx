/*
 *  @doc    INTERNAL
 *
 *  @module TEXT.C -- CTxtPtr implementation |
 *
 *  Authors: <nl>
 *      Original RichEdit code: David R. Fulmer <nl>
 *      Christian Fortini <nl>
 *      Murray Sargent <nl>
 *
 *  History: <nl>
 *      6/25/95     alexgo  cleanup and reorganization (use run pointers now)
 *
 *  @todo
 *      On deactivating, collapse buffer gap.  If no text, _prgRun should be
 *      set to point at a global null array.
 *
 *  Copyright (c) 1995-1996, Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifdef WIN16
#ifndef X_STRTYPE_HXX_
#define X_STRTYPE_HXX_
#include "strtype.hxx"
#endif
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include <intl.hxx>
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TREEPOS_H_
#define X_TREEPOS_H_
#include "treepos.hxx"
#endif

#ifndef X_UNIWBK_H_
#define X_UNIWBK_H_
#include "uniwbk.h"
#endif

#ifndef X_VRSSCAN_H_
#define X_VRSSCAN_H_
#include "vrsscan.h"
#endif

MtDefine(CTxtPtr, Tree, "CTxtPtr")
MtDefine(CTxtPtrFindThaiTypeWordBreak_aryNodePos_pv, Locals, "CTxtPtr::FindThaiTypeWordBreak::aryNodePos::_pv")
MtDefine(CTxtPtrMoveCluster_aryNodePos_pv, Locals, "CTxtPtr::MoveCluster::aryNodePos::_pv")
MtDefine(CTxtPtrItemizeAndBreakRun_aryItems_pv, Locals, "CTxtPtr::ItemizeAndBreakRun::aryItems::_pv")

extern CCriticalSection g_csJitting;
extern BYTE g_bUSPJitState;

// NB (cthrash) Our partners in Korea have requested that ideographs be
// selected differently in Korean mode.  Turn this debug tag on to force
// Trident into Korean seleciton mode.

DeclareTag( tagKoreanSelection, "TextSelection", "Korean Selection Mode.");

DeclareTag( tagOneCharTextInsert, "TextInsert", "Insert text one character at a time");

DeclareTag( tagUrlDetection, "UrlDetection", "trace Url Auto Detection");

// internal functions
// Text Block management
void TxDivideInsertion(DWORD cch, DWORD ichBlock, DWORD cchAfter,
            DWORD *pcchFirst, DWORD *pcchLast);


//
// Node classifications.
//
enum NODE_CLASS
{
    NODECLASS_NONE = 0,
    NODECLASS_SEPARATOR,
    NODECLASS_NOSCOPE,      
    NODECLASS_LINEBREAK,
    NODECLASS_BLOCKBREAK,
    NODECLASS_SITEBREAK,
};

NODE_CLASS
ClassifyNodePos( CTreePos *ptp, BOOL * pfBegin )
{
    CTreeNode   * pNode;
    CFlowLayout * pFL;
    CElement    * pElement;
    ELEMENT_TAG   etag;
    NODE_CLASS    ncClass = NODECLASS_NONE;

    Assert( ptp->IsNode() );

    if( ptp->IsEdgeScope() )
    {
        pNode = ptp->Branch();
        pFL = pNode->GetFlowLayout();
        pElement = pNode->Element();
        etag = (ELEMENT_TAG) pElement->_etag;

        if( pFL && pFL->IsElementBlockInContext( pElement ) )
        {
            if(    etag == ETAG_TABLE
                || etag == ETAG_TEXTAREA
#ifdef  NEVER
                || etag == ETAG_HTMLAREA
#endif
              )
            {
                ncClass = NODECLASS_SITEBREAK;
            } 
            else if(    etag == ETAG_TD
                     || etag == ETAG_TH 
                     || etag == ETAG_TC )
            {
                ncClass = NODECLASS_SITEBREAK;
            }
            else
            {
                ncClass = NODECLASS_BLOCKBREAK;
            }
        }
        else if (pElement->IsNoScope())
        {
            if( etag == ETAG_BR )
            {
                ncClass = NODECLASS_LINEBREAK;
            }
            else if (pNode->HasLayout())
            {
                ncClass = NODECLASS_NOSCOPE;
            }
        }
        else if ( etag == ETAG_RT || etag == ETAG_RP )
        {
            ncClass = NODECLASS_SEPARATOR;
        }

        if( pfBegin )
        {
            *pfBegin = ptp->IsBeginNode();

            // Table cell boundaries are sort of reverse no-scopes.
            if( etag == ETAG_TD || etag == ETAG_TH || etag == ETAG_TC )
                *pfBegin = !*pfBegin;
        }

    }

    return ncClass;
}


/*
 * Classify
 *
 * Synopsis: Classifies the node correpsonding to the character just
 * after _cp.
 *
 * Returns:
 *  NODECLASS_NONE: "Uninteresting" node, such as <B>s, <I>s, etc.
 *  NODECLASS_SEPARATOR: Words separation, such as <RT>s
 *  NODECLASS_NOSCOPE: Nodes with no scope, such as <IMG>s
 *  NODECLASS_LINEBREAK: Line breaks, such as <BR>s
 *  NODECLASS_BLOCKBREAK: Nodes that cause breaks, such as <P>s
 *  NODECLASS_SITEBREAK: Things like <TABLE>s, <TEXTAREA>s
 */

static NODE_CLASS
Classify( CTxtPtr * ptp, BOOL * pfBegin )
{
    Assert( ptp->GetChar() == WCH_NODE );

    return ClassifyNodePos( ptp->_pMarkup->TreePosAtCp( ptp->GetCp(), NULL ), pfBegin );
}

/*
 *  IsEOP(ch)
 *
 *  @func
 *      Used to determine if ch is an EOP char, i.e., CR, LF, VT, FF, PS, or
 *      LS (Unicode paragraph/line separator). This function (or its CR/LF
 *      subset) is often inlined, but is useful if speed isn't critical.
 *
 *  @devnote
 *      It is very important that ch be unsigned, since to determine if ch is
 *      in a range, we need an unsigned compare.  In this way values below
 *      range get mapped into very large unsigned values that have their
 *      sign bit set.
 */
BOOL IsEOP(TCHAR ch)
{
    // NB: ch must be unsigned; TCHAR gives the wrong result!
    return IsASCIIEOP(ch) || ((ch | 1) == PS);
}

/*
 *  IsLaunderChar(ch)
 *
 *  @func
 *      Is the ch a character we launder when we launder spaces?
 */
BOOL
IsLaunderChar(TCHAR ch)
{
    return    _T(' ')  == ch
           || _T('\t') == ch
           || WCH_NBSP == ch;
}

/*
 *  IsWhiteSpace(CTxtPtr *)
 *
 *  @func
 *      Used to determine if ch is an EOP char (see IsEOP() for definition),
 *      TAB or blank. This function is used in identifying sentence start
 *      and end.
 *
 *  @devnote
 *      It is very important that ch be unsigned, since to determine if ch is
 *      in a range (from TAB (= 9) to CR (= 13), we need an unsigned compare.
 *      In this way values below range get mapped into very large unsigned
 *      values that have their sign bit set.
 */
BOOL IsWhiteSpace(CTxtPtr *ptp)
{
    TCHAR ch = ptp->GetChar();

    return (    ch == L' '
             || InRange( ch, TAB, CR )
             || (ch | 1) == PS 
             || (    ch == WCH_NODE 
                  && Classify( ptp, NULL ) ) );
}

/*
 *  IsSentenceTerminator(ch)
 *
 *  @func
 *      Used to determine if ch is a standard sentence terminator character,
 *      namely, '?', '.', or '!'
 */
BOOL IsSentenceTerminator(TCHAR ch)
{
    return ch <= '?' && (ch == '?' || ch == '.' || ch == '!');
}


// ===========================  Invariant stuff  ==================================================

#define DEBUG_CLASSNAME CTxtPtr
#include "_invar.h"

// ===============================  CTxtPtr  ======================================================

#if DBG==1

/*
 *  CTxtPtr::Invariant
 *
 *  @mfunc  invariant check
 */
BOOL CTxtPtr::Invariant( ) const
{
    static LONG numTests = 0;
    numTests++;             // how many times we've been called.

    // make sure _cp is within range.
    Assert(_cp >= 0 );

    CRunPtrBase::Invariant();

    if( _prgRun )
    {
        // we use less than or equals here so that we can be an insertion
        // point at the *end* of the currently existing text (cpLim).
        Assert(long(_cp) <= GetTextLength());

        // make sure all the blocks are consistent...
//        ((CTxtArray *)_prgRun)->Invariant();
    }

    Assert( _cp == CRunPtrBase::GetCp() );

    return TRUE;
}

/*
 *  CTxtPtr::MoveGapToEndOfBlock ()
 *
 *  @mfunc
 *      Function to move buffer gap to current block end to aid in debugging
 */
void CTxtPtr::MoveGapToEndOfBlock () const
{
    CTxtBlk *ptb = GetCurrRun();
    ptb->MoveGap(ptb->_cch);                // Move gaps to end of cur block
}

#endif


/*
 *  CTxtPtr::CTxtPtr(ped, cp)
 *
 *  @mfunc  constructor
 */
CTxtPtr::CTxtPtr (
    CMarkup *pMarkup,   //@parm Ptr to CMarkup instance
    DWORD cp)           //@parm cp to set the pointer to
{
    Reinit(pMarkup, cp);
}

/*
 *  CTxtPtr::CTxtPtr(&tp)
 *
 *  @mfunc  Copy Constructor
 */
CTxtPtr::CTxtPtr (
    const CTxtPtr &tp)
{
    // copy all the values over
    *this = tp;
}

/*
 *  CTxtPtr::Reinit(ped, cp)
 *
 *  @mfunc  reinitializes txt ptr just like constructor
 */
void CTxtPtr::Reinit (
    CMarkup *pMarkup,   //@parm Ptr to CMarkup instance
    DWORD cp)           //@parm cp to set the pointer to
{
    _pMarkup = pMarkup;
    _cp = 0;
    SetRunArray((CRunArray *) &pMarkup->_TxtArray);
    _cp = BindToCp(cp);
}

/*
 *  CTxtPtr::GetChar()
 *
 *  @mfunc
 *      Return character at this text pointer, NULL if text pointer is at
 *      end of text
 *
 *  @rdesc
 *      Character at this text ptr
 */
TCHAR CTxtPtr::GetChar()
{
    LONG         cchValid;
    const TCHAR *pch = GetPch(cchValid);

    return ( pch && cchValid ) ? *pch : 0;
}

/*
 *  CTxtPtr::GetPrevChar()
 *
 *  @mfunc
 *      Return character just before this text pointer, NULL if text pointer
 *      beginning of text
 *
 *  @rdesc
 *      Character just before this text ptr
 */
TCHAR CTxtPtr::GetPrevChar()
{
    long cchValid;
    const TCHAR * pch = GetPchReverse( cchValid );

    return (pch && cchValid) ? *(pch - 1) : 0;
}

/*
 *  CTxtPtr::GetPch(&cchValid)
 *
 *  @mfunc
 *      return a character pointer to the text at this text pointer
 *
 *  @rdesc
 *      a pointer to an array of characters.  May be NULL.  If non-null,
 *      then cchValid is guaranteed to be at least 1
 */
const TCHAR * CTxtPtr::GetPch(
    long &  cchValid)       //@parm Count of characters for which the
{
    _TEST_INVARIANT_

    DWORD       ich = GetIch();
    TCHAR *     pchBase;
    CTxtBlk *   ptb = _prgRun ? GetCurrRun() : NULL;

    if( !ptb )
    {
        cchValid = 0;
        return NULL;
    }

    // if we're at the edge of a run, grab the next run or
    // stay at the current run.

    if( GetIch() == ptb->_cch )
    {
        if( GetIRun() < NumRuns() - 1 )
        {
            // set us to the next text block
            ptb = GetNextRun();
            ich = 0;
        }
        else
        {
            //we're at the very end of the text, just return NULL
            cchValid = 0;
            return NULL;
        }
    }

    AssertSz(CbOfCch(ich) <= ptb->_cbBlock,
        "CTxtPtr::GetPch(): _ich bigger than block");

    pchBase = ptb->_pch + ich;


    // Check to see if we need to skip over gap.  Recall that
    // the game may come anywhere in the middle of a block,
    // so if the current ich (note, no underscore, we want
    // the active ich) is beyond the gap, then recompute pchBase
    // by adding in the size of the block.
    //
    // cchValid will then be the number of characters left in
    // the text block (or _cch - ich)

    if(CbOfCch(ich) >= ptb->_ibGap)
    {
        pchBase += CchOfCb(ptb->_cbBlock) - ptb->_cch;
        cchValid = ptb->_cch - ich;
    }
    else
    {
        //we're valid until the buffer gap (or see below).
        cchValid = CchOfCb(ptb->_ibGap) - ich;
    }

    Assert(cchValid);
    return pchBase;
}

/*
 *  CTxtPtr::GetPchReverse(&cchValidReverse, *pcchValid)
 *
 *  @mfunc
 *      return a character pointer to the text at this text pointer
 *      adjusted so that there are some characters valid *behind* the
 *      pointer.
 *
 *  @rdesc
 *      a pointer to an array of characters.  May be NULL.  If non-null,
 *      then cchValidReverse is guaranteed to be at least 1
 */
const TCHAR * CTxtPtr::GetPchReverse(
    long &  cchValidReverse,        //@parm length for reverse
    long *  pcchValid)              //@parm length forward
{
    _TEST_INVARIANT_

    LONG        cchTemp;
    DWORD       ich = GetIch();
    TCHAR *     pchBase;
    CTxtBlk *   ptb = GetCurrRun();

    if( !ptb )
    {
        cchValidReverse = 0;
        return NULL;
    }

    // if we're at the edge of a run, grab the previous run or
    // stay at the current run.

    if( !GetIch() )
    {
        if( GetIRun() )
        {
            // set us to the next text block
            ptb = GetPrevRun();
            ich = ptb->_cch;
        }
        else
        {
            //we're at the very beginning of the text, just return NULL
            cchValidReverse = 0;
            return NULL;
        }
    }

    AssertSz(CbOfCch(ich) <= ptb->_cbBlock,
        "CTxtPtr::GetPchReverse(): _ich bigger than block");

    pchBase = ptb->_pch + ich;

    // Check to see if we need to skip over gap.  Recall that
    // the game may come anywhere in the middle of a block,
    // so if the current ich (note, no underscore, we want
    // the active ich) is at least one char past the gap, then recompute
    // pchBase by adding the size of the gap (so that it's after
    // the gap).  This differs from GetPch(), which works forward and
    // wants pchBase to include the gap size if ich is at the gap, let
    // alone one or more chars past it.
    //
    // Also figure out the count of valid characters.  It's
    // either the count of characters from the beginning of the
    // text block, i.e. ich, or the count of characters from the
    // end of the buffer gap.

    cchValidReverse = ich;                  // Default for ich <= gap offset
    cchTemp = ich - CchOfCb(ptb->_ibGap);   // Calculate displacement
    if(cchTemp > 0)                         // Positive: pchBase is after gap
    {
        cchValidReverse = cchTemp;
        pchBase += CchOfCb(ptb->_cbBlock) - ptb->_cch;  // Add in gap size
    }
    if ( pcchValid )                         // if client needs forward length
    {
        if ( cchTemp > 0 )
            cchTemp = ptb->_cch - ich;
        else
            cchTemp = -cchTemp;

        *pcchValid = cchTemp;
    }

    return pchBase;
}

/*
 *  CTxtPtr::BindToCp(cp)
 *
 *  @mfunc
 *      set cached _cp = cp (or nearest valid value)
 *
 *  @rdesc
 *      _cp actually set
 *
 *  @comm
 *      This method overrides CRunPtrBase::BindToCp to keep _cp up to date
 *      correctly.
 *
 *  @devnote
 *      Do *not* call this method when high performance is needed; use
 *      AdvanceCp() instead, which advances from 0 or from the cached
 *      _cp, depending on which is closer.
 */
DWORD
CTxtPtr::BindToCp ( DWORD cp )
{
    if(_prgRun)
    {
        //
        // Special case binding to the end of the string to be fast
        //
        if (NumRuns() > 0 && long(cp) == GetTextLength())
        {
            SetIRun( NumRuns() - 1 );

            SetIch( _prgRun->Elem( GetIRun() )->_cch );
            _cp = cp;
        }
        else
        {
            _cp = CRunPtrBase::BindToCp(cp);
        }
    }

    // We want to be able to use this routine to fix up things so we don't do
    // the invariant checking at entry.

    _TEST_INVARIANT_

    return _cp;
}


/*
 *  CTxtPtr::SetCp(cp)
 *
 *  @mfunc
 *      'efficiently' sets cp by advancing from current position or from 0,
 *      depending on which is closer
 *
 *  @rdesc
 *      cp actually set to
 */
DWORD CTxtPtr::SetCp(
    DWORD   cp)     //@parm char position to set to
{
    LONG    cch = (LONG)cp - (LONG)_cp;
    AdvanceCp(cch);

    return _cp;
}

/*
 *  CTxtPtr::AdvanceCp(cch)
 *
 *  @mfunc
 *      Advance cp by cch characters
 *
 *  @rdesc
 *      Actual number of characters advanced by
 *
 *  @comm
 *      We override CRunPtrBase::AdvanceCp so that the cached _cp value
 *      can be correctly updated and so that the advance can be made
 *      from the cached _cp or from 0, depending on which is closer.
 *
 *  @devnote
 *      It's also easy to bind at the end of the story. So an improved
 *      optimization would bind there if 2*(_cp + cch) > _cp + text length.
 */
LONG CTxtPtr::AdvanceCp(
    LONG cch)           // @parm count of chars to advance by
{
    if (!IsValid())
        return 0;

    const LONG  cpSave = _cp;               // Save entry _cp
    LONG        cp = cpSave + cch;          // Requested target cp (maybe < 0)

    if(2*cp < cpSave)                       // Closer to 0 than cached cp
    {
        cp = max(cp, 0L);                    // Don't undershoot
        _cp = CRunPtrBase::BindToCp(cp);
    }
    else
        _cp += CRunPtrBase::AdvanceCp(cch); //  exist

    // NB! the invariant check needs to come at the end; we may be
    // moving 'this' text pointer in order to make it valid again
    // (for the floating range mechanism).

    _TEST_INVARIANT_
    return _cp - cpSave;                    // cch this CTxtPtr moved
}

/*
 *  CTxtPtr::GetText(cch, pch)
 *
 *  @mfunc
 *      get a range of cch characters starting at this text ptr. A literal
 *      copy is made, i.e., with no CR -> CRLF and WCH_EMBEDDING -> ' '
 *      translations.  For these translations, see CTxtPtr::GetPlainText()
 *
 *  @rdesc
 *      count of characters actually copied
 *
 *  @comm
 *      Doesn't change this text ptr
 */

long
CTxtPtr::GetRawText(
    long   cch,          //@parm count of characters to get
    TCHAR *pch)         //@parm buffer to copy the text into
{
    const TCHAR *pchRead;
    long  cchValid;
    long  cchCopy = 0;
    CTxtPtr tp(*this);

    _TEST_INVARIANT_

    // simply take our clone text pointer, and read valid blocks
    // of text until we've either read all the requested text or
    // we've run out of text to read.
    while( cch )
    {
        pchRead = tp.GetPch( cchValid );

        if (!pchRead)
            break;

        cchValid = min( cchValid, cch );
        CopyMemory( pch, pchRead, cchValid * sizeof( TCHAR ) );
        pch += cchValid;
        cch -= cchValid;
        cchCopy += cchValid;
        tp.AdvanceCp( cchValid );

        _TEST_INVARIANT_ON( tp );
    }

    return cchCopy;
}

long
CTxtPtr::GetPlainTextLength ( long cch )
{
    long cchCopy = 0;
    CTxtPtr tp( * this );

    _TEST_INVARIANT_

    while ( cch > 0 )
    {
        long cchValid;
        const TCHAR * pchRead = tp.GetPch( cchValid );

        if (!pchRead || cchValid <= 0)
            break;

        cchValid = min( cchValid, cch );
        cchCopy += cchValid;
        cch -= cchValid;

        //
        // Look for synthetic break chars, and count two chars for them,
        // for we will turn them into \r\n later.
        //

        for ( long i = 0 ; i < cchValid ; i++ )
        {
            TCHAR ch = pchRead[i];
            
            if (ch == _T('\r'))
                cchCopy++;
            else if (ch == WCH_NODE)
                cchCopy--;
        }

        tp.AdvanceCp( cchValid );
    }

    return cchCopy;
}

/*
 *  CTxtPtr::GetPlainText(cchBuff, pch, cpMost, fTextize)
 *
 *  @mfunc
 *      Copy up to cchBuff characters or up to cpMost, whichever comes
 *      first, translating lone CRs into CRLFs.  Move this text ptr just
 *      past the last character processed.  If fTextize, copy up to but
 *      not including the first WCH_EMBEDDING char. If not fTextize,
 *      replace WCH_EMBEDDING by a blank since RichEdit 1.0 does.
 *
 *  @rdesc
 *      Count of characters copied
 *
 *  @comm
 *      An important feature is that this text ptr is moved just past the
 *      last char copied.  In this way, the caller can conveniently read
 *      out plain text in bufferfuls of up to cch chars, which is useful for
 *      stream I/O.  This routine won't copy the final CR even if cpMost
 *      is beyond it.
 */

long
CTxtPtr::GetPlainText( long cch, TCHAR * pch )
{
    const TCHAR * pchRead;
    long          cchValid;
    CTxtPtr       tp ( * this );
    TCHAR *       pchStart = pch;
    TCHAR *       pchEnd = pch + cch;

    _TEST_INVARIANT_

    // simply take our clone text pointer, and read valid blocks
    // of text until we've either read all the requested text or
    // we've run out of text to read.

    // Assume cch is the count of characters *including* extra \r\n insertions
            
    while ( pch < pchEnd )
    {
        pchRead = tp.GetPch( cchValid );

        if (!pchRead)
            break;

        const TCHAR * pchReadEnd = pchRead + cchValid;
        
        for ( ; pchRead < pchReadEnd && pch < pchEnd ; pchRead++ )
        {
            TCHAR ch = *pchRead;
            
            if (ch == _T('\r'))
            {
#ifdef WIN16
                if (*(pchRead + 1) != _T('\n'))
#endif
                {
                    *pch++ = _T('\r');
                    
                    Assert( pch < pchEnd );
                    
                    *pch++ = _T('\n');
                }
            }
            else if (ch != WCH_NODE)
            {
                *pch++ = ch;
            }
        }

        if (pch < pchEnd)
        {
            tp.AdvanceCp( cchValid );

            _TEST_INVARIANT_ON( tp );
        }
    }

    return pch - pchStart;
}

/*
 *  CTxtPtr::AdvanceCpCRLF()
 *
 *  @mfunc
 *      Advance text pointer by one character, safely advancing
 *      over CRLF, CRCRLF, and UTF-16 combinations
 *
 *  @rdesc
 *      Number of characters text pointer has been moved by
 *
 *  @future
 *      Advance over Unicode combining marks
 */
LONG CTxtPtr::AdvanceCpCRLF()
{
    _TEST_INVARIANT_

    long    cpSave  = _cp;
    TCHAR   ch      = GetChar();        // Char on entry
    TCHAR   ch1     = NextChar();       // Advance to and get next char
    BOOL    fTwoCRs = FALSE;

    if (ch == CR)
    {
        if(ch1 == CR && cpSave <
            (GetTextLength() - long(cchCRLF)))
        {
            fTwoCRs = TRUE;             // Need at least 3 chars to go
            ch1 = NextChar();           //  to have CRCRLF at end
        }
        if(ch1 == LF)
            AdvanceCp(1);               // Bypass CRLF
        else if(fTwoCRs)
            AdvanceCp(-1);              // Only bypass one CR of two
    }

//  To support UTF-16, include the following code
//  if ((ch & UTF16) == UTF16_TRAIL)    // Landed on UTF-16 trail word
//      AdvanceCp(1);                   // Bypass UTF-16 trail word

    return _cp - cpSave;                // # chars bypassed
}

/*
 *  CTxtPtr::NextChar()
 *
 *  @mfunc
 *      Increment this text ptr and return char it points at
 *
 *  @rdesc
 *      Next char
 */
TCHAR CTxtPtr::NextChar()
{
    _TEST_INVARIANT_

    AdvanceCp(1);
    return GetChar();
}

/*
 *  CTxtPtr::PrevChar()
 *
 *  @mfunc
 *      Decrement this text ptr and return char it points at
 *
 *  @rdesc
 *      Previous char
 */
TCHAR CTxtPtr::PrevChar()
{
    _TEST_INVARIANT_

    return AdvanceCp(-1) ? GetChar() : 0;
}

/*
 *  CTxtPtr::BackupCpCRLF()
 *
 *  @mfunc
 *      Backup text pointer by one character, safely backing up
 *      over CRLF, CRCRLF, and UTF-16 combinations
 *
 *  @rdesc
 *      Number of characters text pointer has been moved by
 *
 *  @future
 *      Backup over Unicode combining marks
 */
LONG CTxtPtr::BackupCpCRLF()
{
    _TEST_INVARIANT_

    DWORD cpSave = _cp;
    TCHAR ch     = PrevChar();          // Advance to and get previous char

    if (ch == LF &&                     // Try to back up 1 char in any case
        (_cp && PrevChar() != CR ||     // If LF, does prev char = CR?
         _cp && PrevChar() != CR))      // If so, does its prev char = CR?
    {                                   //  (CRLF at BOD checks CR twice; OK)
        AdvanceCp(1);                   // Backed up too far
    }
//  To support UTF-16, include the following code
//  if ((ch & UTF16) == UTF16_TRAIL)    // Landed on UTF-16 trail word
//      AdvanceCp(-1);                  // Backup to UTF-16 lead word

    return _cp - cpSave;                // - # chars this CTxtPtr moved
}

/*
 *  CTxtPtr::AdjustCpCRLF()
 *
 *  @mfunc
 *      Adjust the position of this text pointer to the beginning of a CRLF,
 *      CRCRLF, or UTF-16 combination if it is in the middle of such a
 *      combination
 *
 *  @rdesc
 *      Number of characters text pointer has been moved by
 *
 *  @future
 *      Adjust to beginning of sequence containing Unicode combining marks
 */
LONG CTxtPtr::AdjustCpCRLF()
{
    _TEST_INVARIANT_

    LONG     cpSave = _cp;
    TCHAR    ch     = GetChar();

//  To support UTF-16, include the following code
//  if((ch & UTF16) == UTF16_TRAIL)
//      AdvanceCp(-1);

    if(!IsASCIIEOP(ch))                         // Early out
        return 0;

    if (ch == LF && cpSave && PrevChar() != CR) // Landed on LF not preceded
    {                                           //  by CR, so go back to LF
        AdvanceCp(1);                           // Else on CR of CRLF or
    }                                           //  second CR of CRCRLF

    if(GetChar() == CR)                         // Land on a CR of CRLF or
    {                                           //  second CR of CRCRLF?
        CTxtPtr tp(*this);

        if(tp.NextChar() == LF)
        {
            tp.AdvanceCp(-2);                   // First CR of CRCRLF ?
            if(tp.GetChar() == CR)              // Yes or CRLF is at start of
                AdvanceCp(-1);                  //  story. Try to back up over
        }                                       //  CR (If at BOS, no effect)
    }
    return _cp - cpSave;
}

/*
 *  CTxtPtr::IsAtEOP()
 *
 *  @mfunc
 *      Return TRUE iff this text pointer is at an end-of-paragraph mark
 *
 *  @rdesc
 *      TRUE if at EOP
 *
 *  @devnote
 *      End of paragraph marks for RichEdit 1.0 and the MLE can be CRLF
 *      and CRCRLF.  For RichEdit 2.0, EOPs can also be CR, VT (0xb - Shift-
 *      Enter), and FF (0xc - page break).
 */
BOOL CTxtPtr::IsAtEOP()
{
    _TEST_INVARIANT_

    TCHAR ch = GetChar();
    BOOL  bRet;

    if(IsASCIIEOP(ch))                          // See if LF <= ch <= CR
    {                                           // Clone tp in case
        CTxtPtr tp(*this);                      //  AdjustCpCRLF moves
        bRet = !tp.AdjustCpCRLF();              // Return TRUE unless in
    }                                           //  middle of CRLF or CRCRLF
    else
    {
        bRet = (ch | 1) == PS;
                                                // Allow synthetic break char
                                                // End of site means end of para.
                                                // Note that text site break
                                                // characters don't denote the
                                                // end of a paragraph.
                                                // and Unicode 0x2028/9 also
    }


    return bRet;
}

/*
 *  CTxtPtr::IsAfterEOP()
 *
 *  @mfunc
 *      Return TRUE iff this text pointer is just after an end-of-paragraph
 *      mark
 *
 *  @rdesc
 *      TRUE iff text ptr follows an EOP mark
 */
BOOL CTxtPtr::IsAfterEOP()
{
    _TEST_INVARIANT_

    CTxtPtr tp(*this);                          // Clone to look around with
    TCHAR ch = GetChar();
    BOOL fAfterEOP;

    if (IsASCIIEOP(ch) && tp.AdjustCpCRLF())
    {
        fAfterEOP = FALSE;
    }
    else
    {
        fAfterEOP = IsEOP(tp.PrevChar()); // After EOP if after Unicode
    }                                     //  PS or LF, VT, FF, or CR

    return fAfterEOP;
}


/*
 * AdvanceChars
 *
 * Synopsis: Moves by the given number of "interesting" characters - in other
 * words, it skips NODECLASS_NONE node characters.
 */
static long
AdvanceChars( CTxtPtr *ptp, long cch )
{
    long iDir;
    long nMoved = 0;
    long nSkip;
    long cpLimit;
    
    if( cch > 0)
    {
        iDir = 1;
        cpLimit = ptp->GetTextLength() - 1;
    }
    else
    {
        iDir = -1;
        cch = -cch;
        cpLimit = 1;
    }

    for( ; long(ptp->_cp) != cpLimit && cch; cch-- )
    {
        do
        {
            nSkip = ptp->AdvanceCp( iDir );
            nMoved += nSkip;
        } while (   ptp->GetChar() == WCH_NODE 
                 && Classify( ptp, NULL ) == NODECLASS_NONE 
                 && nSkip 
                 && long(ptp->_cp) != cpLimit );

        // If we can't move any further
        if( !nSkip )
            break;
    }

    return nMoved;
}

/*
 * CTxtPtr::MoveChar( fForward )
 *
 * Synopsis: Moves one character in the given direction, optionally limiting
 *  the search to the given cp.  If cpMost is -1, it will search to the edge
 *  of the document.  This looks strictly at TEXT.
 *
 * Returns: number of characters moved
 */
long
CTxtPtr::MoveChar( BOOL fForward )
{
    long    cpOrig  = _cp;
    long    iDir    = fForward ? 1 : -1;
    long    cch     = fForward ? GetTextLength() - 1 - _cp : _cp - 1;

    // As long as we have room left...
    if( cch-- )
    {
        AdvanceCp( iDir );

        // Move past nodes to user-level text
        while( cch-- && GetChar() == WCH_NODE )
        {
            AdvanceCp( iDir );
        }
    }

    return( _cp - cpOrig );
}
    

BOOL AutoUrl_IsSpecialChar(TCHAR ch)
{
    return (
        ch == _T(':')
        || ch == _T('/')
        || ch == _T('.')
        || ch == _T('\\')
        || ch == _T('@')
        || ch == _T('#')
        || ch == _T('=')
        || ch == _T('+')
        || ch == _T('&')
        || ch == _T('%')
        || ch == _T('_')
        || ch == _T('"')
        || ch == _T('?')
        || ch == _T('$')
        || ch == _T('~')
        || ch == _T('-')
        || ch == _T(',')
        || ch == _T('|')
        || ch == _T(';')   // (tomfakes) semi-colon is special too
        || ch == ((WCHAR) 0x20ac)   // The Euro.  This cast will cause a non-Unicode compile to fail
        );
}

/*
 *  CTxtPtr::FindWordBreak(action, cpMost)
 *
 *  Synopsis: Finds and moves to the word boundary as specified by action,
 *      and returns the offset from the old position to the new position.
 *
 *  WB_CLASSIFY
 *      Returns char class and word break flags of char at start position.
 *
 *  WB_ISDELIMITER
 *      Returns TRUE iff char at start position is a delimeter.
 *
 *  WB_LEFT (MOVEUNIT_PREVPROOFWORD)
 *      Finds nearest proof word beginning before start position.
 *
 *  WB_LEFTBREAK (MOVEUNIT_PREVWORDEND)
 *      Finds nearest word end before start position.
 *
 *  WB_MOVEWORDLEFT (MOVEUNIT_PREVWORDBEGIN)
 *      Finds nearest word beginning before start position.
 *      This value is used during CTRL+LEFT key processing.
 *
 *  WB_MOVEWORDRIGHT (MOVEUNIT_NEXTWORDBEGIN)
 *      Finds nearest word beginning after start position.
 *      This value is used during CTRL+RIGHT key processing.
 *
 *  WB_RIGHT (MOVEUNIT_NEXTPROOFWORD)
 *      Finds nearest proof word beginning after start position.
 *
 *  WB_RIGHTEDGE == WB_RIGHTBREAK (MOVEUNIT_NEXTWORDEND)
 *      Finds nearest word end after start position.
 *
 *
 *  NB (t-johnh): WB_MOVEURLLEFT/RIGHT are used for the autodetector to 
 *      determine a range of characters that should be checked for being a 
 *      URL.  This is no way implies that the given boundary is the boundary
 *      of a URL, just that should the tp be positioned in a URL, that would 
 *      be the end of it.
 *
 *  WB_MOVEURLLEFT
 *      Finds previous boundary of what could be a URL
 *
 *  WB_MOVEURLRIGHT
 *      Finds next boundary of what could be a URL
 *
 *  @rdesc
 *      Character offset from start of buffer (pch) of the word break
 *
 *
 *  Note: The word navigation actions are grouped into 2 different groups
 *      which follow the same pattern:  WB_MOVEWORDRIGHT, WB_RIGHT, and
 *      WB_LEFTBREAK are group 1; WB_MOVEWORDLEFT, WB_LEFT, and WB_RIGHTBREAK
 *      are group 2.  The two steps in finding the appropriate word break 
 *      are (a) Find the end of the current word, and (b) Skip whitespace to
 *      the edge of the next.
 *      The group 1 actions perform step (a) and then step (b).  The group
 *      2 actions perform step (b) and then step (a). In addition, the end-
 *      seeking actions (LEFT/RIGHTBREAK) must adjust for the fact that the
 *      tp is position just _before_ the last character in a word.
 *
 */
long CTxtPtr::FindWordBreak( int action, BOOL fAutoURL )
{
    _TEST_INVARIANT_

    CTxtPtr     tp( *this );
    TCHAR       chPrev, chNext;
    long        iDir = ( action & 1 ) ? 1 : -1;
    
    long        cpLast = 0;
    long        cpOrig = _cp;
    long        cch;
    BOOL        fEnd = ( action == WB_LEFTBREAK || action == WB_RIGHTBREAK );


    if( action == WB_CLASSIFY || action == WB_ISDELIMITER )
    {
        // BUGBUG (t-johnh): Things that were calling WB_CLASSIFY and 
        // WB_ISDELIMITER were based upon the old kinsoku classification,
        // rather than the new character classes and word breaking classes.
        // Any code calling with these actions should be modified to use
        // character classes and/or word break classes.
        AssertSz( FALSE, "WB_CLASSIFY and WB_ISDELIMITER are no longer valid." );

        return 0;
    }

    // Moving left, we need to be looking at the prev char.
    if( iDir == -1 && tp._cp > 1 )
        tp.AdvanceCp( -1 );
    
    cch = ( iDir == 1 ) ? GetTextLength() - 1 - tp._cp : tp._cp - 1;
    
    //
    // Set up the tp to be prior to the first interesting character -
    //  this is our start position
    //

    while( cch && tp.GetChar() == WCH_NODE && !Classify( &tp, NULL ) ) 
    {
        tp.AdvanceCp( iDir );
        --cch;
    }

        
    // Initial state
    cpLast = tp._cp;

    if( !cch )
        goto done;

    chNext = tp.GetChar();

    // If starting at Thai-type, we need to go into FTTWB
    if(!fAutoURL && IsThaiTypeChar( chNext ) )
    {
        if(iDir == -1)
            tp.AdvanceCp(1);
        long cchOffset = tp.FindThaiTypeWordBreak( action );
        cpLast = tp._cp + cchOffset;
        
        goto done;
    }
    

    //
    // NextBegin, NextProof, PrevEnd: First step is to get out of the
    //  current word
    //

    if(    action == WB_MOVEWORDRIGHT 
        || action == WB_RIGHT
        || action == WB_LEFTBREAK )
    {
        if( chNext == WCH_NODE )
        {
            chNext = _T(' ');
        }
        
        for( ; ; )
        {
            chPrev = chNext;
            
            do
            {
                tp.AdvanceCp( iDir );
                --cch;
            } while( cch && tp.GetChar() == WCH_NODE && !Classify( &tp, NULL ) );

            chNext = tp.GetChar();

            //
            // Conditions for leaving a word:
            // 1) !cch                  => ran out of characters
            // 2) chNext == WCH_NODE    => Hit an interesting node
            // 3) IsThaiTypeChar(chNext) and not auto URL detection => Transition to Thai type
            // 4) WordBreakBoundary     => Word boundary between chars
            // (Varies depending on direction and type of word break)
            //
            if(    !cch 
                || chNext == WCH_NODE 
                || (!fAutoURL && IsThaiTypeChar( chNext ))
                || ( action == WB_RIGHT && IsProofWordBreakBoundary( chPrev, chNext ) )
                || ( action == WB_MOVEWORDRIGHT && IsWordBreakBoundaryDefault( chPrev, chNext ) )
                || ( action == WB_LEFTBREAK && IsWordBreakBoundaryDefault( chNext, chPrev ) ) )
            {
                break;
            }
        } 
    }

    //
    // All options: Skip past whitespace.  Note that for 
    // Next/prev proof word, this has been done/would be done anyway
    //
    while(    cch
           && (    chNext == WCH_NODE
                || WordBreakClassFromCharClass( CharClassFromCh( chNext ) ) == wbkclsSpaceA ) )
    {
        // No-scope nodes are basically one-character words, but only
        //  on the appropriate edge.
        if( chNext == WCH_NODE )
        {
            BOOL fBegin;
            NODE_CLASS nc = Classify( &tp, &fBegin );
            if (    (   nc == NODECLASS_NOSCOPE || nc == NODECLASS_SEPARATOR )
                     && ( ( fEnd ) ? !fBegin : fBegin ) )
                break;
        }
    
        chPrev = chNext;
            
        tp.AdvanceCp( iDir );
        --cch;
        chNext = tp.GetChar();
    }

    cpLast = tp._cp;

    
        
    //
    // NextBegin, NextProof and PrevEnd: Done here, except that PrevEnd is
    //  one character too far, so account for that (but only if we actually
    //  did find some non-node, non-spacing character
    //
    
    if(    action == WB_MOVEWORDRIGHT 
        || action == WB_RIGHT
        || action == WB_LEFTBREAK )
    {
        // If we did find text or no-scope of a previous word, then we
        // should be just past it for WB_LEFTBREAK
        if(    action == WB_LEFTBREAK 
            && (    (    chNext != WCH_NODE 
                      && WordBreakClassFromCharClass( 
                            CharClassFromCh( chNext ) ) != wbkclsSpaceA )
                 || (    chNext == WCH_NODE 
                      && (    Classify( &tp, NULL ) == NODECLASS_NOSCOPE 
                           || Classify( &tp, NULL ) == NODECLASS_SEPARATOR ) ) ) )
        {
            Assert( (long)tp._cp < cpOrig );
            ++cpLast;
        }

        goto done;
    }
    
    if( !cch )
        goto done;


            
    if( chNext == WCH_NODE )
    {
        chNext = _T(' ');
    }
    else if(!fAutoURL && IsThaiTypeChar( chNext ) )
    {
        if(iDir == -1)
            tp.AdvanceCp(1);
        // Moving from space->Thai, we need to go into FTTWB
        long cchOffset = tp.FindThaiTypeWordBreak( action );
        cpLast = tp._cp + cchOffset;
        
        goto done;
    }    

    //
    // NextEnd, NextProof and PrevBegin: Need to move to the next
    //  breaking boundary (see conditions for leaving a word above)
    //
    for( ; ; )
    {
        chPrev = chNext;
        cpLast = tp._cp;
        
        do
        {
            tp.AdvanceCp( iDir );
            --cch;
        } while( cch && tp.GetChar() == WCH_NODE && !Classify( &tp, NULL ) );

        chNext = tp.GetChar();

        // Same break conditions as above
        if(    !cch 
            || chNext == WCH_NODE 
            || (!fAutoURL && IsThaiTypeChar( chNext ))
            || ( action == WB_LEFT && IsProofWordBreakBoundary( chNext, chPrev ) )
            || ( action == WB_MOVEWORDLEFT && IsWordBreakBoundaryDefault( chNext, chPrev ) )
            || ( action == WB_RIGHTBREAK && IsWordBreakBoundaryDefault( chPrev, chNext ) ) )
        {
            break;
        }
    } 
    
    //
    // cpLast is now the cp just before the last character of the
    //  word.  For NextEnd, we want to be past this character.
    //
    if( action == WB_RIGHTBREAK )
    {
        Assert( (long)tp._cp > cpOrig );
        ++cpLast;
    }

done:
    Assert( cpLast >= 0 && cpLast < GetTextLength() );

    SetCp( cpLast );
    return _cp - cpOrig;
}

/*
 *  CTxtPtr::FindBlockBreak
 *
 *  Synopsis: Moves to the next paragraph in the direction specified by
 *      fForward.  Paragraphs are defined by nodes classified as BlockBreaks
 *
 *  Returns: Offset to the paragraph beginning.
 *
 */
long
CTxtPtr::FindBlockBreak( BOOL fForward )
{
    long            cchOffset;
    CTreePos *      ptp     = _pMarkup->TreePosAtCp( _cp, &cchOffset );
    long            cpBound = fForward ? GetTextLength() - 1 : 1;
    long            cpOrig  = _cp;
    long            cpNew   = _cp;

    if(    ( fForward && ptp->GetCp() >= cpBound )
        || ( !fForward && ptp->GetCp() <= 1 ) )
        goto Done;
    
    if( fForward )
    {
        //
        // Move out of this paragraph
        //
        do
        {
            ptp = ptp->NextTreePos();
            Assert( ptp );

            // Hitting a table begin from outside, we skip the table
            if( ptp->IsNode() && ptp->IsBeginElementScope() )
            {
                CTreeNode * pNode       = ptp->Branch();
                CElement *  pElement    = pNode->Element();

                if( ClassifyNodePos( ptp, NULL ) == NODECLASS_SITEBREAK )
                {
                    // Skip to the ending treepos
                    pElement->GetTreeExtent( NULL, &ptp );
                    Assert( ptp );
                    ptp = ptp->NextTreePos();
                }
            }
        } while(    ptp->GetCp() < cpBound
                 && (   !ptp->IsNode() 
                     ||  ClassifyNodePos( ptp, NULL ) < NODECLASS_BLOCKBREAK ) );

        //
        // And into the next - until we see text or noscope.
        //
        while(    ptp->GetCp() < cpBound
               && (    !ptp->IsText() 
                    && !(    ptp->IsNode()
                          && (    ClassifyNodePos( ptp, NULL ) == NODECLASS_NOSCOPE 
                               || ClassifyNodePos( ptp, NULL ) == NODECLASS_SEPARATOR ) ) ) )
        {
            ptp = ptp->NextTreePos();
        }

        cpNew = min(ptp->GetCp(), cpBound);
    }
    else
    {
        //
        // Move to the previous paragraph (text or noscope)
        //
        do
        {
            ptp = ptp->PreviousTreePos();
        } while(    ptp->GetCp() > 1
                 && (    !ptp->IsText()
                      && !(    ptp->IsNode()
                            && (    ClassifyNodePos( ptp, NULL ) == NODECLASS_NOSCOPE 
                                 || ClassifyNodePos( ptp, NULL ) == NODECLASS_SEPARATOR ) ) ) );

        // Now, find a blockbreak to define a paragraph boundary
        while(    ptp->GetCp() > 1
               && (    !ptp->IsNode()
                    ||  ClassifyNodePos( ptp, NULL ) < NODECLASS_BLOCKBREAK ) )
        {
            ptp = ptp->PreviousTreePos();

            // Hitting a table end from outside, we skip the table
            if( ptp->IsNode() && ptp->IsEndElementScope() )
            {
                CTreeNode * pNode       = ptp->Branch();
                CElement *  pElement    = pNode->Element();

                if( ClassifyNodePos( ptp, NULL ) == NODECLASS_SITEBREAK )
                {
                    // Skip to the beginning treepos
                    pElement->GetTreeExtent( &ptp, NULL );
                    Assert( ptp );
                    ptp = ptp->PreviousTreePos();
                }
            }
        }

        // Adjust back forward to actual text-type stuff
        while(    ptp->GetCp() < cpOrig
               && (    !ptp->IsText()
                    && !(    ptp->IsNode()
                          && (    ClassifyNodePos( ptp, NULL ) == NODECLASS_NOSCOPE 
                               || ClassifyNodePos( ptp, NULL ) == NODECLASS_SEPARATOR ) ) ) )
        {
            ptp = ptp->NextTreePos();
        }

        // No greater than cpOrig, and no less than cpMost
        cpNew = max(min(ptp->GetCp(), cpOrig), 1L);
    }

Done:
    SetCp( cpNew );
    return _cp - cpOrig;
}


//
// AutoUrl stuff
//

#define AUTOURL_WILDCARD_CHAR   _T('\b')

//+---------------------------------------------------------------------------

// used by UrlAutodetector and associated helper functions
enum {
    AUTOURL_TEXT_PREFIX,
    AUTOURL_HREF_PREFIX
};

typedef struct {
    BOOL  fWildcard;                      // true if tags have a wildcard char
    UINT  iSignificantLength;             // Number of characters of significance when comparing HREF_PREFIXs for equality
    const TCHAR* pszPattern[2];           // the text prefix and the href prefix
}
AUTOURL_TAG;

// BUGBUG: (tomfakes) This needs to be in-step with the same table in EDUTIL.CXX

AUTOURL_TAG s_urlTags[] = {
    { FALSE, 7, {_T("www."),         _T("http://www.")}},
    { FALSE, 7, {_T("http://"),      _T("http://")}},
    { FALSE, 8, {_T("https://"),     _T("https://")}},
    { FALSE, 6, {_T("ftp."),         _T("ftp://ftp.")}},
    { FALSE, 6, {_T("ftp://"),       _T("ftp://")}},
    { FALSE, 9, {_T("gopher."),      _T("gopher://gopher.")}},
    { FALSE, 9, {_T("gopher://"),    _T("gopher://")}},
    { FALSE, 7, {_T("mailto:"),      _T("mailto:")}},
    { FALSE, 5, {_T("news:"),        _T("news:")}},
    { FALSE, 6, {_T("snews:"),       _T("snews:")}},
    { FALSE, 7, {_T("telnet:"),      _T("telnet:")}},
    { FALSE, 5, {_T("wais:"),        _T("wais:")}},
    { FALSE, 7, {_T("file://"),      _T("file://")}},
    { FALSE, 10, {_T("file:\\\\"),    _T("file:///\\\\")}},
    { FALSE, 7, {_T("nntp://"),      _T("nntp://")}},
    { FALSE, 7, {_T("newsrc:"),      _T("newsrc:")}},
    { FALSE, 7, {_T("ldap://"),      _T("ldap://")}},
    { FALSE, 8, {_T("ldaps://"),     _T("ldaps://")}},
    { FALSE, 8, {_T("outlook:"),     _T("outlook:")}},
    { FALSE, 6, {_T("mic://"),       _T("mic://")}},
    { FALSE, 0, {_T("url:"),         _T("")}},

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // N.B. The following have wildcard characters.
    // If you change \b to something else make sure you also change
    // the AUTOURL_WILDCARD_CHAR macro defined above.
    //
    // Note that there should be the same number of wildcards in the left and right strings.
    // Also, all characters in in both strings must be identical after the FIRST wildcard.
    // For example: LEGAL:   {_T("\b@\b"),   _T("mailto:\b@\b")},     [since @\b == @\b]
    //              ILLEGAL: {_T("\b@hi\b"), _T("mailto:\b@there\b")} [since @hi != @there]

    { TRUE,  0, {_T("\\\\\b"),         _T("file://\\\\\b")}},
    { TRUE,  0, {_T("//\b"),           _T("file://\b")}},
    { TRUE,  0, {_T("\b@\b.\b"),       _T("mailto:\b@\b.\b")}},

};


//+----------------------------------------------------------------------------
//
//  Member: AutoUrl_FindWordBreak
//
//  Synopsis: Finds the next URL candidate boundary.  A URL candidate is
//      defined as an alternating sequence of "word"s (as defined by word
//      breaking rules) and sequences of URL characters (as defined by
//      AutoUrl_IsSpecialChar).
//
//  BUGBUG (t-johnh): Limiting the search to 1024 characters (max url length)
//  might be a good idea, but it would be expensive to keep an accurate
//  character count (because of node chars).
//
//-----------------------------------------------------------------------------
long
CTxtPtr::AutoUrl_FindWordBreak( int nAction )
{
    BOOL    fLastWasWord    = FALSE;
    int     iDir;
    int     nWordAction;
    long    cpOrig          = _cp;
    long    cpSave          = cpOrig;
    long    cch;
    

    Assert( nAction == WB_MOVEURLLEFT || nAction == WB_MOVEURLRIGHT );

    //
    // Set up initial state
    //
    if( nAction == WB_MOVEURLRIGHT )
    {
        iDir = 1;
        nWordAction = WB_RIGHTBREAK;
        cch = GetTextLength() - 1 - _cp;
    }
    else
    {
        iDir = -1;
        nWordAction = WB_MOVEWORDLEFT;
        cch = _cp - 1;

        // Going backwards, we want to look at the previous character
        if( cch )
        {
            AdvanceCp( -1 );
            --cch;
        }
    }

    // Adjust for non-interesting nodes.
    while( GetChar() == WCH_NODE && !Classify( this, NULL ) && cch )
    {
        AdvanceCp( iDir );
        --cch;
    }

    // If we're initially in "space", move to the next word edge,
    // which is then our boundary.
    if(    GetChar() == WCH_NODE
        || WordBreakClassFromCharClass( CharClassFromCh( GetChar() ) ) == wbkclsSpaceA )
    {
        if( iDir == 1 )
        {
            FindWordBreak( WB_MOVEWORDRIGHT, TRUE);
        }
        else
        {
            // Need to adjust forward past the space before calling leftbreak
            AdvanceCp( 1 );
            FindWordBreak( WB_LEFTBREAK, TRUE);
        }

        goto Done;
    }

    for( ; ; )
    {
        // Possibility 1: URL character.  Always allowed.
        if( AutoUrl_IsSpecialChar( GetChar() )  )
        {
            if( iDir == 1 )
            {
                AdvanceCp( 1 );
                --cch;
                cpSave = _cp;
            }
            else
            {
                cpSave = _cp;
            }

            fLastWasWord = FALSE;
        }

        // Possibility 2: "Space"-type character.  End of URL Candidate.
        else if(    GetChar() == WCH_NODE
            || WordBreakClassFromCharClass( CharClassFromCh( GetChar() ) ) == wbkclsSpaceA )
        {
            break;
        }

        // Otherwise, normal text.
        else
        {
            // Cannot have two adjacent words.
            if( fLastWasWord )
                break;

            // Going backwards, we need to adjust before wordbreaking.
            if( iDir == -1 )
            {
                AdvanceCp( 1 );
                ++cch;
            }
                
            cch -= iDir * FindWordBreak( nWordAction, TRUE );
            
            cpSave = _cp;
            fLastWasWord = TRUE;
        }
        
        // Adjust to next interesting character.
        if( iDir == -1 && cch )
        {
            AdvanceCp( -1 );
            --cch;
        }
        while( GetChar() == WCH_NODE && !Classify( this, NULL ) && cch )
        {
            AdvanceCp( iDir );
            --cch;
        }
    }

    SetCp( cpSave );

Done:
    return _cp - cpOrig;
}
        
    
//+----------------------------------------------------------------------------
//
//  Member: AutoUrl_GetAnchorDimensions
//
//  Synopsis: Gets the cp range of the text under the influence of the given
//      anchor.
//
//-----------------------------------------------------------------------------
static void AutoUrl_GetAnchorDimensions( 
    CElement *  pAnchor, 
    long *      pcpStart,
    long *      pcpEnd )
{
    CTreePos *ptpStart, *ptpEnd;

    Assert( pAnchor && pcpStart && pcpEnd );

    // Get the tree pos' for either end of the anchor
    pAnchor->GetTreeExtent( &ptpStart, &ptpEnd );
    Assert( ptpStart->IsNode() && ptpEnd->IsNode() );

    // And use the cps on the inside of the pos'
    *pcpStart = ptpStart->GetCp() + ptpStart->GetCch();
    *pcpEnd   = ptpEnd->GetCp();

    Assert( *pcpStart <= *pcpEnd );
}


//+----------------------------------------------------------------------------
//
//  Member: AutoUrl_GetUrlCandidate
//
//  Synopsis: Retrieves a "word" for the purposes of URL detection.  A word is
//      defined either by the boundaries of a URL candidate (as seen by 
//      AutoUrl_FindWordBreak), or the text under the influence of an anchor.
//
//  cpStart and cpEnd reflect proposed boundaries for the word, as determined
//  by AutoUrl_FindWordBreak.  pcpWordStart and pcpWordEnd will denote the 
//  actual boundaries, accounting for node characters, quotes, and anchors.
//
//-----------------------------------------------------------------------------
static long AutoUrl_GetUrlCandidate(
    CTxtPtr *                               pTxtPtr,                                                               
    long                                    cpStart,
    long                                    cpEnd,
    long *                                  pcpWordStart,
    long *                                  pcpWordEnd,
    CStackDataAry<TCHAR, 256> *             paryWord,
    BOOL *                                  pfQuote )
{
    CTxtPtr         tp( pTxtPtr->_pMarkup, cpEnd );
    CMarkup *       pMarkup = pTxtPtr->_pMarkup;
    TCHAR           ch;
    BOOL            fQuote = FALSE;
    CElement *      pElement;
    CTreePos *      ptp;
    CTreeNode *     pNode;
    long            cchIgnored;
    
    Assert( pcpWordStart && pcpWordEnd && paryWord && pfQuote );

    // Don't include node characters at the end of the candidate.
    while( long(tp._cp) > cpStart && tp.GetPrevChar() == WCH_NODE )
    {
        tp.AdvanceCp( -1 );
        if( Classify( &tp, NULL ) )
        {
            tp.AdvanceCp( 1 );
            break;
        }
        --cpEnd;
    }

    tp.SetCp( cpStart );
    
    //
    // Skip any leading quotes and node characters.
    //
    while(    long(tp._cp) < cpEnd 
           && (    ( ( ch = tp.GetChar() ) == _T('"') ) 
                || ( ch == WCH_NODE && !Classify( &tp, NULL ) ) ) )
    {
        if( ch == _T('"') )
        {
            fQuote = TRUE;
        }
        tp.AdvanceCp( 1 );
        ++cpStart;
    }

    // If there only were quotes/nodes, then we've got nothing.
    if( long(tp._cp) >= cpEnd )
    {
        *pcpWordStart = *pcpWordEnd = cpEnd;
        return 0;
    }

    //
    // Check for enclosing anchor.
    //
    ptp = pMarkup->TreePosAtCp( tp._cp, &cchIgnored );
    pNode = pMarkup->SearchBranchForAnchor( ptp->GetBranch() );
    
    if( !pNode )
    {
        // See if end is under an anchor
        tp.SetCp( cpEnd - 1 );
        ptp = pMarkup->TreePosAtCp( tp._cp, &cchIgnored );
        pNode = pMarkup->SearchBranchForAnchor( ptp->GetBranch() );
    }

    // If the word is under the influence of an anchor
    if( pNode )
    {
        pElement = pNode->Element();

        // Use the anchor's dimensions
        AutoUrl_GetAnchorDimensions( pElement, &cpStart, &cpEnd );

        // Anchors do not set the quote flag
        *pfQuote = FALSE;
    }
    else
    {
        // No anchor
        *pfQuote = fQuote;
    }

    //
    // Ensure we've got enough space for the null-terminated word
    //

    // BUGBUG (t-johnh): Why was this +2?  Should just need one for terminator.
    if( paryWord->Grow( cpEnd - cpStart + 1 ) != S_OK )
    {
        *pcpWordStart = *pcpWordEnd = cpEnd;

        // BUGBUG (t-johnh): I don't like this - if we say no deal, the
        // caller shouldn't access the buffer.  However, it appears this may
        // have been an issue.

        // If there's no word, return an empty string.
        Assert( paryWord->Size() >= 1 );
        (*paryWord)[0] = 0;
    }
    else
    {
        // Have to fill the buffer with the word, stripping out nodes.
        TCHAR * pszWord = *paryWord;
        long    cchLength;

        tp.SetCp( cpStart );
        for( ; long(tp._cp) < cpEnd; tp.AdvanceCp( 1 ) )
        {
            TCHAR ch = tp.GetChar();

            if( ch != WCH_NODE )
                *pszWord++ = ch;
        }

        cchLength = pszWord - *paryWord;
        
        // Terminate
        *pszWord = 0;
        pszWord = *paryWord;

        // Certain punctuation characters and sequences of dissimilar
        // characters should not be included if they are at the end
        // of the word.
        Assert( cchLength >= 0 );
        if( cchLength > 1 )
        {
            TCHAR chLast = pszWord[cchLength - 1];
            tp.SetCp( cpEnd );
            
            if (chLast == _T('.') || chLast == _T('"')
                || chLast == _T('?') || chLast == _T(',')
                || chLast == _T('>') || chLast == _T(';'))
            {
                while (cchLength-- && chLast == pszWord[cchLength])
                {
                    do
                    {
                        tp.AdvanceCp( -1 );
                    } while ( cchLength && tp.GetPrevChar() == WCH_NODE );
                }
    
                pszWord[cchLength + 1] = _T('\0');
                Assert( long(tp._cp)>= cpStart);
            }
        }
    }

    *pcpWordStart = cpStart;
    *pcpWordEnd   = tp._cp;

    return *pcpWordEnd - *pcpWordStart;
}


//+----------------------------------------------------------------------------
//
//  Member: AutoUrl_IsAutoDetectable
//
//  Synopsis: Performs most of the intelligence about URL autodetection -
//      checking through the list of patterns, etc.
//
//-----------------------------------------------------------------------------
static BOOL AutoUrl_IsAutoDetectable( const TCHAR *pszWord, int iIndex )
{
    int i;

    for( i = 0; i < ARRAY_SIZE(s_urlTags); i++ )
    {
        BOOL    fMatch = FALSE;
        const TCHAR * pszPattern = s_urlTags[i].pszPattern[iIndex];

        if( !s_urlTags[i].fWildcard )
        {
            long cchLen = _tcslen( pszPattern );
#ifdef UNIX
            //IEUNIX: We need a correct count for UNIX version.
            long iStrLen = _tcslen( pszWord );
            if ((iStrLen > cchLen) && !_tcsnicmp(pszPattern, cchLen, pszWord, cchLen))
#else
            if (!StrCmpNIC(pszPattern, pszWord, cchLen) && pszWord[cchLen])
#endif
                fMatch = TRUE;
        }
        else
        {
            const TCHAR* pSource = pszWord;
            const TCHAR* pMatch  = pszPattern;

            while( *pSource )
            {
                if( *pMatch == AUTOURL_WILDCARD_CHAR )
                {
                    // N.B. (johnv) Never detect a slash at the
                    //  start of a wildcard (so \\\ won't autodetect).
                    if (*pSource == _T('\\') || *pSource == _T('/'))
                        break;

                    if( pMatch[1] == 0 )
                        // simple optimization: wildcard at end we just need to
                        //  match one character
                        fMatch = TRUE;
                    else
                    {
                        while( *pSource && *(++pSource) != pMatch[1] )
                            ;
                        if( *pSource )
                            pMatch++;       // we skipped wildcard here
                        else
                            continue;   // no match
                    }
                }
                else if( *pSource != *pMatch )
                    break;

                if( *(++pMatch) == 0 )
                    fMatch = TRUE;

                pSource++;
            }
        }
        
        if( fMatch )
        {
            return TRUE;
        }
    }        

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method: IsInsideUrl
//
//  Synopsis:   Determines if the TxtPtr is inside or at the end of a URL.
//      If it is, this function returns TRUE and sets pcpStart and pcpEnd to
//      the start and end cps of the url containing the TxtPtr.
//
//-----------------------------------------------------------------------------
MtDefine( CTxtPtr_IsInsideUrl_aryWord_pv, Locals, "CTxtPtr::IsInsideUrl aryWord::_pv" );
BOOL CTxtPtr::IsInsideUrl( long *pcpStart, long *pcpEnd )
{
    long cpStart, cpEnd;
    long cpWordStart, cpWordEnd;
    BOOL fQuote;
    BOOL fRet = FALSE;
    CStackDataAry<TCHAR, 256> aryWord(Mt(CTxtPtr_IsInsideUrl_aryWord_pv));
    
    CTxtPtr tp( *this );

    //
    // Establish range - from previous URL boundary to the following URL boundary
    //
    tp.AutoUrl_FindWordBreak( WB_MOVEURLLEFT );
    cpStart = tp._cp;
    tp.AutoUrl_FindWordBreak( WB_MOVEURLRIGHT );
    cpEnd = tp._cp;

    // Get the word containing this.
    AutoUrl_GetUrlCandidate(
        this, 
        cpStart, 
        cpEnd, 
        &cpWordStart, 
        &cpWordEnd, 
        &aryWord, 
        &fQuote );

    if( AutoUrl_IsAutoDetectable( aryWord, AUTOURL_TEXT_PREFIX ) )
    {
    //
    // BUGBUG: we need to leave enough room for the prefix we are going to add.
    //         Currently, we use MAX_PREFIX_LEN as a temporary hack.  We really should
    //         compute this value [ashrafm]
    //
#define MAX_PREFIX_LEN 20

        if (cpWordEnd - cpWordStart > MAX_URL_LENGTH - MAX_PREFIX_LEN)
        {
            fRet = FALSE;
            goto Cleanup;
        }
            
#undef MAX_PREFIX_LEN

        *pcpStart = cpWordStart;
        *pcpEnd   = cpWordEnd;

        fRet = TRUE;
    }

    TraceTag((tagUrlDetection, "IsInsideUrl: cpStart: %d  cpEnd: %d  Detectable? %s  Word: >>>%ls<<<", 
        cpStart, cpEnd, fRet ? "Yes" : "No", (TCHAR *) &aryWord.Item(0)));

Cleanup:
    return fRet;
}
    
//+----------------------------------------------------------------------------
//
//  Method: FindUrl
//
//  Synopsis:   Finds the requested URL boundary, looking in the specified
//      direction.  
//
//-----------------------------------------------------------------------------
MtDefine( CTxtPtr_FindUrl_aryWord_pv, Locals, "CTxtPtr::FindUrl aryWord::_pv" );
BOOL
CTxtPtr::FindUrl( BOOL fForward, BOOL fBegin )
{
    int     nAction = fForward ? WB_MOVEURLRIGHT : WB_MOVEURLLEFT;
    long    cpStart, cpEnd;
    long    cpSave = _cp;
    BOOL    fFoundUrl = FALSE;
    CStackDataAry<TCHAR, 256> aryWord(Mt(CTxtPtr_FindUrl_aryWord_pv));

    //
    // Set up initial range
    //
    AutoUrl_FindWordBreak( nAction );
    if( fBegin )
    {
        cpStart = _cp;
        AutoUrl_FindWordBreak( WB_MOVEURLRIGHT );
        cpEnd = _cp;
    }
    else
    {
        cpEnd = _cp;
        AutoUrl_FindWordBreak( WB_MOVEURLLEFT );
        cpStart = _cp;
    }

    for( ; ; )
    {
        long cpWordStart, cpWordEnd;
        BOOL fQuote;

        // First step is to get a URL "word".
        AutoUrl_GetUrlCandidate( 
            this, 
            cpStart, 
            cpEnd, 
            &cpWordStart, 
            &cpWordEnd, 
            &aryWord, 
            &fQuote );

        // cpWordStart == cpWordEnd signifies there were no more candidates.
        if( cpWordStart == cpWordEnd )
            break;

        //
        // See if this is an actual URL, but also must verify position of
        // URL "word".  Since AutoUrl_GetUrlCandidate uses anchor boundaries
        // for text under influence of an anchor, it's possible that we could
        // get the same "word" several times.  This ensures that we 
        // actually keep moving, by requring that GetUrlCandidate doesn't give
        // us a boundary in the wrong direction
        //
        if(    (    (  fForward && (    (  fBegin && cpWordStart >  cpSave )
                                     || ( !fBegin && cpWordStart >= cpSave ) ) )
                 || ( !fForward && (    (  fBegin && cpWordEnd   <= cpSave )
                                     || ( !fBegin && cpWordEnd   <  cpSave ) ) ) )
            && AutoUrl_IsAutoDetectable( aryWord, AUTOURL_TEXT_PREFIX ) )
        {
            fFoundUrl = TRUE;

            if( fBegin )
            {
                SetCp( cpWordStart );
            }
            else
            {
                SetCp( cpWordEnd );
            }
            
            break;
        }

        // No URL, so find the next boundary to give to GetUrlCandidate.
        if( fForward )
        {
            // Find the next boundary forward from the end of this word.
            cpStart = cpEnd;
            SetCp( cpStart );
            AutoUrl_FindWordBreak( WB_MOVEURLRIGHT );
            cpEnd = _cp;
        }
        else
        {
            // Find the boundary prior to the beginning of this word.
            cpEnd = cpStart;
            SetCp( cpEnd );
            AutoUrl_FindWordBreak( WB_MOVEURLLEFT );
            cpStart = _cp;
        }
    }

    return fFoundUrl;
}


/*
 *  CTxtPtr::ReplaceRange(cchOld, cchNew, *pch, pcpFirstRecalc)
 *
 *  @mfunc
 *      replace a range of text at this text pointer.
 *
 *  @rdesc
 *      count of new characters added
 *
 *  @comm   SideEffects: <nl>
 *      moves this text pointer to end of replaced text <nl>
 *      moves text block array <nl>
 *
 *  @todo   (alexgo) need to handle undo better for boundary cases
 */
DWORD CTxtPtr::ReplaceRange(
    LONG cchOld,                //@parm length of range to replace
                                // (<lt> 0 means to end of text)
// BUGBUG - EricVas: Why is cch a DWORD here?  Should be a long (there are places
//          which have to cast from long to dword...
    DWORD cchNew,               //@parm length of replacement text
    TCHAR const *pch)           //@parm replacement text
{
    _TEST_INVARIANT_

    DWORD cchAdded = 0;
    DWORD cchInBlock;
    DWORD cchNewInBlock;
    TCHAR ch = 0;

    CTxtBlk *ptb;

    Assert(pch);

    // Bit of a hack: rather than a seperate piece of code for repeating chars,
    // use the pch parameter if it's a small value.

    if ((DWORD_PTR)pch <= 0xFFFF)
        ch = (TCHAR)pch;

    //
    // BUGBUG - EricVas: This hack seems only useful for the initialization
    //          case where a CR is placed at the end of the text, where the
    //          the text is currently empty.  Should Assert( cchOld >= 0 )
    //          here instead.
    //

    if(cchOld < 0)
        cchOld = GetTextLength() - _cp;

    // blocks involving replacement

    while(cchOld > 0 && cchNew > 0)
    {

        ptb = GetCurrRun();

        // we cchOld should never be non zero if the text run is empty

        AssertSz(ptb, "CTxtPtr::Replace() - Pointer to text "
            "block == NULL !");

        ptb->MoveGap(GetIch());
        cchInBlock = min(cchOld, ptb->_cch - GetIch() );
        if(cchInBlock > 0)
        {
            cchOld          -= cchInBlock;
            ptb->_cch       -= cchInBlock;
            ((CTxtArray *)_prgRun)->_cchText    -= cchInBlock;
        }
        cchNewInBlock = CchOfCb(ptb->_cbBlock) - ptb->_cch;

        // if there's room for a gap, leave one
        if(cchNewInBlock > cchGapInitial)
            cchNewInBlock -= cchGapInitial;

        if(cchNewInBlock > cchNew)
            cchNewInBlock = cchNew;

        if(cchNewInBlock > 0)
        {
            if (!ch)
            {
                CopyMemory(ptb->_pch + GetIch(), pch, CbOfCch(cchNewInBlock));
                pch             += cchNewInBlock;
            }
            else
            {
                DWORD cch = cchNewInBlock;
                TCHAR *pch = ptb->_pch + GetIch();
                
                while (cch--)
                    *pch++ = ch;
            }
            cchNew          -= cchNewInBlock;
            _cp             += cchNewInBlock;
            SetIch( GetIch() + cchNewInBlock );
            cchAdded        += cchNewInBlock;
            ptb->_cch       += cchNewInBlock;
            ptb->_ibGap     += CbOfCch(cchNewInBlock);
            ((CTxtArray *)_prgRun)->_cchText    += cchNewInBlock;
        }
        if(GetIRun() >= NumRuns() - 1 || !cchOld )
                 break;

        // go to next block
        SetIRun( GetIRun() + 1 );
        SetIch( 0 );
    }


    if(cchNew > 0)
    {
        if (ch)
        {
            cchAdded += InsertRange(cchNew, pch);
        }
        else
        {
            cchAdded += InsertRepeatingChar(cchNew, ch);
        }
    }
    else if(cchOld > 0)
    {
        DeleteRange(cchOld);
    }

    return cchAdded;
}


/*
 *  CTxtPtr::InsertRepeatingChar(cch, ch)
 *
 *  @mfunc
 *      insert a character a number of tiems
 *
 *  @rdesc
 *      count of new characters added
 *
 *  @comm   SideEffects: <nl>
 *      moves this text pointer to end of replaced text <nl>
 *      moves text block array <nl>
 */
long
CTxtPtr::InsertRepeatingChar(
    LONG cch,                   //@parm number of chars to insert
    TCHAR ch)                   //@parm character to insert
{
    Assert( cch > 0 );
    TCHAR ach[32];
    TCHAR *pch;
    LONG cch2;
    LONG cchLeft;

    // fill up temp array with chars
    for (cch2 = min((LONG)ARRAY_SIZE(ach), cch), pch = ach; cch2; pch += 1, 
cch2 -= 1)
        *pch = ch;
            
    // Insert chars in chunks
    for (cchLeft = cch; cchLeft; cchLeft -= cch2)
    {
        cch2 = InsertRange(min((LONG)ARRAY_SIZE(ach), cchLeft), ach);
        if (!cch2)
            break;
    }

    return cch - cchLeft;
}

/*
 *  TxFindEOP (pchBuff, cch)
 *
 *  @func
 *      Given a string, find the offset to the next EOP marker
 *
 *  @rdesc
 *      Offset to next EOP marker
 *
 *  @devnote
 *      This could probably be replaced by FindEOP()
 */
LONG TxFindEOP(
    const TCHAR *pchBuff,       //@parm the string buffer to look in
    LONG cch)                  //@parm the number valid characters
{
    LONG cchToUse = 0;

    for(; cchToUse < cch && *pchBuff != CR && *pchBuff != LF;
          cchToUse++, pchBuff++) ;

#if DBG==1
    if(cchToUse != cch)
        TraceTag((tagWarning, "TxFindEOP(): found CR or LF at %ld of %ld", cchToUse, cch));
#endif

    return cchToUse;
}

/*
 *  CTxtPtr::InsertRange(cch, pch)
 *
 *  @mfunc
 *      Insert a range of characters at this text pointer
 *
 *  @rdesc
 *      Count of characters successfully inserted
 *
 *  @comm Side Effects: <nl>
 *      moves this text pointer to end of inserted text <nl>
 *      moves the text block array <nl>
 */
long CTxtPtr::InsertRange (
    DWORD cch,              //@parm length of text to insert
    TCHAR const *pch)       //@parm text to insert
{
    _TEST_INVARIANT_

    DWORD cchSave = cch;
    DWORD cchInBlock;
    DWORD cchFirst;
    DWORD cchLast = 0;
    DWORD ctbNew;
    long cRuns;
    CTxtBlk *ptb;

#if DBG == 1
    if (IsTagEnabled(tagOneCharTextInsert) && cch > 1)
    {
        long cchTotal = 0;
        long cchInserted;
        
        while (cch)
        {
            cchInserted = InsertRange(1, pch);
            AssertSz(cchInserted == 1, "Failed to insert single char in one-char-at-a-time mode");
            pch += 1;
            cch -= 1;
            cchTotal += 1;
        }

        return cchTotal;
    }
#endif

    // Ensure text array is allocated
    cRuns = NumRuns();
    
    if(!cRuns)
    {
        LONG    cbSize = -1;

        // If we don't have any blocks, allocate first block to be big enuf
        // for the inserted text *only* if it's smaller than the normal block
        // size. This allows us to be used efficiently as a display engine
        // for small amounts of text.

        if (_pMarkup->_fIncrementalAlloc)
        {
            if( cch < CchOfCb(cbBlockInitial) )
                cbSize = CbOfCch(cch);
        }

        if( !((CTxtArray *)_prgRun)->AddBlock(0, cbSize) )
        {
            goto done;
        }
    }

    ptb = GetCurrRun();
    cchInBlock = CchOfCb(ptb->_cbBlock) - ptb->_cch;
    AssertSz(ptb->_cbBlock <= cbBlockMost, "block too big");

    // try and resize without splitting...
    if(cch > cchInBlock &&
        cch <= cchInBlock + CchOfCb(cbBlockMost - ptb->_cbBlock)
        && (!_pMarkup->IsStreaming() || _pMarkup->_fIncrementalAlloc))
    {
        if (!ptb->ResizeBlock(min((DWORD)cbBlockMost,
                                  CbOfCch(ptb->_cch + cch +
                                          (_pMarkup->_fIncrementalAlloc
                                           ? 0 : cchGapInitial)))))
        {
            goto done;
        }
        cchInBlock = CchOfCb(ptb->_cbBlock) - ptb->_cch;
    }
    if(cch <= cchInBlock)
    {
        // all fits into block without any hassle
        ptb->MoveGap(GetIch());
        CopyMemory(ptb->_pch + GetIch(), pch, CbOfCch(cch));
        _cp             += cch;                 // *this points at end of
        SetIch( GetIch() + cch );
        ptb->_cch       += cch;
        ((CTxtArray *)_prgRun)->_cchText    += cch;
        ptb->_ibGap     += CbOfCch(cch);

        return long( cch );
    }
    
    // logic added (dbau 11/98): before splitting:
    // first try using free space that's in the next block. If we
    // blindly split all the time, we can end up with a pile of mostly-empty
    // huge blocks resulting from splitting one nearly-full block near its edge
    // repeatedly. Note that the problem is exacerbated by our initial small block.
    // To fix the general case, we also need to add logic to try using the
    // previous block as well (not done yet - maybe we should rewrite the
    // whole thing with these considerations...)

    if (cRuns > GetIRun() + 1)
    {
        CTxtBlk *ptbAdj;

        ptbAdj = GetNextRun();
        if (ptbAdj)
        {
            DWORD cchInAdj = CchOfCb(ptbAdj->_cbBlock) - ptbAdj->_cch;
            if (cch <= cchInAdj + cchInBlock)
            {
                // ooh, we have enough room. Do the shuffling around and the insert
                DWORD cchPostGap;

                // move the gaps to the right place
                ptb->MoveGap(GetIch());
                ptbAdj->MoveGap(0);
                
                cchPostGap = ptb->_cch - GetIch();
                
                if (cch <= cchInBlock + cchPostGap)
                {
                    // Case 1: if post-gap text is moved to the second block,
                    // all the new text fits in first block

                    // move as much as we can to the second block to maximize gap in the first block
                    DWORD cchToMove = (cchPostGap <= cchInAdj) ? cchPostGap : cchInAdj;
                    CopyMemory(ptbAdj->_pch + CchOfCb(ptbAdj->_cbBlock) - ptbAdj->_cch - cchToMove,
                               ptb->_pch + CchOfCb(ptb->_cbBlock) - cchToMove, CbOfCch(cchToMove));

                    // Slide anything remaining in the first block to the right
                    if (cchToMove < cchPostGap)
                        MoveMemory(ptb->_pch + CchOfCb(ptb->_cbBlock) - cchPostGap + cchToMove,
                                   ptb->_pch + CchOfCb(ptb->_cbBlock) - cchPostGap, CbOfCch(cchPostGap - cchToMove));

                    // Copy the inserted text
                    CopyMemory(ptb->_pch + GetIch(), pch, CbOfCch(cch));

                    // Then update pointers and counters
                    _cp             += cch;
                    SetIch(GetIch() + cch);
                    ptb->_cch       += cch - cchToMove;
                    ptb->_ibGap     += CbOfCch(cch);
                    ptbAdj->_cch    += cchToMove;
                    ((CTxtArray *)_prgRun)->_cchText    += cch;

                    return long(cch);
                }
                else
                {
                    // Case 2: even after moving post-gap text to the second block,
                    // the new text will overflow to the second block.

                    // move all post-gap text to the second block
                    CopyMemory(ptbAdj->_pch + CchOfCb(ptbAdj->_cbBlock) - ptbAdj->_cch - cchPostGap,
                               ptb->_pch + CchOfCb(ptb->_cbBlock) - cchPostGap, CbOfCch(cchPostGap));
                    
                    // move the part of the new text that fits in the first block
                    CopyMemory(ptb->_pch + GetIch(), pch, CbOfCch(cchInBlock + cchPostGap));
                    
                    // move the part of the new text that needs to go in the second block
                    CopyMemory(ptbAdj->_pch, pch + cchInBlock + cchPostGap, CbOfCch(cch - cchInBlock - cchPostGap));

                    // Then update pointers and counters
                    _cp             += cch;
                    SetIRun(GetIRun() + 1);
                    SetIch(cch - cchInBlock - cchPostGap);
                    ptb->_cch       += cchInBlock;
                    ptb->_ibGap     = 0;                     // first gap is empty; its location is moot
                    ptbAdj->_cch    += cch - cchInBlock;
                    ptbAdj->_ibGap  = CbOfCch(cch - cchInBlock - cchPostGap);
                    ((CTxtArray *)_prgRun)->_cchText    += cch;

                    return long(cch);
                }
            }
        }
    }

    // won't all fit in this block

    // figure out best division into blocks
    TxDivideInsertion(cch, GetIch(), ptb->_cch - GetIch(), & cchFirst, &cchLast);

    if (GetIch() == ptb->_cch)
    {
        // BUGFIX (dbau): we should not do a split at the very end of a block,
        // or else we'll introduce an empty block, which causes problems.
        // BUGBUG (dbau): I don't know if the beginning-of-the-block case is exposed
        // to a similar problem.
        // (Really, when splitting _near_ either edge of a block, we also should try
        // to use an adjacent block's gap instead of always creating a whole new
        // block - but most cases are already fixed by the use-adjacent logic above;
        // further fixes should probably wait until a rewrite).

        ptb->MoveGap(GetIch());
        cchFirst = CchOfCb(ptb->_cbBlock) - ptb->_cch;
        cchLast = 0;
    }
    else
    {
        // Subtract cchLast up front so return value isn't negative
        // if SplitBlock() fails
        cch -= cchLast; // don't include last block in count for middle blocks

        // split block containing insertion point

        // ***** moves _prgtb ***** //
        if(!((CTxtArray *)_prgRun)->SplitBlock(GetIRun(), GetIch(), cchFirst, cchLast,
            _pMarkup->IsStreaming()))
        {
            goto done;
        }
        ptb = GetCurrRun();            // recompute ptb after (*_prgRun) moves
    }

    // copy into first block (first half of split)
    if(cchFirst > 0)
    {
        AssertSz(ptb->_ibGap == CbOfCch(GetIch()), "split first gap in wrong place");
        AssertSz(cchFirst <= CchOfCb(ptb->_cbBlock) - ptb->_cch, "split first not big enough");

        CopyMemory(ptb->_pch + GetIch(), pch, CbOfCch(cchFirst));
        cch             -= cchFirst;
        pch             += cchFirst;
        SetIch( GetIch() + cchFirst );
        ptb->_cch       += cchFirst;
        ((CTxtArray *)_prgRun)->_cchText    += cchFirst;
        ptb->_ibGap     += CbOfCch(cchFirst);
    }

    // copy into middle blocks
    // bugbug: review (jonmat) I increased the size for how large a split block
    // could be and this seems to increase the performance, we need to test the
    // block size difference on a retail build, however. 5/15/1995
    ctbNew = cch / cchBlkInsertmGapI /* cchBlkInitmGapI */;
    if(ctbNew <= 0 && cch > 0)
        ctbNew = 1;
    for(; ctbNew > 0; ctbNew--)
    {
        cchInBlock = cch / ctbNew;
        AssertSz(cchInBlock > 0, "nothing to put into block");

        // ***** moves _prgtb ***** //
        SetIRun( GetIRun() + 1 );
        if(!((CTxtArray *)_prgRun)->AddBlock(GetIRun(),
            CbOfCch(cchInBlock + cchGapInitial)))
        {
            BindToCp(_cp);  //force a rebind;
            goto done;
        }
        // NOTE: next line intentionally advances ptb to next CTxtBlk

        ptb = GetCurrRun();
        AssertSz(ptb->_ibGap == 0, "New block not added correctly");

        CopyMemory(ptb->_pch, pch, CbOfCch(cchInBlock));
        cch             -= cchInBlock;
        pch             += cchInBlock;
        SetIch( cchInBlock );
        ptb->_cch       = cchInBlock;
        ((CTxtArray *)_prgRun)->_cchText    += cchInBlock;
        ptb->_ibGap     = CbOfCch(cchInBlock);
    }
    AssertSz(cch == 0, "Didn't use up all text");

    // copy into last block (second half of split)
    if(cchLast > 0)
    {
        AssertSz(GetIRun() < NumRuns()-1, "no last block");
        SetIRun( GetIRun() + 1 );
        ptb = GetRunAbs(GetIRun());
        AssertSz(ptb->_ibGap == 0,  "split last gap in wrong place");
        AssertSz(cchLast <= CchOfCb(ptb->_cbBlock) - ptb->_cch,
                                    "split last not big enuf");

        CopyMemory(ptb->_pch, pch, CbOfCch(cchLast));
        // don't subtract cchLast from cch; it's already been done
        SetIch( cchLast );
        ptb->_cch       += cchLast;
        ((CTxtArray *)_prgRun)->_cchText    += cchLast;
        ptb->_ibGap     = CbOfCch(cchLast);
        cchLast = 0;                        // Inserted all requested chars
    }

done:
    AssertSz(cch + cchLast >= 0, "we should have inserted some characters");
    AssertSz(cch + cchLast <= cchSave, "don't insert more than was asked for");

    cch = cchSave - cch - cchLast;          // # chars successfully inserted
    _cp += cch;

    AssertSz (GetTextLength() ==
                ((CTxtArray *)_prgRun)->GetCch(),
                "CTxtPtr::InsertRange(): _prgRun->_cchText screwed up !");
    return long( cch );
}


/*
 *  TxDivideInsertion(cch, ichBlock, cchAfter, pcchFirst, pcchLast)
 *
 *  @func
 *      Find best way to distribute an insertion
 *
 *  @rdesc
 *      nothing
 */
void TxDivideInsertion(
    DWORD cch,              //@parm length of text to insert
    DWORD ichBlock,         //@parm offset within block to insert text
    DWORD cchAfter,         //@parm length of text after insertion in block
    DWORD *pcchFirst,       //@parm exit: length of text to put in first block
    DWORD *pcchLast)        //@parm exit: length of text to put in last block
{
    DWORD cchFirst = max(0L, (LONG)(cchBlkCombmGapI - ichBlock));
    DWORD cchLast  = max(0L, (LONG)(cchBlkCombmGapI - cchAfter));
    DWORD cchPartial;
    DWORD cchT;

    // Fill first and last blocks to min block size if possible

    cchFirst = min(cch, cchFirst);
    cch     -= cchFirst;
    cchLast = min(cch, cchLast);
    cch     -= cchLast;

    // How much is left over when we divide up the rest?
    cchPartial = cch % cchBlkInsertmGapI;
    if(cchPartial > 0)
    {
        // Fit as much as the leftover as possible in the first and last
        // w/o growing the first and last over cbBlockInitial
        cchT        = max(0L, (LONG)(cchBlkInsertmGapI - ichBlock - cchFirst));
        cchT        = min(cchT, cchPartial);
        cchFirst    += cchT;
        cch         -= cchT;
        cchPartial  -= cchT;
        if(cchPartial > 0)
        {
            cchT    = max(0L, (LONG)(cchBlkInsertmGapI - cchAfter - cchLast));
            cchT    = min(cchT, cchPartial);
            cchLast += cchT;
        }
    }
    *pcchFirst = cchFirst;
    *pcchLast = cchLast;
}


/*
 *  CTxtPtr::DeleteRange(cch)
 *
 *  @mfunc
 *      Delete cch characters starting at this text pointer
 *
 *  @rdesc
 *      nothing
 *
 *  @comm Side Effects: <nl>
 *      moves text block array
 */
void CTxtPtr::DeleteRange(
    DWORD cch)      //@parm length of text to delete
{
    _TEST_INVARIANT_

    DWORD       cchInBlock;
    DWORD       ctbDel = 0;                 // Default no blocks to delete
    DWORD       itb;
    CTxtBlk *   ptb = GetCurrRun();

    AssertSz(ptb,
        "CTxtPtr::DeleteRange: want to delete, but no text blocks");

    if (cch > GetTextLength() - _cp)// Don't delete beyond EOT
        cch = GetTextLength() - _cp;

    ((CTxtArray *)_prgRun)->_cchText -= cch;

    // remove from first block
    ptb->MoveGap(GetIch());
    cchInBlock = min(long(cch), ptb->_cch - GetIch());
    cch -= cchInBlock;
    ptb->_cch -= cchInBlock;

#if DBG==1
    ((CTxtArray *)_prgRun)->Invariant();
#endif

    for(itb = ptb->_cch ? GetIRun() + 1 : GetIRun();
            cch && long(cch) >= GetRunAbs(itb)->_cch; ctbDel++, itb++)
    {
        // More to go: scan for complete blocks to remove
        cch -= GetRunAbs(itb)->_cch;
    }

    if(ctbDel)
    {
        // ***** moves (*_prgRun) ***** //
        itb -= ctbDel;
        ((CTxtArray *)_prgRun)->RemoveBlocks(itb, ctbDel);
    }


    // remove from last block
    if(cch > 0)
    {
        ptb = GetRunAbs(itb);
        AssertSz(long(cch) < ptb->_cch, "last block too small");
        ptb->MoveGap(0);
        ptb->_cch -= cch;
#if DBG==1
        ((CTxtArray *)_prgRun)->Invariant();
#endif

    }

    if (    ((CTxtArray *)_prgRun)->CombineBlocks(GetIRun())
        ||  GetIRun() >= NumRuns() 
        || !GetRunAbs(GetIRun())->_cch)
        BindToCp(_cp);                  // Empty block or blocks combined: force tp rebind

    AssertSz (GetTextLength() ==
                ((CTxtArray *)_prgRun)->GetCch(),
                "CTxtPtr::DeleteRange(): _prgRun->_cchText screwed up !");
}


/*
 * Method: FindText
 *
 * Synopsis: This does a search optimized for ascii text, using the
 *  Knuth-Morris-Pratt string matching algorithm as described in
 *  "Introduction to Algorithms" by Cormen, Leiserson and Rivest.
 *
 *  The basic idea of the algorithm is to calculate a "prefix function"
 *  (which is a bit of a misnomer for backwards searching) that contains
 *  information about how the pattern to be found matches against itself.
 *  When a partial match fails, all the characters up to the failing point
 *  are known, so there's no reason to look at them again.  By using the
 *  prefix function, we know where the next possible match could occur, so
 *  we skip up to that point.
 *
 *  Positions this at the beginning of the match if found; if not, this does
 *  not move.  Returns the cp just after the match if found; if not, returns
 *  -1.
 */

MtDefine( CTxtPtr_FindAsciiText_aryPrefixFunction_pv, Locals, "CTxtPtr::FindAsciiText aryPrefixFunction::_pv" );
MtDefine( CTxtPtr_FindAsciiText_aryPatternBuffer_pv, Locals, "CTxtPtr::FindAsciiText aryPatterBuffer::_pv" );
LONG CTxtPtr::FindText(
    LONG            cpLimit,
    DWORD           dwFlags,
    TCHAR const *   pch,
    long            cchToFind )
{
    CStackPtrAry<LONG_PTR, 20>  aryPrefixFunction( Mt(CTxtPtr_FindAsciiText_aryPrefixFunction_pv) );
    CStackPtrAry<LONG_PTR, 20>  aryPatternBuffer(  Mt(CTxtPtr_FindAsciiText_aryPatternBuffer_pv) );

    // Flags
    BOOL            fIgnoreCase;
    BOOL            fWholeWord;
    BOOL            fRaw;
    long            iDir;

    const TCHAR *   pchCurr = pch;
    LONG            cchMatched = 0;
    LONG            cch;

    CTxtPtr         tp( *this );
    
    Assert( pchCurr );
    AssertSz( cchToFind > 0, "Zero length pattern should get caught before here" );

    //
    // Argument checking and set-up
    //

    // Set up our options from dwFlags
    fIgnoreCase    = !(FR_MATCHCASE & dwFlags);
    fWholeWord     =   FR_WHOLEWORD & dwFlags;
    fRaw           =   FINDTEXT_RAW & dwFlags;

    iDir = ( FINDTEXT_BACKWARDS & dwFlags ) ? -1 : 1;

    //
    // Compute the prefix function
    //
    {
        LONG          cchDone;
        LONG          cchPrefixLength = 0;

        aryPrefixFunction.EnsureSize( cchToFind );
        aryPatternBuffer.EnsureSize(  cchToFind );

        // If we're searching backwards, start at the end of the pattern
        if( -1 == iDir )
        {
            pchCurr += cchToFind - 1;
        }

        for( cchDone = 0; cchDone < cchToFind; cchDone++ )
        {
            // Make sure we've just got ascii
            if( !fRaw && *pchCurr >= 0xff )
            {
                return FindComplexHelper(
                        cpLimit,
                        dwFlags,
                        pch,
                        cchToFind );
            }

            // Copy into the buffer - for a case insensitive compare,
            // convert to lower case if necessary
            if( fIgnoreCase && *pchCurr >= _T('A') && *pchCurr <= _T('Z') )
            {
                aryPatternBuffer.Append( *pchCurr - ( _T('A') - _T('a') ) );
            }
            else
            {
                aryPatternBuffer.Append( *pchCurr );
            }

            // We can't extend the current match
            while(  cchPrefixLength > 0 
                &&  aryPatternBuffer[cchPrefixLength] != aryPatternBuffer[cchDone] )
            {
                // Go to the next smallest match
                cchPrefixLength = (LONG)aryPrefixFunction.Item( cchPrefixLength - 1 );
            };

            // The first entry is always 0
            if( cchDone && aryPatternBuffer[cchPrefixLength] == *pchCurr )
            {
                ++cchPrefixLength;
            }

            aryPrefixFunction.Append( cchPrefixLength );
            pchCurr += iDir;
        }
    }   // Done with prefix function


    //
    // Perform the search
    //

    // Set/check the limit - cch is the amount of characters left
    // in the document in the appropriate direction.
    if( 1 == iDir )
    {
        LONG cchText = GetTextLength();

        if( cpLimit < 0 || cpLimit > cchText )
            cpLimit = cchText;

        cch = cpLimit - _cp;
    }
    else
    {
        if( cpLimit < 0 )
            cpLimit = 0;

        // Make sure we didn't get a limit farther down than us
        Assert( cpLimit <= (LONG)_cp);

        cch = _cp - cpLimit;
    }

    // Make sure the limit is valid.
    Assert(   ( iDir == -1 && cpLimit <= (long)_cp ) 
           || ( iDir ==  1 && cpLimit >= (long)_cp ) );

    // Make sure we have enough characters to attempt to match
    if ( cchToFind > cch )
    {
        return -1;
    }

    // Outer loop: Keep getting chunks while there's text left
    while( cch > 0 )
    {
        long    cchChunk;
        TCHAR   chCurr;

        // Get the current chunk of text.  cchChunk is how many
        // characters are left in the current chunk.
        if( 1 == iDir )
        {
            pchCurr = tp.GetPch( cchChunk );
            cchChunk = min( cch, cchChunk );
            tp.AdvanceCp( cchChunk );
        }
        else
        {
            // GetPchReverse returns a pointer to the character
            // just after tp, but with text _before_ the pointer,
            // so we skip back to the first thing before this spot.
            pchCurr = tp.GetPchReverse( cchChunk );
            --pchCurr;
            cchChunk = min( cch, cchChunk );
            tp.AdvanceCp( -cchChunk );
        }

        cch -= cchChunk;

        Assert( cch >= 0 );


        // Chunk loop: This does the main bulk of the work
        while( cchChunk > 0 )
        {
            // In case this chunk starts with node characters, skip 'em.
            while( cchChunk && *pchCurr == WCH_NODE )
            {
                pchCurr += iDir;
                --cchChunk;
            }

            if( !cchChunk )
                break;

            chCurr = *pchCurr;

            // Make sure we've just got ascii
            if( chCurr >= 0xff )
            {
                return FindComplexHelper(
                        cpLimit,
                        dwFlags,
                        pch,
                        cchToFind );
            }

            // BUGBUG (t-johnh): Look at maybe doing this with a switch statement
            if( fIgnoreCase && InRange(chCurr, TEXT('A'), TEXT('Z')) )
            {
                chCurr += (TCHAR)( 'a' - 'A' );
            }
            else if( chCurr == WCH_NONBREAKSPACE)
            {
                // &nbsp's have their own character, but should match spaces.
                chCurr = _T(' ');
            }

            // While we can't match any more characters, try jumping back
            // to a smaller match and see if we can match from there
            while( cchMatched > 0 && chCurr != aryPatternBuffer[cchMatched] )
            {
                cchMatched = (LONG)aryPrefixFunction[cchMatched - 1];
            }

            // If the next character matches, then increment match count
            if( chCurr == aryPatternBuffer[cchMatched] )
            {
                ++cchMatched;
            }
            
            // Matched the string - now do some checking
            if( cchMatched == cchToFind)
            {
                CTxtPtr tpStart( _pMarkup, tp._cp );
                CTxtPtr tpEnd ( *this );
                long    cchLength;

                // Here we have to set pointers at the beginning and
                // end of the match.  We only know the edge of the match
                // that we most recently saw (end if going forward,
                // beginning if going backward), so we start from there
                // and count characters to the other side, checking
                // for breaking nodes on the way.
                if( -1 == iDir )
                {
                    // Move tp to the beginning of match
                    tpStart.AdvanceCp( cchChunk - 1 );
                    tpEnd.SetCp( tpStart._cp);

                    // We want to be at the cp just after the last 
                    // character matched, so go right before it, and
                    // then one more cp past.
                    for( cchLength = cchToFind; cchLength; tpEnd.AdvanceCp( 1 ) )
                    {
                        if( tpEnd.GetChar() != WCH_NODE )
                        {
                            --cchLength;
                        }
                        else if( Classify( &tpEnd, NULL ) != NODECLASS_NONE )
                            goto invalid_match;
                    }
                }
                else
                {
                    tpStart.AdvanceCp( -cchChunk + 1 );
                    tpEnd.SetCp( tpStart._cp );

                    for( cchLength = cchToFind; cchLength; )
                    {
                        tpStart.AdvanceCp( -1 );

                        if( tpStart.GetChar() != WCH_NODE )
                        {
                            --cchLength;
                        }
                        else if( Classify( &tpStart, NULL ) != NODECLASS_NONE )
                            goto invalid_match;
                    }
                }

                // Check for whole word matching
                if(     !fWholeWord 
                    || ( tpStart.IsAtBOWord() && tpEnd.IsAtEOWord() ) )
                {
                    SetCp(tpStart._cp);
                    return tpEnd._cp;
                }

                // Either we didn't satisfy the whole word match, or
                // there was a break in the middle, so skip to next
                // smallest match

invalid_match:
                cchMatched = (LONG)aryPrefixFunction[cchMatched - 1];
            }

            // Next char - if we run out of chars in this chunk,
            // we'll drop out of the loop and get some more.
            pchCurr += iDir;
            --cchChunk;

        } // while( cchChunk > 0 )

    } // while( cch > 0 )

    // We ran out of characters without finding a match.
    return -1;
}


typedef struct
{
    long cp;
    long cch;
} StripRecord;


// BUGBUG (t-johnh): This will be replaced with a new version once some stuff
//  is added to shlwapi.

MtDefine( CTxtPtr_FindComplexHelper_aryMatchBuffer_pv, Locals, "CTxtPtr::FindComplexHelper aryMatchBuffer::_pv" );
MtDefine( CTxtPtr_FindComplexHelper_aryPatternBuffer_pv, Locals, "CTxtPtr::FindComplexHelper aryPatternBuffer::_pv" );
MtDefine( CTxtPtr_FindComplexHelper_aryStripped_pv, Locals, "CTxtPtr::FindComplexHelper aryStripped::_pv" );

LONG CTxtPtr::FindComplexHelper(
    LONG            cpLimit,
    DWORD           dwFlags,
    TCHAR const *   pch,
    long            cchToFind )
{
    CDataAry<TCHAR>       aryMatchBuffer( Mt(CTxtPtr_FindComplexHelper_aryMatchBuffer_pv) );
    CDataAry<TCHAR>       aryPatternBuffer( Mt(CTxtPtr_FindComplexHelper_aryPatternBuffer_pv) );
    CDataAry<StripRecord> aryStripped( Mt(CTxtPtr_FindComplexHelper_aryStripped_pv) );

    TCHAR WCH_ALEF = 0x0627;

    _TEST_INVARIANT_

    CTxtPtr         tp(*this);      // Keeps track of where we're looking

    LONG            cch;            
    long            cchText = GetTextLength();

    // Flags
    BOOL            fIgnoreCase;
    BOOL            fWholeWord;
    long            iDir;       

    // For tree navigation
    long            cchOffset;
    CTreePos *      ptp  = _pMarkup->TreePosAtCp( _cp, &cchOffset );

    // For buffer navigation
    const TCHAR *   pchCurr;
    long            cchPatternSize;
    long            cchForward;
    long            cchLeft;
    long            nIndex;         // For looping through records
    long            nRecords;       // of stripped characters


    Assert(pch);

    //
    // Argument checking and set-up
    //

    // Set up our options from dwFlags
    fIgnoreCase    = !(FR_MATCHCASE & dwFlags);
    fWholeWord     =   FR_WHOLEWORD & dwFlags;

    iDir = (FINDTEXT_BACKWARDS & dwFlags ) ? -1 : 1;

    // They didn't give us any characters to find!
    if(cchToFind <= 0)
    {
        return -1;
    }

    // Doctor up the pattern to match
    {
        cchLeft = cchToFind;

        aryPatternBuffer.EnsureSize( cchToFind );
        cchPatternSize = 0;
        pchCurr       = pch;

        for( ; cchLeft; cchLeft--, pchCurr++ )
        {
            // If we're ignoring Diacritics/Kashidas, we want to
            // strip them out of our pattern.
            if(   ( !( dwFlags & FR_MATCHDIAC ) && IsBiDiDiacritic( *pchCurr ) )
               || ( !( dwFlags & FR_MATCHKASHIDA ) && *pchCurr == WCH_KASHIDA ) )
            {
                continue;
            }
            else if( !( dwFlags & FR_MATCHALEFHAMZA ) && IsAlef( *pchCurr ) )
            {
                aryPatternBuffer[cchPatternSize++] = WCH_ALEF;
            }
            else
            {
                aryPatternBuffer[cchPatternSize++] = *pchCurr;
            }
        }

        aryPatternBuffer.SetSize( cchPatternSize );
    }
    
    // Set/check the limiting cp for search.
    if( 1 == iDir )
    {
        if( cpLimit < 0 || cpLimit > cchText )
            cpLimit = cchText;
    }
    else
    {
        if( cpLimit < 0 )
            cpLimit = 0;
    }

    // Loop through paragraphs, buffering up the interesting text
    // and attempting to match on it.
    for( ; ; )
    {
        StripRecord    sr;
        BOOL           fStripping;
        long           cchPara;
        CTxtPtr        tpParaStart( *this );
        CTxtPtr        tpParaEat( *this );
        CTxtPtr        tpParaEnd( *this );
        
        sr.cp = 0;      // init
        sr.cch = 0;
        //////////////////////////////////////////////////////////////////
        //
        // Step 1: Define the paragraph boundaries by getting node
        // information from the tree.
        //
        //////////////////////////////////////////////////////////////////

        cch = 0;
        nRecords = 0;
        fStripping = FALSE;

        // If moving backwards, set ptp to the first interesting pos
        // in the paragraph.
        
        if (iDir == -1)
        {
            // Skip back to an embedding or block break

            for ( ; ; )
            {
                CTreePos * pPrevPos = ptp->PreviousTreePos();

                if (!pPrevPos)
                    break;
                
                if (ptp->IsNode() && ClassifyNodePos( ptp, NULL ) != NODECLASS_NONE)
                    break;

                ptp = pPrevPos;
            }

            // Then move forward to the interesting one.
            ptp = ptp->NextTreePos();

            // Paragraph starts here, but
            
            tpParaStart.SetCp( ptp->GetCp() );

            // Limit our buffer start based on tp and the limit
            
            if (tpParaStart._cp > tp._cp)
                tpParaStart.SetCp( tp._cp );
            else if (long( tpParaStart._cp ) < cpLimit)
                tpParaStart.SetCp( cpLimit );

            tpParaEnd.SetCp( tp._cp );
        }
        else
        {
            // Moving forwards, find the end of this paragraph

            for ( ; ; )
            {
                if (!ptp)
                    break;

                if (ptp->IsNode() && ClassifyNodePos( ptp, NULL ) != NODECLASS_NONE)
                    break;

                ptp = ptp->NextTreePos();
            }

            
            tpParaEnd.SetCp( ptp ? ptp->GetCp() : cpLimit );

            // Limit the buffer ending based on the limit.
            
            if (long(tpParaEnd._cp) > cpLimit)
                tpParaEnd.SetCp( cpLimit );
            
            tpParaStart.SetCp( tp._cp );
        }

        // Make sure we've got enough space.  
        // BUGBUG (t-johnh): This will be a quite
        // wasteful (twice the space needed), but better than repeated 
        // allocations to grow the buffer.  The thing is, I can't just
        // Append with a DataAry - I'd have to allocate new StripRecords
        // and AppendIndirect - this way I can just use [] and assign.

        if (tpParaEnd._cp < tpParaStart._cp)
            goto no_match;
        
        aryMatchBuffer.EnsureSize( tpParaEnd._cp - tpParaStart._cp );
        aryStripped.EnsureSize( tpParaEnd._cp - tpParaStart._cp );

        //////////////////////////////////////////////////////////////////
        //
        // Step 2: Build up the buffer from tpParaStart to tpParaEnd
        // by stripping out any un-interesting characters and storing
        // StripRecords noting where we pulled out characters so that
        // we can rebuild Cp's later.
        //
        //////////////////////////////////////////////////////////////////

        cchPara = tpParaEnd._cp - tpParaStart._cp;
        tpParaEat.SetCp( tpParaStart.GetCp() );
        while( cchPara )

        {
            long cchChunk;

            pch = tpParaEat.GetPch( cchChunk );
            cchChunk = min(cchChunk, cchPara);
            tpParaEat.AdvanceCp( cchChunk );
            cchPara -= cchChunk;

            while( cchChunk )
            {
                // Do we want to strip this character out of our buffer?
                if(   ( *pch == WCH_NODE )
                   || ( !( dwFlags & FR_MATCHDIAC ) && IsBiDiDiacritic( *pch ) )
                   || ( !( dwFlags & FR_MATCHKASHIDA ) && *pch == WCH_KASHIDA )
                  )
                {
                    if( !fStripping )
                    {
                        // If we're not stripping yet, set up a new
                        // strip record.
                        fStripping = TRUE;
                        sr.cp  = cch;
                        sr.cch = 1;
                    }
                    else
                    {
                        // otherwise, just tally up another character.
                        ++sr.cch;
                    }
                }
                else
                {
                    // Real character
                    if( fStripping )
                    {
                        // If we were stripping, save the record.
                        aryStripped[nRecords++] = sr;
                        fStripping = FALSE;
                    }

                    if( !( dwFlags & FR_MATCHALEFHAMZA ) && IsAlef( *pch ) )
                    {
                        aryMatchBuffer[cch++] = WCH_ALEF;
                    }
                    else
                    {
                        aryMatchBuffer[cch++] = *pch;
                    }
                }

                ++pch;
                --cchChunk;
            }
        }

        // Set the correct sizes for our buffers.
        aryMatchBuffer.SetSize( cch );
        aryStripped.SetSize( nRecords );



        //////////////////////////////////////////////////////////////////
        //
        // Step 3: Scan through this buffer in the appropriate direction
        // looking for a match, and then verify it if necessary.
        //
        //////////////////////////////////////////////////////////////////

        if( 1 == iDir )
        {
            // Search forward from buffer start
            pchCurr = (TCHAR *)aryMatchBuffer;
            cchForward = cch;
        }
        else
        {
            // Search backwards from buffer end.
            pchCurr = (TCHAR *)aryMatchBuffer + cch - 1;
            cchForward = 1;
        }

        // Now that we've got our buffer, try and match on it.
        cchLeft = cch;

        while( cchLeft > 0 )
        {
            if( 2 == CompareStringAltW( 
                        LOCALE_USER_DEFAULT, 
                        SORT_STRINGSORT | ( fIgnoreCase ? ( NORM_IGNORECASE | NORM_IGNOREWIDTH ) : 0 ),
                        pchCurr,
                        min(cchForward, cchPatternSize),
                        aryPatternBuffer,
                        cchPatternSize ) )
            {
                CTxtPtr tpStart( _pMarkup, tpParaStart._cp + pchCurr - aryMatchBuffer );
                CTxtPtr tpEnd( *this );

                // Move start to the match point, and adjust for stripped nodes.
                for( nIndex = 0, nRecords = aryStripped.Size();
                     nRecords > 0 && aryStripped[nIndex].cp <= pchCurr - aryMatchBuffer;
                     nRecords--, nIndex++ )
                {
                    tpStart.AdvanceCp( aryStripped[nIndex].cch );
                }

                // Now do the same for the ending cp.
                tpEnd.SetCp( tpStart._cp + cchPatternSize );
                for( ; 
                     nRecords > 0 && aryStripped[nIndex].cp < pchCurr + cchPatternSize - aryMatchBuffer;
                     nRecords--, nIndex++ )
                {
                    tpEnd.AdvanceCp( aryStripped[nIndex].cch );
                }

                if( !fWholeWord ||
                    ( tpStart.IsAtBOWord() && tpEnd.IsAtEOWord() ) )
                {
                    SetCp( tpStart._cp );
                    return tpEnd._cp;
                }
            }

            pchCurr += iDir;
            cchForward -= iDir;
            --cchLeft;
        }

        // No match if we've hit the limit.
        if( (long)tpParaStart._cp == cpLimit || (long)tpParaEnd._cp == cpLimit )
            goto no_match;

        // Move over to the next paragraph.
        if( 1 == iDir )
        {
            ptp = _pMarkup->TreePosAtCp( tpParaEnd._cp, &cchOffset );
            while( ptp->IsNode() && ClassifyNodePos( ptp, NULL ) != NODECLASS_NONE )
            {
                ptp = ptp->NextTreePos();
                if ( !ptp )
                    goto no_match;
            }
            tp.SetCp( ptp->GetCp() );
        }
        else
        {
            ptp = _pMarkup->TreePosAtCp( tpParaStart._cp - 1, &cchOffset );

            while( ptp->IsNode() && ClassifyNodePos( ptp, NULL ) != NODECLASS_NONE )
            {
                ptp = ptp->PreviousTreePos();
                if ( !ptp )
                    goto no_match;
            }
            tp.SetCp( ptp->GetCp() + ptp->GetCch() );
        }

     }
no_match:
     return -1;
}


BOOL
CTxtPtr::FindCrOrLf ( long cchMax )
{
    CTxtPtr txtPtr( * this );
    
    for ( ; ; )
    {
        const TCHAR * pch;
        long cchValid, i;
        
        if (cchMax == 0)
            return FALSE;

        pch = txtPtr.GetPch( cchValid );

        if (cchValid == 0 || !pch)
            return FALSE;

        if (cchMax > 0)
        {
            if (cchValid > cchMax)
                cchValid = cchMax;

            cchMax -= cchValid;
        }

        for ( i = 0 ; i < cchValid ; i++, pch++ )
        {
            TCHAR ch = *pch;
            
            if (ch == _T('\r') || ch == _T('\r'))
            {
                SetCp( long( txtPtr.GetCp() ) + i );
                return TRUE;
            }
        }
        
        txtPtr.AdvanceCp( cchValid );
    }
}

/*
 *  CTxtPtr::FindEOP(cch)
 *
 *  @mfunc
 *      Find EOP mark in a range within cch chars from this text pointer and
 *      position *this after it.  If no EOP is found, position *this at
 *      beginning/end of document (BOD/EOD) for cch <lt>/<gt> 0, respectively,
 *      that is, BOD and EOD are treated as a BOP and an EOP, respectively.
 *
 *  @rdesc
 *      return cch moved and move *this after EOP if found, or else to
 *      BOD/EOD for cch <lt>/<gt> 0, respectively
 *
 *  @future
 *      Find CRLF, CR, LF, or Unicode paragraph separator (0x2029)
 */
LONG CTxtPtr::FindEOP (
    LONG cchMax )       //@parm max signed count of chars to search
{
    LONG        cch, cchStart;              // cch's for scans
    TCHAR       ch;                         // Current char
    DWORD       cpSave  = _cp;              // Save _cp for returning delta
    LONG        iDir    = 1;                // Default forward motion
    const TCHAR*pch;                        // Used to walk text chunks
    CTxtPtr     tp(*this);                  // tp to search text with

    cch = 0;                                // init
    if(cchMax < 0)                          // Backward search
    {
        iDir = -1;                          // Backward motion
        cchMax = -cchMax;                   // Make max count positive
        cch = tp.AdjustCpCRLF();            // If in middle of CRLF or
        if(!cch && IsAfterEOP())            //  CRCRLF, or follow any EOP,
            cch = tp.BackupCpCRLF();        //  backup before EOP
        cchMax += cch;
    }

    while(cchMax > 0)                       // Scan until get out of search
    {                                       //  range or match an EOP
        pch = iDir > 0                      // Point pch at contiguous text
            ? tp.GetPch(cch)                //  chunk going forward or
            : tp.GetPchReverse(cch);        //  going backward

        if(!pch)                            // No more text to search
            break;

        if(iDir < 0)                        // Going backward, point at
            pch--;                          //  previous char

        cch = min(cch, cchMax);             // Limit scan to cchMax chars
        for(cchStart = cch; cch; cch--)     // Scan chunk for EOP
        {
            ch = *pch;
            pch += iDir;
            if(IsEOP(ch))
                break;
        }
        cchStart -= cch;                    // Get cch of chars passed by
        cchMax -= cchStart;                 // Update cchMax
        tp.AdvanceCp(iDir*(cchStart));      // Update tp
        if(cch)                             // Found an EOP
            break;
    }                                       // Continue with next chunk

    long cp = tp.GetCp();

    if(!cch && cp && cp < GetTextLength())  // Didn't find EOP within the
    {                                       //  entry cchMax of chars, so
        return 0;                           //  leave this text ptr unchanged
    }                                       //  and return 0

    SetCp(cp);                              // Found EOP or cp is at beginning
                                            //  or end of story: set _cp = cp
    if(cch)
    {                                       // Match occurred
        if(GetChar() == LF)                 // In case there's a LF there,
            AdvanceCp(1);                   //  bypass it
        else if(iDir > 0)                   // Position this ptr just after
            AdvanceCpCRLF();                //  EOP
    }
    return _cp - cpSave;                    // Return cch this tp moved
}

/*
 *  CTxtPtr::FindBOSentence(cch)
 *
 *  @mfunc
 *      Find beginning of sentence in the document.
 *
 *  @rdesc
 *      Count of chars moved *this moves
 *
 *  @comm
 *      This routine defines a sentence as a character string that ends with
 *      period followed by at least one whitespace character or the EOD.  This
 *      should be replacable so that other kinds of sentence endings can be
 *      used.  This routine also matches initials like "M. " as sentences.
 *      We could eliminate those by requiring that sentences don't end with
 *      a word consisting of a single capital character.  Similarly, common
 *      abbreviations like "Mr." could be bypassed.  To allow a sentence to
 *      end with these "words", two blanks following a period could be used
 *      to mean an unconditional end of sentence.
 */
LONG CTxtPtr::FindBOSentence ( BOOL fForward )
{
    _TEST_INVARIANT_

    LONG    cchWhite = 0;                       // No whitespace chars yet
    long    cpSave   = _cp;                     // Save value for return
    long    cch      = fForward ? GetTextLength() - 1 - _cp : _cp - 1;
    BOOL    fST;                                // TRUE if sent terminator
    LONG    iDir     = fForward ? 1 : -1;       // AdvanceCp() increment
    CTxtPtr tp(*this);                          // tp to search with


    //
    // If moving forward, backup over whitespace.
    // This makes sure we recognize a new sentence
    // if we were in whitespace between 2 previous
    // sentences.
    //
    if(iDir == 1)
    {
        while( IsWhiteSpace(&tp) && cch )
        {
            long cchMoved = AdvanceChars( &tp, -1 );

            cch -= cchMoved;
            if( !cchMoved )
                break;
        }
    }

    while( cch )
    {
        //
        // Find a sentence terminator
        //
        for( fST = FALSE; cch; )
        {
            fST = IsSentenceTerminator( tp.GetChar() );
            if( fST )
                break;

            cch -= iDir * AdvanceChars( &tp, iDir );
        }

        // No ST, or hit forward end of document
        if( !fST || ( fForward && !cch ) )
            break;

        //
        // Skip forward past whitespace
        //
        cchWhite = 0;
        cch -= iDir * AdvanceChars( &tp, 1 );
        while(    IsWhiteSpace( &tp ) 
               && (    ( fForward && cch ) 
                    || ( !fForward && long(tp._cp) < cpSave ) ) )
        {
            cchWhite++;
            cch -= iDir * AdvanceChars( &tp, 1 );
        }

        //
        // Needed whitespace, but if we're moving backwards,
        // make sure we actually are prior to start position.
        //
        if( cchWhite && ( fForward || long(tp._cp) < cpSave ) )
            break;

        //
        // Didn't match - adjust prior to ST if backwards
        //
        if( !fForward )
        {
            cch += AdvanceChars( &tp, -cchWhite - 2 );
        }
    }           

    if(cchWhite || !cch )                       // If sentence found or got
        SetCp(tp._cp);                          //  start/end of story, set
                                                //  _cp to tp's
    return _cp - cpSave;                        // Tell caller cch moved
}

/*
 *  CTxtPtr::IsAtBOSentence()
 *
 *  @mfunc
 *      Return TRUE iff *this is at the beginning of a sentence as defined
 *      in the description of the FindBOSentence(cch) routine
 *
 *  @rdesc
 *      TRUE iff *this is at the beginning of a sentence
 */
BOOL CTxtPtr::IsAtBOSentence()
{
    if(!_cp)                                    // Beginning of story is an
        return TRUE;                            //  unconditional beginning
                                                //  of sentence

    if(    IsWhiteSpace( this )                 // Proper sentences don't
        || IsSentenceTerminator( GetChar() ) )  //  start with whitespace or
    {                                           //  sentence terminators
        return FALSE;
    }

    LONG    cchWhite;
    CTxtPtr tp(*this);                          // tp to walk preceding chars

    for(cchWhite = 0;                           // Backspace over possible
        AdvanceChars( &tp, -1 ) && IsWhiteSpace( &tp ); //  span of whitespace chars
        cchWhite++) ;

    return cchWhite && IsSentenceTerminator( tp.GetChar() );
}

/*
 *  CTxtPtr::IsAtBOWord()
 *
 *  @mfunc
 *      Return TRUE iff *this is at the beginning of a word, that is,
 *      the char at _cp isn't whitespace and either _cp = 0 or the char at
 *      _cp - 1 is whitespace.
 *
 *  @rdesc
 *      TRUE iff *this is at the beginning of a Word
 */
BOOL CTxtPtr::IsAtBOWord()
{
    if(!_cp || IsAtEOP())                   // Story beginning is also
        return TRUE;                        //  a word beginning

    CTxtPtr tp(*this);
    AdvanceChars( &tp, -1 );
    tp.FindWordBreak(WB_MOVEWORDRIGHT);
    return _cp == tp._cp;
}

/*
 *  CTxtPtr::IsAtEOWord()
 *
 *  @mfunc
 *      Return TRUE iff *this is at the end of a word.
 *
 *  @rdesc
 *      TRUE iff *this is at the end of a Word
 */
BOOL CTxtPtr::IsAtEOWord()
{
    if(!_cp == (GetTextLength() - 1) || IsAtEOP())      // Story end is also
        return TRUE;                                    //  a word end

    CTxtPtr tp(*this);
    AdvanceChars( &tp, -1 );
    tp.FindWordBreak(WB_RIGHTBREAK);
    return _cp == tp._cp;
}

/*
 *  CTxtPtr::IsAtMidWord()
 *
 *  @mfunc
 *      Return TRUE iff *this is at the middle of a word, that is, the char
 *      at _cp isn't whitespace and neither _cp - 1 is whitespace.
 *
 *  @rdesc
 *      TRUE iff *this is at the middle of a Word
 */
BOOL CTxtPtr::IsAtMidWord()
{
    long    cchText = GetTextLength();
    long    cpSave  = _cp;
    TCHAR   chNext, chPrev;

    // Get next interesting character
    while(    GetChar() == WCH_NODE 
           && Classify( this, NULL ) == NODECLASS_NONE 
           && long(_cp) < cchText - 1 )
        AdvanceCp( 1 );
        
    chNext = GetChar();
    if( chNext == WCH_NODE )
        return FALSE;

    // Get previous interesting character
    SetCp( cpSave - 1 );
    while(    GetChar() == WCH_NODE 
           && Classify( this, NULL ) == NODECLASS_NONE 
           && _cp > 1 )
       AdvanceCp( -1 );

    chPrev = GetChar();
    if( chPrev == WCH_NODE )
        return FALSE;

    return !IsWordBreakBoundaryDefault( chPrev, chNext );
}


/*
 *  CTxtPtr::CheckMoveGap ( DWORD cchLine )
 *
 *  @mfunc
 *      Helper function for rendering, so that the renderer wont bump
 *      up against a gap when it is trying to render text with overhangs.
 *
 *  @rdesc
 *      We are always called from the renderer when we are at the start of
 *      a line, so _ich is already at the line's start. We check cchLine
 *      to see if it crosses the gap, and if so, we move the gap to the
 *      beginning of the line.
 *
 *  @comm
 *      This code does not handle the case when the line crosses a block
 *      boundry.
 */
void
CTxtPtr::CheckMoveGap (
    DWORD cchLine )  //@parm cch of the line about to be rendered.
{
    CTxtBlk *ptb;
    DWORD   ichGap;

    Assert(IsValid());

    ptb = GetCurrRun();

    ichGap = CchOfCb(ptb->_ibGap);      // if line crosses block gap.
    if ( GetIch() < long(ichGap) && long(ichGap) < (GetIch() + long(cchLine)) )
    {
        ptb->MoveGap(GetIch());             // move gap to line start.
    }
}


/*
 *  CTxtPtr::NextCharCount(&cch)
 *
 *  @mfunc
 *      Helper function for getting next char and decrementing abs(*pcch)
 *
 *  @rdesc
 *      Next char
 */
TCHAR CTxtPtr::NextCharCount (
    LONG& cch)                  //@parm count to use and decrement
{
    LONG    iDelta = (cch > 0) ? 1 : -1;

    if(!cch || !AdvanceCp(iDelta))
        return 0;

    cch -= iDelta;                          // Count down or up
    return GetChar();                       // Return char at _cp
}


/*
 * CTxtPtr::MoveCluster( fForward )
 *
 * Synopsis: Moves one cluster in the given direction. A cluster is defined as 
 * one or more characters that are grouped into a unit.  This looks strictly 
 * at TEXT.
 *
 * Returns: number of characters moved
 */
#define MAX_MOVE_BUFFER  33
#define NODE_EXTRA       20

long
CTxtPtr::MoveCluster( BOOL fForward )
{
    TCHAR aryItemize [ MAX_MOVE_BUFFER ];
    CStackDataAry <TCHAR, MAX_MOVE_BUFFER + NODE_EXTRA> aryNodePos(Mt(CTxtPtrFindThaiTypeWordBreak_aryNodePos_pv));
    SCRIPT_LOGATTR arySLA [ MAX_MOVE_BUFFER ];
    long cchMove, cchBefore, cchAfter, lHoldNode;
    long cpSave, cchText;
    BOOL fMovedPastSignificantNodes = FALSE;
    CTxtPtr tp(*this);
    BOOL fCurrentIsNode = FALSE;
    TCHAR ch;

    cchMove = cchBefore = cchAfter = lHoldNode = 0;
    
    cpSave = GetCp();
    cchText = GetTextLength();
    
    // Adjust starting position
    if(cchText)    
    {
        //
        // Note, we don't include the root node's WCH_NODE chars as valid chars to deal with.
        //
        
        int  iDir = fForward ? 1 : -1;
        long cchLeft = fForward ? cchText - 1 - tp._cp : tp._cp;

        //
        // If we are going backward, and there are characters behind us
        //
        
        if (!fForward && cchLeft)
        {
            tp.AdvanceCp( -1 );
            cchLeft--;
        }

        //
        // Skip over all non scope node chars
        //
        
        fMovedPastSignificantNodes = FALSE;

        while ( cchLeft && tp.GetChar() == WCH_NODE )
        {
            switch ( Classify( & tp, NULL ) )
            {
            case NODECLASS_NONE       :
                break;

            case NODECLASS_SEPARATOR  :
            case NODECLASS_LINEBREAK  :
            case NODECLASS_BLOCKBREAK :
            case NODECLASS_SITEBREAK  :
                fMovedPastSignificantNodes = TRUE;
                break;

            case NODECLASS_NOSCOPE    :
                fMovedPastSignificantNodes = TRUE;

                //
                // Move past the two node chars to get past this noscope, then
                // set cchLeft to 0 to blow out of the loop
                //

                tp.AdvanceCp( 2 * iDir );

                cchLeft = 0;
                break;

            default :
                AssertSz( 0, "Unexpected Node class" );
                break;
            }

            if (cchLeft != 0)
            {
                tp.AdvanceCp( iDir );
                cchLeft--;
            }
        }
    }
   

    //
    // If the current character is not a clustering char, then we can quickly
    // deal with it.
    //
    
    // paulnel - we want to handle password characters in Clusters as normal text
    //           since it is drawn as normal text (with "*")
    ch = tp.GetChar();
    if (!IsClusterTypeChar( ch ) || tp.IsPasswordChar())
    {
        //
        // If we are moving backwards, we have already adjusted to the correct
        // position.  Otherwise, if we are moving forward to the next beginning of
        // a cluster, only do so if we have not skipped past any significant
        // 'synthetic' characters.  THis deals with the case of "a</p><p>b" where
        // one moves from just after the 'a' to just before the 'b'.
        //
        
#ifndef NO_UTF16
        if (IsSurrogateChar(ch))
        {
            if (!fMovedPastSignificantNodes)
            {
                if (fForward)
                {
                    if (   cchText > 1
                        && IsHighSurrogateChar(ch))
                    {
                        CTxtPtr tpNext(tp);

                        ch = tpNext.NextChar();

                        if (IsLowSurrogateChar(ch))
                        {
                            tp.AdvanceCp( 2 );
                        }
                    }
                }
                else
                {
                    if (   tp._cp > 0
                        && IsLowSurrogateChar(ch))
                    {
                        CTxtPtr tpPrev(tp);

                        ch = tpPrev.PrevChar();

                        if (IsHighSurrogateChar(ch))
                        {
                            tp.AdvanceCp( -1 );
                        }
                    }
                }
            }
        }
        else
#endif
        if (fForward && !fMovedPastSignificantNodes)
            tp.AdvanceCp( 1 );

        SetCp( tp.GetCp() );
        
        return GetCp() - cpSave;
    }

    //
    // Did we just arrive on a Thai type character? Move to it.
    //
    // BUGBUG: (paulnel) if moving backwards do we need to adjust the _cp?
    if(fMovedPastSignificantNodes)
    {
        SetCp( tp.GetCp() );

        return GetCp() - cpSave;
    }

    tp.SetCp( cpSave );

    if(tp.GetChar() == WCH_NODE)
        fCurrentIsNode = TRUE;
    
    if(!tp.PrepThaiTextForBreak(FALSE, 
                                fForward, 
                                fCurrentIsNode, 
                                cchText, 
                                &aryNodePos, 
                                aryItemize, 
                                &cchBefore, 
                                &cchAfter, 
                                &cchMove,
                                &lHoldNode))
    {
        cchMove = AdvanceCp(cchMove);
        return cchMove;
    }

    long offset = ItemizeAndBreakRun( aryItemize, &cchBefore, &cchAfter, arySLA );

    if (fForward)
    { 

        do
        {
            cchBefore++;
            cchAfter--;
            cchMove++;

        }
        while ( cchAfter >= 0 && ! arySLA[cchBefore].fCharStop );

    }
    else
    {
        // go backwards in the attribute array until the first word break is encountered

        while ( cchBefore > 0 && ! arySLA[cchBefore].fCharStop )
        {
            cchBefore--;
            cchMove--;

        }

        // We decremented cchBefore before passing it in to be itemized.
        // Therefore we need to increase the offset to move it to the 
        // correct place.
        if(!fCurrentIsNode)
            offset++;

    }

    Assert(offset + cchMove >= 0 &&
           offset + cchMove < aryNodePos.Size());
    Assert( fForward ? aryNodePos [ offset + cchMove ] - aryNodePos [ offset ] >= 0
                     : aryNodePos [ offset + cchMove ] - aryNodePos [ offset ] <= 0);

    cchMove += aryNodePos [ offset + cchMove ] - aryNodePos [ offset ] 
               + ((fForward && fCurrentIsNode) ? lHoldNode : 0);

    AdvanceCp( cchMove );

    return cchMove;
}

//+----------------------------------------------------------------------------
//
//  Member: MoveClusterEnd
//
//  Synopsis: Moves the TxtPtr to the next cluster ending in the given
//      direction.  This accounts for non-interesting nodes, too.
//
//-----------------------------------------------------------------------------
long CTxtPtr::MoveClusterEnd( BOOL fForward )
{
    long cpOrig = _cp;
    long cpSave = _cp;
    long cch;

    // To set up for previous end, we have to start at the next begin
    
    if (!fForward)
        MoveCluster( TRUE );

    for( ; ; )
    {
        if( fForward && !MoveCluster( TRUE ) )
            break;

        // Set limits
        cch     = fForward ? _cp -cpSave : _cp - 1;
        cpSave  = _cp;

        // Scan backwards across nodes,
        while( cch && GetPrevChar() == WCH_NODE )
        {
            AdvanceCp( -1 );
            --cch;
        }

        cch = cpSave - _cp;

        // Then scan forward across un-interesting nodes
        while( cch && GetChar() == WCH_NODE && !Classify( this, NULL ) )
        {
            AdvanceCp( 1 );
            --cch;
        }

        // Make sure we're ending up in the right direction
        if(    (  fForward && long(_cp) > cpOrig )
            || ( !fForward && long(_cp) < cpOrig ) )
            break;

        // If not, try the next one
        if( !MoveCluster( fForward ) )
            break;
    }

    return _cp - cpOrig;
}

/*
 *  CTxtPtr::FindThaiTypeWordBreak(int action )
 *
 *  @mfunc
 *      Find a word break in Thai script and move this text pointer to it.
 *
 *  @rdesc
 *      Offset from cp of the word break
 */
#define MAX_BREAK_BUFFER 75

LONG CTxtPtr::FindThaiTypeWordBreak(
    int     action)     //@parm Direction of movement in run
                        // NOTE: The limit character MUST be on a block boundary if < 47 characters
                        //       to give certainty of valid break location.
{
    long    cchMove = 0;
        
    if(action == WB_MOVEWORDRIGHT || action == WB_MOVEWORDLEFT ||
       action == WB_RIGHTBREAK || action == WB_LEFTBREAK)
    {

        TCHAR aryItemize[MAX_BREAK_BUFFER];
        CStackDataAry <TCHAR, MAX_BREAK_BUFFER + NODE_EXTRA> aryNodePos(Mt(CTxtPtrFindThaiTypeWordBreak_aryNodePos_pv));
        SCRIPT_LOGATTR arySLA[MAX_BREAK_BUFFER];
        long  cchBefore = 0;
        long  cchAfter = 0;
        CTxtPtr tp(*this);
        BOOL  fForward = (action == WB_RIGHTBREAK || action == WB_MOVEWORDRIGHT);
        
        // Set up for ScriptItemize(). We need to re-itemize the string instead
        // of using the cached _Analysis struct because we don't know how many
        // characters are involved.

        // Make sure the current character is ThaiType
        Assert(fForward ? IsThaiTypeChar(tp.GetChar()) : IsThaiTypeChar(tp.GetPrevChar()));

        if(!tp.PrepThaiTextForBreak(TRUE, 
                                    fForward, 
                                    FALSE, 
                                    GetTextLength(), 
                                    &aryNodePos, 
                                    aryItemize, 
                                    &cchBefore, 
                                    &cchAfter, 
                                    &cchMove))
        {
            cchMove = AdvanceCp(cchMove);
            return cchMove;
        }

        if( !fForward && cchBefore > 0 )
        {
            cchBefore--;
            cchMove --;
        }    

        long offset = ItemizeAndBreakRun(aryItemize, &cchBefore, &cchAfter, arySLA);

        if(fForward)
        { 
            do
            {
                cchBefore++;
                cchAfter--;
                cchMove++;
            
            }while(    cchAfter >= 0 
                   && (  !(arySLA[cchBefore].fSoftBreak)
                       || (action == WB_MOVEWORDRIGHT ? (arySLA[cchBefore].fWhiteSpace) : FALSE) ) );


            // if we are at the end of Thai text and have spaces, move past the spaces
            if(cchAfter == -1)
            {
                tp.AdvanceCp(cchMove + aryNodePos[offset + cchMove] - aryNodePos[offset]);
                
                while(IsCharBlank(tp.GetChar()) || (tp.GetChar() == WCH_NODE && !Classify (&tp, NULL)))
                {
                    aryNodePos[offset + cchMove] += 1;
                    if(tp.AdvanceCp(1) != 1)
                        break;
                }

            }

        }
        else
        {
            // go backwards in the attribute array until the first word break is encountered
            while(cchBefore > 0 && !(arySLA[cchBefore].fSoftBreak))
            {
                cchBefore--;
                cchMove --;
            }

        }

        // Adjust back to orignal position for calculating node characters.
        offset += fForward ? 0 : 1;

        Assert( offset + cchMove >= 0 &&
               offset + cchMove < aryNodePos.Size());
        Assert( (action & 1) ? aryNodePos [ offset + cchMove ] - aryNodePos [ offset ] >= 0
                             : aryNodePos [ offset + cchMove ] - aryNodePos [ offset ] <= 0);
        cchMove += aryNodePos[offset + cchMove] - aryNodePos[offset];

    }

    return cchMove;                            // Offset of where to break

}

/*
 *  CTxtPtr::PrepThaiTextForBreak(int action )
 *
 *  @mfunc
 *      Prepare text for Thai breaking by removing all nodes from the text to be
 *      itemized. paryNodePos will keep track of node positions so the tp can
 *      be moved correctly.
 *
 *  @rdesc
 *      TRUE/FALSE to indicate completion of preparing the itemize array.    
 */
#define BREAK_BEFORE     30
#define BREAK_AFTER      30
#define MOVE_BEFORE      16
#define MOVE_AFTER       15

BOOL
CTxtPtr::PrepThaiTextForBreak(
        BOOL fWordBreak,
        BOOL fForward,
        BOOL fCurrentIsNode,
        long cchText,
        CDataAry<TCHAR> *paryNodePos,
        TCHAR *paryItemize,
        long *pcchBefore,
        long *pcchAfter,
        long *pcchMove,
        long *plHoldNode)
{
    long cpSave = GetCp();
    long cchCtrlBefore = 0;
    long cchCtrlAfter = 0;
    long cchTotal = 0;
    long cch = 0;
    long cchBeforeMax = fWordBreak ? BREAK_BEFORE : MOVE_BEFORE;
    long cchAfterMax = fWordBreak ? BREAK_AFTER : MOVE_AFTER;
    BOOL  fOffEnd = FALSE;

    if(!paryNodePos || !paryItemize || !pcchBefore || !pcchAfter || !pcchMove)
        return FALSE;

    // Advance until cchAfterMax characters have passed or a non-ThaiType character
    // is encountered. If we encounter a breaking type of WCH_NODE we will
    // stop looking. Space type characters are included

    while(*pcchAfter < cchAfterMax)
    {
        TCHAR chCur = NextChar();
        
        if (IsClusterTypeChar( chCur ) ||
            wbkclsSpaceA == WordBreakClassFromCharClass( CharClassFromCh( chCur ) ) )
            *pcchAfter += 1;
        else if (chCur == WCH_NODE)
        {
            if(Classify( this, NULL ))
                break;

            cchCtrlAfter++;
        }
        else
        {
            if(chCur == 0)
                fOffEnd = TRUE;
            break;
        }
    }

    // (paulnel) We are making an assumption that a node character is always present at the
    // end of a story
    Assert(fOffEnd ? cchCtrlAfter > 0 : cchCtrlAfter >= 0);

    // Back up until cchAfterMax characters have passed or a non-ThaiType character
    // is encountered. If we encounter a breaking type of WCH_NODE we will
    // stop looking. Space type characters are included
    SetCp( cpSave );
    
    while ( *pcchBefore < cchBeforeMax )
    {
        TCHAR chCur = PrevChar();
        
        if (IsClusterTypeChar( chCur ) ||
            wbkclsSpaceA == WordBreakClassFromCharClass( CharClassFromCh( chCur ) ) )
            *pcchBefore += 1;
        else if (chCur == WCH_NODE)
        {
            if(_cp == 0 || Classify( this, NULL ))
                break;

            cchCtrlBefore++;
        }
        else
            break;
    }

    if (fForward && *pcchAfter == 0)
    {
        *pcchMove = 1;
        return FALSE;
    }
    else if (!fForward && *pcchBefore == 0)
    {
        *pcchMove = 0;
        return FALSE;
    }

    if(*pcchBefore == 0)
        cchCtrlBefore = 0;

    // Position the tp to the start of the Thai-type text. If we do not have
    // *pcchBefore, don't move for ctrl chars.
    Assert(*pcchBefore + cchCtrlBefore <= cpSave &&
           cpSave + *pcchAfter + cchCtrlAfter < cchText);

    SetCp(cpSave - *pcchBefore - cchCtrlBefore);

    cch = *pcchBefore + *pcchAfter + (fCurrentIsNode ? 0 : 1); // we need to include ourself
    cchTotal = cch + cchCtrlBefore + cchCtrlAfter;

    long cchValid;
    paryNodePos->Grow(cchTotal + 1);
    cchValid = GetRawText(cchTotal, *paryNodePos);

    Assert(cchValid == cchTotal);

    // Strip out any control characters
    long lCount = 0;
    long lTotal = 0;
    long lNode = 0;
    const TCHAR* pchCur = (*paryNodePos);

    while ( lCount < cch )
    {
        Assert( lTotal <= cchTotal );

        if (*pchCur != WCH_NODE)
        {
            // paulnel and john harding - If we have a space type character we want
            // to force it to be a normal space character so that Uniscribe keeps it
            // as part of the Thai type text run during itemization.
            if(wbkclsSpaceA != WordBreakClassFromCharClass( CharClassFromCh( *pchCur ) ))
                paryItemize[lCount] = *pchCur;
            else
                paryItemize[lCount] = _T(' ');

            (*paryNodePos)[lCount] = lNode;
            lCount++;
        }
        else
        {
            lNode++;
            if(lCount == *pcchBefore)
            {
                if(plHoldNode)
                    *plHoldNode += 1;
            }
        }
        
        lTotal++;
        pchCur++;
    }
    
    Assert(lCount <= cchTotal);

    // subtract the ending node so we don't walk off of the end.
    if(!fWordBreak)
    {
        (*paryNodePos)[lCount] = cchCtrlBefore + cchCtrlAfter + (fCurrentIsNode ? 1 : 0) - (fOffEnd ? 1 : 0);
    }
    else
    {
        (*paryNodePos)[lCount] = lNode;
    }
    paryNodePos->SetSize( lCount + 1 );
    
    Assert( cch == lCount );

    if(!fForward && !fWordBreak)
    {
        Assert( *pcchBefore > 0 );
        
        *pcchBefore -= 1;
        *pcchAfter +=1;
        *pcchMove -=1;
    }

    if(fCurrentIsNode)
        *pcchAfter -= 1;

    return TRUE;
}

/*
 *  CTxtPtr::ItemizeAndBreakRun
 *
 *  @mfunc
 *      Uses Uniscribe to itemize Thai type text and mark word and character boundaries.
 *
 *  @rdesc
 *      Offset from the beginning of the string to the run in which the desired
 *      breaking opportunities will be used. This will help match up with the
 *      aryNodePos array
 */
long 
CTxtPtr::ItemizeAndBreakRun(TCHAR* aryItemize, long* pcchBefore, long* pcchAfter, SCRIPT_LOGATTR* arySLA)
{
    HRESULT hr;
    CStackDataAry<SCRIPT_ITEM, 8> aryItems(Mt(CTxtPtrItemizeAndBreakRun_aryItems_pv));
    int cItems, nItem;
    long offset = 0;
    long cch = *pcchBefore + *pcchAfter + 1;

    // Prepare SCRIPT_ITEM buffer
    if (FAILED(aryItems.Grow(8)))
    {
        // We should always be able to grow to 8 itemse as we are based on
        // a CStackDataAry of this size.
        Assert(FALSE);
    }

    // Call ScriptItemize() wrapper in usp.cxx.
    if(g_bUSPJitState == JIT_OK)
        hr = ScriptItemize(aryItemize, cch, 16, 
                           NULL, NULL, &aryItems, &cItems);
    else
        hr = E_PENDING;

    if (FAILED(hr))
    {
        if(hr == USP10_NOT_FOUND)
        {
            g_csJitting.Enter();
            if(g_bUSPJitState == JIT_OK)
            {
                g_bUSPJitState = JIT_PENDING;
                
                // We must do this asyncronously.
                IGNORE_HR(GWPostMethodCall(_pMarkup->Doc(), 
                                           ONCALL_METHOD(CDoc, FaultInUSP, faultinusp), 
                                           0, FALSE, "CDoc::FaultInUSP"));

            }
            g_csJitting.Leave();
        }
        // ScriptItemize() failed (for whatever reason). We are unable to
        // break, so assume we've got a single word and return.
        goto done;
    }

    // Find the SCRIPT_ITEM containing cp.
    for(nItem = aryItems.Size() - 1;
        *pcchBefore < aryItems[nItem].iCharPos;
        nItem--);
    if (nItem < 0 || nItem + 1 >= aryItems.Size())
    {
        // Somehow the SCRIPT_ITEM array has gotten screwed up. We can't
        // break, so assume we've got a single word and return.
        goto done;
    }

    // Adjust cch to correspond to the text indicated by this item.
    cch = aryItems[nItem + 1].iCharPos - aryItems[nItem].iCharPos;
    *pcchBefore -= aryItems[nItem].iCharPos;
    *pcchAfter = cch - *pcchBefore - 1;

    Assert(*pcchBefore >= 0 && *pcchAfter >= 0 && *pcchBefore + *pcchAfter + 1 == cch);

    // do script break
    hr = ScriptBreak(aryItemize + aryItems[nItem].iCharPos, cch,
                     (SCRIPT_ANALYSIS *) &aryItems[nItem].a,
                     arySLA);

    if (FAILED(hr))
    {
        // ScriptBreak() failed (for whatever reason). We are unable to break,
        // so assume we've got a single word and return.
        goto done;
    }

    offset = *pcchBefore + aryItems[nItem].iCharPos;
done:
    return offset;                            // Offset of where to break
}


//
// Bookmark get/set methods
//

#define BOOKMARK_CURRENT_VERSION 2

// Structures
struct NastyCharsCounts
{
    long cchEmbed;
    long cchLineBreak;
    long cchBlockBreak;
    long cchWordBreak;
    long cchTxtSiteBreak;
    long cchTxtSiteEnd;
    long cchMiscNasty;
};

#define NUM_ADJACENT_CHARS 10

struct BookEnd
{
    long cp;

    NastyCharsCounts nastyCounts;

    char cchLeft, cchRight;
    TCHAR achLeft [ NUM_ADJACENT_CHARS ];
    TCHAR achRight [ NUM_ADJACENT_CHARS ];
};

struct Bookmark
{
    TCHAR chZero;

    char chVersion;
    char chDegenerate;

    BookEnd left;
    BookEnd right; // Only if not degenerate
};

/*
 * FindBookend:
 *
 * Synopsis: Looks forward or backward, as specified by fForward, trying
 *  to find the given bookend.  If found, the given CTxtPtr will be positioned
 *  at the bookend.
 *
 * Returns: TRUE if found, FALSE if not.
 */
static BOOL FindBookend( CTxtPtr * ptp, BookEnd & bookend, BOOL fForward )
{
    long    cpOrig     = ptp->_cp;
    long    cchLeft, cchRight, cch;
    BOOL    fFoundIt   = FALSE;
    TCHAR   achLeft[NUM_ADJACENT_CHARS], achRight[NUM_ADJACENT_CHARS];

    //
    // Make copies of the achLeft/Right strings, stripping out
    // non-text characters that don't exist now.
    //
    for( cch = 0, cchLeft = 0; cch < bookend.cchLeft; cch++ )
    {
        if( IsValidWideChar( bookend.achLeft[cch] ) )
            achLeft[cchLeft++] = bookend.achLeft[cch];
    }
    for( cch = 0, cchRight = 0; cch < bookend.cchRight; cch++ )
    {
        if( IsValidWideChar( bookend.achRight[cch] ) )
            achRight[cchRight++] = bookend.achRight[cch];
    }

    // Now scan along in the given direction, trying to match achLeft/Right
    for( ; ; )
    {
        long cpSave = ptp->_cp;

        if (0 == cchLeft)
        {
            // If there is no left text, automatically match
            fFoundIt = TRUE;
        }
        else
        {
            for( cch = 0; ptp->MoveChar( FALSE ) && ptp->GetChar() == achLeft[cch]; )
            {
                // Matched achLeft
                if( ++cch >= cchLeft )
                {
                    fFoundIt = TRUE;
                    break;
                }
            }
        }


        ptp->SetCp( cpSave );

        // Only check achRight if we matched achLeft.
        if( fFoundIt )
        {
            fFoundIt = FALSE;
            cch = 0;

            if (0 == cchRight)
            {
                fFoundIt = TRUE;
            }
            else
            {
                for( cch = 0; ptp->GetChar() == achRight[cch]; )
                {
                    // Matched achRight    
                    if( ++cch >= cchRight )
                    {
                        fFoundIt = TRUE;
                        break;
                    }

                    // out of characters
                    if( !ptp->MoveChar( TRUE ) )
                        break;
                }
            }
        }

        ptp->SetCp( cpSave );

        if( fFoundIt )
            return TRUE;

        if( !ptp->MoveChar( fForward ) )
            break;       
    }
    
    // Couldn't find it.    
    ptp->SetCp( cpOrig );
    return FALSE;
}

/*
 * Member: CTxtPtr::MoveToBookmark
 *
 * Synopsis: Moves this to the beginning of given bookmark, and moves
 *  pTxtPtrend to the end of the bookmark.  If bookmark can not be found,
 *  neither is moved.
 *
 * Returns: S_OK if found, S_FALSE if not.
 */
HRESULT CTxtPtr::MoveToBookmark( BSTR bstrBookmark, CTxtPtr *pTxtPtrEnd )
{
    HRESULT             hr      = S_OK;
    Bookmark            bm;
    long                cchBookmark;
    TCHAR *             pch;
    
    BOOL                fFoundIt;


    Assert( pTxtPtrEnd );
    
    //
    // Parse out and verify the bookmark string.
    //
    cchBookmark = FormsStringLen( bstrBookmark );
    
    cchBookmark *= sizeof(TCHAR);

    if (cchBookmark != sizeof( Bookmark ) &&
        cchBookmark != sizeof( Bookmark ) - sizeof( BookEnd ))
    {
        goto InvalidBookmark;
    }

    memcpy( & bm, & bstrBookmark[ 0 ], cchBookmark );

    cchBookmark /= sizeof(TCHAR);

    pch = (TCHAR *) & bm;

    for ( ; ; )
    {
        for (int i = 0 ; i < cchBookmark ; i++ )
            pch[i]--;

        if (bm.chZero == 0)
            break;
    }

    if (bm.chVersion > char( BOOKMARK_CURRENT_VERSION ))
        goto InvalidBookmark;


    //
    // Look for bookends
    //
    SetCp( bm.left.cp );
    fFoundIt =    FindBookend( this, bm.left, FALSE ) 
               || FindBookend( this, bm.left, TRUE );

    if( !bm.chDegenerate )
    {
        pTxtPtrEnd->SetCp( bm.right.cp );
        fFoundIt = fFoundIt && (    FindBookend( pTxtPtrEnd, bm.right, FALSE )
                                 || FindBookend( pTxtPtrEnd, bm.right, TRUE ) );
    }
    else
    {
        pTxtPtrEnd->SetCp( _cp );
    }


    // If we couldn't find it, return S_FALSE to notify caller.
    if( !fFoundIt )
    {
        hr = S_FALSE;
    }

Cleanup:    
    RRETURN1( hr, S_FALSE );
    
InvalidBookmark:
    hr = E_INVALIDARG;
    goto Cleanup;
}

static void CountNasties (
    CTxtPtr * pTxtPtr, long cp, NastyCharsCounts & nasties )
{
    long cchLeft;
    long cpSave = pTxtPtr->_cp;
    
    nasties.cchEmbed = 0;
    nasties.cchLineBreak = 0;
    nasties.cchBlockBreak = 0;
    nasties.cchWordBreak = 0;
    nasties.cchTxtSiteBreak = 0;
    nasties.cchTxtSiteEnd = 0;
    nasties.cchMiscNasty = 0;

    // Count word breaks.
    while( long(pTxtPtr->_cp) < cp )
    {
        nasties.cchWordBreak++;
        pTxtPtr->FindWordBreak( WB_MOVEWORDRIGHT );
    }

    pTxtPtr->SetCp( cpSave );
    cchLeft = cp - pTxtPtr->_cp;

    // Count everything else.
    while( cchLeft-- > 0)
    {
        BOOL    fBegin;
        
        if ( WCH_NODE == pTxtPtr->GetChar() )
        {
            switch( Classify( pTxtPtr, &fBegin ) )
            {
            case NODECLASS_NONE:        nasties.cchMiscNasty++;     break;
            case NODECLASS_SEPARATOR:   nasties.cchEmbed++;         break;
            case NODECLASS_NOSCOPE:     nasties.cchEmbed++;         break;
            case NODECLASS_LINEBREAK:   nasties.cchLineBreak++;     break;
            case NODECLASS_BLOCKBREAK:  nasties.cchBlockBreak++;    break;
            case NODECLASS_SITEBREAK:
                if( fBegin )
                    nasties.cchTxtSiteBreak++;
                else
                    nasties.cchTxtSiteEnd++;
                break;
            }
        }
        pTxtPtr->AdvanceCp( 1 );
    }

}

/*
 * ComputeAdjacent:
 *
 * Synopsis: Computes the text adjacent to the given TxtPtr and fills
 *  bookend appropriately.
 */
static void ComputeAdjacent (
    CTxtPtr * pTxtPtr, BookEnd & bookend )
{
    long    cch, cpSave;
    long    cchText = pTxtPtr->GetTextLength();
    TCHAR * pch;

    cpSave = pTxtPtr->GetCp();
    pch = bookend.achLeft;

    // Look left
    for( cch = 0, pTxtPtr->MoveChar( FALSE ); 
         pTxtPtr->_cp > 1 && cch < NUM_ADJACENT_CHARS;
         cch++, pTxtPtr->MoveChar( FALSE ) )
    {
        *pch++ = pTxtPtr->GetChar();
    }
    bookend.cchLeft = cch;

    // Look right, but make sure we start at text.
    pTxtPtr->SetCp( cpSave );
    
    if( pTxtPtr->GetChar() == WCH_NODE )
        pTxtPtr->MoveChar( TRUE );
    
    pch = bookend.achRight;
    for( cch = 0;
         long(pTxtPtr->_cp) < cchText - 1 && cch < NUM_ADJACENT_CHARS;
         cch++, pTxtPtr->MoveChar( TRUE ) )
    {
        *pch++ = pTxtPtr->GetChar();
    }
    bookend.cchRight = cch;
}

/*
 * Member: CTxtPtr::GetBookmark
 *
 * Synopsis: Gets a bookmark string represnting the positions of
 * this and pTxtPtrEnd, and fills *pbstrBookmark with the string.
 */
HRESULT CTxtPtr::GetBookmark( 
    BSTR *pbstrBookmark, 
    CTxtPtr *pTxtPtrEnd )
{
    HRESULT     hr      = S_OK;
    long        cchBookmark;
    TCHAR       achBookmark[ sizeof( Bookmark ) / sizeof( TCHAR ) + 1 ];
    Bookmark    bm;
    
    bm.chZero = 0;
    bm.chVersion = char(BOOKMARK_CURRENT_VERSION);
    bm.chDegenerate = pTxtPtrEnd->_cp == _cp;

    // Left edge
    bm.left.cp = _cp;
    SetCp( 1 );
    CountNasties( this, bm.left.cp, bm.left.nastyCounts );
    Assert( long(_cp) == bm.left.cp );
    
    ComputeAdjacent( this, bm.left );

    // Right edge
    if( !bm.chDegenerate )
    {
        bm.right.cp = pTxtPtrEnd->_cp;
        pTxtPtrEnd->SetCp( bm.left.cp );
        
        CountNasties( pTxtPtrEnd, bm.right.cp, bm.right.nastyCounts );
        Assert( long(pTxtPtrEnd->_cp) == bm.right.cp );
        ComputeAdjacent( pTxtPtrEnd, bm.right );
    }

    //
    // Garble the bookmark.
    //
    cchBookmark = sizeof( Bookmark );

    if (bm.chDegenerate)
        cchBookmark -= sizeof( BookEnd );

    memcpy( achBookmark, & bm, cchBookmark );

    cchBookmark /= sizeof( TCHAR );

    for ( ; ; )
    {
        BOOL fFoundZero = FALSE;

        for ( int i = 0 ; i < cchBookmark ; i++ )
            if (++achBookmark[i] == 0)
                fFoundZero = TRUE;

        if (!fFoundZero)
            break;
    }

    hr = THR( FormsAllocStringLen( achBookmark, cchBookmark, pbstrBookmark ) );

    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


/*
 * Member: CTxtPtr::IsPasswordChar
 *
 * Synopsis: Find out if _cp has the char format to indicate
 * that it is a password character. Used to bypass cluster movement
 * for cluster type text
 */
BOOL CTxtPtr::IsPasswordChar()
{
    long        ich;
    CTreePos *  ptp = _pMarkup->TreePosAtCp( _cp, &ich );

    if(ptp->IsText())
        return ptp->GetBranch()->GetCharFormat()->_fPassword;
    else
        return FALSE;
}
