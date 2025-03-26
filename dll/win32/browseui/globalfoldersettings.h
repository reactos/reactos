/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once

struct SBFOLDERSETTINGS // Private version of DEFFOLDERSETTINGS used by CShellBrowser
{
    FOLDERSETTINGS FolderSettings;

    enum { DEF_FVM = FVM_DETAILS }; // Windows uses FVM_ICON?
    enum { DEF_FWF = FWF_NOHEADERINALLVIEWS };
    void InitializeDefaults()
    {
        FolderSettings.ViewMode = DEF_FVM;
        FolderSettings.fFlags = DEF_FWF;
    }
    void Load();
};

class CGlobalFolderSettings :
    public CComCoClass<CGlobalFolderSettings, &CLSID_GlobalFolderSettings>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IGlobalFolderSettings
{
private:
public:
    CGlobalFolderSettings();
    ~CGlobalFolderSettings();

    static HRESULT ResetBrowserSettings();
    static HRESULT SaveBrowserSettings(const SBFOLDERSETTINGS &sbfs);
    static HRESULT Load(DEFFOLDERSETTINGS &dfs);
    static HRESULT Save(const DEFFOLDERSETTINGS *pFDS);

    // *** IGlobalFolderSettings methods ***
    STDMETHOD(Get)(struct DEFFOLDERSETTINGS *pFDS, UINT cb) override;
    STDMETHOD(Set)(const struct DEFFOLDERSETTINGS *pFDS, UINT cb, UINT unknown) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_GLOBALFOLDERSETTINGS)
    DECLARE_NOT_AGGREGATABLE(CGlobalFolderSettings)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CGlobalFolderSettings)
        COM_INTERFACE_ENTRY_IID(IID_IGlobalFolderSettings, IGlobalFolderSettings)
    END_COM_MAP()
};
