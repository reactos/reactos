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
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Joe Robins <jmrobins@tgix.com>
#                 Dave Miller <justdave@syndicomm.com>
#                 Christopher Aillon <christopher@aillon.com>
#                 Gervase Markham <gerv@gerv.net>
#                 Christian Reis <kiko@async.com.br>
#                 Bradley Baetz <bbaetz@acm.org>
#                 Erik Stambaugh <erik@dasbistro.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

package Bugzilla::Auth::Login::CGI;
use strict;
use base qw(Bugzilla::Auth::Login);
use constant user_can_create_account => 1;

use Bugzilla::Constants;
use Bugzilla::WebService::Constants;
use Bugzilla::Util;
use Bugzilla::Error;

sub get_login_info {
    my ($self) = @_;
    my $cgi = Bugzilla->cgi;

    my $username = trim($cgi->param("Bugzilla_login"));
    my $password = $cgi->param("Bugzilla_password");

    $cgi->delete('Bugzilla_login', 'Bugzilla_password');

    if (!defined $username || !defined $password) {
        return { failure => AUTH_NODATA };
    }

    return { username => $username, password => $password };
}

sub fail_nodata {
    my ($self) = @_;
    my $cgi = Bugzilla->cgi;
    my $template = Bugzilla->template;

    if (Bugzilla->error_mode == Bugzilla::Constants::ERROR_MODE_DIE_SOAP_FAULT) {
        die SOAP::Fault
            ->faultcode(ERROR_AUTH_NODATA)
            ->faultstring('Login Required');
    }

    # Redirect to SSL if required
    if (Bugzilla->params->{'sslbase'} ne '' 
        and Bugzilla->params->{'ssl'} ne 'never') 
    {
        $cgi->require_https(Bugzilla->params->{'sslbase'});
    }
    print $cgi->header();
    $template->process("account/auth/login.html.tmpl",
                       { 'target' => $cgi->url(-relative=>1) }) 
        || ThrowTemplateError($template->error());
    exit;
}

1;
