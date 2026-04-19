/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero COM interface
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicbase.h"

template <typename T>
class CicInterface_RefCnt
{
protected:
    T* m_ptr;

public:
    CicInterface_RefCnt(T* ptr = NULL) : m_ptr(ptr)
    {
        if (m_ptr)
            m_ptr->AddRef();
    }
    ~CicInterface_RefCnt()
    {
        if (m_ptr)
            m_ptr->Release();
    }

    operator T*() { return m_ptr; }
    T* operator->() { return m_ptr; }
    T** operator&() { return &m_ptr; }

private:
    CicInterface_RefCnt(const CicInterface_RefCnt<T>&) = delete;
    CicInterface_RefCnt<T>&operator=(const CicInterface_RefCnt<T>&) = delete;
};
