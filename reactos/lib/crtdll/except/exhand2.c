#include <windows.h>
#include <ntos/except.h>


#ifdef __GNUC__
#else
#endif

ULONG DbgPrint(PCH Format, ...)
{
    return 0;
}

VOID STDCALL
MsvcrtDebug(ULONG Value)
{
    //DbgPrint("MsvcrtDebug 0x%.08x\n", Value);
}


/*
 * @unimplemented
 */
EXCEPTION_DISPOSITION
_except_handler2(
    struct _EXCEPTION_RECORD* ExceptionRecord, void* Frame,
    struct _CONTEXT *ContextRecord,            void* DispatcherContext)
{
    //printf("_except_handler2()\n");
    return 0;
}
