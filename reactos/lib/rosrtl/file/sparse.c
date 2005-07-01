#if 0
#include <windows.h>
#include <ddk/ntifs.h>
#include <string.h>
#include <rosrtl/sparse.h>

/*
 * Utility to convert a file to a sparse file
 *
 * IN HANDLE hFile -> Handle to the file to be converted
 *
 * Returns TRUE on success.
 * Returns FALSE on failure, use GetLastError() to get extended error information.
 */
BOOL
STDCALL
SetFileSparse(HANDLE hFile)
{
  DWORD BytesRet;
  return DeviceIoControl(hFile,
                         FSCTL_SET_SPARSE,
                         NULL,
                         0,
                         NULL,
                         0,
                         &BytesRet,
                         NULL) != 0;
}


/*
 * Utility to fill a specified range of a file with zeroes.
 *
 * IN HANDLE hFile -> Handle to the file.
 * IN PLARGE_INTEGER pliFileOffset -> Points to a LARGE_INTEGER structure that indicates the file offset of the start of the range in bytes.
 * IN PLARGE_INTEGER pliBeyondFinalZero -> Points to a LARGE_INTEGER structure that indicates the the offset to the first byte beyond the last zeroed byte.
 *
 * Returns TRUE on success.
 * Returns FALSE on failure, use GetLastError() to get extended error information.
 */
BOOL
STDCALL
ZeroFileData(HANDLE hFile,
             PLARGE_INTEGER pliFileOffset,
             PLARGE_INTEGER pliBeyondFinalZero)
{
  DWORD BytesRet;
  FILE_ZERO_DATA_INFORMATION fzdi;
  
  if(!pliFileOffset || !pliBeyondFinalZero)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  fzdi.FileOffset = *pliFileOffset;
  fzdi.BeyondFinalZero = *pliBeyondFinalZero;
  
  return DeviceIoControl(hFile,
                         FSCTL_SET_ZERO_DATA,
                         &fzdi,
                         sizeof(FILE_ZERO_DATA_INFORMATION),
                         NULL,
                         0,
                         &BytesRet,
                         NULL) != 0;
}


/*
 * Utility to determine the allocated ranges of a sparse file
 *
 * IN HANDLE hFile -> Handle to the file.
 * IN PLARGE_INTEGER pliFileOffset -> Points to a LARGE_INTEGER structure that indicates the portion of the file to search for allocated ranges.
 * IN PLARGE_INTEGER pliLength -> Points to a LARGE_INTEGER structure that indicates it's size.
 * OUT PFILE_ALLOCATED_RANGE_BUFFER lpAllocatedRanges -> Points to a buffer that receives an array of FILE_ALLOCATED_RANGE_BUFFER structures.
 * IN DWORD dwBufferSize -> Size of the output buffer.
 *
 * Returns a nonzero value on success.
 * Returns zero on failure, use GetLastError() to get extended error information.
 */
DWORD
STDCALL
QueryAllocatedFileRanges(HANDLE hFile,
                         PLARGE_INTEGER pliFileOffset,
                         PLARGE_INTEGER pliLength,
                         PFILE_ALLOCATED_RANGE_BUFFER lpAllocatedRanges,
                         DWORD dwBufferSize)
{
  DWORD BytesRet;
  FILE_ALLOCATED_RANGE_BUFFER farb;
  
  if(!pliFileOffset || !pliLength || !lpAllocatedRanges || !dwBufferSize)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  farb.FileOffset = *pliFileOffset;
  farb.Length = *pliLength;
  
  if(DeviceIoControl(hFile,
                     FSCTL_QUERY_ALLOCATED_RANGES,
                     &farb,
                     sizeof(FILE_ALLOCATED_RANGE_BUFFER),
                     lpAllocatedRanges,
                     dwBufferSize,
                     &BytesRet,
                     NULL) != 0)
  {
    return BytesRet;
  }
  
  return 0;
}

#endif
