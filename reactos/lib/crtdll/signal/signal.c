#include <crtdll/signal.h>
#include <crtdll/stdlib.h>
#include <crtdll/errno.h>
#include <crtdll/string.h>
#include <crtdll/internal/file.h>

void _default_handler(int signal);

typedef struct _sig_element 
{
	int signal;
	char *signame;
	_p_sig_fn_t handler;
} sig_element;

static sig_element signal_list[SIGMAX] =
  {
    { 0, "Signal 0", SIG_DFL },
    { SIGABRT, "Aborted",SIG_DFL }, 
    { SIGFPE, "Erroneous arithmetic operation",SIG_DFL },
    { SIGILL, "Illegal instruction",SIG_DFL },
    { SIGINT, "Interrupt",SIG_DFL },
    { SIGSEGV, "Invalid access to storage",SIG_DFL },
    { SIGTERM, "Terminated",SIG_DFL },
    { SIGHUP, "Hangup",SIG_DFL },
    { SIGQUIT, "Quit",SIG_DFL },
    { SIGPIPE, "Broken pipe",SIG_DFL },
    { SIGKILL, "Killed",SIG_DFL },
    { SIGALRM, "Alarm clock",SIG_DFL },
    { 0, "Stopped (signal)",SIG_DFL },
    { 0, "Stopped",SIG_DFL },
    { 0, "Continued",SIG_DFL },
    { 0, "Child exited",SIG_DFL },
    { 0, "Stopped (tty input)",SIG_DFL },
    { 0, "Stopped (tty output)",SIG_DFL },
    { 0, NULL, SIG_DFL }
  };

int nsignal = 21;

_p_sig_fn_t	signal(int sig, _p_sig_fn_t func)
{
  _p_sig_fn_t temp;
  int i;
  if(sig <= 0 || sig > SIGMAX || sig == SIGKILL)
  {
    __set_errno(EINVAL);
    return SIG_ERR;
  }
// check with IsBadCodePtr

  if ( func < (_p_sig_fn_t)4096 ) {
	__set_errno(EINVAL);
	return SIG_ERR;
  }

  for(i=0;i<nsignal;i++) {
	if ( signal_list[i].signal == sig ) {
  		temp = signal_list[i].handler;
  		signal_list[i].handler = func;
  		return temp;
	}
  }
  temp = signal_list[i].handler;
  signal_list[i].handler = func;
  signal_list[i].signal = sig;
  signal_list[i].signame = "";
  nsignal++;
  return temp;
  

}


int
raise(int sig)
{
  _p_sig_fn_t temp = SIG_DFL;
  int i;
  if(sig <= 0)
    return -1;
  if(sig > SIGMAX)
    return -1;
  for(i=0;i<nsignal;i++) {
	if ( signal_list[i].signal == sig ) {
  		temp = signal_list[i].handler;
	}
  }
  if(temp == (_p_sig_fn_t)SIG_IGN
     || (sig == SIGQUIT && temp == (_p_sig_fn_t)SIG_DFL))
    return 0;			/* Ignore it */
  if(temp == (_p_sig_fn_t)SIG_DFL)
    _default_handler(sig); /* this does not return */
  else
    temp(sig);

  return 0;
}



void _default_handler(int sig)
{
	_exit(3);
}


