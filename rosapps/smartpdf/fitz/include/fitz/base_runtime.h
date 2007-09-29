#undef nil
#define nil ((void*)0)

#undef offsetof
#define offsetof(s, m) (unsigned long)(&(((s*)0)->m))

#undef nelem
#define nelem(x) (sizeof(x)/sizeof((x)[0]))

#undef ABS
#define ABS(x) ( (x) < 0 ? -(x) : (x) )

#undef MAX
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )

#undef MIN
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )

#undef CLAMP
#define CLAMP(x,a,b) ( (x) > (b) ? (b) : ( (x) < (a) ? (a) : (x) ) )

#define MAX4(a,b,c,d) MAX(MAX(a,b), MAX(c,d))
#define MIN4(a,b,c,d) MIN(MIN(a,b), MIN(c,d))

#define STRIDE(n, bcp) (((bpc) * (n) + 7) / 8)

/* plan9 stuff for utf-8 and path munging */
int chartorune(int *rune, char *str);
int runetochar(char *str, int *rune);
int runelen(long c);
int runenlen(int *r, int nrune);
int fullrune(char *str, int n);
char *cleanname(char *name);

typedef struct fz_error_s fz_error;

struct fz_error_s
{
	int refs;
	char msg[184];
	char file[32];
	char func[32];
	int line;
};

#define fz_outofmem (&fz_koutofmem)
extern fz_error fz_koutofmem;

#ifdef _WIN32
#define fz_throw(...) fz_throw0(__FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#elif HAVE_C99
#define fz_throw(...) fz_throw0(__func__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define fz_throw fz_throw1
#endif
fz_error *fz_throw0(const char *func, const char *file, int line, char *fmt, ...);
fz_error *fz_throw1(char *fmt, ...);

void fz_warn(char *fmt, ...);
void fz_droperror(fz_error *eo);

typedef struct fz_memorycontext_s fz_memorycontext;

struct fz_memorycontext_s
{
	void * (*malloc)(fz_memorycontext *, int);
	void * (*realloc)(fz_memorycontext *, void *, int);
	void (*free)(fz_memorycontext *, void *);
};

fz_memorycontext *fz_currentmemorycontext(void);
void fz_setmemorycontext(fz_memorycontext *memorycontext);

void *fz_malloc(int n);
void *fz_realloc(void *p, int n);
void fz_free(void *p);

char *fz_strdup(char *s);

