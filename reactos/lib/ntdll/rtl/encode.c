/* $Id: encode.c,v 1.1 2003/04/02 00:01:21 hyperion Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security descriptor functions
 * FILE:              lib/ntdll/rtl/encode.c
 * PROGRAMMER:        KJK::Hyperion <noog@libero.it>
 * REVISION HISTORY:
 *                 02/04/2003: created (code contributed by crazylord
 *                             <crazyl0rd@minithins.net>)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <ntdll/ntdll.h>

/* FUNCTIONS ***************************************************************/

VOID NTAPI RtlRunDecodeUnicodeString
(
 IN UCHAR hash,
 IN OUT PUNICODE_STRING uString
)
{
   UCHAR *ptr;
   WORD   i;
   
   ptr = (UCHAR *) uString->Buffer;
   if (uString->Length > 1) {
      for (i=uString->Length; i>1; i--) {
         ptr[i-1] ^= ptr[i-2] ^ hash;
      }
   }
   
   if (uString->Length >= 1) {
      ptr[0] ^= hash | (UCHAR) 0x43;
   }
}

VOID NTAPI RtlRunEncodeUnicodeString
(
 IN OUT PUCHAR hash,
 IN OUT PUNICODE_STRING uString
)
{
   NTSTATUS ntS;
   UCHAR *ptr;
   TIME  CurrentTime;
   WORD   i;

   ptr = (UCHAR *) uString->Buffer;   
   if (*hash == 0) {
      ntS = NtQuerySystemTime(&CurrentTime);
      if (NT_SUCCESS(ntS)) {
         for (i=1; i<sizeof(TIME) && (*hash == 0); i++)
            *hash |= *(PUCHAR) (((PUCHAR) &CurrentTime)+i);
      }
      
      if (*hash == 0)
         *hash = 1;
   }
 
   if (uString->Length >= 1) {
      ptr[0] ^= (*hash) | (UCHAR) 0x43;
      if (uString->Length > 1) {
         for (i=1; i<uString->Length; i++) {
            ptr[i] ^= ptr[i-1] ^ (*hash);
         }
      }
   }
}

/* EOF */
