/*
 *  LSRENDER.HXX -- CLSRenderer class
 *
 *  Authors:
 *      Sujal Parikh
 *      Chris Thrasher
 *      Paul  Parker
 *
 *  History:
 *      2/6/98     sujalp created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I_LSRENDER_HXX_
#define I_LSRENDER_HXX_
#pragma INCMSG("--- Beg 'lsrender.hxx'")

#ifndef X_LSM_HXX_
#define X_LSM_HXX_
#include "lsm.hxx"
#endif

#ifndef X_NUMCONV_HXX_
#define X_NUMCONV_HXX_
#include "numconv.hxx"
#endif

#ifndef X_SELRENSV_HXX_
#define X_SELRENSV_HXX_
#include "selrensv.hxx" // For defintion of HighlightSegment
#endif

class CDisplay;
class CLineServices;

class CLSRenderer : public CLSMeasurer
{
public:
    typedef enum
    {
        DB_NONE, DB_BLAST, DB_LINESERV
    } TEXTOUT_DONE_BY;
    
    friend class CLineServices;
    friend class CGlyphDobj;

private:
    CFormDrawInfo * _pDI;           // device to which the site is rendering

    HFONT       _hfontOrig;         // original font of _hdc

    RECT        _rcView;            // view rect (in _hdc logical coords)
    RECT        _rcRender;          // rendered rect. (in _hdc logical coords)
    RECT        _rcClip;            // running clip/erase rect. (in _hdc logical coords)

#if DBG==1
    RECT        _rcpDIClip;         // The clip rect. Just being paranoid to check
                                    // that the clip rect a'int modified between
                                    // when we cache _rcLine and when we use it.
#endif

    COLORREF    _crForeDisabled;    // foreground color for disabled text
    COLORREF    _crShadowDisabled;  // the shadow color for disabled text


    
#if !defined(MW_MSCOMPATIBLE_STRUCT)
    union
    {
      DWORD     _dwFlagsVar;        // All together now
      struct
      {
#endif
        DWORD   _fDisabled:1;           // draw text with disabled effects?

        DWORD   _fRenderSelection:1;    // Render selection?
        DWORD   _fEnhancedMetafileDC:1; // Use ExtTextOutA to hack around all
                                        // sort of Win95FE EMF or font problems
        DWORD   _fFoundTrailingSpace:1; // TRUE if trailing space found in chunk.
        DWORD   _unused:28;             // To assure use of a DWORD for IEUNIX alignment
#if !defined(MW_MSCOMPATIBLE_STRUCT)
      };
    };
    DWORD& _dwFlags() { return _dwFlagsVar; }
#else
    DWORD& _dwFlags() { return *(((DWORD *)&_bUnderlineType) - 1); }
#endif

    LONG        _xRelOffset;

    void        Init(CFormDrawInfo * pDI);  // initialize everything to zero

protected:

    POINT   _ptCur;     // current rendering position on screen (in hdc logical coord.)

    LONG    _cpStartRender;
    LONG    _cpStopRender;
    LONG    _xAccumulatedWidth;
    
    // args to TextOut
    #define TEXTOUT_DEFAULT      0x0000   // Normal Operation
    #define TEXTOUT_NOTRANSFORM  0x0001   // Do not apply char transform

public:

    CPtrAry<HighlightSegment*> _aryHighlight;
    int _cpSelMin;                  // Min CP of Selection if any
    int _cpSelMax;                  // Max CP of Selection if any
    TEXTOUT_DONE_BY _lastTextOutBy;
    
    LONG TextOut(COneRun *por,              // IN
                 BOOL fStrikeout,           // IN
                 BOOL fUnderline,           // IN
                 const POINT* pptText,      // IN
                 LPCWSTR pch,               // IN was plwchRun
                 const int* lpDx,           // IN was rgDupRun
                 DWORD cch,                 // IN was cwchRun
                 LSTFLOW kTFlow,            // IN
                 UINT kDisp,                // IN
                 const POINT* pptRun,       // IN
                 PCHEIGHTS heightPres,      // IN
                 long dupRun,               // IN
                 long dupLimUnderline,      // IN
                 const RECT* pRectClip);    // IN
    LONG GlyphOut(COneRun * por,            // IN was plsrun
                  BOOL fStrikeout,          // IN
                  BOOL fUnderline,          // IN
                  PCGINDEX pglyph,          // IN
                  const int* pDx,           // IN was rgDu
                  const int* pDxAdvance,    // IN was rgDuBeforeJust
                  PGOFFSET pOffset,         // IN was rgGoffset
                  PGPROP pVisAttr,          // IN was rgGProp
                  PCEXPTYPE pExpType,       // IN was rgExpType
                  DWORD cglyph,             // IN
                  LSTFLOW kTFlow,           // IN
                  UINT kDisp,               // IN
                  const POINT* pptRun,      // IN
                  PCHEIGHTS heightsPres,    // IN
                  long dupRun,              // IN
                  long dupLimUnderline,     // IN
                  const RECT* pRectClip);   // IN
    void    FixupClipRectForSelection();

protected:
    CCcs   *SetNewFont(COneRun *por);
    void    AdjustColors(COneRun *por);

    BOOL    RenderStartLine(COneRun *por);
    BOOL    RenderBullet(COneRun *por);
    BOOL    RenderBulletChar(const CCharFormat *pCFLi, LONG dxOffset, BOOL fRTLOutter);
    BOOL    RenderBulletImage(const CParaFormat *pPF, LONG dxOffset);

    void    GetListIndexInfo(CTreeNode *pLINode,
                             COneRun *por,
                             const CCharFormat *pCFLi,
                             TCHAR achNumbering[NUMCONV_STRLEN]);
    COneRun *FetchLIRun(LONG     cp,         // IN
                        LPCWSTR *ppwchRun,   // OUT
                        DWORD   *pcchRun);   // OUT

    LSERR   DrawUnderline(COneRun *por,           // IN
                          UINT kUlBase,           // IN
                          const POINT* pptStart,  // IN
                          DWORD dupUl,            // IN
                          DWORD dvpUl,            // IN
                          LSTFLOW kTFlow,         // IN
                          UINT kDisp,             // IN
                          const RECT* prcClip);   // IN
    LSERR   DrawEnabledDisabledLine(UINT kUlBase,    // IN
                                    const POINT* pptStart,  // IN
                                    DWORD dupUl,            // IN
                                    DWORD dvpUl,            // IN
                                    LSTFLOW kTFlow,         // IN
                                    UINT kDisp,             // IN
                                    const RECT* prcClip);   // IN
    LSERR   DrawLine(UINT     kUlBase,        // IN
                     COLORREF colorUnderLine, // IN
                     CRect    *pRectLine);    // IN
    
    CTreePos* BlastLineToScreen(CLine& li);
    LONG      TrimTrailingSpaces(LONG cchToRender,
                                 LONG cp,
                                 CTreePos *ptp,
                                 LONG cchRemainingInLine);

public:
    CLSRenderer (const CDisplay * const pdp, CFormDrawInfo * pDI);
    CLSRenderer (const CDisplay * const pdp, long cp, CFormDrawInfo * pDI);
    ~CLSRenderer ();

            VOID    SetCurPoint(const POINT &pt)        {_ptCur = pt;}
    const   POINT&  GetCurPoint() const                 {return _ptCur;}

            HDC     GetDC() const                       {return _hdc;}

    BOOL    StartRender(const RECT &rcView,
                        const RECT &rcRender);

    VOID    NewLine (const CLine &li);
    VOID    RenderLine(CLine &li, long xRelOffset = 0);
    VOID    SkipCurLine(CLine * pli);
    BOOL    ShouldSkipThisRun(COneRun *por, LONG dupRun);
    
    CFormDrawInfo * GetDrawInfo() { return _pDI; }
};

inline
VOID CLSRenderer::SkipCurLine(CLine * pli)
{
    if(pli->_fForceNewLine)
    {
        _ptCur.y += pli->GetYHeight();
    }

    if (pli->_cch)
    {
        Advance(pli->_cch);
    }
}

inline CLSRenderer::ShouldSkipThisRun(COneRun *por, LONG dupRun)
{
    BOOL fRet = FALSE;
    if (   !_pLS->IsAdornment()
        && (   por->Cp() <  _cpStartRender
            || por->Cp() >= _cpStopRender
           )
       )
    {
        _xAccumulatedWidth += dupRun;
        fRet = TRUE;
    }
    return fRet;
}

#pragma INCMSG("--- End 'lsrender.hxx'")
#else
#pragma INCMSG("*** Dup 'lsrender.hxx'")
#endif
