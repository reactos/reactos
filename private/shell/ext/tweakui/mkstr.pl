#
open(I, "strings.c") || die "Can't open strings.c ($!)";
open(O, ">strings.new") || die "Can't create strings.new ($!)";
open(H, ">strings.h") || die "Can't create strings.h ($!)";

while (<I>) {
    print O;
    last if $_ eq "/* DON'T EDIT ANYTHING AFTER THIS POINT; IT WILL BE NUKED BY THE PERL SCRIPT */\n";
    if ($incomment) {
	$incomment = $_ ne " */\n";
    } elsif ($_ eq "/*\n") {
	$incomment = 1;
    } elsif (/^#/) {
    } elsif (/^$/) {
    } else {
	chop;
	($str, $val) = /(\w+),\s*"(.*)"$/;
	$val =~ s/\\\\/\\/g;
	$vals{$str} = $val;
	push(@names, $str);
    }
}

#
#   Now study the string list to figure out how things can be combined.
#   Sort by length descending.
#

@names = sort { length($vals{$b}) <=> length($vals{$a}) } @names;

print H "/* DON'T EDIT ANYTHING AFTER THIS POINT; IT WILL BE NUKED BY THE PERL SCRIPT */\n";

#
#   Emit strings in length descending, coalescing if possible.
#   Record previously-seen strings in @seen to see if there's a match.
$ofs = 0;
for (@names) {
    $val = $vals{$_};
    for $this (keys(%pos)) {
	if ($val eq substr($vals{$this}, -length($val)) || $val eq '') {
	    $pos{$_} = $pos{$this} + length($vals{$this}) - length($val);
	    $val = '__NOTUSED__';
	    last;
	}
    }
    if ($val ne '__NOTUSED__') {
	push(@out, $val);
	$pos{$_} = $ofs;
	$ofs += length($val) + 1;
    }
    print H "#define $_ (c_rgtchCommon + $pos{$_})\n";
}
print H "extern TCHAR c_rgtchCommon[$ofs];\n";

close(H);

print O "#include \"tweakui.h\"\n";
print O "TCHAR c_rgtchCommon[$ofs] = { \n";
$v = 0;
for (@out) {
    for (split(//, $_)) {
	$v ^= ord($_);
	printf O "0x%02x,\n", $v;
    }
    printf O "0x%02x,\n", $v;
}
print O "};\n";

close(O);
close(I);
unlink("strings.bak");
rename("strings.new", "strings.c");
