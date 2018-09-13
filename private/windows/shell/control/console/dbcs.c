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

#include "precomp.h"
#pragma hdrstop


#if defined(FE_SB)

SINGLE_LIST_ENTRY gTTFontList;    // This list contain TTFONTLIST data.


UINT OEMCP;
BOOL gfFESystem;


WORD
ConvertStringToDec(
    LPTSTR lpch,
    LPTSTR *endptr
    )
{
    TCHAR ch;
    WORD val = 0;

    while ( (ch=*lpch) != TEXT('\0'))
    {
        if (TEXT('0') <= ch && ch <= TEXT('9'))
            val = (val * 10) + (ch - TEXT('0'));
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
    LPTSTR lpch,
    LPTSTR *endptr
    )
{
    TCHAR ch;
    WORD val = 0;

    while ( (ch=*lpch) != TEXT('\0'))
    {
        if (TEXT('0') <= ch && ch <= TEXT('9'))
            val = (val << 4) + (ch - TEXT('0'));
        else if (TEXT('A') <= ch && ch <= TEXT('F'))
            val = (val << 4) + (ch - TEXT('A') + 10);
        else if (TEXT('a') <= ch && ch <= TEXT('f'))
            val = (val << 4) + (ch - TEXT('a') + 10);
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
    COORD FontSize = FontInfo[DefaultFontIndex].Size;
    COORD FontDelta;
    BOOL  fDbcsCharSet = IS_ANY_DBCS_CHARSET( CodePageToCharSet( CodePage ) );

    FontIndex = 0;
    Find = (DWORD)-1;
    for (i=0; i < NumberOfFonts; i++)
    {
        if (!TM_IS_TT_FONT(FontInfo[i].Family) &&
            IS_ANY_DBCS_CHARSET(FontInfo[i].tmCharSet) == fDbcsCharSet
           )
        {
            FontDelta.X = abs(FontSize.X - FontInfo[i].Size.X);
            FontDelta.Y = abs(FontSize.Y - FontInfo[i].Size.Y);
            if (Find > (DWORD)(FontDelta.X + FontDelta.Y))
            {
                Find = (DWORD)(FontDelta.X + FontDelta.Y);
                FontIndex = i;
            }
        }
    }

    *AltFontIndex = FontIndex;
    _tcscpy(AltFaceName, FontInfo[*AltFontIndex].FaceName);
    *AltFontSize = FontInfo[*AltFontIndex].Size;
    *AltFontFamily = FontInfo[*AltFontIndex].Family;

    DBGFONTS(("MakeAltRasterFont : AltFontIndex = %ld\n", *AltFontIndex));

    return STATUS_SUCCESS;
}

NTSTATUS
InitializeDbcsMisc(
    VOID
    )
{
    HANDLE hkRegistry = NULL;
    NTSTATUS Status;
    WCHAR awchValue[ 512 ];
    WCHAR awchData[ 512 ];
    DWORD dwIndex;
    LPWSTR pwsz;

    gTTFontList.Next = NULL;

    Status = MyRegOpenKey(NULL,
                          MACHINE_REGISTRY_CONSOLE_TTFONT,
                          &hkRegistry);
    if (!NT_SUCCESS( Status )) {
        DBGPRINT(("CONSRV: NtOpenKey failed %ws\n", MACHINE_REGISTRY_CONSOLE));
    }
    else {
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
            _tcscpy(pTTFontList->FaceName1, pwsz);

            pwsz += _tcslen(pwsz) + 1;
            if (*pwsz == BOLD_MARK) {
                pTTFontList->fDisableBold = TRUE;
                pwsz++;
            }
            _tcscpy(pTTFontList->FaceName2, pwsz);

            PushEntryList(&gTTFontList, &(pTTFontList->List));
        }

        NtClose(hkRegistry);
    }

    ASSERT(OEMCP != 0); // OEMCP must be initialized so far by CPL_INIT
    ASSERT(IsFarEastCP(OEMCP) == gfFESystem);

    return STATUS_SUCCESS;
}

BYTE
CodePageToCharSet(
    UINT CodePage
    )
{
    CHARSETINFO csi;

    if (!TranslateCharsetInfo((DWORD *)CodePage, &csi, TCI_SRCCODEPAGE))
        csi.ciCharset = OEM_CHARSET;

    return (BYTE)csi.ciCharset;
}

LPTTFONTLIST
SearchTTFont(
    LPTSTR ptszFace,
    BOOL   fCodePage,
    UINT   CodePage
    )
{
    PSINGLE_LIST_ENTRY pTemp = gTTFontList.Next;

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
    LPTSTR ptszFace
    )
{
    if (SearchTTFont(ptszFace, FALSE, 0))
        return TRUE;
    else
        return FALSE;
}

BOOL
IsAvailableTTFontCP(
    LPTSTR ptszFace,
    UINT CodePage
    )
{
    if (SearchTTFont(ptszFace, TRUE, CodePage))
        return TRUE;
    else
        return FALSE;
}

BOOL
IsDisableBoldTTFont(
    LPTSTR ptszFace
    )
{
    LPTTFONTLIST pTTFontList;

    pTTFontList = SearchTTFont(ptszFace, FALSE, 0);
    if (pTTFontList != NULL)
        return pTTFontList->fDisableBold;
    else
        return FALSE;
}

LPTSTR
GetAltFaceName(
    LPTSTR ptszFace
    )
{
    LPTTFONTLIST pTTFontList;

    pTTFontList = SearchTTFont(ptszFace, FALSE, 0);
    if (pTTFontList != NULL) {
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
    VOID
    )
{
    while (gTTFontList.Next != NULL) {
        LPTTFONTLIST pTTFontList = (LPTTFONTLIST)PopEntryList(&gTTFontList);

        if (pTTFontList != NULL)
            LocalFree(pTTFontList);
    }

    return STATUS_SUCCESS;
}

typedef struct _LC_List {
    struct _LC_List* Next;
    BOOL   FindFlag;
    TCHAR  LC_String[9];
} LC_List, *PLC_List;

static PLC_List LocaleList = NULL;

BOOL CALLBACK
EnumProc(
    LPTSTR LC_String
    )
{
    PLC_List TmpList;

    if (_tcslen(LC_String) <= (sizeof(LocaleList->LC_String)/sizeof(TCHAR))-1)
    {
        TmpList = (PLC_List)&LocaleList;

        while(TmpList->Next != NULL)
            TmpList = TmpList->Next;

        TmpList->Next = LocalAlloc(LPTR, sizeof(LC_List));
        if (TmpList->Next != NULL)
        {
            TmpList = TmpList->Next;
            _tcscpy(TmpList->LC_String, LC_String);
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
    BOOL fRet;
    CPINFOEX cpinfo;

    /*
     * Enumrate system locale information
     */
    EnumSystemLocales( EnumProc, CP_INSTALLED );

    /*
     * Enumrate registory key
     */
    Status = MyRegOpenKey(NULL,
                          MACHINE_REGISTRY_CONSOLE_NLS,
                          &hkRegistry);
    if (!NT_SUCCESS( Status )) {
        DBGPRINT(("CONSRV: NtOpenKey failed %ws\n", MACHINE_REGISTRY_CONSOLE));
    }
    else {
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
                if (_tcscmp(awchValue, TmpList->LC_String) == 0)
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
    hWndLanguageCombo = GetDlgItem(hDlg, IDD_LANGUAGELIST);
    SendMessage(hWndLanguageCombo, CB_RESETCONTENT, 0, 0L);

    TmpList = (PLC_List)&LocaleList;
    while(TmpList->Next != NULL)
    {
        TmpList = TmpList->Next;

        if (TmpList->FindFlag)
        {
            LangID = ConvertStringToHex(TmpList->LC_String, NULL);
            Locale = MAKELCID( LangID, SORT_DEFAULT );

            cchData = GetLocaleInfo(Locale, LOCALE_IDEFAULTCODEPAGE,
                                    awchData, sizeof(awchData)/sizeof(TCHAR));
            if (cchData)
            {
                awchData[cchData] = TEXT('\0');
                cp = ConvertStringToDec(awchData, NULL);

                if ( (IS_ANY_DBCS_CHARSET(CodePageToCharSet(cp)) && GetOEMCP() == cp) ||
                     (!IS_ANY_DBCS_CHARSET(CodePageToCharSet(cp))) ) {

                    fRet = GetCPInfoEx(cp, 0, &cpinfo);
                    if (fRet) {
                        lListIndex = (LONG)SendMessage(hWndLanguageCombo, CB_ADDSTRING, 0, (LPARAM)cpinfo.CodePageName);
                        SendMessage(hWndLanguageCombo, CB_SETITEMDATA, (DWORD)lListIndex, cp);

                        if (CodePage == cp) {
                            SendMessage(hWndLanguageCombo, CB_SETCURSEL, lListIndex, 0L);
                        }
                    }
                }
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


    /*
     * Get the LocaleIndex from the currently selected item.
     * (i will be LB_ERR if no currently selected item).
     */
    lListIndex = (LONG)SendMessage(hWndLanguageCombo, CB_GETCURSEL, 0, 0L);
    return (LONG)SendMessage(hWndLanguageCombo, CB_GETITEMDATA, lListIndex, 0L);
}


// v-HirShi Nov.20.1996
int
LanguageDisplay(
    HWND hDlg,
    UINT CodePage
    )

/*++

    Display the Language .

    Returns
--*/

{
    HWND hWndLanguageDisp;
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
    BOOL fRet;
    CPINFOEX cpinfo;

    /*
     * Enumrate system locale information
     */
    EnumSystemLocales( EnumProc, CP_INSTALLED );

    /*
     * Enumrate registory key
     */
    Status = MyRegOpenKey(NULL,
                          MACHINE_REGISTRY_CONSOLE_NLS,
                          &hkRegistry);
    if (!NT_SUCCESS( Status )) {
        DBGPRINT(("CONSRV: NtOpenKey failed %ws\n", MACHINE_REGISTRY_CONSOLE));
    }
    else {
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
                if (_tcscmp(awchValue, TmpList->LC_String) == 0)
                {
                    TmpList->FindFlag = TRUE;
                    break;
                }
            }
        }

        NtClose(hkRegistry);

    }

    /*
     * Display Language
     */

    TmpList = (PLC_List)&LocaleList;
    while(TmpList->Next != NULL)
    {
        TmpList = TmpList->Next;

        if (TmpList->FindFlag)
        {
            LangID = ConvertStringToHex(TmpList->LC_String, NULL);
            Locale = MAKELCID( LangID, SORT_DEFAULT );

            cchData = GetLocaleInfo(Locale, LOCALE_IDEFAULTCODEPAGE,
                                    awchData, sizeof(awchData)/sizeof(TCHAR));
            if (cchData)
            {
                awchData[cchData] = TEXT('\0');
                cp = ConvertStringToDec(awchData, NULL);

                fRet = GetCPInfoEx(cp, 0, &cpinfo);
                if (fRet) {
                    if (CodePage == cp) {
                        hWndLanguageDisp = GetDlgItem(hDlg, IDD_LANGUAGE);
                        SetWindowText(hWndLanguageDisp, cpinfo.CodePageName);
                    }
                }
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

    return TRUE;
}


#endif // FE_SB
