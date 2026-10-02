// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Generic stack template class.
//
//  The stack is implemented as a linked list of arrays (frames). The frame
//  size is determined by the template variable TExpansionSize. Whenever the
//  stack runs out of space a new frame is allocated and appended to the linked
//  list. When all elements are popped from the top frame the frame is deleted.
//
//
// The watermark stack is an array based stack that grows based on the
// specified grow factor TGrowFactor. To minimize reallocations of the internal
// array the watermark stack keeps track of the maximum capacity used between
// calls to Optimize (watermark). Every TTrimCount calls to Optimize the
// watermark is inspected and the capacity is adjusted. 
//
// For example: When traversing a scene graph a matrix stack is typically
//              needed to keep track
// of the local to world transform. Because the different parts of the scene
// graph might have different depth a stack that keeps closely track of memory
// usage might allocate and free its internal memory while traversing the
// various path of the scene graph. The watermark stack tries to solve this
// problem by not shrinking the stack till Optimize is called. In this example
// Optimize could be called after every frame. This would avoid the
// reallocations during the traversal but allow the stack to shrink if the
// scene graph changes. By setting TTrimCount to 1 the capacity of the stack
// would be adjusted every frame. A large TTrimCount can be used to average the
// capacity setting over multiple frames in this example.
//
//-----------------------------------------------------------------------------

#pragma once

MtExtern(CWatermarkStack);

template <
    class TValue, 
    UINT TMinCapacity, 
    UINT TGrowFactor, 
    UINT TTrimCount
    >
class CWatermarkStack
{
public:
    CWatermarkStack();

    ~CWatermarkStack();

    // Push
    //     Pushes the argument onto the stack.
    HRESULT Push(__in_ecount(1) const TValue& val);

    // Pop
    //
    //     Returns the top element in the pVal out argument and then
    //     pops the stack by 1. Returns false if the stack was empty
    //     otherwise true.
    BOOL Pop(__out_ecount_opt(1) TValue* pVal = NULL);

    // Top 
    //     Returns the top element in the out argument pVal. Note that 
    //     pVal must be != NULL. If the stack is empty or pVal is NULL
    //     the method returns a failure code.
    HRESULT Top(__out_ecount(1) TValue* pVal);

    // GetSize
    //     Returns the current size of the stack.
    UINT GetSize();

    // IsEmpty
    //     TRUE iff the stack is empty.
    BOOL IsEmpty() const;

    // Clear
    //     Pops all elements of the stack.
    VOID Clear();


    // Optimize
    //     Optimize calculates an optimal stack capacity. The
    //     capacity is optimal in the sense that it tries to
    //     minimize allocations. If a new optimal stack size is
    //     found the internal stack storage is reallocated.
    //     Note that before calling this method the stack must be
    //     empty. 
    VOID Optimize();

    // GetTopByReference
    //     Returns a refernce to the top element, if present.  If the stack
    //     is empty NULL is returned.
    __outro_ecount_opt(1) TValue const *GetTopByReference() const;

private:
    VOID Initialize();

    UINT m_uSize;
    UINT m_uCapacity;
    UINT m_uObserveCount;
    TValue* m_pElements;
    UINT m_uHighWatermark;
};

#include "WatermarkStack.inl"


