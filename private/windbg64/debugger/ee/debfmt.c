
/***    debfmt.c - expression evaluator formatting routines
 *
 *      GLOBAL
 *
 *
 *      LOCAL
 *
 *
 *
 *      DESCRIPTION
 *      Expression evaluator formatting routines
 *
 */


#include "debexpr.h"
#include "ldouble.h"
#ifndef _SBCS
#include <mbctype.h>
#endif

#include "locale.h"

#define _CRTBLD                         // work around bug in headers
#include "undname.h"
#undef _CRTBLD

#ifdef USE_CUSTVIEW

#include "custview.h"

HRESULT WINAPI ExportViewHwnd( DWORD dwAddress, DEBUGHELPER *pHelper, int nBase, BOOL bUniStrings, char *pResult, size_t max, DWORD reserved );
HRESULT WINAPI ExportViewGuid( DWORD dwAddress, DEBUGHELPER *pHelper, int nBase, BOOL bUniStrings, char *pResult, size_t max, DWORD reserved );
HRESULT WINAPI ExportViewVariant( DWORD dwAddress, DEBUGHELPER *pHelper, int nBase, BOOL bUniStrings, char *pResult, size_t max, DWORD reserved );
HRESULT WINAPI ExportViewHresult( DWORD dwAddress, DEBUGHELPER *pHelper, int nBase, BOOL bUniStrings, char *pResult, size_t max, DWORD reserved );
extern DEBUGHELPER StaticHelper;

#endif // USE_CUSTVIEW

void InsertCache (CV_typ_t, HEXE);

typedef enum FMT_ret {
    FMT_error,
    FMT_none,
    FMT_ok
} FMT_ret;

typedef struct FMT_state {
    pexstate_t      pExState;
    char *pExStr;
    PCXT            pCxt;
} FMT_state;

// definitions used for caching auto-expansion rules
// The cache is implemented as a table. Each entry maps a pair
// (type, hExe) to the corresponding auto expansion rule string
// (that string may be NULL if no rule exists for the particular type)

typedef struct aecache_t {
    CV_typ_t    typ;
    HEXE        hExe;
    EEHSTR      hStr;
} aecache_t;

#define AECACHE_MAX     64          // max number of entries in the cache
static aecache_t AECache [AECACHE_MAX] = {0};   // cache for auto-exp. rules
static int cAECache = 0;            // current number of entries

void    AppendCVQualifier (peval_t, char * *, uint *);
int     ConvertWCToMB (char *, wchar_t);
EESTATUS Format (peval_t, uint, char * *, int *, bool_t);
void    EvalString (peval_t, char * *, int   *, bool_t);
void    EvalMem (peval_t, char *FAR *, int   *, char);
void    FormatAutoExpand (peval_t, uint, char * *, int   *);
void    FormatAEChildTM (uint, peval_t, uint, char * *, int   *);
void    FormatAEDownCastType (peval_t, uint, char * *, int   *);
void    FormatAEError (peval_t, uint, char * * , int   *);
BOOL    GetRawAddr( peval_t pv, ADDR *paddr );
void    FormatExpand (peval_t, char * *, uint *, char * *, ulong, PHDR_TYPE);
void    FormatClass (peval_t, uint, char * * , int   *);
EESTATUS FormatEnum (peval_t, char * , int  );
void    FormatProc (peval_t, char * *, uint *, char * *, CV_typ_t, CV_typ_t, CV_call_e, ulong , CV_typ_t, ulong, PHDR_TYPE);
char *  FormatUndecorate( char*, char*);
char *  FormatVirtual (char *, peval_t, PEEHSTR);
FMT_ret VerifyFormat (peval_t, PEEFORMAT, char * *, int *, bool_t *);
INLINE  bool_t          IsPrint (int ch);

void FASTCALL   DoAutoExpand (PHTM);
void FASTCALL   DoBindAutoExpand (PHTM);
void FASTCALL   DoEvalAutoExpand (PHTM);
bool_t FASTCALL FindAERule (peval_t, LSZ, uint);
bool_t FASTCALL     UpdateEEHSTR(EEHSTR *, LSZ);
void FASTCALL   SaveFmtState (FMT_state *);
void FASTCALL   RestoreFmtState (FMT_state *);
bool_t FASTCALL     LookupAECache (CV_typ_t, HEXE, char *);
void FASTCALL   ReorderAECache (int);
void FASTCALL   InsertAECache (CV_typ_t, HEXE, char far *);

static char accessstr[4][4] = {"   ", "PV ", "PR ", "PB "};

struct typestr {
    uchar                               type; // type and subtype
    uchar                               len;
    char SEGBASED (_segname("_CODE")) * name;
};

struct modestr {
    uchar                               mode;
    uchar                               len;
    char SEGBASED (_segname("_CODE")) * name;
};

#pragma warning  ( disable:4120 )   // "based/unbased mismatch"

// define all the strings in the code segment
#define TYPENAME(name, type, len, str) static char SEGBASED(_segname("_CODE")) S##name[] = str;
#define MODENAME(name, mode, len, str) static char SEGBASED(_segname("_CODE")) S##name[] = str;
#define PTRNAME(name, str) static char SEGBASED(_segname("_CODE")) S##name[] = str;
#include "fmtstr.h"
#undef TYPENAME
#undef MODENAME
#undef PTRNAME

// load all the type names into a table
static struct typestr SEGBASED(_segname("_CODE")) nametype[] = {
    #define TYPENAME(name, type, len, str) {type, len, S##name},
    #define MODENAME(name, mode, len, str)
    #define PTRNAME(name, str)
    #include "fmtstr.h"
    #undef TYPENAME
    #undef MODENAME
    #undef PTRNAME
};

// load all the mode names into a table
static struct modestr SEGBASED(_segname("_CODE")) namemode[] = {
    #define TYPENAME(name, type, len, str)
    #define MODENAME(name, mode, len, str) {mode, len, S##name},
    #define PTRNAME(name, str)
    #include "fmtstr.h"
    #undef TYPENAME
    #undef MODENAME
    #undef PTRNAME
};

// load all the pointer type names into an array
static char SEGBASED(_segname("_CODE")) *SEGBASED(_segname("_CODE")) ptrname[] = {
    #define TYPENAME(name, type, len, str)
    #define MODENAME(name, mode, len, str)
    #define PTRNAME(name, str) S##name,
    #include "fmtstr.h"
    #undef TYPENAME
    #undef MODENAME
    #undef PTRNAME
};

#pragma warning  ( default:4120 )   // "based/unbased mismatch"


#define typecount  (sizeof (nametype) / sizeof (nametype[0]))
#define modecount  (sizeof (namemode) / sizeof (namemode[0]))

char *fmt_char_nz[] = {
    "0%03.03o '%c'",
    "%d '%c'",
    "0x%02.02x '%c'",
    "0x%02.02X '%c'",
};

// The IDE wants to show a null character as '\x00', but CV wants to show
// it as ''

#ifdef WINQCXX
#define szNullChar "\\x00"
#else
#define szNullChar ""
#endif

char *fmt_char_zr[] = {
    "0%03.03o '" szNullChar "'",
    "%d '" szNullChar "'",
    "0x%02.02x '" szNullChar "'",
    "0x%02.02X '" szNullChar "'",
};


char *fmt_char[] = {
    "0%03.03o",
    "%d",
    "0x%02.02x",
    "0x%02.02X",
};


char *fmt_short[] = {
    "0%06.06ho",
    "%hd",
    "0x%04.04hx",
    "0x%04.04hX",
};


char *fmt_ushort[] = {
    "0%06.06ho",
    "%hu",
    "0x%04.04hx",
    "0x%04.04hX",
};


char *fmt_long[] = {
    "0%011.011lo",
    "%ld",
    "0x%08.08lx",
    "0x%08.08lX",
};


char *fmt_ulong[] = {
    "0%011.011lo",
    "%lu",
    "0x%08.08lx",
    "0x%08.08lX",
};

char *fmt_quad[] = {
    "0%022.022I64o",
    "%I64d",
    "0x%016.016I64x",
    "0x%016.016I64X",
};

char *fmt_uquad[] = {
    "0%022.022I64o",
    "%I64u",
    "0x%016.016I64x",
    "0x%016.016I64X",
};

char *fmt_ptr_16_16[] = {
    "0x%04hx:0x%04hx",
    "0x%04hX:0x%04hX",
};


char *fmt_ptr_16_32[] = {
    "0x%04hx:0x%08lx",
    "0x%04hX:0x%08lX",
};


char *fmt_ptr_0_16[] = {
    "0x%04hx",
    "0x%04hX",
};


char *fmt_ptr_0_32[] = {
    "0x%08lx",
    "0x%08lX",
};

char *fmt_ptr_64[] = {
    "0x%016I64x",
    "0x%016I64X",
};

char *bailout = "\x006""??? * ";

// on some locales (eg German) use commas instead of periods in floats.
// This confuses the EE greatly as it thinks the commas are part of the format specifier

int sprintf_english( char *buffer, const char *fmtStr, ... )
{
        int cResult;
        va_list marker;
        va_start( marker, fmtStr );

        setlocale( LC_NUMERIC, "C" );

        cResult = vsprintf( buffer, fmtStr, marker );

        setlocale( LC_NUMERIC, "" );

        va_end( marker );

        return cResult;
}

/**     FormatCXT - format context packet
 *
 *      status = FormatCXT (pCXT, phStr, dwFlags);
 *
 *      Entry   pCXT = pointer to context
 *              phStr = return location for string
 *              dwFlags = control flags for formatting
 *
 *      Exit    context formatted into buffer as a context operator
 *
 *      Returns EENONE if no error
 *              EEGENERAL if error
 */

ulong  FormatCXT (PCXT pCXT, PEEHSTR phStr, DWORD dwFlags)
{
    HMOD        hMod;
    HPROC       hProc;
    HSF         hsf;
    HEXE        hExe;
    SYMPTR      pProc;
    char       *pFile;
    char       *pExe;
    char       *pStr;
    uint        len = 6;
    eval_t      eval;
    peval_t     pv = &eval;
    char        buf[FCNSTRMAX + sizeof(HDR_TYPE)];
    char       *pName = buf;
    char        procName[NAMESTRMAX];
    char       *pFName;
    char       *pc = 0;
    uint        lenP;
    CV_typ_t    typ;
    PHDR_TYPE   pHdr;

    hMod = SHHMODFrompCXT (pCXT);
    hProc = SHHPROCFrompCXT (pCXT);
    if ((hMod == 0) && (hProc == 0)) {
        if ((*phStr = MemAllocate (10)) != 0) {
            pStr = (char *) MemLock (*phStr);
            *pStr = 0;
            MemUnLock (*phStr);
            return (EENOERROR);
        }

    }

    hsf = SLHsfFromPcxt (pCXT);
    hExe = SHHexeFromHmod (hMod);

    if (dwFlags & HCXTFMT_Short) {
        pExe = SHGetModNameFromHexe( hExe );
    }
    else {
        pExe = SHGetExeName( hExe );
    }

    if ( pExe == NULL) {

        // we can't generate a CXT if we can't get the exe name
        return (EECATASTROPHIC);
    }

    pFile = SLNameFromHsf (hsf) ;
    // it is possible to get the exe name, but not source
    // file/line information (ex. a public)  In this case we will use
    // pExe instead of pFile (ie. {,,foob.exe} )

    if (hProc != 0) {
        switch ((pProc = (SYMPTR)MHOmfLock ((HSYM)hProc))->rectyp) {
        case S_LPROC16:
        case S_GPROC16:
            lenP = ((PROCPTR16)pProc)->name[0];
            pFName = (char *) &((PROCPTR16)pProc)->name[1];
            typ = ((PROCPTR16)pProc)->typind;
            break;

        case S_LPROC32:
        case S_GPROC32:
            lenP = ((PROCPTR32)pProc)->name[0];
            pFName = (char *) &((PROCPTR32)pProc)->name[1];
            typ = ((PROCPTR32)pProc)->typind;
            break;

        case S_LPROCMIPS:
        case S_GPROCMIPS:
            lenP = ((PROCPTRMIPS)pProc)->name[0];
            pFName = (char *) &((PROCPTRMIPS)pProc)->name[1];
            typ = ((PROCPTRMIPS)pProc)->typind;
            break;

        case S_LPROCIA64:
        case S_GPROCIA64:
            lenP = ((PROCPTRIA64)pProc)->name[0];
            pFName = (char *) &((PROCPTRIA64)pProc)->name[1];
            typ = ((PROCPTRIA64)pProc)->typind;
            break;

        default:
            DASSERT (FALSE);

        //
                // Perhaps this should be handled differently.  The situation is that
                // a thunk has MOD and PROC context, but no typind.
        //
        case S_THUNK32:
            MHOmfUnLock (hProc);
            return (EECATASTROPHIC);
        }

        _tcsncpy(procName, pFName, lenP);
        procName[lenP] = '\0';
        EVAL_MOD(pv) = hMod;
        pv->CXTT = *pCXT;
        SetNodeType(pv, typ);

        pHdr = (PHDR_TYPE)pName;
        memset (pName, 0, FCNSTRMAX + sizeof (HDR_TYPE));
        pName = pName + sizeof (HDR_TYPE);
        pc = pName;
        lenP = FCNSTRMAX - 1;
        pFName = procName;
        // set selection mask in order to supress formatting
        // of the proc return type
        FormatType (pv, &pName, &lenP, &pFName, 0x1L, pHdr);
        lenP = FCNSTRMAX - lenP;

        // ignore buffer header from FormatType

        memmove ((char *)pHdr, pc, lenP);
        pc = (char *)pHdr;
        len += lenP;
    }

    if ( pFile ) {
        len += (UINT)(BYTE)*pFile + (int)_tcslen (pExe) ;
    }
    else {
        len += (int)_tcslen (pExe) ;
    }

    if ((*phStr = MemAllocate (len)) != 0) {
        pStr = (char *) MemLock (*phStr);
        if (dwFlags & HCXTFMT_Short) {
            _tcscpy(pStr, pExe);
            _tcsupr(pStr);
            _tcscat(pStr, "!");
        }
        else {
            _tcscpy (pStr, "{");
            if (hProc != 0 && !(dwFlags & HCXTFMT_No_Procedure)) {
                _tcscat (pStr, pc);
            }
            _tcscat (pStr, ",");

            if ( pFile ) {
                _tcsncat (pStr, pFile + 1, (UINT)(BYTE)*pFile);
            }

            _tcscat (pStr, ",");
            _tcscat (pStr, pExe);
            _tcscat (pStr, "}");
        }
        MemUnLock (*phStr);
    }
    else {
        MHOmfUnLock (hProc);
        return (EECATASTROPHIC);
    }
    MHOmfUnLock (hProc);
    return (EENOERROR);
}


/**     AppendCVQualifier - append "const"/"volatile" qualifier
 *
 *      AppendCVQualifier (pv, buf, buflen);
 *
 *      Entry   pv = pointer to value node
 *              buf = pointer pointer to buffer
 *              buflen = pointer to buffer length
 *
 *      Exit    type qualifier (const / volatile) appended to buffer
 *              *buflen = space remaining in buffer
 *
 *      Returns none
 */

void AppendCVQualifier (peval_t pv, char * *buf, uint *buflen)
{
    uint        len;

    if (EVAL_IS_CONST (pv)) {
        len = min (*buflen, 6);
        _tcsncpy (*buf, "const ", len);
        *buf += len;
        *buflen -= len;
    }
    else if (EVAL_IS_VOLATILE (pv)) {
        len = min (*buflen, 9);
        _tcsncpy (*buf, "volatile ", len);
        *buf += len;
        *buflen -= len;
    }
}


/**     FormatType - format type string
 *
 *      FormatType (pv, ppbuf, pcount, ppName, select, pHdr);
 *
 *      Entry   pv = pointer to value node
 *              ppbuf = pointer pointer to buffer
 *              pcount = pointer to buffer length
 *              ppName = pointer to name if not null
 *              select = selection mask
 *              pHdr = pointer to structure describing formatting
 *
 *      Exit    type formatted into buffer
 *              *pcount = space remaining in buffer
 *              *pHdr updated
 *
 *      Returns none
 */

void FormatType (peval_t pv, char * *buf, uint *buflen,
  char * *ppName, ulong select, PHDR_TYPE pHdr)
{
    eval_t      evalT;
    peval_t     pvT = &evalT;
    uint        skip = 1;
    uint        len;
    int         itype, imode;
    char *tname;
    CV_typ_t    type;

#if defined (NEVR)
    if (EVAL_ACCESS (pv) != 0) {
        len = min (*buflen, 3);
        _tcsncpy (*buf, accessstr[EVAL_ACCESS (pv)], len);
        *buf += len;
        *buflen -= len;
    }
#endif
    if (!EVAL_IS_PTR (pv)) {
        AppendCVQualifier(pv, buf, buflen);
    }

    if (CV_IS_PRIMITIVE (EVAL_TYP (pv))) {
        switch (EVAL_TYP (pv)) {
            default:
                // determine type

                for (itype = 0; itype < typecount; itype++) {
                    if (nametype[itype].type == (EVAL_TYP (pv) & (CV_TMASK|CV_SMASK)))
                        break;
                }

                // determine mode (e.g. direct, near ptr, etc.)

                for (imode = 0; imode < modecount; imode++) {
                    if (namemode[imode].mode == CV_MODE (EVAL_TYP (pv)))
                        break;
                }

                // if the whole type consists of the matched type & mode,
                // then we have a match
                if (EVAL_TYP (pv) ==
                        (CV_typ_t) ((nametype[itype].type) |
                                    (namemode[imode].mode << CV_MSHIFT))) {
                    // copy type string

                    len = min (*buflen, nametype[itype].len);
                    _tcsncpy (*buf, nametype[itype].name, len);
                    *buf += len;
                    *buflen -= len;

                    // copy mode string

                    len = min (*buflen, namemode[imode].len);
                    _tcsncpy (*buf, namemode[imode].name, len);
                    *buf += len;
                    *buflen -= len;
                }
                else {
                    // no match
                    len = min (*buflen, (sizeof("??? ") - 1));
                    _tcsncpy (*buf, "??? ", len);
                    *buf += len;
                    *buflen -= len;
                }

                break;

            case T_NCVPTR:
                tname = "\x007""near * ";
                goto formatmode;

            case T_HCVPTR:
                tname = "\x007""huge * ";
                goto formatmode;

            case T_FCVPTR:
                tname = "\x006""far * ";
                goto formatmode;

            case T_32NCVPTR:
            case T_32FCVPTR:
            case T_64NCVPTR:
                tname = "\x002""* ";

formatmode:
                AppendCVQualifier(pv, buf, buflen);
                *pvT = *pv;
                SetNodeType (pvT, PTR_UTYPE (pv));
                type = EVAL_TYP (pvT);
                if (CV_IS_INTERNAL_PTR (type)) {
                    // we are in a bind here.  The type generator messed
                    // up and generated a created type pointing to a created
                    // type.  we are going to bail out here.

                    len = min (*buflen, (uint)*bailout);
                    _tcsncpy (*buf, bailout + 1, len);
                    *buflen -= len;
                    *buf += len;

                }
                else {
                    FormatType (pvT, buf, buflen, 0, select, pHdr);
                }
                len = min (*buflen, (uint)*tname);
                _tcsncpy (*buf, tname + 1, len);
                *buflen -= len;
                *buf += len;
                break;
        }
    }
    else {
#ifdef NEVER
        if (FormatUDT (pv, buf, buflen) == FALSE)
#endif
        FormatExpand (pv, buf, buflen, ppName, select, pHdr);
    }
    if (ppName != NULL && *ppName != NULL) {
        len = (int)_tcslen (*ppName);
        len = min (len, *buflen);
        pHdr->offname = (ulong)(*buf - (char *)pHdr - sizeof (HDR_TYPE));
        pHdr->lenname = len;
        _tcsncpy (*buf, *ppName, len);
        *buflen -= len;
        *buf += len;
        *ppName = NULL;
    }
}



/**     FormatExpand - format expanded type definition
 *
 *      FormatExpand (pv, ppbuf, pbuflen, ppName, select)
 *
 *      Entry   pv = pointer to value node
 *              ppbuf = pointer to pointer to buffer
 *              pbuflen = pointer to space remaining in buffer
 *              ppName = pointer to name to insert after type if not null
 *              select = selection mask
 *              pHdr = pointer to type formatting header
 *
 *      Exit    buffer contains formatted type
 *              ppbuf = end of formatted string
 *              pbuflen = space remaining in buffer
 *
 *      Returns none
 */


void
FormatExpand (
    peval_t pv,
    char * *buf,
    uint *buflen,
    char * *ppName,
    ulong select,
    PHDR_TYPE pHdr
    )
{
    eval_t      evalT;
    peval_t     pvT = &evalT;
    uint        skip = 1;
    uint        len;
    HTYPE       hType;
    plfEasy     pType;
    unsigned    model;
    ulong       count;
    char        tempbuf[33];
    CV_typ_t    rvtype;
    CV_typ_t    mclass;
    CV_call_e   call;
    ulong       cparam;
    CV_typ_t    parmtype;
    CV_typ_t    thistype;
    char       *movestart;
    uint        movelen;
    uint        movedist;
    HEXE        hExe;
    long        nTypeSize;

    if ((hType = THGetTypeFromIndex (EVAL_MOD (pv), EVAL_TYP (pv))) == 0) {
        return;
    }
    pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
    switch (pType->leaf) {
        case LF_STRUCTURE:
        case LF_CLASS:
            skip = offsetof (lfClass, data);
            RNumLeaf (((char *)(&pType->leaf)) + skip, &skip);
            len = *(((char *)&(pType->leaf)) + skip);
            len = min (len, *buflen);
            _tcsncpy (*buf, ((char *)pType) + skip + 1, len);
            *buflen -= len;
            *buf += len;
            if (*buflen > 1) {
                **buf = ' ';
                (*buf)++;
                (*buflen)--;
            }
            MHOmfUnLock (hType);
            break;

        case LF_UNION:
            skip = offsetof (lfUnion, data);
            RNumLeaf (((char *)(&pType->leaf)) + skip, &skip);
            len = *(((char *)&(pType->leaf)) + skip);
            len = min (len, *buflen);
            _tcsncpy (*buf, ((char *)pType) + skip + 1, len);
            *buflen -= len;
            *buf += len;
            if (*buflen > 1) {
                **buf = ' ';
                (*buf)++;
                (*buflen)--;
            }
            MHOmfUnLock (hType);
            break;

        case LF_ENUM:
            skip = offsetof (lfEnum, Name);
            len = ((plfEnum)pType)->Name[0];
            len = min (len, *buflen);
            _tcsncpy (*buf, (char *) &((plfEnum)pType)->Name[1], len);
            *buflen -= len;
            *buf += len;
            if (*buflen > 1) {
                **buf = ' ';
                (*buf)++;
                (*buflen)--;
            }
            MHOmfUnLock (hType);
            break;

        case LF_POINTER:
            // we're going to be looking for this real soon
            // put it in the cache now...
            hExe = SHHexeFromHmod (EVAL_MOD (pv));
            InsertCache(EVAL_TYP(pv), hExe);

            // set up a node to evaluate this field
            model = ((plfPointer)pType)->attr.ptrtype;
            // format the underlying type
            *pvT = *pv;
            EVAL_TYP (pvT) = ((plfPointer)pType)->utype;


            MHOmfUnLock (hType);
            SetNodeType (pvT, EVAL_TYP (pvT));
            FormatType (pvT, buf, buflen, 0, select, pHdr);
            len = min (*buflen, (uint)*ptrname[model]);
            _tcsncpy (*buf, ptrname[model] + 1, len);
            *buflen -= len;
            *buf += len;
            if (((plfPointer)pType)->attr.ptrmode == CV_PTR_MODE_REF) {
                // this is a reference type -- we must fix the string
                // that we just formatted so that it looks like a reference
                // and not a pointer [rm]

                DASSERT((*buf)[-1] == ' ');
                DASSERT((*buf)[-2] == '*');
                (*buf)[-2] = '&';
            }
            // const/volatile qualifier for the pointer type
            // should be appended after the '*'
            AppendCVQualifier (pv, buf, buflen);
            break;

        case LF_ARRAY:
            *pvT = *pv;
            EVAL_TYP (pvT) = ((plfArray)(&pType->leaf))->elemtype;
            skip = offsetof (lfArray, data);
            count = (ulong) RNumLeaf (((char *)(&pType->leaf)) + skip, &skip);
            MHOmfUnLock (hType);
            SetNodeType (pvT, EVAL_TYP (pvT));
            // continue down until the underlying type is reached

            FormatType (pvT, buf, buflen, ppName, select, pHdr);
            if ((ppName != NULL) && (*ppName != NULL)) {
                len = _tcslen (*ppName);
                len = min (len, *buflen);
                pHdr->offname = (ulong)(*buf - (char *)pHdr - sizeof (HDR_TYPE));
                pHdr->lenname = len;
                _tcsncpy (*buf, *ppName, len);
                *buflen -= len;
                *buf += len;
                *ppName = NULL;
            }

            // display size of array or * if size unknown.  We have to
            // move the trailing part of the string down if it already
            // set so that the array dimensions come out in the proper
            // order

            nTypeSize = TypeSize (pvT);
            if (count != 0 && nTypeSize != 0) {
                ultoa (count / nTypeSize, tempbuf, 10);
                len = _tcslen (tempbuf);
            }
            else {
                *tempbuf = '?';
                *(tempbuf + 1) = 0;
                len = 1;
            }
            if (*buflen >= 2) {
                if (pHdr->offtrail == 0) {
                    pHdr->offtrail = (ulong)(*buf - (char *)pHdr - sizeof (HDR_TYPE));
                    movestart = (char *)pHdr + sizeof (HDR_TYPE) + pHdr->offtrail;
                    movelen = 0;
                    movedist = 0;
                }
                else {
                    movestart = (char *)pHdr + sizeof (HDR_TYPE) + pHdr->offtrail;
                    movelen = _tcslen (movestart);
                    movedist = _tcslen (tempbuf) + 2;
                    movelen = min (*buflen, movelen);
                    memmove (movestart + movedist, movestart, movelen);
                }
                *movestart++ = '[';
                memmove (movestart, tempbuf, len);
                movestart += len;
                *movestart++ = ']';
                *buf += len + 2;
                *buflen -= len + 2;
            }
            break;

        case LF_PROCEDURE:
            mclass = 0;
            rvtype = ((plfProc)pType)->rvtype;
            call = (CV_call_e) ((plfProc)pType)->calltype;
            cparam = ((plfProc)pType)->parmcount;
            parmtype = ((plfProc)pType)->arglist;
            MHOmfUnLock (hType);
            FormatProc (pv, buf, buflen, ppName, rvtype, mclass, call,
                        cparam, parmtype, select, pHdr);
            break;

        case LF_MFUNCTION:
            rvtype = ((plfMFunc)pType)->rvtype;
            mclass = ((plfMFunc)pType)->classtype;
            call = (CV_call_e) ((plfMFunc)pType)->calltype;
            thistype = ((plfMFunc)pType)->thistype;
            cparam = ((plfMFunc)pType)->parmcount;
            parmtype = ((plfMFunc)pType)->arglist;
            MHOmfUnLock (hType);
            FormatProc (pv, buf, buflen, ppName, rvtype, mclass, call,
                        cparam, parmtype, select, pHdr);
            break;

        case LF_MODIFIER:
#ifdef NEVER
        // Disabled. We emitted const in the wrong order if the following
        // node was a pointer. Let FormatType handle this [dolphin #5043]
            if (*buflen >= 6) {
                if (((plfModifier)pType)->attr.MOD_const == TRUE) {
                    _tcsncpy (*buf, "const ", 6);
                    *buf += 6;
                    *buflen -= 6;
                }
            }
            if (*buflen >= 9) {
                if (((plfModifier)pType)->attr.MOD_volatile == TRUE) {
                    _tcsncpy (*buf, "volatile ", 9);
                    *buf += 9;
                    *buflen -= 9;
                }
            }
            EVAL_TYP (pv) = ((plfModifier)pType)->type;
#endif
            MHOmfUnLock (hType);
            SetNodeType (pv, EVAL_TYP (pv));
            FormatType (pv, buf, buflen, ppName, select, pHdr);
            break;

        case LF_LABEL:
            MHOmfUnLock (hType);
            break;

        default:
            MHOmfUnLock (hType);
            break;
    }
}

#ifdef NEVER
bool_t FormatUDT (peval_t pv, char * *buf,
  uint *buflen)
{
// Slow ineffective search disabled  jsg 2/1/92
//
// This code would search all symbols for typedefs to the current type index.
// This was making the local window repaint too sluggish, since 'FormatUDT'
// can be called many times per line.
//
#if NEVER
    search_t    Name;
    UDTPTR      pUDT;
    uint        len;

    //  Format typedef string

    InitSearchtDef (&Name, pv, SCP_module | SCP_global);
    if (SearchSym (&Name) == HR_found) {
        pUDT = (UDTPTR)MHOmfLock (Name.hSym);
        len = pUDT->name[0];
        len = min (len, *buflen);
        _tcsncpy (*buf, &pUDT->name[1], len);
        *buflen -= len;
        *buf += len;
        if (*buflen > 1) {
            **buf = ' ';
            (*buf)++;
            (*buflen)--;
        }
        MHOmfUnLock (Name.hSym);
        return (TRUE);
    }
#endif
    if (pExState != NULL)
        pExState->err_num = ERR_NONE;
    return (FALSE);
}
#endif






/**     FormatProc - format proc or member function
 *
 *      FormatProc (pv. buf, buflen, ppName, rvtype, mclass, call, cparam, paramtype, select)
 */

void
FormatProc (
    peval_t pv,
    char * *buf,
    uint *buflen,
    char * *ppName,
    CV_typ_t rvtype,
    CV_typ_t mclass,
    CV_call_e call,
    ulong  cparam,
    CV_typ_t paramtype,
    ulong select,
    PHDR_TYPE pHdr
    )
{
    eval_t      evalT;
    peval_t     pvT;
    HTYPE       hArg;
    plfArgList  pArg;
    ulong       noffset = 1;
    int         len;
    bool_t      farcall;
    ulong       argCnt;
    ulong       saveOfftrail = pHdr->offtrail;

    Unreferenced( mclass );

    pvT = &evalT;
    *pvT = *pv;

    if (GettingChild == FALSE && (select & 0x00000001) == FALSE ) {
        // output function return type if we are not getting a child TM.
        // If we are getting a child tm and the function type is included,
        // the subsequent parse of the generated expression will fail
        // because the parse cannot handle
        //      type fcn (..........

        // OR...
        // if select == 0x1 then this is a request to format procs
        // without the return type (for BPs etc)

        EVAL_TYP (pvT) = (rvtype == 0)? T_VOID: rvtype;
        FormatType (pvT, buf, buflen, NULL, select, pHdr);
        //M00KLUDGE - need to output call and model here
        switch (call) {
            case CV_CALL_NEAR_C:
                //near C call - caller pops stack
                call = (CV_call_e) FCN_C;
                farcall = FALSE;
                break;

            case CV_CALL_FAR_C:
                // far C call - caller pops stack
                call = (CV_call_e) FCN_C;
                farcall = TRUE;
                break;

            case CV_CALL_NEAR_PASCAL:
                // near pascal call - callee pops stack
                call = (CV_call_e) FCN_PASCAL;
                farcall = FALSE;
                break;

            case CV_CALL_FAR_PASCAL:
                // far pascal call - callee pops stack
                call = (CV_call_e) FCN_PASCAL;
                farcall = TRUE;
                break;

            case CV_CALL_NEAR_FAST:
                // near fast call - callee pops stack
                call = (CV_call_e) FCN_FAST;
                farcall = FALSE;
                break;

            case CV_CALL_FAR_FAST:
                // far fast call - callee pops stack
                call = (CV_call_e) FCN_FAST;
                farcall = TRUE;
                break;

            case CV_CALL_NEAR_STD:
                // near fast call - callee pops stack
                call = (CV_call_e) FCN_STD;
                farcall = FALSE;
                break;

            case CV_CALL_FAR_STD:
                // far fast call - callee pops stack
                call = (CV_call_e) FCN_STD;
                farcall = TRUE;
                break;

            case CV_CALL_THISCALL:
                // this goes in a reg - callee pops stack
                call = (CV_call_e) FCN_THISCALL;
                farcall = TRUE;
                break;

            case CV_CALL_MIPSCALL:
                call = (CV_call_e) FCN_MIPS;
                farcall = TRUE;
                break;

            case CV_CALL_ALPHACALL:
                call = (CV_call_e) FCN_ALPHA;
                farcall = TRUE;
                break;

            case CV_CALL_PPCCALL:
                call = (CV_call_e) FCN_PPC;
                farcall = TRUE;
                break;

            case CV_CALL_IA64CALL:
                call = (CV_call_e) FCN_IA64;
                farcall = TRUE;
                break;

            default:
                DASSERT (FALSE);
                call = (CV_call_e) 0;
                farcall = FALSE;
                break;

        }
    }

    // output function name

    if ((ppName != NULL) && (*ppName != NULL)) {
        len = (int)_tcslen (*ppName);
        len = __min (len, (int  ) *buflen);
        pHdr->offname = (ulong)(*buf - (char *)pHdr - sizeof (HDR_TYPE));
        pHdr->lenname = len;
        _tcsncpy (*buf, *ppName, len);
        *buflen -= len;
        *buf += len;
        *ppName = NULL;
    }
    if (*buflen > 1) {
        saveOfftrail = (ulong)(*buf - (char *)pHdr - sizeof (HDR_TYPE));
        **buf = '(';
        (*buf)++;
        (*buflen)--;
    }
    if (cparam == 0) {
        EVAL_TYP (pvT) = T_VOID;
        FormatType (pvT, buf, buflen, NULL, select, pHdr);
    }
    else {
        if ((hArg = THGetTypeFromIndex (EVAL_MOD (pv), paramtype)) == 0) {
            return;
        }
        argCnt = 0;
        while (argCnt < cparam) {
            pArg = (plfArgList)((&((TYPPTR)MHOmfLock (hArg))->leaf));
            EVAL_TYP (pvT) = pArg->arg[argCnt];
            MHOmfUnLock (hArg);
            FormatType (pvT, buf, buflen, NULL, select, pHdr);
            argCnt++;
            if ((argCnt < cparam) && (*buflen > 1)) {
                // strip trailing blanks after the arg we just formatted
                while (*((*buf)-1) == ' ') {
                    (*buf)--;
                    (*buflen)++;
                }
                // insert a comma and space if there are further arguments
                **buf = ',';
                (*buf)++;
                (*buflen)--;
                **buf = ' ';
                (*buf)++;
                (*buflen)--;
            }
        }
    }
    if (*buflen > 1) {
        // strip trailing blanks
        while (*((*buf)-1) == ' ') {
            (*buf)--;
            (*buflen)++;
        }
        // insert a closing parenthesis
        **buf = ')';
        (*buf)++;
        (*buflen)--;
    }
    pHdr->offtrail = saveOfftrail;
}




/**     FormatNode - format node according to format string
 *
 *      retval = FormatNode (phTM, radix, pFormat, phValue);
 *
 *      Entry   phTM = pointer to handle to TM
 *              radix = default radix for formatting
 *              pFormat = pointer to format string
 *              phValue = pointer to handle for display string
 *              fAutoExpand = flag that enables auto-expanding
 *
 *      Exit    evaluation result formatted
 *
 *      Returns EENOERROR if no error in formatting
 *              error number if error
 */


EESTATUS
FormatNode (
    PHTM phTM,
    uint Radix,
    PEEFORMAT pFormat,
    PEEHSTR phszValue,
    SHFLAG fAutoExpand
    )
{
    char        islong = FALSE;
    char        fc = 0;
    char *buf;
    int         buflen = FMTSTRMAX - 1;
    eval_t      evalT;
    peval_t     pv = &evalT;
    ulong       retval = EECATASTROPHIC;
    bool_t      fPtrAndString;

    DASSERT (*phTM != 0);
    if (*phTM == 0) {
        return (retval);
    }

    if (fAutoExpand) {
        // Prepare auto-expanded child TM List
        // We have to build the list here since the relevant
        // code needs a handle to the TM, not just a pointer
        // to the locked TM state
        DoAutoExpand( phTM );
    }

    if ((*phszValue = MemAllocate (FMTSTRMAX)) == 0) {
        // unable to allocate memory for formatting
        return (retval);
    }
    buf = (char *)MemLock (*phszValue);
    memset (buf, 0, FMTSTRMAX);
    pExState = (pexstate_t) MemLock (*phTM);
    pCxt = &pExState->cxt;

    // Get expression string
    pExStr = (char *)MemLock (pExState->hExStr);

    if (pExState->state.eval_ok == TRUE) {
        *pv = pExState->result;
        if ((EVAL_STATE (pv) == EV_lvalue) ||
          (EVAL_STATE (pv) == EV_type) ||
          (EVAL_STATE (pv) == EV_rvalue && EVAL_IS_PTR (pv))) {
            // do nothing
        }
        else {
            // this handles the case were the return result is a large
            // structure.

            pv =  &pExState->result;
        }
        if (EVAL_IS_REF (pv)) {
            if (!LoadSymVal (pv)) {
                // unable to load value
                goto formatexit;
            }
            EVAL_IS_REF (pv) = FALSE;
            EVAL_STATE (pv) = EV_lvalue;
            EVAL_SYM (pv) = EVAL_PTR (pv);
            SetNodeType (pv, PTR_UTYPE (pv));
        }
        if (EVAL_IS_CLASS (pv)) {
            // For structures and classes ignore format string and format
            // according to element data types

            EVAL_STATE (pv) = EV_rvalue;
            goto format;
        }

        // load value and format according to format string


        if ((EVAL_STATE (pv) == EV_type) || !LoadSymVal (pv)) {
            // unable to load value
            retval = EEGENERAL;
            if (pExState->err_num == ERR_NONE) {
                pExState->err_num = ERR_NOTEVALUATABLE;
            }
            goto formatexit;
        }
        else {
            switch (VerifyFormat (pv, pFormat, &buf, &buflen, &fPtrAndString)) {
                case FMT_error:
                    retval = EEGENERAL;
                    pExState->err_num = ERR_FORMAT;
                    goto formatexit;

                case FMT_none:
                    goto format;

                case FMT_ok:
                    retval = EENOERROR;
                    goto formatexit;
            }
        }
    }
    else {
        // not evaluated, fail
        retval = EEGENERAL;
        pExState->err_num = ERR_NOTEVALUATABLE;
        goto formatexit;
    }

format:
    retval = Format (pv, Radix, &buf, &buflen, fPtrAndString);
    if ( ( retval == EENOERROR ) && fAutoExpand && EVAL_IS_PTR (pv)) {
        FormatAutoExpand (pv, Radix, &buf, &buflen);
    }
formatexit:
    MemUnLock (pExState->hExStr);
    MemUnLock (*phszValue);
    MemUnLock (*phTM);
    return (retval);
}



/**     ExtendToQuad - convert a scalar to a quad
 *
 *      If the input node is of a scalar type, convert it to type T_QUAD,
 *      for display purposes.
 *
 *      (This routine replaced "ExtendToLong" in order to support __int64)
 */

void ExtendToQuad (peval_t pv, bool_t fUnsigned)
{
    CV_typ_t type = EVAL_TYP(pv);

    if (CV_IS_PRIMITIVE (type) &&
        CV_TYP_IS_DIRECT (type) &&
        !CV_TYP_IS_REAL (type) &&
        !CV_TYP_IS_COMPLEX (type)) {

        int size = TypeSizePrim (type);

        // CUDA #4028 [rm]
        // if the incoming type is unsigned, we don't want to sign extend
        // all the way to long even if the user asked us to display a signed
        // integer.  Perserving the unsigned-ness of the base type prevents
        // us from display (unsigned char)255,d as -1.  We would display
        // correctly display 0xffffffff,d as -1 however

        fUnsigned |= CV_TYP_IS_UNSIGNED(type);

        if (fUnsigned) {
            switch (size) {
                case 1:
                    EVAL_QUAD(pv) = EVAL_UCHAR (pv);
                    break;
                case 2:
                    EVAL_QUAD (pv) = EVAL_USHORT (pv);
                    break;
                case 4:
                    EVAL_QUAD (pv) = EVAL_ULONG (pv);
                    break;
            }
        }
        else {
            switch (size) {
                case 1:
                    EVAL_QUAD (pv) = EVAL_CHAR (pv);
                    break;

                case 2:
                    EVAL_QUAD (pv) = EVAL_SHORT (pv);
                    break;

                case 4:
                    EVAL_QUAD (pv) = EVAL_LONG (pv);
                    break;
            }
        }
    }
}

/**     VerifyFormat -
 *
 *
 */

static char stPrefixQuad[] = "\x003" "I64";

FMT_ret
VerifyFormat (
    peval_t pv,
    PEEFORMAT pFmtIn,
    char * *buf,
    int   *buflen,
    bool_t * pfPtrAndString
    )
{
    static      const double MAXFP = 1e40; //max value printed in %f format
    char        tempbuf[128];  //large enough to hold a nice big HRESULT string but not bigger than *buflen
    char        prefix = 0;
    char        postfix = 0;
    char        fmtchar = 0;
    int         nfmtcnt = 0;
    ulong       size = 0;
    char *pf;
    char        fmtstr[10];
    ADDR        addr;
    ulong       cnt;
    uint        fHexUpper;
    CV_typ_t    PtrToCharType = pExState->state.f32bit ? T_32PCHAR : T_PFCHAR;
    bool_t      fUnicode = FALSE;

    DASSERT (*buflen >= sizeof(tempbuf));
    if (EVAL_TYP (pv) == T_VOID) {
        // if the value is void, ignore all formatting
        _tcscpy (*buf, "<void>");
        *buflen -= 6;
        *buf += 6;
        return (FMT_ok);
    }
    if (pFmtIn) {
        pf = pFmtIn;
    } else {
        pf = &pExStr[pExState->strIndex];
    }
    if (*pf == ',') {
        pf++;
    }
    while ((*pf != 0) && ((*pf == ' ') || (*pf == '\t'))) {
        pf++;
    }

//BUGBUG: is this really 68k specific -- if so delete it.

#if defined(TARGMAC68K)
    if ((pFmtIn != NULL) && (*pf == 'p') && (EVAL_IS_ADDR(pv))) {
        if (ADDR_IS_LI (EVAL_PTR(pv))) {
            SHFixupAddr (&(EVAL_PTR(pv)));
        }
    }
#endif

    if (*pf != '\0') {
        // use the format from the command
        *pfPtrAndString = FALSE;
    }
    else if ((pFmtIn != NULL) && (*pf == 'p')) {
        // do not add string to pointer display
        *pfPtrAndString = FALSE;
        return (FMT_none);
    }
    else {
        // add string to pointer display
        *pfPtrAndString = TRUE;
        return (FMT_none);
    }
    size = (ulong )TypeSize (pv);
    if (*pf != 0) {
        // extract the prefix character if it exists

        switch (*pf) {
            case 'h':
            case 'l':
            case 'L':
                prefix = *pf++;
                break;
            case 'I':
                if (!_tcsncmp(pf, &stPrefixQuad[1], stPrefixQuad[0])) {
                    prefix = *pf;
                    pf += stPrefixQuad[0];
                }
                break;
        }

        // extract the format character

        switch (*pf) {
            case 'd':
            case 'i':
            case 'u':
            case 'o':
                if (prefix == 0) {
                    prefix = 'I';
                }
                fmtchar = *pf++;
                break;

            case 'f':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
                if (prefix == 'h') {
                    return (FMT_error);
                }

                // force suitable prefix according to the type of data
                switch (EVAL_TYP (pv)) {
                case T_REAL32:
                    prefix = 0;
                    break;
                case T_REAL64:
                    prefix = 'l';
                    break;
                case T_REAL80:
                    prefix = 'L';
                    break;
                default:
                    return (FMT_error);
                }

                fmtchar = *pf++;
                break;

            case 'c':
                if (prefix != 0) {
                    return (FMT_error);
                }
                fmtchar = *pf++;
                break;

            case 'm':
                if (prefix != 0) {
                    return (FMT_error);
                }
                fmtchar = *pf++;
                if (*pf == 0) {
                    // assume byte display
                    postfix = (char) 'b';
                }
                else {
                    switch (postfix = *pf++) {
                    case 'b':
                    case 'w':
                    case 'd':
                    case 'a':
                    case 'u':
                                        case 'q':
                        break;
                    default:
                        if (_istspace((_TUCHAR)postfix))
                            break;
                        else
                            return (FMT_error);
                    }
                }
                break;

            case 's':
                if (prefix != 0 && prefix != 'l' && prefix != 'L') {
                    return (FMT_error);
                }
                fmtchar = *pf++;
                if (prefix == 'l' || prefix =='L') {
                    fUnicode = TRUE;
                } else
                if (*pf == 'u') {
                    fUnicode = TRUE;
                    pf ++;
                } else
                if (*pf == 't') {
                    fUnicode = GetUnicodeStrings();
                    pf ++;
                } else
                if (T_USHORT == (EVAL_TYP(pv) & (CV_TMASK | CV_SMASK) ) ) {
                    // If we are pointing to an unisgned short
                    // this is a unicode string.
                    fUnicode = TRUE;
                }
                break;

            case 'x':
            case 'X':
                if (prefix == 0) {
                    prefix = 'I';

                    // two hex digits for each byte of the value; since we're
                    // formatting with %lx, there's no point in having a size
                    // greater than 4

                    // __int64 support: we're now formatting with %I64x
                    // and the max size can be 8 bytes
                    nfmtcnt = min(size,8) * 2;
                }
                if (nfmtcnt == 0) {
                    if (prefix == 'h')
                        nfmtcnt = 4;
                    else if (prefix == 'I')
                        nfmtcnt = 16;
                    else
                        nfmtcnt = 8;
                }
                fmtchar = *pf++;
                fHexUpper = (fmtchar == 'X');
                break;

            case 'r':
                if (prefix != 'h') {
                    return FMT_error;
                }
                fmtchar = *pf++;
                break;

            case 'p':
                fmtchar = *pf++;
                break;

            default:
                return (FMT_error);
        }
        while (_istspace((_TUCHAR)*pf))
            pf++;
        if (*pf != 0) {
            return (FMT_error);
        }

        if (EVAL_IS_ENUM (pv)) {
            SetNodeType (pv, ENUM_UTYPE (pv));
        }

        pf = fmtstr;
        *pf++ = '%';
        if (nfmtcnt != 0) {
            pf += sprintf (pf, "%d", nfmtcnt);
        }
        if (prefix == 'I') {
            _tcsncpy (pf, &stPrefixQuad[1], stPrefixQuad[0]);
            pf += stPrefixQuad[0];
        }
        else if (prefix != 0) {
            *pf++ = prefix;
        }
        *pf++ = fmtchar;
        *pf = 0;

        switch (fmtchar) {
            case 'd':
            case 'i':
            case 'u':
                ExtendToQuad (pv, (fmtchar == 'u'));
                cnt = sprintf (tempbuf, fmtstr, EVAL_QUAD (pv));
                break;

            case 'o':
                ExtendToQuad (pv, TRUE);
                pf = fmtstr;
                *pf++ = '0';
                *pf++ = '%';
                if (nfmtcnt != 0) {
                    pf += sprintf (fmtstr, "%d", nfmtcnt);
                }
                if (prefix == 'I') {
                    _tcsncpy (pf, &stPrefixQuad[1], stPrefixQuad[0]);
                    pf += stPrefixQuad[0];
                }
                else if (prefix != 0) {
                    *pf++ = prefix;
                }
                *pf++ = fmtchar;
                *pf = 0;
                cnt = sprintf (tempbuf, fmtstr, EVAL_QUAD (pv));
                break;

            case 'f':
                // prevent internal buffer overflow
                if (prefix == 'l' && EVAL_DOUBLE(pv) > MAXFP ||
                    prefix == 'L' && !Float10LessThanEqual (EVAL_LDOUBLE(pv), Float10FromDouble (MAXFP))) {
                        _tcscpy (fmtstr, prefix == 'l' ? "%le" : "%Le");
                    }
                    // fall through
            case 'e':
            case 'E':
            case 'g':
            case 'G':

                if (TargetMachine != mptmppc) {
                    switch (prefix) {
                        case 'l':
                            cnt = sprintf_english (tempbuf, fmtstr, EVAL_DOUBLE(pv));
                            break;
                        case 'L':
                            //BUGBUG: this seems suspect
    
#if defined(WIN32) && !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_) && !defined(_IA64_)
#pragma message("WARNING: Only one format for 10-byte reals")
                            cnt = _tcslen ( SzFromLd ( tempbuf, sizeof(tempbuf), EVAL_LDOUBLE(pv) ) );
#else
                            cnt = sprintf_english (tempbuf, fmtstr, EVAL_LDOUBLE(pv));
#endif
                            break;

                        default:
                            cnt = sprintf_english (tempbuf, fmtstr, EVAL_FLOAT(pv));
                            break;
                    }

                } else {
                    switch (CV_SUBT (EVAL_TYP (pv))) {
                        case CV_RC_REAL32:
                            cnt = sprintf (tempbuf, fmtstr, EVAL_FLOAT (pv));
                            break;
                        case CV_RC_REAL64:
                            cnt = sprintf (tempbuf, fmtstr, EVAL_DOUBLE (pv));
                            break;
                        default:
                            cnt = sprintf (tempbuf, fmtstr, EVAL_LDOUBLE (pv));
                            break;
                    }
                }
                break;

            case 'm':
                // Need to set Evaluating to 1 to force Normalization
                // of based ptrs in CastNode (and reset to 0 immediately
                // afterwards)

                Evaluating = TRUE;
                // CUDA #4044 : make sure we can do the cast... [rm]
                if (!CastNode (pv, PtrToCharType, PtrToCharType)) {
                    Evaluating = FALSE;
                    return (FMT_error);
                }
                EvalMem (pv, buf, buflen, postfix);
                Evaluating = FALSE;
                return (FMT_ok);

            case 's':
                if (EVAL_IS_ADDR (pv)) {
                    // Need to set Evaluating to 1 to force Normalization
                    // of based ptrs in CastNode (and reset to 0 immediately
                    // afterwards)

                    Evaluating = TRUE;
                    // CUDA #4044 : make sure we can do the cast... [rm]
                    if (!CastNode (pv, PtrToCharType, PtrToCharType)) {
                        Evaluating = FALSE;
                        return (FMT_error);
                    }
                    EvalString (pv, buf, buflen, fUnicode);
                    Evaluating = FALSE;
                    return (FMT_ok);
                }
                // if not an addr then just fall thru and display as
                // individual chars
                // sps - 9/14/92
                fmtchar = 'c';

            case 'c':
                pf = fmtstr;
                *pf++ = '\'';
                if (EVAL_CHAR (pv) != 0) {
                    // if the value is not zero, then display it.
                    // otherwise, display '' (CodeView) or '\x00' (IDE)
                    *pf++ = '%';
                    *pf++ = fmtchar;
                }
                else {
                    pf += sprintf(pf, szNullChar);
                }
                *pf++ = '\'';
                *pf = 0;
                cnt = sprintf (tempbuf, fmtstr, EVAL_CHAR (pv));
                break;

#ifdef USE_CUSTVIEW
                        case 'r':
                                {
                                        DWORD dwValue = EVAL_ULONG(pv);
                                        HRESULT hr = ExportViewHresult( dwValue,
                                                    &StaticHelper,
                                                    16 /* AMPHACK a lie */,
                                                    GetUnicodeStrings(),
                                                    tempbuf,
                                                    sizeof(tempbuf),
                                                    0 );
                                        if (hr!=S_OK) {
                                                sprintf( tempbuf, "0x%08lx", dwValue );
                    }
                                        cnt = _tcslen( tempbuf );
                                }
                                break;
#endif

            case 'x':
            case 'X':
            case 'p':
                if (EVAL_IS_PTR (pv)) {
                    addr = EVAL_PTR (pv);
                    if (ADDR_IS_LI (addr)) {
                        SHFixupAddr (&addr);
                    }
                    fHexUpper = !!fHexUpper; // canonicalize
#if defined(TARGMAC68K)
                    if(GetAddrSeg(addr) != 0)
                        {
                        cnt = sprintf (tempbuf, fmt_ptr_16_32[fHexUpper],
                        GetAddrSeg (addr), (CV_uoff32_t)GetAddrOff (addr));
                        }
                    else
                        {
                        cnt = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
                        (CV_uoff32_t)GetAddrOff (addr));
                        }
#else // !TARGMAC68K
                    // [cuda#4793 5/25/93 mikemo]  Don't use EVAL_IS_NPTR32
                    // and EVAL_IS_FPTR32 here, because they fail on arrays
                    if (EVAL_PTRTYPE (pv) == CV_PTR_NEAR32 ||
                        EVAL_PTRTYPE (pv) == CV_PTR_FAR32) {
                        cnt = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
                          (CV_uoff32_t)GetAddrOff (addr));
                    } else if (EVAL_PTRTYPE (pv) == CV_PTR_64) {
                        cnt = sprintf (tempbuf, fmt_ptr_64[fHexUpper],
                          GetAddrOff (addr));
                    } 
                    else {
                        // if it is a near ptr we will treat is as a ptr
                        // since we always carry around the seg & offset
                        // even if it is near.

                        cnt = sprintf (tempbuf, fmt_ptr_16_16[fHexUpper],
                          GetAddrSeg (addr), (CV_uoff16_t)GetAddrOff (addr));
                    }
#endif // TARGMAC68K
                }
                else {
                    ExtendToQuad(pv, TRUE);
                    pf = fmtstr;
                                        *pf++ = '0';
                                        *pf++ = 'x';
                    *pf++ = '%';
                    *pf++ = '.';
                    if (nfmtcnt != 0) {
                        pf += sprintf(pf, "%d", nfmtcnt);
                    }
                    if (prefix == 'I') {
                        _tcsncpy (pf, &stPrefixQuad[1], stPrefixQuad[0]);
                        pf += stPrefixQuad[0];
                    }
                    else if (prefix != 0) {
                        *pf++ = prefix;
                    }
                    *pf++ = fmtchar;
                    *pf = 0;
                    cnt = sprintf (tempbuf, fmtstr, EVAL_UQUAD (pv));

                }
                break;
        }
        _tcsncpy (*buf, tempbuf, cnt + 1);
        *buf += cnt;
        *buflen -= cnt;
    }

    return(FMT_ok);
}


/*      Format - format data
 *
 */


EESTATUS
Format (
    peval_t pv,
    uint radix,
    char * *buf,
    int   *plen,
    bool_t fPtrAndString
    )
{
    char        tempbuf[FMTSTRMAX];
    char *pTempBuf = tempbuf;
    uint        cnt;
    ulong       isfloat = FALSE;
    HSYM        hProc = 0;
    // M00FLAT32
    SYMPTR      pProc;
    char *pc = NULL;
    ulong       cbTempBuf;
    ulong  *pcbTempBuf = &cbTempBuf;
    ulong       iRadix;
    ADDR        addr;
    CV_typ_t    type;
    EEHSTR      hStr = 0;
    uint        fHexUpper = 0;

    if (*plen < 5 ) {
        return (EENOERROR);
    }

    fHexUpper = !!fHexUpper; // canonicalize

    if (EVAL_IS_BITF (pv)) {
        // for a bitfield, change the type to the underlying type
        SetNodeType (pv, BITF_UTYPE (pv));
    }
    if (EVAL_IS_CLASS (pv)) {
        FormatClass (pv, radix, buf, plen);
        return (EENOERROR);

    }
    else if (EVAL_IS_ENUM (pv)) {
        if (EVAL_STATE(pv) == EV_constant) {
            SetNodeType(pv, ENUM_UTYPE(pv));
        }
        else {
            return (FormatEnum(pv, *buf, *plen));
        }
    }
    if (CV_IS_PRIMITIVE (EVAL_TYP (pv)) && !EVAL_IS_PTR (pv)) {
        if (EVAL_TYP (pv) == T_VOID) {
            _tcscpy (tempbuf, "<void>");
        }
        else {
            // establish format string index
            switch (radix) {
                case 8:
                    iRadix = 0;
                    break;

                case 10:
                    iRadix = 1;
                    break;

                default:
                    DASSERT (FALSE);
                    // note fall through
                case 16:
                    if (fHexUpper) {
                        iRadix = 3;
                    }
                    else {
                        iRadix = 2;
                    }
                    break;
            }

            switch (EVAL_TYP (pv)) {
                case T_CHAR:
                case T_RCHAR:
                case T_INT1:
                    if (fPtrAndString) {
                        if (EVAL_CHAR (pv) != 0) {
                            sprintf (tempbuf, fmt_char_nz[iRadix],
                              (radix==10) ? EVAL_CHAR (pv) : EVAL_UCHAR (pv),
                              EVAL_CHAR (pv));
                        }
                        else {
                            // don't stick a 0 in the string
                            sprintf (tempbuf, fmt_char_zr[iRadix],
                              EVAL_CHAR (pv), EVAL_CHAR (pv));
                        }
                    }
                    else {
                        sprintf (tempbuf, fmt_char[iRadix],
                              (radix==10) ? EVAL_CHAR (pv) : EVAL_UCHAR (pv));
                    }
                    break;

                case T_UCHAR:
                case T_UINT1:
                    if (fPtrAndString) {
                        if (EVAL_UCHAR (pv) != 0) {
                            sprintf (tempbuf, fmt_char_nz[iRadix],
                              EVAL_UCHAR (pv), EVAL_UCHAR (pv));
                        }
                        else {
                            // don't stick a 0 in the string
                            sprintf (tempbuf, fmt_char_zr[iRadix],
                              EVAL_UCHAR (pv), EVAL_UCHAR (pv));
                        }
                    }
                    else {
                        sprintf (tempbuf, fmt_char[iRadix], EVAL_UCHAR (pv));
                    }
                    break;

                case T_SHORT:
                case T_INT2:
                    sprintf (tempbuf, fmt_short[iRadix], EVAL_SHORT (pv));
                    break;

                case T_SEGMENT:
                case T_USHORT:
                case T_UINT2:
                    sprintf (tempbuf, fmt_ushort[iRadix], EVAL_USHORT (pv));
                    break;

                case T_LONG:
                case T_INT4:
                    sprintf (tempbuf, fmt_long[iRadix], EVAL_LONG (pv));
                    break;

                case T_ULONG:
                case T_UINT4:
                    sprintf (tempbuf, fmt_ulong[iRadix], EVAL_ULONG (pv));
                    break;

                case T_QUAD:
                case T_INT8:
                    sprintf (tempbuf, fmt_quad[iRadix], EVAL_QUAD (pv));
                    break;

                case T_UQUAD:
                case T_UINT8:
                    sprintf (tempbuf, fmt_uquad[iRadix], EVAL_UQUAD (pv));
                    break;

                case T_REAL32:
                    sprintf_english (tempbuf, "%#g", EVAL_FLOAT (pv));
                    isfloat = TRUE;
                    break;

                case T_REAL64:
                    sprintf_english (tempbuf, "%#.14g", EVAL_DOUBLE (pv));
                    isfloat = TRUE;
                    break;

                case T_REAL80:
#if defined(WIN32) && !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_IA64_)
#pragma message("WARNING: Only one format for 10-byte reals.")
                    SzFromLd ( tempbuf, sizeof(tempbuf), EVAL_LDOUBLE ( pv ) );
#else
                    sprintf_english (tempbuf, "%#.20Lg", EVAL_LDOUBLE (pv));
#endif
                    isfloat = TRUE;
                    break;

                case T_NOTYPE:
                default:
                    if ( ADDR_IS_OFF32(pv->addr) ) {
                        sprintf (tempbuf, fmt_ulong[iRadix], EVAL_ULONG (pv) );
                    }
                    else {
                        sprintf (tempbuf, fmt_ushort[iRadix], EVAL_USHORT (pv));
                    }
                    break;
            }
        }
    }
    else if (EVAL_IS_ADDR (pv)) {
        addr = EVAL_PTR (pv);
        if (EVAL_IS_BASED (pv)) {
#if defined(TARGMAC68K)
            cbTempBuf = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
                            (ulong)EVAL_PTR_OFF (pv));
#else
            if (pExState->state.f32bit) {
                // a based pointer in a 32bit context is
                // treated as a 32bit based pointer -- dolphin #979
                cbTempBuf = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
                            (ulong)EVAL_PTR_OFF (pv));
            }
            else
                cbTempBuf = sprintf (tempbuf, fmt_ptr_0_16[fHexUpper],
                            (ushort)EVAL_PTR_OFF (pv));
#endif
        }
#if defined(TARGMAC68K)
        else {
            if (ADDR_IS_LI (addr)) {
                SHFixupAddr (&addr);
            }
            if(GetAddrSeg(addr) != 0) {
                cbTempBuf = sprintf (tempbuf,
                fmt_ptr_16_32[fHexUpper],
                GetAddrSeg (addr), (CV_uoff32_t)GetAddrOff (addr));
            }
            else {
                cbTempBuf = sprintf (tempbuf,
                fmt_ptr_0_32[fHexUpper],
                (CV_uoff32_t)GetAddrOff (addr));
            }
        }
#else // !TARGMAC68K
        else if (EVAL_IS_PTR (pv) && EVAL_IS_REG (pv)) {
            if (EVAL_IS_NPTR (pv)) {
                cbTempBuf = sprintf (tempbuf, fmt_ptr_16_16[fHexUpper],
                  GetSegmentRegister(pExState->hframe, REG_DS), (CV_uoff16_t)EVAL_PTR_OFF (pv));
            }
            else if (EVAL_IS_NPTR32 (pv)) {
                cbTempBuf = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
                  (CV_uoff32_t)EVAL_PTR_OFF (pv));
            }
            else {
                if (ADDR_IS_LI (addr)) {
                    SHFixupAddr (&addr);
                }
                if (EVAL_IS_FPTR32 (pv)) {
                    cbTempBuf = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
                      (CV_uoff32_t)GetAddrOff (addr));
                }
                else {
                    cbTempBuf = sprintf (tempbuf, fmt_ptr_16_16[fHexUpper],
                      GetAddrSeg (addr), (CV_uoff16_t)GetAddrOff (addr));
                }
            }
        }
        else if (EVAL_IS_PTR (pv) && EVAL_IS_NPTR (pv)) {
            // if it is a near ptr we will treat is as a far ptr
            // since we always carry around the seg & offset
            // even if it is near.
            // DASSERT( EVAL_PTR_SEG (pv) != 0);
            if (ADDR_IS_LI (addr)) {
                SHFixupAddr (&addr);
            }
            cbTempBuf = sprintf (tempbuf, fmt_ptr_16_16[fHexUpper],
              GetAddrSeg (addr), (CV_uoff16_t)GetAddrOff (addr));
        }
        else if (EVAL_IS_PTR (pv) && EVAL_IS_NPTR32 (pv)) {
            // if it is a near ptr we will treat is as a far ptr
            // since we always carry around the seg & offset
            // even if it is near.
            // DASSERT( EVAL_PTR_SEG (pv) != 0);
            if (ADDR_IS_LI (addr)) {
                SHFixupAddr (&addr);
            }
            cbTempBuf = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
              (CV_uoff32_t)GetAddrOff (addr));
        }
        else {
            if (ADDR_IS_LI (addr)) {
                SHFixupAddr (&addr);
            }
            if (EVAL_IS_FPTR32 (pv)) {
                cbTempBuf = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
                  (CV_uoff32_t)GetAddrOff (addr));
            } else if (EVAL_IS_PTR64 (pv)) {
                cbTempBuf = sprintf (tempbuf, fmt_ptr_64[fHexUpper],
                  /*v-vadimp (CV_uoff64_t)*/GetAddrOff (addr));
            } 
            else {
                if ( ADDR_IS_FLAT ( addr ) ) {

                    DASSERT ( ADDR_IS_OFF32 ( addr) );
                    cbTempBuf = sprintf (
                        tempbuf,
                        (TargetMachine == mptia64)? fmt_ptr_64[fHexUpper] : fmt_ptr_0_32[fHexUpper],
                        GetAddrOff ( addr )
                    );
                }
                else {

                    if ( ADDR_IS_OFF32 ( addr ) ) {
                        cbTempBuf = sprintf (
                            tempbuf,
                            fmt_ptr_16_32[fHexUpper],
                            GetAddrSeg (addr),
                            (CV_uoff32_t) GetAddrOff (addr)
                        );
                    }
                    else {
                        cbTempBuf = sprintf (
                            tempbuf,
                            fmt_ptr_16_16[fHexUpper],
                            GetAddrSeg (addr),
                            (CV_uoff16_t) GetAddrOff (addr)
                        );
                    }
                }
            }
        }
#endif // TARGMAC68K
        if (!EVAL_IS_DPTR (pv)) {
            CXT cxt;
            eval_t nv = *pv;

            memset(&cxt, 0, sizeof(CXT));

            addr = EVAL_PTR (pv);
            if (!ADDR_IS_LI (addr)) {
                SHUnFixupAddr (&addr);
            }
            SHSetCxtMod ( &addr, &cxt );
            nv.CXTT = cxt;
            if (SHGetNearestHsym (&addr, SHHMODFrompCXT (&cxt), EECODE, &hProc) == 0) {
                // the address exactly matches a symbol
                switch ((pProc = (SYMPTR)MHOmfLock ((HSYM)hProc))->rectyp) {
                    case S_LPROC16:
                    case S_GPROC16:
                        EVAL_MOD(&nv) = SHHMODFrompCXT (&cxt);
                        SetNodeType(&nv, (CV_typ_t)((PROCPTR16)pProc)->typind);
                        pc = FormatVirtual ((char *) ((PROCPTR16)pProc)->name, &nv, &hStr);
                        break;

                    case S_THUNK16:
                        pc = (char *) ((THUNKPTR16)pProc)->name;
                        break;

                    case S_LPROC32:
                    case S_GPROC32:
                        EVAL_MOD(&nv) = SHHMODFrompCXT (&cxt);
                        SetNodeType(&nv, (CV_typ_t)((PROCPTR32)pProc)->typind);
                        pc = FormatVirtual ((char *) ((PROCPTR32)pProc)->name, &nv, &hStr);
                        break;

                    case S_LPROCMIPS:
                    case S_GPROCMIPS:
                        EVAL_MOD(&nv) = SHHMODFrompCXT (&cxt);
                        SetNodeType(&nv, (CV_typ_t)((PROCPTRMIPS)pProc)->typind);
                        pc = FormatVirtual ((char *) ((PROCPTRMIPS)pProc)->name, &nv, &hStr);
                        break;

                    case S_LPROCIA64:
                    case S_GPROCIA64:
                        EVAL_MOD(&nv) = SHHMODFrompCXT (&cxt);
                        SetNodeType(&nv, (CV_typ_t)((PROCPTRIA64)pProc)->typind);
                        pc = FormatVirtual ((char *) ((PROCPTRIA64)pProc)->name, &nv, &hStr);
                        break;

                    case S_THUNK32:
                        pc = (char *) ((THUNKPTR32)pProc)->name;
                        break;

                    case S_PUB32:
                        {
                            char undname[256];
                            pc = FormatUndecorate( (char*) ((DATASYM32*)pProc)->name, undname );
                        }
                        break;
                }
                MHOmfUnLock ((HSYM)hProc);
            }
        }

        // M00KLUDGE - display strings of chars
        if ((fPtrAndString) && (EVAL_IS_PTR (pv))) {
            type = EVAL_TYP (pv);
            if (EVAL_IS_BASED (pv) || !CV_IS_PRIMITIVE(type)) {
                type = PTR_UTYPE (pv);
            }
            else {
                type &= (CV_TMASK | CV_SMASK);
            }
            if ((type == T_CHAR) || (type == T_UCHAR) || (type == T_RCHAR) || (type == T_WCHAR)
                || (GetUnicodeStrings () && (type == T_USHORT))
                ) {

                CV_typ_t PtrToCharType;
                BOOL fUnicode = FALSE;
                if ((type == T_USHORT) || (type == T_WCHAR))
                {
                    PtrToCharType = pExState->state.f32bit? ((TargetMachine == mptia64)? T_64PUSHORT : T_32PUSHORT) : T_PFUSHORT;
                    fUnicode = TRUE;
                }
                else
                {
                    PtrToCharType = pExState->state.f32bit ? ((TargetMachine == mptia64)? T_64PCHAR  : T_32PCHAR) : T_PFCHAR;
                }

                tempbuf[cbTempBuf] = ' ';

                // Need to set Evaluating to 1 to force Normalization
                // of based ptrs in CastNode (and reset to 0 immediately
                // afterwards)
                Evaluating = TRUE;

                // CUDA #4044 : make sure we can do the cast... [rm]
                if (!CastNode (pv, PtrToCharType, PtrToCharType)) {
                    Evaluating = FALSE;
                    return (EEGENERAL);
                }
                Evaluating = FALSE;
                pTempBuf += cbTempBuf + 1;
                *pcbTempBuf = FMTSTRMAX - cbTempBuf - 1;
                EvalString (pv, &pTempBuf, (int   *) pcbTempBuf, fUnicode);
            }
        }
    }
    else {
        pExState->err_num = ERR_UNKNOWNTYPE;
        return (EEGENERAL);
    }
    cnt = _tcslen (tempbuf);
    cnt = min ((uint)*plen, cnt);
    _tcsncpy (*buf, tempbuf, cnt);
    *plen -= cnt;
    *buf += cnt;
    if (pc != NULL) {
        cnt = min ((uint)*plen, ((uint)*pc) + 1);
        **buf = ' ';
        (*buf)++;
        _tcsncpy (*buf, pc + 1, cnt - 1);
        *plen -= cnt;
        *buf += cnt;
    }
    if (hStr != 0) {
        MemUnLock (hStr);
        MHMemFree (hStr);
    }
    return (EENOERROR);
}



static char *szClassDisp = "{...}";

void FormatClass (peval_t pv, uint radix, char * *buf,
  int   *buflen)
{
    int     len;

    Unreferenced( radix );
    Unreferenced( pv );

    if (pExState->hAutoExpandRule) {
        FormatAutoExpand (pv, radix, buf, buflen);
    }
    else {
        len = _tcslen (szClassDisp);
        len = min (*buflen, len);
        _tcsncpy (*buf, szClassDisp, len);
        *buflen -= len;
        *buf += len;
    }
}


#ifdef USE_CUSTVIEW

BOOL WINAPI StaticHelperReadMem ( DEBUGHELPER *pThis, DWORD dwAddr, DWORD nWant, VOID* pWhere, DWORD *nGot )
{
    ADDR addr;
    UINT count;
    
    if (pThis!=&StaticHelper)
    {
        // check correct 'this' usage
        DASSERT( !"Bad this pointer to HelperReadMem" );
        return E_INVALIDARG;
    }
    
    AddrInit( &addr, 0, 0, dwAddr, TRUE, TRUE, FALSE, FALSE );
    
    count = GetDebuggeeBytes (addr, nWant, pWhere, T_RCHAR);
    
    *nGot = count;
    
    if (count==0) {
        return E_FAIL;
    } else if (count!=nWant) {
        return S_FALSE;
    } else {
        return S_OK;
}

DEBUGHELPER StaticHelper =
{
    0x10000,
    StaticHelperReadMem
};

#endif

/*
 *  FormatAutoExpand
 *
 *  Format the auto-expanding part of a pointer or class
 */

void
FormatAutoExpand (
    peval_t pv,
    uint radix,
    char * *buf,
    int  *buflen
    )
{
    uint    cHTM = 0;
    int     len;
    LSZ     lsz;
    char   *pcLDelim;
    char   *pcRDelim;
#ifdef USE_CUSTVIEW
        CUSTOMVIEWER pViewer = NULL;
#endif

    if (pExState->hAutoExpandRule) {

        if (EVAL_IS_PTR (pv) && *buflen > 1) {
            // we're right after the ptr value string
            // so leave an extra space
            * ((*buf)++) = ' ';
            **buf = '\0';
            *buflen -= 1;
        }

        if (*buflen > 1) {
            * ((*buf)++) = '{';
            **buf = '\0';
            *buflen -= 1;
        }

        lsz = (LSZ) MemLock (pExState->hAutoExpandRule);


#ifdef USE_CUSTVIEW
                if (_tcsncmp(lsz,"$$ADDON(",8)==0)
                {
                        CUSTOMVIEWER pViewer = NULL;
                        HRESULT hr;
                        char arg[256];
                        char *p = _tcschr( lsz+8, ')' );

                        if (p)
                        {
                                memcpy( arg, lsz+8, p-(lsz+8) );
                                arg[ p-(lsz+8) ] = 0;
                                // one day look up these names in the registry, but
                                // hard-coded for now
                                if (_tcscmp( arg, "GUID" )==0)
                                        pViewer = ExportViewGuid;
                                else if (_tcscmp( arg, "HWND" )==0)
                                        pViewer = ExportViewHwnd;
                                else if (_tcscmp( arg, "VARIANT" )==0)
                                        pViewer = ExportViewVariant;

                                if (pViewer)
                                {
                                        // call the external DLL to expand the item
                                        ADDR addr;

                                        GetRawAddr( pv, &addr );
                                        **buf = 0;

                                        hr = pViewer( GetAddrOff( addr ), &StaticHelper, radix, GetUnicodeStrings(), *buf, *buflen, 0 );
                                        if (hr==S_OK)
                                        {
                                                short len = (short)_tcslen(*buf);
                                                *buflen -= len;
                                                *buf += len;
                                        }
                                }
                        }
                        if ( (pViewer==NULL) || (hr!=S_OK) )
                                FormatAEError( pv, radix, buf, buflen );
                        // skip to end of AE string
                        lsz += _tcslen( lsz );
                }
        else
#endif

         while ( (pcLDelim = _tcschr (lsz, _T ('<'))) != NULL &&
                (pcRDelim = _tcschr (pcLDelim, _T('>'))) != NULL ) {

            // If the '>' we found is actually a '->' skip over this
            // and look for the next one.
            while  (pcRDelim && *(pcRDelim - 1) == '-' )
            {
                pcRDelim = _tcschr(pcRDelim + 1, _T('>'));
            }

            if (pcRDelim == NULL)
            {
                break;
            }

            // copy string that precedes '<'
            len = (int) min (*buflen, pcLDelim - lsz);
            _tcsncpy (*buf, lsz, len);
            *buflen -= len;
            *buf += len;

            lsz = pcLDelim + 1;
            *pcRDelim = 0;

            switch (AEParseRule(lsz)) {
            case AE_downcasttype:
                // print downcast class name (if any)
                FormatAEDownCastType (pv, radix, buf, buflen);
                break;
            case AE_childTM:
                // display value of auto-expand child TM
                FormatAEChildTM (cHTM++, pv, radix, buf, buflen);
                break;
            default:
                FormatAEError (pv, radix, buf, buflen);
            }

            *pcRDelim = _T('>');

            lsz = pcRDelim + 1;
        }
        MemUnLock (pExState->hAutoExpandRule);

        // copy remainder of format string
        len = _tcslen (lsz);
        len = min (*buflen, len);
        _tcsncpy (*buf, lsz, len);
        *buflen -= len;
        *buf += len;

        if (*buflen > 1) {
            * ((*buf)++) = '}';
            **buf = '\0';
            *buflen -= 1;
        }
    }
}

/*
 *  FormatAEChildTM
 *
 *  Format an auto-expanding child TM
 */

void FormatAEChildTM (uint cHTM, peval_t pv, uint radix, char * *buf,
  int *buflen)
{
    pexstate_t      pExStateSav = pExState;
    EEHSTR          hszValue;
    LSZ             szValue;
    PTML            pTML;
    HTM *       rgHTM;
    int             len;
    bool_t          fRebind = pExState->state.fAErebind;
    bool_t          fReeval = pExState->state.fAEreeval;
    bool_t          fFormatOK = FALSE;
    FMT_state       fmtstate;
    EESTATUS        error;

    pTML = &pExState->TMLAutoExpand;

    // Find the appropriate TM in the TMLAutoExpand list
    // and display its value.
    if (cHTM < pTML->cTMListAct) {
        DASSERT (pTML->hTMList);
        rgHTM = (HTM *)MemLock (pTML->hTMList);
        if (rgHTM && rgHTM [cHTM] != NULL) {
            SaveFmtState (&fmtstate);
            error = FormatNode (&rgHTM[cHTM], radix, NULL, &hszValue, FALSE);
            RestoreFmtState  (&fmtstate);
            if (EENOERROR == error) {
                fFormatOK = TRUE;
                szValue = (LSZ) MemLock (hszValue);
                len = _tcslen (szValue);
                len = min (*buflen, len);
                _tcsncpy (*buf, szValue, len);
                MemUnLock (hszValue);
                MemFree (hszValue);
                *buflen -= len;
                *buf += len;
            }
        }
        MemUnLock (pTML->hTMList);
    }
    pExState = pExStateSav;
    if (!fFormatOK) {
        FormatAEError (pv, radix, buf, buflen);
    }
}



/*
 *  FormatAEError
 *
 *  Display Error message while auto-expanding an expression
 */

static char *szErrDisp = "???";

void FormatAEError (peval_t pv, uint radix, char * *buf,
  int *buflen)
{
    int     len;

    Unreferenced( radix );
    Unreferenced( pv );

    len = _tcslen (szErrDisp);
    len = min (*buflen, len);
    _tcsncpy (*buf, szErrDisp, len);
    *buflen -= len;
    *buf += len;
}


/*
 *  FormatAEDownCastType
 *
 *  Format derived-most class type name
 */

void
FormatAEDownCastType (
    peval_t pv,
    uint radix,
    char * *buf,
    int *buflen
    )
{
    LSZ     lsz;
    char   *pch;
    int    len;

    if (pExState->hDClassName)
        lsz = (LSZ) MemLock (pExState->hDClassName);
    else {
        CV_typ_t    type = EVAL_TYP (pv);
        if (EVAL_IS_PTR (pv)) {
            type = PTR_UTYPE (pv);
        }
        lsz = GetTypeName (type, EVAL_MOD (pv));
    }

    DASSERT (lsz);
    // hDClassName may contain a context operator
    // We skip this operator in order to display the class name only
    if ((pch = _tcschr (lsz, '}')) != NULL )
        lsz = pch + 1;
    len = _tcslen (lsz);
    len = min (*buflen, len);
    _tcsncpy (*buf, lsz, len);
    *buflen -= len;
    *buf += len;

    if (pExState->hDClassName)
        MemUnLock (pExState->hDClassName);
}


EESTATUS
FormatEnum (
    peval_t pv,
    char *buf,
    int buflen
    )
{
    HTYPE   hType, hFieldType;
    plfEnum pEnumType;
    plfFieldList pFieldType;
    char * pEnumerate;
    uint    i, skip;
    EESTATUS retval = EENOERROR;

    if ((hType = THGetTypeFromIndex (EVAL_MOD (pv), EVAL_TYP (pv))) == 0) {
        goto NoDisplay;
    }
    pEnumType = (plfEnum)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
    DASSERT(pEnumType->leaf == LF_ENUM);

    if ((hFieldType = THGetTypeFromIndex (EVAL_MOD (pv), pEnumType->field)) == 0) {
        goto NoDisplay;
    }
    pFieldType = (plfFieldList)(&((TYPPTR)(MHOmfLock (hFieldType)))->leaf);

    for (i = 0, pEnumerate = (char *)pFieldType + offsetof(lfFieldList, data);
        i < pEnumType->count;
        i++, pEnumerate += skip)
    {
        DASSERT(*((unsigned short *)pEnumerate) == LF_ENUMERATE);
        skip = offsetof (lfEnumerate, value);
        if (EVAL_UQUAD(pv) == RNumLeaf ((char *)(&(((plfEnumerate)pEnumerate)->value)), &skip)) {
            _tcsncpy(buf, pEnumerate + skip + 1, min(buflen, *(pEnumerate + skip)));
            goto Exit;
        }
        skip += *(pEnumerate + skip) + 1;   //skip name field
        skip += SkipPad((uchar *)pEnumerate + skip);
    }

NoDisplay:
    retval = EEGENERAL;
    pExState->err_num = ERR_INVENUMVAL;

Exit:
    MHOmfUnLock (hType);
    MHOmfUnLock (hFieldType);
    return(retval);
}



BOOL GetRawAddr( peval_t pv, ADDR *paddr )
{
    if (EVAL_IS_ADDR(pv))
    {
        BOOL bCast;
        Evaluating = TRUE;
        bCast = CastNode (pv, T_32PCHAR, T_32PCHAR);
        Evaluating = FALSE;

        if (!bCast) {
            return FALSE;
        }

        *paddr = EVAL_PTR(pv);
    }
    else if (EVAL_IS_PTR(pv)) {
        *paddr = EVAL_PTR(pv);
    }
    else
    {
        ResolveAddr(pv);
        *paddr = EVAL_SYM(pv);
    }

    if (ADDR_IS_LI (*paddr)) {
        SHFixupAddr (paddr);
    }

    if (EVAL_IS_PTR (pv) && (EVAL_IS_NPTR (pv) || EVAL_IS_NPTR32 (pv))) {
        paddr->addr.seg =  GetSegmentRegister(pExState->hframe, REG_DS);
    }

    return TRUE;
}



/*
 *  EvalString
 *
 *  Evaluate an expression whose format string contains an 's'.
 */

void EvalString (peval_t pv, char *FAR *buf,
  int *buflen, bool_t fUnicode)
{
    ADDR    addr;
    int     count;

    if(*buflen < 3) return;
    **buf = '\"';
    (*buf)++;
    (*buflen)--;
    addr = EVAL_PTR (pv);
    if (ADDR_IS_LI (addr)) {
        SHFixupAddr (&addr);
    }
    if (EVAL_IS_PTR (pv) && (EVAL_IS_NPTR (pv) || EVAL_IS_NPTR32 (pv))) {
        addr.addr.seg =  GetSegmentRegister(pExState->hframe, REG_DS);
    }
    count = GetDebuggeeBytes (addr, *buflen - 2, *buf, T_RCHAR);

    if (fUnicode) {
        // Convert unicode characters to MBCS, so that
        // we can display them using the existing non-Unicode
        // formatting routines
        int             cb;
        wchar_t UNALIGNED *pwch = (wchar_t UNALIGNED *)*buf;

        while (count >= sizeof (wchar_t) && *pwch != 0 ) {
            cb = ConvertWCToMB (*buf, *pwch);
            (*buflen) -= cb;
            *buf += cb;
            pwch ++;
            count -= sizeof (wchar_t);
        }
    }
    else {
        while ((**buf != 0) && (count > 0)) {
            int clen = _tclen (*buf);
            (*buflen) -= clen;
            *buf = _tcsinc (*buf);
            count -= clen;
        }
    }

    **buf = '\"';
    (*buf)++;
    (*buflen)--;
    **buf = 0;
    (*buf)++;
    (*buflen)--;
}
/*
 * Use IsPrint instead of the runtime function isprint,
 * since the latter is not exported to the EE
 */
INLINE bool_t IsPrint (int ch)
{
    return (ch >= 0x20) && (ch <= 0x7e) ? TRUE : FALSE;
}

/*
 *  EvalMem
 *
 *  Evaluate an expression whose format string contains an 'm'.
 *  The format is similar to the memory window of codeview :
 *
 *  ,m or ,mb (memory -byte display)
 *  SSSS:OOOO xx xx xx xx ... xx aaaa...a
 *
 *  ,mw       (memory -word display)
 *  SSSS:OOOO xxxx ... xxxx
 *
 *  ,md       (memory -dword display)
 *  SSSS:OOOO xxxxxxxx ... xxxxxxxx
 *
 *  ,mq       (memory -qword display)
 *  SSSS:OOOO xxxxxxxxxxxxxxxx ... xxxxxxxxxxxxxxxx
 *
 *  ,ma       (memory -ascii display)
 *  SSSS:OOOO aaa...a
 *
 *  where SSSS:OOOO is the address, xx is dump info in hex,
 *  and a is an ascii char.  (If the address is a 32-bit
 *  address, we will emit "OOOOOOOO" instead of "SSSS:OOOO".)
 *
 */
#define MCOUNT    16  /* number of bytes to read */
#define MCOUNTA   64  /* number of bytes to read for ,ma display */
#define MINLBUFSZ 64  /* minimum local buf size for byte transfer */
#define MINBUFSZ  256 /* minimum buf size for display */

void EvalMem (peval_t pv, char *FAR *buf, int   *buflen, char postfix)
{
    ADDR        addr;
    int         count;
    char        tempbuf[MINBUFSZ];
    uchar       localbuf[MINLBUFSZ];
    int         cnt = 0;
    int         nItems;
    int         nBytes;
    int         i;
    char       *pFmt;
    uint        fHexUpper = 0;
    bool_t      fDisplayString = FALSE;

    fHexUpper = !!fHexUpper; // canonicalize

    addr = EVAL_PTR (pv);
    if (ADDR_IS_LI (addr)) {
        SHFixupAddr (&addr);
    }
    if (EVAL_IS_PTR (pv) && (EVAL_IS_NPTR (pv) || EVAL_IS_NPTR32 (pv))) {
        addr.addr.seg =  GetSegmentRegister(pExState->hframe, REG_DS);
    }

    if (ADDR_IS_FLAT(addr)) {
        DASSERT ( ADDR_IS_OFF32 ( addr ) );
        cnt = sprintf(tempbuf, fmt_ptr_0_32[fHexUpper],
          (CV_uoff32_t)GetAddrOff (addr));
    }
    else {
        if ( ADDR_IS_OFF32 ( addr ) ) {
            cnt = sprintf(tempbuf, fmt_ptr_16_32[fHexUpper],
              GetAddrSeg (addr), (CV_uoff32_t)GetAddrOff (addr));
        }
        else {
            cnt = sprintf(tempbuf, fmt_ptr_16_16[fHexUpper],
              GetAddrSeg (addr), (CV_uoff16_t)GetAddrOff (addr));
        }
    }
    tempbuf[cnt++] = ' ';
    tempbuf[cnt++] = ' ';

    nBytes = (postfix == 'a' ? MCOUNTA : MCOUNT);

    count = GetDebuggeeBytes (addr, nBytes, (char *)localbuf, T_RCHAR);

    nItems = nBytes; //assume ,m ,mb or ,ma display

    switch (postfix) {
    case 'b':
        if (fHexUpper) {
            pFmt = "%02X ";
        }
        else {
            pFmt = "%02x ";
        }
        for (i=0; i<nItems; i++) {
            cnt += i<count
                ? sprintf (tempbuf+cnt, pFmt, (unsigned int)localbuf[i])
                : sprintf (tempbuf+cnt, "?? ");
        }

        tempbuf[cnt++] = ' ';
        // fall through

    case 'a':
        fDisplayString = TRUE;
        break;

    case 'w':
    case 'u':
        nItems = nBytes >> 1;
        count >>= 1;
        if (fHexUpper) {
            pFmt = "%04hX ";
        }
        else {
            pFmt = "%04hx ";
        }
        for (i=0; i<nItems; i++) {
            cnt += i<count
                ? sprintf (tempbuf+cnt, pFmt, *((unsigned short UNALIGNED *)localbuf+i))
                : sprintf (tempbuf+cnt, "???? ");
        }
        if ('u' == postfix) {
            char   *pch;
            int     i;

            // Low budget unicode display: convert unicode
            // string to MBCS and use existing display code
            // In-place conversion should be OK, since the
            // translated character is at most two bytes long
            tempbuf[cnt++] = ' ';
            fDisplayString = TRUE;
            pch = (char *) localbuf;
            for (i=0; i<nItems; i++) {
                pch += ConvertWCToMB (pch, ((wchar_t *)localbuf)[i]);
            }
        }
        break;

    case 'd':
        nItems = nBytes >> 2;
        count >>= 2;
        if (fHexUpper) {
            pFmt = "%08lX ";
        }
        else {
            pFmt = "%08lx ";
        }
        for (i=0; i<nItems; i++) {
            cnt += i<count
                ? sprintf (tempbuf+cnt, pFmt, *((unsigned long UNALIGNED *)localbuf+i))
                : sprintf (tempbuf+cnt, "???????? ");
        }
        break;

    case 'q':
        nItems = nBytes >> 3;
        count >>= 3;
        if (fHexUpper) {
            pFmt = "%016I64X ";
        }
        else {
            pFmt = "%016I64x ";
        }
        for (i=0; i<nItems; i++) {
            cnt += i<count
                ? sprintf (tempbuf+cnt, pFmt, *((unsigned __int64 UNALIGNED *)localbuf+i))
                : sprintf (tempbuf+cnt, "???????? ");
        }
        break;

    }

    if (fDisplayString) {
#ifndef _SBCS
        for (i=0; i<nItems; i++) {
            if( i<count ) {
                if ( _ismbblead( localbuf[i] ) ) {
                    if ( i+1<count && _ismbbtrail( localbuf[ i+1 ] ) ) {
                        tempbuf[cnt++] = localbuf[ i++ ];
                        tempbuf[cnt++] = localbuf[ i ];
                    }
                    else {
                        tempbuf[cnt++] = '.';
                        i++;
                    }
                }
                else if ( _ismbbkana( localbuf[ i ] ) ||
                    IsPrint( localbuf[ i ] ) ) {

                    tempbuf[cnt++] = localbuf[ i ];
                }
                else {
                    tempbuf[cnt++] = '.';
                }
            }
            else {
                tempbuf[cnt++] = '?';
            }
        }
#else
        for (i=0; i<nItems; i++) {
            if ( i<count ) {
                if ( IsPrint(localbuf[i]) ) {
                    tempbuf[cnt++] = localbuf[i];
                }
                else {
                    tempbuf[cnt++] = '.';
                }
            }
            else {
                tempbuf[cnt++] = '?';
            }
        }
#endif
    }

    // If the size of the buffer is less than the display string,
    // clip the formatted memory string but copy what we can
    if ( *buflen - 1 < cnt ) {
        cnt = *buflen - 1;
    }

    tempbuf[cnt] = '\0';
    _tcsncpy (*buf, tempbuf, cnt + 1);
    *buf += cnt;
    *buflen -= cnt;
}


/*
 * ConvertWCToMB - convert wide char to multi-byte
 */

int ConvertWCToMB (char *pchmb, wchar_t wch)
{
    int     cb;
    if ((cb = wctomb (pchmb, wch)) < 0) {
        // no conversion was possible,
        // use a default character
        *pchmb = '?';
        cb = 1;
    }
    else if (cb == 0) {
        // converting L'\0'
        *pchmb = '\0';
        cb = 1;
    }
    return cb;
}

// undecorate a symbol, currently only does @ILT symbols (for vtable entries)
// turns @ILT+nnn(?mangledname) into unmangled name
// and simply mangled ones

char *
FormatUndecorate (
    char *pc,
    char *newstr
    )
{
    size_t oLen;
    char *pOpen;
    char *pClose;
    char undname[256];
    char *pResult;

    newstr++;                                               // leave room for length byte

    // make null-terminated version in user buffer
    oLen = *(uchar*)pc;
    memcpy( newstr, pc+1, oLen );
    newstr[oLen] = 0;

    if (pc[1]=='?')
    {
        // simple mangled name starts with "?"
        pResult = __unDName( undname, newstr, sizeof(undname), malloc, free, UNDNAME_NAME_ONLY );
    }
    else if ( (pc[0]>5) && (memcmp(pc+1, "@ILT", 4)==0))
    {
        // "@ILT+nnn(?mangled)"
        pOpen = _tcschr(newstr, '(' );
        if (!pOpen) {
            return pc;
        }
        pClose = _tcschr(pOpen+1, ')' );
        if (!pClose) {
            return pc;
        }
        *pClose = 0;

        pResult = __unDName( undname, pOpen+1, sizeof(undname), malloc, free, UNDNAME_NAME_ONLY );
    }
    else {
        return pc;
    }

    if (!pResult) {
        return pc;                                                                      // if failed return original
    }

    // undecorated OK, so convert
    _tcscpy( newstr, undname );

    newstr--;
    newstr[0] = (uchar)_tcslen(newstr+1);                   // set length field
    return newstr;
}

char *
FormatVirtual (
    char *pc,
    peval_t pv,
    PEEHSTR phStr
    )
{
    char    save;
    char   *pEnd;
    char   *bufsave;
    char   *buf;
    uint    buflen;
    PHDR_TYPE   pHdr;
    char   *pName;

    if ((*phStr = MemAllocate (TYPESTRMAX + sizeof (HDR_TYPE))) == 0) {
        // unable to allocate memory for type string.  at least print name
        return (pc);
    }
    bufsave = (char *)MemLock (*phStr);
    memset (bufsave, 0, TYPESTRMAX + sizeof (HDR_TYPE));
    buflen = TYPESTRMAX - 1;
    pHdr = (PHDR_TYPE)bufsave;
    buf = bufsave + sizeof (HDR_TYPE);
    pCxt = &pExState->cxt;
    bnCxt = 0;
    pEnd = pc + *pc + 1;
    save = *pEnd;
    *pEnd = 0;
    pName = pc + 1;
    FormatType (pv, &buf, &buflen, &pName, 1L, pHdr);
    *pEnd = save;
    *(bufsave + sizeof (HDR_TYPE) - 1) = TYPESTRMAX - 1 - buflen;
    return (bufsave + sizeof (HDR_TYPE) - 1);
}


/***    DoAutoExpand - Prepare TM for auto expansion
 *
 *      void DoAutoExpand (phTM)
 *
 *      Entry   phTM = pointer to handle of parent TM
 *
 *      Exit    Creates auto-expanded TM list and stores it
 *              in the parent TM.
 *
 *      Returns void
 */


void FASTCALL DoAutoExpand (PHTM phTM)
{
    static  char    bufRule [256];
    static  char    bufDeriv [256];
    eval_t          Eval;
    peval_t         pv = &Eval;
    bool_t          fNewDownCast = FALSE;
    bool_t          fNewRule;
    bool_t          fAErebind;
    bool_t          fAEreeval;

    DASSERT (phTM && *phTM);

    pExState = (pexstate_t) MemLock (*phTM);
    fAErebind = pExState->state.fAErebind;
    fAEreeval = pExState->state.fAEreeval;

    if (fAErebind || fAEreeval) {
        // Auto-expand rule and down-cast info need to be updated.
        // The auto-expand list has not been bound (or evaluated)
        // since the last time this TM was bound (or evaluated)

        pCxt = &pExState->cxt;
        *pv = pExState->result;

        // if we don't have a class or a pointer to a class,
        // no autoexpansion is performed
        if (EVAL_IS_PTR (pv) && !SetNodeType(pv, PTR_UTYPE(pv)) ||
            !EVAL_IS_CLASS (pv)) {
            EEFreeTML (&pExState->TMLAutoExpand);
            MemFree (pExState->hAutoExpandRule);
            pExState->hAutoExpandRule = 0;
            pExState->state.fAErebind = FALSE;
            pExState->state.fAEreeval = FALSE;
            MemUnLock (*phTM);
            return;
        }

        // Get the expansion rule associated with the derived-most class, if any
        if (EVAL_IS_PTR (&pExState->result) &&
            !EVAL_IS_REF (&pExState->result)) {
            *bufDeriv = 0;
            GetDerivClassName (&pExState->result, TRUE, bufDeriv, sizeof(bufDeriv));
            fNewDownCast = UpdateEEHSTR (&pExState->hDClassName, bufDeriv);

            if (*bufDeriv) {
                FMT_state   fmtstate;
                uint        radix = pExState->radix;
                SHFLAG      fCase = pExState->state.fCase;
                PCXT        pBindCxt = &pExState->cxt;
                ulong       end;
                HTM         hTMOut;
                // bufDeriv contains a derived-most class name prepended
                // by a context operator.
                // Parse and Bind in order to get the corresponding
                // evaluation node
                SaveFmtState (&fmtstate);
                if (EENOERROR == Parse (bufDeriv, radix, fCase, FALSE, &hTMOut, &end) &&
                    EENOERROR == DoBind (&hTMOut, pBindCxt, BIND_fSupOvlOps) ) {
                    pExState = (pexstate_t) MemLock (hTMOut);
                    *pv = pExState->result;
                    MemUnLock (hTMOut);
                }
                if (hTMOut)
                    EEFreeTM (&hTMOut);
                RestoreFmtState (&fmtstate);
            }
        }

        // pv should now contain the class for which we need to find
        // an auto expansion rule

        *bufRule = 0;
        FindAERule (pv, bufRule, sizeof(bufRule));

        fNewRule = UpdateEEHSTR (&pExState->hAutoExpandRule, bufRule);

        if (fNewDownCast || fNewRule) {
            // AutoExpand TMList should be rebound
            pExState->state.fAErebind = TRUE;
            pExState->state.fAEreeval = TRUE;
        }
    }

    MemUnLock (*phTM);

    if (fAErebind) {
        // should apply new rule, so we'll have to
        // reconstruct and rebind child TMs
        DoBindAutoExpand (phTM);
    }

    if (fAEreeval) {
        DoEvalAutoExpand (phTM);
    }

    pExState = (pexstate_t) MemLock (*phTM);
    pExState->state.fAErebind = FALSE;
    pExState->state.fAEreeval = FALSE;
    MemUnLock (*phTM);
}

/***    SaveFmtState - Save Format state
 *
 *      Saves global variables used by Format
 */

void FASTCALL SaveFmtState (FMT_state *pfmtstate)
{
    pfmtstate->pExState = pExState;
    pfmtstate->pExStr = pExStr;
    pfmtstate->pCxt = pCxt;
}


/***    RestoreFmtState - Restore Format state
 *
 *      Restores global variables used by Format
 */

void FASTCALL RestoreFmtState (FMT_state *pfmtstate)
{
    pExState = pfmtstate->pExState;
    pExStr = pfmtstate->pExStr;
    pCxt = pfmtstate->pCxt;
}

/***    DoBindAutoExpand - Construct and bind auto-expanded child-TMs
 *
 *      error = DoBindAutoExpand (phTM)
 *
 *      Entry   phTM = ptr to handle of parent TM
 *
 *      Exit    Auto-expand TM list created and bound
 *
 *      Returns void
 */

void FASTCALL DoBindAutoExpand (PHTM phTM)
{
    pexstate_t  pTM;
    LSZ         lsz;
    char *pcLDelim;
    char *pcRDelim;
    HTM         hTMChild;

    pTM = (pexstate_t) MemLock (*phTM);
    EEFreeTML (&pTM->TMLAutoExpand);
    MemUnLock (*phTM);

    if (pTM->hAutoExpandRule) {
        lsz = (LSZ) MemLock (pTM->hAutoExpandRule);
        while ( (pcLDelim = _tcschr (lsz, _T ('<'))) != NULL &&
                (pcRDelim = _tcschr (pcLDelim, _T('>'))) != NULL ) {

            // If the '>' we found is actually a '->' skip over this
            // and look for the next one.
            while  (pcRDelim && *(pcRDelim - 1) == '-' )
            {
                pcRDelim = _tcschr(pcRDelim + 1, _T('>'));
            }

            if (pcRDelim == NULL)
            {
                break;
            }

            hTMChild = 0;
            lsz = pcLDelim + 1;
            *pcRDelim = '\0';
            switch (AEParseRule(lsz)) {
            case AE_childTM:
                GetDerivedTM (*phTM, lsz, &hTMChild);
                pTM = (pexstate_t) MemLock (*phTM);
                // hTMchild may be NULL, but we still need to add it to
                // the list in order to keep in sync with the "<>" pairs
                // in the expansion rule
                TMLAddHTM ( &pTM->TMLAutoExpand, hTMChild );
                MemUnLock (*phTM);
                break;
            case AE_downcasttype:
                // do nothing, we'll just print the class name
                // during formatting
                break;
            }

            *(lsz = pcRDelim) = _T('>');
        }
        MemUnLock (pTM->hAutoExpandRule);
    }
}

/***    DoEvalAutoExpand - Evaluate auto-expanded child TM list
 *
 *      error = DoEvalAutoExpand (phTM)
 *
 *      Entry   phTM = ptr to handle of parent TM
 *
 *      Exit    Auto-expand TM list is evaluated
 *
 *      Returns void
 */

void FASTCALL DoEvalAutoExpand (PHTM phTM)
{
    HTM        *rgHTM;
    pexstate_t  pTM;
    PTML        pTML;
    uint        i;

    // try to evaluate auto-expand child TMs
    pTM = (pexstate_t) MemLock (*phTM);
    pTML = &pTM->TMLAutoExpand;
    rgHTM = (HTM *)MemLock (pTML->hTMList);
    for (i=0; i<pTML->cTMListAct; i++) {
        if (rgHTM[i])
            DoEval (&rgHTM[i], pTM->hframe, (EEDSP) pTM->style);
    }
    MemUnLock (pTML->hTMList);
    MemUnLock (*phTM);
}


/***    UpdateEEHSTR - Update EE String
 *
 *      flag = UpdateEEHSTR(phStr, lszNew)
 *
 *      Entry   phStr = pointer to handle of EE String to be updated
 *              lszNew = Null-terminated string
 *
 *      Exit    The contents of EE string are replaced with lszNew
 *
 *      Returns TRUE if the contents of the EE string have changed
 *              FALSE otherwise
 */

bool_t FASTCALL UpdateEEHSTR(EEHSTR *phStr, LSZ lszNew)
{
    EEHSTR      hNew;
    uint        lenNew;
    LSZ         lsz;

    lenNew = _tcslen (lszNew) + 1;
    if (*lszNew) {
        if (*phStr) {
            // new string is non-NULL, old string is non-NULL
            lsz = (LSZ) MemLock (*phStr);
            if (!_tcscmp(lsz, lszNew)) {
                // strings are the same, no update necessary
                MemUnLock (*phStr);
                return FALSE;
            }
            else {
                MemUnLock (*phStr);
                if ((hNew = MemReAlloc (*phStr, lenNew)) == 0) {
                    return FALSE;
                }
            }
        }
        else if ((hNew = MemAllocate (lenNew)) == 0) {
            return FALSE;
        }
        // copy new string over the old string
        lsz = (LSZ) MemLock (hNew);
        _tcscpy (lsz, lszNew);
        MemUnLock (hNew);
        *phStr = hNew;
        return TRUE;
    }
    else if (*phStr) {
        // new string is NULL, old string is non-NULL
        // Free old string
        MemFree (*phStr);
        *phStr = 0;
        return TRUE;
    }

    // both strings are NULL, no update
    return FALSE;
}


/***    FindAERule - Find Auto Expansion rule
 *
 *      flag = FindAERule (pvP, buf, cchbuf)
 *
 *      Entry   pvP = pointer to the evaluation node containing the
 *                  result of the expression that is auto-expanded
 *              buf = buffer to hold the auto-expansion rule
 *              cchbuf = max buffer size
 *
 *      Exit    if an auto-expansion rule is found for pvP it is loaded
 *              into buf.
 *              Otherwise the class hierarchy is searched in order to
 *              locate the nearest base class for which an auto-expansion
 *              rule exists
 *
 *      Returns TRUE if an auto-expansion rule was found
 *              FALSE otherwise
 */

bool_t FASTCALL FindAERule (peval_t pvP, LSZ buf, uint cchbuf)
{
    search_t        Name;
    psearch_t       pName = &Name;
    eval_t          Eval;
    peval_t         pv = &Eval;
    LSZ             lsz;
    CV_typ_t        typ = EVAL_TYP (pvP);
    HMOD            hMod = EVAL_MOD (pvP);
    bool_t          retval = FALSE;
    HEXE            hExe = SHHexeFromHmod(hMod);

    if (LookupAECache (typ, SHHexeFromHmod(hMod), buf)) {
        return TRUE;
    }

    // will have to search for the rule based on
    // the type name
    lsz = GetTypeName (typ, hMod);
    if (LoadAERule (lsz, buf, cchbuf)) {
        retval = TRUE;
    }
    else {
        BOOL fSupBaseSaved = pExState->state.fSupBase;
        pExState->state.fSupBase = FALSE;

        InitSearchAErule (pName, pv, EVAL_TYP (pvP), EVAL_MOD (pvP));
        if (HR_found == SearchSym (pName)) {
            PopStack();
            lsz = GetTypeName (EVAL_TYP (pv), EVAL_MOD (pv));
            retval = LoadAERule (lsz, buf, cchbuf);
        }

        pExState->state.fSupBase = fSupBaseSaved;
    }

    InsertAECache (typ, hExe, buf);
    return retval;
}


/*
 *  Interface for accessing the auto-expansion rule cache
 *
 *  void InsertAECache (type, hExe, buf)
 *      inserts the rule containd in buf into the cache and associates
 *      it with type "type" defined in executable "hExe"
 *
 *  void ReorderCache (i)
 *      brings entry i to the top (helper routine for implementing
 *      LRU scheme)
 *
 *  flag = LookupAECache (type, hExe, buf)
 *      looks-up the rule associated with type "type" defined in "hExe"
 *      and loads it in buffer buf
 *      It is assumed that buf is sufficiently large to hold the string
 */


void FASTCALL InsertAECache (CV_typ_t type, HEXE hExe, char far *buf)
{
    int     i;
    EEHSTR  hStr;
    LSZ     lsz;

    if ((hStr = MemAllocate (_tcslen (buf) + 1)) != NULL) {
        lsz = (LSZ) MemLock (hStr);
        _tcscpy (lsz, buf);
        MemUnLock ((HDEP)lsz);

        if (cAECache == AECACHE_MAX) {
            if (AECache[AECACHE_MAX - 1].hStr)
                MemFree (AECache[AECACHE_MAX - 1].hStr);
            cAECache--;
        }

        for (i = cAECache; i > 0; i--) {
            AECache[i] = AECache[i - 1];
        }
        AECache[0].typ = type;
        AECache[0].hExe = hExe;
        AECache[0].hStr = hStr;
        cAECache++;
    }
}


void FASTCALL ReorderAECache (int iAECache)
{
    aecache_t   temp;
    int         i;

    if (iAECache == 0) {
        return;
    }
    temp = AECache[iAECache];
    for (i = iAECache; i > 0; i--) {
        AECache[i] = AECache[i - 1];
    }
    AECache[0] = temp;
}

bool_t FASTCALL LookupAECache (CV_typ_t typ, HEXE hExe, char *buf)
{
    int     i;
    LSZ     lsz;
    bool_t  retval = FALSE;

    for (i=0; i<cAECache; i++) {
        if (typ == AECache[i].typ && hExe == AECache[i].hExe) {
            lsz = (LSZ) MemLock (AECache[i].hStr);
            _tcscpy (buf, lsz);
            MemUnLock (AECache[i].hStr);
            ReorderAECache (i);
            retval = TRUE;
            break;
        }
    }

#ifdef DEBUGVER
    {
        static int hit = 0;
        static int miss = 0;
        if (retval == TRUE)
            hit ++;
        else
            miss ++;
    }
#endif

    return retval;
}
