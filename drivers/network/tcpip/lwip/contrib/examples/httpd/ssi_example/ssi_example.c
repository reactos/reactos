/**
 * @file
 * HTTPD simple SSI example
 *
 * This file demonstrates how to add support for SSI.
 * It does this in a very simple way by providing the three tags 'HelloWorld'
 * 'counter', and 'MultiPart'.
 *
 * This file also demonstrates how to integrate CGI with SSI.
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
#include "ssi_example.h"

#include "lwip/apps/httpd.h"

#include "lwip/def.h"
#include "lwip/mem.h"

#include <stdio.h>
#include <string.h>

/** define LWIP_HTTPD_EXAMPLE_SSI_SIMPLE to 1 to enable this ssi example*/
#ifndef LWIP_HTTPD_EXAMPLE_SSI_SIMPLE
#define LWIP_HTTPD_EXAMPLE_SSI_SIMPLE 0
#endif

/** define LWIP_HTTPD_EXAMPLE_SSI_SIMPLE_CGI_INTEGRATION to 1 to show how to
 * integrate CGI into SSI (LWIP_HTTPD_CGI_SSI) */
#ifndef LWIP_HTTPD_EXAMPLE_SSI_SIMPLE_CGI_INTEGRATION
#define LWIP_HTTPD_EXAMPLE_SSI_SIMPLE_CGI_INTEGRATION 0
#endif

#if LWIP_HTTPD_EXAMPLE_SSI_SIMPLE

#if LWIP_HTTPD_EXAMPLE_SSI_SIMPLE_CGI_INTEGRATION
#if !LWIP_HTTPD_FILE_STATE
#error LWIP_HTTPD_EXAMPLE_SSI_SIMPLE_CGI_INTEGRATION needs LWIP_HTTPD_FILE_STATE
#endif
#if !LWIP_HTTPD_CGI_SSI
#error LWIP_HTTPD_EXAMPLE_SSI_SIMPLE_CGI_INTEGRATION needs LWIP_HTTPD_CGI_SSI
#endif

#define MAX_CGI_LEN   16
#endif

const char * ssi_example_tags[] = {
  "HellWorl",
  "counter",
  "MultPart"
#if LWIP_HTTPD_EXAMPLE_SSI_SIMPLE_CGI_INTEGRATION
  ,"CgiParam"
#endif
};

u16_t ssi_example_ssi_handler(
#if LWIP_HTTPD_SSI_RAW
                             const char* ssi_tag_name,
#else /* LWIP_HTTPD_SSI_RAW */
                             int iIndex,
#endif /* LWIP_HTTPD_SSI_RAW */
                             char *pcInsert, int iInsertLen
#if LWIP_HTTPD_SSI_MULTIPART
                             , u16_t current_tag_part, u16_t *next_tag_part
#endif /* LWIP_HTTPD_SSI_MULTIPART */
#if defined(LWIP_HTTPD_FILE_STATE) && LWIP_HTTPD_FILE_STATE
                             , void *connection_state
#endif /* LWIP_HTTPD_FILE_STATE */
                             )
{
  size_t printed;
#if LWIP_HTTPD_SSI_RAW
  /* a real application could use if(!strcmp) blocks here, but we want to keep
     the differences between configurations small, so translate string to index here */
  int iIndex;
  for (iIndex = 0; iIndex < LWIP_ARRAYSIZE(ssi_example_tags); iIndex++) {
    if(!strcmp(ssi_tag_name, ssi_example_tags[iIndex])) {
      break;
    }
  }
#endif
#if defined(LWIP_HTTPD_FILE_STATE) && LWIP_HTTPD_FILE_STATE
  LWIP_UNUSED_ARG(connection_state);
#endif

  switch (iIndex) {
  case 0: /* "HelloWorld" */
    printed = snprintf(pcInsert, iInsertLen, "Hello World!");
    break;
  case 1: /* "counter" */
    {
      static int counter;
      counter++;
      printed = snprintf(pcInsert, iInsertLen, "%d", counter);
    }
    break;
  case 2: /* "MultPart" */
#if LWIP_HTTPD_SSI_MULTIPART
    switch (current_tag_part) {
    case 0:
      printed = snprintf(pcInsert, iInsertLen, "part0");
      *next_tag_part = 1;
      break;
    case 1:
      printed = snprintf(pcInsert, iInsertLen, "part1");
      *next_tag_part = 2;
      break;
    case 2:
      printed = snprintf(pcInsert, iInsertLen, "part2");
      break;
    default:
      printed = snprintf(pcInsert, iInsertLen, "unhandled part: %d", (int)current_tag_part);
      break;
    }
#else
    printed = snprintf(pcInsert, iInsertLen, "LWIP_HTTPD_SSI_MULTIPART disabled");
#endif
    break;
#if LWIP_HTTPD_EXAMPLE_SSI_SIMPLE_CGI_INTEGRATION
  case 3:
    if (connection_state) {
      char *params = (char *)connection_state;
      if (*params) {
        printed = snprintf(pcInsert, iInsertLen, "%s", (char *)params);
      } else {
        printed = snprintf(pcInsert, iInsertLen, "none");
      }
    } else {
       printed = snprintf(pcInsert, iInsertLen, "NULL");
    }
    break;
#endif
  default: /* unknown tag */
    printed = 0;
    break;
  }
  LWIP_ASSERT("sane length", printed <= 0xFFFF);
  return (u16_t)printed;
}

void
ssi_ex_init(void)
{
  int i;
  for (i = 0; i < LWIP_ARRAYSIZE(ssi_example_tags); i++) {
    LWIP_ASSERT("tag too long for LWIP_HTTPD_MAX_TAG_NAME_LEN",
      strlen(ssi_example_tags[i]) <= LWIP_HTTPD_MAX_TAG_NAME_LEN);
  }

  http_set_ssi_handler(ssi_example_ssi_handler,
#if LWIP_HTTPD_SSI_RAW
    NULL, 0
#else
    ssi_example_tags, LWIP_ARRAYSIZE(ssi_example_tags)
#endif
    );
}

#if LWIP_HTTPD_EXAMPLE_SSI_SIMPLE_CGI_INTEGRATION
void *
fs_state_init(struct fs_file *file, const char *name)
{
  char *ret;
  LWIP_UNUSED_ARG(file);
  LWIP_UNUSED_ARG(name);
  ret = (char *)mem_malloc(MAX_CGI_LEN);
  if (ret) {
    *ret = 0;
  }
  return ret;
}

void
fs_state_free(struct fs_file *file, void *state)
{
  LWIP_UNUSED_ARG(file);
  if (state != NULL) {
    mem_free(state);
  }
}

void
httpd_cgi_handler(struct fs_file *file, const char* uri, int iNumParams,
                              char **pcParam, char **pcValue
#if defined(LWIP_HTTPD_FILE_STATE) && LWIP_HTTPD_FILE_STATE
                                     , void *connection_state
#endif /* LWIP_HTTPD_FILE_STATE */
                                     )
{
  LWIP_UNUSED_ARG(file);
  LWIP_UNUSED_ARG(uri);
  if (connection_state != NULL) {
    char *start = (char *)connection_state;
    char *end = start + MAX_CGI_LEN;
    int i;
    memset(start, 0, MAX_CGI_LEN);
    /* print a string of the arguments: */
    for (i = 0; i < iNumParams; i++) {
      size_t len;
      len = end - start;
      if (len) {
        size_t inlen = strlen(pcParam[i]);
        size_t copylen = LWIP_MIN(inlen, len);
        memcpy(start, pcParam[i], copylen);
        start += copylen;
        len -= copylen;
      }
      if (len) {
        *start = '=';
        start++;
        len--;
      }
      if (len) {
        size_t inlen = strlen(pcValue[i]);
        size_t copylen = LWIP_MIN(inlen, len);
        memcpy(start, pcValue[i], copylen);
        start += copylen;
        len -= copylen;
      }
      if (len) {
        *start = ';';
        len--;
      }
      /* ensure NULL termination */
      end--;
      *end = 0;
    }
  }
}
#endif /* LWIP_HTTPD_EXAMPLE_SSI_SIMPLE_CGI_INTEGRATION */

#endif /* LWIP_HTTPD_EXAMPLE_SSI_SIMPLE */
