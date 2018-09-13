/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    infsdisk.c

Abstract:

    Externally exposed INF routines for source disk descriptor manipulation.

Author:

    Ted Miller (tedm) 9-Feb-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Locations of various fields in the [SourceDisksNames] section
// of an inf
//
#define DISKNAMESECT_DESCRIPTION    1
#define DISKNAMESECT_TAGFILE        2       // cabinet name in win95
#define DISKNAMESECT_OEM            3       // unused, indicates oem disk in win95
#define DISKNAMESECT_PATH           4
#define DISKNAMESECT_FLAGS          5       // indicates if we're installing a service pack file


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupGetSourceInfoA(
    IN  HINF   InfHandle,
    IN  UINT   SourceId,
    IN  UINT   InfoDesired,
    OUT PSTR   ReturnBuffer,     OPTIONAL
    IN  DWORD  ReturnBufferSize,
    OUT PDWORD RequiredSize      OPTIONAL
    )
{
    DWORD rc;
    BOOL b;
    WCHAR buffer[MAX_INF_STRING_LENGTH];
    DWORD requiredsize;
    PCSTR ansi;

    b = pSetupGetSourceInfo(
            InfHandle,
            NULL,
            SourceId,
            InfoDesired,
            buffer,
            MAX_INF_STRING_LENGTH,
            &requiredsize
            );

    rc = GetLastError();

    if(b) {

        rc = NO_ERROR;

        if(ansi = UnicodeToAnsi(buffer)) {

            requiredsize = lstrlenA(ansi)+1;

            if(RequiredSize) {
                try {
                    *RequiredSize = requiredsize;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    rc = ERROR_INVALID_PARAMETER;
                    b = FALSE;
                }
            }

            if((rc == NO_ERROR) && ReturnBuffer) {

                if(!lstrcpynA(ReturnBuffer,ansi,ReturnBufferSize)) {
                    //
                    // ReturnBuffer invalid
                    //
                    rc = ERROR_INVALID_PARAMETER;
                    b = FALSE;
                }
            }

            MyFree(ansi);
        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            b = FALSE;
        }
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupGetSourceInfoW(
    IN  HINF   InfHandle,
    IN  UINT   SourceId,
    IN  UINT   InfoDesired,
    OUT PWSTR  ReturnBuffer,     OPTIONAL
    IN  DWORD  ReturnBufferSize,
    OUT PDWORD RequiredSize      OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(SourceId);
    UNREFERENCED_PARAMETER(InfoDesired);
    UNREFERENCED_PARAMETER(ReturnBuffer);
    UNREFERENCED_PARAMETER(ReturnBufferSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupGetSourceInfo(
    IN  HINF   InfHandle,
    IN  UINT   SourceId,
    IN  UINT   InfoDesired,
    OUT PTSTR  ReturnBuffer,     OPTIONAL
    IN  DWORD  ReturnBufferSize,
    OUT PDWORD RequiredSize      OPTIONAL
    )
//
// Native version
//
{
    return pSetupGetSourceInfo(InfHandle,
                                NULL,
                                SourceId,
                                InfoDesired,
                                ReturnBuffer,
                                ReturnBufferSize,
                                RequiredSize);
}

BOOL
pSetupGetSourceInfo(
    IN  HINF        InfHandle,         OPTIONAL
    IN  PINFCONTEXT LayoutLineContext, OPTIONAL
    IN  UINT        SourceId,
    IN  UINT        InfoDesired,
    OUT PTSTR       ReturnBuffer,      OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT PDWORD      RequiredSize       OPTIONAL
    )
/*++

Routine Description:

    Get information from SourceDisksNames

    BugBug(jamiehun) 8/9/99
    If InfHandle specified instead of LayoutLineContext
    and the ID is specified in more than one INF
    then the wrong information *MAY* be returned.
    This effects callers of SetupGetSourceInfo
    we need a SetupGetSourceInfoEx post 5.0

Arguments:

    InfHandle - required if LayoutLineContext is not provided, else specifies a layout inf

    SourceId  - numerical source ID, used as search key in SourceDisksNames section

    InfoDesired -
        SRCINFO_PATH
        SRCINFO_TAGFILE
        SRCINFO_DESCRIPTION
        SRCINFO_FLAGS

    ReturnBuffer - buffer for returned string
    ReturnBufferSize - size of buffer
    RequiredSize - size buffer needs to be if ReturnBufferSize too small
    LayoutLineContext - if specified, used to determine correct INF to use if SourceID's conflict

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information.
    ReturnBuffer filled out with string
    RequiredSize filled out with required size of buffer to hold string

--*/
{
    UINT ValueIndex;
    BOOL Mandatory;
    BOOL IsPath;
    INFCONTEXT InfContext;
    INFCONTEXT SelectedInfContext;
    int SelectedRank;
    TCHAR SourceIdString[24];
    PCTSTR Value;
    BOOL b;
    UINT Length;
    TCHAR MediaListSectionName[64];
    HINF hInfPreferred = (HINF)(-1);

    try {
        if ((LayoutLineContext != NULL) && (LayoutLineContext != (PINFCONTEXT)(-1))) {
            hInfPreferred = (HINF)LayoutLineContext->CurrentInf;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        hInfPreferred = (HINF)(-1);
    }

    //
    // Determine the index of the value that gives the caller the info he wants.
    //
    switch(InfoDesired) {

    case SRCINFO_PATH:
        ValueIndex = DISKNAMESECT_PATH;
        Mandatory = FALSE;
        IsPath = TRUE;
        break;

    case SRCINFO_TAGFILE:
        ValueIndex = DISKNAMESECT_TAGFILE;
        Mandatory = FALSE;
        IsPath = TRUE;
        break;

    case SRCINFO_DESCRIPTION:
        ValueIndex = DISKNAMESECT_DESCRIPTION;
        Mandatory = TRUE;
        IsPath = FALSE;
        break;

    case SRCINFO_FLAGS:
        ValueIndex = DISKNAMESECT_FLAGS;
        Mandatory = FALSE;
        IsPath = FALSE;
        break;

    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    wsprintf(SourceIdString,TEXT("%d"),SourceId);
    _sntprintf(
        MediaListSectionName,
        sizeof(MediaListSectionName)/sizeof(MediaListSectionName[0]),
        TEXT("%s.%s"),
        pszSourceDisksNames,
        PlatformName
        );

    //
    // we will prefer
    // (1) an entry in hInfPreferred           (Rank 11/12 decorated over undecorated)
    // (2) an entry linked to hInfPreferred    (Rank 21/22 decorated over undecorated)
    // (3) an entry in hInfHandle              (Rank 31/32 decorated over undecorated)
    // (4) an entry linked to InfHandle        (Rank 41/42 decorated over undecorated)
    //

    SelectedRank = 100;       // 11-42 as above

    if ((hInfPreferred != NULL) && (hInfPreferred != (HINF)(-1))) {
        //
        // see if we can find the SourceIdString in the INF that we found the section in
        //
        // rank 11 or 21 (decorated) - always try
        //
        if(SetupFindFirstLine(hInfPreferred,MediaListSectionName,SourceIdString,&InfContext)) {
            if (InfContext.Inf == InfContext.CurrentInf) {
                SelectedRank = 11;
                SelectedInfContext = InfContext;
            } else {
                SelectedRank = 21;
                SelectedInfContext = InfContext;
            }
        }
        if (SelectedRank > 12) {
            //
            // rank 12 or 22 (undecorated) only try if we haven't got anything better than 12
            //
            if(SetupFindFirstLine(hInfPreferred,pszSourceDisksNames,SourceIdString,&InfContext)) {
                if (InfContext.Inf == InfContext.CurrentInf) {
                    SelectedRank = 12;
                    SelectedInfContext = InfContext;
                } else if (SelectedRank > 22) {
                    SelectedRank = 22;
                    SelectedInfContext = InfContext;
                }
            }
        }
    }
    if ((InfHandle != NULL) && (InfHandle != (HINF)(-1)) && (SelectedRank > 31)) {
        //
        // see if we can find the SourceIdString in the supplied INF
        //
        // rank 31 or 41 (decorated) - only try if we haven't got anything better than 31
        //
        if(SetupFindFirstLine(InfHandle,MediaListSectionName,SourceIdString,&InfContext)) {
            if (InfContext.Inf == InfContext.CurrentInf) {
                SelectedRank = 31;
                SelectedInfContext = InfContext;
            } else if (SelectedRank > 41) {
                SelectedRank = 41;
                SelectedInfContext = InfContext;
            }
        }
        if (SelectedRank > 32) {
            //
            // rank 32 or 42 (undecorated) - only try if we haven't got anything better than 32
            //
            if(SetupFindFirstLine(InfHandle,pszSourceDisksNames,SourceIdString,&InfContext)) {
                if (InfContext.Inf == InfContext.CurrentInf) {
                    SelectedRank = 32;
                    SelectedInfContext = InfContext;
                } else if (SelectedRank > 42) {
                    SelectedRank = 42;
                    SelectedInfContext = InfContext;
                }
            }
        }
    }
    if(SelectedRank == 100 || (Value = pSetupGetField(&InfContext,ValueIndex))==NULL) {
        if(Mandatory) {
            SetLastError(ERROR_LINE_NOT_FOUND);
            return(FALSE);
        } else {
            Value = TEXT("");
        }
    }

    //
    // Figure out how many characters are in the output.
    // If the value is a path type value we want to remove
    // the trailing backslash if there is one.
    //
    Length = lstrlen(Value);
#ifdef UNICODE
    if(IsPath && Length && (Value[Length-1] == TEXT('\\'))) {
        Length--;
    }
#else
    if(IsPath && Length && (*CharPrev(Value,Value+Length) == TEXT('\\'))) {
        Length--;
    }
#endif

    //
    // Need to leave space for the trailing nul.
    //
    Length++;
    if(RequiredSize) {
        b = TRUE;
        try {
            *RequiredSize = Length;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            b = FALSE;
        }
        if(!b) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }
    }

    b = TRUE;
    if(ReturnBuffer) {
        if(Length <= ReturnBufferSize) {
            //
            // lstrcpyn is a strange API but the below is correct --
            // the size parameter is actually the capacity of the
            // target buffer. So to get it to put the nul in the
            // right place we pass one larger than the number of chars
            // we want copied.
            //
            if(!lstrcpyn(ReturnBuffer,Value,Length)) {
                //
                // ReturnBuffer invalid
                //
                b = FALSE;
                SetLastError(ERROR_INVALID_PARAMETER);
            }
        } else {
            b = FALSE;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    return(b);
}
