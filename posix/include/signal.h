/*
 * signal.h
 *
 * signals. Conforming to the Single UNIX(r) Specification Version 2,
 * System Interface & Headers Issue 5
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __SIGNAL_H_INCLUDED__
#define __SIGNAL_H_INCLUDED__

/* INCLUDES */
#include <time.h>
#include <sys/types.h>

/* OBJECTS */

/* TYPES */
/* pre-declaration of time.h types to suppress warnings caused by circular
   dependencies */
struct timespec;

typedef int sig_atomic_t; /* Integral type of an object that can be
                      			     accessed as an atomic entity, even in the
                              presence of asynchronous interrupts */ /* FIXME? */

typedef struct __tagsigset_t
{
 int _dummy;
} sigset_t; /* Integral or structure type of an object used to represent
               sets of signals. */ /* TODO */

union sigval
{
 int    sival_int;    /* integer signal value */
 void*  sival_ptr;    /* pointer signal value */
};

struct sigevent
{
 int                   sigev_notify;           /* notification type */
 int                   sigev_signo;            /* signal number */
 union sigval          sigev_value;            /* signal value */
 void (* sigev_notify_function)(union sigval); /* notification function */
 pthread_attr_t *     sigev_notify_attributes; /* notification attributes */
};


typedef struct __tagsiginfo_t
{
 int           si_signo;  /* signal number */
 int           si_errno;  /* if non-zero, an errno value associated with
                             this signal, as defined in <errno.h> */
 int           si_code;   /* signal code */
 pid_t         si_pid;    /* sending process ID */
 uid_t         si_uid;    /* real user ID of sending process */
 void         *si_addr;   /* address of faulting instruction */
 int           si_status; /* exit value or signal */
 long          si_band;   /* band event for SIGPOLL */
 union sigval  si_value;  /* signal value */
} siginfo_t;

struct sigaction
{
 void     (* sa_handler)(int); /* what to do on receipt of signal */
 sigset_t   sa_mask;           /* set of signals to be blocked during
                                  execution of the signal handling
				  function */
 int        sa_flags;          /* special flags */
 void (* sa_sigaction)(int, siginfo_t *, void *);
                               /* pointer to signal handler function
                                  or one of the macros SIG_IGN or SIG_DFL */
};

typedef struct __tagstack_t
{
 void   *ss_sp;   /* stack base or pointer */
 size_t ss_size;  /* stack size */
 int    ss_flags; /* flags */
} stack_t;

struct sigstack
{
 int  ss_onstack; /* non-zero when signal stack is in use */
 void *ss_sp;     /* signal stack pointer */
};

/* CONSTANTS */
/* Request for default signal handling. */
#define SIG_DFL  ((void (*)(int))(0))
/* Return value from signal() in case of error. */
#define SIG_ERR  ((void (*)(int))(1))
/* Request that signal be held. */
#define SIG_HOLD ((void (*)(int))(2))
/* Request that signal be ignored. */
#define SIG_IGN  ((void (*)(int))(3))

/* No asynchronous notification will be delivered when the event of
   interest occurs. */
#define SIGEV_NONE   (0)
/* A queued signal, with an application-defined value, will be generated
   when the event of interest occurs. */
#define SIGEV_SIGNAL (1)
/* A notification function will be called to perform notification. */
#define SIGEV_THREAD (2)

/* TODO: realtime features not supported yet */
#define SIGRTMIN (-1)
#define SIGRTMAX (-1)

/* Process abort signal. */
#define SIGABRT   ( 1)
/* Alarm clock. */
#define SIGALRM   ( 2)
/* Erroneous arithmetic operation. */
#define SIGFPE    ( 3)
/* Hangup. */
#define SIGHUP    ( 4)
/* Illegal instruction. */
#define SIGILL    ( 5)
/* Terminal interrupt signal. */
#define SIGINT    ( 6)
/* Kill (cannot be caught or ignored). */
#define SIGKILL   ( 7)
/* Write on a pipe with no one to read it. */
#define SIGPIPE   ( 8)
/* Terminal quit signal. */
#define SIGQUIT   ( 9)
/* Invalid memory reference. */
#define SIGSEGV   (10)
/* Termination signal. */
#define SIGTERM   (11)
/* User-defined signal 1. */
#define SIGUSR1   (12)
/* User-defined signal 2. */
#define SIGUSR2   (13)
/* Child process terminated or stopped. */
#define SIGCHLD   (14)
/* Continue executing, if stopped. */
#define SIGCONT   (15)
/* Stop executing (cannot be caught or ignored). */
#define SIGSTOP   (16)
/* Terminal stop signal. */
#define SIGTSTP   (17)
/* Background process attempting read. */
#define SIGTTIN   (18)
/* Background process attempting write. */
#define SIGTTOU   (19)
/* Access to an undefined portion of a memory object. */
#define SIGBUS    (20)
/* Pollable event. */
#define SIGPOLL   (21)
/* Profiling timer expired. */
#define SIGPROF   (22)
/* Bad system call. */
#define SIGSYS    (23)
/* Trace/breakpoint trap. */
#define SIGTRAP   (24)
/* High bandwidth data is available at a socket. */
#define SIGURG    (25)
/* Virtual timer expired. */
#define SIGVTALRM (26)
/* CPU time limit exceeded. */
#define SIGXCPU   (27)
/* File size limit exceeded. */
#define SIGXFSZ   (28)

/* FIXME: the following constants need to be reviewed */
/* Do not generate SIGCHLD when children stop. */
#define SA_NOCLDSTOP (0x00000001)
/* The resulting set is the union of the current set and the signal set
   pointed to by the argument set. */
#define SA_ONSTACK   (0x00000002)
/* Causes signal dispositions to be set to SIG_DFL on entry to signal
   handlers. */
#define SA_RESETHAND (0x00000004)
/* Causes certain functions to become restartable. */
#define SA_RESTART   (0x00000008)
/* Causes extra information to be passed to signal handlers at the time
   of receipt of a signal. */
#define SA_SIGINFO   (0x00000010)
/* Causes implementations not to create zombie processes on child death. */
#define SA_NOCLDWAIT (0x00000020)
/* Causes signal not to be automatically blocked on entry to signal
   handler. */
#define SA_NODEFER   (0x00000040)

/* FIXME: the following constants need to be reviewed */
/* The resulting set is the intersection of the current set and the
   complement of the signal set pointed to by the argument set. */
#define SIG_BLOCK   (1)
/* The resulting set is the signal set pointed to by the argument
   set. */
#define SIG_UNBLOCK (2)
/* Causes signal delivery to occur on an alternate stack. */
#define SIG_SETMASK (3)

/* FIXME: the following constants need to be reviewed */
/* Process is executing on an alternate signal stack. */
#define SS_ONSTACK (1)
/* Alternate signal stack is disabled. */
#define SS_DISABLE (2)

/* Minimum stack size for a signal handler. */ /* FIXME */
#define MINSIGSTKSZ (0)
/* Default size in bytes for the alternate signal stack. */ /* FIXME */
#define SIGSTKSZ    (0)

/*
 signal-specific reasons why the signal was generated
 */
/* SIGILL */
/* illegal opcode */
#define ILL_ILLOPC (1)
/* illegal operand */
#define ILL_ILLOPN (2)
/* illegal addressing mode */
#define ILL_ILLADR (3)
/* illegal trap */
#define ILL_ILLTRP (4)
/* privileged opcode */
#define ILL_PRVOPC (5)
/* privileged register */
#define ILL_PRVREG (6)
/* coprocessor error */
#define ILL_COPROC (7)
/* internal stack error */
#define ILL_BADSTK (8)

/* SIGFPE */
/* integer divide by zero */
#define FPE_INTDIV
/* integer overflow */
#define FPE_INTOVF
/* floating point divide by zero */
#define FPE_FLTDIV
/* floating point overflow */
#define FPE_FLTOVF
/* floating point underflow */
#define FPE_FLTUND
/* floating point inexact result */
#define FPE_FLTRES
/* invalid floating point operation */
#define FPE_FLTINV
/* subscript out of range */
#define FPE_FLTSUB

/* SIGSEGV */
/* address not mapped to object */
#define SEGV_MAPERR
/* invalid permissions for mapped object */
#define SEGV_ACCERR

/* SIGBUS */
/* invalid address alignment */
#define BUS_ADRALN
/* non-existent physical address */
#define BUS_ADRERR
/* object specific hardware error */
#define BUS_OBJERR

/* SIGTRAP */
/* process breakpoint */
#define TRAP_BRKPT
/* process trace trap */
#define TRAP_TRACE

/* SIGCHLD */
/* child has exited */
#define CLD_EXITED
/* child has terminated abnormally and did not create a core file */
#define CLD_KILLED
/* child has terminated abnormally and created a core file */
#define CLD_DUMPED
/* traced child has trapped */
#define CLD_TRAPPED
/* child has stopped */
#define CLD_STOPPED
/* stopped child has continued */
#define CLD_CONTINUED

/* SIGPOLL */
/* data input available */
#define POLL_IN
/* output buffers available */
#define POLL_OUT
/* input message available */
#define POLL_MSG
/* I/O error */
#define POLL_ERR
/* high priority input available */
#define POLL_PRI
/* device disconnected */
#define POLL_HUP
/* signal sent by kill() */
#define SI_USER
/* signal sent by the sigqueue() */
#define SI_QUEUE
/* signal generated by expiration of a timer set by timer_settime() */
#define SI_TIMER
/* signal generated by completion of an asynchronous I/O request */
#define SI_ASYNCIO
/* signal generated by arrival of a message on an empty message queue */
#define SI_MESGQ

/* PROTOTYPES */
void (*bsd_signal(int, void (*)(int)))(int);
int    kill(pid_t, int);
int    killpg(pid_t, int);
int    pthread_kill(pthread_t, int);
int    pthread_sigmask(int, const sigset_t *, sigset_t *);
int    raise(int);
int    sigaction(int, const struct sigaction *, struct sigaction *);
int    sigaddset(sigset_t *, int);
int    sigaltstack(const stack_t *, stack_t *);
int    sigdelset(sigset_t *, int);
int    sigemptyset(sigset_t *);
int    sigfillset(sigset_t *);
int    sighold(int);
int    sigignore(int);
int    siginterrupt(int, int);
int    sigismember(const sigset_t *, int);
void (*signal(int, void (*)(int)))(int);
int    sigpause(int);
int    sigpending(sigset_t *);
int    sigprocmask(int, const sigset_t *, sigset_t *);
int    sigqueue(pid_t, int, const union sigval);
int    sigrelse(int);
void (*sigset(int, void (*)(int)))(int);
int    sigstack(struct sigstack *ss,
           struct sigstack *oss); /* LEGACY */
int    sigsuspend(const sigset_t *);
int    sigtimedwait(const sigset_t *, siginfo_t *,
           const struct timespec *);
int    sigwait(const sigset_t *set, int *sig);
int    sigwaitinfo(const sigset_t *, siginfo_t *);

/* MACROS */

#endif /* __SIGNAL_H_INCLUDED__ */

/* EOF */

