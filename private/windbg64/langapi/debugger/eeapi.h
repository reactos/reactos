/**     eeapi.hxx - Public API to the Expression Evaluator
 *
 *      This file contains all types and APIs that are defined by
 *      the Expression Evaluator and are publicly accessible by
 *      other components.
 *
 *      Before including this file, you must include cvtypes.h and shapi.h
 */


/***    The master copy of this file resides in the CVINC project.
 *      All Microsoft projects are required to use the master copy without
 *      modification.  Modification of the master version or a copy
 *      without consultation with all parties concerned is extremely
 *      risky.
 *
 *      The projects known to use this version (1.00.00) are:
 *
 *          Codeview (uses version in CVINC project)
 *          Visual C++
 *          C/C++ expression evaluator (uses version in CVINC project)
 *          Cobol expression evaluator
 *          QC/Windows
 *          Pascal 2.0 expression evaluator
 */

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

#ifndef EE_API
#define EE_API

#ifdef __cplusplus
#pragma warning(disable: 4200) // "non-standard extension: zero-sized array"
#endif

//  **********************************************************************
//  *                                                                    *
//  *   Initialization Structures                                        *
//  *                                                                    *
//  **********************************************************************

#define EEAPI WINAPI

typedef struct {
    void *  (EEAPI *pMHlpvAlloc)( size_t );
    void    (EEAPI *pMHFreeLpv)(void *);
    HEXE    (EEAPI *pSHGetNextExe)(HEXE);
    HEXE    (EEAPI *pSHHexeFromHmod)(HMOD);
    HMOD    (EEAPI *pSHGetNextMod)(HEXE, HMOD);
    PCXT    (EEAPI *pSHGetCxtFromHmod)(HMOD, PCXT);
    PCXT    (EEAPI *pSHGetCxtFromHexe)(HEXE, PCXT);
    PCXT    (EEAPI *pSHSetCxt)(LPADDR, PCXT);
    PCXT    (EEAPI *pSHSetCxtMod)(LPADDR, PCXT);
    HSYM    (EEAPI *pSHFindNameInGlobal)(HSYM, PCXT, LPSSTR, SHFLAG, PFNCMP, PCXT);
    HSYM    (EEAPI *pSHFindNameInContext)(HSYM, PCXT, LPSSTR, SHFLAG, PFNCMP, PCXT);
    HSYM    (EEAPI *pSHGoToParent)(PCXT, PCXT);
    HSYM    (EEAPI *pSHHsymFromPcxt)(PCXT);
    HSYM    (EEAPI *pSHNextHsym)(HMOD, HSYM);
    PCXF    (EEAPI *pSHGetFuncCxf)(LPADDR, PCXF);
    char *  (EEAPI *pSHGetModName)(HMOD);
    char *  (EEAPI *pSHGetExeName)(HEXE);
    char *  (EEAPI *pSHGetModNameFromHexe)(HEXE);
    char *  (EEAPI *pSHGetSymFName)(HEXE);
    HEXE    (EEAPI *pSHGethExeFromName)(char *);
    HEXE    (EEAPI *pSHGethExeFromModuleName)(char *);
    UOFF32  (EEAPI *pSHGetNearestHsym)(LPADDR, HMOD, int, PHSYM);
    SHFLAG  (EEAPI *pSHIsInProlog)(PCXT);
    SHFLAG  (EEAPI *pSHIsAddrInCxt)(PCXT, LPADDR);
    int     (EEAPI *pSHModelFromAddr)(PADDR,LPW,LPB,UOFFSET *);
    HSYM    (EEAPI *pSHFindSLink32)( PCXT );

    BOOL    (EEAPI *pSLLineFromAddr) ( LPADDR, unsigned short *, SHOFF *, SHOFF * );
    BOOL    (EEAPI *pSLFLineToAddr)  ( HSF, DWORD, LPADDR, SHOFF *, DWORD * );
    char *  (EEAPI *pSLNameFromHsf)  ( HSF );
    HMOD    (EEAPI *pSLHmodFromHsf)  ( HEXE, HSF );
    HSF     (EEAPI *pSLHsfFromPcxt)  ( PCXT );
    HSF     (EEAPI *pSLHsfFromFile)  ( HMOD, char * );

    UOFF32  (EEAPI *pPHGetNearestHsym)(LPADDR, HEXE, PHSYM);
    HSYM    (EEAPI *pPHFindNameInPublics)(HSYM, HEXE, LPSSTR, SHFLAG, PFNCMP);
    HTYPE   (EEAPI *pTHGetTypeFromIndex)(HMOD, THIDX);
    HTYPE   (EEAPI *pTHGetNextType)(HMOD, HTYPE);
    HDEP    (EEAPI *pMHMemAllocate)(size_t);
    HDEP    (EEAPI *pMHMemReAlloc)(HDEP, size_t);
    void    (EEAPI *pMHMemFree)(HDEP);
    HVOID   (EEAPI *pMHMemLock)(HDEP);
    void    (EEAPI *pMHMemUnLock)(HDEP);
    SHFLAG  (EEAPI *pMHIsMemLocked)(HDEP);
    HVOID   (EEAPI *pMHOmfLock)(HVOID);
    void    (EEAPI *pMHOmfUnLock)(HVOID);
    SHFLAG  (EEAPI *pDHExecProc)(LPADDR, SHCALL);
    UINT    (EEAPI *pDHGetDebuggeeBytes)(ADDR, UINT, void *);
    UINT    (EEAPI *pDHPutDebuggeeBytes)(ADDR, UINT, void *);
    PSHREG  (EEAPI *pDHGetReg)(PSHREG, PCXT, HFRAME);
    PSHREG  (EEAPI *pDHSetReg)(PSHREG, PCXT, HFRAME);
    HDEP    (EEAPI *pDHSaveReg)(PCXT);
    void    (EEAPI *pDHRestoreReg)(HDEP, PCXT);
    char    *pin386mode;
    char    *pis_assign;
    int     (EEAPI *pquit)(DWORD);
    ushort  *pArrayDefault;
    SHFLAG  (EEAPI *pSHCompareRE)(char *, char *, int);
    XOSD    (EEAPI *pSHFixupAddr)(LPADDR);
    XOSD    (EEAPI *pSHUnFixupAddr)(LPADDR);
    SHFLAG  (EEAPI *pCVfnCmp)(HVOID, HVOID, char *, SHFLAG);
    SHFLAG  (EEAPI *pCVtdCmp)(HVOID, HVOID, char *, SHFLAG);
    SHFLAG  (EEAPI *pCVcsCmp)(HVOID, HVOID, char *, SHFLAG);
    BOOL    (EEAPI *pCVAssertOut)(char *, char *, DWORD);
    SHFLAG  (EEAPI *pDHSetupExecute)(LPHIND);
    SHFLAG  (EEAPI *pDHCleanUpExecute)(HIND);
    SHFLAG  (EEAPI *pDHStartExecute)(HIND, LPADDR, BOOL, SHCALL);
    HSYM    (EEAPI *pSHFindNameInTypes)( PCXT, LPSSTR, SHFLAG , PFNCMP , PCXT );
    UINT    (EEAPI *pSYProcessor)(DWORD);
    BOOL    (EEAPI *pTHAreTypesEqual)(HMOD, CV_typ_t, CV_typ_t);
    MPT     (EEAPI *pGetTargetProcessor) (HPID);
    BOOL    (EEAPI *pGetUnicodeStrings) ();
    BOOL    (EEAPI *pSYGetAddr) (HFRAME, LPADDR lpaddr, ADR addrtype);
    BOOL    (EEAPI *pSYSetAddr) (HFRAME, LPADDR lpaddr, ADR addrtype);
    XOSD    (EEAPI *pSYGetMemInfo)(LPMEMINFO);
    BOOL    (EEAPI *pSHWantSymbols)(HEXE);
} CVF;  // CodeView kernel Functions exported to the Expression Evaluator
typedef CVF * PCVF;

// DO NOT CHANGE THESE DEFINITIONS FROM __cdecl to CDECL
// Windows.h will define CDECL to an empty string if it has not been defined.
// We will run into bugs if windows.h is included before
// any other header files.


typedef struct {
    short   (__cdecl *pintLoadDS)();
    char  * (__cdecl *pultoa)(ulong, char  *, int);
    char  * (__cdecl *pitoa)(int, char  *, int);
    char  * (__cdecl *pltoa)(long, char  *, int);
    int     (__cdecl *peprintf)(const char *, char *, char *, int);
    int     (__cdecl *psprintf)(char  *, const char *, ...);
    FLOAT10 (__cdecl *p_strtold)( const char  *, char  *  *);
} CRF;  // C Runtime functions exported to the Expression Evaluator
typedef CRF * PCRF;

typedef struct CI {
    char    cbCI;
    char    Version;
    CVF *   pStructCVAPI;
    CRF *   pStructCRuntime;
} CI;
typedef CI * PCI;


typedef struct HDR_TYPE {
    ulong  offname;
    ulong  lenname;
    ulong  offtrail;
} HDR_TYPE;
typedef HDR_TYPE *PHDR_TYPE;

//  **********************************************************************
//  *                                                                    *
//  *   the expr evaluator stuff                                         *
//  *                                                                    *
//  **********************************************************************

typedef HDEP            HSYML;      //* An hsym list
typedef HSYML *         PHSYML;     //* A pointer to a hsym list
typedef uint            EERADIX;
typedef EERADIX *       PEERADIX;
typedef uchar *         PEEFORMAT;
typedef ulong           EESTATUS;
typedef HDEP            EEHSTR;
typedef EEHSTR *        PEEHSTR;
typedef HDEP            HTM;
typedef HTM *           PHTM;
typedef HDEP            HTI;
typedef HTI *           PHTI;
typedef HDEP            HBCIA;      // Base class index array.
typedef HBCIA *         PHBCIA;

// Error return values
#define EENOERROR       0
#define EENOMEMORY      1
#define EEGENERAL       2
#define EEBADADDR       3
#define EECATASTROPHIC  0XFF

typedef enum {
    EEHORIZONTAL,
    EEVERTICAL,
    EEBPADDRESS
} EEDSP;                    // Display format specifier

typedef enum {
    ETIPRIMITIVE,
    ETIARRAY,
    ETIPOINTER,
    ETICLASS,
    ETIFUNCTION
} ETI;
typedef ETI *PETI;

typedef enum {
    EENOTEXP,
    EEAGGREGATE,
    EETYPE,
    EEPOINTER,
    EETYPENOTEXP,
    EETYPEPTR
} EEPDTYP;
typedef EEPDTYP *PEEPDTYP;

//
// Flags to control EEFormatAddress
//
 typedef enum {
     EEFMT_32 = 0x01,            /* Display a 32-bit offset      */
     EEFMT_SEG = 0x02,           /* Display a segment            */
     EEFMT_LOWER = 0x04,         /* Use lowercase letters        */
     EEFMT_REAL  = 0x08          /* Real  mode address           */
 } EEFMTFLGS;

typedef struct TML {
    unsigned    cTMListMax;
    unsigned    cTMListAct;
    unsigned    iTMError;
    HDEP        hTMList;
} TML;
typedef TML *PTML;

typedef struct RTMI {
    ulong   fSegType    :1;
    ulong   fAddr       :1;
    ulong   fValue      :1;
    ulong   fSzBits     :1;
    ulong   fSzBytes    :1;
    ulong   fLvalue     :1;
    ulong   fSynthChild :1;
    ulong   fLabel      :1;
    ulong   fFmtStr     :1;
    CV_typ_t Type;
} RTMI;
typedef RTMI *    PRI;

typedef struct TMI {
    RTMI        fResponse;
    struct  {
        ulong   SegType    :4;
        ulong   fLvalue    :1;
        ulong   fAddrInReg :1;
        ulong   fBPRel     :1;
        ulong   fFunction  :1;
        ulong   fLData     :1;      // True if expression references local data
        ulong   fGData     :1;      // True if expression references global data
        ulong   fSynthChild:1;
        ulong   fLabel:1;
        ulong   fFmtStr:1;          // True if expression has format suffix
    };
    union   {
        ADDR    AI;
        ulong   hReg;               // This is really a CV_HREG_e
    };
    ulong       cbValue;
    char        Value[0];
} TMI;
typedef TMI *   PTI;

typedef struct {
    HSYM    hSym;
    CXT     CXT;
} HCS;

typedef struct {
    CXT     CXT;
    ulong   cHCS;
    HCS     rgHCS[0];
} CXTL;

typedef HDEP        HCXTL;
typedef HCXTL * PHCXTL;
typedef CXTL  * PCXTL;

//  Structures for Get/Free HSYMList

//  Search request / response flags for Get/Free HSYMList

#define  HSYMR_lexical  0x0001  // lexical out to function scope
#define  HSYMR_function 0x0002  // function scope
#define  HSYMR_class    0x0004  // class scope
#define  HSYMR_module   0x0008  // module scope
#define  HSYMR_global   0x0010  // global symbol table scope
#define  HSYMR_exe      0x0020  // all other module scope
#define  HSYMR_public   0x0040  // public symbols
#define  HSYMR_nocase   0x8000  // case insensitive
#define  HSYMR_allscopes   \
               (HSYMR_lexical    |\
                HSYMR_function   |\
                HSYMR_class      |\
                HSYMR_module     |\
                HSYMR_global     |\
                HSYMR_exe        |\
                HSYMR_public     |\
                HSYMR_nocase)

//  Controls for EEFormatCXTFromPCXT

#define HCXTFMT_Short                   1
#define HCXTFMT_No_Procedure            2


//  structure describing HSYM list for a context

typedef struct HSL_LIST {
    ulong       request;        // context that this block statisfies
    struct  {
        ulong   isused      :1; // block contains data if true
        ulong   hascxt      :1; // context packet has been stored
        ulong   complete    :1; // block is complete if true
        ulong   isclass     :1; // context is class if true
    } status;
    HSYM        hThis;          // handle of this pointer if class scope
    ulong       symbolcnt;      // number of symbol handles in this block
    CXT         Cxt;            // context for this block of symbols
    HSYM        hSym[];         // list of symbol handles
} HSL_LIST;
typedef HSL_LIST *PHSL_LIST;


typedef struct HSL_HEAD {
    ulong       size;           // number of bytes in buffer
    ulong       remaining;      // remaining space in buffer
    PHSL_LIST   pHSLList;       // pointer to current context list (EE internal)
    struct  {
        ulong   endsearch   :1; // end of search reached if true
        ulong   fatal       :1; // fatal error if true
    } status;
    ulong       blockcnt;       // number of CXT blocks in buffer
    ulong       symbolcnt;      // number of symbol handles in buffer
    HDEP        restart;        // handle of search restart information
} HSL_HEAD;
typedef HSL_HEAD *PHSL_HEAD;

typedef struct HINDEX_ARRAY {
    ulong       count;              // number of indices in this buffer.
    long        rgIndex[0];
}   HINDEX_ARRAY;
typedef HINDEX_ARRAY    *PHINDEX_ARRAY;

typedef struct {
    void     (PASCAL *pEEFreeStr)(EEHSTR);
    EESTATUS (PASCAL *pEEGetError)(PHTM, EESTATUS, PEEHSTR);
    EESTATUS (PASCAL *pEEParse)(const char *, EERADIX, SHFLAG, PHTM, ulong  *);
    EESTATUS (PASCAL *pEEBindTM)(PHTM, PCXT, SHFLAG, SHFLAG);
    EESTATUS (PASCAL *pEEvaluateTM)(PHTM, HFRAME, EEDSP);
    EESTATUS (PASCAL *pEEGetExprFromTM)(PHTM, PEERADIX, PEEHSTR, DWORD *);
    EESTATUS (PASCAL *pEEGetValueFromTM)(PHTM, EERADIX, PEEFORMAT, PEEHSTR);
    EESTATUS (PASCAL *pEEGetNameFromTM)(PHTM, PEEHSTR);
    EESTATUS (PASCAL *pEEGetTypeFromTM)(PHTM, EEHSTR, PEEHSTR, ulong);
    EESTATUS (PASCAL *pEEFormatCXTFromPCXT)(PCXT, PEEHSTR, DWORD);
    void     (PASCAL *pEEFreeTM)(PHTM);
    EESTATUS (PASCAL *pEEParseBP)(char *, EERADIX, SHFLAG, PCXF, PTML, ulong, DWORD *, SHFLAG);
    void     (PASCAL *pEEFreeTML)(PTML);
    EESTATUS (PASCAL *pEEInfoFromTM)(PHTM, PRI, PHTI);
    void     (PASCAL *pEEFreeTI)(PHTI);
    EESTATUS (PASCAL *pEEGetCXTLFromTM)(PHTM, PHCXTL);
    void     (PASCAL *pEEFreeCXTL)(PHCXTL);
    EESTATUS (PASCAL *pEEAssignTMToTM)(PHTM, PHTM);
    EEPDTYP  (PASCAL *pEEIsExpandable)(PHTM);
    SHFLAG   (PASCAL *pEEAreTypesEqual)(PHTM, PHTM);
    EESTATUS (PASCAL *pEEcChildrenTM)(PHTM, long *, PSHFLAG);
    EESTATUS (PASCAL *pEEGetChildTM)(PHTM, long, PHTM, DWORD *, EERADIX, SHFLAG);
    EESTATUS (PASCAL *pEEDereferenceTM)(PHTM, PHTM, DWORD *, SHFLAG);
    EESTATUS (PASCAL *pEEcParamTM)(PHTM, DWORD *, PSHFLAG);
    EESTATUS (PASCAL *pEEGetParmTM)(PHTM, DWORD, PHTM, DWORD *, SHFLAG);
    EESTATUS (PASCAL *pEEGetTMFromHSYM)(HSYM, PCXT, PHTM, DWORD *, SHFLAG, SHFLAG);
    EESTATUS (PASCAL *pEEFormatAddress)(PADDR, char *, DWORD, SHFLAG);
    EESTATUS (PASCAL *pEEGetHSYMList)(PHSYML, PCXT, DWORD, uchar *, SHFLAG);
    void     (PASCAL *pEEFreeHSYMList)(PHSYML);
    EESTATUS (PASCAL *pEEGetExtendedTypeInfo)(PHTM, PETI);
    EESTATUS (PASCAL *pEEGetAccessFromTM)(PHTM, PEEHSTR, ulong);
    BOOL     (PASCAL *pEEEnableAutoClassCast)(BOOL);
    void     (PASCAL *pEEInvalidateCache)(void);
    EESTATUS (PASCAL *pEEcSynthChildTM)(PHTM, long *);
    EESTATUS (PASCAL *pEEGetBCIA)(PHTM, PHBCIA);
    void     (PASCAL *pEEFreeBCIA)(PHBCIA);
    SHFLAG   (PASCAL *pfnCmp)(HVOID, HVOID, char *, SHFLAG);
    SHFLAG   (PASCAL *ptdCmp)(HVOID, HVOID, char *, SHFLAG);
    SHFLAG   (PASCAL *pcsCmp)(HVOID, HVOID, char *, SHFLAG);
    MPT      (PASCAL *pEESetTarget)(MPT);
    void     (PASCAL *pEEUnload)();
} EXF;
typedef EXF * PEXF;

typedef struct EI {
    char    cbEI;
    char    Version;
    PEXF    pStructExprAPI;
    char    Language;
    char   *IdCharacters;
    char   *EETitle;
    char   *EESuffixes;
    char   *Assign;             // length prefixed assignment operator
} EI;
typedef EI * PEI;

// FNEEINIT is the prototype for the EEInitializeExpr function
typedef VOID EXPCALL FNEEINIT(CI *, EI *);
typedef FNEEINIT *  PFNEEINIT;
typedef FNEEINIT *  LPFNEEINIT;

// This is the only EE function that's actually exported from the DLL
FNEEINIT EEInitializeExpr;

#endif // EE_API
