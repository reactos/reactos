/* $Id: encode.c,v 1.1 2004/06/20 23:27:21 gdalsnes Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security descriptor functions
 * FILE:              lib/rtl/encode.c
 * PROGRAMMER:        KJK::Hyperion <noog@libero.it>
 * REVISION HISTORY:
 *                 02/04/2003: created (code contributed by crazylord
 *                             <crazyl0rd@minithins.net>)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <ntdll/ntdll.h>

/* FUNCTIONS ***************************************************************/

VOID STDCALL
RtlRunDecodeUnicodeString (IN UCHAR Hash,
                           IN OUT PUNICODE_STRING String)
{
   PUCHAR ptr;
   USHORT i;

   ptr = (PUCHAR)String->Buffer;
   if (String->Length > 1)
   {
      for (i = String->Length; i > 1; i--)
      {
         ptr[i - 1] ^= ptr[i - 2] ^ Hash;
      }
   }

   if (String->Length >= 1)
   {
      ptr[0] ^= Hash | (UCHAR)0x43;
   }
}


VOID STDCALL
RtlRunEncodeUnicodeString (IN OUT PUCHAR Hash,
                           IN OUT PUNICODE_STRING String)
{
   LARGE_INTEGER CurrentTime;
   PUCHAR ptr;
   USHORT i;
   NTSTATUS Status;

   ptr = (PUCHAR) String->Buffer;
   if (*Hash == 0)
   {
      Status = NtQuerySystemTime (&CurrentTime);
      if (NT_SUCCESS(Status))
      {
         for (i = 1; i < sizeof(LARGE_INTEGER) && (*Hash == 0); i++)
            *Hash |= *(PUCHAR)(((PUCHAR)&CurrentTime) + i);
      }

      if (*Hash == 0)
         *Hash = 1;
   }

   if (String->Length >= 1)
   {
      ptr[0] ^= (*Hash) | (UCHAR)0x43;
      if (String->Length > 1)
      {
         for (i = 1; i < String->Length; i++)
         {
            ptr[i] ^= ptr[i - 1] ^ (*Hash);
         }
      }
   }
}

/* EOF */
