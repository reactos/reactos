/*
 * PROJECT:         ReactOS C++ runtime library
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         __CxxFrameHandler3 to __CxxFrameHandler wrapper
 * PROGRAMMER:      Thomas Faber (thomas.faber@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <ndk/rtltypes.h>

#define WINE_NO_TRACE_MSGS
#include <wine/debug.h>
#include <wine/exception.h>
#include <internal/wine/msvcrt.h>
#include <internal/wine/cppexcept.h>

extern DWORD CDECL CallCxxFrameHandler(PEXCEPTION_RECORD rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                       PCONTEXT context, EXCEPTION_REGISTRATION_RECORD **dispatch,
                                       const cxx_function_descr *descr);

DWORD
__stdcall
CxxHandleV8Frame(
    _In_ PEXCEPTION_RECORD rec,
    _In_ EXCEPTION_REGISTRATION_RECORD *frame,
    _In_ PCONTEXT context,
    _In_ EXCEPTION_REGISTRATION_RECORD **dispatch,
    _In_ const cxx_function_descr *descr)
{
    cxx_function_descr stub_descr;

    if (descr->magic != CXX_FRAME_MAGIC_VC8)
        return CallCxxFrameHandler(rec, frame, context, dispatch, descr);

    if ((descr->flags & FUNC_DESCR_SYNCHRONOUS) &&
        (rec->ExceptionCode != CXX_EXCEPTION))
    {
        return ExceptionContinueSearch;  /* handle only c++ exceptions */
    }

    stub_descr = *descr;
    stub_descr.magic = CXX_FRAME_MAGIC_VC7;
    return CallCxxFrameHandler(rec, frame, context, dispatch, &stub_descr);
}
