#
# Automakefile for JS interpreter.
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

SUBDIRS = src jsc jsas jsdas jswrap docs micros examples projects

include_HEADERS = jsconfig.h

ACLOCAL_AMFLAGS = -I $(srcdir)/am @ACLOCAL_FLAGS_FOR_LIBTOOL@

dist-hook:
	mkdir $(distdir)/am
	cp -p $(srcdir)/am/pthreads.m4 $(distdir)/am/pthreads.m4
	mkdir $(distdir)/am.opt
	cp -p $(srcdir)/am.opt/libtool.m4 $(distdir)/am.opt/libtool.m4
