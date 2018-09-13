/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1991          **/
/********************************************************************/
/* :ts=4 */

/***	timedata.cpp - table of days in each month for time conversion
 */

#include "npcommon.h"
#include <convtime.h>

WORD MonTotal[] = { 0,					// dummy entry for month 0
	0,									// days before Jan 1
	31,									// days before Feb 1
	31+28,								// days before Mar 1
	31+28+31,							// days before Apr 1
	31+28+31+30,						// days before May 1
	31+28+31+30+31,						// days before Jun 1
	31+28+31+30+31+30,					// days before Jul 1
	31+28+31+30+31+30+31,				// days before Aug 1
	31+28+31+30+31+30+31+31, 			// days before Sep 1
	31+28+31+30+31+30+31+31+30,			// days before Oct 1
	31+28+31+30+31+30+31+31+30+31,		// days before Nov 1
	31+28+31+30+31+30+31+31+30+31+30,	// days before Dec 1
	31+28+31+30+31+30+31+31+30+31+30+31	// days before end of year
};
