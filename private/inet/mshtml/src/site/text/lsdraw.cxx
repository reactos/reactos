/*
 *  @doc    INTERNAL
 *
 *  @module LSDRAW.CXX -- line services drawing callbacks
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      12/29/97     cthrash created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

ExternTag(tagLSCallBack);

LSERR WINAPI
CLineServices::DrawUnderline(
    PLSRUN plsrun,          // IN
    UINT kUlBase,           // IN
    const POINT* pptStart,  // IN
    DWORD dupUl,            // IN
    DWORD dvpUl,            // IN
    LSTFLOW kTFlow,         // IN
    UINT kDisp,             // IN
    const RECT* prcClip)    // IN
{
    LSTRACE(DrawUnderline);

    GetRenderer()->DrawUnderline(plsrun, kUlBase, pptStart, dupUl, dvpUl, kTFlow, kDisp, prcClip);
    return lserrNone;
}

LSERR WINAPI
CLineServices::DrawStrikethrough(
    PLSRUN plsrun,          // IN
    UINT kStBase,           // IN
    const POINT* pptStart,  // IN
    DWORD dupSt,            // IN
    DWORD dvpSt,            // IN
    LSTFLOW kTFlow,         // IN
    UINT kDisp,             // IN
    const RECT* prcClip)    // IN
{
    LSTRACE(DrawStrikethrough);
    LSNOTIMPL(DrawStrikethrough);
    return lserrNone;
}

LSERR WINAPI
CLineServices::DrawBorder(
    PLSRUN plsrun,                              // IN
    const POINT* pptStart,                      // IN
    PCHEIGHTS pheightsLineFull,                 // IN
    PCHEIGHTS pheightsLineWithoutAddedSpace,    // IN
    PCHEIGHTS pheightsSubline,                  // IN
    PCHEIGHTS pheightRuns,                      // IN
    long dupBorder,                             // IN
    long dupRunsInclBorders,                    // IN
    LSTFLOW kTFlow,                             // IN
    UINT kDisp,                                 // IN
    const RECT* prcClip)                        // IN
{
    LSTRACE(DrawBorder);
    LSNOTIMPL(DrawBorder);
    return lserrNone;
}

LSERR WINAPI
CLineServices::DrawUnderlineAsText(
    PLSRUN plsrun,          // IN
    const POINT* pptStart,  // IN
    long dupLine,           // IN
    LSTFLOW kTFlow,         // IN
    UINT kDisp,             // IN
    const RECT* prcClip)    // IN
{
    LSTRACE(DrawUnderlineAsText);
    LSNOTIMPL(DrawUnderlineAsText);
    return lserrNone;
}

LSERR WINAPI
CLineServices::ShadeRectangle(
    PLSRUN plsrun,                              // IN
    const POINT* pptStart,                      // IN
    PCHEIGHTS pheightsLineWithAddSpace,         // IN
    PCHEIGHTS pheightsLineWithoutAddedSpace,    // IN
    PCHEIGHTS pheightsSubline,                  // IN
    PCHEIGHTS pheightsRunsExclTrail,            // IN
    PCHEIGHTS pheightsRunsInclTrail,            // IN
    long dupRunsExclTrail,                      // IN
    long dupRunsInclTrail,                      // IN
    LSTFLOW kTFlow,                             // IN
    UINT kDisp,                                 // IN
    const RECT* prcClip)                        // IN
{
    LSTRACE(ShadeRectangle);
    LSNOTIMPL(ShadeRectangle);
    return lserrNone;
}

LSERR WINAPI
CLineServices::DrawTextRun(
    PLSRUN plsrun,          // IN
    BOOL fStrikeout,        // IN
    BOOL fUnderline,        // IN
    const POINT* pptText,   // IN
    LPCWSTR plwchRun,       // IN
    const int* rgDupRun,    // IN
    DWORD cwchRun,          // IN
    LSTFLOW kTFlow,         // IN
    UINT kDisp,             // IN
    const POINT* pptRun,    // IN
    PCHEIGHTS heightPres,   // IN
    long dupRun,            // IN
    long dupLimUnderline,   // IN
    const RECT* pRectClip)  // IN
{
    LSTRACE(DrawTextRun);

#ifdef QUILL
    if (!_pFlowLayout->FExternalLayout())
#endif
    TCHAR *pch = (TCHAR *)plwchRun;
    if (plsrun->_fMakeItASpace)
    {
        Assert(cwchRun == 1);
        pch = _T(" ");
    }
    GetRenderer()->TextOut(plsrun,   fStrikeout, fUnderline, pptText,
                           pch,      rgDupRun,   cwchRun,    kTFlow,
                           kDisp,    pptRun,     heightPres, dupRun,
                           dupLimUnderline,      pRectClip
                          );

    return lserrNone;
}

LSERR WINAPI
CLineServices::DrawSplatLine(
    enum lsksplat,                              // IN
    LSCP cpSplat,                               // IN
    const POINT* pptSplatLine,                  // IN
    PCHEIGHTS pheightsLineFull,                 // IN
    PCHEIGHTS pheightsLineWithoutAddedSpace,    // IN
    PCHEIGHTS pheightsSubline,                  // IN
    long dup,                                   // IN
    LSTFLOW kTFlow,                             // IN
    UINT kDisp,                                 // IN
    const RECT* prcClip)                        // IN
{
    LSTRACE(DrawSplatLine);
    // FUTURE (mikejoch) Need to adjust cpSplat if we ever implement this.
    LSNOTIMPL(DrawSplatLine);
    return lserrNone;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLineServices::DrawGlyphs
//
//  Synopsis:   Draws the glyphs which are passed in
//
//  Arguments:  plsrun              pointer to the run
//              fStrikeout          is this run struck out?
//              fUnderline          is this run underlined?
//              pglyph              array of glyph indices
//              rgDu                array of widths after justification
//              rgDuBeforeJust      array of widths before justification
//              rgGoffset           array of glyph offsets
//              rgGprop             array of glyph properties
//              rgExpType           array of glyph expansion types
//              cglyph              number of glyph indices
//              kTFlow              text direction and orientation
//              kDisp               display mode - opaque, transparent
//              pptRun              starting point of the run
//              heights             presentation height for this run
//              dupRun              presentation width of this run
//              dupLimUnderline     underline limit
//              pRectClip           clipping rectangle
//
//  Returns:    LSERR               lserrNone if succesful
//                                  lserrInvalidRun if failure
//
//----------------------------------------------------------------------------
LSERR WINAPI
CLineServices::DrawGlyphs(
    PLSRUN plsrun,                  // IN
    BOOL fStrikeout,                // IN
    BOOL fUnderline,                // IN
    PCGINDEX pglyph,                // IN
    const int* rgDu,                // IN
    const int* rgDuBeforeJust,      // IN
    PGOFFSET rgGoffset,             // IN
    PGPROP rgGprop,                 // IN
    PCEXPTYPE rgExpType,            // IN
    DWORD cglyph,                   // IN
    LSTFLOW kTFlow,                 // IN
    UINT kDisp,                     // IN
    const POINT* pptRun,            // IN
    PCHEIGHTS heightsPres,          // IN
    long dupRun,                    // IN
    long dupLimUnderline,           // IN
    const RECT* pRectClip)          // IN
{
    LSTRACE(DrawGlyphs);

    GetRenderer()->GlyphOut(plsrun,    fStrikeout, fUnderline, pglyph,
                            rgDu,      rgDuBeforeJust,         rgGoffset,
                            rgGprop,   rgExpType,  cglyph,     kTFlow,
                            kDisp,     pptRun,     heightsPres,
                            dupRun,    dupLimUnderline,        pRectClip
                           );
    return lserrNone;
}

LSERR WINAPI
CLineServices::DrawEffects(
    PLSRUN plsrun,              // IN
    UINT EffectsFlags,          // IN
    const POINT* ppt,           // IN
    LPCWSTR lpwchRun,           // IN
    const int* rgDupRun,        // IN
    const int* rgDupLeftCut,    // IN
    DWORD cwchRun,              // IN
    LSTFLOW kTFlow,             // IN
    UINT kDisp,                 // IN
    PCHEIGHTS heightPres,       // IN
    long dupRun,                // IN
    long dupLimUnderline,       // IN
    const RECT* pRectClip)      // IN
{
    LSTRACE(DrawEffects);
    LSNOTIMPL(DrawEffects);
    return lserrNone;
}
