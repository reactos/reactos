/* $Id: errno.c,v 1.6 2003/08/22 13:55:15 ea Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/errno/errno.c
 * PURPOSE:     Internal errno implementation
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              27/12/2001: Created
 */

#include <ddk/ntddk.h>
#include <psx/pdata.h>
#include <psx/errno.h>
#include <psx/debug.h>

int * __PdxGetThreadErrNum(void)
{
 return &(((__PPDX_TDATA) (NtCurrentTeb()->TlsSlots[__PdxGetProcessData()->TlsIndex]) )->ErrNum);
}

/**********************************************************************
 * NAME							EXPORTED
 * 	RtlNtStatusToPsxErrno
 *
 * DESCRIPTION
 *	Convert an Executive status ID into a POSIX error number
 *	(errno.h).
 *	
 * NOTE
 * 	Not present in the legacy WNT (a ReactOS extension to support
 * 	the POSIX+ subsystem).
 * 	
 * ARGUMENTS
 *	Status	The Executive status ID to convert.
 *
 * RETURN VALUE
 *	errno as in errno.h
 *	
 * REVISIONS
 * 	1999-11-30 ea
 * 	2003-08-22 ea: moved here from NTDLL
 */
INT STDCALL
RtlNtStatusToPsxErrno(IN NTSTATUS Status)
{
#if 0
  switch (Status)
    {
    }
#endif
  return -1; /* generic POSIX error */
}

/* EOF */

