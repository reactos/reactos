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

    dcatitem.h

Abstract:

    This  file  contains the class definition for the PROTO_CATALOG_ITEM class.
    This  class  defines the interface to the entries that can be installed and
    retrieved in the protocol catalog.

Author:

    bugs@brandy.jf.intel.com

Notes:

    $Revision:   1.4  $

    $Modtime:   15 Jul 1996 15:41:08  $

Revision History:

    most-recent-revision-date email-name
        description

--*/

#ifndef _DCATITEM_
#define _DCATITEM_

#include <windows.h>
#include "llist.h"
#include "fwdref.h"


class PROTO_CATALOG_ITEM {
public:

    PROTO_CATALOG_ITEM();

    INT
    Initialize(
        LPWSAPROTOCOL_INFOW  ProtoInfo
        );


    ~PROTO_CATALOG_ITEM();

    LPWSAPROTOCOL_INFOW
    GetProtocolInfo();


    PWCHAR
    GetLibraryPath();

    VOID
    SetProvider(
        IN  PRPROVIDER  Provider
        );

    PRPROVIDER
    GetProvider();

    LIST_ENTRY     m_CatalogLinkage;
    // Used  to  link  items  in  catalog.   Note  that  this particular member
    // variable  is in the public section to make it available for manipulation
    // by the catalog object.


private:

    WCHAR m_LibraryPath[MAX_PATH];
    // Fully qualified path to the provider's DLL image.

    WSAPROTOCOL_INFOW  m_ProtoInfo;
    // The cataloged WSAPROTOCOL_INFOA structure.  This is typically used for
    // comparison  when  selecting  a  provider by address family, socket
    // type, etc.

    PRPROVIDER  m_Provider;
    // Pointer to the RPROVIDER object attached to this catalog entry.

};  // class PROTO_CATALOG_ITEM


#endif // _DCATITEM_
