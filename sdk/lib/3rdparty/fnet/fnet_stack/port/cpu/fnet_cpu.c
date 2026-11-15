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
*  CPU-specific functions.
*
***************************************************************************/
#include "fnet.h"

#if (FNET_CFG_CPU_TIMER_NUMBER<0u)||(FNET_CFG_CPU_TIMER_NUMBER>FNET_CFG_CPU_TIMER_NUMBER_MAX)
    #error "FNET_CFG_CPU_TIMER_NUMBER must be from 0 to FNET_CFG_CPU_TIMER_NUMBER_MAX."
#endif

#if (FNET_CFG_CPU_TIMER_VECTOR_PRIORITY<1u)||(FNET_CFG_CPU_TIMER_VECTOR_PRIORITY>FNET_CFG_CPU_VECTOR_PRIORITY_MAX)
    #error "FNET_CFG_CPU_TIMER_VECTOR_PRIORITY must be from 1 to FNET_CFG_CPU_VECTOR_PRIORITY_MAX."
#endif

#if (FNET_CFG_CPU_ETH0_MTU > 1500u) /* Limit maximum size.*/
    #error "FNET_CFG_CPU_ETH0_MTU must be <= 1500"
#endif

#if (FNET_CFG_CPU_ETH1_MTU > 1500u) /* Limit maximum size.*/
    #error "FNET_CFG_CPU_ETH1_MTU must be <= 1500"
#endif

#if (FNET_CFG_CPU_ETH_VECTOR_PRIORITY<1u)||(FNET_CFG_CPU_ETH_VECTOR_PRIORITY>FNET_CFG_CPU_VECTOR_PRIORITY_MAX)
    #error "FNET_CFG_CPU_ETH_VECTOR_PRIORITY must be from 1 to FNET_CFG_CPU_VECTOR_PRIORITY_MAX."
#endif

#if (FNET_CFG_CPU_ETH_TX_BUFS_MAX < 2u)
    #error "FNET_CFG_CPU_ETH_TX_BUFS_MAX is less than 2, minimal required value is 2 - see errata MCF5235"
#endif

#if (FNET_CFG_CPU_ETH_RX_BUFS_MAX < 2u)
    #error "FNET_CFG_CPU_ETH_RX_BUFS_MAX is less than 2, minimal required value is 2 - see errata MCF5235"
#endif

#if FNET_CFG_CPU_LITTLE_ENDIAN
/* Convert short from host- to network byte order.*/
fnet_uint16_t fnet_htons(fnet_uint16_t short_var)
{
    fnet_uint16_t result = FNET_HTONS(short_var);
    return result;
}
/* Convert long from host- to network byte order.*/
fnet_uint32_t fnet_htonl(fnet_uint32_t long_var)
{
    fnet_uint32_t result = FNET_HTONL(long_var);
    return result;
}
#endif /* FNET_CFG_CPU_LITTLE_ENDIAN */
