// $$ClassType$$EI.cpp : Implementation of C$$ClassType$$EI
#include "stdafx.h"
#include "$$root$$.h"
#include "$$ClassType$$EI.h"

/////////////////////////////////////////////////////////////////////////////
// C$$ClassType$$EI

HRESULT C$$ClassType$$EI::Load(LPCOLESTR pszFileName,
                  DWORD dwMode)
{
    // TODO: Implement this function if the file itself 
    // contains the icon to be display (i.e. An image file)
    return NOERROR;
}

HRESULT C$$ClassType$$EI::GetIconLocation(UINT uFlags,
                         LPTSTR szIconFile,
                         UINT cchMax,
                         int* piIndex,
                         UINT* pwFlags)
{
    if (GetModuleFileName(_Module.GetModuleInstance(), szIconFile, cchMax))
    {
        *piIndex = 0;
        *pwFlags |= GIL_PERINSTANCE;
        return NOERROR;
    }
    return E_FAIL;
}

HRESULT C$$ClassType$$EI::Extract( LPCTSTR pszFile,
                                 UINT nIconIndex,
                                 HICON* phiconLarge,
                                 HICON* phiconSmall,
                                 UINT nIconSize)
{
    // TODO: Implement this function if the file itself 
    // contains the icon to be display (i.e. An image file)
    // and change the return type to NOERROR
    return S_FALSE;
}
