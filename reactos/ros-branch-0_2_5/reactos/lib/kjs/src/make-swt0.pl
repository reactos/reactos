#!/usr/local/bin/perl -w
#
# Create link and execute definitions for switch-basic dispatch method.
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
#
#
# $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/make-swt0.pl,v $
# $Id: make-swt0.pl,v 1.1 2004/01/10 20:38:18 arty Exp $
#

$linenumbers = 0;

$opcount = 0;

$target = "./c1swt0.h";
open(CFP, ">$target") || die "couldn't create `$target': $!\n";

$target = "./eswt0.h";
open(EFP, ">$target") || die "couldn't create `$target': $!\n";

while (<>) {
    if (/operand\s+([a-zA-Z_][a-zA-Z0-9_]+)\s+([\S]+)\s+\{(.*)/) {
	$operand = $1;
	$args = $2;
	$flags = $3;

	$f_symbol = 0;
	if ($flags =~ /symbol/) {
	    $f_symbol = 1;
	}
	$f_jump = 0;
	if ($flags =~ /jump/) {
	    $f_jump = 1;
	}

	if ($linenumbers) {
	    printf CFP ("#line %d \"%s\"\n", $. - 1, $ARGV);
	}
	print CFP "/* operand $operand ($opcount) */\n";
	print CFP "case $opcount:\n";
	if ($operand eq "const") {
	    # Patch const offsets.
	    printf CFP "  JS_BC_READ_INT32 (cp, i);\n";
	    printf CFP "  i += consts_offset;\n";
	    printf CFP "  JS_BC_WRITE_INT32 (cp, i);\n";
	} elsif ($f_symbol) {
	    # Link symbols.
	    printf CFP "  JS_BC_READ_INT32 (cp, i);\n";
	    printf CFP "  i += consts_offset;\n";
	    printf CFP "  i = vm->consts[i].u.vsymbol;\n";
	    printf CFP "  JS_BC_WRITE_INT32 (cp, i);\n";
	}
	print CFP "  cp += $args;\n";
	print CFP "  break;\n\n";
    } else {
	next;
    }

    if ($linenumbers) {
	printf EFP "#line %d\"%s\"\n", $. - 1, $ARGV;
    }
    print EFP "/* operand $operand ($opcount) */\n";
    print EFP "case $opcount:\n";
    while (<>) {
	if (/^\}/) {
	    print EFP "  break;\n\n";
	    last;
	}
	print EFP;
    }

    $opcount++;
}
close(CFP);
close(EFP);
