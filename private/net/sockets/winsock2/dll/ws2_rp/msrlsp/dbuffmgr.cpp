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

dbuffmgr.cpp

Abstract:

     This module defines and interface for a buffer manager for managing
     WSABUF's.  If a layered provider needs to modify application data The
     CopyBuffer() routine should be modifed to do so.
     
Author:

    bugs@brandy.jf.intel.com

Notes:

$Revision:   1.3  $

$Modtime:   10 Jul 1996 13:22:16  $

Revision History:

--*/

#include "precomp.h"
#include "globals.h"



DBUFFERMANAGER::DBUFFERMANAGER(
    )
/*++

Routine Description:

    DBUFFERMANAGER  object  constructor.   Creates and returns a DBUFFERMANAGER
    object.  Note that  the  DBUFFERMANAGER object has not been fully
    initialized.  The "Initialize" member function must be the first member
    function called on the new DBUFFERMANAGER object.

Arguments:

    None

Return Value:

    None
--*/
{
    // Null function body.  A full implemantation would initialize member
    // varialbles to know values for use in Initailize().
}




INT
DBUFFERMANAGER::Initialize(
        )
/*++

Routine Description:

    The initialization routine for a buffer manager object.  This procedure
    completes the initialzation of the object.  This procedure preforms
    initialization operations that may fail and must be reported since there is
    no way to fail the constructor.
    
Arguments:

    None
Return Value:

    The  function returns NO_ERROR if successful.  Otherwise it
    returns an appropriate WinSock error code if the initialization
    cannot be completed.
--*/
{
    return(NO_ERROR);
}



DBUFFERMANAGER::~DBUFFERMANAGER()
/*++

Routine Description:

    DBUFFERMANAGER object destructor.  This procedure has the responsibility to
    perform any required shutdown operations for the DBUFFERMANAGER object
    before the object memory is deallocated.
    
Arguments:

    None

Return Value:

    None
--*/
{
    // Null function body
}

INT
DBUFFERMANAGER::AllocBuffer(
    IN  LPWSABUF   UserBuffer,
    IN  DWORD      UserBufferCount,
    OUT LPWSABUF*  InternalBuffer,
    OUT DWORD*     InternalBufferCount
    )
/*++

Routine Description:

    This routine allocates a set of WSABUF's to pass to the underlying service
    provider.
    
Arguments:

    UserBuffer - A pointer to the array user WSABUFs
    UserBufferCount - The number of user WSABUF structs.
    InternalBuffer - A pointer to a pointer to a WSABUF
    InternalBufferCount - The pointer to a DWORD to revceive the number of
                          WSABUFs pointed to by InternalBuffer.
    

Return Value:

    The  function returns NO_ERROR if successful.  Otherwise it
    returns an appropriate WinSock error code if the initialization
    cannot be completed.

--*/
{
    // ******
    // Note this procedure returns the user buffer(s) undistrubed.  This is the
    // wrong thing for providers that wish to modify the data stream.  The
    // proper buffer management policy is left to the provider developer.
    *InternalBuffer = UserBuffer;
    *InternalBufferCount = UserBufferCount;
    return(NO_ERROR);
}


VOID
DBUFFERMANAGER::FreeBuffer(
    IN LPWSABUF  InternalBuffer,
    IN DWORD     InternalBufferCount
    )
/*++

Routine Description:

    This routine frees a set of WSABUF's allocated by AllocBuffer();
    
Arguments:

    InternalBuffer - A pointer to WSABUF array
    InternalBufferCount - The number of WSABUFs pointed to by InternalBuffer.
    

Return Value:

    The  function returns NO_ERROR if successful.  Otherwise it
    returns an appropriate WinSock error code if the initialization
    cannot be completed.

--*/
{
}

 
INT
DBUFFERMANAGER::CopyBuffer(
    IN  LPWSABUF   SourceBuffer,
    IN  DWORD      SourceBufferCount,
    IN  LPWSABUF   DestinationBuffer,
    IN  DWORD      DestinationBufferCount,
    OUT LPDWORD    BytesCopied
    )
/*++

Routine Description:

    This routine copies one set of WSABUFs to another.
    
Arguments:

    SourceBuffer - A pointer to an array of WSABUFs.

    SourceBuferCount - The number of WSABUFs pointed to by SourceBuffer.

    DestinationBuffer - A pointer to an array of WSABUFs.

    DestinationBufferCount - The number of WSABUFs pointed to by
                             DestinationBuffer .

Return Value:

    The  function returns NO_ERROR if successful.  Otherwise it
    returns an appropriate WinSock error code if the initialization
    cannot be completed.

--*/
{
    DWORD Index;

    *BytesCopied = 0;
    for (Index = 0; Index < SourceBufferCount; Index++ ){
        *BytesCopied += SourceBuffer[Index].len;
    } //for
    return(NO_ERROR);
}

    
    
