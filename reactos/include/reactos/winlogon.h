/* $Id: winlogon.h,v 1.1 2004/07/12 20:09:35 gvg Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS kernel
 * FILE:        include/reactos/winlogon.h
 * PURPOSE:     Interface to winlogon
 * PROGRAMMER:  David Welch (welch@mcmail.com)
 * REVISIONS:
 * 	1999-11-06 (ea)
 * 		Moved from include/internal in include/reactos
 *		to be used by buildno.
 *	2002-01-17 (ea)
 *		KERNEL_VERSION removed. Use
 *		reactos/buildno.h:KERNEL_VERSION_STR instead.
 */

#ifndef REACTOS_WINLOGON_H_INCLUDED
#define REACTOS_WINLOGON_H_INCLUDED

#define WINLOGON_DESKTOP   L"Winlogon"
#define WINLOGON_SAS_CLASS L"SAS window class"
#define WINLOGON_SAS_TITLE L"SAS"

#define PM_WINLOGON_EXITWINDOWS WM_APP

#endif /* REACTOS_WINLOGON_H_INCLUDED */

/* EOF */
