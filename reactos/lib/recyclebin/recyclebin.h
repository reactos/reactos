#ifndef __RECYCLEBIN_H
#define __RECYCLEBIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#define ANY_SIZE 1

typedef struct _DELETED_FILE_DETAILS_A
{
	FILETIME      LastModification;
	FILETIME      DeletionTime;
	ULARGE_INTEGER FileSize;
	ULARGE_INTEGER PhysicalFileSize;
	DWORD         Attributes;
	CHAR          FileName[ANY_SIZE];
} DELETED_FILE_DETAILS_A, *PDELETED_FILE_DETAILS_A;
typedef struct _DELETED_FILE_DETAILS_W
{
	FILETIME      LastModification;
	FILETIME      DeletionTime;
	ULARGE_INTEGER FileSize;
	ULARGE_INTEGER PhysicalFileSize;
	DWORD         Attributes;
	WCHAR         FileName[ANY_SIZE];
} DELETED_FILE_DETAILS_W, *PDELETED_FILE_DETAILS_W;
#ifdef UNICODE
#define DELETED_FILE_DETAILS  DELETED_FILE_DETAILS_W
#define PDELETED_FILE_DETAILS PDELETED_FILE_DETAILS_W
#else
#define DELETED_FILE_DETAILS  DELETED_FILE_DETAILS_A
#define PDELETED_FILE_DETAILS PDELETED_FILE_DETAILS_A
#endif

typedef BOOL (WINAPI *PENUMERATE_RECYCLEBIN_CALLBACK)(IN PVOID Context, IN HANDLE hDeletedFile);

BOOL WINAPI
CloseRecycleBinHandle(
	IN HANDLE hDeletedFile);

BOOL WINAPI
DeleteFileToRecycleBinA(
	IN LPCSTR FileName);
BOOL WINAPI
DeleteFileToRecycleBinW(
	IN LPCWSTR FileName);
#ifdef UNICODE
#define DeleteFileToRecycleBin DeleteFileToRecycleBinW
#else
#define DeleteFileToRecycleBin DeleteFileToRecycleBinA
#endif

BOOL WINAPI
EmptyRecycleBinA(
	IN CHAR driveLetter);
BOOL WINAPI
EmptyRecycleBinW(
	IN WCHAR driveLetter);
#ifdef UNICODE
#define EmptyRecycleBin EmptyRecycleBinW
#else
#define EmptyRecycleBin EmptyRecycleBinA
#endif

BOOL WINAPI
EnumerateRecycleBinA(
	IN CHAR driveLetter,
	IN PENUMERATE_RECYCLEBIN_CALLBACK pFnCallback,
	IN PVOID Context OPTIONAL);
BOOL WINAPI
EnumerateRecycleBinW(
	IN WCHAR driveLetter,
	IN PENUMERATE_RECYCLEBIN_CALLBACK pFnCallback,
	IN PVOID Context OPTIONAL);
#ifdef UNICODE
#define EnumerateRecycleBin EnumerateRecycleBinW
#else
#define EnumerateRecycleBin EnumerateRecycleBinA
#endif

BOOL WINAPI
GetDeletedFileDetailsA(
	IN HANDLE hDeletedFile,
	IN DWORD BufferSize,
	IN OUT PDELETED_FILE_DETAILS_A FileDetails OPTIONAL,
	OUT LPDWORD RequiredSize OPTIONAL);
BOOL WINAPI
GetDeletedFileDetailsW(
	IN HANDLE hDeletedFile,
	IN DWORD BufferSize,
	IN OUT PDELETED_FILE_DETAILS_W FileDetails OPTIONAL,
	OUT LPDWORD RequiredSize OPTIONAL);
#ifdef UNICODE
#define GetDeletedFileDetails GetDeletedFileDetailsW
#else
#define GetDeletedFileDetails GetDeletedFileDetailsA
#endif

BOOL WINAPI
RestoreFile(
	IN HANDLE hDeletedFile);

#ifdef __cplusplus
}
#endif

#endif /* __RECYCLEBIN_H */
