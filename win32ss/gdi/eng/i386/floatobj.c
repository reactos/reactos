/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/eng/i386/floatobj.c
 * PURPOSE:         FLOATOBJ API
 * PROGRAMMERS:     Jérôme Gardou
 */

/** Includes ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

LONG
APIENTRY
FLOATOBJ_GetLong(FLOATOBJ* pf)
{
    LONG val = 0;
    (void)FLOATOBJ_bConvertToLong(pf, &val);
    return val;
}
