/**************************************************************************
*
* Copyright 2008-2018 by Andrey Butok. FNET Community.
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
*  Private. FNET Timers API.
*
***************************************************************************/

#ifndef _FNET_TIMER_PRV_H_

#define _FNET_TIMER_PRV_H_

#include "fnet_timer.h"

/* SW Timer descriptor.*/
typedef void *fnet_timer_desc_t;

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

fnet_return_t _fnet_timer_init( fnet_time_t period_ms );
void fnet_cpu_timer_release( void );
void _fnet_timer_release( void );
fnet_timer_desc_t _fnet_timer_new( fnet_time_t period_ms, void (*handler)( fnet_uint32_t cookie ), fnet_uint32_t cookie );
void _fnet_timer_free( fnet_timer_desc_t timer );
void _fnet_timer_poll(void);
#if !FNET_CFG_TIMER_ALT /* Use bare-metal timer */
void _fnet_timer_ticks_inc( void );
void _fnet_timer_handler_bottom(void *cookie);
fnet_return_t fnet_cpu_timer_init( fnet_time_t period_ms );
#else /* Use application alternative timer */
#define _fnet_timer_handler_bottom FNET_NULL
extern const fnet_timer_api_t  *_fnet_timer_api;
#endif

#if defined(__cplusplus)
}
#endif

#endif
