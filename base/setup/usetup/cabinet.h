/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        apps/cabman/cabinet.h
 * PURPOSE:     Cabinet definitions
 */
#ifndef __CABINET_H
#define __CABINET_H

#include <string.h>

/* Cabinet constants */

#define CAB_SIGNATURE        0x4643534D // "MSCF"
#define CAB_VERSION          0x0103
#define CAB_BLOCKSIZE        32768

#define CAB_COMP_MASK        0x00FF
#define CAB_COMP_NONE        0x0000
#define CAB_COMP_MSZIP       0x0001
#define CAB_COMP_QUANTUM     0x0002
#define CAB_COMP_LZX         0x0003

#define CAB_FLAG_HASPREV     0x0001
#define CAB_FLAG_HASNEXT     0x0002
#define CAB_FLAG_RESERVE     0x0004

#define CAB_ATTRIB_READONLY  0x0001
#define CAB_ATTRIB_HIDDEN    0x0002
#define CAB_ATTRIB_SYSTEM    0x0004
#define CAB_ATTRIB_VOLUME    0x0008
#define CAB_ATTRIB_DIRECTORY 0x0010
#define CAB_ATTRIB_ARCHIVE   0x0020
#define CAB_ATTRIB_EXECUTE   0x0040
#define CAB_ATTRIB_UTF_NAME  0x0080

#define CAB_FILE_MAX_FOLDER  0xFFFC
#define CAB_FILE_CONTINUED   0xFFFD
#define CAB_FILE_SPLIT       0xFFFE
#define CAB_FILE_PREV_NEXT   0xFFFF


/* Cabinet structures */

typedef struct _CFHEADER
{
    ULONG Signature;        // File signature 'MSCF' (CAB_SIGNATURE)
    ULONG Reserved1;        // Reserved field
    ULONG CabinetSize;      // Cabinet file size
    ULONG Reserved2;        // Reserved field
    ULONG FileTableOffset;  // Offset of first CFFILE
    ULONG Reserved3;        // Reserved field
    WORD  Version;          // Cabinet version (CAB_VERSION)
    WORD  FolderCount;      // Number of folders
    WORD  FileCount;        // Number of files
    WORD  Flags;            // Cabinet flags (CAB_FLAG_*)
    WORD  SetID;            // Cabinet set id
    WORD  CabinetNumber;    // Zero-based cabinet number
/* Optional fields (depends on Flags)
    WORD  CabinetResSize    // Per-cabinet reserved area size
    CHAR  FolderResSize     // Per-folder reserved area size
    CHAR  FileResSize       // Per-file reserved area size
    CHAR  CabinetReserved[] // Per-cabinet reserved area
    CHAR  CabinetPrev[]     // Name of previous cabinet file
    CHAR  DiskPrev[]        // Name of previous disk
    CHAR  CabinetNext[]     // Name of next cabinet file
    CHAR  DiskNext[]        // Name of next disk
 */
} CFHEADER, *PCFHEADER;


typedef struct _CFFOLDER
{
    ULONG DataOffset;       // Absolute offset of first CFDATA block in this folder
    WORD  DataBlockCount;   // Number of CFDATA blocks in this folder in this cabinet
    WORD  CompressionType;  // Type of compression used for all CFDATA blocks in this folder
/* Optional fields (depends on Flags)
    CHAR  FolderReserved[]  // Per-folder reserved area
 */
} CFFOLDER, *PCFFOLDER;


typedef struct _CFFILE
{
  ULONG FileSize;         // Uncompressed file size in bytes
  ULONG FileOffset;       // Uncompressed offset of file in the folder
  WORD  FolderIndex;      // Index number of the folder that contains this file
  WORD  FileDate;         // File date stamp, as used by DOS
  WORD  FileTime;         // File time stamp, as used by DOS
  WORD  Attributes;       // File attributes (CAB_ATTRIB_*)
  CHAR  FileName[ANYSIZE_ARRAY];
    /* After this is the NULL terminated filename */
} CFFILE, *PCFFILE;


typedef struct _CFDATA
{
    ULONG Checksum;         // Checksum of CFDATA entry
    WORD  CompSize;         // Number of compressed bytes in this block
    WORD  UncompSize;       // Number of uncompressed bytes in this block
/* Optional fields (depends on Flags)
    CHAR  DataReserved[]    // Per-datablock reserved area
 */
} CFDATA, *PCFDATA;

typedef struct _CAB_SEARCH
{
  WCHAR        Search[MAX_PATH];   // Search criteria
  WCHAR        Cabinet[MAX_PATH];
  USHORT       Index;
  PCFFILE      File;               // Pointer to current CFFILE
} CAB_SEARCH, *PCAB_SEARCH;


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

/* Uncompresses a data block */
typedef ULONG (*PCABINET_CODEC_UNCOMPRESS)(PVOID OutputBuffer,
										   PVOID InputBuffer,
										   PLONG InputLength,
										   PLONG OutputLength);


/* Codec status codes */
#define CS_SUCCESS      0x0000  /* All data consumed */
#define CS_NOMEMORY     0x0001  /* Not enough free memory */
#define CS_BADSTREAM    0x0002  /* Bad data stream */

/* Codec indentifiers */
#define CAB_CODEC_RAW   0x00
#define CAB_CODEC_LZX   0x01
#define CAB_CODEC_MSZIP 0x02

#define MSZIP_MAGIC 0x4B43



/* Event handler prototypes */

typedef BOOL (*PCABINET_OVERWRITE)(PCFFILE File,
  PWCHAR FileName);

typedef VOID (*PCABINET_EXTRACT)(PCFFILE File,
  PWCHAR FileName);

typedef VOID (*PCABINET_DISK_CHANGE)(PWCHAR CabinetName,
  PWCHAR DiskLabel);



/* Classes */

/* Default constructor */
VOID CabinetInitialize();
/* Default destructor */
VOID CabinetCleanup();
/* Returns a pointer to the filename part of a fully qualified filename */
PWCHAR CabinetGetFileName(PWCHAR Path);
/* Removes a filename from a fully qualified filename */
VOID CabinetRemoveFileName(PWCHAR Path);
/* Normalizes a path */
BOOL CabinetNormalizePath(PWCHAR Path, ULONG Length);
/* Returns name of cabinet file */
PWCHAR CabinetGetCabinetName();
/* Sets the name of the cabinet file */
VOID CabinetSetCabinetName(PWCHAR FileName);
/* Sets destination path for extracted files */
VOID CabinetSetDestinationPath(PWCHAR DestinationPath);
/* Returns destination path */
PWCHAR CabinetGetDestinationPath();
/* Returns zero-based current disk number */
ULONG CabinetGetCurrentDiskNumber();
/* Opens the current cabinet file */
ULONG CabinetOpen();
/* Closes the current open cabinet file */
VOID CabinetClose();
/* Locates the first file in the current cabinet file that matches a search criteria */
ULONG CabinetFindFirst(PWCHAR FileName, PCAB_SEARCH Search);
/* Locates the next file in the current cabinet file */
ULONG CabinetFindNext(PCAB_SEARCH Search);
/* Extracts a file from the current cabinet file */
ULONG CabinetExtractFile(PCAB_SEARCH Search);
/* Select codec engine to use */
VOID CabinetSelectCodec(ULONG Id);
/* Set event handlers */
VOID CabinetSetEventHandlers(PCABINET_OVERWRITE Overwrite,
  PCABINET_EXTRACT Extract,
  PCABINET_DISK_CHANGE DiskChange);
/* Get pointer to cabinet reserved area. NULL if none */
PVOID CabinetGetCabinetReservedArea(PULONG Size);

#endif /* __CABINET_H */
