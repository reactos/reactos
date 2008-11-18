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
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 David Gardiner <david.gardiner@unisa.edu.au>
#                 Joe Robins <jmrobins@tgix.com>
#                 Christopher Aillon <christopher@aillon.com>
#                 Gervase Markham <gerv@gerv.net>

use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::User;
use Bugzilla::BugMail;
use Bugzilla::Util;

# Just in case someone already has an account, let them get the correct footer
# on an error message. The user is logged out just after the account is
# actually created.
Bugzilla->login(LOGIN_OPTIONAL);

my $dbh = Bugzilla->dbh;
my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;
my $vars = {};

print $cgi->header();

# If we're using LDAP for login, then we can't create a new account here.
unless (Bugzilla->user->authorizer->user_can_create_account) {
    ThrowUserError("auth_cant_create_account");
}

my $createexp = Bugzilla->params->{'createemailregexp'};
unless ($createexp) {
    ThrowUserError("account_creation_disabled");
}

my $login = $cgi->param('login');

if (defined($login)) {
    $login = Bugzilla::User->check_login_name_for_creation($login);
    $vars->{'login'} = $login;

    if ($login !~ /$createexp/) {
        ThrowUserError("account_creation_disabled");
    }

    # Create and send a token for this new account.
    Bugzilla::Token::issue_new_user_account_token($login);

    $template->process("account/created.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

# Show the standard "would you like to create an account?" form.
$template->process("account/create.html.tmpl", $vars)
  || ThrowTemplateError($template->error());
