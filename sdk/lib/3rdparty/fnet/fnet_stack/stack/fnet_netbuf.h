/**************************************************************************
*
* Copyright 2018 by Andrey Butok. FNET Community.
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
* FNET Buffer and memory management definitions.
*
***************************************************************************/

#ifndef _FNET_NETBUF_H_

#define _FNET_NETBUF_H_

#if defined(__cplusplus)
extern "C" {
#endif

fnet_size_t fnet_free_mem_status( void );
fnet_size_t fnet_malloc_max( void );

#if defined(__cplusplus)
}
#endif

#endif /* _FNET_NETBUF_H_ */
