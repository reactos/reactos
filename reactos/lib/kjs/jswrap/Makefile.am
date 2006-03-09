#
# Automakefile for jswrap.
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

JSCOMPILER = ../src/js
JSCOMPILER_FLAGS = -Wpedantic -O2 -g

JSS 	= process.js main.js
JSCS	= process.jsc main.jsc

EXTRA_DIST = $(JSS)

bin_SCRIPTS = jswrap

compile: $(JSCS)

jswrap: $(JSS)
	rm -f $@.js
	for i in $(JSS); do cat $(srcdir)/$$i >> $@.js; done
	$(JSCOMPILER) $(JSCOMPILER_FLAGS) -c $@.js
	echo "#!$(bindir)/js --file" > $@
#	echo "#!$(JSCOMPILER) --file" > $@
	cat $@.jsc >> $@
	chmod a+x $@

CLEANFILES = jswrap $(JSCS)

SUFFIXES = .jsc .js

.js.jsc:
	$(JSCOMPILER) $(JSCOMPILER_FLAGS) -c $<
