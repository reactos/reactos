/*
 *  @doc    INTERNAL
 *
 *  @module LINESRV.HXX -- line services interface
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      11/20/97     cthrash created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I_LINESRV_HXX_
#define I_LINESRV_HXX_
#pragma INCMSG("--- Beg 'linesrv.hxx'")

#ifdef _MAC
#include "ruby.h"
#include "warichu.h"
#endif

MtExtern(CLineServices)
MtExtern(COneRun)
MtExtern(CComplexRun)
MtExtern(CBidiLine)
MtExtern(CAryDirRun_pv)
MtExtern(CLineServices_aryLineFlags_pv)
MtExtern(CLineServices_aryLineCounts_pv)
MtExtern(CDobjBase)
MtExtern(CNobrDobj)
MtExtern(CEmbeddedDobj)
MtExtern(CGlyphDobj)
MtExtern(CILSObjBase)
MtExtern(CEmbeddedILSObj)
MtExtern(CNobrILSObj)
MtExtern(CGlyphILSObj)
MtExtern(CLSLineChunk)

#ifndef X_WCHDEFS_H
#define X_WCHDEFS_H
#include <wchdefs.h>
#endif

#ifndef X_LSDEFS_H_
#define X_LSDEFS_H_
#include <lsdefs.h>
#endif

#ifndef X_LSCHP_H_
#define X_LSCHP_H_
#include <lschp.h>
#endif

#ifndef X_LSFRUN_H_
#define X_LSFRUN_H_
#include <lsfrun.h>
#endif

#ifndef X_FMTRES_H_
#define X_FMTRES_H_
#include <fmtres.h>
#endif

#ifndef X_PLSDNODE_H_
#define X_PLSDNODE_H_
#include <plsdnode.h>
#endif

#ifndef X_PLSSUBL_H_
#define X_PLSSUBL_H_
#include <plssubl.h>
#endif

#ifndef X_LSDSSUBL_H_
#define X_LSDSSUBL_H_
#include <lsdssubl.h>
#endif

#ifndef X_LSKJUST_H_
#define X_LSKJUST_H_
#include <lskjust.h>
#endif

#ifndef X_LSKALIGN_H_
#define X_LSKALIGN_H_
#include <lskalign.h>
#endif

#ifndef X_LSCONTXT_H_
#define X_LSCONTXT_H_
#include <lscontxt.h>
#endif

#ifndef X_LSLINFO_H_
#define X_LSLINFO_H_
#include <lslinfo.h>
#endif

#ifndef X_PLSLINE_H_
#define X_PLSLINE_H_
#include <plsline.h>
#endif

#ifndef X_PLSSUBL_H_
#define X_PLSSUBL_H_
#include <plssubl.h>
#endif

#ifndef X_PDOBJ_H_
#define X_PDOBJ_H_
#include <pdobj.h>
#endif

#ifndef X_PHEIGHTS_H_
#define X_PHEIGHTS_H_
#include <pheights.h>
#endif

#ifndef X_PLSRUN_H_
#define X_PLSRUN_H_
#include <plsrun.h>
#endif

#ifndef X_LSESC_H_
#define X_LSESC_H_
#include <lsesc.h>
#endif

#ifndef X_POBJDIM_H_
#define X_POBJDIM_H_
#include <pobjdim.h>
#endif

#ifndef X_LSPRACT_H_
#define X_LSPRACT_H_
#include <lspract.h>
#endif

#ifndef X_LSBRK_H_
#define X_LSBRK_H_
#include <lsbrk.h>
#endif

#ifndef X_LSDEVRES_H_
#define X_LSDEVRES_H_
#include <lsdevres.h>
#endif

#ifndef X_LSEXPAN_H_
#define X_LSEXPAN_H_
#include <lsexpan.h>
#endif

#ifndef X_LSPAIRAC_H_
#define X_LSPAIRAC_H_
#include <lspairac.h>
#endif

#ifndef X_LSKTAB_H_
#define X_LSKTAB_H_
#include <lsktab.h>
#endif

#ifndef X_LSQLINE_H_
#define X_LSQLINE_H_
#include <lsqline.h>
#endif

#ifndef X_LSCRLINE_H_
#define X_LSCRLINE_H_
#include <lscrline.h>
#endif

#ifndef X_LSQSINFO_H_
#define X_LSQSINFO_H_
#include <lsqsinfo.h>
#endif

#ifndef X_LSCRSUBL_H_
#define X_LSCRSUBL_H_
#include <lscrsubl.h>
#endif

#ifndef X_LSSUBSET_H_
#define X_LSSUBSET_H_
#include <lssubset.h>
#endif

#ifndef X_LSCELL_H_
#define X_LSCELL_H_
#include <lscell.h>
#endif

#ifndef X_LSTABS_H_
#define X_LSTABS_H_
#include <lstabs.h>
#endif

#ifndef X_LSDSPLY_H_
#define X_LSDSPLY_H_
#include <lsdsply.h>
#endif

#ifndef X_DISPI_H_
#define X_DISPI_H_
#include <dispi.h>
#endif

#ifndef X_LSULINFO_H_
#define X_LSULINFO_H_
#include <lsulinfo.h>
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_LSQIN_H_
#define X_LSQIN_H_
#include <lsqin.h>
#endif

#ifndef X_LSQOUT_H_
#define X_LSQOUT_H_
#include <lsqout.h>
#endif

#ifndef X_LSDNSET_H_
#define X_LSDNSET_H_
#include <lsdnset.h>
#endif

#ifndef X_BRKKIND_H_
#define X_BRKKIND_H_
#include <brkkind.h>
#endif

#ifndef X_TOMCONST_H_
#define X_TOMCONST_H_
#include <tomconst.h>
#endif

#ifndef X__LINE_H_
#define X__LINE_H_
#include "_line.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_UNIPART_H
#define X_UNIPART_H
#include <unipart.h>
#endif

#ifndef X_UNIDIR_H
#define X_UNIDIR_H
#include <unidir.h>
#endif

#ifndef X__EDIT_H
#define X__EDIT_H
#include "_text.h"
#endif

#ifndef X_CGLYPH_HXX_
#define X_CGLYPH_HXX_
#include "cglyph.hxx"
#endif

#define LSTRACE(X) TraceTag(( tagLSCallBack, #X ) );
#define LSTRACE2(X,Y,arg) TraceTag(( tagLSCallBack, #X Y, arg ) );
#define LSTRACE3(X,Y,arg0,arg1) TraceTag(( tagLSCallBack, #X Y, arg0, arg1 ) );
#define LSNOTIMPL(X) AssertSz(0, "Unimplemented LineServices callback: pfn"#X)
#define LSADJUSTLSCP(cp) (cp = CPFromLSCP(cp));

extern HRESULT HRFromLSERR( LSERR lserr );
extern HRESULT LSERRFromHR( HRESULT hr );
#if DBG==1
extern const char * LSERRName( LSERR lserr );
#endif

// For widthmodifictatin

enum CSCOPTION
{
    cscoNone                = 0x00,
    cscoAutospacingAlpha    = 0x01,
    cscoWide                = 0x02,
    cscoCompressKana        = 0x04,
    cscoAutospacingDigit    = 0x08,
    cscoVerticalFont        = 0x10,
    cscoAutospacingParen    = 0x20
};

// This number can be tuned for optimization.  It is used to
// limit the maximum length of a run of chars to return to LS
// from fetchrun.
#define MAX_CHARS_FETCHRUN_RETURNS ((long)200)

// This is another tuning number.  It is a hint to LS.  They
// say if it's too large, we waste some memory.  If it's too small,
// we run slower.
#define LS_AVG_CHARS_PER_LINE ((DWORD)100)

typedef enum rubysyntax RUBYSYNTAX;

class CMarginInfo;
class CLSRenderer;
class CDobjBase;
class CLineServices;
class CTreeInfo;
class COneRun;

// CLSLineChunk - a chunk of characters and their width in the current line

class CLSLineChunk
{
public:
    LONG        _cch;
    LONG        _xWidth;

    unsigned int _fRelative : 1;
    unsigned int _fSingleSite : 1;
    unsigned int _fHasBulletOrNum : 1;

    CLSLineChunk * _plcNext;

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CLSLineChunk));
    CLSLineChunk() { memset(this, 0, sizeof(CLSLineChunk)); }
};

typedef enum
{
    FLAG_NONE             = 0x0000,
    FLAG_HAS_ABSOLUTE_ELT = 0x0001,
    FLAG_HAS_ALIGNED      = 0x0002,
    FLAG_HAS_EMBED_OR_WBR = 0x0004,
    FLAG_HAS_NESTED_RO    = 0x0008,
    FLAG_HAS_RUBY         = 0x0010,
    FLAG_HAS_BACKGROUND   = 0x0020,
    FLAG_HAS_A_BR         = 0x0040,
    FLAG_HAS_RELATIVE     = 0x0080,
    FLAG_HAS_NBSP         = 0x0100,
    FLAG_HAS_NOBLAST      = 0x0200,
    FLAG_HAS_CLEARLEFT    = 0x0400,
    FLAG_HAS_CLEARRIGHT   = 0x0800,
    FLAG_HAS_LINEHEIGHT   = 0x1000,
    WHEN_DBG(
    FLAG_HAS_LETTERSPACING= 0x8000,
            )
} LINE_FLAGS;

class CFlagEntry
{
    friend class CLineFlags;

    CFlagEntry(LONG cp, DWORD dwlf) : _cp(cp), _dwlf(dwlf) {}

    DWORD _dwlf;
    LONG  _cp;
};

class CLineFlags
{
public:
    void  InitLineFlags() { _fForced = FALSE; }
    LSERR AddLineFlag(LONG cp, DWORD dwlf);
    LSERR AddLineFlagForce(LONG cp, DWORD dwlf);
    DWORD GetLineFlags(LONG cp);
    void  DeleteAll() { _fForced = FALSE; _aryLineFlags.DeleteAll(); }
    WHEN_DBG(void DumpFlags());

private:
    DECLARE_CDataAry(CFlagEntries, CFlagEntry, Mt(Mem), Mt(CLineServices_aryLineFlags_pv))
    CFlagEntries _aryLineFlags;
    BOOL _fForced;
};

typedef enum
{
    LC_UNDEFINED     = 0x00,
    LC_INLINEDSITES  = 0x01,
    LC_ALIGNEDSITES  = 0x02,
    LC_HIDDEN        = 0x04,
    LC_ABSOLUTESITES = 0x08,
    LC_LINEHEIGHT    = 0x10,
} LC_TYPE;

class CLineCount
{
    friend class CLineCounts;

    CLineCount (LONG cp, LC_TYPE lcType, LONG count) :
            _cp(cp), _lcType(lcType), _count(count) {}

    LONG    _cp;
    LC_TYPE _lcType;
    LONG    _count;
};

class CLineCounts
{
public:
    LSERR AddLineCount(LONG cp, LC_TYPE lcType, LONG count);
    LSERR AddLineValue(LONG cp, LC_TYPE lcType, LONG count) { return AddLineCount(cp, lcType, count); }
    LONG  GetLineCount(LONG cp, LC_TYPE lcType);
    LONG  GetMaxLineValue(LONG cp, LC_TYPE lcType);
    void  DeleteAll() { _aryLineCounts.DeleteAll(); }
    WHEN_DBG(void DumpCounts());

private:
    DECLARE_CDataAry(CLineCountArray, CLineCount, Mt(Mem), Mt(CLineServices_aryLineCounts_pv))
    CLineCountArray _aryLineCounts;
};

class CComplexRun
{
public:
    CComplexRun()
    {
        _ThaiWord.cp = 0;
        _ThaiWord.cch = 0;
        _pGlyphBuffer = NULL;
    }
    ~CComplexRun()
    {
        MemFree(_pGlyphBuffer);
    }
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CComplexRun));
    void ComputeAnalysis(const CFlowLayout * pFlowLayout,
                         BOOL fRTL,
                         BOOL fForceGlyphing,
                         WCHAR chNext,
                         WCHAR chPassword,
                         COneRun * por,
                         COneRun * porTail,
                         DWORD uLangDigits);
    BOOL IsNumericSeparatorRun(COneRun * por, COneRun * porTail);
    void ThaiTypeBrkcls(CMarkup * pMarkup, LONG cp, BRKCLS* pbrkclsAfter, BRKCLS* pbrkclsBefore);
    SCRIPT_ANALYSIS * GetAnalysis() { return &_Analysis; }
    WORD GetScript() { return _Analysis.eScript; }

    inline void NukeAnalysis() { _Analysis.eScript = 0; }
    inline void CopyPtr(WORD *pGlyphBuffer)
    {
        if (_pGlyphBuffer)
            delete _pGlyphBuffer;
        _pGlyphBuffer = pGlyphBuffer;
    }

private:
    SCRIPT_ANALYSIS _Analysis;
    struct
    {
        DWORD cp    : 26;
        DWORD cch   : 6;
    } _ThaiWord;
    WORD *_pGlyphBuffer;
};

//+------------------------------------------------------------------------
//
//  Class:      CBidiLine
//
//  Synopsis:   This is an array of DIR_RUNs that is used to identify the
//              direction of text within the line.
//
//-------------------------------------------------------------------------

struct dir_run
{
    LONG    cp;     // CP at that starts of this run.
    LONG    iLevel; // Unicode level of the run. We only actually need 5 bits of this.
};

typedef dir_run DIR_RUN;

DECLARE_CStackDataAry(CAryDirRun, DIR_RUN, 8, Mt(Mem), Mt(CAryDirRun_pv));

class CBidiLine
{
public:
    // Creation and destruction
    CBidiLine(const CTreeInfo & treeinfo, LONG cp, BOOL fRTLPara, const CLine * pli);
    CBidiLine(BOOL fRTLPara, LONG cchText, const WCHAR * pchText);
    ~CBidiLine()
    {
        _aryDirRun.DeleteAll();
    };
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBidiLine));

    // Methods
    LONG GetRunCchRemaining(LONG cp, LONG cchMax);
    void LogicalToVisual(BOOL fRTLBuffer, LONG cchText,
                         const WCHAR * pchLogical, WCHAR * pchVisual);

    // Property query
    WORD GetLevel(LONG cp)
    {
        return GetDirRun(cp).iLevel;
    }

#if DBG==1
    // Debug helpers
    BOOL IsEqual(const CTreeInfo & treeinfo, LONG cp, BOOL fRTLPara, const CLine * pli) const;
#endif

private:
    // Helper methods
    DIRCLS GetInitialDirClass(BOOL fRTLLine);
    const DIR_RUN & GetDirRun(LONG cp);
    LONG FindRun(LONG cp);

    // Unicode bidi algorithm
    void EvaluateLayoutToCp(LONG cp, LONG cpLine = -1);
    void EvaluateLayout(const WCHAR * pchText, LONG cchText, DIRCLS dcTrail,
                        DIRCLS dcTrailForNumeric, LONG cp);
    void AdjustTrailingWhitespace(LONG cpEOL);
    LONG GetTextForLayout(WCHAR * pch, LONG cch, LONG cp, CTreePos ** pptp,
                          LONG * pcchRemaining);

    // Translation tables
    static const DIRCLS s_adcEmbedding[5];
    static const DIRCLS s_adcNumeric[3];
    static const DIRCLS s_adcNeutral[2][6][6];
    static const BYTE s_abLevelOffset[2][6];

    // Properties

    // Pointers to the document.
    CFlowLayout * _pFlowLayout;
    CMarkup * _pMarkup;
    CTxtPtr _txtptr;

    // Location of the line in the doc.
    LONG _cpFirst;              // First CP in the line.
    LONG _cpLim;                // Last CP in the layout.

    // Current location in the backing store
    LONG _cp;                   // Next CP to evaluate (limit of evaluated CPs).
    CTreePos * _ptp;            // Next TP to evaluate.
    LONG _cchRemaining;         // Characters remaining in the current TP.

    // Current state of the bidi algorithm
    DIRCLS _dcPrev;             // Last resolved character class.
    DIRCLS _dcPrevStrong;       // Last strong character class.
    LONG _iLevel;               // Current bidi level.
    DIRCLS _dcEmbed;            // Type of current embedding.

    // Bidi stack
    DIRCLS _aEmbed[16];         // Stack of pushed embeddings.
    LONG _iEmbed;               // Depth of embedding stack.
    LONG _iOverflow;            // Overflow from embedding stack.

    DWORD _fRTLPara:1;          // Direction of the paragraph
    DWORD _fVisualLine:1;       // Current line is visual ordering

    // Array of DIR_RUNs
    CAryDirRun _aryDirRun;

    // Most recently fetched run.
    LONG _iRun;

#if DBG==1
    // Flag saying what kind of text we're laying out (from the backing store
    // or from a string.
    BOOL _fStringLayout;
#endif
};

class CTreeInfo
{
public:
    CTreeInfo(CMarkup *pMarkup): _tpFrontier(pMarkup) { _fInited = FALSE; };
    HRESULT InitializeTreeInfo(CFlowLayout *pFlowLayout, BOOL fIsEditable, LONG cp, CTreePos *ptp);
    void SetupCFPF(BOOL fIniting, CTreeNode *pNode);
    BOOL AdvanceTreePos();
    BOOL AdvanceTxtPtr();
    void AdvanceFrontier(COneRun *por);

    CTreePos          *_ptpFrontier;
    long               _cchRemainingInTreePos;

    BOOL               _fHasNestedElement;
    BOOL               _fHasNestedLayout;
    BOOL               _fHasNestedRunOwner;
    const CParaFormat *_pPF;
    long               _iPF;
    BOOL               _fInnerPF;
    const CCharFormat *_pCF;
    BOOL               _fInnerCF;
    //long               _iCF;
    const CFancyFormat*_pFF;
    long               _iFF;

    long               _lscpFrontier;
    long               _cpFrontier;
    long               _chSynthsBefore;

    //
    // Following are the text related state variables
    //
    CTxtPtr            _tpFrontier;
    long               _cchValid;
    const TCHAR       *_pchFrontier;

    CMarkup            *_pMarkup;
    CFlowLayout        *_pFlowLayout;
    LONG                _cpLayoutFirst;
    LONG                _cpLayoutLast;
    CTreePos           *_ptpLayoutLast;
    BOOL                _fIsEditable;

    BOOL _fInited;
};

class COneRunFreeList
{
public:
    COneRunFreeList()  {}
    ~COneRunFreeList() { Deinit(); }
    void Init();
    void Deinit();

    COneRun *GetFreeOneRun(COneRun *porClone);
    void SpliceIn(COneRun *pFirst);

    COneRun *_pHead;
    WHEN_DBG(static LONG s_NextSerialNumber);
};

class COneRunCurrList
{
public:
    COneRunCurrList()  { Init();   }
    ~COneRunCurrList() { Deinit(); }

    void Init();
    void Deinit();
    void SpliceOut(COneRun *pFirst, COneRun *plast);
#if DBG==1
    void SpliceInAfterMe(CLineServices *pLS, COneRun *pAfterMe, COneRun *pFirst);
#else
    void SpliceInAfterMe(COneRun *pAfterMe, COneRun *pFirst);
#endif

    void SpliceInBeforeMe(COneRun *pBeforeMe, COneRun *pFirst);

    WHEN_DBG(void VerifyStuff(CLineServices *pLS));

    COneRun *_pHead;
    COneRun *_pTail;
};

// Abastract base class for Installed LineServices Object types.  For each type of
// installed object that we're gonna give to LS, we subclass this guy.
// In the LS context, objects of this type represent both islobj's and lnobj's.
// We consider the lnobj construct to be useless, so we use the same object for
// ilsobj's and lnobj's.  (An lnobj is created for each line, 1 per type of installed
// object on that line -- we have nothing to add on this level.)
// We play C++ games to make the first parameter in these function calls the "this" object
// so that we can have non-static methods as callbacks.  This is done with the WINAPI thing.
// We don't use virtuals here because it wouldn't do any good.  The callback function list is
// basically an oversized v-table.  C sucks.
// Remember that there will only be one instance of these classes per LS context.  These objects
// represent the type of installed object, not the piece of the document which is the installed
// object.  That is represented by a dobj or a plsrun/conerun.
class CILSObjBase
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))

public:

    CILSObjBase(CLineServices* pols, PCLSCBK plscbk);
    virtual ~CILSObjBase();

    //
    // Line-services callbacks shared by all ILSobj's.
    //

    LSERR WINAPI DestroyILSObj();  // this = pilsobj
    LSERR WINAPI CreateLNObj(PLNOBJ*);  // this= pcilsobj
    LSERR WINAPI DestroyLNObj();  // this= plnobj
    LSERR WINAPI SetDoc(PCLSDOCINF);  // this = pilsobj

    // These should be defined in subclasses.
    //LSERR WINAPI Fmt(PCFMTIN, FMTRES*);  // this = plnobj
    //LSERR WINAPI FmtResume(const BREAKREC*, DWORD, PCFMTIN, FMTRES*);  // this = plnobj

    //
    // Member data
    //

    CLineServices * _pLS;


protected:
    //
    // typedefs
    //
    typedef COneRun* PLSRUN;
    typedef CILSObjBase* PILSOBJ;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

};  //CILSObjBase


// This is the installed LS object for tables, images, WCH_Embedding's, buttons, etc.
// Basically this represents any element which has it's own layout.  (Anything that was a site
// in the old terminology)
class CEmbeddedILSObj : public CILSObjBase
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CEmbeddedILSObj))

public:
    CEmbeddedILSObj(CLineServices* pols, PCLSCBK plscbk);
    // More LS callbacks.

    LSERR WINAPI Fmt(PCFMTIN, FMTRES*);  // this = plnobj
    LSERR WINAPI FmtResume(const BREAKREC*, DWORD, PCFMTIN, FMTRES*);  // this = plnobj

private:
    typedef CILSObjBase* PILSOBJ;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

};


// This is an installed LS object for NOBR tags.
class CNobrILSObj : public CILSObjBase
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CNobrILSObj))

public:
    CNobrILSObj(CLineServices* pols, PCLSCBK plscbk);
    // More LS callbacks.

    LSERR WINAPI Fmt(PCFMTIN, FMTRES*);  // this = plnobj
    LSERR WINAPI FmtResume(const BREAKREC*, DWORD, PCFMTIN, FMTRES*);  // this = plnobj

private:
    typedef enum {NBREAKCHARS = 1};
    static const LSESC s_lsescEndNOBR[NBREAKCHARS];

    typedef CILSObjBase* PILSOBJ;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;
};


// This is an installed LS object for GLYPHS.
class CGlyphILSObj : public CILSObjBase
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CGlyphILSObj))

public:
    CGlyphILSObj(CLineServices* pols, PCLSCBK plscbk);
    // More LS callbacks.

    LSERR WINAPI Fmt(PCFMTIN, FMTRES*);  // this = plnobj
    LSERR WINAPI FmtResume(const BREAKREC*, DWORD, PCFMTIN, FMTRES*);  // this = plnobj

private:
    typedef enum {NBREAKCHARS = 1};

    typedef CILSObjBase* PILSOBJ;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;
};

#if defined(UNIX) || defined(_MAC)
struct RUBYINIT;
struct TATENAKAYOKOINIT;
struct HIHINIT;
struct WARICHUINIT;
struct REVERSEINIT;
#endif

typedef struct tagRubyInfo
{
    LONG cp;
    LONG yHeightRubyBase;
    LONG yDescentRubyBase;
    LONG yDescentRubyText;
} RubyInfo;

//+------------------------------------------------------------------------
//
//  Class:      CLineServices
//
//  Synopsis:   Interface and callback implementations for LineServices DLL
//
//-------------------------------------------------------------------------

class CLineServices
{
    friend HRESULT InitLineServices(CMarkup *pMarkup, BOOL fStartUpLSDLL, CLineServices ** pLS);
    friend HRESULT DeinitLineServices( CLineServices * pLS );
    friend HRESULT StartUpLSDLL(CLineServices *pLS, CMarkup *pMarkup);
    friend class CLSCache;
    friend class CRecalcLinePtr;
    friend class CMeasurer;
    friend class CLSMeasurer;
    friend class CLSRenderer;
    friend class CLine;
    friend class CISLObjBase;
    friend class CNobrILSObj;
    friend class CEmbeddedILSObj;
    friend class CDobjBase;
    friend class CNobrDobj;
    friend class CEmbeddedDobj;
    friend class CGlyphDobj;
    friend class CGlyphILSObj;
    friend class COneRun;
#ifdef QUILL
    friend class CQuillGlue;
#endif  // QUILL

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLineServices));

    ~CLineServices();

    CLineServices(CMarkup *pMarkup) : _treeInfo(pMarkup), _aryRubyInfo(0)
    {
        WHEN_DBG(_nSerial= ++s_nSerialMax);
    }

    CMarkup *GetMarkup() { return _treeInfo._tpFrontier._pMarkup; }

private:

    //
    // typedefs
    //

    typedef COneRun* PLSRUN;
    typedef CILSObjBase* PILSOBJ;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

public:

    //
    // Linebreaking
    //

    enum BRKCLS // Breaking classes
    {
        brkclsNil           = -1,
        brkclsOpen          = 0,
        brkclsClose         = 1,
        brkclsNoStartIdeo   = 2,
        brkclsExclaInterr   = 3,
        brkclsInseparable   = 4,
        brkclsPrefix        = 5,
        brkclsPostfix       = 6,
        brkclsIdeographic   = 7,
        brkclsNumeral       = 8,
        brkclsSpaceN        = 9,
        brkclsAlpha         = 10,
        brkclsGlueA         = 11,
        brkclsSlash         = 12,
        brkclsQuote         = 13,
        brkclsNumSeparator  = 14,
        brkclsHangul        = 15,
        brkclsThaiFirst     = 16,
        brkclsThaiLast      = 17,
        brkclsThaiMiddle    = 18,
        brkclsCombining     = 19,
        brkclsAsciiSpace    = 20,
        brkclsLim
    };

    enum BRKOPT // Option flags in breaking tables
    {
        fBrkFollowInline    = 0x20,
        fBrkPrecedeInline   = 0x10,
        fBrkUnit            = 0x08,
        fBrkStrict          = 0x04,
        fBrkNumeric         = 0x02,
        fCscWide            = 0x01,
        fBrkNone            = 0x00
    };

    typedef struct tagPACKEDBRKINFO
    {
        BRKCLS brkcls:8;
        BRKCLS brkclsAlt:8;
        BRKOPT brkopt:8;
        BRKCLS brkclsLow:8;
    } PACKEDBRKINFO;

    #define BRKCLS_QUICKLOOKUPCUTOFF  161

    static const BRKCLS s_rgBrkclsQuick[BRKCLS_QUICKLOOKUPCUTOFF];
    static const PACKEDBRKINFO s_rgBrkInfo[CHAR_CLASS_MAX];
    static const BRKCOND s_rgbrkcondBeforeChar[brkclsLim];
    static const BRKCOND s_rgbrkcondAfterChar[brkclsLim];

private:

    // This structure is identical to LSCBK, except that POLS has been replaced
    // with CLineServices::*.  This allows us to call member functions of our
    // CLineServices object.

    struct lscbk
    {
        void* (WINAPI CLineServices::* pfnNewPtr)(DWORD);
        void  (WINAPI CLineServices::* pfnDisposePtr)(void*);
        void* (WINAPI CLineServices::* pfnReallocPtr)(void*, DWORD);
        LSERR (WINAPI CLineServices::* pfnFetchRun)(LSCP, LPCWSTR*, DWORD*, BOOL*, PLSCHP, PLSRUN*);
        LSERR (WINAPI CLineServices::* pfnGetAutoNumberInfo)(LSKALIGN*, PLSCHP, PLSRUN*, WCHAR*, PLSCHP, PLSRUN*, BOOL*, long*, long*);
        LSERR (WINAPI CLineServices::* pfnGetNumericSeparators)(PLSRUN, WCHAR*,WCHAR*);
        LSERR (WINAPI CLineServices::* pfnCheckForDigit)(PLSRUN, WCHAR, BOOL*);
        LSERR (WINAPI CLineServices::* pfnFetchPap)(LSCP, PLSPAP);
        LSERR (WINAPI CLineServices::* pfnFetchTabs)(LSCP, PLSTABS, BOOL*, long*, WCHAR*);
        LSERR (WINAPI CLineServices::* pfnGetBreakThroughTab)(long, long, long*);
        LSERR (WINAPI CLineServices::* pfnFGetLastLineJustification)(LSKJUST, LSKALIGN, ENDRES, BOOL*, LSKALIGN*);
        LSERR (WINAPI CLineServices::* pfnCheckParaBoundaries)(LSCP, LSCP, BOOL*);
        LSERR (WINAPI CLineServices::* pfnGetRunCharWidths)(PLSRUN, LSDEVICE, LPCWSTR, DWORD, long, LSTFLOW, int*,long*,long*);
        LSERR (WINAPI CLineServices::* pfnCheckRunKernability)(PLSRUN,PLSRUN, BOOL*);
        LSERR (WINAPI CLineServices::* pfnGetRunCharKerning)(PLSRUN, LSDEVICE, LPCWSTR, DWORD, LSTFLOW, int*);
        LSERR (WINAPI CLineServices::* pfnGetRunTextMetrics)(PLSRUN, LSDEVICE, LSTFLOW, PLSTXM);
        LSERR (WINAPI CLineServices::* pfnGetRunUnderlineInfo)(PLSRUN, PCHEIGHTS, LSTFLOW, PLSULINFO);
        LSERR (WINAPI CLineServices::* pfnGetRunStrikethroughInfo)(PLSRUN, PCHEIGHTS, LSTFLOW, PLSSTINFO);
        LSERR (WINAPI CLineServices::* pfnGetBorderInfo)(PLSRUN, LSTFLOW, long*, long*);
        LSERR (WINAPI CLineServices::* pfnReleaseRun)(PLSRUN);
        LSERR (WINAPI CLineServices::* pfnHyphenate)(PCLSHYPH, LSCP, LSCP, PLSHYPH);
        LSERR (WINAPI CLineServices::* pfnGetHyphenInfo)(PLSRUN, DWORD*, WCHAR*);
        LSERR (WINAPI CLineServices::* pfnDrawUnderline)(PLSRUN, UINT, const POINT*, DWORD, DWORD, LSTFLOW, UINT, const RECT*);
        LSERR (WINAPI CLineServices::* pfnDrawStrikethrough)(PLSRUN, UINT, const POINT*, DWORD, DWORD, LSTFLOW, UINT, const RECT*);
        LSERR (WINAPI CLineServices::* pfnDrawBorder)(PLSRUN, const POINT*, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, long, LSTFLOW, UINT, const RECT*);
        LSERR (WINAPI CLineServices::* pfnDrawUnderlineAsText)(PLSRUN, const POINT*, long, LSTFLOW, UINT, const RECT*);
        LSERR (WINAPI CLineServices::* pfnFInterruptUnderline)(PLSRUN, LSCP, PLSRUN, LSCP,BOOL*);
        LSERR (WINAPI CLineServices::* pfnFInterruptShade)(PLSRUN, PLSRUN, BOOL*);
        LSERR (WINAPI CLineServices::* pfnFInterruptBorder)(PLSRUN, PLSRUN, BOOL*);
        LSERR (WINAPI CLineServices::* pfnShadeRectangle)(PLSRUN, const POINT*, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, long, LSTFLOW, UINT, const RECT*);
        LSERR (WINAPI CLineServices::* pfnDrawTextRun)(PLSRUN, BOOL, BOOL, const POINT*, LPCWSTR, const int*, DWORD, LSTFLOW, UINT, const POINT*, PCHEIGHTS, long, long, const RECT*);
        LSERR (WINAPI CLineServices::* pfnDrawSplatLine)(enum lsksplat, LSCP, const POINT*, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, LSTFLOW, UINT, const RECT*);
        LSERR (WINAPI CLineServices::* pfnFInterruptShaping)(LSTFLOW, PLSRUN, PLSRUN, BOOL*);
        LSERR (WINAPI CLineServices::* pfnGetGlyphs)(PLSRUN, LPCWSTR, DWORD, LSTFLOW, PGMAP, PGINDEX*, PGPROP*, DWORD*);
        LSERR (WINAPI CLineServices::* pfnGetGlyphPositions)(PLSRUN, LSDEVICE, LPWSTR, PCGMAP, DWORD, PCGINDEX, PCGPROP, DWORD, LSTFLOW, int*, PGOFFSET);
        LSERR (WINAPI CLineServices::* pfnResetRunContents)(PLSRUN, LSCP, LSDCP, LSCP, LSDCP);
        LSERR (WINAPI CLineServices::* pfnDrawGlyphs)(PLSRUN, BOOL, BOOL, PCGINDEX, const int*, const int*, PGOFFSET, PGPROP, PCEXPTYPE, DWORD, LSTFLOW, UINT, const POINT*, PCHEIGHTS, long, long, const RECT*);
        LSERR (WINAPI CLineServices::* pfnGetGlyphExpansionInfo)(PLSRUN, LSDEVICE, LPCWSTR, PCGMAP, DWORD, PCGINDEX, PCGPROP, DWORD, LSTFLOW, BOOL, PEXPTYPE, LSEXPINFO*);
        LSERR (WINAPI CLineServices::* pfnGetGlyphExpansionInkInfo)(PLSRUN, LSDEVICE, GINDEX, GPROP, LSTFLOW, DWORD, long*);
        LSERR (WINAPI CLineServices::* pfnGetEms)(PLSRUN, LSTFLOW, PLSEMS);
        LSERR (WINAPI CLineServices::* pfnPunctStartLine)(PLSRUN, MWCLS, WCHAR, LSACT*);
        LSERR (WINAPI CLineServices::* pfnModWidthOnRun)(PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
        LSERR (WINAPI CLineServices::* pfnModWidthSpace)(PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
        LSERR (WINAPI CLineServices::* pfnCompOnRun)(PLSRUN, WCHAR, PLSRUN, WCHAR, LSPRACT*);
        LSERR (WINAPI CLineServices::* pfnCompWidthSpace)(PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR, LSPRACT*);
        LSERR (WINAPI CLineServices::* pfnExpOnRun)(PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
        LSERR (WINAPI CLineServices::* pfnExpWidthSpace)(PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
        LSERR (WINAPI CLineServices::* pfnGetModWidthClasses)(PLSRUN, const WCHAR*, DWORD, MWCLS*);
        LSERR (WINAPI CLineServices::* pfnGetBreakingClasses)(PLSRUN, LSCP, WCHAR, BRKCLS*, BRKCLS*);
        LSERR (WINAPI CLineServices::* pfnFTruncateBefore)(PLSRUN, LSCP, WCHAR, long, PLSRUN, LSCP, WCHAR, long, long, BOOL*);
        LSERR (WINAPI CLineServices::* pfnCanBreakBeforeChar)(BRKCLS, BRKCOND*);
        LSERR (WINAPI CLineServices::* pfnCanBreakAfterChar)(BRKCLS, BRKCOND*);
        LSERR (WINAPI CLineServices::* pfnFHangingPunct)(PLSRUN, MWCLS, WCHAR, BOOL*);
        LSERR (WINAPI CLineServices::* pfnGetSnapGrid)(WCHAR*, PLSRUN*, LSCP*, DWORD, BOOL*, DWORD*);
        LSERR (WINAPI CLineServices::* pfnDrawEffects)(PLSRUN, UINT, const POINT*, LPCWSTR, const int*, const int*, DWORD, LSTFLOW, UINT, PCHEIGHTS, long, long, const RECT*);
        LSERR (WINAPI CLineServices::* pfnFCancelHangingPunct)(LSCP, LSCP, WCHAR, MWCLS, BOOL*);
        LSERR (WINAPI CLineServices::* pfnModifyCompAtLastChar)(LSCP, LSCP, WCHAR, MWCLS, long, long, long*);
        LSERR (WINAPI CLineServices::* pfnEnumText)(PLSRUN, LSCP, LSDCP, LPCWSTR, DWORD, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, BOOL, long*);
        LSERR (WINAPI CLineServices::* pfnEnumTab)(PLSRUN, LSCP, WCHAR *, WCHAR, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long);
        LSERR (WINAPI CLineServices::* pfnEnumPen)(BOOL, LSTFLOW, BOOL, BOOL, const POINT*, long, long);
        LSERR (WINAPI CLineServices::* pfnGetObjectHandlerInfo)(DWORD, void*);
        void  (WINAPI *pfnAssertFailed)(char*, char*, int);

#if defined(UNIX) || defined(_MAC)
#if defined(SPARC) || defined(_MAC)
    // This function is used to removed those 8 bytes method ptrs.
    // It needs to be revised whenever LSCBK is updated.
        void fill(::LSCBK* plscbk)
        {
            DWORD *pdw  = (DWORD*)plscbk;
            DWORD *psrc = (DWORD*)&s_lscbk;
            int i;

            for(i=0; i< 62; i++) // 62 is the # of members of LSCBK
            {
                if (i != 61 )
                {
                    psrc++; // The last one is 4 bytes only.
#ifdef _MAC
                    psrc++;  // extra 4 bytes on Mac
#endif
                }
                *pdw++ = *psrc++;
            }
        }
#elif defined(ux10)
    // This function is used to removed those 8 bytes method ptrs.
    // It needs to be revised whenever LSCBK is updated.
        void fill(::LSCBK* plscbk)
        {
            DWORD *pdw  = (DWORD*)plscbk;
            DWORD *psrc = (DWORD*)&s_lscbk;
            int i;

            for(i=0; i< 62; i++) // 62 is the # of members of LSCBK
            {
                if (i != 61 ) psrc++; // The last one is 4 bytes only.
                *pdw++ = *psrc++;
            }
        }
#
#else
#error "You need to implement differently here"
#endif // arch
#endif // UNIX
    };

    typedef struct lscbk LSCBK; // our implementation of LSCBK

    enum LSOBJID
    {
        LSOBJID_EMBEDDED = 0,
        LSOBJID_NOBR = 1,
        LSOBJID_GLYPH = 2,
        LSOBJID_RUBY,
        LSOBJID_TATENAKAYOKO,
        LSOBJID_HIH,
        LSOBJID_WARICHU,
        LSOBJID_REVERSE,
        LSOBJID_COUNT,
        LSOBJID_TEXT = idObjTextChp
    };

    struct lsimethods
    {
        LSERR (WINAPI CLineServices::* pfnCreateILSObj)(PLSC,  PCLSCBK, DWORD, PILSOBJ*);
        LSERR (WINAPI CILSObjBase::* pfnDestroyILSObj)();
        LSERR (WINAPI CILSObjBase::* pfnSetDoc)(PCLSDOCINF);
        LSERR (WINAPI CILSObjBase::* pfnCreateLNObj)(PLNOBJ*);
        LSERR (WINAPI CILSObjBase::* pfnDestroyLNObj)();
        LSERR (WINAPI CILSObjBase::* pfnFmt)(PCFMTIN, FMTRES*);
        LSERR (WINAPI CILSObjBase::* pfnFmtResume)(BREAKREC*, DWORD, PCFMTIN, FMTRES*);
        LSERR (WINAPI* pfnGetModWidthPrecedingChar)(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
        LSERR (WINAPI* pfnGetModWidthFollowingChar)(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
        LSERR (WINAPI* pfnTruncateChunk)(PCLOCCHNK, PPOSICHNK);
        LSERR (WINAPI* pfnFindPrevBreakChunk)(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
        LSERR (WINAPI* pfnFindNextBreakChunk)(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
        LSERR (WINAPI* pfnForceBreakChunk)(PCLOCCHNK, PCPOSICHNK, PBRKOUT);
        LSERR (WINAPI* pfnSetBreak)(PDOBJ, BRKKIND, DWORD, BREAKREC*, DWORD*);
        LSERR (WINAPI* pfnGetSpecialEffectsInside)(PDOBJ, UINT*);
        LSERR (WINAPI* pfnFExpandWithPrecedingChar)(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
        LSERR (WINAPI* pfnFExpandWithFollowingChar)(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
        LSERR (WINAPI* pfnCalcPresentation)(PDOBJ, long, LSKJUST, BOOL);
        LSERR (WINAPI* pfnQueryPointPcp)(PDOBJ, PCPOINTUV, PCLSQIN, PLSQOUT);
        LSERR (WINAPI* pfnQueryCpPpoint)(PDOBJ, LSDCP, PCLSQIN, PLSQOUT);
        LSERR (WINAPI* pfnEnum)(PDOBJ, PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long);
        LSERR (WINAPI* pfnDisplay)(PDOBJ, PCDISPIN);
        LSERR (WINAPI* pfnDestroyDObj)(PDOBJ);
    };

    typedef struct lsimethods LSIMETHODS; // our implementation of LSIMETHOD

    struct rubyinit
    {
        DWORD dwVersion;
        RUBYSYNTAX rubysyntax;
        WCHAR wchEscRuby;
        WCHAR wchEscMain;
        WCHAR wchUnused1;
        WCHAR wchUnused2;
        LSERR (WINAPI CLineServices::* pfnFetchRubyPosition)(LSCP, LSTFLOW, DWORD, const PLSRUN *, PCHEIGHTS, PCHEIGHTS, DWORD, const PLSRUN *, PCHEIGHTS, PCHEIGHTS, PHEIGHTS, PHEIGHTS, long *, long *, long *, enum rubycharjust *, BOOL *);
        LSERR (WINAPI CLineServices::* pfnFetchRubyWidthAdjust)(LSCP, PLSRUN, WCHAR, MWCLS, PLSRUN, enum rubycharloc, long, long *, long *);
        LSERR (WINAPI CLineServices::* pfnRubyEnum)(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, const POINT *, PCHEIGHTS, long, const POINT *, PCHEIGHTS, long, PLSSUBL, PLSSUBL);
    };

    typedef struct rubyinit RUBYINIT; // our implementation of RUBYINIT

    struct tatenakayokoinit
    {
        DWORD dwVersion;
        WCHAR wchEndTatenakayoko;
        WCHAR wchUnused1;
        WCHAR wchUnused2;
        WCHAR wchUnused3;
        LSERR (WINAPI CLineServices::* pfnGetTatenakayokoLinePosition)(LSCP, LSTFLOW, PLSRUN, long, PHEIGHTS, PHEIGHTS, long *);
        LSERR (WINAPI CLineServices::* pfnTatenakayokoEnum)(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, LSTFLOW, PLSSUBL);

    };

    typedef struct tatenakayokoinit TATENAKAYOKOINIT;

    struct hihinit
    {
        DWORD dwVersion;
        WCHAR wchEndHih;
        WCHAR wchUnused1;
        WCHAR wchUnused2;
        WCHAR wchUnused3;
        LSERR (WINAPI CLineServices::* pfnHihEnum)(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, PLSSUBL);
    };

    typedef struct hihinit HIHINIT;

    struct warichuinit
    {
        DWORD dwVersion;
        WCHAR wchEndFirstBracket;
        WCHAR wchEndText;
        WCHAR wchEndWarichu;
        WCHAR wchUnused1;
        LSERR (WINAPI CLineServices::* pfnGetWarichuInfo)(LSCP, LSTFLOW, PCOBJDIM, PCOBJDIM, PHEIGHTS, PHEIGHTS, long *);
        LSERR (WINAPI CLineServices::* pfnFetchWarichuWidthAdjust)(LSCP, enum warichucharloc, PLSRUN, WCHAR, MWCLS, PLSRUN, long *, long *);
        LSERR (WINAPI CLineServices::* pfnWarichuEnum)(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, const POINT *, PCHEIGHTS,long, const POINT *, PCHEIGHTS,long,const POINT *, PCHEIGHTS, long, const POINT *, PCHEIGHTS, long, PLSSUBL, PLSSUBL, PLSSUBL, PLSSUBL);
        BOOL  fContiguousFetch;
    };

    typedef struct warichuinit WARICHUINIT;

    struct reverseinit
    {
        DWORD dwVersion;
        WCHAR wchEndReverse;
        WCHAR wchUnused1;
        WCHAR wchUnused2;
        WCHAR wchUnused3;
        LSERR (WINAPI CLineServices::* pfnReverseEnum)(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, LSTFLOW, PLSSUBL);
    };

    typedef struct reverseinit REVERSEINIT;

    // Line Services requires the addition of synthetic characters to the text
    // stream in order to handle installed objects such as Ruby or Reverse
    // objects. We store these characters in a CArySynth array that records the
    // the location and type of the synthetic character. We also keep an array
    // of data mapping each synthetic character type to an actual character
    // value and object type. Usually the object type is just idObjTextChp,
    // but for the start of objects (like for SYNTHTYPE_REVERSE) it will have
    // a more interesting value.
    // NB (mikejoch) keep SYNTHTYPE_REVERSE and SYNTHTYPE_ENDREVERSE in order
    // and differing by only the least significant bit. This yields a pleasant
    // optimization in GetDirLevel().

    enum SYNTHTYPE
    {
        SYNTHTYPE_NONE,                     // Not a synthetic character
        SYNTHTYPE_SECTIONBREAK,             // End of section mark
        SYNTHTYPE_REVERSE,                  // Begin reversal from DIR prop
        SYNTHTYPE_ENDREVERSE,               // End reversal from DIR prop
        SYNTHTYPE_NOBR,                     // Beginning of a NOBR block
        SYNTHTYPE_ENDNOBR,                  // End of a NOBR block
        SYNTHTYPE_ENDPARA1,                 // Block break for PRE
        SYNTHTYPE_ALTENDPARA,               // Block break for non-PRE
        SYNTHTYPE_RUBYMAIN,                 // Begin ruby main text
        SYNTHTYPE_ENDRUBYMAIN,              // End ruby main text
        SYNTHTYPE_ENDRUBYTEXT,              // End ruby pronunciation text
        SYNTHTYPE_LINEBREAK,                // The line break synth char
        SYNTHTYPE_GLYPH,                    // For rendering of editing glyphs
        SYNTHTYPE_COUNT,                    // Last entry in the enum
    };

    #define SYNTHTYPE_DIRECTION_FIRST SYNTHTYPE_REVERSE
    #define SYNTHTYPE_DIRECTION_LAST SYNTHTYPE_ENDREVERSE

    struct synthdata
    {
        WCHAR       wch;            // Character represented by SYNTHTYPE
        WORD        idObj;          // LS object represented by SYNTHTYPE
        SYNTHTYPE   typeEndObj;     // The SYNTHTYPE that ends this idObj
        DWORD       fObjStart : 1;  // Is the idObj started by this SYNTHTYPE
        DWORD       fObjEnd : 1;    // Is the idObj ended by this SYNTHTYPE
        DWORD       fHidden : 1;    // Is the character hidden from LS
        DWORD       fLSCPStop : 1;  // Is it valid to return this character when converting LSCP from CP
    WHEN_DBG(
        PTCHAR      pszSynthName;
        )
    };

    typedef struct synthdata SYNTHDATA;

    struct SYNTHCP
    {
        LSCP            lscp;       // LSCP of the synthetic character.
        SYNTHTYPE       type;       // Synthetic character type.
        COneRun *       por;
    };

    #define SIZE_GLYPHABLE_MAP 16

    //
    // Static data declarations
    //

#ifdef _MAC
    static LSCBK s_lscbk;
#else
    static const LSCBK s_lscbk;
#endif
    static const LSTXTCFG s_lstxtcfg;
    static LSIMETHODS s_rgLsiMethods[LSOBJID_COUNT];

    static const RUBYINIT s_rubyinit;
    static const TATENAKAYOKOINIT s_tatenakayokoinit;
    static const HIHINIT s_hihinit;
    static const WARICHUINIT s_warichuinit;
    static const REVERSEINIT s_reverseinit;
    static const WCHAR s_achTabLeader[tomLines];
    static const SYNTHDATA s_aSynthData[SYNTHTYPE_COUNT];

#if defined(UNIX) || defined(_MAC)
    // UNIX can't use these structs directly, because they
    // contain both 8 bytes and 4 bytes members.
    // InitStructName() is used to convert 8->4 bytes members.
    static ::LSIMETHODS s_unix_rgLsiMethods[LSOBJID_COUNT];
    void InitLsiMethodStruct();

    static ::RUBYINIT s_unix_rubyinit;
    int InitRubyinit();

    static ::TATENAKAYOKOINIT s_unix_tatenakayokoinit;
    int InitTatenakayokoinit();

    static ::HIHINIT s_unix_hihinit;
    int InitHihinit();

    static ::WARICHUINIT s_unix_warichuinit;
    int InitWarichuinit();

    static ::REVERSEINIT s_unix_reverseinit;
    int InitReverseinit();
#endif

public:

    //
    // callback functions for LineServices
    //

    void* WINAPI NewPtr(DWORD);
    void  WINAPI DisposePtr(void*);
    void* WINAPI ReallocPtr(void*, DWORD);
    LSERR WINAPI FetchRun(LSCP, LPCWSTR*, DWORD*, BOOL*, PLSCHP, PLSRUN*);
    LSERR WINAPI GetAutoNumberInfo(LSKALIGN*, PLSCHP, PLSRUN*, WCHAR*, PLSCHP, PLSRUN*, BOOL*, long*, long*);
    LSERR WINAPI GetNumericSeparators(PLSRUN, WCHAR*,WCHAR*);
    LSERR WINAPI CheckForDigit(PLSRUN, WCHAR, BOOL*);
    LSERR WINAPI FetchPap(LSCP, PLSPAP);
    LSERR WINAPI FetchTabs(LSCP, PLSTABS, BOOL*, long*, WCHAR*);
    LSERR WINAPI GetBreakThroughTab(long, long, long*);
    LSERR WINAPI FGetLastLineJustification(LSKJUST, LSKALIGN, ENDRES, BOOL*, LSKALIGN*);
    LSERR WINAPI CheckParaBoundaries(LSCP, LSCP, BOOL*);
    LSERR WINAPI GetRunCharWidths(PLSRUN, LSDEVICE, LPCWSTR, DWORD, long, LSTFLOW, int*,long*,long*);
    LSERR WINAPI CheckRunKernability(PLSRUN,PLSRUN, BOOL*);
    LSERR WINAPI GetRunCharKerning(PLSRUN, LSDEVICE, LPCWSTR, DWORD, LSTFLOW, int*);
    LSERR WINAPI GetRunTextMetrics(PLSRUN, LSDEVICE, LSTFLOW, PLSTXM);
    LSERR WINAPI GetRunUnderlineInfo(PLSRUN, PCHEIGHTS, LSTFLOW, PLSULINFO);
    LSERR WINAPI GetRunStrikethroughInfo(PLSRUN, PCHEIGHTS, LSTFLOW, PLSSTINFO);
    LSERR WINAPI GetBorderInfo(PLSRUN, LSTFLOW, long*, long*);
    LSERR WINAPI ReleaseRun(PLSRUN);
    LSERR WINAPI Hyphenate(PCLSHYPH, LSCP, LSCP, PLSHYPH);
    LSERR WINAPI GetHyphenInfo(PLSRUN, DWORD*, WCHAR*);
    LSERR WINAPI DrawUnderline(PLSRUN, UINT, const POINT*, DWORD, DWORD, LSTFLOW, UINT, const RECT*);
    LSERR WINAPI DrawStrikethrough(PLSRUN, UINT, const POINT*, DWORD, DWORD, LSTFLOW, UINT, const RECT*);
    LSERR WINAPI DrawBorder(PLSRUN, const POINT*, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, long, LSTFLOW, UINT, const RECT*);
    LSERR WINAPI DrawUnderlineAsText(PLSRUN, const POINT*, long, LSTFLOW, UINT, const RECT*);
    LSERR WINAPI FInterruptUnderline(PLSRUN, LSCP, PLSRUN, LSCP,BOOL*);
    LSERR WINAPI FInterruptShade(PLSRUN, PLSRUN, BOOL*);
    LSERR WINAPI FInterruptBorder(PLSRUN, PLSRUN, BOOL*);
    LSERR WINAPI ShadeRectangle(PLSRUN, const POINT*, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, long, LSTFLOW, UINT, const RECT*);
    LSERR WINAPI DrawTextRun(PLSRUN, BOOL, BOOL, const POINT*, LPCWSTR, const int*, DWORD, LSTFLOW, UINT, const POINT*, PCHEIGHTS, long, long, const RECT*);
    LSERR WINAPI DrawSplatLine(enum lsksplat, LSCP, const POINT*, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, LSTFLOW, UINT, const RECT*);
    LSERR WINAPI FInterruptShaping(LSTFLOW, PLSRUN, PLSRUN, BOOL*);
    LSERR WINAPI GetGlyphs(PLSRUN, LPCWSTR, DWORD, LSTFLOW, PGMAP, PGINDEX*, PGPROP*, DWORD*);
    LSERR WINAPI GetGlyphPositions(PLSRUN, LSDEVICE, LPWSTR, PCGMAP, DWORD, PCGINDEX, PCGPROP, DWORD, LSTFLOW, int*, PGOFFSET);
    LSERR WINAPI ResetRunContents(PLSRUN, LSCP, LSDCP, LSCP, LSDCP);
    LSERR WINAPI DrawGlyphs(PLSRUN, BOOL, BOOL, PCGINDEX, const int*, const int*, PGOFFSET, PGPROP, PCEXPTYPE, DWORD, LSTFLOW, UINT, const POINT*, PCHEIGHTS, long, long, const RECT*);
    LSERR WINAPI GetGlyphExpansionInfo(PLSRUN, LSDEVICE, LPCWSTR, PCGMAP, DWORD, PCGINDEX, PCGPROP, DWORD, LSTFLOW, BOOL, PEXPTYPE, LSEXPINFO*);
    LSERR WINAPI GetGlyphExpansionInkInfo(PLSRUN, LSDEVICE, GINDEX, GPROP, LSTFLOW, DWORD, long*);
    LSERR WINAPI GetEms(PLSRUN, LSTFLOW, PLSEMS);
    LSERR WINAPI PunctStartLine(PLSRUN, MWCLS, WCHAR, LSACT*);
    LSERR WINAPI ModWidthOnRun(PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
    LSERR WINAPI ModWidthSpace(PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
    LSERR WINAPI CompOnRun(PLSRUN, WCHAR, PLSRUN, WCHAR, LSPRACT*);
    LSERR WINAPI CompWidthSpace(PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR, LSPRACT*);
    LSERR WINAPI ExpOnRun(PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
    LSERR WINAPI ExpWidthSpace(PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
    LSERR WINAPI GetModWidthClasses(PLSRUN, const WCHAR*, DWORD, MWCLS*);
    LSERR WINAPI GetBreakingClasses(PLSRUN, LSCP, WCHAR, BRKCLS*, BRKCLS*);
    LSERR WINAPI FTruncateBefore(PLSRUN, LSCP, WCHAR, long, PLSRUN, LSCP, WCHAR, long, long, BOOL*);
    LSERR WINAPI CanBreakBeforeChar(BRKCLS, BRKCOND*);
    LSERR WINAPI CanBreakAfterChar(BRKCLS, BRKCOND*);
    LSERR WINAPI FHangingPunct(PLSRUN, MWCLS, WCHAR, BOOL*);
    LSERR WINAPI GetSnapGrid(WCHAR*, PLSRUN*, LSCP*, DWORD, BOOL*, DWORD*);
    LSERR WINAPI DrawEffects(PLSRUN, UINT, const POINT*, LPCWSTR, const int*, const int*, DWORD, LSTFLOW, UINT, PCHEIGHTS, long, long, const RECT*);
    LSERR WINAPI FCancelHangingPunct(LSCP, LSCP, WCHAR, MWCLS, BOOL*);
    LSERR WINAPI ModifyCompAtLastChar(LSCP, LSCP, WCHAR, MWCLS, long, long, long*);
    LSERR WINAPI EnumText(PLSRUN, LSCP, LSDCP, LPCWSTR, DWORD, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, BOOL, long*);
    LSERR WINAPI EnumTab(PLSRUN, LSCP, WCHAR *, WCHAR, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long);
    LSERR WINAPI EnumPen(BOOL, LSTFLOW, BOOL, BOOL, const POINT*, long, long);
    LSERR WINAPI GetObjectHandlerInfo(DWORD, void*);

    static void WINAPI AssertFailed(char*, char*, int);
    long WINAPI GetCharGridSize() { return GetLineOrCharGridSize(TRUE); };
    long WINAPI GetLineGridSize() { return GetLineOrCharGridSize(FALSE); };
    long WINAPI GetLineOrCharGridSize(BOOL fGetCharGridSize);
    long WINAPI GetClosestGridMultiple(long lGridSize, long lObjSize);
    RubyInfo* WINAPI GetRubyInfoFromCp(LONG cpRubyText);

    //
    // Installed Line Services object (ILS) support
    // (Note that most of these methods have been moved into subclasses of CILSObjBase)
    //

    HRESULT SetupObjectHandlers();

    LSERR WINAPI CreateILSObj(PLSC,  PCLSCBK, DWORD, PILSOBJ*);
    static LSERR TruncateChunk(PCLOCCHNK, PPOSICHNK);
    static LSERR ForceBreakChunk(PCLOCCHNK, PCPOSICHNK, PBRKOUT);

    //
    // Ruby support
    //

    LSERR WINAPI FetchRubyPosition(LSCP, LSTFLOW, DWORD, const PLSRUN *, PCHEIGHTS, PCHEIGHTS, DWORD, const PLSRUN *, PCHEIGHTS, PCHEIGHTS, PHEIGHTS, PHEIGHTS, long *, long *, long *, enum rubycharjust *, BOOL *);
    LSERR WINAPI FetchRubyWidthAdjust(LSCP, PLSRUN, WCHAR, MWCLS, PLSRUN, enum rubycharloc, long, long *, long *);
    LSERR WINAPI RubyEnum(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, const POINT *, PCHEIGHTS, long, const POINT *, PCHEIGHTS, long, PLSSUBL, PLSSUBL);
    void WINAPI  HandleRubyAlignStyle(COneRun *, enum rubycharjust *, BOOL *);

    //
    // Tatenakayoko (HIV) support
    //

    LSERR WINAPI GetTatenakayokoLinePosition(LSCP, LSTFLOW, PLSRUN, long, PHEIGHTS, PHEIGHTS, long *);
    LSERR WINAPI TatenakayokoEnum(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, LSTFLOW, PLSSUBL);

    //
    // Warichu support
    //

    LSERR WINAPI GetWarichuInfo(LSCP, LSTFLOW, PCOBJDIM, PCOBJDIM, PHEIGHTS, PHEIGHTS, long *);
    LSERR WINAPI FetchWarichuWidthAdjust(LSCP, enum warichucharloc, PLSRUN, WCHAR, MWCLS, PLSRUN, long *, long *);
    LSERR WINAPI WarichuEnum(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, const POINT *, PCHEIGHTS,long, const POINT *, PCHEIGHTS,long,const POINT *, PCHEIGHTS, long, const POINT *, PCHEIGHTS, long, PLSSUBL, PLSSUBL, PLSSUBL, PLSSUBL);

    //
    // HIH support
    //

    LSERR WINAPI HihEnum(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, PLSSUBL);

    //
    // Reverse Object support
    //

    LSERR WINAPI ReverseEnum(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, LSTFLOW, PLSSUBL);

    //
    // Table initialization
    //

    HRESULT CheckSetTables();
    LSERR CheckSetBreaking();
    LSERR CheckSetExpansion();
    LSERR CheckSetCompression();
    LSERR SetModWidthPairs();

private:
    //
    // Helper functions
    //
    LSERR WINAPI GleanInfoFromTheRun(COneRun *por, COneRun **pporOut);
    BOOL WINAPI CheckForSpecialObjectBoundaries(COneRun *por, COneRun **pporOut);
    void DiscardOneRuns();
    COneRun *AdvanceOneRun(LONG lscp);
    BOOL CanMergeTwoRuns(COneRun *por1, COneRun *por2);
    COneRun *MergeIfPossibleIntoCurrentList(COneRun *por);
    COneRun *SplitRun(COneRun *por, LONG cchSplitTill);
    COneRun *AttachOneRunToCurrentList(COneRun *por);


    void PAPFromPF( PLSPAP plspap, const CParaFormat *pPF, BOOL fInnerPF, CComplexRun *pcr );
    void CHPFromCF( COneRun * por, const CCharFormat * pCF );
    void GetCFSymbol(COneRun *por, TCHAR chSymbol, const CCharFormat *pcfIn);
    void GetCFNumber(COneRun *por, const CCharFormat *pcfIn);
    LSERR AppendSynth(COneRun * por, SYNTHTYPE synthtype, COneRun **pporOut);
    LSERR AppendAntiSynthetic(COneRun * por);
    LSERR AppendILSControlChar(COneRun *por, SYNTHTYPE synthtype, COneRun **pporOut);
    LSERR AppendSynthClosingAndReopening(COneRun *por, SYNTHTYPE synthtype, COneRun **pporOut);
    void FreezeSynth() { _fSynthFrozen = TRUE; }
    void ThawSynth() { _fSynthFrozen = FALSE; }
    BOOL IsFrozen() { return _fSynthFrozen; }
    BOOL IsSynthEOL();
    LONG GetDirLevel(LSCP lscp);
    LSERR GetKashidaWidth(PLSRUN plsrun, int *piKashidaWidth);

    typedef enum
    {
        TL_ADDNONE,
        TL_ADDEOS,
        TL_ADDLBREAK
    } TL_ENDMODE;
    LSERR TerminateLine(COneRun *por, TL_ENDMODE tlEndMode, COneRun **pporOut);

    WHEN_DBG( void InitTimeSanityCheck() );

    COneRun * GetAvailableOneRun(CMarkup *pMarkup, CElement *pElement=NULL, LONG cp=-1,
                                 CFlowLayout *pRunOwner=NULL, DWORD tsFlags=0);

    LSERR GetMinDurBreaks( LONG * pdvMin, LONG * pdvMaxDelta );

    HRESULT QueryLinePointPcp(LONG u, LONG v, LSTFLOW *pktFlow, PLSTEXTCELL plstextcell);
    HRESULT QueryLineCpPpoint(LSCP lscp, BOOL fFromGetLineWidth, CDataAry<LSQSUBINFO> * paryLsqsubinfo, PLSTEXTCELL plstextcell, BOOL *pfRTLFlow=NULL);
    HRESULT GetLineWidth(LONG *pdurWithTrailing, LONG *pdurWithoutTrailing);

    LSCP FindPrevLSCP(LSCP lscp, BOOL * pfReverse = NULL);
    void AdjustXPosOfCp(BOOL fRTLDisplay, BOOL fRTLLine, BOOL fRTLFlow, LONG* pxPosOfCp, long xLayoutWidth);

public:
    LSERR ChunkifyTextRun(COneRun  *por, COneRun **pporOut);
    BOOL  NeedToEatThisSpace(COneRun *por);
    LSERR ChunkifyObjectRun(COneRun  *por, COneRun **pporOut);
    LSERR SetRenderingHighlights(COneRun  *por);
    LSERR CheckForUnderLine(COneRun *por);
    LSERR CheckForTransform(COneRun  *por);
    LSERR CheckForPassword(COneRun  *por);

    LSERR AdjustCpLimForNavBRBug(LONG xWrapWidth, LSLINFO *plslinfo);
    VOID  AdjustForRelElementAtEnd();
    
    LSERR ComputeWhiteInfo(LSLINFO *plslinfo, LONG *pxMinLineWidth, DWORD dwlf, LONG durWithTrailing, LONG durWithoutTrailing);
    LONG  RememberLineHeight(LONG cp, const CCharFormat *pCF, const CBaseCcs *pBaseCcs);
    VOID  NoteLineHeight(LONG cp, LONG lLineHeight)
    {
        _lineFlags.AddLineFlag(cp, FLAG_HAS_LINEHEIGHT);
        _lineCounts.AddLineValue(cp, LC_LINEHEIGHT, lLineHeight);
    }
    void  VerticalAlignObjects(CLSMeasurer& rtp, unsigned long cSites, long xLineShift);
    void  AdjustLineHeight();
    LONG  MeasureLineShift(LONG cp, LONG xWidthMax, BOOL fMinMax, LONG * pdxRemainder);
    LONG  CalculateXPositionOfCp(LONG cp, BOOL fAfterPrevCp = FALSE, BOOL* pfRTLFlow=NULL)
    {
        return CalculateXPositionOfLSCP(LSCPFromCP(cp), fAfterPrevCp, pfRTLFlow);
    }
    LONG  CalculateXPositionOfLSCP(LSCP lscp, BOOL fAfterPrevCp = FALSE, BOOL* pfRTLFlow=NULL);

    LONG  CalcPositionsOfRangeOnLine(LONG cpStart, 
                                     LONG cpEnd, 
                                     LONG xShift, 
                                     CDataAry<RECT> * paryChunks);
    LONG  CalcRectsOfRangeOnLine(LONG cpStart,
                                 LONG cpEnd,
                                 LONG xShift,
                                 LONG yPos,
                                 CDataAry<RECT> * paryChunks,
                                 DWORD dwFlags);

    void  RecalcLineHeight(CCcs * pccs, CLine *pli);
    void  TestForTransitionToOrFromRelativeChunk(CLSMeasurer& lsme,
                                                 BOOL fRelative,
                                                 CElement *pElementLayout);
    void  UpdateLastChunkInfo(CLSMeasurer& lsme,
                              CElement *pElementRelative,
                              CElement *pElementLayout);
    LONG  AddNewChunk(LONG cp);
    LONG  WidthOfChunksSoFar() {return _plcFirstChunk ? WidthOfChunksSoFarCore() : 0;}
    LONG  WidthOfChunksSoFarCore();

    long  GetNestedElementCch(CElement *pElement, CTreePos **pptplast = NULL)
    {
        return _pFlowLayout->GetNestedElementCch(pElement, pptplast,
            _treeInfo._cpLayoutLast,
            _treeInfo._ptpLayoutLast);
    }

    BOOL  IsOwnLineSite(COneRun *por);
    VOID  DecideIfLineCanBeBlastedToScreen(LONG cpEndLine, DWORD dwlf);

    void  SetPOLS(CFlowLayout * pFlowLayout,
                  CCalcInfo * pci);
    void  ClearPOLS();

    HRESULT  Setup(LONG xWidthMaxAvail, LONG cp, CTreePos *ptp,
                const CMarginInfo *pMarginInfo, const CLine * pli, BOOL fMinMaxPass);

    // Used in conjunction with WhiteAtBOL.  This function
    // returns true if the specified cp has nothing but whitespace
    // in front of it on the line.  The answer is based on CLineServices
    // instance variables set with the WhiteAtBOL method.
    BOOL IsFirstNonWhiteOnLine(LONG cpCurr)
    {
        Assert(_cpStart != -1);
        return (cpCurr - _cpStart <= CchSkipAtBOL());
    }

    LONG CchSkipAtBOL()
    {
        return _cWhiteAtBOL + _cAlignedSitesAtBOL;
    }

    void DiscardLine();

    // Tells the LS object that this CP is white, and that everything
    // in front of it on the line is white.
    // This must be called
    // in order for every white character at the front
    // of a line to work properly.
    void WhiteAtBOL(LONG cp, LONG cch = 1)
    {
        Assert(_cpStart != -1);
        if (cp >= _cpAccountedTill)
        {
            _cWhiteAtBOL += cch;
            _cpAccountedTill = cp + cch;
        }
    }

    void AlignedAtBOL(LONG cp, LONG cch = 1)
    {
        Assert(_cpStart != -1);
        if (cp >= _cpAccountedTill)
        {
            _cAlignedSitesAtBOL += cch;
            _cpAccountedTill = cp + cch;
        }
    }

    BOOL IsAdornment()
    {
        Assert(!_pCFLi || GetRenderer());
        return _pCFLi == NULL ? FALSE : TRUE;
    }

    BOOL IsDummyLine(LONG cp)
    {
        return IsFirstNonWhiteOnLine(cp) && !_fLineWithHeight && !_li._fHasBulletOrNum;
    }

    BOOL LineHasOnlySites(LONG cp)
    {
        return (   (_lineFlags.GetLineFlags(cp) & FLAG_HAS_EMBED_OR_WBR)
                && LineHasNoText(cp)
               );
    }

    BOOL LineHasNoText(LONG cp);
    LONG CPFromLSCP(LONG lscp);
    LONG LSCPFromCP(LONG cp);
    LONG CPFromLSCPCore(LONG lscp, COneRun **ppor);
    LONG LSCPFromCPCore(LONG cp,   COneRun **ppor);

    COneRun *FindOneRun(LSCP lscp);
    CTreePos *FigureNextPtp(LONG cp);

    void ProcessOneChar(LONG cpCurr, int *pdu, int *rgDu, TCHAR ch);
    int  GetLetterSpacing(const CCharFormat *pCF);
    void MeasureCharacter(TCHAR ch, LONG cpCurr, int *rgDu, CCcs *pccs, int *pdu);

#if  DBG==1    
    LSERR GetRunCharWidthsEx(
                             PLSRUN plsrun,
                             LPCWSTR pwchRun,
                             DWORD cwchRun,
                             LONG du,
                             int *rgDu,
                             long *pduDu,
                             long *plimDu,
                             LONG *pxLetterSpacingRemovedDbg
                            );
#else
    LSERR GetRunCharWidthsEx(
                             PLSRUN plsrun,
                             LPCWSTR pwchRun,
                             DWORD cwchRun,
                             LONG du,
                             int *rgDu,
                             long *pduDu,
                             long *plimDu
                            );
#endif
    
public:
#ifdef PRODUCT_PROF
    static int s_iNotJustAscii;

    void NotJustAscii();  // Just for profiling statistics.  Called from GetRunCharWidths;
#endif // PRODUCT_PROF



    typedef enum
    {
        LSMODE_NONE,
        LSMODE_MEASURER,
        LSMODE_HITTEST,
        LSMODE_RENDERER
    } LSMODE;

    PLSC GetContext() { AssertSz( _plsc, "No LineServices context" ); return _plsc; }

    void SetRenderer(CLSRenderer *pRenderer, BOOL fWrapLongLines, const CCharFormat *pCFLi=NULL);
    void SetMeasurer(CLSMeasurer *pMeasurer, LSMODE lsMode, BOOL fWrapLongLines);
    CLSRenderer *GetRenderer();
    CLSMeasurer *GetMeasurer();

    // chunk related utility functions
    void InitChunkInfo(LONG cp);
    void DeleteChunks();
    CLSLineChunk * GetFirstChunk() { return _plcFirstChunk; }
    BOOL HasChunks() { return _plcFirstChunk != NULL; }

    // Get a cached/new pccs
    CCcs *GetCcs(COneRun *por, HDC hdc, CDocInfo *pdi);

    // Fontlinking support functions
    SCRIPT_ID UnUnifyHan( HDC hdc, UINT uiFamilyCodePage, LCID lcid, COneRun *por );
    BOOL      DisambiguateScriptId( HDC hdc, CBaseCcs * pBaseCcs, COneRun *por, SCRIPT_ID * psidAlt, BYTE *pbCharSetAlt );
    BOOL      ShouldSwitchFontsForPUA( HDC hdc, UINT uiFamilyCodePage, CBaseCcs * pBaseCcs, const CCharFormat * pcf, SCRIPT_ID *psid );
    long      GetScriptTextBlock( CTreePos * ptp, LONG *pcp );
    DWORD     GetTextCodePages( DWORD dwPriorityCodePages, long cp, long cch );

    CDataAry<RubyInfo> _aryRubyInfo;

private:

    operator PLSC() { return _plsc; }

private:

    DWORD GetRenderingHighlights( int cpLineMin, int cpLineMax );

    //
    // Data members
    //
    WHEN_DBG(BOOL _lockRecrsionGuardFetchRun);
    
    PLSC                _plsc;
    CMarkup *           _pMarkup;           // backpointer

    CFlowLayout *       _pFlowLayout;
    CCalcInfo *         _pci;
    CLine               _li;
    const CParaFormat * _pPFFirst;
    BOOL                _fInnerPFFirst;
    const CMarginInfo * _pMarginInfo;

    LONG                _cWhiteAtBOL;
    LONG                _cAlignedSitesAtBOL;
    LONG                _cpStart;
    LONG                _cpAccountedTill;
    LONG                _cInlinedSites;
    LONG                _cAbsoluteSites;
    LONG                _cAlignedSites;
    const CLine *       _pli;
    LONG                _lCharGridSize;
    LONG                _lLineGridSize;
    LONG                _yMaxHeightForRubyBase;
    CTreeNode *         _pNodeForAfterSpace;
    
    union
    {
        DWORD           _dwProps;
        struct
        {
            DWORD       _fSingleSite : 1;       // Unused for now. Maybe use for glyphs?
            DWORD       _fFoundLineHeight : 1;
            DWORD       _fHasTxtSiteEnd : 1;
            DWORD       _fMinMaxPass : 1;       // Calculating min/max?

            DWORD       _fSynthFrozen : 1;      // True if the synthetic store is frozen
            DWORD       _fHasRelative : 1;
            DWORD       _fUnused : 1;
            DWORD       _fLineWithHeight : 1;   // The current line has height eventhough it is empty

            DWORD       _fGlyphOnLine : 1;      // There's something that needs glyphing on the line.
            DWORD       _fWrapLongLines: 1;
            DWORD       _fHasIMEHighlight: 1;   // There's IME action on this line
            DWORD       _fSawATab : 1;          // Tab forces us to not blast.

            DWORD       _fMayHaveANOBR : 1;     // Line may have a nobr on it.
            DWORD       _fScanForCR : 1;
            DWORD       _fExpectCRLF : 1;
            DWORD       _fExpansionOrCompression : 1; // Expansion or compression needs to take place.

            DWORD       _fHasOverhang : 1;      // Does _any_ char on the line have a overhang?
            DWORD       _fNoBreakForMeasurer : 1;
        };
    };

    // Please see declaration in \src\site\include\cfpf.hxx 
    // for what DECLARE_SPECIAL_OBJECT_FLAGS entails.  Essentialy
    // this declares bit fields for CF properties that have to do
    // with LS objects
    DECLARE_SPECIAL_OBJECT_FLAGS();

    // Use this function to clear out line properties and special
    // object flag properties
    inline void ClearLinePropertyFlags() 
    { 
        _dwProps = 0;  
        _bSpecialObjectFlagsVar = 0; 
    }


    // The following flags do not get cleared per line as the previous set of flags do.
    // They are constant for the scope of SetPOLs to ClearPOLs
    union
    {
        DWORD           _dwPropsConstant;
        struct
        {
            BOOL        _fIsEditable : 1;   // is the current flowlayout editable?
            BOOL        _fIsTextLess : 1;   // is the current flowlayout textless?
            BOOL        _fIsTD : 1;         // is the current flowlayout a table cell?

            //
            // if the flowlayout we are measuring has any sites then in the min-max
            // mode, we need not compute the height of the lines (in GetRunTextMetrics)
            // at all since a natural pass will be done later on. Remember that this
            // flag is valid only in min-max mode.
            //
            BOOL        _fHasSites : 1;
        };
    };

    LONG                _xTDWidth;          // non-max-long if TD with width attribute


    CBidiLine *         _pBidiLine;

    PLSLINE             _plsline;
    TCHAR               _chPassword;
    LSTBD               _alstbd[MAX_TAB_STOPS];
    CLSMeasurer       * _pMeasurer;

    const CCharFormat * _pCFLi;         // CF used by the li. Also signifies
                                        // that we are measuring the bullet
                                        // for rendering purposes

    LSMODE              _lsMode;
    CLineFlags          _lineFlags;
    CLineCounts         _lineCounts;
    LONG                _lMaxLineHeight;
    LONG                _xWidthMaxAvail;
    LONG                _xWrappingWidth;
    LONG                _cchSkipAtBOL;

    LONG                _cpLim;
    LONG                _lscpLim;

    WHEN_DBG( long _cpBreakAt );

    // To put serial numbers on the contexts for tracing.
    WHEN_DBG( int _nSerial );
    WHEN_DBG( static int s_nSerialMax );

    //
    // For Min-max enumaration postpass
    //

    long _dvMaxDelta;

    //
    // chunk related stuff
    //

    LONG          _cpLastChunk;
    LONG          _xPosLastChunk;
    CLSLineChunk *_plcFirstChunk;
    CLSLineChunk *_plcLastChunk;
    CElement     *_pElementLastRelative;
    BOOL          _fLastChunkSingleSite;
    BOOL          _fLastChunkHasBulletOrNum;


    //
    // cache for the font
    //
    SCRIPT_ID sid;
    const CCharFormat *_pCFCache;
    CCcs  *_pccsCache;
    WHEN_DBG( LONG _cCalledTotal );
    WHEN_DBG( LONG _cCacheHits );

    //
    // cache for fontlinking
    //

    const CCharFormat * _pCFAltCache; // pCF on which the altfont is based (not for the altfont itself)
    CCcs  *_pccsAltCache;            // alternate pccs, used for fontlinking.
    SCRIPT_ID _sidAltCache;          // script id of alt font
    LANGID _lidAltCache;             // language of alt font

    //
    // Cached Tree Info
    //
    CTreeInfo _treeInfo;

    //
    // The list of free one runs
    //
    COneRunFreeList _listFree;

    //
    // Current breaking/expansion/compression table
    //

    struct lsbrk * _lsbrkCurr;
    BYTE * _abIndexExpansion;
    BYTE * _abIndexPract;

    //
    // List of currently in-use one runs
    //
    COneRunCurrList _listCurrent;
    WHEN_DBG(void DumpList());
    WHEN_DBG(void DumpFlags());
    WHEN_DBG(void DumpCounts());

#if DBG == 1 || defined(DUMPTREE)
    //
    // DumpTree shortcut.
    //
    void DumpTree();
#endif

#if DBG==1
    //
    // Unicode Partition etc. Information
    //

    void DumpUnicodeInfo(TCHAR ch);
    const char * BrkclsNameFromCh(TCHAR ch, BOOL fStrict);
#endif
};  // class CLineServices


class COneRun
{
public:
    enum ORTYPES
    {
        OR_NORMAL    = 0x00,
        OR_SYNTHETIC = 0x01,
        OR_ANTISYNTHETIC = 0x02
    };

    enum charWidthClass {
        charWidthClassFullWidth,
        charWidthClassHalfWidth,
        charWidthClassCursive,
        charWidthClassUnknown
    };

    ~COneRun()
    {
        Deinit();
    }

    void Deinit();

    LONG Cp()
    {
        return _lscpBase - _chSynthsBefore;
    }
    const CCharFormat *GetCF()
    {
        Assert (_pCF);
        return (_pCF);
    }
    CCharFormat *GetOtherCF();
    const CParaFormat *GetPF()
    {
        Assert (_pPF);
        return (_pPF);
    }

    const CFancyFormat *GetFF()
    {
        Assert (_pFF);
        return (_pFF);
    }

    CComplexRun * GetComplexRun()
    {
        return _pComplexRun;
    }
    CComplexRun * GetNewComplexRun()
    {
        if (_pComplexRun != NULL)
        {
            delete _pComplexRun;
        }
        _pComplexRun = new CComplexRun;
        return _pComplexRun;
    }
    void SetConvertMode(CONVERTMODE cm)
    {
        AssertSz(_bConvertMode == CM_UNINITED, "Setting cm more than once!");
        _bConvertMode = cm;
    }
    LPTSTR SetCharacter(TCHAR ch)
    {
        _cstrRunChars.Set(&ch, 1);
        return _cstrRunChars;
    }
    LPTSTR SetString(LPTSTR pstr)
    {
        _cstrRunChars.Set(pstr);
        return _cstrRunChars;
    }
    BOOL IsSelected() { return _fSelected; }
    void Selected(CFlowLayout *pFlowLayout, BOOL fSelected = TRUE);
    void ImeHighlight( CFlowLayout * pFlowLayout, DWORD dwImeHighlight );
    BOOL HasBgColor() { return _fHasBackColor; }      // allow override of formats
    BOOL HasTextColor() { return _fHasTextColor; }    // allow override of formats
    void SetCurrentBgColor(CFlowLayout *pFlowLayout);
    CColorValue GetCurrentBgColor() { return _ccvBackColor; }
    void SetCurrentTextColor(CFlowLayout *pFlowLayout);
    charWidthClass GetCharWidthClass();
    BOOL IsSameGridClass(COneRun * por);
    BOOL IsOneCharPerGridCell();

    CLayout *GetLayout(CFlowLayout *pFlowLayout)
    {
        CTreeNode *pNode = Branch();
        CLayout *pLayout = pNode->NeedsLayout() ? pNode->GetUpdatedLayout() : pFlowLayout;
        Assert(pLayout && pLayout == pNode->GetUpdatedNearestLayout());
        return pLayout;
    }
    // BUGBUG SLOWBRANCH: GetBranch is **way** too slow to be used here.
    CTreeNode *Branch() { return _ptp->GetBranch(); }
    COneRun *Clone(COneRun *porClone);
    void SetSidFromTreePos(const CTreePos * ptp)
    {
        _sid = (_ptp && _ptp->IsText()) ? _ptp->Sid() : sidDefault;
    }

    BOOL IsNormalRun()        { return _fType == OR_NORMAL;}
    BOOL IsSyntheticRun()     { return _fType == OR_SYNTHETIC; }
    BOOL IsAntiSyntheticRun() { return _fType == OR_ANTISYNTHETIC; }
    BOOL WantsLSCPStop()      { return (_fType != OR_SYNTHETIC ||  CLineServices::s_aSynthData[_synthType].fLSCPStop); }

    void MakeRunNormal()        {_fType = OR_NORMAL; }
    void MakeRunSynthetic()     {_fType = OR_SYNTHETIC; }
    void MakeRunAntiSynthetic() {_fType = OR_ANTISYNTHETIC; }
    void FillSynthData(CLineServices::SYNTHTYPE synthtype);

public:
    COneRun *_pNext;
    COneRun *_pPrev;

    LONG   _lscpBase;                   // The lscp of this run
    LONG   _lscch;                      // The number of characters in this run
    LONG   _lscchOriginal;              // The number of original characters in this run
    const  TCHAR *_pchBase;             // The ptr to the characters
    const  TCHAR *_pchBaseOriginal;     // The original ptr to the characters
    LSCHP  _lsCharProps;                // Char props to pass back to LS
    LONG   _chSynthsBefore;             // How many synthetics before this run?

    union
    {
        DWORD _dwProps;
        struct
        {
            //
            // These flags are specific to the list and do not mean
            // anything semantically
            //
            unsigned int _fType: 2;             // Normal/Synthetic/Antisynthetic run?
            BOOL _fGlean: 1;                    // Do we need to glean this run at all?
            BOOL _fNotProcessedYet: 1;          // The run has not been processed yet
            BOOL _fCannotMergeRuns: 1;          // Cannot merge runs
            BOOL _fCharsForNestedElement: 1;    // Chars for nested elements
            BOOL _fCharsForNestedLayout: 1;     // Chars for nested layouts
            BOOL _fCharsForNestedRunOwner: 1;   // Run has nested run owners
            BOOL _fMustDeletePcf: 1;            // Did we allocate a pcf for this run?
            BOOL _fSynthedForEditGlyph:1;       // Was a synth run for this already created?
            BOOL _fMakeItASpace:1;              // Convert into space?
            
            //
            // These runs have some semantic meaning for LS.
            //
            BOOL _fSelected: 1;                 // Is the run selected
            BOOL _fHasBackColor: 1;             // For selection and IME
            BOOL _fHasTextColor: 1;             // For IME
            BOOL _fHidden: 1;                   // Is the run hidden
            BOOL _fNoTextMetrics: 1;            // This run has no text metrics
            DWORD _dwImeHighlight: 3;           // IME highlight
            BOOL _fUnderlineForIME: 1;          // IME highlight
            DWORD _brkopt:6;                    // Use break options for GetBreakingClasses
            DWORD _csco: 6;                     // CSCOPTION
        };
    };

    //
    // The Formats specific stuff here
    //
    CCharFormat       *_pCF;
    BOOL               _fInnerCF;
    const CParaFormat *_pPF;
    BOOL               _fInnerPF;
    const CFancyFormat*_pFF;

    CONVERTMODE               _bConvertMode;
    CStr                      _cstrRunChars;
    CColorValue               _ccvBackColor;
    CColorValue               _ccvTextColor;
    CComplexRun              *_pComplexRun;
    CLineServices::SYNTHTYPE  _synthType;

    union
    {
        DWORD _dwProps2;
        struct
        {
            DWORD   _sid:8;
            DWORD   _fIsStartOrEndOfObj:1;
            DWORD   _fIsArtificiallyStartedNOBR:1;
            DWORD   _fIsArtificiallyTerminatedNOBR:1;
            DWORD   _dwUnused:21;
        };
    };

    // 
    // Run width specific stuff here.
    // These members are valid only for runs that have grid layout.
    // For all others runs these members are set to 0.
    // 
    LONG  _xWidth;
    LONG  _xDrawOffset;
    LONG  _xOverhang; /* Can be a short */

    //
    // Finally the tree pointer. This is where the current run is
    // WRT our tree.
    //
    CTreePos                 *_ptp;

#if DBG == 1 || defined(DUMPRUNS)
    int _nSerialNumber;
#endif
};

// This class replaces struct dobj in a C++ context.  It is the
// abstract base class for all the specific types of ILS objects
// to subclass.
// Each CILSObjBase subclass will have a corresponding subclass
// of this class.
class CDobjBase
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))

// typedefs
private:
    typedef CILSObjBase* PILSOBJ;
    typedef COneRun* PLSRUN;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

public:
    //
    // Data members
    //

    PILSOBJ  _pilsobj;      // pointer back to the CILSObjBase that created us
    PLSDNODE _plsdnTop;

    // We cache the difference between the minimum and maximum width
    // in this variable.  After a min-max pass, we enumerate our dobj
    // and compute the true minimum for those sites where the min and
    // max widths differ (e.g. table)
    LONG        _dvMinMaxDelta;

    //
    // Methods
    //

protected:
    CDobjBase(PILSOBJ pilsobjNew, PLSDNODE plsdn);
    virtual void UselessVirtualToMakeTheCompilerHappy() {}

public:
    CLineServices* GetPLS() { return _pilsobj->_pLS; };
    PLSC GetPLSC() { return GetPLS()->_plsc;  };

    LSERR QueryObjDim(POBJDIM pobjdim)
    {
        return LsdnQueryObjDimRange(GetPLSC(), _plsdnTop, _plsdnTop, pobjdim);
    };
};

class CEmbeddedDobj: public CDobjBase
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CEmbeddedDobj))

// typedefs
private:
    typedef CILSObjBase* PILSOBJ;
    typedef COneRun* PLSRUN;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

public:
    //
    // Data Members
    //

    // IE3-only sites (MARQUEE/OBJECT) will break between themselves and
    // text.  We note whether we have such a site in the dobj to facilitate
    // breaking.

    BOOL        _fIsBreakingSite : 1;

    //
    // Methods
    //

    CEmbeddedDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn);
    static LSERR FindPrevBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
    static LSERR FindNextBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);

    //
    // Line services Callbacks
    //

    static LSERR WINAPI DestroyDObj(PDOBJ);
    static LSERR WINAPI Enum(PDOBJ, PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT *, PCHEIGHTS, long);
    static LSERR WINAPI Display(PDOBJ, PCDISPIN);
    static LSERR WINAPI QueryPointPcp(PDOBJ, PCPOINTUV, PCLSQIN, PLSQOUT);
    static LSERR WINAPI QueryCpPpoint(PDOBJ, LSDCP, PCLSQIN, PLSQOUT);
    static LSERR WINAPI GetModWidthPrecedingChar(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
    static LSERR WINAPI GetModWidthFollowingChar(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
    static LSERR WINAPI SetBreak(PDOBJ, BRKKIND, DWORD, BREAKREC*, DWORD*);
    static LSERR WINAPI GetSpecialEffectsInside(PDOBJ, UINT*);
    static LSERR WINAPI FExpandWithPrecedingChar(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
    static LSERR WINAPI FExpandWithFollowingChar(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
    static LSERR WINAPI CalcPresentation(PDOBJ, long, LSKJUST, BOOL);
};


class CNobrDobj : public CDobjBase
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CNobrDobj))

// typedefs
private:
    typedef CILSObjBase* PILSOBJ;
    typedef COneRun* PLSRUN;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

public:
    //
    // Data Members
    //

    PLSSUBL     _plssubline;    // the subline representing this nobr block
    BRKCOND     _brkcondBefore; // brkcond at the beginning of the NOBR
    BRKCOND     _brkcondAfter;  // brkcond at the end of the NOBR

    //
    // Methods
    //

    CNobrDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn);
    BRKCOND GetBrkcondBefore(COneRun * por);
    BRKCOND GetBrkcondAfter(COneRun * por, LONG dcp);
    static LSERR FindPrevBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
    static LSERR FindNextBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);

    //
    // Line services Callbacks
    //

    static LSERR WINAPI DestroyDObj(PDOBJ);
    static LSERR WINAPI Enum(PDOBJ, PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT *, PCHEIGHTS, long);
    static LSERR WINAPI Display(PDOBJ, PCDISPIN);
    static LSERR WINAPI QueryPointPcp(PDOBJ, PCPOINTUV, PCLSQIN, PLSQOUT);
    static LSERR WINAPI QueryCpPpoint(PDOBJ, LSDCP, PCLSQIN, PLSQOUT);
    static LSERR WINAPI GetModWidthPrecedingChar(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
    static LSERR WINAPI GetModWidthFollowingChar(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
    static LSERR WINAPI SetBreak(PDOBJ, BRKKIND, DWORD, BREAKREC*, DWORD*);
    static LSERR WINAPI GetSpecialEffectsInside(PDOBJ, UINT*);
    static LSERR WINAPI FExpandWithPrecedingChar(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
    static LSERR WINAPI FExpandWithFollowingChar(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
    static LSERR WINAPI CalcPresentation(PDOBJ, long, LSKJUST, BOOL);
};

class CGlyphDobj : public CDobjBase
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CGlyphDobj))

// typedefs
private:
    typedef CILSObjBase* PILSOBJ;
    typedef COneRun* PLSRUN;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

public:

    //
    // Data Members
    //

    PLSSUBL              _plssubline;  // the subline representing this nobr block
    CGlyphRenderInfoType _RenderInfo;
    BOOL                 _fBegin:1;
    BOOL                 _fNoScope:1;
    
    //
    // Methods
    //

    CGlyphDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn);
    static LSERR FindPrevBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
    static LSERR FindNextBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);

    //
    // Line services Callbacks
    //

    static LSERR WINAPI DestroyDObj(PDOBJ);
    static LSERR WINAPI Enum(PDOBJ, PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT *, PCHEIGHTS, long);
    static LSERR WINAPI Display(PDOBJ, PCDISPIN);
    static LSERR WINAPI QueryPointPcp(PDOBJ, PCPOINTUV, PCLSQIN, PLSQOUT);
    static LSERR WINAPI QueryCpPpoint(PDOBJ, LSDCP, PCLSQIN, PLSQOUT);
    static LSERR WINAPI GetModWidthPrecedingChar(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
    static LSERR WINAPI GetModWidthFollowingChar(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
    static LSERR WINAPI SetBreak(PDOBJ, BRKKIND, DWORD, BREAKREC*, DWORD*);
    static LSERR WINAPI GetSpecialEffectsInside(PDOBJ, UINT*);
    static LSERR WINAPI FExpandWithPrecedingChar(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
    static LSERR WINAPI FExpandWithFollowingChar(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
    static LSERR WINAPI CalcPresentation(PDOBJ, long, LSKJUST, BOOL);
};

inline LONG CLineServices::CPFromLSCP(LONG lscp)
{
    return CPFromLSCPCore(lscp, NULL);
}

inline LONG CLineServices::LSCPFromCP(LONG cp)
{
    return LSCPFromCPCore(cp, NULL);
}

inline void CLineServices::ProcessOneChar(LONG cpCurr, int *pdu, int *rgDu, TCHAR ch)
{
    if (ch == WCH_NBSP)
    {
        _lineFlags.AddLineFlag(cpCurr + pdu - rgDu, FLAG_HAS_NBSP);
    }
}

inline int CLineServices::GetLetterSpacing(const CCharFormat *pCF)
{
    int xLetterSpacing;
    CUnitValue cuvLetterSpacing;

    // CSS Letter-spacing
    cuvLetterSpacing = pCF->_cuvLetterSpacing;
    switch (cuvLetterSpacing.GetUnitType())
    {
    case CUnitValue::UNIT_INTEGER:
        xLetterSpacing = (int)cuvLetterSpacing.GetUnitValue();
        break;

    case CUnitValue::UNIT_ENUM:
        xLetterSpacing = 0;     // the only allowable enum value for l-s is normal=0
        break;

    default:
        xLetterSpacing = (int)cuvLetterSpacing.XGetPixelValue(_pci, 0,
            _pFlowLayout->GetFirstBranch()->GetFontHeightInTwips(&cuvLetterSpacing));
    }

    return xLetterSpacing;
}

inline void CLineServices::MeasureCharacter(TCHAR ch, LONG cpCurr, int *rgDu, CCcs *pccs, int *pdu)
{
    long duChar = 0;
    ProcessOneChar(cpCurr, pdu, rgDu, ch);
    pccs->Include(ch, duChar);
    *pdu = duChar;
}

inline BOOL COneRun::IsSameGridClass(COneRun * por)
{
    return (    por 
            &&  por->_ptp->IsText() 
            &&  por->GetCF()->HasCharGrid(TRUE)
            &&  por->GetCF()->GetLayoutGridType(TRUE) == GetCF()->GetLayoutGridType(TRUE) 
            &&  por->GetCharWidthClass() == GetCharWidthClass() );
}

inline BOOL COneRun::IsOneCharPerGridCell()
{
    return (    GetCharWidthClass() == charWidthClassFullWidth 
            ||  (   GetCF()->GetLayoutGridType(TRUE) == styleLayoutGridTypeFixed 
                &&  GetCharWidthClass() == COneRun::charWidthClassHalfWidth ) );
}

long LooseTypeWidthIncrement(TCHAR c, BOOL fWide, long grid);

#if defined(TEXTPRIV)
#define DeclareLSTag(tagName,tagDescr) DeclareTag(tagName, "!!!LineServices", tagDescr);
#else
#define DeclareLSTag(tagName,tagDescr) DeclareTag(tagName, "LineServices", tagDescr);
#endif // TEXTPRIV

#pragma INCMSG("--- End 'linesrv.hxx'")
#else
#pragma INCMSG("*** Dup 'linesrv.hxx'")
#endif
