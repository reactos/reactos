#ifndef __MAD_H
#define __MAD_H

#ifdef HAVE_MAD
#   define INLINE 
#else
#   ifndef INLINE
#       define INLINE inline
#   endif
#endif

#ifdef HAVE_MAD

/* The Memory Allocation Debugging system */

/* GNU headers define this as macros */
#ifdef malloc
#   undef malloc
#endif

#ifdef calloc
#   undef calloc
#endif

#define malloc(x)	mad_alloc (x, __FILE__, __LINE__)
#define calloc(x, y)	mad_alloc (x * y, __FILE__, __LINE__)
#define realloc(x, y)	mad_realloc (x, y, __FILE__, __LINE__)
#define xmalloc(x, y)	mad_alloc (x, __FILE__, __LINE__)
#define strdup(x)	mad_strdup (x, __FILE__, __LINE__)
#define free(x)		mad_free (x, __FILE__, __LINE__)

void mad_check (char *file, int line);
void *mad_alloc (int size, char *file, int line);
void *mad_realloc (void *ptr, int newsize, char *file, int line);
char *mad_strdup (const char *s, char *file, int line);
void mad_free (void *ptr, char *file, int line);
void mad_finalize (char *file, int line);
#else

#define mad_finalize(x, y)
#define mad_check(file,line)

#endif /* HAVE_MAD */

#endif /* __MAD_H */
