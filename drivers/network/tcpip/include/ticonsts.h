/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/ticonsts.h
 * PURPOSE:     TCP/IP protocol driver constants
 */

#pragma once

/* NDIS version this driver supports */
#define NDIS_VERSION_MAJOR 4
#define NDIS_VERSION_MINOR 0

#ifdef _NTTEST_
/* Name of devices */
#define DD_TCP_DEVICE_NAME      L"\\Device\\NTTcp"
#define DD_UDP_DEVICE_NAME      L"\\Device\\NTUdp"
#define DD_IP_DEVICE_NAME       L"\\Device\\NTIp"
#define DD_RAWIP_DEVICE_NAME    L"\\Device\\NTRawIp"

/* For NDIS protocol registration */
#define IP_DEVICE_NAME          L"\\Device\\NTIp"
#else
#define DD_TCP_DEVICE_NAME      L"\\Device\\Tcp"
#define DD_UDP_DEVICE_NAME      L"\\Device\\Udp"
#define DD_IP_DEVICE_NAME       L"\\Device\\Ip"
#define DD_RAWIP_DEVICE_NAME    L"\\Device\\RawIp"

/* For NDIS protocol registration */
/* The DDK says you have to register with the name that's registered with SCM, e.g. tcpip */
#define IP_DEVICE_NAME          L"\\Device\\Ip"
#define TCPIP_PROTOCOL_NAME     L"Tcpip"
#endif /* _NTTEST_ */

/* Unique error values for log entries */
#define TI_ERROR_DRIVERENTRY 0

/* EOF */
