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

package Bugzilla::Auth::Verify::DB;
use strict;
use base qw(Bugzilla::Auth::Verify);

use Bugzilla::Constants;
use Bugzilla::Token;
use Bugzilla::Util;
use Bugzilla::User;

sub check_credentials {
    my ($self, $login_data) = @_;
    my $dbh = Bugzilla->dbh;

    my $username = $login_data->{username};
    my $user_id  = login_to_id($username);

    return { failure => AUTH_NO_SUCH_USER } unless $user_id;

    $login_data->{bz_username} = $username;
    my $password = $login_data->{password};

    trick_taint($username);
    my ($real_password_crypted) = $dbh->selectrow_array(
        "SELECT cryptpassword FROM profiles WHERE userid = ?",
        undef, $user_id);

    # Using the internal crypted password as the salt,
    # crypt the password the user entered.
    my $entered_password_crypted = crypt($password, $real_password_crypted);
 
    return { failure => AUTH_LOGINFAILED }
        if $entered_password_crypted ne $real_password_crypted;

    # The user's credentials are okay, so delete any outstanding
    # password tokens they may have generated.
    Bugzilla::Token::DeletePasswordTokens($user_id, "user_logged_in");

    return $login_data;
}

sub change_password {
    my ($self, $user, $password) = @_;
    my $dbh = Bugzilla->dbh;
    my $cryptpassword = bz_crypt($password);
    $dbh->do("UPDATE profiles SET cryptpassword = ? WHERE userid = ?",
             undef, $cryptpassword, $user->id);
}

1;
