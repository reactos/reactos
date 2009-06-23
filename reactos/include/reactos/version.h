/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS kernel
 * FILE:        include/internal/version.h
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

#define KERNEL_VERSION_MAJOR        0
#define KERNEL_VERSION_MINOR        3
#define KERNEL_VERSION_PATCH_LEVEL  10

#define COPYRIGHT_YEAR              "2009"

/* KERNEL_VERSION_BUILD_TYPE is L"SVN", L"RC1", L"RC2" or L"" (for the release) */
#define KERNEL_VERSION_BUILD_TYPE   L""


#endif

/* EOF */
