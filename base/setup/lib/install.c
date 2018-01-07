/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/install.c
 * PURPOSE:         Installation functions
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"
#include "filesup.h"
#include "infsupp.h"

#include "setuplib.h" // HAXX for USETUP_DATA!!

#include "install.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ****************************************************************/

/*
 * This code enumerates the list of files in reactos.dff / reactos.inf
 * that need to be extracted from reactos.cab and be installed in their
 * respective directories.
 */
/*
 * IMPORTANT NOTE: The INF file specification used for the .CAB in ReactOS
 * is not compliant with respect to TXTSETUP.SIF syntax or the standard syntax.
 */
static BOOLEAN
AddSectionToCopyQueueCab(
    IN PUSETUP_DATA pSetupData,
    IN HINF InfFile,
    IN PCWSTR SectionName,
    IN PCWSTR SourceCabinet,
    IN PCUNICODE_STRING DestinationPath)
{
    INFCONTEXT FilesContext;
    INFCONTEXT DirContext;
    PCWSTR FileKeyName;
    PCWSTR FileKeyValue;
    PCWSTR DirKeyValue;
    PCWSTR TargetFileName;
    WCHAR FileDstPath[MAX_PATH];

    /* Search for the SectionName section */
    if (!SpInfFindFirstLine(InfFile, SectionName, NULL, &FilesContext))
    {
        pSetupData->LastErrorNumber = ERROR_TXTSETUP_SECTION;
        if (pSetupData->ErrorRoutine)
            pSetupData->ErrorRoutine(pSetupData, SectionName);
        return FALSE;
    }

    /*
     * Enumerate the files in the section and add them to the file queue.
     */
    do
    {
        /* Get source file name and target directory id */
        if (!INF_GetData(&FilesContext, &FileKeyName, &FileKeyValue))
        {
            /* FIXME: Handle error! */
            DPRINT1("INF_GetData() failed\n");
            break;
        }

        /* Get optional target file name */
        if (!INF_GetDataField(&FilesContext, 2, &TargetFileName))
            TargetFileName = NULL;

        DPRINT("FileKeyName: '%S'  FileKeyValue: '%S'\n", FileKeyName, FileKeyValue);

        /* Lookup target directory */
        if (!SpInfFindFirstLine(InfFile, L"Directories", FileKeyValue, &DirContext))
        {
            /* FIXME: Handle error! */
            DPRINT1("SetupFindFirstLine() failed\n");
            INF_FreeData(FileKeyName);
            INF_FreeData(FileKeyValue);
            INF_FreeData(TargetFileName);
            break;
        }

        INF_FreeData(FileKeyValue);

        if (!INF_GetData(&DirContext, NULL, &DirKeyValue))
        {
            /* FIXME: Handle error! */
            DPRINT1("INF_GetData() failed\n");
            INF_FreeData(FileKeyName);
            INF_FreeData(TargetFileName);
            break;
        }

#if 1 // HACK moved! (r66604)
        {
        ULONG Length = wcslen(DirKeyValue);
        if ((Length > 0) && (DirKeyValue[Length - 1] == L'\\'))
            Length--;
        *((PWSTR)DirKeyValue + Length) = UNICODE_NULL;
        }

        /* Build the full target path */
        RtlStringCchCopyW(FileDstPath, ARRAYSIZE(FileDstPath),
                          pSetupData->DestinationRootPath.Buffer);
        if (DirKeyValue[0] == UNICODE_NULL)
        {
            /* Installation path */

            /* Add the installation path */
            ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 1, pSetupData->InstallPath.Buffer);
        }
        else if (DirKeyValue[0] == L'\\')
        {
            /* Absolute path */
            // if (DirKeyValue[1] != UNICODE_NULL)
                ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 1, DirKeyValue);
        }
        else // if (DirKeyValue[0] != L'\\')
        {
            /* Path relative to the installation path */

            /* Add the installation path */
            ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 2,
                        pSetupData->InstallPath.Buffer, DirKeyValue);
        }
#endif

        if (!SpFileQueueCopy((HSPFILEQ)pSetupData->SetupFileQueue,
                             pSetupData->SourceRootPath.Buffer,
                             pSetupData->SourceRootDir.Buffer,
                             FileKeyName,
                             NULL,
                             SourceCabinet,
                             NULL,
                             FileDstPath,
                             TargetFileName,
                             0 /* FIXME */))
        {
            /* FIXME: Handle error! */
            DPRINT1("SpFileQueueCopy() failed\n");
        }

        INF_FreeData(FileKeyName);
        INF_FreeData(TargetFileName);
        INF_FreeData(DirKeyValue);
    } while (SpInfFindNextLine(&FilesContext, &FilesContext));

    return TRUE;
}

// Note: Modeled after the SetupQueueCopySection() API
/*
BOOL SetupQueueCopySection(
  _In_ HSPFILEQ QueueHandle,
  _In_ PCTSTR   SourceRootPath,
  _In_ HINF     InfHandle,
  _In_ HINF     ListInfHandle,
  _In_ PCTSTR   Section,
  _In_ DWORD    CopyStyle
);
*/
static BOOLEAN
AddSectionToCopyQueue(
    IN PUSETUP_DATA pSetupData,
    IN HINF InfFile,
    IN PCWSTR SectionName,
    IN PCWSTR SourceCabinet,
    IN PCUNICODE_STRING DestinationPath)
{
    INFCONTEXT FilesContext;
    INFCONTEXT DirContext;
    PCWSTR FileKeyName;
    PCWSTR FileKeyValue;
    PCWSTR DirKeyValue;
    PCWSTR TargetFileName;
    WCHAR CompleteOrigDirName[512]; // FIXME: MAX_PATH is not enough?
    WCHAR FileDstPath[MAX_PATH];

    if (SourceCabinet)
        return AddSectionToCopyQueueCab(pSetupData, InfFile, L"SourceFiles", SourceCabinet, DestinationPath);

    /*
     * This code enumerates the list of files in txtsetup.sif
     * that need to be installed in their respective directories.
     */

    /* Search for the SectionName section */
    if (!SpInfFindFirstLine(InfFile, SectionName, NULL, &FilesContext))
    {
        pSetupData->LastErrorNumber = ERROR_TXTSETUP_SECTION;
        if (pSetupData->ErrorRoutine)
            pSetupData->ErrorRoutine(pSetupData, SectionName);
        return FALSE;
    }

    /*
     * Enumerate the files in the section and add them to the file queue.
     */
    do
    {
        /* Get source file name */
        if (!INF_GetDataField(&FilesContext, 0, &FileKeyName))
        {
            /* FIXME: Handle error! */
            DPRINT1("INF_GetData() failed\n");
            break;
        }

        /* Get target directory id */
        if (!INF_GetDataField(&FilesContext, 13, &FileKeyValue))
        {
            /* FIXME: Handle error! */
            DPRINT1("INF_GetData() failed\n");
            INF_FreeData(FileKeyName);
            break;
        }

        /* Get optional target file name */
        if (!INF_GetDataField(&FilesContext, 11, &TargetFileName))
            TargetFileName = NULL;
        else if (!*TargetFileName)
            TargetFileName = NULL;

        DPRINT("FileKeyName: '%S'  FileKeyValue: '%S'\n", FileKeyName, FileKeyValue);

        /* Lookup target directory */
        if (!SpInfFindFirstLine(InfFile, L"Directories", FileKeyValue, &DirContext))
        {
            /* FIXME: Handle error! */
            DPRINT1("SetupFindFirstLine() failed\n");
            INF_FreeData(FileKeyName);
            INF_FreeData(FileKeyValue);
            INF_FreeData(TargetFileName);
            break;
        }

        INF_FreeData(FileKeyValue);

        if (!INF_GetData(&DirContext, NULL, &DirKeyValue))
        {
            /* FIXME: Handle error! */
            DPRINT1("INF_GetData() failed\n");
            INF_FreeData(FileKeyName);
            INF_FreeData(TargetFileName);
            break;
        }

        if ((DirKeyValue[0] == UNICODE_NULL) || (DirKeyValue[0] == L'\\' && DirKeyValue[1] == UNICODE_NULL))
        {
            /* Installation path */
            DPRINT("InstallationPath: '%S'\n", DirKeyValue);

            RtlStringCchCopyW(CompleteOrigDirName, ARRAYSIZE(CompleteOrigDirName),
                              pSetupData->SourceRootDir.Buffer);

            DPRINT("InstallationPath(2): '%S'\n", CompleteOrigDirName);
        }
        else if (DirKeyValue[0] == L'\\')
        {
            /* Absolute path */
            DPRINT("AbsolutePath: '%S'\n", DirKeyValue);

            RtlStringCchCopyW(CompleteOrigDirName, ARRAYSIZE(CompleteOrigDirName),
                              DirKeyValue);

            DPRINT("AbsolutePath(2): '%S'\n", CompleteOrigDirName);
        }
        else // if (DirKeyValue[0] != L'\\')
        {
            /* Path relative to the installation path */
            DPRINT("RelativePath: '%S'\n", DirKeyValue);

            CombinePaths(CompleteOrigDirName, ARRAYSIZE(CompleteOrigDirName), 2,
                         pSetupData->SourceRootDir.Buffer, DirKeyValue);

            DPRINT("RelativePath(2): '%S'\n", CompleteOrigDirName);
        }

#if 1 // HACK moved! (r66604)
        {
        ULONG Length = wcslen(DirKeyValue);
        if ((Length > 0) && (DirKeyValue[Length - 1] == L'\\'))
            Length--;
        *((PWSTR)DirKeyValue + Length) = UNICODE_NULL;
        }

        /* Build the full target path */
        RtlStringCchCopyW(FileDstPath, ARRAYSIZE(FileDstPath),
                          pSetupData->DestinationRootPath.Buffer);
        if (DirKeyValue[0] == UNICODE_NULL)
        {
            /* Installation path */

            /* Add the installation path */
            ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 1, pSetupData->InstallPath.Buffer);
        }
        else if (DirKeyValue[0] == L'\\')
        {
            /* Absolute path */
            // if (DirKeyValue[1] != UNICODE_NULL)
                ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 1, DirKeyValue);
        }
        else // if (DirKeyValue[0] != L'\\')
        {
            /* Path relative to the installation path */

            /* Add the installation path */
            ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 2,
                        pSetupData->InstallPath.Buffer, DirKeyValue);
        }
#endif

        if (!SpFileQueueCopy((HSPFILEQ)pSetupData->SetupFileQueue,
                             pSetupData->SourceRootPath.Buffer,
                             CompleteOrigDirName,
                             FileKeyName,
                             NULL,
                             SourceCabinet,
                             NULL,
                             FileDstPath,
                             TargetFileName,
                             0 /* FIXME */))
        {
            /* FIXME: Handle error! */
            DPRINT1("SpFileQueueCopy() failed\n");
        }

        INF_FreeData(FileKeyName);
        INF_FreeData(TargetFileName);
        INF_FreeData(DirKeyValue);
    } while (SpInfFindNextLine(&FilesContext, &FilesContext));

    return TRUE;
}

BOOLEAN // ERROR_NUMBER
PrepareCopyInfFile(
    IN OUT PUSETUP_DATA pSetupData,
    IN HINF InfFile,
    IN PCWSTR SourceCabinet OPTIONAL)
{
    NTSTATUS Status;
    INFCONTEXT DirContext;
    PWCHAR AdditionalSectionName = NULL;
    PCWSTR DirKeyValue;
    WCHAR PathBuffer[MAX_PATH];

    /* Add common files */
    if (!AddSectionToCopyQueue(pSetupData, InfFile, L"SourceDisksFiles", SourceCabinet, &pSetupData->DestinationPath))
        return FALSE;

    /* Add specific files depending of computer type */
    if (SourceCabinet == NULL)
    {
        if (!ProcessComputerFiles(InfFile, pSetupData->ComputerList, &AdditionalSectionName))
            return FALSE;

        if (AdditionalSectionName)
        {
            if (!AddSectionToCopyQueue(pSetupData, InfFile, AdditionalSectionName, SourceCabinet, &pSetupData->DestinationPath))
                return FALSE;
        }
    }

    /* Create directories */

    /*
     * FIXME:
     * Copying files to pSetupData->DestinationRootPath should be done from within
     * the SystemPartitionFiles section.
     * At the moment we check whether we specify paths like '\foo' or '\\' for that.
     * For installing to pSetupData->DestinationPath specify just '\' .
     */

    /* Get destination path */
    RtlStringCchCopyW(PathBuffer, ARRAYSIZE(PathBuffer), pSetupData->DestinationPath.Buffer);

    DPRINT("FullPath(1): '%S'\n", PathBuffer);

    /* Create the install directory */
    Status = SetupCreateDirectory(PathBuffer);
    if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION)
    {
        DPRINT1("Creating directory '%S' failed: Status = 0x%08lx\n", PathBuffer, Status);
        pSetupData->LastErrorNumber = ERROR_CREATE_INSTALL_DIR;
        if (pSetupData->ErrorRoutine)
            pSetupData->ErrorRoutine(pSetupData, PathBuffer);
        return FALSE;
    }

    /* Search for the 'Directories' section */
    if (!SpInfFindFirstLine(InfFile, L"Directories", NULL, &DirContext))
    {
        if (SourceCabinet)
            pSetupData->LastErrorNumber = ERROR_CABINET_SECTION;
        else
            pSetupData->LastErrorNumber = ERROR_TXTSETUP_SECTION;

        if (pSetupData->ErrorRoutine)
            pSetupData->ErrorRoutine(pSetupData, L"Directories");
        return FALSE;
    }

    /* Enumerate the directory values and create the subdirectories */
    do
    {
        if (!INF_GetData(&DirContext, NULL, &DirKeyValue))
        {
            DPRINT1("break\n");
            break;
        }

        if ((DirKeyValue[0] == UNICODE_NULL) || (DirKeyValue[0] == L'\\' && DirKeyValue[1] == UNICODE_NULL))
        {
            /* Installation path */
            DPRINT("InstallationPath: '%S'\n", DirKeyValue);

            RtlStringCchCopyW(PathBuffer, ARRAYSIZE(PathBuffer),
                              pSetupData->DestinationPath.Buffer);

            DPRINT("InstallationPath(2): '%S'\n", PathBuffer);
        }
        else if (DirKeyValue[0] == L'\\')
        {
            /* Absolute path */
            DPRINT("AbsolutePath: '%S'\n", DirKeyValue);

            CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 2,
                         pSetupData->DestinationRootPath.Buffer, DirKeyValue);

            DPRINT("AbsolutePath(2): '%S'\n", PathBuffer);

            Status = SetupCreateDirectory(PathBuffer);
            if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION)
            {
                INF_FreeData(DirKeyValue);
                DPRINT("Creating directory '%S' failed: Status = 0x%08lx", PathBuffer, Status);
                pSetupData->LastErrorNumber = ERROR_CREATE_DIR;
                if (pSetupData->ErrorRoutine)
                    pSetupData->ErrorRoutine(pSetupData, PathBuffer);
                return FALSE;
            }
        }
        else // if (DirKeyValue[0] != L'\\')
        {
            /* Path relative to the installation path */
            DPRINT("RelativePath: '%S'\n", DirKeyValue);

            CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 2,
                         pSetupData->DestinationPath.Buffer, DirKeyValue);

            DPRINT("RelativePath(2): '%S'\n", PathBuffer);

            Status = SetupCreateDirectory(PathBuffer);
            if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION)
            {
                INF_FreeData(DirKeyValue);
                DPRINT("Creating directory '%S' failed: Status = 0x%08lx", PathBuffer, Status);
                pSetupData->LastErrorNumber = ERROR_CREATE_DIR;
                if (pSetupData->ErrorRoutine)
                    pSetupData->ErrorRoutine(pSetupData, PathBuffer);
                return FALSE;
            }
        }

        INF_FreeData(DirKeyValue);
    } while (SpInfFindNextLine(&DirContext, &DirContext));

    return TRUE;
}


// #define USE_CABINET_INF

BOOLEAN // ERROR_NUMBER
PrepareFileCopy(
    IN OUT PUSETUP_DATA pSetupData,
    IN PFILE_COPY_STATUS_ROUTINE StatusRoutine OPTIONAL)
{
    HINF InfHandle;
    INFCONTEXT CabinetsContext;
    PCWSTR CabinetName;
    UINT ErrorLine;
#if defined(__REACTOS__) && defined(USE_CABINET_INF)
    ULONG InfFileSize;
    PVOID InfFileData;
    CABINET_CONTEXT CabinetContext;
#endif
    WCHAR PathBuffer[MAX_PATH];

    /* Create the file queue */
    pSetupData->SetupFileQueue = (PVOID)SpFileQueueOpen();
    if (pSetupData->SetupFileQueue == NULL)
    {
        pSetupData->LastErrorNumber = ERROR_COPY_QUEUE;
        if (pSetupData->ErrorRoutine)
            pSetupData->ErrorRoutine(pSetupData);
        return FALSE;
    }

    /* Prepare the copy of the common files that are not in installation cabinets */
    if (!PrepareCopyInfFile(pSetupData, pSetupData->SetupInf, NULL))
    {
        /* FIXME: show an error dialog */
        return FALSE;
    }

    /* Search for the 'Cabinets' section */
    if (!SpInfFindFirstLine(pSetupData->SetupInf, L"Cabinets", NULL, &CabinetsContext))
    {
        /* Skip this step and return success if no cabinet file is listed */
        return TRUE;
    }

    /*
     * Enumerate the installation cabinets listed in the
     * 'Cabinets' section and parse their inf files.
     */
    do
    {
        if (!INF_GetData(&CabinetsContext, NULL, &CabinetName))
            break;

        CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 2,
                     pSetupData->SourcePath.Buffer, CabinetName);

#if defined(__REACTOS__) && defined(USE_CABINET_INF)
        INF_FreeData(CabinetName);

        CabinetInitialize(&CabinetContext);
        CabinetSetEventHandlers(&CabinetContext, NULL, NULL, NULL);
        CabinetSetCabinetName(&CabinetContext, PathBuffer);

        if (CabinetOpen(&CabinetContext) == CAB_STATUS_SUCCESS)
        {
            DPRINT("Cabinet %S\n", PathBuffer);

            InfFileData = CabinetGetCabinetReservedArea(&CabinetContext, &InfFileSize);
            if (InfFileData == NULL)
            {
                CabinetCleanup(&CabinetContext);

                pSetupData->LastErrorNumber = ERROR_CABINET_SCRIPT;
                if (pSetupData->ErrorRoutine)
                    pSetupData->ErrorRoutine(pSetupData, PathBuffer);
                return FALSE;
            }
        }
        else
        {
            DPRINT("Cannot open cabinet: %S.\n", PathBuffer);
            CabinetCleanup(&CabinetContext);

            pSetupData->LastErrorNumber = ERROR_CABINET_MISSING;
            if (pSetupData->ErrorRoutine)
                pSetupData->ErrorRoutine(pSetupData, PathBuffer);
            return FALSE;
        }

        InfHandle = INF_OpenBufferedFileA((PSTR)InfFileData,
                                          InfFileSize,
                                          NULL,
                                          INF_STYLE_WIN4,
                                          pSetupData->LanguageId,
                                          &ErrorLine);

        CabinetCleanup(&CabinetContext);
#else
        {
        PWCHAR ptr;

        /* First find the filename */
        ptr = wcsrchr(PathBuffer, L'\\');
        if (!ptr) ptr = PathBuffer;

        /* Then find its extension */
        ptr = wcsrchr(ptr, L'.');
        if (!ptr)
            ptr = PathBuffer + wcslen(PathBuffer);

        /* Replace it by '.inf' */
        wcscpy(ptr, L".inf");

        InfHandle = SpInfOpenInfFile(PathBuffer,
                                     NULL,
                                     INF_STYLE_OLDNT, // INF_STYLE_WIN4,
                                     pSetupData->LanguageId,
                                     &ErrorLine);
        }
#endif

        if (InfHandle == INVALID_HANDLE_VALUE)
        {
            pSetupData->LastErrorNumber = ERROR_INVALID_CABINET_INF;
            if (pSetupData->ErrorRoutine)
                pSetupData->ErrorRoutine(pSetupData, PathBuffer);
            return FALSE;
        }

        if (!PrepareCopyInfFile(pSetupData, InfHandle, CabinetName))
        {
#if !(defined(__REACTOS__) && defined(USE_CABINET_INF))
            SpInfCloseInfFile(InfHandle);
#endif
            /* FIXME: show an error dialog */
            return FALSE;
        }

#if !(defined(__REACTOS__) && defined(USE_CABINET_INF))
        SpInfCloseInfFile(InfHandle);
#endif
    } while (SpInfFindNextLine(&CabinetsContext, &CabinetsContext));

    return TRUE;
}

BOOLEAN
DoFileCopy(
    IN OUT PUSETUP_DATA pSetupData,
    IN PSP_FILE_CALLBACK_W MsgHandler,
    IN PVOID Context OPTIONAL)
{
    BOOLEAN Success;

    Success = SpFileQueueCommit(NULL,
                                (HSPFILEQ)pSetupData->SetupFileQueue,
                                MsgHandler,
                                Context);

    SpFileQueueClose((HSPFILEQ)pSetupData->SetupFileQueue);
    pSetupData->SetupFileQueue = NULL;

    return Success;
}

/* EOF */
