#!/usr/local/bin/perl -w
#
# Opcode extractor.
# Copyright (c) 1997-1998 New Generation Software (NGS) Oy
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
#
#
# $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/make-op-def.pl,v $
# $Id: make-op-def.pl,v 1.1 2004/01/10 20:38:18 arty Exp $
#

$opcount = 0;

print "# name               \topcode\tdatasize\n";
print "# ----------------------------------------\n";

while (<>) {
    if (/operand\s+([a-zA-Z_][a-zA-Z0-9_]*)\s+([\S]+)\s+\{/) {
	printf ("%-20s\t$opcount\t$2\n", $1);
    } else {
	next;
    }
    $opcount++;
}
