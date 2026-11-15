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
*  Main include header for FNET projects.
*
***************************************************************************/

#ifndef _FNET_H_

#define _FNET_H_

#include "fnet_config.h"
#include "port/compiler/fnet_comp.h"
#include "port/cpu/fnet_cpu.h"
#include "stack/fnet_stack.h"
#include "service/fnet_service.h"

/*! @addtogroup fnet_define
* These definitions are used for reference purpose only.
* @n
*/
/*! @{*/

/**************************************************************************/ /*!
 * @def FNET_DESCRIPTION
 * @brief Description string of the FNET TCP/IP stack.
 * @showinitializer
 ******************************************************************************/
#define FNET_DESCRIPTION        "FNET TCP/IP Stack"

/**************************************************************************/ /*!
 * @def FNET_LICENSE
 * @brief License string of the FNET TCP/IP stack.
 * @showinitializer
 ******************************************************************************/
#define FNET_LICENSE            "APACHEv2"

/**************************************************************************/ /*!
 * @def FNET_COPYRIGHT
 * @brief Copyright string of the FNET TCP/IP stack.
 * @showinitializer
 ******************************************************************************/
#define FNET_COPYRIGHT          "Copyright by FNET Community"

/**************************************************************************/ /*!
 * @def FNET_BUILD_DATE
 * @brief Build date and time of the project as a string.
 * @showinitializer
 ******************************************************************************/
#define FNET_BUILD_DATE         __DATE__ " at " __TIME__

/**************************************************************************/ /*!
 * @def FNET_VERSION
 * @brief Current version number of the FNET TCP/IP stack.
 *        The resulting value format is xx.xx.xx = major.minor.revision, as a
 *        string.
 * @showinitializer
 ******************************************************************************/
#define FNET_VERSION            "4.7.1"

/*! @} */

#endif /* _FNET_H_ */
