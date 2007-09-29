#include "fitz-base.h"
#include "fitz-stream.h"

#define noDEBUG 1

typedef struct fz_pipeline_s fz_pipeline;

fz_error * fz_processpipeline(fz_filter *filter, fz_buffer *in, fz_buffer *out);

struct fz_pipeline_s
{
	fz_filter super;
	fz_filter *head;
	fz_buffer *buffer;
	fz_filter *tail;
	int tailneedsin;
};

fz_error *
fz_chainpipeline(fz_filter **fp, fz_filter *head, fz_filter *tail, fz_buffer *buf)
{
	FZ_NEWFILTER(fz_pipeline, p, pipeline);
	p->head = fz_keepfilter(head);
	p->tail = fz_keepfilter(tail);
	p->tailneedsin = 1;
	p->buffer = fz_keepbuffer(buf);
	return nil;
}

void
fz_unchainpipeline(fz_filter *filter, fz_filter **oldfp, fz_buffer **oldbp)
{
	fz_pipeline *p = (fz_pipeline*)filter;

	*oldfp = fz_keepfilter(p->head);
	*oldbp = fz_keepbuffer(p->buffer);

	fz_dropfilter(filter);
}

fz_error *
fz_newpipeline(fz_filter **fp, fz_filter *head, fz_filter *tail)
{
	fz_error *error;

	FZ_NEWFILTER(fz_pipeline, p, pipeline);
	p->head = fz_keepfilter(head);
	p->tail = fz_keepfilter(tail);
	p->tailneedsin = 1;

	error = fz_newbuffer(&p->buffer, FZ_BUFSIZE);
	if (error) { fz_free(p); return error; }

	return nil;
}

void
fz_droppipeline(fz_filter *filter)
{
	fz_pipeline *p = (fz_pipeline*)filter;
	fz_dropfilter(p->head);
	fz_dropfilter(p->tail);
	fz_dropbuffer(p->buffer);
}

fz_error *
fz_processpipeline(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	fz_pipeline *p = (fz_pipeline*)filter;
	fz_error *e;

	if (p->buffer->eof)
		goto tail;

	if (p->tailneedsin && p->head->produced)
		goto tail;

head:
	e = fz_process(p->head, in, p->buffer);

	if (e == fz_ioneedin)
		return fz_ioneedin;

	else if (e == fz_ioneedout)
	{
		if (p->tailneedsin && !p->head->produced)
		{
			fz_error *be = nil;
			if (p->buffer->rp > p->buffer->bp)
				be = fz_rewindbuffer(p->buffer);
			else
				be = fz_growbuffer(p->buffer);
			if (be)
				return be;
			goto head;
		}
		goto tail;
	}

	else if (e == fz_iodone)
		goto tail;

	else
		return e;

tail:
	e = fz_process(p->tail, p->buffer, out);

	if (e == fz_ioneedin)
	{
		if (p->buffer->eof)
			return fz_throw("ioerror: premature eof in pipeline");
		p->tailneedsin = 1;
		goto head;
	}

	else if (e == fz_ioneedout)
	{
		p->tailneedsin = 0;
		return fz_ioneedout;
	}

	else if (e == fz_iodone)
		return fz_iodone;

	else
		return e;
}

