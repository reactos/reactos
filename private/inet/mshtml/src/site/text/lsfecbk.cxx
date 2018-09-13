/*
 *  ExternTag(@module) LSFECBK.CXX -- line services non-Latin object handlers
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      12/22/97     cthrash created
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

#ifndef X_RUBY_H_
#define X_RUBY_H_
#include <ruby.h>
#endif

#ifndef X_TATENAK_H_
#define X_TATENAK_H_
#include <tatenak.h>
#endif

#ifndef X_HIH_H_
#define X_HIH_H_
#include <hih.h>
#endif

#ifndef X_WARICHU_H_
#define X_WARICHU_H_
#include <warichu.h>
#endif

#ifndef X_LSENSUBL_H_
#define X_LSENSUBL_H_
#include <lsensubl.h>
#endif

ExternTag(tagLSCallBack);

//-----------------------------------------------------------------------------
//
// Ruby support
//
//-----------------------------------------------------------------------------

#define RUBY_OFFSET    2

LSERR WINAPI
CLineServices::FetchRubyPosition(
    LSCP lscp,                          // IN
    LSTFLOW lstflow,                    // IN
    DWORD cdwMainRuns,                  // IN
    const PLSRUN *pplsrunMain,          // IN
    PCHEIGHTS pcheightsRefMain,         // IN
    PCHEIGHTS pcheightsPresMain,        // IN
    DWORD cdwRubyRuns,                  // IN
    const PLSRUN *pplsrunRuby,          // IN
    PCHEIGHTS pcheightsRefRuby,         // IN
    PCHEIGHTS pcheightsPresRuby,        // IN
    PHEIGHTS pheightsRefRubyObj,        // OUT
    PHEIGHTS pheightsPresRubyObj,       // OUT
    long *pdvrOffsetMainBaseline,       // OUT
    long *pdvrOffsetRubyBaseline,       // OUT
    long *pdvpOffsetRubyBaseline,       // OUT
    enum rubycharjust *prubycharjust,   // OUT
    BOOL *pfSpecialLineStartEnd)        // OUT
{
    LSTRACE(FetchRubyPosition);
    LONG yAscent;
    RubyInfo rubyInfo;
    long yRubyOffset = (cdwRubyRuns > 0) ? RUBY_OFFSET : 0;
    
    *pdvrOffsetMainBaseline = 0;  // Don't want to offset the main text from
                                  // the ruby object's baseline

    if(cdwMainRuns)
    {
        HandleRubyAlignStyle((COneRun *)(*pplsrunMain), prubycharjust, pfSpecialLineStartEnd);
    }

    rubyInfo.cp = CPFromLSCP(lscp);

    // Ruby offset is the sum of the ascent of the main text
    // and the descent of the pronunciation text.
    yAscent = max(_yMaxHeightForRubyBase, pcheightsRefMain->dvAscent);
    
    *pdvpOffsetRubyBaseline = 
    *pdvrOffsetRubyBaseline = 
         yAscent + yRubyOffset + pcheightsRefRuby->dvDescent;
    rubyInfo.yHeightRubyBase = yAscent + pcheightsRefMain->dvDescent + yRubyOffset;
    rubyInfo.yDescentRubyBase = pcheightsRefMain->dvDescent;
    rubyInfo.yDescentRubyText = pcheightsRefRuby->dvDescent;

    pheightsRefRubyObj->dvAscent = yAscent + pcheightsRefRuby->dvAscent + pcheightsRefRuby->dvDescent + yRubyOffset;
    pheightsRefRubyObj->dvDescent = pcheightsRefMain->dvDescent;
    pheightsRefRubyObj->dvMultiLineHeight = 
        pheightsRefRubyObj->dvAscent + pheightsRefRubyObj->dvDescent;
    memcpy(pheightsPresRubyObj, pheightsRefRubyObj, sizeof(*pheightsRefRubyObj));

    
    // code in GetRubyInfoFromCp depends on the idea that this callback 
    // is called in order of increasing cps. This of course depends on Line Services.  
    // If this is not guaranteed to be true, then we can't just blindly append the 
    // entry here, we must insert it in sorted order or hold cp ranges in the RubyInfos
    Assert(_aryRubyInfo.Size() == 0
           || _aryRubyInfo[_aryRubyInfo.Size()-1].cp <= rubyInfo.cp);
    if(_aryRubyInfo.FindIndirect(&rubyInfo) == -1)
    {
        _aryRubyInfo.AppendIndirect(&rubyInfo);
    }

    return lserrNone;
}


// Ruby Align Style table
// =========================
// Holds the justification values to pass to line services for each ruby alignment type.
//

static const enum rubycharjust s_aRubyAlignStyleValues[] =
{
    rcjCenter,   // not set
    rcjCenter,   // auto
    rcjLeft,     // left
    rcjCenter,   // center
    rcjRight,    // right
    rcj010,      // distribute-letter
    rcj121,      // distribute-space
    rcjCenter    // line-edge
};

void WINAPI
CLineServices::HandleRubyAlignStyle(
    COneRun *porMain,                   // IN
    enum rubycharjust *prubycharjust,   // OUT
    BOOL *pfSpecialLineStartEnd)        // OUT
{
    Assert(porMain);
    
    CTreeNode *     pNode = porMain->Branch();  // This will call_ptp->GetBranch()
    CElement *      pElement = pNode->SafeElement();
    VARIANT varRubyAlign;
    styleRubyAlign styAlign;

    pElement->ComputeExtraFormat(DISPID_A_RUBYALIGN, TRUE, pNode, &varRubyAlign);
    styAlign = (((CVariant *)&varRubyAlign)->IsEmpty()) 
                                                 ? styleRubyAlignNotSet 
                                                 : (styleRubyAlign) V_I4(&varRubyAlign);

    Assert(styAlign >= styleRubyAlignNotSet && styAlign <= styleRubyAlignLineEdge);

    *prubycharjust = s_aRubyAlignStyleValues[styAlign];
    *pfSpecialLineStartEnd = (styAlign == styleRubyAlignLineEdge);

    if(styAlign == styleRubyAlignNotSet || styAlign == styleRubyAlignAuto) 
    {
        // default behavior should be centered alignment for latin characters,
        // distribute-space for ideographic characters
        const SCRIPT_ID sid = porMain->_ptp->Sid();
        if(sid >= sidFEFirst && sid <= sidFELast)
        {
            *prubycharjust = rcj121;
        }
    }
}


LSERR WINAPI
CLineServices::FetchRubyWidthAdjust(
    LSCP cp,                // IN
    PLSRUN plsrunForChar,   // IN
    WCHAR wch,              // IN
    MWCLS mwclsForChar,     // IN
    PLSRUN plsrunForRuby,   // IN
    enum rubycharloc rcl,   // IN
    long durMaxOverhang,    // IN
    long *pdurAdjustChar,   // OUT
    long *pdurAdjustRuby)   // OUT
{
    LSTRACE(FetchRubyWidthAdjust);
    Assert(plsrunForRuby);

    COneRun *       porRuby = (COneRun *)plsrunForRuby;
    CTreeNode *     pNode = porRuby->Branch();  // This will call_ptp->GetBranch()
    CElement *      pElement = pNode->SafeElement();
    styleRubyOverhang sty;

    {
        VARIANT varValue;

        pElement->ComputeExtraFormat(DISPID_A_RUBYOVERHANG, 
                                     TRUE, 
                                     pNode, 
                                     &varValue);

        sty = (((CVariant *)&varValue)->IsEmpty())
                 ? styleRubyOverhangNotSet
                 : (styleRubyOverhang)V_I4(&varValue);
    }
  
	*pdurAdjustChar = 0;
	*pdurAdjustRuby = (sty == styleRubyOverhangNone) ? 0 : -durMaxOverhang;
    return lserrNone;
}

LSERR WINAPI
CLineServices::RubyEnum(
    PLSRUN plsrun,              // IN
    PCLSCHP plschp,             // IN
    LSCP cp,                    // IN
    LSDCP dcp,                  // IN
    LSTFLOW lstflow,            // IN
    BOOL fReverse,              // IN
    BOOL fGeometryNeeded,       // IN
    const POINT* pt,            // IN
    PCHEIGHTS pcheights,        // IN
    long dupRun,                // IN
    const POINT *ptMain,        // IN
    PCHEIGHTS pcheightsMain,    // IN
    long dupMain,               // IN
    const POINT *ptRuby,        // IN
    PCHEIGHTS pcheightsRuby,    // IN
    long dupRuby,               // IN
    PLSSUBL plssublMain,        // IN
    PLSSUBL plssublRuby)        // IN
{
    LSTRACE(RubyEnum);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
// Tatenakayoko (HIV) support
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetTatenakayokoLinePosition(
    LSCP cp,                            // IN
    LSTFLOW lstflow,                    // IN
    PLSRUN plsrun,                      // IN
    long dvr,                           // IN
    PHEIGHTS pheightsRef,               // OUT
    PHEIGHTS pheightsPres,              // OUT
    long *pdvrDescentReservedForClient) // OUT
{
    LSTRACE(GetTatenakayokoLinePosition);
    // FUTURE (mikejoch) Need to adjust cp if we ever implement this.
    LSNOTIMPL(GetTatenakayokoLinePosition);
    return lserrNone;
}

LSERR WINAPI
CLineServices::TatenakayokoEnum(
    PLSRUN plsrun,          // IN
    PCLSCHP plschp,         // IN
    LSCP cp,                // IN
    LSDCP dcp,              // IN
    LSTFLOW lstflow,        // IN
    BOOL fReverse,          // IN
    BOOL fGeometryNeeded,   // IN
    const POINT* pt,        // IN
    PCHEIGHTS pcheights,    // IN
    long dupRun,            // IN
    LSTFLOW lstflowT,       // IN
    PLSSUBL plssubl)        // IN
{
    LSTRACE(TatenakayokoEnum);
    LSNOTIMPL(TatenakayokoEnum);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
// Warichu support
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetWarichuInfo(
    LSCP cp,                            // IN
    LSTFLOW lstflow,                    // IN
    PCOBJDIM pcobjdimFirst,             // IN
    PCOBJDIM pcobjdimSecond,            // IN
    PHEIGHTS pheightsRef,               // OUT
    PHEIGHTS pheightsPres,              // OUT
    long *pdvrDescentReservedForClient) // OUT
{
    LSTRACE(GetWarichuInfo);
    // FUTURE (mikejoch) Need to adjust cp if we ever implement this.
    LSNOTIMPL(GetWarichuInfo);
    return lserrNone;
}

LSERR WINAPI
CLineServices::FetchWarichuWidthAdjust(
    LSCP cp,                        // IN
    enum warichucharloc wcl,        // IN
    PLSRUN plsrunForChar,           // IN
    WCHAR wch,                      // IN
    MWCLS mwclsForChar,             // IN
    PLSRUN plsrunWarichuBracket,    // IN
    long *pdurAdjustChar,           // OUT
    long *pdurAdjustBracket)        // OUT
{
    LSTRACE(FetchWarichuWidthAdjust);
    // FUTURE (mikejoch) Need to adjust cp if we ever implement this.
    LSNOTIMPL(FetchWarichuWidthAdjust);
    return lserrNone;
}

LSERR WINAPI
CLineServices::WarichuEnum(
    PLSRUN plsrun,                  // IN: plsrun for the entire Warichu Object
    PCLSCHP plschp,                 // IN: lschp for lead character of Warichu Object
    LSCP cp,                        // IN: cp of first character of Warichu Object
    LSDCP dcp,                      // IN: number of characters in Warichu Object
    LSTFLOW lstflow,                // IN: text flow at Warichu Object
    BOOL fReverse,                  // IN: whether text should be reversed for visual order
    BOOL fGeometryNeeded,           // IN: whether Geometry should be returned
    const POINT* pt,                // IN: starting position, iff fGeometryNeeded
    PCHEIGHTS pcheights,            // IN: height of Warichu object, iff fGeometryNeeded
    long dupRun,                    // IN: length of Warichu Object, iff fGeometryNeeded
    const POINT *ptLeadBracket,     // IN: point for second line iff fGeometryNeeded and plssublSecond not NULL
    PCHEIGHTS pcheightsLeadBracket, // IN: height for ruby line iff fGeometryNeeded 
    long dupLeadBracket,            // IN: length of Ruby line iff fGeometryNeeded and plssublSecond not NULL
    const POINT *ptTrailBracket,    // IN: point for second line iff fGeometryNeeded and plssublSecond not NULL
    PCHEIGHTS pcheightsTrailBracket,// IN: height for ruby line iff fGeometryNeeded 
    long dupTrailBracket,           // IN: length of Ruby line iff fGeometryNeeded and plssublSecond not NULL
    const POINT *ptFirst,           // IN: starting point for main line iff fGeometryNeeded
    PCHEIGHTS pcheightsFirst,       // IN: height of main line iff fGeometryNeeded
    long dupFirst,                  // IN: length of main line iff fGeometryNeeded 
    const POINT *ptSecond,          // IN: point for second line iff fGeometryNeeded and plssublSecond not NULL
    PCHEIGHTS pcheightsSecond,      // IN: height for ruby line iff fGeometryNeeded and plssublSecond not NULL
    long dupSecond,                 // IN: length of Ruby line iff fGeometryNeeded and plssublSecond not NULL
    PLSSUBL plssublLeadBracket,     // IN: subline for lead bracket
    PLSSUBL plssublTrailBracket,    // IN: subline for trail bracket
    PLSSUBL plssublFirst,           // IN: first subline in Warichu object
    PLSSUBL plssublSecond)          // IN: second subline in Warichu object
{
    LSTRACE(WarichuEnum);
    LSNOTIMPL(WarichuEnum);
    return lserrNone;
}


//-----------------------------------------------------------------------------
//
// HIH support
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::HihEnum(
    PLSRUN plsrun,          // IN
    PCLSCHP plschp,         // IN
    LSCP cp,                // IN
    LSDCP dcp,              // IN
    LSTFLOW lstflow,        // IN
    BOOL fReverse,          // IN
    BOOL fGeometryNeeded,   // IN
    const POINT* pt,        // IN
    PCHEIGHTS pcheights,    // IN
    long dupRun,            // IN
    PLSSUBL plssubl)        // IN
{
    LSTRACE(HihEnum);
    LSNOTIMPL(HihEnum);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
// Reverse Object support
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::ReverseEnum(
    PLSRUN plsrun,          // IN
    PCLSCHP plschp,         // IN
    LSCP cp,                // IN
    LSDCP dcp,              // IN
    LSTFLOW lstflow,        // IN
    BOOL fReverse,          // IN
    BOOL fGeometryNeeded,   // IN
    const POINT* ppt,       // IN
    PCHEIGHTS pcheights,    // IN
    long dupRun,            // IN
    LSTFLOW lstflowSubline, // IN
    PLSSUBL plssubl)        // IN
{
    LSTRACE(ReverseEnum);

    return LsEnumSubline(plssubl, fReverse, fGeometryNeeded, ppt);
}
