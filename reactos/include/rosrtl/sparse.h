/* $Id: sparse.h,v 1.1 2004/09/15 18:57:01 weiden Exp $
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

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
