/* Systems having sys/param.h should not include this file */

#ifdef HAVE_PARAM_H
#error Remove this file if you have real sys/param.h
#else
/* FIXME: We should warn, that this file should not be included */
#endif
