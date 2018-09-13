/*
 *  @doc
 *
 *  @module _RANGE.H -- CTxtRange Class |
 *
 *      This class implements the internal text range and the TOM ITextRange
 *
 *  Authors: <nl>
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Murray Sargent
 *      Alex Gounares (floating ranges, etc.)
 *
 *  Copyright (c) 1995-1996, Microsoft Corporation. All rights reserved.
 */

#ifndef I__RANGE_H_
#define I__RANGE_H_
#pragma INCMSG("--- Beg '_range.h'")

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

// Round trip problems with the following???
#define FPPTS_TO_TWIPS(x) ((long)(20*x + 0.5))
#define TWIPS_TO_FPPTS(x) (((float)(x)) * (float)0.05)

// Font sizes on the toolbar are from -2 to 4, internally they are 1 to 7
#define FONT_INDEX_SHIFT 3

// the maximum number of chars the URL word breaker will go
#define MAX_URL_WORD_BREAK_CHAR_COUNT 1024

class CTxtFont;
class CTextSelectionRecord;
class CAnchorElement;

MtExtern(CTextSegment)
MtExtern(CTxtRange)

/*
 *  FINDWORD_TYPE
 *
 *  @enum   defines the different cases for finding a word
 */
enum FINDWORD_TYPE {
    FW_EXACT    = 1,        //@emem Finds the word exactly (no extra chars)
    FW_INCLUDE_TRAILING_WHITESPACE = 2, //@emem find the word plus the
                            // following whitespace (ala double-clicking)
    FINDWORD_TYPE_Last_Enum
};

enum MOVES
{
    MOVE_START = -1,
    MOVE_IP = 0,
    MOVE_END = 1,
    MOVE_Last_Enum
};

enum MATCHES
{
    MATCH_UNTIL = 0,
    MATCH_WHILE = 1
};

enum STATUS
{
    STATUS_OFF = 0,
    STATUS_ON,
    STATUS_PARTIAL
};

// used by UrlAutodetector and associated helper functions
enum {
    AUTOURL_TEXT_PREFIX,
    AUTOURL_HREF_PREFIX
};

typedef enum {
    MV_DIR_Left,
    MV_DIR_Right
} MV_DIR;

typedef enum {
    PTR_ADJ_In,
    PTR_ADJ_Out
} PTR_ADJ;

typedef enum {
    CR_Text,
    CR_NoScope,
    CR_Intrinsic,
    CR_Boundary,
    CR_Failed
} CLING_RESULT;


typedef struct {
    BOOL  fWildcard;                      // true if tags have a wildcard char
    const TCHAR* pszPattern[2];           // the text prefix and the href prefix
}
AUTOURL_TAG;


/*
 *  CTxtRange
 *
 *  @class
 *      The CTxtRange class implements RichEdit's text range, which is the
 *      main conduit through which changes are made to the document.
 *      The range inherits from the rich-text ptr, adding a signed length
 *      insertion-point char-format index, and a ref count for use when
 *      instantiated as a TOM ITextRange.  The range object also contains
 *      a flag that reveals whether the range is a selection (with associated
 *      screen behavior) or just a simple range.  This distinction is used
 *      to simplify some of the code.
 *
 *      Some methods are virtual to allow CTextSelectionRecord objects to facilitate
 *      UI features and selection updating.
 *
 *      See tom.doc for lots of discussion on range and selection objects and
 *      on all methods in ITextRange, ITextSelection, ITextFont, and ITextPara.
 */


class CTxtRange 
{
    friend class CTxtFont;
    friend class CWordLikeTxtRange;

protected:

    IMarkupPointer * _pLeft;       // Left boundary of the range
  
    IMarkupPointer * _pRight;      // Right boundary of the range 
   
//    long    _cch;
   
    CMarkup * _pMarkup;

    CTreeNode * _pNodeHint;

    CElement * _pElemContainer;

    DWORD       _snLast;            // Serial number of notification to ignore

    HRESULT     ValidatePointers();

    HRESULT KeepRangeLeftToRight();

    BOOL    IsRangeCollapsed();

    CLING_RESULT
    ClingToText( 
        IMarkupPointer *    pPointer, 
        IMarkupPointer *    pBoundary, 
        MV_DIR              eDir,
        PTR_ADJ             eAdj = PTR_ADJ_In );
            
#if !defined(MW_MSCOMPATIBLE_STRUCT)
    union
    {
        DWORD _dwFlagsVar;
        struct
        {
#endif
            DWORD   _fExtend                  : 1; // Leav "other" end unchanged
            DWORD   _fSel                     : 1; // True iff this range is a CTextSelectionRecord
            DWORD   _fDisableWordLikeTxtRange : 1;
            DWORD   _fUnused                  : 29;

#if !defined(MW_MSCOMPATIBLE_STRUCT)
        };
    };
        DWORD& _dwFlags() { return _dwFlagsVar; }
#else
        DWORD& _dwFlags() { return *(((DWORD*)&_cch) +1); }
#endif

    void ClearFlags()
    {
        _fExtend = FALSE;
        _fSel = FALSE;
        _fDisableWordLikeTxtRange = FALSE;
    }

    void    LaunderSpacesForRange(LONG cpMin, LONG cpMost);
    HRESULT CheckOwnerSiteOrSelection(ULONG cmdID);

    HRESULT ExecBlockFormat(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);


public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTxtRange))

    HRESULT GetLeft( IMarkupPointer * ptp );

    HRESULT GetLeft( CMarkupPointer * ptp );

    HRESULT GetRight( IMarkupPointer * ptp );

    HRESULT GetRight( CMarkupPointer * ptp );

    HRESULT GetLeftAndRight( CMarkupPointer * pLeft, CMarkupPointer * pRight );

    HRESULT SetLeft( IMarkupPointer * ptp );

    HRESULT SetRight( IMarkupPointer * ptp );

    HRESULT SetLeftAndRight( IMarkupPointer * pLeft, IMarkupPointer * pRight, BOOL fAdjustPointers = TRUE  );

    CElement * GetContainer()
    {
        return _pElemContainer;
    }

    CTreeNode * LeftNode();

    CTreeNode * RightNode();

    HRESULT collapse( VARIANT_BOOL fStart );

    WHEN_DBG( BOOL Invariant ( ) const; )

    WHEN_DBG( void DumpTree(); )

    CTxtRange ( const CTxtRange & rg );

    CTxtRange ( CMarkup  * pMarkup,
                CElement * pElemContainer = NULL );

    virtual ~ CTxtRange ( );

    void SetNodeHint(CTreeNode * pNodeHint) 
    { 
        AssertSz( 0 , "SetNodeHint called" ); 
        CTreeNode::ReplacePtr(&_pNodeHint, pNodeHint); 
    }
    
    CTreeNode * GetNodeHint() 
    { 
        AssertSz( 0, "GetNodeHint called" );
        return _pNodeHint; 
    }

    BOOL IsOrphaned ( ) { return !GetMarkup()->Root(); }

    BOOL IsDocLoading ( ) { return GetMarkup()->Doc()->IsLoading(); }
    BOOL IsMarkupLoading ( ) { return ! GetMarkup()->GetLoaded(); }

    ELEMENT_TAG GetDefaultBlockTag()
            { return GetMarkup()->Doc()->GetDefaultBlockTag(); }

    HRESULT ValidateReplace (
        BOOL fNonEmptyReplace,
        BOOL fPerformCorrection = FALSE );

    CMarkup *GetMarkup() { return _pMarkup; }

    // Internal cp/cch methods

//    long    GetCch ( ) const;
    BOOL    CheckTextLength (long cch); //@cmember Says if cch chars can fit
                                        //@cmember Used after possibly changing
    void    CheckChange(long cpSave,    //  _cp, _cch to set selection-changed
                long cchSave);          //  flag and choose _cch <--> _fExtend

    // GetRange() is faster than calling GetCpMin() and GetCpMost();
    long    GetCpMin () const;          //@cmember Get cp of first char in range
    long    GetCpMost () const;         //@cmember Get cp just beyond last char in range
                                        //@cmember Get range ends and count
#if DBG==1
    long    GetRange ( long & cpMin, long & cpMost, BOOL fIsValid = TRUE ) const;
#else
    long    GetRange ( long & cpMin, long & cpMost ) const;
#endif
    void    GetRange (long cp, long cch, long & cpMin, long & cpMost);

    INT GetCurrentFontHeight(INT *pyDescent, BOOL fTrueHeight) const;

    int IsRangeEmpty(LONG iRunStart, LONG iRunEnd);

    BOOL    Set ( long cp, long cch );

    void    SetExtend ( unsigned f ) { _fExtend = !!f; }
    BOOL    GetExtend ( ) { return _fExtend; }

    // Range specific methods

    long Advance ( long cch );
    void FlipRange ( );
    virtual BOOL Update(
        BOOL fScrollIntoView,
        BOOL fIsSelAnchor,
        CLayout *pCurrentLayout = NULL);

    virtual HRESULT ReplaceRange (
        long cchNew, TCHAR const * pch, BOOL fCanMerge = TRUE );

    //
    // Stuff related to hitting <return>
    //

    HRESULT SplitForm ( );

    HRESULT SplitLine ( );

    // Structured formatting

    ELEMENT_TAG GetBlockFormat(BOOL fCheckForList = FALSE);
    HRESULT     ApplyBlockFormat(ELEMENT_TAG etag);
    HRESULT     OutdentSelection(BOOL fRemoveListsOnly = FALSE);
    HRESULT     SetSelectionBlockDirection(htmlDir atDir);
    HRESULT     SetSelectionInlineDirection(htmlDir atDir);
    HRESULT     NukePhraseElementsUnderBlockElement(
                        LONG iRunBegin, LONG iRunEnd,
                        CElement *pElementBlockNukeBelowMe);
    void        RestoreComposeFontAfterBlockFormatting(
                        CElement *pElementBlock, LONG iRunStart, LONG iRunEnd);


    HRESULT     ApplyList (ELEMENT_TAG etagList,
                            long iRunStart, long iRunFinish );

    HRESULT FormatRange ( CElement * pElement,
                          BOOL (* pfnUnFormatCriteria) ( CTreeNode * ) );

    HRESULT UnFormatRange (
        BOOL (* pfnUnFormatCriteria) ( CTreeNode * ) );

    virtual HRESULT AdjustForInsert ( );

    HRESULT NormalizeBlockElements ( );
    HRESULT GetEndRuns ( long * piRunStart, long * piRunFinish) const;
    HRESULT GetEndRunsIgnoringSpaces ( long * piRunStart, long * piRunFinish);

    HRESULT GetExtendedSelectionInfo (
        long * piRunStart,
        long * piRunFinish,
        CElement * (CElement::*pfnSearchCriteria) ( CTxtSite * ) );

    //
    // QueryStatus/Exec support
    //

    STDMETHOD (QueryStatus) (
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext,
            BOOL fStopBobble);

    STDMETHOD (Exec) (
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut,
            BOOL fStopBobble);

    virtual CElement *GetCommonContainer();

    virtual CTreeNode *GetCommonNode();
    virtual BOOL OwnedBySingleFlowLayout();
    virtual BOOL SelectionInOneFlowLayout();

    CFlowLayout *GetCommonFlowLayout()
    {
        return GetCommonNode()->GetFlowLayout();
    }

    BOOL FSupportsHTML();

    void QueryStatusJustify (
        MSOCMD * pCmd, DWORD nCmdID, MSOCMDTEXT * pcmdtext );

    void QueryStatusBlockDirection (
        MSOCMD * pCmd, DWORD nCmdID, MSOCMDTEXT * pcmdtext );

    STATUS GetCurrentRenderFormat( int nCmdID );

    STATUS GetCurrentAlignment( int nCmdID );

    STATUS GetCurrentBlockDirection( int nCmdID );

    HRESULT Delete ( DWORD flags );

    HRESULT ExecFormatAnchor(CLayout * pLayoutSelected,
                              BOOL      fShowUI,
                              VARIANT * pvarArg,
                              BOOL      fLink);

    HRESULT ExecRemoveAnchor(CLayout * pLayoutSelected);

    HRESULT GetAnchorElements(CLayout * pLayoutSelected,
                              CPtrAry < CElement * > * paryElement);

    CTreeNode * FindCommonElement (
        CTreeNode * pNodeSiteSelected, ELEMENT_TAG etag = ETAG_UNKNOWN );

    void QueryStatusAnchor(
        CLayout * pLayoutSelected, MSOCMD * pCMD, DWORD nCmdID );

    // Complex Text Helper
    inline BOOL IsDefaultAlignment(BOOL fRTL, htmlBlockAlign atAlign)
    {
        return (fRTL ? atAlign == htmlBlockAlignRight : atAlign == htmlBlockAlignLeft); 
    }
    //
    // Rich-text methods
    //

    // Get/Set Char/Para Format methods
    void    GetCharFormatForRange(const CCharFormat * * ppcf) const;

    // Find enclosing unit methods

    void    FindParagraph(long *pcpMin, long *pcpMost) const;
    void    FindSentence (long *pcpMin, long *pcpMost) const;
    void    FindWord     (long *pcpMin, long *pcpMost,
                            FINDWORD_TYPE type)const;

    HRESULT Expander (
        long Unit, BOOL fExtend,
        long * pDelta, long * pcpMin, long * pcpMost );

    HRESULT Mover ( long Unit, long Count, long * pDelta, MOVES Mode );

    HRESULT Finder (
        BSTR bstr, long Count, long Flags, long * pDelta, MOVES Mode );

    // Returns font name for the selected text
    HRESULT GetFontNameForRange(VARIANT * pvarName);
    // Returns font size for the selected text
    HRESULT GetFontSizeForRange(VARIANT * pvarSize);
    // Returns text color for the selected text
    HRESULT GetFontColorForRange(VARIANT * pvarColor);
    // Returns back color for the selected text
    HRESULT GetFontBackColorForRange(VARIANT * pvarBackColor);

    HRESULT AdjustCRLF ( long dDir, long * cchAdjust = 0 );

    long    FindWordBreak(INT action);

    void    ValidateRange();

    //
    // BUGBUG (johnbed) RaminH: make this the CAutoRange::SaveHTMLToStream
    //
    
    HRESULT SaveHTMLToStream (CStreamWriteBuff * pswb, DWORD dwMode);

    HRESULT UrlAutodetector(
            BOOL    fUserEnteredChar,
            BOOL    fPasting,
            BOOL *  fpChangedText,
            BOOL    fAllowInBrowseMode = FALSE);

    BOOL    AutoUrl_UpdateAnchorFromHRefIfSynched( LPCTSTR pchHref, CAnchorElement* pAnchorElement );

    BOOL    AutoUrl_GrabAnchorDimensions( CAnchorElement* pAnchorElement, DWORD* pcpStart,
                                            DWORD* pcpEnd );

    BOOL    AutoUrl_CloseQuotes( );

    LONG    AutoUrl_FindWordBreak(int action);

    static BOOL AutoUrl_DoAutoDetectOnInsert(TCHAR ch1, TCHAR ch2);

    static BOOL AutoUrl_IgnoreBoundary(TCHAR ch1, TCHAR ch2);

    // Set the range to encompass text influenced by given element
    HRESULT SetTextRangeToElement( CElement * pElement );

    // Helper function to reparse and insert the HTML text passed in.

    HRESULT PasteHTMLToRange (
        const TCHAR * pStr, long dwCount );

    // Helper function to open up a file and insert it into the range.
    HRESULT PasteFileToRange (const TCHAR * lptszParam );

    // Helper function to fetch a BSTR from an HTML stream in a given mode
    HRESULT GetBstrHelper(BSTR *pbStr, DWORD dwSaveMode, DWORD dwStrWrBuffFlags);

    // Helper function to fetch a BSTR containing the HTML text
    HRESULT GetBstrHTML(BSTR *pbstr);

    // Helper function to get fetch a BSTR containing the plaintext
    HRESULT GetBstrSimpleText(BSTR *pbstr);

    typedef enum {
        DTSB_NODELETE,    // Break char found but cannot delete
        DTSB_NOBREAKCHAR, // No break char at current cp
                          //    go ahead with normal deletion
        DTSB_DELETED,     // Break char found and deleted associated site
        DTSB_Last_Enum
    } DTSB_RET;

    HRESULT DeleteTxtSiteBreak(BOOL fDirForward, DTSB_RET &dtsbRet);

    VOID LaunderSpaces(LONG cp, LONG cch);

private:
    CTreeNode *  GetNode( BOOL fLeft );

    HRESULT InitPointers();
    HRESULT FlipRangePointers();


    // begin UrlAutodetector helper methods

#define AUTOURL_WILDCARD_CHAR   _T('\b')

#define AUTOURL_MAXBUF          256

    static AUTOURL_TAG s_urlTags[ ];

    void    AutoUrl_EstablishSearchRange(
            long *  pcch,
            DWORD * pcpSearchStart,
            DWORD * pcpSearchStop,
            BOOL    fUserEnteredChar,
            BOOL    fPasting);

    void AutoUrl_ApplyPattern( CStackDataAry<TCHAR, AUTOURL_MAXBUF>* paryDestGA, int iIndexDest,
                                 const TCHAR* pszSourceText, int iIndexSrc,
                                 AUTOURL_TAG*  ptag );

    HRESULT AutoUrl_MakeLink( CAnchorElement** ppAnchorElement, DWORD cpStart, DWORD cpEnd );

    void AutoUrl_SetUrl( const TCHAR* pszAnchorText, DWORD cpWordStart, DWORD cpWordEnd,
                                CAnchorElement* pAnchorElement,
                                AUTOURL_TAG* ptag, BOOL fQuote );

    void AutoUrl_GetWord( DWORD cpStart, DWORD cpEnd, CStackDataAry<TCHAR, AUTOURL_MAXBUF>* paryWordGA,
                                DWORD* pcpWordStart, DWORD* pcpWordEnd,
                                CAnchorElement** ppAnchorElement, BOOL* pfQuote,
                                DWORD* pcpNext, BOOL * pfMoreWords);

    BOOL AutoUrl_IsAutodetectable( const TCHAR* szString, int index, AUTOURL_TAG** ppTag );
    // end UrlAutodetector helper methods

#pragma warning(disable:4702)
};
#pragma warning(default:4702)

    //
    //
    //

class CWordLikeTxtRange
{
public:
    CWordLikeTxtRange(CTxtRange * pRange);
    ~CWordLikeTxtRange();
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
    long _cp;
    long _cch;
    CTxtRange * _pRange;
};

    //
    //
    //

//+----------------------------------------------------------------------------
//
//  Class:      CRangeRestorer (rr)
//
//  Purpose:    Manipulates the document (ped) this range lives in such that
//              the range can survive complex document operations.
//
//-----------------------------------------------------------------------------

class CRangeRestorer
{
public:

    CRangeRestorer ( CTxtRange * pRange );

    HRESULT Set ( );

    HRESULT Restore ( );

private:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))

    BOOL _fMarked;

    CTxtRange * _pRange;
};

// Useful Unicode range definitions for use with MoveWhile/Until methods

#define CodeRange(n, m) 0x8000000 | ((m) - (n)) << 16 | n

#define CR_ASCII        CodeRange(0x0, 0x7f)
#define CR_ANSI         CodeRange(0x0, 0xff)
#define CR_ASCIIPrint   CodeRange(0x20, 0x7e)
#define CR_Latin1       CodeRange(0x20, 0xff)
#define CR_Latin1Supp   CodeRange(0xa0, 0xff)
#define CR_LatinXA      CodeRange(0x100, 0x17f)
#define CR_LatinXB      CodeRange(0x180, 0x24f)
#define CR_IPAX         CodeRange(0x250, 0x2af)
#define CR_SpaceMod     CodeRange(0x2b0, 0x2ff)
#define CR_Combining    CodeRange(0x300, 0x36f)
#define CR_Greek        CodeRange(0x370, 0x3ff)
#define CR_BasicGreek   CodeRange(0x370, 0x3cf)
#define CR_GreekSymbols CodeRange(0x3d0, 0x3ff)
#define CR_Cyrillic     CodeRange(0x400, 0x4ff)
#define CR_Armenian     CodeRange(0x530, 0x58f)
#define CR_Hebrew       CodeRange(0x590, 0x5ff)
#define CR_BasicHebrew  CodeRange(0x5d0, 0x5ea)
#define CR_HebrewXA     CodeRange(0x590, 0x5cf)
#define CR_HebrewXB     CodeRange(0x5eb, 0x5ff)
#define CR_Arabic       CodeRange(0x600, 0x6ff)
#define CR_BasicArabic  CodeRange(0x600, 0x652)
#define CR_ArabicX      CodeRange(0x653, 0x6ff)
#define CR_Devengari    CodeRange(0x900, 0x97f)
#define CR_Bengali      CodeRange(0x980, 0x9ff)
#define CR_Gurmukhi     CodeRange(0xa00, 0xa7f)
#define CR_Gujarati     CodeRange(0xa80, 0xaff)
#define CR_Oriya        CodeRange(0xb00, 0xb7f)
#define CR_Tamil        CodeRange(0xb80, 0xbff)
#define CR_Teluga       CodeRange(0xc00, 0xc7f)
#define CR_Kannada      CodeRange(0xc80, 0xcff)
#define CR_Malayalam    CodeRange(0xd00, 0xd7f)
#define CR_Thai         CodeRange(0xe00, 0xe7f)
#define CR_Lao          CodeRange(0xe80, 0xeff)
#define CR_GeorgianX    CodeRange(0x10a0, 0xa0cf)
#define CR_BascGeorgian CodeRange(0x10d0, 0x10ff)
#define CR_Hanguljamo   CodeRange(0x1100, 0x11ff)
#define CR_LatinXAdd    CodeRange(0x1e00, 0x1eff)
#define CR_GreekX       CodeRange(0x1f00, 0x1fff)
#define CR_GenPunct     CodeRange(0x2000, 0x206f)
#define CR_SuperScript  CodeRange(0x2070, 0x207f)
#define CR_SubScript    CodeRange(0x2080, 0x208f)
#define CR_SubSuperScrp CodeRange(0x2070, 0x209f)
#define CR_Currency     CodeRange(0x20a0, 0x20cf)
#define CR_CombMarkSym  CodeRange(0x20d0, 0x20ff)
#define CR_LetterLike   CodeRange(0x2100, 0x214f)
#define CR_NumberForms  CodeRange(0x2150, 0x218f)
#define CR_Arrows       CodeRange(0x2190, 0x21ff)
#define CR_MathOps      CodeRange(0x2200, 0x22ff)
#define CR_MiscTech     CodeRange(0x2300, 0x23ff)
#define CR_CtrlPictures CodeRange(0x2400, 0x243f)
#define CR_OptCharRecog CodeRange(0x2440, 0x245f)
#define CR_EnclAlphaNum CodeRange(0x2460, 0x24ff)
#define CR_BoxDrawing   CodeRange(0x2500, 0x257f)
#define CR_BlockElement CodeRange(0x2580, 0x259f)
#define CR_GeometShapes CodeRange(0x25a0, 0x25ff)
#define CR_MiscSymbols  CodeRange(0x2600, 0x26ff)
#define CR_Dingbats     CodeRange(0x2700, 0x27bf)
#define CR_CJKSymPunct  CodeRange(0x3000, 0x303f)
#define CR_Hiragana     CodeRange(0x3040, 0x309f)
#define CR_Katakana     CodeRange(0x30a0, 0x30ff)
#define CR_Bopomofo     CodeRange(0x3100, 0x312f)
#define CR_HangulJamo   CodeRange(0x3130, 0x318f)
#define CR_CJLMisc      CodeRange(0x3190, 0x319f)
#define CR_EnclCJK      CodeRange(0x3200, 0x32ff)
#define CR_CJKCompatibl CodeRange(0x3300, 0x33ff)
#define CR_Hangul       CodeRange(0x3400, 0x3d2d)
#define CR_HangulA      CodeRange(0x3d2e, 0x44b7)
#define CR_HangulB      CodeRange(0x44b8, 0x4dff)
#define CR_CJKIdeograph CodeRange(0x4e00, 0x9fff)
#define CR_PrivateUse   CodeRange(0xe000, 0xf800)
#define CR_CJKCompIdeog CodeRange(0xf900, 0xfaff)
#define CR_AlphaPres    CodeRange(0xfb00, 0xfb4f)
#define CR_ArabicPresA  CodeRange(0xfb50, 0xfdff)
#define CR_CombHalfMark CodeRange(0xfe20, 0xfe2f)
#define CR_CJKCompForm  CodeRange(0xfe30, 0xfe4f)
#define CR_SmallFormVar CodeRange(0xfe50, 0xfe6f)
#define CR_ArabicPresB  CodeRange(0xfe70, 0xfefe)
#define CR_HalfFullForm CodeRange(0xff00, 0xffef)
#define CR_Specials     CodeRange(0xfff0, 0xfffd)

#pragma INCMSG("--- End '_range.h'")
#else
#pragma INCMSG("*** Dup '_range.h'")
#endif
