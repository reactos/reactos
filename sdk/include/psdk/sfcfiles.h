#ifndef __SFCFILES_H
#define __SFCFILES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PROTECT_FILE_ENTRY
{
    PWSTR SourceFileName;
    PWSTR FileName;
    PWSTR InfName;
} PROTECT_FILE_ENTRY, *PPROTECT_FILE_ENTRY;

NTSTATUS
WINAPI
SfcGetFiles(
    _Out_ PPROTECT_FILE_ENTRY *ProtFileData,
    _Out_ PULONG FileCount);

#ifdef __cplusplus
}
#endif

#endif
