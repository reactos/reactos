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
*  FNET Shell Library configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_SHELL_CONFIG_H_

#define _FNET_SHELL_CONFIG_H_

/*! @addtogroup fnet_shell_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def     FNET_CFG_SHELL_MAX
 * @brief   Maximum number of the Shell services that can be run simultaneously.@n
 *          Default value is @b @c 1.
 ******************************************************************************/
#ifndef FNET_CFG_SHELL_MAX
    #define FNET_CFG_SHELL_MAX          (1u+(FNET_CFG_TELNET*FNET_CFG_TELNET_SESSION_MAX))
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_SHELL_ARGS_MAX
 * @brief Maximum number of arguments that can be handled by the
 *        shell command parser. @n
 *        Arguments must be split by the space symbol. Note that the
 *        quote @ref FNET_SHELL_QUOTE_SYMBOL is used to combine multiple
 *        words to one argument. @n
 *        Default value is @b @c 16.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SHELL_ARGS_MAX
    #define FNET_CFG_SHELL_ARGS_MAX         (16u)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_SHELL_HELP_FORMAT
 * @brief Format of the command-shell @c help message,
 *        that is used by the @ref fnet_shell_help() function.@n
 *        For example it can set to the @c ">%7s %-32s- %s" value , where the first string is
 *        the command name, second one is the brief description of the command
 *        and the third one is the syntax of the command.
 * @see fnet_shell_command_t, fnet_shell_help()
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SHELL_HELP_FORMAT
    #define FNET_CFG_SHELL_HELP_FORMAT      (">%10s %-35s - %s")
#endif

/*! @} */

#endif
