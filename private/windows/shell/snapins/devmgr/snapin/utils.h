#ifndef __UTILS__H
#define __UTILS__H
/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    utils.h

Abstract:

    This module declares utilities classes

Author:

    William Hsieh (williamh) created

Revision History:


--*/

//
// Memory allocation exception class
//
class CMemoryException
{
public:
    CMemoryException(BOOL Global)
    {
        m_Global = Global;
        m_Message[0] = _T('\0');
        m_Caption[0] = _T('\0');
        m_Options = MB_OK | MB_ICONHAND;
    }
    BOOL SetMessage(LPCTSTR Message)
    {
        if (!Message || lstrlen(Message) >= ARRAYLEN(m_Message))
        return FALSE;
        lstrcpy(m_Message, Message);
        return TRUE;

    }
    BOOL SetCaption(LPCTSTR Caption)
    {
        if (!Caption || lstrlen(Caption) >= ARRAYLEN(m_Caption))
        return FALSE;
        lstrcpy(m_Caption, Caption);
        return TRUE;
    }
    BOOL SetOptions(DWORD Options)
    {
        m_Options = Options;
        return TRUE;
    }
    void ReportError(HWND hwndParent = NULL)
    {
        MessageBox(hwndParent, m_Message, m_Caption, m_Options);
    }
    void Delete()
    {
        if (!m_Global)
        delete this;
    }
private:
    TCHAR m_Message[128];
    TCHAR m_Caption[128];
    DWORD m_Options;
    BOOL  m_Global;
};

inline int MAX(int Value1, int Value2)
{
    return (Value1 >= Value2) ? Value1 : Value2;
}

//
// data buffer control class for String class
//
class StringData
{
public:
    StringData() : Ref(1), ptsz(NULL), Len(0)
    {}
    ~StringData()
    {
        delete [] ptsz;
    }
    long AddRef()
    {
        Ref++;
        return Ref;
    }
    long Release()
    {
        ASSERT(Ref);
        if (!(--Ref))
        {
        delete this;
        return 0;
        }
        return Ref;
    }
    TCHAR*  ptsz;
    long    Len;
private:
    long    Ref;
};

class CBlock
{
public:
    CBlock(CBlock* BlockHead, UINT unitCount, UINT unitSize)
    {
        data = new BYTE[unitCount * unitSize];
        if (data)
        {
        if (BlockHead)
        {
            m_Next = BlockHead->m_Next;
            BlockHead->m_Next = this;
        }
        else
        {
            m_Next = NULL;
        }
        }
        else
        {
        throw &g_MemoryException;
        }
    }
    ~CBlock()
    {
        if (data)
        delete [] data;
        if (m_Next)
        delete m_Next;
    }
    void*   data;

private:
    CBlock* m_Next;
};


//
// Text string class
//
class String
{
public:
// constructors
    String();
    String(LPCTSTR lptsz);
    String(const String& strSrc);
    ~String()
    {
        m_pData->Release();
    }
//operators

    TCHAR& operator[](int Index);
    operator LPTSTR();

    const TCHAR& operator[](int Index) const
    {
        ASSERT(Index < m_pData->Len && m_pData->ptsz);
        return m_pData->ptsz[Index];
    }

    operator LPCTSTR () const
    {
        return m_pData->ptsz;
    }
    String& operator=(const String& strSrc);
    String& operator=(LPCTSTR ptsz);
    String& operator+=(const String& strSrc);
    String& operator+=(LPCTSTR prsz);
    friend String operator+(const String& str1, const String& str2);

    int GetLength() const
    {
        return m_pData->Len;
    }
    BOOL IsEmpty() const
    {
        return (0 == m_pData->Len);
    }
    int Compare(const String& strSrc) const
    {
        return lstrcmp(m_pData->ptsz, strSrc.m_pData->ptsz);
    }
    int CompareNoCase(const String& strSrc) const
    {
        return lstrcmpi(m_pData->ptsz, strSrc.m_pData->ptsz);
    }
    void Empty();
    BOOL LoadString(HINSTANCE hInstance, int ResourceId);
    BOOL GetComputerName();
    void Format(LPCTSTR FormatString, ...);
    StringData* m_pData;

protected:
    String(int Len);

};

//
// Command line parsing class
//
class CCommandLine
{
public:
    void ParseCommandLine(LPCTSTR cmdline);
    virtual void ParseParam(LPCTSTR Param, BOOL bFlag, BOOL bLast) = 0;
};




//
// Safe registry class
//
class CSafeRegistry
{
public:
    CSafeRegistry(HKEY hKey = NULL) : m_hKey(hKey)
    {}
    ~CSafeRegistry()
    {
        if (m_hKey)
        RegCloseKey(m_hKey);
#if 0
        if (RemoteConnected)
        {
        WNetCancelConnection2(TEXT("\\\\server\\ipc$", 0, FALSE);
        }
#endif
    }
    operator HKEY()
    {
        return m_hKey;
    }
    BOOL Open(HKEY hKeyAncestor, LPCTSTR KeyName, REGSAM Access = KEY_ALL_ACCESS);
    void Close()
    {
        if (m_hKey)
        RegCloseKey(m_hKey);
        m_hKey = NULL;
    }
    BOOL Create(HKEY hKeyAncestor, LPCTSTR KeyName,
             REGSAM Access = KEY_ALL_ACCESS,
             DWORD * pDisposition = NULL, DWORD  Options = 0,
             LPSECURITY_ATTRIBUTES pSecurity = NULL);
    BOOL SetValue(LPCTSTR ValueName, DWORD Type, PBYTE pData, DWORD DataLen);
    BOOL SetValue(LPCTSTR ValueName, LPCTSTR Value);
    BOOL GetValue(LPCTSTR ValueName, DWORD* pType, PBYTE Buffer, DWORD* BufferLen);
    BOOL GetValue(LPCTSTR ValueName, String& str);
    BOOL DeleteValue(LPCTSTR ValueName);
    BOOL DeleteSubkey(LPCTSTR SubkeyName);
    BOOL EnumerateSubkey(DWORD Index, LPTSTR Buffer, DWORD* BufferSize);
private:
    HKEY    m_hKey;
};

// define iteration context. To be used by CLIST
struct tagPosition{ };
typedef tagPosition* POSITION;

template<class TYPE>
inline void ConstructElements(TYPE* pElements, int Count)
{
    memset((void*)&pElements, 0, Count * sizeof(TYPE));
    for (; Count; Count--, pElements++)
    // call the contructor -- note the placement
    ::new((void*)pElements) TYPE;
}

template<class TYPE>
inline void DestructElements(TYPE* pElements, int Count)
{
    for (; Count; Count--, pElements++)
    pElements->~TYPE();
}
//
// TEMPLATEs
//


//
// CList template, adapted from MFC
//
template<class TYPE, class ARG_TYPE>
class CList
{
protected:
    struct CNode
    {
        CNode* pNext;
        CNode* pPrev;
        TYPE data;
    };
public:
// Construction
    CList(int nBlockSize = 10);

// Attributes (head and tail)
    // count of elements
    int GetCount() const;
    BOOL IsEmpty() const;

    // peek at head or tail
    TYPE& GetHead();
    TYPE GetHead() const;
    TYPE& GetTail();
    TYPE GetTail() const;

// Operations
    // get head or tail (and remove it) - don't call on empty list !
    TYPE RemoveHead();
    TYPE RemoveTail();

    // add before head or after tail
    POSITION AddHead(ARG_TYPE newElement);
    POSITION AddTail(ARG_TYPE newElement);

    // add another list of elements before head or after tail
    void AddHead(CList* pNewList);
    void AddTail(CList* pNewList);

    // remove all elements
    void RemoveAll();

    // iteration
    POSITION GetHeadPosition() const;
    POSITION GetTailPosition() const;
    TYPE& GetNext(POSITION& rPosition); // return *Position++
    TYPE GetNext(POSITION& rPosition) const; // return *Position++
    TYPE& GetPrev(POSITION& rPosition); // return *Position--
    TYPE GetPrev(POSITION& rPosition) const; // return *Position--

    // getting/modifying an element at a given position
    TYPE& GetAt(POSITION position);
    TYPE GetAt(POSITION position) const;
    void SetAt(POSITION pos, ARG_TYPE newElement);
    void RemoveAt(POSITION position);

    // inserting before or after a given position
    POSITION InsertBefore(POSITION position, ARG_TYPE newElement);
    POSITION InsertAfter(POSITION position, ARG_TYPE newElement);

    POSITION FindIndex(int nIndex) const;
        // get the 'nIndex'th element (may return NULL)

// Implementation
protected:
    CNode* m_pNodeHead;
    CNode* m_pNodeTail;
    int m_nCount;
    CNode* m_pNodeFree;
    CBlock* m_pBlocks;
    int m_nBlockSize;

    CNode* NewNode(CNode*, CNode*);
    void FreeNode(CNode*);

public:
    ~CList();
};


/////////////////////////////////////////////////////////////////////////////
// CList<TYPE, ARG_TYPE> inline functions

template<class TYPE, class ARG_TYPE>
inline int CList<TYPE, ARG_TYPE>::GetCount() const
    { return m_nCount; }
template<class TYPE, class ARG_TYPE>
inline BOOL CList<TYPE, ARG_TYPE>::IsEmpty() const
    { return m_nCount == 0; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CList<TYPE, ARG_TYPE>::GetHead()
    { ASSERT(m_pNodeHead != NULL);
        return m_pNodeHead->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CList<TYPE, ARG_TYPE>::GetHead() const
    { ASSERT(m_pNodeHead != NULL);
        return m_pNodeHead->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CList<TYPE, ARG_TYPE>::GetTail()
    { ASSERT(m_pNodeTail != NULL);
        return m_pNodeTail->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CList<TYPE, ARG_TYPE>::GetTail() const
    { ASSERT(m_pNodeTail != NULL);
        return m_pNodeTail->data; }
template<class TYPE, class ARG_TYPE>
inline POSITION CList<TYPE, ARG_TYPE>::GetHeadPosition() const
    { return (POSITION) m_pNodeHead; }
template<class TYPE, class ARG_TYPE>
inline POSITION CList<TYPE, ARG_TYPE>::GetTailPosition() const
    { return (POSITION) m_pNodeTail; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CList<TYPE, ARG_TYPE>::GetNext(POSITION& rPosition) // return *Position++
    { CNode* pNode = (CNode*) rPosition;
        rPosition = (POSITION) pNode->pNext;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CList<TYPE, ARG_TYPE>::GetNext(POSITION& rPosition) const // return *Position++
    { CNode* pNode = (CNode*) rPosition;
        rPosition = (POSITION) pNode->pNext;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CList<TYPE, ARG_TYPE>::GetPrev(POSITION& rPosition) // return *Position--
    { CNode* pNode = (CNode*) rPosition;
        rPosition = (POSITION) pNode->pPrev;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CList<TYPE, ARG_TYPE>::GetPrev(POSITION& rPosition) const // return *Position--
    { CNode* pNode = (CNode*) rPosition;
        rPosition = (POSITION) pNode->pPrev;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CList<TYPE, ARG_TYPE>::GetAt(POSITION position)
    { CNode* pNode = (CNode*) position;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CList<TYPE, ARG_TYPE>::GetAt(POSITION position) const
    { CNode* pNode = (CNode*) position;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline void CList<TYPE, ARG_TYPE>::SetAt(POSITION pos, ARG_TYPE newElement)
    { CNode* pNode = (CNode*) pos;
        pNode->data = newElement; }

template<class TYPE, class ARG_TYPE>
CList<TYPE, ARG_TYPE>::CList(int nBlockSize)
{
    ASSERT(nBlockSize > 0);

    m_nCount = 0;
    m_pNodeHead = m_pNodeTail = m_pNodeFree = NULL;
    m_pBlocks = NULL;
    m_nBlockSize = nBlockSize;
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::RemoveAll()
{
    // destroy elements
    CNode* pNode;
    for (pNode = m_pNodeHead; pNode != NULL; pNode = pNode->pNext)
        DestructElements<TYPE>(&pNode->data, 1);

    m_nCount = 0;
    m_pNodeHead = m_pNodeTail = m_pNodeFree = NULL;
    delete m_pBlocks;
    m_pBlocks = NULL;
}

template<class TYPE, class ARG_TYPE>
CList<TYPE, ARG_TYPE>::~CList()
{
    RemoveAll();
    ASSERT(m_nCount == 0);
}

/////////////////////////////////////////////////////////////////////////////
// Node helpers
//

template<class TYPE, class ARG_TYPE>
CList<TYPE, ARG_TYPE>::CNode*
CList<TYPE, ARG_TYPE>::NewNode(CList::CNode* pPrev, CList::CNode* pNext)
{
    if (m_pNodeFree == NULL)
    {
        // add another block
        CBlock* pNewBlock = new CBlock(m_pBlocks, m_nBlockSize,
                           sizeof(CNode));
                if (m_pBlocks == NULL)
                {
                    m_pBlocks = pNewBlock;
                }

        // chain them into free list
        CNode* pNode = (CNode*) pNewBlock->data;
        // free in reverse order to make it easier to debug
        pNode += m_nBlockSize - 1;
        for (int i = m_nBlockSize-1; i >= 0; i--, pNode--)
        {
            pNode->pNext = m_pNodeFree;
            m_pNodeFree = pNode;
        }
    }
    ASSERT(m_pNodeFree != NULL);  // we must have something

    CList::CNode* pNode = m_pNodeFree;
    m_pNodeFree = m_pNodeFree->pNext;
    pNode->pPrev = pPrev;
    pNode->pNext = pNext;
    m_nCount++;
    ASSERT(m_nCount > 0);  // make sure we don't overflow

    ConstructElements<TYPE>(&pNode->data, 1);
    return pNode;
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::FreeNode(CList::CNode* pNode)
{
    DestructElements<TYPE>(&pNode->data, 1);
    pNode->pNext = m_pNodeFree;
    m_pNodeFree = pNode;
    m_nCount--;
    ASSERT(m_nCount >= 0);  // make sure we don't underflow

    // if no more elements, cleanup completely
    if (m_nCount == 0)
        RemoveAll();
}

template<class TYPE, class ARG_TYPE>
POSITION CList<TYPE, ARG_TYPE>::AddHead(ARG_TYPE newElement)
{
    CNode* pNewNode = NewNode(NULL, m_pNodeHead);
    pNewNode->data = newElement;
    if (m_pNodeHead != NULL)
        m_pNodeHead->pPrev = pNewNode;
    else
        m_pNodeTail = pNewNode;
    m_pNodeHead = pNewNode;
    return (POSITION) pNewNode;
}

template<class TYPE, class ARG_TYPE>
POSITION CList<TYPE, ARG_TYPE>::AddTail(ARG_TYPE newElement)
{
    CNode* pNewNode = NewNode(m_pNodeTail, NULL);
    pNewNode->data = newElement;
    if (m_pNodeTail != NULL)
        m_pNodeTail->pNext = pNewNode;
    else
        m_pNodeHead = pNewNode;
    m_pNodeTail = pNewNode;
    return (POSITION) pNewNode;
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::AddHead(CList* pNewList)
{
    ASSERT(pNewList != NULL);

    // add a list of same elements to head (maintain order)
    POSITION pos = pNewList->GetTailPosition();
    while (pos != NULL)
        AddHead(pNewList->GetPrev(pos));
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::AddTail(CList* pNewList)
{
    ASSERT(pNewList != NULL);

    // add a list of same elements
    POSITION pos = pNewList->GetHeadPosition();
    while (pos != NULL)
        AddTail(pNewList->GetNext(pos));
}

template<class TYPE, class ARG_TYPE>
TYPE CList<TYPE, ARG_TYPE>::RemoveHead()
{
    ASSERT(m_pNodeHead != NULL);  // don't call on empty list !!!

    CNode* pOldNode = m_pNodeHead;
    TYPE returnValue = pOldNode->data;

    m_pNodeHead = pOldNode->pNext;
    if (m_pNodeHead != NULL)
        m_pNodeHead->pPrev = NULL;
    else
        m_pNodeTail = NULL;
    FreeNode(pOldNode);
    return returnValue;
}

template<class TYPE, class ARG_TYPE>
TYPE CList<TYPE, ARG_TYPE>::RemoveTail()
{
    ASSERT(m_pNodeTail != NULL);  // don't call on empty list !!!

    CNode* pOldNode = m_pNodeTail;
    TYPE returnValue = pOldNode->data;

    m_pNodeTail = pOldNode->pPrev;
    if (m_pNodeTail != NULL)
        m_pNodeTail->pNext = NULL;
    else
        m_pNodeHead = NULL;
    FreeNode(pOldNode);
    return returnValue;
}

template<class TYPE, class ARG_TYPE>
POSITION CList<TYPE, ARG_TYPE>::InsertBefore(POSITION position, ARG_TYPE newElement)
{

    if (position == NULL)
        return AddHead(newElement); // insert before nothing -> head of the list

    // Insert it before position
    CNode* pOldNode = (CNode*) position;
    CNode* pNewNode = NewNode(pOldNode->pPrev, pOldNode);
    pNewNode->data = newElement;

    if (pOldNode->pPrev != NULL)
    {
        pOldNode->pPrev->pNext = pNewNode;
    }
    else
    {
        ASSERT(pOldNode == m_pNodeHead);
        m_pNodeHead = pNewNode;
    }
    pOldNode->pPrev = pNewNode;
    return (POSITION) pNewNode;
}

template<class TYPE, class ARG_TYPE>
POSITION CList<TYPE, ARG_TYPE>::InsertAfter(POSITION position, ARG_TYPE newElement)
{

    if (position == NULL)
        return AddTail(newElement); // insert after nothing -> tail of the list

    // Insert it before position
    CNode* pOldNode = (CNode*) position;
    CNode* pNewNode = NewNode(pOldNode, pOldNode->pNext);
    pNewNode->data = newElement;

    if (pOldNode->pNext != NULL)
    {
        pOldNode->pNext->pPrev = pNewNode;
    }
    else
    {
        ASSERT(pOldNode == m_pNodeTail);
        m_pNodeTail = pNewNode;
    }
    pOldNode->pNext = pNewNode;
    return (POSITION) pNewNode;
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::RemoveAt(POSITION position)
{

    CNode* pOldNode = (CNode*) position;

    // remove pOldNode from list
    if (pOldNode == m_pNodeHead)
    {
        m_pNodeHead = pOldNode->pNext;
    }
    else
    {
        pOldNode->pPrev->pNext = pOldNode->pNext;
    }
    if (pOldNode == m_pNodeTail)
    {
        m_pNodeTail = pOldNode->pPrev;
    }
    else
    {
        pOldNode->pNext->pPrev = pOldNode->pPrev;
    }
    FreeNode(pOldNode);
}


template<class TYPE, class ARG_TYPE>
POSITION CList<TYPE, ARG_TYPE>::FindIndex(int nIndex) const
{
    ASSERT(nIndex >= 0);

    if (nIndex >= m_nCount)
        return NULL;  // went too far

    CNode* pNode = m_pNodeHead;
    while (nIndex--)
    {
        pNode = pNode->pNext;
    }
    return (POSITION) pNode;
}



// NOTE:
// dereferencing operator -> is not supported in this template
// because this is designed to allocate intrinsic data types only
//
template<class T>
class BufferPtr
{
public:
    BufferPtr(UINT Size) : m_pBase(NULL), m_Size(Size)
    {
    ASSERT(Size);
    m_pBase = new T[Size];
    m_pCur = m_pBase;
    if (!m_pBase)
        throw &g_MemoryException;
    }
    BufferPtr()
    {
    m_pBase = NULL;
    m_pCur = NULL;
    m_Size = 0;
    }
    ~BufferPtr()
    {
    if (m_pBase)
        delete [] m_pBase;
    }
    // casting operator
    operator T*()
    {
    return m_pCur;
    }
    operator T&()
    {
    ASSERT(m_pCur < m_pBase + m_Size);
    return *m_pCur;
    }
    operator void*()
    {
    return m_pCur;
    }
    T& operator*()
    {
    ASSERT(m_pCur < m_pBase + m_Size);
    return *m_pCur;
    }
    // increment/decrement
    T* operator+(UINT Inc)
    {
    ASSERT(m_pBase + m_Size > m_pCur + Inc);
    return (m_pBase + Inc);
    }
    T* operator-(UINT Dec)
    {
    ASSERT(m_pBase >= m_pCur - Dec);
    m_pCur -= Dec;
    return m_pCur;
    }
    //prefix
    T* operator++()
    {
    ASSERT(m_pBase + m_Size > m_pCur - 1);
    return ++m_pCur;
    }
    //postfix
    T* operator++(int inc)
    {
    pCur
    ASSERT(m_pBase + m_Size > m_pCur);
    return m_pCur++;
    }
    //prefix
    T* operator--()
    {
    ASSERT(m_pCur > m_pBase);
    return --m_pCur;
    }
    //postfix
    T* operator--(int inc)
    {
    ASSERT(m_pCur > m_pBase);
    return m_pCur--;
    }
    T** operator&()
    {
    return &m_pBase;
    }
    // subscripting
    T& operator[](UINT Index)
    {
    ASSERT(Index < m_Size);
    return m_pBase[Index];
    }
    void Attach(T* pT, UINT Size = 1)
    {
    ASSERT(!m_pBase);
    m_pBase = pT;
    m_pCur = m_pBase;
    m_Size = Size;
    }
    void Detach()
    {
    m_pBase = NULL;
    }
    UINT GetSize()
    {
    return m_Size;
    }
private:
    T*   m_pBase;
    T*   m_pCur;
    UINT    m_Size;
};

template<class T>
class SafePtr
{
public:
    SafePtr(T* p)
    {
    __p = p;
    }
    SafePtr()
    {
    __p = NULL;
    }
    ~SafePtr()
    {
    if (__p)
        delete __p;
    }
    void Attach(T* p)
    {
    ASSERT(NULL == __p);
    __p = p;
    }
    void Detach()
    {
    __p = NULL;
    }
    T* operator->()
    {
    ASSERT(__p);

    return __p;
    }
    T& operator*()
    {
    ASSERT(__p);

    return *__p;
    }
    operator T*()
    {
    return __p;
    }
    operator T&()
    {
    ASSERT(__p);
    return *__p;
    }
private:
    T*  __p;

};



class CPropPageProvider;

class CPropSheetData
{
public:
    CPropSheetData();
    ~CPropSheetData();
    virtual BOOL Create(HINSTANCE hInst, HWND hwndParent, UINT MaxPages, LONG_PTR lConsoleHandle = 0);
    BOOL InsertPage(HPROPSHEETPAGE hPage, int Index = -1);
    INT_PTR DoSheet()
    {
        return ::PropertySheet(&m_psh);
    }
    HWND GetWindowHandle()
    {
        return m_hWnd;
    }
    void PageCreateNotify(HWND hWnd);
    void PageDestroyNotify(HWND hWnd);
    PROPSHEETHEADER m_psh;
    BOOL PropertyChangeNotify(long lParam);
    void AddProvider(CPropPageProvider* pProvider)
    {
        m_listProvider.AddTail(pProvider);
    }
protected:
    UINT    m_MaxPages;
    LONG_PTR m_lConsoleHandle;
    HWND    m_hWnd;
private:
    CList<CPropPageProvider*, CPropPageProvider*> m_listProvider;
};
#if 0

class CDataWindow
{
public:
    CDataWindow() : m_hWnd(NULL)
    {}
    virtual ~CDataWindow()
    {}
    BOOL Create();
    static LRESULT DataWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
    }
    virtual LRESULT OnCreate()
    {
        return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
    }
    virtual LRESULT OnDestroy()
    {
        return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
    }
    operator HWND()
    {
        return m_hWnd;
    }
    HWND    m_hWnd;
};

#endif

class CDialog
{
public:
    CDialog(int TemplateId) : m_hDlg(NULL), m_TemplateId(TemplateId)
    {}
    virtual ~CDialog()
    {}
    static INT_PTR CALLBACK DialogWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR DoModal(HWND hwndParent, LPARAM lParam )
    {
        return DialogBoxParam(g_hInstance, MAKEINTRESOURCE(m_TemplateId), hwndParent, DialogWndProc, lParam);
    }
    void DoModaless(HWND hwndParent, LPARAM lParam)
    {
        m_hDlg = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(m_TemplateId), hwndParent, DialogWndProc, lParam);
    }
    virtual BOOL OnInitDialog()
    {
        return TRUE;
    }
    virtual void OnCommand(WPARAM wParam, LPARAM lParam)
    {}
    virtual BOOL OnDestroy()
    {
        return FALSE;
    }
    virtual BOOL OnHelp(LPHELPINFO pHelpInfo)
    {
        return FALSE;
    }
    virtual BOOL OnContextMenu(HWND hWnd, WORD xPos, WORD yPos)
    {
        return FALSE;
    }

    HWND GetControl(int idControl)
    {
        return GetDlgItem(m_hDlg, idControl);
    }
    operator HWND()
    {
        return m_hDlg;
    }
    HWND    m_hDlg;
private:
    int     m_TemplateId;
};


class CFileHandle
{
public:
    CFileHandle(HANDLE hFile = INVALID_HANDLE_VALUE) : m_hFile(hFile)
    {}
    ~CFileHandle()
    {
        if (INVALID_HANDLE_VALUE != m_hFile)
        CloseHandle(m_hFile);
    }
    void Open(HANDLE hFile)
    {
        ASSERT(INVALID_HANDLE_VALUE == m_hFile);
        m_hFile = hFile;
    }
    void Close()
    {
        if (INVALID_HANDLE_VALUE != m_hFile)
        CloseHandle(m_hFile);
    }
    HANDLE hFile()
    {
        return m_hFile;
    }
private:
    HANDLE  m_hFile;
};
class CLogFile
{
public:
    CLogFile() : m_hFile(INVALID_HANDLE_VALUE)
    {}
    ~CLogFile()
    {
        Close();
    }
    BOOL Create(LPCTSTR LogFileName)
    {
        if (!LogFileName)
        return FALSE;

        m_strLogFileName = LogFileName;
        m_hFile = CreateFile(LogFileName,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
        return INVALID_HANDLE_VALUE != m_hFile;
    }
    void Close()
    {
        if (m_hFile)
        {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        }
    }
    LPCTSTR LogFileName()
    {
        return m_strLogFileName.IsEmpty() ? NULL : (LPCTSTR)m_strLogFileName;
    }
    void Delete()
    {
        Close();
        if (m_strLogFileName)
        DeleteFile(m_strLogFileName);
    }
    BOOL LogLastError(LPCTSTR FunctionName);
    BOOL Logf(LPCTSTR Format, ...);
    BOOL Log(LPCTSTR Text);

private:
    HANDLE  m_hFile;
    String  m_strLogFileName;
};

STDAPI_(CONFIGRET) GetLocationInformation(
    DEVNODE dn,
    LPTSTR Location,
    ULONG LocationLen,  // In characters
    HMACHINE hMachine
    );

#endif  // __UTILS_H_
