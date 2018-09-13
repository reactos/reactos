/*
 * @(#)CSLock.hxx 1.0 3/12/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _CSLOCK_HXX
#define _CSLOCK_HXX

//==============================================================================
class CSLock
{
public:
    CSLock(LPCRITICAL_SECTION p) 
    { 
        EnterCriticalSection(p);
        pcs = p; 
    }

    ~CSLock()
    {
        LeaveCriticalSection(pcs);
    }

private:
    LPCRITICAL_SECTION pcs;
};

#define CRITICALSECTIONLOCK CSLock lock(&_cs);

#endif