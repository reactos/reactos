/*
 *  @doc    INTERNAL
 *
 *  @module _TEXT.H -- Declaration for a CTxtRun pointer |
 *
 *  CTxtRun pointers point at the plain text runs (CTxtArray) of the
 *  backing store and derive from CRunPtrBase via the CRunPtr template.
 *
 *  Copyright (c) 1995-1996, Microsoft Corporation. All rights reserved.
 */

#ifndef I__TEXT_H_
#define I__TEXT_H_
#pragma INCMSG("--- Beg '_text.h'")

#ifndef X__RUNPTR_H_
#define X__RUNPTR_H_
#include "_runptr.h"
#endif

#ifndef X_USP_HXX_
#define X_USP_HXX_
#include "usp.hxx"
#endif

MtExtern(CTxtPtr)

/*
 *  CTxtPtr
 *
 *  @class
 *      provides access to the array of characters in the backing store
 *      (i.e. <c CTxtArray>)
 *
 *  @base   public | CRunPtr<lt>CTxtArray<gt>
 *
 *  @devnote
 *      The state transitions for this object are the same as those for
 *      <c CRunPtrBase>.  <md CTxtPtr::_cp> simply caches the current
 *      cp (even though it can be derived from _iRun and _ich).  _cp is
 *      used frequently enough (and computing may be expensive) that
 *      caching the value is worthwhile.
 *
 *      CTxtPtr's *may* be put on the stack, but do so with extreme
 *      caution.  These objects do *not* float; if a change is made to
 *      the backing store while a CTxtPtr is active, it will be out
 *      of sync and may lead to a crash.  If such a situation may
 *      exist, use a <c CTxtRange> instead (as these float and keep
 *      their internal text && format run pointers up-to-date).
 *
 *      Otherwise, a CTxtPtr is a useful, very lightweight plain
 *      text scanner.
 */
class CTxtPtr : public CRunPtr<CTxtBlk>
{
public:
#if DBG==1
    BOOL Invariant( void ) const;       //@cmember  Invariant checking
    void MoveGapToEndOfBlock () const;
#endif

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTxtPtr))

    inline CTxtPtr(CMarkup * pMarkup);
    CTxtPtr(CMarkup * pMarkup, DWORD cp);   //@cmember  Constructor
    CTxtPtr(const CTxtPtr &tp);         //@cmember  Copy Constructor
    CTxtPtr() { }

    void    Reinit(CMarkup * pMarkup, DWORD cp);
    long    GetRawText( long cch, TCHAR *pch ); //@cmember  Fetch <p cch> characters
    long    GetPlainText(  long cch, TCHAR *pch ); // Fetch pch chars cnvrt crlf
    long    GetPlainTextLength ( long cch );
    TCHAR   NextCharCount(LONG& cch);   //@cmember Advance, GetChar, decrement
    TCHAR   NextChar();             //@cmember Advance to & return next char
    TCHAR   PrevChar();             //@cmember Backup to & return previous char
    TCHAR   GetChar();              //@cmember Fetch char at current cp
    TCHAR   GetPrevChar();          //@cmember Fetch char at previous cp
    BOOL    ReplaceChar(TCHAR chNew);   //@cmember Replace char at current cp
    long    GetTextLength() const   //@cmember Get total cch for this document
    {
        return ((CTxtArray *)_prgRun)->_cchText;
    }
    const TCHAR* GetPch(long & cchValid);//@cmember Get ptr to block of chars

                            //@cmember  Get ptr to a reverse block of chars
    const TCHAR* GetPchReverse(long & cchValidReverse, long * cchvalid = NULL);

    // The text array has its own versions of these methods (overuling
    // those in runptr base so that <md CTxtPtr::_cp> can be correctly
    // maintained.

    DWORD   BindToCp(DWORD cp); //@cmember  Rebinds text pointer to cp
    DWORD   SetCp(DWORD cp);    //@cmember  Sets the cp for the run ptr
    DWORD   GetCp() const       //@cmember  Gets the current cp
    {
        // NB!  we do not do invariant checking here so the floating
        // range mechanism can work OK
        return _cp;
    };
    LONG    AdvanceCp(LONG cch);    //@cmember  Advance cp by cch chars

    // Advance/backup/adjust safe over CRLF and UTF-16 word pairs
    LONG    AdjustCpCRLF();     //@cmember  Backup to start of DWORD char
    LONG    AdvanceCpCRLF();    //@cmember  Advance over DWORD chars
    LONG    BackupCpCRLF();     //@cmember  Backup over DWORD chars
    void    CheckMoveGap(DWORD cchLine);//@cmember If line has gap, move gap
    BOOL    IsAfterEOP();       //@cmember  Does current cp follow an EOP?
    BOOL    IsAtEOP();          //@cmember  Is current cp at an EOP marker?
    BOOL    IsAtBOSentence();   //@cmember  At beginning of a sentence?
    BOOL    IsAtBOWord();       //@cmember  At beginning of word?
    BOOL    IsAtEOWord();       //@cmember  At end of word?
    BOOL    IsAtMidWord();      //@cmember  In the middle of a word?


    // Search

    LONG FindComplexHelper (
        LONG cpMost, DWORD dwFlags, TCHAR const *, long cchToFind );

    LONG FindText (
        LONG cpMost, DWORD dwFlags, TCHAR const *, long cchToFind );

    LONG FindEOP(LONG cchMax);

    BOOL FindCrOrLf ( long cchMax );

    // Word break & MoveUnit support
    long    MoveChar(BOOL fForward);
    long    MoveCluster(BOOL fForward);
    long    MoveClusterEnd(BOOL fForward);
    LONG    FindWordBreak(INT action, BOOL fAutoURL=FALSE);//@cmember  Find next word break
    LONG    FindBOSentence(BOOL fForward);   //@cmember  Find beginning of sentence
    long    FindBlockBreak(BOOL fForward);

    long    AutoUrl_FindWordBreak( int nAction );
    BOOL    IsInsideUrl( long *pcpStart, long *pcpEnd );
    BOOL    FindUrl( BOOL fForward, BOOL fBegin );
    BOOL    IsPasswordChar();

    // Bookmark support
    HRESULT MoveToBookmark( BSTR bstrBookmark, CTxtPtr *pTxtPtrEnd );
    HRESULT GetBookmark( BSTR *pbstrBookmark, CTxtPtr *pTxtPtrEnd );
    
                            //@cmember  Replace <p cchOld> characters with
                            // <p cchNew> characters from <p pch>
    DWORD   ReplaceRange(LONG cchOld, DWORD cchNew, TCHAR const *pch);
    
                            //@cmember  Insert ch into the text stream cch times
    long    InsertRepeatingChar( LONG cch, TCHAR ch );

                                    //@cmember  Insert a range of text helper
                                    // for ReplaceRange
    long    InsertRange(DWORD cch, TCHAR const *pch);
    void    DeleteRange(DWORD cch); //@cmember  Delete range of text helper
                                    // for ReplaceRange
    DWORD       _cp;        //@cmember  character position in text stream

    //BUGBUG
    //@todo     (alexgo) see about removing the need to have a _ped member
    //down here.  This creates a cyclic dependency.  Also, having this as a
    //public method is bogus.  But it's used a lot by derived classes...
    CMarkup *   _pMarkup;       //@cmember  pointer to the overall text edit class;
                            //needed for things like the word break proc and
                            // used a lot by derived classes

    // We will protect any specific Thai related functions from the outside world
    // Keep the entry point to these through MoveCluster and FindWordBreak
private:
    LONG    FindThaiTypeWordBreak(INT action);
    BOOL    PrepThaiTextForBreak(BOOL fWordBreak,
                                 BOOL fForward,
                                 BOOL fCurrentIsNode,
                                 long cchText,
                                 CDataAry<TCHAR> *paryNodePos,
                                 TCHAR *paryItemize,
                                 long *pcchBefore,
                                 long *pcchAfter,
                                 long *pcchMove,
                                 long *plHoldNode=NULL);
    long    ItemizeAndBreakRun(TCHAR* aryItemize, long* pcchBefore, long* pcchAfter, SCRIPT_LOGATTR* arySLA);


};


// =======================   Misc. routines  ====================================================

void     TxCopyText(TCHAR const *pchSrc, TCHAR *pchDst, LONG cch);
LONG     TxFindEOP(const TCHAR *pchBuff, LONG cch);
INT      CALLBACK TxWordBreakProc(TCHAR const *pch, INT ich, INT cb, INT action);

inline
CTxtPtr::CTxtPtr ( CMarkup * pMarkup )
  : CRunPtr < CTxtBlk > ( (CRunArray *) & pMarkup->_TxtArray )
{
    _pMarkup = pMarkup;
    _cp = 0;
}

#pragma INCMSG("--- End '_text.h'")
#else
#pragma INCMSG("*** Dup '_text.h'")
#endif
