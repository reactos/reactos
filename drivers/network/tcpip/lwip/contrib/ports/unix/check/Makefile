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

all compile: lwip_unittests
.PHONY: all clean check

LWIPDIR=../../../../src

# The include path to sys_arch.h and lwipopts.h must be first, so this must be before Common.mk
CFLAGS=-DLWIP_NOASSERT_ON_ERROR -I/usr/include/check -I$(LWIPDIR)/../test/unit

# Ignore 'too many arguments for format' warnings which happen with GCCs
# from check 0.15.2 on fail_if/fail_unless macros with text.
# See https://github.com/libcheck/check/pull/298/commits/82540c5428d3818b64d
CFLAGS+=-Wno-error=format-extra-args

ifeq (clang,$(findstring clang,$(CC)))
# check.h causes 'error: token pasting of ',' and __VA_ARGS__ is a GNU extension' with clang 9.0.0
CFLAGS+=-Wno-gnu-zero-variadic-macro-arguments
endif

# Prevent compiling sys_arch.c of unix port because unit test provide their own port
SYSARCH?=
include ../Common.mk

LDFLAGS:=-lcheck -lm $(LDFLAGS)

ifneq ($(UNAME_S),Darwin)
# check installed via brew on Darwin doesn't have a separate subunit library (must be statically linked)
LDFLAGS+=-lsubunit
endif

TESTDIR=$(LWIPDIR)/../test/unit
include $(TESTDIR)/Filelists.mk
TESTOBJS=$(notdir $(TESTFILES:.c=.o))

DEPFILES=.depend_test .depend_lwip .depend_app

clean:
	@rm -f *.o $(LWIPLIBCOMMON) $(APPLIB) lwip_unittests *.s $(DEPFILES) *.core core lwip_unittests.xml

depend dep: $(DEPFILES)
	@true

ifneq ($(MAKECMDGOALS),clean)
include $(DEPFILES)
endif

.depend_test: $(TESTFILES)
	$(CCDEP) $(CFLAGS) -MM $^ > .depend_test || rm -f .depend_test
.depend_lwip: $(LWIPFILES)
	$(CCDEP) $(CFLAGS) -MM $^ > .depend_lwip || rm -f .depend_lwip
.depend_app: $(APPFILES)
	$(CCDEP) $(CFLAGS) -MM $^ > .depend_app || rm -f .depend_app

ifneq ($(UNAME_S),Darwin)
# clang on Darwin doesn't support --start-group
lwip_unittests: $(DEPFILES) $(TESTOBJS) $(LWIPLIBCOMMON) $(APPLIB)
	$(CC) $(CFLAGS) -o lwip_unittests $(TESTOBJS)  -Wl,--start-group $(LWIPLIBCOMMON) $(APPLIB) $(LDFLAGS) -Wl,--end-group
else
lwip_unittests: $(DEPFILES) $(TESTOBJS) $(LWIPLIBCOMMON) $(APPLIB)
	$(CC) $(CFLAGS) -o lwip_unittests $(TESTOBJS) $(LWIPLIBCOMMON) $(APPLIB) $(LDFLAGS)
endif

check: lwip_unittests
	@./lwip_unittests
