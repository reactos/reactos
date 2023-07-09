/******************************Module*Header*******************************\
* Module Name: ssintrnl.hxx
*
* Internal include file for screen saver common shell
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __ssintrnl_hxx__
#define __ssintrnl_hxx__

#include "sscommon.h"

#include "util.hxx"
#include "glscrnsv.hxx"

// one and only screen saver instance
extern SCRNSAVE *gpss;

// delayed start stuff

#define SS_DELAYED_START_KLUGE 1

#if 0
// Delayed start is dependent on ss type
#define SS_DELAY_START( type ) \
    ((type) == SS_TYPE_PREVIEW)
#else
// Use delayed start for all ss types.
#define SS_DELAY_START( type ) \
    (TRUE)
#endif

#ifdef SS_WIN95
// This works around win95 problem with excessive WM_TIMER msgs flooding the
// queue, preventing password dialogs from displaying
#define SS_WIN95_TIMER_HACK 1
#endif

#endif // __ssintrnl_hxx__
