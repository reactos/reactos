/* $Id: version.h,v 1.11 2002/10/31 17:50:55 jfilby Exp $
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

#define KERNEL_VERSION_MAJOR		0
#define KERNEL_VERSION_MINOR		0
#define KERNEL_VERSION_PATCH_LEVEL	21
/* Edit each time a new release is out: format is YYYYMMDD (UTC) */
#define KERNEL_RELEASE_DATE		20021031L


#endif

/* EOF */
