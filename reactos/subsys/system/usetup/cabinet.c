/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS text-mode setup
 * FILE:        subsys/system/usetup/cabinet.c
 * PURPOSE:     Cabinet routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 15/08-2003 Created
 */
#include <ntos.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include "cabinet.h"
#include "usetup.h"

#define NDEBUG
#include <debug.h>

#define SEEK_BEGIN    0
#define SEEK_CURRENT  1
#ifndef SEEK_END
#define SEEK_END      2
#endif

typedef struct __DOSTIME
{
  WORD Second:5;
  WORD Minute:6;
  WORD Hour:5;
} DOSTIME, *PDOSTIME;


typedef struct __DOSDATE
{
  WORD Day:5;
  WORD Month:4;
  WORD Year:5;
} DOSDATE, *PDOSDATE;

static WCHAR CabinetName[256];          // Filename of current cabinet
static WCHAR CabinetPrev[256];          // Filename of previous cabinet
static WCHAR DiskPrev[256];             // Label of cabinet in file CabinetPrev
static WCHAR CabinetNext[256];          // Filename of next cabinet
static WCHAR DiskNext[256];             // Label of cabinet in file CabinetNext
static ULONG FolderUncompSize = 0;      // Uncompressed size of folder
static ULONG BytesLeftInBlock = 0;      // Number of bytes left in current block
static BOOL ReuseBlock = FALSE;
static WCHAR DestPath[MAX_PATH];
static HANDLE FileHandle;
static BOOL FileOpen = FALSE;
static CFHEADER CABHeader;
static ULONG CabinetReserved = 0;
static ULONG FolderReserved = 0;
static ULONG DataReserved = 0;
static PCFFOLDER_NODE FolderListHead = NULL;
static PCFFOLDER_NODE FolderListTail = NULL;
static PCFFOLDER_NODE CurrentFolderNode = NULL;
static PCFDATA_NODE CurrentDataNode = NULL;
static PCFFILE_NODE FileListHead = NULL;
static PCFFILE_NODE FileListTail = NULL;
static ULONG CodecId;
static PCABINET_CODEC_UNCOMPRESS CodecUncompress = NULL;
static BOOL CodecSelected = FALSE;
static PVOID InputBuffer = NULL;
static PVOID CurrentIBuffer = NULL;       // Current offset in input buffer
static ULONG CurrentIBufferSize = 0;      // Bytes left in input buffer
static PVOID OutputBuffer = NULL;
static PVOID CurrentOBuffer = NULL;       // Current offset in output buffer
static ULONG CurrentOBufferSize = 0;      // Bytes left in output buffer
static BOOL RestartSearch = FALSE;
static ULONG LastFileOffset = 0;          // Uncompressed offset of last extracted file
static PCABINET_OVERWRITE OverwriteHandler = NULL;
static PCABINET_EXTRACT ExtractHandler = NULL;
static PCABINET_DISK_CHANGE DiskChangeHandler = NULL;
static z_stream ZStream;
static PVOID CabinetReservedArea = NULL;


/* Needed by zlib, but we don't want the dependency on msvcrt.dll */

void free(void* _ptr)
{
  RtlFreeHeap(ProcessHeap, 0, _ptr);
}

void* calloc(size_t _nmemb, size_t _size)
{
  return (void*)RtlAllocateHeap (ProcessHeap, HEAP_ZERO_MEMORY, _size);
}

/* RAW codec */

ULONG
RawCodecUncompress(PVOID OutputBuffer,
  PVOID InputBuffer,
  ULONG InputLength,
  PULONG OutputLength)
/*
 * FUNCTION: Uncompresses data in a buffer
 * ARGUMENTS:
 *     OutputBuffer = Pointer to buffer to place uncompressed data
 *     InputBuffer  = Pointer to buffer with data to be uncompressed
 *     InputLength  = Length of input buffer
 *     OutputLength = Address of buffer to place size of uncompressed data
 */
{
  memcpy(OutputBuffer, InputBuffer, InputLength);
  *OutputLength = InputLength;
  return CS_SUCCESS;
}


/* MSZIP codec */

ULONG
MSZipCodecUncompress(PVOID OutputBuffer,
  PVOID InputBuffer,
  ULONG InputLength,
  PULONG OutputLength)
/*
 * FUNCTION: Uncompresses data in a buffer
 * ARGUMENTS:
 *     OutputBuffer = Pointer to buffer to place uncompressed data
 *     InputBuffer  = Pointer to buffer with data to be uncompressed
 *     InputLength  = Length of input buffer
 *     OutputLength = Address of buffer to place size of uncompressed data
 */
{
  USHORT Magic;
  INT Status;

  DPRINT("InputLength (%d).\n", InputLength);

  Magic = *((PUSHORT)InputBuffer);

  if (Magic != MSZIP_MAGIC)
    {
      DPRINT("Bad MSZIP block header magic (0x%X)\n", Magic);
      return CS_BADSTREAM;
    }
	
	ZStream.next_in   = (PUCHAR)((ULONG)InputBuffer + 2);
	ZStream.avail_in  = InputLength - 2;
	ZStream.next_out  = (PUCHAR)OutputBuffer;
  ZStream.avail_out = CAB_BLOCKSIZE + 12;

  /* WindowBits is passed < 0 to tell that there is no zlib header.
   * Note that in this case inflate *requires* an extra "dummy" byte
   * after the compressed stream in order to complete decompression and
   * return Z_STREAM_END.
   */
  Status = inflateInit2(&ZStream, -MAX_WBITS);
  if (Status != Z_OK)
    {
      DPRINT("inflateInit2() returned (%d).\n", Status);
      return CS_BADSTREAM;
    }

  while ((ZStream.total_out < CAB_BLOCKSIZE + 12) &&
    (ZStream.total_in < InputLength - 2))
    {
      Status = inflate(&ZStream, Z_NO_FLUSH);
      if (Status == Z_STREAM_END) break;
      if (Status != Z_OK)
        {
          DPRINT("inflate() returned (%d) (%s).\n", Status, ZStream.msg);
          if (Status == Z_MEM_ERROR)
              return CS_NOMEMORY;
          return CS_BADSTREAM;
        }
    }

  *OutputLength = ZStream.total_out;

  Status = inflateEnd(&ZStream);
  if (Status != Z_OK)
    {
      DPRINT("inflateEnd() returned (%d).\n", Status);
      return CS_BADSTREAM;
    }
  return CS_SUCCESS;
}



/* Memory functions */

voidpf MSZipAlloc(voidpf opaque, uInt items, uInt size)
{
  return (voidpf)RtlAllocateHeap (ProcessHeap, 0, items * size);
}

void MSZipFree (voidpf opaque, voidpf address)
{
  RtlFreeHeap(ProcessHeap, 0, address);
}


static DWORD
SeekInFile(HANDLE hFile,
  LONG lDistanceToMove,
  PLONG lpDistanceToMoveHigh,
  DWORD dwMoveMethod,
  PNTSTATUS Status)
{
  FILE_POSITION_INFORMATION FilePosition;
  FILE_STANDARD_INFORMATION FileStandard;
  NTSTATUS errCode;
  IO_STATUS_BLOCK IoStatusBlock;
  LARGE_INTEGER Distance;
  
  DPRINT("SeekInFile(hFile %x, lDistanceToMove %d, dwMoveMethod %d)\n",
    hFile,lDistanceToMove,dwMoveMethod);
  
  Distance.u.LowPart = lDistanceToMove;
  if (lpDistanceToMoveHigh)
    {
      Distance.u.HighPart = *lpDistanceToMoveHigh;
    }
  else if (lDistanceToMove >= 0)
    {
      Distance.u.HighPart = 0;
    }
  else
    {
      Distance.u.HighPart = -1;
    }
  
  if (dwMoveMethod == SEEK_CURRENT)
    {
      NtQueryInformationFile(hFile,
        &IoStatusBlock,
        &FilePosition,
        sizeof(FILE_POSITION_INFORMATION),
        FilePositionInformation);
        FilePosition.CurrentByteOffset.QuadPart += Distance.QuadPart;
    }
  else if (dwMoveMethod == SEEK_END)
    {
      NtQueryInformationFile(hFile,
        &IoStatusBlock,
        &FileStandard,
        sizeof(FILE_STANDARD_INFORMATION),
        FileStandardInformation);
        FilePosition.CurrentByteOffset.QuadPart =
        FileStandard.EndOfFile.QuadPart + Distance.QuadPart;
    }
  else if ( dwMoveMethod == SEEK_BEGIN )
    {
      FilePosition.CurrentByteOffset.QuadPart = Distance.QuadPart;
    }
  
//  DPRINT1("GOTO FILE OFFSET: %I64d\n", FilePosition.CurrentByteOffset.QuadPart);

  errCode = NtSetInformationFile(hFile,
    &IoStatusBlock,
    &FilePosition,
    sizeof(FILE_POSITION_INFORMATION),
    FilePositionInformation);
  if (!NT_SUCCESS(errCode))
    {
      if (Status != NULL)
        {
          *Status = errCode;
        }
      return -1;
    }
  
  if (lpDistanceToMoveHigh != NULL)
    {
      *lpDistanceToMoveHigh = FilePosition.CurrentByteOffset.u.HighPart;
    }
  if (Status != NULL)
    {
      *Status = STATUS_SUCCESS;
    }
  return FilePosition.CurrentByteOffset.u.LowPart;
}


static BOOL
ConvertSystemTimeToFileTime(
  CONST SYSTEMTIME *  lpSystemTime,	
  LPFILETIME  lpFileTime)
{
  TIME_FIELDS TimeFields;
  LARGE_INTEGER liTime;
  
  TimeFields.Year = lpSystemTime->wYear;
  TimeFields.Month = lpSystemTime->wMonth;
  TimeFields.Day = lpSystemTime->wDay;
  TimeFields.Hour = lpSystemTime->wHour;
  TimeFields.Minute = lpSystemTime->wMinute;
  TimeFields.Second = lpSystemTime->wSecond;
  TimeFields.Milliseconds = lpSystemTime->wMilliseconds;
  
  if (RtlTimeFieldsToTime(&TimeFields, &liTime))
    {
      lpFileTime->dwLowDateTime = liTime.u.LowPart;
      lpFileTime->dwHighDateTime = liTime.u.HighPart;
      return TRUE;
    }
  return FALSE;
}


static BOOL
ConvertDosDateTimeToFileTime(
  WORD wFatDate,
  WORD wFatTime,
  LPFILETIME lpFileTime)
{
  PDOSTIME  pdtime = (PDOSTIME) &wFatTime;
  PDOSDATE  pddate = (PDOSDATE) &wFatDate;
  SYSTEMTIME SystemTime;
  
  if (lpFileTime == NULL)
    return FALSE;
  
  SystemTime.wMilliseconds = 0;
  SystemTime.wSecond = pdtime->Second;
  SystemTime.wMinute = pdtime->Minute;
  SystemTime.wHour = pdtime->Hour;
  
  SystemTime.wDay = pddate->Day;
  SystemTime.wMonth = pddate->Month;
  SystemTime.wYear = 1980 + pddate->Year;
  
  ConvertSystemTimeToFileTime(&SystemTime,lpFileTime);
  
  return TRUE;
}


static PWCHAR
GetFileName(PWCHAR Path)
/*
 * FUNCTION: Returns a pointer to file name
 * ARGUMENTS:
 *     Path = Pointer to string with pathname
 * RETURNS:
 *     Pointer to filename
 */
{
  ULONG i, j;
  
  j = i = 0;
  
  while (Path [i++])
    {
      if (Path[i - 1] == L'\\') j = i;
    }
  return Path + j;
}


static VOID
RemoveFileName(PWCHAR Path)
/*
 * FUNCTION: Removes a file name from a path
 * ARGUMENTS:
 *     Path = Pointer to string with path
 */
{
  PWCHAR FileName;
  DWORD i;
  
  i = 0;
  FileName = GetFileName(Path + i);
  
  if ((FileName != (Path + i)) && (FileName [-1] == L'\\'))
    FileName--;
  if ((FileName == (Path + i)) && (FileName [0] == L'\\'))
    FileName++;
  FileName[0] = 0;
}


static BOOL
SetAttributesOnFile(PCFFILE_NODE File, HANDLE hFile)
/*
 * FUNCTION: Sets attributes on a file
 * ARGUMENTS:
 *      File = Pointer to CFFILE node for file
 * RETURNS:
 *     Status of operation
 */
{
  FILE_BASIC_INFORMATION FileBasic;
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS NtStatus;
  ULONG Attributes = 0;

  if (File->File.Attributes & CAB_ATTRIB_READONLY)
    Attributes |= FILE_ATTRIBUTE_READONLY;

  if (File->File.Attributes & CAB_ATTRIB_HIDDEN)
    Attributes |= FILE_ATTRIBUTE_HIDDEN;

  if (File->File.Attributes & CAB_ATTRIB_SYSTEM)
    Attributes |= FILE_ATTRIBUTE_SYSTEM;

  if (File->File.Attributes & CAB_ATTRIB_DIRECTORY)
    Attributes |= FILE_ATTRIBUTE_DIRECTORY;

  if (File->File.Attributes & CAB_ATTRIB_ARCHIVE)
    Attributes |= FILE_ATTRIBUTE_ARCHIVE;

  NtStatus = NtQueryInformationFile(hFile,
    &IoStatusBlock,
    &FileBasic,
    sizeof(FILE_BASIC_INFORMATION),
    FileBasicInformation);
  if (!NT_SUCCESS(NtStatus))
    {
      DPRINT("NtQueryInformationFile() failed (%x).\n", NtStatus);
    }
  else
    {
      FileBasic.FileAttributes = Attributes;

      NtStatus = NtSetInformationFile(hFile,
        &IoStatusBlock,
        &FileBasic,
        sizeof(FILE_BASIC_INFORMATION),
        FileBasicInformation);
      if (!NT_SUCCESS(NtStatus))
        {
          DPRINT("NtSetInformationFile() failed (%x).\n", NtStatus);
        }
    }

  return NT_SUCCESS(NtStatus);
}


static ULONG
ReadBlock(PVOID Buffer,
  ULONG Size,
  PULONG BytesRead)
/*
 * FUNCTION: Read a block of data from file
 * ARGUMENTS:
 *     Buffer    = Pointer to data buffer
 *     Size      = Length of data buffer
 *     BytesRead = Pointer to ULONG that on return will contain
 *                 number of bytes read
 * RETURNS:
 *     Status of operation
 */
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS NtStatus;

  NtStatus = NtReadFile(FileHandle,
	  NULL,
	  NULL,
	  NULL,
	  &IoStatusBlock,
	  Buffer,
	  Size,
	  NULL,
	  NULL);
  if (!NT_SUCCESS(NtStatus))
	  {
      DPRINT("ReadBlock for %d bytes failed (%x)\n", Size, NtStatus);
      *BytesRead = 0;
      return CAB_STATUS_INVALID_CAB;
    }
  *BytesRead = Size;
  return CAB_STATUS_SUCCESS;
}


static PCFFOLDER_NODE
NewFolderNode()
/*
 * FUNCTION: Creates a new folder node
 * RETURNS:
 *     Pointer to node if there was enough free memory available, otherwise NULL
 */
{
  PCFFOLDER_NODE Node;

  Node = (PCFFOLDER_NODE)RtlAllocateHeap (ProcessHeap, 0, sizeof(CFFOLDER_NODE));
  if (!Node)
    return NULL;
  
  RtlZeroMemory(Node, sizeof(CFFOLDER_NODE));
  
  Node->Folder.CompressionType = CAB_COMP_NONE;
  
  Node->Prev = FolderListTail;
  
  if (FolderListTail != NULL)
    {
      FolderListTail->Next = Node;
    }
  else
    {
      FolderListHead = Node;
    }
  FolderListTail = Node;
  
  return Node;
}


static PCFFILE_NODE
NewFileNode()
/*
 * FUNCTION: Creates a new file node
 * ARGUMENTS:
 *     FolderNode = Pointer to folder node to bind file to
 * RETURNS:
 *     Pointer to node if there was enough free memory available, otherwise NULL
 */
{
  PCFFILE_NODE Node;

  Node = (PCFFILE_NODE)RtlAllocateHeap (ProcessHeap, 0, sizeof(CFFILE_NODE));
  if (!Node)
    return NULL;

  RtlZeroMemory(Node, sizeof(CFFILE_NODE));

  Node->Prev = FileListTail;

  if (FileListTail != NULL)
    {
      FileListTail->Next = Node;
    }
  else
    {
      FileListHead = Node;
    }
  FileListTail = Node;

  return Node;
}


static PCFDATA_NODE
NewDataNode(PCFFOLDER_NODE FolderNode)
/*
 * FUNCTION: Creates a new data block node
 * ARGUMENTS:
 *     FolderNode = Pointer to folder node to bind data block to
 * RETURNS:
 *     Pointer to node if there was enough free memory available, otherwise NULL
 */
{
  PCFDATA_NODE Node;

  Node = (PCFDATA_NODE)RtlAllocateHeap (ProcessHeap, 0, sizeof(CFDATA_NODE));
  if (!Node)
    return NULL;

  RtlZeroMemory(Node, sizeof(CFDATA_NODE));

  Node->Prev = FolderNode->DataListTail;

  if (FolderNode->DataListTail != NULL)
    {
      FolderNode->DataListTail->Next = Node;
    }
  else
    {
      FolderNode->DataListHead = Node;
    }
  FolderNode->DataListTail = Node;

  return Node;
}


static VOID
DestroyDataNodes(PCFFOLDER_NODE FolderNode)
/*
 * FUNCTION: Destroys data block nodes bound to a folder node
 * ARGUMENTS:
 *     FolderNode = Pointer to folder node
 */
{
  PCFDATA_NODE PrevNode;
  PCFDATA_NODE NextNode;

  NextNode = FolderNode->DataListHead;
  while (NextNode != NULL)
    {
      PrevNode = NextNode->Next;
      RtlFreeHeap(ProcessHeap, 0, NextNode);
      NextNode = PrevNode;
    }
  FolderNode->DataListHead = NULL;
  FolderNode->DataListTail = NULL;
}


static VOID
DestroyFileNodes()
/*
 * FUNCTION: Destroys file nodes
 * ARGUMENTS:
 *     FolderNode = Pointer to folder node
 */
{
  PCFFILE_NODE PrevNode;
  PCFFILE_NODE NextNode;

  NextNode = FileListHead;
  while (NextNode != NULL)
    {
      PrevNode = NextNode->Next;
      if (NextNode->FileName)
        RtlFreeHeap(ProcessHeap, 0, NextNode->FileName);
      RtlFreeHeap(ProcessHeap, 0, NextNode);
      NextNode = PrevNode;
    }
  FileListHead = NULL;
  FileListTail = NULL;
}


#if 0
static VOID
DestroyDeletedFileNodes()
/*
 * FUNCTION: Destroys file nodes that are marked for deletion
 */
{
  PCFFILE_NODE CurNode;
  PCFFILE_NODE NextNode;

  CurNode = FileListHead;
  while (CurNode != NULL)
    {
      NextNode = CurNode->Next;

      if (CurNode->Delete)
        {
          if (CurNode->Prev != NULL)
            {
              CurNode->Prev->Next = CurNode->Next;
            }
          else
            {
              FileListHead = CurNode->Next;
              if (FileListHead)
                  FileListHead->Prev = NULL;
            }

          if (CurNode->Next != NULL)
            {
              CurNode->Next->Prev = CurNode->Prev;
            }
          else
            {
              FileListTail = CurNode->Prev;
              if (FileListTail)
                  FileListTail->Next = NULL;
            }

          DPRINT("Deleting file: '%S'\n", CurNode->FileName);

          if (CurNode->FileName)
            RtlFreeHeap(ProcessHeap, 0, CurNode->FileName);
          RtlFreeHeap(ProcessHeap, 0, CurNode);
        }
      CurNode = NextNode;
    }
}
#endif

static VOID
DestroyFolderNodes()
/*
 * FUNCTION: Destroys folder nodes
 */
{
  PCFFOLDER_NODE PrevNode;
  PCFFOLDER_NODE NextNode;

  NextNode = FolderListHead;
  while (NextNode != NULL)
    {
      PrevNode = NextNode->Next;
      DestroyDataNodes(NextNode);
      RtlFreeHeap(ProcessHeap, 0, NextNode);
      NextNode = PrevNode;
    }
  FolderListHead = NULL;
  FolderListTail = NULL;
}

#if 0
static VOID
DestroyDeletedFolderNodes()
/*
 * FUNCTION: Destroys folder nodes that are marked for deletion
 */
{
  PCFFOLDER_NODE CurNode;
  PCFFOLDER_NODE NextNode;

  CurNode = FolderListHead;
  while (CurNode != NULL)
    {
      NextNode = CurNode->Next;

      if (CurNode->Delete)
        {
          if (CurNode->Prev != NULL)
            {
              CurNode->Prev->Next = CurNode->Next;
            }
          else
            {
              FolderListHead = CurNode->Next;
              if (FolderListHead)
                FolderListHead->Prev = NULL;
            }

          if (CurNode->Next != NULL)
            {
              CurNode->Next->Prev = CurNode->Prev;
            }
          else
            {
              FolderListTail = CurNode->Prev;
              if (FolderListTail)
                  FolderListTail->Next = NULL;
            }

          DestroyDataNodes(CurNode);
          RtlFreeHeap(ProcessHeap, 0, CurNode);
      }
      CurNode = NextNode;
    }
}
#endif

static PCFFOLDER_NODE
LocateFolderNode(ULONG Index)
/*
 * FUNCTION: Locates a folder node
 * ARGUMENTS:
 *     Index = Folder index
 * RETURNS:
 *     Pointer to folder node or NULL if the folder node was not found
 */
{
  PCFFOLDER_NODE Node;
  
  switch (Index)
    {
      case CAB_FILE_SPLIT:
        return FolderListTail;

      case CAB_FILE_CONTINUED:
      case CAB_FILE_PREV_NEXT:
        return FolderListHead;
    }
  
  Node = FolderListHead;
  while (Node != NULL)
    {
      if (Node->Index == Index)
        return Node;
      Node = Node->Next;
    }
  return NULL;
}


static ULONG
GetAbsoluteOffset(PCFFILE_NODE File)
/*
 * FUNCTION: Returns the absolute offset of a file
 * ARGUMENTS:
 *     File = Pointer to CFFILE_NODE structure for file
 * RETURNS:
 *     Status of operation
 */
{
  PCFDATA_NODE Node;

  DPRINT("FileName '%S'  FileOffset (0x%X)  FileSize (%d).\n",
    (PWCHAR)File->FileName,
    (UINT)File->File.FileOffset,
    (UINT)File->File.FileSize);

  Node = CurrentFolderNode->DataListHead;
  while (Node != NULL)
    {
      DPRINT("GetAbsoluteOffset(): Comparing (0x%X, 0x%X) (%d).\n",
          (UINT)Node->UncompOffset,
          (UINT)Node->UncompOffset + Node->Data.UncompSize,
          (UINT)Node->Data.UncompSize);

      /* Node->Data.UncompSize will be 0 if the block is split
         (ie. it is the last block in this cabinet) */
      if ((Node->Data.UncompSize == 0) ||
        ((File->File.FileOffset >= Node->UncompOffset) &&
        (File->File.FileOffset < Node->UncompOffset +
        Node->Data.UncompSize)))
        {
          File->DataBlock = Node;
          return CAB_STATUS_SUCCESS;
        }

      Node = Node->Next;
    }
  return CAB_STATUS_INVALID_CAB;
}


static ULONG
LocateFile(PWCHAR FileName,
  PCFFILE_NODE *File)
/*
 * FUNCTION: Locates a file in the cabinet
 * ARGUMENTS:
 *     FileName = Pointer to string with name of file to locate
 *     File     = Address of pointer to CFFILE_NODE structure to fill
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     Current folder is set to the folder of the file
 */
{
  PCFFILE_NODE Node;
  ULONG Status;

  DPRINT("FileName '%S'\n", FileName);

  Node = FileListHead;
  while (Node != NULL)
    {
      if (_wcsicmp(FileName, Node->FileName) == 0)
        {
          CurrentFolderNode = LocateFolderNode(Node->File.FileControlID);
          if (!CurrentFolderNode)
            {
              DPRINT("Folder with index number (%d) not found.\n",
                (UINT)Node->File.FileControlID);
              return CAB_STATUS_INVALID_CAB;
            }

          if (Node->DataBlock == NULL)
            {
              Status = GetAbsoluteOffset(Node);
            }
          else
            Status = CAB_STATUS_SUCCESS;
          *File = Node;
          return Status;
      }
      Node = Node->Next;
  }
  return CAB_STATUS_NOFILE;
}


static ULONG
ReadString(PWCHAR String, ULONG MaxLength)
/*
 * FUNCTION: Reads a NULL-terminated string from the cabinet
 * ARGUMENTS:
 *     String    = Pointer to buffer to place string
 *     MaxLength = Maximum length of string
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS NtStatus;
  ULONG BytesRead;
  ULONG Offset;
  ULONG Status;
  ULONG Size;
  BOOL Found;
  CHAR buf[MAX_PATH];
  ANSI_STRING as;
  UNICODE_STRING us;

  Offset = 0;
  Found  = FALSE;
  do
    {
      Size = ((Offset + 32) <= MaxLength)? 32 : MaxLength - Offset;

      if (Size == 0)
        {
          DPRINT("Too long a filename.\n");
          return CAB_STATUS_INVALID_CAB;
        }

      Status = ReadBlock((PCFDATA)&buf[Offset], Size, &BytesRead);
      if ((Status != CAB_STATUS_SUCCESS) || (BytesRead != Size))
        {
          DPRINT("Cannot read from file (%d).\n", (UINT)Status);
          return CAB_STATUS_INVALID_CAB;
        }

      for (Size = Offset; Size < Offset + BytesRead; Size++)
        {
          if (buf[Size] == '\0')
            {
              Found = TRUE;
              break;
            }
        }

      Offset += BytesRead;
    } while (!Found);

  /* Back up some bytes */
  Size = (BytesRead - Size) - 1;
  SeekInFile(FileHandle, -(LONG)Size, NULL, SEEK_CURRENT, &NtStatus);
  if (!NT_SUCCESS(NtStatus))
    {
      DPRINT("SeekInFile() failed (%x).\n", NtStatus);
      return CAB_STATUS_INVALID_CAB;
    }

  RtlInitAnsiString(&as, buf);
  us.Buffer = String;
  us.MaximumLength = MaxLength * sizeof(WCHAR);
  us.Length = 0;

  RtlAnsiStringToUnicodeString(&us, &as, FALSE);

  return CAB_STATUS_SUCCESS;
}


static ULONG
ReadFileTable(VOID)
/*
 * FUNCTION: Reads the file table from the cabinet file
 * RETURNS:
 *     Status of operation
 */
{
  ULONG i;
  ULONG Status;
  ULONG BytesRead;
  PCFFILE_NODE File;
  NTSTATUS NtStatus;

  DPRINT("Reading file table at absolute offset (0x%X).\n",
    CABHeader.FileTableOffset);

  /* Seek to file table */
  SeekInFile(FileHandle, CABHeader.FileTableOffset, NULL, SEEK_BEGIN, &NtStatus);
  if (!NT_SUCCESS(NtStatus))
    {
      DPRINT("SeekInFile() failed (%x).\n", NtStatus);
      return CAB_STATUS_INVALID_CAB;
    }

  for (i = 0; i < CABHeader.FileCount; i++)
    {
      File = NewFileNode();
      if (!File)
        {
          DPRINT("Insufficient memory.\n");
          return CAB_STATUS_NOMEMORY;
        }

      if ((Status = ReadBlock(&File->File, sizeof(CFFILE),
          &BytesRead)) != CAB_STATUS_SUCCESS) {
          DPRINT("Cannot read from file (%d).\n", (UINT)Status);
          return CAB_STATUS_INVALID_CAB;
      }

      File->FileName = (PWCHAR)RtlAllocateHeap(ProcessHeap, 0, MAX_PATH * sizeof(WCHAR));
      if (!File->FileName)
        {
          DPRINT("Insufficient memory.\n");
          return CAB_STATUS_NOMEMORY;
        }

      /* Read file name */
      Status = ReadString(File->FileName, MAX_PATH);
      if (Status != CAB_STATUS_SUCCESS)
        return Status;

      DPRINT("Found file '%S' at uncompressed offset (0x%X).  Size (%d bytes)  ControlId (0x%X).\n",
        (PWCHAR)File->FileName,
        (UINT)File->File.FileOffset,
        (UINT)File->File.FileSize,
        (UINT)File->File.FileControlID);
    }
  return CAB_STATUS_SUCCESS;
}


static ULONG
ReadDataBlocks(PCFFOLDER_NODE FolderNode)
/*
 * FUNCTION: Reads all CFDATA blocks for a folder from the cabinet file
 * ARGUMENTS:
 *     FolderNode = Pointer to CFFOLDER_NODE structure for folder
 * RETURNS:
 *     Status of operation
 */
{
  ULONG AbsoluteOffset;
  ULONG UncompOffset;
  PCFDATA_NODE Node;
  NTSTATUS NtStatus;
  ULONG BytesRead;
  ULONG Status;
  ULONG i;

  DPRINT("Reading data blocks for folder (%d)  at absolute offset (0x%X).\n",
    FolderNode->Index, FolderNode->Folder.DataOffset);

  AbsoluteOffset = FolderNode->Folder.DataOffset;
  UncompOffset = FolderNode->UncompOffset;

  for (i = 0; i < FolderNode->Folder.DataBlockCount; i++)
    {
      Node = NewDataNode(FolderNode);
      if (!Node)
        {
          DPRINT("Insufficient memory.\n");
          return CAB_STATUS_NOMEMORY;
        }

      /* Seek to data block */
      SeekInFile(FileHandle, AbsoluteOffset, NULL, SEEK_BEGIN, &NtStatus);
      if (!NT_SUCCESS(NtStatus))
        {
          DPRINT("SeekInFile() failed (%x).\n", NtStatus);
          return CAB_STATUS_INVALID_CAB;
        }

      if ((Status = ReadBlock(&Node->Data, sizeof(CFDATA),
          &BytesRead)) != CAB_STATUS_SUCCESS)
        {
          DPRINT("Cannot read from file (%d).\n", (UINT)Status);
          return CAB_STATUS_INVALID_CAB;
        }

      DPRINT("AbsOffset (0x%X)  UncompOffset (0x%X)  Checksum (0x%X)  CompSize (%d)  UncompSize (%d).\n",
        (UINT)AbsoluteOffset,
        (UINT)UncompOffset,
        (UINT)Node->Data.Checksum,
        (UINT)Node->Data.CompSize,
        (UINT)Node->Data.UncompSize);

      Node->AbsoluteOffset = AbsoluteOffset;
      Node->UncompOffset = UncompOffset;

      AbsoluteOffset += sizeof(CFDATA) + Node->Data.CompSize;
      UncompOffset += Node->Data.UncompSize;
    }

  FolderUncompSize = UncompOffset;

  return CAB_STATUS_SUCCESS;
}

#if 0
static ULONG
ComputeChecksum(PVOID Buffer,
  UINT Size,
  ULONG Seed)
/*
 * FUNCTION: Computes checksum for data block
 * ARGUMENTS:
 *     Buffer = Pointer to data buffer
 *     Size   = Length of data buffer
 *     Seed   = Previously computed checksum
 * RETURNS:
 *     Checksum of buffer
 */
{
  INT UlongCount; // Number of ULONGs in block
  ULONG Checksum; // Checksum accumulator
  PBYTE pb;
  ULONG ul;

  /* FIXME: Doesn't seem to be correct. EXTRACT.EXE
     won't accept checksums computed by this routine */

  DPRINT("Checksumming buffer (0x%X)  Size (%d)\n", (UINT)Buffer, Size);

  UlongCount = Size / 4;            // Number of ULONGs
  Checksum = Seed;                  // Init checksum
  pb = (PBYTE)Buffer;               // Start at front of data block

  /* Checksum integral multiple of ULONGs */
  while (UlongCount-- > 0)
    {
      /* NOTE: Build ULONG in big/little-endian independent manner */
      ul = *pb++;                     // Get low-order byte
      ul |= (((ULONG)(*pb++)) <<  8); // Add 2nd byte
      ul |= (((ULONG)(*pb++)) << 16); // Add 3nd byte
      ul |= (((ULONG)(*pb++)) << 24); // Add 4th byte

      Checksum ^= ul;                 // Update checksum
  }

  /* Checksum remainder bytes */
  ul = 0;
  switch (Size % 4)
    {
      case 3:
          ul |= (((ULONG)(*pb++)) << 16); // Add 3rd byte
      case 2:
          ul |= (((ULONG)(*pb++)) <<  8); // Add 2nd byte
      case 1:
          ul |= *pb++;                    // Get low-order byte
      default:
          break;
    }
  Checksum ^= ul;                         // Update checksum

  /* Return computed checksum */
  return Checksum;
}
#endif

static ULONG
CloseCabinet(VOID)
/*
 * FUNCTION: Closes the current cabinet
 * RETURNS:
 *     Status of operation
 */
{
  DestroyFileNodes();
  
  DestroyFolderNodes();
  
  if (InputBuffer)
    {
      RtlFreeHeap(ProcessHeap, 0, InputBuffer);
      InputBuffer = NULL;
    }
  
  if (OutputBuffer)
    {
      RtlFreeHeap(ProcessHeap, 0, OutputBuffer);
      OutputBuffer = NULL;
    }

  NtClose(FileHandle);
  return 0;
}


VOID
CabinetInitialize(VOID)
/*
 * FUNCTION: Initialize archiver
 */
{
  ZStream.zalloc = MSZipAlloc;
  ZStream.zfree  = MSZipFree;
  ZStream.opaque = (voidpf)0;

  FileOpen = FALSE;
  wcscpy(DestPath, L"");
  
  FolderListHead = NULL;
  FolderListTail = NULL;
  FileListHead   = NULL;
  FileListTail   = NULL;
  
  CodecId       = CAB_CODEC_RAW;
  CodecSelected = TRUE;
  
  OutputBuffer = NULL;
  CurrentOBuffer = NULL;
  CurrentOBufferSize = 0;
  InputBuffer  = NULL;
  CurrentIBuffer = NULL;
  CurrentIBufferSize = 0;

  FolderUncompSize = 0;
  BytesLeftInBlock = 0;
  CabinetReserved = 0;
  FolderReserved = 0;
  DataReserved = 0;
  ReuseBlock       = FALSE;
  CurrentFolderNode = NULL;
  CurrentDataNode  = NULL;
  CabinetReservedArea = NULL;
  RestartSearch = FALSE;
  LastFileOffset = 0;
}


VOID
CabinetCleanup(VOID)
/*
 * FUNCTION: Cleanup archiver
 */
{
  CabinetClose();
}


BOOL
CabinetNormalizePath(PWCHAR Path,
  ULONG Length)
/*
 * FUNCTION: Normalizes a path
 * ARGUMENTS:
 *     Path   = Pointer to string with pathname
 *     Length = Number of characters in Path
 * RETURNS:
 *     TRUE if there was enough room in Path, or FALSE
 */
{
  ULONG n;
  BOOL OK = TRUE;
  
  if ((n = wcslen(Path)) &&
    (Path[n - 1] != L'\\') &&
    (OK = ((n + 1) < Length)))
    {
      Path[n]     = L'\\';
      Path[n + 1] = 0;
    }
  return OK;
}


PWCHAR
CabinetGetCabinetName()
/*
 * FUNCTION: Returns pointer to cabinet file name
 * RETURNS:
 *     Pointer to string with name of cabinet
 */
{
  return CabinetName;
}


VOID
CabinetSetCabinetName(PWCHAR FileName)
/*
 * FUNCTION: Sets cabinet file name
 * ARGUMENTS:
 *     FileName = Pointer to string with name of cabinet
 */
{
  wcscpy(CabinetName, FileName);
}


VOID
CabinetSetDestinationPath(PWCHAR DestinationPath)
/*
 * FUNCTION: Sets destination path
 * ARGUMENTS:
 *    DestinationPath = Pointer to string with name of destination path
 */
{
  wcscpy(DestPath, DestinationPath);
  if (wcslen(DestPath) > 0)
    CabinetNormalizePath(DestPath, MAX_PATH);
}


PWCHAR
CabinetGetDestinationPath()
/*
 * FUNCTION: Returns destination path
 * RETURNS:
 *    Pointer to string with name of destination path
 */
{
  return DestPath;
}


ULONG
CabinetOpen(VOID)
/*
 * FUNCTION: Opens a cabinet file
 * RETURNS:
 *     Status of operation
 */
{
  WCHAR CabinetFileName[256];
  PCFFOLDER_NODE FolderNode;
  ULONG Status;
  ULONG Index;
  
  if (!FileOpen)
    {
      OBJECT_ATTRIBUTES ObjectAttributes;
      IO_STATUS_BLOCK IoStatusBlock;
      UNICODE_STRING FileName;
      NTSTATUS NtStatus;
      ULONG BytesRead;
      ULONG Size;
    
      OutputBuffer = RtlAllocateHeap(ProcessHeap, 0, CAB_BLOCKSIZE + 12); // This should be enough
      if (!OutputBuffer)
        return CAB_STATUS_NOMEMORY;

      RtlInitUnicodeString(&FileName,
        CabinetName);

      InitializeObjectAttributes(&ObjectAttributes,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

      NtStatus = NtOpenFile(&FileHandle,
        FILE_READ_ACCESS,
		    &ObjectAttributes,
		    &IoStatusBlock,
		    FILE_SHARE_READ,
		    FILE_SYNCHRONOUS_IO_NONALERT);
      if (!NT_SUCCESS(NtStatus))
        {
          DPRINT("Cannot open file (%S) (%x).\n", CabinetName, NtStatus);
          return CAB_STATUS_CANNOT_OPEN;
        }
    
      FileOpen = TRUE;
    
      /* Load CAB header */
      if ((Status = ReadBlock(&CABHeader, sizeof(CFHEADER), &BytesRead)) != CAB_STATUS_SUCCESS)
        {
          DPRINT("Cannot read from file (%d).\n", (UINT)Status);
          return CAB_STATUS_INVALID_CAB;
        }
    
      /* Check header */
      if ((BytesRead               != sizeof(CFHEADER)) ||
        (CABHeader.Signature       != CAB_SIGNATURE   ) ||
        (CABHeader.Version         != CAB_VERSION     ) ||
        (CABHeader.FolderCount     == 0               ) ||
        (CABHeader.FileCount       == 0               ) ||
        (CABHeader.FileTableOffset < sizeof(CFHEADER))) {
        CloseCabinet();
        DPRINT("File has invalid header.\n");
        return CAB_STATUS_INVALID_CAB;
      }
  
      Size = 0;
  
      /* Read/skip any reserved bytes */
      if (CABHeader.Flags & CAB_FLAG_RESERVE)
        {
          if ((Status = ReadBlock(&Size, sizeof(ULONG), &BytesRead)) != CAB_STATUS_SUCCESS)
            {
              DPRINT("Cannot read from file (%d).\n", (UINT)Status);
              return CAB_STATUS_INVALID_CAB;
            }
          CabinetReserved = Size & 0xFFFF;
          FolderReserved = (Size >> 16) & 0xFF;
          DataReserved = (Size >> 24) & 0xFF;

          if (CabinetReserved > 0)
            {
              CabinetReservedArea = RtlAllocateHeap(ProcessHeap, 0, CabinetReserved);
              if (!CabinetReservedArea)
                {
                  return CAB_STATUS_NOMEMORY;
                }

              if ((Status = ReadBlock(CabinetReservedArea, CabinetReserved, &BytesRead)) != CAB_STATUS_SUCCESS)
                {
                  DPRINT("Cannot read from file (%d).\n", (UINT)Status);
                  return CAB_STATUS_INVALID_CAB;
                }
            }
#if 0
          SeekInFile(FileHandle, CabinetReserved, NULL, SEEK_CURRENT, &NtStatus);
          if (!NT_SUCCESS(NtStatus))
            {
              DPRINT("SeekInFile() failed (%x).\n", NtStatus);
              return CAB_STATUS_INVALID_CAB;
            }
#endif
        }
  
      if ((CABHeader.Flags & CAB_FLAG_HASPREV) > 0)
        {
          /* Read name of previous cabinet */
          Status = ReadString(CabinetFileName, 256);
          if (Status != CAB_STATUS_SUCCESS)
            return Status;

          /* The previous cabinet file is in the same directory as the current */
          wcscpy(CabinetPrev, CabinetName);
          RemoveFileName(CabinetPrev);
          CabinetNormalizePath(CabinetPrev, 256);
          wcscat(CabinetPrev, CabinetFileName);

          /* Read label of previous disk */
          Status = ReadString(DiskPrev, 256);
          if (Status != CAB_STATUS_SUCCESS)
            return Status;
        }
      else
        {
          wcscpy(CabinetPrev, L"");
          wcscpy(DiskPrev, L"");
        }
    
      if ((CABHeader.Flags & CAB_FLAG_HASNEXT) > 0)
        {
          /* Read name of next cabinet */
          Status = ReadString(CabinetFileName, 256);
          if (Status != CAB_STATUS_SUCCESS)
              return Status;

          /* The next cabinet file is in the same directory as the previous */
          wcscpy(CabinetNext, CabinetName);
          RemoveFileName(CabinetNext);
          CabinetNormalizePath(CabinetNext, 256);
          wcscat(CabinetNext, CabinetFileName);

          /* Read label of next disk */
          Status = ReadString(DiskNext, 256);
          if (Status != CAB_STATUS_SUCCESS)
              return Status;
        }
      else
        {
          wcscpy(CabinetNext, L"");
          wcscpy(DiskNext,    L"");
        }
  
      /* Read all folders */
      for (Index = 0; Index < CABHeader.FolderCount; Index++)
        {
          FolderNode = NewFolderNode();
          if (!FolderNode)
            {
              DPRINT("Insufficient resources.\n");
              return CAB_STATUS_NOMEMORY;
            }
  
          if (Index == 0)
            FolderNode->UncompOffset = FolderUncompSize;
    
          FolderNode->Index = Index;
  
          if ((Status = ReadBlock(&FolderNode->Folder,
            sizeof(CFFOLDER), &BytesRead)) != CAB_STATUS_SUCCESS)
            {
              DPRINT("Cannot read from file (%d).\n", (UINT)Status);
              return CAB_STATUS_INVALID_CAB;
            }
        }
  
      /* Read file entries */
      Status = ReadFileTable();
      if (Status != CAB_STATUS_SUCCESS)
        {
          DPRINT("ReadFileTable() failed (%d).\n", (UINT)Status);
          return Status;
        }
  
      /* Read data blocks for all folders */
      FolderNode = FolderListHead;
      while (FolderNode != NULL)
        {
          Status = ReadDataBlocks(FolderNode);
          if (Status != CAB_STATUS_SUCCESS)
            {
              DPRINT("ReadDataBlocks() failed (%d).\n", (UINT)Status);
              return Status;
            }
          FolderNode = FolderNode->Next;
        }
    }
  return CAB_STATUS_SUCCESS;
}


VOID
CabinetClose(VOID)
/*
 * FUNCTION: Closes the cabinet file
 */
{
  if (FileOpen)
    {
      CloseCabinet();

      if (CabinetReservedArea != NULL)
        {
          RtlFreeHeap(ProcessHeap, 0, CabinetReservedArea);
          CabinetReservedArea = NULL;
        }

      FileOpen = FALSE;
    }
}


ULONG
CabinetFindFirst(PWCHAR FileName,
  PCAB_SEARCH Search)
/*
 * FUNCTION: Finds the first file in the cabinet that matches a search criteria
 * ARGUMENTS:
 *     FileName = Pointer to search criteria
 *     Search   = Pointer to search structure
 * RETURNS:
 *     Status of operation
 */
{
  RestartSearch = FALSE;
  wcsncpy(Search->Search, FileName, MAX_PATH);
  Search->Next = FileListHead;
  return CabinetFindNext(Search);
}


ULONG
CabinetFindNext(PCAB_SEARCH Search)
/*
 * FUNCTION: Finds next file in the cabinet that matches a search criteria
 * ARGUMENTS:
 *     Search = Pointer to search structure
 * RETURNS:
 *     Status of operation
 */
{
  ULONG Status;

  if (RestartSearch)
    {
      Search->Next = FileListHead;

      /* Skip split files already extracted */
      while ((Search->Next) &&
        (Search->Next->File.FileControlID > CAB_FILE_MAX_FOLDER) &&
        (Search->Next->File.FileOffset <= LastFileOffset))
        {
          DPRINT("Skipping file (%s)  FileOffset (0x%X)  LastFileOffset (0x%X).\n",
            Search->Next->FileName, Search->Next->File.FileOffset, LastFileOffset);
          Search->Next = Search->Next->Next;
        }

      RestartSearch = FALSE;
    }

  /* FIXME: Check search criteria */

  if (!Search->Next)
    {
      if (wcslen(DiskNext) > 0)
        {
          CloseCabinet();

          CabinetSetCabinetName(CabinetNext);

          if (DiskChangeHandler != NULL)
            {
              DiskChangeHandler(CabinetNext, DiskNext);
            }

          Status = CabinetOpen();
          if (Status != CAB_STATUS_SUCCESS) 
            return Status;

          Search->Next = FileListHead;
          if (!Search->Next)
            return CAB_STATUS_NOFILE;
        }
      else
        {
          return CAB_STATUS_NOFILE;
        }
  }

  Search->File = &Search->Next->File;
  Search->FileName = Search->Next->FileName;
  Search->Next = Search->Next->Next;
  return CAB_STATUS_SUCCESS;
}


ULONG
CabinetExtractFile(PWCHAR FileName)
/*
 * FUNCTION: Extracts a file from the cabinet
 * ARGUMENTS:
 *     FileName = Pointer to buffer with name of file
 * RETURNS
 *     Status of operation
 */
{
  ULONG Size;
  ULONG Offset;
  ULONG BytesRead;
  ULONG BytesToRead;
  ULONG BytesWritten;
  ULONG BytesSkipped;
  ULONG BytesToWrite;
  ULONG TotalBytesRead;
  ULONG CurrentOffset;
  PUCHAR Buffer;
  PUCHAR CurrentBuffer;
  HANDLE DestFile;
  PCFFILE_NODE File;
  CFDATA CFData;
  ULONG Status;
  BOOL Skip;
  FILETIME FileTime;
  WCHAR DestName[MAX_PATH];
  WCHAR TempName[MAX_PATH];
  PWCHAR s;
  NTSTATUS NtStatus;
  UNICODE_STRING UnicodeString;
  IO_STATUS_BLOCK IoStatusBlock;
  OBJECT_ATTRIBUTES ObjectAttributes;
  FILE_BASIC_INFORMATION FileBasic;

  Status = LocateFile(FileName, &File);
  if (Status != CAB_STATUS_SUCCESS)
    {
      DPRINT("Cannot locate file (%d).\n", (UINT)Status);
      return Status;
    }

  LastFileOffset = File->File.FileOffset;
  
  switch (CurrentFolderNode->Folder.CompressionType & CAB_COMP_MASK)
    {
      case CAB_COMP_NONE:
        CabinetSelectCodec(CAB_CODEC_RAW);
        break;
      case CAB_COMP_MSZIP:
        CabinetSelectCodec(CAB_CODEC_MSZIP);
        break;
      default:
        return CAB_STATUS_UNSUPPCOMP;
    }
  
  DPRINT("Extracting file at uncompressed offset (0x%X)  Size (%d bytes)  AO (0x%X)  UO (0x%X).\n",
    (UINT)File->File.FileOffset,
    (UINT)File->File.FileSize,
    (UINT)File->DataBlock->AbsoluteOffset,
    (UINT)File->DataBlock->UncompOffset);

  wcscpy(DestName, DestPath);
  wcscat(DestName, FileName);

  while (NULL != (s = wcsstr(DestName, L"\\.\\")))
    {
      memmove(s, s + 2, (wcslen(s + 2) + 1) *sizeof(WCHAR));
    }

  /* Create destination file, fail if it already exists */
  RtlInitUnicodeString(&UnicodeString,
    DestName);


  InitializeObjectAttributes(&ObjectAttributes,
    &UnicodeString,
    OBJ_CASE_INSENSITIVE,
    NULL,
    NULL);

  NtStatus = NtCreateFile(&DestFile,
    FILE_WRITE_ACCESS,
    &ObjectAttributes,
    &IoStatusBlock,
    NULL,
    FILE_ATTRIBUTE_NORMAL,
    0,
    FILE_CREATE,
    FILE_SYNCHRONOUS_IO_NONALERT,
    NULL,
    0);
  if (!NT_SUCCESS(NtStatus))
    {
      DPRINT("NtCreateFile() failed (%S) (%x).\n", DestName, NtStatus);

      /* If file exists, ask to overwrite file */
      if (OverwriteHandler == NULL || OverwriteHandler(&File->File, FileName))
        {
          /* Create destination file, overwrite if it already exists */
          NtStatus = NtCreateFile(&DestFile,
            FILE_WRITE_ACCESS,
            &ObjectAttributes,
            &IoStatusBlock,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            0,
            FILE_OVERWRITE,
            FILE_SYNCHRONOUS_IO_ALERT,
            NULL,
            0);
          if (!NT_SUCCESS(NtStatus))
            {
              DPRINT("NtCreateFile() failed 2 (%S) (%x).\n", DestName, NtStatus);
              return CAB_STATUS_CANNOT_CREATE;
            }
        }
      else
        {
          DPRINT("File (%S) exists.\n", DestName);
          return CAB_STATUS_FILE_EXISTS;
        }
    }
  
  if (!ConvertDosDateTimeToFileTime(File->File.FileDate, File->File.FileTime, &FileTime))
    {
      NtClose(DestFile);
      DPRINT("DosDateTimeToFileTime() failed.\n");
      return CAB_STATUS_CANNOT_WRITE;
    }

  NtStatus = NtQueryInformationFile(DestFile,
    &IoStatusBlock,
    &FileBasic,
    sizeof(FILE_BASIC_INFORMATION),
    FileBasicInformation);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtQueryInformationFile() failed (%x).\n", NtStatus);
    }
  else
    {
      memcpy(&FileBasic.LastAccessTime, &FileTime, sizeof(FILETIME));

      NtStatus = NtSetInformationFile(DestFile,
        &IoStatusBlock,
        &FileBasic,
        sizeof(FILE_BASIC_INFORMATION),
        FileBasicInformation);
      if (!NT_SUCCESS(NtStatus))
        {
          DPRINT("NtSetInformationFile() failed (%x).\n", NtStatus);
        }
    }

  SetAttributesOnFile(File, DestFile);
  
  Buffer = RtlAllocateHeap(ProcessHeap, 0, CAB_BLOCKSIZE + 12); // This should be enough
  if (!Buffer)
    {
      NtClose(DestFile);
      DPRINT("Insufficient memory.\n");
      return CAB_STATUS_NOMEMORY; 
    }
  
  /* Call extract event handler */
  if (ExtractHandler != NULL)
    {
      ExtractHandler(&File->File, FileName);
    }
  
  /* Search to start of file */
  Offset = SeekInFile(FileHandle,
    File->DataBlock->AbsoluteOffset,
    NULL,
    SEEK_BEGIN,
    &NtStatus);
  if (!NT_SUCCESS(NtStatus))
    {
      DPRINT("SeekInFile() failed (%x).\n", NtStatus);
      return CAB_STATUS_INVALID_CAB;
    }
  
  Size   = File->File.FileSize;
  Offset = File->File.FileOffset;
  CurrentOffset = File->DataBlock->UncompOffset;
  
  Skip = TRUE;
  
  ReuseBlock = (CurrentDataNode == File->DataBlock);
  if (Size > 0)
    {
      do
        {
      	  DPRINT("CO (0x%X)    ReuseBlock (%d)    Offset (0x%X)   Size (%d)  BytesLeftInBlock (%d)\n",
      		  File->DataBlock->UncompOffset, (UINT)ReuseBlock, Offset, Size,
      		  BytesLeftInBlock);
      
      	  if (/*(CurrentDataNode != File->DataBlock) &&*/ (!ReuseBlock) || (BytesLeftInBlock <= 0))
            {
      		    DPRINT("Filling buffer. ReuseBlock (%d)\n", (UINT)ReuseBlock);
      
              CurrentBuffer  = Buffer;
              TotalBytesRead = 0;
              do
                {
                  DPRINT("Size (%d bytes).\n", Size);
  
                  if (((Status = ReadBlock(&CFData, sizeof(CFDATA), &BytesRead)) != 
                    CAB_STATUS_SUCCESS) || (BytesRead != sizeof(CFDATA)))
                    {
                      NtClose(DestFile);
                      RtlFreeHeap(ProcessHeap, 0, Buffer);
                      DPRINT("Cannot read from file (%d).\n", (UINT)Status);
                      return CAB_STATUS_INVALID_CAB;
                    }
      
                  DPRINT("Data block: Checksum (0x%X)  CompSize (%d bytes)  UncompSize (%d bytes)  Offset (0x%X).\n",
                    (UINT)CFData.Checksum,
                    (UINT)CFData.CompSize,
                    (UINT)CFData.UncompSize,
    				      (UINT)SeekInFile(FileHandle, 0, NULL, SEEK_CURRENT, &NtStatus));
    
                  //ASSERT(CFData.CompSize <= CAB_BLOCKSIZE + 12);
    
                  BytesToRead = CFData.CompSize;
    
            			DPRINT("Read: (0x%X,0x%X).\n",
            				CurrentBuffer, Buffer);

                  if (((Status = ReadBlock(CurrentBuffer, BytesToRead, &BytesRead)) != 
                      CAB_STATUS_SUCCESS) || (BytesToRead != BytesRead))
                    {
                      NtClose(DestFile);
                      RtlFreeHeap(ProcessHeap, 0, Buffer);
                      DPRINT("Cannot read from file (%d).\n", (UINT)Status);
                      return CAB_STATUS_INVALID_CAB;
                    }

                    /* FIXME: Does not work with files generated by makecab.exe */
/*
                  if (CFData.Checksum != 0)
                    {
                      ULONG Checksum = ComputeChecksum(CurrentBuffer, BytesRead, 0);
                      if (Checksum != CFData.Checksum)
                        {
                          NtClose(DestFile);
                          RtlFreeHeap(ProcessHeap, 0, Buffer);
                          DPRINT("Bad checksum (is 0x%X, should be 0x%X).\n",
                            Checksum, CFData.Checksum);
                          return CAB_STATUS_INVALID_CAB;
                        }
                    }
*/
                  TotalBytesRead += BytesRead;
    
                  CurrentBuffer += BytesRead;
    
                  if (CFData.UncompSize == 0)
                    {
                      if (wcslen(DiskNext) == 0)
                          return CAB_STATUS_NOFILE;

                      /* CloseCabinet() will destroy all file entries so in case
                         FileName refers to the FileName field of a CFFOLDER_NODE
                         structure, we have to save a copy of the filename */
                      wcscpy(TempName, FileName);
  
                      CloseCabinet();
  
                      CabinetSetCabinetName(CabinetNext);
  
                      if (DiskChangeHandler != NULL)
                        {
                          DiskChangeHandler(CabinetNext, DiskNext);
                        }
  
                      Status = CabinetOpen();
                      if (Status != CAB_STATUS_SUCCESS) 
                        return Status;
  
                      /* The first data block of the file will not be
                         found as it is located in the previous file */
                      Status = LocateFile(TempName, &File);
                      if (Status == CAB_STATUS_NOFILE)
                        {
                          DPRINT("Cannot locate file (%d).\n", (UINT)Status);
                          return Status;
                        }
    
                      /* The file is continued in the first data block in the folder */
                      File->DataBlock = CurrentFolderNode->DataListHead;
  
                      /* Search to start of file */
                      SeekInFile(FileHandle,
                        File->DataBlock->AbsoluteOffset,
                        NULL,
                        SEEK_BEGIN,
                        &NtStatus);
                      if (!NT_SUCCESS(NtStatus))
                        {
                          DPRINT("SeekInFile() failed (%x).\n", NtStatus);
                          return CAB_STATUS_INVALID_CAB;
                        }
    
                      DPRINT("Continuing extraction of file at uncompressed offset (0x%X)  Size (%d bytes)  AO (0x%X)  UO (0x%X).\n",
                          (UINT)File->File.FileOffset,
                          (UINT)File->File.FileSize,
                          (UINT)File->DataBlock->AbsoluteOffset,
                          (UINT)File->DataBlock->UncompOffset);
    
                      CurrentDataNode = File->DataBlock;
                      ReuseBlock = TRUE;

                      RestartSearch = TRUE;
                    }
                } while (CFData.UncompSize == 0);

              DPRINT("TotalBytesRead (%d).\n", TotalBytesRead);
  
              Status = CodecUncompress(OutputBuffer, Buffer, TotalBytesRead, &BytesToWrite);
              if (Status != CS_SUCCESS)
                {
                  NtClose(DestFile);
                  RtlFreeHeap(ProcessHeap, 0, Buffer);
                  DPRINT("Cannot uncompress block.\n");
                  if (Status == CS_NOMEMORY)
                    return CAB_STATUS_NOMEMORY;
                  return CAB_STATUS_INVALID_CAB;
                }

              if (BytesToWrite != CFData.UncompSize)
                {
                  DPRINT("BytesToWrite (%d) != CFData.UncompSize (%d)\n",
                    BytesToWrite, CFData.UncompSize);
                  return CAB_STATUS_INVALID_CAB;
                }

              BytesLeftInBlock = BytesToWrite;
            }
          else
            {
              DPRINT("Using same buffer. ReuseBlock (%d)\n", (UINT)ReuseBlock);
    
              BytesToWrite = BytesLeftInBlock;
    
          		DPRINT("Seeking to absolute offset 0x%X.\n",
          			CurrentDataNode->AbsoluteOffset + sizeof(CFDATA) +
                  CurrentDataNode->Data.CompSize);
    
          		if (((Status = ReadBlock(&CFData, sizeof(CFDATA), &BytesRead)) != 
          			CAB_STATUS_SUCCESS) || (BytesRead != sizeof(CFDATA)))
                {
            			NtClose(DestFile);
                  RtlFreeHeap(ProcessHeap, 0, Buffer);
            			DPRINT("Cannot read from file (%d).\n", (UINT)Status);
            			return CAB_STATUS_INVALID_CAB;
            		}
    
          		DPRINT("CFData.CompSize 0x%X  CFData.UncompSize 0x%X.\n",
          			CFData.CompSize, CFData.UncompSize);
    
              /* Go to next data block */
              SeekInFile(FileHandle,
                CurrentDataNode->AbsoluteOffset + sizeof(CFDATA) +
                CurrentDataNode->Data.CompSize,
                NULL,
                SEEK_BEGIN,
                &NtStatus);
              if (!NT_SUCCESS(NtStatus))
                {
                  DPRINT("SeekInFile() failed (%x).\n", NtStatus);
                  return CAB_STATUS_INVALID_CAB;
                }

              ReuseBlock = FALSE;
            }

          if (Skip)
              BytesSkipped = (Offset - CurrentOffset);
          else
              BytesSkipped = 0;
  
  	      BytesToWrite -= BytesSkipped;
  
        	if (Size < BytesToWrite)
            BytesToWrite = Size;

          DPRINT("Offset (0x%X)  CurrentOffset (0x%X)  ToWrite (%d)  Skipped (%d)(%d)  Size (%d).\n",
            (UINT)Offset,
            (UINT)CurrentOffset,
            (UINT)BytesToWrite,
            (UINT)BytesSkipped, (UINT)Skip,
            (UINT)Size);
  
//          if (!WriteFile(DestFile, (PVOID)((ULONG)OutputBuffer + BytesSkipped),
//            BytesToWrite, &BytesWritten, NULL) ||
//              (BytesToWrite != BytesWritten))

          NtStatus = NtWriteFile(DestFile,
            NULL,
            NULL,
            NULL,
            &IoStatusBlock,
            (PVOID)((ULONG)OutputBuffer + BytesSkipped), 
            BytesToWrite,
            NULL,
            NULL);
          BytesWritten = BytesToWrite;
          if (!NT_SUCCESS(NtStatus))
            {
      		    DPRINT("Status 0x%X.\n", NtStatus);
  
              NtClose(DestFile);
              RtlFreeHeap(ProcessHeap, 0, Buffer);
              DPRINT("Cannot write to file.\n");
              return CAB_STATUS_CANNOT_WRITE;
            }
          Size -= BytesToWrite;
  
    	    CurrentOffset += BytesToWrite;

          /* Don't skip any more bytes */
          Skip = FALSE;
        } while (Size > 0);
    }

  NtClose(DestFile);

  RtlFreeHeap(ProcessHeap, 0, Buffer);

  return CAB_STATUS_SUCCESS;
}


VOID
CabinetSelectCodec(ULONG Id)
/*
 * FUNCTION: Selects codec engine to use
 * ARGUMENTS:
 *     Id = Codec identifier
 */
{
  if (CodecSelected)
    {
      if (Id == CodecId)
        return;

      CodecSelected = FALSE;
    }

  switch (Id)
    {
      case CAB_CODEC_RAW:
        CodecUncompress = RawCodecUncompress;
        break;
      case CAB_CODEC_MSZIP:
        CodecUncompress = MSZipCodecUncompress;
        break;
      default:
        return;
    }

  CodecId = Id;
  CodecSelected = TRUE;
}


VOID
CabinetSetEventHandlers(PCABINET_OVERWRITE Overwrite,
  PCABINET_EXTRACT Extract,
  PCABINET_DISK_CHANGE DiskChange)
/*
 * FUNCTION: Set event handlers
 * ARGUMENTS:
 *     Overwrite = Handler called when a file is to be overwritten
 *     Extract = Handler called when a file is to be extracted
 *     DiskChange = Handler called when changing the disk
 */
{
  OverwriteHandler = Overwrite;
  ExtractHandler = Extract;
  DiskChangeHandler = DiskChange;
}


PVOID
CabinetGetCabinetReservedArea(PULONG Size)
/*
 * FUNCTION: Get pointer to cabinet reserved area. NULL if none
 */
{
  if (CabinetReservedArea != NULL)
    {
      if (Size != NULL)
        {
          *Size = CabinetReserved;
        }
      return CabinetReservedArea;
    }
  else
    {
      if (Size != NULL)
        {
          *Size = 0;
        }
      return NULL;
    }
}
