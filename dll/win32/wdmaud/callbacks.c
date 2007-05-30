/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 lib/wdmaud/wdmaud.h
 * PURPOSE:              WDM Audio Support - Callbacks
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Nov 24, 2005: Started
 */

#include <windows.h>
#include "wdmaud.h"


/*
    CheckCallbacks

    This appears to just be used by the mixer stuff.

    If the global callback file mapping handle isn't set, return FALSE.

    Check the first parameter. If the value is below that of the first
    DWORD in the mapped view, return FALSE.

    TODO: Finish analysis
*/

BOOL CheckCallbacks()
{
    return FALSE;
}

/*
    CreateWdmaudCallbacks

    The original appears to use a security descriptor... We won't bother
    with that for now.

    Create a file mapping with the name "Global\WDMAUD_Callbacks".
    The current process is used as the file handle for the file mapping. The
    file mapping attributes should be set to a length of 12 bytes, the
    security descriptor we ignore and the handle doesn't need to be
    inheritable.

    The maximum size should be set to 1028 bytes, and the file view
    protection should be set to PAGE_READWRITE.

    The result of the file mapping creation is stored in the global callback
    handle.

    If the creation succeeded, try and map a view of the file, from offset 0
    for 1028 bytes, with READ and WRITE access.

    If this succeeds, close the created file mapping handle, and set the
    global handle to the one returned by the file mapping function.
*/

BOOL CreateWdmaudCallbacks()
{
    return FALSE;
}


/*
    GetWdmaudCallbacks

    If the global callback handle is already set, do nothing.

    Open the file mapping to "Global\WDMAUD_Callbacks" with READ and WRITE
    access. The handle doesn't need to be inherited.

    The handle is stored as the global callback handle.

    If the file was opened successfully, map the view of 1028 bytes from
    offset 0 with READ and WRITE access.

    If this fails, close the global callback handle and set it to NULL.

    ...and then return
*/

BOOL GetWdmaudCallbacks()
{
    return FALSE;
}
