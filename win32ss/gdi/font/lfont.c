/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include "font.h"

#define NDEBUG
#include <debug.h>

INT
FASTCALL
LFONT_GetObject(PLFONT plfnt, INT cjSize, PVOID pvBuffer)
{
    ASSERT(FALSE);
    return 0;
}

HFONT
NTAPI
GreHfontCreate(
    IN ENUMLOGFONTEXDVW *pelfw,
    IN ULONG cjElfw,
    IN LFTYPE lft,
    IN FLONG  fl,
    IN PVOID pvCliData)
{
    PLFONT plfnt;
    HFONT hfont;

    /* Allocate an LFONT object */
    plfnt = (PLFONT)GDIOBJ_AllocObjWithHandle(GDIObjType_LFONT_TYPE << 16, sizeof(LFONT));
    if (!plfnt)
    {
        DPRINT1("Could not allocate LFONT object\n");
        return 0;
    }

    /* Get the handle */
    hfont = plfnt->baseobj.hHmgr;

    /* Set the fields */
    plfnt->lft = lft;
    plfnt->fl = fl;
    plfnt->elfexw = *pelfw;

    /* Set client data */
    GDIOBJ_vSetObjectAttr(&plfnt->baseobj, pvCliData);

    /* Unlock the object */
    GDIOBJ_vUnlockObject(&plfnt->baseobj);

    /* Return the handle */
    return hfont;
}


W32KAPI
HFONT
APIENTRY
NtGdiHfontCreate(
    IN ENUMLOGFONTEXDVW *pelfw,
    IN ULONG cjElfw,
    IN LFTYPE lft,
    IN FLONG  fl,
    IN PVOID pvCliData)
{
    ENUMLOGFONTEXDVW elfw;

    if (!pelfw)
    {
        DPRINT1("NtGdiHfontCreate: pelfw == NULL\n");
        return NULL;
    }

    /* Set maximum logfont size */
    if (cjElfw > sizeof(elfw)) cjElfw = sizeof(elfw);

    /* Enter SEH for buffer copy */
    _SEH2_TRY
    {
        ProbeForRead(pelfw, cjElfw, 1);
        RtlCopyMemory(&elfw, pelfw, cjElfw);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return NULL;)
    }
    _SEH2_END

    /* Call internal function */
    return GreHfontCreate(&elfw, cjElfw, lft, fl, pvCliData);
}

