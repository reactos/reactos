#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>


DWORD
_cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    ULONG ErrorParameters[2];
    ULONG ErrorResponse;
    UNICODE_STRING PathName;

    RtlInitUnicodeString(&PathName,L"\\\\??\\O:\\AUTORUN.EXE");
    ErrorResponse = ResponseOk;
    ErrorParameters[0] = (ULONG)&PathName;

    NtRaiseHardError( STATUS_IMAGE_MACHINE_TYPE_MISMATCH_EXE,
                      1,
                      1,
                      ErrorParameters,
                      OptionOk,
                      &ErrorResponse
                    );

    RtlInitUnicodeString(&PathName,L"\\\\??\\O:\\autorun.exe");
    ErrorResponse = ResponseOk;
    ErrorParameters[0] = (ULONG)&PathName;

    NtRaiseHardError( STATUS_IMAGE_MACHINE_TYPE_MISMATCH_EXE,
                      1,
                      1,
                      ErrorParameters,
                      OptionOk,
                      &ErrorResponse
                    );
    return 1;
}
