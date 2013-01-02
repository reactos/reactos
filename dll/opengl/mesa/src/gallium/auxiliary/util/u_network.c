
#include "pipe/p_compiler.h"
#include "util/u_network.h"
#include "util/u_debug.h"

#if defined(PIPE_SUBSYSTEM_WINDOWS_USER)
#  include <winsock2.h>
#  include <windows.h>
#elif defined(PIPE_OS_LINUX) || defined(PIPE_OS_HAIKU) || \
   defined(PIPE_OS_APPLE) || defined(PIPE_OS_CYGWIN) || defined(PIPE_OS_SOLARIS)
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <netdb.h>
#else
#  warning "No socket implementation"
#endif

boolean
u_socket_init()
{
#if defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   WORD wVersionRequested;
   WSADATA wsaData;
   int err;

   /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
   wVersionRequested = MAKEWORD(1, 1);

   err = WSAStartup(wVersionRequested, &wsaData);
   if (err != 0) {
      debug_printf("WSAStartup failed with error: %d\n", err);
      return FALSE;
   }
   return TRUE;
#elif defined(PIPE_HAVE_SOCKETS)
   return TRUE;
#else
   return FALSE;
#endif
}

void
u_socket_stop()
{
#if defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   WSACleanup();
#endif
}

void
u_socket_close(int s)
{
   if (s < 0)
      return;

#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_HAIKU) \
    || defined(PIPE_OS_APPLE) || defined(PIPE_OS_SOLARIS)
   shutdown(s, SHUT_RDWR);
   close(s);
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   shutdown(s, SD_BOTH);
   closesocket(s);
#else
   assert(0);
#endif
}

int u_socket_accept(int s)
{
#if defined(PIPE_HAVE_SOCKETS)
   return accept(s, NULL, NULL);
#else
   return -1;
#endif
}

int
u_socket_send(int s, void *data, size_t size)
{
#if defined(PIPE_HAVE_SOCKETS)
   return send(s, data, size, 0);
#else
   return -1;
#endif
}

int
u_socket_peek(int s, void *data, size_t size)
{
#if defined(PIPE_HAVE_SOCKETS)
   return recv(s, data, size, MSG_PEEK);
#else
   return -1;
#endif
}

int
u_socket_recv(int s, void *data, size_t size)
{
#if defined(PIPE_HAVE_SOCKETS)
   return recv(s, data, size, 0);
#else
   return -1;
#endif
}

int
u_socket_connect(const char *hostname, uint16_t port)
{
#if defined(PIPE_HAVE_SOCKETS)
   int s;
   struct sockaddr_in sa;
   struct hostent *host = NULL;

   memset(&sa, 0, sizeof(struct sockaddr_in));
   host = gethostbyname(hostname);
   if (!host)
      return -1;

   memcpy((char *)&sa.sin_addr,host->h_addr_list[0],host->h_length);
   sa.sin_family= host->h_addrtype;
   sa.sin_port = htons(port);

   s = socket(host->h_addrtype, SOCK_STREAM, IPPROTO_TCP);
   if (s < 0)
      return -1;

   if (connect(s, (struct sockaddr *)&sa, sizeof(sa))) {
      u_socket_close(s);
      return -1;
   }

   return s;
#else
   assert(0);
   return -1;
#endif
}

int
u_socket_listen_on_port(uint16_t portnum)
{
#if defined(PIPE_HAVE_SOCKETS)
   int s;
   struct sockaddr_in sa;
   memset(&sa, 0, sizeof(struct sockaddr_in));

   sa.sin_family = AF_INET;
   sa.sin_port = htons(portnum);

   s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (s < 0)
      return -1;

   if (bind(s, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) == -1) {
      u_socket_close(s);
      return -1;
   }

   listen(s, 0);

   return s;
#else
   assert(0);
   return -1;
#endif
}

void
u_socket_block(int s, boolean block)
{
#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_HAIKU) \
    || defined(PIPE_OS_APPLE) || defined(PIPE_OS_SOLARIS)
   int old = fcntl(s, F_GETFL, 0);
   if (old == -1)
      return;

   /* TODO obey block */
   if (block)
      fcntl(s, F_SETFL, old & ~O_NONBLOCK);
   else
      fcntl(s, F_SETFL, old | O_NONBLOCK);
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   u_long iMode = block ? 0 : 1;
   ioctlsocket(s, FIONBIO, &iMode);
#else
   assert(0);
#endif
}
