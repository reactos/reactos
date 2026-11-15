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
*  Azure-IOT adapter configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_AZURE_CONFIG_H_

#define _FNET_AZURE_CONFIG_H_

/*! @addtogroup fnet_azure_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_AZURE
 * @brief    Azure-IOT support:
 *               - @c 1 = is enabled. FNET provides Azure IoT Hub client adapter.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_AZURE
    #define FNET_CFG_AZURE                            (0)
#endif

/*! @} */

#endif /* _FNET_AZURE_CONFIG_H_ */
