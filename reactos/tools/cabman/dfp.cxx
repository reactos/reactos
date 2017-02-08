/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        tools/cabman/dfp.cxx
 * PURPOSE:     Directive file parser
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Colin Finck <mail@colinfinck.de>
 * NOTES:       The directive file format is similar to the
 *              directive file format used by Microsoft's MAKECAB
 * REVISIONS:
 *   CSH 21/03-2001 Created
 *   CSH 15/08-2003 Made it portable
 *   CF  04/05-2007 Made it compatible with 64-bit operating systems
 */
#include <stdlib.h>
#include <stdio.h>
#include "cabman.h"
#include "dfp.h"


#if defined(_WIN32)
#define GetSizeOfFile(handle) _GetSizeOfFile(handle)
static LONG _GetSizeOfFile(FILEHANDLE handle)
{
    ULONG size = GetFileSize(handle, NULL);
    if (size == INVALID_FILE_SIZE)
        return -1;

    return size;
}
#define ReadFileData(handle, buffer, size, bytesread) _ReadFileData(handle, buffer, size, bytesread)
static BOOL _ReadFileData(FILEHANDLE handle, void* buffer, ULONG size, PULONG bytesread)
{
    return ReadFile(handle, buffer, size, (LPDWORD)bytesread, NULL);
}
#else
#define GetSizeOfFile(handle) _GetSizeOfFile(handle)
static LONG _GetSizeOfFile(FILEHANDLE handle)
{
    LONG size;
    fseek(handle, 0, SEEK_END);
    size = ftell(handle);
    fseek(handle, 0, SEEK_SET);
    return size;
}
#define ReadFileData(handle, buffer, size, bytesread) _ReadFileData(handle, buffer, size, bytesread)
static bool _ReadFileData(FILEHANDLE handle, void* buffer, ULONG size, PULONG bytesread)
{
    *bytesread = fread(buffer, 1, size, handle);
    return *bytesread == size;
}
#endif

/* CDFParser */

CDFParser::CDFParser()
/*
 * FUNCTION: Default constructor
 */
{
    InfFileOnly     = false;
    DontGenerateInf = false;

    FileBuffer     = NULL;
    FileLoaded     = false;
    CurrentOffset  = 0;
    CurrentLine    = 0;
    CabinetCreated = false;
    DiskCreated    = false;
    FolderCreated  = false;
    CabinetName    = NULL;
    DiskLabel      = NULL;
    MaxDiskSize    = NULL;

    MaxDiskSizeAllSet      = false;
    CabinetNameTemplateSet = false;
    DiskLabelTemplateSet   = false;
    InfFileNameSet         = false;

    InfModeEnabled = false;
    InfFileHandle = NULL;

    strcpy(FileRelativePath, "");
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
        FreeMemory(FileBuffer);
    CNNext = CabinetName;
    while (CNNext != NULL)
    {
        CNPrev = CNNext->Next;
        FreeMemory(CNNext);
        CNNext = CNPrev;
    }
    CNNext = DiskLabel;
    while (CNNext != NULL)
    {
        CNPrev = CNNext->Next;
        FreeMemory(CNNext);
        CNNext = CNPrev;
    }
    DNNext = MaxDiskSize;
    while (DNNext != NULL)
    {
        DNPrev = DNNext->Next;
        FreeMemory(DNNext);
        DNNext = DNPrev;
    }

    if (InfFileHandle != NULL)
        CloseFile(InfFileHandle);
}

void CDFParser::WriteInfLine(char* InfLine)
{
    char buf[PATH_MAX];
    char eolbuf[2];
    char* destpath;
#if defined(_WIN32)
    ULONG BytesWritten;
#endif

    if (DontGenerateInf)
        return;

    if (InfFileHandle == NULL)
    {
        if (!InfFileNameSet)
            /* FIXME: Use cabinet name with extension .inf */
            return;

        destpath = GetDestinationPath();
        if (strlen(destpath) > 0)
        {
            strcpy(buf, destpath);
            strcat(buf, InfFileName);
        }
        else
            strcpy(buf, InfFileName);

        /* Create .inf file, overwrite if it already exists */
#if defined(_WIN32)
        InfFileHandle = CreateFile(buf,     // Create this file
            GENERIC_WRITE,                  // Open for writing
            0,                              // No sharing
            NULL,                           // No security
            CREATE_ALWAYS,                  // Create or overwrite
            FILE_ATTRIBUTE_NORMAL,          // Normal file
            NULL);                          // No attribute template
        if (InfFileHandle == INVALID_HANDLE_VALUE)
        {
            DPRINT(MID_TRACE, ("Error creating '%u'.\n", (UINT)GetLastError()));
            return;
        }
#else /* !_WIN32 */
        InfFileHandle = fopen(buf, "wb");
        if (InfFileHandle == NULL)
        {
            DPRINT(MID_TRACE, ("Error creating '%i'.\n", errno));
            return;
        }
#endif
    }

#if defined(_WIN32)
    if (!WriteFile(InfFileHandle, InfLine, (DWORD)strlen(InfLine), (LPDWORD)&BytesWritten, NULL))
    {
        DPRINT(MID_TRACE, ("ERROR WRITING '%u'.\n", (UINT)GetLastError()));
        return;
    }
#else
    if (fwrite(InfLine, strlen(InfLine), 1, InfFileHandle) < 1)
        return;
#endif

    eolbuf[0] = 0x0d;
    eolbuf[1] = 0x0a;

#if defined(_WIN32)
    if (!WriteFile(InfFileHandle, eolbuf, sizeof(eolbuf), (LPDWORD)&BytesWritten, NULL))
    {
        DPRINT(MID_TRACE, ("ERROR WRITING '%u'.\n", (UINT)GetLastError()));
        return;
    }
#else
    if (fwrite(eolbuf, 1, sizeof(eolbuf), InfFileHandle) < 1)
        return;
#endif
}


ULONG CDFParser::Load(char* FileName)
/*
 * FUNCTION: Loads a directive file into memory
 * ARGUMENTS:
 *     FileName = Pointer to name of directive file
 * RETURNS:
 *     Status of operation
 */
{
    ULONG BytesRead;
    LONG FileSize;

    if (FileLoaded)
        return CAB_STATUS_SUCCESS;

    /* Create cabinet file, overwrite if it already exists */
#if defined(_WIN32)
    FileHandle = CreateFile(FileName, // Create this file
        GENERIC_READ,                 // Open for reading
        0,                            // No sharing
        NULL,                         // No security
        OPEN_EXISTING,                // Open the file
        FILE_ATTRIBUTE_NORMAL,        // Normal file
        NULL);                        // No attribute template
    if (FileHandle == INVALID_HANDLE_VALUE)
        return CAB_STATUS_CANNOT_OPEN;
#else /* !_WIN32 */
    FileHandle = fopen(FileName, "rb");
    if (FileHandle == NULL)
        return CAB_STATUS_CANNOT_OPEN;
#endif

    FileSize = GetSizeOfFile(FileHandle);
    if (FileSize == -1)
    {
        CloseFile(FileHandle);
        return CAB_STATUS_CANNOT_OPEN;
    }

    FileBufferSize = (ULONG)FileSize;

    FileBuffer = (char*)AllocateMemory(FileBufferSize);
    if (!FileBuffer)
    {
        CloseFile(FileHandle);
        return CAB_STATUS_NOMEMORY;
    }

    if (!ReadFileData(FileHandle, FileBuffer, FileBufferSize, &BytesRead))
    {
        CloseFile(FileHandle);
        FreeMemory(FileBuffer);
        FileBuffer = NULL;
        return CAB_STATUS_CANNOT_READ;
    }

    CloseFile(FileHandle);

    FileLoaded = true;

    DPRINT(MAX_TRACE, ("File (%u bytes)\n", (UINT)FileBufferSize));

    return CAB_STATUS_SUCCESS;
}


ULONG CDFParser::Parse()
/*
 * FUNCTION: Parses a loaded directive file
 * RETURNS:
 *     Status of operation
 */
{
    bool Command;
    ULONG Status;

    if (!FileLoaded)
        return CAB_STATUS_NOFILE;

    while (ReadLine())
    {
        Command = false;

        if (InfModeEnabled)
        {
            bool WriteLine = true;
            while (CurrentToken != TokenEnd)
            {
                switch (CurrentToken)
                {
                    case TokenIdentifier:
                        if (Command)
                        {
                            /* Command */
                            Status = PerformCommand();
                            if (Status == CAB_STATUS_FAILURE)
                                WriteLine = true;
                            else
                                if (!InfModeEnabled)
                                    WriteLine = false;

                            CurrentToken = TokenEnd;
                            continue;
                        }
                        else
                        {
                            WriteLine = true;
                            CurrentToken = TokenEnd;
                            continue;
                        }
                        break;

                    case TokenSpace:
                        break;

                    case TokenPeriod:
                        Command = true;
                        break;

                    default:
                        WriteLine = true;
                        CurrentToken = TokenEnd;
                        continue;
                }
                NextToken();
            }
            if (WriteLine)
                WriteInfLine(Line);
        }
        else
        {
            while (CurrentToken != TokenEnd)
            {
                switch (CurrentToken)
                {
                    case TokenInteger:
                        sprintf(CurrentString, "%u", (UINT)CurrentInteger);
                    case TokenIdentifier:
                    case TokenString:
                        if (Command)
                        {
                            /* Command */
                            Status = PerformCommand();

                            if (Status == CAB_STATUS_FAILURE)
                            {
                                printf("ERROR: Directive file contains errors at line %u.\n", (UINT)CurrentLine);
                                DPRINT(MID_TRACE, ("Error while executing command.\n"));
                            }

                            if (Status != CAB_STATUS_SUCCESS)
                                return Status;
                        }
                        else
                        {
                            /* File copy */
                            Status = PerformFileCopy();

                            if (Status != CAB_STATUS_SUCCESS)
                            {
                                printf("ERROR: Directive file contains errors at line %u.\n", (UINT)CurrentLine);
                                DPRINT(MID_TRACE, ("Error while copying file.\n"));
                                return Status;
                            }
                        }
                        break;

                    case TokenSpace:
                        break;

                    case TokenSemi:
                        CurrentToken = TokenEnd;
                        continue;

                    case TokenPeriod:
                        Command = true;
                        break;

                    default:
                        printf("ERROR: Directive file contains errors at line %u.\n", (UINT)CurrentLine);
                        DPRINT(MID_TRACE, ("Token is (%u).\n", (UINT)CurrentToken));
                        return CAB_STATUS_SUCCESS;
                    }
                    NextToken();
            }
        }
    }

    if (!InfFileOnly)
    {
        if (CABMgr.IsVerbose())
        {
            printf("Writing cabinet. This may take a while...\n");
        }

        if (DiskCreated)
        {
            Status = WriteDisk(false);
            if (Status == CAB_STATUS_SUCCESS)
                Status = CloseDisk();
            if (Status != CAB_STATUS_SUCCESS)
            {
                DPRINT(MIN_TRACE, ("Cannot write disk (%u).\n", (UINT)Status));
                return Status;
            }
        }

        if (CabinetCreated)
        {
            Status = CloseCabinet();
            if (Status != CAB_STATUS_SUCCESS)
            {
                DPRINT(MIN_TRACE, ("Cannot close cabinet (%u).\n", (UINT)Status));
                return Status;
            }
        }

        if (CABMgr.IsVerbose())
        {
            printf("Done.\n");
        }
    }

    return CAB_STATUS_SUCCESS;
}


void CDFParser::SetFileRelativePath(char* Path)
/*
 * FUNCTION: Sets path where files in the .dff is assumed relative to
 * ARGUMENTS:
 *    Path = Pointer to string with path
 */
{
    strcpy(FileRelativePath, Path);
    ConvertPath(FileRelativePath, false);
    if (strlen(FileRelativePath) > 0)
        NormalizePath(FileRelativePath, PATH_MAX);
}


bool CDFParser::OnDiskLabel(ULONG Number, char* Label)
/*
 * FUNCTION: Called when a disk needs a label
 * ARGUMENTS:
 *     Number = Cabinet number that needs a label
 *     Label  = Pointer to buffer to place label of disk
 * RETURNS:
 *     true if a disk label was returned, false if not
 */
{
    char Buffer[20];
    ULONG i;
    int j;
    char ch;

    Number += 1;

    DPRINT(MID_TRACE, ("Giving disk (%u) a label...\n", (UINT)Number));

    if (GetDiskName(&DiskLabel, Number, Label))
        return true;

    if (DiskLabelTemplateSet)
    {
        j = 0;
        strcpy(Label, "");
        for (i = 0; i < strlen(DiskLabelTemplate); i++)
        {
            ch = DiskLabelTemplate[i];
            if (ch == '*')
            {
                sprintf(Buffer, "%u", (UINT)Number);
                strcat(Label, Buffer);
                j += (LONG)strlen(Buffer);
            }
            else
            {
                Label[j] = ch;
                j++;
            }
            Label[j] = '\0';
        }

        DPRINT(MID_TRACE, ("Giving disk (%s) as a label...\n", Label));

        return true;
    }
    else
        return false;
}


bool CDFParser::OnCabinetName(ULONG Number, char* Name)
/*
 * FUNCTION: Called when a cabinet needs a name
 * ARGUMENTS:
 *     Number = Disk number that needs a name
 *     Name   = Pointer to buffer to place name of cabinet
 * RETURNS:
 *     true if a cabinet name was returned, false if not
 */
{
    char Buffer[PATH_MAX];
    ULONG i;
    int j;
    char ch;

    Number += 1;

    DPRINT(MID_TRACE, ("Giving cabinet (%u) a name...\n", (UINT)Number));

    if (GetDiskName(&CabinetName, Number, Buffer))
    {
        strcpy(Name, GetDestinationPath());
        strcat(Name, Buffer);
        return true;
    }

    if (CabinetNameTemplateSet)
    {
        strcpy(Name, GetDestinationPath());
        j = (LONG)strlen(Name);
        for (i = 0; i < strlen(CabinetNameTemplate); i++)
        {
            ch = CabinetNameTemplate[i];
            if (ch == '*')
            {
                sprintf(Buffer, "%u", (UINT)Number);
                strcat(Name, Buffer);
                j += (LONG)strlen(Buffer);
            }
            else
            {
                Name[j] = ch;
                j++;
            }
            Name[j] = '\0';
        }

        DPRINT(MID_TRACE, ("Giving cabinet (%s) as a name...\n", Name));
        return true;
    }
    else
        return false;
}


bool CDFParser::SetDiskName(PCABINET_NAME *List, ULONG Number, char* String)
/*
 * FUNCTION: Sets an entry in a list
 * ARGUMENTS:
 *     List   = Address of pointer to list
 *     Number = Disk number
 *     String = Pointer to string
 * RETURNS:
 *     false if there was not enough free memory available
 */
{
    PCABINET_NAME CN;

    CN = *List;
    while (CN != NULL)
    {
        if (CN->DiskNumber == Number)
        {
            strcpy(CN->Name, String);
            return true;
        }
        CN = CN->Next;
    }

    CN = (PCABINET_NAME)AllocateMemory(sizeof(CABINET_NAME));
    if (!CN)
        return false;

    CN->DiskNumber = Number;
    strcpy(CN->Name, String);

    CN->Next = *List;
    *List = CN;

    return true;
}


bool CDFParser::GetDiskName(PCABINET_NAME *List, ULONG Number, char* String)
/*
 * FUNCTION: Returns an entry in a list
 * ARGUMENTS:
 *     List   = Address of pointer to list
 *     Number = Disk number
 *     String = Address of buffer to copy string to
 * RETURNS:
 *     false if there was not enough free memory available
 */
{
    PCABINET_NAME CN;

    CN = *List;
    while (CN != NULL)
    {
        if (CN->DiskNumber == Number)
        {
            strcpy(String, CN->Name);
            return true;
        }
        CN = CN->Next;
    }

    return false;
}


bool CDFParser::SetDiskNumber(PDISK_NUMBER *List, ULONG Number, ULONG Value)
/*
 * FUNCTION: Sets an entry in a list
 * ARGUMENTS:
 *     List   = Address of pointer to list
 *     Number = Disk number
 *     Value  = Value to set
 * RETURNS:
 *     false if there was not enough free memory available
 */
{
    PDISK_NUMBER DN;

    DN = *List;
    while (DN != NULL)
    {
        if (DN->DiskNumber == Number)
        {
            DN->Number = Value;
            return true;
        }
        DN = DN->Next;
    }

    DN = (PDISK_NUMBER)AllocateMemory(sizeof(DISK_NUMBER));
    if (!DN)
        return false;

    DN->DiskNumber = Number;
    DN->Number = Value;

    DN->Next = *List;
    *List = DN;

    return true;
}


bool CDFParser::GetDiskNumber(PDISK_NUMBER *List, ULONG Number, PULONG Value)
/*
 * FUNCTION: Returns an entry in a list
 * ARGUMENTS:
 *     List   = Address of pointer to list
 *     Number = Disk number
 *     Value  = Address of buffer to place value
 * RETURNS:
 *     true if the entry was found
 */
{
    PDISK_NUMBER DN;

    DN = *List;
    while (DN != NULL)
    {
        if (DN->DiskNumber == Number)
        {
            *Value = DN->Number;
            return true;
        }
        DN = DN->Next;
    }

    return false;
}


bool CDFParser::DoDiskLabel(ULONG Number, char* Label)
/*
 * FUNCTION: Sets the label of a disk
 * ARGUMENTS:
 *     Number = Disk number
 *     Label  = Pointer to label of disk
 * RETURNS:
 *     false if there was not enough free memory available
 */
{
    DPRINT(MID_TRACE, ("Setting label of disk (%u) to '%s'\n", (UINT)Number, Label));

    return SetDiskName(&DiskLabel, Number, Label);
}


void CDFParser::DoDiskLabelTemplate(char* Template)
/*
 * FUNCTION: Sets a disk label template to use
 * ARGUMENTS:
 *     Template = Pointer to disk label template
 */
{
    DPRINT(MID_TRACE, ("Setting disk label template to '%s'\n", Template));

    strcpy(DiskLabelTemplate, Template);
    DiskLabelTemplateSet = true;
}


bool CDFParser::DoCabinetName(ULONG Number, char* Name)
/*
 * FUNCTION: Sets the name of a cabinet
 * ARGUMENTS:
 *     Number = Disk number
 *     Name   = Pointer to name of cabinet
 * RETURNS:
 *     false if there was not enough free memory available
 */
{
    DPRINT(MID_TRACE, ("Setting name of cabinet (%u) to '%s'\n", (UINT)Number, Name));

    return SetDiskName(&CabinetName, Number, Name);
}


void CDFParser::DoCabinetNameTemplate(char* Template)
/*
 * FUNCTION: Sets a cabinet name template to use
 * ARGUMENTS:
 *     Template = Pointer to cabinet name template
 */
{
    DPRINT(MID_TRACE, ("Setting cabinet name template to '%s'\n", Template));

    strcpy(CabinetNameTemplate, Template);
    CabinetNameTemplateSet = true;
}


ULONG CDFParser::DoMaxDiskSize(bool NumberValid, ULONG Number)
/*
 * FUNCTION: Sets the maximum disk size
 * ARGUMENTS:
 *     NumberValid = true if disk number is valid
 *     Number      = Disk number
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     Standard sizes are 2.88M, 1.44M, 1.25M, 1.2M, 720K, 360K, and CDROM
 */
{
    ULONG A, B, Value;

    if (IsNextToken(TokenInteger, true))
    {
        A = CurrentInteger;

        if (IsNextToken(TokenPeriod, false))
        {
            if (!IsNextToken(TokenInteger, false))
                return CAB_STATUS_FAILURE;

            B = CurrentInteger;

        }
        else
            B = 0;

        if (CurrentToken == TokenIdentifier)
        {
            switch (CurrentString[0])
            {
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
                    if (A == 1)
                    {
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
                    }
                    else if (A == 2)
                    {
                        if (B == 88)
                            /* 2.88M disk */
                            Value = 2915328;
                        else
                            return CAB_STATUS_FAILURE;
                    }
                    else
                        return CAB_STATUS_FAILURE;
                    break;

                default:
                    DPRINT(MID_TRACE, ("Bad suffix (%c)\n", CurrentString[0]));
                    return CAB_STATUS_FAILURE;
            }
        }
        else
            Value = A;
    }
    else
    {
        if ((CurrentToken != TokenString) &&
            (strcasecmp(CurrentString, "CDROM") != 0))
            return CAB_STATUS_FAILURE;
        /* CDROM */
        Value = 640*1024*1024;  // FIXME: Correct size for CDROM?
    }

    if (NumberValid)
        return (SetDiskNumber(&MaxDiskSize, Number, Value)?
            CAB_STATUS_SUCCESS : CAB_STATUS_FAILURE);

    MaxDiskSizeAll    = Value;
    MaxDiskSizeAllSet = true;

    SetMaxDiskSize(Value);

    return CAB_STATUS_SUCCESS;
}


void CDFParser::DoInfFileName(char* FileName)
/*
 * FUNCTION: Sets filename of the generated .inf file
 * ARGUMENTS:
 *     FileName = Pointer to .inf filename
 */
{
    DPRINT(MID_TRACE, ("Setting .inf filename to '%s'\n", FileName));

    strcpy(InfFileName, FileName);
    InfFileNameSet = true;
}

ULONG CDFParser::SetupNewDisk()
/*
 * FUNCTION: Sets up parameters for a new disk
 * RETURNS:
 *     Status of operation
 */
{
    ULONG Value;

    if (!GetDiskNumber(&MaxDiskSize, GetCurrentDiskNumber(), &Value))
    {
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
    bool NumberValid = false;
    ULONG Number = 0;

    if (!IsNextToken(TokenIdentifier, true))
        return CAB_STATUS_FAILURE;

    if (strcasecmp(CurrentString, "DiskLabel") == 0)
        SetType = stDiskLabel;
    else if (strcasecmp(CurrentString, "DiskLabelTemplate") == 0)
        SetType = stDiskLabelTemplate;
    else if (strcasecmp(CurrentString, "CabinetName") == 0)
        SetType = stCabinetName;
    else if (strcasecmp(CurrentString, "CabinetNameTemplate") == 0)
        SetType = stCabinetNameTemplate;
    else if (strcasecmp(CurrentString, "MaxDiskSize") == 0)
        SetType = stMaxDiskSize;
    else if (strcasecmp(CurrentString, "InfFileName") == 0)
        SetType = stInfFileName;
    else
        return CAB_STATUS_FAILURE;

    if ((SetType == stDiskLabel) || (SetType == stCabinetName))
    {
        if (!IsNextToken(TokenInteger, false))
            return CAB_STATUS_FAILURE;
        Number = CurrentInteger;

        if (!IsNextToken(TokenEqual, true))
            return CAB_STATUS_FAILURE;
    }
    else if (SetType == stMaxDiskSize)
    {
        if (IsNextToken(TokenInteger, false))
        {
            NumberValid = true;
            Number = CurrentInteger;
        }
        else
        {
            NumberValid = false;
            while (CurrentToken == TokenSpace)
                NextToken();
            if (CurrentToken != TokenEqual)
                return CAB_STATUS_FAILURE;
        }
    }
    else if (!IsNextToken(TokenEqual, true))
            return CAB_STATUS_FAILURE;

    if (SetType != stMaxDiskSize)
    {
        if (!IsNextToken(TokenString, true))
            return CAB_STATUS_FAILURE;
    }

    switch (SetType)
    {
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

        case stInfFileName:
            DoInfFileName(CurrentString);
            return CAB_STATUS_SUCCESS;

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

    if (!IsNextToken(TokenIdentifier, true))
        return CAB_STATUS_FAILURE;

    if (strcasecmp(CurrentString, "Disk") == 0)
        NewType = ntDisk;
    else if (strcasecmp(CurrentString, "Cabinet") == 0)
        NewType = ntCabinet;
    else if (strcasecmp(CurrentString, "Folder") == 0)
        NewType = ntFolder;
    else
        return CAB_STATUS_FAILURE;

    switch (NewType)
    {
        case ntDisk:
            if (DiskCreated)
            {
                Status = WriteDisk(true);
                if (Status == CAB_STATUS_SUCCESS)
                    Status = CloseDisk();
                if (Status != CAB_STATUS_SUCCESS)
                {
                    DPRINT(MIN_TRACE, ("Cannot write disk (%u).\n", (UINT)Status));
                    return CAB_STATUS_SUCCESS;
                }
                DiskCreated = false;
            }

            Status = NewDisk();
            if (Status != CAB_STATUS_SUCCESS)
            {
                DPRINT(MIN_TRACE, ("Cannot create disk (%u).\n", (UINT)Status));
                return CAB_STATUS_SUCCESS;
            }
            DiskCreated = true;
            SetupNewDisk();
            return CAB_STATUS_SUCCESS;

        case ntCabinet:
            if (DiskCreated)
            {
                Status = WriteDisk(true);
                if (Status == CAB_STATUS_SUCCESS)
                    Status = CloseDisk();
                if (Status != CAB_STATUS_SUCCESS)
                {
                    DPRINT(MIN_TRACE, ("Cannot write disk (%u).\n", (UINT)Status));
                    return CAB_STATUS_SUCCESS;
                }
                DiskCreated = false;
            }

            Status = NewCabinet();
            if (Status != CAB_STATUS_SUCCESS)
            {
                DPRINT(MIN_TRACE, ("Cannot create cabinet (%u).\n", (UINT)Status));
                return CAB_STATUS_SUCCESS;
            }
            DiskCreated = true;
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


ULONG CDFParser::PerformInfBeginCommand()
/*
 * FUNCTION: Begins inf mode
 * RETURNS:
 *     Status of operation
 */
{
    InfModeEnabled = true;
    return CAB_STATUS_SUCCESS;
}


ULONG CDFParser::PerformInfEndCommand()
/*
 * FUNCTION: Begins inf mode
 * RETURNS:
 *     Status of operation
 */
{
    InfModeEnabled = false;
    return CAB_STATUS_SUCCESS;
}


ULONG CDFParser::PerformCommand()
/*
 * FUNCTION: Performs a command
 * RETURNS:
 *     Status of operation
 */
{
    if (strcasecmp(CurrentString, "Set") == 0)
        return PerformSetCommand();
    if (strcasecmp(CurrentString, "New") == 0)
        return PerformNewCommand();
    if (strcasecmp(CurrentString, "InfBegin") == 0)
        return PerformInfBeginCommand();
    if (strcasecmp(CurrentString, "InfEnd") == 0)
        return PerformInfEndCommand();

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
    char ch;
    char SrcName[PATH_MAX];
    char DstName[PATH_MAX];
    char InfLine[PATH_MAX];
    char Options[128];
    char BaseFilename[PATH_MAX];

    *SrcName = '\0';
    *DstName = '\0';
    *Options = '\0';

    // source file
    i = CurrentChar;
    while ((i < LineLength) &&
        ((ch = Line[i]) != ' ') &&
         (ch != 0x09) &&
         (ch != ';'))
    {
        CurrentString[i] = ch;
        i++;
    }
    CurrentString[i] = '\0';
    CurrentToken = TokenString;
    CurrentChar  = i + 1;
    strcpy(BaseFilename, CurrentString);
    strcat(SrcName, BaseFilename);

    // destination
    SkipSpaces();

    if (CurrentToken != TokenEnd)
    {
        j = (ULONG)strlen(CurrentString); i = 0;
        while ((CurrentChar + i < LineLength) &&
            ((ch = Line[CurrentChar + i]) != ' ') &&
             (ch != 0x09) &&
             (ch != ';'))
        {
            CurrentString[j + i] = ch;
            i++;
        }
        CurrentString[j + i] = '\0';
        CurrentToken = TokenString;
        CurrentChar += i + 1;
        strcpy(DstName, CurrentString);
    }

    // options (it may be empty)
    SkipSpaces ();

    if (CurrentToken != TokenEnd)
    {
        j = (ULONG)strlen(CurrentString); i = 0;
        while ((CurrentChar + i < LineLength) &&
            ((ch = Line[CurrentChar + i]) != ' ') &&
             (ch != 0x09) &&
             (ch != ';'))
        {
            CurrentString[j + i] = ch;
            i++;
        }
        CurrentString[j + i] = '\0';
        CurrentToken = TokenString;
        CurrentChar += i + 1;
        strcpy(Options, CurrentString);
    }

    if (!CabinetCreated)
    {
        DPRINT(MID_TRACE, ("Creating cabinet.\n"));

        Status = NewCabinet();
        if (Status != CAB_STATUS_SUCCESS)
        {
            DPRINT(MIN_TRACE, ("Cannot create cabinet (%u).\n", (UINT)Status));
            printf("ERROR: Cannot create cabinet.\n");
            return CAB_STATUS_FAILURE;
        }
        CabinetCreated = true;

        DPRINT(MID_TRACE, ("Creating disk.\n"));

        Status = NewDisk();
        if (Status != CAB_STATUS_SUCCESS)
        {
            DPRINT(MIN_TRACE, ("Cannot create disk (%u).\n", (UINT)Status));
            printf("ERROR: Cannot create disk.\n");
            return CAB_STATUS_FAILURE;
        }
        DiskCreated = true;
        SetupNewDisk();
    }

    DPRINT(MID_TRACE, ("Adding file: '%s'   destination: '%s'.\n", SrcName, DstName));

    Status = AddFile(SrcName);
    if (Status == CAB_STATUS_CANNOT_OPEN)
    {
        strcpy(SrcName, FileRelativePath);
        strcat(SrcName, BaseFilename);
        Status = AddFile(SrcName);
    }
    switch (Status)
    {
        case CAB_STATUS_SUCCESS:
            sprintf(InfLine, "%s=%s", GetFileName(SrcName), DstName);
            WriteInfLine(InfLine);
            break;

        case CAB_STATUS_CANNOT_OPEN:
            if (strstr(Options,"optional"))
            {
                Status = CAB_STATUS_SUCCESS;
                printf("Optional file skipped (does not exist): %s.\n", SrcName);
            }
            else
                printf("ERROR: File not found: %s.\n", SrcName);

            break;

        case CAB_STATUS_NOMEMORY:
            printf("ERROR: Insufficient memory to add file: %s.\n", SrcName);
            break;

        default:
            printf("ERROR: Cannot add file: %s (%u).\n", SrcName, (UINT)Status);
            break;
    }
    return Status;
}


void CDFParser::SkipSpaces()
/*
 * FUNCTION: Skips any spaces in the current line
 */
{
    NextToken();
    while (CurrentToken == TokenSpace)
        NextToken();
}


bool CDFParser::IsNextToken(DFP_TOKEN Token, bool NoSpaces)
/*
 * FUNCTION: Checks if next token equals Token
 * ARGUMENTS:
 *     Token  = Token to compare with
 *     SkipSp = true if spaces should be skipped
 * RETURNS:
 *     false if next token is diffrent from Token
 */
{
    if (NoSpaces)
        SkipSpaces();
    else
        NextToken();
    return (CurrentToken == Token);
}


bool CDFParser::ReadLine()
/*
 * FUNCTION: Reads the next line into the line buffer
 * RETURNS:
 *     true if there is a new line, false if not
 */
{
    ULONG i, j;
    char ch;

    if (CurrentOffset >= FileBufferSize)
        return false;

    i = 0;
    while (((j = CurrentOffset + i) < FileBufferSize) && (i < sizeof(Line) - 1) &&
        ((ch = FileBuffer[j]) != 0x0D && (ch = FileBuffer[j]) != 0x0A))
    {
        Line[i] = ch;
        i++;
    }

    Line[i]    = '\0';
    LineLength = i;

    if ((FileBuffer[CurrentOffset + i] == 0x0D) && (FileBuffer[CurrentOffset + i + 1] == 0x0A))
        CurrentOffset++;

    CurrentOffset += i + 1;

    CurrentChar = 0;

    CurrentLine++;

    NextToken();

    return true;
}


void CDFParser::NextToken()
/*
 * FUNCTION: Reads the next token from the current line
 */
{
    ULONG i;
    char ch = ' ';

    if (CurrentChar >= LineLength)
    {
        CurrentToken = TokenEnd;
        return;
    }

    switch (Line[CurrentChar])
    {
        case ' ':
        case 0x09:
            CurrentToken = TokenSpace;
            break;

        case ';':
            CurrentToken = TokenSemi;
            break;

        case '=':
            CurrentToken = TokenEqual;
            break;

        case '.':
            CurrentToken = TokenPeriod;
            break;

        case '\\':
            CurrentToken = TokenBackslash;
            break;

        case '"':
            i = 0;
            while ((CurrentChar + i + 1 < LineLength) &&
                ((ch = Line[CurrentChar + i + 1]) != '"'))
            {
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
                ((ch = Line[CurrentChar + i]) >= '0') && (ch <= '9'))
            {
                CurrentString[i] = ch;
                i++;
            }
            if (i > 0)
            {
                CurrentString[i] = '\0';
                CurrentInteger = atoi(CurrentString);
                CurrentToken = TokenInteger;
                CurrentChar += i;
                return;
            }
            i = 0;
            while (((CurrentChar + i < LineLength) &&
                (((ch = Line[CurrentChar + i]) >= 'a') && (ch <= 'z'))) ||
                ((ch >= 'A') && (ch <= 'Z')) || (ch == '_'))
            {
                CurrentString[i] = ch;
                i++;
            }
            if (i > 0)
            {
                CurrentString[i] = '\0';
                CurrentToken = TokenIdentifier;
                CurrentChar += i;
                return;
            }
            CurrentToken = TokenEnd;
    }
    CurrentChar++;
}

/* EOF */
