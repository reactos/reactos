/*
** Copyright (c) 2002-2016, Erik de Castro Lopo <erikd@mega-nerd.com>
** All rights reserved.
**
** This code is released under 2-clause BSD license. Please see the
** file at : https://github.com/erikd/libsamplerate/blob/master/COPYING
*/

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#ifdef HAVE_STDINT_H
#include <stdint.h>
#elif (SIZEOF_INT == 4)
typedef	int		int32_t ;
#elif (SIZEOF_LONG == 4)
typedef	long	int32_t ;
#endif

#define	SRC_MAX_RATIO			256
#define	SRC_MAX_RATIO_STR		"256"

#define	SRC_MIN_RATIO_DIFF		(1e-20)

#define	MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define	MIN(a,b)	(((a) < (b)) ? (a) : (b))

#define	ARRAY_LEN(x)			((int) (sizeof (x) / sizeof ((x) [0])))
#define OFFSETOF(type,member)	((int) (&((type*) 0)->member))

#define	MAKE_MAGIC(a,b,c,d,e,f)	((a) + ((b) << 4) + ((c) << 8) + ((d) << 12) + ((e) << 16) + ((f) << 20))

/*
** Inspiration : http://sourcefrog.net/weblog/software/languages/C/unused.html
*/
#ifdef UNUSED
#elif defined (__GNUC__)
#	define UNUSED(x) UNUSED_ ## x __attribute__ ((unused))
#elif defined (__LCLINT__)
#	define UNUSED(x) /*@unused@*/ x
#else
#	define UNUSED(x) x
#endif

#ifdef __GNUC__
#	define WARN_UNUSED	__attribute__ ((warn_unused_result))
#else
#	define WARN_UNUSED
#endif


#include "samplerate.h"

enum
{	SRC_FALSE	= 0,
	SRC_TRUE	= 1,

	SRC_MODE_PROCESS	= 555,
	SRC_MODE_CALLBACK	= 556
} ;

enum
{	SRC_ERR_NO_ERROR = 0,

	SRC_ERR_MALLOC_FAILED,
	SRC_ERR_BAD_STATE,
	SRC_ERR_BAD_DATA,
	SRC_ERR_BAD_DATA_PTR,
	SRC_ERR_NO_PRIVATE,
	SRC_ERR_BAD_SRC_RATIO,
	SRC_ERR_BAD_PROC_PTR,
	SRC_ERR_SHIFT_BITS,
	SRC_ERR_FILTER_LEN,
	SRC_ERR_BAD_CONVERTER,
	SRC_ERR_BAD_CHANNEL_COUNT,
	SRC_ERR_SINC_BAD_BUFFER_LEN,
	SRC_ERR_SIZE_INCOMPATIBILITY,
	SRC_ERR_BAD_PRIV_PTR,
	SRC_ERR_BAD_SINC_STATE,
	SRC_ERR_DATA_OVERLAP,
	SRC_ERR_BAD_CALLBACK,
	SRC_ERR_BAD_MODE,
	SRC_ERR_NULL_CALLBACK,
	SRC_ERR_NO_VARIABLE_RATIO,
	SRC_ERR_SINC_PREPARE_DATA_BAD_LEN,
	SRC_ERR_BAD_INTERNAL_STATE,

	/* This must be the last error number. */
	SRC_ERR_MAX_ERROR
} ;

typedef struct SRC_PRIVATE_tag
{	double	last_ratio, last_position ;

	int		error ;
	int		channels ;

	/* SRC_MODE_PROCESS or SRC_MODE_CALLBACK */
	int		mode ;

	/* Pointer to data to converter specific data. */
	void	*private_data ;

	/* Varispeed process function. */
	int		(*vari_process) (struct SRC_PRIVATE_tag *psrc, SRC_DATA *data) ;

	/* Constant speed process function. */
	int		(*const_process) (struct SRC_PRIVATE_tag *psrc, SRC_DATA *data) ;

	/* State reset. */
	void	(*reset) (struct SRC_PRIVATE_tag *psrc) ;

	/* Data specific to SRC_MODE_CALLBACK. */
	src_callback_t	callback_func ;
	void			*user_callback_data ;
	long			saved_frames ;
	const float		*saved_data ;
} SRC_PRIVATE ;

/* In src_sinc.c */
const char* sinc_get_name (int src_enum) ;
const char* sinc_get_description (int src_enum) ;

int sinc_set_converter (SRC_PRIVATE *psrc, int src_enum) ;

/* In src_linear.c */
const char* linear_get_name (int src_enum) ;
const char* linear_get_description (int src_enum) ;

int linear_set_converter (SRC_PRIVATE *psrc, int src_enum) ;

/* In src_zoh.c */
const char* zoh_get_name (int src_enum) ;
const char* zoh_get_description (int src_enum) ;

int zoh_set_converter (SRC_PRIVATE *psrc, int src_enum) ;

/*----------------------------------------------------------
**	Common static inline functions.
*/

static inline double
fmod_one (double x)
{	double res ;

	res = x - lrint (x) ;
	if (res < 0.0)
		return res + 1.0 ;

	return res ;
} /* fmod_one */

static inline int
is_bad_src_ratio (double ratio)
{	return (ratio < (1.0 / SRC_MAX_RATIO) || ratio > (1.0 * SRC_MAX_RATIO)) ;
} /* is_bad_src_ratio */


#endif	/* COMMON_H_INCLUDED */

