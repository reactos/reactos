#
# Automakefile for JavaScript JavaScript compiler.
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

JSCOMPILER = ../src/js
JSCOMPILER_FLAGS = -Wpedantic -O2

JSS	= defs.js lexer.js parser.js gram.js namespace.js \
	  streams.js asm.js entry.js
JSCS	= defs.jsc lexer.jsc parser.jsc gram.jsc namespace.jsc \
	  streams.jsc asm.jsc entry.jsc

EXTRA_DIST = $(JSS) $(JSCS) bs.js bs.jsc compiler.js compiler.jsc

compile: $(JSCS) bs.jsc

compiler: compiler.jsc

compiler.jsc: compiler.js
	$(JSCOMPILER) $(JSCOMPILER_FLAGS) -c $<
	cp compiler.jsc bak/compiler-`date | sed 's/ /-/g'`.jsc || true

compiler.js: $(JSS)
	cat $(JSS) >compiler.js

run: compile
	$(JSCOMPILER) --load $(JSCS) --file bs.jsc

wc:
	wc $(JSS)

my-clean:
	rm -f $(JSCS) bs.jsc

SUFFIXES = .jsc .js

.js.jsc:
	$(JSCOMPILER) $(JSCOMPILER_FLAGS) -g -c $<
