/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero file mapping
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicmutex.h"

// class CicFileMappingStatic;
// class CicFileMapping;

class CicFileMappingStatic
{
protected:
    LPCTSTR m_pszName = NULL;
    LPVOID m_pView = NULL;
    HANDLE m_hMapping = NULL;
    BOOL m_bCreated = FALSE;
    BOOL m_bHasMutex = FALSE;
    CicMutex* m_pMutex = NULL;

    LPVOID _Map();

public:
    ~CicFileMappingStatic() { }

    void Init(LPCTSTR pszName, CicMutex *pMutex);

    LPVOID Create(LPSECURITY_ATTRIBUTES pSA, DWORD dwMaximumSizeLow, LPBOOL pbAlreadyExists);
    LPVOID Open();
    void Close();

    BOOL Enter();
    void Leave();
    BOOL Flush(SIZE_T dwNumberOfBytesToFlush);
    void Finalize();
};

class CicFileMapping : public CicFileMappingStatic
{
public:
    CicFileMapping(LPCTSTR pszName, CicMutex *pMutex);
    virtual ~CicFileMapping() { Finalize(); }
};
