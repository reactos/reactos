/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    WOW.C

Abstract:

    This file contains the code which is responsable for dealing with
    the WOW debug structures and debug events.

Author:

    James L Schaad (jimsch) 05-11-92

Environment:

    User Mode WIN32

--*/

#include "precomp.h"
#pragma hdrstop

/************************* GLOBAL VARIABLES ******************************/

extern char abEMReplyBuf[];
extern DDVECTOR RgfnFuncEventDispatch[];
extern DDVECTOR DebugDispatchTable[];
extern char nameBuffer[];

/************************** FUNCTIONS ************************************/


void
ProcessEntryPointEvent(
    DEBUG_EVENT64 *   pde,
    HTHDX           hthdx
    )
/*++

Routine Description:

    This handles task start events after the first one.

Arguments:


Return Value:


--*/
{
}



void
ProcessSegmentLoadEvent(
    DEBUG_EVENT64 *   pde,
    HTHDX           hthdx
    )

/*++

Routine Description:

    This function takes care of dealing with segment load events from
    the wow system.  These come in as exceptions and are translated
    to segment load events in ProcessDebugEvent.

Arguments:

    pde         - Supplies a pointer to the modified debug event
    hthdx       - Supplies the handle to the thread of the debug event

Return Value:

    None.

--*/

{
}                               /* ProcessSegmentLoadEvent() */

/*++

Routine Description:

    Callback for VDMEnumProcessWOW

Arguments:

    See PROCESSENUMPROC in vdmdbg.h

Return Value:

    TRUE

--*/
BOOL WINAPI EnumCallBack(
    DWORD   dwProcessId,
    DWORD   dwAttributes,
    LPARAM  lpUserDefined
    )
{
    UNREFERENCED_PARAMETER( dwProcessId );
    UNREFERENCED_PARAMETER( dwAttributes );
    UNREFERENCED_PARAMETER( lpUserDefined );

    return TRUE;
}



BOOL
IsWOWPresent(
    VOID
    )

/*++

Routine Description:

    Determines if WOW is running

Arguments:

    None

Return Value:

    TRUE if WOW is running, FALSE otherwise

--*/

{
    return FALSE;
}



BOOL
TranslateAddress(
    HPRCX  hprc,
    HTHDX  hthd,
    LPADDR lpaddr,
    BOOL   f16ToFlat
    )

/*++

Routine Description:

    This function is used to preform address translations from the segmented
    to the flat world and back again.

Arguments:

    hprc        - Supplies the handle to the current process

    hthd        - Supplies the handle to the thread for the address

    lpaddr      - Supplies the address to be translated

    f16ToFlat   - Supplies the direction the translation is to be made

Return Value:

    TRUE on success and FALSE on failure

--*/

{
    return TRUE;
}                               /* TranslateAddress() */


BOOL
WOWGetThreadContext(
    HTHDX     hthdx,
    LPCONTEXT lpcxt
    )

/*++

Routine Description:

    This routine is called to g the context of a WOW thread.  We have
    a current assumption that we will only have one WOW event at a time.

Arguments:

    hthdx       - supplies the handle to the thread to change the context of

    lpcxt       - supplies the new context.

Return Value:

    TRUE on success and FALSE on failure

--*/

{
    return FALSE;
}                               /* WOWGetThreadContext() */



BOOL
WOWSetThreadContext(
    HTHDX     hthdx,
    LPCONTEXT lpcxt
    )

/*++

Routine Description:

    This routine is called to set the context of a WOW thread.  We have
    a current assumption that we will only have one WOW event at a time.

Arguments:

    hthdx       - supplies the handle to the thread to change the context of
    lpcxt       - supplies the new context.

Return Value:

    TRUE on success and FALSE on failure

--*/

{
    return FALSE;
}                               /* WOWSetThreadContext() */
