/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020-2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "shelldesktop.h"
#include "CDirectoryList.h"
#include <atlstr.h>

WINE_DEFAULT_DEBUG_CHANNEL(shcn);

///////////////////////////////////////////////////////////////////////////////////////
// File-system path iterator

class CFSPathIterator
{
public:
    CStringW m_strFullName;
    INT m_ich;

    CFSPathIterator(CStringW strFullName) : m_strFullName(strFullName), m_ich(0)
    {
    }

    bool Next(CStringW& strNext);
};

///////////////////////////////////////////////////////////////////////////////////////

bool CFSPathIterator::Next(CStringW& strNext)
{
    if (m_ich >= m_strFullName.GetLength())
        return false;

    auto ich = m_strFullName.Find(L'\\', m_ich);
    if (ich < 0)
    {
        ich = m_strFullName.GetLength();
        strNext = m_strFullName.Mid(m_ich, ich - m_ich);
        m_ich = ich;
    }
    else
    {
        strNext = m_strFullName.Mid(m_ich, ich - m_ich);
        m_ich = ich + 1;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////
// File-system node

class CFSNode
{
public:
    CStringW m_strName;

    CFSNode(const CStringW& strName, CFSNode* pParent = NULL);
    ~CFSNode();

    CStringW GetFullName();
    CFSNode* BuildPath(const CStringW& strFullName, BOOL bMarkNotExpanded = TRUE);

    CFSNode* FindChild(const CStringW& strName);
    CFSNode* Find(const CStringW& strFullName);

    BOOL RemoveChild(CFSNode *pNode);
    BOOL Remove();

    void MarkNotExpanded();

    void Expand();
    void clear();

protected:
    BOOL m_bExpand;
    CFSNode* m_pParent;
    CSimpleArray<CFSNode*> m_children;
};

///////////////////////////////////////////////////////////////////////////////////////

CFSNode::CFSNode(const CStringW& strName, CFSNode* pParent)
    : m_strName(strName)
    , m_bExpand(FALSE)
    , m_pParent(pParent)
{
}

CFSNode::~CFSNode()
{
    clear();
}

CStringW CFSNode::GetFullName()
{
    CStringW ret;
    if (m_pParent)
        ret = m_pParent->GetFullName();
    if (ret.GetLength())
        ret += L'\\';
    ret += m_strName;
    return ret;
}

CFSNode* CFSNode::FindChild(const CStringW& strName)
{
    for (INT iItem = 0; iItem < m_children.GetSize(); ++iItem)
    {
        auto pChild = m_children[iItem];
        if (pChild &&
            pChild->m_strName.GetLength() == strName.GetLength() &&
            lstrcmpiW(pChild->m_strName, strName) == 0)
        {
            return pChild;
        }
    }
    return NULL;
}

BOOL CFSNode::RemoveChild(CFSNode *pNode)
{
    for (INT iItem = 0; iItem < m_children.GetSize(); ++iItem)
    {
        auto& pChild = m_children[iItem];
        if (pChild == pNode)
        {
            auto pOld = pChild;
            pChild = NULL;
            delete pOld;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CFSNode::Remove()
{
    if (m_pParent)
        return m_pParent->RemoveChild(this);
    return FALSE;
}

CFSNode* CFSNode::Find(const CStringW& strFullName)
{
    CFSPathIterator it(strFullName);
    CStringW strName;
    CFSNode *pChild, *pNode;
    for (pNode = this; it.Next(strName); pNode = pChild)
    {
        pChild = pNode->FindChild(strName);
        if (!pChild)
            return NULL;
    }
    return pNode;
}

void CFSNode::MarkNotExpanded()
{
    for (auto pNode = this; pNode; pNode = pNode->m_pParent)
        pNode->m_bExpand = FALSE;
}

CFSNode* CFSNode::BuildPath(const CStringW& strFullName, BOOL bMarkNotExpanded)
{
    CFSPathIterator it(strFullName);
    CStringW strName;
    CFSNode *pNode, *pChild = NULL;
    for (pNode = this; it.Next(strName); pNode = pChild)
    {
        pChild = pNode->FindChild(strName);
        if (pChild)
            continue;

        pChild = new CFSNode(strName, pNode);
        pNode->m_children.Add(pChild);
        if (bMarkNotExpanded)
            pNode->MarkNotExpanded();
    }
    return pNode;
}

void CFSNode::Expand()
{
    if (m_bExpand)
        return;

    auto strSpec = GetFullName();
    strSpec += L"\\*";

    WIN32_FIND_DATAW find;
    HANDLE hFind = ::FindFirstFileW(strSpec, &find);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (lstrcmpW(find.cFileName, L".") == 0 ||
            lstrcmpW(find.cFileName, L"..") == 0 ||
            !(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            (find.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
        {
            continue;
        }

        auto pNode = FindChild(find.cFileName);
        if (!pNode)
        {
            pNode = new CFSNode(find.cFileName, this);
            m_children.Add(pNode);
        }
        pNode->Expand();
    } while (::FindNextFileW(hFind, &find));
    ::FindClose(hFind);

    m_bExpand = TRUE;
}

void CFSNode::clear()
{
    for (INT iItem = 0; iItem < m_children.GetSize(); ++iItem)
    {
        auto& pChild = m_children[iItem];
        delete pChild;
        pChild = NULL;
    }
    m_children.RemoveAll();
}

///////////////////////////////////////////////////////////////////////////////////////
// CDirectoryList

CDirectoryList::CDirectoryList(CFSNode *pRoot)
    : m_pRoot(pRoot ? pRoot : (new CFSNode(L"")))
    , m_fRecursive(FALSE)
{
}

CDirectoryList::CDirectoryList(CFSNode *pRoot, LPCWSTR pszDirectoryPath, BOOL fRecursive)
    : m_pRoot(pRoot ? pRoot : (new CFSNode(L"")))
    , m_fRecursive(fRecursive)
{
    AddPathsFromDirectory(pszDirectoryPath);
}

CDirectoryList::~CDirectoryList()
{
    delete m_pRoot;
}

BOOL CDirectoryList::ContainsPath(LPCWSTR pszPath) const
{
    ATLASSERT(!PathIsRelativeW(pszPath));

    return !!m_pRoot->Find(pszPath);
}

BOOL CDirectoryList::AddPath(LPCWSTR pszPath)
{
    ATLASSERT(!PathIsRelativeW(pszPath));

    auto pNode = m_pRoot->BuildPath(pszPath);
    if (pNode && m_fRecursive)
        pNode->Expand();

    return TRUE;
}

BOOL CDirectoryList::RenamePath(LPCWSTR pszPath1, LPCWSTR pszPath2)
{
    ATLASSERT(!PathIsRelativeW(pszPath1));
    ATLASSERT(!PathIsRelativeW(pszPath2));

    auto pNode = m_pRoot->Find(pszPath1);
    if (!pNode)
        return FALSE;

    LPWSTR pch = wcsrchr(pszPath2, L'\\');
    if (!pch)
        return FALSE;

    pNode->m_strName = pch + 1;
    return TRUE;
}

BOOL CDirectoryList::DeletePath(LPCWSTR pszPath)
{
    ATLASSERT(!PathIsRelativeW(pszPath));

    auto pNode = m_pRoot->Find(pszPath);
    if (!pNode)
        return FALSE;

    pNode->Remove();
    return TRUE;
}

BOOL CDirectoryList::AddPathsFromDirectory(LPCWSTR pszDirectoryPath)
{
    ATLASSERT(!PathIsRelativeW(pszPath));

    auto pNode = m_pRoot->BuildPath(pszDirectoryPath);
    if (pNode)
        pNode->Expand();

    return TRUE;
}
