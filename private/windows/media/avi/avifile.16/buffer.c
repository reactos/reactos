#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <mmsystem.h>
#include "debug.h"
#include "aviidx.h"
#include "buffer.h"
#include "avifile.h"

#ifndef WIN32
LONG    glDosBufUsage;
LPVOID  glpDosBuf;
LONG    glDosBufSize;
#endif

// Idea: keep a bunch (five, maybe) of buffers.

PBUFSYSTEM PASCAL InitBuffered(int nBuffers,
				LONG lBufSize,
                                HSHFILE hshfile,
                                PAVIINDEX px)
{
    PBUFSYSTEM pb = (PBUFSYSTEM)LocalAlloc(LPTR,
            sizeof(BUFSYSTEM) + sizeof(BUFFER) * nBuffers);

    int		i;
    LONG	l;

    if (!pb)
        return NULL;

    DPF("InitBuffered (%04x): %dx%ldK, pIndex = %08lx\n", pb, nBuffers, lBufSize / 1024, (DWORD) (LPVOID) px);

    pb->nBuffers = nBuffers;
    pb->lBufSize = lBufSize;

    pb->px = px;
    pb->lx = 0;

    pb->lpBufMem = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE, lBufSize * nBuffers);

    if (!pb->lpBufMem) {
	DPF("Couldn't allocate buffer memory!\n");
	EndBuffered(pb);
	return NULL;
    }

    pb->hshfile = hshfile;
    
    l = shfileSeek(hshfile, 0, SEEK_CUR);
    pb->lFileLength = shfileSeek(hshfile, 0, SEEK_END);
    shfileSeek(hshfile, l, SEEK_SET);

    for (i = 0; i < nBuffers; i++) {
	pb->aBuf[i].lpBuffer = (BYTE _huge *) pb->lpBufMem + i * lBufSize;
        pb->aBuf[i].lOffset  = -1;
    }

    return pb;
}

LONG FAR PASCAL BufferedRead(PBUFSYSTEM pb, LONG lPos, LONG cb, LPVOID lp)
{
    int	    i;
    LPVOID  lpCopy;
    LONG    cbCopy;
    LONG    cbRead = cb;
    LONG    l;

#if 0
    if (cb > pb->lBufSize) {
        if (shfileSeek(pb->hshfile, lPos, SEEK_SET) == -1)
            return 0;

        if (shfileRead(pb->hshfile, lp, cb) != cb)
            return 0;

        return cb;
    }
#endif
    
    while (cb > 0) {

	if (lPos >= pb->lFileLength)
	    break;
	
	// Find a buffer.
	for (i = 0; i < pb->nBuffers; i++) {
	    if (pb->aBuf[i].lOffset < 0)
		continue;

	    if (pb->aBuf[i].lOffset <= lPos &&
			pb->aBuf[i].lOffset + pb->aBuf[i].lLength > lPos)
		break;
	}

        // If we didn't find a buffer with valid data, read more.

	if (i >= pb->nBuffers) {
	    i = pb->iNextBuf;

            if (pb->px) {

                LONG off,len;

                for (l = pb->lx; l>=0 && l<pb->px->nIndex; ) {

                    off = IndexOffset(pb->px, l);
                    len = IndexLength(pb->px, l) + 2*sizeof(DWORD);

                    if (off <= lPos && lPos < off + len)
                        break;

                    if (lPos < off)
                        l--;
                    else
                        l++;
                }

                if (l == pb->px->nIndex || l < 0) {
                    DPF("Ran out of index!\n");
                    goto ack;
                }

                if (len > pb->lBufSize) {
                    DPF("Chunk is bigger than buffer.\n");
                    goto ack;
                }

                pb->aBuf[i].lOffset = off;
                pb->aBuf[i].lLength = len;

                DPF2("Buffer: Reading %lx bytes at %lx\n", pb->aBuf[i].lLength, pb->aBuf[i].lOffset);

                //
                //  read as many records that will fit in our buffer
                //
                //  we should scan backward!
                //
                for (l++; l<pb->px->nIndex; l++) {

                    off = IndexOffset(pb->px, l);
                    len = IndexLength(pb->px, l) + 2*sizeof(DWORD);

                    if (off < pb->aBuf[i].lOffset + pb->aBuf[i].lLength)
                        continue;

                    if (off != pb->aBuf[i].lOffset + pb->aBuf[i].lLength)
                        break;

                    if (pb->aBuf[i].lLength + len > pb->lBufSize)
                        break;

                    pb->aBuf[i].lLength += len;

                    DPF2("        Reading %lx bytes at %lx\n", pb->aBuf[i].lLength, pb->aBuf[i].lOffset);
                }

                if (l < pb->px->nIndex)
                    pb->lx = l;     // save this for next time.

	    } else
	    {
ack:
		// Always read aligned with the buffer size....
                pb->aBuf[i].lOffset = lPos - (lPos % pb->lBufSize);

		pb->aBuf[i].lLength =
			min(pb->lFileLength - pb->aBuf[i].lOffset,
                                      pb->lBufSize);

                DPF("Buffer: Reading %lx bytes at %lx\n", pb->aBuf[i].lLength, pb->aBuf[i].lOffset);
            }

            shfileSeek(pb->hshfile, pb->aBuf[i].lOffset, SEEK_SET);
	    if (glpDosBuf) {
		if (shfileRead(pb->hshfile,
			       glpDosBuf,
			       pb->aBuf[i].lLength) != pb->aBuf[i].lLength)
                    return 0;
		hmemcpy(pb->aBuf[i].lpBuffer, glpDosBuf, pb->aBuf[i].lLength);
	    } else {
		if (shfileRead(pb->hshfile,
			       pb->aBuf[i].lpBuffer,
			       pb->aBuf[i].lLength) != pb->aBuf[i].lLength)
                    return 0;
	    }

	    // !!! We should use an LRU algorithm or something here....
	    pb->iNextBuf = (i + 1) % pb->nBuffers;
	}

	lpCopy = (BYTE _huge *) pb->aBuf[i].lpBuffer + lPos - pb->aBuf[i].lOffset;

	cbCopy = min(cb, pb->aBuf[i].lLength - (lPos - pb->aBuf[i].lOffset));

	hmemcpy(lp, lpCopy, cbCopy);

	lp = (BYTE _huge *) lp + cbCopy;
	cb -= cbCopy;
	lPos += cbCopy;
    }

    return cbRead;
}

LONG FAR PASCAL BeginBufferedStreaming(PBUFSYSTEM pb, BOOL fForward)
{
    if (pb->fStreaming++)
	return 0;

    DPF("Streaming....\n");
    
#ifndef WIN32
    if (pb->px) {
	if (glDosBufSize < pb->lBufSize && GetProfileInt("avifile", "dosbuffer", 1)) {
	    LPVOID lpDosBuf;
	    
	    lpDosBuf = (LPVOID)MAKELONG(0, LOWORD(GlobalDosAlloc(pb->lBufSize)));
	    
	    if (!lpDosBuf) {
		DPF("Couldn't get DOS buffer!\n");
            } else {
                GlobalReAlloc((HANDLE)HIWORD(lpDosBuf), 0, GMEM_MODIFY|GMEM_SHARE);

		if (glpDosBuf)
                    GlobalDosFree(HIWORD(glpDosBuf));

                glpDosBuf = lpDosBuf;
                glDosBufSize = pb->lBufSize;
	    }
        }

	if (glpDosBuf && (glDosBufSize >= pb->lBufSize)) {
	    pb->fUseDOSBuf = TRUE;
	    glDosBufUsage++;
	} else
	    pb->fUseDOSBuf = FALSE;	
    }
#endif

    return 0;
}

LONG FAR PASCAL EndBufferedStreaming(PBUFSYSTEM pb)
{
    if (!pb->fStreaming)
	return AVIERR_INTERNAL;
    
    if (--pb->fStreaming)
	return 0;

    DPF("No longer streaming....\n");
    
#ifndef WIN32
    if (pb->fUseDOSBuf) {
	if (--glDosBufUsage == 0) {
	    if (glpDosBuf)
		GlobalDosFree(HIWORD(glpDosBuf));

	    glpDosBuf = NULL;
	}

	pb->fUseDOSBuf = FALSE;
    }
#endif

    return 0;
}


void FAR PASCAL EndBuffered(PBUFSYSTEM pb)
{
    DPF("Freeing bufsystem %04x....\n", pb);
    
    if (pb->lpBufMem)
	GlobalFreePtr(pb->lpBufMem);

#ifndef WIN32
    if (pb->fUseDOSBuf) {
	if (--glDosBufUsage == 0) {
	    if (glpDosBuf)
		GlobalDosFree(HIWORD(glpDosBuf));

	    glpDosBuf = NULL;
	    glDosBufSize = 0;
	}
    }
#endif

    LocalFree((HLOCAL)pb);
}
