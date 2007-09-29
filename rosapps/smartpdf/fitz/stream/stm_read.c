/*
 * Input streams.
 */

#include "fitz-base.h"
#include "fitz-stream.h"

int
fz_makedata(fz_stream *stm)
{
	fz_buffer *buf = stm->buffer;
	fz_error *error;
	fz_error *reason;
	int produced;
	int n;

	if (stm->dead)
		return -1;

	if (stm->mode != FZ_SREAD)
		return -1;

	if (buf->eof)
		return 0;

	error = fz_rewindbuffer(buf);
	if (error)
		goto cleanup;

	if (buf->ep - buf->wp == 0)
	{
		error = fz_growbuffer(buf);
		if (error)
			goto cleanup;
	}

	switch (stm->kind)
	{

	case FZ_SFILE:
		n = read(stm->file, buf->wp, buf->ep - buf->wp);
		if (n == -1)
		{
			stm->error = fz_throw("ioerror: read: %s", strerror(errno));
			stm->dead = 1;
			return -1;
		}
		if (n == 0)
			buf->eof = 1;
		buf->wp += n;
		return n;

	case FZ_SFILTER:
		produced = 0;

		while (1)
		{
			reason = fz_process(stm->filter, stm->chain->buffer, buf);

			if (stm->filter->produced)
				produced = 1;

			if (reason == fz_ioneedin)
			{
				if (fz_makedata(stm->chain) < 0)
				{
					stm->dead = 1;
					return -1;
				}
			}

			else if (reason == fz_ioneedout)
			{
				if (produced)
					return 0;

				if (buf->rp > buf->bp)
				{
					error = fz_rewindbuffer(buf);
					if (error)
						goto cleanup;
				}
				else
				{
					error = fz_growbuffer(buf);
					if (error)
						goto cleanup;
				}
			}

			else if (reason == fz_iodone)
			{
				return 0;
			}

			else
			{
				error = reason;
				goto cleanup;
			}
		}

	case FZ_SBUFFER:
		return 0;
	}

	return -1;

cleanup:
	stm->error = error;
	stm->dead = 1;
	return -1;
}

int fz_rtell(fz_stream *stm)
{
	fz_buffer *buf = stm->buffer;
	int t;

	if (stm->dead)
		return -1;

	if (stm->mode != FZ_SREAD)
		return -1;

	switch (stm->kind)
	{
	case FZ_SFILE:
		t = lseek(stm->file, 0, 1);
		if (t < 0)
		{
			stm->dead = 1;
			return -1;
		}
		return t - (buf->wp - buf->rp);

	case FZ_SFILTER:
		return stm->filter->count - (buf->wp - buf->rp);

	case FZ_SBUFFER:
		return buf->rp - buf->bp;
	}

	return -1;
}

int fz_rseek(fz_stream *stm, int offset, int whence)
{
	fz_buffer *buf = stm->buffer;
	int t, c;

	if (stm->dead)
		return -1;

	if (stm->mode != FZ_SREAD)
		return -1;

	if (whence == 1)
	{
		int cur = fz_rtell(stm);
		if (cur < 0)
			return -1;
		offset = cur + offset;
		whence = 0;
	}

	buf->eof = 0;

	switch (stm->kind)
	{
	case FZ_SFILE:
		t = lseek(stm->file, offset, whence);
		if (t < 0)
		{
			stm->error = fz_throw("ioerror: lseek: %s", strerror(errno));
			stm->dead = 1;
			return -1;
		}

		buf->rp = buf->bp;
		buf->wp = buf->bp;

		return t;

	case FZ_SFILTER:
		if (whence == 0)
		{
			if (offset < fz_tell(stm))
			{
				stm->error = fz_throw("ioerror: cannot seek back in filter");
				stm->dead = 1;
				return -1;
			}
			while (fz_tell(stm) < offset)
			{
				c = fz_readbyte(stm);
				if (c == EOF)
					break;
			}
			return fz_tell(stm);
		}
		else
		{
			stm->dead = 1;
			return -1;
		}

	case FZ_SBUFFER:
		if (whence == 0)
			buf->rp = CLAMP(buf->bp + offset, buf->bp, buf->ep);
		else
			buf->rp = CLAMP(buf->ep + offset, buf->bp, buf->ep);
		return buf->rp - buf->bp;
	}

	return -1;
}

int fz_readbytex(fz_stream *stm)
{
	fz_buffer *buf = stm->buffer;
	if (buf->rp == buf->wp)
	{
		if (buf->eof) return EOF;
		if (fz_makedata(stm) < 0) return EOF;
	}
	if (buf->rp < buf->wp)
		return *buf->rp++;
	return EOF;
}

int fz_peekbytex(fz_stream *stm)
{
	fz_buffer *buf = stm->buffer;
	if (buf->rp == buf->wp)
	{
		if (buf->eof) return EOF;
		if (fz_makedata(stm) < 0) return EOF;
	}
	if (buf->rp < buf->wp)
		return *buf->rp;
	return EOF;
}

int fz_read(fz_stream *stm, unsigned char *mem, int n)
{
	fz_buffer *buf = stm->buffer;
	int i = 0;

	while (i < n)
	{
		while (buf->rp < buf->wp && i < n)
			mem[i++] = *buf->rp++;
		if (buf->rp == buf->wp)
		{
			if (buf->eof) return i;
			if (fz_makedata(stm) < 0) return -1;
		}
	}

	return i;
}

