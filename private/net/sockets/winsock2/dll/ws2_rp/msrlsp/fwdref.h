//---------------------------------------------------------------------------
//  Copyright (c) 1997 Microsoft Corporation
//
//  fwdref.h
//
//  Forward reference definitions for classes used by the Restricted
//  Process TCP/IP Layered Service Provider.
//
//  Revision History:
//
//  edwardr    11-05-97    Initial version.
//
//---------------------------------------------------------------------------
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

--*/

#ifndef _FWDREF_
#define _FWDREF_

#include <windows.h>

class RSOCKET;
typedef RSOCKET FAR * PRSOCKET;

class RPROVIDER;
typedef RPROVIDER FAR * PRPROVIDER;

class PROTO_CATALOG_ITEM;
typedef PROTO_CATALOG_ITEM  FAR * PPROTO_CATALOG_ITEM;

class DCATALOG;
typedef DCATALOG FAR * PDCATALOG;

class RWORKERTHREAD;
typedef RWORKERTHREAD FAR * PRWORKERTHREAD;

class DOVERLAPPEDSTRUCTMGR;
typedef DOVERLAPPEDSTRUCTMGR FAR * PDOVERLAPPEDSTRUCTMGR;

class DBUFFERMANAGER;
typedef DBUFFERMANAGER FAR * PDBUFFERMANAGER;

#endif  // _FWDREF_
