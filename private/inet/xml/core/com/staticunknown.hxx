/*
 * @(#)StaticUnknown.hxx 1.0 3/12/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _STATICUNKNOWN_HXX
#define _STATICUNKNOWN_HXX

//------------------------------------------------------------------------
// Every time we create a foreign COM object that we hold onto as a static
// pointer, you need to add the addres of register that pointer so we can
// cleanup those static objects at the right time.

struct StaticUnknown
{
    IUnknown** ppUnk;
    StaticUnknown* pNext;
};

extern StaticUnknown* g_pUnkList;

HRESULT RegisterStaticUnknown(IUnknown** ppUnk);

//------------------------------------------------------------------------
// All COM objects that we hand out to clients have to call
// IncrementComponents() in their constructors and DecrementComponents()
// in their destructors.  DecrementComponents() releases all static
// foreign IUnknown Objects when the component count goes to zero.
// This global count is also used to implement the DLLCanUnloadNow function.
long IncrementComponents();
long DecrementComponents();
long GetComponentCount();   // return the current count.


#endif _STATICUNKNOWN_HXX