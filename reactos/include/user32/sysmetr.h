/*
 * System metrics definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_SYSMETRICS_H
#define __WINE_SYSMETRICS_H

#include <windows.h>


  /* Constant system metrics */
#if 0
#ifdef WIN_95_LOOK
#define SYSMETRICS_CXDLGFRAME         3
#define SYSMETRICS_CYDLGFRAME         3
#define SYSMETRICS_CYVTHUMB          13
#define SYSMETRICS_CXHTHUMB          13
#else
#define SYSMETRICS_CXDLGFRAME         4
#define SYSMETRICS_CYDLGFRAME         4
#define SYSMETRICS_CYVTHUMB          16
#define SYSMETRICS_CXHTHUMB          16
#endif
#define SYSMETRICS_CXICON            32
#define SYSMETRICS_CYICON            32
#define SYSMETRICS_CXCURSOR          32
#define SYSMETRICS_CYCURSOR          32
#ifdef WIN_95_LOOK
#define SYSMETRICS_CYVSCROLL         14
#define SYSMETRICS_CXHSCROLL         14
#define SYSMETRICS_CXMIN            112
#define SYSMETRICS_CYMIN             27
#else
#define SYSMETRICS_CYVSCROLL         16
#define SYSMETRICS_CXHSCROLL         16
#define SYSMETRICS_CXMIN            100
#define SYSMETRICS_CYMIN             28
#endif
#ifdef WIN_95_LOOK
#define SYSMETRICS_CXMINTRACK       112
#define SYSMETRICS_CYMINTRACK        27
#else
#define SYSMETRICS_CXMINTRACK       100
#define SYSMETRICS_CYMINTRACK        28
#endif
#endif 0

/* Some non-constant system metrics */
#define SYSMETRICS_CXSCREEN             sysMetrics[SM_CXSCREEN]             /* 0 */
#define SYSMETRICS_CYSCREEN             sysMetrics[SM_CYSCREEN]             /* 1 */
#define SYSMETRICS_CXVSCROLL            sysMetrics[SM_CXVSCROLL]            /* 2 */
#define SYSMETRICS_CYHSCROLL            sysMetrics[SM_CYHSCROLL]            /* 3 */
#define SYSMETRICS_CYCAPTION            sysMetrics[SM_CYCAPTION]            /* 4 */
#define SYSMETRICS_CXBORDER             sysMetrics[SM_CXBORDER]             /* 5 */
#define SYSMETRICS_CYBORDER             sysMetrics[SM_CYBORDER]             /* 6 */
#define SYSMETRICS_CXDLGFRAME           sysMetrics[SM_CXDLGFRAME]           /* 7 */
#define SYSMETRICS_CYDLGFRAME           sysMetrics[SM_CYDLGFRAME]           /* 8 */
#define SYSMETRICS_CYVTHUMB             sysMetrics[SM_CYVTHUMB]             /* 9 */
#define SYSMETRICS_CXHTHUMB             sysMetrics[SM_CXHTHUMB]             /* 10 */
#define SYSMETRICS_CXICON               sysMetrics[SM_CXICON]               /* 11 */
#define SYSMETRICS_CYICON               sysMetrics[SM_CYICON]               /* 12 */
#define SYSMETRICS_CXCURSOR             sysMetrics[SM_CXCURSOR]             /* 13 */
#define SYSMETRICS_CYCURSOR             sysMetrics[SM_CYCURSOR]             /* 14 */
#define SYSMETRICS_CYMENU               sysMetrics[SM_CYMENU]               /* 15 */
#define SYSMETRICS_CXFULLSCREEN         sysMetrics[SM_CXFULLSCREEN]         /* 16 */
#define SYSMETRICS_CYFULLSCREEN         sysMetrics[SM_CYFULLSCREEN]         /* 17 */
#define SYSMETRICS_CYKANJIWINDOW        sysMetrics[SM_CYKANJIWINDOW]        /* 18 */
#define SYSMETRICS_MOUSEPRESENT         sysMetrics[SM_MOUSEPRESENT]         /* 19 */
#define SYSMETRICS_CYVSCROLL            sysMetrics[SM_CYVSCROLL]            /* 20 */
#define SYSMETRICS_CXHSCROLL            sysMetrics[SM_CXHSCROLL]            /* 21 */
#define SYSMETRICS_DEBUG                sysMetrics[SM_DEBUG]                /* 22 */
#define SYSMETRICS_SWAPBUTTON           sysMetrics[SM_SWAPBUTTON]           /* 23 */
#define SYSMETRICS_RESERVED1            sysMetrics[SM_RESERVED1]            /* 24 */
#define SYSMETRICS_RESERVED2            sysMetrics[SM_RESERVED2]            /* 25 */
#define SYSMETRICS_RESERVED3            sysMetrics[SM_RESERVED3]            /* 26 */
#define SYSMETRICS_RESERVED4            sysMetrics[SM_RESERVED4]            /* 27 */
#define SYSMETRICS_CXMIN                sysMetrics[SM_CXMIN]                /* 28 */
#define SYSMETRICS_CYMIN                sysMetrics[SM_CYMIN]                /* 29 */
#define SYSMETRICS_CXSIZE               sysMetrics[SM_CXSIZE]               /* 30 */
#define SYSMETRICS_CYSIZE               sysMetrics[SM_CYSIZE]               /* 31 */
#define SYSMETRICS_CXFRAME              sysMetrics[SM_CXFRAME]              /* 32 */
#define SYSMETRICS_CYFRAME              sysMetrics[SM_CYFRAME]              /* 33 */
#define SYSMETRICS_CXMINTRACK           sysMetrics[SM_CXMINTRACK]           /* 34 */
#define SYSMETRICS_CYMINTRACK           sysMetrics[SM_CYMINTRACK]           /* 35 */
#define SYSMETRICS_CXDOUBLECLK          sysMetrics[SM_CXDOUBLECLK]          /* 36 */
#define SYSMETRICS_CYDOUBLECLK          sysMetrics[SM_CYDOUBLECLK]          /* 37 */
#define SYSMETRICS_CXICONSPACING        sysMetrics[SM_CXICONSPACING]        /* 38 */
#define SYSMETRICS_CYICONSPACING        sysMetrics[SM_CYICONSPACING]        /* 39 */
#define SYSMETRICS_MENUDROPALIGNMENT    sysMetrics[SM_MENUDROPALIGNMENT]    /* 40 */
#define SYSMETRICS_PENWINDOWS           sysMetrics[SM_PENWINDOWS]           /* 41 */
#define SYSMETRICS_DBCSENABLED          sysMetrics[SM_DBCSENABLED]          /* 42 */
#define SYSMETRICS_CMOUSEBUTTONS        sysMetrics[SM_CMOUSEBUTTONS]        /* 43 */
#define SYSMETRICS_CXFIXEDFRAME         sysMetrics[SM_CXDLGFRAME]           /* win40 name change */
#define SYSMETRICS_CYFIXEDFRAME         sysMetrics[SM_CYDLGFRAME]           /* win40 name change */
#define SYSMETRICS_CXSIZEFRAME          sysMetrics[SM_CXFRAME]              /* win40 name change */
#define SYSMETRICS_CYSIZEFRAME          sysMetrics[SM_CYFRAME]              /* win40 name change */
#define SYSMETRICS_SECURE               sysMetrics[SM_SECURE]               /* 44 */
#define SYSMETRICS_CXEDGE               sysMetrics[SM_CXEDGE]               /* 45 */
#define SYSMETRICS_CYEDGE               sysMetrics[SM_CYEDGE]               /* 46 */
#define SYSMETRICS_CXMINSPACING         sysMetrics[SM_CXMINSPACING]         /* 47 */
#define SYSMETRICS_CYMINSPACING         sysMetrics[SM_CYMINSPACING]         /* 48 */
#define SYSMETRICS_CXSMICON             sysMetrics[SM_CXSMICON]             /* 49 */
#define SYSMETRICS_CYSMICON             sysMetrics[SM_CYSMICON]             /* 50 */
#define SYSMETRICS_CYSMCAPTION          sysMetrics[SM_CYSMCAPTION]          /* 51 */
#define SYSMETRICS_CXSMSIZE             sysMetrics[SM_CXSMSIZE]             /* 52 */
#define SYSMETRICS_CYSMSIZE             sysMetrics[SM_CYSMSIZE]             /* 53 */
#define SYSMETRICS_CXMENUSIZE           sysMetrics[SM_CXMENUSIZE]           /* 54 */
#define SYSMETRICS_CYMENUSIZE           sysMetrics[SM_CYMENUSIZE]           /* 55 */
#define SYSMETRICS_ARRANGE              sysMetrics[SM_ARRANGE]              /* 56 */
#define SYSMETRICS_CXMINIMIZED          sysMetrics[SM_CXMINIMIZED]          /* 57 */
#define SYSMETRICS_CYMINIMIZED          sysMetrics[SM_CYMINIMIZED]          /* 58 */
#define SYSMETRICS_CXMAXTRACK           sysMetrics[SM_CXMAXTRACK]           /* 59 */
#define SYSMETRICS_CYMAXTRACK           sysMetrics[SM_CYMAXTRACK]           /* 60 */
#define SYSMETRICS_CXMAXIMIZED          sysMetrics[SM_CXMAXIMIZED]          /* 61 */
#define SYSMETRICS_CYMAXIMIZED          sysMetrics[SM_CYMAXIMIZED]          /* 62 */
#define SYSMETRICS_NETWORK              sysMetrics[SM_NETWORK]              /* 63 */
#define SYSMETRICS_CLEANBOOT            sysMetrics[SM_CLEANBOOT]            /* 67 */
#define SYSMETRICS_CXDRAG               sysMetrics[SM_CXDRAG]               /* 68 */
#define SYSMETRICS_CYDRAG               sysMetrics[SM_CYDRAG]               /* 69 */
#define SYSMETRICS_SHOWSOUNDS           sysMetrics[SM_SHOWSOUNDS]           /* 70 */

/* Use the following instead of sysMetrics[SM_CXMENUCHECK] GetMenuCheckMarkDimensions()! */
#define SYSMETRICS_CXMENUCHECK          sysMetrics[SM_CXMENUCHECK]          /* 71 */
#define SYSMETRICS_CYMENUCHECK          sysMetrics[SM_CYMENUCHECK]          /* 72 */

#define SYSMETRICS_SLOWMACHINE          sysMetrics[SM_SLOWMACHINE]          /* 73 */
#define SYSMETRICS_MIDEASTENABLED       sysMetrics[SM_MIDEASTENABLED]       /* 74 */
#define SYSMETRICS_MOUSEWHEELPRESENT    sysMetrics[SM_MOUSEWHEELPRESENT]    /* 75 */

#define SYSMETRICS_CXVIRTUALSCREEN	sysMetrics[SM_CXVIRTUALSCREEN]	    /* 77 */
#define SYSMETRICS_CYVIRTUALSCREEN	sysMetrics[SM_CYVIRTUALSCREEN]	    /* 77 */
#define SYSMETRICS_YVIRTUALSCREEN	sysMetrics[SM_YVIRTUALSCREEN]	    /* 78 */
#define SYSMETRICS_XVIRTUALSCREEN	sysMetrics[SM_XVIRTUALSCREEN]	    /* 79 */
#define SYSMETRICS_CMONITORS		sysMetrics[SM_CMONITORS]	    /* 81 */
#define SYSMETRICS_SAMEDISPLAYFORMAT	sysMetrics[SM_SAMEDISPLAYFORMAT]    /* 82 */

#undef SM_CMETRICS
#define SM_CMETRICS	(83)

extern short sysMetrics[SM_CMETRICS+1];


#endif  /* __WINE_SYSMETRICS_H */
