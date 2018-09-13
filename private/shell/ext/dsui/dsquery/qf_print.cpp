#include "pch.h"
#pragma hdrstop

#include <initguid.h>
#include "winprtp.h"

/*-----------------------------------------------------------------------------
/ Local functions / data
/----------------------------------------------------------------------------*/

static TCHAR c_szColor[]                    = TEXT("color");
static TCHAR c_szDuplex[]                   = TEXT("duplex");
static TCHAR c_szStaple[]                   = TEXT("stapling");
static TCHAR c_szResolution[]               = TEXT("resolution");
static TCHAR c_szSpeed[]                    = TEXT("speed");
static TCHAR c_szPaperSize[]                = TEXT("size");
static TCHAR c_szPrintPaperSize[]           = TEXT("(printMediaReady=%s*)");
static TCHAR c_szPrintResolution[]          = TEXT("(printMaxResolutionSupported>=%s)");
static TCHAR c_szPrintSpeed[]               = TEXT("(printPagesPerMinute>=%d)");
static TCHAR c_szLocationQuery[]            = TEXT("(location=%s*)");
static TCHAR c_szBlank[]                    = TEXT("");
static TCHAR c_szLocationTag[]              = TEXT("Location");
static TCHAR c_szDynamicTag[]               = TEXT("$DynamicLocation$");
static TCHAR c_szPrinterPolicy[]            = TEXT("Software\\Policies\\Microsoft\\Windows NT\\Printers");
static TCHAR c_szPhysicalLocationFeature[]  = TEXT("PhysicalLocationSupport");

static WCHAR c_szPrinterName[]              = L"printerName";
static WCHAR c_szServerName[]               = L"serverName";
static WCHAR c_szQueryPrefix[]              = L"(uncName=*)(objectCategory=printQueue)";
static WCHAR c_szPrintColor[]               = L"(printColor=TRUE)";
static WCHAR c_szPrintDuplex[]              = L"(printDuplexSupported=TRUE)";
static WCHAR c_szPrintStapling[]            = L"(printStaplingSupported=TRUE)";
static WCHAR c_szPrintModelProp[]           = L"driverName";

#define MAX_LOCATION_WAIT_TIME              30000
#define MAX_LOCATION_MSG_WAIT_TIME          60000
#define MAX_LOCATION                        MAX_PATH

static LPWSTR c_szClassList[] =
{
    L"printQueue",
};

static PAGECTRL ctrls1[] =
{
    IDC_PRINTNAME,     c_szPrinterName,     FILTER_CONTAINS,
    IDC_PRINTMODEL,    c_szPrintModelProp,  FILTER_CONTAINS,
};

static COLUMNINFO columns[] =
{
    0, 0, IDS_CN,          0, c_szPrinterName,
    0, 0, IDS_LOCATION,    0, c_szLocation,
    0, 0, IDS_MODEL,       0, c_szPrintModelProp,
    0, 0, IDS_SERVERNAME,  0, c_szServerName,
    0, DEFAULT_WIDTH_DESCRIPTION, IDS_COMMENT, 0, c_szDescription,
};

static struct
{
    INT     idString;
    LPCTSTR szString;
}
Resolutions [] =
{
    IDS_ANY,        NULL,
    IDS_72,         TEXT("72"),
    IDS_144,        TEXT("144"),
    IDS_300,        TEXT("300"),
    IDS_600,        TEXT("600"),
    IDS_1200,       TEXT("1200"),
    IDS_2400,       TEXT("2400"),
    IDS_4800,       TEXT("4800"),
    IDS_9600,       TEXT("9600"),
    IDS_32000,      TEXT("32000"),
};

#define IDH_NOHELP                      ((DWORD)-1) // Disables Help for a control
static const DWORD aFormHelpIDs[]=
{
    IDC_PRINTNAME,      IDH_PRINTER_NAME,
    IDC_PRINTLOCATION,  IDH_PRINTER_LOCATION,
    IDC_PRINTBROWSE,    IDH_PRINTER_LOCATION,
    IDC_PRINTMODEL,     IDH_PRINTER_MODEL,
    IDC_PRINTDUPLEX,    IDH_DOUBLE_SIDED,
    IDC_PRINTSTAPLE,    IDH_STAPLE,
    IDC_PRINTCOLOR,     IDH_PRINT_COLOR,
    IDC_PRINTPAGESIZE,  IDH_PAPER_SIZE,
    IDC_PRINTRES,       IDH_RESOLUTION,
    IDC_PRINTRES_POSTFIX, IDH_RESOLUTION,
    IDC_PRINTSPEED,     IDH_SPEED,
    IDC_PRINTSPEED_UPDN,IDH_SPEED,
    IDC_PRINTSPEED_POSTFIX, IDH_SPEED,
    IDC_SEPLINE,        IDH_NOHELP,
    0, 0,
};

/*-----------------------------------------------------------------------------
/ CPrintQueryPage class
/----------------------------------------------------------------------------*/
class CPrintQueryPage
{
public:

    CPrintQueryPage( HWND hwnd );
    ~CPrintQueryPage();
    HRESULT Initialize( HWND hwnd, BOOL bSynchronous );
    LPCTSTR GetSearchText( VOID );
    UINT AddRef( VOID );
    UINT Release( VOID );
    VOID TimerExpire();
    VOID EnableLocationEditText( HWND hwnd, BOOL bEnable );
    VOID LocationEditTextChanged( HWND hwnd );
    VOID BrowseForLocation( HWND hwnd );
    HRESULT PersistLocation(HWND hwnd, IPersistQuery* pPersistQuery, BOOL fRead);
    VOID OnInitDialog( HWND hwnd );

private:

    CPrintQueryPage( CPrintQueryPage &rhs );
    CPrintQueryPage & operator=( CPrintQueryPage &rhs );

    VOID WaitForLocation( HWND hwnd );
    DWORD Discovery( VOID );
    VOID TimerCreate( VOID );
    VOID TimerRelease( VOID );
    VOID SetLocationText( HWND hCtrl, LPCTSTR pszString, BOOL fReadOnly, BOOL fIgnoreWorkingText );
    static DWORD WINAPI _PhysicalLocationThread( PVOID pVoid );

    IPhysicalLocation *m_pPhysicalLocation;
    LPTSTR             m_pszPhysicalLocation;
    LONG               m_cRef;
    HWND               m_hCtrl;
    BOOL               m_fThreadCreated;
    BOOL               m_fComplete;
    BOOL               m_fLocationEnableState;
    BOOL               m_fLocationUserModified;
    BOOL               m_bValid;
    HWND               m_hwnd;
    UINT_PTR           m_hTimer;
    HANDLE             m_hComplete;
    LPTSTR             m_pszWorkingText;
};

/*-----------------------------------------------------------------------------
/ CPrintQueryPage
/ ---------------------
/   Constructor, creates the IPhysicalLocation object.  If we are returned
/   a good interface pointer indicates the class is valid.
/
/ In:
/   None.
/
/ Out:
/   Nothing.
/----------------------------------------------------------------------------*/
CPrintQueryPage::CPrintQueryPage( HWND hwnd )
    : m_pPhysicalLocation( NULL ),
      m_pszPhysicalLocation( NULL ),
      m_cRef( 1 ),
      m_hCtrl( NULL ),
      m_fThreadCreated( FALSE ),
      m_fComplete( FALSE ),
      m_hwnd( hwnd ),
      m_hTimer( NULL ),
      m_fLocationEnableState( TRUE ),
      m_fLocationUserModified( FALSE ),
      m_hComplete( NULL ),
      m_pszWorkingText( NULL ),
      m_bValid( FALSE )
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::CPrintQueryPage");

    //
    // The physical location feature can be disable using a group
    // policy setting.  If the feature is disabled we will just
    // fail to aquire the physical location interface and continue
    // operation with out pre-populating the location edit control.
    //
    HRESULT hr = CoCreateInstance( CLSID_PrintUIShellExtension,
                                   0,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IPhysicalLocation,
                                   (VOID**)&m_pPhysicalLocation );
    if (SUCCEEDED( hr ))
    {
        //
        // Check if the physical location policy is enabled.
        //
        if (SUCCEEDED(m_pPhysicalLocation->ShowPhysicalLocationUI()))
        {
            TimerCreate();

            m_hComplete = CreateEvent( NULL, TRUE, FALSE, NULL );

            if (m_hComplete)
            {
                //
                // Attempt to fetch the working text from the resource file.
                //
                TCHAR szBuffer[MAX_PATH] = {0};

                if (LoadString(GLOBAL_HINSTANCE, IDS_PRINT_WORKING_TEXT, szBuffer, ARRAYSIZE(szBuffer)))
                {
                    hr = LocalAllocString (&m_pszWorkingText, szBuffer);
                }
                else
                {
                    TraceAssert(FALSE);
                }

                //
                // Indicate the class is in a valid state, i.e. usable.
                //
                m_bValid = TRUE;
            }
        }
    }

    TraceLeave();
}

/*-----------------------------------------------------------------------------
/ ~CPrintQueryPage
/ ---------------------
/   Destructor, release the IPhysicalLocation object and the location string.
/
/ In:
/   None.
/
/ Out:
/   Nothing.
/----------------------------------------------------------------------------*/
CPrintQueryPage::~CPrintQueryPage()
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::~CPrintQueryPage");

    if (m_pPhysicalLocation)
    {
        m_pPhysicalLocation->Release();
    }

    LocalFreeString(&m_pszPhysicalLocation);

    //
    // Only release the string if it was allocated and it is not the null string.
    //
    if (m_pszWorkingText && (m_pszWorkingText != c_szBlank))
    {
        LocalFreeString(&m_pszWorkingText);
    }

    TimerRelease();

    if (m_hComplete)
    {
        CloseHandle( m_hComplete );
    }

    TraceLeave();
}

/*-----------------------------------------------------------------------------
/ AddRef
/ ---------------------
/   Increases the reference count of this object.   This is method is used to
/   control the life time of this class when a backgroud thread is used to fetch
/   the physical location string.
/
/ In:
/   None.
/
/ Out:
/   New object refrence count.
/----------------------------------------------------------------------------*/
UINT CPrintQueryPage::AddRef( VOID )
{
    return InterlockedIncrement(&m_cRef);
}

/*-----------------------------------------------------------------------------
/ Release
/ ---------------------
/   Decreases the reference count of this object.   This is method is used to
/   control the life time of this class when a backgroud thread is used to fetch
/   the physical location string.
/
/ In:
/   None.
/
/ Out:
/   New object refrence count.
/----------------------------------------------------------------------------*/
UINT CPrintQueryPage::Release (VOID)
{
    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

/*-----------------------------------------------------------------------------
/ GetSearchText
/ ---------------------
/   Returns a pointer to the current search text.  The search text is the
/   physical location path returned from the IPhysicalLocation object.  If either
/   the search text does not exist or not found this routine will return a
/   NULL string.
/
/ In:
/   None.
/
/ Out:
/   Ponter to the search text or the NULL string.
/----------------------------------------------------------------------------*/
LPCTSTR CPrintQueryPage::GetSearchText( VOID )
{
    return m_pszPhysicalLocation ? m_pszPhysicalLocation : c_szBlank;
}

/*-----------------------------------------------------------------------------
/ Initialize
/ ---------------------
/   Creates the background thread and calls the physical location discovery
/   method.
/
/ In:
/   Edit control window handle where to place text when done.
/   bSynchronous flag TRUE use backgroud thread, FALSE call synchronously.
/
/ Out:
/   HRESULT hr.
/----------------------------------------------------------------------------*/
HRESULT CPrintQueryPage::Initialize( HWND hwnd, BOOL bSynchronous )
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::Initialize");

    HRESULT hr          = S_OK;
    DWORD   dwThreadID  = 0;
    HANDLE  hThread     = NULL;

    //
    // If we have a valid physical location interface and the thread was not created,
    // then create it now and call the discovery method.
    //
    if (m_bValid && !m_fThreadCreated)
    {
        //
        // Bump the objects refrence count, needed for the async thread.
        //
        AddRef();

        //
        // Save the window handle in the class so the background thread
        // knows what window to set the location text to.
        //
        m_hCtrl = hwnd;

        //
        // Increase this libraries object refcount, ole will not unload until
        // we hit zeor and DllCanUnloadNow returns true.
        //
        InterlockedIncrement(&GLOBAL_REFCOUNT);

        //
        // Only create the thread once.
        //
        m_fThreadCreated = TRUE;

        //
        // If we are requested to do a synchronous call then just call the
        // thread proc directly.
        //
        if (bSynchronous)
        {
            hr = (_PhysicalLocationThread( this ) == ERROR_SUCCESS) ? S_OK : E_FAIL;
        }
        else
        {
            //
            // Create the background thread.
            //
            hThread = CreateThread( NULL,
                                    0,
                                    reinterpret_cast<LPTHREAD_START_ROUTINE>(CPrintQueryPage::_PhysicalLocationThread),
                                    reinterpret_cast<LPVOID>( this ),
                                    0,
                                    &dwThreadID);

            TraceAssert(hThread);

            //
            // If the thread failed creation clean up the dll refrence count
            // and the object refrence and the thread created flag.
            //
            if (!hThread)
            {
                m_fThreadCreated = FALSE;
                InterlockedDecrement(&GLOBAL_REFCOUNT);
                Release();
                hr = E_FAIL;
            }
            else
            {
                //
                // Thread is running just close the handle, we let the thread die
                // on its own normally.
                //
                CloseHandle(hThread);

                //
                // Indicate the request is pending.
                //
                hr = HRESULT_FROM_WIN32 (ERROR_IO_PENDING);
            }
        }
    }

    //
    // If we have a valid interface pointer and the background thread
    // has not completed then indicated the data is still pending.
    //
    else if(m_bValid && !m_fComplete)
    {
        //
        // Indicate the request is pending.
        //
        hr = HRESULT_FROM_WIN32 (ERROR_IO_PENDING);
    }

    //
    // If we failed with IO_PENDING then set the working text.
    //
    if (FAILED(hr) && HRESULT_CODE(hr) == ERROR_IO_PENDING)
    {
        //
        // Set the new location text.
        //
        SetLocationText (hwnd, m_pszWorkingText, TRUE, TRUE);
        PostMessage (m_hCtrl, EM_SETSEL, 0, 0);
    }

    TraceLeaveResult(hr);
}

/*-----------------------------------------------------------------------------
/ _PhysicalLocationThread
/ ---------------------
/   This routine is the backgroud thread thunk.  It accepts the CPrintQueryPage
/   this pointer and then calles the actual discovery method.  The purpose of
/   this routine is simple to capture the this pointer after the thread was
/   created and then invoke a method.
/
/ In:
/   Pointer to PrintQueryPage class.
/
/ Out:
/   TRUE success, FALSE error occurred.
/----------------------------------------------------------------------------*/
DWORD WINAPI CPrintQueryPage::_PhysicalLocationThread( PVOID pVoid )
{
    DWORD dwRetval = ERROR_OUTOFMEMORY;

    if ( SUCCEEDED(CoInitialize(NULL)) )
    {
        //
        // Get a pointer to this class.
        //
        CPrintQueryPage *pPrintQueryPage = reinterpret_cast<CPrintQueryPage *>( pVoid );

        //
        // Invoke the location discovery process.
        //
        dwRetval = pPrintQueryPage->Discovery();

        //
        // Set the completion event, in case someone is waiting.
        //
        SetEvent(pPrintQueryPage->m_hComplete);

        //
        // Indicate the discovery process completed.
        //
        pPrintQueryPage->m_fComplete = TRUE;

        //
        // Release the timer
        //
        pPrintQueryPage->TimerRelease();

        //
        // Release the refrence to the PrintQueryPage class.
        //
        pPrintQueryPage->Release();

        //
        // COM no longer needed
        //

        CoUninitialize();
    }

    InterlockedDecrement(&GLOBAL_REFCOUNT);
    return dwRetval;
}

/*-----------------------------------------------------------------------------
/ Discovery
/ ---------
/   This routine is the backgroud thread discovery process.  Since the act
/   of figuring out the physical location of this machin must hit the net
/   it can take a significant amount of time.  Hence we do this in a separate
/   thread.
/
/ In:
/   Nothing.
/
/ Out:
/   TRUE success, FALSE error occurred.
/----------------------------------------------------------------------------*/
DWORD CPrintQueryPage::Discovery( VOID )
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::Discovery");

    //
    // Start the discovery process for finding the physical location search text
    // for this machine.
    //
    HRESULT hr = m_pPhysicalLocation->DiscoverPhysicalLocation();

    if (SUCCEEDED( hr ))
    {
        BSTR pbsPhysicalLocation = NULL;

        //
        // Get the physical location search text.
        //
        hr = m_pPhysicalLocation->GetSearchPhysicalLocation( &pbsPhysicalLocation );

        //
        // If the error indicates the length was returned then allocate the text buffer.
        //
        if (SUCCEEDED( hr ) && pbsPhysicalLocation)
        {
            //
            // Release the previous string if any.
            //
            if (m_pszPhysicalLocation)
            {
                LocalFreeString(&m_pszPhysicalLocation);
            }

            //
            // Convert the BSTR location string to a TSTR string.
            //
            hr = LocalAllocStringW2T( &m_pszPhysicalLocation, pbsPhysicalLocation );
        }

        //
        // Release the physical location string if it was allocated.
        //
        if( pbsPhysicalLocation )
        {
            SysFreeString( pbsPhysicalLocation );
        }
    }

    //
    // Set the new location text.
    //
    SetLocationText( m_hCtrl, GetSearchText(), FALSE, FALSE );

    TraceLeaveValue(SUCCEEDED( hr ) ? ERROR_SUCCESS : ERROR_OUTOFMEMORY);
}

/*-----------------------------------------------------------------------------
/ WaitForLocation
/ ---------------------
/   Wait for the printer location information.
/
/ In:
/   hwnd parent window handle.
/
/ Out:
/   BOOL TRUE if success, FALSE if error.
/----------------------------------------------------------------------------*/
VOID CPrintQueryPage::WaitForLocation( HWND hwnd )
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::WaitForLocation");

    //
    // Only wait if we have a valid location interface pointer and
    // completion event handle was created and the thread is running.
    //
    if (m_bValid && m_hComplete && m_fThreadCreated)
    {
        //
        // Keep waiting until the physical location is avaialble or a timeout.
        //
        for (BOOL fExit = FALSE; !fExit; )
        {
            switch (MsgWaitForMultipleObjects(1, &m_hComplete, FALSE, MAX_LOCATION_MSG_WAIT_TIME, QS_ALLINPUT))
            {
            case WAIT_OBJECT_0:
                fExit = TRUE;
                break;

            case WAIT_TIMEOUT:
                fExit = TRUE;
                break;

            default:
                {
                    //
                    // Process any message now.
                    //
                    MSG msg;

                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                    break;
                }
            }
        }
    }

    TraceLeave();
}

/*-----------------------------------------------------------------------------
/ TimerCreate
/ ---------------------
/ Create the timer event to detect if the discovery method is taking too long.
/
/ In:
/
/ Out:
/----------------------------------------------------------------------------*/
VOID CPrintQueryPage::TimerCreate( VOID )
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::TimerCreate");

    if (!m_hTimer)
    {
        m_hTimer = SetTimer(m_hwnd, WM_USER, MAX_LOCATION_WAIT_TIME, NULL);
    }

    TraceLeave();
}

/*-----------------------------------------------------------------------------
/ TimerRelease
/ ---------------------
/ Release the timer event.
/
/ In:
/
/ Out:
/----------------------------------------------------------------------------*/
VOID CPrintQueryPage::TimerRelease( VOID )
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::TimerRelease");

    if (m_hTimer)
    {
        KillTimer(m_hwnd, m_hTimer);
        m_hTimer = NULL;
    }

    TraceLeave();
}

/*-----------------------------------------------------------------------------
/ TimerExpire
/ ---------------------
/
/ In:
/
/ Out:
/----------------------------------------------------------------------------*/
VOID CPrintQueryPage::TimerExpire( VOID )
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::TimerExpire");

    //
    // The search data is not complete
    //
    if (!m_fComplete)
    {
        //
        // Blank out the location text, it took too long to find.
        //
        SetLocationText(m_hCtrl, c_szBlank, FALSE, TRUE);

        //
        // Set the completion event, in case someone is waiting.
        //
        SetEvent(m_hComplete);

        //
        // Indicate the discovery process completed.
        //
        m_fComplete = TRUE;
    }

    //
    // Release the timer, the time is a one shot notification.
    //
    TimerRelease();

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ EnableLocationEditText
/ ---------------------
/   Enabled or disable the location edit text only if it is does not contain
/   the pending text.
/
/ In:
/   hwnd parent window handle.
/
/ Out:
/   BOOL TRUE if success, FALSE if error.
/----------------------------------------------------------------------------*/
VOID CPrintQueryPage::EnableLocationEditText( HWND hwnd, BOOL bEnable )
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::EnableLocationEditText");

    HWND hCtrl          = GetDlgItem(hwnd, IDC_PRINTLOCATION);
    HWND hBrowseCtrl    = GetDlgItem(hwnd, IDC_PRINTBROWSE);

    //
    // If the CPrintQueryPage is valid then handle the location
    // edit control differently.
    //
    if (m_bValid)
    {
        TCHAR szBuffer[MAX_LOCATION] = {0};

        //
        // Save the previous location enable state.
        //
        m_fLocationEnableState = bEnable;

        //
        // Get the current location text.
        //
        GetWindowText(hCtrl, szBuffer, ARRAYSIZE(szBuffer));

        //
        // Do not change the location edit control enable state when the
        // working text is there.  The reason for this is the text
        // is hard to read when the control is disabled, but when the
        // control is just read only the text is black not gray hence
        // eaiser to read.
        //
        if (!_tcscmp(szBuffer, m_pszWorkingText))
        {
            //
            // For an unknown reason the control with the location
            // text has the input focus, the default input focus
            // should be on the printer name therefore I will
            // set the focus here.
            //
            SetFocus(GetDlgItem(hwnd, IDC_PRINTNAME));
        }
        else
        {
            EnableWindow(hBrowseCtrl, bEnable);
            EnableWindow(hCtrl, bEnable);
        }
    }
    else
    {
        EnableWindow(hBrowseCtrl, bEnable);
        EnableWindow(hCtrl, bEnable);
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ LocationEditTextChanged
/ ---------------------
/
/ In:
/   hwnd parent window handle.
/
/ Out:
/   BOOL TRUE if success, FALSE if error.
/----------------------------------------------------------------------------*/
VOID CPrintQueryPage::LocationEditTextChanged( HWND hwnd )
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::LocationEditTextChanged");

    //
    // The search data is complete
    //
    if (m_fComplete)
    {
        m_fLocationUserModified = TRUE;
    }

    TraceLeave();
}

/*-----------------------------------------------------------------------------
/ PersistLocation
/ ---------------------
/
/ In:
/   hwnd parent window handle.
/
/ Out:
/   BOOL TRUE if success, FALSE if error.
/----------------------------------------------------------------------------*/
HRESULT CPrintQueryPage::PersistLocation(HWND hwnd, IPersistQuery* pPersistQuery, BOOL fRead)
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::PersistLocation");

    HRESULT hr                      = S_OK;
    TCHAR   szBuffer[MAX_LOCATION]  = {0};

    //
    // Get the control handle for the location edit control
    //
    HWND hCtrl = GetDlgItem(hwnd, IDC_PRINTLOCATION);

    //
    // Are we to read the persisted query string.
    //
    if (fRead)
    {
        //
        // Read the persisted location string.
        //
        hr = pPersistQuery->ReadString( c_szMsPrintersMore, c_szLocationTag, szBuffer, ARRAYSIZE( szBuffer ) );
        FailGracefully(hr, "Failed to read location state");

        //
        // Assume this is the exact string.
        //
        LPCTSTR pLocation = szBuffer;

        //
        // If the dynamic sentinal was found then wait for the dynamic location
        // text to be avaiable.
        //
        if (!_tcscmp(szBuffer, c_szDynamicTag))
        {
            WaitForLocation(hwnd);
            pLocation = GetSearchText();
        }

        //
        // Set the persisted location string in the query form.
        //
        SetLocationText(hCtrl, pLocation, FALSE, TRUE);
    }
    else
    {
        //
        // If the user modified the location text then save this text, otherwize
        // save a sentinal string which indicates we are to determine the location
        // dynamically when the persisted query is read back.
        //
        if (m_fLocationUserModified)
        {
            GetWindowText(hCtrl, szBuffer, ARRAYSIZE(szBuffer));
            hr = pPersistQuery->WriteString( c_szMsPrintersMore, c_szLocationTag, szBuffer );
            FailGracefully(hr, "Failed to write location state");
        }
        else
        {
            hr = pPersistQuery->WriteString( c_szMsPrintersMore, c_szLocationTag, c_szDynamicTag );
            FailGracefully(hr, "Failed to write location working state");
        }
    }

exit_gracefully:

    TraceLeaveResult(hr);
}

/*-----------------------------------------------------------------------------
/ SetLocationText
/ ---------------------
/
/ In:
/   hwnd parent window handle.
/
/ Out:
/   BOOL TRUE if success, FALSE if error.
/----------------------------------------------------------------------------*/
VOID CPrintQueryPage::SetLocationText( HWND hCtrl, LPCTSTR pszString, BOOL fReadOnly, BOOL fIgnoreWorkingText )
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::SetLocationText");

    if (IsWindow(hCtrl))
    {
        //
        // Is the CPrintQueryPage in a valid state.
        //
        if (m_bValid)
        {
            TCHAR szBuffer[MAX_LOCATION];

            //
            // Read the current location text.
            //
            GetWindowText(hCtrl, szBuffer, ARRAYSIZE(szBuffer));

            //
            // Stick the location string in the edit control if it contains working.
            //
            if (!_tcscmp(szBuffer, m_pszWorkingText) || fIgnoreWorkingText)
            {
                SetWindowText(hCtrl, pszString);
            }

            //
            // Reset the control to non read only state.
            //
            SendMessage(hCtrl, EM_SETREADONLY, fReadOnly, 0);

            //
            // Enable the control if the read only is disabled.
            //
            if (!fReadOnly)
            {
                //
                // Enable the edit control.
                //
                EnableWindow(hCtrl, m_fLocationEnableState);
            }

            //
            // Only enable the browse button when we have a location string
            // and the then control is not in read only mode.
            //
            EnableWindow(GetDlgItem(m_hwnd, IDC_PRINTBROWSE), !fReadOnly && m_fLocationEnableState);
        }
        else
        {
            //
            // If we are not using the location interface, just set the location text.
            //
            SetWindowText(hCtrl, pszString);
        }
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ BrowseForLocation
/ ---------------------
/   Starts the browse for location tree view and populates the edit control
/   with a valid selection.
/
/ In:
/   hwnd parent window handle.
/
/ Out:
/   Nothing.
/----------------------------------------------------------------------------*/
VOID CPrintQueryPage::BrowseForLocation( HWND hwnd )
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::BrowseForLocation");

    if (m_bValid)
    {
        BSTR    pbPhysicalLocation  = NULL;
        BSTR    pbDefaultLocation   = NULL;
        LPTSTR  pszPhysicalLocation = NULL;
        HRESULT hr                  = E_FAIL;
        TCHAR   szText[MAX_LOCATION]= {0};

        //
        // Convert the physical location to a BSTR for the IPhysicalLocation
        // object can pre-expand the browse tree.
        //
        if (GetWindowText(GetDlgItem(hwnd, IDC_PRINTLOCATION), szText, ARRAYSIZE(szText)))
        {
            pbDefaultLocation = T2BSTR(szText);
        }
        else
        {
            pbDefaultLocation = T2BSTR(m_pszPhysicalLocation);
        }

        //
        // Display the location tree.
        //
        hr = m_pPhysicalLocation->BrowseForLocation(hwnd, pbDefaultLocation, &pbPhysicalLocation);

        if(SUCCEEDED(hr) && pbPhysicalLocation)
        {
            //
            // Convert the BSTR location string to a TSTR string.
            //
            hr = LocalAllocStringW2T(&pszPhysicalLocation, pbPhysicalLocation);

            if(SUCCEEDED(hr))
            {
                //
                // Set the location text.
                //
                SetLocationText(m_hCtrl, pszPhysicalLocation, FALSE, TRUE);
            }

            //
            // Release the TCHAR physical location string.
            //
            LocalFreeString(&pszPhysicalLocation);

            //
            // Release the physical location string.
            //
            SysFreeString(pbPhysicalLocation);
        }

        //
        // Release the default locatin string.
        //
        SysFreeString(pbDefaultLocation);
    }

    TraceLeave();
}

/*-----------------------------------------------------------------------------
/ OnInitDialog
/ ---------------------
/   Set the UI's initial state, on down level machines the browse button is
/   removed and the edit control is stetched to match the size of the other
/   edit controls, i.e. name, model.
/
/ In:
/   hwnd parent window handle.
/
/ Out:
/   Nothing.
/----------------------------------------------------------------------------*/
VOID CPrintQueryPage::OnInitDialog( HWND hwnd )
{
    TraceEnter(TRACE_FORMS, "CPrintQueryPage::OnInitDialog");

    if (!m_bValid)
    {
        //
        // If the IPhysicalLocation interface is not available, hide the browse
        // button and extend the location edit control appropriately
        //
        RECT rcName     = {0};
        RECT rcLocation = {0};

        GetWindowRect (GetDlgItem (hwnd, IDC_PRINTNAME), &rcName);

        GetWindowRect (GetDlgItem (hwnd, IDC_PRINTLOCATION), &rcLocation);

        SetWindowPos (GetDlgItem (hwnd, IDC_PRINTLOCATION),
                      NULL,
                      0,0,
                      rcName.right - rcName.left,
                      rcLocation.bottom - rcLocation.top,
                      SWP_NOMOVE|SWP_NOZORDER);

        ShowWindow (GetDlgItem (hwnd, IDC_PRINTBROWSE), SW_HIDE);
    }

    TraceLeave();
}

/*-----------------------------------------------------------------------------
/ PopulateLocationEditText
/ ---------------------
/   Populates the location edit control with the default location of this
/   machine.
/
/ In:
/   hwnd parent window handle.
/
/ Out:
/   BOOL TRUE if success, FALSE if error.
/----------------------------------------------------------------------------*/
BOOL PopulateLocationEditText( HWND hwnd, BOOL bClearField )
{
    TraceEnter(TRACE_FORMS, "PopulateLocationEditText");

    CPrintQueryPage *pPrintQueryPage = reinterpret_cast<CPrintQueryPage *>(GetWindowLongPtr(hwnd, DWLP_USER));

    if (pPrintQueryPage)
    {
        HWND hCtrl = GetDlgItem(hwnd, IDC_PRINTLOCATION);

        HRESULT hr = pPrintQueryPage->Initialize( hCtrl, FALSE );

        if (SUCCEEDED( hr ))
        {
            if( bClearField )
            {
                SetWindowText( hCtrl, c_szBlank);
            }
            else
            {
                SetWindowText( hCtrl, pPrintQueryPage->GetSearchText( ));
            }
        }
    }

    TraceLeaveValue(TRUE);
}


#ifndef WINNT

/*-----------------------------------------------------------------------------
/ PopulatePrintPageSize
/ ---------------------
/   Eumerates all the pages size from this machine's print spooler.  This allows
/   a user to choose from a list of available forms rather than remembering the
/   name of the particular form.
/
/ In:
/   hwnd parent window handle.
/
/ Out:
/   BOOL TRUE if success, FALSE if error.
/----------------------------------------------------------------------------*/

BOOL PopulatePrintPageSize( HWND hwnd )
{
    TCHAR szBuffer[MAX_PATH];

    //
    // Fill the combo box.
    //
    for( UINT i = IDS_PRINT_FORM_BEGIN+1; i < IDS_PRINT_FORM_END; i++ )
    {
        if( !LoadString(GLOBAL_HINSTANCE, i, szBuffer, ARRAYSIZE(szBuffer)) )
        {
            TraceAssert(FALSE);
        }
        ComboBox_AddString( GetDlgItem( hwnd, IDC_PRINTPAGESIZE ), szBuffer );
    }

    //
    // Set the limit text in the form name edit control
    //
    ComboBox_LimitText( GetDlgItem( hwnd, IDC_PRINTPAGESIZE ), CCHFORMNAME-1 );

    //
    // Return success.
    //
    return TRUE;
}

#else

/*-----------------------------------------------------------------------------
/ bEnumForms
/ ----------
/   Enumerates the forms on the printer identified by the handle.
/
/ In:
/   IN HANDLE   hPrinter,
/   IN DWORD    dwLevel,
/   IN PBYTE   *ppBuff,
/   IN PDWORD   pcReturned
/
/ Out:
/   Pointer to forms array and count of forms in the array if
/   success, NULL ponter and zero number of forms if failure.
/   BOOL TRUE if success, FALSE if error.
/----------------------------------------------------------------------------*/
BOOL
bEnumForms(
    IN HANDLE       hPrinter,
    IN DWORD        dwLevel,
    IN PFORM_INFO_1 *ppFormInfo,
    IN PDWORD       pcReturned
    )
{
    BOOL            bReturn     = FALSE;
    DWORD           dwReturned  = 0;
    DWORD           dwNeeded    = 0;
    PBYTE           p           = NULL;
    BOOL            bStatus     = FALSE;

    //
    // Get buffer size for enum forms.
    //
    bStatus = EnumForms( hPrinter, dwLevel, NULL, 0, &dwNeeded, &dwReturned );

    //
    // Check if the function returned the buffer size.
    //
    if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
    {
        goto Cleanup;
    }

    //
    // If buffer allocation fails.
    //
    p = (PBYTE)LocalAlloc( LPTR, dwNeeded );

    if( p ==  NULL )
    {
        goto Cleanup;
    }

    //
    // Get the forms enumeration
    //
    bStatus = EnumForms( hPrinter, dwLevel, p, dwNeeded, &dwNeeded, &dwReturned );

    //
    // Copy back the buffer pointer and count.
    //
    if( bStatus )
    {
        bReturn     = TRUE;
        *ppFormInfo = (PFORM_INFO_1)p;
        *pcReturned = dwReturned;
    }

Cleanup:

    if( bReturn == FALSE )
    {
        //
        // Indicate failure.
        //
        *ppFormInfo = NULL;
        *pcReturned = 0;

        //
        // Release any allocated memory.
        //
        if ( p )
        {
            LocalFree( p );
        }
    }

    return bReturn;
}

/*-----------------------------------------------------------------------------
/ PopulatePrintPageSize
/ ----------------
/   Eumerates all the pages size from this machine's print spooler.  This allows
/   a user to choose from a list of available forms rather than remembering the
/   name of the particular form.
/
/ In:
/   hwnd parent window handle.
/
/ Out:
/   BOOL TRUE if success, FALSE if error.
/----------------------------------------------------------------------------*/
BOOL PopulatePrintPageSize( HWND hwnd )
{
    HANDLE          hServer     = NULL;
    PFORM_INFO_1    pFormInfo   = NULL;
    DWORD           FormCount   = 0;
    BOOL            bRetval     = FALSE;
    TCHAR           szBuffer[MAX_PATH];

    //
    // Open the local print server with default access.
    //
    BOOL bStatus = OpenPrinter( NULL, &hServer, NULL );

    if( bStatus )
    {
        //
        // Enumerate the forms.
        //
        bStatus = bEnumForms( hServer, 1, &pFormInfo, &FormCount );
    }

    if( bStatus && pFormInfo )
    {
        //
        // Fill the combo box.
        //
        for( UINT i = 0; i < FormCount; i++ )
        {
            ComboBox_AddString( GetDlgItem( hwnd, IDC_PRINTPAGESIZE ), pFormInfo[i].pName );
        }

        //
        // Set the limit text in the form name edit control
        //
        ComboBox_LimitText( GetDlgItem( hwnd, IDC_PRINTPAGESIZE ), CCHFORMNAME-1 );

        //
        // Return success.
        //
        bRetval = TRUE;
    }

    if( pFormInfo )
    {
        //
        // Release the forms buffer if it was allocated.
        //
        LocalFree( pFormInfo );
    }

    if ( hServer )
    {
        ClosePrinter(hServer);
    }

    return bRetval;
}

#endif

/*-----------------------------------------------------------------------------
/ PopulatePrintSpeed
/ ----------------
/ Set the print speed up down arrow control with an upper and lower
/ bound range.
/
/ In:
/   hwnd parent window handle
/
/ Out:
/   BOOL TRUE if success, FALSE if error.
/----------------------------------------------------------------------------*/
BOOL PopulatePrintSpeed( HWND hwnd )
{
    //
    // Set the print speed up down arrow range.
    //
    SendMessage( GetDlgItem( hwnd, IDC_PRINTSPEED_UPDN ), UDM_SETRANGE, 0, MAKELPARAM( 9999, 1 ) );
    Edit_LimitText(GetDlgItem(hwnd, IDC_PRINTSPEED), 4);
    return TRUE;
}


/*-----------------------------------------------------------------------------
/ PopulateResolution
/ ----------------
/ Fill the print resolution contrl with valid resolution information.
/
/ In:
/   hwnd
/
/ Out:
/   BOOL TRUE if success, FALSE if error.
/----------------------------------------------------------------------------*/
BOOL PopulatePrintResolution( HWND hwnd )
{
    TCHAR szBuffer[MAX_PATH];

    //
    // Fill in the print resolution combo-box.
    //
    for( INT i = 0; i < ARRAYSIZE( Resolutions ); i++ )
    {
        if( !LoadString(GLOBAL_HINSTANCE, Resolutions[i].idString, szBuffer, ARRAYSIZE(szBuffer)))
        {
            TraceAssert(FALSE);
        }
        ComboBox_AddString( GetDlgItem( hwnd, IDC_PRINTRES ), szBuffer );
    }

    return TRUE;
}

/*-----------------------------------------------------------------------------
/ GetPrinterMoreParameters.
/ ----------------
/ Build the query string from the controls on the printer more page.
/
/ In:
/   hwnd parent window handle.
/   pLen pointer to length of query string.
/   pszBuffer pointer to buffer where to return the query string.
/
/ Out:
/   Nothing.
/----------------------------------------------------------------------------*/
VOID GetPrinterMoreParameters( HWND hwnd, UINT *puLen, LPWSTR pszBuffer )
{
    USES_CONVERSION;
    TCHAR   szScratch[MAX_PATH] = {0};
    TCHAR   szText[MAX_PATH]    = {0};
    INT     i                   = 0;

    //
    // Read the check box states and build the query string.
    //
    if( Button_GetCheck( GetDlgItem( hwnd, IDC_PRINTDUPLEX ) ) == BST_CHECKED )
        PutStringElementW(pszBuffer, puLen, c_szPrintDuplex);

    if( Button_GetCheck( GetDlgItem( hwnd, IDC_PRINTCOLOR ) ) == BST_CHECKED )
        PutStringElementW(pszBuffer, puLen, c_szPrintColor);

    if( Button_GetCheck( GetDlgItem( hwnd, IDC_PRINTSTAPLE ) ) == BST_CHECKED )
        PutStringElementW(pszBuffer, puLen, c_szPrintStapling);

    //
    // Read the paper size setting.
    //
    ComboBox_GetText( GetDlgItem( hwnd, IDC_PRINTPAGESIZE ), szText, ARRAYSIZE( szText ) );

    if( lstrlen( szText ) )
    {
        wsprintf( szScratch, c_szPrintPaperSize, szText );
        PutStringElementW(pszBuffer, puLen, T2W(szScratch));
    }

    //
    // Read the printer resolution setting
    //
    i = ComboBox_GetCurSel( GetDlgItem( hwnd, IDC_PRINTRES ) );

    if( i > 0 && i < ARRAYSIZE( Resolutions ) )
    {
        wsprintf( szScratch, c_szPrintResolution, Resolutions[i].szString );
        PutStringElementW(pszBuffer, puLen, T2W(szScratch));
    }

    //
    // Read the printer speed setting
    //
    i = (LONG)SendMessage( GetDlgItem( hwnd, IDC_PRINTSPEED_UPDN ), UDM_GETPOS, 0, 0 );

    if( LOWORD(i) > 1 && i != -1 )
    {
        wsprintf( szScratch, c_szPrintSpeed, i );
        PutStringElementW(pszBuffer, puLen, T2W(szScratch));
    }
}

/*-----------------------------------------------------------------------------
/ GetPrinterLocationParameter.
/ ----------------
/ Build the query string from the location control on the printer page.
/
/ In:
/   hwnd parent window handle.
/   pLen pointer to length of query string.
/   pszBuffer pointer to buffer where to return the query string.
/
/ Out:
/   Nothing.
/----------------------------------------------------------------------------*/
VOID GetPrinterLocationParameter( HWND hwnd, UINT *puLen, LPWSTR pszBuffer )
{
    USES_CONVERSION;
    TCHAR   szScratch[MAX_PATH*2]   = {0};
    TCHAR   szText[MAX_PATH]        = {0};
    TCHAR   szWorkingText[MAX_PATH] = {0};

    HWND hCtrl = GetDlgItem(hwnd, IDC_PRINTLOCATION);

    if (GetWindowText(hCtrl, szText, ARRAYSIZE(szText)))
    {
        if (LoadString(GLOBAL_HINSTANCE, IDS_PRINT_WORKING_TEXT, szWorkingText, ARRAYSIZE(szWorkingText)))
        {
            if (_tcscmp(szText, szWorkingText))
            {
                wsprintf(szScratch, c_szLocationQuery, szText);

                PutStringElementW(pszBuffer, puLen, T2W(szScratch));
            }
            else
            {
                //
                // We are not going to wait the location field if the search process
                // has been kicked off. Just hit the expire timer to cancel the location
                // thread. This will ensure that the result list and the query params 
                // will be consistent.
                //
                CPrintQueryPage *pPrintQueryPage = reinterpret_cast<CPrintQueryPage *>(GetWindowLongPtr(hwnd, DWLP_USER));
                if (pPrintQueryPage)
                {
                    pPrintQueryPage->TimerExpire();
                }
            }
        }
        else
        {
            TraceAssert(FALSE);
        }
    }
}

/*-----------------------------------------------------------------------------
/ Query Page: Printers
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ PageProc_Printer
/ ----------------
/   PageProc for handling the messages for this object.
/
/ In:
/   pPage -> instance data for this form
/   hwnd = window handle for the form dialog
/   uMsg, wParam, lParam = message parameters
/
/ Out:
/   HRESULT (E_NOTIMPL) if not handled
/----------------------------------------------------------------------------*/
HRESULT CALLBACK PageProc_Printers(LPCQPAGE pPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    LPWSTR pQuery = NULL;
    UINT uLen = 0;

    TraceEnter(TRACE_FORMS, "PageProc_Printers");

    switch ( uMsg )
    {
        case CQPM_INITIALIZE:
        case CQPM_RELEASE:
            break;

        case CQPM_ENABLE:
        {
            CPrintQueryPage *pPrintQueryPage = reinterpret_cast<CPrintQueryPage *>(GetWindowLongPtr(hwnd, DWLP_USER));

            if (pPrintQueryPage)
            {
                pPrintQueryPage->EnableLocationEditText( hwnd, (BOOL)wParam );
            }

            // Enable the form controls,
            EnablePageControls(hwnd, ctrls1, ARRAYSIZE(ctrls1), (BOOL)wParam);
            break;
        }

        case CQPM_GETPARAMETERS:
        {
            //
            // Get the printer name and model paramters.
            //
            hr = GetQueryString(&pQuery, c_szQueryPrefix, hwnd, ctrls1, ARRAYSIZE(ctrls1));

            if ( SUCCEEDED(hr) )
            {
                hr = QueryParamsAlloc((LPDSQUERYPARAMS*)lParam, pQuery, GLOBAL_HINSTANCE, ARRAYSIZE(columns), columns);
                LocalFreeStringW(&pQuery);
            }

            //
            // Get the location parameter.
            //
            GetPrinterLocationParameter( hwnd, &uLen, NULL );

            if (uLen)
            {
                hr = LocalAllocStringLenW(&pQuery, uLen);

                if ( SUCCEEDED(hr) )
                {
                    GetPrinterLocationParameter( hwnd, &uLen, pQuery );
                    hr = QueryParamsAddQueryString((LPDSQUERYPARAMS*)lParam, pQuery );
                    LocalFreeStringW(&pQuery);
                }
            }

            FailGracefully(hr, "PageProc_Printers: Failed to build DS argument block");

            break;
        }

        case CQPM_CLEARFORM:
        {
            // Reset the form controls.
            PopulateLocationEditText( hwnd, TRUE );
            ResetPageControls(hwnd, ctrls1, ARRAYSIZE(ctrls1));
            break;
        }

        case CQPM_PERSIST:
        {
            BOOL fRead = (BOOL)wParam;
            IPersistQuery* pPersistQuery = (IPersistQuery*)lParam;

            CPrintQueryPage *pPrintQueryPage = reinterpret_cast<CPrintQueryPage *>(GetWindowLongPtr(hwnd, DWLP_USER));

            if (pPrintQueryPage)
            {
                hr = pPrintQueryPage->PersistLocation(hwnd, pPersistQuery, fRead);
            }

            if (SUCCEEDED(hr))
            {
                // Read the standard controls from the page,
                hr = PersistQuery(pPersistQuery, fRead, c_szMsPrinters, hwnd, ctrls1, ARRAYSIZE(ctrls1));
            }
            FailGracefully(hr, "Failed to persist page");
            break;
        }

        case CQPM_SETDEFAULTPARAMETERS:
        {
            //
            // so that the caller can pass parameters to the form we support an IPropertyBag in the
            // OPENQUERYWINDOW structure.   If wParam == TRUE, and lParam is non-zero then we
            // assume we should decode this structure to get the information we need from it.
            //

            if ( wParam && lParam )
            {
                OPENQUERYWINDOW *poqw = (OPENQUERYWINDOW*)lParam;
                if ( poqw->dwFlags & OQWF_PARAMISPROPERTYBAG )
                {
                    IPropertyBag *ppb = poqw->ppbFormParameters;
                    SetDlgItemFromProperty(ppb, L"printName", hwnd, IDC_PRINTNAME, NULL);
                    SetDlgItemFromProperty(ppb, L"printLocation", hwnd, IDC_PRINTLOCATION, NULL);
                    SetDlgItemFromProperty(ppb, L"printModel", hwnd, IDC_PRINTMODEL, NULL);
                }
            }

            break;
        }

        case CQPM_HELP:
        {
            LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
            WinHelp((HWND)pHelpInfo->hItemHandle,
                    DSQUERY_HELPFILE,
                    HELP_WM_HELP,
                    (DWORD_PTR)aFormHelpIDs);
            break;
        }

        case DSQPM_GETCLASSLIST:
        {
            hr = ClassListAlloc((LPDSQUERYCLASSLIST*)lParam, c_szClassList, ARRAYSIZE(c_szClassList));
            FailGracefully(hr, "Failed to allocate class list");
            break;
        }

        case DSQPM_HELPTOPICS:
        {
            HWND hwndFrame = (HWND)lParam;
#if DOWNLEVEL_SHELL
            TraceMsg("About to display help topics for find printers - dsclient.chm");
            HtmlHelp(hwndFrame, TEXT("dsclient.chm"), HH_HELP_FINDER, 0);
#else
            TraceMsg("About to display help topics for find printers - ocm.chm");
            HtmlHelp(hwndFrame, TEXT("omc.chm"), HH_HELP_FINDER, 0);
#endif            
            break;
        }

        default:
            hr = E_NOTIMPL;
            break;
    }

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ DlgProc_Printers
/ ----------------
/   Standard dialog proc for the form, handle any special buttons and other
/   such nastyness we must here.
/
/ In:
/   hwnd, uMsg, wParam, lParam = standard parameters
/
/ Out:
/   BOOL
/----------------------------------------------------------------------------*/
INT_PTR CALLBACK DlgProc_Printers(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR fResult = TRUE;

    CPrintQueryPage *pPrintQueryPage = reinterpret_cast<CPrintQueryPage *>(GetWindowLongPtr(hwnd, DWLP_USER));

    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            Edit_LimitText(GetDlgItem(hwnd, IDC_PRINTNAME),     MAX_PATH-1);
            Edit_LimitText(GetDlgItem(hwnd, IDC_PRINTLOCATION), MAX_LOCATION-1);
            Edit_LimitText(GetDlgItem(hwnd, IDC_PRINTMODEL),    MAX_PATH-1);

            pPrintQueryPage = new CPrintQueryPage(hwnd);

            if (pPrintQueryPage)
            {
                SetWindowLongPtr(hwnd, DWLP_USER, reinterpret_cast<LONG_PTR>(pPrintQueryPage));
                pPrintQueryPage->OnInitDialog(hwnd);
                PopulateLocationEditText(hwnd, FALSE);
            }
            else
            {
                fResult = FALSE;
            }
            break;
        }

        case WM_CONTEXTMENU:
        {
            WinHelp((HWND)wParam, DSQUERY_HELPFILE, HELP_CONTEXTMENU, (DWORD_PTR)aFormHelpIDs);
            break;
        }

        case WM_NCDESTROY:
        {
            if (pPrintQueryPage)
            {
                pPrintQueryPage->Release();
            }
            SetWindowLongPtr(hwnd, DWLP_USER, NULL);
            break;
        }

        case WM_TIMER:
        {
            if (pPrintQueryPage)
            {
                pPrintQueryPage->TimerExpire();
            }
            break;
        }

        case WM_COMMAND:
        {
            if((GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE) &&
               (GET_WM_COMMAND_ID(wParam, lParam) == IDC_PRINTLOCATION))
            {
                if (pPrintQueryPage)
                {
                    pPrintQueryPage->LocationEditTextChanged(hwnd);
                }
            }
            else if((GET_WM_COMMAND_ID(wParam, lParam) == IDC_PRINTBROWSE))
            {
                if (pPrintQueryPage)
                {
                    pPrintQueryPage->BrowseForLocation(hwnd);
                }
            }
            else
            {
                fResult = FALSE;
            }
            break;
        }

        default:
        {
            fResult = FALSE;
            break;
        }
    }

    return fResult;
}

/*-----------------------------------------------------------------------------
/ PageProc_PrintersMore
/ ---------------------
/   PageProc for handling the messages for this object.
/
/ In:
/   pPage -> instance data for this form
/   hwnd = window handle for the form dialog
/   uMsg, wParam, lParam = message parameters
/
/ Out:
/   HRESULT (E_NOTIMPL) if not handled
/----------------------------------------------------------------------------*/
HRESULT CALLBACK PageProc_PrintersMore(LPCQPAGE pPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT     hr          = S_OK;

    TraceEnter(TRACE_FORMS, "PageProc_PrintersMore");

    switch ( uMsg )
    {
        case CQPM_INITIALIZE:
        case CQPM_RELEASE:
            break;

        case CQPM_ENABLE:
            EnableWindow( GetDlgItem( hwnd, IDC_PRINTPAGESIZE ),    (BOOL)wParam );
            EnableWindow( GetDlgItem( hwnd, IDC_PRINTRES ),         (BOOL)wParam );
            EnableWindow( GetDlgItem( hwnd, IDC_PRINTSPEED ),       (BOOL)wParam );
            EnableWindow( GetDlgItem( hwnd, IDC_PRINTSPEED_UPDN ),  (BOOL)wParam );
            EnableWindow( GetDlgItem( hwnd, IDC_PRINTDUPLEX ),      (BOOL)wParam );
            EnableWindow( GetDlgItem( hwnd, IDC_PRINTCOLOR ),       (BOOL)wParam );
            EnableWindow( GetDlgItem( hwnd, IDC_PRINTSTAPLE ),      (BOOL)wParam );
            break;

        case CQPM_GETPARAMETERS:
        {
            LPWSTR pszBuffer = NULL;
            UINT uLen = 0;

            // Format the parameters for the 2nd page of the query form, this builds
            // an LDAP string and then appends it to the string we have in the
            // existing query parameter block.

            GetPrinterMoreParameters( hwnd, &uLen, NULL );

            if ( uLen )
            {
                hr = LocalAllocStringLenW(&pszBuffer, uLen);

                if ( SUCCEEDED(hr) )
                {
                    GetPrinterMoreParameters( hwnd, &uLen, pszBuffer );
                    hr = QueryParamsAddQueryString((LPDSQUERYPARAMS*)lParam, pszBuffer );
                    LocalFreeStringW(&pszBuffer);
                }

                FailGracefully(hr, "PageProc_PrintersMore: Failed to build DS argument block");
            }

            break;
        }

        case CQPM_CLEARFORM:
            SetDlgItemText( hwnd, IDC_PRINTPAGESIZE, TEXT("") );
            ComboBox_SetCurSel( GetDlgItem( hwnd, IDC_PRINTRES ), 0 );
            SendMessage( GetDlgItem( hwnd, IDC_PRINTSPEED_UPDN ), UDM_SETPOS, 0, MAKELPARAM( 1, 0 ) );
            Button_SetCheck( GetDlgItem( hwnd, IDC_PRINTDUPLEX ), BST_UNCHECKED );
            Button_SetCheck( GetDlgItem( hwnd, IDC_PRINTCOLOR ),  BST_UNCHECKED );
            Button_SetCheck( GetDlgItem( hwnd, IDC_PRINTSTAPLE ), BST_UNCHECKED );
            break;

        case CQPM_PERSIST:
        {
            IPersistQuery*  pPersistQuery   = (IPersistQuery*)lParam;
            BOOL            fRead           = (BOOL)wParam;
            INT             i               = 0;
            TCHAR           szBuffer[MAX_PATH];

            if ( fRead )
            {
                hr = pPersistQuery->ReadInt( c_szMsPrintersMore, c_szColor, &i );
                FailGracefully(hr, "Failed to read color state");
                Button_SetCheck( GetDlgItem( hwnd, IDC_PRINTCOLOR ), i ? BST_CHECKED : BST_UNCHECKED );

                hr = pPersistQuery->ReadInt( c_szMsPrintersMore, c_szDuplex, &i );
                FailGracefully(hr, "Failed to read duplex state");
                Button_SetCheck( GetDlgItem( hwnd, IDC_PRINTDUPLEX ), i ? BST_CHECKED : BST_UNCHECKED );

                hr = pPersistQuery->ReadInt( c_szMsPrintersMore, c_szStaple, &i );
                FailGracefully(hr, "Failed to read staple state");
                Button_SetCheck( GetDlgItem( hwnd, IDC_PRINTSTAPLE ), i ? BST_CHECKED : BST_UNCHECKED );

                hr = pPersistQuery->ReadInt( c_szMsPrintersMore, c_szResolution, &i );
                FailGracefully(hr, "Failed to read resolution state");
                ComboBox_SetCurSel( GetDlgItem( hwnd, IDC_PRINTRES ), i );

                hr = pPersistQuery->ReadInt( c_szMsPrintersMore, c_szSpeed, &i );
                FailGracefully(hr, "Failed to read speed state");
                SendMessage( GetDlgItem( hwnd, IDC_PRINTSPEED_UPDN ), UDM_SETPOS, 0, MAKELPARAM( i, 0 ) );

                hr = pPersistQuery->ReadString( c_szMsPrintersMore, c_szPaperSize, szBuffer, ARRAYSIZE( szBuffer ) );
                FailGracefully(hr, "Failed to read paper size state");
                ComboBox_SetText( GetDlgItem( hwnd, IDC_PRINTPAGESIZE ), szBuffer );

            }
            else
            {
                i = Button_GetCheck( GetDlgItem( hwnd, IDC_PRINTCOLOR ) ) == BST_CHECKED ? TRUE : FALSE;
                hr = pPersistQuery->WriteInt( c_szMsPrintersMore, c_szColor, i );
                FailGracefully(hr, "Failed to write color state");

                i = Button_GetCheck( GetDlgItem( hwnd, IDC_PRINTDUPLEX ) ) == BST_CHECKED ? TRUE : FALSE;
                hr = pPersistQuery->WriteInt( c_szMsPrintersMore, c_szDuplex, i );
                FailGracefully(hr, "Failed to write duplex state");

                i = Button_GetCheck( GetDlgItem( hwnd, IDC_PRINTSTAPLE ) ) == BST_CHECKED ? TRUE : FALSE;
                hr = pPersistQuery->WriteInt( c_szMsPrintersMore, c_szStaple, i );
                FailGracefully(hr, "Failed to write staple state");

                i = (INT)ComboBox_GetCurSel( GetDlgItem( hwnd, IDC_PRINTRES ) );
                hr = pPersistQuery->WriteInt( c_szMsPrintersMore, c_szResolution, i );
                FailGracefully(hr, "Failed to write resolution state");

                i = (INT)SendMessage( GetDlgItem( hwnd, IDC_PRINTSPEED_UPDN ), UDM_GETPOS, 0, 0 );
                hr = pPersistQuery->WriteInt( c_szMsPrintersMore, c_szSpeed, LOWORD(i) );
                FailGracefully(hr, "Failed to write speed state");

                ComboBox_GetText( GetDlgItem( hwnd, IDC_PRINTPAGESIZE ), szBuffer, ARRAYSIZE( szBuffer ) );
                hr = pPersistQuery->WriteString( c_szMsPrintersMore, c_szPaperSize, szBuffer );
                FailGracefully(hr, "Failed to write paper size state");

            }

            FailGracefully(hr, "Failed to persist page");

            break;
        }

        case CQPM_HELP:
        {
            LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
            WinHelp((HWND)pHelpInfo->hItemHandle,
                    DSQUERY_HELPFILE,
                    HELP_WM_HELP,
                    (DWORD_PTR)aFormHelpIDs);
            break;
        }

        case DSQPM_GETCLASSLIST:
            // the PageProc_Printers will have already handled this, no need to do it again! (daviddv, 19jun98)
            break;

        default:
            hr = E_NOTIMPL;
            break;
    }

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ DlgProc_Printers
/ ----------------
/   Standard dialog proc for the form, handle any special buttons and other
/   such nastyness we must here.
/
/ In:
/   hwnd, uMsg, wParam, lParam = standard parameters
/
/ Out:
/   INT_PTR
/----------------------------------------------------------------------------*/
INT_PTR CALLBACK DlgProc_PrintersMore(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR     fResult     = FALSE;
    LPCQPAGE    pQueryPage  = NULL;

    if ( uMsg == WM_INITDIALOG )
    {
        pQueryPage = (LPCQPAGE)lParam;

        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pQueryPage);

        //
        // Fill in the printer forms combo-box.
        //
        PopulatePrintPageSize( hwnd );

        //
        // Fill in the print speed combo-box.
        //
        PopulatePrintSpeed( hwnd );

        //
        // Fill in the print speed combo-box.
        //
        PopulatePrintResolution( hwnd );
    }
    else if ( uMsg == WM_CONTEXTMENU )
    {
        WinHelp((HWND)wParam, DSQUERY_HELPFILE, HELP_CONTEXTMENU, (DWORD_PTR)aFormHelpIDs);
        fResult = TRUE;
    }

    return fResult;
}
