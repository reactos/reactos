/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/country.c
 * PURPOSE:         DOS32 Country support
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE:            Support for default (english) language only.
 *                  For other languages, please use COUNTRY.SYS
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"

#include "country.h"
#include "memory.h"

/* PRIVATE VARIABLES **********************************************************/

/* CaseMap routine: should call INT 65h, AL=20h */
// ATM, just do nothing.
static const BYTE CaseMapRoutine[] =
{
    0xCB // retf
};

#pragma pack(push, 1)

#define DATATABLE(name, type, len)   \
    typedef struct _##name      \
    {                           \
        WORD Size;              \
        type Data[(len)];       \
    } name

DATATABLE(UPPERCASE, CHAR, 0xFF-0x80+1);
DATATABLE(LOWERCASE, CHAR, 0xFF-0x00+1);
DATATABLE(FNAMETERM, BYTE,          22);
DATATABLE(COLLATE  , BYTE, 0xFF-0x00+1);
DATATABLE(DBCSLEAD , WORD,      0x00+1);

typedef struct _COUNTRY_DATA
{
    BYTE      CaseMapRoutine[sizeof(CaseMapRoutine)];
    UPPERCASE UpCaseTbl;    // Used also for filename uppercase
    LOWERCASE LoCaseTbl;
    FNAMETERM FNameTermTbl;
    COLLATE   CollateTbl;
    DBCSLEAD  DBCSLeadTbl;
} COUNTRY_DATA, *PCOUNTRY_DATA;

#pragma pack(pop)

/* Global data contained in guest memory */
static WORD CountryDataSegment;
static PCOUNTRY_DATA CountryData;

WORD YesNoTable[2] = { MAKEWORD('Y', 0), MAKEWORD('N', 0) };

/*
 * See: http://www.ctyme.com/intr/rb-3163.htm#Table1754
 *      http://www.ctyme.com/intr/rb-3164.htm
 *      http://www.ctyme.com/intr/rb-3166.htm
 */

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

WORD
DosGetCountryInfo(IN OUT PWORD CountryId,
                  OUT PDOS_COUNTRY_INFO CountryInfo)
{
    INT Return;
    DWORD NumVal;

    Return = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IDATE | LOCALE_RETURN_NUMBER, // LOCALE_ILDATE | LOCALE_RETURN_NUMBER
                            (LPSTR)&NumVal,
                            sizeof(NumVal));
    if (Return == 0) return LOWORD(GetLastError());
    CountryInfo->DateTimeFormat = (WORD)NumVal;

    Return = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY,
                            (LPSTR)&CountryInfo->CurrencySymbol,
                            sizeof(CountryInfo->CurrencySymbol));
    if (Return == 0) return LOWORD(GetLastError());

    Return = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND,
                            (LPSTR)&CountryInfo->ThousandSep,
                            sizeof(CountryInfo->ThousandSep));
    if (Return == 0) return LOWORD(GetLastError());

    Return = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                            (LPSTR)&CountryInfo->DecimalSep,
                            sizeof(CountryInfo->DecimalSep));
    if (Return == 0) return LOWORD(GetLastError());
    
    Return = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDATE,
                            (LPSTR)&CountryInfo->DateSep,
                            sizeof(CountryInfo->DateSep));
    if (Return == 0) return LOWORD(GetLastError());
    
    Return = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STIME,
                            (LPSTR)&CountryInfo->TimeSep,
                            sizeof(CountryInfo->TimeSep));
    if (Return == 0) return LOWORD(GetLastError());
    
    // NOTE: '4: Symbol replace decimal separator' is unsupported.
    Return = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ICURRENCY | LOCALE_RETURN_NUMBER,
                            (LPSTR)&NumVal,
                            sizeof(NumVal));
    if (Return == 0) return LOWORD(GetLastError());
    CountryInfo->CurrencyFormat = (BYTE)NumVal;
    
    Return = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ICURRDIGITS | LOCALE_RETURN_NUMBER, // LOCALE_IDIGITS | LOCALE_RETURN_NUMBER
                            (LPSTR)&NumVal,
                            sizeof(NumVal));
    if (Return == 0) return LOWORD(GetLastError());
    CountryInfo->CurrencyDigitsNum = (BYTE)NumVal;
    
    Return = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ITIME | LOCALE_RETURN_NUMBER,
                            (LPSTR)&NumVal,
                            sizeof(NumVal));
    if (Return == 0) return LOWORD(GetLastError());
    CountryInfo->TimeFormat = (BYTE)NumVal;

    CountryInfo->CaseMapPtr = MAKELONG(FIELD_OFFSET(COUNTRY_DATA, CaseMapRoutine), CountryDataSegment);
    
    // CountryInfo->DataListSep;

    return ERROR_SUCCESS;
}

WORD
DosGetCountryInfoEx(IN BYTE InfoId,
                    IN WORD CodePage,
                    IN WORD CountryId,
                    OUT PDOS_COUNTRY_INFO_2 CountryInfo,
                    IN OUT PWORD BufferSize)
{
    // FIXME: use: CodePage; CountryId
    // FIXME: Check BufferSize
    // FIXME: Use NLSFUNC resident?

    switch (InfoId)
    {
        /* Get General Internationalization Info (similar to AX=3800h) */
        case 0x01:
        {
            WORD ErrorCode;
            ErrorCode = DosGetCountryInfo(&CountryId,
                                          &CountryInfo->CountryInfoEx.CountryInfo);
            if (ErrorCode != ERROR_SUCCESS) return ErrorCode;
            CountryInfo->CountryInfoEx.Size = sizeof(CountryInfo->CountryInfoEx);
            CountryInfo->CountryInfoEx.CountryId = CountryId;
            // CountryInfo->CodePage;
            break;
        }

        /* Get Pointer to Uppercase Table */
        case 0x02:
            CountryInfo->UpCaseTblPtr = MAKELONG(FIELD_OFFSET(COUNTRY_DATA, UpCaseTbl), CountryDataSegment);
            break;

        /* Get Pointer to Lowercase Table */
        case 0x03:
            CountryInfo->LoCaseTblPtr = MAKELONG(FIELD_OFFSET(COUNTRY_DATA, LoCaseTbl), CountryDataSegment);
            break;

        /* Get Pointer to Filename Uppercase Table */
        case 0x04:
            CountryInfo->FNameUpCaseTblPtr = MAKELONG(FIELD_OFFSET(COUNTRY_DATA, UpCaseTbl), CountryDataSegment);
            break;

        /* Get Pointer to Filename Terminator Table */
        case 0x05:
            CountryInfo->FNameTermTblPtr = MAKELONG(FIELD_OFFSET(COUNTRY_DATA, FNameTermTbl), CountryDataSegment);
            break;

        /* Get Pointer to Collating Sequence Table */
        case 0x06:
            CountryInfo->CollateTblPtr = MAKELONG(FIELD_OFFSET(COUNTRY_DATA, CollateTbl), CountryDataSegment);
            break;

        /* Get Pointer to Double-Byte Character Set Table */
        case 0x07:
            CountryInfo->DBCSLeadTblPtr = MAKELONG(FIELD_OFFSET(COUNTRY_DATA, DBCSLeadTbl), CountryDataSegment);
            break;

        default:
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
    CountryInfo->InfoId = InfoId;

    return ERROR_SUCCESS;
}

WORD DosIfCharYesNo(WORD Char)
{
    Char = toupper(Char);

    /* NO-type */
    if (Char == YesNoTable[1])
        return 0x0000;
    /* YES-type */
    if (Char == YesNoTable[0])
        return 0x0001;
    /* Unknown type */
        return 0x0002;
}

CHAR DosToUpper(CHAR Char)
{
    // FIXME: Use the current locale
    return toupper(Char);
}

VOID DosToUpperStrN(PCHAR DestStr, PCHAR SrcStr, WORD Length)
{
    while (Length-- > 0)
        *DestStr++ = toupper(*SrcStr++);
}

VOID DosToUpperStrZ(PSTR DestStr, PSTR SrcStr)
{
    while (*SrcStr)
        *DestStr++ = toupper(*SrcStr++);
}

BOOLEAN DosCountryInitialize(VOID)
{
    UINT i;

    /* Initialize some memory to store country information */
    // FIXME: Can we use instead some static area from the DOS data structure??
    CountryDataSegment = DosAllocateMemory(sizeof(COUNTRY_DATA), NULL);
    if (CountryDataSegment == 0) return FALSE;
    CountryData = (PCOUNTRY_DATA)SEG_OFF_TO_PTR(CountryDataSegment, 0x0000);
    
    RtlMoveMemory(CountryData->CaseMapRoutine,
                  CaseMapRoutine,
                  sizeof(CaseMapRoutine));

    CountryData->UpCaseTbl.Size = ARRAYSIZE(CountryData->UpCaseTbl.Data);
    for (i = 0; i < CountryData->UpCaseTbl.Size; ++i)
        CountryData->UpCaseTbl.Data[i] = 0x80 + i;

    CountryData->LoCaseTbl.Size = ARRAYSIZE(CountryData->LoCaseTbl.Data);
    for (i = 0; i < CountryData->LoCaseTbl.Size; ++i)
        CountryData->LoCaseTbl.Data[i] = i;

    CountryData->FNameTermTbl.Size = ARRAYSIZE(CountryData->FNameTermTbl.Data);
    CountryData->FNameTermTbl.Data[ 0] = 0x01; // Dummy Byte
    CountryData->FNameTermTbl.Data[ 1] = 0x00; //  Lowest permissible Character Value for Filename
    CountryData->FNameTermTbl.Data[ 2] = 0xFF; // Highest permissible Character Value for Filename
    CountryData->FNameTermTbl.Data[ 3] = 0x00; // Dummy Byte
    CountryData->FNameTermTbl.Data[ 4] = 0x00; // First excluded Character in Range \ all characters in this
    CountryData->FNameTermTbl.Data[ 5] = 0x20; //  Last excluded Character in Range / range are illegal
    CountryData->FNameTermTbl.Data[ 6] = 0x02; // Dummy Byte
    CountryData->FNameTermTbl.Data[ 7] = 14;   // Number of illegal (terminator) Characters
//  CountryData->FNameTermTbl.Data[ 8] = ".\"/\\[]:|<>+=;,"; // Characters which terminate a Filename
    CountryData->FNameTermTbl.Data[ 8] = '.';
    CountryData->FNameTermTbl.Data[ 9] = '\"';
    CountryData->FNameTermTbl.Data[10] = '/';
    CountryData->FNameTermTbl.Data[11] = '\\';
    CountryData->FNameTermTbl.Data[12] = '[';
    CountryData->FNameTermTbl.Data[13] = ']';
    CountryData->FNameTermTbl.Data[14] = ':';
    CountryData->FNameTermTbl.Data[15] = '|';
    CountryData->FNameTermTbl.Data[16] = '<';
    CountryData->FNameTermTbl.Data[17] = '>';
    CountryData->FNameTermTbl.Data[18] = '+';
    CountryData->FNameTermTbl.Data[19] = '=';
    CountryData->FNameTermTbl.Data[20] = ';';
    CountryData->FNameTermTbl.Data[21] = ',';

    CountryData->CollateTbl.Size = ARRAYSIZE(CountryData->CollateTbl.Data);
    for (i = 0; i < CountryData->LoCaseTbl.Size; ++i)
        CountryData->LoCaseTbl.Data[i] = i;

    CountryData->DBCSLeadTbl.Size = 0; // Empty DBCS table
    CountryData->DBCSLeadTbl.Data[0] = 0x0000;

    return TRUE;
}
