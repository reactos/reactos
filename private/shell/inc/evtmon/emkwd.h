/*****************************************************************************
    emkwd.h

    Owner: DaleG
    Copyright (c) 1992-1997 Microsoft Corporation

    Keyword Table header file.

*****************************************************************************/

#ifndef KWD_H


MSOEXTERN_C_BEGIN   // ***************** Begin extern "C" ********************


// Limits
#define msoipkwdMax         997                         // Size of keywd hash



#ifndef KWD_HASH_ONLY

// Limits...
#define msoikwdAllocMax     100                         // Max num KWDs/batch
#define msoichKwdAllocMax   100                         // Max # kwd chs/batch



/*************************************************************************
    Types:

    kwdf        Keyword-table entry record fixed.
    kwd         Keyword-table entry record.
    kwtb        Keyword table.

 *************************************************************************/


#ifndef TK_DEFINED
// Definition of token type returned by lexer
typedef int TK;

#define TK_DEFINED
#endif /* !TK_DEFINED */


/* M  S  O  K  W  D */
/*----------------------------------------------------------------------------
    %%Structure: MSOKWD
    %%Contact: daleg

    Keyword-table entry record, and batch-allocation record.
----------------------------------------------------------------------------*/

// Look up as a string key value
typedef struct _MSOKWDSZ
    {
    const XCHAR        *pxch;                           // String key value
    short               cch;                            // Length of kwd string
    TK                  tk;                             // Token value
    } MSOKWDSZ;


// Hash-table look up as a string key value
typedef struct _MSOKWD
    {
    const XCHAR        *pxch;                           // String key value
    short               cch;                            // Length of kwd string
    TK                  tk;                             // Token value
    struct _MSOKWD     *pkwdNext;                       // Next key in hash row
    } MSOKWD;


// Batch allocation
typedef struct _MSOKWDBLK
    {
    struct _MSOKWDBLK  *pkwdblkNext;                    // Next in chain
    MSOKWD             *pkwdRg;                         // Rg of MSOKWDs
    } MSOKWDBLK;


// Look up as an integer key value
typedef struct _MSOKWDL
    {
    long                lValue;                         // Integer key value
    TK                  tk;                             // Token value
    } MSOKWDL;


// Hash-table look up as a integer key value
typedef struct _MSOKWDLH
    {
    long                lValue;                         // Integer key value
    TK                  tk;                             // Token value
    struct _MSOKWDLH   *pkwdlhNext;                     // Next key in hash row
    } MSOKWDLH;




/* M  S  O  K  W  T  B */
/*----------------------------------------------------------------------------
    %%Structure: MSOKWTB
    %%Contact: daleg

    Keyword table record, and batch-allocation record.
    The hash table is in rgpkwdHash.  It is an array of linked lists,
    of type MSOKWD.
    The string buffer pxchBuf contains the character strings, PLUS the
    MSOKWD records of the hash table at the end.

    REVIEW: rgpkwdHash, pxchBuf and pkwdStart should be moved to the end of
    this struct to avoid allocating excess space for non-hashed keytables.
    Mso*Lookup* routines should assert that input MSOKWTB* arguments are of
    suitable type.
----------------------------------------------------------------------------*/

typedef struct _MSOKWTB
    {
    // Table data
    int                 kwtbt;                          // Keytable type
    int                 ckwdMax;                        // Num of MSOKWD recs
    MSOKWD             *pkwdUnknown;                    // Returned when !found
    void               *pvTable;                        // Hash/Binary table

    // Allocation/Init info
    unsigned char       fStaticInit;                    // Static Init-ed?
    unsigned char       fDynAlloced : 1;                // MSOKWTB mem alloced?
    unsigned char       fHashTblAlloced : 1;            // pvTable mem alloced?
    unsigned char       fSpare : 1;

    // String support
    XCHAR              *pxchBuf;                        // String buffer
    MSOKWD             *pkwdStart;                      // First MSOKWD rec

    // Extensions
    int                 cxchKwdBufRemain;               // Num avail chars
    XCHAR              *pxchKwdNext;                    // Next avail string
    MSOKWD             *pkwdNextFree;                   // Next av MSOKWD rec
    struct _MSOKWDBLK  *pkwdblkNext;                    // List of extra KWDs
    struct _MSOKWSBLK  *pkwsblkNext;                    // List of extra bufs
    } MSOKWTB;


#define msokwtbtString          0x00
#define msokwtbtInteger         0x01
#define msokwtbtHashed          0x02
#define msokwtbtStringHashed    0x02
#define msokwtbtIntegerHashed   0x03


// String buffer batch allocation
typedef struct _MSOKWSBLK
    {
    struct _MSOKWSBLK  *pkwsblkNext;                    // Next in chain
    XCHAR              *pxchBuf;                        // String buffer
    } MSOKWSBLK;


/*************************************************************************
    STRING LOOKUP
 *************************************************************************/

// Lookup keyword
#define MsoTkLookupName(pxch, cchLen, pkwtb) \
            (MsoPkwdLookupName((pxch), (cchLen), (pkwtb))->tk)

MSOAPI_(MSOKWD *) MsoPkwdLookupName(                    // Lookup keyword
    const XCHAR        *pxchStr,
    int                 cchLen,
    MSOKWTB            *pkwtb
    );

MSOAPI_(MSOKWD *) MsoPkwdAddTkLookupName(               // Add kwd to table
    const XCHAR        *pxchStr,
    int                 cchLen,
    TK                  tk,
    MSOKWTB            *pkwtb,
    int                 fCopyStr
    );

MSOAPI_(int) MsoFRemoveTkLookupName(                    // Remove kwd from tbl
    const XCHAR        *pxchStr,
    int                 cchLen,
    MSOKWTB            *pkwtb,
    TK                 *ptk                             // RETURN
    );

MSOAPI_(void) MsoInitHashKwtb(MSOKWTB *pkwtb);          // Init a kwd hash tbl

MSOAPI_(MSOKWD *) _MsoPkwdNew(                          // Batch-alloc MSOKWDs
    int                 ikwdMax,
    MSOKWTB            *pkwtb
    );

int _FAddKwdRgchBuf(int ixchMax, MSOKWTB *pkwtb);       // Batch-alloc kwd stzs

MSOAPI_(MSOKWTB *) MsoPkwtbNew(void);                   // Create new kwd tbl

#ifdef DEBUG
void _DumpKwds(MSOKWTB *pkwtb);                         // Print kwd table
#endif /* DEBUG */


// Get a new MSOKWD record from free list
_inline MSOKWD *MsoPkwdNew(MSOKWTB *pkwtb)
{
    MSOKWD             *pkwd;

    return ((pkwtb)->pkwdNextFree
                ? (pkwd = (pkwtb)->pkwdNextFree,
                        (pkwtb)->pkwdNextFree = pkwd->pkwdNext,
                        pkwd->pkwdNext = (MSOKWD *) NULL,
                        pkwd)
                : _MsoPkwdNew(msoikwdAllocMax, (pkwtb)));
}

// Put an existing MSOKWD record onto the free list
#define MsoDiscardPkwd(pkwd, pkwtb) \
            ((pkwd)->pkwdNext = (pkwtb)->pkwdNextFree, \
                (pkwtb)->pkwdNextFree = (pkwd))

// Return address of first MSOKWD record for hash value in keyword lookup table
#define MsoPpkwdGetHashAddr(pkwtb, ikwd) \
            (&(MsoRgpkwdHashFromKwtb(pkwtb)[ikwd]))

// Return hash table from keytable
#define MsoRgpkwdHashFromKwtb(pkwtb) \
            ((MSOKWD **) (pkwtb)->pvTable)

// Return integer binary table from keytable
#define MsoPkwdlFromKwtb(pkwtb) \
            ((MSOKWDL *) (pkwtb)->pvTable)






/*************************************************************************
    INTEGER LOOKUP
 *************************************************************************/

// REVIEW: references to this function should become references to
// MsoPkwdlLookupL with an MSOKWTB
MSOAPI_(int) MsoWLookupKwdl(                            // Binary search lookup
    long                lValue,
    MSOKWDL const      *pkwdlTbl,
    int                 ikwdlMac
    );

MSOAPI_(MSOKWDL *) MsoPkwdlLookupL(                     // Nohash-lookup a long
    long                lValue,
    MSOKWTB            *pkwtb
    );

#ifdef DEBUG
#define MsoAssertKwtb(pkwtb) \
    MsoAssertKwtbSz(pkwtb, #pkwtb)
MSOAPI_(void) MsoAssertKwtbSz(MSOKWTB *pkwtb, char *szName); // Ensure sorted
#else
#define MsoAssertKwtb(pkwtb)
#endif // DEBUG

MSOAPI_(MSOKWDLH *) MsoPkwdlhLookupL(                   // Hash-lookup a long
    long                lValue,
    MSOKWTB            *pkwtb
    );

MSOAPI_(MSOKWDLH *) MsoPkwdlhAddTkLookupL(              // Add int hash lookup
    long                lValue,
    TK                  tk,
    MSOKWTB            *pkwtb
    );

MSOAPI_(MSOKWDLH *) _MsoPkwdlhNew(int ikwdMax, MSOKWTB *pkwtb);

// Return addr of first MSOKWDLH record for hash value in keyword lookup table
#define MsoPpkwdlhGetHashAddr(pkwtb, ikwd) \
            (&(MsoRgpkwdlhFromKwtb(pkwtb)[ikwd]))

// Return hash table from keytable
#define MsoRgpkwdlhFromKwtb(pkwtb) \
            ((MSOKWDLH **) (pkwtb)->pvTable)


// Get a new MSOKWDLH record from free list
_inline MSOKWDLH *MsoPkwdlhNew(MSOKWTB *pkwtb)
{
    MSOKWDLH           *pkwdlh;

    if (pkwtb->pkwdNextFree)
        {
        pkwdlh = (MSOKWDLH *) (pkwtb)->pkwdNextFree;
        pkwtb->pkwdNextFree = (MSOKWD *) pkwdlh->pkwdlhNext;
        pkwdlh->pkwdlhNext = (MSOKWDLH *) NULL;
        return pkwdlh;
        }
    else
        return _MsoPkwdlhNew(msoikwdAllocMax, pkwtb);
}

// Put an existing MSOKWDLH record onto the free list
#define MsoDiscardPkwdlh(pkwdlh, pkwtb) \
            ((pkwdlh)->pkwdlhNext = (MSOKWDLH *) (pkwtb)->pkwdNextFree, \
                (pkwtb)->pkwdNextFree = (MSOKWD *) (pkwdlh))


#endif /* !KWD_HASH_ONLY */


#ifndef ANSI_XCHAR
#define MsoIpkwdHashPxch(pch, cch)          MsoIpkwdHashPwch(pch, cch)
#define MsoXchUpper MsoWchToUpper
#else /* ANSI_XCHAR */
#define MsoIpkwdHashPxch(pch, cch)          MsoIpkwdHashPch(pch, cch)
#define MsoXchUpper MsoChToUpper
#endif /* !ANSI_XCHAR */



/* M S O  C H  T O  U P P E R */
/*----------------------------------------------------------------------------
    %%Function: MsoChToUpper
    %%Contact: daleg

    Convert ANSI character to uppercase.
----------------------------------------------------------------------------*/

_inline unsigned char MsoChToUpper(unsigned char ch)
{
    return (islower(ch) ? (unsigned char) toupper(ch) : ch);
}


/* M S O  I P K W D  H A S H  P C H */
/*----------------------------------------------------------------------------
    %%Function: MsoIpkwdHashPch
    %%Contact: daleg

    Return a hash index for the given string and length, case insensitive.
----------------------------------------------------------------------------*/

_inline int MsoIpkwdHashPch(const unsigned char *pchStr, int cchLen)
{
    /* Hash on first and last character of string, ignoring case */
    return ((MsoChToUpper(*pchStr) * 419)
                + (MsoChToUpper(*(pchStr + (cchLen - 1)/2)) * 467)
                + (MsoChToUpper(*(pchStr + cchLen - 1)) * 359))
            % msoipkwdMax;
}


/* M S O  I P K W D  H A S H  P W C H */
/*----------------------------------------------------------------------------
    %%Function: MsoIpkwdHashPwch
    %%Contact: daleg

    Return a hash index for the given string and length, case insensitive.
----------------------------------------------------------------------------*/

_inline int MsoIpkwdHashPwch(const WCHAR *pwchStr, int cchLen)
{
    /* Hash on first and last character of string, ignoring case */
    return ((MsoWchToUpper(*pwchStr) * 419)
                + (MsoWchToUpper(*(pwchStr + (cchLen - 1)/2)) * 467)
                + (MsoWchToUpper(*(pwchStr + cchLen - 1)) * 359))
            % msoipkwdMax;
}


/* M S O  I P K W D L H  H A S H */
/*----------------------------------------------------------------------------
    %%Function: MsoIpkwdlhHash
    %%Contact: daleg

    Return a hash index for the given integer value.
----------------------------------------------------------------------------*/

_inline int MsoIpkwdlhHash(long lValue)
{
    return (((lValue + (lValue >> 1)) & ~0x80000000) % msoipkwdMax);
}


MSOEXTERN_C_END     // ****************** End extern "C" *********************

#define KWD_H
#endif /* !KWD_H */
