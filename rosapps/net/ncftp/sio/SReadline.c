#include "syshdrs.h"

void
FlushSReadlineInfo(SReadlineInfo *srl)
{
	/* Discards any input left in the current buffer,
	 * and resets the buffer and its pointer.
	 */
	srl->bufSize = srl->bufSizeMax;
	memset(srl->buf, 0, srl->bufSize);
	srl->bufLim = srl->buf + srl->bufSizeMax;

	/* This line sets the buffer pointer
	 * so that the first thing to do is reset and fill the buffer
	 * using real I/O.
	 */
	srl->bufPtr = srl->bufLim;
}	/* FlushSReadlineInfo */




int
InitSReadlineInfo(SReadlineInfo *srl, int fd, char *buf, size_t bsize, int tlen, int requireEOLN)
{
	if (buf == NULL) {
		if (bsize < 512)
			bsize = 512;	/* Pointless, otherwise. */
		buf = (char *) malloc(bsize);
		if (buf == NULL)
			return (-1);
		srl->malloc = 1;
	} else {
		srl->malloc = 0;
	}
	memset(buf, 0, bsize);
	srl->buf = buf;
	srl->bufSize = bsize;
	srl->bufSizeMax = bsize;
	srl->bufLim = srl->buf + bsize;
	srl->fd = fd;
	srl->timeoutLen = tlen;
	srl->requireEOLN = requireEOLN;

	/* This line sets the buffer pointer
	 * so that the first thing to do is reset and fill the buffer
	 * using real I/O.
	 */
	srl->bufPtr = srl->bufLim;
	return (0);
}	/* InitSReadlineInfo */




void
DisposeSReadlineInfo(SReadlineInfo *srl)
{
	memset(srl->buf, 0, srl->bufSizeMax);
	if (srl->malloc != 0)
		free(srl->buf);
	memset(srl, 0, sizeof(SReadlineInfo));

	/* Note: it does not close(srl->fd). */
}	/* DisposeSReadlineInfo */




/* Returns the number of bytes read, including the newline which is
 * also appended to the buffer.  If you don't want that newline,
 * set buf[nread - 1] = '\0', if nread > 0.
 */

int 
SReadline(SReadlineInfo *srl, char *const linebuf, size_t linebufsize)
{
	int err;
	char *src;
	char *dst;
	char *dstlim;
	int len;
	int nr;
	int requireEOLN;
	int illegals;

	illegals = 0;
	err = 0;
	dst = linebuf;
	dstlim = dst + linebufsize - 1;		       /* Leave room for NUL. */
	src = srl->bufPtr;
	requireEOLN = srl->requireEOLN;
	if (requireEOLN)
		dstlim--;
	if (dstlim <= dst)
		return (-1);				/* Buffer too small. */

	forever {
		if ((requireEOLN == 0) && (dst >= dstlim))
			break;
		if (src >= srl->bufLim) {
			/* Fill the buffer. */
			if (illegals > 1) {
				/* Probable DOS -- return now and give you an
				 * opportunity to handle bogus input.
				 */
				goto done;
			}
			nr = SRead(srl->fd, srl->buf, srl->bufSizeMax, srl->timeoutLen, 0);
			if (nr == 0) {
				/* EOF. */
				goto done;
			} else if (nr < 0) {
				/* Error. */
				err = nr;
				goto done;
			}
			srl->bufPtr = src = srl->buf;
			srl->bufLim = srl->buf + nr;
		}
		if (*src == '\0') {
			++src;
			illegals++;
		} else if (*src == '\r') {
			++src;
			/* If the next character is a \n that is valid,
			 * otherwise treat a stray \r as an illegal character.
			 */
			if ((src < srl->bufLim) && (*src != '\n'))
				illegals++;
		} else {
			if (*src == '\n') {
				if (dst < dstlim)
					*dst++ = *src++;
				else
					src++;
				goto done;
			}
			if (dst < dstlim)
				*dst++ = *src++;
			else
				src++;
		}
	}

done:
	srl->bufPtr = src;
	if ((requireEOLN != 0) && (dst != linebuf) && (dst[-1] != '\n'))
		*dst++ = '\n';
	if ((requireEOLN != 0) && (dst == linebuf) && (illegals > 0))
		*dst++ = '\n';
	*dst = '\0';
	len = (int) (dst - linebuf);
	if (err < 0)
		return (err);
	return (len);
}						       /* SReadline */
