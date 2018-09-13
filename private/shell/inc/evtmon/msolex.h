/******************************************************************************
    MSOLEX.H

    Owner: smueller
    Copyright (c) 1997 Microsoft Corporation

    General Purpose Text Lexer definitions and prototypes

    There are currently multiple instances of this component in Office 9.
    Keep them in sync:
        %mso%\inc\msolex.h
        %otools%\src\em\emtest\msolex.h
        %ppt%\office\lexpp.h
        %word%\src\inc\lex.h

    FUTURE: some of these definitions don't need to be exported and could
    live in an msolex.i file.
******************************************************************************/

#ifndef MSOLEX_H
#define MSOLEX_H


/*----------------------------------------------------------------------------
    Enabled features
----------------------------------------------------------------------------*/

#define EM_MULT_DIM_SCAN


/*----------------------------------------------------------------------------
    System limits
----------------------------------------------------------------------------*/

#define ichTkLenMax         256                         // Max token str len
#define ichLexsCacheMax     (ichTkLenMax - 1)           // Mx num chars cached

#define dirultkNotFound     30000                       // arbitrarily lg. flag
#define dtkNotFound         dirultkNotFound             // alias


/*************************************************************************
    Types:

    tk          Token returned by the lexer.
    lexs        Lexer state structure.

 *************************************************************************/


// Define state tables used by lexer (use with plexs->isttbl)
#define isttblDefault   0                               // Default MUST be 0
#define isttblNoBlanks  1


/* T K */
/*----------------------------------------------------------------------------
    %%Structure: TK
    %%Contact: daleg

    Lexer token definitions.
----------------------------------------------------------------------------*/

#ifndef TK_DEFINED
// Definition of token type returned by lexer
typedef int TK;

#define TK_DEFINED
#endif /* !TK_DEFINED */


// Lexer tokens: Plain Text and Delimiters
#define tk_RESCAN_      (-2)                            // Dummy: force rescan
#define tkERROR         (-1)                            // Lexer error
#define tkNil           0                               // No token at all
#define tkEND_OBJ       1                               // End of object
#define tkPARA          2                               // 0xB6 (Para mark)
#define tkNEWLINE       3                               // \n
#define tkWSPC          4                               // Blanks, tabs
#define tkWSPCMULT      5                               // Multiple Blanks
#define tkTAB           6                               // Tab character
#define tkWORD          7                               // E.g. abc
#define tkINTEGER       8                               // E.g. 123
#define tkCOMMA         9                               // ,
#define tkPERIOD        10                              // .
#define tkEXCLAIM       11                              // !
#define tkPOUND         12                              // #
#define tkDOLLAR        13                              // $
#define tkPERCENT       14                              // %
#define tkAMPER         15                              // &
#define tkLPAREN        16                              // (
#define tkRPAREN        17                              // )
#define tkASTER         18                              // *
#define tkPLUS          19                              // +
#define tkMINUS         20                              // -
#define tkSLASH         21                              // /
#define tkCOLON         22                              // :
#define tkSEMI          23                              // ;
#define tkLESSTHAN      24                              // <
#define tkEQUAL         25                              // =
#define tkGREATER       26                              // >
#define tkQUEST         27                              // ?
#define tkATSIGN        28                              // @
#define tkLBRACK        29                              // [
#define tkRBRACK        30                              // ]
#define tkBSLASH        31                              // \ 
#define tkCARET         32                              // ^
#define tkUSCORE        33                              // _
#define tkBQUOTE        34                              // `
#define tkLBRACE        35                              // {
#define tkRBRACE        36                              // }
#define tkVBAR          37                              // |
#define tkTILDA         38                              // ~
#define tkDQUOTE        39                              // "
#define tkLDQUOTE       40                              // " left curly dbl
#define tkRDQUOTE       41                              // " right curly dbl
#define tkQUOTE         42                              // '
#define tkLQUOTE        43                              // ' left curly sgl
#define tkRQUOTE        44                              // ' right curly sgl
#define tkLCHEVRON      45                              // << French LDQuote
#define tkRCHEVRON      46                              // >> French RDQuote
#define tkENDASH        47                              // - en-dash
#define tkEMDASH        48                              // -- em-dash

// Lexer tokens: Plain-Text Symbol tokens
#define tkSYMBOL        49                              // Symbol char
#define tkBULLET        50                              // Std bullet char
#define tkFEWORD        51                              // FE word
#define tkFESYMBOL      52                              // FE symbol char
#define tkFESPACE       53                              // FE Space char

// Lexer tokens: Plain-text formatting info
#define tkSTARTCAP      54                              // Word is capitalized
#define tkALLCAPS       55                              // Word is all caps
#define tkHASCAPS       56                              // Word has 1+ CAPs


// Is TK valid (not tkNil and not tkERROR)
#define FValidTk(tk) \
            ((tk) > 0)



/* M  S  O  L  E  X  S */
/*----------------------------------------------------------------------------
    %%Structure: MSOLEXS
    %%Contact: daleg

    AutoFormat LEX State

    Contains information about the Event Monitor lexer's current position
    in the document.

    This information is initialized by LexReset(), and advanced by other
    lexer APIs.
----------------------------------------------------------------------------*/

// Callback typedefs
typedef XCHAR (OFC_CALLBACK *PFNLEXBUF)(MSOCP cpLim, struct _MSOLEXS *plexs);
typedef int (OFC_CALLBACK *PFNLEXRUN)(MSOCP cpLim, struct _MSOLEXS *plexs);
typedef int (OFC_CALLBACK *PFNLEXTXT)
    (MSORULTK *prultk, const XCHAR **ppxch, int *pcch, struct _MSOLEXS *plexs);
typedef void (OFC_CALLBACK *PFNLEXFMT)
    (int *pfForceTkBreak, struct _MSOLEXS *plexs);
typedef int (OFC_CALLBACK *PFNLEXCNT)(struct _MSOLEXS *plexs);


typedef struct _MSOLEXS
    {
    // --- values requiring initialization ---

    // Keyword lookup information
    struct _MSOKWTB    *pkwtb;                          // Keyword-lookup tbl

    // Token-history cache information
    MSORULTKH           rultkhToken;                    // Text Token cache

    // Formatting token-history cache information
    MSORULTKH           rultkhFormat;                   // Format Token cache

    // Init state
    MSOBF               fInited : 1;                    // Lexer inited?
    int                 isttbl;                         // Which STT?
    union
        {
        unsigned short  grpfLexFlags;
        struct
            {
            MSOBF       fNoReset : 1;                   // Reset leave alone
            MSOBF       fLookup : 1;                    // Lookup name as kwd?
            MSOBF       fLookupIntsAndSyms : 1;         // Lookup ints as kwd?
            MSOBF       fAllCapsAsFormat : 1;           // ALLCAPS as fmt tk?
            MSOBF       fRefetchOnOverscan : 1;         // Force fetch on OS?
            MSOBF       fSpare2 : 11;
            };
        };

    // Buffer management callback functions
    void               *hObjectNil;                     // Nil object
    PFNLEXBUF           pfnlexbuf;                      // Fetch next buffer
    PFNLEXRUN           pfnlexrun;                      // Fetch next run
    PFNLEXTXT           pfnlextxt;                      // Fetch token text
    PFNLEXFMT           pfnlexfmt;                      // Gen format tokens
    PFNLEXCNT           pfnlexrunDiscontig;             // Next run contiguous?
    PFNLEXRUN           pfnlexrunForceComplete;         // Force tk to complete

    // Run state information
    int                 ichRun;                         // Index to vfli.rgch
    int                 cchLookahead;                   // Num chars lookahead

    // --- values initially zero ---

    // Run state information
    int                 cchRemain;                      // Num chars unlexed
    MSOCP               cpRun;                          // CP of start of run
    MSOCP               ccpRun;                         // Num of CPs in run
    MSOCP               cpObject;                       // CP of start of obj
    int                 cchRun;                         // Num chars run

    // Token state information
    MSOCP               cpTokenFirst;                   // CP of first char
    MSOCP               dcpToken;                       // Num CPs in token
    MSOCP               cpTokenNext;                    // CP of next token
    int                 tkTokenIndirect;                // Indirect token
    int                 ichTokenFirst;                  // ich of first char
    const XCHAR        *pxchTkStart;                    // First char in token
    const XCHAR        *pxchNext;                       // Next char to lex
    const XCHAR        *pxchRun;                        // First char of run
    const XCHAR        *pxchBuffer;                     // Token string buffer
    const XCHAR        *pxchBufferIp;                   // Buffer of obj at IP
    union
        {
        unsigned short  grfCurTk;
        struct
            {
            MSOBF       fMustSyncLexDocBuffer : 1;      // Reset lexer?
            };
        };

    // Vanished/Created text handling
#ifdef EM_LEX_VANISHED
    MSOCP               cpFirstVanished;                // CP of vanished
    MSOCP               dcpVanished;                    // dcp of vanished txt
#endif /* EM_LEX_VANISHED */
    MSOCP               cpFirstCreated;                 // CP of created
    MSOCP               dcpCreated;                     // dcp of created txt
    union
        {
        unsigned short  grpfLineFlags;
        struct
            {
            MSOBF       fAdjustTokenCps : 1;            // Created/Vanished txt
            };
        };

    // Lexer state information
    int                 ichCache;                       // Num chars cached
    XCHAR               rgxchCache[ichTkLenMax];        // Cache leading chars
    XCHAR               rgxchHistToken[ichTkLenMax];    // Text of history tk
    void               *pObject;                        // Current object(cell)
    void               *pObjectIp;                      // Object at IP
#ifdef EM_MULT_DIM_SCAN
    long                iCol;                           // Column of cell
    long                iRow;                           // Row of cell
    long                iColIp;                         // Column of IP
    long                iRowIp;                         // Row of IP
    int                 dcellScanToIp;                  // #rows/cols 2 prescan
    int                 iScanDirection;                 // 0 == row, 1 == col
#endif /* EM_MULT_DIM_SCAN */
    MSOCP               cpFirst;                        // CP start of scan
    MSOCP               cpLim;                          // CP limit of scan
    MSOCP               cpFirstDoc;                     // CP limit of scan
    MSOCP               cpMacDoc;                       // CP limit of scan
    long                wInterval;                      // Count of intervals

    // Format lexer state information
    union
        {
        unsigned long   grpfFormatFlags;
        struct
            {
            MSOBF       fBold : 1;                      // Is text bold?
            MSOBF       fItalic : 1;                    // Is text italic?
            MSOBF       fUnderline : 1;                 // Is text underlined?
            MSOBF       fVanish : 1;                    // Is text hidden?
            MSOBF       ico : 5;                        // Is text colored?
            MSOBF       fSpareFmt : 7;
            };
        };
    union
        {
        unsigned short  grpfEndFlags;
        struct
            {
            MSOBF       fCreateEndObjCh : 1;            // Create EOO tk?
            MSOBF       fEOL : 1;                       // End of line?
            MSOBF       fEOP : 1;                       // End of paragraph?
            };
        };

    // Asynchronous lexer support
    unsigned short      iuState;                        // Async state
    MSOBF               fInvalLexer : 1;                // Lexer not synched?
    MSOBF               fBufferAlloced : 1;             // Obj buffer alloced?
    MSOBF               fAsyncSpare4 : 14;
    MSOCP               cpIp;                           // CP of IP if forced

    // Multiple lexical scan support
    MSOBF               fDynAlloced : 1;                // struct alloced?
    MSOBF               fTkCacheDynAlloced : 1;         // TK Cache alloced?
    MSOBF               fFmtTkCacheDynAlloced : 1;      // Format Che alloced?
    struct _MSOLEXS    *plexsNext;                      // Next struct LIFO

    // App-specific goo
    void               *pUserData;                      // Cast as desired
    } MSOLEXS;


// grpfLexFlags
#define MsoGrfLexFNoReset               (1 << 0)
#define MsoGrfLexFLookup                (1 << 1)
#define MsoGrfLexFLookupIntsAndSyms     (1 << 2)
#define MsoGrfLexFAllCapsAsFormat       (1 << 3)
#define MsoGrfLexFRefetchOnOverscan     (1 << 4)


extern unsigned short const **vppchtblCharTrans;        // Ptr to lexer ch tbl


#ifdef EM_MULT_DIM_SCAN
#define iScanVert       0
#define iScanHoriz      1
#endif /* EM_MULT_DIM_SCAN */

// Return the object the lexer is currently scanning
#define PobjectLexToken(plexs) \
            ((plexs)->pObject)

// Return the current lexer token starting CP value
#define CpLexTokenFirst(plexs) \
            ((plexs)->cpTokenFirst)

// Set the current lexer token starting CP value
#define SetCpLexTokenFirst(plexs, cp) \
            ((plexs)->cpTokenFirst = (cp))

// Return the next lexer token starting CP value
#define CpLexTokenNext(plexs) \
            ((plexs)->cpTokenNext)

// Set the next lexer token starting CP value
#define SetCpLexTokenNext(plexs, cp) \
            ((plexs)->cpTokenNext = (cp))

// Get the current lexer token dCP (length of CPs consumed)
#define DcpLexToken(plexs) \
            ((plexs)->dcpToken)

// Set the current lexer token dCP (length of CPs consumed)
#define SetDcpLexToken(plexs, dcp) \
            ((plexs)->dcpToken = (dcp))

// Update the current lexer token dCP (length of CPs consumed)
#define UpdateDcpLexToken(plexs, dcp) \
            IncrDcpLexToken(plexs, dcp)

// Update the current lexer token dCP (length of CPs consumed)
#define IncrDcpLexToken(plexs, dcp) \
            ((plexs)->dcpToken += (dcp))

#ifndef EM_LEX_VANISHED
// Update the current lexer token dCP (length of CPs consumed)
#define ClearDcpLexToken(plexs) \
            SetDcpLexToken(plexs, 0L)

#else /* EM_LEX_VANISHED */

// Update the current lexer token dCP (length of CPs consumed)
#define ClearDcpLexToken(plexs) \
            (SetDcpLexToken(plexs, 0L), \
             plexs->cpFirstVanished = 0L, \
             plexs->dcpVanished = 0L)
#endif /* !EM_LEX_VANISHED */


// Return the current lexer token *running* dCP (length of CPs consumed)
#define DcpLexCurr(plexs) \
            (DcpLexToken(plexs) + CchTokenLen(plexs))

// Return the current lexer CP value
#define CpLexCurr(plexs) \
            (CpLexTokenFirst(plexs) + DcpLexToken(plexs))

// Is this the last run, period?
#define FLexEndOfScan(plexs) \
            ((plexs)->cpRun + (plexs)->ccpRun >= (plexs)->cpLim)

#define CchTokenLen(plexs) \
            (CchTokenUncachedLen(plexs) + (plexs)->ichCache)
#define CchTokenUncachedLen(plexs) \
            ((plexs)->pxchNext - (plexs)->pxchTkStart)

// Return the index of the start of curr tk into line buffer (vfli.lrgxch)
#define IchLexTkFirst(plexs) \
            ((plexs)->pxchTkStart - (plexs)->pxchBuffer)

// Encode a relative TK index as an absolute number
#define _IrultkTokenAbsEncoded(plexs, dirultk) \
            ((plexs)->rultkhToken.irultkAbsBase \
                + (plexs)->irultkLim + (dirultk))

// Mark that the lexer must reset on next char typed
#define InvalLex(plexs) \
            ((plexs)->cchLookahead = -1)

// Return whether the lexer must reset on next char typed
#define FInvalLex(plexs) \
            ((plexs)->cchLookahead < 0)


// Mark lexer as probably out of synch with app buffer
#define InvalLexFetch(plexs) \
            ((plexs)->cchRemain = 0, \
             (plexs)->fInvalLexer = fTrue)

// Return whether lexer out of synch with app buffer
#define FInvalLexFetch(plexs) \
            ((plexs)->fInvalLexer)



/*************************************************************************
    Token History Cache
 *************************************************************************/

// Token-history cache access
#define PrultkFromTokenIrultk(plexs, irultk) \
            PrultkFromIrultk(irultk, (plexs)->rultkhToken.rgrultkCache)

// Increment pointer to token-history cache access
#define IncrTokenPrultk(plexs, pprultk, pirultkPrev) \
            IncrPrultk(pprultk, pirultkPrev, \
                        (plexs)->rultkhToken.rgrultkCache, \
                        (plexs)->rultkhToken.irultkMac)

// Decrement pointer to token-history cache access
#define DecrTokenPrultk(plexs, pprultk, pirultkPrev) \
            DecrPrultk(pprultk, pirultkPrev, \
                        (plexs)->rultkhToken.rgrultkCache, \
                        (plexs)->rultkhToken.irultkMac)

// Increment index to token-history cache access
#define IncrTokenPirultk(plexs, pirultk, dirultk) \
            IncrPirultk(pirultk, dirultk, (plexs)->rultkhToken.irultkMac)

// Increment index to token-history cache access
#define DecrTokenPirultk(plexs, pirultk, dirultk) \
            DecrPirultk(pirultk, dirultk, (plexs)->rultkhToken.irultkMac)

// Fill in next tk cache record even if incomplete.
#define _CacheTkTextNext(plexs) \
            { \
            MSORULTK   *prultk; \
            int         cchPartialTk = (plexs)->cchLookahead; \
             \
            prultk = PrultkFromTokenIrultk((plexs), \
                                           (plexs)->rultkhToken.irultkLim); \
            prultk->pObject = plexs->pObject; \
            prultk->cpFirst = CpLexTokenFirst(plexs); \
            prultk->dcp = DcpLexCurr(plexs) + cchPartialTk; \
            prultk->ich = (plexs)->pxchTkStart - (plexs)->pxchBuffer; \
            prultk->dich = CchTokenLen(plexs) + cchPartialTk; \
            prultk->wInterval = (plexs)->wInterval; \
            prultk->tk = tkNil; \
            }


/*************************************************************************
    Formatting Token History Cache
 *************************************************************************/

// Format token-history cache access
#define PrultkFormatFromIrultk(plexs, irultk) \
            PrultkFromIrultk(irultk, (plexs)->rultkhFormat.rgrultkCache)

// Increment pointer to Format token-history cache access
#define IncrFormatPrultk(plexs, pprultk, pirultkPrev) \
            IncrPrultk(pprultk, pirultkPrev, \
                       (plexs)->rultkhFormat.rgrultkCache, \
                       (plexs)->rultkhFormat.irultkMac)

// Increment pointer to Format token-history cache access
#define DecrFormatPrultk(plexs, pprultk, pirultkPrev) \
            DecrPrultk(pprultk, pirultkPrev, \
                        (plexs)->rultkhFormat.rgrultkCache, \
                        (plexs)->rultkhFormat.irultkMac)

// Increment index to Format token-history cache access
#define IncrFormatPirultk(pirultk, dirultk) \
            IncrPirultk(pirultk, dirultk, (plexs)->rultkhFormat.irultkMac)

// Increment index to Format token-history cache access
#define DecrFormatPirultk(pirultk, dirultk) \
            DecrPirultk(pirultk, dirultk, (plexs)->rultkhFormat.irultkMac)



/*************************************************************************
    Prototypes and macros for lex.c
 *************************************************************************/


// Get the next character from the input buffer
#define XchLexGetChar(plexs, cpLim) \
            ((plexs)->cchRemain-- > 0\
                ? *(plexs)->pxchNext++ \
                : XchLexGetNextBuffer(cpLim, plexs))

// Get the next input buffer
#define XchLexGetNextBuffer(cpLim, plexs) \
            ((*(plexs)->pfnlexbuf)(cpLim, plexs))

// Return last char to input buffer
#ifndef AS_FUNCTION
#define LexUngetChar(plexs, cch) \
            ((plexs)->pxchNext -= cch, (plexs)->cchRemain += cch)
#else
void LexUngetChar(MSOLEXS *plexs, int cch);
#endif /* !AS_FUNCTION */

// Peek at the next character from the input buffer
#define XchLexPeekChar(plexs) \
            ((plexs)->cchRemain > 0 \
                ? *(plexs)->pxchNext \
                : (XchLexGetNextBuffer(msocpMax, plexs), (plexs)->cchRemain++,\
                        *(--(plexs)->pxchNext)))

// Number of bytes to copy if we are "peeking" at next char via lexer
#define cbLexsPeek  (offset(MSOLEXS, ichCache))

// Define (CH)aracter Translation (T)a(BL)e
typedef unsigned short ISTT;                            // Col index to Lex STT
typedef ISTT const *CHTBL;                              // Char trans table
#ifndef VIEWER

// Definition of size of State Transition Table
#define WSttblNumRows       5                           // Num rows in Sttbl
#define WSttblNumCols       15                          // Num cols in Sttbl

typedef unsigned short const STTBL [WSttblNumCols];     // State trans table

extern unsigned short const rgsttblWsIndirect[WSttblNumRows][WSttblNumCols];
extern unsigned short const rgsttblWsDirect[WSttblNumRows][WSttblNumCols];

extern CHTBL _rgchtblNormal[256];                       // Normal ch trans tbl

// Based pointer to current Character Transition Table
extern CHTBL const *vpchtblCharTrans;                   // Curr ch trans table


// Translate a character into a column in the lexer STTBL
#define IsttFromXch(xch)    \
            vpchtblCharTrans[MsoHighByteXch(xch)][MsoLowByteXch(xch)]
#endif // !VIEWER


// Define Delimiter Lookup table
extern TK const * Win(const) vrgptkCharToken[256];

// Return delimiter token associated with character
#define TkDelimFromXch(xch) \
            vrgptkCharToken[MsoHighByteXch(xch)][MsoLowByteXch(xch)]


// Map a STT column index into a Character token value
extern TK vmpistttkCh[];



MSOAPI_(TK) MsoTkLexText(MSOLEXS *plexs);               // Get next token
MSOAPI_(TK) MsoTkLexTextCpLim(                          // Get next tk < CP
    MSOLEXS            *plexs,
    MSOCP               cpLim
    );
MSOAPI_(int) MsoFLexTokenCh(MSOLEXS *plexs, XCHAR xch); // Token ready?
void SetLexTokenLim(MSOLEXS *plexs);                    // Set token Lim
MSOCP DcpLexCurrAdjusted(MSOLEXS *plexs);               // Return dCP used
MSOAPI_(XCHAR) MsoWchLexGetNextBufferDoc(               // Reload char buf
    MSOCP               cpLim,
    MSOLEXS            *plexs
    );
void ForceLexEOF(void);                                 // FUTURE: Force EOF
MSOAPI_(MSOLEXS *) MsoPlexsLexInitDoc(                  // Init from doc
    MSOLEXS            *plexs,
    void               *hObjectNil,
    PFNLEXRUN           pfnlexrun,
    PFNLEXTXT           pfnlextxt,
    PFNLEXFMT           pfnlexfmt,
    PFNLEXCNT           pfnlexrunDiscontig,
    int                 irultkTokenMac,
    int                 irultkFormatMac
    );
#ifdef DEBUG
MSOAPI_(void) MsoAssertPlexsInitDoc(                    // Ensure doc init
    MSOLEXS            *plexs,
    void               *hObjectNil,
    PFNLEXRUN           pfnlexrun,
    PFNLEXTXT           pfnlextxt,
    PFNLEXFMT           pfnlexfmt,
    PFNLEXCNT           pfnlexrunDiscontig,
    int                 irultkTokenMac,
    int                 irultkFormatMac
    );
#endif // DEBUG
MSOAPI_(void) MsoLexSetPos(                             // Reposition in file
    MSOLEXS            *plexs,
    MSOCP               cpFirst,
    MSOCP               cpLim
    );
MSOAPI_(XCHAR) MsoWchLexGetNextBufferPxch(              // Gen EOF for rgch
    MSOCP               cpLim,
    MSOLEXS            *plexs
    );
MSOAPI_(MSOLEXS *) MsoPlexsLexInitPxch(                 // Init from rgch
    MSOLEXS            *plexs,
    XCHAR              *pxch,
    int                 cch,
    PFNLEXBUF           pfnlexbuf,
    int                 irultkTokenMac,
    int                 irultkFormatMac
    );
#ifdef DEBUG
MSOAPI_(void) MsoAssertPlexsInitPxch(                   // Ensure rgch init
    MSOLEXS            *plexs,
    XCHAR              *pxch,
    int                 cch,
    PFNLEXBUF           pfnlexbuf,
    int                 irultkTokenMac,
    int                 irultkFormatMac
    );
#endif // DEBUG
void LexFinishPch(void);                                // Complete rgch scan
STTBL *PsttblFromIsttbl(int isttbl);                    // table ptr from index
MSOLEXS *PlexsNew(void);                                // Alloc new MSOLEXS
MSOLEXS *PlexsInitLex(                                  // Init lexer memory
    MSOLEXS            *plexs,
    int                 irultkTokenMac,
    int                 irultkFormatMac
    );
#ifdef DEBUG
void AssertPlexsInit(                                   // Ensure lexer memory
    MSOLEXS            *plexs,
    int                 irultkTokenMac,
    int                 irultkFormatMac
    );
#endif
MSOAPI_(void) MsoResetLexState(                         // Reset lexer state
    MSOLEXS            *plexs,
    int                 fFullReset
    );
MSOAPI_(void) MsoFreeLexMem(MSOLEXS *plexs);            // Free lexer memory
#if defined(DEBUG)  &&  !defined(STANDALONE)
MSOAPI_(void) MsoMarkLexMem(MSOLEXS *plexs);            // Mark lexer mem used
#endif // DEBUG  &&  !STANDALONE
int FResetLexDocBuffer(                                 // Reset cpObject
    void               *pObject,
    MSOCP               cpObject,
    MSOCP               cpScan,
    MSOCP              *pcpObject                       // RETURN
    );

// Return token associated with string by looking up in keyword table
#define TkLookupNameLexs(pxchStr, cchLen, plexs) \
            MsoTkLookupName((pxchStr), (cchLen), (plexs)->pkwtb)

// Return token associated with string by looking up in keyword table
#define PkwdLookupNameLexs(pxchStr, cchLen, plexs) \
            MsoPkwdLookupName((pxchStr), (cchLen), (plexs)->pkwtb)

// Add a keyword to the lexer lookup table
#define PkwdAddTkLookupNameLexs(pxchStr, cchLen, tk, plexs, fCopyStr) \
            MsoPkwdAddTkLookupName((pxchStr), (cchLen), (tk), (plexs)->pkwtb,\
                                   (fCopyStr))

// Remove a keyword from the lexer lookup table
#define FRemoveTkLookupNameLexs(pxchStr, cchLen, plexs, ptk) \
            MsoFRemoveTkLookupName((pxchStr), (cchLen), (plexs)->pkwtb, (ptk))

void AppendRultkFormat(                                 // Append format token
    MSOLEXS            *plexs,
    TK                  tk,
    int                 dcp,
    long                lValue
    );
void InsertRultkFormat(                                 // Insert format token
    MSOLEXS            *plexs,
    TK                  tk,
    MSOCP               cp,
    long                lValue
    );
MSOAPI_(void) MsoCacheTkText(                           // Save text tokens
    MSOLEXS            *plexs,
    TK                  tk,
    long                lValue
    );
MSOAPI_(int) MsoCchTokenText(                           // Return token text
    MSOLEXS            *plexs,
    int                 dtk,
    const XCHAR       **ppxch                           // RETURN
    );
#define TokenLen(plexs, dtk) \
            MsoCchTokenText((plexs), (dtk), NULL)
MSOAPI_(MSOCA *) MsoPcaOfDtk(                           // Get CA of tk range
    MSOCA              *pca,
    int                 dtkStart,
    int                 dtk,
    MSOLEXS            *plexs
    );
MSOAPI_(MSOCA *) MsoPcaOfDtkExclusive(                  // Get CA inside tk rg
    MSOCA              *pca,
    int                 dtkStart,
    int                 dtk,
    MSOLEXS            *plexs
    );
#ifdef NEVER
int CchCopyTextOfDtk(                                   // Return mult tk text
    int                 dtkStart,
    int                 dtk,
    XCHAR              *rgxch,                          // IN, RETURN
    int                 cchMax,
    int                 fPartialTkOK
    );
#endif // NEVER
MSOAPI_(long) MsoLFromDtk(                              // Get integer value
    MSOLEXS            *plexs,
    int                 dtk,
    int                 fCheckForSign
    );
long LFromPxch(                                         // Convert str to long
    const XCHAR        *pxch,
    int                 cch,
    int                *pfOverflow
    );
int FUpperXch(XCHAR xch);                               // Char uppercase?
int FLowerXch(XCHAR xch);                               // Char lowercase?

#ifdef NEVER
int OFC_CALLBACK DxaOfDirultk(                          // Return tk coord,len
    int                 dtk,
    int                *pdxaLen                         // RETURN: optional
    );

TK TkFromXch(XCHAR xch);                                // Return tk from a ch
TK TkFromXchNoLookup(XCHAR xch);                        // Return tk from ch
XCHAR *PxchTkStartFromPxchReverse(XCHAR *, XCHAR *);
int FXchEndsTk(XCHAR);                                  // Does this xch end a tk?
TK TkFromChIsttbl(XCHAR xch, int isttbl);               // Return a tk from a char
int OFC_CALLBACK DtkCacheTkTextToCp(                    // Fill Text TK cache
    int                 dtk,
    MSOCP               cpLim,
    int                 fForce
    );
#endif // NEVER
void CopyTkTextToCache(MSOLEXS *plexs);                 // Flush pend lex text



#if defined(DEBUG)  ||  defined(STANDALONE)
char *SzFromPch(                                        // Temp make string
    const char         *pchStr,
    int                 cchLen,
    char               *rgchStrBuf
    );
XCHAR *XszFromPxch(                                     // Temp make string
    const XCHAR        *pxchStr,
    int                 cchLen,
    XCHAR              *rgxchStrBuf
    );
#ifndef STANDALONE
char *SzFromPxch(                                       // Temp make string
    const XCHAR        *pxchStr,
    int                 cchLen,
    char               *rgchStrBuf
    );
#else /* STANDALONE */
#define SzFromPxch(p1,p2,p3) SzFromPch(p1, p2, p3)
#endif /* !STANDALONE */
#endif // DEBUG  ||  STANDALONE


/*************************************************************************
    Utilities
 *************************************************************************/

#ifndef FAREAST
#define FWhitespaceXch(xch) \
            ((xch) == ' '  ||  (xch) == '\t'  ||  (xch) == xchColumnBreak)
#else /* FAREAST */
#define FWhitespaceXch(xch) \
            ((xch) == ' '  ||  (xch) == '\t'  ||  (xch) == xchColumnBreak \
                ||  (xch) == wchSpace)
#endif /* FAREAST */



/*************************************************************************
    Prototypes and macros for Debugging and Error Handling
 *************************************************************************/

const XCHAR *PxchLexTokenText(                          // Return tk text, len
    MSOLEXS            *plexs,
    int                *pwLen);
#ifdef DEBUG
MSOAPI_(XCHAR *) MsoLxszLexTokenText(MSOLEXS *plexs);   // Return token text
MSOAPI_(CHAR *) MsoSzLexTokenText(MSOLEXS *plexs);      // Return token sz
#endif // DEBUG

#endif // MSOLEX_H

