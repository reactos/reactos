/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        apps/cabman/dfp.cpp
 * PURPOSE:     Directive file parser
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:       The directive file format is similar to the
 *              directive file format used by Microsoft's MAKECAB
 * REVISIONS:
 *   CSH 21/03-2001 Created
 */
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "cabinet.h"
#include "dfp.h"


/* CDFParser */

CDFParser::CDFParser()
/*
 * FUNCTION: Default constructor
 */
{
    FileBuffer     = NULL;
    FileLoaded     = FALSE;
    CurrentOffset  = 0;
    CurrentLine    = 0;
    CabinetCreated = FALSE;
    DiskCreated    = FALSE;
    FolderCreated  = FALSE;
    CabinetName    = NULL;
    DiskLabel      = NULL;
    MaxDiskSize    = NULL;

    MaxDiskSizeAllSet      = FALSE;
    CabinetNameTemplateSet = FALSE;
    DiskLabelTemplateSet   = FALSE;
}


CDFParser::~CDFParser()
/*
 * FUNCTION: Default destructor
 */
{
    PCABINET_NAME CNPrev;
    PCABINET_NAME CNNext;
    PDISK_NUMBER DNPrev;
    PDISK_NUMBER DNNext;

    if (FileBuffer)
        HeapFree(GetProcessHeap(), 0, FileBuffer);
    CNNext = CabinetName;
    while (CNNext != NULL) {
        CNPrev = CNNext->Next;
        HeapFree(GetProcessHeap(), 0, CNNext);
        CNNext = CNPrev;
    }
    CNNext = DiskLabel;
    while (CNNext != NULL) {
        CNPrev = CNNext->Next;
        HeapFree(GetProcessHeap(), 0, CNNext);
        CNNext = CNPrev;
    }
    DNNext = MaxDiskSize;
    while (DNNext != NULL) {
        DNPrev = DNNext->Next;
        HeapFree(GetProcessHeap(), 0, DNNext);
        DNNext = DNPrev;
    }
}


ULONG CDFParser::Load(LPTSTR FileName)
/*
 * FUNCTION: Loads a directive file into memory
 * ARGUMENTS:
 *     FileName = Pointer to name of directive file
 * RETURNS:
 *     Status of operation
 */
{
    DWORD BytesRead;
    LONG FileSize;

    if (FileLoaded)
        return CAB_STATUS_SUCCESS;

    /* Create cabinet file, overwrite if it already exists */
    FileHandle = CreateFile(FileName, // Create this file
        GENERIC_READ,                 // Open for reading
        0,                            // No sharing
        NULL,                         // No security
        OPEN_EXISTING,                // Open the file
        FILE_ATTRIBUTE_NORMAL,        // Normal file
        NULL);                        // No attribute template
    if (FileHandle == INVALID_HANDLE_VALUE)
        return CAB_STATUS_CANNOT_OPEN;

    FileSize = GetFileSize(FileHandle, NULL);
    if (FileSize < 0) {
        CloseHandle(FileHandle);
        return CAB_STATUS_CANNOT_OPEN;
    }

    FileBufferSize = (ULONG)FileSize;

    FileBuffer = (PCHAR)HeapAlloc(GetProcessHeap(), 0, FileBufferSize);
    if (!FileBuffer) {
        CloseHandle(FileHandle);
        return CAB_STATUS_NOMEMORY;
    }

    if (!ReadFile(FileHandle, FileBuffer, FileBufferSize, &BytesRead, NULL)) {
        CloseHandle(FileHandle);
        HeapFree(GetProcessHeap(), 0, FileBuffer);
        FileBuffer = NULL;
        return CAB_STATUS_CANNOT_READ;
    }

    CloseHandle(FileHandle);

    FileLoaded = TRUE;

    DPRINT(MAX_TRACE, ("File (%d bytes)\n", FileBufferSize));

    return CAB_STATUS_SUCCESS;
}


ULONG CDFParser::Parse()
/*
 * FUNCTION: Parses a loaded directive file
 * RETURNS:
 *     Status of operation
 */
{
    BOOL Command;
    ULONG Status;

    if (!FileLoaded)
        return CAB_STATUS_NOFILE;

    while (ReadLine()) {
        Command = FALSE;

        while (CurrentToken != TokenEnd) {
            switch (CurrentToken) {
            case TokenInteger:
                itoa(CurrentInteger, (LPTSTR)&CurrentString, 10);
            case TokenIdentifier:
                if (Command) {
                    /* Command */
                    Status = PerformCommand();

                    if (Status == CAB_STATUS_FAILURE) {
                        printf("Directive file contains errors at line %d.\n", (UINT)CurrentLine);
                        DPRINT(MID_TRACE, ("Error while executing command.\n"));
                    }

                    if (Status != CAB_STATUS_SUCCESS)
                        return Status;
                } else {
                    /* File copy */
                    Status = PerformFileCopy();

                    if (Status == CAB_STATUS_FAILURE) {
                        printf("Directive file contains errors at line %d.\n", (UINT)CurrentLine);
                        DPRINT(MID_TRACE, ("Error while copying file.\n"));
                    }

                    if (Status != CAB_STATUS_SUCCESS)
                        return Status;
                }
                break;
            case TokenSpace:
                break;
            case TokenSemi:
                CurrentToken = TokenEnd;
                continue;
            case TokenPeriod:
                Command = TRUE;
                break;
            default:
                printf("Directive file contains errors at line %d.\n", (UINT)CurrentLine);
                DPRINT(MIN_TRACE, ("Token is (%d).\n", (UINT)CurrentToken));
                return CAB_STATUS_SUCCESS;
            }
            NextToken();
        }
    }

	printf("\nWriting cabinet. This may take a while...\n\n");

    if (DiskCreated) {
        Status = WriteDisk(FALSE);
        if (Status == CAB_STATUS_SUCCESS)
            Status = CloseDisk();
        if (Status != CAB_STATUS_SUCCESS) {
            DPRINT(MIN_TRACE, ("Cannot write disk (%d).\n", (UINT)Status));
            return Status;
        }
    }

    if (CabinetCreated) {
        Status = CloseCabinet();
        if (Status != CAB_STATUS_SUCCESS) {
            DPRINT(MIN_TRACE, ("Cannot close cabinet (%d).\n", (UINT)Status));
            return Status;
        }
    }

	printf("\nDone.\n");

    return CAB_STATUS_SUCCESS;
}


BOOL CDFParser::OnDiskLabel(ULONG Number, LPTSTR Label)
/*
 * FUNCTION: Called when a disk needs a label
 * ARGUMENTS:
 *     Number = Cabinet number that needs a label
 *     Label  = Pointer to buffer to place label of disk
 * RETURNS:
 *     TRUE if a disk label was returned, FALSE if not
 */
{
    TCHAR Buffer[20];
    INT i, j;
    TCHAR ch;

    Number += 1;

    DPRINT(MID_TRACE, ("Giving disk (%d) a label...\n", (UINT)Number));

    if (GetDiskName(&DiskLabel, Number, Label))
        return TRUE;

    if (DiskLabelTemplateSet) {
        j = 0;
        lstrcpy(Label, "");
        for (i = 0; i < lstrlen(DiskLabelTemplate); i++) {
            ch = DiskLabelTemplate[i];
            if (ch == '*') {
                lstrcat(Label, itoa(Number, Buffer, 10));
                j += lstrlen(Buffer);
            } else {
                Label[j] = ch;
                j++;
            }
            Label[j] = '\0';
        }

        DPRINT(MID_TRACE, ("Giving disk (%s) as a label...\n", Label));

        return TRUE;
    } else
        return FALSE;
}


BOOL CDFParser::OnCabinetName(ULONG Number, LPTSTR Name)
/*
 * FUNCTION: Called when a cabinet needs a name
 * ARGUMENTS:
 *     Number = Disk number that needs a name
 *     Name   = Pointer to buffer to place name of cabinet
 * RETURNS:
 *     TRUE if a cabinet name was returned, FALSE if not
 */
{
    TCHAR Buffer[20];
    INT i, j;
    TCHAR ch;

    Number += 1;

    DPRINT(MID_TRACE, ("Giving cabinet (%d) a name...\n", (UINT)Number));

    if (GetDiskName(&CabinetName, Number, Name))
        return TRUE;

    if (CabinetNameTemplateSet) {
        j = 0;
        lstrcpy(Name, "");
        for (i = 0; i < lstrlen(CabinetNameTemplate); i++) {
            ch = CabinetNameTemplate[i];
            if (ch == '*') {
                lstrcat(Name, itoa(Number, Buffer, 10));
                j += lstrlen(Buffer);
            } else {
                Name[j] = ch;
                j++;
            }
            Name[j] = '\0';
        }

        DPRINT(MID_TRACE, ("Giving cabinet (%s) as a name...\n", Name));

        return TRUE;
    } else
        return FALSE;
}


BOOL CDFParser::SetDiskName(PCABINET_NAME *List, ULONG Number, LPTSTR String)
/*
 * FUNCTION: Sets an entry in a list
 * ARGUMENTS:
 *     List   = Address of pointer to list
 *     Number = Disk number
 *     String = Pointer to string
 * RETURNS:
 *     FALSE if there was not enough free memory available
 */
{
    PCABINET_NAME CN;

    CN = *List;
    while (CN != NULL) {
        if (CN->DiskNumber == Number) {
            lstrcpy(CN->Name, String);
            return TRUE;
        }
        CN = CN->Next;
    }

    CN = (PCABINET_NAME)HeapAlloc(GetProcessHeap(), 0, sizeof(CABINET_NAME));
    if (!CN)
        return FALSE;

    CN->DiskNumber = Number;
    lstrcpy(CN->Name, String);

    CN->Next = *List;
    *List = CN;

    return TRUE;
}


BOOL CDFParser::GetDiskName(PCABINET_NAME *List, ULONG Number, LPTSTR String)
/*
 * FUNCTION: Returns an entry in a list
 * ARGUMENTS:
 *     List   = Address of pointer to list
 *     Number = Disk number
 *     String = Address of buffer to copy string to
 * RETURNS:
 *     FALSE if there was not enough free memory available
 */
{
    PCABINET_NAME CN;

    CN = *List;
    while (CN != NULL) {
        if (CN->DiskNumber == Number) {
            lstrcpy(String, CN->Name);
            return TRUE;
        }
        CN = CN->Next;
    }

    return FALSE;
}


BOOL CDFParser::SetDiskNumber(PDISK_NUMBER *List, ULONG Number, ULONG Value)
/*
 * FUNCTION: Sets an entry in a list
 * ARGUMENTS:
 *     List   = Address of pointer to list
 *     Number = Disk number
 *     Value  = Value to set
 * RETURNS:
 *     FALSE if there was not enough free memory available
 */
{
    PDISK_NUMBER DN;

    DN = *List;
    while (DN != NULL) {
        if (DN->DiskNumber == Number) {
            DN->Number = Value;
            return TRUE;
        }
        DN = DN->Next;
    }

    DN = (PDISK_NUMBER)HeapAlloc(GetProcessHeap(), 0, sizeof(DISK_NUMBER));
    if (!DN)
        return FALSE;

    DN->DiskNumber = Number;
    DN->Number = Value;

    DN->Next = *List;
    *List = DN;

    return TRUE;
}


BOOL CDFParser::GetDiskNumber(PDISK_NUMBER *List, ULONG Number, PULONG Value)
/*
 * FUNCTION: Returns an entry in a list
 * ARGUMENTS:
 *     List   = Address of pointer to list
 *     Number = Disk number
 *     Value  = Address of buffer to place value
 * RETURNS:
 *     TRUE if the entry was found
 */
{
    PDISK_NUMBER DN;

    DN = *List;
    while (DN != NULL) {
        if (DN->DiskNumber == Number) {
            *Value = DN->Number;
            return TRUE;
        }
        DN = DN->Next;
    }

    return FALSE;
}


BOOL CDFParser::DoDiskLabel(ULONG Number, LPTSTR Label)
/*
 * FUNCTION: Sets the label of a disk
 * ARGUMENTS:
 *     Number = Disk number
 *     Label  = Pointer to label of disk
 * RETURNS:
 *     FALSE if there was not enough free memory available
 */
{
    DPRINT(MID_TRACE, ("Setting label of disk (%d) to '%s'\n", (UINT)Number, Label));

    return SetDiskName(&DiskLabel, Number, Label);
}


VOID CDFParser::DoDiskLabelTemplate(LPTSTR Template)
/*
 * FUNCTION: Sets a disk label template to use
 * ARGUMENTS:
 *     Template = Pointer to disk label template
 */
{
    DPRINT(MID_TRACE, ("Setting disk label template to '%s'\n", Template));

    lstrcpy(DiskLabelTemplate, Template);
    DiskLabelTemplateSet = TRUE;
}


BOOL CDFParser::DoCabinetName(ULONG Number, LPTSTR Name)
/*
 * FUNCTION: Sets the name of a cabinet
 * ARGUMENTS:
 *     Number = Disk number
 *     Name   = Pointer to name of cabinet
 * RETURNS:
 *     FALSE if there was not enough free memory available
 */
{
    DPRINT(MID_TRACE, ("Setting name of cabinet (%d) to '%s'\n", (UINT)Number, Name));

    return SetDiskName(&CabinetName, Number, Name);
}


VOID CDFParser::DoCabinetNameTemplate(LPTSTR Template)
/*
 * FUNCTION: Sets a cabinet name template to use
 * ARGUMENTS:
 *     Template = Pointer to cabinet name template
 */
{
    DPRINT(MID_TRACE, ("Setting cabinet name template to '%s'\n", Template));

    lstrcpy(CabinetNameTemplate, Template);
    CabinetNameTemplateSet = TRUE;
}


ULONG CDFParser::DoMaxDiskSize(BOOL NumberValid, ULONG Number)
/*
 * FUNCTION: Sets the maximum disk size
 * ARGUMENTS:
 *     NumberValid = TRUE if disk number is valid
 *     Number      = Disk number
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     Standard sizes are 2.88M, 1.44M, 1.25M, 1.2M, 720K, 360K, and CDROM
 */
{
    ULONG A, B, Value;

    if (IsNextToken(TokenInteger, TRUE)) {

        A = CurrentInteger;

        if (IsNextToken(TokenPeriod, FALSE)) {
            if (!IsNextToken(TokenInteger, FALSE))
                return CAB_STATUS_FAILURE;

            B = CurrentInteger;

        } else
            B = 0;

        if (CurrentToken == TokenIdentifier) {
            switch (CurrentString[0]) {
            case 'K':
                if (B != 0)
                    return CAB_STATUS_FAILURE;

                if (A == 720)
                    /* 720K disk */
                    Value = 730112;
                else if (A == 360)
                    /* 360K disk */
                    Value = 362496;
                else
                    return CAB_STATUS_FAILURE;
                break;
            case 'M':
                if (A == 1) {
                    if (B == 44)
                        /* 1.44M disk */
                        Value = 1457664;
                    else if (B == 25)
                        /* 1.25M disk */
                        Value = 1300000; // FIXME: Value?
                    else if (B == 2)
                        /* 1.2M disk */
                        Value = 1213952;
                    else
                        return CAB_STATUS_FAILURE;
                } else if (A == 2) {
                    if (B == 88)
                        /* 2.88M disk */
                        Value = 2915328;
                    else
                        return CAB_STATUS_FAILURE;
                } else
                    return CAB_STATUS_FAILURE;
                break;
            default:
                DPRINT(MID_TRACE, ("Bad suffix (%c)\n", CurrentString[0]));
                return CAB_STATUS_FAILURE;
            }
        } else
            Value = A;
    } else {
        if ((CurrentToken != TokenString) &&
            (strcmpi(CurrentString, "CDROM") != 0))
            return CAB_STATUS_FAILURE;
        /* CDROM */
        Value = 640*1024*1024;  // FIXME: Correct size for CDROM?
    }

    if (NumberValid)
        return (SetDiskNumber(&MaxDiskSize, Number, Value)?
            CAB_STATUS_SUCCESS : CAB_STATUS_FAILURE);

    MaxDiskSizeAll    = Value;
    MaxDiskSizeAllSet = TRUE;

    SetMaxDiskSize(Value);

    return CAB_STATUS_SUCCESS;
}


ULONG CDFParser::SetupNewDisk()
/*
 * FUNCTION: Sets up parameters for a new disk
 * RETURNS:
 *     Status of operation
 */
{
    ULONG Value;

    if (!GetDiskNumber(&MaxDiskSize, GetCurrentDiskNumber(), &Value))  {
        if (MaxDiskSizeAllSet)
            Value = MaxDiskSizeAll;
        else
            Value = 0;
    }
    SetMaxDiskSize(Value);

    return CAB_STATUS_SUCCESS;
}


ULONG CDFParser::PerformSetCommand()
/*
 * FUNCTION: Performs a set variable command
 * RETURNS:
 *     Status of operation
 */
{
    SETTYPE SetType;
    BOOL NumberValid = FALSE;
    ULONG Number = 0;

    if (!IsNextToken(TokenIdentifier, TRUE))
        return CAB_STATUS_FAILURE;

    if (strcmpi(CurrentString, "DiskLabel") == 0)
        SetType = stDiskLabel;
    else if (strcmpi(CurrentString, "DiskLabelTemplate") == 0)
        SetType = stDiskLabelTemplate;
    else if (strcmpi(CurrentString, "CabinetName") == 0)
        SetType = stCabinetName;
    else if (strcmpi(CurrentString, "CabinetNameTemplate") == 0)
        SetType = stCabinetNameTemplate;
    else if (strcmpi(CurrentString, "MaxDiskSize") == 0)
        SetType = stMaxDiskSize;
    else
        return CAB_STATUS_FAILURE;

    if ((SetType == stDiskLabel) || (SetType == stCabinetName)) {
        if (!IsNextToken(TokenInteger, FALSE))
            return CAB_STATUS_FAILURE;
        Number = CurrentInteger;

        if (!IsNextToken(TokenEqual, TRUE))
            return CAB_STATUS_FAILURE;
    } else if (SetType == stMaxDiskSize) {
        if (IsNextToken(TokenInteger, FALSE)) {
            NumberValid = TRUE;
            Number = CurrentInteger;
        } else {
            NumberValid = FALSE;
            while (CurrentToken == TokenSpace)
                NextToken();
            if (CurrentToken != TokenEqual)
                return CAB_STATUS_FAILURE;
        }
    } else if (!IsNextToken(TokenEqual, TRUE))
            return CAB_STATUS_FAILURE;

    if (SetType != stMaxDiskSize) {
        if (!IsNextToken(TokenString, TRUE))
            return CAB_STATUS_FAILURE;
    }

    switch (SetType) {
    case stDiskLabel:
        if (!DoDiskLabel(Number, CurrentString))
            DPRINT(MIN_TRACE, ("Not enough available free memory.\n"));
        return CAB_STATUS_SUCCESS;
    case stCabinetName:
        if (!DoCabinetName(Number, CurrentString))
            DPRINT(MIN_TRACE, ("Not enough available free memory.\n"));
        return CAB_STATUS_SUCCESS;
    case stDiskLabelTemplate:
        DoDiskLabelTemplate(CurrentString);
        return CAB_STATUS_SUCCESS;
    case stCabinetNameTemplate:
        DoCabinetNameTemplate(CurrentString);
        return CAB_STATUS_SUCCESS;
    case stMaxDiskSize:
        return DoMaxDiskSize(NumberValid, Number);
    default:
        return CAB_STATUS_FAILURE;
    }
}


ULONG CDFParser::PerformNewCommand()
/*
 * FUNCTION: Performs a new disk|cabinet|folder command
 * RETURNS:
 *     Status of operation
 */
{
    NEWTYPE NewType;
    ULONG Status;

    if (!IsNextToken(TokenIdentifier, TRUE))
        return CAB_STATUS_FAILURE;

    if (strcmpi(CurrentString, "Disk") == 0)
        NewType = ntDisk;
    else if (strcmpi(CurrentString, "Cabinet") == 0)
        NewType = ntCabinet;
    else if (strcmpi(CurrentString, "Folder") == 0)
        NewType = ntFolder;
    else
        return CAB_STATUS_FAILURE;

    switch (NewType) {
    case ntDisk:
        if (DiskCreated) {
            Status = WriteDisk(TRUE);
            if (Status == CAB_STATUS_SUCCESS)
                Status = CloseDisk();
            if (Status != CAB_STATUS_SUCCESS) {
                DPRINT(MIN_TRACE, ("Cannot write disk (%d).\n", (UINT)Status));
                return CAB_STATUS_SUCCESS;
            }
            DiskCreated = FALSE;
        }

        Status = NewDisk();
        if (Status != CAB_STATUS_SUCCESS) {
            DPRINT(MIN_TRACE, ("Cannot create disk (%d).\n", (UINT)Status));
            return CAB_STATUS_SUCCESS;
        }
        DiskCreated = TRUE;
        SetupNewDisk();
        return CAB_STATUS_SUCCESS;
    case ntCabinet:
        if (DiskCreated) {
            Status = WriteDisk(TRUE);
            if (Status == CAB_STATUS_SUCCESS)
                Status = CloseDisk();
            if (Status != CAB_STATUS_SUCCESS) {
                DPRINT(MIN_TRACE, ("Cannot write disk (%d).\n", (UINT)Status));
                return CAB_STATUS_SUCCESS;
            }
            DiskCreated = FALSE;
        }

        Status = NewCabinet();
        if (Status != CAB_STATUS_SUCCESS) {
            DPRINT(MIN_TRACE, ("Cannot create cabinet (%d).\n", (UINT)Status));
            return CAB_STATUS_SUCCESS;
        }
        DiskCreated = TRUE;
        SetupNewDisk();
        return CAB_STATUS_SUCCESS;
    case ntFolder:
        Status = NewFolder();
		ASSERT(Status == CAB_STATUS_SUCCESS);
		return CAB_STATUS_SUCCESS;
    default:
        return CAB_STATUS_FAILURE;
    }
}


ULONG CDFParser::PerformCommand()
/*
 * FUNCTION: Performs a command
 * RETURNS:
 *     Status of operation
 */
{
    if (strcmpi(CurrentString, "Set") == 0)
        return PerformSetCommand();
    if (strcmpi(CurrentString, "New") == 0)
        return PerformNewCommand();

    return CAB_STATUS_FAILURE;
}


ULONG CDFParser::PerformFileCopy()
/*
 * FUNCTION: Performs a file copy
 * RETURNS:
 *     Status of operation
 */
{
    ULONG Status;
    ULONG i, j;
    TCHAR ch;
    TCHAR SrcName[MAX_PATH];
    TCHAR DstName[MAX_PATH];

    lstrcpy(SrcName, "");
    lstrcpy(DstName, "");

    i = lstrlen(CurrentString);
    while ((i < LineLength) &&
        ((ch = Line[i]) != ' ') &&
         (ch != 0x09) &&
         (ch != ';')) {
        CurrentString[i] = ch;
        i++;
    }
    CurrentString[i] = '\0';
    CurrentToken = TokenString;
    CurrentChar  = i + 1;
    lstrcpy(SrcName, CurrentString);

    SkipSpaces();

    if (CurrentToken != TokenEnd) {
        j = lstrlen(CurrentString); i = 0;
        while ((CurrentChar + i < LineLength) &&
            ((ch = Line[CurrentChar + i]) != ' ') &&
             (ch != 0x09) &&
             (ch != ';')) {
            CurrentString[j + i] = ch;
            i++;
        }
        CurrentString[j + i] = '\0';
        CurrentToken = TokenString;
        CurrentChar += i + 1;
        lstrcpy(DstName, CurrentString);
    }

    if (!CabinetCreated) {

        DPRINT(MID_TRACE, ("Creating cabinet.\n"));

        Status = NewCabinet();
        if (Status != CAB_STATUS_SUCCESS) {
            DPRINT(MIN_TRACE, ("Cannot create cabinet (%d).\n", (UINT)Status));
            printf("Cannot create cabinet.\n");
            return CAB_STATUS_FAILURE;
        }
        CabinetCreated = TRUE;

        DPRINT(MID_TRACE, ("Creating disk.\n"));

        Status = NewDisk();
        if (Status != CAB_STATUS_SUCCESS) {
            DPRINT(MIN_TRACE, ("Cannot create disk (%d).\n", (UINT)Status));
            printf("Cannot create disk.\n");
            return CAB_STATUS_FAILURE;
        }
        DiskCreated = TRUE;
        SetupNewDisk();
    }

    DPRINT(MID_TRACE, ("Adding file: '%s'   destination: '%s'.\n", SrcName, DstName));

    Status = AddFile(SrcName);
    if (Status != CAB_STATUS_SUCCESS) {
        if (Status == CAB_STATUS_CANNOT_OPEN)
		    printf("File does not exist: %s.\n", SrcName);
        else if (Status == CAB_STATUS_NOMEMORY)
            printf("Insufficient memory to add file: %s.\n", SrcName);
        else
            printf("Cannot add file: %s (%d).\n", SrcName, Status);
        return Status;
    }

    return CAB_STATUS_SUCCESS;
}


VOID CDFParser::SkipSpaces()
/*
 * FUNCTION: Skips any spaces in the current line
 */
{
    NextToken();
    while (CurrentToken == TokenSpace)
        NextToken();
}


BOOL CDFParser::IsNextToken(TOKEN Token, BOOL NoSpaces)
/*
 * FUNCTION: Checks if next token equals Token
 * ARGUMENTS:
 *     Token  = Token to compare with
 *     SkipSp = TRUE if spaces should be skipped
 * RETURNS:
 *     FALSE if next token is diffrent from Token
 */
{
    if (NoSpaces)
        SkipSpaces();
    else
        NextToken();
    return (CurrentToken == Token);
}


BOOL CDFParser::ReadLine()
/*
 * FUNCTION: Reads the next line into the line buffer
 * RETURNS:
 *     TRUE if there is a new line, FALSE if not
 */
{
    ULONG i, j;
    TCHAR ch;

    if (CurrentOffset >= FileBufferSize)
        return FALSE;

    i = 0;
    while (((j = CurrentOffset + i) < FileBufferSize) && (i < 127) &&
        ((ch = FileBuffer[j]) != 0x0D)) {
        Line[i] = ch;
        i++;
    }

    Line[i]    = '\0';
    LineLength = i;
    
    if (FileBuffer[CurrentOffset + i + 1] == 0x0A)
        CurrentOffset++;

    CurrentOffset += i + 1;

    CurrentChar = 0;

    CurrentLine++;

    NextToken();

    return TRUE;
}


VOID CDFParser::NextToken()
/*
 * FUNCTION: Reads the next token from the current line
 */
{
    ULONG i;
    TCHAR ch = ' ';

    if (CurrentChar >= LineLength) {
        CurrentToken = TokenEnd;
        return;
    }

    switch (Line[CurrentChar]) {
    case ' ':
    case 0x09: CurrentToken = TokenSpace;
        break;
    case ';': CurrentToken = TokenSemi;
        break;
    case '=': CurrentToken = TokenEqual;
        break;
    case '.': CurrentToken = TokenPeriod;
        break;
    case '\\': CurrentToken = TokenBackslash;
        break;
    case '"':
        i = 0;
        while ((CurrentChar + i + 1 < LineLength) &&
            ((ch = Line[CurrentChar + i + 1]) != '"')) {
            CurrentString[i] = ch;
            i++;
        }
        CurrentString[i] = '\0';
        CurrentToken = TokenString;
        CurrentChar += i + 2;
        return;
    default:
        i = 0;
        while ((CurrentChar + i < LineLength) &&
            ((ch = Line[CurrentChar + i]) >= '0') && (ch <= '9')) {
            CurrentString[i] = ch;
            i++;
        }
        if (i > 0) {
            CurrentString[i] = '\0';
            CurrentInteger = atoi((PCHAR)CurrentString);
            CurrentToken = TokenInteger;
            CurrentChar += i;
            return;
        }
        i = 0;
        while (((CurrentChar + i < LineLength) &&
            (((ch = Line[CurrentChar + i]) >= 'a') && (ch <= 'z')) ||
            ((ch >= 'A') && (ch <= 'Z')) || (ch == '_'))) {
            CurrentString[i] = ch;
            i++;
        }
        if (i > 0) {
            CurrentString[i] = '\0';
            CurrentToken = TokenIdentifier;
            CurrentChar += i;
            return;
        }
        CurrentToken = TokenUnknown;
    }
    CurrentChar++;
}

/* EOF */
