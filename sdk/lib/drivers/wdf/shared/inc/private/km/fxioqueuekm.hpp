/*++

  Copyright (c) Microsoft Corporation

  Module Name:

  FxIoQueueKm.hpp

  Abstract:

  This module implements km specific functions for FxIoQueue.

  Author:



  Environment:

  Kernel mode only

  Revision History:

  --*/

#ifndef _FXIOQUEUEKM_HPP_
#define _FXIOQUEUEKM_HPP_

__inline
BOOLEAN
FxIoQueue::IsPagingIo(
    __in MdIrp Irp
    )
/*++

  Routine Description:
  Paging IO is treated especially depending on what Forward Progress policy
  was set on the Queue
  --*/
{
    //
    // NOTE: IRP_INPUT_OPERATION has the same value as IRP_SYNCHRONOUS_PAGING_IO
    // and IRP_MOUNT_COMPLETION the same as  IRP_PAGING_IO  so how does one know if
    // the IO is a paging IO ?
    //

    // One can assume that if IRP_PAGING_IO is set and the MJ code is not
    // FILE_SYSTEM_CONTROL then it is a paging I/O.
    //
    if (Irp->Flags & IRP_PAGING_IO) {
      if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction
    != IRP_MJ_FILE_SYSTEM_CONTROL) {
        return  TRUE;
      }
    }

    return FALSE;
}

#endif // _FXIOQUEUEKM_HPP
