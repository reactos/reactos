/* $Id: strcoll.c,v 1.4 2002/10/29 04:45:42 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
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

