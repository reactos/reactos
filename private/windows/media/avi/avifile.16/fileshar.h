
#if !defined _FILESHAR_H
#define _FILESHAR_H

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

DECLARE_HANDLE(HSHFILE);

HSHFILE WINAPI shfileOpen(LPSTR szFileName, MMIOINFO FAR* lpmmioinfo,
    DWORD dwOpenFlags);
UINT WINAPI shfileClose(HSHFILE hsh, UINT uFlags);
LONG WINAPI shfileRead(HSHFILE hsh, HPSTR pch, LONG cch);
LONG WINAPI shfileWrite(HSHFILE hsh, const char _huge* pch, LONG cch);
LONG WINAPI shfileSeek(HSHFILE hsh, LONG lOffset, int iOrigin);
LONG WINAPI shfileFlush(HSHFILE hsh, UINT uFlags);


LONG WINAPI shfileAddRef(HSHFILE hsh);
LONG WINAPI shfileRelease(HSHFILE hsh);

#ifndef _MMRESULT_
#define _MMRESULT_
typedef UINT                MMRESULT;
#endif

MMRESULT WINAPI
shfileDescend(HSHFILE hshfile, LPMMCKINFO lpck, const LPMMCKINFO lpckParent, UINT wFlags);
MMRESULT WINAPI
shfileAscend(HSHFILE hshfile, LPMMCKINFO lpck, UINT wFlags);
MMRESULT WINAPI
shfileCreateChunk(HSHFILE hshfile, LPMMCKINFO lpck, UINT wFlags);


#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#endif // _FILESHAR_H
