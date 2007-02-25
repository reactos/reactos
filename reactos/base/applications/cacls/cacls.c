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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

static GENERIC_MAPPING FileGenericMapping =
{
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS
};


static INT
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


static INT
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


static VOID
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


static VOID
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


static DWORD
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


static BOOL
PrintFileDacl(IN LPTSTR FilePath,
              IN LPTSTR FileName)
{
    SIZE_T Indent;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    DWORD SDSize = 0;
    TCHAR FullFileName[MAX_PATH + 1];
    BOOL Error = FALSE, Ret = FALSE;

    Indent = _tcslen(FilePath) + _tcslen(FileName);
    if (Indent++ > MAX_PATH - 1)
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


int __cdecl _tmain(int argc, const TCHAR *argv[])
{
    if (argc < 2)
    {
        PrintHelp();
        return 1;
    }
    else
    {
        TCHAR FullPath[MAX_PATH + 1];
        TCHAR *FilePart = NULL;
        WIN32_FIND_DATA FindData;
        HANDLE hFind;
        DWORD LastError;
        BOOL ContinueAccessDenied = FALSE;

        if (argc > 2)
        {
            /* FIXME - parse arguments */
        }

        /* get the full path of where we're searching in */
        if (GetFullPathName(argv[1],
                            sizeof(FullPath) / sizeof(FullPath[0]),
                            FullPath,
                            &FilePart) != 0)
        {
            if (FilePart != NULL)
                *FilePart = _T('\0');
        }
        else
            goto Error;

        /* find the file(s) */
        hFind = FindFirstFile(argv[1],
                              &FindData);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                    (_tcscmp(FindData.cFileName,
                             _T(".")) &&
                     _tcscmp(FindData.cFileName,
                             _T(".."))))
                {
                    if (argc > 2)
                    {
                        /* FIXME - edit or replace the descriptor */
                    }
                    else
                    {
                        if (!PrintFileDacl(FullPath,
                                           FindData.cFileName))
                        {
                            LastError = GetLastError();

                            if (LastError == ERROR_ACCESS_DENIED &&
                                ContinueAccessDenied)
                            {
                                PrintErrorMessage(LastError);
                            }
                            else
                                break;
                        }
                        else
                            _tprintf(_T("\n"));
                    }
                }
            } while (FindNextFile(hFind,
                                  &FindData));

            FindClose(hFind);

            if (GetLastError() != ERROR_NO_MORE_FILES)
            {
                goto Error;
            }
        }
        else
        {
Error:
            PrintErrorMessage(GetLastError());
            return 1;
        }
    }

    return 0;
}
