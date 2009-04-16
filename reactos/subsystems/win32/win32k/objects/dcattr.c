/* 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for creation and destruction of DCs
 * FILE:              subsystem/win32/win32k/objects/dcattr.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@rectos.org)
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>


VOID
FASTCALL
DC_AllocateDcAttr(HDC hDC)
{
  PVOID NewMem = NULL;
  PDC pDC;
  HANDLE Pid = NtCurrentProcess();
  ULONG MemSize = sizeof(DC_ATTR); //PAGE_SIZE it will allocate that size

  NTSTATUS Status = ZwAllocateVirtualMemory(Pid,
                                        &NewMem,
                                              0,
                                       &MemSize,
                         MEM_COMMIT|MEM_RESERVE,
                                 PAGE_READWRITE);
  KeEnterCriticalRegion();
  {
    INT Index = GDI_HANDLE_GET_INDEX((HGDIOBJ)hDC);
    PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
    // FIXME: dc could have been deleted!!! use GDIOBJ_InsertUserData
    if (NT_SUCCESS(Status))
    {
      RtlZeroMemory(NewMem, MemSize);
      Entry->UserData  = NewMem;
      DPRINT("DC_ATTR allocated! 0x%x\n",NewMem);
    }
    else
    {
       DPRINT("DC_ATTR not allocated!\n");
    }
  }
  KeLeaveCriticalRegion();
  pDC = DC_LockDc(hDC);
  ASSERT(pDC->pdcattr == &pDC->dcattr);
  if(NewMem)
  {
     pDC->pdcattr = NewMem; // Store pointer
  }
  DC_UnlockDc(pDC);
}

VOID
FASTCALL
DC_FreeDcAttr(HDC  DCToFree )
{
  HANDLE Pid = NtCurrentProcess();
  PDC pDC = DC_LockDc(DCToFree);
  if (pDC->pdcattr == &pDC->dcattr) return; // Internal DC object!
  pDC->pdcattr = &pDC->dcattr;
  DC_UnlockDc(pDC);

  KeEnterCriticalRegion();
  {
    INT Index = GDI_HANDLE_GET_INDEX((HGDIOBJ)DCToFree);
    PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
    if(Entry->UserData)
    {
      ULONG MemSize = sizeof(DC_ATTR); //PAGE_SIZE;
      NTSTATUS Status = ZwFreeVirtualMemory(Pid,
                               &Entry->UserData,
                                       &MemSize,
                                   MEM_RELEASE);
      if (NT_SUCCESS(Status))
      {
        DPRINT("DC_FreeDC DC_ATTR 0x%x\n", Entry->UserData);
        Entry->UserData = NULL;
      }
    }
  }
  KeLeaveCriticalRegion();
}


static
VOID
CopytoUserDcAttr(PDC dc, PDC_ATTR pdcattr)
{
  NTSTATUS Status = STATUS_SUCCESS;
  dc->dcattr.mxWorldToDevice = dc->dclevel.mxWorldToDevice;
  dc->dcattr.mxDeviceToWorld = dc->dclevel.mxDeviceToWorld;
  dc->dcattr.mxWorldToPage = dc->dclevel.mxWorldToPage;

  _SEH2_TRY
  {
      ProbeForWrite( pdcattr,
             sizeof(DC_ATTR),
                           1);
      RtlCopyMemory( pdcattr,
                &dc->dcattr,
             sizeof(DC_ATTR));
  }
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
  {
     Status = _SEH2_GetExceptionCode();
     ASSERT(FALSE);
  }
  _SEH2_END;
}

// FIXME: wtf? 2 functions, where one has a typo in the name????
BOOL
FASTCALL
DCU_SyncDcAttrtoUser(PDC dc)
{
  PDC_ATTR pdcattr = dc->pdcattr;

  if (pdcattr == &dc->dcattr) return TRUE; // No need to copy self.
  ASSERT(pdcattr);
  CopytoUserDcAttr( dc, pdcattr);
  return TRUE;
}

BOOL
FASTCALL
DCU_SynchDcAttrtoUser(HDC hDC)
{
  BOOL Ret;
  PDC pDC = DC_LockDc ( hDC );
  if (!pDC) return FALSE;
  Ret = DCU_SyncDcAttrtoUser(pDC);
  DC_UnlockDc( pDC );
  return Ret;
}

