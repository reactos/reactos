/*
 * PROJECT:     ReactOS TCP/IP protocol driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FNET glue, same interface the lwIP glue gives the ReactOS TCP code
 * COPYRIGHT:   Copyright 2026 Muhammad Yahya Bin Ehsan Qureshi <yahyauni54@gmail.com>
 */

#ifndef _FNET_GLUE_H_
#define _FNET_GLUE_H_

#include <tcpip.h>
#include <fnet_ros_port.h>

/* Type only lwIP headers for err_t, ERR_*, ip4_addr_t and u*_t; no lwIP code is built */
#include "lwip/err.h"
#include "lwip/ip_addr.h"

#define FNET_GLUE_PCB_TAG       'pTNF'
#define FNET_GLUE_IF_TAG        'fTNF'
#define FNET_GLUE_PACKET_TAG    'kTNF'

/* Opaque to everything outside the glue */
typedef struct tcp_pcb *PTCP_PCB;

NTSTATUS    LibTCPGetDataFromConnectionQueue(PCONNECTION_ENDPOINT Connection, PUCHAR RecvBuffer, UINT RecvLen, UINT *Received);

/* External TCP event handlers */
extern void TCPConnectEventHandler(void *arg, const err_t err);
extern void TCPAcceptEventHandler(void *arg, PTCP_PCB newpcb);
extern void TCPSendEventHandler(void *arg, const u16_t space);
extern void TCPFinEventHandler(void *arg, const err_t err);
extern void TCPRecvEventHandler(void *arg);

/* TCP functions */
PTCP_PCB    LibTCPSocket(void *arg);
VOID        LibTCPFreeSocket(PTCP_PCB pcb);
err_t       LibTCPBind(PCONNECTION_ENDPOINT Connection, ip4_addr_t *const ipaddr, const u16_t port);
PTCP_PCB    LibTCPListen(PCONNECTION_ENDPOINT Connection, const u8_t backlog);
err_t       LibTCPSend(PCONNECTION_ENDPOINT Connection, void *const dataptr, const ULONG len, ULONG *sent, const int safe);
err_t       LibTCPConnect(PCONNECTION_ENDPOINT Connection, ip4_addr_t *const ipaddr, const u16_t port);
err_t       LibTCPShutdown(PCONNECTION_ENDPOINT Connection, const int shut_rx, const int shut_tx);
err_t       LibTCPClose(PCONNECTION_ENDPOINT Connection, const int safe, const int callback);

err_t       LibTCPGetPeerName(PTCP_PCB pcb, ip4_addr_t *const ipaddr, u16_t *const port);
err_t       LibTCPGetHostName(PTCP_PCB pcb, ip4_addr_t *const ipaddr, u16_t *const port);
void        LibTCPAccept(PTCP_PCB pcb, PTCP_PCB listen_pcb, void *arg);
void        LibTCPSetNoDelay(PTCP_PCB pcb, BOOLEAN Set);
void        LibTCPSetKeepAlive(PTCP_PCB pcb, BOOLEAN Set);
void        LibTcpSetKeepAliveValues(PTCP_PCB pcb, u32_t KeepAliveTime, u32_t KeepAliveInterval);
void        LibTCPGetSocketStatus(PTCP_PCB pcb, PULONG State);

/* FNET only: a listen IRP was queued; connections may already wait in the FNET backlog */
void        LibTCPAcceptQueued(PCONNECTION_ENDPOINT Listener);

/* FNET only: a receive was queued while data may be waiting in FNET */
#define TCP_MAX_RECEIVE_NESTING 4
void        LibTCPKickReceive(PCONNECTION_ENDPOINT Connection);

/* IP functions */
void LibIPInsertPacket(void *ifarg, const void *const data, const u32_t size);
void LibIPInitialize(void);
void LibIPShutdown(void);

/* Between the two glue files */
ULONG       FnetGlueInterfaceScopeId(PIP_INTERFACE IF);
ULONG       FnetGlueInterfaceMtu(PIP_INTERFACE IF);
VOID        FnetGlueAbortAddress(ULONG Address);

#endif
