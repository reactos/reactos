/****************************************************************************
 *
 *   capio.c
 *
 *   i/o routines for video capture
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992 - 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 ***************************************************************************/

//#define USE_AVIFILE 1
#define JMK_HACK_DONTWRITE TRUE

#define INC_OLE2
#pragma warning(disable:4103)
#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <mmsystem.h>
#include <vfw.h>
#include <mmreg.h>
#include <mmddk.h>

#include "ivideo32.h"
#include "mmdebug.h"

#ifdef USE_ACM
#include <msacm.h>
#endif

#include "avicapi.h"
#include "time.h"

extern UINT GetSizeOfWaveFormat (LPWAVEFORMATEX lpwf);
STATICFN BOOL IndexVideo (LPCAPSTREAM lpcs, DWORD dwSize, BOOL bKeyFrame);
STATICFN BOOL IndexAudio (LPCAPSTREAM lpcs, DWORD dwSize);

#ifdef _DEBUG
    #define DSTATUS(lpcs, sz) statusUpdateStatus(lpcs, IDS_CAP_INFO, (LPTSTR) TEXT(sz))
#else
    #define DSTATUS(lpcs, sz)
#endif

//
// Define function variables for dynamically linking to the async IO
// completion routines on NT.  This should allow the same code to run
// on Win95 which does not have these entry points.
//

HANDLE (WINAPI *pfnCreateIoCompletionPort)(
    HANDLE FileHandle,
    HANDLE ExistingCompletionPort,
    DWORD CompletionKey,
    DWORD NumberOfConcurrentThreads
    );

BOOL (WINAPI *pfnGetQueuedCompletionStatus)(
    HANDLE CompletionPort,
    LPDWORD lpNumberOfBytesTransferred,
    LPDWORD lpCompletionKey,
    LPOVERLAPPED *lpOverlapped,
    DWORD dwMilliseconds
    );

HINSTANCE hmodKernel;           // Handle to loaded Kernel32.dll

#ifdef USE_AVIFILE
#include "capio.avf"
#else //---------------- ! using Avifile ----------------------------

// The following are anded with the size in the index
#define IS_AUDIO_CHUNK        0x80000000
#define IS_KEYFRAME_CHUNK     0x40000000
#define IS_DUMMY_CHUNK        0x20000000
#define IS_GRANULAR_CHUNK     0x10000000
#define INDEX_MASK  (IS_AUDIO_CHUNK | IS_KEYFRAME_CHUNK | IS_DUMMY_CHUNK | IS_GRANULAR_CHUNK)


// Add an index entry for a video frame
// dwSize is the size of data ONLY, not including the chunk or junk
// Returns: TRUE if index space is not exhausted
//
STATICFN BOOL IndexVideo (LPCAPSTREAM lpcs, DWORD dwSize, BOOL bKeyFrame)
{
    if (lpcs->dwIndex < lpcs->sCapParms.dwIndexSize) {
        *lpcs->lpdwIndexEntry = dwSize | (bKeyFrame ? IS_KEYFRAME_CHUNK : 0);
        ++lpcs->lpdwIndexEntry;
        ++lpcs->dwIndex;
        ++lpcs->dwVideoChunkCount;
        return TRUE;
    }
    dprintf("\n***WARNING*** Indexvideo space exhausted\n");
    return FALSE;
}

// Add an index entry for an audio buffer
// dwSize is the size of data ONLY, not including the chunk or junk
// Returns: TRUE if index space is not exhausted
//
STATICFN BOOL IndexAudio (LPCAPSTREAM lpcs, DWORD dwSize)
{
    if (lpcs->dwIndex < lpcs->sCapParms.dwIndexSize) {
       *lpcs->lpdwIndexEntry = dwSize | IS_AUDIO_CHUNK;
       ++lpcs->lpdwIndexEntry;
       ++lpcs->dwIndex;
       ++lpcs->dwWaveChunkCount;
       return TRUE;
    }
    dprintf("\n***WARNING*** Indexaudio space exhausted\n");
    return FALSE;
}

DWORD CalcWaveBufferSize (LPCAPSTREAM lpcs)
{
    DWORD dw;

    if (!lpcs->lpWaveFormat)
        return 0L;

    // at least .5 second
    dw = lpcs->lpWaveFormat->nAvgBytesPerSec / 2;
    if (lpcs->sCapParms.wChunkGranularity) {
        if (dw % lpcs->sCapParms.wChunkGranularity) {
            dw += lpcs->sCapParms.wChunkGranularity -
                dw % lpcs->sCapParms.wChunkGranularity;
        }
    }
    dw = max ((1024L * 16), dw);                // at least 16K
    dw -= sizeof(RIFF);

    dprintf("Wave buffer size = %ld", dw);
    return dw;
}

/*
 * AVIPreloadFat
 *
 *   Force FAT for this file to be loaded into the FAT cache
 *
 */

VOID WINAPI AVIPreloadFat (LPCAPSTREAM lpcs)
{
    DWORD dw;
   #ifdef CHICAGO
    DWORD dwPos;

    assert (lpcs->lpDropFrame);

    // save the current file pointer then seek to the end of the file
    //
    dwPos = SetFilePointer (lpcs->hFile, 0, NULL, FILE_CURRENT);
    dw = SetFilePointer (lpcs->hFile, 0, NULL, FILE_END);
    if ((dw == (DWORD)-1) || (dw < lpcs->dwBytesPerSector)) {
        // put the file pointer back to where it was
        SetFilePointer (lpcs->hFile, dwPos, NULL, FILE_BEGIN);
        return;
    }

    // read the last sector of the file, just to force
    // the fat for the file to be loaded
    //
    ReadFile (lpcs->hFile, lpcs->lpDropFrame, lpcs->dwBytesPerSector, &dw, NULL);

    // put the file pointer back to where it was
    //
    SetFilePointer (lpcs->hFile, dwPos, NULL, FILE_BEGIN);
   #else
    // Load all the FAT information.   On NT this is sufficient for FAT
    // files.  On NTFS partitiions there is no way we can read in all the
    // mapping information.
    GetFileSize(lpcs->hFile, &dw);
   #endif
}


#ifdef JMK_HACK_DONTWRITE
static BOOL bDontWrite;
#endif

// Write data to the capture file
// Returns: TRUE on a successful write
//
UINT NEAR PASCAL AVIWrite (
    LPCAPSTREAM lpcs,
    LPVOID      pbuf,
    DWORD       dwSize,
    UINT        uIndex, // index of header for this buffer, -1 for step capture
    UINT        uType,
    LPBOOL      lpbPending)
{
    DWORD dwWritten;
    DWORD dwGran;

    // the buffer must be sector aligned if using non-buffered IO
    // and the size must be at least word aligned
    // uIndex == -1 if this is a dummy frame write
    // uIndex == Index into alpVideoHdr OR index in alpWaveHdr based on uType
    //
    assert (!lpcs->fUsingNonBufferedIO || (!((DWORD_PTR)pbuf & (lpcs->dwBytesPerSector - 1))));
    assert (!(dwSize & 1));
    assert (dwSize);

    assert (*lpbPending == FALSE);

    // if we are doing non-buffered io, we need to pad each write
    // to an even multiple of sector size bytes, we do this by adding
    // a junk riff chunk into the write buffer after dwSize bytes
    //
    dwGran = lpcs->sCapParms.wChunkGranularity;
    if (lpcs->fUsingNonBufferedIO)
       dwGran = max (lpcs->dwBytesPerSector,
                (DWORD) lpcs->sCapParms.wChunkGranularity);

    assert (dwGran);

    if (dwSize % dwGran)
    {
        DWORD dwSizeT = dwGran - (dwSize % dwGran);
        LPRIFF priff = (LPRIFF)((LPBYTE)pbuf + dwSize + (dwSize & 1));

        if (dwSizeT < sizeof(RIFF))
            dwSizeT += dwGran;

        // add the junk riff chunk to the end of the buffer
        //
        priff->dwType = ckidAVIPADDING;
        priff->dwSize = dwSizeT - sizeof(RIFF);
        dwSize += dwSizeT;
    }

   #ifdef _DEBUG
    if (dwSize)
    {
        volatile BYTE bt;
        AuxDebugEx (8, DEBUGLINE "touch test of AviWrite buffer %08X\r\n", pbuf);
        bt = ((LPBYTE)pbuf)[dwSize-1];
    }

    // List all of the RIFF chunks within the block being written
    //
    dwWritten = 0;
    while (dwWritten < dwSize)
    {
        LPRIFF priff = (LPVOID)((LPBYTE)pbuf + dwWritten);
        AuxDebugEx (4, DEBUGLINE "RIFF=%.4s size=%08X\r\n",
                   &priff->dwType, priff->dwSize);
        dwWritten += priff->dwSize + sizeof(RIFF);
    }
   #endif


    // BUGBUG, Remove the following line when done performance testing
   #ifdef JMK_HACK_DONTWRITE
    if (bDontWrite)
        return 0;
   #endif

    if (lpcs->pAsync)
    {
        struct _avi_async * lpah = &lpcs->pAsync[lpcs->iLastAsync];
        UINT  iLastAsync;

        // set iLastAsync to point to what lpcs->iLastAsync
        // would be if we were to increment it.  If we end up
        // with an i/o that does not complete synchronously
        // we will then update lpcs->iLastAsync so that we can
        // remember to check for completion later
        //
        if ((iLastAsync = lpcs->iLastAsync+1) >= lpcs->iNumAsync)
            iLastAsync = 0;

        // is the async buffer that we are trying to use
        // already in use?
        //
        if (iLastAsync == lpcs->iNextAsync) {
	    AuxDebugEx(1, DEBUGLINE "async buffer already in use\r\n");
            return IDS_CAP_FILE_WRITE_ERROR;
	}
        assert (!lpah->uType);

        // init the async buffer with the info that we will need
        // to release the buffer when the io is complete
        //
        ZeroMemory (&lpah->ovl, sizeof(lpah->ovl));
        if (uIndex == -1) {
            // We want a synchronous write
            assert (!(((DWORD_PTR)(lpcs->heSyncWrite))&1));
            lpah->ovl.hEvent = (HANDLE)(((DWORD_PTR)lpcs->heSyncWrite) | 1);
            // OR'ing hEvent with 1 prevents the IOCompletionPort being used
	    // ...and I know this sounds a bit tacky but this is what the
	    // docs actually say.
        } else {
            lpah->ovl.hEvent = 0;
        }

        lpah->ovl.Offset = lpcs->dwAsyncWriteOffset;
        // attempt an async write.  if WriteFile fails, we then
        // need to check if it's a real failure, or just an instance
        // of delayed completion.  if delayed completion, we fill out
        // the lpah structure so that we know what buffer to re-use
        // when the io finally completes.
        //
	if ( ! WriteFile (lpcs->hFile, pbuf, dwSize, &dwWritten, &lpah->ovl))
        {
            UINT n = GetLastError();
            if ((ERROR_IO_PENDING == n) || (ERROR_INVALID_HANDLE == n))
            {
                // if we are passed a index of -1, that means that
                // this buffer is not associated with any entry in the
                // header array.  in this case, we must have the io complete
                // before we return from this function.
               //
               if (uIndex == (UINT)-1)
               {
                    AuxDebugEx(3, "Waiting for a block to write synchonously\n");
                    if ( ! GetOverlappedResult (lpcs->hFile, &lpah->ovl, &dwWritten, TRUE))
                    {
                        AuxDebugEx (1, DEBUGLINE "WriteFile failed %d\r\n", GetLastError());
                        return IDS_CAP_FILE_WRITE_ERROR;
                    }
                }
                else
                {
                    // io is begun, but not yet completed. so setup info in
                    // the pending io array so that we can check later for completion
                    //
                    *lpbPending = TRUE;
                    lpah->uType = uType | ASYNCIOPENDING;
                    lpah->uIndex = uIndex;
		    AuxDebugEx(2, DEBUGLINE "IOPending... iLastAsync was %d, will be %d, uIndex=%d, Event=%d\r\n",lpcs->iLastAsync , iLastAsync, uIndex, lpah->ovl.hEvent);
                    lpcs->iLastAsync = iLastAsync;
                }
            }
            else
            {
                AuxDebugEx (1, DEBUGLINE "WriteFile failed %d\r\n", GetLastError());
                return IDS_CAP_FILE_WRITE_ERROR;
	    }
	}

        // we get to here only when the io succeeds or is pending
        // so update the seek offset for use in the next write operation
        //
        lpcs->dwAsyncWriteOffset += dwSize;
    }
    else
    {
	// We are writing synchronously to the file
        if (!WriteFile (lpcs->hFile, pbuf, dwSize, &dwWritten, NULL) ||
            !(dwWritten == dwSize))
            return IDS_CAP_FILE_WRITE_ERROR;
    }

    return 0;
}

/*
 * CapFileInit
 *
 *       Perform all initialization required to write a capture file.
 *
 *       We take a slightly strange approach: We don't write
 *       out the header until we're done capturing.  For now,
 *       we just seek 2K into the file, which is where all of
 *       the real data will go.
 *
 *       When we're done, we'll come back and write out the header,
 *       because then we'll know all of the values we need.
 *
 *      Also allocate and init the index.
 */
BOOL CapFileInit (LPCAPSTREAM lpcs)
{
    LONG l;
    LPBITMAPINFO lpBitsInfoOut;    // Possibly compressed output format
    DWORD dwOpenFlags;

    // No special video format given -- use the default
    lpBitsInfoOut = lpcs->CompVars.lpbiOut;
    if (lpcs->CompVars.hic == NULL)
        lpBitsInfoOut = lpcs->lpBitsInfo;


    assert (lpcs->hmmio == NULL);   // Should never have a file handle on entry

    // if the capture file has not been set then set it now
    if (!(*lpcs->achFile))
         goto INIT_FILE_OPEN_ERROR;

    // Get the Bytes per sector for the drive
    {
        DWORD dwSectorsPerCluster;
        DWORD dwFreeClusters;
        DWORD dwClusters;
        TCHAR szFullPathName[MAX_PATH];
        LPTSTR pszFilePart;

        GetFullPathName (lpcs->achFile,
                NUMELMS (szFullPathName),
                szFullPathName,
                &pszFilePart);

        if (szFullPathName[1] == TEXT(':') && szFullPathName[2] == TEXT('\\')) {
            szFullPathName[3] = TEXT('\0');  // Terminate after "x:\"

            GetDiskFreeSpace (szFullPathName,
                   &dwSectorsPerCluster,
                   &lpcs->dwBytesPerSector,
                   &dwFreeClusters,
                   &dwClusters);
            AuxDebugEx (3, DEBUGLINE "BytesPerSector=%d\r\n",
                lpcs->dwBytesPerSector);
        }
        else {
            // This handles cases where we do not have a "x:\" filename
            // Principally this will be "\\server\name\path..."
            lpcs->dwBytesPerSector = DEFAULT_BYTESPERSECTOR;
            AuxDebugEx (3, DEBUGLINE "FullPath=%s\r\n", szFullPathName);
            AuxDebugEx (3, DEBUGLINE "GetFullPath failed, Forcing dwBytesPerSector to %d\r\n",DEFAULT_BYTESPERSECTOR);
        }

    // bytes per sector must be non-zero and a power of two
    //
    assert (lpcs->dwBytesPerSector);
    assert (!(lpcs->dwBytesPerSector & (lpcs->dwBytesPerSector-1)));
    }

   #ifdef ZERO_THE_FILE_FOR_TESTING
    {
    char c[64 * 1024];
    DWORD dwSize;
    DWORD dwBW;
    // Open the file just to zero it

    lpcs->hFile = CreateFile (lpcs->achFile,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (lpcs->hFile == INVALID_HANDLE_VALUE) {
        lpcs->hFile = 0;
        goto INIT_FILE_OPEN_ERROR;
    }

    ZeroMemory (c, sizeof(c));
    SetFilePointer (lpcs->hFile, 0, NULL, FILE_BEGIN);
    dwSize = GetFileSize (lpcs->hFile, NULL);

    while (SetFilePointer (lpcs->hFile, 0, NULL, FILE_CURRENT) < dwSize)
        WriteFile (lpcs->hFile, c, sizeof(c), &dwBW, NULL);
    }

    CloseHandle(lpcs->hFile);  // Close the "normal" open
   #endif

    // We can use non-buffered I/O if the ChunkGranularity is
    // a multiple of BytesPerSector.  Better check that wChunkGranularity
    // has indeed been set

    if (0 == lpcs->sCapParms.wChunkGranularity)
        lpcs->sCapParms.wChunkGranularity = lpcs->dwBytesPerSector;

    dwOpenFlags = FILE_ATTRIBUTE_NORMAL;
    lpcs->fUsingNonBufferedIO =
            (lpcs->sCapParms.wChunkGranularity >= lpcs->dwBytesPerSector) &&
           ((lpcs->sCapParms.wChunkGranularity % lpcs->dwBytesPerSector) == 0) &&
            (lpcs->CompVars.hic == NULL) &&
            (!(lpcs->fCaptureFlags & CAP_fStepCapturingNow)) &&
            (!(lpcs->fCaptureFlags & CAP_fFrameCapturingNow));

    AuxDebugEx (3, DEBUGLINE "fUsingNonBufferedIO=%d\r\n", lpcs->fUsingNonBufferedIO);

    // setup CreateFile flags based on whether we are using
    // non-buffered io and/or overlapped io
    //
    if (lpcs->fUsingNonBufferedIO)
    {
        dwOpenFlags |= FILE_FLAG_NO_BUFFERING;
#ifdef CHICAGO
    #define DOASYNCIO FALSE
    #pragma message (SQUAWK "find a better way to set AsyncIO flag")
#else
    #define DOASYNCIO TRUE
#endif
       #ifdef CHICAGO
        if (GetProfileIntA ("Avicap32", "AsyncIO", DOASYNCIO))
       #else
            if (!pfnCreateIoCompletionPort) {
                hmodKernel = LoadLibrary(TEXT("kernel32"));
                if (hmodKernel) {

#define IOCP (void *(__stdcall *)(void *,void *,unsigned long ,unsigned long ))
#define GQCS (int (__stdcall *)(void *,unsigned long *,unsigned long *,struct _OVERLAPPED ** ,unsigned long ))
                    pfnCreateIoCompletionPort = IOCP GetProcAddress(hmodKernel, "CreateIoCompletionPort");
                    pfnGetQueuedCompletionStatus = GQCS GetProcAddress(hmodKernel, "GetQueuedCompletionStatus");
                    if (!pfnCreateIoCompletionPort && !pfnGetQueuedCompletionStatus) {
                        pfnCreateIoCompletionPort = NULL;
                        pfnGetQueuedCompletionStatus = NULL;
                        FreeLibrary(hmodKernel);
                    }
                }
            }
            DPF("CreateIoCompletionPort @%x", pfnCreateIoCompletionPort);
            DPF("GetQueuedCompletionStatus @%x", pfnGetQueuedCompletionStatus);

            // give a way to override the async default option.
            if (!GetProfileIntA ("Avicap32", "AsyncIO",  DOASYNCIO)
              || !pfnCreateIoCompletionPort) {
		AuxDebugEx (2, DEBUGLINE "NOT doing Async IO\r\n");
	    } else
       #endif
        {
            AuxDebugEx (3, DEBUGLINE "Doing Async IO\r\n");
            dwOpenFlags |= FILE_FLAG_OVERLAPPED;

            // We are requested to do async io.  Allocate an array
            // of async io headers and initialize the async io fields
            // in the CAPSTREAM structure
            //
            {
                UINT iNumAsync = NUMELMS(lpcs->alpVideoHdr) + NUMELMS(lpcs->alpWaveHdr) + 2;
		// This is quite a lot of buffers.  Perhaps we should limit
		// ourselves to lpcs->iNumVideo and lpcs->iNumAudio EXCEPT
		// these fields have not yet been set up.  We would need
		// to look in the cap stream structure to get the information.
		// It is simpler to assume the maximum numbers.
		// Set the offset for the first write to the file
                lpcs->dwAsyncWriteOffset = lpcs->dwAVIHdrSize;
                lpcs->iNextAsync = lpcs->iLastAsync = 0;
                // Create the manual reset event for synchronous writing
                if ((lpcs->heSyncWrite = CreateEvent(NULL, TRUE, FALSE, NULL))
                  && (NULL != (lpcs->pAsync = (LPVOID)GlobalAllocPtr (GMEM_MOVEABLE | GMEM_ZEROINIT,
                                               sizeof(*lpcs->pAsync) * iNumAsync)))) {
                    lpcs->iNumAsync = iNumAsync;
                } else {
                    // cannot allocate the memory.  Go synchronous
                    dprintf("Failed to allocate async buffers");
                    if (lpcs->heSyncWrite) {
                        CloseHandle(lpcs->heSyncWrite);
                        lpcs->heSyncWrite = 0;
                    }
                    dwOpenFlags &= ~(FILE_FLAG_OVERLAPPED);
		}
            }
        }
    }

    // Open the capture file, using Non Buffered I/O
    // if possible, given sector size, and buffer granularity
    //
reopen:
    lpcs->hFile = CreateFile (lpcs->achFile,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_ALWAYS,
                    dwOpenFlags,
                    NULL);

    if (lpcs->hFile == INVALID_HANDLE_VALUE) {
        lpcs->hFile = 0;
        goto INIT_FILE_OPEN_ERROR;
    }

#ifdef ASYNCIO_PORT
    if (dwOpenFlags & FILE_FLAG_OVERLAPPED) {
        lpcs->hCompletionPort = pfnCreateIoCompletionPort(lpcs->hFile, NULL, (DWORD)1, 0);

        if (!lpcs->hCompletionPort) {
            // if we cannot create the completion port, write synchronously.
            dwOpenFlags &= ~FILE_FLAG_OVERLAPPED;
            CloseHandle(lpcs->hFile);
            GlobalFreePtr(lpcs->pAsync);
            lpcs->iNumAsync=0;
            if (lpcs->heSyncWrite) {
                CloseHandle(lpcs->heSyncWrite);
                lpcs->heSyncWrite = 0;
            }
            DPF("COULD NOT create the async completion port");
            goto reopen;
        } else {
            DPF("Created the async completion port");
        }
    }
#endif

    // BUGBUG, Remove the following line when done performance testing
   #ifdef JMK_HACK_DONTWRITE
    bDontWrite = GetProfileIntA("AVICAP32", "DontWrite", FALSE);
   #endif

    // Seek to a multiple of ChunkGranularity + AVIHEADERSIZE.
    // This is the point at which we'll start writing
    // Later, we'll come back and fill in the AVI header and index.

    // l is zero for standard wave and video formats
    l = (GetSizeOfWaveFormat ((LPWAVEFORMATEX) lpcs->lpWaveFormat) -
                sizeof (PCMWAVEFORMAT)) +
                (lpBitsInfoOut->bmiHeader.biSize -
                sizeof (BITMAPINFOHEADER));

    // (2K + size of wave and video stream headers) rounded to next 2K
    lpcs->dwAVIHdrSize = AVI_HEADERSIZE +
        (((lpcs->cbInfoChunks + l + lpcs->sCapParms.wChunkGranularity - 1)
        / lpcs->sCapParms.wChunkGranularity) * lpcs->sCapParms.wChunkGranularity);
    // we should assert that AVI_HEADERSIZE is a multiple of wChunkGranularity


    dprintf("AVIHdrSize = %ld", lpcs->dwAVIHdrSize);

    SetFilePointer (lpcs->hFile, lpcs->dwAVIHdrSize, NULL, FILE_BEGIN);
    if (lpcs->pAsync) {
        lpcs->dwAsyncWriteOffset = lpcs->dwAVIHdrSize;
    }

    // do all Index allocations
    if (!InitIndex (lpcs))
        CloseHandle (lpcs->hFile), lpcs->hFile = 0;

    lpcs->dwVideoChunkCount = 0;
    lpcs->dwWaveChunkCount = 0;

INIT_FILE_OPEN_ERROR:
    if (lpcs->hFile) {
	return(TRUE);
    }
    if (lpcs->pAsync) {
	GlobalFreePtr(lpcs->pAsync), lpcs->pAsync=NULL;
    }
    if (lpcs->heSyncWrite) {
        CloseHandle(lpcs->heSyncWrite);
        lpcs->heSyncWrite = 0;
    }
    return (FALSE);
}

///////////////////////////////////////////////////////////////////////////
//  The index array is used to record the positions
//  of every chunk in the RIFF (avi) file.
//
//  what this array is:
//
//      each entry contains the size of the data
//      high order bits encode the type of data (audio / video)
//      and whether the video chunk is a key frame, dropped frame, etc.
///////////////////////////////////////////////////////////////////////////

// Allocate the index table
// Returns: TRUE if index can be allocated
//
BOOL InitIndex (LPCAPSTREAM lpcs)
{
    lpcs->dwIndex = 0;

    // we assume that we have not already allocated an index
    //
    assert (lpcs->lpdwIndexStart == NULL);

    // Limit index size between 1 minute at 30fps and 3 hours at 30fps
    lpcs->sCapParms.dwIndexSize = max (lpcs->sCapParms.dwIndexSize, 1800);
    lpcs->sCapParms.dwIndexSize = min (lpcs->sCapParms.dwIndexSize, 324000L);
    dprintf("Max Index Size = %ld", lpcs->sCapParms.dwIndexSize);

    if (lpcs->hIndex = GlobalAlloc (GMEM_MOVEABLE,
                lpcs->sCapParms.dwIndexSize * sizeof (DWORD))) {
        if (lpcs->lpdwIndexEntry =
            lpcs->lpdwIndexStart = (LPDWORD)GlobalLock (lpcs->hIndex))
            return TRUE;        // Success

        GlobalFree (lpcs->hIndex);
	lpcs->hIndex = NULL;
    }
    lpcs->lpdwIndexStart = NULL;
    return FALSE;
}

// Deallocate the index table
//
void FiniIndex (LPCAPSTREAM lpcs)
{
    if (lpcs->hIndex) {
        if (lpcs->lpdwIndexStart)
            GlobalUnlock (lpcs->hIndex);
        GlobalFree (lpcs->hIndex);
	lpcs->hIndex = NULL;
    }
    lpcs->lpdwIndexStart = NULL;
}



// Write out the index at the end of the capture file.
// The single frame capture methods do not append
// JunkChunks!  Audio chunks also now may have junk appended.
//
BOOL WriteIndex (LPCAPSTREAM lpcs, BOOL fJunkChunkWritten)
{
    BOOL  fChunkIsAudio;
    BOOL  fChunkIsKeyFrame;
    BOOL  fChunkIsDummy;
    BOOL  fChunkIsGranular;
    DWORD dwIndex;
    DWORD dw;
    DWORD dwJunk;
    DWORD off;
    AVIINDEXENTRY   avii;
    MMCKINFO    ck;
    LPDWORD lpdw;
    DWORD   dwGran;

    // determine which granularity (if any) to use
    // when calculating junk appended
    //
    dwGran = 0;
    if (fJunkChunkWritten)
    {
        dwGran = lpcs->sCapParms.wChunkGranularity;
        if (lpcs->fUsingNonBufferedIO)
           dwGran = max (lpcs->dwBytesPerSector, dwGran);
    }


    if (lpcs->dwIndex > lpcs->sCapParms.dwIndexSize)
        return TRUE;

    off        = lpcs->dwAVIHdrSize;

    ck.cksize  = 0;
    ck.ckid    = ckidAVINEWINDEX;
    ck.fccType = 0;

    if (mmioCreateChunk(lpcs->hmmio,&ck,0)) {
	dprintf("Failed to create chunk for index");
        return FALSE;
    }

    lpdw = lpcs->lpdwIndexStart;
    for (dwIndex= 0; dwIndex < lpcs->dwIndex; dwIndex++) {

        dw = *lpdw++;

        fChunkIsAudio      = (BOOL) ((dw & IS_AUDIO_CHUNK) != 0);
        fChunkIsKeyFrame   = (BOOL) ((dw & IS_KEYFRAME_CHUNK) != 0);
        fChunkIsDummy      = (BOOL) ((dw & IS_DUMMY_CHUNK) != 0);
        fChunkIsGranular   = (BOOL) ((dw & IS_GRANULAR_CHUNK) != 0);
        dw &= ~(INDEX_MASK);

        if (fChunkIsAudio) {
            avii.ckid         = MAKEAVICKID(cktypeWAVEbytes, 1);
            avii.dwFlags      = 0;
        } else {
            avii.ckid         = MAKEAVICKID(cktypeDIBbits, 0);
            if (lpcs->lpBitsInfo->bmiHeader.biCompression == BI_RLE8)
                avii.ckid         = MAKEAVICKID(cktypeDIBcompressed, 0);
            avii.dwFlags      = fChunkIsKeyFrame ? AVIIF_KEYFRAME : 0;
        }
        avii.dwChunkLength    = dw;
        avii.dwChunkOffset    = off;

        if (mmioWrite(lpcs->hmmio, (LPVOID)&avii, sizeof(avii)) != sizeof(avii)) {
	    dprintf("Failed to write index chunk %d", dwIndex);
            return FALSE;
	}

        dw += sizeof (RIFF);
        // round to word boundary
        //
        dw += (dw & 1);
        off += dw;

        // If a Junk chunk was appended, move past it
        //
        if (fChunkIsGranular && dwGran && (off % dwGran)) {
            dwJunk = dwGran - (off % dwGran);

            if (dwJunk < sizeof (RIFF))
                off += dwGran;
            off += dwJunk;
        }
    }

    if (mmioAscend(lpcs->hmmio, &ck, 0)){
	dprintf("Failed to ascend at end of index writing");
        return FALSE;
    }

    return TRUE;
}

/*
 * AVIFileFini
 *
 *       Write out the index, deallocate the index, and close the file.
 *
 */
BOOL AVIFileFini (LPCAPSTREAM lpcs, BOOL fWroteJunkChunks, BOOL fAbort)
{
    MMCKINFO      ckRiff;
    MMCKINFO      ckList;
    MMCKINFO      ckStream;
    MMCKINFO      ck;
    UINT          ii;
    DWORD         dw;
    AVIStreamHeader        strhdr;
    DWORD         dwDataEnd;
    BOOL          fRet = TRUE;
    RGBQUAD       argbq[256];
    MainAVIHeader aviHdr;
    BOOL          fSound;
    LPBITMAPINFO  lpBitsInfoOut;    // Possibly compressed output format

    // No special video format given -- use the default
    //
    lpBitsInfoOut = lpcs->lpBitsInfo;
   #ifdef NEW_COMPMAN
    if (lpcs->CompVars.hic != NULL)
        lpBitsInfoOut = lpcs->CompVars.lpbiOut;
   #endif

    // if the capture file has not been opened, we have nothing to do
    //
    if (lpcs->hFile == 0)
        return FALSE;

    // save off the current seek position.  this is the end of the capture
    // data.  then close the capture file,  we will do the final work
    // on the capture file using mmio & buffered io.
    //
    if (lpcs->pAsync)
        dwDataEnd = lpcs->dwAsyncWriteOffset;
    else
        dwDataEnd = SetFilePointer (lpcs->hFile, 0, NULL, FILE_CURRENT);

    CloseHandle (lpcs->hFile), lpcs->hFile = 0;

    // if we had allocated space for async buffers, free them now
    //
    if (lpcs->pAsync)
    {
        GlobalFreePtr (lpcs->pAsync);
        lpcs->pAsync = NULL;
        lpcs->iNextAsync = lpcs->iLastAsync = lpcs->iNumAsync = 0;
    }

    // if we are aborting capture, we are done
    lpcs->hmmio = mmioOpen(lpcs->achFile, NULL, MMIO_WRITE);
    assert (lpcs->hmmio != NULL);

    //
    if (fAbort)
        goto FileError;

    if (!lpcs->dwWaveBytes)
        fSound = FALSE;
    else
        fSound = lpcs->sCapParms.fCaptureAudio && (!(lpcs->fCaptureFlags & CAP_fFrameCapturingNow));

    // Seek to beginning of file, so we can write the header.
    mmioSeek(lpcs->hmmio, 0, SEEK_SET);

    DSTATUS(lpcs, "Writing AVI header");

    // Create RIFF/AVI chunk
    ckRiff.cksize = 0;
    ckRiff.fccType = formtypeAVI;
    if (mmioCreateChunk(lpcs->hmmio,&ckRiff,MMIO_CREATERIFF))
         goto FileError;

    // Create header list
    ckList.cksize = 0;
    ckList.fccType = listtypeAVIHEADER;
    if (mmioCreateChunk(lpcs->hmmio,&ckList,MMIO_CREATELIST))
         goto FileError;

    // Create AVI header chunk
    ck.cksize = sizeof(MainAVIHeader);
    ck.ckid = ckidAVIMAINHDR;
    if (mmioCreateChunk(lpcs->hmmio,&ck,0))
         goto FileError;

    lpcs->dwAVIHdrPos = ck.dwDataOffset;

    // Calculate AVI header info
    //
    ZeroMemory (&aviHdr, sizeof(aviHdr));

    //
    // Set the stream lengths based on the Master stream
    //
   #if 0 // stream length calc with unconditional audio master
    aviHdr.dwMicroSecPerFrame = lpcs->sCapParms.dwRequestMicroSecPerFrame;
    if (fSound && lpcs->dwVideoChunkCount) {
         /* HACK HACK */
         /* Set rate that was captured based on length of audio data */

         aviHdr.dwMicroSecPerFrame = (DWORD) MulDiv ((LONG)lpcs->dwWaveBytes,
                   1000000,
                   (LONG)(lpcs->lpWaveFormat->nAvgBytesPerSec * lpcs->dwVideoChunkCount));
    }
   #else

    // Init a value in case we're not capturing audio
    aviHdr.dwMicroSecPerFrame = lpcs->sCapParms.dwRequestMicroSecPerFrame;

    switch (lpcs->sCapParms.AVStreamMaster) {
        case AVSTREAMMASTER_NONE:
            break;

        case AVSTREAMMASTER_AUDIO:
        default:
            // VFW 1.0 and 1.1 ALWAYS munged frame rate to match audio
            // duration.
            if (fSound && lpcs->sCapParms.fCaptureAudio && lpcs->dwVideoChunkCount) {
                // Modify the video framerate based on audio duration
                aviHdr.dwMicroSecPerFrame = (DWORD)
                    ((double)lpcs->dwWaveBytes * 1000000. /
                    ((double)lpcs->lpWaveFormat->nAvgBytesPerSec *
                    lpcs->dwVideoChunkCount + 0.5));
            }
            break;
    }
   #endif
    lpcs->dwActualMicroSecPerFrame = aviHdr.dwMicroSecPerFrame;

    aviHdr.dwMaxBytesPerSec = (DWORD) MulDiv (lpBitsInfoOut->bmiHeader.biSizeImage,
                                      1000000,
                                      lpcs->sCapParms.dwRequestMicroSecPerFrame) +
                                      (fSound ? lpcs->lpWaveFormat->nAvgBytesPerSec : 0);
    aviHdr.dwPaddingGranularity = 0L;
    aviHdr.dwFlags = AVIF_WASCAPTUREFILE | AVIF_HASINDEX;
    aviHdr.dwStreams = fSound ? 2 : 1;
    aviHdr.dwTotalFrames = lpcs->dwVideoChunkCount;
    aviHdr.dwInitialFrames = 0L;
    aviHdr.dwSuggestedBufferSize = 0L;
    aviHdr.dwWidth = lpBitsInfoOut->bmiHeader.biWidth;
    aviHdr.dwHeight = lpBitsInfoOut->bmiHeader.biHeight;

#if 0 // unnecessary due to the ZeroMemory call above
    aviHdr.dwReserved[0] = 0;
    aviHdr.dwReserved[1] = 0;
    aviHdr.dwReserved[2] = 0;
    aviHdr.dwReserved[3] = 0;
#endif
    //aviHdr.dwRate = 1000000L;
    //aviHdr.dwScale = aviHdr.dwMicroSecPerFrame;
    //aviHdr.dwStart = 0L;
    //aviHdr.dwLength = lpcs->dwVideoChunkCount;

    // Write AVI header info
    if (mmioWrite(lpcs->hmmio, (LPBYTE)&aviHdr, sizeof(aviHdr)) != sizeof(aviHdr) ||
        mmioAscend(lpcs->hmmio, &ck, 0))
        goto FileError;

    DSTATUS(lpcs, "Writing AVI Stream header");

    // Create stream header list
    ckStream.cksize = 0;
    ckStream.fccType = listtypeSTREAMHEADER;
    if (mmioCreateChunk(lpcs->hmmio,&ckStream,MMIO_CREATELIST))
        goto FileError;

    ZeroMemory (&strhdr, sizeof(strhdr));
    strhdr.fccType = streamtypeVIDEO;
    strhdr.fccHandler = lpBitsInfoOut->bmiHeader.biCompression;
   #ifdef NEW_COMPMAN
    if (lpcs->CompVars.hic)
        strhdr.fccHandler = lpcs->CompVars.fccHandler;
   #endif

    // A bit of history...
    // In VFW 1.0, we set fccHandler to 0 for BI_RLE8 formats
    // as a kludge to make Mplayer and Videdit play the files.
    // Just prior to 1.1 release, we found this broke Premiere,
    // so now (after AVICAP beta is on Compuserve), we change the
    // fccHandler to "MRLE".  Just ask Todd...
    // And now, at RC1, we change it again to "RLE ", Just ask Todd...
    if (strhdr.fccHandler == BI_RLE8)
        strhdr.fccHandler = mmioFOURCC('R', 'L', 'E', ' ');

    //strhdr.dwFlags = 0L;
   #ifdef NEW_COMPMAN
    //strhdr.wPriority = 0L;
    //strhdr.wLanguage = 0L;
   #else
    //strhdr.dwPriority = 0L;
   #endif

    //strhdr.dwInitialFrames = 0L;
    strhdr.dwScale = aviHdr.dwMicroSecPerFrame;
    strhdr.dwRate = 1000000L;
    //strhdr.dwStart = 0L;
    strhdr.dwLength = lpcs->dwVideoChunkCount;        /* Needs to get filled in! */
    strhdr.dwQuality = (DWORD) -1L;         /* !!! ICQUALITY_DEFAULT */
    //strhdr.dwSampleSize = 0L;

    //
    // Write stream header data
    //
    ck.ckid = ckidSTREAMHEADER;
    if (mmioCreateChunk(lpcs->hmmio,&ck,0) ||
        mmioWrite(lpcs->hmmio, (LPBYTE)&strhdr, sizeof(strhdr)) != sizeof(strhdr) ||
        mmioAscend(lpcs->hmmio, &ck, 0))
        goto FileError;

    /*
    **  !!! dont write palette for full color?
    */
    if (lpBitsInfoOut->bmiHeader.biBitCount > 8)
        lpBitsInfoOut->bmiHeader.biClrUsed = 0;

    /* Create DIB header chunk */
    ck.cksize = lpBitsInfoOut->bmiHeader.biSize +
                           lpBitsInfoOut->bmiHeader.biClrUsed *
                           sizeof(RGBQUAD);
    ck.ckid = ckidSTREAMFORMAT;
    if (mmioCreateChunk(lpcs->hmmio,&ck,0))
         goto FileError;

    /* Write DIB header data */
    if (mmioWrite(lpcs->hmmio, (LPBYTE)&lpBitsInfoOut->bmiHeader,
                               lpBitsInfoOut->bmiHeader.biSize) !=
             (LONG) lpBitsInfoOut->bmiHeader.biSize)
         goto FileError;

    if (lpBitsInfoOut->bmiHeader.biClrUsed > 0) {
        // Get Palette info
        if ((ii = GetPaletteEntries(lpcs->hPalCurrent, 0,
                                (UINT) lpBitsInfoOut->bmiHeader.biClrUsed,
                                (LPPALETTEENTRY) argbq)) !=
                    (UINT)lpBitsInfoOut->bmiHeader.biClrUsed)
            goto FileError;

	// Reorder the palette from PALETTEENTRY order to RGBQUAD order
	// by swapping the red and blue palette entries.
        //for (ii = 0; ii < lpBitsInfoOut->bmiHeader.biClrUsed; ++ii)
        while (ii--)
            SWAPTYPE(argbq[ii].rgbRed, argbq[ii].rgbBlue, BYTE);


        // Write Palette Info
        dw = sizeof(RGBQUAD) * lpBitsInfoOut->bmiHeader.biClrUsed;
        if (mmioWrite(lpcs->hmmio, (LPBYTE)argbq, dw) != (long)dw)
            goto FileError;
    }

    if (mmioAscend(lpcs->hmmio, &ck, 0))
         goto FileError;

    // ADD FOURCC stuff here!!! for Video stream

    // Ascend out of stream header
    if (mmioAscend(lpcs->hmmio, &ckStream, 0))
         goto FileError;

    /* If sound is enabled, then write WAVE header */
    if (fSound) {

         /* Create stream header list */
         ckStream.cksize = 0;
         ckStream.fccType = listtypeSTREAMHEADER;
         if (mmioCreateChunk(lpcs->hmmio,&ckStream,MMIO_CREATELIST))
             goto FileError;

         ZeroMemory (&strhdr, sizeof(strhdr));
         strhdr.fccType = streamtypeAUDIO;
         strhdr.fccHandler = 0L;
         strhdr.dwFlags = 0L;
        #ifdef NEW_COMPMAN
         strhdr.wPriority = 0L;
         strhdr.wLanguage = 0L;
        #else
         strhdr.dwPriority  = 0L;
        #endif
         strhdr.dwInitialFrames = 0L;
         strhdr.dwScale = lpcs->lpWaveFormat->nBlockAlign;
         strhdr.dwRate = lpcs->lpWaveFormat->nAvgBytesPerSec;
         strhdr.dwStart = 0L;
         strhdr.dwLength =  lpcs->dwWaveBytes /
                        lpcs->lpWaveFormat->nBlockAlign;
         strhdr.dwQuality = (DWORD)-1L;    /* !!! ICQUALITY_DEFAULT */
         strhdr.dwSampleSize = lpcs->lpWaveFormat->nBlockAlign;

         ck.ckid = ckidSTREAMHEADER;
         if (mmioCreateChunk(lpcs->hmmio,&ck,0) ||
             mmioWrite(lpcs->hmmio, (LPBYTE)&strhdr, sizeof(strhdr)) != sizeof(strhdr) ||
             mmioAscend(lpcs->hmmio, &ck, 0))
             goto FileError;

         ck.cksize = (LONG) GetSizeOfWaveFormat ((LPWAVEFORMATEX) lpcs->lpWaveFormat);
         ck.ckid = ckidSTREAMFORMAT;
         if (mmioCreateChunk(lpcs->hmmio,&ck,0) ||
             mmioWrite(lpcs->hmmio, (LPBYTE)lpcs->lpWaveFormat, ck.cksize) != (LONG) ck.cksize ||
             mmioAscend(lpcs->hmmio, &ck, 0))
             goto FileError;

         /* Ascend out of stream header */
         if (mmioAscend(lpcs->hmmio, &ckStream, 0))
             goto FileError;
    }

    // ADD FOURCC stuff here!!! for entire file
    DSTATUS(lpcs, "Writing Info chunks");
    if (lpcs->lpInfoChunks) {
        DSTATUS(lpcs, "Writing Info chunks");
        if (mmioWrite (lpcs->hmmio, lpcs->lpInfoChunks, lpcs->cbInfoChunks) !=
                lpcs->cbInfoChunks)
            goto FileError;
    }

    /* ascend from the Header list */
    if (mmioAscend(lpcs->hmmio, &ckList, 0))
         goto FileError;


    ck.ckid = ckidAVIPADDING;
    if (mmioCreateChunk(lpcs->hmmio,&ck,0))
         goto FileError;

    // The data must begin at offset lpcs->dwAVIHdrSize.
    // To create a valid RIFF file we must write the LIST/AVI chunk before
    // this point.  Hence we end the junk section at the end of the header
    // leaving room for the LIST chunk header.
    mmioSeek(lpcs->hmmio, lpcs->dwAVIHdrSize - 3 * sizeof(DWORD), SEEK_SET);

    if (mmioAscend(lpcs->hmmio, &ck, 0))
         goto FileError;

    DSTATUS(lpcs, "Writing Movie LIST");

    /* Start the movi list */
    ckList.cksize = 0;
    ckList.fccType = listtypeAVIMOVIE;
    if (mmioCreateChunk(lpcs->hmmio,&ckList,MMIO_CREATELIST))
         goto FileError;

    // Force the chunk to end on the next word boundary
    dprintf("IndexStartOffset = %8X\n", dwDataEnd);
    mmioSeek(lpcs->hmmio, dwDataEnd + (dwDataEnd & 1L), SEEK_SET);

    /* Ascend out of the movi list and the RIFF chunk so that */
    /* the sizes can be fixed */
    mmioAscend(lpcs->hmmio, &ckList, 0);

    /*
    ** Now write index out!
    */
    DSTATUS(lpcs, "Writing Index...");
    WriteIndex(lpcs, fWroteJunkChunks);

    lpcs->fFileCaptured = TRUE;     // we got a good file, allow editing of it
    goto Success;

FileError:
    lpcs->fFileCaptured = fRet = FALSE;      // bogus file - no editing allowed

Success:
    DSTATUS(lpcs, "Freeing Index...");
    FiniIndex (lpcs);
    mmioAscend(lpcs->hmmio, &ckRiff, 0);

    mmioSeek(lpcs->hmmio, 0, SEEK_END);

    mmioFlush(lpcs->hmmio, 0);

    // Close the file
    mmioClose(lpcs->hmmio, 0);
    lpcs->hmmio = NULL;

    return fRet;
}

//
// Prepends dummy frame entries to the current valid video frame.
// Bumps the index, but does not actually trigger a write operation.
// nCount is a count of the number of frames to write
// Returns: TRUE on a successful write

BOOL WINAPI AVIWriteDummyFrames (
    LPCAPSTREAM lpcs,
    UINT        nCount,
    LPUINT      lpuError,
    LPBOOL      lpbPending)
{
    DWORD  dwBytesToWrite;
    DWORD  dwType;
    LPRIFF priff;
    UINT   jj;

    *lpbPending = FALSE;
    *lpuError = 0;
    if ( ! nCount)
        return TRUE;

    // create a buffer full of dummy chunks to act as placeholders
    // for the dropped frames
    //
    dwType = MAKEAVICKID(cktypeDIBbits, 0);
    if (lpcs->lpBitsInfo->bmiHeader.biCompression == BI_RLE8)
        dwType = MAKEAVICKID(cktypeDIBcompressed, 0);

    // dont try to write more than 1 'sector' worth of dummy
    // frames
    //
    dwBytesToWrite = nCount * sizeof(RIFF);
    if (dwBytesToWrite > lpcs->dwBytesPerSector)
    {
#ifdef DEBUG
	UINT n = nCount;
#endif
        dwBytesToWrite = lpcs->dwBytesPerSector;
#ifdef DEBUG
        nCount = dwBytesToWrite / sizeof(RIFF);
	assert(nCount*sizeof(RIFF) == dwBytesToWrite);
	dprintf("Forced to reduce dummy frames from %d to %d", n, nCount);
#endif
    }

    // create index entries for the dummy chunks
    //
    for (jj = 0; jj < nCount-1; ++jj)
        IndexVideo (lpcs, IS_DUMMY_CHUNK, FALSE);
    IndexVideo (lpcs, IS_DUMMY_CHUNK | IS_GRANULAR_CHUNK, FALSE);

    // fill in the drop frame buffer with dummy frames
    //
    priff = (LPRIFF)lpcs->lpDropFrame;
    for (jj = 0; jj < nCount; ++jj, ++priff)
    {
        priff->dwSize  = 0;
        priff->dwType  = dwType;
    }

    //
    // cant use a single dummy frame buffer when we are doing async
    // write because we cant write 'n' dummy frames to the buffer
    // if it is currently already queued to an IO.
    //
    // perhaps several dummy frames?  1 frame, 2 frames, 3 frames, etc
    // create dynamically?
    //

    // write out the dummy frames
    //
    AuxDebugEx (3, DEBUGLINE "DummyFrames  Count=%d, ToWrite=%d\r\n",
                nCount, dwBytesToWrite);

    *lpuError = AVIWrite (lpcs,
                          lpcs->lpDropFrame,
                          dwBytesToWrite,
                          (UINT)-1,  // force sync completion
                          ASYNC_BUF_DROP,
                          lpbPending);
    return !(*lpuError);
}

// Writes compressed or uncompressed frames to the AVI file
// returns TRUE if no error, FALSE if end of file.
//
BOOL WINAPI AVIWriteVideoFrame (
    LPCAPSTREAM lpcs,
    LPBYTE      lpData,
    DWORD       dwBytesUsed,
    BOOL        fKeyFrame,
    UINT        uIndex,
    UINT        nDropped,
    LPUINT      lpuError,
    LPBOOL      lpbPending)
{
    DWORD  dwBytesToWrite;
    LPRIFF priff;

    *lpuError = 0;
    *lpbPending = FALSE;
    if (!IndexVideo (lpcs,
                dwBytesUsed | (nDropped ? 0 : IS_GRANULAR_CHUNK),
                fKeyFrame))
        return FALSE;

    // adjust the size field of the RIFF chunk that preceeds the
    // data to be written
    //
    priff = ((LPRIFF)lpData)-1;
    priff->dwSize = dwBytesUsed;
    dwBytesUsed += dwBytesUsed & 1;
    dwBytesToWrite = dwBytesUsed + sizeof(RIFF);

    if (nDropped)
    {
        UINT  jj;
        DWORD dwType;

        // determine the 'type' of the dummy chunks
        //
        dwType = MAKEAVICKID(cktypeDIBbits, 0);
        if (lpcs->lpBitsInfo->bmiHeader.biCompression == BI_RLE8)
            dwType = MAKEAVICKID(cktypeDIBcompressed, 0);

        // dont try to write more than 1 'sector' worth of dummy
        // frames
        //
        if (nDropped > (lpcs->dwBytesPerSector / sizeof(RIFF)))
            nDropped = lpcs->dwBytesPerSector / sizeof(RIFF);

        // create index entries for the dummy chunks
        //
        for (jj = 0; jj < nDropped-1; ++jj)
            IndexVideo (lpcs, IS_DUMMY_CHUNK, FALSE);

        IndexVideo (lpcs, IS_DUMMY_CHUNK | IS_GRANULAR_CHUNK, FALSE);

        // fill in the drop frame buffer with dummy frames
        //
        priff = (LPRIFF)(lpData + dwBytesToWrite - sizeof(RIFF));
        for (jj = 0; jj < nDropped; ++jj, ++priff)
        {
            priff->dwSize  = 0;
            priff->dwType  = dwType;
        }
        dwBytesToWrite += nDropped * sizeof(RIFF);
    }

    // AviWrite will write the data and create any trailing junk
    // that is necessary
    //

    // write out the chunk, video data, and possibly the junk chunk
    //
    AuxDebugEx (3, DEBUGLINE "Calling AVIWrite - Video=%8x dw=%8x\r\n",
                (LPBYTE)lpData - sizeof(RIFF), dwBytesToWrite);

    *lpuError = AVIWrite (lpcs,
                          (LPBYTE)lpData - sizeof(RIFF),
                          dwBytesToWrite,
                          uIndex,
                          ASYNC_BUF_VIDEO,
                          lpbPending);
    return !(*lpuError);
}

// New for Chicago, align audio buffers on wChunkGranularity boundaries!
//
BOOL WINAPI AVIWriteAudio (
    LPCAPSTREAM lpcs,
    LPWAVEHDR   lpwh,
    UINT        uIndex,
    LPUINT      lpuError,
    LPBOOL      lpbPending)
{
    DWORD  dwBytesToWrite;
    LPRIFF priff;

    *lpuError = 0;
    *lpbPending = FALSE;

    // change the dwSize field in the RIFF chunk
    priff = ((LPRIFF)lpwh->lpData) -1;
    priff->dwSize = lpwh->dwBytesRecorded;

    if ( ! IndexAudio (lpcs, lpwh->dwBytesRecorded | IS_GRANULAR_CHUNK))
        return FALSE;

    // update total bytes of wave audio recorded
    //
    lpcs->dwWaveBytes += lpwh->dwBytesRecorded;

    // pad the data to be written to a WORD (16 bit) boundary
    //
    lpwh->dwBytesRecorded += lpwh->dwBytesRecorded & 1;
    dwBytesToWrite = lpwh->dwBytesRecorded + sizeof(RIFF);

    // write out the chunk, audio data, and possibly the junk chunk
    AuxDebugEx (3, DEBUGLINE "Audio=%8x dw=%8x\r\n",
                lpwh->lpData - sizeof(RIFF), dwBytesToWrite);
    *lpuError = AVIWrite (lpcs,
                          lpwh->lpData - sizeof(RIFF),
                          dwBytesToWrite,
                          uIndex,
                          ASYNC_BUF_AUDIO,
                          lpbPending);
    return !(*lpuError);
}
    #endif  //---------------- USE_AVIFILE ----------------------------
