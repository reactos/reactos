// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    File: ResourcePool.cpp

    Module Name: MIL

\*=========================================================================*/

#include "precomp.hpp"

/*=========================================================================*\
    CMILPoolResource - Base object with a manager controlled lifetime
\*=========================================================================*/

CMILPoolResource::CMILPoolResource(
    __in_ecount_opt(1) IMILPoolManager *pManager
    )
{
    m_pManager = pManager;
}

CMILPoolResource::~CMILPoolResource()
{
    Assert(m_cRef == 0);
}

/**************************************************************************\
*
* Function Description:
*
*   Release for CMILPoolResource objects.
*
*
\**************************************************************************/
STDMETHODIMP_(ULONG)
CMILPoolResource::Release()
{
    ULONG cRef = InterlockedDecrementULONG(&m_cRef);

    if (0 == cRef)
    {
        // The manager is responsible for creating this object and
        //  retaining a reference to it, but doesn't manage its lifetime
        //  through a reference count.  So just let the manager know this
        //  object is no longer referenced externally.  (The only way
        //  it could again be referenced externally would be for the
        //  manager to hand out a reference.)

        // The manager does not retain a reference because that would
        //  just mean this Release would also have to check
        //  for 1 and then notify the manager.  That just stands to
        //  confound any users who check the ref count, without 
        //  providing real value.

        if (m_pManager)
        {
            m_pManager->UnusedNotification(this);
        }
        else
        {
            TraceTag((tagWarning, "CMILPoolResource was NOT being managed."));
            delete this;
        }
    }

    return cRef;
}

ULONG CMILPoolResource::GetRefCount() const
{
    return m_cRef;
}




