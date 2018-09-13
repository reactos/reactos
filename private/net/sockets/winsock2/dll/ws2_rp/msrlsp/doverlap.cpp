/*++

     Copyright (c) 1996 Intel Corporation
     Copyright (c) 1996 Microsoft Corporation
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

    doverlap.cpp

Abstract:

    This module defines the layered class dprovider along with its methods.

Author:



Revision History:

--*/

#include "precomp.h"
#include "globals.h"

#define STRUCTSIGNATURE 0xbeadface

DOVERLAPPEDSTRUCTMGR::DOVERLAPPEDSTRUCTMGR()
/*++

Routine Description:

    DOVERLAPPEDSTRUCTMGR  object  constructor.   Creates and returns a
    DOVERLAPPEDSTRUCTMGR object.  Note that  the  DOVERLAPPEDSTRUCTMGR object
    has not been fully initialized.  The "Initialize" member function must be
    the first member function called on the new DOVERLAPPEDSTRUCTMGR object. 

Arguments:

    None

Return Value:

    None
--*/
{
    // Setup the free list
    m_overlapped_free_list.Next = NULL;
    m_context_free_list.Next = NULL;
    m_overlapped_struct_block = NULL;
    InitializeCriticalSection(&m_overlapped_free_list_lock);
    InitializeCriticalSection(&m_context_free_list_lock);
}


INT
DOVERLAPPEDSTRUCTMGR::Initialize(
        )
/*++

Routine Description:

    The initialization routine for a DOVERLAPPEDSTRUCTMGR  object.  This
    procedure completes the initialzation of the object.  This procedure
    preforms initialization operations that may fail and must be reported since
    there is no way to fail the constructor. 
    
Arguments:

    None
Return Value:

    The  function returns NO_ERROR if successful.  Otherwise it
    returns an appropriate WinSock error code if the initialization
    cannot be completed.
--*/
{
    INT   ReturnCode;
    ULONG BlockSize;
    PBYTE CurrentBlock;
    ULONG BytesAvailable;
    ULONG StructSize;
    
    
    ReturnCode = WSAENOBUFS;

    //
    // Initialize the pool of internal overlapped structs.
    //
    
    // Get memory for our overlapped structs
    BlockSize = sizeof(INTERNALOVERLAPPEDSTRUCT) *
        OUTSTANDINGOVERLAPPEDSTRUCTS;
    m_overlapped_struct_block = (PBYTE) new BYTE[BlockSize];
    
    if (m_overlapped_struct_block){

        // Dice up the memory block into internal overlapped structs and add
        // them to the free list.
        EnterCriticalSection(&m_overlapped_free_list_lock);

        StructSize = sizeof(INTERNALOVERLAPPEDSTRUCT);
        
        BytesAvailable = BlockSize;
        CurrentBlock   = m_overlapped_struct_block;

        while (BytesAvailable > StructSize){
            PushEntryList(&m_overlapped_free_list,
                          (PSINGLE_LIST_ENTRY) CurrentBlock);

            BytesAvailable -= StructSize;
            CurrentBlock += StructSize;
        } //while
        ReturnCode = NO_ERROR;
        LeaveCriticalSection(&m_overlapped_free_list_lock);
    } //if

    //
    // Initialize the pool of APC context structs.
    //
    
    if (NO_ERROR == ReturnCode){

        ReturnCode = WSAENOBUFS;

        BlockSize =  sizeof(APCCONTEXT)* OUTSTANDINGOVERLAPPEDSTRUCTS;
        
        m_context_struct_block = (PBYTE) new BYTE[BlockSize];
        if (m_context_struct_block){

            // Dice up the memory block into APC context structs and add
            // them to the free list.
        
            EnterCriticalSection(&m_context_free_list_lock);

            StructSize = sizeof(APCCONTEXT);
        
            BytesAvailable = BlockSize;
            CurrentBlock   = m_context_struct_block;

            while (BytesAvailable > StructSize){
                PushEntryList(&m_context_free_list,
                              (PSINGLE_LIST_ENTRY) CurrentBlock);

                BytesAvailable -= StructSize;
                CurrentBlock += StructSize;
            } //while
            ReturnCode = NO_ERROR;
            LeaveCriticalSection(&m_context_free_list_lock);
        } //if
    } //if

    if (NO_ERROR != ReturnCode){
        delete(m_overlapped_struct_block);
    } //if
    return(ReturnCode);
}


DOVERLAPPEDSTRUCTMGR::~DOVERLAPPEDSTRUCTMGR()
/*++

Routine Description:

    DOVERLAPPEDSTRUCTMGR object destructor.  This procedure has the
    responsibility to perform any required shutdown operations for the
    DOVERLAPPEDSTRUCTMGR object before the object memory is deallocated. 
    
Arguments:

    None

Return Value:

    None
--*/
{
    // Clean up the overlapped struct pool
    EnterCriticalSection(&m_overlapped_free_list_lock);
    delete(m_overlapped_struct_block);
    m_overlapped_free_list.Next = NULL;
    LeaveCriticalSection(&m_overlapped_free_list_lock);
    DeleteCriticalSection(&m_overlapped_free_list_lock);

    // Clean up the APC context pool
    EnterCriticalSection(&m_context_free_list_lock);
    delete(m_context_struct_block);
    m_context_free_list.Next = NULL;
    LeaveCriticalSection(&m_context_free_list_lock);
    DeleteCriticalSection(&m_context_free_list_lock);
}


PINTERNALOVERLAPPEDSTRUCT
DOVERLAPPEDSTRUCTMGR::AllocateOverlappedStruct(
    )
/*++

Routine Description:

    Allocates an interanl overlapped structure from the pool of available
    structures.     
Arguments:

    None

Return Value:

    A pointer the a internal overlapped struct on success else NULL.
    
--*/
{
    PINTERNALOVERLAPPEDSTRUCT ReturnValue;

    ReturnValue = PopOverlappedStruct();

    if (ReturnValue){
        ReturnValue->Signature = STRUCTSIGNATURE;
    } //if
    
    return(ReturnValue);
}


VOID
DOVERLAPPEDSTRUCTMGR::FreeOverlappedStruct(
    LPWSAOVERLAPPED   OverlappedStruct
    )
/*++

Routine Description:

    Frees an interanl overlapped structure to the pool of available
    structures.     
Arguments:

    OverlappedStruct - A pointer to the InternalOverlappedStruct member of an
    interanl overlapped struct.

Return Value:

    NONE
    
--*/
{
    PushOverlappedStruct(OverlappedStruct);
}



PINTERNALOVERLAPPEDSTRUCT
DOVERLAPPEDSTRUCTMGR::PopOverlappedStruct()
/*++
  
Routine Description:

    Pops a internal overlapped structure off of the free list and initializes
    the structure.
    
Arguments:

    NONE
Return Value:

    A pointer to a internal overlapped structure on success else NULL
    
--*/
{
    PINTERNALOVERLAPPEDSTRUCT ReturnValue;
    
    EnterCriticalSection(&m_overlapped_free_list_lock);

    ReturnValue =
        (PINTERNALOVERLAPPEDSTRUCT)PopEntryList(&m_overlapped_free_list);

    if (ReturnValue){

        // Init the structure
        ZeroMemory(
            ReturnValue,
            sizeof(INTERNALOVERLAPPEDSTRUCT));
        ReturnValue->Signature = STRUCTSIGNATURE;    
    } //if
    
    LeaveCriticalSection(&m_overlapped_free_list_lock);
    return(ReturnValue);
}


VOID
DOVERLAPPEDSTRUCTMGR::PushOverlappedStruct(
    LPWSAOVERLAPPED OverlappedStruct
    )
/*++
  
Routine Description:

    Pushes an internal overlapped structure onto the free list.
    
Arguments:

    A pointer to the InteranlOverlappedStruct member of an internal overlapped
    struct. 
Return Value:

    NONE    
--*/
{
    PINTERNALOVERLAPPEDSTRUCT InternalOverlappedStruct;
    
    
    EnterCriticalSection(&m_overlapped_free_list_lock);

    InternalOverlappedStruct = GetInternalStuctPointer(
        OverlappedStruct);

    assert(STRUCTSIGNATURE == InternalOverlappedStruct->Signature);
    
    PushEntryList(&m_overlapped_free_list,
                  (PSINGLE_LIST_ENTRY)InternalOverlappedStruct);

    LeaveCriticalSection(&m_overlapped_free_list_lock);
    
}

 
PAPCCONTEXT
DOVERLAPPEDSTRUCTMGR::AllocateApcContext()
/*++

Routine Description:

    Allocates an APC context structure from the pool of available structures.
    
Arguments:

    None

Return Value:

    A pointer the a APC context struct on success else NULL.
    
--*/
{
    PAPCCONTEXT ReturnValue;
    
    EnterCriticalSection(&m_context_free_list_lock);

    ReturnValue = (PAPCCONTEXT)PopEntryList(&m_context_free_list);
    if (ReturnValue){
        ZeroMemory(
            ReturnValue,
            sizeof(APCCONTEXT));
    } //if
    
    LeaveCriticalSection(&m_context_free_list_lock);
    return(ReturnValue);    
}


VOID
DOVERLAPPEDSTRUCTMGR::FreeApcContext(
    PAPCCONTEXT Context
    )
/*++

Routine Description:

    Frees an APC context structure to the pool of available structures.
    
Arguments:

    Context - A pointer to an APC context structure.

Return Value:

    NONE
    
--*/
{
    EnterCriticalSection(&m_overlapped_free_list_lock);

    PushEntryList(&m_overlapped_free_list,
                  (PSINGLE_LIST_ENTRY)Context);
    
    LeaveCriticalSection(&m_overlapped_free_list_lock);
    
}
