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
*  FNET Serial lib. configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_SERIAL_CONFIG_H_

#define _FNET_SERIAL_CONFIG_H_

/*! @addtogroup fnet_serial_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def     FNET_CFG_SERIAL_PRINTF_N_TO_RN
 * @brief   Automatic replacing of all line feeds ("\n") inside the format
 *          string to CR LF ("\r\n") by fnet_(s)printf() functions:
 *               - @b @c 1 = is enabled (Deafault value, to save some ROM).
 *               - @c 0 = is disabled.@n
 *           @n
 * To save some bytes from all the hard coded strings the fnet_(s)printf() functions will
 * expand all line feeds ("\n") inside the format string to CR LF ("\r\n").
 * So do not use "\r\n" in	the format string - it will be expanded to
 * "\r\r\n". It is save to add it via a parameter though, e.g.
 * fnet_printf("%s", "\r\n");
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SERIAL_PRINTF_N_TO_RN
    #define FNET_CFG_SERIAL_PRINTF_N_TO_RN     (1)
#endif

/*! @} */

#endif
