
#ifndef _FILESHAR_H_
#define _FILESHAR_H_

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

DECLARE_HANDLE(HSHFILE);

#ifdef _WIN32
#ifndef _huge
#define _huge
#endif
#endif

HSHFILE WINAPI shfileOpen(LPTSTR szFileName, MMIOINFO FAR* lpmmioinfo,
    DWORD dwOpenFlags);
UINT WINAPI shfileClose(HSHFILE hsh, UINT uFlags);
LONG WINAPI shfileRead(HSHFILE hsh, HPSTR pch, LONG cch);
LONG WINAPI shfileWrite(HSHFILE hsh, const char _huge* pch, LONG cch);
LONG WINAPI shfileSeek(HSHFILE hsh, LONG lOffset, int iOrigin);
LONG WINAPI shfileFlush(HSHFILE hsh, UINT uFlags);
LONG WINAPI shfileZero(HSHFILE hsh, LONG lBytes);


LONG WINAPI shfileAddRef(HSHFILE hsh);
LONG WINAPI shfileRelease(HSHFILE hsh);

#ifdef USE_DIRECTIO
BOOL shfileIsDirect(HSHFILE hsh);
void shfileStreamStart(HSHFILE hsh);
void shfileStreamStop(HSHFILE hsh);
#endif


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

#endif // _FILESHAR_H_
