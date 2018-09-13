/*****************************************************************************
    emrultk.h

    Owner: DaleG
    Copyright (c) 1992-1997 Microsoft Corporation

    Rule and Lexer-token history recording header file.

*****************************************************************************/

#ifndef EMRULTK_H
#define EMRULTK_H

MSOEXTERN_C_BEGIN   // ***************** Begin extern "C" ********************

#ifndef MSOCP_DEFINED
typedef long MSOCP;                                     // Character position
#define msocpNil ((MSOCP) -1)
#define msocp0 ((MSOCP) 0)
#define msocpMax ((MSOCP) 0x7FFFFFFF)
#define MSOCP_DEFINED
#endif /* !MSOCP_DEFINED */


/*************************************************************************
    Types:

    rultk       Rule/lexer-token history record.
    rultkh      Rule/lexer-token history cache.

 *************************************************************************/


/* M  S  O  R  U  L  T  K */
/*----------------------------------------------------------------------------
    %%Structure: MSORULTK
    %%Contact: daleg

    Rule/lexer-token history record.
----------------------------------------------------------------------------*/

typedef struct _MSORULTK
    {
    void               *pObject;                        // Object, e.g. doc
    MSOCP               cpFirst;                        // First CP of token
    MSOCP               dcp;                            // CP len of token
    int                 ich;                            // Char offset of tk
    long                dich;                           // Num of chars of tk
    long                wInterval;                      // Num intervals
    int                 tk;                             // Token number
    long                lValue;                         // Token value
    } MSORULTK;


#define irultkNil   (-2)


/* M  S  O  R  U  L  T  K  H */
/*----------------------------------------------------------------------------
    %%Structure: MSORULTKH
    %%Contact: daleg

    Rule/lexer-token history cache structure.
----------------------------------------------------------------------------*/

typedef struct _MSORULTKH
    {
    MSORULTK           *rgrultkCache;                   // History record list
    int                 irultkMac;                      // Lim of allocated
    int                 irultkMin;                      // User-defined marker
    int                 irultkLim;                      // Next cache index
    int                 irultkAbsBase;                  // Absolute irultk base
    } MSORULTKH;


/* M  S  O  C  A */
/*----------------------------------------------------------------------------
    %%Structure: MSOCA
    %%Contact: daleg

    Rule/lexer text range
----------------------------------------------------------------------------*/

typedef struct _MSOCA
    {
    void               *pObject;                        // Object, e.g. doc
    long                cpFirst;                        // First char pos
    long                cpLim;                          // Lim char pos
    } MSOCA;


// Define (D)elta of a (T)o(K)en cache index
typedef int DTK;

// Define (D)elta of a (T)o(K)en cache (I)nterval
typedef int DTKI;


// Token-history cache access
#define PrultkFromIrultk(irultk, prultk) \
            (&prultk[irultk])

// Move to next cache record, wrapping around if necessary
#define IncrPrultk(pprultk, pirultk, prultkHist, irultkMax) \
            if (++(*pirultk) >= (irultkMax)) \
                { \
                (*pirultk) = 0; \
                (*pprultk) = PrultkFromIrultk(0, prultkHist); \
                } \
            else \
                (*pprultk)++;

// Move to next cache record, wrapping around if necessary
#define DecrPrultk(pprultk, pirultk, prultkHist, irultkMax) \
            if (--(*pirultk) < 0) \
                { \
                (*pirultk) += (irultkMax); \
                (*pprultk) += (irultkMax) - 1; \
                } \
            else \
                (*pprultk)--;

// Move index to next cache record, wrapping around if necessary
#define IncrPirultk(pirultk, dirultk, irultkMax) \
            if (((*pirultk) += dirultk) >= (irultkMax)) \
                (*pirultk) -= (irultkMax);

// Move index to prev cache record, wrapping around if necessary
#define DecrPirultk(pirultk, dirultk, irultkMax) \
            if (((*pirultk) -= dirultk) < 0) \
                (*pirultk) += (irultkMax);

MSORULTK *PrultkExpand(                                 // Expand cache
    MSORULTK          **pprultkCache,
    int                *pirultkMax,
    int                 irultkMax,
    int                 irultkSplit
    );

// defines
#define msodtkNotFound      30000                       // arbitrarily lg. flag


MSOEXTERN_C_END     // ****************** End extern "C" *********************

#endif /* EMRULTK_H */
