/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    eudc.c

Abstract:

Author:

    KazuM Apr.19.1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#if defined(FE_SB)


NTSTATUS
CreateEUDC(
    PCONSOLE_INFORMATION Console
    )
{
    PEUDC_INFORMATION EudcInfo;

    EudcInfo = ConsoleHeapAlloc( MAKE_TAG( EUDC_TAG ), sizeof(EUDC_INFORMATION));
    if (EudcInfo == NULL) {
        return STATUS_NO_MEMORY;
    }
    EudcInfo->LocalVDMEudcMode = FALSE;
    EudcInfo->LocalKeisenEudcMode = FALSE;
    EudcInfo->hDCLocalEudc = NULL;
    EudcInfo->hBmpLocalEudc = NULL;
    EudcInfo->EudcFontCacheInformation = NULL;
    EudcInfo->LocalEudcSize.X = DEFAULT_EUDCSIZE;
    EudcInfo->LocalEudcSize.Y = DEFAULT_EUDCSIZE;

    RtlZeroMemory(&EudcInfo->EudcRange,sizeof(EudcInfo->EudcRange));
    EudcInfo->EudcRangeSize = GetSystemEUDCRangeW(EudcInfo->EudcRange, EUDC_RANGE_SIZE);
    if (EudcInfo->EudcRangeSize)
        EudcInfo->EudcRangeSize--;    // remove terminator

    Console->EudcInformation = (PVOID)EudcInfo;

    return STATUS_SUCCESS;
}

VOID
DeleteEUDC(
    PCONSOLE_INFORMATION Console
    )
{
    PEUDC_INFORMATION EudcInfo = Console->EudcInformation;

    if (EudcInfo->hDCLocalEudc) {
         ReleaseDC(NULL, EudcInfo->hDCLocalEudc);
         DeleteObject(EudcInfo->hBmpLocalEudc);
    }
}

NTSTATUS
RegisterLocalEUDC(
    IN PCONSOLE_INFORMATION Console,
    IN WCHAR wChar,
    IN COORD FontSize,
    IN PCHAR FontFace
    )
{
    NTSTATUS Status;
    PCHAR TmpBuff;
    DWORD BuffSize;
    PEUDC_INFORMATION EudcInfo = Console->EudcInformation;

    if (EudcInfo->EudcFontCacheInformation == NULL) {
        Status = (NTSTATUS)CreateFontCache(&(PFONT_CACHE_INFORMATION)EudcInfo->EudcFontCacheInformation);
        if (!NT_SUCCESS(Status)) {
            RIPMSG1(RIP_WARNING, "RegisterLocalEUDC: failed in CreateFontCache, Status is %08x", Status);
            return Status;
        }
    }

    BuffSize = CalcBitmapBufferSize(FontSize, BYTE_ALIGN);
    TmpBuff = FontFace;
    while(BuffSize--)
        *TmpBuff++ = ~(*TmpBuff);

    return (NTSTATUS)SetFontImage(EudcInfo->EudcFontCacheInformation,
                                  wChar,
                                  FontSize,
                                  BYTE_ALIGN,
                                  FontFace
                                 );
}

VOID
FreeLocalEUDC(
    IN PCONSOLE_INFORMATION Console
    )
{
    PEUDC_INFORMATION EudcInfo = Console->EudcInformation;

    if (EudcInfo->EudcFontCacheInformation != NULL) {
        DestroyFontCache((PFONT_CACHE_INFORMATION)EudcInfo->EudcFontCacheInformation);
    }

    ConsoleHeapFree(Console->EudcInformation);
}

VOID
GetFitLocalEUDCFont(
    IN PCONSOLE_INFORMATION Console,
    IN WCHAR wChar
    )
{
    NTSTATUS Status;
    COORD FontSize;
    VOID *FontFace;
    DWORD BuffSize;
    PEUDC_INFORMATION EudcInfo;
    PFONT_CACHE_INFORMATION FontCacheInfo;

    EudcInfo = (PEUDC_INFORMATION)Console->EudcInformation;
    FontCacheInfo = (PFONT_CACHE_INFORMATION)EudcInfo->EudcFontCacheInformation;

    FontSize = CON_FONTSIZE(Console);
    if (IsConsoleFullWidth(Console->hDC,Console->OutputCP,wChar)) {
        FontSize.X *= 2;
    }

    if ((EudcInfo->LocalEudcSize.X != FontSize.X) ||
        (EudcInfo->LocalEudcSize.Y != FontSize.Y)   ) {
        ReleaseDC(NULL, EudcInfo->hDCLocalEudc);
        DeleteObject(EudcInfo->hBmpLocalEudc);
        EudcInfo->hDCLocalEudc = CreateCompatibleDC(Console->hDC);
        EudcInfo->hBmpLocalEudc = CreateBitmap(FontSize.X, FontSize.Y, BITMAP_PLANES, BITMAP_BITS_PIXEL, NULL);
        SelectObject(EudcInfo->hDCLocalEudc, EudcInfo->hBmpLocalEudc);
        EudcInfo->LocalEudcSize.X = FontSize.X;
        EudcInfo->LocalEudcSize.Y = FontSize.Y;
    }

    BuffSize = CalcBitmapBufferSize(FontSize,WORD_ALIGN);
    FontFace = ConsoleHeapAlloc( MAKE_TAG( TMP_DBCS_TAG ), BuffSize);
    if (FontFace == NULL) {
        RIPMSG0(RIP_WARNING, "GetFitLocalEUDCFont: failed to allocate FontFace.");
        return;
    }

    Status = (NTSTATUS)GetFontImage(FontCacheInfo,
                                    wChar,
                                    FontSize,
                                    WORD_ALIGN,
                                    FontFace
                                   );
    if (! NT_SUCCESS(Status)) {

        if ((Console->Flags & CONSOLE_VDM_REGISTERED) &&
            FontSize.X == DefaultFontSize.X * 2       &&
            FontSize.Y == DefaultFontSize.Y           &&
            FontSize.X == VDM_EUDC_FONT_SIZE_X        &&
            FontSize.Y - 2 == VDM_EUDC_FONT_SIZE_Y      ) {

            COORD TmpFontSize = FontSize;

            TmpFontSize.Y -= 2;
            RtlFillMemory((PVOID)FontFace,BuffSize,0xff);
            Status = (NTSTATUS)GetFontImage(FontCacheInfo,
                                            wChar,
                                            TmpFontSize,
                                            WORD_ALIGN,
                                            FontFace
                                           );
            if (! NT_SUCCESS(Status)) {
                Status = (NTSTATUS)GetStretchedFontImage(FontCacheInfo,
                                                         wChar,
                                                         FontSize,
                                                         WORD_ALIGN,
                                                         FontFace
                                                        );
                if (! NT_SUCCESS(Status)) {
                    ASSERT(FALSE);
                    ConsoleHeapFree(FontFace);
                    return;
                }
            }
        }
        else {
            Status = (NTSTATUS)GetStretchedFontImage(FontCacheInfo,
                                                     wChar,
                                                     FontSize,
                                                     WORD_ALIGN,
                                                     FontFace
                                                    );
            if (! NT_SUCCESS(Status)) {
                ASSERT(FALSE);
                ConsoleHeapFree(FontFace);
                return;
            }
        }

        Status = (NTSTATUS)SetFontImage(FontCacheInfo,
                                        wChar,
                                        FontSize,
                                        WORD_ALIGN,
                                        FontFace
                                       );
        if (! NT_SUCCESS(Status)) {
            ASSERT(FALSE);
            ConsoleHeapFree(FontFace);
            return;
        }
    }

    SetBitmapBits(EudcInfo->hBmpLocalEudc, BuffSize, (PBYTE)FontFace);

    ConsoleHeapFree(FontFace);
}

BOOL
IsEudcRange(
    IN PCONSOLE_INFORMATION Console,
    IN WCHAR ch
    )
{
    PEUDC_INFORMATION EudcInfo;
    int i;

    EudcInfo = (PEUDC_INFORMATION)Console->EudcInformation;

    for (i=0; i < EudcInfo->EudcRangeSize; i+=2)
    {
        if (EudcInfo->EudcRange[i] <= ch && ch <= EudcInfo->EudcRange[i+1])
            return TRUE;
    }
    return FALSE;
}

BOOL
CheckEudcRangeInString(
    IN PCONSOLE_INFORMATION Console,
    IN  PWCHAR string,
    IN  SHORT  len,
    OUT SHORT  *find_pos
    )
{
    SHORT i;

    for (i = 0; i < len; i++,string++)
    {
        if (IsEudcRange(Console, *string))
        {
            *find_pos = i;
            return TRUE;
        }
    }
    return FALSE;
}

LPWSTR
SkipWhite(
    LPWSTR lpch
    )
{
    if( lpch == NULL )
        return( NULL );

    for ( ; ; lpch++ )
    {
        switch (*lpch)
        {
            case L' ':
            case L'\t':
            case L'\r':
            case L'\n':
                break;

            default:
                return(lpch);
        }
    }
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

INT
GetSystemEUDCRangeW(
    WORD  *pwEUDCCharTable,
    UINT   cjSize
    )
{
    NTSTATUS Status;
    HKEY     hkRegistry;
    UNICODE_STRING SystemACPString;
    WCHAR    awcACP[10];
    WCHAR    awchBuffer[ 512 ];
    INT      iEntry = 0;

    /*
     * Check parameter
     *
     * If pwEUDCWideCharTable == NULL && cjSize == 0
     * We have to return the needed buffer size to store data
     */
    if( ( pwEUDCCharTable == NULL && cjSize != 0 ) ||
        ( pwEUDCCharTable != NULL && cjSize == 0 )
      )
    {
        return 0;
    }

    /*
     * Open registry key
     */
    Status = MyRegOpenKey(NULL,
                          MACHINE_REGISTRY_EUDC,
                          &hkRegistry);
    if (!NT_SUCCESS( Status )) {
        DBGPRINT(("CONSRV:GetSystemEUDCRangeW() RegOpenKeyExW( %ws ) fail\n", MACHINE_REGISTRY_EUDC));
        return 0;
    }

    /*
     * Convert ACP to Unicode string..
     */
    SystemACPString.Length        = 0;
    SystemACPString.MaximumLength = sizeof(awcACP)/sizeof(WCHAR);
    SystemACPString.Buffer        = awcACP;
    RtlIntegerToUnicodeString( WINDOWSCP, 10, &SystemACPString );

    /*
     * Read registry data
     */
    Status = MyRegQueryValue(hkRegistry,
                             awcACP,
                             sizeof(awchBuffer), (PBYTE)&awchBuffer);
    if (!NT_SUCCESS( Status )) {
        DBGPRINT(("CONSRV:GetSystemEUDCRangeW NtQueryValueKey failed %ws\n", awcACP));
    }
    else {
        LPWSTR pwszBuf = awchBuffer;

        /*
         *  Perse the data
         */
        while( pwszBuf != NULL && *pwszBuf != L'\0' )
        {
            WORD ch1,ch2;

            // Get Start Range value

            pwszBuf = SkipWhite( pwszBuf );
            ch1 = ConvertStringToHex( pwszBuf, &pwszBuf );

            pwszBuf = SkipWhite( pwszBuf );
            if (*pwszBuf != L'-')
            {
                DBGPRINT(("CONSRV:GetSystemEUDCRangeW() Invalid format\n"));
                return( 0 );
            }

            // Get End Range value

            pwszBuf = SkipWhite( pwszBuf+1 );
            ch2 = ConvertStringToHex( pwszBuf, &pwszBuf );

            // Confirm the data sort order is correct

            if( ch1 > ch2 )
            {
                DBGPRINT(("CONSRV:GetSystemEUDCRangeW() Sort order is incorrect\n"));
                return( 0 );
            }

            // Move pointer to next

            pwszBuf = SkipWhite( pwszBuf );
            if( *pwszBuf == L',' )
                pwszBuf = SkipWhite( pwszBuf+1 );

            // Above , if pwszBuf is NULL , We reach the EOD

            iEntry ++;

            // If caller buffer is enough to store the data , store it.
            // If even not so, We have to continue perse data to compute the number of entry.

            // 3 - Because we have to store NULL as a mark of EOD

            if( cjSize >= 3 )
            {
                *pwEUDCCharTable++ = ch1;
                *pwEUDCCharTable++ = ch2;
                cjSize -= 2;
            }
        }

        *pwEUDCCharTable = L'\0';


        iEntry = iEntry * 2 + 1;

    }

    /*
     * Close registry handle
     */
    NtClose( hkRegistry );

    return (iEntry);
}
#endif
