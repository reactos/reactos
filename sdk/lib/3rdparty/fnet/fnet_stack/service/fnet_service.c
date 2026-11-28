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
*  FNET Service API.
*
***************************************************************************/

#include "fnet.h"
#include "stack/fnet_stack_prv.h"

/************************************************************************
*     Definitions
*************************************************************************/

/* Polling list element type definition */

typedef struct
{
    fnet_service_poll_t service;
    fnet_bool_t         is_running;         /* Run flag, to avoid possible recursion. */
    void                *service_cookie;
} fnet_poll_list_entry_t;

/* Polling interface structure */
static struct
{
    fnet_poll_list_entry_t list[FNET_CFG_SERVICE_MAX]; /* Polling list.*/
} fnet_poll_if;


#if FNET_CFG_MULTITHREADING

    static fnet_mutex_t fnet_service_mutex = FNET_NULL;

    fnet_return_t _fnet_service_mutex_init(void);
    void _fnet_service_mutex_release(void);
#else
    #define _fnet_service_mutex_init()        FNET_OK
    #define _fnet_service_mutex_release()        do{}while(0)
#endif

/************************************************************************
* DESCRIPTION: This function calls all registered service routines in
*              the polling list.
*************************************************************************/
void fnet_service_poll( void )
{
    fnet_index_t i;

    fnet_service_mutex_lock();
    for (i = 0u; i < FNET_CFG_SERVICE_MAX; i++)
    {
        if(fnet_poll_if.list[i].service && (fnet_poll_if.list[i].is_running == FNET_FALSE))
        {
            fnet_poll_if.list[i].is_running = FNET_TRUE;
            fnet_poll_if.list[i].service(fnet_poll_if.list[i].service_cookie);
            fnet_poll_if.list[i].is_running = FNET_FALSE;
        }
    }
    fnet_service_mutex_unlock();
}

/************************************************************************
* DESCRIPTION: This function adds service routine into the polling list.
*************************************************************************/
fnet_service_desc_t fnet_service_register( fnet_service_poll_t service, void *service_cookie )
{
    fnet_index_t            i = 0u;
    fnet_poll_list_entry_t  *poll_entry = 0;

    if(service)
    {
        _fnet_stack_mutex_lock();

        for(i = 0; i < FNET_CFG_SERVICE_MAX; i++)
        {
            if(fnet_poll_if.list[i].service == FNET_NULL)
            {
                poll_entry = &fnet_poll_if.list[i];
                poll_entry->service_cookie = service_cookie;
                poll_entry->service = service;
                break;
            }
        }

        _fnet_stack_mutex_unlock();
    }

    return (fnet_service_desc_t)poll_entry;
}

/************************************************************************
* DESCRIPTION: This function removes service routine from the polling list.
*************************************************************************/
void fnet_service_unregister( fnet_service_desc_t desc )
{
    fnet_poll_list_entry_t  *poll_entry = (fnet_poll_list_entry_t *)desc;

    if(poll_entry)
    {
        _fnet_stack_mutex_lock();
        poll_entry->service = 0;
        _fnet_stack_mutex_unlock();
    }
}

/************************************************************************
* DESCRIPTION: Initialize service module
*************************************************************************/
fnet_return_t _fnet_service_init(void)
{
    fnet_return_t result;

    fnet_memset_zero(&fnet_poll_if, sizeof(fnet_poll_if));

    result = _fnet_service_mutex_init();

    return result;
}

/************************************************************************
* DESCRIPTION: Release service module
*************************************************************************/
void _fnet_service_release(void)
{
    _fnet_service_mutex_release();
}

#if FNET_CFG_MULTITHREADING

/* ================ Core Stack Mutex ==========================*/
fnet_return_t _fnet_service_mutex_init(void)
{
    return _fnet_mutex_init(&fnet_service_mutex);
}

void fnet_service_mutex_lock(void)
{
    _fnet_mutex_lock(&fnet_service_mutex);
}

void fnet_service_mutex_unlock(void)
{
    _fnet_mutex_unlock(&fnet_service_mutex);
}

void _fnet_service_mutex_release(void)
{
    _fnet_mutex_release(&fnet_service_mutex);
}

#endif /* FNET_CFG_MULTITHREADING */
