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
# The Original Code is the Bugzilla Bug Tracking System.
#
# Contributor(s): Jacob Steenhagen <jake@bugzilla.org>

use strict;

use lib ".";

use Bugzilla;
use Bugzilla::Error;

Bugzilla->login();
my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;

###############################################################################
# Main Body Execution
###############################################################################

# This sidebar is currently for use with Mozilla based web browsers.
# Internet Explorer 6 is supposed to have a similar feature, but it
# most likely won't support XUL ;)  When that does come out, this
# can be expanded to output normal HTML for IE.  Until then, I like
# the way Scott's sidebar looks so I'm using that as the base for
# this file.
# http://bugzilla.mozilla.org/show_bug.cgi?id=37339

my $useragent = $ENV{HTTP_USER_AGENT};
if ($useragent =~ m:Mozilla/([1-9][0-9]*):i && $1 >= 5 && $useragent !~ m/compatible/i) {
    print $cgi->header("application/vnd.mozilla.xul+xml");
    # Generate and return the XUL from the appropriate template.
    $template->process("sidebar.xul.tmpl")
      || ThrowTemplateError($template->error());
} else {
    ThrowUserError("sidebar_supports_mozilla_only");
}
