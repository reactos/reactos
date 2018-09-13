
//
// unbuffered rapid disk i/o class. Provides streaming write to disk using
// unbuffered, overlapped i/o via large buffers. Inter-thread sync
// must be provided elsewhere.
//

#ifndef _DIRECTIO_H_
#define _DIRECTIO_H_

#ifdef _WIN32
  #ifdef CHICAGO
  #include "disk32.h"
  #endif

// all 'tunable' constants now found in CFileStream::EnsureBuffersValid()

// maximum number of buffers that can be requested
#define NR_OF_BUFFERS   	4

// min read size
#define MIN_READ_SIZE   (12 * 1024)

// --- we use these internally ----
// unbuffered i/o handler class. requires copy of data.
// will round reads and writes to correct sector size, and
// will pre-read if start location of read or write is not in buffer.
// read or write will terminate early if insufficient space or data.
// writes must be explicitly initiated from the buffer to disk
// by calling commit. reads will be initiated by the Read function, or may
// be initiated offline using ReadAhead.

class CFileBuffer {

public:
    // initiate to an invalid (no buffer ready) state
    CFileBuffer();

    // allocate memory and become idle.
   #ifdef CHICAGO
    BOOL Init(DWORD nBytesPerSector, DWORD buffersize, LPQIO pqio);
   #else
    BOOL Init(DWORD nBytesPerSector, DWORD buffersize, HANDLE hfile);
   #endif

    // revert to invalid state (eg when streaming stops)
    void FreeMemory();


    // write some data to buffer (must be committed separately)
    // filesize parameter is the current file size before this write
    // (used to control reading of partial sectors).
    BOOL Write(DWORD pos, LPBYTE pData, DWORD count, DWORD filesize,
            DWORD * pBytesWritten);

    // read data from buffer (will seek and read if necessary first)
    BOOL Read(DWORD pos, LPBYTE pData, DWORD count,
                DWORD filelength, DWORD * pBytesRead);

    // does this position occur anywhere within the current buffer ?
    // needs to know current eof for some cases (writing beyond eof
    // if eof is within this buffer is ok to this buffer).
    BOOL QueryPosition(DWORD pos, DWORD filesize);

    // what is the first file position after this buffer's valid data
    DWORD GetNextPosition();

    // initiate a read-ahead
    void ReadAhead(DWORD start, DWORD filelength);


    // initiate the i/o from the buffer
    BOOL Commit();

    // wait for any pending commit to complete
    BOOL WaitComplete();

    // is the buffer idle - FALSE if currently busy or invalid
    BOOL IsIdle() {
        return (m_State == Idle);
    };

    // calls commit if dirty before freeing everything.
    ~CFileBuffer();

private:

    // non-blocking check to see if pending i/o is complete and ok
    BOOL CheckComplete();

    BOOL ReadIntoBuffer(int offset, DWORD pos, DWORD count);

    DWORD_PTR RoundPosToSector(DWORD_PTR pos)
    {
        // positions round down to the previous sector start
        return (pos / m_BytesPerSector) * m_BytesPerSector;
    };

    DWORD_PTR RoundSizeToSector(DWORD_PTR size)
    {
        // sizes round up to total sector count
        return ((size + m_BytesPerSector - 1) / m_BytesPerSector)
                    * m_BytesPerSector;
    }


    // buffer states
    enum BufferState { Idle, Busy, Invalid, ErrorOccurred };

    BufferState m_State;
    BOOL        m_bDirty;
    LPBYTE      m_pBuffer;	// buffer with start addr rounded
    LPBYTE	m_pAllocedMem;	// buffer before rounding
    DWORD       m_TotalSize;        // allocated buffer size
    DWORD       m_DataLength;       // bytes of valid data in buffer
    DWORD       m_Position;         // file position of start of buffer
    DWORD       m_BytesPerSector;   // sector boundaries are important
    DWORD	m_FileLength;	    // actual file size (not rounded)


   #ifdef CHICAGO
    QIOBUF      m_qiobuf;
    LPQIO       m_pqio;
   #else
    OVERLAPPED  m_Overlapped;
    HANDLE      m_hFile;
   #endif
};



class CFileStream {

public:
    CFileStream();         // does not do much (cannot return error)

    BOOL Open(LPTSTR file, BOOL bWrite, BOOL bTruncate);

    BOOL Seek(DWORD pos);

    BOOL Write(LPBYTE pData, DWORD count, DWORD * pbyteswritten);

    BOOL Read(LPBYTE pData, DWORD count, DWORD * pbytesread);

    DWORD GetCurrentPosition();

    BOOL StartStreaming();		// default (write if opened for write)
    BOOL StartWriteStreaming();
    BOOL StartReadStreaming();
    BOOL StopStreaming();

    // wait for all transfers to complete.
    BOOL CommitAndWait();

    // destructor will call Commit()
    ~CFileStream();

private:

    // enable extra buffers for streaming
    BOOL EnsureBuffersValid();

    // advance to next buffer
    int NextBuffer(int i) {
        return (i + 1) % m_NrValid;
    };


    // unbuffered i/o is only allowed in multiples of this
    DWORD m_SectorSize;

    CFileBuffer m_Buffers[NR_OF_BUFFERS];


    // how many buffers are valid ?
    int m_NrValid;

    // which is the current buffer
    int m_Current;

    // which buffer has the highest position - we will issue the
    // readahead when we start using this buffer
    int m_HighestBuffer;

    // next read/write position within file
    DWORD m_Position;

    enum StreamingState { Invalid, Stopped, Reading, Writing };
    StreamingState m_State;

    DWORD m_Size;   // current file size

    // file handle
   #ifdef CHICAGO
    QIO         m_qio;
    #define     m_hFile m_qio.hFile
   #else
    HANDLE      m_hFile;
   #endif

    // if opened for writing, then default streaming mode is write
    BOOL m_bWrite;


};



#endif //_WIN32


#endif  // _DIRECTIO_H_



