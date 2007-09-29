/*
 * Data buffers for streams and filters.
 *
 *   bp is the pointer to the allocated memory
 *   rp is read-position (*in->rp++ to read data)
 *   wp is write-position (*out->wp++ to write data)
 *   ep is the sentinel
 *
 * Only the data between rp and wp is valid data.
 *
 * Writers set eof to true at the end.
 * Readers look at eof.
 *
 * A buffer owns the memory it has allocated, unless ownsdata is false,
 * in which case the creator of the buffer owns it.
 */

typedef struct fz_buffer_s fz_buffer;

#define FZ_BUFSIZE (8 * 1024)

struct fz_buffer_s
{
	int refs;
	int ownsdata;
	unsigned char *bp;
	unsigned char *rp;
	unsigned char *wp;
	unsigned char *ep;
	int eof;
};

fz_error *fz_newbuffer(fz_buffer **bufp, int size);
fz_error *fz_newbufferwithmemory(fz_buffer **bufp, unsigned char *data, int size);

fz_error *fz_rewindbuffer(fz_buffer *buf);
fz_error *fz_growbuffer(fz_buffer *buf);

fz_buffer *fz_keepbuffer(fz_buffer *buf);
void fz_dropbuffer(fz_buffer *buf);

