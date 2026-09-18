// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    Module Name: MIL

\*=========================================================================*/

#pragma once

class IMILPoolManager;
class CMILPoolResource;

/*=========================================================================*\
    CMILPoolResource - Base object with a manager controled lifetime
\*=========================================================================*/

class CMILPoolResource : public CMILRefCountBase
{
public:

    CMILPoolResource(__in_ecount_opt(1) IMILPoolManager *pManager);
    virtual ~CMILPoolResource();

    // CMILRefCountBase overrides
    STDMETHOD_(ULONG, Release)(void) override;

    ULONG GetRefCount() const;

protected:

    IMILPoolManager *m_pManager;

};

/*=========================================================================*\

    DEFINE_POOLRESOURCE_REF_COUNT_BASE:

    Include this macro in the public methods list for all classes multiply
    inheriting reference counting interfaces such as CMILPoolResource and
    IUnknown to resolve ambiguity.

\*=========================================================================*/

#define DEFINE_POOLRESOURCE_REF_COUNT_BASE                                               \
    override ULONG STDMETHODCALLTYPE AddRef(void) {return CMILPoolResource::AddRef();}   \
    override ULONG STDMETHODCALLTYPE Release(void) {return CMILPoolResource::Release();}

/*=========================================================================*\
    IMILPoolManager - MIL interface pooled resource lifetime manager
\*=========================================================================*/

class IMILPoolManager
{
public:

    // Used to notify the manager that there are no outstanding uses and
    //  the manager has full control.
    virtual void UnusedNotification(__inout_ecount(1) CMILPoolResource *pUnused) = 0;

    // Used to notify the manager that the resource is no longer usable
    //  and should be removed from the pool.
    virtual void UnusableNotification(__inout_ecount(1) CMILPoolResource *pUnusable) = 0;
};

/*=========================================================================*/



