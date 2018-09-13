/*
 * debspew.h - Debug macros and their retail translations.
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
   ((void)(g_pcszSpewFile = __FILE__, \
           g_uSpewLine = __LINE__, \
           SpewOut args, \
           g_pcszSpewFile = NULL, \
           g_uSpewLine = 0, \
           g_uSpewSev = 0, \
           g_dwSpewFlags = 0, \
           0))

#define PLAIN_TRACE_OUT(args) \
   (g_dwSpewFlags = 0, \
    g_uSpewSev = SPEW_TRACE, \
    SPEW_OUT(args))

#define TRACE_OUT(args) \
   (g_dwSpewFlags = SPEW_FL_SPEW_PREFIX, \
    g_uSpewSev = SPEW_TRACE, \
    SPEW_OUT(args))

#define WARNING_OUT(args) \
   (g_dwSpewFlags = SPEW_FL_SPEW_PREFIX | SPEW_FL_SPEW_LOCATION, \
    g_uSpewSev = SPEW_WARNING, \
    SPEW_OUT(args))

#define ERROR_OUT(args) \
   (g_dwSpewFlags = SPEW_FL_SPEW_PREFIX | SPEW_FL_SPEW_LOCATION, \
    g_uSpewSev = SPEW_ERROR, \
    SPEW_OUT(args))

#define FATAL_OUT(args) \
   (g_dwSpewFlags = SPEW_FL_SPEW_PREFIX | SPEW_FL_SPEW_LOCATION, \
    g_uSpewSev = SPEW_FATAL, \
    SPEW_OUT(args))

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
    (ERROR_OUT(("invalid %s read pointer - %#08lx", (PCSTR)"P"#type, (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_WRITE_PTR(ptr, type) \
   (IsBadWritePtr((PVOID)(ptr), sizeof(type)) ? \
    (ERROR_OUT(("invalid %s write pointer - %#08lx", (PCSTR)"P"#type, (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_STRING_PTR(ptr, type) \
   (IsBadStringPtr((ptr), (UINT)-1) ? \
    (ERROR_OUT(("invalid %s pointer - %#08lx", (PCSTR)"P"#type, (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_CODE_PTR(ptr, type) \
   (IsBadCodePtr((FARPROC)(ptr)) ? \
    (ERROR_OUT(("invalid %s code pointer - %#08lx", (PCSTR)#type, (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_READ_BUFFER_PTR(ptr, type, len) \
   (IsBadReadPtr((ptr), len) ? \
    (ERROR_OUT(("invalid %s read pointer - %#08lx", (PCSTR)"P"#type, (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_WRITE_BUFFER_PTR(ptr, type, len) \
   (IsBadWritePtr((ptr), len) ? \
    (ERROR_OUT(("invalid %s write pointer - %#08lx", (PCSTR)"P"#type, (ptr))), FALSE) : \
    TRUE)

#define FLAGS_ARE_VALID(dwFlags, dwAllFlags) \
   (((dwFlags) & (~(dwAllFlags))) ? \
    (ERROR_OUT(("invalid flags set - %#08lx", ((dwFlags) & (~(dwAllFlags))))), FALSE) : \
    TRUE)

#else

#define IS_VALID_READ_PTR(ptr, type) \
   (! IsBadReadPtr((ptr), sizeof(type)))

#define IS_VALID_WRITE_PTR(ptr, type) \
   (! IsBadWritePtr((PVOID)(ptr), sizeof(type)))

#define IS_VALID_STRING_PTR(ptr, type) \
   (! IsBadStringPtr((ptr), (UINT)-1))

#define IS_VALID_CODE_PTR(ptr, type) \
   (! IsBadCodePtr((FARPROC)(ptr)))

#define IS_VALID_READ_BUFFER_PTR(ptr, type, len) \
   (! IsBadReadPtr((ptr), len))

#define IS_VALID_WRITE_BUFFER_PTR(ptr, type, len) \
   (! IsBadWritePtr((ptr), len))

#define FLAGS_ARE_VALID(dwFlags, dwAllFlags) \
   (((dwFlags) & (~(dwAllFlags))) ? FALSE : TRUE)

#endif

/* handle validation macros */

#ifdef DEBUG

#define IS_VALID_HANDLE(hnd, type) \
   (IsValidH##type(hnd) ? \
    TRUE : \
    (ERROR_OUT(("invalid H" #type " - %#08lx", (hnd))), FALSE))

#else

#define IS_VALID_HANDLE(hnd, type) \
   (IsValidH##type(hnd))

#endif

/* structure validation macros */

#ifdef VSTF

#ifdef DEBUG

#define IS_VALID_STRUCT_PTR(ptr, type) \
   (IsValidP##type(ptr) ? \
    TRUE : \
    (ERROR_OUT(("invalid %s pointer - %#08lx", (PCSTR)"P"#type, (ptr))), FALSE))

#else

#define IS_VALID_STRUCT_PTR(ptr, type) \
   (IsValidP##type(ptr))

#endif

#else

#define IS_VALID_STRUCT_PTR(ptr, type) \
   (! IsBadReadPtr((ptr), sizeof(type)))

#endif

/* OLE interface validation macro */

#define IS_VALID_INTERFACE_PTR(ptr, iface) \
   IS_VALID_STRUCT_PTR(ptr, C##iface)

/* debug break */

#ifdef DEBUG

#define DebugBreak() \
   __try \
   { \
      DebugBreak(); \
   } __except (EXCEPTION_CONTINUE_EXECUTION) {}

#else

#define DebugBreak()

#endif

/* debug exported function entry */

#ifdef DEBUG

#define DebugEntry(szFunctionName) \
   (TRACE_OUT((#szFunctionName "() entered.")), \
    StackEnter())

#else

#define DebugEntry(szFunctionName)

#endif

/* debug exported function exit */

#ifdef DEBUG

#define DebugExit(szFunctionName, szResult) \
   (StackLeave(), \
    TRACE_OUT(("%s() exiting, returning %s.", #szFunctionName, szResult)))

#define DebugExitBOOL(szFunctionName, bool) \
   DebugExit(szFunctionName, GetBOOLString(bool))

#define DebugExitCOMPARISONRESULT(szFunctionName, cr) \
   DebugExit(szFunctionName, GetCOMPARISONRESULTString(cr))

#define DebugExitDWORD(szFunctionName, dw) \
   DebugExitULONG(szFunctionName, dw)

#define DebugExitHRESULT(szFunctionName, hr) \
   DebugExit(szFunctionName, GetHRESULTString(hr))

#define DebugExitINT(szFunctionName, n) \
   DebugExit(szFunctionName, GetINTString(n))

#define DebugExitULONG(szFunctionName, ul) \
   DebugExit(szFunctionName, GetULONGString(ul))

#define DebugExitVOID(szFunctionName) \
   (StackLeave(), \
    TRACE_OUT(("%s() exiting.", #szFunctionName)))

#else

#define DebugExit(szFunctionName, szResult)
#define DebugExitBOOL(szFunctionName, bool)
#define DebugExitCOMPARISONRESULT(szFunctionName, cr)
#define DebugExitDWORD(szFunctionName, dw)
#define DebugExitHRESULT(szFunctionName, hr)
#define DebugExitINT(szFunctionName, n)
#define DebugExitULONG(szFunctionName, ul)
#define DebugExitVOID(szFunctionName)

#endif


/* Types
 ********/

/* g_dwSpewFlags flags */

typedef enum _spewflags
{
   SPEW_FL_SPEW_PREFIX        = 0x0001,

   SPEW_FL_SPEW_LOCATION      = 0x0002,

   ALL_SPEW_FLAGS             = (SPEW_FL_SPEW_PREFIX |
                                 SPEW_FL_SPEW_LOCATION)
}
SPEWFLAGS;

/* g_uSpewSev values */

typedef enum _spewsev
{
   SPEW_TRACE,

   SPEW_WARNING,

   SPEW_ERROR,

   SPEW_FATAL
}
SPEWSEV;


/* Prototypes
 *************/

/* debspew.c */

#ifdef DEBUG

extern BOOL SetDebugModuleIniSwitches(void);
extern BOOL InitDebugModule(void);
extern void ExitDebugModule(void);
extern void StackEnter(void);
extern void StackLeave(void);
extern ULONG GetStackDepth(void);
extern void SpewOut(PCSTR pcszFormat, ...);

#endif


/* Global Variables
 *******************/

#ifdef DEBUG

/* debspew.c */

extern DWORD g_dwSpewFlags;
extern UINT g_uSpewSev;
extern UINT g_uSpewLine;
extern PCSTR g_pcszSpewFile;

/* defined by client */

extern PCSTR g_pcszSpewModule;

#endif

