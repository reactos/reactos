/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/wine/winesup.c
 * PURPOSE:         Wine supporting functions
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

void set_error( unsigned int err )
{
}

const SID *token_get_user( void *token )
{
    return NULL;
}

/* EOF */
