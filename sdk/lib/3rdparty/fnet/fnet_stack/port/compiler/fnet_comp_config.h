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
* Compiler-specific default configuration file.
*
***************************************************************************/

/************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 ************************************************************************/

#ifndef _FNET_COMP_CONFIG_H_

#define _FNET_COMP_CONFIG_H_

/*! @addtogroup fnet_platform_config  */
/*! @{ */
/**************************************************************************/ /*!
 * @def      FNET_CFG_COMP_compiler_type
 * @brief    This is the set of the @c FNET_CFG_COMP_[compiler_type] definitions that
 *           define a currently used compiler. @n
 *           Current version of the FNET supports the following compiler definitions:
 *            - @c FNET_CFG_COMP_IAR = Used compiler is IAR.
 *            - @c FNET_CFG_COMP_GNUC = Used compiler is GCC (KDS).
 *            - @c FNET_CFG_COMP_UV  = Used compiler is Keil uVision.
 *            - @c FNET_CFG_COMP_GHS = Used compiler is Green Hills.
 *            - @c FNET_CFG_COMP_DCC = Used compiler is Diab.
 *            - @c FNET_CFG_COMP_CW  = Used compiler is CodeWarrior.
 *            @n @n
 *            Selected compiler definition should be only one and must be defined as 1.
 *            All others may be defined but must have the 0 value.
 *            @note If no compiler definition is defined, the used compiler is autodetected.
 *
 ******************************************************************************/
#define FNET_CFG_COMP_compiler_type /* Ignore it. Just only for Doxygen documentation */

/* CW compiler. */
#if (defined(FNET_CFG_COMP_CW) && FNET_CFG_COMP_CW)
    #define FNET_COMP_STR       "CW"
    /* IAR compiler. */
#elif (defined(FNET_CFG_COMP_IAR) && FNET_CFG_COMP_IAR)
    #define FNET_COMP_STR       "IAR"
    /* GHS compiler. */
#elif (defined(FNET_CFG_COMP_GHS) && FNET_CFG_COMP_GHS)
    #define FNET_COMP_STR       "GHS"
    /* Keil uVision compiler. */
#elif (defined(FNET_CFG_COMP_UV) && FNET_CFG_COMP_UV)
    #define FNET_COMP_STR       "UV"
    /* GNU GCC */
#elif (defined(FNET_CFG_COMP_GNUC) && FNET_CFG_COMP_GNUC)
    #define FNET_COMP_STR       "GCC"
    /* Diab DCC */
#elif (defined(FNET_CFG_COMP_DCC) && FNET_CFG_COMP_DCC)
    #define FNET_COMP_STR       "DCC"
#endif

#if !defined(FNET_COMP_STR)
    /* CW compiler. */
    #if (defined(__MWERKS__) || defined(__CODEWARRIOR__))
        #define FNET_CFG_COMP_CW    (1)
        #define FNET_COMP_STR       "CW"
        /* IAR compiler. */
    #elif defined(__IAR_SYSTEMS_ICC__) || defined(__IAR_SYSTEMS_ASM__) || defined(__ICCARM__)
        #define FNET_CFG_COMP_IAR   (1)
        #define FNET_COMP_STR       "IAR"
        /* GHS compiler. */
    #elif defined(__ghs__)
        #define FNET_CFG_COMP_GHS   (1)
        #define FNET_COMP_STR       "GHS"
        /* ARM, Keil uVision compiler. */
    #elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
        #define FNET_CFG_COMP_UV    (1)
        #define FNET_COMP_STR       "UV"
        /* GNU GCC */
    #elif defined(__GNUC__)
        #define FNET_CFG_COMP_GNUC  (1)
        #define FNET_COMP_STR       "GCC"
        /* Diab DCC */
    #elif defined(__DCC__)
        #define FNET_CFG_COMP_DCC   (1)
        #define FNET_COMP_STR       "DCC"
    #else
        #error "It is not possible to detect the compiler. Please set a FNET_CFG_COMP_XXXX parameter."
    #endif
#endif

/* Set default value */
#ifndef FNET_CFG_COMP_IAR
    #define FNET_CFG_COMP_IAR   (0)
#endif
#ifndef FNET_CFG_COMP_CW
    #define FNET_CFG_COMP_CW    (0)
#endif
#ifndef FNET_CFG_COMP_UV
    #define FNET_CFG_COMP_UV    (0)
#endif
#ifndef FNET_CFG_COMP_GNUC
    #define FNET_CFG_COMP_GNUC  (0)
#endif
#ifndef FNET_CFG_COMP_GHS
    #define FNET_CFG_COMP_GHS   (0)
#endif
#ifndef FNET_CFG_COMP_DCC
    #define FNET_CFG_COMP_DCC   (0)
#endif

/*! @} */

#endif /* _FNET_COMP_CONFIG_H_ */
