#!/usr/local/bin/perl -w
#
# Create link and execute definitions for switch dispatch method.
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
# $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/make-switch.pl,v $
# $Id: make-switch.pl,v 1.1 2004/01/10 20:38:18 arty Exp $
#

$linenumbers = 0;

$opcount = 0;

$target = "./c1switch.h";
open(CFP, ">$target") || die "couldn't create `$target': $!\n";

$target = "./c2switch.h";
open(CFP2, ">$target") || die "couldn't create `$target': $!\n";

$target = "./eswitch.h";
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

	#
	# Compilation phase 1.
	#
	print CFP "/* operand $operand ($opcount) */\n";
	print CFP "case $opcount:\n";
	print CFP "  SAVE_OP ($opcount);\n";

	if ($operand eq "const") {
	    # Patch const offsets.
	    printf CFP "  JS_BC_READ_INT32 (cp, i);\n";
	    printf CFP "  i += consts_offset;\n";
	    printf CFP "  SAVE_INT32 (i);\n";
	} elsif ($f_symbol) {
	    # Link symbols.
	    printf CFP "  JS_BC_READ_INT32 (cp, i);\n";
	    printf CFP "  i += consts_offset;\n";
	    printf CFP "  i = vm->consts[i].u.vsymbol;\n";
	    printf CFP "  SAVE_INT32 (i);\n";
	} else {
	    # Handle standard arguments.
	    if ($args =~ /0/) {
	    } elsif ($args =~ /1/) {
		print CFP "  JS_BC_READ_INT8 (cp, i);\n";
		print CFP "  SAVE_INT8 (i);\n";
	    } elsif ($args =~ /2/) {
		print CFP "  JS_BC_READ_INT16 (cp, i);\n";
		print CFP "  SAVE_INT16 (i);\n";
	    } elsif ($args =~ /4/) {
		print CFP "  JS_BC_READ_INT32 (cp, i);\n";
		print CFP "  SAVE_INT32 (i);\n";
	    } else {
		die("$ARGV:$.: illegal argument: $args\n");
	    }
	}

	print CFP "  cp += $args;\n";
	print CFP "  break;\n\n";
    } else {
	next;
    }

    #
    # Compilation phase 2.
    #
    if ($linenumbers) {
	printf CFP2 "#line %d \"%s\"\n", $. - 1, $ARGV;
    }
    print CFP2 "/* operand $operand ($opcount) */\n";
    print CFP2 "case $opcount:\n";
    print CFP2 "  cpos++;\n";
    if ($f_jump) {
	print CFP2 "  i = ARG_INT32 ();\n";
	if (0) {
	    print CFP2 "  printf(\"%s: delta=%d, cp=%d\\n\", \"$operand\", i, cp - code_start - 1);\n";
	}
	print CFP2 "  i = reloc[cp - fixed_code + 4 + i] - &f->code[cpos + 1];\n";
	if (0) {
	    print CFP2 "  printf (\"%s: newdelta=%d\\n\", \"$operand\", i);\n";
	}
	print CFP2 "  ARG_INT32 () = i;\n";
    }
    if ($args ne "0") {
	print CFP2 "  cp += $args;\n";
	print CFP2 "  cpos++;\n";
    }
    print CFP2 "  break;\n\n";

    #
    # Execution.
    #
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
close(CFP2);
close(EFP);
