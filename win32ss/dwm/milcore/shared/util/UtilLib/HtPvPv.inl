// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       HtPvPv.inl
//  Contents:   Hash table mapping PVOID to PVOID
//------------------------------------------------------------------------------

#ifndef UtilLib__HtPvPv_inl__INCLUDED
#define UtilLib__HtPvPv_inl__INCLUDED

MtExtern(CHtPvPv)

//------------------------------------------------------------------------------
//  Member:     CHtPvPv::HtKeyEqual
//------------------------------------------------------------------------------
inline BOOL 
CHtPvPv::HtKeyEqual(HTENT *pEnt, void *pvKey, const void *pvData) const
{
    BOOL fRet;
    if (((void *)((DWORD_PTR)pEnt->pvKey & ~1L)) == (pvKey))
    {
        Assert(!_lpfnCompare || pvData);
        fRet = _lpfnCompare ? (*_lpfnCompare)(_pObject, pvData, pEnt->pvVal) : TRUE;
    }
    else
    {
        fRet = FALSE;
    }
    return fRet;
}

//------------------------------------------------------------------------------
//  Member:     CHtPvPv::HtKeyEqualWithValue
//------------------------------------------------------------------------------
inline BOOL 
CHtPvPv::HtKeyEqualWithValue(HTENT *pEnt, void *pvKey, void *pvVal) const
{
    return      ((void *)((DWORD_PTR)pEnt->pvKey & ~1L)) == pvKey
            &&  pEnt->pvVal == pvVal;
}

#if DBG==1
//------------------------------------------------------------------------------
//  Member:     CHtPvPv::IsPresent
//------------------------------------------------------------------------------
inline BOOL
CHtPvPv::IsPresent(void * pvKey, const void *pvData)
{
    void *pVal;
    return (LookupSlow(pvKey, pvData, &pVal) == S_OK);
}
#endif

#endif // UtilLib__HtPvPv_inl__INCLUDED


