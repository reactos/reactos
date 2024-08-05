#
# Copyright (c) 2001, 2002 Swedish Institute of Computer Science.
# All rights reserved. 
# 
# Redistribution and use in source and binary forms, with or without modification, 
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
# SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
# OF SUCH DAMAGE.
#
# This file is part of the lwIP TCP/IP stack.
# 
# Author: Adam Dunkels <adam@sics.se>
#

# Architecture specific files.
LWIPARCH?=$(CONTRIBDIR)/ports/unix/port
SYSARCH?=$(LWIPARCH)/sys_arch.c
ARCHFILES=$(LWIPARCH)/perf.c \
  $(SYSARCH) \
	$(LWIPARCH)/netif/tapif.c \
	$(LWIPARCH)/netif/list.c \
	$(LWIPARCH)/netif/sio.c \
	$(LWIPARCH)/netif/fifo.c

UNIX_COMMON_MK_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(UNIX_COMMON_MK_DIR)../Common.allports.mk

LDFLAGS+=-lutil

UNAME_S:= $(shell uname -s)
ifneq ($(UNAME_S),Darwin)
# Darwin doesn't have pthreads or POSIX real-time extensions libs
LDFLAGS+=-pthread -lrt
endif
