/*
    AG: These are just temporary stubs for comctl32.dll - I am working on
    porting WinMM over, offline. Once it compiles and links, I will commit
    it.
*/


#include <windows.h>
typedef UINT *LPUINT;
#include <mmsystem.h>

MMRESULT WINAPI mmioAscend(HMMIO hmmio, LPMMCKINFO lpck, UINT wFlags)
{
    return MMSYSERR_NOERROR;
}

MMRESULT WINAPI mmioClose(HMMIO hmmio, UINT wFlags)
{
    return MMSYSERR_NOERROR;
}

MMRESULT WINAPI mmioDescend(HMMIO hmmio, LPMMCKINFO lpck, const MMCKINFO* lpckParent, UINT wFlags)
{
    return MMSYSERR_NOERROR;
}

HMMIO WINAPI mmioOpenA(LPSTR szFilename, LPMMIOINFO lpmmioinfo, DWORD dwOpenFlags)
{
    return 12345;
}

LONG WINAPI mmioRead(HMMIO hmmio, HPSTR pch, LONG cch)
{
    return 0;
}

LONG WINAPI mmioSeek(HMMIO hmmio, LONG lOffset, int iOrigin)
{
    return 0;
}
