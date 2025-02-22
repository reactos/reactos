/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for creation and destruction of DCs
 * FILE:              win32ss/gdi/ntgdi/dcattr.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@rectos.org)
 */

#include <win32k.h>

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


BOOL
NTAPI
DC_bAllocDcAttr(PDC pdc)
{
    PPROCESSINFO ppi;
    PDC_ATTR pdcattr;

    ppi = PsGetCurrentProcessWin32Process();
    ASSERT(ppi);

    pdcattr = GdiPoolAllocate(ppi->pPoolDcAttr);
    if (!pdcattr)
    {
        DPRINT1("Could not allocate DC attr\n");
        return FALSE;
    }

    /* Copy the content from the kernel mode dc attr */
    pdc->pdcattr = pdcattr;
    *pdc->pdcattr = pdc->dcattr;

    /* Set the object attribute in the handle table */
    GDIOBJ_vSetObjectAttr(&pdc->BaseObject, pdcattr);

    DPRINT("DC_AllocDcAttr: pdc=%p, pdc->pdcattr=%p\n", pdc, pdc->pdcattr);
    return TRUE;
}

VOID
NTAPI
DC_vFreeDcAttr(PDC pdc)
{
    PPROCESSINFO ppi;

    if (pdc->pdcattr == &pdc->dcattr)
    {
        // Internal DC object!
        return;
    }

    /* Reset the object attribute in the handle table */
    GDIOBJ_vSetObjectAttr(&pdc->BaseObject, NULL);

    ppi = PsGetCurrentProcessWin32Process();
    ASSERT(ppi);
    GdiPoolFree(ppi->pPoolDcAttr, pdc->pdcattr);

    /* Reset to kmode dcattr */
    pdc->pdcattr = &pdc->dcattr;
}
