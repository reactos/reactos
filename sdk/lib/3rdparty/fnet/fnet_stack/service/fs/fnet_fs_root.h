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
*  Private. Root FS API.
*
***************************************************************************/

#ifndef _FNET_FS_ROOT_H_

#define _FNET_FS_ROOT_H_

#include "fnet.h"

#if FNET_CFG_FS

/* Root FS name string */
#define FNET_FS_ROOT_NAME   "root"

#if defined(__cplusplus)
extern "C" {
#endif

void _fnet_fs_root_register( void );
void _fnet_fs_root_unregister( void );

#if defined(__cplusplus)
}
#endif


#endif /* FNET_CFG_FS */

#endif /* _FNET_FS_ROOT_H_ */
