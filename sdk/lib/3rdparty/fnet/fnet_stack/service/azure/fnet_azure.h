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
*  Azure-IOT adapter API.
*
***************************************************************************/

#ifndef _FNET_AZURE_H_

#define _FNET_AZURE_H_

#if FNET_CFG_AZURE || defined(__DOXYGEN__)

    /*! @addtogroup fnet_azure
    *
    * FNET provides adapters for Azure IoT Hub Device C SDK.@n
    * It is enabled by @ref FNET_CFG_AZURE. Also, Azure requires enabling of @ref FNET_CFG_TLS and @ref FNET_CFG_TIME. @n
    * To communicate with Azure IoT Hub a user application has to use the SDK API (https://docs.microsoft.com/en-gb/azure/iot-hub/iot-hub-device-sdk-c-intro). @n
    * FNET provides the Azure demo application @ref quick_start_azure.
    *
    * Configuration parameters:
    * - @ref FNET_CFG_AZURE
    * - @ref FNET_CFG_TLS
    * - @ref FNET_CFG_TIME
    *
    */


#endif /* FNET_CFG_AZURE */

#endif /* _FNET_AZURE_H_ */
