// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Class C_LazyVar - the wrapper to hold on one of C_Variable derivatives.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_LazyVar
//
//  Synopsis:
//      The wrapper to hold on one of C_Variable derivatives.
//
//      The instance of C_LazyVar is initialized on demand.
//      Intended to use in the cases when variable type is not known
//      at construction time.
//
//------------------------------------------------------------------------------
class C_LazyVar : public C_Variable
{
public:
    static C_LazyVar * Alloc(UINT32 size);
    operator C_u32x4&();
    operator C_f32x4&();
private:
    // Define placement operator new and do-nothing delete
    void * __cdecl operator new[](size_t /*cbSize*/, void * pvMemory) { return pvMemory; }
    void __cdecl operator delete[](void*, void*)
    {
        WarpError("C_LazyVar::operator delete[]");
    }
};

