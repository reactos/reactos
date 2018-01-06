/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS GUI first stage setup application
 * FILE:            base/setup/lib/infsupp.c
 * PURPOSE:         Interfacing with Setup* API .INF Files support functions
 * PROGRAMMERS:     Hervé Poussineau
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES ******************************************************************/

#include "reactos.h"

#define NDEBUG
#include <debug.h>

/* SETUP* API COMPATIBILITY FUNCTIONS ****************************************/

/*
 * This function corresponds to an undocumented but exported SetupAPI function
 * that exists since WinNT4 and is still present in Win10.
 * The returned string pointer is a read-only pointer to a string in the
 * maintained INF cache, and is always in UNICODE (on NT systems).
 */
PCWSTR
WINAPI
pSetupGetField(
    IN PINFCONTEXT Context,
    IN ULONG FieldIndex);

/* SetupOpenInfFileW with support for a user-provided LCID */
HINF
WINAPI
SetupOpenInfFileExW(
    IN PCWSTR FileName,
    IN PCWSTR InfClass,
    IN DWORD InfStyle,
    IN LCID LocaleId,
    OUT PUINT ErrorLine)
{
    WCHAR Win32FileName[MAX_PATH];

    UNREFERENCED_PARAMETER(LocaleId);

    /*
     * SetupOpenInfFileExW is called within setuplib with NT paths, however
     * the Win32 SetupOpenInfFileW API only takes Win32 paths. We therefore
     * map the NT path to Win32 path and then call the Win32 API.
     */
    if (!ConvertNtPathToWin32Path(Win32FileName,
                                  _countof(Win32FileName),
                                  FileName))
    {
        return INVALID_HANDLE_VALUE;
    }

    return SetupOpenInfFileW(Win32FileName,
                             InfClass,
                             InfStyle,
                             ErrorLine);
}


/* GLOBALS *******************************************************************/

pSpInfCloseInfFile  SpInfCloseInfFile  = SetupCloseInfFile;
pSpInfFindFirstLine SpInfFindFirstLine = SetupFindFirstLineW;
pSpInfFindNextLine  SpInfFindNextLine  = SetupFindNextLine;
pSpInfGetFieldCount SpInfGetFieldCount = SetupGetFieldCount;
pSpInfGetBinaryField  SpInfGetBinaryField  = SetupGetBinaryField;
pSpInfGetIntField     SpInfGetIntField     = SetupGetIntField;
pSpInfGetMultiSzField SpInfGetMultiSzField = SetupGetMultiSzFieldW;
pSpInfGetStringField  SpInfGetStringField  = SetupGetStringFieldW;
pSpInfGetField    SpInfGetField    = pSetupGetField;
pSpInfOpenInfFile SpInfOpenInfFile = SetupOpenInfFileExW;


/* HELPER FUNCTIONS **********************************************************/

#if 0

HINF WINAPI
INF_OpenBufferedFileA(
    IN PSTR FileBuffer,
    IN ULONG FileSize,
    IN PCSTR InfClass,
    IN DWORD InfStyle,
    IN LCID LocaleId,
    OUT PUINT ErrorLine)
{
#ifdef __REACTOS__
    HINF hInf = NULL;
    ULONG ErrorLineUL;
    NTSTATUS Status;

    Status = InfOpenBufferedFile(&hInf,
                                 FileBuffer,
                                 FileSize,
                                 LANGIDFROMLCID(LocaleId),
                                 &ErrorLineUL);
    *ErrorLine = (UINT)ErrorLineUL;
    if (!NT_SUCCESS(Status))
        return INVALID_HANDLE_VALUE;

    return hInf;
#else
    return INVALID_HANDLE_VALUE;
#endif /* !__REACTOS__ */
}

#endif

/* EOF */
