/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        base/setup/usetup/cabinet.h
 * PURPOSE:     Cabinet definitions
 */
#pragma once

/* Cabinet structures */

// Shadow types, implementation-specific
typedef struct _CFHEADER *PCFHEADER;
typedef struct _CFFOLDER *PCFFOLDER;
typedef struct _CFFILE *PCFFILE;
typedef struct _CFDATA *PCFDATA;

struct _CABINET_CONTEXT;


/* Constants */

/* Status codes */
#define CAB_STATUS_SUCCESS       0x00000000
#define CAB_STATUS_FAILURE       0x00000001
#define CAB_STATUS_NOMEMORY      0x00000002
#define CAB_STATUS_CANNOT_OPEN   0x00000003
#define CAB_STATUS_CANNOT_CREATE 0x00000004
#define CAB_STATUS_CANNOT_READ   0x00000005
#define CAB_STATUS_CANNOT_WRITE  0x00000006
#define CAB_STATUS_FILE_EXISTS   0x00000007
#define CAB_STATUS_INVALID_CAB   0x00000008
#define CAB_STATUS_NOFILE        0x00000009
#define CAB_STATUS_UNSUPPCOMP    0x0000000A


/* Codecs */

typedef struct _CAB_CODEC *PCAB_CODEC;

/* Codec status codes */
#define CS_SUCCESS      0x0000  /* All data consumed */
#define CS_NOMEMORY     0x0001  /* Not enough free memory */
#define CS_BADSTREAM    0x0002  /* Bad data stream */

/* Codec identifiers */
#define CAB_CODEC_RAW   0x00
#define CAB_CODEC_LZX   0x01
#define CAB_CODEC_MSZIP 0x02


/* Event handler prototypes */

typedef BOOL (*PCABINET_OVERWRITE)(
    IN struct _CABINET_CONTEXT* CabinetContext,
    IN PCFFILE File,
    IN PCWSTR FileName);

typedef VOID (*PCABINET_EXTRACT)(
    IN struct _CABINET_CONTEXT* CabinetContext,
    IN PCFFILE File,
    IN PCWSTR FileName);

typedef VOID (*PCABINET_DISK_CHANGE)(
    IN struct _CABINET_CONTEXT* CabinetContext,
    IN PCWSTR CabinetName,
    IN PCWSTR DiskLabel);


/* Classes */

typedef struct _CAB_SEARCH
{
    WCHAR        Search[MAX_PATH];  // Search criteria
    WCHAR        Cabinet[MAX_PATH];
    USHORT       Index;
    PCFFILE      File;              // Pointer to current CFFILE
    PCFDATA      CFData;
    ULONG        Offset;
} CAB_SEARCH, *PCAB_SEARCH;

typedef struct _CABINET_CONTEXT
{
    WCHAR CabinetName[256];         // Filename of current cabinet
    WCHAR CabinetPrev[256];         // Filename of previous cabinet
    WCHAR DiskPrev[256];            // Label of cabinet in file CabinetPrev
    WCHAR CabinetNext[256];         // Filename of next cabinet
    WCHAR DiskNext[256];            // Label of cabinet in file CabinetNext
    ULONG FolderUncompSize;         // Uncompressed size of folder
    ULONG BytesLeftInBlock;         // Number of bytes left in current block
    WCHAR DestPath[MAX_PATH];
    HANDLE FileHandle;
    HANDLE FileSectionHandle;
    PUCHAR FileBuffer;
    SIZE_T DestFileSize;
    SIZE_T FileSize;
    BOOL FileOpen;
    PCFHEADER PCABHeader;
    PCFFOLDER CabinetFolders;
    ULONG CabinetReserved;
    ULONG FolderReserved;
    ULONG DataReserved;
    PCAB_CODEC Codec;
    ULONG CodecId;
    BOOL CodecSelected;
    ULONG LastFileOffset;           // Uncompressed offset of last extracted file
    PCABINET_OVERWRITE OverwriteHandler;
    PCABINET_EXTRACT ExtractHandler;
    PCABINET_DISK_CHANGE DiskChangeHandler;
    PVOID CabinetReservedArea;
} CABINET_CONTEXT, *PCABINET_CONTEXT;


/* Default constructor */
VOID
CabinetInitialize(
    IN OUT PCABINET_CONTEXT CabinetContext);

/* Default destructor */
VOID
CabinetCleanup(
    IN OUT PCABINET_CONTEXT CabinetContext);

#if 0
/* Returns a pointer to the filename part of a fully qualified filename */
PWCHAR CabinetGetFileName(PWCHAR Path);
/* Removes a filename from a fully qualified filename */
VOID CabinetRemoveFileName(PWCHAR Path);
/* Normalizes a path */
BOOL CabinetNormalizePath(PWCHAR Path, ULONG Length);
#endif

/* Returns name of cabinet file */
PCWSTR
CabinetGetCabinetName(
    IN PCABINET_CONTEXT CabinetContext);

/* Sets the name of the cabinet file */
VOID
CabinetSetCabinetName(
    IN PCABINET_CONTEXT CabinetContext,
    IN PCWSTR FileName);

/* Sets destination path for extracted files */
VOID
CabinetSetDestinationPath(
    IN PCABINET_CONTEXT CabinetContext,
    IN PCWSTR DestinationPath);

/* Returns destination path */
PCWSTR
CabinetGetDestinationPath(
    IN PCABINET_CONTEXT CabinetContext);

#if 0
/* Returns zero-based current disk number */
ULONG CabinetGetCurrentDiskNumber(VOID);
#endif

/* Opens the current cabinet file */
ULONG
CabinetOpen(
    IN OUT PCABINET_CONTEXT CabinetContext);

/* Closes the current open cabinet file */
VOID
CabinetClose(
    IN OUT PCABINET_CONTEXT CabinetContext);

/* Locates the first file in the current cabinet file that matches a search criteria */
ULONG
CabinetFindFirst(
    IN PCABINET_CONTEXT CabinetContext,
    IN PCWSTR FileName,
    IN OUT PCAB_SEARCH Search);

/* Locates the next file that matches the current search criteria */
ULONG
CabinetFindNext(
    IN PCABINET_CONTEXT CabinetContext,
    IN OUT PCAB_SEARCH Search);

/* Locates the next file in the current cabinet file sequentially */
ULONG
CabinetFindNextFileSequential(
    IN PCABINET_CONTEXT CabinetContext,
    IN PCWSTR FileName,
    IN OUT PCAB_SEARCH Search);

/* Extracts a file from the current cabinet file */
ULONG
CabinetExtractFile(
    IN PCABINET_CONTEXT CabinetContext,
    IN PCAB_SEARCH Search);

/* Select codec engine to use */
VOID
CabinetSelectCodec(
    IN PCABINET_CONTEXT CabinetContext,
    IN ULONG Id);

/* Set event handlers */
VOID
CabinetSetEventHandlers(
    IN PCABINET_CONTEXT CabinetContext,
    IN PCABINET_OVERWRITE Overwrite,
    IN PCABINET_EXTRACT Extract,
    IN PCABINET_DISK_CHANGE DiskChange);

/* Get pointer to cabinet reserved area. NULL if none */
PVOID
CabinetGetCabinetReservedArea(
    IN PCABINET_CONTEXT CabinetContext,
    OUT PULONG Size);
