/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS kernel
 * FILE:        include/reactos/winlogon.h
 * PURPOSE:     Private interface between CSRSS and WinLogon
 * PROGRAMMER:  Ge van Geldorp (gvg@reactos.com)
 */

#ifndef REACTOS_WINLOGON_H_INCLUDED
#define REACTOS_WINLOGON_H_INCLUDED

#define WINLOGON_DESKTOP   L"Winlogon"
#define WINLOGON_SAS_CLASS L"SAS window class"
#define WINLOGON_SAS_TITLE L"SAS"

#define PM_WINLOGON_EXITWINDOWS WM_APP

#define EWX_INTERNAL_FLAG          0x10000
#define EWX_INTERNAL_KILL_USER_APPS (EWX_INTERNAL_FLAG | 0x100)
#define EWX_INTERNAL_KILL_ALL_APPS  (EWX_INTERNAL_FLAG | 0x200)

#endif /* REACTOS_WINLOGON_H_INCLUDED */

/* EOF */
