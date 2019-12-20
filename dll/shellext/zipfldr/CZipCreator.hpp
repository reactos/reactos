/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Create a zip file
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#ifndef CZIPCREATOR_HPP_
#define CZIPCREATOR_HPP_

struct CZipCreatorImpl;

class CZipCreator
{
public:
    struct CZipCreatorImpl *m_pimpl;

    virtual ~CZipCreator();

    static CZipCreator* DoCreate()
    {
        return new CZipCreator();
    }

    virtual void DoAddItem(LPCWSTR pszFile);
    static BOOL runThread(CZipCreator* pCreator);

protected:
    CZipCreator();
};

#endif
