/****************************************************************************
 *
 *  DIRECTIO.CPP
 *
 *  routines for reading Standard AVI files
 *
 *  Copyright (c) 1992 - 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *
 * implementation of a disk i/o class designed to optimise
 * sequential reading and writing to disk by using overlapped i/o (for read
 * ahead and write behind) and using large buffers written with no buffering.
 *
 ***************************************************************************/
#include <windows.h>
#include <win32.h>
#include "debug.h"

#include "directio.h"

#ifdef USE_DIRECTIO

//
// implementation of a disk i/o class designed to optimise
// sequential reading and writing to disk by using overlapped i/o (for read
// ahead and write behind) and using large buffers written with no buffering.



// -- CFileStream class methods ---------------------------------------


// initialise to known (invalid) state
CFileStream::CFileStream()
{
        m_State = Invalid;
        m_Position = 0;
        m_hFile = INVALID_HANDLE_VALUE;
#ifdef CHICAGO
        ZeroMemory(&m_qio, sizeof(m_qio));
#endif
}


BOOL
CFileStream::Open(LPTSTR file, BOOL bWrite, BOOL bTruncate)
{
    if (m_State != Invalid) {
        return FALSE;
    }


    // remember this for default streaming mode
    m_bWrite = bWrite;

    DWORD dwAccess = GENERIC_READ;
    if (bWrite) {
        dwAccess |= GENERIC_WRITE;
    }


    // open the file. Always get read access. exclusive open if we
    // are writing the file, otherwise deny other write opens.

    // never truncate the file, since the file may be de-fragmented.

   #ifdef CHICAGO
    DWORD dwFlags = FILE_FLAG_NO_BUFFERING;
   #else
    DWORD dwFlags = FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING;
   #endif

    m_hFile = CreateFile(file,
                dwAccess,
                (bWrite ? 0 : FILE_SHARE_READ),
                NULL,
                OPEN_ALWAYS,
                dwFlags,
                0);

    if (m_hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

   #ifdef CHICAGO
    if ( ! QioInitialize(&m_qio, m_hFile, THREAD_PRIORITY_HIGHEST)) {
        CloseHandle (m_hFile);
        return FALSE;
    }
   #endif

    // find the bytes per sector that we have to round to for this file
    // -requires finding the 'root path' for this file.
    TCHAR ch[MAX_PATH];
    LPTSTR ptmp;    //required arg

    GetFullPathName(file, sizeof(ch)/sizeof(ch[0]), ch, &ptmp);

    // truncate this to the name of the root directory
    if ((ch[0] == TEXT('\\')) && (ch[1] == TEXT('\\'))) {

        // path begins with  \\server\share\path so skip the first
        // three backslashes
        ptmp = &ch[2];
        while (*ptmp && (*ptmp != TEXT('\\'))) {
            ptmp++;
        }
        if (*ptmp) {
            // advance past the third backslash
            ptmp++;
        }
    } else {
        // path must be drv:\path
        ptmp = ch;
    }

    // find next backslash and put a null after it
    while (*ptmp && (*ptmp != TEXT('\\'))) {
        ptmp++;
    }
    // found a backslash ?
    if (*ptmp) {
        // skip it and insert null
        ptmp++;
        *ptmp = TEXT('\0');
    }

    DWORD dwtmp1, dwtmp2, dwtmp3;
    if (!GetDiskFreeSpace(ch,
        	&dwtmp1,
        	&m_SectorSize,
        	&dwtmp2,
        	&dwtmp3))
	m_SectorSize = 2048;

    // sigh. now init the first buffer

    // sets the right buffer count and size for current mode
    m_State = Stopped;
    if (!EnsureBuffersValid()) {
        return FALSE;
    }
    m_Current = 0;
    m_Position = 0;

    // if asked to truncate the file, we will not actually do so, since this
    // could throw away a de-fragged file. We will however, note that the file
    // size is 0 and use this to affect reading and writing past 'eof' - eg
    // if you write 8 bytes to the beginning of a truncated file, we do not
    // need to read in the first sector beforehand.
    if (bTruncate) {
	m_Size = 0;
    } else {
	// get the current file size
	m_Size = GetFileSize(m_hFile, NULL);
    }

    // all set
    return TRUE;
}




BOOL
CFileStream::Seek(DWORD pos)
{
    // we just record this and go away
    //if (pos < m_Position) {
    //    DPF("seek back by 0x%x to 0x%x\n", m_Position - pos, pos);
    //}

    m_Position  = pos;

    return TRUE;
}

DWORD
CFileStream::GetCurrentPosition()
{
    return m_Position;
}

BOOL
CFileStream::Write(LPBYTE pData, DWORD count, DWORD * pbyteswritten)
{
    *pbyteswritten = 0;


    // error if file not opened
    if (m_State == Invalid) {
        return FALSE;
    }

    DWORD nBytes;

    while (count > 0) {


        // is our current buffer ready to write this data ?
        // (we need to tell it eof pos as well since if eof is
        // in middle of buffer but beyond valid data, ok to write.)

	if ((m_Current < 0) ||
	    (!m_Buffers[m_Current].QueryPosition(m_Position, m_Size))) {

            // commit this buffer if we have changed position beyond it
            if (m_Current >= 0) {
		if (!m_Buffers[m_Current].Commit()) {
		    // file error - abort
		    return FALSE;
		}

		// if we are streaming, then advance to next buffer while
		// current one is writing.
		if (m_State != Stopped) {
		    m_Current = NextBuffer(m_Current);
		}
	    } else {
		m_Current = 0;
	    }

            // make sure that previous operations on this buffer have completed
            if (!m_Buffers[m_Current].WaitComplete()) {
                // i/o error
                return FALSE;
            }
        }

        // we either have a buffer that has already pre-read the sector
        // we start writing to, or we have an idle buffer that
        // will do the pre-read for us
        if (!m_Buffers[m_Current].Write(m_Position, pData, count, m_Size, &nBytes)) {
            return FALSE;
        }

        count -= nBytes;
        pData += nBytes;
        m_Position += nBytes;
        *pbyteswritten += nBytes;
    }

    if (m_Position > m_Size) {
        m_Size = m_Position;
    }

    return TRUE;
}



BOOL
CFileStream::Read(LPBYTE pData, DWORD count, DWORD * pbytesread)
{

    *pbytesread = 0;

    // error if file not opened
    if (m_State == Invalid) {
        return FALSE;
    }

    // force the read to be within the file size limits
    if (m_Position >= m_Size) {
        // all done - nothing read
        return TRUE;
    } else {
        count = min(count, (m_Size - m_Position));
    }

    BOOL bDoReadAhead = FALSE;
    DWORD nBytes;

    while (count > 0) {

        // is data within current buffer
        if ((m_Current < 0) ||
	    (!m_Buffers[m_Current].QueryPosition(m_Position, m_Size))) {

	    if (m_Current >= 0) {
		// commit this buffer if we have changed position beyond it
		if (!m_Buffers[m_Current].Commit()) {
		    // file error - abort
		    return FALSE;
		}

		// advance to next buffer (if streaming)
		if (m_State == Writing) {
		    m_Current = NextBuffer(m_Current);
		} else if (m_State == Reading) {

		    // smart read-ahead strategy: try to find in existing
		    // buffers, and only issue a read-ahead if we take the
		    // highest buffer
		    int n = NextBuffer(m_Current);
		    m_Current = -1;
		    for (int i = 0; i < m_NrValid; i++) {
			if (m_Buffers[n].QueryPosition(m_Position, m_Size)) {
			    m_Current = n;
			    break;
			}
			n = NextBuffer(n);
		    }
		    if (m_Current < 0) {
			// read-ahead is screwed because we have made too big
			// a seek for the current buffer size
			// Best thing is to use the lowest buffer (should be the
			// one after the highest, and to restart readaheads with
			// this position).
			m_Current = NextBuffer(m_HighestBuffer);
			m_HighestBuffer = m_Current;
			DPF("using idle %d\n", m_Current);

		    }

		    if (m_Current == m_HighestBuffer) {
			bDoReadAhead = TRUE;
		    }
		}
	    } else {
		m_Current = 0;
		if (m_Current == m_HighestBuffer) {
		    bDoReadAhead = TRUE;
		}
	    }



            // make sure that previous operations on this buffer have completed
            if (!m_Buffers[m_Current].WaitComplete()) {
                // i/o error
                return FALSE;
            }
        }

        // now we have a buffer that either contains the data we want, or
        // is idle and ready to fetch it.
        if (!m_Buffers[m_Current].Read(m_Position, pData, count, m_Size, &nBytes)) {
            return FALSE;
        }

        count -= nBytes;
        pData += nBytes;
        m_Position += nBytes;
        *pbytesread += nBytes;

        // do read ahead now if necessary (the Read() call may have required
        // a seek and read if the data was not in the buffer, so delay the
        // read-ahead until after it has completed).
        if (bDoReadAhead) {

            // remember that this new buffer contains the highest position
            // -- we should issue another readahead when we start using this
            // buffer.

            m_HighestBuffer = NextBuffer(m_Current);

            DWORD p = m_Buffers[m_Current].GetNextPosition();

            m_Buffers[m_HighestBuffer].ReadAhead(p, m_Size);

            bDoReadAhead = FALSE;
        }
    }

    return TRUE;
}


// set the right buffer size and count for current mode
BOOL
CFileStream::EnsureBuffersValid()
{
    if (m_State == Invalid) {
        // file not opened
        return FALSE;
    }

   #ifdef CHICAGO
    if (m_State == Writing) {
        m_NrValid = 4;          // total 256k
    } else if (m_State == Reading) {
        m_NrValid = 4;		// total 256k
    } else {
        m_NrValid = 1;		// total 64k
    }

    int size = (64 * 1024);
   #else
    if (m_State == Writing) {
        m_NrValid = 2;		// total 512k
    } else if (m_State == Reading) {
        m_NrValid = 4;		// total 256k
    } else {
        m_NrValid = 1;		// total 64k
    }

    int size = (64 * 1024);
    if (m_State == Writing)
        size = (256 * 1024);
   #endif

    int i =0;

    Assert(m_NrValid <= NR_OF_BUFFERS);

    // init valid buffers
    for (; i < m_NrValid; i++) {
       #ifdef CHICAGO
        if (!m_Buffers[i].Init(m_SectorSize, size, &m_qio)) {
       #else
        if (!m_Buffers[i].Init(m_SectorSize, size, m_hFile)) {
       #endif
            return FALSE;
        }
    }

    // discard others
    for (; i < NR_OF_BUFFERS; i++) {
        m_Buffers[i].FreeMemory();
    }
    return TRUE;
}

BOOL
CFileStream::StartStreaming()
{
    if (m_bWrite) {
	return StartWriteStreaming();
    } else {
	return StartReadStreaming();
    }
}

BOOL
CFileStream::StartWriteStreaming()
{
    m_State = Writing;

    if (!EnsureBuffersValid()) {
        return FALSE;
    }

    return TRUE;
}

BOOL
CFileStream::StartReadStreaming()
{
    // commit the current buffer
    if (!m_Buffers[m_Current].Commit()) {
        return FALSE;
    }

    m_State = Reading;

    if (!EnsureBuffersValid()) {
        return FALSE;
    }

    // start read-ahead on buffer 0 - read from current position
    // (tell buffer the eof point so it won't bother reading beyond it)
    // remember that this is the highest current buffer - when we start using
    // this buffer it is time to issue the next readahead (this allows for
    // seeks backwards and forwards within the valid buffers without upsetting
    // the read-aheads).

    m_HighestBuffer = 0;
    m_Buffers[0].ReadAhead(m_Position, m_Size);

    // set m_Current invalid: this ensures that we will wait for read-ahead
    // to complete before getting data, and that when we start using it, we
    // will issue the next read-ahead.
    m_Current = -1;

    return TRUE;

}

BOOL
CFileStream::StopStreaming()
{
    // complete all i/o
    if (!CommitAndWait()) {
        return FALSE;
    }

    m_Current = 0;
    m_State = Stopped;

    // recalc buffer size/count for new mode
    if (!EnsureBuffersValid()) {
        return FALSE;
    }

    return TRUE;
}


// wait for all transfers to complete.
BOOL CFileStream::CommitAndWait()
{
    // write current buffer
    //
    if (!m_Buffers[m_Current].Commit())
        return FALSE;

   #ifdef CHICAGO
    // flush all buffers that have been queued
    //
    //QioCommit (&m_qio);
   #endif

    // wait for all buffers to complete
    for (int i = 0; i < m_NrValid; i++) {

        if (!m_Buffers[i].WaitComplete()) {
            return FALSE;
        }
    }
    // no need to reset m_Current
    return TRUE;
}


// destructor will call Commit()
CFileStream::~CFileStream()
{
    if (m_hFile != INVALID_HANDLE_VALUE) {
        CommitAndWait();

       #ifdef CHICAGO
        QioShutdown (&m_qio);
       #endif

        CloseHandle(m_hFile);
    }
}


// --- CFileBuffer methods -----------------------------------------



// initiate to an invalid (no buffer ready) state
CFileBuffer::CFileBuffer()
{
    m_pBuffer = NULL;
    m_pAllocedMem = NULL;
    m_State = Invalid;
#ifdef CHICAGO
    m_pqio = NULL;
#endif

}

// allocate memory and become idle.
BOOL
#ifdef CHICAGO
CFileBuffer::Init(DWORD nBytesPerSector, DWORD buffersize, LPQIO pqio)
#else
CFileBuffer::Init(DWORD nBytesPerSector, DWORD buffersize, HANDLE hfile)
#endif
{
    if (m_State != Invalid) {

        if ((nBytesPerSector == m_BytesPerSector) &&
            (buffersize == RoundSizeToSector(m_TotalSize))) {

                // we're there already
                return TRUE;
        }

        // discard what we have
        FreeMemory();
    }

    Assert(m_State == Invalid);

    // round up RAWIO_SIZE to a multiple of sector size
    m_BytesPerSector = nBytesPerSector;
    m_TotalSize = (DWORD) RoundSizeToSector(buffersize);

    m_DataLength = 0;
    m_State = Idle;
    m_bDirty = FALSE;

   #ifdef CHICAGO

    m_pqio = pqio;
    m_pAllocedMem = (unsigned char *)VirtualAlloc (NULL, m_TotalSize,
                              MEM_RESERVE | MEM_COMMIT,
                              PAGE_READWRITE);
    if (m_pAllocedMem == NULL)
        return FALSE;

   #else

    m_hFile = hfile;
    m_pAllocedMem = new BYTE[m_TotalSize];

    if (m_pAllocedMem == NULL)
        return FALSE;

    m_Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!m_Overlapped.hEvent) {
        delete[] m_pAllocedMem;
        return FALSE;
    }

   #endif

   // this is where my naming scheme falls down. RoundPos rounds down, and
   // RoundSize rounds up. to correctly align the buffer and stay within it,
   // we need to round the start address up and the size down.

   // round start address up to sector size
   m_pBuffer = (LPBYTE) RoundSizeToSector((LONG_PTR) m_pAllocedMem);
   // remove rounding from size - and round it again!
   m_TotalSize = (DWORD) RoundPosToSector(m_TotalSize - (m_pBuffer - m_pAllocedMem));



    return TRUE;
}


// revert to invalid state (eg when streaming stops)
void
CFileBuffer::FreeMemory()
{
    if (m_State == Idle) {
        Commit();
    }
    if (m_State == Busy) {
        WaitComplete();
    }

    if (m_State != Invalid) {

       #ifdef CHICAGO

        VirtualFree (m_pAllocedMem, 0, MEM_RELEASE);
        m_pBuffer = NULL;
	m_pAllocedMem = NULL;

       #else

        CloseHandle(m_Overlapped.hEvent);
        delete[] m_pAllocedMem;

       #endif

        m_State = Invalid;
    }
}

// calls commit if dirty before freeing everything.
CFileBuffer::~CFileBuffer()
{
    FreeMemory();
}


// does this position occur anywhere within the current buffer ?
// needs to know current eof for some cases (writing beyond eof
// if eof is within this buffer is ok to this buffer).
//
// we can use this buffer if:
// 1. if the buffer is empty and the write is past eof (where eof is rounded
//    to a sector boundary).
//
// 2. if the start position is within the current m_DataLength
//
// 3. if eof is within the buffer and the write is past eof
//
// all reads are limited by the caller to be within the file limits, so
// the reading case is covered by 2 above (1 and 3 will not occur).
//
// all other cases will require the read of data that is not in the buffer.
// or the (early) committing of data in the buffer
//
BOOL
CFileBuffer::QueryPosition(DWORD pos, DWORD filesize)
{

    if (m_State == Invalid) {
        return FALSE;
    }

    // round filesize to sector boundary
    filesize = (DWORD) RoundSizeToSector(filesize);

    if (pos >= filesize) {

        // write is past eof. ok if buffer empty or if buffer contains
        // eof (and has space in it)
        if ((m_DataLength == 0) ||
            ((m_Position + m_DataLength == filesize) &&
             (m_DataLength < m_TotalSize))) {
                return TRUE;
        }

        // we have data that needs to be flushed before we can do this
        return FALSE;
    } else {

        if ((pos >= m_Position) &&
            (pos < m_Position + m_DataLength)) {

                // we have this byte
                return TRUE;
        }

        // we don't have this byte of valid data. we have some other.
        //
        // you might think that if the write begins on a sector boundary, and
        // this buffer's data is not dirty you could permit this without a
        // pre-read - but we don't know yet where the write will end, and if
        // it ends mid-sector and not past current eof, we will need to
        // read that sector in.
        return FALSE;
    }
}



// write some data to buffer (must be committed separately)
// filesize parameter is the file size before this write, and is used to
// control what we do with the partial sector at beginning and end
// -if not past current eof, we need to read the current sector before
// writing to it.
BOOL
CFileBuffer::Write(
    DWORD pos,
    LPBYTE pData,
    DWORD count,
    DWORD filesize,
    DWORD * pbytesWritten)
{

    // remember for later (during commit)
    m_FileLength = filesize;

    *pbytesWritten = 0;

    if (m_State != Idle) {
        if (!WaitComplete()) {
            return FALSE;
        }
    }

    if (m_State == Invalid) {
        // naughty boy!
        return FALSE;
    }


    // do we need to commit the current contents or read anything ?

    // if there is data, and the start position is not within the valid data
    // range, then flush this lot. note that we count the region from
    // end of valid data to end of actual buffer as valid data if the eof
    // is within this buffer.
    if ((m_DataLength > 0) &&
        ((pos < m_Position) ||
        (pos >= m_Position + m_TotalSize) ||
        ((pos >= m_Position + m_DataLength) &&
         ((m_Position + m_DataLength) < filesize)))) {

            // we're not ok - need to flush current contents
            if (!Commit() || !WaitComplete()) {
                return FALSE;
            }
            m_DataLength = 0;
    }

    // if empty (or we just flushed it), we can start at the beginning
    if (m_DataLength == 0) {
        m_Position = (DWORD) RoundPosToSector(pos);

        // do we need to read the partial sector?
        if ((pos < RoundSizeToSector(filesize))  &&
            (pos % m_BytesPerSector != 0)) {

            // yes - write starts partway through a valid sector
	    m_DataLength = m_BytesPerSector;
            if (!ReadIntoBuffer(0, m_Position, m_BytesPerSector) ||
                !WaitComplete()) {
                    return FALSE;
            }
        }
    }

    // we can start the data. now what about the end?
    // if it all fits within the buffer, and it ends mid-sector and the
    // final sector is within the file length but not currently in the
    // buffer, we will need to pre-read the final buffer

    if ((pos + count) < (m_Position + m_TotalSize)) {

        if ((pos + count) % m_BytesPerSector) {

            // we have to write a partial sector - is it past eof or within
            // valid region ?
            if ((pos+count > m_Position + m_DataLength) &&
                (pos+count < filesize)) {

                    // yes need to read partial sector
                    DWORD sec = (DWORD) RoundPosToSector(pos+count);

		    // need to temporarily set m_DataLength
		    // to the amount read so that WaitComplete can check
		    // its ok
		    m_DataLength = m_BytesPerSector;

                    if (!ReadIntoBuffer(
                        sec - m_Position,       // index in buffer
                        sec,                    // position in file
                        m_BytesPerSector) ||
                        !WaitComplete()) {
                            return FALSE;
                    }
		    // set size correctly again
                    m_DataLength = (sec - m_Position) + m_BytesPerSector;
            }
        }
    }

    // now we can stuff the data in
    int index = pos - m_Position;
    *pbytesWritten = min(count,  m_TotalSize - index);

    CopyMemory(
        &m_pBuffer[index],
        pData,
        *pbytesWritten);

    // adjust data length
    if ((index + *pbytesWritten) > m_DataLength) {
	m_DataLength = (DWORD) RoundSizeToSector(index + *pbytesWritten);
    }

    m_bDirty = TRUE;

    return TRUE;
}




// read data from buffer (will seek and read if necessary first)
BOOL
CFileBuffer::Read(
    DWORD pos,
    LPBYTE pData,
    DWORD count,
    DWORD filelength,
    DWORD * pBytesRead)
{

    Assert(m_State == Idle);

    // remember this for read completion checking
    m_FileLength = filelength;

    *pBytesRead = 0;

    if ((pos < m_Position) ||
        (pos >= m_Position + m_DataLength)) {

        // not in current buffer - flush current contents if dirty
        if (!Commit() || !WaitComplete()) {
            return FALSE;
        }

        m_Position = (DWORD) RoundPosToSector(pos);

        // remember if we round the start down, we also need to increase
        // the length (as well as rounding it up at the other end)
        // force a minimum read size to avoid lots of single sectors
        m_DataLength = count + (pos - m_Position);
        m_DataLength = max(MIN_READ_SIZE, m_DataLength);

        m_DataLength = (DWORD) RoundSizeToSector(m_DataLength);

        m_DataLength = min(m_DataLength, m_TotalSize);

        if (!ReadIntoBuffer(0, m_Position, m_DataLength) ||
	    !WaitComplete()) {
            return FALSE;
        }
    }

    // we have (at least the start part of) the data in the buffer

    int offset = pos - m_Position;
    count = min(count, m_DataLength - offset);
    CopyMemory(pData, &m_pBuffer[offset], count);

    *pBytesRead = count;

    return TRUE;
}


// what is the first file position after this buffer's valid data
// ---return this even if still busy reading it
DWORD
CFileBuffer::GetNextPosition()
{
    if ((m_State == Invalid) || (m_DataLength == 0)) {
        return 0;
    } else {
        return m_Position + m_DataLength;
    }
}

// initiate a read-ahead
void
CFileBuffer::ReadAhead(DWORD start, DWORD filelength)
{
    if (m_State != Idle) {
        if (!CheckComplete()) {
            return;
        }
    }

    // we may already hold this position
    if (QueryPosition(start, filelength)) {
	return;
    }

    m_FileLength = filelength;

    if (m_bDirty) {

        // current data needs to be flushed to disk.
        // we should initiate this, but we can't wait for
        // it to complete, so we won't do the read-ahead
        Commit();
        return;
    }

    m_Position = (DWORD) RoundPosToSector(start);
    m_DataLength = min((DWORD) RoundSizeToSector(filelength - m_Position),
                        m_TotalSize);

    ReadIntoBuffer(0, m_Position, m_DataLength);
    // no wait - this is an async readahead.

}



// initiate the i/o from the buffer
BOOL
CFileBuffer::Commit()
{
    if ((m_State != Idle) || (!m_bDirty)) {
        return TRUE;
    }

#ifndef CHICAGO
    DWORD nrWritten;
#endif

   #ifdef CHICAGO

    m_State = Busy;

    m_qiobuf.dwOffset = m_Position;
    m_qiobuf.lpv = m_pBuffer;
    m_qiobuf.cb = m_DataLength;
    m_qiobuf.cbDone = 0;
    m_qiobuf.bWrite = TRUE;
    m_qiobuf.dwError = ERROR_IO_PENDING;

    QioAdd (m_pqio, &m_qiobuf);

   #else

    ResetEvent(m_Overlapped.hEvent);

    m_State = Busy;

    //start from m_Position
    m_Overlapped.Offset = m_Position;
    m_Overlapped.OffsetHigh = 0;


    if (WriteFile(m_hFile, m_pBuffer, m_DataLength,
            &nrWritten, &m_Overlapped)) {

	DPF(("instant completion"));

        // if it completed already, then sort out the new position
        if (nrWritten != m_DataLength) {
	    DPF("commit- bad length %d not %d", nrWritten, m_DataLength);
            return FALSE;
        }
        m_bDirty = FALSE;
        m_State = Idle;
    } else {
        // should be pending
        if (GetLastError() != ERROR_IO_PENDING) {

	    // no longer busy
	    m_State = Idle;

	    DPF("commit error %d", GetLastError());

            return FALSE;
        }
    }

   #endif

    // we must do this here, since WaitComplete could complete a
    // partial read that would leave the buffer dirty.
    // we are safe since the buffer will remain Busy until this is
    // actually TRUE. (if we fail to write to the disk then the
    // file state is guaranteed screwed).
    m_bDirty = FALSE;

    return TRUE;


}

// wait for any pending commit or read to complete and check for errors.
BOOL
CFileBuffer::WaitComplete()
{
    if (m_State == ErrorOccurred) {

        // the i/o has completed in error but we haven't been able to
        // report the fact yet
        m_State = Idle;
        return FALSE;
    }

    if (m_State == Busy) {
        DWORD actual;

	// no longer busy
        m_State = Idle;

       #ifdef CHICAGO
        if ( ! QioWait (m_pqio, &m_qiobuf, TRUE))
            return FALSE;
        actual = m_qiobuf.cbDone;
       #else
        if (!GetOverlappedResult(m_hFile, &m_Overlapped, &actual, TRUE)) {
	    DPF("WC: GetOverlapped failed %d", GetLastError());
            return FALSE;
        }
       #endif
        if (actual != m_DataLength) {

	    // rounding to sector size may have taken us past eof
	    if (m_Position + actual != m_FileLength) {
		DPF("WC: actual wrong (%d not %d)", actual, m_DataLength);
		return FALSE;
	    }
        }
    }

    return TRUE;

}

// non-blocking check to see if async io is complete
BOOL
CFileBuffer::CheckComplete()
{
    if (m_State == Idle) {
        return TRUE;
    }

    if (m_State != Busy) {
        return FALSE;   // invalid or error
    }

   #ifdef CHICAGO

    if (QioWait(m_pqio, &m_qiobuf, FALSE))
        return FALSE;

    else if (m_qiobuf.dwError == 0) {
        m_State = Idle;
        return TRUE;
        }

    m_State = ErrorOccurred;
    return FALSE;

   #else

    DWORD actual;

    if (GetOverlappedResult(m_hFile, &m_Overlapped, &actual, FALSE)) {

        if ((actual == m_DataLength) ||
            (actual + m_Position == m_FileLength)) {
                m_State = Idle;
                return TRUE;
        }

    } else if (GetLastError() == ERROR_IO_INCOMPLETE) {
        // still busy
        return FALSE;
    }

    // some error state occurred - this must be reported by WaitComplete()
    m_State = ErrorOccurred;
    DPF("CheckComplete error %d", GetLastError());
    return FALSE;

   #endif
}



// initiates an async read request into the buffer (can be an insertion into
// middle of buffer rather than a complete buffer fill - and so will not
// adjust m_Position or m_DataLength). reads count bytes
// offset bytes from the start of the buffer, pos bytes from the start of the
// file. Assumes necessary rounding of length and position has already happened.
BOOL
CFileBuffer::ReadIntoBuffer(int offset, DWORD pos, DWORD count)
{

    Assert(m_State == Idle);

#ifndef CHICAGO
    DWORD nrRead;
#endif

   #ifdef CHICAGO

    m_State = Busy;

    m_qiobuf.dwOffset = pos;
    m_qiobuf.lpv = (LPVOID)(m_pBuffer + offset);
    m_qiobuf.cb = count;
    m_qiobuf.cbDone = 0;
    m_qiobuf.bWrite = FALSE;
    m_qiobuf.dwError = ERROR_IO_PENDING;

    // if this read is not sector aligned, we cannot do it
    // in async in chicago, so do it right now!
    //
    if ((count & 511) || (pos & 511) || (offset & 511))
    {
        DWORD dwOff;

        m_qiobuf.bPending = FALSE;

	DPF("%s %X bytes (non-aligned) at %08X into %08X\r\n", m_qiobuf.bWrite ? "Writing" : "Reading", m_qiobuf.cb, m_qiobuf.dwOffset, m_qiobuf.lpv);
	
        dwOff = SetFilePointer (m_pqio->hFile, m_qiobuf.dwOffset, NULL, FILE_BEGIN);
        if (dwOff != m_qiobuf.dwOffset)
        {
            m_qiobuf.dwError = GetLastError();
	    DPF("avifile32 non-aligned seek error %d", m_qiobuf.dwError);
            return FALSE;
        }
        else if ( ! ReadFile (m_pqio->hFile, m_qiobuf.lpv, m_qiobuf.cb,
                              &m_qiobuf.cbDone, NULL) ||
                  (m_qiobuf.cbDone != m_qiobuf.cb))
        {
            m_qiobuf.dwError = GetLastError ();
	    DPF("avifile32 non-aligned read error %d", m_qiobuf.dwError);
            return FALSE;
        }
        m_State = Idle;
    }
    else
       return QioAdd (m_pqio, &m_qiobuf);

   #else

    ResetEvent(m_Overlapped.hEvent);

    m_State = Busy;


    //start from pos
    m_Overlapped.Offset = pos;
    m_Overlapped.OffsetHigh = 0;


    if (ReadFile(m_hFile, &m_pBuffer[offset], count,
            &nrRead, &m_Overlapped)) {

        m_State = Idle;

	DPF(("instant completion"));

        // if it completed already, then sort out the new position
        if (nrRead != count) {

	    // rounding to sector size may have taken us past eof -
	    // in this case we must still ask for the full sector, but
	    // we will be told about the actual size
	    if (m_Position + nrRead != m_FileLength) {
		DPF("ReadInto: actual wrong");
		return FALSE;
	    }
        }
    } else {
        // should be pending
        if (GetLastError() != ERROR_IO_PENDING) {
            DPF("read failed %d\n", GetLastError());

	    // no longer busy
	    m_State = Idle;
	    DPF("ReadInto failed %d", GetLastError());
            return FALSE;
        }
    }
   #endif
    return TRUE;
}


#endif // USE_DIRECTIO
