#include "fitz-base.h"

/* Make this thread local storage if you wish.  */

static void *stdmalloc(fz_memorycontext *mem, int n)
{
#if 0
	void *p = malloc(n);
	if (!p)
		fprintf(stderr, "failed to malloc %d bytes\n", n);
	return p;
#else
	return malloc(n);
#endif
}

static void *stdrealloc(fz_memorycontext *mem, void *p, int n)
{
#if 0
	void *np = realloc(p, n);
	if (np == nil)
		fprintf(stderr, "realloc failed %d nytes", n);
	else if (np == p)
		fprintf(stderr, "realloc kept %d\n", n);
	else
		fprintf(stderr, "realloc moved %d\n", n);
	return np;
#else
	return realloc(p, n);
#endif
}

static void stdfree(fz_memorycontext *mem, void *p)
{
	free(p);
}

static fz_memorycontext defmem = { stdmalloc, stdrealloc, stdfree };
static fz_memorycontext *curmem = &defmem;

fz_error fz_koutofmem = {
	-1,
	{"out of memory"}, 
	{"<malloc>"},
	{"memory.c"},
	0
};

fz_memorycontext *
fz_currentmemorycontext()
{
	return curmem;
}

void
fz_setmemorycontext(fz_memorycontext *mem)
{
	curmem = mem;
}

void *
fz_malloc(int n)
{
	fz_memorycontext *mem = fz_currentmemorycontext();
	return mem->malloc(mem, n);
}

void *
fz_realloc(void *p, int n)
{
	fz_memorycontext *mem = fz_currentmemorycontext();
	return mem->realloc(mem, p, n);
}

void
fz_free(void *p)
{
	fz_memorycontext *mem = fz_currentmemorycontext();
	mem->free(mem, p);
}

char *
fz_strdup(char *s)
{
	int len = strlen(s);
	char *ns = fz_malloc(len + 1);
	if (ns)
		strcpy(ns, s);
	return ns;
}

