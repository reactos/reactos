/*
 */

#ifndef ROSRTL_PATH_H__
#define ROSRTL_PATH_H__

#ifdef __cplusplus
extern "C"
{
#endif

BOOL STDCALL MakeSureDirectoryPathExistsExA(LPCSTR DirPath, BOOL FileAtEnd);
BOOL STDCALL MakeSureDirectoryPathExistsExW(LPCWSTR DirPath, BOOL FileAtEnd);

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
