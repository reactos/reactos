/* $Id: version.h,v 1.4 2000/08/05 18:01:48 dwelch Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS kernel
 * FILE:        include/internal/version.h
 * PURPOSE:     Defines the current version
 * PROGRAMMER:  David Welch (welch@mcmail.com)
 * REVISIONS:
 * 	1999-11-06 (ea)
 * 		Moved from include/internal in include/reactos
 * 		to be used by buildno.
 */

#ifndef __VERSION_H
#define __VERSION_H

#define KERNEL_VERSION_MAJOR    	0
#define KERNEL_VERSION_MINOR    	0
#define KERNEL_VERSION_PATCH_LEVEL      16
/* Edit each time a new release is out: format is YYYYMMDD (UTC) */
#define KERNEL_RELEASE_DATE		20000805L

#define KERNEL_VERSION                  "0.0.16"

#endif

/* EOF */
