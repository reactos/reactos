// pure.c - standalone definition of purecall in case its needed

#include <windows.h>

#define STATUS_NOT_IMPLEMENTED           0xC0000002L        // Copied from ntstatus.h
void __cdecl _purecall(void) 
{
#if DBG == 1
    #ifdef _M_IX86
        _asm int 3;
    #else
        DebugBreak();
    #endif // _M_IX86
#endif // DBG == 1

    RaiseException(STATUS_NOT_IMPLEMENTED, EXCEPTION_NONCONTINUABLE, 0, NULL);
}

