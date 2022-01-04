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

/*
Implements a class that keeps track of a PIDL history and allows
navigation forward and backward. This really should be in shdocvw, but it
is not registered for external instantiation, and the entire IBrowserService
hierarchy that normally spans browseui and shdocvw are collapsed into one
hierarchy in browseui, so I am moving it to browseui for now. If someone
decides to refactor code later, it wouldn't be difficult to move it.

TODO:
****Does original travel log update the current item in the Travel method before or after calling ITravelEntry::Invoke?
****Change to load maximum size from registry
****Add code to track current size
****Fix InsertMenuEntries to not exceed limit of menu item ids provided. Perhaps the method should try to be intelligent and if there are
        too many items, center around the current item? Could cause dispatch problems...
****Move tool tip text templates to resources
  **Simplify code in InsertMenuEntries
    Implement UpdateExternal
    Implement FindTravelEntry
    Implement Clone
    Implement Revert

*/

#include "precomp.h"

class CTravelEntry :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public ITravelEntry
{
public:
    CTravelEntry                            *fNextEntry;
    CTravelEntry                            *fPreviousEntry;
private:
    LPITEMIDLIST                            fPIDL;
    HGLOBAL                                 fPersistState;
public:
    CTravelEntry();
    ~CTravelEntry();
    HRESULT GetToolTipText(IUnknown *punk, LPWSTR pwzText) const;
    long GetSize() const;

    // *** ITravelEntry methods ***
    virtual HRESULT STDMETHODCALLTYPE Invoke(IUnknown *punk);
    virtual HRESULT STDMETHODCALLTYPE Update(IUnknown *punk, BOOL fIsLocalAnchor);
    virtual HRESULT STDMETHODCALLTYPE GetPidl(LPITEMIDLIST *ppidl);

BEGIN_COM_MAP(CTravelEntry)
    COM_INTERFACE_ENTRY_IID(IID_ITravelEntry, ITravelEntry)
END_COM_MAP()
};

class CTravelLog :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public ITravelLog
{
private:
    CTravelEntry                            *fListHead;
    CTravelEntry                            *fListTail;
    CTravelEntry                            *fCurrentEntry;
    long                                    fMaximumSize;
    long                                    fCurrentSize;
    unsigned long                           fEntryCount;
public:
    CTravelLog();
    ~CTravelLog();
    HRESULT Initialize();
    HRESULT FindRelativeEntry(int offset, CTravelEntry **foundEntry);
    void DeleteChain(CTravelEntry *startHere);
    void AppendEntry(CTravelEntry *afterEntry, CTravelEntry *newEntry);
public:

    // *** ITravelLog methods ***
    virtual HRESULT STDMETHODCALLTYPE AddEntry(IUnknown *punk, BOOL fIsLocalAnchor);
    virtual HRESULT STDMETHODCALLTYPE UpdateEntry(IUnknown *punk, BOOL fIsLocalAnchor);
    virtual HRESULT STDMETHODCALLTYPE UpdateExternal(IUnknown *punk, IUnknown *punkHLBrowseContext);
    virtual HRESULT STDMETHODCALLTYPE Travel(IUnknown *punk, int iOffset);
    virtual HRESULT STDMETHODCALLTYPE GetTravelEntry(IUnknown *punk, int iOffset, ITravelEntry **ppte);
    virtual HRESULT STDMETHODCALLTYPE FindTravelEntry(IUnknown *punk, LPCITEMIDLIST pidl, ITravelEntry **ppte);
    virtual HRESULT STDMETHODCALLTYPE GetToolTipText(IUnknown *punk, int iOffset, int idsTemplate, LPWSTR pwzText, DWORD cchText);
    virtual HRESULT STDMETHODCALLTYPE InsertMenuEntries(IUnknown *punk, HMENU hmenu, int nPos, int idFirst, int idLast, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE Clone(ITravelLog **pptl);
    virtual DWORD STDMETHODCALLTYPE CountEntries(IUnknown *punk);
    virtual HRESULT STDMETHODCALLTYPE Revert();

BEGIN_COM_MAP(CTravelLog)
    COM_INTERFACE_ENTRY_IID(IID_ITravelLog, ITravelLog)
END_COM_MAP()
};

CTravelEntry::CTravelEntry()
{
    fNextEntry = NULL;
    fPreviousEntry = NULL;
    fPIDL = NULL;
    fPersistState = NULL;
}

CTravelEntry::~CTravelEntry()
{
    ILFree(fPIDL);
    GlobalFree(fPersistState);
}

HRESULT CTravelEntry::GetToolTipText(IUnknown *punk, LPWSTR pwzText) const
{
    HRESULT                                 hResult;

    hResult = ILGetDisplayNameEx(NULL, fPIDL, pwzText, ILGDN_NORMAL) ? S_OK : E_FAIL;
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    return S_OK;
}

long CTravelEntry::GetSize() const
{
    return 0;
}

HRESULT STDMETHODCALLTYPE CTravelEntry::Invoke(IUnknown *punk)
{
    CComPtr<IPersistHistory>                persistHistory;
    CComPtr<IStream>                        globalStream;
    HRESULT                                 hResult;

    TRACE("CTravelEntry::Invoke for IUnknown punk=%p\n", punk);

    hResult = punk->QueryInterface(IID_PPV_ARG(IPersistHistory, &persistHistory));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = CreateStreamOnHGlobal(fPersistState, FALSE, &globalStream);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = persistHistory->LoadHistory(globalStream, NULL);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CTravelEntry::Update(IUnknown *punk, BOOL fIsLocalAnchor)
{
    CComPtr<ITravelLogClient>               travelLogClient;
    CComPtr<IPersistHistory>                persistHistory;
    CComPtr<IStream>                        globalStream;
    WINDOWDATA                              windowData;
    HRESULT                                 hResult;

    TRACE("CTravelEntry::Update for IUnknown punk=%p, fIsLocalAnchor=%s\n", punk, fIsLocalAnchor ? "TRUE" : "FALSE");


    WCHAR wch[MAX_PATH * 2];
    GetToolTipText(punk, wch);
    TRACE("Updating entry with display name: %S\n", wch);

    ZeroMemory(&windowData, sizeof(WINDOWDATA));
    ILFree(fPIDL);
    fPIDL = NULL;
    GlobalFree(fPersistState);
    fPersistState = NULL;
    hResult = punk->QueryInterface(IID_PPV_ARG(ITravelLogClient, &travelLogClient));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = punk->QueryInterface(IID_PPV_ARG(IPersistHistory, &persistHistory));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = CreateStreamOnHGlobal(NULL, FALSE, &globalStream);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = persistHistory->SaveHistory(globalStream);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = travelLogClient->GetWindowData(globalStream, &windowData);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    fPIDL = windowData.pidl;
    // TODO: Properly free the windowData
    hResult = GetHGlobalFromStream(globalStream, &fPersistState);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    GetToolTipText(punk, wch);
    TRACE("Updated entry display name is now: %S\n", wch);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CTravelEntry::GetPidl(LPITEMIDLIST *ppidl)
{
    if (ppidl == NULL)
        return E_POINTER;
    *ppidl = ILClone(fPIDL);
    if (*ppidl == NULL)
        return E_OUTOFMEMORY;

    TRACE("CTravelEntry::GetPidl returning ppidl=%p\n", *ppidl);

    return S_OK;
}

CTravelLog::CTravelLog()
{
    fListHead = NULL;
    fListTail = NULL;
    fCurrentEntry = NULL;
    fMaximumSize = 0;
    fCurrentSize = 0;
    fEntryCount = 0;
    TRACE("CTravelLog created\n");
}

CTravelLog::~CTravelLog()
{
    CTravelEntry                            *anEntry;
    CTravelEntry                            *next;

    anEntry = fListHead;
    while (anEntry != NULL)
    {
        next = anEntry->fNextEntry;
        anEntry->Release();
        anEntry = next;
    }
    TRACE("CTravelLog destroyed\n");
}

HRESULT CTravelLog::Initialize()
{
    fMaximumSize = 1024 * 1024;         // TODO: change to read this from registry
    // Software\Microsoft\Windows\CurrentVersion\Explorer\TravelLog
    // MaxSize
    return S_OK;
}

HRESULT CTravelLog::FindRelativeEntry(int _offset, CTravelEntry **foundEntry)
{
    CTravelEntry                            *curEntry;
    int offset = _offset;

    if (foundEntry == NULL)
        return E_INVALIDARG;

    *foundEntry = NULL;
    curEntry = fCurrentEntry;
    if (offset < 0)
    {
        while (offset < 0 && curEntry != NULL)
        {
            curEntry = curEntry->fPreviousEntry;
            offset++;
        }
    }
    else
    {
        while (offset > 0 && curEntry != NULL)
        {
            curEntry = curEntry->fNextEntry;
            offset--;
        }
    }
    if (curEntry == NULL)
        return E_INVALIDARG;

    *foundEntry = curEntry;

    TRACE("CTravelLog::FindRelativeEntry for offset %d, returning %p\n", offset, *foundEntry);

    return S_OK;
}

void CTravelLog::DeleteChain(CTravelEntry *startHere)
{
    CTravelEntry                            *saveNext;
    long                                    itemSize;

    TRACE("CTravelLog::DeleteChain deleting chain starting at %p\n", startHere);

    long startEntryCount = fEntryCount;

    if (startHere->fPreviousEntry != NULL)
    {
        startHere->fPreviousEntry->fNextEntry = NULL;
        fListTail = startHere->fPreviousEntry;
    }
    else
    {
        fListHead = NULL;
        fListTail = NULL;
    }
    while (startHere != NULL)
    {
        saveNext = startHere->fNextEntry;
        itemSize = startHere->GetSize();
        fCurrentSize -= itemSize;
        startHere->Release();
        startHere = saveNext;
        fEntryCount--;
    }

    TRACE("CTravelLog::DeleteChain chain of %d items deleted\n", startEntryCount - fEntryCount);
}

void CTravelLog::AppendEntry(CTravelEntry *afterEntry, CTravelEntry *newEntry)
{
    if (afterEntry == NULL)
    {
        TRACE("CTravelLog::AppendEntry appending %p after NULL. Resetting head and tail\n", newEntry);
        fListHead = newEntry;
        fListTail = newEntry;
    }
    else
    {
        TRACE("CTravelLog::AppendEntry appending %p after %p\n", newEntry, afterEntry);
        newEntry->fNextEntry = afterEntry->fNextEntry;
        afterEntry->fNextEntry = newEntry;
        newEntry->fPreviousEntry = afterEntry;
        if (newEntry->fNextEntry == NULL)
            fListTail = newEntry;
        else
            newEntry->fNextEntry->fPreviousEntry = newEntry;
    }
    fEntryCount++;
}

HRESULT STDMETHODCALLTYPE CTravelLog::AddEntry(IUnknown *punk, BOOL fIsLocalAnchor)
{
    CComObject<CTravelEntry>                *newEntry;
    long                                    itemSize;

    TRACE("CTravelLog::AddEntry for IUnknown punk=%p, fIsLocalAnchor=%s\n", punk, fIsLocalAnchor ? "TRUE" : "FALSE");

    if (punk == NULL)
        return E_INVALIDARG;
    ATLTRY (newEntry = new CComObject<CTravelEntry>);
    if (newEntry == NULL)
        return E_OUTOFMEMORY;
    newEntry->AddRef();
    if (fCurrentEntry != NULL && fCurrentEntry->fNextEntry != NULL)
        DeleteChain(fCurrentEntry->fNextEntry);
    AppendEntry(fCurrentEntry, newEntry);
    itemSize = newEntry->GetSize();
    fCurrentSize += itemSize;
    fCurrentEntry = newEntry;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CTravelLog::UpdateEntry(IUnknown *punk, BOOL fIsLocalAnchor)
{
    if (punk == NULL)
        return E_INVALIDARG;
    if (fCurrentEntry == NULL)
        return E_UNEXPECTED;
    return fCurrentEntry->Update(punk, fIsLocalAnchor);
}

HRESULT STDMETHODCALLTYPE CTravelLog::UpdateExternal(IUnknown *punk, IUnknown *punkHLBrowseContext)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CTravelLog::Travel(IUnknown *punk, int iOffset)
{
    CTravelEntry                            *destinationEntry;
    HRESULT                                 hResult;

    TRACE("CTravelLog::Travel for IUnknown punk=%p at offset=%d\n", punk, iOffset);

    hResult = FindRelativeEntry(iOffset, &destinationEntry);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    fCurrentEntry = destinationEntry;
    hResult = destinationEntry->Invoke(punk);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CTravelLog::GetTravelEntry(IUnknown *punk, int iOffset, ITravelEntry **ppte)
{
    CTravelEntry                            *destinationEntry;
    HRESULT                                 hResult;

    hResult = FindRelativeEntry(iOffset, &destinationEntry);
    if (FAILED(hResult))
        return hResult;
    hResult = destinationEntry->QueryInterface(IID_PPV_ARG(ITravelEntry, ppte));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    TRACE("CTravelLog::GetTravelEntry for IUnknown punk=%p at offset=%d returning %p\n", punk, iOffset, *ppte);

    return hResult;
}

HRESULT STDMETHODCALLTYPE CTravelLog::FindTravelEntry(IUnknown *punk, LPCITEMIDLIST pidl, ITravelEntry **ppte)
{
    if (ppte == NULL)
        return E_POINTER;
    if (punk == NULL || pidl == NULL)
        return E_INVALIDARG;

    UNIMPLEMENTED;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CTravelLog::GetToolTipText(IUnknown *punk, int iOffset, int idsTemplate, LPWSTR pwzText, DWORD cchText)
{
    CTravelEntry                            *destinationEntry;
    wchar_t                                 tempString[MAX_PATH];
    wchar_t                                 templateString[200];
    HRESULT                                 hResult;

    if (pwzText == NULL)
        return E_POINTER;
    if (punk == NULL || cchText == 0)
        return E_INVALIDARG;
    hResult = FindRelativeEntry(iOffset, &destinationEntry);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = destinationEntry->GetToolTipText(punk, tempString);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    if (iOffset < 0)
    {
        if(LoadStringW(_AtlBaseModule.GetResourceInstance(),
                            IDS_BACK, templateString, sizeof(templateString) / sizeof(wchar_t)) == 0)
            return HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        if(LoadStringW(_AtlBaseModule.GetResourceInstance(),
                            IDS_FORWARD, templateString, sizeof(templateString) / sizeof(wchar_t)) == 0)
            return HRESULT_FROM_WIN32(GetLastError());
    }
    _snwprintf(pwzText, cchText, templateString, tempString);

    TRACE("CTravelLog::GetToolTipText for IUnknown punk=%p at offset=%d returning L\"%S\"\n", punk, iOffset, pwzText);

    return S_OK;
}

static void FixAmpersands(wchar_t *buffer)
{
    wchar_t                                 tempBuffer[MAX_PATH * 2];
    wchar_t                                 ch;
    wchar_t                                 *srcPtr;
    wchar_t                                 *dstPtr;

    srcPtr = buffer;
    dstPtr = tempBuffer;
    while (*srcPtr != 0)
    {
        ch = *srcPtr++;
        *dstPtr++ = ch;
        if (ch == '&')
            *dstPtr++ = '&';
    }
    *dstPtr = 0;
    wcscpy(buffer, tempBuffer);
}

HRESULT STDMETHODCALLTYPE CTravelLog::InsertMenuEntries(IUnknown *punk, HMENU hmenu,
    int nPos, int idFirst, int idLast, DWORD dwFlags)
{
    CTravelEntry                            *currentItem;
    MENUITEMINFO                            menuItemInfo;
    wchar_t                                 itemTextBuffer[MAX_PATH * 2];
    HRESULT                                 hResult;

    TRACE("CTravelLog::InsertMenuEntries for IUnknown punk=%p, nPos=%d, idFirst=%d, idLast=%d\n", punk);

    // TLMENUF_BACK - include back entries
    // TLMENUF_INCLUDECURRENT - include current entry, if TLMENUF_CHECKCURRENT then check the entry
    // TLMENUF_FORE - include next entries
    // if fore+back, list from oldest to newest
    // if back, list from newest to oldest
    // if fore, list from newest to oldest

    // don't forget to patch ampersands before adding to menu
    if (punk == NULL)
        return E_INVALIDARG;
    if (idLast <= idFirst)
        return E_INVALIDARG;
    menuItemInfo.cbSize = sizeof(menuItemInfo);
    menuItemInfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING;
    menuItemInfo.fType = MFT_STRING;
    menuItemInfo.wID = idFirst;
    menuItemInfo.fState = MFS_ENABLED;
    menuItemInfo.dwTypeData = itemTextBuffer;
    if ((dwFlags & TLMENUF_BACK) != 0)
    {
        if ((dwFlags & TLMENUF_FORE) != 0)
        {
            currentItem = fCurrentEntry;
            if (currentItem != NULL)
            {
                while (currentItem->fPreviousEntry != NULL)
                    currentItem = currentItem->fPreviousEntry;
            }
            while (currentItem != fCurrentEntry)
            {
                hResult = currentItem->GetToolTipText(punk, itemTextBuffer);
                if (SUCCEEDED(hResult))
                {
                    FixAmpersands(itemTextBuffer);
                    TRACE("CTravelLog::InsertMenuEntries adding entry L\"%S\"/L\"%S\" with id %d\n", itemTextBuffer, menuItemInfo.dwTypeData, menuItemInfo.wID);
                    if (InsertMenuItem(hmenu, nPos, TRUE, &menuItemInfo))
                    {
                        nPos++;
                        menuItemInfo.wID++;
                    }
                }
                currentItem = currentItem->fNextEntry;
            }
        }
        else
        {
            currentItem = fCurrentEntry;
            if (currentItem != NULL)
                currentItem = currentItem->fPreviousEntry;
            while (currentItem != NULL)
            {
                hResult = currentItem->GetToolTipText(punk, itemTextBuffer);
                if (SUCCEEDED(hResult))
                {
                    FixAmpersands(itemTextBuffer);
                    TRACE("CTravelLog::InsertMenuEntries adding entry L\"%S\"/L\"%S\" with id %d\n", itemTextBuffer, menuItemInfo.dwTypeData, menuItemInfo.wID);
                    if (InsertMenuItem(hmenu, nPos, TRUE, &menuItemInfo))
                    {
                        nPos++;
                        menuItemInfo.wID++;
                    }
                }
                currentItem = currentItem->fPreviousEntry;
            }
        }
    }
    if ((dwFlags & TLMENUF_INCLUDECURRENT) != 0)
    {
        if (fCurrentEntry != NULL)
        {
            hResult = fCurrentEntry->GetToolTipText(punk, itemTextBuffer);
            if (SUCCEEDED(hResult))
            {
                FixAmpersands(itemTextBuffer);
                if ((dwFlags & TLMENUF_CHECKCURRENT) != 0)
                    menuItemInfo.fState |= MFS_CHECKED;
                TRACE("CTravelLog::InsertMenuEntries adding entry L\"%S\"/L\"%S\" with id %d\n", itemTextBuffer, menuItemInfo.dwTypeData, menuItemInfo.wID);
                if (InsertMenuItem(hmenu, nPos, TRUE, &menuItemInfo))
                {
                    nPos++;
                    menuItemInfo.wID++;
                }
                menuItemInfo.fState &= ~MFS_CHECKED;
            }
        }
    }
    if ((dwFlags & TLMENUF_FORE) != 0)
    {
        currentItem = fCurrentEntry;
        if (currentItem != NULL)
            currentItem = currentItem->fNextEntry;
        while (currentItem != NULL)
        {
            hResult = currentItem->GetToolTipText(punk, itemTextBuffer);
            if (SUCCEEDED(hResult))
            {
                FixAmpersands(itemTextBuffer);
                TRACE("CTravelLog::InsertMenuEntries adding entry L\"%S\"/L\"%S\" with id %d\n", itemTextBuffer, menuItemInfo.dwTypeData, menuItemInfo.wID);
                if (InsertMenuItem(hmenu, nPos, TRUE, &menuItemInfo))
                {
                    nPos++;
                    menuItemInfo.wID++;
                }
            }
            currentItem = currentItem->fNextEntry;
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CTravelLog::Clone(ITravelLog **pptl)
{
    if (pptl == NULL)
        return E_POINTER;
    *pptl = NULL;
    // duplicate the log
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

DWORD STDMETHODCALLTYPE CTravelLog::CountEntries(IUnknown *punk)
{
    if (punk == NULL)
        return E_INVALIDARG;
    return fEntryCount;
}

HRESULT STDMETHODCALLTYPE CTravelLog::Revert()
{
    // remove the current entry?
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT CTravelLog_CreateInstance(REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CTravelLog>(riid, ppv);
}
