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

#define PM_WINLOGON_EXITWINDOWS WM_APP

#define EWX_INTERNAL_FLAG           0x10000
#define EWX_INTERNAL_KILL_USER_APPS (EWX_INTERNAL_FLAG | 0x100)
#define EWX_INTERNAL_KILL_ALL_APPS  (EWX_INTERNAL_FLAG | 0x200)
#define EWX_INTERNAL_FLAG_LOGOFF    0x1000

#endif /* REACTOS_WINLOGON_H_INCLUDED */

/* EOF */
