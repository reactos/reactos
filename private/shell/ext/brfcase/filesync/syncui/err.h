//
// err.h: Declares data, defines and struct types for error handling
//          module.
//
//

#ifndef __ERR_H__
#define __ERR_H__

// Requires comm.h to be included prior to this
//

/////////////////////////////////////////////////////  INCLUDES
/////////////////////////////////////////////////////  DEFINES

#ifdef DEBUG

// Dump flags used in g_uDumpFlags
//
#define DF_RECLIST      0x0001
#define DF_RECITEM      0x0002
#define DF_RECNODE      0x0004
#define DF_CREATETWIN   0x0008
#define DF_ATOMS        0x0010
#define DF_CRL          0x0020
#define DF_CBS          0x0040
#define DF_CPATH        0x0080
#define DF_PATHS        0x0100
#define DF_UPDATECOUNT  0x0200
#define DF_TWINPAIR     0x0400
#define DF_FOLDERTWIN   0x0800
#define DF_CHOOSESIDE   0x1000

// Break flags used in g_uBreakFlags
//
#define BF_ONOPEN       0x0001
#define BF_ONCLOSE      0x0002
#define BF_ONRUNONCE    0x0004
#define BF_ONVALIDATE   0x0010
#define BF_ONTHREADATT  0x0100
#define BF_ONTHREADDET  0x0200
#define BF_ONPROCESSATT 0x0400
#define BF_ONPROCESSDET 0x0800

#endif

// Trace flags used in g_uTraceFlags (defined in retail on purpose)
//
#define TF_ALWAYS       0x0000
#define TF_WARNING      0x0001
#define TF_ERROR        0x0002
#define TF_GENERAL      0x0004      // Standard briefcase trace messages
#define TF_FUNC         0x0008      // Trace function calls
#define TF_CACHE        0x0010      // Cache-specific trace messages
#define TF_ATOM         0x0020      // Atom-specific trace messages
#define TF_PROGRESS     0x0040      // Progress bar deltas

//---------------------------------------------------------------------------
// HRESULT error codes
//---------------------------------------------------------------------------

// Map a TWINRESULT error value into a HRESULT
// Note: TR_SUCCESS should not be mapped
// Note: This assumes that TWINRESULT errors fall in the range -32k to 32k.
//
#define TR_DELTAVALUE           1000
#define FACILITY_TR             0x018a          // magic number

#define HRESULT_FROM_TR(x)      (TR_SUCCESS == (x) ? NOERROR : \
                                                   ((HRESULT) ((((x) + TR_DELTAVALUE) & 0x0000FFFF) | (FACILITY_TR << 16) | 0x80000000)))
#define IS_ENGINE_ERROR(hr)     IsFlagSet(hr, ((FACILITY_TR << 16) | 0x80000000))
#define GET_TR(hr)              ((TWINRESULT)((hr) & 0x0000FFFF) - TR_DELTAVALUE)

HRESULT PUBLIC MapToOfficialHresult(HRESULT hres);

// SCODE values that correspond to TWINRESULT values
#define E_TR_RH_LOAD_FAILED         MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_RH_LOAD_FAILED)
#define E_TR_SRC_OPEN_FAILED        MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_SRC_OPEN_FAILED)
#define E_TR_SRC_READ_FAILED        MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_SRC_READ_FAILED)
#define E_TR_DEST_OPEN_FAILED       MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_DEST_OPEN_FAILED)
#define E_TR_DEST_WRITE_FAILED      MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_DEST_WRITE_FAILED)
#define E_TR_ABORT                  MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_ABORT)
#define E_TR_UNAVAILABLE_VOLUME     MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_UNAVAILABLE_VOLUME)
#define E_TR_OUT_OF_MEMORY          MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_OUT_OF_MEMORY)
#define E_TR_FILE_CHANGED           MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_FILE_CHANGED)
#define E_TR_DUPLICATE_TWIN         MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_DUPLICATE_TWIN)
#define E_TR_DELETED_TWIN           MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_DELETED_TWIN)
#define E_TR_HAS_FOLDER_TWIN_SRC    MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_HAS_FOLDER_TWIN_SRC)
#define E_TR_INVALID_PARAMETER      MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_INVALID_PARAMETER)
#define E_TR_SAME_FOLDER            MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_SAME_FOLDER)
#define E_TR_SUBTREE_CYCLE_FOUND    MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_SUBTREE_CYCLE_FOUND)
#define E_TR_NO_MERGE_HANDLER       MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_NO_MERGE_HANDLER)
#define E_TR_MERGE_INCOMPLETE       MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_MERGE_INCOMPLETE)
#define E_TR_TOO_DIFFERENT          MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_TOO_DIFFERENT)
#define E_TR_BRIEFCASE_LOCKED       MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_BRIEFCASE_LOCKED)
#define E_TR_BRIEFCASE_OPEN_FAILED  MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_BRIEFCASE_OPEN_FAILED)
#define E_TR_BRIEFCASE_READ_FAILED  MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_BRIEFCASE_READ_FAILED)
#define E_TR_BRIEFCASE_WRITE_FAILED MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_BRIEFCASE_WRITE_FAILED)
#define E_TR_CORRUPT_BRIEFCASE      MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_CORRUPT_BRIEFCASE)
#define E_TR_NEWER_BRIEFCASE        MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_NEWER_BRIEFCASE)
#define E_TR_NO_MORE                MAKE_SCODE(SEVERITY_ERROR, FACILITY_TR, TR_DELTAVALUE + TR_NO_MORE)

/////////////////////////////////////////////////////  MACROS

// Error table for lookup strings.  Usually an array of these
// structures is created and placed in the readonly data segment.
//
typedef struct _SETbl
    {
    HRESULT hres;       // standard result
    UINT    ids;        // String ID of message
    UINT    uStyle;     // MB_ flags
    } SETbl, * PSETBL;

typedef SETbl const *  PCSETBL;

int PUBLIC SEMsgBox(HWND hwnd, UINT idsCaption, SCODE sc, PCSETBL pseTable, UINT cArraySize);


// Retry loop
//
#define RETRY_BEGIN(bInit)      {BOOL bMyRetry; do { bMyRetry = (bInit);
#define RETRY_END()             } while (bMyRetry); }
#define RETRY_SET()             bMyRetry = TRUE
#define RETRY_CLEAR()           bMyRetry = FALSE


// Debugging macros
//

#ifdef DEBUG

#define DEBUG_CASE_STRING(x)    case x: return TEXT( #x )

#define ASSERTSEG

// Use this macro to declare message text that will be placed
// in the CODE segment (useful if DS is getting full)
//
// Ex: DEBUGTEXT(szMsg, "Invalid whatever: %d");
//
#define DEBUGTEXT(sz, msg)      /* ;Internal */ \
    static const TCHAR ASSERTSEG sz[] = msg;

void PUBLIC BrfAssertFailed(LPCTSTR szFile, int line);
void CPUBLIC BrfAssertMsg(BOOL f, LPCTSTR pszMsg, ...);
void CPUBLIC BrfDebugMsg(UINT mask, LPCTSTR pszMsg, ...);

// ASSERT(f)  -- Generate "assertion failed in line x of file.c"
//               message if f is NOT true.
//
#define ASSERT(f)                                                       \
    {                                                                   \
        DEBUGTEXT(szFile, TEXT(__FILE__));                              \
        if (!(f))                                                       \
            BrfAssertFailed(szFile, __LINE__);                          \
    }
#define ASSERT_E(f)  ASSERT(f)

// ASSERT_MSG(f, msg, args...)  -- Generate wsprintf-formatted msg w/params
//                          if f is NOT true.
//
#define ASSERT_MSG   BrfAssertMsg

// DEBUG_MSG(mask, msg, args...) - Generate wsprintf-formatted msg using
//                          specified debug mask.  System debug mask
//                          governs whether message is output.
//
#define DEBUG_MSG    BrfDebugMsg
#define TRACE_MSG    DEBUG_MSG

// VERIFYSZ(f, msg, arg)  -- Generate wsprintf-formatted msg w/ 1 param
//                          if f is NOT true 
//
#define VERIFYSZ(f, szFmt, x)   ASSERT_MSG(f, szFmt, x)


// VERIFYSZ2(f, msg, arg1, arg2)  -- Generate wsprintf-formatted msg w/ 2
//                          param if f is NOT true 
//
#define VERIFYSZ2(f, szFmt, x1, x2)   ASSERT_MSG(f, szFmt, x1, x2)



// DBG_ENTER(szFn)  -- Generates a function entry debug spew for
//                          a function 
//
#define DBG_ENTER(szFn)                  \
    TRACE_MSG(TF_FUNC, TEXT(" > ") szFn TEXT("()"))


// DBG_ENTER_SZ(szFn, sz)  -- Generates a function entry debug spew for
//                          a function that accepts a string as one of its
//                          parameters.
//
#define DBG_ENTER_SZ(szFn, sz)                  \
    TRACE_MSG(TF_FUNC, TEXT(" > ") szFn TEXT("(..., \"%s\",...)"), Dbg_SafeStr(sz))


// DBG_ENTER_DTOBJ(szFn, pdtobj, szBuf)  -- Generates a function entry 
//                          debug spew for a function that accepts a 
//                          string as one of its parameters.
//
#define DBG_ENTER_DTOBJ(szFn, pdtobj, szBuf) \
    TRACE_MSG(TF_FUNC, TEXT(" > ") szFn TEXT("(..., %s,...)"), Dbg_DataObjStr(pdtobj, szBuf))


// DBG_ENTER_RIID(szFn, riid)  -- Generates a function entry debug spew for
//                          a function that accepts an riid as one of its
//                          parameters.
//
#define DBG_ENTER_RIID(szFn, riid)                  \
    TRACE_MSG(TF_FUNC, TEXT(" > ") szFn TEXT("(..., %s,...)"), Dbg_GetRiidName(riid))


// DBG_EXIT(szFn)  -- Generates a function exit debug spew 
//
#define DBG_EXIT(szFn)                              \
        TRACE_MSG(TF_FUNC, TEXT(" < ") szFn TEXT("()"))

// DBG_EXIT_US(szFn, us)  -- Generates a function exit debug spew for
//                          functions that return a USHORT.
//
#define DBG_EXIT_US(szFn, us)                       \
        TRACE_MSG(TF_FUNC, TEXT(" < ") szFn TEXT("() with %#x"), (USHORT)us)

// DBG_EXIT_UL(szFn, ul)  -- Generates a function exit debug spew for
//                          functions that return a ULONG.
//
#define DBG_EXIT_UL(szFn, ul)                   \
        TRACE_MSG(TF_FUNC, TEXT(" < ") szFn TEXT("() with %#lx"), (ULONG)ul)

// DBG_EXIT_PTR(szFn, pv)  -- Generates a function exit debug spew for
//                          functions that return a pointer.
//
#define DBG_EXIT_PTR(szFn, pv)                   \
        TRACE_MSG(TF_FUNC, TEXT(" < ") szFn TEXT("() with %#lx"), (LPVOID)pv)

// DBG_EXIT_HRES(szFn, hres)  -- Generates a function exit debug spew for
//                          functions that return an HRESULT.
//
#define DBG_EXIT_HRES(szFn, hres)                   \
        TRACE_MSG(TF_FUNC, TEXT(" < ") szFn TEXT("() with %s"), Dbg_GetScode(hres))


#else

#define ASSERT(f)
#define ASSERT_E(f)      (f)
#define ASSERT_MSG   1 ? (void)0 : (void)
#define DEBUG_MSG    1 ? (void)0 : (void)
#define TRACE_MSG    1 ? (void)0 : (void)

#define VERIFYSZ(f, szFmt, x)     (f)

#define DBG_ENTER(szFn)
#define DBG_ENTER_SZ(szFn, sz)
#define DBG_ENTER_DTOBJ(szFn, pdtobj, sz)
#define DBG_ENTER_RIID(szFn, riid)   

#define DBG_EXIT(szFn)                            
#define DBG_EXIT_US(szFn, us)
#define DBG_EXIT_UL(szFn, ul)
#define DBG_EXIT_PTR(szFn, ptr)                            
#define DBG_EXIT_HRES(szFn, hres)   

#endif

/////////////////////////////////////////////////////  TYPEDEFS

/////////////////////////////////////////////////////  EXPORTED DATA

/////////////////////////////////////////////////////  PUBLIC PROTOTYPES

#ifdef DEBUG

void PUBLIC DEBUG_BREAK(UINT flag);

LPCTSTR PUBLIC Dbg_GetRiidName(REFIID riid);
LPCTSTR PUBLIC Dbg_GetScode(HRESULT hres);
LPCTSTR PUBLIC Dbg_SafeStr(LPCTSTR psz);
LPCTSTR PUBLIC Dbg_DataObjStr(LPDATAOBJECT pdtobj, LPTSTR pszBuf);

#endif

#endif // __ERR_H__

