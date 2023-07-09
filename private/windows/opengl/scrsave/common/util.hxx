/******************************Module*Header*******************************\
* Module Name: util.hxx
*
* Utility classes
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __util_hxx__
#define __util_hxx__

class SS_TIME {
public:
    SS_TIME()   { Zero(); };
    double  Seconds();
    void    Update();
    void    Zero();
    SS_TIME operator+( SS_TIME addTime );
    SS_TIME operator-( SS_TIME subTime );
    SS_TIME operator+=( SS_TIME addTime );
    SS_TIME operator-=( SS_TIME subTime );
    operator double() { return seconds; };
    operator float() { return (float) seconds; };
    operator int() { return (int) (seconds + 0.5); };
private:
    double seconds;
};

class SS_TIMER {
public:
    SS_TIMER() { Reset(); };
    void    Start();
    SS_TIME Stop();
    void    Reset();
    SS_TIME ElapsedTime();
private:
    SS_TIME startTime;
    SS_TIME elapsed;
};

#endif // __util_hxx__
