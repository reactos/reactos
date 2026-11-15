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
*  On-chip Flash Module driver API.
*
***************************************************************************/
#ifndef _FNET_FLASH_H_

#define _FNET_FLASH_H_

#if (FNET_CFG_FLASH && FNET_CFG_CPU_FLASH) || defined(__DOXYGEN__)

/*! @addtogroup fnet_cfm
*
* The Flash driver provides the ability to
* reprogram the internal Flash memory while an application is running
* in normal operational mode.@n
* @n
* The on-chip Flash module is a non-volatile memory (NVM) module
* integrated with a CPU. The Flash memory is ideal for program and
* data storage for single-chip applications, allowing field
* reprogramming without requiring external programming voltage sources.@n
* @n
*
* For Falsh driver example, refer to FNET demo application source code.@n
* @n
* Configuration parameters:
* - @ref FNET_CFG_FLASH
* - @ref FNET_CFG_CPU_FLASH
* - @ref FNET_CFG_CPU_FLASH_ADDRESS
* - @ref FNET_CFG_CPU_FLASH_SIZE
* - @ref FNET_CFG_CPU_FLASH_PAGE_SIZE
*/
/*! @{ */

#if defined(__cplusplus)
extern "C" {
#endif

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
 * @see fnet_flash_memcpy()
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
 * Erase page size is defined by @ref FNET_CFG_CPU_FLASH_PAGE_SIZE.
 *
 ******************************************************************************/
fnet_return_t fnet_flash_erase( void *flash_addr, fnet_size_t bytes);

/***************************************************************************/ /*!
 *
 * @brief    Writes the specified data to the Flash memory.
 *
 * @param flash_addr      Address in the Flash to write to.
 *
 * @param src             Pointer to the buffer containing the
 *                        data to write to the Flash memory.
 *
 * @param n               Number of bytes contained in the data buffer
 *                        pointed by @c src.
 *
 * @return This function returns:
 *   - @ref FNET_OK if successful.
 *   - @ref FNET_ERR if failed.
 *
 * @see fnet_flash_erase(), fnet_flash_flush()
 *
 ******************************************************************************
 *
 * This function copies the number of @c bytes bytes from the location
 * pointed by @c src directly to the Flash memory pointed by @c flash_addr.
 * If the flash write-cache is enabled (@ref FNET_CFG_FLASH_CACHE),
 * after last data block is copyed, application must call fnet_flash_flush(0 to be
 * sure that all data were writen to the flash.
 *
 ******************************************************************************/
fnet_return_t fnet_flash_memcpy( FNET_COMP_PACKED_VAR void *flash_addr, FNET_COMP_PACKED_VAR const void *src, fnet_size_t n );

/***************************************************************************/ /*!
 *
 * @brief    Writes all data from cache to the Flash memory.
 *
 * @return This function returns:
 *   - @ref FNET_OK if successful.
 *   - @ref FNET_ERR if failed.
 *
 * @see fnet_flash_erase(), fnet_flash_memcpy()
 *
 ******************************************************************************
 *
 * If the write-cache is enabled (@ref FNET_CFG_FLASH_CACHE),
 * this function copies all data from the write-cache directly to the Flash memory.
 *
 ******************************************************************************/
fnet_return_t fnet_flash_flush( void );

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* FNET_CFG_FLASH */

#endif /* _FNET_FLASH_H_ */
