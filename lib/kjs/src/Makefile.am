#
# Automakefile for JavaScript virtual machine.
# Copyright (c) 1998 New Generation Software (NGS) Oy
#
# Author: Markku Rossi <mtr@ngs.fi>
#

#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
# MA 02111-1307, USA
#

SUBDIRS = tests

# CFLAGS = -g -O2 -Wall -Wno-unused

lib_LTLIBRARIES = libjs.la
include_HEADERS = js.h jsint.h

libjs_la_SOURCES = bc.c heap.c object.c debug.c compiler.c js.c vm.c \
		vmswitch.c vmjumps.c alloc.c utils.c mrgsort.c \
		regex.c crc32.c iostream.c \
		b_array.c b_bool.c b_func.c b_core.c b_number.c \
		b_object.c b_string.c b_date.c b_dir.c b_file.c b_math.c \
		b_regexp.c b_system.c b_vm.c

libjs_la_LIBADD = @INTERPRETER_FEATURES@ @EXTENSIONS@
libjs_la_DEPENDENCIES = $(libjs_la_LIBADD)
EXTRA_libjs_la_SOURCES = vmswt0.c r_std.c r_pthrs.c dl_open.c dl_dummy.c \
	xjs.c xcurses.c xmd5.c md5c.c

man_MANS = js.1

bin_PROGRAMS = js

js_SOURCES = main.c getopt.c getopt1.c

js_LDADD = libjs.la @EXTENSIONS_LIBS@ -lm

# noinst_PROGRAMS = errtest
# errtest_SOURCES = errtest.c
# errtest_LDADD = libjs.a @EXTENSIONS_LIBS@ -lm

noinst_HEADERS = c1swt0.h eswt0.h c1switch.h c2switch.h eswitch.h \
	c1jumps.h c2jumps.h ejumps.h rentrant.h \
	getopt.h mrgsort.h regex.h md5.h

EXTRA_DIST = operands.def make-swt0.pl make-switch.pl make-jumps.pl \
	make-op-def.pl make-data.c js.1

operands:
	./make-op-def.pl operands.def > op.def
	./make-swt0.pl operands.def
	./make-switch.pl operands.def
	./make-jumps.pl operands.def

compiler-source: make-data
	./make-data js_compiler_bytecode ../jsc/compiler.jsc compiler.c

make-data: make-data.o
	$(CC) $(ALLLDFLAGS) make-data.o -o make-data

wc:
	wc alloc.c b_*.c bc.c debug.c dl_*.c heap.c iostream.c js.c	\
	main.c mrgsort.c object.c r_*.c utils.c vm.c vmjumps.c		\
	vmswitch.c vmswt0.c xcurses.c xjs.c xmd5.c js.h jsint.h		\
	mrgsort.h rentrant.h operands.def
