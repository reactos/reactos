#!/usr/bin/perl -wT
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
# Contributor(s): Joel Peshkin <bugreport@peshkin.net>

# This script is used by servertest.pl to confirm that cgi scripts
# are being run instead of shown. This script does not rely on database access
# or correct params.

use strict;
print "content-type:text/plain\n\n";
print "OK " . ($::ENV{MOD_PERL} || "mod_cgi") . "\n";
exit;

