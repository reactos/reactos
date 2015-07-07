/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/country.h
 * PURPOSE:         DOS32 Country support
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE:            Support for default (english) language only.
 *                  For other languages, please use COUNTRY.SYS
 */

#ifndef _COUNTRY_H_
#define _COUNTRY_H_

/* DEFINITIONS ****************************************************************/

#pragma pack(push, 1)

#if 0 // Keep here for information purposes only
// DOS 2.00-2.10 country info structure
typedef struct _DOS_COUNTRY_INFO_OLD
{
    WORD DateTimeFormat;
    CHAR CurrencySymbol[2];
    CHAR ThousandSep[2];
    CHAR DecimalSep[2];
    BYTE Reserved[24];
} DOS_COUNTRY_INFO_OLD, *PDOS_COUNTRY_INFO_OLD;
C_ASSERT(sizeof(DOS_COUNTRY_INFO_OLD) == 0x20);
#endif

// DOS 2.11+ compatible country info structure
typedef struct _DOS_COUNTRY_INFO
{
    WORD DateTimeFormat;
    CHAR CurrencySymbol[5];
    CHAR ThousandSep[2];
    CHAR DecimalSep[2];
    CHAR DateSep[2];
    CHAR TimeSep[2];
    BYTE CurrencyFormat;
    BYTE CurrencyDigitsNum;
    BYTE TimeFormat;
    DWORD CaseMapPtr;
    CHAR DataListSep[2];
    BYTE Reserved[10];
} DOS_COUNTRY_INFO, *PDOS_COUNTRY_INFO;
C_ASSERT(sizeof(DOS_COUNTRY_INFO) == 0x22);

typedef struct _DOS_COUNTRY_INFO_EX
{
    WORD Size;
    WORD CountryId;
    WORD CodePage;
    DOS_COUNTRY_INFO CountryInfo;
} DOS_COUNTRY_INFO_EX, *PDOS_COUNTRY_INFO_EX;
C_ASSERT(sizeof(DOS_COUNTRY_INFO_EX) == 0x28);

typedef struct _DOS_COUNTRY_INFO_2
{
    BYTE InfoId;
    union
    {
        DOS_COUNTRY_INFO_EX CountryInfoEx;

        DWORD UpCaseTblPtr;
        DWORD LoCaseTblPtr;

        DWORD FNameUpCaseTblPtr;
        DWORD FNameTermTblPtr;

        DWORD CollateTblPtr;
        DWORD DBCSLeadTblPtr;
    };
} DOS_COUNTRY_INFO_2, *PDOS_COUNTRY_INFO_2;

#pragma pack(pop)

/* FUNCTIONS ******************************************************************/

WORD
DosGetCountryInfo(IN OUT PWORD CountryId,
                  OUT PDOS_COUNTRY_INFO CountryInfo);

WORD
DosGetCountryInfoEx(IN BYTE InfoId,
                    IN WORD CodePage,
                    IN WORD CountryId,
                    OUT PDOS_COUNTRY_INFO_2 CountryInfo,
                    IN OUT PWORD BufferSize);

WORD DosIfCharYesNo(WORD Char);
CHAR DosToUpper(CHAR Char);
VOID DosToUpperStrN(PCHAR DestStr, PCHAR SrcStr, WORD Length);
VOID DosToUpperStrZ(PSTR DestStr, PSTR SrcStr);

BOOLEAN DosCountryInitialize(VOID);

#endif
