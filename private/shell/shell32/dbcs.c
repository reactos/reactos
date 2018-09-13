/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dbcs.c

Abstract:

    This module contains the code for console DBCS font dialog

Author:

    kazum Feb-27-1995

Revision History:

--*/

#include "shellprv.h"
#pragma hdrstop

#include "lnkcon.h"

#ifdef DBCS

// This definition shares in windows\inc\wincon.w file
//
#define MACHINE_REGISTRY_CONSOLE_TTFONT (L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont")

#define MACHINE_REGISTRY_CONSOLE_NLS    (L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Console\\Nls")


NTSTATUS
MyRegOpenKey(
    IN HANDLE hKey,
    IN LPWSTR lpSubKey,
    OUT PHANDLE phResult
    )
{
    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      SubKey;

    //
    // Convert the subkey to a counted Unicode string.
    //

    RtlInitUnicodeString( &SubKey, lpSubKey );

    //
    // Initialize the OBJECT_ATTRIBUTES structure and open the key.
    //

    InitializeObjectAttributes(
        &Obja,
        &SubKey,
        OBJ_CASE_INSENSITIVE,
        hKey,
        NULL
        );

    return NtOpenKey(
              phResult,
              KEY_READ,
              &Obja
              );
}

NTSTATUS
MyRegEnumValue(
    IN HANDLE hKey,
    IN DWORD dwIndex,
    OUT DWORD dwValueLength,
    OUT LPWSTR lpValueName,
    OUT DWORD dwDataLength,
    OUT LPBYTE lpData
    )
{
    ULONG BufferLength;
    ULONG ResultLength;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    NTSTATUS Status;

    //
    // Convert the subkey to a counted Unicode string.
    //

    BufferLength = sizeof(KEY_VALUE_FULL_INFORMATION) + dwValueLength + dwDataLength;
    KeyValueInformation = LocalAlloc(LPTR,BufferLength);
    if (KeyValueInformation == NULL)
        return STATUS_NO_MEMORY;

    Status = NtEnumerateValueKey(
                hKey,
                dwIndex,
                KeyValueFullInformation,
                KeyValueInformation,
                BufferLength,
                &ResultLength
                );
    if (NT_SUCCESS(Status)) {
        ASSERT(KeyValueInformation->NameLength <= dwValueLength);
        RtlMoveMemory(lpValueName,
                      KeyValueInformation->Name,
                      KeyValueInformation->NameLength);
        lpValueName[ KeyValueInformation->NameLength >> 1 ] = UNICODE_NULL;


        ASSERT(KeyValueInformation->DataLength <= dwDataLength);
        RtlMoveMemory(lpData,
            (PBYTE)KeyValueInformation + KeyValueInformation->DataOffset,
            KeyValueInformation->DataLength);
        if (KeyValueInformation->Type == REG_SZ ||
            KeyValueInformation->Type == REG_MULTI_SZ
           ) {
            if (KeyValueInformation->DataLength + sizeof(WCHAR) > dwDataLength) {
                KeyValueInformation->DataLength -= sizeof(WCHAR);
            }
            lpData[KeyValueInformation->DataLength++] = 0;
            lpData[KeyValueInformation->DataLength] = 0;
        }
    }
    LocalFree(KeyValueInformation);
    return Status;
}





WORD
ConvertStringToDec(
    LPWSTR lpch,
    LPWSTR *endptr
    )
{
    WCHAR ch;
    WORD val = 0;

    while ( (ch=*lpch) != L'\0')
    {
        if (L'0' <= ch && ch <= L'9')
            val = (val * 10) + (ch - L'0');
        else
            break;

        lpch++;
    }

    if (endptr)
        *endptr = lpch;
    return val;
}

WORD
ConvertStringToHex(
    LPWSTR lpch,
    LPWSTR *endptr
    )
{
    WCHAR ch;
    WORD val = 0;

    while ( (ch=*lpch) != L'\0')
    {
        if (L'0' <= ch && ch <= L'9')
            val = (val << 4) + (ch - L'0');
        else if (L'A' <= ch && ch <= L'F')
            val = (val << 4) + (ch - L'A' + 10);
        else if (L'a' <= ch && ch <= L'f')
            val = (val << 4) + (ch - L'a' + 10);
        else
            break;

        lpch++;
    }

    if (endptr)
        *endptr = lpch;
    return val;
}


NTSTATUS
MakeAltRasterFont(
    LPCONSOLEPROP_DATA pcpd,
    UINT CodePage,
    COORD *AltFontSize,
    BYTE  *AltFontFamily,
    ULONG *AltFontIndex,
    LPTSTR AltFaceName
    )
{
    DWORD i;
    DWORD Find;
    ULONG FontIndex;
    COORD FontSize = pcpd->FontInfo[pcpd->DefaultFontIndex].Size;
    COORD FontDelta;
    BOOL  fDbcsCharSet = IS_ANY_DBCS_CHARSET( CodePageToCharSet( CodePage ) );

    FontIndex = 0;
    Find = (DWORD)-1;
    for (i=0; i < pcpd->NumberOfFonts; i++)
    {
        if (!TM_IS_TT_FONT(pcpd->FontInfo[i].Family) &&
            IS_ANY_DBCS_CHARSET(pcpd->FontInfo[i].tmCharSet) == fDbcsCharSet
           )
        {
            FontDelta.X = (SHORT)abs(FontSize.X - pcpd->FontInfo[i].Size.X);
            FontDelta.Y = (SHORT)abs(FontSize.Y - pcpd->FontInfo[i].Size.Y);
            if (Find > (DWORD)(FontDelta.X + FontDelta.Y))
            {
                Find = (DWORD)(FontDelta.X + FontDelta.Y);
                FontIndex = i;
            }
        }
    }

    *AltFontIndex = FontIndex;
    lstrcpy(AltFaceName, pcpd->FontInfo[*AltFontIndex].FaceName);
    *AltFontSize = pcpd->FontInfo[*AltFontIndex].Size;
    *AltFontFamily = pcpd->FontInfo[*AltFontIndex].Family;

    return STATUS_SUCCESS;
}

NTSTATUS
InitializeDbcsMisc(
    LPCONSOLEPROP_DATA pcpd
    )
{
    HANDLE hkRegistry = NULL;
    NTSTATUS Status;
    WCHAR awchValue[ 512 ];
    WCHAR awchData[ 512 ];
    DWORD dwIndex;
    LPWSTR pwsz;

    pcpd->gTTFontList.Next = NULL;

    Status = MyRegOpenKey(NULL,
                          MACHINE_REGISTRY_CONSOLE_TTFONT,
                          &hkRegistry);
    if (NT_SUCCESS( Status )) {
        LPTTFONTLIST pTTFontList;

        for( dwIndex = 0; ; dwIndex++) {
            Status = MyRegEnumValue(hkRegistry,
                                    dwIndex,
                                    sizeof(awchValue), (LPWSTR)&awchValue,
                                    sizeof(awchData),  (PBYTE)&awchData);
            if (!NT_SUCCESS( Status )) {
                break;
            }

            pTTFontList = LocalAlloc(LPTR, sizeof(TTFONTLIST));
            if (pTTFontList == NULL) {
                break;
            }

            pTTFontList->List.Next = NULL;
            pTTFontList->CodePage = ConvertStringToDec(awchValue, NULL);
            pwsz = awchData;
            if (*pwsz == BOLD_MARK) {
                pTTFontList->fDisableBold = TRUE;
                pwsz++;
            }
            else
                pTTFontList->fDisableBold = FALSE;
#ifdef UNICODE
            lstrcpyW(pTTFontList->FaceName1, pwsz);

            pwsz += lstrlenW(pwsz) + 1;
            if (*pwsz == BOLD_MARK)
            {
                pTTFontList->fDisableBold = TRUE;
                pwsz++;
            }
            lstrcpyW(pTTFontList->FaceName2, pwsz);
#else
            // if we're the ANSI shell, we need to convert FACENAME
            // over to ASCII before saving...
            {
                CHAR szFaceName[LF_FACESIZE];
                SHUnicodeToAnsi(pwsz, szFaceName, ARRAYSIZE(szFaceName));
                lstrcpyA(pTTFontList->FaceName1, szFaceName);

                pwsz += lstrlenW(pwsz) + 1;
                if (*pwsz == BOLD_MARK)
                {
                    pTTFontList->fDisableBold = TRUE;
                    pwsz++;
                }
                SHUnicodeToAnsi(pwsz, szFaceName, ARRAYSIZE(szFaceName));
                lstrcpyA(pTTFontList->FaceName2, szFaceName);
            }
#endif

            PushEntryList(&pcpd->gTTFontList, &(pTTFontList->List));
        }

        NtClose(hkRegistry);
    }

    pcpd->fChangeCodePage = FALSE;
    pcpd->uOEMCP = GetOEMCP();

    return STATUS_SUCCESS;
}

BYTE
CodePageToCharSet(
    UINT CodePage
    )
{
    CHARSETINFO csi;

    if (!TranslateCharsetInfo((DWORD *)UIntToPtr( CodePage ), &csi, TCI_SRCCODEPAGE)) // Sundown: valid zero-extension of CodePage for TCI_SRCCOPAGE.
        csi.ciCharset = OEM_CHARSET;

    return (BYTE)csi.ciCharset;
}

LPTTFONTLIST
SearchTTFont(
    LPCONSOLEPROP_DATA pcpd,
    LPTSTR ptszFace,
    BOOL   fCodePage,
    UINT   CodePage
    )
{
    PSINGLE_LIST_ENTRY pTemp = pcpd->gTTFontList.Next;

    if (ptszFace) {
        while (pTemp != NULL) {
            LPTTFONTLIST pTTFontList = (LPTTFONTLIST)pTemp;

            if (wcscmp(ptszFace, pTTFontList->FaceName1) == 0 ||
                wcscmp(ptszFace, pTTFontList->FaceName2) == 0    ) {
                if (fCodePage)
                    if (pTTFontList->CodePage == CodePage )
                        return pTTFontList;
                    else
                        return NULL;
                else
                    return pTTFontList;
            }

            pTemp = pTemp->Next;
        }
    }

    return NULL;
}

BOOL
IsAvailableTTFont(
    LPCONSOLEPROP_DATA pcpd,
    LPTSTR ptszFace
    )
{
    if (SearchTTFont(pcpd, ptszFace, FALSE, 0))
        return TRUE;
    else
        return FALSE;
}

BOOL
IsAvailableTTFontCP(
    LPCONSOLEPROP_DATA pcpd,
    LPTSTR ptszFace,
    UINT CodePage
    )
{
    if (SearchTTFont(pcpd, ptszFace, TRUE, CodePage))
        return TRUE;
    else
        return FALSE;
}

BOOL
IsDisableBoldTTFont(
    LPCONSOLEPROP_DATA pcpd,
    LPTSTR ptszFace
    )
{
    LPTTFONTLIST pTTFontList;

    pTTFontList = SearchTTFont(pcpd, ptszFace, FALSE, 0);
    if (pTTFontList != NULL)
        return pTTFontList->fDisableBold;
    else
        return FALSE;
}

LPTSTR
GetAltFaceName(
    LPCONSOLEPROP_DATA pcpd,
    LPTSTR ptszFace
    )
{
    LPTTFONTLIST pTTFontList;

    pTTFontList = SearchTTFont(pcpd, ptszFace, FALSE, 0);
    if (pTTFontList) {
        if (wcscmp(ptszFace, pTTFontList->FaceName1) == 0) {
            return pTTFontList->FaceName2;
        }
        if (wcscmp(ptszFace, pTTFontList->FaceName2) == 0) {
            return pTTFontList->FaceName1;
        }
        return NULL;
    }
    else
        return NULL;
}

NTSTATUS
DestroyDbcsMisc(
    LPCONSOLEPROP_DATA pcpd
    )
{
    while (pcpd->gTTFontList.Next != NULL) {
        LPTTFONTLIST pTTFontList = (LPTTFONTLIST)PopEntryList(&pcpd->gTTFontList);

        if (pTTFontList != NULL)
            LocalFree(pTTFontList);
    }

    return STATUS_SUCCESS;
}

typedef struct _LC_List {
    struct _LC_List* Next;
    BOOL   FindFlag;
    WCHAR  LC_String[9];
} LC_List, *PLC_List;

static PLC_List LocaleList;

BOOL CALLBACK
EnumProc(
    LPWSTR LC_String
    )
{
    PLC_List TmpList;

    if (lstrlenW(LC_String) <= (sizeof(LocaleList->LC_String)/sizeof(WCHAR))-1)
    {
        TmpList = (PLC_List)&LocaleList;

        while(TmpList->Next != NULL)
            TmpList = TmpList->Next;

        TmpList->Next = LocalAlloc(LPTR, sizeof(LC_List));
        if (TmpList->Next != NULL)
        {
            TmpList = TmpList->Next;
            lstrcpyW(TmpList->LC_String, LC_String);
        }
    }
    return TRUE;
}


int
LanguageListCreate(
    HWND hDlg,
    UINT CodePage
    )

/*++

    Initializes the Language list by enumerating all Locale Information.

    Returns
--*/

{
    HWND hWndLanguageCombo;
    HANDLE hkRegistry = NULL;
    NTSTATUS Status;
    WCHAR awchValue[ 512 ];
    WCHAR awchData[ 512 ];
    DWORD dwIndex;
    PLC_List TmpList;
    WORD LangID;
    LCID Locale;
    int  cchData;
    LONG lListIndex;
    UINT cp;

    ENTERCRITICAL;

    /*
     * Enumrate system locale information
     */
    EnumSystemLocalesW( EnumProc, CP_INSTALLED );

    /*
     * Enumrate registory key
     */
    Status = MyRegOpenKey(NULL,
                          MACHINE_REGISTRY_CONSOLE_NLS,
                          &hkRegistry);
    if (NT_SUCCESS( Status )) {
        for( dwIndex = 0; ; dwIndex++)
        {
            Status = MyRegEnumValue(hkRegistry,
                                    dwIndex,
                                    sizeof(awchValue), (LPWSTR)&awchValue,
                                    sizeof(awchData),  (PBYTE)&awchData);
            if (!NT_SUCCESS( Status ))
            {
                break;
            }

            TmpList = (PLC_List)&LocaleList;
            while(TmpList->Next != NULL)
            {
                TmpList = TmpList->Next;
                if (lstrcmpW(awchValue, TmpList->LC_String) == 0)
                {
                    TmpList->FindFlag = TRUE;
                    break;
                }
            }
        }

        NtClose(hkRegistry);

    }

    /*
     * Create ComboBox items
     */
    hWndLanguageCombo = GetDlgItem(hDlg, IDC_CNSL_LANGUAGELIST);
    SendMessage(hWndLanguageCombo, CB_RESETCONTENT, 0, 0L);

    TmpList = (PLC_List)&LocaleList;
    while(TmpList->Next != NULL)
    {
        TmpList = TmpList->Next;

        if (TmpList->FindFlag)
        {
            LangID = ConvertStringToHex(TmpList->LC_String, NULL);
            Locale = MAKELCID( LangID, SORT_DEFAULT );

            awchValue[0] = L'\0';
            cp = 0;

#ifdef WINNT
            if (g_bRunOnNT5) {
                #define KERNEL32    _T("KERNEL32.DLL")

                #ifdef UNICODE
                #define GETCPINFOEX "GetCPInfoExW"
                #else
                #define GETCPINFOEX "GetCPInfoExA"
                #endif

                typedef BOOL (CALLBACK *LPFNGETCPINFOEX)(UINT, DWORD, LPCPINFOEX);
                LPFNGETCPINFOEX lpfnGetCPInfoEx;

                BOOL fRet = FALSE;
                CPINFOEX cpinfo;

                HMODULE hMod;

                cchData = GetLocaleInfoW(Locale, LOCALE_IDEFAULTCODEPAGE,
                                         awchData, sizeof(awchData)/sizeof(TCHAR));
                if (cchData)
                {
                    cp = ConvertStringToDec(awchData, NULL);

                    hMod = GetModuleHandle(KERNEL32);
                    if (hMod) {
                        lpfnGetCPInfoEx = (LPFNGETCPINFOEX)GetProcAddress(hMod,GETCPINFOEX);
                        if (lpfnGetCPInfoEx)
                            fRet = (*lpfnGetCPInfoEx)(cp, 0, &cpinfo);
                    }
                    if (fRet) {
                        lListIndex = (LONG) SendMessageW(hWndLanguageCombo, CB_ADDSTRING, 0, (LPARAM)cpinfo.CodePageName);
                        SendMessage(hWndLanguageCombo, CB_SETITEMDATA, (DWORD)lListIndex, cp);

                        if (CodePage == cp) {
                            SendMessage(hWndLanguageCombo, CB_SETCURSEL, lListIndex, 0L);
                        }
                    }
                }
            }
            else {
                cchData = GetLocaleInfoW(Locale, LOCALE_IDEFAULTCODEPAGE,
                                         awchData, sizeof(awchData)/sizeof(TCHAR));
                if (cchData)
                {
                    awchData[cchData] = L'\0';
                    lstrcatW(awchValue, awchData);
                    cp = ConvertStringToDec(awchData, NULL);
                }
                lstrcatW(awchValue, L" : ");

                cchData = GetLocaleInfoW(Locale, LOCALE_SCOUNTRY,
                                         awchData, sizeof(awchData)/sizeof(TCHAR));
                if (cchData)
                {
                    awchData[cchData] = L'\0';
                    lstrcatW(awchValue, awchData);
                }
                lstrcatW(awchValue, L" - ");

                cchData = GetLocaleInfoW(Locale, LOCALE_SLANGUAGE,
                                         awchData, sizeof(awchData)/sizeof(TCHAR));
                if (cchData)
                {
                    awchData[cchData] = L'\0';
                    lstrcatW(awchValue, awchData);
                }

                lListIndex = (LONG) SendMessageW(hWndLanguageCombo, CB_ADDSTRING, 0, (LPARAM)awchValue);
                SendMessage(hWndLanguageCombo, CB_SETITEMDATA, (DWORD)lListIndex, cp);
            }
#endif

            if (CodePage == cp) {
                SendMessage(hWndLanguageCombo, CB_SETCURSEL, lListIndex, 0L);
            }

        }
    }

    {
        PLC_List Tmp;

        TmpList = (PLC_List)&LocaleList;
        while(TmpList->Next != NULL)
        {
            Tmp = TmpList;
            TmpList = TmpList->Next;

            if (Tmp != (PLC_List)&LocaleList)
                LocalFree(Tmp);
        }

        LocaleList = NULL;
    }

    LEAVECRITICAL;

    /*
     * Get the LocaleIndex from the currently selected item.
     * (i will be LB_ERR if no currently selected item).
     */
    lListIndex = (LONG) SendMessage(hWndLanguageCombo, CB_GETCURSEL, 0, 0L);
    return (int) SendMessage(hWndLanguageCombo, CB_GETITEMDATA, lListIndex, 0L);
}
#endif // DBCS
