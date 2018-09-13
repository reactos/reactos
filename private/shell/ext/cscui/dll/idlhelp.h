//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       idlhelp.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCUI_IDLHELP_H
#define _INC_CSCUI_IDLHELP_H

HRESULT 
BindToObject(
    IShellFolder *psf, 
    REFIID riid, 
    LPCITEMIDLIST pidl, 
    void **ppvOut);


HRESULT 
BindToIDListParent(
    LPCITEMIDLIST pidl, 
    REFIID riid, 
    void **ppv, 
    LPCITEMIDLIST *ppidlLast);


HRESULT IsOfflineFilesFolderID(LPCITEMIDLIST pidl);

#endif // _INC_CSCUI_IDLHELP_H
