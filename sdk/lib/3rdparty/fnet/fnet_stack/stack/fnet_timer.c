/**************************************************************************
*
* Copyright 2008-2018 by Andrey Butok. FNET Community
*
***************************************************************************
*
*  Licensed under the Apache License, Version 2.0 (the "License"); you may
*  not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
*  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
***************************************************************************
*
*  SW timer implementation.
*
***************************************************************************/

#include "fnet.h"
#include "fnet_timer_prv.h"
#include "fnet_netbuf_prv.h"
#include "fnet_stack_prv.h"

#if FNET_CFG_DEBUG_TIMER && FNET_CFG_DEBUG
    #define FNET_DEBUG_TIMER   FNET_DEBUG
#else
    #define FNET_DEBUG_TIMER(...)
#endif

/* Queue of the software timers*/
struct fnet_net_timer
{
    struct fnet_net_timer *next;            /* Next timer in list.*/
    fnet_time_t timer_start_ms;             /* Timer start time. */
    fnet_time_t timer_timeout_ms;           /* Timer timeout. */
    void (*handler)(fnet_uint32_t cookie);  /* Timer handler. */
    fnet_uint32_t cookie;                   /* Handler Cookie. */
};

#if FNET_CFG_TIME
    static void _fnet_time_set(time_t sec);
#endif

static struct fnet_net_timer    *_fnet_timer_head;

#if !FNET_CFG_TIMER_ALT  /* Bare-metal timer */
    static volatile fnet_time_t     _fnet_timer_counter_ms;
#endif

#if FNET_CFG_TIME
    static time_t       _fnet_time_start;
    static fnet_time_t  _fnet_timer_start_ms;
#endif

#if FNET_CFG_TIMER_ALT
    const fnet_timer_api_t  *_fnet_timer_api = FNET_NULL;
#endif

/************************************************************************
* DESCRIPTION: Starts TCP/IP hardware timer. delay_ms - period of timer (ms)
*************************************************************************/
fnet_return_t _fnet_timer_init( fnet_time_t period_ms )
{
    fnet_return_t result;

#if FNET_CFG_TIME
    _fnet_time_start = 0;
    _fnet_timer_start_ms = 0;
#endif

#if FNET_CFG_TIMER_ALT /* Application Timer API */
    if((_fnet_timer_api == FNET_NULL) || (_fnet_timer_api->timer_get_ms == FNET_NULL))
    {
        result = FNET_ERR;
    }
    else
    {
        result = FNET_OK;
    }
#else /* Init FNET bare-metal driver */
    _fnet_timer_counter_ms = 0u;                /* Reset RTC counter. */
    result = FNET_HW_TIMER_INIT(period_ms);     /* Start HW timer. */
#endif

    return result;
}

/************************************************************************
* DESCRIPTION: Frees the memory, which was allocated for all
*              TCP/IP timers, and removes hardware timer
*************************************************************************/
void _fnet_timer_release( void )
{
    struct fnet_net_timer *tmp_tl;

#if !FNET_CFG_TIMER_ALT
    FNET_HW_TIMER_RELEASE();
#endif

    while(_fnet_timer_head != 0)
    {
        tmp_tl = _fnet_timer_head->next;

        _fnet_free(_fnet_timer_head);

        _fnet_timer_head = tmp_tl;
    }
}

/************************************************************************
* DESCRIPTION: This function returns current value of the timer
* in milliseconds.
*************************************************************************/
fnet_time_t fnet_timer_get_ms( void )
{
    fnet_time_t result;

#if FNET_CFG_TIMER_ALT /* Application Timer API */
    result = _fnet_timer_api->timer_get_ms();
#else /* Bare-metal timer */
    result = _fnet_timer_counter_ms;
#endif

    return result;
}

/************************************************************************
* DESCRIPTION: This function increments current value of the RTC counter.
*************************************************************************/
#if !FNET_CFG_TIMER_ALT  /* Bare-metal timer */
void _fnet_timer_ticks_inc( void )
{
    _fnet_timer_counter_ms += FNET_TIMER_PERIOD_MS;

#if FNET_CFG_DEBUG_TIMER && FNET_CFG_DEBUG
    /* Print once per second */
    if((_fnet_timer_counter_ms % FNET_TIMER_MS_IN_SEC) == 0)
    {
        FNET_DEBUG_TIMER("!");
    }
#endif
}
#endif

/************************************************************************
* DESCRIPTION: Handles timer interrupts
*************************************************************************/
#if !FNET_CFG_TIMER_ALT
void _fnet_timer_handler_bottom(void *cookie)
{
    FNET_COMP_UNUSED_ARG(cookie);

    _fnet_timer_poll();
}
#endif

/************************************************************************
* DESCRIPTION: Timer polling function.
*************************************************************************/
void fnet_timer_poll(void)
{
    _fnet_stack_mutex_lock();
    _fnet_timer_poll();
    _fnet_stack_mutex_unlock();
}
void _fnet_timer_poll(void)
{
    struct fnet_net_timer *timer;

    fnet_isr_lock();

    timer = _fnet_timer_head;

    while(timer)
    {
        if((fnet_timer_get_ms() - timer->timer_start_ms) >= timer->timer_timeout_ms)
        {
            timer->timer_start_ms = fnet_timer_get_ms();

            if(timer->handler)
            {
                timer->handler(timer->cookie);
            }
        }

        timer = timer->next;
    }

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: Creates new software timer with the period
*************************************************************************/
fnet_timer_desc_t _fnet_timer_new( fnet_time_t period_ms, void (*handler)(fnet_uint32_t cookie), fnet_uint32_t cookie )
{
    struct fnet_net_timer *timer = FNET_NULL;

    if( period_ms && handler )
    {
        timer = (struct fnet_net_timer *)_fnet_malloc_zero(sizeof(struct fnet_net_timer));

        if(timer)
        {
            timer->next = _fnet_timer_head;

            _fnet_timer_head = timer;

            timer->timer_start_ms = fnet_timer_get_ms();
            timer->timer_timeout_ms = period_ms;
            timer->handler = handler;
            timer->cookie = cookie;
        }
    }

    return (fnet_timer_desc_t)timer;
}

/************************************************************************
* DESCRIPTION: Frees software timer, which is pointed by tl_ptr
*************************************************************************/
void _fnet_timer_free( fnet_timer_desc_t timer )
{
    struct fnet_net_timer *tl = (struct fnet_net_timer *)timer;
    struct fnet_net_timer *tl_temp;

    if(tl)
    {
        if(tl == _fnet_timer_head)
        {
            _fnet_timer_head = _fnet_timer_head->next;
        }
        else
        {
            tl_temp = _fnet_timer_head;

            while(tl_temp->next != tl)
            {
                tl_temp = tl_temp->next;
            }

            tl_temp->next = tl->next;
        }

        _fnet_free(tl);
    }
}

/************************************************************************
* DESCRIPTION: Do delay for a given number of milliseconds.
*************************************************************************/
void fnet_timer_delay( fnet_time_t delay_ms )
{
#if FNET_CFG_TIMER_ALT
    if(_fnet_timer_api->timer_delay) /* Application specific delay */
    {
        _fnet_timer_api->timer_delay(delay_ms);
    }
    else
#endif
    {
        fnet_time_t start_ms = fnet_timer_get_ms();

        if(delay_ms == 0) /* set minimum value */
        {
            delay_ms = 1;
        }

        while((fnet_timer_get_ms() - start_ms) < delay_ms)
        {}
    }
}

#if FNET_CFG_TIME
/************************************************************************
* DESCRIPTION: Set current time as the elapsed time in seconds since 00:00:00, January 1, 1970.
************************************************************************/
void fnet_time_set(time_t sec)
{
    _fnet_stack_mutex_lock();
    _fnet_time_set(sec);
    _fnet_stack_mutex_unlock();
}
static void _fnet_time_set(time_t sec)
{
    /* Get the timer counter value in seconds */
    _fnet_timer_start_ms = fnet_timer_get_ms();
    _fnet_time_start = sec;
}
/************************************************************************
* DESCRIPTION: Get current time as the elapsed time in seconds since 00:00:00, January 1, 1970.
************************************************************************/
time_t fnet_time(time_t *sec)
{
    time_t result;

    if(_fnet_time_start == 0) /* Never initialized before */
    {
        time_t      time_buld; /* The number of seconds elapsed since the Epoch, 1970-01-01 00:00:00 +0000 (UTC).*/
        struct tm   date_time;

        /* Get buld time as the elapsed time in seconds since 00:00:00, January 1, 1970.*/

        /* __DATE__: This macro expands to a string constant that describes the date on which the preprocessor is being run. The string constant contains eleven characters and looks
        like "Feb 12 1996". If the day of the month is less than 10, it is padded with a space on the left.
        __TIME__: This macro expands to a string constant that describes the time at which the preprocessor is being run. The string constant contains eight characters and looks like "23:59:01"*/

        /* Seconds after the minute	0-61 */
        date_time.tm_sec = ((__TIME__[0] == '?') ? 0 :  ((__TIME__[6] - '0') * 10 + __TIME__[7] - '0'));
        /* Minutes after the hour	0-59 */
        date_time.tm_min = ((__TIME__[0] == '?') ? 0 :  ((__TIME__[3] - '0') * 10 + __TIME__[4] - '0'));
        /* Hours since midnight	0-23 */
        date_time.tm_hour = ((__TIME__[0] == '?') ? 0 :  ((__TIME__[0] - '0') * 10 + __TIME__[1] - '0'));
        /* Day of the month	1-31 */
        date_time.tm_mday = ((__DATE__[0] == '?') ? 14 : (((__DATE__[4] >= '0') ? (__DATE__[4] - '0') * 10 : 0) + (__DATE__[5] - '0')));
        /* Months since January	0-11 */
        date_time.tm_mon = ((__DATE__[0] == '?') ? 6 : ((__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n') ?  1 :
                            (__DATE__[0] == 'F') ?  2 :
                            (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r') ?  3 :
                            (__DATE__[0] == 'A' && __DATE__[1] == 'p') ?  4 :
                            (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y') ?  5 :
                            (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n') ?  6 :
                            (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l') ?  7 :
                            (__DATE__[0] == 'A' && __DATE__[1] == 'u') ?  8 :
                            (__DATE__[0] == 'S') ?  9 :
                            (__DATE__[0] == 'O') ? 10 :
                            (__DATE__[0] == 'N') ? 11 :
                            (__DATE__[0] == 'D') ? 12 : 6 )) - 1;
        /* Years since 1900 */
        date_time.tm_year = ((__DATE__[0] == '?') ? 2018 : (((__DATE__[ 7] - '0') * 1000) + ((__DATE__[ 8] - '0') * 100) + ((__DATE__[ 9] - '0') * 10) + (__DATE__[10] - '0'))) - 1900;
        /* Days since Sunday	0-6 */
        date_time.tm_wday = -1;
        /* Days since January 1	0-365 */
        date_time.tm_yday = -1;
        /* Daylight Saving Time flag */
        date_time.tm_isdst = -1;

        time_buld = mktime(&date_time);

        fnet_time_set(time_buld);
    }

    result = _fnet_time_start + ((fnet_timer_get_ms() - _fnet_timer_start_ms) / FNET_TIMER_MS_IN_SEC);

    if(sec)
    {
        *sec = result;
    }
    return result;
}
#endif /* FNET_CFG_TIME */
