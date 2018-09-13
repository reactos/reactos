/*================================================================================
  File: gmacros.h
  Contains: Macros used in common by both the DHCP Server and the DHCP Client.
  	Most of them are inlines for sake of elegance and ease of usage.
  Author: RameshV
  Created: 04-Jun-97 00:01

================================================================================*/
//#include <align.h>

//  Some block macros; usage at end

//  Disable the warning about unreference labels.
#pragma warning(disable : 4102)

#define _shorten(string)    ( strrchr(string, '\\')? strrchr(string, '\\') : (string) )

// print a message and the file and line # of whoever is printing this.
#define _TracePrintLine(Msg)  DhcpPrint((DEBUG_TRACE_CALLS, "%s:%d %s\n", _shorten(__FILE__), __LINE__, Msg))

#define BlockBegin(Name)    { BlockStart_ ## Name : _TracePrintLine( "Block -> " #Name );
#define BlockEnd(Name)      BlockEnd_ ## Name : _TracePrintLine( "Block <- " #Name ) ;}
#define BlockContinue(Name) do { _TracePrintLine( "Continue to " #Name); goto BlockStart_ ## Name; } while (0)
#define BlockBreak(Name)    do { _TracePrintLine( "Breaking out of " #Name); goto BlockEnd_ ## Name; } while (0)
#define RetFunc(F,Ret)      do {_TracePrintLine( "Quitting function " #F ); return Ret ; } while (0)

// The way to use the above set of simple block macros is as follows: (example usage)
#if     0
int
DummyFunction(VOID) {
    BlockBegin(DummyFunctionMain) {
        if(GlobalCount > 0 )
            BlockContinue(DummyFunctionMain);
        else GlobalCount ++;

        if(GlobalCount > GlobalCountMax)
            BlockBreak(DummyFunctionMain);
    } BlockEnd(DummyFunctionMain);

    RetFunc(DummyFunction, RetVal);
}
#endif

// now come some little more complicated functions..
// note that these can be freely mixed with the above set of simple functions.
#define BlockBeginEx(Name, String)    {BlockStart_ ## Name : _TracePrintLine( #String );
#define BlockEndEx(Name, String)      BlockEnd_## Name : _TracePrintLine( #String );}
#define BlockContinueEx(Name, String) do {_TracePrintLine( #String); goto BlockStart_ ## Name; } while (0)
#define BlockBreakEx(Name, String)    do {_TracePrintLine( #String); goto BlockEnd_ ## Name; } while(0)

#define RetFuncEx(Name,Ret,DebMsg)    do {_TracePrintLine( "QuittingFunction " #Name); DhcpPrint(DebMsg); return Ret;} while(0)

// usage example:

#if 0
int
DummyFunction(VOID) {
    BlockBeginEx(Main, "Entering Dummy Function" ) {
        if( GlobalCount > 0)
            BlockContinueEx(Main, GlobalCount > 0);
        else GlobalCount ++;

        if(GlobalCount > GlobalCountMax)
            BlockBreak(Main);
    } BlockEndEx(Main, "Done Dummy Function");

    RetFunc(DummyFunc, RetVal);
    // OR
    RetFuncEx(DummyFunc, RetVal, (DEBUG_ERRROS, "Function returning, gcount = %ld\n", GlobalCount));

}

#endif 0


#define NOTHING

// Now if a VOID function (procedure) returns, we can say RetFunc(VoidFunc, NOTHING) and things will work.


//================================================================================
//  Now some useful inlines.
//================================================================================

VOID _inline
FreeEx(LPVOID Ptr) {
    if(Ptr) DhcpFreeMemory(Ptr);
}

VOID _inline
FreeEx2(LPVOID Ptr1, LPVOID Ptr2) {
    FreeEx(Ptr1); FreeEx(Ptr2);
}

VOID _inline
FreeEx3(LPVOID Ptr1, LPVOID Ptr2, LPVOID Ptr3) {
    FreeEx(Ptr1); FreeEx(Ptr2); FreeEx(Ptr3);
}

VOID _inline
FreeEx4(LPVOID Ptr1, LPVOID Ptr2, LPVOID Ptr3, LPVOID Ptr4) {
    FreeEx2(Ptr1, Ptr2); FreeEx2(Ptr3, Ptr4);
}

//--------------------------------------------------------------------------------
//  All the alloc functions below, allocate in one shot a few pointers,
//  and initialize them.. aligning them correctly.
//--------------------------------------------------------------------------------
LPVOID _inline
AllocEx(LPVOID *Ptr1, DWORD Size1, LPVOID *Ptr2, DWORD Size2) {
    DWORD  Size = ROUND_UP_COUNT(Size1, ALIGN_WORST) + Size2;
    LPBYTE Ptr = DhcpAllocateMemory(Size);

    if(!Ptr) return NULL;
    (*Ptr1) = Ptr;
    (*Ptr2) = Ptr + ROUND_UP_COUNT(Size1, ALIGN_WORST);

    return Ptr;
}

LPVOID _inline
AllocEx2(LPVOID *Ptr1, DWORD Size1, LPVOID *Ptr2, DWORD Size2) {
    DWORD  Size = ROUND_UP_COUNT(Size1, ALIGN_WORST) + Size2;
    LPBYTE Ptr = DhcpAllocateMemory(Size);

    if(!Ptr) return NULL;
    (*Ptr1) = Ptr;
    (*Ptr2) = Ptr + ROUND_UP_COUNT(Size1, ALIGN_WORST);

    return Ptr;
}

LPVOID _inline
AllocEx3(LPVOID *Ptr1, DWORD Size1, LPVOID *Ptr2, DWORD Size2, LPVOID *Ptr3, DWORD Size3) {
    DWORD  Size = ROUND_UP_COUNT(Size1, ALIGN_WORST) + ROUND_UP_COUNT(Size2, ALIGN_WORST) + Size3;
    LPBYTE Ptr = DhcpAllocateMemory(Size);

    if(!Ptr) return NULL;
    (*Ptr1) = Ptr;
    (*Ptr2) = Ptr + ROUND_UP_COUNT(Size1, ALIGN_WORST);
    (*Ptr3) = Ptr + ROUND_UP_COUNT(Size1, ALIGN_WORST) + ROUND_UP_COUNT(Size2, ALIGN_WORST);
    return Ptr;
}

LPVOID _inline
AllocEx4(LPVOID *Ptr1, DWORD Size1, LPVOID *Ptr2, DWORD Size2,
         LPVOID *Ptr3, DWORD Size3, LPVOID *Ptr4, DWORD Size4) {
    DWORD  Size = ROUND_UP_COUNT(Size1, ALIGN_WORST) +
    	ROUND_UP_COUNT(Size2, ALIGN_WORST) + ROUND_UP_COUNT(Size3, ALIGN_WORST) + Size4;
    LPBYTE Ptr = DhcpAllocateMemory(Size);

    if(!Ptr) return NULL;
    (*Ptr1) = Ptr;
    (*Ptr2) = Ptr + ROUND_UP_COUNT(Size1, ALIGN_WORST);
    (*Ptr3) = Ptr + ROUND_UP_COUNT(Size1, ALIGN_WORST) + ROUND_UP_COUNT(Size2, ALIGN_WORST);
    (*Ptr4) = Ptr + ROUND_UP_COUNT(Size1, ALIGN_WORST) +
    	ROUND_UP_COUNT(Size2, ALIGN_WORST) + ROUND_UP_COUNT(Size3, ALIGN_WORST);
    return Ptr;
}

//--------------------------------------------------------------------------------
//  This function takes an input string and a static buffer and if the input
//  string is not nul terminated, copies it to the static buffer and then null
//  terminates it.  It also change the size to reflect the new size..
//--------------------------------------------------------------------------------
LPBYTE _inline
AsciiNulTerminate(LPBYTE Input, DWORD *Size, LPBYTE StaticBuf, DWORD BufSize) {
    if( 0 == *Size) return Input;   // nothing to copy
    if(!Input[(*Size)-1]) return Input; // Everything is fine.

    if(*Size >= BufSize) {
        // Nothing much can be done here.. this is an error.. insufficient buffer space.
        DhcpAssert(FALSE);

        *Size = BufSize - 1;
    }

    memcpy(StaticBuf, Input, (*Size));
    StaticBuf[*Size] = '\0';
    (*Size) ++;
    return StaticBuf;
}

#if DBG
#define INLINE
#else
#define INLINE _inline
#endif

#define BEGIN_EXPORT
#define END_EXPORT

#define AssertReturn(Condition, RetVal )    do { DhcpAssert(Condition); return RetVal ;} while(0)

//================================================================================
//  End of File.
//================================================================================
