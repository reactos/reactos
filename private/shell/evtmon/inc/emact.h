/*****************************************************************************
    emact.h

    Owner: DaleG
    Copyright (c) 1997 Microsoft Corporation

    Delayed Action Interpreter mechanism, shared header file.

*****************************************************************************/

#ifndef _EMACT_H
#define _EMACT_H

#include "emrultk.h"


MSOEXTERN_C_BEGIN   // ***************** Begin extern "C" ********************

#define iactAllocMax        256


/* M  S  O  A  C  T */
/*----------------------------------------------------------------------------
    %%Structure: MSOACT
    %%Contact:   daleg

    Delayed-Action structure.  Used to pass argument values to functions
    delay-called from the rulebase.
----------------------------------------------------------------------------*/

typedef struct _MSOACT
    {
    union
        {
        struct
            {
            short           actt;                       // Action type
            MSOBF           fComposite : 1;             // Composite action?
            MSOBF           fValidate : 1;              // Check if act OK?
            MSOBF           fAdjust : 1;                // Adjusting CA?
            MSOBF           fDeferred : 1;              // Deferred eval?
            long            rgl[8];                     // Arg values, 1st rec
            struct _MSOACT *pactSublist;                // Child record
            } rec1;
        struct
            {
            long            hdrCA;                      // Shared rec1 header
            MSOCA           ca;                         // Edit range
            } rec1_ca;
        long                hdr;                        // Shared header
        long                rglSublist[10];             // Arg values, 2nd+ rec
        char               *rszSublist[10];             // Arg values, 2nd+ rec
        };
    struct _MSOACT         *pactNext;                   // Next record
    } MSOACT;


#define msoacttNil          (-1)                        // Out of range actt
#define msopactNULL         ((MSOACT *) (-1))           // End of list marker


// Return object pointer of (first record of) a pact
#define PObjectPact(pact) \
            ((pact)->rec1_ca.ca.pObject)

// Return cpFirst of (first record of) a pact
#define CpFirstPact(pact) \
            ((pact)->rec1_ca.ca.cpFirst)

// Return cpLim of (first record of) a pact
#define CpLimPact(pact) \
            ((pact)->rec1_ca.ca.cpLim)



/* M S O  A  C  T  B  L  K */
/*----------------------------------------------------------------------------
    %%Function: MSOACTBLK
    %%Contact: daleg

    Delayed-Action structure allocation block.
----------------------------------------------------------------------------*/

typedef struct _MSOACTBLK
    {
    struct _MSOACTBLK  *pactblkNext;                    // Next alloc block
    MSOACT              rgact[iactAllocMax];            // Array of MSOACTs
    } MSOACTBLK;



/* M  S  O  A  C  T  T  R  E  C */
/*----------------------------------------------------------------------------
    %%Function: MSOACTTREC
    %%Contact: daleg

    Action-type table record.  Holds flags associated with actt values.
----------------------------------------------------------------------------*/

typedef struct _MSOACTTREC
    {
    short               cargs;                          // Num of args to fn
    unsigned short      actf;                           // Action flags
    } MSOACTTREC;


// Base action flags
#define msoactfExclusiveEdit    0x0001                  // Truncates other acts
#define msoactfTruncatable      0x0002                  // Trucated by Excl act
#define msoactfNonExclPaired    0x0004                  // Trunc unless paired

// "User"-level action flags
#define msoactfNonEdit          0x0000                  // Not an edit
#define msoactfEdit             0x0003                  // e.g. Edits
#define msoactfProp             0x0002                  // e.g. Props
#define msoactfPairedProp       0x0004                  // Props paired w/edits
#define msoactfOverlapCalc      0x0007                  // Overlap calc necess
#define msoactfCond             0x0010                  // Cond exec next act
#define msoactfConposite        0x0020                  // Composite action

// Return whether the action is a composite, composed of multiple records
#define MsoFCompositeAct(actt, pacttbl) \
            MsoFActfActt(actt, msoactfConposite, pacttbl)

// Return whether ACT record has actf flag set
#define MsoFActfActt(actt, actfMask, pacttbl) \
            (MsoActfActt((actt), (pacttbl)) & (actfMask))

// Return whether ACT record has actf flag set
#define MsoFActfIs(actf, actfMask) \
            ((actf) & (actfMask))

// Return whether ACT record type has actf flag set
#define MsoFActfActtIs(actt, actfMatch, pacttbl) \
            (MsoActfActt((actt), (pacttbl)) == (actfMatch))

// Return whether ACT record type has actf flag set
#define MsoActfActt(actt, pacttbl) \
            (MsoPActtrec((actt), (pacttbl))->actf)

// Return action record (MSOACTTREC) associated with ACT record
#define MsoPActtrec(actt, pacttbl) \
            (&(pacttbl)->rgacttrec[actt])



/* M  S  O  A  C  T  T  B  L */
/*----------------------------------------------------------------------------
    %%Structure: MSOACTTBL
    %%Contact: daleg

    Action-type table.  Contains MSOACT state and action flags.
----------------------------------------------------------------------------*/

typedef struct _MSOACTTBL
    {
    const MSOACTTREC   *rgacttrec;                      // Per-action data
    MSOCP               cpFirstEditPrev;                // 1st CP of last edit
    MSOCP               dcpEditPrev;                    // dCP of last edit
    MSOCP               cpLimEdit;                      // cpLim, edit so far
    MSORULTKH          *prultkh;                        // Text Token cache
    MSOACT             *pactPending;                    // Pending actions
    MSOACT             *pactPendingPrev;                // Prev pending acts
    MSOACT             *pactFree;                       // Free list
    MSOACTBLK          *pactblkAlloc;                   // Allocation list
    } MSOACTTBL;



// Start a new MSOACT action list frame
#define MsoPushPactPending() \
            MsoBeginActSublist((_pacttbl), &(_pacttbl)->pactPendingPrev)


// End and close a new MSOACT sub-list, and return sublist
#define MsoPopPactPending(ppact) \
            (*(ppact) = (_pacttbl)->pactPending, \
             MsoEndActSublist((_pacttbl), &(_pacttbl)->pactPendingPrev))


// Start a new MSOACT action list frame
__inline void MsoBeginActSublist(MSOACTTBL *pacttbl, MSOACT **ppactPrev)
{
    *ppactPrev = pacttbl->pactPending;
    pacttbl->pactPending = (MSOACT *) NULL;
}


// End and close a new MSOACT sub-list
__inline void MsoEndActSublist(MSOACTTBL *pacttbl, MSOACT **ppactPrev)
{
    pacttbl->pactPending = *ppactPrev;
    *ppactPrev = (MSOACT *) NULL;
}



// Callback to evaluate the MSOACT
typedef long (WIN_CALLBACK * MSOPFNACT)(
    MSOACT             *pact,
    MSOACTTBL          *pacttbl,
    long               *pdcp,                           // IN, RETURN
    MSOCA              *pcaAdjusted,
    MSOACT           **ppactNext,                       // RETURN
    int                *pfDiscard                       // RETURN
    );


// Are there pending action records?
#define MsoPendingActions(pacttbl) \
            ((pacttbl)->pactPending)

// Return value for args 0-7
#define MsoLPact(pact, iarg) \
            ((pact)->rec1.rgl[iarg])

// Return value for args 8-17
#define MsoLPact2(pact, iarg) \
            ((pact)->rec1.pactSublist->rglSublist[(iarg) - 8])

// Return value for args 18-27
#define MsoLPact3(pact, iarg) \
            ((pact)->rec1.pactSublist-> \
            ((pact)->rec1.pactSublist->pactNext->rglSublist[(iarg) - 18])

// Return value for args 28-31
#define MsoLPact4(pact, iarg) \
            ((pact)->rec1.pactSublist->pactNext->pactNext \
                ->rglSublist[(iarg) - 28])


MSOCDECLAPI_(MSOACT *) MsoPact(                         // Build new MSOACT rec
    MSOACTTBL          *pacttbl,
    int actt,
    ...
    );
MSOCDECLAPI_(MSOACT *) MsoPactNq(                       // As above, not queued
    MSOACTTBL          *pacttbl,
    int actt,
    ...
    );
MSOCDECLAPI_(MSOACT *) MsoPactDtk(                      // Bld MSOACT rec w/tks
    MSOACTTBL          *pacttbl,
    int                 actt,
    int                 dtkStart,
    int                 dtk,
    ...
    );
MSOCDECLAPI_(MSOACT *) MsoPactDtkNq(                    // As above, not queued
    MSOACTTBL          *pacttbl,
    int                 actt,
    int                 dtkStart,
    int                 dtk,
    ...
    );
MSOCDECLAPI_(MSOACT *) MsoPactPca(                      // Bld MSOACT rec w/CPs
    MSOACTTBL          *pacttbl,
    int                 actt,
    MSOCA              *pca,
    ...
    );
MSOCDECLAPI_(MSOACT *) MsoPactPcaNq(                    // As above, not queued
    MSOACTTBL          *pacttbl,
    int                 actt,
    MSOCA              *pca,
    ...
    );
MSOAPI_(MSOACT *) MsoPactCompositeDtk(                  // Bld Composite action
    MSOACTTBL          *pacttbl,
    int                 actt,
    int                 dtkStart,
    int                 dtk,
    MSOACT            **ppactPrev
    );
void SetPactCaFromSublist(MSOACT *pact);                // Set CA of Comp pact
#define MsoReversePactPending(pacttbl) \
            MsoReversePact(&pacttbl->pactPending)       // Reverse pending acts
MSOAPI_(void) MsoReversePact(MSOACT **ppact);           // Reverse ACT list
MSOAPI_(long) MsoDcpDoActs(                             // Execute ACT list
    MSOACT            **ppact,
    MSOACTTBL          *pacttbl,
    long                dcp,
    int                 fDiscardActs,
    int                 dactLim,
    MSOPFNACT           pfnact                          // MSOACT handler
    );
MSOAPI_(MSOACT *) MsoPactDtkAp(                         // Bld MSOACT tks & ap
    MSOACTTBL          *pacttbl,
    int                 actt,
    int                 dtkStart,
    int                 dtk,
    va_list             ap
    );
MSOAPI_(MSOACT *) MsoPactPcaAp(                         // Bld MSOACT CPs & ap
    MSOACTTBL          *pacttbl,
    int                 actt,
    MSOCA               *pca,
    va_list             ap
    );
MSOAPI_(MSOACT *) MsoPactAp(                            // Bld MSOACT rec ap
    MSOACTTBL          *pacttbl,
    int                 actt,
    int                 cargsOffset,
    va_list             ap
    );
MSOAPI_(void) MsoInsertPact(                            // Insert ACT by MSOCA
    MSOACT             *pact,
    MSOACT            **ppactHead
    );
MSOAPI_(int) MsoFEnsurePactFirst(                       // Sort ACT 1st in rng
    MSOACT             *pact,
    MSOACTTBL          *pacttbl
    );
MSOAPI_(MSOACT *) MsoFindPactOfActt(                    // Find rec by actt
    short               actt,
    MSOACT             *pact,
    MSOCP               cpFirst,
    MSOCP               cpLim
    );
MSOAPI_(void) MsoDeletePact(                            // Remove & free act
    MSOACT             *pact,
    MSOACTTBL          *pacttbl
    );
MSOAPI_(void) MsoSkipNextPact(                          // Skip over next ACT
    MSOACT            **ppactNext,
    MSOACTTBL          *pacttbl
    );
MSOAPI_(MSOACT *) _MsoPactNew(MSOACTTBL *pacttbl);      // Create blk of MSOACT

_inline MSOACT *MsoPactNew(MSOACTTBL *pacttbl)          // Return new MSOACT
{
    MSOACT             *pact;

    if ((pact = pacttbl->pactFree))
        {
        pacttbl->pactFree = pact->pactNext;
        return pact;
        }
    return _MsoPactNew(pacttbl);
}

void ClearPactPending(MSOACTTBL *pacttbl);              // Free pending acts
MSOAPI_(void) MsoFreePact(                              // Free MSOACT rec
    MSOACT             *pact,
    MSOACTTBL          *pacttbl
    );

_inline int MsoFAllocPact(MSOACTTBL *pacttbl)           // Pre-alloc ACT list
{
    MSOACT             *pact;

    if ((pact = MsoPactNew(pacttbl)) == NULL)
        return FALSE;
    MsoFreePact(pact, pacttbl);
    return TRUE;
}

MSOAPI_(void) MsoFreeActMem(MSOACTTBL *pacttbl);        // Free act mem used
#ifdef DEBUG
MSOAPI_(void) MsoMarkActMem(MSOACTTBL *pacttbl);        // Mark act mem used
#endif // DEBUG

MSOEXTERN_C_END     // ****************** End extern "C" *********************

#endif /* !_EMACT_H */
