//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       share.hxx
//
//  Contents:   CShare class definition to handle Sharing context menu
//              and property sheet shell extensions.
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

#ifndef _SHARE_HXX_
#define _SHARE_HXX_

class CShare
            : public IShellExtInit,
              public IShellPropSheetExt,
              public IContextMenu
{
    DECLARE_SIG;

public:

    CShare();
    ~CShare();

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

    //
    // IContextMenu methods
    //

    STDMETHOD(QueryContextMenu)(
        HMENU hmenu,
        UINT indexMenu,
        UINT idCmdFirst,
        UINT idCmdLast,
        UINT uFlags);

    STDMETHOD(InvokeCommand)(
        LPCMINVOKECOMMANDINFO lpici);

    STDMETHOD(GetCommandString)(
        UINT        idCmd,
        UINT        uType,
        UINT*       pwReserved,
        LPSTR       pszName,
        UINT        cchMax);

private:

    HRESULT
    _GetFSObject(
        LPWSTR lpPath,
        UINT cbMaxPath
        );

    BOOL
    _IsShareableDrive(
        VOID
        );

    BOOL
    _OKToShare(
        VOID
        );

    ULONG           _uRefs;             // OLE reference count
    LPDATAOBJECT    _pDataObject;
    HKEY            _hkeyProgID;        // reg. database key to ProgID
    PWSTR           _pszPath;
    BOOL            _fPathChecked;
    BOOL            _fOkToSharePath;

	BOOL			_bRemote;
	WCHAR			_szServer[MAX_PATH];
	WCHAR			_szShare[NNLEN + 1];
	WCHAR			_szRemotePath[MAX_PATH];

};

#endif // _SHARE_HXX_
