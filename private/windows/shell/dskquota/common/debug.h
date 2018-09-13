#ifndef _INC_DSKQUOTA_DEBUG_H
#define _INC_DSKQUOTA_DEBUG_H
///////////////////////////////////////////////////////////////////////////////
/*  File: debug.h

    Description: Provides debugging macros to support tracing, debugger print
        statements, error message debugger output and assertions.
        
        I'm sure you're saying "why ANOTHER debugger output implementation".
        There are many around but I haven't found one that is as flexible and
        consistent as I would like.  This library suports the concept of
        both functional "masks" and detail "levels" to control the quantity
        of debugger output.
        
        Masks let you control debugger output based on program function.  For 
        instance, if you tag a DBGPRINT statement with the mask DM_XYZ, it 
        will only be activated if the global variable DebugParams::PrintMask
        has the DM_XYZ bit set.

        Levels let you control debugger output based on a level of desired 
        detail.  Sometimes you just want to see the basic functions happening 
        but other times, you need to see everything that's going on.  This 
        leveling allows you to specify at which level a macro is enabled.

        The library is designed to be activated with the DEBUG macro.
        If DBG is not defined as 1, there is no trace of this code in your
        product.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    01/19/98    Replaced module with version from CSC cache viewer.  BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#if DBG            // Only include file contents if DBG is defined as 1.
#   ifndef STRICT
#       define STRICT           // STRICT is required
#   endif
#   ifndef _WINDOWS_
#       include <windows.h>
#   endif
#   ifndef _INC_STDARG
#       include <stdarg.h>      // For va_list stuff.
#   endif

//
// If DBG is NOT defined as 1, none of this is included in your source.  
// The library is designed so that without DBG defined as 1, there is no trace of 
// this code in your product.
//
// The following 4 macros are placed in the program to generate debugger output.
//
// DBGTRACE  - Place at entry to function.  Will print message on entry and on exit.
// DBGPRINT  - For printing general program status messages to debugger.
// DBGERROR  - For printing error messages to debugger.
// DBGASSERT - Conventional assert macro.
//
// There are a couple of forms for the DBGTRACE and DBGPRINT macros.  The most
// basic form, assumes a mask of -1 and a level of 0.  This ensures the macro is
// enabled if any bit in the associated DebugParams mask is set and the associated
// DebugParams level is greater than 0.  The second version allows you to explicitly
// set the mask and level for the specific macro.
//
//      DBGTRACE((TEXT("MyFunction")));
//      DBGPRINT((TEXT("Thread ID = %d"), GetCurrentThreadId()));
//               |                                            |
//               +------- All args enclosed in parens (1) ----+
//
// or:
//
//      DBGTRACE((DM_REGISTRY, 2, TEXT("MyFunction")));
//      DBGPRINT((DM_REGISTRY, 2, TEXT("Thread ID = %d"), GetCurrentThreadId()));
//                     |       |
//                     |       +-- Level
//                     +-- Mask
//                     
//
// (1) So that no debug code is included in your retail product when DBG is not 
// defined as 1, the entire set of arguments to the DBGTRACE, DBGPRINT and DBGERROR macros 
// must be enclosed in parentheses.  This produces a single argument to the macro 
// which can be eliminated when DBG is not defined as 1 (See example above).
//
// The DBGERROR and DBGASSERT macros do not take mask and level arguments.  The
// mask is fixed at -1 and the level is fixed at 0 for both.
//
//
#define DBGTRACE(x)            DebugTrace _TraceThis(TEXT(__FILE__), __LINE__);  _TraceThis.Enter x
#define DBGPRINT(x)            DebugPrint(DebugParams::ePrint, TEXT(__FILE__),__LINE__).Print x
#define DBGERROR(x)            DebugError(TEXT(__FILE__),__LINE__).Error x
#define DBGASSERT(test)        ((test) ? (void)0 : DebugAssert(TEXT(__FILE__),__LINE__, TEXT("Assert failed: \"")TEXT(#test)TEXT("\"")))

//
// The following macros set the global control variables that affect the 4 debugger
// output macros.  All they do is set the static values in the DebugParams class.
// By default, DBGTRACE and DBGPRINT are silent.  You must activate them with the
// appropriate DBGxxxxxMASK macro.  DBGTRACE and DBGASSERT are always active whenever
// DBG is defined as 1.
//
// DBGMODULE        - Sets the "module name" included in debugger output.
// DBGPRINTMASK     - Sets the "mask" value applied to DBGPRINT macros.
// DBGPRINTLEVEL    - Sets the "level" value applied to DBGPRINT macros.
// DBGPRINTVERBOSE  - Controls if DBGPRINT output includes filename and line no.
// DBGTRACEMASK     - Sets the "mask" value applied to DBGTRACE macros.
// DBGTRACELEVEL    - Sets the "level" value applied to DBGTRACE macros.
// DBGTRACEVERBOSE  - Controls if DBGTRACE output includes filename and line no.
//                    This is helpful if you're tracing overloaded C++ functions.
// DBGMASK          - Sets the "mask" for both DBGPRINT and DBGTRACE macros.
//                    Same as calling DBGPRINTMASK(x) and DBGTRACEMASK(x)
// DBGLEVEL         - Sets the "level" for both DBGPRINT and DBGTRACE macros.
//                    Same as calling DBGPRINTLEVEL(x) and DBGTRACELEVEL(x)
// DBGVERBOSE       - Sets the "verbose" flag for both DBGPRINT and DBGTRACE macros.
//                    Same as calling DBGPRINTVERBOSE(x) and DBGTRACEVERBOSE(x)
// DBGTRACEONEXIT   - Enables DBGTRACE output on exit from functions.
//
#define DBGMODULE(modname)     DebugParams::SetModule(modname)
#define DBGPRINTMASK(mask)     DebugParams::SetPrintMask((ULONGLONG)mask)
#define DBGPRINTLEVEL(level)   DebugParams::SetPrintLevel(level)
#define DBGPRINTVERBOSE(tf)    DebugParams::SetPrintVerbose(tf)
#define DBGTRACEMASK(mask)     DebugParams::SetTraceMask((ULONGLONG)mask)
#define DBGTRACELEVEL(level)   DebugParams::SetTraceLevel(level)
#define DBGTRACEVERBOSE(tf)    DebugParams::SetTraceVerbose(tf)
#define DBGMASK(mask)          DebugParams::SetDebugMask((ULONGLONG)mask);
#define DBGLEVEL(level)        DebugParams::SetDebugLevel(level);
#define DBGVERBOSE(tf)         DebugParams::SetDebugVerbose(tf);
#define DBGTRACEONEXIT(tf)     DebugParams::SetTraceOnExit(tf);

//
// Pre-defined debug "levels".
// You can use whatever level values you want.  I've found that using more
// than 3 is confusing.  Basically, you want to define macro levels as
// "show me fundamental stuff", "show me more detail" and "show me everything".
// These three macros make it easier to stick to 3 levels.
// "DL_" = "Debug Level"
//
#define DL_HIGH   0  // "Show me fundamental stuff" - high priority
#define DL_MID    1  // "Show me more detail"       - mid priority
#define DL_LOW    2  // "Show me everything"        - low priority


//
// Some pre-defined debug mask values that I thought might be useful.
// These are not application-specific.  You can interpret them as you
// wish.  I've listed my interpretation in the comments.  In general,
// application function-specific mask values are more useful.  For
// example, you might create one called DM_DUMPSYMTAB to dump the
// contents of a symbol table at a specific point during execution.
// Create new mask values using the DBGCREATEMASK(x) macro defined below.
// "DM_" = "Debug Mask"
//
#define DM_NONE        (ULONGLONG)0x0000000000000000  // No debugging.
#define DM_NOW         (ULONGLONG)0x0000000000000001  // Activate temporarily
#define DM_CTOR        (ULONGLONG)0x0000000000000002  // C++ ctors and dtors
#define DM_REG         (ULONGLONG)0x0000000000000004  // Registry functions.
#define DM_FILE        (ULONGLONG)0x0000000000000008  // File accesses.
#define DM_GDI         (ULONGLONG)0x0000000000000010  // GDI functions.
#define DM_MEM         (ULONGLONG)0x0000000000000011  // Memory functions.
#define DM_NET         (ULONGLONG)0x0000000000000012  // Network functions.
#define DM_WEB         (ULONGLONG)0x0000000000000014  // Web browsing functions.
#define DM_DLG         (ULONGLONG)0x0000000000000018  // Dialog messages.
#define DM_WND         (ULONGLONG)0x0000000000000020  // Window messages.    
#define DM_ALL         (ULONGLONG)0xffffffffffffffff  // Activate always.

//
// Lower 16 bits reserved for pre-defined mask values.
// This leaves 48 mask values that the app can define.
// Use this macro to create app-specific values.
//
// i.e. 
//      #define DBGMASK_XYZ  DBGCREATEMASK(0x0001)
//      #define DBGMASK_ABC  DBCCREATEMASK(0x0002)
//
#define DBGCREATEMASK(value)  (ULONGLONG)((ULONGLONG)value << 16)

//
// Macro to print out an IID for debugging QI functions.
//
#define DBGPRINTIID(mask, level, riid) \
{ \
    TCHAR szTemp[50]; \
    StringFromGUID2(riid, szTemp, ARRAYSIZE(szTemp)); \
    DBGPRINT((mask, level, TEXT("IID = %s"), szTemp)); \
}

//
// For storing debug info in registry.
//
struct DebugRegParams
{
    ULONGLONG PrintMask;
    ULONGLONG TraceMask;
    UINT PrintLevel;    
    UINT TraceLevel;    
    bool PrintVerbose;
    bool TraceVerbose;
    bool TraceOnExit;
};
   

//
// Global debug parameters.
//
struct DebugParams
{
    //
    // Enumeration representing each of the debugging functions.
    //
    enum Type { eTrace = 0, ePrint, eAssert,  eTypeMax };
    //
    // Enumeration representing each of the debug parameters.
    //
    enum Item { eMask  = 0, eLevel, eVerbose, eItemMax };

    //
    // "Mask" that controls if a debugging function is enabled depending upon 
    // a desired function in the application domain.  Each bit in the mask 
    // corresponds to a given program function.  If at runtime, the bitwise
    // OR of this value and the "mask" value passed to the debugging function
    // is non-zero, the function is considered "mask enabled".
    // If a function is both "level enabled" and "mask enabled", it 
    // performs it's prescribed duties.
    // These values can be set by using the following macros:
    //
    //  DBGPRINTMASK(x)    - Sets mask for DBGPRINT only.
    //  DBGTRACEMASK(x)    - Sets mask for DBGTRACE only.
    //  DBGMASK(x)         - Sets mask for both.
    //
    // Note that there's no mask value for DebugAssert or DebugError.
    // These classes are always mask-enabled when DBG is defined as 1.
    //
    static ULONGLONG PrintMask;
    static ULONGLONG TraceMask;
    //
    // "Level" at which debug output is "enabled".
    // If at runtime, this value is >= the "level" value passed to the
    // debugging function, the function is considered "level enabled".
    // If a function is both "level enabled" and "mask enabled", it 
    // performs it's prescribed duties.
    // It is recommended that the set of allowable levels be limited
    // to avoid undue complexity.  The library doesn't impose a restriction
    // on allowable values.  However, I've found [0,1,2] to be sufficient.
    // These values can be set by using the following macros:
    //
    //  DBGPRINTLEVEL(x)    - Sets level for DBGPRINT only.
    //  DBGTRACELEVEL(x)    - Sets level for DBGTRACE only.
    //  DBGLEVEL(x)         - Sets level for both.
    //
    // Note that there's no level value for DebugAssert or DebugError.
    // These classes are always level-enabled when DBG is defined as 1.
    // 
    static UINT PrintLevel;    
    static UINT TraceLevel;    
    //
    // Flag to indicate if the debugger output should include the filename
    // and line number where the debug statement resides in the source file.
    // These values can be set by using the following macros:
    //
    //  DBGPRINTVERBOSE(x)  - Sets the verbose flag for DBGPRINT only.
    //  DBGTRACEVERBOSE(x)  - Sets the verbose flag for DBGTRACE only.
    //  DBGVERBOSE(x)       - Sets the verbose flag for both.
    //
    // Note that there's no verbose flag for DebugAssert or DebugError.
    // These classes always output verbose information.
    //
    static bool PrintVerbose;
    static bool TraceVerbose;
    //
    // Flag to indicate if DBGTRACE output is generated when leaving a function.
    // This value can be set by using the following macro:
    //
    //  DBGTRACEONEXIT
    //
    //      1 = Generate output [default]
    //      0 = Don't generate output.
    //
    static bool TraceOnExit;
    //
    // Address of the name string for the "current" module.  This name will be
    // included with each debugger message.  
    // It can be set using the DBGMODULE(name) macro.
    //
    static LPCTSTR m_pszModule;

    //
    // Some helper functions used by the DebugXxxxx classes.
    //
    static LPCTSTR SetModule(LPCTSTR pszModule);

    static void SetDebugMask(ULONGLONG llMask);

    static ULONGLONG SetPrintMask(ULONGLONG llMask)
        { return SetMask(llMask, ePrint); }

    static ULONGLONG SetTraceMask(ULONGLONG llMask)
        { return SetMask(llMask, eTrace); }

    static void SetDebugLevel(UINT uLevel);

    static UINT SetPrintLevel(UINT uLevel)
        { return SetLevel(uLevel, ePrint); }

    static UINT SetTraceLevel(UINT uLevel)
        { return SetLevel(uLevel, eTrace); }

    static void SetDebugVerbose(bool bVerbose);

    static bool SetPrintVerbose(bool bVerbose)
        { return SetVerbose(bVerbose, ePrint); }

    static bool SetTraceVerbose(bool bVerbose)
        { return SetVerbose(bVerbose, eTrace); }

    static void SetTraceOnExit(bool bTrace);

    static void *GetItemPtr(DebugParams::Item item, DebugParams::Type type);

    private:
        static ULONGLONG SetMask(ULONGLONG llMask, enum Type type);
        static UINT SetLevel(UINT uLevel, enum Type type);
        static bool SetVerbose(bool bVerbose, enum Type type);
};

//
// Class that prints a message an "ENTER" message upon construction
// and a "LEAVE" message upon destruction.  It is intended that the client
// place a DBGTRACE macro at the start of each function.  
// Depending on the current debug "level" and "mask" (see DebugParams),
// a message is printed to the debugger.  When the object goes out of scope,
// another message is automatically printed to the debugger.
// This class is only intended to be instantiated through the 
// DBGTRACE(x) macro.
//
class DebugTrace
{
    public:
        DebugTrace(LPCTSTR pszFile, INT iLineNo);
        ~DebugTrace(void);

        void Enter(void) const { m_llMask = (ULONGLONG)0; }
        void Enter(LPCTSTR pszBlockName) const;
        void Enter(ULONGLONG llMask, UINT uLevel, LPCTSTR pszBlockName) const;
        void Enter(LPCTSTR pszBlockName, LPCTSTR pszFmt, ...) const;
        void Enter(ULONGLONG llMask, UINT uLevel, LPCTSTR pszBlockName, LPCTSTR pszFmt, ...) const;

    private:
        INT                    m_iLineNo;        // Macro's source line number.
        LPCTSTR                m_pszFile;        // Macro's source file name.
        mutable ULONGLONG      m_llMask;         // Macro's "mask".
        mutable UINT           m_uLevel;         // Macro's "level".
        mutable LPCTSTR        m_pszBlockName;   // Ptr to string to print.
        static const ULONGLONG DEFMASK;          // Default mask for DebugTrace.
        static const UINT      DEFLEVEL;         // Default level for DebugTrace.
};


//
// Class for printing general messages to the debugger output.
// Place a DBGPRINT macro wherever you want to send interesting output to
// the debugger. Note that DebugError specifically handles error message output.
// Note that the DebugPrint class is used by the DebugAssert, DebugError
// and DebugTrace to perform debugger output.  The m_type member is used
// to identify which class the output is being produced for.
// This class is only intended to be instantiated through the 
// DBGPRINT(x) macro.
//
class DebugPrint
{
    public:
        DebugPrint(DebugParams::Type type, LPCTSTR pszFile, INT iLineNo);

        void Print(void) const { };
        void Print(LPCTSTR pszFmt, ...) const;
        void Print(ULONGLONG llMask, UINT uLevel, LPCTSTR pszFmt, ...) const;
        void Print(ULONGLONG llMask, UINT uLevel, LPCTSTR pszFmt, va_list args) const;

    private:
        INT                    m_iLineNo;    // Macro's source line number.
        LPCTSTR                m_pszFile;    // Macro's source file name.
        DebugParams::Type      m_type;       // Type of printing being done.
        static const ULONGLONG DEFMASK;      // Default mask for DebugPrint.
        static const UINT      DEFLEVEL;     // Default level for DebugPrint.

        static bool AnyBitSet(ULONGLONG llMask, ULONGLONG llTest);
};

//
// Specialization of the DebugPrint class.  It's just a DebugPrint with the
// mask fixed at -1 and the level fixed at 0 so that DBGERROR messages are 
// always output when DBG is defined as 1.  Note private inheritance prevents
// someone from calling DebugError::Print().  They must call 
// DebugError.Error() which calls DebugPrint::Print after setting a 
// default mask and level.
//
class DebugError : private DebugPrint
{
    public:
        DebugError(LPCTSTR pszFile, INT iLineNo);

        void Error(LPCTSTR pszFmt, ...) const;
};


//
// Creating a DebugAssert object automatically fires an assertion after 
// printing out the debug information.
//
class DebugAssert
{
    public:
        DebugAssert(LPCTSTR pszFile, INT iLineNo, LPCTSTR pszTest);
};


#else // DBG

#define DBGTRACE(x)
#define DBGPRINT(x)                          
#define DBGERROR(x)
#define DBGASSERT(test)
#define DBGMODULE(modname)
#define DBGPRINTMASK(mask)
#define DBGPRINTLEVEL(level)
#define DBGPRINTVERBOSE(tf)
#define DBGTRACEMASK(mask)
#define DBGTRACELEVEL(level)
#define DBGTRACEVERBOSE(tf)
#define DBGMASK(mask)
#define DBGLEVEL(level)
#define DBGVERBOSE(tf)
#define DBGTRACEONEXIT(tf)
#define DBGPRINTIID(mask, level, riid)
#endif // DBG

#endif // _INC_DSKQUOTA_DEBUG_H

