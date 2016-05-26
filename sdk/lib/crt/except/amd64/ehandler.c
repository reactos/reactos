
#include <precomp.h>


_CRTIMP
EXCEPTION_DISPOSITION
__cdecl
__C_specific_handler(
    struct _EXCEPTION_RECORD *_ExceptionRecord,
    void *_EstablisherFrame,
    struct _CONTEXT *_ContextRecord,
    struct _DISPATCHER_CONTEXT *_DispatcherContext)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0;
}

DWORD
__CxxFrameHandler(
    PEXCEPTION_RECORD rec,
    EXCEPTION_REGISTRATION_RECORD* ExceptionRegistrationFrame,
    PCONTEXT context,
    EXCEPTION_REGISTRATION_RECORD** _ExceptionRecord)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0;
}

