/*++

    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.

Module Name:

    nsprovid.cpp

Abstract:

    This module gives the class implementation for the NSPROVIDE object type.

Author:

    Dirk Brandewie (dirk@mink.intel.com) 04-December-1995

Notes:

    $Revision:   1.8  $

    $Modtime:   08 Mar 1996 16:04:36  $


Revision History:

    most-recent-revision-date email-name
        description

    04-Dec-1995 dirk@mink.intel.com
        Initial revision

--*/

#include "precomp.h"

NSPROVIDER::NSPROVIDER()
/*++

Routine Description:

    Coustructor for a NSPROVIDER object. Initializes object member variables to
    default values.

Arguments:

    None

Return Value:

    None
--*/
{
    // Init all member variable to known values
    m_reference_count = 1;
    memset(&m_proctable, 0, sizeof(m_proctable));
    m_proctable.cbSize = sizeof(m_proctable);
    m_library_handle = NULL;
#ifdef DEBUG_TRACING
    m_library_name = NULL;
#endif
}


inline INT WSAAPI
NSPROVIDER::NSPCleanup(
    VOID
    )
/*++
Routine Description:

    Terminate use of the WinSock name space service provider.

Arguments:

    None

Return Value:

    If no error occurs, NSPCleanup returns a value of NO_ERROR (zero).
    Otherwise, SOCKET_ERROR (-1) is returned and the provider must 
    set the appropriate error code using SetLastError

--*/
{
    INT ReturnValue = NO_ERROR;

    if (m_library_handle) {
        LPNSPCLEANUP    lpNSPCleanup;
        
        lpNSPCleanup = 
            (LPNSPCLEANUP)InterlockedExchangePointer (
                            (PVOID *)&m_proctable.NSPCleanup,
                            NULL
                            );
        if (lpNSPCleanup!=NULL) {

            DEBUGF( DBG_TRACE,
                    ("Calling NSPCleanup for provider %X\n", this));

            ReturnValue = lpNSPCleanup(&m_provider_id);

        }
    }
    return ReturnValue;
}

NSPROVIDER::~NSPROVIDER()
/*++

Routine Description:

    Destructor for NSPROVIDER object.  Frees resoures used by the object and
    set the member variables to the uninitialized state.

Arguments:

    None

Return Value:

    None
--*/
{
#ifdef DEBUG_TRACING
    if (m_library_name)
    {
        delete(m_library_name);
        m_library_name = NULL;
    } //if
#endif


    if (m_library_handle)
    {
        NSPCleanup();
        FreeLibrary(m_library_handle);
        m_library_handle = NULL;
    } //if
}


INT
NSPROVIDER::Initialize(
    IN LPWSTR lpszLibFile,
    IN LPGUID  lpProviderId
    )
/*++

Routine Description:

    This routine initializes an NSPROVIDER object.

Arguments:

    lpszLibFile - A string containing the path to the DLL for the name space
                  provider to be associated with this object.

    lpProviderId - A pointer to a GUID containing the provider ID for the
                   namespace provider.

Return Value:

    ERROR_SUCCESS if the provider was successfully initialized else an
    apropriate winsock error code.
--*/
{
    LPNSPSTARTUP        NSPStartupFunc;
    CHAR                AnsiPath[MAX_PATH];
    CHAR                ExpandedAnsiPath[MAX_PATH];
    INT                 ReturnCode;
    INT                 PathLength;
    DWORD               ExpandedPathLen;

    DEBUGF( DBG_TRACE,
            ("Initializing namespace provider %S\n", lpszLibFile));

    //
    // Map the UNICODE path to ANSI.
    //

    PathLength = WideCharToMultiByte(
        CP_ACP,                                       // CodePage
        0,                                            // dwFlags
        lpszLibFile,                                  // lpWideCharStr
        -1,                                           // cchWideChar
        AnsiPath,                                     // lpMultiByteStr
        sizeof(AnsiPath),                             // cchMultiByte
        NULL,
        NULL
        );

    if( PathLength == 0 ) {
        DEBUGF(
            DBG_ERR,
            ("Cannot map library path from UNICODE to ANSI\n"));
        return WSASYSCALLFAILURE;
    }

    //
    // Expand the library name to pickup environment/registry variables
    //

    ExpandedPathLen = ExpandEnvironmentStringsA(AnsiPath,
                                                ExpandedAnsiPath,
                                                MAX_PATH);

    if (ExpandedPathLen == 0) {
        DEBUGF(
            DBG_ERR,
            ("Expansion of environment variables failed\n"));
        return WSASYSCALLFAILURE;
    } //if

    TRY_START(guard_memalloc) {
#ifdef DEBUG_TRACING
        m_library_name = new CHAR[ExpandedPathLen];
        if (m_library_name == NULL) {
            DEBUGF(
                DBG_ERR,
                ("Allocation of m_lib_name failed\n"));
            ReturnCode = WSA_NOT_ENOUGH_MEMORY;
            TRY_THROW(guard_memalloc);
        }
        lstrcpy(m_library_name, ExpandedAnsiPath);

#endif
        //
        // Load the provider DLL, Call the provider startup routine and validate
        // that the provider filled out all the NSP_ROUTINE function pointers.
        //
        m_library_handle = LoadLibraryA(ExpandedAnsiPath);
        if (NULL == m_library_handle)
        {
            DEBUGF(
                DBG_ERR,
                ("Failed to load DLL %s\n",ExpandedAnsiPath));
            ReturnCode = WSAEPROVIDERFAILEDINIT;
            TRY_THROW(guard_memalloc);
        } //if

        //Get the procedure address of the NSPStartup routine
        NSPStartupFunc = (LPNSPSTARTUP)GetProcAddress(
            m_library_handle,
            "NSPStartup");
        if (NULL == NSPStartupFunc)
        {
            DEBUGF( DBG_ERR,("Could get startup entry point for NSP %S\n",
                             lpszLibFile));
            ReturnCode = WSAEPROVIDERFAILEDINIT;
            TRY_THROW(guard_memalloc);
        } //if

        // BUGBUG!!!
        //**** Hmmm.   There's  actually a small problem here.  The call to the NSP
        //**** startup  function  won't  go through the debug/trace stuff.  Look at
        //**** the file "dprovide.cpp" for a method of fixing this.  I have no time
        //**** to do it just now.
        //**** DTCODE_NSPSTARTUP is not defined in DT_DLL.H!!!!


        //
        // Set exception handler around this call since we
        // hold critical section (catalog lock).
        //
        __try {
            ReturnCode = (*NSPStartupFunc)(
                lpProviderId,
                &m_proctable);
        }
        __except (WS2_EXCEPTION_FILTER ()) {
            DEBUGF(DBG_ERR, ("Unhandled exception in NSPStartup for %s, code:%lx\n",
                    lpszLibFile, GetExceptionCode()));
            assert (FALSE);
        }

        if (ERROR_SUCCESS != ReturnCode)
        {
            DEBUGF(DBG_ERR, ("NSPStartup for %S Failed\n",lpszLibFile));
            ReturnCode = GetLastError();
            if(!ReturnCode)
            {
                ReturnCode = WSAEPROVIDERFAILEDINIT;
            }
            TRY_THROW(guard_memalloc);
        } //if

        // Check to see that the namespce provider filled in all the fields in the
        // NSP_ROUTINE struct like a good provider
        if (NULL == m_proctable.NSPCleanup             ||
            NULL == m_proctable.NSPLookupServiceBegin  ||
            NULL == m_proctable.NSPLookupServiceNext   ||
            NULL == m_proctable.NSPLookupServiceEnd    ||
            NULL == m_proctable.NSPSetService          ||
            NULL == m_proctable.NSPInstallServiceClass ||
            NULL == m_proctable.NSPRemoveServiceClass  ||
            NULL == m_proctable.NSPGetServiceClassInfo
            )
        {
            DEBUGF(DBG_ERR,
                   ("Service provider %S returned an invalid procedure table\n",
                    lpszLibFile));
            ReturnCode = WSAEINVALIDPROCTABLE;
            TRY_THROW(guard_memalloc);
        } //if
        m_provider_id = *lpProviderId;
        return(ERROR_SUCCESS);
    }
    TRY_CATCH(guard_memalloc) {
        // Cleanup
        if (m_library_handle!=NULL) {
            FreeLibrary (m_library_handle);
            m_library_handle = NULL;
        }
#ifdef DEBUG_TRACING
        if (m_library_name!=NULL) {
            delete m_library_name;
            m_library_name = NULL;
        }
#endif
        return ReturnCode;
    } TRY_END(guard_memalloc);
}



