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
*  FNET init/release functions.
*
***************************************************************************/

#include "fnet.h"
#include "fnet_socket_prv.h"
#include "fnet_prot.h"
#include "fnet_stack_prv.h"
#include "fnet_loop.h"
#include "service/fnet_service_prv.h"


/************************************************************************
*     Global Data Structures
*************************************************************************/
fnet_bool_t _fnet_is_enabled = FNET_FALSE;   /* Flag that the stack is initialized. */
#if FNET_CFG_MULTITHREADING
    static const fnet_mutex_api_t  *_fnet_mutex_api = FNET_NULL;
    static fnet_mutex_t fnet_stack_mutex = FNET_NULL;
#endif

/************************************************************************
*     Function Prototypes
*************************************************************************/
static fnet_return_t _fnet_stack_init( void );
static void _fnet_stack_release( void );

/************************************************************************
* DESCRIPTION:
*************************************************************************/
fnet_return_t fnet_init( struct fnet_init_params *init_params )
{
    fnet_return_t result = FNET_ERR;

    if(init_params && init_params->netheap_size)
    {
#if FNET_CFG_MULTITHREADING
        _fnet_mutex_api = init_params->mutex_api;
#endif
#if FNET_CFG_TIMER_ALT
        _fnet_timer_api = init_params->timer_api;
#endif
        if(_fnet_stack_mutex_init() == FNET_OK)
        {
            _fnet_stack_mutex_lock();

            if(_fnet_is_enabled == FNET_FALSE) /* Is enabled already?. */
            {
                if((result = _fnet_heap_init(init_params->netheap_ptr, init_params->netheap_size)) == FNET_OK )
                {
                    if((result = _fnet_service_init()) == FNET_OK )
                    {
                        if((result = _fnet_stack_init()) == FNET_OK)
                        {
                            _fnet_is_enabled = FNET_TRUE; /* Mark the stack is enabled. */
                        }
                    }
                }
            }
            _fnet_stack_mutex_unlock();
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
void fnet_release(void)
{
    _fnet_stack_mutex_lock();

    if(_fnet_is_enabled)
    {
        _fnet_stack_release();
        _fnet_is_enabled = FNET_FALSE;
    }

    _fnet_service_release();

    _fnet_stack_mutex_unlock();

    _fnet_stack_mutex_release();

#if FNET_CFG_MULTITHREADING
    _fnet_mutex_api = FNET_NULL;
#endif
}

/************************************************************************
* DESCRIPTION: TCP/IP Stack initialization.
************************************************************************/
static fnet_return_t _fnet_stack_init( void )
{
    fnet_isr_init();

    fnet_netif_list = 0;     /* Reset netif list. */
    fnet_netif_default = 0;  /* Reset default interface.*/

    /* Initialize Timer Module */
    if (_fnet_timer_init(FNET_TIMER_PERIOD_MS) == FNET_ERR)
    {
        goto ERROR;
    }

#if FNET_CFG_DEBUG_STARTUP_MS && FNET_CFG_DEBUG
    /* Add start-up */
    fnet_println("\n Waiting %d Seconds...", FNET_CFG_DEBUG_STARTUP_MS / FNET_TIMER_MS_IN_SEC);
    fnet_timer_delay(FNET_CFG_DEBUG_STARTUP_MS);
#endif

    /* Initialize protocol layer */
    if(_fnet_prot_init() == FNET_ERR)
    {
        goto ERROR;
    }

    /* Initialize socket layer.*/
    if(_fnet_socket_init() == FNET_ERR)
    {
        goto ERROR;
    }

#if FNET_CFG_LOOPBACK
    /* Initialize Loop-back interface.*/
    if(_fnet_netif_init(&fnet_loop_if, FNET_NULL, 0u) == FNET_ERR)
    {
        goto ERROR;
    }
#endif

    return (FNET_OK);
ERROR:
    _fnet_stack_release();

    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION: TCP/IP Stack release.
************************************************************************/
static void _fnet_stack_release( void )
{
    _fnet_netif_release_all();
    _fnet_prot_release();
    _fnet_timer_release();
    _fnet_mem_release();
}

/************************************************************************
* DESCRIPTION: General FNET poll.
************************************************************************/
void fnet_poll( void )
{
#if FNET_CFG_TIMER_ALT
    fnet_timer_poll();      /* Poll FNET stack timer.*/
#endif
    fnet_service_poll();    /* Poll registered services.*/
}

#if FNET_CFG_MULTITHREADING

/* =============== General mutex API ==============================*/
fnet_return_t _fnet_mutex_init(fnet_mutex_t *mutex)
{
    fnet_return_t result;
    if(_fnet_mutex_api) /* Check if multithreading is enabled.*/
    {
        if(_fnet_mutex_api->mutex_init && mutex)
        {
            result = _fnet_mutex_api->mutex_init(mutex);
        }
        else
        {
            result = FNET_ERR;
        }
    }
    else
    {
        result = FNET_OK; /* OK - it is just disabled.*/
    }

    return result;
}

void _fnet_mutex_lock(fnet_mutex_t *mutex)
{
    if(_fnet_mutex_api) /* Check if multithreading is enabled.*/
    {
        if(_fnet_mutex_api->mutex_lock && mutex && *mutex )
        {
            _fnet_mutex_api->mutex_lock(mutex);
        }
    }
}

void _fnet_mutex_unlock(fnet_mutex_t *mutex)
{
    if(_fnet_mutex_api) /* Check if multithreading is enabled.*/
    {
        if(_fnet_mutex_api->mutex_unlock && mutex && *mutex )
        {
            _fnet_mutex_api->mutex_unlock(mutex);
        }
    }
}

void _fnet_mutex_release(fnet_mutex_t *mutex)
{
    if(_fnet_mutex_api) /* Check if multithreading is enabled.*/
    {
        if(_fnet_mutex_api->mutex_release && mutex && *mutex )
        {
            _fnet_mutex_api->mutex_release(mutex);
            *mutex = FNET_NULL;
        }
    }
}

/* ================ Core Stack Mutex ==========================*/
fnet_return_t _fnet_stack_mutex_init(void)
{
    return _fnet_mutex_init(&fnet_stack_mutex);
}

void _fnet_stack_mutex_lock(void)
{
    _fnet_mutex_lock(&fnet_stack_mutex);
}

void _fnet_stack_mutex_unlock(void)
{
    _fnet_mutex_unlock(&fnet_stack_mutex);
}

void _fnet_stack_mutex_release(void)
{
    _fnet_mutex_release(&fnet_stack_mutex);
}

#endif /* FNET_CFG_MULTITHREADING */
