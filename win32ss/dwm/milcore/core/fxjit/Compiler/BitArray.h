// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Definitions of class CBitArray.
//
//-----------------------------------------------------------------------------
#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      CBitArray
//
//  Synopsis:
//      Compact storage for linear array of 1-bit variables.
//      Provides bitwise operations on the whole array.
//      Serves as helper for class CDiagram.
//
//      CBitArray does not hold on memory and have no constructor/destructor.
//      Caller should allocate an array of UINT32 values and cast it
//      to single CBitArray or an array of CBitArray instances.
//
//-----------------------------------------------------------------------------
class CBitArray
{
public:

    static UINT32 GetSizeInDWords(UINT32 uBitSize)
    {
        return (uBitSize + 31) >> 5;
    }

    CBitArray& Clear(UINT32 size)
    {
        UINT32 *pDst = Data();
        for (UINT32 i = 0; i < size; i++) pDst[i] = 0;
        return *this;
    }

    CBitArray& Copy(CBitArray const &that, UINT32 size)
    {
        UINT32 *pDst = Data();
        UINT32 const *pSrc = that.Data();
        for (UINT32 i = 0; i < size; i++) pDst[i] = pSrc[i];
        return *this;
    }

    CBitArray& And(CBitArray const &that, UINT32 size)
    {
        UINT32 *pDst = Data();
        UINT32 const *pSrc = that.Data();
        for (UINT32 i = 0; i < size; i++) pDst[i] &= pSrc[i];
        return *this;
    }

    CBitArray& Or(CBitArray const &that, UINT32 size)
    {
        UINT32 *pDst = Data();
        UINT32 const *pSrc = that.Data();
        for (UINT32 i = 0; i < size; i++) pDst[i] |= pSrc[i];
        return *this;
    }

    bool Get(UINT32 index) const
    {
        return (Data()[index >> 5] & (1 << (index & 0x1F))) != 0;
    }

    void Set(UINT32 index)
    {
        Data()[index >> 5] |= 1 << (index & 0x1F);
    }

    void Reset(UINT32 index)
    {
        Data()[index >> 5] &= ~(1 << (index & 0x1F));
    }

    UINT32 Count(UINT32 size) const
    {
        UINT32 const *pSrc = Data();
        UINT32 uCount = 0;
        for (UINT32 i = 0; i < size; i++)
        {
            UINT32 uData = pSrc[i];
            if (uData)
            {
                uData = (uData & 0x55555555) + ((uData >>  1) & 0x55555555);
                uData = (uData & 0x33333333) + ((uData >>  2) & 0x33333333);
                uData = (uData & 0x0F0F0F0F) + ((uData >>  4) & 0x0F0F0F0F);
                uData = (uData & 0x00FF00FF) + ((uData >>  8) & 0x00FF00FF);
                uData = (uData & 0x0000FFFF) + ((uData >> 16) & 0x0000FFFF);
                uCount += uData;
            }
        }
        return uCount;
    }

private:
    CBitArray(); // private to prohibit direct usage
    UINT32 const *Data() const { return reinterpret_cast<const UINT32*>(this); }
    UINT32       *Data()       { return reinterpret_cast<      UINT32*>(this); }
};

