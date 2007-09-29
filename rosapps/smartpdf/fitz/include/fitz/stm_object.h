/*
 * Dynamic objects.
 * The same type of objects as found in PDF and PostScript.
 * Used by the filter library and the mupdf parser.
 */

typedef struct fz_obj_s fz_obj;
typedef struct fz_keyval_s fz_keyval;

typedef enum fz_objkind_e
{
	FZ_NULL,
	FZ_BOOL,
	FZ_INT,
	FZ_REAL,
	FZ_STRING,
	FZ_NAME,
	FZ_ARRAY,
	FZ_DICT,
	FZ_INDIRECT,
	FZ_POINTER
} fz_objkind;

struct fz_keyval_s
{
	fz_obj *k;
	fz_obj *v;
};

struct fz_obj_s
{
	unsigned short refs;
	char kind;				/* fz_objkind takes 4 bytes :( */
	union
	{
		int b;
		int i;
		float f;
		struct {
			unsigned short len;
			char buf[1];
		} s;
		char n[1];
		struct {
			int len;
			int cap;
			fz_obj **items;
		} a;
		struct {
			char sorted;
			int len;
			int cap;
			fz_keyval *items;
		} d;
		struct {
			int oid;
			int gid;
		} r;
		void *p;
	} u;
};

fz_error *fz_newnull(fz_obj **op);
fz_error *fz_newbool(fz_obj **op, int b);
fz_error *fz_newint(fz_obj **op, int i);
fz_error *fz_newreal(fz_obj **op, float f);
fz_error *fz_newname(fz_obj **op, char *str);
fz_error *fz_newstring(fz_obj **op, char *str, int len);
fz_error *fz_newindirect(fz_obj **op, int oid, int gid);
fz_error *fz_newpointer(fz_obj **op, void *p);

fz_error *fz_newarray(fz_obj **op, int initialcap);
fz_error *fz_newdict(fz_obj **op, int initialcap);
fz_error *fz_copyarray(fz_obj **op, fz_obj *array);
fz_error *fz_copydict(fz_obj **op, fz_obj *dict);
fz_error *fz_deepcopyarray(fz_obj **op, fz_obj *array);
fz_error *fz_deepcopydict(fz_obj **op, fz_obj *dict);

fz_obj *fz_keepobj(fz_obj *obj);
void fz_dropobj(fz_obj *obj);

/* type queries */
int fz_isnull(fz_obj *obj);
int fz_isbool(fz_obj *obj);
int fz_isint(fz_obj *obj);
int fz_isreal(fz_obj *obj);
int fz_isname(fz_obj *obj);
int fz_isstring(fz_obj *obj);
int fz_isarray(fz_obj *obj);
int fz_isdict(fz_obj *obj);
int fz_isindirect(fz_obj *obj);
int fz_ispointer(fz_obj *obj);

int fz_objcmp(fz_obj *a, fz_obj *b);

/* silent failure, no error reporting */
int fz_tobool(fz_obj *obj);
int fz_toint(fz_obj *obj);
float fz_toreal(fz_obj *obj);
char *fz_toname(fz_obj *obj);
char *fz_tostrbuf(fz_obj *obj);
int fz_tostrlen(fz_obj *obj);
int fz_tonum(fz_obj *obj);
int fz_togen(fz_obj *obj);
void *fz_topointer(fz_obj *obj);

fz_error *fz_newnamefromstring(fz_obj **op, fz_obj *str);

int fz_arraylen(fz_obj *array);
fz_obj *fz_arrayget(fz_obj *array, int i);
fz_error *fz_arrayput(fz_obj *array, int i, fz_obj *obj);
fz_error *fz_arraypush(fz_obj *array, fz_obj *obj);

int fz_dictlen(fz_obj *dict);
fz_obj *fz_dictgetkey(fz_obj *dict, int idx);
fz_obj *fz_dictgetval(fz_obj *dict, int idx);
fz_obj *fz_dictget(fz_obj *dict, fz_obj *key);
fz_obj *fz_dictgets(fz_obj *dict, char *key);
fz_obj *fz_dictgetsa(fz_obj *dict, char *key, char *abbrev);
fz_error *fz_dictput(fz_obj *dict, fz_obj *key, fz_obj *val);
fz_error *fz_dictputs(fz_obj *dict, char *key, fz_obj *val);
fz_error *fz_dictdel(fz_obj *dict, fz_obj *key);
fz_error *fz_dictdels(fz_obj *dict, char *key);
void fz_sortdict(fz_obj *dict);

int fz_sprintobj(char *s, int n, fz_obj *obj, int tight);
void fz_debugobj(fz_obj *obj);

fz_error *fz_parseobj(fz_obj **objp, char *s);
fz_error *fz_packobj(fz_obj **objp, char *fmt, ...);
fz_error *fz_unpackobj(fz_obj *obj, char *fmt, ...);

