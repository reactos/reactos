/**
 *
 * @file tftp.c
 *
 * @author   Logan Gunthorpe <logang@deltatee.com>
 *           Dirk Ziegelmeier <dziegel@gmx.de>
 *
 * @brief    Trivial File Transfer Protocol (RFC 1350)
 *
 * Copyright (c) Deltatee Enterprises Ltd. 2013
 * All rights reserved.
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification,are permitted provided that the following conditions are met:
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
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Logan Gunthorpe <logang@deltatee.com>
 *         Dirk Ziegelmeier <dziegel@gmx.de>
 *
 */

/**
 * @defgroup tftp TFTP client/server
 * @ingroup apps
 *
 * This is simple TFTP client/server for the lwIP raw API.
 * You need to increase MEMP_NUM_SYS_TIMEOUT by one if you use TFTP!
 */

#include "lwip/apps/tftp_client.h"
#include "lwip/apps/tftp_server.h"

#if LWIP_UDP

#include "lwip/udp.h"
#include "lwip/timeouts.h"
#include "lwip/debug.h"

#define TFTP_MAX_PAYLOAD_SIZE 512
#define TFTP_HEADER_LENGTH    4

#define TFTP_RRQ   1
#define TFTP_WRQ   2
#define TFTP_DATA  3
#define TFTP_ACK   4
#define TFTP_ERROR 5

enum tftp_error {
  TFTP_ERROR_FILE_NOT_FOUND    = 1,
  TFTP_ERROR_ACCESS_VIOLATION  = 2,
  TFTP_ERROR_DISK_FULL         = 3,
  TFTP_ERROR_ILLEGAL_OPERATION = 4,
  TFTP_ERROR_UNKNOWN_TRFR_ID   = 5,
  TFTP_ERROR_FILE_EXISTS       = 6,
  TFTP_ERROR_NO_SUCH_USER      = 7
};

#include <string.h>

struct tftp_state {
  const struct tftp_context *ctx;
  void *handle;
  struct pbuf *last_data;
  struct udp_pcb *upcb;
  ip_addr_t addr;
  u16_t port;
  int timer;
  int last_pkt;
  u16_t blknum;
  u8_t retries;
  u8_t mode_write;
  u8_t tftp_mode;
};

static struct tftp_state tftp_state;

static void tftp_tmr(void *arg);

static void
close_handle(void)
{
  tftp_state.port = 0;
  ip_addr_set_any(0, &tftp_state.addr);

  if (tftp_state.last_data != NULL) {
    pbuf_free(tftp_state.last_data);
    tftp_state.last_data = NULL;
  }

  sys_untimeout(tftp_tmr, NULL);

  if (tftp_state.handle) {
    tftp_state.ctx->close(tftp_state.handle);
    tftp_state.handle = NULL;
    LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("tftp: closing\n"));
  }
}

static struct pbuf*
init_packet(u16_t opcode, u16_t extra, size_t size)
{
  struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, (u16_t)(TFTP_HEADER_LENGTH + size), PBUF_RAM);
  u16_t* payload;

  if (p != NULL) {
    payload = (u16_t*) p->payload;
    payload[0] = PP_HTONS(opcode);
    payload[1] = lwip_htons(extra);
  }

  return p;
}

static err_t
send_request(const ip_addr_t *addr, u16_t port, u16_t opcode, const char* fname, const char* mode)
{
  size_t fname_length = strlen(fname)+1;
  size_t mode_length = strlen(mode)+1;
  struct pbuf* p = init_packet(opcode, 0, fname_length + mode_length - 2);
  char* payload;
  err_t ret;

  if (p == NULL) {
    return ERR_MEM;
  }

  payload = (char*) p->payload;
  MEMCPY(payload+2,              fname, fname_length);
  MEMCPY(payload+2+fname_length, mode,  mode_length);

  ret = udp_sendto(tftp_state.upcb, p, addr, port);
  pbuf_free(p);
  return ret;
}

static err_t
send_error(const ip_addr_t *addr, u16_t port, enum tftp_error code, const char *str)
{
  int str_length = strlen(str);
  struct pbuf *p;
  u16_t *payload;
  err_t ret;

  p = init_packet(TFTP_ERROR, code, str_length + 1);
  if (p == NULL) {
    return ERR_MEM;
  }

  payload = (u16_t *) p->payload;
  MEMCPY(&payload[2], str, str_length + 1);

  ret = udp_sendto(tftp_state.upcb, p, addr, port);
  pbuf_free(p);
  return ret;
}

static err_t
send_ack(const ip_addr_t *addr, u16_t port, u16_t blknum)
{
  struct pbuf *p;
  err_t ret;

  p = init_packet(TFTP_ACK, blknum, 0);
  if (p == NULL) {
    return ERR_MEM;
  }

  ret = udp_sendto(tftp_state.upcb, p, addr, port);
  pbuf_free(p);
  return ret;
}

static err_t
resend_data(const ip_addr_t *addr, u16_t port)
{
  err_t ret;
  struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, tftp_state.last_data->len, PBUF_RAM);
  if (p == NULL) {
    return ERR_MEM;
  }

  ret = pbuf_copy(p, tftp_state.last_data);
  if (ret != ERR_OK) {
    pbuf_free(p);
    return ret;
  }

  ret = udp_sendto(tftp_state.upcb, p, addr, port);
  pbuf_free(p);
  return ret;
}

static void
send_data(const ip_addr_t *addr, u16_t port)
{
  u16_t *payload;
  int ret;

  if (tftp_state.last_data != NULL) {
    pbuf_free(tftp_state.last_data);
  }

  tftp_state.last_data = init_packet(TFTP_DATA, tftp_state.blknum, TFTP_MAX_PAYLOAD_SIZE);
  if (tftp_state.last_data == NULL) {
    return;
  }

  payload = (u16_t *) tftp_state.last_data->payload;

  ret = tftp_state.ctx->read(tftp_state.handle, &payload[2], TFTP_MAX_PAYLOAD_SIZE);
  if (ret < 0) {
    send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "Error occurred while reading the file.");
    close_handle();
    return;
  }

  pbuf_realloc(tftp_state.last_data, (u16_t)(TFTP_HEADER_LENGTH + ret));
  resend_data(addr, port);
}

static void
tftp_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  u16_t *sbuf = (u16_t *) p->payload;
  int opcode;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(upcb);

  if (((tftp_state.port != 0) && (port != tftp_state.port)) ||
      (!ip_addr_isany_val(tftp_state.addr) && !ip_addr_eq(&tftp_state.addr, addr))) {
    send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "Only one connection at a time is supported");
    pbuf_free(p);
    return;
  }

  opcode = sbuf[0];

  tftp_state.last_pkt = tftp_state.timer;
  tftp_state.retries = 0;

  switch (opcode) {
    case PP_HTONS(TFTP_RRQ): /* fall through */
    case PP_HTONS(TFTP_WRQ): {
      const char tftp_null = 0;
      char filename[TFTP_MAX_FILENAME_LEN + 1];
      char mode[TFTP_MAX_MODE_LEN + 1];
      u16_t filename_end_offset;
      u16_t mode_end_offset;

      if (tftp_state.handle != NULL) {
        send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "Only one connection at a time is supported");
        break;
      }

      if ((tftp_state.tftp_mode & LWIP_TFTP_MODE_SERVER) == 0) {
        send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "TFTP server not enabled");
        break;
      }

      sys_timeout(TFTP_TIMER_MSECS, tftp_tmr, NULL);

      /* find \0 in pbuf -> end of filename string */
      filename_end_offset = pbuf_memfind(p, &tftp_null, sizeof(tftp_null), 2);
      if ((u16_t)(filename_end_offset - 1) > sizeof(filename)) {
        send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "Filename too long/not NULL terminated");
        break;
      }
      pbuf_copy_partial(p, filename, filename_end_offset - 1, 2);

      /* find \0 in pbuf -> end of mode string */
      mode_end_offset = pbuf_memfind(p, &tftp_null, sizeof(tftp_null), filename_end_offset + 1);
      if ((u16_t)(mode_end_offset - filename_end_offset) > sizeof(mode)) {
        send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "Mode too long/not NULL terminated");
        break;
      }
      pbuf_copy_partial(p, mode, mode_end_offset - filename_end_offset, filename_end_offset + 1);

      tftp_state.handle = tftp_state.ctx->open(filename, mode, opcode == PP_HTONS(TFTP_WRQ));
      tftp_state.blknum = 1;

      if (!tftp_state.handle) {
        send_error(addr, port, TFTP_ERROR_FILE_NOT_FOUND, "Unable to open requested file.");
        break;
      }

      LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("tftp: %s request from ", (opcode == PP_HTONS(TFTP_WRQ)) ? "write" : "read"));
      ip_addr_debug_print(TFTP_DEBUG | LWIP_DBG_STATE, addr);
      LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, (" for '%s' mode '%s'\n", filename, mode));

      ip_addr_copy(tftp_state.addr, *addr);
      tftp_state.port = port;

      if (opcode == PP_HTONS(TFTP_WRQ)) {
        tftp_state.mode_write = 1;
        send_ack(addr, port, 0);
      } else {
        tftp_state.mode_write = 0;
        send_data(addr, port);
      }

      break;
    }

    case PP_HTONS(TFTP_DATA): {
      int ret;
      u16_t blknum;

      if (tftp_state.handle == NULL) {
        send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "No connection");
        break;
      }

      if (tftp_state.mode_write != 1) {
        send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "Not a write connection");
        break;
      }

      blknum = lwip_ntohs(sbuf[1]);
      if (blknum == tftp_state.blknum) {
        pbuf_remove_header(p, TFTP_HEADER_LENGTH);

        ret = tftp_state.ctx->write(tftp_state.handle, p);
        if (ret < 0) {
          send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "error writing file");
          close_handle();
        } else {
          send_ack(addr, port, blknum);
        }

        if (p->tot_len < TFTP_MAX_PAYLOAD_SIZE) {
          close_handle();
        } else {
          tftp_state.blknum++;
        }
      } else if ((u16_t)(blknum + 1) == tftp_state.blknum) {
        /* retransmit of previous block, ack again (casting to u16_t to care for overflow) */
        send_ack(addr, port, blknum);
      } else {
        send_error(addr, port, TFTP_ERROR_UNKNOWN_TRFR_ID, "Wrong block number");
      }
      break;
    }

    case PP_HTONS(TFTP_ACK): {
      u16_t blknum;
      int lastpkt;

      if (tftp_state.handle == NULL) {
        send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "No connection");
        break;
      }

      if (tftp_state.mode_write != 0) {
        send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "Not a read connection");
        break;
      }

      blknum = lwip_ntohs(sbuf[1]);
      if (blknum != tftp_state.blknum) {
        send_error(addr, port, TFTP_ERROR_UNKNOWN_TRFR_ID, "Wrong block number");
        break;
      }

      lastpkt = 0;

      if (tftp_state.last_data != NULL) {
        lastpkt = tftp_state.last_data->tot_len != (TFTP_MAX_PAYLOAD_SIZE + TFTP_HEADER_LENGTH);
      }

      if (!lastpkt) {
        tftp_state.blknum++;
        send_data(addr, port);
      } else {
        close_handle();
      }

      break;
    }
    case PP_HTONS(TFTP_ERROR):
      if (tftp_state.handle != NULL) {
        pbuf_remove_header(p, TFTP_HEADER_LENGTH);
        tftp_state.ctx->error(tftp_state.handle, sbuf[1], (const char*)p->payload, p->len);
        close_handle();
      }
      break;
    default:
      send_error(addr, port, TFTP_ERROR_ILLEGAL_OPERATION, "Unknown operation");
      break;
  }

  pbuf_free(p);
}

static void
tftp_tmr(void *arg)
{
  LWIP_UNUSED_ARG(arg);

  tftp_state.timer++;

  if (tftp_state.handle == NULL) {
    return;
  }

  sys_timeout(TFTP_TIMER_MSECS, tftp_tmr, NULL);

  if ((tftp_state.timer - tftp_state.last_pkt) > (TFTP_TIMEOUT_MSECS / TFTP_TIMER_MSECS)) {
    if ((tftp_state.last_data != NULL) && (tftp_state.retries < TFTP_MAX_RETRIES)) {
      LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("tftp: timeout, retrying\n"));
      resend_data(&tftp_state.addr, tftp_state.port);
      tftp_state.retries++;
    } else {
      LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("tftp: timeout\n"));
      close_handle();
    }
  }
}

/**
 * Initialize TFTP client/server.
 * @param mode TFTP mode (client/server)
 * @param ctx TFTP callback struct
 */
err_t
tftp_init_common(u8_t mode, const struct tftp_context *ctx)
{
  err_t ret;

  /* LWIP_ASSERT_CORE_LOCKED(); is checked by udp_new() */
  struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
  if (pcb == NULL) {
    return ERR_MEM;
  }

  ret = udp_bind(pcb, IP_ANY_TYPE, TFTP_PORT);
  if (ret != ERR_OK) {
    udp_remove(pcb);
    return ret;
  }

  tftp_state.handle    = NULL;
  tftp_state.port      = 0;
  tftp_state.ctx       = ctx;
  tftp_state.timer     = 0;
  tftp_state.last_data = NULL;
  tftp_state.upcb      = pcb;
  tftp_state.tftp_mode = mode;

  udp_recv(pcb, tftp_recv, NULL);

  return ERR_OK;
}

/** @ingroup tftp
 * Initialize TFTP server.
 * @param ctx TFTP callback struct
 */
err_t
tftp_init_server(const struct tftp_context *ctx)
{
  return tftp_init_common(LWIP_TFTP_MODE_SERVER, ctx);
}

/** @ingroup tftp
 * Initialize TFTP client.
 * @param ctx TFTP callback struct
 */
err_t
tftp_init_client(const struct tftp_context *ctx)
{
  return tftp_init_common(LWIP_TFTP_MODE_CLIENT, ctx);
}

/** @ingroup tftp
 * Deinitialize ("turn off") TFTP client/server.
 */
void tftp_cleanup(void)
{
  LWIP_ASSERT("Cleanup called on non-initialized TFTP", tftp_state.upcb != NULL);
  udp_remove(tftp_state.upcb);
  close_handle();
  memset(&tftp_state, 0, sizeof(tftp_state));
}

static const char *
mode_to_string(enum tftp_transfer_mode mode)
{
  if (mode == TFTP_MODE_OCTET) {
    return "octet";
  }
  if (mode == TFTP_MODE_NETASCII) {
    return "netascii";
  }
  if (mode == TFTP_MODE_BINARY) {
    return "binary";
  }
  return NULL;
}

err_t
tftp_get(void* handle, const ip_addr_t *addr, u16_t port, const char* fname, enum tftp_transfer_mode mode)
{
  LWIP_ERROR("TFTP client is not enabled (tftp_init)", (tftp_state.tftp_mode & LWIP_TFTP_MODE_CLIENT) != 0, return ERR_VAL);
  LWIP_ERROR("tftp_get: invalid file name", fname != NULL, return ERR_VAL);
  LWIP_ERROR("tftp_get: invalid mode", mode <= TFTP_MODE_BINARY, return ERR_VAL);

  tftp_state.handle = handle;
  tftp_state.blknum = 1;
  tftp_state.mode_write = 1; /* We want to receive data */
  return send_request(addr, port, TFTP_RRQ, fname, mode_to_string(mode));
}

err_t
tftp_put(void* handle, const ip_addr_t *addr, u16_t port, const char* fname, enum tftp_transfer_mode mode)
{
  LWIP_ERROR("TFTP client is not enabled (tftp_init)", (tftp_state.tftp_mode & LWIP_TFTP_MODE_CLIENT) != 0, return ERR_VAL);
  LWIP_ERROR("tftp_put: invalid file name", fname != NULL, return ERR_VAL);
  LWIP_ERROR("tftp_put: invalid mode", mode <= TFTP_MODE_BINARY, return ERR_VAL);

  tftp_state.handle = handle;
  tftp_state.blknum = 1;
  tftp_state.mode_write = 0; /* We want to send data */
  return send_request(addr, port, TFTP_WRQ, fname, mode_to_string(mode));
}

#endif /* LWIP_UDP */
