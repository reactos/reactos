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
# Contributor(s): Frédéric Buclin <LpSolit@gmail.com>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Error;

Bugzilla->login();

my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;

# Return the appropriate HTTP response headers.
print $cgi->header('application/xml');

$template->process("search/search-plugin.xml.tmpl")
  || ThrowTemplateError($template->error());
