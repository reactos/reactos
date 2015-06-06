////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////

#include "getopt.h"


#ifdef  __cplusplus
extern "C" {
#endif

/* Describe how to deal with options that follow non-option ARGV-elements.

   If the caller did not specify anything,
   the default is REQUIRE_ORDER if the environment variable
   POSIXLY_CORRECT is defined, PERMUTE otherwise.

   REQUIRE_ORDER means don't recognize them as options;
   stop option processing when the first non-option is seen.
   This is what Unix does.
   This mode of operation is selected by either setting the environment
   variable POSIXLY_CORRECT, or using `+' as the first character
   of the list of option characters.

   PERMUTE is the default.  We permute the contents of ARGV as we scan,
   so that eventually all the non-options are at the end.  This allows options
   to be given in any order, even with programs that were not written to
   expect this.

   RETURN_IN_ORDER is an option available to programs that were written
   to expect options and other ARGV-elements in any order and that care about
   the ordering of the two.  We describe each non-option ARGV-element
   as if it were the argument of an option with character code 1.
   Using `-' as the first character of the list of option characters
   selects this mode of operation.

   The special argument `--' forces an end of option-scanning regardless
   of the value of `ordering'.  In the case of RETURN_IN_ORDER, only
   `--' can cause `getopt' to return EOF with `optind' != ARGC.  */

static enum
{
    REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
} ordering;

#define my_index    wcschr

#define my_strtoul  wcstoul
#define my_strlen   wcslen
#define my_strncmp  wcsncmp
#define my_strcpy   wcscpy
#define my_strcat   wcscat
#define my_strcmp   wcscmp

/* Handle permutation of arguments.  */


void
getopt_init(optarg_ctx* o) {

    o->optarg = NULL;
    o->optind = 0;
    o->optopt = BAD_OPTION;
    o->opterr = 1;
}

static void
exchange (
    optarg_ctx* o,
    WCHAR **argv
    )
{
    WCHAR *temp, **first, **last;

    /* Reverse all the elements [first_nonopt, optind) */
    first = &argv[o->first_nonopt];
    last  = &argv[o->optind-1];
    while (first < last) {
        temp = *first; *first = *last; *last = temp; first++; last--;
    }
    /* Put back the options in order */
    first = &argv[o->first_nonopt];
    o->first_nonopt += (o->optind - o->last_nonopt);
    last  = &argv[o->first_nonopt - 1];
    while (first < last) {
        temp = *first; *first = *last; *last = temp; first++; last--;
    }

    /* Put back the non options in order */
    first = &argv[o->first_nonopt];
    o->last_nonopt = o->optind;
    last  = &argv[o->last_nonopt-1];
    while (first < last) {
        temp = *first; *first = *last; *last = temp; first++; last--;
    }
}

/* Scan elements of ARGV (whose length is ARGC) for option characters
   given in OPTSTRING.

   If an element of ARGV starts with '-', and is not exactly "-" or "--",
   then it is an option element.  The characters of this element
   (aside from the initial '-') are option characters.  If `getopt'
   is called repeatedly, it returns successively each of the option characters
   from each of the option elements.

   If `getopt' finds another option character, it returns that character,
   updating `optind' and `nextchar' so that the next call to `getopt' can
   resume the scan with the following option character or ARGV-element.

   If there are no more option characters, `getopt' returns `EOF'.
   Then `optind' is the index in ARGV of the first ARGV-element
   that is not an option.  (The ARGV-elements have been permuted
   so that those that are not options now come last.)

   OPTSTRING is a string containing the legitimate option characters.
   If an option character is seen that is not listed in OPTSTRING,
   return BAD_OPTION after printing an error message.  If you set `opterr' to
   zero, the error message is suppressed but we still return BAD_OPTION.

   If a char in OPTSTRING is followed by a colon, that means it wants an arg,
   so the following text in the same ARGV-element, or the text of the following
   ARGV-element, is returned in `optarg'.  Two colons mean an option that
   wants an optional arg; if there is text in the current ARGV-element,
   it is returned in `optarg', otherwise `optarg' is set to zero.

   If OPTSTRING starts with `-' or `+', it requests different methods of
   handling the non-option ARGV-elements.
   See the comments about RETURN_IN_ORDER and REQUIRE_ORDER, above.

   Long-named options begin with `--' instead of `-'.
   Their names may be abbreviated as long as the abbreviation is unique
   or is an exact match for some defined option.  If they have an
   argument, it follows the option name in the same ARGV-element, separated
   from the option name by a `=', or else the in next ARGV-element.
   When `getopt' finds a long-named option, it returns 0 if that option's
   `flag' field is nonzero, the value of the option's `val' field
   if the `flag' field is zero.

   The elements of ARGV aren't really const, because we permute them.
   But we pretend they're const in the prototype to be compatible
   with other systems.

   LONGOPTS is a vector of `struct option' terminated by an
   element containing a name which is zero.

   LONGIND returns the index in LONGOPT of the long-named option found.
   It is only valid when a long-named option has been found by the most
   recent call.

   If LONG_ONLY is nonzero, '-' as well as '--' can introduce
   long-named options.  */

int
_getopt_internal(
    optarg_ctx* o,
    int argc,
    WCHAR *const *argv,
    const WCHAR *optstring,
    const struct option *longopts,
    int *longind,
    int long_only)
{
    int option_index;

    o->optarg = 0;

    /* Initialize the internal data when the first call is made.
       Start processing options with ARGV-element 1 (since ARGV-element 0
       is the program name); the sequence of previously skipped
       non-option ARGV-elements is empty.  */

    if (o->optind == 0)
    {
        o->first_nonopt = o->last_nonopt = o->optind = 1;

        o->nextchar = NULL;

        /* Determine how to handle the ordering of options and nonoptions.  */

        if (optstring[0] == '-') {
            ordering = RETURN_IN_ORDER;
            ++optstring;
        } else if (optstring[0] == '+') {
            ordering = REQUIRE_ORDER;
            ++optstring;
/*        } else if (getenv ("POSIXLY_CORRECT") != NULL) {
            ordering = REQUIRE_ORDER;*/
        } else {
            ordering = PERMUTE;
        }
    }

    if (o->nextchar == NULL || *(o->nextchar) == '\0')
    {
        if (ordering == PERMUTE)
        {
            /* If we have just processed some options following some non-options,
               exchange them so that the options come first.  */

            if (o->first_nonopt != o->last_nonopt && o->last_nonopt != o->optind) {
                exchange (o, (WCHAR **) argv);
            } else if (o->last_nonopt != o->optind) {
                o->first_nonopt = o->optind;
            }

            /* Now skip any additional non-options
               and extend the range of non-options previously skipped.  */

            while (o->optind < argc
                   && (argv[o->optind][0] != '-' || argv[o->optind][1] == '\0')
                  ) {
                o->optind++;
            }
            o->last_nonopt = o->optind;
        }

        /* Special ARGV-element `--' means premature end of options.
       Skip it like a null option,
       then exchange with previous non-options as if it were an option,
       then skip everything else like a non-option.  */

        if (o->optind != argc && !my_strcmp (argv[o->optind], L"--"))
        {
            o->optind++;

            if (o->first_nonopt != o->last_nonopt && o->last_nonopt != o->optind) {
                exchange (o, (WCHAR **) argv);
            } else if (o->first_nonopt == o->last_nonopt) {
                o->first_nonopt = o->optind;
            }
            o->last_nonopt = argc;

            o->optind = argc;
        }

        /* If we have done all the ARGV-elements, stop the scan
        and back over any non-options that we skipped and permuted.  */

        if (o->optind == argc)
        {
            /* Set the next-arg-index to point at the non-options
               that we previously skipped, so the caller will digest them.  */
            if (o->first_nonopt != o->last_nonopt)
                o->optind = o->first_nonopt;
            return EOF;
        }

        /* If we have come to a non-option and did not permute it,
       either stop the scan or describe it to the caller and pass it by.  */

        if ((argv[o->optind][0] != '-' || argv[o->optind][1] == '\0'))
        {
            if (ordering == REQUIRE_ORDER)
                return EOF;
            o->optarg = argv[o->optind++];
            return 1;
        }

         /* We have found another option-ARGV-element.
        Start decoding its characters.  */
        o->nextchar = (argv[o->optind] + 1
            + (longopts != NULL && argv[o->optind][1] == '-'));
    }

    if (longopts != NULL
        && ((argv[o->optind][0] == '-'
         && (argv[o->optind][1] == '-' || long_only))
        ))
    {
        const struct option *p;
        WCHAR *s = o->nextchar;
        int exact = 0;
        int ambig = 0;
        const struct option *pfound = NULL;
        int indfound = 0;

        while (*s && *s != '=')
            s++;

        /* Test all options for either exact match or abbreviated matches.  */
        for (p = longopts, option_index = 0;
             p->name;
             p++, option_index++)
        if ( (p->val) && (!my_strncmp (p->name, o->nextchar, s - o->nextchar)) )
        {
            if (s - o->nextchar == (int)my_strlen (p->name))
            {
                /* Exact match found.  */
                pfound = p;
                indfound = option_index;
                exact = 1;
                break;
            } else if (pfound == NULL) {
                /* First nonexact match found.  */
                pfound = p;
                indfound = option_index;
            } else {
              /* Second nonexact match found.  */
                ambig = 1;
            }
        }

        if (ambig && !exact) {
            if (o->opterr) {
                KdPrint(("%ws: option `%s' is ambiguous\n",
                     argv[0], argv[o->optind]));
            }
            o->nextchar += my_strlen (o->nextchar);
            o->optind++;
            return BAD_OPTION;
        }

        if (pfound != NULL)
        {
            option_index = indfound;
            o->optind++;
            if (*s) {
                /* Don't test has_arg with >, because some C compilers don't
               allow it to be used on enums.  */
                if (pfound->has_arg) {
                    o->optarg = s + 1;
                } else {
                    if (o->opterr) {
                        if (argv[o->optind - 1][1] == '-') {
                            /* --option */
                            KdPrint((
                                 "%ws: option `--%ws' doesn't allow an argument\n",
                                 argv[0], pfound->name));
                        } else {
                            /* +option or -option */
                            KdPrint((
                                 "%ws: option `%c%ws' doesn't allow an argument\n",
                                 argv[0], argv[o->optind - 1][0], pfound->name));
                        }
                    }
                    o->nextchar += my_strlen (o->nextchar);
                    return BAD_OPTION;
                }
            }
            else if (pfound->has_arg == 1)
            {
                if (o->optind < argc) {
                    o->optarg = argv[(o->optind)++];
                } else {
                    if (o->opterr)
                        KdPrint(("%ws: option `%ws' requires an argument\n",
                           argv[0], argv[o->optind - 1]));
                    o->nextchar += my_strlen (o->nextchar);
                    return optstring[0] == ':' ? ':' : BAD_OPTION;
                }
            }
            o->nextchar += my_strlen (o->nextchar);
            if (longind != NULL)
                *longind = option_index;
            if (pfound->flag) {
                *(pfound->flag) = pfound->val;
                return 0;
            }
            return pfound->val;
        }
         /* Can't find it as a long option.  If this is not getopt_long_only,
        or the option starts with '--' or is not a valid short
        option, then it's an error.
        Otherwise interpret it as a short option.  */
        if (!long_only || argv[o->optind][1] == '-'
            || my_index (optstring, *(o->nextchar)) == NULL)
        {
            if (o->opterr)
            {
                if (argv[o->optind][1] == '-') {
                    /* --option */
                    KdPrint(("%ws: unrecognized option `--%ws'\n",
                         argv[0], o->nextchar));
                } else {
                    /* +option or -option */
                    KdPrint(("%ws: unrecognized option `%c%ws'\n",
                         argv[0], argv[o->optind][0], o->nextchar));
                }
            }
            o->nextchar = (WCHAR *) L"";
            o->optind++;
            return BAD_OPTION;
        }
    }

    /* Look at and handle the next option-character.  */

    {
        WCHAR c = *(o->nextchar)++;
        WCHAR *temp = my_index (optstring, c);

        /* Increment `optind' when we start to process its last character.  */
        if (*(o->nextchar) == '\0')
          ++(o->optind);

        if (temp == NULL || c == ':')
        {
            if (o->opterr)
            {
                KdPrint(("%ws: illegal option -- %c\n", argv[0], c));
            }
            o->optopt = c;
            return BAD_OPTION;
        }
        if (temp[1] == ':')
        {
            if (temp[2] == ':')
            {
                /* This is an option that accepts an argument optionally.  */
                if (*(o->nextchar) != '\0') {
                    o->optarg = o->nextchar;
                    o->optind++;
                } else {
                    o->optarg = 0;
                }
                o->nextchar = NULL;
            }
            else
            {
                /* This is an option that requires an argument.  */
                if (*(o->nextchar) != '\0')
                {
                    o->optarg = o->nextchar;
                    /* If we end this ARGV-element by taking the rest as an arg,
                       we must advance to the next element now.  */
                    o->optind++;
                }
                else if (o->optind == argc)
                {
                    if (o->opterr)
                    {
                        KdPrint(("%ws: option requires an argument -- %c\n",
                             argv[0], c));
                    }
                    o->optopt = c;
                    if (optstring[0] == ':') {
                        c = ':';
                    } else {
                        c = BAD_OPTION;
                    }
                }
                else
                {
                    /* We already incremented `optind' once;
                   increment it again when taking next ARGV-elt as argument.  */
                    o->optarg = argv[o->optind++];
                }
                o->nextchar = NULL;
            }
        }
        return c;
    }
}

int
getopt (
    optarg_ctx* o,
    int argc,
    WCHAR *const *argv,
    const WCHAR *optstring)
{
    return _getopt_internal (o, argc, argv, optstring,
               (const struct option *) 0,
               (int *) 0,
               0);
}

int
getopt_long (
    optarg_ctx* o,
    int argc,
    WCHAR *const *argv,
    const WCHAR *options,
    const struct option *long_options,
    int *opt_index)
{
  return _getopt_internal (o, argc, argv, options, long_options, opt_index, 0);
}


#ifdef  __cplusplus
}
#endif
