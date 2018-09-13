///////////////////////////////////////////////////////////////////////////////
/*  File: prshtext.cpp

    Description: DSKQUOTA property sheet extention implementation.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
    07/03/97    Added m_hrInitialization member.                     BrianAu
    01/23/98    Removed m_hrInitialization member.                   BrianAu
    06/25/98    Disabled snapin code with #ifdef POLICY_MMC_SNAPIN.  BrianAu
                Switching to ADM-file approach to entering policy
                data.  Keeping snapin code available in case
                we decide to switch back at a later time.
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"  // PCH
#pragma hdrstop

#include "dskquota.h"
#include "prshtext.h" 
#include "guidsp.h"

extern LONG g_cLockThisDll;


DiskQuotaPropSheetExt::DiskQuotaPropSheetExt(
    VOID
    ) : m_cRef(0),
        m_dwDlgTemplateID(0),
        m_lpfnDlgProc(NULL),
        m_hPage(NULL),
        m_pQuotaControl(NULL),
        m_cOleInitialized(0)
{
    DBGTRACE((DM_PRSHTEXT, DL_HIGH, TEXT("DiskQuotaPropSheetExt::DiskQuotaPropSheetExt")));
    InterlockedIncrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaPropSheetExt::~DiskQuotaPropSheetExt

    Description: Destructor for the property sheet extension class.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DiskQuotaPropSheetExt::~DiskQuotaPropSheetExt(VOID)
{
    DBGTRACE((DM_PRSHTEXT, DL_HIGH, TEXT("DiskQuotaPropSheetExt::~DiskQuotaPropSheetExt")));

    if (NULL != m_pQuotaControl)
    {
        m_pQuotaControl->Release();
        m_pQuotaControl = NULL;
    }

    //
    // Call OleUninitialize for each time OleInitialize was called in Initialize().
    //
    while(0 != m_cOleInitialized--)
    {
        DBGASSERT((0 <= m_cOleInitialized)); // Make sure we don't go negative.
        CoUninitialize();
    }

    InterlockedDecrement(&g_cRefThisDll);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaPropSheetExt::QueryInterface

    Description: Returns an interface pointer to the object's IUnknown
        and IShellPropSheetExt interfaces.  

    Arguments:
        riid - Reference to requested interface ID.

        ppvOut - Address of interface pointer variable to accept interface ptr.

    Returns:
        NO_ERROR        - Success.
        E_NOINTERFACE   - Requested interface not supported.
        E_INVALIDARG    - ppvOut argument was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaPropSheetExt::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    HRESULT hResult = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IID_IUnknown == riid || 
        IID_IShellPropSheetExt == riid
#ifdef POLICY_MMC_SNAPIN
        || IID_ISnapInPropSheetExt == riid     // This is not a "real" interface.
#endif 
        )
    {
        *ppvOut = this;
    }

#ifdef POLICY_MMC_SNAPIN
    else if (IID_IDiskQuotaPolicy == riid)
    {
        hResult = CreateDiskQuotaPolicyObject(reinterpret_cast<IDiskQuotaPolicy **>(ppvOut));
    }
#endif

    if (NULL != *ppvOut)
    {
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hResult = NOERROR;
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaPropSheetExt::AddRef

    Description: Increments object reference count.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
DiskQuotaPropSheetExt::AddRef(
    VOID
    )
{
    ULONG ulReturn = m_cRef + 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("DiskQuotaPropSheetExt::AddRef, 0x%08X  %d -> %d\n"),
             this, ulReturn - 1, ulReturn));

    InterlockedIncrement(&m_cRef);

    return ulReturn;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaPropSheetExt::Release

    Description: Decrements object reference count.  If count drops to 0,
        object is deleted.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
DiskQuotaPropSheetExt::Release(
    VOID
    )
{
    ULONG ulReturn = m_cRef - 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("DiskQuotaPropSheetExt::Release, 0x%08X  %d -> %d\n"),
             this, ulReturn + 1, ulReturn));

    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaPropSheetExt::Initialize

    Description: Initializes a new property sheet extension object.

    Arguments:
        idVolume - Reference to a CVolumeID object containing both parsable
                   and displayable names for the volume.  In the case of
                   normal volumes, this is the same string.  In the case
                   of mounted volumes, it may not be depending on what's
                   provided by the OS for the mounted volume.  Most mounted
                   volumes have names like "\\?\Volume{ GUID }\".

        dwDlgTemplateID - Resource ID for the dialog template to use for the
            property sheet.

        lpfnDlgProc - Address of dialog's window message procedure.

    Returns:
        NO_ERROR            - Success.
        ERROR_ACCESS_DENIED (hr) - Read access denied to the device.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
    06/27/98    Replaced volume name arg with CVolumeID arg to       BrianAu
                support mounted volumes.
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
DiskQuotaPropSheetExt::Initialize(
    const CVolumeID& idVolume,
    DWORD dwDlgTemplateID,
    DLGPROC lpfnDlgProc
    )
{
    HRESULT hResult = NO_ERROR;

    DBGASSERT((NULL != lpfnDlgProc));
    DBGASSERT((0    != dwDlgTemplateID));

    m_idVolume        = idVolume;
    m_dwDlgTemplateID = dwDlgTemplateID;
    m_lpfnDlgProc     = lpfnDlgProc;

    //
    // Volume parsing name will be blank for a snap-in prop page since
    // it isn't displayed on behalf of any particular volume.
    //
    if (!m_idVolume.ForParsing().IsEmpty())
    {
        hResult = CoInitialize(NULL);
        if (SUCCEEDED(hResult))
        {
            IDiskQuotaControl *pqc;
            m_cOleInitialized++;  // Need to call OleUninitialize once more in dtor.

            //
            // Validate that we can use quotas by instantiating the quota control
            // object.  If this fails the user probably doesn't have access
            // to manipulate quotas.
            //
            hResult = GetQuotaController(&pqc);
            if (SUCCEEDED(hResult))
            {
                pqc->Release();
                //
                // Also release the cached m_pQuotaControl ptr.  
                // We don't want to hold open a handle to the volume if our
                // page is not active.
                //
                m_pQuotaControl->Release();
                m_pQuotaControl = NULL;
            }
        }
    }

    return hResult;
}

//
// Get a pointer to the IDiskQuotaControl interface.
// If the cached m_pQuotaControl ptr is non-NULL we merely AddRef this
// and return it to the caller. Otherwise we CoCreate a new controller,
// cache the pointer in m_pQuotaControl and return that.
// 
// History:  
//  Originally we opened the controller in ::Initialize() and cached
//  the pointer in m_pQuotaControl.  The controller remained alive 
//  until the prop SHEET was destroyed.  This was holding open a handle
//  to the volume device which prevented the disk checking function 
//  on the "Tools" page from accessing the volume.  Therefore I 
//  changed the code so that now we call GetQuotaController whenever
//  we want an IDiskQuotaControl pointer.  The caller releases that
//  ptr when done with it.  Whenever the prop page becomes inactive
//  we release the cached m_pQuotaControl.  This ensures that the 
//  code has the volume open only when the page is active.
//  [brianau - 5/21/99]
//  
//
HRESULT
DiskQuotaPropSheetExt::GetQuotaController(
    IDiskQuotaControl **ppqc
    )
{
    HRESULT hr = NOERROR;

    *ppqc = NULL;
    if (NULL == m_pQuotaControl)
    {
        //
        // No cached ptr.  Create a new controller.
        //
        hr = CoCreateInstance(CLSID_DiskQuotaControl,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDiskQuotaControl,
                              (LPVOID *)&m_pQuotaControl);

        if (SUCCEEDED(hr))
        {
            hr = m_pQuotaControl->Initialize(m_idVolume.ForParsing(), TRUE);
            if (FAILED(hr))
            {
                m_pQuotaControl->Release();
                m_pQuotaControl = NULL;
            }
        }
    }

    if (NULL != m_pQuotaControl)
    {
        //
        // Ptr is cached.  Merely addref and return it.
        //
        *ppqc = m_pQuotaControl;
        static_cast<IUnknown *>(*ppqc)->AddRef();
    }
    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaPropSheetExt::AddPages

    Description: Called by the shell when a page is to be added to the property
        sheet.

    Arguments:
        lpfnAddPage - Address of a callback function provided by the shell 
            that is to be called if the property page creation succedes.

        lParam - Parameter to pass to lpfnAddPage function.

    Returns:
        NO_ERROR            - Success.
        E_FAIL              - Failed to create or add page.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaPropSheetExt::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM lParam
    )
{
    HRESULT hResult = E_FAIL; // Assume failure.

    PROPSHEETPAGE psp;

    psp.dwSize          = sizeof(psp);
    psp.dwFlags         = PSP_USECALLBACK | PSP_USEREFPARENT;
    psp.hInstance       = g_hInstDll;
    psp.pszTemplate     = MAKEINTRESOURCE(m_dwDlgTemplateID);
    psp.hIcon           = NULL;
    psp.pszTitle        = NULL;
    psp.pfnDlgProc      = (DLGPROC)m_lpfnDlgProc;
    psp.lParam          = (LPARAM)this;
    psp.pcRefParent     = (UINT *)& g_cRefThisDll;
    psp.pfnCallback     = (LPFNPSPCALLBACK)PropSheetPageCallback;

    m_hPage = CreatePropertySheetPage(&psp);
    if (NULL != m_hPage)
    {
        if (!lpfnAddPage(m_hPage, lParam))
        {
            DBGERROR((TEXT("PRSHTEXT - Failed to add page.\n")));
            DestroyPropertySheetPage(m_hPage);
            m_hPage = NULL;
        }
    }
    else
    {
        DBGERROR((TEXT("PRSHTEXT - CreatePropertySheetPage failed.\n")));
    }
    if (NULL != m_hPage)
    {
        //
        // Incr ref count to keep the extension object alive.
        // The shell will release it as soon as the page is created.
        // We'll release it on PSPCB_RELEASE in PropSheetPageCallback.
        //
        AddRef();
        hResult = NO_ERROR;
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaPropSheetExt::PropSheetPageCallback

    Description: Called by the property sheet code when the property page
        is being created and again when it is being destroyed.  This gives the
        page a chance to act at these critical points.  We primarily use it
        to call Release() on the page extension which calls the virtual
        destructor, ultimately destroying the VolumePropPage or FolderPropPage
        object.

    Arguments:
        hwnd - Always NULL (according to SDK docs).

        uMsg - PSPCB_CREATE  = Creating page.
               PSPCB_RELEASE = Destroying page.

        ppsp - Pointer to the PROPSHEETPAGE structure for the page.

    Returns:
        Return value is ignore when uMsg is PSPCB_RELEASE.
        On PSPCB_CREATE, returning 0 instructs the PropertySheet to NOT
            display the page.  1 means OK to display page.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/12/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
UINT CALLBACK 
DiskQuotaPropSheetExt::PropSheetPageCallback(
    HWND hwnd,	
    UINT uMsg,	
    LPPROPSHEETPAGE ppsp	
    )
{
    UINT uReturn = 1;
    DiskQuotaPropSheetExt *pThis = (DiskQuotaPropSheetExt *)ppsp->lParam;
    DBGASSERT((NULL != pThis));

    switch(uMsg)
    {
        case PSPCB_CREATE:
            //
            // uReturn == 0 means Don't create the prop page.
            //
            uReturn = pThis->OnPropSheetPageCreate(ppsp);
            break;

        case PSPCB_RELEASE:
            //
            // uReturn is ignored for this uMsg.
            //
            pThis->OnPropSheetPageRelease(ppsp);
            //
            // This will release the extension and call the virtual
            // destructor (which will destroy the prop page object).
            //
            pThis->Release();
            CoFreeUnusedLibraries();
            break;
    }
    return uReturn;
}

