/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    This file contains functions to read and _rite values
    to the registry.

Author:

    Jerry Shea (JerrySh) 30-Sep-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#define CONSOLE_REGISTRY_CURRENTPAGE  (L"CurrentPage")


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
MyRegDeleteKey(
    IN HANDLE hKey,
    IN LPWSTR lpSubKey
    )
{
    UNICODE_STRING      SubKey;

    //
    // Convert the subkey to a counted Unicode string.
    //

    RtlInitUnicodeString( &SubKey, lpSubKey );

    //
    // Delete the subkey
    //

    return NtDeleteValueKey(
              hKey,
              &SubKey
              );
}

NTSTATUS
MyRegCreateKey(
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

    return NtCreateKey(
                    phResult,
                    KEY_READ | KEY_WRITE,
                    &Obja,
                    0,
                    NULL,
                    0,
                    NULL
                    );
}

NTSTATUS
MyRegQueryValue(
    IN HANDLE hKey,
    IN LPWSTR lpValueName,
    IN DWORD dwValueLength,
    OUT LPBYTE lpData
    )
{
    UNICODE_STRING ValueName;
    ULONG BufferLength;
    ULONG ResultLength;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    NTSTATUS Status;

    //
    // Convert the subkey to a counted Unicode string.
    //

    RtlInitUnicodeString( &ValueName, lpValueName );

    BufferLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + dwValueLength;
    KeyValueInformation = HeapAlloc(RtlProcessHeap(),0,BufferLength);
    if (KeyValueInformation == NULL)
        return STATUS_NO_MEMORY;

    Status = NtQueryValueKey(
                hKey,
                &ValueName,
                KeyValuePartialInformation,
                KeyValueInformation,
                BufferLength,
                &ResultLength
                );
    if (NT_SUCCESS(Status)) {
        ASSERT(KeyValueInformation->DataLength <= dwValueLength);
        RtlCopyMemory(lpData,
            KeyValueInformation->Data,
            KeyValueInformation->DataLength);
        if (KeyValueInformation->Type == REG_SZ) {
            if (KeyValueInformation->DataLength + sizeof(WCHAR) > dwValueLength) {
                KeyValueInformation->DataLength -= sizeof(WCHAR);
            }
            lpData[KeyValueInformation->DataLength++] = 0;
            lpData[KeyValueInformation->DataLength] = 0;
        }
    }
    HeapFree(RtlProcessHeap(),0,KeyValueInformation);
    return Status;
}


#if defined(FE_SB)
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
            KeyValueInformation->Type ==REG_MULTI_SZ
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
#endif

LPWSTR
TranslateConsoleTitle(
    LPWSTR ConsoleTitle
    )
/*++

    this routine translates path characters into '_' characters because
    the NT registry apis do not allow the creation of keys with
    names that contain path characters.  it allocates a buffer that
    must be freed.

--*/
{
    int ConsoleTitleLength, i;
    LPWSTR TranslatedTitle;

    ConsoleTitleLength = lstrlenW(ConsoleTitle) + 1;
    TranslatedTitle = HeapAlloc(RtlProcessHeap(), 0,
                                ConsoleTitleLength * sizeof(WCHAR));
    if (TranslatedTitle == NULL) {
        return NULL;
    }
    for (i = 0; i < ConsoleTitleLength; i++) {
        if (ConsoleTitle[i] == '\\') {
            TranslatedTitle[i] = (WCHAR)'_';
        } else {
            TranslatedTitle[i] = ConsoleTitle[i];
        }
    }
    return TranslatedTitle;
}


NTSTATUS
MyRegSetValue(
    IN HANDLE hKey,
    IN LPWSTR lpValueName,
    IN DWORD dwType,
    IN LPVOID lpData,
    IN DWORD cbData
    )
{
    UNICODE_STRING ValueName;

    //
    // Convert the subkey to a counted Unicode string.
    //

    RtlInitUnicodeString( &ValueName, lpValueName );

    return NtSetValueKey(
                    hKey,
                    &ValueName,
                    0,
                    dwType,
                    lpData,
                    cbData
                    );
}


NTSTATUS
MyRegUpdateValue(
    IN HANDLE hConsoleKey,
    IN HANDLE hKey,
    IN LPWSTR lpValueName,
    IN DWORD dwType,
    IN LPVOID lpData,
    IN DWORD cbData
    )
{
    NTSTATUS Status;
    BYTE Data[MAX_PATH];

    //
    // If this is not the main console key but the value is the same,
    // delete it. Otherwise, set it.
    //

    if (hConsoleKey != hKey) {
        Status = MyRegQueryValue(hConsoleKey, lpValueName, sizeof(Data), Data);
        if (NT_SUCCESS(Status)) {
            if (RtlCompareMemory(lpData, Data, cbData) == cbData) {
                return MyRegDeleteKey(hKey, lpValueName);
            }
        }
    }

    return MyRegSetValue(hKey, lpValueName, dwType, lpData, cbData);
}


PCONSOLE_STATE_INFO
InitRegistryValues(VOID)

/*++

Routine Description:

    This routine allocates a state info structure and fill it in with
    default values.

Arguments:

    none

Return Value:

    pStateInfo - pointer to structure to receive information

--*/

{
    PCONSOLE_STATE_INFO pStateInfo;

    pStateInfo = HeapAlloc(RtlProcessHeap(), 0, sizeof(CONSOLE_STATE_INFO));
    if (pStateInfo == NULL) {
        return NULL;
    }

    pStateInfo->Length = sizeof(CONSOLE_STATE_INFO);
    pStateInfo->ScreenAttributes = 0x07;            // white on black
    pStateInfo->PopupAttributes = 0xf5;             // purple on white
    pStateInfo->InsertMode = FALSE;
    pStateInfo->QuickEdit = FALSE;
    pStateInfo->FullScreen = FALSE;
    pStateInfo->ScreenBufferSize.X = 80;
    pStateInfo->ScreenBufferSize.Y = 25;
    pStateInfo->WindowSize.X = 80;
    pStateInfo->WindowSize.Y = 25;
    pStateInfo->WindowPosX = 0;
    pStateInfo->WindowPosY = 0;
    pStateInfo->AutoPosition = TRUE;
    pStateInfo->FontSize.X = 0;
    pStateInfo->FontSize.Y = 0;
    pStateInfo->FontFamily = 0;
    pStateInfo->FontWeight = 0;
    pStateInfo->FaceName[0] = TEXT('\0');
    pStateInfo->CursorSize = 25;
    pStateInfo->HistoryBufferSize = 25;
    pStateInfo->NumberOfHistoryBuffers = 4;
    pStateInfo->HistoryNoDup = 0;
    pStateInfo->ColorTable[ 0] = RGB(0,   0,   0   );
    pStateInfo->ColorTable[ 1] = RGB(0,   0,   0x80);
    pStateInfo->ColorTable[ 2] = RGB(0,   0x80,0   );
    pStateInfo->ColorTable[ 3] = RGB(0,   0x80,0x80);
    pStateInfo->ColorTable[ 4] = RGB(0x80,0,   0   );
    pStateInfo->ColorTable[ 5] = RGB(0x80,0,   0x80);
    pStateInfo->ColorTable[ 6] = RGB(0x80,0x80,0   );
    pStateInfo->ColorTable[ 7] = RGB(0xC0,0xC0,0xC0);
    pStateInfo->ColorTable[ 8] = RGB(0x80,0x80,0x80);
    pStateInfo->ColorTable[ 9] = RGB(0,   0,   0xFF);
    pStateInfo->ColorTable[10] = RGB(0,   0xFF,0   );
    pStateInfo->ColorTable[11] = RGB(0,   0xFF,0xFF);
    pStateInfo->ColorTable[12] = RGB(0xFF,0,   0   );
    pStateInfo->ColorTable[13] = RGB(0xFF,0,   0xFF);
    pStateInfo->ColorTable[14] = RGB(0xFF,0xFF,0   );
    pStateInfo->ColorTable[15] = RGB(0xFF,0xFF,0xFF);
#if defined(FE_SB)
    pStateInfo->CodePage = OEMCP; // scotthsu
#endif
    pStateInfo->hWnd = NULL;
    pStateInfo->ConsoleTitle[0] = TEXT('\0');

    return pStateInfo;
}


DWORD
GetRegistryValues(
    PCONSOLE_STATE_INFO pStateInfo
    )

/*++

Routine Description:

    This routine reads in values from the registry and places them
    in the supplied structure.

Arguments:

    pStateInfo - optional pointer to structure to receive information

Return Value:

    current page number

--*/

{
    HANDLE hCurrentUserKey;
    HANDLE hConsoleKey;
    HANDLE hTitleKey;
    NTSTATUS Status;
    LPWSTR TranslatedTitle;
    DWORD dwValue;
    DWORD dwRet = 0;
    DWORD i;
    WCHAR awchBuffer[LF_FACESIZE];

    //
    // Open the current user registry key
    //

    Status = RtlOpenCurrentUser(MAXIMUM_ALLOWED, &hCurrentUserKey);
    if (!NT_SUCCESS(Status)) {
        return 0;
    }

    //
    // Open the console registry key
    //

    Status = MyRegOpenKey(hCurrentUserKey,
                          CONSOLE_REGISTRY_STRING,
                          &hConsoleKey);
    if (!NT_SUCCESS(Status)) {
        NtClose(hCurrentUserKey);
        return 0;
    }

    //
    // If there is no structure to fill out, just get the current
    // page and bail out.
    //

    if (pStateInfo == NULL) {
        if (NT_SUCCESS(MyRegQueryValue(hConsoleKey,
                       CONSOLE_REGISTRY_CURRENTPAGE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
            dwRet = dwValue;
        }
        goto CloseKeys;
    }

    //
    // Open the console title subkey, if there is one
    //

    if (pStateInfo->ConsoleTitle[0] != TEXT('\0')) {
        TranslatedTitle = TranslateConsoleTitle(pStateInfo->ConsoleTitle);
        if (TranslatedTitle == NULL) {
            NtClose(hConsoleKey);
            NtClose(hCurrentUserKey);
            return 0;
        }
        Status = MyRegOpenKey(hConsoleKey,
                              TranslatedTitle,
                              &hTitleKey);
        HeapFree(RtlProcessHeap(),0,TranslatedTitle);
        if (!NT_SUCCESS(Status)) {
            NtClose(hConsoleKey);
            NtClose(hCurrentUserKey);
            return 0;
        }
    } else {
        hTitleKey = hConsoleKey;
    }

    //
    // Initial screen fill
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_FILLATTR,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->ScreenAttributes = (WORD)dwValue;
    }

    //
    // Initial popup fill
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_POPUPATTR,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->PopupAttributes = (WORD)dwValue;
    }

    //
    // Initial color table
    //

    for (i = 0; i < 16; i++) {
        wsprintf(awchBuffer, CONSOLE_REGISTRY_COLORTABLE, i);
        if (NT_SUCCESS(MyRegQueryValue(hTitleKey, awchBuffer,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
            pStateInfo->ColorTable[i] = dwValue;
        }
    }

    //
    // Initial insert mode
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_INSERTMODE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->InsertMode = !!dwValue;
    }

    //
    // Initial quick edit mode
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_QUICKEDIT,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->QuickEdit = !!dwValue;
    }

#ifdef i386
    //
    // Initial full screen mode
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_FULLSCR,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->FullScreen = !!dwValue;
    }
#endif
#if defined(FE_SB) // scotthsu
    //
    // Initial code page
    //

    ASSERT(OEMCP != 0);
    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_CODEPAGE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        if (IsValidCodePage(dwValue)) {
            pStateInfo->CodePage = (UINT) dwValue;
        }
    }
#endif

    //
    // Initial screen buffer size
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_BUFFERSIZE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->ScreenBufferSize.X = LOWORD(dwValue);
        pStateInfo->ScreenBufferSize.Y = HIWORD(dwValue);
    }

    //
    // Initial window size
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_WINDOWSIZE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->WindowSize.X = LOWORD(dwValue);
        pStateInfo->WindowSize.Y = HIWORD(dwValue);
    }

    //
    // Initial window position
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_WINDOWPOS,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->WindowPosX = (SHORT)LOWORD(dwValue);
        pStateInfo->WindowPosY = (SHORT)HIWORD(dwValue);
        pStateInfo->AutoPosition = FALSE;
    }

    //
    // Initial font size
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_FONTSIZE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->FontSize.X = LOWORD(dwValue);
        pStateInfo->FontSize.Y = HIWORD(dwValue);
    }

    //
    // Initial font family
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_FONTFAMILY,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->FontFamily = dwValue;
    }

    //
    // Initial font weight
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_FONTWEIGHT,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->FontWeight = dwValue;
    }

    //
    // Initial font face name
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_FACENAME,
                       sizeof(awchBuffer), (PBYTE)awchBuffer))) {
        RtlCopyMemory(pStateInfo->FaceName, awchBuffer, sizeof(awchBuffer));
    }

    //
    // Initial cursor size
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_CURSORSIZE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->CursorSize = dwValue;
    }

    //
    // Initial history buffer size
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_HISTORYSIZE,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->HistoryBufferSize = dwValue;
    }

    //
    // Initial number of history buffers
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_HISTORYBUFS,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->NumberOfHistoryBuffers = dwValue;
    }

    //
    // Initial history duplication mode
    //

    if (NT_SUCCESS(MyRegQueryValue(hTitleKey,
                       CONSOLE_REGISTRY_HISTORYNODUP,
                       sizeof(dwValue), (PBYTE)&dwValue))) {
        pStateInfo->HistoryNoDup = dwValue;
    }

    //
    // Close the registry keys
    //

    if (hTitleKey != hConsoleKey) {
        NtClose(hTitleKey);
    }

CloseKeys:
    NtClose(hConsoleKey);
    NtClose(hCurrentUserKey);

    return dwRet;
}


VOID
SetRegistryValues(
    PCONSOLE_STATE_INFO pStateInfo,
    DWORD dwPage
    )

/*++

Routine Description:

    This routine writes values to the registry from the supplied
    structure.

Arguments:

    pStateInfo - optional pointer to structure containing information
    dwPage     - current page number

Return Value:

    none

--*/

{
    HANDLE hCurrentUserKey;
    HANDLE hConsoleKey;
    HANDLE hTitleKey;
    NTSTATUS Status;
    LPWSTR TranslatedTitle;
    DWORD dwValue;
    DWORD i;
    WCHAR awchBuffer[LF_FACESIZE];

    //
    // Open the current user registry key
    //

    Status = RtlOpenCurrentUser(MAXIMUM_ALLOWED, &hCurrentUserKey);
    if (!NT_SUCCESS(Status)) {
        return;
    }

    //
    // Open the console registry key
    //

    Status = MyRegCreateKey(hCurrentUserKey,
                            CONSOLE_REGISTRY_STRING,
                            &hConsoleKey);
    if (!NT_SUCCESS(Status)) {
        NtClose(hCurrentUserKey);
        return;
    }

    //
    // Save the current page
    //

    MyRegSetValue(hConsoleKey,
                  CONSOLE_REGISTRY_CURRENTPAGE,
                  REG_DWORD, &dwPage, sizeof(dwPage));

    //
    // If we only want to save the current page, bail out
    //

    if (pStateInfo == NULL) {
        goto CloseKeys;
    }

    //
    // Open the console title subkey, if there is one
    //

    if (pStateInfo->ConsoleTitle[0] != TEXT('\0')) {
        TranslatedTitle = TranslateConsoleTitle(pStateInfo->ConsoleTitle);
        if (TranslatedTitle == NULL) {
            NtClose(hConsoleKey);
            NtClose(hCurrentUserKey);
            return;
        }
        Status = MyRegCreateKey(hConsoleKey,
                                TranslatedTitle,
                                &hTitleKey);
        HeapFree(RtlProcessHeap(),0,TranslatedTitle);
        if (!NT_SUCCESS(Status)) {
            NtClose(hConsoleKey);
            NtClose(hCurrentUserKey);
            return;
        }
    } else {
        hTitleKey = hConsoleKey;
    }

    //
    // Save screen and popup colors and color table
    //

    dwValue = pStateInfo->ScreenAttributes;
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_FILLATTR,
                     REG_DWORD, &dwValue, sizeof(dwValue));
    dwValue = pStateInfo->PopupAttributes;
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_POPUPATTR,
                     REG_DWORD, &dwValue, sizeof(dwValue));
    for (i = 0; i < 16; i++) {
        dwValue = pStateInfo->ColorTable[i];
        wsprintf(awchBuffer, CONSOLE_REGISTRY_COLORTABLE, i);
        MyRegUpdateValue(hConsoleKey, hTitleKey, awchBuffer,
                         REG_DWORD, &dwValue, sizeof(dwValue));
    }

    //
    // Save insert, quickedit, and fullscreen mode settings
    //

    dwValue = pStateInfo->InsertMode;
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_INSERTMODE,
                     REG_DWORD, &dwValue, sizeof(dwValue));
    dwValue = pStateInfo->QuickEdit;
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_QUICKEDIT,
                     REG_DWORD, &dwValue, sizeof(dwValue));
#ifdef i386
    dwValue = pStateInfo->FullScreen;
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_FULLSCR,
                     REG_DWORD, &dwValue, sizeof(dwValue));
#endif
#if defined(FE_SB) // scotthsu

    ASSERT(OEMCP != 0);
    if (gfFESystem) {
        dwValue = (DWORD) pStateInfo->CodePage;
        MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_CODEPAGE,
                         REG_DWORD, &dwValue, sizeof(dwValue));
    }
#endif

    //
    // Save screen buffer size
    //

    dwValue = MAKELONG(pStateInfo->ScreenBufferSize.X,
                       pStateInfo->ScreenBufferSize.Y);
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_BUFFERSIZE,
                     REG_DWORD, &dwValue, sizeof(dwValue));

    //
    // Save window size
    //

    dwValue = MAKELONG(pStateInfo->WindowSize.X,
                       pStateInfo->WindowSize.Y);
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_WINDOWSIZE,
                     REG_DWORD, &dwValue, sizeof(dwValue));

    //
    // Save window position
    //

    if (pStateInfo->AutoPosition) {
        MyRegDeleteKey(hTitleKey, CONSOLE_REGISTRY_WINDOWPOS);
    } else {
        dwValue = MAKELONG(pStateInfo->WindowPosX,
                           pStateInfo->WindowPosY);
        MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_WINDOWPOS,
                         REG_DWORD, &dwValue, sizeof(dwValue));
    }

    //
    // Save font size, family, weight, and face name
    //

    dwValue = MAKELONG(pStateInfo->FontSize.X,
                       pStateInfo->FontSize.Y);
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_FONTSIZE,
                     REG_DWORD, &dwValue, sizeof(dwValue));
    dwValue = pStateInfo->FontFamily;
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_FONTFAMILY,
                     REG_DWORD, &dwValue, sizeof(dwValue));
    dwValue = pStateInfo->FontWeight;
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_FONTWEIGHT,
                     REG_DWORD, &dwValue, sizeof(dwValue));
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_FACENAME,
                     REG_SZ, pStateInfo->FaceName,
                      (_tcslen(pStateInfo->FaceName) + 1) * sizeof(TCHAR));

    //
    // Save cursor size
    //

    dwValue = pStateInfo->CursorSize;
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_CURSORSIZE,
                     REG_DWORD, &dwValue, sizeof(dwValue));

    //
    // Save history buffer size and number
    //

    dwValue = pStateInfo->HistoryBufferSize;
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_HISTORYSIZE,
                     REG_DWORD, &dwValue, sizeof(dwValue));
    dwValue = pStateInfo->NumberOfHistoryBuffers;
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_HISTORYBUFS,
                     REG_DWORD, &dwValue, sizeof(dwValue));
    dwValue = pStateInfo->HistoryNoDup;
    MyRegUpdateValue(hConsoleKey, hTitleKey, CONSOLE_REGISTRY_HISTORYNODUP,
                     REG_DWORD, &dwValue, sizeof(dwValue));

    //
    // Close the registry keys
    //

    if (hTitleKey != hConsoleKey) {
        NtClose(hTitleKey);
    }

CloseKeys:
    NtClose(hConsoleKey);
    NtClose(hCurrentUserKey);
}
