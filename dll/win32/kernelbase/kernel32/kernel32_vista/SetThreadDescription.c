
#include "k32_vista.h"
#define NDEBUG
#include <debug.h>

#undef TRACE
#define TRACE DPRINT

/***********************************************************************
 *           SetThreadDescription   (kernelbase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH SetThreadDescription( HANDLE thread, PCWSTR description )
{
    THREAD_NAME_INFORMATION info;
    int length;

    TRACE( "(%p, %s)\n", thread, debugstr_w( description ));

    length = description ? lstrlenW( description ) * sizeof(WCHAR) : 0;

    if (length > USHRT_MAX)
        return HRESULT_FROM_NT(STATUS_INVALID_PARAMETER);

    info.ThreadName.Length = info.ThreadName.MaximumLength = length;
    info.ThreadName.Buffer = (WCHAR *)description;

    return HRESULT_FROM_NT(NtSetInformationThread( thread, ThreadNameInformation, &info, sizeof(info) ));
}
