//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       htmtags.hxx
//
//  Contents:   TAGDESC
//              ELEMENT_TAG
//              CHtmTag
//              CHtmParseCtx
//
//----------------------------------------------------------------------------

#ifndef I_HTMTAGS_HXX_
#define I_HTMTAGS_HXX_
#pragma INCMSG("--- Beg 'htmtags.hxx'")

#ifndef X_TAGS_H_
#define X_TAGS_H_
#include "tags.h"
#endif

#ifndef X_ASSOC_HXX_
#define X_ASSOC_HXX_
#include "assoc.hxx"
#endif

class CElement;
class CTreeNode;
class CBufferedStr;
class CTreePos;
class CHtmTagStm;

//+---------------------------------------------------------------------------
//
// CTagDesc
//
//----------------------------------------------------------------------------

enum TAGDESC_FLAGS
{
    TAGDESC_TEXTLESS                = 0x00000001,    // No text in a textrun should point to this
    TAGDESC_BLOCKELEMENT            = 0x00000002,    // causes a line break
    TAGDESC_LIST                    = 0x00000004,    // LI, DD, etc
    TAGDESC_LISTITEM                = 0x00000008,    // possibly preceded by a bullet/numeral/etc
    TAGDESC_HEADER                  = 0x00000010,    // H1, H2, etc
    TAGDESC_EDITREMOVABLE           = 0x00000020,    // Removed by "apply normal format"
    TAGDESC_BLKSTYLEDD              = 0x00000040,    // Block style in drop down combo
    TAGDESC_ALIGN                   = 0x00000080,    // element has alignment
    TAGDESC_CONDITIONALBLOCKELEMENT = 0x00000100,    // block element unless width specified
    TAGDESC_WONTBREAKLINE           = 0x00000200,    // block element's that dont cause the line breaker break a line (TABLE)
    TAGDESC_OWNLINE                 = 0x00000400,    // These tags have their own line.
    TAGDESC_ACCEPTHTML              = 0x00000800,    // Autodetect URLs when your ped has this.
    TAGDESC_UNUSED_0                = 0x00001000,    // - unused -
    TAGDESC_LITCTX                  = 0x00002000,    // Don't notify parent context; just create context and eat text (TITLE)
    TAGDESC_LITERALTAG              = 0x00004000,    // Taglike markup inside the scope of this tag is text
    TAGDESC_LITERALENT              = 0x00008000,    // Entitylike markup inside the scope of this tag is text
    TAGDESC_SPECIALTOKEN            = 0x00010000,    // Call SpecialToken when tag is tokenized
    TAGDESC_XSMBEGIN                = 0x00020000,    // Need to know XSM state before outputting begin tag
    TAGDESC_SAVEINDENT              = 0x00040000,    // Causes an indent while saving
    TAGDESC_SAVENBSPIFEMPTY         = 0x00080000,    // Element requires an nbsp if empty
    TAGDESC_SAVEALWAYSEND           = 0x00100000,    // Save an end tag even if _fExplicitEndTag is not set
    TAGDESC_SAVETAGOWNLINE          = 0x00200000,    // Save the tag on its own line
    TAGDESC_NEVERSAVEEND            = 0x00400000,    // Never save the end tag (BASE)
    TAGDESC_SPLITBLOCKINLIST        = 0x00800000,    // When editing split if this block is in a list
    TAGDESC_LOGICALINVISUAL         = 0x01000000,    // Element has logical text for visual page
    TAGDESC_CONTAINER               = 0x02000000,    // This flag is set for container tags: BODY, TEXTAREA, BUTTON, MARQUEE, RICHTEXT
    TAGDESC_WAITATSTART             = 0x04000000,    // Stop postparser at start of tag
    TAGDESC_WAITATEND               = 0x08000000,    // Stop postparser at end of tag
    TAGDESC_DONTWAITFORINPLACE      = 0x10000000,    // Stop but don't wait for in-place activation
    TAGDESC_SLOWPROCESS             = 0x20000000,    // Postparser needs to carefully examine this tag
    TAGDESC_ENTER_TREE_IMMEDIATELY  = 0x40000000,    // the tag needs to enter tree immediately
};

// CHtmTag --------------------------------------------------------------------

class CHtmTag
{
public:

    // Sub-types

    struct CAttr
    {
        TCHAR *     _pchName;       // Pointer to attribute name (NULL terminated)
        ULONG       _cchName;       // Count of attribute name characters
        TCHAR *     _pchVal;        // Pointer to attribute value (NULL terminated)
        ULONG       _cchVal;        // Count of attribute value characters
        ULONG       _ulLine;        // Source line where attribute begins
        ULONG       _ulOffset;      // Source offset where attribute begins
    };

    // Methods

    void            Reset()                     { *(DWORD *)this = 0; }

    void            SetTag(ELEMENT_TAG etag)    { _etag = (BYTE)etag; }
    ELEMENT_TAG     GetTag()                    { return((ELEMENT_TAG)_etag); }
    BOOL            Is(ELEMENT_TAG etag)        { return(etag == (ELEMENT_TAG)_etag); }
    BOOL            IsBegin(ELEMENT_TAG etag)   { return(Is(etag) && !IsEnd()); }
    BOOL            IsEnd(ELEMENT_TAG etag)     { return(Is(etag) &&  IsEnd()); }

    void            SetNextBuf()                { _bFlags |= TAGF_NEXTBUF; }
    BOOL            IsNextBuf()                 { return(!!(_bFlags & TAGF_NEXTBUF)); }
    void            SetRestart()                { _bFlags |= TAGF_RESTART; }
    BOOL            IsRestart()                 { return(!!(_bFlags & TAGF_RESTART)); }
    void            SetEmpty()                  { _bFlags |= TAGF_EMPTY; }
    BOOL            IsEmpty()                   { return(!!(_bFlags & TAGF_EMPTY)); }
    void            SetEnd()                    { _bFlags |= TAGF_END; }
    BOOL            IsEnd()                     { return(!!(_bFlags & TAGF_END)); }
    void            SetTiny()                   { _bFlags |= TAGF_TINY; }
    BOOL            IsTiny()                    { return(!!(_bFlags & TAGF_TINY)); }
    void            SetAscii()                  { _bFlags |= TAGF_ASCII; }
    BOOL            IsAscii()                   { return(!!(_bFlags & TAGF_ASCII)); }
    void            SetScriptCreated()          { _bFlags |= TAGF_SCRIPTED; }
    BOOL            IsScriptCreated()           { return(!!(_bFlags & TAGF_SCRIPTED)); }
    void            SetDefer()                  { _bFlags |= TAGF_DEFER; }
    BOOL            IsDefer()                   { return(!!(_bFlags & TAGF_DEFER)); }
#ifdef VSTUDIO7
    void            SetDerivedTag()             { Assert(_etag != ETAG_SCRIPT); _bFlags |= TAGF_DEFER; }
    BOOL            IsDerivedTag()              { return ((_etag != ETAG_SCRIPT) && !!(_bFlags & TAGF_DEFER)); }
#endif //VSTUDIO7
    BOOL            IsSource()                  { return(GetTag() == ETAG_RAW_SOURCE); }

    TCHAR *         GetPch()                    { Assert(!IsTiny() && !IsSource()); return(_pch); }
    void            SetPch(TCHAR * pch)         { Assert(!IsTiny() && !IsSource()); _pch = pch; }
    ULONG           GetCch()                    { Assert(!IsTiny() && !IsSource()); return(_cch); }
    void            SetCch(ULONG cch)           { Assert(!IsTiny() && !IsSource()); _cch = cch; }
    CHtmTagStm *    GetHtmTagStm()              { Assert(!IsTiny() && IsSource()); return(_pHTS); }
    void            SetHtmTagStm(CHtmTagStm *p) { Assert(!IsTiny() && IsSource()); _pHTS = p; }
    ULONG           GetSourceCch()              { Assert(!IsTiny() && IsSource()); return(_cch); }
    void            SetSourceCch(ULONG cch)     { Assert(!IsTiny() && IsSource()); _cch = cch; }
    ULONG           GetLine()                   { Assert(!IsTiny()); return(_ulLine); }
    void            SetLine(ULONG ulLine)       { Assert(!IsTiny()); _ulLine = ulLine; }
    ULONG           GetOffset()                 { Assert(!IsTiny()); return(_ulOffset); }
    void            SetOffset(ULONG ulOffset)   { Assert(!IsTiny()); _ulOffset = ulOffset; }
    CODEPAGE        GetCodepage()               { Assert(!IsTiny()); return(_cp); }
    void            SetCodepage(CODEPAGE cp)    { Assert(!IsTiny()); _cp = cp; }
    ULONG           GetDocSize()                { Assert(!IsTiny()); return(_ulSize); }
    void            SetDocSize(ULONG ulSize)    { Assert(!IsTiny()); _ulSize = ulSize; }

    UINT            GetAttrCount()              { return(_cAttr); }
    void            SetAttrCount(UINT i)        { _cAttr = (WORD)i; }
    CAttr *         GetAttr(UINT i)             { Assert(i < _cAttr); return(&_aAttr[i]); }

    BOOL            ValFromName(const TCHAR *pchName, TCHAR **ppchVal);
    CAttr *         AttrFromName(const TCHAR *pchName);

    LPTSTR          GetXmlNamespace(int * pIdx);

    static UINT     ComputeSize(BOOL fTiny, UINT cAttr) { return(offsetof(CHtmTag, _pch) + ((!fTiny) * (offsetof(CHtmTag, _aAttr) - offsetof(CHtmTag, _pch))) + cAttr * sizeof(CAttr)); }
    UINT            ComputeSize()               { return(ComputeSize(!!(_bFlags & TAGF_TINY), _cAttr)); }

private:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))

    enum
    {
        TAGF_END        = 0x01,     // This is a closing tag (</FOO>)
        TAGF_EMPTY      = 0x02,     // This is an XML noscope tag (<FOO/>)
        TAGF_RESTART    = 0x04,     // True if parsing should restart (ETAG_RAW_CODEPAGE only)
        TAGF_ASCII      = 0x08,     // True if text contains only chars 0x00<ch<0x80 (ETAG_RAW_TEXT only)
        TAGF_NEXTBUF    = 0x10,     // This token should advance the text buffer (CHtmTagStm use only)
        TAGF_TINY       = 0x20,     // This token is only 4 bytes long (CHtmTagStm use only)
        TAGF_SCRIPTED   = 0x40,     // Element created through script (i.e. not through parser)
        TAGF_DEFER      = 0x80,     // True if script is known to be defered and preparser didn't block (</SCRIPT> only)
#ifdef VSTUDIO7
                                    // True if tag is not equal to </SCRIPT> and tag derives from a HTML tag.
#endif //VSTUDIO7
    };

    // Data Members

    BYTE            _bFlags;        // Flags (TGF_*)
    BYTE            _etag;          // Tag code (ETAG_*)
    WORD            _cAttr;         // Count of attributes in _aAttr[]

    union
    {
        union
        {
            TCHAR *     _pch;           // ETAG_RAW_TEXT        - Pointer to text (not NULL terminated)
                                        // ETAG_RAW_COMMENT     - Pointer to comment text (not NULL terminated)
                                        // Unknown tag          - Pointer to tag name (NULL terminated)
            CHtmTagStm *_pHTS;         // ETAG_RAW_SOURCE      - Pointer to CHtmTagStm for retrieving echoed source
        };
        ULONG       _ulLine;        // ETAG_SCRIPT          - Line of closing {>} of <SCRIPT> tag
        CODEPAGE    _cp;            // ETAG_RAW_CODEPAGE    - The codepage to switch to
        ULONG       _ulSize;        // ETAG_RAW_DOCSIZE     - The current document source size
    };

    union
    {
        ULONG       _cch;           // ETAG_RAW_TEXT        - Count of text characters
                                    // ETAG_RAW_COMMENT     - Count of comment characters
                                    // Unknown tag          - Count of tag name characters
        ULONG       _ulOffset;      // ETAG_SCRIPT          - Offset to closing {>} of <SCRIPT> tag
    };

    CAttr           _aAttr[4];      // The attributs (real size is _cAttr)

};

// CHtmParseCtx ---------------------------------------------------------------

class CHtmParseCtx
{
public:

    CHtmParseCtx(CHtmParseCtx *phpxParent)
        { _phpxParent = phpxParent; _phpxEmbed = phpxParent->GetHpxEmbed(); }
    CHtmParseCtx(FLOAT f)
        { };
    virtual ~CHtmParseCtx() {}

    virtual HRESULT Init()
        { return S_OK; }
    virtual HRESULT Prepare()
        { return S_OK; }
    virtual HRESULT Commit()
        { return S_OK; }
    virtual HRESULT Finish()
        { return S_OK; }
    virtual HRESULT Execute()
        { return S_OK; }
        
    virtual HRESULT BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty)
        { RRETURN(_phpxEmbed->BeginElement(ppNodeNew, pel, pNodeCur, fEmpty)); }
    virtual HRESULT EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd)
        { RRETURN(_phpxEmbed->EndElement(ppNodeNew, pNodeCur, pNodeEnd)); }
    virtual HRESULT AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii)
        { return S_OK; }
    virtual HRESULT AddTag(CHtmTag *pht)
        { return S_OK; }
    virtual HRESULT AddSource(CHtmTag *pht);
    
    virtual BOOL QueryTextlike(ELEMENT_TAG etag, CHtmTag *pht)
        { return _phpxParent->QueryTextlike(etag, pht); }
    virtual CElement *GetMergeElement()
        { return NULL; }
    virtual HRESULT InsertLPointer(CTreePos **pptp, CTreeNode *pNodeCur)
        { RRETURN(_phpxParent->InsertLPointer(pptp, pNodeCur)); }
    virtual HRESULT InsertRPointer(CTreePos **pptp, CTreeNode *pNodeCur)
        { RRETURN(_phpxParent->InsertRPointer(pptp, pNodeCur)); }

    virtual CHtmParseCtx *GetHpxRoot()
        { CHtmParseCtx *phpx = this; while (phpx->_phpxParent) phpx = phpx->_phpxParent; return phpx; }
    virtual CHtmParseCtx *GetHpxEmbed()
        { return _phpxEmbed; }

    ELEMENT_TAG *_atagReject;       // reject these tokens from normal processing
    ELEMENT_TAG *_atagAccept;       // allow these tokens for normal processing (tokens taken are !R || A)
    ELEMENT_TAG *_atagTag;          // tags which should be fed via AddTag (subset of R)
    ELEMENT_TAG *_atagAlwaysEnd;    // transform begin tag to end tag (subset of R)
    ELEMENT_TAG *_atagIgnoreEnd;    // throw away End tag, and don't turn into unknown (subset of R)
    BOOL         _fNeedExecute;     // call Execute (from parser-reentrant location) after Finish
    BOOL         _fDropUnknownTags; // give the context unknown tags (via AddUnknownTag)
    BOOL         _fExecuteOnEof;    // even execute me on EOF (no end tag required)
    BOOL         _fIgnoreSubsequent;// ignore the rest of the HTML file after I'm finished
    CHtmParseCtx *_phpxParent;      // immediate parent
    CHtmParseCtx *_phpxEmbed;       // context which requires notification of begin/end elements
};

class CHtmCrlfParseCtx : public CHtmParseCtx
{
    typedef CHtmParseCtx super;

public:

    CHtmCrlfParseCtx(CHtmParseCtx *phpxParent) : CHtmParseCtx(phpxParent) {};
            
    virtual HRESULT AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii);
    virtual HRESULT Finish();

    virtual HRESULT AddNonspaces(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii) = 0;
    virtual HRESULT AddSpaces(CTreeNode *pNode, TCHAR *pch, ULONG cch) = 0;

private:
    BOOL _fLastCr;
};

#define FILL_MASK 0x03
#define FILL_SHIFT   2

#define FILL_NUL  0x00
#define FILL_PUT  0x01
#define FILL_EAT  0x02

class CHtmSpaceParseCtx : public CHtmCrlfParseCtx
{
    typedef CHtmCrlfParseCtx super;
    
public:
    CHtmSpaceParseCtx(CHtmParseCtx *phpxParent) : CHtmCrlfParseCtx(phpxParent) {};
    virtual ~CHtmSpaceParseCtx();

    virtual HRESULT AddNonspaces(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii);
    virtual HRESULT AddSpaces(CTreeNode *pNode, TCHAR *pch, ULONG cch);

    virtual HRESULT AddWord(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii) = 0;
    virtual HRESULT AddSpace(CTreeNode *pNode) = 0;
    
    HRESULT LFill(UINT fillcode);
    HRESULT RFill(UINT fillcode, CTreeNode *pNode);
            
private:
    CTreeNode *_pNodeSpace;
    BOOL _fEatSpace;
    BOOL _fOneLine;
    TCHAR _chLast;
};

#define ETAG_IMPLICIT_BEGIN     ETAG_UNKNOWN

enum PARSESCOPE
{
    SCOPE_EMPTY,        // Elements with no scope like <IMG>
    SCOPE_OVERLAP,      // Overlapping elements like <B>
    SCOPE_NESTED,       // Nesting elements like <P>
    PARSESCOPE_Last_Enum
};

enum PARSETEXTSCOPE
{
    TEXTSCOPE_NEUTRAL,  // Ability to contain text depends on parent (<FORM>)
    TEXTSCOPE_INCLUDE,  // Able to contain text (<BODY>, <TD>, <OPTION>)
    TEXTSCOPE_EXCLUDE,  // Not able to contain text (<TABLE>, <SELECT>)
    PARSETEXTSCOPE_Last_Enum
};

enum PARSETEXTTYPE
{
    TEXTTYPE_NEVER = 0, // Not textlike
    TEXTTYPE_ALWAYS,    // textlike
    TEXTTYPE_QUERY,     // Must query context
    PARSETEXTTYPE_Last_Enum
};

class CHtmlParseClass
{
public:
    // Empty, overlapping, or nested.
    PARSESCOPE      _scope;

    // Textlike tags must be treated as text with respect to including
    // or excluding them from text-sensitive containers.
    PARSETEXTTYPE _texttype;

    // EndContainers hide end tags:
    // An end tag cannot "see" a corresponding begin tag through a root
    // container. For example, in <TD><TABLE></TD></TABLE>, the </TD>
    // cannot see the <TD> because TABLE is an end container.
    // Also a tag cannot "extend" beyond the end of an end container:
    // when the end container ends, so does the contained element.
    ELEMENT_TAG *_atagEndContainers;

    // BeginContainers hide begin tags:
    // A tag cannot "see" prohibited, required, or masking containers
    // outside its begin containers. For example, in <LI><UL><LI>,
    // the second <LI> cannot see the first LI (which would be
    // prohibited) because UL is a start container for LI.
    ELEMENT_TAG *_atagBeginContainers;

    // MaskingContainers supress parsing:
    // If a masking container is on the stack while parsing a begin
    // tag, the begin tag is treated as an unknown tag.
    ELEMENT_TAG *_atagMaskingContainers;

    // ProhibitedContainers specifies end tags implied by begin tags:
    // If an element is parented by a prohibited container, the prohibited
    // container is closed before inserting the element.
    ELEMENT_TAG *_atagProhibitedContainers;

    // Required/DefaultContainer specifies begin tags implied by begin tags:
    // If an element is not parented by one of its required containers,
    // the element specified by _etagDefaultContainer is inserted if
    // possible. (if not possible, the tag is treated as an unknown tag.)
    // Also, if _fQueue is TRUE, instead of implying _etagDefaultContainer
    // immediately, the tag is queued and replayed after _etagDefaultContainer
    // appears.
    ELEMENT_TAG *_atagRequiredContainers;
    ELEMENT_TAG _etagDefaultContainer;
    BOOL _fQueueForRequired;


    // Text include/exclude specifies whether this element can contain
    // text. If TEXTSCOPE_EXCLUDE, it cannot contain text, and it must
    // specify a default subcontainer which can be inserted which can
    // contain text. Unlike the rest of the parsing DTD, these rules
    // apply downward because they are context-sensitive. (The default
    // container for text depends on which parent is TEXTSCOPE_EXCLUDE.)
    PARSETEXTSCOPE _textscope;
    ELEMENT_TAG _etagTextSubcontainer;

    // Alternate matching begin tags:
    // If non-null, an end tag of this type will match any begin tag
    // in the specified set.
    ELEMENT_TAG *_atagMatch;

    // Begin-tag substitute for an unmatched end tag
    // If an end tag is encountered which does have a corresponding
    // begin tag, the end tag is replaced by the specified begin tag.
    // E.g., an unmatched </P> is replaced by a <P>
    ELEMENT_TAG _etagUnmatchedSubstitute;

    // Context creator
    HRESULT (*_pfnHpxCreator)(CHtmParseCtx **pphpx, CElement *pelTop, CHtmParseCtx *phpxParent);

    BOOL _fMerge;

    // Implicit child
    // When an element is created, its required child is immediately created underneath it
    ELEMENT_TAG _etagImplicitChild;

    // Close required child
    // If TRUE, the implicit child is implicitly closed; otherwise, it is left open
    BOOL _fCloseImplicitChild;
};


extern CHtmlParseClass s_hpcUnknown;

class CTagDesc
{
public:
    TCHAR              *_pchTagName;
    CHtmlParseClass    *_pParseClass;
    HRESULT           (*_pfnElementCreator)(CHtmTag *pht, CDoc *pDoc, CElement **ppElementResult);
    DWORD               _dwTagDescFlags;

    inline BOOL         HasFlag(TAGDESC_FLAGS) const;
};

// global array of tagdescs
extern const CTagDesc g_atagdesc[];
extern CPtrBagCi<ELEMENT_TAG> g_bKnownTags;

inline const CTagDesc *
TagDescFromEtag(ELEMENT_TAG tag)
{
    Assert(tag < ETAG_LAST);
    return &g_atagdesc[tag];
}

inline const TCHAR *
NameFromEtag(ELEMENT_TAG tag)
{
    Assert(tag < ETAG_LAST);
    return(g_atagdesc[tag]._pchTagName);
}

inline ELEMENT_TAG
EtagFromName(const TCHAR *pch, int nLen)
{
    return g_bKnownTags.GetCi(pch, nLen);
}

inline BOOL
CTagDesc::HasFlag(TAGDESC_FLAGS flag) const
{
    return !!((_dwTagDescFlags & flag));
}

inline BOOL
IsGenericTag(ELEMENT_TAG etag)
{
    return  ETAG_GENERIC         == etag ||
            ETAG_GENERIC_BUILTIN == etag ||
            ETAG_GENERIC_LITERAL == etag;
}

//+------------------------------------------------------------------------
//
//  Function:   IsEtagInSet
//
//  Synopsis:   Determines if an element is within a set.
//
//-------------------------------------------------------------------------
inline BOOL IsEtagInSet(ELEMENT_TAG etag, ELEMENT_TAG *petagSet)
{
    Assert(petagSet);

    while (*petagSet)
    {
        if (etag == *petagSet)
            return TRUE;
        petagSet++;
    }
    return FALSE;
}

BOOL MergableTags(ELEMENT_TAG etag1, ELEMENT_TAG etag2);

CHtmlParseClass *HpcFromEtag(ELEMENT_TAG tag);
BOOL TagProhibitedContainer(ELEMENT_TAG tag1, ELEMENT_TAG tag2);
BOOL TagEndContainer(ELEMENT_TAG tag1, ELEMENT_TAG tag2);
BOOL TagHasNoEndTag(ELEMENT_TAG tag);

#pragma INCMSG("--- End 'htmtags.hxx'")
#else
#pragma INCMSG("*** Dup 'htmtags.hxx'")
#endif
