/* $Id: debug.h,v 1.2 2002/02/20 09:17:55 hyperion Exp $
 */
/*
 * psx/debug.h
 *
 * debugging utilities
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
#ifndef __PSX_DEBUG_H_INCLUDED__
#define __PSX_DEBUG_H_INCLUDED__

/* INCLUDES */
#ifdef __PSX_DEBUG_TO_STDERR__
#include <stdio.h>
#else /* !defined(__PSX_DEBUG_TO_STDERR__) */
#include <ddk/ntddk.h>
#endif /* defined(__PSX_DEBUG_TO_STDERR__) */

/* OBJECTS */

/* TYPES */

/* CONSTANTS */

/* PROTOTYPES */

/* MACROS */

#define __PSX_MODULE__ "psxdll.dll"

#ifndef NDEBUG

#ifdef __PSX_DEBUG_TO_STDERR__

#if 0
#define DEBUGOUT(MODULE,TYPE,ARGS...) \
do{ \
 fprintf(stderr,"%s:%s:%s:%d:%s():\n\t",(MODULE),(TYPE),__FILE__,__LINE__,__FUNCTION__); \
 fprintf(stderr,ARGS); \
 fprintf("\n"); \
} \
while(0)
#endif

#define DEBUGOUT(MODULE,TYPE,ARGS...) \
do{ \
 printf("%s:%s:%s:%d:%s():\n\t",(MODULE),(TYPE),__FILE__,__LINE__,__FUNCTION__); \
 printf(ARGS); \
 printf("\n"); \
} \
while(0)


#else /* !defined(__PSX_DEBUG_TO_STDERR__) */

#define DEBUGOUT(MODULE,TYPE,ARGS...) \
do{ \
 DbgPrint("%s:%s:%s:%d:%s():\n\t",(MODULE),(TYPE),__FILE__,__LINE__,__FUNCTION__); \
 DbgPrint(ARGS); \
 DbgPrint("\n"); \
} \
while(0)

#endif /* defined(__PSX_DEBUG_TO_STDERR__) */

#define DEBUGOUTIF(CONDITION,MODULE,TYPE,ARGS...) \
if((CONDITION)) \
{ \
 DEBUGOUT((MODULE),(TYPE),ARGS); \
}

#else /* defined(NDEBUG) */

#define DEBUGOUTIF(c,m,t,args...)
#define DEBUGOUT(m,t,args...)

#endif /* !defined(NDEBUG) */

#if defined(__PSX_DEBUG_WANT_ALL__) || defined(__PSX_DEBUG_WANT_HINTS__)
#define HINT(args...)  DEBUGOUT(__PSX_MODULE__,"HINT",args)
#define HINTIF(c,args...)  DEBUGOUTIF((c),__PSX_MODULE__,"HINT",args)
#else
#define HINT(args...)
#define HINTIF(c,args...)
#endif

#if defined(__PSX_DEBUG_WANT_ALL__) || defined(__PSX_DEBUG_WANT_INFOS__)
#define INFO(args...)  DEBUGOUT(__PSX_MODULE__,"INFO",args)
#define INFOIF(c,args...)  DEBUGOUTIF((c),__PSX_MODULE__,"INFO",args)
#else
#define INFO(args...)
#define INFOIF(c,args...)
#endif

#if defined(__PSX_DEBUG_WANT_ALL__) || defined(__PSX_DEBUG_WANT_WARNS__)
#define WARN(args...)  DEBUGOUT(__PSX_MODULE__,"WARN",args)
#define WARNIF(c,args...)  DEBUGOUTIF((c),__PSX_MODULE__,"WARN",args)
#else
#define WARN(args...)
#define WARNIF(c,args...)
#endif

#if defined(__PSX_DEBUG_WANT_ALL__) || defined(__PSX_DEBUG_WANT_ERRS__)
#define ERR(args...)   DEBUGOUT(__PSX_MODULE__,"ERR",args)
#define ERRIF(c,args...)   DEBUGOUTIF((c),__PSX_MODULE__,"ERR",args)
#else
#define ERR(args...)
#define ERRIF(c,args...)
#endif

#if defined(__PSX_DEBUG_WANT_ALL__) || defined(__PSX_DEBUG_WANT_TODOS__)
#define TODO(args...)  DEBUGOUT(__PSX_MODULE__,"TODO",args)
#define TODOIF(c,args...)  DEBUGOUTIF((c),__PSX_MODULE__,"TODO",args)
#else
#define TODO(args...)
#define TODOIF(c,args...)
#endif

#if defined(__PSX_DEBUG_WANT_ALL__) || defined(__PSX_DEBUG_WANT_FIXMES__)
#define FIXME(args...) DEBUGOUT(__PSX_MODULE__,"FIXME",args)
#define FIXMEIF(c,args...) DEBUGOUTIF((c),__PSX_MODULE__,"FIXME",args)
#else
#define FIXME(args...)
#define FIXMEIF(c,args...)
#endif


#endif /* __PSX_DEBUG_H_INCLUDED__ */

/* EOF */

