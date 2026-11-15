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
*  FNET Timer API.
*
***************************************************************************/

#ifndef _FNET_TIMER_H

#define _FNET_TIMER_H

/*! @addtogroup fnet_timer
* An application can use the @ref fnet_timer_get_ms() function to get a number of
* milliseconds since the timer startup.
*/

/*! @{ */

/**************************************************************************/ /*!
 * @brief Timer period in milliseconds (period of one timer tick).
 ******************************************************************************/
#define FNET_TIMER_PERIOD_MS        (10U) /* It may be 1-100ms */

/**************************************************************************/ /*!
 * @brief Number of milliseconds in one second.
 ******************************************************************************/
#define FNET_TIMER_MS_IN_SEC        (1000U)

/**************************************************************************/ /*!
 * @brief Number of milliseconds in one minute.
 ******************************************************************************/
#define FNET_TIMER_MS_IN_MIN        (FNET_TIMER_MS_IN_SEC*60U)

/**************************************************************************/ /*!
 * @brief Number of milliseconds in one hour.
 ******************************************************************************/
#define FNET_TIMER_MS_IN_HOUR       (FNET_TIMER_MS_IN_MIN*60U)

#if defined(__cplusplus)
extern "C" {
#endif

/**************************************************************************/ /*!
 * @brief Unsigned integer type representing time uinits.
 * It can be seconds or milliseconds.
 ******************************************************************************/
typedef fnet_uint32_t fnet_time_t;

/***************************************************************************/ /*!
 *
 * @brief    Get the timer counter value in milliseconds.
 *
 * @return   This function returns a current value of the timer counter
 *           in milliseconds.
 *
 ******************************************************************************
 *
 * This function returns a current value of the timer counter in milliseconds,
 * from the moment of the hardware timer initialization
 * (it's done in the FNET stack initialization).@n
 *
 ******************************************************************************/
fnet_time_t fnet_timer_get_ms( void );

/***************************************************************************/ /*!
 *
 * @brief    Perform a delay for the given number of milliseconds.
 *
 * @param    delay_ms Time value used for delay, in milliseconds.
 *
 ******************************************************************************
 *
 * This function performs a delay for a given number of milliseconds.
 * The function is blocked, till the @c delay_ms expires.
 *
 ******************************************************************************/
void fnet_timer_delay( fnet_time_t delay_ms );

/***************************************************************************/ /*!
 *
 * @brief    Poll SW timers.
 *
 * @see FNET_CFG_TIMER_ALT
 *
 ******************************************************************************
 *
 * This function checks timeouts for TCP, ARP, IP and other stack activities.@n
 * The user application should call this function periodically,
 * after the FNET stack initialization, and only if @ref FNET_CFG_TIMER_ALT is 1.@n
 * Recommended polling period is 100ms or less.@n
 * If @ref FNET_CFG_TIMER_ALT is 0, this function is called automatically
 * by FNET in the timer HW interrupt.
 *
 ******************************************************************************/
void fnet_timer_poll(void);

#if FNET_CFG_TIME || defined(__DOXYGEN__)
#include <time.h>

#define fnet_time   time  /* Compliance with <time.h> */

/***************************************************************************/ /*!
 *
 * @brief    Set time since the Epoch (00:00:00 UTC, January 1, 1970).
 *
 * @param [in] sec     Time since the Epoch (00:00:00 UTC, January 1, 1970), in seconds.
 *
 * @see fnet_time(), FNET_CFG_TIME
 *
 ******************************************************************************
 *
 * This function sets the time since the Epoch (00:00:00 UTC, January 1, 1970),
 * in seconds. @n
 * This function is available only if FNET_CFG_TIME is set.
 *
 ******************************************************************************/
void fnet_time_set(time_t sec);

/***************************************************************************/ /*!
 *
 * @brief    Get time since the Epoch (00:00:00 UTC, January 1, 1970).
 *
 * @param [out] sec     If the pointer is not FNET_NULL, it will get the time since the Epoch (00:00:00 UTC, January 1, 1970), in seconds.
 *
 * @return   This function returns time since the Epoch, in seconds.
 *
 * @see fnet_time_set(), FNET_CFG_TIME
 *
 ******************************************************************************
 *
 * This function returns the time since the Epoch (00:00:00 UTC, January 1, 1970),
 * in seconds. It is defined by standard C library @c time.h, and may be used as time(). @n
 * This function is available only if FNET_CFG_TIME is set.
 *
 ******************************************************************************/
time_t fnet_time(time_t *sec);

#endif /* FNET_CFG_TIME */

/*! @} */

#if !FNET_CFG_TIMER_ALT  /* Use bare-metal timer */

#ifndef  FNET_HW_TIMER_INIT
#define FNET_HW_TIMER_INIT    fnet_cpu_timer_init
#endif
#ifndef  FNET_HW_TIMER_RELEASE
#define FNET_HW_TIMER_RELEASE fnet_cpu_timer_release
#endif

#endif /* !FNET_CFG_TIMER_ALT */

#if defined(__cplusplus)
}
#endif

#endif /* _FNET_TIMER_H */
