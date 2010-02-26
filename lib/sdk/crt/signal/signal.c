#include <precomp.h>
#include "include/internal/wine/msvcrt.h"

static sig_element signal_list[] =
   {
      { SIGINT, "CTRL+C",SIG_DFL },
      { SIGILL, "Illegal instruction",SIG_DFL },
      { SIGFPE, "Floating-point exception",SIG_DFL },
      { SIGSEGV, "Illegal storage access",SIG_DFL },
      { SIGTERM, "Termination request",SIG_DFL },
      { SIGBREAK, "CTRL+BREAK",SIG_DFL },
      { SIGABRT, "Abnormal termination",SIG_DFL }
   };

//int nsignal = 21;

/*
 * @implemented
 */
//void ( *signal( int sig, void (__cdecl *func) ( int sig [, int subcode ] )) ) ( int sig );


__p_sig_fn_t signal(int sig, __p_sig_fn_t func)
{
   __p_sig_fn_t temp;
   unsigned int i;

   switch (sig)
   {
      case SIGINT:
      case SIGILL:
      case SIGFPE:
      case SIGSEGV:
      case SIGTERM:
      case SIGBREAK:
      case SIGABRT:
         break;

      default:
         __set_errno(EINVAL);
         return SIG_ERR;
   }

   // check with IsBadCodePtr
   if ( func < (__p_sig_fn_t)4096 && func != SIG_DFL && func != SIG_IGN)
   {
      __set_errno(EINVAL);
      return SIG_ERR;
   }

   for(i=0; i < sizeof(signal_list)/sizeof(signal_list[0]); i++)
   {
      if ( signal_list[i].signal == sig )
      {
         temp = signal_list[i].handler;
         signal_list[i].handler = func;
         return temp;
      }
   }

   /* should be impossible to get here */
   __set_errno(EINVAL);
   return SIG_ERR;
}


/*
 * @implemented
 */
int
raise(int sig)
{
   __p_sig_fn_t temp = 0;
   unsigned int i;

   switch (sig)
   {
      case SIGINT:
      case SIGILL:
      case SIGFPE:
      case SIGSEGV:
      case SIGTERM:
      case SIGBREAK:
      case SIGABRT:
         break;

      default:
         //FIXME: set last err?
         return -1;
   }


   //  if(sig <= 0)
   //    return -1;
   //  if(sig > SIGMAX)
   //    return -1;

   for(i=0;i<sizeof(signal_list)/sizeof(signal_list[0]);i++)
   {
      if ( signal_list[i].signal == sig )
      {
         temp = signal_list[i].handler;
         break;
      }
   }

   if(temp == SIG_IGN)// || (sig == SIGQUIT && temp == (_p_sig_fn_t)SIG_DFL))
      return 0;   /* Ignore it */

   if(temp == SIG_DFL)
      _default_handler(sig); /* this does not return */
   else
      temp(sig);

   return 0;
}



void _default_handler(int sig)
{
   _exit(3);
}





