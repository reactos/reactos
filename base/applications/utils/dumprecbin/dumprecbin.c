/* 
 *
 *  dumprecbin - dumps a recycle bin database
 *
 *  Copyright (c) 2005 by Thomas Weidenmueller <w3seek@reactos.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * TODO: - Support for Vista recycle bins (read the DeleteInfo NTFS streams, also NT 5.x)
 *       - Support for INFO databases (win95)
 */
#include <windows.h>
#include <winternl.h>
#include <sddl.h>
#include <ntsecapi.h>
#include <stdio.h>
#include <ctype.h>
#include <tchar.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(status) ((LONG)(status) >= 0)
#endif

typedef struct _RECYCLE_BIN
{
    struct _RECYCLE_BIN *Next;
    PSID Sid;
    WCHAR User[255];
    WCHAR Path[MAX_PATH + 1];
} RECYCLE_BIN, *PRECYCLE_BIN;

typedef enum
{
    ivUnknown = 0,
    ivINFO2
} INFO_VERSION, *PINFO_VERSION;

typedef struct _INFO2_HEADER
{
    DWORD Version;
    DWORD Zero1;
    DWORD Zero2;
    DWORD RecordSize;
} INFO2_HEADER, *PINFO2_HEADER;

typedef struct _INFO2_RECORD
{
    DWORD Unknown;
    CHAR AnsiFileName[MAX_PATH];
    DWORD RecordNumber;
    DWORD DriveLetter;
    FILETIME DeletionTime;
    DWORD DeletedPhysicalSize;
    WCHAR FileName[MAX_PATH - 2];
} INFO2_RECORD, *PINFO2_RECORD;

static HANDLE
OpenAndMapInfoDatabase(IN LPTSTR szFileName,
                       OUT PVOID *MappingBasePtr,
                       OUT PLARGE_INTEGER FileSize)
{
    HANDLE hFile, hMapping = INVALID_HANDLE_VALUE;

    hFile = CreateFile(szFileName,
                       FILE_READ_DATA,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
                       NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (GetFileSizeEx(hFile,
                          FileSize) &&
            FileSize->QuadPart >= 0xF)
        {
            hMapping = CreateFileMapping(hFile,
                                         NULL,
                                         PAGE_READONLY,
                                         0,
                                         0,
                                         NULL);
            if (hMapping == NULL ||
                !(*MappingBasePtr = MapViewOfFile(hMapping,
                                                  FILE_MAP_READ,
                                                  0,
                                                  0,
                                                  0)))
            {
                if (hMapping != NULL)
                {
                    CloseHandle(hMapping);
                }
                hMapping = INVALID_HANDLE_VALUE;
            }
        }
        CloseHandle(hFile);
    }
    
    return hMapping;
}

static VOID
UnmapAndCloseDatabase(IN HANDLE hMapping,
                      IN PVOID MappingBasePtr)
{
    UnmapViewOfFile(MappingBasePtr);
    CloseHandle(hMapping);
}

static INFO_VERSION
DetectDatabaseVersion(PVOID Header)
{
    PINFO2_HEADER Info2 = (PINFO2_HEADER)Header;
    INFO_VERSION Version = ivUnknown;

    if (Info2->Version == 5 &&
        Info2->Zero1 == 0 &&
        Info2->Zero2 == 0 &&
        Info2->RecordSize == 0x320)
    {
        Version = ivINFO2;
    }
    
    return Version;
}

static BOOL
IsValidRecycleBin(IN LPTSTR szPath)
{
    TCHAR szFile[MAX_PATH + 1];
    TCHAR szClsId[48];
    INFO_VERSION DbVersion = ivUnknown;
    
    _stprintf(szFile,
              _T("%s\\desktop.ini"),
              szPath);

    /* check if directory contains a valid desktop.ini for the recycle bin */
    if (GetPrivateProfileString(TEXT(".ShellClassInfo"),
                                TEXT("CLSID"),
                                NULL,
                                szClsId,
                                sizeof(szClsId) / sizeof(szClsId[0]),
                                szFile) &&
        !_tcsicmp(_T("{645FF040-5081-101B-9F08-00AA002F954E}"),
                  szClsId))
    {
        HANDLE hDb;
        LARGE_INTEGER FileSize;
        PVOID pDbBase = NULL;

        /* open the database and check the signature */
        _stprintf(szFile,
                  _T("%s\\INFO2"),
                  szPath);
        hDb = OpenAndMapInfoDatabase(szFile,
                                     &pDbBase,
                                     &FileSize);
        if (hDb != INVALID_HANDLE_VALUE)
        {
            DbVersion = DetectDatabaseVersion(pDbBase);
            UnmapAndCloseDatabase(hDb,
                                  pDbBase);
        }
    }

    return DbVersion != ivUnknown;
}

static BOOL
OpenLocalLSAPolicyHandle(IN ACCESS_MASK DesiredAccess,
                         OUT PLSA_HANDLE PolicyHandle)
{
    LSA_OBJECT_ATTRIBUTES LsaObjectAttributes = {0};
    NTSTATUS Status;

    Status = LsaOpenPolicy(NULL,
                           &LsaObjectAttributes,
                           DesiredAccess,
                           PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(LsaNtStatusToWinError(Status));
        return FALSE;
    }

    return TRUE;
}

static BOOL
ConvertSIDToAccountName(IN PSID Sid,
                        OUT LPWSTR User)
{
    DWORD AccountNameLen = 0;
    DWORD DomainNameLen = 0;
    SID_NAME_USE NameUse;
    DWORD Error = ERROR_SUCCESS;
    LPWSTR AccountName, DomainName;
    BOOL Ret = FALSE;

    if (!LookupAccountSidW(NULL,
                           Sid,
                           NULL,
                           &AccountNameLen,
                           NULL,
                           &DomainNameLen,
                           &NameUse))
    {
        Error = GetLastError();
        if (Error == ERROR_NONE_MAPPED ||
            Error != ERROR_INSUFFICIENT_BUFFER)
        {
            /* some unexpected error occured! */
            goto ConvertSID;
        }
    }
    
    AccountName = (LPWSTR)HeapAlloc(GetProcessHeap(),
                                    0,
                                    (AccountNameLen + DomainNameLen) * sizeof(WCHAR));
    if (AccountName != NULL)
    {
        LSA_HANDLE PolicyHandle;
        DomainName = AccountName + AccountNameLen;
        
        if (!LookupAccountSidW(NULL,
                               Sid,
                               AccountName,
                               &AccountNameLen,
                               DomainName,
                               &DomainNameLen,
                               &NameUse))
        {
            goto BailFreeAccountName;
        }
        
        wcscpy(User,
               AccountName);
        Ret = TRUE;
        
        if (OpenLocalLSAPolicyHandle(POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,
                                     &PolicyHandle))
        {
            PLSA_REFERENCED_DOMAIN_LIST ReferencedDomain;
            PLSA_TRANSLATED_NAME Names;
            PLSA_TRUST_INFORMATION Domain;
            PLSA_UNICODE_STRING DomainName;
            PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo = NULL;
            NTSTATUS Status;
            
            Status = LsaLookupSids(PolicyHandle,
                                   1,
                                   &Sid,
                                   &ReferencedDomain,
                                   &Names);
            if (NT_SUCCESS(Status))
            {
                if (ReferencedDomain != NULL &&
                    Names->DomainIndex >= 0)
                {
                    Domain = &ReferencedDomain->Domains[Names->DomainIndex];
                    DomainName = &Domain->Name;
                }
                else
                {
                    Domain = NULL;
                    DomainName = NULL;
                }
                
                switch (Names->Use)
                {
                    case SidTypeAlias:
                        if (Domain != NULL)
                        {
                            /* query the domain name for BUILTIN accounts */
                            Status = LsaQueryInformationPolicy(PolicyHandle,
                                                               PolicyAccountDomainInformation,
                                                               (PVOID*)&PolicyAccountDomainInfo);
                            if (NT_SUCCESS(Status))
                            {
                                DomainName = &PolicyAccountDomainInfo->DomainName;
                            }
                        }
                        /* fall through */

                    case SidTypeUser:
                    {
                        if (Domain != NULL)
                        {
                            WCHAR *s;

                            /* NOTE: LSA_UNICODE_STRINGs are not always NULL-terminated! */

                            wcscpy(User,
                                   AccountName);
                            wcscat(User,
                                   L" (");
                            s = User + wcslen(User);
                            CopyMemory(s,
                                       DomainName->Buffer,
                                       DomainName->Length);
                            s += DomainName->Length / sizeof(WCHAR);
                            *(s++) = L'\\';
                            CopyMemory(s,
                                       Names->Name.Buffer,
                                       Names->Name.Length);
                            s += Names->Name.Length / sizeof(WCHAR);
                            *(s++) = L')';
                            *s = L'\0';
                        }
                        break;
                    }

                    case SidTypeWellKnownGroup:
                    {
                        break;
                    }

                    default:
                    {
                        _ftprintf(stderr,
                                  _T("Unhandled SID type: 0x%x\n"),
                                  Names->Use);
                        break;
                    }
                }

                if (PolicyAccountDomainInfo != NULL)
                {
                    LsaFreeMemory(PolicyAccountDomainInfo);
                }

                LsaFreeMemory(ReferencedDomain);
                LsaFreeMemory(Names);
            }

            LsaClose(PolicyHandle);
            
            if (!NT_SUCCESS(Status))
            {
                Ret = FALSE;
                goto BailFreeAccountName;
            }
        }
        else
        {
BailFreeAccountName:
            HeapFree(GetProcessHeap(),
                     0,
                     AccountName);
            goto ConvertSID;
        }
    }
    
ConvertSID:
    if (!Ret)
    {
        LPWSTR StrSid;
        Ret = ConvertSidToStringSidW(Sid,
                                     &StrSid);
        if (Ret)
        {
            wcscpy(User,
                   StrSid);
            LocalFree((HLOCAL)StrSid);
        }
    }

    return Ret;
}

static VOID
FreeRecycleBinsList(IN OUT PRECYCLE_BIN *RecycleBinsListHead)
{
    PRECYCLE_BIN CurrentBin, NextBin;

    CurrentBin = *RecycleBinsListHead;
    while (CurrentBin != NULL)
    {
        NextBin = CurrentBin->Next;
        LocalFree((HLOCAL)CurrentBin->Sid);
        HeapFree(GetProcessHeap(),
                 0,
                 CurrentBin);
        CurrentBin = NextBin;
    }

    *RecycleBinsListHead = NULL;
}

static BOOL
LocateRecycleBins(IN LPWSTR szDrive,
                  OUT PRECYCLE_BIN *RecycleBinsListHead)
{
    TCHAR szRecBinPath[MAX_PATH + 1];
    HANDLE FindResult;
    WIN32_FIND_DATA FindData;
    PRECYCLE_BIN NewBin;
    BOOL Ret = FALSE;
    
    FreeRecycleBinsList(RecycleBinsListHead);
    
    /*
     * search for recycle bins on volumes that support file security (NTFS)
     */
    _stprintf(szRecBinPath,
              _T("%lS\\RECYCLER\\*"),
              szDrive);
    FindResult = FindFirstFile(szRecBinPath,
                               &FindData);
    if (FindResult != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
                _tcscmp(FindData.cFileName,
                        _T("..")) &&
                _tcscmp(FindData.cFileName,
                        _T(".")))
            {
                PSID Sid;
                
                if (ConvertStringSidToSid(FindData.cFileName,
                                          &Sid))
                {
                    _stprintf(szRecBinPath,
                              _T("%s\\RECYCLER\\%s"),
                              szDrive,
                              FindData.cFileName);
                    if (IsValidRecycleBin(szRecBinPath))
                    {
                        NewBin = (PRECYCLE_BIN)HeapAlloc(GetProcessHeap(),
                                                         HEAP_ZERO_MEMORY,
                                                         sizeof(RECYCLE_BIN));
                        if (NewBin != NULL)
                        {
                            _tcscpy(NewBin->Path,
                                    szRecBinPath);

                            /* convert the SID to an account name */
                            ConvertSIDToAccountName(Sid,
                                                    NewBin->User);

                            /* append the recycle bin */
                            *RecycleBinsListHead = NewBin;
                            RecycleBinsListHead = &NewBin->Next;
                            
                            Ret = TRUE;
                        }
                        else
                            goto ContinueFreeSid;
                    }
                    else
                    {
ContinueFreeSid:
                        LocalFree((HLOCAL)Sid);
                    }
                }
            }
        } while (FindNextFile(FindResult,
                              &FindData));

        FindClose(FindResult);
    }
    
    /*
     * search for recycle bins on volumes that don't support file security (FAT)
     */
    _stprintf(szRecBinPath,
              _T("%s\\Recycled"),
              szDrive);
    FindResult = FindFirstFile(szRecBinPath,
                               &FindData);
    if (FindResult != INVALID_HANDLE_VALUE)
    {
        if (IsValidRecycleBin(szRecBinPath))
        {
            SID_IDENTIFIER_AUTHORITY WorldSia = {SECURITY_WORLD_SID_AUTHORITY};
            PSID EveryoneSid;
            
            /* create an Everyone SID */
            if (AllocateAndInitializeSid(&WorldSia,
                                         1,
                                         SECURITY_WORLD_RID,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &EveryoneSid))
            {
                NewBin = (PRECYCLE_BIN)HeapAlloc(GetProcessHeap(),
                                                 HEAP_ZERO_MEMORY,
                                                 sizeof(RECYCLE_BIN));
                if (NewBin != NULL)
                {
                    _tcscpy(NewBin->Path,
                            szRecBinPath);

                    /* convert the SID to an account name */
                    ConvertSIDToAccountName(EveryoneSid,
                                            NewBin->User);

                    /* append the recycle bin */
                    *RecycleBinsListHead = NewBin;
                    RecycleBinsListHead = &NewBin->Next;

                    Ret = TRUE;
                }
                else
                    FreeSid(EveryoneSid);
            }
        }
        FindClose(FindResult);
    }

    return Ret;
}

static VOID
DiskFileNameFromRecord(OUT LPTSTR szShortFileName,
                       IN DWORD RecordNumber,
                       IN WCHAR cDriveLetter,
                       IN LPWSTR szFileName)
{
    LPWSTR FileExt;
    
    FileExt = wcsrchr(szFileName,
                      L'.');
    if (FileExt != NULL)
    {
        _stprintf(szShortFileName,
                  _T("D%lC%d%lS"),
                  cDriveLetter,
                  RecordNumber,
                  FileExt);
    }
    else
    {
        _stprintf(szShortFileName,
                  _T("D%lC%d"),
                  cDriveLetter,
                  RecordNumber);
    }
}

static BOOL
DumpRecycleBin(IN PRECYCLE_BIN RecycleBin)
{
    WCHAR szFile[MAX_PATH + 1];
    HANDLE hDb;
    LARGE_INTEGER FileSize;
    PVOID pDbBase = NULL;
    INFO_VERSION Version = ivUnknown;

    _tprintf(_T("Dumping recycle bin of \"%lS\":\n"),
             RecycleBin->User);
    _tprintf(_T("Directory: %lS\n\n"),
             RecycleBin->Path);

    _stprintf(szFile,
              _T("%s\\INFO2"),
              RecycleBin->Path);
    hDb = OpenAndMapInfoDatabase(szFile,
                                 &pDbBase,
                                 &FileSize);
    if (hDb != INVALID_HANDLE_VALUE)
    {
        Version = DetectDatabaseVersion(pDbBase);

        /* dump the INFO2 database */
        switch (Version)
        {
            case ivINFO2:
            {
                DWORD nRecords;
                PINFO2_HEADER Info2Header = (PINFO2_HEADER)pDbBase;
                PINFO2_RECORD Info2 = (PINFO2_RECORD)(Info2Header + 1);
                int i = 0;

                nRecords = (FileSize.QuadPart - sizeof(INFO2_HEADER)) / Info2Header->RecordSize;
                
                while (nRecords != 0)
                {
                    /* if the first character of the AnsiFileName is zero, the record
                       is considered deleted */
                    if (Info2->AnsiFileName[0] != '\0')
                    {
                        _tprintf(_T(" [%d] Record: #%d \"%lS\"\n"),
                                 ++i,
                                 Info2->RecordNumber,
                                 Info2->FileName);

                        DiskFileNameFromRecord(szFile,
                                               Info2->RecordNumber,
                                               (WCHAR)Info2->DriveLetter + L'a',
                                               Info2->FileName);
                        _tprintf(_T("      Name on disk: \"%s\"\n"),
                                 szFile);
                        _tprintf(_T("      Deleted size on disk: %d KB\n"),
                                 Info2->DeletedPhysicalSize / 1024);
                    }
                    nRecords--;
                    Info2++;
                }
                
                break;
            }

            default:
                break;
        }

        UnmapAndCloseDatabase(hDb,
                              pDbBase);
    }

    return FALSE;
}

static BOOL
SelectRecycleBin(IN LPWSTR szDrive)
{
    BOOL Ret;
    PRECYCLE_BIN RecycleBinsList = NULL;
    
    Ret = LocateRecycleBins(szDrive,
                            &RecycleBinsList);
    if (Ret)
    {
        if (RecycleBinsList->Next != NULL)
        {
            PRECYCLE_BIN CurrentBin = RecycleBinsList;
            int n = 0, i = 0;

            /* if there are multiple recycle bins ask the user which one to dump */
            _tprintf(_T("There are several recycle bins on this drive. Select one:\n"));
            
            while (CurrentBin != NULL)
            {
                _tprintf(_T("  [%d] %lS\n"),
                         ++i,
                         CurrentBin->User);
                CurrentBin = CurrentBin->Next;
                n++;
            }

            _tprintf(_T("Enter the number: "));
DisplayPrompt:
            _tscanf(_T("%d"),
                    &i);
            if (i > n || i < 1)
            {
                _tprintf(_T("Please enter a number between 1 and %d: "),
                         n);
                goto DisplayPrompt;
            }
            
            /* walk to the selected recycle bin */
            CurrentBin = RecycleBinsList;
            while (CurrentBin != NULL && i != 1)
            {
                CurrentBin = CurrentBin->Next;
                i--;
            }
            
            /* dump it */
            Ret = DumpRecycleBin(CurrentBin);
        }
        else
        {
            /* dump the first (and only) recycle bin */
            Ret = DumpRecycleBin(RecycleBinsList);
        }
    }
    else
    {
        _ftprintf(stderr,
                  _T("No recycle bins on this volume!\n"));
    }
    
    FreeRecycleBinsList(&RecycleBinsList);

    return Ret;
}

static VOID
PrintHelp(VOID)
{
    _ftprintf(stderr,
              _T("Usage: dumprecbin C:\n"));
}

int
main(int argc,
     char *argv[])
{
    if (argc != 2 ||
        strlen(argv[1]) != 2 || argv[1][1] != ':' ||
        toupper(argv[1][0]) < 'A' || toupper(argv[1][0]) > 'Z')
    {
        PrintHelp();
        return 1;
    }
    else
    {
        WCHAR szDrive[3];
        _stprintf(szDrive,
                  _T("%lC:"),
                  argv[1][0]);

        if (!SelectRecycleBin(szDrive))
            return 1;
        else
            return 0;
    }
}

