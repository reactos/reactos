/*----------------------------------------------------------------------------
    %%File: EMDYN.C
    %%Unit: Event Monitor (mntr)
    %%Contact: daleg

    Event Monitor Sample Application, Main Program.

    The purpose of this application is to demonstrate how to process events
    using the Event Monitor's Rule Compiler and rule engine.
----------------------------------------------------------------------------*/

#include "mso.h"
#include "msoem.h"

#if FEATURE_NYI // include'er file will give name
DEBUGASSERTSZ
#endif

#include "emruloci.h"

#if FEATURE_DEAD
extern MSOKWTB vkwtbEmCHAR_KEYTABLE;
#endif


#include "emact.h"
#include "msolex.h"
#if 0
#include "emtest.h"
#include "emintf.c"
#include "emfn.c"
#endif


/* F  L O A D  D Y N  E M  R U L E S */
/*----------------------------------------------------------------------------
    %%Function: FLoadDynEmRules
    %%Contact: daleg

    Load rules dynamically from a file or a data structure, depending upon
    #ifdef
----------------------------------------------------------------------------*/

#ifdef DYN_RULES_FROM_STRUCT

#include "emruli.oci"

#endif /* DYN_RULES_FROM_STRUCT */



int FLoadDynEmRules(void)
{
    int                 docii;

#ifndef DYN_RULES_FROM_STRUCT

    /* Read opcodes from a disk file */
    if (!MsoFReadDynOciRules("em", &docii))
        return fFalse;

#else /* DYN_RULES_FROM_STRUCT */

    /* Get opcodes from a structure */
    vlpruls->pociiDynRules = (PV) vrgociiEm;
    docii = IMaxRg(vrgociiEm, MSOOCII);
    vlpruls->rgpchDynNames = _rgszEmRulNames;

#endif /* !DYN_RULES_FROM_STRUCT */

#define NON_CONST_OCI_VTABLE
#ifdef NON_CONST_OCI_VTABLE
    /* Insert Mso functions into OCI vtable */
    if (!MsoFCopyBaseRulRgpfnoci(vpfnociEm))
        return fFalse;
#endif /* NON_CONST_OCI_VTABLE */

    /* Generate rulebase nodes from opcode stream */
    return MsoFLoadDynRulesPocii(vlpruls->pociiDynRules, docii,
                                 vpfnociEm, vrgocadEm,
                                 vrgcbOciArgEm, IMaxRg(vpfnociEm, MSOPFNOCI),
                                 DebugElse(vlpruls->rgpchDynNames, pNil));
}



