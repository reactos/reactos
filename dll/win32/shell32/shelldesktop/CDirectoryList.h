#pragma once

#include <atlsimpcoll.h> // for CSimpleArray

//////////////////////////////////////////////////////////////////////////////

// A pathname with info
class CDirectoryItem
{
public:
    CDirectoryItem() : m_pszPath(NULL)
    {
    }

    CDirectoryItem(LPCWSTR pszPath)
    {
        m_pszPath = _wcsdup(pszPath);
    }

    CDirectoryItem(const CDirectoryItem& item)
        : m_pszPath(_wcsdup(item.m_pszPath))
    {
    }

    CDirectoryItem& operator=(const CDirectoryItem& item)
    {
        if (this != &item)
        {
            free(m_pszPath);
            m_pszPath = _wcsdup(item.m_pszPath);
        }
        return *this;
    }

    ~CDirectoryItem()
    {
        free(m_pszPath);
    }

    BOOL IsEmpty() const
    {
        return m_pszPath == NULL;
    }

    LPCWSTR GetPath() const
    {
        return m_pszPath;
    }

    void SetPath(LPCWSTR pszPath)
    {
        free(m_pszPath);
        m_pszPath = _wcsdup(pszPath);
    }

    BOOL EqualPath(LPCWSTR pszPath) const
    {
        return m_pszPath != NULL && lstrcmpiW(m_pszPath, pszPath) == 0;
    }

protected:
    LPWSTR m_pszPath;    // A full path, malloc'ed
};

// the directory list
class CDirectoryList
{
public:
    CDirectoryList() : m_fRecursive(FALSE)
    {
    }

    CDirectoryList(LPCWSTR pszDirectoryPath, BOOL fRecursive)
        : m_fRecursive(fRecursive)
    {
        AddPathsFromDirectory(pszDirectoryPath);
    }

    BOOL ContainsPath(LPCWSTR pszPath) const;
    BOOL AddPath(LPCWSTR pszPath);
    BOOL AddPathsFromDirectory(LPCWSTR pszDirectoryPath);
    BOOL RenamePath(LPCWSTR pszPath1, LPCWSTR pszPath2);
    BOOL DeletePath(LPCWSTR pszPath);

    void RemoveAll()
    {
        m_items.RemoveAll();
    }

protected:
    BOOL m_fRecursive;
    CSimpleArray<CDirectoryItem> m_items;
};
