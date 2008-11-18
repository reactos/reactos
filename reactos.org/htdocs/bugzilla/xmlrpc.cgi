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
# Contributor(s): Marc Schumann <wurblzap@gmail.com>

use strict;
use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;

# Use an eval here so that runtests.pl accepts this script even if SOAP-Lite
# is not installed.
eval 'use XMLRPC::Transport::HTTP;
      use Bugzilla::WebService;';
$@ && ThrowCodeError('soap_not_installed');

Bugzilla->usage_mode(Bugzilla::Constants::USAGE_MODE_WEBSERVICE);

my $response = Bugzilla::WebService::XMLRPC::Transport::HTTP::CGI
    ->dispatch_with({'Bugzilla' => 'Bugzilla::WebService::Bugzilla',
                     'Bug'      => 'Bugzilla::WebService::Bug',
                     'User'     => 'Bugzilla::WebService::User',
                     'Product'  => 'Bugzilla::WebService::Product',
                    })
    ->on_action(\&Bugzilla::WebService::handle_login)
    ->handle;
