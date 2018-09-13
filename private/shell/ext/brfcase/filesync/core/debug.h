/*
 * debug.h - Debug macros and their retail translations.
 */


/* Macros
 *********/

/* debug output macros */

/*
 * Do not call SPEW_OUT directly.  Instead, call TRACE_OUT, WARNING_OUT,
 * ERROR_OUT, or FATAL_OUT.
 */

/*
 * call like printf(), but with an extra pair of parentheses:
 *
 * ERROR_OUT(("'%s' too big by %d bytes.", pszName, nExtra));
 */

#ifdef DEBUG

#define SPEW_OUT(args) \
   ((void)(GpcszSpewFile = TEXT(__FILE__), GuSpewLine = __LINE__, SpewOut args, 0))

#define PLAIN_TRACE_OUT(args) \
   (GdwSpewFlags = 0, GuSpewSev = SPEW_TRACE, SPEW_OUT(args))

#define TRACE_OUT(args) \
   (GdwSpewFlags = SPEW_FL_SPEW_PREFIX, GuSpewSev = SPEW_TRACE, SPEW_OUT(args))

#define WARNING_OUT(args) \
   (GdwSpewFlags = SPEW_FL_SPEW_PREFIX | SPEW_FL_SPEW_LOCATION, GuSpewSev = SPEW_WARNING, SPEW_OUT(args))

#define ERROR_OUT(args) \
   (GdwSpewFlags = SPEW_FL_SPEW_PREFIX | SPEW_FL_SPEW_LOCATION, GuSpewSev = SPEW_ERROR, SPEW_OUT(args))

#define FATAL_OUT(args) \
   (GdwSpewFlags = SPEW_FL_SPEW_PREFIX | SPEW_FL_SPEW_LOCATION, GuSpewSev = SPEW_FATAL, SPEW_OUT(args))

#else

#define PLAIN_TRACE_OUT(args)
#define TRACE_OUT(args)
#define WARNING_OUT(args)
#define ERROR_OUT(args)
#define FATAL_OUT(args)

#endif

/* parameter validation macros */

/*
 * call as:
 *
 * bPTwinOK = IS_VALID_READ_PTR(ptwin, CTWIN);
 *
 * bHTwinOK = IS_VALID_HANDLE(htwin, TWIN);
 */

#ifdef DEBUG

#define IS_VALID_READ_PTR(ptr, type) \
   (IsBadReadPtr((ptr), sizeof(type)) ? \
    (ERROR_OUT((TEXT("invalid %s read pointer - %#08lx"), (LPCTSTR)TEXT("P")TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_WRITE_PTR(ptr, type) \
   (IsBadWritePtr((PVOID)(ptr), sizeof(type)) ? \
    (ERROR_OUT((TEXT("invalid %s write pointer - %#08lx"), (LPCTSTR)TEXT("P")TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_STRING_PTRA(ptr, type) \
   (IsBadStringPtrA((ptr), (UINT)-1) ? \
    (ERROR_OUT((TEXT("invalid %s pointer - %#08lx"), (LPCTSTR)TEXT("P")TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_STRING_PTRW(ptr, type) \
   (IsBadStringPtrW((ptr), (UINT)-1) ? \
    (ERROR_OUT((TEXT("invalid %s pointer - %#08lx"), (LPCTSTR)TEXT("P")TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#ifdef UNICODE
#define IS_VALID_STRING_PTR(ptr, type) IS_VALID_STRING_PTRW(ptr, type)
#else
#define IS_VALID_STRING_PTR(ptr, type) IS_VALID_STRING_PTRA(ptr, type)
#endif

#define IS_VALID_CODE_PTR(ptr, type) \
   (IsBadCodePtr((PROC)(ptr)) ? \
    (ERROR_OUT((TEXT("invalid %s code pointer - %#08lx"), (LPCTSTR)TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_READ_BUFFER_PTR(ptr, type, len) \
   (IsBadReadPtr((ptr), len) ? \
    (ERROR_OUT((TEXT("invalid %s read pointer - %#08lx"), (LPCTSTR)TEXT("P")TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_WRITE_BUFFER_PTR(ptr, type, len) \
   (IsBadWritePtr((ptr), len) ? \
    (ERROR_OUT((TEXT("invalid %s write pointer - %#08lx"), (LPCTSTR)TEXT("P")TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#define FLAGS_ARE_VALID(dwFlags, dwAllFlags) \
   (((dwFlags) & (~(dwAllFlags))) ? \
    (ERROR_OUT((TEXT("invalid flags set - %#08lx"), ((dwFlags) & (~(dwAllFlags))))), FALSE) : \
    TRUE)

#else

#define IS_VALID_READ_PTR(ptr, type) \
   (! IsBadReadPtr((ptr), sizeof(type)))

#define IS_VALID_WRITE_PTR(ptr, type) \
   (! IsBadWritePtr((PVOID)(ptr), sizeof(type)))

#define IS_VALID_STRING_PTR(ptr, type) \
   (! IsBadStringPtr((ptr), (UINT)-1))

#define IS_VALID_CODE_PTR(ptr, type) \
   (! IsBadCodePtr((PROC)(ptr)))

#define IS_VALID_READ_BUFFER_PTR(ptr, type, len) \
   (! IsBadReadPtr((ptr), len))

#define IS_VALID_WRITE_BUFFER_PTR(ptr, type, len) \
   (! IsBadWritePtr((ptr), len))

#define FLAGS_ARE_VALID(dwFlags, dwAllFlags) \
   (((dwFlags) & (~(dwAllFlags))) ? FALSE : TRUE)

#endif

/* handle validation macros */

#define IS_VALID_HANDLE(hnd, type) \
   (IsValidH##type(hnd))

/* structure validation macros */

#ifdef VSTF

#ifdef DEBUG

#define IS_VALID_STRUCT_PTR(ptr, type) \
   (IsValidP##type(ptr) ? \
    TRUE : \
    (ERROR_OUT((TEXT("invalid %s pointer - %#08lx"), (LPCTSTR)TEXT("P")TEXT(#type), (ptr))), FALSE))

#else

#define IS_VALID_STRUCT_PTR(ptr, type) \
   (IsValidP##type(ptr))

#endif

#else

#define IS_VALID_STRUCT_PTR(ptr, type) \
   (! IsBadReadPtr((ptr), sizeof(type)))

#endif

/* debug assertion macro */

/*
 * ASSERT() may only be used as a statement, not as an expression.
 *
 * call as:
 *
 * ASSERT(pszRest);
 */

#ifdef DEBUG

#define ASSERT(exp) \
   if (exp) \
      ; \
   else \
      ERROR_OUT((TEXT("assertion failed '%s'"), (LPCTSTR)TEXT(#exp)))

#else

#define ASSERT(exp)

#endif

/* debug evaluation macro */

/*
 * EVAL() may be used as an expression.
 *
 * call as:
 *
 * if (EVAL(pszFoo))
 *    bResult = TRUE;
 */

#ifdef DEBUG

#define EVAL(exp) \
   ((exp) || (ERROR_OUT((TEXT("evaluation failed '%s'"), (LPCTSTR)TEXT(#exp))), 0))

#else

#define EVAL(exp) \
   (exp)

#endif

/* debug break */

#ifndef DEBUG

#define DebugBreak()

#endif

/* debug exported function entry */

#ifdef DEBUG

#define DebugEntry(szFunctionName) \
   (TRACE_OUT((TEXT(#szFunctionName) TEXT("() entered."))), StackEnter())

#else

#define DebugEntry(szFunctionName)

#endif

/* debug exported function exit */

#ifdef DEBUG

#define DebugExitVOID(szFunctionName) \
   (StackLeave(), TRACE_OUT((TEXT("%s() exiting."), TEXT(#szFunctionName))))

#define DebugExit(szFunctionName, szResult) \
   (StackLeave(), TRACE_OUT((TEXT("%s() exiting, returning %s."), TEXT(#szFunctionName), szResult)))

#define DebugExitINT(szFunctionName, n) \
   DebugExit(szFunctionName, GetINTString(n))

#define DebugExitULONG(szFunctionName, ul) \
   DebugExit(szFunctionName, GetULONGString(ul))

#define DebugExitBOOL(szFunctionName, bool) \
   DebugExit(szFunctionName, GetBOOLString(bool))

#define DebugExitHRESULT(szFunctionName, hr) \
   DebugExit(szFunctionName, GetHRESULTString(hr))

#define DebugExitCOMPARISONRESULT(szFunctionName, cr) \
   DebugExit(szFunctionName, GetCOMPARISONRESULTString(cr))

#define DebugExitTWINRESULT(szFunctionName, tr) \
   DebugExit(szFunctionName, GetTWINRESULTString(tr))

#define DebugExitRECRESULT(szFunctionName, rr) \
   DebugExit(szFunctionName, GetRECRESULTString(rr))

#else

#define DebugExitVOID(szFunctionName)
#define DebugExit(szFunctionName, szResult)
#define DebugExitINT(szFunctionName, n)
#define DebugExitULONG(szFunctionName, ul)
#define DebugExitBOOL(szFunctionName, bool)
#define DebugExitHRESULT(szFunctionName, hr)
#define DebugExitCOMPARISONRESULT(szFunctionName, cr)
#define DebugExitTWINRESULT(szFunctionName, tr)
#define DebugExitRECRESULT(szFunctionName, rr)

#endif


/* Types
 ********/

/* GdwSpewFlags flags */

typedef enum _spewflags
{
   SPEW_FL_SPEW_PREFIX     = 0x0001,

   SPEW_FL_SPEW_LOCATION   = 0x0002,

   ALL_SPEW_FLAGS          = (SPEW_FL_SPEW_PREFIX |
                              SPEW_FL_SPEW_LOCATION)
}
SPEWFLAGS;

/* GuSpewSev values */

typedef enum _spewsev
{
   SPEW_TRACE              = 1,

   SPEW_WARNING            = 2,

   SPEW_ERROR              = 3,

   SPEW_FATAL              = 4
}
SPEWSEV;


/* Prototypes
 *************/

/* debug.c */

#ifdef DEBUG

extern BOOL SetDebugModuleIniSwitches(void);
extern BOOL InitDebugModule(void);
extern void ExitDebugModule(void);
extern void StackEnter(void);
extern void StackLeave(void);
extern ULONG GetStackDepth(void);
extern void __cdecl SpewOut(LPCTSTR pcszFormat, ...);

#endif


/* Global Variables
 *******************/

#ifdef DEBUG

/* debug.c */

extern DWORD GdwSpewFlags;
extern UINT GuSpewSev;
extern UINT GuSpewLine;
extern LPCTSTR GpcszSpewFile;

/* defined by client */

extern LPCTSTR GpcszSpewModule;

#endif
