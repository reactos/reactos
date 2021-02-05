/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions for managing print Forms
 * COPYRIGHT:   Copyright 2020 ReactOS
 */

#include "precomp.h"

#define FORMINFOSIG '.2'
#define FORMMAXNAMESIZE CCHDEVICENAME-1

typedef struct _FORM_INFO_LIST
{
    LIST_ENTRY List;
    DWORD Sig;
    DWORD Index;
    FORM_INFO_2W;
} FORM_INFO_LIST, *PFORM_INFO_LIST;

LIST_ENTRY FormList;
static DWORD _dwLastForm;

// Local Constants
static DWORD dwFormInfo1Offsets[] = {
    FIELD_OFFSET(FORM_INFO_1W, pName),
    MAXDWORD
};

static DWORD dwFormInfo2Offsets[] = {
    FIELD_OFFSET(FORM_INFO_2W, pName),
    FIELD_OFFSET(FORM_INFO_2W, pKeyword),
    FIELD_OFFSET(FORM_INFO_2W, pMuiDll),
    FIELD_OFFSET(FORM_INFO_2W, pDisplayName),
    MAXDWORD
};


// Built in Form names
WCHAR wszLetter[] = L"Letter";
WCHAR wszLetterSmall[] = L"Letter Small";
WCHAR wszTabloid[] = L"Tabloid";
WCHAR wszLedger[] = L"Ledger";
WCHAR wszLegal[] = L"Legal";
WCHAR wszStatement[] = L"Statement";
WCHAR wszExecutive[] = L"Executive";
WCHAR wszA3[] = L"A3";
WCHAR wszA4[] = L"A4";
WCHAR wszA4Small[] = L"A4 Small";
WCHAR wszA5[] = L"A5";
WCHAR wszB4JIS[] = L"B4 (JIS)";
WCHAR wszB5JIS[] = L"B5 (JIS)";
WCHAR wszFolio[] = L"Folio";
WCHAR wszQuarto[] = L"Quarto";
WCHAR wsz10x14[] = L"10 x 14";
WCHAR wsz11x17[] = L"11 x 17";
WCHAR wszNote[] = L"Note";
WCHAR wszEnvelope9[] = L"Envelope #9";
WCHAR wszEnvelope10[] = L"Envelope #10";
WCHAR wszEnvelope11[] = L"Envelope #11";
WCHAR wszEnvelope12[] = L"Envelope #12";
WCHAR wszEnvelope14[] = L"Envelope #14";
WCHAR wszCsizesheet[] = L"C size sheet";
WCHAR wszDsizesheet[] = L"D size sheet";
WCHAR wszEsizesheet[] = L"E size sheet";
WCHAR wszEnvelopeDL[] = L"Envelope DL";
WCHAR wszEnvelopeC5[] = L"Envelope C5";
WCHAR wszEnvelopeC3[] = L"Envelope C3";
WCHAR wszEnvelopeC4[] = L"Envelope C4";
WCHAR wszEnvelopeC6[] = L"Envelope C6";
WCHAR wszEnvelope65[] = L"Envelope 65";
WCHAR wszEnvelopeB4[] = L"Envelope B4";
WCHAR wszEnvelopeB5[] = L"Envelope B5";
WCHAR wszEnvelopeB6[] = L"Envelope B6";
WCHAR wszEnvelope[] = L"Envelope";
WCHAR wszEnvelopeMonarch[] = L"Envelope Monarch";
WCHAR wsz634Envelope[] = L"6 3/4 Envelope";
WCHAR wszUSStdFanfold[] = L"US Std Fanfold";
WCHAR wszGermanStdFanfold[] = L"German Std Fanfold";
WCHAR wszGermanLegalFanfold[] = L"German Legal Fanfold";
WCHAR wszB4ISO[] = L"B4 (ISO)";
WCHAR wszJapanesePostcard[] = L"Japanese Postcard";
WCHAR wsz9x11[] = L"9 x 11";
WCHAR wsz10x11[] = L"10 x 11";
WCHAR wsz15x11[] = L"15 x 11";
WCHAR wszEnvelopeInvite[] = L"Envelope Invite";
WCHAR wszReserved48[] = L"Reserved48";
WCHAR wszReserved49[] = L"Reserved49";
WCHAR wszLetterExtra[] = L"Letter Extra";
WCHAR wszLegalExtra[] = L"Legal Extra";
WCHAR wszTabloidExtra[] = L"Tabloid Extra";
WCHAR wszA4Extra[] = L"A4 Extra";
WCHAR wszLetterTransverse[] = L"Letter Transverse";
WCHAR wszA4Transverse[] = L"A4 Transverse";
WCHAR wszLetterExtraTransverse[] = L"Letter Extra Transverse";
WCHAR wszSuperA[] = L"Super A";
WCHAR wszSuperB[] = L"Super B";
WCHAR wszLetterPlus[] = L"Letter Plus";
WCHAR wszA4Plus[] = L"A4 Plus";
WCHAR wszA5Transverse[] = L"A5 Transverse";
WCHAR wszB5JISTransverse[] = L"B5 (JIS) Transverse";
WCHAR wszA3Extra[] = L"A3 Extra";
WCHAR wszA5Extra[] = L"A5 Extra";
WCHAR wszB5ISOExtra[] = L"B5 (ISO) Extra";
WCHAR wszA0[] = L"A0";
WCHAR wszA3Transverse[] = L"A3 Transverse";
WCHAR wszA3ExtraTransverse[] = L"A3 Extra Transverse";
WCHAR wszJapaneseDoublePostcard[] = L"Japanese Double Postcard";
WCHAR wszA1[] = L"A1";
WCHAR wszJapaneseEnvelopeKaku2[] = L"Japanese Envelope Kaku #2";
WCHAR wszJapaneseEnvelopeKaku3[] = L"Japanese Envelope Kaku #3";
WCHAR wszJapaneseEnvelopeChou3[] = L"Japanese Envelope Chou #3";
WCHAR wszJapaneseEnvelopeChou4[] = L"Japanese Envelope Chou #4";
WCHAR wszLetterRotated[] = L"Letter Rotated";
WCHAR wszA3Rotated[] = L"A3 Rotated";
WCHAR wszA4Rotated[] = L"A4 Rotated";
WCHAR wszA5Rotated[] = L"A5 Rotated";
WCHAR wszB4JISRotated[] = L"B4 (JIS) Rotated";
WCHAR wszB5JISRotated[] = L"B5 (JIS) Rotated";
WCHAR wszJapanesePostcardRotated[] = L"Japanese Postcard Rotated";
WCHAR wszDoubleJapanPostcardRotated[] = L"Double Japan Postcard Rotated";
WCHAR wsA6Rotatedz[] = L"A6 Rotated";
WCHAR wszJapanEnvelopeKaku2Rotated[] = L"Japan Envelope Kaku #2 Rotated";
WCHAR wszJapanEnvelopeKaku3Rotated[] = L"Japan Envelope Kaku #3 Rotated";
WCHAR wszJapanEnvelopeChou3Rotated[] = L"Japan Envelope Chou #3 Rotated";
WCHAR wszJapanEnvelopeChou4Rotated[] = L"Japan Envelope Chou #4 Rotated";
WCHAR wszB6JIS[] = L"B6 (JIS)";
WCHAR wszB6JISRotated[] = L"B6 (JIS) Rotated";
WCHAR wsz12x11[] = L"12 x 11";
WCHAR wszJapanEnvelopeYou4[] = L"Japan Envelope You #4";
WCHAR wszJapanEnvelopeYou4Rotated[] = L"Japan Envelope You #4 Rotated";
WCHAR wszPRC16K[] = L"PRC 16K";
WCHAR wszPRC32K[] = L"PRC 32K";
WCHAR wszPRC32KBig[] = L"PRC 32K(Big)";
WCHAR wszPRCEnvelope1[] = L"PRC Envelope #1";
WCHAR wszPRCEnvelope2[] = L"PRC Envelope #2";
WCHAR wszPRCEnvelope3[] = L"PRC Envelope #3";
WCHAR wszPRCEnvelope4[] = L"PRC Envelope #4";
WCHAR wszPRCEnvelope5[] = L"PRC Envelope #5";
WCHAR wszPRCEnvelope6[] = L"PRC Envelope #6";
WCHAR wszPRCEnvelope7[] = L"PRC Envelope #7";
WCHAR wszPRCEnvelope8[] = L"PRC Envelope #8";
WCHAR wszPRCEnvelope9[] = L"PRC Envelope #9";
WCHAR wszPRCEnvelope10[] = L"PRC Envelope #10";
WCHAR wszPRC16KRotated[] = L"PRC 16K Rotated";
WCHAR wszPRC32KRotated[] = L"PRC 32K Rotated";
WCHAR wszPRC32KBigRotated[] = L"PRC 32K(Big) Rotated";
WCHAR wszPRCEnvelope1Rotated[] = L"PRC Envelope #1 Rotated";
WCHAR wszPRCEnvelope2Rotated[] = L"PRC Envelope #2 Rotated";
WCHAR wszPRCEnvelope3Rotated[] = L"PRC Envelope #3 Rotated";
WCHAR wszPRCEnvelope4Rotated[] = L"PRC Envelope #4 Rotated";
WCHAR wszPRCEnvelope5Rotated[] = L"PRC Envelope #5 Rotated";
WCHAR wszPRCEnvelope6Rotated[] = L"PRC Envelope #6 Rotated";
WCHAR wszPRCEnvelope7Rotated[] = L"PRC Envelope #7 Rotated";
WCHAR wszPRCEnvelope8Rotated[] = L"PRC Envelope #8 Rotated";
WCHAR wszPRCEnvelope9Rotated[] = L"PRC Envelope #9 Rotated";
WCHAR wszPRCEnvelope10Rotated[] = L"PRC Envelope #10 Rotated";

// Built in Forms
FORM_INFO_1W BuiltInForms[] =
{
    { FORM_USER, wszLetter,                    {215900, 279400},{ 0, 0, 215900, 279400}},
    { FORM_USER, wszLetterSmall,               {215900, 279400},{ 0, 0, 215900, 279400}},
    { FORM_USER, wszTabloid,                   {279400, 431800},{ 0, 0, 279400, 431800}},
    { FORM_USER, wszLedger,                    {431800, 279400},{ 0, 0, 431800, 279400}},
    { FORM_USER, wszLegal,                     {215900, 355600},{ 0, 0, 215900, 355600}},
    { FORM_USER, wszStatement,                 {139700, 215900},{ 0, 0, 139700, 215900}},
    { FORM_USER, wszExecutive,                 {184150, 266700},{ 0, 0, 184150, 266700}},
    { FORM_USER, wszA3,                        {297000, 420000},{ 0, 0, 297000, 420000}},
    { FORM_USER, wszA4,                        {210000, 297000},{ 0, 0, 210000, 297000}},
    { FORM_USER, wszA4Small,                   {210000, 297000},{ 0, 0, 210000, 297000}},
    { FORM_USER, wszA5,                        {148000, 210000},{ 0, 0, 148000, 210000}},
    { FORM_USER, wszB4JIS,                     {257000, 364000},{ 0, 0, 257000, 364000}},
    { FORM_USER, wszB5JIS,                     {182000, 257000},{ 0, 0, 182000, 257000}},
    { FORM_USER, wszFolio,                     {215900, 330200},{ 0, 0, 215900, 330200}},
    { FORM_USER, wszQuarto,                    {215000, 275000},{ 0, 0, 215000, 275000}},
    { FORM_USER, wsz10x14,                     {254000, 355600},{ 0, 0, 254000, 355600}},
    { FORM_USER, wsz11x17,                     {279400, 431800},{ 0, 0, 279400, 431800}},
    { FORM_USER, wszNote,                      {215900, 279400},{ 0, 0, 215900, 279400}},
    { FORM_USER, wszEnvelope9,                 { 98425, 225425},{ 0, 0,  98425, 225425}},
    { FORM_USER, wszEnvelope10,                {104775, 241300},{ 0, 0, 104775, 241300}},
    { FORM_USER, wszEnvelope11,                {114300, 263525},{ 0, 0, 114300, 263525}},
    { FORM_USER, wszEnvelope12,                {120650, 279400},{ 0, 0, 120650, 279400}},
    { FORM_USER, wszEnvelope14,                {127000, 292100},{ 0, 0, 127000, 292100}},
    { FORM_USER, wszCsizesheet,                {431800, 558800},{ 0, 0, 431800, 558800}},
    { FORM_USER, wszDsizesheet,                {558800, 863600},{ 0, 0, 558800, 863600}},
    { FORM_USER, wszEsizesheet,                {863600,1117600},{ 0, 0, 863600,1117600}},
    { FORM_USER, wszEnvelopeDL,                {110000, 220000},{ 0, 0, 110000, 220000}},
    { FORM_USER, wszEnvelopeC5,                {162000, 229000},{ 0, 0, 162000, 229000}},
    { FORM_USER, wszEnvelopeC3,                {324000, 458000},{ 0, 0, 324000, 458000}},
    { FORM_USER, wszEnvelopeC4,                {229000, 324000},{ 0, 0, 229000, 324000}},
    { FORM_USER, wszEnvelopeC6,                {114000, 162000},{ 0, 0, 114000, 162000}},
    { FORM_USER, wszEnvelope65,                {114000, 229000},{ 0, 0, 114000, 229000}},
    { FORM_USER, wszEnvelopeB4,                {250000, 353000},{ 0, 0, 250000, 353000}},
    { FORM_USER, wszEnvelopeB5,                {176000, 250000},{ 0, 0, 176000, 250000}},
    { FORM_USER, wszEnvelopeB6,                {176000, 125000},{ 0, 0, 176000, 125000}},
    { FORM_USER, wszEnvelope,                  {110000, 230000},{ 0, 0, 110000, 230000}},
    { FORM_USER, wszEnvelopeMonarch,           { 98425, 190500},{ 0, 0,  98425, 190500}},
    { FORM_USER, wsz634Envelope,               { 92075, 165100},{ 0, 0,  92075, 165100}},
    { FORM_USER, wszUSStdFanfold,              {377825, 279400},{ 0, 0, 377825, 279400}},
    { FORM_USER, wszGermanStdFanfold,          {215900, 304800},{ 0, 0, 215900, 304800}},
    { FORM_USER, wszGermanLegalFanfold,        {215900, 330200},{ 0, 0, 215900, 330200}},
// add more, I'm lazy
    { FORM_USER, wszDoubleJapanPostcardRotated,{148000, 200000},{ 0, 0, 148000, 200000}},
//
    { FORM_USER, wszPRCEnvelope10Rotated,      {458000, 324009},{ 0, 0, 458000, 324009}},
    { FORM_USER, 0,                            {     0,      0},{ 0, 0,      0,      0}}
};

//
// Form information Registry entry stucture, in correct format.
//
// Example : 11 x 17  REG_BINARY  68 43 04 00 b8 96 06 00 00 00 00 00 00 00 00 00 68 43 04 00 b8 96 06 00 01 00 00 00 02 00 00 00
//                               [        Size           |               ImageableArea                   | Index 1 + |   Flags   ] , is FORM_PRINTER.
//
typedef struct _REGISTRYFORMINFO
{
    SIZEL Size;
    RECTL ImageableArea;
    DWORD Index;
    DWORD Flags;
} REGISTRYFORMINFO, *PREGISTRYFORMINFO;

HKEY hFormCKey = NULL;
HKEY hFormsKey = NULL;

BOOL
InitializeFormList(VOID)
{
    DWORD dwErrorCode;
    PFORM_INFO_LIST pfil;
    REGISTRYFORMINFO rfi;

    FIXME("InitializeFormList\n");

    dwErrorCode = (DWORD)RegCreateKeyExW( HKEY_LOCAL_MACHINE,
                                         L"SYSTEM\\CurrentControlSet\\Control\\Print\\Forms",
                                          0,
                                          NULL,
                                          REG_OPTION_VOLATILE,
                                          KEY_ALL_ACCESS,
                                          NULL,
                                         &hFormCKey,
                                          NULL ); // KEY_OPENED_EXISTING_KEY );

    if ( dwErrorCode != ERROR_SUCCESS && dwErrorCode != ERROR_ALREADY_EXISTS )
    {
        ERR("RegCreateKeyExW failed for the Forms with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Open some registry keys and leave them open. We need them multiple times throughout the Local Spooler.
    dwErrorCode = (DWORD)RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Print\\Forms", 0, KEY_ALL_ACCESS, &hFormsKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed for \"Forms\" with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    _dwLastForm = 1;

    InitializeListHead(&FormList);

    {
        int i = 0, Size;
        while ( BuiltInForms[ i ].pName != NULL )
        {
            TRACE("InitializeFormList L s %S\n",BuiltInForms[ i ].pName );

            Size = sizeof(FORM_INFO_LIST) + ((wcslen(BuiltInForms[ i ].pName) + 1) * sizeof(WCHAR));

            pfil = DllAllocSplMem( Size );

            pfil->pName         = wcscpy( (PWSTR)(pfil+1), BuiltInForms[ i ].pName );
            pfil->Flags         = BuiltInForms[ i ].Flags;
            pfil->Size          = BuiltInForms[ i ].Size;
            pfil->ImageableArea = BuiltInForms[ i ].ImageableArea;
            pfil->Sig           = FORMINFOSIG;
            pfil->Index         = _dwLastForm++;
            pfil->pKeyword      = NULL;
            pfil->StringType    = STRING_NONE;
            pfil->pMuiDll       = NULL;
            pfil->dwResourceId  = 0;
            pfil->pDisplayName  = NULL;
            pfil->wLangId       = 0;

            InsertTailList( &FormList, &pfil->List );

            rfi.Size          = pfil->Size;
            rfi.ImageableArea = pfil->ImageableArea;
            rfi.Index         = pfil->Index;
            rfi.Flags         = pfil->Flags;

            dwErrorCode = RegSetValueExW( hFormsKey, pfil->pName, 0, REG_BINARY, (PBYTE)&rfi, sizeof( rfi ) );
            if ( dwErrorCode == ERROR_SUCCESS )
            {
                TRACE("Init : RQVEW : %S added\n",pfil->pName);
            }

            i++;
        }
    }

Cleanup:
    return TRUE;
}

PFORM_INFO_LIST
FASTCALL
FindForm( WCHAR * pFormName, WCHAR * pKeyword )
{
    PLIST_ENTRY ListEntry;
    PFORM_INFO_LIST pfil;

    ListEntry = FormList.Flink;

    while (ListEntry != &FormList)
    {
        pfil = CONTAINING_RECORD(ListEntry, FORM_INFO_LIST, List);

        ListEntry = ListEntry->Flink;

        if ( pFormName && !_wcsicmp( pfil->pName, pFormName ) )
            return pfil;

        if ( pKeyword && !_wcsicmp( (WCHAR*)pfil->pKeyword, pKeyword ) )
            return pfil;
    }
    return NULL;
}

static void
_LocalGetFormLevel1(PFORM_INFO_LIST pfil, PFORM_INFO_1W* ppFormInfo, PBYTE* ppFormInfoEnd, PDWORD pcbNeeded)
{
    DWORD n;
    PCWSTR pwszStrings[1];

    pwszStrings[0] = pfil->pName;

    // Calculate the string lengths.
    if (!ppFormInfo)
    {
        for (n = 0; n < _countof(pwszStrings); ++n)
        {
            *pcbNeeded += (wcslen(pwszStrings[n]) + 1) * sizeof(WCHAR);
        }

        *pcbNeeded += sizeof(FORM_INFO_1W);
        return;
    }

    (*ppFormInfo)->Flags         = pfil->Flags;
    (*ppFormInfo)->Size          = pfil->Size;
    (*ppFormInfo)->ImageableArea = pfil->ImageableArea;

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppFormInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppFormInfo), dwFormInfo1Offsets, *ppFormInfoEnd);
    (*ppFormInfo)++;
}

static void
_LocalGetFormLevel2(PFORM_INFO_LIST pfil, PFORM_INFO_2W* ppFormInfo, PBYTE* ppFormInfoEnd, PDWORD pcbNeeded)
{
    DWORD n;
    PCWSTR pwszStrings[4];

    pwszStrings[0] = pfil->pName;
    pwszStrings[1] = (PCWSTR)pfil->pKeyword;
    pwszStrings[2] = pfil->pMuiDll;
    pwszStrings[3] = pfil->pDisplayName;

    // Calculate the string lengths.
    if (!ppFormInfo)
    {
        for (n = 0; n < _countof(pwszStrings); ++n)
        {
            if (pwszStrings[n])
            {
                *pcbNeeded += (wcslen(pwszStrings[n]) + 1) * sizeof(WCHAR);
            }
        }
        *pcbNeeded += sizeof(FORM_INFO_2W);
        return;
    }

    (*ppFormInfo)->Flags         = pfil->Flags;
    (*ppFormInfo)->Size          = pfil->Size;
    (*ppFormInfo)->ImageableArea = pfil->ImageableArea;
    (*ppFormInfo)->StringType    = pfil->StringType; //// If caller is remote, set STRING_LANGPAIR always;
    (*ppFormInfo)->dwResourceId  = pfil->dwResourceId;

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppFormInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppFormInfo), dwFormInfo2Offsets, *ppFormInfoEnd);
    (*ppFormInfo)++;
}

typedef void (*PLocalGetFormLevelFunc)(PFORM_INFO_LIST, PVOID, PBYTE*, PDWORD);

static const PLocalGetFormLevelFunc pfnGetFormLevels[] = {
    NULL,
    (PLocalGetFormLevelFunc)&_LocalGetFormLevel1,
    (PLocalGetFormLevelFunc)&_LocalGetFormLevel2
};

//
// API Functions
//
BOOL WINAPI
LocalAddForm(HANDLE hPrinter, DWORD Level, PBYTE pForm)
{
    DWORD dwErrorCode, Size, cbNeeded;
    PFORM_INFO_LIST pfil;
    PLOCAL_HANDLE pHandle;
    REGISTRYFORMINFO rfi;
    PFORM_INFO_1W pfi1w = (PFORM_INFO_1W)pForm;
    PFORM_INFO_2W pfi2w = (PFORM_INFO_2W)pForm;

    FIXME("AddForm(%p, %lu, %p)\n", hPrinter, Level, pForm);

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Only support 1 & 2
    if (Level < 1 || Level > 2)
    {
        // The caller supplied an invalid level.
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    pfil = FindForm( pfi1w->pName, NULL );
    if ( pfil )
    {
        dwErrorCode = ERROR_FILE_EXISTS;
        goto Cleanup;
    }

    dwErrorCode = RegQueryValueExW( hFormsKey, pfi1w->pName, NULL, NULL, NULL, &cbNeeded );
    if ( dwErrorCode == ERROR_SUCCESS )
    {
        dwErrorCode = ERROR_FILE_EXISTS;
        goto Cleanup;
    }

    if ( wcslen(pfi1w->pName) > FORMMAXNAMESIZE ) // Limit REG Name size.
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    Size = sizeof(FORM_INFO_LIST) + ((MAX_PATH + 1) * sizeof(WCHAR));

    pfil = DllAllocSplMem( Size );
    if ( !pfil )
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    pfil->Sig           = FORMINFOSIG;
    pfil->Index         = _dwLastForm++;
    pfil->pName         = wcscpy( (PWSTR)(pfil+1), pfi1w->pName );
    pfil->Flags         = pfi1w->Flags;
    pfil->Size          = pfi1w->Size;
    pfil->ImageableArea = pfi1w->ImageableArea;
    pfil->StringType    = STRING_NONE;

    if ( Level > 1 )
    {
        pfil->pKeyword     = pfi2w->pKeyword;
        pfil->pMuiDll      = pfi2w->pMuiDll;
        pfil->pDisplayName = pfi2w->pDisplayName;
        pfil->StringType   = pfi2w->StringType;
        pfil->dwResourceId = pfi2w->dwResourceId;
    }

    rfi.Size          = pfil->Size;
    rfi.ImageableArea = pfil->ImageableArea;
    rfi.Index         = pfil->Index;
    rfi.Flags         = pfil->Flags;

    dwErrorCode = RegSetValueExW( hFormsKey, pfil->pName, 0, REG_BINARY, (PBYTE)&rfi, sizeof( rfi ) );

    BroadcastChange(pHandle);

    InsertTailList( &FormList, &pfil->List );

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalDeleteForm(HANDLE hPrinter, PWSTR pFormName)
{
    DWORD dwErrorCode, cbNeeded;
    PFORM_INFO_LIST pfil;
    REGISTRYFORMINFO rfi;
    PLOCAL_HANDLE pHandle;

    FIXME("DeleteForm(%p, %S)\n", hPrinter, pFormName);

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pfil = FindForm( pFormName, NULL );
    if ( !pfil )
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    dwErrorCode = RegQueryValueExW( hFormsKey, pFormName, NULL, NULL, (PBYTE)&rfi, &cbNeeded );
    if ( dwErrorCode != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    dwErrorCode = RegDeleteValueW(hFormsKey, pFormName);
    if ( dwErrorCode != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    RemoveEntryList(&pfil->List);

    DllFreeSplMem(pfil);

    BroadcastChange(pHandle);

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalEnumForms(HANDLE hPrinter, DWORD Level, PBYTE pForm, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;
    PFORM_INFO_LIST pfil = NULL;
    PBYTE pEnd = &pForm[cbBuf];
    PLOCAL_HANDLE pHandle;
    PLIST_ENTRY ListEntry;

    FIXME("EnumForms(%p, %lu, %p, %lu, %p, %p)\n", hPrinter, Level, pForm, cbBuf, pcbNeeded, pcReturned);

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Only support 1 & 2
    if (Level < 1 || Level > 2)
    {
        // The caller supplied an invalid level.
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Count the required buffer size.
    *pcbNeeded = 0;

    ListEntry = FormList.Flink;

    if (IsListEmpty(ListEntry))
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    while ( ListEntry != &FormList )
    {
        pfil = CONTAINING_RECORD(ListEntry, FORM_INFO_LIST, List);
        ListEntry = ListEntry->Flink;

        pfnGetFormLevels[Level](pfil, NULL, NULL, pcbNeeded);
    }

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        ERR("Insuffisient Buffer size\n");
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Copy over the information.
    pEnd = &pForm[*pcbNeeded];

    ListEntry = FormList.Flink;

    while ( ListEntry != &FormList )
    {
        pfil = CONTAINING_RECORD(ListEntry, FORM_INFO_LIST, List);
        ListEntry = ListEntry->Flink;

        pfnGetFormLevels[Level](pfil, &pForm, &pEnd, NULL);
        (*pcReturned)++;
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalGetForm(HANDLE hPrinter, PWSTR pFormName, DWORD Level, PBYTE pForm, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PFORM_INFO_LIST pfil;
    PBYTE pEnd = &pForm[cbBuf];
    PLOCAL_HANDLE pHandle;

    FIXME("GetForm(%p, %S, %lu, %p, %lu, %p)\n", hPrinter, pFormName, Level, pForm, cbBuf, pcbNeeded);

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Only support 1 & 2
    if (Level < 1 || Level > 2)
    {
        // The caller supplied an invalid level.
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    pfil = FindForm( pFormName, NULL );
    if ( !pfil )
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Count the required buffer size.
    *pcbNeeded = 0;

    pfnGetFormLevels[Level](pfil, NULL, NULL, pcbNeeded);

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        ERR("Insuffisient Buffer size\n");
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Copy over the information.
    pEnd = &pForm[*pcbNeeded];

    pfnGetFormLevels[Level](pfil, &pForm, &pEnd, NULL);

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalSetForm(HANDLE hPrinter, PWSTR pFormName, DWORD Level, PBYTE pForm)
{
    DWORD dwErrorCode, cbNeeded;
    PFORM_INFO_LIST pfil;
    REGISTRYFORMINFO rfi;
    PLOCAL_HANDLE pHandle;
    PFORM_INFO_1W pfi1w = (PFORM_INFO_1W)pForm;
    PFORM_INFO_2W pfi2w = (PFORM_INFO_2W)pForm;

    FIXME("SetFormW(%p, %S, %lu, %p)\n", hPrinter, pFormName, Level, pForm);

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Only support 1 & 2
    if (Level < 1 || Level > 2)
    {
        // The caller supplied an invalid level.
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    pfil = FindForm( pFormName, NULL );
    if ( !pfil )
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    dwErrorCode = RegQueryValueExW( hFormsKey, pFormName, NULL, NULL, (PBYTE)&rfi, &cbNeeded) ;
    if ( dwErrorCode != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    pfil->Flags         = pfi1w->Flags;
    pfil->Size          = pfi1w->Size;
    pfil->ImageableArea = pfi1w->ImageableArea;

    if ( Level > 1 )
    {
        pfil->pKeyword     = pfi2w->pKeyword;
        pfil->pMuiDll      = pfi2w->pMuiDll;
        pfil->pDisplayName = pfi2w->pDisplayName;
        pfil->StringType   = pfi2w->StringType;
        pfil->dwResourceId = pfi2w->dwResourceId;
    }

    rfi.Size          = pfil->Size;
    rfi.ImageableArea = pfil->ImageableArea;
    rfi.Flags         = pfil->Flags;

    dwErrorCode = RegSetValueExW( hFormsKey, pfil->pName, 0, REG_BINARY, (PBYTE)&rfi, sizeof( rfi ) );

    BroadcastChange(pHandle);

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
