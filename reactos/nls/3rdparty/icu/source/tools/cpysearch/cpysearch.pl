#!/usr/bin/perl -w
#  ***********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002-2007, International Business Machines Corporation
#  * and others. All Rights Reserved.
#  ***********************************************************************
#
# Search for files modified this year, that need to have copyright indicating
# this current year on them.
#
use strict;

my $icuSource = $ARGV[0];
my $ignore = "CVS|\\.svn|\\~|\\#|Debug|Release|\\.dll|\\.ilk|\\.idb|\\.pdb|\\.dsp|\\.dsw|\\.opt|\\.ncb|\\.vcproj|\\.sln|\\.suo|\\.cvsignore|\\.cnv|\\.res|\\.icu|\\.exe|\\.obj|\\.bin|\\.exp|\\.lib|\\.out|\\.plg|positions|unidata|\\.jar|\\.spp|\\.stub|\\.policy|\\.ttf|\\.TTF|\\.otf";

my ($sec, $min, $hour, , $day, $mon, $year, $wday, $yday, $isdst) = localtime;
$year += 1900;

my $command = "find $icuSource -type f -mtime -$yday | fgrep -v -f cpyskip.txt";
my @files = `$command`;
@files = grep(!/$ignore/, @files);
my $file;
foreach $file (@files) {
  chomp $file;
  my @lines = `head -n 20 "$file"`;
  if (grep(/copyright.*$year/i, @lines) == 0) {
    print "$file\n";
  }
}
