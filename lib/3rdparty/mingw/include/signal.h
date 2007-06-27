/* 
 * signal.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * A way to set handlers for exceptional conditions (also known as signals).
 *
 */

#ifndef	_SIGNAL_H_
#define	_SIGNAL_H_

/* All the headers include this file. */
#include <_mingw.h>

/*
 * The actual signal values. Using other values with signal
 * produces a SIG_ERR return value.
 *
 * NOTE: SIGINT is produced when the user presses Ctrl-C.
 *       SIGILL has not been tested.
 *       SIGFPE doesn't seem to work?
 *       SIGSEGV does not catch writing to a NULL pointer (that shuts down
 *               your app; can you say "segmentation violation core dump"?).
 *       SIGTERM comes from what kind of termination request exactly?
 *       SIGBREAK is indeed produced by pressing Ctrl-Break.
 *       SIGABRT is produced by calling abort.
 * TODO: The above results may be related to not installing an appropriate
 *       structured exception handling frame. Results may be better if I ever
 *       manage to get the SEH stuff down.
 */
#define	SIGINT		2	/* Interactive attention */
#define	SIGILL		4	/* Illegal instruction */
#define	SIGFPE		8	/* Floating point error */
#define	SIGSEGV		11	/* Segmentation violation */
#define	SIGTERM		15	/* Termination request */
#define SIGBREAK	21	/* Control-break */
#define	SIGABRT		22	/* Abnormal termination (abort) */

#define NSIG 23     /* maximum signal number + 1 */

#ifndef	RC_INVOKED

#ifndef _SIG_ATOMIC_T_DEFINED
typedef int sig_atomic_t;
#define _SIG_ATOMIC_T_DEFINED
#endif

/*
 * The prototypes (below) are the easy part. The hard part is figuring
 * out what signals are available and what numbers they are assigned
 * along with appropriate values of SIG_DFL and SIG_IGN.
 */

/*
 * A pointer to a signal handler function. A signal handler takes a
 * single int, which is the signal it handles.
 */
typedef	void (*__p_sig_fn_t)(int);

/*
 * These are special values of signal handler pointers which are
 * used to send a signal to the default handler (SIG_DFL), ignore
 * the signal (SIG_IGN), indicate an error return (SIG_ERR),
 * get an error (SIG_SGE), or acknowledge (SIG_ACK).
 */
#define	SIG_DFL	((__p_sig_fn_t) 0)
#define	SIG_IGN	((__p_sig_fn_t) 1)
#define	SIG_ERR ((__p_sig_fn_t) -1)
#define SIG_SGE ((__p_sig_fn_t) 3)
#define SIG_ACK ((__p_sig_fn_t) 4)

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Call signal to set the signal handler for signal sig to the
 * function pointed to by handler. Returns a pointer to the
 * previous handler, or SIG_ERR if an error occurs. Initially
 * unhandled signals defined above will return SIG_DFL.
 */
_CRTIMP __p_sig_fn_t __cdecl	signal(int, __p_sig_fn_t);

/*
 * Raise the signal indicated by sig. Returns non-zero on success.
 */
_CRTIMP int __cdecl	raise (int);

#ifdef	__cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* Not _SIGNAL_H_ */

