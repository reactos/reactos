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

dbuffmgr.h

Abstract:

   
Author:



Notes:

$Revision:   1.2  $

$Modtime:   10 Jul 1996 13:22:06  $

Revision History:

most-recent-revision-date email-name
description

Original version

--*/

#ifndef _DBUFFERMANAGER_
#define _DBUFFERMANAGER_

#include "windows.h"

class DBUFFERMANAGER
{
  public:

    DBUFFERMANAGER();

    ~DBUFFERMANAGER();

    INT
    Initialize(
        );

    INT
    AllocBuffer(
        IN  LPWSABUF   UserBuffer,
        IN  DWORD      UserBufferCount,
        OUT LPWSABUF*  InternalBuffer,
        OUT DWORD*     InternalBufferCount
        );

    VOID
    FreeBuffer(
        IN LPWSABUF  InternalBuffer,
        IN DWORD     InternalBufferCount
        );
    
    INT
    CopyBuffer(
        IN  LPWSABUF   SourceBuffer,
        IN  DWORD      SourceBufferCount,
        IN  LPWSABUF   DestinationBuffer,
        IN  DWORD      DestinationBufferCount,
        OUT LPDWORD    BytesCopied
        );
    
  private:
    
};   // class DBUFFERMANAGER

#endif // _DBUFFERMANAGER_
