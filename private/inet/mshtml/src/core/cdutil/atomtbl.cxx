//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       atomtbl.cxx
//
//  History:    20-Sep-1996     AnandRa     Created
//
//  Contents:   CAtomTable implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ATOMTBL_HXX_
#define X_ATOMTBL_HXX_
#include "atomtbl.hxx"
#endif

MtDefine(CAtomTable, Utilities, "CAtomTable")
MtDefine(CAtomTable_pv, CAtomTable, "CAtomTable::_pv")

HRESULT
CAtomTable::AddNameToAtomTable(LPCTSTR pch, long *plIndex)
{
    HRESULT hr = S_OK;
    long    lIndex;
    CStr *  pstr;
    
    for (lIndex = 0; lIndex < Size(); lIndex++)
    {
        pstr = (CStr *)Deref(sizeof(CStr), lIndex);
        if (_tcsequal(pch, *pstr))
            break;
    }
    if (lIndex == Size())
    {
        CStr cstr;
        
        //
        // Not found, so add element to array.
        //

        hr = THR(cstr.Set(pch));
        if (hr)
            goto Cleanup;
       
        hr = THR(AppendIndirect(&cstr));
        if (hr)
            goto Cleanup;

        // The array now owns the memory for the cstr, so take it away from
        // the cstr on the stack.

        cstr.TakePch();
    }

    if (plIndex)
    {
        *plIndex = lIndex;
    }

Cleanup:
    RRETURN(hr);
}


HRESULT
CAtomTable::GetAtomFromName(LPCTSTR pch, long *plIndex, BOOL fCaseSensitive /*= TRUE */,
                                    BOOL fStartFromGivenIndex /* = FALSE */)
{
    long    lIndex;
    HRESULT hr = S_OK;
    CStr *  pstr;

    if(fStartFromGivenIndex)
        lIndex = *plIndex;
    else
        lIndex = 0;
    
    for (; lIndex < Size(); lIndex++)
    {
        pstr = (CStr *)Deref(sizeof(CStr), lIndex);
        if(fCaseSensitive)
        {
            if (_tcsequal(pch, *pstr))
                break;
        }
        else
        {
            if(_tcsicmp(pch, *pstr) == 0)
                break;
        }
    }
    
    if (lIndex == Size())
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    if (plIndex)
    {
        *plIndex = lIndex;
    }

Cleanup:    
    RRETURN(hr);
}


HRESULT 
CAtomTable::GetNameFromAtom(long lIndex, LPCTSTR *ppch)
{
    HRESULT hr = S_OK;
    CStr *  pcstr;
    
    if (Size() <= lIndex)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    pcstr = (CStr *)Deref(sizeof(CStr), lIndex);
    *ppch = (TCHAR *)*pcstr;
    
Cleanup:    
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}


void
CAtomTable::Free()
{
    CStr *  pcstr;
    long    i;
    
    for (i = 0; i < Size(); i++)
    {
        pcstr = (CStr *)Deref(sizeof(CStr), i);
        pcstr->Free();
    }
    DeleteAll();
}

