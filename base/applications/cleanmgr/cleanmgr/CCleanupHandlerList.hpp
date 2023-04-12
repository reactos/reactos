/*
 * PROJECT:     ReactOS Disk Cleanup
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CCleanupHandlerList definition
 * COPYRIGHT:   Copyright 2023-2025 Mark Jansen <mark.jansen@reactos.org>
 */


class CCleanupHandlerList
{
private:
    CAtlList<CCleanupHandler *> m_Handlers;
    CStringW m_DriveStr;

public:

    void LoadHandlers(WCHAR Drive);
    DWORDLONG ScanDrive(IEmptyVolumeCacheCallBack* picb);
    void ExecuteCleanup(IEmptyVolumeCacheCallBack *picb);

    template<typename Fn>
    void ForEach(Fn callback)
    {
        for (POSITION it = m_Handlers.GetHeadPosition(); it; m_Handlers.GetNext(it))
        {
            CCleanupHandler *current = m_Handlers.GetAt(it);

            callback(current);
        }
    }
};
