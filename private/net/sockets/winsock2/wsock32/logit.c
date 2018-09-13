/*****************************************\
 *        Data Logging -- Debug only      *
\*****************************************/
//
// NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


//
// Windows Headers
//
#include <windows.h>

#pragma  hdrstop

#include "logit.h"

#if DBG

int LoggingMode;
time_t  long_time;      // has to be in DS, assumed by time() funcs
int LineCount;

char    *month[] =
{
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
} ;


/*
 -  LogInit
 -
 *  Purpose:
 *  Determines if logging is desired and if so, adds a header to log file.
 *
 *  Parameters:
 *
 */
void LogInit()
{
    FILE    *fp;
    struct  tm  *newtime;
    char    am_pm[] = "a.m.";

    LoggingMode = 0;
    LineCount = 0;

    if ( fp = fopen( "ws2.log", "r+" ) )
    {
        LoggingMode = 1;
        fclose( fp );

        // Get time and date information

        long_time = time( NULL);        /* Get time as long integer. */
        newtime = localtime( &long_time ); /* Convert to local time. */

        if( newtime->tm_hour > 12 )    /* Set up extension. */
            am_pm[0] = 'p';
        if( newtime->tm_hour > 12 )    /* Convert from 24-hour */
            newtime->tm_hour -= 12;    /*   to 12-hour clock.  */
        if( newtime->tm_hour == 0 )    /*Set hour to 12 if midnight. */
            newtime->tm_hour = 12;

        // Write out a header to file

        fp = fopen("ws2.log", "w" );

        fprintf( fp, "Logging information for WinSock 2 API\n" );
        fprintf( fp, "****************************************************\n" );
        fprintf( fp, "\tTime: %d:%02d %s\n\tDate: %s %d, 19%d\n", 
                 newtime->tm_hour, newtime->tm_min, am_pm,
                 month[newtime->tm_mon], newtime->tm_mday,
                 newtime->tm_year );
        fprintf( fp, "****************************************************\n\n" );
        fclose( fp );
    }
}


/*
 -  LogIt
 -
 *  Purpose:
 *  Formats a string and prints it to a log file with handle hLog.
 *
 *  Parameters:
 *  LPSTR - Pointer to string to format
 *  ...   - variable argument list
 */

#ifdef WIN32
#define S16
#else
#define S16 static
#endif

void CDECL LogIt( char * lpszFormat, ... )
{
    FILE *  fp;
    struct  tm  *newtime;
    char    am_pm[] = "a.m.";
#ifndef _ALPHA_
    va_list pArgs = NULL;       // reference to quiet compiler
#else
    va_list pArgs = {NULL,0};
#endif
    S16 char    szLogStr[1024];    
    int     i;

    if ( !LoggingMode )
        return;
    
#ifdef WIN32        // parse parameters and insert in string
    va_start( pArgs, lpszFormat);
    vsprintf(szLogStr, lpszFormat, pArgs);
    va_end(pArgs);

    i = lstrlenA( szLogStr);
#else              // parsing doesn't work, just give them string.
    _fstrcpy( szLogStr, lpszFormat);
    i = _fstrlen( szLogStr);
#endif
    szLogStr[i] = '\n';
    szLogStr[i+1] = '\0';


    // Get time and date information

    long_time = time( NULL);        /* Get time as long integer. */
    newtime = localtime( &long_time ); /* Convert to local time. */

    if( newtime->tm_hour > 12 )    /* Set up extension. */
        am_pm[0] = 'p';
    if( newtime->tm_hour > 12 )    /* Convert from 24-hour */
        newtime->tm_hour -= 12;    /*   to 12-hour clock.  */
    if( newtime->tm_hour == 0 )    /*Set hour to 12 if midnight. */
        newtime->tm_hour = 12;

    if ( LineCount > 10000 )
    {
        fp = fopen( "ws2.log", "w" );
        LineCount = 0;
    }
    else
    {
        fp = fopen( "ws2.log", "a" );
    }
    if( fp)
    {   // if we can't open file, do nothing
        fprintf( fp, "%d:%02d %s - %s",
                 newtime->tm_hour,
                 newtime->tm_min,
                 am_pm,
                 szLogStr );
        // KdPrint(( szLogStr ));
        LineCount++;
        fclose( fp );
    }
}


DWORD LogIn( char * string )
{
    LogIt( "%s", string );
    return GetTickCount();
}


void LogOut( char * string, DWORD InTime )
{
    LogIt( "%s  ---  Duration: %ld milliseconds", string, GetTickCount() - InTime );
}


#endif
