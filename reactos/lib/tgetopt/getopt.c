/* $Id: getopt.c,v 1.1 2003/04/22 03:20:25 hyperion Exp $
*/
/*
 tgetopt -- POSIX-compliant implementation of getopt() with string-type-generic
 semantics

 This is public domain software
*/

#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <tgetopt.h>

int _topterr = 1;
int _toptind = 1;
int _toptopt;
_TCHAR * _toptarg;

int _tgetopt(int argc, _TCHAR * const argv[], const _TCHAR * optstring)
{
 static int s_nArgChar = 0;
 _TCHAR * pcOptChar;

 /* we're done */
 if(_toptind >= argc) return -1;

 /* last time we reached the end of argv[_toptind] */
 if(s_nArgChar != 0 && argv[_toptind][s_nArgChar] == 0)
 {
  /* scan the next argument */
  ++ _toptind;

  /* we're done */
  if(_toptind >= argc) return -1;

  s_nArgChar = 0;
 }

 /* first time we scan argv[_toptind] */
 if(s_nArgChar == 0)
 {
  /* argument is NULL - we're done */
  if(argv[_toptind] == NULL)
   return (int)-1;
  /* argument is empty - we're done */
  else if(argv[_toptind][0] == 0)
   return (int)-1;
  /* argument begins with '-' */
  else if(argv[_toptind][0] == _T('-'))
  {
   /* argument is "--" */
   if(argv[_toptind][1] == _T('-'))
   {
    /* increment optind */
    ++ _toptind;
    s_nArgChar = 0;

    /* we're done */
    return (int)-1;
   }
   /* argument is "-" */
   else if(argv[_toptind][1] == 0)
   {
    /* we're done */
    return (int)-1;
   }
   /* possible option */
   else
    ++ s_nArgChar;
  }
  /* argument doesn't begin with a dash - we're done */
  else
   return (int)-1;
 }

 /* return the current character */
 _toptopt = argv[_toptind][s_nArgChar];

 /* advance to the next character */
 ++ s_nArgChar;

 /* unrecognized option */
 if(_toptopt == _T(':') || (pcOptChar = _tcschr(optstring, _toptopt)) == NULL)
 {
  /* print an error message */
  if(_topterr && optstring[0] != _T(':'))
   _ftprintf(stderr, _T("%s: illegal option -- %c\n"), argv[0], _toptopt);;

  /* return an error */
  return _T('?');
 }

 /* the option requires an argument */
 if(pcOptChar[1] == _T(':'))
 {
  /* we are at the end of the argument */
  if(argv[_toptind][s_nArgChar] == 0)
  {
   /* the argument of the option is the next argument */
   ++ _toptind;
   s_nArgChar = 0;

   /* this is the last argument */
   if(_toptopt >= argc)
   {
    /* print an error message */
    if(_topterr && optstring[0] != _T(':'))
    {
     _ftprintf
     (
      stderr,
      _T("%s: option requires an argument -- %c\n"),
      argv[0],
      _toptopt
     );
    }

    /* return an error */
    return ((optstring[0] == _T(':')) ? _T(':') : _T('?'));
   }

   /* return the argument */
   _toptarg = argv[_toptind];
  }
  /* the rest of the argument is the argument of the option */
  else
   _toptarg = argv[_toptind] + s_nArgChar;
 }
 
 /* success */
 return _toptopt;
}

/* EOF */
