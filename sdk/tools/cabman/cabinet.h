/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        tools/cabman/cabinet.h
 * PURPOSE:     Cabinet definitions
 */

#pragma once

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <intrin.h>
    #include <windows.h>
#else
    #include <typedefs.h>
    #include <unistd.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <string>
#include <list>

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#if !defined(C_ASSERT)
#define C_ASSERT(expr) extern char (*c_assert(void)) [(expr) ? 1 : -1]
#endif

#ifndef _countof
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

#if defined(_WIN32)
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STRING "\\"

#define snprintf _snprintf
#define strcasecmp _stricmp
#define strdup _strdup
#else
#define DIR_SEPARATOR_CHAR '/'
#define DIR_SEPARATOR_STRING "/"
#endif // _WIN32

inline LONG GetSizeOfFile(FILE* handle)
{
    LONG size;
    LONG currentPos = ftell(handle);

    if (fseek(handle, 0, SEEK_END) != 0)
        return (LONG)-1;

    size = ftell(handle);
    fseek(handle, 0, SEEK_SET);
    return size;
}

/* Debugging */

#define NORMAL_MASK    0x000000FF
#define SPECIAL_MASK   0xFFFFFF00
#define MIN_TRACE      0x00000001
#define MID_TRACE      0x00000002
#define MAX_TRACE      0x00000003

#define DEBUG_MEMORY   0x00000100

#if DBG

extern ULONG DebugTraceLevel;

#undef DPRINT
#define DPRINT(_t_, _x_) \
    if ((_t_ > NORMAL_MASK) \
        ? (DebugTraceLevel & _t_) > NORMAL_MASK \
        : (DebugTraceLevel & NORMAL_MASK) >= _t_) { \
        printf("(%s:%d)(%s) ", __FILE__, __LINE__, __FUNCTION__); \
        printf _x_ ; \
    }

#undef ASSERT
#define ASSERT(_b_) { \
    if (!(_b_)) { \
        printf("(%s:%d)(%s) ASSERTION: ", __FILE__, __LINE__, __FUNCTION__); \
        printf(#_b_); \
        exit(0); \
    } \
}

#else /* DBG */

#undef DPRINT
#define DPRINT(_t_, _x_) do { } while(0)

#undef ASSERT
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
    ULONG Signature;        // File signature 'MSCF' (CAB_SIGNATURE)
    ULONG Reserved1;        // Reserved field
    ULONG CabinetSize;      // Cabinet file size
    ULONG Reserved2;        // Reserved field
    ULONG FileTableOffset;  // Offset of first CFFILE
    ULONG Reserved3;        // Reserved field
    USHORT Version;          // Cabinet version (CAB_VERSION)
    USHORT FolderCount;      // Number of folders
    USHORT FileCount;        // Number of files
    USHORT Flags;            // Cabinet flags (CAB_FLAG_*)
    USHORT SetID;            // Cabinet set id
    USHORT CabinetNumber;    // Zero-based cabinet number
/* Optional fields (depends on Flags)
    USHORT CabinetResSize    // Per-cabinet reserved area size
    char     FolderResSize     // Per-folder reserved area size
    char     FileResSize       // Per-file reserved area size
    char     CabinetReserved[] // Per-cabinet reserved area
    char     CabinetPrev[]     // Name of previous cabinet file
    char     DiskPrev[]        // Name of previous disk
    char     CabinetNext[]     // Name of next cabinet file
    char     DiskNext[]        // Name of next disk
 */
} CFHEADER, *PCFHEADER;


typedef struct _CFFOLDER
{
    ULONG DataOffset;       // Absolute offset of first CFDATA block in this folder
    USHORT DataBlockCount;   // Number of CFDATA blocks in this folder in this cabinet
    USHORT CompressionType;  // Type of compression used for all CFDATA blocks in this folder
/* Optional fields (depends on Flags)
    char     FolderReserved[]  // Per-folder reserved area
 */
} CFFOLDER, *PCFFOLDER;

C_ASSERT(sizeof(CFFOLDER) == 8);


typedef struct _CFFILE
{
    ULONG FileSize;         // Uncompressed file size in bytes
    ULONG FileOffset;       // Uncompressed offset of file in the folder
    USHORT FileControlID;    // File control ID (CAB_FILE_*)
    USHORT FileDate;         // File date stamp, as used by DOS
    USHORT FileTime;         // File time stamp, as used by DOS
    USHORT Attributes;       // File attributes (CAB_ATTRIB_*)
    /* After this is the NULL terminated filename */
} CFFILE, *PCFFILE;

C_ASSERT(sizeof(CFFILE) == 16);

typedef struct _CFDATA
{
    ULONG Checksum;         // Checksum of CFDATA entry
    USHORT CompSize;         // Number of compressed bytes in this block
    USHORT UncompSize;       // Number of uncompressed bytes in this block
/* Optional fields (depends on Flags)
    char  DataReserved[]    // Per-datablock reserved area
 */
} CFDATA, *PCFDATA;

C_ASSERT(sizeof(CFDATA) == 8);

/* Application structures */

typedef struct _CFDATA_NODE
{
    ULONG       ScratchFilePosition = 0;    // Absolute offset in scratch file
    ULONG       AbsoluteOffset = 0;         // Absolute offset in cabinet
    ULONG       UncompOffset = 0;           // Uncompressed offset in folder
    CFDATA      Data = { 0 };
} CFDATA_NODE, *PCFDATA_NODE;

typedef struct _CFFOLDER_NODE
{
    ULONG           UncompOffset = 0;       // File size accumulator
    ULONG           AbsoluteOffset = 0;
    ULONG           TotalFolderSize = 0;    // Total size of folder in current disk
    std::list<PCFDATA_NODE> DataList;
    ULONG           Index = 0;
    bool            Commit = false;         // true if the folder should be committed
    bool            Delete = false;         // true if marked for deletion
    CFFOLDER        Folder = { 0 };
} CFFOLDER_NODE, *PCFFOLDER_NODE;

typedef struct _CFFILE_NODE
{
    CFFILE              File = { 0 };
    std::string         FileName;
    std::string         TargetFolder;
    PCFDATA_NODE        DataBlock = nullptr;    // First data block of file. NULL if not known
    bool                Commit = false;         // true if the file data should be committed
    bool                Delete = false;         // true if marked for deletion
    PCFFOLDER_NODE      FolderNode = nullptr;   // Folder this file belong to
} CFFILE_NODE, *PCFFILE_NODE;

typedef struct _SEARCH_CRITERIA
{
    std::string             Search;             // The actual search criteria
    std::string             TargetFolder;       // The filename will be TargetFolder\file
} SEARCH_CRITERIA, *PSEARCH_CRITERIA;

typedef struct _CAB_SEARCH
{
    std::list<PCFFILE_NODE>::iterator      Next;   // Pointer to next node
    PCFFILE           File = nullptr;   // Pointer to current CFFILE
    std::string       FileName;         // Current filename
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

class CCABCodec
{
public:
    /* Default constructor */
    CCABCodec() {};
    /* Default destructor */
    virtual ~CCABCodec() {};
    /* Compresses a data block */
    virtual ULONG Compress(void* OutputBuffer,
                           void* InputBuffer,
                           ULONG InputLength,
                           PULONG OutputLength) = 0;
    /* Uncompresses a data block */
    virtual ULONG Uncompress(void* OutputBuffer,
                             void* InputBuffer,
                             ULONG InputLength,
                             PULONG OutputLength) = 0;
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

class CCabinet
{
public:
    /* Default constructor */
    CCabinet();
    /* Default destructor */
    virtual ~CCabinet();
    /* Determines if a character is a separator */
    bool IsSeparator(char Char);
    /* Replaces \ or / with the one used be the host environment */
    void ConvertPath(std::string& Path);
    /* Returns the filename part of a fully qualified filename */
    std::string GetFileName(const std::string& Path);
    /* Normalizes a path */
    void NormalizePath(std::string& Path);
    /* Returns name of cabinet file */
    char* GetCabinetName();
    /* Sets the name of the cabinet file */
    void SetCabinetName(const char* FileName);
    /* Sets destination path for extracted files */
    void SetDestinationPath(const char* DestinationPath);
    /* Sets cabinet reserved file */
    bool SetCabinetReservedFile(const char* FileName);
    /* Returns destination path */
    const char* GetDestinationPath();
    /* Returns zero-based current disk number */
    ULONG GetCurrentDiskNumber();
    /* Opens the current cabinet file */
    ULONG Open();
    /* Closes the current open cabinet file */
    void Close();
    /* Locates the first file in the current cabinet file that matches a search criteria */
    ULONG FindFirst(PCAB_SEARCH Search);
    /* Locates the next file in the current cabinet file */
    ULONG FindNext(PCAB_SEARCH Search);
    /* Extracts a file from the current cabinet file */
    ULONG ExtractFile(const char* FileName);
    /* Select codec engine to use */
    void SelectCodec(LONG Id);
    /* Returns whether a codec engine is selected */
    bool IsCodecSelected();
    /* Adds a search criteria for adding files to a simple cabinet, displaying files in a cabinet or extracting them */
    ULONG AddSearchCriteria(const std::string& SearchCriteria, const std::string& TargetFolder);
    /* Destroys the search criteria list */
    void DestroySearchCriteria();
    /* Returns whether we have search criteria */
    bool HasSearchCriteria();

    std::string CreateCabFilename(PCFFILE_NODE Node);

#ifndef CAB_READ_ONLY
    /* Creates a simple cabinet based on the search criteria data */
    bool CreateSimpleCabinet();
    /* Sets the codec to use for compression (based on a string value) */
    bool SetCompressionCodec(const char* CodecName);
    /* Creates a new cabinet file */
    ULONG NewCabinet();
    /* Forces a new disk to be created */
    ULONG NewDisk();
    /* Forces a new folder to be created */
    ULONG NewFolder();
    /* Writes a file to scratch storage */
    ULONG WriteFileToScratchStorage(PCFFILE_NODE FileNode);
    /* Forces the current disk to be written */
    ULONG WriteDisk(ULONG MoreDisks);
    /* Commits the current disk */
    ULONG CommitDisk(ULONG MoreDisks);
    /* Closes the current disk */
    ULONG CloseDisk();
    /* Closes the current cabinet */
    ULONG CloseCabinet();
    /* Adds a file to the current disk */
    ULONG AddFile(const std::string& FileName, const std::string& TargetFolder);
    /* Sets the maximum size of the current disk */
    void SetMaxDiskSize(ULONG Size);
#endif /* CAB_READ_ONLY */

    /* Default event handlers */

    /* Handler called when a file is about to be overridden */
    virtual bool OnOverwrite(PCFFILE Entry, const char* FileName);
    /* Handler called when a file is about to be extracted */
    virtual void OnExtract(PCFFILE Entry, const char* FileName);
    /* Handler called when a new disk is to be processed */
    virtual void OnDiskChange(const char* CabinetName, const char* DiskLabel);

    virtual void OnVerboseMessage(const char* Message);
#ifndef CAB_READ_ONLY
    /* Handler called when a file is about to be added */
    virtual void OnAdd(PCFFILE Entry, const char* FileName);
    /* Handler called when a cabinet need a name */
    virtual bool OnCabinetName(ULONG Number, char* Name);
    /* Handler called when a disk needs a label */
    virtual bool OnDiskLabel(ULONG Number, char* Label);
#endif /* CAB_READ_ONLY */
private:
    PCFFOLDER_NODE LocateFolderNode(ULONG Index);
    ULONG GetAbsoluteOffset(PCFFILE_NODE File);
    ULONG LocateFile(const char* FileName, PCFFILE_NODE *File);
    ULONG ReadString(char* String, LONG MaxLength);
    ULONG ReadFileTable();
    ULONG ReadDataBlocks(PCFFOLDER_NODE FolderNode);
    PCFFOLDER_NODE NewFolderNode();
    PCFFILE_NODE NewFileNode();
    PCFDATA_NODE NewDataNode(PCFFOLDER_NODE FolderNode);
    void DestroyDataNodes(PCFFOLDER_NODE FolderNode);
    void DestroyFileNodes();
    void DestroyDeletedFileNodes();
    void DestroyFolderNodes();
    void DestroyDeletedFolderNodes();
    ULONG ComputeChecksum(void* Buffer, ULONG Size, ULONG Seed);
    ULONG ReadBlock(void* Buffer, ULONG Size, PULONG BytesRead);
    bool MatchFileNamePattern(const char* FileName, const char* Pattern);
#ifndef CAB_READ_ONLY
    ULONG InitCabinetHeader();
    ULONG WriteCabinetHeader(bool MoreDisks);
    ULONG WriteFolderEntries();
    ULONG WriteFileEntries();
    ULONG CommitDataBlocks(PCFFOLDER_NODE FolderNode);
    ULONG WriteDataBlock();
    ULONG GetAttributesOnFile(PCFFILE_NODE File);
    ULONG SetAttributesOnFile(char* FileName, USHORT FileAttributes);
    ULONG GetFileTimes(FILE* FileHandle, PCFFILE_NODE File);
#if !defined(_WIN32)
    void ConvertDateAndTime(time_t* Time, PUSHORT DosDate, PUSHORT DosTime);
#endif
#endif /* CAB_READ_ONLY */
    ULONG CurrentDiskNumber;    // Zero based disk number
    char CabinetName[256];     // Filename of current cabinet
    char CabinetPrev[256];     // Filename of previous cabinet
    char DiskPrev[256];        // Label of cabinet in file CabinetPrev
    char CabinetNext[256];     // Filename of next cabinet
    char DiskNext[256];        // Label of cabinet in file CabinetNext
    ULONG TotalHeaderSize;      // Size of header and optional fields
    ULONG NextFieldsSize;       // Size of next cabinet name and next disk label
    ULONG TotalFolderSize;      // Size of all folder entries
    ULONG TotalFileSize;        // Size of all file entries
    ULONG FolderUncompSize;     // Uncompressed size of folder
    ULONG BytesLeftInBlock;     // Number of bytes left in current block
    bool ReuseBlock;
    std::string DestPath;
    std::string CabinetReservedFile;
    void* CabinetReservedFileBuffer;
    ULONG CabinetReservedFileSize;
    FILE* FileHandle;
    bool FileOpen;
    CFHEADER CABHeader;
    ULONG CabinetReserved;
    ULONG FolderReserved;
    ULONG DataReserved;
    std::list<PCFFOLDER_NODE> FolderList;
    PCFFOLDER_NODE CurrentFolderNode;
    PCFDATA_NODE CurrentDataNode;
    std::list<PCFFILE_NODE> FileList;
    std::list<PSEARCH_CRITERIA> CriteriaList;
    CCABCodec *Codec;
    LONG CodecId;
    bool CodecSelected;
    void* InputBuffer;
    void* CurrentIBuffer;               // Current offset in input buffer
    ULONG CurrentIBufferSize;   // Bytes left in input buffer
    void* OutputBuffer;
    ULONG TotalCompSize;        // Total size of current CFDATA block
    void* CurrentOBuffer;               // Current offset in output buffer
    ULONG CurrentOBufferSize;   // Bytes left in output buffer
    ULONG BytesLeftInCabinet;
    bool RestartSearch;
    ULONG LastFileOffset;       // Uncompressed offset of last extracted file
#ifndef CAB_READ_ONLY
    ULONG LastBlockStart;       // Uncompressed offset of last block in folder
    ULONG MaxDiskSize;
    ULONG DiskSize;
    ULONG PrevCabinetNumber;    // Previous cabinet number (where split file starts)
    bool CreateNewDisk;
    bool CreateNewFolder;

    class CCFDATAStorage *ScratchFile;
    FILE* SourceFile;
    bool ContinueFile;
    ULONG TotalBytesLeft;
    bool BlockIsSplit;                  // true if current data block is split
    ULONG NextFolderNumber;     // Zero based folder number
#endif /* CAB_READ_ONLY */
};

/* EOF */
