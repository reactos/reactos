/*
 * Creation and destruction.
 */

#include "fitz-base.h"
#include "fitz-stream.h"

static fz_stream *
newstm(int kind, int mode)
{
	fz_stream *stm;

	stm = fz_malloc(sizeof(fz_stream));
	if (!stm)
		return nil;

	stm->refs = 1;
	stm->kind = kind;
	stm->mode = mode;
	stm->dead = 0;
	stm->error = nil;
	stm->buffer = nil;

	stm->chain = nil;
	stm->filter = nil;
	stm->file = -1;

	return stm;
}

fz_error *
fz_ioerror(fz_stream *stm)
{
	fz_error *error;
	if (stm->error)
	{
		error = stm->error;
		stm->error = nil;
		return error;
	}
	return fz_throw("ioerror: no error");
}

fz_stream *
fz_keepstream(fz_stream *stm)
{
	stm->refs ++;
	return stm;
}

void
fz_dropstream(fz_stream *stm)
{
	stm->refs --;
	if (stm->refs == 0)
	{
		if (stm->error)
		{
			fz_warn("unhandled %s", stm->error->msg);
			fz_droperror(stm->error);
		}

		if (stm->mode == FZ_SWRITE)
		{
			stm->buffer->eof = 1;
			fz_flush(stm);
		}

		switch (stm->kind)
		{
		case FZ_SFILE:
			close(stm->file);
			break;
		case FZ_SFILTER:
			fz_dropfilter(stm->filter);
			fz_dropstream(stm->chain);
			break;
		case FZ_SBUFFER:
			break;
		}

		fz_dropbuffer(stm->buffer);
		fz_free(stm);
	}
}

static fz_error *
openfile(fz_stream **stmp, char *path, int mode, int realmode)
{
	fz_error *error;
	fz_stream *stm;

	stm = newstm(FZ_SFILE, mode);
	if (!stm)
		return fz_outofmem;

	error = fz_newbuffer(&stm->buffer, FZ_BUFSIZE);
	if (error)
	{
		fz_free(stm);
		return error;
	}

	stm->file = open(path, realmode, 0666);
	if (stm->file < 0)
	{
		fz_dropbuffer(stm->buffer);
		fz_free(stm);
		return fz_throw("ioerror: open '%s' failed: %s", path, strerror(errno));
	}

	*stmp = stm;
	return nil;
}

static fz_error *
openfilter(fz_stream **stmp, fz_filter *flt, fz_stream *src, int mode)
{
	fz_error *error;
	fz_stream *stm;

	stm = newstm(FZ_SFILTER, mode);
	if (!stm)
		return fz_outofmem;

	error = fz_newbuffer(&stm->buffer, FZ_BUFSIZE);
	if (error)
	{
		fz_free(stm);
		return error;
	}

	stm->chain = fz_keepstream(src);
	stm->filter = fz_keepfilter(flt);

	*stmp = stm;
	return nil;
}

static fz_error *
openbuffer(fz_stream **stmp, fz_buffer *buf, int mode)
{
	fz_stream *stm;

	stm = newstm(FZ_SBUFFER, mode);
	if (!stm)
		return fz_outofmem;

	stm->buffer = fz_keepbuffer(buf);

	if (mode == FZ_SREAD)
		stm->buffer->eof = 1;

	*stmp = stm;
	return nil;
}

fz_error * fz_openrfile(fz_stream **stmp, char *path)
{
	return openfile(stmp, path, FZ_SREAD, O_BINARY | O_RDONLY);
}

fz_error * fz_openwfile(fz_stream **stmp, char *path)
{
	return openfile(stmp, path, FZ_SWRITE,
			O_BINARY | O_WRONLY | O_CREAT | O_TRUNC);
}

fz_error * fz_openafile(fz_stream **stmp, char *path)
{
	fz_error *error;
	int t;

	error = openfile(stmp, path, FZ_SWRITE, O_BINARY | O_WRONLY);
	if (error)
		return error;

	t = lseek((*stmp)->file, 0, 2);
	if (t < 0)
	{
		(*stmp)->dead = 1;
		return fz_throw("ioerror: lseek: %s", strerror(errno));
	}

	return nil;
}

fz_error * fz_openrfilter(fz_stream **stmp, fz_filter *flt, fz_stream *src)
{
	return openfilter(stmp, flt, src, FZ_SREAD);
}

fz_error * fz_openwfilter(fz_stream **stmp, fz_filter *flt, fz_stream *src)
{
	return openfilter(stmp, flt, src, FZ_SWRITE);
}

fz_error * fz_openrbuffer(fz_stream **stmp, fz_buffer *buf)
{
	return openbuffer(stmp, buf, FZ_SREAD);
}

fz_error * fz_openwbuffer(fz_stream **stmp, fz_buffer *buf)
{
	return openbuffer(stmp, buf, FZ_SWRITE);
}

fz_error * fz_openrmemory(fz_stream **stmp, char *mem, int len)
{
	fz_error *error;
	fz_buffer *buf;

	error = fz_newbufferwithmemory(&buf, mem, len);
	if (error)
		return error;

	error = fz_openrbuffer(stmp, buf);

	fz_dropbuffer(buf);

	return error;
}

