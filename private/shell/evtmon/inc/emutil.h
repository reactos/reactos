/*****************************************************************************
    emutil.h

    Owner: SMueller
    Copyright (c) 1997 Microsoft Corporation

    Miscellaneous event monitor utilities
*****************************************************************************/

#ifndef _EMUTIL_H
#define _EMUTIL_H

#include "emrule.h"

MSOEXTERN_C_BEGIN   // ***************** Begin extern "C" ********************

MSOCDECLAPI_(int) MsoIMatchIrul(IRUL irul, ...);
MSOAPI_(MSORULTK *) MsoPrultkFromDtk(int dtk, MSORULTKH* ptkh);
MSOAPI_(int) MsoDtkFromTokenDtki(int dtki, MSORULTKH* ptkh);
MSOAPI_(int) MsoDtkiFromTokenDtk(int dtk, MSORULTKH* ptkh);
MSOAPI_(int) MsoFTokenWas(int dtkStart, int irul, MSORULTKH* ptkh);
MSOAPI_(long) MsoCpLimOfDtk(int dtk, MSORULTKH* ptkh);
MSOAPI_(int) MsoDtkFindPrevToken(int dtkStart, int irul, MSORULTKH *ptkh);

// Get cpFirst of token from index to history
#define MsoCpFirstOfDtk(dtk, ptkh) \
        (MsoPrultkFromDtk(dtk, ptkh)->cpFirst)



#ifdef DEBUG

#if defined(OFFICE_BUILD)  ||  defined(STANDALONE_WORD)

#include "msosdm.h"

MSOAPI_(const char *) MsoSzFromDlmDebug(DLM dlm);

#endif /* OFFICE_BUILD  ||  STANDALONE_WORD */

MSOAPI_(const char *) MsoSzRulName(IRUL irul);

#endif // DEBUG

MSOEXTERN_C_END     // ****************** End extern "C" *********************

#endif // !_EMUTIL_H

