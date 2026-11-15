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
*  Main including header for the FNET TCP/IP stack.
*
***************************************************************************/

#ifndef _FNET_STACK_PRV_H_

#define _FNET_STACK_PRV_H_

#include "fnet.h"

extern fnet_bool_t _fnet_is_enabled;

#if defined(__cplusplus)
extern "C" {
#endif

#if FNET_CFG_MULTITHREADING

/* General mutex API */
fnet_return_t _fnet_mutex_init(fnet_mutex_t *mutex);
void _fnet_mutex_lock(fnet_mutex_t *mutex);
void _fnet_mutex_unlock(fnet_mutex_t *mutex);
void _fnet_mutex_release(fnet_mutex_t *mutex);

/* Stack mutex */
fnet_return_t _fnet_stack_mutex_init(void);
void _fnet_stack_mutex_release(void);
void _fnet_stack_mutex_lock(void);
void _fnet_stack_mutex_unlock(void);

#else

#define _fnet_stack_mutex_init()        FNET_OK
#define _fnet_stack_mutex_release()     do{}while(0)
#define _fnet_stack_mutex_lock()        do{}while(0)
#define _fnet_stack_mutex_unlock()      do{}while(0)

#endif /* FNET_CFG_MULTITHREADING */

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _FNET_STACK_H_ */

