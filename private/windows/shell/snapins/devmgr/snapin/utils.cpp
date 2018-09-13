/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    utils.cpp

Abstract:

    This module implements some utilities classes

Author:

    William Hsieh (williamh) created

Revision History:


--*/


#include "devmgr.h"


const TCHAR* const DEVMGR_DATAWINDOW_CASS_NAME = TEXT("DevMgrDataWindowClass");


//
// CPropSheetData implementation
//
// Every device or class has a CPropSheetData as a member.
// When m_hWnd contains a valid window handle, it indicates the device/class
// has a active property sheet. This helps us to do this:
// (1). We are sure that there is only one property sheet can be created
//  for the device/class at time in a single console no matter how many
//  IComponents(snapins, windows) are running in the same console.
//  For example, when users asks for the properties for the device/class
//  we can bring the active one to the foreground without creating a
//  new one.
// (2). We can warn the user that a removal of the device is not allowed
//  when the device has an active property sheet.
// (3). We can warn the user that there are property sheets active
//  when a "refresh" is requsted.
CPropSheetData::CPropSheetData()
{
    memset(&m_psh, 0, sizeof(m_psh));
    m_MaxPages = 0;
    m_lConsoleHandle = 0;
    m_hWnd = NULL;
}

// This function creates(or initialize) the propery sheet data header.
//
// INPUT: hInst -- the module instance handle
//    hwndParent -- parent window handle
//    MaxPages -- max pages allowed for this property sheet.
//    lConsoleHandle -- MMC property change notify handle.
//
// OUTPUT:  TRUE if succeeded.
//      FALSE if failed(mostly, memory allocation error). GetLastError
//      will report the error code.
//

BOOL
CPropSheetData::Create(
    HINSTANCE hInst,
    HWND hwndParent,
    UINT MaxPages,
    LONG_PTR lConsoleHandle
    )
{

    // nobody should try to create the property sheet while it is
    // still alive.
    ASSERT (NULL == m_hWnd);

    if (MaxPages > 64 || NULL == hInst)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    // if not page array is allocated or the existing
    // array is too small, allocate a new array.
    if (!m_psh.phpage || m_MaxPages < MaxPages)
    {
        if (m_MaxPages)
        {
            ASSERT(m_psh.phpage);
            delete [] m_psh.phpage;
            m_psh.phpage = NULL;
        }
        
        m_psh.phpage = new HPROPSHEETPAGE[MaxPages];
        m_MaxPages = MaxPages;
    }
    
    // initialize the header
    m_psh.nPages = 0;
    m_psh.dwSize = sizeof(m_psh);
    m_psh.dwFlags = PSH_PROPTITLE | PSH_NOAPPLYNOW;
    m_psh.hwndParent = hwndParent;
    m_psh.hInstance = hInst;
    m_psh.pszCaption = NULL;
    m_lConsoleHandle = lConsoleHandle;
    
    return TRUE;
}

// This function inserts the given HPROPSHEETPAGE to the
// specific location.
//
// INPUT: hPage  -- the page to be inserted.
//    Position -- the location to be inserted.
//            Position < 0, then append the page
//
// OUTPUT:  TRUE if the page is inserted successfully.
//      FALSE if the page is not inserted. GetLastError will
//      return the error code.
//
BOOL
CPropSheetData::InsertPage(
    HPROPSHEETPAGE hPage,
    int Position
    )
{
    if (NULL == hPage || (Position > 0 && (UINT)Position >= m_MaxPages))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    // make sure we have space for a new page.
    if (m_psh.nPages >= m_MaxPages)
    {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }
    
    if (Position < 0 || (UINT)Position >= m_psh.nPages)
    {
        // append the page.  This also include the very first page.
        // Most pages are appened.
        m_psh.phpage[m_psh.nPages++] = hPage;
    }
    
    else
    {
    
        ASSERT(m_psh.nPages);
    
        // move the page around so that we
        // can insert the new page to the
        // specific location.
        // At this moment, we know we have space to accomodate the
        // new page(so we can assume that &m_psh.phpage[m_psh.nPage]
        // is valid. Also, we are here because there is at least one
        // pages in the array.
        for (int i = m_psh.nPages; i > Position; i--)
            m_psh.phpage[i] = m_psh.phpage[i - 1];
        
        m_psh.phpage[Position] = hPage;
        m_psh.nPages++;
    }
    
    return TRUE;
}
//
// This function receives notification from its attached
// property pages about their window(dialog) creation
// It takes a chance to record the property sheet window handle
// which we can use to dismiss the property sheet or bring it
// to the foreground.
// INPUT:
//  hWnd -- the property page's window handle
//
// OUTPUT:
//  NONE
//
void
CPropSheetData::PageCreateNotify(HWND hWnd)
{
    ASSERT(hWnd);
    hWnd = ::GetParent(hWnd);
    
    if (!m_hWnd)
        m_hWnd = hWnd;
}

//
// This function receives notification from its attached
// property pages about their window(dialog) destroy.
// When all attached pages are gone, this function
// reset its internal states and free memory allocation
// WARNING!!!! Do not delete the object when the attached
// window handle counts reaches 0 because we can be reused --
// the reason we have a separate Create functions.
//
// INPUT:
//  hWnd -- the property page's window handle
//
// OUTPUT:
//  NONE
//
void
CPropSheetData::PageDestroyNotify(HWND hWnd)
{
    //
    m_hWnd = NULL;
    delete [] m_psh.phpage;
    m_psh.phpage = NULL;
    m_MaxPages = 0;
    memset(&m_psh, 0, sizeof(m_psh));
    
    if (m_lConsoleHandle)
        MMCFreeNotifyHandle(m_lConsoleHandle);

    m_lConsoleHandle = 0;
    
    if (!m_listProvider.IsEmpty())
    {
        POSITION pos = m_listProvider.GetHeadPosition();
        
        while (NULL != pos)
        {
            delete m_listProvider.GetNext(pos);
        }
        
        m_listProvider.RemoveAll();
    }
}

CPropSheetData::~CPropSheetData()
{
    if (m_lConsoleHandle)
        MMCFreeNotifyHandle(m_lConsoleHandle);

    if (!m_listProvider.IsEmpty())
    {
        POSITION pos = m_listProvider.GetHeadPosition();
        
        while (NULL != pos)
        {
            delete m_listProvider.GetNext(pos);
        }
        
        m_listProvider.RemoveAll();
    }
    
    if (m_psh.phpage)
        delete [] m_psh.phpage;
}

BOOL
CPropSheetData::PropertyChangeNotify(
    long lParam
    )
{
    if (m_lConsoleHandle)
    {
        MMCPropertyChangeNotify(m_lConsoleHandle, lParam);
        return TRUE;
    }
    
    return FALSE;
}

#if 0
//
// CDataWindow implementation
//

BOOL
CDataWindow::Create()
{
    WNDCLASS wndClass;
    
    //lets see if the class has been registered.
    if (!GetClassInfo(g_hInstance, DEVMGR_DATAWINDOW_CLASS_NAME, &wndClass))
    {
        // register the class
        memset(&wndClass, 0, sizeof(wndClass));
        wndClass.lpfnWndProc = DataWindowProc;
        wndClass.hInstance = g_hInstance;
        wndClass.lpszClassName = DEVMGR_DATAWINDOW_CLASS_NAME;
        if (!RegisterClass(&wndClass))
            return FALSE;
    }
    
    // create a window
    m_hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, DEVMGR_NOTIFY_CLASS_NAME,
                TEXT(""),
                WS_DLGFRAME|WS_BORDER|WS_DISABLED,
                CW_USEDEFAULT, CW_USEDEFAULT,
                0, 0, NULL, NULL, g_hInstance, (void*)this);
    
    return (NULL != m_hWnd);
}

LRESULT
CDataWindow::DataWindowProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LRESULT lResult;

    CDataWindow* pThis;
    pThis = (CDataWindow*)GetWindowLong(hWnd, GWL_USERDATA);
    switch (uMsg)
    {
    case WM_CREATE:
        {
        pThis =  (CDataWindow*)((CREATESTRUCT*)lParam)->lpCreateParams;
        SetWindowLong(hWnd, GWL_USERDATA, (long)pThis);
        ASSERT(pThis)
        lResult = pThis->OnCreate();
        break;
        }
    case WM_DESTROY:
        {
        ASSERT(pThis);
        lResult = pThis->OnDestroy();
        break;
        }
    default:
        {
        if (pThis)
            lResult =  pThis->OnMsg(uMsg, wParam, lParam);
        else
            lResult =  DefaultWindowProc(m_hWnd, wParam, lParam);
        break;
        }
    }
    return lResult;
}

#endif

//
// CDialog implementation
//

INT_PTR CALLBACK
CDialog::DialogWndProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CDialog* pThis = (CDialog *) GetWindowLongPtr(hDlg, DWLP_USER);
    BOOL Result;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
        pThis = (CDialog *)lParam;
        ASSERT(pThis);
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pThis);
        pThis->m_hDlg = hDlg;
        Result = pThis->OnInitDialog();
        break;
        }
    case WM_COMMAND:
        {
        if (pThis)
            pThis->OnCommand(wParam, lParam);
        Result = FALSE;
        break;
        }
    case WM_DESTROY:
        {
        if (pThis)
            Result = pThis->OnDestroy();
        else
            Result = FALSE;
        break;
        }
    case WM_HELP:
        {
        if (pThis)
            pThis->OnHelp((LPHELPINFO)lParam);
        Result = FALSE;
        break;
        }
    case WM_CONTEXTMENU:
        {
        if (pThis)
            pThis->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
        Result = FALSE;
        break;
        }

    default:
        Result = FALSE;
        break;
    }
    return Result;
}


//
// class String implementation
//
String::String()
{
    m_pData = new StringData;
    if (!m_pData)
    throw &g_MemoryException;
}

String::String(
    const String& strSrc
    )
{
    m_pData = strSrc.m_pData;
    m_pData->AddRef();
}

String::String(
    int Len
    )
{
    StringData* pNewData = new StringData;
    TCHAR* ptszNew = new TCHAR[Len + 1];
    
    if (pNewData && ptszNew)
    {
        pNewData->Len = 0;
        pNewData->ptsz = ptszNew;
        m_pData = pNewData;
    }
    
    else
    {
        delete pNewData;
        delete [] ptszNew;
        throw &g_MemoryException;
    }
}

String::String(
    LPCTSTR lptsz
    )
{
    int Len = lstrlen(lptsz);
    StringData* pNewData = new StringData;
    TCHAR* ptszNew = new TCHAR[Len + 1];
    
    if (pNewData && ptszNew)
    {
        lstrcpy(ptszNew, lptsz);
        pNewData->Len = Len;
        pNewData->ptsz = ptszNew;
        m_pData = pNewData;
    }
    
    else
    {
        delete pNewData;
        delete [] ptszNew;
        throw &g_MemoryException;
    }
}

void
String::Empty()
{
    if (m_pData->Len)
    {
        StringData* pNewData = new StringData;
        
        if (pNewData)
        {
            m_pData->Release();
            m_pData = pNewData;
        }
        
        else
        {
            throw &g_MemoryException;
        }
    }
}
String&
String::operator=(
    const String&  strSrc
    )
{
    // look out for aliasings !!!!
    if (this != &strSrc)
    {
        // add the reference count first before release the old one
        // in case our string data is the same as strSrc's.
        strSrc.m_pData->AddRef();
        m_pData->Release();
        m_pData = strSrc.m_pData;
    }
    
    return *this;
}

String&
String::operator=(
    LPCTSTR ptsz
    )
{
    // if we are pointing to the same string,
    // do nothing
    if (ptsz == m_pData->ptsz)
        return *this;
    
    //
    // str = NULL --> empty the string
    //
    if (!ptsz)
    {
        Empty();
        return *this;
    }
    
    // a new assignment, allocate a new string data
    StringData* pNewData = new StringData;
    int len = lstrlen(ptsz);
    TCHAR* ptszNew = new TCHAR[len + 1];
    
    if (pNewData && ptszNew)
    {
        lstrcpy(ptszNew, ptsz);
        pNewData->Len = len;
        pNewData->ptsz = ptszNew;
        m_pData->Release();
        m_pData = pNewData;
    }
    
    else
    {
        //memory allocation failure
        delete pNewData;
        delete [] ptszNew;
        throw g_MemoryException;
    }
    
    return *this;
}

String&
String::operator+=(
    const String& strSrc
    )
{
    if (strSrc.GetLength())
    {
        int TotalLen = m_pData->Len + strSrc.GetLength();
        StringData* pNewData = new StringData;
        TCHAR* ptszNew = new TCHAR[TotalLen + 1];
        
        if (pNewData && ptszNew)
        {
            lstrcpy(ptszNew, m_pData->ptsz);
            lstrcat(ptszNew, (LPCTSTR)strSrc);
            pNewData->Len = TotalLen;
            pNewData->ptsz = ptszNew;
            m_pData->Release();
            m_pData = pNewData;
        }
        
        else
        {
            delete pNewData;
            delete [] ptszNew;
            throw &g_MemoryException;
        }
    }
    
    return *this;
}
String&
String::operator+=(
    LPCTSTR ptsz
    )
{
    if (ptsz)
    {
        int len = lstrlen(ptsz);
        if (len)
        {
            StringData* pNewData = new StringData;
            TCHAR* ptszNew = new TCHAR[len + m_pData->Len + 1];
            
            if (ptszNew && pNewData)
            {
                lstrcpy(ptszNew, m_pData->ptsz);
                lstrcat(ptszNew, ptsz);
                pNewData->Len = len + m_pData->Len;
                pNewData->ptsz = ptszNew;
                m_pData->Release();
                m_pData = pNewData;
            }
            
            else
            {
                delete pNewData;
                delete [] ptszNew;
                throw &g_MemoryException;
            }
        }
    }
    
    return *this;
}

TCHAR&
String::operator[](
    int Index
    )
{
    ASSERT(Index < m_pData->Len);
    // make a separate copy of the string data
    TCHAR* ptszNew = new TCHAR[m_pData->Len + 1];
    StringData* pNewData = new StringData;
    
    if (ptszNew && pNewData)
    {
        lstrcpy(ptszNew, m_pData->ptsz);
        pNewData->ptsz = ptszNew;
        pNewData->Len = m_pData->Len;
        m_pData->Release();
        m_pData = pNewData;
        return ptszNew[Index];
    }
    
    else
    {
        delete pNewData;
        delete [] ptszNew;
        throw &g_MemoryException;
        return m_pData->ptsz[Index];
    }
}

String::operator LPTSTR()
{
    StringData* pNewData = new StringData;
    if (pNewData)
    {
        if (m_pData->Len)
        {
            TCHAR* ptszNew = new TCHAR[m_pData->Len + 1];
            
            if (ptszNew)
            {
                lstrcpy(ptszNew, m_pData->ptsz);
                pNewData->ptsz = ptszNew;
            }
            
            else
            {
                throw &g_MemoryException;
                delete pNewData;
                return NULL;
            }
        }
        
        pNewData->Len = m_pData->Len;
        m_pData->Release();
        m_pData = pNewData;
        return  m_pData->ptsz;
    }
    
    else
    {
        throw &g_MemoryException ;
        return NULL;
    }
}

//
// This is a friend function to String
// Remember that we can NOT return a reference or a pointer.
// This function must return "by-value"
String
operator+(
    const String& str1,
    const String& str2
    )
{
    int TotalLen = str1.GetLength() + str2.GetLength();
    String strThis(TotalLen);
    lstrcpy(strThis.m_pData->ptsz, str1);
    lstrcat(strThis.m_pData->ptsz, str2);
    strThis.m_pData->Len = TotalLen;
    return strThis;
}

BOOL
String::LoadString(
    HINSTANCE hInstance,
    int ResourceId
    )
{
    // we have no idea how long the string will be.
    // The strategy here is to allocate a stack-based buffer which
    // is large enough for most cases. If the buffer is too small,
    // we then use heap-based buffer and increment the buffer size
    // on each try.
    TCHAR tszTemp[256];
    long FinalSize, BufferSize;
    BufferSize = ARRAYLEN(tszTemp);
    TCHAR* HeapBuffer = NULL;
    
    // first try
    FinalSize = ::LoadString(hInstance, ResourceId, tszTemp, BufferSize);

    //
    // LoadString returns the size of the string it loaded, not including the
    // NULL termiated char. So if the returned len is one less then the
    // provided buffer size, our buffer is too small.
    //
    if (FinalSize < (BufferSize - 1))
    {
        // we got what we want
        HeapBuffer = tszTemp;
    }
    
    else
    {
        // the stack based buffer is too small, we have to switch to heap
        // based.
        BufferSize = ARRAYLEN(tszTemp);
    
        // should 32k chars big enough????
        while (BufferSize < 0x8000)
        {
            BufferSize += 256;
    
            // make sure there is no memory leak
            ASSERT(NULL == HeapBuffer);
    
            // allocate a new buffer
            HeapBuffer = new TCHAR[BufferSize];
            
            if (HeapBuffer)
            {
                // got a new buffer, another try...
                FinalSize = ::LoadString(hInstance, ResourceId, HeapBuffer,
                              BufferSize);

                if (FinalSize < (BufferSize - 1))
                {
                    //got it!
                    break;
                }
            }

            else
            {
                throw &g_MemoryException;
            }

            // discard the buffer
            delete [] HeapBuffer;
            HeapBuffer = NULL;
        }
    }

    if (HeapBuffer)
    {
        TCHAR* ptszNew = new TCHAR[FinalSize + 1];
        StringData* pNewData = new StringData;
        
        if (pNewData && ptszNew)
        {
            lstrcpy(ptszNew, HeapBuffer);
            
            // release the old string data because we will have a new one
            m_pData->Release();
            m_pData = pNewData;
            m_pData->ptsz = ptszNew;
            m_pData->Len = FinalSize;
            
            if (HeapBuffer != tszTemp) {
            
                delete [] HeapBuffer;
            }

            return TRUE;
        }
        
        else
        {
            delete [] ptszNew;
            delete pNewData;

            if (HeapBuffer != tszTemp) {
            
                delete [] HeapBuffer;
            }

            throw &g_MemoryException;
        }
    }

    return FALSE;
}

//
// This function creates an full-qualified machine name for the
// local computer.
//
BOOL
String::GetComputerName()
{
    TCHAR tszTemp[MAX_PATH];
    // the GetComputerName api only return the name only.
    // we must prepend the UNC signature.
    tszTemp[0] = _T('\\');
    tszTemp[1] = _T('\\');
    ULONG NameLength = ARRAYLEN(tszTemp) - 2;
    
    if (::GetComputerName(tszTemp + 2, &NameLength))
    {
        int Len = lstrlen(tszTemp);
        StringData* pNewData = new StringData;
        TCHAR* ptszNew = new TCHAR[Len + 1];
        
        if (pNewData && ptszNew)
        {
            pNewData->Len = Len;
            lstrcpy(ptszNew, tszTemp);
            pNewData->ptsz = ptszNew;
            m_pData->Release();
            m_pData = pNewData;
            return TRUE;
        }
        
        else
        {
            delete pNewData;
            delete []ptszNew;
            throw &g_MemoryException;
        }
    }
    
    return FALSE;
}


void
String::Format(
    LPCTSTR FormatString,
    ...
    )
{
    // according to wsprintf specification, the max buffer size is
    // 1024
    TCHAR* pBuffer = new TCHAR[1024];
    if (pBuffer)
    {
        va_list arglist;
        va_start(arglist, FormatString);
        int len;
        len = wvsprintf(pBuffer, FormatString, arglist);
        va_end(arglist);
    
        if (len)
        {
            TCHAR* ptszNew = new TCHAR[len + 1];
            StringData* pNewData = new StringData;
            
            if (pNewData && ptszNew)
            {
                pNewData->Len = len;
                lstrcpy(ptszNew, pBuffer);
                pNewData->ptsz = ptszNew;
                m_pData->Release();
                m_pData = pNewData;
                delete [] pBuffer;
                return;
            }
            
            else
            {
                delete [] pBuffer;
                delete [] ptszNew;
                delete [] pNewData;
                throw &g_MemoryException;
            }
        }
    }

    throw &g_MemoryException;
}


//
// templates
//

template <class T>
inline void ContructElements(T* pElements, int Count)
{
    ASSERT(Count > 0);
    memset((void*)pElements, Count * sizeof(T));
    for (; Count; pElments++, Count--)
    {
        // call the class's ctor
        // note the placement.
        new((void*)pElements) T;
    }
}


//
// CCommandLine implementation
//

// code adapted from C startup code -- see stdargv.c
// It walks through the given CmdLine and calls ParseParam
// when an argument is encountered.
// An argument must in this format:
// </command arg_to_command>  or <-command arg_to_command>
void
CCommandLine::ParseCommandLine(
    LPCTSTR CmdLine
    )
{
    LPCTSTR p;
    LPTSTR args, pDst;
    BOOL bInQuote;
    BOOL bCopyTheChar;
    int  nSlash;
    p = CmdLine;
    args = new TCHAR[lstrlen(CmdLine) + 1];
    
    if (!args)
        return;
    
    for (;;)
    {
        // skip blanks
        while (_T(' ') == *p || _T('\t') == *p)
            p++;
        
        // nothing left, bail
        if (_T('\0') == *p)
            break;
        
        // 2N backslashes + '\"' ->N backslashes plus string delimiter
        // 2N + 1 baclslashes + '\"' ->N backslashes plus literal '\"'
        // N backslashes -> N backslashes
        nSlash = 0;
        bInQuote = FALSE;
        pDst = args;
        
        for (;;)
        {
            bCopyTheChar = TRUE;
            //count how may backslashes
            while(_T('\\') == *p)
            {
                p++;
                nSlash++;
            }
            
            if (_T('\"') == *p)
            {
                if (0 == (nSlash % 2))
                {
                    // 2N backslashes plus '\"' ->N baskslashes plus
                    // delimiter
                    if (bInQuote)
                    // double quote inside quoted string
                    // skip the first and copy the second.
                    if (_T('\"') == p[1])
                        p++;
                    else
                        bCopyTheChar = FALSE;
                    else
                    bCopyTheChar = FALSE;
                    // toggle quoted status
                    bInQuote = !bInQuote;
                }
            
                nSlash /= 2;
            }
            
            while (nSlash)
            {
                *pDst++ = _T('\\');
                nSlash--;
            }
    
            if (_T('\0') == *p || (!bInQuote && (_T(' ') == *p || _T('\t') == *p)))
            {
               break;
            }

            // copy char to args
            if (bCopyTheChar)
            {
                *pDst++ = *p;
            }
            p++;
        }
        // we have a complete argument now. Null terminates it and
        // let the derived class parse the argument.
        *pDst = _T('\0');
        // skip blanks to see if this is the last argument
        while (_T(' ') == *p || _T('\t') == *p)
            p++;
        BOOL bFlag;
        bFlag = (_T('/') == *args || _T('-') == *args);
        pDst = (bFlag) ? args + 1 : args;
        ParseParam(pDst, bFlag, _T('\0') == *p);
    }
    
    delete [] args;
}


//
// CSafeRegistry implementation
//

BOOL
CSafeRegistry::Open(
    HKEY hKeyAncestor,
    LPCTSTR KeyName,
    REGSAM Access
    )
{
    DWORD LastError;
    // we shouldn't has a valid key -- or memory leak
    // Also, a key name must be provided -- or open nothing
    ASSERT(!m_hKey && KeyName);
    LastError =  ::RegOpenKeyEx(hKeyAncestor, KeyName, 0, Access, &m_hKey);
    SetLastError(LastError);
    return ERROR_SUCCESS == LastError;
}


BOOL
CSafeRegistry::Create(
    HKEY hKeyAncestor,
    LPCTSTR KeyName,
    REGSAM Access,
    DWORD* pDisposition,
    DWORD Options,
    LPSECURITY_ATTRIBUTES pSecurity
    )
{
    ASSERT(KeyName && !m_hKey);
    DWORD Disposition;
    DWORD LastError;
    LastError = ::RegCreateKeyEx(hKeyAncestor, KeyName, 0, TEXT(""),
                   Options, Access, pSecurity,
                   &m_hKey, &Disposition
                   );
    SetLastError(LastError);
    
    if (ERROR_SUCCESS == LastError && pDisposition)
    {
        *pDisposition = Disposition;
    }
    
    if (ERROR_SUCCESS != LastError)
        m_hKey = NULL;
    
    return ERROR_SUCCESS == LastError;
}

BOOL
CSafeRegistry::SetValue(
    LPCTSTR ValueName,
    DWORD Type,
    const PBYTE pData,
    DWORD DataLen
    )
{
    ASSERT(m_hKey);
    DWORD LastError;
    LastError = ::RegSetValueEx(m_hKey, ValueName, 0, Type, pData, DataLen);
    SetLastError(LastError);
    return ERROR_SUCCESS == LastError;
}

BOOL
CSafeRegistry::SetValue(
    LPCTSTR ValueName,
    LPCTSTR Value
    )
{
    return  SetValue(ValueName,
             REG_SZ,
             (PBYTE)Value,
             (lstrlen(Value) + 1) * sizeof(TCHAR)
            );
}
BOOL
CSafeRegistry::GetValue(
    LPCTSTR ValueName,
    DWORD* pType,
    const PBYTE pData,
    DWORD* pDataLen
    )
{
    ASSERT(m_hKey);
    DWORD LastError;
    LastError = ::RegQueryValueEx(m_hKey, ValueName, NULL, pType, pData,
                    pDataLen);
    SetLastError(LastError);
    return ERROR_SUCCESS == LastError;
}

BOOL
CSafeRegistry::GetValue(
    LPCTSTR ValueName,
    String& str
    )
{
    DWORD Type, Size;
    PBYTE Buffer = NULL;
    Size = 0;
    BOOL Result = FALSE;

    // check size before Type because when the size is zero, type contains
    // undefined data.
    if (GetValue(ValueName, &Type, NULL, &Size) && Size && REG_SZ == Type)
    {
        // we do not want to throw an exception here.
        // so guard it
        try
        {
            BufferPtr<BYTE> BufferPtr(Size);
            Result = GetValue(ValueName, &Type, BufferPtr, &Size);
            if (Result)
            str = (LPCTSTR)(BYTE*)BufferPtr;
        }
        
        catch(CMemoryException* e)
        {
            e->Delete();
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            Result = FALSE;
        }
    }
    
    return Result;
}

BOOL
CSafeRegistry::EnumerateSubkey(
    DWORD Index,
    LPTSTR Buffer,
    DWORD* BufferSize
    )
{
    DWORD LastError;
    FILETIME LastWrite;
    LastError = ::RegEnumKeyEx(m_hKey, Index, Buffer, BufferSize,
                   NULL, NULL, NULL, &LastWrite);
    SetLastError(LastError);
    return ERROR_SUCCESS == LastError;
}

BOOL
CSafeRegistry::DeleteValue(
    LPCTSTR ValueName
    )
{
    ASSERT(m_hKey);
    DWORD LastError = ::RegDeleteValue(m_hKey, ValueName);
    SetLastError(LastError);
    return ERROR_SUCCESS == LastError;
}

BOOL
CSafeRegistry::DeleteSubkey(
    LPCTSTR SubkeyName
    )
{
    ASSERT(m_hKey);
    CSafeRegistry regSubkey;
    TCHAR KeyName[MAX_PATH];
    FILETIME LastWrite;
    DWORD KeyNameLen;
    
    while (TRUE)
    {
        KeyNameLen = ARRAYLEN(KeyName);
        // always uses index 0(the first subkey)
        if (!regSubkey.Open(m_hKey, SubkeyName, KEY_WRITE | KEY_ENUMERATE_SUB_KEYS) ||
            ERROR_SUCCESS != ::RegEnumKeyEx(regSubkey, 0, KeyName,
                        &KeyNameLen, NULL, NULL, NULL,
                        &LastWrite) ||
            !regSubkey.DeleteSubkey(KeyName))
        {
            break;
        }
        
        // close the key so that we will re-open it on each loop
        // -- we have deleted one subkey and without closing
        // the key, the index to RegEnumKeyEx will be confusing
        regSubkey.Close();
    }
    
    // now delete the subkey
    ::RegDeleteKey(m_hKey, SubkeyName);
    
    return TRUE;
}


//
//CLogFile::Log implementation
//

BOOL
CLogFile::Log(
    LPCTSTR Text
    )
{
    DWORD BytesWritten;
    if (Text && m_hFile)
    {
    int Size = lstrlen(Text);
#ifdef UNICODE
    BufferPtr<CHAR> Buffer(Size * sizeof(WCHAR));
    Size = WideCharToMultiByte(CP_ACP, 0, Text, Size, Buffer, Size*sizeof(WCHAR), NULL, NULL);
    return WriteFile(m_hFile, Buffer, Size, &BytesWritten, NULL);
#else
    retrun WriteFile(m_hFile, Text, Size, &BytesWritten, NULL);
#endif
    }
    return TRUE;
}
//
// CLogFile::Logf implementation
//

BOOL
CLogFile::Logf(
    LPCTSTR Format,
    ...
    )
{
    if (m_hFile)
    {
        // according to wsprintf specification, the max buffer size is
        // 1024
        TCHAR Buffer[1024];
        va_list arglist;
        va_start(arglist, Format);
        wvsprintf(Buffer, Format, arglist);
        va_end(arglist);
        return Log(Buffer);
    }
    
    return TRUE;
}

//
// CLogFile::LogLastError implementation
//

BOOL
CLogFile::LogLastError(
    LPCTSTR FunctionName
    )
{
    if (m_hFile && FunctionName)
    {
        TCHAR szMsg[MAX_PATH];
        FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
              NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
              szMsg, ARRAYLEN(szMsg), NULL);
        Logf(TEXT("%s: error(%ld) : %s\n"), FunctionName, GetLastError(), szMsg);
    }
    
    return TRUE;
}
