/* FIXME:  This should query the time for caps instead.
           However, this should work fine for our needs */

#include <windows.h>
#include <MMSystem.h>

/* This is what it seems to be on my machine.  (WinXP) */
#define MMSYSTIME_MININTERVAL 1
#define MMSYSTIME_MAXINTERVAL 1000000

MMRESULT timeGetDevCaps(
  LPTIMECAPS ptc,  
  UINT cbtc        
)
{
	ptc->wPeriodMin = 1;
	ptc->wPeriodMax = 1000000;

	return TIMERR_NOERROR;
}

MMRESULT timeBeginPeriod(
  UINT uPeriod  
)
{
	if (uPeriod < MMSYSTIME_MININTERVAL || uPeriod > MMSYSTIME_MAXINTERVAL)
		return TIMERR_NOCANDO;
	else
		return TIMERR_NOERROR;
}

MMRESULT timeEndPeriod(
  UINT uPeriod  
)
{
	if (uPeriod < MMSYSTIME_MININTERVAL || uPeriod > MMSYSTIME_MAXINTERVAL)
		return TIMERR_NOCANDO;
	else
		return TIMERR_NOERROR;
}

/* EOF */
