/* $Id:
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        subsys/psx/lib/psxdll/string/strcoll.c
 * PURPOSE:     string comparison using collating information
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              20/01/2002: Created
 */

#include <string.h>
#include <psx/debug.h>

int strcoll(const char *s1, const char *s2)
{
 TODO("locale semantics currently unimplemented");
 return (strcmp(s1, s2));
}

/* EOF */

