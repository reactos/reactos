
typedef struct _NTOS_INSTALLATION
{
    LIST_ENTRY ListEntry;
    ULONG DiskNumber;
    ULONG PartitionNumber;
// Vendor????
    WCHAR SystemRoot[MAX_PATH];
/**/WCHAR InstallationName[MAX_PATH];/**/
} NTOS_INSTALLATION, *PNTOS_INSTALLATION;

// EnumerateNTOSInstallations
PGENERIC_LIST
CreateNTOSInstallationsList(
    IN PPARTLIST List);
