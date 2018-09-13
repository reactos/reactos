#include "precomp.h"
#pragma hdrstop


BOOL
FIsDirectJump(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    // PPC direct (inlink) thunk looks like this:
    // <Unknown at this time>
    return FALSE;
}


BOOL
FIsIndirectJump(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    // PPC indirect (Dll Import) thunk looks like this:
    //  0x81620000              lwz   r.11,[toc]IAT(r.2) (requires fixup)
    //  0x818b0000              lwz   r.0,0(r.11)
    //  0x90410004              stw   r.2,glsave1(r.1)
    //             thunk.body:
    //  0x7d8903a6              mtctr r.0
    //  0x804b0004              lwz   r.2,4(r.11)
    //  0x4e800420              bctr

    return FALSE;
}

BOOL
FIsVCallThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    return FALSE;
}


BOOL
FIsVTDispAdjustorThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    return FALSE;
}


BOOL
FIsAdjustorThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    return FALSE;
}


BOOL
GetPMFDest(
    HTHDX hthd,
    UOFFSET uThis,
    UOFFSET uPMF,
    UOFFSET *lpuOffDest
    )
{
    return FALSE;
}
