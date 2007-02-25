/* WSIPX.H - initially taken from the Wine project
 */

#ifndef _WSIPX_H
#define _WSIPX_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NSPROTO_IPX	1000
#define NSPROTO_SPX	1256
#define NSPROTO_SPXII	1257

typedef struct sockaddr_ipx {
	short sa_family;
	char sa_netnum[4];
	char sa_nodenum[6];
	unsigned short sa_socket;
} SOCKADDR_IPX, *PSOCKADDR_IPX, *LPSOCKADDR_IPX;

#ifdef __cplusplus
}
#endif
#endif
