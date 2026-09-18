// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    Module Name: MIL

\*=========================================================================*/

#include "precomp.hpp"


/*=========================================================================*\
    CMILRefCountBase - Base refcounting object for MIL
\*=========================================================================*/

CMILRefCountBase::CMILRefCountBase()
{
    m_cRef = 0; // <<=== Ref needed after construction
}

CMILRefCountBase::~CMILRefCountBase()
{
}

ULONG 
CMILRefCountBase::AddRef()
{
    return InterlockedIncrementULONG(&m_cRef);
}

ULONG 
CMILRefCountBase::Release()
{
    AssertConstMsgW(
        m_cRef != 0,
        L"Attempt to release an object with 0 references! Possible memory leak."
        );

    ULONG cRef = InterlockedDecrementULONG(&m_cRef);

    if(0 == cRef)
    {
        delete this;
    }

    return cRef;
}



