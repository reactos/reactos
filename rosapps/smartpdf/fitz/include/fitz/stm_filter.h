/*
 * Data filters for encryption, compression and decompression.
 *
 * A filter has one method, process, that takes an input and an output buffer.
 *
 * It returns one of three statuses:
 *    ioneedin -- input buffer exhausted, please give me more data (wp-rp)
 *    ioneedout -- output buffer exhausted, please provide more space (ep-wp)
 *    iodone -- finished, please never call me again. ever!
 * or...
 *    any other error object -- oops, something blew up.
 *
 * To make using the filter easier, three variables are updated:
 *    produced -- if we actually produced any new data
 *    consumed -- like above
 *    count -- number of bytes produced in total since the beginning
 *
 * Most filters take fz_obj as a way to specify parameters.
 * In most cases, this is a dictionary that contains the same keys
 * that the corresponding PDF filter would expect.
 *
 * The pipeline filter is special, and needs some care when chaining
 * and unchaining new filters.
 */

typedef struct fz_filter_s fz_filter;

#define fz_ioneedin (&fz_kioneedin)
#define fz_ioneedout (&fz_kioneedout)
#define fz_iodone (&fz_kiodone)

extern fz_error fz_kioneedin;
extern fz_error fz_kioneedout;
extern fz_error fz_kiodone;

/*
 * Evil looking macro to create an initialize a filter struct.
 */

#define FZ_NEWFILTER(TYPE,VAR,NAME)                                     \
	fz_error * fz_process ## NAME (fz_filter*,fz_buffer*,fz_buffer*);   \
	void fz_drop ## NAME (fz_filter*);                                  \
	TYPE *VAR;                                                          \
	*fp = fz_malloc(sizeof(TYPE));                                      \
	if (!*fp) return fz_outofmem;                                       \
	(*fp)->refs = 1;                                                    \
	(*fp)->process = fz_process ## NAME ;                               \
	(*fp)->drop = fz_drop ## NAME ;                                     \
	(*fp)->consumed = 0;                                                \
	(*fp)->produced = 0;                                                \
	(*fp)->count = 0;                                                   \
	VAR = (TYPE*) *fp

struct fz_filter_s
{
	int refs;
	fz_error* (*process)(fz_filter *filter, fz_buffer *in, fz_buffer *out);
	void (*drop)(fz_filter *filter);
	int consumed;
	int produced;
	int count;
};

fz_error *fz_process(fz_filter *f, fz_buffer *in, fz_buffer *out);
fz_filter *fz_keepfilter(fz_filter *f);
void fz_dropfilter(fz_filter *f);

fz_error *fz_newpipeline(fz_filter **fp, fz_filter *head, fz_filter *tail);
fz_error *fz_chainpipeline(fz_filter **fp, fz_filter *head, fz_filter *tail, fz_buffer *buf);
void fz_unchainpipeline(fz_filter *pipe, fz_filter **oldfp, fz_buffer **oldbp);

/* stop and reverse! special case needed for postscript only */
void fz_pushbackahxd(fz_filter *filter, fz_buffer *in, fz_buffer *out, int n);

fz_error *fz_newnullfilter(fz_filter **fp, int len);
fz_error *fz_newarc4filter(fz_filter **fp, unsigned char *key, unsigned keylen);
fz_error *fz_newa85d(fz_filter **filterp, fz_obj *param);
fz_error *fz_newa85e(fz_filter **filterp, fz_obj *param);
fz_error *fz_newahxd(fz_filter **filterp, fz_obj *param);
fz_error *fz_newahxe(fz_filter **filterp, fz_obj *param);
fz_error *fz_newrld(fz_filter **filterp, fz_obj *param);
fz_error *fz_newrle(fz_filter **filterp, fz_obj *param);
fz_error *fz_newdctd(fz_filter **filterp, fz_obj *param);
fz_error *fz_newdcte(fz_filter **filterp, fz_obj *param);
fz_error *fz_newfaxd(fz_filter **filterp, fz_obj *param);
fz_error *fz_newfaxe(fz_filter **filterp, fz_obj *param);
fz_error *fz_newflated(fz_filter **filterp, fz_obj *param);
fz_error *fz_newflatee(fz_filter **filterp, fz_obj *param);
fz_error *fz_newlzwd(fz_filter **filterp, fz_obj *param);
fz_error *fz_newlzwe(fz_filter **filterp, fz_obj *param);
fz_error *fz_newpredictd(fz_filter **filterp, fz_obj *param);
fz_error *fz_newpredicte(fz_filter **filterp, fz_obj *param);
fz_error *fz_newjbig2d(fz_filter **filterp, fz_obj *param);
fz_error *fz_newjpxd(fz_filter **filterp, fz_obj *param);

