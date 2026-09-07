// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//------------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:     CDoubleAlignedArray
//
//  Synopsis:  This provides a clean way to 8-byte-align an array on the
//             stack. Replace:
//               Foo afoo[42];
//             with:
//               CDoubleAlignedArray<Foo, 42> afoo;
//
//             Some things may need to be rewritten because pointer arithmetic
//             on afoo won't work (there will be compile-time errors). But
//             normal array indexing works fine because of the [] operator.
//
//             Why would you want to 8-byte-align something on the stack? This
//             is not about alignment exceptions, it's about perf - on x86, a
//             stack array of "double" or "__int64" values (or of structures
//             which contain those values) will incur processor penalties
//             whenever an 8-byte value spans a cache line boundary.
//
//  Notes:     This class was added as a workaround until we can rely on
//             automatic double-alignment provided by "link-time code
//             generation". See Src\Media\Render\sources.inc for more details.
//
//------------------------------------------------------------------------------

template <class T, int n> class CDoubleAlignedArray
{
public:
    CDoubleAlignedArray()
    {
        // This setup code optimizes fairly well. Example:
        //
        //  lea         eax,[esp+74h]  
        //  and         eax,0FFFFFFFBh

        // If there are multiple elements, then T must be 8-byte packed -
        // otherwise, array elements won't be aligned properly.
        
        Assert((n == 1) || ((sizeof(T) & 7) == 0));
        
        UINT_PTR uiStorage = reinterpret_cast<UINT_PTR>(&(m_aStorage[0]));

        // We expect to be at least DWORD-aligned
        Assert((uiStorage & 3) == 0);

        uiStorage = (uiStorage + 4) & (~4);
        
        m_pArray = reinterpret_cast<T *>(uiStorage);
    }
    
    // Index operator

    T &operator[](INT n) const
    {
        return m_pArray[n];
    }

private:
    BYTE m_aStorage[sizeof(T) * n + 4];
    T *m_pArray;
};



