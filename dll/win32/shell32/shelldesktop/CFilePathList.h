#pragma once

#include <atlsimpcoll.h> // for CSimpleArray

//////////////////////////////////////////////////////////////////////////////

// A pathname with info
class CFilePathItem
{
public:
    CFilePathItem() : m_pszPath(NULL)
    {
    }

    CFilePathItem(LPCWSTR pszPath, DWORD dwFileSize, BOOL IsDirectory)
    {
        m_pszPath = _wcsdup(pszPath);
        m_dwFileSize = dwFileSize;
        m_fIsDirectory = IsDirectory;
    }

    CFilePathItem(const CFilePathItem& item)
        : m_pszPath(_wcsdup(item.m_pszPath))
        , m_dwFileSize(item.m_dwFileSize)
        , m_fIsDirectory(item.m_fIsDirectory)
    {
    }

    CFilePathItem& operator=(const CFilePathItem& item)
    {
        free(m_pszPath);
        m_pszPath = _wcsdup(item.m_pszPath);
        m_dwFileSize = item.m_dwFileSize;
        m_fIsDirectory = item.m_fIsDirectory;
        return *this;
    }

    ~CFilePathItem()
    {
        free(m_pszPath);
    }

    BOOL IsEmpty() const
    {
        return m_pszPath == NULL;
    }

    BOOL IsDirectory() const
    {
        return m_fIsDirectory;
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

    DWORD GetSize() const
    {
        return m_dwFileSize;
    }

protected:
    LPWSTR m_pszPath;    // A full path, malloc'ed
    DWORD m_dwFileSize;  // the size of a file
    BOOL m_fIsDirectory; // is it a directory?
};

// the file list
class CFilePathList
{
public:
    CFilePathList()
    {
    }

    CFilePathList(LPCWSTR pszDirectoryPath, BOOL fRecursive)
    {
        AddPathsFromDirectory(pszDirectoryPath, fRecursive);
    }

    BOOL ContainsPath(LPCWSTR pszPath, BOOL fIsDirectory) const;
    BOOL GetFirstChange(LPWSTR pszPath) const;

    BOOL AddPath(LPCWSTR pszPath, DWORD dwFileSize, BOOL fIsDirectory);
    BOOL AddPathsFromDirectory(LPCWSTR pszDirectoryPath, BOOL fRecursive);
    BOOL RenamePath(LPCWSTR pszPath1, LPCWSTR pszPath2, BOOL fIsDirectory);
    BOOL DeletePath(LPCWSTR pszPath, BOOL fIsDirectory);

    void RemoveAll()
    {
        m_items.RemoveAll();
    }

protected:
    CSimpleArray<CFilePathItem> m_items;
};
