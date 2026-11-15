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
*  CPU-specific library API.
*
***************************************************************************/

#ifndef _FNET_CPU_H_

#define _FNET_CPU_H_

#include "fnet_config.h"

#if FNET_MCF    /* ColdFire.*/
    #include "mcf/fnet_mcf.h"
#endif

#if FNET_MK     /* Kinetis.*/
    #include "mk/fnet_mk.h"
#endif

#if FNET_MPC     /* MPC.*/
    #include "mpc/fnet_mpc.h"
#endif

#if FNET_LPC     /* LPC.*/
    #include "lpc/fnet_lpc.h"
#endif

#if FNET_MIMXRT     /* i.MX RT.*/
    #include "mimxrt/fnet_mimxrt.h"
#endif

#include "stack/fnet_stdlib.h"


/* Non-cacheable region definition macro. */
/* Used MCUX "NonCacheable" name for this section. */
#if FNET_CFG_CPU_CACHE
    #if FNET_CFG_COMP_IAR
        #define FNET_AT_NONCACHEABLE_SECTION(var) var @FNET_CFG_CPU_NONCACHEABLE_SECTION
    #elif FNET_CFG_COMP_UV || FNET_CFG_COMP_GNUC || FNET_CFG_COMP_GHS
        #define FNET_AT_NONCACHEABLE_SECTION(var) __attribute__((section(FNET_CFG_CPU_NONCACHEABLE_SECTION))) var
    #else
        #error Compiler is not supported
        #define FNET_AT_NONCACHEABLE_SECTION(var) var
    #endif
#else
    #define FNET_AT_NONCACHEABLE_SECTION(var) var
#endif


/*! @addtogroup fnet_socket */
/*! @{ */

#if defined(__cplusplus)
extern "C" {
#endif

/************************************************************************
*     For doing byte order conversions between CPU
*     and the independent "network" order.
*     For Freescale CPUs they just return the variable passed.
*************************************************************************/

#if FNET_CFG_CPU_LITTLE_ENDIAN /* Little endian.*/ || defined(__DOXYGEN__)

/**************************************************************************/ /*!
 * @def FNET_HTONS
 * @param short_var A 16-bit number in host byte order.
 * @hideinitializer
 * @see FNET_NTOHS(), FNET_HTONL(), FNET_NTOHL(), fnet_htons(), fnet_ntohs(), fnet_htonl(), fnet_ntohl()
 * @brief Macros which converts the unsigned short integer from host byte order to
 * network byte order.
 ******************************************************************************/
#define FNET_HTONS(short_var)   (((((fnet_uint16_t)(short_var)) & 0x00FFU) << 8U) | (((fnet_uint16_t)(short_var) & 0xFF00U) >> 8U))
/**************************************************************************/ /*!
 * @def FNET_NTOHS
 * @param short_var A 16-bit number in network byte order.
 * @hideinitializer
 * @see FNET_HTONS(), FNET_HTONL(), FNET_NTOHL(), fnet_htons(), fnet_ntohs(), fnet_htonl(), fnet_ntohl()
 * @brief Macros which converts the unsigned 16-bit integer from network byte order to
 * host byte order.
 ******************************************************************************/
#define FNET_NTOHS(short_var)   FNET_HTONS(short_var)
/**************************************************************************/ /*!
 * @def FNET_HTONL
 * @param long_var A 32-bit number in host byte order.
 * @hideinitializer
 * @see FNET_HTONS(), FNET_NTOHS(), FNET_NTOHL(), fnet_htons(), fnet_ntohs(), fnet_htonl(), fnet_ntohl()
 * @brief Macros which converts the unsigned long integer from host byte order to
 * network byte order.
 ******************************************************************************/
#define FNET_HTONL(long_var)    ((((long_var) & 0x000000FFU) << 24U) | (((long_var) & 0x0000FF00U) << 8U) | (((long_var) & 0x00FF0000U) >> 8U) | (((long_var) & 0xFF000000U) >> 24U))
/**************************************************************************/ /*!
 * @def FNET_NTOHL
 * @param long_var A 32-bit number in network byte order.
 * @hideinitializer
 * @see FNET_HTONS(), FNET_NTOHS(), FNET_HTONL(), fnet_htons(), fnet_ntohs(), fnet_htonl(), fnet_ntohl()
 * @brief Macros which converts the unsigned long integer from network byte order to
 * host byte order.
 ******************************************************************************/
#define FNET_NTOHL(long_var)    FNET_HTONL(long_var)

/***************************************************************************/ /*!
 *
 * @brief    Converts 16-bit value from host to network byte order.
 *
 *
 * @param short_var A 16-bit number in host byte order.
 *
 *
 * @return This function returns the network byte-ordered @c short_var.
 *
 * @see FNET_HTONS(), FNET_NTOHS(), FNET_HTONL(), FNET_NTOHL(), fnet_ntohs(), fnet_htonl(), fnet_ntohl()
 *
 ******************************************************************************
 *
 * The function converts the unsigned 16-bit integer from host byte order to
 * network byte order.
 *
 ******************************************************************************/
fnet_uint16_t fnet_htons(fnet_uint16_t short_var);

#define fnet_ntohs   fnet_htons
/***************************************************************************/ /*!
 *
 * @brief    Converts 16-bit value from network to host byte order.
 *
 *
 * @param short_var A 16-bit number in network byte order.
 *
 *
 * @return This function returns the host byte-ordered @c short_var.
 *
 * @see FNET_HTONS(), FNET_NTOHS(), FNET_HTONL(), FNET_NTOHL(), fnet_htons(), fnet_htonl(), fnet_ntohl()
 *
 ******************************************************************************
 *
 * The function converts the unsigned 16-bit integer from network byte order to
 * host byte order.
 *
 ******************************************************************************/
fnet_uint16_t fnet_ntohs(fnet_uint16_t short_var);

/***************************************************************************/ /*!
 *
 * @brief    Converts 32-bit value from host to network byte order.
 *
 *
 * @param long_var A 32-bit number in host byte order.
 *
 *
 * @return This function returns the network byte-ordered @c long_var.
 *
 * @see FNET_HTONS(), FNET_NTOHS(), FNET_HTONL(), FNET_NTOHL(), fnet_ntohs(), fnet_htons(), fnet_ntohl()
 *
 ******************************************************************************
 *
 * The function converts the unsigned long integer from host byte order to
 * network byte order.
 *
 ******************************************************************************/
fnet_uint32_t fnet_htonl(fnet_uint32_t long_var);

#define fnet_ntohl    fnet_htonl
/***************************************************************************/ /*!
 *
 * @brief    Converts 32-bit value from network to host byte order.
 *
 *
 * @param long_var A 32-bit number in network byte order.
 *
 *
 * @return This function returns the host byte-ordered @c long_var.
 *
 * @see FNET_HTONS(), FNET_NTOHS(), FNET_HTONL(), FNET_NTOHL(), fnet_htons(), fnet_ntohs(), fnet_htonl()
 *
 ******************************************************************************
 *
 * The function converts the unsigned long integer from network byte order to
 * host byte order.
 *
 ******************************************************************************/
fnet_uint32_t fnet_ntohl(fnet_uint32_t long_var);


#else /* Big endian. */
#define FNET_HTONS(short_var)   (short_var)
#define FNET_NTOHS(short_var)   (short_var)
#define FNET_HTONL(long_var)    (long_var)
#define FNET_NTOHL(long_var)    (long_var)

#define fnet_htons(short_var)   (short_var) /* Convert 16 bit integer from host- to network byte order.*/
#define fnet_ntohs(short_var)   (short_var) /* Convert 16 bit integer from network - to host byte order.*/
#define fnet_htonl(long_var)    (long_var)  /* Convert 32 bit integer from host- to network byte order.*/
#define fnet_ntohl(long_var)    (long_var)  /* Convert 32 bit integer from network - to host byte order.*/
#endif

/*! @} */

/*! @addtogroup fnet_cpu
* The CPU-specific library provides commonly used platform dependent functions.
* Most of these functions are not used by the FNET, but can be useful for
* an application.
*/
/*! @{ */

/**************************************************************************/ /*!
 * @brief IRQ status descriptor returned by the @ref fnet_cpu_irq_disable() function.
 * Actually it corresponds to the interrupt level mask value.
 * @see fnet_cpu_irq_disable(), fnet_cpu_irq_enable()
 ******************************************************************************/
typedef fnet_uint32_t fnet_cpu_irq_desc_t;

/***************************************************************************/ /*!
 *
 * @brief    Initiates software reset.
 *
 ******************************************************************************
 *
 * This function performs software reset and asserts the external reset
 * (RSTO) pin.
 *
 ******************************************************************************/
void fnet_cpu_reset (void);

/***************************************************************************/ /*!
 *
 * @brief    Disables all interrupts.
 *
 * @return This function returns the current IRQ status defined
 * by @ref fnet_cpu_irq_desc_t.
 *
 * @see fnet_cpu_irq_enable()
 *
 ******************************************************************************
 *
 * This function disables all interrupts. @n
 * The interrupts can be enabled again by the @ref fnet_cpu_irq_enable() function.
 *
 ******************************************************************************/
fnet_cpu_irq_desc_t fnet_cpu_irq_disable(void);

/***************************************************************************/ /*!
 *
 * @brief    Enables interrupts.
 *
 * @param irq_desc     IRQ status descriptor returned by the
 *                      @ref fnet_cpu_irq_disable() function.@n
 *                      Pass @c 0 value to enable all interrupts.
 *
 * @see fnet_cpu_irq_disable()
 *
 ******************************************************************************
 *
 * This function enables interrupts that were disabled by the
 * @ref fnet_cpu_irq_disable function. @n
 * The functions can enable all interrupts by passing into it the @c 0 value.
 *
 ******************************************************************************/
void fnet_cpu_irq_enable(fnet_cpu_irq_desc_t irq_desc);

/***************************************************************************/ /*!
 *
 * @brief    Writes character to the serial port.
 *
 * @param port_number     Serial port number.
 *
 * @param character       Character to be written to the serial port.
 *
 * @see fnet_cpu_serial_getchar(), fnet_cpu_serial_init()
 *
 ******************************************************************************
 *
 * This function writes @c character to the serial port defined
 * by @c port_number. @n
 *
 ******************************************************************************/
void fnet_cpu_serial_putchar( fnet_index_t port_number, fnet_char_t character );

/***************************************************************************/ /*!
 *
 * @brief    Reads character from the serial port.
 *
 * @param port_number     Serial port number.
 *
 * @return This function returns:
 *   - character received by the serial port.
 *   - @ref FNET_ERR if no character is available.
 *
 * @see fnet_cpu_serial_putchar(), fnet_cpu_serial_init()
 *
 ******************************************************************************
 *
 * This function reads character from the serial port defined
 * by @c port_number. @n
 *
 ******************************************************************************/
fnet_int32_t fnet_cpu_serial_getchar( fnet_index_t port_number );

/***************************************************************************/ /*!
 *
 * @brief    Initializes the serial port.
 *
 * @param port_number     Serial port number.
 *
 * @param baud_rate       Baud rate to be set to the serial port.
 *
 * @return This function returns:
 *   - @ref FNET_OK if successful.
 *   - @ref FNET_ERR if failed.
 *
 * @see fnet_cpu_serial_putchar(), fnet_cpu_serial_getchar()
 *
 ******************************************************************************
 *
 * This function executes the HW initialization of the serial port defined
 * by the @c port_number.
 *
 ******************************************************************************/
fnet_return_t fnet_cpu_serial_init(fnet_index_t port_number, fnet_uint32_t baud_rate);

/***************************************************************************/ /*!
 *
 * @brief    Erases the specified range of the Flash memory.
 *
 * @param flash_addr      Address in the Flash to erase from.
 *
 * @param bytes           Number of bytes to erase in the Flash memory.
 *
 * @return This function returns:
 *   - @ref FNET_OK if successful.
 *   - @ref FNET_ERR if failed.
 *
 * @see fnet_cpu_flash_write()
 *
 ******************************************************************************
 *
 * This function attempt to erase the number of @c bytes bytes beginning
 * at @c flash_addr.@n
 * It should be noted that the Flash is block oriented when erasing.
 * It is not possible to erase a few bytes within a page, the whole page will
 * be erased.
 * The @c flash_addr parameter may be anywhere within the first page to be
 * erased and @c flash_addr+ @c bytes may be anywhere in the last block to
 * be erased. @n
 *
 ******************************************************************************/
fnet_return_t  fnet_cpu_flash_erase(void *flash_addr, fnet_size_t bytes);

/***************************************************************************/ /*!
 *
 * @brief    Writes the specified data to the Flash memory.
 *
 * @param dest            Destination address in the Flash to write to.
 *
 * @param data            Pointer to the block of data to write to the Flash memory
 *                        Size of the data block must be equal to
 *                        @ref FNET_CFG_CPU_FLASH_PROGRAM_SIZE.
 *
 * @return This function returns:
 *   - @ref FNET_OK if successful.
 *   - @ref FNET_ERR if failed.
 *
 * @see fnet_cpu_flash_erase()
 *
 ******************************************************************************
 *
 * This function copies @ref FNET_CFG_CPU_FLASH_PROGRAM_SIZE bytes pointed by @c data
 * to the Flash memory pointed by @c dest.
 *
 ******************************************************************************/
fnet_return_t  fnet_cpu_flash_write(fnet_uint8_t *dest, const fnet_uint8_t *data);

/***************************************************************************/ /*!
 *
 * @brief    CPU-specific FNET interrupt service routine.
 *
 ******************************************************************************
 *
 * This is CPU-specific ISR which is executed on every FNET interrupt
 * (from Ethernet and timer module).@n
 * It extracts vector number and calls _fnet_isr_handler().
 *
 ******************************************************************************/
void fnet_cpu_isr(void);

/***************************************************************************/ /*!
 * @def FNET_CPU_ADDR_TO_INSTRUCTION
 *
 * @brief           Converts address value to the instruction.
 *
 * @param addr      Address to be adjusted.
 *
 * @see FNET_CPU_INSTRUCTION_TO_ADDR
 ******************************************************************************
 *
 * This is CPU-specific macro that Converts address value to the instruction address. @n
 * If the current platform is Kinetis, it sets the Thumb bit (bit 0) of
 * the address to 1. @n
 * If the current platform is ColdFire or MPC, it does nothing.
 *
 ******************************************************************************/
#ifndef FNET_CPU_ADDR_TO_INSTRUCTION
#define FNET_CPU_ADDR_TO_INSTRUCTION(addr)    (addr)
#endif

/***************************************************************************/ /*!
 * @def FNET_CPU_INSTRUCTION_TO_ADDR
 *
 * @brief           Converts instruction to address value.
 *
 * @param addr      Instruction to be adjusted.
 *
 * @see FNET_CPU_ADDR_TO_INSTRUCTION
 ******************************************************************************
 *
 * This is CPU-specific macro that Converts instruction to address value. @n
 * If the current platform is Kinetis, it sets the Thumb bit (bit 0) of
 * the address to 0. @n
 * If the current platform is ColdFire or MPC, it does nothing.
 *
 ******************************************************************************/
#ifndef FNET_CPU_INSTRUCTION_TO_ADDR
#define FNET_CPU_INSTRUCTION_TO_ADDR(addr)    (addr)
#endif

/***************************************************************************/ /*!
 * @def FNET_CPU_DATA_MEMORY_BARRIER
 *
 * @brief       Ensures that all data memory transfers are completed.
 *
 ******************************************************************************
 *
 * This is barrier which ensures that all data memory transfers are completed.
 * It is required between a CPU memory access and a DMA operation.
 *
 ******************************************************************************/
#ifndef FNET_CPU_DATA_MEMORY_BARRIER
#define FNET_CPU_DATA_MEMORY_BARRIER do{}while(0)
#endif


struct fnet_netif; /* Forward declaration.*/
#if FNET_CFG_CPU_ETH0
extern struct fnet_netif fnet_cpu_eth0_if;
#define FNET_CPU_ETH0_IF ((fnet_netif_desc_t)(&fnet_cpu_eth0_if))
#endif
#if FNET_CFG_CPU_ETH1
extern struct fnet_netif  fnet_cpu_eth1_if;
#define FNET_CPU_ETH1_IF ((fnet_netif_desc_t)(&fnet_cpu_eth1_if))
#endif
#if FNET_CFG_CPU_WIFI
extern struct fnet_netif  fnet_cpu_wifi_if;
#define FNET_CPU_WIFI_IF ((fnet_netif_desc_t)(&fnet_cpu_wifi_if))
#endif

/***************************************************************************/ /*!
 * @def FNET_CPU_NETIF_NUMBER
 * @brief Number of supported network interfaces.
 ******************************************************************************/
#define FNET_CPU_NETIF_NUMBER      (FNET_CFG_CPU_ETH0+FNET_CFG_CPU_ETH1+FNET_CFG_CPU_WIFI)

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _FNET_CPU_H_ */
