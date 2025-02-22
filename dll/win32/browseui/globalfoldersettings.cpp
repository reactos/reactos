/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
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

#include "precomp.h"

#define DEFBROWSERSTREAM L"Settings"
#define DEFAULT_VID GUID_NULL // We don't support WebView
#define CURRENT_VERSION ( DEFFOLDERSETTINGS::VER_XP )

template<class S, class D> static void CopyTo(const S &Src, D &Dst)
{
    Dst.FolderSettings = Src.FolderSettings;
}

static void EnsureValid(FOLDERSETTINGS &fs)
{
    if ((int)fs.ViewMode < FVM_AUTO || (int)fs.ViewMode > FVM_LAST)
        fs.ViewMode = SBFOLDERSETTINGS::DEF_FVM;

    fs.fFlags &= ~(FWF_OWNERDATA | FWF_DESKTOP | FWF_TRANSPARENT | FWF_NOCLIENTEDGE |
                   FWF_NOSUBFOLDERS | FWF_NOSCROLL | FWF_NOENUMREFRESH | FWF_NOVISIBLE);
}

static void EnsureValid(DEFFOLDERSETTINGS &dfs)
{
    EnsureValid(dfs.FolderSettings);
}

static void InitializeDefaults(DEFFOLDERSETTINGS &dfs)
{
    C_ASSERT(FIELD_OFFSET(DEFFOLDERSETTINGS, FolderSettings) == 4);
    C_ASSERT(FIELD_OFFSET(DEFFOLDERSETTINGS, ViewPriority) == DEFFOLDERSETTINGS::SIZE_IE4);
    C_ASSERT(sizeof(DEFFOLDERSETTINGS) == DEFFOLDERSETTINGS::SIZE_XP);

    *(UINT*)&dfs = FALSE; // Set all unknown flags to FALSE
    dfs.Statusbar = TRUE;
    dfs.FolderSettings.ViewMode = SBFOLDERSETTINGS::DEF_FVM;
    dfs.FolderSettings.fFlags = SBFOLDERSETTINGS::DEF_FWF;
    dfs.vid = DEFAULT_VID;
    dfs.Version = CURRENT_VERSION;
    dfs.Counter = 0;
    dfs.ViewPriority = VIEW_PRIORITY_CACHEMISS;
}

void SBFOLDERSETTINGS::Load()
{
    DEFFOLDERSETTINGS dfs;
    CGlobalFolderSettings::Load(dfs);
    CopyTo(dfs, *this);
}

CGlobalFolderSettings::CGlobalFolderSettings()
{
}

CGlobalFolderSettings::~CGlobalFolderSettings()
{
}

HRESULT CGlobalFolderSettings::ResetBrowserSettings()
{
    return Save(NULL);
}

HRESULT CGlobalFolderSettings::SaveBrowserSettings(const SBFOLDERSETTINGS &sbfs)
{
    DEFFOLDERSETTINGS dfs;
    InitializeDefaults(dfs);
    CopyTo(sbfs, dfs);
    return Save(&dfs);
}

HRESULT CGlobalFolderSettings::Load(DEFFOLDERSETTINGS &dfs)
{
    HKEY hStreamsKey = SHGetShellKey(SHKEY_Key_Explorer, L"Streams", FALSE);
    if (hStreamsKey)
    {
        DWORD cb = sizeof(DEFFOLDERSETTINGS);
        DWORD err = SHRegGetValueW(hStreamsKey, NULL, DEFBROWSERSTREAM,
                                   SRRF_RT_REG_BINARY, NULL, &dfs, &cb);
        RegCloseKey(hStreamsKey);
        if (!err && cb >= DEFFOLDERSETTINGS::SIZE_NT4)
        {
            if (cb < FIELD_OFFSET(DEFFOLDERSETTINGS, vid))
            {
                dfs.FolderSettings.fFlags = SBFOLDERSETTINGS::DEF_FWF;
            }
            if (cb < FIELD_OFFSET(DEFFOLDERSETTINGS, Version))
            {
                dfs.vid = DEFAULT_VID;
            }
            if (cb <= FIELD_OFFSET(DEFFOLDERSETTINGS, ViewPriority))
            {
                dfs.Version = CURRENT_VERSION;
            }
            dfs.ViewPriority = VIEW_PRIORITY_STALECACHEHIT;
            EnsureValid(dfs);
            return S_OK;
        }
    }
    InitializeDefaults(dfs);
    return S_FALSE;
}

HRESULT CGlobalFolderSettings::Save(const DEFFOLDERSETTINGS *pFDS)
{
    HKEY hStreamsKey = SHGetShellKey(SHKEY_Key_Explorer, L"Streams", TRUE);
    if (!hStreamsKey)
        return E_FAIL;

    HRESULT hr = E_INVALIDARG;
    if (!pFDS)
    {
        DWORD err = SHDeleteValueW(hStreamsKey, NULL, DEFBROWSERSTREAM);
        hr = (!err || err == ERROR_FILE_NOT_FOUND) ? S_OK : HResultFromWin32(err);
    }
    else
    {
        DWORD cb = sizeof(DEFFOLDERSETTINGS);
        hr = HResultFromWin32(SHSetValueW(hStreamsKey, NULL, DEFBROWSERSTREAM, REG_BINARY, pFDS, cb));
    }
    RegCloseKey(hStreamsKey);
    return hr;
}

HRESULT STDMETHODCALLTYPE CGlobalFolderSettings::Get(struct DEFFOLDERSETTINGS *pFDS, UINT cb)
{
    if (cb != sizeof(DEFFOLDERSETTINGS) || !pFDS)
        return E_INVALIDARG;
    return Load(*pFDS);
}

HRESULT STDMETHODCALLTYPE CGlobalFolderSettings::Set(const struct DEFFOLDERSETTINGS *pFDS, UINT cb, UINT unknown)
{
    // NULL pFDS means reset
    if (cb != sizeof(DEFFOLDERSETTINGS) && pFDS)
        return E_INVALIDARG;
    return Save(pFDS);
}
