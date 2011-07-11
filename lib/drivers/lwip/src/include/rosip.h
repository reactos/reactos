#ifndef _ROS_IP_H_
#define _ROS_IP_H_

#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"

/* External TCP event handlers */
extern void TCPConnectEventHandler(void *arg, const err_t err);
extern void TCPAcceptEventHandler(void *arg, struct tcp_pcb *newpcb);
extern void TCPSendEventHandler(void *arg, const u16_t space);
extern void TCPFinEventHandler(void *arg, const err_t err);
extern u32_t TCPRecvEventHandler(void *arg, struct pbuf *p);

/* TCP functions */
struct tcp_pcb *LibTCPSocket(void *arg);
err_t LibTCPBind(struct tcp_pcb *pcb, struct ip_addr *const ipaddr, const u16_t port);
struct tcp_pcb *LibTCPListen(struct tcp_pcb *pcb, const u8_t backlog);
err_t LibTCPSend(struct tcp_pcb *pcb, void *const dataptr, const u16_t len, const int safe);
err_t LibTCPConnect(struct tcp_pcb *pcb, struct ip_addr *const ipaddr, const u16_t port);
err_t LibTCPShutdown(struct tcp_pcb *pcb, const int shut_rx, const int shut_tx);
err_t LibTCPClose(struct tcp_pcb *pcb, const int safe);
err_t LibTCPGetPeerName(struct tcp_pcb *pcb, struct ip_addr *const ipaddr, u16_t *const port);
err_t LibTCPGetHostName(struct tcp_pcb *pcb, struct ip_addr *const ipaddr, u16_t *const port);
void LibTCPAccept(struct tcp_pcb *pcb, struct tcp_pcb *listen_pcb, void *arg);

/* IP functions */
void LibIPInsertPacket(void *ifarg, const void *const data, const u32_t size);
void LibIPInitialize(void);
void LibIPShutdown(void);

#endif