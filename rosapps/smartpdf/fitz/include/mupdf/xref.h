/*
 * xref and object / stream api
 */

typedef struct pdf_xrefentry_s pdf_xrefentry;
typedef struct pdf_xref_s pdf_xref;

struct pdf_xref_s
{
	fz_stream *file;
	float version;
	int startxref;
	pdf_crypt *crypt;

	fz_obj *trailer;		/* TODO split this into root/info/encrypt/id */
	fz_obj *root;			/* resolved catalog dict */
	fz_obj *info;			/* resolved info dict */
	fz_obj *dests;			/* flattened dests nametree */

	int len;
	int cap;
	pdf_xrefentry *table;

	struct pdf_store_s *store;
	struct pdf_pagetree_s *pages;
	struct pdf_outline_s *outlines;
};

struct pdf_xrefentry_s
{
	unsigned int ofs;		/* file offset / objstm object number */
	unsigned short gen;		/* generation / objstm index */
	char type;				/* 0=unset (f)ree i(n)use (o)bjstm (d)elete (a)dd */
	char mark;				/* for garbage collection etc */
	fz_buffer *stmbuf;		/* in-memory stream */
	int stmofs;				/* on-disk stream */
	fz_obj *obj;			/* stored/cached object */
};

fz_error *pdf_newxref(pdf_xref **);
fz_error *pdf_repairxref(pdf_xref *, char *filename);
fz_error *pdf_loadxref(pdf_xref *, char *filename);
fz_error *pdf_initxref(pdf_xref *);

fz_error *pdf_openpdf(pdf_xref **, char *filename);
fz_error *pdf_updatexref(pdf_xref *, char *filename);
fz_error *pdf_savexref(pdf_xref *, char *filename, pdf_crypt *encrypt);

void pdf_debugxref(pdf_xref *);
void pdf_flushxref(pdf_xref *, int force);
void pdf_closexref(pdf_xref *);

fz_error *pdf_allocobject(pdf_xref *, int *oidp, int *genp);
fz_error *pdf_deleteobject(pdf_xref *, int oid, int gen);
fz_error *pdf_updateobject(pdf_xref *, int oid, int gen, fz_obj *obj);
fz_error *pdf_updatestream(pdf_xref *, int oid, int gen, fz_buffer *stm);

fz_error *pdf_cacheobject(pdf_xref *, int oid, int gen);
fz_error *pdf_loadobject(fz_obj **objp, pdf_xref *, int oid, int gen);
fz_error *pdf_loadindirect(fz_obj **objp, pdf_xref *, fz_obj *ref);
fz_error *pdf_resolve(fz_obj **reforobj, pdf_xref *);

int pdf_isstream(pdf_xref *xref, int oid, int gen);
fz_error *pdf_buildinlinefilter(fz_filter **filterp, fz_obj *stmobj);
fz_error *pdf_loadrawstream(fz_buffer **bufp, pdf_xref *xref, int oid, int gen);
fz_error *pdf_loadstream(fz_buffer **bufp, pdf_xref *xref, int oid, int gen);
fz_error *pdf_openrawstream(fz_stream **stmp, pdf_xref *, int oid, int gen);
fz_error *pdf_openstream(fz_stream **stmp, pdf_xref *, int oid, int gen);

fz_error *pdf_garbagecollect(pdf_xref *xref);
fz_error *pdf_transplant(pdf_xref *dst, pdf_xref *src, fz_obj **newp, fz_obj *old);

/* private */
fz_error *pdf_loadobjstm(pdf_xref *xref, int oid, int gen, char *buf, int cap);
fz_error *pdf_decryptxref(pdf_xref *xref);

