/*
 * ReactOS Control ACLs Program
 * Copyright (C) 2006 Thomas Weidenmueller
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

/* command line options */
BOOL OptionT = FALSE, OptionE = FALSE, OptionC = FALSE;
BOOL OptionG = FALSE, OptionR = FALSE, OptionP = FALSE, OptionD = FALSE;
LPCTSTR GUser, GPerm, RUser, PUser, PPerm, DUser;

static GENERIC_MAPPING FileGenericMapping =
{
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS
};


static
INT
LengthOfStrResource(IN HINSTANCE hInst,
                    IN UINT uID)
{
    HRSRC hrSrc;
    HGLOBAL hRes;
    LPWSTR lpName, lpStr;

    if (hInst == NULL)
    {
        hInst = GetModuleHandle(NULL);
    }

    /* There are always blocks of 16 strings */
    lpName = (LPWSTR)MAKEINTRESOURCE((uID >> 4) + 1);

    /* Find the string table block */
    hrSrc = FindResourceW(hInst, lpName, (LPWSTR)RT_STRING);
    if (hrSrc)
    {
        hRes = LoadResource(hInst, hrSrc);
        if (hRes)
        {
            lpStr = LockResource(hRes);
            if (lpStr)
            {
                UINT x;

                /* Find the string we're looking for */
                uID &= 0xF; /* position in the block, same as % 16 */
                for (x = 0; x < uID; x++)
                {
                    lpStr += (*lpStr) + 1;
                }

                /* Found the string */
                return (int)(*lpStr);
            }
        }
    }
    return -1;
}


static
INT
AllocAndLoadString(OUT LPTSTR *lpTarget,
                   IN HINSTANCE hInst,
                   IN UINT uID)
{
    INT ln;

    ln = LengthOfStrResource(hInst,
                             uID);
    if (ln++ > 0)
    {
        (*lpTarget) = (LPTSTR)HeapAlloc(GetProcessHeap(),
                                        0,
                                        ln * sizeof(TCHAR));
        if ((*lpTarget) != NULL)
        {
            INT Ret;
            Ret = LoadString(hInst,
                             uID,
                             *lpTarget,
                             ln);
            if (!Ret)
            {
                HeapFree(GetProcessHeap(),
                         0,
                         *lpTarget);
            }
            return Ret;
        }
    }
    return 0;
}


static
VOID
PrintHelp(VOID)
{
    LPTSTR szHelp;

    if (AllocAndLoadString(&szHelp,
                           NULL,
                           IDS_HELP) != 0)
    {
        _tprintf(_T("%s"),
                 szHelp);

        HeapFree(GetProcessHeap(),
                 0,
                 szHelp);
    }
}


static
VOID
PrintErrorMessage(IN DWORD dwError)
{
    LPTSTR szError;

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_IGNORE_INSERTS |
                          FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      dwError,
                      MAKELANGID(LANG_NEUTRAL,
                                 SUBLANG_DEFAULT),
                      (LPTSTR)&szError,
                      0,
                      NULL) != 0)
    {
        _tprintf(_T("%s"),
                 szError);
        LocalFree((HLOCAL)szError);
    }
}


static
DWORD
LoadAndPrintString(IN HINSTANCE hInst,
                   IN UINT uID)
{
    TCHAR szTemp[255];
    DWORD Len;

    Len = (DWORD)LoadString(hInst,
                            uID,
                            szTemp,
                            sizeof(szTemp) / sizeof(szTemp[0]));

    if (Len != 0)
    {
        _tprintf(_T("%s"),
                 szTemp);
    }

    return Len;
}


static
BOOL
PrintFileDacl(IN LPTSTR FilePath,
              IN LPTSTR FileName)
{
    SIZE_T Length;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    DWORD SDSize = 0;
    TCHAR FullFileName[MAX_PATH + 1];
    BOOL Error = FALSE, Ret = FALSE;

    Length = _tcslen(FilePath) + _tcslen(FileName);
    if (Length > MAX_PATH)
    {
        /* file name too long */
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    _tcscpy(FullFileName,
            FilePath);
    _tcscat(FullFileName,
            FileName);

    /* find out how much memory we need */
    if (!GetFileSecurity(FullFileName,
                         DACL_SECURITY_INFORMATION,
                         NULL,
                         0,
                         &SDSize) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        return FALSE;
    }

    SecurityDescriptor = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(),
                                                         0,
                                                         SDSize);
    if (SecurityDescriptor != NULL)
    {
        if (GetFileSecurity(FullFileName,
                            DACL_SECURITY_INFORMATION,
                            SecurityDescriptor,
                            SDSize,
                            &SDSize))
        {
            PACL Dacl;
            BOOL DaclPresent;
            BOOL DaclDefaulted;

            if (GetSecurityDescriptorDacl(SecurityDescriptor,
                                          &DaclPresent,
                                          &Dacl,
                                          &DaclDefaulted))
            {
                if (DaclPresent)
                {
                    PACCESS_ALLOWED_ACE Ace;
                    DWORD AceIndex = 0;

                    /* dump the ACL */
                    while (GetAce(Dacl,
                                  AceIndex,
                                  (PVOID*)&Ace))
                    {
                        SID_NAME_USE Use;
                        DWORD NameSize = 0;
                        DWORD DomainSize = 0;
                        LPTSTR Name = NULL;
                        LPTSTR Domain = NULL;
                        LPTSTR SidString = NULL;
                        DWORD IndentAccess;
                        DWORD AccessMask = Ace->Mask;
                        PSID Sid = (PSID)&Ace->SidStart;

                        /* attempt to translate the SID into a readable string */
                        if (!LookupAccountSid(NULL,
                                              Sid,
                                              Name,
                                              &NameSize,
                                              Domain,
                                              &DomainSize,
                                              &Use))
                        {
                            if (GetLastError() == ERROR_NONE_MAPPED || NameSize == 0)
                            {
                                goto BuildSidString;
                            }
                            else
                            {
                                if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                                {
                                    Error = TRUE;
                                    break;
                                }

                                Name = (LPTSTR)HeapAlloc(GetProcessHeap(),
                                                         0,
                                                         (NameSize + DomainSize) * sizeof(TCHAR));
                                if (Name == NULL)
                                {
                                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                                    Error = TRUE;
                                    break;
                                }

                                Domain = Name + NameSize;
                                Name[0] = _T('\0');
                                if (DomainSize != 0)
                                    Domain[0] = _T('\0');
                                if (!LookupAccountSid(NULL,
                                                      Sid,
                                                      Name,
                                                      &NameSize,
                                                      Domain,
                                                      &DomainSize,
                                                      &Use))
                                {
                                    HeapFree(GetProcessHeap(),
                                             0,
                                             Name);
                                    Name = NULL;
                                    goto BuildSidString;
                                }
                            }
                        }
                        else
                        {
BuildSidString:
                            if (!ConvertSidToStringSid(Sid,
                                                       &SidString))
                            {
                                Error = TRUE;
                                break;
                            }
                        }

                        /* print the file name or space */
                        _tprintf(_T("%s "),
                                 FullFileName);

                        /* attempt to map the SID to a user name */
                        if (AceIndex == 0)
                        {
                            DWORD i = 0;

                            /* overwrite the full file name with spaces so we
                               only print the file name once */
                            while (FullFileName[i] != _T('\0'))
                                FullFileName[i++] = _T(' ');
                        }

                        /* print the domain and/or user if possible, or the SID string */
                        if (Name != NULL && Domain[0] != _T('\0'))
                        {
                            _tprintf(_T("%s\\%s:"),
                                     Domain,
                                     Name);
                            IndentAccess = (DWORD)_tcslen(Domain) + _tcslen(Name);
                        }
                        else
                        {
                            LPTSTR DisplayString = (Name != NULL ? Name : SidString);

                            _tprintf(_T("%s:"),
                                     DisplayString);
                            IndentAccess = (DWORD)_tcslen(DisplayString);
                        }

                        /* print the ACE Flags */
                        if (Ace->Header.AceFlags & CONTAINER_INHERIT_ACE)
                        {
                            IndentAccess += LoadAndPrintString(NULL,
                                                               IDS_ABBR_CI);
                        }
                        if (Ace->Header.AceFlags & OBJECT_INHERIT_ACE)
                        {
                            IndentAccess += LoadAndPrintString(NULL,
                                                               IDS_ABBR_OI);
                        }
                        if (Ace->Header.AceFlags & INHERIT_ONLY_ACE)
                        {
                            IndentAccess += LoadAndPrintString(NULL,
                                                               IDS_ABBR_IO);
                        }

                        IndentAccess += 2;

                        /* print the access rights */
                        MapGenericMask(&AccessMask,
                                       &FileGenericMapping);
                        if (Ace->Header.AceType & ACCESS_DENIED_ACE_TYPE)
                        {
                            if (AccessMask == FILE_ALL_ACCESS)
                            {
                                LoadAndPrintString(NULL,
                                                   IDS_ABBR_NONE);
                            }
                            else
                            {
                                LoadAndPrintString(NULL,
                                                   IDS_DENY);
                                goto PrintSpecialAccess;
                            }
                        }
                        else
                        {
                            if (AccessMask == FILE_ALL_ACCESS)
                            {
                                LoadAndPrintString(NULL,
                                                   IDS_ABBR_FULL);
                            }
                            else if (!(Ace->Mask & (GENERIC_READ | GENERIC_EXECUTE)) &&
                                     AccessMask == (FILE_GENERIC_READ | FILE_EXECUTE))
                            {
                                LoadAndPrintString(NULL,
                                                   IDS_ABBR_READ);
                            }
                            else if (AccessMask == (FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_EXECUTE | DELETE))
                            {
                                LoadAndPrintString(NULL,
                                                   IDS_ABBR_CHANGE);
                            }
                            else if (AccessMask == FILE_GENERIC_WRITE)
                            {
                                LoadAndPrintString(NULL,
                                                   IDS_ABBR_WRITE);
                            }
                            else
                            {
                                DWORD x, x2;
                                static const struct
                                {
                                    DWORD Access;
                                    UINT uID;
                                }
                                AccessRights[] =
                                {
                                    {FILE_WRITE_ATTRIBUTES, IDS_FILE_WRITE_ATTRIBUTES},
                                    {FILE_READ_ATTRIBUTES, IDS_FILE_READ_ATTRIBUTES},
                                    {FILE_DELETE_CHILD, IDS_FILE_DELETE_CHILD},
                                    {FILE_EXECUTE, IDS_FILE_EXECUTE},
                                    {FILE_WRITE_EA, IDS_FILE_WRITE_EA},
                                    {FILE_READ_EA, IDS_FILE_READ_EA},
                                    {FILE_APPEND_DATA, IDS_FILE_APPEND_DATA},
                                    {FILE_WRITE_DATA, IDS_FILE_WRITE_DATA},
                                    {FILE_READ_DATA, IDS_FILE_READ_DATA},
                                    {FILE_GENERIC_EXECUTE, IDS_FILE_GENERIC_EXECUTE},
                                    {FILE_GENERIC_WRITE, IDS_FILE_GENERIC_WRITE},
                                    {FILE_GENERIC_READ, IDS_FILE_GENERIC_READ},
                                    {GENERIC_ALL, IDS_GENERIC_ALL},
                                    {GENERIC_EXECUTE, IDS_GENERIC_EXECUTE},
                                    {GENERIC_WRITE, IDS_GENERIC_WRITE},
                                    {GENERIC_READ, IDS_GENERIC_READ},
                                    {MAXIMUM_ALLOWED, IDS_MAXIMUM_ALLOWED},
                                    {ACCESS_SYSTEM_SECURITY, IDS_ACCESS_SYSTEM_SECURITY},
                                    {SPECIFIC_RIGHTS_ALL, IDS_SPECIFIC_RIGHTS_ALL},
                                    {STANDARD_RIGHTS_REQUIRED, IDS_STANDARD_RIGHTS_REQUIRED},
                                    {SYNCHRONIZE, IDS_SYNCHRONIZE},
                                    {WRITE_OWNER, IDS_WRITE_OWNER},
                                    {WRITE_DAC, IDS_WRITE_DAC},
                                    {READ_CONTROL, IDS_READ_CONTROL},
                                    {DELETE, IDS_DELETE},
                                    {STANDARD_RIGHTS_ALL, IDS_STANDARD_RIGHTS_ALL},
                                };

                                LoadAndPrintString(NULL,
                                                   IDS_ALLOW);

PrintSpecialAccess:
                                LoadAndPrintString(NULL,
                                                   IDS_SPECIAL_ACCESS);

                                /* print the special access rights */
                                x = sizeof(AccessRights) / sizeof(AccessRights[0]);
                                while (x-- != 0)
                                {
                                    if ((Ace->Mask & AccessRights[x].Access) == AccessRights[x].Access)
                                    {
                                        _tprintf(_T("\n%s "),
                                                 FullFileName);
                                        for (x2 = 0;
                                             x2 < IndentAccess;
                                             x2++)
                                        {
                                            _tprintf(_T(" "));
                                        }

                                        LoadAndPrintString(NULL,
                                                           AccessRights[x].uID);
                                    }
                                }

                                _tprintf(_T("\n"));
                            }
                        }

                        _tprintf(_T("\n"));

                        /* free up all resources */
                        if (Name != NULL)
                        {
                            HeapFree(GetProcessHeap(),
                                     0,
                                     Name);
                        }

                        if (SidString != NULL)
                        {
                            LocalFree((HLOCAL)SidString);
                        }

                        AceIndex++;
                    }

                    if (!Error)
                        Ret = TRUE;
                }
                else
                {
                    SetLastError(ERROR_NO_SECURITY_ON_OBJECT);
                }
            }
        }

        HeapFree(GetProcessHeap(),
                 0,
                 SecurityDescriptor);
    }
    else
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return Ret;
}

/* add a backslash at end to a path string if necessary */
static VOID
AddBackslash(LPTSTR FilePath)
{
    INT len = lstrlen(FilePath);
    LPTSTR pch = CharPrev(FilePath, FilePath + len);
    if (*pch != _T('\\'))
        lstrcat(pch, _T("\\"));
}

static BOOL
GetPathOfFile(LPTSTR FilePath, LPCTSTR pszFiles)
{
    TCHAR FullPath[MAX_PATH];
    LPTSTR pch;
    DWORD attrs;

    lstrcpyn(FilePath, pszFiles, MAX_PATH);
    pch = _tcsrchr(FilePath, _T('\\'));
    if (pch != NULL)
    {
        *pch = 0;
        if (!GetFullPathName(FilePath, MAX_PATH, FullPath, NULL))
        {
            PrintErrorMessage(GetLastError());
            return FALSE;
        }
        lstrcpyn(FilePath, FullPath, MAX_PATH);

        attrs = GetFileAttributes(FilePath);
        if (attrs == 0xFFFFFFFF || !(attrs & FILE_ATTRIBUTE_DIRECTORY))
        {
            PrintErrorMessage(ERROR_DIRECTORY);
            return FALSE;
        }
    }
    else
        GetCurrentDirectory(MAX_PATH, FilePath);

    AddBackslash(FilePath);
    return TRUE;
}

static BOOL
PrintDaclsOfFiles(LPCTSTR pszFiles)
{
    TCHAR FilePath[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    DWORD LastError;

    /*
     * get the file path
     */
    if (!GetPathOfFile(FilePath, pszFiles))
        return FALSE;

    /*
     * search for the files
     */
    hFind = FindFirstFile(pszFiles, &FindData);
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;

    do
    {
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        if (!PrintFileDacl(FilePath, FindData.cFileName))
        {
            LastError = GetLastError();
            if (LastError == ERROR_ACCESS_DENIED)
            {
                PrintErrorMessage(LastError);
                if (!OptionC)
                {
                    FindClose(hFind);
                    return FALSE;
                }
            }
            else
                break;
        }
        else
            _tprintf(_T("\n"));
    } while(FindNextFile(hFind, &FindData));
    LastError = GetLastError();
    FindClose(hFind);

    if (LastError != ERROR_NO_MORE_FILES)
    {
        PrintErrorMessage(LastError);
        return FALSE;
    }

    return TRUE;
}

static BOOL
GrantUserAccessRights(LPCTSTR FilePath, LPCTSTR File, LPCTSTR User, TCHAR Perm)
{
    /* TODO & FIXME */
    switch(Perm)
    {
        case _T('R'): // Read
            break;
        case _T('W'): // Write
            break;
        case _T('C'): // Change (write)
            break;
        case _T('F'): // Full control
            break;
        default:
            break;
    }
    return FALSE;
}

static BOOL
ReplaceUserAccessRights(
    LPCTSTR FilePath,
    LPCTSTR File,
    LPCTSTR User,
    TCHAR Perm)
{
    /* TODO & FIXME */
    switch(Perm)
    {
        case _T('N'): // None
            break;
        case _T('R'): // Read
            break;
        case _T('W'): // Write
            break;
        case _T('C'): // Change (write)
            break;
        case _T('F'): // Full control
            break;
        default:
            break;
    }
    return FALSE;
}

static BOOL
EditUserAccessRights(
    LPCTSTR FilePath,
    LPCTSTR File,
    LPCTSTR User,
    TCHAR Perm)
{
    /* TODO & FIXME */
    switch(Perm)
    {
        case _T('N'): // None
            break;
        case _T('R'): // Read
            break;
        case _T('W'): // Write
            break;
        case _T('C'): // Change (write)
            break;
        case _T('F'): // Full control
            break;
        default:
            break;
    }
    return FALSE;
}

static BOOL
DenyUserAccess(LPCTSTR FilePath, LPCTSTR File, LPCTSTR User)
{
    /* TODO & FIXME */
    return FALSE;
}

static BOOL
RevokeUserAccessRights(LPCTSTR FilePath, LPCTSTR File, LPCTSTR User)
{
    /* TODO & FIXME */
    return FALSE;
}

static BOOL
ChangeFileACL(LPCTSTR FilePath, LPCTSTR File)
{
    if (OptionG)
    {
        /* Grant specified user access rights. */
        GrantUserAccessRights(FilePath, File, GUser, *GPerm);
    }

    if (OptionP)
    {
        if (!OptionE)
        {
            /* Replace specified user's access rights. */
            ReplaceUserAccessRights(FilePath, File, PUser, *PPerm);
        }
        else
        {
            /* Edit ACL instead of replacing it. */
            EditUserAccessRights(FilePath, File, PUser, *PPerm);
        }
    }

    if (OptionD)
    {
        /* Deny specified user access. */
        DenyUserAccess(FilePath, File, DUser);
    }

    if (OptionR)
    {
        /* Revoke specified user's access rights. */
        RevokeUserAccessRights(FilePath, File, RUser);
    }

    return TRUE;
}

static BOOL
ChangeACLsOfFiles(LPCTSTR pszFiles)
{
    TCHAR FilePath[MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATA FindData;
    DWORD LastError;

    /*
     * get the file path
     */
    if (!GetPathOfFile(FilePath, pszFiles))
        return FALSE;

    /*
     * search for files in current directory
     */
    hFind = FindFirstFile(pszFiles, &FindData);
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;

    do
    {
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        if (!ChangeFileACL(FilePath, FindData.cFileName))
        {
            LastError = GetLastError();
            if (LastError == ERROR_ACCESS_DENIED)
            {
                PrintErrorMessage(LastError);
                if (!OptionC)
                {
                    FindClose(hFind);
                    return FALSE;
                }
            }
            else
                break;
        }
    } while(FindNextFile(hFind, &FindData));

    LastError = GetLastError();
    FindClose(hFind);

    if (LastError != ERROR_NO_MORE_FILES)
    {
        PrintErrorMessage(LastError);
        return FALSE;
    }

    return TRUE;
}

static BOOL
ChangeACLsOfFilesInCurDir(LPCTSTR pszFiles)
{
    HANDLE hFind;
    WIN32_FIND_DATA FindData;
    TCHAR szCurDir[MAX_PATH];
    DWORD LastError;

    /*
     * get the file path (current directory)
     */
    GetCurrentDirectory(MAX_PATH, szCurDir);
    AddBackslash(szCurDir);

    /*
     * search for files in current directory
     */
    hFind = FindFirstFile(pszFiles, &FindData);
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;

    do
    {
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        if (!ChangeFileACL(szCurDir, FindData.cFileName))
        {
            LastError = GetLastError();
            if (LastError == ERROR_ACCESS_DENIED)
            {
                PrintErrorMessage(LastError);
                if (!OptionC)
                {
                    FindClose(hFind);
                    return FALSE;
                }
            }
            else
                break;
        }
    } while(FindNextFile(hFind, &FindData));

    LastError = GetLastError();
    FindClose(hFind);

    if (LastError != ERROR_NO_MORE_FILES)
    {
        PrintErrorMessage(LastError);
        return FALSE;
    }

    /*
     * search for subdirectory in current directory
     */
    hFind = FindFirstFile(_T("*"), &FindData);
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;
    do
    {
        if (_tcscmp(FindData.cFileName, _T(".")) == 0 ||
            _tcscmp(FindData.cFileName, _T("..")) == 0)
            continue;

        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            GetCurrentDirectory(MAX_PATH, szCurDir);
            if (SetCurrentDirectory(FindData.cFileName))
            {
                ChangeACLsOfFilesInCurDir(pszFiles);
                SetCurrentDirectory(szCurDir);
            }
            else
            {
                LastError = GetLastError();
                if (LastError == ERROR_ACCESS_DENIED)
                {
                    PrintErrorMessage(LastError);
                    if (!OptionC)
                    {
                        FindClose(hFind);
                        return FALSE;
                    }
                }
                else
                    break;
            }
        }
    } while(FindNextFile(hFind, &FindData));
    LastError = GetLastError();
    FindClose(hFind);

    if (LastError != ERROR_NO_MORE_FILES)
    {
        PrintErrorMessage(LastError);
        return FALSE;
    }
    return TRUE;
}

int
__cdecl
_tmain(int argc, const TCHAR *argv[])
{
    INT i;
    LPTSTR pch;
    BOOL InvalidParameter = FALSE;

    if (argc <= 1)
    {
        PrintHelp();
        return 0;
    }

    /*
     * parse command line options
     */
    for (i = 2; i < argc; i++)
    {
        if (lstrcmpi(argv[i], _T("/T")) == 0)
        {
            OptionT = TRUE;
        }
        else if (lstrcmpi(argv[i], _T("/E")) == 0)
        {
            OptionE = TRUE;
        }
        else if (lstrcmpi(argv[i], _T("/C")) == 0)
        {
            OptionC = TRUE;
        }
        else if (lstrcmpi(argv[i], _T("/G")) == 0)
        {
            if (i + 1 < argc)
            {
                pch = _tcschr(argv[++i], _T(':'));
                if (pch != NULL)
                {
                    OptionG = TRUE;
                    *pch = 0;
                    GUser = argv[i];
                    GPerm = pch + 1;
                    continue;
                }
            }
            InvalidParameter = TRUE;
            break;
        }
        else if (lstrcmpi(argv[i], _T("/R")) == 0)
        {
            if (i + 1 < argc)
            {
                RUser = argv[++i];
                OptionR = TRUE;
                continue;
            }
            InvalidParameter = TRUE;
            break;
        }
        else if (lstrcmpi(argv[i], _T("/P")) == 0)
        {
            if (i + 1 < argc)
            {
                pch = _tcschr(argv[++i], _T(':'));
                if (pch != NULL)
                {
                    OptionP = TRUE;
                    *pch = 0;
                    PUser = argv[i];
                    PPerm = pch + 1;
                    continue;
                }
            }
            InvalidParameter = TRUE;
            break;
        }
        else if (lstrcmpi(argv[i], _T("/D")) == 0)
        {
            if (i + 1 < argc)
            {
                OptionD = TRUE;
                DUser = argv[++i];
                continue;
            }
            InvalidParameter = TRUE;
            break;
        }
        else
        {
            InvalidParameter = TRUE;
            break;
        }
    }

    if (InvalidParameter)
    {
        PrintErrorMessage(ERROR_INVALID_PARAMETER);
        PrintHelp();
        return 1;
    }

    /* /R is only valid with /E */
    if (OptionR && !OptionE)
    {
        OptionR = FALSE;
    }

    PrintDaclsOfFiles(argv[1]);

    if (OptionT)
    {
        ChangeACLsOfFilesInCurDir(argv[1]);
    }
    else
    {
        ChangeACLsOfFiles(argv[1]);
    }

    return 0;
}
