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

package Bugzilla::Auth::Persist::Cookie;
use strict;
use fields qw();

use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Token;

use List::Util qw(first);

sub new {
    my ($class) = @_;
    my $self = fields::new($class);
    return $self;
}

sub persist_login {
    my ($self, $user) = @_;
    my $dbh = Bugzilla->dbh;
    my $cgi = Bugzilla->cgi;

    my $ip_addr = $cgi->remote_addr;
    unless ($cgi->param('Bugzilla_restrictlogin') ||
            Bugzilla->params->{'loginnetmask'} == 32) 
    {
        $ip_addr = get_netaddr($ip_addr);
    }

    # The IP address is valid, at least for comparing with itself in a
    # subsequent login
    trick_taint($ip_addr);

    my $login_cookie = 
        Bugzilla::Token::GenerateUniqueToken('logincookies', 'cookie');

    $dbh->do("INSERT INTO logincookies (cookie, userid, ipaddr, lastused)
              VALUES (?, ?, ?, NOW())",
              undef, $login_cookie, $user->id, $ip_addr);

    # Remember cookie only if admin has told so
    # or admin didn't forbid it and user told to remember.
    if ( Bugzilla->params->{'rememberlogin'} eq 'on' ||
         (Bugzilla->params->{'rememberlogin'} ne 'off' &&
          $cgi->param('Bugzilla_remember') &&
          $cgi->param('Bugzilla_remember') eq 'on') ) 
    {
        $cgi->send_cookie(-name => 'Bugzilla_login',
                          -value => $user->id,
                          -expires => 'Fri, 01-Jan-2038 00:00:00 GMT');
        $cgi->send_cookie(-name => 'Bugzilla_logincookie',
                          -value => $login_cookie,
                          -expires => 'Fri, 01-Jan-2038 00:00:00 GMT');

    }
    else {
        $cgi->send_cookie(-name => 'Bugzilla_login',
                          -value => $user->id);
        $cgi->send_cookie(-name => 'Bugzilla_logincookie',
                          -value => $login_cookie);
    }
}

sub logout {
    my ($self, $param) = @_;

    my $dbh = Bugzilla->dbh;
    my $cgi = Bugzilla->cgi;
    $param = {} unless $param;
    my $user = $param->{user} || Bugzilla->user;
    my $type = $param->{type} || LOGOUT_ALL;

    if ($type == LOGOUT_ALL) {
        $dbh->do("DELETE FROM logincookies WHERE userid = ?",
                 undef, $user->id);
        return;
    }

    # The LOGOUT_*_CURRENT options require the current login cookie.
    # If a new cookie has been issued during this run, that's the current one.
    # If not, it's the one we've received.
    my $cookie = first {$_->name eq 'Bugzilla_logincookie'}
                       @{$cgi->{'Bugzilla_cookie_list'}};
    my $login_cookie;
    if ($cookie) {
        $login_cookie = $cookie->value;
    }
    else {
        $login_cookie = $cgi->cookie("Bugzilla_logincookie");
    }
    trick_taint($login_cookie);

    # These queries use both the cookie ID and the user ID as keys. Even
    # though we know the userid must match, we still check it in the SQL
    # as a sanity check, since there is no locking here, and if the user
    # logged out from two machines simultaneously, while someone else
    # logged in and got the same cookie, we could be logging the other
    # user out here. Yes, this is very very very unlikely, but why take
    # chances? - bbaetz
    if ($type == LOGOUT_KEEP_CURRENT) {
        $dbh->do("DELETE FROM logincookies WHERE cookie != ? AND userid = ?",
                 undef, $login_cookie, $user->id);
    } elsif ($type == LOGOUT_CURRENT) {
        $dbh->do("DELETE FROM logincookies WHERE cookie = ? AND userid = ?",
                 undef, $login_cookie, $user->id);
    } else {
        die("Invalid type $type supplied to logout()");
    }

    if ($type != LOGOUT_KEEP_CURRENT) {
        clear_browser_cookies();
    }

}

sub clear_browser_cookies {
    my $cgi = Bugzilla->cgi;
    $cgi->remove_cookie('Bugzilla_login');
    $cgi->remove_cookie('Bugzilla_logincookie');
}

1;
