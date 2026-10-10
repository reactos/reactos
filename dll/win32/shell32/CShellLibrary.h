/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     IShellLibrary header
 * COPYRIGHT:   Copyright 2021 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */

#ifndef _CSHELLLIBRARY_H_
#define _CSHELLLIBRARY_H_

class CShellLibrary :
    public CComCoClass<CShellLibrary, &CLSID_ShellLibrary>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellLibrary
{
public:
    CShellLibrary();
    ~CShellLibrary();

    /*** IShellLibrary methods ***/
    STDMETHODIMP AddFolder(IShellItem *psiLocation) override;
    STDMETHODIMP Commit() override;
    STDMETHODIMP GetDefaultSaveFolder(DEFAULTSAVEFOLDERTYPE dsft, REFIID riid, void **ppv) override;
    STDMETHODIMP GetFolders(LIBRARYFOLDERFILTER lff, REFIID riid, void **ppv) override;
    STDMETHODIMP GetFolderType(FOLDERTYPEID *pftid) override;
    STDMETHODIMP GetIcon(LPWSTR *ppszIcon) override;
    STDMETHODIMP GetOptions(LIBRARYOPTIONFLAGS *plofOptions) override;
    STDMETHODIMP LoadLibraryFromItem(IShellItem *psiLibrary, DWORD grfMode) override;
    STDMETHODIMP LoadLibraryFromKnownFolder(REFKNOWNFOLDERID kfidLibrary, DWORD grfMode) override;
    STDMETHODIMP RemoveFolder(IShellItem *psiLocation) override;
    STDMETHODIMP ResolveFolder(IShellItem *psiFolderToResolve, DWORD dwTimeout, REFIID riid, void **ppv) override;
    STDMETHODIMP Save(IShellItem *psiFolderToSaveIn, LPCWSTR pszLibraryName, LIBRARYSAVEFLAGS lsf, IShellItem **ppsiSavedTo) override;
    STDMETHODIMP SaveInKnownFolder(REFKNOWNFOLDERID kfidToSaveIn, LPCWSTR pszLibraryName, LIBRARYSAVEFLAGS lsf, IShellItem **ppsiSavedTo) override;
    STDMETHODIMP SetDefaultSaveFolder(DEFAULTSAVEFOLDERTYPE dsft, IShellItem *psi) override;
    STDMETHODIMP SetFolderType(REFFOLDERTYPEID ftid) override;
    STDMETHODIMP SetIcon(LPCWSTR pszIcon) override;
    STDMETHODIMP SetOptions(LIBRARYOPTIONFLAGS lofMask, LIBRARYOPTIONFLAGS lofOptions) override;


DECLARE_REGISTRY_RESOURCEID(IDR_SHELLLIBRARY)
DECLARE_NOT_AGGREGATABLE(CShellLibrary)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CShellLibrary)
    COM_INTERFACE_ENTRY_IID(IID_IShellLibrary, IShellLibrary)
END_COM_MAP()
};

#endif // _CSHELLLIBRARY_H_
