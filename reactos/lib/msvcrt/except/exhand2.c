#include <windows.h>

#ifdef __GNUC__
#else
ULONG DbgPrint(PCH Format,...)
{
    return 0;
}
#endif

VOID STDCALL
MsvcrtDebug(ULONG Value)
{
    //DbgPrint("MsvcrtDebug 0x%.08x\n", Value);
}

/*
 * @implemented
 */
EXCEPTION_DISPOSITION
_except_handler2(
struct _EXCEPTION_RECORD *ExceptionRecord,
void *Frame,
struct _CONTEXT *ContextRecord,
void *DispatcherContext)
{
    //printf("_except_handler2()\n");
    return 0;
}
