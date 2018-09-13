/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    r_comp.c

Abstract:

    This module implements routines to compress and expand names in DNS
    resolver messages.

Author:

    Mike Massa (mikemas)           Sept 20, 1991

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     9-20-91     created

Notes:

    Exports:
        dn_expand()
        dn_comp()

--*/

#ident "@(#)res_comp.c  5.3     3/8/91"

/******************************************************************
 *
 *  SpiderTCP BIND
 *
 *  Copyright 1990  Spider Systems Limited
 *
 *  RES_COMP.C
 *
 ******************************************************************/

/*
 *       /usr/projects/tcp/SCCS.rel3/rel/src/lib/net/0/s.res_comp.c
 *      @(#)res_comp.c  5.3
 *
 *      Last delta created      14:11:35 3/4/91
 *      This file extracted     11:20:30 3/8/91
 *
 *      Modifications:
 *
 *              GSS     24 Jul 90       New File
 */
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_comp.c  6.18 (Berkeley) 6/27/90";
#endif /* LIBC_SCCS and not lint */

#include "winsockp.h"

#include <sys/types.h>

/*
 * Expand compressed domain name 'comp_dn' to full domain name.
 * 'msg' is a pointer to the begining of the message,
 * 'eomorig' points to the first location after the message,
 * 'exp_dn' is a pointer to a buffer of size 'length' for the result.
 * Return size of compressed name or -1 if there was an error.
 */
int
dn_expand(
    IN  unsigned char *msg,
    IN  unsigned char *eomorig,
    IN  unsigned char *comp_dn,
    OUT unsigned char *exp_dn,
    IN  int            length
    )
{
        register u_char *cp, *dn;
        register int n, c;
        u_char *eom;
        int len = -1, checked = 0;

        IF_DEBUG(RESOLVER) {
            WS_PRINT(("dn_expand entered\n"));
        }

        dn = exp_dn;
        cp = comp_dn;
        eom = exp_dn + length;
        /*
         * fetch next label in domain name
         */
        while (n = *cp++) {
                /*
                 * Check for indirection
                 */
                switch (n & INDIR_MASK) {
                case 0:
                        if (dn != exp_dn) {
                                if (dn >= eom)
                                        return (-1);
                                *dn++ = '.';
                        }
                        if (dn+n >= eom)
                                return (-1);
                        checked += n + 1;
                        while (--n >= 0) {
                                if ((c = *cp++) == '.') {
                                        if (dn + n + 2 >= eom)
                                                return (-1);
                                        *dn++ = '\\';
                                }
                                *dn++ = (u_char) c;
                                if (cp >= eomorig)      /* out of range */
                                        return(-1);
                        }
                        break;

                case INDIR_MASK:
                        if (len < 0)
                                len = (INT)(LONG_PTR)(cp - comp_dn + 1);
                        cp = msg + (((n & 0x3f) << 8) | (*cp & 0xff));
                        if (cp < msg || cp >= eomorig)  /* out of range */
                                return(-1);
                        checked += 2;
                        /*
                         * Check for loops in the compressed name;
                         * if we've looked at the whole message,
                         * there must be a loop.
                         */
                        if (checked >= eomorig - msg)
                                return (-1);
                        break;

                default:
                        return (-1);                    /* flag error */
                }
        }
        *dn = '\0';
        if (len < 0)
                len = (INT)(LONG_PTR)(cp - comp_dn);
        return (len);
}

/*
 * Compress domain name 'exp_dn' into 'comp_dn'.
 * Return the size of the compressed name or -1.
 * 'length' is the size of the array pointed to by 'comp_dn'.
 * 'dnptrs' is a list of pointers to previous compressed names. dnptrs[0]
 * is a pointer to the beginning of the message. The list ends with NULL.
 * 'lastdnptr' is a pointer to the end of the arrary pointed to
 * by 'dnptrs'. Side effect is to update the list of pointers for
 * labels inserted into the message as we compress the name.
 * If 'dnptr' is NULL, we don't try to compress names. If 'lastdnptr'
 * is NULL, we don't update the list.
 */
int
dn_comp(
    IN      unsigned char  *exp_dn,
    OUT     unsigned char  *comp_dn,
    IN      int             length,
    IN      unsigned char **dnptrs,     OPTIONAL
    IN OUT  unsigned char **lastdnptr   OPTIONAL
    )
{
        register u_char *cp, *dn;
        register int c, l;
        u_char **cpp, **lpp, *sp, *eob;
        u_char *msg;

        IF_DEBUG(RESOLVER) {
            WS_PRINT(("dn_comp entered\n"));
        }

        dn = exp_dn;
        cp = comp_dn;
        eob = cp + length;
        if (dnptrs != NULL) {
                if ((msg = *dnptrs++) != NULL) {
                        for (cpp = dnptrs; *cpp != NULL; cpp++)
                                ;
                        lpp = cpp;      /* end of list to search */
                }
        } else
                msg = NULL;
        for (c = *dn++; c != '\0'; ) {
                /* look to see if we can use pointers */
                if (msg != NULL) {
                        if ((l = dn_find(dn-1, msg, dnptrs, lpp)) >= 0) {
                                if (cp+1 >= eob)
                                        return (-1);
                                *cp++ = (unsigned char) ((l >> 8) | INDIR_MASK);
                                *cp++ = (u_char) (l % 256);
                                return (INT)(LONG_PTR)(cp - comp_dn);
                        }
                        /* not found, save it */
                        if (lastdnptr != NULL && cpp < lastdnptr-1) {
                                *cpp++ = cp;
                                *cpp = NULL;
                        }
                }
                sp = cp++;      /* save ptr to length byte */
                do {
                        if (c == '.') {
                                c = *dn++;
                                break;
                        }
                        if (c == '\\') {
                                if ((c = *dn++) == '\0')
                                        break;
                        }
                        if (cp >= eob) {
                                if (msg != NULL)
                                        *lpp = NULL;
                                return (-1);
                        }
                        *cp++ = (u_char) c;
                } while ((c = *dn++) != '\0');
                /* catch trailing '.'s but not '..' */
                if ((l = (INT)(LONG_PTR)(cp - sp - 1)) == 0 && c == '\0') {
                        cp--;
                        break;
                }
                if (l <= 0 || l > MAXLABEL) {
                        if (msg != NULL)
                                *lpp = NULL;
                        return (-1);
                }
                *sp = (u_char) l;
        }
        if (cp >= eob) {
                if (msg != NULL)
                        *lpp = NULL;
                return (-1);
        }
        *cp++ = '\0';
        return (INT)(LONG_PTR)(cp - comp_dn);
}

/*
 * Skip over a compressed domain name. Return the size or -1.
 */
int
dn_skipname(
    unsigned char *comp_dn,
    unsigned char *eom
    )
{
        register u_char *cp;
        register int n;

        IF_DEBUG(RESOLVER) {
            WS_PRINT(("dn_skipname entered\n"));
        }

        cp = comp_dn;
        while (cp < eom && (n = *cp++)) {
                /*
                 * check for indirection
                 */
                switch (n & INDIR_MASK) {
                case 0:         /* normal case, n == len */
                        cp += n;
                        continue;
                default:        /* illegal type */
                        return (-1);
                case INDIR_MASK:        /* indirection */
                        cp++;
                }
                break;
        }
        return (INT)(LONG_PTR)(cp - comp_dn);
}

/*
 * Search for expanded name from a list of previously compressed names.
 * Return the offset from msg if found or -1.
 * dnptrs is the pointer to the first name on the list,
 * not the pointer to the start of the message.
 */
static int
dn_find(
    unsigned char  *exp_dn,
    unsigned char  *msg,
    unsigned char **dnptrs,
    unsigned char **lastdnptr
    )
{
        register u_char *dn, *cp, **cpp;
        register int n;
        u_char *sp;

        IF_DEBUG(RESOLVER) {
            WS_PRINT(("dn_find entered\n"));
        }

        for (cpp = dnptrs; cpp < lastdnptr; cpp++) {
                dn = exp_dn;
                sp = cp = *cpp;
                while (n = *cp++) {
                        /*
                         * check for indirection
                         */
                        switch (n & INDIR_MASK) {
                        case 0:         /* normal case, n == len */
                                while (--n >= 0) {
                                        if (*dn == '.')
                                                goto next;
                                        if (*dn == '\\')
                                                dn++;
                                        if (*dn++ != *cp++)
                                                goto next;
                                }
                                if ((n = *dn++) == '\0' && *cp == '\0')
                                        return (INT)(LONG_PTR)(sp - msg);
                                if (n == '.')
                                        continue;
                                goto next;

                        default:        /* illegal type */
                                return (-1);

                        case INDIR_MASK:        /* indirection */
                                cp = msg + (((n & 0x3f) << 8) | *cp);
                        }
                }
                if (*dn == '\0')
                        return (INT)(LONG_PTR)(sp - msg);
        next:   ;
        }
        return (-1);
}

/*
 * Routines to insert/extract short/long's. Must account for byte
 * order and non-alignment problems. This code at least has the
 * advantage of being portable.
 *
 * used by sendmail.
 */

u_short
_getshort(
    unsigned char *msgp
    )
{
        register u_char *p = (u_char *) msgp;
#ifdef vax
        /*
         * vax compiler doesn't put shorts in registers
         */
        register u_long u;
#else
        register u_short u;
#endif

        u = ((u_short) *p++) << 8;
        return (u_short) (u | ((u_short) *p));
}

u_long
_getlong(msgp)
        u_char *msgp;
{
        register u_char *p = (u_char *) msgp;
        register u_long u;

        u = *p++; u <<= 8;
        u |= *p++; u <<= 8;
        u |= *p++; u <<= 8;
        return (u | *p);
}

void
putshort(s, msgp)
        register u_short s;
        register u_char *msgp;
{

        msgp[1] = (u_char) s;
        msgp[0] = (u_char) (s >> 8);
}

void
putlong(l, msgp)
        register u_long l;
        register u_char *msgp;
{

        msgp[3] = (u_char) l;
        msgp[2] = (u_char) (l >>= 8);
        msgp[1] = (u_char) (l >>= 8);
        msgp[0] = (u_char)(l >> 8);
}

