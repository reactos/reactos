/*++

  Copyright (c) Microsoft Corporation

  Module Name:

  FxRequestBaseKm.hpp

  Abstract:

  This module implements km specific functions for FxRequestBase.

  Author:



  Environment:

  Kernel mode only

  Revision History:

  --*/

#ifndef _FXREQUESTBASEKM_HPP_
#define _FXREQUESTBASEKM_HPP_


VOID
__inline
FxRequestBase::FreeMdls(
    VOID
    )
{
    PMDL pMdl, pNext;

    if (IsAllocatedFromIo() || IsCanComplete()) {
      return;
    }

    pMdl = m_Irp.GetIrp()->MdlAddress;

    //
    // Free any PMDLs that the lower layer allocated.  Since we are going
    // to free the PIRP ourself and not call IoCompleteRequest, we must mimic
    // the behavior in IoCompleteRequest which does the same thing.
    //
    while (pMdl != NULL) {
      pNext = pMdl->Next;

      if (pMdl->MdlFlags & MDL_PAGES_LOCKED) {
        MmUnlockPages(pMdl);
      }
      else if (GetDriverGlobals()->FxVerifierOn) {
        DbgPrint("pMdl %p, Flags 0x%x in PIRP %p should be locked",
            pMdl, pMdl->MdlFlags, m_Irp.GetIrp());

        FxVerifierDbgBreakPoint(GetDriverGlobals());
      }

      FxIrpMdlFree(pMdl);
      pMdl = pNext;
    }

    m_Irp.GetIrp()->MdlAddress = NULL;
}

#endif // _FXREQUESTBASEKM_HPP
