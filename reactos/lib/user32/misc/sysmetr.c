/*
 * System metrics functions
 *
 * Copyright 1994 Alexandre Julliard
 *
 */

#include <stdio.h>
#include <windows.h>
#include <user32/sysmetr.h>

#undef SM_CMETRICS
#define SM_CMETRICS	(83)

short sysMetrics[SM_CMETRICS+1] = { 

    800, // [SM_CXSCREEN]             /* 0 */                
    600, // [SM_CYSCREEN]             /* 1 */                
    19,  // [SM_CXVSCROLL]            /* 2 */                
    17,  // [SM_CYHSCROLL]            /* 3 */                
    19,  // [SM_CYCAPTION]            /* 4 */                
    1,   // [SM_CXBORDER]             /* 5 */                
    1,   // [SM_CYBORDER]             /* 6 */                
    3,   // [SM_CXDLGFRAME]           /* 7 */                
    3,   // [SM_CYDLGFRAME]           /* 8 */                
    17,  // [SM_CYVTHUMB]             /* 9 */    sysMetrics[SM_CXVSCROLL] - 1;            
    17,  // [SM_CXHTHUMB]             /* 10 */   sysMetrics[SM_CXVSCROLL] - 1;            
    32,  // [SM_CXICON]               /* 11 */               
    32,  // [SM_CYICON]               /* 12 */               
    32,  // [SM_CXCURSOR]             /* 13 */               
    32,  // [SM_CYCURSOR]             /* 14 */               
    19,  // [SM_CYMENU]               /* 15 */               
    800, // [SM_CXFULLSCREEN]         /* 16 */               
    600, // [SM_CYFULLSCREEN]         /* 17 */               
    0,   // [SM_CYKANJIWINDOW]        /* 18 */               
    0,   // [SM_MOUSEPRESENT]         /* 19 */               
    0,   // [SM_CYVSCROLL]            /* 20 */               
    0,   // [SM_CXHSCROLL]            /* 21 */               
    0,   // [SM_DEBUG]                /* 22 */               
    0,   // [SM_SWAPBUTTON]           /* 23 */               
    0,   // [SM_RESERVED1]            /* 24 */               
    0,   // [SM_RESERVED2]            /* 25 */               
    0,   // [SM_RESERVED3]            /* 26 */               
    0,   // [SM_RESERVED4]            /* 27 */               
    112, // [SM_CXMIN]                /* 28 */               
    27,  // [SM_CYMIN]                /* 29 */               
    17,  // [SM_CXSIZE]               /* 30 */               
    17,  // [SM_CYSIZE]               /* 31 */               
    4,   // [SM_CXFRAME]              /* 32 */               
    4,   // [SM_CYFRAME]              /* 33 */               
    0,   // [SM_CXMINTRACK]           /* 34 */               
    0,   // [SM_CYMINTRACK]           /* 35 */               
    0,   // [SM_CXDOUBLECLK]          /* 36 */               
    0,   // [SM_CYDOUBLECLK]          /* 37 */               
    0,   // [SM_CXICONSPACING]        /* 38 */               
    0,   // [SM_CYICONSPACING]        /* 39 */               
    0,   // [SM_MENUDROPALIGNMENT]    /* 40 */               
    0,   // [SM_PENWINDOWS]           /* 41 */               
    0,   // [SM_DBCSENABLED]          /* 42 */               
    0,   // [SM_CMOUSEBUTTONS]        /* 43 */               
         // [SM_CXDLGFRAME]           /* win40 name change */
         // [SM_CYDLGFRAME]           /* win40 name change */
         // [SM_CXFRAME]              /* win40 name change */
         // [SM_CYFRAME]              /* win40 name change */
    0,   // [SM_SECURE]               /* 44 */               
    2,   // [SM_CXEDGE]               /* 45 */   // sysMetrics[SM_CXBORDER] + 1;            
    2,   // [SM_CYEDGE]               /* 46 */               
    0,   // [SM_CXMINSPACING]         /* 47 */               
    0,   // [SM_CYMINSPACING]         /* 48 */               
    14,   // [SM_CXSMICON]             /* 49 */               
    14,   // [SM_CYSMICON]             /* 50 */               
    0,   // [SM_CYSMCAPTION]          /* 51 */               
    0,   // [SM_CXSMSIZE]             /* 52 */               
    0,   // [SM_CYSMSIZE]             /* 53 */               
    19,   // [SM_CXMENUSIZE]           /* 54 */               
    19,   // [SM_CYMENUSIZE]           /* 55 */               
    8,   // [SM_ARRANGE]              /* 56 */               
    160,   // [SM_CXMINIMIZED]          /* 57 */               
    24,   // [SM_CYMINIMIZED]          /* 58 */               
    0,   // [SM_CXMAXTRACK]           /* 59 */               
    0,   // [SM_CYMAXTRACK]           /* 60 */               
    0,   // [SM_CXMAXIMIZED]          /* 61 */               
    0,   // [SM_CYMAXIMIZED]          /* 62 */               
    3,   // [SM_NETWORK]              /* 63 */               
    0,   // [SM_CLEANBOOT]            /* 67 */               
    2,   // [SM_CXDRAG]               /* 68 */               
    2,   // [SM_CYDRAG]               /* 69 */               
    0,   // [SM_SHOWSOUNDS]           /* 70 */                     
    2,   // [SM_CXMENUCHECK]          /* 71 */               
    2,   // [SM_CYMENUCHECK]          /* 72 */               
    0,   // [SM_SLOWMACHINE]          /* 73 */               
    0,   // [SM_MIDEASTENABLED]       /* 74 */               
    0,   // [SM_MOUSEWHEELPRESENT]    /* 75 */               
    800,   // [SM_CXVIRTUALSCREEN]      /* 76 */               
    600,   // [SM_CYVIRTUALSCREEN]      /* 77 */               
    0,   // [SM_YVIRTUALSCREEN]	      /* 78 */               
    0,   // [SM_XVIRTUALSCREEN]	      /* 79 */               
    1,   // [SM_CMONITORS]	      /* 81 */               
    1    // [SM_SAMEDISPLAYFORMAT]    /* 82 */               
   
   };
   


int STDCALL GetSystemMetrics(int  nIndex)
{
	if ( nIndex >= 0 && nIndex <= SM_CMETRICS+1 )
		return sysMetrics[nIndex];
	return 0;
}


WINBOOL STDCALL SystemParametersInfo(UINT  uiAction,
    UINT  uiParam, PVOID  pvParam, UINT  fWinIni )
{
	return FALSE;
}

#if 0
   /***********************************************************************
    *           SYSMETRICS_Init
    *
    * Initialisation of the system metrics array.
    *
    * Differences in return values between 3.1 and 95 apps under Win95 (FIXME ?):
    * SM_CXVSCROLL        x+1      x
    * SM_CYHSCROLL        x+1      x
    * SM_CXDLGFRAME       x-1      x
    * SM_CYDLGFRAME       x-1      x
    * SM_CYCAPTION        x+1      x
    * SM_CYMENU           x-1      x
    * SM_CYFULLSCREEN     x-1      x
    * 
    * (collides with TWEAK_WineLook sometimes,
    * so changing anything might be difficult) 
    */
   
   
   
   void SYSMETRICS_Init(void)
   {
       sysMetrics[SM_CXCURSOR] = 32;
       sysMetrics[SM_CYCURSOR] = 32;
       sysMetrics[SM_CXSCREEN] = screenWidth;
       sysMetrics[SM_CYSCREEN] = screenHeight;
       sysMetrics[SM_CXVSCROLL] =
   	PROFILE_GetWineIniInt("Tweak.Layout", "ScrollBarWidth", 16) + 1;
       sysMetrics[SM_CYHSCROLL] = sysMetrics[SM_CXVSCROLL];
       if (TWEAK_WineLook > WIN31_LOOK)
   	sysMetrics[SM_CYCAPTION] =
   	    PROFILE_GetWineIniInt("Tweak.Layout", "CaptionHeight", 19);
       else
   	sysMetrics[SM_CYCAPTION] = 2 +
   	    PROFILE_GetWineIniInt("Tweak.Layout", "CaptionHeight", 18);
       sysMetrics[SM_CXBORDER] = 1;
       sysMetrics[SM_CYBORDER] = sysMetrics[SM_CXBORDER];
       sysMetrics[SM_CXDLGFRAME] =
   	PROFILE_GetWineIniInt("Tweak.Layout", "DialogFrameWidth",
   			      (TWEAK_WineLook > WIN31_LOOK) ? 3 : 4);
       sysMetrics[SM_CYDLGFRAME] = sysMetrics[SM_CXDLGFRAME];
 */    sysMetrics[SM_CYVTHUMB] = sysMetrics[SM_CXVSCROLL] - 1;
 */    sysMetrics[SM_CXHTHUMB] = sysMetrics[SM_CYVTHUMB];
 */    sysMetrics[SM_CXICON] = 32;
 */    sysMetrics[SM_CYICON] = 32;
       if (TWEAK_WineLook > WIN31_LOOK)
   	sysMetrics[SM_CYMENU] =
   	    PROFILE_GetWineIniInt("Tweak.Layout", "MenuHeight", 19);
       else
   	sysMetrics[SM_CYMENU] =
   	    PROFILE_GetWineIniInt("Tweak.Layout", "MenuHeight", 18);
       sysMetrics[SM_CXFULLSCREEN] = sysMetrics[SM_CXSCREEN];
       sysMetrics[SM_CYFULLSCREEN] =
   	sysMetrics[SM_CYSCREEN] - sysMetrics[SM_CYCAPTION];
       sysMetrics[SM_CYKANJIWINDOW] = 0;
       sysMetrics[SM_MOUSEPRESENT] = 1;
       sysMetrics[SM_CYVSCROLL] = sysMetrics[SM_CYVTHUMB];
       sysMetrics[SM_CXHSCROLL] = sysMetrics[SM_CXHTHUMB];
       sysMetrics[SM_DEBUG] = 0;
   
       /* FIXME: The following should look for the registry key to see if the
          buttons should be swapped. */
       sysMetrics[SM_SWAPBUTTON] = 0;
   
       sysMetrics[SM_RESERVED1] = 0;
       sysMetrics[SM_RESERVED2] = 0;
       sysMetrics[SM_RESERVED3] = 0;
       sysMetrics[SM_RESERVED4] = 0;
   
       /* FIXME: The following two are calculated, but how? */
       sysMetrics[SM_CXMIN] = (TWEAK_WineLook > WIN31_LOOK) ? 112 : 100;
       sysMetrics[SM_CYMIN] = (TWEAK_WineLook > WIN31_LOOK) ? 27 : 28;
   
       sysMetrics[SM_CXSIZE] = sysMetrics[SM_CYCAPTION] - 2;
       sysMetrics[SM_CYSIZE] = sysMetrics[SM_CXSIZE];
       sysMetrics[SM_CXFRAME] = GetProfileInt32A("Windows", "BorderWidth", 4);
       sysMetrics[SM_CYFRAME] = sysMetrics[SM_CXFRAME];
       sysMetrics[SM_CXMINTRACK] = sysMetrics[SM_CXMIN];
  sysMetrics[SM_CYMINTRACK] = sysMetrics[SM_CYMIN];
  sysMetrics[SM_CXDOUBLECLK] =
(GetProfileInt32A("Windows", "DoubleClickWidth", 4) + 1) & ~1;
  sysMetrics[SM_CYDOUBLECLK] =
(GetProfileInt32A("Windows","DoubleClickHeight", 4) + 1) & ~1;
          sysMetrics[SM_CXICONSPACING] =
	GetProfileInt32A("Desktop","IconSpacing", 75);
    sysMetrics[SM_CYICONSPACING] =
	GetProfileInt32A("Desktop", "IconVerticalSpacing", 75);
    sysMetrics[SM_MENUDROPALIGNMENT] =
	GetProfileInt32A("Windows", "MenuDropAlignment", 0);
    sysMetrics[SM_PENWINDOWS] = 0;
    sysMetrics[SM_DBCSENABLED] = 0;

    /* FIXME: Need to query X for the following */
    sysMetrics[SM_CMOUSEBUTTONS] = 3;

    sysMetrics[SM_SECURE] = 0;
    sysMetrics[SM_CXEDGE] = sysMetrics[SM_CXBORDER] + 1;
    sysMetrics[SM_CYEDGE] = sysMetrics[SM_CXEDGE];
    sysMetrics[SM_CXMINSPACING] = 160;
    sysMetrics[SM_CYMINSPACING] = 24;
    sysMetrics[SM_CXSMICON] =
	sysMetrics[SM_CYSIZE] - (sysMetrics[SM_CYSIZE] % 2) - 2;
    sysMetrics[SM_CYSMICON] = sysMetrics[SM_CXSMICON];
    sysMetrics[SM_CYSMCAPTION] = 16;
    sysMetrics[SM_CXSMSIZE] = 15;
    sysMetrics[SM_CYSMSIZE] = sysMetrics[SM_CXSMSIZE];
    sysMetrics[SM_CXMENUSIZE] = sysMetrics[SM_CYMENU];
    sysMetrics[SM_CYMENUSIZE] = sysMetrics[SM_CXMENUSIZE];

    /* FIXME: What do these mean? */
    sysMetrics[SM_ARRANGE] = 8;
    sysMetrics[SM_CXMINIMIZED] = 160;
    sysMetrics[SM_CYMINIMIZED] = 24;

    /* FIXME: How do I calculate these? */
    sysMetrics[SM_CXMAXTRACK] = 
	sysMetrics[SM_CXSCREEN] + 4 + 2 * sysMetrics[SM_CXFRAME];
    sysMetrics[SM_CYMAXTRACK] =
	sysMetrics[SM_CYSCREEN] + 4 + 2 * sysMetrics[SM_CYFRAME];
    sysMetrics[SM_CXMAXIMIZED] =
	sysMetrics[SM_CXSCREEN] + 2 * sysMetrics[SM_CXFRAME];
    sysMetrics[SM_CYMAXIMIZED] =
	sysMetrics[SM_CYSCREEN] - 45;
    sysMetrics[SM_NETWORK] = 3;

    /* For the following: 0 = ok, 1 = failsafe, 2 = failsafe + network */
    sysMetrics[SM_CLEANBOOT] = 0;

    sysMetrics[SM_CXDRAG] = 2;
    sysMetrics[SM_CYDRAG] = 2;
    sysMetrics[SM_SHOWSOUNDS] = 0;
    sysMetrics[SM_CXMENUCHECK] = 2;
    sysMetrics[SM_CYMENUCHECK] = 2;

    /* FIXME: Should check the type of processor for the following */
    sysMetrics[SM_SLOWMACHINE] = 0;

    /* FIXME: Should perform a check */
    sysMetrics[SM_MIDEASTENABLED] = 0;

    sysMetrics[SM_MOUSEWHEELPRESENT] = 0;
    
    sysMetrics[SM_CXVIRTUALSCREEN] = sysMetrics[SM_CXSCREEN];
    sysMetrics[SM_CYVIRTUALSCREEN] = sysMetrics[SM_CYSCREEN];
    sysMetrics[SM_XVIRTUALSCREEN] = 0;
    sysMetrics[SM_YVIRTUALSCREEN] = 0;
    sysMetrics[SM_CMONITORS] = 1;
    sysMetrics[SM_SAMEDISPLAYFORMAT] = 1;
    sysMetrics[SM_CMETRICS] = SM_CMETRICS;
}



#endif

