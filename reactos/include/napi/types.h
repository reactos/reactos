#ifndef __INCLUDE_NAPI_TYPES_H
#define __INCLUDE_NAPI_TYPES_H

// these should be moved to a file like ntdef.h

typedef const int CINT;

typedef LONG NTSTATUS, *PNTSTATUS;

typedef ULONG DEVICE_TYPE;





enum
{
   DIRECTORY_QUERY,
   DIRECTORY_TRAVERSE,
   DIRECTORY_CREATE_OBJECT,
   DIRECTORY_CREATE_SUBDIRECTORY,
   DIRECTORY_ALL_ACCESS,
};

/*
 * General type for status information
 */
//typedef LONG NTSTATUS;

typedef struct _UNICODE_STRING
{
   USHORT Length;
   USHORT MaximumLength;
   PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;


/*  File information for IRP_MJ_QUERY_INFORMATION (and SET)  */
typedef enum _FILE_INFORMATION_CLASS
{
  FileDirectoryInformation = 1,
  FileFullDirectoryInformation,
  FileBothDirectoryInformation,
  FileBasicInformation,
  FileStandardInformation,
  FileInternalInformation,
  FileEaInformation,
  FileAccessInformation,
  FileNameInformation,
  FileRenameInformation,
  FileLinkInformation,
  FileNamesInformation,
  FileDispositionInformation,
  FilePositionInformation,
  FileFullEaInformation,
  FileModeInformation,
  FileAlignmentInformation,
  FileAllInformation,
  FileAllocationInformation,
  FileEndOfFileInformation,
  FileAlternateNameInformation,
  FileStreamInformation,
  FilePipeInformation,
  FilePipeLocalInformation,
  FilePipeRemoteInformation,
  FileMailslotQueryInformation,
  FileMailslotSetInformation,
  FileCompressionInformation,
  FileCopyOnWriteInformation,
  FileCompletionInformation,
  FileMoveClusterInformation,
  FileOleClassIdInformation,
  FileOleStateBitsInformation,
  FileNetworkOpenInformation,
  FileObjectIdInformation,
  FileOleAllInformation,
  FileOleDirectoryInformation,
  FileContentIndexInformation,
  FileInheritContentIndexInformation,
  FileOleInformation,
  FileMaximumInformation,
} FILE_INFORMATION_CLASS;

typedef ULONG WAIT_TYPE;
typedef USHORT CSHORT;


#if 0
typedef struct _TIME {
	DWORD LowPart;
	LONG HighPart;
} TIME, *PTIME;
#endif

typedef ULARGE_INTEGER TIME, *PTIME;

#endif /* __INCLUDE_NAPI_TYPES_H */
