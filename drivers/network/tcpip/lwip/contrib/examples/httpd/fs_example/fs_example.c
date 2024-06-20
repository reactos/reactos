/**
 * @file
 * HTTPD custom file system example
 *
 * This file demonstrates how to add support for an external file system to httpd.
 * It provides access to the specified root directory and uses stdio.h file functions
 * to read files.
 *
 * ATTENTION: This implementation is *not* secure: no checks are added to ensure
 * files are only read below the specified root directory!
 */
 
 /*
 * Copyright (c) 2017 Simon Goldschmidt
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
 *
 */

#include "lwip/opt.h"
#include "fs_example.h"

#include "lwip/apps/fs.h"
#include "lwip/def.h"
#include "lwip/mem.h"

#include <stdio.h>
#include <string.h>

/** define LWIP_HTTPD_EXAMPLE_CUSTOMFILES to 1 to enable this file system */
#ifndef LWIP_HTTPD_EXAMPLE_CUSTOMFILES
#define LWIP_HTTPD_EXAMPLE_CUSTOMFILES 0
#endif

/** define LWIP_HTTPD_EXAMPLE_CUSTOMFILES_DELAYED to 1 to delay open and read
 * as if e.g. reading from external SPI flash */
#ifndef LWIP_HTTPD_EXAMPLE_CUSTOMFILES_DELAYED
#define LWIP_HTTPD_EXAMPLE_CUSTOMFILES_DELAYED 1
#endif

/** define LWIP_HTTPD_EXAMPLE_CUSTOMFILES_LIMIT_READ to the number of bytes
 * to read to emulate limited transfer buffers and don't read whole files in
 * one chunk.
 * WARNING: lowering this slows down the connection!
 */
#ifndef LWIP_HTTPD_EXAMPLE_CUSTOMFILES_LIMIT_READ
#define LWIP_HTTPD_EXAMPLE_CUSTOMFILES_LIMIT_READ 0
#endif

#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES

#if !LWIP_HTTPD_CUSTOM_FILES
#error This needs LWIP_HTTPD_CUSTOM_FILES
#endif
#if !LWIP_HTTPD_DYNAMIC_HEADERS
#error This needs LWIP_HTTPD_DYNAMIC_HEADERS
#endif
#if !LWIP_HTTPD_DYNAMIC_FILE_READ
#error This wants to demonstrate read-after-open, so LWIP_HTTPD_DYNAMIC_FILE_READ is required!
#endif
#if !LWIP_HTTPD_FS_ASYNC_READ
#error This needs LWIP_HTTPD_FS_ASYNC_READ
#endif
#if !LWIP_HTTPD_FILE_EXTENSION
#error This needs LWIP_HTTPD_FILE_EXTENSION
#endif

#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES_DELAYED
#include "lwip/tcpip.h"
#endif

struct fs_custom_data {
  FILE *f;
#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES_DELAYED
  int delay_read;
  fs_wait_cb callback_fn;
  void *callback_arg;
#endif
};

const char* fs_ex_root_dir;

void
fs_ex_init(const char *httpd_root_dir)
{
  fs_ex_root_dir = strdup(httpd_root_dir);
}

#if LWIP_HTTPD_CUSTOM_FILES
int
fs_open_custom(struct fs_file *file, const char *name)
{
  char full_filename[256];
  FILE *f;

  snprintf(full_filename, 255, "%s%s", fs_ex_root_dir, name);
  full_filename[255] = 0;

  f = fopen(full_filename, "rb");
  if (f != NULL) {
    if (!fseek(f, 0, SEEK_END)) {
      int len = (int)ftell(f);
      if(!fseek(f, 0, SEEK_SET)) {
        struct fs_custom_data *data = (struct fs_custom_data *)mem_malloc(sizeof(struct fs_custom_data));
        LWIP_ASSERT("out of memory?", data != NULL);
        memset(file, 0, sizeof(struct fs_file));
#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES_DELAYED
        file->len = 0; /* read size delayed */
        data->delay_read = 3;
        LWIP_UNUSED_ARG(len);
#else
        file->len = len;
#endif
        file->flags = FS_FILE_FLAGS_HEADER_PERSISTENT;
        data->f = f;
        file->pextension = data;
        return 1;
      }
    }
    fclose(f);
  }
  return 0;
}

void
fs_close_custom(struct fs_file *file)
{
  if (file && file->pextension) {
    struct fs_custom_data *data = (struct fs_custom_data *)file->pextension;
    if (data->f != NULL) {
      fclose(data->f);
      data->f = NULL;
    }
    mem_free(data);
  }
}

#if LWIP_HTTPD_FS_ASYNC_READ
u8_t
fs_canread_custom(struct fs_file *file)
{
  /* This function is only necessary for asynchronous I/O:
     If reading would block, return 0 and implement fs_wait_read_custom() to call the
     supplied callback if reading works. */
#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES_DELAYED
  struct fs_custom_data *data;
  LWIP_ASSERT("file != NULL", file != NULL);
  data = (struct fs_custom_data *)file->pextension;
  if (data == NULL) {
    /* file transfer has been completed already */
    LWIP_ASSERT("transfer complete", file->index == file->len);
    return 1;
  }
  LWIP_ASSERT("data != NULL", data != NULL);
  /* This just simulates a simple delay. This delay would normally come e.g. from SPI transfer */
  if (data->delay_read == 3) {
    /* delayed file size mode */
    data->delay_read = 1;
    LWIP_ASSERT("", file->len == 0);
    if (!fseek(data->f, 0, SEEK_END)) {
      int len = (int)ftell(data->f);
      if(!fseek(data->f, 0, SEEK_SET)) {
        file->len = len; /* read size delayed */
        data->delay_read = 1;
        return 0;
      }
    }
    /* if we come here, something is wrong with the file */
    LWIP_ASSERT("file error", 0);
  }
  if (data->delay_read == 1) {
    /* tell read function to delay further */
  }
#endif
  LWIP_UNUSED_ARG(file);
  return 1;
}

#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES_DELAYED
static void
fs_example_read_cb(void *arg)
{
  struct fs_custom_data *data = (struct fs_custom_data *)arg;
  fs_wait_cb callback_fn = data->callback_fn;
  void *callback_arg = data->callback_arg;
  data->callback_fn = NULL;
  data->callback_arg = NULL;

  LWIP_ASSERT("no callback_fn", callback_fn != NULL);

  callback_fn(callback_arg);
}
#endif

u8_t
fs_wait_read_custom(struct fs_file *file, fs_wait_cb callback_fn, void *callback_arg)
{
#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES_DELAYED
  err_t err;
  struct fs_custom_data *data = (struct fs_custom_data *)file->pextension;
  LWIP_ASSERT("data not set", data != NULL);
  data->callback_fn = callback_fn;
  data->callback_arg = callback_arg;
  err = tcpip_try_callback(fs_example_read_cb, data);
  LWIP_ASSERT("out of queue elements?", err == ERR_OK);
  LWIP_UNUSED_ARG(err);
#else
  LWIP_ASSERT("not implemented in this example configuration", 0);
#endif
  LWIP_UNUSED_ARG(file);
  LWIP_UNUSED_ARG(callback_fn);
  LWIP_UNUSED_ARG(callback_arg);
  /* Return
     - 0 if ready to read (at least one byte)
     - 1 if reading should be delayed (call 'tcpip_callback(callback_fn, callback_arg)' when ready) */
  return 1;
}

int
fs_read_async_custom(struct fs_file *file, char *buffer, int count, fs_wait_cb callback_fn, void *callback_arg)
{
  struct fs_custom_data *data = (struct fs_custom_data *)file->pextension;
  FILE *f;
  int len;
  int read_count = count;
  LWIP_ASSERT("data not set", data != NULL);

#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES_DELAYED
  /* This just simulates a delay. This delay would normally come e.g. from SPI transfer */
  LWIP_ASSERT("invalid state", data->delay_read >= 0 && data->delay_read <= 2);
  if (data->delay_read == 2) {
    /* no delay next time */
    data->delay_read = 0;
    return FS_READ_DELAYED;
  } else if (data->delay_read == 1) {
    err_t err;
    /* execute requested delay */
    data->delay_read = 2;
    LWIP_ASSERT("duplicate callback request", data->callback_fn == NULL);
    data->callback_fn = callback_fn;
    data->callback_arg = callback_arg;
    err = tcpip_try_callback(fs_example_read_cb, data);
    LWIP_ASSERT("out of queue elements?", err == ERR_OK);
    LWIP_UNUSED_ARG(err);
    return FS_READ_DELAYED;
  }
  /* execute this read but delay the next one */
  data->delay_read = 1;
#endif

#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES_LIMIT_READ
  read_count = LWIP_MIN(read_count, LWIP_HTTPD_EXAMPLE_CUSTOMFILES_LIMIT_READ);
#endif

  f = data->f;
  len = (int)fread(buffer, 1, read_count, f);

  LWIP_UNUSED_ARG(callback_fn);
  LWIP_UNUSED_ARG(callback_arg);

  file->index += len;

  /* Return
     - FS_READ_EOF if all bytes have been read
     - FS_READ_DELAYED if reading is delayed (call 'tcpip_callback(callback_fn, callback_arg)' when done) */
  if (len == 0) {
    /* all bytes read already */
    return FS_READ_EOF;
  }
  return len;
}

#else /* LWIP_HTTPD_FS_ASYNC_READ */
int
fs_read_custom(struct fs_file *file, char *buffer, int count)
{
  struct fs_custom_data *data = (struct fs_custom_data *)file->pextension;
  FILE *f;
  int len;
  int read_count = count;
  LWIP_ASSERT("data not set", data != NULL);

#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES_LIMIT_READ
  read_count = LWIP_MIN(read_count, LWIP_HTTPD_EXAMPLE_CUSTOMFILES_LIMIT_READ);
#endif

  f = data->f;
  len = (int)fread(buffer, 1, read_count, f);

  file->index += len;

  /* Return FS_READ_EOF if all bytes have been read */
  return len;
}

#endif /* LWIP_HTTPD_FS_ASYNC_READ */
#endif /* LWIP_HTTPD_CUSTOM_FILES */

#endif /* LWIP_HTTPD_EXAMPLE_CUSTOMFILES */
