/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Retrieve / store information about a snapin
 * COPYRIGHT:   Copyright 2017-2019 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#pragma once


// https://web.archive.org/web/20010614160959/msdn.microsoft.com/library/psdk/mmc/mmc12apx02_8jc5.htm
// https://msdn.microsoft.com/en-us/library/aa814846(v=vs.85).aspx


// https://msdn.microsoft.com/en-us/library/aa815510(v=vs.85).aspx
// MMC initializes a snap-in by calling the Initialize method of the IComponentData object.
// During initialization, IComponentData should query the console for its IConsoleNamespace and IConsole interfaces using
// the console's IUnknown interface pointer that is passed into the call to Initialize.
// The snap-in should then cache the returned pointers and use them for calling IConsoleNamespace and IConsole interface methods.


class CSnapin
{
private:
    CSnapinCacheEntry *m_CacheEntry;
    CAtlString m_DisplayName;

public:
    CSnapin(CSnapinCacheEntry *CacheEntry, PWSTR displayName = NULL);
    ~CSnapin();

    const CAtlString& Name() const;
    const CAtlString& Provider() const;
    const CAtlString& Description() const;
    const CAtlString& Version() const;
    const CAtlString& DisplayName() const;

    CSnapinCacheEntry *GetCacheEntry();

    void OnAdd(IConsole* console);
    void OnAccept(IConsole* console);
};
