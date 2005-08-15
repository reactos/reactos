/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/rtl.c
 * PURPOSE:         Loader utilities
 *
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL
LdrGetProcedureAddress (IN PVOID BaseAddress,
                        IN PANSI_STRING Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   ULONG ExportDirSize = 0;
   PUSHORT OrdinalPtr;
   PULONG NamePtr;
   PCHAR CurrentNamePtr;
   PULONG AddressPtr;

   if (ProcedureAddress == NULL)
      return STATUS_INVALID_PARAMETER;

   /* get the pointer to the export directory */
   ExportDir = RtlImageDirectoryEntryToData(BaseAddress, TRUE,
				            IMAGE_DIRECTORY_ENTRY_EXPORT,
				            &ExportDirSize);

   if (ExportDir == NULL || ExportDirSize == 0)
      return STATUS_INVALID_PARAMETER;

   AddressPtr = (PULONG)RVA(BaseAddress, ExportDir->AddressOfFunctions);

   if (Name && Name->Length)
   {
      LONG minn, maxn, mid, res;

      /* Search for export by name */

      /*
       * NOTE: Exports are always sorted and so we can apply binary search.
       * Also the function names are _case sensitive_, so respect that.
       * -- Filip Navara, August 1st, 2005
       */

      OrdinalPtr = (PUSHORT)RVA(BaseAddress, ExportDir->AddressOfNameOrdinals);
      NamePtr = (PULONG)RVA(BaseAddress, ExportDir->AddressOfNames);

      minn = 0; maxn = ExportDir->NumberOfNames - 1;
      while (minn <= maxn)
      {
         mid = (minn + maxn) / 2;
         CurrentNamePtr = (PCHAR)RVA(BaseAddress, NamePtr[mid]);
         res = strncmp(CurrentNamePtr, Name->Buffer, Name->Length);
         if (res == 0)
         {
            /*
             * Check if the beginning of the name matched, but it's still
             * not the whole name.
             */
            if (CurrentNamePtr[Name->Length] != 0)
            {
               res = -1;
            }
            else
            {
               *ProcedureAddress = (PVOID)RVA(BaseAddress, AddressPtr[OrdinalPtr[mid]]);
               return STATUS_SUCCESS;
            }
         }
         if (res > 0)
            maxn = mid - 1;
         else
            minn = mid + 1;
      }

      CPRINT("LdrGetProcedureAddress: Can't resolve symbol '%Z'\n", Name);
   }
   else
   {
      /* Search for export by ordinal */

      Ordinal &= 0x0000FFFF;
      if (Ordinal - ExportDir->Base < ExportDir->NumberOfFunctions)
      {
         *ProcedureAddress = (PVOID)RVA(BaseAddress, AddressPtr[Ordinal - ExportDir->Base]);
         return STATUS_SUCCESS;
      }

      CPRINT("LdrGetProcedureAddress: Can't resolve symbol @%d\n", Ordinal);
   }

   return STATUS_PROCEDURE_NOT_FOUND;
}

/* EOF */
