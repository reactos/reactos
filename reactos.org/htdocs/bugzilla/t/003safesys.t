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
# Copyright (C) 2001 Zach Lipton.  All
# Rights Reserved.
# 
# Contributor(s): Zach Lipton <zach@zachlipton.com>


#################
#Bugzilla Test 3#
###Safesystem####

use strict;

use lib 't';

use Support::Files;

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
my $perlapp = "\"$^X\"";

foreach my $file (@testitems) {
    $file =~ s/\s.*$//; # nuke everything after the first space (#comment)
    next if (!$file); # skip null entries
    my $command = "$perlapp -c -It -MSupport::Systemexec $file 2>&1";
    my $loginfo=`$command`;
    if ($loginfo =~ /arguments for Support::Systemexec::(system|exec)/im) {
        ok(0,"$file DOES NOT use proper system or exec calls");
        print $fh $loginfo;
    } else {
        ok(1,"$file uses proper system and exec calls");
    }
}

exit 0;
