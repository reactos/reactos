/*****************************************************************************
    emrule.h

    Owner: DaleG
    Copyright (c) 1992-1997 Microsoft Corporation

    General Rule-Network Propagation Engine functions and prototypes

----------------------------------------------------------------------------
    NOTE:

    1.  BAD CODE, but it works: FIsDelayFactor(),LpruldependFromCDelay() and
        CDelayFromLpruldepend() rely upon the fact (?) that pointers
        on the machines we support are never (?) small integers.  But if
        this should ever prove false, it would be bad.  We should rewrite
        this to either have a RULDEP structure (as does the rule compiler),
        or make the list of dependencies indexes to rules, rather than
        pointers.

*****************************************************************************/

#ifndef EMRULE_H

#include "msodbglg.h"
#include "emkwd.h"
#include "emrulini.h"
#include "emrultk.h"


MSOEXTERN_C_BEGIN   // ***************** Begin extern "C" ********************

// REVIEW: configure this in makefile, so emtest (for example) can build
// debug without dynamic rules
#ifdef DEBUG
#ifndef YY_NODYNRULES
#define DYN_RULES 1
#endif
#endif


/*----------------------------------------------------------------------------
    System limits
----------------------------------------------------------------------------*/

#define wDelayMax           100                         // Maximum delay poss
#define irultkRuleMax       256                         // Max events cached
#define iruldepAllocMax     256                         // Max ruldeps alloced
#define ichRulNameMax       128                         // Max trace name len
#define ichRulNameAllocMax  1024                        // Max name alloc


/*************************************************************************
    Types:

    irul        Rule node ID.
    ruldep      Rule node dependency link.
    ruldepblk   Rule node dependency link allocation structure.
    sv          Split Value structure.
    rul         Rule node structure.
    ruls        Rule state structure.

 *************************************************************************/


/* I R U L */
/*----------------------------------------------------------------------------
    %%Structure: IRUL
    %%Contact: daleg

    Rule Index definition
----------------------------------------------------------------------------*/

typedef short IRUL;

#define IrulFromTk(tk)      ((IRUL) (tk))
#define TkFromIrul(irul)    (irul)

// Lexer tokens: hard-coded rule node IDs
#define irulERROR       tkERROR
#ifdef tkNil
#define irulNil         tkNil
#else /* !tkNil */
#define irulNil         0
#endif /* tkNil */

// Is rule ID valid (not irulNil and not irulERROR)
#define FValidIrul(irul) \
            ((irul) > 0)




/* R  U  L  D  E  P */
/*----------------------------------------------------------------------------
    %%Structure: RULDEP
    %%Contact: daleg

    Rule node dependency structure.
    This structure holds the dependency information that links a rule node
    to its dependents.
----------------------------------------------------------------------------*/

typedef struct _RULDEP
    {
    struct _MSORUL             *prul;                   // Node referenced
    struct _RULDEP             *pruldepNext;            // Next dependency
//  int                         cDelay;                 // Delay in eval
    } RULDEP;


MSOAPI_(RULDEP *) _MsoPruldepNew(                       // Alloc blk of deps
    int                 cruldep,
    int                 fOnFreeList
    );


// Return a new ruldep record from the free list (requires local var pruldep)
// Operates in either sequential mode (vlpruls->lpruldepNextFree) or free list
#define LpruldepNew() \
            (vlpruls->lpruldepNextFree != NULL \
                ? (vlpruls->fSeqentialFreeRuldeps \
                    ? vlpruls->lpruldepNextFree++ \
                    : (pruldep = vlpruls->lpruldepNextFree, \
                            vlpruls->lpruldepNextFree \
                                    = pruldep->pruldepNext, \
                            pruldep->pruldepNext = NULL, \
                            pruldep)) \
                : _MsoPruldepNew(iruldepAllocMax, TRUE))

// Push a ruldep record onto the free list
#define PushLpruldepOnFreeList(pruldep) \
            ((pruldep)->pruldepNext = vlpruls->lpruldepNextFree, \
             vlpruls->lpruldepNextFree = (pruldep))



/* R  U  L  D  E  P  B  L  K */
/*----------------------------------------------------------------------------
    %%Structure: RULDEPBLK
    %%Contact: daleg

    Rule node dependency allocation block structure.
    This structure allows us to batch-allocate RULDEP records.
----------------------------------------------------------------------------*/

typedef struct _RULDEPBLK
    {
    struct _RULDEPBLK      *lpruldepblkNext;            // Next block
    RULDEP                  rgruldep[1];                // Array of ruldeps
    } RULDEPBLK;


/* R  U  L  C  X  T */
/*----------------------------------------------------------------------------
    %%Structure: RULCXT
    %%Contact: daleg

    (Rul)e (C)onte(x)t-group (T)able structure.
    This structure allows the rule engine to support cheap sparse dependents
    lists for special contexts.
    The callback function allows us to perform any desired side effects
    during the propagation, as well as offering the chance to optimize
    the lookup algorithm.
----------------------------------------------------------------------------*/

typedef struct _RULCXT
    {
    struct _RULCXT         *lprulcxtNext;               // Next context
    LPFNRULCXT              lpfnrulcxt;                 // Callback fn
    int                     iruldepMax;                 // Size of array
    struct _RULCXL         *rglprulcxl[1];              // Hash table
    } RULCXT;



/* R  U  L  C  X  L */
/*----------------------------------------------------------------------------
    %%Structure: RULCXL
    %%Contact: daleg

    (Rul)e (C)onte(x)t-group (L)ist item structure.
    This is the item used in the hash table of the RULCXT above.
----------------------------------------------------------------------------*/

typedef struct _RULCXL
    {
    IRUL                    irul;                       // Rule ID
    struct _RULDEP         *pruldep;                    // Dependent list
    struct _RULCXL         *lprulcxlNext;               // Next in hash chain
    } RULCXL;


#ifdef DEBUG
/* R  U  L  N  B  L  K */
/*----------------------------------------------------------------------------
    %%Structure: RULNBLK
    %%Contact: daleg

    (Rul)e (N)ame block
    Store dynamically constructed rule names.
----------------------------------------------------------------------------*/

typedef struct _RULNBLK
    {
    struct _RULNBLK        *lprulnblkNext;              // Next block
    char                    rgch[1];                    // Block of text
    } RULNBLK;


char *LszGenNameForLprul(                               // Gen dyn rule name
    struct _MSORUL     *prul,
    int                 irulAssert
    );
char *SzSaveRulName(char *szName);                      // Save node name sz
char *LpchRulNameNew(int dichNeeded);                   // Get new name lpch

#endif /* DEBUG */





/* M  S  O  R  U  L */
/*----------------------------------------------------------------------------
    %%Structure: MSORUL
    %%Contact: daleg

    Rule node structure.
    This structure holds the state information for a rule within the
    propagation network.

    ASSUMPTIONS:

        1.  The wDelayMask field only applies to rules, but it is (currently)
            faster to not have to test for whether a node is a rule, by
            merely leaving the field empty for values.
----------------------------------------------------------------------------*/

typedef long RULV;
typedef struct _MSORUL *MSOPRUL;

#pragma pack(2)

typedef struct _MSORUL
    {
    IRUL                        irul;                   // Rule ID
    char                        rultType;               // Type: rule/event
    char                        ipfnrulscSeqCheck;      // Fn to ensure contig
    short                       rulevl;                 // Evaluation level
    short                       birulDependsOn;         // Depends on: nodes
    SVL                         svl;                    // 32-bit storage area
#ifdef DEBUG
    char const                 *lpchName;               // Name or rule text
#endif /* DEBUG */
    IRUL                        irulNextChanged;        // Next changed value
    union
        {
        short                   ipociiInstrs;           // Interp instrs
        short                   USEME_IN_EVENTS;        // Unsed slot
        };
    short                       wIntervalCount;         // Ensure contig seqs
    short                       wDelayMask;             // Delays as bit pos's
    struct _MSORUL             *prulNext;               // Next node in queue
#ifdef DEBUG
    IRUL                        irulNextTrace;          // Next traced rule
    short                       wDebugFlags;            // Random debug flags
#endif /* DEBUG */
    } MSORUL;

#pragma pack()



// Allocate new nodes
MSOAPI_(MSORUL *) MsoPrulNew(void);                     // Allocate new node
MSOAPI_(int) MsoFEnsureIrulAllocated(int irulMax);      // Pre-allocated nodes

// Discard an existing rul node
#define DiscardIrul(irul) \
            (vlpruls->irulLim--)



#define msoprulNil      ((MSORUL *) -1)                 // NIL value for MSORUL
#define msoprulInactive ((MSORUL *) -2)                 // Node is deactivated
#define wRulTrue        1000
#define wRulFalse       0

// Rule node type flags: shared with emrulini.h and rulc.h
#define rultNil             0
#define rultRule            0x00                        // Rule
#define rultEvent           0x01                        // Event/Variable
#define rultPrimaryRule     0x02                        // Rule auto-scheduled
#define rultImmediateRule   0x04                        // Rule executes immed
#define rultPersistentRule  0x20                        // Rule not cleared
#define rultAlwaysPersist   0x40                        // Rule never cleared
#define rultSpecialKwd      0x80                        // Node is generic type

// Debug check that rule is not marked as both NonTerminal and Seq
#define rultRuleMask        0x19

#ifdef NEVER
#define FRultRuleIs(rult, rultExpected) \
            (((rult) & rultRuleMask) == ((rultExpected)))
#endif /* NEVER */


#ifdef DEBUG
#define rultDynActionRule   0x04                        // Dyn DEBUG only
#define rultNonTermRule     0x08                        // Dyn DEBUG only
#define rultSeqRule         0x10                        // Dyn DEBUG only
#endif /* DEBUG */


/* R  U  L  S */
/*----------------------------------------------------------------------------
    %%Structure: RULS
    %%Contact: daleg

    Rule state structure.
    This structure holds the state information for the rule engine.
----------------------------------------------------------------------------*/

typedef int (WIN_CALLBACK *LPFNRul)(IRUL irul);         // Rule Eval function
typedef short (WIN_CALLBACK *PFNRULSC)(void);           // Interval seq chk fn
typedef int (*PFNRULVT)(IRUL irul);                     // Rule V-Table

typedef struct _RULS
    {
    // Rule base limits
    RULLIMS             rullims;                        // Rule base limits
    int                 irulMax;                        // Num nodes allocated
    int                 irulLim;                        // Num nodes used

    // Rule base state information
    RULDEP           ***rgrgpruldepDependents;          // List of Dep lists
    RULDEP            **rgpruldepDependents;            // Active Dep lists
    MSORUL            **lrglprulBlk;                    // Array of node arrays
    int                 ilprulNodesLim;                 // #arrays of arrays
#ifdef DEBUG
    MSORUL            **rgprulNodes;                    // Debug node array
#endif /* DEBUG */
    const short        *prulgAppendTo;                  // Group linkages
    const short        *prulgAppendedFrom;              // Group linkages
    short const        *rgrulevlRulevt;                 // Event_type eval lvls
    int                *rgrulevlRulevtLast;             // Highest Q of rulevts
    MSORUL            **rgprulActiveQueues;             // Active eval queues
    MSORUL            **rgprulDelayedQueues;            // Delayed eval queues
    MSORUL             *lprulQPersistent;               // Temp Q: persistent
    int                *rgirulRulevtChanged;            // Nodes changed in evt
    int                *rgrulevtEval;                   // Pending rulevt eval
    MSORULTKH          *prultkhRulevtHistory;           // Ev history for type
    long               *rgdtkiRulevt;                   // #times enter rulevt
    const int          *rgrulevtFromRulevl;             // Trans lvls to evts
    const short        *lpgrpirulDependBack;            // Back dependencies
    LPFNRul             lpfnEvalRule;                   // Evaluate rule code
    PFNRULSC const     *rgpfnrulscSeqCheck;             // Interval seq chk fns
    MSOKWTB           **rgpkwtbKeyTables;               // Keyword tables

    // Allocation information
    WORD                fDynamicInit : 1;               // Structs alloced?
    WORD                fDynAlloced : 1;                // vlpruls alloced?
    WORD                fDependBackDynAlloced : 1;      // Back deps alloced?
    WORD                fRgDependDynAlloced : 1;        // Dep lists alloced?
    WORD                fDynRulesAlloced : 1;           // Dyn rulebase alloc?
    WORD                fRgrulevlRulevtAlloced : 1;     // Last lvl tbl alloc?
    WORD                fRgprulQueuesAlloced : 1;       // Eval queues alloc?
    WORD                fRgrulevtFromRulevlAlloced : 1; // Lvl-evt tbl alloc?
    WORD                fRgprulNodesAlloced : 1;        // DEBUG nd arr alloc?

    int                 rulgCurr;                       // Current rule group
    IRUL                irulSelf;                       // irulSelf under eval
    MSORUL             *prulEvent;                      // Event causing eval
    IRUL                irulPrimaryEvent;               // Primary ev of intvl
    RULDEP             *lpruldepNextFree;               // Next free dep rec
    RULDEPBLK          *lpruldepblkDependBlocks;        // List of dep blocks
    RULCXT             *lprulcxtActive;                 // Active context list
    RULCXT            **lrglprulcxtContexts;            // List of cntx groups
    int                 rulevtCurr;                     // Current event_type
    int                *prulevtEvalLim;                 // Num Ev types to eval
    MSORUL             *lprulQueue;                     // Current queue
    int                 rulevlRultevtMin;               // 1st eval lvl in evt
    int                 rulevlRulevtLast;               // Last eval lvl in evt
    void               *pociiDynRules;                  // Dyn-loaded rulebase
#ifdef DEBUG
    char               *lpchNames;                      // Name/string buf
    char const * const *rgpchDynNames;                  // Interp: node names
    int                 irulQTrace;                     // Backtrace list
    int                 dichNameFree;                   // Num chars avail
    char               *lpchNameNextFree;               // Next free name rgch
    RULNBLK            *lprulnblkNames;                 // Dyn name list
#endif /* DEBUG */

    // Run-time flags
    WORD                fInited : 1;                    // Rule base inited?
    WORD                fNew : 1;                       // Rule base new?
    WORD                fSeqentialFreeRuldeps : 1;      // Free ruldeps are seq
    WORD                fEvaluatingDeferred : 1;        // Evaling deferred nd?
    WORD                fEvaluating: 1;                 // Eval recursion check

    // Multiple Rule base support
    struct _RULS       *lprulsNext;                     // Next struct LIFO

    // Allocation info
    WORD                fAllocedSpare1 : 1;
    WORD                fAllocedSpare2 : 1;
    WORD                fAllocedSpare3 : 1;
    WORD                fAllocedSpare4 : 1;
    WORD                fAllocedSpare5 : 1;
    WORD                fAllocedSpare6 : 1;
    WORD                fAllocedSpare7 : 1;
    WORD                fAllocedSpare8 : 1;
    WORD                fAllocedSpare9 : 1;
    WORD                fAllocedSpare10 : 1;
    WORD                fAllocedSpare11 : 1;
    int                 ilprulNodesAllocFirstLim;       // Start alloc'd nds +1
    long                lReturn;                        // Return value
    LPV                 lpvSpare3;
    LPV                 lpvSpare4;
    LPV                 lpvSpare5;
    LPV                 lpvSpare6;
    LPV                 lpvSpare7;
    LPV                 lpvSpare8;
    LPV                 lpvSpare9;
    LPV                 lpvSpare10;
    LPV                 lpvSpare11;

    // Debug logging
#ifdef DEBUG
    unsigned int        grfDebugLogFilter;              // DEBUG: how to log
#endif /* DEBUG */

#ifdef DYN_RULES
    // Dynamically-loaded rulebase support
    struct _MSOOCIS    *pocis;                          // Op-Code Interp State
    short               irulRuleInterpLim;              // #Interpreted rules
    short               irulRuleInterpMac;              // #Alloced inter ptrs
    void              **rgpociiRules;                   // Rule instructions
#endif /* DYN_RULES */
    } RULS;

extern RULS            *vlpruls;                        // Global rule state


//----------------------------------------------------
// If using debugger, an rulebase node's value is given by:
//      rulv_<irul> == vlpruls->rgprulNodes[irul]->svl.lValue
//
// Or if the rulebase is statically initialized,    and
// DEBUG_RULE_POINTERS is #defined              then
//      event FOO   can be accessed as *prulFOO     and
//      rule 126    can be accessed as *prul126
//----------------------------------------------------

// Return the number of rules in rule base
#define IrulMax()               (vlpruls->irulMax)

// Return the number of compiled rules in rule base
#define IrulCompiledMax()       (vlpruls->rullims.irulRulesMax)

// Return the number of event_types in rule base
#define RulevtMax()             RulevtMaxPruls(vlpruls)

// Return the number of event_types in rule base
#define RulevtMaxPruls(pruls) \
            ((pruls)->rullims.rulevtMax)

// Return the number of evaluation levels in rule base
#define RulevlMax()         RulevlMaxPruls(vlpruls)

// Return the number of evaluation levels in rule base
#define RulevlMaxPruls(pruls) \
            ((pruls)->rullims.rulevlMax)

#define irulMaxAlloc        128                         // Max nodes/array
#define cbfIrulShift        7                           // #bits irulMaxAlloc
#define wIrulMask           0x7F                        // Mask: irulMaxAlloc
#define ilprulMaxAlloc      256L                        // Max arrays of arrys

///#define irulMaxAlloc     2048                        // Max nodes/array
///#define cbfIrulShift     11                          // #bits irulMaxAlloc
///#define wIrulMask        0x7FF                       // Mask: irulMaxAlloc
///#define ilprulMaxAlloc   100L                        // Max arrays of arrys

///#define irulMaxAlloc     1024                        // Max nodes/array
///#define cbfIrulShift     10                          // #bits irulMaxAlloc
///#define wIrulMask        0x3FF                       // Mask: irulMaxAlloc
///#define ilprulMaxAlloc   100L                        // Max arrays of arrys

// Return the rule node structure pointer for the rule ID
#define LprulFromIrul(irul) \
            (&vlpruls->lrglprulBlk \
                [(irul) >> cbfIrulShift] [(irul) & wIrulMask])

// Return the rule ID of the rule node structure pointer
#define IrulFromLprul(prul)  \
            ((prul)->irul)

// Return the Lim irul value for iruls that are contiguous with the irul
#define IrulLimContig(irul) \
            ((((irul) >> cbfIrulShift) << cbfIrulShift) + irulMaxAlloc)

// Return whether rule node is a primary rule
#define FPrimaryRule(prul)      ((prul)->rultType & rultPrimaryRule)

#ifdef NEVER
// Return whether rule node is a action rule
#define FActionRule(prul)       ((prul)->wDebugFlags & rultActionRule)
#endif /* NEVER */

#ifdef DEBUG
// Return whether rule node is a non-terminal rule (then)
#define FNonTermRule(prul)      ((prul)->wDebugFlags & rultNonTermRule)

// Return whether rule node is a sequence rule (...)
#define FSeqRule(prul)          ((prul)->wDebugFlags & rultSeqRule)
#endif /* DEBUG */

// Return whether the node ID refers to an event node
#define FIsEventIrul(irul)          FIsEventPrul(LprulFromIrul(irul))

// Return whether the node is an event node
#define FIsEventPrul(prul)          ((prul)->rultType & rultEvent)

// Return whether the node is a rule node
#define FIsRulePrul(prul)           (!FIsEventPrul(prul))

#define IMMEDIATE_RULES
#ifdef IMMEDIATE_RULES
// Return whether rule node is a sequence rule (...)
#define FImmediateRulePrul(prul)    ((prul)->rultType & rultImmediateRule)
#endif /* IMMEDIATE_RULES */

// Return whether the node is an *undefined* event
#define FSpecialKwdIrul(irul) \
            FSpecialKwdLprul(LprulFromIrul(irul))

// Return whether the node is an *undefined* event
#define SetFSpecialKwdIrul(irul) \
            SetFSpecialKwdLprul(LprulFromIrul(irul))

// Return whether the node is an *undefined* event
#define FSpecialKwdLprul(prul)      ((prul)->rultType & rultSpecialKwd)

// Return whether the node is an *undefined* event
#define SetFSpecialKwdLprul(prul)  \
            ((prul)->rultType |= rultSpecialKwd)

// Return whether rule node can persist in delayed queue in soft resets
#define FPersistentLprul(prul) \
            ((prul)->rultType & rultPersistentRule)

// Mark that rule node can persist in delayed queue in soft resets
#define SetFPersistentIrul(irul) \
            SetFPersistentLprul(LprulFromIrul(irul))

// Mark that rule node can persist in delayed queue in soft resets
#define SetFPersistentLprul(prul) \
            ((prul)->rultType |= rultPersistentRule)

// Return whether rule node can persist in delayed queue in all resets
#define FAlwaysPersistLprul(prul) \
            ((prul)->rultType & rultAlwaysPersist)

// Mark that rule node can persist in delayed queue in all resets
#define SetFAlwaysPersistIrul(irul) \
            SetFAlwaysPersistLprul(LprulFromIrul(irul))

// Mark that rule node can persist in delayed queue in all resets
#define SetFAlwaysPersistLprul(prul) \
            ((prul)->rultType |= rultAlwaysPersist)

// Return whether rule node can persist in delayed queue
#define FPersistLprulGrf(prul, grf) \
            ((prul)->rultType & (grf))

// Return the value for the current rule
#define RulvSelf() \
            RulvOfIrul(irulSelf)

// Set the value for the current rule
#define SetRulvSelf(rulv) \
            SetRulvOfIrul(irulSelf, (rulv))

// Increment the value for the current rule
#define IncrRulvSelf(drulv) \
            IncrRulvOfIrul(irulSelf, (drulv))


// Return the value1 for the current rule
#define Rulv1Self() \
            Rulv1OfIrul(irulSelf)

// Set the value1 for the current rule
#define SetRulv1Self(rulv) \
            SetRulv1OfIrul(irulSelf, (rulv))

// Increment the value1 for the current rule
#define IncrRulv1Self(drulv) \
            IncrRulv1OfIrul(irulSelf, (drulv))


// Return the value2 for the current rule
#define Rulv2Self() \
            Rulv2OfIrul(irulSelf)

// Set the value2 for the current rule
#define SetRulv2Self(rulv) \
            SetRulv2OfIrul(irulSelf, (rulv))

// Increment the value2 for the current rule
#define IncrRulv2Self(drulv) \
            IncrRulv2OfIrul(irulSelf, (drulv))


// Return the value for the rule ID
#define RulvOfIrul(irul) \
            (LprulFromIrul(irul)->svl.lValue)

// Set the value for the rule ID
#define SetRulvOfIrul(irul, rulv) \
            (LprulFromIrul(irul)->svl.lValue = (rulv))

// Increment the value1 field for the rule ID
#define IncrRulvOfIrul(irul, drulv) \
            IncrRulvOfLprul(LprulFromIrul(irul), (drulv))


// Return the lValue field for a rule node
#define RulvOfLprul(prul)               ((prul)->svl.lValue)

// Set the lValue field for a rule node
#define SetRulvOfLprul(prul, w)         ((prul)->svl.lValue = (w))

// Increment the value1 field for a rule node
#define IncrRulvOfLprul(prul, drulv)    ((prul)->svl.lValue += (drulv))


// Set the value for the rule ID
#define PlValueOfIrul(irul) \
            (&LprulFromIrul(irul)->svl.lValue)

// Return the value1 field for the rule ID
#define Rulv1OfIrul(irul) \
            Rulv1OfLprul(LprulFromIrul(irul))

// Set the value1 field for the rule ID
#define SetRulv1OfIrul(irul, rulv) \
            SetRulv1OfLprul(LprulFromIrul(irul), (rulv))

// Increment the value1 field for the rule ID
#define IncrRulv1OfIrul(irul, drulv) \
            IncrRulv1OfLprul(LprulFromIrul(irul), (drulv))

// Return the value2 field for the rule ID
#define Rulv2OfIrul(irul) \
            Rulv2OfLprul(LprulFromIrul(irul))

// Set the value2 field for the rule ID
#define SetRulv2OfIrul(irul, rulv) \
            SetRulv2OfLprul(LprulFromIrul(irul), (rulv))

// Increment the value2 field for the rule ID
#define IncrRulv2OfIrul(irul, drulv) \
            IncrRulv2OfLprul(LprulFromIrul(irul), (drulv))

// Return the value1 field for a rule node
#define Rulv1OfLprul(prul)              W1OfPsv(PsvOfLprul(prul))

// Set the value1 field for a rule node
#define SetRulv1OfLprul(prul, w)        SetW1OfPsv(PsvOfLprul(prul), (w))

// Increment the value1 field for a rule node
#define IncrRulv1OfLprul(prul, dw)      IncrW1OfPsv(PsvOfLprul(prul), (dw))


// Return the value2 field for a rule node
#define Rulv2OfLprul(prul)              W2OfPsv(PsvOfLprul(prul))

// Set the value2 field for a rule node
#define SetRulv2OfLprul(prul, w)        SetW2OfPsv(PsvOfLprul(prul), (w))

// Increment the value2 field for a rule node
#define IncrRulv2OfLprul(prul, dw)      IncrW2OfPsv(PsvOfLprul(prul), (dw))


// Return the Split Value pointer of a node
#define PsvOfLprul(prul)                (&(prul)->svl.sv)


// Return the value1 field for an rule node
#define Rulv1(rulv)                     W1OfPsv((SV *) &(rulv))

// Set the value1 field for a node
#define SetRulv1(rulv, w)               SetW1OfPsv(((SV *) &(rulv)), (w))

// Return the value2 field
#define Rulv2(rulv)                     W2OfPsv((SV *) &(rulv))

// Set the value2 field for a node
#define SetRulv2(rulv, w)               SetW2OfPsv(((SV *) &(rulv)), (w))


// Return the confidence value of a node node
#define WConfidence(prul)               Rulv1OfLprul(prul)

// Set the confidence value of a node node
#define SetConfidence(prul, wValue)     SetRulv1OfLprul((prul), (wValue))

// Return the doubt value of a node node
#define WDoubt(prul)                    Rulv2OfLprul(prul)

// Set the doubt value of a node node
#define SetDoubt(prul, wValue)          SetRulv2OfLprul((prul), (wValue))


// OBSOLETE FORMS OF MACROS
#define WValueOfIrul(irul)              RulvOfIrul(irul)
#define SetWValueOfIrul(irul, rulv)     SetRulvOfIrul(irul, rulv)
#define WRulValue1(prul)                Rulv1OfLprul(prul)
#define SetWRulValue1(prul, w)          SetRulv1OfLprul((prul), (w))
#define WRulValue2(prul)                Rulv2OfLprul(prul)
#define SetWRulValue2(prul, w)          SetRulv2OfLprul((prul), (w))

// Return the rule node for the rule value
#define LprulOfWValue(lplValue) \
            ((MSORUL *) \
                (((char *) lplValue) - CchStructOffset(MSORUL, svl.lValue)))

#ifdef DEBUG
// Return the value name or rule text of the rule node structure pointer
#define LpchRulName(prul)   ((prul)->lpchName)

// Return the value name or rule text of the rule node structure pointer
#define LpchIrulName(irul)  LpchRulName(LprulFromIrul(irul))

// Return debug rule name for dynamic rule
#define PszNameDynLprul(prul) \
            (vlpruls->rgpchDynNames[(prul)->irul - IrulCompiledMax()])
#endif /* DEBUG */

// Return the node evaluation level for the node
#define RulevlOfPrul(prul) \
            (prul->rulevl)

// Return the event_type for the node
#define RulevtOfLprul(prul) \
            RulevtOfRulevl(RulevlOfPrul(prul))

// Return the event_type for the evaluation level
#define RulevtOfRulevl(rulevl) \
            (vlpruls->rgrulevtFromRulevl[rulevl])

// Return the rule queue of the rule level
#define LplprulQueueOf(rulevl)  (&vlpruls->rgprulActiveQueues[rulevl])

// Return the delayed-evaluation rule queue of the event_type
#define LplprulDelayedQueueOf(rulevt) \
            (&vlpruls->rgprulDelayedQueues[rulevt])

// Return the minimum evaluation level of the event_type
#define RulevlMinOfRulevt(rulevt) \
            (vlpruls->rgrulevlRulevt[(rulevt)])

// Return the maximum evaluation level of the event_type
#define RulevlMaxOfRulevt(rulevt) \
            (vlpruls->rgrulevlRulevt[(rulevt) + 1])

// Return the list of dependent node references of node ID
#define LpruldepFromIrul(irul) \
            (vlpruls->rgpruldepDependents[irul])

// Return the list of dependent node references of node
#define LpruldepGetDependents(prul) \
            LpruldepFromIrul(IrulFromLprul(prul))

// Set the list of dependent node references of node ID
#define SetLpruldepFromIrul(irul, pruldep) \
            (vlpruls->rgpruldepDependents[irul] = (pruldep))

// Set the list of dependent node references of node
#define LpruldepSetDependents(prul, pruldep) \
            SetLpruldepFromIrul(IrulFromLprul(prul), (pruldep))

// Return the list of dependent node references of node ID for specific group
#define LpruldepFromRulgIrul(rulg, irul) \
            (*LplpruldepForRulgIrul(rulg, irul))

// Set the list of dependent node references of node ID for specific group
#define SetLpruldepFromRulgIrul(rulg, irul, pruldep) \
            (*LplpruldepForRulgIrul(rulg, irul) = (pruldep))

// Return the address of the start of a ruldep list for the irul and group
#define LplpruldepForRulgIrul(rulg, irul) \
            (&(vlpruls->rgrgpruldepDependents[rulg][irul]))

// Return whether a dependent reference is in fact a delay specfication
#define FIsDelayFactor(lprulDepend) \
            ((unsigned long) (lprulDepend) < wDelayMax)

// Return the delay value associated with the dependency record
#define CDelayFromLpruldepend(lprulDepend) \
            ((int) ((unsigned long) lprulDepend))

// Return a dependency record to represent the delay factor
#define LpruldependFromCDelay(cDelay) \
            ((MSORUL *) (cDelay))

// Add a delay to the delay field of a delayed rule
#define AddCDelayToLprul(prul, cDelay) \
            ((prul)->wDelayMask |= (cDelay))

// Return whether a rule has any delay factor
#define FHaveCDelay(prul) \
            ((prul)->wDelayMask)

// Return whether a rule has a specific delay factor
#define FHaveCDelayOf(prul, cDelay) \
            ((prul)->wDelayMask & (cDelay))

// Decrement the node's delay counts (by shifting right)
#define DecrementCDelaysOfLprul(prul) \
            ((prul)->wDelayMask >>= 1)


// Return whether the (event) node is marked for history recording
#define FHistoryRecordLprul(prul) \
            (TRUE)                                      // First version
//          ((prul)->fRecordHistory)                    // Correct version


// Return whether node must check interval counts to detect seq discontinuities
#define FIntervalsSeqCheckedPrul(prul) \
            ((prul)->ipfnrulscSeqCheck)

// Return interval counts associated with node that has sequence checking
#define WIntervalsSeqCheckedPrul(prul) \
            ((*vlpruls->rgpfnrulscSeqCheck[(prul)->ipfnrulscSeqCheck])())


// Return whether the rule base is initialized
#define FRulesInited(lpruls)    (lpruls != NULL  &&  lpruls->fInited)


// Return the op-code instructions for an interpreted rule ID
#define PociiForIrul(irul) \
            PociiForPrul(LprulFromIrul(irul))

// Return the op-code instructions for an interpreted rule
#define PociiForPrul(prul) \
            ((MSOOCII *) vlpruls->rgpociiRules[((prul)->ipociiInstrs)])

// Return any group(s) that append(s) the current group
#define RulgAppendedFrom(rulg) \
            (vlpruls->prulgAppendedFrom[rulg])

// Return the list of groups that append from other groups
#define PrulgAppendedFrom() \
            (vlpruls->prulgAppendedFrom)

// Return the group (if any) that the current group appends to
#define RulgAppendTo(rulg) \
            (vlpruls->prulgAppendTo[rulg])

// Return the list of groups that append to other groups
#define PrulgAppendTo() \
            (vlpruls->prulgAppendTo)


#define rulgNil         (-1)                            // "No" rule group
#define rulevtNil       (-1)                            // "No" event type

#ifdef DEBUG
// Return whether node is marked for automatic backtracing */
#define FTraceLprul(prul) \
            ((prul)->irulNextTrace != 0)
#endif /* DEBUG */

// Return the list of nodes that current node depends upon
#define LpirulGetDependsOn(prul) \
            (&vlpruls->lpgrpirulDependBack[(prul)->birulDependsOn])



/*************************************************************************
    Prototypes and macros for rule.c
 *************************************************************************/

#ifndef max
#define max(a,b)    ((a) > (b) ? (a) : (b))
#endif /* !max */
#ifndef min
#define min(a,b)    ((a) < (b) ? (a) : (b))
#endif /* !max */


// Push the node onto the queue
#define PushLprul(prul, lplprulQ)  \
            ((prul)->prulNext = *(lplprulQ), \
             *(lplprulQ) = (prul))

// Pop the node from the queue into the variable
#define PopLprul(lplprul, lplprulQ)  \
            (*lplprul = *lplprulQ, \
             *lplprulQ = (*lplprul)->prulNext, \
             (*lplprul)->prulNext = 0)

// Push event node into Auto-Clear (Changed) list
#define PushLpRulChanged(prul) \
            if ((prul)->irulNextChanged == 0) \
                { \
                int     rulevt = RulevtOfLprul(prul); \
                \
                (prul)->irulNextChanged \
                    = vlpruls->rgirulRulevtChanged[rulevt]; \
                vlpruls->rgirulRulevtChanged[rulevt] = IrulFromLprul(prul); \
                }

// Mark event as never Auto-Clearing
#define SetNoAutoClearRulv(rulv) \
            (LprulFromRulv(rulv)->irulNextChanged = irulNoAutoClear)
#define SetNoAutoClearLprul(prul) \
            ((prul)->irulNextChanged = irulNoAutoClear)

#define irulChangedNil      -1
#define irulNoAutoClear     -2

// Return the offset of a field from the start of its typedef'd structure
// NOTE: THIS IS TRICKY CODE, BUT COMPLETELY LEGAL C!!
//       To understand it, remember that 0 is a valid pointer for ALL types!
#define CchStructOffset(type, field) \
            (((char *) (&((type *) 0)->field)) - ((char *) 0))


// Call Rule Evaluation function provided by Application
#define FEvalRule(irul) \
            (*vlpruls->lpfnEvalRule)(irul)


#ifndef STATIC_LINK_EM
MSOAPI_(RULS **) MsoPvlprulsMirror(RULS **pvlprulsApp); // Exchange &vlpruls
#else /* STATIC_LINK_EM */
#define MsoPvlprulsMirror(pvlprulsApp)  pvlprulsApp
#endif /* !STATIC_LINK_EM */
MSOAPI_(int) MsoFInitRules(                             // Init rule base
    LPFNRulinit         lpfnRulInit,
    RULS               *lpruls
    );
IRUL IrulDefineEvent(int rulevt, char *szName);         // Define simple event
MSOAPI_(MSOKWD *) MsoFDefineStringKwdEvent(             // Define str kwd event
    int                 rulevt,
    char               *szName,
    XCHAR              *pxch,
    int                 cch,
    int                 ikwtb
    );
MSOAPI_(MSOKWDLH *) MsoFDefineIntegerKwdEvent(          // Define int kwd event
    int                 rulevt,
    char               *szName,
    long                lValue,
    int                 ikwtb
    );
MSOAPI_(void) MsoClearRules(void);                      // Clear nodes & state
MSOAPI_(void) MsoClearEventsForRulevts(                 // Clr rg of ev types
    int                 rulevtFirst,
    int                 drulevtCount,
    int                 fSavePersistentDelayed,
    int                 fClearChanged,
    int                 fClearIntervalCounts
    );
MSOAPI_(void) MsoRestorePersistentDelayedRules(void);   // Restore delayed Q
MSOAPI_(int) MsoFAddPruldepDependent(                   // Add dependent link
    IRUL                irul,
    MSORUL             *prulDependent,
    int                 cDelay,
    int                 rulg
    );
MSOAPI_(void) MsoFixUpPruldeps(                         // Fix up after insert
    IRUL                irul,
    int                 rulg,
    int                 rulgBase,
    RULDEP             *lpruldepOldList,
    RULDEP             *lpruldepNewList
    );
MSOAPI_(int) MsoFDelPruldepDependent(                   // Del dependent link
    IRUL                irul,
    MSORUL             *prulDependent,
    int                 rulg,
    int                 fDiscard
    );
MSOAPI_(void) MsoSetActiveRuls(RULS *pruls);            // Set curr vlpruls
MSOAPI_(void) MsoFreeRuleMem(RULS *pruls);              // Free rule memory
#ifdef OFFICE_BUILD
MSOAPI_(void) MsoMarkRuleMem(RULS *pruls);              // Mark rule mem used
#endif /* OFFICE_BUILD */
MSOAPI_(void) MsoSetEventTypeRulevt(int rulevt);        // Change event_types
MSOAPI_(void) MsoClearChangedEventsForRulevt(int rulevt);// Clr event_type vals
void RemoveLprulChanged(MSORUL *prul);                  // Un-auto-clear event
MSOAPI_(int) MsoEvaluateEvents(int rulevt);             // Event evaluation
MSOAPI_(int) MsoFEvalIrul(IRUL irul);                   // Eval single node
MSOAPI_(void) MsoDelaySignalIrul(                       // Signal node w/delay
    IRUL                irul,
    long                lValue,
    int                 cDelay
    );
MSOAPI_(void) MsoDelaySignalIrulFrom(                   // Signal node w/delay
    IRUL                irul,
    IRUL                irulFrom,
    int                 cDelay
    );
MSOAPI_(void) MsoSignalIrul(IRUL irul, long lValue);    // Cond schedule irul
#define PushIrulToEval(irul, lValue) \
            MsoSignalIrul(irul, lValue)
MSOAPI_(void) MsoScheduleIrul(IRUL irul, long lValue);  // Schedule rule to run
#ifdef DEBUG
MSOAPI_(void) MsoScheduleIrulDebug(IRUL irul, long lValue);// Log and schedule
MSOAPI_(void) MsoScheduleIrulDebugMso(IRUL irul, long lValue);//Log &sched MSO
#else
#define MsoScheduleIrulDebug    MsoScheduleIrul
#define MsoScheduleIrulDebugMso MsoScheduleIrul
#endif /* DEBUG */

MSOAPI_(void) MsoDelayScheduleIrul(                     // Schedule after delay
    IRUL                irul,
    long                lValue,
    int                 cDelay
    );
#ifdef DEBUG
MSOAPI_(void) MsoDelayScheduleIrulDebug(                // Log and schedule
    IRUL                irul,
    long                lValue,
    int                 cDelay
    );
#else
#define MsoDelayScheduleIrulDebug   MsoDelayScheduleIrul
#endif /* DEBUG */
MSOAPI_(void) MsoDelayScheduleIrulFrom(                 // Sched, pass value
    IRUL                irul,
    IRUL                irulFrom,
    int                 cDelay
    );
MSOAPI_(int) MsoFEvalIrulImmediately(                   // Eval irul now
    IRUL                irul,
    long                lValue
    );
MSOAPI_(void) MsoPushLprulDependents(MSORUL *prul);     // Push dependents
MSOAPI_(void) MsoPushDelayedEvalForRulevt(int rulevt);  // Push delayed nodes
MSOAPI_(void) MsoAutoClearIrul(IRUL irul);              // Mark node for clear
MSOAPI_(int) MsoFAliasPrulPrul(                         // Ret if 2 are aliases
    MSORUL             *prul,
    MSORUL             *prulTarget
    );
void RecordLprulHistory(MSORUL *prul);                  // Push ev into hist
#ifdef NEVER
MSOAPI_(int) MsoFIrulHistoryValueWas(                   // Look 4 event in hist
    int                 dirultkBackwards,
    long               *lpwVar
    );
#endif // NEVER
#ifdef NEED_AS_FUNCTION
void SetCurrRulg(int rulgGroup);                        // Set rule group
#endif /* NEED_AS_FUNCTION */
MSOAPI_(void) MsoSignalEventIrul(IRUL irul, long lValue);// Signal an event
MSOAPI_(void) MsoSignalEventIrulFrom(                   // Signal ev from node
    IRUL                irul,
    IRUL                irulFrom
    );
#ifdef NEVER
MSOAPI_(void) MsoSetRuleConfid(IRUL irul, int wFactor); // Set confidence val
#endif // NEVER

// Schedule a node for deferred evaluation, based upon a decision rule
//  This macro gets invoked twice.  See _MsoFDeferIrul() for details.
#define FDeferIrul(irul) \
            (vlpruls->fEvaluatingDeferred ||  _MsoFDeferIrul(irul))

#define FDeferIrulExpr(irul, lExpr) \
            (vlpruls->fEvaluatingDeferred \
                ||  (((int) lExpr)  &&  _MsoFDeferIrul(irul)))

#define FDeferIrulExprL(irul, lExpr, lValue) \
            (vlpruls->fEvaluatingDeferred \
                ||  (((int) lExpr)  &&  _MsoFDeferIrulL((irul), (lValue))))

#define IrulPickDeferred(irulDecision) \
            MsoIrulPickDeferred(irulDecision)

MSOAPI_(int) _MsoFDeferIrul(IRUL irul);                 // Add to defer list
MSOAPI_(int) _MsoFDeferIrulL(IRUL irul, long lValue);   // Add to defer w/value
MSOAPI_(IRUL) MsoIrulPickDeferred(IRUL irulDecision);   // Pick from defer list


MSOAPI_(void) MsoSetRulNotify(                          // Set up notification
    long                lExprValue,                     // Really an int
    IRUL                irulDecision,
    IRUL                irulNotify
    );
MSOAPI_(int) MsoFRulNotify(long wNotify);               // Notify on next eval
MSOAPI_(int) MsoFRulNotifyImmediately(long wNotify);    // Notify immediately
MSOAPI_(long) MsoRulvElement(IRUL irulArray, IRUL iirul);// Get value of array
MSOAPI_(void) MsoSetElementRulv(                        // Set value of array
    IRUL                irulArray,
    IRUL                iirul,
    long                lValue
    );
MSOAPI_(void) MsoSetAllElementsToRulv(                  // Set all vals of arr
    IRUL                irulArray,
    IRUL                cirul,
    long                lValue
    );


/* M S O  F  A C T I V A T E  I R U L */
/*----------------------------------------------------------------------------
    %%Function: MsoFActivateIrul
    %%Contact: daleg

    Activate a node in the rulebase
----------------------------------------------------------------------------*/

_inline int MsoFActivateIrul(IRUL irul)                 // Activate a node
{
    MSOPRUL             prul = LprulFromIrul(irul);

    if (prul->prulNext == msoprulInactive)
        {
        prul->prulNext = (MSOPRUL) NULL;
        return TRUE;
        }
    else
        return FALSE;
}


/* M S O  F  D E A C T I V A T E  I R U L */
/*----------------------------------------------------------------------------
    %%Function: MsoFDeactivateIrul
    %%Contact: daleg

    Deactivate a node in the rulebase
----------------------------------------------------------------------------*/

MSOAPI_(int) _MsoFDeactivateIrul(IRUL irul);            // Deactivate a node

_inline int MsoFDeactivateIrul(IRUL irul)
{
    MSOPRUL             prul = LprulFromIrul(irul);

    if (prul->prulNext == NULL)
        {
        prul->prulNext = msoprulInactive;
        return TRUE;
        }
    else
        return _MsoFDeactivateIrul(irul);
}


MSOAPI_(int) MsoFDeleteIrul(IRUL irul, int rulg);       // Delete a node


/*************************************************************************
    Prototypes and macros for rultest.c and rulconcl.c.
    These prototypes "Hungarianize" the rule code so that the rule
    authors do not have to know Hungarian, but it is preserved within
    the application code.
 *************************************************************************/

// Return the current interval number for the event_type
#define CIntervalsRulevt(rulevt) \
            (vlpruls->rgdtkiRulevt[rulevt])

// OBSOLETE: Return the current interval number for the event_type
#define CIntervalsRsct(rulevt)              CIntervalsRulevt(rulevt)

// Increment the current interval number for the event_type
#define IncrIntervalsRsct(rulevt, dc) \
            (vlpruls->rgdtkiRulevt[rulevt] += (dc))

#ifdef NEVER
// Set long value of a node and force propagation
#define SetValue(wVar, lValue) \
            SignalEvent(wVar, lValue)
#endif /* NEVER */

// Convert a "variable" event reference to an node address
#define LprulFromRulv(rulvVar) \
            LprulOfWValue(&(rulvVar))

// Notify a node, on its next evaluation pass
#define FNotify(rgwVar) \
            MsoFRulNotify(rgwVar)

// Notify an action rule, immediately
#define FNotifyImmediately(rgwVar) \
            MsoFRulNotifyImmediately(rgwVar)

// Indicate the next event_type to enter when current event_type is exited
#define SetNextEventType(rulevt) \
            if (vlpruls->prulevtEvalLim \
                    < vlpruls->rgrulevtEval + RulevtMax()) \
                (*vlpruls->prulevtEvalLim++ = (rulevt)); \
            else \
                AssertSz0(FALSE, "Exceeded max number of rulevts to eval");

// Force delayed evaluation of rule of given ID: GENERATED BY RULE COMPILER
#define DelayEvalIrul(irul, cDelay) \
            MsoDelayScheduleIrulFrom((irul), (irul), (cDelay))

// Return whether the dirultkBackwards'th previous value was the given event
#define ValueWas(dirultkBackwards, wVar) \
            MsoFIrulHistoryValueWas(dirultkBackwards, &(wVar))

// Find the given value in its event_type history, and return the (neg) offset
#define FindPrevValueFrom(dirultkBackwards, wVar) \
            DirultkFindPrevValueFrom(dirultkBackwards, &(wVar))

// Mark event/rule for automatic clearing on event_type exit
#define AutoClear(irul) \
            MsoAutoClearIrul(irul)

// Mark expression as exempt from dependency linkage in rule if
#define Value(expr) (expr)

// Push all dependents of node of ID onto their evaluation queues
#define Propagate(irul) \
            MsoPushLprulDependents(LprulFromIrul((int) irul))

// Evaluate the rule at normal time
#define GoToRule(irul) \
            MsoDelayScheduleIrul((irul), TRUE, 0 /* cDelay */)

// Evaluate the rule at normal time
#define GoToIrul(irul) \
            MsoDelayScheduleIrulFrom((IRUL) (irul), (irulSelf), 0 /* cDelay */)

// Evaluate the rule after delay of 1, incrementing value
#define GoToIrulNoValue(irul) \
            MsoDelayScheduleIrulFrom \
                ((IRUL) (irul), (IRUL) (irul), 1 /* cDelay */)

// Evaluate the rule at normal time
#define GoToDirul(dirul) \
            MsoDelayScheduleIrulFrom \
                ((IRUL) (irulSelf) + (dirul), (irulSelf), 0 /* cDelay */)

// Evaluate the rule after delay of 1
#define DelayGoToRule(irul) \
            DelayGoToIrulNoValue(irul)

// Signal the event with the given value
#define SignalIrul(irul, lValue) \
            MsoSignalIrul(irul, lValue)

// Signal the event with the given value
#define SignalEvent(wVar, lValue) \
            MsoSignalEventIrul(IrulFromLprul(LprulFromRulv(wVar)), (lValue))

// Signal the event with the given value
#define SignalEventIrul(irul, lValue) \
            MsoSignalEventIrul(irul, lValue)

// Signal the node with the value from the current rule
// REVIEW: THIS IS WRONG, NOT CONDITIONALLY SCHEDULING IF EVENT
#define SignalIrulSelf(irul) \
            MsoDelayScheduleIrulFrom((IRUL) (irul), (irulSelf), 0)

// Signal the event with the value from the current rule
#define SignalEventIrulSelf(irul) \
            MsoSignalEventIrulFrom((IRUL) (irul), (irulSelf))

// Evaluate the event after delay of 1
#define DelaySignalEventIrul(irul, lValue) \
            MsoDelayScheduleIrul((irul), (lValue), 1 /* cDelay */)

// Evaluate the event after specified delay
#define DelaySignalEventIrulAfter(irul, lValue, cDelay) \
            MsoDelayScheduleIrul((irul), (lValue), 1 << ((cDelay) - 1))

// Evaluate the node after delay of 1, getting value from rule
#define DelaySignalIrulSelf(irul) \
            MsoDelayScheduleIrulFrom((irul), (irulSelf), 1 /* cDelay */)

// Evaluate the event after delay of 1, getting value from rule
#define DelaySignalEventIrulSelf(irul) \
            MsoDelayScheduleIrulFrom((irul), (irulSelf), 1 /* cDelay */)

// Evaluate the event after delay of 1
// REVIEW daleg: OBSOLETE: USE DelaySignalEventIrul or "then ... (<event>)"
#define DelaySignalIrul(irul) \
            MsoDelayScheduleIrul((irul), TRUE, 1 /* cDelay */)

// Evaluate the event after delay of 1
#define DelaySignal(rulvVar) \
            DelaySignalRulv((rulvVar), TRUE)

// Evaluate the event after delay of 1
#define DelaySignalRulv(rulvVar, lValue) \
            CDelaySignalRulv((rulvVar), (lValue), 1 /* cDelay */)

// Evaluate the event after delay of 1
#define CDelaySignalRulv(rulvVar, lValue, cDelay) \
            MsoDelayScheduleIrul(LprulFromRulv(rulvVar)->irul, (lValue), \
                                 (cDelay))

// Evaluate the rule after delay of 1 passing TRUE as the value
#define DelayGoToIrul1(irul) \
            MsoDelayScheduleIrul((irul), TRUE, 1 /* cDelay */)

// Evaluate the rule after specified delay
#define DelayGoToIrulAfter(irul, cDelay) \
            MsoDelayScheduleIrul((irul), TRUE, 1 << ((cDelay) - 1))

// Evaluate the rule with value after specified delay
#define DelayGoToIrulWithRulvAfter(irul, lValue, cDelay) \
            MsoDelayScheduleIrul((irul), (lValue) - (cDelay) + 1, \
                                 1 << ((cDelay) - 1))

// Evaluate the rule after specified delay
#define DelayGoToDirulAfter(dirul, cDelay) \
            MsoDelayScheduleIrulFrom((IRUL) (irulSelf + (dirul)), (irulSelf), \
                                     1 << ((cDelay) - 1))

// Evaluate the rule after delay of 1, incrementing value
#define DelayGoToIrulNoValue(irul) \
            MsoDelayScheduleIrulFrom \
                ((IRUL) (irul), (IRUL) (irul), 1 /* cDelay */)

// Evaluate the rule after delay of 1, passing the value of the current node
#define DelayGoToIrul(irul) \
            MsoDelayScheduleIrulFrom((IRUL) (irul), (irulSelf), 1 /* cDelay */)

// Evaluate the rule (via relative offset) after delay of 1
#define DelayGoToDirul(dirul) \
            MsoDelayScheduleIrulFrom \
                ((IRUL) (irulSelf + (dirul)), (irulSelf), 1)

// Evaluate the rule (via relative offset) after delay of 1
#define DelayGoToDirulNoValue(dirul) \
            MsoDelayScheduleIrulFrom((IRUL) (irulSelf + (dirul)), \
                                     (IRUL) (irulSelf + (dirul)), 1)

// Evaluate the rule (via relative offset) after delay of 1, with value rulv
#define DelayGoToDirulWithRulv(dirul, rulv) \
            (SetRulvOfIrul(irulSelf + (dirul), (rulv)), \
             DelayGoToDirulNoValue(dirul))

// Evaluate the rule (via relative offset) after delay of 1, with value rulv2
#define DelayGoToDirulWithRulv2(dirul, rulv2) \
            (SetRulv2OfIrul(irulSelf + (dirul), (short) (rulv2)), \
             DelayGoToDirulNoValue(dirul))

// Evaluate the rule (via relative offset) after delay of 1, with value rulv2
#define DelayGoToDirulWithRulv1(dirul, rulv1) \
            (SetRulv1OfIrul(irulSelf + (dirul), (short) (rulv1)), \
             DelayGoToDirulNoValue(dirul))

// Evaluate the rule (via relative offset) after delay of 1, with values
#define DelayGoToDirulWithRulvs(dirul, rulv1, rulv2) \
            (SetRulv1OfIrul(irulSelf + (dirul), (short) (rulv1)), \
             SetRulv2OfIrul(irulSelf + (dirul), (short) (rulv2)), \
             DelayGoToDirulNoValue(dirul))

// Deactivate curr rule if the fTest value is FALSE: used in rule test clause
#define FAutoDeactivateSelf(fTest) \
            FAutoDeactivateIrul(irulSelf, (fTest))

// Deactivate the rule if the fTest value is FALSE: used in rule test clause
#define FAutoDeactivateIrul(irul, fTest) \
            ((fTest) ? FALSE : (_MsoFDeactivateIrul(irul), TRUE))

// Set the Rule Propagation Group to the given value
#define SetCurrRulg(rulgGroup) \
            (vlpruls->rgpruldepDependents \
                = vlpruls->rgrgpruldepDependents[vlpruls->rulgCurr \
                                                        = rulgGroup])

// Return the current main Rule Propagation Group
#define FCurrRulg(rulg) \
            (vlpruls->rulgCurr == (rulg))



/*************************************************************************
    Prototypes and macros for Debugging and Error Handling
 *************************************************************************/

#ifdef DEBUG
MSOAPI_(int) MsoFTraceIrul(IRUL irul, int fTraceOn);    // Trace a node
char *SzFromFixed3(long lValue, char *pchBuf);          // Fixed to sz conv

#define DebugDumpQueues(wTraceLvl, sz, lValue, wToLevel) \
            { \
            static const unsigned char      _szDump[] = sz; \
            \
            _DumpQueues(wTraceLvl, _szDump, lValue, wToLevel); \
            }
#define DebugDumpQueue(wTraceLvl, rulevl) \
            _DumpQueue(wTraceLvl, rulevl, LplprulQueueOf(rulevl))

#else /* !DEBUG */
#define DebugDumpQueues(wTraceLvl, sz, lValue, wToLevel)
#define DebugDumpQueue(wTraceLvl, rulevl)

#endif /* DEBUG */


MSOEXTERN_C_END     // ****************** End extern "C" *********************


#define EMRULE_H

#if !(defined(OFFICE_BUILD)  ||  defined(XL))

#ifdef DYN_RULES
int FLoadDynEmRules(void);                              // Load dyn rulebase
#include "emruloci.h"
#endif /* DYN_RULES */

#endif /* !OFFICE_BUILD */

#endif /* !EMRULE_H */
