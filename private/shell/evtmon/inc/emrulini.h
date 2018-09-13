/*****************************************************************************
    emrulini.h

    Owner: DaleG
    Copyright (c) 1992-1997 Microsoft Corporation

    General Rule-Network Propagation Engine initialization types.

*****************************************************************************/

#ifndef EMRULINI_H
#define EMRULINI_H

MSOEXTERN_C_BEGIN   // ***************** Begin extern "C" ********************

/*************************************************************************
    Types:

    rullims     Rule limits/sizes structure.
    ruldef      Rule node initial definition structure.
    rulinit     Rule initialization structure.

 *************************************************************************/


/* R  U  L  L  I  M  S */
/*----------------------------------------------------------------------------
    %%Structure: RULLIMS
    %%Contact: daleg

    Rule base limits structure.
----------------------------------------------------------------------------*/

typedef struct _RULLIMS
    {
    short               irulRulesMax;                   // Number of nodes
    short               irulVarsMax;                    // Number of var nds
    short               rulevtMax;                      // Num of event_types
    short               rulevlMax;                      // Num of eval levels
    short               rulgGroupMax;                   // Num dependency grps
    short               rulgRegularMax;                 // Num non-context grps
    short               clprulDependMax;                // Num forward depends
    short               clprulBackDependMax;            // Num backwrd depends
    short               ckwdMax;                        // Number of KWD recs
    short               cchKwdTextMax;                  // Len kwd text buffer
    unsigned short      cchNamesMax;                    // Name/string buf len
    } RULLIMS;



/* R  U  L  D  E  F */
/*----------------------------------------------------------------------------
    %%Structure: RULDEF
    %%Contact: daleg

    Rule base node initialization structure.
----------------------------------------------------------------------------*/

typedef struct _RULDEF
    {
    short               rulevl;                         // Event_type/Level
    short               bwDepend;                       // Fwd depend offset
    short               bwBackDepend;                   // Bkwd depend offset
    const XCHAR        *xstzKeywordName;                // Keyword string
    } RULDEF;



/* L  P  F  N  R  U  L  C  X  T */
/*----------------------------------------------------------------------------
    %%Structure: LPFNRULCXT
    %%Contact: daleg

    Rule base Context group callback function
----------------------------------------------------------------------------*/

typedef void (* LPFNRULCXT) (int irul, struct _RULCXT *lprulcxt);




/* R  U  L  I  N  I  T */
/*----------------------------------------------------------------------------
    %%Structure: RULINIT
    %%Contact: daleg

    Rule base global initialization structure.
----------------------------------------------------------------------------*/

typedef int (WIN_CALLBACK *PFNEVAL)(short irul);        // Rule Eval function

typedef struct _RULINIT
    {
    const RULLIMS      *lprullims;                      // Rulebase limits
    const RULDEF       *lprulinit;                      // Rulebase def
    const short        *rgrulevlRulevt;                 // Event_type eval lvls
    const short        *rgirultkRulevtHistoryMax;       // Evt history depths
    const short        *lprulgAppendTo;                 // Group linkages
    const short        *lprulgAppendedFrom;             // Group linkages
    const LPFNRULCXT   *lplpfnrulcxt;                   // Context grp callbks
    const short* const *lplpirulDependents;             // Dependent lists
    const short* const *lplpirulBackDependsOn;          // DependsOn lists
    PFNEVAL             lpfnEvalRule;                   // Evaluate rule code
    MSOKWTB           **rgpkwtbKeyTables;               // Keyword tables
    const char * const *lpszRulNames;                   // Node names
    } RULINIT;


typedef int (WIN_CALLBACK *LPFNRulinit)(struct _RULS *lpruls);


// Initialize pre-constructed rulebase of static nodes
MSOAPI_(int) MsoFInitStaticRuls(
    struct _RULS       *pruls,
    struct _RULS       *prulsInit
    );


// Create and initialize rulebase of static nodes from compressed rulebase
MSOAPI_(int) MsoFInitCompressedRulebase(
    struct _RULS       *lpruls,
    const RULINIT      *lprulinit
    );


#ifndef pNil
#define pNil    NULL
#endif /* !pNil */

#define IN_DATASEG
#define NOT_IN_DATASEG

// Rule node type flags: shared with rule.h
#define rultRule            0x00                        // Rule
#define rultEvent           0x01                        // Event/Variable
#define rultPrimaryRule     0x02                        // Rule auto-scheduled
#define rultActionRule      0x00                        // Not really a flag
///#define rultNonTermRule  0x08
///#define rultSeqRule      0x10

MSOEXTERN_C_END     // ****************** End extern "C" *********************

#endif /* EMRULINI_H */
