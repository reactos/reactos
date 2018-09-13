/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    infline.c

Abstract:

    Externally exposed INF routines for INF line retreival and information.

Author:

    Ted Miller (tedm) 20-Jan-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupFindFirstLineA(
    IN  HINF        InfHandle,
    IN  PCSTR       Section,
    IN  PCSTR       Key,          OPTIONAL
    OUT PINFCONTEXT Context
    )
{
    PCTSTR section,key;
    BOOL b;
    DWORD d;

    if((d = CaptureAndConvertAnsiArg(Section,&section)) != NO_ERROR) {
        //
        // Invalid arg.
        //
        SetLastError(d);
        return(FALSE);
    }

    if(Key) {
        if((d = CaptureAndConvertAnsiArg(Key,&key)) != NO_ERROR) {
            //
            // Invalid arg.
            //
            MyFree(section);
            SetLastError(d);
            return(FALSE);
        }
    } else {
        key = NULL;
    }

    b = SetupFindFirstLine(InfHandle,section,key,Context);
    //
    // We're safe in calling this here regardless of success or failure, since
    // we are ensured that SetupFindFirstLine will always call SetLastError().
    //
    d = GetLastError();

    if(key) {
        MyFree(key);
    }
    MyFree(section);

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupFindFirstLineW(
    IN  HINF        InfHandle,
    IN  PCWSTR      Section,
    IN  PCWSTR      Key,          OPTIONAL
    OUT PINFCONTEXT Context
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(Section);
    UNREFERENCED_PARAMETER(Key);
    UNREFERENCED_PARAMETER(Context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupFindFirstLine(
    IN  HINF        InfHandle,
    IN  PCTSTR      Section,
    IN  PCTSTR      Key,          OPTIONAL
    OUT PINFCONTEXT Context
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PLOADED_INF CurInf;
    PINF_SECTION InfSection;
    PINF_LINE InfLine;
    UINT LineNumber;
    UINT SectionNumber;
    DWORD d;

    d = NO_ERROR;
    try {
        if(!LockInf((PLOADED_INF)InfHandle)) {
            d = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Assume InfHandle was bad pointer
        //
        d = ERROR_INVALID_HANDLE;
    }
    if(d != NO_ERROR) {
        SetLastError(d);
        return(FALSE);
    }

    //
    // Traverse the linked list of loaded INFs, looking for the specified
    // section.
    //
    try {
        for(CurInf = (PLOADED_INF)InfHandle; CurInf; CurInf = CurInf->Next) {
            //
            // Locate the section.
            //
            if(!(InfSection = InfLocateSection(CurInf, Section, &SectionNumber))) {
                continue;
            }

            //
            // Attempt to locate the line within this section.
            //
            LineNumber = 0;
            if(InfLocateLine(CurInf, InfSection, Key, &LineNumber, &InfLine)) {
                break;
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }

    UnlockInf((PLOADED_INF)InfHandle);

    if(d != NO_ERROR) {
        SetLastError(d);
        return(FALSE);
    }

    if(CurInf) {
        //
        // Then we found the specified line.
        //
        MYASSERT(Key || !LineNumber);
        try {
            Context->Inf = (PVOID)InfHandle;
            Context->CurrentInf = (PVOID)CurInf;
            Context->Section = SectionNumber;
            Context->Line = LineNumber;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            d = ERROR_INVALID_PARAMETER;
        }
    } else {
        d = ERROR_LINE_NOT_FOUND;
    }

    SetLastError(d);
    return(d == NO_ERROR);
}


BOOL
SetupFindNextLine(
    IN  PINFCONTEXT ContextIn,
    OUT PINFCONTEXT ContextOut
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    return(SetupFindNextMatchLine(ContextIn,NULL,ContextOut));
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupFindNextMatchLineA(
    IN  PINFCONTEXT ContextIn,
    IN  PCSTR       Key,        OPTIONAL
    OUT PINFCONTEXT ContextOut
    )
{
    PWSTR key;
    BOOL b;
    DWORD d;

    if(!Key) {
        key = NULL;
        d = NO_ERROR;
    } else {
        d = CaptureAndConvertAnsiArg(Key,&key);
    }

    if (d == NO_ERROR) {
    

        b = SetupFindNextMatchLineW(ContextIn,key,ContextOut);
        d = GetLastError();

        if (key) {
            MyFree(key);
        }

    } else {
        b = FALSE;
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupFindNextMatchLineW(
    IN  PINFCONTEXT ContextIn,
    IN  PCWSTR      Key,        OPTIONAL
    OUT PINFCONTEXT ContextOut
    )
{
    UNREFERENCED_PARAMETER(ContextIn);
    UNREFERENCED_PARAMETER(Key);
    UNREFERENCED_PARAMETER(ContextOut);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupFindNextMatchLine(
    IN  PINFCONTEXT ContextIn,
    IN  PCTSTR      Key,        OPTIONAL
    OUT PINFCONTEXT ContextOut
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PLOADED_INF CurInf;
    UINT LineNumber;
    UINT SectionNumber;
    PINF_LINE Line;
    PINF_SECTION Section;
    PCTSTR SectionName;
    DWORD d;

    d = NO_ERROR;
    try {
        if(!LockInf((PLOADED_INF)ContextIn->Inf)) {
            d = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // ContextIn is a bad pointer
        //
        d = ERROR_INVALID_PARAMETER;
    }
    if(d != NO_ERROR) {
        SetLastError(d);
        return(FALSE);
    }

    //
    // Fetch values from context
    //
    try {
        CurInf = ContextIn->CurrentInf;
        SectionNumber = ContextIn->Section;
        Section = &CurInf->SectionBlock[SectionNumber];
        SectionName = pStringTableStringFromId(CurInf->StringTable, Section->SectionName);
        MYASSERT(SectionName);

        //
        // Either want next line, or to start searching for key on next line
        //
        LineNumber = ContextIn->Line+1;

        do {
            if(Section) {
                if(InfLocateLine(CurInf, Section, Key, &LineNumber, &Line)) {
                    break;
                }
            }
            if(CurInf = CurInf->Next) {
                Section = InfLocateSection(CurInf, SectionName, &SectionNumber);
                LineNumber = 0;
            }
        } while(CurInf);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }

    UnlockInf((PLOADED_INF)ContextIn->Inf);

    if(d != NO_ERROR) {
        SetLastError(d);
        return(FALSE);
    }


    if(CurInf) {
        //
        // Then we found the next line.
        //
        try {
            ContextOut->Inf = ContextIn->Inf;
            ContextOut->CurrentInf = CurInf;
            ContextOut->Section = SectionNumber;
            ContextOut->Line = LineNumber;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            d = ERROR_INVALID_PARAMETER;
        }
    } else {
        d = ERROR_LINE_NOT_FOUND;
    }

    SetLastError(d);
    return(d == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupGetLineByIndexA(
    IN  HINF        InfHandle,
    IN  PCSTR       Section,
    IN  DWORD       Index,
    OUT PINFCONTEXT Context
    )
{
    PCWSTR section;
    DWORD d;
    BOOL b;

    if((d = CaptureAndConvertAnsiArg(Section,&section)) == NO_ERROR) {

        b = SetupGetLineByIndexW(InfHandle,section,Index,Context);
        d = GetLastError();

        MyFree(section);

    } else {
        b = FALSE;
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupGetLineByIndexW(
    IN  HINF        InfHandle,
    IN  PCWSTR      Section,
    IN  DWORD       Index,
    OUT PINFCONTEXT Context
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(Section);
    UNREFERENCED_PARAMETER(Index);
    UNREFERENCED_PARAMETER(Context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupGetLineByIndex(
    IN  HINF        InfHandle,
    IN  PCTSTR      Section,
    IN  DWORD       Index,
    OUT PINFCONTEXT Context
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PLOADED_INF CurInf;
    PINF_SECTION InfSection;
    PINF_LINE InfLine;
    UINT LineNumber, CurLineNumberUB;
    UINT SectionNumber;
    DWORD d;

    d = NO_ERROR;
    try {
        if(!LockInf((PLOADED_INF)InfHandle)) {
            d =  ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_HANDLE;
    }
    if(d != NO_ERROR) {
        SetLastError(d);
        return(FALSE);
    }

    try {
        //
        // Traverse the list of loaded INFs.  For each INF that contains
        // the specified section, we check to see if the line number we're
        // looking for lies within its (adjusted) range of line numbers.
        //
        CurLineNumberUB = 0;
        for(CurInf = (PLOADED_INF)InfHandle; CurInf; CurInf = CurInf->Next) {
            //
            // Locate the section.
            //
            if(!(InfSection = InfLocateSection(CurInf, Section, &SectionNumber))) {
                continue;
            }

            //
            // See if the line number lies in this INF section's range.
            //
            MYASSERT(Index >= CurLineNumberUB);
            LineNumber = Index - CurLineNumberUB;
            if(InfLocateLine(CurInf, InfSection, NULL, &LineNumber, &InfLine)) {
                break;
            } else {
                //
                // Subtract the number of lines this INF contributes to the section's
                // total line count, and continue with the next one.
                //
                CurLineNumberUB += InfSection->LineCount;
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }

    UnlockInf((PLOADED_INF)InfHandle);

    if(d != NO_ERROR) {
        SetLastError(d);
        return(FALSE);
    }

    if(CurInf) {
        //
        // Then we found the specified line.
        //
        try {
            Context->Inf = (PVOID)InfHandle;
            Context->CurrentInf = (PVOID)CurInf;
            Context->Section = SectionNumber;
            Context->Line = LineNumber;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            d = ERROR_INVALID_PARAMETER;
        }
    } else {
        d = ERROR_LINE_NOT_FOUND;
    }

    SetLastError(d);
    return(d == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
LONG
SetupGetLineCountA(
    IN HINF  InfHandle,
    IN PCSTR Section
    )
{
    PCWSTR section;
    LONG l;
    DWORD d;

    if((d = CaptureAndConvertAnsiArg(Section,&section)) == NO_ERROR) {

        l = SetupGetLineCountW(InfHandle,section);
        d = GetLastError();

        MyFree(section);

    } else {

        l = -1;
    }

    SetLastError(d);
    return(l);
}
#else
//
// Unicode stub
//
LONG
SetupGetLineCountW(
    IN HINF   InfHandle,
    IN PCWSTR Section
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(Section);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(-1);
}
#endif

LONG
SetupGetLineCount(
    IN HINF   InfHandle,
    IN PCTSTR Section
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PLOADED_INF CurInf;
    PINF_SECTION InfSection;
    LONG LineCount;
    DWORD d;

    d = NO_ERROR;
    try {
        if(!LockInf((PLOADED_INF)InfHandle)) {
            d = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_HANDLE;
    }
    if(d != NO_ERROR) {
        SetLastError(d);
        return(-1);
    }

    try {
        //
        // Traverse the linked list of loaded INFs, and sum up the section line
        // counts for each INF containing the specified section.
        //
        LineCount = -1;
        for(CurInf = (PLOADED_INF)InfHandle; CurInf; CurInf = CurInf->Next) {
            if(InfSection = InfLocateSection(CurInf, Section, NULL)) {
                if(LineCount == -1) {
                    LineCount = InfSection->LineCount;
                } else {
                    LineCount += InfSection->LineCount;
                }
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }

    UnlockInf((PLOADED_INF)InfHandle);

    if(d != NO_ERROR) {
        SetLastError(d);
        return(-1);
    }

    if(LineCount == -1) {
        SetLastError(ERROR_SECTION_NOT_FOUND);
    } else {
        SetLastError(NO_ERROR);
    }

    return LineCount;
}

