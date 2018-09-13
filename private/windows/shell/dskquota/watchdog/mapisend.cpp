///////////////////////////////////////////////////////////////////////////////
/*  File: mapisend.cpp

    Description: Implements the most basic MAPI email client to send a message
        to one or more recipients.  All operations are done without UI.

        classes:    CMapiSession
                    CMapiRecipients
                    CMapiMessage
                    CMapiMessageBody
                    MAPI

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
    06/22/97    Added class MAPI.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "precomp.hxx"
#pragma hdrstop

#include "mapisend.h"


//
// Global MAPI object to provide dynamic linking to MAPI32.DLL.
// All MAPI32.DLL functions are called through this object.
//
MAPI MAPI32;


//
// How many entries we grow the recipients list by when it is enlarged.
//
#ifdef DEBUG
    //
    // For debugging and development, stress the list-growth code by making
    // it extend the list each time a recipient is added.
    //
    UINT CMapiRecipients::m_cGrowIncr = 1;
#else // !DEBUG
    //
    // For production builds, fix the growth increment at 3.  3 is arbitrary
    // but is probably a good conservative guess.  Want to avoid
    // list growth in the typical scenario.  
    //
    //      1 for the quota user.
    //      1 for the volume administrator.
    //      1 for the user's manager (? probably not)
    //
    // Note that the list growth code always runs at least once because
    // we use it to create the initial list.  
    //
    UINT CMapiRecipients::m_cGrowIncr = 3;
#endif // DEBUG

///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiRecipients::CMapiRecipients

    Description: Constructor.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
CMapiRecipients::CMapiRecipients(
    BOOL bUnicode
    ) : m_pal(NULL),
        m_bUnicode(bUnicode),
        m_cMaxEntries(0)
{

}

///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiRecipients::~CMapiRecipients

    Description: Destructor.
        Frees the MAPI address list.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
CMapiRecipients::~CMapiRecipients(
    VOID
    )
{
    if (NULL != m_pal)
    {
        MAPI32.FreePadrlist(m_pal);
    }
}

CMapiRecipients::CMapiRecipients(
    const CMapiRecipients& rhs
    ) : m_pal(NULL),
        m_bUnicode(FALSE),
        m_cMaxEntries(0)
{
#ifdef UNICODE
    m_bUnicode = TRUE;
#endif

    *this = rhs;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiRecipients::operator = (const CMapiRecipients& rhs)

    Description: Assignment copy.

    Arguments:
        rhs - Reference to source recipient list.

    Returns: Reference to destination recipient list.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
CMapiRecipients& 
CMapiRecipients::operator = (
    const CMapiRecipients& rhs
    )
{
    if (this != &rhs)
    {
        BOOL bConvertStrings = m_bUnicode != rhs.m_bUnicode;

        //
        // Delete the current address list.
        //
        if (NULL != m_pal)
        {
            MAPI32.FreePadrlist(m_pal);
            m_pal = NULL;
        }

        //
        // Copy the Max entry count.
        //
        // NOTE:  We DO NOT copy the m_bUnicode attribute.
        //        This attribute stays with the object for life.
        //
        m_cMaxEntries = rhs.m_cMaxEntries;

        if (NULL != rhs.m_pal)
        {
            HRESULT hr = E_FAIL;
            UINT cb;

            cb = sizeof(ADRLIST) + ((m_cMaxEntries - 1) * sizeof(ADRENTRY));
            hr = MAPI32.AllocateBuffer(cb, (LPVOID *)&m_pal);
            if (SUCCEEDED(hr))
            {
                ZeroMemory(m_pal, cb); // Note: m_pal->cEntries is init'd to 0.
                if (NULL != m_pal)
                {
                    for (UINT i = 0; i < rhs.m_pal->cEntries && SUCCEEDED(hr); i++)
                    {
                        hr = CopyAdrListEntry(m_pal->aEntries[i], 
                                              rhs.m_pal->aEntries[i], 
                                              bConvertStrings);
                        if (SUCCEEDED(hr))
                        {
                            m_pal->cEntries++;
                        }
                    }
                }
            }
            if (FAILED(hr))
            {
                //
                // Something went wrong.  Leave the object in an empty state.
                //
                if (NULL != m_pal)
                    MAPI32.FreePadrlist(m_pal);

                m_pal         = NULL;
                m_cMaxEntries = 0;
            }
        }
    }
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiRecipients::Count

    Description: Returns the count of valid entries in the address list.

    Arguments: None.

    Returns: Count of entries.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT 
CMapiRecipients::Count(
    VOID
    ) const
{
    return m_pal ? m_pal->cEntries : 0;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiRecipients::AddRecipient

    Description: Adds a new recipient/recipient-type pair to the address list.

    Arguments:
        pszEmailName - Name of recipient typically used as an email destination.

        dwType - Recipient type.  Can be one of the following:

            MAPI_ORIG
            MAPI_TO
            MAPI_CC
            MAPI_BCC

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CMapiRecipients::AddRecipient(
    LPCTSTR pszEmailName, 
    DWORD dwType
    )
{
    HRESULT hr = NO_ERROR;
    
    if (NULL == m_pal || m_pal->cEntries == m_cMaxEntries)
    {
        //
        // Either we're just starting up (no list created yet),
        // or the current list is full.  Grow the list.
        //
        hr = Grow(m_cGrowIncr);
    }
    if (SUCCEEDED(hr) && NULL != m_pal)
    {
        UINT cb;
        INT i = m_pal->cEntries++;

        m_pal->aEntries[i].ulReserved1 = 0;
        m_pal->aEntries[i].cValues     = 2;

        //
        // Allocate the SPropValue buffer for this new entry.
        // Our entries have 2 values (name and type).
        // Caller must call IAddrBook::ResolveName() to get the remaining
        // recipient information.
        //
        cb = sizeof(SPropValue) * m_pal->aEntries[i].cValues;
        hr = MAPI32.AllocateBuffer(cb, (LPVOID *)&m_pal->aEntries[i].rgPropVals);
        if (SUCCEEDED(hr))
        {
            ZeroMemory(m_pal->aEntries[i].rgPropVals, cb);
            //
            // Allocate the buffer for the recipient email name string.
            //
            hr = MAPI32.AllocateMore((lstrlen(pszEmailName)+1) * sizeof(TCHAR),
                                    (LPVOID)m_pal->aEntries[i].rgPropVals,
                                    (LPVOID *)&m_pal->aEntries[i].rgPropVals[0].Value.LPSZ);
            if (SUCCEEDED(hr))
            {
                //
                // Store the recipient email name string.
                //
                m_pal->aEntries[i].rgPropVals[0].ulPropTag  = PR_DISPLAY_NAME;
                lstrcpy(m_pal->aEntries[i].rgPropVals[0].Value.LPSZ, pszEmailName);

                //
                // Store the recipient type (i.e. MAPI_TO, MAPI_CC etc).
                //
                m_pal->aEntries[i].rgPropVals[1].ulPropTag  = PR_RECIPIENT_TYPE;
                m_pal->aEntries[i].rgPropVals[1].Value.l    = dwType;
            }
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiRecipients::Grow

    Description: Increases the size of the address list, preserving any
        existing list entries.

    Arguments:
        cGrowIncr - Number of entries to grow the list by.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiRecipients::Grow(
    UINT cGrowIncr
    )
{
    HRESULT hr         = E_FAIL;
    LPADRLIST m_palNew = NULL;
    UINT cb;

    //
    // Allocate the new buffer m_cGrowIncr entries larger than the
    // current buffer.  The (-1) is because the declaration of ADRLIST already
    // includes one entry.
    //
    cb = sizeof(ADRLIST) + ((m_cMaxEntries + cGrowIncr - 1) * sizeof(ADRENTRY));
    hr = MAPI32.AllocateBuffer(cb, (LPVOID *)&m_palNew);
    if (SUCCEEDED(hr))
    {
        ZeroMemory(m_palNew, cb); // Note: m_palNew->cEntries is init'd to 0.
        if (NULL != m_pal)
        {
            //
            // We have an existing address list.
            // Copy it to the new list buffer.
            // If we fail the copy of an entry, we abort the loop and m_palNew->cEntries
            // accurately reflects how many valid entries we have.
            //
            for (UINT i = 0; i < m_pal->cEntries && SUCCEEDED(hr); i++)
            {
                hr = CopyAdrListEntry(m_palNew->aEntries[i], m_pal->aEntries[i], FALSE);
                if (SUCCEEDED(hr))
                {
                    m_palNew->cEntries++;
                }
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        //
        // Delete the original list (if it exists) and store the
        // address of the new list in m_pal.
        //
        LPADRLIST palOrig = m_pal;
        m_pal = m_palNew;
        if (NULL != palOrig)
            MAPI32.FreePadrlist(palOrig);

        m_cMaxEntries += cGrowIncr;
    }
    else
    {
        //
        // Something went wrong.  Just delete the new list and keep the old one
        // the way it was.
        //
        if (NULL != m_palNew)
            MAPI32.FreePadrlist(m_palNew);
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiRecipients::CopyAdrListEntry

    Description: Copies a single list entry from one entry structure to another.
        All entry-type-specific issues are addressed.

    Arguments:
        Dest - Reference to destination entry structure.

        Src - Reference to source entry structure.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiRecipients::CopyAdrListEntry(
    ADRENTRY& Dest,
    ADRENTRY& Src,
    BOOL bConvertStrings
    )
{
    HRESULT hr;
    UINT cb = 0;
    
    //
    // Allocate buffer for the new entry and it's property values.
    //
    cb = sizeof(SPropValue) * Src.cValues;
    hr = MAPI32.AllocateBuffer(cb, (LPVOID *)&Dest.rgPropVals);
    if (SUCCEEDED(hr))
    {
        ZeroMemory(Dest.rgPropVals, cb);
        Dest.cValues = 0;
        //
        // Copy each of the values.
        // If we fail the copy of a value, we abort the loop and Dest.cValues
        // accurately reflects how many valid prop values we have.
        //
        for (UINT i = 0; i < Src.cValues && SUCCEEDED(hr); i++)
        {
            hr = CopySPropVal((LPVOID)Dest.rgPropVals, // Base for new allocations
                              Dest.rgPropVals[i],      // Destination SPropVal
                              Src.rgPropVals[i],       // Source SPropVal
                              bConvertStrings);
            if (SUCCEEDED(hr))
            {
                Dest.cValues++;
            }
        }
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiRecipients::CopySPropVal

    Description: Copies a single property value from one SPropValue object
        to another.  Used by CopyAdrListEntry to copy the individual properties
        belonging to an entry.

    Arguments:
        pvBaseAlloc - Pointer to use as the "lpObject" argument in a call
            to MAPIAllocateMore() if memory must be reallocated during
            the copy process.

        Dest - Reference to destination property value structure.

        Src - Reference to source property value structure.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiRecipients::CopySPropVal(
    LPVOID pvBaseAlloc,
    SPropValue& Dest,
    SPropValue& Src,
    BOOL bConvertStrings
    )
{
    HRESULT hr = NO_ERROR;
    BOOL bCopyTag = TRUE;

    //
    // Copy method varies depending on property type.
    //
    switch(PROP_TYPE(Src.ulPropTag))
    {
        case PT_I2:
            Dest.Value.i = Src.Value.i;
            break;
        case PT_LONG:
            Dest.Value.l = Src.Value.l;
            break;
        case PT_R4:
            Dest.Value.flt = Src.Value.flt;
            break;
        case PT_DOUBLE:
            Dest.Value.dbl = Src.Value.dbl;
            break;
        case PT_BOOLEAN:
            Dest.Value.b = Src.Value.b;
            break;
        case PT_CURRENCY:
            Dest.Value.cur = Src.Value.cur;
            break;
        case PT_APPTIME:
            Dest.Value.at = Src.Value.at;
            break;
        case PT_SYSTIME:
            Dest.Value.ft = Src.Value.ft;
            break;
        case PT_I8:
            Dest.Value.li = Src.Value.li;
            break;
        case PT_ERROR:
            Dest.Value.err = Src.Value.err;
            break;
        case PT_NULL:
        case PT_OBJECT:
            Dest.Value.x = Src.Value.x;
            break;
        case PT_STRING8:
            if (bConvertStrings && m_bUnicode)
            {
                //
                // The recipients list is unicode, the source is ANSI
                // and we're supposed to convert strings.  That means
                // we need to convert from Ansi to Unicode.
                //
                hr = CopySPropValString(pvBaseAlloc,
                                        &Dest.Value.lpszW,
                                        Src.Value.lpszA);

                Dest.ulPropTag = PROP_TAG(PT_UNICODE, PROP_ID(Src.ulPropTag));
                bCopyTag       = FALSE;
            }
            else
            {
                //
                // No conversion required.  Just a straight copy.
                //
                hr = CopyVarLengthSPropVal(pvBaseAlloc, 
                                           (LPVOID *)&Dest.Value.lpszA, 
                                           Src.Value.lpszA,
                                           (lstrlenA(Src.Value.lpszA)+1) * sizeof(char));
            }
            break;
                
        case PT_UNICODE:
            if (bConvertStrings && !m_bUnicode)
            {
                //
                // The recipients list is Ansi, the source is Unicode
                // and we're supposed to convert strings.  That means
                // we need to convert from Unicode to Ansi.
                //
                hr = CopySPropValString(pvBaseAlloc,
                                        &Dest.Value.lpszA,
                                        Src.Value.lpszW);

                Dest.ulPropTag = PROP_TAG(PT_STRING8, PROP_ID(Src.ulPropTag));
                bCopyTag       = FALSE;
            }
            else
            {
                //
                // No conversion required.  Just a straight copy.
                //
                hr = CopyVarLengthSPropVal(pvBaseAlloc, 
                                           (LPVOID *)&Dest.Value.lpszW, 
                                           Src.Value.lpszW,
                                           (lstrlenW(Src.Value.lpszW)+1) * sizeof(WCHAR));
            }
            break;

        case PT_BINARY:
            hr = CopyVarLengthSPropVal(pvBaseAlloc, 
                                       (LPVOID *)&Dest.Value.bin.lpb, 
                                       Src.Value.bin.lpb,
                                       Src.Value.bin.cb);
            break;
        case PT_CLSID:
            hr = CopyVarLengthSPropVal(pvBaseAlloc, 
                                       (LPVOID *)&Dest.Value.lpguid, 
                                       Src.Value.lpguid,
                                       sizeof(*(Src.Value.lpguid)));
            break;
        default:
            //
            // BUGBUG:  Assert here so we know if we should be handling
            //          some other property type.
            //
            hr = E_FAIL;
            break;
    }
    if (SUCCEEDED(hr))
    {
        //
        // Copy the common stuff.
        //
        Dest.dwAlignPad = Src.dwAlignPad;
        if (bCopyTag)
        {
            Dest.ulPropTag  = Src.ulPropTag;
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiRecipients::CopyVarLengthSPropVal

    Description: Copies a single variable-length property value item from
        one location in memory to another.  Memory for the destination is
        automatically allocated.  This function is used to copy prop values
        of types STRING, BINARY etc.

    Arguments:
        pvBaseAlloc - Pointer to use as the "lpObject" argument in a call
            to MAPIAllocateMore().

        ppvDest - Address of pointer variable to receive the address of  
            the newly allocated buffer.

        pvSrc - Address of source buffer.

        cb - Number of bytes to copy from pvSrc to *ppvDest.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiRecipients::CopyVarLengthSPropVal(
    LPVOID pvBaseAlloc,
    LPVOID *ppvDest,
    LPVOID pvSrc,
    INT cb
    )
{
    HRESULT hr;
             
    hr = MAPI32.AllocateMore(cb, pvBaseAlloc, ppvDest);
    if (SUCCEEDED(hr))
    {
        CopyMemory(*ppvDest, pvSrc, cb);
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiRecipients::CopySPropValString (ANSI -> UNICODE)

    Description: Copies a string property value item from one location
        to another; performing ANSI/UNICODE translations as required.

    Arguments:
        pvBaseAlloc - Pointer to use as the "lpObject" argument in a call
            to MAPIAllocateMore().

        ppszDestW - Address of pointer variable to receive the address of  
            the newly wide character buffer.

        pszSrcA - Address of ANSI source buffer.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiRecipients::CopySPropValString(
    LPVOID pvBaseAlloc,
    LPWSTR *ppszDestW,
    LPCSTR pszSrcA
    )
{
    HRESULT hr;

    INT cchW = MultiByteToWideChar(CP_ACP,
                                   0,
                                   pszSrcA,
                                   -1,
                                   NULL,
                                   0);

    hr = MAPI32.AllocateMore(cchW * sizeof(WCHAR), pvBaseAlloc, (LPVOID *)ppszDestW);
    if (SUCCEEDED(hr))
    {
        MultiByteToWideChar(CP_ACP,
                            0,
                            pszSrcA,
                            -1,
                            *ppszDestW,
                            cchW);
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiRecipients::CopySPropValString (UNICODE -> ANSI)

    Description: Copies a string property value item from one location
        to another; performing ANSI/UNICODE translations as required.

    Arguments:
        pvBaseAlloc - Pointer to use as the "lpObject" argument in a call
            to MAPIAllocateMore().

        ppszDestA - Address of pointer variable to receive the address of  
            the newly ANSI character buffer.

        pszSrcW - Address of UNICODE source buffer.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiRecipients::CopySPropValString(
    LPVOID pvBaseAlloc,
    LPSTR *ppszDestA,
    LPCWSTR pszSrcW
    )
{
    HRESULT hr;

    INT cchA = WideCharToMultiByte(CP_ACP,
                                   0,
                                   pszSrcW,
                                   -1,
                                   NULL,
                                   0,
                                   NULL, NULL);

    hr = MAPI32.AllocateMore(cchA * sizeof(CHAR), pvBaseAlloc, (LPVOID *)ppszDestA);
    if (SUCCEEDED(hr))
    {
        WideCharToMultiByte(CP_ACP,
                            0,
                            pszSrcW,
                            -1,
                            *ppszDestA,
                            cchA,
                            NULL, NULL);
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiSession::CMapiSession

    Description: Constructor.

    Arguments: None.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
CMapiSession::CMapiSession(
    VOID
    ) : m_pSession(NULL),
        m_pDefMsgStore(NULL),
        m_pOutBoxFolder(NULL),
        m_bMapiInitialized(FALSE)
{
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiSession::CMapiSession

    Description: Constructor.  Initializes MAPI.

    Arguments: None.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
CMapiSession::~CMapiSession(
    VOID
    )
{
    if (NULL != m_pOutBoxFolder)
    {
        m_pOutBoxFolder->Release();
    }
    if (NULL != m_pDefMsgStore)
    {
        m_pDefMsgStore->Release();
    }
    if (NULL != m_pSession)
    {
        m_pSession->Release();
    }
    if (m_bMapiInitialized)
    {
        //
        // If MAPI was initialized, uninitialize it.
        //
        MAPI32.Uninitialize();
        //
        // Unload MAPI32.DLL.
        //
        MAPI32.Unload();
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiSession::Initialize

    Description: Initializes the session object by:
    
        1. Initializing MAPI if neccessary, 
        2. Logging on to create the MAPI session object.
        3. Open the default msg store.
        4. Open the outbox folder in the default msg store.

    Arguments: None.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CMapiSession::Initialize(
    VOID
    )
{
    HRESULT hr = NO_ERROR;

    if (!m_bMapiInitialized)
    {
        //
        // Load MAPI32.DLL.
        //
        MAPI32.Load();

        //
        // Initialize MAPI if it hasn't been initialized.
        //
        hr = MAPI32.Initialize(NULL);
        if (SUCCEEDED(hr))
        {
            //
            // Remember that MAPI has been initialized.
            //
            m_bMapiInitialized = TRUE;
        }
    }
    if (m_bMapiInitialized)
    {
        //
        // Attempt logon only if MAPI has been initialized.
        //
        DWORD dwLogonFlags = MAPI_TIMEOUT_SHORT | MAPI_USE_DEFAULT | MAPI_EXTENDED | MAPI_ALLOW_OTHERS;
#ifdef UNICODE
        dwLogonFlags |= MAPI_UNICODE;
#endif
        if (NULL != m_pSession)
        {
            //
            // Release any previously-held session interface ptr.
            //
            m_pSession->Release();
            m_pSession = NULL;
        }
        hr = MAPI32.LogonEx(0,             // Hwnd for any UI.
                            NULL,          // Profile name.
                            NULL,          // Password.
                            dwLogonFlags,  // Flags
                            &m_pSession);  // Session obj ptr (out).

        if (SUCCEEDED(hr))
        {
            ReportErrorsReturned(hr);
            //
            // We're logged on.  Open the default msg store and out box folder.
            //
            hr = OpenDefMsgStore();
            if (SUCCEEDED(hr))
            {
                hr = OpenOutBoxFolder();
            }
        }
    }
    return hr;
}
    

///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiSession::OpenDefMsgStore

    Description: Opens the session's default message store.
        Stores the resulting IMsgStore ptr in m_pDefMsgStore.

    Arguments: None.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiSession::OpenDefMsgStore(
    VOID
    )
{
    HRESULT hr = E_FAIL;
    SPropValue spv;

    SizedSPropTagArray(2, sptCols) = {2, PR_ENTRYID, PR_DEFAULT_STORE};
    SRestriction sres;

    if (NULL != m_pSession)
    {
        LPMAPITABLE pTable;
        hr = m_pSession->GetMsgStoresTable(0, &pTable);
        ReportErrorsReturned(hr);
        if (SUCCEEDED(hr))
        {
            //
            // Find the entry ID for the default store in the session's
            // msg stores table.
            //
            LPSRowSet pRow = NULL;

            sres.rt                        = RES_PROPERTY;
            sres.res.resProperty.relop     = RELOP_EQ;
            sres.res.resProperty.ulPropTag = PR_DEFAULT_STORE;
            sres.res.resProperty.lpProp    = &spv;

            spv.ulPropTag = PR_DEFAULT_STORE;
            spv.Value.b   = TRUE;

            hr = MAPI32.HrQueryAllRows(pTable,                   // Table ptr.
                                      (LPSPropTagArray)&sptCols, // Column set
                                      &sres,                     // Row restrictions
                                      NULL,                      // Sort order set
                                      0,                         // All rows.
                                      &pRow);                    // Resulting row set (out)
  
            ReportErrorsReturned(hr);
            if (SUCCEEDED(hr))
            {
                SBinary sbEID = {0, NULL};

                if (NULL != pRow &&
                    0 != pRow->cRows &&
                    0 != pRow->aRow[0].cValues &&
                    PR_ENTRYID == pRow->aRow[0].lpProps[0].ulPropTag)
                {
                    sbEID = pRow->aRow[0].lpProps[0].Value.bin;

                    //
                    // Found the ID.  Now open the store.
                    //
                    if (NULL != m_pDefMsgStore)
                    {
                        //
                        // Release any previously-held store interface ptr.
                        //
                        m_pDefMsgStore->Release();
                        m_pDefMsgStore = NULL;
                    }
                    hr = m_pSession->OpenMsgStore(0,                    // Hwnd for any UI
                                                  sbEID.cb,             // Entry ID size.
                                                  (LPENTRYID)sbEID.lpb, // Entry ID ptr.
                                                  NULL,                 // Use Std iface.
                                                  MAPI_BEST_ACCESS,     // read/write access
                                                  &m_pDefMsgStore);     // Store ptr (out)

                    ReportErrorsReturned(hr);
                }
                else
                {
                    hr = MAPI_E_NOT_FOUND;
                }
                MAPI32.FreeProws(pRow);
            }
            pTable->Release();
        }
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiSession::GetSessionUser

    Description: Returns the address properties for the session user.

    Arguments: None.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiSession::GetSessionUser(
    LPSPropValue *ppProps,
    ULONG        *pcbProps
    )
{
    HRESULT hr = E_FAIL;

    if (NULL != m_pSession)
    {
        ULONG cbEID = 0;
        LPENTRYID lpEID = NULL;
        //
        // Get the "identity" of the session.
        // In general, this is the user's entry ID.
        //
        hr = m_pSession->QueryIdentity(&cbEID, &lpEID);
        if (SUCCEEDED(hr))
        {
            LPADRBOOK pAddrBook = NULL;
            hr = GetAddressBook(&pAddrBook);
            if (SUCCEEDED(hr))
            {
                ULONG ulObjType = 0;
                IMAPIProp *pMailUser = NULL;

                //
                // Open the user's entry in the address book.
                //
                hr = pAddrBook->OpenEntry(cbEID,
                                          lpEID,
                                          NULL,
                                          0,
                                          &ulObjType,
                                          (LPUNKNOWN *)&pMailUser);
                if (SUCCEEDED(hr))
                {
                    ULONG ulFlags = 0;
                    ULONG cProps  = 0;
#ifdef UNICODE
                    //
                    // For unicode builds, we want UNICODE property strings.
                    //
                    ulFlags |= MAPI_UNICODE;
#endif

                    SizedSPropTagArray(5, tags) = { 5, PR_ADDRTYPE,
                                                       PR_DISPLAY_NAME,
                                                       PR_EMAIL_ADDRESS,
                                                       PR_ENTRYID,
                                                       PR_SEARCH_KEY };

                    //
                    // Retrieve the user properties and return them to
                    // the caller.
                    //
                    hr = pMailUser->GetProps((LPSPropTagArray)&tags, // Prop tags
                                              ulFlags,                     
                                              pcbProps,              // Prop cnt (out)
                                              ppProps);              // Prop ptr (out)
                    ReportErrorsReturned(hr);
                    pMailUser->Release();
                }
                pAddrBook->Release();
            }
            MAPI32.FreeBuffer(lpEID);
        }
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiSession::OpenOutBoxFolder

    Description: Opens the outbox folder in the default message store.
        Stores the resulting IMAPIFolder ptr in m_pOutBoxFolder.

    Arguments: None.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiSession::OpenOutBoxFolder(
    VOID
    )
{
    HRESULT hr = E_FAIL;

    if (NULL != m_pSession && NULL != m_pDefMsgStore)
    {
        LPSPropValue pProps = NULL;
        ULONG ulObjType;
        ULONG cProps;
        ULONG ulFlags = 0;
#ifdef UNICODE
        //
        // For unicode builds, we want UNICODE property strings.
        //
        ulFlags |= MAPI_UNICODE;
#endif
        
        SizedSPropTagArray(1, sptFolders) = { 1, PR_IPM_OUTBOX_ENTRYID };

        //
        // Retrieve the entry ID for the outbox in the default msg store.
        //
        hr = m_pDefMsgStore->GetProps((LPSPropTagArray)&sptFolders, // Prop tags
                                      ulFlags,                     
                                      &cProps,                      // Prop cnt (out)
                                      &pProps);                     // Prop ptr (out)

        ReportErrorsReturned(hr);
        if (SUCCEEDED(hr))
        {
            if (0 != cProps && NULL != pProps)
            {
                if (pProps[0].ulPropTag == sptFolders.aulPropTag[0])
                {
                    //
                    // Get the MAPI folder interface ptr for the outbox folder.
                    //
                    if (NULL != m_pOutBoxFolder)
                    {
                        //
                        // Release any previously-held outbox interface ptr.
                        //
                        m_pOutBoxFolder->Release();
                        m_pOutBoxFolder = NULL;
                    }
                    hr = m_pSession->OpenEntry(pProps[0].Value.bin.cb,
                                               (LPENTRYID)pProps[0].Value.bin.lpb,
                                               NULL,
                                               MAPI_MODIFY,
                                               &ulObjType,
                                               (LPUNKNOWN *)&m_pOutBoxFolder);

                    ReportErrorsReturned(hr);
                }
                else
                {
                    hr = MAPI_E_NOT_FOUND;
                }
                MAPI32.FreeBuffer(pProps);
            }
        }
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiSession::Send

    Description: Sends a message to a list of recipients.  There are 3
        versions that allow simple and complex formatting of the
        message body:

        1. First version lets you pass in simple text strings for the
           subject and body text.  This is the simplest way to send a 
           short, simple message.

        2. Second version lets you pass in the subject as a simple text
           string but then lets you create a complex message body as
           a CMapiMessageBody object.  This probably won't get used much.  

        3. The third version lets you create a CMapiMessage object
           containing subject line and body text. This is the method
           to use if you want to create a message with complex formatting.

        Sending a message with CMapiSession::Send ensures that the
        recipient list contains fully resolved names.  If you send
        a message with CMapiMessage::Send, you must resolve the names
        before the Send call is made.

    Arguments:
        pAdrList - Address of MAPI ADRLIST structure containing the
            list of recipient addresses.  It is assumed that
            the list contains unresolved recipients but this is not a 
            requirement.

        pszSubject - Address of subject line text.

        pszBody - Address of msg body text.

        body - Reference to CMapiMessageBody object containing the message text.

        msg - Reference to CMapiMessage object containing the subject line 
            and message body text.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
//
// Simplest form.  Just give an address list, subject line and body 
// text string.
//
HRESULT
CMapiSession::Send(
    LPADRLIST pAdrList,
    LPCTSTR pszSubject,
    LPCTSTR pszBody
    )
{
    HRESULT hr;
    //
    // Create a local CMapiMessage object with the given subject
    // line and body text.  Then just send it.
    //
    CMapiMessage msg(m_pOutBoxFolder);
    hr = msg.SetSubject(pszSubject);
    if (SUCCEEDED(hr))
    {
        hr = msg.Append(pszBody);
        if (SUCCEEDED(hr))
        {
            hr = Send(pAdrList, msg);
        }
    }
    return hr;
}

//
// If you already have a CMapiMessageBody object created with text, this is the
// version you want to use.
//
HRESULT
CMapiSession::Send(
    LPADRLIST pAdrList,
    LPCTSTR pszSubject,
    CMapiMessageBody& body
    )
{
    //
    // Create a local CMapiMessage object with the given subject
    // line and body text.  Then just send it.
    //
    CMapiMessage msg(m_pOutBoxFolder, body, pszSubject);

    return Send(pAdrList, msg);
}


//
// If you already have a CMapiMessage object create with subject line
// and body text, use this.  The other versions of Send() eventually
// call this one.
//
HRESULT
CMapiSession::Send(
    LPADRLIST pAdrList,
    CMapiMessage& msg
    )
{
    HRESULT hr = E_FAIL;

    hr = ResolveAddresses(pAdrList);
    if (SUCCEEDED(hr))
    {
        hr = msg.Send(pAdrList);
        DebugMsg(DM_ERROR, TEXT("CMapiSession::Send, Result = 0x%08X"), hr);
    }
    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiSession::GetAddressBook

    Description: Returns the session's address book pointer.

    Arguments:
        ppAdrBook - Address of pointer variable to receive pointer value.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiSession::GetAddressBook(
    LPADRBOOK *ppAdrBook
    )
{
    HRESULT hr = E_POINTER;
    if (NULL != m_pSession && NULL != ppAdrBook)
    {  
        hr = m_pSession->OpenAddressBook(0,             // Hwnd for UI
                                         NULL,          // Use std interface
                                         AB_NO_DIALOG,  // No UI.
                                         ppAdrBook);    // Book ptr (out)

        ReportErrorsReturned(hr);
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiSession::ReportErrorsReturned [static]

    Description: When a MAPI function returns MAPI_W_ERRORS_RETURNED,
        error information can be obtained by calling IMapiSession::GetLastError.
        This function encapsulates the necessary behavior to get this error
        information and dump it to the debugger.  An option to write a 
        warning/error to the system event log is planned.

    Arguments:
        hr - HRESULT containing the error code.

        bLogEvent [optional] - If True, write an event to the system event
            log.  This option is not in place yet.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/16/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
CMapiSession::ReportErrorsReturned(
    HRESULT hr, 
    BOOL bLogEvent // Unused at this time.
    )
{
    if (MAPI_W_ERRORS_RETURNED == hr)
    {
        LPMAPIERROR pMapiErrors = NULL;
        DWORD dwFlags           = 0;
#ifdef UNICODE
        dwFlags |= MAPI_UNICODE;
#endif
        hr = m_pSession->GetLastError(hr, dwFlags, &pMapiErrors);
        if (S_OK == hr)
        {
            if (NULL != pMapiErrors)
            {
                DebugMsg(DM_ERROR, TEXT("MAPI returned errors.\n"));
                DebugMsg(DM_ERROR, TEXT("\tVersion.......: %d"), pMapiErrors->ulVersion);
                DebugMsg(DM_ERROR, TEXT("\tComponent.....: %s"), pMapiErrors->lpszComponent ?
                                                                 pMapiErrors->lpszComponent :
                                                                 TEXT("Not Provided"));
                DebugMsg(DM_ERROR, TEXT("\tError.........: %s"), pMapiErrors->lpszError ?
                                                                 pMapiErrors->lpszError :
                                                                 TEXT("Not Provided"));
                DebugMsg(DM_ERROR, TEXT("\tContext.......: %d"), pMapiErrors->ulContext);
                DebugMsg(DM_ERROR, TEXT("\tLowLevel Error: %d\n"), pMapiErrors->ulLowLevelError);
                if (bLogEvent)
                {
                    //
                    // Open event log and write error?
                    //
                }
                MAPI32.FreeBuffer(pMapiErrors);
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiSession::GetOutBoxFolder

    Description: Returns the session's outbox folder pointer.

    Arguments:
        ppOutBoxFolder - Address of pointer variable to receive pointer value.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CMapiSession::GetOutBoxFolder(
    LPMAPIFOLDER *ppOutBoxFolder
    )
{
    HRESULT hr = E_POINTER;

    if (NULL != m_pOutBoxFolder && NULL != ppOutBoxFolder)
    {
        *ppOutBoxFolder = m_pOutBoxFolder;
        (*ppOutBoxFolder)->AddRef();
        hr = NO_ERROR;
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiSession::ResolveAddresses

    Description: Resolves names in an address list.

    Arguments:
        pAdrList - Pointer to the address list to be resolved.

    Returns: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CMapiSession::ResolveAddresses(
    LPADRLIST pAdrList
    )
{
    HRESULT hr;
    LPADRBOOK pAdrBook = NULL;

    hr = GetAddressBook(&pAdrBook);
    if (SUCCEEDED(hr))
    {
        hr = pAdrBook->ResolveName(0,          // Hwnd for UI
                                   0,          // Flags (no UI).
                                   NULL,       // Dlg title (none).
                                   pAdrList);  // ADRLIST ptr

        ReportErrorsReturned(hr);
        pAdrBook->Release();
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiMessageBody::CMapiMessageBody

    Description: Constructors.

    Arguments:
        rhs - Reference to source CMapiMessageBody object in copy ctor.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
CMapiMessageBody::CMapiMessageBody(
    VOID
    ) : m_pStg(NULL),
        m_pStm(NULL)
{
    CommonConstruct();
}

CMapiMessageBody::CMapiMessageBody(
    const CMapiMessageBody& rhs
    ) : m_pStg(NULL),
        m_pStm(NULL)
{
    CommonConstruct();
    *this = rhs;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiMessageBody::~CMapiMessageBody

    Description: Destructor.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
CMapiMessageBody::~CMapiMessageBody(
    VOID
    )
{
    if (NULL != m_pStm)
    {
        m_pStm->Release();
    }
    if (NULL != m_pStg)
    {
        m_pStg->Release();
    }
}
    
    
///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiMessageBody::operator = 

    Description: Assignment operator.

    Arguments:
        rhs - Reference to source CMapiMessageBody object.

    Returns: Reference to *this.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
CMapiMessageBody& 
CMapiMessageBody::operator = (
    const CMapiMessageBody& rhs
    )
{
    if (this != &rhs)
    {
        if (NULL != m_pStm && NULL != rhs.m_pStm)
        {
            HRESULT hr;
            LPSTREAM pStmClone;

            //
            // Create a clone of the source stream so that we don't alter
            // the source's seek ptr.
            //
            hr = rhs.m_pStm->Clone(&pStmClone);
            if (SUCCEEDED(hr))
            {
                ULARGE_INTEGER ulSize = {0, 0};
                LARGE_INTEGER lSeek   = {0, 0};
                //
                // Reset the source stream seek ptr to the beginnging.
                //
                pStmClone->Seek(lSeek, STREAM_SEEK_SET, NULL);
                //
                // Truncate the destination stream to clear it.
                //
                m_pStm->SetSize(ulSize);
                //
                // Copy all of the source stream to the dest stream.
                //
                ulSize.LowPart = 0xFFFFFFFF;
                hr = pStmClone->CopyTo(m_pStm,      // Destination stream.
                                       ulSize,      // Write all bytes.
                                       NULL,        // pcbRead
                                       NULL);       // pcbWritten
                pStmClone->Release();
            }
        }
    }
    return *this;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiMessageBody::CommonConstruct

    Description: Performs operations common to all forms of constructors.

        1. Creates Storage object.
        2. Creates Stream "MSGBODY" in storage.

    Arguments: None.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiMessageBody::CommonConstruct(
    VOID
    )
{
    HRESULT hr;
    DWORD grfMode = STGM_DIRECT | STGM_READWRITE |
                    STGM_CREATE | STGM_SHARE_EXCLUSIVE;
    //
    // Create the output doc file.
    //
    hr = StgCreateDocfile(NULL,     // Temp file with unique name.
                          grfMode,  // Access flags.
                          0,        // Reserved
                          &m_pStg); // Stg ptr (out)

    if (SUCCEEDED(hr))
    {
        //
        // Create the stream in the doc file.
        //
        hr = m_pStg->CreateStream(L"MSGBODY",  // Stream name.
                                  grfMode,     // Access flags.
                                  0,           // Reserved.
                                  0,           // Reserved.
                                  &m_pStm);    // Stream ptr (out)
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiMessageBody::Append

    Description: Appends text to the msg body stream.  Two versions are 
        provided.  One accepts a nul-terminated format string while the other
        accepts the resource ID for a string resource.  Both formats allow
        variable replacement arguments for replaceable arguments in the
        format strings (i.e. %1, %2 etc.)

    Arguments:
        hInst - Module instance handle for string/message resource.

        pszFmt - Address of format string.

        idFmt - ID of format resource string.  May be a string or message
            resource.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiMessageBody::Append(
    LPCTSTR pszFmt,
    ...
    )
{
    HRESULT hr;
    va_list args;
    va_start(args, pszFmt);

    hr = Append(pszFmt, &args);

    va_end(args);

    return hr;
}

HRESULT 
CMapiMessageBody::Append(
    LPCTSTR pszFmt,
    va_list *pargs
    )
{
    HRESULT hr = E_POINTER;

    if (NULL != m_pStm)
    {
        CString str;
        if (str.Format(pszFmt, pargs))
        {
            hr = m_pStm->Write((LPCTSTR)str, str.LengthBytes(), NULL);
        }
    }
    return hr;
}

HRESULT
CMapiMessageBody::Append(
    HINSTANCE hInst,
    UINT idFmt,
    ...
    )
{
    HRESULT hr;

    va_list(args);
    va_start(args, idFmt);

    hr = Append(hInst, idFmt, &args);

    va_end(args);
    return hr;
}


HRESULT
CMapiMessageBody::Append(
    HINSTANCE hInst,
    UINT idFmt,
    va_list *pargs
    )
{
    HRESULT hr = E_POINTER;

    if (NULL != m_pStm)
    {
        CString str;
        if (str.Format(hInst, idFmt, pargs))
        {
            hr = m_pStm->Write((LPCTSTR)str, str.LengthBytes(), NULL);
        }
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiMessage::CMapiMessage

    Description: Constructors.

    Arguments:
        pFolder - Address of LPMAPIFOLDER object in which the message is
            to be created.  

        body - Reference to a CMapiMessageBody object containing text for the
            body of the message.

        pszSubject [optional] - Address of message subject line string.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
CMapiMessage::CMapiMessage(
    LPMAPIFOLDER pFolder
    ) : m_pMsg(NULL)
{
    CommonConstruct(pFolder);
}

CMapiMessage::CMapiMessage(
    LPMAPIFOLDER pFolder,
    CMapiMessageBody& body, 
    LPCTSTR pszSubject /* optional */
    ) : m_pMsg(NULL),
        m_body(body)
{
    CommonConstruct(pFolder);
    if (NULL != pszSubject)
    {
        SetSubject(pszSubject);
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiMessage::~CMapiMessage

    Description: Destructor.

    Arguments: None.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
CMapiMessage::~CMapiMessage(
    VOID
    )
{
    if (NULL != m_pMsg)
    {
        m_pMsg->Release();
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiMessage::CommonConstruct

    Description: Performs operations common to all forms of constructors.

        1. Creates the MAPI message object.
        2. Sets the DELETE_AFTER_SUBMIT property (common to all messages).

    Arguments:
        pFolder - Address of LPMAPIFOLDER object in which the message is
            to be created.  

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CMapiMessage::CommonConstruct(
    LPMAPIFOLDER pFolder
    )
{
    HRESULT hr = E_POINTER;

    if (NULL != pFolder)
    {
        hr = pFolder->CreateMessage(NULL, 0, &m_pMsg);
        if (SUCCEEDED(hr))
        {
            //
            // Don't want sent message hanging around in users's outbox.
            //
            SPropValue rgProp[1];
            rgProp[0].ulPropTag  = PR_DELETE_AFTER_SUBMIT;
            rgProp[0].Value.b    = TRUE;

            hr = m_pMsg->SetProps(ARRAYSIZE(rgProp), rgProp, NULL);
        }
    }
    return hr;
}


HRESULT 
CMapiMessage::SetProps(
    ULONG cValues, 
    LPSPropValue lpPropArray, 
    LPSPropProblemArray *lppProblems
    )
{
    HRESULT hr = E_UNEXPECTED;
    if (NULL != m_pMsg)
    {
        hr = m_pMsg->SetProps(cValues, lpPropArray, lppProblems);
    }
    return hr;

}


HRESULT
CMapiMessage::SaveChanges(
    ULONG ulFlags
    )
{
    HRESULT hr = E_UNEXPECTED;
    if (NULL != m_pMsg)
    {
        hr = m_pMsg->SaveChanges(ulFlags);
    }
    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiMessage::SetSubject

    Description: Sets the PR_SUBJECT property of the message.

    Arguments:
        pszSubject - Address of new subject string.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CMapiMessage::SetSubject(
    LPCTSTR pszSubject
    )
{
    HRESULT hr = E_POINTER;
    if (NULL != m_pMsg)
    {
        //
        // Set the msg subject text property.
        //
        SPropValue spvProp;
        spvProp.ulPropTag  = PR_SUBJECT;
        spvProp.Value.LPSZ = (LPTSTR)pszSubject;

        hr = m_pMsg->SetProps(1, &spvProp, NULL);
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiMessage::SetRecipients

    Description: Sets the recipients for the message.  It is assumed that
        the recipients in pAdrList have been resolved.

    Arguments:
        pAdrList - Address of MAPI address list containing resolved addresses.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiMessage::SetRecipients(
    LPADRLIST pAdrList
    )
{
    HRESULT hr = E_POINTER;
    if (NULL != m_pMsg)
    {
        hr = m_pMsg->ModifyRecipients(0, pAdrList);
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiMessage::Send

    Description: Sends the message to a list of recipients.

    Arguments:
        pAdrList [optional] - Address of MAPI address list containing 
            resolved addresses.  If this argument is NULL, the caller must
            call SetRecipients before calling Send.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiMessage::Send(
    LPADRLIST pAdrList /* optional */
    )
{
    HRESULT hr = E_POINTER;
    if (NULL != m_pMsg)
    {
        hr = NO_ERROR;
        if (NULL != pAdrList)
        {
            //
            // If there's an address list, set the recipients.
            // If not, the caller must call SetRecipients before calling Send().
            // Otherwise, there will be no recipients to send it to.
            //
            hr = SetRecipients(pAdrList);
        }
        if (SUCCEEDED(hr))
        {
            LPSTREAM pPropStream;
            hr = m_pMsg->OpenProperty(PR_BODY,
                                      &IID_IStream,
                                      STGM_READWRITE | STGM_DIRECT,
                                      MAPI_CREATE | MAPI_MODIFY,
                                      (LPUNKNOWN *)&pPropStream);
            if (S_OK == hr)
            {
                LPSTREAM pMsgBodyStm = (LPSTREAM)m_body;
                LPSTREAM pMsgBodyStmClone;

                //
                // Clone the body stream so that we don't alter it's seek ptr.
                //
                hr = pMsgBodyStm->Clone(&pMsgBodyStmClone);
                if (SUCCEEDED(hr))
                {
                    ULARGE_INTEGER ulSize = {0xFFFFFFFF, 0};
                    LARGE_INTEGER lSeek   = {0, 0};

                    //
                    // Copy the msg body stream to the PR_BODY property.
                    //
                    pMsgBodyStmClone->Seek(lSeek, STREAM_SEEK_SET, NULL);
                    pMsgBodyStmClone->CopyTo(pPropStream, ulSize, NULL, NULL);
                    pPropStream->Commit(STGC_DEFAULT);
                    //
                    // Release temp streams.
                    //
                    pPropStream->Release();
                    pMsgBodyStmClone->Release();
                    //
                    // Send it!
                    // Note:  Calling SaveChanges() is not required if the message
                    //        is being sent immediately.
                    //
                    hr = m_pMsg->SubmitMessage(FORCE_SUBMIT);
                }
            }
        }
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CMapiMessage::Append

    Description: Appends text to the msg body stream.  Two versions are 
        provided.  One accepts a nul-terminated format string while the other
        accepts the resource ID for a string resource.  Both formats allow
        variable replacement arguments for replaceable arguments in the
        format strings (i.e. %1, %2 etc.)

    Arguments:
        hInst - Module instance handle for string/message resource.

        pszFmt - Address of format string.

        idFmt - ID of format resource string.  May be a string or message
            resource.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CMapiMessage::Append(
    LPCTSTR pszFmt,
    ...
    )
{
    HRESULT hr;

    va_list args;
    va_start(args, pszFmt);
    hr = m_body.Append(pszFmt, &args);
    va_end(args);

    return hr;
}


HRESULT
CMapiMessage::Append(
    HINSTANCE hInst,
    UINT idFmt,
    ...
    )
{
    HRESULT hr;

    va_list(args);
    va_start(args, idFmt);
    hr = m_body.Append(hInst, idFmt, &args);
    va_end(args);

    return hr;
}



//
// Static members of class MAPI.
//
LONG                 MAPI::m_cLoadCount;
HINSTANCE            MAPI::m_hmodMAPI;
LPMAPIINITIALIZE     MAPI::m_pfnInitialize;
LPMAPILOGONEX        MAPI::m_pfnLogonEx;
LPMAPIUNINITIALIZE   MAPI::m_pfnUninitialize;
LPMAPIALLOCATEBUFFER MAPI::m_pfnAllocateBuffer;
LPMAPIALLOCATEMORE   MAPI::m_pfnAllocateMore;
LPMAPIFREEBUFFER     MAPI::m_pfnFreeBuffer;
LPMAPIHRQUERYALLROWS MAPI::m_pfnHrQueryAllRows;
LPMAPIFREEPADRLIST   MAPI::m_pfnFreePadrlist;             
LPMAPIFREEPROWS      MAPI::m_pfnFreeProws;


///////////////////////////////////////////////////////////////////////////////
/*  Function: MAPI::MAPI

    Description: Constructor.  

    Arguments: None.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
MAPI::MAPI(
    VOID
    )
{

}


///////////////////////////////////////////////////////////////////////////////
/*  Function: MAPI::~MAPI

    Description: Destructor.  Ensures MAPI32.DLL is unloaded.

    Arguments: None.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
MAPI::~MAPI(
    VOID
    )
{
    //
    // m_cLoadCount should be 0 at this point if calls to Load() and
    // Unload() are balanced.  
    //
    ASSERT(0 == m_cLoadCount);
    if (0 < m_cLoadCount)
    {
        //
        // Calls to Load() and Unload() are not balanced due to programmer 
        // error or maybe an exception preventing a call to Unload().
        // This will force Unload to call FreeLibrary().
        //
        m_cLoadCount = 1;
    }
    Unload();
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: MAPI::Load

    Description: Load MAPI32.DLL and call GetProcAddress for all of the
        MAPI32 functions we're interested in using.  Maintains a reference
        count so redundant calls to LoadLibrary are avoided.

    Arguments: None.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
MAPI::Load(
    VOID
    )
{
    Assert(0 <= m_cLoadCount);
    if (0 == m_cLoadCount++)
    {
        m_hmodMAPI = ::LoadLibrary(TEXT("MAPI32.DLL"));
        if (NULL != m_hmodMAPI)
        {
            m_pfnInitialize     = (LPMAPIINITIALIZE)    ::GetProcAddress(m_hmodMAPI, "MAPIInitialize");
            m_pfnLogonEx        = (LPMAPILOGONEX)       ::GetProcAddress(m_hmodMAPI, "MAPILogonEx");
            m_pfnUninitialize   = (LPMAPIUNINITIALIZE)  ::GetProcAddress(m_hmodMAPI, "MAPIUninitialize");
            m_pfnAllocateBuffer = (LPMAPIALLOCATEBUFFER)::GetProcAddress(m_hmodMAPI, "MAPIAllocateBuffer");
            m_pfnAllocateMore   = (LPMAPIALLOCATEMORE)  ::GetProcAddress(m_hmodMAPI, "MAPIAllocateMore");
            m_pfnFreeBuffer     = (LPMAPIFREEBUFFER)    ::GetProcAddress(m_hmodMAPI, "MAPIFreeBuffer");
            m_pfnHrQueryAllRows = (LPMAPIHRQUERYALLROWS)::GetProcAddress(m_hmodMAPI, "HrQueryAllRows@24");
            m_pfnFreePadrlist   = (LPMAPIFREEPADRLIST)  ::GetProcAddress(m_hmodMAPI, "FreePadrlist@4");
            m_pfnFreeProws      = (LPMAPIFREEPROWS)     ::GetProcAddress(m_hmodMAPI, "FreeProws@4");
        }
    }
    return (NULL != m_hmodMAPI) ? NO_ERROR : E_FAIL;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: MAPI::Unload

    Description: Unloads MAPI32.DLL if the reference count drops to 0. If
        the library is unloaded, all of the function pointers are
        set to NULL.

    Arguments: None.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
MAPI::Unload(
    VOID
    )
{
    ASSERT(0 < m_cLoadCount);
    if (0 == --m_cLoadCount)
    {
        if (NULL != m_hmodMAPI)
        {
            ::FreeLibrary(m_hmodMAPI);
            m_hmodMAPI = NULL;
        }
        m_pfnInitialize     = NULL;
        m_pfnLogonEx        = NULL;
        m_pfnUninitialize   = NULL;
        m_pfnAllocateBuffer = NULL;
        m_pfnAllocateMore   = NULL;
        m_pfnFreeBuffer     = NULL;
        m_pfnHrQueryAllRows = NULL;
        m_pfnFreePadrlist   = NULL;
        m_pfnFreeProws      = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
// The remaining MAPI::XXXX functions are merely simple wrappers around the
// corresponding functions in MAPI32.DLL.
// See the MAPI SDK for information concerning their use.
///////////////////////////////////////////////////////////////////////////////

HRESULT 
MAPI::LogonEx(
    ULONG ulUIParam,   
    LPTSTR lpszProfileName,   
    LPTSTR lpszPassword,   
    FLAGS flFlags,   
    LPMAPISESSION FAR * lppSession
    )
{
    ASSERT(NULL != m_pfnLogonEx);
    if (NULL != m_pfnLogonEx)
        return (*m_pfnLogonEx)(ulUIParam, lpszProfileName, lpszPassword, flFlags, lppSession);
    else
        return E_POINTER;
}

HRESULT 
MAPI::Initialize(
    LPVOID lpMapiInit
    )
{
    ASSERT(NULL != m_pfnInitialize);
    if (NULL != m_pfnInitialize)
        return (*m_pfnInitialize)(lpMapiInit);
    else
        return E_POINTER;
}


VOID
MAPI::Uninitialize(
    VOID
    )
{
    ASSERT(NULL != m_pfnUninitialize);
    if (NULL != m_pfnUninitialize)
        (*m_pfnUninitialize)();
}    



SCODE 
MAPI::AllocateBuffer(
    ULONG cbSize,   
    LPVOID FAR * lppBuffer
    )
{
    ASSERT(NULL != m_pfnAllocateBuffer);
    if (NULL != m_pfnAllocateBuffer)
        return (*m_pfnAllocateBuffer)(cbSize, lppBuffer);
    else
        return E_POINTER;
}


SCODE 
MAPI::AllocateMore(
    ULONG cbSize,   
    LPVOID lpObject,   
    LPVOID FAR * lppBuffer
    )
{
    ASSERT(NULL != m_pfnAllocateMore);
    if (NULL != m_pfnAllocateMore)
        return (*m_pfnAllocateMore)(cbSize, lpObject, lppBuffer);
    else
        return E_POINTER;
}


ULONG 
MAPI::FreeBuffer(
    LPVOID lpBuffer
    )
{
    ASSERT(NULL != m_pfnFreeBuffer);
    if (NULL != m_pfnFreeBuffer)
        return (*m_pfnFreeBuffer)(lpBuffer);
    else
        return (ULONG)E_POINTER;
}


HRESULT 
MAPI::HrQueryAllRows(
    LPMAPITABLE lpTable,
    LPSPropTagArray lpPropTags,
    LPSRestriction lpRestriction,
    LPSSortOrderSet lpSortOrderSet,
    LONG crowsMax,
    LPSRowSet FAR *lppRows
    )
{
    ASSERT(NULL != m_pfnHrQueryAllRows);
    if (NULL != m_pfnHrQueryAllRows)
        return (*m_pfnHrQueryAllRows)(lpTable, 
                                      lpPropTags, 
                                      lpRestriction, 
                                      lpSortOrderSet, 
                                      crowsMax, 
                                      lppRows);
    else
        return E_POINTER;
}


VOID 
MAPI::FreePadrlist(
    LPADRLIST lpAdrList
    )
{
    ASSERT(NULL != m_pfnFreePadrlist);
    if (NULL != m_pfnFreePadrlist)
        (*m_pfnFreePadrlist)(lpAdrList);
}


VOID 
MAPI::FreeProws(
    LPSRowSet lpRows
    )
{
    ASSERT(NULL != m_pfnFreeProws);
    if (NULL != m_pfnFreeProws)
        (*m_pfnFreeProws)(lpRows);
}

