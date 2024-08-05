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

CC=gcc

# Architecture specific files.
LWIPARCH?=$(CONTRIBDIR)/ports/win32
SYSARCH?=$(LWIPARCH)/sys_arch.c
ARCHFILES=$(SYSARCH) $(LWIPARCH)/pcapif.c \
	$(LWIPARCH)/pcapif_helper.c $(LWIPARCH)/sio.c

WIN32_COMMON_MK_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(WIN32_COMMON_MK_DIR)/../Common.allports.mk

PCAPDIR=$(PCAP_DIR)/Include
LDFLAGS+=-L$(PCAP_DIR)/lib -lwpcap -lpacket
# -Wno-format: GCC complains about non-standard 64 bit modifier needed for MSVC runtime
CFLAGS+=-I$(PCAPDIR) -Wno-format

pcapif.o:
	$(CC) $(CFLAGS) -Wno-error -Wno-redundant-decls -c $(<:.o=.c)
pcapif_helper.o:
	$(CC) $(CFLAGS) -std=c99 -Wno-redundant-decls -c $(<:.o=.c)
