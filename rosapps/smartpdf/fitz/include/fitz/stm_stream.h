/*
 * Stream API for Fitz.
 * Read and write data to and from files, memory buffers and filters.
 */

typedef struct fz_stream_s fz_stream;

enum { FZ_SFILE, FZ_SBUFFER, FZ_SFILTER };
enum { FZ_SREAD, FZ_SWRITE };

struct fz_stream_s
{
	int refs;
	int kind;
	int mode;
	int dead;
	fz_buffer *buffer;
	fz_filter *filter;
	fz_stream *chain;
	fz_error *error;
	int file;
};

/*
 * Various stream creation functions.
 */

/* open() and creat() & co */
fz_error *fz_openrfile(fz_stream **stmp, char *filename);
fz_error *fz_openwfile(fz_stream **stmp, char *filename);
fz_error *fz_openafile(fz_stream **stmp, char *filename);

/* write to memory buffers! */
fz_error *fz_openrmemory(fz_stream **stmp, char *buf, int len);
fz_error *fz_openrbuffer(fz_stream **stmp, fz_buffer *buf);
fz_error *fz_openwbuffer(fz_stream **stmp, fz_buffer *buf);

/* almost like fork() exec() pipe() */
fz_error *fz_openrfilter(fz_stream **stmp, fz_filter *flt, fz_stream *chain);
fz_error *fz_openwfilter(fz_stream **stmp, fz_filter *flt, fz_stream *chain);

/*
 * Functions that are common to both input and output streams.
 */

fz_error *fz_ioerror(fz_stream *stm);

fz_stream *fz_keepstream(fz_stream *stm);
void fz_dropstream(fz_stream *stm);

int fz_tell(fz_stream *stm);
int fz_seek(fz_stream *stm, int offset, int whence);

/*
 * Input stream functions.
 * Return EOF (-1) on errors.
 */

int fz_rtell(fz_stream *stm);
int fz_rseek(fz_stream *stm, int offset, int whence);

int fz_makedata(fz_stream *stm);
int fz_read(fz_stream *stm, unsigned char *buf, int len);

int fz_readall(fz_buffer **bufp, fz_stream *stm);
int fz_readline(fz_stream *stm, char *buf, int max);

int fz_readbytex(fz_stream *stm);
int fz_peekbytex(fz_stream *stm);

#ifdef DEBUG
#define fz_readbyte fz_readbytex
#define fz_peekbyte fz_peekbytex
#else

#define FZ_READBYTE(XXX) { \
	fz_buffer *buf = stm->buffer; \
	if (buf->rp == buf->wp) \
		if (fz_makedata(stm) < 0) \
			return EOF; \
	return buf->rp < buf->wp ? XXX : EOF ; \
}

static inline int fz_readbyte(fz_stream *stm) FZ_READBYTE(*buf->rp++)
static inline int fz_peekbyte(fz_stream *stm) FZ_READBYTE(*buf->rp)

#endif

/*
 * Output stream functions.
 * Return N or 0 on success, -1 on failure.
 */

int fz_wtell(fz_stream *stm);
int fz_wseek(fz_stream *stm, int offset, int whence);

int fz_write(fz_stream *stm, unsigned char *buf, int n);
int fz_flush(fz_stream *stm);

int fz_printstr(fz_stream *stm, char *s);
int fz_printobj(fz_stream *stm, fz_obj *obj, int tight);
int fz_print(fz_stream *stm, char *fmt, ...);

