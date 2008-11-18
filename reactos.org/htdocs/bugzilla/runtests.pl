#!/usr/bin/perl -w
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
# The Original Code is the Bugzilla Bug Tracking System. 
#
# The Initial Developer of the Original Code is Mike Norton.
# Portions created by the Initial Developer are Copyright (C) 2002
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#

# Make it harder for us to do dangerous things in Perl.
use diagnostics;
use strict;

use Test::Harness qw(&runtests $verbose);

$verbose = 0;
my $onlytest = "";

foreach (@ARGV) {
    if (/^(?:-v|--verbose)$/) {
        $verbose = 1;
    }
    else {
        $onlytest = sprintf("%0.3d",$_);
    }
}

runtests(glob("t/$onlytest*.t"));

