///////////////////////////////////////////////////////////////////////////////
/*  File: debug.cpp

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

        The library is designed to be activated with the DBG macro.
        If DBG is not defined as 1, there is no trace of this code in your
        product.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    01/19/98    Replaced with version from CSC cache viewer.         BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#if DBG

//
// Defaults for the DebugParams members.
// By default, tracing and printing should be silent (no output).
// The default mask values of 0 ensure this.
// Also, printing and tracing are not verbose by default.
// Note that errors and asserts are always active when DBG is defined as 1.
// Errors and asserts are also always verbose.
//

LPCTSTR DebugParams::m_pszModule = TEXT("");
UINT DebugParams::TraceLevel     = 0;
UINT DebugParams::PrintLevel     = 0;
bool DebugParams::TraceVerbose   = false;
bool DebugParams::PrintVerbose   = false;
bool DebugParams::TraceOnExit    = true;
ULONGLONG DebugParams::TraceMask = 0;
ULONGLONG DebugParams::PrintMask = 0;

//
// Static default values for DebugPrint and DebugTrace classes.
//
const ULONGLONG DebugPrint::DEFMASK = (ULONGLONG)-1;
const UINT DebugPrint::DEFLEVEL     = 0;
const ULONGLONG DebugTrace::DEFMASK = (ULONGLONG)-1;
const UINT DebugTrace::DEFLEVEL     = 0;


LPCTSTR 
DebugParams::SetModule(
    LPCTSTR pszModule
    )
{
    LPCTSTR pszModulePrev = m_pszModule;
    m_pszModule = pszModule;
    return pszModulePrev;
}

void
DebugParams::SetDebugMask(
    ULONGLONG llMask
    )
{
    TraceMask = PrintMask = llMask;
}

void
DebugParams::SetDebugLevel(
    UINT uLevel
    )
{
    TraceLevel = PrintLevel = uLevel;
}

void
DebugParams::SetDebugVerbose(
    bool bVerbose
    )
{
    TraceVerbose = PrintVerbose = bVerbose;
}

void
DebugParams::SetTraceOnExit(
    bool bTrace
    )
{
    TraceOnExit = bTrace;
}


void *
DebugParams::GetItemPtr(
    DebugParams::Item item,
    DebugParams::Type type
    )
{
    //
    // Assertions are active for all levels, any program function (mask = -1)
    // and are always verbose.
    //
    static bool      bAssertVerbose = true;
    static UINT      uAssertLevel   = 0;
    static ULONGLONG llAssertMask   = DM_ALL;

    //
    // This array just eliminates the need for a lot of code when setting
    // or reading the various global DebugParam members.
    //
    static void *rgpMember[eTypeMax][eItemMax] = { { &TraceMask,  &TraceLevel,  &TraceVerbose },
                                                   { &PrintMask,  &PrintLevel,  &PrintVerbose },
                                                   { &llAssertMask,   &uAssertLevel,   &bAssertVerbose  }
                                                 };
    
    return rgpMember[type][item];
}


ULONGLONG 
DebugParams::SetMask(
    ULONGLONG llMask,
    DebugParams::Type type
    )
{
    ULONGLONG *pllMask   = (ULONGLONG *)GetItemPtr(DebugParams::eMask, type);
    ULONGLONG llMaskPrev = *pllMask;
    *pllMask = llMask;
    return llMaskPrev;
}


UINT 
DebugParams::SetLevel(
    UINT uLevel,
    DebugParams::Type type
    )
{
    UINT *puLevel = (UINT *)GetItemPtr(DebugParams::eLevel, type);
    UINT uLevelPrev = *puLevel;
    *puLevel = uLevel;
    return uLevelPrev;
}


bool 
DebugParams::SetVerbose(
    bool bVerbose,
    DebugParams::Type type
    )
{
    bool *pbVerbose = (bool *)GetItemPtr(DebugParams::eVerbose, type);
    bool bVerbosePrev = *pbVerbose;
    *pbVerbose = bVerbose;
    return bVerbosePrev;
}


DebugTrace::DebugTrace(
    LPCTSTR pszFile, 
    INT iLineNo
    ) : m_pszFile(pszFile),
        m_iLineNo(iLineNo),
        m_llMask(0),
        m_uLevel(0)
{
    //
    // Do nothing.
    //
}


void
DebugTrace::Enter(
    ULONGLONG llMask,
    UINT uLevel,
    LPCTSTR pszBlockName
    ) const
{
    DebugPrint(DebugParams::eTrace, m_pszFile, m_iLineNo).Print(m_llMask = llMask, m_uLevel = uLevel, TEXT("++ ENTER %s"), m_pszBlockName = pszBlockName);
}

void
DebugTrace::Enter(
    ULONGLONG llMask,
    UINT uLevel,
    LPCTSTR pszBlockName,
    LPCTSTR pszFmt,
    ...
    ) const
{
    va_list args;
    va_start(args, pszFmt);
    TCHAR szMsg[1024];
    wvsprintf(szMsg, pszFmt, args);
    va_end(args);
    DebugPrint(DebugParams::eTrace, m_pszFile, m_iLineNo).Print(m_llMask = llMask, m_uLevel = uLevel, TEXT("++ ENTER %s: %s"), m_pszBlockName = pszBlockName, szMsg);
}

void
DebugTrace::Enter(
    LPCTSTR pszBlockName
    ) const
{
    Enter(DebugTrace::DEFMASK, DebugTrace::DEFLEVEL, pszBlockName);
}

void
DebugTrace::Enter(
    LPCTSTR pszBlockName,
    LPCTSTR pszFmt,
    ...
    ) const
{
    va_list args;
    va_start(args, pszFmt);
    TCHAR szMsg[1024];
    wvsprintf(szMsg, pszFmt, args);
    va_end(args);
    DebugPrint(DebugParams::eTrace, m_pszFile, m_iLineNo).Print(m_llMask = DebugTrace::DEFMASK, m_uLevel = DebugTrace::DEFLEVEL, TEXT("++ ENTER %s: %s"), m_pszBlockName = pszBlockName, szMsg);
}


DebugTrace::~DebugTrace(void)
{
    if (DebugParams::TraceOnExit)
        DebugPrint(DebugParams::eTrace, m_pszFile, m_iLineNo).Print(m_llMask, m_uLevel, TEXT("-- LEAVE %s"), m_pszBlockName);
}


DebugPrint::DebugPrint(
    DebugParams::Type type,
    LPCTSTR pszFile,
    INT iLineNo
    ) : m_pszFile(pszFile),
        m_iLineNo(iLineNo),
        m_type(type)
{
    //
    // Do nothing.
    //
}


void
DebugPrint::Print(
    LPCTSTR pszFmt,
    ...
    ) const
{
    va_list args;
    va_start(args, pszFmt);
    Print(DebugPrint::DEFMASK, DebugPrint::DEFLEVEL, pszFmt, args);
    va_end(args);
}


void 
DebugPrint::Print(
    ULONGLONG llMask,
    UINT uLevel,
    LPCTSTR pszFmt,
    ...
    ) const
{
    va_list args;
    va_start(args, pszFmt);
    Print(llMask, uLevel, pszFmt, args);
    va_end(args);
}


//
// Determine if there are any corresponding bits set in two ULONGLONG
// values.  Can't just do a simple bitwise AND operation because the compiler
// truncates the operands to integer size.
//
bool
DebugPrint::AnyBitSet(
    ULONGLONG llMask,
    ULONGLONG llTest
    )
{
    ULARGE_INTEGER ulMask, ulTest;
    ulMask.QuadPart = llMask;
    ulTest.QuadPart = llTest;

    return (ulMask.LowPart & ulTest.LowPart) || (ulMask.HighPart & ulTest.HighPart);
}



//
// Internal [private] print function.
// All other print functions end up here.
//
void
DebugPrint::Print(
    ULONGLONG llMask,
    UINT uLevel,
    LPCTSTR pszFmt,
    va_list args
    ) const
{
//
// Crude check to make sure we haven't overflowed the text buffer.
// It's 1K so I don't expect it.  But if we do, it needs to be 
// announced somehow so either the buffer can be enlarged or the 
// message text reduced.  I can't use DBGASSERT because that will
// cause recursion.
//
#define CHECKOVERFLOW \
if (pszWrite >= (pszEnd - 3)) {\
    OutputDebugString(TEXT("Buffer overflow in DebugPrint::Print, File:")TEXT(__FILE__)TEXT(" Line:")TEXT("#__LINE__")); \
    DebugBreak(); }

    //
    // Retrieve the global DebugParam members for the "type" being printed.
    // i.e. ePrint, eAssert or eTrace.
    //
    ULONGLONG *pllMask = (ULONGLONG *)DebugParams::GetItemPtr(DebugParams::eMask, m_type);
    UINT *puLevel      = (UINT *)DebugParams::GetItemPtr(DebugParams::eLevel, m_type);
    bool *pbVerbose    = (bool *)DebugParams::GetItemPtr(DebugParams::eVerbose, m_type);

    if ((uLevel <= *puLevel) && AnyBitSet(llMask, *pllMask))
    {
        //
        // The statement is both "mask" and "level" enabled.
        // Generate debugger output.
        //
        TCHAR szText[1024];
        LPTSTR pszWrite = szText;
        LPCTSTR pszEnd  = pszWrite + ARRAYSIZE(szText);

        //
        // Each message has "[<module>:<thread>]" prefix.
        //
        pszWrite += wsprintf(pszWrite, 
                             TEXT("[%s:%d] "), 
                             DebugParams::m_pszModule,
                             GetCurrentThreadId());
        CHECKOVERFLOW;

        //
        // Append the message text (formatted).
        //
        pszWrite += wvsprintf(pszWrite, pszFmt, args);

        CHECKOVERFLOW;

        if (*pbVerbose)
        {
            //
            // Verbose output is desired.  Add the filename/line number pair
            // indented on the next line.
            //
            pszWrite += wsprintf(pszWrite, TEXT("\n\r\t+->File: %s, Line: %d"), m_pszFile, m_iLineNo);
        }

        CHECKOVERFLOW;

        //
        // Append a CRLF.
        //
        lstrcpy(pszWrite, TEXT("\n\r"));
        OutputDebugString(szText);
    }
}


DebugError::DebugError(
    LPCTSTR pszFile,
    INT iLineNo
    ) : DebugPrint(DebugParams::ePrint, pszFile, iLineNo)
{
    //
    // Do nothing.
    //
}

void
DebugError::Error(
    LPCTSTR pszFmt,
    ...
    ) const
{
    va_list args;
    va_start(args, pszFmt);    
    ULONGLONG llMaskSaved   = DebugParams::PrintMask;
    UINT      uLevelSaved   = DebugParams::PrintLevel;
    DebugParams::PrintMask = (ULONGLONG)-1;
    DebugParams::PrintLevel = 99999;
    Print((ULONGLONG)-1, 0, pszFmt, args);
    DebugParams::PrintLevel = uLevelSaved;
    DebugParams::PrintMask = llMaskSaved;
    va_end(args);
}        


DebugAssert::DebugAssert(
    LPCTSTR pszFile,
    INT iLineNo,
    LPCTSTR pszTest
    )
{
    DebugPrint PrintThis(DebugParams::eAssert, pszFile, iLineNo);
    ULONGLONG llMaskSaved   = DebugParams::PrintMask;
    UINT      uLevelSaved   = DebugParams::PrintLevel;
    DebugParams::PrintMask = (ULONGLONG)-1;
    DebugParams::PrintLevel = 99999;
    PrintThis.Print((ULONGLONG)-1, 0, pszTest);
    DebugParams::PrintLevel = uLevelSaved;
    DebugParams::PrintMask = llMaskSaved;
    DebugBreak();
}

#endif // DBG
