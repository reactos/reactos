/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/ntos/ddk.h
 * PURPOSE:      Missing definitions in MinGW Windows DDK
 * PROGRAMMER:   Casper S. Hornstrup <chorns@users.sourceforge.net>
 * NOTES:        Put defines that is missing in MinGW DDK, in this file and
 *               send a patch to the MinGW project. Then, when a new
 *               MinGW DDK is released, remove the defines from this file
 *               and use the new release.
 *               Please verify with documentation at msdn.microsoft.com
 *               before putting definitions here and submitting a patch
 *               to the MinGW project.
 */

#ifndef __INCLUDE_NTOS_DDK_H
#define __INCLUDE_NTOS_DDK_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifndef AS_INVOKED

#endif /* !AS_INVOKED */

#endif /* __INCLUDE_NTOS_DDK_H */
