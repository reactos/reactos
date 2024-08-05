/**
 * @file
 * HTTPD simple CGI example
 *
 * This file demonstrates how to add support for basic CGI.
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
#include "cgi_example.h"

#include "lwip/apps/httpd.h"

#include "lwip/def.h"
#include "lwip/mem.h"

#include <stdio.h>
#include <string.h>

/** define LWIP_HTTPD_EXAMPLE_CGI_SIMPLE to 1 to enable this cgi example */
#ifndef LWIP_HTTPD_EXAMPLE_CGI_SIMPLE
#define LWIP_HTTPD_EXAMPLE_CGI_SIMPLE 0
#endif

#if LWIP_HTTPD_EXAMPLE_CGI_SIMPLE

#if !LWIP_HTTPD_CGI
#error LWIP_HTTPD_EXAMPLE_CGI_SIMPLE needs LWIP_HTTPD_CGI
#endif

static const char *cgi_handler_basic(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

static const tCGI cgi_handlers[] = {
  {
    "/basic_cgi",
    cgi_handler_basic
  },
  {
    "/basic_cgi_2",
    cgi_handler_basic
  }
};

void
cgi_ex_init(void)
{
  http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
}

/** This basic CGI function can parse param/value pairs and return an url that
 * is sent as a response by httpd.
 *
 * This example function just checks that the input url has two key value
 * parameter pairs: "foo=bar" and "test=123"
 * If not, it returns 404
 */
static const char *
cgi_handler_basic(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  LWIP_ASSERT("check index", iIndex < LWIP_ARRAYSIZE(cgi_handlers));

  if (iNumParams == 2) {
    if (!strcmp(pcParam[0], "foo")) {
      if (!strcmp(pcValue[0], "bar")) {
        if (!strcmp(pcParam[1], "test")) {
          if (!strcmp(pcValue[1], "123")) {
            return "/index.html";
          }
        }
      }
    }
  }
  return "/404.html";
}

#endif /* LWIP_HTTPD_EXAMPLE_CGI_SIMPLE */
