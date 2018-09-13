#include "mmcpl.h"

#include <tchar.h>
#include <initguid.h>
#include <devguid.h>

#include "drivers.h"
#include "sulib.h"
#include "trayvol.h"
#include "debug.h"

static PTSTR szLocalAllocFailMsg = TEXT("Failed memory allocation\n");

#define GUESS_LEGACY_SERVICE_NAME 0
#define tsizeof(s)  (sizeof(s)/sizeof(TCHAR))

// Generic listnode structure
typedef struct _LISTNODE
{
    struct _LISTNODE *pNext;
} LISTNODE;

// Structure describing a source disk entry
typedef struct _SOURCEDISK
{
    struct _SOURCEDISK *pNext;  // Next source disk in list
    TCHAR szDiskName[_MAX_PATH]; // Description of this disk
    int   DiskId;
} SOURCEDISK;

// Structure describing a file to copy
// We keep a list of these files in two places:
// 1. A global list of all files copied by the inf attached to the LEGACY_INF struct.
// 2. A pair driver-specific lists (user & kernel) attached to the LEGACY_DRIVER struct.
typedef struct _FILETOCOPY
{
    struct _FILETOCOPY *pNext;      // Next file to copy
    TCHAR szFileName[_MAX_FNAME];   // Name of file to copy
    int   DiskId;
} FILETOCOPY;

// Structure representing a legacy driver's information
typedef struct _LEGACY_DRIVER
{
    struct _LEGACY_DRIVER *pNext;

    TCHAR szDevNameKey[32];     // Device name key
    TCHAR szUserDevDrv[32];     // User-level device driver
    TCHAR szClasses[128];       // List of device classes this driver supports
    TCHAR szDesc[128];          // Description of device
    TCHAR szVxD[32];            // Name of VxD driver (not supported)
    TCHAR szParams[128];        // Params (not supported)
    TCHAR szDependency[128];    // Dependent device (not supported)
    FILETOCOPY *UserCopyList;   // List of all user files to copy
    FILETOCOPY *KernCopyList;   // List of all kernel files to copy
} LEGACY_DRIVER;

// Structure representing a legacy inf
typedef struct _LEGACY_INF
{
    struct _LEGACY_INF *pNext;

    TCHAR szLegInfPath[_MAX_PATH];   // Path to original legacy inf
    TCHAR szNewInfPath[_MAX_PATH];   // Path to converted inf
    LEGACY_DRIVER *DriverList;      // List of all drivers in this inf
    SOURCEDISK *SourceDiskList;     // List of all source disks in this inf
    FILETOCOPY *FileList;           // List of all files copied as part of this inf
} LEGACY_INF;

// Root structure for the whole tree
typedef struct _PROCESS_INF_INFO
{
    TCHAR szLegInfDir[_MAX_PATH];    // Directory where legacy infs are located
    TCHAR szNewInfDir[_MAX_PATH];    // Temp directory where new infs are generated
    TCHAR szSysInfDir[_MAX_PATH];    // Windows inf directory
    TCHAR szTemplate[_MAX_PATH];     // Template to search for
    LEGACY_INF *LegInfList;          // List of all infs to be converted
} PROCESS_INF_INFO;

#if defined DEBUG || defined _DEBUG || defined DEBUG_RETAIL
// Debugging routine to dump the contents of a LEGACY_INF list
void DumpLegacyInfInfo(PROCESS_INF_INFO *pPII)
{
    LEGACY_INF *pLI;
    LEGACY_DRIVER *pLD;

    for (pLI=pPII->LegInfList;pLI;pLI=pLI->pNext)
    {
        dlog1("Dumping legacy inf %s\n",pLI->szLegInfPath);

        dlog("Dump of legacy driver info:\n");
        for (pLD=pLI->DriverList; pLD; pLD=pLD->pNext)
        {
            dlog1("DriverNode=0x%x",    pLD);
            dlog1("\tszDevNameKey=%s",  pLD->szDevNameKey);
            dlog1("\tszUserDevDrv=%s",  pLD->szUserDevDrv);
            dlog1("\tszClasses=%s",     pLD->szClasses);
            dlog1("\tszDesc=%s",        pLD->szDesc);
            dlog1("\tszVxD=%s",         pLD->szVxD);
            dlog1("\tszParams=%s",      pLD->szParams);
            dlog1("\tszDependency=%s",  pLD->szDependency);
        }
    }

    return;
}
#else
    #define DumpLegacyInfInfo()
#endif


// Function to remove a directory tree and all its subtrees
void RemoveDirectoryTree(PTSTR szDirTree)
{
    TCHAR  PathBuffer[_MAX_PATH];
    PTSTR CurrentFile;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;

    // Build a file spec to find all files in specified directory
    // (i.e., <DirPath>\*.INF)
    _tcscpy(PathBuffer, szDirTree);
    catpath(PathBuffer,TEXT("\\*"));

    // Get a pointer to the end of the path part of the string
    // (minus the wildcard filename), so that we can append
    // each filename to it.
    CurrentFile = _tcsrchr(PathBuffer, TEXT('\\')) + 1;

    FindHandle = FindFirstFile(PathBuffer, &FindData);
    if (FindHandle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        // Skip '.' and '..' files, or else we crash!
        if ( (!_tcsicmp(FindData.cFileName,TEXT("."))) ||
             (!_tcsicmp(FindData.cFileName,TEXT(".."))) )
        {
            continue;
        }

        // Build the full pathname.
        _tcscpy(CurrentFile, FindData.cFileName);

        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            RemoveDirectoryTree(PathBuffer);
        }
        else
        {
            DeleteFile(PathBuffer);
        }
    } while (FindNextFile(FindHandle, &FindData));

    // Remember to close the find handle
    FindClose(FindHandle);

    // Now remove the directory
    RemoveDirectory(szDirTree);

    return;
}

// Generic routine to free a list
void FreeList(LISTNODE *pList)
{
    LISTNODE *pNext;
    while (pList)
    {
        pNext = pList->pNext;
        LocalFree(pList);
        pList=pNext;
    }

    return;
}

// Routine to free all memory that is part of a PROCESS_INF_INFO struct
void DestroyLegacyInfInfo(PROCESS_INF_INFO *pPII)
{

    LEGACY_INF    *pLIList, *pLINext;
    LEGACY_DRIVER *pLDList, *pLDNext;

    pLIList=pPII->LegInfList;
    while (pLIList)
    {
        pLINext = pLIList->pNext;

        pLDList = pLIList->DriverList;
        while (pLDList)
        {
            pLDNext = pLDList->pNext;

            // Free file copy lists
            FreeList((LISTNODE *)pLDList->UserCopyList);
            FreeList((LISTNODE *)pLDList->KernCopyList);

            // Free Driver node
            LocalFree(pLDList);

            pLDList = pLDNext;
        }

        // Free the source disk list
        FreeList((LISTNODE *)pLIList->SourceDiskList);

        // Free the file list
        FreeList((LISTNODE *)pLIList->FileList);

        // Free up the legacy inf structure
        LocalFree(pLIList);

        pLIList = pLINext;
    }

    // Free the pPII struct
    LocalFree(pPII);

    return;
}

// Searches a file list for a matching entry
FILETOCOPY *FindFile(PTSTR szFileName, FILETOCOPY *pFileList)
{
    FILETOCOPY *pFTC;

    for (pFTC=pFileList;pFTC;pFTC=pFTC->pNext)
    {
        if (!_tcsicmp(szFileName,pFTC->szFileName))
        {
            return pFileList;
        }
    }

    return NULL;
}

// Generic function for adding a file to a copy list
BOOL AddFileToFileList(PTSTR szFileName, int DiskId, FILETOCOPY **ppList)
{
    FILETOCOPY *pFTC;

    // Only add the entry if another one doesn't already exist
    if (!FindFile(szFileName,*ppList))
    {
        pFTC = (FILETOCOPY *)LocalAlloc(LPTR, sizeof(FILETOCOPY));
        if (!pFTC)
        {
            dlogt(szLocalAllocFailMsg);
            return FALSE;
        }

        // Save the fields
        pFTC->DiskId=DiskId;
        _tcscpy(pFTC->szFileName,szFileName);

        // Put it in the list
        pFTC->pNext=(*ppList);
        (*ppList)=pFTC;
    }

    return TRUE;
}

// Add a file to both the global and driver-specific copy lists
BOOL AddFileToCopyList(LEGACY_INF *pLI, LEGACY_DRIVER *pLD, TCHAR *szIdFile)
{
    int DiskId;
    TCHAR *szFileName;

    // szFile has both a disk ID and a file name, e.g. "1:foo.drv"
    // Get the disk ID and file name from szFile
    DiskId = _ttol(szIdFile);
    szFileName = RemoveDiskId(szIdFile);

    // Add the file to the global list
    AddFileToFileList(szFileName,DiskId,&(pLI->FileList));

    // Add the file to the correct driver-specific list
    if (IsFileKernelDriver(szFileName))
    {
        AddFileToFileList(szFileName,DiskId,&(pLD->KernCopyList));
    }
    else
    {
        AddFileToFileList(szFileName,DiskId,&(pLD->UserCopyList));
    }

    return TRUE;
}

// Build the data structure associated with a legacy inf file
// and return a pointer to it, or NULL if failure
LEGACY_INF *CreateLegacyInf(IN PCTSTR szLegInfPath)
{
    HINF hInf;              // Handle to the legacy inf
    INFCONTEXT InfContext;  // Inf context struct for parsing inf file

    LEGACY_INF *pLI;        // Struct describing this inf
    LEGACY_DRIVER *pLDList; // ptrs to drivers in this inf
    LEGACY_DRIVER *pLD;
    TCHAR szIdFile[32];     // Holds <DiskId>:<File> strings, e.g. "1:foo.drv"
    int MediaDescFieldId;

    // Open the inf file
    hInf = SetupOpenInfFile( szLegInfPath, NULL, INF_STYLE_OLDNT, NULL);
    if (hInf==INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    // Try to open the Installable.drivers32 or Installable.drivers section
    if (!SetupFindFirstLine(  hInf,TEXT("Installable.drivers32"),NULL,&InfContext))
    {
        if (!SetupFindFirstLine(  hInf,TEXT("Installable.drivers"),NULL,&InfContext))
        {
            SetupCloseInfFile(hInf);
            return NULL;
        }
    }

    // Allocate the LEGACY_INF struct which is the root of the data struct
    pLI = (LEGACY_INF *)LocalAlloc(LPTR, sizeof(LEGACY_INF));
    if (!pLI)
    {
        dlogt(szLocalAllocFailMsg);
        SetupCloseInfFile(hInf);
        return NULL;
    }

    // Save off the path to the legacy inf
    _tcscpy(pLI->szLegInfPath, szLegInfPath);

    // Init all other fields to 'safe' values
    pLI->szNewInfPath[0]='\0';
    pLI->DriverList=NULL;
    pLI->SourceDiskList=NULL;
    pLI->FileList=NULL;

    // Build legacy driver list
    pLDList = NULL;
    do
    {
        // Allocate a structure to hold info about this driver
        pLD = (LEGACY_DRIVER *)LocalAlloc(LPTR, sizeof(LEGACY_DRIVER));
        if (!pLD)
        {
            dlogt(szLocalAllocFailMsg);
            break;
        }

        // Init fields
        pLD->UserCopyList=NULL;
        pLD->KernCopyList=NULL;

        // parse the driver installation line
        SetupGetStringField(&InfContext,0,pLD->szDevNameKey,tsizeof(pLD->szDevNameKey),NULL);

        // The user-level driver has a disk id prepended to it. Throw it away.
        SetupGetStringField(&InfContext,1,szIdFile         ,tsizeof(szIdFile) ,NULL);
        _tcscpy(pLD->szUserDevDrv,RemoveDiskId(szIdFile));

        SetupGetStringField(&InfContext,2,pLD->szClasses   ,tsizeof(pLD->szClasses)   ,NULL);
        SetupGetStringField(&InfContext,3,pLD->szDesc      ,tsizeof(pLD->szDesc)      ,NULL);
        SetupGetStringField(&InfContext,4,pLD->szVxD       ,tsizeof(pLD->szVxD)       ,NULL);
        SetupGetStringField(&InfContext,5,pLD->szParams    ,tsizeof(pLD->szParams)    ,NULL);
        SetupGetStringField(&InfContext,6,pLD->szDependency,tsizeof(pLD->szDependency),NULL);

        // Remember to also copy the user-level driver
        AddFileToCopyList(pLI,pLD,szIdFile);

        // Put it into the list
        pLD->pNext = pLDList;
        pLDList = pLD;
    } while (SetupFindNextLine(&InfContext,&InfContext));

    // Status check- did we find any drivers?
    // If not, clean up and get out now!
    if (pLDList==NULL)
    {
        dlog1("CreateLegacyInf: Didn't find any drivers in inf %s\n",szLegInfPath);
        SetupCloseInfFile(hInf);
        LocalFree(pLI);
        return NULL;
    }

    // Save list in Legacy Inf structure
    pLI->DriverList = pLDList;

    // Generate file copy lists
    for (pLD=pLDList;pLD;pLD=pLD->pNext)
    {
        // Now add the other files into the list
        if (SetupFindFirstLine(hInf,pLD->szDevNameKey,NULL,&InfContext))
        {
            do
            {
                SetupGetStringField(&InfContext,0,szIdFile,tsizeof(szIdFile),NULL);
                AddFileToCopyList(pLI,pLD,szIdFile);
            } while (SetupFindNextLine(&InfContext,&InfContext));
        }
    }

    // Generate the SourceDiskList iff we find a section that lists them
    if (SetupFindFirstLine(hInf,TEXT("Source Media Descriptions"),NULL,&InfContext))
    {
        MediaDescFieldId=1;
    }
    else if (SetupFindFirstLine(hInf,TEXT("disks"),NULL,&InfContext))   // Old style
    {
        MediaDescFieldId=2;
    }
    else if (SetupFindFirstLine(hInf,TEXT("disks"),NULL,&InfContext))   // Old style
    {
        MediaDescFieldId=2;
    }
    else
    {
        MediaDescFieldId=0;
    }

    if (MediaDescFieldId)
    {
        do
        {
            SOURCEDISK *pSD;

            TCHAR szDiskId[8];
            pSD = (SOURCEDISK *)LocalAlloc(LPTR, sizeof(SOURCEDISK));
            if (!pSD)
            {
                dlogt(szLocalAllocFailMsg);
                break;
            }

            // Read the disk ID and description
            SetupGetIntField(&InfContext,0,&pSD->DiskId);
            SetupGetStringField(&InfContext,MediaDescFieldId,pSD->szDiskName,tsizeof(pSD->szDiskName),NULL);

            // Put it in the list
            pSD->pNext = pLI->SourceDiskList;
            pLI->SourceDiskList = pSD;
        } while (SetupFindNextLine(&InfContext,&InfContext));
    }

    SetupCloseInfFile(hInf);

    return pLI;
}


// Build a list containing information about all the legacy infs in the specified directory
PROCESS_INF_INFO *BuildLegacyInfInfo(PTSTR szLegacyInfDir, BOOL bEnumSingleInf)
{
    LEGACY_INF *pLegacyInf;
    PROCESS_INF_INFO *pPII;
    TCHAR  PathBuffer[_MAX_PATH];

    dlog1("ProcessLegacyInfDirectory processing directory %s\n",szLegacyInfDir);

    // Allocate a process inf info struct to hold params relating to the conversion process
    pPII = (PROCESS_INF_INFO *)LocalAlloc(LPTR, sizeof(PROCESS_INF_INFO));
    if (!pPII)
    {
        dlogt(szLocalAllocFailMsg);
        return NULL;
    }

    // Get a path to the windows inf directory
    GetWindowsDirectory(pPII->szSysInfDir,tsizeof(pPII->szSysInfDir));
    catpath(pPII->szSysInfDir,TEXT("\\INF"));

    // Create a temp dir for the new infs under the windows inf directory
    _tcscpy(pPII->szNewInfDir,pPII->szSysInfDir);
    catpath(pPII->szNewInfDir,TEXT("\\MEDIAINF"));

    // If the directory exists, delete it
    RemoveDirectoryTree(pPII->szNewInfDir);

    // Now create it.
    CreateDirectory(pPII->szNewInfDir,NULL);

    // Init list to NULL
    pPII->LegInfList=NULL;

    if (bEnumSingleInf) // If bEnumSingleInf true, szLegacyInfDir points to a single file
    {
        // Grab the path to the directory and store it in pPII->szLegInfDir
        _tcscpy(PathBuffer,szLegacyInfDir);
        _tcscpy(pPII->szLegInfDir,StripPathName(PathBuffer));

        // Load all the information about the legacy inf
        pLegacyInf = CreateLegacyInf(szLegacyInfDir);

        // If no error, link it into the list
        if (pLegacyInf)
        {
            pLegacyInf->pNext = pPII->LegInfList;
            pPII->LegInfList  = pLegacyInf;
        }

    }
    else    // bEnumSingleInf false, szLegacyInfDir points to a directory
    {
        HANDLE FindHandle;
        WIN32_FIND_DATA FindData;
        PTSTR CurrentInfFile;

        // Save path to original infs
        _tcscpy(pPII->szLegInfDir,szLegacyInfDir);

        // Build a file spec to find all INFs in specified directory, i.e., "<DirPath>\*.INF"
        _tcscpy(PathBuffer, szLegacyInfDir);
        catpath(PathBuffer,TEXT("\\*.INF"));

        // Get a pointer to the end of the path part of the string
        // (minus the wildcard filename), so that we can append
        // each filename to it.
        CurrentInfFile = _tcsrchr(PathBuffer, TEXT('\\')) + 1;

        // Search for all the inf files in this directory
        FindHandle = FindFirstFile(PathBuffer, &FindData);
        if (FindHandle != INVALID_HANDLE_VALUE)
        {
            do
            {
                // Build the full pathname.
                _tcscpy(CurrentInfFile, FindData.cFileName);

                // Load all the information about the legacy inf
                pLegacyInf = CreateLegacyInf(PathBuffer);

                // If no error, link it into the list
                if (pLegacyInf)
                {
                    pLegacyInf->pNext = pPII->LegInfList;
                    pPII->LegInfList = pLegacyInf;
                }
            } while (FindNextFile(FindHandle, &FindData));

            // Remember to close the find handle
            FindClose(FindHandle);
        }
    }

    // If we didn't find any drivers, just return NULL.
    if (pPII->LegInfList==NULL)
    {
        DestroyLegacyInfInfo(pPII);
        return NULL;
    }

    return pPII;
}

// Create a unique inf file in the temp directory
// Files will have the name INFxxxx.INF, where xxxx is a value between 0 and 1000
HANDLE OpenUniqueInfFile(PTSTR szDir, PTSTR szNewPath)
{
    HANDLE hInf;
    int Id;

    // Try up to 1000 values before giving up
    for (Id=0;Id<1000;Id++)
    {
        wsprintf(szNewPath,TEXT("%s\\INF%d.inf"),szDir,Id);

        // Setting CREATE_NEW flag will make call fail if file already exists
        hInf = CreateFile(  szNewPath,
                            GENERIC_WRITE|GENERIC_READ,
                            0,
                            NULL,
                            CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL,
                            0);

        // If we got back a valid handle, we can return
        if (hInf!=INVALID_HANDLE_VALUE)
        {
            return hInf;
        }
    }

    // Never found a valid handle. Give up.
    dlog("OpenUniqueInfFile: Couldn't create unique inf\n");
    return INVALID_HANDLE_VALUE;
}

// Helper function to append one formatted line of text to an open inf
void cdecl InfPrintf(HANDLE hInf, LPTSTR szFormat, ...)
{
    TCHAR Buf[MAXSTRINGLEN];
    int   nChars;

    // format into buffer
    va_list va;
    va_start (va, szFormat);
    nChars = wvsprintf (Buf,szFormat,va);
    va_end (va);

    // Append cr-lf
    _tcscpy(&Buf[nChars],TEXT("\r\n"));
    nChars+=2;

#ifdef UNICODE
    {
        int   mbCount;
        char  mbBuf[MAXSTRINGLEN];

        // Need to converto to mbcs before writing to file
        mbCount = WideCharToMultiByte(  GetACP(),               // code page
                                        WC_NO_BEST_FIT_CHARS,   // performance and mapping flags
                                        Buf,                    // address of wide-character string
                                        nChars,                 // number of characters in string
                                        mbBuf,                  // address of buffer for new string
                                        sizeof(mbBuf),          // size of buffer
                                        NULL,                   // address of default for unmappable characters
                                        NULL                    // address of flag set when default char. used
                                     );

        // Write line out to file
        WriteFile(hInf,mbBuf,mbCount,&mbCount,NULL);
    }
#else
    WriteFile(hInf,Buf,nChars,&nChars,NULL);
#endif

    return;
}

// Creates a new NT5-style inf file in the temporary directory.
BOOL CreateNewInfFile(PROCESS_INF_INFO *pPII, LEGACY_INF *pLI)
{
    SOURCEDISK *pSD;
    TCHAR szTmpKey[_MAX_PATH];
    LEGACY_DRIVER *pLDList, *pLD;
    HANDLE hInf;
    FILETOCOPY *pFTC;

    // Get a pointer to the legacy driver list
    pLDList = pLI->DriverList;

    dlog1("Creating new inf file %s\n",pPII->szNewInfDir);

    hInf = OpenUniqueInfFile(pPII->szNewInfDir, pLI->szNewInfPath);
    if (hInf==INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    // Write out version section
    InfPrintf(hInf,TEXT("[version]"));
    InfPrintf(hInf,TEXT("Signature=\"$WINDOWS NT$\""));
    InfPrintf(hInf,TEXT("Class=MEDIA"));
    InfPrintf(hInf,TEXT("ClassGUID=\"{4d36e96c-e325-11ce-bfc1-08002be10318}\""));
    InfPrintf(hInf,TEXT("Provider=Unknown"));

    // Write out Manufacturer section
    InfPrintf(hInf,TEXT("[Manufacturer]"));
    InfPrintf(hInf,TEXT("Unknown=OldDrvs"));

    // Write out OldDrvs section
    InfPrintf(hInf,TEXT("[OldDrvs]"));
    for (pLD=pLDList;pLD;pLD=pLD->pNext)
    {
        // Create an key to index into the strings section
        // This gives us something like:
        // %foo% = foo
        InfPrintf(hInf,TEXT("%%%s%%=%s"),pLD->szDevNameKey,pLD->szDevNameKey);
    }

    // Write out install section for each device
    for (pLD=pLDList;pLD;pLD=pLD->pNext)
    {
        // install section header, remember NT only
        InfPrintf(hInf,TEXT("[%s.NT]"),pLD->szDevNameKey);

        // DriverVer entry. Pick a date that's earlier than any NT5 infs
        InfPrintf(hInf,TEXT("DriverVer = 1/1/1998, 4.0.0.0"));

        // Addreg entry
        InfPrintf(hInf,TEXT("AddReg=%s.AddReg"),pLD->szDevNameKey);

        // CopyFiles entry
        InfPrintf(hInf,TEXT("CopyFiles=%s.CopyFiles.User,%s.CopyFiles.Kern"),pLD->szDevNameKey,pLD->szDevNameKey);

        // Reboot entry. Legacy drivers always require a reboot
        InfPrintf(hInf,TEXT("Reboot"));
    }

    // Write out the services section for each device
    // Legacy drivers have a stub services key
    for (pLD=pLDList;pLD;pLD=pLD->pNext)
    {

        InfPrintf(hInf,TEXT("[%s.NT.Services]"),pLD->szDevNameKey);
#if GUESS_LEGACY_SERVICE_NAME
        // If we install a .sys file, assume that the service name is the same as the filename
        pFTC=pLD->KernCopyList;
        if (pFTC)
        {
            TCHAR szServiceName[_MAX_FNAME];
            lsplitpath(pFTC->szFileName,NULL,NULL,szServiceName,NULL);

            InfPrintf(hInf,TEXT("AddService=%s,0x2,%s_Service_Inst"),szServiceName,szServiceName);
            InfPrintf(hInf,TEXT("[%s_Service_Inst]"),szServiceName);
            InfPrintf(hInf,TEXT("DisplayName    = %%%s%%"),pLD->szDevNameKey);
            InfPrintf(hInf,TEXT("ServiceType    = 1"));
            InfPrintf(hInf,TEXT("StartType      = 1"));
            InfPrintf(hInf,TEXT("ErrorControl   = 1"));
            InfPrintf(hInf,TEXT("ServiceBinary  = %%12%%\\%s"),pFTC->szFileName);
            InfPrintf(hInf,TEXT("LoadOrderGroup = Base"));
        }
        else
        {
            InfPrintf(hInf,TEXT("AddService=,0x2"));
        }
#else
        InfPrintf(hInf,TEXT("AddService=,0x2"));
#endif

    }

    // Write out the AddReg section for each device
    for (pLD=pLDList;pLD;pLD=pLD->pNext)
    {
        int nClasses;
        TCHAR szClasses[_MAX_PATH];
        TCHAR *pszState, *pszClass;

        // section header
        InfPrintf(hInf,TEXT("[%s.AddReg]"),pLD->szDevNameKey);
        InfPrintf(hInf,TEXT("HKR,Drivers,SubClasses,,\"%s\""),pLD->szClasses);

        // For safety, copy the string (mystrtok corrupts the original source string)
        _tcscpy(szClasses,pLD->szClasses);
        for (
            pszClass = mystrtok(szClasses,NULL,&pszState);
            pszClass;
            pszClass = mystrtok(NULL,NULL,&pszState)
            )
        {
            InfPrintf(hInf,TEXT("HKR,\"Drivers\\%s\\%s\", Driver,,%s"),         pszClass,pLD->szUserDevDrv,pLD->szUserDevDrv);
            InfPrintf(hInf,TEXT("HKR,\"Drivers\\%s\\%s\", Description,,%%%s%%"),pszClass,pLD->szUserDevDrv,pLD->szDevNameKey);
        }
    }

    // Write out the CopyFiles section for each device for user files
    for (pLD=pLDList;pLD;pLD=pLD->pNext)
    {
        // section header
        InfPrintf(hInf,TEXT("[%s.CopyFiles.User]"),pLD->szDevNameKey);
        for (pFTC=pLD->UserCopyList;pFTC;pFTC=pFTC->pNext)
        {
            InfPrintf(hInf,TEXT("%s"),pFTC->szFileName);
        }
    }

    // Write out the CopyFiles section for each device for kern files
    for (pLD=pLDList;pLD;pLD=pLD->pNext)
    {
        // section header
        InfPrintf(hInf,TEXT("[%s.CopyFiles.Kern]"),pLD->szDevNameKey);
        for (pFTC=pLD->KernCopyList;pFTC;pFTC=pFTC->pNext)
        {
            InfPrintf(hInf,TEXT("%s"),pFTC->szFileName);
        }
    }

    // Write out DestinationDirs section
    InfPrintf(hInf,TEXT("[DestinationDirs]"));
    for (pLD=pLDList;pLD;pLD=pLD->pNext)
    {
        InfPrintf(hInf,TEXT("%s.CopyFiles.User = 11"),pLD->szDevNameKey);
        InfPrintf(hInf,TEXT("%s.CopyFiles.Kern = 12"),pLD->szDevNameKey);
    }

    // Write out the SourceDisksNames section
    InfPrintf(hInf,TEXT("[SourceDisksNames]"));
    for (pSD=pLI->SourceDiskList;pSD;pSD=pSD->pNext)
    {
        InfPrintf(hInf,TEXT("%d = \"%s\",\"\",1"),pSD->DiskId,pSD->szDiskName);
    }
    // Write out the SourceDisksFiles section
    InfPrintf(hInf,TEXT("[SourceDisksFiles]"));
    for (pFTC=pLI->FileList;pFTC;pFTC=pFTC->pNext)
    {
        InfPrintf(hInf,TEXT("%s=%d"),pFTC->szFileName,pFTC->DiskId);
    }

    // Write out Strings section
    InfPrintf(hInf,TEXT("[Strings]"));
    for (pLD=pLDList;pLD;pLD=pLD->pNext)
    {
        // Create the device description
        InfPrintf(hInf,TEXT("%s=\"%s\""),pLD->szDevNameKey,pLD->szDesc);
    }

    CloseHandle(hInf);

    return TRUE;
}

// Creates a PNF file in the temporary directory to go along with the inf
// This allows us to have the inf file in one directory while the driver's
// files are in a different directory.
BOOL CreateNewPnfFile(PROCESS_INF_INFO *pPII, LEGACY_INF *pLI)
{
    BOOL bSuccess;

    TCHAR szSysInfPath[_MAX_PATH];
    TCHAR szSysPnfPath[_MAX_PATH];
    TCHAR szTmpPnfPath[_MAX_PATH];

    TCHAR szSysInfDrive[_MAX_DRIVE];
    TCHAR szSysInfDir[_MAX_DIR];
    TCHAR szSysInfFile[_MAX_FNAME];

    TCHAR szNewInfDrive[_MAX_DRIVE];
    TCHAR szNewInfDir[_MAX_DIR];
    TCHAR szNewInfFile[_MAX_FNAME];

    // Copy inf to inf directory to create pnf file
    bSuccess = SetupCopyOEMInf(
                              pLI->szNewInfPath,       //    IN  PCSTR   SourceInfFileName,
                              pPII->szLegInfDir,       //    IN  PCSTR   OEMSourceMediaLocation,         OPTIONAL
                              SPOST_PATH,   //    IN  DWORD   OEMSourceMediaType,
                              0,            //    IN  DWORD   CopyStyle,
                              szSysInfPath, //    OUT PSTR    DestinationInfFileName,         OPTIONAL
                              tsizeof(szSysInfPath),   //    IN  DWORD   DestinationInfFileNameSize,
                              NULL,         //    OUT PDWORD  RequiredSize,                   OPTIONAL
                              NULL          //    OUT PSTR   *DestinationInfFileNameComponent OPTIONAL
                              );

    if (!bSuccess)
    {
        dlog1("CreateNewPnfFile: SetupCopyOEMInf failed for inf %s\n",pLI->szNewInfPath);
        return FALSE;
    }

    // Cut apart the directory names
    lsplitpath(szSysInfPath,      szSysInfDrive, szSysInfDir, szSysInfFile, NULL);
    lsplitpath(pLI->szNewInfPath, szNewInfDrive, szNewInfDir, szNewInfFile, NULL);

    // Copy the pnf file back to the original directory
    wsprintf(szSysPnfPath,TEXT("%s%s%s.pnf"), szSysInfDrive, szSysInfDir, szSysInfFile);
    wsprintf(szTmpPnfPath,TEXT("%s%s%s.pnf"), szNewInfDrive, szNewInfDir, szNewInfFile);
    CopyFile(szSysPnfPath, szTmpPnfPath, FALSE);

    // Delete the inf and pnf file in the system inf directory
    DeleteFile(szSysInfPath);
    DeleteFile(szSysPnfPath);

    return TRUE;
}

// Create a new inf file for each legacy inf in the list
BOOL ProcessLegacyInfInfo(PROCESS_INF_INFO *pPII)
{
    LEGACY_INF *pLI;
    BOOL bSuccess;

    for (pLI=pPII->LegInfList;pLI;pLI=pLI->pNext)
    {
        bSuccess = CreateNewInfFile(pPII,pLI);
        if (bSuccess)
        {
            CreateNewPnfFile(pPII,pLI);
        }
    }

    return TRUE;
}

BOOL ConvertLegacyInfDir(PTSTR szLegacyDir, PTSTR szNewDir, BOOL bEnumSingleInf)
{
    PROCESS_INF_INFO *pPII;

    // Couldn't find any NT5-style drivers. Try to find some legacy inf files.
    // Build the list
    pPII = BuildLegacyInfInfo(szLegacyDir, bEnumSingleInf);
    if (!pPII)
    {
        return FALSE;
    }

    // Process the list
    ProcessLegacyInfInfo(pPII);

    if (bEnumSingleInf)
    {
        // if bEnumSingleInf is true, we should return a path to the new inf
        // (there should be exactly one)
        _tcscpy(szNewDir,pPII->LegInfList->szNewInfPath);
    }
    else
    {
        // if bEnumSingleInf is false, we should return a path to the directory
        _tcscpy(szNewDir,pPII->szNewInfDir);
    }

    // Cleanup data structures
    DestroyLegacyInfInfo(pPII);

    return TRUE;
}

int CountDriverInfoList(IN HDEVINFO         DeviceInfoSet,
                        IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
                        IN DWORD            DriverType
                       )
{
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;
    int DriverCount = 0;
    int Count = 0;

    // Count the number of drivers in the list
    DriverInfoData.cbSize = sizeof(DriverInfoData);
    while (SetupDiEnumDriverInfo(DeviceInfoSet,
                                 DeviceInfoData,
                                 DriverType,
                                 Count,
                                 &DriverInfoData))
    {
        // Only count drivers which don't have the DNF_BAD_DRIVER flag set
        DriverInstallParams.cbSize=sizeof(DriverInstallParams);
        if (SetupDiGetDriverInstallParams(DeviceInfoSet, DeviceInfoData, &DriverInfoData, &DriverInstallParams))
        {
            if (!(DriverInstallParams.Flags & DNF_BAD_DRIVER))
            {
                DriverCount++;
            }
        }
        Count++;
    }

    return DriverCount;
}

// Called to display a list of drivers to be installed
DWORD Media_SelectDevice(IN HDEVINFO         DeviceInfoSet,
                         IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                        )
{
    BOOL bResult;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    int DriverCount;

    // Undocumented: When user selects "Have Disk", setupapi only looks at the
    // class driver list. Therefore, we'll only work with that list
    DWORD DriverType = SPDIT_CLASSDRIVER;

    // Get the path to where the inf files are located
    DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);
    bResult = SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                            DeviceInfoData,
                                            &DeviceInstallParams);

    if (!bResult)
    {
        return ERROR_DI_DO_DEFAULT;
    }

    // For safety, don't support append mode
    if (DeviceInstallParams.FlagsEx & DI_FLAGSEX_APPENDDRIVERLIST)
    {
        return ERROR_DI_DO_DEFAULT;
    }        

    // If not going outside of inf directory, the DriverPath field will be an
    // empty string. In this case, don't do special processing.
    if (DeviceInstallParams.DriverPath[0]=='\0')
    {
        return ERROR_DI_DO_DEFAULT;
    }

    // We're going off to an OEM directory.

    // See if setup can find any NT5-compatible inf files

    // Try to build a driver info list in the current directory
    SetupDiDestroyDriverInfoList(DeviceInfoSet,DeviceInfoData,DriverType);
    SetupDiBuildDriverInfoList(DeviceInfoSet,DeviceInfoData,DriverType);

    // Filter out non NT inf files (e.g. Win9x inf files)
    FilterOutNonNTInfs(DeviceInfoSet,DeviceInfoData,DriverType);

    // Now count the number of drivers
    DriverCount = CountDriverInfoList(DeviceInfoSet,DeviceInfoData,DriverType);

    // If we found at least one NT5 driver for this device, just return
    if (DriverCount>0)
    {
        return ERROR_DI_DO_DEFAULT;
    }

    // Didn't find any NT5 drivers.

    // Destroy the existing list
    SetupDiDestroyDriverInfoList(DeviceInfoSet,DeviceInfoData,DriverType);

    // Convert any legacy infs and get back ptr to temp directory for converted infs
    bResult = ConvertLegacyInfDir(DeviceInstallParams.DriverPath, DeviceInstallParams.DriverPath, (DeviceInstallParams.Flags & DI_ENUMSINGLEINF));
    if (!bResult)
    {
        return ERROR_DI_DO_DEFAULT; // Didn't find any legacy infs
    }

    // Save new driver path
    bResult = SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                            DeviceInfoData,
                                            &DeviceInstallParams);

    // Note: We don't have to call SetupDiBuildDriverInfoList; setupapi will do that for us
    // with the new path to the infs.
    return ERROR_DI_DO_DEFAULT;
}

BOOL CreateRootDevice( IN     HDEVINFO    DeviceInfoSet,
                       IN     PTSTR       DeviceId,
                       IN     BOOL        bInstallNow
                     )
{
    BOOL                    bResult;
    SP_DEVINFO_DATA         DeviceInfoData;
    SP_DRVINFO_DATA         DriverInfoData;
    SP_DEVINSTALL_PARAMS    DeviceInstallParams;
    TCHAR                   tmpBuffer[100];
    DWORD                   bufferLen;
    DWORD                   Error;

    // Attempt to manufacture a new device information element for the root enumerated device
    _tcscpy(tmpBuffer,TEXT("ROOT\\MEDIA\\"));
    _tcscat(tmpBuffer,DeviceId);

    dlog2("CreateRootDevice: DeviceId = %s, Device = %s%",DeviceId,tmpBuffer);

    // Try to create the device info
    DeviceInfoData.cbSize = sizeof( DeviceInfoData );
    bResult = SetupDiCreateDeviceInfo( DeviceInfoSet,
                                       tmpBuffer,
                                       (GUID *) &GUID_DEVCLASS_MEDIA,
                                       NULL, // PCTSTR DeviceDescription
                                       NULL, // HWND hwndParent
                                       0,
                                       &DeviceInfoData );
    if (!bResult)
    {
        Error = GetLastError();
        dlog1("CreateRootDevice: SetupDiCreateDeviceInfo failed Error=%x",Error);
        return (Error == ERROR_DEVINST_ALREADY_EXISTS);
    }

    // Set the hardware ID.
    _tcscpy(tmpBuffer, DeviceId);
    bufferLen = _tcslen(tmpBuffer);                 // Get buffer len in chars
    tmpBuffer[bufferLen+1] = TEXT('\0');            // must terminate with an extra null (so we have two nulls)
    bufferLen = (bufferLen + 2) * sizeof(TCHAR);    // Convert buffer length to bytes & add extra for two nulls
    bResult = SetupDiSetDeviceRegistryProperty( DeviceInfoSet,
                                                &DeviceInfoData,
                                                SPDRP_HARDWAREID,
                                                (PBYTE)tmpBuffer,
                                                bufferLen );
    if (!bResult) goto CreateRootDevice_err;

    // Setup some flags before building a driver list
    bResult = SetupDiGetDeviceInstallParams( DeviceInfoSet,&DeviceInfoData,&DeviceInstallParams);
    if (bResult)
    {

        _tcscpy( DeviceInstallParams.DriverPath, TEXT( "" ) );
        DeviceInstallParams.FlagsEx |= DI_FLAGSEX_USECLASSFORCOMPAT;
        bResult = SetupDiSetDeviceInstallParams( DeviceInfoSet,&DeviceInfoData,&DeviceInstallParams);
    }

    // Build a compatible driver list for this new device...
    bResult = SetupDiBuildDriverInfoList( DeviceInfoSet,
                                          &DeviceInfoData,
                                          SPDIT_COMPATDRIVER);
    if (!bResult) goto CreateRootDevice_err;

    // Get the first driver on the list
    DriverInfoData.cbSize = sizeof (DriverInfoData);
    bResult = SetupDiEnumDriverInfo( DeviceInfoSet,
                                     &DeviceInfoData,
                                     SPDIT_COMPATDRIVER,
                                     0,
                                     &DriverInfoData);
    if (!bResult) goto CreateRootDevice_err;

    // Save the device description
    bResult = SetupDiSetDeviceRegistryProperty( DeviceInfoSet,
                                                &DeviceInfoData,
                                                SPDRP_DEVICEDESC,
                                                (PBYTE) DriverInfoData.Description,
                                                (_tcslen( DriverInfoData.Description ) + 1) * sizeof( TCHAR ) );
    if (!bResult) goto CreateRootDevice_err;

    // Set the selected driver
    bResult = SetupDiSetSelectedDriver( DeviceInfoSet,
                                        &DeviceInfoData,
                                        &DriverInfoData);
    if (!bResult) goto CreateRootDevice_err;

    // Register the device so it is not a phantom anymore
    bResult = SetupDiRegisterDeviceInfo( DeviceInfoSet,
                                         &DeviceInfoData,
                                         0,
                                         NULL,
                                         NULL,
                                         NULL);
    if (!bResult) goto CreateRootDevice_err;

    return bResult;

    // Error, delete the device info and give up
    CreateRootDevice_err:
    SetupDiDeleteDeviceInfo (DeviceInfoSet, &DeviceInfoData);
    return FALSE;
}

DWORD Media_MigrateLegacy(IN HDEVINFO         DeviceInfoSet,
                          IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                         )
{
    BOOL bInstallNow = TRUE;

    CreateRootDevice(DeviceInfoSet, TEXT("MS_MMMCI"), bInstallNow);
    CreateRootDevice(DeviceInfoSet, TEXT("MS_MMVID"), bInstallNow);
    CreateRootDevice(DeviceInfoSet, TEXT("MS_MMACM"), bInstallNow);
    CreateRootDevice(DeviceInfoSet, TEXT("MS_MMVCD"), bInstallNow);
    CreateRootDevice(DeviceInfoSet, TEXT("MS_MMDRV"), bInstallNow);
    return NO_ERROR;
}

int IsSpecialDriver(HDEVINFO         DeviceInfoSet,
                    PSP_DEVINFO_DATA DeviceInfoData)
{
    BOOL bResult;
    TCHAR HardwareId[32];
    bResult = SetupDiGetDeviceRegistryProperty( DeviceInfoSet,
                                                DeviceInfoData,
                                                SPDRP_HARDWAREID,
                                                NULL,
                                                (PBYTE)HardwareId,
                                                sizeof(HardwareId),
                                                NULL );

    if (!_tcscmp(HardwareId,TEXT("MS_MMMCI")))
        return IS_MS_MMMCI;
    else if (!_tcscmp(HardwareId,TEXT("MS_MMVID")))
        return IS_MS_MMVID;
    else if (!_tcscmp(HardwareId,TEXT("MS_MMACM")))
        return IS_MS_MMACM;
    else if (!_tcscmp(HardwareId,TEXT("MS_MMVCD")))
        return IS_MS_MMVCD;
    else if (!_tcscmp(HardwareId,TEXT("MS_MMDRV")))
        return IS_MS_MMDRV;
    return 0;
}

BOOL IsPnPDriver(IN PTSTR szName)
{
    LONG lRet;
    HKEY hkClass;

    int  iDriverInst;
    TCHAR szDriverInst[32];
    HKEY hkDriverInst;

    int iDriverType;
    TCHAR szDriverType[32];
    HKEY hkDriverType;

    int iDriverName;
    TCHAR szDriverName[32];

    // Open class key
    hkClass = SetupDiOpenClassRegKey((GUID *) &GUID_DEVCLASS_MEDIA, KEY_READ);
    if (hkClass == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    // enumerate each driver instances (e.g. 0000, 0001, etc.)
    for (iDriverInst = 0;
        !RegEnumKey(hkClass, iDriverInst, szDriverInst, sizeof(szDriverInst)/sizeof(TCHAR));
        iDriverInst++)
    {
        // Open the Drivers subkey, (e.g. 0000\Drivers)
        _tcscat(szDriverInst,TEXT("\\Drivers"));
        lRet = RegOpenKey(hkClass, szDriverInst, &hkDriverInst);
        if (lRet!=ERROR_SUCCESS)
        {
            continue;
        }

        // Enumerate each of the driver types (e.g. wave, midi, mixer, etc.)
        for (iDriverType = 0;
            !RegEnumKey(hkDriverInst, iDriverType, szDriverType, sizeof(szDriverType)/sizeof(TCHAR));
            iDriverType++)
        {

            // Open the driver type subkey
            lRet = RegOpenKey(hkDriverInst, szDriverType, &hkDriverType);
            if (lRet!=ERROR_SUCCESS)
            {
                continue;
            }

            // Enumerate each of the driver names (e.g. foo.drv)
            for (iDriverName = 0;
                !RegEnumKey(hkDriverType, iDriverName, szDriverName, sizeof(szDriverName)/sizeof(TCHAR));
                iDriverName++)
            {

                // Does this name match the one we were passed?
                if (!_tcsicmp(szName,szDriverName))
                {
                    RegCloseKey(hkDriverType);
                    RegCloseKey(hkDriverInst);
                    RegCloseKey(hkClass);
                    return TRUE;
                }
            }
            RegCloseKey(hkDriverType);
        }
        RegCloseKey(hkDriverInst);
    }
    RegCloseKey(hkClass);

    return FALSE;
}

