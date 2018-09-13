/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    autodial.c

Abstract:

    This module contains Autodial support for Winsock.

Author:

    Anthony Discolo (adiscolo)    15-May-1996

Revision History:

--*/

#include "precomp.h"

#ifdef RASAUTODIAL

//
// Registry value under the Winsock
// registry root that contains the
// path of the Autodial DLL.
//
#define REGVAL_AUTODIAL_DLL    "AutodialDLL"

//
// The default Autodial DLL if one
// isn't defined in the registry.
//
#define AUTODIAL_DLL            "rasadhlp.dll"

//
// The well-known entry points in the
// Autodial DLL that we call to
// invoke an Autodial attempt.
//
#define WSATTEMPTAUTODIALADDR       "WSAttemptAutodialAddr"
#define WSATTEMPTAUTODIALNAME       "WSAttemptAutodialName"
#define WSNOTESUCCESSFULHOSTENTLOOKUP  "WSNoteSuccessfulHostentLookup"

//
// Definition of the Autodial APIs.
//
typedef int (*WSAttemptAutodialAddrProc)(
    IN const struct sockaddr FAR *name,
    IN int namelen
    );

typedef int (*WSAttemptAutodialNameProc)(
    IN const LPWSAQUERYSETW lpqsRestrictions
    );

typedef void (*WSNoteSuccessfulHostentLookupProc)(
    IN const char FAR *name,
    IN const ULONG ipaddr
    );

CRITICAL_SECTION AutodialHelperLockG;
BOOLEAN fAutodialHelperInitG;
HINSTANCE hAutodialHelperDllG;
WSAttemptAutodialAddrProc lpfnWSAttemptAutodialAddrG;
WSAttemptAutodialNameProc lpfnWSAttemptAutodialNameG;
WSNoteSuccessfulHostentLookupProc lpfnWSNoteSuccessfulHostentLookupG;



VOID
InitializeAutodial(VOID)

/*++

Routine Description:
    Initialize the resources necessary for loading
    the Autodial helper DLL.

Arguments:
    None.

Return Value:
    None.

--*/

{
    InitializeCriticalSection(&AutodialHelperLockG);
} // InitializeAutodial



VOID
UninitializeAutodial(VOID)

/*++

Routine Description:
    Free the Autodial helper DLL if it has been loaded.

Arguments:
    None.

Return Value:
    None.

--*/

{
    EnterCriticalSection(&AutodialHelperLockG);
    if (hAutodialHelperDllG != NULL) {
        FreeLibrary(hAutodialHelperDllG);
        hAutodialHelperDllG = NULL;
    }
    LeaveCriticalSection(&AutodialHelperLockG);
    DeleteCriticalSection (&AutodialHelperLockG);
} // UninitializeAutodial



BOOL
LoadAutodialHelperDll(void)

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    HKEY hKey;
    BOOL bSuccess;

    EnterCriticalSection(&AutodialHelperLockG);
    if (!fAutodialHelperInitG) {
        PCHAR pszPath;
        //
        // Bail out if we were unable to allocate
        // the path string.
        //
        pszPath = new char[MAX_PATH];
        if (pszPath == NULL)
            return FALSE;
        //
        // Read the registry to determine the
        // location of the Autodial helper DLL.
        //
        hKey = OpenWinSockRegistryRoot();
        if (hKey != NULL) {
            bSuccess = ReadRegistryEntry(
                         hKey,
                         REGVAL_AUTODIAL_DLL,
                         (PVOID)pszPath,
                         MAX_PATH,
                         REG_SZ);
            CloseWinSockRegistryRoot(hKey);
            if (bSuccess)
                hAutodialHelperDllG = LoadLibrary(pszPath);
        }
        delete pszPath;

        //
        // If the registry key doesn't exist, then
        // try to load the default helper DLL.
        //
        if (hAutodialHelperDllG == NULL)
            hAutodialHelperDllG = LoadLibrary(AUTODIAL_DLL);
        if (hAutodialHelperDllG != NULL) {
            lpfnWSAttemptAutodialAddrG = (WSAttemptAutodialAddrProc)
              GetProcAddress(hAutodialHelperDllG, WSATTEMPTAUTODIALADDR);
            lpfnWSAttemptAutodialNameG = (WSAttemptAutodialNameProc)
              GetProcAddress(hAutodialHelperDllG, WSATTEMPTAUTODIALNAME);
            lpfnWSNoteSuccessfulHostentLookupG = (WSNoteSuccessfulHostentLookupProc)
              GetProcAddress(hAutodialHelperDllG, WSNOTESUCCESSFULHOSTENTLOOKUP);
        }
        fAutodialHelperInitG = TRUE;
    }
    LeaveCriticalSection(&AutodialHelperLockG);

    return (hAutodialHelperDllG != NULL);
} // LoadAutodialHelperDll



BOOL
WSAttemptAutodialAddr(
    IN const struct sockaddr FAR *name,
    IN int namelen
    )

/*++

Routine Description:
    Attempt an Autodial connection given the parameters
    to an unsuccessful call to connect().

Arguments:
    name: a pointer to the sockaddr structure used in
        the call to connect().

    namelen: the length of the name parameter.

Return Value:
    TRUE if the connection was made successfully,
    FALSE otherwise.

--*/

{
    //
    // Load helper DLL, if necessary.
    //
    if (!LoadAutodialHelperDll() || lpfnWSAttemptAutodialAddrG == NULL)
        return FALSE;
    //
    // Call the Autodial DLL.  It will return
    // TRUE if a new connection was made, and
    // FALSE otherwise.
    //
    return (*lpfnWSAttemptAutodialAddrG)(name, namelen);
} // WSAttemptAutoDialAddr



BOOL
WSAttemptAutodialName(
    IN const LPWSAQUERYSETW lpqsRestrictions
    )

/*++

Routine Description:
    Attempt an autodial connection given the parameters
    to an unsuccessful to WSALookupServiceNext().

Arguments:
    lpqsRestrictions: a pointer to the WSAQUERYSETW
        structure used in the call to
        WSALookupServiceBegin().

Return Value:
    TRUE if the connection was made successfully,
    FALSE otherwise.

--*/

{
    //
    // Load helper DLL, if necessary.
    //
    if (!LoadAutodialHelperDll() || lpfnWSAttemptAutodialNameG == NULL)
        return FALSE;
    //
    // Call the Autodial DLL.  It will return
    // TRUE if a new connection was made, and
    // FALSE otherwise.
    //
    return (*lpfnWSAttemptAutodialNameG)(lpqsRestrictions);
} // WSAttemptAutoDialName



VOID
WSNoteSuccessfulHostentLookup(
    IN const char FAR *name,
    IN const ULONG ipaddr
    )

/*++

Routine Description:
    Give Autodial information about successful name
    resolutions via gethostbyname().  This is
    useful for resolvers like DNS where it is impossible
    to get complete alias information about aliases via a
    low-level address.

    Ultimately, this should be called from WSLookupServiceNext(),
    but it's impossible at that level to determine what format
    the results are in.

Arguments:
    name: a pointer to the DNS name

    ipaddr: the IP address

Return Value:
    None.

--*/

{
    //
    // Load helper DLL, if necessary.
    //
    if (!LoadAutodialHelperDll() || lpfnWSNoteSuccessfulHostentLookupG == NULL)
        return;
    //
    // Call the Autodial DLL.
    //
    (*lpfnWSNoteSuccessfulHostentLookupG)(name, ipaddr);
} // WSNoteSuccessulNameLookup

#endif // RASAUTODIAL
