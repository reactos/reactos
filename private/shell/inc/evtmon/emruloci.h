/*****************************************************************************
    emruloci.h

    Owner: DaleG
    Copyright (c) 1996-1997 Microsoft Corporation

    Header file of Interface of Op-Code Interpreter to Rulebase.

*****************************************************************************/

#ifndef EMRULOCI_H
#define EMRULOCI_H

#include "emoci.h"
#include "emkwd.h"
#include "emrule.h"


MSOEXTERN_C_BEGIN   // ***************** Begin extern "C" ********************


MSOAPI_(MSOOCV) MsoOcvEvalIrul(IRUL irul);              // Eval dyn rule instrs

MSOAPI_(int) MsoFReadDynOciRules(                       // Load oci file
    char               *szFilePrefix,
    int                *pdocii                          // RETURN
    );

MSOAPI_(int) MsoFLoadDynRulesPocii(                     // Load dyn rulebase
    MSOOCII            *pocii,
    int                 docii,
    MSOPFNOCI const    *rgpfn,
    MSOOCAD const      *rgocadArgDesc,
    unsigned char const*rgcbImmedArg,
    int                 ipfnMax,
    char const * const *rgpchDynNames                   // DEBUG ONLY
    );

MSOAPI_(int) MsoFCopyBaseRulRgpfnoci(MSOPFNOCI *rgpfn); // Set 1st n oci fns

MSOAPIX_(MSOOCV *) PocvOfIrul(short irul);              // Return addr of node

MSOAPI_(MSOOCV) MsoOcv_DelayGoToDirul(MSOOCV *pocvSP);  // DelayGoToDirul()

MSOAPI_(MSOOCV) MsoOcv_Signal(MSOOCV *pocvSP);          // Signal a node

MSOAPI_(MSOOCV) MsoOcv_SignalFrom(MSOOCV *pocvSP);      // Signal node from

MSOAPI_(MSOOCV) MsoOcv_RulParams(                       // Set RB params
    MSOOCII           **ppocii,
    MSOOCIS            *pocis
    );

MSOAPI_(MSOOCV) MsoOcv_MapEvalLevels(                   // map static rule lvls
    MSOOCII           **ppocii,
    MSOOCIS            *pocis
    );

MSOAPI_(MSOOCV) MsoOcv_DefEvent(                        // Define an event
    MSOOCII           **ppocii,
    MSOOCIS            *pocis
    );

MSOAPI_(MSOOCV) MsoOcv_DefRule(                         // Define a rule
    MSOOCII           **ppocii,
    MSOOCIS            *pocis
    );

MSOEXTERN_C_END     // ****************** End extern "C" *********************

#endif /* !EMRULOCI_H */
