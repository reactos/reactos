//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       atomtbl.hxx
//
//  History:    20-Sep-1996     AnandRa     Created
//
//  Contents:   CAtomTable
//
//----------------------------------------------------------------------------

#ifndef I_ATOMTBL_HXX_
#define I_ATOMTBL_HXX_
#pragma INCMSG("--- Beg 'atomtbl.hxx'")

#ifndef X_CSTR_HXX_
#define X_CSTR_HXX_
#include "cstr.hxx"
#endif

MtExtern(CAtomTable)
MtExtern(CAtomTable_pv)

class CAtomTable : public CDataAry<CStr>
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAtomTable))
    CAtomTable() : CDataAry<CStr>(Mt(CAtomTable_pv)) {}
    HRESULT AddNameToAtomTable(LPCTSTR pch, long *plIndex);
    HRESULT GetAtomFromName(LPCTSTR pch, long *plIndex, BOOL fCaseSensitive = TRUE,
                                    BOOL fStartFromGivenIndex = FALSE);
    HRESULT GetNameFromAtom(long lIndex, LPCTSTR *ppch);
    void    Free();
};

#pragma INCMSG("--- End 'atomtbl.hxx'")
#else
#pragma INCMSG("*** Dup 'atomtbl.hxx'")
#endif
