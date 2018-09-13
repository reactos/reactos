// Implementation of class CFileNode
//
// During parsing an inf file, information about each file
// is stored in an instance of this class.  Such information
// includes the name of the file, its section in the inf file,
// its location (directory), etc.

#include "filenode.h"

CPNode::CPNode(LPCTSTR szName)
{
    Assert (szName != NULL);
    lstrcpyn(m_szName, szName, MAX_PATH);
    m_pNext = NULL;
    m_bRemovable = FALSE;
}

CPNode::~CPNode()
{
    if (m_pNext != NULL)
        delete m_pNext;
}

// insert a new file node into list
//HRESULT CFileNode::Insert(LPCTSTR szName, LPCTSTR szSection)
HRESULT CPNode::Insert(CPNode* pNewNode)
{
    if (pNewNode == NULL)
        return HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);

    m_pNext = pNewNode;
    return S_OK;
}

// get the file node placed right after this one in list
CPNode* CPNode::GetNext() const
{
    return m_pNext;
}

// tell the path in which this file is located
HRESULT CPNode::SetStr(LPTSTR lpszMember, LPCTSTR lpszNew )
{
    Assert (lpszNew != NULL);
    if (lpszNew == NULL)
        return HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);

    lstrcpyn(lpszMember, lpszNew, MAX_PATH); // all our string members are MAX_PATH
    return S_OK;
}

// retrieve the name of the file represented by this node
LPCTSTR CPNode::GetName() const
{
    return m_szName;
}


// retrieve the path of the file represented by this node
LPCTSTR CPNode::GetPath() const
{
    return (m_szPath[0] == '\0' ? NULL : m_szPath);
}

// constructor
CPackageNode::CPackageNode(LPCTSTR szName, LPCTSTR szNamespace, LPCTSTR szPath) : CPNode(szName)
{
    Assert (szNamespace != NULL);
    lstrcpyn(m_szName, szName, MAX_PATH);
    lstrcpyn(m_szNamespace, szNamespace, MAX_PATH);
    if (szPath != NULL)
    {
        lstrcpyn(m_szPath, szPath, MAX_PATH);
    }
    else
    {
        m_szPath[0] = '\0';
    }
    m_pNext = NULL;
    m_fIsSystemClass = FALSE;
}

// destructor
CPackageNode::~CPackageNode()
{
}

// retrieve the name of the section in the inf file which
// which the file represented by this node was installed
LPCTSTR CPackageNode::GetNamespace() const
{
    return m_szNamespace;
}

// constructor
CFileNode::CFileNode(LPCTSTR szName, LPCTSTR szSection, LPCTSTR szPath) : CPNode(szName)
{
    Assert (szSection != NULL);
    lstrcpyn(m_szName, szName, MAX_PATH);
    lstrcpyn(m_szSection, szSection, MAX_PATH);
    if (szPath != NULL)
    {
        lstrcpyn(m_szPath, szPath, MAX_PATH);
    }
    else
    {
        m_szPath[0] = '\0';
    }
    m_pNext = NULL;
}

// destructor
CFileNode::~CFileNode()
{
}



// retrieve the name of the section in the inf file which
// which the file represented by this node was installed
LPCTSTR CFileNode::GetSection() const
{
    return m_szSection;
}

