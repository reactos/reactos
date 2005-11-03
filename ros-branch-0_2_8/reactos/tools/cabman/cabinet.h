/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        tools/cabman/cabinet.h
 * PURPOSE:     Cabinet definitions
 */
#ifndef __CABINET_H
#define __CABINET_H

#if defined(WIN32)
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(WIN32)
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STRING "\\"

#define strcasecmp strcmpi
#define AllocateMemory(size) HeapAlloc(GetProcessHeap(), 0, size)
#define FreeMemory(buffer) HeapFree(GetProcessHeap(), 0, buffer)
#define FILEHANDLE HANDLE
#define CloseFile(handle) CloseHandle(handle)
#else
#define DIR_SEPARATOR_CHAR '/'
#define DIR_SEPARATOR_STRING "/"

#define AllocateMemory(size) malloc(size)
#define FreeMemory(buffer) free(buffer)
#define CloseFile(handle) fclose(handle)
#define FILEHANDLE FILE*
#endif

/* Debugging */

#define DBG

#define NORMAL_MASK    0x000000FF
#define SPECIAL_MASK   0xFFFFFF00
#define MIN_TRACE      0x00000001
#define MID_TRACE      0x00000002
#define MAX_TRACE      0x00000003

#define DEBUG_MEMORY   0x00000100

#ifdef DBG

extern unsigned long DebugTraceLevel;

#ifdef _MSC_VER
#define __FUNCTION__ ""
#endif//_MSC_VER

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
        exit(0); \
    } \
}

#else /* DBG */

#define DPRINT(_t_, _x_)

#define ASSERT(_x_)

#endif /* DBG */


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
    unsigned long Signature;        // File signature 'MSCF' (CAB_SIGNATURE)
    unsigned long Reserved1;        // Reserved field
    unsigned long CabinetSize;      // Cabinet file size
    unsigned long Reserved2;        // Reserved field
    unsigned long FileTableOffset;  // Offset of first CFFILE
    unsigned long Reserved3;        // Reserved field
    unsigned short  Version;          // Cabinet version (CAB_VERSION)
    unsigned short  FolderCount;      // Number of folders
    unsigned short  FileCount;        // Number of files
    unsigned short  Flags;            // Cabinet flags (CAB_FLAG_*)
    unsigned short  SetID;            // Cabinet set id
    unsigned short  CabinetNumber;    // Zero-based cabinet number
/* Optional fields (depends on Flags)
    unsigned short  CabinetResSize    // Per-cabinet reserved area size
    char  FolderResSize     // Per-folder reserved area size
    char  FileResSize       // Per-file reserved area size
    char  CabinetReserved[] // Per-cabinet reserved area
    char  CabinetPrev[]     // Name of previous cabinet file
    char  DiskPrev[]        // Name of previous disk
    char  CabinetNext[]     // Name of next cabinet file
    char  DiskNext[]        // Name of next disk
 */
} CFHEADER, *PCFHEADER;


typedef struct _CFFOLDER
{
    unsigned long  DataOffset;       // Absolute offset of first CFDATA block in this folder
    unsigned short DataBlockCount;   // Number of CFDATA blocks in this folder in this cabinet
    unsigned short CompressionType;  // Type of compression used for all CFDATA blocks in this folder
/* Optional fields (depends on Flags)
    char  FolderReserved[]  // Per-folder reserved area
 */
} CFFOLDER, *PCFFOLDER;


typedef struct _CFFILE
{
    unsigned long  FileSize;         // Uncompressed file size in bytes
    unsigned long  FileOffset;       // Uncompressed offset of file in the folder
    unsigned short FileControlID;    // File control ID (CAB_FILE_*)
    unsigned short FileDate;         // File date stamp, as used by DOS
    unsigned short FileTime;         // File time stamp, as used by DOS
    unsigned short Attributes;       // File attributes (CAB_ATTRIB_*)
    /* After this is the NULL terminated filename */
} CFFILE, *PCFFILE;


typedef struct _CFDATA
{
    unsigned long  Checksum;         // Checksum of CFDATA entry
    unsigned short CompSize;         // Number of compressed bytes in this block
    unsigned short UncompSize;       // Number of uncompressed bytes in this block
/* Optional fields (depends on Flags)
    char  DataReserved[]    // Per-datablock reserved area
 */
} CFDATA, *PCFDATA;

typedef struct _CFDATA_NODE
{
    struct _CFDATA_NODE *Next;
    struct _CFDATA_NODE *Prev;
    unsigned long       ScratchFilePosition;    // Absolute offset in scratch file
    unsigned long       AbsoluteOffset;         // Absolute offset in cabinet
    unsigned long       UncompOffset;           // Uncompressed offset in folder
    CFDATA              Data;
} CFDATA_NODE, *PCFDATA_NODE;

typedef struct _CFFOLDER_NODE
{
    struct _CFFOLDER_NODE *Next;
    struct _CFFOLDER_NODE *Prev;
    unsigned long         UncompOffset;     // File size accumulator
    unsigned long         AbsoluteOffset;
    unsigned long         TotalFolderSize;  // Total size of folder in current disk
    PCFDATA_NODE          DataListHead;
    PCFDATA_NODE          DataListTail;
    unsigned long         Index;
    bool                  Commit;           // true if the folder should be committed
    bool                  Delete;           // true if marked for deletion
    CFFOLDER              Folder;
} CFFOLDER_NODE, *PCFFOLDER_NODE;

typedef struct _CFFILE_NODE
{
    struct _CFFILE_NODE *Next;
    struct _CFFILE_NODE *Prev;
    CFFILE              File;
    char*               FileName;
    PCFDATA_NODE        DataBlock;      // First data block of file. NULL if not known
    bool                Commit;         // true if the file data should be committed
    bool                Delete;         // true if marked for deletion
	  PCFFOLDER_NODE		  FolderNode;		  // Folder this file belong to
} CFFILE_NODE, *PCFFILE_NODE;


typedef struct _CAB_SEARCH
{
    char        Search[MAX_PATH];   // Search criteria
    PCFFILE_NODE Next;               // Pointer to next node
    PCFFILE      File;               // Pointer to current CFFILE
    char*        FileName;           // Current filename
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
    virtual unsigned long Compress(void* OutputBuffer,
                           void* InputBuffer,
                           unsigned long InputLength,
                           unsigned long* OutputLength) = 0;
    /* Uncompresses a data block */
    virtual unsigned long Uncompress(void* OutputBuffer,
                             void* InputBuffer,
                             unsigned long InputLength,
                             unsigned long* OutputLength) = 0;
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
    unsigned long Create(char* FileName);
    unsigned long Destroy();
    unsigned long Truncate();
    unsigned long Position();
    unsigned long Seek(long Position);
    unsigned long ReadBlock(PCFDATA Data, void* Buffer, unsigned long* BytesRead);
    unsigned long WriteBlock(PCFDATA Data, void* Buffer, unsigned long* BytesWritten);
private:
    char FullName[MAX_PATH];
    bool FileCreated;
    FILEHANDLE FileHandle;
};

#endif /* CAB_READ_ONLY */

class CCabinet {
public:
    /* Default constructor */
    CCabinet();
    /* Default destructor */
    virtual ~CCabinet();
    /* Determines if a character is a separator */
    bool CCabinet::IsSeparator(char Char);
    /* Replaces \ or / with the one used be the host environment */
    char* CCabinet::ConvertPath(char* Path, bool Allocate);
    /* Returns a pointer to the filename part of a fully qualified filename */
    char* GetFileName(char* Path);
    /* Removes a filename from a fully qualified filename */
    void RemoveFileName(char* Path);
    /* Normalizes a path */
    bool NormalizePath(char* Path, unsigned long Length);
    /* Returns name of cabinet file */
    char* GetCabinetName();
    /* Sets the name of the cabinet file */
    void SetCabinetName(char* FileName);
    /* Sets destination path for extracted files */
    void SetDestinationPath(char* DestinationPath);
    /* Sets cabinet reserved file */
    bool SetCabinetReservedFile(char* FileName);
    /* Returns cabinet reserved file */
    char* GetCabinetReservedFile();
    /* Returns destination path */
    char* GetDestinationPath();
    /* Returns zero-based current disk number */
    unsigned long GetCurrentDiskNumber();
    /* Opens the current cabinet file */
    unsigned long Open();
    /* Closes the current open cabinet file */
    void Close();
    /* Locates the first file in the current cabinet file that matches a search criteria */
    unsigned long FindFirst(char* FileName, PCAB_SEARCH Search);
    /* Locates the next file in the current cabinet file */
    unsigned long FindNext(PCAB_SEARCH Search);
    /* Extracts a file from the current cabinet file */
    unsigned long ExtractFile(char* FileName);
    /* Select codec engine to use */
    void SelectCodec(unsigned long Id);
#ifndef CAB_READ_ONLY
    /* Creates a new cabinet file */
    unsigned long NewCabinet();
    /* Forces a new disk to be created */
    unsigned long NewDisk();
    /* Forces a new folder to be created */
    unsigned long NewFolder();
    /* Writes a file to scratch storage */
    unsigned long WriteFileToScratchStorage(PCFFILE_NODE FileNode);
    /* Forces the current disk to be written */
    unsigned long WriteDisk(unsigned long MoreDisks);
    /* Commits the current disk */
    unsigned long CommitDisk(unsigned long MoreDisks);
    /* Closes the current disk */
    unsigned long CloseDisk();
    /* Closes the current cabinet */
    unsigned long CloseCabinet();
    /* Adds a file to the current disk */
    unsigned long AddFile(char* FileName);
    /* Sets the maximum size of the current disk */
    void SetMaxDiskSize(unsigned long Size);
#endif /* CAB_READ_ONLY */

    /* Default event handlers */

    /* Handler called when a file is about to be overridden */
    virtual bool OnOverwrite(PCFFILE Entry, char* FileName);
    /* Handler called when a file is about to be extracted */
    virtual void OnExtract(PCFFILE Entry, char* FileName);
    /* Handler called when a new disk is to be processed */
    virtual void OnDiskChange(char* CabinetName, char* DiskLabel);
#ifndef CAB_READ_ONLY
    /* Handler called when a file is about to be added */
    virtual void OnAdd(PCFFILE Entry, char* FileName);
    /* Handler called when a cabinet need a name */
    virtual bool OnCabinetName(unsigned long Number, char* Name);
    /* Handler called when a disk needs a label */
    virtual bool OnDiskLabel(unsigned long Number, char* Label);
#endif /* CAB_READ_ONLY */
private:
    PCFFOLDER_NODE LocateFolderNode(unsigned long Index);
    unsigned long GetAbsoluteOffset(PCFFILE_NODE File);
    unsigned long LocateFile(char* FileName, PCFFILE_NODE *File);
    unsigned long ReadString(char* String, unsigned long MaxLength);
    unsigned long ReadFileTable();
    unsigned long ReadDataBlocks(PCFFOLDER_NODE FolderNode);
    PCFFOLDER_NODE NewFolderNode();
    PCFFILE_NODE NewFileNode();
    PCFDATA_NODE NewDataNode(PCFFOLDER_NODE FolderNode);
    void DestroyDataNodes(PCFFOLDER_NODE FolderNode);
    void DestroyFileNodes();
    void DestroyDeletedFileNodes();
    void DestroyFolderNodes();
    void DestroyDeletedFolderNodes();
    unsigned long ComputeChecksum(void* Buffer, unsigned int Size, unsigned long Seed);
    unsigned long ReadBlock(void* Buffer, unsigned long Size, unsigned long* BytesRead);
#ifndef CAB_READ_ONLY
    unsigned long InitCabinetHeader();
    unsigned long WriteCabinetHeader(bool MoreDisks);
    unsigned long WriteFolderEntries();
    unsigned long WriteFileEntries();
    unsigned long CommitDataBlocks(PCFFOLDER_NODE FolderNode);
    unsigned long WriteDataBlock();
    unsigned long GetAttributesOnFile(PCFFILE_NODE File);
    unsigned long SetAttributesOnFile(PCFFILE_NODE File);
    unsigned long GetFileTimes(FILEHANDLE FileHandle, PCFFILE_NODE File);
#if !defined(WIN32)
    void ConvertDateAndTime(time_t* Time, unsigned short* DosDate, unsigned short* DosTime);
#endif
#endif /* CAB_READ_ONLY */
    unsigned long CurrentDiskNumber;    // Zero based disk number
    char CabinetName[256];     // Filename of current cabinet
    char CabinetPrev[256];     // Filename of previous cabinet
    char DiskPrev[256];        // Label of cabinet in file CabinetPrev
    char CabinetNext[256];     // Filename of next cabinet
    char DiskNext[256];        // Label of cabinet in file CabinetNext
    unsigned long TotalHeaderSize;      // Size of header and optional fields
    unsigned long NextFieldsSize;       // Size of next cabinet name and next disk label
    unsigned long TotalFolderSize;      // Size of all folder entries
    unsigned long TotalFileSize;        // Size of all file entries
    unsigned long FolderUncompSize;     // Uncompressed size of folder
    unsigned long BytesLeftInBlock;     // Number of bytes left in current block
    bool ReuseBlock;
    char DestPath[MAX_PATH];
    char CabinetReservedFile[MAX_PATH];
    void* CabinetReservedFileBuffer;
    unsigned long CabinetReservedFileSize;
    FILEHANDLE FileHandle;
    bool FileOpen;
    CFHEADER CABHeader;
    unsigned long CabinetReserved;
    unsigned long FolderReserved;
    unsigned long DataReserved;
    PCFFOLDER_NODE FolderListHead;
    PCFFOLDER_NODE FolderListTail;
    PCFFOLDER_NODE CurrentFolderNode;
    PCFDATA_NODE CurrentDataNode;
    PCFFILE_NODE FileListHead;
    PCFFILE_NODE FileListTail;
    CCABCodec *Codec;
    unsigned long CodecId;
    bool CodecSelected;
    void* InputBuffer;
    void* CurrentIBuffer;               // Current offset in input buffer
    unsigned long CurrentIBufferSize;   // Bytes left in input buffer
    void* OutputBuffer;
    unsigned long TotalCompSize;        // Total size of current CFDATA block
    void* CurrentOBuffer;               // Current offset in output buffer
    unsigned long CurrentOBufferSize;   // Bytes left in output buffer
    unsigned long BytesLeftInCabinet;
    bool RestartSearch;
    unsigned long LastFileOffset;       // Uncompressed offset of last extracted file
#ifndef CAB_READ_ONLY
    unsigned long LastBlockStart;       // Uncompressed offset of last block in folder
    unsigned long MaxDiskSize;
    unsigned long DiskSize;
    unsigned long PrevCabinetNumber;    // Previous cabinet number (where split file starts)
    bool CreateNewDisk;
    bool CreateNewFolder;

    CCFDATAStorage *ScratchFile;
    FILEHANDLE SourceFile;
    bool ContinueFile;
    unsigned long TotalBytesLeft;
    bool BlockIsSplit;                  // true if current data block is split
    unsigned long NextFolderNumber;     // Zero based folder number
#endif /* CAB_READ_ONLY */
};

#endif /* __CABINET_H */

/* EOF */
