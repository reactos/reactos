# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
# 
# The Original Code are the Bugzilla Tests.
# 
# The Initial Developer of the Original Code is Zach Lipton
# Portions created by Zach Lipton are 
# Copyright (C) 2002 Zach Lipton.  All
# Rights Reserved.
# 
# Contributor(s): Zach Lipton <zach@zachlipton.com>


#################
#Bugzilla Test 6#
####Spelling#####

use lib 't';
use Support::Files;

BEGIN { # yes the indenting is off, deal with it
#add the words to check here:
@evilwords = qw(
anyways
arbitary
databasa
dependan
existance
existant
paramater
varsion
);

$testcount = scalar(@Support::Files::testitems);
}

use Test::More tests => $testcount;

# Capture the TESTOUT from Test::More or Test::Builder for printing errors.
# This will handle verbosity for us automatically.
my $fh;
{
    local $^W = 0;  # Don't complain about non-existent filehandles
    if (-e \*Test::More::TESTOUT) {
        $fh = \*Test::More::TESTOUT;
    } elsif (-e \*Test::Builder::TESTOUT) {
        $fh = \*Test::Builder::TESTOUT;
    } else {
        $fh = \*STDOUT;
    }
}

my @testitems = @Support::Files::testitems;

# at last, here we actually run the test...
my $evilwordsregexp = join('|', @evilwords);

foreach my $file (@testitems) {
    $file =~ s/\s.*$//; # nuke everything after the first space (#comment)
    next if (!$file); # skip null entries

    if (open (FILE, $file)) { # open the file for reading

        my $found_word = '';

        while (my $file_line = <FILE>) { # and go through the file line by line
            if ($file_line =~ /($evilwordsregexp)/i) { # found an evil word
                $found_word = $1;
                last;
            }
        }
            
        close (FILE);
            
        if ($found_word) {
            ok(0,"$file: found SPELLING ERROR $found_word --WARNING");
        } else {
            ok(1,"$file does not contain registered spelling errors");
        }
    } else {
        ok(0,"could not open $file for spellcheck --WARNING");
    }
} 

exit 0;
