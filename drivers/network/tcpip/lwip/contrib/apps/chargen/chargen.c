/** @file
 *
 *  chargen server for lwip
 */
/*
 * Copyright (c) 2003 NBS Card Technology, Paramus, NJ.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: David Haas <dhaas@alum.rpi.edu>
 *
 * Purpose:   chargen server for testing and demonstration purposes
 *
 * This file implements a nice example of handling multiple tcp sockets in a
 * server environment. Just call chargen_init() from your application after
 * you have initialized lwip and added your network interfaces. Change the
 * MAX_SERV option to increase or decrease the number of sessions supported.
 *
 * chargen will jam as much data as possible into the output socket, so it
 * will take up a lot of CPU time. Therefore it will be a good idea to run it
 * as the lowest possible priority (just ahead of any idle task).
 *
 * This is also a good example of how to support multiple sessions in an
 * embedded system where you might not have fork().
 */

#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/mem.h"

#include <string.h>

#include "chargen.h"

#if LWIP_SOCKET && LWIP_SOCKET_SELECT

#define MAX_SERV                 5         /* Maximum number of chargen services. Don't need too many */
#define CHARGEN_THREAD_NAME      "chargen"
#define CHARGEN_PRIORITY         254       /* Really low priority */
#define CHARGEN_THREAD_STACKSIZE 0
#define SEND_SIZE TCP_SNDLOWAT             /* If we only send this much, then when select
                                              says we can send, we know we won't block */

struct charcb {
  struct charcb *next;
  int socket;
  struct sockaddr_storage cliaddr;
  socklen_t clilen;
  char nextchar;
};

static struct charcb *charcb_list = NULL;

/**************************************************************
 * void close_chargen(struct charcb *p_charcb)
 *
 * Close the socket and remove this charcb from the list.
 **************************************************************/
static void
close_chargen(struct charcb *p_charcb)
{
  struct charcb *p_search_charcb;

  /* Either an error or tcp connection closed on other
   * end. Close here */
  lwip_close(p_charcb->socket);
  /* Free charcb */
  if (charcb_list == p_charcb) {
    charcb_list = p_charcb->next;
  } else {
    for (p_search_charcb = charcb_list; p_search_charcb; p_search_charcb = p_search_charcb->next) {
      if (p_search_charcb->next == p_charcb) {
        p_search_charcb->next = p_charcb->next;
        break;
      }
    }
  }
  mem_free(p_charcb);
}

/**************************************************************
 * void do_read(struct charcb *p_charcb)
 *
 * Socket definitely is ready for reading. Read a buffer from the socket and
 * discard the data.  If no data is read, then the socket is closed and the
 * charcb is removed from the list and freed.
 **************************************************************/
static int
do_read(struct charcb *p_charcb)
{
  char buffer[80];
  int readcount;

  /* Read some data */
  readcount = lwip_read(p_charcb->socket, &buffer, 80);
  if (readcount <= 0) {
    close_chargen(p_charcb);
    return -1;
  }
  return 0;
}

/**************************************************************
 * void chargen_thread(void *arg)
 *
 * chargen task. This server will wait for connections on well
 * known TCP port number: 19. For every connection, the server will
 * write as much data as possible to the tcp port.
 **************************************************************/
static void
chargen_thread(void *arg)
{
  int listenfd;
#if LWIP_IPV6
  struct sockaddr_in6 chargen_saddr;
#else /* LWIP_IPV6 */
  struct sockaddr_in chargen_saddr;
#endif /* LWIP_IPV6 */
  fd_set readset;
  fd_set writeset;
  int i, maxfdp1;
  struct charcb *p_charcb;
  LWIP_UNUSED_ARG(arg);

  memset(&chargen_saddr, 0, sizeof (chargen_saddr));
#if LWIP_IPV6
  /* First acquire our socket for listening for connections */
  listenfd = lwip_socket(AF_INET6, SOCK_STREAM, 0);
  chargen_saddr.sin6_family = AF_INET6;
  chargen_saddr.sin6_addr = in6addr_any;
  chargen_saddr.sin6_port = lwip_htons(19); /* Chargen server port */
#else /* LWIP_IPV6 */
  /* First acquire our socket for listening for connections */
  listenfd = lwip_socket(AF_INET, SOCK_STREAM, 0);
  chargen_saddr.sin_family = AF_INET;
  chargen_saddr.sin_addr.s_addr = PP_HTONL(INADDR_ANY);
  chargen_saddr.sin_port = lwip_htons(19); /* Chargen server port */
#endif /* LWIP_IPV6 */

  LWIP_ASSERT("chargen_thread(): Socket create failed.", listenfd >= 0);

  if (lwip_bind(listenfd, (struct sockaddr *) &chargen_saddr, sizeof (chargen_saddr)) == -1) {
    LWIP_ASSERT("chargen_thread(): Socket bind failed.", 0);
  }

  /* Put socket into listening mode */
  if (lwip_listen(listenfd, MAX_SERV) == -1) {
    LWIP_ASSERT("chargen_thread(): Listen failed.", 0);
  }


  /* Wait forever for network input: This could be connections or data */
  for (;;) {
    maxfdp1 = listenfd + 1;

    /* Determine what sockets need to be in readset */
    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_SET(listenfd, &readset);
    for (p_charcb = charcb_list; p_charcb; p_charcb = p_charcb->next) {
      if (maxfdp1 < p_charcb->socket + 1) {
        maxfdp1 = p_charcb->socket + 1;
      }
      FD_SET(p_charcb->socket, &readset);
      FD_SET(p_charcb->socket, &writeset);
    }

    /* Wait for data or a new connection */
    i = lwip_select(maxfdp1, &readset, &writeset, NULL, NULL);

    if (i == 0) {
      continue;
    }
    /* At least one descriptor is ready */
    if (FD_ISSET(listenfd, &readset)) {
      /* We have a new connection request!!! */
      /* Lets create a new control block */
      p_charcb = (struct charcb *) mem_malloc(sizeof (struct charcb));
      if (p_charcb) {
        p_charcb->socket = lwip_accept(listenfd,
                (struct sockaddr *) &p_charcb->cliaddr,
                &p_charcb->clilen);
        if (p_charcb->socket < 0) {
          mem_free(p_charcb);
        } else {
          /* Keep this tecb in our list */
          p_charcb->next = charcb_list;
          charcb_list = p_charcb;
          p_charcb->nextchar = 0x21;
        }
      } else {
        /* No memory to accept connection. Just accept and then close */
        int sock;
        struct sockaddr cliaddr;
        socklen_t clilen;

        sock = lwip_accept(listenfd, &cliaddr, &clilen);
        if (sock >= 0) {
          lwip_close(sock);
        }
      }
    }
    /* Go through list of connected clients and process data */
    for (p_charcb = charcb_list; p_charcb; p_charcb = p_charcb->next) {
      if (FD_ISSET(p_charcb->socket, &readset)) {
        /* This socket is ready for reading. This could be because someone typed
         * some characters or it could be because the socket is now closed. Try reading
         * some data to see. */
        if (do_read(p_charcb) < 0) {
          break;
        }
      }
      if (FD_ISSET(p_charcb->socket, &writeset)) {
        char line[80];
        char setchar = p_charcb->nextchar;

        for (i = 0; i < 59; i++) {
          line[i] = setchar;
          if (++setchar == 0x7f) {
            setchar = 0x21;
          }
        }
        line[i] = 0;
        strcat(line, "\n\r");
        if (lwip_write(p_charcb->socket, line, strlen(line)) < 0) {
          close_chargen(p_charcb);
          break;
        }
        if (++p_charcb->nextchar == 0x7f) {
          p_charcb->nextchar = 0x21;
        }
      }
    }
  }
}


/**************************************************************
 * void chargen_init(void)
 *
 * This function initializes the chargen service. This function
 * may only be called either before or after tasking has started.
 **************************************************************/
void
chargen_init(void)
{
  sys_thread_new(CHARGEN_THREAD_NAME, chargen_thread, NULL, CHARGEN_THREAD_STACKSIZE, CHARGEN_PRIORITY);
}

#endif /* LWIP_SOCKET && LWIP_SOCKET_SELECT */
