#include "config.h"

#include <stdio.h>
#include <signal.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <errno.h>

#include "slang.h"
#include "_slang.h"

/* This function will cause system calls to be restarted after signal if possible */
SLSig_Fun_Type *SLsignal (int sig, SLSig_Fun_Type *f)
{
#ifdef SLANG_POSIX_SIGNALS
   struct sigaction old_sa, new_sa;
   
# ifdef SIGALRM
   /* We want system calls to be interrupted by SIGALRM. */
   if (sig == SIGALRM) return SLsignal_intr (sig, f);
# endif

   sigemptyset (&new_sa.sa_mask);
   new_sa.sa_handler = f;
   
   new_sa.sa_flags = 0;
# ifdef SA_RESTART
   new_sa.sa_flags |= SA_RESTART;
# endif
   
   if (-1 == sigaction (sig, &new_sa, &old_sa))
     return (SLSig_Fun_Type *) SIG_ERR;
   
   return old_sa.sa_handler;
#else
   /* Not POSIX. */
   return signal (sig, f);
#endif
}

/* This function will NOT cause system calls to be restarted after 
 * signal if possible 
 */
SLSig_Fun_Type *SLsignal_intr (int sig, SLSig_Fun_Type *f)
{
#ifdef SLANG_POSIX_SIGNALS
   struct sigaction old_sa, new_sa;
   
   sigemptyset (&new_sa.sa_mask);
   new_sa.sa_handler = f;
   
   new_sa.sa_flags = 0;
# ifdef SA_INTERRUPT
   new_sa.sa_flags |= SA_INTERRUPT;
# endif
   
   if (-1 == sigaction (sig, &new_sa, &old_sa))
     return (SLSig_Fun_Type *) SIG_ERR;
   
   return old_sa.sa_handler;
#else
   /* Not POSIX. */
   return signal (sig, f);
#endif
}


/* We are primarily interested in blocking signals that would cause the 
 * application to reset the tty.  These include suspend signals and
 * possibly interrupt signals.
 */
#ifdef SLANG_POSIX_SIGNALS
static sigset_t Old_Signal_Mask;
#endif

static volatile unsigned int Blocked_Depth;

int SLsig_block_signals (void)
{
#ifdef SLANG_POSIX_SIGNALS
   sigset_t new_mask;
#endif
   
   Blocked_Depth++;
   if (Blocked_Depth != 1)
     {
	return 0;
     }
   
#ifdef SLANG_POSIX_SIGNALS
   sigemptyset (&new_mask);
# ifdef SIGQUIT
   sigaddset (&new_mask, SIGQUIT);
# endif
# ifdef SIGTSTP
   sigaddset (&new_mask, SIGTSTP);
# endif
# ifdef SIGINT
   sigaddset (&new_mask, SIGINT);
# endif
# ifdef SIGTTIN
   sigaddset (&new_mask, SIGTTIN);
# endif
# ifdef SIGTTOU
   sigaddset (&new_mask, SIGTTOU);
# endif
   
   (void) sigprocmask (SIG_BLOCK, &new_mask, &Old_Signal_Mask);
   return 0;
#else
   /* Not implemented. */
   return -1;
#endif
}

int SLsig_unblock_signals (void)
{
   if (Blocked_Depth == 0)
     return -1;
   
   Blocked_Depth--;
   
   if (Blocked_Depth != 0)
     return 0;
   
#ifdef SLANG_POSIX_SIGNALS
   (void) sigprocmask (SIG_SETMASK, &Old_Signal_Mask, NULL);
   return 0;
#else
   return -1;
#endif
}
