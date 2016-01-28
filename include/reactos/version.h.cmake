/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS kernel
 * FILE:        include/reactos/version.h.cmake
 * PURPOSE:     Defines the current version
 * PROGRAMMER:  David Welch (welch@mcmail.com)
 * REVISIONS:
 * 	1999-11-06 (ea)
 * 		Moved from include/internal in include/reactos
 *		to be used by buildno.
 *	2002-01-17 (ea)
 *		KERNEL_VERSION removed. Use
 *		reactos/buildno.h:KERNEL_VERSION_STR instead.
 */

#ifndef __VERSION_H
#define __VERSION_H

#define KERNEL_VERSION_MAJOR @KERNEL_VERSION_MAJOR@
#define KERNEL_VERSION_MINOR @KERNEL_VERSION_MINOR@
#define KERNEL_VERSION_PATCH_LEVEL @KERNEL_VERSION_PATCH_LEVEL@

#define COPYRIGHT_YEAR "@COPYRIGHT_YEAR@"

/* KERNEL_VERSION_BUILD_TYPE is L"SVN", L"RC1", L"RC2" or L"" (for the release) */
#define KERNEL_VERSION_BUILD_TYPE "@KERNEL_VERSION_BUILD_TYPE@"

#endif

/* EOF */
