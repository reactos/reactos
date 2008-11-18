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
# Contributor(s): Frédéric Buclin <LpSolit@gmail.com>


##################
#Bugzilla Test 11#
##POD validation##

use strict;

use lib 't';

use Support::Files;
use Pod::Checker;

use Test::More tests => scalar(@Support::Files::testitems);

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

foreach my $file (@testitems) {
    $file =~ s/\s.*$//; # nuke everything after the first space (#comment)
    next if (!$file); # skip null entries
    my $error_count = podchecker($file, $fh);
    if ($error_count < 0) {
        ok(1,"$file does not contain any POD");
    } elsif ($error_count == 0) {
        ok(1,"$file has correct POD syntax");
    } else {
        ok(0,"$file has incorrect POD syntax --ERROR");
    }
}

exit 0;
