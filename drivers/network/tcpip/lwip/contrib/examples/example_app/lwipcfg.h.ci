/**
 * Additional settings for the example app.
 * Copy this to lwipcfg.h and make the config changes you need.
 */

/* configuration for this port */
#define PPP_USERNAME  "Admin"
#define PPP_PASSWORD  "pass"

/** Define this to the index of the windows network adapter to use */
#define PACKET_LIB_ADAPTER_NR         1
/** Define this to the GUID of the windows network adapter to use
 * or NOT define this if you want PACKET_LIB_ADAPTER_NR to be used */
/*#define PACKET_LIB_ADAPTER_GUID       "00000000-0000-0000-0000-000000000000"*/
/*#define PACKET_LIB_GET_ADAPTER_NETADDRESS(addr) IP4_ADDR((addr), 192,168,1,0)*/
/*#define PACKET_LIB_QUIET*/

/* #define USE_PCAPIF 1 */
#define LWIP_PORT_INIT_IPADDR(addr)   IP4_ADDR((addr), 192,168,1,200)
#define LWIP_PORT_INIT_GW(addr)       IP4_ADDR((addr), 192,168,1,1)
#define LWIP_PORT_INIT_NETMASK(addr)  IP4_ADDR((addr), 255,255,255,0)

/* remember to change this MAC address to suit your needs!
   the last octet will be increased by netif->num for each netif */
#define LWIP_MAC_ADDR_BASE            {0x00,0x01,0x02,0x03,0x04,0x05}

/* #define USE_SLIPIF 0 */
/* #define SIO_USE_COMPORT 0 */
#ifdef USE_SLIPIF
#if USE_SLIPIF
#define LWIP_PORT_INIT_SLIP1_IPADDR(addr)   IP4_ADDR((addr), 192, 168,   2, 2)
#define LWIP_PORT_INIT_SLIP1_GW(addr)       IP4_ADDR((addr), 192, 168,   2, 1)
#define LWIP_PORT_INIT_SLIP1_NETMASK(addr)  IP4_ADDR((addr), 255, 255, 255, 0)
#if USE_SLIPIF > 1
#define LWIP_PORT_INIT_SLIP2_IPADDR(addr)   IP4_ADDR((addr), 192, 168,   2, 1)
#define LWIP_PORT_INIT_SLIP2_GW(addr)       IP4_ADDR((addr), 0,     0,   0, 0)
#define LWIP_PORT_INIT_SLIP2_NETMASK(addr)  IP4_ADDR((addr), 255, 255, 255, 0)*/
#endif /* USE_SLIPIF > 1 */
#endif /* USE_SLIPIF */
#endif /* USE_SLIPIF */

/* configuration for applications */

#define LWIP_CHARGEN_APP              1
#define LWIP_DNS_APP                  1
#define LWIP_HTTPD_APP                LWIP_TCP
/* Set this to 1 to use the netconn http server,
 * otherwise the raw api server will be used. */
/*#define LWIP_HTTPD_APP_NETCONN     */
#define LWIP_NETBIOS_APP              LWIP_IPV4 && LWIP_UDP
#define LWIP_NETIO_APP                1
#define LWIP_MDNS_APP                 LWIP_UDP
#define LWIP_MQTT_APP                 LWIP_TCP
#define LWIP_PING_APP                 1
#define LWIP_RTP_APP                  1
#define LWIP_SHELL_APP                LWIP_TCP
#define LWIP_SNMP_APP                 LWIP_UDP
#define LWIP_SNTP_APP                 LWIP_UDP
#define LWIP_SOCKET_EXAMPLES_APP      1
#define LWIP_TCPECHO_APP              LWIP_TCP
/* Set this to 1 to use the netconn tcpecho server,
 * otherwise the raw api server will be used. */
/*#define LWIP_TCPECHO_APP_NETCONN   */
#define LWIP_TFTP_APP                 LWIP_UDP
#define LWIP_TFTP_CLIENT_APP          LWIP_UDP
#define LWIP_UDPECHO_APP              LWIP_UDP
#define LWIP_LWIPERF_APP              LWIP_TCP

#define USE_DHCP                      LWIP_DHCP
#define USE_AUTOIP                    LWIP_AUTOIP

/* define this to your custom application-init function */
/* #define LWIP_APP_INIT my_app_init() */
