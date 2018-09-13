/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifdef UNIX
#include "core.hxx"

EXTERN_C void Runtime_init();
EXTERN_C HANDLE g_hProcessHeap;
CRITICAL_SECTION g_csHeap;
EXTERN_C BOOL Memory_init();
EXTERN_C void Runtime_exit();
EXTERN_C void Memory_exit();

// This is a Unix Hack. 
#include "teb.hxx"
extern struct TEB g_teb;

// For debugging
#define ASSERT(x)  if(!(x)) DebugBreak()

class InitUnix 
{
public:

    InitUnix()
    {
        //        ASSERT(0); // For debugging
	// BUGBUG - This needs to be fixed - we need to fail loading the 
        // DLL if this fails. Need to talk to jayf about this, since it is 
        // during program startup.
        if (!Memory_init())
            ASSERT(0);
        Runtime_init();
        
    }

    ~InitUnix()
    {
        Runtime_exit();
        Memory_exit();
    }

};


static InitUnix  initUnix;
#endif // UNIX
