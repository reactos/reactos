/*++

    Implements IShellFolder.

    IUnknown
    IPersist
    IPersistFolder
    IShellFolder
    IExtractIcon

Soon:

    IContextMenu
    IDataObject
    IShellPropSheetExt ?

--*/

#include <windows.h>
#include <shlobj.h>

#include <docobj.h> // IOleCommandTarget

#include "pstore.h"

#include "enumid.h"
#include "utility.h"

#include "shfolder.h"
#include "shview.h"
#include "icon.h"   // icon handling
#include "guid.h"
#include "resource.h"


extern HINSTANCE    g_hInst;
extern LONG         g_DllRefCount;


CShellFolder::CShellFolder(
    CShellFolder *pParent,
    LPCITEMIDLIST pidl
    )
{

    m_pSFParent = pParent;
    if(m_pSFParent) {
        m_pSFParent->AddRef();
    }

    //
    // get the shell's IMalloc pointer
    // we'll keep this until we get destroyed
    //

    if(FAILED(SHGetMalloc(&m_pMalloc)))
        delete this;

    //
    // make a copy of the pidl in it's entirety.  We do this so we are free
    // to look at the pidl later.
    //

    m_pidl = CopyPidl(m_pMalloc, pidl);

    m_ObjRefCount = 1;
    InterlockedIncrement(&g_DllRefCount);
}


CShellFolder::~CShellFolder()
{
    if(m_pSFParent)
        m_pSFParent->Release();

    if(m_pidl)
        m_pMalloc->Free(m_pidl);

    if(m_pMalloc)
        m_pMalloc->Release();

    InterlockedDecrement(&g_DllRefCount);
}


STDMETHODIMP
CShellFolder::QueryInterface(
    REFIID riid,
    LPVOID *ppReturn
    )
{
    *ppReturn = NULL;

    if(IsEqualIID(riid, IID_IUnknown))
        *ppReturn = (IUnknown*)(IShellFolder*)this;
    else if(IsEqualIID(riid, IID_IPersistFolder))
        *ppReturn = (IPersistFolder*)(CShellFolder*)this;
    else if(IsEqualIID(riid, IID_IShellFolder))
        *ppReturn = (CShellFolder*)this;

    if(*ppReturn == NULL)
        return E_NOINTERFACE;

    (*(LPUNKNOWN*)ppReturn)->AddRef();
    return S_OK;
}


STDMETHODIMP_(DWORD)
CShellFolder::AddRef()
{
    return InterlockedIncrement(&m_ObjRefCount);
}


STDMETHODIMP_(DWORD)
CShellFolder::Release()
{
    LONG lDecremented = InterlockedDecrement(&m_ObjRefCount);

    if(lDecremented == 0)
        delete this;

    return lDecremented;
}


STDMETHODIMP
CShellFolder::GetClassID(
    LPCLSID lpClassID
    )
/*++

    IPersist::GetClassID

--*/
{
    *lpClassID = CLSID_PStoreNameSpace;

    return S_OK;
}


STDMETHODIMP
CShellFolder::Initialize(
    LPCITEMIDLIST pidl
    )
/*++

    IPersistFolder::Initialize

--*/
{
    return S_OK;
}



STDMETHODIMP
CShellFolder::BindToObject(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID *ppvOut
    )
/*++

    Creates an IShellFolder object for a subfolder.

--*/
{
    CShellFolder   *pShellFolder;

    pShellFolder = new CShellFolder(this, pidl);
    if(pShellFolder == NULL)
        return E_OUTOFMEMORY;

    HRESULT  hr = pShellFolder->QueryInterface(riid, ppvOut);

    pShellFolder->Release();

    return hr;
}

STDMETHODIMP
CShellFolder::CompareIDs(
    LPARAM lParam,
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2
    )
/*++
    Determines the relative ordering of two file objects or folders,
    given their item identifier lists.

    Returns a handle to a result code. If this method is successful,
    the CODE field of the status code (SCODE) has the following meaning:

    Less than zero      The first item should precede the second (pidl1 < pidl2).
    Greater than zero   The first item should follow the second (pidl1 > pidl2)
    Zero                The two items are the same (pidl1 = pidl2).

    Passing 0 as the lParam indicates sort by name.
    0x00000001-0x7fffffff are for folder specific sorting rules.
    0x80000000-0xfffffff are used by the system.

--*/
{
    LPCWSTR szString1;
    LPCWSTR szString2;
    DWORD cbLength1;
    DWORD cbLength2;
    DWORD dwCompare;

    //
    // compare strings first, then GUID if equality is encountered
    // this is important because we do not want to return a value indicating
    // equality based on display string only; we also want to account for the
    // GUID associated with the display name, since we can have multiple GUIDs
    // with the same display name.  If we just compared the string values,
    // the shell would not display all types/subtypes.
    //

    szString1 = GetPidlText(pidl1);
    szString2 = GetPidlText(pidl2);

    cbLength1 = lstrlenW(szString1) ;
    cbLength2 = lstrlenW(szString2) ;

    //
    // check via shortest length string
    //

    if(cbLength2 < cbLength1)
        cbLength1 = cbLength2;

    cbLength1 *= sizeof(WCHAR);

    dwCompare = memcmp(szString1, szString2, cbLength1);

    if(dwCompare == 0) {
        GUID *guid1;
        GUID *guid2;

        //
        // now compare the GUIDs.
        //

        guid1 = GetPidlGuid(pidl1);
        guid2 = GetPidlGuid(pidl2);

        dwCompare = memcmp(guid1, guid2, sizeof(GUID));
    }

    //
    // still equal? sort by PST_KEY_CURRENT_USER, then PST_KEY_LOCAL_MACHINE
    //

	if(dwCompare == 0) {
    	dwCompare = GetPidlKeyType(pidl1) - GetPidlKeyType(pidl2);
	}

    return ResultFromDWORD(dwCompare);
}


STDMETHODIMP
CShellFolder::CreateViewObject(
    HWND hwndOwner,
    REFIID riid,
    LPVOID *ppvOut
    )
/*++

    CreateViewWindow creates a view window. This can be either the right pane
    of the Explorer or the client window of a folder window.

--*/
{
    HRESULT     hr;
    CShellView  *pShellView;

    pShellView = new CShellView(this, m_pidl);
    if(pShellView == NULL)
        return E_OUTOFMEMORY;

    hr = pShellView->QueryInterface(riid, ppvOut);

    pShellView->Release();

    return hr;
}

STDMETHODIMP
CShellFolder::EnumObjects(
    HWND hwndOwner,
    DWORD dwFlags,
    LPENUMIDLIST *ppEnumIDList
    )
/*++
    Determines the contents of a folder by creating an item enumeration
    object (a set of item identifiers) that can be retrieved using the
    IEnumIDList interface.

--*/
{
    *ppEnumIDList = new CEnumIDList(m_pidl, FALSE);
    if(*ppEnumIDList == NULL)
        return E_FAIL;

    return NOERROR;
}


STDMETHODIMP
CShellFolder::GetAttributesOf(
    UINT uCount,
    LPCITEMIDLIST aPidls[],
    ULONG *pulAttribs
    )
/*++
    Retrieves the attributes of one or more file objects or subfolders.

    Builds a ULONG value that specifies the common (logically AND'ed)
    attributes of specified file objects, which is supplied to the caller
    via the pdwAttribs parameter.

--*/
{
    UINT  i;

    *pulAttribs = (ULONG)-1; // assume all in common initially

    for(i = 0; i < uCount; i++)
    {
        DWORD dwSubFolder = SFGAO_CANDELETE | SFGAO_HASPROPSHEET;

        if(HasSubFolders(aPidls[i]))
            dwSubFolder |= SFGAO_HASSUBFOLDER;

        *pulAttribs &= (SFGAO_FOLDER | dwSubFolder);
    }

    return NOERROR;
}

BOOL
CShellFolder::HasSubFolders(
    LPCITEMIDLIST pidl
    )
/*++

    This function is used to test if the specified pidl has subfolders.

    The combination of this->m_pidl and the supplied pidl can be used
    to make this determination.

--*/
{
    //
    // If we are at the subtype level, then no subfolders exist
    //

    if(GetPidlType(pidl) >= PIDL_TYPE_SUBTYPE)
        return FALSE;

    //
    // TODO: is there anyway to check if the root has subfolders?
    // m_pidl == NULL or pidl == NULL ???
    // then try to enum providers.
    //


	//
	// make a fully qualified (absolute) pidl out of m_pidl and pidl,
	// then call the enum interface to see if subfolders exist.
	//

	LPITEMIDLIST pidlNew = CopyCatPidl(m_pidl, pidl);
	if(pidlNew == NULL)
		return FALSE;

	BOOL bSubfolder = FALSE;

    LPENUMIDLIST pEnumIDList = new CEnumIDList(pidlNew, FALSE);
    if(pEnumIDList != NULL) {

	    ULONG ulFetched;
    	LPITEMIDLIST pidl = NULL;

		if( NOERROR == pEnumIDList->Next(1, &pidl, &ulFetched) && ulFetched == 1) {
			FreePidl(pidl);
			bSubfolder = TRUE;	
		}

		pEnumIDList->Release();
	}

	FreePidl(pidlNew);

	return bSubfolder;
}


STDMETHODIMP
CShellFolder::GetDisplayNameOf(
    LPCITEMIDLIST pidl,
    DWORD dwFlags,
    LPSTRRET lpName
    )
/*++
    Retrieves the display name for the specified file object or subfolder,
    returning it in a STRRET structure.

    If the ID contains the display name (in the local character set),
    it returns the offset to the name. If not, it returns a pointer to
    the display name string (UNICODE) allocated by the task allocator,
    or it fills in a buffer. The type of string returned depends on the
    type of display specified.

    Values identifying different types of display names are contained
    in the enumeration SHGNO.

--*/
{
    switch(dwFlags)
    {
        case SHGDN_NORMAL:
        case SHGDN_INFOLDER:
        case SHGDN_FORPARSING:
        {
            LPCWSTR szDisplay;
            DWORD cbDisplay;
            LPWSTR pOleStr;

            szDisplay = GetPidlText(pidl);
            cbDisplay = (lstrlenW(szDisplay) + 1) * sizeof(WCHAR);

            pOleStr = (LPWSTR)CoTaskMemAlloc(cbDisplay);
            if(pOleStr == NULL)
                return E_OUTOFMEMORY;

            CopyMemory(pOleStr, szDisplay, cbDisplay);

            lpName->uType = STRRET_WSTR;
            lpName->pOleStr = pOleStr;

            return NOERROR;
        }

        default:
            return E_INVALIDARG;
    }
}




BOOL
CShellFolder::GetPidlFullText(
    LPCITEMIDLIST pidl,
    LPTSTR lpszOut,
    DWORD dwOutSize
    )
/*++
    This function returns a string containing the full-text associated with
    the supplied pidl.  The full text will consist of a "full path" string,
    similiar to a fully qualified directory entry.

--*/
{


    return FALSE;
}



STDMETHODIMP
CShellFolder::BindToStorage(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID *ppvOut
    )
/*++
    ...Reserved for a future use. This method should return E_NOTIMPL. ...

--*/
{
    return E_NOTIMPL;
}


STDMETHODIMP
CShellFolder::GetUIObjectOf(
    HWND hwndOwner,
    UINT cidl,
    LPCITEMIDLIST *pPidl,
    REFIID riid,
    LPUINT puReserved,
    LPVOID *ppvReturn
    )
/*++
    Creates a COM object that can be used to carry out actions on the
    specified file objects or folders, typically, to create context menus
    or carry out drag-and-drop operations.

--*/
{
    *ppvReturn = NULL;

    if(IsEqualIID(riid, IID_IExtractIcon)) {
        if(cidl != 1)
            return E_INVALIDARG;
        *ppvReturn = (IExtractIcon*)( new CExtractIcon( pPidl[0] ) );
    }
    else if(IsEqualIID(riid, IID_IContextMenu)) {
        if(cidl == 0)
            return E_INVALIDARG;
    }
    else if(IsEqualIID(riid, IID_IDataObject)) {
        if(cidl == 0)
            return E_INVALIDARG;
    }

    if(*ppvReturn == NULL)
        return E_NOINTERFACE;

    (*(LPUNKNOWN*)ppvReturn)->AddRef();
    return NOERROR;
}




STDMETHODIMP
CShellFolder::ParseDisplayName(
    HWND hwndOwner,
    LPBC pbcReserved,
    LPOLESTR lpDisplayName,
    LPDWORD pdwEaten,
    LPITEMIDLIST *pPidlNew,
    LPDWORD pdwAttributes
    )
{
    return E_NOTIMPL;
}


STDMETHODIMP
CShellFolder::SetNameOf(
    HWND hwndOwner,
    LPCITEMIDLIST pidl,
    LPCOLESTR lpName,
    DWORD dw,
    LPITEMIDLIST *pPidlOut
    )
{
    return E_NOTIMPL;
}


