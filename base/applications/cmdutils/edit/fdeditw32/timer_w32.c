#include <windows.h>

static char timerused[3] = {0, 0, 0};
static long unsigned int timerend[3] = {0, 0, 0};
static long unsigned int timerstart[3] = {0, 0, 0};

/* ------- timer interrupt service routine ------- */
/* More complex countdown handling by Eric Auer */
/* Allows us to work without hooking intr. 0x08 */
int timed_out(int timer)		/* was: countdown 0? */
{
    if ((timer > 2) || (timer < 0))
        return -1;			/* invalid -> always elapsed */

    if (timerused[timer] == 0)		/* not active at all? */
        return 0;

    if (timerused[timer] == 2)		/* timeout already known? */
        return 1;

    if (GetTickCount() < timerstart[timer] ||
        GetTickCount() >= timerend[timer])
        {
        timerused[timer] = 2;		/* countdown elapsed */
        return 1;
        }

    return 0;				/* still waiting */

}

int timer_running(int timer)		/* was: countdown > 0? */
{
    if ((timer > 2) || (timer < 0))
        return 0;			/* invalid -> never running */

    if (timerused[timer] == 1)		/* running? */
        {
        return (1 - timed_out(timer));  /* if not elapsed, running */
        }
    else
        return 0;                       /* certainly not running */

}

int timer_disabled(int timer)		/* was: countdown -1? */
{
    if ((timer > 2) || (timer < 0))
        return 1;			/* invalid -> always disabled */

    return (timerused[timer] == 0);

}

void disable_timer(int timer)		/* was: countdown = -1 */
{
    if ((timer > 2) || (timer < 0))
        return;

    timerused[timer] = 0;

}

void set_timer(int timer, int secs)
{
    if ((timer > 2) || (timer < 0))
        return;

    timerstart[timer]=GetTickCount();
    timerend[timer]=timerstart[timer] + (secs*1000) + 1;
    timerused[timer]=1;                 /* mark as running */

}

void set_timer_ticks(int timer, int ticks)
{
    if ((timer > 2) || (timer < 0))
        return;

    timerstart[timer]=GetTickCount();
    timerend[timer]=timerstart[timer] + ticks*55;
    timerused[timer]=1;                 /* mark as running */
}
