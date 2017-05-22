
typedef struct _NTOS_INSTALLATION
{
    LIST_ENTRY ListEntry;
    ULONG DiskNumber;
    ULONG PartitionNumber;
    PPARTENTRY PartEntry;
// BOOLEAN IsDefault;   // TRUE / FALSE whether this installation is marked as "default" in its corresponding loader configuration file.
// Vendor???? (Microsoft / ReactOS)
/**/WCHAR SystemRoot[MAX_PATH];/**/
    UNICODE_STRING SystemArcPath;   // Normalized ARC path
    UNICODE_STRING SystemNtPath;    // Corresponding NT path
/**/WCHAR InstallationName[MAX_PATH];/**/
} NTOS_INSTALLATION, *PNTOS_INSTALLATION;

// EnumerateNTOSInstallations
PGENERIC_LIST
CreateNTOSInstallationsList(
    IN PPARTLIST List);
