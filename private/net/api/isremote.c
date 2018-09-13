/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    isremote.c

Abstract:

    Contains the NetpIsRemote routine. This checks if a computer name
    designates the local machine

Author:

    Richard L Firth (rfirth) 24th April 1991

Revision History:

    01-Nov-1991 JohnRo
        Fixed RAID 3414: allow explicit local server name.  (NetpIsRemote was
        not canonizaling the computer name from NetWkstaGetInfo, so it was
        always saying that the local computer name was remote if it wasn't
        already canonicalized.)

    07-Jun-1991 rfirth
        * Changed name of routine to conform to Nt naming conventions

        * Added LocalOrRemote parameter - returning just ISLOCAL or ISREMOTE
          is not sufficient - we can return error codes too

        * Added CanonicalizedName parameter - now passes back canonicalized
          name if requested. This is because, usually, subsequent routines call
          to NetRemoteComputerSupports which performs minimal checking on the
          name. Ergo, if we hand it a canonicalized name (which we have to do
          with this routine anyway) it can't complain. Can it?

        * NetpIsRemote no longer a NET_API_FUNCTION

        * Made semantics of CanonicalizedName orhogonal - if NULL or NUL passed
          in as ComputerName, caller still gets back the local computer name
          IF CanonicalizedName NOT NULL AND IF THE REDIRECTOR HAS BEEN STARTED
--*/

#include "nticanon.h"


NET_API_STATUS
NetpIsRemote(
    IN  LPTSTR  ComputerName OPTIONAL,
    OUT LPDWORD LocalOrRemote,
    OUT LPTSTR  CanonicalizedName OPTIONAL,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Determines whether a computer name designates this machine or a renote
    one. Values of ComputerName which equate to the local machine are:

        NULL pointer to name
        pointer to NULL name
        pointer to non-NULL name which is lexically equivalent to this
        machine's name

    NB. This routine expects that the canonicalized version of ComputerName
        will fit into the buffer pointed at by CanonicalizedName. Since this
        is an INTERNAL function, this assumption is deemed valid

Arguments:

    IN  LPTSTR  ComputerName OPTIONAL
                    Pointer to computer name to check. May assume any of
                    the above forms
                    If a non-NULL string is passed in, then it may have
                    preceeding back-slashes. This is kind of expected since
                    this routine is called by remotable APIs and it is they
                    who specify that the form a computer name is \\<name>

    OUT LPDWORD LocalOrRemote
                    Points to the DWORD where the specifier for the
                    symbolic location will be returned. MUST NOT BE NULL. On
                    return will be one of:
                        ISLOCAL     The name defined by ComputerName specifies
                                    the local machine specifically or by default
                                    (ie. was NULL)
                        ISREMOTE    The name defined by ComputerName was non-NULL
                                    and was not the name of this machine

    OUT LPTSTR  CanonicalizedName OPTIONAL
                    Pointer to caller's buffer into which a copy of the
                    canonicalized name will be placed if requested. This
                    can then be used in subsequent calls, with the knowledge
                    that no further checking of the computer name is required.
                    Note that the format of this buffer will be \\<computername>
                    on return. The contents of this buffer will not be
                    modified unless this routine returns success

    IN  DWORD   Flags
                    A bitmap. Flags are:

                        NIRFLAG_MAPLOCAL    if set, will map (ie canonicalize)
                                            the NULL local name to this
                                            computer's name proper. Used in
                                            conjunction with CanonicalizedName
                                            This stops extraneous calls to
                                            NetpNameCanonicalize with the
                                            inherited CanonicalizedName
                                            parameter. See below for elucidation

Return Value:

    NET_API_STATUS
        Success = NERR_Success
        Failure = return code from:
                    NetpNameCanonicalize
                    NetWkstaGetInfo
                    NetpNameCompare
--*/

{
    LPBYTE  wksta_buffer_pointer;
    BOOL    map_local_name = FALSE;
    LONG    result;
    NET_API_STATUS  rc;

    //
    // BUGBUG
    // Computer names are defined as being 15 bytes which may mean 7 UNICODE
    // characters. Do we have to perform any UNICODE-ASCII mapping here?
    //

    TCHAR   name[MAX_PATH];     // canonicalized version of ComputerName
    LPTSTR  wksta_name_uncanon; // our computer name (from NetWkstaGetInfo)
    TCHAR   wksta_name_canon[MAX_PATH]; // our computer name (from canon)
    LPTSTR  canonicalized_name; // as returned to caller


    //
    // Assert that we have a valid pointer in LocalOrRemote
    //

    //
    // Once again, shouldn't have to do this, since this routine is internal
    // and there is no interpretation about inputs. However, lets catch any
    // possible problems...
    //

    NetpAssert(ARGUMENT_PRESENT(LocalOrRemote));

#ifdef CANONDBG
    DbgPrint("NetpIsRemote(%s, %x, %x, %x)\n",
        ComputerName,
        LocalOrRemote,
        CanonicalizedName,
        Flags
        );
#endif

    //
    // NB. It is important to check this case first, before we call any Netp
    // routines since these could call back to this routine and we may get
    // stuck in an infinite loop
    //

    if (!ARGUMENT_PRESENT(ComputerName) || (*ComputerName == TCHAR_EOS)) {

        //
        // in this case its probably an internal call from one of our routines
        // and we want to return as quickly as possible. This will be borne out
        // by the NIRFLAG_MAPLOCAL flag being reset in the Flags parameter
        //

        //
        // A note about NIRFLAG_MAPLOCAL
        // This routine makes local calls to NetpNameValidate and
        // NetpNameCompare. If the NIRFLAG_MAPLOCAL flag is not reset then
        // these routines in turn will cause the local name to be returned
        // (because they always pass in non-NULL CanonicalizedName parameter)
        // which in most cases is inefficient, since the name won't be used
        // so we always say (in the Netp routines) that we don't want local
        // name canonicalization
        // Therefore, if (local) name canonicalization is implied by non-NULL
        // CanonicalizedName, verify this by checking Flags.NIRFLAG_MAPLOCAL
        // If it, too, is set then local name canonicalization is performed
        //

        if (!ARGUMENT_PRESENT(CanonicalizedName) || !(Flags & NIRFLAG_MAPLOCAL)) {
            *LocalOrRemote = ISLOCAL;
#ifdef CANONDBG
            DbgPrint("NetpIsRemote(%s) - returning early\n", ComputerName);
#endif
            return NERR_Success;
        } else {

            //
            // signify that the input name was NULL or NUL string but that the
            // caller wants a canonicalized name returned (from NetWkstaGetInfo)
            //

            map_local_name = TRUE;
        }
    } else {

        //
        // if the computername starts with \\ or // or any combination thereof,
        // skip the path separators - the canonicalization routines expect
        // computer names NOT to have these (pretty stupid, eh?)
        //

        if (IS_PATH_SEPARATOR(ComputerName[0]) && IS_PATH_SEPARATOR(ComputerName[1])) {
            ComputerName += 2;
        }

        //
        // here's a use for canonicalization (!): ensure that we have been passed
        // a real and proper computer name and not some pale imitation
        //

        rc = NetpNameCanonicalize(
                NULL,                   // performed here, on our own premises
                ComputerName,           // this is input
                name,                   // this is output
                sizeof(name),           // how much buffer we have
                NAMETYPE_COMPUTER,      // what we think it is
                INNCA_FLAGS_FULL_BUFLEN // say that o/p buffer must be large
                                        // enough for maximum-sized computer
                                        // name. Why? you ask, well its a fair
                                        // cop - the reason is that we can't
                                        // get into trouble on the one time that
                                        // we exercise the maximum requirement
                );
        if (rc) {
            return rc;  // duff name (?)
        } else {
            canonicalized_name = name;
        }
    }

    //
    // get the name of this machine from the redirector. If we can't get the
    // name for whatever reason, return the error code.
    //

    if (rc = NetWkstaGetInfo(NULL, 102, &wksta_buffer_pointer)) {
#ifdef CANONDBG
        DbgPrint("error: NetWkstaGetInfo returns %lu\n", rc);
#endif
        return rc;  // didn't work
    }

    wksta_name_uncanon =
            ((LPWKSTA_INFO_102)wksta_buffer_pointer)->wki102_computername;

#ifdef CANONDBG
    DbgPrint("NetWkstaGetInfo returns level 102 computer name (uncanon)= %s\n",
            wksta_name_uncanon);
#endif
    rc = NetpNameCanonicalize(
            NULL,                       // performed here, on our own premises
            wksta_name_uncanon,         // this is input
            wksta_name_canon,           // this is output
            sizeof(wksta_name_canon),   // how much buffer we have
            NAMETYPE_COMPUTER,          // what we think it is
            INNCA_FLAGS_FULL_BUFLEN     // say that o/p buffer must be large
                                        // enough for maximum-sized computer
                                        // name. Why? you ask, well its a fair
                                        // cop - the reason is that we can't
                                        // get into trouble on the one time that
                                        // we exercise the maximum requirement
            );
    NetpAssert( rc == NERR_Success );

    //
    // compare our name and the name passed to us. NetpNameCompare returns
    // 0 if the names match else 1 or -1 (a la strcmp)
    //

    //
    // if the caller gave us a NULL computer name but wants a canonicalized
    // name output then get a pointer to the canonicalized name from
    // NetWkstaGetInfo
    //

    if (map_local_name) {
        canonicalized_name = wksta_name_canon;
    } else {

        //
        // otherwise, we have a non-NULL computername to compare with this
        // computer's name
        //

        result = NetpNameCompare(
                    NULL,   // performed here, on our own premises
                    name,   // canonicalized version of passed name
                    wksta_name_canon,  // name of our computer
                    NAMETYPE_COMPUTER,
                    INNC_FLAGS_NAMES_CANONICALIZED
                    );
    }

    //
    // if the specified name equates to our computer name then its still local
    //

    *LocalOrRemote = (DWORD)((result == 0) ? ISLOCAL : ISREMOTE);

    //
    // if the caller specified that the canonicalized name be returned, then
    // give it to 'em. Note that the returned name is prefixed with \\ - it
    // is assumed the name is then used in a call to eg NetRemoteComputerSupports
    //

    if (ARGUMENT_PRESENT(CanonicalizedName)) {
        STRCPY(CanonicalizedName, TEXT("\\\\"));
        STRCAT(CanonicalizedName, canonicalized_name);
    }

    //
    // free the buffer created by NetWkstaGetInfo
    //

    NetApiBufferFree(wksta_buffer_pointer);

    return NERR_Success;
}
