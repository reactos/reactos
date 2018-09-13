/******************************Module*Header*******************************\
* Module Name: util.cxx
*
* Misc. utility functions
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <sys/timeb.h>
#include <GL/gl.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <math.h>
#include "ssintrnl.hxx"
#include "util.hxx"


/******************************Public*Routine******************************\
* SS_TIME class
*
\**************************************************************************/

void
SS_TIME::Update()
{
    struct _timeb time;

    _ftime( &time );
    seconds = time.time + time.millitm/1000.0;
}

void
SS_TIME::Zero()
{
    seconds = 0.0;
}

double
SS_TIME::Seconds()
{
    return seconds;
}

SS_TIME
SS_TIME::operator+( SS_TIME addTime )
{
    return( *this += addTime );
}


SS_TIME
SS_TIME::operator-( SS_TIME subTime )
{
    return( *this -= subTime );
}

SS_TIME
SS_TIME::operator+=( SS_TIME addTime )
{
    seconds += addTime.seconds;
    return *this;
}

SS_TIME
SS_TIME::operator-=( SS_TIME subTime )
{
    seconds -= subTime.seconds;
    return *this;
}

/******************************Public*Routine******************************\
* SS_TIMER class
*
\**************************************************************************/

void
SS_TIMER::Start()
{
    startTime.Update();
}

SS_TIME
SS_TIMER::Stop()
{
    elapsed.Update();
    return elapsed - startTime;
}

void
SS_TIMER::Reset()
{
    elapsed.Zero();
}

SS_TIME
SS_TIMER::ElapsedTime()
{
    elapsed.Update();
    return elapsed - startTime;
}

/******************************Public*Routine******************************\
* ss_Rand
*
* Generates integer random number 0..(max-1)
*
\**************************************************************************/

int ss_iRand( int max )
{
    return (int) ( max * ( ((float)rand()) / ((float)(RAND_MAX+1)) ) );
}

/******************************Public*Routine******************************\
* ss_Rand2
*
* Generates integer random number min..max
*
\**************************************************************************/

int ss_iRand2( int min, int max )
{
    if( min == max )
        return min;
    else if( max < min ) {
        int temp = min;
        min = max;
        max = temp;
    }

    return min + (int) ( (max-min+1) * ( ((float)rand()) / ((float)(RAND_MAX+1)) ) );
}

/******************************Public*Routine******************************\
* ss_fRand
*
* Generates float random number min...max
*
\**************************************************************************/

FLOAT ss_fRand( FLOAT min, FLOAT max )
{
    FLOAT diff;

    diff = max - min;
    return min + ( diff * ( ((float)rand()) / ((float)(RAND_MAX)) ) );
}

/******************************Public*Routine******************************\
* ss_RandInit
*
* Initializes the randomizer
*
\**************************************************************************/

void ss_RandInit( void )
{
    struct _timeb time;

    _ftime( &time );
    srand( time.millitm );

    for( int i = 0; i < 10; i ++ )
        rand();
}

#if DBG
/******************************Public*Routine******************************\
* DbgPrint
*
* Formatted string output to the debugger.
*
* History:
*  26-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG
DbgPrint(PCH DebugMessage, ...)
{
    va_list ap;
    char buffer[256];

    va_start(ap, DebugMessage);

    vsprintf(buffer, DebugMessage, ap);

    OutputDebugStringA(buffer);

    va_end(ap);

    return(0);
}
#endif
