/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     IShellLibrary implementation
 * COPYRIGHT:   Copyright 2021 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/***********************************************************************
*   IShellLibrary implementation
*/

CShellLibrary::CShellLibrary()
{
}

CShellLibrary::~CShellLibrary()
{
}

STDMETHODIMP CShellLibrary::AddFolder(IShellItem *psiLocation)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::Commit()
{
    UNIMPLEMENTED;
    /* Fake success */
    return S_OK;
}

STDMETHODIMP CShellLibrary::GetDefaultSaveFolder(DEFAULTSAVEFOLDERTYPE dsft, REFIID riid, void **ppv)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::GetFolders(LIBRARYFOLDERFILTER lff, REFIID riid, void **ppv)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::GetFolderType(FOLDERTYPEID *pftid)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::GetIcon(LPWSTR *ppszIcon)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::GetOptions(LIBRARYOPTIONFLAGS *plofOptions)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::LoadLibraryFromItem(IShellItem *psiLibrary, DWORD grfMode)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::LoadLibraryFromKnownFolder(REFKNOWNFOLDERID kfidLibrary, DWORD grfMode)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::RemoveFolder(IShellItem *psiLocation)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::ResolveFolder(IShellItem *psiFolderToResolve, DWORD dwTimeout, REFIID riid, void **ppv)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::Save(IShellItem *psiFolderToSaveIn, LPCWSTR pszLibraryName, LIBRARYSAVEFLAGS lsf, IShellItem **ppsiSavedTo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::SaveInKnownFolder(REFKNOWNFOLDERID kfidToSaveIn, LPCWSTR pszLibraryName, LIBRARYSAVEFLAGS lsf, IShellItem **ppsiSavedTo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::SetDefaultSaveFolder(DEFAULTSAVEFOLDERTYPE dsft, IShellItem *psi)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::SetFolderType(REFFOLDERTYPEID ftid)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::SetIcon(LPCWSTR pszIcon)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CShellLibrary::SetOptions(LIBRARYOPTIONFLAGS lofMask, LIBRARYOPTIONFLAGS lofOptions)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

