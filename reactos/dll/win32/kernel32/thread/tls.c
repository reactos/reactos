/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/thread/tls.c
 * PURPOSE:         Thread functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Tls functions are modified from WINE
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
DWORD STDCALL
TlsAlloc(VOID)
{
   ULONG Index;

   RtlAcquirePebLock();

   /* Try to get regular TEB slot. */
   Index = RtlFindClearBitsAndSet(NtCurrentPeb()->TlsBitmap, 1, 0);
   if (Index == ~0)
   {
      /* If it fails, try to find expansion TEB slot. */
      Index = RtlFindClearBitsAndSet(NtCurrentPeb()->TlsExpansionBitmap, 1, 0);
      if (Index != ~0)
      {
         if (NtCurrentTeb()->TlsExpansionSlots == NULL)
         {
            NtCurrentTeb()->TlsExpansionSlots = HeapAlloc(
               GetProcessHeap(), HEAP_ZERO_MEMORY,
               TLS_EXPANSION_SLOTS * sizeof(PVOID));
         }

         if (NtCurrentTeb()->TlsExpansionSlots == NULL)
         {
            RtlClearBits(NtCurrentPeb()->TlsExpansionBitmap, Index, 1);
            Index = ~0;
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
         }
         else
         {
            /* Clear the value. */
            NtCurrentTeb()->TlsExpansionSlots[Index] = 0;
            Index += TLS_MINIMUM_AVAILABLE;
         }
      }
      else
      {
         SetLastError(ERROR_NO_MORE_ITEMS);
      }
   }
   else
   {
      /* Clear the value. */
      NtCurrentTeb()->TlsSlots[Index] = 0;
   }

   RtlReleasePebLock();

   return Index;
}


/*
 * @implemented
 */
BOOL STDCALL
TlsFree(DWORD Index)
{
   BOOL BitSet;
   
   if (Index >= TLS_EXPANSION_SLOTS + TLS_MINIMUM_AVAILABLE)
   {
      SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
      return FALSE;
   }

   RtlAcquirePebLock();

   if (Index >= TLS_MINIMUM_AVAILABLE)
   {
      BitSet = RtlAreBitsSet(NtCurrentPeb()->TlsExpansionBitmap,
                             Index - TLS_MINIMUM_AVAILABLE, 1);
      if (BitSet)
         RtlClearBits(NtCurrentPeb()->TlsExpansionBitmap,
                      Index - TLS_MINIMUM_AVAILABLE, 1);
   }
   else
   {
      BitSet = RtlAreBitsSet(NtCurrentPeb()->TlsBitmap, Index, 1);
      if (BitSet)
         RtlClearBits(NtCurrentPeb()->TlsBitmap, Index, 1);
   }

   if (BitSet)
   {
      /* Clear the TLS cells (slots) in all threads of the current process. */
      NtSetInformationThread(NtCurrentThread(), ThreadZeroTlsCell,
                             &Index, sizeof(DWORD));
   }
   else
   {
      SetLastError(ERROR_INVALID_PARAMETER);
   }

   RtlReleasePebLock();

   return BitSet;
}


/*
 * @implemented
 */
LPVOID STDCALL
TlsGetValue(DWORD Index)
{
   if (Index >= TLS_EXPANSION_SLOTS + TLS_MINIMUM_AVAILABLE)
   {
      SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
      return NULL;
   }

   SetLastError(NO_ERROR);

   if (Index >= TLS_MINIMUM_AVAILABLE)
   {
      /* The expansion slots are allocated on demand, so check for it. */
      if (NtCurrentTeb()->TlsExpansionSlots == NULL)
         return NULL;
      return NtCurrentTeb()->TlsExpansionSlots[Index - TLS_MINIMUM_AVAILABLE];
   }
   else
   {
      return NtCurrentTeb()->TlsSlots[Index];
   }
}


/*
 * @implemented
 */
BOOL STDCALL
TlsSetValue(DWORD Index, LPVOID Value)
{
   if (Index >= TLS_EXPANSION_SLOTS + TLS_MINIMUM_AVAILABLE)
   {
      SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
      return FALSE;
   }

   if (Index >= TLS_MINIMUM_AVAILABLE)
   {
      if (NtCurrentTeb()->TlsExpansionSlots == NULL)
      {
         NtCurrentTeb()->TlsExpansionSlots = HeapAlloc(
            GetProcessHeap(), HEAP_ZERO_MEMORY,
            TLS_EXPANSION_SLOTS * sizeof(PVOID));

         if (NtCurrentTeb()->TlsExpansionSlots == NULL)
         {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
         }
      }

      NtCurrentTeb()->TlsExpansionSlots[Index - TLS_MINIMUM_AVAILABLE] = Value;
   }
   else
   {
      NtCurrentTeb()->TlsSlots[Index] = Value;
   }

   return TRUE;
}

/* EOF */
