#!/usr/bin/perl -w
#  ***********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002-2007, International Business Machines Corporation
#  * and others. All Rights Reserved.
#  ***********************************************************************
# 
#  Search for and list files which don't have a copyright notice, and should.
#
use strict;

my $icuSource = $ARGV[0];
my $ignore = "data/out/build|CVS|\\.svn|\\~|\\#|Debug|Release|\\.dll|\\.ilk|\\.idb|\\.pdb|\\.dsp|\\.dsw|\\.opt|\\.ncb|\\.vcproj|\\.sln|\\.suo|\\.cvsignore|\\.cnv|\\.res|\\.icu|\\.exe|\\.obj|\\.bin|\\.exp|\\.lib|\\.out|\\.plg|positions|unidata|\\.jar|\\.spp|\\.stub|\\.policy|\\.otf|\\.ttf|\\.TTF";

my $command = "find $icuSource -type f | fgrep -v -f cpyskip.txt";
my @files = `$command`;
@files = grep(!/$ignore/, @files);
my $file;
foreach $file (@files) {
  chomp $file;
  my @lines = `head -n 20 "$file"`;
  if (grep(/copyright.*(international|ibm)/i, @lines) == 0) {
    print "$file\n";
  }
}
