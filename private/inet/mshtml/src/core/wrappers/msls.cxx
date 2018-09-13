//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       msls.cxx
//
//  Contents:   Dynamic wrappers for Line Services procedures.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef X_LSDEFS_H_
#define X_LSDEFS_H_
#include "lsdefs.h"
#endif

#ifndef X_LSFRUN_H_
#define X_LSFRUN_H_
#include "lsfrun.h"
#endif

#ifndef X_FMTRES_H_
#define X_FMTRES_H_
#include "fmtres.h"
#endif

#ifndef X_PLSDNODE_H_
#define X_PLSDNODE_H_
#include "plsdnode.h"
#endif

#ifndef X_PLSSUBL_H_
#define X_PLSSUBL_H_
#include "plssubl.h"
#endif

#ifndef X_LSKJUST_H_
#define X_LSKJUST_H_
#include "lskjust.h"
#endif

#ifndef X_LSCONTXT_H_
#define X_LSCONTXT_H_
#include "lscontxt.h"
#endif

#ifndef X_LSLINFO_H_
#define X_LSLINFO_H_
#include "lslinfo.h"
#endif

#ifndef X_PLSLINE_H_
#define X_PLSLINE_H_
#include "plsline.h"
#endif

#ifndef X_PLSSUBL_H_
#define X_PLSSUBL_H_
#include "plssubl.h"
#endif

#ifndef X_PDOBJ_H_
#define X_PDOBJ_H_
#include "pdobj.h"
#endif

#ifndef X_PHEIGHTS_H_
#define X_PHEIGHTS_H_
#include "pheights.h"
#endif

#ifndef X_PLSRUN_H_
#define X_PLSRUN_H_
#include "plsrun.h"
#endif

#ifndef X_LSESC_H_
#define X_LSESC_H_
#include "lsesc.h"
#endif

#ifndef X_POBJDIM_H_
#define X_POBJDIM_H_
#include "pobjdim.h"
#endif

#ifndef X_LSPRACT_H_
#define X_LSPRACT_H_
#include "lspract.h"
#endif

#ifndef X_LSBRK_H_
#define X_LSBRK_H_
#include "lsbrk.h"
#endif

#ifndef X_LSDEVRES_H_
#define X_LSDEVRES_H_
#include "lsdevres.h"
#endif

#ifndef X_LSEXPAN_H_
#define X_LSEXPAN_H_
#include "lsexpan.h"
#endif

#ifndef X_LSPAIRAC_H_
#define X_LSPAIRAC_H_
#include "lspairac.h"
#endif

#ifndef X_LSKTAB_H_
#define X_LSKTAB_H_
#include "lsktab.h"
#endif

#ifndef X_LSKEOP_H_
#define X_LSKEOP_H_
#include "lskeop.h"
#endif

#ifndef X_PLSQSINF_H_
#define X_PLSQSINF_H_
#include "plsqsinf.h"
#endif

#ifndef X_PCELLDET_H_
#define X_PCELLDET_H_
#include "pcelldet.h"
#endif

#ifndef X_PLSCELL_H_
#define X_PLSCELL_H_
#include "plscell.h"
#endif

#ifndef X_BRKPOS_H_
#define X_BRKPOS_H_
#include "brkpos.h"
#endif

DYNLIB g_dynlibMSLS = { NULL, NULL, "MSLS31.DLL" };

#define WRAPIT(fn, a1, a2)\
LSERR WINAPI fn a1\
{\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibMSLS, #fn };\
    if(THR(LoadProcedure(&s_dynproc##fn)))\
        return lserrOutOfMemory;\
    return (*(LSERR (WINAPI *) a1)s_dynproc##fn.pfn) a2;\
}

WRAPIT( LsAppendRunToCurrentSubline,
        (PLSC plsc,              /* IN: LS context               */
         const LSFRUN* plsfrun,  /* IN: given run                */
         BOOL* pfSuccessful,     /* OUT: Need to refetch?        */
         FMTRES* pfmtres,        /* OUT: result of last formatter*/
         LSCP* pcpLim,           /* OUT: cpLim                   */
         PLSDNODE* pplsdn),      /* OUT: DNODE created           */
        (plsc, plsfrun, pfSuccessful, pfmtres, pcpLim, pplsdn) )

WRAPIT( LsCompressSubline,
        (PLSSUBL plssubl,       /* IN: subline context      */
         LSKJUST lskjust,       /* IN: justification type   */
         long dup),             /* IN: dup                  */
        (plssubl, lskjust, dup) )

WRAPIT( LsCreateContext,
        (const LSCONTEXTINFO* plsci, PLSC* pplsc),
        (plsci, pplsc) )

WRAPIT( LsCreateLine,
        (PLSC plsc,            
         LSCP cpFirst,         
         long duaColumn,           
         const BREAKREC* rgBreakRecordIn,
         DWORD nBreakRecordIn,
         DWORD nBreakRecordOut,
         BREAKREC* rgBreakRecordOut,
         DWORD* pnActualBreakRecord,
         LSLINFO* plslinfo,        
         PLSLINE* pplsline),
        (plsc, cpFirst, duaColumn, rgBreakRecordIn, nBreakRecordIn, nBreakRecordOut, rgBreakRecordOut, pnActualBreakRecord, plslinfo, pplsline) )

WRAPIT( LsCreateSubline,
        (PLSC plsc,                  /* IN: LS context               */
         LSCP cpFirst,               /* IN: cpFirst                  */
         long urColumnMax,           /* IN: urColumnMax              */
         LSTFLOW lstflow,            /* IN: text flow                */
         BOOL fContiguos),           /* IN: fContiguous              */
        (plsc, cpFirst, urColumnMax, lstflow, fContiguos) )

WRAPIT( LsDestroyContext,
        (PLSC plsc),
        (plsc) )

WRAPIT( LsDestroyLine,
        (PLSC plsc,        /* IN: ptr to line services context */
         PLSLINE plsline), /* IN: ptr to line -- opaque to client */
        (plsc, plsline) )

WRAPIT( LsDestroySubline,
        (PLSSUBL plssubl),
        (plssubl) )

WRAPIT( LsDisplayLine,
        (PLSLINE plsline, const POINT* pptorg, UINT kdispmode, const RECT *prectClip),
        (plsline, pptorg, kdispmode, prectClip) )

WRAPIT( LsDisplaySubline,
        (PLSSUBL plssubl, const POINT* pptorg, UINT kdispmode,
         const RECT *prectClip),
        (plssubl, pptorg, kdispmode, prectClip) )

WRAPIT( LsEnumLine,
        ( PLSLINE plsline,
          BOOL fReversedOrder,      /* IN: enumerate in reverse order?                  */
          BOOL fGeometryProvided,   /* IN: geometry needed?                             */
          const POINT* pptStart),   /* IN: starting position(xp, yp) iff fGeometryNeeded*/
        (plsline, fReversedOrder, fGeometryProvided, pptStart) )

WRAPIT( LsEnumSubline,
        ( PLSSUBL plssubl,
          BOOL fReversedOrder,      /* IN: enumerate in reverse order?                  */
          BOOL fGeometryProvided,   /* IN: geometry needed?                             */
          const POINT* pptStart),   /* IN: starting position(xp, yp) iff fGeometryNeeded*/
        (plssubl, fReversedOrder, fGeometryProvided, pptStart) )

WRAPIT( LsExpandSubline,
        (PLSSUBL plssubl,      /* IN: subline context      */
         LSKJUST lskjust,      /* IN: justification type   */
         long dup),            /* IN: dup                  */
        (plssubl, lskjust, dup) )

WRAPIT( LsFetchAppendToCurrentSubline,
        (PLSC plsc,              /* IN: LS context               */
         LSDCP lsdcp,            /* IN:Increse cp before fetching*/
         const LSESC* plsesc,    /* IN: escape characters        */
         DWORD cEsc,             /* IN: # of escape characters   */
         BOOL * pfSuccessful,    /* OUT: Need to refetch?        */
         FMTRES* pfmtres,        /* OUT: result of last formatter*/
         LSCP* pcpLim,           /* OUT: cpLim                   */
         PLSDNODE* pplsdnFirst,  /* OUT: First DNODE created     */
         PLSDNODE* pplsdnLast),  /* OUT: Last DNODE created      */
        (plsc, lsdcp, plsesc, cEsc, pfSuccessful, pfmtres, pcpLim, pplsdnFirst, pplsdnLast) )

WRAPIT( LsFetchAppendToCurrentSublineResume,
        (PLSC plsc,                         /* IN: LS context                       */
         const BREAKREC* rgBreakRecord,     /* IN: array of break records           */
         DWORD nBreakRecord,                /* IN: number of records in array       */
         LSDCP lsdcp,                       /* IN:Increase cp before fetching       */
         const LSESC* plsesc,               /* IN: escape characters                */
         DWORD cEsc,                        /* IN: # of escape characters           */
         BOOL * pfSuccessful,               /* OUT: Need to refetch?        */
         FMTRES* pfmtres,                   /* OUT: result of last formatter        */
         LSCP* pcpLim,                      /* OUT: cpLim                           */
         PLSDNODE* pplsdnFirst,             /* OUT: First DNODE created             */
         PLSDNODE* pplsdnLast),             /* OUT: Last DNODE created              */
        (plsc, rgBreakRecord, nBreakRecord, lsdcp, plsesc, cEsc, pfSuccessful, pfmtres, pcpLim, pplsdnFirst, pplsdnLast) )

WRAPIT( LsFindNextBreakSubline,
        (PLSSUBL plssubl,            /* IN: subline context          */
         LSCP cpTruncate,            /* IN: truncation cp            */
         long urColumnMax,           /* IN: urColumnMax              */
         BOOL* pfSuccessful,         /* OUT: fSuccessful?            */
         LSCP* pcpBreak,             /* OUT: cpBreak                 */
         POBJDIM pobjdimDnode,       /* OUT: objdimSub up to break   */
         BRKPOS* pbrkpos ),          /* OUT: Before/Inside/After     */
        (plssubl, cpTruncate, urColumnMax, pfSuccessful, pcpBreak, pobjdimDnode, pbrkpos ) )

WRAPIT( LsFindPrevBreakSubline,
        (PLSSUBL plssubl,            /* IN: subline context          */
         LSCP cpTruncate,            /* IN: truncation cp            */
         long urColumnMax,           /* IN: urColumnMax              */
         BOOL* pfSuccessful,         /* OUT: fSuccessful?            */
         LSCP* pcpBreak,             /* OUT: cpBreak                 */
         POBJDIM pobjdimDnode,       /* OUT: objdimSub up to break   */
         BRKPOS* pbrkpos ),          /* OUT: Before/Inside/After     */
        (plssubl, cpTruncate, urColumnMax, pfSuccessful, pcpBreak, pobjdimDnode, pbrkpos ) )

WRAPIT( LsFinishCurrentSubline,
        (PLSC plsc,                  /* IN: LS context               */
         PLSSUBL* pplssubl),         /* OUT: subline context         */
        (plsc, pplssubl) )

WRAPIT( LsForceBreakSubline,
        (PLSSUBL plssubl,            /* IN: subline context          */
         LSCP cpTruncate,            /* IN: truncation cp            */
         long urColumnMax,           /* IN: urColumnMax              */
         LSCP* pcpBreak,             /* OUT: cpBreak                 */
         POBJDIM pobjdimDnode,       /* OUT: objdimSub up to break   */
         BRKPOS* pbrkpos ),          /* OUT: Before/Inside/After     */
        (plssubl, cpTruncate, urColumnMax, pcpBreak, pobjdimDnode, pbrkpos) )

WRAPIT( LsGetHihLsimethods,
        (LSIMETHODS *plsim),
        (plsim) )

WRAPIT( LsGetReverseLsimethods,
        (LSIMETHODS *plsim),
        (plsim) )

WRAPIT( LsGetRubyLsimethods,
        (LSIMETHODS *plsim),
        (plsim) )

WRAPIT( LsGetSpecialEffectsSubline,
        (PLSSUBL plssubl,            /* IN: subline context      */
         UINT* pfSpecialEffects),    /* IN: special effects      */
        (plssubl, pfSpecialEffects) )

WRAPIT( LsGetTatenakayokoLsimethods,
        (LSIMETHODS *plsim),
        (plsim) )

WRAPIT( LsGetWarichuLsimethods,
        (LSIMETHODS *plsim),         /* (OUT): Warichu object callbacks. */
        (plsim) )

WRAPIT( LsMatchPresSubline,
        (PLSSUBL plssubl),           /* IN: subline context      */
        (plssubl) )

WRAPIT( LsModifyLineHeight,
        (PLSC plsc,
         PLSLINE plsline,
         long dvpAbove,
         long dvpAscent,
         long dvpDescent,
         long dvpBelow),
        (plsc, plsline, dvpAbove, dvpAscent, dvpDescent, dvpBelow) )

WRAPIT( LsPointUV2FromPointUV1,
        (LSTFLOW lstflow1,         /* IN: text flow 1 */
         PCPOINTUV pptStart,       /* IN: start input point (TF1) */
         PCPOINTUV pptEnd,         /* IN: end input point (TF1) */
         LSTFLOW lstflow2,         /* IN: text flow 2 */
         PPOINTUV pptOut),         /* OUT: vector in TF2 */
        (lstflow1, pptStart, pptEnd, lstflow2, pptOut) )

WRAPIT( LsPointXYFromPointUV,
        (const POINT* pptXY,         /* IN: input point (x,y) */
         LSTFLOW lstflow,            /* IN: text flow for */
         PCPOINTUV pptUV,            /* IN: vector in (u,v) */
         POINT* pptXYOut),           /* OUT: point (x,y) */
        (pptXY, lstflow, pptUV, pptXYOut) )

WRAPIT( LsQueryCpPpointSubline,
        (PLSSUBL plssubl,               /* IN: pointer to line info -- opaque to client */
         LSCP cpQ,                      /* IN: cpQuery                                  */
         DWORD nDepthQueryMax,          /* IN: nDepthQueryMax                           */
         PLSQSUBINFO rglsqsubinfo,      /* OUT: array[nDepthQueryMax] of LSQSUBINFO     */
         DWORD* pnActualDepth,          /* OUT: nActualDepth                            */
         PLSTEXTCELL pcell),            /* OUT: Text cell info                          */
        (plssubl, cpQ, nDepthQueryMax, rglsqsubinfo, pnActualDepth, pcell) )

WRAPIT( LsQueryFLineEmpty,
        (PLSLINE plsline,      /* IN: pointer to line -- opaque to client */
         BOOL* pfEmpty),       /* OUT: Is line empty? */
        (plsline, pfEmpty) )

WRAPIT( LsQueryLineCpPpoint,
        (PLSLINE plsline,               /* IN: pointer to line info -- opaque to client */
         LSCP cp,                       /* IN: cpQuery                                  */
         DWORD nDepthQueryMax,          /* IN: nDepthQueryMax                           */
         PLSQSUBINFO rglsqsubinfo,      /* OUT: array[nDepthQueryMax] of LSQSUBINFO     */
         DWORD* pnActualDepth,          /* OUT: nActualDepth                            */
         PLSTEXTCELL pcell),            /* OUT: Text cell info                          */
        (plsline, cp, nDepthQueryMax, rglsqsubinfo, pnActualDepth, pcell) )

WRAPIT( LsQueryLineDup,
        (PLSLINE plsline,                   /* IN: pointer to line -- opaque to client  */
         long* pupStartAutonumberingText,   /* OUT: upStartAutonumberingText            */
         long* pupLimAutonumberingText,     /* OUT: upStartAutonumberingText            */
         long* pupStartMainText,            /* OUT: upStartMainText                     */
         long* pupStartTrailing,            /* OUT: upStartTrailing                     */
         long* pupLimLine),                 /* OUT: upLimLine                           */
        (plsline, pupStartAutonumberingText, pupLimAutonumberingText, pupStartMainText, pupStartTrailing, pupLimLine) )

WRAPIT( LsQueryLinePointPcp,
        (PLSLINE plsline,               /* IN: pointer to line -- opaque to client          */
         PCPOINTUV pptuv,               /* IN: query point (uQuery,vQuery) (line text flow) */
         DWORD nDepthQueryMax,          /* IN: nDepthQueryMax                               */
         PLSQSUBINFO rglsqsubinfo,      /* OUT: array[nDepthQueryMax] of LSQSUBINFO         */
         DWORD* pnActualDepth,          /* OUT: nActualDepth                                */
         PLSTEXTCELL pcell),            /* OUT: Text cell info                              */
        (plsline, pptuv, nDepthQueryMax, rglsqsubinfo, pnActualDepth, pcell) )

WRAPIT( LsQueryPointPcpSubline,
        (PLSSUBL plssubl,               /* IN: pointer to line info -- opaque to client     */
         PCPOINTUV ppointuv,            /* IN: query point (uQuery,vQuery) (line text flow) */
         DWORD nDepthQueryMax,          /* IN: nDepthQueryMax                               */
         PLSQSUBINFO rglsqsubinfo,      /* OUT: array[nDepthQueryMax] of LSQSUBINFO         */
         DWORD* pnActualDepth,          /* OUT: nActualDepth                                */
         PLSTEXTCELL pcell),            /* OUT: Text cell info                              */
        (plssubl, ppointuv, nDepthQueryMax, rglsqsubinfo, pnActualDepth, pcell) )

WRAPIT( LsQueryTextCellDetails,
        (PLSLINE plsline,               /* IN: pointer to line -- opaque to client                  */
         PCELLDETAILS pCellDetails,     /* IN: query point (uQuery,vQuery) (line text flow)         */
         LSCP cpStartCell,              /* IN: cpStartCell                                          */
         DWORD nCharsInContext,         /* IN: nCharsInContext                                      */
         DWORD nGlyphsInContext,        /* IN: nGlyphsInContext                                     */
         WCHAR* rgwch,                  /* OUT: pointer array[nCharsInContext] of char codes        */
         PGINDEX rgGindex,              /* OUT: pointer array[nGlyphsInContext] of glyph indices    */
         long* rgDu,                    /* OUT: pointer array[nGlyphsInContext] of glyph widths     */
         PGOFFSET rgGoffset,            /* OUT: pointer array[nGlyphsInContext] of glyph offsets    */
         PGPROP rgGprop),               /* OUT: pointer array[nGlyphsInContext] of glyph handles    */
        (plsline, pCellDetails, cpStartCell, nCharsInContext, nGlyphsInContext, rgwch, rgGindex, rgDu, rgGoffset, rgGprop) )

WRAPIT( LsResetRMInCurrentSubline,
        (PLSC plsc,                  /* IN: LS context               */
         long urColumnMax),          /* IN: urColumnMax              */
        (plsc, urColumnMax) )

WRAPIT( LsSetBreakSubline,
        (PLSSUBL plssubl,               /* IN: subline context                        */
         POBJDIM pobjdimDnode,          /* IN: objdimBreak of DNODE                   */
         LSCP cpBreak,                  /* IN: cpBreak                                */
         BREAKREC* rgBreakRecord,       /* IN/OUT: array of break records             */
         DWORD nBreakRecord,            /* IN: size of array                          */
         DWORD* pnActualBreakRecord),   /* OUT: number of used elements of the array  */
        (plssubl, pobjdimDnode, cpBreak, rgBreakRecord, nBreakRecord, pnActualBreakRecord) )

WRAPIT( LsSetBreaking,
        (PLSC plsc,                /* IN: ptr to line services context */
         DWORD clsbrk,             /* IN: Number of breaking info units*/
         const LSBRK* rglsbrk,     /* IN: Breaking info units array    */
         DWORD cBreakingClasses,   /* IN: Number of breaking classes   */
         const BYTE* rgilsbrk),    /* IN: Breaking information(square): indexes in the LSBRK array  */
        (plsc, clsbrk, rglsbrk, cBreakingClasses, rgilsbrk) )

WRAPIT( LsSetCompression,
        (PLSC plsc,                /* IN: ptr to line services context */
         DWORD cPriorities,        /* IN: Number of compression priorities*/
         DWORD clspract,           /* IN: Number of compression info units*/
         const LSPRACT* rglspract, /* IN: Compession info units array  */
         DWORD cModWidthClasses,   /* IN: Number of Mod Width classes  */
         const BYTE* rgilspract),  /* IN: Compression information: indexes in the LSPRACT array  */
        (plsc, cPriorities, clspract, rglspract, cModWidthClasses, rgilspract) )

WRAPIT( LsSetDoc,
        (PLSC plsc,
         BOOL fDisplay,
         BOOL fPresEqualRef,
         const LSDEVRES* pclsdevres),
        (plsc, fDisplay, fPresEqualRef, pclsdevres) )

WRAPIT( LsSetExpansion,
        (PLSC plsc,                /* IN: ptr to line services context */
         DWORD cExpansionClasses,  /* IN: Number of expansion info units*/
         const LSEXPAN* rglsexpan, /* IN: Expansion info units array   */
         DWORD cModWidthClasses,   /* IN: Number of Mod Width classes  */
         const BYTE* rgilsexpan),  /* IN: Expansion information(square): indexes in the LSEXPAN array  */
        (plsc, cExpansionClasses, rglsexpan, cModWidthClasses, rgilsexpan) )

WRAPIT( LsSetModWidthPairs,
        (PLSC  plsc,                   /* IN: ptr to line services context */
         DWORD clspairact,             /* IN: Number of mod pairs info units*/
         const LSPAIRACT* rglspairact, /* IN: Mod pairs info units array  */
         DWORD cModWidthClasses,       /* IN: Number of Mod Width classes  */
         const BYTE* rgilspairact),    /* IN: Mod width information(square): indexes in the LSPAIRACT array */
        (plsc, clspairact, rglspairact, cModWidthClasses, rgilspairact) )

WRAPIT( LsSqueezeSubline,
        (PLSSUBL plssubl,      /* IN: subline context      */
         long durTarget,       /* IN: durTarget            */
         BOOL* pfSuccessful,   /* OUT: fSuccessful?        */
         long* pdurExtra),     /* OUT: if nof successful, extra dur */
        (plssubl, durTarget, pfSuccessful, pdurExtra) )
        

WRAPIT( LsTruncateSubline,
        (PLSSUBL plssubl,        /* IN: subline context          */
         long urColumnMax,       /* IN: urColumnMax              */
         LSCP* pcpTruncate),     /* OUT: cpTruncate              */
        (plssubl, urColumnMax, pcpTruncate) )

WRAPIT( LsdnDistribute,
        (PLSC plsc,                  /* IN: Pointer to LS Context */
         PLSDNODE plsdnFirst,        /* IN: First DNODE          */
         PLSDNODE plsdnLast,         /* IN: Last DNODE           */
         long durToDistribute),      /* IN: durToDistribute      */
        (plsc, plsdnFirst, plsdnLast, durToDistribute) )

WRAPIT( LsdnFinishByPen,
        (PLSC plsc,               /* IN: Pointer to LS Context */
         LSDCP lsdcp,             /* IN: dcp  adopted          */
         PLSRUN plsrun,           /* IN: PLSRUN                */
         PDOBJ pdobj,             /* IN: PDOBJ                 */
         long durPen,             /* IN: dur                   */
         long dvrPen,             /* IN: dvr                   */
         long dvpPen),            /* IN: dvp                   */
        (plsc, lsdcp, plsrun, pdobj, durPen, dvrPen, dvpPen) )

WRAPIT( LsdnFinishDeleteAll,
        (PLSC plsc,        /* IN: Pointer to LS Context */
         LSDCP lsdcp),     /* IN: dcp adopted           */
        (plsc, lsdcp) )

WRAPIT( LsdnFinishRegular,
        (PLSC  plsc,
         LSDCP lsdcp,
         PLSRUN plsrun,
         PCLSCHP plschp,
         PDOBJ pdobj,
         PCOBJDIM pobjdim),
        (plsc, lsdcp, plsrun, plschp, pdobj, pobjdim) )

WRAPIT( LsdnFinishRegularAddAdvancePen,
        (PLSC plsc,            /* IN: Pointer to LS Context */
         LSDCP lsdcp,          /* IN: dcp adopted           */
         PLSRUN plsrun,        /* IN: PLSRUN                */
         PCLSCHP plschp,       /* IN: CHP                   */
         PDOBJ pdobj,          /* IN: PDOBJ                 */
         PCOBJDIM pobjdim,     /* IN: OBJDIM                */
         long durPen,          /* IN: durPen                */
         long dvrPen,          /* IN: dvrPen                */
         long dvpPen),         /* IN: dvpPen                */
        (plsc, lsdcp, plsrun, plschp, pdobj, pobjdim, durPen, dvrPen, dvpPen) )

WRAPIT( LsdnGetCurTabInfo,
        (PLSC plsc,              /* IN: Pointer to LS Context */
         LSKTAB* plsktab),       /* OUT: Type of current tab  */
        (plsc, plsktab) )

WRAPIT( LsdnGetDup,
        (PLSC plsc,             /* IN: Pointer to LS Context */
         PLSDNODE plsdn,        /* IN: DNODE queried         */
         long* pdup),           /* OUT: dup                  */
        (plsc, plsdn, pdup) )

WRAPIT( LsdnGetFormatDepth,
        (PLSC plsc,                         /* IN: Pointer to LS Context    */
         DWORD* pnDepthFormatLineMax),      /* OUT: nDepthFormatLineMax     */
        (plsc, pnDepthFormatLineMax) )

WRAPIT( LsdnQueryObjDimRange,
        (PLSC plsc,
         PLSDNODE plsdnFirst,
         PLSDNODE plsdnLast,
         POBJDIM pobjdim),
        (plsc, plsdnFirst, plsdnLast, pobjdim) )

WRAPIT( LsdnQueryPenNode,
        (PLSC plsc,                /* IN: Pointer to LS Context */
         PLSDNODE plsdnPen,        /* IN: DNODE to be modified */
         long* pdvpPen,            /* OUT: &dvpPen */
         long* pdurPen,            /* OUT: &durPen */
         long* pdvrPen),           /* OUT: &dvrPen */
        (plsc, plsdnPen, pdvpPen, pdurPen, pdvrPen) )

WRAPIT( LsdnResetObjDim,
        (PLSC plsc,             /* IN: Pointer to LS Context */
         PLSDNODE plsdn,        /* IN: plsdn to modify */
         PCOBJDIM pobjdimNew),  /* IN: dimensions of dnode */
        (plsc, plsdn, pobjdimNew) )

WRAPIT( LsdnResetPenNode,
        (PLSC plsc,                /* IN: Pointer to LS Context */
         PLSDNODE plsdnPen,        /* IN: DNODE to be modified */
         long dvpPen,              /* IN: dvpPen */
         long durPen,              /* IN: durPen */
         long dvrPen),             /* IN: dvrPen */
        (plsc, plsdnPen, dvpPen, durPen, dvrPen) )

WRAPIT( LsdnResolvePrevTab,
        (PLSC plsc),
        (plsc) )

WRAPIT( LsdnSetAbsBaseLine,
        (PLSC plsc,              /* IN: Pointer to LS Context */
         long vaAdvanceNew),     /* IN: new vaBase            */
        (plsc, vaAdvanceNew) )

WRAPIT( LsdnModifyParaEnding,
        (PLSC plsc,              /* IN: Pointer to LS Context */
         LSKEOP lskeop),         /* IN: kind of line ending   */
        (plsc, lskeop) )

WRAPIT( LsdnSetRigidDup,
        (PLSC plsc,                 /* IN: Pointer to LS Context */
         PLSDNODE plsdn,            /* IN: DNODE to be modified  */
         long dup),                 /* IN: dup                   */
        (plsc, plsdn, dup) )

WRAPIT( LsdnSubmitSublines,
        (PLSC plsc,                     /* IN: Pointer to LS Context    */
         PLSDNODE plsdnode,             /* IN: DNODE                    */
         DWORD cSublinesSubmitted,      /* IN: cSublinesSubmitted       */
         PLSSUBL* rgpsublSubmitted,     /* IN: rgpsublSubmitted         */
         BOOL fUseForJustification,     /* IN: fUseForJustification     */
         BOOL fUseForCompression,       /* IN: fUseForCompression       */
         BOOL fUseForDisplay,           /* IN: fUseForDisplay           */
         BOOL fUseForDecimalTab,        /* IN: fUseForDecimalTab        */
         BOOL fUseForTrailingArea ),    /* IN: fUseForTrailingArea      */
        (plsc, plsdnode, cSublinesSubmitted, rgpsublSubmitted, fUseForJustification, fUseForCompression, fUseForDisplay, fUseForDecimalTab, fUseForTrailingArea) )

WRAPIT( LsdnSkipCurTab,
        (PLSC plsc),                /* IN: Pointer to LS Context */
        (plsc) )

WRAPIT( LssbFDoneDisplay,
        (PLSSUBL plssubl,           /* IN: Subline Context  */
         BOOL* pfDoneDisplay),      /* OUT: Is it displayed */
        (plssubl, pfDoneDisplay) )

WRAPIT( LssbFDonePresSubline,
        (PLSSUBL plssubl,            /* IN: Subline Context          */
         BOOL* pfDonePresSubline),   /* OUT: Is it CalcPresrd        */
        (plssubl, pfDonePresSubline) )

WRAPIT( LssbGetDupSubline,
        (PLSSUBL plssubl,            /* IN: Subline Context          */
         LSTFLOW* plstflow,          /* OUT: subline's lstflow       */
         long* pdup),                /* OUT: dup of subline          */
        (plssubl, plstflow, pdup) )

WRAPIT( LssbGetNumberDnodesInSubline,
        (PLSSUBL plssubl,            /* IN: Subline Context          */
         DWORD* pcDnodes),           /* OUT: N of DNODES in subline  */
        (plssubl, pcDnodes) )

WRAPIT( LssbGetObjDimSubline,
        (PLSSUBL plssubl,            /* IN: Subline Context          */
         LSTFLOW* plstflow,          /* OUT: subline's lstflow       */
         POBJDIM pobjdim),           /* OUT: dimensions of subline   */
        (plssubl, plstflow, pobjdim) )

WRAPIT( LssbGetPlsrunsFromSubline,
        (PLSSUBL plssubl,            /* IN: Subline Context          */
         DWORD cDnodes,              /* IN: N of DNODES in subline   */
         PLSRUN* rgplsrun),          /* OUT: array of PLSRUN's       */
        (plssubl, cDnodes, rgplsrun) )

WRAPIT( LssbGetVisibleDcpInSubline,
        (PLSSUBL plssubl,            /* IN: Subline Context          */
         LSDCP* pdcp),               /* OUT: count of characters     */
        (plssubl, pdcp) )

WRAPIT( LsGetLineDur,
        (PLSC plsc,                  /* IN: ptr to line services context    */
         PLSLINE plsline,            /* IN: ptr to line -- opaque to client */
         long * durWithTrailing,     /* OUT: dur of line incl. trailing area    */
         long * durWithoutTrailing), /* OUT: dur of line excl. trailing area    */
        (plsc, plsline, durWithTrailing, durWithoutTrailing) )

WRAPIT( LsGetMinDurBreaks,
        (PLSC plsc,                  /* IN: ptr to line services context    */
         PLSLINE plsline,            /* IN: ptr to line -- opaque to client */
         long* durWithTrailing,      /* OUT: min dur between breaks including trailing white */
         long* durWithoutTrailing),  /* OUT: min dur between breaks excluding trailing white */
        (plsc, plsline, durWithTrailing, durWithoutTrailing) )
