/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        apps/cabman/cabinet.h
 * PURPOSE:     Cabinet definitions
 */
#ifndef __CABINET_H
#define __CABINET_H

#include <string.h>

/* Debugging */

#define DBG

#define NORMAL_MASK    0x000000FF
#define SPECIAL_MASK   0xFFFFFF00
#define MIN_TRACE      0x00000001
#define MID_TRACE      0x00000002
#define MAX_TRACE      0x00000003

#define DEBUG_MEMORY   0x00000100

#ifdef DBG

extern DWORD DebugTraceLevel;

#define DPRINT(_t_, _x_) \
    if (((DebugTraceLevel & NORMAL_MASK) >= _t_) || \
        ((DebugTraceLevel & _t_) > NORMAL_MASK)) { \
        printf("(%s:%d)(%s) ", __FILE__, __LINE__, __FUNCTION__); \
        printf _x_ ; \
    }

#define ASSERT(_b_) { \
    if (!(_b_)) { \
        printf("(%s:%d)(%s) ASSERTION: ", __FILE__, __LINE__, __FUNCTION__); \
        printf(#_b_); \
        ExitProcess(0); \
    } \
}

#else /* DBG */

#define DPRINT(_t_, _x_)

#define ASSERT(_x_)

#endif /* DBG */


/* Macros */

#ifndef CopyMemory
#define CopyMemory(destination, source, length) memcpy(destination, source, length)
#endif /* CopyMemory */

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
    DWORD Signature;        // File signature 'MSCF' (CAB_SIGNATURE)
    DWORD Reserved1;        // Reserved field
    DWORD CabinetSize;      // Cabinet file size
    DWORD Reserved2;        // Reserved field
    DWORD FileTableOffset;  // Offset of first CFFILE
    DWORD Reserved3;        // Reserved field
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
    DWORD DataOffset;       // Absolute offset of first CFDATA block in this folder
    WORD  DataBlockCount;   // Number of CFDATA blocks in this folder in this cabinet
    WORD  CompressionType;  // Type of compression used for all CFDATA blocks in this folder
/* Optional fields (depends on Flags)
    CHAR  FolderReserved[]  // Per-folder reserved area
 */
} CFFOLDER, *PCFFOLDER;


typedef struct _CFFILE
{
    DWORD FileSize;         // Uncompressed file size in bytes
    DWORD FileOffset;       // Uncompressed offset of file in the folder
    WORD  FileControlID;    // File control ID (CAB_FILE_*)
    WORD  FileDate;         // File date stamp, as used by DOS
    WORD  FileTime;         // File time stamp, as used by DOS
    WORD  Attributes;       // File attributes (CAB_ATTRIB_*)
    /* After this is the NULL terminated filename */
} CFFILE, *PCFFILE;


typedef struct _CFDATA
{
    DWORD Checksum;         // Checksum of CFDATA entry
    WORD  CompSize;         // Number of compressed bytes in this block
    WORD  UncompSize;       // Number of uncompressed bytes in this block
/* Optional fields (depends on Flags)
    CHAR  DataReserved[]    // Per-datablock reserved area
 */
} CFDATA, *PCFDATA;

typedef struct _CFDATA_NODE
{
    struct _CFDATA_NODE *Next;
    struct _CFDATA_NODE *Prev;
    DWORD               ScratchFilePosition;    // Absolute offset in scratch file
    DWORD               AbsoluteOffset;         // Absolute offset in cabinet
    DWORD               UncompOffset;           // Uncompressed offset in folder
    CFDATA              Data;
} CFDATA_NODE, *PCFDATA_NODE;

typedef struct _CFFOLDER_NODE
{
    struct _CFFOLDER_NODE *Next;
    struct _CFFOLDER_NODE *Prev;
    DWORD                 UncompOffset;     // File size accumulator
    DWORD                 AbsoluteOffset;
    DWORD                 TotalFolderSize;  // Total size of folder in current disk
    PCFDATA_NODE          DataListHead;
    PCFDATA_NODE          DataListTail;
    DWORD                 Index;
    BOOL                  Commit;           // TRUE if the folder should be committed
    BOOL                  Delete;           // TRUE if marked for deletion
    CFFOLDER              Folder;
} CFFOLDER_NODE, *PCFFOLDER_NODE;

typedef struct _CFFILE_NODE
{
    struct _CFFILE_NODE *Next;
    struct _CFFILE_NODE *Prev;
    CFFILE              File;
    LPTSTR              FileName;
    PCFDATA_NODE        DataBlock;      // First data block of file. NULL if not known
    BOOL                Commit;         // TRUE if the file data should be committed
    BOOL                Delete;         // TRUE if marked for deletion
	PCFFOLDER_NODE		FolderNode;		// Folder this file belong to
} CFFILE_NODE, *PCFFILE_NODE;


typedef struct _CAB_SEARCH
{
    TCHAR        Search[MAX_PATH];   // Search criteria
    PCFFILE_NODE Next;               // Pointer to next node
    PCFFILE      File;               // Pointer to current CFFILE
    LPTSTR       FileName;           // Current filename
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

class CCABCodec {
public:
    /* Default constructor */
    CCABCodec() {};
    /* Default destructor */
    virtual ~CCABCodec() {};
    /* Compresses a data block */
    virtual ULONG Compress(PVOID OutputBuffer,
                           PVOID InputBuffer,
                           DWORD InputLength,
                           PDWORD OutputLength) = 0;
    /* Uncompresses a data block */
    virtual ULONG Uncompress(PVOID OutputBuffer,
                             PVOID InputBuffer,
                             DWORD InputLength,
                             PDWORD OutputLength) = 0;
};


/* Codec status codes */
#define CS_SUCCESS      0x0000  /* All data consumed */
#define CS_NOMEMORY     0x0001  /* Not enough free memory */
#define CS_BADSTREAM    0x0002  /* Bad data stream */


/* Codec indentifiers */
#define CAB_CODEC_RAW   0x00
#define CAB_CODEC_LZX   0x01
#define CAB_CODEC_MSZIP 0x02



/* Classes */

#ifndef CAB_READ_ONLY

class CCFDATAStorage {
public:
    /* Default constructor */
    CCFDATAStorage();
    /* Default destructor */
    virtual ~CCFDATAStorage();
    ULONG Create(LPTSTR FileName);
    ULONG Destroy();
    ULONG Truncate();
    ULONG Position();
    ULONG Seek(LONG Position);
    ULONG ReadBlock(PCFDATA Data, PVOID Buffer, PDWORD BytesRead);
    ULONG WriteBlock(PCFDATA Data, PVOID Buffer, PDWORD BytesWritten);
private:
    BOOL FileCreated;
    HANDLE FileHandle;
};

#endif /* CAB_READ_ONLY */

class CCabinet {
public:
    /* Default constructor */
    CCabinet();
    /* Default destructor */
    virtual ~CCabinet();
    /* Returns a pointer to the filename part of a fully qualified filename */
    LPTSTR GetFileName(LPTSTR Path);
    /* Removes a filename from a fully qualified filename */
    VOID RemoveFileName(LPTSTR Path);
    /* Normalizes a path */
    BOOL NormalizePath(LPTSTR Path, DWORD Length);
    /* Returns name of cabinet file */
    LPTSTR GetCabinetName();
    /* Sets the name of the cabinet file */
    VOID SetCabinetName(LPTSTR FileName);
    /* Sets destination path for extracted files */
    VOID SetDestinationPath(LPTSTR DestinationPath);
    /* Returns destination path */
    LPTSTR GetDestinationPath();
    /* Returns zero-based current disk number */
    DWORD GetCurrentDiskNumber();
    /* Opens the current cabinet file */
    ULONG Open();
    /* Closes the current open cabinet file */
    VOID Close();
    /* Locates the first file in the current cabinet file that matches a search criteria */
    ULONG FindFirst(LPTSTR FileName, PCAB_SEARCH Search);
    /* Locates the next file in the current cabinet file */
    ULONG FindNext(PCAB_SEARCH Search);
    /* Extracts a file from the current cabinet file */
    ULONG ExtractFile(LPTSTR FileName);
    /* Select codec engine to use */
    VOID SelectCodec(ULONG Id);
#ifndef CAB_READ_ONLY
    /* Creates a new cabinet file */
    ULONG NewCabinet();
    /* Forces a new disk to be created */
    ULONG NewDisk();
    /* Forces a new folder to be created */
    ULONG NewFolder();
    /* Writes a file to scratch storage */
    ULONG WriteFileToScratchStorage(PCFFILE_NODE FileNode);
    /* Forces the current disk to be written */
    ULONG WriteDisk(DWORD MoreDisks);
    /* Commits the current disk */
    ULONG CommitDisk(DWORD MoreDisks);
    /* Closes the current disk */
    ULONG CloseDisk();
    /* Closes the current cabinet */
    ULONG CloseCabinet();
    /* Adds a file to the current disk */
    ULONG AddFile(LPTSTR FileName);
    /* Sets the maximum size of the current disk */
    VOID SetMaxDiskSize(DWORD Size);
#endif /* CAB_READ_ONLY */

    /* Default event handlers */

    /* Handler called when a file is about to be overridden */
    virtual BOOL OnOverwrite(PCFFILE Entry, LPTSTR FileName);
    /* Handler called when a file is about to be extracted */
    virtual VOID OnExtract(PCFFILE Entry, LPTSTR FileName);
    /* Handler called when a new disk is to be processed */
    virtual VOID OnDiskChange(LPTSTR CabinetName, LPTSTR DiskLabel);
#ifndef CAB_READ_ONLY
    /* Handler called when a file is about to be added */
    virtual VOID OnAdd(PCFFILE Entry, LPTSTR FileName);
    /* Handler called when a cabinet need a name */
    virtual BOOL OnCabinetName(ULONG Number, LPTSTR Name);
    /* Handler called when a disk needs a label */
    virtual BOOL OnDiskLabel(ULONG Number, LPTSTR Label);
#endif /* CAB_READ_ONLY */
private:
    PCFFOLDER_NODE LocateFolderNode(DWORD Index);
    ULONG GetAbsoluteOffset(PCFFILE_NODE File);
    ULONG LocateFile(LPTSTR FileName, PCFFILE_NODE *File);
    ULONG ReadString(LPTSTR String, DWORD MaxLength);
    ULONG ReadFileTable();
    ULONG ReadDataBlocks(PCFFOLDER_NODE FolderNode);
    PCFFOLDER_NODE NewFolderNode();
    PCFFILE_NODE NewFileNode();
    PCFDATA_NODE NewDataNode(PCFFOLDER_NODE FolderNode);
    VOID DestroyDataNodes(PCFFOLDER_NODE FolderNode);
    VOID DestroyFileNodes();
    VOID DestroyDeletedFileNodes();
    VOID DestroyFolderNodes();
    VOID DestroyDeletedFolderNodes();
    ULONG ComputeChecksum(PVOID Buffer, UINT Size, ULONG Seed);
    ULONG ReadBlock(PVOID Buffer, DWORD Size, PDWORD BytesRead);
#ifndef CAB_READ_ONLY
    ULONG InitCabinetHeader();
    ULONG WriteCabinetHeader(BOOL MoreDisks);
    ULONG WriteFolderEntries();
    ULONG WriteFileEntries();
    ULONG CommitDataBlocks(PCFFOLDER_NODE FolderNode);
    ULONG WriteDataBlock();
    ULONG GetAttributesOnFile(PCFFILE_NODE File);
    ULONG SetAttributesOnFile(PCFFILE_NODE File);
#endif /* CAB_READ_ONLY */
    DWORD CurrentDiskNumber;    // Zero based disk number
    TCHAR CabinetName[256];     // Filename of current cabinet
    TCHAR CabinetPrev[256];     // Filename of previous cabinet
    TCHAR DiskPrev[256];        // Label of cabinet in file CabinetPrev
    TCHAR CabinetNext[256];     // Filename of next cabinet
    TCHAR DiskNext[256];        // Label of cabinet in file CabinetNext
    DWORD TotalHeaderSize;      // Size of header and optional fields
    DWORD NextFieldsSize;       // Size of next cabinet name and next disk label
    DWORD TotalFolderSize;      // Size of all folder entries
    DWORD TotalFileSize;        // Size of all file entries
    DWORD FolderUncompSize;     // Uncompressed size of folder
    DWORD BytesLeftInBlock;     // Number of bytes left in current block
    BOOL ReuseBlock;
    TCHAR DestPath[MAX_PATH];
    HANDLE FileHandle;
    BOOL FileOpen;
    CFHEADER CABHeader;
    ULONG CabinetReserved;
    ULONG FolderReserved;
    ULONG DataReserved;
    PCFFOLDER_NODE FolderListHead;
    PCFFOLDER_NODE FolderListTail;
    PCFFOLDER_NODE CurrentFolderNode;
    PCFDATA_NODE CurrentDataNode;
    PCFFILE_NODE FileListHead;
    PCFFILE_NODE FileListTail;
    CCABCodec *Codec;
    ULONG CodecId;
    BOOL CodecSelected;
    PVOID InputBuffer;
    PVOID CurrentIBuffer;       // Current offset in input buffer
    DWORD CurrentIBufferSize;   // Bytes left in input buffer
    PVOID OutputBuffer;
    DWORD TotalCompSize;        // Total size of current CFDATA block
    PVOID CurrentOBuffer;       // Current offset in output buffer
    DWORD CurrentOBufferSize;   // Bytes left in output buffer
    DWORD BytesLeftInCabinet;
    BOOL RestartSearch;
    DWORD LastFileOffset;       // Uncompressed offset of last extracted file
#ifndef CAB_READ_ONLY
    DWORD LastBlockStart;       // Uncompressed offset of last block in folder
    DWORD MaxDiskSize;
    DWORD DiskSize;
    DWORD PrevCabinetNumber;    // Previous cabinet number (where split file starts)
    BOOL CreateNewDisk;
    BOOL CreateNewFolder;

    CCFDATAStorage *ScratchFile;
    HANDLE SourceFile;
    BOOL ContinueFile;
    DWORD TotalBytesLeft;
    BOOL BlockIsSplit;          // TRUE if current data block is split
    ULONG NextFolderNumber;     // Zero based folder number
#endif /* CAB_READ_ONLY */
};

#endif /* __CABINET_H */

/* EOF */
