// routines for managing the icon cache tables, and file type tables.
// Jan 95, ToddLa
//
//  icon cache
//
//      the icon cache is two ImageLists (g_himlIcons and g_himlIconsSmall)
//      and a table mapping a name/icon number/flags to a ImageList
//      index, the global hash table (pht==NULL) is used to hold
//      the names.
//
//          AddToIconTable      - associate a name/number/flags with a image index
//          SHLookupIconIndex   - return a image index, given name/number/flags
//          RemoveFromIconTable - remove all entries with the given name
//          FlushIconCache      - remove all entries.
//          GetFreeImageIndex   - return a free ImageList index.
//
//      the worst part about the whole icon cache design is that people
//      can add or lookup a image index (given a name/number/flags) but
//      they never have to release it.  we never know if a ImageList index
//      is currently in use or not.  this should be the first thing
//      fixed about the shell.  currently we use a MRU type scheme when
//      we need to remove a entry from the icon cache, it is far from
//      perfect.
//
//  file type cache
//
//      the file type cache is a hash table with two DWORDs of extra data.
//      DWORD #0 holds flags, DWORD #1 holds a pointer to the name of
//      the class.
//
//          LookupFileClass     - given a file class (ie ".doc" or "Directory")
//                                maps it to a DWORD of flags, return 0 if not found.
//
//          AddFileClass        - adds a class (and flags) to cache
//
//          LookupFileClassName - given a file class, returns it name.
//          AddFileClassName    - sets the name of a class.
//          FlushFileClass      - removes all items in cache.
//

#include "shellprv.h"
#pragma  hdrstop

#include "fstreex.h"
#include <ntverp.h>
#include "ovrlaymn.h"

extern int g_ccIcon;

TIMEVAR(LookupFileClass);
TIMEVAR(AddFileClass);

TIMEVAR(LookupFileClassName);
TIMEVAR(AddFileClassName);

TIMEVAR(LookupIcon);
TIMEVAR(RemoveIcon);
TIMEVAR(AddIcon);
TIMEVAR(IconFlush);

TCHAR const c_szIconCacheFile[] = TEXT("ShellIconCache");

#pragma data_seg(DATASEG_SHARED)

DWORD IconTimeBase     = ICONTIME_ZERO;
DWORD IconTimeFlush    = ICONTIME_ZERO;
DWORD FreeImageCount   = 0;
DWORD FreeEntryCount   = 0;

HDSA g_hdsaIcons = NULL;
BOOL g_DirtyIcons = FALSE;
UINT g_iLastSysIcon = 0;

#pragma data_seg()

LOCATION_ENTRY *_LookupIcon(LPCTSTR szName, int iIconIndex, UINT uFlags)
{
    LOCATION_ENTRY *p=NULL;
    int i,n;

    ASSERTCRITICAL
    ASSERT(szName == PathFindFileName(szName));

    szName = FindHashItem(NULL, szName);

    if (szName && g_hdsaIcons && (n = DSA_GetItemCount(g_hdsaIcons)) > 0)
    {
        for (i=0,p=DSA_GetItemPtr(g_hdsaIcons, 0); i<n; i++,p++)
        {
            if ((p->szName == szName) &&
                ((UINT)(p->uFlags&GIL_COMPARE) == (uFlags&GIL_COMPARE)) &&
                (p->iIconIndex == iIconIndex))
            {
                p->Access = GetIconTime();
                goto exit;
            }
        }

        p = NULL;       // not found
    }

exit:
    return p;
}


int LookupIconIndex(LPCTSTR pszName, int iIconIndex, UINT uFlags)
{
    PLOCATION_ENTRY p;
    int iIndex=-1;

    ASSERT(IS_VALID_STRING_PTR(pszName, -1));
    ASSERT(pszName == PathFindFileName(pszName));

    ENTERCRITICAL;
    TIMESTART(LookupIcon);

    if (NULL != (p = _LookupIcon(pszName, iIconIndex, uFlags)))
        iIndex = p->iIndex;

    TIMESTOP(LookupIcon);
    LEAVECRITICAL;

    return iIndex;
}


STDAPI_(int) SHLookupIconIndex(LPCTSTR pszName, int iIconIndex, UINT uFlags)
{
    return LookupIconIndex(pszName, iIconIndex, uFlags);
}


#ifdef UNICODE

STDAPI_(int) SHLookupIconIndexA(LPCSTR pszName, int iIconIndex, UINT uFlags)
{
    WCHAR wsz[MAX_PATH];

    SHAnsiToUnicode(pszName, wsz, ARRAYSIZE(wsz));
    return SHLookupIconIndex(wsz, iIconIndex, uFlags);
}    

#else

STDAPI_(int) SHLookupIconIndexW(LPCWSTR pszName, int iIconIndex, UINT uFlags)
{
    char sz[MAX_PATH];
    
    SHUnicodeToAnsi(pszName, sz, ARRAYSIZE(sz));
    return SHLookupIconIndex(sz, iIconIndex, uFlags);
}    

#endif


//
//  GetFreeImageIndex()
//
//      returns a free image index, or -1 if none
//
int GetFreeImageIndex(void)
{
    PLOCATION_ENTRY p=NULL;
    int i,n;
    int iIndex=-1;

    ASSERTCRITICAL

    if (FreeImageCount && g_hdsaIcons && (n = DSA_GetItemCount(g_hdsaIcons)) > 0)
    {
        for (i=0,p=DSA_GetItemPtr(g_hdsaIcons, 0); i<n; i++,p++)
        {
            if (p->szName == NULL && p->iIndex != 0)
            {
                iIndex = p->iIndex;     // get free index
                p->iIndex=0;            // claim it.
                p->Access=ICONTIME_ZERO;// mark unused entry.
                FreeImageCount--;
                FreeEntryCount++;
                break;
            }
        }
    }

    return iIndex;
}

//
//  GetImageIndexUsage()
//
int GetImageIndexUsage(int iIndex)
{
    PLOCATION_ENTRY p=NULL;
    int i,n,usage=0;

    ASSERT(g_hdsaIcons);
    ASSERTCRITICAL

    if (g_hdsaIcons && (n = DSA_GetItemCount(g_hdsaIcons)) > 0)
    {
        for (i=0,p=DSA_GetItemPtr(g_hdsaIcons, 0); i<n; i++,p++)
        {
            if (p->iIndex == iIndex)
            {
                usage++;
            }
        }
    }

    return usage;
}

void _FreeEntry(LOCATION_ENTRY *p)
{
    ASSERTCRITICAL

    g_DirtyIcons = TRUE;        // we need to save now.

    ASSERT(p->szName);
    DeleteHashItem(NULL, p->szName);
    p->szName = 0;

    if (GetImageIndexUsage(p->iIndex) > 1)
    {
        FreeEntryCount++;
        p->iIndex = 0;              // unused entry
        p->Access=ICONTIME_ZERO;
    }
    else
    {
        FreeImageCount++;
        p->Access=ICONTIME_ZERO;
    }
}

//
//  GetFreeEntry()
//
LOCATION_ENTRY *GetFreeEntry(void)
{
    PLOCATION_ENTRY p;
    int i,n;

    ASSERTCRITICAL

    if (FreeEntryCount && g_hdsaIcons && (n = DSA_GetItemCount(g_hdsaIcons)) > 0)
    {
        for (i=0,p=DSA_GetItemPtr(g_hdsaIcons, 0); i<n; i++,p++)
        {
            if (p->szName == NULL && p->iIndex == 0)
            {
                FreeEntryCount--;
                return p;
            }
        }
    }

    return NULL;
}

//
//  AddToIconTable  - add a item the the cache
//
//      lpszIconFile    - filename to add
//      iIconIndex      - icon index in file.
//      uFlags          - flags
//                          GIL_SIMULATEDOC - this is a simulated doc icon
//                          GIL_NOTFILENAME - file is not a path/index that
//                                            ExtractIcon can deal with
//      iIndex          - image index to use.
//
//  returns:
//      image index for new entry.
//
//  notes:
//      if the item already exists it is replaced.
//
void AddToIconTable(LPCTSTR szName, int iIconIndex, UINT uFlags, int iIndex)
{
    LOCATION_ENTRY LocationEntry;
    LOCATION_ENTRY *p;

    szName = PathFindFileName(szName);

    ENTERCRITICAL;
    TIMESTART(AddIcon);

    if (g_hdsaIcons == NULL)
    {
        g_hdsaIcons = DSA_Create(SIZEOF(LOCATION_ENTRY), 8);
        FreeEntryCount = 0;
        FreeImageCount = 0;
        IconTimeBase   = 0;
        IconTimeFlush  = 0;

        if (g_hdsaIcons == NULL)
            goto exit;
    }

    g_DirtyIcons = TRUE;        // we need to save now.

    if (NULL != (p = _LookupIcon(szName, iIconIndex, uFlags)))
    {
        if (p->iIndex == iIndex)
        {
            TraceMsg(TF_IMAGE, "IconCache: adding %s;%d (%d) to cache again", szName, iIconIndex, iIndex);
            goto exit;
        }

        TraceMsg(TF_IMAGE, "IconCache: re-adding %s;%d (%d) to cache", szName, iIconIndex, iIndex);
        _FreeEntry(p);
    }

    szName = AddHashItem(NULL, szName);
    ASSERT(szName);

    if (szName == NULL)
        goto exit;

    LocationEntry.szName = szName;
    LocationEntry.iIconIndex = iIconIndex;
    LocationEntry.iIndex = iIndex;
    LocationEntry.uFlags = uFlags;
    LocationEntry.Access = GetIconTime();
    
    if (NULL != (p = GetFreeEntry()))
        *p = LocationEntry;
    else
        DSA_AppendItem(g_hdsaIcons, &LocationEntry);

exit:
    TIMESTOP(AddIcon);
    LEAVECRITICAL;
    return;
}

void RemoveFromIconTable(LPCTSTR szName)
{
    PLOCATION_ENTRY p;

    UINT i,n;

    ENTERCRITICAL;
    TIMESTART(RemoveIcon);

    ASSERT(szName == PathFindFileName(szName));
    szName = FindHashItem(NULL, szName);

    if (szName && g_hdsaIcons && (n = DSA_GetItemCount(g_hdsaIcons)) > 0)
    {
        TraceMsg(TF_IMAGE, "IconCache: flush %s", szName);

        for (i=0,p=DSA_GetItemPtr(g_hdsaIcons, 0); i<n; i++,p++)
        {
            if (p->szName == szName && i > g_iLastSysIcon)
            {
                _FreeEntry(p);
            }
        }
    }

    TIMESTOP(RemoveIcon);
    LEAVECRITICAL;
    return;
}
//
// empties the icon cache
//
void FlushIconCache(void)
{

    ENTERCRITICAL;

    if (g_hdsaIcons != NULL)
    {
        PLOCATION_ENTRY p;
        int i,n;

        TraceMsg(TF_IMAGE, "IconCache: flush all");

        n = DSA_GetItemCount(g_hdsaIcons);

        for (i=0,p=DSA_GetItemPtr(g_hdsaIcons, 0); i<n; i++,p++)
        {
            if (p->szName)
                DeleteHashItem(NULL, p->szName);
        }

        DSA_DeleteAllItems(g_hdsaIcons);
        FreeEntryCount = 0;
        FreeImageCount = 0;
        IconTimeBase   = 0;
        IconTimeFlush  = 0;
        g_DirtyIcons   = TRUE;        // we need to save now.
    }

    LEAVECRITICAL;
}

//
// if the icon cache is too big get rid of some old items.
//
// remember FlushIconCache() removes *all* items from the
// icon table, and this function gets rid of *some* old items.
//
void _IconCacheFlush(BOOL fForce)
{
    DWORD dt;
    PLOCATION_ENTRY p;
    UINT i,n;
    int nuked=0;
    int active;

    ENTERCRITICAL;

    if (g_hdsaIcons)
    {
        //
        // conpute the time from the last flush call
        //
        dt = GetIconTime() - IconTimeFlush;

        //
        // compute the number of "active" table entries.
        //
        active = DSA_GetItemCount(g_hdsaIcons) - FreeEntryCount - FreeImageCount;
        ASSERT(active >= 0);

        if (fForce || (dt > MIN_FLUSH && active >= g_MaxIcons))
        {
            TraceMsg(TF_IMAGE, "IconCacheFlush: removing all items older than %d", dt/2);

            n = DSA_GetItemCount(g_hdsaIcons);

            for (i=0,p=DSA_GetItemPtr(g_hdsaIcons, 0); i<n; i++,p++)
            {
                if (i <= g_iLastSysIcon)
                    continue;

                if (p->szName && p->Access < (IconTimeFlush + dt/2))
                {
                    nuked++;
                    _FreeEntry(p);
                }
            }

            if (nuked > 0)
            {
                IconTimeFlush = GetIconTime();
                g_DirtyIcons  = TRUE;        // we need to save now.
            }
        }
    }

    LEAVECRITICAL;

    if (nuked > 0)
    {
        TraceMsg(TF_IMAGE, "IconCacheFlush: got rid of %d items (of %d), sending notify...", nuked, active);
        FlushFileClass();
        SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_DWORD, (LPCVOID)-1, NULL);
    }
}

//----------------------------- dump icon ------------------------------
#ifdef DEBUG

void _IconCacheDump()
{
    int i, cItems;
    TCHAR szBuffer[MAX_PATH];

    ENTERCRITICAL;
    if (g_hdsaIcons && g_himlIcons &&
        IsFlagSet(g_dwDumpFlags, DF_ICONCACHE)) {
        cItems = DSA_GetItemCount(g_hdsaIcons);

        TraceMsg(TF_IMAGE, "Icon cache: %d icons  (%d free)", cItems, FreeEntryCount);
        TraceMsg(TF_IMAGE, "Icon cache: %d images (%d free)", ImageList_GetImageCount(g_himlIcons), FreeImageCount);

        for (i = 0; i < cItems; i++) {
            PLOCATION_ENTRY pLocEntry = DSA_GetItemPtr(g_hdsaIcons, i);

            if (pLocEntry->szName)
                GetHashItemName(NULL, pLocEntry->szName, szBuffer, ARRAYSIZE(szBuffer));
            else
                lstrcpy(szBuffer, TEXT("(free)"));

            TraceMsg(TF_ALWAYS, "%s;%d%s%s\timage=%d access=%d",
                (LPTSTR)szBuffer,
                pLocEntry->iIconIndex,
                ((pLocEntry->uFlags & GIL_SIMULATEDOC) ? TEXT(" doc"):TEXT("")),
                ((pLocEntry->uFlags & GIL_NOTFILENAME) ? TEXT(" not file"):TEXT("")),
                pLocEntry->iIndex, pLocEntry->Access);
        }
    }
    LEAVECRITICAL;
}

#endif


DWORD GetBuildNumber()
{
#ifdef USE_OS_VERSION
    OSVERSIONINFO ver = {SIZEOF(ver)};

    GetVersionEx(&ver);
    return ver.dwBuildNumber;
#else
    // Need to use DLL version as we are updating this dll plus others and
    // we need the cache to be invalidated as we may change the icons...
    return VER_PRODUCTVERSION_DW;
#endif
}

#ifdef _WIN64

//
//  ps        - stream to which to save
//  hda       - DSA of LOCATION_ENTRY structures
//  cle       - count of LOCATION_ENTRY32's to write
//
//  The structures are stored as LOCATION_ENTRY32 on disk.
//

HRESULT _IconCacheWriteLocations(IStream *ps, HDSA hdsa, int cle)
{
    HRESULT hres = E_OUTOFMEMORY;

    // Convert from LOCATION_ENTRY to LOCATION_ENTRY32, then write out
    // the LOCATION_ENTRY32 structures.

    LOCATION_ENTRY32 *rgle32 = LocalAlloc(LPTR, cle * sizeof(LOCATION_ENTRY32));
    if (rgle32)
    {
        LOCATION_ENTRY *rgle = DSA_GetItemPtr(hdsa, 0);
        int i;
        for (i = 0; i < cle; i++)
        {
            rgle32[i].iIconIndex = rgle[i].iIconIndex;
            rgle32[i].uFlags     = rgle[i].uFlags;
            rgle32[i].iIndex     = rgle[i].iIndex;
            rgle32[i].Access     = rgle[i].Access;
        }

        hres = ps->lpVtbl->Write(ps, rgle32, cle * SIZEOF(LOCATION_ENTRY32), 0);
        LocalFree(rgle32);
    }
    return hres;
}

#else

__inline HRESULT _IconCacheWriteLocations(IStream *ps, HDSA hdsa, int cle)
{
    // LOCATION_ENTRY and LOCATION_ENTRY32 are the same, so we can
    // read straight into the DSA data block
    COMPILETIME_ASSERT(sizeof(LOCATION_ENTRY) == sizeof(LOCATION_ENTRY32));
    return ps->lpVtbl->Write(ps, DSA_GetItemPtr(hdsa, 0), cle * SIZEOF(LOCATION_ENTRY), 0);
}
#endif

/*
** Save the icon cache.
*/
BOOL _IconCacheSave()
{
    int i;
    IC_HEAD ich;
    TCHAR szPath[MAX_PATH];
    IStream * ps;
    BOOL sts = FALSE;
    DWORD dwAttr;

    if (!IsMainShellProcess())
        return TRUE;
    //
    // no icon cache nothing to save
    //
    if (g_hdsaIcons == NULL)
    {
        TraceMsg(TF_IMAGE, "IconCacheSave: no cache to save.");
        return TRUE;
    }

    //
    // if the icon cache is not dirty no need to save anything
    //
    if (!g_DirtyIcons)
    {
        TraceMsg(TF_IMAGE, "IconCacheSave: no need to save cache not dirty.");
        return TRUE;
    }

    // if the icon cache is way too big dont save it.
    // reload g_MaxIcons in case the user set it before shutting down.
    //
    QueryNewMaxIcons();
    if ((UINT)DSA_GetItemCount(g_hdsaIcons) > (UINT)g_MaxIcons)
    {
        TraceMsg(TF_IMAGE, "IconCacheSave: cache is too big not saving");
        return TRUE;
    }

    GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
    PathAppend(szPath, c_szIconCacheFile);
    PathQualifyDef(szPath, NULL, 0);

    // Clear the hidden bit (and what the heck, readonly as well) around the write
    // open, since on NT hidden prevents writing

    dwAttr = GetFileAttributes(szPath);
    if (0xFFFFFFFF != dwAttr)
        SetFileAttributes(szPath, dwAttr & ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY));

    SHCreateStreamOnFile(szPath, STGM_CREATE | STGM_WRITE | STGM_SHARE_DENY_WRITE, &ps);

    // Restore the old file attributes

    if (0xFFFFFFFF != dwAttr)
        SetFileAttributes(szPath, dwAttr);

    if (!ps)
    {
        TraceMsg(TF_IMAGE, "IconCacheSave: can't create %s.", szPath);
        return FALSE;
    }

    ENTERCRITICAL;

    // write out header
    ich.cbSize    = 0;                  // start invalid
    ich.uMagic    = ICONCACHE_MAGIC;
    ich.uVersion  = ICONCACHE_VERSION;
    ich.uNumIcons = DSA_GetItemCount(g_hdsaIcons);
    ich.uColorRes = GetCurColorRes();
    ich.flags     = g_ccIcon;
    ich.dwBuild   = GetBuildNumber();
    ich.TimeSave  = GetIconTime();
    ich.TimeFlush = IconTimeFlush;
    ich.FreeImageCount = FreeImageCount;
    ich.FreeEntryCount = FreeEntryCount;

    ImageList_GetIconSize(g_himlIcons, &ich.cxIcon, &ich.cyIcon);
    ImageList_GetIconSize(g_himlIconsSmall, &ich.cxSmIcon, &ich.cySmIcon);

    //
    // BUGBUG:: This is the age old sledgehammer approach, but if it
    // keeps the war team from my office...
    // Basically: if we are in clean boot mode, don't save out out icon
    // cache as some of the items may be refering to disks that are not
    // available in clean boot mode and we end up with a blank piece of
    // paper.
    //
    if (GetSystemMetrics(SM_CLEANBOOT))
    {
        TraceMsg(TF_IMAGE, "IconCacheSave: clean boot not saving");
        ich.uNumIcons = 0;
    }

    if (FAILED(ps->lpVtbl->Write(ps, &ich, SIZEOF(ich), NULL)))
        goto ErrorExit;

    // write out entries (assumes all entries are contigious in memory)
    if (FAILED(_IconCacheWriteLocations(ps, g_hdsaIcons, ich.uNumIcons)))
        goto ErrorExit;

    // write out the path names
    for (i = 0; i < (int)ich.uNumIcons; i++)
    {
        TCHAR ach[MAX_PATH];
        LOCATION_ENTRY *p = DSA_GetItemPtr(g_hdsaIcons, i);

        if (p->szName)
            GetHashItemName(NULL, p->szName, ach, ARRAYSIZE(ach));
        else
            ach[0] = 0;

        if (FAILED(Stream_WriteString(ps, ach, TRUE)))
            goto ErrorExit;
    }

    // write out the imagelist of the icons
    if (!ImageList_Write(g_himlIcons, ps))
        goto ErrorExit;
    if (!ImageList_Write(g_himlIconsSmall, ps))
        goto ErrorExit;

    if (FAILED(ps->lpVtbl->Commit(ps, 0)))
        goto ErrorExit;

    // Now make it valid ie change the signature to be valid...
    if (FAILED(ps->lpVtbl->Seek(ps, g_li0, STREAM_SEEK_SET, NULL)))
        goto ErrorExit;

    ich.cbSize = SIZEOF(ich);
    if (FAILED(ps->lpVtbl->Write(ps, &ich, SIZEOF(ich), NULL)))
        goto ErrorExit;

    sts = TRUE;

ErrorExit:
    ps->lpVtbl->Release(ps);

    if (sts && ich.uNumIcons > 0)
    {
        g_DirtyIcons = FALSE;
        SetFileAttributes(szPath, FILE_ATTRIBUTE_HIDDEN);
        TraceMsg(TF_IMAGE, "IconCacheSave: saved to %s.", szPath);
    }
    else
    {
        // saving failed.  no use leaving a bad cache around...
        TraceMsg(TF_IMAGE, "IconCacheSave: cache not saved!");
        DeleteFile(szPath);
    }

    LEAVECRITICAL;

    return sts;
}

#ifdef _WIN64

//
//  ps        - stream from which to load
//  hda       - DSA of LOCATION_ENTRY structures
//  cle       - count of LOCATION_ENTRY32's to read
//
//  The structures are stored as LOCATION_ENTRY32 on disk.
//

HRESULT _IconCacheReadLocations(IStream *ps, HDSA hdsa, int cle)
{
    HRESULT hres = E_OUTOFMEMORY;

    // read into a scratch buffer, then convert
    // LOCATION_ENTRY32 into LOCATION_ENTRY.

    LOCATION_ENTRY32 *rgle32 = LocalAlloc(LPTR, cle * sizeof(LOCATION_ENTRY32));
    if (rgle32)
    {
        hres = ps->lpVtbl->Read(ps, rgle32, cle * SIZEOF(LOCATION_ENTRY32), 0);
        if (SUCCEEDED(hres))
        {
            LOCATION_ENTRY *rgle = DSA_GetItemPtr(hdsa, 0);
            int i;
            for (i = 0; i < cle; i++)
            {
                rgle[i].iIconIndex = rgle32[i].iIconIndex;
                rgle[i].uFlags     = rgle32[i].uFlags;
                rgle[i].iIndex     = rgle32[i].iIndex;
                rgle[i].Access     = rgle32[i].Access;
            }
        }
        LocalFree(rgle32);
    }
    return hres;
}

#else

__inline HRESULT _IconCacheReadLocations(IStream *ps, HDSA hdsa, int cle)
{
    // LOCATION_ENTRY and LOCATION_ENTRY32 are the same, so we can
    // read straight into the DSA data block
    COMPILETIME_ASSERT(sizeof(LOCATION_ENTRY) == sizeof(LOCATION_ENTRY32));
    return ps->lpVtbl->Read(ps, DSA_GetItemPtr(hdsa, 0), cle * SIZEOF(LOCATION_ENTRY), 0);
}
#endif

//
//  get the icon cache back from disk, it must be the requested size and
//  bitdepth or we will not use it.
//
BOOL _IconCacheRestore(int cxIcon, int cyIcon, int cxSmIcon, int cySmIcon, UINT flags)
{
    IStream *ps;
    TCHAR szPath[MAX_PATH];

    ASSERTCRITICAL;

    GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
    PathAppend(szPath, c_szIconCacheFile);
    PathQualifyDef(szPath, NULL, 0);

    if (GetSystemMetrics(SM_CLEANBOOT))
        return FALSE;

    if (SUCCEEDED(SHCreateStreamOnFile(szPath, STGM_READ, &ps)))
    {
        IC_HEAD ich;
        if (SUCCEEDED(ps->lpVtbl->Read(ps, &ich, SIZEOF(ich), NULL)))
        {
            if (ich.cbSize    == SIZEOF(ich) &&
                ich.uVersion  == ICONCACHE_VERSION &&
                ich.uMagic    == ICONCACHE_MAGIC &&
                ich.dwBuild   == GetBuildNumber() &&
                ich.cxIcon    == (DWORD)cxIcon &&
                ich.cyIcon    == (DWORD)cyIcon &&
                ich.cxSmIcon  == (DWORD)cxSmIcon &&
                ich.cySmIcon  == (DWORD)cySmIcon &&
                ich.flags     == (DWORD)flags)
            {
                HDSA hdsaTemp;
                UINT cres = GetCurColorRes();

                //
                // dont load a mono image list on a color device, and
                // dont load a color image list on a mono device, get it?
                //
                if (ich.uColorRes == 1 && cres != 1 ||
                    ich.uColorRes != 1 && cres == 1)
                {
                    TraceMsg(TF_IMAGE, "IconCacheRestore: mono/color depth is wrong");
                    hdsaTemp = NULL;
                }
                else if (ich.uNumIcons > (UINT)g_MaxIcons)
                {
                    TraceMsg(TF_IMAGE, "IconCacheRestore: icon cache is too big not loading it.");
                    hdsaTemp = NULL;
                }
                else
                {
                    hdsaTemp = DSA_Create(SIZEOF(LOCATION_ENTRY), 8);
                }

                // load the icon table
                if (hdsaTemp)
                {
                    LOCATION_ENTRY dummy;

                    // grow the array out so we can read data into it
                    if (DSA_SetItem(hdsaTemp, ich.uNumIcons - 1, &dummy))
                    {
                        ASSERT(DSA_GetItemCount(hdsaTemp) == (int)ich.uNumIcons);
                        if (SUCCEEDED(_IconCacheReadLocations(ps, hdsaTemp, ich.uNumIcons)))
                        {
                            HIMAGELIST himlTemp;
                            int i;
                            // read the paths, patching up the table with the hashitem info
                            for (i = 0; i < (int)ich.uNumIcons; i++)
                            {
                                PLOCATION_ENTRY pLocation = DSA_GetItemPtr(hdsaTemp, i);

                                if (SUCCEEDED(Stream_ReadString(ps, szPath, ARRAYSIZE(szPath), TRUE)) && szPath[0])
                                    pLocation->szName = AddHashItem(NULL, szPath);
                                else
                                    pLocation->szName = 0;
                            }

                            // restore the image lists
                            himlTemp = ImageList_Read(ps);
                            if (himlTemp)
                            {
                                int cx, cy;

                                // If we read the list from disk and it does not contain the
                                // parallel mirrored list while we are on a mirrored system,
                                // let's not use the cache in this case
                                // Example of this is ARA/HEB MUI on US W2k

                                if(IS_BIDI_LOCALIZED_SYSTEM() && !(ImageList_GetFlags(himlTemp) & ILC_MIRROR))
                                {
                                    ImageList_Destroy(himlTemp);
                                    DSA_DeleteAllItems(hdsaTemp);
                                    ps->lpVtbl->Release(ps);
                                    return FALSE;
                                }
                                ImageList_GetIconSize(himlTemp, &cx, &cy);
                                if (cx == cxIcon && cy == cyIcon)
                                {
                                    HIMAGELIST himlTempSmall = ImageList_Read(ps);
                                    if (himlTempSmall)
                                    {
                                        IShellIconOverlayManager *psiom;

                                        ps->lpVtbl->Release(ps);

                                        if (g_himlIcons)
                                            ImageList_Destroy(g_himlIcons);
                                        g_himlIcons = himlTemp;

                                        if (g_himlIconsSmall)
                                            ImageList_Destroy(g_himlIconsSmall);
                                        g_himlIconsSmall = himlTempSmall;

                                        if (g_hdsaIcons)
                                            DSA_DeleteAllItems(g_hdsaIcons);
                                        g_hdsaIcons = hdsaTemp;

                                        // init the icon overlay indices...
                                        if (SUCCEEDED(GetIconOverlayManager(&psiom)))
                                        {
                                            psiom->lpVtbl->RefreshOverlayImages(psiom, SIOM_OVERLAYINDEX | SIOM_ICONINDEX);
                                            psiom->lpVtbl->Release(psiom);
                                        }
                                            
                                        //
                                        // we want GetIconTime() to pick up
                                        // where it left off when we saved.
                                        //
                                        IconTimeBase   = 0;     // GetIconTime() uses IconTimeBase
                                        IconTimeBase   = ich.TimeSave - GetIconTime();
                                        IconTimeFlush  = ich.TimeFlush;
                                        FreeImageCount = ich.FreeImageCount;
                                        FreeEntryCount = ich.FreeEntryCount;
                                        g_DirtyIcons   = FALSE;

                                        TraceMsg(TF_IMAGE, "IconCacheRestore: loaded %s", szPath);
                                        return TRUE;        // success
                                    }
                                }
                                ImageList_Destroy(himlTemp);
                            }
                        }
                    }
                    DSA_DeleteAllItems(hdsaTemp);
                }
            }
            else
            {
                TraceMsg(TF_IMAGE, "IconCacheRestore: Icon cache header changed");
            }
        }
        ps->lpVtbl->Release(ps);
    }
    else
    {
        TraceMsg(TF_IMAGE, "IconCacheRestore: unable to open file.");
    }

    TraceMsg(TF_IMAGE, "IconCacheRestore: cache not restored!");

    return FALSE;
}


//------------------ file class table ------------------------

PHASHTABLE g_phtClass = NULL;

BOOL InitFileClassTable(void)
{
    ASSERTCRITICAL;

    if (!g_phtClass )
    {
        if (!g_phtClass)
            g_phtClass = CreateHashItemTable(0, 2*SIZEOF(DWORD_PTR), TRUE);
    }

    return BOOLIFY(g_phtClass);
}
        
    
void FlushFileClass(void)
{
    ENTERCRITICAL;

#ifdef DEBUG
    if (g_phtClass != NULL) {
        
        DebugMsg(DM_TRACE, TEXT("Flushing file class table"));
        TIMEOUT(LookupFileClass);
        TIMEOUT(AddFileClass);
        TIMEOUT(LookupFileClassName);
        TIMEOUT(AddFileClassName);
        TIMEOUT(LookupIcon);
        TIMEOUT(AddIcon);
        TIMEOUT(RemoveIcon);

        TIMEIN(LookupFileClass);
        TIMEIN(AddFileClass);
        TIMEIN(LookupFileClassName);
        TIMEIN(AddFileClassName);
        TIMEIN(LookupIcon);
        TIMEIN(AddIcon);
        TIMEIN(RemoveIcon);

        DumpHashItemTable(g_phtClass);
    }
#endif
    if (g_phtClass != NULL)
    {
        DestroyHashItemTable(g_phtClass);
        g_phtClass = NULL;
    }

    LEAVECRITICAL;
}


DWORD LookupFileClass(LPCTSTR pszClass)
{
    DWORD dw = 0;

    ENTERCRITICAL;
    TIMESTART(LookupFileClass);
    
    if (g_phtClass && (NULL != (pszClass = FindHashItem(g_phtClass, pszClass))))   
        dw = (DWORD) GetHashItemData(g_phtClass, pszClass, 0);

    TIMESTOP(LookupFileClass);
    LEAVECRITICAL;

    return dw;
}


void AddFileClass(LPCTSTR pszClass, DWORD dw)
{

    ENTERCRITICAL;
    TIMESTART(AddFileClass);

    //
    // create a hsa table to keep the file class info in.
    //
    //  DWORD #0 is the type flags
    //  DWORD #1 is the class name
    //
    if (InitFileClassTable() && (NULL != (pszClass = AddHashItem(g_phtClass, pszClass))))
        SetHashItemData(g_phtClass, pszClass, 0, dw);

    TIMESTOP(AddFileClass);
    LEAVECRITICAL;
    return;
}

LPCTSTR LookupFileClassName(LPCTSTR pszClass)
{
    LPCTSTR pszClassName=NULL;

    ASSERTCRITICAL
    TIMESTART(LookupFileClassName);

    if (g_phtClass && (NULL != (pszClass = FindHashItem(g_phtClass, pszClass))))
        pszClassName = (LPCTSTR)GetHashItemData(g_phtClass, pszClass, 1);

    TIMESTOP(LookupFileClassName);

    return pszClassName;
}

LPCTSTR AddFileClassName(LPCTSTR pszClass, LPCTSTR pszClassName)
{
    ASSERTCRITICAL
    TIMESTART(AddFileClassName);

    //
    // create a hsa table to keep the file class info in.
    //
    //  DWORD #0 is the type flags
    //  DWORD #1 is the class name
    //

    if (InitFileClassTable() && (NULL != (pszClass = AddHashItem(g_phtClass, pszClass)))) {
        pszClassName = AddHashItem(g_phtClass, pszClassName);
        SetHashItemData(g_phtClass, pszClass, 1, (DWORD_PTR)pszClassName);
    }

    TIMESTOP(AddFileClassName);
    return pszClassName;
}
