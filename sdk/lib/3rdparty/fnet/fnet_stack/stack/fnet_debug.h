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
*  Private. Debug definitions.
*
***************************************************************************/

#ifndef _FNET_DEBUG_H_

#define _FNET_DEBUG_H_

#if defined(__cplusplus)
extern "C" {
#endif

#if FNET_CFG_DEBUG

#ifndef FNET_DEBUG
#define FNET_DEBUG(...) fnet_println(__VA_ARGS__)
#endif

#ifndef FNET_DEBUG_PRINTF
#define FNET_DEBUG_PRINTF(...) fnet_printf(__VA_ARGS__)
#endif

#if FNET_CFG_DEBUG_STACK && FNET_CFG_DEBUG
extern fnet_size_t fnet_dbg_stack_max;
#endif

#else

#define FNET_DEBUG(...) do{}while(0)
#define FNET_DEBUG_PRINTF(...) do{}while(0)

#endif

#if defined(__cplusplus)
}
#endif

#endif
