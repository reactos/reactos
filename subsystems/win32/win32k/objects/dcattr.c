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

#define GDIDCATTRFREE 8

typedef struct _GDI_DC_ATTR_FREELIST
{
  LIST_ENTRY Entry;
  DWORD nEntries;
  PVOID AttrList[GDIDCATTRFREE];
} GDI_DC_ATTR_FREELIST, *PGDI_DC_ATTR_FREELIST;

typedef struct _GDI_DC_ATTR_ENTRY
{
  DC_ATTR Attr[GDIDCATTRFREE];
} GDI_DC_ATTR_ENTRY, *PGDI_DC_ATTR_ENTRY;


PDC_ATTR
FASTCALL
AllocateDcAttr(VOID)
{
  PTHREADINFO pti;
  PPROCESSINFO ppi;
  PDC_ATTR pDc_Attr;
  PGDI_DC_ATTR_FREELIST pGdiDcAttrFreeList;
  PGDI_DC_ATTR_ENTRY pGdiDcAttrEntry;
  int i;
  
  pti = PsGetCurrentThreadWin32Thread();
  if (pti->pgdiDcattr)
  {
     pDc_Attr = pti->pgdiDcattr; // Get the free one.
     pti->pgdiDcattr = NULL;
     return pDc_Attr;
  }

  ppi = PsGetCurrentProcessWin32Process();

  if (!ppi->pDCAttrList) // If set point is null, allocate new group.
  {
     pGdiDcAttrEntry = EngAllocUserMem(sizeof(GDI_DC_ATTR_ENTRY), 0);

     if (!pGdiDcAttrEntry)
     {
        DPRINT1("DcAttr Failed User Allocation!\n");
        return NULL;
     }

     DPRINT("AllocDcAttr User 0x%x\n",pGdiDcAttrEntry);

     pGdiDcAttrFreeList = ExAllocatePoolWithTag( PagedPool,
                                                 sizeof(GDI_DC_ATTR_FREELIST),
                                                 GDITAG_DC_FREELIST);
     if ( !pGdiDcAttrFreeList )
     {
        EngFreeUserMem(pGdiDcAttrEntry);
        return NULL;
     }

     RtlZeroMemory(pGdiDcAttrFreeList, sizeof(GDI_DC_ATTR_FREELIST));

     DPRINT("AllocDcAttr Ex 0x%x\n",pGdiDcAttrFreeList);

     InsertHeadList( &ppi->GDIDcAttrFreeList, &pGdiDcAttrFreeList->Entry);

     pGdiDcAttrFreeList->nEntries = GDIDCATTRFREE;
     // Start at the bottom up and set end of free list point.
     ppi->pDCAttrList = &pGdiDcAttrEntry->Attr[GDIDCATTRFREE-1];
     // Build the free attr list.
     for ( i = 0; i < GDIDCATTRFREE; i++)
     {
         pGdiDcAttrFreeList->AttrList[i] = &pGdiDcAttrEntry->Attr[i];
     }
  }

  pDc_Attr = ppi->pDCAttrList;
  pGdiDcAttrFreeList = (PGDI_DC_ATTR_FREELIST)ppi->GDIDcAttrFreeList.Flink;

  // Free the list when it is full!
  if ( pGdiDcAttrFreeList->nEntries-- == 1)
  {  // No more free entries, so yank the list.
     RemoveEntryList( &pGdiDcAttrFreeList->Entry );

     ExFreePoolWithTag( pGdiDcAttrFreeList, GDITAG_DC_FREELIST );

     if ( IsListEmpty( &ppi->GDIDcAttrFreeList ) )
     {
        ppi->pDCAttrList = NULL;
        return pDc_Attr;
     }

     pGdiDcAttrFreeList = (PGDI_DC_ATTR_FREELIST)ppi->GDIDcAttrFreeList.Flink;
  }

  ppi->pDCAttrList = pGdiDcAttrFreeList->AttrList[pGdiDcAttrFreeList->nEntries-1];

  return pDc_Attr;
}

VOID
FASTCALL
FreeDcAttr(PDC_ATTR pDc_Attr)
{
  PTHREADINFO pti;
  PPROCESSINFO ppi;
  PGDI_DC_ATTR_FREELIST pGdiDcAttrFreeList;

  pti = PsGetCurrentThreadWin32Thread();
  
  if (!pti) return;
  
  if (!pti->pgdiDcattr)
  {  // If it is null, just cache it for the next time.
     pti->pgdiDcattr = pDc_Attr;
     return;
  }

  ppi = PsGetCurrentProcessWin32Process();

  pGdiDcAttrFreeList = (PGDI_DC_ATTR_FREELIST)ppi->GDIDcAttrFreeList.Flink;

  // We add to the list of free entries, so this will grows!
  if ( IsListEmpty(&ppi->GDIDcAttrFreeList) ||
       pGdiDcAttrFreeList->nEntries == GDIDCATTRFREE )
  {
     pGdiDcAttrFreeList = ExAllocatePoolWithTag( PagedPool,
                                                 sizeof(GDI_DC_ATTR_FREELIST),
                                                 GDITAG_DC_FREELIST);
     if ( !pGdiDcAttrFreeList )
     {
        return;
     }
     InsertHeadList( &ppi->GDIDcAttrFreeList, &pGdiDcAttrFreeList->Entry);
     pGdiDcAttrFreeList->nEntries = 0;
  }
  // Up count, save the entry and set end of free list point.
  ++pGdiDcAttrFreeList->nEntries; // Top Down...
  pGdiDcAttrFreeList->AttrList[pGdiDcAttrFreeList->nEntries-1] = pDc_Attr;
  ppi->pDCAttrList = pDc_Attr;

  return;
}

VOID
FASTCALL
DC_AllocateDcAttr(HDC hDC)
{
  PVOID NewMem = NULL;
  PDC pDC;

  {
    INT Index = GDI_HANDLE_GET_INDEX((HGDIOBJ)hDC);
    PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];

    NewMem = AllocateDcAttr();

    // FIXME: dc could have been deleted!!! use GDIOBJ_InsertUserData

    if (NewMem)
    {
       RtlZeroMemory(NewMem, sizeof(DC_ATTR));
       Entry->UserData = NewMem;
       DPRINT("DC_ATTR allocated! 0x%x\n",NewMem);
    }
    else
    {
       DPRINT1("DC_ATTR not allocated!\n");
    }
  }
  pDC = DC_LockDc(hDC);
  ASSERT(pDC->pdcattr == &pDC->dcattr);
  if (NewMem)
  {
     pDC->pdcattr = NewMem; // Store pointer
  }
  DC_UnlockDc(pDC);
}

VOID
FASTCALL
DC_FreeDcAttr(HDC  DCToFree )
{
  PDC pDC = DC_LockDc(DCToFree);
  if (pDC->pdcattr == &pDC->dcattr) return; // Internal DC object!
  pDC->pdcattr = &pDC->dcattr;
  DC_UnlockDc(pDC);

  {
    INT Index = GDI_HANDLE_GET_INDEX((HGDIOBJ)DCToFree);
    PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
    if (Entry->UserData)
    {
       FreeDcAttr(Entry->UserData);
       Entry->UserData = NULL;
    }
  }
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
// LOL! DCU_ Sync hDc Attr to User,,, need it speeled out for you?
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

