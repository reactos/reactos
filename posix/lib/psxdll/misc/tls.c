/* $Id: tls.c,v 1.1 2002/05/17 02:07:14 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/misc/tls.c
 * PURPOSE:     Thread local storage
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              30/04/2002: Created
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <psx/tls.h>
#include <psx/errno.h>

__tls_index_t __tls_alloc()
{
 ULONG nIndex;

 RtlAcquirePebLock();

 nIndex = RtlFindClearBitsAndSet(NtCurrentPeb()->TlsBitmap, 1, 0);

 if (nIndex == (ULONG)-1)
  errno = ENOMEM;
 else
  NtCurrentTeb()->TlsSlots[nIndex] = 0;

 RtlReleasePebLock();

 return(nIndex);

}

int __tls_free(__tls_index_t index)
{
 if (index >= TLS_MINIMUM_AVAILABLE)
 {
	 errno = ERANGE;
	 return (-1);
 }

 RtlAcquirePebLock();

 if(RtlAreBitsSet(NtCurrentPeb()->TlsBitmap, index, 1))
 {
	 NtSetInformationThread
  (
   NtCurrentThread(),
			ThreadZeroTlsCell,
			&index,
			sizeof(DWORD)
  );

	 RtlClearBits(NtCurrentPeb()->TlsBitmap, index, 1);
 }

 RtlReleasePebLock();

 return (0);
}

void * __tls_get_val(__tls_index_t index)
{
 if(index >= TLS_MINIMUM_AVAILABLE)
 {
	 errno = ERANGE;
	 return (0);
 }

 return (NtCurrentTeb()->TlsSlots[index]);
}

int __tls_put_val(__tls_index_t index, void * data)
{
 if(index >= TLS_MINIMUM_AVAILABLE)
 {
 	errno = ERANGE;
  return (-1);
 }

 NtCurrentTeb()->TlsSlots[index] = data;
 return (0);
}

/* EOF */

