//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       ext.hxx
//
//  Contents:   CNetObj class definition to handle network object context menu
//              and property sheet shell extensions.
//
//  History:    25-Sep-95 BruceFo  Created
//
//--------------------------------------------------------------------------

#ifndef _EXT_HXX_
#define _EXT_HXX_

#define MAX_ONE_RESOURCE    2048

class CNetObj
            : public IShellExtInit,
              public IShellPropSheetExt
{
    DECLARE_SIG;

    friend class CPage;

public:

    CNetObj();
    ~CNetObj();

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    //
    // IShellExtInit methods
    //

    STDMETHOD(Initialize)(
        LPCITEMIDLIST pidlFolder,
        LPDATAOBJECT pDataObject,
        HKEY hkeyProgID);

    //
    // IShellPropSheetExt methods
    //

    STDMETHOD(AddPages)(
        LPFNADDPROPSHEETPAGE lpfnAddPage,
        LPARAM lParam);

    STDMETHOD(ReplacePage)(
        UINT uPageID,
        LPFNADDPROPSHEETPAGE lpfnReplaceWith,
        LPARAM lParam);

private:

    HRESULT
    FillAndAddPage(
        LPFNADDPROPSHEETPAGE lpfnAddPage,
        LPARAM lParam,
        LPTSTR pszTemplate
        );

    ULONG           _uRefs;             // OLE reference count
    LPDATAOBJECT    _pDataObject;
    HKEY            _hkeyProgID;        // reg. database key to ProgID
    BYTE            _bufNetResource[MAX_ONE_RESOURCE];

};

#endif // _EXT_HXX_
