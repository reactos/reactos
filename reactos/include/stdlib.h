/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_stdlib_h_
#define __dj_include_stdlib_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

//#include <sys/djtypes.h>
  
#define EXIT_FAILURE	1
#define EXIT_SUCCESS	0
#define MB_CUR_MAX	__dj_mb_cur_max
#ifndef NULL
#define NULL	0
#endif	
#define RAND_MAX	2147483647

#define _MAX_DRIVE 26
#define _MAX_FNAME 100 
#define	_MAX_EXT  100
#define _MAX_DIR 26
#define _MAX_PATH 250


extern int errno;
extern int __dj_mb_cur_max;

extern char ** environ; 


typedef struct {
  int quot;
  int rem;
} div_t;

typedef struct {
  long quot;
  long rem;
} ldiv_t;

#include <internal/types.h>
#ifndef _WCHAR_T_
#define _WCHAR_T_
#define _WCHAR_T
	typedef int wchar_t;
#endif


void	abort(void) __attribute__((noreturn));
int	abs(int _i);
int	atexit(void (*_func)(void));
double	atof(const char *_s);
int	atoi(const char *_s);
long	atol(const char *_s);
void *	bsearch(const void *_key, const void *_base, size_t _nelem,
		size_t _size, int (*_cmp)(const void *_ck, const void *_ce));
void *	calloc(size_t _nelem, size_t _size);
div_t	div(int _numer, int _denom);
void	exit(int _status) __attribute__((noreturn));
void	free(void *_ptr);
char *	getenv(const char *_name);
long	labs(long _i);
ldiv_t	ldiv(long _numer, long _denom);
void *	malloc(size_t _size);
int	mblen(const char *_s, size_t _n);
size_t	mbstowcs(wchar_t *_wcs, const char *_s, size_t _n);
int	mbtowc(wchar_t *_pwc, const char *_s, size_t _n);
void	qsort(void *_base, size_t _nelem, size_t _size,
	      int (*_cmp)(const void *_e1, const void *_e2));
int	rand(void);
void *	realloc(void *_ptr, size_t _size);
void	srand(unsigned _seed);
double	strtod(const char *_s, char **_endptr);
long	strtol(const char *_s, char **_endptr, int _base);
unsigned long	strtoul(const char *_s, char **_endptr, int _base);
int	system(const char *_s);
size_t	wcstombs(char *_s, const wchar_t *_wcs, size_t _n);
int	wctomb(char *_s, wchar_t _wchar);

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE

typedef struct {
  long long quot;
  long long rem;
} lldiv_t;

void *		alloca(size_t _size);
long double	_atold(const char *_s);
long long	atoll(const char *_s);
void		cfree(void *_ptr);
char *		getpass(const char *_prompt);
int		getlongpass(const char *_prompt, char *_buffer, int _max_len);
char *		itoa(int value, char *buffer, int radix);
long long	llabs(long long _i);
lldiv_t		lldiv(long long _numer, long long _denom);
int		putenv(const char *_val);
int		setenv(const char *_var, const char *_val, int _replace);
double		_strtold(const char *_s, char **_endptr);
long		strtoll(const char *_s, char **_endptr, int _base);
unsigned long 	strtoull(const char *_s, char **_endptr, int _base);
void		swab(const void *from, void *to, int nbytes);

#ifndef alloca
#define alloca __builtin_alloca
#endif

/* BSD Random Number Generator */
char  *	initstate (unsigned _seed, char *_arg_state, int _n);
char  *	setstate(char *_arg_state);
long	random(void);
int	srandom(int _seed);



#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_stdlib_h_ */
