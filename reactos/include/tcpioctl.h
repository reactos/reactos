/*
 * tcpioctl.h
 *
 * Set and query ioctl constants for tcpip.sys
 *
 * Contributors:
 *   Created by Art Yerkes (ayerkes@speakeasy.net) from 
 *    drivers/net/tcpip/include/ticonsts.h
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _TCPIOCTL_H
#define _TCPIOCTL_H

/* TCP/UDP/RawIP IOCTL code definitions */

#define FSCTL_TCP_BASE     FILE_DEVICE_NETWORK

#define _TCP_CTL_CODE(Function, Method, Access) \
    CTL_CODE(FSCTL_TCP_BASE, Function, Method, Access)

#define IOCTL_TCP_QUERY_INFORMATION_EX \
    _TCP_CTL_CODE(0, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_TCP_SET_INFORMATION_EX \
    _TCP_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#endif/*_TCPIOCTL_H*/
