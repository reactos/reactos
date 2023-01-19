/**
 * @file
 * HTTP client
 */

/*
 * Copyright (c) 2018 Simon Goldschmidt <goldsimon@gmx.de>
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
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 */

/**
 * @defgroup httpc HTTP client
 * @ingroup apps
 * @todo:
 * - persistent connections
 * - select outgoing http version
 * - optionally follow redirect
 * - check request uri for invalid characters? (e.g. encode spaces)
 * - IPv6 support
 */

#include "lwip/apps/http_client.h"

#include "lwip/altcp_tcp.h"
#include "lwip/dns.h"
#include "lwip/debug.h"
#include "lwip/mem.h"
#include "lwip/altcp_tls.h"
#include "lwip/init.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if LWIP_TCP && LWIP_CALLBACK_API

/**
 * HTTPC_DEBUG: Enable debugging for HTTP client.
 */
#ifndef HTTPC_DEBUG
#define HTTPC_DEBUG                 LWIP_DBG_OFF
#endif

/** Set this to 1 to keep server name and uri in request state */
#ifndef HTTPC_DEBUG_REQUEST
#define HTTPC_DEBUG_REQUEST         0
#endif

/** This string is passed in the HTTP header as "User-Agent: " */
#ifndef HTTPC_CLIENT_AGENT
#define HTTPC_CLIENT_AGENT "lwIP/" LWIP_VERSION_STRING " (http://savannah.nongnu.org/projects/lwip)"
#endif

/* the various debug levels for this file */
#define HTTPC_DEBUG_TRACE        (HTTPC_DEBUG | LWIP_DBG_TRACE)
#define HTTPC_DEBUG_STATE        (HTTPC_DEBUG | LWIP_DBG_STATE)
#define HTTPC_DEBUG_WARN         (HTTPC_DEBUG | LWIP_DBG_LEVEL_WARNING)
#define HTTPC_DEBUG_WARN_STATE   (HTTPC_DEBUG | LWIP_DBG_LEVEL_WARNING | LWIP_DBG_STATE)
#define HTTPC_DEBUG_SERIOUS      (HTTPC_DEBUG | LWIP_DBG_LEVEL_SERIOUS)

#define HTTPC_POLL_INTERVAL     1
#define HTTPC_POLL_TIMEOUT      30 /* 15 seconds */

#define HTTPC_CONTENT_LEN_INVALID 0xFFFFFFFF

/* GET request basic */
#define HTTPC_REQ_11 "GET %s HTTP/1.1\r\n" /* URI */\
    "User-Agent: %s\r\n" /* User-Agent */ \
    "Accept: */*\r\n" \
    "Connection: Close\r\n" /* we don't support persistent connections, yet */ \
    "\r\n"
#define HTTPC_REQ_11_FORMAT(uri) HTTPC_REQ_11, uri, HTTPC_CLIENT_AGENT

/* GET request with host */
#define HTTPC_REQ_11_HOST "GET %s HTTP/1.1\r\n" /* URI */\
    "User-Agent: %s\r\n" /* User-Agent */ \
    "Accept: */*\r\n" \
    "Host: %s\r\n" /* server name */ \
    "Connection: Close\r\n" /* we don't support persistent connections, yet */ \
    "\r\n"
#define HTTPC_REQ_11_HOST_FORMAT(uri, srv_name) HTTPC_REQ_11_HOST, uri, HTTPC_CLIENT_AGENT, srv_name

/* GET request with proxy */
#define HTTPC_REQ_11_PROXY "GET http://%s%s HTTP/1.1\r\n" /* HOST, URI */\
    "User-Agent: %s\r\n" /* User-Agent */ \
    "Accept: */*\r\n" \
    "Host: %s\r\n" /* server name */ \
    "Connection: Close\r\n" /* we don't support persistent connections, yet */ \
    "\r\n"
#define HTTPC_REQ_11_PROXY_FORMAT(host, uri, srv_name) HTTPC_REQ_11_PROXY, host, uri, HTTPC_CLIENT_AGENT, srv_name

/* GET request with proxy (non-default server port) */
#define HTTPC_REQ_11_PROXY_PORT "GET http://%s:%d%s HTTP/1.1\r\n" /* HOST, host-port, URI */\
    "User-Agent: %s\r\n" /* User-Agent */ \
    "Accept: */*\r\n" \
    "Host: %s\r\n" /* server name */ \
    "Connection: Close\r\n" /* we don't support persistent connections, yet */ \
    "\r\n"
#define HTTPC_REQ_11_PROXY_PORT_FORMAT(host, host_port, uri, srv_name) HTTPC_REQ_11_PROXY_PORT, host, host_port, uri, HTTPC_CLIENT_AGENT, srv_name

typedef enum ehttpc_parse_state {
  HTTPC_PARSE_WAIT_FIRST_LINE = 0,
  HTTPC_PARSE_WAIT_HEADERS,
  HTTPC_PARSE_RX_DATA
} httpc_parse_state_t;

typedef struct _httpc_state
{
  struct altcp_pcb* pcb;
  ip_addr_t remote_addr;
  u16_t remote_port;
  int timeout_ticks;
  struct pbuf *request;
  struct pbuf *rx_hdrs;
  u16_t rx_http_version;
  u16_t rx_status;
  altcp_recv_fn recv_fn;
  const httpc_connection_t *conn_settings;
  void* callback_arg;
  u32_t rx_content_len;
  u32_t hdr_content_len;
  httpc_parse_state_t parse_state;
#if HTTPC_DEBUG_REQUEST
  char* server_name;
  char* uri;
#endif
} httpc_state_t;

/** Free http client state and deallocate all resources within */
static err_t
httpc_free_state(httpc_state_t* req)
{
  struct altcp_pcb* tpcb;

  if (req->request != NULL) {
    pbuf_free(req->request);
    req->request = NULL;
  }
  if (req->rx_hdrs != NULL) {
    pbuf_free(req->rx_hdrs);
    req->rx_hdrs = NULL;
  }

  tpcb = req->pcb;
  mem_free(req);
  req = NULL;

  if (tpcb != NULL) {
    err_t r;
    altcp_arg(tpcb, NULL);
    altcp_recv(tpcb, NULL);
    altcp_err(tpcb, NULL);
    altcp_poll(tpcb, NULL, 0);
    altcp_sent(tpcb, NULL);
    r = altcp_close(tpcb);
    if (r != ERR_OK) {
      altcp_abort(tpcb);
      return ERR_ABRT;
    }
  }
  return ERR_OK;
}

/** Close the connection: call finished callback and free the state */
static err_t
httpc_close(httpc_state_t* req, httpc_result_t result, u32_t server_response, err_t err)
{
  if (req != NULL) {
    if (req->conn_settings != NULL) {
      if (req->conn_settings->result_fn != NULL) {
        req->conn_settings->result_fn(req->callback_arg, result, req->rx_content_len, server_response, err);
      }
    }
    return httpc_free_state(req);
  }
  return ERR_OK;
}

/** Parse http header response line 1 */
static err_t
http_parse_response_status(struct pbuf *p, u16_t *http_version, u16_t *http_status, u16_t *http_status_str_offset)
{
  u16_t end1 = pbuf_memfind(p, "\r\n", 2, 0);
  if (end1 != 0xFFFF) {
    /* get parts of first line */
    u16_t space1, space2;
    space1 = pbuf_memfind(p, " ", 1, 0);
    if (space1 != 0xFFFF) {
      if ((pbuf_memcmp(p, 0, "HTTP/", 5) == 0)  && (pbuf_get_at(p, 6) == '.')) {
        char status_num[10];
        size_t status_num_len;
        /* parse http version */
        u16_t version = pbuf_get_at(p, 5) - '0';
        version <<= 8;
        version |= pbuf_get_at(p, 7) - '0';
        *http_version = version;

        /* parse http status number */
        space2 = pbuf_memfind(p, " ", 1, space1 + 1);
        if (space2 != 0xFFFF) {
          *http_status_str_offset = space2 + 1;
          status_num_len = space2 - space1 - 1;
        } else {
          status_num_len = end1 - space1 - 1;
        }
        memset(status_num, 0, sizeof(status_num));
        if (pbuf_copy_partial(p, status_num, (u16_t)status_num_len, space1 + 1) == status_num_len) {
          int status = atoi(status_num);
          if ((status > 0) && (status <= 0xFFFF)) {
            *http_status = (u16_t)status;
            return ERR_OK;
          }
        }
      }
    }
  }
  return ERR_VAL;
}

/** Wait for all headers to be received, return its length and content-length (if available) */
static err_t
http_wait_headers(struct pbuf *p, u32_t *content_length, u16_t *total_header_len)
{
  u16_t end1 = pbuf_memfind(p, "\r\n\r\n", 4, 0);
  if (end1 < (0xFFFF - 2)) {
    /* all headers received */
    /* check if we have a content length (@todo: case insensitive?) */
    u16_t content_len_hdr;
    *content_length = HTTPC_CONTENT_LEN_INVALID;
    *total_header_len = end1 + 4;

    content_len_hdr = pbuf_memfind(p, "Content-Length: ", 16, 0);
    if (content_len_hdr != 0xFFFF) {
      u16_t content_len_line_end = pbuf_memfind(p, "\r\n", 2, content_len_hdr);
      if (content_len_line_end != 0xFFFF) {
        char content_len_num[16];
        u16_t content_len_num_len = (u16_t)(content_len_line_end - content_len_hdr - 16);
        memset(content_len_num, 0, sizeof(content_len_num));
        if (pbuf_copy_partial(p, content_len_num, content_len_num_len, content_len_hdr + 16) == content_len_num_len) {
          int len = atoi(content_len_num);
          if ((len >= 0) && ((u32_t)len < HTTPC_CONTENT_LEN_INVALID)) {
            *content_length = (u32_t)len;
          }
        }
      }
    }
    return ERR_OK;
  }
  return ERR_VAL;
}

/** http client tcp recv callback */
static err_t
httpc_tcp_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t r)
{
  httpc_state_t* req = (httpc_state_t*)arg;
  LWIP_UNUSED_ARG(r);

  if (p == NULL) {
    httpc_result_t result;
    if (req->parse_state != HTTPC_PARSE_RX_DATA) {
      /* did not get RX data yet */
      result = HTTPC_RESULT_ERR_CLOSED;
    } else if ((req->hdr_content_len != HTTPC_CONTENT_LEN_INVALID) &&
      (req->hdr_content_len != req->rx_content_len)) {
      /* header has been received with content length but not all data received */
      result = HTTPC_RESULT_ERR_CONTENT_LEN;
    } else {
      /* receiving data and either all data received or no content length header */
      result = HTTPC_RESULT_OK;
    }
    return httpc_close(req, result, req->rx_status, ERR_OK);
  }
  if (req->parse_state != HTTPC_PARSE_RX_DATA) {
    if (req->rx_hdrs == NULL) {
      req->rx_hdrs = p;
    } else {
      pbuf_cat(req->rx_hdrs, p);
    }
    if (req->parse_state == HTTPC_PARSE_WAIT_FIRST_LINE) {
      u16_t status_str_off;
      err_t err = http_parse_response_status(req->rx_hdrs, &req->rx_http_version, &req->rx_status, &status_str_off);
      if (err == ERR_OK) {
        /* don't care status string */
        req->parse_state = HTTPC_PARSE_WAIT_HEADERS;
      }
    }
    if (req->parse_state == HTTPC_PARSE_WAIT_HEADERS) {
      u16_t total_header_len;
      err_t err = http_wait_headers(req->rx_hdrs, &req->hdr_content_len, &total_header_len);
      if (err == ERR_OK) {
        struct pbuf *q;
        /* full header received, send window update for header bytes and call into client callback */
        altcp_recved(pcb, total_header_len);
        if (req->conn_settings) {
          if (req->conn_settings->headers_done_fn) {
            err = req->conn_settings->headers_done_fn(req, req->callback_arg, req->rx_hdrs, total_header_len, req->hdr_content_len);
            if (err != ERR_OK) {
              return httpc_close(req, HTTPC_RESULT_LOCAL_ABORT, req->rx_status, err);
            }
          }
        }
        /* hide header bytes in pbuf */
        q = pbuf_free_header(req->rx_hdrs, total_header_len);
        p = q;
        req->rx_hdrs = NULL;
        /* go on with data */
        req->parse_state = HTTPC_PARSE_RX_DATA;
      }
    }
  }
  if ((p != NULL) && (req->parse_state == HTTPC_PARSE_RX_DATA)) {
    req->rx_content_len += p->tot_len;
    if (req->recv_fn != NULL) {
      /* directly return here: the connection migth already be aborted from the callback! */
      return req->recv_fn(req->callback_arg, pcb, p, r);
    } else {
      altcp_recved(pcb, p->tot_len);
      pbuf_free(p);
    }
  }
  return ERR_OK;
}

/** http client tcp err callback */
static void
httpc_tcp_err(void *arg, err_t err)
{
  httpc_state_t* req = (httpc_state_t*)arg;
  if (req != NULL) {
    /* pcb has already been deallocated */
    req->pcb = NULL;
    httpc_close(req, HTTPC_RESULT_ERR_CLOSED, 0, err);
  }
}

/** http client tcp poll callback */
static err_t
httpc_tcp_poll(void *arg, struct altcp_pcb *pcb)
{
  /* implement timeout */
  httpc_state_t* req = (httpc_state_t*)arg;
  LWIP_UNUSED_ARG(pcb);
  if (req != NULL) {
    if (req->timeout_ticks) {
      req->timeout_ticks--;
    }
    if (!req->timeout_ticks) {
      return httpc_close(req, HTTPC_RESULT_ERR_TIMEOUT, 0, ERR_OK);
    }
  }
  return ERR_OK;
}

/** http client tcp sent callback */
static err_t
httpc_tcp_sent(void *arg, struct altcp_pcb *pcb, u16_t len)
{
  /* nothing to do here for now */
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(len);
  return ERR_OK;
}

/** http client tcp connected callback */
static err_t
httpc_tcp_connected(void *arg, struct altcp_pcb *pcb, err_t err)
{
  err_t r;
  httpc_state_t* req = (httpc_state_t*)arg;
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(err);

  /* send request; last char is zero termination */
  r = altcp_write(req->pcb, req->request->payload, req->request->len - 1, TCP_WRITE_FLAG_COPY);
  if (r != ERR_OK) {
     /* could not write the single small request -> fail, don't retry */
     return httpc_close(req, HTTPC_RESULT_ERR_MEM, 0, r);
  }
  /* everything written, we can free the request */
  pbuf_free(req->request);
  req->request = NULL;

  altcp_output(req->pcb);
  return ERR_OK;
}

/** Start the http request when the server IP addr is known */
static err_t
httpc_get_internal_addr(httpc_state_t* req, const ip_addr_t *ipaddr)
{
  err_t err;
  LWIP_ASSERT("req != NULL", req != NULL);

  if (&req->remote_addr != ipaddr) {
    /* fill in remote addr if called externally */
    req->remote_addr = *ipaddr;
  }

  err = altcp_connect(req->pcb, &req->remote_addr, req->remote_port, httpc_tcp_connected);
  if (err == ERR_OK) {
    return ERR_OK;
  }
  LWIP_DEBUGF(HTTPC_DEBUG_WARN_STATE, ("tcp_connect failed: %d\n", (int)err));
  return err;
}

#if LWIP_DNS
/** DNS callback
 * If ipaddr is non-NULL, resolving succeeded and the request can be sent, otherwise it failed.
 */
static void
httpc_dns_found(const char* hostname, const ip_addr_t *ipaddr, void *arg)
{
  httpc_state_t* req = (httpc_state_t*)arg;
  err_t err;
  httpc_result_t result;

  LWIP_UNUSED_ARG(hostname);

  if (ipaddr != NULL) {
    err = httpc_get_internal_addr(req, ipaddr);
    if (err == ERR_OK) {
      return;
    }
    result = HTTPC_RESULT_ERR_CONNECT;
  } else {
    LWIP_DEBUGF(HTTPC_DEBUG_WARN_STATE, ("httpc_dns_found: failed to resolve hostname: %s\n",
      hostname));
    result = HTTPC_RESULT_ERR_HOSTNAME;
    err = ERR_ARG;
  }
  httpc_close(req, result, 0, err);
}
#endif /* LWIP_DNS */

/** Start the http request after converting 'server_name' to ip address (DNS or address string) */
static err_t
httpc_get_internal_dns(httpc_state_t* req, const char* server_name)
{
  err_t err;
  LWIP_ASSERT("req != NULL", req != NULL);

#if LWIP_DNS
  err = dns_gethostbyname(server_name, &req->remote_addr, httpc_dns_found, req);
#else
  err = ipaddr_aton(server_name, &req->remote_addr) ? ERR_OK : ERR_ARG;
#endif

  if (err == ERR_OK) {
    /* cached or IP-string */
    err = httpc_get_internal_addr(req, &req->remote_addr);
  } else if (err == ERR_INPROGRESS) {
    return ERR_OK;
  }
  return err;
}

static int
httpc_create_request_string(const httpc_connection_t *settings, const char* server_name, int server_port, const char* uri,
                            int use_host, char *buffer, size_t buffer_size)
{
  if (settings->use_proxy) {
    LWIP_ASSERT("server_name != NULL", server_name != NULL);
    if (server_port != HTTP_DEFAULT_PORT) {
      return snprintf(buffer, buffer_size, HTTPC_REQ_11_PROXY_PORT_FORMAT(server_name, server_port, uri, server_name));
    } else {
      return snprintf(buffer, buffer_size, HTTPC_REQ_11_PROXY_FORMAT(server_name, uri, server_name));
    }
  } else if (use_host) {
    LWIP_ASSERT("server_name != NULL", server_name != NULL);
    return snprintf(buffer, buffer_size, HTTPC_REQ_11_HOST_FORMAT(uri, server_name));
  } else {
    return snprintf(buffer, buffer_size, HTTPC_REQ_11_FORMAT(uri));
  }
}

/** Initialize the connection struct */
static err_t
httpc_init_connection_common(httpc_state_t **connection, const httpc_connection_t *settings, const char* server_name,
                      u16_t server_port, const char* uri, altcp_recv_fn recv_fn, void* callback_arg, int use_host)
{
  size_t alloc_len;
  mem_size_t mem_alloc_len;
  int req_len, req_len2;
  httpc_state_t *req;
#if HTTPC_DEBUG_REQUEST
  size_t server_name_len, uri_len;
#endif

  LWIP_ASSERT("uri != NULL", uri != NULL);

  /* get request len */
  req_len = httpc_create_request_string(settings, server_name, server_port, uri, use_host, NULL, 0);
  if ((req_len < 0) || (req_len > 0xFFFF)) {
    return ERR_VAL;
  }
  /* alloc state and request in one block */
  alloc_len = sizeof(httpc_state_t);
#if HTTPC_DEBUG_REQUEST
  server_name_len = server_name ? strlen(server_name) : 0;
  uri_len = strlen(uri);
  alloc_len += server_name_len + 1 + uri_len + 1;
#endif
  mem_alloc_len = (mem_size_t)alloc_len;
  if ((mem_alloc_len < alloc_len) || (req_len + 1 > 0xFFFF)) {
    return ERR_VAL;
  }

  req = (httpc_state_t*)mem_malloc((mem_size_t)alloc_len);
  if(req == NULL) {
    return ERR_MEM;
  }
  memset(req, 0, sizeof(httpc_state_t));
  req->timeout_ticks = HTTPC_POLL_TIMEOUT;
  req->request = pbuf_alloc(PBUF_RAW, (u16_t)(req_len + 1), PBUF_RAM);
  if (req->request == NULL) {
    httpc_free_state(req);
    return ERR_MEM;
  }
  if (req->request->next != NULL) {
    /* need a pbuf in one piece */
    httpc_free_state(req);
    return ERR_MEM;
  }
  req->hdr_content_len = HTTPC_CONTENT_LEN_INVALID;
#if HTTPC_DEBUG_REQUEST
  req->server_name = (char*)(req + 1);
  if (server_name) {
    memcpy(req->server_name, server_name, server_name_len + 1);
  }
  req->uri = req->server_name + server_name_len + 1;
  memcpy(req->uri, uri, uri_len + 1);
#endif
  req->pcb = altcp_new(settings->altcp_allocator);
  if(req->pcb == NULL) {
    httpc_free_state(req);
    return ERR_MEM;
  }
  req->remote_port = settings->use_proxy ? settings->proxy_port : server_port;
  altcp_arg(req->pcb, req);
  altcp_recv(req->pcb, httpc_tcp_recv);
  altcp_err(req->pcb, httpc_tcp_err);
  altcp_poll(req->pcb, httpc_tcp_poll, HTTPC_POLL_INTERVAL);
  altcp_sent(req->pcb, httpc_tcp_sent);

  /* set up request buffer */
  req_len2 = httpc_create_request_string(settings, server_name, server_port, uri, use_host,
    (char *)req->request->payload, req_len + 1);
  if (req_len2 != req_len) {
    httpc_free_state(req);
    return ERR_VAL;
  }

  req->recv_fn = recv_fn;
  req->conn_settings = settings;
  req->callback_arg = callback_arg;

  *connection = req;
  return ERR_OK;
}

/**
 * Initialize the connection struct
 */
static err_t
httpc_init_connection(httpc_state_t **connection, const httpc_connection_t *settings, const char* server_name,
                      u16_t server_port, const char* uri, altcp_recv_fn recv_fn, void* callback_arg)
{
  return httpc_init_connection_common(connection, settings, server_name, server_port, uri, recv_fn, callback_arg, 1);
}


/**
 * Initialize the connection struct (from IP address)
 */
static err_t
httpc_init_connection_addr(httpc_state_t **connection, const httpc_connection_t *settings,
                           const ip_addr_t* server_addr, u16_t server_port, const char* uri,
                           altcp_recv_fn recv_fn, void* callback_arg)
{
  char *server_addr_str = ipaddr_ntoa(server_addr);
  if (server_addr_str == NULL) {
    return ERR_VAL;
  }
  return httpc_init_connection_common(connection, settings, server_addr_str, server_port, uri,
    recv_fn, callback_arg, 1);
}

/**
 * @ingroup httpc 
 * HTTP client API: get a file by passing server IP address
 *
 * @param server_addr IP address of the server to connect
 * @param port tcp port of the server
 * @param uri uri to get from the server, remember leading "/"!
 * @param settings connection settings (callbacks, proxy, etc.)
 * @param recv_fn the http body (not the headers) are passed to this callback
 * @param callback_arg argument passed to all the callbacks
 * @param connection retreives the connection handle (to match in callbacks)
 * @return ERR_OK if starting the request succeeds (callback_fn will be called later)
 *         or an error code
 */
err_t
httpc_get_file(const ip_addr_t* server_addr, u16_t port, const char* uri, const httpc_connection_t *settings,
               altcp_recv_fn recv_fn, void* callback_arg, httpc_state_t **connection)
{
  err_t err;
  httpc_state_t* req;

  LWIP_ERROR("invalid parameters", (server_addr != NULL) && (uri != NULL) && (recv_fn != NULL), return ERR_ARG;);

  err = httpc_init_connection_addr(&req, settings, server_addr, port,
    uri, recv_fn, callback_arg);
  if (err != ERR_OK) {
    return err;
  }

  if (settings->use_proxy) {
    err = httpc_get_internal_addr(req, &settings->proxy_addr);
  } else {
    err = httpc_get_internal_addr(req, server_addr);
  }
  if(err != ERR_OK) {
    httpc_free_state(req);
    return err;
  }

  if (connection != NULL) {
    *connection = req;
  }
  return ERR_OK;
}

/**
 * @ingroup httpc 
 * HTTP client API: get a file by passing server name as string (DNS name or IP address string)
 *
 * @param server_name server name as string (DNS name or IP address string)
 * @param port tcp port of the server
 * @param uri uri to get from the server, remember leading "/"!
 * @param settings connection settings (callbacks, proxy, etc.)
 * @param recv_fn the http body (not the headers) are passed to this callback
 * @param callback_arg argument passed to all the callbacks
 * @param connection retreives the connection handle (to match in callbacks)
 * @return ERR_OK if starting the request succeeds (callback_fn will be called later)
 *         or an error code
 */
err_t
httpc_get_file_dns(const char* server_name, u16_t port, const char* uri, const httpc_connection_t *settings,
                   altcp_recv_fn recv_fn, void* callback_arg, httpc_state_t **connection)
{
  err_t err;
  httpc_state_t* req;

  LWIP_ERROR("invalid parameters", (server_name != NULL) && (uri != NULL) && (recv_fn != NULL), return ERR_ARG;);

  err = httpc_init_connection(&req, settings, server_name, port, uri, recv_fn, callback_arg);
  if (err != ERR_OK) {
    return err;
  }

  if (settings->use_proxy) {
    err = httpc_get_internal_addr(req, &settings->proxy_addr);
  } else {
    err = httpc_get_internal_dns(req, server_name);
  }
  if(err != ERR_OK) {
    httpc_free_state(req);
    return err;
  }

  if (connection != NULL) {
    *connection = req;
  }
  return ERR_OK;
}

#if LWIP_HTTPC_HAVE_FILE_IO
/* Implementation to disk via fopen/fwrite/fclose follows */

typedef struct _httpc_filestate
{
  const char* local_file_name;
  FILE *file;
  httpc_connection_t settings;
  const httpc_connection_t *client_settings;
  void *callback_arg;
} httpc_filestate_t;

static void httpc_fs_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len,
  u32_t srv_res, err_t err);

/** Initalize http client state for download to file system */
static err_t
httpc_fs_init(httpc_filestate_t **filestate_out, const char* local_file_name,
              const httpc_connection_t *settings, void* callback_arg)
{
  httpc_filestate_t *filestate;
  size_t file_len, alloc_len;
  FILE *f;

  file_len = strlen(local_file_name);
  alloc_len = sizeof(httpc_filestate_t) + file_len + 1;

  filestate = (httpc_filestate_t *)mem_malloc((mem_size_t)alloc_len);
  if (filestate == NULL) {
    return ERR_MEM;
  }
  memset(filestate, 0, sizeof(httpc_filestate_t));
  filestate->local_file_name = (const char *)(filestate + 1);
  memcpy((char *)(filestate + 1), local_file_name, file_len + 1);
  filestate->file = NULL;
  filestate->client_settings = settings;
  filestate->callback_arg = callback_arg;
  /* copy client settings but override result callback */
  memcpy(&filestate->settings, settings, sizeof(httpc_connection_t));
  filestate->settings.result_fn = httpc_fs_result;

  f = fopen(local_file_name, "wb");
  if(f == NULL) {
    /* could not open file */
    mem_free(filestate);
    return ERR_VAL;
  }
  filestate->file = f;
  *filestate_out = filestate;
  return ERR_OK;
}

/** Free http client state for download to file system */
static void
httpc_fs_free(httpc_filestate_t *filestate)
{
  if (filestate != NULL) {
    if (filestate->file != NULL) {
      fclose(filestate->file);
      filestate->file = NULL;
    }
    mem_free(filestate);
  }
}

/** Connection closed (success or error) */
static void
httpc_fs_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len,
                u32_t srv_res, err_t err)
{
  httpc_filestate_t *filestate = (httpc_filestate_t *)arg;
  if (filestate != NULL) {
    if (filestate->client_settings->result_fn != NULL) {
      filestate->client_settings->result_fn(filestate->callback_arg, httpc_result, rx_content_len,
        srv_res, err);
    }
    httpc_fs_free(filestate);
  }
}

/** tcp recv callback */
static err_t
httpc_fs_tcp_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
  httpc_filestate_t *filestate = (httpc_filestate_t*)arg;
  struct pbuf* q;
  LWIP_UNUSED_ARG(err);

  LWIP_ASSERT("p != NULL", p != NULL);

  for (q = p; q != NULL; q = q->next) {
    fwrite(q->payload, 1, q->len, filestate->file);
  }
  altcp_recved(pcb, p->tot_len);
  pbuf_free(p);
  return ERR_OK;
}

/**
 * @ingroup httpc 
 * HTTP client API: get a file to disk by passing server IP address
 *
 * @param server_addr IP address of the server to connect
 * @param port tcp port of the server
 * @param uri uri to get from the server, remember leading "/"!
 * @param settings connection settings (callbacks, proxy, etc.)
 * @param callback_arg argument passed to all the callbacks
 * @param connection retreives the connection handle (to match in callbacks)
 * @return ERR_OK if starting the request succeeds (callback_fn will be called later)
 *         or an error code
 */
err_t
httpc_get_file_to_disk(const ip_addr_t* server_addr, u16_t port, const char* uri, const httpc_connection_t *settings,
                       void* callback_arg, const char* local_file_name, httpc_state_t **connection)
{
  err_t err;
  httpc_state_t* req;
  httpc_filestate_t *filestate;

  LWIP_ERROR("invalid parameters", (server_addr != NULL) && (uri != NULL) && (local_file_name != NULL), return ERR_ARG;);

  err = httpc_fs_init(&filestate, local_file_name, settings, callback_arg);
  if (err != ERR_OK) {
    return err;
  }

  err = httpc_init_connection_addr(&req, &filestate->settings, server_addr, port,
    uri, httpc_fs_tcp_recv, filestate);
  if (err != ERR_OK) {
    httpc_fs_free(filestate);
    return err;
  }

  if (settings->use_proxy) {
    err = httpc_get_internal_addr(req, &settings->proxy_addr);
  } else {
    err = httpc_get_internal_addr(req, server_addr);
  }
  if(err != ERR_OK) {
    httpc_fs_free(filestate);
    httpc_free_state(req);
    return err;
  }

  if (connection != NULL) {
    *connection = req;
  }
  return ERR_OK;
}

/**
 * @ingroup httpc 
 * HTTP client API: get a file to disk by passing server name as string (DNS name or IP address string)
 *
 * @param server_name server name as string (DNS name or IP address string)
 * @param port tcp port of the server
 * @param uri uri to get from the server, remember leading "/"!
 * @param settings connection settings (callbacks, proxy, etc.)
 * @param callback_arg argument passed to all the callbacks
 * @param connection retreives the connection handle (to match in callbacks)
 * @return ERR_OK if starting the request succeeds (callback_fn will be called later)
 *         or an error code
 */
err_t
httpc_get_file_dns_to_disk(const char* server_name, u16_t port, const char* uri, const httpc_connection_t *settings,
                           void* callback_arg, const char* local_file_name, httpc_state_t **connection)
{
  err_t err;
  httpc_state_t* req;
  httpc_filestate_t *filestate;

  LWIP_ERROR("invalid parameters", (server_name != NULL) && (uri != NULL) && (local_file_name != NULL), return ERR_ARG;);

  err = httpc_fs_init(&filestate, local_file_name, settings, callback_arg);
  if (err != ERR_OK) {
    return err;
  }

  err = httpc_init_connection(&req, &filestate->settings, server_name, port,
    uri, httpc_fs_tcp_recv, filestate);
  if (err != ERR_OK) {
    httpc_fs_free(filestate);
    return err;
  }

  if (settings->use_proxy) {
    err = httpc_get_internal_addr(req, &settings->proxy_addr);
  } else {
    err = httpc_get_internal_dns(req, server_name);
  }
  if(err != ERR_OK) {
    httpc_fs_free(filestate);
    httpc_free_state(req);
    return err;
  }

  if (connection != NULL) {
    *connection = req;
  }
  return ERR_OK;
}
#endif /* LWIP_HTTPC_HAVE_FILE_IO */

#endif /* LWIP_TCP && LWIP_CALLBACK_API */
