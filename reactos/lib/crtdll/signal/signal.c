/* Copyright (C) 1994, 1995 Charles Sandmann (sandmann@clio.rice.edu)
   Exception handling and basis for signal support for DJGPP V2.0
   This software may be freely distributed, no warranty. */

#include <crtdll/signal.h>
#include <crtdll/stdlib.h>

//extern unsigned end __asm__ ("end");

void __djgpp_traceback_exit(int);

static _p_sig_fn_t signal_list[SIGMAX];	/* SIG_DFL = 0 */

_p_sig_fn_t	signal(int sig, _p_sig_fn_t func)
{
  _p_sig_fn_t temp;
  if(sig <= 0 || sig > SIGMAX || sig == SIGKILL)
  {
    //errno = EINVAL;
    return SIG_ERR;
  }
  temp = signal_list[sig - 1];
  signal_list[sig - 1] = func;
  return temp;
}


int
raise(int sig)
{
#if 0
  _p_sig_fn_t temp;

  if(sig <= 0)
    return -1;
  if(sig > SIGMAX)
    return -1;
  temp = signal_list[sig - 1];
  if(temp == (_p_sig_fn_t)SIG_IGN
     || (sig == SIGQUIT && temp == (_p_sig_fn_t)SIG_DFL))
    return 0;			/* Ignore it */
  if(temp == (_p_sig_fn_t)SIG_DFL)
    __djgpp_traceback_exit(sig); /* this does not return */
  else if((unsigned)temp < 4096 || temp > (_p_sig_fn_t)&end)
  {
   // err("Bad signal handler, ");
    __djgpp_traceback_exit(sig); /* does not return */
  }
  else
    temp(sig);

#endif
  return 0;
}

void __djgpp_traceback_exit(int sig)
{
	_exit(3);
}
