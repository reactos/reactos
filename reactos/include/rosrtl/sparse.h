/* $Id: sparse.h,v 1.2 2004/09/15 19:01:23 weiden Exp $
 */

#ifndef ROSRTL_SPARSE_H__
#define ROSRTL_SPARSE_H__

#ifdef __cplusplus
extern "C"
{
#endif

BOOL
STDCALL
SetFileSparse(HANDLE hFile);

BOOL
STDCALL
ZeroFileData(HANDLE hFile,
             PLARGE_INTEGER pliFileOffset,
             PLARGE_INTEGER pliBeyondFinalZero);

DWORD
STDCALL
QueryAllocatedFileRanges(HANDLE hFile,
                         PLARGE_INTEGER pliFileOffset,
                         PLARGE_INTEGER pliLength,
                         PFILE_ALLOCATED_RANGE_BUFFER lpAllocatedRanges,
                         DWORD dwBufferSize);

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
