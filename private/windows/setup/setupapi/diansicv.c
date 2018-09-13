/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    diansicv.c

Abstract:

    Routine to convert device installer data structures between
    ANSI and Unicode.

    The contents of this file are compiled only when UNICODE
    is #define'd.

Author:

    Ted Miller (tedm) 19-July-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#ifdef UNICODE

DWORD
pSetupDiDevInstParamsAnsiToUnicode(
    IN  PSP_DEVINSTALL_PARAMS_A AnsiDevInstParams,
    OUT PSP_DEVINSTALL_PARAMS_W UnicodeDevInstParams
    )

/*++

Routine Description:

    This routine converts an SP_DEVINSTALL_PARAMS_A structure to
    an SP_DEVINSTALL_PARAMS_W, guarding against bogus pointers
    passed in the by the caller.

Arguments:

    AnsiDevInstParams - supplies ANSI device installation parameters
        to be converted to unicode.

    UnicodeDevInstParams - if successful, receives Unicode equivalent of
        AnsiDevInstParams.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    It is not believed that, given valid arguments, text conversion itself
    can fail, since all ANSI chars always have Unicode equivalents.

--*/

{
    int i;
    DWORD rc;

    rc = NO_ERROR;

    try {

        if(AnsiDevInstParams->cbSize == sizeof(SP_DEVINSTALL_PARAMS_A)) {

            //
            // Fixed part of structure.
            //
            MYASSERT(offsetof(SP_DEVINSTALL_PARAMS_A,DriverPath) == offsetof(SP_DEVINSTALL_PARAMS_W,DriverPath));

            CopyMemory(
                UnicodeDevInstParams,
                AnsiDevInstParams,
                offsetof(SP_DEVINSTALL_PARAMS_W,DriverPath)
                );

            UnicodeDevInstParams->cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);

            //
            // Convert the single string in the structure. To make things easier
            // we'll just convert the entire buffer. There's no potential for overflow.
            //
            i = MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    AnsiDevInstParams->DriverPath,
                    sizeof(AnsiDevInstParams->DriverPath),
                    UnicodeDevInstParams->DriverPath,
                    sizeof(UnicodeDevInstParams->DriverPath) / sizeof(WCHAR)
                    );

            if(!i) {
                rc = GetLastError();
            }
        } else {
            rc = ERROR_INVALID_USER_BUFFER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    return(rc);
}


DWORD
pSetupDiDevInstParamsUnicodeToAnsi(
    IN  PSP_DEVINSTALL_PARAMS_W UnicodeDevInstParams,
    OUT PSP_DEVINSTALL_PARAMS_A AnsiDevInstParams
    )

/*++

Routine Description:

    This routine converts an SP_DEVINSTALL_PARAMS_W structure to
    an SP_DEVINSTALL_PARAMS_A, guarding against bogus pointers
    passed in the by the caller.

Arguments:

    UnicodeDevInstParams - supplies Unicode device installation parameters
        to be converted to ANSI.

    AnsiDevInstParams - if successful, receives Ansi equivalent of
        UnicodeDevInstParams.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    Unicode chars that can't be represented in the current system ANSI codepage
    will be replaced with a system default in the ANSI structure.

--*/

{
    int i;
    DWORD rc;
    UCHAR AnsiString[MAX_PATH*2];

    MYASSERT(UnicodeDevInstParams->cbSize == sizeof(SP_DEVINSTALL_PARAMS_W));

    rc = NO_ERROR;

    try {

        if(AnsiDevInstParams->cbSize == sizeof(SP_DEVINSTALL_PARAMS_A)) {
            //
            // Fixed part of structure.
            //
            MYASSERT(offsetof(SP_DEVINSTALL_PARAMS_A,DriverPath) == offsetof(SP_DEVINSTALL_PARAMS_W,DriverPath));

            CopyMemory(
                AnsiDevInstParams,
                UnicodeDevInstParams,
                offsetof(SP_DEVINSTALL_PARAMS_W,DriverPath)
                );

            AnsiDevInstParams->cbSize = sizeof(SP_DEVINSTALL_PARAMS_A);

            //
            // Convert the single string in the structure. Unfortunately there is
            // potential for overflow because some Unicode chars could convert to
            // double-byte ANSI characters -- but the string in the ANSI structure
            // is only MAX_PATH *bytes* (not MAX_PATH double-byte *characters*) long.
            //
            i = WideCharToMultiByte(
                    CP_ACP,
                    0,
                    UnicodeDevInstParams->DriverPath,
                    sizeof(UnicodeDevInstParams->DriverPath) / sizeof(WCHAR),
                    AnsiString,
                    sizeof(AnsiString),
                    NULL,
                    NULL
                    );

            if(i) {
                //
                // Copy converted string into caller's structure, limiting
                // its length to avoid overflow.
                //
                if(!lstrcpynA(AnsiDevInstParams->DriverPath,AnsiString,sizeof(AnsiDevInstParams->DriverPath))) {
                    //
                    // lstrcpyn faulted, a pointer must be bogus
                    //
                    rc = ERROR_INVALID_PARAMETER;
                }
            } else {
                rc = GetLastError();
            }
        } else {
            rc = ERROR_INVALID_USER_BUFFER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    return(rc);
}


DWORD
pSetupDiSelDevParamsAnsiToUnicode(
    IN  PSP_SELECTDEVICE_PARAMS_A AnsiSelDevParams,
    OUT PSP_SELECTDEVICE_PARAMS_W UnicodeSelDevParams
    )

/*++

Routine Description:

    This routine converts an SP_SELECTDEVICE_PARAMS_A structure to
    an SP_SELECTDEVICE_PARAMS_W, guarding against bogus pointers
    passed in the by the caller.

Arguments:

    AnsiSelDevParams - supplies ANSI device selection parameters
        to be converted to unicode.

    UnicodeSelDevParams - if successful, receives Unicode equivalent of
        AnsiSelDevParams.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    It is not believed that, given valid arguments, text conversion itself
    can fail, since all ANSI chars always have Unicode equivalents.

--*/

{
    int i;
    DWORD rc;

    rc = NO_ERROR;

    try {
        if(AnsiSelDevParams->ClassInstallHeader.cbSize == sizeof(SP_CLASSINSTALL_HEADER)) {
            //
            // Fixed part of structure.
            //
            MYASSERT(offsetof(SP_SELECTDEVICE_PARAMS_A,Title) == offsetof(SP_SELECTDEVICE_PARAMS_W,Title));

            CopyMemory(
                UnicodeSelDevParams,
                AnsiSelDevParams,
                offsetof(SP_SELECTDEVICE_PARAMS_W,Title)
                );

            //
            // Convert the strings in the structure. To make things easier
            // we'll just convert the entire buffers. There's no potential for overflow.
            //
            i = MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    AnsiSelDevParams->Title,
                    sizeof(AnsiSelDevParams->Title),
                    UnicodeSelDevParams->Title,
                    sizeof(UnicodeSelDevParams->Title) / sizeof(WCHAR)
                    );

            if(i) {
                i = MultiByteToWideChar(
                        CP_ACP,
                        MB_PRECOMPOSED,
                        AnsiSelDevParams->Instructions,
                        sizeof(AnsiSelDevParams->Instructions),
                        UnicodeSelDevParams->Instructions,
                        sizeof(UnicodeSelDevParams->Instructions) / sizeof(WCHAR)
                        );

                if(i) {
                    i = MultiByteToWideChar(
                            CP_ACP,
                            MB_PRECOMPOSED,
                            AnsiSelDevParams->ListLabel,
                            sizeof(AnsiSelDevParams->ListLabel),
                            UnicodeSelDevParams->ListLabel,
                            sizeof(UnicodeSelDevParams->ListLabel) / sizeof(WCHAR)
                            );
                    if(i) {
                        i = MultiByteToWideChar(
                                CP_ACP,
                                MB_PRECOMPOSED,
                                AnsiSelDevParams->SubTitle,
                                sizeof(AnsiSelDevParams->SubTitle),
                                UnicodeSelDevParams->SubTitle,
                                sizeof(UnicodeSelDevParams->SubTitle) / sizeof(WCHAR)
                                );

                        if(!i) {
                            rc = GetLastError();
                        }
                    } else {
                        rc = GetLastError();
                    }
                } else {
                    rc = GetLastError();
                }
            } else {
                rc = GetLastError();
            }
        } else {
            rc = ERROR_INVALID_USER_BUFFER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    return(rc);
}


DWORD
pSetupDiSelDevParamsUnicodeToAnsi(
    IN  PSP_SELECTDEVICE_PARAMS_W UnicodeSelDevParams,
    OUT PSP_SELECTDEVICE_PARAMS_A AnsiSelDevParams
    )

/*++

Routine Description:

    This routine converts an SP_SELECTDEVICE_PARAMS_W structure to
    an SP_SELECTDEVICE_PARAMS_A, guarding against bogus pointers
    passed in the by the caller.

Arguments:

    UnicodeSelDevParams - supplies Unicode device selection parameters
        to be converted to ANSI.

    AnsiSelDevParams - if successful, receives Ansi equivalent of
        UnicodeSelDevParams.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    Unicode chars that can't be represented in the current system ANSI codepage
    will be replaced with a system default in the ANSI structure.

--*/

{
    int i;
    DWORD rc;
    UCHAR AnsiTitle[MAX_TITLE_LEN*2];
    UCHAR AnsiInstructions[MAX_INSTRUCTION_LEN*2];
    UCHAR AnsiListLabel[MAX_LABEL_LEN*2];
    UCHAR AnsiSubTitle[MAX_SUBTITLE_LEN*2];
    PVOID p;

    MYASSERT(UnicodeSelDevParams->ClassInstallHeader.cbSize == sizeof(SP_CLASSINSTALL_HEADER));

    rc = NO_ERROR;

    try {

        if(AnsiSelDevParams->ClassInstallHeader.cbSize == sizeof(SP_CLASSINSTALL_HEADER)) {
            //
            // Fixed part of structure.
            //
            MYASSERT(offsetof(SP_SELECTDEVICE_PARAMS_A,Title) == offsetof(SP_SELECTDEVICE_PARAMS_W,Title));

            CopyMemory(
                AnsiSelDevParams,
                UnicodeSelDevParams,
                offsetof(SP_SELECTDEVICE_PARAMS_W,Title)
                );

            ZeroMemory(AnsiSelDevParams->Reserved,sizeof(AnsiSelDevParams->Reserved));

            //
            // Convert the strings in the structure. Unfortunately there is
            // potential for overflow because some Unicode chars could convert to
            // double-byte ANSI characters -- but the strings in the ANSI structure
            // are sized in *bytes* (not double-byte *characters*).
            //
            i = WideCharToMultiByte(
                    CP_ACP,
                    0,
                    UnicodeSelDevParams->Title,
                    sizeof(UnicodeSelDevParams->Title) / sizeof(WCHAR),
                    AnsiTitle,
                    sizeof(AnsiTitle),
                    NULL,
                    NULL
                    );

            if(i) {

                i = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        UnicodeSelDevParams->Instructions,
                        sizeof(UnicodeSelDevParams->Instructions) / sizeof(WCHAR),
                        AnsiInstructions,
                        sizeof(AnsiInstructions),
                        NULL,
                        NULL
                        );

                if(i) {

                    i = WideCharToMultiByte(
                            CP_ACP,
                            0,
                            UnicodeSelDevParams->ListLabel,
                            sizeof(UnicodeSelDevParams->ListLabel) / sizeof(WCHAR),
                            AnsiListLabel,
                            sizeof(AnsiListLabel),
                            NULL,
                            NULL
                            );

                    if(i) {

                        i = WideCharToMultiByte(
                                CP_ACP,
                                0,
                                UnicodeSelDevParams->SubTitle,
                                sizeof(UnicodeSelDevParams->SubTitle) / sizeof(WCHAR),
                                AnsiSubTitle,
                                sizeof(AnsiSubTitle),
                                NULL,
                                NULL
                                );
                        if(i) {
                            //
                            // Copy converted strings into caller's structure, limiting
                            // lengths to avoid overflow. If any lstrcpynA call returns NULL
                            // then it faulted meaning a pointer is bad.
                            //
                            #undef CPYANS
                            #define CPYANS(field) lstrcpynA(AnsiSelDevParams->field,Ansi##field,sizeof(AnsiSelDevParams->field))

                            if(!CPYANS(Title) || !CPYANS(Instructions) || !CPYANS(ListLabel)) {
                                rc = ERROR_INVALID_PARAMETER;
                            }
                        } else {
                            rc = GetLastError();
                        }
                    } else {
                        rc = GetLastError();
                    }
                } else {
                    rc = GetLastError();
                }
            } else {
                rc = GetLastError();
            }
        } else {
            rc = ERROR_INVALID_USER_BUFFER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    return(rc);
}


DWORD
pSetupDiDrvInfoDataAnsiToUnicode(
    IN  PSP_DRVINFO_DATA_A AnsiDrvInfoData,
    OUT PSP_DRVINFO_DATA_W UnicodeDrvInfoData
    )

/*++

Routine Description:

    This routine converts an SP_DRVINFO_DATA_A structure to
    an SP_DRVINFO_DATA_W, guarding against bogus pointers
    passed in the by the caller.

Arguments:

    AnsiDrvInfoData - supplies ANSI structure to be converted to unicode.

    UnicodeDrvInfoData - if successful, receives Unicode equivalent of
        AnsiDrvInfoData.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    It is not believed that, given valid arguments, text conversion itself
    can fail, since all ANSI chars always have Unicode equivalents.

--*/

{
    int i;
    DWORD rc;

    rc = NO_ERROR;

    try {
        if((AnsiDrvInfoData->cbSize == sizeof(SP_DRVINFO_DATA_A)) ||
           (AnsiDrvInfoData->cbSize == sizeof(SP_DRVINFO_DATA_V1_A))) {
            //
            // Fixed part of structure.
            //
            MYASSERT(offsetof(SP_DRVINFO_DATA_A,Description) == offsetof(SP_DRVINFO_DATA_W,Description));

            ZeroMemory(UnicodeDrvInfoData, sizeof(SP_DRVINFO_DATA_W));

            CopyMemory(
                UnicodeDrvInfoData,
                AnsiDrvInfoData,
                offsetof(SP_DRVINFO_DATA_W,Description)
                );

            UnicodeDrvInfoData->cbSize = sizeof(SP_DRVINFO_DATA_W);

            //
            // Convert the strings in the structure. To make things easier
            // we'll just convert the entire buffers. There's no potential for overflow.
            //
            i = MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    AnsiDrvInfoData->Description,
                    sizeof(AnsiDrvInfoData->Description),
                    UnicodeDrvInfoData->Description,
                    sizeof(UnicodeDrvInfoData->Description) / sizeof(WCHAR)
                    );

            if(i) {
                i = MultiByteToWideChar(
                        CP_ACP,
                        MB_PRECOMPOSED,
                        AnsiDrvInfoData->MfgName,
                        sizeof(AnsiDrvInfoData->MfgName),
                        UnicodeDrvInfoData->MfgName,
                        sizeof(UnicodeDrvInfoData->MfgName) / sizeof(WCHAR)
                        );

                if(i) {
                    i = MultiByteToWideChar(
                            CP_ACP,
                            MB_PRECOMPOSED,
                            AnsiDrvInfoData->ProviderName,
                            sizeof(AnsiDrvInfoData->ProviderName),
                            UnicodeDrvInfoData->ProviderName,
                            sizeof(UnicodeDrvInfoData->ProviderName) / sizeof(WCHAR)
                            );

                    if(i) {
                        //
                        // Successfully converted all strings to unicode.  Set
                        // the final two fields (DriverDate and DriverVersion)
                        // unless the caller supplied us with a version 1
                        // structure.
                        //
                        if(AnsiDrvInfoData->cbSize == sizeof(SP_DRVINFO_DATA_A)) {
                            UnicodeDrvInfoData->DriverDate = AnsiDrvInfoData->DriverDate;
                            UnicodeDrvInfoData->DriverVersion = AnsiDrvInfoData->DriverVersion;
                        }

                    } else {
                        rc = GetLastError();
                    }
                } else {
                    rc = GetLastError();
                }
            } else {
                rc = GetLastError();
            }
        } else {
            rc = ERROR_INVALID_USER_BUFFER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    return(rc);
}


DWORD
pSetupDiDrvInfoDataUnicodeToAnsi(
    IN  PSP_DRVINFO_DATA_W UnicodeDrvInfoData,
    OUT PSP_DRVINFO_DATA_A AnsiDrvInfoData
    )

/*++

Routine Description:

    This routine converts an SP_DRVINFO_DATA_W structure to
    an SP_DRVINFO_DATA_A, guarding against bogus pointers
    passed in the by the caller.

Arguments:

    UnicodeDrvInfoData - supplies Unicode structure to be converted
        to ANSI.

    AnsiDrvInfoData - if successful, receives Ansi equivalent of
        UnicodeDrvInfoData.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    Unicode chars that can't be represented in the current system ANSI codepage
    will be replaced with a system default in the ANSI structure.

--*/

{
    int i;
    DWORD rc;
    UCHAR AnsiDescription[LINE_LEN*2];
    UCHAR AnsiMfgName[LINE_LEN*2];
    UCHAR AnsiProviderName[LINE_LEN*2];
    PVOID p;

    MYASSERT(UnicodeDrvInfoData->cbSize == sizeof(SP_DRVINFO_DATA_W));

    rc = NO_ERROR;

    try {

        if((AnsiDrvInfoData->cbSize == sizeof(SP_DRVINFO_DATA_A)) ||
           (AnsiDrvInfoData->cbSize == sizeof(SP_DRVINFO_DATA_V1_A))) {

            //
            // Copy over the DriverType and the Reserved field.
            //
            AnsiDrvInfoData->DriverType = UnicodeDrvInfoData->DriverType;
            AnsiDrvInfoData->Reserved = UnicodeDrvInfoData->Reserved;

            //
            // Convert the strings in the structure. Unfortunately there is
            // potential for overflow because some Unicode chars could convert to
            // double-byte ANSI characters -- but the strings in the ANSI structure
            // are sized in *bytes* (not double-byte *characters*).
            //
            i = WideCharToMultiByte(
                    CP_ACP,
                    0,
                    UnicodeDrvInfoData->Description,
                    sizeof(UnicodeDrvInfoData->Description) / sizeof(WCHAR),
                    AnsiDescription,
                    sizeof(AnsiDescription),
                    NULL,
                    NULL
                    );

            if(i) {

                i = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        UnicodeDrvInfoData->MfgName,
                        sizeof(UnicodeDrvInfoData->MfgName) / sizeof(WCHAR),
                        AnsiMfgName,
                        sizeof(AnsiMfgName),
                        NULL,
                        NULL
                        );

                if(i) {

                    i = WideCharToMultiByte(
                            CP_ACP,
                            0,
                            UnicodeDrvInfoData->ProviderName,
                            sizeof(UnicodeDrvInfoData->ProviderName) / sizeof(WCHAR),
                            AnsiProviderName,
                            sizeof(AnsiProviderName),
                            NULL,
                            NULL
                            );

                    if(i) {
                        //
                        // Copy converted strings into caller's structure, limiting
                        // lengths to avoid overflow. If any lstrcpynA call returns NULL
                        // then it faulted meaning a pointer is bad.
                        //
                        #undef CPYANS
                        #define CPYANS(field) lstrcpynA(AnsiDrvInfoData->field,Ansi##field,sizeof(AnsiDrvInfoData->field))

                        if(!CPYANS(Description) || !CPYANS(MfgName) || !CPYANS(ProviderName)) {
                            rc = ERROR_INVALID_PARAMETER;
                        } else {
                            //
                            // Successfully converted/transferred all the
                            // unicode strings back to ANSI.  Now, set the
                            // final two fields (DriverDate and DriverVersion)
                            // unless the caller supplied us with a version 1
                            // structure.
                            //
                            if(AnsiDrvInfoData->cbSize == sizeof(SP_DRVINFO_DATA_A)) {
                                AnsiDrvInfoData->DriverDate = UnicodeDrvInfoData->DriverDate;
                                AnsiDrvInfoData->DriverVersion = UnicodeDrvInfoData->DriverVersion;
                            }
                        }
                    } else {
                        rc = GetLastError();
                    }
                } else {
                    rc = GetLastError();
                }
            } else {
                rc = GetLastError();
            }
        } else {
            rc = ERROR_INVALID_USER_BUFFER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    return(rc);
}


DWORD
pSetupDiDevInfoSetDetailDataUnicodeToAnsi(
    IN  PSP_DEVINFO_LIST_DETAIL_DATA_W UnicodeDevInfoSetDetails,
    OUT PSP_DEVINFO_LIST_DETAIL_DATA_A AnsiDevInfoSetDetails
    )

/*++

Routine Description:

    This routine converts an SP_DEVINFO_LIST_DETAIL_DATA_W structure
    to an SP_DEVINFO_LIST_DETAIL_DATA_A, guarding against bogus pointers
    passed in the by the caller.

Arguments:

    UnicodeDevInfoSetDetails - supplies Unicode structure to be converted
        to ANSI.

    AnsiDevInfoSetDetails - if successful, receives Ansi equivalent of
        UnicodeDevInfoSetDetails.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    Unicode chars that can't be represented in the current system ANSI codepage
    will be replaced with a system default in the ANSI structure.

--*/

{
    int i;
    DWORD rc;
    UCHAR AnsiRemoteMachineName[SP_MAX_MACHINENAME_LENGTH * 2];
    PVOID p;

    MYASSERT(UnicodeDevInfoSetDetails->cbSize == sizeof(SP_DEVINFO_LIST_DETAIL_DATA_W));

    rc = NO_ERROR;

    try {

        if(AnsiDevInfoSetDetails->cbSize == sizeof(SP_DEVINFO_LIST_DETAIL_DATA_A)) {
            //
            // Fixed part of structure.
            //
            MYASSERT(offsetof(SP_DEVINFO_LIST_DETAIL_DATA_A, RemoteMachineName) ==
                     offsetof(SP_DEVINFO_LIST_DETAIL_DATA_W, RemoteMachineName)
                    );

            CopyMemory(AnsiDevInfoSetDetails,
                       UnicodeDevInfoSetDetails,
                       offsetof(SP_DEVINFO_LIST_DETAIL_DATA_W, RemoteMachineName)
                      );

            AnsiDevInfoSetDetails->cbSize = sizeof(SP_DEVINFO_LIST_DETAIL_DATA_A);

            //
            // Convert the strings in the structure. Unfortunately there is
            // potential for overflow because some Unicode chars could convert to
            // double-byte ANSI characters -- but the strings in the ANSI structure
            // are sized in *bytes* (not double-byte *characters*).
            //
            i = WideCharToMultiByte(CP_ACP,
                                    0,
                                    UnicodeDevInfoSetDetails->RemoteMachineName,
                                    SIZECHARS(UnicodeDevInfoSetDetails->RemoteMachineName),
                                    AnsiRemoteMachineName,
                                    sizeof(AnsiRemoteMachineName),
                                    NULL,
                                    NULL
                                   );

            if(i) {
                //
                // Copy converted string into caller's structure, limiting
                // lengths to avoid overflow. If any lstrcpynA call returns NULL
                // then it faulted meaning a pointer is bad.
                //
                if(!lstrcpynA(AnsiDevInfoSetDetails->RemoteMachineName,
                              AnsiRemoteMachineName,
                              sizeof(AnsiDevInfoSetDetails->RemoteMachineName))) {

                    rc = ERROR_INVALID_PARAMETER;
                }
            } else {
                rc = GetLastError();
            }
        } else {
            rc = ERROR_INVALID_USER_BUFFER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    return rc;
}

#endif // def UNICODE

