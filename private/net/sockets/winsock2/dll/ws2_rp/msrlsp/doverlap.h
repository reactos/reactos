/*++


     Copyright c 1996 Intel Corporation
     All Rights Reserved

     Permission is granted to use, copy and distribute this software and
     its documentation for any purpose and without fee, provided, that
     the above copyright notice and this statement appear in all copies.
     Intel makes no representations about the suitability of this
     software for any purpose.  This software is provided "AS IS."

     Intel specifically disclaims all warranties, express or implied,
     and all liability, including consequential and other indirect
     damages, for the use of this software, including liability for
     infringement of any proprietary rights, and including the
     warranties of merchantability and fitness for a particular purpose.
     Intel does not assume any responsibility for any errors which may
     appear in this software nor any responsibility to update it.


Module Name:

doverlap.h

Abstract:

    This module defines the DOVERLAPPEDSTRUCTMGR class.  This class
	maintains a pool of internal overlapped structures.  These structures
	are used to store the information the layered service provider will
	need to satisfy users overlapped I/O requests.

Author:

    bugs@brandy.jf.intel.com

Notes:

$Revision:   1.3  $

$Modtime:   11 Jul 1996 09:55:06  $

Revision History:

most-recent-revision-date email-name
description

Original version

--*/

#ifndef _DOVERLAPPEDSTRUCTMGR_
#define _DOVERLAPPEDSTRUCTMGR_

#include <windows.h>
#include "llist.h"
#include "fwdref.h"
#include "stddef.h"

#define  OUTSTANDINGOVERLAPPEDSTRUCTS 100
#define  STRUCTSIGNATURE 0xbeadface

// The internal overlapped struct
typedef struct
{
    DWORD                              Signature;
    DWORD                              OperationType;
    PRPROVIDER                         Provider;
    SOCKET                             Socket;
    LPWSAOVERLAPPED                    UserOverlappedStruct;
    LPWSAOVERLAPPED_COMPLETION_ROUTINE UserCompletionRoutine;
    LPWSATHREADID                      UserThreadId;
    LPWSABUF                           UserBuffer;
    DWORD                              UserBufferCount;
    LPWSABUF                           InternalBuffer;
    DWORD                              InternalBufferCount;
    struct sockaddr FAR *              SockAddr;
    INT                                SockAddrLen;
    LPINT                              SockAddrLenPtr;
    LPDWORD                            FlagsPtr;
    DWORD                              Flags;
    LIST_ENTRY                         ListLinkage;
    WSAOVERLAPPED                      InternalOverlappedStruct;
} INTERNALOVERLAPPEDSTRUCT, *PINTERNALOVERLAPPEDSTRUCT;

// The APC context structure.  This structure is used to store the
// arguments passed to the layered providers completion routine.  The
// address of the structure is passed as the context value in the call to
// WPUQueueApc().
typedef struct
{
    DWORD            Error;
    DWORD            BytesTransferred;
    LPWSAOVERLAPPED  lpOverlapped;
    LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletetionProc;
} APCCONTEXT, *PAPCCONTEXT;

class DOVERLAPPEDSTRUCTMGR
{
  public:

    DOVERLAPPEDSTRUCTMGR();

    INT
    Initialize(
        );

    ~DOVERLAPPEDSTRUCTMGR();

    PINTERNALOVERLAPPEDSTRUCT
    AllocateOverlappedStruct(
        );

    VOID
    FreeOverlappedStruct(
        LPWSAOVERLAPPED   pOverlappedStruct
        );


    PAPCCONTEXT
    AllocateApcContext();

    VOID
    FreeApcContext(
        PAPCCONTEXT Context
        );
    PINTERNALOVERLAPPEDSTRUCT
    GetInternalStuctPointer(
        LPWSAOVERLAPPED OverlappedStruct);

  private:

    PINTERNALOVERLAPPEDSTRUCT
    PopOverlappedStruct();

    VOID
    PushOverlappedStruct(
        LPWSAOVERLAPPED OverlappedStruct
        );

    SINGLE_LIST_ENTRY m_overlapped_free_list;
    // A list of available internal overlapped structs.

    CRITICAL_SECTION  m_overlapped_free_list_lock;
    // Syncronization object for updateing m_overlapped_free_list.

    PBYTE             m_overlapped_struct_block;
    // Pointer to the memory block containing the internal overlapped
    // structures.

    SINGLE_LIST_ENTRY m_context_free_list;
    // A list of available APC context structs.

    CRITICAL_SECTION  m_context_free_list_lock;
    // Syncronization object for updateing m_context_free_list.

    PBYTE             m_context_struct_block;
    // Pointer to the memory block containing the APC context structures.

};   // class DOVERLAPPEDSTRUCTMGR

inline
PINTERNALOVERLAPPEDSTRUCT
DOVERLAPPEDSTRUCTMGR::GetInternalStuctPointer(
    LPWSAOVERLAPPED OverlappedStruct)
/*++

Routine Description:

    Calculates the address of and internal overlapped structure from the
    address of the Overlapped structure passed to the underlaying provider.

Arguments:

    OverlappedStruct - A pointer to an WSAOVERLAPPED structure that is part of
    an internal overlapped structure.

Return Value:
    The address of the internal overlapped structure containing
    OverlappedStruct.
--*/
{
    return( (PINTERNALOVERLAPPEDSTRUCT)
            ((ULONG_PTR) OverlappedStruct - offsetof(INTERNALOVERLAPPEDSTRUCT,
                                                 InternalOverlappedStruct)));
}

#endif // _DOVERLAPPEDSTRUCTMGR_
