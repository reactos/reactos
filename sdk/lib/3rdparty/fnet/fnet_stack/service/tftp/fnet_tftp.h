/**************************************************************************
*
* Copyright 2008-2016 by Andrey Butok. FNET Community.
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
*  TFTP common API.
*
***************************************************************************/

#ifndef _FNET_TFTP_H_

#define _FNET_TFTP_H_

#if FNET_CFG_TFTP_CLN || FNET_CFG_TFTP_SRV || defined(__DOXYGEN__)

/*! @addtogroup fnet_tftp
* The user application can use the Trivial File Transfer Protocol (TFTP) client and server to transfer files
* between machines on different networks implementing UDP.
*/
/*! @{ */

/**************************************************************************/ /*!
 * @brief Maximum data size transferred in one data block.
 * @showinitializer
 *
 * The single data transfer is from zero to 512 bytes long. If it is 512 bytes
 * long, the transferred data block is not the last block of data. If it is from
 * zero to 511 bytes long, it signals the end of transfer.
 ******************************************************************************/
#define FNET_TFTP_DATA_SIZE_MAX         (512u)

/**************************************************************************/ /*!
 * @brief The TFTP request type. It defines a TFTP service behavior,
 * if it will read or write a file from/to a TFTP server or client.
 ******************************************************************************/
typedef enum
{
    FNET_TFTP_REQUEST_READ,   /**< @brief TFTP read request (PRQ). The TFTP client
                               * reads a file from the TFTP server.
                               */
    FNET_TFTP_REQUEST_WRITE   /**< @brief TFTP write request (WRQ). The TFTP client
                               * writes a file to the TFTP server.
                               */
} fnet_tftp_request_t;


/**************************************************************************/ /*!
 * @brief TFTP error codes indicating the nature of the error according
 * to RFC 1350.
 ******************************************************************************/
typedef enum
{
    FNET_TFTP_ERROR_NOT_DEFINED         = 0,    /**< @brief Not defined, see error
                                                 * message (if any).*/
    FNET_TFTP_ERROR_FILE_NOT_FOUND      = 1,    /**< @brief File not found.*/
    FNET_TFTP_ERROR_ACCESS_VIOLATION    = 2,    /**< @brief Access violation.*/
    FNET_TFTP_ERROR_DISK_FULL           = 3,    /**< @brief Disk full or allocation
                                                 * exceeded.*/
    FNET_TFTP_ERROR_ILLEGAL_OPERATION   = 4,    /**< @brief Illegal TFTP operation.*/
    FNET_TFTP_ERROR_UNKNOWN_TID         = 5,    /**< @brief Unknown transfer ID.*/
    FNET_TFTP_ERROR_FILE_ALREADY_EXISTS = 6,    /**< @brief File already exists.*/
    FNET_TFTP_ERROR_NO_SUCH_USER        = 7     /**< @brief No such user*/
} fnet_tftp_error_t;

/*! @} */

#endif /* FNET_CFG_TFTP_CLN || FNET_CFG_TFTP_SRV */

#endif /* _FNET_TFTP_H_ */
