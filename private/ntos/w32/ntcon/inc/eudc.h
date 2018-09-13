/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    eudc.h

Abstract:

Author:

    KazuM Apr.19.1996

Revision History:

--*/

typedef struct _EUDC_INFORMATION {
    BOOL LocalVDMEudcMode;
    BOOL LocalKeisenEudcMode;

    HDC hDCLocalEudc;                   // Double colored DBCS hDC
    HBITMAP hBmpLocalEudc;

    PVOID EudcFontCacheInformation;     // Same as PFONT_CACHE_INFORMATION

    COORD LocalEudcSize;

    INT EudcRangeSize;
        #define EUDC_RANGE_SIZE 16
    WCHAR EudcRange[EUDC_RANGE_SIZE];
} EUDC_INFORMATION, *PEUDC_INFORMATION;


NTSTATUS
CreateEUDC(
    PCONSOLE_INFORMATION Console
    );

VOID
DeleteEUDC(
    PCONSOLE_INFORMATION Console
    );

NTSTATUS
RegisterLocalEUDC(
    IN PCONSOLE_INFORMATION Console,
    IN WCHAR wChar,
    IN COORD FontSize,
    IN PCHAR FontFace
    );

VOID
FreeLocalEUDC(
    IN PCONSOLE_INFORMATION Console
    );

VOID
GetFitLocalEUDCFont(
    IN PCONSOLE_INFORMATION Console,
    IN WCHAR wChar
    );

BOOL
IsEudcRange(
    IN PCONSOLE_INFORMATION Console,
    IN WCHAR ch
    );

BOOL
CheckEudcRangeInString(
    IN PCONSOLE_INFORMATION Console,
    IN  PWCHAR string,
    IN  SHORT  len,
    OUT SHORT  *find_pos
    );

INT
GetSystemEUDCRangeW(
    WORD  *pwEUDCCharTable,
    UINT   cjSize
    );

WORD
ConvertStringToHex(
    LPWSTR lpch,
    LPWSTR *endptr
    );

WORD
ConvertStringToDec(
    LPWSTR lpch,
    LPWSTR *endptr
    );
