/**************************** Module Header ********************************\
* Module Name: exports.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Routines exported from winsrv.dll
*
* History:
* 03-04-95 JimA                Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* _UserSoundSentry
*
* Private API for BASE to use for SoundSentry support.
*
* History:
* 08-02-93 GregoryW         Created.
\***************************************************************************/
BOOL
_UserSoundSentry(
    UINT uVideoMode)
{
    UNREFERENCED_PARAMETER(uVideoMode);

    return NT_SUCCESS(NtUserSoundSentry());
}

/***************************************************************************\
* _UserTestTokenForInteractive
*
* Returns TRUE if the token passed represents an interactive user logged
* on by winlogon, otherwise FALSE
*
* The token handle passed must have TOKEN_QUERY access.
*
* History:
* 05-06-92 Davidc       Created
\***************************************************************************/

NTSTATUS
_UserTestTokenForInteractive(
    HANDLE Token,
    PLUID pluidCaller
    )
{
    PTOKEN_STATISTICS pStats;
    ULONG BytesRequired;
    NTSTATUS Status;

    /*
     * Get the session id of the caller.
     */
    Status = NtQueryInformationToken(
                 Token,                     // Handle
                 TokenStatistics,           // TokenInformationClass
                 NULL,                      // TokenInformation
                 0,                         // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    if (Status != STATUS_BUFFER_TOO_SMALL) {
        return Status;
        }

    //
    // Allocate space for the user info
    //

    pStats = (PTOKEN_STATISTICS)LocalAlloc(LPTR, BytesRequired);
    if (pStats == NULL) {
        return Status;
        }

    //
    // Read in the user info
    //

    Status = NtQueryInformationToken(
                 Token,             // Handle
                 TokenStatistics,       // TokenInformationClass
                 pStats,                // TokenInformation
                 BytesRequired,         // TokenInformationLength
                 &BytesRequired         // ReturnLength
                 );

    if (NT_SUCCESS(Status)) {
        if (pluidCaller != NULL)
             *pluidCaller = pStats->AuthenticationId;

        /*
         * A valid session id has been returned.  Compare it
         * with the id of the logged on user.
         */
        Status = NtUserTestForInteractiveUser(&pStats->AuthenticationId);
#ifdef LATER
        if (pStats->AuthenticationId.QuadPart == pwinsta->luidUser.QuadPart)
            Status = STATUS_SUCCESS;
        else
            Status = STATUS_ACCESS_DENIED;
#endif
    }

    LocalFree(pStats);

    return Status;
}

