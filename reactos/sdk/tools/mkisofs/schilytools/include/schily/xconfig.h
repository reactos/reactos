/* @(#)xconfig.h	1.13 13/07/23 Copyright 1995-2013 J. Schilling */
/*
 *	This file either includes the dynamic or manual autoconf stuff.
 *
 *	Copyright (c) 1995-2013 J. Schilling
 *
 *	This file is included from <schily/mconfig.h> and usually
 *	includes $(SRCROOT)/incs/$(OARCH)/xconfig.h via
 *	-I$(SRCROOT)/incs/$(OARCH)/
 *
 *	Use only cpp instructions.
 *
 *	NOTE: SING: (Schily Is Not Gnu)
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef _SCHILY_XCONFIG_H
#define	_SCHILY_XCONFIG_H

/*
 * This hack that is needed as long as VMS has no POSIX shell.
 * It will go away soon. VMS users: in future you need to specify:
 * cc -DUSE_STATIC_CONF
 */
#ifdef	VMS
#	define	USE_STATIC_CONF
#endif

#ifdef	NO_STATIC_CONF
#undef	USE_STATIC_CONF
#endif

#ifdef	USE_STATIC_CONF
#	include <schily/xmconfig.h>	/* The static autoconf stuff */
#else	/* USE_STATIC_CONF */


#ifdef	SCHILY_BUILD	/* #defined by Schily makefile system */
	/*
	 * Include $(SRCROOT)/incs/$(OARCH)/xconfig.h via
	 * -I$(SRCROOT)/incs/$(OARCH)/
	 */
#	include <xconfig.h>	/* The current dynamic autoconf stuff */
#else	/* !SCHILY_BUILD */
/*
 * The stuff for static compilation. Include files from a previous
 * dynamic autoconfiguration.
 */
#ifdef	__SUNOS5_SPARC_CC32
#include <schily/sparc-sunos5-cc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_SPARC_CC64
#include <schily/sparc-sunos5-cc64/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_SPARC_GCC32
#include <schily/sparc-sunos5-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_SPARC_GCC64
#include <schily/sparc-sunos5-gcc64/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_X86_CC32
#include <schily/i386-sunos5-cc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_X86_CC64
#include <schily/i386-sunos5-cc64/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_X86_GCC32
#include <schily/i386-sunos5-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_X86_GCC64
#include <schily/i386-sunos5-gcc64/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif

#ifdef	__SUNOS4_MC68000_CC32
#ifdef	__mc68020
#include <schily/mc68020-sunos4-cc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#endif
#ifdef	__SUNOS4_MC68000_GCC32
#ifdef	__mc68020
#include <schily/mc68020-sunos4-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#endif

#ifdef	__HPUX_HPPA_CC32
#include <schily/hppa-hp-ux-cc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__HPUX_HPPA_CC64
#include <schily/hppa-hp-ux-cc64/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__HPUX_HPPA_GCC32
#include <schily/hppa-hp-ux-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__HPUX_HPPA_GCC64
#include <schily/hppa-hp-ux-gcc64/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif

#ifdef	__LINUX_ARMV6L_GCC32
#include <schily/armv6l-linux-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__LINUX_ARMV5L_GCC32
#include <schily/armv6l-linux-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__LINUX_ARMV5TEJL_GCC32
#include <schily/armv5tejl-linux-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__LINUX_I386_GCC32
#include <schily/i686-linux-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__LINUX_amd64_GCC64
#include <schily/x86_64-linux-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif

#ifdef	__MSWIN_X86_CL32
#include <schily/i686-cygwin32_nt-cl/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif

#ifdef	__CYGWIN_X86_GCC
#include <schily/i686-cygwin32_nt-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif

#ifndef	__JS_ARCH_CONF_INCL
/*
 * #error will not work for all compilers (e.g. sunos4)
 * The following line will abort compilation on all compilers
 * if none of the above is defined. And that's  what we want.
 */
Error unconfigured architecture

#include <schily/err_arch.h>	/* Avoid "unknown directive" with K&R */
#endif

#endif	/* SCHILY_BUILD */

#endif	/* USE_STATIC_CONF */

#ifdef	__cplusplus
extern "C" {
#endif
/*
 * Fake in order to keep the silly hdrchk(1) quiet.
 */
#ifdef	__cplusplus
}
#endif

#endif /* _SCHILY_XCONFIG_H */
