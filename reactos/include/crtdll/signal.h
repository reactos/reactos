/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_signal_h_
#define __dj_include_signal_h_

#ifdef __cplusplus
extern "C" {
#endif


/* 256 software interrupts + 32 exceptions = 288 */

#define SIGABRT	288
#define SIGFPE	289
#define SIGILL	290
#define SIGSEGV	291
#define SIGTERM	292
#define SIGINT  295

#define SIG_DFL ((void (*)(int))(0))
#define SIG_ERR	((void (*)(int))(1))
#define SIG_IGN	((void (*)(int))(-1))


typedef int sig_atomic_t;

int	raise(int _sig);
void	(*signal(int _sig, void (*_func)(int)))(int);
  


#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_signal_h_ */
