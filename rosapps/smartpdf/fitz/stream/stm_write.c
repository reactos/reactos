/*
 * Output streams.
 */

#include "fitz-base.h"
#include "fitz-stream.h"

int fz_wtell(fz_stream *stm)
{
	fz_buffer *buf = stm->buffer;
	int t;

	if (stm->dead)
		return -1;

	if (stm->mode != FZ_SWRITE)
		return -1;

	switch (stm->kind)
	{
	case FZ_SFILE:
		t = lseek(stm->file, 0, 1);
		if (t < 0)
		{
			stm->error = fz_throw("ioerror: lseek: %s", strerror(errno));
			stm->dead = 1;
			return -1;
		}
		return t + (buf->wp - buf->rp);

	case FZ_SFILTER:
		return stm->filter->count + (buf->wp - buf->rp);

	case FZ_SBUFFER:
		return buf->wp - buf->bp;
	}

	return -1;
}

int fz_wseek(fz_stream *stm, int offset, int whence)
{
	fz_buffer *buf = stm->buffer;
	int t;

	if (stm->dead)
		return -1;

	if (stm->mode != FZ_SWRITE)
		return -1;

	if (stm->kind != FZ_SFILE)
		return -1;

	t = lseek(stm->file, offset, whence);
	if (t < 0)
	{
		stm->error = fz_throw("ioerror: lseek: %s", strerror(errno));
		stm->dead = 1;
		return -1;
	}

	buf->rp = buf->bp;
	buf->wp = buf->bp;
	buf->eof = 0;

	return t;
}

static int flushfilter(fz_stream *stm)
{
	fz_buffer *buf = stm->buffer;
	fz_error *error;
	fz_error *reason;
	int t;

loop:

	reason = fz_process(stm->filter, stm->buffer, stm->chain->buffer);

	if (reason == fz_ioneedin)
	{
		if (buf->rp > buf->ep)
			fz_rewindbuffer(buf);
		else
		{
			error = fz_growbuffer(buf);
			if (error)
				goto cleanup;
		}
	}

	else if (reason == fz_ioneedout)
	{
		t = fz_flush(stm->chain);
		if (t < 0)
			return -1;
	}

	else if (reason == fz_iodone)
	{
		stm->dead = 2;	/* special flag that we are dead because of eod */
	}

	else
	{
		error = reason;
		goto cleanup;
	}

	/* if we are at eof, repeat until other filter sets otherside to eof */
	if (buf->eof && !stm->chain->buffer->eof)
		goto loop;

	return 0;

cleanup:
	stm->error = error;
	stm->dead = 1;
	return -1;
}

/*
 * Empty the buffer into the sink.
 * Promise to make more space available.
 * Called by fz_write and fz_dropstream.
 * If buffer is eof, then all data must be flushed.
 */
int fz_flush(fz_stream *stm)
{
	fz_buffer *buf = stm->buffer;
	fz_error *error;
	int t;

	if (stm->dead == 2)
		return 0;

	if (stm->dead)
		return -1;

	if (stm->mode != FZ_SWRITE)
		return -1;

	switch (stm->kind)
	{
	case FZ_SFILE:
		while (buf->rp < buf->wp)
		{
			t = write(stm->file, buf->rp, buf->wp - buf->rp);
			if (t < 0)
			{
				stm->error = fz_throw("ioerror: write: %s", strerror(errno));
				stm->dead = 1;
				return -1;
			}

			buf->rp += t;
		}

		if (buf->rp > buf->bp)
			fz_rewindbuffer(buf);

		return 0;

	case FZ_SFILTER:
		return flushfilter(stm);

	case FZ_SBUFFER:
		if (!buf->eof && buf->wp == buf->ep)
		{
			error = fz_growbuffer(buf);
			if (error)
			{
				stm->error = error;
				stm->dead = 1;
				return -1;
			}
		}
		return 0;
	}

	return -1;
}

/*
 * Write data to stream.
 * Buffer until internal buffer is full.
 * When full, call fz_flush to make more space available.
 */
int fz_write(fz_stream *stm, unsigned char *mem, int n)
{
	fz_buffer *buf = stm->buffer;
	int i = 0;
	int t;

	if (stm->dead == 2)
		return 0;

	if (stm->dead)
		return -1;

	if (stm->mode != FZ_SWRITE)
		return -1;

	while (i < n)
	{
		while (buf->wp < buf->ep && i < n)
			*buf->wp++ = mem[i++];

		if (buf->wp == buf->ep && i < n)
		{
			t = fz_flush(stm);
			if (t < 0)
				return -1;
			if (stm->dead)
				return i;
		}
	}

	return n;
}

int fz_printstr(fz_stream *stm, char *s)
{
	return fz_write(stm, s, strlen(s));
}

int fz_printobj(fz_stream *file, fz_obj *obj, int tight)
{
	char buf[1024];
	char *ptr;
	int n;

	n = fz_sprintobj(nil, 0, obj, tight);
	if (n < sizeof buf)
	{
		fz_sprintobj(buf, sizeof buf, obj, tight);
		return fz_write(file, buf, n);
	}
	else
	{
		ptr = fz_malloc(n);
		if (!ptr)
			return -1;
		fz_sprintobj(ptr, n, obj, tight);
		n = fz_write(file, ptr, n);
		fz_free(ptr);
		return n;
	}
}

int fz_print(fz_stream *stm, char *fmt, ...)
{
	va_list ap;
	char buf[1024];
	char *p;
	int n;

	va_start(ap, fmt);
	n = vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);

	if (n < sizeof buf)
		return fz_write(stm, buf, n);

	p = fz_malloc(n);
	if (!p)
		return -1;

	va_start(ap, fmt);
	vsnprintf(p, n, fmt, ap);
	va_end(ap);

	n = fz_write(stm, p, n);

	fz_free(p);

	return n;
}

