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

    globals.h

Abstract:

    This module contains the external definitions of the global objects
    contained in the layered service provider sample

Author:

    bugs@brandy.jf.intel.com

Revision History:

--*/
#ifndef _GLOBALS_H_
#define _GLOBALS_H_

//
// Global variables from SPI.CPP
//

// The WinSock2 UpCallTable.
extern WSPUPCALLTABLE g_UpCallTable;

// Variables to track Startup/Cleanup Pairing.
extern CRITICAL_SECTION  g_InitCriticalSection;
extern DWORD             g_StartupCount;

// Variables to keep track of the sockets we have open
extern LIST_ENTRY        g_SocketList;
extern CRITICAL_SECTION  g_SocketListLock;

// The catalog of providers
extern PDCATALOG gProviderCatalog;

// The worker thread for async and overlapped functions
extern PRWORKERTHREAD gWorkerThread;

// The buffer manager for providers that modify the data stream
extern PDBUFFERMANAGER gBufferManager;


#endif // _GLOBALS_H_


