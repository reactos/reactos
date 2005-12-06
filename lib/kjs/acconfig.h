/*
 * Define the PACKAGE and VERSION only if they are undefined.  This means
 * that we do not redefine them, when the library is used in another
 * GNU like package that defines PACKAGE and VERSION.
 */

/* Package name. */
#ifndef PACKAGE
#undef PACKAGE
#endif /* no PACKAGE */

/* Version number. */
#ifndef VERSION
#undef VERSION
#endif /* no VERSION */

/* Are C prototypes supported. */
#undef PROTOTYPES

/* Canonical host name and its parts. */
#undef CANONICAL_HOST
#undef CANONICAL_HOST_CPU
#undef CANONICAL_HOST_VENDOR
#undef CANONICAL_HOST_OS

/* Do we want to build all instruction dispatchers? */
#undef ALL_DISPATCHERS

/* Do we want to profile byte-code operands. */
#undef PROFILING

/* Do we want the byte-code operand hooks. */
#undef BC_OPERAND_HOOKS

/*
 * Unconditionall disable the jumps byte-code instruction dispatch
 * method.
 */
#undef DISABLE_JUMPS

/* Does the struct stat has st_blksize member? */
#undef HAVE_STAT_ST_ST_BLKSIZE

/* Does the struct stat has st_blocks member? */
#undef HAVE_STAT_ST_ST_BLOCKS


/*
 * Posix threads features.
 */

/* Does the asctime_r() function take three arguments. */
#undef ASCTIME_R_WITH_THREE_ARGS

/* Does the drand48_r() work with DRAND48D data. */
#undef DRAND48_R_WITH_DRAND48D

/* How the attribute structures are passed to the init functions. */
#undef CONDATTR_BY_VALUE
#undef MUTEXATTR_BY_VALUE
#undef THREADATTR_BY_VALUE

/*
 * Extensions.
 */

/* JS */
#undef WITH_JS

/* Curses. */
#undef WITH_CURSES
#undef HAVE_CURSES_H
#undef HAVE_NCURSES_H

/* MD5 */
#undef WITH_MD5
