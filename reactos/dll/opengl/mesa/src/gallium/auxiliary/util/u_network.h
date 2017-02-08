
#ifndef _U_NETWORK_H_
#define _U_NETWORK_H_

#include "pipe/p_compiler.h"

#if defined(PIPE_SUBSYSTEM_WINDOWS_USER)
#  define PIPE_HAVE_SOCKETS
#elif defined(PIPE_OS_LINUX) || defined(PIPE_OS_HAIKU) || \
    defined(PIPE_OS_APPLE) || defined(PIPE_OS_SOLARIS)
#  define PIPE_HAVE_SOCKETS
#endif

boolean u_socket_init(void);
void u_socket_stop(void);
void u_socket_close(int s);
int u_socket_listen_on_port(uint16_t portnum);
int u_socket_accept(int s);
int u_socket_connect(const char *host, uint16_t port);
int u_socket_send(int s, void *data, size_t size);
int u_socket_peek(int s, void *data, size_t size);
int u_socket_recv(int s, void *data, size_t size);
void u_socket_block(int s, boolean block);

#endif
