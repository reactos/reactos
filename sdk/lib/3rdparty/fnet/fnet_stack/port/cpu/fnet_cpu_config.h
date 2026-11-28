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
*  Default platform-specific configuration.
*
***************************************************************************/

/************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 ************************************************************************/

#ifndef _FNET_CPU_CONFIG_H_

#define _FNET_CPU_CONFIG_H_

/*! @addtogroup fnet_platform_config  */
/*! @{ */
/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_processor_type
 * @brief    This is the set of the @c FNET_CFG_CPU_[processor_type] definitions that
 *           define a currently used processor. @n
 *           Current version of the FNET supports the following processor definitions:
 *            - @c FNET_CFG_CPU_MK60DN512  = Used platform is MK60DN512.
 *            - @c FNET_CFG_CPU_MK60N512  = Used platform is MK60N512.
 *            - @c FNET_CFG_CPU_MK66FN2  = Used platform is MK66FN2.
 *            - @c FNET_CFG_CPU_MK64FN1  = Used platform is MK64FN1.
 *            - @c FNET_CFG_CPU_MK70FN1  = Used platform is MK70FN1.
 *            - @c FNET_CFG_CPU_MK60FN1  = Used platform is MK60FN1.
 *            - @c FNET_CFG_CPU_MPC5668G  = Used platform is MPC5668G.
 *            - @c FNET_CFG_CPU_MPC5566  = Used platform is MPC5566.
 *            - @c FNET_CFG_CPU_MPC564xBC  = Used platform is MPC564xBC.
 *            - @c FNET_CFG_CPU_MPC5744P  = Used platform is MPC5744P.
 *            - @c FNET_CFG_CPU_MCF52235  = Used platform is MCF52235.
 *            - @c FNET_CFG_CPU_MCF52259 = Used platform is MCF52259.
 *            - @c FNET_CFG_CPU_MCF5282  = Used platform is MCF5282.
 *            - @c FNET_CFG_CPU_MCF51CN128  = Used platform is MCF51CN128.
 *            - @c FNET_CFG_CPU_MCF54418  = Used platform is MCF54418.
 *            - @c FNET_CFG_CPU_S32R274 = Used platform is S32R274.
 *            - @c FNET_CFG_CPU_LPC54628 = Used platform is LPC54628.
 *            - @c FNET_CFG_CPU_MIMXRT1052 = Used platform is MIMXRT1052.
 *            - @c FNET_CFG_CPU_MIMXRT1062 = Used platform is MIMXRT1062.
 *            @n @n
 *            Selected processor definition should be only one and must be defined as 1.
 *            All others may be defined but must have the 0 value.
 *
 ******************************************************************************/

#define FNET_CFG_CPU_processor_type /* Ignore it. Just only for Doxygen documentation */

/*-----------*/
#ifndef FNET_CFG_CPU_MCF52235
    #define FNET_CFG_CPU_MCF52235   (0)
#endif
#ifndef FNET_CFG_CPU_MCF52259
    #define FNET_CFG_CPU_MCF52259   (0)
#endif
/* MCF5235 NOT SUPPORTED/TESTED */
#ifndef FNET_CFG_CPU_MCF5235
    #define FNET_CFG_CPU_MCF5235    (0)
#endif
#ifndef FNET_CFG_CPU_MCF5282
    #define FNET_CFG_CPU_MCF5282    (0)
#endif
#ifndef FNET_CFG_CPU_MCF51CN128
    #define FNET_CFG_CPU_MCF51CN128 (0)
#endif
#ifndef FNET_CFG_CPU_MCF54418
    #define FNET_CFG_CPU_MCF54418   (0)
#endif
#ifndef FNET_CFG_CPU_MK60N512
    #define FNET_CFG_CPU_MK60N512   (0)
#endif
#ifndef FNET_CFG_CPU_MK60DN512
    #define FNET_CFG_CPU_MK60DN512  (0)
#endif
#ifndef FNET_CFG_CPU_MK64FN1
    #define FNET_CFG_CPU_MK64FN1    (0)
#endif
#ifndef FNET_CFG_CPU_MK66FN2
    #define FNET_CFG_CPU_MK66FN2    (0)
#endif
#ifndef FNET_CFG_CPU_MK70FN1
    #define FNET_CFG_CPU_MK70FN1   	(0)
#endif
#ifndef FNET_CFG_CPU_MK60FN1
    #define FNET_CFG_CPU_MK60FN1    (0)
#endif
#ifndef FNET_CFG_CPU_MPC5668G
    #define FNET_CFG_CPU_MPC5668G   (0)
#endif
#ifndef FNET_CFG_CPU_MPC5566
    #define FNET_CFG_CPU_MPC5566    (0)
#endif
#ifndef FNET_CFG_CPU_MPC564xBC
    #define FNET_CFG_CPU_MPC564xBC  (0)
#endif
#ifndef FNET_CFG_CPU_MPC5744P
    #define FNET_CFG_CPU_MPC5744P   (0)
#endif
#ifndef FNET_CFG_CPU_S32R274
    #define FNET_CFG_CPU_S32R274    (0)
#endif
#ifndef FNET_CFG_CPU_LPC54628
    #define FNET_CFG_CPU_LPC54628  	(0)
#endif
#ifndef FNET_CFG_CPU_MIMXRT1052
    #define FNET_CFG_CPU_MIMXRT1052 (0)
#endif
#ifndef FNET_CFG_CPU_MIMXRT1062
    #define FNET_CFG_CPU_MIMXRT1062 (0)
#endif

/*********** MFC ********************/
#if FNET_CFG_CPU_MCF52235 /* Kirin2 */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mcf/fnet_mcf52235_config.h"
    #define FNET_CPU_STR    "MCF52235"
#endif

#if FNET_CFG_CPU_MCF52259 /* Kirin3 */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mcf/fnet_mcf52259_config.h"
    #define FNET_CPU_STR    "MCF52259"
#endif

#if FNET_CFG_CPU_MCF5235 /* CPUV2 */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mcf/fnet_mcf5235_config.h"
    #define FNET_CPU_STR    "MCF5235"
#endif

#if FNET_CFG_CPU_MCF5282 /* Reindeer */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mcf/fnet_mcf5282_config.h"
    #define FNET_CPU_STR    "MCF5282"
#endif

#if FNET_CFG_CPU_MCF51CN128 /* Lasko */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mcf/fnet_mcf51cn128_config.h"
    #define FNET_CPU_STR    "MCF51CN128"
#endif

#if FNET_CFG_CPU_MCF54418 /* Modelo */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mcf/fnet_mcf54418_config.h"
    #define FNET_CPU_STR    "MCF54418"
#endif

/*********** MK ********************/

#if FNET_CFG_CPU_MK60N512 /* Kinetis */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mk/fnet_mk60n512_config.h"
    #define FNET_CPU_STR    "MK60N512"
#endif

#if FNET_CFG_CPU_MK60DN512 /* Kinetis */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mk/fnet_mk60dn512_config.h"
    #define FNET_CPU_STR    "MK60DN512"
#endif

#if FNET_CFG_CPU_MK64FN1 /* Kinetis */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mk/fnet_mk64fn1_config.h"
    #define FNET_CPU_STR    "MK64FN1"
#endif

#if FNET_CFG_CPU_MK66FN2 /* Kinetis */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mk/fnet_mk66fn2_config.h"
    #define FNET_CPU_STR    "MK66FN2"
#endif

#if FNET_CFG_CPU_MK70FN1 /* Kinetis */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mk/fnet_mk70fn1_config.h"
    #define FNET_CPU_STR    "MK70FN1"
#endif

#if FNET_CFG_CPU_MK60FN1 /* Kinetis*/
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mk/fnet_mk60fn1_config.h"
    #define FNET_CPU_STR    "MK60FN1"
#endif

/*********** MPC ********************/

#if FNET_CFG_CPU_MPC5668G /* Fado */
    #ifdef FNET_CPU_STR
        #error More than one CPU selected FNET_CFG_CPU_XXXX
    #endif

    #include "port/cpu/mpc/fnet_mpc5668g_config.h"
    #define FNET_CPU_STR    "MPC5668G"
#endif

#if FNET_CFG_CPU_MPC5566  /* Viper */
    #ifdef FNET_CPU_STR
        #error More than one CPU selected FNET_CFG_CPU_XXXX
    #endif

    #include "port/cpu/mpc/fnet_mpc5566_config.h"
    #define FNET_CPU_STR    "MPC5566"
#endif

/* TBD NOT SUPPORTED/TESTED YET */

#if FNET_CFG_CPU_MPC564xBC /* Bolero3M */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mpc/fnet_mpc564xbc_config.h"
    #define FNET_CPU_STR    "MPC56xBC"
#endif

#if FNET_CFG_CPU_MPC5744P /* Panther */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mpc/fnet_mpc5744p_config.h"
    #define FNET_CPU_STR    "MPC5744P"
#endif

/*********** S32R274 ********************/

#if FNET_CFG_CPU_S32R274 /* Racerunner Ultra */
    #ifdef FNET_CPU_STR
        #error More than one CPU selected FNET_CPU_XXXX
    #endif

    #include "port/cpu/mpc/fnet_s32r274_config.h"
    #define FNET_CPU_STR    "S32R274"
#endif

/*********** NXP's LPC ********************/
#if FNET_CFG_CPU_LPC54628 /* Aruba */
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/lpc/fnet_lpc54628_config.h"
    #define FNET_CPU_STR    "LPC54628"
#endif

/*********** iMX RT ********************/
#if FNET_CFG_CPU_MIMXRT1052
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mimxrt/fnet_mimxrt1052_config.h"
    #define FNET_CPU_STR    "MIMXRT1052"
#endif

#if FNET_CFG_CPU_MIMXRT1062
    #ifdef FNET_CPU_STR
        #error "More than one CPU selected FNET_CFG_CPU_XXXX"
    #endif

    #include "port/cpu/mimxrt/fnet_mimxrt1062_config.h"
    #define FNET_CPU_STR    "MIMXRT1062"
#endif


/*-----------*/
#ifndef FNET_CPU_STR
    #error "Select/Define proper CPU FNET_CFG_CPU_XXXX !"
#endif

/*-----------*/
#ifndef FNET_MCF
    #define FNET_MCF    (0)
#endif

#ifndef FNET_MK
    #define FNET_MK     (0)
#endif

#ifndef FNET_MPC
    #define FNET_MPC    (0)
#endif

#ifndef FNET_LPC
    #define FNET_LPC    (0)
#endif

#ifndef FNET_MIMXRT
    #define FNET_MIMXRT (0)
#endif

/*-----------*/
#if FNET_MCF
    #include "port/cpu/mcf/fnet_mcf_config.h"
#endif

#if FNET_MK
    #include "port/cpu/mk/fnet_mk_config.h"
#endif

#if FNET_MPC
    #include "port/cpu/mpc/fnet_mpc_config.h"
#endif

#if FNET_LPC
    #include "port/cpu/lpc/fnet_lpc_config.h"
#endif

#if FNET_MIMXRT
    #include "port/cpu/mimxrt/fnet_mimxrt_config.h"
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_LITTLE_ENDIAN
 * @brief    Byte order is:
 *               - @c 1 = little endian (for ARM).
 *               - @c 0 = big endian (for ColdFire, MPC).
 *           @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_LITTLE_ENDIAN
    #define FNET_CFG_CPU_LITTLE_ENDIAN      (0)
#endif

/*****************************************************************************
 * @def      FNET_CFG_CPU_INDEX
 * @brief    Processor index (0 or 1).It defines which core should be used,
 *           in multiprocessor systems.@n
 *           Default value is @b 0. @n
 *           Used only by multi-core MPC platform and ignored for MCF and MK:
 *           - MPC5668g has 0 (z6 core up to 128MHz) or 1 (z0 core up to 60MHz).
 *           - MPC564xBC has 0 (z4 core up to 120MHz) or 1 (z0 core up to 60MHz).
******************************************************************************/
#ifndef FNET_CFG_CPU_INDEX
    #define FNET_CFG_CPU_INDEX              (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_CLOCK_HZ
 * @brief    System frequency in Hz. @n
 *           This parameter used by FNET for correct initialization of
 *           Ethernet, Serial and Timer modules. @n
 *           The CPU system clock initialization must be done by user-application
 *           startup code.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_CLOCK_HZ
    #error "Please define  FNET_CFG_CPU_CLOCK_HZ"
    #define FNET_CFG_CPU_CLOCK_HZ       (xx) /* Just only for Doxygen documentation */
#endif

#define FNET_CPU_CLOCK_KHZ       (FNET_CFG_CPU_CLOCK_HZ/1000U)
#define FNET_CPU_CLOCK_MHZ       (FNET_CFG_CPU_CLOCK_HZ/1000000U)

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_SERIAL_PORT_DEFAULT
 * @brief    Defines the default serial port number.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_SERIAL_PORT_DEFAULT
    #define FNET_CFG_CPU_SERIAL_PORT_DEFAULT    (0U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_SERIAL_IO_INIT
 * @brief    Serial module Input/Output pin initialization:
 *               - @b @c 1 = is enabled (default value).
 *               - @b @c 0 = is disabled. In this case a user application must do own initialization.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_SERIAL_IO_INIT
    #define FNET_CFG_CPU_SERIAL_IO_INIT   (1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_VECTOR_TABLE
 * @brief    Vector table address.@n
 *           For Kinetis platform, it is used the NVIC vector table register (VTOR).
 *           For other platforms, the symbol should be defined in the linker file.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_VECTOR_TABLE
    #define FNET_CFG_CPU_VECTOR_TABLE           __VECTOR_RAM
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_VECTOR_TABLE_IS_IN_RAM
 * @brief    Vector table in RAM
 *               - @c 1 = is enabled. In this case FNET installs own interrupt service routine into the RAM vector table pointed by @ref FNET_CFG_CPU_VECTOR_TABLE.
 *               - @c 0 = is disabled. In this case assumed FNET interrupt service routine fnet_cpu_isr() is preinstalled into the ROM vector table.
 *                                     @note For Kinetis platform, is used weak symbol overload to preinstall the FNET interrupt service routine.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_VECTOR_TABLE_IS_IN_RAM
    #define FNET_CFG_CPU_VECTOR_TABLE_IS_IN_RAM (1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_TIMER_NUMBER_MAX
 * @brief    Maximum Timer number that is avaiable on the used platform.
 *           For example, if the platform has only one timer, it should be set to zero @n
 *           For  Coldfire and Kinetis it is set to 3. For MPC it is set to 7.
 *           @n @n NOTE: It's ignored for MCF V1.
 *                       User application should not change this parameter.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_TIMER_NUMBER_MAX
    #define FNET_CFG_CPU_TIMER_NUMBER_MAX      (3u)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_VECTOR_PRIORITY_MAX
 * @brief    The maximum the value that can be set to interrupt piority.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_VECTOR_PRIORITY_MAX
    #define FNET_CFG_CPU_VECTOR_PRIORITY_MAX    (7u)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_TIMER_NUMBER
 * @brief    Timer number used by the FNET. It can range from 0 to @ref FNET_CFG_CPU_TIMER_NUMBER_MAX.
 *           By default it set to the @ref FNET_CFG_CPU_TIMER_NUMBER_MAX value.
 *           @n @n NOTE: It's ignored for MCF V1.@n
 ******************************************************************************/
#ifndef FNET_CFG_CPU_TIMER_NUMBER
    #error "FNET_CFG_CPU_TIMER_NUMBER is not defined."
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_TIMER_VECTOR_NUMBER
 * @brief    Vector number of the timer interrupt.
 *           @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_TIMER_VECTOR_NUMBER
    #error "FNET_CFG_CPU_TIMER_VECTOR_NUMBER is not defined."
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_TIMER_VECTOR_PRIORITY
 * @brief    Default Interrupt priority level for a timer used by the FNET.
 *           It can range from 1 to FNET_CFG_CPU_VECTOR_PRIORITY_MAX. The higher the value, the greater
 *           the priority of the corresponding interrupt.
 *           @n @n NOTE: It's ignored for MCF V1.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_TIMER_VECTOR_PRIORITY
    #define FNET_CFG_CPU_TIMER_VECTOR_PRIORITY        (3u)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_CACHE
 * @brief    Data Cache:
 *               - @c 1 = is present and enabled on current platform (for MCF5282/5235, MPC, i.MX-RT).
 *                     @note The MPU platforms have to define @ref FNET_CFG_CPU_NONCACHEABLE_SECTION section - non-cacheable memory region.
 *               - @c 0 = is absent or disabled on current platform.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_CACHE
    #define FNET_CFG_CPU_CACHE              (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_NONCACHEABLE_SECTION
 * @brief    Name of the non-cacheable memory region. The MPU platforms with
 *           enabled Data-cache (@ref FNET_CFG_CPU_CACHE) must define it.
 *           It is used my network driver for data buffers accessed by DMA.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_NONCACHEABLE_SECTION
    #define FNET_CFG_CPU_NONCACHEABLE_SECTION              "NonCacheable"
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_FLASH
 * @brief    On-chip Flash Module:
 *               - @c 1 = Current platform has the On-chip Flash Module
 *                        (CFM for ColdFire, FTFL for Kinetis).
 *               - @c 0 = Current platform does not have the On-chip Flash Module. @n
 *                        MPC flash module is not supported.
 *              @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_FLASH
    #define FNET_CFG_CPU_FLASH              (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_CPU_FLASH_ADDRESS
 * @brief   On-chip Flash memory start address. @n
 *          It is not used by the FNET, but can be useful for an application.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_FLASH_ADDRESS
    #define FNET_CFG_CPU_FLASH_ADDRESS      (0x0U)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_CPU_FLASH_SIZE
 * @brief   On-chip Flash memory size (in bytes). @n
 *          It is not used by the FNET stack, but can be useful for an application.
 *          @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_FLASH_SIZE
    #define FNET_CFG_CPU_FLASH_SIZE         (0U)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_CPU_FLASH_PAGE_SIZE
 * @brief   Erase-page size of the on-chip Flash memory (in bytes). @n
 *          Flash logical blocks are divided into multiple logical pages that can be
 *          erased separately. @n
 *          It is not possible to read from any flash logical block while the same
 *          logical block is being erased, programmed, or verified.
 *          @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_FLASH_PAGE_SIZE
    #define FNET_CFG_CPU_FLASH_PAGE_SIZE    (0U)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_CPU_FLASH_PROGRAM_SIZE
 * @brief   Size of block that can be written to the on-chip Flash memory (in bytes). @n
 *          For MCF and K60 it is set to 4. For K70 it is set to 8.
 *          @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_FLASH_PROGRAM_SIZE
    #define FNET_CFG_CPU_FLASH_PROGRAM_SIZE (0U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_FLASH_VERIFY
 * @brief    Verification of data written to the Flash memory by fnet_cpu_flash_write():
 *               - @c 1 = is enabled.
 *               - @c 0 = is disabled.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_FLASH_VERIFY
    #define FNET_CFG_CPU_FLASH_VERIFY       (0)
#endif

/*! @} */

/*! @addtogroup fnet_platform_netif_config  */
/*! @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH0
 * @brief    Ethernet-0 interface:
 *               - @b @c 1 = is enabled (default value).
 *               - @c 0 = is disabled.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH0
    #define FNET_CFG_CPU_ETH0        	(1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH1
 * @brief    Ethernet-1 interface:
 *               - @c 1 = is enabled. @n
 *                        Supported only by Modelo platform.
 *               - @b @c 0 = is disabled (default value). @n
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH1
    #define FNET_CFG_CPU_ETH1        	(0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH0_NAME
 * @brief    Defines name for the Ethernet-0 interface.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH0_NAME
    #define FNET_CFG_CPU_ETH0_NAME            "eth0"
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH1_NAME
 * @brief    Defines name for the Ethernet-0 interface.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH1_NAME
    #define FNET_CFG_CPU_ETH1_NAME            "eth1"
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_WIFI_NAME
 * @brief    Defines name for the Wi-Fi interface.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_WIFI_NAME
    #define FNET_CFG_CPU_WIFI_NAME            "wifi"
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH0_PHY_ADDR
 * @brief    Default PHY address used by the Ethernet-0 module. @n
 *           Specifies one of up to 32 attached PHY devices. @n
 *           In run-time, the address may be changed  by @ref fnet_eth_phy_set_addr().
 * @see FNET_CFG_CPU_ETH_PHY_ADDR_DISCOVER
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH0_PHY_ADDR
    #define FNET_CFG_CPU_ETH0_PHY_ADDR	(0U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH1_PHY_ADDR
 * @brief    Default PHY address used by the Ethernet-1 module. @n
 *           Specifies one of up to 32 attached PHY devices. @n
 *           In run-time, the address may be changed  by @ref fnet_eth_phy_set_addr().
 * @see FNET_CFG_CPU_ETH_PHY_ADDR_DISCOVER
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH1_PHY_ADDR
    #define FNET_CFG_CPU_ETH1_PHY_ADDR	(1U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_PHY_ADDR_DISCOVER
 * @brief    PHY address discover:
 *               - @c 1 = is enabled. @n
 *                        Ethernet driver trying to discover a valid PHY address.
 *               - @c 0 = is disabled (default value).@n
 *                        PHY address is set to FNET_CFG_CPU_ETHx_PHY_ADDR.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_PHY_ADDR_DISCOVER
    #define FNET_CFG_CPU_ETH_PHY_ADDR_DISCOVER	(0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH0_MTU
 * @brief    Defines the Maximum Transmission Unit for the Ethernet-0 interface.
 *           The largest value is 1500. The Internet Minimum MTU is 576.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH0_MTU
    #if FNET_CFG_CPU_ETH0
        #define FNET_CFG_CPU_ETH0_MTU            (FNET_ETH_MTU)
    #else
        #define FNET_CFG_CPU_ETH0_MTU            (0u)
    #endif
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH1_MTU
 * @brief    Defines the Maximum Transmission Unit for the Ethernet-1 interface.
 *           The largest value is 1500. The Internet Minimum MTU is 576.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH1_MTU
    #if FNET_CFG_CPU_ETH1
        #define FNET_CFG_CPU_ETH1_MTU            (FNET_ETH_MTU)
    #else
        #define FNET_CFG_CPU_ETH1_MTU            (0u)
    #endif
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH0_VECTOR_NUMBER
 * @brief    Vector number of the Ethernet Receive Frame interrupt.
 *           @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#if defined(__DOXYGEN__)
    #define FNET_CFG_CPU_ETH0_VECTOR_NUMBER (0)
#endif
#if FNET_CFG_CPU_ETH0
    #ifndef FNET_CFG_CPU_ETH0_VECTOR_NUMBER
        #error "FNET_CFG_CPU_ETH0_VECTOR_NUMBER is not defined."
    #endif
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH1_VECTOR_NUMBER
 * @brief    Vector number of the Ethernet Receive Frame interrupt.
 *           @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#if defined(__DOXYGEN__)
    #define FNET_CFG_CPU_ETH1_VECTOR_NUMBER (0)
#endif
#if FNET_CFG_CPU_ETH1
    #ifndef FNET_CFG_CPU_ETH1_VECTOR_NUMBER
        #error "FNET_CFG_CPU_ETH1_VECTOR_NUMBER is not defined."
    #endif
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_VECTOR_PRIORITY
 * @brief    Default Interrupt priority level for the Ethernet module.
 *           It can range from 1 to FNET_CFG_CPU_VECTOR_PRIORITY_MAX. The higher the value, the greater
 *           the priority of the corresponding interrupt.
 *           @n @n NOTE: It's ignored for MCF V1.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_VECTOR_PRIORITY
    #if FNET_MCF || FNET_MPC  /* Timer should have a higher priority to provide correct tick value */
        #define FNET_CFG_CPU_ETH_VECTOR_PRIORITY    (FNET_CFG_CPU_TIMER_VECTOR_PRIORITY-1u)
    #else /* ARM */
        #define FNET_CFG_CPU_ETH_VECTOR_PRIORITY    (FNET_CFG_CPU_TIMER_VECTOR_PRIORITY+1u)
    #endif
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_TX_BUFS_MAX
 * @brief    Defines the maximum number of outgoing frames that may
 *           be buffered by the Ethernet module.
 *           As a result
 *           ((FNET_CFG_CPU_ETHx_MTU+18) * @ref FNET_CFG_CPU_ETH_TX_BUFS_MAX)
 *           bytes will be allocated.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_TX_BUFS_MAX
    #define FNET_CFG_CPU_ETH_TX_BUFS_MAX        (2u)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_RX_BUFS_MAX
 * @brief    Defines the maximum number of incoming frames that may
 *           be buffered by the Ethernet module.
 *           As a result
 *           ((FNET_CFG_CPU_ETHx_MTU+18) * @ref FNET_CFG_CPU_ETH_RX_BUFS_MAX)
 *           bytes will be allocated.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_RX_BUFS_MAX
    #define FNET_CFG_CPU_ETH_RX_BUFS_MAX        (2u)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_ATONEGOTIATION_TIMEOUT
 * @brief    Defines Ethernet Autonegotiation Timeout (in ms),
 *           which is performed during the Ethernet interface initilisation.@n
 *           Autonegotiation is an Ethernet procedure by which two connected
 *           devices choose common transmission parameters, such as speed,
 *           duplex mode, and flow control. In this process, the connected
 *           devices first share their capabilities regarding
 *           these parameters and then choose the highest performance
 *           transmission mode they both support.
 *           On practice, it usually takes less than 700ms.@n
 *           @n
 *           Set it to 0 to disable the waiting.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_ATONEGOTIATION_TIMEOUT
    #define FNET_CFG_CPU_ETH_ATONEGOTIATION_TIMEOUT     (2000U) /*ms*/
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_PROMISCUOUS
 * @brief    The Ethernet interface promiscuous mode:
 *               - @c 1 = is enabled. An interface will capture all packets on the LAN.
 *               - @b @c 0 = is disabled (default value). @n
 *                        An interface will only capture packets destined to your interface
 *                        (not all packets on your LAN), to reduce CPU load.
 *  @n @n NOTE: This mode is used mainly for testing needs.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_PROMISCUOUS
    #define FNET_CFG_CPU_ETH_PROMISCUOUS        (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_FULL_DUPLEX
 * @brief    The Ethernet interface full duplex mode:
 *               - @b @c 1 = is enabled (Default value).@n
 *                        Receive path operates independently of transmit.
 *               - @c 0 = is disabled. It corresponds to half-duplex mode. @n
 *                        It disables reception of frames while transmitting.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_FULL_DUPLEX
    #define FNET_CFG_CPU_ETH_FULL_DUPLEX        (1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_RMII
 * @brief    The Reduced Media Independent Interface (RMII) mode for MAC:
 *               - @c 1 = is enabled (Default value for Kinetis and Modelo).
 *               - @c 0 = is disabled (Default value for MCF MCUs).@n
 *                        In this case, it is configured Media Independent Interface (MII) mode.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_RMII
    #define FNET_CFG_CPU_ETH_RMII        		(0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_RMII_10T
 * @brief     10-Mbps mode of the RMII:
 *               - @c 1 = is enabled. @n 10 Mbps operation.
 *               - @b @c 0 = is disabled (Default value).@n
 *                        100 Mbps operation.
 *              @n NOTE: Valid only if @ref FNET_CFG_CPU_ETH_RMII is set.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_RMII_10T
    #define FNET_CFG_CPU_ETH_RMII_10T           (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_MIB
 * @brief    Ethernet Management Information Base (MIB) Block Counters:
 *               - @c 1 = Current platform has the Ethernet MIB Block.
 *               - @c 0 = Current platform does not have Ethernet MIB Block
 *                        (only for MCF51CNx).
 *              @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_MIB
    #define FNET_CFG_CPU_ETH_MIB                (1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_HW_TX_PROTOCOL_CHECKSUM
 * @brief    Insertion of protocol checksum by Ethernet-module:
 *               - @c 1 = is enabled. If an IP frame with a known protocol (UDP, TCP or ICMP) is transmitted, the checksum is inserted automatically into the
 *                        frame. The checksum field must be cleared. The other frames are not modified. @n
 *                        It is supported only by ENET module (K60).
 *               - @c 0 = is disabled. Checksum not inserted.
 *              @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_HW_TX_PROTOCOL_CHECKSUM
    #define FNET_CFG_CPU_ETH_HW_TX_PROTOCOL_CHECKSUM    (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_HW_TX_IP_CHECKSUM
 * @brief    Insertion of IPv4 header checksum by Ethernet-module:
 *               - @c 1 = is enabled. If an IP frame is transmitted, the checksum is inserted automatically. The IP header checksum field
 *                        must be cleared. If a non-IP frame is transmitted the frame is not modified. @n
 *                        It is supported only by ENET module (K60).
 *               - @c 0 = is disabled. Checksum not inserted.
 *              @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_HW_TX_IP_CHECKSUM
    #define FNET_CFG_CPU_ETH_HW_TX_IP_CHECKSUM          (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_CPU_ETH_HW_RX_PROTOCOL_CHECKSUM
 * @brief   Discard of frames with wrong protocol checksum by Ethernet-module:
 *               - @c 1 = is enabled. If a TCP, UDP or ICMP frame is
 *                        received that has a wrong checksum,
 *                        the frame is discarded. @n
 *                        It is supported only by ENET module (K60).
 *               - @c 0 = is disabled. Frames with wrong checksum are not discarded.
 *              @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_HW_RX_PROTOCOL_CHECKSUM
    #define FNET_CFG_CPU_ETH_HW_RX_PROTOCOL_CHECKSUM    (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_HW_RX_IP_CHECKSUM
 * @brief    Discard of frames with wrong IPv4 header checksum by Ethernet-module:
 *               - @c 1 = is enabled. If an IPv4 frame is received with a mismatching header checksum,
 *                         the frame is discarded. @n
 *                        It is supported only by ENET module (K60).
 *               - @c 0 = is disabled. Frames with wrong IPv4 header checksum are not discarded.
 *              @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_HW_RX_IP_CHECKSUM
    #define FNET_CFG_CPU_ETH_HW_RX_IP_CHECKSUM          (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_HW_RX_MAC_ERR
 * @brief    Discard of frames with MAC layer errors by Ethernet-module:
 *               - @c 1 = is enabled. Any frame received with a CRC, length,
 *                        or PHY error is automatically discarded and not forwarded to
 *                        the user application interface. @n
 *                        It is supported only by ENET module (K60).
 *               - @c 0 = is disabled. Frames with errors are not discarded.
 *              @n @n NOTE: User application should not change this parameter.
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_HW_RX_MAC_ERR
    #define FNET_CFG_CPU_ETH_HW_RX_MAC_ERR              (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_ETH_IO_INIT
 * @brief    Ethernet module Input/Output pin initialization:
 *               - @b @c 1 = is enabled (default value).
 *               - @b @c 0 = is disabled. In this case a user application must do own initialization.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_ETH_IO_INIT
    #define FNET_CFG_CPU_ETH_IO_INIT                    (1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_WIFI
 * @brief    Wi-Fi module interface (QCA4002/QCA4004):
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (default value).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_WIFI
    #define FNET_CFG_CPU_WIFI                           (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_WIFI_FW_UPDATE
 * @brief    Wi-Fi module interface firmware update:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (default value).
 * @see fnet_wifi_fw_update()
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_WIFI_FW_UPDATE
    #define FNET_CFG_CPU_WIFI_FW_UPDATE                 (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_CPU_WIFI_IO_INIT
 * @brief    WiFi module Input/Output pin initialization:
 *               - @b @c 1 = is enabled (default value).
 *               - @b @c 0 = is disabled. In this case a user application must do own initialization.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_CPU_WIFI_IO_INIT
    #define FNET_CFG_CPU_WIFI_IO_INIT                   (1)
#endif

/*! @} */

#ifdef FNET_CFG_CPU_ETH0_MAC_ADDR
    #error "FNET_CFG_CPU_ETH0_MAC_ADDR parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_CPU_ETH1_MAC_ADDR
    #error "FNET_CFG_CPU_ETH1_MAC_ADDR parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH_IP4_GW
    #error "FNET_CFG_ETH_IP4_GW parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH_IP4_DNS
    #error "FNET_CFG_ETH_IP4_DNS parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH_MAC_ADDR
    #error "FNET_CFG_ETH_MAC_ADDR parameter is obsolete. It is user application parameter now."
#endif

#endif /* _FNET_CPU_CONFIG_H_ */
