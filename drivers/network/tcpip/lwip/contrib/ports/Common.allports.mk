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

#CC=gcc
#CC=clang
CCDEP?=$(CC)

CFLAGS+=-g -DLWIP_DEBUG -Wall -pedantic -Werror \
	-Wparentheses -Wsequence-point -Wswitch-default \
	-Wextra -Wundef -Wshadow -Wpointer-arith -Wcast-qual \
	-Wc++-compat -Wwrite-strings -Wold-style-definition -Wcast-align \
	-Wmissing-prototypes -Wnested-externs \
	-Wunreachable-code -Wuninitialized -Wmissing-prototypes \
	-Wredundant-decls -Waggregate-return -Wlogical-not-parentheses
#	-Wconversion -Wsign-compare -Wmissing-include-dirs

ifeq (,$(findstring clang,$(CC)))
CFLAGS+= -Wlogical-op -Wc90-c99-compat -Wtrampolines
# if GCC is newer than 4.8/4.9 you may use:
#CFLAGS:=$(CFLAGS) -fsanitize=address -fstack-protector -fstack-check -fsanitize=undefined -fno-sanitize=alignment
else
# we cannot sanitize alignment on x86-64 targets because clang wants 64 bit alignment
CFLAGS+= -fsanitize=address -fsanitize=undefined -fno-sanitize=alignment -Wdocumentation -Wno-documentation-deprecated-sync
endif

CONTRIBDIR?=../../..
ARFLAGS?=rs

#Set this to where you have the lwip core module checked out from git
#default assumes it's a dir above the contrib module
LWIPDIR?=$(CONTRIBDIR)/..

CFLAGS+=-I. \
	-I$(CONTRIBDIR) \
	-I$(LWIPDIR)/include \
	-I$(LWIPARCH)/include

# Add include path and link to mbedTLS lib if available
MBEDTLSDIR?=$(LWIPDIR)/../mbedtls
ifneq (,$(wildcard $(MBEDTLSDIR)/include/mbedtls/*.h))
LDFLAGS+=-L$(MBEDTLSDIR)/library -lmbedtls -lmbedcrypto -lmbedx509
CFLAGS+=-I$(MBEDTLSDIR)/include -Wno-redundant-decls -DLWIP_HAVE_MBEDTLS=1 -Wno-c90-c99-compat
endif

include $(CONTRIBDIR)/Filelists.mk
include $(LWIPDIR)/Filelists.mk

# LWIPFILES: All the above.
LWIPFILES=$(LWIPNOAPPSFILES) $(ARCHFILES)
LWIPOBJS=$(notdir $(LWIPFILES:.c=.o))

LWIPLIBCOMMON=liblwipcommon.a
$(LWIPLIBCOMMON): $(LWIPOBJS)
	$(AR) $(ARFLAGS) $(LWIPLIBCOMMON) $?

APPFILES=$(CONTRIBAPPFILES) $(LWIPAPPFILES)
APPLIB=liblwipapps.a
APPOBJS=$(notdir $(APPFILES:.c=.o))
$(APPLIB): $(APPOBJS)
	$(AR) $(ARFLAGS) $(APPLIB) $?

%.o:
	$(CC) $(CFLAGS) -c $<
