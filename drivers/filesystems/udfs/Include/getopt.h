////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#ifndef _GETOPT_H
#define _GETOPT_H

#ifdef  __cplusplus
extern "C" {
#endif

#define BAD_OPTION '\0'

typedef struct _optarg_ctx {
    /* For communication from `getopt' to the caller.
       When `getopt' finds an option that takes an argument,
       the argument value is returned here.
       Also, when `ordering' is RETURN_IN_ORDER,
       each non-option ARGV-element is returned here.  */

    WCHAR* optarg;

    /* Index in ARGV of the next element to be scanned.
       This is used for communication to and from the caller
       and for communication between successive calls to `getopt'.

       On entry to `getopt', zero means this is the first call; initialize.

       When `getopt' returns EOF, this is the index of the first of the
       non-option elements that the caller should itself scan.

       Otherwise, `optind' communicates from one call to the next
       how much of ARGV has been scanned so far.  */

    int optind;

    /* The next char to be scanned in the option-element
       in which the last option character we returned was found.
       This allows us to pick up the scan where we left off.

       If this is zero, or a null string, it means resume the scan
       by advancing to the next ARGV-element.  */

    WCHAR *nextchar;

    /* Callers store zero here to inhibit the error message
       for unrecognized options.  */

    int opterr;

    /* Set to an option character which was unrecognized.
       This must be initialized on some systems to avoid linking in the
       system's own getopt implementation.  */

    int optopt;

    /* Describe the part of ARGV that contains non-options that have
       been skipped.  `first_nonopt' is the index in ARGV of the first of them;
       `last_nonopt' is the index after the last of them.  */

    int first_nonopt;
    int last_nonopt;

    /* Exchange two adjacent subsequences of ARGV.
       One subsequence is elements [first_nonopt,last_nonopt)
       which contains all the non-options that have been skipped so far.
       The other is elements [last_nonopt,optind), which contains all
       the options processed since those non-options were skipped.

       `first_nonopt' and `last_nonopt' are relocated so that they describe
       the new indices of the non-options in ARGV after they are moved.

       To perform the swap, we first reverse the order of all elements. So
       all options now come before all non options, but they are in the
       wrong order. So we put back the options and non options in original
       order by reversing them again. For example:
           original input:      a b c -x -y
           reverse all:         -y -x c b a
           reverse options:     -x -y c b a
           reverse non options: -x -y a b c
    */

} optarg_ctx, *P_optarg_ctx;

/* Describe the long-named options requested by the application.
   The LONG_OPTIONS argument to getopt_long or getopt_long_only is a vector
   of `struct option' terminated by an element containing a name which is
   zero.

   The field `has_arg' is:
   no_argument          (or 0) if the option does not take an argument,
   required_argument    (or 1) if the option requires an argument,
   optional_argument    (or 2) if the option takes an optional argument.

   If the field `flag' is not NULL, it points to a variable that is set
   to the value given in the field `val' when the option is found, but
   left unchanged if the option is not found.

   To have a long-named option do something other than set an `int' to
   a compiled-in constant, such as set a value from `optarg', set the
   option's `flag' field to zero and its `val' field to a nonzero
   value (the equivalent single-letter option character, if there is
   one).  For long options that have a zero `flag' field, `getopt'
   returns the contents of the `val' field.  */

struct option
{
  WCHAR *name;
  /* has_arg can't be an enum because some compilers complain about
     type mismatches in all the code that assumes it is an int.  */
  int has_arg;
  int *flag;
  int val;
};

extern void getopt_init(optarg_ctx* o);

/* Names for the values of the `has_arg' field of `struct option'.  */

#define no_argument             0
#define required_argument       1
#define optional_argument       2

extern int getopt (optarg_ctx* o, int argc, WCHAR *const *argv, const WCHAR *shortopts);
extern int getopt_long (optarg_ctx* o, int argc, WCHAR *const *argv, const WCHAR *shortopts,
                        const struct option *longopts, int *longind);
extern int getopt_long_only (optarg_ctx* o, int argc, WCHAR *const *argv,
                             const WCHAR *shortopts,
                             const struct option *longopts, int *longind);

/* Internal only.  Users should not call this directly.  */
extern int _getopt_internal (optarg_ctx* o, int argc, WCHAR *const *argv,
                             const WCHAR *shortopts,
                             const struct option *longopts, int *longind,
                             int long_only);

#ifdef  __cplusplus
}
#endif

#endif /* _GETOPT_H */
