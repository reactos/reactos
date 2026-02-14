/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Create a zip file
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2019-2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

struct CZipCreatorImpl;

class CZipCreator
{
public:
    struct CZipCreatorImpl *m_pimpl;
    CComHeapPtr<ITEMIDLIST> m_pidlNotify;

    virtual ~CZipCreator();

    static CZipCreator* DoCreate(PCWSTR pszExistingZip = NULL, PCWSTR pszTargetDir = NULL);

    virtual void DoAddItem(PCWSTR pszFile);
    static BOOL runThread(CZipCreator* pCreator);

    void SetNotifyPidl(PCIDLIST_ABSOLUTE pidl)
    {
        if (pidl)
            m_pidlNotify.Attach(ILClone(pidl));
    }

protected:
    CZipCreator();
};
